#include "driver.h"

static int bgcharbank;
static tilemap *bg_tilemap, *fg_tilemap;

WRITE16_HANDLER( tigeroad_videoram_w )
{
	if (videoram16[offset] != data)
	{
		COMBINE_DATA(&videoram16[offset]);
		tilemap_mark_tile_dirty(fg_tilemap, offset);
	}
}

WRITE16_HANDLER( tigeroad_videoctrl_w )
{
	int bank;

	if (ACCESSING_MSB)
	{
		data = (data >> 8) & 0xff;

		/* bit 1 flips screen */

		if (flip_screen != (data & 0x02))
		{
			flip_screen_set(data & 0x02);
			tilemap_mark_all_tiles_dirty(ALL_TILEMAPS);
		}

		/* bit 2 selects bg char bank */

		bank = (data & 0x04) >> 2;

		if (bgcharbank != bank)
		{
			bgcharbank = bank;
			tilemap_mark_all_tiles_dirty(bg_tilemap);
		}

		/* bits 4-5 are coin lockouts */

		coin_lockout_w(0, !(data & 0x10));
		coin_lockout_w(1, !(data & 0x20));

		/* bits 6-7 are coin counters */

		coin_counter_w(0, data & 0x40);
		coin_counter_w(1, data & 0x80);
	}
}

WRITE16_HANDLER( tigeroad_scroll_w )
{
	int scroll = 0;

	COMBINE_DATA(&scroll);

	switch (offset)
	{
	case 0:
		tilemap_set_scrollx(bg_tilemap, 0, scroll);
		break;
	case 1:
		tilemap_set_scrolly(bg_tilemap, 0, -scroll - 32 * 8);
		break;
	}
}

static void tigeroad_draw_sprites( mame_bitmap *bitmap, int priority )
{
	UINT16 *source = &buffered_spriteram16[spriteram_size/2] - 4;
	UINT16 *finish = buffered_spriteram16;

	// TODO: The Track Map should probably be drawn on top of the background tilemap...
	//       Also convert the below into a for loop!

	while (source >= finish)
	{
		int tile_number = source[0];

		if (tile_number != 0xfff) {
			int attr = source[1];
			int sy = source[2] & 0x1ff;
			int sx = source[3] & 0x1ff;

			int flipx = attr & 0x02;
			int flipy = attr & 0x01;
			int color = (attr >> 2) & 0x0f;

			if (sx > 0x100) sx -= 0x200;
			if (sy > 0x100) sy -= 0x200;

			if (flip_screen)
			{
				sx = 240 - sx;
				sy = 240 - sy;
				flipx = !flipx;
				flipy = !flipy;
			}

			drawgfx(bitmap, Machine->gfx[2],
				tile_number,
				color,
				flipx, flipy,
				sx, 240 - sy,
				&Machine->visible_area,
				TRANSPARENCY_PEN, 15);
		}

		source -= 4;
	}
}

static void get_bg_tile_info(int tile_index)
{
	UINT8 *tilerom = memory_region(REGION_GFX4);

	int data = tilerom[tile_index];
	int attr = tilerom[tile_index + 1];
	int code = data + ((attr & 0xc0) << 2) + (bgcharbank << 10);
	int color = attr & 0x0f;
	int flags = ((attr & 0x20) ? TILE_FLIPX : 0) | ((attr & 0x10) ? TILE_SPLIT(1) : 0);

	SET_TILE_INFO(1, code, color, flags)
}

static void get_fg_tile_info(int tile_index)
{
	int data = videoram16[tile_index];
	int attr = data >> 8;
	int code = (data & 0xff) + ((attr & 0xc0) << 2) + ((attr & 0x20) << 5);
	int color = attr & 0x0f;
	int flags = (attr & 0x10) ? TILE_FLIPY : 0;

	SET_TILE_INFO(0, code, color, flags)
}

static UINT32 tigeroad_tilemap_scan( UINT32 col, UINT32 row, UINT32 num_cols, UINT32 num_rows )
{
	/* logical (col,row) -> memory offset */
	return 2 * (col % 8) + 16 * ((127 - row) % 8) + 128 * (col / 8) + 2048 * ((127 - row) / 8);
}

VIDEO_START( tigeroad )
{
	bg_tilemap = tilemap_create(get_bg_tile_info, tigeroad_tilemap_scan,
		TILEMAP_SPLIT, 32, 32, 128, 128);

	if ( !bg_tilemap )
		return 1;

	fg_tilemap = tilemap_create(get_fg_tile_info, tilemap_scan_rows,
		TILEMAP_TRANSPARENT, 8, 8, 32, 32);

	if ( !fg_tilemap )
		return 1;

	tilemap_set_transmask(bg_tilemap, 0, 0xffff, 0);
	tilemap_set_transmask(bg_tilemap, 1, 0x1ff, 0xfe00);

	tilemap_set_transparent_pen(fg_tilemap, 3);

	return 0;
}

VIDEO_UPDATE( tigeroad )
{
	tilemap_draw(bitmap, &Machine->visible_area, bg_tilemap, TILEMAP_BACK, 0);
	tigeroad_draw_sprites(bitmap, 0);
	tilemap_draw(bitmap, &Machine->visible_area, bg_tilemap, TILEMAP_FRONT, 1);
	//tigeroad_draw_sprites(bitmap, 1); draw priority sprites?
	tilemap_draw(bitmap, &Machine->visible_area, fg_tilemap, 0, 2);
}

VIDEO_EOF( tigeroad )
{
	buffer_spriteram16_w(0,0,0);
}
