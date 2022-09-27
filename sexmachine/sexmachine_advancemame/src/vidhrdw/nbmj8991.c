/******************************************************************************

    Video Hardware for Nichibutsu Mahjong series.

    Driver by Takahiro Nogi <nogi@kt.rim.or.jp> 1999/11/05 -

******************************************************************************/

#include "driver.h"
#include "nb1413m3.h"


static int nbmj8991_scrollx, nbmj8991_scrolly;
static int blitter_destx, blitter_desty;
static int blitter_sizex, blitter_sizey;
static int blitter_src_addr;
static int blitter_direction_x, blitter_direction_y;
static int nbmj8991_gfxrom;
static int nbmj8991_dispflag;
static int nbmj8991_flipscreen;
static int nbmj8991_clutsel;
static int nbmj8991_screen_refresh;

static mame_bitmap *nbmj8991_tmpbitmap;
static UINT8 *nbmj8991_videoram;
static UINT8 *nbmj8991_clut;


static void nbmj8991_vramflip(void);
static void nbmj8991_gfxdraw(void);
static void update_pixel(int x, int y);


/******************************************************************************


******************************************************************************/

WRITE8_HANDLER( nbmj8991_palette_type1_w )
{
	int r, g, b;

	paletteram[offset] = data;

	if (!(offset & 1)) return;

	offset &= 0x1fe;

	r = ((paletteram[offset + 0] & 0x0f) << 4);
	g = ((paletteram[offset + 1] & 0xf0) << 0);
	b = ((paletteram[offset + 1] & 0x0f) << 4);

	r = (r | (r >> 4));
	g = (g | (g >> 4));
	b = (b | (b >> 4));

	palette_set_color((offset >> 1), r, g, b);
}

WRITE8_HANDLER( nbmj8991_palette_type2_w )
{
	int r, g, b;

	paletteram[offset] = data;

	if (!(offset & 1)) return;

	offset &= 0x1fe;

	r = ((paletteram[offset + 0] & 0x7c) >> 2);
	g = (((paletteram[offset + 0] & 0x03) << 3) | ((paletteram[offset + 1] & 0xe0) >> 5));
	b = ((paletteram[offset + 1] & 0x1f) >> 0);

	r = ((r << 3) | (r >> 2));
	g = ((g << 3) | (g >> 2));
	b = ((b << 3) | (b >> 2));

	palette_set_color((offset / 2), r, g, b);
}

WRITE8_HANDLER( nbmj8991_palette_type3_w )
{
	int r, g, b;

	paletteram[offset] = data;

	if (!(offset & 1)) return;

	offset &= 0x1fe;

	r = ((paletteram[offset + 1] & 0x0f) << 4);
	g = ((paletteram[offset + 0] & 0xf0) << 0);
	b = ((paletteram[offset + 0] & 0x0f) << 4);

	r = (r | (r >> 4));
	g = (g | (g >> 4));
	b = (b | (b >> 4));

	palette_set_color((offset >> 1), r, g, b);
}

/******************************************************************************


******************************************************************************/
WRITE8_HANDLER( nbmj8991_blitter_w )
{
	switch (offset)
	{
		case 0x00:	blitter_src_addr = (blitter_src_addr & 0xff00) | data; break;
		case 0x01:	blitter_src_addr = (blitter_src_addr & 0x00ff) | (data << 8); break;
		case 0x02:	break;
		case 0x03:	break;
		case 0x04:	blitter_sizex = data; break;
		case 0x05:	blitter_sizey = data;
					/* writing here also starts the blit */
					nbmj8991_gfxdraw();
					break;
		case 0x06:	blitter_direction_x = (data & 0x01) ? 1 : 0;
					blitter_direction_y = (data & 0x02) ? 1 : 0;
					nbmj8991_flipscreen = (data & 0x04) ? 0 : 1;
					nbmj8991_dispflag = (data & 0x10) ? 0 : 1;
					nbmj8991_vramflip();
					break;
		case 0x07:	break;
		case 0x10:	blitter_destx = (blitter_destx & 0xff00) | data; break;
		case 0x20:	blitter_desty = (blitter_desty & 0xff00) | data; break;
		case 0x30:	nbmj8991_scrollx = (nbmj8991_scrollx & 0xff00) | data; break;
		case 0x40:	nbmj8991_scrolly = (nbmj8991_scrolly & 0xff00) | data; break;
		case 0x50: 	blitter_destx = (blitter_destx & 0x00ff) | ((data & 0x01) << 8);
					blitter_desty = (blitter_desty & 0x00ff) | ((data & 0x02) << 7);
					nbmj8991_scrollx = (nbmj8991_scrollx & 0x00ff) | ((data & 0x04) << 6);
					nbmj8991_scrolly = (nbmj8991_scrolly & 0x00ff) | ((data & 0x08) << 5);
					break;
		case 0x60:	nbmj8991_gfxrom = data; break;
		case 0x70:	nbmj8991_clutsel = data; break;
	}

	if ((0x20000 * nbmj8991_gfxrom) > (memory_region_length(REGION_GFX1) - 1))
	{
#ifdef MAME_DEBUG
		ui_popup("GFXROM BANK OVER!!");
#endif
		nbmj8991_gfxrom &= (memory_region_length(REGION_GFX1) / 0x20000 - 1);
	}
}

