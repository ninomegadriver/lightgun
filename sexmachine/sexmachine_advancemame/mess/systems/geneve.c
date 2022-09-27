/*
	MESS Driver for the Myarc Geneve 9640.
	Raphael Nabet, 2003.

	The Geneve has two operation modes.  One is compatible with the TI-99/4a,
	the other is not.


	General architecture:

	TMS9995@12MHz (including 256 bytes of on-chip 16-bit RAM and a timer),
	V9938, SN76496 (compatible with TMS9919), TMS9901, MM58274 RTC, 512 kbytes
	of 1-wait-state CPU RAM (expandable to almost 2 Mbytes), 32 kbytes of
	0-wait-state CPU RAM (expandable to 64 kbytes), 128 kbytes of VRAM
	(expandable to 192 kbytes).


	Memory map:

	64-kbyte CPU address space is mapped to a 2-Mbyte virtual address space.
	8 8-kbyte pages are available simultaneously, out of a total of 256.

	Page map (regular console):
	>00->3f: 512kbytes of CPU RAM (pages >36 and >37 are used to emulate
	  cartridge CPU ROMs in ti99 mode, and pages >38 through >3f are used to
	  emulate console and cartridge GROMs in ti99 mode)
	>40->7f: optional Memex RAM (except >7a and >7c that are mirrors of >ba and
	  >bc?)
	>80->b7: PE-bus space using spare address lines (AMA-AMC)?  Used by RAM
		extension (Memex or PE-Bus 512k card).
	>b8->bf: PE-bus space (most useful pages are >ba: DSR space and >bc: speech
	  synthesizer page; other pages are used by RAM extension)
	>c0->e7: optional Memex RAM
	>e8->ef: 64kbytes of 0-wait-state RAM (with 32kbytes of SRAM installed,
	  only even pages (>e8, >ea, >ec and >ee) are available)
	>f0->f1: boot ROM
	>f2->fe: optional Memex RAM? (except >fa and >fc that are mirrors of >ba
	  and >bc?)
	>ff: always empty?

	Page map (genmod console):
	>00->39, >40->b9, >bb, >bd->ef, f2->fe: Memex RAM (except >3a, >3c, >7a,
	  >7c, >fa and >fc that are mirrors of >ba and >bc?) (I don't know if
	  >e8->ef is on the Memex board or the Geneve motherboard)
	>ba: DSR space
	>bc: speech synthesizer space
	>f0->f1: boot ROM
	>ff: always empty?

	>00->3f: switch-selectable(?): 512kbytes of onboard RAM (1-wait-state DRAM)
	  or first 512kbytes of the Memex Memory board (0-wait-state SRAM).  The
	  latter setting is incompatible with TI mode.

	Note that >bc is actually equivalent to >8000->9fff on the /4a bus,
	therefore the speech synthesizer is only available at offsets >1800->1bff
	(read port) and >1c00->1fff (write port).  OTOH, if you installed a FORTI
	sound card, it will be available in the same page at offsets >0400->7ff.


	Unpaged locations (ti99 mode):
	>8000->8007: memory page registers (>8000 for page 0, >8001 for page 1,
	  etc.  register >8003 is ignored (this page is hard-wired to >36->37).
	>8008: key buffer
	>8009->800f: ???
	>8010->801f: clock chip
	>8400->9fff: sound, VDP, speech, and GROM registers (according to one
	  source, the GROM and sound registers are only available if page >3
	  is mapped in at location >c000 (register 6).  I am not sure the Speech
	  registers are implemented, though I guess they are.)
	Note that >8020->83ff is mapped to regular CPU RAM according to map
	register 4.

	Unpaged locations (native mode):
	>f100: VDP data port (read/write)
	>f102: VDP address/status port (read/write)
	>f104: VDP port 2
	>f106: VDP port 3
	>f108->f10f: VDP mirror (used by Barry Boone's converted Tomy cartridges)
	>f110->f117: memory page registers (>f110 for page 0, >f111 for page 1,
	  etc.)
	>f118: key buffer
	>f119->f11f: ???
	>f120: sound chip
	>f121->f12f: ???
	>f130->f13f: clock chip

	Unpaged locations (tms9995 locations):
	>f000->f0fb and >fffc->ffff: tms9995 RAM
	>fffa->fffb: tms9995 timer register
	Note: accessing tms9995 locations will also read/write corresponding paged
	  locations.


	GROM emulator:

	The GPL interface is accessible only in TI99 mode.  GPL data is fetched
	from pages >38->3f.  It uses a straight 16-bit address pointer.  The
	address pointer is incremented when the read data and write data ports
	are accessed, and when the second byte of the GPL address is written.  A
	weird consequence of this is the fact that GPL data is always off by one
	byte, i.e. GPL bytes 0, 1, 2... are stored in bytes 1, 2, 3... of pages 
	>38->3f (byte 0 of page >38 corresponds to GPL address >ffff).

	I think that I have once read that the geneve GROM emulator does not
	emulate wrap-around within a GROM, i.e. address >1fff is followed by >2000
	(instead of >0000 with a real GROM).

	There are two ways to implement GPL address load and store.
	One is maintaining 2 flags (one for address read and one for address write)
	that tell if you are accessing address LSB: these flags must be cleared
	whenever data is read, and the read flag must be cleared when the write
	address port is written to.
	The other is to shift the register and always read/write the MSByte of the
	address pointer.  The LSByte is copied to the MSbyte when the address is
	read, whereas the former MSByte is copied to the LSByte when the address is
	written.  The address pointer must be incremented after the address is
	written to.  It will not harm if it is incremented after the address is
	read, provided the LSByte has been cleared.


	Cartridge emulator:

	The cartridge interface is in the >6000->7fff area.

	If CRU bit @>F7C is set, the cartridge area is always mapped to virtual
	page >36.  Writes in the >6000->6fff area are ignored if the CRU bit @>F7D
	is 0, whereas writes in the >7000->7fff area are ignored if the CRU bit
	@>F7E is 0.

	If CRU bit @>F7C is clear, the cartridge area is mapped either to virtual
	page >36 or to page >37 according to which page is currently selected.
	Writing data to address >6000->7fff will select page >36 if A14 is 0,
	>37 if A14 is 1.


	CRU map:

	Base >0000: tms9901
	tms9901 pin assignment:
	int1: external interrupt (INTA on PE-bus) (connected to tms9995 (int4/EC)*
	  pin as well)
	int2: VDP interrupt
	int3-int7: joystick port inputs (fire, left, right, down, up)
	int8: keyboard interrupt (asserted when key buffer full)
	int9/p13: unused
	int10/p12: third mouse button
	int11/p11: clock interrupt?
	int12/p10: INTB from PE-bus
	int13/p9: (used as output)
	int14/p8: unused
	int15/p7: (used as output)
	p0: PE-bus reset line
	p1: VDP reset line
	p2: joystick select (0 for joystick 1, 1 for joystick 2)
	p3-p5: unused
	p6: keyboard reset line
	p7/int15: external mem cycles 0=long, 1=short
	p9/int13: vdp wait cycles 1=add 15 cycles, 0=add none

	Base >1EE0 (>0F70): tms9995 flags and geneve mode register
	bits 0-1: tms9995 flags
	Bits 2-4: tms9995 interrupt state register
	Bits 5-15: tms9995 user flags - overlaps geneve mode, but hopefully the
	  geneve registers are write-only, so no real conflict happens
	TMS9995 user flags:
	Bit 5: 0 if NTSC, 1 if PAL video
	Bit 6: unused???
	Bit 7: some keyboard flag, set to 1 if caps is on
	Geneve gate array + TMS9995 user flags:
	Bit 8: 1 = allow keyboard clock
	Bit 9: 0 = clear keyboard input buffer, 1 = allow keyboard buffer to be
	  loaded
	Bit 10: 1 = geneve mode, 0 = ti99 mode
	Bit 11: 1 = ROM mode, 0 = map mode
	Bit 12: 0 = Enable cartridge paging
	Bit 13: 0 = protect cartridge range >6000->6fff
	Bit 14: 0 = protect cartridge range >7000->7fff
	bit 15: 1 = add 1 extra wait state when accessing 0-wait-state SRAM???


	Keyboard interface:

	The XT keyboard interface is described in various places on the internet,
	like (http://www-2.cs.cmu.edu/afs/cs/usr/jmcm/www/info/key2.txt).  It is a
	synchronous unidirectional serial interface: the data line is driven by the
	keyboard to send data to the CPU; the CTS/clock line has a pull up resistor
	and can be driven low by both keyboard and CPU.  To send data to the CPU,
	the keyboard pulses the clock line low 9 times, and the Geneve samples all
	8 bits of data (plus one start bit) on each falling edge of the clock.
	When the key code buffer is full, the Geneve gate array asserts the kbdint*
	line (connected to 9901 INT8*).  The Geneve gate array will hold the
	CTS/clock line low as long as the keyboard buffer is full or CRU bit @>F78
	is 0.  Writing a 0 to >F79 will clear the Geneve keyboard buffer, and
	writing a 1 will resume normal operation: you need to write a 0 to >F78
	before clearing >F79, or the keyboard will be enabled to send data the gate
	array when >F79 is is set to 0, and any such incoming data from the
	keyboard will be cleared as soon as it is buffered by the gate array.
*/

