/***************************************************************************

Vendetta (GX081) (c) 1991 Konami

Preliminary driver by:
Ernesto Corvi
someone@secureshell.com

Notes:
- collision detection is handled by a protection chip. Its emulation might
  not be 100% accurate.

// ********************************************************************************
//   Game driver for "ESCAPE KIDS (TM)"  (KONAMI, 1991)
// --------------------------------------------------------------------------------
//
//            This driver was made on the basis of 'src/drivers/vendetta.c' file.
//                                         Driver by OHSAKI Masayuki (2002/08/13)
//
// ********************************************************************************


// ***** NOTES *****
//      -------
//  1) ESCAPE KIDS uses 053246's unknown function. (see vidhrdw/konamiic.c)
//                   (053246 register #5  UnKnown Bit #5, #3, #2 always set "1")


// ***** On the "error.log" *****
//      --------------------
//  1) "YM2151 Write 00 to undocumented register #xx" (xx=00-1f)
//                Why???

//  2) "xxxx: read from unknown 052109 address yyyy"
//  3) "xxxx: write zz to unknown 052109 address yyyy"
//                These are vidhrdw/konamiic.c's message.
//                "vidhrdw/konamiic.c" checks 052109 RAM area access.
//                If accessed over 0x1800 (0x3800), logged 2) or 3) messages.
//                Escape Kids use 0x1800-0x19ff and 0x3800-0x39ff area.


// ***** UnEmulated *****
//      ------------
//  1) 0x3fc0-0x3fcf (052109 RAM area) access (053252 ???)
//  2) 0x7c00 (Banked ROM area) access to data WRITE (???)
//  3) 0x3fda (053248 RAM area) access to data WRITE (Watchdog ???)


// ***** ESCAPE KIDS PCB layout/ Need to dump *****
//      --------------------------------------
//   (Parts side view)
//   +-------------------------------------------------------+
//   |   R          ROM9                               [CN1] |  CN1:Player4 Input?
//   |   O                                             [CN2] |           (Labeled '4P')
//   |   M          ROM8                       ROM1    [SW1] |  CN2:Player3 Input?
//   |   7                              [CUS1]             +-+           (Labeled '3P')
//   |        [CUS7]   [CUS8]                              +-+  CN3:Stereo sound out
//   | R                                       [CUS2]        |
//   | O                                                   J |  SW1:Test Switch
//   | M                                                   A |
//   | 6    [CUS6]                                         M | ***  Custom Chips  ***
//   |                                                     M |      CUS1: 053248
//   | R                                                   A |      CUS2: 053252
//   | O    [CUS5]                                        56P|      CUS3: 053260
//   | M                                                     |      CUS4: 053246
//   | 5                           ROM2  [ Z80 ]           +-+      CUS5: 053247
//   |                                                     +-+      CUS6: 053251
//   | R    [CUS4]                     [CUS3] [YM2151] [CN3] |      CUS7: 051962
//   | O                                                     |      CUS8: 052109
//   | M                                 ROM3                |
//   | 4                                         [Sound AMP] |
//   +-------------------------------------------------------+
//
//  ***  Dump ROMs  ***
//     1) ROM1 (17C)  32Pin 1Mbit UV-EPROM          -> save "975r01" file
//     2) ROM2 ( 5F)  28Pin 512Kbit One-Time PROM   -> save "975f02" file
//     3) ROM3 ( 1D)  40Pin 4Mbit MASK ROM          -> save "975c03" file
//     4) ROM4 ( 3K)  42Pin 8Mbit MASK ROM          -> save "975c04" file
//     5) ROM5 ( 8L)  42Pin 8Mbit MASK ROM          -> save "975c05" file
//     6) ROM6 (12M)  42Pin 8Mbit MASK ROM          -> save "975c06" file
//     7) ROM7 (16K)  42Pin 8Mbit MASK ROM          -> save "975c07" file
//     8) ROM8 (16I)  40Pin 4Mbit MASK ROM          -> save "975c08" file
//     9) ROM9 (18I)  40Pin 4Mbit MASK ROM          -> save "975c09" file
//                                                        vvvvvvvvvvvv
//                                                        esckidsj.zip

***************************************************************************/

#include "driver.h"
#include "vidhrdw/konamiic.h"
#include "cpu/konami/konami.h" /* for the callback and the firq irq definition */
#include "machine/eeprom.h"
#include "sound/2151intf.h"
#include "sound/k053260.h"

/* prototypes */
static MACHINE_RESET( vendetta );
static void vendetta_banking( int lines );
static void vendetta_video_banking( int select );

VIDEO_START( vendetta );
VIDEO_START( esckids );
VIDEO_UPDATE( vendetta );


/***************************************************************************

  EEPROM

***************************************************************************/

static int init_eeprom_count;


static struct EEPROM_interface eeprom_interface =
{
	7,				/* address bits */
	8,				/* data bits */
	"011000",		/*  read command */
	"011100",		/* write command */
	0,				/* erase command */
	"0100000000000",/* lock command */
	"0100110000000" /* unlock command */
};

static NVRAM_HANDLER( vendetta )
{
	if (read_or_write)
		EEPROM_save(file);
	else
	{
		EEPROM_init(&eeprom_interface);

		if (file)
		{
			init_eeprom_count = 0;
			EEPROM_load(file);
		}
		else
			init_eeprom_count = 1000;
	}
}

static READ8_HANDLER( vendetta_eeprom_r )
{
	int res;

	res = EEPROM_read_bit();

	res |= 0x02;//konami_eeprom_ack() << 5; /* add the ack */

	res |= readinputport( 3 ) & 0x0c; /* test switch */

	if (init_eeprom_count)
	{
		init_eeprom_count--;
		res &= 0xfb;
	}
	return res;
}

static int irq_enabled;

