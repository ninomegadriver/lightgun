/*
    Magic's 10     (c) 1995 AWP Games
    Magic's 10 2   (c) 1997 ABM Games

    driver by Pierpaolo Prazzoli

    TODO:
    - ticket / coin dispenser
    - some unknown writes
    - finish magic10_2 (association coin - credits handling its inputs
      and some reads that drive the note displayed?)

Magic's 10 instruction for the 1st boot:
- Switch "Disable Free Play" to ON
- Enter a coin
- Press Collect to get the 1st game over

*/

#include "driver.h"
#include "sound/okim6295.h"

static tilemap *layer0_tilemap, *layer1_tilemap, *layer2_tilemap;
static UINT16 *layer0_videoram, *layer1_videoram, *layer2_videoram;
static int layer2_offset[2];

static WRITE16_HANDLER( layer0_videoram_w )
{
	COMBINE_DATA(&layer0_videoram[offset]);
	tilemap_mark_tile_dirty(layer0_tilemap,offset>>1);
}

static WRITE16_HANDLER( layer1_videoram_w )
{
	COMBINE_DATA(&layer1_videoram[offset]);
	tilemap_mark_tile_dirty(layer1_tilemap,offset>>1);
}

static WRITE16_HANDLER( layer2_videoram_w )
{
	COMBINE_DATA(&layer2_videoram[offset]);
	tilemap_mark_tile_dirty(layer2_tilemap,offset>>1);
}

static WRITE16_HANDLER( paletteram_w )
{
	int r,g,b;
	data = COMBINE_DATA(&paletteram16[offset]);
	g = (data >> 0) & 0xF;
	r = (data >> 4) & 0xF;
	b = (data >> 8) & 0xF;
	palette_set_color( offset, r * 0x11, g * 0x11, b * 0x11 );
}

static WRITE16_HANDLER( magic10_misc_w )
{
/*
    lamps:
    1 -> data & 0x01
    2 -> data & 0x02
    3 -> data & 0x04
    4 -> data & 0x08
    5 -> data & 0x10
    6 -> data & 0x20
    7 -> data & 0x40
*/

/*
    data & 0x0180 toggles
    data & 0x2000 ? written with 0x400 when coin1 is inserted
*/

	coin_counter_w(0, data & 0x400);
}

static READ16_HANDLER( magic102_r )
{
	static UINT16 ret = 0;
	ret ^= 0x20;
	return ret;
}

static ADDRESS_MAP_START( magic10_map, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x03ffff) AM_ROM
	AM_RANGE(0x100000, 0x100fff) AM_RAM AM_WRITE(layer1_videoram_w) AM_BASE(&layer1_videoram)
	AM_RANGE(0x101000, 0x101fff) AM_RAM AM_WRITE(layer0_videoram_w) AM_BASE(&layer0_videoram)
	AM_RANGE(0x102000, 0x103fff) AM_RAM AM_WRITE(layer2_videoram_w) AM_BASE(&layer2_videoram)
	AM_RANGE(0x200000, 0x2007ff) AM_RAM AM_BASE(&generic_nvram16) AM_SIZE(&generic_nvram_size)
	AM_RANGE(0x300000, 0x3001ff) AM_RAM AM_WRITE(paletteram_w) AM_BASE(&paletteram16)
	AM_RANGE(0x400000, 0x400001) AM_READ(input_port_0_word_r)
	AM_RANGE(0x400002, 0x400003) AM_READ(input_port_1_word_r)
	AM_RANGE(0x400008, 0x400009) AM_WRITE(magic10_misc_w)
	AM_RANGE(0x40000a, 0x40000b) AM_READWRITE(OKIM6295_status_0_lsb_r, OKIM6295_data_0_lsb_w)
	AM_RANGE(0x40000e, 0x40000f) AM_WRITENOP
	AM_RANGE(0x400080, 0x400087) AM_WRITENOP
	AM_RANGE(0x600000, 0x603fff) AM_RAM
ADDRESS_MAP_END

