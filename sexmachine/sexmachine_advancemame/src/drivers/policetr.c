/***************************************************************************

    P&P Marketing Police Trainer hardware

    driver by Aaron Giles

    Games supported:
        * Police Trainer
        * Sharpshooter

    Known bugs:
        * Flip screen not supported

Note:   Police Trainer v1.3B runs on the same revision PCB as Sharpshooter - Rev 0.5B
        If you hold the test button down and boot the game, all program roms
        fail the checksum.  However, each checksum listed matches the checksum
        printed on the ROM label.  This has been verified on an original PCB.
Note:   Police Trainer v1.0 (Rev 0.2 PCB), the checksum results in MAME have been
        verified to be the same as an original PCB.

To ID the version of your SharpShooter, check the 2nd printed line on each type of ROM.

Program Roms:  C121012 - Code version 1.2, Graphics v1.0 & Sound v1.2
Graphic Roms:  G10     - Graphics rom v1.0 (in diagnostics mode it's called "Art")
  Sound Roms:  S12     - Sound rom v1.2

Noted differences in versions of SharpShooter:
 Initial High Score names are changed between v1.1 and v1.2
  Circus of Mystery:
    The ballon challenge has been rewritten for v1.7
    Jugglers throw balls painted with targets for v1.1 & v1.2  Version 1.7 uses regular targets
  Alien Encounter:
    First saucer challenge has been modified for v1.7

The ATTILA Video System PCB (by EXIT Entertainment):

Sharpshooter PCB is Rev 0.5B
Police Trainer PCB is Rev 0.3 / Rev 0.2

|------------JAMMA Connector------------|
|                     CN7               |
| GUN1   XILINX-1              93C66    |
| GUN2                                  |
|                                       |
| LED1 LED2         IDT71024 x 2  Bt481 |
|             AT001                     |
|  DSW(8)                               |
|U127                       U113    U162|
|U125  IDT71256 x 4         U112        |
|U123                       U111    U160|
|U121                       U110        |
|U126                                   |
|U124  OSC    IDT79R3041  XILINX-2      |
|U122  48.000MHz             XILINX-3   |
|U120                          BSMT2000 |
|---------------------------------------|

Chips:
  CPU: IDT 79R3041-25J (MIPS R3000 core)
Sound: BSMT2000
Other: Bt481AKPJ110 (44 Pin PQFP, Brooktree RAMDAC)
       AT001 (160 Pin PQFP, P & P Marketing, custom programmed XILINX XC4310)
       ATMEL 93C66 (EEPROM)
       CN7 - 4 pin connector for stero speaker output
PLDs:
       XILINX-1 XC9536 Labeled as U175A (Rev 2/3: Not Used)
       XILINX-2 XC9536 Labeled as U109A (Rev 2/3: Lattice ispLSI 2032-80LJ - U109.P)
       XILINX-3 XC9536 Labeled as U151A (Rev 2/3: Lattice ispLSI 2032-80LJ - U151.P)

Note #1: Bt481A 256-Word Color Palette 15, 16 & 24-bit Color Power-Down RAMDAC
Note #2: For Rev 2 & 3 PCBs there is an optional daughter card to help with horizontal
         light gun accuracy

The main video chip is stamped:

Rev 2 PCB              Rev 3 PCB              Rev 5B PCB
------------------------------------------------------------
XILINX                 P & P                  P & P
XC4310                 Marketing              Marketing
PQ160C 5380            AJ001                  AT001
PC5380-9651            5380-JY3306A           5380-N1045503A
 PROTO                                        AKI9749

***************************************************************************/

#include "driver.h"
#include "cpu/mips/r3000.h"
#include "machine/eeprom.h"
#include "policetr.h"
#include "sound/bsmt2000.h"


/* constants */
#define MASTER_CLOCK	48000000


/* global variables */
UINT32 *	policetr_rambase;


/* local variables */
static UINT32 control_data;

static UINT32 bsmt_reg;
static UINT32 bsmt_data_bank;
static UINT32 bsmt_data_offset;

static UINT32 *speedup_data;
static UINT32 last_cycles;
static UINT32 loop_count;

static offs_t speedup_pc;



/*************************************
 *
 *  Interrupt handling
 *
 *************************************/

static void irq5_gen(int param)
{
	cpunum_set_input_line(0, R3000_IRQ5, ASSERT_LINE);
}


static INTERRUPT_GEN( irq4_gen )
{
	cpunum_set_input_line(0, R3000_IRQ4, ASSERT_LINE);
	timer_set(cpu_getscanlinetime(0), 0, irq5_gen);
}



/*************************************
 *
 *  Input ports
 *
 *************************************/

static READ32_HANDLER( port0_r )
{
	return readinputport(0) << 16;
}


static READ32_HANDLER( port1_r )
{
	return (readinputport(1) << 16) | (EEPROM_read_bit() << 29);
}


static READ32_HANDLER( port2_r )
{
	return readinputport(2) << 16;
}



/*************************************
 *
 *  Output ports
 *
 *************************************/