static WRITE8_HANDLER( vendetta_eeprom_w )
{
	/* bit 0 - VOC0 - Video banking related */
	/* bit 1 - VOC1 - Video banking related */
	/* bit 2 - MSCHNG - Mono Sound select (Amp) */
	/* bit 3 - EEPCS - Eeprom CS */
	/* bit 4 - EEPCLK - Eeprom CLK */
	/* bit 5 - EEPDI - Eeprom data */
	/* bit 6 - IRQ enable */
	/* bit 7 - Unused */

	if ( data == 0xff ) /* this is a bug in the eeprom write code */
		return;

	/* EEPROM */
	EEPROM_write_bit(data & 0x20);
	EEPROM_set_clock_line((data & 0x10) ? ASSERT_LINE : CLEAR_LINE);
	EEPROM_set_cs_line((data & 0x08) ? CLEAR_LINE : ASSERT_LINE);

	irq_enabled = ( data >> 6 ) & 1;

	vendetta_video_banking( data & 1 );
}

/********************************************/

static READ8_HANDLER( vendetta_K052109_r ) { return K052109_r( offset + 0x2000 ); }
//static WRITE8_HANDLER( vendetta_K052109_w ) { K052109_w( offset + 0x2000, data ); }
static WRITE8_HANDLER( vendetta_K052109_w ) {
	// *************************************************************************************
	// *  Escape Kids uses 052109's mirrored Tilemap ROM bank selector, but only during    *
	// *  Tilemap MASK-ROM Test       (0x1d80<->0x3d80, 0x1e00<->0x3e00, 0x1f00<->0x3f00)  *
	// *************************************************************************************
	if ( ( offset == 0x1d80 ) || ( offset == 0x1e00 ) || ( offset == 0x1f00 ) )		K052109_w( offset, data );
	K052109_w( offset + 0x2000, data );
}

static offs_t video_banking_base;

static void vendetta_video_banking( int select )
{
	if ( select & 1 )
	{
		memory_install_read8_handler(0, ADDRESS_SPACE_PROGRAM, video_banking_base + 0x2000, video_banking_base + 0x2fff, 0, 0, paletteram_r );
		memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM, video_banking_base + 0x2000, video_banking_base + 0x2fff, 0, 0, paletteram_xBBBBBGGGGGRRRRR_be_w );
		memory_install_read8_handler(0, ADDRESS_SPACE_PROGRAM, video_banking_base + 0x0000, video_banking_base + 0x0fff, 0, 0, K053247_r );
		memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM, video_banking_base + 0x0000, video_banking_base + 0x0fff, 0, 0, K053247_w );
	}
	else
	{
		memory_install_read8_handler(0, ADDRESS_SPACE_PROGRAM, video_banking_base + 0x2000, video_banking_base + 0x2fff, 0, 0, vendetta_K052109_r );
		memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM, video_banking_base + 0x2000, video_banking_base + 0x2fff, 0, 0, vendetta_K052109_w );
		memory_install_read8_handler(0, ADDRESS_SPACE_PROGRAM, video_banking_base + 0x0000, video_banking_base + 0x0fff, 0, 0, K052109_r );
		memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM, video_banking_base + 0x0000, video_banking_base + 0x0fff, 0, 0, K052109_w );
	}
}

static WRITE8_HANDLER( vendetta_5fe0_w )
{
	/* bit 0,1 coin counters */
	coin_counter_w(0,data & 0x01);
	coin_counter_w(1,data & 0x02);

	/* bit 2 = BRAMBK ?? */

	/* bit 3 = enable char ROM reading through the video RAM */
	K052109_set_RMRD_line((data & 0x08) ? ASSERT_LINE : CLEAR_LINE);

	/* bit 4 = INIT ?? */

	/* bit 5 = enable sprite ROM reading */
	K053246_set_OBJCHA_line((data & 0x20) ? ASSERT_LINE : CLEAR_LINE);
}

static void z80_nmi_callback( int param )
{
	cpunum_set_input_line(1, INPUT_LINE_NMI, ASSERT_LINE );
}

static WRITE8_HANDLER( z80_arm_nmi_w )
{
	cpunum_set_input_line(1, INPUT_LINE_NMI, CLEAR_LINE );

	timer_set( TIME_IN_USEC( 25 ), 0, z80_nmi_callback );
}

static WRITE8_HANDLER( z80_irq_w )
{
	cpunum_set_input_line_and_vector( 1, 0, HOLD_LINE, 0xff );
}

READ8_HANDLER( vendetta_sound_interrupt_r )
{
	cpunum_set_input_line_and_vector( 1, 0, HOLD_LINE, 0xff );
	return 0x00;
}

READ8_HANDLER( vendetta_sound_r )
{
	return K053260_0_r(2 + offset);
}

/********************************************/

static ADDRESS_MAP_START( readmem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x1fff) AM_READ(MRA8_BANK1	)
	AM_RANGE(0x2000, 0x3fff) AM_READ(MRA8_RAM)
	AM_RANGE(0x5f80, 0x5f9f) AM_READ(K054000_r)
	AM_RANGE(0x5fc0, 0x5fc0) AM_READ(input_port_0_r)
	AM_RANGE(0x5fc1, 0x5fc1) AM_READ(input_port_1_r)
	AM_RANGE(0x5fc2, 0x5fc2) AM_READ(input_port_4_r)
	AM_RANGE(0x5fc3, 0x5fc3) AM_READ(input_port_5_r)
	AM_RANGE(0x5fd0, 0x5fd0) AM_READ(vendetta_eeprom_r) /* vblank, service */
	AM_RANGE(0x5fd1, 0x5fd1) AM_READ(input_port_2_r)
	AM_RANGE(0x5fe4, 0x5fe4) AM_READ(vendetta_sound_interrupt_r)
	AM_RANGE(0x5fe6, 0x5fe7) AM_READ(vendetta_sound_r)
	AM_RANGE(0x5fe8, 0x5fe9) AM_READ(K053246_r)
	AM_RANGE(0x5fea, 0x5fea) AM_READ(watchdog_reset_r)
	/* what is the desired effect of overlapping these memory regions anyway? */
	AM_RANGE(0x4000, 0x4fff) AM_READ(MRA8_BANK3)
	AM_RANGE(0x6000, 0x6fff) AM_READ(MRA8_BANK2)
	AM_RANGE(0x4000, 0x7fff) AM_READ(K052109_r)
	AM_RANGE(0x8000, 0xffff) AM_READ(MRA8_ROM)
