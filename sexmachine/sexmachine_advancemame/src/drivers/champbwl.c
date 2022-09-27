/*

Championship Bowling
Romstar Inc., 1989

This game runs on Seta Hardware.

PCB Layout
----------

PO-052A
|---------------------------------------------------------|
| MB3712     SW1        AB001009  AB001007  AB001005      |
|                 X1-007                                  |
|        VOL                          AB001006  AB001004  |
|                                                         |
|                       AB001008                          |
|                                                         |
|        4050                                             |
|                                                         |
|      CN1                             X1-002A            |
|                                                         |
|J                                                3V_BATT |
|A                        2063         X1-001A            |
|M                                                        |
|M                                                  SW2   |
|A         16MHz                                          |
|                            4520                         |
|                                                         |
|                                                  6116   |
|       X1-010                                            |
|                                                         |
|      |---------------ROM-Sub-Board----------------|     |
|  3404|                                            |     |
|      |    DIP32                                   |     |
|      |                                          AB001001|
|      |          6116                       Z80    |     |
|      |                                            |     |
|      |                                            |     |
|------|--------------------------------------------|-----|


ROM Sub Board (plugs into DIP32 socket on main board)
-------------

PO-047A
|--------------------------------------------|
|                                            |
|     AB003003           AB002003            |
|                                            |
|              AB003002            AB002002  |
|                                            |
| 74HC139                                    |
|--------------------------------------------|

Notes:
      Z80 clock     - 4.000MHz [16/4]
      X1-010 clocks - pin1 16.000MHz, pin2 8.000MHz, pin79 4.000MHz, pin80 2.000MHz
      VSync         - 57.5Hz
      2063          - Toshiba TMM2063AP-10 8K x8 SRAM (DIP28)
      6116          - Hitachi 6116LP-2 2K x8 SRAM (DIP24)
      MB3712        - Fujitsu MB3712 5.7 Watt Power AMP (SIP8)
      4050          - Toshiba TC4050BP Non-Inverting Hex Buffer / Converter (DIP16)
      4520          - Hitachi HD14520 Dual Up Counter (DIP16)
      3404          - Japan Radio Co. JRC3404 Low Power Quad Op AMP (DIP8)
      SW1           - Reset Switch
      SW2           - 2-Position Switch (when ON, powers 6116 RAM near it)
      3V_BATT       - Sony CR2032 3V Lithium Coin Battery
      DIP32         - Empty DIP32 socket for connection of ROM Sub Board
      CN1           - 4-pin Connector

      Custom IC's -
                    X1-001A (SDIP64) \ Sprite Generators
                    X1-002A (SDIP64) /
                    X1-007  (SDIP42)   Video DAC? (connected to RGB output)
                    X1-010  (QFP80)    Sound Chip, 16Bit PCM

      ROMs -
            Filename        Type                Use
            -----------------------------------------------
            AB001001.U1    27C1000 (DIP32)      Z80 Program

            AB001004.U7    512K MaskROM (DIP28) \
            AB001005.U9    512K MaskROM (DIP28) | GFX
            AB001006.U15   512K MaskROM (DIP28) |
            AB001007.U22   512K MaskROM (DIP28) /

            AB001008.U26   82S147 PROM (DIP20)
            AB001009.U27   82S147 PROM (DIP20)

            AB002002.2-2   2M MaskROM (DIP32)   \
            AB002003.2-3   2M MaskROM (DIP32)   | PCM Samples (Connected to X1-010 via a sub-board)
            AB003002.3-2   2M MaskROM (DIP32)   |
            AB003003.3-3   2M MaskROM (DIP32)   /

  Driver by Pierpaolo Prazzoli

*/

#include "driver.h"
#include "sound/x1_010.h"

extern unsigned char *tnzs_objram, *tnzs_sharedram;
extern unsigned char *tnzs_vdcram, *tnzs_scrollram, *tnzs_objctrl;

PALETTE_INIT( arknoid2 );
VIDEO_UPDATE( tnzs );
VIDEO_EOF( tnzs );

static UINT8 last_trackball_val[2] = {0,0};

