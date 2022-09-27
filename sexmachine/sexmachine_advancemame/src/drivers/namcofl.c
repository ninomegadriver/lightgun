/*
    Namco System FL
    Driver by R. Belmont and ElSemi

    IMPORTANT
    ---------
    To calibrate the steering, do the following:
    1) Delete the .nv file
    2) Start the game.  Speed Racer will guide you through the calibration
       (the "jump button" is the same as player 1 start).
       For Final Lap R, hold down service (9) and tap test (F2).  If you do
       not get an "initializing" message followed by the input test, keep doing
       it until you do.
    3) Exit MAME and restart the game, it's now calibrated.


PCB Layout
----------

SYSTEM-FL MAIN PCB 8636960100 (8636970100)
       |--------------------------------------------------------------------------------------------|
       |                                                                       |-----------------|  |
       |                                                                       |       4Z IC3 IC1| Z|
       |                21Y 20Y 19Y 18Y     16Y     14Y 13Y 12Y                |       4Y        | Y|
       |                    20X                                                |       4X        | X|
    |--|                                                                       |          IC4 IC2| W|
    |               22U 21U         18U     16U     14U 13U 12U                |    Sub Board    | U|
    |               22T             18T                                        |-----------------| T|
    |                                                                                               |
    |J          23S                                         12S                        4S    2S    S|
    |                                                                                               |
    |A          23R                                     13R                                        R|
    |                                                                                               |
    |M                  21P 20P 19P 18P                             10P    8P 7P    5P             P|
    |                       20N                                                               OSC1 N|
    |M                                                                                    OSC2      |
    |           23L     21L                             OSC3                              3L       L|
    |A                                              14K         11K                                K|
    |                                                                                               |
    |                           19J 18J 17J 16J                                                    J|
    |--|        23F     21F                                                                        F|
       |                                                                                            |
    |--|        23E                                     13E                                        E|
    |N          23D             19D                     13D                               3D       D|
    |A                                                                                              |
    |M          23C                                                                                C|
    |C  25B             21B                                                                        B|
    |O                                                                                              |
    |4              22A 21A     19A 18A     16A 15A 14A 13A 12A 11A 10A 9A 8A 7A 6A 5A    3A       A|
    |4                                                                                              |
    |--|                                                                                            |
       |25  24  23  22  21  20  19  18  17  16  15  14  13  12  11  10  9  8  7  6  5  4  3  2  1   |
       |--------------------------------------------------------------------------------------------|

CUSTOM
------
4X : NAMCO C355 (sprite chip)
18Y: NAMCO 156
20X: NAMCO C116
21U: NAMCO 145
18T: NAMCO 123
23R: NAMCO C352
3D : NAMCO C380
4S : NAMCO 187
23L: NAMCO 75 (M37702 MCU)
11K: NAMCO 137
21B: NAMCO C345  9348EV  333791  VY06436A  NX25024K JAPAN
21F: NAMCO 195


ROMs - Main Board
-----------------
23S: MASK ROM - SE1_VOI.23S (PCB LABEL 'VOICE'), mounted on a small plug-in PCB
     labelled MEMEXT 32M MROM PCB 8635909200 (8635909300). This chip is programmed in BYTE mode.
18U: MB834000 MASK ROM - SE1_SSH.18U (PCB LABEL 'SSHAPE')
21P: MB838000 MASK ROM - SE1_SCH0.21P (PCB LABEL 'SCHA0')
20P: MB838000 MASK ROM - SE1_SCH1.20P (PCB LABEL 'SCHA1')
19P: MB838000 MASK ROM - SE1_SCH2.19P (PCB LABEL 'SCHA2')
18P: MB838000 MASK ROM - SE1_SCH3.18P (PCB LABEL 'SCHA3')
21L: M27C4002 EPROM - SE1_SPR.21L (PCB LABEL 'SPROG')
14K: MB834000 MASK ROM - SE1_RSH.14K (PCB LABEL 'RSHAPE')
19J: MB838000 MASK ROM - SE1_RCH0.19J (PCB LABEL 'RCHA0')
18J: MB838000 MASK ROM - SE1_RCH1.18J (PCB LABEL 'RCHA1')
17J, 16J: RCH2, RCH3 but sockets not populated
19A: D27C4096 EPROM - SE2MPEA4.19A (PCB LABEL 'PROGE')
18A: D27C4096 EPROM - SE2MPOA4.18A (PCB LABEL 'PROGO')
16A: AM27C040 EPROM - SE1_DAT3.16A (PCB LABEL 'DATA3')
15A: AM27C040 EPROM - SE1_DAT2.15A (PCB LABEL 'DATA2')
14A: AM27C040 EPROM - SE1_DAT1.14A (PCB LABEL 'DATA1')
13A: AM27C040 EPROM - SE1_DAT0.13A (PCB LABEL 'DATA0')


ROMs - Sub Board
----------------
IC1: MB8316200 SOP44 MASK ROM - SE1OBJ0L.IC1 (PCB LABEL 'OBJ0L')
IC2: MB8316200 SOP44 MASK ROM - SE1OBJ0U.IC2 (PCB LABEL 'OBJ0U')
IC3: MB8316200 SOP44 MASK ROM - SE1OBJ1L.IC3 (PCB LABEL 'OBJ1L')
IC4: MB8316200 SOP44 MASK ROM - SE1OBJ1U.IC4 (PCB LABEL 'OBJ1U')


PALs
----
2S : PAL16L8BCN (PCB LABEL 'SYSFL-1')
3L : PAL16L8BCN (PCB LABEL 'SYSFL-2')
12S: PALCE16V8H (PCB LABEL 'SYSFL-3')
20N: PAL20L8BCN (PCB LABEL 'SYSFL-4')
19D: PALCE16V8H (PCB LABEL 'SYSFL-5')


OTHER
-----
OSC1: 80.000MHz
OSC2: 27.800MHz
OSC3: 48.384MHz
13D : KEYCUS2 (not populated)
3A  : Intel NG80960KA-20 (i960 CPU)
25B : Sharp PC9D10
23C : IR2C24AN
23D : IR2C24AN
13E : HN58C65 EEPROM

*/