READ8_HANDLER( nbmj8991_clut_r )
{
	return nbmj8991_clut[offset];
}

WRITE8_HANDLER( nbmj8991_clut_w )
{
	nbmj8991_clut[((nbmj8991_clutsel & 0x7f) * 0x10) + (offset & 0x0f)] = data;
}

/******************************************************************************


******************************************************************************/
static void nbmj8991_vramflip(void)
{
	static int nbmj8991_flipscreen_old = 0;
	int x, y;
	unsigned char color1, color2;

	if (nbmj8991_flipscreen == nbmj8991_flipscreen_old) return;

	for (y = 0; y < Machine->drv->screen_height / 2; y++)
	{
		for (x = 0; x < Machine->drv->screen_width / 2; x++)
		{
			// rotate 180 degrees (   0,   0) - ( 511, 511)
			color1 = nbmj8991_videoram[(y * Machine->drv->screen_width) + x];
			color2 = nbmj8991_videoram[(((Machine->drv->screen_height - 1) - y) * Machine->drv->screen_width) + (((Machine->drv->screen_width / 2) - 1) - x)];
			nbmj8991_videoram[(y * Machine->drv->screen_width) + x] = color2;
			nbmj8991_videoram[(((Machine->drv->screen_height - 1) - y) * Machine->drv->screen_width) + (((Machine->drv->screen_width / 2) - 1) - x)] = color1;
			// rotate 180 degrees ( 512,   0) - (1023, 511)
			color1 = nbmj8991_videoram[(y * Machine->drv->screen_width) + (x + (Machine->drv->screen_width / 2))];
			color2 = nbmj8991_videoram[(((Machine->drv->screen_height - 1) - y) * Machine->drv->screen_width) + ((((Machine->drv->screen_width / 2) - 1) - x) + (Machine->drv->screen_width / 2))];
			nbmj8991_videoram[(y * Machine->drv->screen_width) + (x + (Machine->drv->screen_width / 2))] = color2;
			nbmj8991_videoram[(((Machine->drv->screen_height - 1) - y) * Machine->drv->screen_width) + ((((Machine->drv->screen_width / 2) - 1) - x) + (Machine->drv->screen_width / 2))] = color1;
		}
	}

	nbmj8991_flipscreen_old = nbmj8991_flipscreen;
	nbmj8991_screen_refresh = 1;
}

static void update_pixel(int x, int y)
{
	UINT8 color = nbmj8991_videoram[(y * Machine->drv->screen_width) + x];
	plot_pixel(nbmj8991_tmpbitmap, x, y, Machine->pens[color]);
}

static void blitter_timer_callback(int param)
{
	nb1413m3_busyflag = 1;
}