#include "driver.h"
#include "inputx.h"
#include "vidhrdw/generic.h"
#include "vidhrdw/v9938.h"

#include "includes/geneve.h"
#include "machine/ti99_4x.h"
#include "machine/tms9901.h"
#include "sndhrdw/spchroms.h"
#include "machine/99_peb.h"
#include "machine/994x_ser.h"
#include "machine/99_dsk.h"
#include "machine/99_ide.h"
#include "devices/mflopimg.h"
#include "machine/smartmed.h"
#include "sound/5220intf.h"
#include "devices/harddriv.h"

/*
	memory map
*/

static ADDRESS_MAP_START(memmap, ADDRESS_SPACE_PROGRAM, 8)

	AM_RANGE(0x0000, 0xffff) AM_READWRITE(geneve_r, geneve_w)

ADDRESS_MAP_END


/*
	CRU map
*/

static ADDRESS_MAP_START(writecru, ADDRESS_SPACE_IO, 8)

	AM_RANGE(0x0000, 0x07ff) AM_WRITE(tms9901_0_cru_w)
	AM_RANGE(0x0800, 0x0fff) AM_WRITE(geneve_peb_mode_cru_w)

ADDRESS_MAP_END

static ADDRESS_MAP_START(readcru, ADDRESS_SPACE_IO, 8)

	AM_RANGE(0x0000, 0x00ff) AM_READ(tms9901_0_cru_r)
	AM_RANGE(0x0100, 0x01ff) AM_READ(geneve_peb_cru_r)

