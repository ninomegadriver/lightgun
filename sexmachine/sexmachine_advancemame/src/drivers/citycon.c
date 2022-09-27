/***************************************************************************


***************************************************************************/

#include "driver.h"
#include "sound/ay8910.h"
#include "sound/2203intf.h"


extern UINT8 *citycon_videoram;
extern UINT8 *citycon_scroll;
extern UINT8 *citycon_linecolor;
WRITE8_HANDLER( citycon_videoram_w );
WRITE8_HANDLER( citycon_linecolor_w );
WRITE8_HANDLER( citycon_background_w );

VIDEO_UPDATE( citycon );
VIDEO_START( citycon );


READ8_HANDLER( citycon_in_r )
{
	return readinputport(flip_screen ? 1 : 0);
}



static ADDRESS_MAP_START( readmem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x1fff) AM_READ(MRA8_RAM)
	AM_RANGE(0x3000, 0x3000) AM_READ(citycon_in_r)	/* player 1 & 2 inputs multiplexed */
	AM_RANGE(0x3001, 0x3001) AM_READ(input_port_2_r)
	AM_RANGE(0x3002, 0x3002) AM_READ(input_port_3_r)
	AM_RANGE(0x3007, 0x3007) AM_READ(watchdog_reset_r)	/* ? */
	AM_RANGE(0x4000, 0xffff) AM_READ(MRA8_ROM)
ADDRESS_MAP_END

static ADDRESS_MAP_START( writemem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x0fff) AM_WRITE(MWA8_RAM)
	AM_RANGE(0x1000, 0x1fff) AM_WRITE(citycon_videoram_w) AM_BASE(&citycon_videoram)
	AM_RANGE(0x2000, 0x20ff) AM_WRITE(citycon_linecolor_w) AM_BASE(&citycon_linecolor)
	AM_RANGE(0x2800, 0x28ff) AM_WRITE(MWA8_RAM) AM_BASE(&spriteram) AM_SIZE(&spriteram_size)
	AM_RANGE(0x3000, 0x3000) AM_WRITE(citycon_background_w)
	AM_RANGE(0x3001, 0x3001) AM_WRITE(soundlatch_w)
	AM_RANGE(0x3002, 0x3002) AM_WRITE(soundlatch2_w)
	AM_RANGE(0x3004, 0x3005) AM_WRITE(MWA8_RAM) AM_BASE(&citycon_scroll)
	AM_RANGE(0x3800, 0x3cff) AM_WRITE(paletteram_RRRRGGGGBBBBxxxx_be_w) AM_BASE(&paletteram)
	AM_RANGE(0x4000, 0xffff) AM_WRITE(MWA8_ROM)
ADDRESS_MAP_END

static ADDRESS_MAP_START( readmem_sound, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x0fff) AM_READ(MRA8_RAM)
//  AM_RANGE(0x4002, 0x4002) AM_READ(AY8910_read_port_0_r)  /* ?? */
	AM_RANGE(0x6001, 0x6001) AM_READ(YM2203_read_port_0_r)
	AM_RANGE(0x8000, 0xffff) AM_READ(MRA8_ROM)
ADDRESS_MAP_END

static ADDRESS_MAP_START( writemem_sound, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x0fff) AM_WRITE(MWA8_RAM)
	AM_RANGE(0x4000, 0x4000) AM_WRITE(AY8910_control_port_0_w)
	AM_RANGE(0x4001, 0x4001) AM_WRITE(AY8910_write_port_0_w)
	AM_RANGE(0x6000, 0x6000) AM_WRITE(YM2203_control_port_0_w)
	AM_RANGE(0x6001, 0x6001) AM_WRITE(YM2203_write_port_0_w)
	AM_RANGE(0x8000, 0xffff) AM_WRITE(MWA8_ROM)
ADDRESS_MAP_END