static READ8_HANDLER( trackball_r )
{
	UINT8 ret;
	UINT8 port4 = readinputport(4);
	UINT8 port5 = readinputport(5);

	ret = (((port4 - last_trackball_val[0]) & 0x0f)<<4) | ((port5 - last_trackball_val[1]) & 0x0f);

	last_trackball_val[0] = port4;
	last_trackball_val[1] = port5;

	return ret;
}

static WRITE8_HANDLER( champbwl_misc_w )
{
	coin_counter_w(0, data & 1);
	coin_counter_w(1, data & 2);

	coin_lockout_w(0, ~data & 8);
	coin_lockout_w(1, ~data & 4);

	memory_set_bankptr(1, memory_region(REGION_CPU1) + 0x10000 + 0x4000 * ((data & 0x30)>>4));
}

static WRITE8_HANDLER( champbwl_objctrl_w )
{
	if(offset != 0)
		data ^= 0xff;

	tnzs_objctrl[offset] = data;
}

static ADDRESS_MAP_START( champbwl_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x3fff) AM_ROM AM_REGION(REGION_CPU1, 0x10000)
	AM_RANGE(0x4000, 0x7fff) AM_ROMBANK(1)
	AM_RANGE(0x8000, 0x87ff) AM_RAM AM_BASE(&generic_nvram) AM_SIZE(&generic_nvram_size)
	AM_RANGE(0xa000, 0xbfff) AM_RAM AM_BASE(&tnzs_objram)
	AM_RANGE(0xc000, 0xdfff) AM_READWRITE(seta_sound_r, seta_sound_w)
	AM_RANGE(0xe000, 0xe1ff) AM_RAM AM_BASE(&tnzs_vdcram)
	AM_RANGE(0xe200, 0xe2ff) AM_RAM AM_BASE(&tnzs_scrollram) /* scrolling info */
	AM_RANGE(0xe300, 0xe303) AM_MIRROR(0xfc) AM_WRITE(champbwl_objctrl_w) AM_BASE(&tnzs_objctrl) /* control registers (0x80 mirror used by Arkanoid 2) */

	AM_RANGE(0xe800, 0xe800) AM_WRITENOP

	AM_RANGE(0xf000, 0xf000) AM_READ(trackball_r)
	AM_RANGE(0xf002, 0xf002) AM_READ(input_port_0_r)
	AM_RANGE(0xf004, 0xf004) AM_READ(input_port_1_r)
	AM_RANGE(0xf006, 0xf006) AM_READ(input_port_2_r)
	AM_RANGE(0xf007, 0xf007) AM_READ(input_port_3_r)

	AM_RANGE(0xf000, 0xf000) AM_WRITE(champbwl_misc_w)
	AM_RANGE(0xf002, 0xf002) AM_WRITENOP //buttons light?
	AM_RANGE(0xf004, 0xf004) AM_WRITENOP //buttons light?
	AM_RANGE(0xf006, 0xf006) AM_WRITENOP //buttons light?
	AM_RANGE(0xf800, 0xf800) AM_WRITENOP
ADDRESS_MAP_END

INPUT_PORTS_START( champbwl )
	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_NAME("Player Change")
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_SPECIAL ) // INT( 4M)
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_SPECIAL ) // INT(16M)

	PORT_START
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START
	PORT_SERVICE( 0x01, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Free_Play ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x0c, 0x0c, "License" )
	PORT_DIPSETTING(    0x00, "Romstar (1)")
	PORT_DIPSETTING(    0x04, "Romstar (2)")
	PORT_DIPSETTING(    0x08, "Seta U.S.A, Romstar License" )
	PORT_DIPSETTING(    0x0c, "Seta" )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START	/* FAKE */
	PORT_BIT( 0xff, 0x00, IPT_TRACKBALL_X )PORT_SENSITIVITY(50) PORT_KEYDELTA(50) PORT_CENTERDELTA(0)

	PORT_START	/* FAKE */
	PORT_BIT( 0xff, 0x00, IPT_TRACKBALL_Y ) PORT_SENSITIVITY(50) PORT_KEYDELTA(45) PORT_CENTERDELTA(0) PORT_REVERSE
INPUT_PORTS_END