ADDRESS_MAP_END

static ADDRESS_MAP_START( writemem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x1fff) AM_WRITE(MWA8_ROM)
	AM_RANGE(0x2000, 0x3fff) AM_WRITE(MWA8_RAM)
	AM_RANGE(0x5f80, 0x5f9f) AM_WRITE(K054000_w)
	AM_RANGE(0x5fa0, 0x5faf) AM_WRITE(K053251_w)
	AM_RANGE(0x5fb0, 0x5fb7) AM_WRITE(K053246_w)
	AM_RANGE(0x5fe0, 0x5fe0) AM_WRITE(vendetta_5fe0_w)
	AM_RANGE(0x5fe2, 0x5fe2) AM_WRITE(vendetta_eeprom_w)
	AM_RANGE(0x5fe4, 0x5fe4) AM_WRITE(z80_irq_w)
	AM_RANGE(0x5fe6, 0x5fe7) AM_WRITE(K053260_0_w)
	/* what is the desired effect of overlapping these memory regions anyway? */
	AM_RANGE(0x4000, 0x4fff) AM_WRITE(MWA8_BANK3)
	AM_RANGE(0x6000, 0x6fff) AM_WRITE(MWA8_BANK2)
	AM_RANGE(0x4000, 0x7fff) AM_WRITE(K052109_w)
	AM_RANGE(0x8000, 0xffff) AM_WRITE(MWA8_ROM)
ADDRESS_MAP_END

static ADDRESS_MAP_START( esckids_readmem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x1fff) AM_READ(MRA8_RAM)			// 053248 64K SRAM
	AM_RANGE(0x3f80, 0x3f80) AM_READ(input_port_0_r)		// Player 1 Control
	AM_RANGE(0x3f81, 0x3f81) AM_READ(input_port_1_r)		// Player 2 Control
	AM_RANGE(0x3f82, 0x3f82) AM_READ(input_port_4_r) 	// Player 3 Control ???  (But not used)
	AM_RANGE(0x3f83, 0x3f83) AM_READ(input_port_5_r)		// Player 4 Control ???  (But not used)
	AM_RANGE(0x3f92, 0x3f92) AM_READ(vendetta_eeprom_r)	// vblank, TEST SW on PCB
	AM_RANGE(0x3f93, 0x3f93) AM_READ(input_port_2_r)		// Start, Service
	AM_RANGE(0x3fd4, 0x3fd4) AM_READ(vendetta_sound_interrupt_r)		// Sound
	AM_RANGE(0x3fd6, 0x3fd7) AM_READ(vendetta_sound_r)				// Sound
	AM_RANGE(0x3fd8, 0x3fd9) AM_READ(K053246_r)			// 053246 (Sprite)
	/* what is the desired effect of overlapping these memory regions anyway? */
	AM_RANGE(0x2000, 0x2fff) AM_READ(MRA8_BANK3)			// 052109 (Tilemap) 0x0000-0x0fff
	AM_RANGE(0x4000, 0x4fff) AM_READ(MRA8_BANK2)			// 052109 (Tilemap) 0x2000-0x3fff, Tilemap MASK-ROM bank selector (MASK-ROM Test)
	AM_RANGE(0x2000, 0x5fff) AM_READ(K052109_r)			// 052109 (Tilemap)
	AM_RANGE(0x6000, 0x7fff) AM_READ(MRA8_BANK1)			// 053248 '975r01' 1M ROM (Banked)
	AM_RANGE(0x8000, 0xffff) AM_READ(MRA8_ROM)			// 053248 '975r01' 1M ROM (0x18000-0x1ffff)
ADDRESS_MAP_END

static ADDRESS_MAP_START( esckids_writemem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x1fff) AM_WRITE(MWA8_RAM)			// 053248 64K SRAM
	AM_RANGE(0x3fa0, 0x3fa7) AM_WRITE(K053246_w)			// 053246 (Sprite)
	AM_RANGE(0x3fb0, 0x3fbf) AM_WRITE(K053251_w)			// 053251 (Priority Encoder)
	AM_RANGE(0x3fc0, 0x3fcf) AM_WRITE(MWA8_NOP)			// Not Emulated (053252 ???)
	AM_RANGE(0x3fd0, 0x3fd0) AM_WRITE(vendetta_5fe0_w)	// Coin Counter, 052109 RMRD, 053246 OBJCHA
	AM_RANGE(0x3fd2, 0x3fd2) AM_WRITE(vendetta_eeprom_w)	// EEPROM, Video banking
	AM_RANGE(0x3fd4, 0x3fd4) AM_WRITE(z80_irq_w)			// Sound
	AM_RANGE(0x3fd6, 0x3fd7) AM_WRITE(K053260_0_w)		// Sound
	AM_RANGE(0x3fda, 0x3fda) AM_WRITE(MWA8_NOP)			// Not Emulated (Watchdog ???)
	/* what is the desired effect of overlapping these memory regions anyway? */
	AM_RANGE(0x2000, 0x2fff) AM_WRITE(MWA8_BANK3)			// 052109 (Tilemap) 0x0000-0x0fff
	AM_RANGE(0x4000, 0x4fff) AM_WRITE(MWA8_BANK2)			// 052109 (Tilemap) 0x2000-0x3fff, Tilemap MASK-ROM bank selector (MASK-ROM Test)
	AM_RANGE(0x2000, 0x5fff) AM_WRITE(K052109_w)			// 052109 (Tilemap)
	AM_RANGE(0x6000, 0x7fff) AM_WRITE(MWA8_ROM)			// 053248 '975r01' 1M ROM (Banked)
	AM_RANGE(0x8000, 0xffff) AM_WRITE(MWA8_ROM)			// 053248 '975r01' 1M ROM (0x18000-0x1ffff)