#include "driver.h"
#include "machine/eeprom.h"
#include "namconb1.h"
#include "namcos2.h"
#include "namcoic.h"
#include "cpu/i960/i960.h"
#include "sndhrdw/namcoc7x.h"

VIDEO_START( namcofl );
VIDEO_UPDATE( namcofl );

extern UINT32 *namcofl_spritebank32;
extern UINT32 *namcofl_mcuram;
extern WRITE32_HANDLER(namcofl_spritebank_w);

static UINT32 *namcofl_workram;

static READ32_HANDLER( fl_unk1_r )
{
	return 0xffffffff;
}

static READ32_HANDLER( fl_network_r )
{
	return 0xffffffff;
}

static READ32_HANDLER( namcofl_sysreg_r )
{
	return 0;
}

static WRITE32_HANDLER( namcofl_sysreg_w )
{
	if ((offset == 2) && !(mem_mask & 0xff))  // address space configuration
	{
		if (data == 0)	// RAM at 00000000, ROM at 10000000
		{
			memory_set_bankptr( 1, namcofl_workram );
			memory_set_bankptr( 2, memory_region(REGION_CPU1) );
		}
		else		// ROM at 00000000, RAM at 10000000
		{
			memory_set_bankptr( 1, memory_region(REGION_CPU1) );
			memory_set_bankptr( 2, namcofl_workram );
		}
	}
}

