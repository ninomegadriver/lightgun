/***************************************************************************


Taito X-system

driver by Richard Bush, Howie Cohen and Yochizo

25th Nov 2003
vidhrdw merged with vidhrdw/seta.c


Supported games:
----------------------------------------------------
 Name                    Company               Year
  Superman                Taito Corp.           1988
  Twin Hawk (World)       Taito Corp. Japan     1988
  Twin Hawk (US)          Taito America Corp.   1988
  Daisenpu (Japan)        Taito Corp.           1988
  Balloon Brothers        East Technology Corp. 1990
  Gigandes                East Technology Corp. 1989
  Last Striker            East Technology Corp. 1989

Please tell me the games worked on this board.


Memory map:
----------------------------------------------------
  0x000000 - 0x07ffff : ROM
  0x300000   ??
  0x400000   ??
  0x500000 - 0x50000f : Dipswitches a & b, 4 bits to each word
  0x600000   ?? 0, 10, 0x4001, 0x4006
  0x700000   ??
  0x800000 - 0x800003 : sound chip
  0x900000 - 0x900fff : c-chip shared RAM space
  0xb00000 - 0xb00fff : palette RAM, words in the format xRRRRRGGGGGBBBBB
  0xc00000   ??
  0xd00000 - 0xd007ff : video attribute RAM
      0000 - 03ff : sprite y coordinate
      0400 - 07ff : tile x & y scroll
  0xe00000 - 0xe00fff : object RAM bank 1
      0000 - 03ff : sprite number (bit mask 0x3fff)
                    sprite y flip (bit mask 0x4000)
                    sprite x flip (bit mask 0x8000)
      0400 - 07ff : sprite x coordinate (bit mask 0x1ff)
                    sprite color (bit mask 0xf800)
      0800 - 0bff : tile number (bit mask 0x3fff)
                    tile y flip (bit mask 0x4000)
                    tile x flip (bit mask 0x8000)
      0c00 - 0fff : tile color (bit mask 0xf800)
  0xe01000 - 0xe01fff : unused(?) portion of object RAM
  0xe02000 - 0xe02fff : object RAM bank 2
  0xe03000 - 0xe03fff : unused(?) portion of object RAM

Interrupt:
----------------------------------------------------
  IRQ level 6 : Superman
  IRQ level 2 : Daisenpu, Balloon Brothers, Gigandes

Screen resolution:
----------------------------------------------------
  384 x 240   : Superman, Balloon Brothers, Gigandes
  384 x 224   : Daisenpu

Sound chip:
----------------------------------------------------
  YM2610 x 1   : Superman, Balloon Brothers, Gigandes
  YM2151 x 1   : Daisenpu


Gigandes
--------

  - Gigandes has background tile glitches on the demo of cave
    stage (the last one before it does demo of level 1 again).
    It seems to be rapidly switching between two different
    background layers. This may be because the game has put
    different tiles in bank 0 and bank 1 (which alternate each
    frame). The other strangeness is that the background is not
    scrolling. So chances are the graphics chip emulation is
    flawed.

    When this cuts to hiscore screen there is flickering white
    tilemap garbage beneath. These leftover tiles have not been
    cleared from $e00800-bff (but they have been from $e02800).
    So alternate 2 frames display the bank with garbage tiles and
    the one without.

    Probably some control register should be keeping the graphics
    chip in the cleared bank, so the garbage is never visible.
    Separate bank control for sprite / tile layers ?
    Maybe this effect is also desired during the cave stage.


TODO
----

  - Fix known problems
  - Any other games that worked on this board?
  - What is correct date for Ballbros? Two different ones
    appear in this driver!


Dumpers Notes
-------------

Details of custom chip numbers on Superman would be welcome.

Daisenpu
--------

(c)1989 Toaplan/Taito
Taito X System

K1100443A MAIN PCB
J1100188A X SYSTEM
P0-051A

CPU  : 68000 (Toshiba TMP68000N-8)
Sound: Z80 (Sharp LH0080A)
       YM2151+YM3012
OSC  : 16.000MHz

B87-01 (Mask ROMs, read as 27C4200)
B87-02
B87-03
B87-04
(These 4 images could be wrong)

B87-05 (M5M27C101K) - Main PRG
B87-06 (M5M27C101K) - Main PRG
B87-07 (27C256) - Sound PRG

This board uses SETA's custom chips.
X1-001A, X1-002A, X1-004, X1-006, X1-007.
Arkanoid II uses X1-001, X1-002, X1-003, X1-004.

Control: 8-way Joystick + 2-buttons


Gigandes
--------

East Technology 1989

P0-057A

           68000-8    1  2  51832  3  4  51832

6264                        16MHz
5
10                                       8
11       TC0140SYT     X1-001A X1-002A   7
                                         9
YM2610                                   6
        Z80A
                     X1-004          X1-006
                                     X1-007


Balloon Brothers
----------------

East Technology 1992

    68000-8    10A   51832           5A 51832

 6264                  2063
 8D                    2063                 SWB  SWA
 EAST-10                    16MHz
 EAST-11    Taito TC0140SYT
 YM2610                                             3
          Z80A         SETA X1-001A SETA X1-002A

                 SETA X1-004                        2
                                                    1
                                                    0


Kyuukyoku no Striker
East Technology/Taito, 1989

This game runs on Seta hardware and also using one Taito custom chip.


PCB Layout
----------

PO-057A
|------------------------------- |
|  YM2610    IC.18D  6264        |
|YM3016                          |
|    Z80  TCO140SYT        68000 |
| X1-004  X1-001A          PE.9A |
|J         X1-002A  6264   62256 |
|A                  6264         |
|M           16MHz         PO.4A |
|M X1-007           DSW1   62256 |
|A X1-006           DSW2         |
|              M-8-3             |
|M-8-5  M-8-4  M-8-2             |
|              M-8-1             |
|              M-8-0             |
|--------------------------------|

Notes:
        All M-8-x ROMs are held on a plug-in sub-board.
        The sub-board has printed on it "East Technology" and has PCB Number PO-046A

         68000 clock: 8.000MHz
           Z80 clock: 4.000MHz
        YM2610 clock: 8.000MHz
               Vsync: 58Hz
               HSync: 15.22kHz


C-Chip notes
------------

Superman seems to be the only game of the four with a c-chip. Daisenpu
appears to use a simple input device with coin counter and lockout in
its place. The East Technology games on this hardware follow Daisenpu.

***************************************************************************/

#include "driver.h"
#include "sndhrdw/taitosnd.h"
#include "seta.h"
#include "sound/2610intf.h"
#include "sound/2151intf.h"

MACHINE_RESET( cchip1 );
READ16_HANDLER ( cchip1_word_r );
WRITE16_HANDLER( cchip1_word_w );



static READ16_HANDLER( superman_dsw_input_r )
{
	switch (offset)
	{
		case 0x00:
			return  readinputport (0) & 0x0f;
		case 0x01:
			return (readinputport (0) & 0xf0) >> 4;
		case 0x02:
			return  readinputport (1) & 0x0f;
		case 0x03:
			return (readinputport (1) & 0xf0) >> 4;
		default:
			logerror("taitox unknown dsw read offset: %04x\n", offset);
			return 0x00;
	}
}

static READ16_HANDLER( daisenpu_input_r )
{
	switch (offset)
	{
		case 0x00:
			return readinputport(2);
		case 0x01:
			return readinputport(3);
		case 0x02:
			return readinputport(4);

		default:
			logerror("taitox unknown input read offset: %04x\n", offset);
			return 0x00;
	}
}

static WRITE16_HANDLER( daisenpu_input_w )
{
	switch (offset)
	{
		case 0x04:	/* coin counters and lockout */
			coin_counter_w(0,data & 0x01);
			coin_counter_w(1,data & 0x02);
			coin_lockout_w(0,~data & 0x04);
			coin_lockout_w(1,~data & 0x08);
//logerror("taitox coin control %04x to offset %04x\n",data,offset);
			break;

		default:
			logerror("taitox unknown input write %04x to offset %04x\n",data,offset);
	}
}


