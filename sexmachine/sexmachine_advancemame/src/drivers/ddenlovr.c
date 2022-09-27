/***************************************************************************

Some Dynax/Nakanihon games using the third version of their blitter

Driver by Nicola Salmoria based on preliminary work by Luca Elia

the driver is a complete mess ATM.

Hardware:
CPU: Z80 or 68000
Sound: (AY-3-8910) + YM2413 + MSM6295
Other: Real Time Clock (Oki MSM6242B or 72421B)

---------------------------------------------------------------------------------------------------------------------------
Year + Game                 Board           CPU     Sound                       Custom                              Notes
---------------------------------------------------------------------------------------------------------------------------
92 Monkey Mole Panic                        2xZ80   AY8910 + YM2413 + M6295     Dynax NL-001                        8251
93 Animalandia Jr.                          2xZ80   AY8910 + YM2413 + M6295     NL-001, NL-003 (x2), NL-004 (x2)    TMP82C51 (USART)
93 Quiz Channel Question    N7311208L1-2    Z80              YM2413 + M6295     NAKANIHON NL-002
93 The First Funky Fighter  N7403208L-2     2xZ80   YM2149 + YM2413 + M6295     NL-001, NL-002, NL-005
94 Mahjong Mysterious World D7107058L1-1    Z80     YM2149 + YM2413 + M6295
94 Quiz 365                                 68000   AY8910 + YM2413 + M6295
94 Rong Rong                                Z80              YM2413 + M6295     NAKANIHON NL-002
95 Mahjong Dai Chuuka Ken   D11107218L1     Z80     AY8910 + YM2413 + M6295     70C160F009
95 Nettoh Quiz Champion                     68000   AY8910 + YM2413 + M6295
96 Don Den Lover Vol 1      D11309208L1     68000   AY8910 + YM2413 + M6295     NAKANIHON NL-005
96 Hana Kanzashi                            Z80              YM2413 + M6295     70C160F011?
97 Hana Kagerou                             Z80              YM2413 + M6295     70C160F011
98 Mahjong Chuukanejyo      D11107218L1     Z80     AY8910 + YM2413 + M6295     70C160F009
98 Mahjong Reach Ippatsu                    Z80              YM2413 + M6295     70C160F011
---------------------------------------------------------------------------------------------------------------------------

Notes:

- the zooming Dynax logo in ddenlovr would flicker because the palette is
  updated one frame after the bitmap. This is fixed using a framebuffer but I
  don't think it's correct.


TODO:

- verify whether clip_width/height is actually clip_x_end/y_end
  (this also applies to rectangles drawing, command 1c):
  the girl in hanakanz divided in 3 chunks (during the intro when bet is on)
  is ok with the latter setting; scene 2 of gal 1 check in hkagerou (press 1 in scene 1)
  is maybe clipped too much this way and hints at the former setting being correct.
  There is an #if to switch between the two modes in do_plot.

- ddenlovr: understand the extra commands for the blitter compressed data,
  used only by this game.

- ddenlovr: sometimes the colors of the girl in the presentation before the
  beginning of a stage are wrong, and they correct themselves when the board
  is drawn.
- The palette problems mentioned above happen in other games as well, e.g.
  quizchq attract mode.

- the registers right after the palette bank selectors (e00048-e0004f in ddenlovr)
  are not understood. They are related to the layer enable register and to the
  unknown blitter register 05.
  ddenlovr has a function at 001798 to initialize these four registers. It uses
  a table with 7 possible different layouts:
  0f 0f 3f cf
  4f 0f af 1f
  0f 0f 6f 9f
  0f 0f 0f ff
  0f 0f 7f 8f
  0f 0f cf 3f
  0f 0f 8f 7f
  the table is copied to e00048-e0004f and is also used to create a 16-entry
  array used to feed blitter register 05. Every element of the array is the OR
  of the values in the table above corresponding to bits set in the layer enable
  register. Note that in the table above the top 4 bits are split among the four
  entries.

- The meaning of blitter commands 43 and 8c is not understood.

- quizchq: it locks up, some samples are played at the wrong pitch

- quiz365 protection

- NVRAM

***************************************************************************/

#include "driver.h"
#include "cpu/z80/z80.h"
#include "sound/ay8910.h"
#include "sound/okim6295.h"
#include "sound/2413intf.h"
#include "profiler.h"
#include <time.h>

static UINT8 *pixmap[8];
static mame_bitmap *framebuffer;
static int extra_layers;


/***************************************************************************

                        Blitter Data Format

The gfx data is a bitstream. Command size is always 3 bits, argument size
can be from 1 to 8 bits (up to 16 bits seem to be allowed, but not used).

Data starts with an 8 bit header:
7------- not used
-654---- size-1 of arguments indicating pen number (B)
----3210 size-1 of arguments indicating number of pixels (A)

The commands are:
000 Increment Y
001 Followed by A bits (N) and by B bits (P): draw N+1 pixels using pen P
010 Followed by A bits (N) and by (N+1)*B bits: copy N+1 pixels
011 Followed by A bits (N): skip N pixels
100 not used
101 Followed by 4 bits: change argument size
110 Followed by 3 bits: change pen size
111 Stop.

The drawing operation is verified (quiz365) to modify dynax_blit_y.

***************************************************************************/

static int dynax_dest_layer;
static int dynax_blit_flip;
static int dynax_blit_x;
static int dynax_blit_y;
static int dynax_blit_address;
static int dynax_blit_pen,dynax_blit_pen_mode;
static int dynax_blitter_irq_flag,dynax_blitter_irq_enable;
static int dynax_rect_width, dynax_rect_height;
static int dynax_clip_width, dynax_clip_height;
static int dynax_line_length;
static int dynax_clip_ctrl = 0xf,dynax_clip_x,dynax_clip_y;
static int dynax_scroll[8*2];
static int dynax_priority, dynax_priority2;
static int dynax_bgcolor, dynax_bgcolor2;
static int dynax_layer_enable=0x0f, dynax_layer_enable2=0x0f;
static int dynax_palette_base[8], dynax_palette_mask[8];
static int dynax_transparency_pen[8], dynax_transparency_mask[8];
static int dynax_blit_reg;
static int dynax_blit_pen_mask = 0xff;	// not implemented


VIDEO_START(ddenlovr)
{
	int i;
	for (i = 0; i < 8; i++)
		pixmap[i] = auto_malloc(512*512);

	if (!(framebuffer = auto_bitmap_alloc(Machine->drv->screen_width,Machine->drv->screen_height))) return 1;

	extra_layers = 0;

	// older games do not set these !?
	dynax_clip_width = 0x400;
	dynax_clip_height = 0x400;

	return 0;
}

VIDEO_START(mmpanic)
{
	if (video_start_ddenlovr())
		return 1;

	extra_layers = 1;
	return 0;
}


static void dynax_flipscreen_w( UINT8 data )
{
	logerror("flipscreen = %02x (%s)\n",data,(data&1)?"off":"on");
}

static void dynax_blit_flip_w( UINT8 data )
{
	if ((data ^ dynax_blit_flip) & 0xec)
	{
#ifdef MAME_DEBUG
		ui_popup("warning dynax_blit_flip = %02x",data);
#endif
		logerror("warning dynax_blit_flip = %02x\n",data);
	}

	dynax_blit_flip = data;
}

static WRITE8_HANDLER( dynax_bgcolor_w )
{
	dynax_bgcolor = data;
}

static WRITE8_HANDLER( dynax_bgcolor2_w )
{
	dynax_bgcolor2 = data;
}

static WRITE16_HANDLER( ddenlovr_bgcolor_w )
{
	if (ACCESSING_LSB)
		dynax_bgcolor_w(offset,data);
}


static WRITE8_HANDLER( dynax_priority_w )
{
	dynax_priority = data;
}

static WRITE8_HANDLER( dynax_priority2_w )
{
	dynax_priority2 = data;
}

static WRITE16_HANDLER( ddenlovr_priority_w )
{
	if (ACCESSING_LSB)
		dynax_priority_w(offset,data);
}


static WRITE8_HANDLER( dynax_layer_enable_w )
{
	dynax_layer_enable = data;
}

static WRITE8_HANDLER( dynax_layer_enable2_w )
{
	dynax_layer_enable2 = data;
}


static WRITE16_HANDLER( ddenlovr_layer_enable_w )
{
	if (ACCESSING_LSB)
		dynax_layer_enable_w(offset,data);
}




static void do_plot(int x,int y,int pen)
{
	int addr, temp;
	int xclip, yclip;

	y &= 0x1ff;
	x &= 0x1ff;

	// swap x & y (see hanakanz gal check)
	if (dynax_blit_flip & 0x10)	{	temp = x;	x = y;	y = temp;	}

	// clipping rectangle (see hanakanz / hkagerou gal check)
#if 0
	xclip	=	(x < dynax_clip_x) || (x > dynax_clip_x+dynax_clip_width);
	yclip	=	(y < dynax_clip_y) || (y > dynax_clip_y+dynax_clip_height);
#else
	xclip	=	(x < dynax_clip_x) || (x > dynax_clip_width);
	yclip	=	(y < dynax_clip_y) || (y > dynax_clip_height);
#endif

	if (!(dynax_clip_ctrl & 1) &&  xclip) return;
	if (!(dynax_clip_ctrl & 2) && !xclip) return;
	if (!(dynax_clip_ctrl & 4) &&  yclip) return;
	if (!(dynax_clip_ctrl & 8) && !yclip) return;

	addr = 512 * y + x;

	if (dynax_dest_layer & 0x0001) pixmap[0][addr] = pen;
	if (dynax_dest_layer & 0x0002) pixmap[1][addr] = pen;
	if (dynax_dest_layer & 0x0004) pixmap[2][addr] = pen;
	if (dynax_dest_layer & 0x0008) pixmap[3][addr] = pen;

	if (!extra_layers)	return;

	if (dynax_dest_layer & 0x0100) pixmap[4][addr] = pen;
	if (dynax_dest_layer & 0x0200) pixmap[5][addr] = pen;
	if (dynax_dest_layer & 0x0400) pixmap[6][addr] = pen;
	if (dynax_dest_layer & 0x0800) pixmap[7][addr] = pen;
}


INLINE int fetch_bit(UINT8 *src_data,int src_len,int *bit_addr)
{
	int baddr = (*bit_addr)++;

	if (baddr/8 >= src_len)
	{
#ifdef MAME_DEBUG
		ui_popup("GFX ROM OVER %06x",baddr/8);
#endif
		return 1;
	}

	return (src_data[baddr / 8] >> (7 - (baddr & 7))) & 1;
}

INLINE int fetch_word(UINT8 *src_data,int src_len,int *bit_addr,int word_len)
{
	int res = 0;

	while (word_len--)
	{
		res = (res << 1) | fetch_bit(src_data,src_len,bit_addr);
	}
	return res;
}



INLINE void log_draw_error(int src, int cmd)
{
	ui_popup("%06x: warning unknown pixel command %02x",src,cmd);
	logerror("%06x: warning unknown pixel command %02x\n",src,cmd);
}

/*  Copy from ROM
    initialized arguments are
    0D/0E/0F source data pointer
    14 X
    02 Y
    00 dest layer
    05 unknown, related to layer
    04 blit_pen
    06 blit_pen_mode (replace values stored in ROM)
*/

static int blit_draw(int src,int sx)
{
	UINT8 *src_data = memory_region(REGION_GFX1);
	int src_len = memory_region_length(REGION_GFX1);
	int bit_addr = src * 8;	/* convert to bit address */
	int pen_size, arg_size, cmd;
	int x;
	int xinc = (dynax_blit_flip & 1) ? -1 : 1;
	int yinc = (dynax_blit_flip & 2) ? -1 : 1;

	pen_size = fetch_word(src_data,src_len,&bit_addr,4) + 1;
	arg_size = fetch_word(src_data,src_len,&bit_addr,4) + 1;

#ifdef MAME_DEBUG
	if (pen_size > 4 || arg_size > 8)
		ui_popup("warning: pen_size %d arg_size %d",pen_size,arg_size);
#endif

	x = sx;

	for (;;)
	{
		cmd = fetch_word(src_data,src_len,&bit_addr,3);
		switch (cmd)
		{
			case 0:	// NEXT
				/* next line */
				dynax_blit_y += yinc;
				x = sx;
				break;

			case 1:	// LINE
				{
					int length = fetch_word(src_data,src_len,&bit_addr,arg_size);
					int pen = fetch_word(src_data,src_len,&bit_addr,pen_size);
					if (dynax_blit_pen_mode) pen = (dynax_blit_pen & 0x0f);
					pen |= dynax_blit_pen & 0xf0;
					while (length-- >= 0)
					{
						do_plot(x,dynax_blit_y,pen);
						x += xinc;
					}
				}
				break;

			case 2:	// COPY
				{
					int length = fetch_word(src_data,src_len,&bit_addr,arg_size);
					while (length-- >= 0)
					{
						int pen = fetch_word(src_data,src_len,&bit_addr,pen_size);
						if (dynax_blit_pen_mode) pen = (dynax_blit_pen & 0x0f);
						pen |= dynax_blit_pen & 0xf0;
						do_plot(x,dynax_blit_y,pen);
						x += xinc;
					}
				}
				break;

			case 3:	// SKIP
				x += xinc * fetch_word(src_data,src_len,&bit_addr,arg_size);
				break;

			case 5:	// CHGA
				arg_size = fetch_word(src_data,src_len,&bit_addr,4) + 1;
				break;

			case 6:	// CHGP
				pen_size = fetch_word(src_data,src_len,&bit_addr,3) + 1;
				break;

			default:
				log_draw_error(src,cmd);
			// fall through
			case 7:	// STOP
				return (bit_addr + 7) / 8;
		}
	}
}


static int hanakanz_blit_draw(int src,int sx)
{
	UINT8 *src_data = memory_region(REGION_GFX1);
	int src_len = memory_region_length(REGION_GFX1);
	int bit_addr = (src & 0xffffff) * 8 * 2;	/* convert to bit address */
	int pen, pen_size, arg_size, cmd;
	int x;
	int xinc = (dynax_blit_flip & 1) ? -1 : 1;
	int yinc = (dynax_blit_flip & 2) ? -1 : 1;

	pen_size = fetch_word(src_data,src_len,&bit_addr,4) + 1;
	arg_size = fetch_word(src_data,src_len,&bit_addr,4) + 1;

#ifdef MAME_DEBUG
	if (pen_size > 4 || arg_size > 8)
		ui_popup("warning: pen_size %d arg_size %d",pen_size,arg_size);
#endif

	x = sx;

	for (;;)
	{
		cmd = fetch_word(src_data,src_len,&bit_addr,3);
		switch (cmd)
		{
			case 0:	// NEXT
				/* next line */
				dynax_blit_y += yinc;
				x = sx;
				break;

			case 1:	// CHGP
				pen_size = fetch_word(src_data,src_len,&bit_addr,3) + 1;
				break;

			case 2:	// CHGA
				arg_size = fetch_word(src_data,src_len,&bit_addr,4) + 1;
				break;

			case 4:	// SKIP
				x += xinc * fetch_word(src_data,src_len,&bit_addr,arg_size);
				break;

			case 5:	// COPY
			{
				int length = fetch_word(src_data,src_len,&bit_addr,arg_size);
				while (length-- >= 0)
				{
					pen = fetch_word(src_data,src_len,&bit_addr,pen_size);
					if (dynax_blit_pen_mode) pen = (dynax_blit_pen & 0x0f);
					pen |= dynax_blit_pen & 0xf0;
					do_plot(x,dynax_blit_y,pen);
					x += xinc;
				}
			}
			break;

			case 6:	// LINE
			{
				int length = fetch_word(src_data,src_len,&bit_addr,arg_size);
				pen = fetch_word(src_data,src_len,&bit_addr,pen_size);
				if (dynax_blit_pen_mode) pen = (dynax_blit_pen & 0x0f);
				pen |= dynax_blit_pen & 0xf0;
				while (length-- >= 0)
				{
					do_plot(x,dynax_blit_y,pen);
					x += xinc;
				}
			}
			break;

			default:
				log_draw_error(src,cmd);
			// fall through
			case 7:	// STOP
				return (bit_addr + 15) / 16;
		}
	}
}



/*  Draw a simple rectangle
*/
static void blit_rect_xywh(void)
{
	int x,y;

#ifdef MAME_DEBUG
//  if (dynax_clip_ctrl != 0x0f)
//      ui_popup("RECT clipx=%03x clipy=%03x ctrl=%x",dynax_clip_x,dynax_clip_y,dynax_clip_ctrl);
#endif

	for (y = 0; y <= dynax_rect_height; y++)
		for (x = 0; x <= dynax_rect_width; x++)
			do_plot( x + dynax_blit_x, y + dynax_blit_y, dynax_blit_pen);
}



/*  Unknown. Initialized arguments are
    00 dest layer
    05 unknown, related to layer
    14 X - always 0?
    02 Y
    0a width - always 0?
    0b height
    04 blit_pen
    0c line_length - always 0?
*/
static void blit_rect_yh(void)
{
	int start = 512 * dynax_blit_y;
	int length = 512 * (dynax_rect_height+1);

#ifdef MAME_DEBUG
//  if (dynax_clip_ctrl != 0x0f)
//      ui_popup("UNK8C clipx=%03x clipy=%03x ctrl=%x",dynax_clip_x,dynax_clip_y,dynax_clip_ctrl);
#endif

	if (start < 512*512)
	{
		if (start + length > 512*512)
			length = 512*512 - start;

		if (dynax_dest_layer & 0x0001) memset(pixmap[0] + start,dynax_blit_pen,length);
		if (dynax_dest_layer & 0x0002) memset(pixmap[1] + start,dynax_blit_pen,length);
		if (dynax_dest_layer & 0x0004) memset(pixmap[2] + start,dynax_blit_pen,length);
		if (dynax_dest_layer & 0x0008) memset(pixmap[3] + start,dynax_blit_pen,length);

		if (!extra_layers)	return;

		if (dynax_dest_layer & 0x0100) memset(pixmap[4] + start,dynax_blit_pen,length);
		if (dynax_dest_layer & 0x0200) memset(pixmap[5] + start,dynax_blit_pen,length);
		if (dynax_dest_layer & 0x0400) memset(pixmap[6] + start,dynax_blit_pen,length);
		if (dynax_dest_layer & 0x0800) memset(pixmap[7] + start,dynax_blit_pen,length);
	}
}



/*  Fill from (X,Y) to end of pixmap
    initialized arguments are
    00 dest layer
    05 unknown, related to layer
    14 X
    02 Y
    04 blit_pen
*/
static void blit_fill_xy(int x, int y)
{
	int start = 512 * y + x;

#ifdef MAME_DEBUG
//  if (x || y)
//      ui_popup("FILL command X %03x Y %03x",x,y);
#endif

	if (dynax_dest_layer & 0x0001) memset(pixmap[0] + start,dynax_blit_pen,512*512 - start);
	if (dynax_dest_layer & 0x0002) memset(pixmap[1] + start,dynax_blit_pen,512*512 - start);
	if (dynax_dest_layer & 0x0004) memset(pixmap[2] + start,dynax_blit_pen,512*512 - start);
	if (dynax_dest_layer & 0x0008) memset(pixmap[3] + start,dynax_blit_pen,512*512 - start);

	if (!extra_layers)	return;

	if (dynax_dest_layer & 0x0100) memset(pixmap[4] + start,dynax_blit_pen,512*512 - start);
	if (dynax_dest_layer & 0x0200) memset(pixmap[5] + start,dynax_blit_pen,512*512 - start);
	if (dynax_dest_layer & 0x0400) memset(pixmap[6] + start,dynax_blit_pen,512*512 - start);
	if (dynax_dest_layer & 0x0800) memset(pixmap[7] + start,dynax_blit_pen,512*512 - start);
}



/*  Draw horizontal line
    initialized arguments are
    00 dest layer
    05 unknown, related to layer
    14 X
    02 Y
    0c line length
    04 blit_pen
    dynax_blit_x and dynax_blit_y are left pointing to the last pixel at the end of the command
*/
static void blit_horiz_line(void)
{
	int i;

#ifdef MAME_DEBUG
	ui_popup("LINE X");

	if (dynax_clip_ctrl != 0x0f)
		ui_popup("LINE X clipx=%03x clipy=%03x ctrl=%x",dynax_clip_x,dynax_clip_y,dynax_clip_ctrl);

	if (dynax_blit_flip)
		ui_popup("LINE X flip=%x",dynax_blit_flip);
#endif

	for (i = 0; i <= dynax_line_length; i++)
		do_plot(dynax_blit_x++,dynax_blit_y,dynax_blit_pen);
}



/*  Draw vertical line
    initialized arguments are
    00 dest layer
    05 unknown, related to layer
    14 X
    02 Y
    0c line length
    04 blit_pen
    dynax_blit_x and dynax_blit_y are left pointing to the last pixel at the end of the command
*/
static void blit_vert_line(void)
{
	int i;

#ifdef MAME_DEBUG
	ui_popup("LINE Y");

	if (dynax_clip_ctrl != 0x0f)
		ui_popup("LINE Y clipx=%03x clipy=%03x ctrl=%x",dynax_clip_x,dynax_clip_y,dynax_clip_ctrl);
#endif

	for (i = 0; i <= dynax_line_length; i++)
		do_plot(dynax_blit_x,dynax_blit_y++,dynax_blit_pen);
}




INLINE void log_blit(int data)
{
	logerror("%06x: blit src %06x x %03x y %03x flags %02x layer %02x pen %02x penmode %02x w %03x h %03x linelen %03x clip: ctrl %x xy %03x %03x wh %03x %03x\n",
			activecpu_get_pc(),
			dynax_blit_address,dynax_blit_x,dynax_blit_y,data,
			dynax_dest_layer,dynax_blit_pen,dynax_blit_pen_mode,dynax_rect_width,dynax_rect_height,dynax_line_length,
			dynax_clip_ctrl,dynax_clip_x,dynax_clip_y, dynax_clip_width,dynax_clip_height		);
}

static void blitter_w(int blitter, offs_t offset,UINT8 data,int irq_vector)
{
	static int dynax_blit_reg[2];
	int hi_bits;

profiler_mark(PROFILER_VIDEO);

	switch(offset)
	{
	case 0:
		dynax_blit_reg[blitter] = data;
		break;

	case 1:
		hi_bits = (dynax_blit_reg[blitter] & 0xc0) << 2;

		switch(dynax_blit_reg[blitter] & 0x3f)
		{
		case 0x00:
			if (blitter)	dynax_dest_layer = (dynax_dest_layer & 0x00ff) | (data<<8);
			else			dynax_dest_layer = (dynax_dest_layer & 0xff00) | (data<<0);
			break;

		case 0x01:
			dynax_flipscreen_w(data);
			break;

		case 0x02:
			dynax_blit_y = data | hi_bits;
			break;

		case 0x03:
			dynax_blit_flip_w(data);
			break;

		case 0x04:
			dynax_blit_pen = data;
			break;

		case 0x05:
			dynax_blit_pen_mask = data;
			break;

		case 0x06:
			// related to pen, can be 0 or 1 for 0x10 blitter command
			// 0 = only bits 7-4 of dynax_blit_pen contain data
			// 1 = bits 3-0 contain data as well
			dynax_blit_pen_mode = data;
			break;

		case 0x0a:
			dynax_rect_width = data | hi_bits;
			break;

		case 0x0b:
			dynax_rect_height = data | hi_bits;
			break;

		case 0x0c:
			dynax_line_length = data | hi_bits;
			break;

		case 0x0d:
			dynax_blit_address = (dynax_blit_address & ~0x0000ff) | (data <<0);
			break;
		case 0x0e:
			dynax_blit_address = (dynax_blit_address & ~0x00ff00) | (data <<8);
			break;
		case 0x0f:
			dynax_blit_address = (dynax_blit_address & ~0xff0000) | (data<<16);
			break;

		case 0x14:
			dynax_blit_x = data | hi_bits;
			break;

		case 0x16:
			dynax_clip_x = data | hi_bits;
			break;

		case 0x17:
			dynax_clip_y = data | hi_bits;
			break;

		case 0x18:
		case 0x19:
		case 0x1a:
		case 0x1b:
		case 0x1c:
		case 0x1d:
		case 0x1e:
		case 0x1f:
			dynax_scroll[blitter*8 + (dynax_blit_reg[blitter] & 7)] = data | hi_bits;
			break;

		case 0x20:
			dynax_clip_ctrl = data;
			break;

		case 0x24:

			log_blit(data);

			switch (data)
			{
				case 0x04:	blit_fill_xy(0, 0);
							break;
				case 0x14:	blit_fill_xy(dynax_blit_x, dynax_blit_y);
							break;

				case 0x10:	dynax_blit_address = blit_draw(dynax_blit_address,dynax_blit_x);
							break;

				case 0x13:	blit_horiz_line();
							break;
				case 0x1b:	blit_vert_line();
							break;

				case 0x1c:	blit_rect_xywh();
							break;

				// These two are issued one after the other (43 then 8c)
				// 8c is issued immediately after 43 has finished, without
				// changing any argument
				case 0x43:	break;
				case 0x8c:	blit_rect_yh();
							break;

				default:
							;
				#ifdef MAME_DEBUG
					ui_popup("unknown blitter command %02x",data);
					logerror("%06x: unknown blitter command %02x\n", activecpu_get_pc(), data);
				#endif
			}

			if (irq_vector)
			{
				/* quizchq */
				cpunum_set_input_line_and_vector(0, 0, HOLD_LINE, irq_vector);
			}
			else
			{
				/* ddenlovr */
				if (dynax_blitter_irq_enable)
				{
					dynax_blitter_irq_flag = 1;
					cpunum_set_input_line(0,1,HOLD_LINE);
				}
			}
			break;

		default:
			logerror("%06x: Blitter %d reg %02x = %02x\n", activecpu_get_pc(), blitter, dynax_blit_reg[blitter], data);
			break;
		}
	}

profiler_mark(PROFILER_END);
}




// differences wrt blitter_data_w: slightly different blitter commands
static void blitter_w_funkyfig(int blitter, offs_t offset,UINT8 data,int irq_vector)
{
	static int dynax_blit_reg[2];
	int hi_bits;

profiler_mark(PROFILER_VIDEO);

	switch(offset)
	{
	case 0:
		dynax_blit_reg[blitter] = data;
		break;

	case 1:
		hi_bits = (dynax_blit_reg[blitter] & 0xc0) << 2;

		switch(dynax_blit_reg[blitter] & 0x3f)
		{
		case 0x00:
			if (blitter)	dynax_dest_layer = (dynax_dest_layer & 0x00ff) | (data<<8);
			else			dynax_dest_layer = (dynax_dest_layer & 0xff00) | (data<<0);
			break;

		case 0x01:
			dynax_flipscreen_w(data);
			break;

		case 0x02:
			dynax_blit_y = data | hi_bits;
			break;

		case 0x03:
			dynax_blit_flip_w(data);
			break;

		case 0x04:
			dynax_blit_pen = data;
			break;

		case 0x05:
			dynax_blit_pen_mask = data;
			break;

		case 0x06:
			// related to pen, can be 0 or 1 for 0x10 blitter command
			// 0 = only bits 7-4 of dynax_blit_pen contain data
			// 1 = bits 3-0 contain data as well
			dynax_blit_pen_mode = data;
			break;

		case 0x0a:
			dynax_rect_width = data | hi_bits;
			break;

		case 0x0b:
			dynax_rect_height = data | hi_bits;
			break;

		case 0x0c:
			dynax_line_length = data | hi_bits;
			break;

		case 0x0d:
			dynax_blit_address = (dynax_blit_address & ~0x0000ff) | (data <<0);
			break;
		case 0x0e:
			dynax_blit_address = (dynax_blit_address & ~0x00ff00) | (data <<8);
			break;
		case 0x0f:
			dynax_blit_address = (dynax_blit_address & ~0xff0000) | (data<<16);
			break;

		case 0x14:
			dynax_blit_x = data | hi_bits;
			break;

		case 0x16:
			dynax_clip_x = data | hi_bits;
			break;

		case 0x17:
			dynax_clip_y = data | hi_bits;
			break;

		case 0x18:
		case 0x19:
		case 0x1a:
		case 0x1b:
		case 0x1c:
		case 0x1d:
		case 0x1e:
		case 0x1f:
			dynax_scroll[blitter*8 + (dynax_blit_reg[blitter] & 7)] = data | hi_bits;
			break;

		case 0x20:
			dynax_clip_ctrl = data;
			break;

		case 0x24:

			log_blit(data);

			switch (data)
			{

				case 0x84:	// same as 04?
				case 0x04:	blit_fill_xy(0, 0);
							break;

//              unused?
//              case 0x14:  blit_fill_xy(dynax_blit_x, dynax_blit_y);
//                          break;

				case 0x00/*0x10*/:	dynax_blit_address = blit_draw(dynax_blit_address,dynax_blit_x);
							break;

				case 0x0b:	// same as 03? see the drawing of the R in "cRoss hatch" (key test)
				case 0x03/*0x13*/:	blit_horiz_line();
							break;
//              unused?
//              case 0x1b:  blit_vert_line();
//                          break;

				case 0x0c/*0x1c*/:	blit_rect_xywh();
							break;

				// These two are issued one after the other (43 then 8c)
				// 8c is issued immediately after 43 has finished, without
				// changing any argument
				case 0x43:	break;
				case 0x8c:	blit_rect_yh();
							break;

				default:
							;
				#ifdef MAME_DEBUG
					ui_popup("unknown blitter command %02x",data);
					logerror("%06x: unknown blitter command %02x\n", activecpu_get_pc(), data);
				#endif
			}

			cpunum_set_input_line_and_vector(0, 0, HOLD_LINE, irq_vector);
			break;

		default:
			logerror("%06x: Blitter %d reg %02x = %02x\n", activecpu_get_pc(), blitter, dynax_blit_reg[blitter], data);
			break;
		}
	}

profiler_mark(PROFILER_END);
}




static WRITE8_HANDLER( hanakanz_blitter_reg_w )
{
	dynax_blit_reg = data;
}

