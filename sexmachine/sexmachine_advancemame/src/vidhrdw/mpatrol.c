/***************************************************************************

  vidhrdw.c

  Functions to emulate the video hardware of the machine.

***************************************************************************/

#include "driver.h"

#define BGHEIGHT 64

static unsigned char bg1xpos;
static unsigned char bg1ypos;
static unsigned char bg2xpos;
static unsigned char bg2ypos;
static unsigned char bgcontrol;

static tilemap* bg_tilemap;


/***************************************************************************

  Convert the color PROMs into a more useable format.

  Moon Patrol has one 256x8 character palette PROM, one 32x8 background
  palette PROM, one 32x8 sprite palette PROM and one 256x4 sprite lookup
  table PROM.

  The character and background palette PROMs are connected to the RGB output
  this way:

  bit 7 -- 220 ohm resistor  -- BLUE
        -- 470 ohm resistor  -- BLUE
        -- 220 ohm resistor  -- GREEN
        -- 470 ohm resistor  -- GREEN
        -- 1  kohm resistor  -- GREEN
        -- 220 ohm resistor  -- RED
        -- 470 ohm resistor  -- RED
  bit 0 -- 1  kohm resistor  -- RED

  The sprite palette PROM is connected to the RGB output this way. Note that
  RED and BLUE are swapped wrt the usual configuration.

  bit 7 -- 220 ohm resistor  -- RED
        -- 470 ohm resistor  -- RED
        -- 220 ohm resistor  -- GREEN
        -- 470 ohm resistor  -- GREEN
        -- 1  kohm resistor  -- GREEN
        -- 220 ohm resistor  -- BLUE
        -- 470 ohm resistor  -- BLUE
  bit 0 -- 1  kohm resistor  -- BLUE

***************************************************************************/
PALETTE_INIT( mpatrol )
{
	int i;
	#define TOTAL_COLORS(gfxn) (Machine->gfx[gfxn]->total_colors * Machine->gfx[gfxn]->color_granularity)
	#define COLOR(gfxn,offs) (colortable[Machine->drv->gfxdecodeinfo[gfxn].color_codes_start + offs])


	/* character palette */
	for (i = 0;i < 512;i++)
	{
		int bit0,bit1,bit2,r,g,b;

		COLOR(0, i) = i;

		/* red component */
		bit0 = (*color_prom >> 0) & 0x01;
		bit1 = (*color_prom >> 1) & 0x01;
		bit2 = (*color_prom >> 2) & 0x01;
		r = 0x21 * bit0 + 0x47 * bit1 + 0x97 * bit2;
		/* green component */
		bit0 = (*color_prom >> 3) & 0x01;
		bit1 = (*color_prom >> 4) & 0x01;
		bit2 = (*color_prom >> 5) & 0x01;
		g = 0x21 * bit0 + 0x47 * bit1 + 0x97 * bit2;
		/* blue component */
		bit0 = 0;
		bit1 = (*color_prom >> 6) & 0x01;
		bit2 = (*color_prom >> 7) & 0x01;
		b = 0x21 * bit0 + 0x47 * bit1 + 0x97 * bit2;

		palette_set_color(i,r,g,b);
		color_prom++;
	}

	/* background palette */
	for (i = 0;i < 32;i++)
	{
		int bit0,bit1,bit2,r,g,b;


		/* red component */
		bit0 = (*color_prom >> 0) & 0x01;
		bit1 = (*color_prom >> 1) & 0x01;
		bit2 = (*color_prom >> 2) & 0x01;
		r = 0x21 * bit0 + 0x47 * bit1 + 0x97 * bit2;
		/* green component */
		bit0 = (*color_prom >> 3) & 0x01;
		bit1 = (*color_prom >> 4) & 0x01;
		bit2 = (*color_prom >> 5) & 0x01;
		g = 0x21 * bit0 + 0x47 * bit1 + 0x97 * bit2;
		/* blue component */
		bit0 = 0;
		bit1 = (*color_prom >> 6) & 0x01;
		bit2 = (*color_prom >> 7) & 0x01;
		b = 0x21 * bit0 + 0x47 * bit1 + 0x97 * bit2;

		palette_set_color(i+512,r,g,b);
		color_prom++;
	}

	/* color_prom now points to the beginning of the sprite palette */

	/* sprite palette */
	for (i = 0;i < 32;i++)
	{
		int bit0,bit1,bit2,r,g,b;


		/* red component */
		bit0 = 0;
		bit1 = (*color_prom >> 6) & 0x01;
		bit2 = (*color_prom >> 7) & 0x01;
		r = 0x21 * bit0 + 0x47 * bit1 + 0x97 * bit2;
		/* green component */
		bit0 = (*color_prom >> 3) & 0x01;
		bit1 = (*color_prom >> 4) & 0x01;
		bit2 = (*color_prom >> 5) & 0x01;
		g = 0x21 * bit0 + 0x47 * bit1 + 0x97 * bit2;
		/* blue component */
		bit0 = (*color_prom >> 0) & 0x01;
		bit1 = (*color_prom >> 1) & 0x01;
		bit2 = (*color_prom >> 2) & 0x01;
		b = 0x21 * bit0 + 0x47 * bit1 + 0x97 * bit2;

		palette_set_color(i+512+32,r,g,b);
		color_prom++;
	}

	/* color_prom now points to the beginning of the sprite lookup table */

	/* sprite lookup table */
	for (i = 0;i < TOTAL_COLORS(1);i++)
	{
		COLOR(1,i) = 512+32 + (*color_prom++);
		if (i % 4 == 3) color_prom += 4;	/* half of the PROM is unused */
	}

	/* background */
	/* the palette is a 32x8 PROM with many colors repeated. The address of */
	/* the colors to pick is as follows: */
	/* xbb00: mountains */
	/* 0xxbb: hills */
	/* 1xxbb: city */
	COLOR(2,0) = 512;
	COLOR(2,1) = 512+4;
	COLOR(2,2) = 512+8;
	COLOR(2,3) = 512+12;
	COLOR(3,0) = 512;
	COLOR(3,1) = 512+1;
	COLOR(3,2) = 512+2;
	COLOR(3,3) = 512+3;
	COLOR(4,0) = 512;
	COLOR(4,1) = 512+16+1;
	COLOR(4,2) = 512+16+2;
	COLOR(4,3) = 512+16+3;
}



