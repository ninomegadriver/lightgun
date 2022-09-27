/***************************************************************************

	systems\dgn_beta.c

	Dragon Beta prototype, based on two 68B09E processors, WD2797, 6845.

Project Beta was the second machine that Dragon Data had in development at 
the time they ceased trading, the first being Project Alpha (also know as the 
Dragon Professional). 

The machine uses dual 68B09 cpus which both sit on the same bus and access 
the same memory and IO chips ! The first is the main processor, used to run 
user code, the second is uses as a DMA controler, to amongst other things 
disk data transfers. The first processor controled the second by having the 
halt and nmi lines from the second cpu connected to PIA output lines so 
that the could be changed under OS control. The first CPU just passed 
instructions for the block to be transfered in 5 low ram addresses and 
generated an NMI on the second CPU.

Project Beta like the other Dragons used a WD2797 floppy disk controler
which is memory mapped, and controled by the second CPU.

Unlike the other Dragon machines, project Beta used a 68b45 to generate video, 
and totally did away with the SAM. 

The machine has a 6551 ACIA chip, but I have not yet found where this is 
memory mapped.

Project Beta, had a custom MMU bilt from a combination of LSTTL logic, and 
PAL programable logic. This MMU could address 256 blocks of 4K, giving a 
total addressable range of 1 megabyte, of this the first 768KB could be RAM,
however tha machine by default, came with 256K or ram, and a 16K boot rom,
which contained an OS-9 Level 2 bootstrap.

A lot of the infomrmation required to start work on this driver has been
infered from disassembly of the boot rom, and from what little hardware 
documentation still exists. 

***************************************************************************/

#include "driver.h"
#include "inputx.h"
#include "machine/6821pia.h"
#include "vidhrdw/generic.h"
#include "includes/crtc6845.h"
#include "includes/coco.h"
#include "includes/dgn_beta.h"
#include "devices/basicdsk.h"
#include "includes/6551.h"
#include "formats/coco_dsk.h"
#include "devices/mflopimg.h"
#include "devices/coco_vhd.h"

/*
 Colour codes are as below acording to os-9 headers, however the presise values 
 may not be quite correct, also this will need changing as the pallate seems to 
 be controled by a 4x4bit register file in the video hardware 
 The text video ram seems to be aranged of words of character, attribute 
 The colour codes are stored in the attribute byte along with : 
	Underline bit 	$40	 
	Flash bit	$80

These are yet to be implemented.

*/

unsigned char dgnbeta_palette[] =
{
	0x00,0x00,0x00, 	/* black */
	0x80,0x00,0x00,		/* red */
	0x00,0x80,0x00, 	/* green */
	0x80,0x80,0x00,		/* yellow */
	0x00,0x00,0x80,		/* blue */
	0x80,0x00,0x80,		/* magenta */
	0x00,0x80,0x80,		/* cyan */
	0x80,0x80,0x80		/* white */
};

static unsigned short dgnbeta_colortable[][8] =
{
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	/* reverse */
//	{ 1, 0 }
};

static gfx_layout dgnbeta_charlayout =
{
	8,9,
	256,                    /* 512 characters */
	1,                      /* 1 bits per pixel */
	{ 0 },                  /* no bitplanes; 1 bit per pixel */
	/* x offsets */
	{ 0,1,2,3,4,5,6,7,8,9 },
	/* y offsets */
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8, 8*8, 9*8,},
	16*8			/* Each char is 16 bytes apart */
};

static gfx_decode dgnbeta_gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0x0000, &dgnbeta_charlayout, 0, 1 },
	{ -1 } /* end of array */
};