static WRITE16_HANDLER( kyustrkr_input_w )
{
	switch (offset)
	{
		case 0x04:	/* coin counters and lockout */
			coin_counter_w(0,data & 0x01);
			coin_counter_w(1,data & 0x02);
			coin_lockout_w(0,data & 0x04);
			coin_lockout_w(1,data & 0x08);
//logerror("taitox coin control %04x to offset %04x\n",data,offset);
			break;

		default:
			logerror("taitox unknown input write %04x to offset %04x\n",data,offset);
	}
}


/**************************************************************************/

static INT32 banknum = -1;

static void reset_sound_region(void)
{
	memory_set_bankptr( 2, memory_region(REGION_CPU2) + (banknum * 0x4000) + 0x10000 );
}

static WRITE8_HANDLER( sound_bankswitch_w )
{
	banknum = (data - 1) & 3;
	reset_sound_region();
}


/**************************************************************************/

static ADDRESS_MAP_START( superman_readmem, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x07ffff) AM_READ(MRA16_ROM)
	AM_RANGE(0x500000, 0x500007) AM_READ(superman_dsw_input_r)
	AM_RANGE(0x800000, 0x800001) AM_READ(MRA16_NOP)
	AM_RANGE(0x800002, 0x800003) AM_READ(taitosound_comm16_lsb_r)
	AM_RANGE(0x900000, 0x900fff) AM_READ(cchip1_word_r)
	AM_RANGE(0xb00000, 0xb00fff) AM_READ(paletteram16_word_r)
	AM_RANGE(0xd00000, 0xd007ff) AM_READ(MRA16_RAM)	/* video attribute ram */
	AM_RANGE(0xe00000, 0xe03fff) AM_READ(MRA16_RAM)	/* object ram */
	AM_RANGE(0xf00000, 0xf03fff) AM_READ(MRA16_RAM)	/* Main RAM */
ADDRESS_MAP_END

static ADDRESS_MAP_START( superman_writemem, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x07ffff) AM_WRITE(MWA16_ROM)
	AM_RANGE(0x300000, 0x300001) AM_WRITE(MWA16_NOP)	/* written each frame at $3a9c, mostly 0x10 */
	AM_RANGE(0x400000, 0x400001) AM_WRITE(MWA16_NOP)	/* written each frame at $3aa2, mostly 0x10 */
	AM_RANGE(0x600000, 0x600001) AM_WRITE(MWA16_NOP)	/* written each frame at $3ab0, mostly 0x10 */
	AM_RANGE(0x800000, 0x800001) AM_WRITE(taitosound_port16_lsb_w)
	AM_RANGE(0x800002, 0x800003) AM_WRITE(taitosound_comm16_lsb_w)
	AM_RANGE(0x900000, 0x900fff) AM_WRITE(cchip1_word_w)
	AM_RANGE(0xb00000, 0xb00fff) AM_WRITE(paletteram16_xRRRRRGGGGGBBBBB_word_w) AM_BASE(&paletteram16)
	AM_RANGE(0xd00000, 0xd007ff) AM_WRITE(MWA16_RAM) AM_BASE(&spriteram16	)	// Sprites Y
	AM_RANGE(0xe00000, 0xe03fff) AM_WRITE(MWA16_RAM) AM_BASE(&spriteram16_2	)	// Sprites Code + X + Attr
	AM_RANGE(0xf00000, 0xf03fff) AM_WRITE(MWA16_RAM)			/* Main RAM */
ADDRESS_MAP_END

static ADDRESS_MAP_START( daisenpu_readmem, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x03ffff) AM_READ(MRA16_ROM)
	AM_RANGE(0x500000, 0x50000f) AM_READ(superman_dsw_input_r)
	AM_RANGE(0x800000, 0x800001) AM_READ(MRA16_NOP)
	AM_RANGE(0x800002, 0x800003) AM_READ(taitosound_comm16_lsb_r)
	AM_RANGE(0x900000, 0x90000f) AM_READ(daisenpu_input_r)
	AM_RANGE(0xb00000, 0xb00fff) AM_READ(paletteram16_word_r)
	AM_RANGE(0xd00000, 0xd00fff) AM_READ(MRA16_RAM)	/* video attribute ram */
	AM_RANGE(0xe00000, 0xe03fff) AM_READ(MRA16_RAM)	/* object ram */
	AM_RANGE(0xf00000, 0xf03fff) AM_READ(MRA16_RAM)	/* Main RAM */
ADDRESS_MAP_END

static ADDRESS_MAP_START( daisenpu_writemem, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x03ffff) AM_WRITE(MWA16_ROM)
//  AM_RANGE(0x400000, 0x400001) AM_WRITE(MWA16_NOP)    /* written each frame at $2ac, values change */
//  AM_RANGE(0x600000, 0x600001) AM_WRITE(MWA16_NOP)    /* written each frame at $2a2, values change */
	AM_RANGE(0x800000, 0x800001) AM_WRITE(taitosound_port16_lsb_w)
	AM_RANGE(0x800002, 0x800003) AM_WRITE(taitosound_comm16_lsb_w)
	AM_RANGE(0x900000, 0x90000f) AM_WRITE(daisenpu_input_w)
	AM_RANGE(0xb00000, 0xb00fff) AM_WRITE(paletteram16_xRRRRRGGGGGBBBBB_word_w) AM_BASE(&paletteram16)
	AM_RANGE(0xd00000, 0xd007ff) AM_WRITE(MWA16_RAM) AM_BASE(&spriteram16	)	// Sprites Y
	AM_RANGE(0xe00000, 0xe03fff) AM_WRITE(MWA16_RAM) AM_BASE(&spriteram16_2	)	// Sprites Code + X + Attr
	AM_RANGE(0xf00000, 0xf03fff) AM_WRITE(MWA16_RAM)			/* Main RAM */
ADDRESS_MAP_END

static ADDRESS_MAP_START( gigandes_readmem, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x07ffff) AM_READ(MRA16_ROM)
	AM_RANGE(0x500000, 0x500007) AM_READ(superman_dsw_input_r)
	AM_RANGE(0x800000, 0x800001) AM_READ(MRA16_NOP)
	AM_RANGE(0x800002, 0x800003) AM_READ(taitosound_comm16_lsb_r)
	AM_RANGE(0x900000, 0x90000f) AM_READ(daisenpu_input_r)
	AM_RANGE(0xb00000, 0xb00fff) AM_READ(paletteram16_word_r)
	AM_RANGE(0xd00000, 0xd007ff) AM_READ(MRA16_RAM)	/* video attribute ram */
	AM_RANGE(0xe00000, 0xe03fff) AM_READ(MRA16_RAM)	/* object ram */
	AM_RANGE(0xf00000, 0xf03fff) AM_READ(MRA16_RAM)	/* Main RAM */
ADDRESS_MAP_END

static ADDRESS_MAP_START( gigandes_writemem, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x07ffff) AM_WRITE(MWA16_ROM)
	AM_RANGE(0x400000, 0x400001) AM_WRITE(MWA16_NOP)	/* 0x1 written each frame at $d42, watchdog? */
	AM_RANGE(0x600000, 0x600001) AM_WRITE(MWA16_NOP)	/* 0x1 written each frame at $d3c, watchdog? */
	AM_RANGE(0x800000, 0x800001) AM_WRITE(taitosound_port16_lsb_w)
	AM_RANGE(0x800002, 0x800003) AM_WRITE(taitosound_comm16_lsb_w)
	AM_RANGE(0x900000, 0x90000f) AM_WRITE(daisenpu_input_w)
	AM_RANGE(0xb00000, 0xb00fff) AM_WRITE(paletteram16_xRRRRRGGGGGBBBBB_word_w) AM_BASE(&paletteram16)
	AM_RANGE(0xd00000, 0xd007ff) AM_WRITE(MWA16_RAM) AM_BASE(&spriteram16	)	// Sprites Y
	AM_RANGE(0xe00000, 0xe03fff) AM_WRITE(MWA16_RAM) AM_BASE(&spriteram16_2	)	// Sprites Code + X + Attr
	AM_RANGE(0xf00000, 0xf03fff) AM_WRITE(MWA16_RAM)			/* Main RAM */