static ADDRESS_MAP_START( sysfl_mem, ADDRESS_SPACE_PROGRAM, 32 )
	AM_RANGE(0x00000000, 0x000fffff) AM_READWRITE(MRA32_BANK1, MWA32_BANK1)
	AM_RANGE(0x10000000, 0x100fffff) AM_READWRITE(MRA32_BANK2, MWA32_BANK2)
	AM_RANGE(0x20000000, 0x201fffff) AM_ROM AM_REGION(REGION_USER1, 0)	/* data */
	AM_RANGE(0x30000000, 0x30001fff) AM_RAM	AM_BASE((UINT32 **)&generic_nvram) AM_SIZE(&generic_nvram_size) /* nvram */
	AM_RANGE(0x30100000, 0x30100003) AM_WRITE(namcofl_spritebank_w)
	AM_RANGE(0x30284000, 0x3028bfff) AM_RAM	AM_BASE(&namcofl_mcuram) /* shared RAM with C75 MCU */
	AM_RANGE(0x30300000, 0x30303fff) AM_RAM /* COMRAM */
	AM_RANGE(0x30380000, 0x303800ff) AM_READ( fl_network_r )	/* network registers */
	AM_RANGE(0x30400000, 0x3040ffff) AM_RAM AM_BASE(&paletteram32)
	AM_RANGE(0x30800000, 0x3080ffff) AM_READWRITE(namco_tilemapvideoram32_le_r, namco_tilemapvideoram32_le_w )
	AM_RANGE(0x30a00000, 0x30a0003f) AM_READWRITE(namco_tilemapcontrol32_le_r, namco_tilemapcontrol32_le_w )
	AM_RANGE(0x30c00000, 0x30c1ffff) AM_READWRITE(namco_rozvideoram32_le_r,namco_rozvideoram32_le_w)
	AM_RANGE(0x30d00000, 0x30d0001f) AM_READWRITE(namco_rozcontrol32_le_r,namco_rozcontrol32_le_w)
	AM_RANGE(0x30e00000, 0x30e1ffff) AM_READWRITE(namco_obj32_le_r, namco_obj32_le_w)
	AM_RANGE(0x30f00000, 0x30f0000f) AM_RAM /* NebulaM2 code says this is int enable at 0000, int request at 0004, but doesn't do much about it */
	AM_RANGE(0x40000000, 0x4000005f) AM_READWRITE( namcofl_sysreg_r, namcofl_sysreg_w )
	AM_RANGE(0xfffffffc, 0xffffffff) AM_READ( fl_unk1_r )
ADDRESS_MAP_END

INPUT_PORTS_START( sysfl )
	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT(0x04, IP_ACTIVE_HIGH, IPT_SERVICE ) PORT_NAME( DEF_STR( Service_Mode )) PORT_TOGGLE PORT_CODE(KEYCODE_F2)
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_SERVICE1 )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )	// C75 status

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON3 )	// button C
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_BUTTON1 )	// button A
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_BUTTON2 )	// button B
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_START1 )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON4 ) PORT_TOGGLE	// shifter
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START
	PORT_BIT( 0xff, 0x00, IPT_PEDAL ) PORT_MINMAX(0x00,0xff) PORT_SENSITIVITY(100) PORT_KEYDELTA(10)

	PORT_START
	PORT_BIT( 0xff, 0x00, IPT_PEDAL2 ) PORT_MINMAX(0x00,0xff) PORT_SENSITIVITY(100) PORT_KEYDELTA(10)

	PORT_START
	PORT_BIT( 0xff, 0x80, IPT_PADDLE ) PORT_MINMAX(0x00,0xff) PORT_SENSITIVITY(100) PORT_KEYDELTA(10)
INPUT_PORTS_END

static const gfx_layout obj_layout =
{
	16,16,
	RGN_FRAC(1,1),
	8, /* bits per pixel */
	{
		/* plane offsets */
		0,1,2,3,4,5,6,7,
	},
	{
		0*16+8,1*16+8,0*16,1*16,
		2*16+8,3*16+8,2*16,3*16,
		4*16+8,5*16+8,4*16,5*16,
		6*16+8,7*16+8,6*16,7*16
	},
	{
		0x0*128,0x1*128,0x2*128,0x3*128,0x4*128,0x5*128,0x6*128,0x7*128,
		0x8*128,0x9*128,0xa*128,0xb*128,0xc*128,0xd*128,0xe*128,0xf*128
	},
	16*128
};

static const gfx_layout tile_layout =
{
	8,8,
	RGN_FRAC(1,1),
	8,
	{ 0,1,2,3,4,5,6,7 },
	{ 0*8,1*8,2*8,3*8,4*8,5*8,6*8,7*8 },
	{ 0*64,1*64,2*64,3*64,4*64,5*64,6*64,7*64 },
	8*64
};

static const gfx_layout roz_layout =
{
	16,16,
	RGN_FRAC(1,1),
	8,
	{ 0,1,2,3,4,5,6,7 },
	{ 0*8,1*8,2*8,3*8,4*8,5*8,6*8,7*8,8*8,9*8,10*8,11*8,12*8,13*8,14*8,15*8 },
	{
		0*128,1*128,2*128,3*128,4*128,5*128,6*128,7*128,8*128,9*128,10*128,11*128,12*128,13*128,14*128,15*128
	},
	16*128
};