/*
	2005-05-10
	
	I *THINK* I know how the memory paging works, the 64K memory map is devided
	into 16x 4K pages, what is mapped into each page is controled by the IO at
	FE00-FE0F like so :-

	Location	Memory page 	Initialised to 
	$FE00		$0000-$0FFF		$00
	$FE01		$1000-$1FFF		$00	(used as ram test page, when sizing memory)
	$FE02		$2000-$2FFF		$00
	$FE03		$3000-$3FFF		$00
	$FE04		$4000-$4FFF		$00
	$FE05		$5000-$5FFF		$00
	$FE06		$6000-$6FFF		$1F	($1F000)
	$FE07		$7000-$7FFF		$00		
	$FE08		$8000-$8FFF		$00
	$FE09		$9000-$9FFF		$00
	$FE0A		$A000-$AFFF		$00
	$FE0B		$B000-$BFFF		$00
	$FE0C		$C000-$CFFF		$00
	$FE0D		$D000-$DFFF		$00
	$FE0E		$E000-$EFFF		$FE
	$FE0F		$F000-$FFFF		$FF

	The value stored at each location maps it's page to a 4K page within a 1M address 
	space. Acording to the Beta product descriptions released by Dragon Data, the 
	machine could have up to 768K of RAM, if this where true then pages $00-$BF could 
	potentially be RAM, and pages $C0-$FF would be ROM. The initialisation code maps in
	the memory as described above.

	At reset time the Paging would of course be disabled, as the boot rom needs to be 
	mapped in at $C000, the initalisation code would set up the mappings above and then
	enable the paging hardware.

	It appears to be more complicated than this, whilst the above is true, there appear to 
	be 16 sets of banking registers, the active set is controled by the bottom 4 bits of
	FCC0, bit 6 has something to do with enabling and disabling banking.
	
	2005-11-28
	
	The value $C0 is garanteed not to have any memory in it acording to the os9 headers,
	quite how the MMU deals with this is still unknown to me.
	
	Bit 7 of $FCC0, sets maps in the system task which has fixed values for some pages,
	the presise nature of this is yet to be descovered.

*/

static ADDRESS_MAP_START( dgnbeta_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x0FFF) 	AM_READWRITE(MRA8_BANK1		,MWA8_BANK1)
	AM_RANGE(0x1000, 0x1FFF) 	AM_READWRITE(MRA8_BANK2		,MWA8_BANK2)
	AM_RANGE(0x2000, 0x2FFF) 	AM_READWRITE(MRA8_BANK3		,MWA8_BANK3)
	AM_RANGE(0x3000, 0x3FFF) 	AM_READWRITE(MRA8_BANK4		,MWA8_BANK4)
	AM_RANGE(0x4000, 0x4FFF) 	AM_READWRITE(MRA8_BANK5		,MWA8_BANK5)
	AM_RANGE(0x5000, 0x5FFF) 	AM_READWRITE(MRA8_BANK6		,MWA8_BANK6)
	AM_RANGE(0x6000, 0x6FFF) 	AM_READWRITE(MRA8_BANK7		,MWA8_BANK7) AM_BASE(&videoram) AM_SIZE(&videoram_size) 
	AM_RANGE(0x7000, 0x7FFF) 	AM_READWRITE(MRA8_BANK8		,MWA8_BANK8)
	AM_RANGE(0x8000, 0x8FFF) 	AM_READWRITE(MRA8_BANK9		,MWA8_BANK9)
	AM_RANGE(0x9000, 0x9FFF) 	AM_READWRITE(MRA8_BANK10	,MWA8_BANK10)
	AM_RANGE(0xA000, 0xAFFF) 	AM_READWRITE(MRA8_BANK11	,MWA8_BANK11)
	AM_RANGE(0xB000, 0xBFFF) 	AM_READWRITE(MRA8_BANK12	,MWA8_BANK12)
	AM_RANGE(0xC000, 0xCFFF) 	AM_READWRITE(MRA8_BANK13	,MWA8_BANK13)
	AM_RANGE(0xD000, 0xDFFF) 	AM_READWRITE(MRA8_BANK14	,MWA8_BANK14)
	AM_RANGE(0xE000, 0xEFFF) 	AM_READWRITE(MRA8_BANK15	,MWA8_BANK15)
	AM_RANGE(0xF000, 0xFBFF) 	AM_READWRITE(MRA8_BANK16	,MWA8_BANK16)
	AM_RANGE(0xfC00, 0xfC1F)	AM_READWRITE(MRA8_NOP		,MWA8_NOP)	
	AM_RANGE(0xFC20, 0xFC23)	AM_READWRITE(pia_0_r		,pia_0_w)
	AM_RANGE(0xFC24, 0xFC27)	AM_READWRITE(pia_1_r		,pia_1_w)
	AM_RANGE(0xFC28, 0xfC7F)	AM_READWRITE(MRA8_NOP		,MWA8_NOP)	
	AM_RANGE(0xfc80, 0xfc81)	AM_READWRITE(crtc6845_0_port_r	,crtc6845_0_port_w)	
	AM_RANGE(0xfc82, 0xfCBF)	AM_READWRITE(MRA8_NOP		,MWA8_NOP)
	AM_RANGE(0xFCC0, 0xFCC3)	AM_READWRITE(pia_2_r		,pia_2_w)
	AM_RANGE(0xfcC4, 0xfcdf)	AM_READWRITE(MRA8_NOP		,MWA8_NOP)
	AM_RANGE(0xfce0, 0xfce3)	AM_READWRITE(dgnbeta_wd2797_r	,dgnbeta_wd2797_w)	/* Onboard disk interface */
	AM_RANGE(0xfce4, 0xfdff)	AM_READWRITE(MRA8_NOP		,MWA8_NOP)
	AM_RANGE(0xFE00, 0xFE0F)	AM_READWRITE(dgn_beta_page_r	,dgn_beta_page_w)
	AM_RANGE(0xfe10, 0xfEff)	AM_READWRITE(MRA8_NOP		,MWA8_NOP)
	AM_RANGE(0xFF00, 0xFFFF) 	AM_READWRITE(MRA8_BANK17	,MWA8_BANK17)

