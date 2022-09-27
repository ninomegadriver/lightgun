/*
	MESS Driver for TI-99/8 Computer.
	Raphael Nabet, 2003.
*/
/*
	TI-99/8 preliminary info:

Name: Texas Instruments Computer TI-99/8 (no "Home")

References:
	* machine room <http://...>
	* TI99/8 user manual
	* TI99/8 schematics
	* TI99/8 ROM source code
	* Message on TI99 yahoo group for CPU info

General:
	* a few dozen units were built in 1983, never released
	* CPU is a custom variant of tms9995 (part code MP9537): the 16-bit RAM and
	  (presumably) the on-chip decrementer are disabled
	* 220kb(?) of ROM, including monitor, GPL interpreter, TI-extended basic
	  II, and a P-code interpreter with a few utilities.  More specifically:
	  - 32kb system ROM with GPL interpreter, TI-extended basic II and a few
	    utilities (no dump, but 90% of source code is available and has been
	    compiled)
	  - 18kb system GROMs, with monitor and TI-extended basic II (no dump,
	    but source code is available and has been compiled)
	  - 4(???)kb DSR ROM for hexbus (no dump)
	  - 32(?)kb speech ROM: contents are slightly different from the 99/4(a)
	    speech ROMs, due to the use of a tms5220 speech synthesizer instead of
	    the older tms0285 (no dump, but 99/4(a) speech ROMs should work mostly
	    OK)
	  - 12(???)kb ROM with PCode interpreter (no dump)
	  - 2(3???)*48kb of GROMs with PCode data files (no dump)
	* 2kb SRAM (16 bytes of which are hidden), 64kb DRAM (expandable to almost
	  16MBytes), 16kb vdp RAM
	* tms9118 vdp (similar to tms9918a, slightly different bus interface and
	  timings)
	* I/O
	  - 50-key keyboard, plus 2 optional joysticks
	  - sound and speech (both ti99/4(a)-like)
	  - Hex-Bus
	  - Cassette
	* cartridge port on the top
	* 50-pin(?) expansion port on the back
	* Programs can enable/disable the ROM and memory mapped register areas.

Mapper:
	Mapper has 4kb page size (-> 16 pages per map file), 32 bits per page
	entry.  Address bits A0-A3 are the page index, whereas bits A4-A15 are the
	offset in the page.  Physical address space is 16Mbytes.  All pages are 4
	kBytes in lenght, and they can start anywhere in the 24-bit physical
	address space.  The mapper can load any of 4 map files from SRAM by DMA.
	Map file 0 is used by BIOS, file 1 by memory XOPs(?), file 2 by P-code
	interpreter(???).

	Format of map table entry:
	* bit 0: WTPROT: page is write protected if 1
	* bit 1: XPROT: page is execute protected if 1
	* bit 2: RDPROT: page is read protected if 1
	* bit 3: reserved, value is ignored
	* bits 4-7: reserved, always forced to 0
	* bits 8-23: page base address in 24-bit virtual address space

	Format of mapper control register:
	* bit 0-4: unused???
	* bit 5-6: map file to load/save (0 for file 0, 1 for file 1, etc.)
	* bit 7: 0 -> load map file from RAM, 1 -> save map file to RAM

	Format of mapper status register (cleared by read):
	* bit 0: WPE - Write-Protect Error
	* bit 1: XCE - eXeCute Error
	* bit 2: RPE - Read-Protect Error
	* bits 3-7: unused???

	Memory error interrupts are enabled by setting WTPROT/XPROT/RDPROT.  When
	an error occurs, the tms9901 INT1* pin is pulled low (active).  The pin
	remains low until the mapper status register is read.

24-bit address map:
	* >000000->00ffff: console RAM
	* >010000->feffff: expansion?
	* >ff0000->ff0fff: empty???
	* >ff1000->ff3fff: unused???
	* >ff4000->ff5fff: DSR space
	* >ff6000->ff7fff: cartridge space
	* >ff8000->ff9fff(???): >4000 ROM (normally enabled with a write to CRU >2700)
	* >ffa000->ffbfff(?): >2000 ROM
	* >ffc000->ffdfff(?): >6000 ROM


CRU map:
	Since the tms9995 supports full 15-bit CRU addresses, the >1000->17ff
	(>2000->2fff) range was assigned to support up to 16 extra expansion slot.
	The good thing with using >1000->17ff is the fact that older expansion
	cards that only decode 12 address bits will think that addresses
	>1000->17ff refer to internal TI99 peripherals (>000->7ff range), which
	suppresses any risk of bus contention.
	* >0000->001f (>0000->003e): tms9901
	  - P4: 1 -> MMD (Memory Mapped Devices?) at >8000, ROM enabled
	  - P5: 1 -> no P-CODE GROMs
	* >0800->17ff (>1000->2ffe): Peripheral CRU space
	* >1380->13ff (>2700->27fe): Internal DSR, with two output bits:
	  - >2700: Internal DSR select (parts of Basic and various utilities)
	  - >2702: SBO -> hardware reset


Memory map (TMS9901 P4 == 1):
	When TMS9901 P4 output is set, locations >8000->9fff are ignored by mapper.
	* >8000->83ff: SRAM (>8000->80ff is used by the mapper DMA controller
	  to hold four map files) (r/w)
	* >8400: sound port (w)
	* >8410->87ff: SRAM (r/w)
	* >8800: VDP data read port (r)
	* >8802: VDP status read port (r)
	* >8810: memory mapper status and control registers (r/w)
	* >8c00: VDP data write port (w)
	* >8c02: VDP address and register write port (w)
	* >9000: speech synthesizer read port (r)
	* >9400: speech synthesizer write port (w)
	* >9800 GPL data read port (r)
	* >9802 GPL address read port (r)
	* >9c00 GPL data write port -- unused (w)
	* >9c02 GPL address write port (w)


Memory map (TMS9901 P5 == 0):
	When TMS9901 P5 output is cleared, locations >f840->f8ff(?) are ignored by
	mapper.
	* >f840: data port for P-code grom library 0 (r?)
	* >f880: data port for P-code grom library 1 (r?)
	* >f8c0: data port for P-code grom library 2 (r?)
	* >f842: address port for P-code grom library 0 (r/w?)
	* >f882: address port for P-code grom library 1 (r/w?)
	* >f8c2: address port for P-code grom library 2 (r/w?)


Cassette interface:
	Identical to ti99/4(a), except that the CS2 unit is not implemented.


Keyboard interface:
	The keyboard interface uses the console tms9901 PSI, but the pin assignment
	and key matrix are different from both 99/4 and 99/4a.
	- P0-P3: column select
	- INT6*-INT11*: row inputs (int6* is only used for joystick fire)
*/