ADDRESS_MAP_END

static ADDRESS_MAP_START( ballbros_readmem, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x03ffff) AM_READ(MRA16_ROM)
	AM_RANGE(0x500000, 0x50000f) AM_READ(superman_dsw_input_r)
	AM_RANGE(0x800000, 0x800001) AM_READ(MRA16_NOP)
	AM_RANGE(0x800002, 0x800003) AM_READ(taitosound_comm16_lsb_r)
	AM_RANGE(0x900000, 0x90000f) AM_READ(daisenpu_input_r)
	AM_RANGE(0xb00000, 0xb00fff) AM_READ(paletteram16_word_r)
	AM_RANGE(0xd00000, 0xd007ff) AM_READ(MRA16_RAM)	/* video attribute ram */
	AM_RANGE(0xe00000, 0xe03fff) AM_READ(MRA16_RAM)	/* object ram */
	AM_RANGE(0xf00000, 0xf03fff) AM_READ(MRA16_RAM)	/* Main RAM */
ADDRESS_MAP_END

static ADDRESS_MAP_START( ballbros_writemem, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x03ffff) AM_WRITE(MWA16_ROM)
	AM_RANGE(0x400000, 0x400001) AM_WRITE(MWA16_NOP)	/* 0x1 written each frame at $c56, watchdog? */
	AM_RANGE(0x600000, 0x600001) AM_WRITE(MWA16_NOP)	/* 0x1 written each frame at $c4e, watchdog? */
	AM_RANGE(0x800000, 0x800001) AM_WRITE(taitosound_port16_lsb_w)
	AM_RANGE(0x800002, 0x800003) AM_WRITE(taitosound_comm16_lsb_w)
	AM_RANGE(0x900000, 0x90000f) AM_WRITE(daisenpu_input_w)
	AM_RANGE(0xb00000, 0xb00fff) AM_WRITE(paletteram16_xRRRRRGGGGGBBBBB_word_w) AM_BASE(&paletteram16)
	AM_RANGE(0xd00000, 0xd007ff) AM_WRITE(MWA16_RAM) AM_BASE(&spriteram16	)	// Sprites Y
	AM_RANGE(0xe00000, 0xe03fff) AM_WRITE(MWA16_RAM) AM_BASE(&spriteram16_2	)	// Sprites Code + X + Attr
	AM_RANGE(0xf00000, 0xf03fff) AM_WRITE(MWA16_RAM)			/* Main RAM */
ADDRESS_MAP_END


/**************************************************************************/

static ADDRESS_MAP_START( sound_readmem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x3fff) AM_READ(MRA8_ROM)
	AM_RANGE(0x4000, 0x7fff) AM_READ(MRA8_BANK2)
	AM_RANGE(0xc000, 0xdfff) AM_READ(MRA8_RAM)
	AM_RANGE(0xe000, 0xe000) AM_READ(YM2610_status_port_0_A_r)
	AM_RANGE(0xe001, 0xe001) AM_READ(YM2610_read_port_0_r)
	AM_RANGE(0xe002, 0xe002) AM_READ(YM2610_status_port_0_B_r)
	AM_RANGE(0xe200, 0xe200) AM_READ(MRA8_NOP)
	AM_RANGE(0xe201, 0xe201) AM_READ(taitosound_slave_comm_r)
	AM_RANGE(0xea00, 0xea00) AM_READ(MRA8_NOP)
ADDRESS_MAP_END

static ADDRESS_MAP_START( sound_writemem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x7fff) AM_WRITE(MWA8_ROM)
	AM_RANGE(0xc000, 0xdfff) AM_WRITE(MWA8_RAM)
	AM_RANGE(0xe000, 0xe000) AM_WRITE(YM2610_control_port_0_A_w)
	AM_RANGE(0xe001, 0xe001) AM_WRITE(YM2610_data_port_0_A_w)
	AM_RANGE(0xe002, 0xe002) AM_WRITE(YM2610_control_port_0_B_w)
	AM_RANGE(0xe003, 0xe003) AM_WRITE(YM2610_data_port_0_B_w)
	AM_RANGE(0xe200, 0xe200) AM_WRITE(taitosound_slave_port_w)
	AM_RANGE(0xe201, 0xe201) AM_WRITE(taitosound_slave_comm_w)
	AM_RANGE(0xe400, 0xe403) AM_WRITE(MWA8_NOP) /* pan */
	AM_RANGE(0xee00, 0xee00) AM_WRITE(MWA8_NOP) /* ? */
	AM_RANGE(0xf000, 0xf000) AM_WRITE(MWA8_NOP) /* ? */
	AM_RANGE(0xf200, 0xf200) AM_WRITE(sound_bankswitch_w) /* bankswitch ? */
ADDRESS_MAP_END

static ADDRESS_MAP_START( daisenpu_sound_readmem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x3fff) AM_READ(MRA8_ROM)
	AM_RANGE(0x4000, 0x7fff) AM_READ(MRA8_BANK2)
	AM_RANGE(0xc000, 0xdfff) AM_READ(MRA8_RAM)
	AM_RANGE(0xe000, 0xe001) AM_READ(YM2151_status_port_0_r)
	AM_RANGE(0xe200, 0xe200) AM_READ(MRA8_NOP)
	AM_RANGE(0xe201, 0xe201) AM_READ(taitosound_slave_comm_r)
	AM_RANGE(0xea00, 0xea00) AM_READ(MRA8_NOP)
ADDRESS_MAP_END

static ADDRESS_MAP_START( daisenpu_sound_writemem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x7fff) AM_WRITE(MWA8_ROM)
	AM_RANGE(0xc000, 0xdfff) AM_WRITE(MWA8_RAM)
	AM_RANGE(0xe000, 0xe000) AM_WRITE(YM2151_register_port_0_w)
	AM_RANGE(0xe001, 0xe001) AM_WRITE(YM2151_data_port_0_w)
	AM_RANGE(0xe200, 0xe200) AM_WRITE(taitosound_slave_port_w)
	AM_RANGE(0xe201, 0xe201) AM_WRITE(taitosound_slave_comm_w)
	AM_RANGE(0xe400, 0xe403) AM_WRITE(MWA8_NOP) /* pan */
	AM_RANGE(0xee00, 0xee00) AM_WRITE(MWA8_NOP) /* ? */
	AM_RANGE(0xf000, 0xf000) AM_WRITE(MWA8_NOP)
	AM_RANGE(0xf200, 0xf200) AM_WRITE(sound_bankswitch_w)
ADDRESS_MAP_END

static ADDRESS_MAP_START( ballbros_sound_readmem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x3fff) AM_READ(MRA8_ROM)
	AM_RANGE(0x4000, 0x7fff) AM_READ(MRA8_BANK2)
	AM_RANGE(0xc000, 0xdfff) AM_READ(MRA8_RAM)
	AM_RANGE(0xe000, 0xe000) AM_READ(YM2610_status_port_0_A_r)
	AM_RANGE(0xe001, 0xe001) AM_READ(YM2610_read_port_0_r)
	AM_RANGE(0xe002, 0xe002) AM_READ(YM2610_status_port_0_B_r)
	AM_RANGE(0xe003, 0xe003) AM_READ(YM2610_read_port_0_r)
	AM_RANGE(0xe200, 0xe200) AM_READ(MRA8_NOP)
	AM_RANGE(0xe200, 0xe200) AM_READ(MRA8_NOP)
	AM_RANGE(0xe201, 0xe201) AM_READ(taitosound_slave_comm_r)
	AM_RANGE(0xea00, 0xea00) AM_READ(MRA8_NOP)
ADDRESS_MAP_END