static ADDRESS_MAP_START( magic10a_map, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x03ffff) AM_ROM
	AM_RANGE(0x100000, 0x100fff) AM_RAM AM_WRITE(layer1_videoram_w) AM_BASE(&layer1_videoram)
	AM_RANGE(0x101000, 0x101fff) AM_RAM AM_WRITE(layer0_videoram_w) AM_BASE(&layer0_videoram)
	AM_RANGE(0x102000, 0x103fff) AM_RAM AM_WRITE(layer2_videoram_w) AM_BASE(&layer2_videoram)
	AM_RANGE(0x200000, 0x2007ff) AM_RAM AM_BASE(&generic_nvram16) AM_SIZE(&generic_nvram_size)
	AM_RANGE(0x300000, 0x3001ff) AM_RAM AM_WRITE(paletteram_w) AM_BASE(&paletteram16)
	AM_RANGE(0x500000, 0x500001) AM_READ(input_port_0_word_r)
	AM_RANGE(0x500002, 0x500003) AM_READ(input_port_1_word_r)
	AM_RANGE(0x500008, 0x500009) AM_WRITE(magic10_misc_w)
	AM_RANGE(0x50000a, 0x50000b) AM_READWRITE(OKIM6295_status_0_lsb_r, OKIM6295_data_0_lsb_w)
	AM_RANGE(0x50000e, 0x50000f) AM_WRITENOP
	AM_RANGE(0x500080, 0x500087) AM_WRITENOP
	AM_RANGE(0x600000, 0x603fff) AM_RAM
ADDRESS_MAP_END

static ADDRESS_MAP_START( magic102_map, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x03ffff) AM_ROM
	AM_RANGE(0x100000, 0x100fff) AM_RAM AM_WRITE(layer1_videoram_w) AM_BASE(&layer1_videoram)
	AM_RANGE(0x101000, 0x101fff) AM_RAM AM_WRITE(layer0_videoram_w) AM_BASE(&layer0_videoram)
	AM_RANGE(0x102000, 0x103fff) AM_RAM AM_WRITE(layer2_videoram_w) AM_BASE(&layer2_videoram)
	AM_RANGE(0x200000, 0x2007ff) AM_RAM AM_BASE(&generic_nvram16) AM_SIZE(&generic_nvram_size)
	AM_RANGE(0x400000, 0x4001ff) AM_RAM AM_WRITE(paletteram_w) AM_BASE(&paletteram16)
	AM_RANGE(0x500000, 0x500001) AM_READ(magic102_r)
	AM_RANGE(0x500004, 0x500005) AM_READNOP // gives credits
	AM_RANGE(0x500006, 0x500007) AM_READNOP // gives credits
	AM_RANGE(0x50001a, 0x50001b) AM_READ(input_port_0_word_r)
	AM_RANGE(0x50001c, 0x50001d) AM_READ(input_port_1_word_r)
	AM_RANGE(0x500002, 0x50001f) AM_READNOP
	AM_RANGE(0x500002, 0x50001f) AM_WRITENOP
	AM_RANGE(0x600000, 0x603fff) AM_RAM
	AM_RANGE(0x700000, 0x700001) AM_READWRITE(OKIM6295_status_0_lsb_r, OKIM6295_data_0_lsb_w)
	AM_RANGE(0x700080, 0x700087) AM_WRITENOP
ADDRESS_MAP_END