#include "driver.h"
#include "inputx.h"
#include "vidhrdw/tms9928a.h"

#include "machine/ti99_4x.h"
#include "machine/tms9901.h"
#include "sndhrdw/spchroms.h"
#include "machine/99_peb.h"
#include "machine/994x_ser.h"
#include "machine/99_dsk.h"
#include "machine/99_ide.h"
#include "cpu/tms9900/tms9900.h"
#include "devices/mflopimg.h"
#include "devices/cassette.h"
#include "machine/smartmed.h"
#include "sound/5220intf.h"
#include "devices/harddriv.h"

/*
	Memory map - see description above
*/

static ADDRESS_MAP_START(ti99_8_memmap, ADDRESS_SPACE_PROGRAM, 8)

	AM_RANGE(0x0000, 0xffff) AM_READWRITE(ti99_8_r, ti99_8_w)

ADDRESS_MAP_END


/*
	CRU map - see description above
*/

static ADDRESS_MAP_START(ti99_8_writecru, ADDRESS_SPACE_IO, 8)

	AM_RANGE(0x0000, 0x07ff) AM_WRITE(tms9901_0_cru_w)
	AM_RANGE(0x0800, 0x17ff) AM_WRITE(ti99_8_peb_cru_w)

ADDRESS_MAP_END

