/***************************************************************************

    Atari Mini Golf (prototype) driver

***************************************************************************/

#include "driver.h"

static UINT8* mgolf_video_ram;

static double time_pushed;
static double time_released;

static UINT8 prev = 0;
static UINT8 mask = 0;

static tilemap* bg_tilemap;


static void get_tile_info(int tile_index)
{
	UINT8 code = mgolf_video_ram[tile_index];

	SET_TILE_INFO(0, code, code >> 7, 0)
}


static WRITE8_HANDLER( mgolf_vram_w )
{
	if (mgolf_video_ram[offset] != data)
	{
		tilemap_mark_tile_dirty(bg_tilemap, offset);
	}

	mgolf_video_ram[offset] = data;
}


static VIDEO_START( mgolf )
{
	bg_tilemap = tilemap_create(get_tile_info, tilemap_scan_rows, TILEMAP_OPAQUE, 8, 8, 32, 32);

	return bg_tilemap == NULL;
}


static VIDEO_UPDATE( mgolf )
{
	int i;

	/* draw playfield */

	tilemap_draw(bitmap, cliprect, bg_tilemap, 0, 0);

	/* draw sprites */

	for (i = 0; i < 2; i++)
	{
		drawgfx(bitmap, Machine->gfx[1],
			mgolf_video_ram[0x399 + 4 * i],
			i,
			0, 0,
			mgolf_video_ram[0x390 + 2 * i] - 7,
			mgolf_video_ram[0x398 + 4 * i] - 16,
			cliprect, TRANSPARENCY_PEN, 0);

		drawgfx(bitmap, Machine->gfx[1],
			mgolf_video_ram[0x39b + 4 * i],
			i,
			0, 0,
			mgolf_video_ram[0x390 + 2 * i] - 15,
			mgolf_video_ram[0x39a + 4 * i] - 16,
			cliprect, TRANSPARENCY_PEN, 0);
	}
}


static void update_plunger(void)
{
	UINT8 val = readinputport(5);

	if (prev != val)
	{
		if (val == 0)
		{
			time_released = timer_get_time();

			if (!mask)
			{
				cpunum_set_input_line(0, INPUT_LINE_NMI, PULSE_LINE);
			}
		}
		else
		{
			time_pushed = timer_get_time();
		}

		prev = val;
	}
}


static void interrupt_callback(int scanline)
{
	update_plunger();

	cpunum_set_input_line(0, 0, PULSE_LINE);

	scanline = scanline + 32;

	if (scanline >= 262)
	{
		scanline = 16;
	}

	timer_set(cpu_getscanlinetime(scanline), scanline, interrupt_callback);
}


static double calc_plunger_pos(void)
{
	return (timer_get_time() - time_released) * (time_released - time_pushed + 0.2);
}


static MACHINE_RESET( mgolf )
{
	timer_set(cpu_getscanlinetime(16), 16, interrupt_callback);
}


static PALETTE_INIT( mgolf )
{
	palette_set_color(0, 0x80, 0x80, 0x80);
	palette_set_color(1, 0x00, 0x00, 0x00);
	palette_set_color(2, 0x80, 0x80, 0x80);
	palette_set_color(3, 0xff, 0xff, 0xff);
}


static READ8_HANDLER( mgolf_wram_r )
{
	return mgolf_video_ram[0x380 + offset];
}


static READ8_HANDLER( mgolf_dial_r )
{
	UINT8 val = readinputport(1);

	if ((readinputport(4) + 0x00) & 0x20)
	{
		val |= 0x01;
	}
	if ((readinputport(4) + 0x10) & 0x20)
	{
		val |= 0x02;
	}

	return val;
}


static READ8_HANDLER( mgolf_misc_r )
{
	double plunger = calc_plunger_pos(); /* see Video Pinball */

	UINT8 val = readinputport(3);

	if (plunger >= 0.000 && plunger <= 0.001)
	{
		val &= ~0x20;   /* PLUNGER1 */
	}
	if (plunger >= 0.006 && plunger <= 0.007)
	{
		val &= ~0x40;   /* PLUNGER2 */
	}

	return val;
}


static WRITE8_HANDLER( mgolf_wram_w )
{
	mgolf_video_ram[0x380 + offset] = data;
}



