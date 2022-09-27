/******************************************************************************

  'Face' LINDA board
 driver by Paul Priest + David Haywood

*******************************************************************************

 Games on this Hardware

 Magical Cat Adventure (c)1993 Wintechno
 Nostradamus (c)1993 Face

*******************************************************************************

 Hardware Overview:

Magical Cat (C) 1993 Wintechno
Board Name: LINDA5

 Main CPU: 68000-16
 Sound CPU: Z80
 Sound Chip: YMF286-K

 Custom: FACE FX1037 x1
         038 x2 (As in Cave)


Nostradamus (C) 1993 FACE
Board Name: LINDA25

  Main CPU: MC68000P12F 16MHz
 Sound CPU: Z8400B PS (Goldstar)
Sound chip: YMF286-K & Y3016-F

Graphics chips:
176 Pin PQFP 038 9330EX705
176 Pin PQFP 038 9320EX702
176 Pin PQFP FX1037 FACE FA01-2075 (Face Custom)

OSC 28.000 MHz - SEOAN SX0-T100
OSC 16.000 MHz - Sunny SC0-010T

8 Way DIP Switch x 2
Push Button Test Switch

Roms:
NOS-PO-U 2740000PC-15 (68k Program) U29 - Odd
NOS-PE-U 2740000PC-15 (68k Program) U30 - Even
NOS-PS     D27C020-15 (Z80 program) U9

As labelled on PCB, with location:
NOS-SO-00.U83-
NOS-SO-01.U85 \
NOS-SO-02.U87  | Sprites Odd/Even (These are 27C8001)
NOS-SE-00.U82  |
NOS-SE-01.U84 /
NOS-SE-02.U86-
U92 & U93 are unpopulated

NOS-SN-00.U53 Sound samples (Near the YMF286-K)

NOS-B0-00.U58-
NOS-B0-01.U59 \ Background (Seperate for each 038 chip?)
NOS-B1-00.U60 /
NOS-B1-01.U61-

YMF286-K is compatible to YM2610 - see psikyo.c driver
038 9320EX702 / 038 9330EX705    - see Cave.c driver

Note # = Pin #1    PCB Layout:

+----------------------------------------------------------------------------+
| ___________                                                                |_
|| NOS-B1-00 |                                                                J|
|#___________|                ________   ________                             A|
| ___________   __________   |NOS-PO-U| |NOS-PE-U|                            M|
|| NOS-B1-01 | |          |  #________| #________|                            M|
|#___________| | 038      |   ___________________    _______                  A|
|              | 9330EX705|  |   MC68000P12F     |  |NOS-PS |                  |
|              |__________#  |   16MHz           |  #_______|                 C|
|                            #___________________|  ___________               o|
| ___________   __________                         | Z8400B PS |              n|
|| NOS-B0-00 | |          |                        #___________|              n|
|#___________| | 038      |                        ______________             e|
| ___________  | 9320EX702|                   SW1 |   YMF286-K   |            c|
|| NOS-B0-01 | |__________#     _________         #______________|            t|
|#___________|                 |FX1037   #  SW2                    _______    i|
|                              |(C) Face |         ___________    |Y3016-F#   o|
|                              |FA01-2075|        | NOS-SN-00 |   |_______|   n|
|                              |_________|        #___________|               _|
| ______                   ___  ___  ___       ___  ___  ___                 |
||OSC 28|                 # N |# N |# N |  E  # N |# N |# N | E              |
|#______|                 | O || O || O |  m  | O || O || O | m              |
|                         | S || S || S |  p  | S || S || S | p              |
| Empty                   | | || | || | |  t  | | || | || | | t              |
|  OSC                    | S || S || S |  y  | S || S || S | y              |
| ______                  | E || E || E |     | O || O || O |                |
||OSC 16|                 | | || | || | |  S  | | || | || | | S              |
|#______|                 | 0 || 0 || 0 |  C  | 0 || 0 || 0 | C              |
|                         | 0 || 1 || 2 |  K  | 0 || 1 || 2 | K              |
|    PUSHBTN              |___||___||___|  T  |___||___||___| T              |
+----------------------------------------------------------------------------+

*******************************************************************************

Stephh's notes (based on the games M68000 code and some tests) :

1) "mcatadv*'

  - Player 1 Button 3 is only used in the "test" mode :
      * to select "OBJECT ROM CHECK"
      * in "BG ROM", to change the background number

  - Do NOT trust the "NORMAL TESTMODE" for the system inputs !

  - The Japan version has extra GFX/anims and it's harder than the other set.

2) 'nost*'

  - When entering the "test mode", you need to press SERVICE1 to cycle through
    the different screens.

*******************************************************************************

 todo:

 Flip Screen

*******************************************************************************

 trivia:

 Magical Cat Adventure tests for 'MASICAL CAT ADVENTURE' in RAM on start-up
 and will write it there if not found, expecting a reset, great engrish ;-)

******************************************************************************/

#include "driver.h"
#include "sound/2610intf.h"

VIDEO_UPDATE( mcatadv );
VIDEO_START( mcatadv );
VIDEO_EOF( mcatadv );
VIDEO_UPDATE( nost );
VIDEO_START( nost );

WRITE16_HANDLER( mcatadv_videoram1_w );
WRITE16_HANDLER( mcatadv_videoram2_w );
UINT16* mcatadv_videoram1, *mcatadv_videoram2;
UINT16* mcatadv_scroll, *mcatadv_scroll2;
UINT16* mcatadv_vidregs;


/*** Main CPU ***/

static READ16_HANDLER( mcatadv_dsw_r )
{
	return readinputport(2+offset) << 8;
}

static WRITE16_HANDLER( mcat_soundlatch_w )
{
	soundlatch_w(0, data);
	cpunum_set_input_line(1, INPUT_LINE_NMI, PULSE_LINE);
}