ADDRESS_MAP_END


static ADDRESS_MAP_START( readmem_sound, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0xefff) AM_READ(MRA8_ROM)
	AM_RANGE(0xf000, 0xf7ff) AM_READ(MRA8_RAM)
	AM_RANGE(0xf801, 0xf801) AM_READ(YM2151_status_port_0_r)
	AM_RANGE(0xfc00, 0xfc2f) AM_READ(K053260_0_r)
ADDRESS_MAP_END

static ADDRESS_MAP_START( writemem_sound, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0xefff) AM_WRITE(MWA8_ROM)
	AM_RANGE(0xf000, 0xf7ff) AM_WRITE(MWA8_RAM)
	AM_RANGE(0xf800, 0xf800) AM_WRITE(YM2151_register_port_0_w)
	AM_RANGE(0xf801, 0xf801) AM_WRITE(YM2151_data_port_0_w)
	AM_RANGE(0xfa00, 0xfa00) AM_WRITE(z80_arm_nmi_w)
	AM_RANGE(0xfc00, 0xfc2f) AM_WRITE(K053260_0_w)
ADDRESS_MAP_END


/***************************************************************************

    Input Ports

***************************************************************************/

INPUT_PORTS_START( vendet4p )
	PORT_START_TAG("IN0")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(1)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(1)
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN1 )

	PORT_START_TAG("IN1")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(2)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(2)
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN2 )

	PORT_START_TAG("IN2")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START_TAG("IN3")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* EEPROM data */
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* EEPROM ready */
	PORT_SERVICE_NO_TOGGLE(0x04, IP_ACTIVE_LOW)
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_VBLANK ) /* not really vblank, object related. Its timed, otherwise sprites flicker */
	PORT_BIT( 0xf0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START_TAG("IN4")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY PORT_PLAYER(3)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY PORT_PLAYER(3)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY PORT_PLAYER(3)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY PORT_PLAYER(3)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(3)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(3)
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN3 )

	PORT_START_TAG("IN5")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY PORT_PLAYER(4)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY PORT_PLAYER(4)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY PORT_PLAYER(4)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY PORT_PLAYER(4)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(4)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(4)
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN4 )
INPUT_PORTS_END

INPUT_PORTS_START( vendetta )
	PORT_START_TAG("IN0")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN1 )

	PORT_START_TAG("IN1")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(2)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(2)
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN2 )

	PORT_START_TAG("IN2")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START_TAG("IN3")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* EEPROM data */
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* EEPROM ready */
	PORT_SERVICE_NO_TOGGLE(0x04, IP_ACTIVE_LOW)
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_VBLANK ) /* not really vblank, object related. Its timed, otherwise sprites flicker */
	PORT_BIT( 0xf0, IP_ACTIVE_LOW, IPT_UNUSED )
INPUT_PORTS_END

INPUT_PORTS_START( esckids )
	PORT_START_TAG("IN0")		// Player 1 Control
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(1)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(1)
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN1 )

	PORT_START_TAG("IN1")		// Player 2 Control
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(2)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(2)
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN2 )

	PORT_START_TAG("IN2")		// Start, Service
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START_TAG("IN3")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* EEPROM data */
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* EEPROM ready */
	PORT_SERVICE_NO_TOGGLE(0x04, IP_ACTIVE_LOW)
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_VBLANK ) /* not really vblank, object related. Its timed, otherwise sprites flicker */
	PORT_BIT( 0xf0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START_TAG("IN4")		// Player 3 Control ???  (Not used)
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY PORT_PLAYER(3)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY PORT_PLAYER(3)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY PORT_PLAYER(3)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY PORT_PLAYER(3)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(3)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(3)
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN3 )

	PORT_START_TAG("IN5")		// Player 4 Control ???  (Not used)
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY PORT_PLAYER(4)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY PORT_PLAYER(4)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY PORT_PLAYER(4)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY PORT_PLAYER(4)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(4)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(4)
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN4 )
INPUT_PORTS_END

INPUT_PORTS_START( esckidsj )
	PORT_START_TAG("IN0")		// Player 1 Control
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(1)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(1)
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN1 )

	PORT_START_TAG("IN1")		// Player 2 Control
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(2)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(2)
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN2 )

	PORT_START_TAG("IN2")		// Start, Service
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START_TAG("IN3")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* EEPROM data */
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* EEPROM ready */
	PORT_SERVICE_NO_TOGGLE(0x04, IP_ACTIVE_LOW)
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_VBLANK ) /* not really vblank, object related. Its timed, otherwise sprites flicker */
	PORT_BIT( 0xf0, IP_ACTIVE_LOW, IPT_UNUSED )
INPUT_PORTS_END

/***************************************************************************

    Machine Driver

***************************************************************************/

static struct K053260_interface k053260_interface =
{
	REGION_SOUND1 /* memory region */
};

static INTERRUPT_GEN( vendetta_irq )
{
	if (irq_enabled)
		cpunum_set_input_line(0, KONAMI_IRQ_LINE, HOLD_LINE);
}

static MACHINE_DRIVER_START( vendetta )

	/* basic machine hardware */
	MDRV_CPU_ADD_TAG("main", KONAMI, 6000000)	/* this is strange, seems an overclock but */
