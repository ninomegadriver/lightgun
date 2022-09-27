/***************************************************************************

    Killer Instinct hardware

    driver by Aaron Giles and Bryan McPhail

    Games supported:
        * Killer Instinct
        * Killer Instinct 2

    Known bugs:
        * none at this time

****************************************************************************

Killer Instinct 1 / Killer Instinct 2
Rare/Nintendo, 1994/1995

PCB Layout
----------

This is a fighting game using a hard drive to hold the graphics + code, running on
what appears to be Williams Electronics manufactured hardware.

KILLER INSTINCT V4.0
5770-14397-03
(C)1994 Nintendo/Rare
(sticker - MIDWAY GAMES 44464 I457034  A-20333)
|---------------------------------------------------------------|
|       LED1  LED2       GAL          U10    U11    U12    U13  |
|  TDA7240   TL084  AD1851  10MHz                               |
|                      |--------|                               |
|                      |ANALOG  |                               |
|                      |DEVICES |                               |
|                      |ADSP2105|                               |
|                      |--------|                               |
|J3                                   U33    U34    U35    U36  |
|               71256 71256 71256 71256                         |
|               71256 71256 71256 71256      MT4C4001  MT4C4001 |
|J              71256 71256 71256 71256      MT4C4001  MT4C4001 |
|A              71256 71256 71256 71256      MT4C4001  MT4C4001 |
|M                                           MT4C4001  MT4C4001 |
|M    ULN2064B                               MT4C4001  MT4C4001 |
|A                                           MT4C4001  MT4C4001 |
|          *1                                MT4C4001  MT4C4001 |
|                        *5                  MT4C4001  MT4C4001 |
|                                                               |
|                               50MHz                       JP30|
|              *4                MAX705                         |
|                                 JP32  *2        *3        U98 |
|  DSW1  DSW2                                                   |
|       J7       J8           J6        S3                      |
|---------------------|---------------|-------------------------|
                      |     IDE44     |
                      |               |
                      |       *6      |
                      |               |
                      |     IDE44     |
                      |-|-----------|-|
                        |||||||||||||
                        |||||||||||||
                        |||||||||||||
                        |||||||||||||
                        |||||||||||||
                        |||||||||||||
                        |||||||||||||
                        |||||||||||||
                  |-----|||||||||||||-----|
                  |     |-----------|     |
                  |                       |
                  |                       |
                  |                       |
                  |        Seagate        |
                  |                       |
                  |        ST9420AG       |
                  |                       |
                  |      2.5" H/Drive     |
                  |                       |
                  |                       |
                  |                       |
                  |                       |
                  |                       |
                  |                       |
                  |                       |
                  |                       |
                  |-----------------------|

Notes:
      GAL - GAL20V8 labelled 'KI-U1 A-19802' (DIP24)
      MT4C4001 - 1M x4 DRAM (SOJ28)
      71256 - IDT 71256 32k x8 SRAM (SOJ28)
      JP30 - 3 pin jumper to configure boot ROM. Set to 1-2. Settings are 1-2 = 4MBit. 2-3 = 8MBit.
      JP32 - 2 pin jumper to disable Watch Dog (Hard-wired on the PCB shorted 1-2)
      ADSP2105 - Analog Devices ADSP-2105 (PLCC68)
      J7 - 15 pin connector for player 3 controls
      J3 - 10 pin connector for extra controls
      J6 - 44 pin connector for 2.5" IDE hard drive
      H/drive - Seagate Marathon 2.5" IDE hard drive, model ST9420AG
        -for KI2, labelled 'L2.1 KILLER INSTINCT 2 DISK (C)1985 NINTENDO/RARE) CHS - 988/16/52 - 420.8MB
      For KI1 - H/drive - Seagate Marathon 2.5" IDE hard drive, model ST9150AG

      J8 - 8 pin connector for coin 3-4
      S3 - Reset push-button switch
      LED1 - H/Drive activity LED
      LED2 - Sound Active LED
      *1 Altera EPM7096LC68-10 labelled 'KI-U92 A-19488 (C)1994 NINTENDO/RARE' (PLCC68)
      *2 Altera MAX EPM7128ELC84-10 labelled 'KI-U103 A-19486 (C)1994 NINTENDO/RARE' (PLCC68)
      *3 Altera MAX EPM7128ELC84-10 labelled 'KI-U103 A-19486 (C)1994 NINTENDO/RARE' (PLCC68)
      *4 Altera EPM7032LC44 -15T labelled 'K12-U96 A-20351 (C)1996 NINTENDO/RARE' (PLCC44)
      *5 MIPS 4600-based CPU, heatsinked (QFP208)
      *6 Altera EPM7032LC44 -15T labelled 'K12-U1 A-20383 (C)1996 NINTENDO/RARE' (PLCC44)
         - This is an updated hard drive controller sub board used only with KI2

      ROMs
      ----
      U10 - ST 27C4001 EPROM labelled 'L1 KILLER INSTINCT U10 MUSIC/SPCH (C) 1994 Nintendo/Rare' (DIP32)
      U11 - ST 27C4001 EPROM labelled 'L1 KILLER INSTINCT U11 MUSIC/SPCH (C) 1994 Nintendo/Rare' (DIP32)
      U12 - ST 27C4001 EPROM labelled 'L1 KILLER INSTINCT U12 MUSIC/SPCH (C) 1994 Nintendo/Rare' (DIP32)
      U13 - ST 27C4001 EPROM labelled 'L1 KILLER INSTINCT U13 MUSIC/SPCH (C) 1994 Nintendo/Rare' (DIP32)
      U33 - ST 27C4001 EPROM labelled 'L1 KILLER INSTINCT U33 MUSIC/SPCH (C) 1994 Nintendo/Rare' (DIP32)
      U34 - ST 27C4001 EPROM labelled 'L1 KILLER INSTINCT U34 MUSIC/SPCH (C) 1994 Nintendo/Rare' (DIP32)
      U35 - ST 27C4001 EPROM labelled 'L1 KILLER INSTINCT U35 MUSIC/SPCH (C) 1994 Nintendo/Rare' (DIP32)
      U36 - ST 27C4001 EPROM labelled 'L1 KILLER INSTINCT U36 MUSIC/SPCH (C) 1994 Nintendo/Rare' (DIP32)
      U98 - ST 27C4001 EPROM labelled 'L1.4 KILLER INSTINCT U98 ROM 1 (C) 1994 Nintendo/Rare' (DIP32)

***************************************************************************/

