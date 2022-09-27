#include "driver.h"

UINT8 *blueprnt_scrollram;
static int gfx_bank;
static tilemap *bg_tilemap;

/***************************************************************************

  Convert the color PROMs into a more useable format.

  Blue Print doesn't have color PROMs. For sprites, the ROM data is directly
  converted into colors; for characters, it is converted through the color
  code (bits 0-2 = RBG for 01 pixels, bits 3-5 = RBG for 10 pixels, 00 pixels
  always black, 11 pixels use the OR of bits 0-2 and 3-5. Bit 6 is intensity
  control)

***************************************************************************/

PALETTE_INIT( blueprnt )
{
	int i;
	#define TOTAL_COLORS(gfxn) (Machine->gfx[gfxn]->total_colors * Machine->gfx[gfxn]->color_granularity)
	#define COLOR(gfxn,offs) (colortable[Machine->drv->gfxdecodeinfo[gfxn].color_codes_start + offs])

	for (i = 0;i < 16;i++)
	{
		int r = ((i >> 0) & 1) * ((i & 0x08) ? 0xbf : 0xff);
		int g = ((i >> 2) & 1) * ((i & 0x08) ? 0xbf : 0xff);
		int b = ((i >> 1) & 1) * ((i & 0x08) ? 0xbf : 0xff);
		palette_set_color(i,r,g,b);
	}

	/* chars */
	for (i = 0;i < 128;i++)
	{
		int base =  (i & 0x40) ? 8 : 0;
		COLOR(0,4*i+0) = base + 0;
		COLOR(0,4*i+1) = base + ((i >> 0) & 7);
		COLOR(0,4*i+2) = base + ((i >> 3) & 7);
		COLOR(0,4*i+3) = base + (((i >> 0) & 7) | ((i >> 3) & 7));
	}

	/* sprites */
	for (i = 0;i < 8;i++)
		COLOR(1,i) = i;
}

WRITE8_HANDLER( blueprnt_videoram_w )
{
	if (videoram[offset] != data)
	{
		videoram[offset] = data;
		tilemap_mark_tile_dirty(bg_tilemap, offset);
	}
}

WRITE8_HANDLER( blueprnt_colorram_w )
{
	if (colorram[offset] != data)
	{
		colorram[offset] = data;
		tilemap_mark_tile_dirty(bg_tilemap, offset);
	}
}

WRITE8_HANDLER( blueprnt_flipscreen_w )
{
	flip_screen_set(~data & 0x02);

	if (gfx_bank != ((data & 0x04) >> 2))
	{
		gfx_bank = ((data & 0x04) >> 2);
		tilemap_mark_all_tiles_dirty(ALL_TILEMAPS);
	}
}

static void get_bg_tile_info(int tile_index)
{
	int attr = colorram[tile_index];
	int code = videoram[tile_index] + 256 * gfx_bank;
	int color = attr & 0x7f;

	tile_info.priority = (attr & 0x80) ? 1 : 0;

	SET_TILE_INFO(0, code, color, 0)
}

VIDEO_START( blueprnt )
{
	bg_tilemap = tilemap_create(get_bg_tile_info, tilemap_scan_cols_flip_x,
		TILEMAP_TRANSPARENT, 8, 8, 32, 32);

	if ( !bg_tilemap )
		return 1;

	tilemap_set_transparent_pen(bg_tilemap, 0);
	tilemap_set_scroll_cols(bg_tilemap, 32);

	return 0;
}

static void blueprnt_draw_sprites( mame_bitmap *bitmap, const rectangle *cliprect )
{
	int offs;

	for (offs = 0; offs < spriteram_size; offs += 4)
	{
		int code = spriteram[offs + 1];
		int sx = spriteram[offs + 3];
		int sy = 240 - spriteram[offs];
		int flipx = spriteram[offs + 2] & 0x40;
		int flipy = spriteram[offs + 2 - 4] & 0x80;	// -4? Awkward, isn't it?

		if (flip_screen)
		{
			sx = 248 - sx;
			sy = 240 - sy;
			flipx = !flipx;
			flipy = !flipy;
		}

		// sprites are slightly misplaced, regardless of the screen flip
		drawgfx(bitmap, Machine->gfx[1], code, 0, flipx, flipy, 2+sx, sy-1,
			cliprect, TRANSPARENCY_PEN, 0);
	}
}

VIDEO_UPDATE( blueprnt )
{
	int i;

	if (flip_screen)
		for (i = 0; i < 32; i++)
			tilemap_set_scrolly(bg_tilemap, i, blueprnt_scrollram[32 - i]);
	else
		for (i = 0; i < 32; i++)
			tilemap_set_scrolly(bg_tilemap, i, blueprnt_scrollram[30 - i]);

	fillbitmap(bitmap, get_black_pen(), cliprect);
	tilemap_draw(bitmap, cliprect, bg_tilemap, 0, 0);
	blueprnt_draw_sprites(bitmap, cliprect);
	tilemap_draw(bitmap, cliprect, bg_tilemap, 1, 0);
}