static ADDRESS_MAP_START(ti99_8_readcru, ADDRESS_SPACE_IO, 8)

	AM_RANGE(0x0000, 0x00ff) AM_READ(tms9901_0_cru_r)
	AM_RANGE(0x0100, 0x02ff) AM_READ(ti99_8_peb_cru_r)

ADDRESS_MAP_END


/* ti99/8 : 54-key keyboard */
INPUT_PORTS_START(ti99_8)

	/* 1 port for config */
	PORT_START	/* config */
		PORT_BIT( config_fdc_mask << config_fdc_bit, /*fdc_kind_hfdc << config_fdc_bit*/0, IPT_DIPSWITCH_NAME) PORT_NAME("Floppy disk controller")
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
		PORT_BIT( config_rs232_mask << config_rs232_bit, /*1 << config_rs232_bit*/0, IPT_DIPSWITCH_NAME) PORT_NAME("TI RS232 card")
			PORT_DIPSETTING( 0x0000, DEF_STR( Off ) )
			PORT_DIPSETTING( 1 << config_rs232_bit, DEF_STR( On ) )
		PORT_BIT( config_hsgpl_mask << config_hsgpl_bit, 0/*1 << config_hsgpl_bit*/, IPT_DIPSWITCH_NAME) PORT_NAME("SNUG HSGPL card")
			PORT_DIPSETTING( 0x0000, DEF_STR( Off ) )
			PORT_DIPSETTING( 1 << config_hsgpl_bit, DEF_STR( On ) )
		PORT_BIT( config_mecmouse_mask << config_mecmouse_bit, 0, IPT_DIPSWITCH_NAME) PORT_NAME("Mechatronics Mouse")
			PORT_DIPSETTING( 0x0000, DEF_STR( Off ) )
			PORT_DIPSETTING( 1 << config_mecmouse_bit, DEF_STR( On ) )
		PORT_BIT( config_usbsm_mask << config_usbsm_bit, 1 << config_usbsm_bit, IPT_DIPSWITCH_NAME) PORT_NAME("Nouspickel's USB-SM card")
			PORT_DIPSETTING( 0x0000, DEF_STR( Off ) )
			PORT_DIPSETTING( 1 << config_usbsm_bit, DEF_STR( On ) )


	/* 3 ports for mouse */
	PORT_START /* Mouse - X AXIS */
		PORT_BIT( 0xff, 0x00, IPT_TRACKBALL_X) PORT_SENSITIVITY(100) PORT_KEYDELTA(0) PORT_MINMAX(0,0) PORT_PLAYER(1)

	PORT_START /* Mouse - Y AXIS */
		PORT_BIT( 0xff, 0x00, IPT_TRACKBALL_Y) PORT_SENSITIVITY(100) PORT_KEYDELTA(0) PORT_MINMAX(0,0) PORT_PLAYER(1)

	PORT_START /* Mouse - buttons */
		PORT_BIT(0x0001, IP_ACTIVE_HIGH, IPT_BUTTON1) PORT_PLAYER(1)
		PORT_BIT(0x0002, IP_ACTIVE_HIGH, IPT_BUTTON2) PORT_PLAYER(1)


	/* 16 ports for keyboard and joystick */
	PORT_START    /* col 0 */
		PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("ALPHA LOCK") PORT_CODE(KEYCODE_CAPSLOCK)
		PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("FCTN") PORT_CODE(KEYCODE_LALT) PORT_CODE(KEYCODE_RALT)
		PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("CTRL") PORT_CODE(KEYCODE_LCONTROL) PORT_CODE(KEYCODE_RCONTROL)
		PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("LSHIFT") PORT_CODE(KEYCODE_LSHIFT) PORT_CHAR(UCHAR_SHIFT_1)
		PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_UNUSED)

	PORT_START    /* col 1 */
		PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("1 ! DEL") PORT_CODE(KEYCODE_1) PORT_CHAR('1') PORT_CHAR('!')
		PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("q Q") PORT_CODE(KEYCODE_Q) PORT_CHAR('q') PORT_CHAR('Q')
		PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("a A") PORT_CODE(KEYCODE_A) PORT_CHAR('a') PORT_CHAR('A')
		PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("z Z") PORT_CODE(KEYCODE_Z) PORT_CHAR('z') PORT_CHAR('Z')
		PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_UNUSED)

	PORT_START    /* col 2 */
		PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("2 @ INS") PORT_CODE(KEYCODE_2) PORT_CHAR('2') PORT_CHAR('@')
		PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("w W") PORT_CODE(KEYCODE_W) PORT_CHAR('w') PORT_CHAR('W')
		PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("s S (LEFT)") PORT_CODE(KEYCODE_S) PORT_CHAR('s') PORT_CHAR('S')
		PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("x X (DOWN)") PORT_CODE(KEYCODE_X) PORT_CHAR('x') PORT_CHAR('X')
		PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_UNUSED)

	PORT_START    /* col 3 */
		PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("3 #") PORT_CODE(KEYCODE_3) PORT_CHAR('3') PORT_CHAR('#')
		PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("e E (UP)") PORT_CODE(KEYCODE_E) PORT_CHAR('e') PORT_CHAR('E')
		PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("d D (RIGHT)") PORT_CODE(KEYCODE_D) PORT_CHAR('d') PORT_CHAR('D')
		PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("c C") PORT_CODE(KEYCODE_C) PORT_CHAR('c') PORT_CHAR('C')
		PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_UNUSED)

	PORT_START    /* col 4 */
		PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("4 $ CLEAR") PORT_CODE(KEYCODE_4) PORT_CHAR('4') PORT_CHAR('$')
		PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("r R") PORT_CODE(KEYCODE_R) PORT_CHAR('r') PORT_CHAR('R')
		PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("f F") PORT_CODE(KEYCODE_F) PORT_CHAR('f') PORT_CHAR('F')
		PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("v V") PORT_CODE(KEYCODE_V) PORT_CHAR('v') PORT_CHAR('V')
		PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_UNUSED)

	PORT_START    /* col 5 */
		PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("5 % BEGIN") PORT_CODE(KEYCODE_5) PORT_CHAR('5') PORT_CHAR('%')
		PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("t T") PORT_CODE(KEYCODE_T) PORT_CHAR('t') PORT_CHAR('T')
		PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("g G") PORT_CODE(KEYCODE_G) PORT_CHAR('g') PORT_CHAR('G')
		PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("b B") PORT_CODE(KEYCODE_B) PORT_CHAR('b') PORT_CHAR('B')
		PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_UNUSED)

	PORT_START    /* col 6 */
		PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("6 ^ PROC'D") PORT_CODE(KEYCODE_6) PORT_CHAR('6') PORT_CHAR('^')
		PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("y Y") PORT_CODE(KEYCODE_Y) PORT_CHAR('y') PORT_CHAR('Y')
		PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("h H") PORT_CODE(KEYCODE_H) PORT_CHAR('h') PORT_CHAR('H')
		PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("n N") PORT_CODE(KEYCODE_N) PORT_CHAR('n') PORT_CHAR('N')
		PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_UNUSED)

	PORT_START    /* col 7 */
		PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("7 & AID") PORT_CODE(KEYCODE_7) PORT_CHAR('7') PORT_CHAR('&')
		PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("u U") PORT_CODE(KEYCODE_U) PORT_CHAR('u') PORT_CHAR('U')
		PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("j J") PORT_CODE(KEYCODE_J) PORT_CHAR('j') PORT_CHAR('J')
		PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("m M") PORT_CODE(KEYCODE_M) PORT_CHAR('m') PORT_CHAR('M')
		PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_UNUSED)

	PORT_START    /* col 8 */
		PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("8 * REDO") PORT_CODE(KEYCODE_8) PORT_CHAR('8') PORT_CHAR('*')
		PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("i I") PORT_CODE(KEYCODE_I) PORT_CHAR('i') PORT_CHAR('I')
		PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("k K") PORT_CODE(KEYCODE_K) PORT_CHAR('k') PORT_CHAR('K')
		PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME(", <") PORT_CODE(KEYCODE_COMMA) PORT_CHAR(',') PORT_CHAR('<')
		PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_UNUSED)

	PORT_START    /* col 9 */
		PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("9 ( BACK") PORT_CODE(KEYCODE_9) PORT_CHAR('9') PORT_CHAR('(')
		PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("o O") PORT_CODE(KEYCODE_O) PORT_CHAR('o') PORT_CHAR('O')
		PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("l L") PORT_CODE(KEYCODE_L) PORT_CHAR('l') PORT_CHAR('L')
		PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME(". >") PORT_CODE(KEYCODE_STOP) PORT_CHAR('.') PORT_CHAR('>')
		PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_UNUSED)

	PORT_START    /* col 10 */
		PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("0 )") PORT_CODE(KEYCODE_0) PORT_CHAR('0') PORT_CHAR(')')
		PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("p P") PORT_CODE(KEYCODE_P) PORT_CHAR('p') PORT_CHAR('P')
		PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("; :") PORT_CODE(KEYCODE_COLON) PORT_CHAR(';') PORT_CHAR(':')
		PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("/ ?") PORT_CODE(KEYCODE_SLASH) PORT_CHAR('/') PORT_CHAR('?')
		PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_UNUSED)

	PORT_START    /* col 11 */
		PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("= + QUIT") PORT_CODE(KEYCODE_EQUALS) PORT_CHAR('=') PORT_CHAR('+')
		PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("[ {") PORT_CODE(KEYCODE_OPENBRACE) PORT_CHAR('[') PORT_CHAR('{')
		PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("' \"") PORT_CODE(KEYCODE_QUOTE) PORT_CHAR('\'') PORT_CHAR('"')
		PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("RSHIFT") PORT_CODE(KEYCODE_RSHIFT)
		PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_UNUSED)

	PORT_START    /* col 12 */
		PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("- _") PORT_CODE(KEYCODE_MINUS) PORT_CHAR('-') PORT_CHAR('_')
		PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("] }") PORT_CODE(KEYCODE_CLOSEBRACE) PORT_CHAR(']') PORT_CHAR('}')
		PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("ENTER") PORT_CODE(KEYCODE_ENTER) PORT_CHAR(13)
		PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("(SPACE)") PORT_CODE(KEYCODE_SPACE) PORT_CHAR(' ')
		PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_UNUSED)

	PORT_START    /* col 13 */
		PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("\\ |") PORT_CODE(KEYCODE_BACKSLASH) PORT_CHAR('\\') PORT_CHAR('|')
		PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("` ~") PORT_CODE(KEYCODE_BACKSLASH2) PORT_CHAR('`') PORT_CHAR('~')
		PORT_BIT(0x07, IP_ACTIVE_LOW, IPT_UNUSED)

	PORT_START    /* col 14: "wired handset 1" (= joystick 1) */
		PORT_BIT(0x0010, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN/*, "(1DOWN)", CODE_NONE, OSD_JOY_DOWN, 0*/) PORT_PLAYER(1)
		PORT_BIT(0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT/*, "(1RIGHT)", CODE_NONE, OSD_JOY_RIGHT, 0*/) PORT_PLAYER(1)
		PORT_BIT(0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT/*, "(1LEFT)", CODE_NONE, OSD_JOY_LEFT, 0*/) PORT_PLAYER(1)
		PORT_BIT(0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_UP/*, "(1UP)", CODE_NONE, OSD_JOY_UP*/) PORT_PLAYER(1)
		PORT_BIT(0x0001, IP_ACTIVE_LOW, IPT_BUTTON1/*, "(1FIRE)", CODE_NONE, OSD_JOY_FIRE, 0*/) PORT_PLAYER(1)

	PORT_START    /* col 15: "wired handset 2" (= joystick 2) */
		PORT_BIT(0x0010, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN/*, "(2UP)", CODE_NONE, OSD_JOY2_UP, 0*/) PORT_PLAYER(2)
		PORT_BIT(0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT/*, "(2DOWN)", CODE_NONE, OSD_JOY2_DOWN, 0*/) PORT_PLAYER(2)
		PORT_BIT(0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT/*, "(2RIGHT)", CODE_NONE, OSD_JOY2_RIGHT, 0*/) PORT_PLAYER(2)
		PORT_BIT(0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_UP/*, "(2LEFT)", CODE_NONE, OSD_JOY2_LEFT, 0*/) PORT_PLAYER(2)
		PORT_BIT(0x0001, IP_ACTIVE_LOW, IPT_BUTTON1/*, "(2FIRE)", CODE_NONE, OSD_JOY2_FIRE, 0*/) PORT_PLAYER(2)

