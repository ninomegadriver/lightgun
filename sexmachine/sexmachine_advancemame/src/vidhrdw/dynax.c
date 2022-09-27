/***************************************************************************

    The CPU controls a video blitter that can read data from the ROMs
    (instructions to draw pixel by pixel, in a compressed form) and write to
    up to 8 frame buffers.

    hanamai:
    There are four scrolling layers. Each layer consists of 2 frame buffers.
    The 2 images are interleaved to form the final picture sent to the screen.

    drgpunch:
    There are three scrolling layers. Each layer consists of 2 frame buffers.
    The 2 images are interleaved to form the final picture sent to the screen.

    mjdialq2:
    Two scrolling layers.

***************************************************************************/

#include "driver.h"
#include "includes/dynax.h"

// Log Blitter
#define VERBOSE 0


/* 0 B01234 G01234 R01234 */
PALETTE_INIT( sprtmtch )
{
	int i;

	for (i = 0;i < Machine->drv->total_colors;i++)
	{
		int x =	(color_prom[i]<<8) + color_prom[0x200+i];
		/* The bits are in reverse order! */
		int r = BITSWAP8((x >>  0) & 0x1f, 7,6,5, 0,1,2,3,4 );
		int g = BITSWAP8((x >>  5) & 0x1f, 7,6,5, 0,1,2,3,4 );
		int b = BITSWAP8((x >> 10) & 0x1f, 7,6,5, 0,1,2,3,4 );
		r =  (r << 3) | (r >> 2);
		g =  (g << 3) | (g >> 2);
		b =  (b << 3) | (b >> 2);
		palette_set_color(i,r,g,b);
	}
}

/***************************************************************************


                                Video Blitter(s)


***************************************************************************/


static int dynax_blit_scroll_x,		dynax_blit2_scroll_x;
static int dynax_blit_scroll_y,		dynax_blit2_scroll_y;
static int dynax_blit_wrap_enable,	dynax_blit2_wrap_enable;
static int blit_x,blit_y,			blit2_x,blit2_y;
static int blit_src,				blit2_src;
static int dynax_blit_dest,			dynax_blit2_dest;
static int dynax_blit_pen,			dynax_blit2_pen;
static int dynax_blit_palbank,		dynax_blit2_palbank;
static int dynax_blit_palettes,		dynax_blit2_palettes;
static int dynax_layer_enable;
static int dynax_blit_backpen;

static int hanamai_layer_half;
static int hnoridur_layer_half2;

static int extra_scroll_x,extra_scroll_y;
static int flipscreen;

#define LAYOUT_HANAMAI	0	// 4 layers, interleaved
#define LAYOUT_HNORIDUR	1	// same as hanamai but some bits are inverted and layer order is reversed
#define LAYOUT_DRGPUNCH	2	// 3 couples of layers, interleaved
#define LAYOUT_MJDIALQ2	3	// 2 layers
#define LAYOUT_JANTOUKI	4	// 2 x (4 couples of layers, interleaved)

static int layer_layout;

static void (*update_irq_func)(void);	// some games trigger IRQ at blitter end, some don't

// up to 8 layers, 2 images per layer (interleaved on screen)
static UINT8 *dynax_pixmap[8][2];



WRITE8_HANDLER( dynax_extra_scrollx_w )
{
	extra_scroll_x = data;
}

WRITE8_HANDLER( dynax_extra_scrolly_w )
{
	extra_scroll_y = data;
}


/* Destination Pen */
WRITE8_HANDLER( dynax_blit_pen_w )
{
	dynax_blit_pen = data;
#if VERBOSE
	logerror("P=%02X ",data);
#endif
}
WRITE8_HANDLER( dynax_blit2_pen_w )
{
	dynax_blit2_pen = data;
#if VERBOSE
	logerror("P'=%02X ",data);
#endif
}

/* Destination Layers */
WRITE8_HANDLER( dynax_blit_dest_w )
{
	dynax_blit_dest = data;
	if (layer_layout == LAYOUT_HNORIDUR)
		dynax_blit_dest = BITSWAP8(dynax_blit_dest ^ 0x0f, 7,6,5,4, 0,1,2,3);

#if VERBOSE
	logerror("D=%02X ",data);
#endif
}
WRITE8_HANDLER( dynax_blit2_dest_w )
{
	dynax_blit2_dest = data;
#if VERBOSE
	logerror("D'=%02X ",data);
#endif
}

/* Background Color */
WRITE8_HANDLER( dynax_blit_backpen_w )
{
	dynax_blit_backpen = data;
#if VERBOSE
	logerror("B=%02X ",data);
#endif
}

/* Layers 0&1 Palettes (Low Bits) */
WRITE8_HANDLER( dynax_blit_palette01_w )
{
	if (layer_layout == LAYOUT_HNORIDUR)
		dynax_blit_palettes = (dynax_blit_palettes & 0x00ff) | ((data&0x0f)<<12) | ((data&0xf0)<<4);
	else
		dynax_blit_palettes = (dynax_blit_palettes & 0xff00) | data;
#if VERBOSE
	logerror("P01=%02X ",data);
#endif
}
/* Layers 4&5 Palettes (Low Bits) */
WRITE8_HANDLER( dynax_blit_palette45_w )
{
	if (layer_layout == LAYOUT_HNORIDUR)
		dynax_blit2_palettes = (dynax_blit2_palettes & 0x00ff) | ((data&0x0f)<<12) | ((data&0xf0)<<4);
	else
		dynax_blit2_palettes = (dynax_blit2_palettes & 0xff00) | data;
#if VERBOSE
	logerror("P45=%02X ",data);
#endif
}

