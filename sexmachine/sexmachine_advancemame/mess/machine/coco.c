/***************************************************************************

  machine/coco.c

  Functions to emulate general aspects of the machine (RAM, ROM, interrupts,
  I/O ports)

  References:
		There are two main references for the info for this driver
		- Tandy Color Computer Unravelled Series
					(http://www.giftmarket.org/unravelled/unravelled.shtml)
		- Assembly Language Programming For the CoCo 3 by Laurence A. Tepolt
		- Kevin K. Darlings GIME reference
					(http://www.cris.com/~Alxevans/gime.txt)
		- Sock Masters's GIME register reference
					(http://www.axess.com/twilight/sock/gime.html)
		- Robert Gault's FAQ
					(http://home.att.net/~robert.gault/Coco/FAQ/FAQ_main.htm)
		- Discussions with L. Curtis Boyle (LCB) and John Kowalski (JK)

  TODO:
		- Implement unimplemented SAM registers
		- Implement unimplemented interrupts (serial)
		- Choose and implement more appropriate ratios for the speed up poke
		- Handle resets correctly

  In the CoCo, all timings should be exactly relative to each other.  This
  table shows how all clocks are relative to each other (info: JK):
		- Main CPU Clock				0.89 MHz
		- Horizontal Sync Interrupt		15.7 kHz/63.5us	(57 clock cycles)
		- Vertical Sync Interrupt		60 Hz			(14934 clock cycles)
		- Composite Video Color Carrier	3.58 MHz/279ns	(1/4 clock cycles)

  It is also noting that the CoCo 3 had two sets of VSync interrupts.  To quote
  John Kowalski:

	One other thing to mention is that the old vertical interrupt and the new
	vertical interrupt are not the same..  The old one is triggered by the
	video's vertical sync pulse, but the new one is triggered on the next scan
	line *after* the last scan line of the active video display.  That is : new
	vertical interrupt triggers somewheres around scan line 230 of the 262 line
	screen (if a 200 line graphics mode is used, a bit earlier if a 192 line
	mode is used and a bit later if a 225 line mode is used).  The old vsync
	interrupt triggers on scanline zero.

	230 is just an estimate [(262-200)/2+200].  I don't think the active part
	of the screen is exactly centered within the 262 line total.  I can
	research that for you if you want an exact number for scanlines before the
	screen starts and the scanline that the v-interrupt triggers..etc.

Dragon Alpha code added 21-Oct-2004, 
			Phill Harvey-Smith (afra@aurigae.demon.co.uk)

			Added AY-8912 and FDC code 30-Oct-2004.

Fixed Dragon Alpha NMI enable/disable, following circuit traces on a real machine.
	P.Harvey-Smith, 11-Aug-2005.

***************************************************************************/

#include <math.h>
#include <assert.h>

#include "driver.h"
#include "cpu/m6809/m6809.h"
#include "machine/6821pia.h"
#include "includes/coco.h"
#include "includes/cococart.h"
#include "machine/6883sam.h"
#include "includes/6551.h"
#include "vidhrdw/m6847.h"
#include "formats/cocopak.h"
#include "devices/bitbngr.h"
#include "devices/printer.h"
#include "devices/cassette.h"
#include "image.h"
#include "state.h"
#include "machine/wd17xx.h"
#include "sound/dac.h"
#include "sound/ay8910.h"

UINT8 coco3_gimereg[16];

static UINT8 *coco_rom;
static int coco3_enable_64k;
static UINT8 coco3_mmu[16];
static UINT8 coco3_interupt_line;
static UINT8 gime_firq, gime_irq;
static int cart_line, cart_inserted;

static WRITE8_HANDLER ( d_pia1_pb_w );
static WRITE8_HANDLER ( d_pia1_pa_w );
static READ8_HANDLER ( d_pia1_cb1_r );
static READ8_HANDLER ( d_pia1_pa_r );
static READ8_HANDLER ( d_pia1_pb_r_coco );
static READ8_HANDLER ( d_pia1_pb_r_coco2 );
static WRITE8_HANDLER ( d_pia0_pa_w );
static WRITE8_HANDLER ( d_pia0_pb_w );
static WRITE8_HANDLER ( dragon64_pia1_pb_w );
static WRITE8_HANDLER ( d_pia1_cb2_w);
static WRITE8_HANDLER ( d_pia0_cb2_w);
static WRITE8_HANDLER ( d_pia1_ca2_w);
static WRITE8_HANDLER ( d_pia0_ca2_w);
static WRITE8_HANDLER ( coco3_pia0_pb_w );
static void d_pia0_irq_a(int state);
static void d_pia0_irq_b(int state);
static void d_pia1_firq_a(int state);
static void d_pia1_firq_b(int state);
static void coco3_pia0_irq_a(int state);
static void coco3_pia0_irq_b(int state);
static void coco3_pia1_firq_a(int state);
static void coco3_pia1_firq_b(int state);
static void coco_cartridge_enablesound(int enable);
static void d_sam_set_pageonemode(int val);
static void d_sam_set_mpurate(int val);
static void d_sam_set_memorysize(int val);
static void d_sam_set_maptype(int val);
static void dragon64_sam_set_maptype(int val);
static void coco3_sam_set_maptype(int val);
static const UINT8 *coco3_sam_get_rambase(void);
static void coco_setcartline(int data);
static void coco3_setcartline(int data);
static void coco_machine_stop(void);

/* Are we a CoCo or a Dragon ? */
typedef enum
{
	AM_COCO,
	AM_DRAGON
} coco_or_dragon_t;
static coco_or_dragon_t coco_or_dragon;

/* Dragon 64 / Alpha shared */
static int dragon_map_type;					/* Dragonm 64/Alpha map type, used for rom paging */
static UINT8 *dragon_rom_bank;					/* Dragon 64 / Alpha rom bank in use */
static void dragon_page_rom(int	romswitch);

/* Dragon Alpha only */
static WRITE8_HANDLER ( dgnalpha_pia2_pa_w );
static void d_pia2_firq_a(int state);
static void d_pia2_firq_b(int state);

static int dgnalpha_just_reset;		/* Reset flag used to ignore first NMI after reset */

/* End Dragon Alpha */

/* These sets of defines control logging.  When MAME_DEBUG is off, all logging
 * is off.  There is a different set of defines for when MAME_DEBUG is on so I
 * don't have to worry abount accidently committing a version of the driver
 * with logging enabled after doing some debugging work
 *
 * Logging options marked as "Sparse" are logging options that happen rarely,
 * and should not get in the way.  As such, these options are always on
 * (assuming MAME_DEBUG is on).  "Frequent" options are options that happen
 * enough that they might get in the way.
 */
#define LOG_PAK			0	/* [Sparse]   Logging on PAK trailers */
#define LOG_INT_MASKING	1	/* [Sparse]   Logging on changing GIME interrupt masks */
#define LOG_INT_COCO3	0
#define LOG_GIME		0
#define LOG_MMU			0
#define LOG_TIMER       0
#define LOG_IRQ_RECALC	0

#define GIME_TYPE_1987	0

static void coco3_timer_hblank(void);
static int count_bank(void);
static int is_Orch90(void);

#ifdef MAME_DEBUG
static unsigned coco_dasm_override(int cpunum, char *buffer, unsigned pc);
#endif /* MAME_DEBUG */


/* ----------------------------------------------------------------------- */
/* The bordertop and borderbottom return values are used for calculating
 * field sync timing.  Unfortunately, I cannot seem to find an agreement
 * about how exactly field sync works..
 *
 * What I do know is that FS goes high at the top of the screen, and goes
 * low (forcing an VBORD interrupt) at the bottom of the visual area.
 *
 * Unfortunately, I cannot get a straight answer about how many rows each
 * of the three regions (leading edge --> visible top; visible top -->
 * visible bottom/trailing edge; visible bottom/trailing edge --> leading
 * edge) takes up.  Adding the fact that each of the different LPR
 * settings most likely has a different set of values.  Here is a summary
 * of what I know from different sources:
 *
 * In the January 1987 issue of Rainbow Magazine, there is a program called
 * COLOR3 that uses midframe palette rotation to show all 64 colors on the
 * screen at once.  The first box is at line 32, but it waits for 70 HSYNC
 * transitions before changing
 *
 * SockMaster email: 43/192/28, 41/199/23, 132/0/131, 26/225/12
 * m6847 reference:  38/192/32
 * COLOR3            38/192/32
 *
 * Notes of interest:  Below are some observations of key programs and what
 * they rely on:
 *
 *		COLOR3:
 *			 (fs_pia_flip ? fall_scanline : rise_scanline) = border_top - 32
 */

const struct coco3_video_vars coco3_vidvars =
{
	/* border tops */
	38,	36, 132, 25,

	/* gime flip (hs/fs) */
	0, 0,

	/* pia flip (hs/fs) */
	1, 1,

	/* rise scanline & fall scanline */
	258, 6
};



/* ----------------------------------------------------------------------- */

static const pia6821_interface coco_pia_intf[] =
{
	/* PIA 0 */
	{
		/*inputs : A/B,CA/B1,CA/B2 */ 0, 0, 0, 0, 0, 0,
		/*outputs: A/B,CA/B2	   */ d_pia0_pa_w, d_pia0_pb_w, d_pia0_ca2_w, d_pia0_cb2_w,
		/*irqs	 : A/B			   */ d_pia0_irq_a, d_pia0_irq_b
	},

	/* PIA 1 */
	{
		/*inputs : A/B,CA/B1,CA/B2 */ d_pia1_pa_r, d_pia1_pb_r_coco, 0, d_pia1_cb1_r, 0, 0,
		/*outputs: A/B,CA/B2	   */ d_pia1_pa_w, d_pia1_pb_w, d_pia1_ca2_w, d_pia1_cb2_w,
		/*irqs	 : A/B			   */ d_pia1_firq_a, d_pia1_firq_b
	}
};

static const pia6821_interface coco2_pia_intf[] =
{
	/* PIA 0 */
	{
		/*inputs : A/B,CA/B1,CA/B2 */ 0, 0, 0, 0, 0, 0,
		/*outputs: A/B,CA/B2	   */ d_pia0_pa_w, d_pia0_pb_w, d_pia0_ca2_w, d_pia0_cb2_w,
		/*irqs	 : A/B			   */ d_pia0_irq_a, d_pia0_irq_b
	},

	/* PIA 1 */
	{
		/*inputs : A/B,CA/B1,CA/B2 */ d_pia1_pa_r, d_pia1_pb_r_coco2, 0, d_pia1_cb1_r, 0, 0,
		/*outputs: A/B,CA/B2	   */ d_pia1_pa_w, d_pia1_pb_w, d_pia1_ca2_w, d_pia1_cb2_w,
		/*irqs	 : A/B			   */ d_pia1_firq_a, d_pia1_firq_b
	}
};