static ADDRESS_MAP_START( cpu_map, ADDRESS_SPACE_PROGRAM, 8 )
	ADDRESS_MAP_FLAGS( AMEF_ABITS(14) )

	AM_RANGE(0x0040, 0x0040) AM_READ(input_port_0_r)
	AM_RANGE(0x0041, 0x0041) AM_READ(mgolf_dial_r)
	AM_RANGE(0x0060, 0x0060) AM_READ(input_port_2_r)
	AM_RANGE(0x0061, 0x0061) AM_READ(mgolf_misc_r)
	AM_RANGE(0x0080, 0x00ff) AM_READ(mgolf_wram_r)
	AM_RANGE(0x0180, 0x01ff) AM_READ(mgolf_wram_r)
	AM_RANGE(0x0800, 0x0bff) AM_READ(MRA8_RAM)

	AM_RANGE(0x0000, 0x0009) AM_WRITE(MWA8_NOP)
	AM_RANGE(0x0024, 0x0024) AM_WRITE(MWA8_NOP)
	AM_RANGE(0x0028, 0x0028) AM_WRITE(MWA8_NOP)
	AM_RANGE(0x0042, 0x0042) AM_WRITE(MWA8_NOP)
	AM_RANGE(0x0044, 0x0044) AM_WRITE(MWA8_NOP) /* watchdog? */
	AM_RANGE(0x0046, 0x0046) AM_WRITE(MWA8_NOP)
	AM_RANGE(0x0060, 0x0060) AM_WRITE(MWA8_NOP)
	AM_RANGE(0x0061, 0x0061) AM_WRITE(MWA8_NOP)
	AM_RANGE(0x006a, 0x006a) AM_WRITE(MWA8_NOP)
	AM_RANGE(0x006c, 0x006c) AM_WRITE(MWA8_NOP)
	AM_RANGE(0x006d, 0x006d) AM_WRITE(MWA8_NOP)
	AM_RANGE(0x0080, 0x00ff) AM_WRITE(mgolf_wram_w)
	AM_RANGE(0x0180, 0x01ff) AM_WRITE(mgolf_wram_w)
	AM_RANGE(0x0800, 0x0bff) AM_WRITE(mgolf_vram_w) AM_BASE(&mgolf_video_ram)

	AM_RANGE(0x2000, 0x3fff) AM_ROM
ADDRESS_MAP_END


INPUT_PORTS_START( mgolf )

	PORT_START /* 40 */
	PORT_DIPNAME( 0x30, 0x00, DEF_STR( Language ) )
	PORT_DIPSETTING(    0x00, DEF_STR( English ) )
	PORT_DIPSETTING(    0x10, DEF_STR( French ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Spanish ) )
	PORT_DIPSETTING(    0x30, DEF_STR( German ) )
	PORT_DIPNAME( 0xc0, 0x40, "Shots per Coin" )
	PORT_DIPSETTING(    0x00, "25" )
	PORT_DIPSETTING(    0x40, "30" )
	PORT_DIPSETTING(    0x80, "35" )
	PORT_DIPSETTING(    0xc0, "40" )

	PORT_START /* 41 */
	PORT_BIT ( 0x01, IP_ACTIVE_HIGH, IPT_SPECIAL ) /* DIAL A */
	PORT_BIT ( 0x02, IP_ACTIVE_HIGH, IPT_SPECIAL ) /* DIAL B */
	PORT_BIT ( 0x04, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT ( 0x08, IP_ACTIVE_HIGH, IPT_VBLANK )

	PORT_START /* 60 */
	PORT_SERVICE( 0x10, IP_ACTIVE_LOW )
	PORT_BIT ( 0x20, IP_ACTIVE_LOW, IPT_TILT )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_COIN1 ) PORT_IMPULSE(1)
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_COIN2 ) PORT_IMPULSE(1)

	PORT_START /* 61 */
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_NAME("Course Select") PORT_CODE(KEYCODE_SPACE)
	PORT_BIT ( 0x20, IP_ACTIVE_LOW, IPT_SPECIAL ) /* PLUNGER 1 */
	PORT_BIT ( 0x40, IP_ACTIVE_LOW, IPT_SPECIAL ) /* PLUNGER 2 */
	PORT_BIT ( 0x80, IP_ACTIVE_LOW, IPT_START1 )

	PORT_START
	PORT_BIT( 0xff, 0x00, IPT_DIAL ) PORT_SENSITIVITY(100) PORT_KEYDELTA(25)

	PORT_START
	PORT_BIT ( 0xff, IP_ACTIVE_HIGH, IPT_BUTTON1 )

INPUT_PORTS_END