// differences wrt blitter_data_w: registers are shuffled around, hi_bits in the low bits, clip_w/h, includes layers registers
static WRITE8_HANDLER( hanakanz_blitter_data_w )
{
	int hi_bits;

profiler_mark(PROFILER_VIDEO);

	hi_bits = (dynax_blit_reg & 0x03) << 8;

	switch(dynax_blit_reg & 0xfe)
	{
		case 0x00:
			dynax_dest_layer = data;
			break;

		case 0x04:
			dynax_flipscreen_w(data);
			break;

		case 0x08:
			dynax_blit_y = data | hi_bits;
			break;

		case 0x0c:
			dynax_blit_flip_w(data);
			break;

		case 0x10:
			dynax_blit_pen = data;
			break;

		case 0x14:
			dynax_blit_pen_mask = data;
			break;

		case 0x18:
			// related to pen, can be 0 or 1 for 0x10 blitter command
			// 0 = only bits 7-4 of dynax_blit_pen contain data
			// 1 = bits 3-0 contain data as well
			dynax_blit_pen_mode = data;
			break;

		case 0x28:
			dynax_rect_width = data | hi_bits;
			break;

		case 0x2c:
			dynax_rect_height = data | hi_bits;
			break;

		case 0x30:
			dynax_line_length = data | hi_bits;
			break;

		case 0x34:
			dynax_blit_address = (dynax_blit_address & ~0x0000ff) | (data <<0);
			break;
		case 0x38:
			dynax_blit_address = (dynax_blit_address & ~0x00ff00) | (data <<8);
			break;
		case 0x3c:
			dynax_blit_address = (dynax_blit_address & ~0xff0000) | (data<<16);
			break;

		case 0x50:
			dynax_blit_x = data | hi_bits;
			break;

		case 0x58:
			dynax_clip_x = data | hi_bits;
			break;

		case 0x5c:
			dynax_clip_y = data | hi_bits;
			break;

		case 0x60:
		case 0x64:
		case 0x68:
		case 0x6c:
		case 0x70:
		case 0x74:
		case 0x78:
		case 0x7c:
			dynax_scroll[(dynax_blit_reg & 0x1c) >> 2] = data | hi_bits;
			break;

		case 0x80:
			dynax_clip_ctrl = data;
			break;

		case 0x88:
		case 0x8a:	// can be 3ff
			dynax_clip_height = data | hi_bits;
			break;

		case 0x8c:
		case 0x8e:	// can be 3ff
			dynax_clip_width = data | hi_bits;
			break;

		case 0xc0:
		case 0xc2:
		case 0xc4:
		case 0xc6:
			dynax_palette_base[(dynax_blit_reg >> 1) & 3] = data | (hi_bits & 0x100);
			break;

		case 0xc8:
		case 0xca:
		case 0xcc:
		case 0xce:
			dynax_palette_mask[(dynax_blit_reg >> 1) & 3] = data;
			break;

		case 0xd0:
		case 0xd2:
		case 0xd4:
		case 0xd6:
			dynax_transparency_pen[(dynax_blit_reg >> 1) & 3] = data;
			break;

		case 0xd8:
		case 0xda:
		case 0xdc:
		case 0xde:
			dynax_transparency_mask[(dynax_blit_reg >> 1) & 3] = data;
			break;

		case 0xe4:
			dynax_priority_w(0,data);
			break;

		case 0xe6:
			dynax_layer_enable_w(0,data);
			break;

		case 0xe8:
			dynax_bgcolor = data | hi_bits;
			break;

		case 0x90:

			log_blit(data);

			switch (data)
			{
				case 0x04:	blit_fill_xy(0, 0);
							break;
				case 0x14:	blit_fill_xy(dynax_blit_x, dynax_blit_y);
							break;

				case 0x10:	dynax_blit_address = hanakanz_blit_draw(dynax_blit_address,dynax_blit_x);
							break;

				case 0x13:	blit_horiz_line();
							break;
				case 0x1b:	blit_vert_line();
							break;

				case 0x1c:	blit_rect_xywh();
							break;

				// These two are issued one after the other (43 then 8c)
				// 8c is issued immediately after 43 has finished, without
				// changing any argument
				case 0x43:	break;
				case 0x8c:	blit_rect_yh();
							break;

				default:
							;
				#ifdef MAME_DEBUG
					ui_popup("unknown blitter command %02x",data);
					logerror("%06x: unknown blitter command %02x\n", activecpu_get_pc(), data);
				#endif
			}

			// NO IRQ !?

			break;

		default:
			logerror("%06x: Blitter 0 reg %02x = %02x\n", activecpu_get_pc(), dynax_blit_reg, data);
			break;
	}

profiler_mark(PROFILER_END);
}


static WRITE8_HANDLER( rongrong_blitter_w )
{
	blitter_w(0,offset,data,0xf8);
}

static WRITE16_HANDLER( ddenlovr_blitter_w )
{
	if (ACCESSING_LSB)
		blitter_w(0,offset,data & 0xff,0);
}


static WRITE16_HANDLER( ddenlovr_blitter_irq_ack_w )
{
	if (ACCESSING_LSB)
	{
		if (data & 1)
		{
			dynax_blitter_irq_enable = 1;
		}
		else
		{
			dynax_blitter_irq_enable = 0;
			dynax_blitter_irq_flag = 0;
		}
	}
}


static READ8_HANDLER( rongrong_gfxrom_r )
{
	UINT8 *rom	=	memory_region( REGION_GFX1 );
	size_t size		=	memory_region_length( REGION_GFX1 );
	int address	=	dynax_blit_address;

	if (address >= size)
	{
		logerror("CPU#0 PC %06X: Error, Blitter address %06X out of range\n", activecpu_get_pc(), address);
		address %= size;
	}

	dynax_blit_address++;

	return rom[address];
}

static READ16_HANDLER( ddenlovr_gfxrom_r )
{
	return rongrong_gfxrom_r(offset);
}



static void copylayer(mame_bitmap *bitmap,const rectangle *cliprect,int layer)
{
	int x,y;
	int scrollx = dynax_scroll[layer/4*8 + (layer%4) + 0];
	int scrolly = dynax_scroll[layer/4*8 + (layer%4) + 4];

	int palbase = dynax_palette_base[layer];
	int penmask = dynax_palette_mask[layer];

	int transpen = dynax_transparency_pen[layer];
	int transmask = dynax_transparency_mask[layer];

	palbase		&=	~penmask;
	transpen	&=	transmask;

	if (((dynax_layer_enable2 << 4) | dynax_layer_enable) & (1 << layer))
	{
		for (y = cliprect->min_y;y <= cliprect->max_y;y++)
		{
			for (x = cliprect->min_x;x <= cliprect->max_x;x++)
			{
				int pen = pixmap[layer][512 * ((y + scrolly) & 0x1ff) + ((x + scrollx) & 0x1ff)];
				if ( (pen & transmask) != transpen )
				{
					pen &= penmask;
					pen |= palbase;
					((UINT16 *)bitmap->line[y])[x] = pen;
				}
			}
		}
	}
}

VIDEO_UPDATE(ddenlovr)
{
	copybitmap(bitmap,framebuffer,0,0,0,0,cliprect,TRANSPARENCY_NONE,0);
}

/*
    I do the following in a eof handler, to avoid  palette/gfx synchronization
    issues with frameskipping
*/
VIDEO_EOF(ddenlovr)
{
	static const int order[24][4] =
	{
		{ 3,2,1,0 }, { 2,3,1,0 }, { 3,1,2,0 }, { 1,3,2,0 }, { 2,1,3,0 }, { 1,2,3,0 },
		{ 3,2,0,1 }, { 2,3,0,1 }, { 3,0,2,1 }, { 0,3,2,1 }, { 2,0,3,1 }, { 0,2,3,1 },
		{ 3,1,0,2 }, { 1,3,0,2 }, { 3,0,1,2 }, { 0,3,1,2 }, { 1,0,3,2 }, { 0,1,3,2 },
		{ 2,1,0,3 }, { 1,2,0,3 }, { 2,0,1,3 }, { 0,2,1,3 }, { 1,0,2,3 }, { 0,1,2,3 }
	};

	int pri;

	int enab = dynax_layer_enable;
	int enab2 = dynax_layer_enable2;

#if 0
	static int base = 0x0;

	int next;
	memset(pixmap[0],0,512*512);
	memset(pixmap[1],0,512*512);
	memset(pixmap[2],0,512*512);
	memset(pixmap[3],0,512*512);
	dynax_dest_layer = 8;
	dynax_blit_pen = 0;
	dynax_blit_pen_mode = 0;
	dynax_blit_y = 5;
	dynax_clip_ctrl = 0x0f;
//  next = blit_draw(base,0);
	next = hanakanz_blit_draw(base,0);
if (code_pressed(KEYCODE_Z))
	ui_popup("%06x",base);
	if (code_pressed(KEYCODE_S)) base = next;
	if (code_pressed_memory(KEYCODE_X)) base = next;
	if (code_pressed(KEYCODE_C)) { base--; while ((memory_region(REGION_GFX1)[base] & 0xf0) != 0x30) base--; }
	if (code_pressed(KEYCODE_V)) { base++; while ((memory_region(REGION_GFX1)[base] & 0xf0) != 0x30) base++; }
	if (code_pressed_memory(KEYCODE_D)) { base--; while ((memory_region(REGION_GFX1)[base] & 0xf0) != 0x30) base--; }
	if (code_pressed_memory(KEYCODE_F)) { base++; while ((memory_region(REGION_GFX1)[base] & 0xf0) != 0x30) base++; }
#endif

	fillbitmap(framebuffer,dynax_bgcolor,&Machine->visible_area);

#ifdef MAME_DEBUG
	if (code_pressed(KEYCODE_Z))
	{
		int mask,mask2;

		mask = 0;

		if (code_pressed(KEYCODE_Q))	mask |= 1;
		if (code_pressed(KEYCODE_W))	mask |= 2;
		if (code_pressed(KEYCODE_E))	mask |= 4;
		if (code_pressed(KEYCODE_R))	mask |= 8;

		mask2 = 0;

		if (extra_layers)
		{
			if (code_pressed(KEYCODE_A))	mask2 |= 1;
			if (code_pressed(KEYCODE_S))	mask2 |= 2;
			if (code_pressed(KEYCODE_D))	mask2 |= 4;
			if (code_pressed(KEYCODE_F))	mask2 |= 8;
		}

		if (mask || mask2)
		{
			dynax_layer_enable &= mask;
			dynax_layer_enable2 &= mask2;
		}
	}
#endif

	pri = dynax_priority;

		if (pri >= 24)
		{
			ui_popup("priority = %02x",pri);
			pri = 0;
		}

		copylayer(framebuffer,&Machine->visible_area,order[pri][0]);
		copylayer(framebuffer,&Machine->visible_area,order[pri][1]);
		copylayer(framebuffer,&Machine->visible_area,order[pri][2]);
		copylayer(framebuffer,&Machine->visible_area,order[pri][3]);

	if (extra_layers)
	{
		pri = dynax_priority2;

		if (pri >= 24)
		{
			ui_popup("priority2 = %02x",pri);
			pri = 0;
		}

		copylayer(framebuffer,&Machine->visible_area,order[pri][0]+4);
		copylayer(framebuffer,&Machine->visible_area,order[pri][1]+4);
		copylayer(framebuffer,&Machine->visible_area,order[pri][2]+4);
		copylayer(framebuffer,&Machine->visible_area,order[pri][3]+4);
	}

	dynax_layer_enable = enab;
	dynax_layer_enable2 = enab2;
}



/***************************************************************************

    MSM6242 Real Time Clock

***************************************************************************/

static WRITE8_HANDLER( rtc_w )
{
//  logerror("%04X: RTC reg %x = %02x\n", activecpu_get_pc(), offset, data);
}

static READ8_HANDLER( rtc_r )
{
	struct tm *tm;
	time_t tms;

	if (Machine->record_file != NULL || Machine->playback_file != NULL)
		return 0;

	time(&tms);
	tm = localtime(&tms);
	// The clock is not y2k-compatible, wrap back 10 years, screw the leap years
	//  tm->tm_year -= 10;

	switch(offset)
	{
		case 0x0:	return tm->tm_sec % 10;
		case 0x1:	return tm->tm_sec / 10;
		case 0x2:	return tm->tm_min % 10;
		case 0x3:	return tm->tm_min / 10;
		case 0x4:	return tm->tm_hour % 10;
		case 0x5:	return tm->tm_hour / 10;
		case 0x6:	return tm->tm_mday % 10;
		case 0x7:	return tm->tm_mday / 10;
		case 0x8:	return (tm->tm_mon+1) % 10;
		case 0x9:	return (tm->tm_mon+1) / 10;
		case 0xa:	return tm->tm_year % 10;
		case 0xb:	return (tm->tm_year % 100) / 10;
		case 0xc:	return tm->tm_wday;
		case 0xd:	return 1;
		case 0xe:	return 6;
		case 0xf:	return 4;
	}

	return 0;
}

static WRITE16_HANDLER( rtc16_w )
{
	if (ACCESSING_LSB)
		rtc_w(offset,data);
}

static READ16_HANDLER( rtc16_r )
{
	return rtc_r(offset);
}





READ16_HANDLER( ddenlovr_special_r )
{
	int res = readinputport(2) | (dynax_blitter_irq_flag << 6);

	return res;
}

static WRITE16_HANDLER( ddenlovr_coincounter_0_w )
{
	if (ACCESSING_LSB)
		coin_counter_w(0, data & 1);
}
static WRITE16_HANDLER( ddenlovr_coincounter_1_w )
{
	if (ACCESSING_LSB)
		coin_counter_w(1, data & 1);
}


static UINT8 palram[0x200];

static WRITE8_HANDLER( rongrong_palette_w )
{
	int r,g,b,d1,d2,indx;

	palram[offset] = data;

	indx = ((offset & 0x1e0) >> 1) | (offset & 0x00f);
	d1 = palram[offset & ~0x10];
	d2 = palram[offset |  0x10];

	r = d1 & 0x1f;
	g = d2 & 0x1f;
	/* what were they smoking??? */
	b = ((d1 & 0xe0) >> 5) | (d2 & 0xc0) >> 3;

	r =  (r << 3) | (r >> 2);
	g =  (g << 3) | (g >> 2);
	b =  (b << 3) | (b >> 2);

	palette_set_color(indx,r,g,b);
}

static WRITE16_HANDLER( ddenlovr_palette_w )
{
	if (ACCESSING_LSB)
		rongrong_palette_w(offset,data & 0xff);
}


static WRITE8_HANDLER( dynax_palette_base_w )
{
	dynax_palette_base[offset] = data;
}
static WRITE8_HANDLER( dynax_palette_base2_w )
{
	dynax_palette_base[offset+4] = data;
}

static WRITE8_HANDLER( dynax_palette_mask_w )
{
	dynax_palette_mask[offset] = data;
}
static WRITE8_HANDLER( dynax_palette_mask2_w )
{
	dynax_palette_mask[offset+4] = data;
}

static WRITE8_HANDLER( dynax_transparency_pen_w )
{
	dynax_transparency_pen[offset] = data;
}
static WRITE8_HANDLER( dynax_transparency_pen2_w )
{
	dynax_transparency_pen[offset+4] = data;
}

static WRITE8_HANDLER( dynax_transparency_mask_w )
{
	dynax_transparency_mask[offset] = data;
}
static WRITE8_HANDLER( dynax_transparency_mask2_w )
{
	dynax_transparency_mask[offset+4] = data;
}


static WRITE16_HANDLER( ddenlovr_palette_base_w )
{
	if (ACCESSING_LSB)
		dynax_palette_base[offset] = data & 0xff;
}

static WRITE16_HANDLER( ddenlovr_palette_mask_w )
{
	if (ACCESSING_LSB)
		dynax_palette_mask[offset] = data & 0xff;
}

static WRITE16_HANDLER( ddenlovr_transparency_pen_w )
{
	if (ACCESSING_LSB)
		dynax_transparency_pen[offset] = data & 0xff;
}

static WRITE16_HANDLER( ddenlovr_transparency_mask_w )
{
	if (ACCESSING_LSB)
		dynax_transparency_mask[offset] = data & 0xff;
}


static WRITE8_HANDLER( quizchq_oki_bank_w )
{
	OKIM6295_set_bank_base(0, (data & 1) * 0x40000);
}

static WRITE16_HANDLER( ddenlovr_oki_bank_w )
{
	if (ACCESSING_LSB)
		OKIM6295_set_bank_base(0, (data & 7) * 0x40000);
}


static int okibank;

static WRITE16_HANDLER( quiz365_oki_bank1_w )
{
	if (ACCESSING_LSB)
	{
		okibank = (okibank & 2) | (data & 1);
		OKIM6295_set_bank_base(0, okibank * 0x40000);
	}
}

static WRITE16_HANDLER( quiz365_oki_bank2_w )
{
	if (ACCESSING_LSB)
	{
		okibank = (okibank & 1) | ((data & 1) << 1);
		OKIM6295_set_bank_base(0, okibank * 0x40000);
	}
}



static READ8_HANDLER( unk_r )
{
	return 0x78;
}

static READ16_HANDLER( unk16_r )
{
	return unk_r(offset);
}


static UINT8 dynax_select, dynax_select2;

WRITE8_HANDLER( dynax_select_w )
{
	dynax_select = data;
}

WRITE16_HANDLER( dynax_select_16_w )
{
	if (ACCESSING_LSB)
		dynax_select = data;
}

WRITE8_HANDLER( dynax_select2_w )
{
	dynax_select2 = data;
}

WRITE16_HANDLER( dynax_select2_16_w )
{
	if (ACCESSING_LSB)
		dynax_select2 = data;
}

READ8_HANDLER( rongrong_input2_r )
{
//  logerror("%04x: input2_r offset %d select %x\n",activecpu_get_pc(),offset,dynax_select2 );
	/* 0 and 1 are read from offset 1, 2 from offset 0... */
	switch( dynax_select2 )
	{
		case 0x00:	return readinputport(0);
		case 0x01:	return readinputport(1);
		case 0x02:	return readinputport(2);
	}
	return 0xff;
}


READ8_HANDLER( quiz365_input_r )
{
	if (!(dynax_select & 0x01))	return readinputport(3);
	if (!(dynax_select & 0x02))	return readinputport(4);
	if (!(dynax_select & 0x04))	return readinputport(5);
	if (!(dynax_select & 0x08))	return 0xff;//rand();
	if (!(dynax_select & 0x10))	return 0xff;//rand();
	return 0xff;
}

READ16_HANDLER( quiz365_input2_r )
{
//  logerror("%04x: input2_r offset %d select %x\n",activecpu_get_pc(),offset,dynax_select2 );
	/* 0 and 1 are read from offset 1, 2 from offset 0... */
	switch( dynax_select2 )
	{
		case 0x10:	return readinputport(0);
		case 0x11:	return readinputport(1);
		case 0x12:	return readinputport(2) | (dynax_blitter_irq_flag << 6);
	}
	return 0xff;
}

static UINT8 rongrong_blitter_busy_select;

WRITE8_HANDLER( rongrong_blitter_busy_w )
{
	rongrong_blitter_busy_select = data;
	if (data != 0x18)
		logerror("%04x: rongrong_blitter_busy_w data = %02x\n",activecpu_get_pc(),data);
}
READ8_HANDLER( rongrong_blitter_busy_r )
{
	switch( rongrong_blitter_busy_select )
	{
		case 0x18:	return 0;	// bit 5 = blitter busy

		default:
			logerror("%04x: rongrong_blitter_busy_r with select = %02x\n",activecpu_get_pc(),rongrong_blitter_busy_select);
	}
	return 0xff;
}


static WRITE16_HANDLER( quiz365_coincounter_w )
{
	if (ACCESSING_LSB)
	{
		if (dynax_select2 == 0x1c)
		{
			coin_counter_w(0, ~data & 1);
			coin_counter_w(1, ~data & 4);
		}
	}
}

/*
37,28,12    11      ->      88
67,4c,3a    ??      ->      51
*/
static UINT16 quiz365_protection[2];
static READ16_HANDLER( quiz365_protection_r )
{
	switch(quiz365_protection[0])
	{
		case 0x3a:
			return 0x0051;
		default:
			return 0x0088;
	}
}
static WRITE16_HANDLER( quiz365_protection_w )
{
	COMBINE_DATA(quiz365_protection + offset);
}

static ADDRESS_MAP_START( quiz365_readmem, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x17ffff) AM_READ(MRA16_ROM					)	// ROM
	AM_RANGE(0x200c02, 0x200c03) AM_READ(quiz365_protection_r		)	// Protection
	AM_RANGE(0x300204, 0x300207) AM_READ(quiz365_input2_r			)	//
	AM_RANGE(0x300270, 0x300271) AM_READ(unk16_r					)	// ? must be 78 on startup (not necessary in ddlover)
	AM_RANGE(0x300286, 0x300287) AM_READ(ddenlovr_gfxrom_r			)	// Video Chip
	AM_RANGE(0x3002c0, 0x3002c1) AM_READ(OKIM6295_status_0_lsb_r	)	// Sound
	AM_RANGE(0x300340, 0x30035f) AM_READ(rtc16_r					)	// 6242RTC
	AM_RANGE(0x300384, 0x300385) AM_READ(AY8910_read_port_0_lsb_r	)
	AM_RANGE(0xff0000, 0xffffff) AM_READ(MRA16_RAM					)	// RAM
ADDRESS_MAP_END

static ADDRESS_MAP_START( quiz365_writemem, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x17ffff) AM_WRITE(MWA16_ROM						)	// ROM
	AM_RANGE(0x200000, 0x2003ff) AM_WRITE(ddenlovr_palette_w			)	// Palette
	AM_RANGE(0x200e0a, 0x200e0d) AM_WRITE(quiz365_protection_w			)	// Protection
//  AM_RANGE(0x201000, 0x2017ff) AM_WRITE(MWA16_RAM                     )   // ?
	AM_RANGE(0x300200, 0x300201) AM_WRITE(dynax_select2_16_w		)
	AM_RANGE(0x300202, 0x300203) AM_WRITE(quiz365_coincounter_w			)	// Coin Counters + more stuff written on startup
	AM_RANGE(0x300240, 0x300247) AM_WRITE(ddenlovr_palette_base_w		)
	AM_RANGE(0x300248, 0x30024f) AM_WRITE(ddenlovr_palette_mask_w		)
	AM_RANGE(0x300250, 0x300257) AM_WRITE(ddenlovr_transparency_pen_w	)
	AM_RANGE(0x300258, 0x30025f) AM_WRITE(ddenlovr_transparency_mask_w	)
	AM_RANGE(0x300268, 0x300269) AM_WRITE(ddenlovr_bgcolor_w			)
	AM_RANGE(0x30026a, 0x30026b) AM_WRITE(ddenlovr_priority_w			)
	AM_RANGE(0x30026c, 0x30026d) AM_WRITE(ddenlovr_layer_enable_w		)
	AM_RANGE(0x300280, 0x300283) AM_WRITE(ddenlovr_blitter_w			)
	AM_RANGE(0x300300, 0x300301) AM_WRITE(YM2413_register_port_0_lsb_w	)
	AM_RANGE(0x300302, 0x300303) AM_WRITE(YM2413_data_port_0_lsb_w		)
	AM_RANGE(0x300340, 0x30035f) AM_WRITE(rtc16_w						)   // 6242RTC
	AM_RANGE(0x3003ca, 0x3003cb) AM_WRITE(ddenlovr_blitter_irq_ack_w	)	// Blitter irq acknowledge
	AM_RANGE(0x300380, 0x300381) AM_WRITE(AY8910_control_port_0_lsb_w	)
	AM_RANGE(0x300382, 0x300383) AM_WRITE(AY8910_write_port_0_lsb_w		)
	AM_RANGE(0x3002c0, 0x3002c1) AM_WRITE(OKIM6295_data_0_lsb_w			)
	AM_RANGE(0x3003c2, 0x3003c3) AM_WRITE(quiz365_oki_bank1_w			)
	AM_RANGE(0x3003cc, 0x3003cd) AM_WRITE(quiz365_oki_bank2_w			)
	AM_RANGE(0xff0000, 0xffffff) AM_WRITE(MWA16_RAM						)	// RAM
ADDRESS_MAP_END


static ADDRESS_MAP_START( ddenlovr_readmem, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x07ffff) AM_READ(MRA16_ROM					)	// ROM
	AM_RANGE(0xe00086, 0xe00087) AM_READ(ddenlovr_gfxrom_r			)	// Video Chip
	AM_RANGE(0xe00070, 0xe00071) AM_READ(unk16_r					)	// ? must be 78 on startup (not necessary in ddlover)
	AM_RANGE(0xe00100, 0xe00101) AM_READ(input_port_0_word_r		)	// P1?
	AM_RANGE(0xe00102, 0xe00103) AM_READ(input_port_1_word_r		)	// P2?
	AM_RANGE(0xe00104, 0xe00105) AM_READ(ddenlovr_special_r			)	// Coins + ?
	AM_RANGE(0xe00200, 0xe00201) AM_READ(input_port_3_word_r		)	// DSW
	AM_RANGE(0xe00500, 0xe0051f) AM_READ(rtc16_r 					)	// 6242RTC
	AM_RANGE(0xe00604, 0xe00605) AM_READ(AY8910_read_port_0_lsb_r	)
	AM_RANGE(0xe00700, 0xe00701) AM_READ(OKIM6295_status_0_lsb_r	)	// Sound
	AM_RANGE(0xff0000, 0xffffff) AM_READ(MRA16_RAM					)	// RAM
ADDRESS_MAP_END

static ADDRESS_MAP_START( ddenlovr_writemem, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x07ffff) AM_WRITE(MWA16_ROM						)	// ROM
	AM_RANGE(0x300000, 0x300001) AM_WRITE(ddenlovr_oki_bank_w			)
	AM_RANGE(0xd00000, 0xd003ff) AM_WRITE(ddenlovr_palette_w			)	// Palette
//  AM_RANGE(0xd01000, 0xd017ff) MWA16_RAM                              )   // ? B0 on startup, then 00
	AM_RANGE(0xe00040, 0xe00047) AM_WRITE(ddenlovr_palette_base_w		)
	AM_RANGE(0xe00048, 0xe0004f) AM_WRITE(ddenlovr_palette_mask_w		)
	AM_RANGE(0xe00050, 0xe00057) AM_WRITE(ddenlovr_transparency_pen_w	)
	AM_RANGE(0xe00058, 0xe0005f) AM_WRITE(ddenlovr_transparency_mask_w	)
	AM_RANGE(0xe00068, 0xe00069) AM_WRITE(ddenlovr_bgcolor_w			)
	AM_RANGE(0xe0006a, 0xe0006b) AM_WRITE(ddenlovr_priority_w			)
	AM_RANGE(0xe0006c, 0xe0006d) AM_WRITE(ddenlovr_layer_enable_w		)
	AM_RANGE(0xe00080, 0xe00083) AM_WRITE(ddenlovr_blitter_w			)
	AM_RANGE(0xe00302, 0xe00303) AM_WRITE(ddenlovr_blitter_irq_ack_w	)	// Blitter irq acknowledge
	AM_RANGE(0xe00308, 0xe00309) AM_WRITE(ddenlovr_coincounter_0_w		)	// Coin Counters
	AM_RANGE(0xe0030c, 0xe0030d) AM_WRITE(ddenlovr_coincounter_1_w		)	//
	AM_RANGE(0xe00400, 0xe00401) AM_WRITE(YM2413_register_port_0_lsb_w	)
	AM_RANGE(0xe00402, 0xe00403) AM_WRITE(YM2413_data_port_0_lsb_w		)
	AM_RANGE(0xe00500, 0xe0051f) AM_WRITE(rtc16_w						)	// 6242RTC
//  AM_RANGE(0xe00302, 0xe00303) AM_WRITE(MWA16_NOP                     )   // ?
	AM_RANGE(0xe00600, 0xe00601) AM_WRITE(AY8910_control_port_0_lsb_w	)
	AM_RANGE(0xe00602, 0xe00603) AM_WRITE(AY8910_write_port_0_lsb_w		)
	AM_RANGE(0xe00700, 0xe00701) AM_WRITE(OKIM6295_data_0_lsb_w 		)
	AM_RANGE(0xff0000, 0xffffff) AM_WRITE(MWA16_RAM						)	// RAM
ADDRESS_MAP_END


static READ16_HANDLER( nettoqc_special_r )
{
	return readinputport(2) | (dynax_blitter_irq_flag ? 0x60 : 0x00 );
}

static READ16_HANDLER( nettoqc_input_r )
{
	if (!(dynax_select & 0x01))	return readinputport(3);
	if (!(dynax_select & 0x02))	return readinputport(4);
	if (!(dynax_select & 0x04))	return readinputport(5);
	return 0xffff;
}

/*
    Protection:

    Writes 37 28 12 to 200e0b then 11 to 200e0d. Expects to read 88 from 200c03
    Writes 67 4c 3a to 200e0b then 19 to 200e0d. Expects to read 51 from 200c03
*/

static UINT16 *nettoqc_protection_val;

static READ16_HANDLER( nettoqc_protection_r )
{
	switch( nettoqc_protection_val[0] & 0xff )
	{
		case 0x3a:	return 0x0051;
		default:	return 0x0088;
	}
}

static WRITE16_HANDLER( nettoqc_coincounter_w )
{
	if (ACCESSING_LSB)
	{
		coin_counter_w(0, data & 0x01);
		coin_counter_w(1, data & 0x04);
		//                data & 0x80 ?
	}
}

static WRITE16_HANDLER( nettoqc_oki_bank_w )
{
	if (ACCESSING_LSB)
		OKIM6295_set_bank_base(0, (data & 3) * 0x40000);
}

