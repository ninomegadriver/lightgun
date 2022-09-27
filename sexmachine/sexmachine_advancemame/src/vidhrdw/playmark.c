#include "driver.h"



UINT16 *bigtwin_bgvideoram;
UINT16 *wbeachvl_videoram1,*wbeachvl_videoram2,*wbeachvl_videoram3;
UINT16 *wbeachvl_rowscroll;

static int bgscrollx,bgscrolly,bg_enable,bg_full_size;
static int fgscrollx,fg_rowscroll_enable;
static tilemap *tx_tilemap,*fg_tilemap,*bg_tilemap;

static int xoffset = 0;
static int yoffset = 0;

/***************************************************************************

  Callbacks for the TileMap code

***************************************************************************/

static void bigtwin_get_tx_tile_info(int tile_index)
{
	UINT16 code = wbeachvl_videoram1[2*tile_index];
	UINT16 color = wbeachvl_videoram1[2*tile_index+1];
	SET_TILE_INFO(
			2,
			code,
			color,
			0)
}

static void bigtwin_get_fg_tile_info(int tile_index)
{
	UINT16 code = wbeachvl_videoram2[2*tile_index];
	UINT16 color = wbeachvl_videoram2[2*tile_index+1];
	SET_TILE_INFO(
			1,
			code,
			color,
			0)
}

static void wbeachvl_get_tx_tile_info(int tile_index)
{
	UINT16 code = wbeachvl_videoram1[2*tile_index];
	UINT16 color = wbeachvl_videoram1[2*tile_index+1];

	SET_TILE_INFO(
			2,
			code,
			color / 4,
			0)
}

static void wbeachvl_get_fg_tile_info(int tile_index)
{
	UINT16 code = wbeachvl_videoram2[2*tile_index];
	UINT16 color = wbeachvl_videoram2[2*tile_index+1];

	SET_TILE_INFO(
			1,
			code & 0x7fff,
			color / 4 + 8,
			(code & 0x8000) ? TILE_FLIPX : 0)
}

static void wbeachvl_get_bg_tile_info(int tile_index)
{
	UINT16 code = wbeachvl_videoram3[2*tile_index];
	UINT16 color = wbeachvl_videoram3[2*tile_index+1];

	SET_TILE_INFO(
			1,
			code & 0x7fff,
			color / 4,
			(code & 0x8000) ? TILE_FLIPX : 0)
}

static void hotmind_get_tx_tile_info(int tile_index)
{
	int code = wbeachvl_videoram1[tile_index] & 0x07ff;
	int colr = wbeachvl_videoram1[tile_index] & 0xe000;

	SET_TILE_INFO(2,code + 0x3000,colr >> 13,0)
}

static void hotmind_get_fg_tile_info(int tile_index)
{
	int code = wbeachvl_videoram2[tile_index] & 0x07ff;
	int colr = wbeachvl_videoram2[tile_index] & 0xe000;

	SET_TILE_INFO(1,code + 0x800,(colr >> 13) + 8,0)
}

static void hotmind_get_bg_tile_info(int tile_index)
{
	int code = wbeachvl_videoram3[tile_index] & 0x07ff;
	int colr = wbeachvl_videoram3[tile_index] & 0xe000;

	SET_TILE_INFO(1,code,colr >> 13,0)
}

/***************************************************************************

  Start the video hardware emulation.

***************************************************************************/

VIDEO_START( bigtwin )
{
	tx_tilemap = tilemap_create(bigtwin_get_tx_tile_info,tilemap_scan_rows,TILEMAP_TRANSPARENT, 8, 8,64,32);
	fg_tilemap = tilemap_create(bigtwin_get_fg_tile_info,tilemap_scan_rows,TILEMAP_OPAQUE,     16,16,32,32);

	if (!tx_tilemap || !fg_tilemap)
		return 1;

	tilemap_set_transparent_pen(tx_tilemap,0);

	return 0;
}