static const pia6821_interface coco3_pia_intf[] =
{
	/* PIA 0 */
	{
		/*inputs : A/B,CA/B1,CA/B2 */ 0, d_pia1_pb_r_coco2, 0, 0, 0, 0,
		/*outputs: A/B,CA/B2	   */ d_pia0_pa_w, coco3_pia0_pb_w, d_pia0_ca2_w, d_pia0_cb2_w,
		/*irqs	 : A/B			   */ coco3_pia0_irq_a, coco3_pia0_irq_b
	},

	/* PIA 1 */
	{
		/*inputs : A/B,CA/B1,CA/B2 */ d_pia1_pa_r, 0, 0, d_pia1_cb1_r, 0, 0,
		/*outputs: A/B,CA/B2	   */ d_pia1_pa_w, d_pia1_pb_w, d_pia1_ca2_w, d_pia1_cb2_w,
		/*irqs	 : A/B			   */ coco3_pia1_firq_a, coco3_pia1_firq_b
	}
};

static const pia6821_interface dragon64_pia_intf[] =
{
	/* PIA 0 */
	{
		/*inputs : A/B,CA/B1,CA/B2 */ 0, 0, 0, 0, 0, 0,
		/*outputs: A/B,CA/B2	   */ d_pia0_pa_w, d_pia0_pb_w, d_pia0_ca2_w, d_pia0_cb2_w,
		/*irqs	 : A/B			   */ d_pia0_irq_a, d_pia0_irq_b
	},

	/* PIA 1 */
	{
		/*inputs : A/B,CA/B1,CA/B2 */ d_pia1_pa_r, d_pia1_pb_r_coco, 0, 0, 0, 0,
		/*outputs: A/B,CA/B2	   */ d_pia1_pa_w, dragon64_pia1_pb_w, d_pia1_ca2_w, d_pia1_cb2_w,
		/*irqs	 : A/B			   */ d_pia1_firq_a, d_pia1_firq_b
	}
};

static const pia6821_interface dgnalpha_pia_intf[] =
{
	/* PIA 0 and 1 as Dragon 64 */
	/* PIA 0 */
	{
		/*inputs : A/B,CA/B1,CA/B2 */ 0, 0, 0, 0, 0, 0,
		/*outputs: A/B,CA/B2	   */ d_pia0_pa_w, d_pia0_pb_w, d_pia0_ca2_w, d_pia0_cb2_w,
		/*irqs	 : A/B			   */ d_pia0_irq_a, d_pia0_irq_b
	},

	/* PIA 1 */
	{
		/*inputs : A/B,CA/B1,CA/B2 */ d_pia1_pa_r, d_pia1_pb_r_coco, 0, 0, 0, 0,
		/*outputs: A/B,CA/B2	   */ d_pia1_pa_w, d_pia1_pb_w, d_pia1_ca2_w, d_pia1_cb2_w,
		/*irqs	 : A/B			   */ d_pia1_firq_a, d_pia1_firq_b
	},

	/* PIA 2 */
	{
		/*inputs : A/B,CA/B1,CA/B2 */ 0, 0, 0, 0, 0, 0,
		/*outputs: A/B,CA/B2	   */ dgnalpha_pia2_pa_w, 0, 0, 0,
		/*irqs	 : A/B	   		   */ d_pia2_firq_a, d_pia2_firq_b
	}

};

static const sam6883_interface coco_sam_intf =
{
	SAM6883_ORIGINAL,
	NULL,
	d_sam_set_pageonemode,
	d_sam_set_mpurate,
	d_sam_set_memorysize,
	d_sam_set_maptype
};

static const sam6883_interface coco3_sam_intf =
{
	SAM6883_GIME,
	coco3_sam_get_rambase,
	NULL,
	d_sam_set_mpurate,
	NULL,
	coco3_sam_set_maptype
};

static const sam6883_interface dragon64_sam_intf =
{
	SAM6883_ORIGINAL,
	NULL,
	d_sam_set_pageonemode,
	d_sam_set_mpurate,
	d_sam_set_memorysize,
	dragon64_sam_set_maptype
};

static const struct cartridge_slot *coco_cart_interface;

/***************************************************************************
  PAK files

  PAK files were originally for storing Program PAKs, but the file format has
  evolved into a snapshot file format, with a file format so convoluted and
  changing to make it worthy of Microsoft.
***************************************************************************/

static int load_pak_into_region(mame_file *fp, int *pakbase, int *paklen, UINT8 *mem, int segaddr, int seglen)
{
	if (*paklen)
	{
		if (*pakbase < segaddr)
		{
			/* We have to skip part of the PAK file */
			int skiplen;

			skiplen = segaddr - *pakbase;
			if (mame_fseek(fp, skiplen, SEEK_CUR))
			{
				if (LOG_PAK)
					logerror("Could not fully read PAK.\n");
				return 1;
			}

			*pakbase += skiplen;
			*paklen -= skiplen;
		}

		if (*pakbase < segaddr + seglen)
		{
			mem += *pakbase - segaddr;
			seglen -= *pakbase - segaddr;

			if (seglen > *paklen)
				seglen = *paklen;

			if (mame_fread(fp, mem, seglen) < seglen)
			{
				if (LOG_PAK)
					logerror("Could not fully read PAK.\n");
				return 1;
			}

			*pakbase += seglen;
			*paklen -= seglen;
		}
	}
	return 0;
}

static void pak_load_trailer(const pak_decodedtrailer *trailer)
{
	cpunum_set_reg(0, M6809_PC, trailer->reg_pc);
	cpunum_set_reg(0, M6809_X, trailer->reg_x);
	cpunum_set_reg(0, M6809_Y, trailer->reg_y);
	cpunum_set_reg(0, M6809_U, trailer->reg_u);
	cpunum_set_reg(0, M6809_S, trailer->reg_s);
	cpunum_set_reg(0, M6809_DP, trailer->reg_dp);
	cpunum_set_reg(0, M6809_B, trailer->reg_b);
	cpunum_set_reg(0, M6809_A, trailer->reg_a);
	cpunum_set_reg(0, M6809_CC, trailer->reg_cc);

	/* I seem to only be able to get a small amount of the PIA state from the
	 * snapshot trailers. Thus I am going to configure the PIA myself. The
	 * following PIA writes are the same thing that the CoCo ROM does on
	 * startup. I wish I had a better solution
	 */

	pia_write(0, 1, 0x00);
	pia_write(0, 3, 0x00);
	pia_write(0, 0, 0x00);
	pia_write(0, 2, 0xff);
	pia_write(1, 1, 0x00);
	pia_write(1, 3, 0x00);
	pia_write(1, 0, 0xfe);
	pia_write(1, 2, 0xf8);

	pia_write(0, 1, trailer->pia[1]);
	pia_write(0, 0, trailer->pia[0]);
	pia_write(0, 3, trailer->pia[3]);
	pia_write(0, 2, trailer->pia[2]);

	pia_write(1, 1, trailer->pia[5]);
	pia_write(1, 0, trailer->pia[4]);
	pia_write(1, 3, trailer->pia[7]);
	pia_write(1, 2, trailer->pia[6]);

	/* For some reason, specifying use of high ram seems to screw things up;
	 * I'm not sure whether it is because I'm using the wrong method to get
	 * access that bit or or whether it is something else.  So that is why
	 * I am specifying 0x7fff instead of 0xffff here
	 */
	sam_set_state(trailer->sam, 0x7fff);
}

static int generic_pak_load(mame_file *fp, int rambase_index, int rombase_index, int pakbase_index)
{
	UINT8 *ROM;
	UINT8 *rambase;
	UINT8 *rombase;
	UINT8 *pakbase;
	int paklength;
	int pakstart;
	pak_header header;
	int trailerlen;
	UINT8 trailerraw[500];
	pak_decodedtrailer trailer;
	int trailer_load = 0;

	ROM = memory_region(REGION_CPU1);
	rambase = &mess_ram[rambase_index];
	rombase = &ROM[rombase_index];
	pakbase = &ROM[pakbase_index];

	if (mess_ram_size < 0x10000)
	{
		if (LOG_PAK)
			logerror("Cannot load PAK files without at least 64k.\n");
		return INIT_FAIL;
	}

	if (mame_fread(fp, &header, sizeof(header)) < sizeof(header))
	{
		if (LOG_PAK)
			logerror("Could not fully read PAK.\n");
		return INIT_FAIL;
	}

	paklength = header.length ? LITTLE_ENDIANIZE_INT16(header.length) : 0x10000;
	pakstart = LITTLE_ENDIANIZE_INT16(header.start);
	if (pakstart == 0xc000)
		cart_inserted = 1;

	if (mame_fseek(fp, paklength, SEEK_CUR))
	{
		if (LOG_PAK)
			logerror("Could not fully read PAK.\n");
		return INIT_FAIL;
	}

	trailerlen = mame_fread(fp, trailerraw, sizeof(trailerraw));
	if (trailerlen)
	{
		if (pak_decode_trailer(trailerraw, trailerlen, &trailer))
		{
			if (LOG_PAK)
				logerror("Invalid or unknown PAK trailer.\n");
			return INIT_FAIL;
		}

		trailer_load = 1;
	}

	if (mame_fseek(fp, sizeof(pak_header), SEEK_SET))
	{
		if (LOG_PAK)
			logerror("Unexpected error while reading PAK.\n");
		return INIT_FAIL;
	}

	/* Now that we are done reading the trailer; we can cap the length */
	if (paklength > 0xff00)
		paklength = 0xff00;

	/* PAK files reflect the fact that JeffV's emulator did not appear to
	 * differentiate between RAM and ROM memory.  So what we do when a PAK
	 * loads is to copy the ROM into RAM, load the PAK into RAM, and then
	 * copy the part of RAM corresponding to PAK ROM to the actual PAK ROM
	 * area.
	 *
	 * It is ugly, but it reflects the way that JeffV's emulator works
	 */

	memcpy(rambase + 0x8000, rombase, 0x4000);
	memcpy(rambase + 0xC000, pakbase, 0x3F00);

	/* Get the RAM portion */
	if (load_pak_into_region(fp, &pakstart, &paklength, rambase, 0x0000, 0xff00))
		return INIT_FAIL;

	memcpy(pakbase, rambase + 0xC000, 0x3F00);

	if (trailer_load)
		pak_load_trailer(&trailer);
	return INIT_PASS;
}

SNAPSHOT_LOAD ( coco_pak )
{
	return generic_pak_load(fp, 0x0000, 0x0000, 0x4000);
}

SNAPSHOT_LOAD ( coco3_pak )
{
	return generic_pak_load(fp, (0x70000 % mess_ram_size), 0x0000, 0xc000);
}

/***************************************************************************
  ROM files

  ROM files are simply raw dumps of cartridges.  I believe that they should
  be used in place of PAK files, when possible
***************************************************************************/

