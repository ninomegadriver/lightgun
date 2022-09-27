/*****************************************************************************

Markham (c) 1983 Sun Electronics

    Driver by Uki

    17/Jun/2001 -

*****************************************************************************/

#include "driver.h"
#include "sound/sn76496.h"

extern WRITE8_HANDLER( markham_videoram_w );
extern WRITE8_HANDLER( markham_scroll_x_w );
extern WRITE8_HANDLER( markham_flipscreen_w );

extern PALETTE_INIT( markham );
extern VIDEO_START( markham );
extern VIDEO_UPDATE( markham );

static UINT8 *markham_sharedram;

/****************************************************************************/


WRITE8_HANDLER( markham_sharedram_w )
{
	markham_sharedram[offset] = data;
}

READ8_HANDLER( markham_sharedram_r )
{
	return markham_sharedram[offset];
}

READ8_HANDLER( markham_e004_r )
{
	return 0;
}

/****************************************************************************/

static ADDRESS_MAP_START( readmem1, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x5fff) AM_READ(MRA8_ROM)

	AM_RANGE(0xc000, 0xc7ff) AM_READ(MRA8_RAM)
	AM_RANGE(0xc800, 0xcfff) AM_READ(spriteram_r)
	AM_RANGE(0xd000, 0xd7ff) AM_READ(MRA8_RAM)
	AM_RANGE(0xd800, 0xdfff) AM_READ(markham_sharedram_r)

	AM_RANGE(0xe000, 0xe000) AM_READ(input_port_1_r) /* dsw 1 */
	AM_RANGE(0xe001, 0xe001) AM_READ(input_port_0_r) /* dsw 2 */
	AM_RANGE(0xe002, 0xe002) AM_READ(input_port_2_r) /* player1 */
	AM_RANGE(0xe003, 0xe003) AM_READ(input_port_3_r) /* player2 */

	AM_RANGE(0xe004, 0xe004) AM_READ(markham_e004_r) /* from CPU2 busack */

	AM_RANGE(0xe005, 0xe005) AM_READ(input_port_4_r) /* other inputs */

ADDRESS_MAP_END

static ADDRESS_MAP_START( writemem1, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x5fff) AM_WRITE(MWA8_ROM)

	AM_RANGE(0xc000, 0xc7ff) AM_WRITE(MWA8_RAM)
	AM_RANGE(0xc800, 0xcfff) AM_WRITE(MWA8_RAM) AM_BASE(&spriteram) AM_SIZE(&spriteram_size)
	AM_RANGE(0xd000, 0xd7ff) AM_WRITE(markham_videoram_w) AM_BASE(&videoram)
	AM_RANGE(0xd800, 0xdfff) AM_WRITE(markham_sharedram_w)

	AM_RANGE(0xe008, 0xe008) AM_WRITE(MWA8_NOP) /* coin counter? */

	AM_RANGE(0xe009, 0xe009) AM_WRITE(MWA8_NOP) /* to CPU2 busreq */

	AM_RANGE(0xe00c, 0xe00d) AM_WRITE(markham_scroll_x_w)
	AM_RANGE(0xe00e, 0xe00e) AM_WRITE(markham_flipscreen_w)
ADDRESS_MAP_END

static ADDRESS_MAP_START( readmem2, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x5fff) AM_READ(MRA8_ROM)
	AM_RANGE(0x8000, 0x87ff) AM_READ(MRA8_RAM)
ADDRESS_MAP_END

static ADDRESS_MAP_START( writemem2, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x5fff) AM_WRITE(MWA8_ROM)
	AM_RANGE(0x8000, 0x87ff) AM_WRITE(MWA8_RAM) AM_BASE(&markham_sharedram)

	AM_RANGE(0xc000, 0xc000) AM_WRITE(SN76496_0_w)
	AM_RANGE(0xc001, 0xc001) AM_WRITE(SN76496_1_w)

	AM_RANGE(0xc002, 0xc002) AM_WRITE(MWA8_NOP) /* unknown */
	AM_RANGE(0xc003, 0xc003) AM_WRITE(MWA8_NOP) /* unknown */
ADDRESS_MAP_END


/****************************************************************************/

