/*

AGEMAME driver by Curt Coder

Any fixes for this driver should be forwarded to AGEMAME at (http://www.mameworld.net/agemame)

*/

#include "driver.h"
#include "sound/ay8910.h"

/*static UINT8 *nvram;
static size_t nvram_size;*/

/* vidhrdw */

static tilemap *bg_tilemap;

PALETTE_INIT( cm )
{
	#define COLOR(gfxn,offs) (colortable[Machine->drv->gfxdecodeinfo[gfxn].color_codes_start + offs])
	int i;

	for ( i = 0; i < Machine->drv->total_colors; i++ )
	{
		int bit0,bit1,bit2,bit3,r,g,b;

		// red component
		bit0 = (color_prom[0] >> 3) & 0x01;
		bit1 = (color_prom[0] >> 2) & 0x01;
		bit2 = (color_prom[0] >> 1) & 0x01;
		bit3 = (color_prom[0] >> 0) & 0x01;
		r = 0x0e * bit0 + 0x1f * bit1 + 0x43 * bit2 + 0x8f * bit3;

		// green component
		bit0 = (color_prom[Machine->drv->total_colors] >> 3) & 0x01;
		bit1 = (color_prom[Machine->drv->total_colors] >> 2) & 0x01;
		bit2 = (color_prom[Machine->drv->total_colors] >> 1) & 0x01;
		bit3 = (color_prom[Machine->drv->total_colors] >> 0) & 0x01;
		g = 0x0e * bit0 + 0x1f * bit1 + 0x43 * bit2 + 0x8f * bit3;

		// blue component
		bit0 = (color_prom[2*Machine->drv->total_colors] >> 3) & 0x01;
		bit1 = (color_prom[2*Machine->drv->total_colors] >> 2) & 0x01;
		bit2 = (color_prom[2*Machine->drv->total_colors] >> 1) & 0x01;
		bit3 = (color_prom[2*Machine->drv->total_colors] >> 0) & 0x01;
		b = 0x0e * bit0 + 0x1f * bit1 + 0x43 * bit2 + 0x8f * bit3;

		palette_set_color(i,r,g,b);

		color_prom++;
	}
}

static WRITE8_HANDLER( cm_videoram_w )
{
	if (videoram[offset] != data)
	{
		videoram[offset] = data;
		tilemap_mark_tile_dirty(bg_tilemap, offset);
	}
}

static WRITE8_HANDLER( cm_colorram_w )
{
	if (colorram[offset] != data)
	{
		colorram[offset] = data;
		tilemap_mark_tile_dirty(bg_tilemap, offset);
	}
}

static void get_bg_tile_info(int tile_index)
{
	int attr = colorram[tile_index];
	int code = videoram[tile_index] + ((attr & 0x0f) << 8);
	int color = (attr & 0xf0) >> 4;

	SET_TILE_INFO(0, code, color, 0)
}

#if 0
static void chrmstr_draw_sprites( mame_bitmap *bitmap, const rectangle *cliprect )
{
	// THIS IS COMPLETELY WRONG AND ONLY A PLACEHOLDER, IT HAS BEEN DISABLED TO KEEP THE COMPILER HAPPY
	int offs;

	for (offs = 0; offs < spriteram_size; offs += 4)
	{
		if (spriteram[offs + 3])
		{
			int attr = spriteram[offs + 1];
			int code = spriteram[offs+0] + ((attr & 0x40) << 2);
			int color = attr & 0x0f;
			int sx = spriteram[offs + 2] - 2 * (attr & 0x80);
			int sy = spriteram[offs + 3];
			int flipx = attr & 0x10;
			int flipy = attr & 0x20;

			drawgfx(bitmap, Machine->gfx[1], code, color, flipx, flipy, sx, sy,
				cliprect, TRANSPARENCY_COLOR, 0);
		}
	}
}
#endif

VIDEO_START(cm)
{
	bg_tilemap = tilemap_create(get_bg_tile_info, tilemap_scan_rows,
		TILEMAP_OPAQUE, 8, 8, 64, 32);

	if ( !bg_tilemap )
		return 1;

	return 0;
}

VIDEO_UPDATE(cm)
{
	tilemap_draw(bitmap, cliprect, bg_tilemap, 0, 0);
//  chrmstr_draw_sprites(bitmap, cliprect);
}

