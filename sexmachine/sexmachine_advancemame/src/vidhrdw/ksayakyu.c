#include "driver.h"

tilemap *ksayakyu_tilemap;
tilemap *ksayakyu_textmap;

static int video_ctrl;
static int flipscreen;

WRITE8_HANDLER(ksayakyu_videoram_w)
{
	videoram[offset]=data;
	tilemap_mark_tile_dirty(ksayakyu_textmap,offset>>1);
}

WRITE8_HANDLER(ksayakyu_videoctrl_w)
{
	/*
        bits:
        76543210
              xx - ?? layers enable ?
             x   - screen flip
           xx    - ??
        xxx      - scroll offset

     */
    video_ctrl=data;

    flipscreen = (data&4)?(TILEMAP_FLIPX|TILEMAP_FLIPY):0;
	tilemap_set_flip( ALL_TILEMAPS,flipscreen );
	tilemap_set_scrolly( ksayakyu_tilemap, 0, (data&0xe0)<<3 );
}

PALETTE_INIT( ksayakyu )
{
	int i,j,r,g,b,b1,b2;
	for(j=0;j<16;j++)
		for(i=0;i<8;i++)
		{
			b1=memory_region(REGION_PROMS)[j*16+i];
			b2=memory_region(REGION_PROMS)[j*16+i+8];

			b1=b2|(b1<<8);
			g=(b1&31)<<3;
			b=((b1>>5)&31)<<3;
			r=((b1>>10)&31)<<3;

			palette_set_color(j*8+i,r,g,b);
		}
}

static void get_ksayakyu_tile_info(int tile_index)
{
	int code = memory_region(REGION_USER1)[tile_index];
	int attr = memory_region(REGION_USER1)[tile_index+0x2000];
	code+=(attr&3)<<8;
	SET_TILE_INFO(1,code,((attr>>2)&0x07)*2,(attr&0x80) ? TILE_FLIPX : 0)
}

static void get_text_tile_info(int tile_index)
{
	int code = videoram[tile_index*2+1];
	int attr = videoram[tile_index*2];
	int  flags=((attr&0x80) ? TILE_FLIPX : 0) | ((attr&0x40) ? TILE_FLIPY : 0);

	code|=(attr&3)<<8;

	SET_TILE_INFO(0,code,((attr>>2)&7),flags)
}

static void draw_sprites( mame_bitmap *bitmap, const rectangle *cliprect )
{
	const unsigned char *source = spriteram+spriteram_size-4;
	const unsigned char *finish = spriteram;

	while( source>=finish ) /* is order correct ? */
	{
		int sx=source[2];
		int sy=240-source[1];
		int attributes=source[3];
		int tile=source[0];
		int flipx=(tile&0x80)?1:0;
		int flipy=0;

		gfx_element *gfx = Machine->gfx[2];

		if (flipscreen)
		{
			sx = 240-sx;
			sy = 240-sy;
			flipx^=1;
			flipy^=1;
		}

			drawgfx(bitmap,gfx,
				tile&0x7f,
				(attributes) & 7,
				flipx,flipy,
				sx,sy,
				cliprect,TRANSPARENCY_PEN,0 );

		source -= 4;
	}
}

VIDEO_START(ksayakyu)
{
	ksayakyu_tilemap = tilemap_create(get_ksayakyu_tile_info,tilemap_scan_rows,TILEMAP_OPAQUE, 8, 8,32,32*8);
	ksayakyu_textmap = tilemap_create(get_text_tile_info,tilemap_scan_rows,TILEMAP_TRANSPARENT, 8, 8,32,32);
	tilemap_set_transparent_pen(ksayakyu_textmap,0);
	return 0;
}

VIDEO_UPDATE(ksayakyu)
{
	fillbitmap(bitmap,Machine->pens[0],cliprect);
	if(video_ctrl&1)
		tilemap_draw(bitmap,cliprect,ksayakyu_tilemap,0,0);
	draw_sprites(bitmap,cliprect);
	tilemap_draw(bitmap,cliprect,ksayakyu_textmap, 0,0);
}