ADDRESS_MAP_END


/*
	Input ports, used by machine code for keyboard and joystick emulation.
*/

#define AT_KEYB_HELPER(bit, text, key1) \
	PORT_BIT( bit, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME(text) PORT_CODE(key1)

/* 101-key PC XT keyboard + TI joysticks */
INPUT_PORTS_START(geneve)

	PORT_START	/* config */
		PORT_BIT( config_speech_mask << config_speech_bit, 1 << config_speech_bit, IPT_DIPSWITCH_NAME) PORT_NAME("Speech synthesis")
			PORT_DIPSETTING( 0x0000, DEF_STR( Off ) )
			PORT_DIPSETTING( 1 << config_speech_bit, DEF_STR( On ) )
		PORT_BIT( config_fdc_mask << config_fdc_bit, fdc_kind_hfdc << config_fdc_bit, IPT_DIPSWITCH_NAME) PORT_NAME("Floppy disk controller")
			PORT_DIPSETTING( fdc_kind_none << config_fdc_bit, DEF_STR( None ) )
			PORT_DIPSETTING( fdc_kind_TI << config_fdc_bit, "Texas Instruments SD" )
#if HAS_99CCFDC
			PORT_DIPSETTING( fdc_kind_CC << config_fdc_bit, "CorComp" )
#endif
			PORT_DIPSETTING( fdc_kind_BwG << config_fdc_bit, "SNUG's BwG" )
			PORT_DIPSETTING( fdc_kind_hfdc << config_fdc_bit, "Myarc's HFDC" )
		PORT_BIT( config_ide_mask << config_ide_bit, /*1 << config_ide_bit*/0, IPT_DIPSWITCH_NAME) PORT_NAME("Nouspickel's IDE card")
			PORT_DIPSETTING( 0x0000, DEF_STR( Off ) )
			PORT_DIPSETTING( 1 << config_ide_bit, DEF_STR( On ) )
		PORT_BIT( config_rs232_mask << config_rs232_bit, 1 << config_rs232_bit, IPT_DIPSWITCH_NAME) PORT_NAME("TI RS232 card")
			PORT_DIPSETTING( 0x0000, DEF_STR( Off ) )
			PORT_DIPSETTING( 1 << config_rs232_bit, DEF_STR( On ) )
		PORT_BIT( config_usbsm_mask << config_usbsm_bit, 1 << config_usbsm_bit, IPT_DIPSWITCH_NAME) PORT_NAME("Nouspickel's USB-SM card")
			PORT_DIPSETTING( 0x0000, DEF_STR( Off ) )
			PORT_DIPSETTING( 1 << config_usbsm_bit, DEF_STR( On ) )

	PORT_START	/* col 1: "wired handset 1" (= joystick 1) */
		PORT_BIT(0x0080, IP_ACTIVE_LOW, IPT_JOYSTICK_UP/*, "(1UP)", CODE_NONE, OSD_JOY_UP*/) PORT_PLAYER(1)
		PORT_BIT(0x0040, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN/*, "(1DOWN)", CODE_NONE, OSD_JOY_DOWN, 0*/) PORT_PLAYER(1)
		PORT_BIT(0x0020, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT/*, "(1RIGHT)", CODE_NONE, OSD_JOY_RIGHT, 0*/) PORT_PLAYER(1)
		PORT_BIT(0x0010, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT/*, "(1LEFT)", CODE_NONE, OSD_JOY_LEFT, 0*/) PORT_PLAYER(1)
		PORT_BIT(0x0008, IP_ACTIVE_LOW, IPT_BUTTON1/*, "(1FIRE)", CODE_NONE, OSD_JOY_FIRE, 0*/) PORT_PLAYER(1)
		PORT_BIT(0x0007, IP_ACTIVE_HIGH, IPT_UNUSED)
			/* col 2: "wired handset 2" (= joystick 2) */
		PORT_BIT(0x8000, IP_ACTIVE_LOW, IPT_JOYSTICK_UP/*, "(2UP)", CODE_NONE, OSD_JOY2_UP, 0*/) PORT_PLAYER(2)
		PORT_BIT(0x4000, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN/*, "(2DOWN)", CODE_NONE, OSD_JOY2_DOWN, 0*/) PORT_PLAYER(2)
		PORT_BIT(0x2000, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT/*, "(2RIGHT)", CODE_NONE, OSD_JOY2_RIGHT, 0*/) PORT_PLAYER(2)
		PORT_BIT(0x1000, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT/*, "(2LEFT)", CODE_NONE, OSD_JOY2_LEFT, 0*/) PORT_PLAYER(2)
		PORT_BIT(0x0800, IP_ACTIVE_LOW, IPT_BUTTON1/*, "(2FIRE)", CODE_NONE, OSD_JOY2_FIRE, 0*/) PORT_PLAYER(2)
		PORT_BIT(0x0700, IP_ACTIVE_HIGH, IPT_UNUSED)

	PORT_START /* mouse buttons */
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_BUTTON1) PORT_NAME("mouse button 1")
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_BUTTON2) PORT_NAME("mouse button 2")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_BUTTON3) PORT_NAME("mouse button 3")

	PORT_START /* Mouse - X AXIS */
	PORT_BIT( 0xff, 0x00, IPT_TRACKBALL_X) PORT_SENSITIVITY(100) PORT_KEYDELTA(0) PORT_MINMAX(0,0) PORT_PLAYER(1)

	PORT_START /* Mouse - Y AXIS */
	PORT_BIT( 0xff, 0x00, IPT_TRACKBALL_Y) PORT_SENSITIVITY(100) PORT_KEYDELTA(0) PORT_MINMAX(0,0) PORT_PLAYER(1)

	PORT_START	/* IN3 */
	PORT_BIT ( 0x0001, 0x0000, IPT_UNUSED ) 	/* unused scancode 0 */
	AT_KEYB_HELPER( 0x0002, "Esc",			KEYCODE_ESC			) /* Esc						01	81 */
	AT_KEYB_HELPER( 0x0004, "1 !",			KEYCODE_1			) /* 1							02	82 */
	AT_KEYB_HELPER( 0x0008, "2 @",			KEYCODE_2			) /* 2							03	83 */
	AT_KEYB_HELPER( 0x0010, "3 #",			KEYCODE_3			) /* 3							04	84 */
	AT_KEYB_HELPER( 0x0020, "4 $",			KEYCODE_4			) /* 4							05	85 */
	AT_KEYB_HELPER( 0x0040, "5 %",			KEYCODE_5			) /* 5							06	86 */
	AT_KEYB_HELPER( 0x0080, "6 ^",			KEYCODE_6			) /* 6							07	87 */
	AT_KEYB_HELPER( 0x0100, "7 &",			KEYCODE_7			) /* 7							08	88 */
	AT_KEYB_HELPER( 0x0200, "8 *",			KEYCODE_8			) /* 8							09	89 */
	AT_KEYB_HELPER( 0x0400, "9 (",			KEYCODE_9			) /* 9							0A	8A */
	AT_KEYB_HELPER( 0x0800, "0 )",			KEYCODE_0			) /* 0							0B	8B */
	AT_KEYB_HELPER( 0x1000, "- _",			KEYCODE_MINUS		) /* -							0C	8C */
	AT_KEYB_HELPER( 0x2000, "= +",			KEYCODE_EQUALS		) /* =							0D	8D */
	AT_KEYB_HELPER( 0x4000, "Backspace",	KEYCODE_BACKSPACE	) /* Backspace					0E	8E */
	AT_KEYB_HELPER( 0x8000, "Tab",			KEYCODE_TAB			) /* Tab						0F	8F */

	PORT_START	/* IN4 */
	AT_KEYB_HELPER( 0x0001, "Q",			KEYCODE_Q			) /* Q							10	90 */
	AT_KEYB_HELPER( 0x0002, "W",			KEYCODE_W			) /* W							11	91 */
	AT_KEYB_HELPER( 0x0004, "E",			KEYCODE_E			) /* E							12	92 */
	AT_KEYB_HELPER( 0x0008, "R",			KEYCODE_R			) /* R							13	93 */
	AT_KEYB_HELPER( 0x0010, "T",			KEYCODE_T			) /* T							14	94 */
	AT_KEYB_HELPER( 0x0020, "Y",			KEYCODE_Y			) /* Y							15	95 */
	AT_KEYB_HELPER( 0x0040, "U",			KEYCODE_U			) /* U							16	96 */
	AT_KEYB_HELPER( 0x0080, "I",			KEYCODE_I			) /* I							17	97 */
	AT_KEYB_HELPER( 0x0100, "O",			KEYCODE_O			) /* O							18	98 */
	AT_KEYB_HELPER( 0x0200, "P",			KEYCODE_P			) /* P							19	99 */
	AT_KEYB_HELPER( 0x0400, "[ {",			KEYCODE_OPENBRACE	) /* [							1A	9A */
	AT_KEYB_HELPER( 0x0800, "] }",			KEYCODE_CLOSEBRACE	) /* ]							1B	9B */
	AT_KEYB_HELPER( 0x1000, "Enter",		KEYCODE_ENTER		) /* Enter						1C	9C */
	AT_KEYB_HELPER( 0x2000, "L-Ctrl",		KEYCODE_LCONTROL	) /* Left Ctrl					1D	9D */
	AT_KEYB_HELPER( 0x4000, "A",			KEYCODE_A			) /* A							1E	9E */
	AT_KEYB_HELPER( 0x8000, "S",			KEYCODE_S			) /* S							1F	9F */

	PORT_START	/* IN5 */
	AT_KEYB_HELPER( 0x0001, "D",			KEYCODE_D			) /* D							20	A0 */
	AT_KEYB_HELPER( 0x0002, "F",			KEYCODE_F			) /* F							21	A1 */
	AT_KEYB_HELPER( 0x0004, "G",			KEYCODE_G			) /* G							22	A2 */
	AT_KEYB_HELPER( 0x0008, "H",			KEYCODE_H			) /* H							23	A3 */
	AT_KEYB_HELPER( 0x0010, "J",			KEYCODE_J			) /* J							24	A4 */
	AT_KEYB_HELPER( 0x0020, "K",			KEYCODE_K			) /* K							25	A5 */
	AT_KEYB_HELPER( 0x0040, "L",			KEYCODE_L			) /* L							26	A6 */
	AT_KEYB_HELPER( 0x0080, "; :",			KEYCODE_COLON		) /* ;							27	A7 */
	AT_KEYB_HELPER( 0x0100, "' \"",			KEYCODE_QUOTE		) /* '							28	A8 */
	AT_KEYB_HELPER( 0x0200, "` ~",			KEYCODE_TILDE		) /* `							29	A9 */
	AT_KEYB_HELPER( 0x0400, "L-Shift",		KEYCODE_LSHIFT		) /* Left Shift					2A	AA */
	AT_KEYB_HELPER( 0x0800, "\\ |",			KEYCODE_BACKSLASH	) /* \							2B	AB */
	AT_KEYB_HELPER( 0x1000, "Z",			KEYCODE_Z			) /* Z							2C	AC */
	AT_KEYB_HELPER( 0x2000, "X",			KEYCODE_X			) /* X							2D	AD */
	AT_KEYB_HELPER( 0x4000, "C",			KEYCODE_C			) /* C							2E	AE */
	AT_KEYB_HELPER( 0x8000, "V",			KEYCODE_V			) /* V							2F	AF */

	PORT_START	/* IN6 */
	AT_KEYB_HELPER( 0x0001, "B",			KEYCODE_B			) /* B							30	B0 */
	AT_KEYB_HELPER( 0x0002, "N",			KEYCODE_N			) /* N							31	B1 */
	AT_KEYB_HELPER( 0x0004, "M",			KEYCODE_M			) /* M							32	B2 */
	AT_KEYB_HELPER( 0x0008, ", <",			KEYCODE_COMMA		) /* ,							33	B3 */
	AT_KEYB_HELPER( 0x0010, ". >",			KEYCODE_STOP		) /* .							34	B4 */
	AT_KEYB_HELPER( 0x0020, "/ ?",			KEYCODE_SLASH		) /* /							35	B5 */
	AT_KEYB_HELPER( 0x0040, "R-Shift",		KEYCODE_RSHIFT		) /* Right Shift				36	B6 */
	AT_KEYB_HELPER( 0x0080, "KP * (PrtScr)",KEYCODE_ASTERISK	) /* Keypad *  (PrtSc)			37	B7 */
	AT_KEYB_HELPER( 0x0100, "Alt",			KEYCODE_LALT		) /* Left Alt					38	B8 */
	AT_KEYB_HELPER( 0x0200, "Space",		KEYCODE_SPACE		) /* Space						39	B9 */
	AT_KEYB_HELPER( 0x0400, "Caps",			KEYCODE_CAPSLOCK	) /* Caps Lock					3A	BA */
	AT_KEYB_HELPER( 0x0800, "F1",			KEYCODE_F1			) /* F1							3B	BB */
	AT_KEYB_HELPER( 0x1000, "F2",			KEYCODE_F2			) /* F2							3C	BC */
	AT_KEYB_HELPER( 0x2000, "F3",			KEYCODE_F3			) /* F3							3D	BD */
	AT_KEYB_HELPER( 0x4000, "F4",			KEYCODE_F4			) /* F4							3E	BE */
	AT_KEYB_HELPER( 0x8000, "F5",			KEYCODE_F5			) /* F5							3F	BF */

	PORT_START	/* IN7 */
	AT_KEYB_HELPER( 0x0001, "F6",			KEYCODE_F6			) /* F6							40	C0 */
	AT_KEYB_HELPER( 0x0002, "F7",			KEYCODE_F7			) /* F7							41	C1 */
	AT_KEYB_HELPER( 0x0004, "F8",			KEYCODE_F8			) /* F8							42	C2 */
	AT_KEYB_HELPER( 0x0008, "F9",			KEYCODE_F9			) /* F9							43	C3 */
	AT_KEYB_HELPER( 0x0010, "F10",			KEYCODE_F10			) /* F10						44	C4 */
	AT_KEYB_HELPER( 0x0020, "NumLock",		KEYCODE_NUMLOCK		) /* Num Lock					45	C5 */
	AT_KEYB_HELPER( 0x0040, "ScrLock (F14)",KEYCODE_SCRLOCK		) /* Scroll Lock				46	C6 */
	AT_KEYB_HELPER( 0x0080, "KP 7 (Home)",	KEYCODE_7_PAD		) /* Keypad 7  (Home)			47	C7 */
	AT_KEYB_HELPER( 0x0100, "KP 8 (Up)",	KEYCODE_8_PAD		) /* Keypad 8  (Up arrow)		48	C8 */
	AT_KEYB_HELPER( 0x0200, "KP 9 (PgUp)",	KEYCODE_9_PAD		) /* Keypad 9  (PgUp)			49	C9 */
	AT_KEYB_HELPER( 0x0400, "KP -",			KEYCODE_MINUS_PAD	) /* Keypad -					4A	CA */
	AT_KEYB_HELPER( 0x0800, "KP 4 (Left)",	KEYCODE_4_PAD		) /* Keypad 4  (Left arrow)		4B	CB */
	AT_KEYB_HELPER( 0x1000, "KP 5",			KEYCODE_5_PAD		) /* Keypad 5					4C	CC */
	AT_KEYB_HELPER( 0x2000, "KP 6 (Right)",	KEYCODE_6_PAD		) /* Keypad 6  (Right arrow)	4D	CD */
	AT_KEYB_HELPER( 0x4000, "KP +",			KEYCODE_PLUS_PAD	) /* Keypad +					4E	CE */
	AT_KEYB_HELPER( 0x8000, "KP 1 (End)",	KEYCODE_1_PAD		) /* Keypad 1  (End)			4F	CF */

	PORT_START	/* IN8 */
	AT_KEYB_HELPER( 0x0001, "KP 2 (Down)",	KEYCODE_2_PAD		) /* Keypad 2  (Down arrow)		50	D0 */
	AT_KEYB_HELPER( 0x0002, "KP 3 (PgDn)",	KEYCODE_3_PAD		) /* Keypad 3  (PgDn)			51	D1 */
	AT_KEYB_HELPER( 0x0004, "KP 0 (Ins)",	KEYCODE_0_PAD		) /* Keypad 0  (Ins)			52	D2 */
	AT_KEYB_HELPER( 0x0008, "KP . (Del)",	KEYCODE_DEL_PAD		) /* Keypad .  (Del)			53	D3 */
	PORT_BIT ( 0x0030, 0x0000, IPT_UNUSED )
	AT_KEYB_HELPER( 0x0040, "(84/102)\\",	KEYCODE_BACKSLASH2	) /* Backslash 2				56	D6 */
	AT_KEYB_HELPER( 0x0080, "(101)F11",		KEYCODE_F11			) /* F11						57	D7 */
	AT_KEYB_HELPER( 0x0100, "(101)F12",		KEYCODE_F12			) /* F12						58	D8 */
	PORT_BIT ( 0xfe00, 0x0000, IPT_UNUSED )

	PORT_START	/* IN9 */
	AT_KEYB_HELPER( 0x0001, "(101)KP Enter",	KEYCODE_ENTER_PAD	) /* PAD Enter					60	e0 */
	AT_KEYB_HELPER( 0x0002, "(101)R-Control",	KEYCODE_RCONTROL	) /* Right Control				61	e1 */
	AT_KEYB_HELPER( 0x0004, "(101)ALTGR",		KEYCODE_RALT		) /* ALTGR						64	e4 */

	AT_KEYB_HELPER( 0x0008, "(101)KP /",		KEYCODE_SLASH_PAD	) /* PAD Slash					62	e2 */

	AT_KEYB_HELPER( 0x0010, "(101)Home",		KEYCODE_HOME		) /* Home						66	e6 */
	AT_KEYB_HELPER( 0x0020, "(101)Cursor Up",	KEYCODE_UP			) /* Up							67	e7 */
	AT_KEYB_HELPER( 0x0040, "(101)Page Up",		KEYCODE_PGUP		) /* Page Up					68	e8 */
	AT_KEYB_HELPER( 0x0080, "(101)Cursor Left",	KEYCODE_LEFT		) /* Left						69	e9 */
	AT_KEYB_HELPER( 0x0100, "(101)Cursor Right",KEYCODE_RIGHT		) /* Right						6a	ea */
	AT_KEYB_HELPER( 0x0200, "(101)End",			KEYCODE_END			) /* End						6b	eb */
	AT_KEYB_HELPER( 0x0400, "(101)Cursor Down",	KEYCODE_DOWN		) /* Down						6c	ec */
	AT_KEYB_HELPER( 0x0800, "(101)Page Down",	KEYCODE_PGDN		) /* Page Down					6d	ed */
	AT_KEYB_HELPER( 0x1000, "(101)Insert",		KEYCODE_INSERT		) /* Insert						6e	ee */
	AT_KEYB_HELPER( 0x2000, "(101)Delete",		KEYCODE_DEL			) /* Delete						6f	ef */

	AT_KEYB_HELPER( 0x4000, "(101)PrtScr (F13)",KEYCODE_PRTSCR		) /* Print Screen				63	e3 */
	AT_KEYB_HELPER( 0x8000, "(101)Pause (F15)",	KEYCODE_PAUSE		) /* Pause						65	e5 */

	PORT_START	/* IN10 */
	PORT_BIT ( 0xffff, 0x0000, IPT_UNUSED )
