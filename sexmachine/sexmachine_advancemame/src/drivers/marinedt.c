/*
---------------------------
Marine Date by TAITO (1981)
---------------------------

Location     Device      File ID     Checksum
---------------------------------------------
LB 3D         2716        MG01         BB4B
LB 4D         2716        MG02         89B3
LB 5D         2716        MG03         A5CE
LB 6D         2716        MG04         CE20
LB 7D         2716        MG05         16B9
LB 9D         2716        MG06         39A9
LB 10D        2716        MG07         B7F1
LB 1F         2716        MG09         9934
LB 3F         2716        MG10         F185
LB 4F         2716        MG11         1603
MB 6C         2532        MG12         66C3
MB 6H         2532        MG13         23E2
MB 2A       82S123        MG14.BPR     1CB1
MB 1A       82S123        MG15.BPR     1471
MB 4E       82S123        MG16.BPR     0570
TB 5F       82S123        MG17.BPR     129B


Notes:     TB - Top PCB        MG070001  MGN00001
           MB - Middle PCB     MG070002  MGN00002
           LB - Lower PCB      AA017779  MGN00002


Brief Hardware Overview
-----------------------

Main processor    -  Z80  2.5MHz

Sound             - Discrete audio, like Space Invaders

-------------------------------------------------------------------------

a static underwater scence with obstacles in it, like seaweed,
crabs and other stuff.  You have a limited number of "strokes"
per screen as well as a timer to work against.  Your goal is
to *bounce* yourself around the screen using *Strokes* on the
trackball to try to reach a *female* octopus before your run out
of strokes or time.  You sort of bounce yourself around the screen
like a billiard ball would bounce, but once in a while bubbles
and other stuff will come up from underneath you and carry you
away from where you are trying to get.  When you reach your goal
you get another more difficult screen, etc.

I think it was manufactured by Taito, I'm not sure but I seem to
recall that it was a full blown Japanese machine.


todo:
in cocktail mopde p1 is flipped
after inking the shark on the far right octi was moved to goal?
for the colours, goal has to be black otherwise it would register
    qas a hit, is goal pen 0 or 6?
rom writes when finishing a game
    worth looking at before the collision is correct?
playing dot hit when eaten by a shark?
dont use any ints, s/b UINT8?
enemy sprite not disabled at end of game
tilemap
palette may only be around 4 colours
    is 14 the palette?
how do you know if you've got an ink left?
prom 14 is the top bits? 4 bpp? or so?
why is level 37 chosen?
should it be 30fps?
    check other taito games of the time
look at other taito 1981 games for ideas on the ports
    bking
    jhunt?
"Marine Deto" or "Marine Date"
    look in the roms for all the text
simplify gfx decode
why does the player sprite need 4 colours?
    check if more than 1 are used
check service test ram wipes for confirmation of ram spots
    anything after trackball test?
obj1 to obj2 draw order
2nd trackball
flip/cocktail issues

done:
timer?
    you get 200 for each shot, don't think it's actually a timer
have i been using x/y consistently, ie non rotated or rotated origin?
    yes, seems to be best using xy raw (ie non-rotated)
p2 ink doesn't always light up in test mode
    after p1 ink pressed, p2 ink doesn't light up
    this is correct behavior if DSW set as Upright mode
*/

#include "driver.h"

static mame_bitmap *tile, *obj1, *obj2;
static tilemap *tx_tilemap;

static UINT8 *tx_tileram;

static UINT8 marinedt_obj1_a, marinedt_obj1_x, marinedt_obj1_y;
static UINT8 marinedt_obj2_a, marinedt_obj2_x, marinedt_obj2_y;
static UINT8 marinedt_pd, marinedt_pf;
static UINT8 marinedt_music, marinedt_sound;
static UINT8 coll, cx, cyr, cyq;
static UINT8 collh, cxh, cyrh, cyqh;