static ADDRESS_MAP_START( ballbros_sound_writemem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x7fff) AM_WRITE(MWA8_ROM)
	AM_RANGE(0xc000, 0xdfff) AM_WRITE(MWA8_RAM)
	AM_RANGE(0xe000, 0xe000) AM_WRITE(YM2610_control_port_0_A_w)
	AM_RANGE(0xe001, 0xe001) AM_WRITE(YM2610_data_port_0_A_w)
	AM_RANGE(0xe002, 0xe002) AM_WRITE(YM2610_control_port_0_B_w)
	AM_RANGE(0xe003, 0xe003) AM_WRITE(YM2610_data_port_0_B_w)
	AM_RANGE(0xe200, 0xe200) AM_WRITE(taitosound_slave_port_w)
	AM_RANGE(0xe201, 0xe201) AM_WRITE(taitosound_slave_comm_w)
	AM_RANGE(0xe400, 0xe403) AM_WRITE(MWA8_NOP) /* pan */
	AM_RANGE(0xee00, 0xee00) AM_WRITE(MWA8_NOP) /* ? */
	AM_RANGE(0xf000, 0xf000) AM_WRITE(MWA8_NOP) /* ? */
	AM_RANGE(0xf200, 0xf200) AM_WRITE(sound_bankswitch_w) /* bankswitch ? */
ADDRESS_MAP_END


/**************************************************************************/

#define TAITO_COINAGE_JAPAN_8 \
	PORT_DIPNAME( 0x30, 0x30, DEF_STR( Coin_A ) ) \
	PORT_DIPSETTING(    0x10, DEF_STR( 2C_1C ) ) \
	PORT_DIPSETTING(    0x30, DEF_STR( 1C_1C ) ) \
	PORT_DIPSETTING(    0x00, DEF_STR( 2C_3C ) ) \
	PORT_DIPSETTING(    0x20, DEF_STR( 1C_2C ) ) \
	PORT_DIPNAME( 0xc0, 0xc0, DEF_STR( Coin_B ) ) \
	PORT_DIPSETTING(    0x40, DEF_STR( 2C_1C ) ) \
	PORT_DIPSETTING(    0xc0, DEF_STR( 1C_1C ) ) \
	PORT_DIPSETTING(    0x00, DEF_STR( 2C_3C ) ) \
	PORT_DIPSETTING(    0x80, DEF_STR( 1C_2C ) )

#define TAITO_COINAGE_WORLD_8 \
	PORT_DIPNAME( 0x30, 0x30, DEF_STR( Coin_A ) ) \
	PORT_DIPSETTING(    0x00, DEF_STR( 4C_1C ) ) \
	PORT_DIPSETTING(    0x10, DEF_STR( 3C_1C ) ) \
	PORT_DIPSETTING(    0x20, DEF_STR( 2C_1C ) ) \
	PORT_DIPSETTING(    0x30, DEF_STR( 1C_1C ) ) \
	PORT_DIPNAME( 0xc0, 0xc0, DEF_STR( Coin_B ) ) \
	PORT_DIPSETTING(    0xc0, DEF_STR( 1C_2C ) ) \
	PORT_DIPSETTING(    0x80, DEF_STR( 1C_3C ) ) \
	PORT_DIPSETTING(    0x40, DEF_STR( 1C_4C ) ) \
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_6C ) )

#define TAITO_COINAGE_US_8 \
	PORT_DIPNAME( 0x30, 0x30, DEF_STR( Coinage ) ) \
	PORT_DIPSETTING(    0x00, DEF_STR( 4C_1C ) ) \
	PORT_DIPSETTING(    0x10, DEF_STR( 3C_1C ) ) \
	PORT_DIPSETTING(    0x20, DEF_STR( 2C_1C ) ) \
	PORT_DIPSETTING(    0x30, DEF_STR( 1C_1C ) ) \
	PORT_DIPNAME( 0xc0, 0xc0, "Price to Continue" ) \
	PORT_DIPSETTING(    0x00, DEF_STR( 3C_1C ) ) \
	PORT_DIPSETTING(    0x40, DEF_STR( 2C_1C ) ) \
	PORT_DIPSETTING(    0x80, DEF_STR( 1C_1C ) ) \
	PORT_DIPSETTING(    0xc0, "Same as Start" )

INPUT_PORTS_START( taitox )
	PORT_START_TAG("DSWA")
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_SERVICE( 0x04, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0x08, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	// Coinage depends on territory, so leave it blank here
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

	PORT_START_TAG("DSWB")
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Easy ) )
	PORT_DIPSETTING(    0x03, DEF_STR( Normal ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Hard ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Hardest ) )
	// Bonus_Life and Lives dips vary between games and
	// even territories (twinhawk), so leave them blank here
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
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

	PORT_START_TAG("IN0")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_PLAYER(1) PORT_8WAY
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_PLAYER(1) PORT_8WAY
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_PLAYER(1) PORT_8WAY
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_PLAYER(1) PORT_8WAY
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(1)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(1)
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_PLAYER(1)
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START1 )

	PORT_START_TAG("IN1")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_PLAYER(2) PORT_8WAY
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_PLAYER(2) PORT_8WAY
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_PLAYER(2) PORT_8WAY
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_PLAYER(2) PORT_8WAY
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(2)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(2)
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_PLAYER(2)
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START2 )

	PORT_START	/* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_TILT )
	PORT_BIT( 0xF0, IP_ACTIVE_LOW, IPT_UNKNOWN )
INPUT_PORTS_END

INPUT_PORTS_START( superman )
	PORT_INCLUDE( taitox )
	PORT_MODIFY("DSWA")
	TAITO_COINAGE_WORLD_8

	PORT_MODIFY("DSWB")
	// It seems that there are no Bonus_Life dips
	// You get always bonus at 50k, 150k, 300k,
	// 450k and 600k, no matter the values of bit 2 and 3
	PORT_DIPNAME( 0x30, 0x30, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x20, "2" )
	PORT_DIPSETTING(    0x30, "3" )
	PORT_DIPSETTING(    0x10, "4" )
	PORT_DIPSETTING(    0x00, "5" )

	PORT_MODIFY("IN0")
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNUSED )	// only two buttons

	PORT_MODIFY("IN1")
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNUSED )	// only two buttons
INPUT_PORTS_END

INPUT_PORTS_START( suprmanj )
	PORT_INCLUDE( superman )

	PORT_MODIFY("DSWA")
	TAITO_COINAGE_JAPAN_8
INPUT_PORTS_END

INPUT_PORTS_START( twinhawk )
	PORT_INCLUDE( taitox )

	PORT_MODIFY("DSWA")
	TAITO_COINAGE_WORLD_8

	PORT_MODIFY("DSWB")
	PORT_DIPNAME( 0x0c, 0x0c, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x0c, "50k and every 150k" )
	PORT_DIPSETTING(    0x08, "70k and every 200k" )
	PORT_DIPSETTING(    0x04, "50k only" )
	PORT_DIPSETTING(    0x00, DEF_STR( None ) )
	PORT_DIPNAME( 0x30, 0x30, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x00, "2" )
	PORT_DIPSETTING(    0x30, "3" )
	PORT_DIPSETTING(    0x10, "4" )
	PORT_DIPSETTING(    0x20, "5" )
	PORT_DIPNAME( 0x80, 0x00, DEF_STR( Allow_Continue ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
INPUT_PORTS_END

INPUT_PORTS_START( twinhwku )
	PORT_INCLUDE( twinhawk )

	PORT_MODIFY("DSWA")
	TAITO_COINAGE_US_8
INPUT_PORTS_END

INPUT_PORTS_START( daisenpu )
	PORT_INCLUDE( taitox )

	PORT_MODIFY("DSWA")
	PORT_DIPNAME( 0x01, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Cocktail ) )
	TAITO_COINAGE_JAPAN_8

	PORT_MODIFY("DSWB")
	PORT_DIPNAME( 0x0c, 0x0c, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x08, "50k and every 150k" )
	PORT_DIPSETTING(    0x0c, "70k and every 200k" )
	PORT_DIPSETTING(    0x04, "100k only" )
	PORT_DIPSETTING(    0x00, DEF_STR( None ) )
	PORT_DIPNAME( 0x30, 0x30, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x00, "2" )
	PORT_DIPSETTING(    0x30, "3" )
	PORT_DIPSETTING(    0x10, "4" )
	PORT_DIPSETTING(    0x20, "5" )
	PORT_DIPNAME( 0x80, 0x00, DEF_STR( Allow_Continue ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
INPUT_PORTS_END

INPUT_PORTS_START( easttech )
	PORT_INCLUDE( taitox )

	// dips are totally different from the Taito games
	PORT_MODIFY("DSWA")
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 2C_1C ))
	PORT_DIPSETTING(    0x03, DEF_STR( 1C_1C ))
	PORT_DIPSETTING(    0x00, DEF_STR( 2C_3C ))
	PORT_DIPSETTING(    0x02, DEF_STR( 1C_2C ))
	PORT_DIPNAME( 0x0c, 0x0c, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 2C_1C ))
	PORT_DIPSETTING(    0x0c, DEF_STR( 1C_1C ))
	PORT_DIPSETTING(    0x00, DEF_STR( 2C_3C ))
	PORT_DIPSETTING(    0x08, DEF_STR( 1C_2C ))
	PORT_DIPNAME( 0x60, 0x40, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x60, DEF_STR( Easy ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Normal ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Hard ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Hardest ) )

	PORT_MODIFY("DSWB")
	PORT_DIPNAME( 0x01, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x00, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x02, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x00, DEF_STR( Allow_Continue ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_SERVICE( 0x80, IP_ACTIVE_LOW )
INPUT_PORTS_END

INPUT_PORTS_START( gigandes )
	PORT_INCLUDE( easttech )

	PORT_MODIFY("DSWA")
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Free_Play ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_MODIFY("DSWB")
	PORT_DIPNAME( 0x18, 0x18, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x18, "3" )
	PORT_DIPSETTING(    0x10, "4" )
	PORT_DIPSETTING(    0x08, "5" )
	PORT_DIPSETTING(    0x00, "6" )
	PORT_DIPNAME( 0x20, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0x40, 0x00, DEF_STR( Language ) )
	PORT_DIPSETTING(    0x00, DEF_STR( English ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Japanese ) )
INPUT_PORTS_END

INPUT_PORTS_START( ballbros )
	PORT_INCLUDE( easttech )

	PORT_MODIFY("DSWA")
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 1C_1C ))
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_2C ))
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_MODIFY("DSWB")
	PORT_DIPNAME( 0x20, 0x00, DEF_STR( Language ) )
	PORT_DIPSETTING(    0x00, DEF_STR( English ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Japanese ) )
INPUT_PORTS_END

INPUT_PORTS_START( kyustrkr )
	PORT_INCLUDE( easttech )

	PORT_MODIFY("DSWA")
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )	// Free play in test mode, does not work
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_MODIFY("DSWB")
	PORT_DIPNAME( 0x40, 0x00, DEF_STR( Language ) )
	PORT_DIPSETTING(    0x00, DEF_STR( English ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Japanese ) )
INPUT_PORTS_END


/**************************************************************************/