#if 0
	AT_KEYB_HELPER( 0x0001, "Print Screen", KEYCODE_PRTSCR		) /* Print Screen alternate		77	f7 */
	AT_KEYB_HELPER( 0x2000, "Left Win",		CODE_NONE			) /* Left Win					7d	fd */
	AT_KEYB_HELPER( 0x4000, "Right Win",	CODE_NONE			) /* Right Win					7e	fe */
	AT_KEYB_HELPER( 0x8000, "Menu",			CODE_NONE			) /* Menu						7f	ff */
#endif

INPUT_PORTS_END




static struct TMS5220interface tms5220interface =
{
	NULL,						/* no IRQ callback */
#if 1
	spchroms_read,				/* speech ROM read handler */
	spchroms_load_address,		/* speech ROM load address handler */
	spchroms_read_and_branch	/* speech ROM read and branch handler */
#endif
};


static MACHINE_DRIVER_START(geneve_60hz)

	/* basic machine hardware */
	/* TMS9995 CPU @ 12.0 MHz */
	MDRV_CPU_ADD(TMS9995, 12000000)
	/*MDRV_CPU_CONFIG(0)*/
	MDRV_CPU_PROGRAM_MAP(memmap, 0)
	MDRV_CPU_IO_MAP(readcru, writecru)
	MDRV_CPU_VBLANK_INT(geneve_hblank_interrupt, 262)	/* 262.5 in 60Hz, 312.5 in 50Hz */
	/*MDRV_CPU_PERIODIC_INT(func, rate)*/

	MDRV_FRAMES_PER_SECOND(60)	/* or 50Hz */
	MDRV_VBLANK_DURATION(DEFAULT_REAL_60HZ_VBLANK_DURATION)
	/*MDRV_INTERLEAVE(interleave)*/

	MDRV_MACHINE_START( geneve )

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	/*MDRV_ASPECT_RATIO(num, den)*/
	MDRV_SCREEN_SIZE(512 + 32, (212 + 28) * 2)
	MDRV_VISIBLE_AREA(0, 512 + 32 - 1, 0, (212 + 28) * 2 - 1)

	/*MDRV_GFXDECODE(NULL)*/
	MDRV_PALETTE_LENGTH(512)
	MDRV_COLORTABLE_LENGTH(512)

	MDRV_PALETTE_INIT(v9938)
	MDRV_VIDEO_START(geneve)
	/*MDRV_VIDEO_EOF(name)*/
	MDRV_VIDEO_UPDATE(generic_bitmapped)

	MDRV_SPEAKER_STANDARD_MONO("mono")
	MDRV_SOUND_ADD(SN76496, 3579545)	/* 3.579545 MHz */
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.75)
	MDRV_SOUND_ADD(TMS5220, 680000L)
	MDRV_SOUND_CONFIG(tms5220interface)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.50)