#if 0 // mcat only.. install read handler?
static WRITE16_HANDLER( mcat_coin_w )
{
	if(ACCESSING_MSB16)
	{
		coin_counter_w(0, data & 0x1000);
		coin_counter_w(1, data & 0x2000);
		coin_lockout_w(0, ~data & 0x4000);
		coin_lockout_w(1, ~data & 0x8000);
	}
}
#endif

static READ16_HANDLER( mcat_wd_r )
{
	watchdog_reset_r(0);
	return 0xc00;
}


static ADDRESS_MAP_START( mcatadv_readmem, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x0fffff) AM_READ(MRA16_ROM)
	AM_RANGE(0x100000, 0x10ffff) AM_READ(MRA16_RAM)

	AM_RANGE(0x200000, 0x200005) AM_READ(MRA16_RAM)
	AM_RANGE(0x300000, 0x300005) AM_READ(MRA16_RAM)

//  AM_RANGE(0x180018, 0x18001f) AM_READ(MRA16_NOP) // ?

	AM_RANGE(0x400000, 0x401fff) AM_READ(MRA16_RAM) // Tilemap 0
	AM_RANGE(0x500000, 0x501fff) AM_READ(MRA16_RAM) // Tilemap 1

	AM_RANGE(0x600000, 0x601fff) AM_READ(MRA16_RAM)
	AM_RANGE(0x602000, 0x602fff) AM_READ(MRA16_RAM)

	AM_RANGE(0x700000, 0x707fff) AM_READ(MRA16_RAM) // Sprites
	AM_RANGE(0x708000, 0x70ffff) AM_READ(MRA16_RAM) // Tests more than is needed?

	AM_RANGE(0x800000, 0x800001) AM_READ(input_port_0_word_r)	// Inputs
	AM_RANGE(0x800002, 0x800003) AM_READ(input_port_1_word_r)	// Inputs
	AM_RANGE(0xa00000, 0xa00003) AM_READ(mcatadv_dsw_r)		// Dip Switches

	AM_RANGE(0xb00000, 0xb0000f) AM_READ(MRA16_RAM)

	AM_RANGE(0xb0001e, 0xb0001f) AM_READ(mcat_wd_r) // MCAT Only
	AM_RANGE(0xc00000, 0xc00001) AM_READ(soundlatch2_word_r)
ADDRESS_MAP_END

static ADDRESS_MAP_START( mcatadv_writemem, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x0fffff) AM_WRITE(MWA16_ROM)
	AM_RANGE(0x100000, 0x10ffff) AM_WRITE(MWA16_RAM)
	AM_RANGE(0x200000, 0x200005) AM_WRITE(MWA16_RAM) AM_BASE(&mcatadv_scroll)
	AM_RANGE(0x300000, 0x300005) AM_WRITE(MWA16_RAM) AM_BASE(&mcatadv_scroll2)

	AM_RANGE(0x400000, 0x401fff) AM_WRITE(mcatadv_videoram1_w) AM_BASE(&mcatadv_videoram1) // Tilemap 0
	AM_RANGE(0x500000, 0x501fff) AM_WRITE(mcatadv_videoram2_w) AM_BASE(&mcatadv_videoram2) // Tilemap 1

	AM_RANGE(0x600000, 0x601fff) AM_WRITE(paletteram16_xGGGGGRRRRRBBBBB_word_w) AM_BASE(&paletteram16)
	AM_RANGE(0x602000, 0x602fff) AM_WRITE(MWA16_RAM) // Bigger than needs to be?

	AM_RANGE(0x700000, 0x707fff) AM_WRITE(MWA16_RAM) AM_BASE(&spriteram16) AM_SIZE(&spriteram_size) // Sprites, two halves for double buffering
	AM_RANGE(0x708000, 0x70ffff) AM_WRITE(MWA16_RAM) // Tests more than is needed?

//  AM_RANGE(0x900000, 0x900001) AM_WRITE(mcat_coin_w) // Lockout / Counter MCAT Only
	AM_RANGE(0xb00000, 0xb0000f) AM_WRITE(MWA16_RAM) AM_BASE(&mcatadv_vidregs)
	AM_RANGE(0xb00018, 0xb00019) AM_WRITE(watchdog_reset16_w) // NOST Only
	AM_RANGE(0xc00000, 0xc00001) AM_WRITE(mcat_soundlatch_w)
ADDRESS_MAP_END

/*** Sound ***/

static WRITE8_HANDLER ( mcatadv_sound_bw_w )
{
	UINT8 *rom = memory_region(REGION_CPU2) + 0x10000;

	memory_set_bankptr(1,rom + data * 0x4000);
}


static ADDRESS_MAP_START( mcatadv_sound_readmem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x3fff) AM_READ(MRA8_ROM		)	// ROM
	AM_RANGE(0x4000, 0xbfff) AM_READ(MRA8_BANK1		)	// ROM
	AM_RANGE(0xc000, 0xdfff) AM_READ(MRA8_RAM		)	// RAM
	AM_RANGE(0xe000, 0xe000) AM_READ(YM2610_status_port_0_A_r		)
	AM_RANGE(0xe002, 0xe002) AM_READ(YM2610_status_port_0_B_r		)
ADDRESS_MAP_END