static const gfx_decode gfxdecodeinfo2[] =
{
	{ NAMCONB1_TILEGFXREGION,	0, &tile_layout,	0x1000, 0x08 },
	{ NAMCONB1_SPRITEGFXREGION,	0, &obj_layout,		0x0000, 0x10 },
	{ NAMCONB1_ROTGFXREGION,	0, &roz_layout,		0x1800, 0x08 },
	{ -1 }
};


static INTERRUPT_GEN(namcofl_interrupt)
{
	int currentscanline=224-cpu_getiloops();
	int v=(paletteram32[0x1808/4]>>16)&0xffff;
	int triggerscanline=(((v>>8)&0xff)|((v&0xff)<<8))-(32+1);

	if (triggerscanline==currentscanline)
	{
		force_partial_update(currentscanline);
		cpunum_set_input_line(0, I960_IRQ1, ASSERT_LINE);
	}


	switch (cpu_getiloops())
	{
		case 0:
			cpunum_set_input_line(0, I960_IRQ2, ASSERT_LINE); //VSYNC
			break;
		case 2:
			cpunum_set_input_line(0, I960_IRQ0, ASSERT_LINE); //Network
			break;
	}
}
NAMCO_C7X_HARDWARE

static MACHINE_DRIVER_START( sysfl )
	MDRV_CPU_ADD(I960, 20000000)	// i80960KA-20 == 20 MHz part
	MDRV_CPU_PROGRAM_MAP(sysfl_mem, 0)
	MDRV_CPU_VBLANK_INT(namcofl_interrupt, 224)

	NAMCO_C7X_MCU_SHARED( 16384000 )

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	MDRV_NVRAM_HANDLER(generic_1fill)

	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER | VIDEO_UPDATE_AFTER_VBLANK/* | VIDEO_RGB_DIRECT*/)
	MDRV_SCREEN_SIZE(NAMCONB1_COLS*8, NAMCONB1_ROWS*8) /* 288x224 pixels */
	MDRV_VISIBLE_AREA(0*8, NAMCONB1_COLS*8-1, 0*8, NAMCONB1_ROWS*8-1)
	MDRV_PALETTE_LENGTH(8192)

	MDRV_GFXDECODE(gfxdecodeinfo2)

	MDRV_VIDEO_START(namcofl)
	MDRV_VIDEO_UPDATE(namcofl)

	NAMCO_C7X_SOUND( 16384000 )
MACHINE_DRIVER_END

