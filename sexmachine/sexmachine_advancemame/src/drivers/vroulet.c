/*

    Vegas Roulette
    World Games 1989

    LE1000

           6116  PGM  Z80    8255

                             8255  SW1

    GFX  6116                YM2149   SW3
                                      SW2
                        2148
                        2148
                        2148
                N4
    24MHz

    ---

    AGEMAME driver by Curt Coder

Any fixes for this driver should be forwarded to AGEMAME HQ forum at (http://www.mameworld.net/agemame/)

TODO:

Find 'payout on' command to add simulator

Tomasz Slanina 20050225
 - colors (4bpp tiles and 3bpp palette ? something is wrong then ....)
 - 8255x2
 - ball sprite (maybe it's something else in real machine , not sprite)
   (hardcoded tile number and palette for now .. maybe x/y must be swapped)
   are writes to 8000/c000 related to sprite tile/pal ?

*/

#include "driver.h"
#include "machine/8255ppi.h"
#include "sound/ay8910.h"

/* vidhrdw */

static UINT8 *vroulet_ball;
static tilemap *bg_tilemap;

static WRITE8_HANDLER(vroulet_paletteram_w)
{
	/*
     paletteram_xxxxBBBBGGGGRRRR_be_w
     but... each palette has 8 colors only, not 16 as expected...
    */

	int i,j,a,b;
	paletteram[offset]=data;
	for(i=0;i<32;i++)
	{
		for(j=0;j<16;j++)
		{
			a=paletteram[((i*8+j)*2)&0xff ];
			b=paletteram[((i*8+j)*2+1)&0xff ];
			palette_set_color(i*16+j,(b&0xf)<<4,(b&0xf0),(a&0xf)<<4);
		}
	}
}

static WRITE8_HANDLER( vroulet_videoram_w )
{
	if (videoram[offset] != data)
	{
		videoram[offset] = data;
		tilemap_mark_tile_dirty(bg_tilemap, offset);
	}
}

static WRITE8_HANDLER( vroulet_colorram_w )
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
	int code = videoram[tile_index] + ((attr & 0xc0) << 2);
	int color = attr & 0x1f;

	SET_TILE_INFO(0, code, color, 0)
}

VIDEO_START(vroulet)
{
	bg_tilemap = tilemap_create(get_bg_tile_info, tilemap_scan_rows,
		TILEMAP_OPAQUE, 8, 8, 32, 32);

	if ( !bg_tilemap )
		return 1;

	return 0;
}

VIDEO_UPDATE(vroulet)
{
	tilemap_draw(bitmap, cliprect, bg_tilemap, 0, 0);
	drawgfx(bitmap, Machine->gfx[0], 0x320, 1, 0, 0,
		vroulet_ball[1], vroulet_ball[0] - 12, cliprect, TRANSPARENCY_PEN, 0);
}

/* Memory Maps */

static ADDRESS_MAP_START( vroulet_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x5fff) AM_ROM
	AM_RANGE(0x6000, 0x67ff) AM_RAM AM_BASE(&generic_nvram) AM_SIZE(&generic_nvram_size)
	AM_RANGE(0x8000, 0x8000) AM_NOP
	AM_RANGE(0x9000, 0x93ff) AM_RAM AM_WRITE(vroulet_videoram_w) AM_BASE(&videoram)
	AM_RANGE(0x9400, 0x97ff) AM_RAM AM_WRITE(vroulet_colorram_w) AM_BASE(&colorram)
	AM_RANGE(0xa000, 0xa001) AM_RAM AM_BASE(&vroulet_ball)
	AM_RANGE(0xb000, 0xb0ff) AM_WRITE(vroulet_paletteram_w) AM_BASE(&paletteram)
	AM_RANGE(0xc000, 0xc000) AM_NOP
ADDRESS_MAP_END