static WRITE8_HANDLER( tx_tileram_w )
{
	if (tx_tileram[offset] != data)
	{
		tx_tileram[offset] = data;
		tilemap_mark_tile_dirty(tx_tilemap, offset);
	}
}

static ADDRESS_MAP_START( marinedt_readmem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x37ff) AM_READ(MRA8_ROM)
	AM_RANGE(0x4000, 0x43ff) AM_READ(MRA8_RAM)
	AM_RANGE(0x4400, 0x47ff) AM_READ(MRA8_RAM)	//unused, vram mirror?
	AM_RANGE(0x4000, 0x4bff) AM_READ(MRA8_RAM)
ADDRESS_MAP_END

static ADDRESS_MAP_START( marinedt_writemem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x37ff) AM_WRITE(MWA8_ROM)
	AM_RANGE(0x4000, 0x47ff) AM_WRITE(MWA8_RAM)
	AM_RANGE(0x4800, 0x4bff) AM_WRITE(tx_tileram_w) AM_BASE(&tx_tileram)
	AM_RANGE(0x4c00, 0x4c00) AM_WRITE(MWA8_NOP)	//?? maybe off by one error
ADDRESS_MAP_END

static READ8_HANDLER( marinedt_port1_r )
{
//might need to be reversed for cocktail stuff

	/* x/y multiplexed */
	return readinputport(3 + ((marinedt_pf&0x08)>>3));
}

static READ8_HANDLER( marinedt_coll_r )
{
	//76543210
	//x------- obj1 to obj2 collision
	//-xxx---- unused
	//----x--- obj1 to playfield collision
	//-----xxx unused

	return coll | collh;
}

//are these returning only during a collision?
//id imagine they are returning the pf char where the collission took place?
//what about where there is lots of colls?
//maybe the first on a scanline basis
static READ8_HANDLER( marinedt_obj1_x_r )
{
	//76543210
	//xxxx---- unknown
	//----xxxx x pos in tile ram

	UINT8 *RAM = memory_region(REGION_CPU1);
if(RAM[0x430e]) --cx; else ++cx;
//figure out why inc/dec based on 430e?
	return cx | (cxh<<4);
}

static READ8_HANDLER( marinedt_obj1_yr_r )
{
	//76543210
	//xxxx---- unknown
	//----xxxx row in current screen quarter
//has to be +1 if cx went over?
if (cx==0x10) cyr++;

	return cyr | (cyrh<<4);
}

static READ8_HANDLER( marinedt_obj1_yq_r )
{
	//76543210
	//xx------ unknown
	//--xx---- screen quarter when flipped?
	//----xx-- unknown
	//------xx screen quarter

	return cyq | (cyqh<<4);
}