VIDEO_START( wbeachvl )
{
	tx_tilemap = tilemap_create(wbeachvl_get_tx_tile_info,tilemap_scan_rows,TILEMAP_TRANSPARENT, 8, 8,64,32);
	fg_tilemap = tilemap_create(wbeachvl_get_fg_tile_info,tilemap_scan_rows,TILEMAP_TRANSPARENT,16,16,64,32);
	bg_tilemap = tilemap_create(wbeachvl_get_bg_tile_info,tilemap_scan_rows,TILEMAP_OPAQUE,     16,16,64,32);

	if (!tx_tilemap || !fg_tilemap || !bg_tilemap)
		return 1;

	tilemap_set_transparent_pen(tx_tilemap,0);
	tilemap_set_transparent_pen(fg_tilemap,0);

	return 0;
}

VIDEO_START( excelsr )
{
	tx_tilemap = tilemap_create(bigtwin_get_tx_tile_info,tilemap_scan_rows,TILEMAP_TRANSPARENT,16,16,32,32);
	fg_tilemap = tilemap_create(bigtwin_get_fg_tile_info,tilemap_scan_rows,TILEMAP_OPAQUE,     16,16,32,32);

	if (!tx_tilemap || !fg_tilemap)
		return 1;

	tilemap_set_transparent_pen(tx_tilemap,0);

	return 0;
}

VIDEO_START( hotmind )
{
	tx_tilemap = tilemap_create(hotmind_get_tx_tile_info,tilemap_scan_rows,TILEMAP_TRANSPARENT, 8, 8,64,64);
	fg_tilemap = tilemap_create(hotmind_get_fg_tile_info,tilemap_scan_rows,TILEMAP_TRANSPARENT,16,16,32,32);
	bg_tilemap = tilemap_create(hotmind_get_bg_tile_info,tilemap_scan_rows,TILEMAP_OPAQUE,     16,16,32,32);

	if (!tx_tilemap || !fg_tilemap || !bg_tilemap)
		return 1;

	tilemap_set_transparent_pen(tx_tilemap,0);
	tilemap_set_transparent_pen(fg_tilemap,0);

	xoffset = -9;
	yoffset = -8;

	return 0;
}

/***************************************************************************

  Memory handlers

***************************************************************************/

WRITE16_HANDLER( wbeachvl_txvideoram_w )
{
	int oldword = wbeachvl_videoram1[offset];
	COMBINE_DATA(&wbeachvl_videoram1[offset]);
	if (oldword != wbeachvl_videoram1[offset])
		tilemap_mark_tile_dirty(tx_tilemap,offset / 2);
}

WRITE16_HANDLER( wbeachvl_fgvideoram_w )
{
	int oldword = wbeachvl_videoram2[offset];
	COMBINE_DATA(&wbeachvl_videoram2[offset]);
	if (oldword != wbeachvl_videoram2[offset])
		tilemap_mark_tile_dirty(fg_tilemap,offset / 2);
}

WRITE16_HANDLER( wbeachvl_bgvideoram_w )
{
	int oldword = wbeachvl_videoram3[offset];
	COMBINE_DATA(&wbeachvl_videoram3[offset]);
	if (oldword != wbeachvl_videoram3[offset])
		tilemap_mark_tile_dirty(bg_tilemap,offset / 2);
}

WRITE16_HANDLER( hotmind_txvideoram_w )
{
	COMBINE_DATA(&wbeachvl_videoram1[offset]);
	tilemap_mark_tile_dirty(tx_tilemap,offset);
}

WRITE16_HANDLER( hotmind_fgvideoram_w )
{
	COMBINE_DATA(&wbeachvl_videoram2[offset]);
	tilemap_mark_tile_dirty(fg_tilemap,offset);
}

WRITE16_HANDLER( hotmind_bgvideoram_w )
{
	COMBINE_DATA(&wbeachvl_videoram3[offset]);
	tilemap_mark_tile_dirty(bg_tilemap,offset);
}