INPUT_PORTS_END

/*
	TMS5220 speech synthesizer
*/
static struct TMS5220interface tms5220interface =
{
	NULL,						/* no IRQ callback */
	spchroms_read,				/* speech ROM read handler */
	spchroms_load_address,		/* speech ROM load address handler */
	spchroms_read_and_branch	/* speech ROM read and branch handler */
};



static const TMS9928a_interface tms9118_interface =
{
	TMS99x8A,
	0x4000,
	tms9901_set_int2
};

static const TMS9928a_interface tms9129_interface =
{
	TMS9929A,
	0x4000,
	tms9901_set_int2
};

static struct tms9995reset_param ti99_8_processor_config =
{
	1,				/* enable automatic wait state generation */
					/* (in January 83 99/8 schematics sheet 9: the delay logic
					seems to keep READY low for one cycle when RESET* is
					asserted, but the timings are completely wrong this way) */
	0,				/* no IDLE callback */
	1				/* MP9537 mask */
};

static MACHINE_DRIVER_START(ti99_8_60hz)

	/* basic machine hardware */
	/* TMS9995-MP9537 CPU @ 10.7 MHz */
	MDRV_CPU_ADD(TMS9995, 10738635)
	MDRV_CPU_CONFIG(ti99_8_processor_config)
	MDRV_CPU_PROGRAM_MAP(ti99_8_memmap, 0)
	MDRV_CPU_IO_MAP(ti99_8_readcru, ti99_8_writecru)
	MDRV_CPU_VBLANK_INT(ti99_vblank_interrupt, 1)
	/*MDRV_CPU_PERIODIC_INT(func, rate)*/

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_REAL_60HZ_VBLANK_DURATION)
	/*MDRV_INTERLEAVE(interleave)*/

	MDRV_MACHINE_RESET( ti99 )

	/* video hardware */
	MDRV_TMS9928A( &tms9118_interface )

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")
	MDRV_SOUND_ADD(WAVE, 0)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.20)
	MDRV_SOUND_ADD(SN76496, 3579545)	/* 3.579545 MHz */
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.75)
	MDRV_SOUND_ADD(TMS5220, 680000L)
	MDRV_SOUND_CONFIG(tms5220interface)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.50)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START(ti99_8_50hz)

	/* basic machine hardware */
	/* TMS9995-MP9537 CPU @ 10.7 MHz */
	MDRV_CPU_ADD(TMS9995, 10738635)
	MDRV_CPU_CONFIG(ti99_8_processor_config)
	MDRV_CPU_PROGRAM_MAP(ti99_8_memmap, 0)
	MDRV_CPU_IO_MAP(ti99_8_readcru, ti99_8_writecru)
	MDRV_CPU_VBLANK_INT(ti99_vblank_interrupt, 1)
	/*MDRV_CPU_PERIODIC_INT(func, rate)*/

	MDRV_FRAMES_PER_SECOND(50)
	MDRV_VBLANK_DURATION(DEFAULT_REAL_60HZ_VBLANK_DURATION)
	/*MDRV_INTERLEAVE(interleave)*/

	MDRV_MACHINE_RESET( ti99 )

	/* video hardware */
	MDRV_TMS9928A( &tms9129_interface )

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")
	MDRV_SOUND_ADD(SN76496, 3579545)	/* 3.579545 MHz */
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.75)
	MDRV_SOUND_ADD(TMS5220, 680000L)
	MDRV_SOUND_CONFIG(tms5220interface)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.50)
	MDRV_SOUND_ADD(WAVE, 0)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.20)