static ADDRESS_MAP_START( marinedt_readport, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_FLAGS( AMEF_ABITS(8) )
	AM_RANGE(0x00, 0x00) AM_READ(input_port_0_r)		//dips coinage
	AM_RANGE(0x01, 0x01) AM_READ(marinedt_port1_r)		//trackball xy muxed
	AM_RANGE(0x02, 0x02) AM_READ(marinedt_obj1_x_r)
	AM_RANGE(0x03, 0x03) AM_READ(input_port_1_r)		//buttons
	AM_RANGE(0x04, 0x04) AM_READ(input_port_2_r)		//dips
	AM_RANGE(0x06, 0x06) AM_READ(marinedt_obj1_yr_r)
	AM_RANGE(0x0a, 0x0a) AM_READ(marinedt_obj1_yq_r)
	AM_RANGE(0x0e, 0x0e) AM_READ(marinedt_coll_r)
ADDRESS_MAP_END

static WRITE8_HANDLER( marinedt_obj1_a_w ) {	marinedt_obj1_a = data; }
static WRITE8_HANDLER( marinedt_obj1_x_w ) {	marinedt_obj1_x = data; }
static WRITE8_HANDLER( marinedt_obj1_y_w ) {	marinedt_obj1_y = data; }
static WRITE8_HANDLER( marinedt_obj2_a_w ) {	marinedt_obj2_a = data; }
static WRITE8_HANDLER( marinedt_obj2_x_w ) {	marinedt_obj2_x = data; }
static WRITE8_HANDLER( marinedt_obj2_y_w ) {	marinedt_obj2_y = data; }

static WRITE8_HANDLER( marinedt_music_w ){	marinedt_music = data; }

static WRITE8_HANDLER( marinedt_sound_w )
{
	//76543210
	//xx------ ??
	//--x----- jet sound
	//---x---- foam
	//----x--- ink
	//-----x-- collision
	//------x- dots hit
	//-------x ??

	marinedt_sound = data;
}

static WRITE8_HANDLER( marinedt_pd_w )
{
	//76543210
	//xxx----- ?? unused
	//---x---- ?? the rest should be used based on the table
	//----x--- ??
	//-----x-- ??
	//------x- obj2 enable
	//-------x obj1 enable

	marinedt_pd = data;
}

/*
upright
marinedt_pf_w: 00   // upright
marinedt_pf_w: 01   // ??

cocktail
marinedt_pf_w: 02   // cocktail
marinedt_pf_w: 03   // ??

marinedt_pf_w: 01   // upright
marinedt_pf_w: 05   // flip sprite?

marinedt_pf_w: 07   // cocktail
marinedt_pf_w: 03   // non-flip sprite?
*/
static WRITE8_HANDLER( marinedt_pf_w )
{
	//76543210
	//xxxx---- ?? unused (will need to understand table of written values)
	//----x--- xy trackball select
	//-----x-- ?? flip screen/controls
	//------x- ?? upright/cocktail
	//-------x ?? service mode (coin lockout??)

	//if ((marinedt_pf & 0x07) != (data & 0x07))
	//  printf("marinedt_pf_w: %02x\n", data & 0x07);

	if ((marinedt_pf & 0x02) != (data & 0x02))
	{
		if (data & 0x02)
			printf("tile flip\n");
		else
			printf("tile non-flip\n");

		if (data & 0x02)
			tilemap_set_flip(tx_tilemap, TILEMAP_FLIPX | TILEMAP_FLIPY);
		else
			tilemap_set_flip(tx_tilemap, 0);
	}

	marinedt_pf = data;

//if(data&0xf0)
//  logerror("pf:%02x %d\n",marinedt_pf);
//logerror("pd:%02x %d\n",marinedt_pd, cpu_getcurrentframe());

}

static ADDRESS_MAP_START( marinedt_writeport, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_FLAGS( AMEF_ABITS(8) )
	AM_RANGE(0x02, 0x02) AM_WRITE(marinedt_obj1_a_w)
	AM_RANGE(0x03, 0x03) AM_WRITE(marinedt_obj1_x_w)
	AM_RANGE(0x04, 0x04) AM_WRITE(marinedt_obj1_y_w)
	AM_RANGE(0x05, 0x05) AM_WRITE(marinedt_music_w)
	AM_RANGE(0x06, 0x06) AM_WRITE(marinedt_sound_w)
	AM_RANGE(0x08, 0x08) AM_WRITE(marinedt_obj2_a_w)
	AM_RANGE(0x09, 0x09) AM_WRITE(marinedt_obj2_x_w)
	AM_RANGE(0x0a, 0x0a) AM_WRITE(marinedt_obj2_y_w)
	AM_RANGE(0x0d, 0x0d) AM_WRITE(marinedt_pd_w)
	AM_RANGE(0x0e, 0x0e) AM_WRITE(watchdog_reset_w)
	AM_RANGE(0x0f, 0x0f) AM_WRITE(marinedt_pf_w)
ADDRESS_MAP_END

INPUT_PORTS_START( marinedt )
	PORT_START	/* IN0 */
	PORT_DIPNAME( 0x0f, 0x00, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x0f, DEF_STR( 9C_1C ) )
	PORT_DIPSETTING(    0x0e, DEF_STR( 8C_1C ) )
	PORT_DIPSETTING(    0x0d, DEF_STR( 7C_1C ) )
	PORT_DIPSETTING(    0x0c, DEF_STR( 6C_1C ) )
	PORT_DIPSETTING(    0x0b, DEF_STR( 5C_1C ) )
	PORT_DIPSETTING(    0x0a, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x09, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(    0x05, DEF_STR( 1C_6C ) )
	PORT_DIPSETTING(    0x06, DEF_STR( 1C_7C ) )
	PORT_DIPSETTING(    0x07, DEF_STR( 1C_8C ) )
	PORT_DIPNAME( 0xf0, 0x00, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0xf0, DEF_STR( 9C_1C ) )
	PORT_DIPSETTING(    0xe0, DEF_STR( 8C_1C ) )
	PORT_DIPSETTING(    0xd0, DEF_STR( 7C_1C ) )
	PORT_DIPSETTING(    0xc0, DEF_STR( 6C_1C ) )
	PORT_DIPSETTING(    0xb0, DEF_STR( 5C_1C ) )
	PORT_DIPSETTING(    0xa0, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x90, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x80, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x20, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x30, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x40, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(    0x50, DEF_STR( 1C_6C ) )
	PORT_DIPSETTING(    0x60, DEF_STR( 1C_7C ) )
	PORT_DIPSETTING(    0x70, DEF_STR( 1C_8C ) )

	PORT_START	/* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_SERVICE1 )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_TILT )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_BUTTON1 ) PORT_PLAYER(2)
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_START1 )

	PORT_START	/* IN2 */
	PORT_DIPNAME( 0x01, 0x00, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x01, "5000" )
	PORT_DIPSETTING(    0x00, "10000" )
//cheat?
	PORT_DIPNAME( 0x02, 0x00, "ignore internal bounce?" )	//maybe die/bounce of rocks/coral?
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x02, DEF_STR( On ) )
//freezes the game before the reset
//doesn't seem to be done as a dip, but what about mixing with diops like this?
	PORT_BIT(0x04, IP_ACTIVE_HIGH, IPT_SERVICE ) PORT_NAME( DEF_STR( Service_Mode )) PORT_TOGGLE PORT_CODE(KEYCODE_F2)
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0x10, 0x00, "Coin Chutes" )
	PORT_DIPSETTING(    0x00, "Common" )
	PORT_DIPSETTING(    0x10, "Individual" )
	PORT_DIPNAME( 0x20, 0x00, "Year Display" )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0xc0, 0x00, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x00, "3" )
	PORT_DIPSETTING(    0x40, "4" )
	PORT_DIPSETTING(    0x80, "5" )
	PORT_DIPSETTING(    0xc0, "6" )

	PORT_START  /* IN3 - FAKE MUXED */
//check all bits are used
	PORT_BIT( 0xff, 0x00, IPT_TRACKBALL_X ) PORT_SENSITIVITY(25) PORT_KEYDELTA(10) PORT_REVERSE

	PORT_START  /* IN4 - FAKE MUXED */
	PORT_BIT( 0xff, 0x00, IPT_TRACKBALL_Y ) PORT_SENSITIVITY(25) PORT_KEYDELTA(10)
INPUT_PORTS_END

static const gfx_layout marinedt_charlayout =
{
	8,8,
	RGN_FRAC(1,3),
	3,
	{ RGN_FRAC(2,3), RGN_FRAC(1,3), RGN_FRAC(0,3) },	//maybe 120
	{ STEP8(0,1) },
	{ STEP8(0,8) },
	8*8
};

static const gfx_layout marinedt_objlayout =
{
	32,32,
	RGN_FRAC(1,1),
	2,
	{ 0, 4 },
	{ STEP4(32*8*7,1), STEP4(32*8*6,1), STEP4(32*8*5,1), STEP4(32*8*4,1), STEP4(32*8*3,1), STEP4(32*8*2,1), STEP4(32*8*1,1), STEP4(32*8*0,1) },
	{ STEP16(0,8), STEP16(16*8,8) },
	32*32*2
};

static const gfx_decode marinedt_gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &marinedt_charlayout, 0,  4 },	//really only 1 colour set?
	{ REGION_GFX2, 0, &marinedt_objlayout,  48, 4 },
	{ REGION_GFX3, 0, &marinedt_objlayout,  32, 4 },
	{ -1 }
};