static ADDRESS_MAP_START( nettoqc_readmem, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x17ffff) AM_READ(MRA16_ROM					)	// ROM
	AM_RANGE(0x200c02, 0x200c03) AM_READ(nettoqc_protection_r		)	//
	AM_RANGE(0x300070, 0x300071) AM_READ(unk16_r					)	// ? must be 78 on startup (not necessary in ddlover)
	AM_RANGE(0x300086, 0x300087) AM_READ(ddenlovr_gfxrom_r			)	// Video Chip
	AM_RANGE(0x300100, 0x30011f) AM_READ(rtc16_r					)	// 6242RTC
	AM_RANGE(0x300180, 0x300181) AM_READ(input_port_0_word_r		)	//
	AM_RANGE(0x300182, 0x300183) AM_READ(input_port_1_word_r		)	//
	AM_RANGE(0x300184, 0x300185) AM_READ(nettoqc_special_r			)	// Coins + ?
	AM_RANGE(0x300186, 0x300187) AM_READ(nettoqc_input_r			)	// DSW's
	AM_RANGE(0x300240, 0x300241) AM_READ(OKIM6295_status_0_lsb_r	)	// Sound
	AM_RANGE(0xff0000, 0xffffff) AM_READ(MRA16_RAM					)	// RAM
ADDRESS_MAP_END

static ADDRESS_MAP_START( nettoqc_writemem, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x17ffff) AM_WRITE(MWA16_ROM						)	// ROM
	AM_RANGE(0x200000, 0x2003ff) AM_WRITE(ddenlovr_palette_w			)	// Palette
	AM_RANGE(0x200e0a, 0x200e0d) AM_WRITE(MWA16_RAM) AM_BASE(&nettoqc_protection_val	)	//
	AM_RANGE(0x201000, 0x2017ff) AM_WRITE(MWA16_RAM 					)	// ?
	AM_RANGE(0x300040, 0x300047) AM_WRITE(ddenlovr_palette_base_w		)
	AM_RANGE(0x300048, 0x30004f) AM_WRITE(ddenlovr_palette_mask_w		)
	AM_RANGE(0x300050, 0x300057) AM_WRITE(ddenlovr_transparency_pen_w	)
	AM_RANGE(0x300058, 0x30005f) AM_WRITE(ddenlovr_transparency_mask_w	)
	AM_RANGE(0x300068, 0x300069) AM_WRITE(ddenlovr_bgcolor_w			)
	AM_RANGE(0x30006a, 0x30006b) AM_WRITE(ddenlovr_priority_w			)
	AM_RANGE(0x30006c, 0x30006d) AM_WRITE(ddenlovr_layer_enable_w		)
	AM_RANGE(0x300080, 0x300083) AM_WRITE(ddenlovr_blitter_w			)
	AM_RANGE(0x3000c0, 0x3000c1) AM_WRITE(YM2413_register_port_0_lsb_w	)
	AM_RANGE(0x3000c2, 0x3000c3) AM_WRITE(YM2413_data_port_0_lsb_w		)
	AM_RANGE(0x300100, 0x30011f) AM_WRITE(rtc16_w						)	// 6242RTC
	AM_RANGE(0x300140, 0x300141) AM_WRITE(AY8910_control_port_0_lsb_w	)
	AM_RANGE(0x300142, 0x300143) AM_WRITE(AY8910_write_port_0_lsb_w		)
	AM_RANGE(0x300188, 0x300189) AM_WRITE(nettoqc_coincounter_w			)	// Coin Counters
	AM_RANGE(0x30018a, 0x30018b) AM_WRITE(dynax_select_16_w				)	//
	AM_RANGE(0x30018c, 0x30018d) AM_WRITE(nettoqc_oki_bank_w			)
	AM_RANGE(0x3001ca, 0x3001cb) AM_WRITE(ddenlovr_blitter_irq_ack_w	)	// Blitter irq acknowledge
	AM_RANGE(0x300240, 0x300241) AM_WRITE(OKIM6295_data_0_lsb_w 		)
	AM_RANGE(0xff0000, 0xffffff) AM_WRITE(MWA16_RAM						)	// RAM
ADDRESS_MAP_END


/***************************************************************************
                                Rong Rong
***************************************************************************/

READ8_HANDLER( rongrong_input_r )
{
	if (!(dynax_select & 0x01))	return readinputport(3);
	if (!(dynax_select & 0x02))	return readinputport(4);
	if (!(dynax_select & 0x04))	return 0xff;//rand();
	if (!(dynax_select & 0x08))	return 0xff;//rand();
	if (!(dynax_select & 0x10))	return readinputport(5);
	return 0xff;
}

WRITE8_HANDLER( rongrong_select_w )
{
	UINT8 *rom = memory_region(REGION_CPU1);

//logerror("%04x: rongrong_select_w %02x\n",activecpu_get_pc(),data);
	/* bits 0-4 = **both** ROM bank **AND** input select */
	memory_set_bankptr(1, &rom[0x10000 + 0x8000 * (data & 0x1f)]);
	dynax_select = data;

	/* bits 5-7 = RAM bank */
	memory_set_bankptr(2, &rom[0x110000 + 0x1000 * ((data & 0xe0) >> 5)]);
}



static ADDRESS_MAP_START( quizchq_readmem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x5fff) AM_READ(MRA8_ROM					)	// ROM
	AM_RANGE(0x6000, 0x6fff) AM_READ(MRA8_RAM					)	// RAM
	AM_RANGE(0x7000, 0x7fff) AM_READ(MRA8_BANK2					)	// RAM (Banked)
	AM_RANGE(0x8000, 0xffff) AM_READ(MRA8_BANK1					)	// ROM (Banked)
ADDRESS_MAP_END

static ADDRESS_MAP_START( quizchq_writemem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x6000, 0x6fff) AM_WRITE(MWA8_RAM					)	// RAM
	AM_RANGE(0x7000, 0x7fff) AM_WRITE(MWA8_BANK2				)	// RAM (Banked)
	AM_RANGE(0x8000, 0x81ff) AM_WRITE(rongrong_palette_w		)
ADDRESS_MAP_END

static ADDRESS_MAP_START( quizchq_readport, ADDRESS_SPACE_IO, 8 )	ADDRESS_MAP_FLAGS( AMEF_ABITS(8) )
	AM_RANGE(0x03, 0x03) AM_READ(rongrong_gfxrom_r			)
	AM_RANGE(0x1b, 0x1b) AM_READ(rongrong_blitter_busy_r	)
	AM_RANGE(0x1c, 0x1c) AM_READ(rongrong_input_r			)
	AM_RANGE(0x22, 0x23) AM_READ(rongrong_input2_r			)
	AM_RANGE(0x40, 0x40) AM_READ(OKIM6295_status_0_r		)
	AM_RANGE(0x98, 0x98) AM_READ(unk_r						)	// ? must be 78 on startup
	AM_RANGE(0xa0, 0xaf) AM_READ(rtc_r						)	// 6242RTC
ADDRESS_MAP_END

static ADDRESS_MAP_START( quizchq_writeport, ADDRESS_SPACE_IO, 8 )	ADDRESS_MAP_FLAGS( AMEF_ABITS(8) )
	AM_RANGE(0x00, 0x01) AM_WRITE(rongrong_blitter_w		)
	AM_RANGE(0x1b, 0x1b) AM_WRITE(rongrong_blitter_busy_w	)
	AM_RANGE(0x1e, 0x1e) AM_WRITE(rongrong_select_w			)
	AM_RANGE(0x20, 0x20) AM_WRITE(dynax_select2_w			)
	AM_RANGE(0x40, 0x40) AM_WRITE(OKIM6295_data_0_w			)
	AM_RANGE(0x60, 0x60) AM_WRITE(YM2413_register_port_0_w	)
	AM_RANGE(0x61, 0x61) AM_WRITE(YM2413_data_port_0_w		)
	AM_RANGE(0x80, 0x83) AM_WRITE(dynax_palette_base_w		)
	AM_RANGE(0x84, 0x87) AM_WRITE(dynax_palette_mask_w		)
	AM_RANGE(0x88, 0x8b) AM_WRITE(dynax_transparency_pen_w	)
	AM_RANGE(0x8c, 0x8f) AM_WRITE(dynax_transparency_mask_w	)
	AM_RANGE(0x94, 0x94) AM_WRITE(dynax_bgcolor_w			)
	AM_RANGE(0x95, 0x95) AM_WRITE(dynax_priority_w			)
	AM_RANGE(0x96, 0x96) AM_WRITE(dynax_layer_enable_w		)
	AM_RANGE(0xa0, 0xaf) AM_WRITE(rtc_w						)	// 6242RTC
	AM_RANGE(0xc0, 0xc0) AM_WRITE(quizchq_oki_bank_w		)
	AM_RANGE(0xc2, 0xc2) AM_WRITE(MWA8_NOP					)	// enables palette RAM at 8000
ADDRESS_MAP_END



static ADDRESS_MAP_START( rongrong_readmem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x5fff) AM_READ(MRA8_ROM				)	// ROM
	AM_RANGE(0x6000, 0x6fff) AM_READ(MRA8_RAM				)	// RAM
	AM_RANGE(0x7000, 0x7fff) AM_READ(MRA8_BANK2				)	// RAM (Banked)
	AM_RANGE(0x8000, 0xffff) AM_READ(MRA8_BANK1				)	// ROM (Banked)
ADDRESS_MAP_END

static ADDRESS_MAP_START( rongrong_writemem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x6000, 0x6fff) AM_WRITE(MWA8_RAM				)	// RAM
	AM_RANGE(0x7000, 0x7fff) AM_WRITE(MWA8_BANK2			)	// RAM (Banked)
	AM_RANGE(0xf000, 0xf1ff) AM_WRITE(rongrong_palette_w	)
ADDRESS_MAP_END

static ADDRESS_MAP_START( rongrong_readport, ADDRESS_SPACE_IO, 8 )	ADDRESS_MAP_FLAGS( AMEF_ABITS(8) )
	AM_RANGE(0x03, 0x03) AM_READ(rongrong_gfxrom_r			)
	AM_RANGE(0x1b, 0x1b) AM_READ(rongrong_blitter_busy_r	)
	AM_RANGE(0x1c, 0x1c) AM_READ(rongrong_input_r			)
	AM_RANGE(0x20, 0x2f) AM_READ(rtc_r						)	// 6242RTC
	AM_RANGE(0x40, 0x40) AM_READ(OKIM6295_status_0_r		)
	AM_RANGE(0x98, 0x98) AM_READ(unk_r						)	// ? must be 78 on startup
	AM_RANGE(0xa2, 0xa3) AM_READ(rongrong_input2_r			)
ADDRESS_MAP_END

static ADDRESS_MAP_START( rongrong_writeport, ADDRESS_SPACE_IO, 8 )	ADDRESS_MAP_FLAGS( AMEF_ABITS(8) )
	AM_RANGE(0x00, 0x01) AM_WRITE(rongrong_blitter_w		)
	AM_RANGE(0x1b, 0x1b) AM_WRITE(rongrong_blitter_busy_w	)
	AM_RANGE(0x1e, 0x1e) AM_WRITE(rongrong_select_w			)
	AM_RANGE(0x20, 0x2f) AM_WRITE(rtc_w						)	// 6242RTC
	AM_RANGE(0x40, 0x40) AM_WRITE(OKIM6295_data_0_w			)
	AM_RANGE(0x60, 0x60) AM_WRITE(YM2413_register_port_0_w	)
	AM_RANGE(0x61, 0x61) AM_WRITE(YM2413_data_port_0_w		)
	AM_RANGE(0x80, 0x83) AM_WRITE(dynax_palette_base_w		)
	AM_RANGE(0x84, 0x87) AM_WRITE(dynax_palette_mask_w		)
	AM_RANGE(0x88, 0x8b) AM_WRITE(dynax_transparency_pen_w	)
	AM_RANGE(0x8c, 0x8f) AM_WRITE(dynax_transparency_mask_w	)
	AM_RANGE(0x94, 0x94) AM_WRITE(dynax_bgcolor_w			)
	AM_RANGE(0x95, 0x95) AM_WRITE(dynax_priority_w			)
	AM_RANGE(0x96, 0x96) AM_WRITE(dynax_layer_enable_w		)
	AM_RANGE(0xa0, 0xa0) AM_WRITE(dynax_select2_w			)
	AM_RANGE(0xc2, 0xc2) AM_WRITE(MWA8_NOP					)	// enables palette RAM at f000, and protection device at f705/f706/f601
ADDRESS_MAP_END
/*
1e input select,1c input read
    3e=dsw1 3d=dsw2
a0 input select,a2 input read (protection?)
    0=? 1=? 2=coins(from a3)
*/


/***************************************************************************
                                Monkey Mole Panic
***************************************************************************/


static READ8_HANDLER( magic_r )
{
	return 0x01;
}

static WRITE8_HANDLER( mmpanic_rombank_w )
{
	UINT8 *rom = memory_region(REGION_CPU1);
	memory_set_bankptr(1, &rom[0x10000 + 0x8000 * (data & 0x7)]);
	/* Bit 4? */
}

static WRITE8_HANDLER( mmpanic_soundlatch_w )
{
	soundlatch_w(0,data);
	cpunum_set_input_line(1, INPUT_LINE_NMI, PULSE_LINE);
}

static WRITE8_HANDLER( mmpanic_blitter_w )
{
	blitter_w(0,offset,data,0xdf);	// RST 18
}
static WRITE8_HANDLER( mmpanic_blitter2_w )
{
	blitter_w(1,offset,data,0xdf);	// RST 18
}

/* A led for each of the 9 buttons */
static UINT16 mmpanic_leds;

static void mmpanic_update_leds(void)
{
	set_led_status(0,mmpanic_leds);
}

/* leds 1-8 */
static WRITE8_HANDLER( mmpanic_leds_w )
{
	mmpanic_leds = (mmpanic_leds & 0xff00) | data;
	mmpanic_update_leds();
}
/* led 9 */
static WRITE8_HANDLER( mmpanic_leds2_w )
{
	mmpanic_leds = (mmpanic_leds & 0xfeff) | (data ? 0x0100 : 0);
	mmpanic_update_leds();
}


static WRITE8_HANDLER( mmpanic_lockout_w )
{
	if (dynax_select == 0x0c)
	{
		coin_counter_w(0,(~data) & 0x01);
		coin_lockout_w(0,(~data) & 0x02);
		set_led_status(1,(~data) & 0x04);
	}
}

static READ8_HANDLER( mmpanic_link_r )	{ return 0xff; }

/* Main CPU */

static ADDRESS_MAP_START( mmpanic_readmem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0051, 0x0051) AM_READ(magic_r					)	// ?
	AM_RANGE(0x0000, 0x5fff) AM_READ(MRA8_ROM					)	// ROM
	AM_RANGE(0x6000, 0x6fff) AM_READ(MRA8_RAM					)	// RAM
	AM_RANGE(0x7000, 0x7fff) AM_READ(MRA8_BANK2					)	// RAM (Banked)
	AM_RANGE(0x8000, 0xffff) AM_READ(MRA8_BANK1					)	// ROM (Banked)
ADDRESS_MAP_END

static ADDRESS_MAP_START( mmpanic_writemem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x6000, 0x6fff) AM_WRITE(MWA8_RAM					)	// RAM
	AM_RANGE(0x7000, 0x7fff) AM_WRITE(MWA8_BANK2				)	// RAM (Banked)
	AM_RANGE(0x8000, 0x81ff) AM_WRITE(rongrong_palette_w		)
ADDRESS_MAP_END

static ADDRESS_MAP_START( mmpanic_readport, ADDRESS_SPACE_IO, 8 )	ADDRESS_MAP_FLAGS( AMEF_ABITS(8) )
	AM_RANGE(0x00, 0x0f) AM_READ(rtc_r					)	// 6242RTC
	AM_RANGE(0x38, 0x38) AM_READ(unk_r					)	// ? must be 78 on startup
	AM_RANGE(0x58, 0x58) AM_READ(unk_r					)	// ? must be 78 on startup
	AM_RANGE(0x63, 0x63) AM_READ(rongrong_gfxrom_r		)	// Video Chip
	AM_RANGE(0x6a, 0x6a) AM_READ(input_port_0_r			)
	AM_RANGE(0x6b, 0x6b) AM_READ(input_port_1_r			)
	AM_RANGE(0x6c, 0x6d) AM_READ(mmpanic_link_r			)	// Other cabinets?
	AM_RANGE(0x7c, 0x7c) AM_READ(OKIM6295_status_0_r	)	// Sound
	AM_RANGE(0x94, 0x94) AM_READ(input_port_2_r			)	// DSW 1
	AM_RANGE(0x98, 0x98) AM_READ(input_port_3_r			)	// DSW 2
	AM_RANGE(0x9c, 0x9c) AM_READ(input_port_4_r			)	// DSW 1&2 high bits
ADDRESS_MAP_END

static ADDRESS_MAP_START( mmpanic_writeport, ADDRESS_SPACE_IO, 8 )	ADDRESS_MAP_FLAGS( AMEF_ABITS(8) )
	AM_RANGE(0x00, 0x0f) AM_WRITE(rtc_w						)	// 6242RTC
	// Layers 0-3:
	AM_RANGE(0x20, 0x23) AM_WRITE(dynax_palette_base_w		)
	AM_RANGE(0x24, 0x27) AM_WRITE(dynax_palette_mask_w		)
	AM_RANGE(0x28, 0x2b) AM_WRITE(dynax_transparency_pen_w	)
	AM_RANGE(0x2c, 0x2f) AM_WRITE(dynax_transparency_mask_w	)
	AM_RANGE(0x34, 0x34) AM_WRITE(dynax_bgcolor_w			)
	AM_RANGE(0x35, 0x35) AM_WRITE(dynax_priority_w			)
	AM_RANGE(0x36, 0x36) AM_WRITE(dynax_layer_enable_w		)
	// Layers 4-7:
	AM_RANGE(0x40, 0x43) AM_WRITE(dynax_palette_base2_w		)
	AM_RANGE(0x44, 0x47) AM_WRITE(dynax_palette_mask2_w		)
	AM_RANGE(0x48, 0x4b) AM_WRITE(dynax_transparency_pen2_w	)
	AM_RANGE(0x4c, 0x4f) AM_WRITE(dynax_transparency_mask2_w)
	AM_RANGE(0x54, 0x54) AM_WRITE(dynax_bgcolor2_w			)
	AM_RANGE(0x55, 0x55) AM_WRITE(dynax_priority2_w			)
	AM_RANGE(0x56, 0x56) AM_WRITE(dynax_layer_enable2_w		)

	AM_RANGE(0x60, 0x61) AM_WRITE(mmpanic_blitter_w			)
	AM_RANGE(0x64, 0x65) AM_WRITE(mmpanic_blitter2_w		)
	AM_RANGE(0x68, 0x68) AM_WRITE(dynax_select_w			)
	AM_RANGE(0x69, 0x69) AM_WRITE(mmpanic_lockout_w			)
	AM_RANGE(0x74, 0x74) AM_WRITE(mmpanic_rombank_w			)

	AM_RANGE(0x78, 0x78) AM_WRITE(MWA8_NOP					)	// 0, during RST 08 (irq acknowledge?)

	AM_RANGE(0x7c, 0x7c) AM_WRITE(OKIM6295_data_0_w			)	// Sound
	AM_RANGE(0x8c, 0x8c) AM_WRITE(mmpanic_soundlatch_w		)	//
	AM_RANGE(0x88, 0x88) AM_WRITE(mmpanic_leds_w			)	// Leds
	AM_RANGE(0x90, 0x90) AM_WRITE(MWA8_NOP					)	// written just before port 8c
	AM_RANGE(0xa6, 0xa6) AM_WRITE(mmpanic_leds2_w			)	//
ADDRESS_MAP_END

/* Sound CPU */

static ADDRESS_MAP_START( mmpanic_sound_readmem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x5fff) AM_READ(MRA8_ROM		)	// ROM
	AM_RANGE(0x6000, 0x66ff) AM_READ(MRA8_RAM		)	// RAM
	AM_RANGE(0x8000, 0xffff) AM_READ(MRA8_ROM		)	// ROM
ADDRESS_MAP_END

static ADDRESS_MAP_START( mmpanic_sound_writemem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x6000, 0x66ff) AM_WRITE(MWA8_RAM		)	// RAM
ADDRESS_MAP_END

static ADDRESS_MAP_START( mmpanic_sound_readport, ADDRESS_SPACE_IO, 8 )	ADDRESS_MAP_FLAGS( AMEF_ABITS(8) )
	AM_RANGE(0x00, 0x00) AM_READ(soundlatch_r		)
	AM_RANGE(0x02, 0x02) AM_READ(MRA8_NOP			)	// read just before port 00
	AM_RANGE(0x04, 0x04) AM_READ(MRA8_NOP			)	// read only once at the start
ADDRESS_MAP_END

static ADDRESS_MAP_START( mmpanic_sound_writeport, ADDRESS_SPACE_IO, 8 )	ADDRESS_MAP_FLAGS( AMEF_ABITS(8) )
	AM_RANGE(0x04, 0x04) AM_WRITE(MWA8_NOP					)	// 0, during IRQ
	AM_RANGE(0x06, 0x06) AM_WRITE(MWA8_NOP					)	// almost always 1, sometimes 0
	AM_RANGE(0x08, 0x08) AM_WRITE(YM2413_register_port_0_w	)
	AM_RANGE(0x09, 0x09) AM_WRITE(YM2413_data_port_0_w		)
	AM_RANGE(0x0c, 0x0c) AM_WRITE(AY8910_write_port_0_w		)
	AM_RANGE(0x0e, 0x0e) AM_WRITE(AY8910_control_port_0_w	)
ADDRESS_MAP_END



/***************************************************************************
                            The First Funky Fighter
***************************************************************************/

/* Main CPU */

static ADDRESS_MAP_START( funkyfig_readmem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x5fff) AM_READ( MRA8_ROM					)
	AM_RANGE(0x6000, 0x6fff) AM_READ( MRA8_RAM					)
	AM_RANGE(0x7000, 0x7fff) AM_READ( MRA8_BANK2				)	// RAM (Banked)
	AM_RANGE(0x8000, 0xffff) AM_READ( MRA8_BANK1				)	// ROM (Banked)
ADDRESS_MAP_END

static ADDRESS_MAP_START( funkyfig_writemem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x6000, 0x6fff) AM_WRITE( MWA8_RAM					)
	AM_RANGE(0x7000, 0x7fff) AM_WRITE( MWA8_BANK2				)	// RAM (Banked)
	AM_RANGE(0x8000, 0x81ff) AM_WRITE( rongrong_palette_w		)
	AM_RANGE(0x8400, 0x87ff) AM_WRITE( MWA8_NOP					)
ADDRESS_MAP_END


static READ8_HANDLER( funkyfig_busy_r )
{
					// bit 0 ?
	return 0x00;	// bit 7 = blitter busy
}

static WRITE8_HANDLER( funkyfig_blitter_w )
{
	blitter_w_funkyfig(0,offset,data,0xe0);
}

WRITE8_HANDLER( funkyfig_rombank_w )
{
	UINT8 *rom = memory_region(REGION_CPU1);

	dynax_select = data;

	memory_set_bankptr(1, &rom[0x10000 + 0x8000 * (data & 0x0f)]);
	// bit 4 selects palette ram at 8000?
	memory_set_bankptr(2, &rom[0x90000 + 0x1000 * ((data & 0xe0) >> 5)]);
}

READ8_HANDLER( funkyfig_dsw_r )
{
	if (!(dynax_select & 0x01))	return readinputport(3);
	if (!(dynax_select & 0x02))	return readinputport(4);
	if (!(dynax_select & 0x04))	return readinputport(5);
	logerror("%06x: warning, unknown bits read, dynax_select = %02x\n", activecpu_get_pc(), dynax_select);
	return 0xff;
}

static UINT8 funkyfig_lockout;

READ8_HANDLER( funkyfig_coin_r )
{
	switch( dynax_select2 )
	{
		case 0x22:	return readinputport(2);
		case 0x23:	return funkyfig_lockout;
	}
	logerror("%06x: warning, unknown bits read, dynax_select2 = %02x\n", activecpu_get_pc(), dynax_select2);
	return 0xff;
}

READ8_HANDLER( funkyfig_key_r )
{
	switch( dynax_select2 )
	{
		case 0x20:	return readinputport(0);
		case 0x21:	return readinputport(1);
	}
	logerror("%06x: warning, unknown bits read, dynax_select2 = %02x\n", activecpu_get_pc(), dynax_select2);
	return 0xff;
}

static WRITE8_HANDLER( funkyfig_lockout_w )
{
	switch( dynax_select2 )
	{
		case 0x2c:
			funkyfig_lockout = data;
			coin_counter_w(0,  data  & 0x01);
			coin_lockout_w(0,(~data) & 0x02);
			if (data & ~0x03)
				logerror("%06x: warning, unknown bits written, lockout = %02x\n", activecpu_get_pc(), data);
			break;

//      case 0xef:  16 bytes on startup

		default:
			logerror("%06x: warning, unknown bits written, dynax_select2 = %02x, data = %02x\n", activecpu_get_pc(), dynax_select2, data);
	}
}

static ADDRESS_MAP_START( funkyfig_readport, ADDRESS_SPACE_IO, 8 )	ADDRESS_MAP_FLAGS( AMEF_ABITS(8) )
	AM_RANGE(0x00, 0x00) AM_READ( OKIM6295_status_0_r	)	// Sound
	AM_RANGE(0x04, 0x04) AM_READ( funkyfig_busy_r		)
	AM_RANGE(0x1c, 0x1c) AM_READ( funkyfig_dsw_r		)
	AM_RANGE(0x23, 0x23) AM_READ( rongrong_gfxrom_r		)	// Video Chip
	AM_RANGE(0x40, 0x4f) AM_READ( rtc_r					)	// 6242RTC
	AM_RANGE(0x78, 0x78) AM_READ( unk_r					)	// ? must be 78 on startup
	AM_RANGE(0x82, 0x82) AM_READ( funkyfig_coin_r		)
	AM_RANGE(0x83, 0x83) AM_READ( funkyfig_key_r		)
//  Other cabinets?
ADDRESS_MAP_END

static ADDRESS_MAP_START( funkyfig_writeport, ADDRESS_SPACE_IO, 8 )	ADDRESS_MAP_FLAGS( AMEF_ABITS(8) )
	AM_RANGE(0x00, 0x00) AM_WRITE( OKIM6295_data_0_w		)	// Sound
	AM_RANGE(0x01, 0x01) AM_WRITE( mmpanic_leds_w			)	// Leds
	AM_RANGE(0x02, 0x02) AM_WRITE( mmpanic_soundlatch_w		)	//
	AM_RANGE(0x1e, 0x1e) AM_WRITE( funkyfig_rombank_w		)
	AM_RANGE(0x20, 0x21) AM_WRITE( funkyfig_blitter_w		)
	AM_RANGE(0x40, 0x4f) AM_WRITE( rtc_w					)	// 6242RTC

	// Layers 0-3:
	AM_RANGE(0x60, 0x63) AM_WRITE( dynax_palette_base_w		)
	AM_RANGE(0x64, 0x67) AM_WRITE( dynax_palette_mask_w		)
	AM_RANGE(0x68, 0x6b) AM_WRITE( dynax_transparency_pen_w	)
	AM_RANGE(0x6c, 0x6f) AM_WRITE( dynax_transparency_mask_w)
	AM_RANGE(0x74, 0x74) AM_WRITE( dynax_bgcolor_w			)
	AM_RANGE(0x75, 0x75) AM_WRITE( dynax_priority_w			)
	AM_RANGE(0x76, 0x76) AM_WRITE( dynax_layer_enable_w		)

	AM_RANGE(0x80, 0x80) AM_WRITE( dynax_select2_w			)
	AM_RANGE(0x81, 0x81) AM_WRITE( funkyfig_lockout_w		)
	AM_RANGE(0xa2, 0xa2) AM_WRITE( mmpanic_leds2_w			)
ADDRESS_MAP_END


/* Sound CPU */

static ADDRESS_MAP_START( funkyfig_sound_readport, ADDRESS_SPACE_IO, 8 )	ADDRESS_MAP_FLAGS( AMEF_ABITS(8) )
	AM_RANGE(0x02, 0x02) AM_READ(soundlatch_r		)
	AM_RANGE(0x04, 0x04) AM_READ(MRA8_NOP			)	// read only once at the start
ADDRESS_MAP_END



/***************************************************************************

    Hana Kanzashi

***************************************************************************/

WRITE8_HANDLER( hanakanz_rombank_w )
{
	UINT8 *rom = memory_region(REGION_CPU1);

	memory_set_bankptr(1, &rom[0x10000 + 0x8000 * (data & 0x0f)]);

	memory_set_bankptr(2, &rom[0x90000 + 0x1000 * ((data & 0xf0) >> 4)]);
}

static ADDRESS_MAP_START( hanakanz_readmem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x5fff) AM_READ(MRA8_ROM		)	// ROM
	AM_RANGE(0x6000, 0x6fff) AM_READ(MRA8_RAM		)	// RAM
	AM_RANGE(0x7000, 0x7fff) AM_READ(MRA8_BANK2		)	// RAM (Banked)
	AM_RANGE(0x8000, 0xffff) AM_READ(MRA8_BANK1		)	// ROM (Banked)
ADDRESS_MAP_END
static ADDRESS_MAP_START( hanakanz_writemem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x6000, 0x6fff) AM_WRITE(MWA8_RAM		)	// RAM
	AM_RANGE(0x7000, 0x7fff) AM_WRITE(MWA8_BANK2	)	// RAM (Banked)
ADDRESS_MAP_END


static UINT8 keyb,dsw;
static WRITE8_HANDLER( hanakanz_keyb_w )
{
	keyb = data;
}
static WRITE8_HANDLER( hanakanz_dsw_w )
{
	dsw = data;
}

static READ8_HANDLER( hanakanz_keyb_r )
{
	UINT8 val = 0xff;

	if      (!(keyb & 0x01))	val = readinputport(offset * 5 + 1);
	else if (!(keyb & 0x02))	val = readinputport(offset * 5 + 2);
	else if (!(keyb & 0x04))	val = readinputport(offset * 5 + 3);
	else if (!(keyb & 0x08))	val = readinputport(offset * 5 + 4);
	else if (!(keyb & 0x10))	val = readinputport(offset * 5 + 5);

	val |= readinputport(16 + offset);
	return val;
}

static READ8_HANDLER( hanakanz_dsw_r )
{
	if (!(dsw & 0x01))	return readinputport(11);
	if (!(dsw & 0x02))	return readinputport(12);
	if (!(dsw & 0x04))	return readinputport(13);
	if (!(dsw & 0x08))	return readinputport(14);
	if (!(dsw & 0x10))	return readinputport(15);
	return 0xff;
}