/* Layer 2&3 Palettes (Low Bits) */
WRITE8_HANDLER( dynax_blit_palette23_w )
{
	if (layer_layout == LAYOUT_HNORIDUR)
		dynax_blit_palettes = (dynax_blit_palettes & 0xff00) | ((data&0x0f)<<4) | ((data&0xf0)>>4);
	else
		dynax_blit_palettes = (dynax_blit_palettes & 0x00ff) | (data<<8);
#if VERBOSE
	logerror("P23=%02X ",data);
#endif
}
/* Layer 6&7 Palettes (Low Bits) */
WRITE8_HANDLER( dynax_blit_palette67_w )
{
	if (layer_layout == LAYOUT_HNORIDUR)
		dynax_blit2_palettes = (dynax_blit2_palettes & 0xff00) | ((data&0x0f)<<4) | ((data&0xf0)>>4);
	else
		dynax_blit2_palettes = (dynax_blit2_palettes & 0x00ff) | (data<<8);
#if VERBOSE
	logerror("P67=%02X ",data);
#endif
}



/* Layers Palettes (High Bits) */
WRITE8_HANDLER( dynax_blit_palbank_w )
{
	dynax_blit_palbank = data;
#if VERBOSE
	logerror("PB=%02X ",data);
#endif
}
WRITE8_HANDLER( dynax_blit2_palbank_w )
{
	dynax_blit2_palbank = data;
#if VERBOSE
	logerror("PB'=%02X ",data);
#endif
}

/* Which half of the layers to write two (interleaved games only) */
WRITE8_HANDLER( hanamai_layer_half_w )
{
	hanamai_layer_half = data & 1;
#if VERBOSE
	logerror("H=%02X ",data);
#endif
}

/* Write to both halves of the layers (interleaved games only) */
WRITE8_HANDLER( hnoridur_layer_half2_w )
{
	hnoridur_layer_half2 = (~data) & 1;
#if VERBOSE
	logerror("H2=%02X ",data);
#endif
}

WRITE8_HANDLER( mjdialq2_blit_dest_w )
{
	int mask = (2 >> offset);	/* 1 or 2 */

	dynax_blit_dest &= ~mask;
	if (~data & 1) dynax_blit_dest |= mask;
}

/* Layers Enable */
WRITE8_HANDLER( dynax_layer_enable_w )
{
	dynax_layer_enable = data;
#if VERBOSE
	logerror("E=%02X ",data);
#endif
}

WRITE8_HANDLER( jantouki_layer_enable_w )
{
	int mask = 1 << (7-offset);
	dynax_layer_enable = (dynax_layer_enable & ~mask) | ((data & 1) ? mask : 0);
	dynax_layer_enable |= 1;
}

WRITE8_HANDLER( mjdialq2_layer_enable_w )
{
	int mask = (2 >> offset);	/* 1 or 2 */

	dynax_layer_enable &= ~mask;
	if (~data & 1) dynax_layer_enable |= mask;
}


WRITE8_HANDLER( dynax_flipscreen_w )
{
	flipscreen = data & 1;
	if (data & ~1)
		logerror("CPU#0 PC %06X: Warning, flip screen <- %02X\n", activecpu_get_pc(), data);
#if VERBOSE
	logerror("F=%02X ",data);
#endif
}


/***************************************************************************

                            Blitter Data Format

    The blitter reads its commands from the gfx ROMs. They are
    instructions to draw an image pixel by pixel (in a compressed
    form) in a frame buffer.

    Fetch 1 Byte from the ROM:

    7654 ----   Pen to draw with
    ---- 3210   Command

    Other bytes may follow, depending on the command

    Commands:

    0       Stop.
    1-b     Draw 1-b pixels along X.
    c       Followed by 1 byte (N): draw N pixels along X.
    d       Followed by 2 bytes (X,N): skip X pixels, draw N pixels along X.
    e       ? unused
    f       Increment Y

***************************************************************************/