//	AM_RANGE(0x1F000, 0x1FFFF) 	AM_READWRITE(MRA8_BANK32	,MWA8_BANK32) AM_BASE(&videoram) AM_SIZE(&videoram_size)
ADDRESS_MAP_END



INPUT_PORTS_START( dgnbeta )
	PORT_START /* Key ROw 0 */
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("CLEAR") PORT_CODE(KEYCODE_HOME) PORT_CHAR(12)
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("8  (") PORT_CODE(KEYCODE_8) PORT_CHAR('8') 
	PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("ESC") PORT_CODE(KEYCODE_ESC) PORT_CHAR(0x1B) 

	PORT_START /* Key row 2 */
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("b") PORT_CODE(KEYCODE_B) PORT_CHAR('b') 
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("j") PORT_CODE(KEYCODE_J) PORT_CHAR('j') 
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("i") PORT_CODE(KEYCODE_I) PORT_CHAR('i') 
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("9") PORT_CODE(KEYCODE_9) PORT_CHAR('9') 
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("u") PORT_CODE(KEYCODE_U) PORT_CHAR('u') 
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("h") PORT_CODE(KEYCODE_H) PORT_CHAR('h') 
	PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("7") PORT_CODE(KEYCODE_7) PORT_CHAR('7') 

	PORT_START /* Key row 3 */
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("n") PORT_CODE(KEYCODE_N) PORT_CHAR('n') 
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("k") PORT_CODE(KEYCODE_K) PORT_CHAR('k') 
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("o") PORT_CODE(KEYCODE_O) PORT_CHAR('o') 
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("0") PORT_CODE(KEYCODE_0) PORT_CHAR('0') 
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("y") PORT_CODE(KEYCODE_Y) PORT_CHAR('y') 
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("g") PORT_CODE(KEYCODE_G) PORT_CHAR('g') 
	PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("6") PORT_CODE(KEYCODE_6) PORT_CHAR('6') 
	
	PORT_START /* Key row  4 */
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("m") PORT_CODE(KEYCODE_M) PORT_CHAR('m') 
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("l") PORT_CODE(KEYCODE_L) PORT_CHAR('l') 
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("p") PORT_CODE(KEYCODE_P) PORT_CHAR('p') 
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("-") PORT_CODE(KEYCODE_MINUS) PORT_CHAR('-') 
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("t") PORT_CODE(KEYCODE_T) PORT_CHAR('t') 
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("f") PORT_CODE(KEYCODE_F) PORT_CHAR('f') 
	PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("5") PORT_CODE(KEYCODE_5) PORT_CHAR('5') 
	
	PORT_START /* Key row  5 */
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME(",") PORT_CODE(KEYCODE_COMMA) PORT_CHAR(',') 
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME(";") PORT_CODE(KEYCODE_QUOTE) PORT_CHAR(';') 
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("@") PORT_CODE(KEYCODE_ASTERISK) PORT_CHAR('@') 
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("^") PORT_CODE(KEYCODE_UP) PORT_CHAR('^') 
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("v") PORT_CODE(KEYCODE_V) PORT_CHAR('v') 
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("d") PORT_CODE(KEYCODE_D) PORT_CHAR('d') 
	PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("4") PORT_CODE(KEYCODE_4) PORT_CHAR('4') 
	
	PORT_START /* Key row  6 */
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME(".") PORT_CODE(KEYCODE_STOP) PORT_CHAR('.') 
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME(":") PORT_CODE(KEYCODE_COLON) PORT_CHAR(':') 
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("\\") PORT_CODE(KEYCODE_BACKSLASH) PORT_CHAR('\\') 
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("[") PORT_CODE(KEYCODE_OPENBRACE) PORT_CHAR('[') 
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("r") PORT_CODE(KEYCODE_R) PORT_CHAR('r') 
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("s") PORT_CODE(KEYCODE_S) PORT_CHAR('s') 
	PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("3") PORT_CODE(KEYCODE_3) PORT_CHAR('3') 
	
	PORT_START /* Key row  7 */
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("/") PORT_CODE(KEYCODE_SLASH) PORT_CHAR('/') 
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("ENTER") PORT_CODE(KEYCODE_ENTER) PORT_CHAR(13) 
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("DELETE") PORT_CODE(KEYCODE_BACKSPACE) PORT_CHAR(8) 
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("]") PORT_CODE(KEYCODE_CLOSEBRACE) PORT_CHAR(']') 
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("c") PORT_CODE(KEYCODE_C) PORT_CHAR('c') 
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("x") PORT_CODE(KEYCODE_X) PORT_CHAR('x') 
	PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("SPACE") PORT_CODE(KEYCODE_SPACE) PORT_CHAR(' ') 
	
	PORT_START /* Key row  8 */
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("ASTERISK") PORT_CODE(KEYCODE_ASTERISK) PORT_CHAR('*') 
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("PAD_7") PORT_CODE(KEYCODE_7_PAD) PORT_CHAR('7') 
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("PAD_4") PORT_CODE(KEYCODE_4_PAD) PORT_CHAR('4') 
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("PAD_1") PORT_CODE(KEYCODE_1_PAD) PORT_CHAR('1') 
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("e") PORT_CODE(KEYCODE_E) PORT_CHAR('e') 
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("z") PORT_CODE(KEYCODE_Z) PORT_CHAR('z') 
	PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("2") PORT_CODE(KEYCODE_2) PORT_CHAR('2') 
	
	PORT_START /* Key row  9 */
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("PAD_0") PORT_CODE(KEYCODE_0_PAD) PORT_CHAR('0') 
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("PAD_8") PORT_CODE(KEYCODE_8_PAD) PORT_CHAR('8') 
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("PAD_5") PORT_CODE(KEYCODE_5_PAD) PORT_CHAR('5') 
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("PAD_2") PORT_CODE(KEYCODE_2_PAD) PORT_CHAR('2') 
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("w") PORT_CODE(KEYCODE_W) PORT_CHAR('w') 
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("a") PORT_CODE(KEYCODE_A) PORT_CHAR('a') 
	PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("1") PORT_CODE(KEYCODE_1) PORT_CHAR('1') 
	
	PORT_START /* Key row  10 */
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("PAD_HASH") PORT_CODE(KEYCODE_MINUS_PAD) PORT_CHAR('#') 
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("PAD_9") PORT_CODE(KEYCODE_9_PAD) PORT_CHAR('9') 
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("PAD_6") PORT_CODE(KEYCODE_6_PAD) PORT_CHAR('6') 
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("PAD_3") PORT_CODE(KEYCODE_3_PAD) PORT_CHAR('3') 
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("q") PORT_CODE(KEYCODE_Q) PORT_CHAR('q') 
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("TAB") PORT_CODE(KEYCODE_TAB) PORT_CHAR(9) 
	PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("DOT_PAD") PORT_CODE(KEYCODE_DEL_PAD) PORT_CHAR('.') 
		
	PORT_START