#if 0// For when NVRAM is hooked up
static NVRAM_HANDLER( cm )
{
	if (read_or_write)
	{
		mame_fwrite(file, nvram, nvram_size);
	}
	else
	{
		if (file)
		{
			mame_fread(file, nvram, nvram_size);
		}
		else
		{
			memset(nvram, 0xff, nvram_size);
		}
	}
}
#endif

static ADDRESS_MAP_START( cm_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0xbfff) AM_ROM AM_WRITENOP
	AM_RANGE(0xd000, 0xd7ff) AM_RAM AM_BASE(&spriteram) AM_SIZE(&spriteram_size)// is it here?
	AM_RANGE(0xd800, 0xdfff) AM_RAM
	AM_RANGE(0xe000, 0xe7ff) AM_RAM AM_WRITE(cm_videoram_w) AM_BASE(&videoram)
	AM_RANGE(0xe800, 0xefff) AM_RAM AM_WRITE(cm_colorram_w) AM_BASE(&colorram)
	AM_RANGE(0xf000, 0xffff) AM_RAM
ADDRESS_MAP_END

static ADDRESS_MAP_START( cm_io_map, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_FLAGS( AMEF_ABITS(8) )
	AM_RANGE(0x01, 0x01) AM_READ(AY8910_read_port_0_r)
	AM_RANGE(0x02, 0x02) AM_WRITE(AY8910_write_port_0_w)
	AM_RANGE(0x03, 0x03) AM_WRITE(AY8910_control_port_0_w)
	AM_RANGE(0x04, 0x04) AM_READ(input_port_0_r)
	AM_RANGE(0x05, 0x05) AM_READ(input_port_1_r)
	AM_RANGE(0x06, 0x06) AM_READ(input_port_2_r)
	AM_RANGE(0x07, 0x07) AM_WRITENOP
	AM_RANGE(0x08, 0x08) AM_READ(input_port_3_r)
	AM_RANGE(0x09, 0x09) AM_READ(input_port_4_r)
	AM_RANGE(0x0a, 0x0a) AM_READ(input_port_5_r)
	AM_RANGE(0x0b, 0x0b) AM_WRITENOP
	AM_RANGE(0x10, 0x10) AM_WRITENOP
	AM_RANGE(0x11, 0x11) AM_WRITENOP
	AM_RANGE(0x12, 0x12) AM_WRITENOP
	AM_RANGE(0x13, 0x13) AM_WRITENOP
ADDRESS_MAP_END

INPUT_PORTS_START( cmv801 )
	PORT_START_TAG("PLAYER")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_SERVICE ) PORT_NAME("Bit 0") PORT_CODE(KEYCODE_Q)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_SERVICE ) PORT_NAME("Bit 1") PORT_CODE(KEYCODE_W)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_NAME("P1 Big")
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON4 ) PORT_NAME("Double Up")
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON5 ) PORT_NAME("Stop / Take Score")
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_NAME("Bet")
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_NAME("P1 Small")
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START1 )

	PORT_START_TAG("COIN")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_SERVICE ) PORT_NAME("Bit 0") PORT_CODE(KEYCODE_A)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_SERVICE ) PORT_NAME("Bit 1") PORT_CODE(KEYCODE_S)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_SERVICE ) PORT_NAME("Bit 3") PORT_CODE(KEYCODE_F)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_SERVICE ) PORT_NAME("Bit 4") PORT_CODE(KEYCODE_G)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_SERVICE ) PORT_NAME("Bit 5") PORT_CODE(KEYCODE_H)
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_COIN3 )		// 50 Credits
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN2 )		// 10 Credits

	PORT_START_TAG("TEST")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_SERVICE ) PORT_NAME("Bit 0") PORT_CODE(KEYCODE_Z)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_SERVICE ) PORT_NAME("Bit 1") PORT_CODE(KEYCODE_X)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_SERVICE ) PORT_NAME("Bit 2") PORT_CODE(KEYCODE_C)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_SERVICE ) PORT_NAME("Bit 3") PORT_CODE(KEYCODE_V)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_OTHER ) PORT_CODE(KEYCODE_4) PORT_NAME("Payout")
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_SERVICE3 ) PORT_NAME("Limit Over")
	PORT_SERVICE( 0x40, IP_ACTIVE_LOW )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_OTHER ) PORT_CODE(KEYCODE_F2) PORT_NAME("Analyzer")

	PORT_START_TAG("DSW1")
	PORT_DIPNAME( 0x01, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x01, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x02, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x04, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x08, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x00, "Double Up %" )
	PORT_DIPSETTING(    0x00, "80%" )
	PORT_DIPSETTING(    0x10, "90%" )
	PORT_DIPNAME( 0x20, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x20, DEF_STR( On ) )
	PORT_DIPNAME( 0xc0, 0x00, "Bet Max" )
	PORT_DIPSETTING(    0x00, "16" )
	PORT_DIPSETTING(    0x40, "32" )
	PORT_DIPSETTING(    0x80, "64" )
	PORT_DIPSETTING(    0xc0, "96" )

	PORT_START_TAG("DSW2")
	PORT_DIPNAME( 0x07, 0x00, "Main Game %" )
	PORT_DIPSETTING(    0x07, "35%" )
	PORT_DIPSETTING(    0x06, "40%" )
	PORT_DIPSETTING(    0x05, "45%" )
	PORT_DIPSETTING(    0x04, "50%" )
	PORT_DIPSETTING(    0x03, "55%" )
	PORT_DIPSETTING(    0x02, "60%" )
	PORT_DIPSETTING(    0x01, "65%" )
	PORT_DIPSETTING(    0x00, "70%" )
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
	PORT_DIPNAME( 0x80, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x80, DEF_STR( On ) )

	PORT_START_TAG("DSW3")
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

	PORT_START_TAG("DSW4")
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
	PORT_DIPNAME( 0x20, 0x20, "Bet Over" )
	PORT_DIPSETTING(    0x20, "16" )
	PORT_DIPSETTING(    0x00, "8" )
	PORT_DIPNAME( 0x40, 0x40, "Reel Speed" )
	PORT_DIPSETTING(    0x40, DEF_STR( Low ) )
	PORT_DIPSETTING(    0x00, DEF_STR( High ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START_TAG("DSW5")
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x18, 0x18, "Cherry/Bell Bonus" )
	PORT_DIPSETTING(    0x00, "3/2" )
	PORT_DIPSETTING(    0x08, "6/3" )
	PORT_DIPSETTING(    0x10, "9/5" )
	PORT_DIPSETTING(    0x18, "12/7" )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
INPUT_PORTS_END

static const gfx_layout charlayout =
{
	8, 8,
	4096,
	3,
	{ 0, 0x8000*8, 0x10000*8 },
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8
};

static const UINT32 spritelayout_xoffset[80] =
{
	  0+0*32*8, 1+0*32*8, 2+0*32*8, 3+0*32*8, 4+0*32*8, 5+0*32*8, 6+0*32*8, 7+0*32*8,
	  0+1*32*8, 1+1*32*8, 2+1*32*8, 3+1*32*8, 4+1*32*8, 5+1*32*8, 6+1*32*8, 7+1*32*8,
	  0+2*32*8, 1+2*32*8, 2+2*32*8, 3+2*32*8, 4+2*32*8, 5+2*32*8, 6+2*32*8, 7+2*32*8,
	  0+3*32*8, 1+3*32*8, 2+3*32*8, 3+3*32*8, 4+3*32*8, 5+3*32*8, 6+3*32*8, 7+3*32*8,
	  0+4*32*8, 1+4*32*8, 2+4*32*8, 3+4*32*8, 4+4*32*8, 5+4*32*8, 6+4*32*8, 7+4*32*8,
	  0+5*32*8, 1+5*32*8, 2+5*32*8, 3+5*32*8, 4+5*32*8, 5+5*32*8, 6+5*32*8, 7+5*32*8,
	  0+6*32*8, 1+6*32*8, 2+6*32*8, 3+6*32*8, 4+6*32*8, 5+6*32*8, 6+6*32*8, 7+6*32*8,
	  0+7*32*8, 1+7*32*8, 2+7*32*8, 3+7*32*8, 4+7*32*8, 5+7*32*8, 6+7*32*8, 7+7*32*8,
	  0+8*32*8, 1+8*32*8, 2+8*32*8, 3+8*32*8, 4+8*32*8, 5+8*32*8, 6+8*32*8, 7+8*32*8,
	  0+9*32*8, 1+9*32*8, 2+9*32*8, 3+9*32*8, 4+9*32*8, 5+9*32*8, 6+9*32*8, 7+9*32*8
};

static const gfx_layout spritelayout =
{
	80, 32,
	12,
	4,
	{ 0, 0x2000*8, 0x4000*8, 0x6000*8 },
	{ 0 },
	{
		   0*8,  1*8,  2*8,  3*8,  4*8,  5*8,  6*8,  7*8,  8*8,  9*8, 10*8, 11*8, 12*8, 13*8, 14*8, 15*8,
		  16*8, 17*8, 18*8, 19*8, 20*8, 21*8, 22*8, 23*8, 24*8, 25*8, 26*8, 27*8, 28*8, 29*8, 30*8, 31*8
	},
	10*32*8,
	spritelayout_xoffset,
	NULL
};

static const gfx_decode gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0x0000, &charlayout,	  0, 16 },
	{ REGION_GFX2, 0x0020, &spritelayout, 0, 16 },
	{ -1 }
};