/* Plot a pixel (in the pixmaps specified by dynax_blit_dest) */
INLINE void blitter_plot_pixel(int layer,int mask, int x, int y, int pen, int wrap, int flags)
{
	int addr;

	if ( (y > 0xff) && (!(wrap & 2)) ) return;	// fixes mjdialq2 & mjangels title screens
	if ( (x > 0xff) && (!(wrap & 1)) ) return;

	x &= 0xff;	// confirmed by some mjdialq2 gfx and especially by mjfriday, which
				// uses the front layer to mask out the right side of the screen as
				// it draws stuff on the left, when it shows the girls scrolling
				// horizontally after you win.
	y &= 0xff;	// seems confirmed by mjdialq2 last picture of gal 6, but it breaks
				// mjdialq2 title screen so there's something we are missing. <fixed, see above>

	/* "Flip Screen" just means complement the coordinates to 255 */
	if (flipscreen)	{	x ^= 0xff;	y ^= 0xff;	}

	/* Rotate: rotation = SWAPXY + FLIPY */
	if (flags & 0x08)	{ int t = x; x = y; y = t;	}

	addr = x + (y<<8);

	switch (layer_layout)
	{
		case LAYOUT_HANAMAI:
			if (mask & 0x01)	dynax_pixmap[layer+0][hanamai_layer_half][addr] = pen;
			if (mask & 0x02)	dynax_pixmap[layer+1][hanamai_layer_half][addr] = pen;
			if (mask & 0x04)	dynax_pixmap[layer+2][hanamai_layer_half][addr] = pen;
			if (mask & 0x08)	dynax_pixmap[layer+3][hanamai_layer_half][addr] = pen;
			break;

		case LAYOUT_HNORIDUR:
			if (mask & 0x01)	dynax_pixmap[layer+0][hanamai_layer_half][addr] = pen;
			if (mask & 0x02)	dynax_pixmap[layer+1][hanamai_layer_half][addr] = pen;
			if (mask & 0x04)	dynax_pixmap[layer+2][hanamai_layer_half][addr] = pen;
			if (mask & 0x08)	dynax_pixmap[layer+3][hanamai_layer_half][addr] = pen;
			if (!hnoridur_layer_half2) break;
			if (mask & 0x01)	dynax_pixmap[layer+0][1-hanamai_layer_half][addr] = pen;
			if (mask & 0x02)	dynax_pixmap[layer+1][1-hanamai_layer_half][addr] = pen;
			if (mask & 0x04)	dynax_pixmap[layer+2][1-hanamai_layer_half][addr] = pen;
			if (mask & 0x08)	dynax_pixmap[layer+3][1-hanamai_layer_half][addr] = pen;
			break;

		case LAYOUT_JANTOUKI:
			if (mask & 0x80)	dynax_pixmap[layer+3][1][addr] = pen;
			if (mask & 0x40)	dynax_pixmap[layer+3][0][addr] = pen;
		case LAYOUT_DRGPUNCH:
			if (mask & 0x20)	dynax_pixmap[layer+2][1][addr] = pen;
			if (mask & 0x10)	dynax_pixmap[layer+2][0][addr] = pen;
			if (mask & 0x08)	dynax_pixmap[layer+1][1][addr] = pen;
			if (mask & 0x04)	dynax_pixmap[layer+1][0][addr] = pen;
			if (mask & 0x02)	dynax_pixmap[layer+0][1][addr] = pen;
			if (mask & 0x01)	dynax_pixmap[layer+0][0][addr] = pen;
			break;

		case LAYOUT_MJDIALQ2:
			if (mask & 0x01)	dynax_pixmap[layer+0][0][addr] = pen;
			if (mask & 0x02)	dynax_pixmap[layer+1][0][addr] = pen;
			break;
	}
}


static int blitter_drawgfx( int layer, int mask, int gfx, int src, int pen, int x, int y, int wrap, int flags )
{
	UINT8 cmd;

	UINT8 *ROM		=	memory_region( gfx );
	size_t   ROM_size	=	memory_region_length( gfx );

	int sx;

	if (layer_layout == LAYOUT_HNORIDUR)	// e.g. yarunara
		pen = ((pen >> 4) & 0xf) | ( (mask & 0x10) ? ((pen & 0x08) << 1) : 0 );
	else
		pen = (pen >> 4) & 0xf;

if (flags & 0xf4) ui_popup("flags %02x",flags);
	if ( flags & 1 )
	{
		int start,len;

		/* Clear the buffer(s) starting from the given scanline and exit */

		int addr = x + (y<<8);

		if (flipscreen)
			start = 0;
		else
			start = addr;

		len = 0x10000 - addr;

		switch (layer_layout)
		{
			case LAYOUT_HANAMAI:
				if (mask & 0x01)	memset(&dynax_pixmap[layer+0][0][start],pen,len);
				if (mask & 0x01)	memset(&dynax_pixmap[layer+0][1][start],pen,len);
				if (mask & 0x02)	memset(&dynax_pixmap[layer+1][0][start],pen,len);
				if (mask & 0x02)	memset(&dynax_pixmap[layer+1][1][start],pen,len);
				if (mask & 0x04)	memset(&dynax_pixmap[layer+2][0][start],pen,len);
				if (mask & 0x04)	memset(&dynax_pixmap[layer+2][1][start],pen,len);
				if (mask & 0x08)	memset(&dynax_pixmap[layer+3][0][start],pen,len);
				if (mask & 0x08)	memset(&dynax_pixmap[layer+3][1][start],pen,len);
				break;

			case LAYOUT_HNORIDUR:
				if (mask & 0x01)	memset(&dynax_pixmap[layer+0][hanamai_layer_half][start],pen,len);
				if (mask & 0x02)	memset(&dynax_pixmap[layer+1][hanamai_layer_half][start],pen,len);
				if (mask & 0x04)	memset(&dynax_pixmap[layer+2][hanamai_layer_half][start],pen,len);
				if (mask & 0x08)	memset(&dynax_pixmap[layer+3][hanamai_layer_half][start],pen,len);
				if (!hnoridur_layer_half2) break;
				if (mask & 0x01)	memset(&dynax_pixmap[layer+0][1-hanamai_layer_half][start],pen,len);
				if (mask & 0x02)	memset(&dynax_pixmap[layer+1][1-hanamai_layer_half][start],pen,len);
				if (mask & 0x04)	memset(&dynax_pixmap[layer+2][1-hanamai_layer_half][start],pen,len);
				if (mask & 0x08)	memset(&dynax_pixmap[layer+3][1-hanamai_layer_half][start],pen,len);
				break;

			case LAYOUT_JANTOUKI:
				if (mask & 0x80)	memset(&dynax_pixmap[layer+3][1][start],pen,len);
				if (mask & 0x40)	memset(&dynax_pixmap[layer+3][0][start],pen,len);
			case LAYOUT_DRGPUNCH:
				if (mask & 0x20)	memset(&dynax_pixmap[layer+2][1][start],pen,len);
				if (mask & 0x10)	memset(&dynax_pixmap[layer+2][0][start],pen,len);
				if (mask & 0x08)	memset(&dynax_pixmap[layer+1][1][start],pen,len);
				if (mask & 0x04)	memset(&dynax_pixmap[layer+1][0][start],pen,len);
				if (mask & 0x02)	memset(&dynax_pixmap[layer+0][1][start],pen,len);
				if (mask & 0x01)	memset(&dynax_pixmap[layer+0][0][start],pen,len);
				break;

			case LAYOUT_MJDIALQ2:
				if (mask & 0x01)	memset(&dynax_pixmap[layer+0][0][start],pen,len);
				if (mask & 0x02)	memset(&dynax_pixmap[layer+1][0][start],pen,len);
				break;
		}

		return src;
	}

	sx = x;

	for ( ;; )
	{
		if (src >= ROM_size)
		{
ui_popup("GFXROM OVER %08x",src);
			return src;
		}
		cmd = ROM[src++];
		if (!(flags & 0x02))	// Ignore the pens specified in ROM, draw everything with the pen supplied as parameter
			pen = (pen & 0xf0) | ((cmd & 0xf0)>>4);
		cmd = (cmd & 0x0f);

		switch (cmd)
		{
		case 0xf:	// Increment Y
			/* Rotate: rotation = SWAPXY + FLIPY */
			if (flags & 0x08)
				y--;
			else
				y++;
			x = sx;
			break;

		case 0xe:	// unused ? was "change dest mask" in the "rev1" blitter
			ui_popup("Blitter unknown command %06X: %02X\n", src-1, cmd);

		case 0xd:	// Skip X pixels
			if (src >= ROM_size)
			{
ui_popup("GFXROM OVER %08x",src);
				return src;
			}
			x = sx + ROM[src++];
			/* fall through into next case */

		case 0xc:	// Draw N pixels
			if (src >= ROM_size)
			{
ui_popup("GFXROM OVER %08x",src);
				return src;
			}
			cmd = ROM[src++];
			/* fall through into next case */

		case 0xb:
		case 0xa:
		case 0x9:
		case 0x8:
		case 0x7:
		case 0x6:
		case 0x5:
		case 0x4:
		case 0x3:
		case 0x2:
		case 0x1:	// Draw N pixels
			while (cmd--)
				blitter_plot_pixel(layer, mask, x++, y, pen, wrap, flags);
			break;

		case 0x0:	// Stop
			return src;
		}
	}
}