//  MDRV_CPU_ADD_TAG("main", KONAMI, 3000000)   /* is needed to have correct music speed */
	MDRV_CPU_PROGRAM_MAP(readmem,writemem)
	MDRV_CPU_VBLANK_INT(vendetta_irq,1)

	MDRV_CPU_ADD(Z80, 3579545)	/* verified with PCB */
	/* audio CPU */
	MDRV_CPU_PROGRAM_MAP(readmem_sound,writemem_sound)
                            /* interrupts are triggered by the main CPU */
	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_REAL_60HZ_VBLANK_DURATION)

	MDRV_MACHINE_RESET(vendetta)
	MDRV_NVRAM_HANDLER(vendetta)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER | VIDEO_HAS_SHADOWS)
	MDRV_SCREEN_SIZE(64*8, 32*8)
	MDRV_VISIBLE_AREA(13*8, (64-13)*8-1, 2*8, 30*8-1 )
	MDRV_PALETTE_LENGTH(2048)

	MDRV_VIDEO_START(vendetta)
	MDRV_VIDEO_UPDATE(vendetta)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_STEREO("left", "right")

	MDRV_SOUND_ADD(YM2151, 3579545)	/* verified with PCB */
	MDRV_SOUND_ROUTE(0, "left", 1.0)
	MDRV_SOUND_ROUTE(1, "right", 1.0)

	MDRV_SOUND_ADD(K053260, 3579545)	/* verified with PCB */
	MDRV_SOUND_CONFIG(k053260_interface)
	MDRV_SOUND_ROUTE(0, "left", 0.75)
	MDRV_SOUND_ROUTE(1, "right", 0.75)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( esckids )

	/* basic machine hardware */
	MDRV_IMPORT_FROM(vendetta)
	MDRV_CPU_MODIFY("main")
	MDRV_CPU_PROGRAM_MAP(esckids_readmem,esckids_writemem)

//MDRV_VISIBLE_AREA(13*8, (64-13)*8-1, 2*8, 30*8-1 )    /* black areas on the edges */
	MDRV_VISIBLE_AREA(14*8, (64-14)*8-1, 2*8, 30*8-1 )

	MDRV_VIDEO_START(esckids)

MACHINE_DRIVER_END



/***************************************************************************

  Game ROMs

***************************************************************************/

ROM_START( vendetta )
	ROM_REGION( 0x49000, REGION_CPU1, 0 ) /* code + banked roms + banked ram */
	ROM_LOAD( "081t01", 0x10000, 0x38000, CRC(e76267f5) SHA1(efef6c2edb4c181374661f358dad09123741b63d) )
	ROM_CONTINUE(		0x08000, 0x08000 )

	ROM_REGION( 0x10000, REGION_CPU2, 0 ) /* 64k for the sound CPU */
	ROM_LOAD( "081b02", 0x000000, 0x10000, CRC(4c604d9b) SHA1(22d979f5dbde7912dd927bf5538fdbfc5b82905e) )

	ROM_REGION( 0x100000, REGION_GFX1, 0 ) /* graphics ( don't dispose as the program can read them ) */
	ROM_LOAD( "081a09", 0x000000, 0x080000, CRC(b4c777a9) SHA1(cc2b1dff4404ecd72b604e25d00fffdf7f0f8b52) ) /* characters */
	ROM_LOAD( "081a08", 0x080000, 0x080000, CRC(272ac8d9) SHA1(2da12fe4c13921bf0d4ebffec326f8d207ec4fad) ) /* characters */

	ROM_REGION( 0x400000, REGION_GFX2, 0 ) /* graphics ( don't dispose as the program can read them ) */
	ROM_LOAD( "081a04", 0x000000, 0x100000, CRC(464b9aa4) SHA1(28066ff0a07c3e56e7192918a882778c1b316b37) ) /* sprites */
	ROM_LOAD( "081a05", 0x100000, 0x100000, CRC(4e173759) SHA1(ce803f2aca7d7dedad00ab30e112443848747bd2) ) /* sprites */
	ROM_LOAD( "081a06", 0x200000, 0x100000, CRC(e9fe6d80) SHA1(2b7fc9d7fe43cd85dc8b975fe639c273cb0d9256) ) /* sprites */
	ROM_LOAD( "081a07", 0x300000, 0x100000, CRC(8a22b29a) SHA1(be539f21518e13038ab1d4cc2b2a901dd3e621f4) ) /* sprites */

	ROM_REGION( 0x100000, REGION_SOUND1, 0 ) /* 053260 samples */
	ROM_LOAD( "081a03", 0x000000, 0x100000, CRC(14b6baea) SHA1(fe15ee57f19f5acaad6c1642d51f390046a7468a) )
ROM_END

ROM_START( vendetao )
	ROM_REGION( 0x49000, REGION_CPU1, 0 ) /* code + banked roms + banked ram */
	ROM_LOAD( "081r01", 0x10000, 0x38000, CRC(84796281) SHA1(e4330c6eaa17adda5b4bd3eb824388c89fb07918) )
	ROM_CONTINUE(		0x08000, 0x08000 )

	ROM_REGION( 0x10000, REGION_CPU2, 0 ) /* 64k for the sound CPU */
	ROM_LOAD( "081b02", 0x000000, 0x10000, CRC(4c604d9b) SHA1(22d979f5dbde7912dd927bf5538fdbfc5b82905e) )

	ROM_REGION( 0x100000, REGION_GFX1, 0 ) /* graphics ( don't dispose as the program can read them ) */
	ROM_LOAD( "081a09", 0x000000, 0x080000, CRC(b4c777a9) SHA1(cc2b1dff4404ecd72b604e25d00fffdf7f0f8b52) ) /* characters */
	ROM_LOAD( "081a08", 0x080000, 0x080000, CRC(272ac8d9) SHA1(2da12fe4c13921bf0d4ebffec326f8d207ec4fad) ) /* characters */

	ROM_REGION( 0x400000, REGION_GFX2, 0 ) /* graphics ( don't dispose as the program can read them ) */
	ROM_LOAD( "081a04", 0x000000, 0x100000, CRC(464b9aa4) SHA1(28066ff0a07c3e56e7192918a882778c1b316b37) ) /* sprites */
	ROM_LOAD( "081a05", 0x100000, 0x100000, CRC(4e173759) SHA1(ce803f2aca7d7dedad00ab30e112443848747bd2) ) /* sprites */
	ROM_LOAD( "081a06", 0x200000, 0x100000, CRC(e9fe6d80) SHA1(2b7fc9d7fe43cd85dc8b975fe639c273cb0d9256) ) /* sprites */
	ROM_LOAD( "081a07", 0x300000, 0x100000, CRC(8a22b29a) SHA1(be539f21518e13038ab1d4cc2b2a901dd3e621f4) ) /* sprites */

	ROM_REGION( 0x100000, REGION_SOUND1, 0 ) /* 053260 samples */
	ROM_LOAD( "081a03", 0x000000, 0x100000, CRC(14b6baea) SHA1(fe15ee57f19f5acaad6c1642d51f390046a7468a) )