static int generic_rom_load(mess_image *img, mame_file *fp, UINT8 *dest, UINT16 destlength)
{
	UINT8 *rombase;
	int   romsize;

	romsize = mame_fsize(fp);

	/* The following hack is for Arkanoid running on the CoCo2.
		The issuse is the CoCo2 hardware only allows the cartridge
		interface to access 0xC000-0xFEFF (16K). The Arkanoid ROM is
		32K starting at 0x8000. The first 16K is totally inaccessable
		from a CoCo2. Thus we need to skip ahead in the ROM file. On
		the CoCo3 the entire 32K ROM is accessable. */

	if (image_crc(img) == 0x25C3AA70)     /* Test for Arkanoid  */
	{
		if ( destlength == 0x4000 )						/* Test if CoCo2      */
		{
			mame_fseek( fp, 0x4000, SEEK_SET );			/* Move ahead in file */
			romsize -= 0x4000;							/* Adjust ROM size    */
		}
	}

	if (romsize > destlength)
		romsize = destlength;

	mame_fread(fp, dest, romsize);

	cart_inserted = 1;

	/* Now we need to repeat the mirror the ROM throughout the ROM memory */
	rombase = dest;
	dest += romsize;
	destlength -= romsize;
	while(destlength > 0)
	{
		if (romsize > destlength)
			romsize = destlength;
		memcpy(dest, rombase, romsize);
		dest += romsize;
		destlength -= romsize;
	}
	return INIT_PASS;
}

DEVICE_LOAD(coco_rom)
{
	UINT8 *ROM = memory_region(REGION_CPU1);
	return generic_rom_load(image, file, &ROM[0x4000], 0x4000);
}

DEVICE_UNLOAD(coco_rom)
{
	UINT8 *ROM = memory_region(REGION_CPU1);
	memset(&ROM[0x4000], 0, 0x4000);
	
	cart_inserted = 0;	/* Flag cart no longer inserted */
}

DEVICE_LOAD(coco3_rom)
{
	UINT8 	*ROM = memory_region(REGION_CPU1);
	int		count;

	count = count_bank();
	if (file)
		mame_fseek(file, 0, SEEK_SET);

	if( count == 0 )
	{
		/* Load roms starting at 0x8000 and mirror upwards. */
		/* ROM size is 32K max */
		return generic_rom_load(image, file, &ROM[0x8000], 0x8000);
	}
	else
	{
		/* Load roms starting at 0x8000 and mirror upwards. */
		/* ROM bank is 16K max */
		return generic_rom_load(image, file, &ROM[0x8000], 0x4000);
	}
}

DEVICE_UNLOAD(coco3_rom)
{
	UINT8 *ROM = memory_region(REGION_CPU1);
	memset(&ROM[0x8000], 0, 0x8000);
}

/***************************************************************************
  Interrupts

  The Dragon/CoCo2 have two PIAs.  These PIAs can trigger interrupts.  PIA0
  is set up to trigger IRQ on the CPU, and PIA1 can trigger FIRQ.  Each PIA
  has two output lines, and an interrupt will be triggered if either of these
  lines are asserted.

  -----  IRQ
  6809 |-<----------- PIA0
       |
       |
       |
       |
       |
       |-<----------- PIA1
  -----

  The CoCo 3 still supports these interrupts, but the GIME can chose whether
  "old school" interrupts are generated, or the new ones generated by the GIME

  -----  IRQ
  6809 |-<----------- PIA0
       |       |                ------
       |       -<-------<-------|    |
       |                        |GIME|
       |       -<-------<-------|    |
       | FIRQ  |                ------
       |-<----------- PIA1
  -----

  In an email discussion with JK, he informs me that when GIME interrupts are
  enabled, this actually does not prevent PIA interrupts.  Apparently JeffV's
  CoCo 3 emulator did not handle this properly.
***************************************************************************/

enum
{
	COCO3_INT_TMR	= 0x20,		/* Timer */
	COCO3_INT_HBORD	= 0x10,		/* Horizontal border sync */
	COCO3_INT_VBORD	= 0x08,		/* Vertical border sync */
	COCO3_INT_EI2	= 0x04,		/* Serial data */
	COCO3_INT_EI1	= 0x02,		/* Keyboard */
	COCO3_INT_EI0	= 0x01,		/* Cartridge */

	COCO3_INT_ALL	= 0x3f
};

static int is_cpu_suspended(void)
{
	return !cpunum_is_suspended(0, SUSPEND_REASON_HALT | SUSPEND_REASON_RESET | SUSPEND_REASON_DISABLE);
}

static void d_recalc_irq(void)
{
	UINT8 pia0_irq_a = pia_get_irq_a(0);
	UINT8 pia0_irq_b = pia_get_irq_b(0);

	if ((pia0_irq_a || pia0_irq_b) && is_cpu_suspended())
		cpunum_set_input_line(0, M6809_IRQ_LINE, ASSERT_LINE);
	else
		cpunum_set_input_line(0, M6809_IRQ_LINE, CLEAR_LINE);
}

static void d_recalc_firq(void)
{
	UINT8 pia1_firq_a = pia_get_irq_a(1);
	UINT8 pia1_firq_b = pia_get_irq_b(1);
	UINT8 pia2_firq_a = pia_get_irq_a(2);
	UINT8 pia2_firq_b = pia_get_irq_b(2);

	if ((pia1_firq_a || pia1_firq_b ||
	     pia2_firq_a || pia2_firq_b) && is_cpu_suspended())
		cpunum_set_input_line(0, M6809_FIRQ_LINE, ASSERT_LINE);
	else
		cpunum_set_input_line(0, M6809_FIRQ_LINE, CLEAR_LINE);
}

static void coco3_recalc_irq(void)
{
	if ((coco3_gimereg[0] & 0x20) && gime_irq && is_cpu_suspended())
		cpunum_set_input_line(0, M6809_IRQ_LINE, ASSERT_LINE);
	else
		d_recalc_irq();
}

static void coco3_recalc_firq(void)
{
	if ((coco3_gimereg[0] & 0x10) && gime_firq && is_cpu_suspended())
		cpunum_set_input_line(0, M6809_FIRQ_LINE, ASSERT_LINE);
	else
		d_recalc_firq();
}

static void d_pia0_irq_a(int state)
{
	d_recalc_irq();
}

static void d_pia0_irq_b(int state)
{
	d_recalc_irq();
}

static void d_pia1_firq_a(int state)
{
	d_recalc_firq();
}

static void d_pia1_firq_b(int state)
{
	d_recalc_firq();
}

/* Dragon Alpha second PIA IRQ lines also cause FIRQ */
static void d_pia2_firq_a(int state)
{
	d_recalc_firq();
}

static void d_pia2_firq_b(int state)
{
	d_recalc_firq();
}

static void coco3_pia0_irq_a(int state)
{
	coco3_recalc_irq();
}

static void coco3_pia0_irq_b(int state)
{
	coco3_recalc_irq();
}

static void coco3_pia1_firq_a(int state)
{
	coco3_recalc_firq();
}

static void coco3_pia1_firq_b(int state)
{
	coco3_recalc_firq();
}

static void coco3_raise_interrupt(UINT8 mask, int state)
{
	int lowtohigh;

	lowtohigh = state && ((coco3_interupt_line & mask) == 0);

	if (state)
		coco3_interupt_line |= mask;
	else
		coco3_interupt_line &= ~mask;

	if (lowtohigh)
	{
		if ((coco3_gimereg[0] & 0x20) && (coco3_gimereg[2] & mask))
		{
			gime_irq |= (coco3_gimereg[2] & mask);
			coco3_recalc_irq();

			if (LOG_INT_COCO3)
				logerror("CoCo3 Interrupt: Raising IRQ; scanline=%i\n", cpu_getscanline());
		}
		if ((coco3_gimereg[0] & 0x10) && (coco3_gimereg[3] & mask))
		{
			gime_firq |= (coco3_gimereg[3] & mask);
			coco3_recalc_firq();

			if (LOG_INT_COCO3)
				logerror("CoCo3 Interrupt: Raising FIRQ; scanline=%i\n", cpu_getscanline());
		}
	}
}



void coco3_horizontal_sync_callback(int data)
{
	pia_0_ca1_w(0, data);
	coco3_raise_interrupt(COCO3_INT_HBORD, data);
}



void coco3_field_sync_callback(int data)
{
	pia_0_cb1_w(0, data);
}



void coco3_gime_field_sync_callback(void)
{
	/* the CoCo 3 VBORD interrupt triggers right after the display */
	coco3_raise_interrupt(COCO3_INT_VBORD, 1);
	coco3_raise_interrupt(COCO3_INT_VBORD, 0);
}



/***************************************************************************
  Halt line
***************************************************************************/

static void d_recalc_interrupts(int dummy)
{
	d_recalc_firq();
	d_recalc_irq();
}

static void coco3_recalc_interrupts(int dummy)
{
	coco3_recalc_firq();
	coco3_recalc_irq();
}

static void (*recalc_interrupts)(int dummy);

void coco_set_halt_line(int halt_line)
{
	cpunum_set_input_line(0, INPUT_LINE_HALT, halt_line);
	if (halt_line == CLEAR_LINE)
		timer_set(TIME_IN_CYCLES(1,0), 0, recalc_interrupts);
}


/***************************************************************************
  Joystick Abstractions
***************************************************************************/

#define JOYSTICK_RIGHT_X	7
#define JOYSTICK_RIGHT_Y	8
#define JOYSTICK_LEFT_X		9
#define JOYSTICK_LEFT_Y		10

static double read_joystick(int joyport)
{
	return readinputport(joyport) / 255.0;
}


typedef enum
{
	JOYSTICKMODE_NORMAL			= 0x00,
	JOYSTICKMODE_HIRES			= 0x10,
	JOYSTICKMODE_HIRES_CC3MAX	= 0x30,
	JOYSTICKMODE_RAT			= 0x20
} joystick_mode_t;

static joystick_mode_t joystick_mode(void)
{
	return (joystick_mode_t) (int) readinputportbytag_safe("joystick_mode", JOYSTICKMODE_NORMAL);
}



/***************************************************************************
  Hires Joystick
***************************************************************************/

static int coco_hiresjoy_ca = 1;
static double coco_hiresjoy_xtransitiontime = 0;
static double coco_hiresjoy_ytransitiontime = 0;

static double coco_hiresjoy_computetransitiontime(int inputport)
{
	double val;

	val = read_joystick(inputport);

	if (joystick_mode() == JOYSTICKMODE_HIRES_CC3MAX)
	{
		/* CoCo MAX 3 Interface */
		val = val * 2500.0 + 400.0;
	}
	else
	{
		/* Normal Hires Interface */
		val = val * 4160.0 + 592.0;
	}

	return timer_get_time() + (COCO_CPU_SPEED * val);
}

