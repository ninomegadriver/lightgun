/**********************************************************
Strike Bowling  (c)1982 Taito

driver by Jaroslaw Burczynski
          Tomasz Slanina

Todo:
 - analog sound
 - colors

***********************************************************

Runs on 3 board (color) hardware, similar to Space Invaders,
but enhanced slightly (more ram, updated sound hardware etc.)

Top Board
---------
PCB No: KBO70001  KBN00001
DIPSW : 8 position x2
SOUND : AY-3-8910
Volume POTs x4 (Master volume + 3 for separate sound levels)

Middle Board
------------
PCB No: KBO70002  KBN00002
CPU   : 8080
XTAL  : 19.968MHz
RAM   : 2114 x2
ROMs  : 2732 x3 (main program)

Bottom Board
------------
PCB No: KBO70003  KBN00003
RAM   : TMS4060 x32
ROMs  : 2716 x3, 2732 x1
PROMs : NEC B406 (1kx4) x2

***********************************************************/

#include "driver.h"
#include "cpu/i8039/i8039.h"
#include "vidhrdw/res_net.h"
#include "sound/ay8910.h"


static int bgmap = 0;

static int sbw_system = 0;
static tilemap *sb_tilemap;
static UINT32 color_prom_address = 0;
static UINT8 pix_sh = 0;
static UINT8 pix[2] = {0, 0};

static void get_sb_tile_info(int tile_index)
{
	unsigned char *rom = memory_region(REGION_USER1);
	int tileno = rom[tile_index + bgmap * 1024];

	SET_TILE_INFO(0, tileno, 0, 0)
}

static void plot_pixel_sbw(int x, int y, int col)
{
	if (flip_screen)
	{
		y = 255-y;
		x = 247-x;
	}
	plot_pixel(tmpbitmap,x,y,Machine->pens[col]);
}

static WRITE8_HANDLER( sbw_videoram_w )
{
	int x,y,i,v1,v2;

	videoram[offset] = data;

	offset &= 0x1fff;

	y = offset / 32;
	x = (offset % 32) * 8;

	v1 = videoram[offset];
	v2 = videoram[offset+0x2000];

	for(i = 0; i < 8; i++)
	{
		plot_pixel_sbw(x++, y, color_prom_address | ( ((v1&1)*0x20) | ((v2&1)*0x40) ) );
		v1 >>= 1;
		v2 >>= 1;
	}
}

VIDEO_UPDATE(sbowling)
{
	fillbitmap(bitmap,Machine->pens[0x18],cliprect);
	tilemap_draw(bitmap,cliprect,sb_tilemap,0,0);
	copybitmap(bitmap,tmpbitmap,0,0,0,0,cliprect, TRANSPARENCY_PEN, color_prom_address);
}

VIDEO_START(sbowling)
{
	tmpbitmap = auto_bitmap_alloc(32*8,32*8);
	sb_tilemap = tilemap_create(get_sb_tile_info, tilemap_scan_rows, TILEMAP_OPAQUE, 8, 8, 32, 32);
	return 0;
}

static WRITE8_HANDLER( pix_shift_w )
{
	pix_sh = data;
}
static WRITE8_HANDLER( pix_data_w )
{
	pix[0] = pix[1];
	pix[1] = data;
}
static READ8_HANDLER( pix_data_r )
{
	UINT32 p1, p0;
	int res;
	int sh = pix_sh & 7;

	p1 = pix[1];
	p0 = pix[0];

	res = (((p1 << (sh+8)) | (p0 << sh)) & 0xff00) >> 8;

	return res;
}



static INTERRUPT_GEN( sbw_interrupt )
{
	int vector = cpu_getvblank() ? 0xcf : 0xd7;	/* RST 08h/10h */

	cpunum_set_input_line_and_vector(0, 0, HOLD_LINE, vector);
}

static WRITE8_HANDLER (system_w)
{
	/*
        76543210
        -------x flip screen/controls?
        ------x- trackball x/y  select
        -----x-- 1 ?
        ----x--- flip screen/controls
    */
	flip_screen_set(data&1);

	if((sbw_system^data)&1)
	{
		int offs;
		for (offs = 0;offs < videoram_size; offs++)
			sbw_videoram_w(offs, videoram[offs]);
	}
	sbw_system = data;
}

