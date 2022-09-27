/***************************************************************************

    HAR MadMax hardware

**************************************************************************/


#include "driver.h"
#include "dcheese.h"


/*************************************
 *
 *  Constants
 *
 *************************************/

#define DSTBITMAP_WIDTH		512
#define DSTBITMAP_HEIGHT	512



/*************************************
 *
 *  Local variables
 *
 *************************************/

static UINT16 blitter_color[2];
static UINT16 blitter_xparam[16];
static UINT16 blitter_yparam[16];
static UINT16 blitter_vidparam[32];

static mame_bitmap *dstbitmap;
static mame_timer *blitter_timer;



/*************************************
 *
 *  Palette translation
 *
 *************************************/

PALETTE_INIT( dcheese )
{
	UINT16 *src = (UINT16 *)memory_region(REGION_USER1);
	int i;

	/* really 65536 colors, but they don't use the later ones so we can stay */
	/* within MAME's limits */
	for (i = 0; i < 65534; i++)
	{
		int data = *src++;
		int b = (data >> 11) & 0x1f;
		int g = (data >> 6) & 0x1f;
		int r = (data >> 0) & 0x3f;

		/* up to 8 bits */
		r = (r << 2) | (r >> 4);
		g = (g << 3) | (g >> 2);
		b = (b << 3) | (b >> 2);
		palette_set_color(i, r, g, b);
	}
}



/*************************************
 *
 *  Scanline interrupt
 *
 *************************************/

static void update_scanline_irq(void)
{
	/* if not in range, don't bother */
	if (blitter_vidparam[0x22/2] <= blitter_vidparam[0x1e/2])
	{
		int effscan;
		double time;

		/* compute the effective scanline of the interrupt */
		effscan = blitter_vidparam[0x22/2] - blitter_vidparam[0x1a/2];
		if (effscan < 0)
			effscan += blitter_vidparam[0x1e/2];

		/* determine the time; if it's in this scanline, bump to the next frame */
		time = cpu_getscanlinetime(effscan);
		if (time < cpu_getscanlineperiod())
			time += TIME_IN_HZ(Machine->refresh_rate);
		timer_adjust(blitter_timer, time, 0, 0);
	}
}


static void blitter_scanline_callback(int param)
{
	dcheese_signal_irq(3);
	update_scanline_irq();
}



/*************************************
 *
 *  Video start
 *
 *************************************/

VIDEO_START( dcheese )
{
	/* the destination bitmap is not directly accessible to the CPU */
	dstbitmap = auto_bitmap_alloc(DSTBITMAP_WIDTH, DSTBITMAP_HEIGHT);

	/* create a timer */
	blitter_timer = timer_alloc(blitter_scanline_callback);

	/* register for saving */
	state_save_register_global_array(blitter_color);
	state_save_register_global_array(blitter_xparam);
	state_save_register_global_array(blitter_yparam);
	state_save_register_global_array(blitter_vidparam);
	state_save_register_global_bitmap(dstbitmap);

	return 0;
}



/*************************************
 *
 *  Video update
 *
 *************************************/

VIDEO_UPDATE( dcheese )
{
	int x, y;

	/* update the pixels */
	for (y = cliprect->min_y; y <= cliprect->max_y; y++)
	{
		UINT16 *dest = (UINT16 *)bitmap->line[y];
		UINT16 *src = (UINT16 *)dstbitmap->line[(y + blitter_vidparam[0x28/2]) % DSTBITMAP_HEIGHT];

		for (x = cliprect->min_x; x <= cliprect->max_x; x++)
			dest[x] = src[x];
	}
}



/*************************************
 *
 *  Blitter implementation
 *
 *************************************/