INPUT_PORTS_START( citycon )
	PORT_START_TAG("IN0")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START2 )

	PORT_START_TAG("IN1")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY PORT_COCKTAIL
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY PORT_COCKTAIL
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY PORT_COCKTAIL
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY PORT_COCKTAIL
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_COCKTAIL
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_COCKTAIL
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START_TAG("IN2")
	PORT_DIPNAME( 0x03, 0x00, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x00, "3" )
	PORT_DIPSETTING(    0x01, "4" )
	PORT_DIPSETTING(    0x02, "5" )
	PORT_DIPSETTING(    0x03, "Infinite (Cheat)")
	PORT_DIPNAME( 0x04, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x04, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x08, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x10, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Cocktail ) )
	/* the coin input must stay low for exactly 2 frames to be consistently recognized. */
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN1 ) PORT_IMPULSE(2)

	PORT_START_TAG("IN3")
	PORT_DIPNAME( 0x07, 0x00, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0x07, DEF_STR( 5C_1C ) )
	PORT_DIPSETTING(    0x06, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x05, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 1C_4C ) )
	PORT_DIPNAME( 0x08, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x08, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x10, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x20, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x40, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
INPUT_PORTS_END



static const gfx_layout charlayout =
{
	8,8,
	RGN_FRAC(1,2),
	5,
	{ 16, 12, 8, 4, 0 },
	{ 0, 1, 2, 3, RGN_FRAC(1,2)+0, RGN_FRAC(1,2)+1, RGN_FRAC(1,2)+2, RGN_FRAC(1,2)+3 },
	{ 0*24, 1*24, 2*24, 3*24, 4*24, 5*24, 6*24, 7*24 },
	24*8
};

static const gfx_layout tilelayout =
{
	8,8,	/* 8*8 characters */
	256,	/* 256 characters */
	4,	/* 4 bits per pixel */
	{ 4, 0, 0xc000*8+4, 0xc000*8+0 },
	{ 0, 1, 2, 3, 256*8*8+0, 256*8*8+1, 256*8*8+2, 256*8*8+3 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8	/* every char takes 8 consecutive bytes */
};

static const gfx_layout spritelayout =
{
	8,16,	/* 8*16 sprites */
	128,	/* 128 sprites */
	4,	/* 4 bits per pixel */
	{ 4, 0, 0x2000*8+4, 0x2000*8+0 },
	{ 0, 1, 2, 3, 128*16*8+0, 128*16*8+1, 128*16*8+2, 128*16*8+3 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8,
            8*8, 9*8, 10*8, 11*8, 12*8, 13*8, 14*8, 15*8 },
	16*8	/* every sprite takes 16 consecutive bytes */
};


static const gfx_decode gfxdecodeinfo[] =
{
//  { REGION_GFX1, 0x00000, &charlayout, 512, 32 }, /* colors 512-639 */
	{ REGION_GFX1, 0x00000, &charlayout, 640, 32 },	/* colors 512-639 */
	{ REGION_GFX2, 0x00000, &spritelayout, 0, 16 },	/* colors 0-255 */
	{ REGION_GFX2, 0x01000, &spritelayout, 0, 16 },
	{ REGION_GFX3, 0x00000, &tilelayout, 256, 16 },	/* colors 256-511 */
	{ REGION_GFX3, 0x01000, &tilelayout, 256, 16 },
	{ REGION_GFX3, 0x02000, &tilelayout, 256, 16 },
	{ REGION_GFX3, 0x03000, &tilelayout, 256, 16 },
	{ REGION_GFX3, 0x04000, &tilelayout, 256, 16 },
	{ REGION_GFX3, 0x05000, &tilelayout, 256, 16 },
	{ REGION_GFX3, 0x06000, &tilelayout, 256, 16 },
	{ REGION_GFX3, 0x07000, &tilelayout, 256, 16 },
	{ REGION_GFX3, 0x08000, &tilelayout, 256, 16 },
	{ REGION_GFX3, 0x09000, &tilelayout, 256, 16 },
	{ REGION_GFX3, 0x0a000, &tilelayout, 256, 16 },
	{ REGION_GFX3, 0x0b000, &tilelayout, 256, 16 },
	{ -1 } /* end of array */
};



static struct YM2203interface ym2203_interface =
{
	soundlatch_r,
	soundlatch2_r,
};



static MACHINE_DRIVER_START( citycon )

	/* basic machine hardware */
	MDRV_CPU_ADD(M6809, 2048000)        /* 2.048 MHz ??? */
	MDRV_CPU_PROGRAM_MAP(readmem,writemem)
	MDRV_CPU_VBLANK_INT(irq0_line_hold,1)

	MDRV_CPU_ADD(M6809, 640000)
	/* audio CPU */        /* 0.640 MHz ??? */
	MDRV_CPU_PROGRAM_MAP(readmem_sound,writemem_sound)
	MDRV_CPU_VBLANK_INT(irq0_line_hold,1)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(32*8, 32*8)
	MDRV_VISIBLE_AREA(1*8, 31*8-1, 2*8, 30*8-1)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(640+1024)	/* 640 real palette + 1024 virtual palette */

	MDRV_VIDEO_START(citycon)
	MDRV_VIDEO_UPDATE(citycon)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")

	MDRV_SOUND_ADD(AY8910, 1250000)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.40)

	MDRV_SOUND_ADD(YM2203, 1250000)
	MDRV_SOUND_CONFIG(ym2203_interface)
	MDRV_SOUND_ROUTE(0, "mono", 0.40)
	MDRV_SOUND_ROUTE(1, "mono", 0.40)
	MDRV_SOUND_ROUTE(2, "mono", 0.40)
	MDRV_SOUND_ROUTE(3, "mono", 0.20)
MACHINE_DRIVER_END



/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START( citycon )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )     /* 64k for code */
	ROM_LOAD( "c10",          0x4000, 0x4000, CRC(ae88b53c) SHA1(dd12310bd9c9b93462446e8e0a1c853506bf3aa1) )
	ROM_LOAD( "c11",          0x8000, 0x8000, CRC(139eb1aa) SHA1(c570e8ca1499f7ea61938e78c32c1cc3050ca2b7) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* 64k for the audio CPU */
	ROM_LOAD( "c1",           0x8000, 0x8000, CRC(1fad7589) SHA1(2e626bbbab8cffe11ee7de3e56aa1871c29d5fa9) )

	ROM_REGION( 0x03000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "c4",           0x00000, 0x2000, CRC(a6b32fc6) SHA1(d99d5a527440e9a91525c1084b95b213e3b760ec) )	/* Characters */

	ROM_REGION( 0x04000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "c12",          0x00000, 0x2000, CRC(08eaaccd) SHA1(a970381e3ba22bcdea6df2d31cd8a10c4b2bc413) )	/* Sprites    */
	ROM_LOAD( "c13",          0x02000, 0x2000, CRC(1819aafb) SHA1(8a5ffcd8866e09c5568879257384767d61796111) )

	ROM_REGION( 0x18000, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_LOAD( "c9",           0x00000, 0x8000, CRC(8aeb47e6) SHA1(bb09dbe6b37e1bd02abf3024ac4d954c8f0e70f2) )	/* Background tiles */
	ROM_LOAD( "c8",           0x08000, 0x4000, CRC(0d7a1eeb) SHA1(60b8d4124ce857a248d3c41fdb050f11be58549f) )
	ROM_LOAD( "c6",           0x0c000, 0x8000, CRC(2246fe9d) SHA1(f7f8708d499bcbd1a583e1092b54425ad1105f94) )
	ROM_LOAD( "c7",           0x14000, 0x4000, CRC(e8b97de9) SHA1(f4d1b7075f47ab4522c36281b97eaa02fe383814) )

	ROM_REGION( 0xe000, REGION_GFX4, 0 )	/* background tilemaps */
	ROM_LOAD( "c2",           0x0000, 0x8000, CRC(f2da4f23) SHA1(5ea1a51c3ac283796f7eafb6719d88356767340d) )	/* background maps */
	ROM_LOAD( "c3",           0x8000, 0x4000, CRC(7ef3ac1b) SHA1(8a0497c4e4733f9c50d576f632210b82497a5e1c) )
	ROM_LOAD( "c5",           0xc000, 0x2000, CRC(c03d8b1b) SHA1(641c1eba334d36ea64b9293a20320b31c7c88858) )	/* color codes for the background */
ROM_END

ROM_START( citycona )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )     /* 64k for code */
	ROM_LOAD( "c10",          0x4000, 0x4000, CRC(ae88b53c) SHA1(dd12310bd9c9b93462446e8e0a1c853506bf3aa1) )
	ROM_LOAD( "c11b",         0x8000, 0x8000, CRC(d64af468) SHA1(5bb3541af3ce632e8eca313231205713d72fb9dc) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* 64k for the audio CPU */
	ROM_LOAD( "c1",           0x8000, 0x8000, CRC(1fad7589) SHA1(2e626bbbab8cffe11ee7de3e56aa1871c29d5fa9) )

	ROM_REGION( 0x03000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "c4",           0x00000, 0x2000, CRC(a6b32fc6) SHA1(d99d5a527440e9a91525c1084b95b213e3b760ec) )	/* Characters */

	ROM_REGION( 0x04000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "c12",          0x00000, 0x2000, CRC(08eaaccd) SHA1(a970381e3ba22bcdea6df2d31cd8a10c4b2bc413) )	/* Sprites    */
	ROM_LOAD( "c13",          0x02000, 0x2000, CRC(1819aafb) SHA1(8a5ffcd8866e09c5568879257384767d61796111) )

	ROM_REGION( 0x18000, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_LOAD( "c9",           0x00000, 0x8000, CRC(8aeb47e6) SHA1(bb09dbe6b37e1bd02abf3024ac4d954c8f0e70f2) )	/* Background tiles */
	ROM_LOAD( "c8",           0x08000, 0x4000, CRC(0d7a1eeb) SHA1(60b8d4124ce857a248d3c41fdb050f11be58549f) )
	ROM_LOAD( "c6",           0x0c000, 0x8000, CRC(2246fe9d) SHA1(f7f8708d499bcbd1a583e1092b54425ad1105f94) )
	ROM_LOAD( "c7",           0x14000, 0x4000, CRC(e8b97de9) SHA1(f4d1b7075f47ab4522c36281b97eaa02fe383814) )

	ROM_REGION( 0xe000, REGION_GFX4, 0 )	/* background tilemaps */
	ROM_LOAD( "c2",           0x0000, 0x8000, CRC(f2da4f23) SHA1(5ea1a51c3ac283796f7eafb6719d88356767340d) )	/* background maps */
	ROM_LOAD( "c3",           0x8000, 0x4000, CRC(7ef3ac1b) SHA1(8a0497c4e4733f9c50d576f632210b82497a5e1c) )
	ROM_LOAD( "c5",           0xc000, 0x2000, CRC(c03d8b1b) SHA1(641c1eba334d36ea64b9293a20320b31c7c88858) )	/* color codes for the background */
ROM_END

ROM_START( cruisin )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )     /* 64k for code */
	ROM_LOAD( "cr10",         0x4000, 0x4000, CRC(cc7c52f3) SHA1(69d76f146fb1dac62c6def3a4269012b3880f03b) )
	ROM_LOAD( "cr11",         0x8000, 0x8000, CRC(5422f276) SHA1(d384fc4f853fe79b73e939a8fc7b7af780659c5e) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* 64k for the audio CPU */
	ROM_LOAD( "c1",           0x8000, 0x8000, CRC(1fad7589) SHA1(2e626bbbab8cffe11ee7de3e56aa1871c29d5fa9) )

	ROM_REGION( 0x03000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "cr4",          0x00000, 0x2000, CRC(8cd0308e) SHA1(7303b9e074bda557d64b39e04cef0f965a756be6) )	/* Characters */

	ROM_REGION( 0x04000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "c12",          0x00000, 0x2000, CRC(08eaaccd) SHA1(a970381e3ba22bcdea6df2d31cd8a10c4b2bc413) )	/* Sprites    */
	ROM_LOAD( "c13",          0x02000, 0x2000, CRC(1819aafb) SHA1(8a5ffcd8866e09c5568879257384767d61796111) )

	ROM_REGION( 0x18000, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_LOAD( "c9",           0x00000, 0x8000, CRC(8aeb47e6) SHA1(bb09dbe6b37e1bd02abf3024ac4d954c8f0e70f2) )	/* Background tiles */
	ROM_LOAD( "c8",           0x08000, 0x4000, CRC(0d7a1eeb) SHA1(60b8d4124ce857a248d3c41fdb050f11be58549f) )
	ROM_LOAD( "c6",           0x0c000, 0x8000, CRC(2246fe9d) SHA1(f7f8708d499bcbd1a583e1092b54425ad1105f94) )
	ROM_LOAD( "c7",           0x14000, 0x4000, CRC(e8b97de9) SHA1(f4d1b7075f47ab4522c36281b97eaa02fe383814) )

	ROM_REGION( 0xe000, REGION_GFX4, 0 )	/* background tilemaps */
	ROM_LOAD( "c2",           0x0000, 0x8000, CRC(f2da4f23) SHA1(5ea1a51c3ac283796f7eafb6719d88356767340d) )	/* background maps */
	ROM_LOAD( "c3",           0x8000, 0x4000, CRC(7ef3ac1b) SHA1(8a0497c4e4733f9c50d576f632210b82497a5e1c) )
	ROM_LOAD( "c5",           0xc000, 0x2000, CRC(c03d8b1b) SHA1(641c1eba334d36ea64b9293a20320b31c7c88858) )	/* color codes for the background */
ROM_END



static DRIVER_INIT( citycon )
{
	UINT8 *rom = memory_region(REGION_GFX1);
	int i;


	/*
      City Connection controls the text color code for each _scanline_, not
      for each character as happens in most games. To handle that conveniently,
      I convert the 2bpp char data into 5bpp, and create a virtual palette so
      characters can still be drawn in one pass.
      */
	for (i = 0x0fff;i >= 0;i--)
	{
		int mask;

		rom[3*i] = rom[i];
		rom[3*i+1] = 0;
		rom[3*i+2] = 0;
		mask = rom[i] | (rom[i] << 4) | (rom[i] >> 4);
		if (i & 0x01) rom[3*i+1] |= mask & 0xf0;
		if (i & 0x02) rom[3*i+1] |= mask & 0x0f;
		if (i & 0x04) rom[3*i+2] |= mask & 0xf0;
	}
}



GAME( 1985, citycon,  0,       citycon, citycon, citycon, ROT0, "Jaleco", "City Connection (set 1)", 0 )
GAME( 1985, citycona, citycon, citycon, citycon, citycon, ROT0, "Jaleco", "City Connection (set 2)", 0 )
GAME( 1985, cruisin,  citycon, citycon, citycon, citycon, ROT0, "Jaleco (Kitkorp license)", "Cruisin", 0 )