static void coco_hiresjoy_w(int data)
{
	if (!data && coco_hiresjoy_ca) {
		/* Hi to lo */
		coco_hiresjoy_xtransitiontime = coco_hiresjoy_computetransitiontime(JOYSTICK_RIGHT_X);
		coco_hiresjoy_ytransitiontime = coco_hiresjoy_computetransitiontime(JOYSTICK_RIGHT_Y);
	}
	else if (data && !coco_hiresjoy_ca) {
		/* Lo to hi */
		coco_hiresjoy_ytransitiontime = 0;
		coco_hiresjoy_ytransitiontime = 0;
	}
	coco_hiresjoy_ca = data;
}

static int coco_hiresjoy_readone(double transitiontime)
{
	return (transitiontime) ? (timer_get_time() >= transitiontime) : 1;
}

static int coco_hiresjoy_rx(void)
{
	return coco_hiresjoy_readone(coco_hiresjoy_xtransitiontime);
}

static int coco_hiresjoy_ry(void)
{
	return coco_hiresjoy_readone(coco_hiresjoy_ytransitiontime);
}

/***************************************************************************
  Sound MUX

  The sound MUX has 4 possible settings, depend on SELA and SELB inputs:

  00	- DAC (digital - analog converter)
  01	- CSN (cassette)
  10	- SND input from cartridge (NYI because we only support the FDC)
  11	- Grounded (0)

  Source - Tandy Color Computer Service Manual
***************************************************************************/

#define SOUNDMUX_STATUS_ENABLE	4
#define SOUNDMUX_STATUS_SEL2	2
#define SOUNDMUX_STATUS_SEL1	1

static mess_image *cartslot_image(void)
{
	return image_from_devtype_and_index(IO_CARTSLOT, 0);
}

static mess_image *cassette_device_image(void)
{
	return image_from_devtype_and_index(IO_CASSETTE, 0);
}

static mess_image *bitbanger_image(void)
{
	return image_from_devtype_and_index(IO_BITBANGER, 0);
}

static mess_image *printer_image(void)
{
	return image_from_devtype_and_index(IO_PRINTER, 0);
}

static int get_soundmux_status(void)
{
	int soundmux_status = 0;
	if (pia_get_output_cb2(1))
		soundmux_status |= SOUNDMUX_STATUS_ENABLE;
	if (pia_get_output_ca2(0))
		soundmux_status |= SOUNDMUX_STATUS_SEL1;
	if (pia_get_output_cb2(0))
		soundmux_status |= SOUNDMUX_STATUS_SEL2;
	return soundmux_status;
}

static void soundmux_update(void)
{
	/* This function is called whenever the MUX (selector switch) is changed
	 * It mainly turns on and off the cassette audio depending on the switch.
	 * It also calls a function into the cartridges device to tell it if it is
	 * switch on or off.
	 */
	cassette_state new_state;
	int soundmux_status = get_soundmux_status();

	switch(soundmux_status) {
	case SOUNDMUX_STATUS_ENABLE | SOUNDMUX_STATUS_SEL1:
		/* CSN */
		new_state = CASSETTE_SPEAKER_ENABLED;
		break;
	default:
		new_state = CASSETTE_SPEAKER_MUTED;
		break;
	}

	coco_cartridge_enablesound(soundmux_status == (SOUNDMUX_STATUS_ENABLE|SOUNDMUX_STATUS_SEL2));
	cassette_change_state(cassette_device_image(), new_state, CASSETTE_MASK_SPEAKER);
}

static void coco_sound_update(void)
{
	/* Call this function whenever you need to update the sound. It will
	 * automatically mute any devices that are switched out.
	 */
	UINT8 dac = pia_get_output_a(1) & 0xFC;
	UINT8 pia1_pb1 = pia_get_output_b(1) & 0x02;
	int soundmux_status = get_soundmux_status();

	if (soundmux_status & SOUNDMUX_STATUS_ENABLE)
	{
		switch(soundmux_status) {
		case SOUNDMUX_STATUS_ENABLE:
			/* DAC */
			DAC_data_w(0, pia1_pb1 + (dac >> 1) );  /* Mixing the two sources */
			break;
		case SOUNDMUX_STATUS_ENABLE | SOUNDMUX_STATUS_SEL1:
			/* CSN */
			DAC_data_w(0, pia1_pb1); /* Mixing happens elsewhere */
			break;
		case SOUNDMUX_STATUS_ENABLE | SOUNDMUX_STATUS_SEL2:
			/* CART Sound */
			DAC_data_w(0, pia1_pb1); /* To do: mix in cart signal */
			break;
		default:
			/* This pia line is always connected to the output */
			DAC_data_w(0, pia1_pb1);
			break;
		}
	}
}

READ8_HANDLER ( dgnalpha_psg_porta_read )
{	
	return 0;
}

WRITE8_HANDLER ( dgnalpha_psg_porta_write )
{
	/* Bits 0..3 are the drive select lines for the internal floppy interface */
	/* Bit 4 is the motor on, in the real hardware these are inverted on their way to the drive */
	/* Bits 5,6,7 are connected to /DDEN, ENP and 5/8 on the WD2797 */ 
	switch (data & 0xF)
	{
		case(0x01) :
			wd179x_set_drive(0);
			break;
		case(0x02) :
			wd179x_set_drive(1);
			break;
		case(0x04) :
			wd179x_set_drive(2);
			break;
		case(0x08) :
			wd179x_set_drive(3);
			break;
	}
}

/***************************************************************************
  PIA0 ($FF00-$FF1F) (Chip U8)

  PIA0 PA0-PA7	- Keyboard/Joystick read
  PIA0 PB0-PB7	- Keyboard write
  PIA0 CA1		- M6847 HS (Horizontal Sync)
  PIA0 CA2		- SEL1 (Used by sound mux and joystick)
  PIA0 CB1		- M6847 FS (Field Sync)
  PIA0 CB2		- SEL2 (Used by sound mux and joystick)
***************************************************************************/

static WRITE8_HANDLER ( d_pia0_ca2_w )
{
	soundmux_update();
}



static WRITE8_HANDLER ( d_pia0_cb2_w )
{
	soundmux_update();
}



static UINT8 coco_update_keyboard(void)
{
	UINT8 porta = 0x7F;
	int joyport;
	const char *joyport_tag;
	int joyval;
	static const int joy_rat_table[] = {15, 24, 42, 33 };
	UINT8 pia0_pb;
	UINT8 dac = pia_get_output_a(1) & 0xFC;
	int joystick_axis, joystick;
	
	pia0_pb = pia_get_output_b(0);
	joystick_axis = pia_get_output_ca2(0);
	joystick = pia_get_output_cb2(0);

	if ((input_port_0_r(0) | pia0_pb) != 0xff) porta &= ~0x01;
	if ((input_port_1_r(0) | pia0_pb) != 0xff) porta &= ~0x02;
	if ((input_port_2_r(0) | pia0_pb) != 0xff) porta &= ~0x04;
	if ((input_port_3_r(0) | pia0_pb) != 0xff) porta &= ~0x08;
	if ((input_port_4_r(0) | pia0_pb) != 0xff) porta &= ~0x10;
	if ((input_port_5_r(0) | pia0_pb) != 0xff) porta &= ~0x20;
	if ((input_port_6_r(0) | pia0_pb) != 0xff) porta &= ~0x40;

	if (joystick_mode() == JOYSTICKMODE_RAT)
	{
		/* The RAT graphic mouse */
		joyport_tag = joystick_axis ? "rat_mouse_y" : "rat_mouse_x";
		joyval = readinputportbytag(joyport_tag);

		if ((dac >> 2) <= joy_rat_table[joyval])
			porta |= 0x80;
	}
	else if (!joystick && (joystick_mode() != JOYSTICKMODE_NORMAL))
	{
		/* Hi res joystick */
		if (joystick_axis ? coco_hiresjoy_ry() : coco_hiresjoy_rx())
			porta |= 0x80;
	}
	else
	{
		/* Normal joystick */
		joyport = joystick ? (joystick_axis ? JOYSTICK_LEFT_Y : JOYSTICK_LEFT_X) : (joystick_axis ? JOYSTICK_RIGHT_Y : JOYSTICK_RIGHT_X);
		joyval = read_joystick(joyport) * 64.0;

		if ((dac >> 2) <= joyval)
			porta |= 0x80;
	}

	porta &= ~readinputport(11);
	
	pia_set_input_a(0, porta);
	return porta;
}



static UINT8 coco3_update_keyboard(void)
{
	UINT8 porta;
	porta = coco_update_keyboard();
	coco3_raise_interrupt(COCO3_INT_EI1, ((porta & 0x7F) == 0x7F) ? CLEAR_LINE : ASSERT_LINE);
	return porta;
}



static WRITE8_HANDLER ( d_pia0_pb_w )		{ coco_update_keyboard(); }
static WRITE8_HANDLER ( coco3_pia0_pb_w )	{ coco3_update_keyboard(); }

static void coco_poll_keyboard(void *param, UINT32 value, UINT32 mask)
{
	coco_update_keyboard();
}

static void coco3_poll_keyboard(void *param, UINT32 value, UINT32 mask)
{
	coco3_update_keyboard();
}



static WRITE8_HANDLER ( d_pia0_pa_w )
{
	if (joystick_mode() == JOYSTICKMODE_HIRES_CC3MAX)
		coco_hiresjoy_w(data & 0x04);
}



/* The following hacks are necessary because a large portion of cartridges
 * tie the Q line to the CART line.  Since Q pulses with every single clock
 * cycle, this would be prohibitively slow to emulate.  Thus we are only
 * going to pulse when the PIA is read from; which seems good enough (for now)
 */
READ8_HANDLER( coco_pia_1_r )
{
	if (cart_line == CARTLINE_Q)
	{
		coco_setcartline(CARTLINE_CLEAR);
		coco_setcartline(CARTLINE_Q);
	}
	return pia_1_r(offset);
}

READ8_HANDLER( coco3_pia_1_r )
{
	if (cart_line == CARTLINE_Q)
	{
		coco3_setcartline(CARTLINE_CLEAR);
		coco3_setcartline(CARTLINE_Q);
	}
	return pia_1_r(offset);
}

/***************************************************************************
  PIA1 ($FF20-$FF3F) (Chip U4)

  PIA1 PA0		- CASSDIN
  PIA1 PA1		- RS232 OUT (CoCo), Printer Strobe (Dragon)
  PIA1 PA2-PA7	- DAC
  PIA1 PB0		- RS232 IN
  PIA1 PB1		- Single bit sound
  PIA1 PB2		- RAMSZ (32/64K, 16K, and 4K three position switch)
  PIA1 PB3		- M6847 CSS
  PIA1 PB4		- M6847 INT/EXT and M6847 GM0
  PIA1 PB5		- M6847 GM1
  PIA1 PB6		- M6847 GM2
  PIA1 PB7		- M6847 A/G
  PIA1 CA1		- CD (Carrier Detect; NYI)
  PIA1 CA2		- CASSMOT (Cassette Motor)
  PIA1 CB1		- CART (Cartridge Detect)
  PIA1 CB2		- SNDEN (Sound Enable)
***************************************************************************/

