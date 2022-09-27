/***************************************************************************

                                Air Buster
                            (C) 1990  Kaneko

                    driver by Luca Elia (l.elia@tin.it)

CPU   : Z-80 x 3
SOUND : YM2203C     M6295
OSC.  : 12.000MHz   16.000MHz

                    Interesting routines (main cpu)
                    -------------------------------

fd-fe   address of int: 0x38    (must alternate? see e600/1)
ff-100  address of int: 0x16
66      print "sub cpu caused nmi" and die!

7       after tests

1497    print string following call: (char)*,0. program continues after 0.
        base addr = c300 + HL & 08ff, DE=xy , BC=dxdy
        +0<-(e61b)  +100<-D +200<-E +300<-char  +400<-(e61c)

1642    A<- buttons status (bits 0&1)

                    Interesting locations (main cpu)
                    --------------------------------

2907    table of routines (f150 index = 1-3f, copied to e612)
        e60e<-address of routine, f150<-0. e60e is used until f150!=0

    1:  2bf4    service mode                                next

    2:  2d33    3:  16bd        4:  2dcb        5:  2fcf
    6:  3262    7:  32b8

    8:  335d>   print gfx/color test page                   next
    9:  33c0>   handle the above page

    a:  29c6        b:  2a24        c:  16ce

    d:  3e7e>   *
    e:  3ec5>   print "Sub Cpu / Ram Error"; **
    f:  3e17>   print "Coin error"; **
    10: 3528>   print (c) notice, not shown                 next
    11: 3730>   show (c) notice, wait 0x100 calls           next

    12:     9658    13: 97c3        14: a9fa        15: aa6e
    16-19:  2985    1a: 9c2e        1b: 9ffa        1c: 29c6

    1d: 2988>   *

    1e: a2c4        1f: a31a        20: a32f        21: a344
    22: a359        23: a36e        24: a383        25: a398
    26: a3ad        27: a3c2        28: a3d7        29: a3f1
    2a: a40e        2b: a4e5        2c: a69d        2d: adb8
    2e: ade9        2f: 2b8b        30: a823

    31: 3d17>   print "warm up, wait few mins. secs left: 00"   next
    32: 3dc0>   pause (e624 counter).e626                       next

    33: 96b4        34: 97ad

    35-3f:  3e7e>   *

*   Print "Command Error [$xx]" where xx is last routine index (e612)
**  ld (0000h),A (??) ; loop

3cd7    hiscores table (0x40 bytes, copied to e160)
        Try entering TERU as your name :)

7fff    country code: 0 <-> Japan; else World

e615    rank:   0-easy  1-normal    2-hard  3-hardest
e624    sound code during sound test

-- Shared RAM --

f148<-  sound code (copied from e624)
f14a->  read on nmi routine. main cpu writes the value and writes to port 02
f150<-  index of table of routines at 2907

----------------





                    Interesting routines (sub cpu)
                    -------------------------------

491     copy palette data   d000<-f200(a0)  d200<-f300(a0)  d400<-f400(200)

61c     f150<-A     f151<-A (routine index of main cpu!)
        if dsw1-2 active, it does nothing (?!)

c8c     c000-c7ff<-0    c800-cfff<-0    f600<-f200(400)
1750    copies 10 lines of 20 bytes from 289e to f800

22cd    copy 0x100 bytes
22cf    copy 0x0FF bytes
238d    copy 0x0A0 bytes

                    Interesting locations (sub cpu)
                    --------------------------------

fd-fe   address of int: 0x36e   (same as 38)
ff-100  address of int: 0x4b0

-- Shared RAM --

f000    credits

f001/d<-IN 24 (Service)
f00e<-  bank
f002<-  dsw1 (cpl'd)
f003<-  dsw2 (cpl'd)
f004<-  IN 20 (cpl'd) (player 1)
f005<-  IN 22 (cpl'd) (player 2)
f006<-  start lives: dsw-2 & 0x30 index; values: 3,4,5,7        (5da table)
f007    current lives p1
f008    current lives p2

f009<-  coin/credit 1: dsw-1 & 0x30 index; values: 11,12,21,23  (5de table)
f00a<-  coin 1
f00b<-  coin/credit 2: dsw-1 & 0xc0 index; values: 11,12,21,23  (5e2 table)
f00c<-  coin 2

f00f    ?? outa (28h)
f010    written by sub cpu, bit 4 read by main cpu.
        bit 0   p1 playing
        bit 1   p2 playing

f014    index (1-f) of routine called during int 36e (table at c3f)
    1:  62b         2:  66a         3:  6ad         4:  79f
    5:  7e0         6:  81b         7:  8a7         8:  8e9
    9:  b02         a:  0           b:  0           c:  bfc
    d:  c0d         e:  a6f         f:  ac3

f015    index of the routine to call, usually the one selected by f014
        but sometimes written directly.

Scroll registers: ports 04/06/08/0a/0c, written in sequence (101f)
port 06 <- f100 + f140  x       port 04 <- f104 + f142  y
port 0a <- f120 + f140  x       port 08 <- f124 + f142  y
port 0c <- f14c = bit 0/1/2/3 = port 6/4/a/8 val < FF

f148->  sound code (from main cpu)
f14c    scroll regs high bits

----------------








                    Interesting routines (sound cpu)
                    -------------------------------

50a     6295
521     6295
a96     2203 reg<-B     val<-A

                    Interesting locations (sound cpu)
                    ---------------------------------

c715
c716    pending sound command
c760    rom bank


                                To Do
                                -----

- Is the sub cpu / sound cpu communication status port (0e) correct ?
- Main cpu: port  01 ?
- Sub  cpu: port 0x38 ? Plus it can probably cause a nmi to main cpu
- incomplete DSW's
- Spriteram low 0x300 bytes (priority?)

*/