static WRITE32_HANDLER( control_w )
{
	// bit $80000000 = BSMT access/ROM read
	// bit $20000000 = toggled every 64 IRQ4's
	// bit $10000000 = ????
	// bit $00800000 = EEPROM data
	// bit $00400000 = EEPROM clock
	// bit $00200000 = EEPROM enable (on 1)

	COMBINE_DATA(&control_data);

	/* handle EEPROM I/O */
	if (!(mem_mask & 0x00ff0000))
	{
		EEPROM_write_bit(data & 0x00800000);
		EEPROM_set_cs_line((data & 0x00200000) ? CLEAR_LINE : ASSERT_LINE);
		EEPROM_set_clock_line((data & 0x00400000) ? ASSERT_LINE : CLEAR_LINE);
	}

	/* log any unknown bits */
	if (data & 0x4f1fffff)
		logerror("%08X: control_w = %08X & %08X\n", activecpu_get_previouspc(), data, ~mem_mask);
}



/*************************************
 *
 *  BSMT2000 I/O
 *
 *************************************/

static WRITE32_HANDLER( bsmt2000_reg_w )
{
	if (control_data & 0x80000000)
		BSMT2000_data_0_w(bsmt_reg, data & 0xffff, mem_mask | 0xffff0000);
	else
		COMBINE_DATA(&bsmt_data_offset);
}


static WRITE32_HANDLER( bsmt2000_data_w )
{
	if (control_data & 0x80000000)
		COMBINE_DATA(&bsmt_reg);
	else
		COMBINE_DATA(&bsmt_data_bank);
}


static READ32_HANDLER( bsmt2000_data_r )
{
	return memory_region(REGION_SOUND1)[bsmt_data_bank * 0x10000 + bsmt_data_offset] << 8;
}



/*************************************
 *
 *  Busy loop optimization
 *
 *************************************/

static WRITE32_HANDLER( speedup_w )
{
	COMBINE_DATA(speedup_data);

	/* see if the PC matches */
	if ((activecpu_get_previouspc() & 0x1fffffff) == speedup_pc)
	{
		UINT32 curr_cycles = activecpu_gettotalcycles();

		/* if less than 50 cycles from the last time, count it */
		if (curr_cycles - last_cycles < 50)
		{
			loop_count++;

			/* more than 2 in a row and we spin */
			if (loop_count > 2)
				cpu_spinuntil_int();
		}
		else
			loop_count = 0;

		last_cycles = curr_cycles;
	}
}



/*************************************
 *
 *  EEPROM interface/saving
 *
 *************************************/

struct EEPROM_interface eeprom_interface_policetr =
{
	8,				// address bits 8
	16,				// data bits    16
	"*110",			// read         1 10 aaaaaa
	"*101",			// write        1 01 aaaaaa dddddddddddddddd
	"*111",			// erase        1 11 aaaaaa
	"*10000xxxx",	// lock         1 00 00xxxx
	"*10011xxxx"	// unlock       1 00 11xxxx
};


static NVRAM_HANDLER( policetr )
{
	if (read_or_write)
		EEPROM_save(file);
	else
	{
		EEPROM_init(&eeprom_interface_policetr);
		if (file)	EEPROM_load(file);
	}
}



/*************************************
 *
 *  Main CPU memory handlers
 *
 *************************************/

static ADDRESS_MAP_START( policetr_map, ADDRESS_SPACE_PROGRAM, 32 )
	AM_RANGE(0x00000000, 0x0001ffff) AM_RAM AM_BASE(&policetr_rambase)
	AM_RANGE(0x00200000, 0x0020000f) AM_WRITE(policetr_video_w)
	AM_RANGE(0x00400000, 0x00400003) AM_READ(policetr_video_r)
	AM_RANGE(0x00500000, 0x00500003) AM_WRITENOP		// copies ROM here at startup, plus checksum
	AM_RANGE(0x00600000, 0x00600003) AM_READ(bsmt2000_data_r)
	AM_RANGE(0x00700000, 0x00700003) AM_WRITE(bsmt2000_reg_w)
	AM_RANGE(0x00800000, 0x00800003) AM_WRITE(bsmt2000_data_w)
	AM_RANGE(0x00900000, 0x00900003) AM_WRITE(policetr_palette_offset_w)
	AM_RANGE(0x00920000, 0x00920003) AM_WRITE(policetr_palette_data_w)
	AM_RANGE(0x00a00000, 0x00a00003) AM_WRITE(control_w)
	AM_RANGE(0x00a00000, 0x00a00003) AM_READ(port0_r)
	AM_RANGE(0x00a20000, 0x00a20003) AM_READ(port1_r)
	AM_RANGE(0x00a40000, 0x00a40003) AM_READ(port2_r)
	AM_RANGE(0x00e00000, 0x00e00003) AM_WRITENOP		// watchdog???
	AM_RANGE(0x1fc00000, 0x1fc7ffff) AM_ROM AM_REGION(REGION_USER1, 0)
ADDRESS_MAP_END