MACHINE_DRIVER_END

/*
  ROM loading
*/
ROM_START(ti99_8)
	/*CPU memory space*/
	ROM_REGION(region_cpu1_len_8,REGION_CPU1,0)
	ROM_LOAD("998rom.bin", 0x0000, 0x8000, NO_DUMP)		/* system ROMs */

	/*GROM memory space*/
	ROM_REGION(0x10000, region_grom, 0)
	ROM_LOAD("998grom.bin", 0x0000, 0x6000, NO_DUMP)	/* system GROMs */

	/*DSR ROM space*/
	ROM_REGION(region_dsr_len, region_dsr, 0)
	ROM_LOAD_OPTIONAL("disk.bin", offset_fdc_dsr, 0x2000, CRC(8f7df93f) SHA1(ed91d48c1eaa8ca37d5055bcf67127ea51c4cad5)) /* TI disk DSR ROM */
#if HAS_99CCFDC
	ROM_LOAD_OPTIONAL("ccfdc.bin", offset_ccfdc_dsr, 0x4000, BAD_DUMP CRC(f69cc69d)) /* CorComp disk DSR ROM */
#endif
	ROM_LOAD_OPTIONAL("bwg.bin", offset_bwg_dsr, 0x8000, CRC(06f1ec89) SHA1(6ad77033ed268f986d9a5439e65f7d391c4b7651)) /* BwG disk DSR ROM */
	ROM_LOAD_OPTIONAL("hfdc.bin", offset_hfdc_dsr, 0x4000, CRC(66fbe0ed) SHA1(11df2ecef51de6f543e4eaf8b2529d3e65d0bd59)) /* HFDC disk DSR ROM */
	ROM_LOAD_OPTIONAL("rs232.bin", offset_rs232_dsr, 0x1000, CRC(eab382fb) SHA1(ee609a18a21f1a3ddab334e8798d5f2a0fcefa91)) /* TI rs232 DSR ROM */

	/* HSGPL memory space */
	ROM_REGION(region_hsgpl_len, region_hsgpl, 0)

	/*TMS5220 ROM space*/
	ROM_REGION(0x8000, region_speech_rom, 0)
	ROM_LOAD_OPTIONAL("spchrom.bin", 0x0000, 0x8000, BAD_DUMP CRC(58b155f7)) /* system speech ROM */