#include "driver.h"
#include "cpu/mips/mips3.h"
#include "cpu/adsp2100/adsp2100.h"
#include "machine/idectrl.h"
#include "machine/midwayic.h"
#include "sndhrdw/dcs.h"


/* constants */
#define MASTER_CLOCK	50000000


/* local variables */
static UINT32 *rambase, *rambase2, *rombase;
static UINT32 *video_base;
static UINT32 *kinst_control;
static UINT32 *kinst_speedup;

static const UINT8 *control_map;



/*************************************
 *
 *  Machine init
 *
 *************************************/

static MACHINE_RESET( kinst )
{
	/* set the fastest DRC options */
	cpunum_set_info_int(0, CPUINFO_INT_MIPS3_DRC_OPTIONS, MIPS3DRC_FASTEST_OPTIONS);

	/* configure fast RAM regions for DRC */
	cpunum_set_info_int(0, CPUINFO_INT_MIPS3_FASTRAM_SELECT, 0);
	cpunum_set_info_int(0, CPUINFO_INT_MIPS3_FASTRAM_START, 0x08000000);
	cpunum_set_info_int(0, CPUINFO_INT_MIPS3_FASTRAM_END, 0x087fffff);
	cpunum_set_info_ptr(0, CPUINFO_PTR_MIPS3_FASTRAM_BASE, rambase2);
	cpunum_set_info_int(0, CPUINFO_INT_MIPS3_FASTRAM_READONLY, 0);

	cpunum_set_info_int(0, CPUINFO_INT_MIPS3_FASTRAM_SELECT, 1);
	cpunum_set_info_int(0, CPUINFO_INT_MIPS3_FASTRAM_START, 0x00000000);
	cpunum_set_info_int(0, CPUINFO_INT_MIPS3_FASTRAM_END, 0x0007ffff);
	cpunum_set_info_ptr(0, CPUINFO_PTR_MIPS3_FASTRAM_BASE, rambase);
	cpunum_set_info_int(0, CPUINFO_INT_MIPS3_FASTRAM_READONLY, 0);

	cpunum_set_info_int(0, CPUINFO_INT_MIPS3_FASTRAM_SELECT, 2);
	cpunum_set_info_int(0, CPUINFO_INT_MIPS3_FASTRAM_START, 0x1fc00000);
	cpunum_set_info_int(0, CPUINFO_INT_MIPS3_FASTRAM_END, 0x1fc7ffff);
	cpunum_set_info_ptr(0, CPUINFO_PTR_MIPS3_FASTRAM_BASE, rombase);
	cpunum_set_info_int(0, CPUINFO_INT_MIPS3_FASTRAM_READONLY, 1);

	/* keep the DCS held in reset at startup */
	dcs_reset_w(1);

	/* reset the IDE controller */
	ide_controller_reset(0);

	/* set a safe base location for video */
	video_base = &rambase[0x30000/4];
}



/*************************************
 *
 *  Video refresh
 *
 *************************************/

VIDEO_UPDATE( kinst )
{
	int y;

	/* loop over rows and copy to the destination */
	for (y = cliprect->min_y; y <= cliprect->max_y; y++)
	{
		UINT32 *src = &video_base[640/4 * y];
		UINT16 *dest = &((UINT16 *)bitmap->line[y])[cliprect->min_x];
		int x;

		/* loop over columns */
		for (x = cliprect->min_x; x < cliprect->max_x; x += 2)
		{
			/* data is BGR; convert to RGB */
			UINT32 data = *src++;
			data = (data & 0x03e003e0) | ((data << 10) & 0x7c007c00) | ((data >> 10) & 0x001f001f);

			/* store two pixels */
			*dest++ = data & 0x7fff;
			*dest++ = (data >> 16) & 0x7fff;
		}
	}
}



/*************************************
 *
 *  Interrupt handling
 *
 *************************************/

static void irq0_stop(int param)
{
	cpunum_set_input_line(0, 0, CLEAR_LINE);
}


static INTERRUPT_GEN( irq0_start )
{
	cpunum_set_input_line(0, 0, ASSERT_LINE);
	timer_set(TIME_IN_USEC(50), 0, irq0_stop);
}


static void ide_interrupt(int state)
{
	cpunum_set_input_line(0, 1, state);
}


static struct ide_interface ide_intf =
{
	ide_interrupt
};



/*************************************
 *
 *  IDE controller access
 *
 *************************************/

static READ32_HANDLER( ide_controller_r )
{
	return midway_ide_asic_r(offset / 2, mem_mask);
}


static WRITE32_HANDLER( ide_controller_w )
{
	midway_ide_asic_w(offset / 2, data, mem_mask);
}


static READ32_HANDLER( ide_controller_extra_r )
{
	return ide_controller32_0_r(0x3f6/4, 0xff00ffff) >> 16;
}


static WRITE32_HANDLER( ide_controller_extra_w )
{
	ide_controller32_0_w(0x3f6/4, data << 16, 0xff00ffff);
}



/*************************************
 *
 *  Control handling
 *
 *************************************/

static READ32_HANDLER( kinst_control_r )
{
	UINT32 result;

	/* apply shuffling */
	offset = control_map[offset / 2];
	result = kinst_control[offset];

	switch (offset)
	{
		case 2:		/* $90 -- sound return */
			result = 0xffff0000 | readinputport(offset);
			result &= ~0x0002;
			if (dcs_control_r() & 0x800)
				result |= 0x0002;
			break;

		case 0:		/* $80 */
		case 1:		/* $88 */
		case 3:		/* $98 */
			result = 0xffff0000 | readinputport(offset);
			break;

		case 4:		/* $a0 */
			result = 0xffff0000 | readinputport(offset);
			if (activecpu_get_pc() == 0x802d428)
				cpu_spinuntil_int();
			break;
	}

	return result;
}


static WRITE32_HANDLER( kinst_control_w )
{
	UINT32 olddata;

	/* apply shuffling */
	offset = control_map[offset / 2];
	olddata = kinst_control[offset];
	COMBINE_DATA(&kinst_control[offset]);

	switch (offset)
	{
		case 0:		/* $80 - VRAM buffer control */
			if (data & 4)
				video_base = &rambase[0x58000/4];
			else
				video_base = &rambase[0x30000/4];
			break;

		case 1:		/* $88 - sound reset */
			dcs_reset_w(~data & 0x01);
			break;

		case 2:		/* $90 - sound control */
			if (!(olddata & 0x02) && (kinst_control[offset] & 0x02))
				dcs_data_w(kinst_control[3]);
			break;

		case 3:		/* $98 - sound data */
			break;
	}
}



/*************************************
 *
 *  Speedups
 *
 *************************************/

