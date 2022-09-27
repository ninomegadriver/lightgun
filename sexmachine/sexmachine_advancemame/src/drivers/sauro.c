/***************************************************************************

Sauro
-----

driver by Zsolt Vasvari

Main CPU
--------

Memory mapped:

0000-dfff   ROM
e000-e7ff   RAM
e800-ebff   Sprite RAM
f000-fbff   Background Video RAM
f400-ffff   Background Color RAM
f800-fbff   Foreground Video RAM
fc00-ffff   Foreground Color RAM

Ports:

00      R   DSW #1
20      R   DSW #2
40      R   Input Ports Player 1
60      R   Input Ports Player 2
80       W  Sound Commnand
c0       W  Flip Screen
c1       W  ???
c2-c4    W  ???
c6-c7    W  ??? (Loads the sound latch?)
c8       W  ???
c9       W  ???
ca-cd    W  ???
ce       W  ???
e0       W  Watchdog


Sound CPU
---------

Memory mapped:

0000-7fff       ROM
8000-87ff       RAM
a000         W  ADPCM trigger
c000-c001    W  YM3812
e000        R   Sound latch
e000-e006    W  ???
e00e-e00f    W  ???


TODO
----

- The readme claims there is a GI-SP0256A-AL ADPCM on the PCB. Needs to be
  emulated.

- Verify all clock speeds

- I'm only using colors 0-15. The other 3 banks are mostly the same, but,
  for example, the color that's used to paint the gradients of the sky (color 2)
  is different, so there might be a palette select. I don't see anything
  obviously wrong the way it is right now. It matches the screen shots found
  on the Spanish Dump site.

- What do the rest of the ports in the range c0-ce do?

Tricky Doc
----------

Addition by Reip

***************************************************************************/

#include "driver.h"
#include "cpu/z80/z80.h"
#include "sound/3812intf.h"

extern UINT8 *tecfri_videoram;
extern UINT8 *tecfri_colorram;
extern UINT8 *tecfri_videoram2;
extern UINT8 *tecfri_colorram2;

extern WRITE8_HANDLER( tecfri_videoram_w );
extern WRITE8_HANDLER( tecfri_colorram_w );
extern WRITE8_HANDLER( tecfri_videoram2_w );
extern WRITE8_HANDLER( tecfri_colorram2_w );
extern WRITE8_HANDLER( tecfri_scroll_bg_w );
extern WRITE8_HANDLER( flip_screen_w );
extern WRITE8_HANDLER( sauro_scroll_fg_w );
extern WRITE8_HANDLER( trckydoc_spriteram_mirror_w );

extern VIDEO_START( sauro );
extern VIDEO_START( trckydoc );

extern VIDEO_UPDATE( sauro );
extern VIDEO_UPDATE( trckydoc );


static WRITE8_HANDLER( sauro_sound_command_w )
{
	data |= 0x80;
	soundlatch_w(offset, data);
}

static READ8_HANDLER( sauro_sound_command_r )
{
	int ret	= soundlatch_r(offset);
	soundlatch_clear_w(offset, 0);
	return ret;
}

static WRITE8_HANDLER( sauro_coin1_w )
{
	coin_counter_w(0, data);
	coin_counter_w(0, 0); // to get the coin counter working in sauro, as it doesn't write 0
}

static WRITE8_HANDLER( sauro_coin2_w )
{
	coin_counter_w(1, data);
	coin_counter_w(1, 0); // to get the coin counter working in sauro, as it doesn't write 0
}

static ADDRESS_MAP_START( sauro_readmem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0xdfff) AM_READ(MRA8_ROM)
	AM_RANGE(0xe000, 0xebff) AM_READ(MRA8_RAM)
	AM_RANGE(0xf000, 0xffff) AM_READ(MRA8_RAM)
ADDRESS_MAP_END

static ADDRESS_MAP_START( sauro_writemem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0xdfff) AM_WRITE(MWA8_ROM)
	AM_RANGE(0xe000, 0xe7ff) AM_WRITE(MWA8_RAM)
	AM_RANGE(0xe800, 0xebff) AM_WRITE(MWA8_RAM) AM_BASE(&spriteram) AM_SIZE(&spriteram_size)
	AM_RANGE(0xf000, 0xf3ff) AM_WRITE(tecfri_videoram_w) AM_BASE(&tecfri_videoram)
	AM_RANGE(0xf400, 0xf7ff) AM_WRITE(tecfri_colorram_w) AM_BASE(&tecfri_colorram)
	AM_RANGE(0xf800, 0xfbff) AM_WRITE(tecfri_videoram2_w) AM_BASE(&tecfri_videoram2)
	AM_RANGE(0xfc00, 0xffff) AM_WRITE(tecfri_colorram2_w) AM_BASE(&tecfri_colorram2)