WRITE16_HANDLER( bigtwin_paletteram_w )
{
	int r,g,b,val;

	COMBINE_DATA(&paletteram16[offset]);

	val = paletteram16[offset];
	r = (val >> 11) & 0x1e;
	g = (val >>  7) & 0x1e;
	b = (val >>  3) & 0x1e;

	r |= ((val & 0x08) >> 3);
	g |= ((val & 0x04) >> 2);
	b |= ((val & 0x02) >> 1);

	r = (r << 3) | (r >> 2);
	g = (g << 3) | (g >> 2);
	b = (b << 3) | (b >> 2);

	palette_set_color(offset,r,g,b);
}

WRITE16_HANDLER( bigtwin_scroll_w )
{
	static UINT16 scroll[6];

	data = COMBINE_DATA(&scroll[offset]);

	switch (offset)
	{
		case 0: tilemap_set_scrollx(tx_tilemap,0,data+2); break;
		case 1: tilemap_set_scrolly(tx_tilemap,0,data);   break;
		case 2: bgscrollx = -(data+4);                    break;
		case 3: bgscrolly = (-data) & 0x1ff;
				bg_enable = data & 0x0200;
				bg_full_size = data & 0x0400;
				break;
		case 4: tilemap_set_scrollx(fg_tilemap,0,data+6); break;
		case 5: tilemap_set_scrolly(fg_tilemap,0,data);   break;
	}
}

WRITE16_HANDLER( wbeachvl_scroll_w )
{
	static UINT16 scroll[6];

	data = COMBINE_DATA(&scroll[offset]);

	switch (offset)
	{
		case 0: tilemap_set_scrollx(tx_tilemap,0,data+2); break;
		case 1: tilemap_set_scrolly(tx_tilemap,0,data);   break;
		case 2: fgscrollx = data+4;break;
		case 3: tilemap_set_scrolly(fg_tilemap,0,data & 0x3ff);
				fg_rowscroll_enable = data & 0x0800;
				break;
		case 4: tilemap_set_scrollx(bg_tilemap,0,data+6); break;
		case 5: tilemap_set_scrolly(bg_tilemap,0,data);   break;
	}
}

WRITE16_HANDLER( excelsr_scroll_w )
{
	static UINT16 scroll[6];

	data = COMBINE_DATA(&scroll[offset]);

	switch (offset)
	{
		case 0:	tilemap_set_scrollx(tx_tilemap,0,data+2); break;
		case 1: tilemap_set_scrolly(tx_tilemap,0,data);   break;
		case 2: bgscrollx = -data;                        break;
		case 3: bgscrolly = (-data+2)& 0x1ff;
				bg_enable = data & 0x0200;
				bg_full_size = data & 0x0400;
				break;
		case 4:	tilemap_set_scrollx(fg_tilemap,0,data+6); break;
		case 5:	tilemap_set_scrolly(fg_tilemap,0,data);   break;
	}
}

WRITE16_HANDLER( hotmind_scroll_w )
{
	static UINT16 scroll[6];

	data = COMBINE_DATA(&scroll[offset]);

	switch (offset)
	{
		case 0: tilemap_set_scrollx(tx_tilemap,0,data+14); break;
		case 1: tilemap_set_scrolly(tx_tilemap,0,data);    break;
		case 2: tilemap_set_scrollx(fg_tilemap,0,data+14); break;
		case 3: tilemap_set_scrolly(fg_tilemap,0,data);	   break;
		case 4: tilemap_set_scrollx(bg_tilemap,0,data+14); break;
		case 5: tilemap_set_scrolly(bg_tilemap,0,data);    break;
	}
}

/***************************************************************************

  Display refresh

***************************************************************************/