static void end_spin(int param)
{
	cpu_triggerint(0);
}


static READ32_HANDLER( kinst_speedup_r )
{
	if (activecpu_get_pc() == 0x88029890 ||	/* KI */
		activecpu_get_pc() == 0x8802c2d0	/* KI2 */)
	{
		UINT32 r3 = activecpu_get_reg(MIPS3_R3);
		UINT32 r26 = activecpu_get_reg(MIPS3_R26) - *kinst_speedup;
		if (r26 < r3)
		{
			timer_set(TIME_IN_CYCLES((r3 - r26) * 2, 0), 0, end_spin);
			cpu_spinuntil_int();
		}
	}
	return *kinst_speedup;
}




/*************************************
 *
 *  Main CPU memory handlers
 *
 *************************************/

static ADDRESS_MAP_START( main_map, ADDRESS_SPACE_PROGRAM, 32 )
	ADDRESS_MAP_FLAGS( AMEF_UNMAP(1) )
	AM_RANGE(0x00000000, 0x0007ffff) AM_RAM AM_BASE(&rambase)
	AM_RANGE(0x08000000, 0x087fffff) AM_RAM AM_BASE(&rambase2)
	AM_RANGE(0x10000080, 0x100000ff) AM_READWRITE(kinst_control_r, kinst_control_w) AM_BASE(&kinst_control)
	AM_RANGE(0x10000100, 0x1000013f) AM_READWRITE(ide_controller_r, ide_controller_w)
	AM_RANGE(0x10000170, 0x10000173) AM_READWRITE(ide_controller_extra_r, ide_controller_extra_w)
	AM_RANGE(0x1fc00000, 0x1fc7ffff) AM_ROM AM_REGION(REGION_USER1, 0) AM_BASE(&rombase)

ADDRESS_MAP_END




/*************************************
 *
 *  Port definitions
 *
 *************************************/