ROM_END

ROM_START( vendet2p )
	ROM_REGION( 0x49000, REGION_CPU1, 0 ) /* code + banked roms + banked ram */
	ROM_LOAD( "081w01", 0x10000, 0x38000, CRC(cee57132) SHA1(8b6413877e127511daa76278910c2ee3247d613a) )
	ROM_CONTINUE(		0x08000, 0x08000 )

	ROM_REGION( 0x10000, REGION_CPU2, 0 ) /* 64k for the sound CPU */
	ROM_LOAD( "081b02", 0x000000, 0x10000, CRC(4c604d9b) SHA1(22d979f5dbde7912dd927bf5538fdbfc5b82905e) )

	ROM_REGION( 0x100000, REGION_GFX1, 0 ) /* graphics ( don't dispose as the program can read them ) */
	ROM_LOAD( "081a09", 0x000000, 0x080000, CRC(b4c777a9) SHA1(cc2b1dff4404ecd72b604e25d00fffdf7f0f8b52) ) /* characters */
	ROM_LOAD( "081a08", 0x080000, 0x080000, CRC(272ac8d9) SHA1(2da12fe4c13921bf0d4ebffec326f8d207ec4fad) ) /* characters */

	ROM_REGION( 0x400000, REGION_GFX2, 0 ) /* graphics ( don't dispose as the program can read them ) */
	ROM_LOAD( "081a04", 0x000000, 0x100000, CRC(464b9aa4) SHA1(28066ff0a07c3e56e7192918a882778c1b316b37) ) /* sprites */
	ROM_LOAD( "081a05", 0x100000, 0x100000, CRC(4e173759) SHA1(ce803f2aca7d7dedad00ab30e112443848747bd2) ) /* sprites */
	ROM_LOAD( "081a06", 0x200000, 0x100000, CRC(e9fe6d80) SHA1(2b7fc9d7fe43cd85dc8b975fe639c273cb0d9256) ) /* sprites */
	ROM_LOAD( "081a07", 0x300000, 0x100000, CRC(8a22b29a) SHA1(be539f21518e13038ab1d4cc2b2a901dd3e621f4) ) /* sprites */

	ROM_REGION( 0x100000, REGION_SOUND1, 0 ) /* 053260 samples */
	ROM_LOAD( "081a03", 0x000000, 0x100000, CRC(14b6baea) SHA1(fe15ee57f19f5acaad6c1642d51f390046a7468a) )
ROM_END

ROM_START( vendetas )
	ROM_REGION( 0x49000, REGION_CPU1, 0 ) /* code + banked roms + banked ram */
	ROM_LOAD( "081u01", 0x10000, 0x38000, CRC(b4d9ade5) SHA1(fbd543738cb0b68c80ff05eed7849b608de03395) )
	ROM_CONTINUE(		0x08000, 0x08000 )

	ROM_REGION( 0x10000, REGION_CPU2, 0 ) /* 64k for the sound CPU */
	ROM_LOAD( "081b02", 0x000000, 0x10000, CRC(4c604d9b) SHA1(22d979f5dbde7912dd927bf5538fdbfc5b82905e) )

	ROM_REGION( 0x100000, REGION_GFX1, 0 ) /* graphics ( don't dispose as the program can read them ) */
	ROM_LOAD( "081a09", 0x000000, 0x080000, CRC(b4c777a9) SHA1(cc2b1dff4404ecd72b604e25d00fffdf7f0f8b52) ) /* characters */
	ROM_LOAD( "081a08", 0x080000, 0x080000, CRC(272ac8d9) SHA1(2da12fe4c13921bf0d4ebffec326f8d207ec4fad) ) /* characters */

	ROM_REGION( 0x400000, REGION_GFX2, 0 ) /* graphics ( don't dispose as the program can read them ) */
	ROM_LOAD( "081a04", 0x000000, 0x100000, CRC(464b9aa4) SHA1(28066ff0a07c3e56e7192918a882778c1b316b37) ) /* sprites */
	ROM_LOAD( "081a05", 0x100000, 0x100000, CRC(4e173759) SHA1(ce803f2aca7d7dedad00ab30e112443848747bd2) ) /* sprites */
	ROM_LOAD( "081a06", 0x200000, 0x100000, CRC(e9fe6d80) SHA1(2b7fc9d7fe43cd85dc8b975fe639c273cb0d9256) ) /* sprites */
	ROM_LOAD( "081a07", 0x300000, 0x100000, CRC(8a22b29a) SHA1(be539f21518e13038ab1d4cc2b2a901dd3e621f4) ) /* sprites */

	ROM_REGION( 0x100000, REGION_SOUND1, 0 ) /* 053260 samples */
	ROM_LOAD( "081a03", 0x000000, 0x100000, CRC(14b6baea) SHA1(fe15ee57f19f5acaad6c1642d51f390046a7468a) )
ROM_END