static ADDRESS_MAP_START( sshooter_map, ADDRESS_SPACE_PROGRAM, 32 )
	AM_RANGE(0x00000000, 0x0001ffff) AM_RAM AM_BASE(&policetr_rambase)
	AM_RANGE(0x00200000, 0x00200003) AM_WRITE(bsmt2000_data_w)
	AM_RANGE(0x00300000, 0x00300003) AM_WRITE(policetr_palette_offset_w)
	AM_RANGE(0x00320000, 0x00320003) AM_WRITE(policetr_palette_data_w)
	AM_RANGE(0x00400000, 0x00400003) AM_READ(policetr_video_r)
	AM_RANGE(0x00500000, 0x00500003) AM_WRITENOP		// copies ROM here at startup, plus checksum
	AM_RANGE(0x00600000, 0x00600003) AM_READ(bsmt2000_data_r)
	AM_RANGE(0x00700000, 0x00700003) AM_WRITE(bsmt2000_reg_w)
	AM_RANGE(0x00800000, 0x0080000f) AM_WRITE(policetr_video_w)
	AM_RANGE(0x00a00000, 0x00a00003) AM_WRITE(control_w)
	AM_RANGE(0x00a00000, 0x00a00003) AM_READ(port0_r)
	AM_RANGE(0x00a20000, 0x00a20003) AM_READ(port1_r)
	AM_RANGE(0x00a40000, 0x00a40003) AM_READ(port2_r)
	AM_RANGE(0x00e00000, 0x00e00003) AM_WRITENOP		// watchdog???
	AM_RANGE(0x1fc00000, 0x1fcfffff) AM_ROM AM_REGION(REGION_USER1, 0)
ADDRESS_MAP_END



/*************************************
 *
 *  Port definitions
 *
 *************************************/

INPUT_PORTS_START( policetr )
	PORT_START
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_SERVICE( 0x0020, IP_ACTIVE_LOW )		/* Not actually a dipswitch */
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(1)
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(2)
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x2000, IP_ACTIVE_HIGH, IPT_SPECIAL )		/* EEPROM read */
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Unknown ))
	PORT_DIPSETTING(    0x00, DEF_STR( Off ))
	PORT_DIPSETTING(    0x01, DEF_STR( On ))
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Unknown ))
	PORT_DIPSETTING(    0x00, DEF_STR( Off ))
	PORT_DIPSETTING(    0x02, DEF_STR( On ))
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unknown ))
	PORT_DIPSETTING(    0x00, DEF_STR( Off ))
	PORT_DIPSETTING(    0x04, DEF_STR( On ))
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unknown ))
	PORT_DIPSETTING(    0x00, DEF_STR( Off ))
	PORT_DIPSETTING(    0x08, DEF_STR( On ))
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unknown ))
	PORT_DIPSETTING(    0x00, DEF_STR( Off ))
	PORT_DIPSETTING(    0x10, DEF_STR( On ))
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unknown )) /* Manuals show dips 1 through 6 as unused */
	PORT_DIPSETTING(    0x00, DEF_STR( Off ))
	PORT_DIPSETTING(    0x20, DEF_STR( On ))
	PORT_DIPNAME( 0x40, 0x40, "Monitor Sync")
	PORT_DIPSETTING(    0x00, "+")
	PORT_DIPSETTING(    0x40, "-")
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Flip_Screen )) /* For use with mirrored CRTs - Not supported */
	PORT_DIPSETTING(    0x00, DEF_STR( Off ))
	PORT_DIPSETTING(    0x80, DEF_STR( On ))	/* Will invert the Y axis of guns */
	PORT_BIT( 0xff00, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START				/* fake analog X */
	PORT_BIT( 0xff, 0x80, IPT_LIGHTGUN_X ) PORT_MINMAX(0,255) PORT_SENSITIVITY(50) PORT_KEYDELTA(10)

	PORT_START				/* fake analog Y */
	PORT_BIT( 0xff, 0x80, IPT_LIGHTGUN_Y ) PORT_MINMAX(0,255) PORT_SENSITIVITY(70) PORT_KEYDELTA(10)

	PORT_START				/* fake analog X */
	PORT_BIT( 0xff, 0x80, IPT_LIGHTGUN_X ) PORT_MINMAX(0,255) PORT_SENSITIVITY(50) PORT_KEYDELTA(10) PORT_PLAYER(2)

	PORT_START				/* fake analog Y */
	PORT_BIT( 0xff, 0x80, IPT_LIGHTGUN_Y ) PORT_MINMAX(0,255) PORT_SENSITIVITY(70) PORT_KEYDELTA(10) PORT_PLAYER(2)
INPUT_PORTS_END




/*************************************
 *
 *  Sound definitions
 *
 *************************************/

static struct BSMT2000interface bsmt2000_interface =
{
	11,
	REGION_SOUND1
};



/*************************************
 *
 *  Machine driver
 *
 *************************************/

static struct r3000_config config =
{
	0,		/* 1 if we have an FPU, 0 otherwise */
	4096,	/* code cache size */
	4096	/* data cache size */
};


MACHINE_DRIVER_START( policetr )

	/* basic machine hardware */
	MDRV_CPU_ADD_TAG("main", R3000BE, MASTER_CLOCK/2)
	MDRV_CPU_CONFIG(config)
	MDRV_CPU_PROGRAM_MAP(policetr_map,0)
	MDRV_CPU_VBLANK_INT(irq4_gen,1)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_REAL_60HZ_VBLANK_DURATION)

	MDRV_NVRAM_HANDLER(policetr)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER | VIDEO_UPDATE_BEFORE_VBLANK)
	MDRV_SCREEN_SIZE(400, 240)
	MDRV_VISIBLE_AREA(0, 393, 0, 239)
	MDRV_PALETTE_LENGTH(256)

	MDRV_VIDEO_START(policetr)
	MDRV_VIDEO_UPDATE(policetr)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_STEREO("left", "right")

	MDRV_SOUND_ADD(BSMT2000, MASTER_CLOCK/2)
	MDRV_SOUND_CONFIG(bsmt2000_interface)
	MDRV_SOUND_ROUTE(0, "left", 1.0)
	MDRV_SOUND_ROUTE(1, "right", 1.0)
