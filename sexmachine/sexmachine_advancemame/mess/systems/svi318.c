/*
** svi318.c : driver for Spectravideo SVI-318 and SVI-328
**
** Sean Young, Tomas Karlsson
*/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "machine/8255ppi.h"
#include "vidhrdw/tms9928a.h"
#include "includes/svi318.h"
#include "machine/wd17xx.h"
#include "devices/basicdsk.h"
#include "devices/printer.h"
#include "devices/cartslot.h"
#include "devices/cassette.h"
#include "formats/svi_cas.h"
#include "sound/dac.h"
#include "sound/ay8910.h"

static ADDRESS_MAP_START( svi318_mem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE( 0x0000, 0x7fff) AM_READWRITE( MRA8_BANK1, svi318_writemem0 )
	AM_RANGE( 0x8000, 0xbfff) AM_READ( MRA8_BANK2 )
	AM_RANGE( 0xc000, 0xffff) AM_READ( MRA8_BANK3 )
	AM_RANGE( 0x8000, 0xffff) AM_WRITE( svi318_writemem1 )
ADDRESS_MAP_END



static ADDRESS_MAP_START( svi318_io, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_FLAGS( AMEF_UNMAP(0xff) )
	ADDRESS_MAP_FLAGS( AMEF_ABITS(8) )
	AM_RANGE( 0x10, 0x11) AM_WRITE( svi318_printer_w )
	AM_RANGE( 0x12, 0x12) AM_READ( svi318_printer_r )
	AM_RANGE( 0x30, 0x30) AM_READWRITE( wd179x_status_r, wd179x_command_w )
	AM_RANGE( 0x31, 0x31) AM_READWRITE( wd179x_track_r, wd179x_track_w )
	AM_RANGE( 0x32, 0x32) AM_READWRITE( wd179x_sector_r, wd179x_sector_w )
	AM_RANGE( 0x33, 0x33) AM_READWRITE( wd179x_data_r, wd179x_data_w )
	AM_RANGE( 0x34, 0x34) AM_READWRITE( svi318_fdc_irqdrq_r, svi318_fdc_drive_motor_w )
	AM_RANGE( 0x38, 0x38) AM_WRITE( svi318_fdc_density_side_w )
	AM_RANGE( 0x80, 0x80) AM_WRITE( TMS9928A_vram_w )
	AM_RANGE( 0x81, 0x81) AM_WRITE( TMS9928A_register_w )
	AM_RANGE( 0x84, 0x84) AM_READ( TMS9928A_vram_r )
	AM_RANGE( 0x85, 0x85) AM_READ( TMS9928A_register_r )
	AM_RANGE( 0x88, 0x88) AM_WRITE( AY8910_control_port_0_w )
	AM_RANGE( 0x8c, 0x8c) AM_WRITE( AY8910_write_port_0_w )
	AM_RANGE( 0x90, 0x90) AM_READ( AY8910_read_port_0_r )
	AM_RANGE( 0x96, 0x97) AM_WRITE( svi318_ppi_w )
	AM_RANGE( 0x98, 0x9a) AM_READ( svi318_ppi_r )
ADDRESS_MAP_END

static ADDRESS_MAP_START( svi318_io2, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_FLAGS( AMEF_UNMAP(0xff) )
	ADDRESS_MAP_FLAGS( AMEF_ABITS(8) )
	AM_RANGE( 0x10, 0x11) AM_WRITE( svi318_printer_w )
	AM_RANGE( 0x12, 0x12) AM_READ( svi318_printer_r )
	AM_RANGE( 0x30, 0x30) AM_READWRITE( wd179x_status_r, wd179x_command_w )
	AM_RANGE( 0x31, 0x31) AM_READWRITE( wd179x_track_r, wd179x_track_w )
	AM_RANGE( 0x32, 0x32) AM_READWRITE( wd179x_sector_r, wd179x_sector_w )
	AM_RANGE( 0x33, 0x33) AM_READWRITE( wd179x_data_r, wd179x_data_w )
	AM_RANGE( 0x34, 0x34) AM_READWRITE( svi318_fdc_irqdrq_r, svi318_fdc_drive_motor_w )
	AM_RANGE( 0x38, 0x38) AM_WRITE( svi318_fdc_density_side_w )
	AM_RANGE( 0x50, 0x51) AM_READWRITE( svi318_crtc_r, svi318_crtc_w )
	AM_RANGE( 0x58, 0x58) AM_WRITE( svi318_crtcbank_w )
	AM_RANGE( 0x80, 0x80) AM_WRITE( TMS9928A_vram_w )
	AM_RANGE( 0x81, 0x81) AM_WRITE( TMS9928A_register_w )
	AM_RANGE( 0x84, 0x84) AM_READ( TMS9928A_vram_r )
	AM_RANGE( 0x85, 0x85) AM_READ( TMS9928A_register_r )
	AM_RANGE( 0x88, 0x88) AM_WRITE( AY8910_control_port_0_w )
	AM_RANGE( 0x8c, 0x8c) AM_WRITE( AY8910_write_port_0_w )
	AM_RANGE( 0x90, 0x90) AM_READ( AY8910_read_port_0_r )
	AM_RANGE( 0x96, 0x97) AM_WRITE( svi318_ppi_w )
	AM_RANGE( 0x98, 0x9a) AM_READ( svi318_ppi_r )
ADDRESS_MAP_END

/*
  Keyboard status table
  
       Bit#:|  7  |  6  |  5  |  4  |  3  |  2  |  1  |  0  |
            |     |     |     |     |     |     |     |     |
  Line:     |     |     |     |     |     |     |     |     |
   ---------+-----+-----+-----+-----+-----+-----+-----+-----|
    0       | "7" | "6" | "5" | "4" | "3" | "2" | "1" | "0" |
   ---------+-----+-----+-----+-----+-----+-----+-----+-----|
    1       | "/" | "." | "=" | "," | "'" | ":" | "9" | "8" |
   ---------+-----+-----+-----+-----+-----+-----+-----+-----|
    2       | "G" | "F" | "E" | "D" | "C" | "B" | "A" | "-" |
   ---------+-----+-----+-----+-----+-----+-----+-----+-----|
    3       | "O" | "N" | "M" | "L" | "K" | "J" | "I" | "H" |
   ---------+-----+-----+-----+-----+-----+-----+-----+-----|
    4       | "W" | "V" | "U" | "T" | "S" | "R" | "Q" | "P" |
   ---------+-----+-----+-----+-----+-----+-----+-----+-----|
    5       | UP  | BS  | "]" | "\" | "[" | "Z" | "Y" | "X" |
   ---------+-----+-----+-----+-----+-----+-----+-----+-----|
    6       |LEFT |ENTER|STOP | ESC |RGRAP|LGRAP|CTRL |SHIFT|
   ---------+-----+-----+-----+-----+-----+-----+-----+-----|
    7       |DOWN | INS | CLS | F5  | F4  | F3  | F2  | F1  |
   ---------+-----+-----+-----+-----+-----+-----+-----+-----|
    8       |RIGHT|     |PRINT| SEL |CAPS | DEL | TAB |SPACE|
   ---------+-----+-----+-----+-----+-----+-----+-----+-----|
    9*      | "7" | "6" | "5" | "4" | "3" | "2" | "1" | "0" |
   ---------+-----+-----+-----+-----+-----+-----+-----+-----|
   10*      | "," | "." | "/" | "*" | "-" | "+" | "9" | "8" |
   ----------------------------------------------------------
   * Numcerical keypad (SVI-328 only)
*/

INPUT_PORTS_START( svi318_keys )

 PORT_START /* 0 */
  PORT_BIT (0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("0 )") PORT_CODE(KEYCODE_0)
  PORT_BIT (0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("1 !") PORT_CODE(KEYCODE_1)
  PORT_BIT (0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("2 @") PORT_CODE(KEYCODE_2)
  PORT_BIT (0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("3 #") PORT_CODE(KEYCODE_3)
  PORT_BIT (0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("4 $") PORT_CODE(KEYCODE_4)
  PORT_BIT (0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("5 %") PORT_CODE(KEYCODE_5)
  PORT_BIT (0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("6 ^") PORT_CODE(KEYCODE_6)
  PORT_BIT (0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("7 &") PORT_CODE(KEYCODE_7)

 PORT_START /* 1 */
  PORT_BIT (0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("8 *") PORT_CODE(KEYCODE_8)
  PORT_BIT (0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("9 (") PORT_CODE(KEYCODE_9)
  PORT_BIT (0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME(": ;") PORT_CODE(KEYCODE_COLON)
  PORT_BIT (0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("' \"") PORT_CODE(KEYCODE_QUOTE)
  PORT_BIT (0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME(", <") PORT_CODE(KEYCODE_COMMA)
  PORT_BIT (0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("= +") PORT_CODE(KEYCODE_EQUALS)
  PORT_BIT (0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME(". >") PORT_CODE(KEYCODE_STOP)
  PORT_BIT (0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("/ ?") PORT_CODE(KEYCODE_SLASH)

 PORT_START /* 2 */
  PORT_BIT (0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("- _") PORT_CODE(KEYCODE_MINUS)
  PORT_BIT (0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("a A") PORT_CODE(KEYCODE_A)
  PORT_BIT (0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("b B") PORT_CODE(KEYCODE_B)
  PORT_BIT (0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("c C") PORT_CODE(KEYCODE_C)
  PORT_BIT (0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("d D") PORT_CODE(KEYCODE_D)
  PORT_BIT (0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("e E") PORT_CODE(KEYCODE_E)
  PORT_BIT (0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("f F") PORT_CODE(KEYCODE_F)
  PORT_BIT (0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("g G") PORT_CODE(KEYCODE_G)

 PORT_START /* 3 */
  PORT_BIT (0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("h H") PORT_CODE(KEYCODE_H)
  PORT_BIT (0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("i I") PORT_CODE(KEYCODE_I)
  PORT_BIT (0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("j J") PORT_CODE(KEYCODE_J)
  PORT_BIT (0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("k K") PORT_CODE(KEYCODE_K)
  PORT_BIT (0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("l L") PORT_CODE(KEYCODE_L)
  PORT_BIT (0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("m M") PORT_CODE(KEYCODE_M)
  PORT_BIT (0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("n N") PORT_CODE(KEYCODE_N)
  PORT_BIT (0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("o O") PORT_CODE(KEYCODE_O)

 PORT_START /* 4 */
  PORT_BIT (0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("p P") PORT_CODE(KEYCODE_P)
  PORT_BIT (0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("q Q") PORT_CODE(KEYCODE_Q)
  PORT_BIT (0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("r R") PORT_CODE(KEYCODE_R)
  PORT_BIT (0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("s S") PORT_CODE(KEYCODE_S)
  PORT_BIT (0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("t T") PORT_CODE(KEYCODE_T)
  PORT_BIT (0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("u U") PORT_CODE(KEYCODE_U)
  PORT_BIT (0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("v V") PORT_CODE(KEYCODE_V)
  PORT_BIT (0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("w W") PORT_CODE(KEYCODE_W)

 PORT_START /* 5 */
  PORT_BIT (0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("x X") PORT_CODE(KEYCODE_X)
  PORT_BIT (0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("y Y") PORT_CODE(KEYCODE_Y)
  PORT_BIT (0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("z Z") PORT_CODE(KEYCODE_Z)
  PORT_BIT (0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("[ {") PORT_CODE(KEYCODE_OPENBRACE)
  PORT_BIT (0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("\\ ~") PORT_CODE(KEYCODE_BACKSLASH)
  PORT_BIT (0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("] }") PORT_CODE(KEYCODE_CLOSEBRACE)
  PORT_BIT (0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("BACKSPACE") PORT_CODE(KEYCODE_BACKSPACE)
  PORT_BIT (0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("UP") PORT_CODE(KEYCODE_UP)

 PORT_START /* 6 */
  PORT_BIT (0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("LEFT SHIFT") PORT_CODE(KEYCODE_LSHIFT)
  PORT_BIT (0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("LEFT CTRL") PORT_CODE(KEYCODE_LCONTROL)
  PORT_BIT (0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("LEFT GRAPH") PORT_CODE(KEYCODE_LALT)
  PORT_BIT (0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("RIGHT GRAPH") PORT_CODE(KEYCODE_RALT)
  PORT_BIT (0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("ESC") PORT_CODE(KEYCODE_ESC)
  PORT_BIT (0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("STOP") PORT_CODE(KEYCODE_END)
  PORT_BIT (0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("ENTER") PORT_CODE(KEYCODE_ENTER)
  PORT_BIT (0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("LEFT") PORT_CODE(KEYCODE_LEFT)

 PORT_START /* 7 */
  PORT_BIT (0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("F1") PORT_CODE(KEYCODE_F1)
  PORT_BIT (0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("F2") PORT_CODE(KEYCODE_F2)
  PORT_BIT (0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("F3") PORT_CODE(KEYCODE_F3)
  PORT_BIT (0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("F4") PORT_CODE(KEYCODE_F4)
  PORT_BIT (0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("F5") PORT_CODE(KEYCODE_F5)
  PORT_BIT (0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("CLS") PORT_CODE(KEYCODE_HOME)
  PORT_BIT (0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("INS") PORT_CODE(KEYCODE_INSERT)
  PORT_BIT (0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("DOWN") PORT_CODE(KEYCODE_DOWN)

 PORT_START /* 8 */
  PORT_BIT (0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("SPACE") PORT_CODE(KEYCODE_SPACE)
  PORT_BIT (0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("TAB") PORT_CODE(KEYCODE_TAB)
  PORT_BIT (0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("DEL") PORT_CODE(KEYCODE_DEL)
  PORT_BIT (0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("CAPS") PORT_CODE(KEYCODE_CAPSLOCK)
  PORT_BIT (0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("SELECT") PORT_CODE(KEYCODE_PAUSE)
  PORT_BIT (0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("PRINT") PORT_CODE(KEYCODE_PRTSCR)
  PORT_BIT (0x40, IP_ACTIVE_LOW, IPT_UNUSED) 
  PORT_BIT (0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("RIGHT") PORT_CODE(KEYCODE_RIGHT)
INPUT_PORTS_END

INPUT_PORTS_START( svi318 )

 PORT_INCLUDE( svi318_keys )

 PORT_START /* 9 */
  PORT_BIT (0xff, IP_ACTIVE_LOW, IPT_UNUSED)

 PORT_START /* 10 */
  PORT_BIT (0xff, IP_ACTIVE_LOW, IPT_UNUSED)

 PORT_START /* 11 */
  PORT_BIT (0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP)
  PORT_BIT (0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN)
  PORT_BIT (0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT)
  PORT_BIT (0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT)
  PORT_BIT (0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_UP) PORT_PLAYER(2)
  PORT_BIT (0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN) PORT_PLAYER(2)
  PORT_BIT (0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT) PORT_PLAYER(2)
  PORT_BIT (0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT) PORT_PLAYER(2)

 PORT_START /* 12 */
  PORT_BIT (0x10, IP_ACTIVE_LOW, IPT_BUTTON1)
  PORT_BIT (0x20, IP_ACTIVE_LOW, IPT_BUTTON1) PORT_PLAYER(2)

 PORT_START /* 13 */
  PORT_DIPNAME( 0x20, 0x20, "Enforce 4 sprites/line")
  PORT_DIPSETTING( 0, DEF_STR( No ) )
  PORT_DIPSETTING( 0x20, DEF_STR( Yes ) )
  PORT_DIPNAME( 0x01, 0x00, "Bank 21 RAM")
  PORT_DIPSETTING( 0, DEF_STR( Off ) )
  PORT_DIPSETTING( 0x01, DEF_STR( On ) )
  PORT_DIPNAME( 0x02, 0x00, "Bank 22 RAM")
  PORT_DIPSETTING( 0, DEF_STR( Off ) )
  PORT_DIPSETTING( 0x02, DEF_STR( On ) )
  PORT_DIPNAME( 0x04, 0x00, "Bank 31 RAM")
  PORT_DIPSETTING( 0, DEF_STR( Off ) )
  PORT_DIPSETTING( 0x04, DEF_STR( On ) )
  PORT_DIPNAME( 0x08, 0x00, "Bank 32 RAM")
  PORT_DIPSETTING( 0, DEF_STR( Off ) )
  PORT_DIPSETTING( 0x08, DEF_STR( On ) )

 PORT_START /* 14 */
  PORT_BIT (0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("RIGHT SHIFT") PORT_CODE(KEYCODE_RSHIFT)
  PORT_BIT (0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("RIGHT CTRL") PORT_CODE(KEYCODE_RCONTROL)
  PORT_BIT (0x04, IP_ACTIVE_LOW, IPT_UNUSED)
  PORT_BIT (0x08, IP_ACTIVE_LOW, IPT_UNUSED)
  PORT_BIT (0x10, IP_ACTIVE_LOW, IPT_UNUSED)
  PORT_BIT (0x20, IP_ACTIVE_LOW, IPT_UNUSED)
  PORT_BIT (0x40, IP_ACTIVE_LOW, IPT_UNUSED)
  PORT_BIT (0x80, IP_ACTIVE_LOW, IPT_UNUSED)

INPUT_PORTS_END


INPUT_PORTS_START ( svi328 )

 PORT_INCLUDE(svi318_keys)

 PORT_START /* 9 */
  PORT_BIT (0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("NUM 0") PORT_CODE(KEYCODE_0_PAD)
  PORT_BIT (0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("NUM 1") PORT_CODE(KEYCODE_1_PAD)
  PORT_BIT (0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("NUM 2") PORT_CODE(KEYCODE_2_PAD)
  PORT_BIT (0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("NUM 3") PORT_CODE(KEYCODE_3_PAD)
  PORT_BIT (0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("NUM 4") PORT_CODE(KEYCODE_4_PAD)
  PORT_BIT (0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("NUM 5") PORT_CODE(KEYCODE_5_PAD)
  PORT_BIT (0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("NUM 6") PORT_CODE(KEYCODE_6_PAD)
  PORT_BIT (0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("NUM 7") PORT_CODE(KEYCODE_7_PAD)

 PORT_START /* 10 */
  PORT_BIT (0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("NUM 8") PORT_CODE(KEYCODE_8_PAD)
  PORT_BIT (0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("NUM 9") PORT_CODE(KEYCODE_9_PAD)
  PORT_BIT (0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("NUM +") PORT_CODE(KEYCODE_PLUS_PAD)
  PORT_BIT (0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("NUM -") PORT_CODE(KEYCODE_MINUS_PAD)
  PORT_BIT (0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("NUM *") PORT_CODE(KEYCODE_ASTERISK)
  PORT_BIT (0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("NUM /") PORT_CODE(KEYCODE_SLASH_PAD)
  PORT_BIT (0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("NUM .") PORT_CODE(KEYCODE_LALT)
  PORT_BIT (0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("NUM ,") PORT_CODE(KEYCODE_RALT)

 PORT_START /* 11 */
  PORT_BIT (0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP)
  PORT_BIT (0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN)
  PORT_BIT (0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT)
  PORT_BIT (0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT)
  PORT_BIT (0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_UP) PORT_PLAYER(2)
  PORT_BIT (0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN) PORT_PLAYER(2)
  PORT_BIT (0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT) PORT_PLAYER(2)
  PORT_BIT (0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT) PORT_PLAYER(2)

 PORT_START /* 12 */
  PORT_BIT (0x10, IP_ACTIVE_LOW, IPT_BUTTON1)
  PORT_BIT (0x20, IP_ACTIVE_LOW, IPT_BUTTON1) PORT_PLAYER(2)

 PORT_START /* 13 */
  PORT_DIPNAME( 0x20, 0x20, "Enforce 4 sprites/line")
  PORT_DIPSETTING( 0, DEF_STR( No ) )
  PORT_DIPSETTING( 0x20, DEF_STR( Yes ) )
  PORT_BIT (0x01, IP_ACTIVE_LOW, IPT_UNUSED)
  PORT_DIPNAME( 0x02, 0x00, "Bank 22 RAM")
  PORT_DIPSETTING( 0, DEF_STR( Off ) )
  PORT_DIPSETTING( 0x02, DEF_STR( On ) )
  PORT_DIPNAME( 0x04, 0x00, "Bank 31 RAM")
  PORT_DIPSETTING( 0, DEF_STR( Off ) )
  PORT_DIPSETTING( 0x04, DEF_STR( On ) )
  PORT_DIPNAME( 0x08, 0x00, "Bank 32 RAM")
  PORT_DIPSETTING( 0, DEF_STR( Off ) )
  PORT_DIPSETTING( 0x08, DEF_STR( On ) )

 PORT_START /* 14 */
  PORT_BIT (0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("RIGHT SHIFT") PORT_CODE(KEYCODE_RSHIFT)
  PORT_BIT (0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("RIGHT CTRL") PORT_CODE(KEYCODE_RCONTROL)
  PORT_BIT (0x04, IP_ACTIVE_LOW, IPT_UNUSED)
  PORT_BIT (0x08, IP_ACTIVE_LOW, IPT_UNUSED)
  PORT_BIT (0x10, IP_ACTIVE_LOW, IPT_UNUSED)
  PORT_BIT (0x20, IP_ACTIVE_LOW, IPT_UNUSED)
  PORT_BIT (0x40, IP_ACTIVE_LOW, IPT_UNUSED)
  PORT_BIT (0x80, IP_ACTIVE_LOW, IPT_UNUSED)

INPUT_PORTS_END

static struct AY8910interface ay8910_interface =
{
	svi318_psg_port_a_r,
	NULL,
	NULL,
	svi318_psg_port_b_w
};

static const TMS9928a_interface tms9928a_interface =
{
	TMS9929A,
	0x4000,
	svi318_vdp_interrupt
};

static MACHINE_DRIVER_START( svi318 )
	/* Basic machine hardware */
	MDRV_CPU_ADD_TAG( "main", Z80, 3579545 )	/* 3.579545 Mhz */
	MDRV_CPU_PROGRAM_MAP( svi318_mem, 0 )
	MDRV_CPU_IO_MAP( svi318_io, 0 )
	MDRV_CPU_VBLANK_INT( svi318_interrupt, 1 )
	MDRV_FRAMES_PER_SECOND(50)
	MDRV_VBLANK_DURATION(DEFAULT_REAL_60HZ_VBLANK_DURATION)
	MDRV_INTERLEAVE(1)

	MDRV_MACHINE_RESET( svi318 )

	/* Video hardware */
	MDRV_TMS9928A( &tms9928a_interface )

	/* Sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")
	MDRV_SOUND_ADD(DAC, 0)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.25)
	MDRV_SOUND_ADD(WAVE, 0)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.25)
	MDRV_SOUND_ADD(AY8910, 1789773)
	MDRV_SOUND_CONFIG(ay8910_interface)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.75)	
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( svi328b )
	MDRV_IMPORT_FROM( svi318 )

	MDRV_CPU_MODIFY("main")
	MDRV_CPU_IO_MAP( svi318_io2, 0 )
	MDRV_MACHINE_RESET( svi328b )

	/* video hardware */
	MDRV_SCREEN_SIZE(640, 400)
	MDRV_VISIBLE_AREA(0,640-1, 0, 400-1)
	MDRV_VIDEO_UPDATE( svi328b )
MACHINE_DRIVER_END

/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START (svi318)
    ROM_REGION (0x10000, REGION_CPU1,0)
    ROM_LOAD ("svi100.rom", 0x0000, 0x8000, CRC(98d48655) SHA1(07758272df475e5e06187aa3574241df1b14035b))
ROM_END

ROM_START (svi328)
    ROM_REGION (0x10000, REGION_CPU1,0)
    ROM_LOAD ("svi110.rom", 0x0000, 0x8000, CRC(709904e9) SHA1(7d8daf52f78371ca2c9443e04827c8e1f98c8f2c))
ROM_END

ROM_START (svi328a)
    ROM_REGION (0x10000, REGION_CPU1,0)
    ROM_LOAD ("svi111.rom", 0x0000, 0x8000, CRC(bc433df6) SHA1(10349ce675f6d6d47f0976e39cb7188eba858d89))
ROM_END

ROM_START (svi328b)
    ROM_REGION (0x10000, REGION_CPU1,0)
    ROM_LOAD ("svi111.rom", 0x0000, 0x8000, CRC(bc433df6) SHA1(10349ce675f6d6d47f0976e39cb7188eba858d89))
    ROM_REGION( 0x1000, REGION_GFX1, 0)
    ROM_LOAD ("svi806.rom", 0x0000, 0x1000, CRC(850bc232) SHA1(ed45cb0e9bd18a9d7bd74f87e620f016a7ae840f))
ROM_END

ROM_START (svi328c)
    ROM_REGION (0x10000, REGION_CPU1,0)
    ROM_LOAD ("svi111.rom", 0x0000, 0x8000, CRC(bc433df6) SHA1(10349ce675f6d6d47f0976e39cb7188eba858d89))
    ROM_REGION( 0x1000, REGION_GFX1, 0)
    ROM_LOAD ("svi806se.rom", 0x0000, 0x1000, CRC(daea8956) SHA1(3f16d5513ad35692488ae7d864f660e76c6e8ed3))
ROM_END

static void svi318_printer_getinfo(const device_class *devclass, UINT32 state, union devinfo *info)
{
	/* printer */
	switch(state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case DEVINFO_INT_COUNT:							info->i = 1; break;

		default:										printer_device_getinfo(devclass, state, info); break;
	}
}

static void svi318_cassette_getinfo(const device_class *devclass, UINT32 state, union devinfo *info)
{
	/* cassette */
	switch(state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case DEVINFO_INT_COUNT:							info->i = 1; break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case DEVINFO_PTR_CASSETTE_FORMATS:				info->p = (void *) svi_cassette_formats; break;

		default:										cassette_device_getinfo(devclass, state, info); break;
	}
}

static void svi318_cartslot_getinfo(const device_class *devclass, UINT32 state, union devinfo *info)
{
	/* cartslot */
	switch(state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case DEVINFO_INT_COUNT:							info->i = 1; break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case DEVINFO_PTR_LOAD:							info->load = device_load_svi318_cart; break;
		case DEVINFO_PTR_UNLOAD:						info->unload = device_unload_svi318_cart; break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case DEVINFO_STR_FILE_EXTENSIONS:				strcpy(info->s = device_temp_str(), "rom"); break;

		default:										cartslot_device_getinfo(devclass, state, info); break;
	}
}

static void svi318_floppy_getinfo(const device_class *devclass, UINT32 state, union devinfo *info)
{
	/* floppy */
	switch(state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case DEVINFO_INT_COUNT:							info->i = 2; break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case DEVINFO_PTR_LOAD:							info->load = device_load_svi318_floppy; break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case DEVINFO_STR_FILE_EXTENSIONS:				strcpy(info->s = device_temp_str(), "dsk"); break;

		default:										legacybasicdsk_device_getinfo(devclass, state, info); break;
	}
}

SYSTEM_CONFIG_START(svi318)
	CONFIG_DEVICE(svi318_printer_getinfo)
	CONFIG_DEVICE(svi318_cassette_getinfo)
	CONFIG_DEVICE(svi318_cartslot_getinfo)
	CONFIG_DEVICE(svi318_floppy_getinfo)
SYSTEM_CONFIG_END

/*   YEAR  NAME     PARENT  COMPAT  MACHINE  INPUT   INIT    CONFIG  COMPANY FULLNAME */
COMP(1983, svi318,       0,      0, svi318,  svi318, svi318, svi318, "Spectravideo", "SVI-318 (SV BASIC v1.0)" , 0)
COMP(1983, svi328,  svi318,      0, svi318,  svi328, svi318, svi318, "Spectravideo", "SVI-328 (SV BASIC v1.1)" , 0)
COMP(1983, svi328a, svi318,      0, svi318,  svi328, svi318, svi318, "Spectravideo", "SVI-328 (SV BASIC v1.11)" , 0)
COMP(1983, svi328b, svi318,      0, svi328b, svi328, svi318, svi318, "Spectravideo", "SVI-328 + 80 column card", GAME_NOT_WORKING  )
COMP(1983, svi328c, svi318,      0, svi328b, svi328, svi318, svi318, "Spectravideo", "SVI-328 + 80 column card (Swedish)", GAME_NOT_WORKING  )