INPUT_PORTS_START( markham )
	PORT_START  /* dsw1 */
	PORT_DIPNAME( 0x01, 0x00, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x00, "3" )
	PORT_DIPSETTING(    0x01, "5" )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0x04, 0x00, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x04, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x00, "Unknown 1-4" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x08, DEF_STR( On ) )
	PORT_DIPNAME( 0xf0, 0x00, "Coin1 / Coin2" )
	PORT_DIPSETTING(    0x00, "1C 1C / 1C 1C" )
	PORT_DIPSETTING(    0x10, "2C 1C / 2C 1C" )
	PORT_DIPSETTING(    0x20, "2C 1C / 1C 3C" )
	PORT_DIPSETTING(    0x30, "1C 1C / 1C 2C" )
	PORT_DIPSETTING(    0x40, "1C 1C / 1C 3C" )
	PORT_DIPSETTING(    0x50, "1C 1C / 1C 4C" )
	PORT_DIPSETTING(    0x60, "1C 1C / 1C 5C" )
	PORT_DIPSETTING(    0x70, "1C 1C / 1C 6C" )
	PORT_DIPSETTING(    0x80, "1C 2C / 1C 2C" )
	PORT_DIPSETTING(    0x90, "1C 2C / 1C 4C" )
	PORT_DIPSETTING(    0xa0, "1C 2C / 1C 5C" )
	PORT_DIPSETTING(    0xb0, "1C 2C / 1C 10C" )
	PORT_DIPSETTING(    0xc0, "1C 2C / 1C 11C" )
	PORT_DIPSETTING(    0xd0, "1C 2C / 1C 12C" )
	PORT_DIPSETTING(    0xe0, "1C 2C / 1C 6C" )
	PORT_DIPSETTING(    0xf0, DEF_STR( Free_Play ) )

	PORT_START  /* dsw2 */
	PORT_DIPNAME( 0x03, 0x02, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x00, DEF_STR( None ) )
	PORT_DIPSETTING(    0x01, "20000" )
	PORT_DIPSETTING(    0x02, "20000, Every 50000" )
	PORT_DIPSETTING(    0x03, "20000, Every 80000" )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x04, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x00, "Unknown 2-4" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x08, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x00, "Unknown 2-5" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x10, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x00, "Unknown 2-6" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x20, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x00, "Unknown 2-7" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x40, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x00, "Freeze" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x80, DEF_STR( On ) )

	PORT_START /* e002 */
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_BUTTON2 )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN ) PORT_8WAY
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP ) PORT_8WAY
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT ) PORT_8WAY
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT ) PORT_8WAY

	PORT_START /* e003 */
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_BUTTON2 ) PORT_COCKTAIL
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 ) PORT_COCKTAIL
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN ) PORT_8WAY PORT_COCKTAIL
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP ) PORT_8WAY PORT_COCKTAIL
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT ) PORT_8WAY PORT_COCKTAIL
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT ) PORT_8WAY PORT_COCKTAIL

	PORT_START /* e005 */
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_SERVICE1 )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_SERVICE( 0x10, IP_ACTIVE_HIGH )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_COIN2 )
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_COIN1 )
INPUT_PORTS_END


/****************************************************************************/

static const gfx_layout charlayout =
{
	8,8,    /* 8*8 characters */
	1024,   /* 1024 characters */
	3,      /* 3 bits per pixel */
	{0,8192*8,8192*8*2},
	{7,6,5,4,3,2,1,0},
	{8*0, 8*1, 8*2, 8*3, 8*4, 8*5, 8*6, 8*7},
	8*8
};

static const gfx_layout spritelayout =
{
	16,16,  /* 16*16 characters */
	256,    /* 256 characters */
	3,      /* 3 bits per pixel */
	{8192*8*2,8192*8,0},
	{7,6,5,4,3,2,1,0,
		8*16+7,8*16+6,8*16+5,8*16+4,8*16+3,8*16+2,8*16+1,8*16+0},
	{8*0, 8*1, 8*2, 8*3, 8*4, 8*5, 8*6, 8*7,
		8*8,8*9,8*10,8*11,8*12,8*13,8*14,8*15},
	8*8*4
};

static const gfx_decode gfxdecodeinfo[] =
{
	{ REGION_GFX2, 0x0000, &charlayout,   512, 64 },
	{ REGION_GFX1, 0x0000, &spritelayout, 0,   64 },
	{ -1 } /* end of array */
};