MACHINE_DRIVER_END


MACHINE_DRIVER_START( sshooter )
	MDRV_IMPORT_FROM(policetr)

	/* basic machine hardware */
	MDRV_CPU_MODIFY("main")
	MDRV_CPU_PROGRAM_MAP(sshooter_map,0)
MACHINE_DRIVER_END



/*************************************
 *
 *  ROM definition(s)
 *
 *************************************/

ROM_START( policetr ) /* Rev 0.3 PCB , unknown program rom date */
	ROM_REGION( 0x400000, REGION_GFX1, ROMREGION_ERASE00 )
	ROM_LOAD16_BYTE( "pt-u121.bin", 0x000000, 0x100000, CRC(56b0b00a) SHA1(4034fe373a61f756f4813f0c20b1cf05e4338059) )
	ROM_LOAD16_BYTE( "pt-u120.bin", 0x000001, 0x100000, CRC(ca664142) SHA1(2727ecb9287b4ed30088e017bb6b8763dfb75b2f) )
	ROM_LOAD16_BYTE( "pt-u125.bin", 0x200000, 0x100000, CRC(e9ccf3a0) SHA1(b3fd8c094f76ace4cf403c3d0f6bd6c5d8db7d6a) )
	ROM_LOAD16_BYTE( "pt-u124.bin", 0x200001, 0x100000, CRC(f4acf921) SHA1(5b244e9a51304318fa0c03eb7365b3c12627d19b) )

	ROM_REGION32_BE( 0x80000, REGION_USER1, 0 )
	ROM_LOAD32_BYTE( "pt-u113.bin", 0x00000, 0x20000, CRC(7b34d366) SHA1(b86cfe155e0685992aebbcc7db705fdbadc42bf9) )
	ROM_LOAD32_BYTE( "pt-u112.bin", 0x00001, 0x20000, CRC(57d059c8) SHA1(ed0c624fc0afbeb6616bba8a67ce5b18d7c119fc) )
	ROM_LOAD32_BYTE( "pt-u111.bin", 0x00002, 0x20000, CRC(fb5ce933) SHA1(4a07ac3e2d86262061092f112cab89f8660dce3d) )
	ROM_LOAD32_BYTE( "pt-u110.bin", 0x00003, 0x20000, CRC(40bd6f60) SHA1(156000d3c439eab45962f0a2681bd806a17f47ee) )

	ROM_REGION( 0x600000, REGION_SOUND1, 0 )
	ROM_LOAD( "pt-u160.bin", 0x000000, 0x100000, CRC(f267f813) SHA1(ae58507947fe2e9701b5df46565fd9908e2f9d77) )
	ROM_RELOAD(              0x3f8000, 0x100000 )
	ROM_LOAD( "pt-u162.bin", 0x100000, 0x100000, CRC(75fe850e) SHA1(ab8cf24ae6e5cf80f6a9a34e46f2b1596879643b) )
	ROM_RELOAD(              0x4f8000, 0x100000 )
ROM_END


ROM_START( polict11 ) /* Rev 0.3 PCB with all chips dated 01/06/97 */
	ROM_REGION( 0x400000, REGION_GFX1, ROMREGION_ERASE00 )
	ROM_LOAD16_BYTE( "pt-u121.bin", 0x000000, 0x100000, CRC(56b0b00a) SHA1(4034fe373a61f756f4813f0c20b1cf05e4338059) )
	ROM_LOAD16_BYTE( "pt-u120.bin", 0x000001, 0x100000, CRC(ca664142) SHA1(2727ecb9287b4ed30088e017bb6b8763dfb75b2f) )
	ROM_LOAD16_BYTE( "pt-u125.bin", 0x200000, 0x100000, CRC(e9ccf3a0) SHA1(b3fd8c094f76ace4cf403c3d0f6bd6c5d8db7d6a) )
	ROM_LOAD16_BYTE( "pt-u124.bin", 0x200001, 0x100000, CRC(f4acf921) SHA1(5b244e9a51304318fa0c03eb7365b3c12627d19b) )

	ROM_REGION32_BE( 0x80000, REGION_USER1, 0 )	/* 2MB for R3000 code */
	ROM_LOAD32_BYTE( "pt-u113.v11", 0x00000, 0x20000, CRC(3d62f6d6) SHA1(342ffa38a6972bbb03c89b4dd603c2cc60609d3d) )
	ROM_LOAD32_BYTE( "pt-u112.v11", 0x00001, 0x20000, CRC(942b280b) SHA1(c342ba3255203ce28ff59479da00f26f0bd026e0) )
	ROM_LOAD32_BYTE( "pt-u111.v11", 0x00002, 0x20000, CRC(da6c45a7) SHA1(471bd372d2ad5bcb29af19dae09f3cfab4b010fd) )
	ROM_LOAD32_BYTE( "pt-u110.v11", 0x00003, 0x20000, CRC(f1c8a8c0) SHA1(8a2d1ada002be6f2a3c2d21d193e7cde6531545a) )

	ROM_REGION( 0x600000, REGION_SOUND1, 0 )
	ROM_LOAD( "pt-u160.bin", 0x000000, 0x100000, CRC(f267f813) SHA1(ae58507947fe2e9701b5df46565fd9908e2f9d77) )
	ROM_RELOAD(              0x3f8000, 0x100000 )
	ROM_LOAD( "pt-u162.bin", 0x100000, 0x100000, CRC(75fe850e) SHA1(ab8cf24ae6e5cf80f6a9a34e46f2b1596879643b) )
	ROM_RELOAD(              0x4f8000, 0x100000 )