/*
**
**              Main cpu data
**
*/

/*  Runs in IM 2    fd-fe   address of int: 0x38
                    ff-100  address of int: 0x16    */

/*
**
**              Sub cpu data
**
**
*/

/*  Runs in IM 2    fd-fe   address of int: 0x36e   (same as 0x38)
                    ff-100  address of int: 0x4b0   (only writes to port 38h)   */
/*
   Sub cpu and Sound cpu communicate bidirectionally:

       Sub   cpu writes to soundlatch,  reads from soundlatch2
       Sound cpu writes to soundlatch2, reads from soundlatch

   Each latch raises a flag when it's been written.
   The flag is cleared when the latch is read.

Code at 505: waits for bit 1 to go low, writes command, waits for bit
0 to go low, reads back value. Code at 3b2 waits bit 2 to go high
(called during int fd)

*/

#include "driver.h"
#include "cpu/z80/z80.h"
#include "sound/2203intf.h"
#include "sound/okim6295.h"

static UINT8 *devram;
static int soundlatch_status, soundlatch2_status;

extern UINT8 *airbustr_videoram2, *airbustr_colorram2;
extern int airbustr_clear_sprites;

extern WRITE8_HANDLER( airbustr_videoram_w );
extern WRITE8_HANDLER( airbustr_colorram_w );
extern WRITE8_HANDLER( airbustr_videoram2_w );
extern WRITE8_HANDLER( airbustr_colorram2_w );
extern WRITE8_HANDLER( airbustr_scrollregs_w );
extern VIDEO_START( airbustr );
extern VIDEO_UPDATE( airbustr );

/* Read/Write Handlers */

static READ8_HANDLER( devram_r )
{
	// There's an MCU here, possibly

	switch (offset)
	{
		/* Reading efe0 probably resets a watchdog mechanism
           that would reset the main cpu. We avoid this and patch
           the rom instead (main cpu has to be reset once at startup) */
		case 0xfe0:
			return 0/*watchdog_reset_r(0)*/;

		/* Reading a word at eff2 probably yelds the product
           of the words written to eff0 and eff2 */
		case 0xff2:
		case 0xff3:
		{
			int	x = (devram[0xff0] + devram[0xff1] * 256) *
					(devram[0xff2] + devram[0xff3] * 256);
			if (offset == 0xff2)	return (x & 0x00FF) >> 0;
			else				return (x & 0xFF00) >> 8;
		}	break;

		/* Reading eff4, F0 times must yield at most 80-1 consecutive
           equal values */
		case 0xff4:
			return rand();

		default:
			return devram[offset];
	}
}

static WRITE8_HANDLER( master_nmi_trigger_w )
{
	cpunum_set_input_line(1, INPUT_LINE_NMI, PULSE_LINE);
}