INPUT_PORTS_START( kinst )
	PORT_START
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(1)
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(1)
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_PLAYER(1)
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_BUTTON4 ) PORT_PLAYER(1)
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_BUTTON5 ) PORT_PLAYER(1)
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_BUTTON6 ) PORT_PLAYER(1)
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_PLAYER(1)
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_PLAYER(1)
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_PLAYER(1)
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_PLAYER(1)
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_SERVICE ) PORT_NAME( DEF_STR( Service_Mode )) PORT_CODE(KEYCODE_F2)
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_COIN3 )
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_COIN4 )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_SPECIAL )	/* door */

	PORT_START
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(2)
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(2)
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_PLAYER(2)
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_BUTTON4 ) PORT_PLAYER(2)
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_BUTTON5 ) PORT_PLAYER(2)
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_BUTTON6 ) PORT_PLAYER(2)
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_PLAYER(2)
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_PLAYER(2)
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_PLAYER(2)
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_PLAYER(2)
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_TILT )
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_SPECIAL )	/* bill */
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_SPECIAL )	/* coin door */

	PORT_START
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_SPECIAL )	/* sound status */
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_VOLUME_UP )
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_VOLUME_DOWN )
	PORT_BIT( 0xfff0, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START
	PORT_BIT( 0xffff, IP_ACTIVE_LOW, IPT_UNUSED )	/* verify */

	PORT_START
	PORT_DIPNAME( 0x0003, 0x0003, "Blood Level" )
	PORT_DIPSETTING(      0x0003, DEF_STR( High ))
	PORT_DIPSETTING(      0x0002, DEF_STR( Medium ))
	PORT_DIPSETTING(      0x0001, DEF_STR( Low ))
	PORT_DIPSETTING(      0x0000, DEF_STR( None ))
	PORT_DIPNAME( 0x0004, 0x0004, DEF_STR( Demo_Sounds ))
	PORT_DIPSETTING(      0x0000, DEF_STR( Off ))
	PORT_DIPSETTING(      0x0004, DEF_STR( On ))
	PORT_DIPNAME( 0x0008, 0x0008, "Finishing Moves" )
	PORT_DIPSETTING(      0x0000, DEF_STR( Off ))
	PORT_DIPSETTING(      0x0008, DEF_STR( On ))
	PORT_DIPNAME( 0x0010, 0x0010, "Display Warning" )
	PORT_DIPSETTING(      0x0000, DEF_STR( Off ))
	PORT_DIPSETTING(      0x0010, DEF_STR( On ))
	PORT_DIPNAME( 0x0020, 0x0020, "Blood" )
	PORT_DIPSETTING(      0x0020, "Red" )
	PORT_DIPSETTING(      0x0000, "White" )
	PORT_DIPNAME( 0x0040, 0x0040, DEF_STR( Unused ))
	PORT_DIPSETTING(      0x0040, DEF_STR( Off ))
	PORT_DIPSETTING(      0x0000, DEF_STR( On ))
	PORT_DIPNAME( 0x0080, 0x0080, DEF_STR( Unused ))
	PORT_DIPSETTING(      0x0080, DEF_STR( Off ))
	PORT_DIPSETTING(      0x0000, DEF_STR( On ))
	PORT_DIPNAME( 0x0100, 0x0100, "Coinage Source" )
	PORT_DIPSETTING(      0x0100, "Dipswitch" )
	PORT_DIPSETTING(      0x0000, "Disk" )
	PORT_DIPNAME( 0x3e00, 0x3e00, DEF_STR( Coinage ))
	PORT_DIPSETTING(      0x3e00, "USA-1" )
	PORT_DIPSETTING(      0x3c00, "USA-2" )
	PORT_DIPSETTING(      0x3a00, "USA-3" )
	PORT_DIPSETTING(      0x3800, "USA-4" )
	PORT_DIPSETTING(      0x3400, "USA-9" )
	PORT_DIPSETTING(      0x3200, "USA-10" )
	PORT_DIPSETTING(      0x3600, "USA-ECA" )
	PORT_DIPSETTING(      0x3000, "USA-Free Play" )
	PORT_DIPSETTING(      0x2e00, "German-1" )
	PORT_DIPSETTING(      0x2c00, "German-2" )
	PORT_DIPSETTING(      0x2a00, "German-3" )
	PORT_DIPSETTING(      0x2800, "German-4" )
	PORT_DIPSETTING(      0x2600, "German-ECA" )
	PORT_DIPSETTING(      0x2000, "German-Free Play" )
	PORT_DIPSETTING(      0x1e00, "French-1" )
	PORT_DIPSETTING(      0x1c00, "French-2" )
	PORT_DIPSETTING(      0x1a00, "French-3" )
	PORT_DIPSETTING(      0x1800, "French-4" )
	PORT_DIPSETTING(      0x1600, "French-ECA" )
	PORT_DIPSETTING(      0x1000, "French-Free Play" )
	PORT_DIPNAME( 0x4000, 0x4000, "Coin Counters" )
	PORT_DIPSETTING(      0x4000, "1" )
	PORT_DIPSETTING(      0x0000, "2" )
	PORT_DIPNAME( 0x8000, 0x8000, "Test Switch" )
	PORT_DIPSETTING(      0x8000, DEF_STR( Off ))
	PORT_DIPSETTING(      0x0000, DEF_STR( On ))
INPUT_PORTS_END


INPUT_PORTS_START( kinst2 )
	PORT_START
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(1)
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(1)
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_PLAYER(1)
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_BUTTON4 ) PORT_PLAYER(1)
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_BUTTON5 ) PORT_PLAYER(1)
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_BUTTON6 ) PORT_PLAYER(1)
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_PLAYER(1)
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_PLAYER(1)
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_PLAYER(1)
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_PLAYER(1)
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_SERVICE ) PORT_NAME( DEF_STR( Service_Mode )) PORT_CODE(KEYCODE_F2)
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_COIN3 )
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_COIN4 )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_SPECIAL )	/* door */

	PORT_START
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(2)
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(2)
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_PLAYER(2)
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_BUTTON4 ) PORT_PLAYER(2)
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_BUTTON5 ) PORT_PLAYER(2)
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_BUTTON6 ) PORT_PLAYER(2)
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_PLAYER(2)
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_PLAYER(2)
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_PLAYER(2)
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_PLAYER(2)
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_TILT )
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_SPECIAL )	/* bill */
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_SPECIAL )	/* coin door */

	PORT_START
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_SPECIAL )	/* sound status */
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_VOLUME_UP )
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_VOLUME_DOWN )
	PORT_BIT( 0xfff0, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START
	PORT_BIT( 0xffff, IP_ACTIVE_LOW, IPT_UNUSED )	/* verify */

	PORT_START
	PORT_DIPNAME( 0x0003, 0x0003, "Blood Level" )
	PORT_DIPSETTING(      0x0003, DEF_STR( High ))
	PORT_DIPSETTING(      0x0002, DEF_STR( Medium ))
	PORT_DIPSETTING(      0x0001, DEF_STR( Low ))
	PORT_DIPSETTING(      0x0000, DEF_STR( None ))
	PORT_DIPNAME( 0x0004, 0x0004, DEF_STR( Demo_Sounds ))
	PORT_DIPSETTING(      0x0000, DEF_STR( Off ))
	PORT_DIPSETTING(      0x0004, DEF_STR( On ))
	PORT_DIPNAME( 0x0008, 0x0008, "Finishing Moves" )
	PORT_DIPSETTING(      0x0000, DEF_STR( Off ))
	PORT_DIPSETTING(      0x0008, DEF_STR( On ))
	PORT_DIPNAME( 0x0010, 0x0010, "Display Warning" )
	PORT_DIPSETTING(      0x0000, DEF_STR( Off ))
	PORT_DIPSETTING(      0x0010, DEF_STR( On ))
	PORT_DIPNAME( 0x0020, 0x0020, DEF_STR( Unused ))
	PORT_DIPSETTING(      0x0020, DEF_STR( Off ))
	PORT_DIPSETTING(      0x0000, DEF_STR( On ))
	PORT_DIPNAME( 0x0040, 0x0040, DEF_STR( Unused ))
	PORT_DIPSETTING(      0x0040, DEF_STR( Off ))
	PORT_DIPSETTING(      0x0000, DEF_STR( On ))
	PORT_DIPNAME( 0x0080, 0x0080, DEF_STR( Unused ))
	PORT_DIPSETTING(      0x0080, DEF_STR( Off ))
	PORT_DIPSETTING(      0x0000, DEF_STR( On ))
	PORT_DIPNAME( 0x0100, 0x0100, "Coinage Source" )
	PORT_DIPSETTING(      0x0100, "Dipswitch" )
	PORT_DIPSETTING(      0x0000, "Disk" )
	PORT_DIPNAME( 0x3e00, 0x3e00, DEF_STR( Coinage ))
	PORT_DIPSETTING(      0x3e00, "USA-1" )
	PORT_DIPSETTING(      0x3c00, "USA-2" )
	PORT_DIPSETTING(      0x3a00, "USA-3" )
	PORT_DIPSETTING(      0x3800, "USA-4" )
	PORT_DIPSETTING(      0x3400, "USA-9" )
	PORT_DIPSETTING(      0x3200, "USA-10" )
	PORT_DIPSETTING(      0x3600, "USA-ECA" )
	PORT_DIPSETTING(      0x3000, "USA-Free Play" )
	PORT_DIPSETTING(      0x2e00, "German-1" )
	PORT_DIPSETTING(      0x2c00, "German-2" )
	PORT_DIPSETTING(      0x2a00, "German-3" )
	PORT_DIPSETTING(      0x2800, "German-4" )
	PORT_DIPSETTING(      0x2600, "German-ECA" )
	PORT_DIPSETTING(      0x2000, "German-Free Play" )
	PORT_DIPSETTING(      0x1e00, "French-1" )
	PORT_DIPSETTING(      0x1c00, "French-2" )
	PORT_DIPSETTING(      0x1a00, "French-3" )
	PORT_DIPSETTING(      0x1800, "French-4" )
	PORT_DIPSETTING(      0x1600, "French-ECA" )
	PORT_DIPSETTING(      0x1000, "French-Free Play" )
	PORT_DIPNAME( 0x4000, 0x4000, "Coin Counters" )
	PORT_DIPSETTING(      0x4000, "1" )
	PORT_DIPSETTING(      0x0000, "2" )
	PORT_DIPNAME( 0x8000, 0x8000, "Test Switch" )
	PORT_DIPSETTING(      0x8000, DEF_STR( Off ))
	PORT_DIPSETTING(      0x0000, DEF_STR( On ))
INPUT_PORTS_END




/*************************************
 *
 *  Machine driver
 *
 *************************************/

static struct mips3_config config =
{
	16384,				/* code cache size */
	16384				/* data cache size */
};


MACHINE_DRIVER_START( kinst )

	/* basic machine hardware */
	MDRV_CPU_ADD(R4600LE, MASTER_CLOCK*2)
	MDRV_CPU_CONFIG(config)
	MDRV_CPU_PROGRAM_MAP(main_map,0)
	MDRV_CPU_VBLANK_INT(irq0_start,1)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_REAL_60HZ_VBLANK_DURATION)

	MDRV_MACHINE_RESET(kinst)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER | VIDEO_UPDATE_BEFORE_VBLANK | VIDEO_RGB_DIRECT)
	MDRV_SCREEN_SIZE(320, 240)
	MDRV_VISIBLE_AREA(0, 319, 0, 239)
	MDRV_PALETTE_LENGTH(32768)

	MDRV_VIDEO_UPDATE(kinst)

	/* sound hardware */
	MDRV_IMPORT_FROM(dcs_audio_2k)