static READ8_HANDLER( hanakanz_busy_r )
{
	return 0x80;	// bit 7 == 0 -> blitter busy
}

static READ8_HANDLER( hanakanz_gfxrom_r )
{
	UINT8 *rom	=	memory_region( REGION_GFX1 );
	size_t size		=	memory_region_length( REGION_GFX1 );
	int address		=	(dynax_blit_address & 0xffffff) * 2;

	static UINT8 romdata[2];

	if (address >= size)
	{
		logerror("CPU#0 PC %06X: Error, Blitter address %06X out of range\n", activecpu_get_pc(), address);
		address %= size;
	}

	if (offset == 0)
	{
		romdata[0] = rom[address + 0];
		romdata[1] = rom[address + 1];

		dynax_blit_address++;

		return romdata[0];
	}
	else
	{
		return romdata[1];
	}
}


static WRITE8_HANDLER( hanakanz_coincounter_w )
{
	// bit 0 = coin counter
	// bit 1 = out counter
	// bit 2 = 1 if bet on
	// bit 3 = 1 if bet off

	coin_counter_w(0, data & 1);

	if (data & 0xf0)
		logerror("%04x: warning, coin counter = %02x\n", activecpu_get_pc(), data);

#ifdef MAME_DEBUG
//      ui_popup("93 = %02x",data);
#endif
}

static WRITE8_HANDLER( hanakanz_palette_w )
{
	static int palette_index;

	if (dynax_blit_reg & 0x80)
	{
		palette_index = data | ((dynax_blit_reg & 1) << 8);
	}
	else
	{
		// 0bbggggg bbbrrrrr
		// 04343210 21043210

		int g = dynax_blit_reg & 0x1f;
		int r = data & 0x1f;
		int b = ((data & 0xe0) >> 5) | ((dynax_blit_reg & 0x60) >> 2);
		r =  (r << 3) | (r >> 2);
		g =  (g << 3) | (g >> 2);
		b =  (b << 3) | (b >> 2);

		palette_set_color((palette_index++)&0x1ff,r,g,b);
	}
}

static WRITE8_HANDLER( hanakanz_oki_bank_w )
{
	OKIM6295_set_bank_base(0, (data & 0x40) ? 0x40000 : 0);
}

static READ8_HANDLER( hanakanz_rand_r )
{
	return mame_rand();
}

static ADDRESS_MAP_START( hanakanz_readport, ADDRESS_SPACE_IO, 8 )	ADDRESS_MAP_FLAGS( AMEF_ABITS(8) )
	AM_RANGE(0x2c, 0x2c) AM_READ( hanakanz_busy_r			)
	AM_RANGE(0x32, 0x32) AM_READ( hanakanz_dsw_r			)
	AM_RANGE(0x83, 0x84) AM_READ( hanakanz_gfxrom_r			)
	AM_RANGE(0x90, 0x90) AM_READ( input_port_0_r			)
	AM_RANGE(0x91, 0x92) AM_READ( hanakanz_keyb_r			)
	AM_RANGE(0x96, 0x96) AM_READ( hanakanz_rand_r			)
	AM_RANGE(0xc0, 0xc0) AM_READ( OKIM6295_status_0_r		)
	AM_RANGE(0xe0, 0xef) AM_READ( rtc_r						)	// 6242RTC
ADDRESS_MAP_END

static ADDRESS_MAP_START( hanakanz_writeport, ADDRESS_SPACE_IO, 8 )	ADDRESS_MAP_FLAGS( AMEF_ABITS(8) )
	AM_RANGE(0x2c, 0x2c) AM_WRITE( hanakanz_oki_bank_w		)
	AM_RANGE(0x2e, 0x2e) AM_WRITE( hanakanz_blitter_reg_w	)
	AM_RANGE(0x30, 0x30) AM_WRITE( hanakanz_rombank_w		)
	AM_RANGE(0x31, 0x31) AM_WRITE( hanakanz_dsw_w			)
	AM_RANGE(0x80, 0x80) AM_WRITE( hanakanz_blitter_data_w	)
	AM_RANGE(0x81, 0x81) AM_WRITE( hanakanz_palette_w		)
	AM_RANGE(0x93, 0x93) AM_WRITE( hanakanz_coincounter_w	)
	AM_RANGE(0x94, 0x94) AM_WRITE( hanakanz_keyb_w			)
	AM_RANGE(0xa0, 0xa0) AM_WRITE( YM2413_register_port_0_w	)
	AM_RANGE(0xa1, 0xa1) AM_WRITE( YM2413_data_port_0_w		)
	AM_RANGE(0xc0, 0xc0) AM_WRITE( OKIM6295_data_0_w		)
	AM_RANGE(0xe0, 0xef) AM_WRITE( rtc_w					)	// 6242RTC
ADDRESS_MAP_END


static ADDRESS_MAP_START( hkagerou_readport, ADDRESS_SPACE_IO, 8 )	ADDRESS_MAP_FLAGS( AMEF_ABITS(8) )
	AM_RANGE(0x2c, 0x2c) AM_READ( hanakanz_busy_r			)
	AM_RANGE(0x32, 0x32) AM_READ( hanakanz_dsw_r			)
	AM_RANGE(0x83, 0x84) AM_READ( hanakanz_gfxrom_r			)
	AM_RANGE(0xb0, 0xb0) AM_READ( input_port_0_r			)
	AM_RANGE(0xb1, 0xb2) AM_READ( hanakanz_keyb_r			)
	AM_RANGE(0xb6, 0xb6) AM_READ( hanakanz_rand_r			)
	AM_RANGE(0xc0, 0xc0) AM_READ( OKIM6295_status_0_r		)
	AM_RANGE(0xe0, 0xef) AM_READ( rtc_r						)	// 6242RTC
ADDRESS_MAP_END

static ADDRESS_MAP_START( hkagerou_writeport, ADDRESS_SPACE_IO, 8 )	ADDRESS_MAP_FLAGS( AMEF_ABITS(8) )
	AM_RANGE(0x2c, 0x2c) AM_WRITE( hanakanz_oki_bank_w		)
	AM_RANGE(0x2e, 0x2e) AM_WRITE( hanakanz_blitter_reg_w	)
	AM_RANGE(0x30, 0x30) AM_WRITE( hanakanz_rombank_w		)
	AM_RANGE(0x31, 0x31) AM_WRITE( hanakanz_dsw_w			)
	AM_RANGE(0x80, 0x80) AM_WRITE( hanakanz_blitter_data_w	)
	AM_RANGE(0x81, 0x81) AM_WRITE( hanakanz_palette_w		)
	AM_RANGE(0xa0, 0xa0) AM_WRITE( YM2413_register_port_0_w	)
	AM_RANGE(0xa1, 0xa1) AM_WRITE( YM2413_data_port_0_w		)
	AM_RANGE(0xb3, 0xb3) AM_WRITE( hanakanz_coincounter_w	)
	AM_RANGE(0xb4, 0xb4) AM_WRITE( hanakanz_keyb_w			)
	AM_RANGE(0xc0, 0xc0) AM_WRITE( OKIM6295_data_0_w		)
	AM_RANGE(0xe0, 0xef) AM_WRITE( rtc_w					)	// 6242RTC
ADDRESS_MAP_END


static UINT8 mjreach1_protection_val;

static WRITE8_HANDLER( mjreach1_protection_w )
{
	mjreach1_protection_val = data;
}

static READ8_HANDLER( mjreach1_protection_r )
{
	return mjreach1_protection_val;
}

static ADDRESS_MAP_START( mjreach1_readport, ADDRESS_SPACE_IO, 8 )	ADDRESS_MAP_FLAGS( AMEF_ABITS(8) )
	AM_RANGE(0x2c, 0x2c) AM_READ( hanakanz_busy_r			)
	AM_RANGE(0x32, 0x32) AM_READ( hanakanz_dsw_r			)
	AM_RANGE(0x83, 0x84) AM_READ( hanakanz_gfxrom_r			)
	AM_RANGE(0x92, 0x92) AM_READ( hanakanz_rand_r			)
	AM_RANGE(0x93, 0x93) AM_READ( mjreach1_protection_r		)
	AM_RANGE(0x94, 0x94) AM_READ( input_port_0_r			)
	AM_RANGE(0x95, 0x96) AM_READ( hanakanz_keyb_r			)
	AM_RANGE(0xc0, 0xc0) AM_READ( OKIM6295_status_0_r		)
	AM_RANGE(0xe0, 0xef) AM_READ( rtc_r						)	// 6242RTC
ADDRESS_MAP_END

static ADDRESS_MAP_START( mjreach1_writeport, ADDRESS_SPACE_IO, 8 )	ADDRESS_MAP_FLAGS( AMEF_ABITS(8) )
	AM_RANGE(0x2c, 0x2c) AM_WRITE( hanakanz_oki_bank_w		)
	AM_RANGE(0x2e, 0x2e) AM_WRITE( hanakanz_blitter_reg_w	)
	AM_RANGE(0x30, 0x30) AM_WRITE( hanakanz_rombank_w		)
	AM_RANGE(0x31, 0x31) AM_WRITE( hanakanz_dsw_w			)
	AM_RANGE(0x80, 0x80) AM_WRITE( hanakanz_blitter_data_w	)
	AM_RANGE(0x81, 0x81) AM_WRITE( hanakanz_palette_w		)
	AM_RANGE(0x90, 0x90) AM_WRITE( hanakanz_keyb_w			)
	AM_RANGE(0x93, 0x93) AM_WRITE( mjreach1_protection_w	)
	AM_RANGE(0x97, 0x97) AM_WRITE( hanakanz_coincounter_w	)
	AM_RANGE(0xa0, 0xa0) AM_WRITE( YM2413_register_port_0_w	)
	AM_RANGE(0xa1, 0xa1) AM_WRITE( YM2413_data_port_0_w		)
	AM_RANGE(0xc0, 0xc0) AM_WRITE( OKIM6295_data_0_w		)
	AM_RANGE(0xe0, 0xef) AM_WRITE( rtc_w					)	// 6242RTC
ADDRESS_MAP_END


/***************************************************************************
     Mahjong Chuukanejyo
***************************************************************************/

static READ8_HANDLER( mjchuuka_keyb_r )
{
	UINT8 val = 0xff;

	if      (!(keyb & 0x01))	val = readinputport(offset * 5 + 1);
	else if (!(keyb & 0x02))	val = readinputport(offset * 5 + 2);
	else if (!(keyb & 0x04))	val = readinputport(offset * 5 + 3);
	else if (!(keyb & 0x08))	val = readinputport(offset * 5 + 4);
	else if (!(keyb & 0x10))	val = readinputport(offset * 5 + 5);

	val |= readinputport(16 + offset);
	if (offset)	val |= 0x80;	// blitter busy
	return val;
}

static WRITE8_HANDLER( mjchuuka_blitter_w )
{
	hanakanz_blitter_reg_w(0,offset >> 8);
	hanakanz_blitter_data_w(0,data);
}

static UINT8 mjchuuka_romdata[2];

static void mjchuuka_get_romdata(void)
{
	UINT8 *rom	=	memory_region( REGION_GFX1 );
	size_t size		=	memory_region_length( REGION_GFX1 );
	int address		=	(dynax_blit_address & 0xffffff) * 2;

	if (address >= size)
	{
		logerror("CPU#0 PC %06X: Error, Blitter address %06X out of range\n", activecpu_get_pc(), address);
		address %= size;
	}

	mjchuuka_romdata[0] = rom[address + 0];
	mjchuuka_romdata[1] = rom[address + 1];
}

static READ8_HANDLER( mjchuuka_gfxrom_0_r )
{
	mjchuuka_get_romdata();
	dynax_blit_address++;
	return mjchuuka_romdata[0];
}
static READ8_HANDLER( mjchuuka_gfxrom_1_r )
{
	return mjchuuka_romdata[1];
}

static WRITE8_HANDLER( mjchuuka_palette_w )
{
	static int palette_index;
	UINT16 rgb = (offset & 0xff00) | data;

	if (rgb & 0x8000)
	{
		palette_index = rgb & 0x1ff;
	}
	else
	{
		// 0bbggggg bbbrrrrr
		// 04343210 21043210

		int r = (rgb >> 0) & 0x1f;
		int g = (rgb >> 8) & 0x1f;
		int b = ((rgb >> 5) & 0x07) | ((rgb & 0x6000) >> 10);
		r =  (r << 3) | (r >> 2);
		g =  (g << 3) | (g >> 2);
		b =  (b << 3) | (b >> 2);

		palette_set_color((palette_index++)&0x1ff,r,g,b);
	}
}

static WRITE8_HANDLER( mjchuuka_coincounter_w )
{
	// bit 0 = in counter
	// bit 1 = out counter
	// bit 3 = lockout
	// bit 8?

	coin_counter_w(0, data   & 0x01);
	coin_lockout_w(0,(~data) & 0x08);

	if (data & 0x74)
		logerror("%04x: warning, coin counter = %02x\n", activecpu_get_pc(), data);

#ifdef MAME_DEBUG
//    ui_popup("40 = %02x",data);
#endif
}

static WRITE8_HANDLER( mjchuuka_oki_bank_w )
{
	// data & 0x08 ?
	OKIM6295_set_bank_base(0, (data & 0x01) ? 0x40000 : 0);

#ifdef MAME_DEBUG
//    ui_popup("1e = %02x",data);
#endif
}

static ADDRESS_MAP_START( mjchuuka_readport, ADDRESS_SPACE_IO, 8 )	// 16 bit I/O
	AM_RANGE(0x13, 0x13) AM_MIRROR(0xff00) AM_READ( hanakanz_rand_r			)
	AM_RANGE(0x23, 0x23) AM_MIRROR(0xff00) AM_READ( mjchuuka_gfxrom_0_r		)
	AM_RANGE(0x42, 0x42) AM_MIRROR(0xff00) AM_READ( input_port_0_r			)
	AM_RANGE(0x43, 0x44) AM_MIRROR(0xff00) AM_READ( mjchuuka_keyb_r			)
	AM_RANGE(0x45, 0x45) AM_MIRROR(0xff00) AM_READ( mjchuuka_gfxrom_1_r		)
	AM_RANGE(0x60, 0x60) AM_MIRROR(0xff00) AM_READ( input_port_11_r			)	// DSW 1
	AM_RANGE(0x61, 0x61) AM_MIRROR(0xff00) AM_READ( input_port_12_r			)	// DSW 2
	AM_RANGE(0x62, 0x62) AM_MIRROR(0xff00) AM_READ( input_port_13_r			)	// DSW 3
	AM_RANGE(0x63, 0x63) AM_MIRROR(0xff00) AM_READ( input_port_14_r			)	// DSW 4
	AM_RANGE(0x64, 0x64) AM_MIRROR(0xff00) AM_READ( input_port_15_r			)	// DSW 1-4 high bits
	AM_RANGE(0x80, 0x80) AM_MIRROR(0xff00) AM_READ( OKIM6295_status_0_r		)
	AM_RANGE(0xc0, 0xcf) AM_MIRROR(0xff00) AM_READ( rtc_r					)	// 6242RTC
ADDRESS_MAP_END

static ADDRESS_MAP_START( mjchuuka_writeport, ADDRESS_SPACE_IO, 8 )	// 16 bit I/O
	AM_RANGE(0x1c, 0x1c) AM_MIRROR(0xff00) AM_WRITE( hanakanz_rombank_w			)
	AM_SPACE(0x20, 0xff)                   AM_WRITE( mjchuuka_blitter_w			)
	AM_SPACE(0x21, 0xff)                   AM_WRITE( mjchuuka_palette_w			)
	AM_RANGE(0x40, 0x40) AM_MIRROR(0xff00) AM_WRITE( mjchuuka_coincounter_w		)
	AM_RANGE(0x41, 0x41) AM_MIRROR(0xff00) AM_WRITE( hanakanz_keyb_w			)
	AM_RANGE(0x80, 0x80) AM_MIRROR(0xff00) AM_WRITE( OKIM6295_data_0_w			)
	AM_RANGE(0xc0, 0xcf) AM_MIRROR(0xff00) AM_WRITE( rtc_w						)	// 6242RTC
	AM_RANGE(0xa0, 0xa0) AM_MIRROR(0xff00) AM_WRITE( YM2413_register_port_0_w	)
	AM_RANGE(0xa1, 0xa1) AM_MIRROR(0xff00) AM_WRITE( YM2413_data_port_0_w		)
	AM_RANGE(0xe0, 0xe0) AM_MIRROR(0xff00) AM_WRITE( AY8910_control_port_0_w	)
	AM_RANGE(0xe1, 0xe1) AM_MIRROR(0xff00) AM_WRITE( AY8910_write_port_0_w		)
	AM_RANGE(0x1e, 0x1e) AM_MIRROR(0xff00) AM_WRITE( mjchuuka_oki_bank_w		)
ADDRESS_MAP_END


/***************************************************************************
                        Mahjong The Mysterious World
***************************************************************************/

static ADDRESS_MAP_START( mjmyster_readmem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x5fff) AM_READ( MRA8_ROM				)	// ROM
	AM_RANGE(0x6000, 0x6fff) AM_READ( MRA8_RAM				)	// RAM
	AM_RANGE(0x7000, 0x7fff) AM_READ( MRA8_BANK2			)	// RAM (Banked)
	AM_RANGE(0x8000, 0xffff) AM_READ( MRA8_BANK1			)	// ROM/RAM (Banked)
ADDRESS_MAP_END

static ADDRESS_MAP_START( mjmyster_writemem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x6000, 0x6fff) AM_WRITE( MWA8_RAM				)	// RAM
	AM_RANGE(0x7000, 0x7fff) AM_WRITE( MWA8_BANK2			)	// RAM (Banked)
	AM_RANGE(0xf000, 0xf1ff) AM_WRITE( rongrong_palette_w	)	// RAM enabled by bit 4 of rombank
	AM_RANGE(0xf200, 0xffff) AM_WRITE( MWA8_NOP				)	// ""
ADDRESS_MAP_END

static WRITE8_HANDLER( mjmyster_rambank_w )
{
	UINT8 *rom = memory_region(REGION_CPU1);
	memory_set_bankptr(2, &rom[0x90000 + 0x1000 * (data & 0x07)]);
//  logerror("%04x: rambank = %02x\n", activecpu_get_pc(), data);
}

static WRITE8_HANDLER( mjmyster_select2_w )
{
	dynax_select2 = data;

	if (data & 0x80)	keyb = 1;
}

static READ8_HANDLER( mjmyster_coins_r )
{
	switch( dynax_select2 )
	{
		case 0x00:	return readinputport(0);
		case 0x01:	return 0xff;
		case 0x02:	return 0xff;	// bit 7 = 0 -> blitter busy, + hopper switch
		case 0x03:	return 0xff;
	}

	logerror("%06x: warning, unknown bits read, dynax_select2 = %02x\n", activecpu_get_pc(), dynax_select2);

	return 0xff;
}

static READ8_HANDLER( mjmyster_keyb_r )
{
	UINT8 ret = 0xff;

	if		(keyb & 0x01)	ret = readinputport(1);
	else if	(keyb & 0x02)	ret = readinputport(2);
	else if	(keyb & 0x04)	ret = readinputport(3);
	else if	(keyb & 0x08)	ret = readinputport(4);
	else if	(keyb & 0x10)	ret = readinputport(5);
	else	logerror("%06x: warning, unknown bits read, keyb = %02x\n", activecpu_get_pc(), keyb);

	keyb <<= 1;

	return ret;
}

static READ8_HANDLER( mjmyster_dsw_r )
{
	if (!(dynax_select & 0x01))	return readinputport(9);
	if (!(dynax_select & 0x02))	return readinputport(8);
	if (!(dynax_select & 0x04))	return readinputport(7);
	if (!(dynax_select & 0x08))	return readinputport(6);
	if (!(dynax_select & 0x10))	return readinputport(10);
	logerror("%06x: warning, unknown bits read, dynax_select = %02x\n", activecpu_get_pc(), dynax_select);
	return 0xff;
}

static WRITE8_HANDLER( mjmyster_coincounter_w )
{
	switch( dynax_select2 )
	{
		case 0x0c:
			coin_counter_w(0, (~data) & 0x01);	// coin in
			coin_counter_w(0, (~data) & 0x02);	// coin out actually
			#ifdef MAME_DEBUG
//              ui_popup("cc: %02x",data);
			#endif

			break;

		default:
			logerror("%06x: warning, unknown bits written, dynax_select2 = %02x, data = %02x\n", activecpu_get_pc(), dynax_select2, data);
	}
}

static WRITE8_HANDLER( mjmyster_blitter_w )
{
	blitter_w(0,offset,data,0xfc);
}

static ADDRESS_MAP_START( mjmyster_readport, ADDRESS_SPACE_IO, 8 )	ADDRESS_MAP_FLAGS( AMEF_ABITS(8) )
	AM_RANGE(0x03, 0x03) AM_READ( rongrong_gfxrom_r			)
	AM_RANGE(0x22, 0x22) AM_READ( mjmyster_coins_r			)
	AM_RANGE(0x23, 0x23) AM_READ( mjmyster_keyb_r			)
	AM_RANGE(0x40, 0x40) AM_READ( OKIM6295_status_0_r		)
	AM_RANGE(0x44, 0x44) AM_READ( AY8910_read_port_0_r		)
	AM_RANGE(0x60, 0x6f) AM_READ( rtc_r						)	// 6242RTC
	AM_RANGE(0x98, 0x98) AM_READ( unk_r						)	// ? must be 78 on startup
	AM_RANGE(0xc2, 0xc2) AM_READ( hanakanz_rand_r			)
	AM_RANGE(0xc3, 0xc3) AM_READ( mjmyster_dsw_r			)
ADDRESS_MAP_END

static ADDRESS_MAP_START( mjmyster_writeport, ADDRESS_SPACE_IO, 8 )	ADDRESS_MAP_FLAGS( AMEF_ABITS(8) )
	AM_RANGE(0x00, 0x01) AM_WRITE( mjmyster_blitter_w		)
	AM_RANGE(0x1c, 0x1c) AM_WRITE( mjmyster_rambank_w		)
	AM_RANGE(0x1e, 0x1e) AM_WRITE( mmpanic_rombank_w		)
	AM_RANGE(0x20, 0x20) AM_WRITE( mjmyster_select2_w		)
	AM_RANGE(0x21, 0x21) AM_WRITE( mjmyster_coincounter_w	)
	AM_RANGE(0x40, 0x40) AM_WRITE( OKIM6295_data_0_w		)
	AM_RANGE(0x42, 0x42) AM_WRITE( YM2413_register_port_0_w	)
	AM_RANGE(0x43, 0x43) AM_WRITE( YM2413_data_port_0_w		)
	AM_RANGE(0x46, 0x46) AM_WRITE( AY8910_write_port_0_w	)
	AM_RANGE(0x48, 0x48) AM_WRITE( AY8910_control_port_0_w	)
	AM_RANGE(0x60, 0x6f) AM_WRITE( rtc_w					)	// 6242RTC
	AM_RANGE(0x80, 0x83) AM_WRITE( dynax_palette_base_w		)
	AM_RANGE(0x84, 0x87) AM_WRITE( dynax_palette_mask_w		)
	AM_RANGE(0x88, 0x8b) AM_WRITE( dynax_transparency_pen_w	)
	AM_RANGE(0x8c, 0x8f) AM_WRITE( dynax_transparency_mask_w)
	AM_RANGE(0x94, 0x94) AM_WRITE( dynax_bgcolor_w			)
	AM_RANGE(0x95, 0x95) AM_WRITE( dynax_priority_w			)
	AM_RANGE(0x96, 0x96) AM_WRITE( dynax_layer_enable_w		)
ADDRESS_MAP_END