static WRITE8_HANDLER(graph_control_w)
{
	/*
        76543210
        -----xxx color PROM address lines A9,A8,A7
        ----?--- nc ?
        --xx---- background image select (address lines on tilemap rom)
        xx------ color PROM address lines A4,A3
    */

	color_prom_address = ((data&0x07)<<7) | ((data&0xc0)>>3);

	bgmap = ((data>>4)^3) & 0x3;
	tilemap_mark_all_tiles_dirty(sb_tilemap);
}

static READ8_HANDLER (controls_r)
{
	if(sbw_system&2)
		return input_port_2_r(0);
	else
		return input_port_3_r(0);
}

static ADDRESS_MAP_START( main_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x2fff) AM_ROM
	AM_RANGE(0x8000, 0xbfff) AM_READWRITE(MRA8_RAM, sbw_videoram_w) AM_BASE(&videoram) AM_SIZE(&videoram_size)
	AM_RANGE(0xf800, 0xf800) AM_WRITE(AY8910_control_port_0_w)
	AM_RANGE(0xf801, 0xf801) AM_READWRITE(AY8910_read_port_0_r, AY8910_write_port_0_w)
	AM_RANGE(0xfc00, 0xffff) AM_RAM
ADDRESS_MAP_END


static ADDRESS_MAP_START( port_map, ADDRESS_SPACE_IO, 8 )
	AM_RANGE(0x00, 0x00) AM_READWRITE(input_port_0_r, watchdog_reset_w)
	AM_RANGE(0x01, 0x01) AM_READWRITE(controls_r, pix_data_w)
	AM_RANGE(0x02, 0x02) AM_READWRITE(pix_data_r, pix_shift_w)
	AM_RANGE(0x03, 0x03) AM_READWRITE(input_port_1_r, MWA8_NOP)
	AM_RANGE(0x04, 0x04) AM_READWRITE(input_port_4_r, system_w)
	AM_RANGE(0x05, 0x05) AM_READWRITE(input_port_5_r, graph_control_w)
ADDRESS_MAP_END



INPUT_PORTS_START( sbowling )
	PORT_START
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_COIN1   )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH,	IPT_TILT )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_COIN2 )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_START2 )

	PORT_START
	PORT_BIT( 0xff, 0, IPT_TRACKBALL_Y ) PORT_SENSITIVITY(30) PORT_KEYDELTA(30)

	PORT_START
	PORT_BIT( 0xff, 0, IPT_TRACKBALL_X ) PORT_SENSITIVITY(30) PORT_KEYDELTA(30) PORT_REVERSE

	PORT_START	/* coin slots: A 4 LSB, B 4 MSB */
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
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, "Year Display" )
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
	PORT_DIPNAME( 0x40, 0x00, "Ball Controll Check" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x40, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x00, "Video Test" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x80, DEF_STR( On ) )
INPUT_PORTS_END

static const gfx_layout charlayout =
{
	8,8,
	256,
	3,
	{ 0x800*0*8, 0x800*1*8, 0x800*2*8 },
	{ 7, 6, 5, 4, 3, 2, 1, 0 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8
};

static const gfx_decode gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &charlayout,   0x18, 1 },
	{ -1 }
};