ADDRESS_MAP_END

static ADDRESS_MAP_START( sauro_readport, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_FLAGS( AMEF_ABITS(8) )
	AM_RANGE(0x00, 0x00) AM_READ(input_port_2_r)
	AM_RANGE(0x20, 0x20) AM_READ(input_port_3_r)
	AM_RANGE(0x40, 0x40) AM_READ(input_port_0_r)
	AM_RANGE(0x60, 0x60) AM_READ(input_port_1_r)
ADDRESS_MAP_END

static ADDRESS_MAP_START( sauro_writeport, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_FLAGS( AMEF_ABITS(8) )
	AM_RANGE(0xa0, 0xa0) AM_WRITE(tecfri_scroll_bg_w)
	AM_RANGE(0xa1, 0xa1) AM_WRITE(sauro_scroll_fg_w)
	AM_RANGE(0x80, 0x80) AM_WRITE(sauro_sound_command_w)
	AM_RANGE(0xc0, 0xc0) AM_WRITE(flip_screen_w)
	AM_RANGE(0xc1, 0xc2) AM_WRITE(MWA8_NOP)
	AM_RANGE(0xc3, 0xc3) AM_WRITE(sauro_coin1_w)
	AM_RANGE(0xc4, 0xc4) AM_WRITE(MWA8_NOP)
	AM_RANGE(0xc5, 0xc5) AM_WRITE(sauro_coin2_w)
	AM_RANGE(0xc6, 0xce) AM_WRITE(MWA8_NOP)
	AM_RANGE(0xe0, 0xe0) AM_WRITE(watchdog_reset_w)
ADDRESS_MAP_END

static ADDRESS_MAP_START( sauro_sound_readmem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x7fff) AM_READ(MRA8_ROM)
	AM_RANGE(0x8000, 0x87ff) AM_READ(MRA8_RAM)
	AM_RANGE(0xe000, 0xe000) AM_READ(sauro_sound_command_r)
ADDRESS_MAP_END

static ADDRESS_MAP_START( sauro_sound_writemem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x8000, 0x87ff) AM_WRITE(MWA8_RAM)
	AM_RANGE(0xc000, 0xc000) AM_WRITE(YM3812_control_port_0_w)
	AM_RANGE(0xc001, 0xc001) AM_WRITE(YM3812_write_port_0_w)
//  AM_RANGE(0xa000, 0xa000) AM_WRITE(ADPCM_trigger)
	AM_RANGE(0xe000, 0xe006) AM_WRITE(MWA8_NOP)
	AM_RANGE(0xe00e, 0xe00f) AM_WRITE(MWA8_NOP)
ADDRESS_MAP_END

static ADDRESS_MAP_START( trckydoc_readmem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0xdfff) AM_READ(MRA8_ROM)
	AM_RANGE(0xe000, 0xe7ff) AM_READ(MRA8_RAM)
	AM_RANGE(0xf800, 0xf800) AM_READ(input_port_2_r)
	AM_RANGE(0xf808, 0xf808) AM_READ(input_port_3_r)
	AM_RANGE(0xf810, 0xf810) AM_READ(input_port_0_r)
	AM_RANGE(0xf818, 0xf818) AM_READ(input_port_1_r)
	AM_RANGE(0xf828, 0xf828) AM_READ(watchdog_reset_r)
ADDRESS_MAP_END

static ADDRESS_MAP_START( trckydoc_writemem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0xdfff) AM_WRITE(MWA8_ROM)
	AM_RANGE(0xe000, 0xe7ff) AM_WRITE(MWA8_RAM)
	AM_RANGE(0xe800, 0xebff) AM_WRITE(MWA8_RAM) AM_BASE(&spriteram) AM_SIZE(&spriteram_size)
	AM_RANGE(0xec00, 0xefff) AM_WRITE(trckydoc_spriteram_mirror_w) // it clears sprites from the screen by writing here to set some of the attributes
	AM_RANGE(0xf000, 0xf3ff) AM_WRITE(tecfri_videoram_w) AM_BASE(&tecfri_videoram)
	AM_RANGE(0xf400, 0xf7ff) AM_WRITE(tecfri_colorram_w) AM_BASE(&tecfri_colorram)
	AM_RANGE(0xf820, 0xf820) AM_WRITE(YM3812_control_port_0_w)
	AM_RANGE(0xf821, 0xf821) AM_WRITE(YM3812_write_port_0_w)
	AM_RANGE(0xf830, 0xf830) AM_WRITE(tecfri_scroll_bg_w)
	AM_RANGE(0xf838, 0xf838) AM_WRITE(MWA8_NOP)
	AM_RANGE(0xf839, 0xf839) AM_WRITE(flip_screen_w)
	AM_RANGE(0xf83a, 0xf83a) AM_WRITE(sauro_coin1_w)
	AM_RANGE(0xf83b, 0xf83b) AM_WRITE(sauro_coin2_w)
	AM_RANGE(0xf83c, 0xf83c) AM_WRITE(watchdog_reset_w)
	AM_RANGE(0xf83f, 0xf83f) AM_WRITE(MWA8_NOP)
ADDRESS_MAP_END

INPUT_PORTS_START( tecfri )
	PORT_START      /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_BUTTON1 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_BUTTON2 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_COIN2 )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT ) PORT_8WAY
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT ) PORT_8WAY
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP ) PORT_8WAY
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN ) PORT_8WAY

	PORT_START      /* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_BUTTON1 ) PORT_COCKTAIL
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_BUTTON2 ) PORT_COCKTAIL
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT ) PORT_8WAY PORT_COCKTAIL
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT ) PORT_8WAY PORT_COCKTAIL
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP ) PORT_8WAY PORT_COCKTAIL
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN ) PORT_8WAY PORT_COCKTAIL

	PORT_START
	PORT_SERVICE( 0x01, IP_ACTIVE_HIGH )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x02, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0x08, 0x00, DEF_STR( Free_Play ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x08, DEF_STR( On ) )
	PORT_DIPNAME( 0x30, 0x20, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x30, DEF_STR( Very_Easy) )
	PORT_DIPSETTING(    0x20, DEF_STR( Easy ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Hard ) )	                      /* This crashes test mode!!! */
	PORT_DIPSETTING(    0x00, DEF_STR( Very_Hard ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Allow_Continue ) )
	PORT_DIPSETTING(    0x00, DEF_STR( No ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x80, 0x00, "Freeze" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x80, DEF_STR( On ) )

	PORT_START
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 1C_1C ) )
	PORT_DIPNAME( 0x0c, 0x0c, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x0c, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_5C ) )
	PORT_DIPNAME( 0x30, 0x20, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x30, "2" )
	PORT_DIPSETTING(    0x20, "3" )
	PORT_DIPSETTING(    0x10, "4" )
	PORT_DIPSETTING(    0x00, "5" )
	PORT_DIPNAME( 0x40, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x40, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x80, DEF_STR( On ) )