static void do_clear(void)
{
	int y;

	/* clear the requested scanlines */
	for (y = blitter_vidparam[0x2c/2]; y < blitter_vidparam[0x2a/2]; y++)
		memset(dstbitmap->line[y % DSTBITMAP_HEIGHT], 0, DSTBITMAP_WIDTH * 2);

	/* signal an IRQ when done (timing is just a guess) */
	timer_set(cpu_getscanlineperiod(), 1, dcheese_signal_irq);
}


static void do_blit(void)
{
	INT32 srcminx = blitter_xparam[0] << 12;
	INT32 srcmaxx = blitter_xparam[1] << 12;
	INT32 srcminy = blitter_yparam[0] << 12;
	INT32 srcmaxy = blitter_yparam[1] << 12;
	INT32 srcx = ((blitter_xparam[2] & 0x0fff) | ((blitter_xparam[3] & 0x0fff) << 12)) << 7;
	INT32 srcy = ((blitter_yparam[2] & 0x0fff) | ((blitter_yparam[3] & 0x0fff) << 12)) << 7;
	INT32 dxdx = (INT32)(((blitter_xparam[4] & 0x0fff) | ((blitter_xparam[5] & 0x0fff) << 12)) << 12) >> 12;
	INT32 dxdy = (INT32)(((blitter_xparam[6] & 0x0fff) | ((blitter_xparam[7] & 0x0fff) << 12)) << 12) >> 12;
	INT32 dydx = (INT32)(((blitter_yparam[4] & 0x0fff) | ((blitter_yparam[5] & 0x0fff) << 12)) << 12) >> 12;
	INT32 dydy = (INT32)(((blitter_yparam[6] & 0x0fff) | ((blitter_yparam[7] & 0x0fff) << 12)) << 12) >> 12;
	UINT8 *src = memory_region(REGION_GFX1);
	UINT32 pagemask = (memory_region_length(REGION_GFX1) - 1) / 0x40000;
	int xstart = blitter_xparam[14];
	int xend = blitter_xparam[15] + 1;
	int ystart = blitter_yparam[14];
	int yend = blitter_yparam[15];
	int color = (blitter_color[0] << 8) & 0xff00;
	int mask = (blitter_color[0] >> 8) & 0x00ff;
	int opaque = (dxdx | dxdy | dydx | dydy) == 0;	/* bit of a hack for fredmem */
	int x, y;

	/* loop over target rows */
	for (y = ystart; y <= yend; y++)
	{
		UINT16 *dst = (UINT16 *)dstbitmap->line[y % DSTBITMAP_HEIGHT];

		/* loop over target columns */
		for (x = xstart; x <= xend; x++)
		{
			/* compute current X/Y positions */
			int sx = (srcx + dxdx * (x - xstart) + dxdy * (y - ystart)) & 0xffffff;
			int sy = (srcy + dydx * (x - xstart) + dydy * (y - ystart)) & 0xffffff;

			/* clip to source cliprect */
			if (sx >= srcminx && sx <= srcmaxx && sy >= srcminy && sy <= srcmaxy)
			{
				/* page comes from bit 22 of Y and bit 21 of X */
				int page = (((sy >> 21) & 2) | ((sx >> 21) & 1) | ((sx >> 20) & 4)) & pagemask;
				int pix = src[0x40000 * page + ((sy >> 12) & 0x1ff) * 512 + ((sx >> 12) & 0x1ff)];

				/* only non-zero pixels get written */
				if (pix | opaque)
					dst[x % DSTBITMAP_WIDTH] = (pix & mask) | color;
			}
		}
	}

	/* signal an IRQ when done (timing is just a guess) */
	timer_set(cpu_getscanlineperiod() / 2, 2, dcheese_signal_irq);

	/* these extra parameters are written but they are always zero, so I don't know what they do */
	if (blitter_xparam[8] != 0 || blitter_xparam[9] != 0 || blitter_xparam[10] != 0 || blitter_xparam[11] != 0 ||
		blitter_yparam[8] != 0 || blitter_yparam[9] != 0 || blitter_yparam[10] != 0 || blitter_yparam[11] != 0)
	{
		logerror("%06X:blit! (%04X)\n", activecpu_get_pc(), blitter_color[0]);
		logerror("   %04X %04X %04X %04X - %04X %04X %04X %04X - %04X %04X %04X %04X - %04X %04X %04X %04X\n",
				blitter_xparam[0], blitter_xparam[1], blitter_xparam[2], blitter_xparam[3],
				blitter_xparam[4], blitter_xparam[5], blitter_xparam[6], blitter_xparam[7],
				blitter_xparam[8], blitter_xparam[9], blitter_xparam[10], blitter_xparam[11],
				blitter_xparam[12], blitter_xparam[13], blitter_xparam[14], blitter_xparam[15]);
		logerror("   %04X %04X %04X %04X - %04X %04X %04X %04X - %04X %04X %04X %04X - %04X %04X %04X %04X\n",
				blitter_yparam[0], blitter_yparam[1], blitter_yparam[2], blitter_yparam[3],
				blitter_yparam[4], blitter_yparam[5], blitter_yparam[6], blitter_yparam[7],
				blitter_yparam[8], blitter_yparam[9], blitter_yparam[10], blitter_yparam[11],
				blitter_yparam[12], blitter_yparam[13], blitter_yparam[14], blitter_yparam[15]);
	}
}



