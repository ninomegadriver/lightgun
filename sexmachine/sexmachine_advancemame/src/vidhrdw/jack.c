/***************************************************************************

  vidhrdw/jack.c

  Functions to emulate the video hardware of the machine.

***************************************************************************/

#include "driver.h"

static tilemap *bg_tilemap;

WRITE8_HANDLER( jack_videoram_w )
{
	if (videoram[offset] != data)
	{
		videoram[offset] = data;
		tilemap_mark_tile_dirty(bg_tilemap, offset);
	}
}

WRITE8_HANDLER( jack_colorram_w )
{
	if (colorram[offset] != data)
	{
		colorram[offset] = data;
		tilemap_mark_tile_dirty(bg_tilemap, offset);
	}
}

WRITE8_HANDLER( jack_paletteram_w )
{
	/* RGB output is inverted */
	paletteram_BBGGGRRR_w(offset,~data);
}

READ8_HANDLER( jack_flipscreen_r )
{
	flip_screen_set(offset);
	return 0;
}

WRITE8_HANDLER( jack_flipscreen_w )
{
	flip_screen_set(offset);
}

static void get_bg_tile_info(int tile_index)
{
	int code = videoram[tile_index] + ((colorram[tile_index] & 0x18) << 5);
	int color = colorram[tile_index] & 0x07;

	SET_TILE_INFO(0, code, color, 0)
}

static UINT32 tilemap_scan_cols_flipy( UINT32 col, UINT32 row, UINT32 num_cols, UINT32 num_rows )
{
	/* logical (col,row) -> memory offset */
	return (col * num_rows) + (num_rows - 1 - row);
}

VIDEO_START( jack )
{
	bg_tilemap = tilemap_create(get_bg_tile_info, tilemap_scan_cols_flipy, TILEMAP_OPAQUE, 8, 8, 32, 32);

	if ( !bg_tilemap )
		return 1;

	return 0;
}

static void jack_draw_sprites( mame_bitmap *bitmap )
{
	int offs;

	for (offs = spriteram_size - 4;offs >= 0;offs -= 4)
	{
		int sx,sy,num, color,flipx,flipy;

		sx    = spriteram[offs + 1];
		sy    = spriteram[offs];
		num   = spriteram[offs + 2] + ((spriteram[offs + 3] & 0x08) << 5);
		color = spriteram[offs + 3] & 0x07;
		flipx = (spriteram[offs + 3] & 0x80);
		flipy = (spriteram[offs + 3] & 0x40);

		if (flip_screen)
		{
			sx = 248 - sx;
			sy = 248 - sy;
			flipx = !flipx;
			flipy = !flipy;
		}

		drawgfx(bitmap,Machine->gfx[0],
				num,
				color,
				flipx,flipy,
				sx,sy,
				&Machine->visible_area,TRANSPARENCY_PEN,0);
	}
}

VIDEO_UPDATE( jack )
{
	tilemap_draw(bitmap,cliprect,bg_tilemap,0,0);
	jack_draw_sprites(bitmap);
}

/*
   Joinem has a bit different video hardware with proms based palette,
   3bpp gfx and different banking / colors bits
*/

PALETTE_INIT( joinem )
{
	int i;

	for (i = 0;i < Machine->drv->total_colors;i++)
	{
		int bit0,bit1,bit2,r,g,b;
		bit0 = (color_prom[i] >> 0) & 0x01;
		bit1 = (color_prom[i] >> 1) & 0x01;
		bit2 = (color_prom[i] >> 2) & 0x01;
		r = 0x21 * bit0 + 0x47 * bit1 + 0x97 * bit2;
		bit0 = (color_prom[i] >> 3) & 0x01;
		bit1 = (color_prom[i] >> 4) & 0x01;
		bit2 = (color_prom[i] >> 5) & 0x01;
		g = 0x21 * bit0 + 0x47 * bit1 + 0x97 * bit2;
		bit0 = 0;
		bit1 = (color_prom[i] >> 6) & 0x01;
		bit2 = (color_prom[i] >> 7) & 0x01;
		b = 0x21 * bit0 + 0x47 * bit1 + 0x97 * bit2;

		palette_set_color(i,r,g,b);
	}
}

static void joinem_get_bg_tile_info(int tile_index)
{
	int code = videoram[tile_index] + ((colorram[tile_index] & 0x01) << 8);
	int color = (colorram[tile_index] & 0x38) >> 3;

	SET_TILE_INFO(0, code, color, 0)
}

VIDEO_START( joinem )
{
	bg_tilemap = tilemap_create(joinem_get_bg_tile_info, tilemap_scan_cols_flipy, TILEMAP_OPAQUE, 8, 8, 32, 32);

	if ( !bg_tilemap )
		return 1;

	return 0;
}

static void joinem_draw_sprites( mame_bitmap *bitmap )
{
	int offs;

	for (offs = spriteram_size - 4;offs >= 0;offs -= 4)
	{
		int sx,sy,num, color,flipx,flipy;

		sx    = spriteram[offs + 1];
		sy    = spriteram[offs];
		num   = spriteram[offs + 2] + ((spriteram[offs + 3] & 0x01) << 8);
		color = (spriteram[offs + 3] & 0x38) >> 3;
		flipx = (spriteram[offs + 3] & 0x80);
		flipy = (spriteram[offs + 3] & 0x40);

		if (flip_screen)
		{
			sx = 248 - sx;
			sy = 248 - sy;
			flipx = !flipx;
			flipy = !flipy;
		}

		drawgfx(bitmap,Machine->gfx[0],
				num,
				color,
				flipx,flipy,
				sx,sy,
				&Machine->visible_area,TRANSPARENCY_PEN,0);
	}
}

VIDEO_UPDATE( joinem )
{
	tilemap_draw(bitmap,cliprect,bg_tilemap,0,0);
	joinem_draw_sprites(bitmap);
}
