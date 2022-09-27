/***************************************************************************

    Atari Cannonball (prototype) driver

***************************************************************************/

#include "driver.h"

static UINT8* cball_video_ram;

static tilemap* bg_tilemap;


static void get_tile_info(int tile_index)
{
	UINT8 code = cball_video_ram[tile_index];

	SET_TILE_INFO(0, code, code >> 7, 0)
}


static WRITE8_HANDLER( cball_vram_w )
{
	if (cball_video_ram[offset] != data)
	{
		tilemap_mark_tile_dirty(bg_tilemap, offset);
	}

	cball_video_ram[offset] = data;
}


static VIDEO_START( cball )
{
	bg_tilemap = tilemap_create(get_tile_info, tilemap_scan_rows, TILEMAP_OPAQUE, 8, 8, 32, 32);

	return bg_tilemap == NULL;
}


static VIDEO_UPDATE( cball )
{
	/* draw playfield */

	tilemap_draw(bitmap, cliprect, bg_tilemap, 0, 0);

	/* draw sprite */

	drawgfx(bitmap, Machine->gfx[1],
		cball_video_ram[0x399] >> 4,
		0,
		0, 0,
		240 - cball_video_ram[0x390],
		240 - cball_video_ram[0x398],
		cliprect, TRANSPARENCY_PEN, 0);
}


static void interrupt_callback(int scanline)
{
	cpunum_set_input_line(0, 0, PULSE_LINE);

	scanline = scanline + 32;

	if (scanline >= 262)
	{
		scanline = 16;
	}

	timer_set(cpu_getscanlinetime(scanline), scanline, interrupt_callback);
}


static MACHINE_RESET( cball )
{
	timer_set(cpu_getscanlinetime(16), 16, interrupt_callback);
}


static PALETTE_INIT( cball )
{
	palette_set_color(0, 0x80, 0x80, 0x80);
	palette_set_color(1, 0x00, 0x00, 0x00);
	palette_set_color(2, 0x80, 0x80, 0x80);
	palette_set_color(3, 0xff, 0xff, 0xff);
	palette_set_color(4, 0x80, 0x80, 0x80);
	palette_set_color(5, 0xc0, 0xc0, 0xc0);
}


static READ8_HANDLER( cball_wram_r )
{
	return cball_video_ram[0x380 + offset];
}


static WRITE8_HANDLER( cball_wram_w )
{
	cball_video_ram[0x380 + offset] = data;
}



static ADDRESS_MAP_START( cpu_map, ADDRESS_SPACE_PROGRAM, 8 )
	ADDRESS_MAP_FLAGS( AMEF_ABITS(15) )

	AM_RANGE(0x0000, 0x03ff) AM_READ(cball_wram_r) AM_MASK(0x7f)
	AM_RANGE(0x0400, 0x07ff) AM_READ(MRA8_RAM)
	AM_RANGE(0x1001, 0x1001) AM_READ(input_port_0_r)
	AM_RANGE(0x1003, 0x1003) AM_READ(input_port_1_r)
	AM_RANGE(0x1020, 0x1020) AM_READ(input_port_2_r)
	AM_RANGE(0x1040, 0x1040) AM_READ(input_port_3_r)
	AM_RANGE(0x1060, 0x1060) AM_READ(input_port_4_r)
	AM_RANGE(0x2000, 0x2001) AM_NOP
	AM_RANGE(0x2800, 0x2800) AM_READ(input_port_5_r)

	AM_RANGE(0x0000, 0x03ff) AM_WRITE(cball_wram_w) AM_MASK(0x7f)
	AM_RANGE(0x0400, 0x07ff) AM_WRITE(cball_vram_w) AM_BASE(&cball_video_ram)
	AM_RANGE(0x1800, 0x1800) AM_NOP /* watchdog? */
	AM_RANGE(0x1810, 0x1811) AM_NOP
	AM_RANGE(0x1820, 0x1821) AM_NOP
	AM_RANGE(0x1830, 0x1831) AM_NOP
	AM_RANGE(0x1840, 0x1841) AM_NOP
	AM_RANGE(0x1850, 0x1851) AM_NOP
	AM_RANGE(0x1870, 0x1871) AM_NOP

	AM_RANGE(0x7000, 0x7fff) AM_ROM
ADDRESS_MAP_END