static void dynax_blitter_start(int flags)
{
#if VERBOSE
	logerror("XY=%X,%X SRC=%X BLIT=%X\n",blit_x,blit_y,blit_src,flags);
#endif

	int blit_newsrc =
		blitter_drawgfx(
			0,						// layer
			dynax_blit_dest,		// layer mask
			REGION_GFX1,			// rom region
			blit_src & 0x3fffff,	// rom address
			dynax_blit_pen,			// pen
			blit_x, blit_y,			// x,y
			dynax_blit_wrap_enable,	// wrap around
			flags					// flags
		);

	blit_src	=	(blit_src		&	~0x3fffff) |
					(blit_newsrc	&	 0x3fffff) ;

	/* Generate an IRQ */
	if (update_irq_func)
	{
		dynax_blitter_irq = 1;
		update_irq_func();
	}
}

static void jantouki_blitter_start(int flags)
{
#if VERBOSE
	logerror("XY=%X,%X SRC=%X BLIT=%X\n",blit_x,blit_y,blit_src,flags);
#endif

	int blit_newsrc =
		blitter_drawgfx(
			0,						// layer
			dynax_blit_dest,		// layer mask
			REGION_GFX1,			// rom region
			blit_src & 0x3fffff,	// rom address
			dynax_blit_pen,			// pen
			blit_x, blit_y,			// x,y
			dynax_blit_wrap_enable,	// wrap around
			flags					// flags
		);

	blit_src	=	(blit_src		&	~0x3fffff) |
					(blit_newsrc	&	 0x3fffff) ;

	/* Generate an IRQ */
	if (update_irq_func)
	{
		dynax_blitter_irq = 1;
		update_irq_func();
	}
}

static void jantouki_blitter2_start(int flags)
{
#if VERBOSE
	logerror("XY'=%X,%X SRC'=%X BLIT'=%02X\n",blit2_x,blit2_y,blit2_src,flags);
#endif

	int blit2_newsrc =
		blitter_drawgfx(
			4,							// layer
			dynax_blit2_dest,			// layer mask
			REGION_GFX2,				// rom region
			blit2_src & 0x3fffff,		// rom address
			dynax_blit2_pen,			// pen
			blit2_x, blit2_y,			// x,y
			dynax_blit2_wrap_enable,	// wrap around
			flags						// flags
		);

	blit2_src	=	(blit2_src		&	~0x3fffff) |
					(blit2_newsrc	&	 0x3fffff) ;

	/* Generate an IRQ */
	if (update_irq_func)
	{
		dynax_blitter2_irq = 1;
		update_irq_func();
	}
}



WRITE8_HANDLER( dynax_blit_scroll_w )
{
	switch( blit_src & 0xc00000 )
	{
		case 0x000000:	dynax_blit_scroll_x = data;
						#if VERBOSE
						logerror("SX=%02X ",data);
						#endif
						break;
		case 0x400000:	dynax_blit_scroll_y = data;
						#if VERBOSE
						logerror("SY=%02X ",data);
						#endif
						break;
		case 0x800000:
		case 0xc00000:	dynax_blit_wrap_enable = data;
						#if VERBOSE
						logerror("WE=%02X ",data);
						#endif
						break;
	}
}