static void airbustr_bankswitch(int cpunum, int data)
{
	UINT8 *ROM = memory_region(REGION_CPU1 + cpunum);

	if ((data & 0x07) <  3)
		ROM = &ROM[0x4000 * (data & 0x07)];
	else
		ROM = &ROM[0x10000 + 0x4000 * ((data & 0x07) - 3)];

	memory_set_bankptr(cpunum + 1, ROM);
}

static WRITE8_HANDLER( master_bankswitch_w )
{
	airbustr_bankswitch(0, data);
}

static WRITE8_HANDLER( slave_bankswitch_w )
{
	airbustr_bankswitch(1, data);

	flip_screen_set(data & 0x10);

	airbustr_clear_sprites = data & 0x20;
}

static WRITE8_HANDLER( sound_bankswitch_w )
{
	airbustr_bankswitch(2, data);
}

static READ8_HANDLER( soundcommand_status_r )
{
	// bits: 2 <-> ?    1 <-> soundlatch full   0 <-> soundlatch2 empty
	return 4 + soundlatch_status * 2 + (1 - soundlatch2_status);
}

static READ8_HANDLER( soundcommand_r )
{
	soundlatch_status = 0;	// soundlatch has been read
	return soundlatch_r(0);
}

static READ8_HANDLER( soundcommand2_r )
{
	soundlatch2_status = 0;	// soundlatch2 has been read
	return soundlatch2_r(0);
}

static WRITE8_HANDLER( soundcommand_w )
{
	soundlatch_w(0, data);
	soundlatch_status = 1;	// soundlatch has been written
	cpunum_set_input_line(2, INPUT_LINE_NMI, PULSE_LINE);	// cause a nmi to sub cpu
}

static WRITE8_HANDLER( soundcommand2_w )
{
	soundlatch2_w(0, data);
	soundlatch2_status = 1;	// soundlatch2 has been written
}

static WRITE8_HANDLER( airbustr_paletteram_w )
{
	int r, g, b;
	int val;

	/*  ! byte 1 ! ! byte 0 !   */
	/*  xGGG GGRR   RRRB BBBB   */
	/*  x432 1043   2104 3210   */

	paletteram[offset] = data;
	val = (paletteram[offset | 1] << 8) | paletteram[offset & ~1];

	g = (val >> 10) & 0x1f;
	r = (val >>  5) & 0x1f;
	b = (val >>  0) & 0x1f;

	palette_set_color(offset/2, (r * 0xff) / 0x1f, (g * 0xff) / 0x1f, (b * 0xff) / 0x1f);
}

static WRITE8_HANDLER( airbustr_coin_counter_w )
{
	coin_counter_w(0, data & 1);
	coin_counter_w(1, data & 2);
	coin_lockout_w(0, ~data & 4);
	coin_lockout_w(1, ~data & 8);
}

/* Memory Maps */

static ADDRESS_MAP_START( master_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x7fff) AM_ROM
	AM_RANGE(0x8000, 0xbfff) AM_ROMBANK(1)
	AM_RANGE(0xc000, 0xcfff) AM_RAM AM_BASE(&spriteram)
	AM_RANGE(0xd000, 0xdfff) AM_RAM
	AM_RANGE(0xe000, 0xefff) AM_RAM AM_READ(devram_r) AM_BASE(&devram)
	AM_RANGE(0xf000, 0xffff) AM_RAM AM_SHARE(1)
ADDRESS_MAP_END

