
#include "driver.h"

extern UINT8 *xyonix_vidram;

static tilemap *xyonix_tilemap;


PALETTE_INIT( xyonix )
{
	int i;


	for (i = 0;i < Machine->drv->total_colors;i++)
	{
		int bit0,bit1,bit2,r,g,b;

		/* red component */
		bit0 = (color_prom[i] >> 0) & 0x01;
		bit1 = (color_prom[i] >> 1) & 0x01;
		bit2 = (color_prom[i] >> 2) & 0x01;
		r = 0x21 * bit0 + 0x47 * bit1 + 0x97 * bit2;
		/* green component */
		bit0 = (color_prom[i] >> 5) & 0x01;
		bit1 = (color_prom[i] >> 6) & 0x01;
		bit2 = (color_prom[i] >> 7) & 0x01;
		g = 0x21 * bit0 + 0x47 * bit1 + 0x97 * bit2;
		/* blue component */
		bit0 = (color_prom[i] >> 3) & 0x01;
		bit1 = (color_prom[i] >> 4) & 0x01;
		b = 0x4f * bit0 + 0xa8 * bit1;

		palette_set_color(i,r,g,b);
	}
}


static void get_xyonix_tile_info(int tile_index)
{
	int tileno;
	int attr = xyonix_vidram[tile_index+0x1000+1];

	tileno = (xyonix_vidram[tile_index+1] << 0) | ((attr & 0x0f) << 8);

	SET_TILE_INFO(0,tileno,attr >> 4,0)
}

WRITE8_HANDLER( xyonix_vidram_w )
{
	xyonix_vidram[offset] = data;
	tilemap_mark_tile_dirty(xyonix_tilemap,(offset-1)&0x0fff);
}

VIDEO_START(xyonix)
{
	xyonix_tilemap = tilemap_create(get_xyonix_tile_info,tilemap_scan_rows,TILEMAP_OPAQUE, 4, 8,80,32);

	return 0;
}

VIDEO_UPDATE(xyonix)
{
	tilemap_draw(bitmap,cliprect,xyonix_tilemap,0,0);
}