INPUT_PORTS_START( magic10 )
	PORT_START
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_BUTTON3 )
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_BUTTON4 )
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_BUTTON5 )
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_OTHER ) PORT_NAME("Play") PORT_CODE(KEYCODE_X)
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_COIN2 ) PORT_NAME("Note A")
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_COIN3 ) PORT_NAME("Note B")
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_COIN4 ) PORT_NAME("Note C")
	PORT_SERVICE_NO_TOGGLE( 0x1000, IP_ACTIVE_LOW )
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_OTHER ) PORT_NAME("Out Hole") PORT_CODE(KEYCODE_V)
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_COIN5 ) PORT_NAME("Note D")
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_OTHER ) PORT_NAME("Collect") PORT_CODE(KEYCODE_C)

	PORT_START_TAG("DSW")
	PORT_DIPNAME( 0x0001, 0x0001, "Display Logo" )
	PORT_DIPSETTING(      0x0000, DEF_STR( No ) )
	PORT_DIPSETTING(      0x0001, DEF_STR( Yes ) )
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_SPECIAL ) // empty dispenser
	PORT_DIPNAME( 0x00ee, 0x00ee, "Disable Free Play" )
	PORT_DIPSETTING(      0x00ee, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0300, 0x0100, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(      0x0300, DEF_STR( Hardest ) )
	PORT_DIPSETTING(      0x0200, DEF_STR( Hard ) )
	PORT_DIPSETTING(      0x0100, DEF_STR( Medium ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( Easy ) )
	PORT_DIPNAME( 0x0c00, 0x0000, "Notes Settings" )
	PORT_DIPSETTING(      0x0000, "Note A: 10 - Note B: 20 - Note C: 50 - Note D: 100" )
	PORT_DIPSETTING(      0x0800, "Note A: 20 - Note B: 40 - Note C: 100 - Note D: 200" )
	PORT_DIPSETTING(      0x0400, "Note A: 50 - Note B: 100 - Note C: 500 - Note D: 1000" )
	PORT_DIPSETTING(      0x0c00, "Note A: 100 - Note B: 200 - Note C: 1000 - Note D: 2000" )
	PORT_DIPNAME( 0x3000, 0x3000, "Lots At" ) PORT_CONDITION("DSW",0xc000,PORTCOND_EQUALS,0xc000)
	PORT_DIPSETTING(      0x0000, "50 200 500 1000 2000" ) PORT_CONDITION("DSW",0xc000,PORTCOND_EQUALS,0xc000)
	PORT_DIPSETTING(      0x1000, "100 300 1000 3000 5000" ) PORT_CONDITION("DSW",0xc000,PORTCOND_EQUALS,0xc000)
	PORT_DIPSETTING(      0x2000, "200 500 2000 3000 5000" ) PORT_CONDITION("DSW",0xc000,PORTCOND_EQUALS,0xc000)
	PORT_DIPSETTING(      0x3000, "500 1000 2000 4000 8000" ) PORT_CONDITION("DSW",0xc000,PORTCOND_EQUALS,0xc000)
	PORT_DIPNAME( 0x3000, 0x3000, "1 Ticket Won" ) PORT_CONDITION("DSW",0xc000,PORTCOND_EQUALS,0x8000)
//  PORT_DIPSETTING(      0x0000, "Every 100 Score" ) PORT_CONDITION("DSW",0xc000,PORTCOND_EQUALS,0x8000)
//  PORT_DIPSETTING(      0x1000, "Every 100 Score" ) PORT_CONDITION("DSW",0xc000,PORTCOND_EQUALS,0x8000)
//  PORT_DIPSETTING(      0x2000, "Every 100 Score" ) PORT_CONDITION("DSW",0xc000,PORTCOND_EQUALS,0x8000)
	PORT_DIPSETTING(      0x3000, "Every 100 Score" ) PORT_CONDITION("DSW",0xc000,PORTCOND_EQUALS,0x8000)
	PORT_DIPNAME( 0x1000, 0x1000, DEF_STR( Unused ) ) PORT_CONDITION("DSW",0xc000,PORTCOND_EQUALS,0x4000)
	PORT_DIPSETTING(      0x1000, DEF_STR( Off ) ) PORT_CONDITION("DSW",0xc000,PORTCOND_EQUALS,0x4000)
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) ) PORT_CONDITION("DSW",0xc000,PORTCOND_EQUALS,0x4000)
	PORT_DIPNAME( 0x2000, 0x2000, DEF_STR( Unused ) ) PORT_CONDITION("DSW",0xc000,PORTCOND_EQUALS,0x4000)
	PORT_DIPSETTING(      0x2000, DEF_STR( Off ) ) PORT_CONDITION("DSW",0xc000,PORTCOND_EQUALS,0x4000)
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) ) PORT_CONDITION("DSW",0xc000,PORTCOND_EQUALS,0x4000)
	PORT_DIPNAME( 0x3000, 0x3000, "1 Play Won" ) PORT_CONDITION("DSW",0xc000,PORTCOND_EQUALS,0x0000)