WRITE8_HANDLER( dynax_blit2_scroll_w )
{
	switch( blit2_src & 0xc00000 )
	{
		case 0x000000:	dynax_blit2_scroll_x = data;
						#if VERBOSE
						logerror("SX'=%02X ",data);
						#endif
						break;
		case 0x400000:	dynax_blit2_scroll_y = data;
						#if VERBOSE
						logerror("SY'=%02X ",data);
						#endif
						break;
		case 0x800000:
		case 0xc00000:	dynax_blit2_wrap_enable = data;
						#if VERBOSE
						logerror("WE'=%02X ",data);
						#endif
						break;
	}
}



WRITE8_HANDLER( dynax_blitter_rev2_w )
{
	switch (offset)
	{
		case 0: dynax_blitter_start(data); break;
		case 1:	blit_x		=	data; break;
		case 2: blit_y		=	data; break;
		case 3:	blit_src	=	(blit_src & 0xffff00) | (data << 0); break;
		case 4: blit_src	=	(blit_src & 0xff00ff) | (data << 8); break;
		case 5: blit_src	=	(blit_src & 0x00ffff) | (data <<16); break;
		case 6: dynax_blit_scroll_w(0,data); break;
	}
}


WRITE8_HANDLER( jantouki_blitter_rev2_w )
{
	switch (offset)
	{
		case 0: jantouki_blitter_start(data); break;
		case 1:	blit_x		=	data; break;
		case 2: blit_y		=	data; break;
		case 3:	blit_src	=	(blit_src & 0xffff00) | (data << 0); break;
		case 4: blit_src	=	(blit_src & 0xff00ff) | (data << 8); break;
		case 5: blit_src	=	(blit_src & 0x00ffff) | (data <<16); break;
		case 6: dynax_blit_scroll_w(0,data); break;
	}
}

WRITE8_HANDLER( jantouki_blitter2_rev2_w )
{
	switch (offset)
	{
		case 0: jantouki_blitter2_start(data); break;
		case 1:	blit2_x		=	data; break;
		case 2: blit2_y		=	data; break;
		case 3:	blit2_src	=	(blit2_src & 0xffff00) | (data << 0); break;
		case 4: blit2_src	=	(blit2_src & 0xff00ff) | (data << 8); break;
		case 5: blit2_src	=	(blit2_src & 0x00ffff) | (data <<16); break;
		case 6: dynax_blit2_scroll_w(0,data); break;
	}
}


/***************************************************************************


                                Video Init


***************************************************************************/

static const int *priority_table;
//                           0       1       2       3       4       5       6       7
static const int priority_hnoridur[8] = { 0x0231, 0x2103, 0x3102, 0x2031, 0x3021, 0x1302, 0x2310, 0x1023 };
static const int priority_mcnpshnt[8] = { 0x3210, 0x2103, 0x3102, 0x2031, 0x3021, 0x1302, 0x2310, 0x1023 };
static const int priority_mjelctrn[8] = { 0x0231, 0x0321, 0x2031, 0x2301, 0x3021, 0x3201 ,0x0000, 0x0000 };	// this game doesn't use (hasn't?) layer 1

static void Video_Reset(void)
{
	extra_scroll_x = 0;
	extra_scroll_y = 0;

	hnoridur_layer_half2 = 0;

	update_irq_func = sprtmtch_update_irq;
}

VIDEO_START( hanamai )
{
	dynax_pixmap[0][0] = auto_malloc(256*256);
	dynax_pixmap[0][1] = auto_malloc(256*256);
	dynax_pixmap[1][0] = auto_malloc(256*256);
	dynax_pixmap[1][1] = auto_malloc(256*256);
	dynax_pixmap[2][0] = auto_malloc(256*256);
	dynax_pixmap[2][1] = auto_malloc(256*256);
	dynax_pixmap[3][0] = auto_malloc(256*256);
	dynax_pixmap[3][1] = auto_malloc(256*256);

	Video_Reset();
	layer_layout = LAYOUT_HANAMAI;

	return 0;
}

VIDEO_START( hnoridur )
{
	dynax_pixmap[0][0] = auto_malloc(256*256);
	dynax_pixmap[0][1] = auto_malloc(256*256);
	dynax_pixmap[1][0] = auto_malloc(256*256);
	dynax_pixmap[1][1] = auto_malloc(256*256);
	dynax_pixmap[2][0] = auto_malloc(256*256);
	dynax_pixmap[2][1] = auto_malloc(256*256);
	dynax_pixmap[3][0] = auto_malloc(256*256);
	dynax_pixmap[3][1] = auto_malloc(256*256);

	Video_Reset();
	layer_layout = LAYOUT_HNORIDUR;

	priority_table = priority_hnoridur;

	return 0;
}

VIDEO_START( mcnpshnt )
{
	if (video_start_hnoridur())	return 1;
	priority_table = priority_mcnpshnt;
	return 0;
}

VIDEO_START( sprtmtch )
{
	dynax_pixmap[0][0] = auto_malloc(256*256);
	dynax_pixmap[0][1] = auto_malloc(256*256);
	dynax_pixmap[1][0] = auto_malloc(256*256);
	dynax_pixmap[1][1] = auto_malloc(256*256);
	dynax_pixmap[2][0] = auto_malloc(256*256);
	dynax_pixmap[2][1] = auto_malloc(256*256);

	Video_Reset();
	layer_layout = LAYOUT_DRGPUNCH;

	return 0;
}