PALETTE_INIT( marinedt )
{
	int i,r,b,g;

	for (i = 0;i < Machine->drv->total_colors; i++)
	{
		int bit0,bit1,bit2;

		/* red component */
		bit0 = (~color_prom[i] >> 0) & 0x01;
		bit1 = (~color_prom[i] >> 1) & 0x01;
		bit2 = (~color_prom[i] >> 2) & 0x01;
//      *(palette++) = 0x92 * bit0 + 0x46 * bit1 + 0x27 * bit2;
		r = 0x27 * bit0 + 0x46 * bit1 + 0x92 * bit2;
		/* green component */
		bit0 = (~color_prom[i] >> 3) & 0x01;
		bit1 = (~color_prom[i] >> 4) & 0x01;
		bit2 = (~color_prom[i] >> 5) & 0x01;
//      *(palette++) = 0x92 * bit0 + 0x46 * bit1 + 0x27 * bit2;
		g = 0x27 * bit0 + 0x46 * bit1 + 0x92 * bit2;
		/* blue component */
		bit0 = (~color_prom[i] >> 5) & 0x01;
		bit1 = (~color_prom[i] >> 6) & 0x01;
		bit2 = (~color_prom[i] >> 7) & 0x01;
bit0=0;
//      *(palette++) = 0x92 * bit0 + 0x46 * bit1 + 0x27 * bit2;
		b = 0x27 * bit0 + 0x46 * bit1 + 0x92 * bit2;

		palette_set_color(i, r, g, b);
	}
}