static struct AY8910interface ay8910_interface =
{
	input_port_6_r,	//
	input_port_7_r,	//
	0,
	0
};

static MACHINE_DRIVER_START( cmv801 )
	// basic machine hardware
	MDRV_CPU_ADD(Z80, 8000000/2)	// ???
	MDRV_CPU_PROGRAM_MAP(cm_map, 0)
	MDRV_CPU_IO_MAP(cm_io_map, 0)
	MDRV_CPU_VBLANK_INT(irq0_line_hold, 1)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	//MDRV_NVRAM_HANDLER(cmv801)

	// video hardware
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(64*8, 32*8)
	MDRV_VISIBLE_AREA(0*8, 64*8-1, 2*8, 30*8-1)

	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(256)
	MDRV_COLORTABLE_LENGTH(256)

	MDRV_PALETTE_INIT(cm)
	MDRV_VIDEO_START(cm)
	MDRV_VIDEO_UPDATE(cm)

	// sound hardware
	MDRV_SPEAKER_STANDARD_MONO("mono")
	MDRV_SOUND_ADD(AY8910,18432000/12)
	MDRV_SOUND_CONFIG(ay8910_interface)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.25)

MACHINE_DRIVER_END

ROM_START( cmv801 )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )
	ROM_LOAD( "prg512",   0x0000, 0x10000, CRC(2f6e3fe9) SHA1(c5ffa51478a0dc2d8ff6a0f286cfb461011bb55d) )

	ROM_REGION( 0x18000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "m5.256",   0x00000, 0x8000, CRC(19cc1d67) SHA1(47487f9362bfb36a32100ed772960628844462bf) )
	ROM_LOAD( "m6.256",   0x08000, 0x8000, CRC(63b3df4e) SHA1(9bacd23da598805ec18ec5ad15cab95d71eb9262) )
	ROM_LOAD( "m7.256",   0x10000, 0x8000, CRC(e39fff9c) SHA1(22fdc517fa478441622c6245cecb5728c5595757) )

	ROM_REGION( 0x8000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "m1.64",     0x0000, 0x2000, CRC(6dfcb188) SHA1(22430429c798954d9d979e62699b58feae7fdbf4) )
	ROM_LOAD( "m2.64",     0x2000, 0x2000, CRC(9678ead2) SHA1(e80aefa98b2363fe9e6b2415762695ace272e4d3) )
	ROM_LOAD( "m3.64",     0x4000, 0x2000, CRC(8607ffd9) SHA1(9bc94715554aa2473ae2ed249a47f29c7886b3dc) )
	ROM_LOAD( "m4.64",	   0x6000, 0x2000, CRC(c32367be) SHA1(ff217021b9c58e23b2226f8b0a7f5da966225715) )

	ROM_REGION( 0x300, REGION_PROMS, 0 )
	ROM_LOAD( "prom1.287", 0x0000, 0x0100, CRC(50ec383b) SHA1(ae95b92bd3946b40134bcdc22708d5c6b0f4c23e) )
	ROM_LOAD( "prom2.287", 0x0100, 0x0100, CRC(0489b760) SHA1(78f8632b17a76335183c5c204cdec856988368b0) )
	ROM_LOAD( "prom3.287", 0x0200, 0x0100, CRC(21eb5b19) SHA1(9b8425bdb97f11f4855c998c7792c3291fd07470) )
ROM_END

GAME( 198?, cmv801, 0, cmv801, cmv801, 0, ROT0, "Corsica", "Cherry Master (v8.01)", GAME_IMPERFECT_GRAPHICS | GAME_WRONG_COLORS|GAME_NOT_WORKING )