VIDEO_START( jantouki )
{
	dynax_pixmap[0][0] = auto_malloc(256*256);
	dynax_pixmap[0][1] = auto_malloc(256*256);
	dynax_pixmap[1][0] = auto_malloc(256*256);
	dynax_pixmap[1][1] = auto_malloc(256*256);
	dynax_pixmap[2][0] = auto_malloc(256*256);
	dynax_pixmap[2][1] = auto_malloc(256*256);
	dynax_pixmap[3][0] = auto_malloc(256*256);
	dynax_pixmap[3][1] = auto_malloc(256*256);
	dynax_pixmap[4][0] = auto_malloc(256*256);
	dynax_pixmap[4][1] = auto_malloc(256*256);
	dynax_pixmap[5][0] = auto_malloc(256*256);
	dynax_pixmap[5][1] = auto_malloc(256*256);
	dynax_pixmap[6][0] = auto_malloc(256*256);
	dynax_pixmap[6][1] = auto_malloc(256*256);
	dynax_pixmap[7][0] = auto_malloc(256*256);
	dynax_pixmap[7][1] = auto_malloc(256*256);

	Video_Reset();
	layer_layout = LAYOUT_JANTOUKI;

	update_irq_func = jantouki_update_irq;

	return 0;
}

VIDEO_START( mjdialq2 )
{
	dynax_pixmap[0][0] = auto_malloc(256*256);
	dynax_pixmap[1][0] = auto_malloc(256*256);

	Video_Reset();
	layer_layout = LAYOUT_MJDIALQ2;

	update_irq_func = 0;

	return 0;
}

VIDEO_START( mjelctrn )
{
	if (video_start_hnoridur())	return 1;

	priority_table = priority_mjelctrn;
	update_irq_func = mjelctrn_update_irq;

	return 0;
}

VIDEO_START( neruton )
{
	if (video_start_hnoridur())	return 1;

//  priority_table = priority_mjelctrn;
	update_irq_func = neruton_update_irq;

	return 0;
}

/***************************************************************************


                                Screen Drawing


***************************************************************************/

void hanamai_copylayer(mame_bitmap *bitmap,const rectangle *cliprect,int i)
{
	int color;
	int scrollx,scrolly;

	switch ( i )
	{
		case 0:	color = (dynax_blit_palettes >>  0) & 0xf;	break;
		case 1:	color = (dynax_blit_palettes >>  4) & 0xf;	break;
		case 2:	color = (dynax_blit_palettes >>  8) & 0xf;	break;
		case 3:	color = (dynax_blit_palettes >> 12) & 0xf;	break;
		default:	return;
	}

	color += (dynax_blit_palbank & 0x0f) * 16;

	scrollx = dynax_blit_scroll_x;
	scrolly = dynax_blit_scroll_y;

	if (	(layer_layout == LAYOUT_HANAMAI		&&	i == 1)	||
			(layer_layout == LAYOUT_HNORIDUR	&&	i == 1)	)
	{
		scrollx = extra_scroll_x;
		scrolly = extra_scroll_y;
	}

	{
		int dy,length,pen,offs;
		UINT8 *src1 = dynax_pixmap[i][1];
		UINT8 *src2 = dynax_pixmap[i][0];

		int palbase = 16*color;
		offs = 0;

		for (dy = 0; dy < 256; dy++)
		{
			UINT16 *dst;
			UINT16 *dstbase = (UINT16 *)bitmap->base + ((dy - scrolly) & 0xff) * bitmap->rowpixels;

			length = scrollx;
			dst = dstbase + 2*(256 - length);
			while (length--)
			{
				pen = *(src1++);
				if (pen) *dst     = palbase + pen;
				pen = *(src2++);
				if (pen) *(dst+1) = palbase + pen;
				dst += 2;
			}

			length = 256 - scrollx;
			dst = dstbase;
			while (length--)
			{
				pen = *(src1++);
				if (pen) *dst     = palbase + pen;
				pen = *(src2++);
				if (pen) *(dst+1) = palbase + pen;
				dst += 2;
			}
		}
	}
}


void jantouki_copylayer(mame_bitmap *bitmap,const rectangle *cliprect,int i, int y)
{
	int color,scrollx,scrolly,palettes,palbank;

	if (i < 4)	{		scrollx		=	dynax_blit_scroll_x;
						scrolly		=	dynax_blit_scroll_y;
						palettes	=	dynax_blit_palettes;
						palbank		=	dynax_blit_palbank;		}
	else		{		scrollx		=	dynax_blit2_scroll_x;
						scrolly		=	dynax_blit2_scroll_y;
						palettes	=	dynax_blit2_palettes;
						palbank		=	dynax_blit2_palbank;	}

	switch ( i % 4 )
	{
		case 0:	color = ((palettes  >> 12) & 0xf) + ((palbank & 1) << 4);	break;
		case 1:	color = ((palettes  >>  8) & 0xf) + ((palbank & 1) << 4);	break;
		case 2:	color = ((palettes  >>  4) & 0xf) + ((palbank & 1) << 4);	break;
		case 3:	color = ((palettes  >>  0) & 0xf) + ((palbank & 1) << 4);	break;
		default: return;
	}

	{
		int dy,length,pen,offs;
		UINT8 *src1 = dynax_pixmap[i][1];
		UINT8 *src2 = dynax_pixmap[i][0];

		int palbase = 16*color;
		offs = 0;

		for (dy = 0; dy < 256; dy++)
		{
			int sy = ((dy - scrolly) & 0xff) + y;
			UINT16 *dst;
			UINT16 *dstbase = (UINT16 *)bitmap->base + sy* bitmap->rowpixels;

			if ((sy < cliprect->min_y) || (sy > cliprect->max_y))
			{
				src1+=256;
				src2+=256;
				continue;
			}

			length = scrollx;
			dst = dstbase + 2*(256 - length);
			while (length--)
			{
				pen = *(src1++);
				if (pen) *dst     = palbase + pen;
				pen = *(src2++);
				if (pen) *(dst+1) = palbase + pen;
				dst += 2;
			}

			length = 256 - scrollx;
			dst = dstbase;
			while (length--)
			{
				pen = *(src1++);
				if (pen) *dst     = palbase + pen;
				pen = *(src2++);
				if (pen) *(dst+1) = palbase + pen;
				dst += 2;
			}
		}
	}
}