MACHINE_DRIVER_END



/*************************************
 *
 *  ROM definition(s)
 *
 *************************************/

ROM_START( kinst )
	ROM_REGION32_LE( 0x80000, REGION_USER1, 0 )	/* 512k for R4600 code */
	ROM_LOAD( "u98-l15d", 0x00000, 0x80000, CRC(7b65ca3d) SHA1(607394d4ba1713f38c2cb5159303cace9cde991e) )

	ROM_REGION16_LE( 0x1000000, REGION_SOUND1, ROMREGION_ERASEFF )	/* sound data */
	ROM_LOAD16_BYTE( "u10-l1", 0x000000, 0x80000, CRC(b6cc155f) SHA1(810d455df8f385d76143e9d7d048f2b555ff8bf0) )
	ROM_LOAD16_BYTE( "u11-l1", 0x200000, 0x80000, CRC(0b5e05df) SHA1(0595909cb667c38ac7c8c7bd0646b28899e27777) )
	ROM_LOAD16_BYTE( "u12-l1", 0x400000, 0x80000, CRC(d05ce6ad) SHA1(7a8ee405c118fd176b66353fa7bfab888cc63cd2) )
	ROM_LOAD16_BYTE( "u13-l1", 0x600000, 0x80000, CRC(7d0954ea) SHA1(ea4d1f153eb284f1bcfc5295fbce316bba6083f4) )
	ROM_LOAD16_BYTE( "u33-l1", 0x800000, 0x80000, CRC(8bbe4f0c) SHA1(b22e365bc8d58a80eaac226be14b4bb8d9a04844) )
	ROM_LOAD16_BYTE( "u34-l1", 0xa00000, 0x80000, CRC(b2e73603) SHA1(ee439f5162a2b3379d3f802328017bb3c68547d2) )
	ROM_LOAD16_BYTE( "u35-l1", 0xc00000, 0x80000, CRC(0aaef4fc) SHA1(48c4c954ac9db648f28ad64f9845e19ec432eec3) )
	ROM_LOAD16_BYTE( "u36-l1", 0xe00000, 0x80000, CRC(0577bb60) SHA1(cc78070cc41701e9a91fde5cfbdc7e1e83354854) )

	DISK_REGION( REGION_DISKS )
	DISK_IMAGE( "kinst", 0, MD5(6d4c2f152c9a18ab3a9b05b8804306a8) SHA1(a37a2c5e52ea936a715210d237874dd573bb002f) )
ROM_END


ROM_START( kinst14 )
	ROM_REGION32_LE( 0x80000, REGION_USER1, 0 )	/* 512k for R4600 code */
	ROM_LOAD( "u98-l14", 0x00000, 0x80000, CRC(afedb75f) SHA1(07254f20707377f7195e64675eb6458e663c1a9a) )

	ROM_REGION16_LE( 0x1000000, REGION_SOUND1, ROMREGION_ERASEFF )	/* sound data */
	ROM_LOAD16_BYTE( "u10-l1", 0x000000, 0x80000, CRC(b6cc155f) SHA1(810d455df8f385d76143e9d7d048f2b555ff8bf0) )
	ROM_LOAD16_BYTE( "u11-l1", 0x200000, 0x80000, CRC(0b5e05df) SHA1(0595909cb667c38ac7c8c7bd0646b28899e27777) )
	ROM_LOAD16_BYTE( "u12-l1", 0x400000, 0x80000, CRC(d05ce6ad) SHA1(7a8ee405c118fd176b66353fa7bfab888cc63cd2) )
	ROM_LOAD16_BYTE( "u13-l1", 0x600000, 0x80000, CRC(7d0954ea) SHA1(ea4d1f153eb284f1bcfc5295fbce316bba6083f4) )
	ROM_LOAD16_BYTE( "u33-l1", 0x800000, 0x80000, CRC(8bbe4f0c) SHA1(b22e365bc8d58a80eaac226be14b4bb8d9a04844) )
	ROM_LOAD16_BYTE( "u34-l1", 0xa00000, 0x80000, CRC(b2e73603) SHA1(ee439f5162a2b3379d3f802328017bb3c68547d2) )
	ROM_LOAD16_BYTE( "u35-l1", 0xc00000, 0x80000, CRC(0aaef4fc) SHA1(48c4c954ac9db648f28ad64f9845e19ec432eec3) )
	ROM_LOAD16_BYTE( "u36-l1", 0xe00000, 0x80000, CRC(0577bb60) SHA1(cc78070cc41701e9a91fde5cfbdc7e1e83354854) )

	DISK_REGION( REGION_DISKS )
	DISK_IMAGE( "kinst", 0, MD5(6d4c2f152c9a18ab3a9b05b8804306a8) SHA1(a37a2c5e52ea936a715210d237874dd573bb002f) )
ROM_END


ROM_START( kinst13 )
	ROM_REGION32_LE( 0x80000, REGION_USER1, 0 )	/* 512k for R4600 code */
	ROM_LOAD( "ki113.bin", 0x00000, 0x80000, CRC(65f7ea31) SHA1(7f21620a512549db6821a0b4fa53681a767b7974) )

	ROM_REGION16_LE( 0x1000000, REGION_SOUND1, ROMREGION_ERASEFF )	/* sound data */
	ROM_LOAD16_BYTE( "u10-l1", 0x000000, 0x80000, CRC(b6cc155f) SHA1(810d455df8f385d76143e9d7d048f2b555ff8bf0) )
	ROM_LOAD16_BYTE( "u11-l1", 0x200000, 0x80000, CRC(0b5e05df) SHA1(0595909cb667c38ac7c8c7bd0646b28899e27777) )
	ROM_LOAD16_BYTE( "u12-l1", 0x400000, 0x80000, CRC(d05ce6ad) SHA1(7a8ee405c118fd176b66353fa7bfab888cc63cd2) )
	ROM_LOAD16_BYTE( "u13-l1", 0x600000, 0x80000, CRC(7d0954ea) SHA1(ea4d1f153eb284f1bcfc5295fbce316bba6083f4) )
	ROM_LOAD16_BYTE( "u33-l1", 0x800000, 0x80000, CRC(8bbe4f0c) SHA1(b22e365bc8d58a80eaac226be14b4bb8d9a04844) )
	ROM_LOAD16_BYTE( "u34-l1", 0xa00000, 0x80000, CRC(b2e73603) SHA1(ee439f5162a2b3379d3f802328017bb3c68547d2) )
	ROM_LOAD16_BYTE( "u35-l1", 0xc00000, 0x80000, CRC(0aaef4fc) SHA1(48c4c954ac9db648f28ad64f9845e19ec432eec3) )
	ROM_LOAD16_BYTE( "u36-l1", 0xe00000, 0x80000, CRC(0577bb60) SHA1(cc78070cc41701e9a91fde5cfbdc7e1e83354854) )

	DISK_REGION( REGION_DISKS )
	DISK_IMAGE( "kinst", 0, MD5(6d4c2f152c9a18ab3a9b05b8804306a8) SHA1(a37a2c5e52ea936a715210d237874dd573bb002f) )