MACHINE_DRIVER_END


/*
	ROM loading

	Note that we use the same ROMset for 50Hz and 60Hz versions.
*/

ROM_START(geneve)
	/*CPU memory space*/
	ROM_REGION(region_cpu1_len_geneve, REGION_CPU1, 0)
	ROM_LOAD("genbt100.bin", offset_rom_geneve, 0x4000, CRC(8001e386) SHA1(b44618b54dabac3882543e18555d482b299e0109)) /* CPU ROMs */

	/*DSR ROM space*/
	ROM_REGION(region_dsr_len, region_dsr, 0)
	ROM_LOAD_OPTIONAL("disk.bin", offset_fdc_dsr, 0x2000, CRC(8f7df93f) SHA1(ed91d48c1eaa8ca37d5055bcf67127ea51c4cad5)) /* TI disk DSR ROM */
#if HAS_99CCFDC
	ROM_LOAD_OPTIONAL("ccfdc.bin", offset_ccfdc_dsr, 0x4000, BAD_DUMP CRC(f69cc69d)) /* CorComp disk DSR ROM */
#endif
	ROM_LOAD_OPTIONAL("bwg.bin", offset_bwg_dsr, 0x8000, CRC(06f1ec89) SHA1(6ad77033ed268f986d9a5439e65f7d391c4b7651)) /* BwG disk DSR ROM */
	ROM_LOAD_OPTIONAL("hfdc.bin", offset_hfdc_dsr, 0x4000, CRC(66fbe0ed) SHA1(11df2ecef51de6f543e4eaf8b2529d3e65d0bd59)) /* HFDC disk DSR ROM */
	ROM_LOAD_OPTIONAL("rs232.bin", offset_rs232_dsr, 0x1000, CRC(eab382fb) SHA1(ee609a18a21f1a3ddab334e8798d5f2a0fcefa91)) /* TI rs232 DSR ROM */

	/*TMS5220 ROM space*/
	ROM_REGION(0x8000, region_speech_rom, 0)
	ROM_LOAD_OPTIONAL("spchrom.bin", 0x0000, 0x8000, CRC(58b155f7) SHA1(382292295c00dff348d7e17c5ce4da12a1d87763)) /* system speech ROM */