INPUT_PORTS_END


static const char *dgnbeta_floppy_getname(const struct IODevice *dev, int id, char *buf, size_t bufsize)
{
	/* CoCo people like their floppy drives zero counted */
	snprintf(buf, bufsize, "Floppy #%d", id);
	return buf;
}

static void dgnbeta_floppy_getinfo(const device_class *devclass, UINT32 state, union devinfo *info)
{
	/* floppy */
	switch(state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case DEVINFO_INT_COUNT:							info->i = 4; break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case DEVINFO_PTR_FLOPPY_OPTIONS:				info->p = (void *) floppyoptions_coco; break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case DEVINFO_STR_NAME+0:						strcpy(info->s = device_temp_str(), "floppydisk0"); break;
		case DEVINFO_STR_NAME+1:						strcpy(info->s = device_temp_str(), "floppydisk1"); break;
		case DEVINFO_STR_NAME+2:						strcpy(info->s = device_temp_str(), "floppydisk2"); break;
		case DEVINFO_STR_NAME+3:						strcpy(info->s = device_temp_str(), "floppydisk3"); break;
		case DEVINFO_STR_SHORT_NAME+0:					strcpy(info->s = device_temp_str(), "flop0"); break;
		case DEVINFO_STR_SHORT_NAME+1:					strcpy(info->s = device_temp_str(), "flop1"); break;
		case DEVINFO_STR_SHORT_NAME+2:					strcpy(info->s = device_temp_str(), "flop2"); break;
		case DEVINFO_STR_SHORT_NAME+3:					strcpy(info->s = device_temp_str(), "flop3"); break;
		case DEVINFO_STR_DESCRIPTION+0:					strcpy(info->s = device_temp_str(), "Floppy #0"); break;
		case DEVINFO_STR_DESCRIPTION+1:					strcpy(info->s = device_temp_str(), "Floppy #1"); break;
		case DEVINFO_STR_DESCRIPTION+2:					strcpy(info->s = device_temp_str(), "Floppy #2"); break;
		case DEVINFO_STR_DESCRIPTION+3:					strcpy(info->s = device_temp_str(), "Floppy #3"); break;

		default:										floppy_device_getinfo(devclass, state, info); break;
	}
}