ROM_START( vendtaso )
	ROM_REGION( 0x49000, REGION_CPU1, 0 ) /* code + banked roms + banked ram */
	ROM_LOAD( "081d01", 0x10000, 0x38000, CRC(335da495) SHA1(ea74680eb898aeecf9f1eec95f151bcf66e6b6cb) )
	ROM_CONTINUE(		0x08000, 0x08000 )

	ROM_REGION( 0x10000, REGION_CPU2, 0 ) /* 64k for the sound CPU */
	ROM_LOAD( "081b02", 0x000000, 0x10000, CRC(4c604d9b) SHA1(22d979f5dbde7912dd927bf5538fdbfc5b82905e) )

	ROM_REGION( 0x100000, REGION_GFX1, 0 ) /* graphics ( don't dispose as the program can read them ) */
	ROM_LOAD( "081a09", 0x000000, 0x080000, CRC(b4c777a9) SHA1(cc2b1dff4404ecd72b604e25d00fffdf7f0f8b52) ) /* characters */
	ROM_LOAD( "081a08", 0x080000, 0x080000, CRC(272ac8d9) SHA1(2da12fe4c13921bf0d4ebffec326f8d207ec4fad) ) /* characters */

	ROM_REGION( 0x400000, REGION_GFX2, 0 ) /* graphics ( don't dispose as the program can read them ) */
	ROM_LOAD( "081a04", 0x000000, 0x100000, CRC(464b9aa4) SHA1(28066ff0a07c3e56e7192918a882778c1b316b37) ) /* sprites */
	ROM_LOAD( "081a05", 0x100000, 0x100000, CRC(4e173759) SHA1(ce803f2aca7d7dedad00ab30e112443848747bd2) ) /* sprites */
	ROM_LOAD( "081a06", 0x200000, 0x100000, CRC(e9fe6d80) SHA1(2b7fc9d7fe43cd85dc8b975fe639c273cb0d9256) ) /* sprites */
	ROM_LOAD( "081a07", 0x300000, 0x100000, CRC(8a22b29a) SHA1(be539f21518e13038ab1d4cc2b2a901dd3e621f4) ) /* sprites */

	ROM_REGION( 0x100000, REGION_SOUND1, 0 ) /* 053260 samples */
	ROM_LOAD( "081a03", 0x000000, 0x100000, CRC(14b6baea) SHA1(fe15ee57f19f5acaad6c1642d51f390046a7468a) )
ROM_END

ROM_START( vendettj )
	ROM_REGION( 0x49000, REGION_CPU1, 0 ) /* code + banked roms + banked ram */
	ROM_LOAD( "081p01", 0x10000, 0x38000, CRC(5fe30242) SHA1(2ea98e66637fa2ad60044b1a2b0dd158a82403a2) )
	ROM_CONTINUE(		0x08000, 0x08000 )

	ROM_REGION( 0x10000, REGION_CPU2, 0 ) /* 64k for the sound CPU */
	ROM_LOAD( "081b02", 0x000000, 0x10000, CRC(4c604d9b) SHA1(22d979f5dbde7912dd927bf5538fdbfc5b82905e) )

	ROM_REGION( 0x100000, REGION_GFX1, 0 ) /* graphics ( don't dispose as the program can read them ) */
	ROM_LOAD( "081a09", 0x000000, 0x080000, CRC(b4c777a9) SHA1(cc2b1dff4404ecd72b604e25d00fffdf7f0f8b52) ) /* characters */
	ROM_LOAD( "081a08", 0x080000, 0x080000, CRC(272ac8d9) SHA1(2da12fe4c13921bf0d4ebffec326f8d207ec4fad) ) /* characters */

	ROM_REGION( 0x400000, REGION_GFX2, 0 ) /* graphics ( don't dispose as the program can read them ) */
	ROM_LOAD( "081a04", 0x000000, 0x100000, CRC(464b9aa4) SHA1(28066ff0a07c3e56e7192918a882778c1b316b37) ) /* sprites */
	ROM_LOAD( "081a05", 0x100000, 0x100000, CRC(4e173759) SHA1(ce803f2aca7d7dedad00ab30e112443848747bd2) ) /* sprites */
	ROM_LOAD( "081a06", 0x200000, 0x100000, CRC(e9fe6d80) SHA1(2b7fc9d7fe43cd85dc8b975fe639c273cb0d9256) ) /* sprites */
	ROM_LOAD( "081a07", 0x300000, 0x100000, CRC(8a22b29a) SHA1(be539f21518e13038ab1d4cc2b2a901dd3e621f4) ) /* sprites */

	ROM_REGION( 0x100000, REGION_SOUND1, 0 ) /* 053260 samples */
	ROM_LOAD( "081a03", 0x000000, 0x100000, CRC(14b6baea) SHA1(fe15ee57f19f5acaad6c1642d51f390046a7468a) )
ROM_END

ROM_START( esckids )
	ROM_REGION( 0x049000, REGION_CPU1, 0 )		// Main CPU (053248) Code & Banked (1M x 1)
	ROM_LOAD( "17c.bin", 0x010000, 0x018000, CRC(9dfba99c) SHA1(dbcb89aad5a9addaf7200b2524be999877313a6e) )
	ROM_CONTINUE(		0x008000, 0x008000 )

	ROM_REGION( 0x010000, REGION_CPU2, 0 )		// Sound CPU (Z80) Code (512K x 1)
	ROM_LOAD( "975f02", 0x000000, 0x010000, CRC(994fb229) SHA1(bf194ae91240225b8edb647b1a62cd83abfa215e) )

	ROM_REGION( 0x100000, REGION_GFX1, 0 )		// Tilemap MASK-ROM (4M x 2)
	ROM_LOAD( "975c09", 0x000000, 0x080000, CRC(bc52210e) SHA1(301a3892d250495c2e849d67fea5f01fb0196bed) )
	ROM_LOAD( "975c08", 0x080000, 0x080000, CRC(fcff9256) SHA1(b60d29f4d04f074120d4bb7f2a71b9e9bf252d33) )

	ROM_REGION( 0x400000, REGION_GFX2, 0 )		// Sprite MASK-ROM (8M x 4)
	ROM_LOAD( "975c04", 0x000000, 0x100000, CRC(15688a6f) SHA1(a445237a11e5f98f0f9b2573a7ef0583366a137e) )
	ROM_LOAD( "975c05", 0x100000, 0x100000, CRC(1ff33bb7) SHA1(eb17da33ba2769ea02f91fece27de2e61705e75a) )
	ROM_LOAD( "975c06", 0x200000, 0x100000, CRC(36d410f9) SHA1(2b1fd93c11839480aa05a8bf27feef7591704f3d) )
	ROM_LOAD( "975c07", 0x300000, 0x100000, CRC(97ec541e) SHA1(d1aa186b17cfe6e505f5b305703319299fa54518) )

	ROM_REGION( 0x100000, REGION_SOUND1, 0 )	// Samples MASK-ROM (4M x 1)
	ROM_LOAD( "975c03", 0x000000, 0x080000, CRC(dc4a1707) SHA1(f252d08483fd664f8fc03bf8f174efd452b4cdc5) )