static  READ8_HANDLER ( d_pia1_cb1_r )
{
	return cart_line ? ASSERT_LINE : CLEAR_LINE;
}

static WRITE8_HANDLER ( d_pia1_cb2_w )
{
	soundmux_update();
}

static WRITE8_HANDLER ( d_pia1_pa_w )
{
	/*
	 *	This port appears at $FF20
	 *
	 *	Bits
	 *  7-2:	DAC to speaker or cassette
	 *    1:	Serial out (CoCo), Printer strobe (Dragon)
	 */
	UINT8 dac = pia_get_output_a(1) & 0xFC;

	coco_sound_update();
	coco_update_keyboard();

	if (joystick_mode() == JOYSTICKMODE_HIRES)
		coco_hiresjoy_w(dac >= 0x80);
	else
		cassette_output(cassette_device_image(), ((int) dac - 0x80) / 128.0);

	switch(coco_or_dragon)
	{
		case AM_COCO:
			/* Only the coco has a bitbanger */
			bitbanger_output(bitbanger_image(), (data & 2) >> 1);
			break;
	
		case AM_DRAGON:
			/* If strobe bit is high send data from pia0 port b to dragon parallel printer */
			if (data & 0x02)
			{
				printer_output(printer_image(), pia_get_output_b(0));
			}
			break;
	}
}

/*
 * This port appears at $FF23
 *
 * The CoCo 1/2 and Dragon kept the gmode and vmode separate, and the CoCo
 * 3 tied them together.  In the process, the CoCo 3 dropped support for the
 * semigraphics modes
 */

static WRITE8_HANDLER( d_pia1_pb_w )
{
	m6847_video_changed();

	/* PB1 will drive the sound output.  This is a rarely
	 * used single bit sound mode. It is always connected thus
	 * cannot be disabled.
	 *
	 * Source:  Page 31 of the Tandy Color Computer Serice Manual
	 */
	coco_sound_update();
}

enum
{
	DRAGON64_SAMMAP = 1,
	DRAGON64_PIAMAP = 2,
	DRAGON64_ALL = 3
};

static WRITE8_HANDLER( dragon64_pia1_pb_w )
{
	d_pia1_pb_w(0, data);

    /* If bit 2 of the pia1 ddra is 1 then this pin is an output so use it */
	/* to control the paging of the 32k and 64k basic roms */
	/* Otherwise it set as an input, with an internal pull-up so it should */
	/* always be high (enabling 32k basic rom) */
	if (pia_get_ddr_b(1) & 0x04)
	{
		dragon_page_rom(data & 0x04);
	}	
}

/***************************************************************************
  PIA2 ($FF24-$FF28) on Daragon Alpha/Professional

	PIA2 PA0		bcdir to AY-8912
	PIA2 PA1		bc0	to AY-8912
	PIA2 PA2		Rom switch, 0=basic rom, 1=boot rom.
	PIA2 PA3-PA7	Unknown/unused ?
	PIA2 PB0-PB7	connected to D0..7 of the AY8912.
	CB1				DRQ from WD2797 disk controler.
***************************************************************************/
  
static WRITE8_HANDLER( dgnalpha_pia2_pa_w )
{
	int	bc_flags;		/* BCDDIR/BC1, as connected to PIA2 port a bits 0 and 1 */

	/* If bit 2 of the pia2 ddra is 1 then this pin is an output so use it */
	/* to control the paging of the boot and basic roms */
	/* Otherwise it set as an input, with an internal pull-up so it should */
	/* always be high (enabling boot rom) */
	if (pia_get_ddr_a(2) & 0x04)
	{
		dragon_page_rom(data & 0x04);	/* bit 2 controls boot or basic rom */
	}
	
	/* Bits 0 and 1 for pia2 port a control the BCDIR and BC1 lines of the */
	/* AY-8912 */
	bc_flags = data & 0x03;	/* mask out bits */
	
	switch (bc_flags)
	{
		case 0x00	: 		/* Inactive, do nothing */
			break;			
		case 0x01	: 		/* Write to selected port */
			AY8910_write_port_0_w(0, pia_get_output_b(2));
			break;
		case 0x02	: 		/* Read from selected port */
			pia_set_input_b(2, AY8910_read_port_0_r(0));
			break;
		case 0x03	:		/* Select port to write to */
			AY8910_control_port_0_w(0, pia_get_output_b(2));
			break;
	}
}

/* Controls rom paging in Dragon 64, and Dragon Alpha */
/* On 64, switches between the two versions of the basic rom mapped in at 0x8000 */
/* on the alpha switches between the Boot/Diagnostic rom and the basic rom */
static void dragon_page_rom(int	romswitch)
{
	UINT8 *bank;
	
	if (romswitch) 
		bank = coco_rom;			/* This is the 32k mode basic(64)/boot rom(alpha)  */
	else
		bank = coco_rom + 0x8000;	/* This is the 64k mode basic(64)/basic rom(alpha) */
	
	dragon_rom_bank = bank;			/* Record which rom we are using so that the irq routine */
									/* uses the vectors from the correct rom ! (alpha) */

	dragon64_sam_set_maptype(dragon_map_type);	/* Call to setup correct mapping */
}

/********************************************************************************************/
/* Dragon Alpha onboard FDC */
/********************************************************************************************/

static void	dgnalpha_fdc_callback(int event)
{
	/* As far as I can tell, the NMI just goes straight onto the processor line on the alpha */
	/* The DRQ line goes through pia2 cb1, in exactly the same way as DRQ from DragonDos does */
	/* for pia1 cb1 */
	switch(event) 
	{
		case WD179X_IRQ_CLR:
			cpunum_set_input_line(0, INPUT_LINE_NMI, CLEAR_LINE);
			break;
		case WD179X_IRQ_SET:
		    if(dgnalpha_just_reset)
				dgnalpha_just_reset=0;
			else
			{
				if (pia_2_ca2_r(0)) 
					cpunum_set_input_line(0, INPUT_LINE_NMI, ASSERT_LINE);
			}
			break;
		case WD179X_DRQ_CLR:
			pia_2_cb1_w(0,CARTLINE_CLEAR);
			break;
		case WD179X_DRQ_SET:
			pia_2_cb1_w(0,CARTLINE_ASSERTED);
			break;
	}
}

/* The Dragon Alpha hardware reverses the order of the WD2797 registers */
READ8_HANDLER(wd2797_r)
{
	int result = 0;

	switch(offset & 0x03) 
	{
		case 0:
			result = wd179x_data_r(0);
			break;
		case 1:
			result = wd179x_sector_r(0);
			break;
		case 2:
			result = wd179x_track_r(0);
			break;
		case 3:
			result = wd179x_status_r(0);
			break;
		default:
			break;
	}
		
	return result;
}

WRITE8_HANDLER(wd2797_w)
{
    switch(offset & 0x3) 
	{
		case 0:
			wd179x_data_w(0, data);
			break;
		case 1:
			wd179x_sector_w(0, data);
			break;
		case 2:
			wd179x_track_w(0, data);
			break;
		case 3:
			wd179x_command_w(0, data);

			/* disk head is encoded in the command byte */
			wd179x_set_side((data & 0x02) ? 1 : 0);
			break;
	};
}



static WRITE8_HANDLER ( d_pia1_ca2_w )
{
	cassette_change_state(
		cassette_device_image(),
		data ? CASSETTE_MOTOR_ENABLED : CASSETTE_MOTOR_DISABLED,
		CASSETTE_MASK_MOTOR);
}



static READ8_HANDLER ( d_pia1_pa_r )
{
	return (cassette_input(cassette_device_image()) >= 0) ? 1 : 0;
}



static READ8_HANDLER ( d_pia1_pb_r_coco )
{
	/* This handles the reading of the memory sense switch (pb2) for the Dragon and CoCo 1,
	 * on the CoCo serial-in (pb0). Serial-in not yet implemented.
	 * on the Dragon pb0, is the printer /busy line.
	 */
	int result;

	if (mess_ram_size >= 0x8000)
		result = (pia_get_output_b(0) & 0x80) >> 5;
	else if (mess_ram_size >= 0x4000)
		result = 0x04;
	else
		result = 0x00;
			
	return result;
}



static READ8_HANDLER ( d_pia1_pb_r_coco2 )
{
	/* This handles the reading of the memory sense switch (pb2) for the CoCo 2 and 3,
	 * and serial-in (pb0). Serial-in not yet implemented.
	 */
	int result;

	if (mess_ram_size <= 0x1000)
		result = 0x00;					/* 4K: wire pia1_pb2 low */
	else if (mess_ram_size <= 0x4000)
		result = 0x04;					/* 16K: wire pia1_pb2 high */
	else
		result = (pia_get_output_b(0) & 0x40) >> 4;		/* 32/64K: wire output of pia0_pb6 to input pia1_pb2  */
	return result;
}



/***************************************************************************
  Misc
***************************************************************************/

static void d_sam_set_mpurate(int val)
{
	/* The infamous speed up poke.
	 *
	 * This was a SAM switch that occupied 4 addresses:
	 *
	 *		$FFD9	(set)	R1
	 *		$FFD8	(clear)	R1
	 *		$FFD7	(set)	R0
	 *		$FFD6	(clear)	R0
	 *
	 * R1:R0 formed the following states:
	 *		00	- slow          0.89 MHz
	 *		01	- dual speed    ???
	 *		1x	- fast          1.78 MHz
	 *
	 * R1 controlled whether the video addressing was speeded up and R0
	 * did the same for the CPU.  On pre-CoCo 3 machines, setting R1 caused
	 * the screen to display garbage because the M6847 could not display
	 * fast enough.
	 *
	 * TODO:  Make the overclock more accurate.  In dual speed, ROM was a fast
	 * access but RAM was not.  I don't know how to simulate this.
	 */
    cpunum_set_clockscale(0, val ? 2 : 1);
}
READ8_HANDLER(dragon_alpha_mapped_irq_r)
{
	return dragon_rom_bank[0x3ff0 + offset];
}




static void d_sam_set_pageonemode(int val)
{
	/* Page mode - allowed switching between the low 32k and the high 32k,
	 * assuming that 64k wasn't enabled
	 *
	 * TODO:  Actually implement this.  Also find out what the CoCo 3 did with
	 * this (it probably ignored it)
	 */
}

