/***************************************************************************

  vidhrdw.c

  Functions to emulate the video hardware of the machine.

***************************************************************************/

#include "driver.h"

static tilemap *bg_tilemap;

PALETTE_INIT( ponttehk )
{
	int i;

	for ( i = 0; i < Machine->drv->total_colors; i++ )
	{
		int bit0,bit1,bit2,bit3,r,g,b;

		/* red component */
		bit0 = (color_prom[0] >> 0) & 0x01;
		bit1 = (color_prom[0] >> 1) & 0x01;
		bit2 = (color_prom[0] >> 2) & 0x01;
		bit3 = (color_prom[0] >> 3) & 0x01;
		r = 0x0e * bit0 + 0x1f * bit1 + 0x43 * bit2 + 0x8f * bit3;

		/* green component */
		bit0 = (color_prom[Machine->drv->total_colors] >> 0) & 0x01;
		bit1 = (color_prom[Machine->drv->total_colors] >> 1) & 0x01;
		bit2 = (color_prom[Machine->drv->total_colors] >> 2) & 0x01;
		bit3 = (color_prom[Machine->drv->total_colors] >> 3) & 0x01;
		g = 0x0e * bit0 + 0x1f * bit1 + 0x43 * bit2 + 0x8f * bit3;

		/* blue component */
		bit0 = (color_prom[2*Machine->drv->total_colors] >> 0) & 0x01;
		bit1 = (color_prom[2*Machine->drv->total_colors] >> 1) & 0x01;
		bit2 = (color_prom[2*Machine->drv->total_colors] >> 2) & 0x01;
		bit3 = (color_prom[2*Machine->drv->total_colors] >> 3) & 0x01;
		b = 0x0e * bit0 + 0x1f * bit1 + 0x43 * bit2 + 0x8f * bit3;

		palette_set_color(i,r,g,b);

		color_prom++;
	}
}

PALETTE_INIT( lvcards ) //Ever so slightly different, but different enough.
{
	int i;

	for ( i = 0; i < Machine->drv->total_colors; i++ )
	{
		int bit0,bit1,bit2,bit3,r,g,b;

		/* red component */
		bit0 = (color_prom[0] >> 0) & 0x11;
		bit1 = (color_prom[0] >> 1) & 0x11;
		bit2 = (color_prom[0] >> 2) & 0x11;
		bit3 = (color_prom[0] >> 3) & 0x11;
		r = 0x0e * bit0 + 0x1f * bit1 + 0x43 * bit2 + 0x8f * bit3;

		/* green component */
		bit0 = (color_prom[Machine->drv->total_colors] >> 0) & 0x11;
		bit1 = (color_prom[Machine->drv->total_colors] >> 1) & 0x11;
		bit2 = (color_prom[Machine->drv->total_colors] >> 2) & 0x11;
		bit3 = (color_prom[Machine->drv->total_colors] >> 3) & 0x11;
		g = 0x0e * bit0 + 0x1f * bit1 + 0x43 * bit2 + 0x8f * bit3;

		/* blue component */
		bit0 = (color_prom[2*Machine->drv->total_colors] >> 0) & 0x11;
		bit1 = (color_prom[2*Machine->drv->total_colors] >> 1) & 0x11;
		bit2 = (color_prom[2*Machine->drv->total_colors] >> 2) & 0x11;
		bit3 = (color_prom[2*Machine->drv->total_colors] >> 3) & 0x11;
		b = 0x0e * bit0 + 0x1f * bit1 + 0x43 * bit2 + 0x8f * bit3;

		palette_set_color(i,r,g,b);

		color_prom++;
	}
}

WRITE8_HANDLER( lvcards_videoram_w )
{
	if (videoram[offset] != data)
	{
		videoram[offset] = data;
		tilemap_mark_tile_dirty(bg_tilemap, offset);
	}
}

WRITE8_HANDLER( lvcards_colorram_w )
{
	if (colorram[offset] != data)
	{
		colorram[offset] = data;
		tilemap_mark_tile_dirty(bg_tilemap, offset);
	}
}

static void get_bg_tile_info(int tile_index)
{
	int attr = colorram[tile_index];
	int code = videoram[tile_index] + ((attr & 0x30) << 4) + ((attr & 0x80) << 3);
	int color = attr & 0x0f;
	int flags = (attr & 0x40) ? TILE_FLIPX : 0;

	SET_TILE_INFO(0, code, color, flags)
}

VIDEO_START( lvcards )
{
	bg_tilemap = tilemap_create(get_bg_tile_info, tilemap_scan_rows,
		TILEMAP_OPAQUE, 8, 8, 32, 32);

	if ( !bg_tilemap )
		return 1;

	return 0;
}

VIDEO_UPDATE( lvcards )
{
	tilemap_draw(bitmap, cliprect, bg_tilemap, 0, 0);
}
