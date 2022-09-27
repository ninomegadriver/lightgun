/* Funny Bubble Video hardware

todo - convert to tilemap

 */


#include "driver.h"

UINT8* funybubl_banked_videoram;
UINT8 *funybubl_paletteram;


WRITE8_HANDLER ( funybubl_paldatawrite )
{
	int colchanged ;

	UINT32 coldat;
	int r,g,b;

	funybubl_paletteram[offset] = data;

	colchanged = offset >> 2;

	coldat = funybubl_paletteram[colchanged*4] | (funybubl_paletteram[colchanged*4+1] << 8) | (funybubl_paletteram[colchanged*4+2] << 16) | (funybubl_paletteram[colchanged*4+3] << 24);

	g = coldat & 0x003f;
	b = (coldat >> 6) & 0x3f;
	r = (coldat >> 12) & 0x3f;

	palette_set_color(colchanged,r<<2,g<<2,b<<2);
}


VIDEO_START(funybubl)
{
	return 0;
}

static void funybubl_drawsprites( mame_bitmap *bitmap, const rectangle *cliprect )
{


	UINT8 *source = &funybubl_banked_videoram[0x2000-0x20];
	UINT8 *finish = source - 0x1000;


	while( source>finish )
	{
		int xpos, ypos, tile;

		/* the sprites are in the sprite list twice
         the first format (in comments) appears to be a buffer, if you use
         this list you get garbage sprites in 2 player mode
         the second format (used) seems correct

         */
/*
        ypos = 0xff-source[1+0x10];
        xpos = source[2+0x10];
        tile =  source[0+0x10] | ( (source[3+0x10] & 0x0f) <<8);
        if (source[3+0x10] & 0x80) tile += 0x1000;
        if (source[3+0x10] & 0x20) xpos += 0x100;
        // bits 0x40 (not used?) and 0x10 (just set during transition period of x co-ord 0xff and 0x00) ...
        xpos -= 8;
        ypos -= 14;

*/
		ypos = source[2];
		xpos = source[3];
		tile =  source[0] | ( (source[1] & 0x0f) <<8);
		if (source[1] & 0x80) tile += 0x1000;
		if (source[1] & 0x20) {	if (xpos < 0xe0) xpos += 0x100; }
		// bits 0x40 and 0x10 not used?...

		drawgfx(bitmap,Machine->gfx[1],tile,0,0,0,xpos,ypos,cliprect,TRANSPARENCY_PEN,255);

		source -= 0x20;
	}

}


VIDEO_UPDATE(funybubl)
{
	int x,y, offs;

	offs = 0;

	fillbitmap(bitmap, get_black_pen(), cliprect);


	/* tilemap .. convert it .. banking makes it slightly more annoying but still easy */
	for (y = 0; y < 32; y++)
	{
		for (x = 0; x< 64; x++)
		{
			int data;

			data = funybubl_banked_videoram[offs] | (funybubl_banked_videoram[offs+1] << 8);
			drawgfx(bitmap,Machine->gfx[0],data&0x7fff,(data&0x8000)?2:1,0,0,x*8,y*8,cliprect,TRANSPARENCY_PEN,0);
			offs+=2;
		}
	}

	funybubl_drawsprites(bitmap,cliprect);

/*
    if ( code_pressed_memory(KEYCODE_W) )
    {
        FILE *fp;

        fp=fopen("funnybubsprites", "w+b");
        if (fp)
        {
            fwrite(&funybubl_banked_videoram[0x1000], 0x1000, 1, fp);
            fclose(fp);
        }
    }
*/
}