static PALETTE_INIT( sbowling )
{
	int i;

	static const int resistances_rg[3] = { 470, 270, 100 };
	static const int resistances_b[2]  = { 270, 100 };
	double outputs_r[1<<3], outputs_g[1<<3], outputs_b[1<<2];

	/* the game uses output collector PROMs type: NEC B406  */
	compute_resistor_net_outputs(0, 255,	-1.0,
		3,	resistances_rg, outputs_r,	0,	100,
		3,	resistances_rg, outputs_g,	0,	100,
		2,	resistances_b,  outputs_b,	0,	100);

	for (i = 0;i < Machine->drv->total_colors;i++)
	{
		int bit0,bit1,bit2,r,g,b;

		/* blue component */
		bit0 = (color_prom[i] >> 0) & 0x01;
		bit1 = (color_prom[i] >> 1) & 0x01;
		b = (int)(outputs_b[ (bit0<<0) | (bit1<<1) ] + 0.5);

		/* green component */
		bit0 = (color_prom[i] >> 2) & 0x01;
		bit1 = (color_prom[i] >> 3) & 0x01;
		bit2 = (color_prom[i+0x400] >> 0) & 0x01;
		g = (int)(outputs_g[ (bit0<<0) | (bit1<<1) | (bit2<<2) ] + 0.5);

		/* red component */
		bit0 = (color_prom[i+0x400] >> 1) & 0x01;
		bit1 = (color_prom[i+0x400] >> 2) & 0x01;
		bit2 = (color_prom[i+0x400] >> 3) & 0x01;
		r = (int)(outputs_r[ (bit0<<0) | (bit1<<1) | (bit2<<2) ] + 0.5);

		palette_set_color(i,r,g,b);
	}
}

static MACHINE_DRIVER_START( sbowling )

	MDRV_CPU_ADD(8080, 19968000/10 )
	MDRV_CPU_PROGRAM_MAP(main_map,0)
	MDRV_CPU_IO_MAP(port_map,0)
	MDRV_CPU_VBLANK_INT(sbw_interrupt, 2)
	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)
	MDRV_GFXDECODE(gfxdecodeinfo)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(32*8, 32*8)
	MDRV_VISIBLE_AREA(1*8, 31*8-1, 4*8, 32*8-1)
	MDRV_PALETTE_LENGTH(0x400)
	MDRV_PALETTE_INIT(sbowling)
	MDRV_VIDEO_START(sbowling)
	MDRV_VIDEO_UPDATE(sbowling)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")

	MDRV_SOUND_ADD(AY8910, 19968000 / 16)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.12)
MACHINE_DRIVER_END

ROM_START( sbowling )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )
	ROM_LOAD( "kb01.6h",        0x0000, 0x1000, CRC(dd5d411a) SHA1(ca15676d234353bc47f642be13d58f3d6d880126))
	ROM_LOAD( "kb02.5h",        0x1000, 0x1000, CRC(75d3c45f) SHA1(af6e6237b7b28efaac258e6ddd85518c3406b24a))
	ROM_LOAD( "kb03.3h",        0x2000, 0x1000, CRC(955fbfb8) SHA1(05d501f924adc5b816670f6f5e58a98a0c1bc962))

	ROM_REGION( 0x1800, REGION_GFX1, 0 )
	ROM_LOAD( "kb05.9k",        0x0000, 0x800,  CRC(4b4d9569) SHA1(d69e69add69ec11724090e34838ec8c61de81f4e))
	ROM_LOAD( "kb06.7k",        0x0800, 0x800,  CRC(d89ba78b) SHA1(9e01be976e1e14feb8f7bd9f699a977a15a72e0d))
	ROM_LOAD( "kb07.6k",        0x1000, 0x800,  CRC(9fb5db1a) SHA1(0b28ca5277ebe0d78d1a3f2d414efb5fd7c6e9ee))

	ROM_REGION( 0x01000, REGION_USER1, 0 )
	ROM_LOAD( "kb04.10k",       0x0000, 0x1000, CRC(1c27adc1) SHA1(a68748fbdbd8fb48f20b3675d793e5c156d1bd02))

	ROM_REGION( 0x0800, REGION_PROMS, 0 )
	ROM_LOAD( "kb08.7m",        0x0000, 0x0400, CRC(e949e441) SHA1(8e0fe71ed6d4e6f94a703c27a8364da27b443730))
	ROM_LOAD( "kb09.6m",        0x0400, 0x0400, CRC(e29191a6) SHA1(9a2c78a96ef6d118f4dacbea0b7d454b66a452ae))
ROM_END

GAME( 1982, sbowling, 0, sbowling, sbowling, 0, ROT90, "Taito Corporation", "Strike Bowling",GAME_IMPERFECT_SOUND)

