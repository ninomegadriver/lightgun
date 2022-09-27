/***************************************************************************

  vidhrdw.c

  Written by Kenneth Lin (kenneth_lin@ai.vancouver.bc.ca)

  Functions to emulate the video hardware of the machine.

***************************************************************************/

#include "driver.h"

UINT8 *jackal_scrollram, *jackal_videoctrl;

static tilemap *bg_tilemap;


PALETTE_INIT( jackal )
{
	int i;
	#define TOTAL_COLORS(gfxn) (Machine->gfx[gfxn]->total_colors * Machine->gfx[gfxn]->color_granularity)
	#define COLOR(gfxn,offs) (colortable[Machine->drv->gfxdecodeinfo[gfxn].color_codes_start + offs])

	for (i = 0;i < TOTAL_COLORS(0);i++)
	{
		COLOR(0,i) = (i & 0xff) + 256;
		/* this is surely wrong - is there a PROM missing? */
		if (i & 0x0f)
			COLOR(0,i) |= i/256;
	}

	for (i = 0;i < TOTAL_COLORS(1);i++)
	{
		COLOR(1,i) = (*color_prom & 0x0f);
		color_prom++;
	}

	for (i = 0;i < TOTAL_COLORS(3);i++)
	{
		COLOR(3,i) = (*color_prom & 0x0f) + 16;
		color_prom++;
	}
}

void jackal_mark_tile_dirty(int offset)
{
	tilemap_mark_tile_dirty(bg_tilemap, offset);
}

static void get_bg_tile_info(int tile_index)
{
	UINT8 *RAM = memory_region(REGION_CPU1);

	int attr = RAM[0x2000 + tile_index];
	int code = RAM[0x2400 + tile_index] + ((attr & 0xc0) << 2) + ((attr & 0x30) << 6);
	int color = attr & 0x0f;
	int flags = ((attr & 0x10) ? TILE_FLIPX : 0) | ((attr & 0x20) ? TILE_FLIPY : 0);

	SET_TILE_INFO(0, code, color, flags)
}

VIDEO_START( jackal )
{
	bg_tilemap = tilemap_create(get_bg_tile_info, tilemap_scan_rows,
		TILEMAP_OPAQUE, 8, 8, 32, 32);

	if ( !bg_tilemap )
		return 1;

	return 0;
}

static void jackal_draw_background( mame_bitmap *bitmap, const rectangle *cliprect )
{
	UINT8 *RAM = memory_region(REGION_CPU1);
	int i;

	jackal_scrollram = &RAM[0x0020];

	tilemap_set_scroll_rows(bg_tilemap, 1);
	tilemap_set_scroll_cols(bg_tilemap, 1);

	tilemap_set_scrolly(bg_tilemap, 0, jackal_videoctrl[0]);
	tilemap_set_scrollx(bg_tilemap, 0, jackal_videoctrl[1]);

	if (jackal_videoctrl[2] & 0x02)
	{
		if (jackal_videoctrl[2] & 0x08)
		{
			tilemap_set_scroll_rows(bg_tilemap, 32);

			for (i = 0; i < 32; i++)
			{
				tilemap_set_scrollx(bg_tilemap, i, jackal_scrollram[i]);
			}
		}

		if (jackal_videoctrl[2] & 0x04)
		{
			tilemap_set_scroll_cols(bg_tilemap, 32);

			for (i = 0; i < 32; i++)
			{
				tilemap_set_scrolly(bg_tilemap, i, jackal_scrollram[i]);
			}
		}
	}

	tilemap_draw(bitmap, cliprect, bg_tilemap, 0, 0);
}

#define DRAW_SPRITE(bank, code, sx, sy) drawgfx(bitmap, Machine->gfx[bank], code, color, flipx, flipy, sx, sy, cliprect, TRANSPARENCY_PEN, 0);

static void jackal_draw_sprites_region( mame_bitmap *bitmap, const rectangle *cliprect, const UINT8 *sram, int length, int bank )
{
	int offs;

	for (offs = 0; offs < length; offs += 5)
	{
		int sn1 = sram[offs];
		int sn2 = sram[offs+1];
		int sy  = sram[offs+2];
		int sx  = sram[offs+3];
		int attr = sram[offs+4];
		int flipx = attr & 0x20;
		int flipy = attr & 0x40;
		int color = ((sn2 & 0xf0) >> 4);

		if (attr & 0x01) sx = sx - 256;
		if (sy > 0xf0)   sy = sy - 256;

		if (flip_screen)
		{
			sx = 240 - sx;
			sy = 240 - sy;
			flipx = !flipx;
			flipy = !flipy;
		}

		if (attr & 0xC)    // half-size sprite
		{
			int spritenum = sn1*4 + ((sn2 & (8+4)) >> 2) + ((sn2 & (2+1)) << 10);
			int mod = -8;

			if (flip_screen)
			{
				sx += 8;
				sy -= 8;
				mod = 8;
			}

			if ((attr & 0x0C) == 0x0C)
			{
				if (flip_screen) sy += 16;
				DRAW_SPRITE(bank + 1, spritenum, sx, sy)
			}

			if ((attr & 0x0C) == 0x08)
			{
				sy += 8;
				DRAW_SPRITE(bank + 1, spritenum,     sx, sy)
				DRAW_SPRITE(bank + 1, spritenum - 2, sx, sy + mod)
			}

			if ((attr & 0x0C) == 0x04)
			{
				DRAW_SPRITE(bank + 1, spritenum,     sx,       sy)
				DRAW_SPRITE(bank + 1, spritenum + 1, sx + mod, sy)
			}
		}
		else
		{
			int spritenum = sn1 + ((sn2 & 0x03) << 8);

			if (attr & 0x10)
			{
				if (flip_screen)
				{
					sx -= 16;
					sy -= 16;
				}

				DRAW_SPRITE(bank, spritenum,     flipx ? sx+16 : sx, flipy ? sy+16 : sy)
				DRAW_SPRITE(bank, spritenum + 1, flipx ? sx : sx+16, flipy ? sy+16 : sy)
				DRAW_SPRITE(bank, spritenum + 2, flipx ? sx+16 : sx, flipy ? sy : sy+16)
				DRAW_SPRITE(bank, spritenum + 3, flipx ? sx : sx+16, flipy ? sy : sy+16)
			}
			else
			{
				DRAW_SPRITE(bank, spritenum, sx, sy)
			}
		}
	}
}

static void jackal_draw_sprites( mame_bitmap *bitmap, const rectangle *cliprect )
{
	UINT8 *RAM = memory_region(REGION_CPU1);
	UINT8 *sr, *ss;

	if (jackal_videoctrl[0x03] & 0x08)
	{
		sr = &RAM[0x03800];	// Sprite 2
		ss = &RAM[0x13800];	// Additional Sprite 2
	}
	else
	{
		sr = &RAM[0x03000];	// Sprite 1
		ss = &RAM[0x13000];	// Additional Sprite 1
	}

	jackal_draw_sprites_region(bitmap, cliprect, ss, 0x0f5, 3);
	jackal_draw_sprites_region(bitmap, cliprect, sr, 0x500, 1);
}

VIDEO_UPDATE( jackal )
{
	jackal_draw_background(bitmap, cliprect);
	jackal_draw_sprites(bitmap, cliprect);
}
