/***************************************************************************

  vidhrdw.c

  Functions to emulate the video hardware of the machine.

***************************************************************************/

#include "driver.h"

UINT8 *commando_videoram2, *commando_colorram2;

static tilemap *bg_tilemap, *fg_tilemap;

WRITE8_HANDLER( commando_videoram_w )
{
	if (videoram[offset] != data)
	{
		videoram[offset] = data;
		tilemap_mark_tile_dirty(bg_tilemap, offset);
	}
}

WRITE8_HANDLER( commando_colorram_w )
{
	if (colorram[offset] != data)
	{
		colorram[offset] = data;
		tilemap_mark_tile_dirty(bg_tilemap, offset);
	}
}

WRITE8_HANDLER( commando_videoram2_w )
{
	if (commando_videoram2[offset] != data)
	{
		commando_videoram2[offset] = data;
		tilemap_mark_tile_dirty(fg_tilemap, offset);
	}
}

WRITE8_HANDLER( commando_colorram2_w )
{
	if (commando_colorram2[offset] != data)
	{
		commando_colorram2[offset] = data;
		tilemap_mark_tile_dirty(fg_tilemap, offset);
	}
}

WRITE8_HANDLER( commando_scrollx_w )
{
	static UINT8 scroll[2];

	scroll[offset] = data;
	tilemap_set_scrollx(bg_tilemap,0,scroll[0] | (scroll[1] << 8));
}

WRITE8_HANDLER( commando_scrolly_w )
{
	static UINT8 scroll[2];

	scroll[offset] = data;
	tilemap_set_scrolly(bg_tilemap,0,scroll[0] | (scroll[1] << 8));
}

WRITE8_HANDLER( commando_c804_w )
{
	// bits 0 and 1 are coin counters
	coin_counter_w(0, data & 0x01);
	coin_counter_w(1, data & 0x02);

	// bit 4 resets the sound CPU
	cpunum_set_input_line(1, INPUT_LINE_RESET, (data & 0x10) ? ASSERT_LINE : CLEAR_LINE);

	// bit 7 flips screen
	flip_screen_set(data & 0x80);
}

static void get_bg_tile_info(int tile_index)
{
	int attr = colorram[tile_index];
	int code = videoram[tile_index] + ((attr & 0xc0) << 2);
	int color = attr & 0x0f;
	int flags = TILE_FLIPYX((attr & 0x30) >> 4);

	SET_TILE_INFO(1, code, color, flags)
}

static void get_fg_tile_info(int tile_index)
{
	int attr = commando_colorram2[tile_index];
	int code = commando_videoram2[tile_index] + ((attr & 0xc0) << 2);
	int color = attr & 0x0f;
	int flags = TILE_FLIPYX((attr & 0x30) >> 4);

	SET_TILE_INFO(0, code, color, flags)
}

VIDEO_START( commando )
{
	bg_tilemap = tilemap_create(get_bg_tile_info, tilemap_scan_cols,
		TILEMAP_OPAQUE, 16, 16, 32, 32);

	if ( !bg_tilemap )
		return 1;

	fg_tilemap = tilemap_create(get_fg_tile_info, tilemap_scan_rows,
		TILEMAP_TRANSPARENT, 8, 8, 32, 32);

	if ( !fg_tilemap )
		return 1;

	tilemap_set_transparent_pen(fg_tilemap, 3);

	return 0;
}

static void commando_draw_sprites( mame_bitmap *bitmap, const rectangle *cliprect )
{
	int offs;

	for (offs = spriteram_size - 4; offs >= 0; offs -= 4)
	{
		// bit 1 of attr is not used
		int attr = buffered_spriteram[offs + 1];
		int bank = (attr & 0xc0) >> 6;
		int code = buffered_spriteram[offs] + 256 * bank;
		int color = (attr & 0x30) >> 4;
		int flipx = attr & 0x04;
		int flipy = attr & 0x08;
		int sx = buffered_spriteram[offs + 3] - ((attr & 0x01) << 8);
		int sy = buffered_spriteram[offs + 2];

		if (flip_screen)
		{
			sx = 240 - sx;
			sy = 240 - sy;
			flipx = !flipx;
			flipy = !flipy;
		}

		if (bank < 3)
			drawgfx(bitmap,Machine->gfx[2], code, color, flipx, flipy, sx, sy,
				cliprect, TRANSPARENCY_PEN, 15);
	}
}

VIDEO_UPDATE( commando )
{
	tilemap_draw(bitmap, cliprect, bg_tilemap, 0, 0);
	commando_draw_sprites(bitmap, cliprect);
	tilemap_draw(bitmap, cliprect, fg_tilemap, 0, 0);
}

VIDEO_EOF( commando )
{
	buffer_spriteram_w(0, 0);
}