ROM_END

ROM_START(genmod)
	/*CPU memory space*/
	ROM_REGION(region_cpu1_len_geneve, REGION_CPU1, 0)
	ROM_LOAD("gnmbt100.bin", offset_rom_geneve, 0x4000, CRC(19b89479) SHA1(6ef297eda78dc705946f6494e9d7e95e5216ec47)) /* CPU ROMs */

	/*DSR ROM space*/
	ROM_REGION(region_dsr_len, region_dsr, 0)
	ROM_LOAD_OPTIONAL("disk.bin", offset_fdc_dsr, 0x2000, CRC(8f7df93f) SHA1(ed91d48c1eaa8ca37d5055bcf67127ea51c4cad5)) /* TI disk DSR ROM */
#if HAS_99CCFDC
	ROM_LOAD_OPTIONAL("ccfdc.bin", offset_ccfdc_dsr, 0x4000, BAD_DUMP CRC(f69cc69d)) /* CorComp disk DSR ROM */
#endif
	ROM_LOAD_OPTIONAL("bwg.bin", offset_bwg_dsr, 0x8000, CRC(06f1ec89) SHA1(6ad77033ed268f986d9a5439e65f7d391c4b7651)) /* BwG disk DSR ROM */
	ROM_LOAD_OPTIONAL("hfdc.bin", offset_hfdc_dsr, 0x4000, CRC(66fbe0ed) SHA1(11df2ecef51de6f543e4eaf8b2529d3e65d0bd59)) /* HFDC disk DSR ROM */
	ROM_LOAD_OPTIONAL("rs232.bin", offset_rs232_dsr, 0x1000, CRC(eab382fb) SHA1(ee609a18a21f1a3ddab334e8798d5f2a0fcefa91)) /* TI rs232 DSR ROM */

	/*TMS5220 ROM space*/
	ROM_REGION(0x8000, region_speech_rom, 0)
	ROM_LOAD_OPTIONAL("spchrom.bin", 0x0000, 0x8000, CRC(58b155f7) SHA1(382292295c00dff348d7e17c5ce4da12a1d87763)) /* system speech ROM */