static void get_tile_info(int tile_index)
{
	UINT8 video = videoram[tile_index];
	UINT8 color = colorram[tile_index];

	int flag = 0;
	int code = 0;

	code = video;

	if (color & 0x80)
	{
		code |= 0x100;
	}

	if (tile_index / 32 <= 6)
	{
		flag |= TILE_IGNORE_TRANSPARENCY; /* lines 0 to 6 are opaqe? */
	}

	SET_TILE_INFO(0, code, color & 0x3f, flag)
}



VIDEO_START( mpatrol )
{
	bg_tilemap = tilemap_create(get_tile_info, tilemap_scan_rows, TILEMAP_TRANSPARENT, 8, 8, 32, 32);

	if (bg_tilemap == 0)
		return 1;

	tilemap_set_transparent_pen(bg_tilemap, 0);

	tilemap_set_scroll_rows(bg_tilemap, 16);

	return 0;
}



WRITE8_HANDLER( mpatrol_scroll_w )
{
	tilemap_set_scrollx(bg_tilemap, offset, -data);
}



WRITE8_HANDLER( mpatrol_videoram_w )
{
	if (data != videoram[offset])
	{
		tilemap_mark_tile_dirty(bg_tilemap, offset);
	}

	videoram[offset] = data;
}



WRITE8_HANDLER( mpatrol_colorram_w )
{
	if (data != colorram[offset])
	{
		tilemap_mark_tile_dirty(bg_tilemap, offset);
	}

	colorram[offset] = data;
}



