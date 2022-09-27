//fix me -- blitz2k dies when starting a game with heavy fog (in DRC)
//optimize -- 64-bit multiplies for S/T

/*************************************************************************

    3dfx Voodoo Graphics SST-1/2 emulator

    emulator by Aaron Giles

    --------------------------

    Specs:

    Voodoo 1 (SST1):
        2,4MB frame buffer RAM
        1,2,4MB texture RAM
        50MHz clock frequency
        clears @ 2 pixels/clock (RGB and depth simultaneously)
        renders @ 1 pixel/clock
        64 entry PCI FIFO
        memory FIFO up to 65536 entries

    Voodoo 2:
        2,4MB frame buffer RAM
        2,4,8,16MB texture RAM
        90MHz clock frquency
        clears @ 2 pixels/clock (RGB and depth simultaneously)
        renders @ 1 pixel/clock
        ultrafast clears @ 16 pixels/clock
        128 entry PCI FIFO
        memory FIFO up to 65536 entries

    Voodoo Banshee (h3):
        Integrated VGA support
        2,4,8MB frame buffer RAM
        90MHz clock frquency
        clears @ 2 pixels/clock (RGB and depth simultaneously)
        renders @ 1 pixel/clock
        ultrafast clears @ 32 pixels/clock

    Voodoo 3 ("Avenger"/h4):
        Integrated VGA support
        4,8,16MB frame buffer RAM
        143MHz clock frquency
        clears @ 2 pixels/clock (RGB and depth simultaneously)
        renders @ 1 pixel/clock
        ultrafast clears @ 32 pixels/clock

    --------------------------

    still to be implemented:
        * trilinear textures

    things to verify:
        * floating Z buffer


iterated RGBA = 12.12 [24 bits]
iterated Z    = 20.12 [32 bits]
iterated W    = 18.32 [48 bits]

>mamepm blitz
Stall PCI for HWM: 1
PCI FIFO Empty Entries LWM: D
LFB -> FIFO: 1
Texture -> FIFO: 1
Memory FIFO: 1
Memory FIFO HWM: 2000
Memory FIFO Write Burst HWM: 36
Memory FIFO LWM for PCI: 5
Memory FIFO row start: 120
Memory FIFO row rollover: 3FF
Video dither subtract: 0
DRAM banking: 1
Triple buffer: 0
Video buffer offset: 60
DRAM banking: 1

>mamepm wg3dh
Stall PCI for HWM: 1
PCI FIFO Empty Entries LWM: D
LFB -> FIFO: 1
Texture -> FIFO: 1
Memory FIFO: 1
Memory FIFO HWM: 2000
Memory FIFO Write Burst HWM: 36
Memory FIFO LWM for PCI: 5
Memory FIFO row start: C0
Memory FIFO row rollover: 3FF
Video dither subtract: 0
DRAM banking: 1
Triple buffer: 0
Video buffer offset: 40
DRAM banking: 1


As a point of reference, the 3D engine uses the following algorithm to calculate the linear memory address as a
function of the video buffer offset (fbiInit2 bits(19:11)), the number of 32x32 tiles in the X dimension (fbiInit1
bits(7:4) and bit(24)), X, and Y:

    tilesInX[4:0] = {fbiInit1[24], fbiInit1[7:4], fbiInit6[30]}
    rowBase = fbiInit2[19:11]
    rowStart = ((Y>>5) * tilesInX) >> 1

    if (!(tilesInX & 1))
    {
        rowOffset = (X>>6);
        row[9:0] = rowStart + rowOffset (for color buffer 0)
        row[9:0] = rowBase + rowStart + rowOffset (for color buffer 1)
        row[9:0] = (rowBase<<1) + rowStart + rowOffset (for depth/alpha buffer when double color buffering[fbiInit5[10:9]=0])
        row[9:0] = (rowBase<<1) + rowStart + rowOffset (for color buffer 2 when triple color buffering[fbiInit5[10:9]=1 or 2])
        row[9:0] = (rowBase<<1) + rowBase + rowStart + rowOffset (for depth/alpha buffer when triple color buffering[fbiInit5[10:9]=2])
        column[8:0] = ((Y % 32) <<4) + ((X % 32)>>1)
        ramSelect[1] = ((X&0x20) ? 1 : 0) (for color buffers)
        ramSelect[1] = ((X&0x20) ? 0 : 1) (for depth/alpha buffers)
    }
    else
    {
        rowOffset = (!(Y&0x20)) ? (X>>6) : ((X>31) ? (((X-32)>>6)+1) : 0)
        row[9:0] = rowStart + rowOffset (for color buffer 0)
        row[9:0] = rowBase + rowStart + rowOffset (for color buffer 1)
        row[9:0] = (rowBase<<1) + rowStart + rowOffset (for depth/alpha buffer when double color buffering[fbiInit5[10:9]=0])
        row[9:0] = (rowBase<<1) + rowStart + rowOffset (for color buffer 2 when triple color buffering[fbiInit5[10:9]=1 or 2])
        row[9:0] = (rowBase<<1) + rowBase + rowStart + rowOffset (for depth/alpha buffer when triple color buffering[fbiInit5[10:9]=2])
        column[8:0] = ((Y % 32) <<4) + ((X % 32)>>1)
        ramSelect[1] = (((X&0x20)^(Y&0x20)) ? 1 : 0) (for color buffers)
        ramSelect[1] = (((X&0x20)^(Y&0x20)) ? 0 : 1) (for depth/alpha buffers)
    }
    ramSelect[0] = X % 2
    pixelMemoryAddress[21:0] = (row[9:0]<<12) + (column[8:0]<<3) + (ramSelect[1:0]<<1)
    bankSelect = pixelMemoryAddress[21]

**************************************************************************/

#ifndef EXPAND_RASTERIZERS
#define EXPAND_RASTERIZERS

#include "driver.h"
#include "profiler.h"
#include "voodoo.h"
#include "vooddefs.h"
#include <math.h>


/*************************************
 *
 *  Debugging
 *
 *************************************/

#define DEBUG_DEPTH			(0)
#define DEBUG_LOD			(0)

#define LOG_VBLANK_SWAP		(0)
#define LOG_FIFO			(0)
#define LOG_FIFO_VERBOSE	(0)
#define LOG_REGISTERS		(0)
#define LOG_LFB				(0)
#define LOG_TEXTURE_RAM		(0)
#define LOG_RASTERIZERS		(0)
#define LOG_CMDFIFO			(0)
#define LOG_CMDFIFO_VERBOSE	(0)

#define MODIFY_PIXEL(VV)



/*************************************
 *
 *  Statics
 *
 *************************************/

static voodoo_state *voodoo[MAX_VOODOO];

/* fast reciprocal+log2 lookup */
UINT32 reciplog[(2 << RECIPLOG_LOOKUP_BITS) + 2];



/*************************************
 *
 *  Prototypes
 *
 *************************************/

static void init_fbi(voodoo_state *v, fbi_state *f, void *memory, int fbmem);
static void init_tmu_shared(tmu_shared_state *s);
static void init_tmu(voodoo_state *v, tmu_state *t, int type, voodoo_reg *reg, void *memory, int tmem);
static void soft_reset(voodoo_state *v);
static void check_stalled_cpu(voodoo_state *v, mame_time current_time);
static void flush_fifos(voodoo_state *v, mame_time current_time);
static void stall_cpu_callback(void *param);
static void stall_cpu(voodoo_state *v, int state, mame_time current_time);
static void vblank_callback(void *param);
static INT32 register_w(voodoo_state *v, offs_t offset, UINT32 data);
static INT32 lfb_w(voodoo_state *v, offs_t offset, UINT32 data, UINT32 mem_mask, int forcefront);
static INT32 texture_w(voodoo_state *v, offs_t offset, UINT32 data);
static void voodoo_w(voodoo_state *v, offs_t offset, UINT32 data, UINT32 mem_mask);
static UINT32 voodoo_r(voodoo_state *v, offs_t offset);
static void banshee_w(voodoo_state *v, offs_t offset, UINT32 data, UINT32 mem_mask);
static UINT32 banshee_r(voodoo_state *v, offs_t offset, UINT32 mem_mask);
static void banshee_fb_w(voodoo_state *v, offs_t offset, UINT32 data, UINT32 mem_mask);
static UINT32 banshee_fb_r(voodoo_state *v, offs_t offset, UINT32 mem_mask);
static void banshee_io_w(voodoo_state *v, offs_t offset, UINT32 data, UINT32 mem_mask);
static UINT32 banshee_io_r(voodoo_state *v, offs_t offset, UINT32 mem_mask);
static UINT32 banshee_rom_r(voodoo_state *v, offs_t offset, UINT32 mem_mask);
static INT32 fastfill(voodoo_state *v);
static INT32 swapbuffer(voodoo_state *v, UINT32 data);
static INT32 begin_triangle(voodoo_state *v);
static INT32 draw_triangle(voodoo_state *v);
static INT32 triangle(voodoo_state *v);
static INT32 setup_and_draw_triangle(voodoo_state *v);
static raster_info *add_rasterizer(voodoo_state *v, const raster_info *cinfo);
static raster_info *find_rasterizer(voodoo_state *v, int texcount);
static void dump_rasterizer_stats(voodoo_state *v);

static void raster_generic_0tmu(voodoo_state *v, UINT16 *drawbuf);
static void raster_generic_1tmu(voodoo_state *v, UINT16 *drawbuf);
static void raster_generic_2tmu(voodoo_state *v, UINT16 *drawbuf);



/*************************************
 *
 *  Specific rasterizers
 *
 *************************************/

