/***************************************************************************

    SNK 68000 video routines

Notes:
    Search & Rescue uses Y flip on sprites only.
    Street Smart uses X flip on sprites only.

    Seems to be controlled in same byte as flipscreen.

    Emulation by Bryan McPhail, mish@tendril.co.uk

***************************************************************************/

#include "driver.h"

static int sprite_flip, pow_charbase=0; //*
static tilemap *fix_tilemap;

/***************************************************************************

  Callbacks for the TileMap code

***************************************************************************/

static void get_pow_tile_info(int tile_index)
{
	int tile=videoram16[2*tile_index]&0xff;
	int color=videoram16[2*tile_index+1];

	tile += pow_charbase; //AT: (powj36rc2gre)
	color&=0xf;

	SET_TILE_INFO(
			0,
			tile,
			color,
			0)
}

static void get_sar_tile_info(int tile_index)
{
	int tile=videoram16[2*tile_index];
	int color=tile >> 12;

	tile=tile&0xfff;

	SET_TILE_INFO(
			0,
			tile,
			color,
			0)
}

static void get_ikari3_tile_info(int tile_index)
{
	int tile=videoram16[2*tile_index];
	int color=tile >> 12;

	/* Kludge - Tile 0x80ff is meant to be opaque black, but isn't.  This fixes it */
	if (tile==0x80ff) {
		tile=0x2ca;
		color=7;
	} else
		tile=tile&0xfff;

	SET_TILE_INFO(
			0,
			tile,
			color,
			0)
}

/***************************************************************************

  Start the video hardware emulation.

***************************************************************************/

VIDEO_START( pow )
{
	fix_tilemap = tilemap_create(get_pow_tile_info,tilemap_scan_cols,TILEMAP_TRANSPARENT,8,8,32,32);

	if (!fix_tilemap)
		return 1;

	tilemap_set_transparent_pen(fix_tilemap,0);

	return 0;
}

VIDEO_START( searchar )
{
	fix_tilemap = tilemap_create(get_sar_tile_info,tilemap_scan_cols,TILEMAP_TRANSPARENT,8,8,32,32);

	if (!fix_tilemap)
		return 1;

	tilemap_set_transparent_pen(fix_tilemap,0);

	return 0;
}

VIDEO_START( ikari3 )
{
	fix_tilemap = tilemap_create(get_ikari3_tile_info,tilemap_scan_cols,TILEMAP_TRANSPARENT,8,8,32,32);

	if (!fix_tilemap)
		return 1;

	tilemap_set_transparent_pen(fix_tilemap,0);

	return 0;
}

/***************************************************************************

  Memory handlers

***************************************************************************/

WRITE16_HANDLER( pow_flipscreen16_w )
{
	if (ACCESSING_LSB)
	{
	    flip_screen_set(data & 0x08);
	    sprite_flip=data&0x4;
	    pow_charbase = (data & 0x70) << 4;
	}
}

WRITE16_HANDLER( pow_paletteram16_word_w )
{
	UINT16 newword;
	int r,g,b;

	COMBINE_DATA(&paletteram16[offset]);
	newword = paletteram16[offset];

	r = ((newword >> 7) & 0x1e) | ((newword >> 14) & 0x01);
	g = ((newword >> 3) & 0x1e) | ((newword >> 13) & 0x01) ;
	b = ((newword << 1) & 0x1e) | ((newword >> 12) & 0x01) ;

	r = (r << 3) | (r >> 2);
	g = (g << 3) | (g >> 2);
	b = (b << 3) | (b >> 2);

	palette_set_color(offset,r,g,b);
}

WRITE16_HANDLER( pow_video16_w )
{
	COMBINE_DATA(&videoram16[offset]);
	tilemap_mark_tile_dirty(fix_tilemap,offset>>1);
}


/***************************************************************************

  Display refresh

***************************************************************************/