ROM_END


ROM_START( polict10 ) /* Rev 0.2 PCB with all chips dated 10/07/96 */
	ROM_REGION( 0x400000, REGION_GFX1, ROMREGION_ERASE00 )
	/* Same data as the other sets, but split in 4 meg roms */
	ROM_LOAD16_BYTE( "pt-u121.v10", 0x000000, 0x080000, CRC(9d31e805) SHA1(482f38e07ddb758e1fb444af7b56a0ef6ea945c8) )
	ROM_LOAD16_BYTE( "pt-u120.v10", 0x000001, 0x080000, CRC(b03b9d46) SHA1(2bb8fcb1df09aa762b98adf2e1edd186203746c0) )
	ROM_LOAD16_BYTE( "pt-u123.v10", 0x100000, 0x080000, CRC(80557cf1) SHA1(ba96fd5b6673b382013e1810a36edb827caaff4b) )
	ROM_LOAD16_BYTE( "pt-u122.v10", 0x100001, 0x080000, CRC(eca09f41) SHA1(bbb1466d39c09598899a3f50b3bb8f9d58b274ec) )
	ROM_LOAD16_BYTE( "pt-u125.v10", 0x200000, 0x080000, CRC(24bddc51) SHA1(6d7c85dba47c675c65e1cb751d581af0d2c678ad) )
	ROM_LOAD16_BYTE( "pt-u124.v10", 0x200001, 0x080000, CRC(f1a43dee) SHA1(2c0aa894e148315168239c7df391ef1f2b4d32a1) )
	ROM_LOAD16_BYTE( "pt-u127.v10", 0x300000, 0x080000, CRC(5031ea1e) SHA1(c1f9272f9874150d510f22c44c186fad0ed3a7e4) )
	ROM_LOAD16_BYTE( "pt-u126.v10", 0x300001, 0x080000, CRC(33bf2653) SHA1(357da2da7df417109adbf600f3455c224f6c076f) )

	ROM_REGION32_BE( 0x80000, REGION_USER1, 0 )	/* 2MB for R3000 code */
	ROM_LOAD32_BYTE( "pt-u113.v10", 0x00000, 0x20000, CRC(3e27a0ce) SHA1(0d010da68f950a10a74eddc57941e4c0e2144071) )
	ROM_LOAD32_BYTE( "pt-u112.v10", 0x00001, 0x20000, CRC(fcbcf4ca) SHA1(374291600043cfbbd87260b12961ac6d68caeda0) )
	ROM_LOAD32_BYTE( "pt-u111.v10", 0x00002, 0x20000, CRC(61f79667) SHA1(25298cd8706b5c59f7c9e0f8d44db0df73c23403) )
	ROM_LOAD32_BYTE( "pt-u110.v10", 0x00003, 0x20000, CRC(5c3c1548) SHA1(aab977274ecff7cb5fd540a3d0da7940e9707906) )

	ROM_REGION( 0x600000, REGION_SOUND1, 0 )
	/* Same data as the other sets, but split in 4 meg roms */
	ROM_LOAD( "pt-u160.v10", 0x000000, 0x080000, CRC(cd374405) SHA1(e53689d4344c78c3faac22747ada28bc3add8c56) )
	ROM_RELOAD(              0x3f8000, 0x080000 )
	ROM_LOAD( "pt-u161.v10", 0x080000, 0x080000, CRC(c33e3497) SHA1(a7d488f04bba3f1b884b0df210c3793f41967d73) )
	ROM_RELOAD(              0x478000, 0x080000 )
	ROM_LOAD( "pt-u162.v10", 0x100000, 0x080000, CRC(e7e02312) SHA1(ac92b8615b18528820a40dad025173e9f24072bf) )
	ROM_RELOAD(              0x4f8000, 0x080000 )
	ROM_LOAD( "pt-u163.v10", 0x180000, 0x080000, CRC(a45b3f85) SHA1(21965dcf89e04d5ee21e27eefd6baa34d6d4479a) )
	ROM_RELOAD(              0x578000, 0x080000 )
ROM_END