INPUT_PORTS_END


static const gfx_layout charlayout =
{
	8,8,	/* 8*8 chars */
    2048,   /* 2048 characters */
    4,      /* 4 bits per pixel */
    { 0,1,2,3 },  /* The 4 planes are packed together */
    { 0*4, 1*4, 2*4, 3*4, 4*4, 5*4, 6*4, 7*4},
    { 0*4*8, 1*4*8, 2*4*8, 3*4*8, 4*4*8, 5*4*8, 6*4*8, 7*4*8},
    8*8*4     /* every char takes 32 consecutive bytes */
};

static const gfx_layout trckydoc_spritelayout =
{
	16,16,	/* 16*16 sprites */
    512,	/* 512 sprites */
    4,      /* 4 bits per pixel */
    { 0,1,2,3 },  /* The 4 planes are packed together */
    { 1*4, 0*4, 3*4, 2*4, 5*4, 4*4, 7*4, 6*4, 9*4, 8*4, 11*4, 10*4, 13*4, 12*4, 15*4, 14*4},
    { RGN_FRAC(3,4)+0*4*16, RGN_FRAC(2,4)+0*4*16, RGN_FRAC(1,4)+0*4*16, RGN_FRAC(0,4)+0*4*16,
      RGN_FRAC(3,4)+1*4*16, RGN_FRAC(2,4)+1*4*16, RGN_FRAC(1,4)+1*4*16, RGN_FRAC(0,4)+1*4*16,
      RGN_FRAC(3,4)+2*4*16, RGN_FRAC(2,4)+2*4*16, RGN_FRAC(1,4)+2*4*16, RGN_FRAC(0,4)+2*4*16,
      RGN_FRAC(3,4)+3*4*16, RGN_FRAC(2,4)+3*4*16, RGN_FRAC(1,4)+3*4*16, RGN_FRAC(0,4)+3*4*16 },
    16*16     /* every sprite takes 32 consecutive bytes */
};