static void d_sam_set_memorysize(int val)
{
	/* Memory size - allowed restricting memory accesses to something less than
	 * 32k
	 *
	 * This was a SAM switch that occupied 4 addresses:
	 *
	 *		$FFDD	(set)	R1
	 *		$FFDC	(clear)	R1
	 *		$FFDB	(set)	R0
	 *		$FFDA	(clear)	R0
	 *
	 * R1:R0 formed the following states:
	 *		00	- 4k
	 *		01	- 16k
	 *		10	- 64k
	 *		11	- static RAM (??)
	 *
	 * If something less than 64k was set, the low RAM would be smaller and
	 * mirror the other parts of the RAM
	 *
	 * TODO:  Find out what "static RAM" is
	 * TODO:  This should affect _all_ memory accesses, not just video ram
	 * TODO:  Verify that the CoCo 3 ignored this
	 */
}



/***************************************************************************
  CoCo 3 Timer

  The CoCo 3 had a timer that had would activate when first written to, and
  would decrement over and over again until zero was reached, and at that
  point, would flag an interrupt.  At this point, the timer starts back up
  again.

  I am deducing that the timer interrupt line was asserted if the timer was
  zero and unasserted if the timer was non-zero.  Since we never truly track
  the timer, we just use timer callback (coco3_timer_callback() asserts the
  line)

  Most CoCo 3 docs, including the specs that Tandy released, say that the
  high speed timer is 70ns (half of the speed of the main clock crystal).
  However, it seems that this is in error, and the GIME timer is really a
  280ns timer (one eighth the speed of the main clock crystal.  Gault's
  FAQ agrees with this
***************************************************************************/

static mame_timer *coco3_gime_timer;

static void coco3_timer_reset(void)
{
	/* reset the timer; take the value stored in $FF94-5 and start the timer ticking */
	UINT64 current_time;
	UINT16 timer_value;
	m6847_timing_type timing;
	mame_time target_time;

	/* value is from 0-4095 */
	timer_value = ((coco3_gimereg[4] & 0x0F) * 0x100) | coco3_gimereg[5];

	if (timer_value > 0)
	{
		/* depending on the GIME type, cannonicalize the value */
		if (GIME_TYPE_1987)
			timer_value += 1;	/* the 1987 GIME reset to the value plus one */
		else
			timer_value += 2;	/* the 1986 GIME reset to the value plus two */

		/* choose which timing clock source */
		timing = (coco3_gimereg[1] & 0x20) ? M6847_CLOCK : M6847_HSYNC;

		/* determine the current time */
		current_time = m6847_time(timing);

		/* calculate the time */
		target_time = m6847_time_until(timing, current_time + timer_value);
		if (LOG_TIMER)
			logerror("coco3_reset_timer(): target_time=%g\n", mame_time_to_double(target_time));

		/* and adjust the timer */
		mame_timer_adjust(coco3_gime_timer, target_time, 0, time_zero);
	}
	else
	{
		/* timer is shut off */
		mame_timer_reset(coco3_gime_timer, time_never);
		if (LOG_TIMER)
			logerror("coco3_reset_timer(): timer is off\n");
	}
}



static void coco3_timer_proc(int dummy)
{
	coco3_timer_reset();
	coco3_vh_blink();
	coco3_raise_interrupt(COCO3_INT_TMR, 1);
	coco3_raise_interrupt(COCO3_INT_TMR, 0);
}



static void coco3_timer_init(void)
{
	coco3_gime_timer = mame_timer_alloc(coco3_timer_proc);
}



/***************************************************************************
  MMU
***************************************************************************/

static void d_sam_set_maptype(int val)
{
	UINT8 *readbank;
	write8_handler writebank;

	if (val && (mess_ram_size > 0x8000))
	{
		readbank = &mess_ram[0x8000];
		writebank = MWA8_BANK2;
	}
	else
	{
		readbank = coco_rom;
		writebank = MWA8_ROM;
	}

	memory_set_bankptr(2, readbank);
	memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM, 0x8000, 0xfeff, 0, 0, writebank);
}

/* The hack that was dragon64_set_hipage, is now included in dragon64_set_maptype */
/* this now correctly selects between roms and ram in the 0x8000-0xFEFF reigon */
/* these can now be changed by code in *ANY* memory loaction, this more closeley */
/* resembles what happens on a real Dragon 64/Alpha */

static void dragon64_sam_set_maptype(int val)
{
	UINT8 *readbank2;
	UINT8 *readbank3;
	write8_handler writebank2;
	write8_handler writebank3;

	dragon_map_type = val;				/* Save in global for later use by rom pager */

	if (val && (mess_ram_size > 0x8000))
	{
		readbank2 = &mess_ram[0x8000];
		readbank3 = &mess_ram[0xc000];
		writebank2 = MWA8_BANK2;
		writebank3 = MWA8_BANK3;
	}
	else
	{
		readbank2 = dragon_rom_bank;	/* Basic/Boot Rom */
		readbank3 = coco_rom+0x4000;	/* Cartrage rom */
		writebank2 = MWA8_ROM;
		writebank3 = MWA8_ROM;
	}

	memory_set_bankptr(2, readbank2);
	memory_set_bankptr(3, readbank3);
	memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM, 0x8000, 0xbfff, 0, 0, writebank2);
	memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM, 0xc000, 0xfeff, 0, 0, writebank3);
}



/*************************************
 *
 *	CoCo 3
 *
 *************************************/


/*
 * coco3_mmu_translate() takes a zero counted bank index and an offset and
 * translates it into a physical RAM address.  The following logical memory
 * addresses have the following bank indexes:
 *
 *	Bank 0		$0000-$1FFF
 *	Bank 1		$2000-$3FFF
 *	Bank 2		$4000-$5FFF
 *	Bank 3		$6000-$7FFF
 *	Bank 4		$8000-$9FFF
 *	Bank 5		$A000-$BFFF
 *	Bank 6		$C000-$DFFF
 *	Bank 7		$E000-$FDFF
 *	Bank 8		$FE00-$FEFF
 *
 * The result represents a physical RAM address.  Since ROM/Cartidge space is
 * outside of the standard RAM memory map, ROM addresses get a "physical RAM"
 * address that has bit 31 set.  For example, ECB would be $80000000-
 * $80001FFFF, CB would be $80002000-$80003FFFF etc.  It is possible to force
 * this function to use a RAM address, which is used for video since video can
 * never reference ROM.
 */
offs_t coco3_mmu_translate(int bank, int offset)
{
	int forceram;
	UINT32 block;
	offs_t result;

	/* Bank 8 is the 0xfe00 block; and it is treated differently */
	if (bank == 8)
	{
		if (coco3_gimereg[0] & 8)
		{
			/* this GIME register fixes logical addresses $FExx to physical
			 * addresses $7FExx ($1FExx if 128k */
			assert(offset < 0x200);
			return ((mess_ram_size - 0x200) & 0x7ffff) + offset;
		}
		bank = 7;
		offset += 0x1e00;
		forceram = 1;
	}
	else
	{
		forceram = 0;
	}

	/* Perform the MMU lookup */
	if (coco3_gimereg[0] & 0x40)
	{
		if (coco3_gimereg[1] & 1)
			bank += 8;
		block = coco3_mmu[bank];
		block |= ((UINT32) ((coco3_gimereg[11] >> 4) & 0x03)) << 8;
	}
	else
	{
		block = bank + 56;
	}

	/* Are we actually in ROM?
	 *
	 * In our world, ROM is represented by memory blocks 0x40-0x47
	 *
	 * 0	Extended Color Basic
	 * 1	Color Basic
	 * 2	Reset Initialization
	 * 3	Super Extended Color Basic
	 * 4-7	Cartridge ROM
	 *
	 * This is the level where ROM is mapped, according to Tepolt (p21)
	 */
	if (((block & 0x3f) >= 0x3c) && !coco3_enable_64k && !forceram)
	{
		static const UINT8 rommap[4][4] =
		{
			{ 0, 1, 6, 7 },
			{ 0, 1, 6, 7 },
			{ 0, 1, 2, 3 },
			{ 4, 5, 6, 7 }
		};
		block = rommap[coco3_gimereg[0] & 3][(block & 0x3f) - 0x3c];
		result = (block * 0x2000 + offset) | 0x80000000;
	}
	else
	{
		result = ((block * 0x2000) + offset) % mess_ram_size;
	}
	return result;
}



static void coco3_mmu_update(int lowblock, int hiblock)
{
	static const struct
	{
		offs_t start;
		offs_t end;
	}
	bank_info[] =
	{
		{ 0x0000, 0x1fff },
		{ 0x2000, 0x3fff },
		{ 0x4000, 0x5fff },
		{ 0x6000, 0x7fff },
		{ 0x8000, 0x9fff },
		{ 0xa000, 0xbfff },
		{ 0xc000, 0xdfff },
		{ 0xe000, 0xfdff },
		{ 0xfe00, 0xfeff }
	};

	int i, offset, writebank;
	UINT8 *readbank;

	for (i = lowblock; i <= hiblock; i++)
	{
		offset = coco3_mmu_translate(i, 0);
		if (offset & 0x80000000)
		{
			/* an offset into the CoCo 3 ROM */
			readbank = &coco_rom[offset & ~0x80000000];
			writebank = STATIC_ROM;
		}
		else
		{
			/* offset into normal RAM */
			readbank = &mess_ram[offset];
			writebank = i + 1;
		}

		/* set up the banks */
		memory_set_bankptr(i + 1, readbank);
		memory_install_write_handler(0, ADDRESS_SPACE_PROGRAM, bank_info[i].start, bank_info[i].end, 0, 0, writebank);

		if (LOG_MMU)
		{
			logerror("CoCo3 GIME MMU: Logical $%04x ==> Physical $%05x\n",
				(i == 8) ? 0xfe00 : i * 0x2000,
				offset);
		}
	}
}



READ8_HANDLER(coco3_mmu_r)
{
	/* The high two bits are floating (high resistance).  Therefore their
	 * value is undefined.  But we are exposing them anyways here
	 */
	return coco3_mmu[offset];
}



WRITE8_HANDLER(coco3_mmu_w)
{
	coco3_mmu[offset] = data;

	/* Did we modify the live MMU bank? */
	if ((offset >> 3) == (coco3_gimereg[1] & 1))
	{
		offset &= 7;
		coco3_mmu_update(offset, (offset == 7) ? 8 : offset);
	}
}



/***************************************************************************
  GIME Registers (Reference: Super Extended Basic Unravelled)
***************************************************************************/

READ8_HANDLER(coco3_gime_r)
{
	UINT8 result = 0;

	switch(offset) {
	case 2:	/* Read pending IRQs */
		result = gime_irq;
		if (result) {
			gime_irq = 0;
			coco3_recalc_irq();
		}
		break;

	case 3:	/* Read pending FIRQs */
		result = gime_firq;
		if (result) {
			gime_firq = 0;
			coco3_recalc_firq();
		}
		break;

	case 4:	/* Timer MSB/LSB; these arn't readable */
	case 5:
		/* JK tells me that these values are indeterminate; and $7E appears
		 * to be the value most commonly returned
		 */
		result = 0x7e;
		break;

	default:
		result = coco3_gimereg[offset];
		break;
	}
	return result;
}