/*************************************
 *
 *  Blitter read/write
 *
 *************************************/

WRITE16_HANDLER( madmax_blitter_color_w )
{
	COMBINE_DATA(&blitter_color[offset]);
}


WRITE16_HANDLER( madmax_blitter_xparam_w )
{
	COMBINE_DATA(&blitter_xparam[offset]);
}


WRITE16_HANDLER( madmax_blitter_yparam_w )
{
	COMBINE_DATA(&blitter_yparam[offset]);
}


WRITE16_HANDLER( madmax_blitter_vidparam_w )
{
	COMBINE_DATA(&blitter_vidparam[offset]);

	switch (offset)
	{
		case 0x10/2:		/* horiz front porch */
		case 0x12/2:		/* horiz display start */
		case 0x14/2:		/* horiz display end */
		case 0x16/2:		/* horiz back porch */

		case 0x18/2:		/* vert front porch */
		case 0x1a/2:		/* vert display start */
		case 0x1c/2:		/* vert display end */
		case 0x1e/2:		/* vert back porch */
			break;

		case 0x22/2:		/* scanline interrupt */
			update_scanline_irq();
			break;

		case 0x24/2:		/* writes here after writing to 0x28 */
			break;

		case 0x28/2:		/* display starting y */
		case 0x2a/2:		/* clear end y */
		case 0x2c/2:		/* clear start y */
			break;

		case 0x38/2:		/* blit */
			do_blit();
			break;

		case 0x3e/2:		/* clear */
			do_clear();
			break;

		default:
			logerror("%06X:write to %06X = %04X & %04x\n", activecpu_get_pc(), 0x2a0000 + 2 * offset, data, mem_mask ^ 0xffff);
			break;
	}
}


WRITE16_HANDLER( madmax_blitter_unknown_w )
{
	/* written to just before the blitter command register is written */
  logerror("%06X:write to %06X = %04X & %04X\n", activecpu_get_pc(), 0x300000 + 2 * offset, data, mem_mask ^ 0xffff);
}


READ16_HANDLER( madmax_blitter_vidparam_r )
{
	/* analog inputs seem to be hooked up here -- might not actually map to blitter */
	if (offset == 0x02/2)
		return readinputport(3);
	if (offset == 0x0e/2)
		return readinputport(4);

	/* early code polls on this bit, wants it to be 0 */
	if (offset == 0x36/2)
		return 0xffff ^ (1 << 5);

	/* log everything else */
	logerror("%06X:read from %06X\n", activecpu_get_pc(), 0x2a0000 + 2 * offset);
	return 0xffff;
}
