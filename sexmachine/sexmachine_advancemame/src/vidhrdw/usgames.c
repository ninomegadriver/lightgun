#include "driver.h"

UINT8 *usg_videoram,*usg_charram;


tilemap *usg_tilemap;



PALETTE_INIT(usg)
{
	int j;

	for (j = 0;j < 16;j++)
	{
		int r = (j & 1) >> 0;
		int g = (j & 2) >> 1;
		int b = (j & 4) >> 2;
		int i = (j & 8) >> 3;

		r = 0xff * r;
		g = 0x7f * g * (i + 1);
		b = 0x7f * b * (i + 1);

		palette_set_color(j,r,g,b);
	}

	for (j = 0;j < 256;j++)
	{
		colortable[2*j] = j & 0x0f;
		colortable[2*j+1] = j >> 4;
	}
}



static void get_usg_tile_info(int tile_index)
{
	int tileno, colour;

	tileno = usg_videoram[tile_index*2];
	colour = usg_videoram[tile_index*2+1];

	SET_TILE_INFO(0,tileno,colour,0)
}

VIDEO_START(usg)
{
	usg_tilemap = tilemap_create(get_usg_tile_info,tilemap_scan_rows,TILEMAP_OPAQUE, 8, 8,64,32);

	if (!usg_tilemap)
		return 1;

	return 0;
}


WRITE8_HANDLER( usg_videoram_w )
{
	usg_videoram[offset] = data;
	tilemap_mark_tile_dirty(usg_tilemap,offset/2);
}

WRITE8_HANDLER( usg_charram_w )
{
	usg_charram[offset] = data;

	decodechar(Machine->gfx[0], offset/8, usg_charram, Machine->drv->gfxdecodeinfo[0].gfxlayout);

	tilemap_mark_all_tiles_dirty(usg_tilemap);
}



VIDEO_UPDATE(usg)
{
	tilemap_draw(bitmap,cliprect,usg_tilemap,0,0);
}