INPUT_PORTS_START( ddenlovr )
	PORT_START	// IN0 - Player 1
	PORT_BIT(  0x01, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT(  0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_PLAYER(1)
	PORT_BIT(  0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_PLAYER(1)
	PORT_BIT(  0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_PLAYER(1)
	PORT_BIT(  0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_PLAYER(1)
	PORT_BIT(  0x20, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(1)
	PORT_BIT(  0x40, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(1)
	PORT_BIT(  0x80, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_PLAYER(1)

	PORT_START	// IN1 - Player 2
	PORT_BIT(  0x01, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT(  0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_PLAYER(2)
	PORT_BIT(  0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_PLAYER(2)
	PORT_BIT(  0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_PLAYER(2)
	PORT_BIT(  0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_PLAYER(2)
	PORT_BIT(  0x20, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(2)
	PORT_BIT(  0x40, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(2)
	PORT_BIT(  0x80, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_PLAYER(2)

	PORT_START	// IN2 - Coins + ?
	PORT_BIT(  0x01, IP_ACTIVE_LOW,  IPT_COIN1    )
	PORT_BIT(  0x02, IP_ACTIVE_LOW,  IPT_COIN2    )
	PORT_BIT(  0x04, IP_ACTIVE_LOW,  IPT_SERVICE1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW,  IPT_SERVICE ) PORT_NAME(DEF_STR( Test )) PORT_CODE(KEYCODE_F1)	/* Test */
	PORT_BIT(  0x10, IP_ACTIVE_LOW,  IPT_UNKNOWN  )
	PORT_BIT(  0x20, IP_ACTIVE_LOW,  IPT_SPECIAL  )	// ? quiz365
	PORT_BIT(  0x40, IP_ACTIVE_HIGH, IPT_SPECIAL  )	// blitter irq flag
	PORT_BIT(  0x80, IP_ACTIVE_HIGH, IPT_SPECIAL  )	// blitter busy flag

	/* one of the dips selects the nudity level */
	PORT_START	// IN3 - DSW
	PORT_SERVICE( 0x01, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0x02, 0x02, "Unknown 1-1" )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, "Unknown 1-2" )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, "Unknown 1-3" )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, "Unknown 1-4" )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, "Unknown 1-5*" )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, "Unknown 1-6*" )	// 6&7
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, "Unknown 1-7*" )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
INPUT_PORTS_END


INPUT_PORTS_START( nettoqc )
	PORT_START	// IN0 - Player 1
	PORT_BIT(  0x01, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT(  0x02, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(1)
	PORT_BIT(  0x04, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(1)
	PORT_BIT(  0x08, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_PLAYER(1)
	PORT_BIT(  0x10, IP_ACTIVE_LOW, IPT_BUTTON4 ) PORT_PLAYER(1)
	PORT_BIT(  0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	// IN1 - Player 2
	PORT_BIT(  0x01, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT(  0x02, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(2)
	PORT_BIT(  0x04, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(2)
	PORT_BIT(  0x08, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_PLAYER(2)
	PORT_BIT(  0x10, IP_ACTIVE_LOW, IPT_BUTTON4 ) PORT_PLAYER(2)
	PORT_BIT(  0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	// IN2 - Coins + ?
	PORT_BIT(  0x01, IP_ACTIVE_LOW,  IPT_COIN1    )
	PORT_BIT(  0x02, IP_ACTIVE_LOW,  IPT_COIN2    )
	PORT_BIT(  0x04, IP_ACTIVE_LOW,  IPT_SERVICE1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW,  IPT_SERVICE ) PORT_NAME(DEF_STR( Test )) PORT_CODE(KEYCODE_F1)	/* Test */
	PORT_BIT(  0x10, IP_ACTIVE_HIGH, IPT_SPECIAL  )	// blitter busy flag
	PORT_BIT(  0x20, IP_ACTIVE_HIGH, IPT_SPECIAL  )	// ?
	PORT_BIT(  0x40, IP_ACTIVE_HIGH, IPT_SPECIAL  )	// blitter irq flag
	PORT_BIT(  0x80, IP_ACTIVE_HIGH, IPT_SPECIAL  )

	PORT_START	// IN3 - DSW
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 1C_2C ) )
	PORT_DIPNAME( 0x0c, 0x0c, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x0c, "1" )
	PORT_DIPSETTING(    0x08, "2" )
	PORT_DIPSETTING(    0x04, "3" )
	PORT_DIPSETTING(    0x00, "5" )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x10, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Free_Play ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, "Unknown 1-6*" )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, "Unknown 1-7*" )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START	// IN4 - DSW
	PORT_DIPNAME( 0x03, 0x03, "Unknown 2-0&1*" )
	PORT_DIPSETTING(    0x02, "0" )
	PORT_DIPSETTING(    0x03, "1" )
	PORT_DIPSETTING(    0x01, "2" )
	PORT_DIPSETTING(    0x00, "3" )
	PORT_DIPNAME( 0x04, 0x04, "Unknown 2-2" )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, "Unknown 2-3*" )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, "Unknown 2-4*" )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, "Unknown 2-5*" )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, "Unknown 2-6*" )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, "Unknown 2-7*" )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START	// IN5 - DSW
	PORT_DIPNAME( 0x01, 0x01, "Unknown 1-8*" )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_SERVICE( 0x02, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0x04, 0x04, "Unknown 2-8*" )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, "Unknown 2-9*" )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_BIT( 0x70, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_DIPNAME( 0x80, 0x00, "Detailed Tests" )	// menu "8 OPTION" in service mode
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
INPUT_PORTS_END


INPUT_PORTS_START( quiz365 )
	PORT_START	// IN0 - Player 1
	PORT_BIT(  0x01, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT(  0x02, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(1)
	PORT_BIT(  0x04, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(1)
	PORT_BIT(  0x08, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_PLAYER(1)
	PORT_BIT(  0x10, IP_ACTIVE_LOW, IPT_BUTTON4 ) PORT_PLAYER(1)
	PORT_BIT(  0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	// IN1 - Player 2
	PORT_BIT(  0x01, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT(  0x02, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(2)
	PORT_BIT(  0x04, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(2)
	PORT_BIT(  0x08, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_PLAYER(2)
	PORT_BIT(  0x10, IP_ACTIVE_LOW, IPT_BUTTON4 ) PORT_PLAYER(2)
	PORT_BIT(  0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	// IN2 - Coins + ?
	PORT_BIT(  0x01, IP_ACTIVE_LOW,  IPT_COIN1    )
	PORT_BIT(  0x02, IP_ACTIVE_LOW,  IPT_COIN2    )
	PORT_BIT(  0x04, IP_ACTIVE_LOW,  IPT_SERVICE1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW,  IPT_SERVICE ) PORT_NAME(DEF_STR( Test )) PORT_CODE(KEYCODE_F1)	/* Test */
	PORT_BIT(  0x10, IP_ACTIVE_LOW,  IPT_UNKNOWN  )
	PORT_BIT(  0x20, IP_ACTIVE_LOW,  IPT_SPECIAL  )	// ? quiz365
	PORT_BIT(  0x40, IP_ACTIVE_HIGH, IPT_SPECIAL  )	// blitter irq flag
	PORT_BIT(  0x80, IP_ACTIVE_HIGH, IPT_SPECIAL  )	// blitter busy flag

	PORT_START	// IN3 - DSW
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 1C_2C ) )
	PORT_DIPNAME( 0x0c, 0x0c, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x0c, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 1C_2C ) )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x10, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, "Unknown 1-5*" )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0xc0, 0xc0, "Unknown 1-6&7" )
	PORT_DIPSETTING(    0x40, "0" )
	PORT_DIPSETTING(    0x80, "1" )
	PORT_DIPSETTING(    0xc0, "2" )
//  PORT_DIPSETTING(    0x00, "2" )

	PORT_START	// IN4 - DSW
	PORT_DIPNAME( 0x03, 0x03, "Unknown 2-0&1" )
	PORT_DIPSETTING(    0x02, "0" )
	PORT_DIPSETTING(    0x03, "1" )
	PORT_DIPSETTING(    0x01, "2" )
	PORT_DIPSETTING(    0x00, "3" )
	PORT_DIPNAME( 0x0c, 0x0c, "Unknown 2-2&3" )
	PORT_DIPSETTING(    0x08, "0" )
	PORT_DIPSETTING(    0x0c, "1" )
	PORT_DIPSETTING(    0x04, "2" )
	PORT_DIPSETTING(    0x00, "3" )
	PORT_DIPNAME( 0x10, 0x10, "Unknown 2-4" )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, "Unknown 2-5" )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, "Unknown 2-6" )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, "Unknown 2-7" )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START	// IN5 - DSW
	PORT_DIPNAME( 0x01, 0x01, "Unknown 3-0" )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_SERVICE( 0x02, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0x04, 0x00, "Detailed Tests" )	// menu "8 OPTION" in service mode
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, "Unknown 3-3" )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_BIT( 0xf0, IP_ACTIVE_LOW, IPT_UNKNOWN )
INPUT_PORTS_END


INPUT_PORTS_START( rongrong )
	PORT_START	// IN0 - Player 1
	PORT_BIT(  0x01, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT(  0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_PLAYER(1)
	PORT_BIT(  0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_PLAYER(1)
	PORT_BIT(  0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_PLAYER(1)
	PORT_BIT(  0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_PLAYER(1)
	PORT_BIT(  0x20, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(1)
	PORT_BIT(  0x40, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(1)
	PORT_BIT(  0x80, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_PLAYER(1)

	PORT_START	// IN1 - Player 2
	PORT_BIT(  0x01, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT(  0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_PLAYER(2)
	PORT_BIT(  0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_PLAYER(2)
	PORT_BIT(  0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_PLAYER(2)
	PORT_BIT(  0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_PLAYER(2)
	PORT_BIT(  0x20, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(2)
	PORT_BIT(  0x40, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(2)
	PORT_BIT(  0x80, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_PLAYER(2)

	PORT_START	// IN2 - Coins + ?
	PORT_BIT(  0x01, IP_ACTIVE_LOW, IPT_COIN1    )
	PORT_BIT(  0x02, IP_ACTIVE_LOW, IPT_COIN2    )
	PORT_BIT(  0x04, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT(  0x08, IP_ACTIVE_LOW, IPT_SERVICE ) PORT_NAME(DEF_STR( Test )) PORT_CODE(KEYCODE_F1)	/* Test */
	PORT_BIT(  0x10, IP_ACTIVE_LOW, IPT_UNKNOWN  )
	PORT_BIT(  0x20, IP_ACTIVE_LOW, IPT_SPECIAL  )	// ? quiz365
	PORT_BIT(  0x40, IP_ACTIVE_LOW, IPT_SPECIAL  )	// ? blitter irq flag ?
	PORT_BIT(  0x80, IP_ACTIVE_LOW, IPT_SPECIAL  )	// ? blitter busy flag ?

  	PORT_START	// IN3 - DSW
 	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 2C_1C ) )
 	PORT_DIPSETTING(    0x03, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 1C_2C ) )
 	PORT_DIPNAME( 0x0c, 0x0c, DEF_STR( Coin_B ) )
 	PORT_DIPSETTING(    0x00, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 2C_1C ) )
 	PORT_DIPSETTING(    0x0c, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 1C_2C ) )
 	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Free_Play ) )
  	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
  	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
 	PORT_DIPNAME( 0x20, 0x20, "Help" )
 	PORT_DIPSETTING(    0x20, "3" )
 	PORT_DIPSETTING(    0x00, "2" )
 	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
 	PORT_DIPSETTING(    0x40, DEF_STR( On ) )
 	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )
  	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
  	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

  	PORT_START	// IN4 - DSW
 	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Unknown ) )
  	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
  	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
 	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Unknown ) )
  	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
  	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
 	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unknown ) )
  	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
  	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
 	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unknown ) )
  	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
  	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
 	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unknown ) )
  	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
  	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
 	PORT_DIPNAME( 0x20, 0x20, "VS Round" )
 	PORT_DIPSETTING(    0x20, "1" )
 	PORT_DIPSETTING(    0x00, "2" )
 	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unknown ) )
  	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
  	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
 	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )
  	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
  	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

  	PORT_START	// IN5 - DSW
 	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Unknown ) )
  	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
  	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
 	PORT_DIPNAME( 0x02, 0x02, "Test Mode" )
  	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
  	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
 	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unknown ) )
  	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
  	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
 	PORT_DIPNAME( 0x08, 0x08, "Select Round" )
  	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
  	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
  	PORT_BIT( 0xf0, IP_ACTIVE_LOW, IPT_UNKNOWN )
INPUT_PORTS_END

INPUT_PORTS_START( quizchq )
	PORT_START	// IN0 - Player 1
	PORT_BIT(  0x01, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT(  0x02, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(1)
	PORT_BIT(  0x04, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(1)
	PORT_BIT(  0x08, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_PLAYER(1)
	PORT_BIT(  0x10, IP_ACTIVE_LOW, IPT_BUTTON4 ) PORT_PLAYER(1)
	PORT_BIT(  0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	// IN1 - Player 2
	PORT_BIT(  0x01, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT(  0x02, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(2)
	PORT_BIT(  0x04, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(2)
	PORT_BIT(  0x08, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_PLAYER(2)
	PORT_BIT(  0x10, IP_ACTIVE_LOW, IPT_BUTTON4 ) PORT_PLAYER(2)
	PORT_BIT(  0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	// IN2 - Coins + ?
	PORT_BIT(  0x01, IP_ACTIVE_LOW, IPT_COIN1    )
	PORT_BIT(  0x02, IP_ACTIVE_LOW, IPT_COIN2    )
	PORT_BIT(  0x04, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_SERVICE ) PORT_NAME(DEF_STR( Test )) PORT_CODE(KEYCODE_F1)	/* Test */
	PORT_BIT(  0x10, IP_ACTIVE_LOW, IPT_UNKNOWN  )
	PORT_BIT(  0x20, IP_ACTIVE_LOW, IPT_SPECIAL  )	// ? quiz365
	PORT_BIT(  0x40, IP_ACTIVE_LOW, IPT_SPECIAL  )	// ? blitter irq flag ?
	PORT_BIT(  0x80, IP_ACTIVE_LOW, IPT_SPECIAL  )	// ? blitter busy flag ?

	PORT_START	// IN3 - DSW
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 1C_2C ) )
	PORT_DIPNAME( 0x0c, 0x0c, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x0c, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 1C_2C ) )
	PORT_DIPNAME( 0x10, 0x10, "Unknown 1-4" )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Demo_Sounds ))
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x20, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, "Unknown 1-6" )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, "Set Date" )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START	// IN4 - DSW
	PORT_DIPNAME( 0x01, 0x01, "Unknown 2-0" )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, "Unknown 2-1" )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, "Unknown 2-2" )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, "Unknown 2-3" )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, "Unknown 2-4" )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, "Unknown 2-5" )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, "Unknown 2-6" )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_SERVICE( 0x80, IP_ACTIVE_LOW )

	PORT_START	// IN5 - DSW
	PORT_DIPNAME( 0x01, 0x01, "Unknown 1-0" )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, "Unknown 1-1" )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, "Unknown 1-2" )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, "Unknown 1-3" )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_BIT( 0xf0, IP_ACTIVE_LOW, IPT_UNKNOWN )
INPUT_PORTS_END

INPUT_PORTS_START( mmpanic )
	PORT_START	// IN0 6a (68 = 1:used? 2:normal 3:goes to 69)
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 ) PORT_IMPULSE(2)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )	// tested?
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_SERVICE ) PORT_NAME(DEF_STR( Test )) PORT_CODE(KEYCODE_F1)	/* Test */
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )	// tested?
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_OTHER ) PORT_NAME("8") PORT_CODE(KEYCODE_8_PAD)
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_OTHER ) PORT_NAME("9") PORT_CODE(KEYCODE_9_PAD)
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_SPECIAL )	// busy?

	PORT_START	// IN1 6b (68 = 0 & 1)
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_OTHER ) PORT_NAME("1") PORT_CODE(KEYCODE_1_PAD)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_OTHER ) PORT_NAME("2") PORT_CODE(KEYCODE_2_PAD)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_OTHER ) PORT_NAME("3") PORT_CODE(KEYCODE_3_PAD)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_OTHER ) PORT_NAME("4") PORT_CODE(KEYCODE_4_PAD)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_OTHER ) PORT_NAME("5") PORT_CODE(KEYCODE_5_PAD)
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_OTHER ) PORT_NAME("6") PORT_CODE(KEYCODE_6_PAD)
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_OTHER ) PORT_NAME("7") PORT_CODE(KEYCODE_7_PAD)

	PORT_START	// IN2 - DSW
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 1C_2C ) )
	PORT_DIPNAME( 0x1c, 0x10, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x1c, "0" )
	PORT_DIPSETTING(    0x18, "1" )
	PORT_DIPSETTING(    0x14, "2" )
	PORT_DIPSETTING(    0x10, "3" )
	PORT_DIPSETTING(    0x0c, "4" )
	PORT_DIPSETTING(    0x08, "5" )
//  PORT_DIPSETTING(    0x04, "5" )
//  PORT_DIPSETTING(    0x00, "5" )
	PORT_DIPNAME( 0x20, 0x20, "Linked Cabinets" )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x40, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, "Unknown 1-7*" )	// 2-0 is related to the same thing (flip?)
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START	// IN3 - DSW
	PORT_DIPNAME( 0x01, 0x01, "Unknown 2-0*" )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x0c, 0x0c, "Unknown 2-2&3*" )
	PORT_DIPSETTING(    0x0c, "10" )
	PORT_DIPSETTING(    0x08, "20" )
	PORT_DIPSETTING(    0x04, "40" )
	PORT_DIPSETTING(    0x00, "80" )
	PORT_DIPNAME( 0x10, 0x10, "Unknown 2-4*" )	// used?
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, "Unknown 2-5*" )	// used?
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, "Unknown 2-6*" )	// 6 & 7?
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, "Unknown 2-7*" )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START	// IN4 - DSW
	PORT_DIPNAME( 0x01, 0x01, "Set Date" )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_SERVICE( 0x02, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0x04, 0x04, "Unknown 3-2*" )	// used?
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, "Unknown 3-3*" )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, "Unknown 3-4" )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, "Unknown 3-5" )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, "Unknown 3-6*" )	// used?
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, "Unknown 3-7*" )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
INPUT_PORTS_END


INPUT_PORTS_START( animaljr )
	PORT_START	// IN0 6a (68 = 1:used? 2:normal 3:goes to 69)
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 ) PORT_IMPULSE(2)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_SERVICE ) PORT_NAME(DEF_STR( Test )) PORT_CODE(KEYCODE_F1)	/* Test */
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_SPECIAL )	// busy?

	PORT_START	// IN1 6b (68 = 0 & 1)
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )	// tested ?
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_OTHER ) PORT_NAME("1") PORT_CODE(KEYCODE_1_PAD)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_OTHER ) PORT_NAME("2") PORT_CODE(KEYCODE_2_PAD)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_OTHER ) PORT_NAME("3") PORT_CODE(KEYCODE_3_PAD)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_OTHER ) PORT_NAME("4") PORT_CODE(KEYCODE_4_PAD)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_OTHER ) PORT_NAME("5") PORT_CODE(KEYCODE_5_PAD)
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_OTHER ) PORT_NAME("6") PORT_CODE(KEYCODE_6_PAD)
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	// IN2 - DSW
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 1C_2C ) )
	PORT_DIPNAME( 0x04, 0x04, "Unknown 1-2" )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, "Unknown 1-3" )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, "Unknown 1-4" )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0xe0, 0x80, "Difficulty?" )
	PORT_DIPSETTING(    0xe0, "0" )
	PORT_DIPSETTING(    0xc0, "1" )
	PORT_DIPSETTING(    0xa0, "2" )
	PORT_DIPSETTING(    0x80, "3" )
	PORT_DIPSETTING(    0x60, "4" )
	PORT_DIPSETTING(    0x40, "5" )
	PORT_DIPSETTING(    0x20, "6" )
	PORT_DIPSETTING(    0x00, "7" )

	PORT_START	// IN3 - DSW
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x02, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, "Unknown 2-2*" )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x18, 0x18, "Unknown 2-3&4" )	// used ?
//  PORT_DIPSETTING(    0x10, "0" )
	PORT_DIPSETTING(    0x00, "0" )
	PORT_DIPSETTING(    0x18, "1" )
	PORT_DIPSETTING(    0x08, "2" )
	PORT_DIPNAME( 0x20, 0x20, "Unknown 2-5" )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x00, "Pirate Fight" )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, "Taito Copyright" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x80, DEF_STR( On ) )

	PORT_START	// IN4 - DSW
	PORT_DIPNAME( 0x01, 0x01, "Unknown 3-0*" )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, "Tickets" )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, "Unknown 3-2*" )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_SERVICE( 0x08, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0x10, 0x10, "Unknown 3-4" )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, "Unknown 3-5" )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, "Unknown 3-6" )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, "Unknown 3-7" )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
INPUT_PORTS_END


INPUT_PORTS_START( hanakanz )

	PORT_START	// IN0 - Coins + Service Keys
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_SERVICE3	)	// medal out
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN	)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_SERVICE  ) PORT_NAME(DEF_STR( Test )) PORT_CODE(KEYCODE_F1)	// Test
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_SERVICE1	)	// analyzer
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_SERVICE2	)	// data clear
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_COIN2	)	// note
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_COIN1	)
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN	)

	// keyb 2
	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_MAHJONG_A )	// A
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_MAHJONG_E )	// E
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )	// I
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_OTHER ) PORT_NAME(DEF_STR(Yes)) PORT_CODE(KEYCODE_Y)	// M
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )	// Kan
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_START2 )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_MAHJONG_B )	// B
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_MAHJONG_F )	// F
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )	// J
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_OTHER ) PORT_NAME(DEF_STR(No)) PORT_CODE(KEYCODE_N)		// N
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )	// Reach
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_MAHJONG_BET )	PORT_PLAYER(2)	// BET

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_MAHJONG_C )	// C
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_MAHJONG_G )	// G
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )	// K
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )	// Chi
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )	// Ron
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_MAHJONG_D )	// D
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_MAHJONG_H )	// H
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )	// L
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )	// PON
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN				)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_MAHJONG_SCORE		)	PORT_PLAYER(2)	// "t"
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_MAHJONG_DOUBLE_UP	)	PORT_PLAYER(2)	// "w"
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_MAHJONG_FLIP_FLOP	)	PORT_PLAYER(2)	// Flip Flop
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_MAHJONG_BIG			)	PORT_PLAYER(2)	// "b"
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_MAHJONG_SMALL		)	PORT_PLAYER(2)	// "s"

	// keyb 1
	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_OTHER ) PORT_NAME("1") PORT_CODE(KEYCODE_1_PAD)	// A
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_OTHER ) PORT_NAME("5") PORT_CODE(KEYCODE_5_PAD)	// E
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )	// I
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_OTHER ) PORT_NAME(DEF_STR(Yes)) PORT_CODE(KEYCODE_ENTER_PAD)	// M
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )	// Kan
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_START1 )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_OTHER ) PORT_NAME("2") PORT_CODE(KEYCODE_2_PAD)	// B
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_OTHER ) PORT_NAME("6") PORT_CODE(KEYCODE_6_PAD)	// F
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )	// J
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_OTHER ) PORT_NAME(DEF_STR(No)) PORT_CODE(KEYCODE_DEL_PAD)	// N
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )	// Reach
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_MAHJONG_BET )	// BET

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_OTHER ) PORT_NAME("3") PORT_CODE(KEYCODE_3_PAD)	// C
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_OTHER ) PORT_NAME("7") PORT_CODE(KEYCODE_7_PAD)	// G
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )	// K
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )	// Chi
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )	// Ron
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_OTHER ) PORT_NAME("4") PORT_CODE(KEYCODE_4_PAD)	// D
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_OTHER ) PORT_NAME("8") PORT_CODE(KEYCODE_8_PAD)	// H
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )	// L
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )	// PON
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN				)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_MAHJONG_SCORE		)	// "t"
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_MAHJONG_DOUBLE_UP	)	// "w"
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_MAHJONG_FLIP_FLOP	)	// Flip Flop
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_MAHJONG_BIG			)	// "b"
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_MAHJONG_SMALL		)	// "s"

	PORT_START	// IN11 - DSW1
	PORT_DIPNAME( 0x07, 0x07, "Unknown 1-0&1&2" )
	PORT_DIPSETTING(    0x07, "0" )
	PORT_DIPSETTING(    0x06, "1" )
	PORT_DIPSETTING(    0x05, "2" )
	PORT_DIPSETTING(    0x04, "3" )
	PORT_DIPSETTING(    0x03, "4" )
	PORT_DIPSETTING(    0x02, "5" )
	PORT_DIPSETTING(    0x01, "6" )
	PORT_DIPSETTING(    0x00, "7" )
	PORT_DIPNAME( 0x08, 0x08, "Unknown 1-3" )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, "Unknown 1-4" )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x60, 0x60, "Unknown 1-5&6" )
	PORT_DIPSETTING(    0x60, "0" )
	PORT_DIPSETTING(    0x40, "1" )
	PORT_DIPSETTING(    0x20, "2" )
	PORT_DIPSETTING(    0x00, "3" )
	PORT_DIPNAME( 0x80, 0x80, "Unknown 1-7" )
	PORT_DIPSETTING(    0x00, "5" )
	PORT_DIPSETTING(    0x80, "10" )

	PORT_START	// IN12 - DSW2
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 1C_2C ) )
	PORT_DIPNAME( 0x0c, 0x0c, "Unknown 2-2&3" )
	PORT_DIPSETTING(    0x0c, "10" )
	PORT_DIPSETTING(    0x08, "20" )
	PORT_DIPSETTING(    0x04, "30" )
	PORT_DIPSETTING(    0x00, "50" )
	PORT_DIPNAME( 0x30, 0x30, "Unknown 2-4&5" )
	PORT_DIPSETTING(    0x30, "100" )
	PORT_DIPSETTING(    0x20, "200" )
	PORT_DIPSETTING(    0x10, "250" )
	PORT_DIPSETTING(    0x00, "300" )
	PORT_DIPNAME( 0xc0, 0xc0, "Unknown 2-6&7" )
	PORT_DIPSETTING(    0xc0, "50" )
	PORT_DIPSETTING(    0x80, "60" )
	PORT_DIPSETTING(    0x40, "70" )
	PORT_DIPSETTING(    0x00, "80" )

	PORT_START	// IN13 - DSW3
	PORT_DIPNAME( 0x03, 0x03, "Game Type" )
	PORT_DIPSETTING(    0x03, "8 Cards" )
	PORT_DIPSETTING(    0x02, "6 Cards (Bets)" )
	PORT_DIPSETTING(    0x01, "6 Cards (Bets)?" )
	PORT_DIPSETTING(    0x00, "6 Cards (Bets)??" )
	PORT_DIPNAME( 0x04, 0x04, "(C) Nihon (Censored)" )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x18, 0x18, "Unknown 3-3&4" )
	PORT_DIPSETTING(    0x18, "10" )
	PORT_DIPSETTING(    0x10, "20" )
	PORT_DIPSETTING(    0x08, "30" )
	PORT_DIPSETTING(    0x00, "50" )
	PORT_DIPNAME( 0x60, 0x60, "Unknown 3-5&6" )
	PORT_DIPSETTING(    0x60, "0" )
	PORT_DIPSETTING(    0x40, "1" )
	PORT_DIPSETTING(    0x20, "2" )
	PORT_DIPSETTING(    0x00, "3" )
	PORT_DIPNAME( 0x80, 0x80, "Girl" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x80, DEF_STR( On ) )

	PORT_START	// IN14 - DSW4
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x01, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, "Unknown 4-1" )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, "Unknown 4-2" )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, "Unknown 4-3" )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, "Unknown 4-4" )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, "Unknown 4-5" )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, "Keyboard" )
	PORT_DIPSETTING(    0x40, "Hanafuda" )
	PORT_DIPSETTING(    0x00, "Mahjong" )
	PORT_DIPNAME( 0x80, 0x80, "Unknown 4-7" )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START	// IN15 - DSWs top bits
	PORT_DIPNAME( 0x01, 0x01, "Unknown 1-8" )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, "Unknown 1-9" )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x0c, 0x0c, "Unknown 2-8&9" )
	PORT_DIPSETTING(    0x0c, "20" )
	PORT_DIPSETTING(    0x08, "30" )
	PORT_DIPSETTING(    0x04, "40" )
	PORT_DIPSETTING(    0x00, "50" )
	PORT_DIPNAME( 0x10, 0x10, "Unknown 3-8" )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, "Unknown 3-9" )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, "Unknown 4-8" )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_SERVICE( 0x80, IP_ACTIVE_LOW )

	PORT_START	// IN16 - Fake DSW
	PORT_DIPNAME( 0x40, 0x40, "Allow Bets" )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START	// IN17 - Fake DSW
	PORT_DIPNAME( 0x40, 0x40, "? Hopper M." )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

INPUT_PORTS_END


INPUT_PORTS_START( hkagerou )

	PORT_START	// IN0 - Coins + Service Keys
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_SERVICE3	)	// medal out
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN	)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_SERVICE  ) PORT_NAME(DEF_STR( Test )) PORT_CODE(KEYCODE_F1)	// Test
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_SERVICE1	)	// analyzer
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_SERVICE2	)	// data clear
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_COIN2	)	// note
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_COIN1	)
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN	)

	// keyb 2
	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_MAHJONG_A )	// A
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_MAHJONG_E )	// E
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )	// I
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_OTHER ) PORT_NAME(DEF_STR(Yes)) PORT_CODE(KEYCODE_Y)	// M
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )	// Kan
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_START2 )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_MAHJONG_B )	// B
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_MAHJONG_F )	// F
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )	// J
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_OTHER ) PORT_NAME(DEF_STR(No)) PORT_CODE(KEYCODE_N)		// N
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )	// Reach
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_MAHJONG_BET )	PORT_PLAYER(2)	// BET

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_MAHJONG_C )	// C
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_MAHJONG_G )	// G
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )	// K
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )	// Chi
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )	// Ron
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_MAHJONG_D )	// D
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_MAHJONG_H )	// H
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )	// L
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )	// PON
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN				)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_MAHJONG_SCORE		)	PORT_PLAYER(2)	// "t"
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_MAHJONG_DOUBLE_UP	)	PORT_PLAYER(2)	// "w"
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_MAHJONG_FLIP_FLOP	)	PORT_PLAYER(2)	// Flip Flop
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_MAHJONG_BIG			)	PORT_PLAYER(2)	// "b"
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_MAHJONG_SMALL		)	PORT_PLAYER(2)	// "s"

	// keyb 1
	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_OTHER ) PORT_NAME("1") PORT_CODE(KEYCODE_1_PAD)	// A
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_OTHER ) PORT_NAME("5") PORT_CODE(KEYCODE_5_PAD)	// E
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )	// I
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_OTHER ) PORT_NAME(DEF_STR(Yes)) PORT_CODE(KEYCODE_ENTER_PAD)	// M
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_MAHJONG_SCORE		)	// "t"
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_START1 )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_OTHER ) PORT_NAME("2") PORT_CODE(KEYCODE_2_PAD)	// B
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_OTHER ) PORT_NAME("6") PORT_CODE(KEYCODE_6_PAD)	// F
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )	// J
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_OTHER ) PORT_NAME(DEF_STR(No)) PORT_CODE(KEYCODE_DEL_PAD)	// N
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_MAHJONG_SMALL	)	// "s"
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_MAHJONG_BET		)	// BET

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_OTHER ) PORT_NAME("3") PORT_CODE(KEYCODE_3_PAD)	// C
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_OTHER ) PORT_NAME("7") PORT_CODE(KEYCODE_7_PAD)	// G
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN		)	// K
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_MAHJONG_BIG	)	// "b"
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN		)	// Ron
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN		)

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_OTHER ) PORT_NAME("4") PORT_CODE(KEYCODE_4_PAD)	// D
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_OTHER ) PORT_NAME("8") PORT_CODE(KEYCODE_8_PAD)	// H
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN				)	// L
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_MAHJONG_DOUBLE_UP	)	// "w"
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN				)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_MAHJONG_SCORE		)	// "t"
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_MAHJONG_DOUBLE_UP	)	// "w"
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_MAHJONG_FLIP_FLOP	)	// Flip Flop
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_MAHJONG_BIG			)	// "b"
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_MAHJONG_SMALL		)	// "s"

	PORT_START	// IN11 - DSW1
	PORT_DIPNAME( 0x07, 0x07, "Unknown 1-0&1&2" )
	PORT_DIPSETTING(    0x07, "0" )
	PORT_DIPSETTING(    0x06, "1" )
	PORT_DIPSETTING(    0x05, "2" )
	PORT_DIPSETTING(    0x04, "3" )
	PORT_DIPSETTING(    0x03, "4" )
	PORT_DIPSETTING(    0x02, "5" )
	PORT_DIPSETTING(    0x01, "6" )
	PORT_DIPSETTING(    0x00, "7" )
	PORT_DIPNAME( 0x08, 0x08, "Unknown 1-3" )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, "Unknown 1-4" )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x60, 0x60, "Unknown 1-5&6" )
	PORT_DIPSETTING(    0x60, "0" )
	PORT_DIPSETTING(    0x40, "1" )
	PORT_DIPSETTING(    0x20, "2" )
	PORT_DIPSETTING(    0x00, "3" )
	PORT_DIPNAME( 0x80, 0x80, "Credits Per Note" )
	PORT_DIPSETTING(    0x00, "25" )
	PORT_DIPSETTING(    0x80, "50" )

	PORT_START	// IN12 - DSW2
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(    0x00, "1 Coin/10 Credits" )
	PORT_DIPNAME( 0x0c, 0x0c, "Unknown 2-2&3" )
	PORT_DIPSETTING(    0x0c, "10" )
	PORT_DIPSETTING(    0x08, "20" )
	PORT_DIPSETTING(    0x04, "30" )
	PORT_DIPSETTING(    0x00, "50" )
	PORT_DIPNAME( 0x30, 0x30, "Unknown 2-4&5" )
	PORT_DIPSETTING(    0x30, "100" )
	PORT_DIPSETTING(    0x20, "200" )
	PORT_DIPSETTING(    0x10, "250" )
	PORT_DIPSETTING(    0x00, "300" )
	PORT_DIPNAME( 0xc0, 0xc0, "Unknown 2-6&7" )
	PORT_DIPSETTING(    0xc0, "50" )
	PORT_DIPSETTING(    0x80, "60" )
	PORT_DIPSETTING(    0x40, "70" )
	PORT_DIPSETTING(    0x00, "80" )

	PORT_START	// IN13 - DSW3
	PORT_DIPNAME( 0x01, 0x01, "Game Type?" )
	PORT_DIPSETTING(    0x01, "0" )
	PORT_DIPSETTING(    0x00, "1" )
	PORT_DIPNAME( 0x02, 0x02, "Unknown 3-2" )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, "(C) Nihon (Censored)" )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x18, 0x18, "Unknown 3-3&4" )
	PORT_DIPSETTING(    0x18, "10" )
	PORT_DIPSETTING(    0x10, "20" )
	PORT_DIPSETTING(    0x08, "30" )
	PORT_DIPSETTING(    0x00, "50" )
	PORT_DIPNAME( 0x60, 0x60, "Unknown 3-5&6" )
	PORT_DIPSETTING(    0x60, "0" )
	PORT_DIPSETTING(    0x40, "1" )
	PORT_DIPSETTING(    0x20, "2" )
	PORT_DIPSETTING(    0x00, "3" )
	PORT_DIPNAME( 0x80, 0x80, "Girl?" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x80, DEF_STR( On ) )

	PORT_START	// IN14 - DSW4
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x01, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, "Unknown 4-1" )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, "Unknown 4-2" )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, "Unknown 4-3" )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, "Unknown 4-4" )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, "Unknown 4-5" )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, "Keyboard" )
	PORT_DIPSETTING(    0x40, "Hanafuda" )
	PORT_DIPSETTING(    0x00, "Mahjong" )
	PORT_DIPNAME( 0x80, 0x80, "Unknown 4-7" )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START	// IN15 - DSWs top bits
	PORT_DIPNAME( 0x01, 0x01, "Unknown 1-8" )
	PORT_DIPSETTING(    0x00, "5" )
	PORT_DIPSETTING(    0x01, "10" )
	PORT_DIPNAME( 0x02, 0x02, "Unknown 1-9" )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x0c, 0x0c, "Unknown 2-8&9" )
	PORT_DIPSETTING(    0x0c, "20" )
	PORT_DIPSETTING(    0x08, "30" )
	PORT_DIPSETTING(    0x04, "40" )
	PORT_DIPSETTING(    0x00, "50" )
	PORT_DIPNAME( 0x10, 0x10, "Unknown 3-8" )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, "Unknown 3-9" )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, "Unknown 4-8" )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_SERVICE( 0x80, IP_ACTIVE_LOW )

	PORT_START	// IN16 - Fake DSW
	PORT_DIPNAME( 0x40, 0x40, "Disable Bets" )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START	// IN17 - Fake DSW
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