static ADDRESS_MAP_START( master_io_map, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_FLAGS( AMEF_ABITS(8) )
	AM_RANGE(0x00, 0x00) AM_WRITE(master_bankswitch_w)
	AM_RANGE(0x01, 0x01) AM_WRITENOP // ???
	AM_RANGE(0x02, 0x02) AM_WRITE(master_nmi_trigger_w)
ADDRESS_MAP_END

static ADDRESS_MAP_START( slave_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x7fff) AM_ROM
	AM_RANGE(0x8000, 0xbfff) AM_ROMBANK(2)
	AM_RANGE(0xc000, 0xc3ff) AM_RAM AM_WRITE(airbustr_videoram2_w) AM_BASE(&airbustr_videoram2)
	AM_RANGE(0xc400, 0xc7ff) AM_RAM AM_WRITE(airbustr_colorram2_w) AM_BASE(&airbustr_colorram2)
	AM_RANGE(0xc800, 0xcbff) AM_RAM AM_WRITE(airbustr_videoram_w) AM_BASE(&videoram)
	AM_RANGE(0xcc00, 0xcfff) AM_RAM AM_WRITE(airbustr_colorram_w) AM_BASE(&colorram)
	AM_RANGE(0xd000, 0xd5ff) AM_RAM AM_WRITE(airbustr_paletteram_w) AM_BASE(&paletteram)
	AM_RANGE(0xd600, 0xdfff) AM_RAM
	AM_RANGE(0xe000, 0xefff) AM_RAM
	AM_RANGE(0xf000, 0xffff) AM_RAM AM_SHARE(1)
ADDRESS_MAP_END

static ADDRESS_MAP_START( slave_io_map, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_FLAGS( AMEF_ABITS(8) )
	AM_RANGE(0x00, 0x00) AM_WRITE(slave_bankswitch_w)
	AM_RANGE(0x02, 0x02) AM_READWRITE(soundcommand2_r, soundcommand_w)
	AM_RANGE(0x04, 0x0c) AM_WRITE(airbustr_scrollregs_w)
	AM_RANGE(0x0e, 0x0e) AM_READ(soundcommand_status_r)
	AM_RANGE(0x20, 0x20) AM_READ(input_port_0_r)
	AM_RANGE(0x22, 0x22) AM_READ(input_port_1_r)
	AM_RANGE(0x24, 0x24) AM_READ(input_port_2_r)
	AM_RANGE(0x28, 0x28) AM_WRITE(airbustr_coin_counter_w)
	AM_RANGE(0x38, 0x38) AM_WRITENOP // ???
ADDRESS_MAP_END

static ADDRESS_MAP_START( sound_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x7fff) AM_ROM
	AM_RANGE(0x8000, 0xbfff) AM_ROMBANK(3)
	AM_RANGE(0xc000, 0xdfff) AM_RAM
ADDRESS_MAP_END

static ADDRESS_MAP_START( sound_io_map, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_FLAGS( AMEF_ABITS(8) )
	AM_RANGE(0x00, 0x00) AM_WRITE(sound_bankswitch_w)
	AM_RANGE(0x02, 0x02) AM_READWRITE(YM2203_status_port_0_r, YM2203_control_port_0_w)
	AM_RANGE(0x03, 0x03) AM_READWRITE(YM2203_read_port_0_r, YM2203_write_port_0_w)
	AM_RANGE(0x04, 0x04) AM_READWRITE(OKIM6295_status_0_r, OKIM6295_data_0_w)
	AM_RANGE(0x06, 0x06) AM_READWRITE(soundcommand_r, soundcommand2_w)
ADDRESS_MAP_END

/* Input Ports */

INPUT_PORTS_START( airbustr )
	PORT_START_TAG("IN0")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    ) PORT_8WAY
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  ) PORT_8WAY
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  ) PORT_8WAY
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START_TAG("IN1")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(2)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(2)
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START_TAG("IN2")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )		// used

	PORT_START_TAG("DSW1")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_SERVICE( 0x04, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0x08, 0x08, "Coin Mode" )			//   routine at 0x056d :
	PORT_DIPSETTING(    0x08, "Mode 1" )			//     11 21 12 16 (bit 3 active)
	PORT_DIPSETTING(    0x00, "Mode 2" )			//     11 21 13 14 (bit 3 not active)
	PORT_DIPNAME( 0x30, 0x30, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x20, DEF_STR( 2C_1C ) )	PORT_CONDITION("DSW1", 0x08, PORTCOND_NOTEQUALS, 0x00)
	PORT_DIPSETTING(    0x30, DEF_STR( 1C_1C ) )	PORT_CONDITION("DSW1", 0x08, PORTCOND_NOTEQUALS, 0x00)
	PORT_DIPSETTING(    0x10, DEF_STR( 1C_2C ) )	PORT_CONDITION("DSW1", 0x08, PORTCOND_NOTEQUALS, 0x00)
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_6C ) )	PORT_CONDITION("DSW1", 0x08, PORTCOND_NOTEQUALS, 0x00)
	PORT_DIPSETTING(    0x20, DEF_STR( 2C_1C ) )	PORT_CONDITION("DSW1", 0x08, PORTCOND_EQUALS, 0x00)
	PORT_DIPSETTING(    0x30, DEF_STR( 1C_1C ) )	PORT_CONDITION("DSW1", 0x08, PORTCOND_EQUALS, 0x00)
	PORT_DIPSETTING(    0x10, DEF_STR( 1C_3C ) )	PORT_CONDITION("DSW1", 0x08, PORTCOND_EQUALS, 0x00)
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_4C ) )	PORT_CONDITION("DSW1", 0x08, PORTCOND_EQUALS, 0x00)
	PORT_DIPNAME( 0xc0, 0xc0, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x80, DEF_STR( 2C_1C ) )	PORT_CONDITION("DSW1", 0x08, PORTCOND_NOTEQUALS, 0x00)
	PORT_DIPSETTING(    0xc0, DEF_STR( 1C_1C ) )	PORT_CONDITION("DSW1", 0x08, PORTCOND_NOTEQUALS, 0x00)
	PORT_DIPSETTING(    0x40, DEF_STR( 1C_2C ) )	PORT_CONDITION("DSW1", 0x08, PORTCOND_NOTEQUALS, 0x00)
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_6C ) )	PORT_CONDITION("DSW1", 0x08, PORTCOND_NOTEQUALS, 0x00)
	PORT_DIPSETTING(    0x80, DEF_STR( 2C_1C ) )	PORT_CONDITION("DSW1", 0x08, PORTCOND_EQUALS, 0x00)
	PORT_DIPSETTING(    0xc0, DEF_STR( 1C_1C ) )	PORT_CONDITION("DSW1", 0x08, PORTCOND_EQUALS, 0x00)
	PORT_DIPSETTING(    0x40, DEF_STR( 1C_3C ) )	PORT_CONDITION("DSW1", 0x08, PORTCOND_EQUALS, 0x00)
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_4C ) )	PORT_CONDITION("DSW1", 0x08, PORTCOND_EQUALS, 0x00)

	PORT_START_TAG("DSW2")
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Easy ) )
	PORT_DIPSETTING(    0x03, DEF_STR( Normal ) )
	PORT_DIPSETTING(    0x01, "Difficult" )
	PORT_DIPSETTING(    0x00, "Very Difficult" )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_DIPNAME( 0x08, 0x08, "Freeze" )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x30, 0x30, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x30, "3" )
	PORT_DIPSETTING(    0x20, "4" )
	PORT_DIPSETTING(    0x10, "5" )
	PORT_DIPSETTING(    0x00, "7" )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x40, DEF_STR( On ) )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNUSED )
