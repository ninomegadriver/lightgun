/******************************************************************************

    Gomoku Narabe Renju
    (c)1981 Nihon Bussan Co.,Ltd.

    Driver by Takahiro Nogi <nogi@kt.rim.or.jp> 1999/11/06 -
    Updated to compile again by David Haywood 19th Oct 2002

******************************************************************************/

#include "driver.h"


static int gomoku_flipscreen;
static int gomoku_bg_dispsw;
static tilemap *fg_tilemap;
static mame_bitmap *gomoku_bg_bitmap;

UINT8 *gomoku_videoram;
UINT8 *gomoku_colorram;
UINT8 *gomoku_bgram;
UINT8 *gomoku_bg_dirty;


/******************************************************************************

    palette RAM

******************************************************************************/

PALETTE_INIT( gomoku )
{
	int i;
	int bit0, bit1, bit2, r, g, b;

	for (i = 0; i < Machine->drv->total_colors; i++)
	{
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

		palette_set_color(i, r, g, b);
		color_prom++;
	}
}


/******************************************************************************

    Tilemap callbacks

******************************************************************************/

static void get_fg_tile_info(int tile_index)
{
	int code = (gomoku_videoram[tile_index]);
	int attr = (gomoku_colorram[tile_index]);
	int color = (attr& 0x0f);
	int flipyx = (attr & 0xc0) >> 6;

	SET_TILE_INFO(
			0,
			code,
			color,
			TILE_FLIPYX(flipyx))
}

WRITE8_HANDLER( gomoku_videoram_w )
{
	gomoku_videoram[offset] = data;
	tilemap_mark_tile_dirty(fg_tilemap,offset);
}

WRITE8_HANDLER( gomoku_colorram_w )
{
	gomoku_colorram[offset] = data;
	tilemap_mark_tile_dirty(fg_tilemap,offset);
}

WRITE8_HANDLER( gomoku_bgram_w )
{
	gomoku_bgram[offset] = data;
	gomoku_bg_dirty[offset] = 1;
}

WRITE8_HANDLER( gomoku_flipscreen_w )
{
	gomoku_flipscreen = (data & 0x02) ? 0 : 1;
}

WRITE8_HANDLER( gomoku_bg_dispsw_w )
{
	gomoku_bg_dispsw = (data & 0x02) ? 0 : 1;
}


/******************************************************************************

    Start the video hardware emulation

******************************************************************************/

VIDEO_START( gomoku )
{
	unsigned char *GOMOKU_BG_X = memory_region( REGION_USER1 );
	unsigned char *GOMOKU_BG_Y = memory_region( REGION_USER2 );
	unsigned char *GOMOKU_BG_D = memory_region( REGION_USER3 );
	int x, y;
	int bgdata;
	int color;

	gomoku_bg_bitmap = auto_bitmap_alloc(Machine->drv->screen_width, Machine->drv->screen_height);

	gomoku_bg_dirty = auto_malloc(0x100);

	fg_tilemap = tilemap_create(get_fg_tile_info,tilemap_scan_rows,TILEMAP_TRANSPARENT,8,8,32, 32);

	if ((!gomoku_bg_bitmap) || (!fg_tilemap))
	{
		return 1;
	}

	tilemap_set_transparent_pen(fg_tilemap,0);

	memset(gomoku_bg_dirty, 1, 0x100);

	/* make background bitmap */
	fillbitmap(gomoku_bg_bitmap, 0x20, 0);

	// board
	for (y = 0; y < 256; y++)
	{
		for (x = 0; x < 256; x++)
		{
			bgdata = GOMOKU_BG_D[ GOMOKU_BG_X[x] + (GOMOKU_BG_Y[y] << 4) ];

			color = 0x20;						// outside frame (black)

			if (bgdata & 0x01) color = 0x21;	// board (brown)
			if (bgdata & 0x02) color = 0x20;	// frame line (while)

			plot_pixel(gomoku_bg_bitmap, (255 - x + 7), (255 - y - 1), color);
		}
	}

	return 0;
}


/******************************************************************************

    Display refresh

******************************************************************************/

VIDEO_UPDATE( gomoku )
{
	unsigned char *GOMOKU_BG_X = memory_region( REGION_USER1 );
	unsigned char *GOMOKU_BG_Y = memory_region( REGION_USER2 );
	unsigned char *GOMOKU_BG_D = memory_region( REGION_USER3 );
	int x, y;
	int bgram;
	int bgoffs;
	int bgdata;
	int color;

	/* draw background layer */
	if (gomoku_bg_dispsw)
	{
		/* copy bg bitmap */
		copybitmap(bitmap, gomoku_bg_bitmap, 0, 0, 0, 0, cliprect, TRANSPARENCY_NONE, 0);

		// stone
		for (y = 0; y < 256; y++)
		{
			for (x = 0; x < 256; x++)
			{
				bgoffs = ((((255 - x - 2) / 14) | (((255 - y - 10) / 14) << 4)) & 0xff);

				bgdata = GOMOKU_BG_D[ GOMOKU_BG_X[x] + (GOMOKU_BG_Y[y] << 4) ];
				bgram = gomoku_bgram[bgoffs];

				if (bgdata & 0x04)
				{
					if (bgram & 0x01)
					{
						color = 0x2f;	// stone (black)
					}
					else if (bgram & 0x02)
					{
						color = 0x22;	// stone (white)
					}
					else continue;
				}
				else continue;

				plot_pixel(bitmap, (255 - x + 7), (255 - y - 1), color);
			}
		}

		// cursor
		for (y = 0; y < 256; y++)
		{
			for (x = 0; x < 256; x++)
			{
				bgoffs = ((((255 - x - 2) / 14) | (((255 - y - 10) / 14) << 4)) & 0xff);

				bgdata = GOMOKU_BG_D[ GOMOKU_BG_X[x] + (GOMOKU_BG_Y[y] << 4) ];
				bgram = gomoku_bgram[bgoffs];

				if (bgdata & 0x08)
				{
					if (bgram & 0x04)
					{
							color = 0x2f;	// cursor (black)
					}
					else if (bgram & 0x08)
					{
						color = 0x22;		// cursor (white)
					}
					else continue;
				}
				else continue;

				plot_pixel(bitmap, (255 - x + 7), (255 - y - 1), color);
			}
		}
	}
	else
	{
		fillbitmap(bitmap, 0x20, 0);
	}

	tilemap_draw(bitmap, cliprect, fg_tilemap, 0, 0);
}