static ADDRESS_MAP_START( mcatadv_sound_writemem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x3fff) AM_WRITE(MWA8_ROM		)	// ROM
	AM_RANGE(0x4000, 0xbfff) AM_WRITE(MWA8_ROM		)	// ROM
	AM_RANGE(0xc000, 0xdfff) AM_WRITE(MWA8_RAM		)	// RAM
	AM_RANGE(0xe000, 0xe000) AM_WRITE(YM2610_control_port_0_A_w)
	AM_RANGE(0xe001, 0xe001) AM_WRITE(YM2610_data_port_0_A_w)
	AM_RANGE(0xe002, 0xe002) AM_WRITE(YM2610_control_port_0_B_w)
	AM_RANGE(0xe003, 0xe003) AM_WRITE(YM2610_data_port_0_B_w)
	AM_RANGE(0xf000, 0xf000) AM_WRITE(mcatadv_sound_bw_w)
ADDRESS_MAP_END

static ADDRESS_MAP_START( mcatadv_sound_readport, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_FLAGS( AMEF_ABITS(8) )
	AM_RANGE(0x80, 0x80) AM_READ(soundlatch_r)
ADDRESS_MAP_END

static ADDRESS_MAP_START( mcatadv_sound_writeport, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_FLAGS( AMEF_ABITS(8) )
	AM_RANGE(0x80, 0x80) AM_WRITE(soundlatch2_w)
ADDRESS_MAP_END


static ADDRESS_MAP_START( nost_sound_readmem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x7fff) AM_READ(MRA8_ROM		)	// ROM
	AM_RANGE(0x8000, 0xbfff) AM_READ(MRA8_BANK1		)	// ROM
	AM_RANGE(0xc000, 0xdfff) AM_READ(MRA8_RAM		)	// RAM
ADDRESS_MAP_END

static ADDRESS_MAP_START( nost_sound_writemem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x7fff) AM_WRITE(MWA8_ROM		)	// ROM
	AM_RANGE(0x8000, 0xbfff) AM_WRITE(MWA8_ROM		)	// ROM
	AM_RANGE(0xc000, 0xdfff) AM_WRITE(MWA8_RAM		)	// RAM
ADDRESS_MAP_END

static ADDRESS_MAP_START( nost_sound_readport, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_FLAGS( AMEF_ABITS(8) )
	AM_RANGE(0x04, 0x05) AM_READ(YM2610_status_port_0_A_r)
	AM_RANGE(0x06, 0x07) AM_READ(YM2610_status_port_0_B_r)
	AM_RANGE(0x80, 0x80) AM_READ(soundlatch_r)
ADDRESS_MAP_END

static ADDRESS_MAP_START( nost_sound_writeport, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_FLAGS( AMEF_ABITS(8) )
	AM_RANGE(0x00, 0x00) AM_WRITE(YM2610_control_port_0_A_w)
	AM_RANGE(0x01, 0x01) AM_WRITE(YM2610_data_port_0_A_w)
	AM_RANGE(0x02, 0x02) AM_WRITE(YM2610_control_port_0_B_w)
	AM_RANGE(0x03, 0x03) AM_WRITE(YM2610_data_port_0_B_w)
	AM_RANGE(0x40, 0x40) AM_WRITE(mcatadv_sound_bw_w)
	AM_RANGE(0x80, 0x80) AM_WRITE(soundlatch2_w)
ADDRESS_MAP_END

/*** Inputs ***/

INPUT_PORTS_START( mcatadv )
	PORT_START_TAG("IN0")
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_PLAYER(1)
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_PLAYER(1)
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_PLAYER(1)
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_PLAYER(1)
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(1)	// "Fire"
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(1)	// "Jump"
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_PLAYER(1)	// See notes
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0xfe00, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START_TAG("IN1")
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_PLAYER(2)
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_PLAYER(2)
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_PLAYER(2)
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_PLAYER(2)
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(2)	// "Fire"
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(2)	// "Jump"
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0xfc00, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START_TAG("DSW1")
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x01, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_SERVICE( 0x04, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0x08, 0x08, "Coin Mode" )
	PORT_DIPSETTING(    0x08, "Mode 1" )
	PORT_DIPSETTING(    0x00, "Mode 2" )
	PORT_DIPNAME( 0x30, 0x30, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 4C_1C ) )	PORT_CONDITION("DSW1",0x08,PORTCOND_EQUALS,0x00)
	PORT_DIPSETTING(    0x10, DEF_STR( 3C_1C ) )	PORT_CONDITION("DSW1",0x08,PORTCOND_EQUALS,0x00)
	PORT_DIPSETTING(    0x10, DEF_STR( 2C_1C ) )	PORT_CONDITION("DSW1",0x08,PORTCOND_EQUALS,0x08)
	PORT_DIPSETTING(    0x30, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 2C_3C ) )	PORT_CONDITION("DSW1",0x08,PORTCOND_EQUALS,0x08)
	PORT_DIPSETTING(    0x20, DEF_STR( 1C_2C ) )	PORT_CONDITION("DSW1",0x08,PORTCOND_EQUALS,0x08)
	PORT_DIPSETTING(    0x20, DEF_STR( 1C_4C ) )	PORT_CONDITION("DSW1",0x08,PORTCOND_EQUALS,0x00)
	PORT_DIPNAME( 0xc0, 0xc0, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 4C_1C ) )	PORT_CONDITION("DSW1",0x08,PORTCOND_EQUALS,0x00)
	PORT_DIPSETTING(    0x40, DEF_STR( 3C_1C ) )	PORT_CONDITION("DSW1",0x08,PORTCOND_EQUALS,0x00)
	PORT_DIPSETTING(    0x40, DEF_STR( 2C_1C ) )	PORT_CONDITION("DSW1",0x08,PORTCOND_EQUALS,0x08)
	PORT_DIPSETTING(    0xc0, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 2C_3C ) )	PORT_CONDITION("DSW1",0x08,PORTCOND_EQUALS,0x08)
	PORT_DIPSETTING(    0x80, DEF_STR( 1C_2C ) )	PORT_CONDITION("DSW1",0x08,PORTCOND_EQUALS,0x08)
	PORT_DIPSETTING(    0x80, DEF_STR( 1C_4C ) )	PORT_CONDITION("DSW1",0x08,PORTCOND_EQUALS,0x00)

	PORT_START_TAG("DSW2")
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Easy ) )
	PORT_DIPSETTING(    0x03, DEF_STR( Normal ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Hard ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Hardest ) )
	PORT_DIPNAME( 0x0c, 0x0c, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x04, "2" )
	PORT_DIPSETTING(    0x0c, "3" )
	PORT_DIPSETTING(    0x08, "4" )
	PORT_DIPSETTING(    0x00, "5" )
	PORT_DIPNAME( 0x30, 0x30, "Energy" )
	PORT_DIPSETTING(    0x30, "3" )
	PORT_DIPSETTING(    0x20, "4" )
	PORT_DIPSETTING(    0x10, "5" )
	PORT_DIPSETTING(    0x00, "8" )
	PORT_DIPNAME( 0xc0, 0xc0, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x40, "Upright 1 Player" )
	PORT_DIPSETTING(    0xc0, "Upright 2 Players" )
	PORT_DIPSETTING(    0x80, DEF_STR( Cocktail ) )