ROM_END


ROM_START( kinstp )
	ROM_REGION32_LE( 0x80000, REGION_USER1, 0 )	/* 512k for R4600 code */
	ROM_LOAD( "ki_p47.u98", 0x00000, 0x80000, CRC(05e67bcb) SHA1(501e69b3026394f69229a6e9866c1037502b86bb) )

	ROM_REGION16_LE( 0x1000000, REGION_SOUND1, ROMREGION_ERASEFF )	/* sound data */
	ROM_LOAD16_BYTE( "u10-l1", 0x000000, 0x80000, CRC(b6cc155f) SHA1(810d455df8f385d76143e9d7d048f2b555ff8bf0) )
	ROM_LOAD16_BYTE( "u11-l1", 0x200000, 0x80000, CRC(0b5e05df) SHA1(0595909cb667c38ac7c8c7bd0646b28899e27777) )
	ROM_LOAD16_BYTE( "u12-l1", 0x400000, 0x80000, CRC(d05ce6ad) SHA1(7a8ee405c118fd176b66353fa7bfab888cc63cd2) )
	ROM_LOAD16_BYTE( "u13-l1", 0x600000, 0x80000, CRC(7d0954ea) SHA1(ea4d1f153eb284f1bcfc5295fbce316bba6083f4) )
	ROM_LOAD16_BYTE( "u33-l1", 0x800000, 0x80000, CRC(8bbe4f0c) SHA1(b22e365bc8d58a80eaac226be14b4bb8d9a04844) )
	ROM_LOAD16_BYTE( "u34-l1", 0xa00000, 0x80000, CRC(b2e73603) SHA1(ee439f5162a2b3379d3f802328017bb3c68547d2) )
	ROM_LOAD16_BYTE( "u35-l1", 0xc00000, 0x80000, CRC(0aaef4fc) SHA1(48c4c954ac9db648f28ad64f9845e19ec432eec3) )
	ROM_LOAD16_BYTE( "u36-l1", 0xe00000, 0x80000, CRC(0577bb60) SHA1(cc78070cc41701e9a91fde5cfbdc7e1e83354854) )

	DISK_REGION( REGION_DISKS )
	DISK_IMAGE( "kinst", 0, MD5(6d4c2f152c9a18ab3a9b05b8804306a8) SHA1(a37a2c5e52ea936a715210d237874dd573bb002f) )
ROM_END


ROM_START( kinst2 )
	ROM_REGION32_LE( 0x80000, REGION_USER1, 0 )	/* 512k for R4600 code */
	ROM_LOAD( "ki2-l14.u98", 0x00000, 0x80000, CRC(27d0285e) SHA1(aa7a2a9d72a47dd0ea2ee7b2776b79288060b179) )

	ROM_REGION16_LE( 0x1000000, REGION_SOUND1, ROMREGION_ERASEFF )	/* sound data */
	ROM_LOAD16_BYTE( "ki2_l1.u10", 0x000000, 0x80000, CRC(fdf6ed51) SHA1(acfc9460cd5df01403b7f00b2f68c2a8734ad6d3) )
	ROM_LOAD16_BYTE( "ki2_l1.u11", 0x200000, 0x80000, CRC(f9e70024) SHA1(fe7fc78f1c60b15f2bbdc4c455f55cdf30f48ed4) )
	ROM_LOAD16_BYTE( "ki2_l1.u12", 0x400000, 0x80000, CRC(2994c199) SHA1(9997a83432cb720f65b40a8af46f31a5d0d16d8e) )
	ROM_LOAD16_BYTE( "ki2_l1.u13", 0x600000, 0x80000, CRC(3fe6327b) SHA1(7ff164fc2f079d039921594be92208973d43aa03) )
	ROM_LOAD16_BYTE( "ki2_l1.u33", 0x800000, 0x80000, CRC(6f4dcdcf) SHA1(0ab6dbfb76e9fa2db072e287864ad1f9d514dd9b) )
	ROM_LOAD16_BYTE( "ki2_l1.u34", 0xa00000, 0x80000, CRC(5db48206) SHA1(48456a7b6592c40bc9c664dcd2ee2cfd91942811) )
	ROM_LOAD16_BYTE( "ki2_l1.u35", 0xc00000, 0x80000, CRC(7245ce69) SHA1(24a3ff009c8a7f5a0bfcb198b8dcb5df365770d3) )
	ROM_LOAD16_BYTE( "ki2_l1.u36", 0xe00000, 0x80000, CRC(8920acbb) SHA1(0fca72c40067034939b984b4bf32972a5a6c26af) )

	DISK_REGION( REGION_DISKS )
	DISK_IMAGE( "kinst2", 0, MD5(2563b089b316f2c8636d78af661ac656) SHA1(ab0242233d2eaf9d907abe246a54e09a8a2561a5) )
ROM_END