INPUT_PORTS_END

INPUT_PORTS_START( airbustj )
	PORT_INCLUDE(airbustr)

	PORT_MODIFY("DSW1")
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_DIPNAME( 0x30, 0x30, DEF_STR( Coin_A ) )		// routine at 0x0546 : 11 12 21 23
	PORT_DIPSETTING(    0x10, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x30, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(    0x20, DEF_STR( 1C_2C ) )
	PORT_DIPNAME( 0xc0, 0xc0, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x40, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0xc0, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(    0x80, DEF_STR( 1C_2C ) )
INPUT_PORTS_END

/* Graphics Layout */

static const gfx_layout gfxlayout =
{
	16, 16,
	RGN_FRAC(1,1),
	4,
	{ 0, 1, 2, 3 },
	{ 0*4, 1*4, 2*4, 3*4, 4*4, 5*4, 6*4, 7*4,
	  0*4+32*8, 1*4+32*8, 2*4+32*8, 3*4+32*8, 4*4+32*8, 5*4+32*8, 6*4+32*8, 7*4+32*8 },
	{ 0*32, 1*32, 2*32, 3*32, 4*32, 5*32, 6*32, 7*32,
	  0*32+64*8, 1*32+64*8, 2*32+64*8, 3*32+64*8, 4*32+64*8, 5*32+64*8, 6*32+64*8, 7*32+64*8 },
	16*16*4
};

/* Graphics Decode Information */

static const gfx_decode gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &gfxlayout,   0, 32 }, // tiles
	{ REGION_GFX2, 0, &gfxlayout, 512, 16 }, // sprites
	{ -1 }
};

/* Sound Interfaces */

static struct YM2203interface ym2203_interface =
{
	input_port_3_r,		// DSW-1 connected to port A
	input_port_4_r		// DSW-2 connected to port B
};

/* Interrupt Generators */

static INTERRUPT_GEN( master_interrupt )
{
	static int addr = 0xff;

	addr ^= 0x02;
	cpunum_set_input_line_and_vector(0, 0, HOLD_LINE, addr);
}