INPUT_PORTS_END


INPUT_PORTS_START( mjreach1 )

	PORT_START	// IN0 - Coins + Service Keys
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_SERVICE3	)	// medal out
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN	)
	PORT_SERVICE(0x04, IP_ACTIVE_LOW)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_SERVICE1	)	// analyzer
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_SERVICE2	)	// data clear
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_COIN2	)	// note
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_COIN1	)
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN	)

	// keyb 2
	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_MAHJONG_A		)	PORT_PLAYER(2)	// A
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_MAHJONG_E		)	PORT_PLAYER(2)	// E
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_MAHJONG_I		)	PORT_PLAYER(2)	// I
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_MAHJONG_M		)	PORT_PLAYER(2)	// M
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_MAHJONG_KAN		)	PORT_PLAYER(2)	// Kan
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_START2			)	// Start 2

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_MAHJONG_B		)	PORT_PLAYER(2)	// B
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_MAHJONG_F		)	PORT_PLAYER(2)	// F
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_MAHJONG_J		)	PORT_PLAYER(2)	// J
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_MAHJONG_N		)	PORT_PLAYER(2)	// N
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_MAHJONG_REACH	)	PORT_PLAYER(2)	// Reach
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_MAHJONG_BET		)	PORT_PLAYER(2)	// BET

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_MAHJONG_C		)	PORT_PLAYER(2)	// C
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_MAHJONG_G		)	PORT_PLAYER(2)	// G
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_MAHJONG_K		)	PORT_PLAYER(2)	// K
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_MAHJONG_CHI		)	PORT_PLAYER(2)	// Chi
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_MAHJONG_RON		)	PORT_PLAYER(2)	// Ron
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN			)	// nothing

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_MAHJONG_D		)	PORT_PLAYER(2)	// D
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_MAHJONG_H		)	PORT_PLAYER(2)	// H
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_MAHJONG_L		)	PORT_PLAYER(2)	// L
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_MAHJONG_PON		)	PORT_PLAYER(2)	// Pon
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN			)	// nothing
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN			)	// nothing

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_MAHJONG_LAST_CHANCE	)	PORT_PLAYER(2)	// "l"
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_MAHJONG_SCORE		)	PORT_PLAYER(2)	// "t"
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_MAHJONG_DOUBLE_UP	)	PORT_PLAYER(2)	// "w"
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_MAHJONG_FLIP_FLOP	)	PORT_PLAYER(2)	// Flip Flop
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_MAHJONG_BIG			)	PORT_PLAYER(2)	// "b"
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_MAHJONG_SMALL		)	PORT_PLAYER(2)	// "s"

	// keyb 1
	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_MAHJONG_A		)	// A
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_MAHJONG_E		)	// E
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_MAHJONG_I		)	// I
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_MAHJONG_M		)	// M
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_MAHJONG_KAN		)	// Kan
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_START1			)	// Start 1

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_MAHJONG_B		)	// B
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_MAHJONG_F		)	// F
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_MAHJONG_J		)	// J
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_MAHJONG_N		)	// N
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_MAHJONG_REACH	)	// Reach
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_MAHJONG_BET		)	// BET

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_MAHJONG_C		)	// C
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_MAHJONG_G		)	// G
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_MAHJONG_K		)	// K
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_MAHJONG_CHI		)	// Chi
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_MAHJONG_RON		)	// Ron
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN			)	// nothing

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_MAHJONG_D		)	// D
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_MAHJONG_H		)	// H
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_MAHJONG_L		)	// L
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_MAHJONG_PON		)	// Pon
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN			)	// nothing
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN			)	// nothing

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_MAHJONG_LAST_CHANCE	)	// "l"
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_MAHJONG_SCORE		)	// "t"
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_MAHJONG_DOUBLE_UP	)	// "w"
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_MAHJONG_FLIP_FLOP	)	// Flip Flop
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_MAHJONG_BIG			)	// "b"
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_MAHJONG_SMALL		)	// "s"

	PORT_START	// IN11 - DSW1
	PORT_DIPNAME( 0x0f, 0x07, "Pay Out Rate (%)" )
	PORT_DIPSETTING(    0x00, "50" )
	PORT_DIPSETTING(    0x01, "53" )
	PORT_DIPSETTING(    0x02, "56" )
	PORT_DIPSETTING(    0x03, "59" )
	PORT_DIPSETTING(    0x04, "62" )
	PORT_DIPSETTING(    0x05, "65" )
	PORT_DIPSETTING(    0x06, "68" )
	PORT_DIPSETTING(    0x07, "71" )
	PORT_DIPSETTING(    0x08, "75" )
	PORT_DIPSETTING(    0x09, "78" )
	PORT_DIPSETTING(    0x0a, "81" )
	PORT_DIPSETTING(    0x0b, "84" )
	PORT_DIPSETTING(    0x0c, "87" )
	PORT_DIPSETTING(    0x0d, "90" )
	PORT_DIPSETTING(    0x0e, "93" )
	PORT_DIPSETTING(    0x0f, "96" )
	PORT_DIPNAME( 0x30, 0x30, "Odds Rate" )
	PORT_DIPSETTING(    0x30, "1 2 4 8 12 16 24 32" )
	PORT_DIPSETTING(    0x00, "1 2 3 5 8 15 30 50" )
	PORT_DIPSETTING(    0x20, "2 3 6 8 12 15 30 50" )
	PORT_DIPSETTING(    0x10, "1 2 3 5 10 25 50 100" )
	PORT_DIPNAME( 0xc0, 0xc0, "Max Bet" )
	PORT_DIPSETTING(    0xc0, "1" )
	PORT_DIPSETTING(    0x80, "5" )
	PORT_DIPSETTING(    0x40, "10" )
	PORT_DIPSETTING(    0x00, "20" )

	PORT_START	// IN12 - DSW2
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(    0x00, "1 Coin/10 Credits" )
	PORT_DIPNAME( 0x0c, 0x0c, "Min Credits To Start" )
	PORT_DIPSETTING(    0x0c, "1" )
	PORT_DIPSETTING(    0x08, "2" )
	PORT_DIPSETTING(    0x04, "3" )
	PORT_DIPSETTING(    0x00, "5" )
	PORT_DIPNAME( 0x30, 0x30, "Payout" )
	PORT_DIPSETTING(    0x30, "300" )
	PORT_DIPSETTING(    0x20, "500" )
	PORT_DIPSETTING(    0x10, "700" )
	PORT_DIPSETTING(    0x00, "1000" )
	PORT_DIPNAME( 0x40, 0x40, "W-BET" )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, "Last Chance" )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START	// IN13 - DSW3
	PORT_DIPNAME( 0x07, 0x07, "YAKUMAN Bonus" )
	PORT_DIPSETTING(    0x07, "Cut" )
	PORT_DIPSETTING(    0x06, "1 T" )
	PORT_DIPSETTING(    0x05, "300" )
	PORT_DIPSETTING(    0x04, "500" )
	PORT_DIPSETTING(    0x03, "700" )
	PORT_DIPSETTING(    0x02, "1000" )
	PORT_DIPSETTING(    0x01, "1500" )
	PORT_DIPSETTING(    0x00, "2000" )
	PORT_DIPNAME( 0x18, 0x18, "YAKUMAN Times" )
//  PORT_DIPSETTING(    0x00, "1" )
	PORT_DIPSETTING(    0x18, "1" )
	PORT_DIPSETTING(    0x10, "2" )
	PORT_DIPSETTING(    0x08, "3" )
	PORT_DIPNAME( 0x20, 0x20, "3 BAI In YAKUMAN Bonus Chance" )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, "Auto Tsumo" )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, "Credit Timing" )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START	// IN14 - DSW4
	PORT_DIPNAME( 0x01, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x00, "In Game Music" )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x00, "Girls (Demo)" )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, "Girls Show After 3 Renso" )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x00, "Girls (Play)" )	// Shown as always OFF in dips sheet
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, "Boys In Game" )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, "Boys" )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, "Don Den Key" )
	PORT_DIPSETTING(    0x80, "Start" )
	PORT_DIPSETTING(    0x00, "Flip/Flop" )

	PORT_START	// IN15 - DSWs top bits
	PORT_DIPNAME( 0x01, 0x01, "Credits Per Note" )
	PORT_DIPSETTING(    0x01, "5" )
	PORT_DIPSETTING(    0x00, "10" )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x0c, 0x0c, "Computer Strength" )
	PORT_DIPSETTING(    0x0c, "Weak" )
	PORT_DIPSETTING(    0x08, DEF_STR( Normal ))
	PORT_DIPSETTING(    0x04, "Strong" )
	PORT_DIPSETTING(    0x00, "Very Strong" )
	PORT_DIPNAME( 0x10, 0x10, "Game Style" )
	PORT_DIPSETTING(    0x10, "Credit" )
	PORT_DIPSETTING(    0x00, "Credit Time" )
	PORT_DIPNAME( 0x20, 0x20, "Start Method (Credit Time)" )
	PORT_DIPSETTING(    0x20, "?" )
	PORT_DIPSETTING(    0x00, "Rate" )
	PORT_DIPNAME( 0x40, 0x40, "Unknown 4-8" )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, "Unknown 4-9" )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START	// IN16 - Fake DSW
	PORT_DIPNAME( 0x40, 0x40, "Bets?" )
	PORT_DIPSETTING(    0x40, "0" )
	PORT_DIPSETTING(    0x00, "1" )

	PORT_START	// IN17 - Fake DSW
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

INPUT_PORTS_END


INPUT_PORTS_START( mjchuuka )

	PORT_START	// IN0 - Coins + Service Keys
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_SERVICE3	)	// medal out
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN	)
	PORT_SERVICE(0x04, IP_ACTIVE_LOW)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_SERVICE1	)	// analyzer
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_SERVICE2	)	// data clear
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_COIN2	)	// note
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_COIN1	)
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN	)

	// keyb 2
	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_MAHJONG_A		)	PORT_PLAYER(2)	// A
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_MAHJONG_E		)	PORT_PLAYER(2)	// E
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_MAHJONG_I		)	PORT_PLAYER(2)	// I
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_MAHJONG_M		)	PORT_PLAYER(2)	// M
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_MAHJONG_KAN		)	PORT_PLAYER(2)	// Kan
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_START2			)	// Start 2

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_MAHJONG_B		)	PORT_PLAYER(2)	// B
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_MAHJONG_F		)	PORT_PLAYER(2)	// F
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_MAHJONG_J		)	PORT_PLAYER(2)	// J
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_MAHJONG_N		)	PORT_PLAYER(2)	// N
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_MAHJONG_REACH	)	PORT_PLAYER(2)	// Reach
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_MAHJONG_BET		)	PORT_PLAYER(2)	// BET

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_MAHJONG_C		)	PORT_PLAYER(2)	// C
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_MAHJONG_G		)	PORT_PLAYER(2)	// G
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_MAHJONG_K		)	PORT_PLAYER(2)	// K
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_MAHJONG_CHI		)	PORT_PLAYER(2)	// Chi
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_MAHJONG_RON		)	PORT_PLAYER(2)	// Ron
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN			)	// nothing

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_MAHJONG_D		)	PORT_PLAYER(2)	// D
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_MAHJONG_H		)	PORT_PLAYER(2)	// H
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_MAHJONG_L		)	PORT_PLAYER(2)	// L
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_MAHJONG_PON		)	PORT_PLAYER(2)	// Pon
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN			)	// nothing
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN			)	// nothing

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_MAHJONG_LAST_CHANCE	)	PORT_PLAYER(2)	// "l"
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_MAHJONG_SCORE		)	PORT_PLAYER(2)	// "t"
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_MAHJONG_DOUBLE_UP	)	PORT_PLAYER(2)	// "w"
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_MAHJONG_FLIP_FLOP	)	PORT_PLAYER(2)	// Flip Flop
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_MAHJONG_BIG			)	PORT_PLAYER(2)	// "b"
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_MAHJONG_SMALL		)	PORT_PLAYER(2)	// "s"

	// keyb 1
	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_MAHJONG_A		)	// A
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_MAHJONG_E		)	// E
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_MAHJONG_I		)	// I
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_MAHJONG_M		)	// M
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_MAHJONG_KAN		)	// Kan
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_START1			)	// Start 1

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_MAHJONG_B		)	// B
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_MAHJONG_F		)	// F
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_MAHJONG_J		)	// J
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_MAHJONG_N		)	// N
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_MAHJONG_REACH	)	// Reach
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_MAHJONG_BET		)	// BET

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_MAHJONG_C		)	// C
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_MAHJONG_G		)	// G
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_MAHJONG_K		)	// K
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_MAHJONG_CHI		)	// Chi
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_MAHJONG_RON		)	// Ron
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN			)	// nothing

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_MAHJONG_D		)	// D
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_MAHJONG_H		)	// H
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_MAHJONG_L		)	// L
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_MAHJONG_PON		)	// Pon
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN			)	// nothing
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN			)	// nothing

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_MAHJONG_LAST_CHANCE	)	// "l"
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_MAHJONG_SCORE		)	// "t"
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_MAHJONG_DOUBLE_UP	)	// "w"
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_MAHJONG_FLIP_FLOP	)	// Flip Flop
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_MAHJONG_BIG			)	// "b"
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_MAHJONG_SMALL		)	// "s"

	PORT_START	// IN11 - DSW1
	PORT_DIPNAME( 0x0f, 0x07, "Pay Out Rate (%)" )
	PORT_DIPSETTING(    0x00, "50" )
	PORT_DIPSETTING(    0x01, "53" )
	PORT_DIPSETTING(    0x02, "56" )
	PORT_DIPSETTING(    0x03, "59" )
	PORT_DIPSETTING(    0x04, "62" )
	PORT_DIPSETTING(    0x05, "65" )
	PORT_DIPSETTING(    0x06, "68" )
	PORT_DIPSETTING(    0x07, "71" )
	PORT_DIPSETTING(    0x08, "75" )
	PORT_DIPSETTING(    0x09, "78" )
	PORT_DIPSETTING(    0x0a, "81" )
	PORT_DIPSETTING(    0x0b, "84" )
	PORT_DIPSETTING(    0x0c, "87" )
	PORT_DIPSETTING(    0x0d, "90" )
	PORT_DIPSETTING(    0x0e, "93" )
	PORT_DIPSETTING(    0x0f, "96" )
	PORT_DIPNAME( 0x30, 0x30, "Odds Rate" )
	PORT_DIPSETTING(    0x20, "1 2 3 4 6 8 10 15" )
	PORT_DIPSETTING(    0x30, "1 2 4 8 12 16 24 32" )
	PORT_DIPSETTING(    0x00, "1 2 3 5 8 15 30 50" )
	PORT_DIPSETTING(    0x10, "1 2 3 5 10 25 50 100" )
	PORT_DIPNAME( 0xc0, 0xc0, "Max Bet" )
	PORT_DIPSETTING(    0xc0, "1" )
	PORT_DIPSETTING(    0x80, "5" )
	PORT_DIPSETTING(    0x40, "10" )
	PORT_DIPSETTING(    0x00, "20" )

	PORT_START	// IN12 - DSW2
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(    0x00, "1 Coin/10 Credits" )
	PORT_DIPNAME( 0x0c, 0x0c, "Min Credits To Start" )
	PORT_DIPSETTING(    0x0c, "1" )
//  PORT_DIPSETTING(    0x08, "2" ) // ? these don't let you start a game
//  PORT_DIPSETTING(    0x04, "3" )
//  PORT_DIPSETTING(    0x00, "5" )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x10, "10" )
	PORT_DIPSETTING(    0x00, "255" )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START	// IN13 - DSW3
	PORT_DIPNAME( 0x07, 0x07, "YAKUMAN Bonus" )
	PORT_DIPSETTING(    0x07, "Cut" )
	PORT_DIPSETTING(    0x06, "1 T" )
	PORT_DIPSETTING(    0x05, "300" )
	PORT_DIPSETTING(    0x04, "500" )
	PORT_DIPSETTING(    0x03, "700" )
	PORT_DIPSETTING(    0x02, "1000" )
	PORT_DIPSETTING(    0x01, "1000?" )
	PORT_DIPSETTING(    0x00, "1000?" )
	PORT_DIPNAME( 0x18, 0x18, "YAKUMAN Times" )
//  PORT_DIPSETTING(    0x00, "1" )
	PORT_DIPSETTING(    0x18, "1" )
	PORT_DIPSETTING(    0x10, "2" )
	PORT_DIPSETTING(    0x08, "3?" )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x40, "149" )
	PORT_DIPSETTING(    0x00, "212" )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START	// IN14 - DSW4
	PORT_DIPNAME( 0x01, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x00, "In Game Music" )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x0c, 0x00, "Girls" )
	PORT_DIPSETTING(    0x0c, DEF_STR( No ) )
	PORT_DIPSETTING(    0x08, "Dressed" )
	PORT_DIPSETTING(    0x04, "Underwear" )
	PORT_DIPSETTING(    0x00, "Nude" )
	PORT_DIPNAME( 0x10, 0x00, "Girls Speech" )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START	// IN15 - DSWs top bits
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, "Credits Per Note" )
	PORT_DIPSETTING(    0x02, "5" )
	PORT_DIPSETTING(    0x00, "10" )
	PORT_DIPNAME( 0x0c, 0x0c, "Computer Strength?" )
	PORT_DIPSETTING(    0x0c, "Weak" )
	PORT_DIPSETTING(    0x08, DEF_STR( Normal ))
	PORT_DIPSETTING(    0x04, "Strong" )
	PORT_DIPSETTING(    0x00, "Very Strong" )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, "Unknown 4-8" )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, "Unknown 4-9" )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START	// IN16 - Fake DSW
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START	// IN17 - Fake DSW
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

INPUT_PORTS_END


INPUT_PORTS_START( funkyfig )
	PORT_START	// IN0 - Keys (port 83 with port 80 = 20)
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_OTHER ) PORT_NAME("1") PORT_CODE(KEYCODE_1_PAD)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_OTHER ) PORT_NAME("2") PORT_CODE(KEYCODE_2_PAD)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_OTHER ) PORT_NAME("3") PORT_CODE(KEYCODE_3_PAD)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_OTHER ) PORT_NAME("4") PORT_CODE(KEYCODE_4_PAD)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_OTHER ) PORT_NAME("5") PORT_CODE(KEYCODE_5_PAD)
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_OTHER ) PORT_NAME("6") PORT_CODE(KEYCODE_6_PAD)
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_OTHER ) PORT_NAME("7") PORT_CODE(KEYCODE_7_PAD)

	PORT_START	// IN1 - ? (port 83 with port 80 = 21)
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_START2 )	// ?
	PORT_BIT( 0xfe, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	// IN2 - Coins (port 82 with port 80 = 22)
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 ) PORT_IMPULSE(2)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_SERVICE ) PORT_NAME(DEF_STR( Test )) PORT_CODE(KEYCODE_F1)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_OTHER ) PORT_NAME("8") PORT_CODE(KEYCODE_8_PAD)
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_OTHER ) PORT_NAME("9") PORT_CODE(KEYCODE_9_PAD)
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	// IN3 - DSW1 (low bits, port 1c with rombank = 1e)
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 1C_2C ) )
	PORT_DIPNAME( 0x1c, 0x10, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x18, "0" )
	PORT_DIPSETTING(    0x14, "1" )
	PORT_DIPSETTING(    0x1c, "2" )
	PORT_DIPSETTING(    0x10, "3" )
	PORT_DIPSETTING(    0x0c, "4" )
	PORT_DIPSETTING(    0x08, "5" )
//  PORT_DIPSETTING(    0x04, "5" )
//  PORT_DIPSETTING(    0x00, "5" )
	PORT_DIPNAME( 0x20, 0x20, "Linked Cabinets" )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x40, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, "Play Rock Smash" )
	PORT_DIPSETTING(    0x00, DEF_STR( No ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Yes ) )

	PORT_START	// IN4 - DSW2 (low bits, port 1c with rombank = 1d)
	PORT_DIPNAME( 0x01, 0x01, "2 Player Game" )
	PORT_DIPSETTING(    0x00, DEF_STR( No ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x0c, 0x0c, "Unknown 2-2&3*" )
	PORT_DIPSETTING(    0x0c, "10" )
	PORT_DIPSETTING(    0x08, "20" )
	PORT_DIPSETTING(    0x04, "40" )
	PORT_DIPSETTING(    0x00, "80" )
	PORT_DIPNAME( 0x10, 0x10, "Unknown 2-4*" )	// used
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, "Unknown 2-5*" )	// used
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0xc0, 0xc0, "Unknown 2-6&7*" )	// used
	PORT_DIPSETTING(    0xc0, "0" )
	PORT_DIPSETTING(    0x80, "1" )
	PORT_DIPSETTING(    0x40, "2" )
	PORT_DIPSETTING(    0x00, "3" )

	PORT_START	// IN5 - DSW1 & 2 (high bits, port 1c with rombank = 1b)
	PORT_DIPNAME( 0x01, 0x01, "Continue?" )	// related to continue
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_SERVICE( 0x02, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0x04, 0x04, "Debug Text" )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x00, "Country" )
	PORT_DIPSETTING(    0x08, DEF_STR( Japan ) )
	PORT_DIPSETTING(    0x00, DEF_STR( USA ) )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
INPUT_PORTS_END



INPUT_PORTS_START( mjmyster )
	PORT_START	// IN0 - Coins + Service Keys
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_SERVICE3	)	// medal out
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN	)
	PORT_SERVICE_NO_TOGGLE(0x04, IP_ACTIVE_LOW)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_SERVICE1	)	// analyzer
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_SERVICE2	)	// data clear
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_COIN2	)	// note
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_COIN1	)
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN	)

	// keyb 1
	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_MAHJONG_A		)	// A
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_MAHJONG_E		)	// E
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_MAHJONG_I		)	// I
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_MAHJONG_M		)	// M
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_MAHJONG_KAN		)	// Kan
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_START1			)	// Start 1

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_MAHJONG_B		)	// B
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_MAHJONG_F		)	// F
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_MAHJONG_J		)	// J
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_MAHJONG_N		)	// N
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_MAHJONG_REACH	)	// Reach
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_MAHJONG_BET		)	// BET

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_MAHJONG_C		)	// C
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_MAHJONG_G		)	// G
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_MAHJONG_K		)	// K
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_MAHJONG_CHI		)	// Chi
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_MAHJONG_RON		)	// Ron
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN			)	// nothing

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_MAHJONG_D		)	// D
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_MAHJONG_H		)	// H
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_MAHJONG_L		)	// L
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_MAHJONG_PON		)	// Pon
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN			)	// nothing
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN			)	// nothing

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_MAHJONG_LAST_CHANCE	)	// "l"
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_MAHJONG_SCORE		)	// "t"
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_MAHJONG_DOUBLE_UP	)	// "w"
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_MAHJONG_FLIP_FLOP	)	// Flip Flop
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_MAHJONG_BIG			)	// "b"
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_MAHJONG_SMALL		)	// "s"

	PORT_START	// IN6 - DSW1
	PORT_DIPNAME( 0x0f, 0x07, "Pay Out Rate (%)" )
	PORT_DIPSETTING(    0x00, "50" )
	PORT_DIPSETTING(    0x01, "53" )
	PORT_DIPSETTING(    0x02, "56" )
	PORT_DIPSETTING(    0x03, "59" )
	PORT_DIPSETTING(    0x04, "62" )
	PORT_DIPSETTING(    0x05, "65" )
	PORT_DIPSETTING(    0x06, "68" )
	PORT_DIPSETTING(    0x07, "71" )
	PORT_DIPSETTING(    0x08, "75" )
	PORT_DIPSETTING(    0x09, "78" )
	PORT_DIPSETTING(    0x0a, "81" )
	PORT_DIPSETTING(    0x0b, "84" )
	PORT_DIPSETTING(    0x0c, "87" )
	PORT_DIPSETTING(    0x0d, "90" )
	PORT_DIPSETTING(    0x0e, "93" )
	PORT_DIPSETTING(    0x0f, "96" )
	PORT_DIPNAME( 0x30, 0x30, "Odds Rate" )
	PORT_DIPSETTING(    0x20, "2 3 6 8 12 15 30 50" )
	PORT_DIPSETTING(    0x30, "1 2 4 8 12 16 24 32" )
	PORT_DIPSETTING(    0x00, "1 2 3 5 8 15 30 50" )
	PORT_DIPSETTING(    0x10, "1 2 3 5 10 25 50 100" )
	PORT_DIPNAME( 0xc0, 0xc0, "Max Bet" )
	PORT_DIPSETTING(    0xc0, "1" )
	PORT_DIPSETTING(    0x80, "5" )
	PORT_DIPSETTING(    0x40, "10" )
	PORT_DIPSETTING(    0x00, "20" )

	PORT_START	// IN7 - DSW2
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(    0x00, "1 Coin/10 Credits" )
	PORT_DIPNAME( 0x0c, 0x0c, "Min Credits To Start" )
	PORT_DIPSETTING(    0x0c, "1" )
//  PORT_DIPSETTING(    0x08, "2" ) // ? these don't let you start a game
//  PORT_DIPSETTING(    0x04, "3" )
//  PORT_DIPSETTING(    0x00, "5" )
	PORT_DIPNAME( 0x70, 0x70, "YAKUMAN Bonus" )
	PORT_DIPSETTING(    0x70, "Cut" )
	PORT_DIPSETTING(    0x60, "1 T" )
	PORT_DIPSETTING(    0x50, "300" )
	PORT_DIPSETTING(    0x40, "500" )
	PORT_DIPSETTING(    0x30, "700" )
	PORT_DIPSETTING(    0x20, "1000" )
	PORT_DIPSETTING(    0x10, "1000?" )
	PORT_DIPSETTING(    0x00, "1000?" )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START	// IN8 - DSW3
	PORT_DIPNAME( 0x03, 0x03, "YAKUMAN Times" )
//  PORT_DIPSETTING(    0x00, "1" )
	PORT_DIPSETTING(    0x03, "1" )
	PORT_DIPSETTING(    0x02, "2" )
	PORT_DIPSETTING(    0x01, "3?" )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x18, 0x18, "Payout" )
	PORT_DIPSETTING(    0x18, "300" )
	PORT_DIPSETTING(    0x10, "500" )
	PORT_DIPSETTING(    0x08, "700" )
	PORT_DIPSETTING(    0x00, "1000" )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START	// IN9 - DSW4
	PORT_DIPNAME( 0x01, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x00, "In Game Music" )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Controls ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Region ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Japan ) )
	PORT_DIPSETTING(    0x00, "Hong Kong" )

	PORT_START	// IN10 - DSWs top bits
	PORT_DIPNAME( 0x03, 0x03, "Computer Strength?" )
	PORT_DIPSETTING(    0x03, "Weak" )
	PORT_DIPSETTING(    0x02, DEF_STR( Normal ))
	PORT_DIPSETTING(    0x01, "Strong" )
	PORT_DIPSETTING(    0x00, "Very Strong" )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x08, "149" )
	PORT_DIPSETTING(    0x00, "212" )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, "Credits Per Note" )
	PORT_DIPSETTING(    0x40, "5" )
	PORT_DIPSETTING(    0x00, "10" )
	PORT_DIPNAME( 0x80, 0x00, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x80, DEF_STR( On ) )
INPUT_PORTS_END


/***************************************************************************
                            Don Den Lover Vol.1
***************************************************************************/

static MACHINE_DRIVER_START( ddenlovr )

	/* basic machine hardware */
	MDRV_CPU_ADD_TAG("main",M68000,24000000 / 2)
	MDRV_CPU_PROGRAM_MAP(ddenlovr_readmem,ddenlovr_writemem)
	MDRV_CPU_VBLANK_INT(irq1_line_hold,1)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(336, 256)
	MDRV_VISIBLE_AREA(0, 336-1, 5, 256-16+5-1)
	MDRV_PALETTE_LENGTH(0x100)

	MDRV_VIDEO_START(ddenlovr)
	MDRV_VIDEO_EOF(ddenlovr)
	MDRV_VIDEO_UPDATE(ddenlovr)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")

	MDRV_SOUND_ADD(YM2413, 3579545)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.80)

	MDRV_SOUND_ADD_TAG("ay8910", AY8910, 2000000)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.30)

	MDRV_SOUND_ADD(OKIM6295, 8000)
	MDRV_SOUND_CONFIG(okim6295_interface_region_1)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.80)
MACHINE_DRIVER_END

static struct AY8910interface quiz365_ay8910_interface =
{
	quiz365_input_r,
	0,
	0,
	dynax_select_w
};

static MACHINE_DRIVER_START( quiz365 )

	/* basic machine hardware */
	MDRV_IMPORT_FROM(ddenlovr)
	MDRV_CPU_MODIFY("main")
	MDRV_CPU_PROGRAM_MAP(quiz365_readmem,quiz365_writemem)

	MDRV_SOUND_MODIFY("ay8910")
	MDRV_SOUND_CONFIG(quiz365_ay8910_interface)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( nettoqc )

	/* basic machine hardware */
	MDRV_IMPORT_FROM(ddenlovr)
	MDRV_CPU_MODIFY("main")
	MDRV_CPU_PROGRAM_MAP(nettoqc_readmem,nettoqc_writemem)
MACHINE_DRIVER_END

/***************************************************************************
                                Rong Rong
***************************************************************************/

/* the CPU is in Interrupt Mode 2
   vector can be 0xee, 0xf8 0xfa 0xfc
   rongrong: 0xf8 and 0xfa do nothing
   quizchq: 0xf8 and 0xfa are very similar, they should be triggered by the blitter
   0xee is vblank
   0xfc is from the 6242RTC
 */