//  PORT_DIPSETTING(      0x0000, "Every 10 Score" ) PORT_CONDITION("DSW",0xc000,PORTCOND_EQUALS,0x0000)
//  PORT_DIPSETTING(      0x1000, "Every 10 Score" ) PORT_CONDITION("DSW",0xc000,PORTCOND_EQUALS,0x0000)
//  PORT_DIPSETTING(      0x2000, "Every 10 Score" ) PORT_CONDITION("DSW",0xc000,PORTCOND_EQUALS,0x0000)
	PORT_DIPSETTING(      0x3000, "Every 10 Score" ) PORT_CONDITION("DSW",0xc000,PORTCOND_EQUALS,0x0000)
	PORT_DIPNAME( 0xc000, 0xc000, "Dispenser Type" )
	PORT_DIPSETTING(      0x0000, "MKII Hopper - Supergame" )
	PORT_DIPSETTING(      0x4000, "10 Tokens" )
	PORT_DIPSETTING(      0x8000, "Tickets Dispenser" )
	PORT_DIPSETTING(      0xc000, "Lots Dispenser" )
INPUT_PORTS_END

INPUT_PORTS_START( magic102 )
	PORT_START
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_COIN2 ) PORT_NAME("Note A")
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_COIN3 ) PORT_NAME("Note B")
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_COIN4 ) PORT_NAME("Note C")
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_COIN5 ) PORT_NAME("Note D")
	PORT_SERVICE_NO_TOGGLE( 0x0020, IP_ACTIVE_LOW )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0xff00, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_BUTTON3 )
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_BUTTON4 )
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_BUTTON5 )
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_OTHER ) PORT_NAME("Bet") PORT_CODE(KEYCODE_X)
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_OTHER ) PORT_NAME("Collect") PORT_CODE(KEYCODE_C)
	PORT_BIT( 0xff00, IP_ACTIVE_LOW, IPT_UNUSED )

/*
    credits inputs

    PORT_START
    PORT_DIPNAME( 0x01, 0x00, DEF_STR( Unknown ) )
    PORT_DIPSETTING(      0x00, DEF_STR( Off ) )
    PORT_DIPSETTING(      0x01, DEF_STR( On ) )
    PORT_DIPNAME( 0x02, 0x00, DEF_STR( Unknown ) )
    PORT_DIPSETTING(      0x00, DEF_STR( Off ) )
    PORT_DIPSETTING(      0x02, DEF_STR( On ) )
    PORT_DIPNAME( 0x04, 0x00, DEF_STR( Unknown ) )
    PORT_DIPSETTING(      0x00, DEF_STR( Off ) )
    PORT_DIPSETTING(      0x04, DEF_STR( On ) )
    PORT_DIPNAME( 0x08, 0x00, DEF_STR( Unknown ) )
    PORT_DIPSETTING(      0x00, DEF_STR( Off ) )
    PORT_DIPSETTING(      0x08, DEF_STR( On ) )
    PORT_DIPNAME( 0x10, 0x00, DEF_STR( Unknown ) )
    PORT_DIPSETTING(      0x00, DEF_STR( Off ) )
    PORT_DIPSETTING(      0x10, DEF_STR( On ) )
    PORT_DIPNAME( 0x20, 0x00, DEF_STR( Unknown ) )
    PORT_DIPSETTING(      0x00, DEF_STR( Off ) )
    PORT_DIPSETTING(      0x20, DEF_STR( On ) )
    PORT_DIPNAME( 0x40, 0x00, DEF_STR( Unknown ) )
    PORT_DIPSETTING(      0x00, DEF_STR( Off ) )
    PORT_DIPSETTING(      0x40, DEF_STR( On ) )
    PORT_DIPNAME( 0x80, 0x00, DEF_STR( Unknown ) )
    PORT_DIPSETTING(      0x00, DEF_STR( Off ) )
    PORT_DIPSETTING(      0x80, DEF_STR( On ) )
    PORT_BIT( 0xff00, IP_ACTIVE_HIGH, IPT_UNUSED )

    PORT_START
    PORT_DIPNAME( 0x01, 0x00, DEF_STR( Unknown ) )
    PORT_DIPSETTING(      0x00, DEF_STR( Off ) )
    PORT_DIPSETTING(      0x01, DEF_STR( On ) )
    PORT_DIPNAME( 0x02, 0x00, DEF_STR( Unknown ) )
    PORT_DIPSETTING(      0x00, DEF_STR( Off ) )
    PORT_DIPSETTING(      0x02, DEF_STR( On ) )
    PORT_DIPNAME( 0x04, 0x00, DEF_STR( Unknown ) )
    PORT_DIPSETTING(      0x00, DEF_STR( Off ) )
    PORT_DIPSETTING(      0x04, DEF_STR( On ) )
    PORT_DIPNAME( 0x08, 0x00, DEF_STR( Unknown ) )
    PORT_DIPSETTING(      0x00, DEF_STR( Off ) )
    PORT_DIPSETTING(      0x08, DEF_STR( On ) )
    PORT_DIPNAME( 0x10, 0x00, DEF_STR( Unknown ) )
    PORT_DIPSETTING(      0x00, DEF_STR( Off ) )
    PORT_DIPSETTING(      0x10, DEF_STR( On ) )
    PORT_DIPNAME( 0x20, 0x00, DEF_STR( Unknown ) )
    PORT_DIPSETTING(      0x00, DEF_STR( Off ) )
    PORT_DIPSETTING(      0x20, DEF_STR( On ) )
    PORT_DIPNAME( 0x40, 0x00, DEF_STR( Unknown ) )
    PORT_DIPSETTING(      0x00, DEF_STR( Off ) )
    PORT_DIPSETTING(      0x40, DEF_STR( On ) )
    PORT_DIPNAME( 0x80, 0x00, DEF_STR( Unknown ) )
    PORT_DIPSETTING(      0x00, DEF_STR( Off ) )
    PORT_DIPSETTING(      0x80, DEF_STR( On ) )
    PORT_BIT( 0xff00, IP_ACTIVE_HIGH, IPT_UNUSED )
*/
INPUT_PORTS_END