#define NUM_TILES 16384
static const gfx_layout tilelayout =
{
	16,16,  /* 16*16 sprites */
	NUM_TILES,	/* 16384 of them */
	4,	       /* 4 bits per pixel */
	{ 64*8*NUM_TILES + 8, 64*8*NUM_TILES + 0, 8, 0 },
	{ 0, 1, 2, 3, 4, 5, 6, 7,
		8*16, 8*16+1, 8*16+2, 8*16+3, 8*16+4, 8*16+5, 8*16+6, 8*16+7 },
	{ 0*16, 1*16, 2*16, 3*16, 4*16, 5*16, 6*16, 7*16,
		16*16, 17*16, 18*16, 19*16, 20*16, 21*16, 22*16, 23*16 },

	64*8	/* every sprite takes 64 consecutive bytes */
};
#undef NUM_TILES

static const gfx_layout ballbros_tilelayout =
{
	16,16,  /* 16*16 sprites */
	4096,	/* 4096 of them */
	4,	       /* 4 bits per pixel */
	{ 0x20000*3*8, 0x20000*2*8, 0x20000*1*8, 0 },
	{ 0, 1, 2, 3, 4, 5, 6, 7,
		8*8, 8*8+1, 8*8+2, 8*8+3, 8*8+4, 8*8+5, 8*8+6, 8*8+7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8,
		16*8, 17*8, 18*8, 19*8, 20*8, 21*8, 22*8, 23*8 },

	32*8	/* every sprite takes 64 consecutive bytes */
};

static const gfx_decode superman_gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0x000000, &tilelayout,    0, 256 },	 /* sprites & playfield */
	{ -1 } /* end of array */
};

static const gfx_decode ballbros_gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0x000000, &ballbros_tilelayout,    0, 256 },	 /* sprites & playfield */
	{ -1 } /* end of array */
};


/**************************************************************************/

/* handler called by the YM2610 emulator when the internal timers cause an IRQ */
static void irqhandler(int irq)
{
	cpunum_set_input_line(1,0,irq ? ASSERT_LINE : CLEAR_LINE);
}

static struct YM2610interface ym2610_interface =
{
	irqhandler,
	REGION_SOUND1,
	REGION_SOUND1
};

static struct YM2610interface ballbros_ym2610_interface =
{
	irqhandler,
	REGION_SOUND1,
	REGION_SOUND2
};

static struct YM2151interface ym2151_interface =
{
	irqhandler
};


MACHINE_START( taitox )
{
	state_save_register_global(banknum);
	state_save_register_func_postload(reset_sound_region);
	return 0;
}

/**************************************************************************/