INPUT_PORTS_START( cball )

	PORT_START /* 1001 */
	PORT_DIPNAME( 0xc0, 0xc0, DEF_STR( Coinage ))
	PORT_DIPSETTING(    0x00, "2 Coins each" )
	PORT_DIPSETTING(    0xc0, "1 Coin each" )
	PORT_DIPSETTING(    0x80, "1 Coin 1 Game" )
	PORT_DIPSETTING(    0x40, "1 Coin 2 Games" )

	PORT_START /* 1003 */
	PORT_DIPNAME( 0xc0, 0xc0, DEF_STR( Lives ))
	PORT_DIPSETTING(    0xc0, "3" )
	PORT_DIPSETTING(    0x80, "5" )
	PORT_DIPSETTING(    0x40, "7" )
	PORT_DIPSETTING(    0x00, "9" )

	PORT_START /* 1020 */
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START1 )

	PORT_START /* 1040 */
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN1 )

	PORT_START /* 1060 */
	PORT_SERVICE( 0x80, IP_ACTIVE_LOW )

	PORT_START /* 2800 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON2 )

INPUT_PORTS_END


static const gfx_layout tile_layout =
{
	8, 8,
	64,
	1,
	{ 0 },
	{
		0, 1, 2, 3, 4, 5, 6, 7
	},
	{
		0x00, 0x08, 0x10, 0x18, 0x20, 0x28, 0x30, 0x38
	},
	0x40
};


static const gfx_layout sprite_layout =
{
	16, 16,
	16,
	1,
	{ 0 },
	{
		0x7, 0x6, 0x5, 0x4, 0x3, 0x2, 0x1, 0x0,
		0xf, 0xe, 0xd, 0xc, 0xb, 0xa, 0x9, 0x8
	},
	{
		0x00, 0x10, 0x20, 0x30, 0x40, 0x50, 0x60, 0x70,
		0x80, 0x90, 0xa0, 0xb0, 0xc0, 0xd0, 0xe0, 0xf0
	},
	0x100
};


static const gfx_decode gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &tile_layout, 0, 2 },
	{ REGION_GFX2, 0, &sprite_layout, 4, 1 },
	{ -1 }
};


static MACHINE_DRIVER_START( cball )

	/* basic machine hardware */
	MDRV_CPU_ADD(M6800, 12096000 / 16) /* ? */
	MDRV_CPU_PROGRAM_MAP(cpu_map, 0)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(38 * 1000000 / 15750)
	MDRV_MACHINE_RESET(cball)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(256, 262)
	MDRV_VISIBLE_AREA(0, 255, 0, 223)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(6)

	MDRV_PALETTE_INIT(cball)
	MDRV_VIDEO_START(cball)
	MDRV_VIDEO_UPDATE(cball)

	/* sound hardware */
MACHINE_DRIVER_END


ROM_START( cball )
	ROM_REGION( 0x8000, REGION_CPU1, 0 )
	ROM_LOAD_NIB_LOW ( "canball.1e", 0x7400, 0x0400, CRC(0b34823b) SHA1(0db6b9f78f7c07ee7d35f2bf048ba61fe43b1e26) )
	ROM_LOAD_NIB_HIGH( "canball.1l", 0x7400, 0x0400, CRC(b43ca275) SHA1(a03e03f6366877cfdcec71030a5fb2c5171c8d8a) )
	ROM_LOAD_NIB_LOW ( "canball.1f", 0x7800, 0x0400, CRC(29b4e1f7) SHA1(8cef944b6e0153c304aa2d4cfdc530b8a4eef021) )
	ROM_LOAD_NIB_HIGH( "canball.1k", 0x7800, 0x0400, CRC(a4d1cf12) SHA1(99de6470efd16e57d72019e065f55bc740f3c7fc) )
	ROM_LOAD_NIB_LOW ( "canball.1h", 0x7c00, 0x0400, CRC(13f55937) SHA1(7514c27e60944c4e00992c8ecbc5115f8ff948bb) )
	ROM_LOAD_NIB_HIGH( "canball.1j", 0x7c00, 0x0400, CRC(5b905d69) SHA1(2408dd6e44c51c0c9bdb82d2d33826c03f8308c4) )

	ROM_REGION( 0x0200, REGION_GFX1, ROMREGION_DISPOSE ) /* tiles */
	ROM_LOAD_NIB_LOW ( "canball.6m", 0x0000, 0x0200, NO_DUMP )
	ROM_LOAD_NIB_HIGH( "canball.6l", 0x0000, 0x0200, CRC(5b1c9e88) SHA1(6e9630db9907170c53942a21302bcf8b721590a3) )

	ROM_REGION( 0x0200, REGION_GFX2, ROMREGION_DISPOSE ) /* sprites */
	ROM_LOAD_NIB_LOW ( "canball.5l", 0x0000, 0x0200, CRC(3d0d1569) SHA1(1dfcf5cf9468d476c4b7d76a261c6fec87a99f93) )
	ROM_LOAD_NIB_HIGH( "canball.5k", 0x0000, 0x0200, CRC(c5fdd3c8) SHA1(5aae148439683ff1cf0005a810c81fdcbed525c3) )

	ROM_REGION( 0x0100, REGION_PROMS, ROMREGION_DISPOSE )
	ROM_LOAD( "canball.6h", 0x0000, 0x0100, CRC(b8094b4c) SHA1(82dc6799a19984f3b204ee3aeeb007e55afc8be3) ) /* sync */
ROM_END


GAME( 1976, cball, 0, cball, cball, 0, ROT0, "Atari", "Cannonball (Atari, prototype)", GAME_NO_SOUND | GAME_WRONG_COLORS | GAME_IMPERFECT_GRAPHICS )