void mjdialq2_copylayer(mame_bitmap *bitmap,const rectangle *cliprect,int i)
{
	int color;
	int scrollx,scrolly;

	switch ( i )
	{
		case 0:	color = (dynax_blit_palettes >>  4) & 0xf;	break;
		case 1:	color = (dynax_blit_palettes >>  0) & 0xf;	break;
		default:	return;
	}

	color += (dynax_blit_palbank & 1) * 16;

	scrollx = dynax_blit_scroll_x;
	scrolly = dynax_blit_scroll_y;

	{
		int dy,length,pen,offs;
		UINT8 *src = dynax_pixmap[i][0];

		int palbase = 16*color;
		offs = 0;

		for (dy = 0; dy < 256; dy++)
		{
			UINT16 *dst;
			UINT16 *dstbase = (UINT16 *)bitmap->base + ((dy - scrolly) & 0xff) * bitmap->rowpixels;

			length = scrollx;
			dst = dstbase + 256 - length;
			while (length--)
			{
				pen = *(src++);
				if (pen) *dst = palbase + pen;
				dst++;
			}

			length = 256 - scrollx;
			dst = dstbase;
			while (length--)
			{
				pen = *(src++);
				if (pen) *dst = palbase + pen;
				dst++;
			}
		}
	}
}



static int hanamai_priority;

WRITE8_HANDLER( hanamai_priority_w )
{
	hanamai_priority = data;
}


static int debug_mask(void)
{
#ifdef MAME_DEBUG
	int msk = 0;
	if (code_pressed(KEYCODE_Z))
	{
		if (code_pressed(KEYCODE_Q))	msk |= 0x01;	// layer 0
		if (code_pressed(KEYCODE_W))	msk |= 0x02;	// layer 1
		if (code_pressed(KEYCODE_E))	msk |= 0x04;	// layer 2
		if (code_pressed(KEYCODE_R))	msk |= 0x08;	// layer 3
		if (code_pressed(KEYCODE_A))	msk |= 0x10;	// layer 4
		if (code_pressed(KEYCODE_S))	msk |= 0x20;	// layer 5
		if (code_pressed(KEYCODE_D))	msk |= 0x40;	// layer 6
		if (code_pressed(KEYCODE_F))	msk |= 0x80;	// layer 7
		if (msk != 0)	return msk;
	}
#endif
	return -1;
}

/*  A primitive gfx viewer:

    T          -  Toggle viewer
    I,O        -  Change palette (-,+)
    J,K & N,M  -  Change "tile"  (-,+, slow & fast)
    R          -  move "tile" to the next 1/8th of the gfx  */
static int debug_viewer(mame_bitmap *bitmap,const rectangle *cliprect)
{
#ifdef MAME_DEBUG
	static int toggle;
	if (code_pressed_memory(KEYCODE_T))	toggle = 1-toggle;
	if (toggle)	{
		UINT8 *RAM	=	memory_region( REGION_GFX1 );
		size_t size		=	memory_region_length( REGION_GFX1 );
		static int i = 0, c = 0, r = 0;

		if (code_pressed_memory(KEYCODE_I))	c = (c-1) & 0x1f;
		if (code_pressed_memory(KEYCODE_O))	c = (c+1) & 0x1f;
		if (code_pressed_memory(KEYCODE_R))	{ r = (r+1) & 0x7;	i = size/8 * r; }
		if (code_pressed(KEYCODE_M) | code_pressed_memory(KEYCODE_K))	{
			while( i < size && RAM[i] ) i++;		while( i < size && !RAM[i] ) i++;	}
		if (code_pressed(KEYCODE_N) | code_pressed_memory(KEYCODE_J))	{
			if (i >= 2) i-=2;	while( i > 0 && RAM[i] ) i--;	i++;	}

		dynax_blit_palettes = (c & 0xf) * 0x111;
		dynax_blit_palbank  = (c >>  4) & 1;

		fillbitmap(bitmap,Machine->pens[0],cliprect);
		memset(dynax_pixmap[0][0],0,sizeof(UINT8)*0x100*0x100);
		if (layer_layout != LAYOUT_MJDIALQ2)
			memset(dynax_pixmap[0][1],0,sizeof(UINT8)*0x100*0x100);
		for (hanamai_layer_half = 0; hanamai_layer_half < 2; hanamai_layer_half++)
			blitter_drawgfx(0,1,REGION_GFX1,i,0,cliprect->min_x,cliprect->min_y,3,0);
		if (layer_layout != LAYOUT_MJDIALQ2)	hanamai_copylayer(bitmap, cliprect, 0);
		else									mjdialq2_copylayer(bitmap,cliprect, 0);
		ui_popup("%06X C%02X",i,c);

		return 1;
	}
#endif
	return 0;
}