static const gfx_layout charlayout =
{
	16,16,
	RGN_FRAC(1,4),
	4,
	{ RGN_FRAC(3,4), RGN_FRAC(2,4), RGN_FRAC(1,4), RGN_FRAC(0,4) },
	{ 0, 1, 2, 3, 4, 5, 6, 7,
			8*8+0, 8*8+1, 8*8+2, 8*8+3, 8*8+4, 8*8+5, 8*8+6, 8*8+7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8,
			16*8, 17*8, 18*8, 19*8, 20*8, 21*8, 22*8, 23*8 },
	32*8
};

static const gfx_decode gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &charlayout, 0, 32 },
	{ -1 }	/* end of array */
};

static struct x1_010_interface champbwl_sound_intf =
{
	0x0000		/* address */
};

static MACHINE_DRIVER_START( champbwl )

	/* basic machine hardware */
	MDRV_CPU_ADD(Z80, 16000000/4) /* 4MHz */
	MDRV_CPU_PROGRAM_MAP(champbwl_map,0)
	MDRV_CPU_VBLANK_INT(irq0_line_pulse,1)

	MDRV_FRAMES_PER_SECOND(57.5)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	MDRV_NVRAM_HANDLER(generic_0fill)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(64*8, 32*8)
	MDRV_VISIBLE_AREA(0*8, 48*8-1, 1*8, 31*8-1)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(512)

	MDRV_PALETTE_INIT(arknoid2)
	MDRV_VIDEO_UPDATE(tnzs)
	MDRV_VIDEO_EOF(tnzs)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_STEREO("left", "right")

	MDRV_SOUND_ADD(X1_010, 16000000)
	MDRV_SOUND_CONFIG(champbwl_sound_intf)
	MDRV_SOUND_ROUTE(0, "left", 1.0)
	MDRV_SOUND_ROUTE(1, "right", 1.0)
MACHINE_DRIVER_END

ROM_START( champbwl )
	ROM_REGION( 0x20000, REGION_CPU1, 0 )		/* Z80 Code */
	ROM_LOAD( "ab001001.u1",  0x10000, 0x10000, CRC(6c6f7675) SHA1(19834f25f2644ae5d156c1e1bbb3fc50cae10fd2) )

	ROM_REGION( 0x80000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "ab001007.u22", 0x00000, 0x20000, CRC(1ee9f6b1) SHA1(1a67e969b1f471ec7ada294b89185c15cde8c1ab) )
	ROM_LOAD( "ab001006.u15", 0x20000, 0x20000, CRC(37baf753) SHA1(efa57d915a9e14393b62b161e1ac807b8fcb8501) )
	ROM_LOAD( "ab001005.u9",  0x40000, 0x20000, CRC(b80a9ed6) SHA1(ac7a31ad82a60c4d2034770c59cf383b8a036e6a) )
	ROM_LOAD( "ab001004.u7",  0x60000, 0x20000, CRC(584477b1) SHA1(296f96526044e9bd13673e5d817260e3f98f696c) )

	ROM_REGION( 0x0400, REGION_PROMS, 0 )
	ROM_LOAD( "ab001008.u26", 0x0000, 0x0200, CRC(30ac8d48) SHA1(af034de3f3b8548534effdf4e3717fe3838b7754) )
	ROM_LOAD( "ab001009.u27", 0x0200, 0x0200, CRC(3bbd4bcd) SHA1(8c87ccc42ece2432b8ad25f8679cdf886e12a43c) )

	ROM_REGION( 0x100000, REGION_SOUND1, 0 )	/* Samples */
	ROM_LOAD( "ab003003.3-3", 0x00000, 0x40000, CRC(ad40ad10) SHA1(db0e5744ea3fcda87345b545031f82fcb3fec175) )
	ROM_LOAD( "ab003002.3-2", 0x40000, 0x40000, CRC(7ede8f28) SHA1(b5519c09b4f0019dc76cadca725da1d581912540) )
	ROM_LOAD( "ab002003.2-3", 0x80000, 0x40000, CRC(3051b8c3) SHA1(5f53596d7af1c79db1dde4bdca3878e07c67b5d1) )
	ROM_LOAD( "ab002002.2-2", 0xc0000, 0x40000, CRC(42ebe997) SHA1(1808b9e5e996a395c1d48ac001067f736f96feec) )
ROM_END

GAME( 1989, champbwl, 0, champbwl, champbwl, 0, ROT270, "Seta / Romstar Inc.", "Championship Bowling", 0 )