ROM_START( speedrcr )
	ROM_REGION( 0x200000, REGION_CPU1, 0 ) // i960 program
	ROM_LOAD32_WORD("se2mpea4.19a",   0x000000, 0x080000, CRC(95ab3fd7) SHA1(273a536f8512f3c55260ac1b78533bc35b8390ed) )
	ROM_LOAD32_WORD("se2mpoa4.18a",   0x000002, 0x080000, CRC(5b5ef1eb) SHA1(3e9e4abb1a32269baef772079de825dfe1ea230c) )

	ROM_REGION( 0x200000, REGION_USER1, 0 ) // Data
	ROM_LOAD32_BYTE("se1_dat0.13a",   0x000000, 0x080000, CRC(cc5d6ff5) SHA1(6fad40a1fac75bc64d3b7a7562cf7ce2a3abd36a) )
	ROM_LOAD32_BYTE("se1_dat1.14a",   0x000001, 0x080000, CRC(ddc8b306) SHA1(f169d521b800c108deffdef9fc6b0058621ee909) )
	ROM_LOAD32_BYTE("se1_dat2.15a",   0x000002, 0x080000, CRC(2a29abbb) SHA1(945419ed61e9a656a340214a63a01818396fbe98) )
	ROM_LOAD32_BYTE("se1_dat3.16a",   0x000003, 0x080000, CRC(49849aff) SHA1(b7c7eea1d56304e40e996ee998c971313ff03614) )

	ROM_REGION( 0x200000, NAMCONB1_ROTGFXREGION, 0 )	// "RCHAR" (roz characters)
	ROM_LOAD("se1_rch0.19j",   0x000000, 0x100000, CRC(a0827288) SHA1(13691ef4d402a6dc91851de4f82cfbdf96d417cb) )
	ROM_LOAD("se1_rch1.18j",   0x100000, 0x100000, CRC(af7609ad) SHA1(b16041f0eb47d7566011d9d762a3083411dc422e) )

	ROM_REGION( 0x400000, NAMCONB1_TILEGFXREGION, 0 ) // "SCHAR" (regular BG characters)
	ROM_LOAD("se1_sch0.21p",   0x000000, 0x100000, CRC(7b5cfad0) SHA1(5a0355e37eb191bc0cf8b6b7c3d0274560b9bbd5) )
	ROM_LOAD("se1_sch1.20p",   0x100000, 0x100000, CRC(5086e0d3) SHA1(0aa7d11f4f9a75117e69cc77f1b73a68d9007aef) )
	ROM_LOAD("se1_sch2.19p",   0x200000, 0x100000, CRC(e59a731e) SHA1(3fed72e9bb485d4d689ab51490360c4c6f1dc5cb) )
	ROM_LOAD("se1_sch3.18p",   0x300000, 0x100000, CRC(f817027a) SHA1(71745476f496c60d89c8563b3e46bc85eebc79ce) )

	ROM_REGION( 0x800000, NAMCONB1_SPRITEGFXREGION, 0 )	// OBJ
	ROM_LOAD16_BYTE("se1obj0l.ic1", 0x000001, 0x200000, CRC(17585218) SHA1(3332afa9bd194ac37b8d6f352507c523a0f2e2b3) )
	ROM_LOAD16_BYTE("se1obj0u.ic2", 0x000000, 0x200000, CRC(d14b1236) SHA1(e5447732ef3acec88fb7a00e0deca3e71a40ae65) )
	ROM_LOAD16_BYTE("se1obj1l.ic3", 0x400001, 0x200000, CRC(c4809fd5) SHA1(e0b80fccc17c83fb9d08f7f1cf2cd2f0f3a510b4) )
	ROM_LOAD16_BYTE("se1obj1u.ic4", 0x400000, 0x200000, CRC(0beefa56) SHA1(012fb7b330dbf851ab2217da0a0e7136ddc3d23f) )

	ROM_REGION( 0x100000, NAMCONB1_ROTMASKREGION, 0 ) // "RSHAPE" (roz mask like NB-1?)
	ROM_LOAD("se1_rsh.14k",    0x000000, 0x100000, CRC(7aa5a962) SHA1(ff936dfcfcc4ee1f5f2232df62def76ff99e671e) )

	ROM_REGION( 0x100000, NAMCONB1_TILEMASKREGION, 0 ) // "SSHAPE" (mask for other tiles?)
	ROM_LOAD("se1_ssh.18u",    0x000000, 0x100000, CRC(7a8e0bda) SHA1(f6a508d90274d0205fec0c46f5f783a2715c0c6e) )

	ROM_REGION( 0x100000, REGION_USER4, 0 ) /* sound data and MCU BIOS */
	ROM_LOAD("se1_spr.21l",   0x000000,  0x80000, CRC(850a27ac) SHA1(7d5db840ec67659a1f2e69a62cdb03ce6ee0b47b) )
	NAMCO_C7X_BIOS

	ROM_REGION( 0x400000, REGION_SOUND1, 0 ) // Samples
	ROM_LOAD("se1_voi.23s",   0x000000, 0x400000, CRC(b95e2ffb) SHA1(7669232d772caa9afa4c7593d018e8b6e534114a) )
ROM_END