ROM_START( plctr13b ) /* Rev 0.5B PCB , unknown program rom date */
	ROM_REGION( 0x400000, REGION_GFX1, ROMREGION_ERASE00 )
	ROM_LOAD16_BYTE( "pt-u121.bin", 0x000000, 0x100000, CRC(56b0b00a) SHA1(4034fe373a61f756f4813f0c20b1cf05e4338059) )
	ROM_LOAD16_BYTE( "pt-u120.bin", 0x000001, 0x100000, CRC(ca664142) SHA1(2727ecb9287b4ed30088e017bb6b8763dfb75b2f) )
	ROM_LOAD16_BYTE( "pt-u125.bin", 0x200000, 0x100000, CRC(e9ccf3a0) SHA1(b3fd8c094f76ace4cf403c3d0f6bd6c5d8db7d6a) )
	ROM_LOAD16_BYTE( "pt-u124.bin", 0x200001, 0x100000, CRC(f4acf921) SHA1(5b244e9a51304318fa0c03eb7365b3c12627d19b) )

	ROM_REGION32_BE( 0x100000, REGION_USER1, 0 )
/*
Note: If you set the dipswitch to service mode and reset the game within Mame.
      All 4 program ROMs fail the checksum code... IE: they show in red
      instead of green.  But, the listed checksums on the screen match the
      checksums printed on the ROM labels... this seems wierd to me.
      However, this has been verified to happen on a real PCB
*/
	ROM_LOAD32_BYTE( "ptb-u113.v13", 0x00000, 0x20000, CRC(d636c00d) SHA1(ef989eb85b51a64ca640297c1286514c8d7f8f76) )
	ROM_LOAD32_BYTE( "ptb-u112.v13", 0x00001, 0x20000, CRC(86f0497e) SHA1(d177023f7cb2e01de60ef072212836dc94759c1a) )
	ROM_LOAD32_BYTE( "ptb-u111.v13", 0x00002, 0x20000, CRC(39e96d6a) SHA1(efe6ffe70432b94c98f3d7247408a6d2f6f9e33d) )
	ROM_LOAD32_BYTE( "ptb-u110.v13", 0x00003, 0x20000, CRC(d7e6f4cb) SHA1(9dffe4937bc5cf47d870f06ae0dced362cd2dd66) )

	ROM_REGION( 0x600000, REGION_SOUND1, 0 )
	ROM_LOAD( "pt-u160.bin", 0x000000, 0x100000, CRC(f267f813) SHA1(ae58507947fe2e9701b5df46565fd9908e2f9d77) )
	ROM_RELOAD(              0x3f8000, 0x100000 )
	ROM_LOAD( "pt-u162.bin", 0x100000, 0x100000, CRC(75fe850e) SHA1(ab8cf24ae6e5cf80f6a9a34e46f2b1596879643b) )
	ROM_RELOAD(              0x4f8000, 0x100000 )
ROM_END


ROM_START( sshooter ) /* Rev 0.5B PCB , unknown program rom date */
	ROM_REGION( 0x800000, REGION_GFX1, ROMREGION_ERASE00 ) /* Graphics v1.0 */
	ROM_LOAD16_BYTE( "ss-u121.bin", 0x000000, 0x100000, CRC(22e27dd6) SHA1(cb9e8c450352bb116a9c0407cc8ce6d8ae9d9881) ) // 1:1
	ROM_LOAD16_BYTE( "ss-u120.bin", 0x000001, 0x100000, CRC(30173b1b) SHA1(366464444ce208391ca350f1639403f0c2217330) ) // 1:2
	ROM_LOAD16_BYTE( "ss-u125.bin", 0x200000, 0x100000, CRC(79e8520a) SHA1(682e5c7954f96db65a137f05cde67c310b85b526) ) // 2:1
	ROM_LOAD16_BYTE( "ss-u124.bin", 0x200001, 0x100000, CRC(8e805970) SHA1(bfc9940ed6425f136d768170275279c590da7003) ) // 2:2
	ROM_LOAD16_BYTE( "ss-u123.bin", 0x400000, 0x100000, CRC(d045bb62) SHA1(839209ff6a8e5db63a51a3494a6c973e0068a3c6) ) // 3:1
	ROM_LOAD16_BYTE( "ss-u122.bin", 0x400001, 0x100000, CRC(163cc133) SHA1(a5e84b5060fd32362aa097d0194ce72e8a90357c) ) // 3:2
	ROM_LOAD16_BYTE( "ss-u127.bin", 0x600000, 0x100000, CRC(76a7a591) SHA1(9fd7cce21b01f388966a3e8388ba95820ac10bfd) ) // 4:1
	ROM_LOAD16_BYTE( "ss-u126.bin", 0x600001, 0x100000, CRC(ab1b9d60) SHA1(ff51a71443f7774d3abf96c2eb8ef6a54d73dd8e) ) // 4:2

	ROM_REGION32_BE( 0x100000, REGION_USER1, 0 )
	ROM_LOAD32_BYTE( "ss-u113.v17", 0x00000, 0x40000, CRC(a8c96af5) SHA1(a62458156603b74e0d84ce6928f7bb868bf5a219) ) // 1:1
	ROM_LOAD32_BYTE( "ss-u112.v17", 0x00001, 0x40000, CRC(c732d5fa) SHA1(2bcc26c8bbf55394173ca65b4b0df01bc6b719bb) ) // 1:2
	ROM_LOAD32_BYTE( "ss-u111.v17", 0x00002, 0x40000, CRC(4240fa2f) SHA1(54223207c1e228d6b836918601c0f65c2692e5bc) ) // 1:3
	ROM_LOAD32_BYTE( "ss-u110.v17", 0x00003, 0x40000, CRC(8ae744ce) SHA1(659cd27865cf5507aae6b064c5bc24b927cf5f5a) ) // 1:4

	ROM_REGION( 0x600000, REGION_SOUND1, 0 ) /* Sound v1.2 */
	ROM_LOAD( "ss-u160.bin", 0x000000, 0x100000, CRC(1c603d42) SHA1(880992871be52129684052d542946de0cc32ba9a) ) // 1:1
	ROM_RELOAD(              0x3f8000, 0x100000 )
	ROM_LOAD( "ss-u162.bin", 0x100000, 0x100000, CRC(40ef448a) SHA1(c96f7b169be2576e9f3783af84c07259efefb812) ) // 2:1
	ROM_RELOAD(              0x4f8000, 0x100000 )