static const gfx_layout tile_layout =
{
	8, 8,
	128,
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
	8, 16,
	16,
	1,
	{ 0 },
	{
		7, 6, 5, 4, 3, 2, 1, 0,
	},
	{
		0x00, 0x08, 0x10, 0x18, 0x20, 0x28, 0x30, 0x38,
		0x40, 0x48, 0x50, 0x58, 0x60, 0x68, 0x70, 0x78
	},
	0x80
};


static const gfx_decode gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &tile_layout, 0, 2 },
	{ REGION_GFX2, 0, &sprite_layout, 0, 2 },
	{ -1 }
};


static MACHINE_DRIVER_START( mgolf )

	/* basic machine hardware */
	MDRV_CPU_ADD(M6502, 12096000 / 16) /* ? */
	MDRV_CPU_PROGRAM_MAP(cpu_map, 0)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(38 * 1000000 / 15750)
	MDRV_MACHINE_RESET(mgolf)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(256, 262)
	MDRV_VISIBLE_AREA(0, 255, 0, 223)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(4)

	MDRV_PALETTE_INIT(mgolf)
	MDRV_VIDEO_START(mgolf)
	MDRV_VIDEO_UPDATE(mgolf)

	/* sound hardware */
MACHINE_DRIVER_END


ROM_START( mgolf )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )
	ROM_LOAD_NIB_LOW ( "33496-01.e1", 0x2000, 0x0800, CRC(9ea98f39) SHA1(f5685554c2088032d3e8b9e8066bb3e8274c2425) )
	ROM_LOAD_NIB_HIGH( "33497-01.j1", 0x2000, 0x0800, CRC(0f34962b) SHA1(f71c4a008905bc87cb2ce4971fea357ed7d5d28a) )
	ROM_LOAD_NIB_LOW ( "33498-01.c2", 0x2800, 0x0800, CRC(413b616e) SHA1(dec9d9a86159a378ae79986d7fbc6f326b48c969) )
	ROM_LOAD_NIB_HIGH( "33499-01.k1", 0x2800, 0x0800, CRC(e4566326) SHA1(bc838f1bb82c865ec4357b3274ff3306336a4601) )
	ROM_LOAD_NIB_LOW ( "33500-01.e2", 0x3000, 0x0800, CRC(50bb1eb6) SHA1(6973d4817d4819fb2ada88f96f19d8248228d01f) )
	ROM_LOAD_NIB_HIGH( "33501-01.m2", 0x3000, 0x0800, CRC(a66a6ff2) SHA1(aa58349451e31b9ab28136a424e83dfc796af205) )
	ROM_LOAD_NIB_LOW ( "33502-01.j2", 0x3800, 0x0800, CRC(2177b041) SHA1(c842f8764e28c377e35458f1ae972a3c0278df45) )
	ROM_LOAD_NIB_HIGH( "33503-01.k2", 0x3800, 0x0800, CRC(db6ccbf6) SHA1(84f7b8bf37b487a386f700fb35c15a0c6e5254a4) )

	ROM_REGION( 0x0400, REGION_GFX1, ROMREGION_DISPOSE ) /* tiles */
	ROM_LOAD( "33524-01.h8", 0x0000, 0x0200, CRC(bd0e3bb3) SHA1(d833bf777118800c84fdae3d52c856375e05bc26) )
	ROM_LOAD( "33525-01.f8", 0x0200, 0x0200, CRC(7b2bac96) SHA1(2d2580b66b56de2837ccb3b60d0f24a03d018fbd) )

	ROM_REGION( 0x0100, REGION_GFX2, ROMREGION_DISPOSE ) /* sprites */
	ROM_LOAD_NIB_LOW ( "33526-01.f5", 0x0000, 0x0100, CRC(feee59ad) SHA1(6a7a3e043d7db2c2711029fcd49e1e2ff4cfde78) )
	ROM_LOAD_NIB_HIGH( "33527-01.e5", 0x0000, 0x0100, CRC(d482bdf2) SHA1(59251980bb7c6b02dcd75c46e32c9bf9d8c5e8c1) )

	ROM_REGION( 0x0200, REGION_PROMS, ROMREGION_DISPOSE )
	ROM_LOAD( "33756-01.m7", 0x0000, 0x0200, CRC(4cec9bf3) SHA1(6dd49f045fb53ae9f412639117b107faa93dfd99) )
ROM_END


GAME( 1978, mgolf, 0, mgolf, mgolf, 0, ROT270, "Atari", "Atari Mini Golf (prototype)", GAME_NO_SOUND )