ROM_END

static void geneve_floppy_getinfo(const device_class *devclass, UINT32 state, union devinfo *info)
{
	/* floppy */
	switch(state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case DEVINFO_INT_COUNT:							info->i = 4; break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case DEVINFO_PTR_FLOPPY_OPTIONS:				info->p = (void *) floppyoptions_ti99; break;

		default:										floppy_device_getinfo(devclass, state, info); break;
	}
}

static void geneve_harddisk_getinfo(const device_class *devclass, UINT32 state, union devinfo *info)
{
	/* harddisk */
	switch(state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case DEVINFO_INT_TYPE:							info->i = IO_HARDDISK; break;
		case DEVINFO_INT_READABLE:						info->i = 1; break;
		case DEVINFO_INT_WRITEABLE:						info->i = 1; break;
		case DEVINFO_INT_CREATABLE:						info->i = 0; break;
		case DEVINFO_INT_COUNT:							info->i = 3; break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case DEVINFO_PTR_INIT:							info->init = device_init_mess_hd; break;
		case DEVINFO_PTR_LOAD:							info->load = device_load_ti99_hd; break;
		case DEVINFO_PTR_UNLOAD:						info->unload = device_unload_ti99_hd; break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case DEVINFO_STR_FILE_EXTENSIONS:				strcpy(info->s = device_temp_str(), "hd"); break;
	}
}