static void get_tile_info(int tile_index)
{
	int code = tx_tileram[tile_index];
	int color = 0;
	int flags = TILE_FLIPX;

	SET_TILE_INFO(0, code, color, flags)
}

VIDEO_START( marinedt )
{
	tx_tilemap = tilemap_create(get_tile_info, tilemap_scan_rows, TILEMAP_TRANSPARENT, 8, 8,32,32);
	if (!tx_tilemap)
		return 1;

	tilemap_set_transparent_pen(tx_tilemap, 0);
	tilemap_set_scrolldx(tx_tilemap, 0, 4*8);
	tilemap_set_scrolldy(tx_tilemap, 0, -4*8);

	tile = auto_bitmap_alloc(32 * 8, 32 * 8);
	obj1 = auto_bitmap_alloc(32,32);
	obj2 = auto_bitmap_alloc(32,32);

	if (!tile || !obj1 || !obj2)
		return 1;

	return 0;
}


// x------- flipy
// -x------ unused ??
// --xxx--- sprite code
// -----x-- bank
// ------xx colour

#define OBJ_CODE(a)	((((a) & 0x04) << 1) + (((a) & 0x38) >> 3))
#define OBJ_COLOR(a)	((a) & 0x03)
#define OBJ_X(x)	(256 - 32 - (x))
#define OBJ_Y(y)	(256 - 1 - (y))
#define OBJ_FLIPX(a)	((marinedt_pf & 0x02) == 0)
#define OBJ_FLIPY(a)	((a) & 0x80)