static const gfx_layout sauro_spritelayout =
{
	16,16,	/* 16*16 sprites */
    1024,	/* 1024 sprites */
    4,      /* 4 bits per pixel */
    { 0,1,2,3 },  /* The 4 planes are packed together */
    { 1*4, 0*4, 3*4, 2*4, 5*4, 4*4, 7*4, 6*4, 9*4, 8*4, 11*4, 10*4, 13*4, 12*4, 15*4, 14*4},
    { RGN_FRAC(3,4)+0*4*16, RGN_FRAC(2,4)+0*4*16, RGN_FRAC(1,4)+0*4*16, RGN_FRAC(0,4)+0*4*16,
      RGN_FRAC(3,4)+1*4*16, RGN_FRAC(2,4)+1*4*16, RGN_FRAC(1,4)+1*4*16, RGN_FRAC(0,4)+1*4*16,
      RGN_FRAC(3,4)+2*4*16, RGN_FRAC(2,4)+2*4*16, RGN_FRAC(1,4)+2*4*16, RGN_FRAC(0,4)+2*4*16,
      RGN_FRAC(3,4)+3*4*16, RGN_FRAC(2,4)+3*4*16, RGN_FRAC(1,4)+3*4*16, RGN_FRAC(0,4)+3*4*16 },
    16*16     /* every sprite takes 32 consecutive bytes */
};

static const gfx_decode sauro_gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &charlayout, 0, 64 },
	{ REGION_GFX2, 0, &charlayout, 0, 64 },
	{ REGION_GFX3, 0, &sauro_spritelayout, 0, 64 },
	{ -1 } /* end of array */
};

static const gfx_decode trckydoc_gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &charlayout, 0, 64 },
	{ REGION_GFX2, 0, &trckydoc_spritelayout, 0, 64 },
	{ -1 } /* end of array */
};

static INTERRUPT_GEN( sauro_interrupt )
{
	cpunum_set_input_line(1, INPUT_LINE_NMI, PULSE_LINE);
	cpunum_set_input_line(1, 0, HOLD_LINE);
}

static MACHINE_DRIVER_START( tecfri )
	/* basic machine hardware */
	MDRV_CPU_ADD_TAG("main", Z80, 4000000)        // 4 MHz???
	MDRV_CPU_VBLANK_INT(irq0_line_hold, 1)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(5000)  // frames per second, vblank duration (otherwise sprites lag)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(32 * 8, 32 * 8)
	MDRV_VISIBLE_AREA(1 * 8, 31 * 8 - 1, 2 * 8, 30 * 8 - 1)
	MDRV_PALETTE_LENGTH(1024)
	MDRV_PALETTE_INIT(RRRR_GGGG_BBBB)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")

	MDRV_SOUND_ADD(YM3812, 3600000)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.0)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( trckydoc )
	MDRV_IMPORT_FROM(tecfri)

	MDRV_CPU_MODIFY("main")
	MDRV_CPU_PROGRAM_MAP(trckydoc_readmem, trckydoc_writemem )

	MDRV_GFXDECODE(trckydoc_gfxdecodeinfo)

	MDRV_VIDEO_START(trckydoc)
	MDRV_VIDEO_UPDATE(trckydoc)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( sauro )
	MDRV_IMPORT_FROM(tecfri)

	MDRV_CPU_MODIFY("main")
	MDRV_CPU_PROGRAM_MAP(sauro_readmem, sauro_writemem)
	MDRV_CPU_IO_MAP(sauro_readport, sauro_writeport)

	MDRV_CPU_ADD(Z80, 4000000)	// 4 MHz?
	/* audio CPU */
	MDRV_CPU_PROGRAM_MAP(sauro_sound_readmem, sauro_sound_writemem)
	MDRV_CPU_VBLANK_INT(sauro_interrupt, 8) // ?

	MDRV_GFXDECODE(sauro_gfxdecodeinfo)

	MDRV_VIDEO_START(sauro)
	MDRV_VIDEO_UPDATE(sauro)
MACHINE_DRIVER_END