static INTERRUPT_GEN( quizchq_irq )
{
	static int count;

	/* I haven't found a irq ack register, so I need this kludge to
       make sure I don't lose any interrupt generated by the blitter,
       otherwise quizchq would lock up. */
	if (cpunum_get_info_int(0,CPUINFO_INT_INPUT_STATE + 0))
		return;

	if ((++count % 60) == 0)
		cpunum_set_input_line_and_vector(0, 0, HOLD_LINE, 0xfc);
	else
		cpunum_set_input_line_and_vector(0, 0, HOLD_LINE, 0xee);
}

/*
static INTERRUPT_GEN( rtc_irq )
{
    cpunum_set_input_line_and_vector(0, 0, HOLD_LINE, 0xfc);
}
*/

static MACHINE_DRIVER_START( quizchq )

	/* basic machine hardware */
	MDRV_CPU_ADD_TAG("main", Z80, 8000000)	/* ? */
	MDRV_CPU_PROGRAM_MAP(quizchq_readmem,quizchq_writemem)
	MDRV_CPU_IO_MAP(quizchq_readport,quizchq_writeport)
	MDRV_CPU_VBLANK_INT(quizchq_irq,1)
//  MDRV_CPU_PERIODIC_INT(rtc_irq,1)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(336, 256)
	MDRV_VISIBLE_AREA(0, 336-1, 5, 256-16+5-1)
	MDRV_PALETTE_LENGTH(0x100)
	MDRV_COLORTABLE_LENGTH(0x100)

	MDRV_VIDEO_START(ddenlovr)
	MDRV_VIDEO_EOF(ddenlovr)
	MDRV_VIDEO_UPDATE(ddenlovr)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")

	MDRV_SOUND_ADD(YM2413, 3579545)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.80)

	MDRV_SOUND_ADD(OKIM6295, 8000)
	MDRV_SOUND_CONFIG(okim6295_interface_region_1)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.80)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( rongrong )

	/* basic machine hardware */
	MDRV_IMPORT_FROM(quizchq)
	MDRV_CPU_MODIFY("main")
	MDRV_CPU_PROGRAM_MAP(rongrong_readmem,rongrong_writemem)
	MDRV_CPU_IO_MAP(rongrong_readport,rongrong_writeport)
MACHINE_DRIVER_END

/***************************************************************************

    Monkey Mole Panic

***************************************************************************/

/*  the CPU is in Interrupt Mode 0:

    RST 08 is vblank
    RST 18 is from the blitter
    RST 20 is from the 6242RTC
 */
static INTERRUPT_GEN( mmpanic_irq )
{
	static int count;

	/* I haven't found a irq ack register, so I need this kludge to
       make sure I don't lose any interrupt generated by the blitter,
       otherwise the game would lock up. */
	if (cpunum_get_info_int(0,CPUINFO_INT_INPUT_STATE + 0))
		return;

	if ((++count % 60) == 0)
		cpunum_set_input_line_and_vector(0, 0, HOLD_LINE, 0xe7);	// RST 20, clock
	else
		cpunum_set_input_line_and_vector(0, 0, HOLD_LINE, 0xcf);	// RST 08, vblank
}

static MACHINE_DRIVER_START( mmpanic )

	/* basic machine hardware */
	MDRV_CPU_ADD_TAG("main", Z80, 8000000)	/* ? */
	MDRV_CPU_PROGRAM_MAP(mmpanic_readmem,mmpanic_writemem)
	MDRV_CPU_IO_MAP(mmpanic_readport,mmpanic_writeport)
	MDRV_CPU_VBLANK_INT(mmpanic_irq,1)

	MDRV_CPU_ADD_TAG("sound", Z80, 8000000)	/* ? */
	MDRV_CPU_PROGRAM_MAP(mmpanic_sound_readmem,mmpanic_sound_writemem)
	MDRV_CPU_IO_MAP(mmpanic_sound_readport,mmpanic_sound_writeport)
	MDRV_CPU_VBLANK_INT(irq0_line_hold,1)	// NMI by main cpu

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(336, 256)
	MDRV_VISIBLE_AREA(0, 336-1, 5, 256-16+5-1)
	MDRV_PALETTE_LENGTH(0x100)
	MDRV_COLORTABLE_LENGTH(0x100)

	MDRV_VIDEO_START(mmpanic)	// extra layers
	MDRV_VIDEO_EOF(ddenlovr)
	MDRV_VIDEO_UPDATE(ddenlovr)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")

	MDRV_SOUND_ADD(YM2413, 3579545)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.80)

	MDRV_SOUND_ADD(AY8910, 2000000)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.30)

	MDRV_SOUND_ADD(OKIM6295, 8000)
	MDRV_SOUND_CONFIG(okim6295_interface_region_1)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.80)
MACHINE_DRIVER_END


/***************************************************************************

    Hana Kanzashi

***************************************************************************/

/*  the CPU is in Interrupt Mode 2
    vector can be 0xe0, 0xe2
    0xe0 is vblank
    0xe2 is from the 6242RTC
 */
static INTERRUPT_GEN( hanakanz_irq )
{
	static int count;

	/* I haven't found a irq ack register, so I need this kludge to
       make sure I don't lose any interrupt generated by the blitter,
       otherwise quizchq would lock up. */
	if (cpunum_get_info_int(0,CPUINFO_INT_INPUT_STATE + 0))
		return;

	if ((++count % 60) == 0)
		cpunum_set_input_line_and_vector(0, 0, HOLD_LINE, 0xe2);
	else
		cpunum_set_input_line_and_vector(0, 0, HOLD_LINE, 0xe0);
}

static MACHINE_DRIVER_START( hanakanz )

	/* basic machine hardware */
	MDRV_CPU_ADD_TAG("main",Z80,8000000)	// TMPZ84C015BF-8
	MDRV_CPU_PROGRAM_MAP(hanakanz_readmem,hanakanz_writemem)
	MDRV_CPU_IO_MAP(hanakanz_readport,hanakanz_writeport)
	MDRV_CPU_VBLANK_INT(hanakanz_irq,1)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(336, 256)
	MDRV_VISIBLE_AREA(0, 336-1, 5, 256-11-1)
	MDRV_PALETTE_LENGTH(0x200)

	MDRV_VIDEO_START(ddenlovr)
	MDRV_VIDEO_EOF(ddenlovr)
	MDRV_VIDEO_UPDATE(ddenlovr)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")

	MDRV_SOUND_ADD(YM2413, 3579545)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.80)

	MDRV_SOUND_ADD(OKIM6295, 8000)
	MDRV_SOUND_CONFIG(okim6295_interface_region_1)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.80)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( hkagerou )
	/* basic machine hardware */
	MDRV_IMPORT_FROM( hanakanz )
	MDRV_CPU_MODIFY("main")
	MDRV_CPU_IO_MAP(hkagerou_readport,hkagerou_writeport)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( mjreach1 )
	/* basic machine hardware */
	MDRV_IMPORT_FROM( hanakanz )
	MDRV_CPU_MODIFY("main")
	MDRV_CPU_IO_MAP(mjreach1_readport,mjreach1_writeport)
MACHINE_DRIVER_END


/***************************************************************************
     Mahjong Chuukanejyo
***************************************************************************/

/*  the CPU is in Interrupt Mode 2
    vector can be 0xf8, 0xfa
    0xf8 is vblank
    0xfa is from the 6242RTC
 */
static INTERRUPT_GEN( mjchuuka_irq )
{
	static int count;

	/* I haven't found a irq ack register, so I need this kludge to
       make sure I don't lose any interrupt generated by the blitter,
       otherwise quizchq would lock up. */
	if (cpunum_get_info_int(0,CPUINFO_INT_INPUT_STATE + 0))
		return;

	if ((++count % 60) == 0)
		cpunum_set_input_line_and_vector(0, 0, HOLD_LINE, 0xfa);
	else
		cpunum_set_input_line_and_vector(0, 0, HOLD_LINE, 0xf8);
}


static MACHINE_DRIVER_START( mjchuuka )
	/* basic machine hardware */
	MDRV_IMPORT_FROM( hanakanz )
	MDRV_CPU_MODIFY("main")
	MDRV_CPU_IO_MAP(mjchuuka_readport,mjchuuka_writeport)
	MDRV_CPU_VBLANK_INT(mjchuuka_irq,1)

	MDRV_SOUND_ADD(AY8910, 2000000)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.0)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( funkyfig )

	MDRV_IMPORT_FROM(mmpanic)
	MDRV_CPU_MODIFY("main")
	MDRV_CPU_PROGRAM_MAP(funkyfig_readmem,funkyfig_writemem)
	MDRV_CPU_IO_MAP(funkyfig_readport,funkyfig_writeport)
	MDRV_CPU_VBLANK_INT(mjchuuka_irq,1)

	MDRV_CPU_MODIFY("sound")
	MDRV_CPU_IO_MAP(funkyfig_sound_readport,mmpanic_sound_writeport)

	MDRV_VIDEO_START(ddenlovr)	// no extra layers?
MACHINE_DRIVER_END


/***************************************************************************
                        Mahjong The Mysterious World
***************************************************************************/

/*  the CPU is in Interrupt Mode 2
    vector can be 0xf8, 0xfa, 0xfc
    0xf8 is vblank
    0xfa and/or xfc are from the blitter (almost identical)
    NMI triggered by the RTC

    To do:

    The game randomly locks up (like quizchq?) because of some lost
    blitter interrupt I guess (nested blitter irqs?). Hence the hack
    to trigger the blitter irq every frame.
 */
static INTERRUPT_GEN( mjmyster_irq )
{
	/* I haven't found a irq ack register, so I need this kludge to
       make sure I don't lose any interrupt generated by the blitter,
       otherwise quizchq would lock up. */
	if (cpunum_get_info_int(0,CPUINFO_INT_INPUT_STATE + 0))
		return;

	switch( cpu_getiloops() )
	{
		case 0:	cpunum_set_input_line_and_vector(0, 0, HOLD_LINE, 0xf8);	break;
		case 1:	cpunum_set_input_line_and_vector(0, 0, HOLD_LINE, 0xfa);	break;
	}
}

static INTERRUPT_GEN( rtc_nmi_irq )
{
	cpunum_set_input_line(0, INPUT_LINE_NMI, PULSE_LINE);
}

static struct AY8910interface mjmyster_ay8910_interface =
{
	0,
	0,
	0,
	dynax_select_w
};

static MACHINE_DRIVER_START( mjmyster )
	/* basic machine hardware */
	MDRV_IMPORT_FROM( quizchq )
	MDRV_CPU_MODIFY("main")
	MDRV_CPU_PROGRAM_MAP(mjmyster_readmem,mjmyster_writemem)
	MDRV_CPU_IO_MAP(mjmyster_readport,mjmyster_writeport)
	MDRV_CPU_VBLANK_INT(mjmyster_irq, 2)
	MDRV_CPU_PERIODIC_INT(rtc_nmi_irq, TIME_IN_HZ(1) )

	MDRV_SOUND_ADD(AY8910, 2000000)
	MDRV_SOUND_CONFIG(mjmyster_ay8910_interface)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.0)
MACHINE_DRIVER_END



/***************************************************************************

Monkey Mole Panic
Nakanihon/Taito 1992
                      7001A  5563    6242
                      6295   7002
                             Z80
     8910                   5563   16MHz
     DynaX NL-001           7003              14.318MHz
                            Z80               24 MHz
          2018
                  DynaX   524256  524256       DynaX
                  1108    524256  524256       1427
                  DynaX   524256  524256       DynaX
                  1108    524256  524256       1427

     8251                      7006    7005   7004


The game asks players to slap buttons on a control panel and see mole-like creatures
get crunched on the eye-level video screen.

An on-screen test mode means the ticket dispenser can be adjusted from 1-99 tickets
and 15 score possibilities.

It also checks PCB EPROMs, switches and lamps, and the built-in income analyzer.

There are six levels of difficulty for one or two players.

The games are linkable (up to four) for competitive play.

***************************************************************************/

ROM_START( mmpanic )
	ROM_REGION( 0x50000, REGION_CPU1, 0 )	/* Z80 Code */
	ROM_LOAD( "nwc7002a",     0x00000, 0x40000, CRC(725b337f) SHA1(4d1f1ebc4de524d959dde60498d3f7038c7f3ed2) )
	ROM_RELOAD(               0x10000, 0x40000 )

	ROM_REGION( 0x20000, REGION_CPU2, 0 )	/* Z80 Code */
	ROM_LOAD( "nwc7003",      0x00000, 0x20000, CRC(4f02ce44) SHA1(9a3abd9c555d5863a2110d84d1a3f582ba9d56b9) )	// 1xxxxxxxxxxxxxxxx = 0xFF

	ROM_REGION( 0x280000, REGION_GFX1, 0 )	/* blitter data */
	ROM_LOAD( "nwc7004",      0x000000, 0x100000, CRC(5b4ad8c5) SHA1(a92a0bef01c71e745597ec96e7b8aa0ec26dc59d) )
	ROM_LOAD( "nwc7005",      0x100000, 0x100000, CRC(9ec41956) SHA1(5a92d725cee7052e1c3cd671b58795125c6a4ea9) )
	ROM_LOAD( "nwc7006a",     0x200000, 0x080000, CRC(9099c571) SHA1(9762612f41384602d545d2ec6dabd5f077d5fe21) )

	ROM_REGION( 0x40000, REGION_SOUND1, 0 )	/* Samples */
	ROM_LOAD( "nwc7001a",     0x00000, 0x40000, CRC(1ae3660d) SHA1(c4711f00a30f7d2c80fe241d7e0a464f0bb2555f) )
ROM_END


/***************************************************************************

Z80 x2
OSC 24MHz, 14.31818MHz, 16MHz
Oki M6295 + YM2413 + AY-3-8910
Oki 6242 + 3.6v battery + 32.768kHz (rtc)
Toshiba TMP82C51 (USART)
Custom ICs NL-004 (x2), NL-003 (x2), NL-001
RAM 8kx8 near 7502S
RAM 8kx8 near 7503S
RAM 2kx8 near NL-001
RAM 32kx8 (x8) near NL-003 & NL-003
DIPs 10-position (x2)
PAL near 7504
PAL near 7501S

probably 7501S is damaged, I can not get a consistent read. 10 reads supplied for comparison.

***************************************************************************/

ROM_START( animaljr )
	ROM_REGION( 0x50000, REGION_CPU1, 0 )	/* Z80 Code */
	ROM_LOAD( "7502s.2e",     0x00000, 0x40000, CRC(4b14a4be) SHA1(79f7207f7311c627ece1a0d8571b4bddcdefb336) )
	ROM_RELOAD(               0x10000, 0x40000 )

	ROM_REGION( 0x20000, REGION_CPU2, 0 )	/* Z80 Code */
	ROM_LOAD( "7503s.8e",     0x00000, 0x20000, CRC(d1fac899) SHA1(dde2824d73b13c18b83e4c4b63fe7835bce87ea4) )	// 1xxxxxxxxxxxxxxxx = 0xFF

	ROM_REGION( 0x200000, REGION_GFX1, 0 )	/* blitter data */
	ROM_LOAD( "7504.17b",     0x000000, 0x100000, CRC(b62de6a3) SHA1(62abf09b52844d3b3325e8931cb572c15581964f) )
	ROM_LOAD( "7505.17d",     0x100000, 0x080000, CRC(729b073f) SHA1(8e41fafc47adbe76452e92ab1459536a5a46784d) )
	ROM_LOAD( "7506s.17f",    0x180000, 0x080000, CRC(1be1ae17) SHA1(57bf9bcd9df49cdbb1311ec9e850cb1a141e5069) )

	ROM_REGION( 0x40000, REGION_SOUND1, 0 )	/* Samples */
	ROM_LOAD( "7501s_0.1h",   0x00000, 0x40000, BAD_DUMP CRC(59debb66) SHA1(9021722d3f8956946f102eddc7c676e1ef41574e) )
ROM_END


/***************************************************************************

Quiz Channel Question (JPN ver.)
(c)1993 Nakanihon

N7311208L1-2
N73SUB

CPU:    TMPZ84C015BF-8

Sound:  YM2413
        M6295

OSC:    16MHz
    28.6363MHz
    32.768KHz ?

Custom: NL-002 - Nakanihon
    (1108F0405) - Dynax
    (1427F0071) - Dynax

Others: M6242B (RTC?)

***************************************************************************/

ROM_START( quizchq )
	ROM_REGION( 0x118000, REGION_CPU1, 0 )	/* Z80 Code + space for banked RAM */
	ROM_LOAD( "nwc7302.3e",   0x00000, 0x80000, CRC(14217f2d) SHA1(3cdffcf73e62586893bfaa7c47520b0698d3afda) )
	ROM_RELOAD(               0x10000, 0x80000 )
	ROM_LOAD( "nwc7303.4e",   0x90000, 0x80000, CRC(ffc77601) SHA1(b25c4a027e1fa4397dd86299dfe9251022b0d174) )

	ROM_REGION( 0x320000, REGION_GFX1, 0 )	/* blitter data */
	ROM_LOAD( "nwc7307.s4b",  0x000000, 0x80000, CRC(a09d1dbe) SHA1(f17af24293eea803ebb5c758bffb4519dcad3a71) )
	ROM_LOAD( "nwc7306.s3b",  0x080000, 0x80000, CRC(52d27aac) SHA1(3c38278a5ce757ca0c4a22e4de6052132edd7cbc) )
	ROM_LOAD( "nwc7305.s2b",  0x100000, 0x80000, CRC(5f50914e) SHA1(1fe5df146e028995c53a5aca896546898d7b5914) )
	ROM_LOAD( "nwc7304.s1b",  0x180000, 0x80000, CRC(72866919) SHA1(12b0c95f98c8c76a47e561e1d5035b62f1ec0789) )
	ROM_LOAD( "nwc7310.s4a",  0x200000, 0x80000, CRC(5939aeab) SHA1(6fcf63d6801cb506822a6d06b7bce45ecbb0b4dd) )
	ROM_LOAD( "nwc7309.s3a",  0x280000, 0x80000, CRC(88c863b2) SHA1(60e5098c84ffb302abce788a064c323bece9cc6b) )
	ROM_LOAD( "nwc7308.s2a",  0x300000, 0x20000, CRC(6eb5c81d) SHA1(c8e31e246e1235c045f5a881c6db43a2aff848ff) )

	ROM_REGION( 0x80000, REGION_SOUND1, 0 )	/* Samples */
	ROM_LOAD( "nwc7301.1f",   0x00000, 0x80000, CRC(52c672e8) SHA1(bc05155f4d9c711cc2ed187a4dd2207b886452f0) )	// 2 banks
ROM_END

ROM_START( quizchql )
	ROM_REGION( 0x118000, REGION_CPU1, 0 )	/* Z80 Code + space for banked RAM */
	ROM_LOAD( "2.rom",        0x00000, 0x80000, CRC(1bf8fb25) SHA1(2f9a62654a018f19f6783be655d992c457551fc9) )
	ROM_RELOAD(               0x10000, 0x80000 )
	ROM_LOAD( "3.rom",        0x90000, 0x80000, CRC(6028198f) SHA1(f78c3cfc0663b44655cb75928941a5ec4a57c8ba) )

	ROM_REGION( 0x420000, REGION_GFX1, 0 )	/* blitter data */
	ROM_LOAD( "4.rom",        0x000000, 0x80000, CRC(e6bdea31) SHA1(cb39d1d5e367ad2623c2bd0b2966541aa41bbb9b) )
	ROM_LOAD( "5.rom",        0x080000, 0x80000, CRC(c243f10a) SHA1(22366a9441b8317780e85065accfa59fe1cd8258) )
	ROM_LOAD( "11.rom",       0x100000, 0x80000, CRC(c9ae5880) SHA1(1bbda7293178132797dd017d71b24aba5ce57022) )
	ROM_LOAD( "7.rom",        0x180000, 0x80000, CRC(a490aa4e) SHA1(05ff9982f0fb1062701063905aeeb50f37283e18) )
	ROM_LOAD( "6.rom",        0x200000, 0x80000, CRC(fbf713b6) SHA1(3ce73fa30dc020053b313dca1587ef6dd8ba1690) )
	ROM_LOAD( "8.rom",        0x280000, 0x80000, CRC(68d4b79f) SHA1(5937760495461dbe6a12670d631754c772171289) )
	ROM_LOAD( "10.rom",       0x300000, 0x80000, CRC(d56eaf0e) SHA1(56214de0b08c7db703a9af7dfd7e2deb74f36542) )
	ROM_LOAD( "9.rom",        0x380000, 0x80000, CRC(a11d535a) SHA1(5e95f07807cd2a5a0eae6cb5c70ccf4516d65124) )
	ROM_LOAD( "12.rom",       0x400000, 0x20000, CRC(43f8e5c7) SHA1(de4c8cc0948b0ce9e1ddf4bea434a7640db451e2) )

	ROM_REGION( 0x80000, REGION_SOUND1, 0 )	/* Samples */
	ROM_LOAD( "1snd.rom",     0x00000, 0x80000, CRC(cebb9220) SHA1(7a2ee750f2e608a37858b849914316dc778bcae2) )	// 2 banks
ROM_END



ROM_START( quiz365 )
	ROM_REGION( 0x180000, REGION_CPU1, 0 ) /* 68000 Code */
	ROM_LOAD16_BYTE( "7805.4b",  0x000000, 0x080000, CRC(70f93543) SHA1(03fb3f19b451c49359719e72baf294b2e9873307) )
	ROM_LOAD16_BYTE( "7804.4d",  0x000001, 0x080000, CRC(2ae003f4) SHA1(4aafc75a68989d3a006a5959a64d589472f17474) )
	ROM_LOAD16_BYTE( "7803.3b",  0x100000, 0x040000, CRC(10d315b1) SHA1(9f1bb57ba32152cca3b88fc3f841451b2b506a74) )
	ROM_LOAD16_BYTE( "7802.3d",  0x100001, 0x040000, CRC(6616caa3) SHA1(3b3fda61fa62c10b4d9e07e898018ffc9fab0f91) )

	ROM_REGION( 0x380000, REGION_GFX1, 0 )	/* blitter data */
	ROM_LOAD( "7810.14b", 0x000000, 0x100000, CRC(4b1a4984) SHA1(581ee032b396d65cd604f39846153a4dcb296aad) )
	ROM_LOAD( "7809.13b", 0x100000, 0x100000, CRC(139d52ab) SHA1(08d705301379fcb952cbb1add0e16a148e611bbb) )
	ROM_LOAD( "7808.12b", 0x200000, 0x080000, CRC(a09fd4a4) SHA1(016ecbf1d27a4890dee01e1966ec5efff6eb3afe) )
	ROM_LOAD( "7807.11b", 0x280000, 0x080000, CRC(988b3e84) SHA1(6c42d33c15806d1abe83994370c07ab7e446a111) )
	ROM_LOAD( "7806.10b", 0x300000, 0x080000, CRC(7f9aa228) SHA1(e5b4ece2df4d85c61af1fb9fbb8530fd3b8ef35e) )

	ROM_REGION( 0x100000, REGION_SOUND1, 0 )	/* Samples */
	// piggy-backed sample roms dumped as 2 separate files
	ROM_LOAD( "7801.1fu",     0x000000, 0x080000, CRC(53519d67) SHA1(c83b8504d5154c6667e25ff6e222e190ae771bc0) )
	ROM_LOAD( "7801.1fd",     0x080000, 0x080000, CRC(448c58dd) SHA1(991a4e2f82d2ee9b0839a76962c00e0848623879) )
ROM_END

ROM_START( quiz365t )
	ROM_REGION( 0x180000, REGION_CPU1, 0 ) /* 68000 Code */
	ROM_LOAD16_BYTE( "7805.rom", 0x000000, 0x080000, CRC(6db33222) SHA1(5f0cc9a15815252d8d5e85975ce8770717eb3ac8) )
	ROM_LOAD16_BYTE( "7804.rom", 0x000001, 0x080000, CRC(46d04ace) SHA1(b6489309d7704d2382802aa0f2f7526e367667ad) )
	ROM_LOAD16_BYTE( "7803.rom", 0x100000, 0x040000, CRC(5b7a78d3) SHA1(6ade16df301b57e4a7309834a47ca72300f50ffa) )
	ROM_LOAD16_BYTE( "7802.rom", 0x100001, 0x040000, CRC(c3238a9d) SHA1(6b4b2ab1315fc9e2667b4f8f394e00a27923f926) )

	ROM_REGION( 0x400000, REGION_GFX1, 0 )	/* blitter data */
	ROM_LOAD( "7810.rom", 0x000000, 0x100000, CRC(4b1a4984) SHA1(581ee032b396d65cd604f39846153a4dcb296aad) )
	ROM_LOAD( "7809.rom", 0x100000, 0x100000, CRC(139d52ab) SHA1(08d705301379fcb952cbb1add0e16a148e611bbb) )
	ROM_LOAD( "7808.rom", 0x200000, 0x080000, CRC(a09fd4a4) SHA1(016ecbf1d27a4890dee01e1966ec5efff6eb3afe) )
	ROM_LOAD( "7806.rom", 0x280000, 0x100000, CRC(75767c6f) SHA1(aef925dec3acfc01093d29f44e4a70f0fe28f66d) )
	ROM_LOAD( "7807.rom", 0x380000, 0x080000, CRC(60fb1dfe) SHA1(35317220b6401ccb03bb4ab7d3c0b6ab7637d82a) )

	ROM_REGION( 0x100000, REGION_SOUND1, 0 )	/* Samples */
	ROM_LOAD( "7801.rom", 0x080000, 0x080000, CRC(285cc62a) SHA1(7cb3bd0ead303787964bcf7a0ecf896b6a6bfa54) )	// bank 2,3
	ROM_CONTINUE(         0x000000, 0x080000 )				// bank 0,1
ROM_END



/***************************************************************************

                                Rong Rong

Here are the proms for Nakanihon's Rong Rong
It's a quite nice Puzzle game.
The CPU don't have any numbers on it except for this:
Nakanihon
NL-002
3J3  JAPAN
For the sound it uses A YM2413

***************************************************************************/

ROM_START( rongrong )
	ROM_REGION( 0x118000, REGION_CPU1, 0 )	/* Z80 Code + space for banked RAM */
	ROM_LOAD( "8002e.3e",     0x00000, 0x80000, CRC(062fa1b6) SHA1(f15a78c4192dbc56bb6ac0f92cffee88040b0a17) )
	ROM_RELOAD(               0x10000, 0x80000 )
	/* 90000-10ffff empty */

	ROM_REGION( 0x280000, REGION_GFX1, 0 )	/* blitter data */
	ROM_LOAD( "8003.8c",      0x000000, 0x80000, CRC(f57192e5) SHA1(e33f5243028520492cd876be3e4b6a76a9b20d46) )
	ROM_LOAD( "8004.9c",      0x080000, 0x80000, CRC(c8c0b5cb) SHA1(d0c99908022b7d5d484e6d1990c00f15f7d8665a) )
	ROM_LOAD( "8005e.10c",    0x100000, 0x80000, CRC(11c7a23c) SHA1(96d6b82db2555f7d0df661367a7a09bd4eaecba9) )
	ROM_LOAD( "8006e.11c",    0x180000, 0x80000, CRC(137e9b83) SHA1(5458f8982ce84990f0bc56f9269e46c691301ba1) )
	ROM_LOAD( "8007e.12c",    0x200000, 0x80000, CRC(374a1d50) SHA1(bbbbaf048b06caaca292b9e3d4bf408ba5259ad6) )

	ROM_REGION( 0x40000, REGION_SOUND1, 0 )	/* Samples */
	ROM_LOAD( "8001w.2f",     0x00000, 0x40000, CRC(8edc87a2) SHA1(87e8ad50be025263e682cbfb5623f3a35b17118f) )
ROM_END

ROM_START( rongrngg )
	ROM_REGION( 0x118000, REGION_CPU1, 0 )	/* Z80 Code + space for banked RAM */
	ROM_LOAD( "rr_8002g.rom", 0x00000, 0x80000, CRC(9a5d2885) SHA1(9ca049085d14b1cfba6bd48adbb0b883494e7d29) )
	ROM_RELOAD(               0x10000, 0x80000 )
	/* 90000-10ffff empty */

	ROM_REGION( 0x280000, REGION_GFX1, 0 )	/* blitter data */
	ROM_LOAD( "rr_8003.rom",  0x000000, 0x80000, CRC(f57192e5) SHA1(e33f5243028520492cd876be3e4b6a76a9b20d46) )
	ROM_LOAD( "rr_8004.rom",  0x080000, 0x80000, CRC(c8c0b5cb) SHA1(d0c99908022b7d5d484e6d1990c00f15f7d8665a) )
	ROM_LOAD( "rr_8005g.rom", 0x100000, 0x80000, CRC(11c7a23c) SHA1(96d6b82db2555f7d0df661367a7a09bd4eaecba9) )
	ROM_LOAD( "rr_8006g.rom", 0x180000, 0x80000, CRC(f3de77e6) SHA1(13839837eab6acf6f8d6a9ca08fe56c872d50e6a) )
	ROM_LOAD( "rr_8007g.rom", 0x200000, 0x80000, CRC(38a8caa3) SHA1(41d6745bb340b7f8708a6b772f241989aa7fa09d) )

	ROM_REGION( 0x40000, REGION_SOUND1, 0 )	/* Samples */
	ROM_LOAD( "rr_8001w.rom", 0x00000, 0x40000, CRC(8edc87a2) SHA1(87e8ad50be025263e682cbfb5623f3a35b17118f) )
ROM_END


/***************************************************************************

Netto Quiz Champion (c) Nakanihon

CPU: 68HC000
Sound: OKI6295
Other: HN46505, unknown 68 pin, unknown 100 pin (x2), unknown 64 pin (part numbers scratched off).
PLDs: GAL16L8B (x2, protected)
RAM: TC524258BZ-10 (x5), TC55257BSPL-10 (x2), TC5588P-35
XTAL1: 16 MHz
XTAL2: 28.63636 MHz

***************************************************************************/