//  PORT_DIPSETTING(    0x00, "Upright 2 Players" )     // duplicated setting (NEVER tested)
INPUT_PORTS_END

INPUT_PORTS_START( nost )
	PORT_START_TAG("IN0")
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_PLAYER(1)
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_PLAYER(1)
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_PLAYER(1)
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_PLAYER(1)
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(1)
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_UNKNOWN )					// Button 2 in "test mode"
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_UNKNOWN )					// Button 3 in "test mode"
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_UNKNOWN )					// "test" 3 in "test mode"
	PORT_BIT( 0x0800, IP_ACTIVE_HIGH, IPT_UNKNOWN )					// Must be LOW or startup freezes !
	PORT_BIT( 0xf400, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START_TAG("IN1")
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_PLAYER(2)
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_PLAYER(2)
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_PLAYER(2)
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_PLAYER(2)
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(2)
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_UNKNOWN )					// Button 2 in "test mode"
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_UNKNOWN )					// Button 3 in "test mode"
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0xfc00, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START_TAG("DSW1")
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x02, "2" )
	PORT_DIPSETTING(    0x03, "3" )
	PORT_DIPSETTING(    0x01, "4" )
	PORT_DIPSETTING(    0x00, "5" )
	PORT_DIPNAME( 0x0c, 0x0c, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Easy ) )
	PORT_DIPSETTING(    0x0c, DEF_STR( Normal ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Hard ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Hardest ) )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x20, DEF_STR( On ) )
	PORT_DIPNAME( 0xc0, 0xc0, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x80, "500k 1000k" )
	PORT_DIPSETTING(    0xc0, "800k 1500k" )
	PORT_DIPSETTING(    0x40, "1000k 2000k" )
	PORT_DIPSETTING(    0x00, DEF_STR( None ) )

	PORT_START_TAG("DSW2")
	PORT_DIPNAME( 0x07, 0x07, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 3C_2C ) )
	PORT_DIPSETTING(    0x07, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(    0x06, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x05, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Free_Play ) )
	PORT_DIPNAME( 0x38, 0x38, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x20, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 3C_2C ) )
	PORT_DIPSETTING(    0x38, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x18, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(    0x30, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x28, DEF_STR( 1C_3C ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_SERVICE( 0x80, IP_ACTIVE_LOW )
INPUT_PORTS_END

/*** GFX Decode ***/

static const gfx_layout mcatadv_tiles16x16x4_layout =
{
	16,16,
	RGN_FRAC(1,1),
	4,
	{ STEP4(0,1) },
	{ STEP8(0,4), STEP8(32*8,4) },
	{ STEP8(0,32), STEP8(64*8,32) },
	128*8
};

static const gfx_decode mcatadv_gfxdecodeinfo[] =
{
	{ REGION_GFX2, 0, &mcatadv_tiles16x16x4_layout, 0, 0x200 },
	{ REGION_GFX3, 0, &mcatadv_tiles16x16x4_layout, 0, 0x200 },
	{ -1 }
};


/* Stolen from Psikyo.c */
static void sound_irq( int irq )
{
	cpunum_set_input_line(1,0,irq ? ASSERT_LINE : CLEAR_LINE);
}
struct YM2610interface mcatadv_ym2610_interface =
{
	sound_irq,	/* irq */
	REGION_SOUND1,	/* delta_t */
	REGION_SOUND1	/* adpcm */
};


static MACHINE_DRIVER_START( mcatadv )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000, 16000000)
	MDRV_CPU_PROGRAM_MAP(mcatadv_readmem,mcatadv_writemem)
	MDRV_CPU_VBLANK_INT(irq1_line_hold,1)

	MDRV_CPU_ADD_TAG("sound", Z80, 28000000/4) // Guess, 3.5MHz is too slow and CPU comms fail reporting U9 bad.
	MDRV_CPU_PROGRAM_MAP(mcatadv_sound_readmem,mcatadv_sound_writemem)
	MDRV_CPU_IO_MAP(mcatadv_sound_readport,mcatadv_sound_writeport)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(320, 256)
	MDRV_VISIBLE_AREA(0, 320-1, 0, 224-1)
	MDRV_GFXDECODE(mcatadv_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(0x2000/2)

	MDRV_WATCHDOG_VBLANK_INIT(DEFAULT_60HZ_3S_VBLANK_WATCHDOG)

	MDRV_VIDEO_START(mcatadv)
	MDRV_VIDEO_EOF(mcatadv) // Buffer Spriteram
	MDRV_VIDEO_UPDATE(mcatadv)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_STEREO("left", "right")

	MDRV_SOUND_ADD(YM2610, 16000000/2)
	MDRV_SOUND_CONFIG(mcatadv_ym2610_interface)
	MDRV_SOUND_ROUTE(0, "left",  0.32)
	MDRV_SOUND_ROUTE(0, "right", 0.32)
	MDRV_SOUND_ROUTE(1, "left",  1.0)
	MDRV_SOUND_ROUTE(2, "right", 1.0)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( nost )
	MDRV_IMPORT_FROM( mcatadv )

	MDRV_CPU_MODIFY("sound")
	MDRV_CPU_PROGRAM_MAP(nost_sound_readmem,nost_sound_writemem)
	MDRV_CPU_IO_MAP(nost_sound_readport,nost_sound_writeport)
MACHINE_DRIVER_END


static DRIVER_INIT( mcatadv )
{
	UINT8 *z80rom = memory_region(REGION_CPU2) + 0x10000;

	memory_set_bankptr(1, z80rom + 0x4000);
}


ROM_START( mcatadv )
	ROM_REGION( 0x100000, REGION_CPU1, 0 ) /* M68000 */
	ROM_LOAD16_BYTE( "mca-u30e", 0x00000, 0x80000, CRC(c62fbb65) SHA1(39a30a165d4811141db8687a4849626bef8e778e) )
	ROM_LOAD16_BYTE( "mca-u29e", 0x00001, 0x80000, CRC(cf21227c) SHA1(4012811ebfe3c709ab49946f8138bc4bad881ef7) )

	ROM_REGION( 0x030000, REGION_CPU2, 0 ) /* Z80-A */
	ROM_LOAD( "u9.bin", 0x00000, 0x20000, CRC(fda05171) SHA1(2c69292573ec35034572fa824c0cae2839d23919) )
	ROM_RELOAD( 0x10000, 0x20000 )

	ROM_REGION( 0x500000, REGION_GFX1, 0 ) /* Sprites */
	ROM_LOAD16_BYTE( "mca-u82.bin", 0x000000, 0x100000, CRC(5f01d746) SHA1(11b241456e15299912ee365eedb8f9d5e5ca875d) )
	ROM_LOAD16_BYTE( "mca-u83.bin", 0x000001, 0x100000, CRC(4e1be5a6) SHA1(cb19aad42dba54d6a4a33859f27254c2a3271e8c) )
	ROM_LOAD16_BYTE( "mca-u84.bin", 0x200000, 0x080000, CRC(df202790) SHA1(f6ae54e799af195860ed0ab3c85138cf2f10efa6) )
	ROM_LOAD16_BYTE( "mca-u85.bin", 0x200001, 0x080000, CRC(a85771d2) SHA1(a1817cd72f5bf0a4f24a37c782dc63ecec3b8e68) )
	ROM_LOAD16_BYTE( "mca-u86e",    0x400000, 0x080000, CRC(017bf1da) SHA1(f6446a7219275c0eff62129f59fdfa3a6a3e06c8) )
	ROM_LOAD16_BYTE( "mca-u87e",    0x400001, 0x080000, CRC(bc9dc9b9) SHA1(f525c9f994d5107752aa4d3a499ee376ec75f42b) )

	ROM_REGION( 0x080000, REGION_GFX2, 0 ) /* BG0 */
	ROM_LOAD( "mca-u58.bin", 0x000000, 0x080000, CRC(3a8186e2) SHA1(129c220d72608a8839f779ce1a6cfec8646dbf23) )

	ROM_REGION( 0x280000, REGION_GFX3, 0 ) /* BG1 */
	ROM_LOAD( "mca-u60.bin", 0x000000, 0x100000, CRC(c8942614) SHA1(244fccb9abbb04e33839dd2cd0e2de430819a18c) )
	ROM_LOAD( "mca-u61.bin", 0x100000, 0x100000, CRC(51af66c9) SHA1(1055cf78ea286f02003b0d1bf08c2d7829b36f90) )
	ROM_LOAD( "mca-u100",    0x200000, 0x080000, CRC(b273f1b0) SHA1(39318fe2aaf2792b85426ec6791b3360ac964de3) )

	ROM_REGION( 0x80000, REGION_SOUND1, 0 ) /* Samples */
	ROM_LOAD( "mca-u53.bin", 0x00000, 0x80000, CRC(64c76e05) SHA1(379cef5e0cba78d0e886c9cede41985850a3afb7) )
ROM_END

ROM_START( mcatadvj )
	ROM_REGION( 0x100000, REGION_CPU1, 0 ) /* M68000 */
	ROM_LOAD16_BYTE( "u30.bin", 0x00000, 0x80000, CRC(05762f42) SHA1(3675fb606bf9d7be9462324e68263f4a6c2fea1c) )
	ROM_LOAD16_BYTE( "u29.bin", 0x00001, 0x80000, CRC(4c59d648) SHA1(2ab77ea254f2c11fc016078cedcab2fffbe5ee1b) )

	ROM_REGION( 0x030000, REGION_CPU2, 0 ) /* Z80-A */
	ROM_LOAD( "u9.bin", 0x00000, 0x20000, CRC(fda05171) SHA1(2c69292573ec35034572fa824c0cae2839d23919) )
	ROM_RELOAD( 0x10000, 0x20000 )

	ROM_REGION( 0x500000, REGION_GFX1, 0 ) /* Sprites */
	ROM_LOAD16_BYTE( "mca-u82.bin", 0x000000, 0x100000, CRC(5f01d746) SHA1(11b241456e15299912ee365eedb8f9d5e5ca875d) )
	ROM_LOAD16_BYTE( "mca-u83.bin", 0x000001, 0x100000, CRC(4e1be5a6) SHA1(cb19aad42dba54d6a4a33859f27254c2a3271e8c) )
	ROM_LOAD16_BYTE( "mca-u84.bin", 0x200000, 0x080000, CRC(df202790) SHA1(f6ae54e799af195860ed0ab3c85138cf2f10efa6) )
	ROM_LOAD16_BYTE( "mca-u85.bin", 0x200001, 0x080000, CRC(a85771d2) SHA1(a1817cd72f5bf0a4f24a37c782dc63ecec3b8e68) )
	ROM_LOAD16_BYTE( "u86.bin",     0x400000, 0x080000, CRC(2d3725ed) SHA1(8b4c0f280eb901113d842848ffc26371be7b6067) )
	ROM_LOAD16_BYTE( "u87.bin",     0x400001, 0x080000, CRC(4ddefe08) SHA1(5ade0a694d73f4f3891c1ab7757e37a88afcbf54) )

	ROM_REGION( 0x080000, REGION_GFX2, 0 ) /* BG0 */
	ROM_LOAD( "mca-u58.bin", 0x000000, 0x080000, CRC(3a8186e2) SHA1(129c220d72608a8839f779ce1a6cfec8646dbf23) )

	ROM_REGION( 0x280000, REGION_GFX3, 0 ) /* BG1 */
	ROM_LOAD( "mca-u60.bin", 0x000000, 0x100000, CRC(c8942614) SHA1(244fccb9abbb04e33839dd2cd0e2de430819a18c) )
	ROM_LOAD( "mca-u61.bin", 0x100000, 0x100000, CRC(51af66c9) SHA1(1055cf78ea286f02003b0d1bf08c2d7829b36f90) )
	ROM_LOAD( "u100.bin",    0x200000, 0x080000, CRC(e2c311da) SHA1(cc3217484524de94704869eaa9ce1b90393039d8) )

	ROM_REGION( 0x80000, REGION_SOUND1, 0 ) /* Samples */
	ROM_LOAD( "mca-u53.bin", 0x00000, 0x80000, CRC(64c76e05) SHA1(379cef5e0cba78d0e886c9cede41985850a3afb7) )
ROM_END

ROM_START( catt )
	ROM_REGION( 0x100000, REGION_CPU1, 0 ) /* M68000 */
	ROM_LOAD16_BYTE( "catt-u30.bin",  0x00000, 0x80000, CRC(8c921e1e) SHA1(2fdaa9b743e1731f3cfe9d8334f1b759cf46855d) )
	ROM_LOAD16_BYTE( "catt-u29.bin",  0x00001, 0x80000, CRC(e725af6d) SHA1(78c08fa5744a6a953e13c0ff39736ccd4875fb72) )

	ROM_REGION( 0x030000, REGION_CPU2, 0 ) /* Z80-A */
	ROM_LOAD( "u9.bin", 0x00000, 0x20000, CRC(fda05171) SHA1(2c69292573ec35034572fa824c0cae2839d23919) )
	ROM_RELOAD( 0x10000, 0x20000 )

	ROM_REGION( 0x500000, REGION_GFX1, 0 ) /* Sprites */
	ROM_LOAD16_BYTE( "mca-u82.bin", 0x000000, 0x100000, CRC(5f01d746) SHA1(11b241456e15299912ee365eedb8f9d5e5ca875d) )
	ROM_LOAD16_BYTE( "mca-u83.bin", 0x000001, 0x100000, CRC(4e1be5a6) SHA1(cb19aad42dba54d6a4a33859f27254c2a3271e8c) )
	ROM_LOAD16_BYTE( "u84.bin",     0x200000, 0x100000, CRC(843fd624) SHA1(2e16d8a909fe9447da37a87428bff0734af59a00) )
	ROM_LOAD16_BYTE( "u85.bin",     0x200001, 0x100000, CRC(5ee7b628) SHA1(feedc212ed4893d784dc6b3361930b9199c6876d) )
	ROM_LOAD16_BYTE( "mca-u86e",    0x400000, 0x080000, CRC(017bf1da) SHA1(f6446a7219275c0eff62129f59fdfa3a6a3e06c8) )
	ROM_LOAD16_BYTE( "mca-u87e",    0x400001, 0x080000, CRC(bc9dc9b9) SHA1(f525c9f994d5107752aa4d3a499ee376ec75f42b) )

	ROM_REGION( 0x100000, REGION_GFX2, 0 ) /* BG0 */
	ROM_LOAD( "u58.bin",     0x00000, 0x100000, CRC(73c9343a) SHA1(9efdddbad6244c1ed267bd954563ab43a1017c96) )

	ROM_REGION( 0x280000, REGION_GFX3, 0 ) /* BG1 */
	ROM_LOAD( "mca-u60.bin", 0x000000, 0x100000, CRC(c8942614) SHA1(244fccb9abbb04e33839dd2cd0e2de430819a18c) )
	ROM_LOAD( "mca-u61.bin", 0x100000, 0x100000, CRC(51af66c9) SHA1(1055cf78ea286f02003b0d1bf08c2d7829b36f90) )
	ROM_LOAD( "mca-u100",    0x200000, 0x080000, CRC(b273f1b0) SHA1(39318fe2aaf2792b85426ec6791b3360ac964de3) )

	ROM_REGION( 0x100000, REGION_SOUND1, 0 ) /* Samples */
	ROM_LOAD( "u53.bin",     0x00000, 0x100000, CRC(99f2a624) SHA1(799e8e40e8bdcc8fa4cd763a366cc32473038a49) )
ROM_END

ROM_START( nost )
	ROM_REGION( 0x100000, REGION_CPU1, 0 ) /* M68000 */
	ROM_LOAD16_BYTE( "nos-pe-u.bin", 0x00000, 0x80000, CRC(4b080149) SHA1(e1dbbe5bf554c7c5731cc3079850f257417e3caa) )
	ROM_LOAD16_BYTE( "nos-po-u.bin", 0x00001, 0x80000, CRC(9e3cd6d9) SHA1(db5351ff9a05f602eceae62c0051c16ae0e4ead9) )

	ROM_REGION( 0x050000, REGION_CPU2, 0 ) /* Z80-A */
	ROM_LOAD( "nos-ps.u9", 0x00000, 0x40000, CRC(832551e9) SHA1(86fc481b1849f378c88593594129197c69ea1359) )
	ROM_RELOAD( 0x10000, 0x40000 )

	ROM_REGION( 0x500000, REGION_GFX1, 0 ) /* Sprites */
	ROM_LOAD16_BYTE( "nos-se-0.u82", 0x000000, 0x100000, CRC(9d99108d) SHA1(466540989d7b1b7f6dc7acbae74f6a8201973d45) )
	ROM_LOAD16_BYTE( "nos-so-0.u83", 0x000001, 0x100000, CRC(7df0fc7e) SHA1(2e064cb5367b2839d736d339c4f1a44785b4eedf) )
	ROM_LOAD16_BYTE( "nos-se-1.u84", 0x200000, 0x100000, CRC(aad07607) SHA1(89c51a9cb6b8d8ed3a357f5d8ac8399ff1c7ad46) )
	ROM_LOAD16_BYTE( "nos-so-1.u85", 0x200001, 0x100000, CRC(83d0012c) SHA1(831d36521693891f44e7adcc2ba63fef5d493821) )
	ROM_LOAD16_BYTE( "nos-se-2.u86", 0x400000, 0x080000, CRC(d99e6005) SHA1(49aae72111334ff5cd0fd86500882f559ff921f9) )
	ROM_LOAD16_BYTE( "nos-so-2.u87", 0x400001, 0x080000, CRC(f60e8ef3) SHA1(4f7472b5a465e6cc6a5df520ebfe6a544739dd28) )

	ROM_REGION( 0x180000, REGION_GFX2, 0 ) /* BG0 */
	ROM_LOAD( "nos-b0-0.u58", 0x000000, 0x100000, CRC(0214b0f2) SHA1(678fa3dc739323bda6d7bbb1c7a573c976d69356) )
	ROM_LOAD( "nos-b0-1.u59", 0x100000, 0x080000, CRC(3f8b6b34) SHA1(94c48614782ce6405965bcf6029e3bcc24a6d84f) )

	ROM_REGION( 0x180000, REGION_GFX3, 0 ) /* BG1 */
	ROM_LOAD( "nos-b1-0.u60", 0x000000, 0x100000, CRC(ba6fd0c7) SHA1(516d6e0c4dc6fb12ec9f30877ea1c582e7440a19) )
	ROM_LOAD( "nos-b1-1.u61", 0x100000, 0x080000, CRC(dabd8009) SHA1(1862645b8d6216c3ec2b8dbf74816b8e29dea14f) )

	ROM_REGION( 0x100000, REGION_SOUND1, 0 ) /* Samples */
	ROM_LOAD( "nossn-00.u53", 0x00000, 0x100000, CRC(3bd1bcbc) SHA1(1bcad43792e985402db4eca122676c2c555f3313) )
ROM_END

ROM_START( nostj )
	ROM_REGION( 0x100000, REGION_CPU1, 0 ) /* M68000 */
	ROM_LOAD16_BYTE( "nos-pe-j.u30", 0x00000, 0x80000, CRC(4b080149) SHA1(e1dbbe5bf554c7c5731cc3079850f257417e3caa) )
	ROM_LOAD16_BYTE( "nos-po-j.u29", 0x00001, 0x80000, CRC(7fe241de) SHA1(aa4ffd81cb73efc59690c2038ae9375021a775a4) )

	ROM_REGION( 0x050000, REGION_CPU2, 0 ) /* Z80-A */
	ROM_LOAD( "nos-ps.u9", 0x00000, 0x40000, CRC(832551e9) SHA1(86fc481b1849f378c88593594129197c69ea1359) )
	ROM_RELOAD( 0x10000, 0x40000 )

	ROM_REGION( 0x500000, REGION_GFX1, 0 ) /* Sprites */
	ROM_LOAD16_BYTE( "nos-se-0.u82", 0x000000, 0x100000, CRC(9d99108d) SHA1(466540989d7b1b7f6dc7acbae74f6a8201973d45) )
	ROM_LOAD16_BYTE( "nos-so-0.u83", 0x000001, 0x100000, CRC(7df0fc7e) SHA1(2e064cb5367b2839d736d339c4f1a44785b4eedf) )
	ROM_LOAD16_BYTE( "nos-se-1.u84", 0x200000, 0x100000, CRC(aad07607) SHA1(89c51a9cb6b8d8ed3a357f5d8ac8399ff1c7ad46) )
	ROM_LOAD16_BYTE( "nos-so-1.u85", 0x200001, 0x100000, CRC(83d0012c) SHA1(831d36521693891f44e7adcc2ba63fef5d493821) )
	ROM_LOAD16_BYTE( "nos-se-2.u86", 0x400000, 0x080000, CRC(d99e6005) SHA1(49aae72111334ff5cd0fd86500882f559ff921f9) )
	ROM_LOAD16_BYTE( "nos-so-2.u87", 0x400001, 0x080000, CRC(f60e8ef3) SHA1(4f7472b5a465e6cc6a5df520ebfe6a544739dd28) )

	ROM_REGION( 0x180000, REGION_GFX2, 0 ) /* BG0 */
	ROM_LOAD( "nos-b0-0.u58", 0x000000, 0x100000, CRC(0214b0f2) SHA1(678fa3dc739323bda6d7bbb1c7a573c976d69356) )
	ROM_LOAD( "nos-b0-1.u59", 0x100000, 0x080000, CRC(3f8b6b34) SHA1(94c48614782ce6405965bcf6029e3bcc24a6d84f) )

	ROM_REGION( 0x180000, REGION_GFX3, 0 ) /* BG1 */
	ROM_LOAD( "nos-b1-0.u60", 0x000000, 0x100000, CRC(ba6fd0c7) SHA1(516d6e0c4dc6fb12ec9f30877ea1c582e7440a19) )
	ROM_LOAD( "nos-b1-1.u61", 0x100000, 0x080000, CRC(dabd8009) SHA1(1862645b8d6216c3ec2b8dbf74816b8e29dea14f) )

	ROM_REGION( 0x100000, REGION_SOUND1, 0 ) /* Samples */
	ROM_LOAD( "nossn-00.u53", 0x00000, 0x100000, CRC(3bd1bcbc) SHA1(1bcad43792e985402db4eca122676c2c555f3313) )
ROM_END

ROM_START( nostk )
	ROM_REGION( 0x100000, REGION_CPU1, 0 ) /* M68000 */
	ROM_LOAD16_BYTE( "nos-pe-t.u30", 0x00000, 0x80000, CRC(bee5fbc8) SHA1(a8361fa004bb31471f973ece51a9a87b9f3438ab) )
	ROM_LOAD16_BYTE( "nos-po-t.u29", 0x00001, 0x80000, CRC(f4736331) SHA1(7a6db2db1a4dbf105c22e15deff6f6032e04609c) )

	ROM_REGION( 0x050000, REGION_CPU2, 0 ) /* Z80-A */
	ROM_LOAD( "nos-ps.u9", 0x00000, 0x40000, CRC(832551e9) SHA1(86fc481b1849f378c88593594129197c69ea1359) )
	ROM_RELOAD( 0x10000, 0x40000 )

	ROM_REGION( 0x500000, REGION_GFX1, 0 ) /* Sprites */
	ROM_LOAD16_BYTE( "nos-se-0.u82", 0x000000, 0x100000, CRC(9d99108d) SHA1(466540989d7b1b7f6dc7acbae74f6a8201973d45) )
	ROM_LOAD16_BYTE( "nos-so-0.u83", 0x000001, 0x100000, CRC(7df0fc7e) SHA1(2e064cb5367b2839d736d339c4f1a44785b4eedf) )
	ROM_LOAD16_BYTE( "nos-se-1.u84", 0x200000, 0x100000, CRC(aad07607) SHA1(89c51a9cb6b8d8ed3a357f5d8ac8399ff1c7ad46) )
	ROM_LOAD16_BYTE( "nos-so-1.u85", 0x200001, 0x100000, CRC(83d0012c) SHA1(831d36521693891f44e7adcc2ba63fef5d493821) )
	ROM_LOAD16_BYTE( "nos-se-2.u86", 0x400000, 0x080000, CRC(d99e6005) SHA1(49aae72111334ff5cd0fd86500882f559ff921f9) )
	ROM_LOAD16_BYTE( "nos-so-2.u87", 0x400001, 0x080000, CRC(f60e8ef3) SHA1(4f7472b5a465e6cc6a5df520ebfe6a544739dd28) )

	ROM_REGION( 0x180000, REGION_GFX2, 0 ) /* BG0 */
	ROM_LOAD( "nos-b0-0.u58", 0x000000, 0x100000, CRC(0214b0f2) SHA1(678fa3dc739323bda6d7bbb1c7a573c976d69356) )
	ROM_LOAD( "nos-b0-1.u59", 0x100000, 0x080000, CRC(3f8b6b34) SHA1(94c48614782ce6405965bcf6029e3bcc24a6d84f) )

	ROM_REGION( 0x180000, REGION_GFX3, 0 ) /* BG1 */
	ROM_LOAD( "nos-b1-0.u60", 0x000000, 0x100000, CRC(ba6fd0c7) SHA1(516d6e0c4dc6fb12ec9f30877ea1c582e7440a19) )
	ROM_LOAD( "nos-b1-1.u61", 0x100000, 0x080000, CRC(dabd8009) SHA1(1862645b8d6216c3ec2b8dbf74816b8e29dea14f) )

	ROM_REGION( 0x100000, REGION_SOUND1, 0 ) /* Samples */
	ROM_LOAD( "nossn-00.u53", 0x00000, 0x100000, CRC(3bd1bcbc) SHA1(1bcad43792e985402db4eca122676c2c555f3313) )
ROM_END

GAME( 1993, mcatadv,  0,       mcatadv, mcatadv, mcatadv, ROT0,   "Wintechno", "Magical Cat Adventure", GAME_NO_COCKTAIL )
GAME( 1993, mcatadvj, mcatadv, mcatadv, mcatadv, mcatadv, ROT0,   "Wintechno", "Magical Cat Adventure (Japan)", GAME_NO_COCKTAIL )
GAME( 1993, catt,     mcatadv, mcatadv, mcatadv, mcatadv, ROT0,   "Wintechno", "Catt (Japan)", GAME_NO_COCKTAIL )
GAME( 1993, nost,     0,       nost,    nost,    mcatadv, ROT270, "Face",      "Nostradamus", GAME_NO_COCKTAIL )
GAME( 1993, nostj,    nost,    nost,    nost,    mcatadv, ROT270, "Face",      "Nostradamus (Japan)", GAME_NO_COCKTAIL )
GAME( 1993, nostk,    nost,    nost,    nost,    mcatadv, ROT270, "Face",      "Nostradamus (Korea)", GAME_NO_COCKTAIL )