static void get_layer0_tile_info(int tile_index)
{
	SET_TILE_INFO(1,layer0_videoram[tile_index*2],layer0_videoram[tile_index*2+1] & 0xf,TILE_FLIPYX((layer0_videoram[tile_index*2+1] & 0xc0) >> 6))
}

static void get_layer1_tile_info(int tile_index)
{
	SET_TILE_INFO(1,layer1_videoram[tile_index*2],layer1_videoram[tile_index*2+1] & 0xf,TILE_FLIPYX((layer1_videoram[tile_index*2+1] & 0xc0) >> 6))
}

static void get_layer2_tile_info(int tile_index)
{
	SET_TILE_INFO(0,layer2_videoram[tile_index*2],layer2_videoram[tile_index*2+1] & 0xf,0)
}


VIDEO_START( magic10 )
{
	layer0_tilemap = tilemap_create(get_layer0_tile_info,tilemap_scan_rows,TILEMAP_OPAQUE,     16,16,32,32);
	layer1_tilemap = tilemap_create(get_layer1_tile_info,tilemap_scan_rows,TILEMAP_TRANSPARENT,16,16,32,32);
	layer2_tilemap = tilemap_create(get_layer2_tile_info,tilemap_scan_rows,TILEMAP_TRANSPARENT, 8, 8,64,64);

	if(!layer0_tilemap || !layer1_tilemap || !layer2_tilemap)
		return 1;

	tilemap_set_transparent_pen(layer1_tilemap,0);
	tilemap_set_transparent_pen(layer2_tilemap,0);

	tilemap_set_scrollx(layer2_tilemap,0,layer2_offset[0]);
	tilemap_set_scrolly(layer2_tilemap,0,layer2_offset[1]);

	return 0;
}

VIDEO_UPDATE( magic10 )
{
	tilemap_draw(bitmap,cliprect,layer0_tilemap,0,0);
	tilemap_draw(bitmap,cliprect,layer1_tilemap,0,0);
	tilemap_draw(bitmap,cliprect,layer2_tilemap,0,0);
}

static const gfx_layout tiles8x8_layout =
{
	8,8,
	RGN_FRAC(1,4),
	4,
	{ RGN_FRAC(3,4),RGN_FRAC(2,4),RGN_FRAC(1,4),RGN_FRAC(0,4) },
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8
};