#define RASTERIZER_ENTRY(fbzcp, alpha, fog, fbz, tex0, tex1) \
	RASTERIZER(fbzcp##_##alpha##_##fog##_##fbz##_##tex0##_##tex1, (((tex0) == 0xffffffff) ? 0 : ((tex1) == 0xffffffff) ? 1 : 2), fbzcp, fbz, alpha, fog, tex0, tex1)

#include "voodoo.c"

#undef RASTERIZER_ENTRY



/*************************************
 *
 *  Rasterizer table
 *
 *************************************/

#define RASTERIZER_ENTRY(fbzcp, alpha, fog, fbz, tex0, tex1) \
	{ NULL, raster_##fbzcp##_##alpha##_##fog##_##fbz##_##tex0##_##tex1, FALSE, 0, 0, fbzcp, alpha, fog, fbz, tex0, tex1 },

static const raster_info predef_raster_table[] =
{
#include "voodoo.c"
	{ 0 }
};

#undef RASTERIZER_ENTRY



/*************************************
 *
 *  Main create routine
 *
 *************************************/

int voodoo_start(int which, int type, int fbmem_in_mb, int tmem0_in_mb, int tmem1_in_mb)
{
	void *fbmem, *tmumem[2];
	voodoo_state *v;
	int val;

	/* allocate memory */
	v = auto_malloc(sizeof(*v));
	memset(v, 0, sizeof(*v));
	voodoo[which] = v;

	/* create a table of precomputed 1/n and log2(n) values */
	/* n ranges from 1.0000 to 2.0000 */
	for (val = 0; val <= (1 << RECIPLOG_LOOKUP_BITS); val++)
	{
		UINT32 value = (1 << RECIPLOG_LOOKUP_BITS) + val;
		reciplog[val*2 + 0] = (1 << (RECIPLOG_LOOKUP_PREC + RECIPLOG_LOOKUP_BITS)) / value;
		reciplog[val*2 + 1] = (UINT32)(LOGB2((double)value / (double)(1 << RECIPLOG_LOOKUP_BITS)) * (double)(1 << RECIPLOG_LOOKUP_PREC));
	}

	/* configure type-specific values */
	switch (type)
	{
		case VOODOO_1:
			v->freq = 50000000;
			v->regaccess = voodoo_register_access;
			v->regnames = voodoo_reg_name;
			v->alt_regmap = 0;
			v->fbi.lfb_stride = 10;
			break;

		case VOODOO_2:
			v->freq = 90000000;
			v->regaccess = voodoo2_register_access;
			v->regnames = voodoo_reg_name;
			v->alt_regmap = 0;
			v->fbi.lfb_stride = 10;
			break;

		case VOODOO_BANSHEE:
			v->freq = 90000000;
			v->regaccess = banshee_register_access;
			v->regnames = banshee_reg_name;
			v->alt_regmap = 1;
			v->fbi.lfb_stride = 11;
			break;

		case VOODOO_3:
			v->freq = 132000000;
			v->regaccess = banshee_register_access;
			v->regnames = banshee_reg_name;
			v->alt_regmap = 1;
			v->fbi.lfb_stride = 11;
			break;

		default:
			fatalerror("Unsupported voodoo card in voodoo_start!");
			break;
	}

	/* set the type, and initialize the chip mask */
	v->index = which;
	v->type = type;
	v->chipmask = 0x01;
	v->subseconds_per_cycle = MAX_SUBSECONDS / v->freq;
	v->trigger = 51324 + which;

	/* build the rasterizer table */
#ifndef VOODOO_DRC
{
	const raster_info *info;
	for (info = predef_raster_table; info->callback; info++)
		add_rasterizer(v, info);
}
#endif

	/* set up the PCI FIFO */
	v->pci.fifo.base = v->pci.fifo_mem;
	v->pci.fifo.size = 64*2;
	v->pci.fifo.in = v->pci.fifo.out = 0;
	v->pci.stall_state = NOT_STALLED;
	v->pci.continue_timer = timer_alloc_ptr(stall_cpu_callback, v);

	/* allocate memory */
	if (type <= VOODOO_2)
	{
		/* separate FB/TMU memory */
		fbmem = auto_malloc(fbmem_in_mb << 20);
		tmumem[0] = auto_malloc(tmem0_in_mb << 20);
		tmumem[1] = tmem1_in_mb ? auto_malloc(tmem1_in_mb << 20) : NULL;
	}
	else
	{
		/* shared memory */
		tmumem[0] = tmumem[1] = fbmem = auto_malloc(fbmem_in_mb << 20);
	}

	/* set up frame buffer */
	init_fbi(v, &v->fbi, fbmem, fbmem_in_mb << 20);

	/* build shared TMU tables */
	init_tmu_shared(&v->tmushare);

	/* set up the TMUs */
	init_tmu(v, &v->tmu[0], type, &v->reg[0x100], tmumem[0], tmem0_in_mb << 20);
	v->chipmask |= 0x02;
	if (tmem1_in_mb)
	{
		init_tmu(v, &v->tmu[1], type, &v->reg[0x200], tmumem[1], tmem1_in_mb << 20);
		v->chipmask |= 0x04;
	}

	/* initialize some registers */
	memset(v->reg, 0, sizeof(v->reg));
	v->pci.init_enable = 0;
	v->reg[fbiInit0].u = (1 << 4) | (0x10 << 6);
	v->reg[fbiInit1].u = (1 << 1) | (1 << 8) | (1 << 12) | (2 << 20);
	v->reg[fbiInit2].u = (1 << 6) | (0x100 << 23);
	v->reg[fbiInit3].u = (2 << 13) | (0xf << 17);
	v->reg[fbiInit4].u = (1 << 0);

	/* initialize banshee registers */
	memset(v->banshee.io, 0, sizeof(v->banshee.io));
	v->banshee.io[io_pciInit0] = 0x01800040;
	v->banshee.io[io_sipMonitor] = 0x40000000;
	v->banshee.io[io_lfbMemoryConfig] = 0x000a2200;
	v->banshee.io[io_dramInit0] = 0x00579d29;
	v->banshee.io[io_dramInit1] = 0x00f02200;
	v->banshee.io[io_tmuGbeInit] = 0x00000bfb;

	/* do a soft reset to reset everything else */
	soft_reset(v);
	return 0;
}



/*************************************
 *
 *  Video update
 *
 *************************************/

void voodoo_update(int which, mame_bitmap *bitmap, const rectangle *cliprect)
{
	voodoo_state *v = voodoo[which];
	int drawbuf = v->fbi.frontbuf;
	int statskey;
	int x, y;

	/* if we are blank, just fill with black */
	if (v->type <= VOODOO_2 && FBIINIT1_SOFTWARE_BLANK(v->reg[fbiInit1].u))
	{
		fillbitmap(bitmap, 0, cliprect);
		return;
	}

	/* if the CLUT is dirty, recompute the pens array */
	if (v->fbi.clut_dirty)
	{
		UINT8 rtable[32], gtable[64], btable[32];

		/* Voodoo/Voodoo-2 have an internal 33-entry CLUT */
		if (v->type <= VOODOO_2)
		{
			/* kludge: some of the Midway games write 0 to the last entry when they obviously mean FF */
			if ((v->fbi.clut[32] & 0xffffff) == 0 && (v->fbi.clut[31] & 0xffffff) != 0)
				v->fbi.clut[32] = 0x20ffffff;

			/* compute the R/G/B pens first */
			for (x = 0; x < 32; x++)
			{
				/* treat X as a 5-bit value, scale up to 8 bits, and linear interpolate for red/blue */
				y = (x << 3) | (x >> 2);
				rtable[x] = (RGB_RED(v->fbi.clut[y >> 3]) * (8 - (y & 7)) + RGB_RED(v->fbi.clut[(y >> 3) + 1]) * (y & 7)) >> 3;
				btable[x] = (RGB_BLUE(v->fbi.clut[y >> 3]) * (8 - (y & 7)) + RGB_BLUE(v->fbi.clut[(y >> 3) + 1]) * (y & 7)) >> 3;

				/* treat X as a 6-bit value with LSB=0, scale up to 8 bits, and linear interpolate */
				y = (x * 2) + 0;
				y = (y << 2) | (y >> 4);
				gtable[x*2+0] = (RGB_GREEN(v->fbi.clut[y >> 3]) * (8 - (y & 7)) + RGB_GREEN(v->fbi.clut[(y >> 3) + 1]) * (y & 7)) >> 3;

				/* treat X as a 6-bit value with LSB=1, scale up to 8 bits, and linear interpolate */
				y = (x * 2) + 1;
				y = (y << 2) | (y >> 4);
				gtable[x*2+1] = (RGB_GREEN(v->fbi.clut[y >> 3]) * (8 - (y & 7)) + RGB_GREEN(v->fbi.clut[(y >> 3) + 1]) * (y & 7)) >> 3;
			}
		}

		/* Banshee and later have a 512-entry CLUT that can be bypassed */
		else
		{
			int which = (v->banshee.io[io_vidProcCfg] >> 13) & 1;
			int bypass = (v->banshee.io[io_vidProcCfg] >> 11) & 1;

			/* compute R/G/B pens first */
			for (x = 0; x < 32; x++)
			{
				/* treat X as a 5-bit value, scale up to 8 bits */
				y = (x << 3) | (x >> 2);
				rtable[x] = bypass ? y : RGB_RED(v->fbi.clut[which * 256 + y]);
				btable[x] = bypass ? y : RGB_BLUE(v->fbi.clut[which * 256 + y]);

				/* treat X as a 6-bit value with LSB=0, scale up to 8 bits */
				y = (x * 2) + 0;
				y = (y << 2) | (y >> 4);
				gtable[x*2+0] = bypass ? y : RGB_GREEN(v->fbi.clut[which * 256 + y]);

				/* treat X as a 6-bit value with LSB=1, scale up to 8 bits, and linear interpolate */
				y = (x * 2) + 1;
				y = (y << 2) | (y >> 4);
				gtable[x*2+1] = bypass ? y : RGB_GREEN(v->fbi.clut[which * 256 + y]);
			}
		}

		/* now compute the actual pens array */
		for (x = 0; x < 65536; x++)
		{
			int r = rtable[(x >> 11) & 0x1f];
			int g = gtable[(x >> 5) & 0x3f];
			int b = btable[x & 0x1f];
			v->fbi.pen[x] = MAKE_RGB(r, g, b);
		}

		/* no longer dirty */
		v->fbi.clut_dirty = FALSE;
	}

	/* debugging! */
	if (code_pressed(KEYCODE_L))
		drawbuf = v->fbi.backbuf;

	/* copy from the current front buffer */
	for (y = cliprect->min_y; y <= cliprect->max_y; y++)
	{
		UINT16 *src = v->fbi.rgb[drawbuf] + y * v->fbi.rowpixels;
		UINT32 *dst = (UINT32 *)bitmap->line[y];
		for (x = cliprect->min_x; x <= cliprect->max_x; x++)
			dst[x] = v->fbi.pen[src[x]];
	}

	/* update stats display */
	statskey = (code_pressed(KEYCODE_BACKSLASH) != 0);
	if (statskey && statskey != v->stats.lastkey)
		v->stats.display = !v->stats.display;
	v->stats.lastkey = statskey;

	/* display stats */
	if (v->stats.display)
		ui_draw_text(v->stats.buffer, 0, 0);

	/* update render override */
	v->stats.render_override = code_pressed(KEYCODE_ENTER);
	if (DEBUG_DEPTH && v->stats.render_override)
	{
		for (y = cliprect->min_y; y <= cliprect->max_y; y++)
		{
			UINT16 *src = v->fbi.aux + y * v->fbi.rowpixels;
			UINT32 *dst = (UINT32 *)bitmap->line[y];
			for (x = cliprect->min_x; x <= cliprect->max_x; x++)
				dst[x] = ((src[x] << 8) & 0xff0000) | ((src[x] >> 0) & 0xff00) | ((src[x] >> 8) & 0xff);
		}
	}
}



/*************************************
 *
 *  Chip reset
 *
 *************************************/

void voodoo_reset(int which)
{
	soft_reset(voodoo[which]);
}


int voodoo_get_type(int which)
{
	return voodoo[which]->type;
}


int voodoo_is_stalled(int which)
{
	return (voodoo[which]->pci.stall_state != NOT_STALLED);
}


void voodoo_set_init_enable(int which, UINT32 newval)
{
	voodoo[which]->pci.init_enable = newval;
	if (LOG_REGISTERS)
		logerror("VOODOO.%d.REG:initEnable write = %08X\n", which, newval);
}


void voodoo_set_vblank_callback(int which, void (*vblank)(int))
{
	voodoo[which]->fbi.vblank_client = vblank;
}


void voodoo_set_stall_callback(int which, void (*stall)(int))
{
	voodoo[which]->pci.stall_callback = stall;
}



/*************************************
 *
 *  Specific write handlers
 *
 *************************************/

WRITE32_HANDLER( voodoo_0_w )
{
	voodoo_w(voodoo[0], offset, data, mem_mask);
}


WRITE32_HANDLER( voodoo_1_w )
{
	voodoo_w(voodoo[1], offset, data, mem_mask);
}



/*************************************
 *
 *  Specific read handlers
 *
 *************************************/

READ32_HANDLER( voodoo_0_r )
{
	return voodoo_r(voodoo[0], offset);
}


READ32_HANDLER( voodoo_1_r )
{
	return voodoo_r(voodoo[1], offset);
}



/*************************************
 *
 *  Specific Banshee write handlers
 *
 *************************************/

WRITE32_HANDLER( banshee_0_w )
{
	banshee_w(voodoo[0], offset, data, mem_mask);
}


WRITE32_HANDLER( banshee_fb_0_w )
{
	banshee_fb_w(voodoo[0], offset, data, mem_mask);
}


WRITE32_HANDLER( banshee_io_0_w )
{
	banshee_io_w(voodoo[0], offset, data, mem_mask);
}


WRITE32_HANDLER( banshee_1_w )
{
	banshee_w(voodoo[1], offset, data, mem_mask);
}


WRITE32_HANDLER( banshee_fb_1_w )
{
	banshee_fb_w(voodoo[1], offset, data, mem_mask);
}


WRITE32_HANDLER( banshee_io_1_w )
{
	banshee_io_w(voodoo[1], offset, data, mem_mask);
}




/*************************************
 *
 *  Specific Banshee read handlers
 *
 *************************************/

READ32_HANDLER( banshee_0_r )
{
	return banshee_r(voodoo[0], offset, mem_mask);
}


READ32_HANDLER( banshee_fb_0_r )
{
	return banshee_fb_r(voodoo[0], offset, mem_mask);
}


READ32_HANDLER( banshee_io_0_r )
{
	return banshee_io_r(voodoo[0], offset, mem_mask);
}


READ32_HANDLER( banshee_rom_0_r )
{
	return banshee_rom_r(voodoo[0], offset, mem_mask);
}


READ32_HANDLER( banshee_1_r )
{
	return banshee_r(voodoo[1], offset, mem_mask);
}


READ32_HANDLER( banshee_fb_1_r )
{
	return banshee_fb_r(voodoo[1], offset, mem_mask);
}


READ32_HANDLER( banshee_io_1_r )
{
	return banshee_io_r(voodoo[1], offset, mem_mask);
}


READ32_HANDLER( banshee_rom_1_r )
{
	return banshee_rom_r(voodoo[1], offset, mem_mask);
}



/*************************************
 *
 *  Common initialization
 *
 *************************************/

static void init_fbi(voodoo_state *v, fbi_state *f, void *memory, int fbmem)
{
	int pen;

	/* allocate frame buffer RAM and set pointers */
	f->ram = memory;
	f->mask = fbmem - 1;
	f->rgb[0] = f->rgb[1] = f->rgb[2] = f->ram;
	f->aux = f->ram;

	/* default to 0x0 */
	f->frontbuf = 0;
	f->backbuf = 1;
	f->width = 512;
	f->height = 384;

	/* init the pens */
	for (pen = 0; pen < 65536; pen++)
	{
		int r = (pen >> 11) & 0x1f;
		int g = (pen >> 5) & 0x3f;
		int b = pen & 0x1f;
		r = (r << 3) | (r >> 2);
		g = (g << 2) | (g >> 4);
		b = (b << 3) | (b >> 2);
		f->pen[pen] = MAKE_RGB(r, g, b);
	}

	/* allocate a VBLANK timer */
	f->vblank_timer = timer_alloc_ptr(vblank_callback, v);
	f->vblank = FALSE;

	/* initialize the memory FIFO */
	f->fifo.base = f->ram;
	f->fifo.size = f->fifo.in = f->fifo.out = 0;

	/* set the fog delta mask */
	f->fogdelta_mask = (v->type < VOODOO_2) ? 0xff : 0xfc;
}


static void init_tmu_shared(tmu_shared_state *s)
{
	int val;

	/* build static 8-bit texel tables */
	for (val = 0; val < 256; val++)
	{
		int r, g, b, a;

		/* 8-bit RGB (3-3-2) */
		EXTRACT_332_TO_888(val, r, g, b);
		s->rgb332[val] = MAKE_ARGB(0xff, r, g, b);

		/* 8-bit alpha */
		s->alpha8[val] = MAKE_ARGB(val, val, val, val);

		/* 8-bit intensity */
		s->int8[val] = MAKE_ARGB(0xff, val, val, val);

		/* 8-bit alpha, intensity */
		a = ((val >> 0) & 0xf0) | ((val >> 4) & 0x0f);
		r = ((val << 4) & 0xf0) | ((val << 0) & 0x0f);
		s->ai44[val] = MAKE_ARGB(a, r, r, r);
	}

	/* build static 16-bit texel tables */
	for (val = 0; val < 65536; val++)
	{
		int r, g, b, a;

		/* table 10 = 16-bit RGB (5-6-5) */
		EXTRACT_565_TO_888(val, r, g, b);
		s->rgb565[val] = MAKE_ARGB(0xff, r, g, b);

		/* table 11 = 16 ARGB (1-5-5-5) */
		EXTRACT_1555_TO_8888(val, a, r, g, b);
		s->argb1555[val] = MAKE_ARGB(a, r, g, b);

		/* table 12 = 16-bit ARGB (4-4-4-4) */
		EXTRACT_4444_TO_8888(val, a, r, g, b);
		s->argb4444[val] = MAKE_ARGB(a, r, g, b);
	}
}


static void init_tmu(voodoo_state *v, tmu_state *t, int type, voodoo_reg *reg, void *memory, int tmem)
{
	/* allocate texture RAM */
	t->ram = memory;
	t->mask = tmem - 1;
	t->reg = reg;
	t->regdirty = TRUE;
	t->bilinear_mask = (type >= VOODOO_2) ? 0xff : 0xf0;

	/* mark the NCC tables dirty and configure their registers */
	t->ncc[0].dirty = t->ncc[1].dirty = TRUE;
	t->ncc[0].reg = &t->reg[nccTable+0];
	t->ncc[1].reg = &t->reg[nccTable+12];

	/* create pointers to all the tables */
	t->texel[0] = v->tmushare.rgb332;
	t->texel[1] = t->ncc[0].texel;
	t->texel[2] = v->tmushare.alpha8;
	t->texel[3] = v->tmushare.int8;
	t->texel[4] = v->tmushare.ai44;
	t->texel[5] = t->palette;
	t->texel[6] = (type >= VOODOO_2) ? t->palettea : NULL;
	t->texel[7] = NULL;
	t->texel[8] = v->tmushare.rgb332;
	t->texel[9] = t->ncc[0].texel;
	t->texel[10] = v->tmushare.rgb565;
	t->texel[11] = v->tmushare.argb1555;
	t->texel[12] = v->tmushare.argb4444;
	t->texel[13] = v->tmushare.int8;
	t->texel[14] = t->palette;
	t->texel[15] = NULL;
	t->lookup = t->texel[0];

	/* attach the palette to NCC table 0 */
	t->ncc[0].palette = t->palette;
	if (type >= VOODOO_2)
		t->ncc[0].palettea = t->palettea;

	/* set up texture address calculations */
	if (type <= VOODOO_2)
	{
		t->texaddr_mask = 0x0fffff;
		t->texaddr_shift = 3;
	}
	else
	{
		t->texaddr_mask = 0xfffff0;
		t->texaddr_shift = 0;
	}
}



/*************************************
 *
 *  VBLANK management
 *
 *************************************/

static void swap_buffers(voodoo_state *v)
{
	int count;

	if (LOG_VBLANK_SWAP) logerror("--- swap_buffers @ %d\n", cpu_getscanline());

	/* force a partial update */
	force_partial_update(cpu_getscanline());

	/* keep a history of swap intervals */
	count = v->fbi.vblank_count;
	if (count > 15)
		count = 15;
	v->reg[fbiSwapHistory].u = (v->reg[fbiSwapHistory].u << 4) | count;

	/* rotate the buffers */
	if (v->type <= VOODOO_2)
	{
		if (v->type < VOODOO_2 || !v->fbi.vblank_dont_swap)
		{
			if (v->fbi.rgb[2] == NULL)
			{
				v->fbi.frontbuf = 1 - v->fbi.frontbuf;
				v->fbi.backbuf = 1 - v->fbi.frontbuf;
			}
			else
			{
				v->fbi.frontbuf = (v->fbi.frontbuf + 1) % 3;
				v->fbi.backbuf = (v->fbi.frontbuf + 1) % 3;
			}
		}
	}
	else
	{
		v->fbi.rgb[0] = (UINT16 *)((UINT8 *)v->fbi.ram + (v->reg[leftOverlayBuf].u & v->fbi.mask & ~0x0f));
		v->fbi.rgbmax[0] = (UINT16 *)((UINT8 *)v->fbi.ram + v->fbi.mask + 1) - v->fbi.rgb[0];
	}

	/* decrement the pending count and reset our state */
	if (v->fbi.swaps_pending)
		v->fbi.swaps_pending--;
	v->fbi.vblank_count = 0;
	v->fbi.vblank_swap_pending = FALSE;

	/* reset the last_op_time to now and start processing the next command */
	if (v->pci.op_pending)
	{
		v->pci.op_end_time = mame_timer_get_time();
		flush_fifos(v, v->pci.op_end_time);
	}

	/* we may be able to unstall now */
	if (v->pci.stall_state != NOT_STALLED)
		check_stalled_cpu(v, mame_timer_get_time());

	/* periodically log rasterizer info */
	v->stats.swaps++;
	if (LOG_RASTERIZERS && v->stats.swaps % 100 == 0)
		dump_rasterizer_stats(v);

	/* update the statistics (debug) */
	if (v->stats.display)
	{
		int screen_area = (Machine->visible_area.max_x - Machine->visible_area.min_x + 1) * (Machine->visible_area.max_y - Machine->visible_area.min_y + 1);
		int pixelcount = v->stats.total_pixels_out;
		char *statsptr = v->stats.buffer;
		int i;

		statsptr += sprintf(statsptr, "Swap:%6d\n", v->stats.swaps);
		statsptr += sprintf(statsptr, "Hist:%08X\n", v->reg[fbiSwapHistory].u);
		statsptr += sprintf(statsptr, "Stal:%6d\n", v->stats.stalls);
		statsptr += sprintf(statsptr, "Rend:%6d%%\n", pixelcount * 100 / screen_area);
		statsptr += sprintf(statsptr, "Poly:%6d\n", v->stats.total_triangles);
		statsptr += sprintf(statsptr, "PxIn:%6d\n", v->stats.total_pixels_in);
		statsptr += sprintf(statsptr, "POut:%6d\n", v->stats.total_pixels_out);
		statsptr += sprintf(statsptr, "Clip:%6d\n", v->stats.total_clipped);
		statsptr += sprintf(statsptr, "Stip:%6d\n", v->stats.total_stippled);
		statsptr += sprintf(statsptr, "Chro:%6d\n", v->stats.total_chroma_fail);
		statsptr += sprintf(statsptr, "ZFun:%6d\n", v->stats.total_zfunc_fail);
		statsptr += sprintf(statsptr, "AFun:%6d\n", v->stats.total_afunc_fail);
		statsptr += sprintf(statsptr, "RegW:%6d\n", v->stats.reg_writes);
		statsptr += sprintf(statsptr, "RegR:%6d\n", v->stats.reg_reads);
		statsptr += sprintf(statsptr, "LFBW:%6d\n", v->stats.lfb_writes);
		statsptr += sprintf(statsptr, "LFBR:%6d\n", v->stats.lfb_reads);
		statsptr += sprintf(statsptr, "TexW:%6d\n", v->stats.tex_writes);
		statsptr += sprintf(statsptr, "TexM:");
		for (i = 0; i < 16; i++)
			if (v->stats.texture_mode[i])
				*statsptr++ = "0123456789ABCDEF"[i];
		*statsptr = 0;
	}

	/* update statistics */
	v->stats.stalls = 0;
	v->stats.total_triangles = 0;
	v->stats.total_pixels_in = 0;
	v->stats.total_pixels_out = 0;
	v->stats.total_chroma_fail = 0;
	v->stats.total_zfunc_fail = 0;
	v->stats.total_afunc_fail = 0;
	v->stats.total_clipped = 0;
	v->stats.total_stippled = 0;
	v->stats.reg_writes = 0;
	v->stats.reg_reads = 0;
	v->stats.lfb_writes = 0;
	v->stats.lfb_reads = 0;
	v->stats.tex_writes = 0;
	memset(v->stats.texture_mode, 0, sizeof(v->stats.texture_mode));
}


static void vblank_off_callback(void *param)
{
	voodoo_state *v = param;

	if (LOG_VBLANK_SWAP) logerror("--- vblank end\n");

	/* set internal state and call the client */
	v->fbi.vblank = FALSE;
	if (v->fbi.vblank_client)
		(*v->fbi.vblank_client)(FALSE);

	/* go to the end of the next frame */
	timer_adjust_ptr(v->fbi.vblank_timer, cpu_getscanlinetime(Machine->visible_area.max_y + 1), TIME_NEVER);
}


static void vblank_callback(void *param)
{
	voodoo_state *v = param;

	if (LOG_VBLANK_SWAP) logerror("--- vblank start\n");

	/* flush the pipes */
	if (v->pci.op_pending)
	{
		if (LOG_VBLANK_SWAP) logerror("---- vblank flush begin\n");
		flush_fifos(v, mame_timer_get_time());
		if (LOG_VBLANK_SWAP) logerror("---- vblank flush end\n");
	}

	/* increment the count */
	v->fbi.vblank_count++;
	if (v->fbi.vblank_count > 250)
		v->fbi.vblank_count = 250;
	if (LOG_VBLANK_SWAP) logerror("---- vblank count = %d", v->fbi.vblank_count);
	if (v->fbi.vblank_swap_pending)
		if (LOG_VBLANK_SWAP) logerror(" (target=%d)", v->fbi.vblank_swap);
	if (LOG_VBLANK_SWAP) logerror("\n");

	/* if we're past the swap count, do the swap */
	if (v->fbi.vblank_swap_pending && v->fbi.vblank_count >= v->fbi.vblank_swap)
		swap_buffers(v);

	/* set a timer for the next off state */
	timer_set_ptr(cpu_getscanlinetime(0), v, vblank_off_callback);

	/* set internal state and call the client */
	v->fbi.vblank = TRUE;
	if (v->fbi.vblank_client)
		(*v->fbi.vblank_client)(TRUE);
}



/*************************************
 *
 *  Chip reset
 *
 *************************************/

static void reset_counters(voodoo_state *v)
{
	v->reg[fbiPixelsIn].u = 0;
	v->reg[fbiChromaFail].u = 0;
	v->reg[fbiZfuncFail].u = 0;
	v->reg[fbiAfuncFail].u = 0;
	v->reg[fbiPixelsOut].u = 0;
}


static void soft_reset(voodoo_state *v)
{
	reset_counters(v);
	v->reg[fbiTrianglesOut].u = 0;
	fifo_reset(&v->fbi.fifo);
	fifo_reset(&v->pci.fifo);
}



/*************************************
 *
 *  Recompute video memory layout
 *
 *************************************/

static void recompute_video_memory(voodoo_state *v)
{
	UINT32 buffer_pages = FBIINIT2_VIDEO_BUFFER_OFFSET(v->reg[fbiInit2].u);
	UINT32 fifo_start_page = FBIINIT4_MEMORY_FIFO_START_ROW(v->reg[fbiInit4].u);
	UINT32 fifo_last_page = FBIINIT4_MEMORY_FIFO_STOP_ROW(v->reg[fbiInit4].u);
	UINT32 memory_config;
	int buf;

	/* memory config is determined differently between V1 and V2 */
	memory_config = FBIINIT2_ENABLE_TRIPLE_BUF(v->reg[fbiInit2].u);
	if (v->type == VOODOO_2 && memory_config == 0)
		memory_config = FBIINIT5_BUFFER_ALLOCATION(v->reg[fbiInit5].u);

	/* tiles are 64x16/32; x_tiles specifies how many half-tiles */
	v->fbi.tile_width = (v->type == VOODOO_1) ? 64 : 32;
	v->fbi.tile_height = (v->type == VOODOO_1) ? 16 : 32;
	v->fbi.x_tiles = FBIINIT1_X_VIDEO_TILES(v->reg[fbiInit1].u);
	if (v->type == VOODOO_2)
	{
		v->fbi.x_tiles = (v->fbi.x_tiles << 1) |
						(FBIINIT1_X_VIDEO_TILES_BIT5(v->reg[fbiInit1].u) << 5) |
						(FBIINIT6_X_VIDEO_TILES_BIT0(v->reg[fbiInit6].u));
	}
	v->fbi.rowpixels = v->fbi.tile_width * v->fbi.x_tiles;

//  logerror("VOODOO.%d.VIDMEM: buffer_pages=%X  fifo=%X-%X  tiles=%X  rowpix=%d\n", v->index, buffer_pages, fifo_start_page, fifo_last_page, v->fbi.x_tiles, v->fbi.rowpixels);

	/* first RGB buffer always starts at 0 */
	v->fbi.rgb[0] = v->fbi.ram;

	/* second RGB buffer starts immediately afterwards */
	v->fbi.rgb[1] = (UINT16 *)((UINT8 *)v->fbi.ram + buffer_pages * 0x1000);

	/* remaining buffers are based on the config */
	switch (memory_config)
	{
		case 3:	/* reserved */
			logerror("VOODOO.%d.ERROR:Unexpected memory configuration in recompute_video_memory!\n", v->index);

		case 0:	/* 2 color buffers, 1 aux buffer */
			v->fbi.rgb[2] = NULL;
			v->fbi.aux = (UINT16 *)((UINT8 *)v->fbi.ram + 2 * buffer_pages * 0x1000);
			break;

		case 1:	/* 3 color buffers, 0 aux buffers */
			v->fbi.rgb[2] = (UINT16 *)((UINT8 *)v->fbi.ram + 2 * buffer_pages * 0x1000);
			v->fbi.aux = NULL;
			break;

		case 2:	/* 3 color buffers, 1 aux buffers */
			v->fbi.rgb[2] = (UINT16 *)((UINT8 *)v->fbi.ram + 2 * buffer_pages * 0x1000);
			v->fbi.aux = (UINT16 *)((UINT8 *)v->fbi.ram + 3 * buffer_pages * 0x1000);
			break;
	}

	/* clamp the RGB buffers to video memory */
	for (buf = 0; buf < 3; buf++)
	{
		v->fbi.rgbmax[buf] = 0;
		if (v->fbi.rgb[buf])
		{
			if ((UINT8 *)v->fbi.rgb[buf] > (UINT8 *)v->fbi.ram + v->fbi.mask)
				v->fbi.rgb[buf] = (UINT16 *)((UINT8 *)v->fbi.ram + v->fbi.mask);
			v->fbi.rgbmax[buf] = (UINT16 *)((UINT8 *)v->fbi.ram + v->fbi.mask + 1) - v->fbi.rgb[buf];
		}
	}

	/* clamp the aux buffer to video memory */
	v->fbi.auxmax = 0;
	if (v->fbi.aux)
	{
		if ((UINT8 *)v->fbi.aux > (UINT8 *)v->fbi.ram + v->fbi.mask)
			v->fbi.aux = (UINT16 *)((UINT8 *)v->fbi.ram + v->fbi.mask);
		v->fbi.auxmax = (UINT16 *)((UINT8 *)v->fbi.ram + v->fbi.mask + 1) - v->fbi.aux;
	}

/*  printf("rgb[0] = %08X   rgb[1] = %08X   rgb[2] = %08X   aux = %08X\n",
            (UINT8 *)v->fbi.rgb[0] - (UINT8 *)v->fbi.ram,
            (UINT8 *)v->fbi.rgb[1] - (UINT8 *)v->fbi.ram,
            v->fbi.rgb[2] ? ((UINT8 *)v->fbi.rgb[2] - (UINT8 *)v->fbi.ram) : 0,
            (UINT8 *)v->fbi.aux - (UINT8 *)v->fbi.ram);*/
//  logerror("VOODOO.%d.VIDMEM: rgbmax=%X,%X,%X  auxmax=%X\n", v->index, v->fbi.rgbmax[0], v->fbi.rgbmax[1], v->fbi.rgbmax[2], v->fbi.auxmax);

	/* compute the memory FIFO location and size */
	if (fifo_last_page > v->fbi.mask / 0x1000)
		fifo_last_page = v->fbi.mask / 0x1000;

	/* is it valid and enabled? */
	if (fifo_start_page <= fifo_last_page && FBIINIT0_ENABLE_MEMORY_FIFO(v->reg[fbiInit0].u))
	{
		v->fbi.fifo.base = (UINT32 *)((UINT8 *)v->fbi.ram + fifo_start_page * 0x1000);
		v->fbi.fifo.size = (fifo_last_page + 1 - fifo_start_page) * 0x1000 / 4;
		if (v->fbi.fifo.size > 65536*2)
			v->fbi.fifo.size = 65536*2;
	}

	/* if not, disable the FIFO */
	else
	{
		v->fbi.fifo.base = NULL;
		v->fbi.fifo.size = 0;
	}

	/* reset the FIFO */
	fifo_reset(&v->fbi.fifo);

	/* reset our front/back buffers if they are out of range */
	if (!v->fbi.rgb[2])
	{
		if (v->fbi.frontbuf == 2)
			v->fbi.frontbuf = 0;
		if (v->fbi.backbuf == 2)
			v->fbi.backbuf = 0;
	}
}



/*************************************
 *
 *  NCC table management
 *
 *************************************/

static void ncc_table_write(ncc_table *n, offs_t regnum, UINT32 data)
{
	/* I/Q entries reference the plaette if the high bit is set */
	if (regnum >= 4 && (data & 0x80000000) && n->palette)
	{
		int index = ((data >> 23) & 0xfe) | (regnum & 1);

		/* set the ARGB for this palette index */
		n->palette[index] = 0xff000000 | data;

		/* if we have an ARGB palette as well, compute its value */
		if (n->palettea)
		{
			int a = ((data >> 16) & 0xfc) | ((data >> 22) & 0x03);
			int r = ((data >> 10) & 0xfc) | ((data >> 16) & 0x03);
			int g = ((data >>  4) & 0xfc) | ((data >> 10) & 0x03);
			int b = ((data <<  2) & 0xfc) | ((data >>  4) & 0x03);
			n->palettea[index] = MAKE_ARGB(a, r, g, b);
		}

		/* this doesn't dirty the table or go to the registers, so bail */
		return;
	}

	/* if the register matches, don't update */
	if (data == n->reg[regnum].u)
		return;
	n->reg[regnum].u = data;

	/* first four entries are packed Y values */
	if (regnum < 4)
	{
		regnum *= 4;
		n->y[regnum+0] = (data >>  0) & 0xff;
		n->y[regnum+1] = (data >>  8) & 0xff;
		n->y[regnum+2] = (data >> 16) & 0xff;
		n->y[regnum+3] = (data >> 24) & 0xff;
	}

	/* the second four entries are the I RGB values */
	else if (regnum < 8)
	{
		regnum &= 3;
		n->ir[regnum] = (INT32)(data <<  5) >> 23;
		n->ig[regnum] = (INT32)(data << 14) >> 23;
		n->ib[regnum] = (INT32)(data << 23) >> 23;
	}

	/* the final four entries are the Q RGB values */
	else
	{
		regnum &= 3;
		n->qr[regnum] = (INT32)(data <<  5) >> 23;
		n->qg[regnum] = (INT32)(data << 14) >> 23;
		n->qb[regnum] = (INT32)(data << 23) >> 23;
	}

	/* mark the table dirty */
	n->dirty = TRUE;
}


static void ncc_table_update(ncc_table *n)
{
	int r, g, b, i;

	/* generte all 256 possibilities */
	for (i = 0; i < 256; i++)
	{
		int vi = (i >> 2) & 0x03;
		int vq = (i >> 0) & 0x03;

		/* start with the intensity */
		r = g = b = n->y[(i >> 4) & 0x0f];

		/* add the coloring */
		r += n->ir[vi] + n->qr[vq];
		g += n->ig[vi] + n->qg[vq];
		b += n->ib[vi] + n->qb[vq];

		/* clamp */
		CLAMP(r, 0, 255);
		CLAMP(g, 0, 255);
		CLAMP(b, 0, 255);

		/* fill in the table */
		n->texel[i] = MAKE_ARGB(0xff, r, g, b);
	}

	/* no longer dirty */
	n->dirty = FALSE;
}



/*************************************
 *
 *  Faux DAC implementation
 *
 *************************************/

static void dacdata_w(dac_state *d, UINT8 regnum, UINT8 data)
{
	d->reg[regnum] = data;
}


static void dacdata_r(dac_state *d, UINT8 regnum)
{
	UINT8 result = 0xff;

	/* switch off the DAC register requested */
	switch (regnum)
	{
		case 5:
			/* this is just to make startup happy */
			switch (d->reg[7])
			{
				case 0x01:	result = 0x55; break;
				case 0x07:	result = 0x71; break;
				case 0x0b:	result = 0x79; break;
			}
			break;

		default:
			result = d->reg[regnum];
			break;
	}

	/* remember the read result; it is fetched elsewhere */
	d->read_result = result;
}



/*************************************
 *
 *  Texuture parameter computation
 *
 *************************************/

static void recompute_texture_params(tmu_state *t)
{
	int bppscale;
	UINT32 base;
	int lod;

	/* extract LOD parameters */
	t->lodmin = TEXLOD_LODMIN(t->reg[tLOD].u) << 6;
	t->lodmax = TEXLOD_LODMAX(t->reg[tLOD].u) << 6;
	t->lodbias = (INT8)(TEXLOD_LODBIAS(t->reg[tLOD].u) << 2) << 4;

	/* determine which LODs are present */
	t->lodmask = 0x1ff;
	if (TEXLOD_LOD_TSPLIT(t->reg[tLOD].u))
	{
		if (!TEXLOD_LOD_ODD(t->reg[tLOD].u))
			t->lodmask = 0x155;
		else
			t->lodmask = 0x0aa;
	}

	/* determine base texture width/height */
	t->wmask = t->hmask = 0xff;
	if (TEXLOD_LOD_S_IS_WIDER(t->reg[tLOD].u))
		t->hmask >>= TEXLOD_LOD_ASPECT(t->reg[tLOD].u);
	else
		t->wmask >>= TEXLOD_LOD_ASPECT(t->reg[tLOD].u);

	/* determine the bpp of the texture */
	bppscale = TEXMODE_FORMAT(t->reg[textureMode].u) >> 3;

	/* start with the base of LOD 0 */
	if (t->texaddr_shift == 0 && (t->reg[texBaseAddr].u & 1))
		printf("Tiled texture\n");
	base = (t->reg[texBaseAddr].u & t->texaddr_mask) << t->texaddr_shift;
	t->lodoffset[0] = base & t->mask;

	/* LODs 1-3 are different depending on whether we are in multitex mode */
	/* Several Voodoo 2 games leave the upper bits of TLOD == 0xff, meaning we think */
	/* they want multitex mode when they really don't -- disable for now */
	if (0)//TEXLOD_TMULTIBASEADDR(t->reg[tLOD].u))
	{
		base = (t->reg[texBaseAddr_1].u & t->texaddr_mask) << t->texaddr_shift;
		t->lodoffset[1] = base & t->mask;
		base = (t->reg[texBaseAddr_2].u & t->texaddr_mask) << t->texaddr_shift;
		t->lodoffset[2] = base & t->mask;
		base = (t->reg[texBaseAddr_3_8].u & t->texaddr_mask) << t->texaddr_shift;
		t->lodoffset[3] = base & t->mask;
	}
	else
	{
		if (t->lodmask & (1 << 0))
			base += (((t->wmask >> 0) + 1) * ((t->hmask >> 0) + 1)) << bppscale;
		t->lodoffset[1] = base & t->mask;
		if (t->lodmask & (1 << 1))
			base += (((t->wmask >> 1) + 1) * ((t->hmask >> 1) + 1)) << bppscale;
		t->lodoffset[2] = base & t->mask;
		if (t->lodmask & (1 << 2))
			base += (((t->wmask >> 2) + 1) * ((t->hmask >> 2) + 1)) << bppscale;
		t->lodoffset[3] = base & t->mask;
	}

	/* remaining LODs make sense */
	for (lod = 4; lod <= 8; lod++)
	{
		if (t->lodmask & (1 << (lod - 1)))
		{
			UINT32 size = ((t->wmask >> (lod - 1)) + 1) * ((t->hmask >> (lod - 1)) + 1);
			if (size < 4) size = 4;
			base += size << bppscale;
		}
		t->lodoffset[lod] = base & t->mask;
	}

	/* set the NCC lookup appropriately */
	t->texel[1] = t->texel[9] = t->ncc[TEXMODE_NCC_TABLE_SELECT(t->reg[textureMode].u)].texel;

	/* pick the lookup table */
	t->lookup = t->texel[TEXMODE_FORMAT(t->reg[textureMode].u)];

	/* compute the detail parameters */
	t->detailmax = TEXDETAIL_DETAIL_MAX(t->reg[tDetail].u);
	t->detailbias = (INT8)(TEXDETAIL_DETAIL_BIAS(t->reg[tDetail].u) << 2) << 6;
	t->detailscale = TEXDETAIL_DETAIL_SCALE(t->reg[tDetail].u);

	/* no longer dirty */
	t->regdirty = FALSE;

	/* check for separate RGBA filtering */
	if (TEXDETAIL_SEPARATE_RGBA_FILTER(t->reg[tDetail].u))
		fatalerror("Separate RGBA filters!");
}


INLINE void prepare_tmu(tmu_state *t)
{
	INT64 texdx, texdy;

	/* if the texture parameters are dirty, update them */
	if (t->regdirty)
		recompute_texture_params(t);

	/* ensure that the NCC tables are up to date */
	if ((TEXMODE_FORMAT(t->reg[textureMode].u) & 7) == 1)
	{
		ncc_table *n = &t->ncc[TEXMODE_NCC_TABLE_SELECT(t->reg[textureMode].u)];
		t->texel[1] = t->texel[9] = n->texel;
		if (n->dirty)
			ncc_table_update(n);
	}

	/* compute (ds^2 + dt^2) in both X and Y as 28.36 numbers */
	texdx = (INT64)(t->dsdx >> 14) * (INT64)(t->dsdx >> 14) + (INT64)(t->dtdx >> 14) * (INT64)(t->dtdx >> 14);
	texdy = (INT64)(t->dsdy >> 14) * (INT64)(t->dsdy >> 14) + (INT64)(t->dtdy >> 14) * (INT64)(t->dtdy >> 14);

	/* pick whichever is larger and shift off some high bits -> 28.20 */
	if (texdx < texdy)
		texdx = texdy;
	texdx >>= 16;

	/* use our fast reciprocal/log on this value; it expects input as a */
	/* 16.32 number, and returns the log of the reciprocal, so we have to */
	/* adjust the result: negative to get the log of the original value */
	/* plus 12 to account for the extra exponent, and divided by 2 to */
	/* get the log of the square root of texdx */
	(void)fast_reciplog(texdx, &t->lodbase);
	t->lodbase = (-t->lodbase + (12 << 8)) / 2;
}



/*************************************
 *
 *  Command FIFO depth computation
 *
 *************************************/

static int cmdfifo_compute_expected_depth(voodoo_state *v, cmdfifo_info *f)
{
	UINT32 *fifobase = v->fbi.ram;
	UINT32 readptr = f->rdptr;
	UINT32 command = fifobase[readptr / 4];
	int i, count = 0;

	/* low 3 bits specify the packet type */
	switch (command & 7)
	{
		/*
            Packet type 0: 1 or 2 words

              Word  Bits
                0  31:29 = reserved
                0  28:6  = Address [24:2]
                0   5:3  = Function (0 = NOP, 1 = JSR, 2 = RET, 3 = JMP LOCAL, 4 = JMP AGP)
                0   2:0  = Packet type (0)
                1  31:11 = reserved (JMP AGP only)
                1  10:0  = Address [35:25]
        */
		case 0:
			if (((command >> 3) & 7) == 4)
				return 2;
			return 1;

		/*
            Packet type 1: 1 + N words

              Word  Bits
                0  31:16 = Number of words
                0    15  = Increment?
                0  14:3  = Register base
                0   2:0  = Packet type (1)
                1  31:0  = Data word
        */
		case 1:
			return 1 + (command >> 16);

		/*
            Packet type 2: 1 + N words

              Word  Bits
                0  31:3  = 2D Register mask
                0   2:0  = Packet type (2)
                1  31:0  = Data word
        */
		case 2:
			for (i = 3; i <= 31; i++)
				if (command & (1 << i)) count++;
			return 1 + count;

		/*
            Packet type 3: 1 + N words

              Word  Bits
                0  31:29 = Number of dummy entries following the data
                0   28   = Packed color data?
                0   25   = Disable ping pong sign correction (0=normal, 1=disable)
                0   24   = Culling sign (0=positive, 1=negative)
                0   23   = Enable culling (0=disable, 1=enable)
                0   22   = Strip mode (0=strip, 1=fan)
                0   17   = Setup S1 and T1
                0   16   = Setup W1
                0   15   = Setup S0 and T0
                0   14   = Setup W0
                0   13   = Setup Wb
                0   12   = Setup Z
                0   11   = Setup Alpha
                0   10   = Setup RGB
                0   9:6  = Number of vertices
                0   5:3  = Command (0=Independent tris, 1=Start new strip, 2=Continue strip)
                0   2:0  = Packet type (3)
                1  31:0  = Data word
        */
		case 3:
			count = 2;		/* X/Y */
			if (command & (1 << 28))
			{
				if (command & (3 << 10)) count++;		/* ARGB */
			}
			else
			{
				if (command & (1 << 10)) count += 3;	/* RGB */
				if (command & (1 << 11)) count++;		/* A */
			}
			if (command & (1 << 12)) count++;			/* Z */
			if (command & (1 << 13)) count++;			/* Wb */
			if (command & (1 << 14)) count++;			/* W0 */
			if (command & (1 << 15)) count += 2;		/* S0/T0 */
			if (command & (1 << 16)) count++;			/* W1 */
			if (command & (1 << 17)) count += 2;		/* S1/T1 */
			count *= (command >> 6) & 15;				/* numverts */
			return 1 + count + (command >> 29);

		/*
            Packet type 4: 1 + N words

              Word  Bits
                0  31:29 = Number of dummy entries following the data
                0  28:15 = General register mask
                0  14:3  = Register base
                0   2:0  = Packet type (4)
                1  31:0  = Data word
        */
		case 4:
			for (i = 15; i <= 28; i++)
				if (command & (1 << i)) count++;
			return 1 + count + (command >> 29);

		/*
            Packet type 5: 2 + N words

              Word  Bits
                0  31:30 = Space (0,1=reserved, 2=LFB, 3=texture)
                0  29:26 = Byte disable W2
                0  25:22 = Byte disable WN
                0  21:3  = Num words
                0   2:0  = Packet type (5)
                1  31:30 = Reserved
                1  29:0  = Base address [24:0]
                2  31:0  = Data word
        */
		case 5:
			return 2 + ((command >> 3) & 0x7ffff);

		default:
			printf("UNKNOWN PACKET TYPE %d\n", command & 7);
			return 1;
	}
	return 1;
}



/*************************************
 *
 *  Command FIFO execution
 *
 *************************************/

static UINT32 cmdfifo_execute(voodoo_state *v, cmdfifo_info *f)
{
	UINT32 *fifobase = v->fbi.ram;
	UINT32 readptr = f->rdptr;
	UINT32 *src = &fifobase[readptr / 4];
	UINT32 command = *src++;
	int count, inc, code, i;
	setup_vertex svert;
	offs_t target;
	int cycles = 0;

	switch (command & 7)
	{
		/*
            Packet type 0: 1 or 2 words

              Word  Bits
                0  31:29 = reserved
                0  28:6  = Address [24:2]
                0   5:3  = Function (0 = NOP, 1 = JSR, 2 = RET, 3 = JMP LOCAL, 4 = JMP AGP)
                0   2:0  = Packet type (0)
                1  31:11 = reserved (JMP AGP only)
                1  10:0  = Address [35:25]
        */
		case 0:

			/* extract parameters */
			target = (command >> 4) & 0x1fffffc;

			/* switch off of the specific command */
			switch ((command >> 3) & 7)
			{
				case 0:		/* NOP */
					if (LOG_CMDFIFO) logerror("  NOP\n");
					break;

				case 1:		/* JSR */
					if (LOG_CMDFIFO) logerror("  JSR $%06X\n", target);
					printf("JSR in CMDFIFO!\n");
					src = &fifobase[target / 4];
					break;

				case 2:		/* RET */
					if (LOG_CMDFIFO) logerror("  RET $%06X\n", target);
					fatalerror("RET in CMDFIFO!");
					break;

				case 3:		/* JMP LOCAL FRAME BUFFER */
					if (LOG_CMDFIFO) logerror("  JMP LOCAL FRAMEBUF $%06X\n", target);
					src = &fifobase[target / 4];
					break;

				case 4:		/* JMP AGP */
					if (LOG_CMDFIFO) logerror("  JMP AGP $%06X\n", target);
					fatalerror("JMP AGP in CMDFIFO!");
					src = &fifobase[target / 4];
					break;

				default:
					printf("INVALID JUMP COMMAND!\n");
					fatalerror("  INVALID JUMP COMMAND");
					break;
			}
			break;

		/*
            Packet type 1: 1 + N words

              Word  Bits
                0  31:16 = Number of words
                0    15  = Increment?
                0  14:3  = Register base
                0   2:0  = Packet type (1)
                1  31:0  = Data word
        */
		case 1:

			/* extract parameters */
			count = command >> 16;
			inc = (command >> 15) & 1;
			target = (command >> 3) & 0xfff;

			if (LOG_CMDFIFO) logerror("  PACKET TYPE 1: count=%d inc=%d reg=%04X\n", count, inc, target);

			/* loop over all registers and write them one at a time */
			for (i = 0; i < count; i++, target += inc)
				cycles += register_w(v, target, *src++);
			break;

		/*
            Packet type 2: 1 + N words

              Word  Bits
                0  31:3  = 2D Register mask
                0   2:0  = Packet type (2)
                1  31:0  = Data word
        */
		case 2:
			if (LOG_CMDFIFO) logerror("  PACKET TYPE 2: mask=%X\n", (command >> 3) & 0x1ffffff);

			/* loop over all registers and write them one at a time */
			for (i = 3; i <= 31; i++)
				if (command & (1 << i))
					cycles += register_w(v, bltSrcBaseAddr + (i - 3), *src++);
			break;

		/*
            Packet type 3: 1 + N words

              Word  Bits
                0  31:29 = Number of dummy entries following the data
                0   28   = Packed color data?
                0   25   = Disable ping pong sign correction (0=normal, 1=disable)
                0   24   = Culling sign (0=positive, 1=negative)
                0   23   = Enable culling (0=disable, 1=enable)
                0   22   = Strip mode (0=strip, 1=fan)
                0   17   = Setup S1 and T1
                0   16   = Setup W1
                0   15   = Setup S0 and T0
                0   14   = Setup W0
                0   13   = Setup Wb
                0   12   = Setup Z
                0   11   = Setup Alpha
                0   10   = Setup RGB
                0   9:6  = Number of vertices
                0   5:3  = Command (0=Independent tris, 1=Start new strip, 2=Continue strip)
                0   2:0  = Packet type (3)
                1  31:0  = Data word
        */
		case 3:

			/* extract parameters */
			count = (command >> 6) & 15;
			code = (command >> 3) & 7;

			if (LOG_CMDFIFO) logerror("  PACKET TYPE 3: count=%d code=%d mask=%03X smode=%02X pc=%d\n", count, code, (command >> 10) & 0xfff, (command >> 22) & 0x3f, (command >> 28) & 1);

			/* copy relevant bits into the setup mode register */
			v->reg[sSetupMode].u = ((command >> 10) & 0xff) | ((command >> 6) & 0xf0000);

			/* loop over triangles */
			for (i = 0; i < count; i++)
			{
				/* always extract X/Y */
				svert.x = *(float *)src++;
				svert.y = *(float *)src++;

				/* load ARGB values if packed */
				if (command & (1 << 28))
				{
					if (command & (3 << 10))
					{
						UINT32 argb = *src++;
						if (command & (1 << 10))
						{
							svert.r = RGB_RED(argb);
							svert.g = RGB_GREEN(argb);
							svert.b = RGB_BLUE(argb);
						}
						if (command & (1 << 11))
							svert.a = RGB_ALPHA(argb);
					}
				}

				/* load ARGB values if not packed */
				else
				{
					if (command & (1 << 10))
					{
						svert.r = *(float *)src++;
						svert.g = *(float *)src++;
						svert.b = *(float *)src++;
					}
					if (command & (1 << 11))
						svert.a = *(float *)src++;
				}

				/* load Z and Wb values */
				if (command & (1 << 12))
					svert.z = *(float *)src++;
				if (command & (1 << 13))
					svert.wb = *(float *)src++;

				/* load W0, S0, T0 values */
				if (command & (1 << 14))
					svert.w0 = *(float *)src++;
				if (command & (1 << 15))
				{
					svert.s0 = *(float *)src++;
					svert.t0 = *(float *)src++;
				}

				/* load W1, S1, T1 values */
				if (command & (1 << 16))
					svert.w1 = *(float *)src++;
				if (command & (1 << 17))
				{
					svert.s1 = *(float *)src++;
					svert.t1 = *(float *)src++;
				}

				/* if we're starting a new strip, or if this is the first of a set of verts */
				/* for a series of individual triangles, initialize all the verts */
				if ((code == 1 && i == 0) || (code == 0 && i % 3 == 0))
				{
					v->fbi.sverts = 1;
					v->fbi.svert[0] = v->fbi.svert[1] = v->fbi.svert[2] = svert;
				}

				/* otherwise, add this to the list */
				else
				{
					/* for strip mode, shuffle vertex 1 down to 0 */
					if (!(command & (1 << 22)))
						v->fbi.svert[0] = v->fbi.svert[1];

					/* copy 2 down to 1 and add our new one regardless */
					v->fbi.svert[1] = v->fbi.svert[2];
					v->fbi.svert[2] = svert;

					/* if we have enough, draw */
					if (++v->fbi.sverts >= 3)
						cycles += setup_and_draw_triangle(v);
				}
			}

			/* account for the extra dummy words */
			src += command >> 29;
			break;

		/*
            Packet type 4: 1 + N words

              Word  Bits
                0  31:29 = Number of dummy entries following the data
                0  28:15 = General register mask
                0  14:3  = Register base
                0   2:0  = Packet type (4)
                1  31:0  = Data word
        */
		case 4:

			/* extract parameters */
			target = (command >> 3) & 0xfff;

			if (LOG_CMDFIFO) logerror("  PACKET TYPE 4: mask=%X reg=%04X pad=%d\n", (command >> 15) & 0x3fff, target, command >> 29);

			/* loop over all registers and write them one at a time */
			for (i = 15; i <= 28; i++)
				if (command & (1 << i))
					cycles += register_w(v, target + (i - 15), *src++);

			/* account for the extra dummy words */
			src += command >> 29;
			break;

		/*
            Packet type 5: 2 + N words

              Word  Bits
                0  31:30 = Space (0,1=reserved, 2=LFB, 3=texture)
                0  29:26 = Byte disable W2
                0  25:22 = Byte disable WN
                0  21:3  = Num words
                0   2:0  = Packet type (5)
                1  31:30 = Reserved
                1  29:0  = Base address [24:0]
                2  31:0  = Data word
        */
		case 5:

			/* extract parameters */
			count = (command >> 3) & 0x7ffff;
			target = *src++ / 4;

			/* handle LFB writes */
			if ((command >> 30) == 2)
			{
				if (LOG_CMDFIFO) logerror("  PACKET TYPE 5: LFB count=%d dest=%08X bd2=%X bdN=%X\n", count, target, (command >> 26) & 15, (command >> 22) & 15);

				/* loop over words */
				for (i = 0; i < count; i++)
					cycles += lfb_w(v, target++, *src++, 0, FALSE);
			}
			else if ((command >> 30) == 3)
			{
				if (LOG_CMDFIFO) logerror("  PACKET TYPE 5: textureRAM count=%d dest=%08X bd2=%X bdN=%X\n", count, target, (command >> 26) & 15, (command >> 22) & 15);

				/* loop over words */
				for (i = 0; i < count; i++)
					cycles += texture_w(v, target++, *src++);
			}
			break;

		default:
			fprintf(stderr, "PACKET TYPE %d\n", command & 7);
			break;
	}

	/* by default just update the read pointer past all the data we consumed */
	f->rdptr = 4 * (src - fifobase);
	return cycles;
}



/*************************************
 *
 *  Handle execution if we're ready
 *
 *************************************/

static INT32 cmdfifo_execute_if_ready(voodoo_state *v, cmdfifo_info *f)
{
	int needed_depth;
	int cycles;

	/* all CMDFIFO commands need at least one word */
	if (f->depth == 0)
		return -1;

	/* see if we have enough for the current command */
	needed_depth = cmdfifo_compute_expected_depth(v, f);
	if (f->depth < needed_depth)
		return -1;

	/* execute */
	cycles = cmdfifo_execute(v, f);
	f->depth -= needed_depth;
	return cycles;
}



/*************************************
 *
 *  Handle writes to the CMD FIFO
 *
 *************************************/

static void cmdfifo_w(voodoo_state *v, cmdfifo_info *f, offs_t offset, UINT32 data)
{
	UINT32 addr = f->base + offset * 4;
	UINT32 *fifobase = v->fbi.ram;

	if (LOG_CMDFIFO_VERBOSE) logerror("CMDFIFO_w(%04X) = %08X\n", offset, data);

	/* write the data */
	if (addr < f->end)
		fifobase[addr / 4] = data;

	/* count holes? */
	if (f->count_holes)
	{
		/* in-order, no holes */
		if (f->holes == 0 && addr == f->amin + 4)
		{
			f->amin = f->amax = addr;
			f->depth++;
		}

		/* out-of-order, below the minimum */
		else if (addr < f->amin)
		{
			if (f->holes != 0)
				logerror("Unexpected CMDFIFO: AMin=%08X AMax=%08X Holes=%d WroteTo:%08X\n",
						f->amin, f->amax, f->holes, addr);
			f->amin = f->amax = addr;
			f->depth++;
		}

		/* out-of-order, but within the min-max range */
		else if (addr < f->amax)
		{
			f->holes--;
			if (f->holes == 0)
			{
				f->depth += (f->amax - f->amin) / 4;
				f->amin = f->amax;
			}
		}

		/* out-of-order, bumping max */
		else
		{
			f->holes += (addr - f->amax) / 4 - 1;
			f->amax = addr;
		}
	}

	/* execute if we can */
	if (!v->pci.op_pending)
	{
		INT32 cycles = cmdfifo_execute_if_ready(v, f);
		if (cycles > 0)
		{
			v->pci.op_pending = TRUE;
			v->pci.op_end_time = add_subseconds_to_mame_time(mame_timer_get_time(), (subseconds_t)cycles * v->subseconds_per_cycle);

			if (LOG_FIFO_VERBOSE) logerror("VOODOO.%d.FIFO:direct write start at %d.%08X%08X end at %d.%08X%08X\n", v->index,
				mame_timer_get_time().seconds, (UINT32)(mame_timer_get_time().subseconds >> 32), (UINT32)mame_timer_get_time().subseconds,
				v->pci.op_end_time.seconds, (UINT32)(v->pci.op_end_time.subseconds >> 32), (UINT32)v->pci.op_end_time.subseconds);
		}
	}
}



/*************************************
 *
 *  Stall the activecpu until we are
 *  ready
 *
 *************************************/

static void stall_cpu_callback(void *param)
{
	check_stalled_cpu(param, mame_timer_get_time());
}


static void check_stalled_cpu(voodoo_state *v, mame_time current_time)
{
	int resume = FALSE;

	/* flush anything we can */
	if (v->pci.op_pending)
		flush_fifos(v, current_time);

	/* if we're just stalled until the LWM is passed, see if we're ok now */
	if (v->pci.stall_state == STALLED_UNTIL_FIFO_LWM)
	{
		/* if there's room in the memory FIFO now, we can proceed */
		if (FBIINIT0_ENABLE_MEMORY_FIFO(v->reg[fbiInit0].u))
		{
			if (fifo_items(&v->fbi.fifo) < 2 * 32 * FBIINIT0_MEMORY_FIFO_HWM(v->reg[fbiInit0].u))
				resume = TRUE;
		}
		else if (fifo_space(&v->pci.fifo) > 2 * FBIINIT0_PCI_FIFO_LWM(v->reg[fbiInit0].u))
			resume = TRUE;
	}

	/* if we're stalled until the FIFOs are empty, check now */
	else if (v->pci.stall_state == STALLED_UNTIL_FIFO_EMPTY)
	{
		if (FBIINIT0_ENABLE_MEMORY_FIFO(v->reg[fbiInit0].u))
		{
			if (fifo_empty(&v->fbi.fifo) && fifo_empty(&v->pci.fifo))
				resume = TRUE;
		}
		else if (fifo_empty(&v->pci.fifo))
			resume = TRUE;
	}

	/* resume if necessary */
	if (resume || !v->pci.op_pending)
	{
		if (LOG_FIFO) logerror("VOODOO.%d.FIFO:Stall condition cleared; resuming\n", v->index);
		v->pci.stall_state = NOT_STALLED;

		/* either call the callback, or trigger the trigger */
		if (v->pci.stall_callback)
			(*v->pci.stall_callback)(FALSE);
		else
			cpu_trigger(v->trigger);
	}

	/* if not, set a timer for the next one */
	else
	{
		mame_timer_adjust_ptr(v->pci.continue_timer, sub_mame_times(v->pci.op_end_time, current_time), time_never);
	}
}


static void stall_cpu(voodoo_state *v, int state, mame_time current_time)
{
	/* sanity check */
	if (!v->pci.op_pending) fatalerror("FIFOs not empty, no op pending!");

	/* set the state and update statistics */
	v->pci.stall_state = state;
	v->stats.stalls++;

	/* either call the callback, or spin the CPU */
	if (v->pci.stall_callback)
		(*v->pci.stall_callback)(TRUE);
	else
		cpu_spinuntil_trigger(v->trigger);

	/* set a timer to clear the stall */
	mame_timer_adjust_ptr(v->pci.continue_timer, sub_mame_times(v->pci.op_end_time, current_time), time_never);
}



/*************************************
 *
 *  Voodoo register writes
 *
 *************************************/

static INT32 register_w(voodoo_state *v, offs_t offset, UINT32 data)
{
	UINT32 origdata = data;
	INT32 cycles = 0;
	INT64 data64;
	UINT8 regnum;
	UINT8 chips;

	/* statistics */
	v->stats.reg_writes++;

	/* determine which chips we are addressing */
	chips = (offset >> 8) & 0xf;
	if (chips == 0)
		chips = 0xf;
	chips &= v->chipmask;

	/* the first 64 registers can be aliased differently */
	if ((offset & 0x800c0) == 0x80000 && v->alt_regmap)
		regnum = register_alias_map[offset & 0x3f];
	else
		regnum = offset & 0xff;

	/* first make sure this register is readable */
	if (!(v->regaccess[regnum] & REGISTER_WRITE))
	{
		logerror("VOODOO.%d.ERROR:Invalid attempt to write %s\n", v->index, v->regnames[regnum]);
		return 0;
	}

	/* switch off the register */
	switch (regnum)
	{
		/* Vertex data is 12.4 formatted fixed point */
		case fvertexAx:
			data = float_to_int32(data, 4);
		case vertexAx:
			if (chips & 1) v->fbi.ax = (INT16)data;
			break;

		case fvertexAy:
			data = float_to_int32(data, 4);
		case vertexAy:
			if (chips & 1) v->fbi.ay = (INT16)data;
			break;

		case fvertexBx:
			data = float_to_int32(data, 4);
		case vertexBx:
			if (chips & 1) v->fbi.bx = (INT16)data;
			break;

		case fvertexBy:
			data = float_to_int32(data, 4);
		case vertexBy:
			if (chips & 1) v->fbi.by = (INT16)data;
			break;

		case fvertexCx:
			data = float_to_int32(data, 4);
		case vertexCx:
			if (chips & 1) v->fbi.cx = (INT16)data;
			break;

		case fvertexCy:
			data = float_to_int32(data, 4);
		case vertexCy:
			if (chips & 1) v->fbi.cy = (INT16)data;
			break;

		/* RGB data is 12.12 formatted fixed point */
		case fstartR:
			data = float_to_int32(data, 12);
		case startR:
			if (chips & 1) v->fbi.startr = (INT32)(data << 8) >> 8;
			break;

		case fstartG:
			data = float_to_int32(data, 12);
		case startG:
			if (chips & 1) v->fbi.startg = (INT32)(data << 8) >> 8;
			break;

		case fstartB:
			data = float_to_int32(data, 12);
		case startB:
			if (chips & 1) v->fbi.startb = (INT32)(data << 8) >> 8;
			break;

		case fstartA:
			data = float_to_int32(data, 12);
		case startA:
			if (chips & 1) v->fbi.starta = (INT32)(data << 8) >> 8;
			break;

		case fdRdX:
			data = float_to_int32(data, 12);
		case dRdX:
			if (chips & 1) v->fbi.drdx = (INT32)(data << 8) >> 8;
			break;

		case fdGdX:
			data = float_to_int32(data, 12);
		case dGdX:
			if (chips & 1) v->fbi.dgdx = (INT32)(data << 8) >> 8;
			break;

		case fdBdX:
			data = float_to_int32(data, 12);
		case dBdX:
			if (chips & 1) v->fbi.dbdx = (INT32)(data << 8) >> 8;
			break;

		case fdAdX:
			data = float_to_int32(data, 12);
		case dAdX:
			if (chips & 1) v->fbi.dadx = (INT32)(data << 8) >> 8;
			break;

		case fdRdY:
			data = float_to_int32(data, 12);
		case dRdY:
			if (chips & 1) v->fbi.drdy = (INT32)(data << 8) >> 8;
			break;

		case fdGdY:
			data = float_to_int32(data, 12);
		case dGdY:
			if (chips & 1) v->fbi.dgdy = (INT32)(data << 8) >> 8;
			break;

		case fdBdY:
			data = float_to_int32(data, 12);
		case dBdY:
			if (chips & 1) v->fbi.dbdy = (INT32)(data << 8) >> 8;
			break;

		case fdAdY:
			data = float_to_int32(data, 12);
		case dAdY:
			if (chips & 1) v->fbi.dady = (INT32)(data << 8) >> 8;
			break;

		/* Z data is 20.12 formatted fixed point */
		case fstartZ:
			data = float_to_int32(data, 12);
		case startZ:
			if (chips & 1) v->fbi.startz = (INT32)data;
			break;

		case fdZdX:
			data = float_to_int32(data, 12);
		case dZdX:
			if (chips & 1) v->fbi.dzdx = (INT32)data;
			break;

		case fdZdY:
			data = float_to_int32(data, 12);
		case dZdY:
			if (chips & 1) v->fbi.dzdy = (INT32)data;
			break;

		/* S,T data is 14.18 formatted fixed point, converted to 16.32 internally */
		case fstartS:
			data64 = float_to_int64(data, 32);
			if (chips & 2) v->tmu[0].starts = data64;
			if (chips & 4) v->tmu[1].starts = data64;
			break;
		case startS:
			if (chips & 2) v->tmu[0].starts = (INT64)(INT32)data << 14;
			if (chips & 4) v->tmu[1].starts = (INT64)(INT32)data << 14;
			break;

		case fstartT:
			data64 = float_to_int64(data, 32);
			if (chips & 2) v->tmu[0].startt = data64;
			if (chips & 4) v->tmu[1].startt = data64;
			break;
		case startT:
			if (chips & 2) v->tmu[0].startt = (INT64)(INT32)data << 14;
			if (chips & 4) v->tmu[1].startt = (INT64)(INT32)data << 14;
			break;

		case fdSdX:
			data64 = float_to_int64(data, 32);
			if (chips & 2) v->tmu[0].dsdx = data64;
			if (chips & 4) v->tmu[1].dsdx = data64;
			break;
		case dSdX:
			if (chips & 2) v->tmu[0].dsdx = (INT64)(INT32)data << 14;
			if (chips & 4) v->tmu[1].dsdx = (INT64)(INT32)data << 14;
			break;

		case fdTdX:
			data64 = float_to_int64(data, 32);
			if (chips & 2) v->tmu[0].dtdx = data64;
			if (chips & 4) v->tmu[1].dtdx = data64;
			break;
		case dTdX:
			if (chips & 2) v->tmu[0].dtdx = (INT64)(INT32)data << 14;
			if (chips & 4) v->tmu[1].dtdx = (INT64)(INT32)data << 14;
			break;

		case fdSdY:
			data64 = float_to_int64(data, 32);
			if (chips & 2) v->tmu[0].dsdy = data64;
			if (chips & 4) v->tmu[1].dsdy = data64;
			break;
		case dSdY:
			if (chips & 2) v->tmu[0].dsdy = (INT64)(INT32)data << 14;
			if (chips & 4) v->tmu[1].dsdy = (INT64)(INT32)data << 14;
			break;

		case fdTdY:
			data64 = float_to_int64(data, 32);
			if (chips & 2) v->tmu[0].dtdy = data64;
			if (chips & 4) v->tmu[1].dtdy = data64;
			break;
		case dTdY:
			if (chips & 2) v->tmu[0].dtdy = (INT64)(INT32)data << 14;
			if (chips & 4) v->tmu[1].dtdy = (INT64)(INT32)data << 14;
			break;

		/* W data is 2.30 formatted fixed point, converted to 16.32 internally */
		case fstartW:
			data64 = float_to_int64(data, 32);
			if (chips & 1) v->fbi.startw = data64;
			if (chips & 2) v->tmu[0].startw = data64;
			if (chips & 4) v->tmu[1].startw = data64;
			break;
		case startW:
			if (chips & 1) v->fbi.startw = (INT64)(INT32)data << 2;
			if (chips & 2) v->tmu[0].startw = (INT64)(INT32)data << 2;
			if (chips & 4) v->tmu[1].startw = (INT64)(INT32)data << 2;
			break;

		case fdWdX:
			data64 = float_to_int64(data, 32);
			if (chips & 1) v->fbi.dwdx = data64;
			if (chips & 2) v->tmu[0].dwdx = data64;
			if (chips & 4) v->tmu[1].dwdx = data64;
			break;
		case dWdX:
			if (chips & 1) v->fbi.dwdx = (INT64)(INT32)data << 2;
			if (chips & 2) v->tmu[0].dwdx = (INT64)(INT32)data << 2;
			if (chips & 4) v->tmu[1].dwdx = (INT64)(INT32)data << 2;
			break;

		case fdWdY:
			data64 = float_to_int64(data, 32);
			if (chips & 1) v->fbi.dwdy = data64;
			if (chips & 2) v->tmu[0].dwdy = data64;
			if (chips & 4) v->tmu[1].dwdy = data64;
			break;
		case dWdY:
			if (chips & 1) v->fbi.dwdy = (INT64)(INT32)data << 2;
			if (chips & 2) v->tmu[0].dwdy = (INT64)(INT32)data << 2;
			if (chips & 4) v->tmu[1].dwdy = (INT64)(INT32)data << 2;
			break;

		/* setup bits */
		case sARGB:
			if (chips & 1)
			{
				v->reg[sAlpha].f = RGB_ALPHA(data);
				v->reg[sRed].f = RGB_RED(data);
				v->reg[sGreen].f = RGB_GREEN(data);
				v->reg[sBlue].f = RGB_BLUE(data);
			}
			break;

		/* mask off invalid bits for different cards */
		case fbzColorPath:
			if (v->type < VOODOO_2)
				data &= 0x0fffffff;
			if (chips & 1) v->reg[fbzColorPath].u = data;
			break;

		case fbzMode:
			if (v->type < VOODOO_2)
				data &= 0x001fffff;
			if (chips & 1) v->reg[fbzMode].u = data;
			break;

		case fogMode:
			if (v->type < VOODOO_2)
				data &= 0x0000003f;
			if (chips & 1) v->reg[fogMode].u = data;
			break;

		/* triangle drawing */
		case triangleCMD:
			v->fbi.cheating_allowed = (v->fbi.ax != 0 || v->fbi.ay != 0 || v->fbi.bx > 50 || v->fbi.by != 0 || v->fbi.cx != 0 || v->fbi.cy > 50);
			v->fbi.sign = data;
			cycles = triangle(v);
			break;

		case ftriangleCMD:
			v->fbi.cheating_allowed = TRUE;
			v->fbi.sign = data;
			cycles = triangle(v);
			break;

		case sBeginTriCMD:
			cycles = begin_triangle(v);
			break;

		case sDrawTriCMD:
			cycles = draw_triangle(v);
			break;

		/* other commands */
		case nopCMD:
			if (data & 1)
				reset_counters(v);
			if (data & 2)
				v->reg[fbiTrianglesOut].u = 0;
			break;

		case fastfillCMD:
			cycles = fastfill(v);
			break;

		case swapbufferCMD:
			cycles = swapbuffer(v, data);
			break;

		case userIntrCMD:
			fatalerror("userIntrCMD");
			break;

		/* gamma table access -- Voodoo/Voodoo2 only */
		case clutData:
			if (v->type <= VOODOO_2 && (chips & 1))
			{
				if (!FBIINIT1_VIDEO_TIMING_RESET(v->reg[fbiInit1].u))
				{
					int index = data >> 24;
					if (index <= 32)
					{
						v->fbi.clut[index] = data;
						v->fbi.clut_dirty = TRUE;
					}
				}
				else
					logerror("clutData ignored because video timing reset = 1\n");
			}
			break;

		/* external DAC access -- Voodoo/Voodoo2 only */
		case dacData:
			if (v->type <= VOODOO_2 && (chips & 1))
			{
				if (!(data & 0x800))
					dacdata_w(&v->dac, (data >> 8) & 7, data & 0xff);
				else
					dacdata_r(&v->dac, (data >> 8) & 7);
			}
			break;

		/* video dimensions -- Voodoo/Voodoo2 only */
		case videoDimensions:
			if (v->type <= VOODOO_2 && (chips & 1))
			{
				if (data & 0x3ff)
					v->fbi.width = data & 0x3ff;
				if (data & 0x3ff0000)
					v->fbi.height = (data >> 16) & 0x3ff;
				set_visible_area(0, v->fbi.width - 1, 0, v->fbi.height - 1);
				timer_adjust_ptr(v->fbi.vblank_timer, cpu_getscanlinetime(v->fbi.height), TIME_NEVER);
				recompute_video_memory(v);
			}
			break;

		/* vertical sync rate -- Voodoo/Voodoo2 only */
		case hSync:
		case vSync:
			if (v->type <= VOODOO_2 && (chips & 1))
			{
				v->reg[regnum].u = data;
				if (v->reg[hSync].u != 0 && v->reg[vSync].u != 0)
				{
					float vtotal = (v->reg[vSync].u >> 16) + (v->reg[vSync].u & 0xffff);
					float stdfps = 15750 / vtotal, stddiff = fabs(stdfps - Machine->drv->frames_per_second);
					float medfps = 25000 / vtotal, meddiff = fabs(medfps - Machine->drv->frames_per_second);
					float vgafps = 31500 / vtotal, vgadiff = fabs(vgafps - Machine->drv->frames_per_second);

					if (stddiff < meddiff && stddiff < vgadiff)
						set_refresh_rate(stdfps);
					else if (meddiff < vgadiff)
						set_refresh_rate(medfps);
					else
						set_refresh_rate(vgafps);
				}
			}
			break;

		/* fbiInit0 can only be written if initEnable says we can -- Voodoo/Voodoo2 only */
		case fbiInit0:
			if (v->type <= VOODOO_2 && (chips & 1) && INITEN_ENABLE_HW_INIT(v->pci.init_enable))
			{
				v->reg[fbiInit0].u = data;
				if (FBIINIT0_GRAPHICS_RESET(data))
					soft_reset(v);
				if (FBIINIT0_FIFO_RESET(data))
					fifo_reset(&v->pci.fifo);
				recompute_video_memory(v);
			}
			break;

		/* fbiInit5-7 are Voodoo 2-only; ignore them on anything else */
		case fbiInit5:
		case fbiInit6:
			if (v->type < VOODOO_2)
				break;
			/* else fall through... */

		/* fbiInitX can only be written if initEnable says we can -- Voodoo/Voodoo2 only */
		/* most of these affect memory layout, so always recompute that when done */
		case fbiInit1:
		case fbiInit2:
		case fbiInit4:
			if (v->type <= VOODOO_2 && (chips & 1) && INITEN_ENABLE_HW_INIT(v->pci.init_enable))
			{
				v->reg[regnum].u = data;
				recompute_video_memory(v);
			}
			break;

		case fbiInit3:
			if (v->type <= VOODOO_2 && (chips & 1) && INITEN_ENABLE_HW_INIT(v->pci.init_enable))
			{
				v->reg[regnum].u = data;
				v->alt_regmap = FBIINIT3_TRI_REGISTER_REMAP(data);
				v->fbi.yorigin = FBIINIT3_YORIGIN_SUBTRACT(v->reg[fbiInit3].u);
				recompute_video_memory(v);
			}
			break;

		case fbiInit7:
/*      case swapPending: -- Banshee */
			if (v->type == VOODOO_2 && (chips & 1) && INITEN_ENABLE_HW_INIT(v->pci.init_enable))
			{
				v->reg[regnum].u = data;
				v->fbi.cmdfifo[0].enable = FBIINIT7_CMDFIFO_ENABLE(data);
				v->fbi.cmdfifo[0].count_holes = !FBIINIT7_DISABLE_CMDFIFO_HOLES(data);
			}
			else if (v->type >= VOODOO_BANSHEE)
				v->fbi.swaps_pending++;
			break;

		/* cmdFifo -- Voodoo2 only */
		case cmdFifoBaseAddr:
			if (v->type == VOODOO_2 && (chips & 1))
			{
				v->reg[regnum].u = data;
				v->fbi.cmdfifo[0].base = (data & 0x3ff) << 12;
				v->fbi.cmdfifo[0].end = (((data >> 16) & 0x3ff) + 1) << 12;
			}
			break;

		case cmdFifoBump:
			if (v->type == VOODOO_2 && (chips & 1))
				fatalerror("cmdFifoBump");
			break;

		case cmdFifoRdPtr:
			if (v->type == VOODOO_2 && (chips & 1))
				v->fbi.cmdfifo[0].rdptr = data;
			break;

		case cmdFifoAMin:
/*      case colBufferAddr: -- Banshee */
			if (v->type == VOODOO_2 && (chips & 1))
				v->fbi.cmdfifo[0].amin = data;
			else if (v->type >= VOODOO_BANSHEE && (chips & 1))
			{
				v->fbi.rgb[1] = (UINT16 *)((UINT8 *)v->fbi.ram + (data & v->fbi.mask & ~0x0f));
				v->fbi.rgbmax[1] = (UINT16 *)((UINT8 *)v->fbi.ram + v->fbi.mask + 1) - v->fbi.rgb[1];
			}
			break;

		case cmdFifoAMax:
/*      case colBufferStride: -- Banshee */
			if (v->type == VOODOO_2 && (chips & 1))
				v->fbi.cmdfifo[0].amax = data;
			else if (v->type >= VOODOO_BANSHEE && (chips & 1))
			{
				if (data & 0x8000)
					v->fbi.rowpixels = (data & 0x7f) << 6;
				else
					v->fbi.rowpixels = (data & 0x3fff) >> 1;
			}
			break;

		case cmdFifoDepth:
/*      case auxBufferAddr: -- Banshee */
			if (v->type == VOODOO_2 && (chips & 1))
				v->fbi.cmdfifo[0].depth = data;
			else if (v->type >= VOODOO_BANSHEE && (chips & 1))
			{
				v->fbi.aux = (UINT16 *)((UINT8 *)v->fbi.ram + (data & v->fbi.mask & ~0x0f));
				v->fbi.auxmax = (UINT16 *)((UINT8 *)v->fbi.ram + v->fbi.mask + 1) - v->fbi.aux;
			}
			break;

		case cmdFifoHoles:
/*      case auxBufferStride: -- Banshee */
			if (v->type == VOODOO_2 && (chips & 1))
				v->fbi.cmdfifo[0].holes = data;
			else if (v->type >= VOODOO_BANSHEE && (chips & 1))
			{
				int rowpixels;

				if (data & 0x8000)
					rowpixels = (data & 0x7f) << 6;
				else
					rowpixels = (data & 0x3fff) >> 1;
				if (v->fbi.rowpixels != rowpixels)
					fatalerror("aux buffer stride differs from color buffer stride");
			}
			break;

		/* nccTable entries are processed and expanded immediately */
		case nccTable+0:
		case nccTable+1:
		case nccTable+2:
		case nccTable+3:
		case nccTable+4:
		case nccTable+5:
		case nccTable+6:
		case nccTable+7:
		case nccTable+8:
		case nccTable+9:
		case nccTable+10:
		case nccTable+11:
			if (chips & 2) ncc_table_write(&v->tmu[0].ncc[0], regnum - nccTable, data);
			if (chips & 4) ncc_table_write(&v->tmu[1].ncc[0], regnum - nccTable, data);
			break;

		case nccTable+12:
		case nccTable+13:
		case nccTable+14:
		case nccTable+15:
		case nccTable+16:
		case nccTable+17:
		case nccTable+18:
		case nccTable+19:
		case nccTable+20:
		case nccTable+21:
		case nccTable+22:
		case nccTable+23:
			if (chips & 2) ncc_table_write(&v->tmu[0].ncc[1], regnum - (nccTable+12), data);
			if (chips & 4) ncc_table_write(&v->tmu[1].ncc[1], regnum - (nccTable+12), data);
			break;

		/* fogTable entries are processed and expanded immediately */
		case fogTable+0:
		case fogTable+1:
		case fogTable+2:
		case fogTable+3:
		case fogTable+4:
		case fogTable+5:
		case fogTable+6:
		case fogTable+7:
		case fogTable+8:
		case fogTable+9:
		case fogTable+10:
		case fogTable+11:
		case fogTable+12:
		case fogTable+13:
		case fogTable+14:
		case fogTable+15:
		case fogTable+16:
		case fogTable+17:
		case fogTable+18:
		case fogTable+19:
		case fogTable+20:
		case fogTable+21:
		case fogTable+22:
		case fogTable+23:
		case fogTable+24:
		case fogTable+25:
		case fogTable+26:
		case fogTable+27:
		case fogTable+28:
		case fogTable+29:
		case fogTable+30:
		case fogTable+31:
			if (chips & 1)
			{
				int base = 2 * (regnum - fogTable);
				v->fbi.fogdelta[base + 0] = (data >> 0) & 0xff;
				v->fbi.fogblend[base + 0] = (data >> 8) & 0xff;
				v->fbi.fogdelta[base + 1] = (data >> 16) & 0xff;
				v->fbi.fogblend[base + 1] = (data >> 24) & 0xff;
			}
			break;

		/* texture modifications cause us to recompute everything */
		case textureMode:
		case tLOD:
		case tDetail:
		case texBaseAddr:
		case texBaseAddr_1:
		case texBaseAddr_2:
		case texBaseAddr_3_8:
			if (chips & 2)
			{
				v->tmu[0].reg[regnum].u = data;
				v->tmu[0].regdirty = TRUE;
			}
			if (chips & 4)
			{
				v->tmu[1].reg[regnum].u = data;
				v->tmu[1].regdirty = TRUE;
			}
			break;

		/* by default, just feed the data to the chips */
		default:
			if (chips & 1) v->reg[0x000 + regnum].u = data;
			if (chips & 2) v->reg[0x100 + regnum].u = data;
			if (chips & 4) v->reg[0x200 + regnum].u = data;
			if (chips & 8) v->reg[0x300 + regnum].u = data;
			break;
	}

	if (LOG_REGISTERS)
	{
		if (regnum < fvertexAx || regnum > fdWdY)
			logerror("VOODOO.%d.REG:%s(%d) write = %08X\n", v->index, (regnum < 0x384/4) ? v->regnames[regnum] : "oob", chips, origdata);
		else
			logerror("VOODOO.%d.REG:%s(%d) write = %f\n", v->index, (regnum < 0x384/4) ? v->regnames[regnum] : "oob", chips, u2f(origdata));
	}

	return cycles;
}



/*************************************
 *
 *  Voodoo LFB writes
 *
 *************************************/

static INT32 lfb_w(voodoo_state *v, offs_t offset, UINT32 data, UINT32 mem_mask, int forcefront)
{
	UINT16 *dest, *depth;
	UINT32 destmax, depthmax;
	int sr[2], sg[2], sb[2], sa[2], sw[2];
	int x, y, scry, mask;
	int pixel, destbuf;

	/* statistics */
	v->stats.lfb_writes++;

	/* byte swizzling */
	if (LFBMODE_BYTE_SWIZZLE_WRITES(v->reg[lfbMode].u))
	{
		data = FLIPENDIAN_INT32(data);
		mem_mask = FLIPENDIAN_INT32(mem_mask);
	}

	/* word swapping */
	if (LFBMODE_WORD_SWAP_WRITES(v->reg[lfbMode].u))
	{
		data = (data << 16) | (data >> 16);
		mem_mask = (mem_mask << 16) | (mem_mask >> 16);
	}

	/* extract default depth and alpha values */
	sw[0] = sw[1] = v->reg[zaColor].u & 0xffff;
	sa[0] = sa[1] = v->reg[zaColor].u >> 24;

	/* first extract A,R,G,B from the data */
	switch (LFBMODE_WRITE_FORMAT(v->reg[lfbMode].u) + 16 * LFBMODE_RGBA_LANES(v->reg[lfbMode].u))
	{
		case 16*0 + 0:		/* ARGB, 16-bit RGB 5-6-5 */
		case 16*2 + 0:		/* RGBA, 16-bit RGB 5-6-5 */
			EXTRACT_565_TO_888(data, sr[0], sg[0], sb[0]);
			EXTRACT_565_TO_888(data >> 16, sr[1], sg[1], sb[1]);
			mask = LFB_RGB_PRESENT | (LFB_RGB_PRESENT << 4);
			offset <<= 1;
			break;
		case 16*1 + 0:		/* ABGR, 16-bit RGB 5-6-5 */
		case 16*3 + 0:		/* BGRA, 16-bit RGB 5-6-5 */
			EXTRACT_565_TO_888(data, sb[0], sg[0], sr[0]);
			EXTRACT_565_TO_888(data >> 16, sb[1], sg[1], sr[1]);
			mask = LFB_RGB_PRESENT | (LFB_RGB_PRESENT << 4);
			offset <<= 1;
			break;

		case 16*0 + 1:		/* ARGB, 16-bit RGB x-5-5-5 */
			EXTRACT_x555_TO_888(data, sr[0], sg[0], sb[0]);
			EXTRACT_x555_TO_888(data >> 16, sr[1], sg[1], sb[1]);
			mask = LFB_RGB_PRESENT | (LFB_RGB_PRESENT << 4);
			offset <<= 1;
			break;
		case 16*1 + 1:		/* ABGR, 16-bit RGB x-5-5-5 */
			EXTRACT_x555_TO_888(data, sb[0], sg[0], sr[0]);
			EXTRACT_x555_TO_888(data >> 16, sb[1], sg[1], sr[1]);
			mask = LFB_RGB_PRESENT | (LFB_RGB_PRESENT << 4);
			offset <<= 1;
			break;
		case 16*2 + 1:		/* RGBA, 16-bit RGB x-5-5-5 */
			EXTRACT_555x_TO_888(data, sr[0], sg[0], sb[0]);
			EXTRACT_555x_TO_888(data >> 16, sr[1], sg[1], sb[1]);
			mask = LFB_RGB_PRESENT | (LFB_RGB_PRESENT << 4);
			offset <<= 1;
			break;
		case 16*3 + 1:		/* BGRA, 16-bit RGB x-5-5-5 */
			EXTRACT_555x_TO_888(data, sb[0], sg[0], sr[0]);
			EXTRACT_555x_TO_888(data >> 16, sb[1], sg[1], sr[1]);
			mask = LFB_RGB_PRESENT | (LFB_RGB_PRESENT << 4);
			offset <<= 1;
			break;

		case 16*0 + 2:		/* ARGB, 16-bit ARGB 1-5-5-5 */
			EXTRACT_1555_TO_8888(data, sa[0], sr[0], sg[0], sb[0]);
			EXTRACT_1555_TO_8888(data >> 16, sa[1], sr[1], sg[1], sb[1]);
			mask = LFB_RGB_PRESENT | LFB_ALPHA_PRESENT | ((LFB_RGB_PRESENT | LFB_ALPHA_PRESENT) << 4);
			offset <<= 1;
			break;
		case 16*1 + 2:		/* ABGR, 16-bit ARGB 1-5-5-5 */
			EXTRACT_1555_TO_8888(data, sa[0], sb[0], sg[0], sr[0]);
			EXTRACT_1555_TO_8888(data >> 16, sa[1], sb[1], sg[1], sr[1]);
			mask = LFB_RGB_PRESENT | LFB_ALPHA_PRESENT | ((LFB_RGB_PRESENT | LFB_ALPHA_PRESENT) << 4);
			offset <<= 1;
			break;
		case 16*2 + 2:		/* RGBA, 16-bit ARGB 1-5-5-5 */
			EXTRACT_5551_TO_8888(data, sr[0], sg[0], sb[0], sa[0]);
			EXTRACT_5551_TO_8888(data >> 16, sr[1], sg[1], sb[1], sa[1]);
			mask = LFB_RGB_PRESENT | LFB_ALPHA_PRESENT | ((LFB_RGB_PRESENT | LFB_ALPHA_PRESENT) << 4);
			offset <<= 1;
			break;
		case 16*3 + 2:		/* BGRA, 16-bit ARGB 1-5-5-5 */
			EXTRACT_5551_TO_8888(data, sb[0], sg[0], sr[0], sa[0]);
			EXTRACT_5551_TO_8888(data >> 16, sb[1], sg[1], sr[1], sa[1]);
			mask = LFB_RGB_PRESENT | LFB_ALPHA_PRESENT | ((LFB_RGB_PRESENT | LFB_ALPHA_PRESENT) << 4);
			offset <<= 1;
			break;

		case 16*0 + 4:		/* ARGB, 32-bit RGB x-8-8-8 */
			EXTRACT_x888_TO_888(data, sr[0], sg[0], sb[0]);
			mask = LFB_RGB_PRESENT;
			break;
		case 16*1 + 4:		/* ABGR, 32-bit RGB x-8-8-8 */
			EXTRACT_x888_TO_888(data, sb[0], sg[0], sr[0]);
			mask = LFB_RGB_PRESENT;
			break;
		case 16*2 + 4:		/* RGBA, 32-bit RGB x-8-8-8 */
			EXTRACT_888x_TO_888(data, sr[0], sg[0], sb[0]);
			mask = LFB_RGB_PRESENT;
			break;
		case 16*3 + 4:		/* BGRA, 32-bit RGB x-8-8-8 */
			EXTRACT_888x_TO_888(data, sb[0], sg[0], sr[0]);
			mask = LFB_RGB_PRESENT;
			break;

		case 16*0 + 5:		/* ARGB, 32-bit ARGB 8-8-8-8 */
			EXTRACT_8888_TO_8888(data, sa[0], sr[0], sg[0], sb[0]);
			mask = LFB_RGB_PRESENT | LFB_ALPHA_PRESENT;
			break;
		case 16*1 + 5:		/* ABGR, 32-bit ARGB 8-8-8-8 */
			EXTRACT_8888_TO_8888(data, sa[0], sb[0], sg[0], sr[0]);
			mask = LFB_RGB_PRESENT | LFB_ALPHA_PRESENT;
			break;
		case 16*2 + 5:		/* RGBA, 32-bit ARGB 8-8-8-8 */
			EXTRACT_8888_TO_8888(data, sr[0], sg[0], sb[0], sa[0]);
			mask = LFB_RGB_PRESENT | LFB_ALPHA_PRESENT;
			break;
		case 16*3 + 5:		/* BGRA, 32-bit ARGB 8-8-8-8 */
			EXTRACT_8888_TO_8888(data, sb[0], sg[0], sr[0], sa[0]);
			mask = LFB_RGB_PRESENT | LFB_ALPHA_PRESENT;
			break;

		case 16*0 + 12:		/* ARGB, 32-bit depth+RGB 5-6-5 */
		case 16*2 + 12:		/* RGBA, 32-bit depth+RGB 5-6-5 */
			sw[0] = data >> 16;
			EXTRACT_565_TO_888(data, sr[0], sg[0], sb[0]);
			mask = LFB_RGB_PRESENT | LFB_DEPTH_PRESENT_MSW;
			break;
		case 16*1 + 12:		/* ABGR, 32-bit depth+RGB 5-6-5 */
		case 16*3 + 12:		/* BGRA, 32-bit depth+RGB 5-6-5 */
			sw[0] = data >> 16;
			EXTRACT_565_TO_888(data, sb[0], sg[0], sr[0]);
			mask = LFB_RGB_PRESENT | LFB_DEPTH_PRESENT_MSW;
			break;

		case 16*0 + 13:		/* ARGB, 32-bit depth+RGB x-5-5-5 */
			sw[0] = data >> 16;
			EXTRACT_x555_TO_888(data, sr[0], sg[0], sb[0]);
			mask = LFB_RGB_PRESENT | LFB_DEPTH_PRESENT_MSW;
			break;
		case 16*1 + 13:		/* ABGR, 32-bit depth+RGB x-5-5-5 */
			sw[0] = data >> 16;
			EXTRACT_x555_TO_888(data, sb[0], sg[0], sr[0]);
			mask = LFB_RGB_PRESENT | LFB_DEPTH_PRESENT_MSW;
			break;
		case 16*2 + 13:		/* RGBA, 32-bit depth+RGB x-5-5-5 */
			sw[0] = data >> 16;
			EXTRACT_555x_TO_888(data, sr[0], sg[0], sb[0]);
			mask = LFB_RGB_PRESENT | LFB_DEPTH_PRESENT_MSW;
			break;
		case 16*3 + 13:		/* BGRA, 32-bit depth+RGB x-5-5-5 */
			sw[0] = data >> 16;
			EXTRACT_555x_TO_888(data, sb[0], sg[0], sr[0]);
			mask = LFB_RGB_PRESENT | LFB_DEPTH_PRESENT_MSW;
			break;

		case 16*0 + 14:		/* ARGB, 32-bit depth+ARGB 1-5-5-5 */
			sw[0] = data >> 16;
			EXTRACT_1555_TO_8888(data, sa[0], sr[0], sg[0], sb[0]);
			mask = LFB_RGB_PRESENT | LFB_ALPHA_PRESENT | LFB_DEPTH_PRESENT_MSW;
			break;
		case 16*1 + 14:		/* ABGR, 32-bit depth+ARGB 1-5-5-5 */
			sw[0] = data >> 16;
			EXTRACT_1555_TO_8888(data, sa[0], sb[0], sg[0], sr[0]);
			mask = LFB_RGB_PRESENT | LFB_ALPHA_PRESENT | LFB_DEPTH_PRESENT_MSW;
			break;
		case 16*2 + 14:		/* RGBA, 32-bit depth+ARGB 1-5-5-5 */
			sw[0] = data >> 16;
			EXTRACT_5551_TO_8888(data, sr[0], sg[0], sb[0], sa[0]);
			mask = LFB_RGB_PRESENT | LFB_ALPHA_PRESENT | LFB_DEPTH_PRESENT_MSW;
			break;
		case 16*3 + 14:		/* BGRA, 32-bit depth+ARGB 1-5-5-5 */
			sw[0] = data >> 16;
			EXTRACT_5551_TO_8888(data, sb[0], sg[0], sr[0], sa[0]);
			mask = LFB_RGB_PRESENT | LFB_ALPHA_PRESENT | LFB_DEPTH_PRESENT_MSW;
			break;

		case 16*0 + 15:		/* ARGB, 16-bit depth */
		case 16*1 + 15:		/* ARGB, 16-bit depth */
		case 16*2 + 15:		/* ARGB, 16-bit depth */
		case 16*3 + 15:		/* ARGB, 16-bit depth */
			sw[0] = data & 0xffff;
			sw[1] = data >> 16;
			mask = LFB_DEPTH_PRESENT | (LFB_DEPTH_PRESENT << 4);
			offset <<= 1;
			break;

		default:			/* reserved */
			return 0;
	}

	/* compute X,Y */
	x = (offset << 0) & ((1 << v->fbi.lfb_stride) - 1);
	y = (offset >> v->fbi.lfb_stride) & ((1 << v->fbi.lfb_stride) - 1);

	/* adjust the mask based on which half of the data is written */
	if ((mem_mask & 0x0000ffff) == 0x0000ffff)
		mask &= ~(0x0f - LFB_DEPTH_PRESENT_MSW);
	if ((mem_mask & 0xffff0000) == 0xffff0000)
		mask &= ~(0xf0 + LFB_DEPTH_PRESENT_MSW);

	/* select the target buffer */
	destbuf = (v->type >= VOODOO_BANSHEE) ? (!forcefront) : LFBMODE_WRITE_BUFFER_SELECT(v->reg[lfbMode].u);
	switch (destbuf)
	{
		case 0:			/* front buffer */
			dest = v->fbi.rgb[v->fbi.frontbuf];
			destmax = v->fbi.rgbmax[v->fbi.frontbuf];
			break;

		case 1:			/* back buffer */
			dest = v->fbi.rgb[v->fbi.backbuf];
			destmax = v->fbi.rgbmax[v->fbi.backbuf];
			break;

		default:		/* reserved */
			return 0;
	}
	depth = v->fbi.aux;
	depthmax = v->fbi.auxmax;

	/* simple case: no pipeline */
	if (!LFBMODE_ENABLE_PIXEL_PIPELINE(v->reg[lfbMode].u))
	{
		UINT32 bufoffs;

		if (LOG_LFB) logerror("VOODOO.%d.LFB:write raw mode %X (%d,%d) = %08X & %08X\n", v->index, LFBMODE_WRITE_FORMAT(v->reg[lfbMode].u), x, y, data, mem_mask);

		/* determine the screen Y */
		scry = y;
		if (LFBMODE_Y_ORIGIN(v->reg[lfbMode].u))
			scry = (v->fbi.yorigin - y) & 0x3ff;

		/* advance pointers to the proper row */
		bufoffs = scry * v->fbi.rowpixels + x;

		/* loop over up to two pixels */
		for (pixel = 0; mask; pixel++)
		{
			/* make sure we care about this pixel */
			if (mask & 0x0f)
			{
				/* write to the RGB buffer */
				if ((mask & LFB_RGB_PRESENT) && bufoffs < destmax)
				{
					/* apply dithering and write to the screen */
					APPLY_DITHER(v->reg[fbzMode].u, x, y, sr[pixel], sg[pixel], sb[pixel]);
					dest[bufoffs] = ((sr[pixel] & 0xf8) << 8) | ((sg[pixel] & 0xfc) << 3) | ((sb[pixel] & 0xf8) >> 3);
				}

				/* make sure we have an aux buffer to write to */
				if (depth && bufoffs < depthmax)
				{
					/* write to the alpha buffer */
					if ((mask & LFB_ALPHA_PRESENT) && FBZMODE_ENABLE_ALPHA_PLANES(v->reg[fbzMode].u))
						depth[bufoffs] = sa[pixel];

					/* write to the depth buffer */
					if ((mask & (LFB_DEPTH_PRESENT | LFB_DEPTH_PRESENT_MSW)) && !FBZMODE_ENABLE_ALPHA_PLANES(v->reg[fbzMode].u))
						depth[bufoffs] = sw[pixel];
				}

				/* track pixel writes to the frame buffer regardless of mask */
				v->reg[fbiPixelsOut].u++;
			}

			/* advance our pointers */
			bufoffs++;
			x++;
			mask >>= 4;
		}
	}

	/* tricky case: run the full pixel pipeline on the pixel */
	else
	{
		if (LOG_LFB) logerror("VOODOO.%d.LFB:write pipelined mode %X (%d,%d) = %08X & %08X\n", v->index, LFBMODE_WRITE_FORMAT(v->reg[lfbMode].u), x, y, data, mem_mask);

		/* determine the screen Y */
		scry = y;
		if (FBZMODE_Y_ORIGIN(v->reg[fbzMode].u))
			scry = (v->fbi.yorigin - y) & 0x3ff;

		/* advance pointers to the proper row */
		dest += scry * v->fbi.rowpixels;
		if (depth)
			depth += scry * v->fbi.rowpixels;

		/* loop over up to two pixels */
		for (pixel = 0; mask; pixel++)
		{
			/* make sure we care about this pixel */
			if (mask & 0x0f)
			{
				INT64 iterw = sw[pixel] << (30-16);
				INT32 iterz = sw[pixel] << 12;

				/* pixel pipeline part 1 handles depth testing and stippling */
				PIXEL_PIPELINE_BEGIN(v, x, y, scry, v->reg[fbzColorPath].u, v->reg[fbzMode].u, iterz, iterw);

				/* use the RGBA we stashed above */
				r = sr[pixel];
				g = sg[pixel];
				b = sb[pixel];
				a = sa[pixel];

				/* apply chroma key, alpha mask, and alpha testing */
				APPLY_CHROMAKEY(v, v->reg[fbzMode].u, MAKE_RGB(r, g, b));
				APPLY_ALPHAMASK(v, v->reg[fbzMode].u, a);
				APPLY_ALPHATEST(v, v->reg[alphaMode].u, a);

				/* pixel pipeline part 2 handles color combine, fog, alpha, and final output */
				PIXEL_PIPELINE_END(v, x, y, dest, depth, v->reg[fbzMode].u, v->reg[fbzColorPath].u, v->reg[alphaMode].u, v->reg[fogMode].u, iterz, iterw, v->reg[zaColor].u);
			}

			/* advance our pointers */
			x++;
			mask >>= 4;
		}
	}

	return 0;
}



/*************************************
 *
 *  Voodoo texture RAM writes
 *
 *************************************/

static INT32 texture_w(voodoo_state *v, offs_t offset, UINT32 data)
{
	int tmunum = (offset >> 19) & 0x03;
	tmu_state *t;

	/* statistics */
	v->stats.tex_writes++;

	/* point to the right TMU */
	if (!(v->chipmask & (2 << tmunum)))
		return 0;
	t = &v->tmu[tmunum];

	if (TEXLOD_TDIRECT_WRITE(t->reg[tLOD].u))
		fatalerror("Texture direct write!");

	/* update texture info if dirty */
	if (t->regdirty)
		recompute_texture_params(t);

	/* swizzle the data */
	if (TEXLOD_TDATA_SWIZZLE(t->reg[tLOD].u))
		data = FLIPENDIAN_INT32(data);
	if (TEXLOD_TDATA_SWAP(t->reg[tLOD].u))
		data = (data >> 16) | (data << 16);

	/* 8-bit texture case */
	if (TEXMODE_FORMAT(t->reg[textureMode].u) < 8)
	{
		int lod, tt, ts;
		UINT32 tbaseaddr;
		UINT8 *dest;

		/* extract info */
		if (v->type <= VOODOO_2)
		{
			lod = (offset >> 15) & 0x0f;
			tt = (offset >> 7) & 0xff;

			/* old code has a bit about how this is broken in gauntleg unless we always look at TMU0 */
			if (TEXMODE_SEQ_8_DOWNLD(v->tmu[0].reg/*t->reg*/[textureMode].u))
				ts = (offset << 2) & 0xfc;
			else
				ts = (offset << 1) & 0xfc;

			/* validate parameters */
			if (lod > 8)
				return 0;

			/* compute the base address */
			tbaseaddr = t->lodoffset[lod];
			tbaseaddr += tt * ((t->wmask >> lod) + 1) + ts;

			if (LOG_TEXTURE_RAM) logerror("Texture 8-bit w: lod=%d s=%d t=%d data=%08X\n", lod, ts, tt, data);
		}
		else
		{
			tbaseaddr = t->lodoffset[0] + offset*4;

			if (LOG_TEXTURE_RAM) logerror("Texture 16-bit w: offset=%X data=%08X\n", offset*4, data);
		}

		/* write the four bytes in little-endian order */
		dest = t->ram;
		tbaseaddr &= t->mask;
		dest[BYTE4_XOR_LE(tbaseaddr + 0)] = (data >> 0) & 0xff;
		dest[BYTE4_XOR_LE(tbaseaddr + 1)] = (data >> 8) & 0xff;
		dest[BYTE4_XOR_LE(tbaseaddr + 2)] = (data >> 16) & 0xff;
		dest[BYTE4_XOR_LE(tbaseaddr + 3)] = (data >> 24) & 0xff;
	}

	/* 16-bit texture case */
	else
	{
		int lod, tt, ts;
		UINT32 tbaseaddr;
		UINT16 *dest;

		/* extract info */
		if (v->type <= VOODOO_2)
		{
			tmunum = (offset >> 19) & 0x03;
			lod = (offset >> 15) & 0x0f;
			tt = (offset >> 7) & 0xff;
			ts = (offset << 1) & 0xfe;

			/* validate parameters */
			if (lod > 8)
				return 0;

			/* compute the base address */
			tbaseaddr = t->lodoffset[lod];
			tbaseaddr += 2 * (tt * ((t->wmask >> lod) + 1) + ts);

			if (LOG_TEXTURE_RAM) logerror("Texture 16-bit w: lod=%d s=%d t=%d data=%08X\n", lod, ts, tt, data);
		}
		else
		{
			tbaseaddr = t->lodoffset[0] + offset*4;

			if (LOG_TEXTURE_RAM) logerror("Texture 16-bit w: offset=%X data=%08X\n", offset*4, data);
		}

		/* write the two words in little-endian order */
		dest = (UINT16 *)t->ram;
		tbaseaddr &= t->mask;
		tbaseaddr >>= 1;
		dest[BYTE_XOR_LE(tbaseaddr + 0)] = (data >> 0) & 0xffff;
		dest[BYTE_XOR_LE(tbaseaddr + 1)] = (data >> 16) & 0xffff;
	}

	return 0;
}



/*************************************
 *
 *  Flush data from the FIFOs
 *
 *************************************/

static void flush_fifos(voodoo_state *v, mame_time current_time)
{
	static UINT8 in_flush;

	/* check for recursive calls */
	if (in_flush)
		return;
	in_flush = TRUE;

	if (!v->pci.op_pending) fatalerror("flush_fifos called with no pending operation");

	if (LOG_FIFO_VERBOSE) logerror("VOODOO.%d.FIFO:flush_fifos start -- pending=%d.%08X%08X cur=%d.%08X%08X\n", v->index,
		v->pci.op_end_time.seconds, (UINT32)(v->pci.op_end_time.subseconds >> 32), (UINT32)v->pci.op_end_time.subseconds,
		current_time.seconds, (UINT32)(current_time.subseconds >> 32), (UINT32)current_time.subseconds);

	/* loop while we still have cycles to burn */
	while (compare_mame_times(v->pci.op_end_time, current_time) <= 0)
	{
		INT32 extra_cycles = 0;
		INT32 cycles;

		/* loop over 0-cycle stuff; this constitutes the bulk of our writes */
		do
		{
			fifo_state *fifo;
			UINT32 address;
			UINT32 data;

			/* we might be in CMDFIFO mode */
			if (v->fbi.cmdfifo[0].enable)
			{
				/* if we don't have anything to execute, we're done for now */
				cycles = cmdfifo_execute_if_ready(v, &v->fbi.cmdfifo[0]);
				if (cycles == -1)
				{
					v->pci.op_pending = FALSE;
					in_flush = FALSE;
					if (LOG_FIFO_VERBOSE) logerror("VOODOO.%d.FIFO:flush_fifos end -- CMDFIFO empty\n", v->index);
					return;
				}
			}
			else if (v->fbi.cmdfifo[1].enable)
			{
				/* if we don't have anything to execute, we're done for now */
				cycles = cmdfifo_execute_if_ready(v, &v->fbi.cmdfifo[1]);
				if (cycles == -1)
				{
					v->pci.op_pending = FALSE;
					in_flush = FALSE;
					if (LOG_FIFO_VERBOSE) logerror("VOODOO.%d.FIFO:flush_fifos end -- CMDFIFO empty\n", v->index);
					return;
				}
			}

			/* else we are in standard PCI/memory FIFO mode */
			else
			{
				/* choose which FIFO to read from */
				if (!fifo_empty(&v->fbi.fifo))
					fifo = &v->fbi.fifo;
				else if (!fifo_empty(&v->pci.fifo))
					fifo = &v->pci.fifo;
				else
				{
					v->pci.op_pending = FALSE;
					in_flush = FALSE;
					if (LOG_FIFO_VERBOSE) logerror("VOODOO.%d.FIFO:flush_fifos end -- FIFOs empty\n", v->index);
					return;
				}

				/* extract address and data */
				address = fifo_remove(fifo);
				data = fifo_remove(fifo);

				/* target the appropriate location */
				if ((address & (0xc00000/4)) == 0)
					cycles = register_w(v, address, data);
				else if (address & (0x800000/4))
					cycles = texture_w(v, address, data);
				else
				{
					UINT32 mem_mask = 0;

					/* compute mem_mask */
					if (address & 0x80000000)
						mem_mask |= 0xffff0000;
					if (address & 0x40000000)
						mem_mask |= 0x0000ffff;
					address &= 0xffffff;

					cycles = lfb_w(v, address, data, mem_mask, FALSE);
				}
			}

			/* accumulate smaller operations */
			if (cycles < ACCUMULATE_THRESHOLD)
			{
				extra_cycles += cycles;
				cycles = 0;
			}
		}
		while (cycles == 0);

		/* account for extra cycles */
		cycles += extra_cycles;

		/* account for those cycles */
		v->pci.op_end_time = add_subseconds_to_mame_time(v->pci.op_end_time, (subseconds_t)cycles * v->subseconds_per_cycle);

		if (LOG_FIFO_VERBOSE) logerror("VOODOO.%d.FIFO:update -- pending=%d.%08X%08X cur=%d.%08X%08X\n", v->index,
			v->pci.op_end_time.seconds, (UINT32)(v->pci.op_end_time.subseconds >> 32), (UINT32)v->pci.op_end_time.subseconds,
			current_time.seconds, (UINT32)(current_time.subseconds >> 32), (UINT32)current_time.subseconds);
	}

	if (LOG_FIFO_VERBOSE) logerror("VOODOO.%d.FIFO:flush_fifos end -- pending command complete at %d.%08X%08X\n", v->index,
		v->pci.op_end_time.seconds, (UINT32)(v->pci.op_end_time.subseconds >> 32), (UINT32)v->pci.op_end_time.subseconds);

	in_flush = FALSE;
}



/*************************************
 *
 *  Handle a write to the Voodoo
 *  memory space
 *
 *************************************/

static void voodoo_w(voodoo_state *v, offs_t offset, UINT32 data, UINT32 mem_mask)
{
	int stall = FALSE;

	profiler_mark(PROFILER_USER1);

	/* should not be getting accesses while stalled */
	if (v->pci.stall_state != NOT_STALLED)
		logerror("voodoo_w while stalled!\n");

	/* if we have something pending, flush the FIFOs up to the current time */
	if (v->pci.op_pending)
		flush_fifos(v, mame_timer_get_time());

	/* special handling for registers */
	if ((offset & 0xc00000/4) == 0)
	{
		UINT8 access;

		/* some special stuff for Voodoo 2 */
		if (v->type >= VOODOO_2)
		{
			/* we might be in CMDFIFO mode */
			if (FBIINIT7_CMDFIFO_ENABLE(v->reg[fbiInit7].u))
			{
				/* if bit 21 is set, we're writing to the FIFO */
				if (offset & 0x200000/4)
				{
					/* check for byte swizzling (bit 18) */
					if (offset & 0x40000/4)
						data = FLIPENDIAN_INT32(data);
					cmdfifo_w(v, &v->fbi.cmdfifo[0], offset & 0xffff, data);
					profiler_mark(PROFILER_END);
					return;
				}

				/* we're a register access; but only certain ones are allowed */
				access = v->regaccess[offset & 0xff];
				if (!(access & REGISTER_WRITETHRU))
				{
					/* track swap buffers regardless */
					if ((offset & 0xff) == swapbufferCMD)
						v->fbi.swaps_pending++;

					logerror("Ignoring write to %s in CMDFIFO mode\n", v->regnames[offset & 0xff]);
					profiler_mark(PROFILER_END);
					return;
				}
			}

			/* if not, we might be byte swizzled (bit 20) */
			else if (offset & 0x100000/4)
				data = FLIPENDIAN_INT32(data);
		}

		/* check the access behavior; note that the table works even if the */
		/* alternate mapping is used */
		access = v->regaccess[offset & 0xff];

		/* ignore if writes aren't allowed */
		if (!(access & REGISTER_WRITE))
		{
			profiler_mark(PROFILER_END);
			return;
		}

		/* if this is a non-FIFO command, let it go to the FIFO, but stall until it completes */
		if (!(access & REGISTER_FIFO))
			stall = TRUE;

		/* track swap buffers */
		if ((offset & 0xff) == swapbufferCMD)
			v->fbi.swaps_pending++;
	}

	/* if we don't have anything pending, or if FIFOs are disabled, just execute */
	if (!v->pci.op_pending || !INITEN_ENABLE_PCI_FIFO(v->pci.init_enable))
	{
		int cycles;

		/* target the appropriate location */
		if ((offset & (0xc00000/4)) == 0)
			cycles = register_w(v, offset, data);
		else if (offset & (0x800000/4))
			cycles = texture_w(v, offset, data);
		else
			cycles = lfb_w(v, offset, data, mem_mask, FALSE);

		/* if we ended up with cycles, mark the operation pending */
		if (cycles)
		{
			v->pci.op_pending = TRUE;
			v->pci.op_end_time = add_subseconds_to_mame_time(mame_timer_get_time(), (subseconds_t)cycles * v->subseconds_per_cycle);

			if (LOG_FIFO_VERBOSE) logerror("VOODOO.%d.FIFO:direct write start at %d.%08X%08X end at %d.%08X%08X\n", v->index,
				mame_timer_get_time().seconds, (UINT32)(mame_timer_get_time().subseconds >> 32), (UINT32)mame_timer_get_time().subseconds,
				v->pci.op_end_time.seconds, (UINT32)(v->pci.op_end_time.subseconds >> 32), (UINT32)v->pci.op_end_time.subseconds);
		}
		profiler_mark(PROFILER_END);
		return;
	}

	/* modify the offset based on the mem_mask */
	if (mem_mask)
	{
		if (mem_mask & 0xffff0000)
			offset |= 0x80000000;
		if (mem_mask & 0x0000ffff)
			offset |= 0x40000000;
	}

	/* if there's room in the PCI FIFO, add there */
	if (LOG_FIFO_VERBOSE) logerror("VOODOO.%d.FIFO:voodoo_w adding to PCI FIFO @ %08X=%08X\n", v->index, offset, data);
	if (!fifo_full(&v->pci.fifo))
	{
		fifo_add(&v->pci.fifo, offset);
		fifo_add(&v->pci.fifo, data);
	}
	else
		fatalerror("PCI FIFO full");

	/* handle flushing to the memory FIFO */
	if (FBIINIT0_ENABLE_MEMORY_FIFO(v->reg[fbiInit0].u) &&
		fifo_space(&v->pci.fifo) <= 2 * FBIINIT4_MEMORY_FIFO_LWM(v->reg[fbiInit4].u))
	{
		UINT8 valid[4];

		/* determine which types of data can go to the memory FIFO */
		valid[0] = TRUE;
		valid[1] = FBIINIT0_LFB_TO_MEMORY_FIFO(v->reg[fbiInit0].u);
		valid[2] = valid[3] = FBIINIT0_TEXMEM_TO_MEMORY_FIFO(v->reg[fbiInit0].u);

		/* flush everything we can */
		if (LOG_FIFO_VERBOSE) logerror("VOODOO.%d.FIFO:voodoo_w moving PCI FIFO to memory FIFO\n", v->index);
		while (!fifo_empty(&v->pci.fifo) && valid[(fifo_peek(&v->pci.fifo) >> 22) & 3])
		{
			fifo_add(&v->fbi.fifo, fifo_remove(&v->pci.fifo));
			fifo_add(&v->fbi.fifo, fifo_remove(&v->pci.fifo));
		}

		/* if we're above the HWM as a result, stall */
		if (FBIINIT0_STALL_PCIE_FOR_HWM(v->reg[fbiInit0].u) &&
			fifo_items(&v->fbi.fifo) >= 2 * 32 * FBIINIT0_MEMORY_FIFO_HWM(v->reg[fbiInit0].u))
		{
			if (LOG_FIFO) logerror("VOODOO.%d.FIFO:voodoo_w hit memory FIFO HWM -- stalling\n", v->index);
			stall_cpu(v, STALLED_UNTIL_FIFO_LWM, mame_timer_get_time());
		}
	}

	/* if we're at the LWM for the PCI FIFO, stall */
	if (FBIINIT0_STALL_PCIE_FOR_HWM(v->reg[fbiInit0].u) &&
		fifo_space(&v->pci.fifo) <= 2 * FBIINIT0_PCI_FIFO_LWM(v->reg[fbiInit0].u))
	{
		if (LOG_FIFO) logerror("VOODOO.%d.FIFO:voodoo_w hit PCI FIFO free LWM -- stalling\n", v->index);
		stall_cpu(v, STALLED_UNTIL_FIFO_LWM, mame_timer_get_time());
	}

	/* if we weren't ready, and this is a non-FIFO access, stall until the FIFOs are clear */
	if (stall)
	{
		if (LOG_FIFO_VERBOSE) logerror("VOODOO.%d.FIFO:voodoo_w wrote non-FIFO register -- stalling until clear\n", v->index);
		stall_cpu(v, STALLED_UNTIL_FIFO_EMPTY, mame_timer_get_time());
	}

	profiler_mark(PROFILER_END);
}



/*************************************
 *
 *  Handle a register read
 *
 *************************************/

static UINT32 register_r(voodoo_state *v, offs_t offset)
{
	int regnum = offset & 0xff;
	UINT32 result;

	/* statistics */
	v->stats.reg_reads++;

	/* first make sure this register is readable */
	if (!(v->regaccess[regnum] & REGISTER_READ))
	{
		logerror("VOODOO.%d.ERROR:Invalid attempt to read %s\n", v->index, v->regnames[regnum]);
		return 0xffffffff;
	}

	/* default result is the FBI register value */
	result = v->reg[regnum].u;

	/* some registers are dynamic; compute them */
	switch (regnum)
	{
		case status:

			/* start with a blank slate */
			result = 0;

			/* bits 5:0 are the PCI FIFO free space */
			if (fifo_empty(&v->pci.fifo))
				result |= 0x3f << 0;
			else
			{
				int temp = fifo_space(&v->pci.fifo)/2;
				if (temp > 0x3f)
					temp = 0x3f;
				result |= temp << 0;
			}

			/* bit 6 is the vertical retrace */
			result |= v->fbi.vblank << 6;

			/* bit 7 is FBI graphics engine busy */
			if (v->pci.op_pending)
				result |= 1 << 7;

			/* bit 8 is TREX busy */
			if (v->pci.op_pending)
				result |= 1 << 8;

			/* bit 9 is overall busy */
			if (v->pci.op_pending)
				result |= 1 << 9;

			/* Banshee is different starting here */
			if (v->type < VOODOO_BANSHEE)
			{
				/* bits 11:10 specifies which buffer is visible */
				result |= v->fbi.frontbuf << 10;

				/* bits 27:12 indicate memory FIFO freespace */
				if (!FBIINIT0_ENABLE_MEMORY_FIFO(v->reg[fbiInit0].u) || fifo_empty(&v->fbi.fifo))
					result |= 0xffff << 12;
				else
				{
					int temp = fifo_space(&v->fbi.fifo)/2;
					if (temp > 0xffff)
						temp = 0xffff;
					result |= temp << 12;
				}
			}
			else
			{
				/* bit 10 is 2D busy */

				/* bit 11 is cmd FIFO 0 busy */
				if (v->fbi.cmdfifo[0].enable && v->fbi.cmdfifo[0].depth > 0)
					result |= 1 << 11;

				/* bit 12 is cmd FIFO 1 busy */
				if (v->fbi.cmdfifo[1].enable && v->fbi.cmdfifo[1].depth > 0)
					result |= 1 << 12;
			}

			/* bits 30:28 are the number of pending swaps */
			if (v->fbi.swaps_pending > 7)
				result |= 7 << 28;
			else
				result |= v->fbi.swaps_pending << 28;

			/* bit 31 is not used */

			/* eat some cycles since people like polling here */
			activecpu_eat_cycles(1000);
			break;

		/* bit 2 of the initEnable register maps this to dacRead */
		case fbiInit2:
			if (INITEN_REMAP_INIT_TO_DAC(v->pci.init_enable))
				result = v->dac.read_result;
			break;

		/* return the current scanline for now */
		case vRetrace:
			result = cpu_getscanline();
			break;

		/* reserved area in the TMU read by the Vegas startup sequence */
		case hvRetrace:
			result = 0x200 << 16;	/* should be between 0x7b and 0x267 */
			result |= 0x80;			/* should be between 0x17 and 0x103 */
			break;

		/* cmdFifo -- Voodoo2 only */
		case cmdFifoRdPtr:
			result = v->fbi.cmdfifo[0].rdptr;

			/* eat some cycles since people like polling here */
			activecpu_eat_cycles(1000);
			break;

		case cmdFifoAMin:
			result = v->fbi.cmdfifo[0].amin;
			break;

		case cmdFifoAMax:
			result = v->fbi.cmdfifo[0].amax;
			break;

		case cmdFifoDepth:
			result = v->fbi.cmdfifo[0].depth;
			break;

		case cmdFifoHoles:
			result = v->fbi.cmdfifo[0].holes;
			break;

		/* all counters are 24-bit only */
		case fbiTrianglesOut:
		case fbiPixelsIn:
		case fbiChromaFail:
		case fbiZfuncFail:
		case fbiAfuncFail:
		case fbiPixelsOut:
			result &= 0xffffff;
			break;
	}

	if (LOG_REGISTERS)
	{
		int logit = TRUE;

		/* don't log multiple identical status reads from the same address */
        if (regnum == status)
        {
            offs_t pc = activecpu_get_pc();
            if (pc == v->last_status_pc && result == v->last_status_value)
                logit = FALSE;
            v->last_status_pc = pc;
            v->last_status_value = result;
        }
        if (regnum == cmdFifoRdPtr)
        	logit = FALSE;

		if (logit)
			logerror("VOODOO.%d.REG:%s read = %08X\n", v->index, v->regnames[regnum], result);
	}

	return result;
}



/*************************************
 *
 *  Handle an LFB read
 *
 *************************************/

static UINT32 lfb_r(voodoo_state *v, offs_t offset, int forcefront)
{
	UINT16 *buffer;
	UINT32 bufmax;
	UINT32 bufoffs;
	UINT32 data;
	int x, y, scry, destbuf;

	/* statistics */
	v->stats.lfb_reads++;

	/* compute X,Y */
	x = (offset << 1) & 0x3fe;
	y = (offset >> 9) & 0x3ff;

	/* select the target buffer */
	destbuf = (v->type >= VOODOO_BANSHEE) ? (!forcefront) : LFBMODE_READ_BUFFER_SELECT(v->reg[lfbMode].u);
	switch (destbuf)
	{
		case 0:			/* front buffer */
			buffer = v->fbi.rgb[v->fbi.frontbuf];
			bufmax = v->fbi.rgbmax[v->fbi.frontbuf];
			break;

		case 1:			/* back buffer */
			buffer = v->fbi.rgb[v->fbi.backbuf];
			bufmax = v->fbi.rgbmax[v->fbi.backbuf];
			break;

		case 2:			/* aux buffer */
			buffer = v->fbi.aux;
			bufmax = v->fbi.auxmax;
			if (!buffer)
				return 0xffffffff;
			break;

		default:		/* reserved */
			return 0xffffffff;
	}

	/* determine the screen Y */
	scry = y;
	if (LFBMODE_Y_ORIGIN(v->reg[lfbMode].u))
		scry = (v->fbi.yorigin - y) & 0x3ff;

	/* advance pointers to the proper row */
	bufoffs = scry * v->fbi.rowpixels + x;
	if (bufoffs >= bufmax)
		return 0xffffffff;

	/* compute the data */
	data = buffer[bufoffs + 0] | (buffer[bufoffs + 1] << 16);

	/* word swapping */
	if (LFBMODE_WORD_SWAP_READS(v->reg[lfbMode].u))
		data = (data << 16) | (data >> 16);

	/* byte swizzling */
	if (LFBMODE_BYTE_SWIZZLE_READS(v->reg[lfbMode].u))
		data = FLIPENDIAN_INT32(data);

	if (LOG_LFB) logerror("VOODOO.%d.LFB:read (%d,%d) = %08X\n", v->index, x, y, data);
	return data;
}



/*************************************
 *
 *  Handle a read from the Voodoo
 *  memory space
 *
 *************************************/

static UINT32 voodoo_r(voodoo_state *v, offs_t offset)
{
	/* if we have something pending, flush the FIFOs up to the current time */
	if (v->pci.op_pending)
		flush_fifos(v, mame_timer_get_time());

	/* target the appropriate location */
	if (!(offset & (0xc00000/4)))
		return register_r(v, offset);
	else if (!(offset & (0x800000/4)))
		return lfb_r(v, offset, FALSE);

	return 0xffffffff;
}



/*************************************
 *
 *  Handle a read from the Banshee
 *  I/O space
 *
 *************************************/

static UINT32 banshee_agp_r(voodoo_state *v, offs_t offset, UINT32 mem_mask)
{
	UINT32 result;

	offset &= 0x1ff/4;

	/* switch off the offset */
	switch (offset)
	{
		case cmdRdPtrL0:
			result = v->fbi.cmdfifo[0].rdptr;
			break;

		case cmdAMin0:
			result = v->fbi.cmdfifo[0].amin;
			break;

		case cmdAMax0:
			result = v->fbi.cmdfifo[0].amax;
			break;

		case cmdFifoDepth0:
			result = v->fbi.cmdfifo[0].depth;
			break;

		case cmdHoleCnt0:
			result = v->fbi.cmdfifo[0].holes;
			break;

		case cmdRdPtrL1:
			result = v->fbi.cmdfifo[1].rdptr;
			break;

		case cmdAMin1:
			result = v->fbi.cmdfifo[1].amin;
			break;

		case cmdAMax1:
			result = v->fbi.cmdfifo[1].amax;
			break;

		case cmdFifoDepth1:
			result = v->fbi.cmdfifo[1].depth;
			break;

		case cmdHoleCnt1:
			result = v->fbi.cmdfifo[1].holes;
			break;

		default:
			result = v->banshee.agp[offset];
			break;
	}

	if (LOG_REGISTERS)
		logerror("%08X:banshee_r(AGP:%s)\n", activecpu_get_pc(), banshee_agp_reg_name[offset]);
	return result;
}


static UINT32 banshee_r(voodoo_state *v, offs_t offset, UINT32 mem_mask)
{
	UINT32 result = 0xffffffff;

	/* if we have something pending, flush the FIFOs up to the current time */
	if (v->pci.op_pending)
		flush_fifos(v, mame_timer_get_time());

	if (offset < 0x80000/4)
		result = banshee_io_r(v, offset, mem_mask);
	else if (offset < 0x100000/4)
		result = banshee_agp_r(v, offset, mem_mask);
	else if (offset < 0x200000/4)
		logerror("%08X:banshee_r(2D:%X)\n", activecpu_get_pc(), (offset*4) & 0xfffff);
	else if (offset < 0x600000/4)
		result = register_r(v, offset & 0x1fffff/4);
	else if (offset < 0x800000/4)
		logerror("%08X:banshee_r(TEX:%X)\n", activecpu_get_pc(), (offset*4) & 0x1fffff);
	else if (offset < 0xc00000/4)
		logerror("%08X:banshee_r(RES:%X)\n", activecpu_get_pc(), (offset*4) & 0x3fffff);
	else if (offset < 0x1000000/4)
		logerror("%08X:banshee_r(YUV:%X)\n", activecpu_get_pc(), (offset*4) & 0x3fffff);
	else if (offset < 0x2000000/4)
	{
		UINT8 temp = v->fbi.lfb_stride;
		v->fbi.lfb_stride = 11;
		result = lfb_r(v, offset & 0xffffff/4, FALSE);
		v->fbi.lfb_stride = temp;
	}
	return result;
}


static UINT32 banshee_fb_r(voodoo_state *v, offs_t offset, UINT32 mem_mask)
{
	UINT32 result = 0xffffffff;

	/* if we have something pending, flush the FIFOs up to the current time */
	if (v->pci.op_pending)
		flush_fifos(v, mame_timer_get_time());

	if (offset < v->fbi.lfb_base)
	{
		logerror("%08X:banshee_fb_r(%X)\n", activecpu_get_pc(), offset*4);
		if (offset*4 <= v->fbi.mask)
			result = ((UINT32 *)v->fbi.ram)[offset];
	}
	else
		result = lfb_r(v, offset - v->fbi.lfb_base, FALSE);
	return result;
}


static UINT8 banshee_vga_r(voodoo_state *v, offs_t offset)
{
	UINT8 result = 0xff;

	offset &= 0x1f;

	/* switch off the offset */
	switch (offset + 0x3c0)
	{
		/* attribute access */
		case 0x3c0:
			if (v->banshee.vga[0x3c1 & 0x1f] < ARRAY_LENGTH(v->banshee.att))
				result = v->banshee.att[v->banshee.vga[0x3c1 & 0x1f]];
			if (LOG_REGISTERS)
				logerror("%08X:banshee_att_r(%X)\n", activecpu_get_pc(), v->banshee.vga[0x3c1 & 0x1f]);
			break;

		/* Input status 0 */
		case 0x3c2:
			/*
                bit 7 = Interrupt Status. When its value is ?1?, denotes that an interrupt is pending.
                bit 6:5 = Feature Connector. These 2 bits are readable bits from the feature connector.
                bit 4 = Sense. This bit reflects the state of the DAC monitor sense logic.
                bit 3:0 = Reserved. Read back as 0.
            */
			result = 0x00;
			if (LOG_REGISTERS)
				logerror("%08X:banshee_vga_r(%X)\n", activecpu_get_pc(), 0x300+offset);
			break;

		/* Sequencer access */
		case 0x3c5:
			if (v->banshee.vga[0x3c4 & 0x1f] < ARRAY_LENGTH(v->banshee.seq))
				result = v->banshee.seq[v->banshee.vga[0x3c4 & 0x1f]];
			if (LOG_REGISTERS)
				logerror("%08X:banshee_seq_r(%X)\n", activecpu_get_pc(), v->banshee.vga[0x3c4 & 0x1f]);
			break;

		/* Feature control */
		case 0x3ca:
			result = v->banshee.vga[0x3da & 0x1f];
			v->banshee.attff = 0;
			if (LOG_REGISTERS)
				logerror("%08X:banshee_vga_r(%X)\n", activecpu_get_pc(), 0x300+offset);
			break;

		/* Miscellaneous output */
		case 0x3cc:
			result = v->banshee.vga[0x3c2 & 0x1f];
			if (LOG_REGISTERS)
				logerror("%08X:banshee_vga_r(%X)\n", activecpu_get_pc(), 0x300+offset);
			break;

		/* Graphics controller access */
		case 0x3cf:
			if (v->banshee.vga[0x3ce & 0x1f] < ARRAY_LENGTH(v->banshee.gc))
				result = v->banshee.gc[v->banshee.vga[0x3ce & 0x1f]];
			if (LOG_REGISTERS)
				logerror("%08X:banshee_gc_r(%X)\n", activecpu_get_pc(), v->banshee.vga[0x3ce & 0x1f]);
			break;

		/* CRTC access */
		case 0x3d5:
			if (v->banshee.vga[0x3d4 & 0x1f] < ARRAY_LENGTH(v->banshee.crtc))
				result = v->banshee.crtc[v->banshee.vga[0x3d4 & 0x1f]];
			if (LOG_REGISTERS)
				logerror("%08X:banshee_crtc_r(%X)\n", activecpu_get_pc(), v->banshee.vga[0x3d4 & 0x1f]);
			break;

		/* Input status 1 */
		case 0x3da:
			/*
                bit 7:6 = Reserved. These bits read back 0.
                bit 5:4 = Display Status. These 2 bits reflect 2 of the 8 pixel data outputs from the Attribute
                            controller, as determined by the Attribute controller index 0x12 bits 4 and 5.
                bit 3 = Vertical sync Status. A ?1? indicates vertical retrace is in progress.
                bit 2:1 = Reserved. These bits read back 0x2.
                bit 0 = Display Disable. When this bit is 1, either horizontal or vertical display end has occurred,
                            otherwise video data is being displayed.
            */
			result = 0x04;
			if (LOG_REGISTERS)
				logerror("%08X:banshee_vga_r(%X)\n", activecpu_get_pc(), 0x300+offset);
			break;

		default:
			result = v->banshee.vga[offset];
			if (LOG_REGISTERS)
				logerror("%08X:banshee_vga_r(%X)\n", activecpu_get_pc(), 0x300+offset);
			break;
	}
	return result;
}


static UINT32 banshee_io_r(voodoo_state *v, offs_t offset, UINT32 mem_mask)
{
	UINT32 result;

	offset &= 0xff/4;

	/* switch off the offset */
	switch (offset)
	{
		case io_status:
			result = register_r(v, 0);
			break;

		case io_dacData:
			result = v->fbi.clut[v->banshee.io[io_dacAddr] & 0x1ff] = v->banshee.io[offset];
			if (LOG_REGISTERS)
				logerror("%08X:banshee_dac_r(%X)\n", activecpu_get_pc(), v->banshee.io[io_dacAddr] & 0x1ff);
			break;

		case io_vgab0:	case io_vgab4:	case io_vgab8:	case io_vgabc:
		case io_vgac0:	case io_vgac4:	case io_vgac8:	case io_vgacc:
		case io_vgad0:	case io_vgad4:	case io_vgad8:	case io_vgadc:
			result = 0;
			if (!(mem_mask & 0x000000ff))
				result |= banshee_vga_r(v, offset*4+0) << 0;
			if (!(mem_mask & 0x0000ff00))
				result |= banshee_vga_r(v, offset*4+1) << 8;
			if (!(mem_mask & 0x00ff0000))
				result |= banshee_vga_r(v, offset*4+2) << 16;
			if (!(mem_mask & 0xff000000))
				result |= banshee_vga_r(v, offset*4+3) << 24;
			break;

		default:
			result = v->banshee.io[offset];
			if (LOG_REGISTERS)
				logerror("%08X:banshee_io_r(%s)\n", activecpu_get_pc(), banshee_io_reg_name[offset]);
			break;
	}

	return result;
}


static UINT32 banshee_rom_r(voodoo_state *v, offs_t offset, UINT32 mem_mask)
{
	logerror("%08X:banshee_rom_r(%X)\n", activecpu_get_pc(), offset*4);
	return 0xffffffff;
}



static void banshee_agp_w(voodoo_state *v, offs_t offset, UINT32 data, UINT32 mem_mask)
{
	offset &= 0x1ff/4;

	/* switch off the offset */
	switch (offset)
	{
		case cmdBaseAddr0:
			COMBINE_DATA(&v->banshee.agp[offset]);
			v->fbi.cmdfifo[0].base = (data & 0xffffff) << 12;
			v->fbi.cmdfifo[0].end = v->fbi.cmdfifo[0].base + (((v->banshee.agp[cmdBaseSize0] & 0xff) + 1) << 12);
			break;

		case cmdBaseSize0:
			COMBINE_DATA(&v->banshee.agp[offset]);
			v->fbi.cmdfifo[0].end = v->fbi.cmdfifo[0].base + (((v->banshee.agp[cmdBaseSize0] & 0xff) + 1) << 12);
			v->fbi.cmdfifo[0].enable = (data >> 8) & 1;
			v->fbi.cmdfifo[0].count_holes = (~data >> 10) & 1;
			break;

		case cmdBump0:
			fatalerror("cmdBump0");
			break;

		case cmdRdPtrL0:
			v->fbi.cmdfifo[0].rdptr = data;
			break;

		case cmdAMin0:
			v->fbi.cmdfifo[0].amin = data;
			break;

		case cmdAMax0:
			v->fbi.cmdfifo[0].amax = data;
			break;

		case cmdFifoDepth0:
			v->fbi.cmdfifo[0].depth = data;
			break;

		case cmdHoleCnt0:
			v->fbi.cmdfifo[0].holes = data;
			break;

		case cmdBaseAddr1:
			COMBINE_DATA(&v->banshee.agp[offset]);
			v->fbi.cmdfifo[1].base = (data & 0xffffff) << 12;
			v->fbi.cmdfifo[1].end = v->fbi.cmdfifo[1].base + (((v->banshee.agp[cmdBaseSize1] & 0xff) + 1) << 12);
			break;

		case cmdBaseSize1:
			COMBINE_DATA(&v->banshee.agp[offset]);
			v->fbi.cmdfifo[1].end = v->fbi.cmdfifo[1].base + (((v->banshee.agp[cmdBaseSize1] & 0xff) + 1) << 12);
			v->fbi.cmdfifo[1].enable = (data >> 8) & 1;
			v->fbi.cmdfifo[1].count_holes = (~data >> 10) & 1;
			break;

		case cmdBump1:
			fatalerror("cmdBump1");
			break;

		case cmdRdPtrL1:
			v->fbi.cmdfifo[1].rdptr = data;
			break;

		case cmdAMin1:
			v->fbi.cmdfifo[1].amin = data;
			break;

		case cmdAMax1:
			v->fbi.cmdfifo[1].amax = data;
			break;

		case cmdFifoDepth1:
			v->fbi.cmdfifo[1].depth = data;
			break;

		case cmdHoleCnt1:
			v->fbi.cmdfifo[1].holes = data;
			break;

		default:
			COMBINE_DATA(&v->banshee.agp[offset]);
			break;
	}

	if (LOG_REGISTERS)
		logerror("%08X:banshee_w(AGP:%s) = %08X & %08X\n", activecpu_get_pc(), banshee_agp_reg_name[offset], data, ~mem_mask);
}


static void banshee_w(voodoo_state *v, offs_t offset, UINT32 data, UINT32 mem_mask)
{
	/* if we have something pending, flush the FIFOs up to the current time */
	if (v->pci.op_pending)
		flush_fifos(v, mame_timer_get_time());

	if (offset < 0x80000/4)
		banshee_io_w(v, offset, data, mem_mask);
	else if (offset < 0x100000/4)
		banshee_agp_w(v, offset, data, mem_mask);
	else if (offset < 0x200000/4)
		logerror("%08X:banshee_w(2D:%X) = %08X & %08X\n", activecpu_get_pc(), (offset*4) & 0xfffff, data, ~mem_mask);
	else if (offset < 0x600000/4)
		register_w(v, offset & 0x1fffff/4, data);
	else if (offset < 0x800000/4)
		logerror("%08X:banshee_w(TEX:%X) = %08X & %08X\n", activecpu_get_pc(), (offset*4) & 0x1fffff, data, ~mem_mask);
	else if (offset < 0xc00000/4)
		logerror("%08X:banshee_w(RES:%X) = %08X & %08X\n", activecpu_get_pc(), (offset*4) & 0x3fffff, data, ~mem_mask);
	else if (offset < 0x1000000/4)
		logerror("%08X:banshee_w(YUV:%X) = %08X & %08X\n", activecpu_get_pc(), (offset*4) & 0x3fffff, data, ~mem_mask);
	else if (offset < 0x2000000/4)
	{
		UINT8 temp = v->fbi.lfb_stride;
		v->fbi.lfb_stride = 11;
		lfb_w(v, offset & 0xffffff/4, data, mem_mask, FALSE);
		v->fbi.lfb_stride = temp;
	}
}


static void banshee_fb_w(voodoo_state *v, offs_t offset, UINT32 data, UINT32 mem_mask)
{
	UINT32 addr = offset*4;

	/* if we have something pending, flush the FIFOs up to the current time */
	if (v->pci.op_pending)
		flush_fifos(v, mame_timer_get_time());

	if (offset < v->fbi.lfb_base)
	{
		if (v->fbi.cmdfifo[0].enable && addr >= v->fbi.cmdfifo[0].base && addr < v->fbi.cmdfifo[0].end)
			cmdfifo_w(v, &v->fbi.cmdfifo[0], (addr - v->fbi.cmdfifo[0].base) / 4, data);
		else if (v->fbi.cmdfifo[1].enable && addr >= v->fbi.cmdfifo[1].base && addr < v->fbi.cmdfifo[1].end)
			cmdfifo_w(v, &v->fbi.cmdfifo[1], (addr - v->fbi.cmdfifo[1].base) / 4, data);
		else
		{
			if (offset*4 <= v->fbi.mask)
				COMBINE_DATA(&((UINT32 *)v->fbi.ram)[offset]);
			logerror("%08X:banshee_fb_w(%X) = %08X & %08X\n", activecpu_get_pc(), offset*4, data, ~mem_mask);
		}
	}
	else
		lfb_w(v, offset - v->fbi.lfb_base, data, mem_mask, FALSE);
}


static void banshee_vga_w(voodoo_state *v, offs_t offset, UINT8 data)
{
	offset &= 0x1f;

	/* switch off the offset */
	switch (offset + 0x3c0)
	{
		/* attribute access */
		case 0x3c0:
		case 0x3c1:
			if (v->banshee.attff == 0)
			{
				v->banshee.vga[0x3c1 & 0x1f] = data;
				if (LOG_REGISTERS)
					logerror("%08X:banshee_vga_w(%X) = %02X\n", activecpu_get_pc(), 0x3c0+offset, data);
			}
			else
			{
				if (v->banshee.vga[0x3c1 & 0x1f] < ARRAY_LENGTH(v->banshee.att))
					v->banshee.att[v->banshee.vga[0x3c1 & 0x1f]] = data;
				if (LOG_REGISTERS)
					logerror("%08X:banshee_att_w(%X) = %02X\n", activecpu_get_pc(), v->banshee.vga[0x3c1 & 0x1f], data);
			}
			v->banshee.attff ^= 1;
			break;

		/* Sequencer access */
		case 0x3c5:
			if (v->banshee.vga[0x3c4 & 0x1f] < ARRAY_LENGTH(v->banshee.seq))
				v->banshee.seq[v->banshee.vga[0x3c4 & 0x1f]] = data;
			if (LOG_REGISTERS)
				logerror("%08X:banshee_seq_w(%X) = %02X\n", activecpu_get_pc(), v->banshee.vga[0x3c4 & 0x1f], data);
			break;

		/* Graphics controller access */
		case 0x3cf:
			if (v->banshee.vga[0x3ce & 0x1f] < ARRAY_LENGTH(v->banshee.gc))
				v->banshee.gc[v->banshee.vga[0x3ce & 0x1f]] = data;
			if (LOG_REGISTERS)
				logerror("%08X:banshee_gc_w(%X) = %02X\n", activecpu_get_pc(), v->banshee.vga[0x3ce & 0x1f], data);
			break;

		/* CRTC access */
		case 0x3d5:
			if (v->banshee.vga[0x3d4 & 0x1f] < ARRAY_LENGTH(v->banshee.crtc))
				v->banshee.crtc[v->banshee.vga[0x3d4 & 0x1f]] = data;
			if (LOG_REGISTERS)
				logerror("%08X:banshee_crtc_w(%X) = %02X\n", activecpu_get_pc(), v->banshee.vga[0x3d4 & 0x1f], data);
			break;

		default:
			v->banshee.vga[offset] = data;
			if (LOG_REGISTERS)
				logerror("%08X:banshee_vga_w(%X) = %02X\n", activecpu_get_pc(), 0x3c0+offset, data);
			break;
	}
}


static void banshee_io_w(voodoo_state *v, offs_t offset, UINT32 data, UINT32 mem_mask)
{
	UINT32 old;

	offset &= 0xff/4;
	old = v->banshee.io[offset];

	/* switch off the offset */
	switch (offset)
	{
		case io_vidProcCfg:
			COMBINE_DATA(&v->banshee.io[offset]);
			if ((v->banshee.io[offset] ^ old) & 0x2800)
				v->fbi.clut_dirty = TRUE;
			if (LOG_REGISTERS)
				logerror("%08X:banshee_io_w(%s) = %08X & %08X\n", activecpu_get_pc(), banshee_io_reg_name[offset], data, ~mem_mask);
			break;

		case io_dacData:
			COMBINE_DATA(&v->banshee.io[offset]);
			if (v->banshee.io[offset] != v->fbi.clut[v->banshee.io[io_dacAddr] & 0x1ff])
			{
				v->fbi.clut[v->banshee.io[io_dacAddr] & 0x1ff] = v->banshee.io[offset];
				v->fbi.clut_dirty = TRUE;
			}
			if (LOG_REGISTERS)
				logerror("%08X:banshee_dac_w(%X) = %08X & %08X\n", activecpu_get_pc(), v->banshee.io[io_dacAddr] & 0x1ff, data, ~mem_mask);
			break;

		case io_miscInit0:
			COMBINE_DATA(&v->banshee.io[offset]);
			v->fbi.yorigin = (data >> 18) & 0xfff;
			if (LOG_REGISTERS)
				logerror("%08X:banshee_io_w(%s) = %08X & %08X\n", activecpu_get_pc(), banshee_io_reg_name[offset], data, ~mem_mask);
			break;

		case io_vidScreenSize:
			/* warning: this is a hack for now! We should really compute the screen size */
			/* from the CRTC registers */
			COMBINE_DATA(&v->banshee.io[offset]);
			if (data & 0xfff)
				v->fbi.width = data & 0xfff;
			if (data & 0xfff000)
				v->fbi.height = (data >> 12) & 0xfff;
			set_visible_area(0, v->fbi.width - 1, 0, v->fbi.height - 1);
			timer_adjust_ptr(v->fbi.vblank_timer, cpu_getscanlinetime(v->fbi.height), TIME_NEVER);
			if (LOG_REGISTERS)
				logerror("%08X:banshee_io_w(%s) = %08X & %08X\n", activecpu_get_pc(), banshee_io_reg_name[offset], data, ~mem_mask);
			break;

		case io_lfbMemoryConfig:
			v->fbi.lfb_base = (data & 0x1fff) << 10;
			v->fbi.lfb_stride = ((data >> 13) & 7) + 9;
			if (LOG_REGISTERS)
				logerror("%08X:banshee_io_w(%s) = %08X & %08X\n", activecpu_get_pc(), banshee_io_reg_name[offset], data, ~mem_mask);
			break;

		case io_vgab0:	case io_vgab4:	case io_vgab8:	case io_vgabc:
		case io_vgac0:	case io_vgac4:	case io_vgac8:	case io_vgacc:
		case io_vgad0:	case io_vgad4:	case io_vgad8:	case io_vgadc:
			if (!(mem_mask & 0x000000ff))
				banshee_vga_w(v, offset*4+0, data >> 0);
			if (!(mem_mask & 0x0000ff00))
				banshee_vga_w(v, offset*4+1, data >> 8);
			if (!(mem_mask & 0x00ff0000))
				banshee_vga_w(v, offset*4+2, data >> 16);
			if (!(mem_mask & 0xff000000))
				banshee_vga_w(v, offset*4+3, data >> 24);
			break;

		default:
			COMBINE_DATA(&v->banshee.io[offset]);
			if (LOG_REGISTERS)
				logerror("%08X:banshee_io_w(%s) = %08X & %08X\n", activecpu_get_pc(), banshee_io_reg_name[offset], data, ~mem_mask);
			break;
	}
}



/*************************************
 *
 *  Command handlers
 *
 *************************************/

static INT32 fastfill(voodoo_state *v)
{
	int sx = (v->reg[clipLeftRight].u >> 16) & 0x3ff;
	int ex = (v->reg[clipLeftRight].u >> 0) & 0x3ff;
	int sy = (v->reg[clipLowYHighY].u >> 16) & 0x3ff;
	int ey = (v->reg[clipLowYHighY].u >> 0) & 0x3ff;
	int x, y;

	/* if we're not clearing either, take no time */
	if (!FBZMODE_RGB_BUFFER_MASK(v->reg[fbzMode].u) &&
		!FBZMODE_AUX_BUFFER_MASK(v->reg[fbzMode].u))
		return 0;

	/* are we clearing the RGB buffer? */
	if (FBZMODE_RGB_BUFFER_MASK(v->reg[fbzMode].u))
	{
		UINT16 dither[16];
		UINT16 *drawbuf;
		int destbuf;

		/* determine the draw buffer */
		destbuf = (v->type >= VOODOO_BANSHEE) ? 1 : FBZMODE_DRAW_BUFFER(v->reg[fbzMode].u);
		switch (destbuf)
		{
			case 0:		/* front buffer */
				drawbuf = v->fbi.rgb[v->fbi.frontbuf];
				break;

			case 1:		/* back buffer */
				drawbuf = v->fbi.rgb[v->fbi.backbuf];
				break;

			default:	/* reserved */
				return 0;
		}

		/* determine the dither pattern */
		for (y = 0; y < 4; y++)
			for (x = 0; x < 4; x++)
			{
				int r = RGB_RED(v->reg[color1].u);
				int g = RGB_GREEN(v->reg[color1].u);
				int b = RGB_BLUE(v->reg[color1].u);

				APPLY_DITHER(v->reg[fbzMode].u, x, y, r, g, b);
				dither[y*4 + x] = ((r & 0xf8) << 8) | ((g & 0xfc) << 3) | ((b & 0xf8) >> 3);
			}

		/* now do the fill */
		for (y = sy; y < ey; y++)
		{
			UINT16 *ditherow = &dither[(y & 3) * 4];
			UINT16 *dest;
			int scry;

			/* determine the screen Y */
			scry = y;
			if (FBZMODE_Y_ORIGIN(v->reg[fbzMode].u))
				scry = (v->fbi.yorigin - y) & 0x3ff;

			/* fill this row */
			dest = drawbuf + scry * v->fbi.rowpixels;
			for (x = sx; x < ex; x++)
				dest[x] = ditherow[x & 3];
		}

		/* track pixel writes to the frame buffer regardless of mask */
		v->reg[fbiPixelsOut].u += (ey - sy) * (ex - sx);
	}

	/* are we clearing the depth buffer? */
	if (FBZMODE_AUX_BUFFER_MASK(v->reg[fbzMode].u) && v->fbi.aux)
	{
		UINT16 color = v->reg[zaColor].u;

		/* now do the fill */
		for (y = sy; y < ey; y++)
		{
			UINT16 *dest;
			int scry;

			/* determine the screen Y */
			scry = y;
			if (FBZMODE_Y_ORIGIN(v->reg[fbzMode].u))
				scry = (v->fbi.yorigin - y) & 0x3ff;

			/* fill this row */
			dest = v->fbi.aux + scry * v->fbi.rowpixels;
			for (x = sx; x < ex; x++)
				dest[x] = color;
		}
	}

	/* 2 pixels per clock */
	return ((ey - sy) * (ex - sx)) / 2;
}


static INT32 swapbuffer(voodoo_state *v, UINT32 data)
{
	/* set the don't swap value for Voodoo 2 */
	v->fbi.vblank_swap_pending = TRUE;
	v->fbi.vblank_swap = (data >> 1) & 0xff;
	v->fbi.vblank_dont_swap = (data >> 9) & 1;

	/* if we're not syncing to the retrace, process the command immediately */
	if (!(data & 1))
	{
		swap_buffers(v);
		return 0;
	}

	/* determine how many cycles to wait; we deliberately overshoot here because */
	/* the final count gets updated on the VBLANK */
	return (v->fbi.vblank_swap + 1) * v->freq / 30;
}


static INT32 triangle(voodoo_state *v)
{
	INT32 start_pixels_in = v->reg[fbiPixelsIn].u;
	INT32 start_pixels_out = v->reg[fbiPixelsOut].u;
	INT32 start_chroma_fail = v->reg[fbiChromaFail].u;
	INT32 start_zfunc_fail = v->reg[fbiZfuncFail].u;
	INT32 start_afunc_fail = v->reg[fbiAfuncFail].u;
	raster_info *info;
	int texcount = 0;
	UINT16 *drawbuf;
	int destbuf;

	profiler_mark(PROFILER_USER2);

	/* perform subpixel adjustments */
	if (FBZCP_CCA_SUBPIXEL_ADJUST(v->reg[fbzColorPath].u))
	{
		INT32 dx = 8 - (v->fbi.ax & 15);
		INT32 dy = 8 - (v->fbi.ay & 15);

		v->fbi.startr += (dy * v->fbi.drdy + dx * v->fbi.drdx) >> 4;
		v->fbi.startg += (dy * v->fbi.dgdy + dx * v->fbi.dgdx) >> 4;
		v->fbi.startb += (dy * v->fbi.dbdy + dx * v->fbi.dbdx) >> 4;
		v->fbi.starta += (dy * v->fbi.dady + dx * v->fbi.dadx) >> 4;
		v->fbi.startw += (dy * v->fbi.dwdy + dx * v->fbi.dwdx) >> 4;
		v->fbi.startz += fixed_mul_shift(dy, v->fbi.dzdy, 4) + fixed_mul_shift(dx, v->fbi.dzdx, 4);

		v->tmu[0].startw += (dy * v->tmu[0].dwdy + dx * v->tmu[0].dwdx) >> 4;
		v->tmu[0].starts += (dy * v->tmu[0].dsdy + dx * v->tmu[0].dsdx) >> 4;
		v->tmu[0].startt += (dy * v->tmu[0].dtdy + dx * v->tmu[0].dtdx) >> 4;

		if (v->chipmask & 0x04)
		{
			v->tmu[1].startw += (dy * v->tmu[1].dwdy + dx * v->tmu[1].dwdx) >> 4;
			v->tmu[1].starts += (dy * v->tmu[1].dsdy + dx * v->tmu[1].dsdx) >> 4;
			v->tmu[1].startt += (dy * v->tmu[1].dtdy + dx * v->tmu[1].dtdx) >> 4;
		}
	}

	/* determine the draw buffer */
	destbuf = (v->type >= VOODOO_BANSHEE) ? 1 : FBZMODE_DRAW_BUFFER(v->reg[fbzMode].u);
	switch (destbuf)
	{
		case 0:		/* front buffer */
			drawbuf = v->fbi.rgb[v->fbi.frontbuf];
			break;

		case 1:		/* back buffer */
			drawbuf = v->fbi.rgb[v->fbi.backbuf];
			break;

		default:	/* reserved */
			return TRIANGLE_SETUP_CLOCKS;
	}

	/* set up the textures */
	texcount = 0;
	if (!FBIINIT3_DISABLE_TMUS(v->reg[fbiInit3].u) && FBZCP_TEXTURE_ENABLE(v->reg[fbzColorPath].u))
	{
		texcount = 1;
		prepare_tmu(&v->tmu[0]);
		v->stats.texture_mode[TEXMODE_FORMAT(v->tmu[0].reg[textureMode].u)]++;

		/* see if we need to deal with a second TMU */
		if (v->chipmask & 0x04)
		{
			texcount = 2;
			prepare_tmu(&v->tmu[1]);
			v->stats.texture_mode[TEXMODE_FORMAT(v->tmu[1].reg[textureMode].u)]++;
		}
	}

	/* find a rasterizer that matches our current state */
	info = find_rasterizer(v, texcount);
	(*info->callback)(v, drawbuf);
	info->polys++;
	info->hits += v->reg[fbiPixelsIn].u - start_pixels_in;

	/* update stats */
	v->reg[fbiTrianglesOut].u++;

	if (LOG_REGISTERS) logerror("cycles = %d\n", TRIANGLE_SETUP_CLOCKS + v->reg[fbiPixelsIn].u - start_pixels_in);

	/* update stats */
	v->stats.total_triangles++;
	v->stats.total_pixels_in += v->reg[fbiPixelsIn].u - start_pixels_in;
	v->stats.total_pixels_out += v->reg[fbiPixelsOut].u - start_pixels_out;
	v->stats.total_chroma_fail += v->reg[fbiChromaFail].u - start_chroma_fail;
	v->stats.total_zfunc_fail += v->reg[fbiZfuncFail].u - start_zfunc_fail;
	v->stats.total_afunc_fail += v->reg[fbiAfuncFail].u - start_afunc_fail;

	profiler_mark(PROFILER_END);

	/* 1 pixel per clock, plus some setup time */
	return TRIANGLE_SETUP_CLOCKS + v->reg[fbiPixelsIn].u - start_pixels_in;
}


static INT32 begin_triangle(voodoo_state *v)
{
	setup_vertex *sv = &v->fbi.svert[2];

	/* extract all the data from registers */
	sv->x = v->reg[sVx].f;
	sv->y = v->reg[sVy].f;
	sv->wb = v->reg[sWb].f;
	sv->w0 = v->reg[sWtmu0].f;
	sv->s0 = v->reg[sS_W0].f;
	sv->t0 = v->reg[sT_W0].f;
	sv->w1 = v->reg[sWtmu1].f;
	sv->s1 = v->reg[sS_Wtmu1].f;
	sv->t1 = v->reg[sT_Wtmu1].f;
	sv->a = v->reg[sAlpha].f;
	sv->r = v->reg[sRed].f;
	sv->g = v->reg[sGreen].f;
	sv->b = v->reg[sBlue].f;

	/* spread it across all three verts and reset the count */
	v->fbi.svert[0] = v->fbi.svert[1] = v->fbi.svert[2];
	v->fbi.sverts = 1;

	return 0;
}


static INT32 draw_triangle(voodoo_state *v)
{
	setup_vertex *sv = &v->fbi.svert[2];
	int cycles = 0;

	/* for strip mode, shuffle vertex 1 down to 0 */
	if (!(v->reg[sSetupMode].u & (1 << 16)))
		v->fbi.svert[0] = v->fbi.svert[1];

	/* copy 2 down to 1 regardless */
	v->fbi.svert[1] = v->fbi.svert[2];

	/* extract all the data from registers */
	sv->x = v->reg[sVx].f;
	sv->y = v->reg[sVy].f;
	sv->wb = v->reg[sWb].f;
	sv->w0 = v->reg[sWtmu0].f;
	sv->s0 = v->reg[sS_W0].f;
	sv->t0 = v->reg[sT_W0].f;
	sv->w1 = v->reg[sWtmu1].f;
	sv->s1 = v->reg[sS_Wtmu1].f;
	sv->t1 = v->reg[sT_Wtmu1].f;
	sv->a = v->reg[sAlpha].f;
	sv->r = v->reg[sRed].f;
	sv->g = v->reg[sGreen].f;
	sv->b = v->reg[sBlue].f;

	/* if we have enough verts, go ahead and draw */
	if (++v->fbi.sverts >= 3)
		cycles = setup_and_draw_triangle(v);

	return cycles;
}



/*************************************
 *
 *  Triangle setup
 *
 *************************************/

static INT32 setup_and_draw_triangle(voodoo_state *v)
{
	float dx1, dy1, dx2, dy2;
	float divisor, tdiv;

	/* grab the X/Ys at least */
	v->fbi.ax = (INT16)(v->fbi.svert[0].x * 16.0);
	v->fbi.ay = (INT16)(v->fbi.svert[0].y * 16.0);
	v->fbi.bx = (INT16)(v->fbi.svert[1].x * 16.0);
	v->fbi.by = (INT16)(v->fbi.svert[1].y * 16.0);
	v->fbi.cx = (INT16)(v->fbi.svert[2].x * 16.0);
	v->fbi.cy = (INT16)(v->fbi.svert[2].y * 16.0);

	/* compute the divisor */
	divisor = 1.0f / ((v->fbi.svert[0].x - v->fbi.svert[1].x) * (v->fbi.svert[0].y - v->fbi.svert[2].y) -
					  (v->fbi.svert[0].x - v->fbi.svert[2].x) * (v->fbi.svert[0].y - v->fbi.svert[1].y));

	/* backface culling */
	if (v->reg[sSetupMode].u & 0x20000)
	{
		int culling_sign = (v->reg[sSetupMode].u >> 18) & 1;
		int divisor_sign = (divisor < 0);

		/* if doing strips and ping pong is enabled, apply the ping pong */
		if ((v->reg[sSetupMode].u & 0x90000) == 0x00000)
			culling_sign ^= (v->fbi.sverts - 3) & 1;

		/* if our sign matches the culling sign, we're done for */
		if (divisor_sign == culling_sign)
			return TRIANGLE_SETUP_CLOCKS;
	}

	/* compute the dx/dy values */
	dx1 = v->fbi.svert[0].y - v->fbi.svert[2].y;
	dx2 = v->fbi.svert[0].y - v->fbi.svert[1].y;
	dy1 = v->fbi.svert[0].x - v->fbi.svert[1].x;
	dy2 = v->fbi.svert[0].x - v->fbi.svert[2].x;

	/* set up R,G,B */
	tdiv = divisor * 4096.0f;
	if (v->reg[sSetupMode].u & (1 << 0))
	{
		v->fbi.startr = (INT32)(v->fbi.svert[0].r * 4096.0f);
		v->fbi.drdx = (INT32)(((v->fbi.svert[0].r - v->fbi.svert[1].r) * dx1 - (v->fbi.svert[0].r - v->fbi.svert[2].r) * dx2) * tdiv);
		v->fbi.drdy = (INT32)(((v->fbi.svert[0].r - v->fbi.svert[2].r) * dy1 - (v->fbi.svert[0].r - v->fbi.svert[1].r) * dy2) * tdiv);
		v->fbi.startg = (INT32)(v->fbi.svert[0].g * 4096.0f);
		v->fbi.dgdx = (INT32)(((v->fbi.svert[0].g - v->fbi.svert[1].g) * dx1 - (v->fbi.svert[0].g - v->fbi.svert[2].g) * dx2) * tdiv);
		v->fbi.dgdy = (INT32)(((v->fbi.svert[0].g - v->fbi.svert[2].g) * dy1 - (v->fbi.svert[0].g - v->fbi.svert[1].g) * dy2) * tdiv);
		v->fbi.startb = (INT32)(v->fbi.svert[0].b * 4096.0f);
		v->fbi.dbdx = (INT32)(((v->fbi.svert[0].b - v->fbi.svert[1].b) * dx1 - (v->fbi.svert[0].b - v->fbi.svert[2].b) * dx2) * tdiv);
		v->fbi.dbdy = (INT32)(((v->fbi.svert[0].b - v->fbi.svert[2].b) * dy1 - (v->fbi.svert[0].b - v->fbi.svert[1].b) * dy2) * tdiv);
	}

	/* set up alpha */
	if (v->reg[sSetupMode].u & (1 << 1))
	{
		v->fbi.starta = (INT32)(v->fbi.svert[0].a * 4096.0);
		v->fbi.dadx = (INT32)(((v->fbi.svert[0].a - v->fbi.svert[1].a) * dx1 - (v->fbi.svert[0].a - v->fbi.svert[2].a) * dx2) * tdiv);
		v->fbi.dady = (INT32)(((v->fbi.svert[0].a - v->fbi.svert[2].a) * dy1 - (v->fbi.svert[0].a - v->fbi.svert[1].a) * dy2) * tdiv);
	}

	/* set up Z */
	if (v->reg[sSetupMode].u & (1 << 2))
	{
		v->fbi.startz = (INT32)(v->fbi.svert[0].z * 4096.0);
		v->fbi.dzdx = (INT32)(((v->fbi.svert[0].z - v->fbi.svert[1].z) * dx1 - (v->fbi.svert[0].z - v->fbi.svert[2].z) * dx2) * tdiv);
		v->fbi.dzdy = (INT32)(((v->fbi.svert[0].z - v->fbi.svert[2].z) * dy1 - (v->fbi.svert[0].z - v->fbi.svert[1].z) * dy2) * tdiv);
	}

	/* set up Wb */
	tdiv = divisor * 65536.0f * 65536.0f;
	if (v->reg[sSetupMode].u & (1 << 3))
	{
		v->fbi.startw = v->tmu[0].startw = v->tmu[1].startw = (INT64)(v->fbi.svert[0].wb * 65536.0f * 65536.0f);
		v->fbi.dwdx = v->tmu[0].dwdx = v->tmu[1].dwdx = ((v->fbi.svert[0].wb - v->fbi.svert[1].wb) * dx1 - (v->fbi.svert[0].wb - v->fbi.svert[2].wb) * dx2) * tdiv;
		v->fbi.dwdy = v->tmu[0].dwdy = v->tmu[1].dwdy = ((v->fbi.svert[0].wb - v->fbi.svert[2].wb) * dy1 - (v->fbi.svert[0].wb - v->fbi.svert[1].wb) * dy2) * tdiv;
	}

	/* set up W0 */
	if (v->reg[sSetupMode].u & (1 << 4))
	{
		v->tmu[0].startw = v->tmu[1].startw = (INT64)(v->fbi.svert[0].w0 * 65536.0f * 65536.0f);
		v->tmu[0].dwdx = v->tmu[1].dwdx = ((v->fbi.svert[0].w0 - v->fbi.svert[1].w0) * dx1 - (v->fbi.svert[0].w0 - v->fbi.svert[2].w0) * dx2) * tdiv;
		v->tmu[0].dwdy = v->tmu[1].dwdy = ((v->fbi.svert[0].w0 - v->fbi.svert[2].w0) * dy1 - (v->fbi.svert[0].w0 - v->fbi.svert[1].w0) * dy2) * tdiv;
	}

	/* set up S0,T0 */
	if (v->reg[sSetupMode].u & (1 << 5))
	{
		v->tmu[0].starts = v->tmu[1].starts = (INT64)(v->fbi.svert[0].s0 * 65536.0f * 65536.0f);
		v->tmu[0].dsdx = v->tmu[1].dsdx = ((v->fbi.svert[0].s0 - v->fbi.svert[1].s0) * dx1 - (v->fbi.svert[0].s0 - v->fbi.svert[2].s0) * dx2) * tdiv;
		v->tmu[0].dsdy = v->tmu[1].dsdy = ((v->fbi.svert[0].s0 - v->fbi.svert[2].s0) * dy1 - (v->fbi.svert[0].s0 - v->fbi.svert[1].s0) * dy2) * tdiv;
		v->tmu[0].startt = v->tmu[1].startt = (INT64)(v->fbi.svert[0].t0 * 65536.0f * 65536.0f);
		v->tmu[0].dtdx = v->tmu[1].dtdx = ((v->fbi.svert[0].t0 - v->fbi.svert[1].t0) * dx1 - (v->fbi.svert[0].t0 - v->fbi.svert[2].t0) * dx2) * tdiv;
		v->tmu[0].dtdy = v->tmu[1].dtdy = ((v->fbi.svert[0].t0 - v->fbi.svert[2].t0) * dy1 - (v->fbi.svert[0].t0 - v->fbi.svert[1].t0) * dy2) * tdiv;
	}

	/* set up W1 */
	if (v->reg[sSetupMode].u & (1 << 6))
	{
		v->tmu[1].startw = (INT64)(v->fbi.svert[0].w1 * 65536.0f * 65536.0f);
		v->tmu[1].dwdx = ((v->fbi.svert[0].w1 - v->fbi.svert[1].w1) * dx1 - (v->fbi.svert[0].w1 - v->fbi.svert[2].w1) * dx2) * tdiv;
		v->tmu[1].dwdy = ((v->fbi.svert[0].w1 - v->fbi.svert[2].w1) * dy1 - (v->fbi.svert[0].w1 - v->fbi.svert[1].w1) * dy2) * tdiv;
	}

	/* set up S1,T1 */
	if (v->reg[sSetupMode].u & (1 << 7))
	{
		v->tmu[1].starts = (INT64)(v->fbi.svert[0].s1 * 65536.0f * 65536.0f);
		v->tmu[1].dsdx = ((v->fbi.svert[0].s1 - v->fbi.svert[1].s1) * dx1 - (v->fbi.svert[0].s1 - v->fbi.svert[2].s1) * dx2) * tdiv;
		v->tmu[1].dsdy = ((v->fbi.svert[0].s1 - v->fbi.svert[2].s1) * dy1 - (v->fbi.svert[0].s1 - v->fbi.svert[1].s1) * dy2) * tdiv;
		v->tmu[1].startt = (INT64)(v->fbi.svert[0].t1 * 65536.0f * 65536.0f);
		v->tmu[1].dtdx = ((v->fbi.svert[0].t1 - v->fbi.svert[1].t1) * dx1 - (v->fbi.svert[0].t1 - v->fbi.svert[2].t1) * dx2) * tdiv;
		v->tmu[1].dtdy = ((v->fbi.svert[0].t1 - v->fbi.svert[2].t1) * dy1 - (v->fbi.svert[0].t1 - v->fbi.svert[1].t1) * dy2) * tdiv;
	}

	/* draw the triangle */
	v->fbi.cheating_allowed = 1;
	return triangle(v);
}



/*************************************
 *
 *  Rasterizer management
 *
 *************************************/

static raster_info *add_rasterizer(voodoo_state *v, const raster_info *cinfo)
{
	raster_info *info = &v->rasterizer[v->next_rasterizer++];
	int hash = compute_raster_hash(cinfo);

	assert_always(v->next_rasterizer <= MAX_RASTERIZERS, "Out of space for new rasterizers!");

	/* make a copy of the info */
	*info = *cinfo;

	/* fill in the data */
	info->hits = 0;
	info->polys = 0;

	/* hook us into the hash table */
	info->next = v->raster_hash[hash];
	v->raster_hash[hash] = info;

	if (LOG_RASTERIZERS)
		printf("Adding rasterizer @ %08X : %08X %08X %08X %08X %08X %08X (hash=%d)\n",
				(UINT32)info->callback,
				info->eff_color_path, info->eff_alpha_mode, info->eff_fog_mode, info->eff_fbz_mode,
				info->eff_tex_mode_0, info->eff_tex_mode_1, hash);

	return info;
}


static raster_info *find_rasterizer(voodoo_state *v, int texcount)
{
	raster_info *info, *prev = NULL;
	raster_info curinfo;
	int hash;

	/* build an info struct with all the parameters */
	curinfo.eff_color_path = normalize_color_path(v->reg[fbzColorPath].u);
	curinfo.eff_alpha_mode = normalize_alpha_mode(v->reg[alphaMode].u);
	curinfo.eff_fog_mode = normalize_fog_mode(v->reg[fogMode].u);
	curinfo.eff_fbz_mode = normalize_fbz_mode(v->reg[fbzMode].u);
	curinfo.eff_tex_mode_0 = (texcount >= 1) ? normalize_tex_mode(v->tmu[0].reg[textureMode].u) : 0xffffffff;
	curinfo.eff_tex_mode_1 = (texcount >= 2) ? normalize_tex_mode(v->tmu[1].reg[textureMode].u) : 0xffffffff;

	/* compute the hash */
	hash = compute_raster_hash(&curinfo);

	/* find the appropriate hash entry */
	for (info = v->raster_hash[hash]; info; prev = info, info = info->next)
		if (info->eff_color_path == curinfo.eff_color_path &&
			info->eff_alpha_mode == curinfo.eff_alpha_mode &&
			info->eff_fog_mode == curinfo.eff_fog_mode &&
			info->eff_fbz_mode == curinfo.eff_fbz_mode &&
			info->eff_tex_mode_0 == curinfo.eff_tex_mode_0 &&
			info->eff_tex_mode_1 == curinfo.eff_tex_mode_1)
		{
			/* got it, move us to the head of the list */
			if (prev)
			{
				prev->next = info->next;
				info->next = v->raster_hash[hash];
				v->raster_hash[hash] = info;
			}

			/* return the result */
			return info;
		}

	/* attempt to generate one */
#ifdef VOODOO_DRC
	curinfo.callback = generate_rasterizer(v);
	curinfo.is_generic = FALSE;
	if (curinfo.callback == NULL)
#endif
	{
		curinfo.callback = (texcount == 0) ? raster_generic_0tmu : (texcount == 1) ? raster_generic_1tmu : raster_generic_2tmu;
		curinfo.is_generic = TRUE;
	}

	return add_rasterizer(v, &curinfo);
}


static void dump_rasterizer_stats(voodoo_state *v)
{
	raster_info *cur, *best;
	int hash;

	printf("----\n");

	/* loop until we've displayed everything */
	while (1)
	{
		best = NULL;

		/* find the highest entry */
		for (hash = 0; hash < RASTER_HASH_SIZE; hash++)
			for (cur = v->raster_hash[hash]; cur; cur = cur->next)
				if (!best || cur->hits > best->hits)
					best = cur;

		/* if we're done, we're done */
		if (!best || best->hits == 0)
			break;

		/* print it */
		printf("%9d  %10d %c  ( 0x%08X,  0x%08X, 0x%08X, 0x%08X, 0x%08X, 0x%08X )\n",
			best->polys,
			best->hits,
			best->is_generic ? '*' : ' ',
			best->eff_color_path,
			best->eff_alpha_mode,
			best->eff_fog_mode,
			best->eff_fbz_mode,
			best->eff_tex_mode_0,
			best->eff_tex_mode_1);

		/* reset */
		best->hits = best->polys = 0;
	}
}



/*************************************
 *
 *  Generic rasterizers
 *
 *************************************/

RASTERIZER(generic_0tmu, 0, v->reg[fbzColorPath].u, v->reg[fbzMode].u, v->reg[alphaMode].u,
			v->reg[fogMode].u, 0, 0)

RASTERIZER(generic_1tmu, 1, v->reg[fbzColorPath].u, v->reg[fbzMode].u, v->reg[alphaMode].u,
			v->reg[fogMode].u, v->tmu[0].reg[textureMode].u, 0)

RASTERIZER(generic_2tmu, 2, v->reg[fbzColorPath].u, v->reg[fbzMode].u, v->reg[alphaMode].u,
			v->reg[fogMode].u, v->tmu[0].reg[textureMode].u, v->tmu[1].reg[textureMode].u)



#else

/*************************************
 *
 *  Specific rasterizers
 *
 *************************************/

/* blitz -------> fbzColorPath alphaMode   fogMode,    fbzMode,    texMode0,   texMode1  */
RASTERIZER_ENTRY( 0x01420039,  0x00000000, 0x00000000, 0x000B073B, 0x0C261ACF, 0xFFFFFFFF )	/* title */
RASTERIZER_ENTRY( 0x00582C35,  0x00515110, 0x00000000, 0x000B0739, 0x0C261ACF, 0xFFFFFFFF )	/* video */
RASTERIZER_ENTRY( 0x00000035,  0x00000000, 0x00000000, 0x000B0739, 0x0C261A0F, 0xFFFFFFFF )	/* in-game */
RASTERIZER_ENTRY( 0x00002C35,  0x00515110, 0x00000000, 0x000B07F9, 0x0C261A0F, 0xFFFFFFFF )	/* select screen */
RASTERIZER_ENTRY( 0x01420039,  0x00000000, 0x00000000, 0x000B07F9, 0x0C261ACF, 0xFFFFFFFF )	/* select screens */

/* blitz2k -----> fbzColorPath alphaMode   fogMode,    fbzMode,    texMode0,   texMode1  */
RASTERIZER_ENTRY( 0x01420039,  0x00000000, 0x00000000, 0x000B0739, 0x0C261ACF, 0xFFFFFFFF )	/* select screens */

/* carnevil ----> fbzColorPath alphaMode   fogMode,    fbzMode,    texMode0,   texMode1  */
RASTERIZER_ENTRY( 0x00002435,  0x04045119, 0x00000000, 0x00030279, 0x0C261A0F, 0xFFFFFFFF )	/* clown */
RASTERIZER_ENTRY( 0x00002425,  0x00045119, 0x00000000, 0x00030679, 0x0C261A0F, 0xFFFFFFFF )	/* clown */

/* calspeed ----> fbzColorPath alphaMode   fogMode,    fbzMode,    texMode0,   texMode1  */
RASTERIZER_ENTRY( 0x00002815,  0x05045119, 0x00000001, 0x000A0723, 0x0C261ACF, 0xFFFFFFFF )	/* movie */
RASTERIZER_ENTRY( 0x01022C19,  0x05000009, 0x00000001, 0x000B0739, 0x0C26100F, 0xFFFFFFFF )	/* in-game */
RASTERIZER_ENTRY( 0x00002C15,  0x05045119, 0x00000001, 0x000B0739, 0x0C26180F, 0xFFFFFFFF )	/* in-game */
RASTERIZER_ENTRY( 0x01022C19,  0x05000009, 0x00000001, 0x000B073B, 0x0C26100F, 0xFFFFFFFF )	/* in-game */
RASTERIZER_ENTRY( 0x00480015,  0x05045119, 0x00000001, 0x000B0339, 0x0C26100F, 0xFFFFFFFF )	/* in-game */

/* mace --------> fbzColorPath alphaMode   fogMode,    fbzMode,    texMode0,   texMode1  */
RASTERIZER_ENTRY( 0x00000035,  0x00045119, 0x00000000, 0x000B0779, 0x082418DF, 0xFFFFFFFF )	/* character screen */
RASTERIZER_ENTRY( 0x00600C09,  0x00045119, 0x00000000, 0x000B0779, 0x0824100F, 0xFFFFFFFF )	/* in-game */
RASTERIZER_ENTRY( 0x00600C09,  0x00045119, 0x00000000, 0x000B0779, 0x0824180F, 0xFFFFFFFF )	/* in-game */
RASTERIZER_ENTRY( 0x00600C09,  0x00045119, 0x00000000, 0x000B0779, 0x082410CF, 0xFFFFFFFF )	/* in-game */
RASTERIZER_ENTRY( 0x00600C09,  0x00045119, 0x00000000, 0x000B0779, 0x082418CF, 0xFFFFFFFF )	/* in-game */

/* vaportrx ----> fbzColorPath alphaMode   fogMode,    fbzMode,    texMode0,   texMode1  */
RASTERIZER_ENTRY( 0x00482405,  0x00000009, 0x00000000, 0x000B0739, 0x0C26100F, 0xFFFFFFFF )	/* intro */
RASTERIZER_ENTRY( 0x00482405,  0x00000000, 0x00000000, 0x000B0739, 0x0C26100F, 0xFFFFFFFF )	/* in-game */
RASTERIZER_ENTRY( 0x00482435,  0x00000000, 0x00000000, 0x000B0739, 0x0C261A0F, 0xFFFFFFFF )	/* in-game */
RASTERIZER_ENTRY( 0x00482435,  0x00045119, 0x00000000, 0x000B07F9, 0x0C261ACF, 0xFFFFFFFF )	/* in-game */

/* wg3dh -------> fbzColorPath alphaMode   fogMode,    fbzMode,    texMode0,   texMode1  */
RASTERIZER_ENTRY( 0x00000035,  0x00045119, 0x00000000, 0x000B0779, 0x082410DF, 0xFFFFFFFF )	/* title */
RASTERIZER_ENTRY( 0x00000035,  0x00045119, 0x00000000, 0x000B0779, 0x0824101F, 0xFFFFFFFF )	/* credits */
RASTERIZER_ENTRY( 0x00000035,  0x00045119, 0x00000000, 0x000B0779, 0x08241ADF, 0xFFFFFFFF )	/* in-game */
RASTERIZER_ENTRY( 0x00480035,  0x00045119, 0x00000000, 0x000B0779, 0x082418DF, 0xFFFFFFFF )	/* in-game */
RASTERIZER_ENTRY( 0x00000035,  0x00045119, 0x00000000, 0x000B0779, 0x0824189F, 0xFFFFFFFF )	/* in-game */

/* gradius4 ----> fbzColorPath alphaMode   fogMode,    fbzMode,    texMode0,   texMode1  */
RASTERIZER_ENTRY( 0x02420002,  0x00000009, 0x00000000, 0x00030F7B, 0x08241AC7, 0xFFFFFFFF )	/* intro */
RASTERIZER_ENTRY( 0x01420021,  0x00005119, 0x00000000, 0x00030F7B, 0x14261AC7, 0xFFFFFFFF )	/* intro */
RASTERIZER_ENTRY( 0x00000005,  0x00005119, 0x00000000, 0x00030F7B, 0x14261A87, 0xFFFFFFFF )	/* in-game */

/* nbapbp ------> fbzColorPath alphaMode   fogMode,    fbzMode,    texMode0,   texMode1  */
RASTERIZER_ENTRY( 0x00424219,  0x00000000, 0x00000001, 0x00030B7B, 0x08241AC7, 0xFFFFFFFF )	/* intro */
RASTERIZER_ENTRY( 0x00002809,  0x00004110, 0x00000001, 0x00030FFB, 0x08241AC7, 0xFFFFFFFF )	/* in-game */
RASTERIZER_ENTRY( 0x00424219,  0x00000000, 0x00000001, 0x00030F7B, 0x08241AC7, 0xFFFFFFFF )	/* in-game */
RASTERIZER_ENTRY( 0x0200421A,  0x00001510, 0x00000001, 0x00030F7B, 0x08241AC7, 0xFFFFFFFF )	/* in-game */

#endif