ROM_START( finalapr )
	ROM_REGION( 0x200000, REGION_CPU1, 0 ) // i960 program
	ROM_LOAD32_WORD("flr2mpeb.19a",   0x000000, 0x080000, CRC(8bfe615f) SHA1(7b867eb261268a83177f1f873689f77d1b6c47ca) )
	ROM_LOAD32_WORD("flr2mpob.18a",   0x000002, 0x080000, CRC(91c14e4f) SHA1(934a86daaef0e3e2c2b3066f4677ccb3aaab6eaf) )

	ROM_REGION( 0x200000, REGION_USER1, ROMREGION_ERASEFF ) // Data

	ROM_REGION( 0x200000, NAMCONB1_ROTGFXREGION, 0 )	// "RCHAR" (roz characters)
	ROM_LOAD("flr1rch0.19j",   0x000000, 0x100000, CRC(f413f50d) SHA1(cdd8073dda4feaea78e3b94520cf20a9799fd04d) )
	ROM_LOAD("flr1rch1.18j",   0x100000, 0x100000, CRC(4654d519) SHA1(f8bb473013cdca48dd98df0de2f78c300c156e91) )

	ROM_REGION( 0x400000, NAMCONB1_TILEGFXREGION, 0 ) // "SCHAR" (regular BG characters)
	ROM_LOAD("flr1sch0.21p",   0x000000, 0x100000, CRC(7169efca) SHA1(66c7aa1b50b236b4700b07be0dca7aebdabedb8c) )
	ROM_LOAD("flr1sch1.20p",   0x100000, 0x100000, CRC(aa233a02) SHA1(0011329f585658d90f820daf0ba08ce2735bddfc) )
	ROM_LOAD("flr1sch2.19p",   0x200000, 0x100000, CRC(9b6b7abd) SHA1(5cdec70db1b46bc5d0866ca155b520157fef3adf) )
	ROM_LOAD("flr1sch3.18p",   0x300000, 0x100000, CRC(50a14f54) SHA1(ab9c2f2e11f006a9dc7e5aedd5788d7d67166d36) )

	ROM_REGION( 0x800000, NAMCONB1_SPRITEGFXREGION, 0 )	// OBJ
	ROM_LOAD16_BYTE("flr1obj0l.ic1", 0x000001, 0x200000, CRC(364a902c) SHA1(4a1ea48eee86d410e36096cc100b4c9a5a645034) )
	ROM_LOAD16_BYTE("flr1obj0u.ic2", 0x000000, 0x200000, CRC(a5c7b80e) SHA1(4e0e863cfdd8c051c3c4594bb21e11fb93c28f0c) )
	ROM_LOAD16_BYTE("flr1obj1l.ic3", 0x400001, 0x200000, CRC(51fd8de7) SHA1(b1571c45e8c33d746716fd790c704a3361d02bdc) )
	ROM_LOAD16_BYTE("flr1obj1u.ic4", 0x400000, 0x200000, CRC(1737aa3c) SHA1(8eaf0dc5d60a270d2c1626f54f5edbddbb0a59c8) )

	ROM_REGION( 0x80000, NAMCONB1_ROTMASKREGION, 0 ) // "RSHAPE" (roz mask like NB-1?)
	ROM_LOAD("flr1rsh.14k",    0x000000, 0x080000, CRC(037c0983) SHA1(c48574a8ad125cedfaf2538c5ff824e121204629) )

	ROM_REGION( 0x80000, NAMCONB1_TILEMASKREGION, 0 ) // "SSHAPE" (mask for other tiles?)
	ROM_LOAD("flr1ssh.18u",    0x000000, 0x080000, CRC(f70cb2bf) SHA1(dbddda822287783a43415172b81d0382a8ac43d8) )

	ROM_REGION( 0x100000, REGION_USER4, 0 ) /* sound data and MCU BIOS */
	ROM_LOAD("flr1spr.21l",   0x000000,  0x20000, CRC(69bb0f5e) SHA1(6831d618de42a165e508ad37db594d3aa290c530) )
	NAMCO_C7X_BIOS

	ROM_REGION( 0x200000, REGION_SOUND1, 0 ) // Samples
	ROM_LOAD("flr1voi.23s",   0x000000, 0x200000, CRC(ff6077cd) SHA1(73c289125ddeae3e43153e4c570549ca04501262) )
ROM_END