ROM_END


ROM_START( esckidsj )
	ROM_REGION( 0x049000, REGION_CPU1, 0 )		// Main CPU (053248) Code & Banked (1M x 1)
	ROM_LOAD( "975r01", 0x010000, 0x018000, CRC(7b5c5572) SHA1(b94b58c010539926d112c2dfd80bcbad76acc986) )
	ROM_CONTINUE(		0x008000, 0x008000 )

	ROM_REGION( 0x010000, REGION_CPU2, 0 )		// Sound CPU (Z80) Code (512K x 1)
	ROM_LOAD( "975f02", 0x000000, 0x010000, CRC(994fb229) SHA1(bf194ae91240225b8edb647b1a62cd83abfa215e) )

	ROM_REGION( 0x100000, REGION_GFX1, 0 )		// Tilemap MASK-ROM (4M x 2)
	ROM_LOAD( "975c09", 0x000000, 0x080000, CRC(bc52210e) SHA1(301a3892d250495c2e849d67fea5f01fb0196bed) )
	ROM_LOAD( "975c08", 0x080000, 0x080000, CRC(fcff9256) SHA1(b60d29f4d04f074120d4bb7f2a71b9e9bf252d33) )

	ROM_REGION( 0x400000, REGION_GFX2, 0 )		// Sprite MASK-ROM (8M x 4)
	ROM_LOAD( "975c04", 0x000000, 0x100000, CRC(15688a6f) SHA1(a445237a11e5f98f0f9b2573a7ef0583366a137e) )
	ROM_LOAD( "975c05", 0x100000, 0x100000, CRC(1ff33bb7) SHA1(eb17da33ba2769ea02f91fece27de2e61705e75a) )
	ROM_LOAD( "975c06", 0x200000, 0x100000, CRC(36d410f9) SHA1(2b1fd93c11839480aa05a8bf27feef7591704f3d) )
	ROM_LOAD( "975c07", 0x300000, 0x100000, CRC(97ec541e) SHA1(d1aa186b17cfe6e505f5b305703319299fa54518) )

	ROM_REGION( 0x100000, REGION_SOUND1, 0 )	// Samples MASK-ROM (4M x 1)
	ROM_LOAD( "975c03", 0x000000, 0x080000, CRC(dc4a1707) SHA1(f252d08483fd664f8fc03bf8f174efd452b4cdc5) )
ROM_END


/***************************************************************************

  Game driver(s)

***************************************************************************/

static void vendetta_banking( int lines )
{
	unsigned char *RAM = memory_region(REGION_CPU1);

	if ( lines >= 0x1c )
	{
		logerror("PC = %04x : Unknown bank selected %02x\n", activecpu_get_pc(), lines );
	}
	else
		memory_set_bankptr( 1, &RAM[ 0x10000 + ( lines * 0x2000 ) ] );
}

static MACHINE_RESET( vendetta )
{
	cpunum_set_info_fct(0, CPUINFO_PTR_KONAMI_SETLINES_CALLBACK, (genf *)vendetta_banking);

	paletteram = &memory_region(REGION_CPU1)[0x48000];
	irq_enabled = 0;

	/* init banks */
	memory_set_bankptr( 1, &memory_region(REGION_CPU1)[0x10000] );
	vendetta_video_banking( 0 );
}


static DRIVER_INIT( vendetta )
{
	video_banking_base = 0x4000;
	konami_rom_deinterleave_2(REGION_GFX1);
	konami_rom_deinterleave_4(REGION_GFX2);
}

static DRIVER_INIT( esckids )
{
	video_banking_base = 0x2000;
	konami_rom_deinterleave_2(REGION_GFX1);
	konami_rom_deinterleave_4(REGION_GFX2);
}



GAME( 1991, vendetta, 0,        vendetta, vendet4p, vendetta, ROT0, "Konami", "Vendetta (World 4 Players ver. T)", 0 )
GAME( 1991, vendetao, vendetta, vendetta, vendet4p, vendetta, ROT0, "Konami", "Vendetta (World 4 Players ver. R)", 0 )
GAME( 1991, vendet2p, vendetta, vendetta, vendetta, vendetta, ROT0, "Konami", "Vendetta (World 2 Players ver. W)", 0 )
GAME( 1991, vendetas, vendetta, vendetta, vendetta, vendetta, ROT0, "Konami", "Vendetta (Asia 2 Players ver. U)", 0 )
GAME( 1991, vendtaso, vendetta, vendetta, vendetta, vendetta, ROT0, "Konami", "Vendetta (Asia 2 Players ver. D)", 0 )
GAME( 1991, vendettj, vendetta, vendetta, vendetta, vendetta, ROT0, "Konami", "Crime Fighters 2 (Japan 2 Players ver. P)", 0 )
GAME( 1991, esckids,  0,        esckids,  esckids,  esckids,  ROT0, "Konami", "Escape Kids (Asia, 4 Players)", 0 )
GAME( 1991, esckidsj, esckids,  esckids,  esckidsj, esckids,  ROT0, "Konami", "Escape Kids (Japan, 2 Players)", 0 )