static MACHINE_DRIVER_START( markham )

	/* basic machine hardware */
	MDRV_CPU_ADD(Z80,8000000/2) /* 4.000MHz */
	MDRV_CPU_PROGRAM_MAP(readmem1,writemem1)
	MDRV_CPU_VBLANK_INT(irq0_line_hold,1)

	MDRV_CPU_ADD(Z80,8000000/2) /* 4.000MHz */
	MDRV_CPU_PROGRAM_MAP(readmem2,writemem2)
	MDRV_CPU_VBLANK_INT(irq0_line_hold,1)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_REAL_60HZ_VBLANK_DURATION)
	MDRV_INTERLEAVE(100)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(32*8, 32*8)
	MDRV_VISIBLE_AREA(1*8, 31*8-1, 2*8, 30*8-1)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(256)
	MDRV_COLORTABLE_LENGTH(1024)

	MDRV_PALETTE_INIT(markham)
	MDRV_VIDEO_START(markham)
	MDRV_VIDEO_UPDATE(markham)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")

	MDRV_SOUND_ADD(SN76496, 8000000/2)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.75)

	MDRV_SOUND_ADD(SN76496, 8000000/2)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.75)
MACHINE_DRIVER_END

/****************************************************************************/

ROM_START( markham )
	ROM_REGION( 0x10000, REGION_CPU1, 0 ) /* main CPU */
	ROM_LOAD( "tv3.9",   0x0000,  0x2000, CRC(59391637) SHA1(e0cfe49a5591d6a6e64c3277319a19235b0ee6ea) )
	ROM_LOAD( "tvg4.10", 0x2000,  0x2000, CRC(1837bcce) SHA1(50e1ae0a4937f09a3dced48bb12f57cee846487a) )
	ROM_LOAD( "tvg5.11", 0x4000,  0x2000, CRC(651da602) SHA1(9f33d6ea0526af9be8ac9210910ea768da825ee5) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 ) /* sub CPU */
	ROM_LOAD( "tvg1.5",  0x0000,  0x2000, CRC(c5299766) SHA1(a6c903088ffd6c5ae0ba7ff50c8509a185f88220) )
	ROM_LOAD( "tvg2.6",  0x4000,  0x2000, CRC(b216300a) SHA1(036fafd0277b3422cf491db77748358da1ecfb43) )

	ROM_REGION( 0x6000, REGION_GFX1, ROMREGION_DISPOSE ) /* sprite */
	ROM_LOAD( "tvg6.84", 0x0000,  0x2000, CRC(ab933ae5) SHA1(d2bdbc35d751480ddf8b89b90063510684b00db2) )
	ROM_LOAD( "tvg7.85", 0x2000,  0x2000, CRC(ce8edda7) SHA1(5312754aec20791398de57f08857d4097a7cfc2c) )
	ROM_LOAD( "tvg8.86", 0x4000,  0x2000, CRC(74d1536a) SHA1(ff2efbbe1420282643558a65bfa5fd278cdaf135) )

	ROM_REGION( 0x6000, REGION_GFX2, ROMREGION_DISPOSE ) /* bg */
	ROM_LOAD( "tvg9.87",  0x0000,  0x2000, CRC(42168675) SHA1(d2cce79a05ca7fda9347630fe0045a2d8182025d) )
	ROM_LOAD( "tvg10.88", 0x2000,  0x2000, CRC(fa9feb67) SHA1(669c6e1defc33541c36d4deb9667b67254f53a37) )
	ROM_LOAD( "tvg11.89", 0x4000,  0x2000, CRC(71f3dd49) SHA1(8fecb6b76907c592d545dafeaa47cf765513b3fe) )

	ROM_REGION( 0x0700, REGION_PROMS, 0 ) /* color PROMs */
	ROM_LOAD( "14-3.99",  0x0000,  0x0100, CRC(89d09126) SHA1(1f78f3b3ef8c6ba9c00a58ae89837d9a92e5078f) ) /* R */
	ROM_LOAD( "14-4.100", 0x0100,  0x0100, CRC(e1cafe6c) SHA1(8c37c3829bf1b96690fb853a2436f1b5e8d45e8c) ) /* G */
	ROM_LOAD( "14-5.101", 0x0200,  0x0100, CRC(2d444fa6) SHA1(66b64133ca740686bedd33bafd20a3f9f3df97d4) ) /* B */
	ROM_LOAD( "14-1.61",  0x0300,  0x0200, CRC(3ad8306d) SHA1(877f1d58cb8da9098ec71a7c7aec633dbf9e76e6) ) /* sprite */
	ROM_LOAD( "14-2.115", 0x0500,  0x0200, CRC(12a4f1ff) SHA1(375e37d7162053d45da66eee23d66bd432303c1c) ) /* bg */
ROM_END


GAME( 1983, markham, 0, markham, markham, 0, ROT0, "Sun Electronics", "Markham", 0 )