static const gfx_layout tiles16x16_layout =
{
	16,16,
	RGN_FRAC(1,4),
	4,
	{ RGN_FRAC(3,4),RGN_FRAC(2,4),RGN_FRAC(1,4),RGN_FRAC(0,4) },
	{ 0, 1, 2, 3, 4, 5, 6, 7,
		16*8+0, 16*8+1, 16*8+2, 16*8+3, 16*8+4, 16*8+5, 16*8+6, 16*8+7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8,
		8*8, 9*8, 10*8, 11*8, 12*8, 13*8, 14*8, 15*8 },
	32*8
};

static const gfx_decode gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &tiles8x8_layout,   0, 16 },
	{ REGION_GFX1, 0, &tiles16x16_layout, 0, 16 },
	{ -1 } /* end of array */
};

static MACHINE_DRIVER_START( magic10 )
	MDRV_CPU_ADD_TAG("cpu", M68000, 10000000) // ?
	MDRV_CPU_PROGRAM_MAP(magic10_map,0)
	MDRV_CPU_VBLANK_INT(irq1_line_hold,1)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	MDRV_NVRAM_HANDLER(generic_0fill)

	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(64*8, 64*8)
	MDRV_VISIBLE_AREA(0*8, 44*8-1, 2*8, 32*8-1)
	MDRV_PALETTE_LENGTH(0x100)
	MDRV_GFXDECODE(gfxdecodeinfo)

	MDRV_VIDEO_START(magic10)
	MDRV_VIDEO_UPDATE(magic10)

	MDRV_SPEAKER_STANDARD_MONO("mono")

	MDRV_SOUND_ADD(OKIM6295, 8000)
	MDRV_SOUND_CONFIG(okim6295_interface_region_1)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.0)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( magic10a )

	/* basic machine hardware */
	MDRV_IMPORT_FROM(magic10)
	MDRV_CPU_MODIFY("cpu")
	MDRV_CPU_PROGRAM_MAP(magic10a_map,0)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( magic102 )

	/* basic machine hardware */
	MDRV_IMPORT_FROM(magic10)
	MDRV_CPU_MODIFY("cpu")
	MDRV_CPU_PROGRAM_MAP(magic102_map,0)

	MDRV_VISIBLE_AREA(0*8, 48*8-1, 0*8, 30*8-1)
MACHINE_DRIVER_END

/*

Magic 10 (videopoker)

1x 68k
1x 20mhz OSC near 68k
1x Oki M6295
1x 30mhz OSC near oki chip
2x fpga
1x bank of Dipswitch
1x Dallas Ds1220y-200 Nonvolatile ram

*/

ROM_START( magic10 )
	ROM_REGION( 0x40000, REGION_CPU1, 0 ) /* 68000 Code */
	ROM_LOAD16_BYTE( "u3.bin", 0x000000, 0x20000, CRC(191a46f4) SHA1(65bc22cdcc4b2f102d3eef595626819af709cacb) )
	ROM_LOAD16_BYTE( "u2.bin", 0x000001, 0x20000, CRC(a03a80bc) SHA1(a21da8912f1d2c8c2fa4a8d3ce4d43da8a934e21) )

	ROM_REGION( 0x80000, REGION_GFX1, ROMREGION_DISPOSE ) /* Tiles */
	ROM_LOAD( "u25.bin", 0x00000, 0x20000, CRC(7abb8136) SHA1(1d4daf6a4477853d89d08afb524516ef79f60dd6) )
	ROM_LOAD( "u26.bin", 0x20000, 0x20000, CRC(fd0b912d) SHA1(1cd15fa3459e7fece9fc37595f2b6848c00ffa43) )
	ROM_LOAD( "u27.bin", 0x40000, 0x20000, CRC(8178c907) SHA1(8c3440769ed4e113d84d1f8f9079783497791859) )
	ROM_LOAD( "u28.bin", 0x60000, 0x20000, CRC(dfd41aab) SHA1(82248c7fa4febb1c453f35a0e4cfae062c5da2d5) )

	ROM_REGION( 0x40000, REGION_SOUND1, 0 ) /* Samples */
	ROM_LOAD( "u22.bin", 0x00000, 0x40000, CRC(98885246) SHA1(752d549e6248074f2a7f6c5cc4d0bbc44c7fa4c3) )
ROM_END