static void draw_sprites(mame_bitmap *bitmap, const rectangle *cliprect, int j,int pos)
{
	int offs,mx,my,color,tile,fx,fy,i;

	for (offs = pos; offs < pos+0x800; offs += 0x80 )
	{
		mx=(spriteram16[(offs+4+(4*j))>>1]&0xff)<<4;
		my=spriteram16[(offs+6+(4*j))>>1];
		mx=mx+(my>>12);
		mx=((mx+16)&0x1ff)-16;

		mx=((mx+0x100)&0x1ff)-0x100;
		my=((my+0x100)&0x1ff)-0x100;

		my=0x200 - my;
		my-=0x200;

		if (flip_screen) {
			mx=240-mx;
			my=240-my;
		}

		for (i=0; i<0x80; i+=4) {
			color=spriteram16[(offs+i+(0x1000*j)+0x1000)>>1]&0x7f;

			if (color) {
				tile=spriteram16[(offs+2+i+(0x1000*j)+0x1000)>>1];
				fy=tile&0x8000;
				fx=tile&0x4000;
				tile&=0x3fff;

				if (flip_screen) {
					if (fx) fx=0; else fx=1;
					if (fy) fy=0; else fy=1;
				}

				drawgfx(bitmap,Machine->gfx[1],
					tile,
					color,
					fx,fy,
					mx,my,
					cliprect,TRANSPARENCY_PEN,0);
			}

			if (flip_screen) {
				my-=16;
				if (my < -0x100) my+=0x200;
			}
			else {
				my+=16;
				if (my > 0x100) my-=0x200;
			}
		}
	}
}


VIDEO_UPDATE( pow )
{
	fillbitmap(bitmap,Machine->pens[2047],cliprect);

	/* This appears to be correct priority */
	draw_sprites(bitmap,cliprect,1,0x000);
	draw_sprites(bitmap,cliprect,1,0x800);
	draw_sprites(bitmap,cliprect,2,0x000);
	draw_sprites(bitmap,cliprect,2,0x800);
	draw_sprites(bitmap,cliprect,0,0x000); //AT: (pow37b5yel)
	draw_sprites(bitmap,cliprect,0,0x800);

	tilemap_draw(bitmap,cliprect,fix_tilemap,0,0);
}


static void draw_sprites2(mame_bitmap *bitmap, const rectangle *cliprect, int j, int z, int pos)
{
	int offs,mx,my,color,tile,fx,fy,i;

	for (offs = pos; offs < pos+0x800 ; offs += 0x80 )
	{
		mx=spriteram16[(offs+j)>>1];
		my=spriteram16[(offs+j+2)>>1];

		mx=mx<<4;
		mx=mx|((my>>12)&0xf);
		my=my&0x1ff;

		mx=(mx+0x100)&0x1ff;
		my=(my+0x100)&0x1ff;
		mx-=0x100;
		my-=0x100;
		my=0x200 - my;
		my-=0x200;

		if (flip_screen) {
			mx=240-mx;
			my=240-my;
		}

		for (i=0; i<0x80; i+=4) {
			color=spriteram16[(offs+i+z)>>1]&0x7f;
			if (color) {
				tile=spriteram16[(offs+2+i+z)>>1];
				if (sprite_flip) {
					fx=0;
					fy=tile&0x8000;
				} else {
					fy=0;
					fx=tile&0x8000;
				}

				if (flip_screen) {
					if (fx) fx=0; else fx=1;
					if (fy) fy=0; else fy=1;
				}

				tile&=0x7fff;
				if (tile>0x5fff) break;

				drawgfx(bitmap,Machine->gfx[1],
					tile,
					color,
					fx,fy,
					mx,my,
					cliprect,TRANSPARENCY_PEN,0);
			}
			if (flip_screen) {
				my-=16;
				if (my < -0x100) my+=0x200;
			}
			else {
				my+=16;
				if (my > 0x100) my-=0x200;
			}
		}
	}
}


VIDEO_UPDATE( searchar )
{
	fillbitmap(bitmap,Machine->pens[2047],cliprect);

	/* This appears to be correct priority */
	draw_sprites2(bitmap,cliprect,8,0x2000,0x000);
	draw_sprites2(bitmap,cliprect,8,0x2000,0x800);
	draw_sprites2(bitmap,cliprect,12,0x3000,0x000);
	draw_sprites2(bitmap,cliprect,12,0x3000,0x800);
	draw_sprites2(bitmap,cliprect,4,0x1000,0x000);
	draw_sprites2(bitmap,cliprect,4,0x1000,0x800);

	tilemap_draw(bitmap,cliprect,fix_tilemap,0,0);
}