/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START( sauro )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )          /* 64k for code */
	ROM_LOAD( "sauro-2.bin",     0x00000, 0x8000, CRC(19f8de25) SHA1(52eea7c0416ab0a8dbb3d1664b2f57ab7a405a67) )
	ROM_LOAD( "sauro-1.bin",     0x08000, 0x8000, CRC(0f8b876f) SHA1(6e61a8934a2cc3c80c1f47dd59aa43aaeec12f75) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )          /* 64k for sound CPU */
	ROM_LOAD( "sauro-3.bin",     0x00000, 0x8000, CRC(0d501e1b) SHA1(20a56ff30d4fa5d2f483a449703b49153839f6bc) )

	ROM_REGION( 0x10000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "sauro-6.bin",     0x00000, 0x8000, CRC(4b77cb0f) SHA1(7b9cb2dca561d81390106c1a5c0533dcecaf6f1a) )
	ROM_LOAD( "sauro-7.bin",     0x08000, 0x8000, CRC(187da060) SHA1(1df156e58379bb39acade02aabab6ff1cb7cc288) )

	ROM_REGION( 0x10000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "sauro-4.bin",     0x00000, 0x8000, CRC(9b617cda) SHA1(ce26b84ad5ecd6185ae218520e9972645bbf09ad) )
	ROM_LOAD( "sauro-5.bin",     0x08000, 0x8000, CRC(a6e2640d) SHA1(346ffcf62e27ce8134f4e5e0dbcf11f110e19e04) )

	ROM_REGION( 0x20000, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_LOAD( "sauro-8.bin",     0x00000, 0x8000, CRC(e08b5d5e) SHA1(eaaeaa08b19c034ab2a2140f887edffca5f441b9) )
	ROM_LOAD( "sauro-9.bin",     0x08000, 0x8000, CRC(7c707195) SHA1(0529f6808b0cec3e12ca51bee189841d21577786) )
	ROM_LOAD( "sauro-10.bin",    0x10000, 0x8000, CRC(c93380d1) SHA1(fc9655cc94c2d2058f83eb341be7e7856a08194f) )
	ROM_LOAD( "sauro-11.bin",    0x18000, 0x8000, CRC(f47982a8) SHA1(cbaeac272c015d9439f151cfb3449082f11a57a1) )

	ROM_REGION( 0x0c00, REGION_PROMS, 0 )
	ROM_LOAD( "82s137-3.bin",    0x0000, 0x0400, CRC(d52c4cd0) SHA1(27d6126b46616c06b55d8018c97f6c3d7805ae9e) )  /* Red component */
	ROM_LOAD( "82s137-2.bin",    0x0400, 0x0400, CRC(c3e96d5d) SHA1(3f6f21526a4357e4a9a9d56a6f4ef5911af2d120) )  /* Green component */
	ROM_LOAD( "82s137-1.bin",    0x0800, 0x0400, CRC(bdfcf00c) SHA1(9faf4d7f8959b64faa535c9945eec59c774a3760) )  /* Blue component */
ROM_END

ROM_START( trckydoc )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )          /* 64k for code */
	ROM_LOAD( "trckydoc.d9",  0x0000,  0x8000, CRC(c6242fc3) SHA1(c8a6f6abe8b51061a113ed75fead0479df68ec40) )
	ROM_LOAD( "trckydoc.b9",  0x8000,  0x8000, CRC(8645c840) SHA1(79c2acfc1aeafbe94afd9d230200bd7cdd7bcd1b) )

	ROM_REGION( 0x10000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "trckydoc.e6",     0x00000, 0x8000, CRC(ec326392) SHA1(e6954fecc501a821caa21e67597914519fbbe58f) )
	ROM_LOAD( "trckydoc.g6",     0x08000, 0x8000, CRC(6a65c088) SHA1(4a70c104809d86b4eef6cc0df9452966fe7c9859) )

	ROM_REGION( 0x10000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "trckydoc.h1",    0x00000, 0x4000, CRC(8b73cbf3) SHA1(d10f79a38c1596c90bac9cf4c64ba38ae6ecd8cb) )
	ROM_LOAD( "trckydoc.e1",    0x04000, 0x4000, CRC(841be98e) SHA1(82da07490b73edcbffc3b9247205aab3a1f7d7ad) )
	ROM_LOAD( "trckydoc.c1",    0x08000, 0x4000, CRC(1d25574b) SHA1(924e4376a7fe6cdfff0fa6045aaa3f7c0633d275) )
	ROM_LOAD( "trckydoc.a1",    0x0c000, 0x4000, CRC(436c59ba) SHA1(2aa9c155c432a3c81420520c53bb944dcc613a94) )

	ROM_REGION( 0x0c00, REGION_PROMS, 0 ) // colour proms
	ROM_LOAD( "tdclr3.prm",    0x0000, 0x0100, CRC(671d0140) SHA1(7d5fcd9589c46590b0a240cac428f993201bec2a) )
	ROM_LOAD( "tdclr2.prm",    0x0400, 0x0100, CRC(874f9050) SHA1(db40d68f5166657fce0eadcd82143112b0388894) )
	ROM_LOAD( "tdclr1.prm",    0x0800, 0x0100, CRC(57f127b0) SHA1(3d2b18a7a31933579f06d92fa0cc3f0e1fe8b98a) )

	ROM_REGION( 0x0200, REGION_USER1, 0 ) // unknown
	ROM_LOAD( "tdprm.prm",    0x0000, 0x0200,  CRC(5261bc11) SHA1(1cc7a9a7376e65f4587b75ef9382049458656372) )