static MACHINE_DRIVER_START( superman )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000, 8000000)	/* 8 MHz? */
	MDRV_CPU_PROGRAM_MAP(superman_readmem,superman_writemem)
	MDRV_CPU_VBLANK_INT(irq6_line_hold,1)

	MDRV_CPU_ADD(Z80, 4000000)	/* 4 MHz ??? */
	MDRV_CPU_PROGRAM_MAP(sound_readmem,sound_writemem)

	MDRV_FRAMES_PER_SECOND(57.43)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	MDRV_MACHINE_START(taitox)
	MDRV_MACHINE_RESET(cchip1)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(52*8, 32*8)
	MDRV_VISIBLE_AREA(0*8, 48*8-1, 1*8, 31*8-1)
	MDRV_GFXDECODE(superman_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(2048)

	MDRV_VIDEO_START(seta_no_layers)
	MDRV_VIDEO_UPDATE(seta_no_layers)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_STEREO("left", "right")

	MDRV_SOUND_ADD(YM2610, 8000000)
	MDRV_SOUND_CONFIG(ym2610_interface)
	MDRV_SOUND_ROUTE(0, "left",  0.25)
	MDRV_SOUND_ROUTE(0, "right", 0.25)
	MDRV_SOUND_ROUTE(1, "left",  1.0)
	MDRV_SOUND_ROUTE(2, "right", 1.0)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( daisenpu )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000, 8000000)	/* 8 MHz? */
	MDRV_CPU_PROGRAM_MAP(daisenpu_readmem,daisenpu_writemem)
	MDRV_CPU_VBLANK_INT(irq2_line_hold,1)

	MDRV_CPU_ADD(Z80, 4000000)	/* 4 MHz ??? */
	MDRV_CPU_PROGRAM_MAP(daisenpu_sound_readmem,daisenpu_sound_writemem)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	MDRV_MACHINE_START(taitox)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(52*8, 32*8)
	MDRV_VISIBLE_AREA(0*8, 48*8-1, 2*8, 30*8-1)
	MDRV_GFXDECODE(superman_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(2048)

	MDRV_VIDEO_START(seta_no_layers)
	MDRV_VIDEO_UPDATE(seta_no_layers)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_STEREO("left", "right")

	MDRV_SOUND_ADD(YM2151, 4000000)
	MDRV_SOUND_CONFIG(ym2151_interface)
	MDRV_SOUND_ROUTE(0, "left", 0.45)
	MDRV_SOUND_ROUTE(1, "right", 0.45)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( gigandes )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000, 8000000)	/* 8 MHz? */
	MDRV_CPU_PROGRAM_MAP(gigandes_readmem,gigandes_writemem)
	MDRV_CPU_VBLANK_INT(irq2_line_hold,1)

	MDRV_CPU_ADD(Z80, 4000000)	/* 4 MHz ??? */
	MDRV_CPU_PROGRAM_MAP(ballbros_sound_readmem,ballbros_sound_writemem)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	MDRV_MACHINE_START(taitox)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(52*8, 32*8)
	MDRV_VISIBLE_AREA(0*8, 48*8-1, 1*8, 31*8-1)
	MDRV_GFXDECODE(superman_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(2048)

	MDRV_VIDEO_START(seta_no_layers)
	MDRV_VIDEO_UPDATE(seta_no_layers)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_STEREO("left", "right")

	MDRV_SOUND_ADD(YM2610, 8000000)
	MDRV_SOUND_CONFIG(ballbros_ym2610_interface)
	MDRV_SOUND_ROUTE(0, "left",  0.25)
	MDRV_SOUND_ROUTE(0, "right", 0.25)
	MDRV_SOUND_ROUTE(1, "left",  1.0)
	MDRV_SOUND_ROUTE(2, "right", 1.0)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( ballbros )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000, 8000000)	/* 8 MHz? */
	MDRV_CPU_PROGRAM_MAP(ballbros_readmem,ballbros_writemem)
	MDRV_CPU_VBLANK_INT(irq2_line_hold,1)

	MDRV_CPU_ADD(Z80, 4000000)	/* 4 MHz ??? */
	MDRV_CPU_PROGRAM_MAP(ballbros_sound_readmem,ballbros_sound_writemem)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	MDRV_MACHINE_START(taitox)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(52*8, 32*8)
	MDRV_VISIBLE_AREA(0*8, 48*8-1, 1*8, 31*8-1)

	MDRV_GFXDECODE(ballbros_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(2048)

	MDRV_VIDEO_START(seta_no_layers)
	MDRV_VIDEO_UPDATE(seta_no_layers)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_STEREO("left", "right")

	MDRV_SOUND_ADD(YM2610, 8000000)
	MDRV_SOUND_CONFIG(ballbros_ym2610_interface)
	MDRV_SOUND_ROUTE(0, "left",  0.25)
	MDRV_SOUND_ROUTE(0, "right", 0.25)
	MDRV_SOUND_ROUTE(1, "left",  1.0)
	MDRV_SOUND_ROUTE(2, "right", 1.0)
MACHINE_DRIVER_END


/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START( superman )
	ROM_REGION( 0x80000, REGION_CPU1, 0 )     /* 512k for 68000 code */
	ROM_LOAD16_BYTE( "a10_09.bin", 0x00000, 0x20000, CRC(640f1d58) SHA1(e768d32eae1dba39c23189996fbd5454c8627809) )
	ROM_LOAD16_BYTE( "a05_07.bin", 0x00001, 0x20000, CRC(fddb9953) SHA1(8b562712810a5a72f4647f1ba1314a1be2e249e7) )
	ROM_LOAD16_BYTE( "a08_08.bin", 0x40000, 0x20000, CRC(79fc028e) SHA1(bf42b3f84dcad8fd9085c702a78dc895cc12d670) )
	ROM_LOAD16_BYTE( "a03_13.bin", 0x40001, 0x20000, CRC(9f446a44) SHA1(16f7cd6438e47fdaac93a368df5c093f6ff0f1f0) )

	ROM_REGION( 0x1c000, REGION_CPU2, 0 )     /* 64k for Z80 code */
	ROM_LOAD( "d18_10.bin", 0x00000, 0x4000, CRC(6efe79e8) SHA1(7a76efaaeab71473f4b0b23a89141f203488ce1d) )
	ROM_CONTINUE(           0x10000, 0xc000 ) /* banked stuff */

	ROM_REGION( 0x200000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "f01_14.bin", 0x000000, 0x80000, CRC(89368c3e) SHA1(8d227439ab321fd5d432d860544daea0e78ce588) ) /* Plane 0, 1 */
	ROM_LOAD( "h01_15.bin", 0x080000, 0x80000, CRC(910cc4f9) SHA1(9ecfa84123a8f9d048f0a689647e92f25af73899) )
	ROM_LOAD( "j01_16.bin", 0x100000, 0x80000, CRC(3622ed2f) SHA1(03f4383f6ff8b5f1e26bc6bbef2fb1855d3bb93f) ) /* Plane 2, 3 */
	ROM_LOAD( "k01_17.bin", 0x180000, 0x80000, CRC(c34f27e0) SHA1(07ee02c18ce29f35e8ae87d0c1ed80b726c246a6) )

	ROM_REGION( 0x80000, REGION_SOUND1, 0 )	/* ADPCM samples */
	ROM_LOAD( "e18_01.bin", 0x00000, 0x80000, CRC(3cf99786) SHA1(f6febf9bda87ca04f0a5890d0e8001c26dfa6c81) )
ROM_END

ROM_START( suprmanj )
	ROM_REGION( 0x80000, REGION_CPU1, 0 )     /* 512k for 68000 code */
	ROM_LOAD16_BYTE( "a10_09.bin", 0x00000, 0x20000, CRC(640f1d58) SHA1(e768d32eae1dba39c23189996fbd5454c8627809) )
	ROM_LOAD16_BYTE( "a05_07.bin", 0x00001, 0x20000, CRC(fddb9953) SHA1(8b562712810a5a72f4647f1ba1314a1be2e249e7) )
	ROM_LOAD16_BYTE( "a08_08.bin", 0x40000, 0x20000, CRC(79fc028e) SHA1(bf42b3f84dcad8fd9085c702a78dc895cc12d670) )
	ROM_LOAD16_BYTE( "b61-06.a3",  0x40001, 0x20000, CRC(714a0b68) SHA1(b0b42c55d2404c7c193eb8cab3bd92e321947845) )

	ROM_REGION( 0x1c000, REGION_CPU2, 0 )     /* 64k for Z80 code */
	ROM_LOAD( "d18_10.bin", 0x00000, 0x4000, CRC(6efe79e8) SHA1(7a76efaaeab71473f4b0b23a89141f203488ce1d) )
	ROM_CONTINUE(           0x10000, 0xc000 ) /* banked stuff */

	ROM_REGION( 0x200000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "f01_14.bin", 0x000000, 0x80000, CRC(89368c3e) SHA1(8d227439ab321fd5d432d860544daea0e78ce588) ) /* Plane 0, 1 */
	ROM_LOAD( "h01_15.bin", 0x080000, 0x80000, CRC(910cc4f9) SHA1(9ecfa84123a8f9d048f0a689647e92f25af73899) )
	ROM_LOAD( "j01_16.bin", 0x100000, 0x80000, CRC(3622ed2f) SHA1(03f4383f6ff8b5f1e26bc6bbef2fb1855d3bb93f) ) /* Plane 2, 3 */
	ROM_LOAD( "k01_17.bin", 0x180000, 0x80000, CRC(c34f27e0) SHA1(07ee02c18ce29f35e8ae87d0c1ed80b726c246a6) )

	ROM_REGION( 0x80000, REGION_SOUND1, 0 )	/* ADPCM samples */
	ROM_LOAD( "e18_01.bin", 0x00000, 0x80000, CRC(3cf99786) SHA1(f6febf9bda87ca04f0a5890d0e8001c26dfa6c81) )
ROM_END

ROM_START( twinhawk )
	ROM_REGION( 0x40000, REGION_CPU1, 0 )     /* 256k for 68000 code */
	ROM_LOAD16_BYTE( "b87-11.bin", 0x00000, 0x20000, CRC(fc84a399) SHA1(6e5552b7ee433bee74f8936a8e583b5f81b5f2b2) )
	ROM_LOAD16_BYTE( "b87-10.bin", 0x00001, 0x20000, CRC(17181706) SHA1(b7cab502b68a8f02918412538f23682120cbe1d3) )

	ROM_REGION( 0x14000, REGION_CPU2, 0 )     /* 32k for Z80 code */
	ROM_LOAD( "b87-07", 0x00000, 0x4000, CRC(e2e0efa0) SHA1(4f1435ba738895996f26a64c2237e8349337df4a) )
	ROM_CONTINUE(       0x10000, 0x4000 ) /* banked stuff */

	ROM_REGION( 0x200000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "b87-02", 0x000000, 0x80000, CRC(89ad43a0) SHA1(6ff6ee085c1c06a05f4f8743d979d3552b7475a0) ) /* Plane 0, 1 */
	ROM_LOAD( "b87-01", 0x080000, 0x80000, CRC(81e82ae1) SHA1(d4dbdbf9ae0af69bbeccafb3cc2f67dadda72432) )
	ROM_LOAD( "b87-04", 0x100000, 0x80000, CRC(958434b6) SHA1(cf5912c4468cb2079ff180203045a436175c037c) ) /* Plane 2, 3 */
	ROM_LOAD( "b87-03", 0x180000, 0x80000, CRC(ce155ae0) SHA1(7293125fc23f2411c4edd427a2576c145b3f2dd4) )
ROM_END

ROM_START( twinhwku )
	ROM_REGION( 0x40000, REGION_CPU1, 0 )     /* 256k for 68000 code */
	ROM_LOAD16_BYTE( "b87-09.u6", 0x00000, 0x20000, CRC(7e6267c7) SHA1(a623c1b740008675f36e8b63bbc17a573917db30) )
	ROM_LOAD16_BYTE( "b87-08.u4", 0x00001, 0x20000, CRC(31d9916f) SHA1(8ae491a51a4095717c6f65fe81a83902feccd54b) )

	ROM_REGION( 0x14000, REGION_CPU2, 0 )     /* 32k for Z80 code */
	ROM_LOAD( "b87-07", 0x00000, 0x4000, CRC(e2e0efa0) SHA1(4f1435ba738895996f26a64c2237e8349337df4a) )
	ROM_CONTINUE(       0x10000, 0x4000 ) /* banked stuff */

	ROM_REGION( 0x200000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "b87-02", 0x000000, 0x80000, CRC(89ad43a0) SHA1(6ff6ee085c1c06a05f4f8743d979d3552b7475a0) ) /* Plane 0, 1 */
	ROM_LOAD( "b87-01", 0x080000, 0x80000, CRC(81e82ae1) SHA1(d4dbdbf9ae0af69bbeccafb3cc2f67dadda72432) )
	ROM_LOAD( "b87-04", 0x100000, 0x80000, CRC(958434b6) SHA1(cf5912c4468cb2079ff180203045a436175c037c) ) /* Plane 2, 3 */
	ROM_LOAD( "b87-03", 0x180000, 0x80000, CRC(ce155ae0) SHA1(7293125fc23f2411c4edd427a2576c145b3f2dd4) )
ROM_END

ROM_START( daisenpu )
	ROM_REGION( 0x40000, REGION_CPU1, 0 )     /* 256k for 68000 code */
	ROM_LOAD16_BYTE( "b87-06", 0x00000, 0x20000, CRC(cf236100) SHA1(7944a20950188f64c0a09edd1a4efe0270264b27) )
	ROM_LOAD16_BYTE( "b87-05", 0x00001, 0x20000, CRC(7f15edc7) SHA1(3deba512f3c97f354ed4155f62058da160047bc5) )

	ROM_REGION( 0x14000, REGION_CPU2, 0 )     /* 32k for Z80 code */
	ROM_LOAD( "b87-07", 0x00000, 0x4000, CRC(e2e0efa0) SHA1(4f1435ba738895996f26a64c2237e8349337df4a) )
	ROM_CONTINUE(       0x10000, 0x4000 ) /* banked stuff */

	ROM_REGION( 0x200000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "b87-02", 0x000000, 0x80000, CRC(89ad43a0) SHA1(6ff6ee085c1c06a05f4f8743d979d3552b7475a0) ) /* Plane 0, 1 */
	ROM_LOAD( "b87-01", 0x080000, 0x80000, CRC(81e82ae1) SHA1(d4dbdbf9ae0af69bbeccafb3cc2f67dadda72432) )
	ROM_LOAD( "b87-04", 0x100000, 0x80000, CRC(958434b6) SHA1(cf5912c4468cb2079ff180203045a436175c037c) ) /* Plane 2, 3 */
	ROM_LOAD( "b87-03", 0x180000, 0x80000, CRC(ce155ae0) SHA1(7293125fc23f2411c4edd427a2576c145b3f2dd4) )
ROM_END

ROM_START( gigandes )
	ROM_REGION( 0x80000, REGION_CPU1, 0 )     /* 512k for 68000 code */
	ROM_LOAD16_BYTE( "1.10a",      0x00000, 0x20000, CRC(290c50e0) SHA1(ac8008619891a5b54ba2069e4e18836976532c99) )
	ROM_LOAD16_BYTE( "3.5a",       0x00001, 0x20000, CRC(9cef82af) SHA1(6dad850de699d40dfba54bde6baca75bb0059c83) )
	ROM_LOAD16_BYTE( "east_2.8a",  0x40000, 0x20000, CRC(dd94b4d0) SHA1(2efff9fd51b28fd1fb46d16b359f0991af91054e) )
	ROM_LOAD16_BYTE( "east_4.3a",  0x40001, 0x20000, CRC(a647310a) SHA1(49db488a36f6c74729825bdf0214bcd30773eaf4) )

	ROM_REGION( 0x1c000, REGION_CPU2, 0 )     /* 64k for Z80 code */
	ROM_LOAD( "east_5.17d", 0x00000, 0x4000, CRC(b24ab5f4) SHA1(e4730df984e9686c538df5fc626b795bda1db939) )
	ROM_CONTINUE(           0x10000, 0xc000 ) /* banked stuff */

	ROM_REGION( 0x200000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "east_8.3f", 0x000000, 0x80000, CRC(75eece28) SHA1(7ce66cd8bca7dd214367beae067727c8735c0f7e) ) /* Plane 0, 1 */
	ROM_LOAD( "east_7.3h", 0x080000, 0x80000, CRC(b179a76a) SHA1(cff2caf1eb0dda8a1b8283b9950b908b102f61de) )
	ROM_LOAD( "east_9.3j", 0x100000, 0x80000, CRC(5c5e6898) SHA1(f348ac752a571902c55f36e21aa3fb9ef97528e3) ) /* Plane 2, 3 */
	ROM_LOAD( "east_6.3k", 0x180000, 0x80000, CRC(52db30e9) SHA1(0b6d73f2c6e6c1ad5fcb2a9edf50069cd0691483) )

	ROM_REGION( 0x80000, REGION_SOUND1, 0 )	/* ADPCM samples */
	ROM_LOAD( "east-11.16f", 0x00000, 0x80000, CRC(92111f96) SHA1(e781f24761b7a923388f4cda64c7b31388fd64c5) )

	ROM_REGION( 0x80000, REGION_SOUND2, 0 )	/* Delta-T samples */
	ROM_LOAD( "east-10.16e", 0x00000, 0x80000, CRC(ca0ac419) SHA1(b29f30a8ff1286c65b741353b6551918a45bcafe) )
ROM_END

ROM_START( gigandsj )
	ROM_REGION( 0x80000, REGION_CPU1, 0 )     /* 512k for 68000 code */
	ROM_LOAD16_BYTE( "east_1.10a", 0x00000, 0x20000, CRC(ae74e4e5) SHA1(1CAC0A0E591B63142D8D249C67F803256FB28C2A) )
	ROM_LOAD16_BYTE( "east_3.5a",  0x00001, 0x20000, CRC(8bcf2116) SHA1(9255D7E0AB568AD7A894421D3260FA80B8A0A5D0) )
	ROM_LOAD16_BYTE( "east_2.8a",  0x40000, 0x20000, CRC(dd94b4d0) SHA1(2efff9fd51b28fd1fb46d16b359f0991af91054e) )
	ROM_LOAD16_BYTE( "east_4.3a",  0x40001, 0x20000, CRC(a647310a) SHA1(49db488a36f6c74729825bdf0214bcd30773eaf4) )

	ROM_REGION( 0x1c000, REGION_CPU2, 0 )     /* 64k for Z80 code */
	ROM_LOAD( "east_5.17d", 0x00000, 0x4000, CRC(b24ab5f4) SHA1(e4730df984e9686c538df5fc626b795bda1db939) )
	ROM_CONTINUE(           0x10000, 0xc000 ) /* banked stuff */

	ROM_REGION( 0x200000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "east_8.3f", 0x000000, 0x80000, CRC(75eece28) SHA1(7ce66cd8bca7dd214367beae067727c8735c0f7e) ) /* Plane 0, 1 */
	ROM_LOAD( "east_7.3h", 0x080000, 0x80000, CRC(b179a76a) SHA1(cff2caf1eb0dda8a1b8283b9950b908b102f61de) )
	ROM_LOAD( "east_9.3j", 0x100000, 0x80000, CRC(5c5e6898) SHA1(f348ac752a571902c55f36e21aa3fb9ef97528e3) ) /* Plane 2, 3 */
	ROM_LOAD( "east_6.3k", 0x180000, 0x80000, CRC(52db30e9) SHA1(0b6d73f2c6e6c1ad5fcb2a9edf50069cd0691483) )

	ROM_REGION( 0x80000, REGION_SOUND1, 0 )	/* ADPCM samples */
	ROM_LOAD( "east-11.16f", 0x00000, 0x80000, CRC(92111f96) SHA1(e781f24761b7a923388f4cda64c7b31388fd64c5) )

	ROM_REGION( 0x80000, REGION_SOUND2, 0 )	/* Delta-T samples */
	ROM_LOAD( "east-10.16e", 0x00000, 0x80000, CRC(ca0ac419) SHA1(b29f30a8ff1286c65b741353b6551918a45bcafe) )
ROM_END

ROM_START( ballbros )
	ROM_REGION( 0x40000, REGION_CPU1, 0 )     /* 256k for 68000 code */
	ROM_LOAD16_BYTE( "10a", 0x00000, 0x20000, CRC(4af0e858) SHA1(817817169aee075d52411bdbe568514511760386) )
	ROM_LOAD16_BYTE( "5a",  0x00001, 0x20000, CRC(0b983a69) SHA1(7be06761a19e1dc5d1404d1920797b406421e365) )

	ROM_REGION( 0x1c000, REGION_CPU2, 0 )     /* 64k for Z80 code */
	ROM_LOAD( "8d", 0x00000, 0x4000, CRC(d1c515af) SHA1(00451991b4c793487b156f9be2b2e4688325ff24) )
	ROM_CONTINUE(   0x10000, 0xc000 ) /* banked stuff */

	ROM_REGION( 0x080000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "3", 0x000000, 0x20000, CRC(ec3e0537) SHA1(51fe5c6ef007c188b2f51ad2225753d2b403e35a) ) /* Plane 0, 1 */
	ROM_LOAD( "2", 0x020000, 0x20000, CRC(bb441717) SHA1(205ae0aa3ded11766ae8f6fe7d08fefff17a9b73) )
	ROM_LOAD( "1", 0x040000, 0x20000, CRC(8196d624) SHA1(c859e3b1d3b481f38cfe47576efc1dcdbe6cde28) )
	ROM_LOAD( "0", 0x060000, 0x20000, CRC(1cc584e5) SHA1(18cf607fa06c095d088b80cea2a1e507d19c7126) )

	ROM_REGION( 0x80000, REGION_SOUND1, 0 )	/* ADPCM samples */
	ROM_LOAD( "east-11", 0x00000, 0x80000, CRC(92111f96) SHA1(e781f24761b7a923388f4cda64c7b31388fd64c5) )

	ROM_REGION( 0x80000, REGION_SOUND2, 0 )	/* Delta-T samples */
	ROM_LOAD( "east-10", 0x00000, 0x80000, CRC(ca0ac419) SHA1(b29f30a8ff1286c65b741353b6551918a45bcafe) )
ROM_END

ROM_START( kyustrkr )
	ROM_REGION( 0x40000, REGION_CPU1, 0 )     /* 512k for 68000 code */
	ROM_LOAD16_BYTE( "pe.9a", 0x00000, 0x20000, CRC(082b5f96) SHA1(97c08b506b2a07d63f3323359b8564aa3621f483) )
	ROM_LOAD16_BYTE( "po.4a", 0x00001, 0x20000, CRC(0100361e) SHA1(45791f697c86309c459d0d8c3d3e967a3ece3ede) )

	ROM_REGION( 0x1c000, REGION_CPU2, 0 )     /* 64k for Z80 code */
	ROM_LOAD( "ic.18d", 0x00000, 0x4000, CRC(92cfb788) SHA1(41cd5433584df05652bd0ce8c5a35dc38262d6f2) )
	ROM_CONTINUE(       0x10000, 0xc000 ) /* banked stuff */

	ROM_REGION( 0x80000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "m-8-3.u3",     0x00000, 0x20000, CRC(1c4084e6) SHA1(addea2ba07bddb41fbe7f0fc859e744990bb9ff5) )
	ROM_LOAD( "m-8-2.u4",     0x20000, 0x20000, CRC(ada21c4d) SHA1(a683c8d798370c50d9bd5e67e91d7ed0f1659c20) )
	ROM_LOAD( "m-8-1.u5",     0x40000, 0x20000, CRC(9d95aad6) SHA1(3391b14196fea12223ab247d909791bc68fc8d56) )
	ROM_LOAD( "m-8-0.u6",     0x60000, 0x20000, CRC(0dfb6ed3) SHA1(0937614c8f97040d0216363bfb2bc21161128a3c) )

	ROM_REGION( 0x80000, REGION_SOUND1, 0 )	/* ADPCM samples */
	ROM_LOAD( "m-8-5.u2",     0x00000, 0x20000, CRC(d9d90e0a) SHA1(1011548b4fb5f1a194c93ded512e74cda2c06ceb) )

	ROM_REGION( 0x80000, REGION_SOUND2, 0 )	/* Delta-T samples */
	ROM_LOAD( "m-8-4.u1",     0x00000, 0x20000, CRC(d3f6047a) SHA1(0db6d762bbe2d68cddf30e06125b904e1021b96d) )
ROM_END

DRIVER_INIT( kyustrkr )
{
	memory_install_write16_handler(0, ADDRESS_SPACE_PROGRAM, 0x900000, 0x90000f, 0, 0, kyustrkr_input_w);
}

GAME( 1988, superman, 0,        superman, superman, 0,        ROT0,   "Taito Corporation", "Superman", 0 )
GAME( 1988, suprmanj, superman, superman, suprmanj, 0,        ROT0,   "Taito Corporation", "Superman (Japan)", 0 )
GAME( 1989, twinhawk, 0,        daisenpu, twinhawk, 0,        ROT270, "Taito Corporation Japan", "Twin Hawk (World)", 0 )
GAME( 1989, twinhwku, twinhawk, daisenpu, twinhwku, 0,        ROT270, "Taito America Corporation", "Twin Hawk (US)", 0 )
GAME( 1989, daisenpu, twinhawk, daisenpu, daisenpu, 0,        ROT270, "Taito Corporation", "Daisenpu (Japan)", 0 )
GAME( 1989, gigandes, 0,        gigandes, gigandes, 0,        ROT0,   "East Technology", "Gigandes", 0 )
GAME( 1989, gigandsj, gigandes, gigandes, gigandes, 0,        ROT0,   "East Technology", "Gigandes (Japan)", 0 )
GAME( 1989, kyustrkr, 0,        ballbros, kyustrkr, kyustrkr, ROT180, "East Technology", "Last Striker / Kyuukyoku no Striker", 0 )
GAME( 1992, ballbros, 0,        ballbros, ballbros, 0,        ROT0,   "East Technology", "Balloon Brothers", 0 )