ROM_START( finalapo )
	ROM_REGION( 0x200000, REGION_CPU1, 0 ) // i960 program
	ROM_LOAD32_WORD("flr2mpe.19a",   0x000000, 0x080000, CRC(cc8961ae) SHA1(08ce4d27a723101370d1c536b26256ce0d8a1b6c) )
	ROM_LOAD32_WORD("flr2mpo.18a",   0x000002, 0x080000, CRC(8118f465) SHA1(c4b79878a82fd36b5707e92aa893f69c2b942d57) )

	ROM_REGION( 0x200000, REGION_USER1, ROMREGION_ERASEFF ) // Data

	ROM_REGION( 0x200000, NAMCONB1_ROTGFXREGION, 0 )	// "RCHAR" (roz characters)
	ROM_LOAD("flr1rch0.19j",   0x000000, 0x100000, CRC(f413f50d) SHA1(cdd8073dda4feaea78e3b94520cf20a9799fd04d) )
	ROM_LOAD("flr1rch1.18j",   0x100000, 0x100000, CRC(4654d519) SHA1(f8bb473013cdca48dd98df0de2f78c300c156e91) )

	ROM_REGION( 0x400000, NAMCONB1_TILEGFXREGION, 0 ) // "SCHAR" (regular BG characters)
	ROM_LOAD("flr1sch0.21p",   0x000000, 0x100000, CRC(7169efca) SHA1(66c7aa1b50b236b4700b07be0dca7aebdabedb8c) )
	ROM_LOAD("flr1sch1.20p",   0x100000, 0x100000, CRC(aa233a02) SHA1(0011329f585658d90f820daf0ba08ce2735bddfc) )
	ROM_LOAD("flr1sch2.19p",   0x200000, 0x100000, CRC(9b6b7abd) SHA1(5cdec70db1b46bc5d0866ca155b520157fef3adf) )
	ROM_LOAD("flr1sch3.18p",   0x300000, 0x100000, CRC(50a14f54) SHA1(ab9c2f2e11f006a9dc7e5aedd5788d7d67166d36) )

	ROM_REGION( 0x800000, NAMCONB1_SPRITEGFXREGION, 0 )	// OBJ
	ROM_LOAD16_BYTE("flr1obj0l.ic1", 0x000001, 0x200000, CRC(364a902c) SHA1(4a1ea48eee86d410e36096cc100b4c9a5a645034) )
	ROM_LOAD16_BYTE("flr1obj0u.ic2", 0x000000, 0x200000, CRC(a5c7b80e) SHA1(4e0e863cfdd8c051c3c4594bb21e11fb93c28f0c) )
	ROM_LOAD16_BYTE("flr1obj1l.ic3", 0x400001, 0x200000, CRC(51fd8de7) SHA1(b1571c45e8c33d746716fd790c704a3361d02bdc) )
	ROM_LOAD16_BYTE("flr1obj1u.ic4", 0x400000, 0x200000, CRC(1737aa3c) SHA1(8eaf0dc5d60a270d2c1626f54f5edbddbb0a59c8) )

	ROM_REGION( 0x80000, NAMCONB1_ROTMASKREGION, 0 ) // "RSHAPE" (roz mask like NB-1?)
	ROM_LOAD("flr1rsh.14k",    0x000000, 0x080000, CRC(037c0983) SHA1(c48574a8ad125cedfaf2538c5ff824e121204629) )

	ROM_REGION( 0x80000, NAMCONB1_TILEMASKREGION, 0 ) // "SSHAPE" (mask for other tiles?)
	ROM_LOAD("flr1ssh.18u",    0x000000, 0x080000, CRC(f70cb2bf) SHA1(dbddda822287783a43415172b81d0382a8ac43d8) )

	ROM_REGION( 0x100000, REGION_USER4, 0 ) /* sound data and MCU BIOS */
	ROM_LOAD("flr1spr.21l",   0x000000,  0x20000, CRC(69bb0f5e) SHA1(6831d618de42a165e508ad37db594d3aa290c530) )
	NAMCO_C7X_BIOS

	ROM_REGION( 0x200000, REGION_SOUND1, 0 ) // Samples
	ROM_LOAD("flr1voi.23s",   0x000000, 0x200000, CRC(ff6077cd) SHA1(73c289125ddeae3e43153e4c570549ca04501262) )
ROM_END

static void namcofl_common_init(void)
{
	namcofl_workram = auto_malloc(0x100000);

	memory_set_bankptr( 1, memory_region(REGION_CPU1) );
	memory_set_bankptr( 2, namcofl_workram );

	namcoc7x_on_driver_init();
	namcoc7x_set_host_ram(namcofl_mcuram);
}

static DRIVER_INIT(speedrcr)
{
	namcofl_common_init();
	namcos2_gametype = NAMCOFL_SPEED_RACER;
}

static DRIVER_INIT(finalapr)
{
	namcofl_common_init();
	namcos2_gametype = NAMCOFL_FINAL_LAP_R;
}

GAME( 1995, speedrcr,        0, sysfl, sysfl, speedrcr, ROT0, "Namco", "Speed Racer", GAME_IMPERFECT_GRAPHICS | GAME_IMPERFECT_SOUND )
GAME( 1995, finalapr,        0, sysfl, sysfl, finalapr, ROT0, "Namco", "Final Lap R (Rev B)", GAME_IMPERFECT_SOUND )
GAME( 1995, finalapo, finalapr, sysfl, sysfl, finalapr, ROT0, "Namco", "Final Lap R", GAME_IMPERFECT_SOUND )