ROM_END

#define rom_ti99_8e rom_ti99_8

static void ti99_8_cassette_getinfo(const device_class *devclass, UINT32 state, union devinfo *info)
{
	/* cassette */
	switch(state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case DEVINFO_INT_COUNT:							info->i = 1; break;

		default:										cassette_device_getinfo(devclass, state, info); break;
	}
}

static void ti99_8_cartslot_getinfo(const device_class *devclass, UINT32 state, union devinfo *info)
{
	/* cartslot */
	switch(state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case DEVINFO_INT_TYPE:							info->i = IO_CARTSLOT; break;
		case DEVINFO_INT_READABLE:						info->i = 1; break;
		case DEVINFO_INT_WRITEABLE:						info->i = 0; break;
		case DEVINFO_INT_CREATABLE:						info->i = 0; break;
		case DEVINFO_INT_COUNT:							info->i = 3; break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case DEVINFO_PTR_LOAD:							info->load = device_load_ti99_cart; break;
		case DEVINFO_PTR_UNLOAD:						info->unload = device_unload_ti99_cart; break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case DEVINFO_STR_FILE_EXTENSIONS:				strcpy(info->s = device_temp_str(), "bin,c,d,g,m,crom,drom,grom,mrom"); break;
	}
}

static void ti99_8_floppy_getinfo(const device_class *devclass, UINT32 state, union devinfo *info)
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