ROM_END


ROM_START( sshoot12 ) /* Rev 0.5B PCB , program roms dated 04/17/98 */
	ROM_REGION( 0x800000, REGION_GFX1, ROMREGION_ERASE00 ) /* Graphics v1.0 */
	ROM_LOAD16_BYTE( "ss-u121.bin", 0x000000, 0x100000, CRC(22e27dd6) SHA1(cb9e8c450352bb116a9c0407cc8ce6d8ae9d9881) ) // 1:1
	ROM_LOAD16_BYTE( "ss-u120.bin", 0x000001, 0x100000, CRC(30173b1b) SHA1(366464444ce208391ca350f1639403f0c2217330) ) // 1:2
	ROM_LOAD16_BYTE( "ss-u125.bin", 0x200000, 0x100000, CRC(79e8520a) SHA1(682e5c7954f96db65a137f05cde67c310b85b526) ) // 2:1
	ROM_LOAD16_BYTE( "ss-u124.bin", 0x200001, 0x100000, CRC(8e805970) SHA1(bfc9940ed6425f136d768170275279c590da7003) ) // 2:2
	ROM_LOAD16_BYTE( "ss-u123.bin", 0x400000, 0x100000, CRC(d045bb62) SHA1(839209ff6a8e5db63a51a3494a6c973e0068a3c6) ) // 3:1
	ROM_LOAD16_BYTE( "ss-u122.bin", 0x400001, 0x100000, CRC(163cc133) SHA1(a5e84b5060fd32362aa097d0194ce72e8a90357c) ) // 3:2
	ROM_LOAD16_BYTE( "ss-u127.bin", 0x600000, 0x100000, CRC(76a7a591) SHA1(9fd7cce21b01f388966a3e8388ba95820ac10bfd) ) // 4:1
	ROM_LOAD16_BYTE( "ss-u126.bin", 0x600001, 0x100000, CRC(ab1b9d60) SHA1(ff51a71443f7774d3abf96c2eb8ef6a54d73dd8e) ) // 4:2

	ROM_REGION32_BE( 0x100000, REGION_USER1, 0 )
	ROM_LOAD32_BYTE( "ss-u113.v12", 0x00000, 0x40000, CRC(73dbaf4b) SHA1(a85fad95d63333f4fe5647f31258b3a22c5c2c0d) ) // 1:1
	ROM_LOAD32_BYTE( "ss-u112.v12", 0x00001, 0x40000, CRC(06fbc2de) SHA1(8bdfcbc33b5fc010464dcd7691f9ecd6ba2168ba) ) // 1:2
	ROM_LOAD32_BYTE( "ss-u111.v12", 0x00002, 0x40000, CRC(0b291731) SHA1(bd04f0b1b52198344df625fcddfc6c6ccb0bd923) ) // 1:3
	ROM_LOAD32_BYTE( "ss-u110.v12", 0x00003, 0x40000, CRC(76841008) SHA1(ccbb88c8d63bf929814144a9d8757c9c7048fdef) ) // 1:4

	ROM_REGION( 0x600000, REGION_SOUND1, 0 ) /* Sound v1.2 */
	ROM_LOAD( "ss-u160.bin", 0x000000, 0x100000, CRC(1c603d42) SHA1(880992871be52129684052d542946de0cc32ba9a) ) // 1:1
	ROM_RELOAD(              0x3f8000, 0x100000 )
	ROM_LOAD( "ss-u162.bin", 0x100000, 0x100000, CRC(40ef448a) SHA1(c96f7b169be2576e9f3783af84c07259efefb812) ) // 2:1
	ROM_RELOAD(              0x4f8000, 0x100000 )
ROM_END