static void geneve_parallel_getinfo(const device_class *devclass, UINT32 state, union devinfo *info)
{
	/* parallel */
	switch(state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case DEVINFO_INT_TYPE:							info->i = IO_PARALLEL; break;
		case DEVINFO_INT_READABLE:						info->i = 1; break;
		case DEVINFO_INT_WRITEABLE:						info->i = 1; break;
		case DEVINFO_INT_CREATABLE:						info->i = 1; break;
		case DEVINFO_INT_COUNT:							info->i = 1; break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case DEVINFO_PTR_LOAD:							info->load = device_load_ti99_4_pio; break;
		case DEVINFO_PTR_UNLOAD:						info->unload = device_unload_ti99_4_pio; break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case DEVINFO_STR_FILE_EXTENSIONS:				strcpy(info->s = device_temp_str(), ""); break;
	}
}

static void geneve_serial_getinfo(const device_class *devclass, UINT32 state, union devinfo *info)
{
	/* serial */
	switch(state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case DEVINFO_INT_TYPE:							info->i = IO_SERIAL; break;
		case DEVINFO_INT_READABLE:						info->i = 1; break;
		case DEVINFO_INT_WRITEABLE:						info->i = 1; break;
		case DEVINFO_INT_CREATABLE:						info->i = 1; break;
		case DEVINFO_INT_COUNT:							info->i = 1; break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case DEVINFO_PTR_LOAD:							info->load = device_load_ti99_4_rs232; break;
		case DEVINFO_PTR_UNLOAD:						info->unload = device_unload_ti99_4_rs232; break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case DEVINFO_STR_FILE_EXTENSIONS:				strcpy(info->s = device_temp_str(), ""); break;
	}
}

static void geneve_memcard_getinfo(const device_class *devclass, UINT32 state, union devinfo *info)
{
	/* memcard */
	switch(state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case DEVINFO_INT_TYPE:							info->i = IO_MEMCARD; break;
		case DEVINFO_INT_READABLE:						info->i = 1; break;
		case DEVINFO_INT_WRITEABLE:						info->i = 1; break;
		case DEVINFO_INT_CREATABLE:						info->i = 0; break;
		case DEVINFO_INT_COUNT:							info->i = 1; break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case DEVINFO_PTR_INIT:							info->init = device_init_smartmedia; break;
		case DEVINFO_PTR_LOAD:							info->load = device_load_smartmedia; break;
		case DEVINFO_PTR_UNLOAD:						info->unload = device_unload_smartmedia; break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case DEVINFO_STR_FILE_EXTENSIONS:				strcpy(info->s = device_temp_str(), ""); break;
	}
}

SYSTEM_CONFIG_START(geneve)
	CONFIG_DEVICE(geneve_floppy_getinfo)
	CONFIG_DEVICE(geneve_harddisk_getinfo)
	CONFIG_DEVICE(ti99_ide_harddisk_getinfo)
	CONFIG_DEVICE(geneve_parallel_getinfo)
	CONFIG_DEVICE(geneve_serial_getinfo)
	CONFIG_DEVICE(geneve_memcard_getinfo)
SYSTEM_CONFIG_END

/*	  YEAR	NAME	  PARENT	COMPAT	MACHINE		 INPUT	  INIT		CONFIG	COMPANY		FULLNAME */
COMP( 1987?,geneve,   0,		0,		geneve_60hz,  geneve,  geneve,	geneve,	"Myarc",	"Geneve 9640" , 0)
COMP( 199??,genmod,   geneve,	0,		geneve_60hz,  geneve,  genmod,	geneve,	"Myarc",	"Geneve 9640 (with Genmod modification)" , 0)