static void nbmj8991_gfxdraw(void)
{
	unsigned char *GFX = memory_region(REGION_GFX1);

	int x, y;
	int dx1, dx2, dy;
	int startx, starty;
	int sizex, sizey;
	int skipx, skipy;
	int ctrx, ctry;
	unsigned char color, color1, color2;
	int gfxaddr;

	nb1413m3_busyctr = 0;

	if (blitter_direction_x)
	{
		startx = blitter_destx;
		sizex = blitter_sizex ^ 0xff;
		skipx = 1;
	}
	else
	{
		startx = blitter_destx + blitter_sizex;
		sizex = blitter_sizex;
		skipx = -1;
	}

	if (blitter_direction_y)
	{
		starty = blitter_desty;
		sizey = blitter_sizey ^ 0xff;
		skipy = 1;
	}
	else
	{
		starty = blitter_desty + blitter_sizey;
		sizey = blitter_sizey;
		skipy = -1;
	}

	gfxaddr = (nbmj8991_gfxrom << 17) + (blitter_src_addr << 1);

	for (y = starty, ctry = sizey; ctry >= 0; y += skipy, ctry--)
	{
		for (x = startx, ctrx = sizex; ctrx >= 0; x += skipx, ctrx--)
		{
			if ((gfxaddr > (memory_region_length(REGION_GFX1) - 1)))
			{
#ifdef MAME_DEBUG
				ui_popup("GFXROM ADDRESS OVER!!");
#endif
				gfxaddr &= (memory_region_length(REGION_GFX1) - 1);
			}

			color = GFX[gfxaddr++];

			dx1 = (2 * x + 0) & 0x3ff;
			dx2 = (2 * x + 1) & 0x3ff;
			dy = y & 0x1ff;

			if (!nbmj8991_flipscreen)
			{
				dx1 ^= 0x1ff;
				dx2 ^= 0x1ff;
				dy ^= 0x1ff;
			}

			if (blitter_direction_x)
			{
				// flip
				color1 = (color & 0x0f) >> 0;
				color2 = (color & 0xf0) >> 4;
			}
			else
			{
				// normal
				color1 = (color & 0xf0) >> 4;
				color2 = (color & 0x0f) >> 0;
			}

			color1 = nbmj8991_clut[((nbmj8991_clutsel & 0x7f) * 0x10) + color1];
			color2 = nbmj8991_clut[((nbmj8991_clutsel & 0x7f) * 0x10) + color2];

			if (color1 != 0xff)
			{
				nbmj8991_videoram[(dy * Machine->drv->screen_width) + dx1] = color1;
				update_pixel(dx1, dy);
			}
			if (color2 != 0xff)
			{
				nbmj8991_videoram[(dy * Machine->drv->screen_width) + dx2] = color2;
				update_pixel(dx2, dy);
			}

			nb1413m3_busyctr++;
		}
	}

	nb1413m3_busyflag = 0;
	timer_set((double)nb1413m3_busyctr * TIME_IN_NSEC(1650), 0, blitter_timer_callback);
}

/******************************************************************************


******************************************************************************/
VIDEO_START( nbmj8991 )
{
	if ((nbmj8991_tmpbitmap = auto_bitmap_alloc(Machine->drv->screen_width, Machine->drv->screen_height)) == 0) return 1;
	nbmj8991_videoram = auto_malloc(Machine->drv->screen_width * Machine->drv->screen_height * sizeof(UINT8));
	nbmj8991_clut = auto_malloc(0x800 * sizeof(UINT8));
	memset(nbmj8991_videoram, 0x00, (Machine->drv->screen_width * Machine->drv->screen_height * sizeof(UINT8)));
	return 0;
}

VIDEO_UPDATE( nbmj8991_type1 )
{
	int x, y;

	if (get_vh_global_attribute_changed() || nbmj8991_screen_refresh)
	{
		nbmj8991_screen_refresh = 0;

		for (y = 0; y < Machine->drv->screen_height; y++)
		{
			for (x = 0; x < Machine->drv->screen_width; x++)
			{
				update_pixel(x, y);
			}
		}
	}

	if (nbmj8991_dispflag)
	{
		static int scrollx, scrolly;

		if (nbmj8991_flipscreen)
		{
			scrollx = (((-nbmj8991_scrollx) + 0x000) & 0x1ff) * 2;
			scrolly =  ((-nbmj8991_scrolly) - 0x00f) & 0x1ff;
		}
		else
		{
			scrollx = (((-nbmj8991_scrollx) - 0x100) & 0x1ff) * 2;
			scrolly =  (( nbmj8991_scrolly) + 0x0f1) & 0x1ff;
		}

		copyscrollbitmap(bitmap, nbmj8991_tmpbitmap, 1, &scrollx, 1, &scrolly, &Machine->visible_area, TRANSPARENCY_NONE, 0);
	}
	else
	{
		fillbitmap(bitmap, Machine->pens[0x00], 0);
	}
}

VIDEO_UPDATE( nbmj8991_type2 )
{
	int x, y;

	if (get_vh_global_attribute_changed() || nbmj8991_screen_refresh)
	{
		nbmj8991_screen_refresh = 0;

		for (y = 0; y < Machine->drv->screen_height; y++)
		{
			for (x = 0; x < Machine->drv->screen_width; x++)
			{
				update_pixel(x, y);
			}
		}
	}

	if (nb1413m3_inputport & 0x20)
	{
		static int scrollx, scrolly;

		if (nbmj8991_flipscreen)
		{
			scrollx = (((-nbmj8991_scrollx) + 0x000) & 0x1ff) * 2;
			scrolly =  ((-nbmj8991_scrolly) - 0x00f) & 0x1ff;
		}
		else
		{
			scrollx = (((-nbmj8991_scrollx) - 0x100) & 0x1ff) * 2;
			scrolly =  (( nbmj8991_scrolly) + 0x0f1) & 0x1ff;
		}

		copyscrollbitmap(bitmap, nbmj8991_tmpbitmap, 1, &scrollx, 1, &scrolly, &Machine->visible_area, TRANSPARENCY_NONE, 0);
	}
	else
	{
		fillbitmap(bitmap, Machine->pens[0x00], 0);
	}
}