static INTERRUPT_GEN( slave_interrupt )
{
	static int addr = 0xfd;

	addr ^= 0x02;
	cpunum_set_input_line_and_vector(1, 0, HOLD_LINE, addr);
}

/* Machine Initialization */

static MACHINE_RESET( airbustr )
{
	soundlatch_status = soundlatch2_status = 0;
	master_bankswitch_w(0, 0x02);
	slave_bankswitch_w(0, 0x02);
	sound_bankswitch_w(0, 0x02);
}

/* Machine Driver */

static MACHINE_DRIVER_START( airbustr )
	// basic machine hardware
	MDRV_CPU_ADD(Z80, 6000000)	// ???
	MDRV_CPU_PROGRAM_MAP(master_map, 0)
	MDRV_CPU_IO_MAP(master_io_map, 0)
	MDRV_CPU_VBLANK_INT(master_interrupt, 2)	// nmi caused by sub cpu?, ?

	MDRV_CPU_ADD(Z80, 6000000)	// ???
	MDRV_CPU_PROGRAM_MAP(slave_map, 0)
	MDRV_CPU_IO_MAP(slave_io_map, 0)
	MDRV_CPU_VBLANK_INT(slave_interrupt, 2)		// nmi caused by main cpu, ?

	MDRV_CPU_ADD(Z80, 6000000)	// ???
	MDRV_CPU_PROGRAM_MAP(sound_map, 0)
	MDRV_CPU_IO_MAP(sound_io_map, 0)
	MDRV_CPU_VBLANK_INT(irq0_line_hold, 1)		// nmi are caused by sub cpu writing a sound command

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)
	MDRV_INTERLEAVE(100)	// Palette RAM is filled by sub cpu with data supplied by main cpu
							// Maybe a high value is safer in order to avoid glitches
	MDRV_MACHINE_RESET(airbustr)

	// video hardware
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(32*8, 32*8)
	MDRV_VISIBLE_AREA(0, 32*8-1, 2*8, 30*8-1)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(768)

	MDRV_VIDEO_START(airbustr)
	MDRV_VIDEO_UPDATE(airbustr)

	// sound hardware
	MDRV_SPEAKER_STANDARD_MONO("mono")

	MDRV_SOUND_ADD(YM2203, 3000000)
	MDRV_SOUND_CONFIG(ym2203_interface)
	MDRV_SOUND_ROUTE(0, "mono", 0.25)
	MDRV_SOUND_ROUTE(1, "mono", 0.25)
	MDRV_SOUND_ROUTE(2, "mono", 0.25)
	MDRV_SOUND_ROUTE(3, "mono", 0.50)

	MDRV_SOUND_ADD(OKIM6295, 12000000/4/165)
	MDRV_SOUND_CONFIG(okim6295_interface_region_1)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.80)
MACHINE_DRIVER_END

/* ROMs */

ROM_START( airbustr )
	ROM_REGION( 0x24000, REGION_CPU1, 0 )
	ROM_LOAD( "pr12.h19",   0x00000, 0x0c000, CRC(91362eb2) SHA1(cd85acfa6542af68dd1cad46f9426a95cfc9432e) )
	ROM_CONTINUE(           0x10000, 0x14000 )

	ROM_REGION( 0x24000, REGION_CPU2, 0 )
	ROM_LOAD( "pr13.l15",   0x00000, 0x0c000, CRC(13b2257b) SHA1(325efa54e757a1f08caf81801930d61ea4e7b6d4) )
	ROM_CONTINUE(           0x10000, 0x14000 )

	ROM_REGION( 0x24000, REGION_CPU3, 0 )
	ROM_LOAD( "pr-21.bin",  0x00000, 0x0c000, CRC(6e0a5df0) SHA1(616b7c7aaf52a9a55b63c60717c1866940635cd4) )
	ROM_CONTINUE(           0x10000, 0x14000 )

	ROM_REGION( 0x80000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "pr-000.bin", 0x00000, 0x80000, CRC(8ca68f0d) SHA1(d60389e7e63e9850bcddecb486558de1414f1276) ) // scrolling layers

	ROM_REGION( 0x100000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "pr-001.bin", 0x00000, 0x80000, CRC(7e6cb377) SHA1(005290f9f53a0c3a6a9d04486b16b7fd52cc94b6) ) // sprites
	ROM_LOAD( "pr-02.bin",  0x80000, 0x10000, CRC(6bbd5e46) SHA1(26563737f3f91ee0a056d35ce42217bb57d8a081) )

	ROM_REGION( 0x40000, REGION_SOUND1, 0 )	/* OKI-M6295 samples */
	ROM_LOAD( "pr-200.bin", 0x00000, 0x40000, CRC(a4dd3390) SHA1(2d72b46b4979857f6b66489bebda9f48799f59cf) )