static ADDRESS_MAP_START( vroulet_io_map, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_FLAGS( AMEF_ABITS(8) )
	AM_RANGE(0x00, 0x00) AM_READWRITE(AY8910_read_port_0_r, AY8910_write_port_0_w)
	AM_RANGE(0x01, 0x01) AM_WRITE(AY8910_control_port_0_w)
	AM_RANGE(0x10, 0x13) AM_READWRITE(ppi8255_0_r, ppi8255_0_w)
	AM_RANGE(0x80, 0x83) AM_READWRITE(ppi8255_1_r, ppi8255_1_w)
ADDRESS_MAP_END

/* Input Ports */

INPUT_PORTS_START( vroulet )
	PORT_START_TAG("IN0")
	PORT_SERVICE( 0x01, IP_ACTIVE_LOW )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_TILT )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_OTHER ) PORT_CODE(KEYCODE_F1) PORT_NAME("Memory Reset")
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON9 ) PORT_NAME("Reset Machine")
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_OTHER ) PORT_CODE(KEYCODE_4) PORT_NAME("Payout")

	PORT_START_TAG("IN1")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNUSED ) // button1 duplicate
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON3 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON4 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON5 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON6 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START1 )

	PORT_START_TAG("IN2")
	PORT_BIT( 0xff, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START_TAG("DSWA")
	PORT_DIPNAME( 0x07, 0x07, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x07, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x06, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x05, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(    0x02, "1 Coin/10 Credits" )
	PORT_DIPNAME( 0x38, 0x38, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x30, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x28, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x20, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x18, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(    0x38, "1 Coin/10 Credits" )
	PORT_DIPNAME( 0xc0, 0xc0, "Revolutions" )
	PORT_DIPSETTING(    0x80, "1" )
	PORT_DIPSETTING(    0xc0, "2" )
	PORT_DIPSETTING(    0x40, "3" )
	PORT_DIPSETTING(    0x00, "4" )

	PORT_START_TAG("DSWB")
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, "Max Payout Adjust" )
	PORT_DIPSETTING(    0x02, "48" )
	PORT_DIPSETTING(    0x00, "60" )
	PORT_DIPNAME( 0x04, 0x04, "Extra Payout Control" )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, "Red & Blue Select" )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0xe0, 0xe0, "Winning %" )
	PORT_DIPSETTING(    0xc0, "50%" )
	PORT_DIPSETTING(    0xa0, "60%" )
	PORT_DIPSETTING(    0x80, "65%" )
	PORT_DIPSETTING(    0x60, "70%" )
	PORT_DIPSETTING(    0xe0, "75%" )
	PORT_DIPSETTING(    0x40, "80%" )
	PORT_DIPSETTING(    0x20, "90%" )
	PORT_DIPSETTING(    0x00, "100%" )
INPUT_PORTS_END

/* Graphics Layout */

static const gfx_layout charlayout =
{
	8, 8,
	RGN_FRAC(1,1),
	4,
	{ 0, 1, 2, 3 },
	{ 0*4, 1*4, 2*4, 3*4, 4*4, 5*4, 6*4, 7*4 },
	{ 0*32, 1*32, 2*32, 3*32, 4*32, 5*32, 6*32, 7*32 },
	32*8
};

/* Graphics Decode Information */

static const gfx_decode gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0x0000, &charlayout,	0, 32 },
	{ -1 }
};

/* Sound Interface */

static struct AY8910interface ay8910_interface =
{
 input_port_3_r,// DSWA
 input_port_4_r,// DSWB
 0,
 0
};

/* PPI8255 Interface */

static WRITE8_HANDLER( ppi8255_a_w ){}// watchdog ?
static WRITE8_HANDLER( ppi8255_b_w ){}// lamps ?
static WRITE8_HANDLER( ppi8255_c_w ){}

static ppi8255_interface ppi8255_intf =
{
	2, 							        // 2 chips
	{ input_port_0_r,	NULL },	        // Port A read
	{ input_port_1_r,	NULL },			// Port B read
	{ input_port_2_r,	NULL },			// Port C read
	{ NULL,				ppi8255_a_w },  // Port A write
	{ NULL,				ppi8255_b_w },  // Port B write
	{ NULL,				ppi8255_c_w },  // Port C write
};

/* Machine Initialization */

static MACHINE_RESET( vroulet )
{
	ppi8255_init(&ppi8255_intf);
}

/* Machine Driver */

static MACHINE_DRIVER_START( vroulet )
	// basic machine hardware
	MDRV_CPU_ADD(Z80, 4000000)	//???
	MDRV_CPU_PROGRAM_MAP(vroulet_map, 0)
	MDRV_CPU_IO_MAP(vroulet_io_map, 0)
	MDRV_CPU_VBLANK_INT(irq0_line_hold, 1)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)
	MDRV_MACHINE_RESET(vroulet)

	MDRV_NVRAM_HANDLER(generic_1fill)

	// video hardware
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(32*8, 32*8)
	MDRV_VISIBLE_AREA(0*8, 32*8-1, 2*8, 30*8-1)

	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(128*4)

	MDRV_VIDEO_START(vroulet)
	MDRV_VIDEO_UPDATE(vroulet)

	// sound hardware
	MDRV_SPEAKER_STANDARD_MONO("mono")

	MDRV_SOUND_ADD(AY8910, 2000000)
	MDRV_SOUND_CONFIG(ay8910_interface)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.25)

MACHINE_DRIVER_END

/* ROMs */

ROM_START( vroulet )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )
	ROM_LOAD( "roul1.bin", 0x0000, 0x2000, CRC(0cff99e5) SHA1(0aa6680c4b8d780d71b3e6c6fe511f86f40abc4c) )
	ROM_LOAD( "roul2.bin", 0x2000, 0x2000, CRC(61924d9f) SHA1(8334d6825ed40e8347909817b8b73be97d23faf8) )
	ROM_LOAD( "roul3.bin", 0x4000, 0x2000, CRC(73dedff6) SHA1(d01c4fc99ac8dc03bd6e0cf779c221d403b2b648) )

	ROM_REGION( 0x8000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "roul.gfx", 0x0000, 0x8000, CRC(4e4f46d2) SHA1(efd00e2b564ff4a9013c67ffaaf91124089b310b) )
ROM_END

/* Game Driver */

GAME( 1989, vroulet, 0, vroulet, vroulet, 0, ROT90, "World Game", "Vegas Roulette", GAME_IMPERFECT_GRAPHICS | GAME_IMPERFECT_COLORS )