static void ti99_8_harddisk_getinfo(const device_class *devclass, UINT32 state, union devinfo *info)
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

static void ti99_8_parallel_getinfo(const device_class *devclass, UINT32 state, union devinfo *info)
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

static void ti99_8_serial_getinfo(const device_class *devclass, UINT32 state, union devinfo *info)
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

#if 0
static void ti99_8_quickload_getinfo(const device_class *devclass, UINT32 state, union devinfo *info)
{
	/* quickload */
	switch(state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case DEVINFO_INT_TYPE:							info->i = IO_QUICKLOAD; break;
		case DEVINFO_INT_READABLE:						info->i = 1; break;
		case DEVINFO_INT_WRITEABLE:						info->i = 1; break;
		case DEVINFO_INT_CREATABLE:						info->i = 1; break;
		case DEVINFO_INT_COUNT:							info->i = 1; break;
		case DEVINFO_INT_RESET_ON_LOAD:					info->i = 1; break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case DEVINFO_PTR_LOAD:							info->load = device_load_ti99_hsgpl; break;
		case DEVINFO_PTR_UNLOAD:						info->unload = device_unload_ti99_hsgpl; break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case DEVINFO_STR_FILE_EXTENSIONS:				strcpy(info->s = device_temp_str(), ""); break;
	}
}
#endif