ROM_START( sshoot11 ) /* Rev 0.5B PCB , program roms dated 04/03/98 */
	ROM_REGION( 0x800000, REGION_GFX1, ROMREGION_ERASE00 ) /* Graphics v1.0 */
	ROM_LOAD16_BYTE( "ss-u121.bin", 0x000000, 0x100000, CRC(22e27dd6) SHA1(cb9e8c450352bb116a9c0407cc8ce6d8ae9d9881) ) // 1:1
	ROM_LOAD16_BYTE( "ss-u120.bin", 0x000001, 0x100000, CRC(30173b1b) SHA1(366464444ce208391ca350f1639403f0c2217330) ) // 1:2
	ROM_LOAD16_BYTE( "ss-u125.bin", 0x200000, 0x100000, CRC(79e8520a) SHA1(682e5c7954f96db65a137f05cde67c310b85b526) ) // 2:1
	ROM_LOAD16_BYTE( "ss-u124.bin", 0x200001, 0x100000, CRC(8e805970) SHA1(bfc9940ed6425f136d768170275279c590da7003) ) // 2:2
	ROM_LOAD16_BYTE( "ss-u123.bin", 0x400000, 0x100000, CRC(d045bb62) SHA1(839209ff6a8e5db63a51a3494a6c973e0068a3c6) ) // 3:1
	ROM_LOAD16_BYTE( "ss-u122.bin", 0x400001, 0x100000, CRC(163cc133) SHA1(a5e84b5060fd32362aa097d0194ce72e8a90357c) ) // 3:2
	ROM_LOAD16_BYTE( "ss-u127.bin", 0x600000, 0x100000, CRC(76a7a591) SHA1(9fd7cce21b01f388966a3e8388ba95820ac10bfd) ) // 4:1
	ROM_LOAD16_BYTE( "ss-u126.bin", 0x600001, 0x100000, CRC(ab1b9d60) SHA1(ff51a71443f7774d3abf96c2eb8ef6a54d73dd8e) ) // 4:2

	ROM_REGION32_BE( 0x100000, REGION_USER1, 0 )
	ROM_LOAD32_BYTE( "ss-u113.v11", 0x00000, 0x40000, CRC(c19693f3) SHA1(2f1576261f741d5e69d30f645aea0ed359b8dc03) ) // 1:1
	ROM_LOAD32_BYTE( "ss-u112.v11", 0x00001, 0x40000, CRC(a5ab6d82) SHA1(b2cc3fd875f0c6702cee973b77fd608f4cfe0555) ) // 1:2
	ROM_LOAD32_BYTE( "ss-u111.v11", 0x00002, 0x40000, CRC(ec209b5f) SHA1(1408b509853b325e865d0b23d237bca321e73f60) ) // 1:3
	ROM_LOAD32_BYTE( "ss-u110.v11", 0x00003, 0x40000, CRC(0f1de201) SHA1(5001de3349357545a6a45102340caf0008b50d7b) ) // 1:4

	ROM_REGION( 0x600000, REGION_SOUND1, 0 ) /* Sound v1.2 */
	ROM_LOAD( "ss-u160.bin", 0x000000, 0x100000, CRC(1c603d42) SHA1(880992871be52129684052d542946de0cc32ba9a) ) // 1:1
	ROM_RELOAD(              0x3f8000, 0x100000 )
	ROM_LOAD( "ss-u162.bin", 0x100000, 0x100000, CRC(40ef448a) SHA1(c96f7b169be2576e9f3783af84c07259efefb812) ) // 2:1
	ROM_RELOAD(              0x4f8000, 0x100000 )
ROM_END



/*************************************
 *
 *  Driver initialization
 *
 *************************************/

static DRIVER_INIT( policetr )
{
	speedup_data = memory_install_write32_handler(0, ADDRESS_SPACE_PROGRAM, 0x00000fc8, 0x00000fcb, 0, 0, speedup_w);
	speedup_pc = 0x1fc028ac;
}

static DRIVER_INIT( plctr13b )
{
	speedup_data = memory_install_write32_handler(0, ADDRESS_SPACE_PROGRAM, 0x00000fc8, 0x00000fcb, 0, 0, speedup_w);
	speedup_pc = 0x1fc028bc;
}


static DRIVER_INIT( sshooter )
{
	speedup_data = memory_install_write32_handler(0, ADDRESS_SPACE_PROGRAM, 0x00018fd8, 0x00018fdb, 0, 0, speedup_w);
	speedup_pc = 0x1fc03470;
}

static DRIVER_INIT( sshoot12 )
{
	speedup_data = memory_install_write32_handler(0, ADDRESS_SPACE_PROGRAM, 0x00018fd8, 0x00018fdb, 0, 0, speedup_w);
	speedup_pc = 0x1fc033e0;
}



/*************************************
 *
 *  Game driver(s)
 *
 *************************************/

GAME( 1996, policetr, 0,        policetr, policetr, policetr, ROT0, "P & P Marketing", "Police Trainer (Rev 1.3)", 0 )
GAME( 1996, polict11, policetr, policetr, policetr, policetr, ROT0, "P & P Marketing", "Police Trainer (Rev 1.1)", 0 )
GAME( 1996, polict10, policetr, policetr, policetr, policetr, ROT0, "P & P Marketing", "Police Trainer (Rev 1.0)", 0 )
GAME( 1996, plctr13b, policetr, sshooter, policetr, plctr13b, ROT0, "P & P Marketing", "Police Trainer (Rev 1.3B)", 0 )
GAME( 1998, sshooter, 0,        sshooter, policetr, sshooter, ROT0, "P & P Marketing", "Sharpshooter (Rev 1.7)", 0 )
GAME( 1998, sshoot12, sshooter, sshooter, policetr, sshoot12, ROT0, "P & P Marketing", "Sharpshooter (Rev 1.2)", 0 )
GAME( 1998, sshoot11, sshooter, sshooter, policetr, sshoot12, ROT0, "P & P Marketing", "Sharpshooter (Rev 1.1)", 0 )