ROM_END

ROM_START( airbustj )
	ROM_REGION( 0x24000, REGION_CPU1, 0 )
	ROM_LOAD( "pr-14j.bin", 0x00000, 0x0c000, CRC(6b9805bd) SHA1(db6df33cf17316a4b81d7731dca9fe8bbf81f014) )
	ROM_CONTINUE(           0x10000, 0x14000 )

	ROM_REGION( 0x24000, REGION_CPU2, 0 )
	ROM_LOAD( "pr-11j.bin", 0x00000, 0x0c000, CRC(85464124) SHA1(8cce8dfdede48032c40d5f155fd58061866668de) )
	ROM_CONTINUE(           0x10000, 0x14000 )

	ROM_REGION( 0x24000, REGION_CPU3, 0 )
	ROM_LOAD( "pr-21.bin",  0x00000, 0x0c000, CRC(6e0a5df0) SHA1(616b7c7aaf52a9a55b63c60717c1866940635cd4) )
	ROM_CONTINUE(           0x10000, 0x14000 )

	ROM_REGION( 0x80000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "pr-000.bin", 0x00000, 0x80000, CRC(8ca68f0d) SHA1(d60389e7e63e9850bcddecb486558de1414f1276) ) // scrolling layers

	ROM_REGION( 0x100000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "pr-001.bin", 0x000000, 0x80000, CRC(7e6cb377) SHA1(005290f9f53a0c3a6a9d04486b16b7fd52cc94b6) ) // sprites
	ROM_LOAD( "pr-02.bin",  0x080000, 0x10000, CRC(6bbd5e46) SHA1(26563737f3f91ee0a056d35ce42217bb57d8a081) )

	ROM_REGION( 0x40000, REGION_SOUND1, 0 )	/* OKI-M6295 samples */
	ROM_LOAD( "pr-200.bin", 0x00000, 0x40000, CRC(a4dd3390) SHA1(2d72b46b4979857f6b66489bebda9f48799f59cf) )
ROM_END

/* Driver Initialization */

DRIVER_INIT( airbustr )
{
	int i;
	UINT8 *ROM = memory_region(REGION_GFX1);

	// One gfx rom seems to have scrambled data (bad read?): let's swap even and odd nibbles
	for (i = 0; i < 0x80000; i++)
	{
		ROM[i] = ((ROM[i] & 0xf0) >> 4) + ((ROM[i] & 0x0f) << 4);
	}

	// Startup check. We need a reset so I patch a busy loop with jp 0
	ROM = memory_region(REGION_CPU1);
	ROM[0x37e4] = 0x00;
	ROM[0x37e5] = 0x00;

	// Include EI in the busy loop. It's a hack to repair nested nmi troubles
	ROM = memory_region(REGION_CPU2);
	ROM[0x0258] = 0x53;
}

DRIVER_INIT( airbustj )
{
	int i;
	UINT8 *ROM = memory_region(REGION_GFX1);

	// One gfx rom seems to have scrambled data (bad read?): let's swap even and odd nibbles
	for (i = 0; i < 0x80000; i++)
	{
		ROM[i] = ((ROM[i] & 0xf0) >> 4) + ((ROM[i] & 0x0f) << 4);
	}

	// Startup check. We need a reset so I patch a busy loop with jp 0
	ROM = memory_region(REGION_CPU1);
	ROM[0x37f4] = 0x00;
	ROM[0x37f5] = 0x00;

	// Include EI in the busy loop. It's a hack to repair nested nmi troubles
	ROM = memory_region(REGION_CPU2);
	ROM[0x0258] = 0x53;
}

/* Game Drivers */

GAME( 1990, airbustr, 0,        airbustr, airbustr, airbustr, ROT0, "Kaneko (Namco license)", "Air Buster: Trouble Specialty Raid Unit (World)" , 0)	// 891220
GAME( 1990, airbustj, airbustr, airbustr, airbustj, airbustj, ROT0, "Kaneko (Namco license)", "Air Buster: Trouble Specialty Raid Unit (Japan)" , 0)	// 891229