static void draw_sprites(mame_bitmap *bitmap,const rectangle *cliprect,int priority,int codeshift)
{
	int offs;
	int height = Machine->gfx[0]->height;
	int colordiv = Machine->gfx[0]->color_granularity / 16;

	for (offs = 4;offs < spriteram_size/2;offs += 4)
	{
		int sx,sy,code,color,flipx,pri;

		sy = spriteram16[offs+3-4];	/* -4? what the... ??? */
		if (sy == 0x2000) return;	/* end of list marker */

		flipx = sy & 0x4000;
		sx = (spriteram16[offs+1] & 0x01ff) - 16-7;
		sy = (256-8-height - sy) & 0xff;
		code = spriteram16[offs+2] >> codeshift;
		color = ((spriteram16[offs+1] & 0x3e00) >> 9) / colordiv;
		pri = (spriteram16[offs+1] & 0x8000) >> 15;

		if(!pri && (color & 0x0c) == 0x0c)
			pri = 2;

		if (pri != priority)
			continue;

		drawgfx(bitmap,Machine->gfx[0],
				code,
				color,
				flipx,0,
				sx + xoffset,sy + yoffset,
				cliprect,TRANSPARENCY_PEN,0);
	}
}

static void draw_bitmap(mame_bitmap *bitmap)
{
	int x,y,count;
	int color;

	count = 0;
	for (y=0;y<512;y++)
	{
		for (x=0;x<512;x++)
		{
			color = bigtwin_bgvideoram[count] & 0xff;

			if(color)
			{
				if(bg_full_size)
				{
					plot_pixel(bitmap, (x + bgscrollx) & 0x1ff, (y + bgscrolly) & 0x1ff, Machine->pens[0x100 + color]);
				}
				else
				{
					/* 50% size */
					if(!(x % 2) && !(y % 2))
					{
						plot_pixel(bitmap, (x / 2 + bgscrollx) & 0x1ff, (y / 2 + bgscrolly) & 0x1ff, Machine->pens[0x100 + color]);
					}
				}
			}

			count++;
		}
	}
}

VIDEO_UPDATE( bigtwin )
{
	tilemap_draw(bitmap,cliprect,fg_tilemap,0,0);
	if (bg_enable)
		draw_bitmap(bitmap);
	draw_sprites(bitmap,cliprect,2,4);
	draw_sprites(bitmap,cliprect,1,4);
	draw_sprites(bitmap,cliprect,0,4);
	tilemap_draw(bitmap,cliprect,tx_tilemap,0,0);
}

VIDEO_UPDATE( excelsr )
{
	tilemap_draw(bitmap,cliprect,fg_tilemap,0,0);
	draw_sprites(bitmap,cliprect,1,2);
	if (bg_enable)
		draw_bitmap(bitmap);
	draw_sprites(bitmap,cliprect,2,2);
	tilemap_draw(bitmap,cliprect,tx_tilemap,0,0);
	draw_sprites(bitmap,cliprect,0,2);
}

VIDEO_UPDATE( wbeachvl )
{
	if (fg_rowscroll_enable)
	{
		int i;

		tilemap_set_scroll_rows(fg_tilemap,512);
		for (i = 0;i < 256;i++)
			tilemap_set_scrollx(fg_tilemap,i+1,wbeachvl_rowscroll[8*i]);
	}
	else
	{
		tilemap_set_scroll_rows(fg_tilemap,1);
		tilemap_set_scrollx(fg_tilemap,0,fgscrollx);
	}

	tilemap_draw(bitmap,cliprect,bg_tilemap,0,0);
	draw_sprites(bitmap,cliprect,1,0);
	tilemap_draw(bitmap,cliprect,fg_tilemap,0,0);
	draw_sprites(bitmap,cliprect,2,0);
	draw_sprites(bitmap,cliprect,0,0);
	tilemap_draw(bitmap,cliprect,tx_tilemap,0,0);
}

VIDEO_UPDATE( hotmind )
{
	tilemap_draw(bitmap,cliprect,bg_tilemap,0,0);
	draw_sprites(bitmap,cliprect,1,2);
	tilemap_draw(bitmap,cliprect,fg_tilemap,0,0);
	draw_sprites(bitmap,cliprect,2,2);
	draw_sprites(bitmap,cliprect,0,2);
	tilemap_draw(bitmap,cliprect,tx_tilemap,0,0);
}