ROM_END

ROM_START( trckydca )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )          /* 64k for code */
	ROM_LOAD( "trckydca.d9",  0x0000,  0x8000, CRC(99c38aa4) SHA1(298a19439cc17743e10d101c50a26b9a7348299e) )
	ROM_LOAD( "trckydca.b9",  0x8000,  0x8000, CRC(b6048a15) SHA1(d982fafbfa391ef9bab50bfd52607494e2a9eedf) )

	ROM_REGION( 0x10000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "trckydoc.e6",     0x00000, 0x8000, CRC(ec326392) SHA1(e6954fecc501a821caa21e67597914519fbbe58f) )
	ROM_LOAD( "trckydoc.g6",     0x08000, 0x8000, CRC(6a65c088) SHA1(4a70c104809d86b4eef6cc0df9452966fe7c9859) )

	ROM_REGION( 0x10000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "trckydoc.h1",    0x00000, 0x4000, CRC(8b73cbf3) SHA1(d10f79a38c1596c90bac9cf4c64ba38ae6ecd8cb) )
	ROM_LOAD( "trckydoc.e1",    0x04000, 0x4000, CRC(841be98e) SHA1(82da07490b73edcbffc3b9247205aab3a1f7d7ad) )
	ROM_LOAD( "trckydoc.c1",    0x08000, 0x4000, CRC(1d25574b) SHA1(924e4376a7fe6cdfff0fa6045aaa3f7c0633d275) )
	ROM_LOAD( "trckydoc.a1",    0x0c000, 0x4000, CRC(436c59ba) SHA1(2aa9c155c432a3c81420520c53bb944dcc613a94) )

	ROM_REGION( 0x0c00, REGION_PROMS, 0 ) // colour proms
	ROM_LOAD( "tdclr3.prm",    0x0000, 0x0100, CRC(671d0140) SHA1(7d5fcd9589c46590b0a240cac428f993201bec2a) )
	ROM_LOAD( "tdclr2.prm",    0x0400, 0x0100, CRC(874f9050) SHA1(db40d68f5166657fce0eadcd82143112b0388894) )
	ROM_LOAD( "tdclr1.prm",    0x0800, 0x0100, CRC(57f127b0) SHA1(3d2b18a7a31933579f06d92fa0cc3f0e1fe8b98a) )

	ROM_REGION( 0x0200, REGION_USER1, 0 ) // unknown
	ROM_LOAD( "tdprm.prm",    0x0000, 0x0200,  CRC(5261bc11) SHA1(1cc7a9a7376e65f4587b75ef9382049458656372) )
ROM_END

static DRIVER_INIT( tecfri )
{
	/* This game doesn't like all memory to be initialized to zero, it won't
       initialize the high scores */

	UINT8 *RAM = memory_region(REGION_CPU1);

	memset(&RAM[0xe000], 0, 0x100);
	RAM[0xe000] = 1;
}

GAME( 1987, sauro,    0,        sauro,    tecfri, tecfri, ROT0, "Tecfri", "Sauro", GAME_IMPERFECT_SOUND )
GAME( 1987, trckydoc, 0,        trckydoc, tecfri, tecfri, ROT0, "Tecfri", "Tricky Doc (Set 1)", 0 )
GAME( 1987, trckydca, trckydoc, trckydoc, tecfri, tecfri, ROT0, "Tecfri", "Tricky Doc (Set 2)", 0 )