ROM_START( kinst2k )
	ROM_REGION32_LE( 0x80000, REGION_USER1, 0 )	/* 512k for R4600 code */
	ROM_LOAD( "ki2_l14k.u98", 0x00000, 0x80000, CRC(9cbd00a8) SHA1(926dce4bb9016331ea40d3c337a9ace896f07493) )

	ROM_REGION16_LE( 0x1000000, REGION_SOUND1, ROMREGION_ERASEFF )	/* sound data */
	ROM_LOAD16_BYTE( "ki2_l1.u10", 0x000000, 0x80000, CRC(fdf6ed51) SHA1(acfc9460cd5df01403b7f00b2f68c2a8734ad6d3) )
	ROM_LOAD16_BYTE( "ki2_l1.u11", 0x200000, 0x80000, CRC(f9e70024) SHA1(fe7fc78f1c60b15f2bbdc4c455f55cdf30f48ed4) )
	ROM_LOAD16_BYTE( "ki2_l1.u12", 0x400000, 0x80000, CRC(2994c199) SHA1(9997a83432cb720f65b40a8af46f31a5d0d16d8e) )
	ROM_LOAD16_BYTE( "ki2_l1.u13", 0x600000, 0x80000, CRC(3fe6327b) SHA1(7ff164fc2f079d039921594be92208973d43aa03) )
	ROM_LOAD16_BYTE( "ki2_l1.u33", 0x800000, 0x80000, CRC(6f4dcdcf) SHA1(0ab6dbfb76e9fa2db072e287864ad1f9d514dd9b) )
	ROM_LOAD16_BYTE( "ki2_l1.u34", 0xa00000, 0x80000, CRC(5db48206) SHA1(48456a7b6592c40bc9c664dcd2ee2cfd91942811) )
	ROM_LOAD16_BYTE( "ki2_l1.u35", 0xc00000, 0x80000, CRC(7245ce69) SHA1(24a3ff009c8a7f5a0bfcb198b8dcb5df365770d3) )
	ROM_LOAD16_BYTE( "ki2_l1.u36", 0xe00000, 0x80000, CRC(8920acbb) SHA1(0fca72c40067034939b984b4bf32972a5a6c26af) )

	DISK_REGION( REGION_DISKS )
	DISK_IMAGE( "kinst2", 0, MD5(2563b089b316f2c8636d78af661ac656) SHA1(ab0242233d2eaf9d907abe246a54e09a8a2561a5) )
ROM_END


ROM_START( kinst213 )
	ROM_REGION32_LE( 0x80000, REGION_USER1, 0 )	/* 512k for R4600 code */
	ROM_LOAD( "ki2-v13.u98", 0x00000, 0x80000, CRC(25ebde3b) SHA1(771d150fb4de0a2ceb279954b9545458e93e2405) )

	ROM_REGION16_LE( 0x1000000, REGION_SOUND1, ROMREGION_ERASEFF )	/* sound data */
	ROM_LOAD16_BYTE( "ki2_l1.u10", 0x000000, 0x80000, CRC(fdf6ed51) SHA1(acfc9460cd5df01403b7f00b2f68c2a8734ad6d3) )
	ROM_LOAD16_BYTE( "ki2_l1.u11", 0x200000, 0x80000, CRC(f9e70024) SHA1(fe7fc78f1c60b15f2bbdc4c455f55cdf30f48ed4) )
	ROM_LOAD16_BYTE( "ki2_l1.u12", 0x400000, 0x80000, CRC(2994c199) SHA1(9997a83432cb720f65b40a8af46f31a5d0d16d8e) )
	ROM_LOAD16_BYTE( "ki2_l1.u13", 0x600000, 0x80000, CRC(3fe6327b) SHA1(7ff164fc2f079d039921594be92208973d43aa03) )
	ROM_LOAD16_BYTE( "ki2_l1.u33", 0x800000, 0x80000, CRC(6f4dcdcf) SHA1(0ab6dbfb76e9fa2db072e287864ad1f9d514dd9b) )
	ROM_LOAD16_BYTE( "ki2_l1.u34", 0xa00000, 0x80000, CRC(5db48206) SHA1(48456a7b6592c40bc9c664dcd2ee2cfd91942811) )
	ROM_LOAD16_BYTE( "ki2_l1.u35", 0xc00000, 0x80000, CRC(7245ce69) SHA1(24a3ff009c8a7f5a0bfcb198b8dcb5df365770d3) )
	ROM_LOAD16_BYTE( "ki2_l1.u36", 0xe00000, 0x80000, CRC(8920acbb) SHA1(0fca72c40067034939b984b4bf32972a5a6c26af) )

	DISK_REGION( REGION_DISKS )
	DISK_IMAGE( "kinst2", 0, MD5(2563b089b316f2c8636d78af661ac656) SHA1(ab0242233d2eaf9d907abe246a54e09a8a2561a5) )
ROM_END


ROM_START( kinst211 )
	ROM_REGION32_LE( 0x80000, REGION_USER1, 0 )	/* 512k for R4600 code */
	ROM_LOAD( "ki2-l11.u98", 0x00000, 0x80000, CRC(0cb8de1e) SHA1(fe447f4b1d29b524f57c5ba1890652ef6afff88a) )

	ROM_REGION16_LE( 0x1000000, REGION_SOUND1, ROMREGION_ERASEFF )	/* sound data */
	ROM_LOAD16_BYTE( "ki2_l1.u10", 0x000000, 0x80000, CRC(fdf6ed51) SHA1(acfc9460cd5df01403b7f00b2f68c2a8734ad6d3) )
	ROM_LOAD16_BYTE( "ki2_l1.u11", 0x200000, 0x80000, CRC(f9e70024) SHA1(fe7fc78f1c60b15f2bbdc4c455f55cdf30f48ed4) )
	ROM_LOAD16_BYTE( "ki2_l1.u12", 0x400000, 0x80000, CRC(2994c199) SHA1(9997a83432cb720f65b40a8af46f31a5d0d16d8e) )
	ROM_LOAD16_BYTE( "ki2_l1.u13", 0x600000, 0x80000, CRC(3fe6327b) SHA1(7ff164fc2f079d039921594be92208973d43aa03) )
	ROM_LOAD16_BYTE( "ki2_l1.u33", 0x800000, 0x80000, CRC(6f4dcdcf) SHA1(0ab6dbfb76e9fa2db072e287864ad1f9d514dd9b) )
	ROM_LOAD16_BYTE( "ki2_l1.u34", 0xa00000, 0x80000, CRC(5db48206) SHA1(48456a7b6592c40bc9c664dcd2ee2cfd91942811) )
	ROM_LOAD16_BYTE( "ki2_l1.u35", 0xc00000, 0x80000, CRC(7245ce69) SHA1(24a3ff009c8a7f5a0bfcb198b8dcb5df365770d3) )
	ROM_LOAD16_BYTE( "ki2_l1.u36", 0xe00000, 0x80000, CRC(8920acbb) SHA1(0fca72c40067034939b984b4bf32972a5a6c26af) )

	DISK_REGION( REGION_DISKS )
	DISK_IMAGE( "kinst2", 0, MD5(2563b089b316f2c8636d78af661ac656) SHA1(ab0242233d2eaf9d907abe246a54e09a8a2561a5) )