VIDEO_UPDATE( hanamai )
{
	int layers_ctrl = ~dynax_layer_enable;
	int lay[4];

	if (debug_viewer(bitmap,cliprect))	return;
	layers_ctrl &= debug_mask();

	fillbitmap(
		bitmap,
		Machine->pens[(dynax_blit_backpen & 0xff) + (dynax_blit_palbank & 1) * 256],
		cliprect);

	/* bit 4 = display enable? */
	if (!(hanamai_priority & 0x10)) return;

	switch (hanamai_priority)
	{
		default:	ui_popup("unknown priority %02x",hanamai_priority);
		case 0x10:	lay[0] = 0; lay[1] = 1; lay[2] = 2; lay[3] = 3; break;
		case 0x11:	lay[0] = 0; lay[1] = 3; lay[2] = 2; lay[3] = 1; break;
		case 0x12:	lay[0] = 0; lay[1] = 1; lay[2] = 3; lay[3] = 2; break;
		case 0x13:	lay[0] = 0; lay[1] = 3; lay[2] = 1; lay[3] = 2; break;
		case 0x14:	lay[0] = 0; lay[1] = 2; lay[2] = 1; lay[3] = 3; break;
		case 0x15:	lay[0] = 0; lay[1] = 2; lay[2] = 3; lay[3] = 1; break;
	}

	if (layers_ctrl & (1 << lay[0]))	hanamai_copylayer( bitmap, cliprect, lay[0] );
	if (layers_ctrl & (1 << lay[1]))	hanamai_copylayer( bitmap, cliprect, lay[1] );
	if (layers_ctrl & (1 << lay[2]))	hanamai_copylayer( bitmap, cliprect, lay[2] );
	if (layers_ctrl & (1 << lay[3]))	hanamai_copylayer( bitmap, cliprect, lay[3] );
}


VIDEO_UPDATE( hnoridur )
{
	int layers_ctrl = ~BITSWAP8(hanamai_priority, 7,6,5,4, 0,1,2,3);
	int lay[4];
	int pri;

	if (debug_viewer(bitmap,cliprect))	return;
	layers_ctrl &= debug_mask();

	fillbitmap(
		bitmap,
		Machine->pens[(dynax_blit_backpen & 0xff) + (dynax_blit_palbank & 0x0f) * 256],
		cliprect);

	pri = hanamai_priority >> 4;

	if (pri > 7)
	{
		ui_popup("unknown priority %02x",hanamai_priority);
		pri = 0;
	}

	pri = priority_table[pri];
	lay[0] = (pri >> 12) & 3;
	lay[1] = (pri >>  8) & 3;
	lay[2] = (pri >>  4) & 3;
	lay[3] = (pri >>  0) & 3;

	if (layers_ctrl & (1 << lay[0]))	hanamai_copylayer( bitmap, cliprect, lay[0] );
	if (layers_ctrl & (1 << lay[1]))	hanamai_copylayer( bitmap, cliprect, lay[1] );
	if (layers_ctrl & (1 << lay[2]))	hanamai_copylayer( bitmap, cliprect, lay[2] );
	if (layers_ctrl & (1 << lay[3]))	hanamai_copylayer( bitmap, cliprect, lay[3] );
}


VIDEO_UPDATE( sprtmtch )
{
	int layers_ctrl = ~dynax_layer_enable;

	if (debug_viewer(bitmap,cliprect))	return;
	layers_ctrl &= debug_mask();

	fillbitmap(
		bitmap,
		Machine->pens[(dynax_blit_backpen & 0xff) + (dynax_blit_palbank & 1) * 256],
		cliprect);

	if (layers_ctrl & 1)	hanamai_copylayer( bitmap, cliprect, 0 );
	if (layers_ctrl & 2)	hanamai_copylayer( bitmap, cliprect, 1 );
	if (layers_ctrl & 4)	hanamai_copylayer( bitmap, cliprect, 2 );
}

VIDEO_UPDATE( jantouki )
{
	rectangle cliprect1 = Machine->visible_area, cliprect2 = Machine->visible_area;
	int middle_y = (Machine->visible_area.min_y + Machine->visible_area.max_y + 1) / 2;

	int layers_ctrl = dynax_layer_enable;

	if (debug_viewer(bitmap,cliprect))	return;
	layers_ctrl &= debug_mask();

	fillbitmap(
		bitmap,
		Machine->pens[(dynax_blit_backpen & 0xff) + (dynax_blit_palbank & 1) * 256],
		cliprect);

	cliprect1.max_y = middle_y - 1;
	cliprect2.min_y = middle_y;

//  if (layers_ctrl & 0x01) jantouki_copylayer( bitmap, &cliprect2, 3, middle_y-16 );
	if (layers_ctrl & 0x02)	jantouki_copylayer( bitmap, &cliprect2, 2, middle_y-16 );
	if (layers_ctrl & 0x04)	jantouki_copylayer( bitmap, &cliprect2, 1, middle_y-16 );
	if (layers_ctrl & 0x08)	jantouki_copylayer( bitmap, &cliprect2, 0, middle_y-16 );

	if (layers_ctrl & 0x01)	jantouki_copylayer( bitmap, &cliprect1, 3, 0 );
	if (layers_ctrl & 0x10)	jantouki_copylayer( bitmap, &cliprect1, 7, 0 );
	if (layers_ctrl & 0x20)	jantouki_copylayer( bitmap, &cliprect1, 6, 0 );
	if (layers_ctrl & 0x40)	jantouki_copylayer( bitmap, &cliprect1, 5, 0 );
	if (layers_ctrl & 0x80)	jantouki_copylayer( bitmap, &cliprect1, 4, 0 );
}


VIDEO_UPDATE( mjdialq2 )
{
	int layers_ctrl = ~dynax_layer_enable;

	if (debug_viewer(bitmap,cliprect))	return;
	layers_ctrl &= debug_mask();

	fillbitmap(
		bitmap,
		Machine->pens[(dynax_blit_backpen & 0xff) + (dynax_blit_palbank & 1) * 256],
		cliprect);

	if (layers_ctrl & 1)	mjdialq2_copylayer( bitmap, cliprect, 0 );
	if (layers_ctrl & 2)	mjdialq2_copylayer( bitmap, cliprect, 1 );
}