WRITE8_HANDLER(coco3_gime_w)
{
	coco3_gimereg[offset] = data;

	if (LOG_GIME)
		logerror("CoCo3 GIME: $%04x <== $%02x pc=$%04x\n", offset + 0xff90, data, activecpu_get_pc());

	/* Features marked with '!' are not yet implemented */
	switch(offset)
	{
		case 0:
			/*	$FF90 Initialization register 0
			*		  Bit 7 COCO 1=CoCo compatible mode
			*		  Bit 6 MMUEN 1=MMU enabled
			*		  Bit 5 IEN 1 = GIME chip IRQ enabled
			*		  Bit 4 FEN 1 = GIME chip FIRQ enabled
			*		  Bit 3 MC3 1 = RAM at FEXX is constant
			*		  Bit 2 MC2 1 = standard SCS (Spare Chip Select)
			*		  Bit 1 MC1 ROM map control
			*		  Bit 0 MC0 ROM map control
			*/
			coco3_mmu_update(0, 8);
			break;

		case 1:
			/*	$FF91 Initialization register 1
			*		  Bit 7 Unused
			*		  Bit 6 Unused
			*		  Bit 5 TINS Timer input select; 1 = 280 nsec, 0 = 63.5 usec
			*		  Bit 4 Unused
			*		  Bit 3 Unused
			*		  Bit 2 Unused
			*		  Bit 1 Unused
			*		  Bit 0 TR Task register select
			*/
			coco3_mmu_update(0, 8);
			coco3_timer_reset();
			break;

		case 2:
			/*	$FF92 Interrupt request enable register
			*		  Bit 7 Unused
			*		  Bit 6 Unused
			*		  Bit 5 TMR Timer interrupt
			*		  Bit 4 HBORD Horizontal border interrupt
			*		  Bit 3 VBORD Vertical border interrupt
			*		! Bit 2 EI2 Serial data interrupt
			*		  Bit 1 EI1 Keyboard interrupt
			*		  Bit 0 EI0 Cartridge interrupt
			*/
			if (LOG_INT_MASKING)
			{
				logerror("CoCo3 IRQ: Interrupts { %s%s%s%s%s%s} enabled\n",
					(data & 0x20) ? "TMR " : "",
					(data & 0x10) ? "HBORD " : "",
					(data & 0x08) ? "VBORD " : "",
					(data & 0x04) ? "EI2 " : "",
					(data & 0x02) ? "EI1 " : "",
					(data & 0x01) ? "EI0 " : "");
			}
			break;

		case 3:
			/*	$FF93 Fast interrupt request enable register
			*		  Bit 7 Unused
			*		  Bit 6 Unused
			*		  Bit 5 TMR Timer interrupt
			*		  Bit 4 HBORD Horizontal border interrupt
			*		  Bit 3 VBORD Vertical border interrupt
			*		! Bit 2 EI2 Serial border interrupt
			*		  Bit 1 EI1 Keyboard interrupt
			*		  Bit 0 EI0 Cartridge interrupt
			*/
			if (LOG_INT_MASKING)
			{
				logerror("CoCo3 FIRQ: Interrupts { %s%s%s%s%s%s} enabled\n",
					(data & 0x20) ? "TMR " : "",
					(data & 0x10) ? "HBORD " : "",
					(data & 0x08) ? "VBORD " : "",
					(data & 0x04) ? "EI2 " : "",
					(data & 0x02) ? "EI1 " : "",
					(data & 0x01) ? "EI0 " : "");
			}
			break;

		case 4:
			/*	$FF94 Timer register MSB
			*		  Bits 4-7 Unused
			*		  Bits 0-3 High order four bits of the timer
			*/
			coco3_timer_reset();
			break;

		case 5:
			/*	$FF95 Timer register LSB
			*		  Bits 0-7 Low order eight bits of the timer
			*/
			break;

		case 8:
			/*	$FF98 Video Mode Register
			*		  Bit 7 BP 0 = Text modes, 1 = Graphics modes
			*		  Bit 6 Unused
			*		! Bit 5 BPI Burst Phase Invert (Color Set)
			*		  Bit 4 MOCH 1 = Monochrome on Composite
			*		! Bit 3 H50 1 = 50 Hz power, 0 = 60 Hz power
			*		  Bits 0-2 LPR Lines per row
			*/
			break;

		case 9:
			/*	$FF99 Video Resolution Register
			*		  Bit 7 Undefined
			*		  Bits 5-6 LPF Lines per Field (Number of Rows)
			*		  Bits 2-4 HRES Horizontal Resolution
			*		  Bits 0-1 CRES Color Resolution
			*/
			break;

		case 10:
			/*	$FF9A Border Register
			*		  Bits 6,7 Unused
			*		  Bits 0-5 BRDR Border color
			*/
			break;

		case 12:
			/*	$FF9C Vertical Scroll Register
			*		  Bits 4-7 Reserved
			*		! Bits 0-3 VSC Vertical Scroll bits
			*/
			break;

		case 11:
		case 13:
		case 14:
			/*	$FF9B,$FF9D,$FF9E Vertical Offset Registers
			*
			*	According to JK, if an odd value is placed in $FF9E on the 1986
			*	GIME, the GIME crashes
			*
			*  The reason that $FF9B is not mentioned in offical documentation
			*  is because it is only meaninful in CoCo 3's with the 2MB upgrade
			*/
			break;

		case 15:
			/*
			*	$FF9F Horizontal Offset Register
			*		  Bit 7 HVEN Horizontal Virtual Enable
			*		  Bits 0-6 X0-X6 Horizontal Offset Address
			*
			*  Unline $FF9D-E, this value can be modified mid frame
			*/
			break;
	}
}



static const UINT8 *coco3_sam_get_rambase(void)
{
	UINT32 video_base;
	video_base = coco3_get_video_base(0xE0, 0x3F);
	return &mess_ram[video_base % mess_ram_size];
}




static void coco3_sam_set_maptype(int val)
{
	coco3_enable_64k = val;
	coco3_mmu_update(4, 8);
}



/***************************************************************************
  Cartridge Expansion Slot
 ***************************************************************************/

static void coco_cartrige_init(const struct cartridge_slot *cartinterface, const struct cartridge_callback *callbacks)
{
	coco_cart_interface = cartinterface;

	if (cartinterface)
		cartinterface->init(callbacks);
}

READ8_HANDLER(coco_cartridge_r)
{
	return (coco_cart_interface && coco_cart_interface->io_r) ? coco_cart_interface->io_r(offset) : 0;
}

WRITE8_HANDLER(coco_cartridge_w)
{
	if (coco_cart_interface && coco_cart_interface->io_w)
		coco_cart_interface->io_w(offset, data);
}

 READ8_HANDLER(coco3_cartridge_r)
{
	/* This behavior is documented in Super Extended Basic Unravelled, page 14 */
	return ((coco3_gimereg[0] & 0x04) || (offset >= 0x10)) ? coco_cartridge_r(offset) : 0;
}

WRITE8_HANDLER(coco3_cartridge_w)
{
	/* This behavior is documented in Super Extended Basic Unravelled, page 14 */
	if ((coco3_gimereg[0] & 0x04) || (offset >= 0x10))
		coco_cartridge_w(offset, data);
}


static void coco_cartridge_enablesound(int enable)
{
	if (coco_cart_interface && coco_cart_interface->enablesound)
		coco_cart_interface->enablesound(enable);
}

/***************************************************************************
  Machine Initialization
***************************************************************************/

static void coco_setcartline(int data)
{
	/* If cart autostart enabled then start it, else do not autostart */
	if (cart_inserted)
		cart_line = data;
	else
		cart_line = 0;
	
	pia_1_cb1_w(0, data ? ASSERT_LINE : CLEAR_LINE);
}

static void coco3_setcartline(int data)
{
	/* If cart autostart enabled then start it, else do not autostart */
	if (cart_inserted)
		cart_line = data;
	else
		cart_line = 0;

	pia_1_cb1_w(0, data ? ASSERT_LINE : CLEAR_LINE);
	coco3_raise_interrupt(COCO3_INT_EI0, cart_line ? 1 : 0);
}

/* This function, and all calls of it, are hacks for bankswitched games */
static int count_bank(void)
{
	unsigned int crc;
	mess_image *img;

	img = cartslot_image();
	if (!image_exists(img))
		return FALSE;

	crc = image_crc(img);

	switch( crc )
	{
		case 0x83bd6056:		/* Mind-Roll */
			logerror("ROM cartridge bankswitching enabled: Mind-Roll (26-3100)\n");
			return 1;
			break;
		case 0xBF4AD8F8:		/* Predator */
			logerror("ROM cartridge bankswitching enabled: Predator (26-3165)\n");
			return 3;
			break;
		case 0xDD94DD06:		/* RoboCop */
			logerror("ROM cartridge bankswitching enabled: RoboCop (26-3164)\n");
			return 7;
			break;
		default:				/* No bankswitching */
			return 0;
			break;
	}
}

/* This function, and all calls of it, are hacks for bankswitched games */
static int is_Orch90(void)
{
	unsigned int crc;
	mess_image *img;

	img = cartslot_image();
	if (!image_exists(img))
		return FALSE;

	crc = image_crc(img);
	return crc == 0x15FB39AF;
}

static void generic_setcartbank(int bank, UINT8 *cartpos)
{
	mame_file *fp;

	if (count_bank() > 0)
	{
		/* pin variable to proper bit width */
		bank &= count_bank();

		/* read the bank */
		fp = image_fp(cartslot_image());
		mame_fseek(fp, bank * 0x4000, SEEK_SET);
		mame_fread(fp, cartpos, 0x4000);
	}
}

static void coco_setcartbank(int bank)
{
	generic_setcartbank(bank, &coco_rom[0x4000]);
}

static void coco3_setcartbank(int bank)
{
	generic_setcartbank(bank, &coco_rom[0xC000]);
}

static const struct cartridge_callback coco_cartcallbacks =
{
	coco_setcartline,
	coco_setcartbank
};

static const struct cartridge_callback coco3_cartcallbacks =
{
	coco3_setcartline,
	coco3_setcartbank
};