ROM_END


ROM_START( kinst210 )
	ROM_REGION32_LE( 0x80000, REGION_USER1, 0 )	/* 512k for R4600 code */
	ROM_LOAD( "ki2_l10.u98", 0x00000, 0x80000, CRC(b17b4b3d) SHA1(756629cd1b51ae50f2b9818765dd3d277c3019b3) )

	ROM_REGION16_LE( 0x1000000, REGION_SOUND1, ROMREGION_ERASEFF )	/* sound data */
	ROM_LOAD16_BYTE( "ki2_l1.u10", 0x000000, 0x80000, CRC(fdf6ed51) SHA1(acfc9460cd5df01403b7f00b2f68c2a8734ad6d3) )
	ROM_LOAD16_BYTE( "ki2_l1.u11", 0x200000, 0x80000, CRC(f9e70024) SHA1(fe7fc78f1c60b15f2bbdc4c455f55cdf30f48ed4) )
	ROM_LOAD16_BYTE( "ki2_l1.u12", 0x400000, 0x80000, CRC(2994c199) SHA1(9997a83432cb720f65b40a8af46f31a5d0d16d8e) )
	ROM_LOAD16_BYTE( "ki2_l1.u13", 0x600000, 0x80000, CRC(3fe6327b) SHA1(7ff164fc2f079d039921594be92208973d43aa03) )
	ROM_LOAD16_BYTE( "ki2_l1.u33", 0x800000, 0x80000, CRC(6f4dcdcf) SHA1(0ab6dbfb76e9fa2db072e287864ad1f9d514dd9b) )
	ROM_LOAD16_BYTE( "ki2_l1.u34", 0xa00000, 0x80000, CRC(5db48206) SHA1(48456a7b6592c40bc9c664dcd2ee2cfd91942811) )
	ROM_LOAD16_BYTE( "ki2_l1.u35", 0xc00000, 0x80000, CRC(7245ce69) SHA1(24a3ff009c8a7f5a0bfcb198b8dcb5df365770d3) )
	ROM_LOAD16_BYTE( "ki2_l1.u36", 0xe00000, 0x80000, CRC(8920acbb) SHA1(0fca72c40067034939b984b4bf32972a5a6c26af) )

	DISK_REGION( REGION_DISKS )
	DISK_IMAGE( "kinst2", 0, MD5(2563b089b316f2c8636d78af661ac656) SHA1(ab0242233d2eaf9d907abe246a54e09a8a2561a5) )
ROM_END



/*************************************
 *
 *  Driver initialization
 *
 *************************************/

static DRIVER_INIT( kinst )
{
	static const UINT8 kinst_control_map[8] = { 0,1,2,3,4,5,6,7 };
	UINT8 *features;

	dcs_init();

	/* set up the control register mapping */
	control_map = kinst_control_map;

	/* spin up the hard disk */
	ide_controller_init(0, &ide_intf);

	/* tweak the model number so we pass the check */
	features = ide_get_features(0);
	features[27*2+0] = 0x54;
	features[27*2+1] = 0x53;
	features[28*2+0] = 0x31;
	features[28*2+1] = 0x39;
	features[29*2+0] = 0x30;
	features[29*2+1] = 0x35;
	features[30*2+0] = 0x47;
	features[30*2+1] = 0x41;
	features[31*2+0] = 0x20;
	features[31*2+1] = 0x20;

	/* optimize one of the non-standard loops */
	kinst_speedup = memory_install_read32_handler(0, ADDRESS_SPACE_PROGRAM, 0x8808f5bc, 0x8808f5bf, 0, 0, kinst_speedup_r);
}


static DRIVER_INIT( kinst2 )
{
	static const UINT8 kinst2_control_map[8] = { 2,4,1,0,3,5,6,7 };
	UINT8 *features;

	// read: $80 on ki2 = $90 on ki
	// read: $88 on ki2 = $a0 on ki
	// write: $80 on ki2 = $90 on ki
	// write: $90 on ki2 = $88 on ki
	// write: $98 on ki2 = $80 on ki
	// write: $a0 on ki2 = $98 on ki

	dcs_init();

	/* set up the control register mapping */
	control_map = kinst2_control_map;

	/* spin up the hard disk */
	ide_controller_init(0, &ide_intf);

	/* tweak the model number so we pass the check */
	features = ide_get_features(0);
	features[10*2+0] = 0x30;
	features[10*2+1] = 0x30;
	features[11*2+0] = 0x54;
	features[11*2+1] = 0x53;
	features[12*2+0] = 0x31;
	features[12*2+1] = 0x39;
	features[13*2+0] = 0x30;
	features[13*2+1] = 0x35;
	features[14*2+0] = 0x47;
	features[14*2+1] = 0x41;

	/* optimize one of the non-standard loops */
	kinst_speedup = memory_install_read32_handler(0, ADDRESS_SPACE_PROGRAM, 0x887ff544, 0x887ff547, 0, 0, kinst_speedup_r);
}



/*************************************
 *
 *  Game driver(s)
 *
 *************************************/

GAME( 1994, kinst,    0,      kinst, kinst,  kinst,   ROT0, "Rare", "Killer Instinct (v1.5d)", 0 )
GAME( 1994, kinst14,  kinst,  kinst, kinst2, kinst,   ROT0, "Rare", "Killer Instinct (v1.4)", 0 )
GAME( 1994, kinst13,  kinst,  kinst, kinst2, kinst,   ROT0, "Rare", "Killer Instinct (v1.3)", 0 )
GAME( 1994, kinstp,   kinst,  kinst, kinst2, kinst,   ROT0, "Rare", "Killer Instinct (proto v4.7)", 0 )

GAME( 1995, kinst2,   0,      kinst, kinst2, kinst2,  ROT0, "Rare", "Killer Instinct 2 (v1.4)", 0 )
GAME( 1995, kinst2k,  kinst2, kinst, kinst2, kinst2,  ROT0, "Rare", "Killer Instinct 2 (v1.4k, upgrade kit)", GAME_NOT_WORKING )
GAME( 1995, kinst213, kinst2, kinst, kinst2, kinst2,  ROT0, "Rare", "Killer Instinct 2 (v1.3)", 0 )
GAME( 1995, kinst211, kinst2, kinst, kinst2, kinst2,  ROT0, "Rare", "Killer Instinct 2 (v1.1)", 0 )
GAME( 1995, kinst210, kinst2, kinst, kinst2, kinst2,  ROT0, "Rare", "Killer Instinct 2 (v1.0)", 0 )