ROM_START( nettoqc )
	ROM_REGION( 0x180000, REGION_CPU1, 0 )		/* 68000 Code */
	ROM_LOAD16_BYTE( "10305.rom", 0x000000, 0x080000, CRC(ebb14a1f) SHA1(5e4511a878d0bcede79a287fb184e912c9eb7dc5) )
	ROM_LOAD16_BYTE( "10303.rom", 0x000001, 0x080000, CRC(30c114c3) SHA1(fa9c26d465d2d919e141bbc080a04ac0f87c7010) )
	ROM_LOAD16_BYTE( "10306.rom", 0x100000, 0x040000, CRC(f19fe827) SHA1(37907bf3206af5f4613dc80b6bd91c87dd6645ab) )
	ROM_LOAD16_BYTE( "10304.rom", 0x100001, 0x040000, CRC(da1f56e5) SHA1(76c865927ee8392dd77476a248816e04e60c784a) )
	ROM_CONTINUE(                 0x100001, 0x040000 )	// 1ST AND 2ND HALF IDENTICAL

	ROM_REGION( 0x400000, REGION_GFX1, 0 )	/* blitter data */
	ROM_LOAD( "10307.rom", 0x000000, 0x100000, CRC(c7a3b05f) SHA1(c931670c5d14f8446404ad00d785fa73d97dedfc) )
	ROM_LOAD( "10308.rom", 0x100000, 0x100000, CRC(416807a1) SHA1(bccf746ddc9750e3956299fec5b3737a53b24c36) )
	ROM_LOAD( "10309.rom", 0x200000, 0x100000, CRC(81841272) SHA1(659c009c41ae54d330da41922c8afd1fb293d854) )
	ROM_LOAD( "10310.rom", 0x300000, 0x080000, CRC(0f790cda) SHA1(97c79b02ba95551514f8dee701bd71b53e41abf4) )
	ROM_LOAD( "10311.rom", 0x380000, 0x080000, CRC(41109231) SHA1(5e2f4684fd65dcdfb61a94099e0600c23a4740b2) )

	ROM_REGION( 0x100000, REGION_SOUND1, 0 )	/* Samples */
	ROM_LOAD( "10301.rom", 0x000000, 0x080000, CRC(52afd952) SHA1(3ed6d92b78552d390ee305bb216648dbf6d63daf) )
	ROM_LOAD( "10302.rom", 0x080000, 0x080000, CRC(6e2d4660) SHA1(d7924af8807f7238a7885b204a8c352ff75298b7) )
ROM_END


/***************************************************************************

Don Den Lover Vol 1
(C) Dynax Inc 1995

CPU: TMP68HC000N-12
SND: OKI M6295, YM2413 (18 pin DIL), YMZ284-D (16 pin DIL. This chip is in place where a 40 pin chip is marked on PCB,
                                     possibly a replacement for some other 40 pin YM chip?)
OSC: 28.636MHz (near large GFX chip), 24.000MHz (near CPU)
DIPS: 1 x 8 Position switch. DIP info is in Japanese !
RAM: 1 x Toshiba TC5588-35, 2 x Toshiba TC55257-10, 5 x OKI M514262-70

OTHER:
Battery
RTC 72421B   4382 (18 pin DIL)
3 X PAL's (2 on daughter-board at locations 2E & 2D, 1 on main board near CPU at location 4C)
GFX Chip - NAKANIHON NL-005 (208 pin, square, surface-mounted)

***************************************************************************/

ROM_START( ddenlovr )
	ROM_REGION( 0x080000, REGION_CPU1, 0 )		/* 68000 Code */
	ROM_LOAD16_BYTE( "1134h.1a", 0x000000, 0x040000, CRC(43accdff) SHA1(3023d4a071fc877f8e4325e95e586739077ccb02) )
	ROM_LOAD16_BYTE( "1133h.1c", 0x000001, 0x040000, CRC(361bf7b6) SHA1(1727112284cd1dcc1ed17ccba214cb0f8993650a) )

	ROM_REGION( 0x480000, REGION_GFX1, 0 )	/* blitter data */
	/* 000000-1fffff empty */
	ROM_LOAD( "1135h.3h", 0x200000, 0x080000, CRC(ee143d8e) SHA1(61a36c64d450209071e996b418adf416dfa68fd9) )
	ROM_LOAD( "1136h.3f", 0x280000, 0x080000, CRC(58a662be) SHA1(3e2fc167bdee74ebfa63c3b1b0d822e3d898c30c) )
	ROM_LOAD( "1137h.3e", 0x300000, 0x080000, CRC(f96e0708) SHA1(e910970a4203b9b1943c853e3d869dd43cdfbc2d) )
	ROM_LOAD( "1138h.3d", 0x380000, 0x080000, CRC(633cff33) SHA1(aaf9ded832ae8889f413d3734edfcde099f9c319) )
	ROM_LOAD( "1139h.3c", 0x400000, 0x080000, CRC(be1189ca) SHA1(34b4102c6341ade03a1d44b6049ffa15666c6bb6) )

	ROM_REGION( 0x200000, REGION_SOUND1, 0 )	/* Samples */
	ROM_LOAD( "1131h.1f", 0x080000, 0x080000, CRC(32f68241) SHA1(585b5e0d2d959af8b57ecc0a277aeda27e5cae9c) )	// bank 2, 3
	ROM_LOAD( "1132h.1e", 0x100000, 0x080000, CRC(2de6363d) SHA1(2000328e41bc0261f19e02323434e9dfdc61013a) )	// bank 4, 5
ROM_END


static DRIVER_INIT( rongrong )
{
	/* Rong Rong seems to have a protection that works this way:
        - write 01 to port c2
        - write three times to f705 (a fixed command?)
        - write a parameter to f706
        - read the answer back from f601
        - write 00 to port c2
       The parameter is read from RAM location 60d4, and the answer
       is written back there. No matter what the protection device
       does, it seems that making 60d4 always read 0 is enough to
       bypass the protection. Actually, I'm wondering if this
       version of the game might be a bootleg with the protection
       patched. (both sets need this)
     */
	memory_install_read8_handler(0, ADDRESS_SPACE_PROGRAM, 0x60d4, 0x60d4, 0, 0, MRA8_NOP);
}

/***************************************************************************

HANAKANZASHI
(c)1996 DYNAX.INC
CPU : Z-80 (TMPZ84C015BF-8)
SOUND : MSM6295 YM2413
REAL TIME CLOCK : MSM6242

***************************************************************************/

ROM_START( hanakanz )
	ROM_REGION( 0x90000+16*0x1000, REGION_CPU1, 0 )	/* Z80 Code */
	ROM_LOAD( "50720.5b",     0x00000, 0x80000, CRC(dc40fcfc) SHA1(32c8b3d23039ac47504c881552572f2c22afa585) )
	ROM_RELOAD(               0x10000, 0x80000 )

	ROM_REGION( 0x300000, REGION_GFX1, 0 )	/* blitter data */
	ROM_LOAD16_BYTE( "50740.8b",     0x000000, 0x80000, CRC(999e70ce) SHA1(421c137b43522fbf9f3f5aa86692dc563af86880) )
	ROM_LOAD16_BYTE( "50730.8c",     0x000001, 0x80000, CRC(54e1731d) SHA1(c3f60c4412665b379b4b630ead576691d7b2a598) )
	ROM_LOAD16_BYTE( "50760.10b",    0x100000, 0x80000, CRC(8fcb5da3) SHA1(86bd4f89e860cd476a026c21a87f34b7a208c539) )
	ROM_LOAD16_BYTE( "50750.10c",    0x100001, 0x80000, CRC(0e58bf9e) SHA1(5e04a637fc81fd48c6e1626ec06f2f1f4f52264a) )
	ROM_LOAD16_BYTE( "50780.12b",    0x200000, 0x80000, CRC(6dfd8a86) SHA1(4d0c9f2028533ebe51f2963cb776bde5c802883e) )
	ROM_LOAD16_BYTE( "50770.12c",    0x200001, 0x80000, CRC(118e6baf) SHA1(8e14baa967af87a74558f80584b7d483c98112be) )

	ROM_REGION( 0x80000, REGION_SOUND1, 0 )	/* Samples */
	ROM_LOAD( "50710.1c",     0x00000, 0x80000, CRC(72ae072f) SHA1(024af2ae6aa12b7f76d12a9c589f07ec7f47e395) )	// 2 banks
ROM_END


/***************************************************************************

Hana Kagerou
(c)1996 Nakanihon (Dynax)

CPU:    KL5C80A12

Sound:  YM2413
        M6295?

OSC:    20.000MHz
        28.63636MHz

Custom: (70C160F011)


NM5101.1C   samples

NM5102.5B   prg.

NM5103.8C   chr.
NM5104.8B
NM5105.10C
NM5106.10B
NM5107.12C
NM5108.12B

***************************************************************************/

ROM_START( hkagerou )
	ROM_REGION( 0x90000+16*0x1000, REGION_CPU1, 0 )	/* Z80 Code */
	ROM_LOAD( "nm5102.5b",    0x00000, 0x80000, CRC(c56c0856) SHA1(9b3c17c80498c9fa0ea91aa876aa4853c95ebb8c) )
	ROM_RELOAD(               0x10000, 0x80000 )

	ROM_REGION( 0xe80000, REGION_GFX1, 0 )	/* blitter data */

	ROM_LOAD16_BYTE( "nm5104.8b",    0xc00000, 0x080000, CRC(e91dd92b) SHA1(a4eb8a6237e63639da5fc1bc504c8dc2aee99ff5) )
	ROM_LOAD16_BYTE( "nm5103.8c",    0xc00001, 0x080000, CRC(4d4e248b) SHA1(f981ba8a05bac59c665fb0fd201ea8ff3bd87a3c) )
	ROM_LOAD16_BYTE( "nm5106.10b",   0xd00000, 0x080000, CRC(0853c32d) SHA1(120094d439f6bee05681e5d22998616639412011) )
	ROM_LOAD16_BYTE( "nm5105.10c",   0xd00001, 0x080000, CRC(f109ec10) SHA1(05b86f7e02329745b6208941d5ca02d392e8526f) )
	ROM_LOAD16_BYTE( "nm5108.12b",   0xe00000, 0x040000, CRC(d0a99b19) SHA1(555ba04f13e6f372f2b5fd6b6bafc9de65c78505) )
	ROM_LOAD16_BYTE( "nm5107.12c",   0xe00001, 0x040000, CRC(65a0ebbd) SHA1(81c108ed647b8f8c2903c4b01c8bc314ecfd9796) )

	ROM_REGION( 0x80000, REGION_SOUND1, 0 )	/* Samples */
	ROM_LOAD( "nm5101.1c",    0x00000, 0x80000, CRC(bf7a397e) SHA1(163dfe68873bfcdf28bf11f235b3ca17e8bbf02d) )	// 2 banks
ROM_END


/***************************************************************************

Mahjong Reach Ippatsu
(c)1998 Nihon System/Dynax

CPU:   KL5C80A12

Sound: YM2413
       M6295

OSC:   20.000MHz
       28.63636MHz
       32.768KHz

Custom: (70C160F011)
Others: M6242B (RTC)


52601.1C    samples

52602-N.5B  prg.

52603.8C    chr.
52604.8B
52605.10C
52606.10B
52607.12C
52608.12B

***************************************************************************/

ROM_START( mjreach1 )
	ROM_REGION( 0x90000+16*0x1000, REGION_CPU1, 0 )	/* Z80 Code */
	ROM_LOAD( "52602-n.5b",   0x00000, 0x80000, CRC(6bef7978) SHA1(56e38448fb03e868094d75e5b7de4e4f4a4e850a) )
	ROM_RELOAD(               0x10000, 0x80000 )

	ROM_REGION( 0x500000, REGION_GFX1, 0 )	/* blitter data */
	ROM_LOAD16_BYTE( "52604.8b",     0x000000, 0x100000, CRC(6ce01bb4) SHA1(800043d8203ab5560ed0b24e0a4e01c14b6a3ac0) )
	ROM_LOAD16_BYTE( "52603.8c",     0x000001, 0x100000, CRC(16d2c169) SHA1(3e50b1109c86d0e8f931ce5a3abf20d807ebabba) )
	ROM_LOAD16_BYTE( "52606.10b",    0x200000, 0x100000, CRC(07fe5dae) SHA1(221ec21c2d84497af5b769d7409f8775be933783) )
	ROM_LOAD16_BYTE( "52605.10c",    0x200001, 0x100000, CRC(b5d57163) SHA1(d6480904bd72d298d48fbcb251b902b0b994cab1) )
	ROM_LOAD16_BYTE( "52608.12b",    0x400000, 0x080000, CRC(2f93dde4) SHA1(8efaa920e485f50ef7f4396cc8c47dfbfc97bd01) )
	ROM_LOAD16_BYTE( "52607.12c",    0x400001, 0x080000, CRC(5e685c4d) SHA1(57c99fb791429d0edb7416cffb4d1d1eb34a2813) )

	ROM_REGION( 0x80000, REGION_SOUND1, 0 )	/* Samples */
	ROM_LOAD( "52601.1c",     0x00000, 0x80000, CRC(52666107) SHA1(1e1c17b1da7ded5fc52422c7e569ef02af1ee11d) )	// 2 banks
ROM_END

/***************************************************************************

Mahjong Chuukanejyo
Dynax, 1995

PCB Layout
----------
D11107218L1
|-----------------------------------------------|
|10WAY           18WAY          D12101 5.5V_BATT|
|          358     358        6606              |
|      VOL    6868A                             |
|                         16MHz                 |
|           95101                   62256       |
|                        TMPZ84C015F-6          |
|                                D12102         |
|2                                        3631  |
|8                                              |
|W                                  PAL         |
|A            28.322MHz                         |
|Y                                              |
|                          PAL                  |
|             70C160F009                        |
|                           D12103      D12104  |
|              TC524256Z-12                     |
|              TC524256Z-12 D12105      D12106  |
|DIP1     DIP2 TC524256Z-12                     |
|DIP3     DIP4 TC524256Z-12 D12107      D12108  |
|-----------------------------------------------|
Notes:
      Main CPU is Toshiba TMPZ84C015F-6 (QFP100)
      95101 - Unknown 40 pin DIP, maybe equivalent to AY-3-8910?
      6868A - Unknown 18 pin DIP, maybe some other sound related chip or a PIC?
      3631  - Unknown 18 pin DIP, maybe RTC or a PIC?
      6606  - Unknown QFP44, probably OKI M6295?
      70C160F009 - QFP208 Dynax Custom


***************************************************************************/

ROM_START( mjchuuka )
	ROM_REGION( 0x90000+16*0x1000, REGION_CPU1, 0 )	/* Z80 Code */
	ROM_LOAD( "d12102.5b", 0x00000, 0x80000, CRC(585a0a8e) SHA1(94b3eede36117fe0a34b61454484c72cd7f0ce6a) )
	ROM_RELOAD(            0x10000, 0x80000 )

	ROM_REGION( 0x300000, REGION_GFX1, ROMREGION_ERASEFF )	/* blitter data */
	ROM_LOAD16_BYTE( "d12103.11c", 0x000000, 0x080000, CRC(83bfc841) SHA1(36547e737244f95004c598adeb46cebce9ab3231) )
	ROM_LOAD16_BYTE( "d12104.11a", 0x000001, 0x080000, CRC(1bf6220a) SHA1(ea18fdf6e1298a3b4c91fbf6219b1edcfecaeca3) )
	ROM_LOAD16_BYTE( "d12105.12c", 0x100000, 0x080000, CRC(3424c8ac) SHA1(ee48622b478d39c6bdb5a18cab204e14f7d54f7a) )
	ROM_LOAD16_BYTE( "d12106.12a", 0x100001, 0x080000, CRC(9052bd09) SHA1(3e8e32dea6c0cea895b7f16883e500e487689e72) )
	ROM_LOAD16_BYTE( "d12107.13c", 0x280000, 0x020000, CRC(184afa94) SHA1(57566123a6dde661770740ad7a6c364c7ef5de86) )	// 1xxxxxxxxxxxxxxxx = 0xFF
	ROM_LOAD16_BYTE( "d12108.13a", 0x280001, 0x020000, CRC(f8e8558a) SHA1(69e64c83945c6462b704b6d9d0250c9d98f66859) )	// 1xxxxxxxxxxxxxxxx = 0xFF

	ROM_REGION( 0x80000, REGION_SOUND1, 0 )	/* Samples */
	ROM_LOAD( "d12101.1b", 0x00000, 0x80000, CRC(9759c65e) SHA1(cf098c07616b6d2a2ba10ff6ae0006442b675326) )
ROM_END


/***************************************************************************

Mahjong The Dai Chuuka Ken (China Version)
Dynax, 1995

PCB Layout
----------

D11107218L1 DYNAX INC. NAGOYA JAPAN
|-----------------------------------------------------|
|10-WAY              18-WAY                  1    5.5V|
|                               6606         x        |
|   MB3712  VOL  358                                  |
|                358                                  |
|                               16MHz        43256    |
|                 6868A                               |
|              95101            Z84C015      2        |
|                                                     |
|2                                              3631  |
|8                                                    |
|W                                                    |
|A                                         PAL        |
|Y                     28.322MHz      PAL             |
|                                                     |
|                         |---------|                 |
|                         |NAKANIHON|                 |
|                         |70C160F009   3       4     |
|                 44C251  |         |                 |
|                 44C251  |         |   5       6     |
| DSW1     DSW2   44C251  |---------|                 |
| DSW3     DSW4   44C251                7       8     |
|-----------------------------------------------------|
Notes:
      PCB uses common 10-way/18-way and 28-way Mahjong pinouts
      5.5V    - Battery
      6606    - Compatible to M6295 (QFP44)
      6868A   - compatible to YM2413 (DIP18)
      3631    - Unknown DIP18 chip (maybe RTC?)
      Z84C015 - Toshiba TMPZ84C015BF-6 Z80 compatible CPU (clock input 16.0MHz)
      44C251  - Texas Instruments TMS44C251-12SD 256k x4 Dual Port VRAM (ZIP28)
      95101   - Winbond 95101, compatible to AY-3-8910 (DIP40)
      43256   - NEC D43256 32k x8 SRAM (DIP28)
      70C160F009 - Custom Dynax graphics generator (QFP160)
      All DIPSW's have 10 switches per DIPSW
      All ROMs are 27C040
                          1   - Sound samples
                          2   - Main program
                          3,4 - Graphics
                          5-8 - unused DIP32 sockets

      The same PCB is used with 'Mahjong Zhong Hua Er Nu', with ROM locations
      as follows....
                    1 - D1111-A.1B
                    2 - D12102.5B
                    3 - D12103.11C
                    4 - D12104.11A
                    5 - D12105.12C
                    6 - D12106.12A
                    7 - D12107.13C
                    8 - D12108.13A

***************************************************************************/

ROM_START( mjdchuka )
	ROM_REGION( 0x90000+16*0x1000, REGION_CPU1, 0 )	/* Z80 Code */
	ROM_LOAD( "2.5b", 0x00000, 0x80000, CRC(7957b4e7) SHA1(8b76c15694e42ff0b2ec5aeae059bf342f6bf476) )
	ROM_RELOAD(       0x10000, 0x80000 )

	ROM_REGION( 0x100000, REGION_GFX1, ROMREGION_ERASEFF )	/* blitter data */
	ROM_LOAD16_BYTE( "3.11c", 0x000000, 0x080000, CRC(c66553c3) SHA1(6e5380fdb97cc8b52986f3a3a8cac43c0f38cf54) )
	ROM_LOAD16_BYTE( "4.11a", 0x000001, 0x080000, CRC(972852fb) SHA1(157f0a772bf060efc39033b10e63a6cb1022edf6) )

	ROM_REGION( 0x80000, REGION_SOUND1, 0 )	/* Samples */
	ROM_LOAD( "1.1b", 0x00000, 0x80000, CRC(9759c65e) SHA1(cf098c07616b6d2a2ba10ff6ae0006442b675326) )
ROM_END


/***************************************************************************

The First Funky Fighter
Nakanihon, 1994

PCB Layout
----------

N7403208L-2
|------------------------------------------------------------------|
|    VR1                7401          7402  32.768kHz M6242 3V_BATT|
|    VR2          358         PAL        M6295  TC55257  PAL       |
|       YM2413          TC5563                             16MHz   |
|                 358                          7403                |
|       YM2149          Z80                         TMPZ84C015BF-8 |
|                                                                  |
|J                                                                 |
|A                                                                 |
|M          NL-002      PAL                                        |
|M                                                                 |
|A                                                     DSW(10)     |
|                                                                  |
|                                                      DSW(10)     |
|                                                                  |
|       TC5588                           28.6363MHz                |
|                                          |-ROM-sub-board-N73RSUB-|
|                                          |                       |
| DSW(4)                                   |NL-005         PAL     |
|       SN75179                            |                       |         Sub-board contains 12 sockets.
|                                          |        7404   7411 |----------- Only these 3 are populated.
|                                          |        7405   7410 /  |
|DB9   OMRON              NL-006     TC524258BZ-10  7406   7409/   |
|      G6A-474P        TC524258BZ-10 TC524258BZ-10  7407           |
|                      TC524258BZ-10 TC524258BZ-10  7408           |
|DB9                                 TC524258BZ-10         PAL     |
|                                          |             (on sub)  |
|------------------------------------------|-----------------------|

the second halves of 7408.13b, 7409.4b, 7410.3b and 7411.2b are identical

***************************************************************************/

ROM_START( funkyfig )
	ROM_REGION( 0x90000 + 0x1000*8, REGION_CPU1, 0 )	/* Z80 Code + space for banked RAM */
	ROM_LOAD( "7403.3c",      0x00000, 0x80000, CRC(ad0f5e14) SHA1(82de58d7ba35266f2d96503d72487796a9693996) )
	ROM_RELOAD(               0x10000, 0x80000 )

	ROM_REGION( 0x20000, REGION_CPU2, 0 )	/* Z80 Code */
	ROM_LOAD( "7401.1h",      0x00000, 0x20000, CRC(0f47d785) SHA1(d57733db6dcfb4c2cdaad04b5d3f0f569a0e7461) )	// 1xxxxxxxxxxxxxxxx = 0xFF

ROM_REGION( 0x500000, REGION_GFX1, ROMREGION_ERASE00 )	/* blitter data */
	ROM_LOAD( "7404.8b",      0x000000, 0x080000, CRC(aa4ddf32) SHA1(864890795a238ab34a85ca55a387d7e5efafccee) )			// \ 7e6f +
	ROM_LOAD( "7405.9b",      0x080000, 0x080000, CRC(fc125bd8) SHA1(150578f67d89be59eeeb811c159a789e5e9c993e) )			// / 35bb = b42a OK
	ROM_LOAD( "7406.10b",     0x100000, 0x080000, BAD_DUMP CRC(04a214b1) SHA1(af3e652377f5652377c7dedfad7c2677695eaf46) )	// \ af08 +
	ROM_LOAD( "7407.11b",     0x180000, 0x080000, BAD_DUMP CRC(635d4052) SHA1(7bc2f20d633c69352fc2d5634349c83055c99408) )	// / 6d64 = 1c6c ERR (should be 1c68!)
	ROM_LOAD( "7409.4b",      0x200000, 0x100000, CRC(064082c3) SHA1(26b0eec56b06365740b213b34e33a4b94ebc1d25) )			// \ 15bd +
	ROM_LOAD( "7410.3b",      0x280000, 0x100000, CRC(0ba67874) SHA1(3d984c77a843501e1075cadcc27820a35410ea3b) )			// / 2e4c = 4409 OK
	ROM_LOAD( "7408.13b",     0x300000, 0x100000, CRC(9efe4c60) SHA1(6462dca2af38517639bd2f182e68b7b1fc98a312) )			// 0f46 + 1825 = 276b OK
	ROM_LOAD( "7411.2b",      0x400000, 0x100000, CRC(1e9c73dc) SHA1(ba64de6168dc626dc89d38b3f9d8991163f5e63e) )			// f248 + 1825 OK (first half)

	ROM_REGION( 0x40000, REGION_SOUND1, 0 )	/* Samples */
	ROM_LOAD( "7402.1e",      0x000000, 0x040000, CRC(5038cc34) SHA1(65618b232a6592ad36f4abbaa40625c208a015fd) )
ROM_END

/***************************************************************************

The Mysterious World
(c) 1994 DynaX

Board has a sticker labeled D7707308L1
The actual PCB is printed as D7107058L1-1

Most all chips are surface scratched

OSC: 24.000MHz, 14.318MHz
4 x 10 Switch Dipswitch
1 4 Switch Dipswitch
VR1, VR2 & Reset Switch
3.6V Ni/CD Battery
OKI M6242B - Real Time Clock

56 pin Non-JAMMA Connector
20 pin unknown Connector
36 pin unknown Connector

Sound Chips:
K-665 (OKI M6295)
YM2149F
YM2413

***************************************************************************/

ROM_START( mjmyster )
	ROM_REGION( 0x90000 + 0x1000*8, REGION_CPU1, 0 )	/* Z80 Code + space for banked RAM */
	ROM_LOAD( "77t2.c3", 0x00000, 0x40000, CRC(b1427cce) SHA1(1640f5bb6275cce92e38cf3e0c788b4e65606459) )
	ROM_RELOAD(          0x10000, 0x40000 )

	ROM_REGION( 0x1a0000, REGION_GFX1, ROMREGION_ERASE00 )	/* blitter data */
	ROM_LOAD( "77t6.b12", 0x000000, 0x080000, CRC(a287589a) SHA1(58659dd7e019d1d32efeaec548c84a7ded637c50) )
	ROM_LOAD( "77t5.b11", 0x080000, 0x080000, CRC(a3475059) SHA1(ec86dcea3314b65d391a970680c021899c16449e) )
	ROM_LOAD( "77t4.b10", 0x100000, 0x080000, CRC(f45c24d6) SHA1(0eca68f2ca5722717f27ac0839359966daa2715b) )
	ROM_LOAD( "77t3.b9",  0x180000, 0x020000, CRC(8671165b) SHA1(23fad112909e82ac9d25dbb69bf6334f30fa6540) )

	ROM_REGION( 0x40000, REGION_SOUND1, 0 )	/* Samples */
	ROM_LOAD( "77t1.d1", 0x000000, 0x020000, CRC(09b7a9b2) SHA1(64d9ccbb726bb6c5b362afc92bca2e3db87fd454) )
ROM_END


GAME( 1992, mmpanic,  0,       mmpanic,  mmpanic,  0,        ROT0, "Nakanihon + East Technology (Taito license)", "Monkey Mole Panic (USA)",                    GAME_NO_COCKTAIL )
GAME( 1993, funkyfig, 0,       funkyfig, funkyfig, 0,        ROT0, "Nakanihon + East Technology (Taito license)", "The First Funky Fighter",                    GAME_NO_COCKTAIL | GAME_IMPERFECT_GRAPHICS )	// scrolling, priority?
GAME( 1993, quizchq,  0,       quizchq,  quizchq,  0,        ROT0, "Nakanihon",                                   "Quiz Channel Question (Ver 1.00) (Japan)",   GAME_NO_COCKTAIL | GAME_IMPERFECT_GRAPHICS | GAME_IMPERFECT_SOUND | GAME_NOT_WORKING )
GAME( 1993, quizchql, quizchq, quizchq,  quizchq,  0,        ROT0, "Nakanihon (Laxan license)",                   "Quiz Channel Question (Ver 1.23) (Taiwan?)", GAME_NO_COCKTAIL | GAME_IMPERFECT_GRAPHICS | GAME_IMPERFECT_SOUND | GAME_NOT_WORKING )
GAME( 1993, animaljr, 0,       mmpanic,  animaljr, 0,        ROT0, "Nakanihon + East Technology (Taito license)", "Animalandia Jr.",                            GAME_NO_COCKTAIL | GAME_IMPERFECT_SOUND )
GAME( 1994, mjmyster, 0,       mjmyster, mjmyster, 0,        ROT0, "Dynax",                                       "Mahjong The Mysterious World",               GAME_NO_COCKTAIL )
GAME( 1994, quiz365,  0,       quiz365,  quiz365,  0,        ROT0, "Nakanihon",                                   "Quiz 365 (Japan)",                           GAME_NO_COCKTAIL | GAME_IMPERFECT_GRAPHICS | GAME_UNEMULATED_PROTECTION )
GAME( 1994, quiz365t, quiz365, quiz365,  quiz365,  0,        ROT0, "Nakanihon + Taito",                           "Quiz 365 (Hong Kong & Taiwan)",              GAME_NO_COCKTAIL | GAME_IMPERFECT_GRAPHICS | GAME_UNEMULATED_PROTECTION )
GAME( 1994, rongrong, 0,       rongrong, rongrong, rongrong, ROT0, "Nakanihon",                                   "Rong Rong (Europe)",                         GAME_NO_COCKTAIL | GAME_IMPERFECT_COLORS )
GAME( 1994, rongrngg, rongrong,rongrong, rongrong, rongrong, ROT0, "Nakanihon",                                   "Rong Rong (Germany)",                        GAME_NO_COCKTAIL | GAME_IMPERFECT_COLORS )
GAME( 1995, mjdchuka, 0,       mjchuuka, mjchuuka, 0,        ROT0, "Dynax",                                       "Mahjong The Dai Chuuka Ken (China, v. D111)",GAME_NO_COCKTAIL )
GAME( 1995, nettoqc,  0,       nettoqc,  nettoqc,  0,        ROT0, "Nakanihon",                                   "Nettoh Quiz Champion (Japan)",               GAME_NO_COCKTAIL | GAME_IMPERFECT_COLORS )
GAME( 1996, ddenlovr, 0,       ddenlovr, ddenlovr, 0,        ROT0, "Dynax",                                       "Don Den Lover Vol. 1 (Hong Kong)",           GAME_NO_COCKTAIL )
GAME( 1996, hanakanz, 0,       hanakanz, hanakanz, 0,        ROT0, "Dynax",                                       "Hana Kanzashi (Japan)",                      GAME_NO_COCKTAIL )
GAME( 1997, hkagerou, 0,       hkagerou, hkagerou, 0,        ROT0, "Nakanihon + Dynax",                           "Hana Kagerou [BET] (Japan)",                 GAME_NO_COCKTAIL )
GAME( 1998, mjchuuka, 0,       mjchuuka, mjchuuka, 0,        ROT0, "Dynax",                                       "Mahjong Chuukanejyo (China)",                GAME_NO_COCKTAIL )
GAME( 1998, mjreach1, 0,       mjreach1, mjreach1, 0,        ROT0, "Nihon System",                                "Mahjong Reach Ippatsu (Japan)",              GAME_NO_COCKTAIL )