WRITE8_HANDLER( mpatrol_bg1ypos_w )
{
	bg1ypos = data;
}
WRITE8_HANDLER( mpatrol_bg1xpos_w )
{
	bg1xpos = data;
}
WRITE8_HANDLER( mpatrol_bg2xpos_w )
{
	bg2xpos = data;
}
WRITE8_HANDLER( mpatrol_bg2ypos_w )
{
	bg2ypos = data;
}
WRITE8_HANDLER( mpatrol_bgcontrol_w )
{
	bgcontrol = data;
}



WRITE8_HANDLER( mpatrol_flipscreen_w )
{
	coin_counter_w(0, data & 0x02);
	coin_counter_w(1, data & 0x20);

	/* screen flip is handled both by software and hardware */

	flip_screen_set((data ^ ~readinputport(4)) & 1);
}



static void draw_background(mame_bitmap *bitmap, int xpos, int ypos, int image)
{
	rectangle rect;

	if (flip_screen)
	{
		xpos = 255 - xpos;
		ypos = 255 - ypos - BGHEIGHT;
	}

	drawgfx(bitmap, Machine->gfx[image],
		0, 0,
		flip_screen,
		flip_screen,
		xpos,
		ypos,
		&Machine->visible_area,
		TRANSPARENCY_PEN, 0);

	drawgfx(bitmap, Machine->gfx[image],
		0, 0,
		flip_screen,
		flip_screen,
		xpos - 256,
		ypos,
		&Machine->visible_area,
		TRANSPARENCY_PEN, 0);

	rect.min_x = Machine->visible_area.min_x;
	rect.max_x = Machine->visible_area.max_x;

	if (flip_screen)
	{
		rect.min_y = ypos - BGHEIGHT;
		rect.max_y = ypos - 1;
	}
	else
	{
		rect.min_y = ypos + BGHEIGHT;
		rect.max_y = ypos + 2 * BGHEIGHT - 1;
}

	fillbitmap(bitmap, Machine->gfx[image]->colortable[3], &rect);
}



VIDEO_UPDATE( mpatrol )
{
	int offs;

	fillbitmap(bitmap, 0, cliprect);

	if (!(bgcontrol & 0x20))
	{
		if (!(bgcontrol & 0x10))
		{
			draw_background(bitmap, bg2xpos, bg2ypos, 2); /* distant mountains */
		}
		if (!(bgcontrol & 0x02))
		{
			draw_background(bitmap, bg1xpos, bg1ypos, 3); /* hills */
		}
		if (!(bgcontrol & 0x04))
		{
			draw_background(bitmap, bg1xpos, bg1ypos, 4); /* cityscape */
		}
	}

	tilemap_set_flip(bg_tilemap, flip_screen ? TILEMAP_FLIPX | TILEMAP_FLIPY : 0);

	tilemap_draw(bitmap, cliprect, bg_tilemap, 0, 0);

	/* draw the sprites */

	for (offs = spriteram_size - 4; offs >= 0; offs -= 4)
	{
		int sx,sy,flipx,flipy;

		if (!(offs & 0x60))
		{
			continue; /* some addresses in sprite RAM are skipped? */
		}

		sx = spriteram[offs + 3];
		sy = 240 - spriteram[offs];
		flipx = spriteram[offs + 1] & 0x40;
		flipy = spriteram[offs + 1] & 0x80;

		if (flip_screen)
		{
			flipx = !flipx;
			flipy = !flipy;
			sx = 240 - sx;
			sy = 240 - sy;
		}

		sy++; /* odd */

		drawgfx(bitmap, Machine->gfx[1],
			spriteram[offs + 2],
			spriteram[offs + 1] & 0x3f,
			flipx, flipy,
			sx, sy,
			cliprect, TRANSPARENCY_COLOR, 512+32);
	}
}