ROM_START( magic10a )
	ROM_REGION( 0x40000, REGION_CPU1, 0 ) /* 68000 Code */
	ROM_LOAD16_BYTE( "u3_1645.bin",  0x00000, 0x20000, CRC(7f2549e4) SHA1(6578ad29273c357faae7c6be3fa1b49087e088a2) )
	ROM_LOAD16_BYTE( "u2_1645.bin",  0x00001, 0x20000, CRC(c075234e) SHA1(d9bc38f0b984082a77088fbb52b02c8f5c49846c) )

	ROM_REGION( 0x80000, REGION_GFX1, ROMREGION_DISPOSE ) /* Tile */
	ROM_LOAD( "u25.bin", 0x00000, 0x20000, CRC(7abb8136) SHA1(1d4daf6a4477853d89d08afb524516ef79f60dd6) )
	ROM_LOAD( "u26.bin", 0x20000, 0x20000, CRC(fd0b912d) SHA1(1cd15fa3459e7fece9fc37595f2b6848c00ffa43) )
	ROM_LOAD( "u27.bin", 0x40000, 0x20000, CRC(8178c907) SHA1(8c3440769ed4e113d84d1f8f9079783497791859) )
	ROM_LOAD( "u28.bin", 0x60000, 0x20000, CRC(dfd41aab) SHA1(82248c7fa4febb1c453f35a0e4cfae062c5da2d5) )

	ROM_REGION( 0x40000, REGION_SOUND1, 0 ) /* Samples */
	ROM_LOAD( "u22.bin", 0x00000, 0x40000, CRC(98885246) SHA1(752d549e6248074f2a7f6c5cc4d0bbc44c7fa4c3) )
ROM_END

/*

pcb is marked: Copyright ABM - 9605 Rev.02

1x 68000
1x osc 30mhz
1x osc 20mhz (near the 68k)
1x h8/330 HD6473308cp10
1x dipswitch
1x battery
1x fpga by Actel
1x oki6295

*/

ROM_START( magic102 )
	ROM_REGION( 0x40000, REGION_CPU1, 0 )		/* 68000 Code */
	ROM_LOAD16_BYTE( "2.u3",  0x00000, 0x20000, CRC(6fc55fe4) SHA1(392ad92e55aeac9bf5235cceb6b0b415942105a4) )
	ROM_LOAD16_BYTE( "1.u2",  0x00001, 0x20000, CRC(501507af) SHA1(ceed50c9380a9838cd3d171d2387334edfeff77f) )

	ROM_REGION( 0x80000, REGION_GFX1, ROMREGION_DISPOSE ) /* Tiles */
	ROM_LOAD( "3.u35",        0x00000, 0x20000, CRC(df47bb12) SHA1(b8bcbc9ab764d3159344d93776d13a14c9154086) )
	ROM_LOAD( "4.u36",        0x20000, 0x20000, CRC(dc242034) SHA1(6a2983c79776df07f29b77f23799fef6f20df24f) )
	ROM_LOAD( "5.u37",        0x40000, 0x20000, CRC(a048e26e) SHA1(788c28470298896902120e74fd8b9b283b8e9b79) )
	ROM_LOAD( "6.u38",        0x60000, 0x20000, CRC(469efb34) SHA1(b16646fb0c4757132e272b3877cf546b6f616786) )

	ROM_REGION( 0x40000, REGION_SOUND1, 0 ) /* Samples */
	ROM_LOAD( "7.u32",        0x00000, 0x40000, CRC(47804af7) SHA1(602dc0361869b52532e2adcb0de3cbdd042761b3) )
ROM_END

DRIVER_INIT( magic10 )
{
	layer2_offset[0] = 32;
	layer2_offset[1] = 2;
}

DRIVER_INIT( magic102 )
{
	layer2_offset[0] = 8;
	layer2_offset[1] = 20;
}

GAME( 1995, magic10,   0,       magic10,   magic10,   magic10,   ROT0, "A.W.P. Games", "Magic's 10 (ver. 16.55)", 0 )
GAME( 1995, magic10a,  magic10, magic10a,  magic10,   magic10,   ROT0, "A.W.P. Games", "Magic's 10 (ver. 16.45)", 0 )
GAME( 1997, magic102,  0,       magic102,  magic102,  magic102,  ROT0, "ABM Games",    "Magic's 10 2 (ver 1.1)", GAME_NOT_WORKING )