static PALETTE_INIT( dgnbeta )
{
	palette_set_colors(0, dgnbeta_palette, sizeof(dgnbeta_palette) / 3);
	memcpy(colortable, dgnbeta_colortable, sizeof(colortable));
}

static MACHINE_DRIVER_START( dgnbeta )
	/* basic machine hardware */
	MDRV_CPU_ADD_TAG("main", M6809E, DGNBETA_CPU_SPEED_HZ)        /* 2 Mhz */
	MDRV_CPU_PROGRAM_MAP(dgnbeta_map,0)
	
	/* both cpus in the beta share the same address/data busses */
	MDRV_CPU_ADD_TAG("dma", M6809E, DGNBETA_CPU_SPEED_HZ)        /* 2 Mhz */
	MDRV_CPU_PROGRAM_MAP(dgnbeta_map,0)

	MDRV_FRAMES_PER_SECOND(DGNBETA_FRAMES_PER_SECOND)
	MDRV_VBLANK_DURATION(100)
	MDRV_CPU_VBLANK_INT(dgn_beta_frame_interrupt,1)		/* Call once / vblank */
	
	MDRV_MACHINE_START( dgnbeta )

	/* video hardware */
	
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(640, 480)
	MDRV_VISIBLE_AREA(0, 640 - 1, 0, 480 - 1)
	MDRV_GFXDECODE( dgnbeta_gfxdecodeinfo )
	MDRV_PALETTE_LENGTH(sizeof (dgnbeta_palette) / sizeof (dgnbeta_palette[0]) / 3)
	MDRV_COLORTABLE_LENGTH(sizeof (dgnbeta_colortable) / sizeof(dgnbeta_colortable[0][0]))
	MDRV_PALETTE_INIT( dgnbeta )

	MDRV_VIDEO_START( generic )
	MDRV_VIDEO_UPDATE( dgnbeta )
MACHINE_DRIVER_END

ROM_START(dgnbeta)
	ROM_REGION(0x4000,REGION_CPU1,0)
	ROM_LOAD("beta_bt.rom"	,0x0000	,0x4000	,CRC(4c54c1de) SHA1(141d9fcd2d187c305dff83fce2902a30072aed76))

	ROM_REGION (0x2000, REGION_GFX1, 0)
	ROM_LOAD("betachar.rom"	,0x0000	,0x2000	,CRC(ca79d66c) SHA1(8e2090d471dd97a53785a7f44a49d3c8c85b41f2)) 	
ROM_END

SYSTEM_CONFIG_START(dgnbeta)
	CONFIG_RAM(RamSize * 1024)
	CONFIG_DEVICE( dgnbeta_floppy_getinfo )
SYSTEM_CONFIG_END

/*     YEAR	NAME		PARENT	COMPAT		MACHINE    	INPUT		INIT    	CONFIG		COMPANY			FULLNAME */
COMP(  1984,	dgnbeta,	0,	0,		dgnbeta,	dgnbeta,	0,	dgnbeta,	"Dragon Data Ltd",    "Dragon Beta Prototype" , 0)