static void generic_init_machine(const pia6821_interface *piaintf, const sam6883_interface *samintf,
	const struct cartridge_slot *cartinterface, const struct cartridge_callback *cartcallback,
	void (*recalc_interrupts_)(int dummy))
{
	const struct cartridge_slot *cartslottype;
	int portnum;

	recalc_interrupts = recalc_interrupts_;

	coco_rom = memory_region(REGION_CPU1);

	/* Setup Dragon 64/ Alpha default bank */
	dragon_rom_bank = coco_rom;

	cart_line = CARTLINE_CLEAR;

	pia_config(0, PIA_STANDARD_ORDERING | PIA_8BIT, &piaintf[0]);
	pia_config(1, PIA_STANDARD_ORDERING | PIA_8BIT, &piaintf[1]);
	pia_config(2, PIA_STANDARD_ORDERING | PIA_8BIT, &piaintf[2]); /* Dragon Alpha 3rd pia */
	pia_reset();

	sam_init(samintf);

	/* HACK for bankswitching carts */
	if( is_Orch90() )
		cartslottype = &cartridge_Orch90;
	else
	    cartslottype = (count_bank() > 0) ? &cartridge_banks : &cartridge_standard;

	coco_cartrige_init(cart_inserted ? cartslottype : cartinterface, cartcallback);

	for (portnum = 0; portnum <= 6; portnum++)
		input_port_set_changed_callback(portnum, ~0, coco_poll_keyboard, NULL);

#ifdef MAME_DEBUG
	cpuintrf_set_dasm_override(coco_dasm_override);
#endif

	add_exit_callback(coco_machine_stop);
}

MACHINE_START( dragon32 )
{
	memory_set_bankptr(1, &mess_ram[0]);
	generic_init_machine(coco_pia_intf, &coco_sam_intf, &cartridge_fdc_dragon, &coco_cartcallbacks, d_recalc_interrupts);

	coco_or_dragon = AM_DRAGON;
	return 0;
}

MACHINE_START( dragon64 )
{
	memory_set_bankptr(1, &mess_ram[0]);
	generic_init_machine(dragon64_pia_intf, &dragon64_sam_intf, &cartridge_fdc_dragon, &coco_cartcallbacks, d_recalc_interrupts);
	acia_6551_init();
	
	coco_or_dragon = AM_DRAGON;
	return 0;
}

MACHINE_START( dgnalpha )
{
	memory_set_bankptr(1, &mess_ram[0]);
	generic_init_machine(dgnalpha_pia_intf, &dragon64_sam_intf, 0 /*&cartridge_fdc_dragon*/, &coco_cartcallbacks, d_recalc_interrupts);
    	
	acia_6551_init();
	
	/* dgnalpha_just_reset, is here to flag that we should ignore the first irq generated */
	/* by the WD2797, it is reset to 0 after the first inurrupt */
	dgnalpha_just_reset=1;
	
	wd179x_init(WD_TYPE_179X, dgnalpha_fdc_callback);

	coco_or_dragon = AM_DRAGON;
	return 0;
}

MACHINE_START( coco )
{
	memory_set_bankptr(1, &mess_ram[0]);
	generic_init_machine(coco_pia_intf, &coco_sam_intf, &cartridge_fdc_coco, &coco_cartcallbacks, d_recalc_interrupts);

	coco_or_dragon = AM_COCO;
	return 0;
}

MACHINE_START( coco2 )
{
	memory_set_bankptr(1, &mess_ram[0]);
	generic_init_machine(coco2_pia_intf, &coco_sam_intf, &cartridge_fdc_coco, &coco_cartcallbacks, d_recalc_interrupts);

	coco_or_dragon = AM_COCO;
	return 0;
}

static void coco3_machine_reset(void)
{
	int i;

	/* Tepolt verifies that the GIME registers are all cleared on initialization */
	coco3_enable_64k = 0;
	gime_irq = 0;
	gime_firq = 0;
	for (i = 0; i < 8; i++)
	{
		coco3_mmu[i] = coco3_mmu[i + 8] = 56 + i;
		coco3_gimereg[i] = 0;
	}
	coco3_mmu_update(0, 8);
}

static void coco3_state_postload(void)
{
	coco3_mmu_update(0, 8);
}

MACHINE_START( coco3 )
{
	int portnum;

	generic_init_machine(coco3_pia_intf, &coco3_sam_intf, &cartridge_fdc_coco, &coco3_cartcallbacks, coco3_recalc_interrupts);

	coco3_machine_reset();
	coco3_timer_init();

	coco3_interupt_line = 0;
	
	coco_or_dragon = AM_COCO;

	for (portnum = 0; portnum <= 6; portnum++)
		input_port_set_changed_callback(portnum, ~0, coco_poll_keyboard, NULL);

	state_save_register_global_array(coco3_mmu);
	state_save_register_global_array(coco3_gimereg);
	state_save_register_global(coco3_interupt_line);
	state_save_register_global(gime_irq);
	state_save_register_global(gime_firq);
	state_save_register_func_postload(coco3_state_postload);

	add_reset_callback(coco3_machine_reset);
	return 0;
}

static void coco_machine_stop(void)
{
	if (coco_cart_interface && coco_cart_interface->term)
		coco_cart_interface->term();
	cart_inserted = 0;
}

/***************************************************************************
  OS9 Syscalls for disassembly
****************************************************************************/

#ifdef MAME_DEBUG

static const char *os9syscalls[] =
{
	"F$Link",          /* Link to Module */
	"F$Load",          /* Load Module from File */
	"F$UnLink",        /* Unlink Module */
	"F$Fork",          /* Start New Process */
	"F$Wait",          /* Wait for Child Process to Die */
	"F$Chain",         /* Chain Process to New Module */
	"F$Exit",          /* Terminate Process */
	"F$Mem",           /* Set Memory Size */
	"F$Send",          /* Send Signal to Process */
	"F$Icpt",          /* Set Signal Intercept */
	"F$Sleep",         /* Suspend Process */
	"F$SSpd",          /* Suspend Process */
	"F$ID",            /* Return Process ID */
	"F$SPrior",        /* Set Process Priority */
	"F$SSWI",          /* Set Software Interrupt */
	"F$PErr",          /* Print Error */
	"F$PrsNam",        /* Parse Pathlist Name */
	"F$CmpNam",        /* Compare Two Names */
	"F$SchBit",        /* Search Bit Map */
	"F$AllBit",        /* Allocate in Bit Map */
	"F$DelBit",        /* Deallocate in Bit Map */
	"F$Time",          /* Get Current Time */
	"F$STime",         /* Set Current Time */
	"F$CRC",           /* Generate CRC */
	"F$GPrDsc",        /* get Process Descriptor copy */
	"F$GBlkMp",        /* get System Block Map copy */
	"F$GModDr",        /* get Module Directory copy */
	"F$CpyMem",        /* Copy External Memory */
	"F$SUser",         /* Set User ID number */
	"F$UnLoad",        /* Unlink Module by name */
	"F$Alarm",         /* Color Computer Alarm Call (system wide) */
	NULL,
	NULL,
	"F$NMLink",        /* Color Computer NonMapping Link */
	"F$NMLoad",        /* Color Computer NonMapping Load */
	NULL,
	NULL,
	"F$TPS",           /* Return System's Ticks Per Second */
	"F$TimAlm",        /* COCO individual process alarm call */
	"F$VIRQ",          /* Install/Delete Virtual IRQ */
	"F$SRqMem",        /* System Memory Request */
	"F$SRtMem",        /* System Memory Return */
	"F$IRQ",           /* Enter IRQ Polling Table */
	"F$IOQu",          /* Enter I/O Queue */
	"F$AProc",         /* Enter Active Process Queue */
	"F$NProc",         /* Start Next Process */
	"F$VModul",        /* Validate Module */
	"F$Find64",        /* Find Process/Path Descriptor */
	"F$All64",         /* Allocate Process/Path Descriptor */
	"F$Ret64",         /* Return Process/Path Descriptor */
	"F$SSvc",          /* Service Request Table Initialization */
	"F$IODel",         /* Delete I/O Module */
	"F$SLink",         /* System Link */
	"F$Boot",          /* Bootstrap System */
	"F$BtMem",         /* Bootstrap Memory Request */
	"F$GProcP",        /* Get Process ptr */
	"F$Move",          /* Move Data (low bound first) */
	"F$AllRAM",        /* Allocate RAM blocks */
	"F$AllImg",        /* Allocate Image RAM blocks */
	"F$DelImg",        /* Deallocate Image RAM blocks */
	"F$SetImg",        /* Set Process DAT Image */
	"F$FreeLB",        /* Get Free Low Block */
	"F$FreeHB",        /* Get Free High Block */
	"F$AllTsk",        /* Allocate Process Task number */
	"F$DelTsk",        /* Deallocate Process Task number */
	"F$SetTsk",        /* Set Process Task DAT registers */
	"F$ResTsk",        /* Reserve Task number */
	"F$RelTsk",        /* Release Task number */
	"F$DATLog",        /* Convert DAT Block/Offset to Logical */
	"F$DATTmp",        /* Make temporary DAT image (Obsolete) */
	"F$LDAXY",         /* Load A [X,[Y]] */
	"F$LDAXYP",        /* Load A [X+,[Y]] */
	"F$LDDDXY",        /* Load D [D+X,[Y]] */
	"F$LDABX",         /* Load A from 0,X in task B */
	"F$STABX",         /* Store A at 0,X in task B */
	"F$AllPrc",        /* Allocate Process Descriptor */
	"F$DelPrc",        /* Deallocate Process Descriptor */
	"F$ELink",         /* Link using Module Directory Entry */
	"F$FModul",        /* Find Module Directory Entry */
	"F$MapBlk",        /* Map Specific Block */
	"F$ClrBlk",        /* Clear Specific Block */
	"F$DelRAM",        /* Deallocate RAM blocks */
	"F$GCMDir",        /* Pack module directory */
	"F$AlHRam",        /* Allocate HIGH RAM Blocks */
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	"F$RegDmp",        /* Ron Lammardo's debugging register dump call */
	"F$NVRAM",         /* Non Volatile RAM (RTC battery backed static) read/write */
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	"I$Attach",        /* Attach I/O Device */
	"I$Detach",        /* Detach I/O Device */
	"I$Dup",           /* Duplicate Path */
	"I$Create",        /* Create New File */
	"I$Open",          /* Open Existing File */
	"I$MakDir",        /* Make Directory File */
	"I$ChgDir",        /* Change Default Directory */
	"I$Delete",        /* Delete File */
	"I$Seek",          /* Change Current Position */
	"I$Read",          /* Read Data */
	"I$Write",         /* Write Data */
	"I$ReadLn",        /* Read Line of ASCII Data */
	"I$WritLn",        /* Write Line of ASCII Data */
	"I$GetStt",        /* Get Path Status */
	"I$SetStt",        /* Set Path Status */
	"I$Close",         /* Close Path */
	"I$DeletX"         /* Delete from current exec dir */
};


static unsigned coco_dasm_override(int cpunum, char *buffer, unsigned pc)
{
	unsigned call;
	unsigned result = 0;

	if ((cpu_readop(pc + 0) == 0x10) && (cpu_readop(pc + 1) == 0x3F))
	{
		call = cpu_readop(pc + 2);
		if ((call >= 0) && (call < sizeof(os9syscalls) / sizeof(os9syscalls[0])) && os9syscalls[call])
		{
			sprintf(buffer, "OS9   %s", os9syscalls[call]);
			result = 3;
		}
	}
	return result;
}



#endif /* MAME_DEBUG */