VIDEO_UPDATE( marinedt )
{
	int sx, sy;

	fillbitmap(tile, Machine->pens[0], NULL);
	tilemap_draw(tile, cliprect, tx_tilemap, 0, 0);

	fillbitmap(obj1, Machine->pens[0], NULL);
	drawgfx(obj1, Machine->gfx[1],
			OBJ_CODE(marinedt_obj1_a),
			OBJ_COLOR(marinedt_obj1_a),
			OBJ_FLIPX(marinedt_obj1_a), OBJ_FLIPY(marinedt_obj1_a),
			0, 0,
			NULL, TRANSPARENCY_PEN, 0);

	fillbitmap(obj2, Machine->pens[0], NULL);
	drawgfx(obj2, Machine->gfx[2],
			OBJ_CODE(marinedt_obj2_a),
			OBJ_COLOR(marinedt_obj2_a),
			OBJ_FLIPX(marinedt_obj2_a), OBJ_FLIPY(marinedt_obj2_a),
			0, 0,
			NULL, TRANSPARENCY_PEN, 0);

	fillbitmap(bitmap, Machine->pens[0], NULL);

	if (marinedt_pd & 0x02)
		copybitmap(bitmap, obj2, 0, 0, OBJ_X(marinedt_obj2_x), OBJ_Y(marinedt_obj2_y), cliprect, TRANSPARENCY_COLOR, Machine->pens[0]);

	if (marinedt_pd & 0x01)
		copybitmap(bitmap, obj1, 0, 0, OBJ_X(marinedt_obj1_x), OBJ_Y(marinedt_obj1_y), cliprect, TRANSPARENCY_COLOR, Machine->pens[0]);

	copybitmap(bitmap, tile, 0, 0, 0, 0, cliprect, TRANSPARENCY_COLOR, Machine->pens[0]);

	coll = cx = cyr = cyq = 0;
	if (marinedt_pd & 0x01)
	{
		for (sx = 0; sx < 32; sx++)
			for (sy = 0; sy < 32; sy++)
			{
				int x = OBJ_X(marinedt_obj1_x) + sx;
				int y = OBJ_Y(marinedt_obj1_y) + sy;

				if (x < cliprect->min_x || x > cliprect->max_x
				 || y < cliprect->min_y || y > cliprect->max_y)
					continue;

				if (read_pixel(obj1, sx, sy) == Machine->pens[0])
					continue;

				if (read_pixel(tile, x, y) != Machine->pens[0])
				{
					coll = 0x08;

					cx = (x % 128) / 8;
					cx &= 0x0f;

					cyr = ((y % 64) / 8) * 2 + (x > 127 ? 1 : 0);
					cyr &= 0x0f;

					cyq = y / 64;
					cyq &= 0x0f;

					break;
				}
			}
	}

	collh = cxh = cyrh = cyqh = 0;
	if ((marinedt_pd & 0x03) == 0x03)
	{
		for (sx = 0; sx < 32; sx++)
			for (sy = 0; sy < 32; sy++)
			{
				int x = OBJ_X(marinedt_obj1_x + sx);
				int y = OBJ_Y(marinedt_obj1_y + sy);

				int xx = OBJ_X(marinedt_obj2_x) - x;
				int yy = OBJ_Y(marinedt_obj2_y) - y;

				if (xx < 0 || xx >= 32
				 || yy < 0 || yy >= 32)
					continue;

				if (read_pixel(obj1, sx, sy) == Machine->pens[0])
					continue;

				if (read_pixel(obj2, xx, yy) != Machine->pens[0])
				{
					collh = 0x80;

					cxh = (x % 128) / 8;
					cxh &= 0x0f;

					cyrh = ((y % 64) / 8) * 2 + (x > 127 ? 1 : 0);
					cyrh &= 0x0f;

					cyqh= y / 64;
					cyqh &= 0x0f;

					break;
				}
			}
	}
}

