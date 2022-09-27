/* Free Kick Video Hardware */

#include "driver.h"

tilemap *freek_tilemap;
UINT8 *freek_videoram;


static void get_freek_tile_info(int tile_index)
{
	int tileno,palno;

	tileno = freek_videoram[tile_index]+((freek_videoram[tile_index+0x400]&0xe0)<<3);
	palno=freek_videoram[tile_index+0x400]&0x1f;
	SET_TILE_INFO(0,tileno,palno,0)
}



VIDEO_START(freekick)
{
	freek_tilemap = tilemap_create(get_freek_tile_info,tilemap_scan_rows,TILEMAP_OPAQUE, 8, 8,32,32);
	return 0;
}



WRITE8_HANDLER( freek_videoram_w )
{
	freek_videoram[offset] = data;
	tilemap_mark_tile_dirty(freek_tilemap,offset&0x3ff);
}

static void gigas_draw_sprites( mame_bitmap *bitmap, const rectangle *cliprect )
{
	int offs;

	for (offs = 0;offs < spriteram_size;offs += 4)
	{
		int xpos = spriteram[offs + 3];
		int ypos = spriteram[offs + 2];
		int code = spriteram[offs + 0]|( (spriteram[offs + 1]&0x20) <<3 );

		int flipx = 0;
		int flipy = 0;
		int color = spriteram[offs + 1] & 0x1f;

		if (flip_screen_x)
		{
			xpos = 240 - xpos;
			flipx = !flipx;
		}
		if (flip_screen_y)
		{
			ypos = 256 - ypos;
			flipy = !flipy;
		}

		drawgfx(bitmap,Machine->gfx[1],
				code,
				color,
				flipx,flipy,
				xpos,240-ypos,
				cliprect,TRANSPARENCY_PEN,0);
	}
}


static void pbillrd_draw_sprites( mame_bitmap *bitmap, const rectangle *cliprect )
{
	int offs;

	for (offs = 0;offs < spriteram_size;offs += 4)
	{
		int xpos = spriteram[offs + 3];
		int ypos = spriteram[offs + 2];
		int code = spriteram[offs + 0];

		int flipx = 0;//spriteram[offs + 0] & 0x80; //?? unused ?
		int flipy = 0;//spriteram[offs + 0] & 0x40;
		int color = spriteram[offs + 1] & 0x0f;

		if (flip_screen_x)
		{
			xpos = 240 - xpos;
			flipx = !flipx;
		}
		if (flip_screen_y)
		{
			ypos = 256 - ypos;
			flipy = !flipy;
		}

		drawgfx(bitmap,Machine->gfx[1],
				code,
				color,
				flipx,flipy,
				xpos,240-ypos,
				cliprect,TRANSPARENCY_PEN,0);
	}
}



static void freekick_draw_sprites( mame_bitmap *bitmap, const rectangle *cliprect )
{
	int offs;

	for (offs = 0;offs < spriteram_size;offs += 4)
	{
		int xpos = spriteram[offs + 3];
		int ypos = spriteram[offs + 0];
		int code = spriteram[offs + 1]+ ((spriteram[offs + 2] & 0x20) << 3);

		int flipx = spriteram[offs + 2] & 0x80;	//?? unused ?
		int flipy = spriteram[offs + 2] & 0x40;
		int color = spriteram[offs + 2] & 0x1f;

		if (flip_screen_x)
		{
			xpos = 240 - xpos;
			flipx = !flipx;
		}
		if (flip_screen_y)
		{
			ypos = 256 - ypos;
			flipy = !flipy;
		}

		drawgfx(bitmap,Machine->gfx[1],
				code,
				color,
				flipx,flipy,
				xpos,248-ypos,
				cliprect,TRANSPARENCY_PEN,0);
	}
}

VIDEO_UPDATE(gigas)
{
	tilemap_draw(bitmap,cliprect,freek_tilemap,0,0);
	gigas_draw_sprites(bitmap,cliprect);
}

VIDEO_UPDATE(pbillrd)
{
	tilemap_draw(bitmap,cliprect,freek_tilemap,0,0);
	pbillrd_draw_sprites(bitmap,cliprect);
}

VIDEO_UPDATE(freekick)
{
	tilemap_draw(bitmap,cliprect,freek_tilemap,0,0);
	freekick_draw_sprites(bitmap,cliprect);
}