static void ti99_8_memcard_getinfo(const device_class *devclass, UINT32 state, union devinfo *info)
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

SYSTEM_CONFIG_START(ti99_8)
	/* one cartridge port */
	/* one cassette unit port */
	/* Hex-bus disk controller: supports up to 4 floppy disk drives */
	/* expansion port (similar to 99/4(a) - yet slightly different) */

	CONFIG_DEVICE(ti99_8_cassette_getinfo)
	CONFIG_DEVICE(ti99_8_cartslot_getinfo)
#if 1
	CONFIG_DEVICE(ti99_8_floppy_getinfo)
	CONFIG_DEVICE(ti99_8_harddisk_getinfo)
	CONFIG_DEVICE(ti99_ide_harddisk_getinfo)
	CONFIG_DEVICE(ti99_8_parallel_getinfo)
	CONFIG_DEVICE(ti99_8_serial_getinfo)
	/*CONFIG_DEVICE(ti99_8_quickload_getinfo)*/
	CONFIG_DEVICE(ti99_8_memcard_getinfo)
#endif

SYSTEM_CONFIG_END

/*		YEAR	NAME		PARENT		COMPAT	MACHINE		INPUT	INIT		CONFIG		COMPANY					FULLNAME */
COMP(	1983,	ti99_8,		0,			0,		ti99_8_60hz,ti99_8,	ti99_8,		ti99_8,		"Texas Instruments",	"TI-99/8 Computer (US)" , 0)
COMP(	1983,	ti99_8e,	ti99_8,		0,		ti99_8_50hz,ti99_8,	ti99_8,		ti99_8,		"Texas Instruments",	"TI-99/8 Computer (Europe)" , 0)