static MACHINE_DRIVER_START( marinedt )

	/* basic machine hardware */
	MDRV_CPU_ADD(Z80,10000000/4)
	MDRV_CPU_PROGRAM_MAP(marinedt_readmem,marinedt_writemem)
	MDRV_CPU_IO_MAP(marinedt_readport,marinedt_writeport)
	MDRV_CPU_VBLANK_INT(irq0_line_hold,1)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(4*8+32*8, 32*8)
	MDRV_VISIBLE_AREA(0*8, 32*8-1, 4*8, 32*8-1)
	MDRV_GFXDECODE(marinedt_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(64)

	MDRV_PALETTE_INIT(marinedt)
	MDRV_VIDEO_START(marinedt)
	MDRV_VIDEO_UPDATE(marinedt)

	/* sound hardware */
	//discrete sound
MACHINE_DRIVER_END


/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START( marinedt )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )
	ROM_LOAD( "mg01",     0x0000, 0x0800, CRC(ad09f04d) SHA1(932fc973b4a2fbbebd7e6437ed30c8444e3d4afb))
	ROM_LOAD( "mg02",     0x0800, 0x0800, CRC(555a2b0f) SHA1(143a8953ce5070c31dc4c1f623833b2a5a2cf657))
	ROM_LOAD( "mg03",     0x1000, 0x0800, CRC(2abc79b3) SHA1(1afb331a2c0e320b6d026bc5cb47a53ac3356c2a))
	ROM_LOAD( "mg04",     0x1800, 0x0800, CRC(be928364) SHA1(8d9ae71e2751c009187e41d84fbad9519ab551e1) )
	ROM_LOAD( "mg05",     0x2000, 0x0800, CRC(44cd114a) SHA1(833165c5c00c6e505acf29fef4a3ae3f9647b443) )
	ROM_LOAD( "mg06",     0x2800, 0x0800, CRC(a7e2c69b) SHA1(614fc479d13c1726382fe7b4b0379c1dd4915af0) )
	ROM_LOAD( "mg07",     0x3000, 0x0800, CRC(b85d1f9a) SHA1(4fd3e76b1816912df84477dba4655d395f5e7072) )

	ROM_REGION( 0x1800, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "mg09",     0x0000, 0x0800, CRC(f4c349ca) SHA1(077f65eeac616a778d6c42bb95677fa2892ab697) )
	ROM_LOAD( "mg10",     0x0800, 0x0800, CRC(b41251e3) SHA1(e125a971b401c78efeb4b03d0fab43e392d3fc14) )
	ROM_LOAD( "mg11",     0x1000, 0x0800, CRC(50d66dd7) SHA1(858d1d2a75e091b0e382d964c5e4ddcd8e6f07dd))

	ROM_REGION( 0x1000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "mg12",     0x0000, 0x1000, CRC(7c6486d5) SHA1(a7f17a803937937f05fc90621883a0fd44b297a0) )

	ROM_REGION( 0x1000, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_LOAD( "mg13",     0x0000, 0x1000, CRC(17817044) SHA1(8c9b96620e3c414952e6d85c6e81b0df85c88e7a) )

	ROM_REGION( 0x0080, REGION_PROMS, 0 )
	ROM_LOAD( "mg14.bpr", 0x0000, 0x0020, CRC(f75f4e3a) SHA1(36e665987f475c57435fa8c224a2a3ce0c5e672b) )	//char clr
	ROM_LOAD( "mg15.bpr", 0x0020, 0x0020, CRC(cd3ab489) SHA1(a77478fb94d0cf8f4317f89cc9579def7c294b4f) )	//obj clr
	ROM_LOAD( "mg16.bpr", 0x0040, 0x0020, CRC(92c868bc) SHA1(483ae6f47845ddacb701528e82bd388d7d66a0fb) )	//?? collisions
	ROM_LOAD( "mg17.bpr", 0x0060, 0x0020, CRC(13261a02) SHA1(050edd18e4f79d19d5206f55f329340432fd4099) )	//?? table of increasing values
ROM_END

GAME( 1981, marinedt, 0, marinedt, marinedt, 0, ROT270, "Taito", "Marine Date", GAME_NO_SOUND )
