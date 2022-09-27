/***************************************************************************

  vidhrdw.c

  Functions to emulate the video hardware of the machine.

***************************************************************************/

#include "driver.h"

UINT8 *tecfri_videoram;
UINT8 *tecfri_colorram;
UINT8 *tecfri_videoram2;
UINT8 *tecfri_colorram2;

static tilemap *bg_tilemap, *fg_tilemap;

/* General */

WRITE8_HANDLER( tecfri_videoram_w )
{
	if (tecfri_videoram[offset] != data)
	{
		tecfri_videoram[offset] = data;
		tilemap_mark_tile_dirty(bg_tilemap, offset);
	}
}

WRITE8_HANDLER( tecfri_colorram_w )
{
	if (tecfri_colorram[offset] != data)
	{
		tecfri_colorram[offset] = data;
		tilemap_mark_tile_dirty(bg_tilemap, offset);
	}
}

WRITE8_HANDLER( tecfri_videoram2_w )
{
	if (tecfri_videoram2[offset] != data)
	{
		tecfri_videoram2[offset] = data;
		tilemap_mark_tile_dirty(fg_tilemap, offset);
	}
}

WRITE8_HANDLER( tecfri_colorram2_w )
{
	if (tecfri_colorram2[offset] != data)
	{
		tecfri_colorram2[offset] = data;
		tilemap_mark_tile_dirty(fg_tilemap, offset);
	}
}

WRITE8_HANDLER( tecfri_scroll_bg_w )
{
	tilemap_set_scrollx(bg_tilemap, 0, data);
}

WRITE8_HANDLER( flip_screen_w )
{
	flip_screen_set(data);
}

static void get_tile_info_bg(int tile_index)
{
	int code = tecfri_videoram[tile_index] + ((tecfri_colorram[tile_index] & 0x07) << 8);
	int color = (tecfri_colorram[tile_index] >> 4) & 0x0f;
	int flags = tecfri_colorram[tile_index] & 0x08 ? TILE_FLIPX : 0;

	SET_TILE_INFO(0, code, color, flags)
}

static void get_tile_info_fg(int tile_index)
{
	int code = tecfri_videoram2[tile_index] + ((tecfri_colorram2[tile_index] & 0x07) << 8);
	int color = (tecfri_colorram2[tile_index] >> 4) & 0x0f;
	int flags = tecfri_colorram2[tile_index] & 0x08 ? TILE_FLIPX : 0;

	SET_TILE_INFO(1, code, color, flags)
}

/* Sauro */

static int scroll2_map     [8] = {2, 1, 4, 3, 6, 5, 0, 7};
static int scroll2_map_flip[8] = {0, 7, 2, 1, 4, 3, 6, 5};

WRITE8_HANDLER( sauro_scroll_fg_w )
{
	int *map = (flip_screen ? scroll2_map_flip : scroll2_map);
	int scroll = (data & 0xf8) | map[data & 7];

	tilemap_set_scrollx(fg_tilemap, 0, scroll);
}

VIDEO_START( sauro )
{
	bg_tilemap = tilemap_create(get_tile_info_bg, tilemap_scan_cols,
		TILEMAP_OPAQUE, 8, 8, 32, 32);

	if (!bg_tilemap)
		return 1;

	fg_tilemap = tilemap_create(get_tile_info_fg, tilemap_scan_cols,
		TILEMAP_TRANSPARENT, 8, 8, 32, 32);

	if (!fg_tilemap)
		return 1;

	tilemap_set_transparent_pen(fg_tilemap, 0);

	return 0;
}

static void sauro_draw_sprites( mame_bitmap *bitmap )
{
	int offs,code,sx,sy,color,flipx;

	for (offs = 3;offs < spriteram_size - 1;offs += 4)
	{
		sy = spriteram[offs];
		if (sy == 0xf8) continue;

		code = spriteram[offs+1] + ((spriteram[offs+3] & 0x03) << 8);
		sx = spriteram[offs+2];
		sy = 236 - sy;
		color = (spriteram[offs+3] >> 4) & 0x0f;

		// I'm not really sure how this bit works
		if (spriteram[offs+3] & 0x08)
		{
			if (sx > 0xc0)
			{
				// Sign extend
				sx = (signed int)(signed char)sx;
			}
		}
		else
		{
			if (sx < 0x40) continue;
		}

		flipx = spriteram[offs+3] & 0x04;

		if (flip_screen)
		{
			flipx = !flipx;
			sx = (235 - sx) & 0xff;  // The &0xff is not 100% percent correct
			sy = 240 - sy;
		}

		drawgfx(bitmap, Machine->gfx[2],
				code,
				color,
				flipx,flip_screen,
				sx,sy,
				&Machine->visible_area,TRANSPARENCY_PEN,0);
	}
}

VIDEO_UPDATE( sauro )
{
	tilemap_draw(bitmap, &Machine->visible_area, bg_tilemap, 0, 0);
	tilemap_draw(bitmap, &Machine->visible_area, fg_tilemap, 0, 0);
	sauro_draw_sprites(bitmap);
}

/* Tricky Doc */

WRITE8_HANDLER ( trckydoc_spriteram_mirror_w )
{
	spriteram[offset] = data;
}

VIDEO_START( trckydoc )
{
	bg_tilemap = tilemap_create(get_tile_info_bg, tilemap_scan_cols,
		TILEMAP_OPAQUE, 8, 8, 32, 32);

	if (!bg_tilemap)
		return 1;

	return 0;
}

static void trckydoc_draw_sprites( mame_bitmap *bitmap )
{
	int offs,code,sy,color,flipx,sx;

	/* Weird, sprites entries don't start on DWORD boundary */
	for (offs = 3;offs < spriteram_size - 1;offs += 4)
	{
		sy = spriteram[offs];

		if(spriteram[offs+3] & 0x08)
		{
			/* needed by the elevator cable (2nd stage), balls bouncing (3rd stage) and maybe other things */
			sy += 6;
		}

		code = spriteram[offs+1] + ((spriteram[offs+3] & 0x01) << 8);

		sx = spriteram[offs+2]-2;
		color = (spriteram[offs+3] >> 4) & 0x0f;

		sy = 236 - sy;

		/* similar to sauro but different bit is used .. */
		if (spriteram[offs+3] & 0x02)
		{
			if (sx > 0xc0)
			{
				/* Sign extend */
				sx = (signed int)(signed char)sx;
			}
		}
		else
		{
			if (sx < 0x40) continue;
		}

		flipx = spriteram[offs+3] & 0x04;

		if (flip_screen)
		{
			flipx = !flipx;
			sx = (235 - sx) & 0xff;  /* The &0xff is not 100% percent correct */
			sy = 240 - sy;
		}

		drawgfx(bitmap, Machine->gfx[1],
				code,
				color,
				flipx,flip_screen,
				sx,sy,
				&Machine->visible_area,TRANSPARENCY_PEN,0);
	}
}

VIDEO_UPDATE( trckydoc )
{
	tilemap_draw(bitmap, &Machine->visible_area, bg_tilemap, 0, 0);
	trckydoc_draw_sprites(bitmap);
}
