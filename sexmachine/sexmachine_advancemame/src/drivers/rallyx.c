/***************************************************************************

Rally X      (c) 1980 Namco
New Rally X  (c) 1981 Namco

and also

Jungler      (c) 1981 Konami
Tactician    (c) 1981 Konami
Loco-Motion  (c) 1982 Konami
Commando     (c) 1983 Sega

driver by Nicola Salmoria

There doesn't seem to be much doubt that Konami copied the video hardware of
Rally X.
The boards are surely very different; Rally X has video and sound split on the
two boards, while the Konami version has all video in one board and all sound
in the other.
Rally X uses a single Z80 and Namco sound hardware, while the others use the
standard Konami sound hardware of that era (slave Z80 + 2xAY-3-8910).
Also, the Konami design includes an optional starfield generator, only used
by Tactician.


Rally X Memory map:
------------------
Note: the memory map for the RAM 6x chips derived from the schematics doesn't
seem to be entirely correct. Here it's modified to match program behaviour.
The names might be assigned incorrectly.

Address          Dir Data     Name      Description
---------------- --- -------- --------- -----------------------
0-000xxxxxxxxxxx R   xxxxxxxx ROM 1B    program ROM \[1]
0-001xxxxxxxxxxx R   xxxxxxxx ROM 1C    program ROM /
0-010xxxxxxxxxxx R   xxxxxxxx ROM 1D    program ROM \[1]
0-011xxxxxxxxxxx R   xxxxxxxx ROM 1E    program ROM /
0-100xxxxxxxxxxx R   xxxxxxxx ROM 1H    program ROM \[1]
0-101xxxxxxxxxxx R   xxxxxxxx ROM 1J    program ROM /
0-110xxxxxxxxxxx R   xxxxxxxx ROM 1K    program ROM \[1]
0-111xxxxxxxxxxx R   xxxxxxxx ROM 1L    program ROM /
1-0000xxxxxxxxxx R/W xxxxxxxx RAM 6A/6C radar tilemap RAM + sprites
1-0001xxxxxxxxxx R/W xxxxxxxx RAM 6B/6D playfield tilemap RAM
1-0010xxxxxxxxxx R/W xxxxxxxx RAM 6J/6K radar tilemap RAM + sprites
1-0011xxxxxxxxxx R/W xxxxxxxx RAM 6H/6L playfield tilemap RAM
1-0110xxxxxxxxxx R/W xxxxxxxx RAM 6F/6M work RAM
1-0111xxxxxxxxxx R/W xxxxxxxx RAM 6E/6N work RAM
1-1----00---xxxx   W ----xxxx SODWR     bullets shape and X pos msb [2]
1-1----01-------   W --------           watchdog reset
1-1----10000xxxx   W ----xxxx RAM 2N    \ sound control registers
1-1----10001xxxx   W ----xxxx RAM 2P    /
1-1----10010----   W          n.c.
1-1----10011----   W xxxxxxxx POSIX     playfield X scroll
1-1----10100----   W xxxxxxxx POSIY     playfield Y scroll
1-1----10101----   W          n.c.
1-1----10110       W          WR2       ?
1-1----10111       W          WR3       ? this is written to A LOT of times every frame
1-1----11----000   W -------x BANG      explosion sound trigger
1-1----11----001   W -------x INT ON    interrupt enable
1-1----11----010   W -------x SOUND ON  sound enable [3]
1-1----11----011   W -------x FLIP      flip screen
1-1----11----100   W -------x           1 player start lamp
1-1----11----101   W -------x           2 players start lamp
1-1----11----110   W -------x           coin lockout
1-1----11----111   W -------x           coin counter
1-1----00------- R   xxxxxxxx           switch inputs
1-1----01------- R   xxxxxxxx           switch inputs
1-1----10------- R   xxxxxxxx           dip switches
1-1----11        R            RDSTB     ?

[1] either 2x2716 or 1x2732
[2] SO = Small Objects? Only locations 4-F are used.
[3] doesn't seem to work in New Rally X.

I/O ports:
OUT on port $0 sets the interrupt vector/instruction (the game uses both
IM 2 and IM 0)


Loco-Motion Memory map:
----------------------
Main CPU:

Address          Dir Data     Name      Description
---------------- --- -------- --------- -----------------------
0000xxxxxxxxxxxx R   xxxxxxxx ROM 3D    program ROM
0001xxxxxxxxxxxx R   xxxxxxxx ROM 3E    program ROM
0010xxxxxxxxxxxx R   xxxxxxxx ROM 3F    program ROM
0011xxxxxxxxxxxx R   xxxxxxxx ROM 3G    program ROM
0100xxxxxxxxxxxx R   xxxxxxxx ROM 3J    program ROM
0101xxxxxxxxxxxx R   xxxxxxxx ROM 3K    program ROM
0110xxxxxxxxxxxx R   xxxxxxxx ROM 3L    program ROM
0111xxxxxxxxxxxx R   xxxxxxxx ROM 3M    program ROM
1-000xxxxxxxxxxx R/W xxxxxxxx RAM 3C    tilemap RAM (tile code) [1]
1-001xxxxxxxxxxx R/W xxxxxxxx RAM 3B    tilemap RAM (tile attr) [1]
1-011xxxxxxxxxxx R/W xxxxxxxx RAM 3A    work RAM
1-1----00------- R   xxxxxxxx IN1       switch inputs
1-1----01------- R   xxxxxxxx IN2       switch inputs
1-1----10------- R   xxxxxxxx IN3       switch inputs / dip switches
1-1----11------- R   xxxxxxxx IN4       dip switches
1-1----00---xxxx   W ----xxxx SCARW     bullets shape and X pos msb
1-1----01-------   W --------           watchdog reset
1-1----10-------   W xxxxxxxx SOUNDDATA command for sound CPU
1-1----11----000   W -------x SOUNDON   trigger irq on sound CPU
1-1----11----001   W -------x INTST     irq enable/acknowledge
1-1----11----010   W -------x MUT       sound disable
1-1----11----011   W -------x FLIP      flip screen
1-1----11----100   W -------x OUT1      coin counter #1
1-1----11----101   W -------x OUT2      ?
1-1----11----110   W -------x OUT3      coin counter #2
1-1----11----111   W -------x STARSON   stars enable (optional)

[1] 1st half is "radar" + sprite registers, 2nd half is scrolling playfield


Sound CPU (standard Konami sound board):

Address          Dir Data     Name      Description
---------------- --- -------- --------- -----------------------
0000xxxxxxxxxxxx R   xxxxxxxx ROM 1B    program ROM
0001xxxxxxxxxxxx R   xxxxxxxx ROM 1C    program ROM
0010--xxxxxxxxxx R/W xxxxxxxx RAM 2B/2C work RAM
0011xxxxxx------   W -------- CKB       RC filters enable for 8910 #1 (data is in A6-A11)
0011------xxxxxx   W -------- CKB       RC filters enable for 8910 #2 (data is in A0-A5)
0100------------ R/W xxxxxxxx SEN1      AY-3-8910 #1 r/w [1]
0101------------   W xxxxxxxx SEN2      AY-3-8910 #1 ctrl
0110------------ R/W xxxxxxxx SEN3      AY-3-8910 #2 r/w
0111------------   W xxxxxxxx SEN4      AY-3-8910 #2 ctrl

[1] port A: command from main CPU; port B: timer


Notes:
- Easter egg (both Rally X and New Rally X):
  - enter service mode
  - keep B1 pressed and enter the following sequence:
    2xU 7xD 1xR 6xL
  (c) NAMCO LTD. 1980 will be added at the bottom of the screen.

- The Test Mode "dip switch" actually comes from the edge connector, but is mapped
  in memory in place of dip switch #8. Dip switch #8 is supposed to freeze the
  game and is entirely handled by hardware.

- PROM 7a controls the video shape. This is used to hide the rightmost 4 char
  columns in Locomotion and Commando, while showing them in Jungler and
  Tactician.

- The playfield scroll registers used in Rally X are present, but not useful
  (always 0) in Jungler and Tactician. They were removed in Locomotn and Commando.

- commsega has more sprites and more "bullets" than the other games.

- it seems that Jungler doesn't support high priority tiles. Maybe they
  disabled that feature because they needed more color combinations.

- there are also 1-pixel sprite and bullet placement differences from game to game.

- cottong is a bootleg of a very different version of locomotn, possibly a
  prototype.

- commsega:
  Due to a bug at 0x1259, bit 3 of DSW1 also affects the "Bonus Life" value:
     - when bit 3 is OFF, you get an extra life at 30000 points
     - when bit 3 is ON , you get an extra life at 50000 points
   At 0x0050 there is code to give infinite lives for player 1 when bit 3 of DSW0
   is ON. I can't tell however when it is supposed to be called.

TODO:
- commsega: the first time you kill a soldier, the music stops. When you die,
  the music restarts and won't stop a second time.

- rallyx: Three things in the schematics that I haven't been able to trace:
  WR2, WR3 and RDSTB. Only WR3 is actually used by the game.

- rallyx: emulate the explosion with discrete sound components. The schematics
  are available so it should be possible eventually.

- tactician: the bouncing bomb seems to show incorrect graphics when it's hit.

***************************************************************************/

#include "driver.h"
#include "sndhrdw/timeplt.h"
#include "sound/namco.h"
#include "sound/samples.h"


extern UINT8 *rallyx_videoram,*rallyx_radarattr;
WRITE8_HANDLER( rallyx_videoram_w );
WRITE8_HANDLER( rallyx_scrollx_w );
WRITE8_HANDLER( rallyx_scrolly_w );
WRITE8_HANDLER( tactcian_starson_w );
PALETTE_INIT( rallyx );
VIDEO_START( rallyx );
VIDEO_UPDATE( rallyx );
DRIVER_INIT( rallyx );
DRIVER_INIT( jungler );
DRIVER_INIT( tactcian );
DRIVER_INIT( locomotn );
DRIVER_INIT( commsega );


static WRITE8_HANDLER( rallyx_interrupt_vector_w )
{
	cpunum_set_input_line_vector(0, 0, data);
	cpunum_set_input_line(0, 0, CLEAR_LINE);
}



static WRITE8_HANDLER( rallyx_bang_w )
{
	static int last;

	if (data == 0 && last != 0)
		sample_start(0,0,0);

	last = data;
}


static WRITE8_HANDLER( rallyx_latch_w )
{
	int bit = data & 1;

	switch (offset)
	{
		case 0x00:	/* BANG */
			rallyx_bang_w(0,bit);
			break;

		case 0x01:	/* INT ON */
			cpu_interrupt_enable(0,bit);
			if (!bit)
				cpunum_set_input_line(0, 0, CLEAR_LINE);
			break;

		case 0x02:	/* SOUND ON */
			/* this doesn't work in New Rally X so I'm not supporting it */
//          pacman_sound_enable_w(0,bit);
			break;

		case 0x03:	/* FLIP */
			flip_screen_set(bit);
			break;

		case 0x04:
			set_led_status(0,bit);
			break;

		case 0x05:
			set_led_status(1,bit);
			break;

		case 0x06:
			coin_lockout_w(0,!bit);
			break;

		case 0x07:
			coin_counter_w(0,bit);
			break;
	}
}


static WRITE8_HANDLER( locomotn_latch_w )
{
	int bit = data & 1;

	switch (offset)
	{
		case 0x00:	/* SOUNDON */
			timeplt_sh_irqtrigger_w(0,bit);
			break;

		case 0x01:	/* INTST */
			cpu_interrupt_enable(0,bit);
			break;

		case 0x02:	/* MUT */
//          sound disable
			break;

		case 0x03:	/* FLIP */
			flip_screen_set(bit);
			break;

		case 0x04:	/* OUT1 */
			coin_counter_w(0,bit);
			break;

		case 0x05:	/* OUT2 */
			break;

		case 0x06:	/* OUT3 */
			coin_counter_w(1,bit);
			break;

		case 0x07:	/* STARSON */
			tactcian_starson_w(offset,bit);
			break;
	}
}



static ADDRESS_MAP_START( rallyx_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x3fff) AM_ROM
	AM_RANGE(0x8000, 0x8fff) AM_READWRITE(MRA8_RAM, rallyx_videoram_w) AM_BASE(&rallyx_videoram)
	AM_RANGE(0x9800, 0x9fff) AM_RAM
	AM_RANGE(0xa000, 0xa000) AM_READ(input_port_0_r)
	AM_RANGE(0xa080, 0xa080) AM_READ(input_port_1_r)
	AM_RANGE(0xa100, 0xa100) AM_READ(input_port_2_r)
	AM_RANGE(0xa000, 0xa00f) AM_WRITE(MWA8_RAM) AM_BASE(&rallyx_radarattr)
	AM_RANGE(0xa080, 0xa080) AM_WRITE(watchdog_reset_w)
	AM_RANGE(0xa100, 0xa11f) AM_WRITE(pacman_sound_w) AM_BASE(&namco_soundregs)
	AM_RANGE(0xa130, 0xa130) AM_WRITE(rallyx_scrollx_w)
	AM_RANGE(0xa140, 0xa140) AM_WRITE(rallyx_scrolly_w)
	AM_RANGE(0xa170, 0xa170) AM_WRITE(MWA8_NOP)			/* ? */
	AM_RANGE(0xa180, 0xa187) AM_WRITE(rallyx_latch_w)
ADDRESS_MAP_END

static ADDRESS_MAP_START( writeport, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_FLAGS( AMEF_ABITS(8) )
	AM_RANGE(0, 0) AM_WRITE(rallyx_interrupt_vector_w)
ADDRESS_MAP_END


static ADDRESS_MAP_START( locomotn_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x7fff) AM_ROM
	AM_RANGE(0x8000, 0x8fff) AM_READWRITE(MRA8_RAM, rallyx_videoram_w) AM_BASE(&rallyx_videoram)
	AM_RANGE(0x9800, 0x9fff) AM_RAM
	AM_RANGE(0xa000, 0xa000) AM_READ(input_port_0_r)
	AM_RANGE(0xa080, 0xa080) AM_READ(input_port_1_r)
	AM_RANGE(0xa100, 0xa100) AM_READ(input_port_2_r)
	AM_RANGE(0xa180, 0xa180) AM_READ(input_port_3_r)
	AM_RANGE(0xa000, 0xa00f) AM_MIRROR(0x00f0) AM_WRITE(MWA8_RAM) AM_BASE(&rallyx_radarattr)	// jungler writes to a03x
	AM_RANGE(0xa080, 0xa080) AM_WRITE(watchdog_reset_w)
	AM_RANGE(0xa100, 0xa100) AM_WRITE(soundlatch_w)
	AM_RANGE(0xa130, 0xa130) AM_WRITE(rallyx_scrollx_w)	/* only jungler and tactcian */
	AM_RANGE(0xa140, 0xa140) AM_WRITE(rallyx_scrolly_w)	/* only jungler and tactcian */
	AM_RANGE(0xa180, 0xa187) AM_WRITE(locomotn_latch_w)
ADDRESS_MAP_END



INPUT_PORTS_START( rallyx )
	PORT_START_TAG("IN0")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_4WAY
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_4WAY
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_4WAY
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_4WAY
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN1 )

	PORT_START_TAG("IN1")
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(	0x01, DEF_STR( Upright ) )
	PORT_DIPSETTING(	0x00, DEF_STR( Cocktail ) )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_COCKTAIL
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_4WAY PORT_COCKTAIL
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_4WAY PORT_COCKTAIL
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_4WAY PORT_COCKTAIL
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_4WAY PORT_COCKTAIL
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN2 )

	PORT_START_TAG("DSW0")
	PORT_DIPNAME( 0xc0, 0xc0, DEF_STR( Coinage ) )
	PORT_DIPSETTING(	0x40, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(	0xc0, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(	0x80, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(	0x00, DEF_STR( Free_Play ) )
	PORT_DIPNAME( 0x38, 0x08, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(	0x10, "1 Car, Medium" )
	PORT_DIPSETTING(	0x28, "1 Car, Hard" )
	PORT_DIPSETTING(	0x00, "2 Cars, Easy" )
	PORT_DIPSETTING(	0x18, "2 Cars, Medium" )
	PORT_DIPSETTING(	0x30, "2 Cars, Hard" )
	PORT_DIPSETTING(	0x08, "3 Cars, Easy" )
	PORT_DIPSETTING(	0x20, "3 Cars, Medium" )
	PORT_DIPSETTING(	0x38, "3 Cars, Hard" )
	PORT_DIPNAME( 0x06, 0x02, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(	0x02, "10000" )		PORT_CONDITION("DSW0",0x38,PORTCOND_EQUALS,0x10)
	PORT_DIPSETTING(	0x02, "10000" )		PORT_CONDITION("DSW0",0x38,PORTCOND_EQUALS,0x28)
	PORT_DIPSETTING(	0x02, "15000" )		PORT_CONDITION("DSW0",0x38,PORTCOND_EQUALS,0x00)
	PORT_DIPSETTING(	0x02, "15000" )		PORT_CONDITION("DSW0",0x38,PORTCOND_EQUALS,0x18)
	PORT_DIPSETTING(	0x02, "15000" )		PORT_CONDITION("DSW0",0x38,PORTCOND_EQUALS,0x30)
	PORT_DIPSETTING(	0x02, "20000" )		PORT_CONDITION("DSW0",0x38,PORTCOND_EQUALS,0x08)
	PORT_DIPSETTING(	0x02, "20000" )		PORT_CONDITION("DSW0",0x38,PORTCOND_EQUALS,0x20)
	PORT_DIPSETTING(	0x02, "20000" )		PORT_CONDITION("DSW0",0x38,PORTCOND_EQUALS,0x38)
	PORT_DIPSETTING(	0x04, "20000" )		PORT_CONDITION("DSW0",0x38,PORTCOND_EQUALS,0x10)
	PORT_DIPSETTING(	0x04, "20000" )		PORT_CONDITION("DSW0",0x38,PORTCOND_EQUALS,0x28)
	PORT_DIPSETTING(	0x04, "30000" )		PORT_CONDITION("DSW0",0x38,PORTCOND_EQUALS,0x00)
	PORT_DIPSETTING(	0x04, "30000" )		PORT_CONDITION("DSW0",0x38,PORTCOND_EQUALS,0x18)
	PORT_DIPSETTING(	0x04, "30000" )		PORT_CONDITION("DSW0",0x38,PORTCOND_EQUALS,0x30)
	PORT_DIPSETTING(	0x04, "40000" )		PORT_CONDITION("DSW0",0x38,PORTCOND_EQUALS,0x08)
	PORT_DIPSETTING(	0x04, "40000" )		PORT_CONDITION("DSW0",0x38,PORTCOND_EQUALS,0x20)
	PORT_DIPSETTING(	0x04, "40000" )		PORT_CONDITION("DSW0",0x38,PORTCOND_EQUALS,0x38)
	PORT_DIPSETTING(	0x06, "30000" )		PORT_CONDITION("DSW0",0x38,PORTCOND_EQUALS,0x10)
	PORT_DIPSETTING(	0x06, "30000" )		PORT_CONDITION("DSW0",0x38,PORTCOND_EQUALS,0x28)
	PORT_DIPSETTING(	0x06, "40000" )		PORT_CONDITION("DSW0",0x38,PORTCOND_EQUALS,0x00)
	PORT_DIPSETTING(	0x06, "40000" )		PORT_CONDITION("DSW0",0x38,PORTCOND_EQUALS,0x18)
	PORT_DIPSETTING(	0x06, "40000" )		PORT_CONDITION("DSW0",0x38,PORTCOND_EQUALS,0x30)
	PORT_DIPSETTING(	0x06, "60000" )		PORT_CONDITION("DSW0",0x38,PORTCOND_EQUALS,0x08)
	PORT_DIPSETTING(	0x06, "60000" )		PORT_CONDITION("DSW0",0x38,PORTCOND_EQUALS,0x20)
	PORT_DIPSETTING(	0x06, "60000" )		PORT_CONDITION("DSW0",0x38,PORTCOND_EQUALS,0x38)
	PORT_DIPSETTING(	0x00, DEF_STR( None ) )
	PORT_SERVICE( 0x01, IP_ACTIVE_LOW )
INPUT_PORTS_END

INPUT_PORTS_START( nrallyx )
	PORT_START_TAG("IN0")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_4WAY
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_4WAY
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_4WAY
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_4WAY
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN1 )

	PORT_START_TAG("IN1")
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(	0x01, DEF_STR( Upright ) )
	PORT_DIPSETTING(	0x00, DEF_STR( Cocktail ) )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_COCKTAIL
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_4WAY PORT_COCKTAIL
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_4WAY PORT_COCKTAIL
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_4WAY PORT_COCKTAIL
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_4WAY PORT_COCKTAIL
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN2 )

	PORT_START_TAG("DSW")
	PORT_DIPNAME( 0xc0, 0xc0, DEF_STR( Coinage ) )
	PORT_DIPSETTING(	0x40, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(	0xc0, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(	0x80, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(	0x00, DEF_STR( Free_Play ) )
	PORT_DIPNAME( 0x38, 0x00, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(	0x10, "1 Car, Medium" )
	PORT_DIPSETTING(	0x28, "1 Car, Hard" )
	PORT_DIPSETTING(	0x18, "2 Cars, Medium" )
	PORT_DIPSETTING(	0x30, "2 Cars, Hard" )
	PORT_DIPSETTING(	0x00, "3 Cars, Easy" )
	PORT_DIPSETTING(	0x20, "3 Cars, Medium" )
	PORT_DIPSETTING(	0x38, "3 Cars, Hard" )
	PORT_DIPSETTING(	0x08, "4 Cars, Easy" )
	/* TODO: the bonus score depends on the number of lives */
	PORT_DIPNAME( 0x06, 0x02, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(	0x02, "A" )
	PORT_DIPSETTING(	0x04, "B" )
	PORT_DIPSETTING(	0x06, "C" )
	PORT_DIPSETTING(	0x00, DEF_STR( None ) )
	PORT_SERVICE( 0x01, IP_ACTIVE_LOW )
INPUT_PORTS_END


INPUT_PORTS_START( jungler )
	PORT_START_TAG("IN0")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_4WAY PORT_COCKTAIL
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_4WAY
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_4WAY
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN1 )

	PORT_START_TAG("IN1")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_4WAY
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_4WAY PORT_COCKTAIL
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_COCKTAIL
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_4WAY PORT_COCKTAIL
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_4WAY PORT_COCKTAIL
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START1 )

	PORT_START_TAG("DSW0")      /* Sound board */
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_4WAY
	PORT_BIT( 0x7f, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START_TAG("DSW1")      /* CPU board */
	PORT_DIPNAME( 0x07, 0x07, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 4C_3C ) )
	PORT_DIPSETTING(    0x07, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x06, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x05, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 1C_4C ) )
	PORT_DIPNAME( 0x38, 0x38, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x18, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 4C_3C ) )
	PORT_DIPSETTING(    0x38, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x30, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x28, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x20, DEF_STR( 1C_4C ) )
	PORT_DIPNAME( 0x40, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0x80, 0x80, "Test (255 lives)" )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
INPUT_PORTS_END


INPUT_PORTS_START( locomotn )
	PORT_START_TAG("IN0")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_4WAY PORT_COCKTAIL
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_4WAY
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_4WAY
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN1 )

	PORT_START_TAG("IN1")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_4WAY
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_4WAY PORT_COCKTAIL
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_COCKTAIL
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_4WAY PORT_COCKTAIL
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_4WAY PORT_COCKTAIL
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START1 )

	PORT_START_TAG("DSW0")      /* Sound board */
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_4WAY
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_DIPNAME( 0x30, 0x30, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x30, "3" )
	PORT_DIPSETTING(    0x20, "4" )
	PORT_DIPSETTING(    0x10, "5" )
	PORT_DIPSETTING(    0x00, "255" )
	PORT_DIPNAME( 0x08, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, "Intermissions" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x02, DEF_STR( On ) )
	PORT_DIPNAME( 0x01, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START_TAG("DSW1")      /* CPU board */
	PORT_DIPNAME( 0x0f, 0x0f, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x0a, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 3C_2C ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 4C_3C ) )
	PORT_DIPSETTING(    0x0f, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x0c, DEF_STR( 3C_4C ) )
	PORT_DIPSETTING(    0x0e, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(    0x07, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x06, DEF_STR( 2C_5C ) )
	PORT_DIPSETTING(    0x0b, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x0d, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(    0x05, DEF_STR( 1C_6C ) )
	PORT_DIPSETTING(    0x09, DEF_STR( 1C_7C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Free_Play ) )
	PORT_DIPNAME( 0xf0, 0xf0, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x40, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0xa0, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x20, DEF_STR( 3C_2C ) )
	PORT_DIPSETTING(    0x80, DEF_STR( 4C_3C ) )
	PORT_DIPSETTING(    0xf0, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0xc0, DEF_STR( 3C_4C ) )
	PORT_DIPSETTING(    0xe0, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(    0x70, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x60, DEF_STR( 2C_5C ) )
	PORT_DIPSETTING(    0xb0, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x30, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0xd0, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(    0x50, DEF_STR( 1C_6C ) )
	PORT_DIPSETTING(    0x90, DEF_STR( 1C_7C ) )
	PORT_DIPSETTING(    0x00, "Disabled" )
INPUT_PORTS_END


INPUT_PORTS_START( tactcian )
	PORT_START_TAG("IN0")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_4WAY PORT_COCKTAIL
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_4WAY
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_4WAY
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN1 )

	PORT_START_TAG("IN1")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_4WAY
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_4WAY PORT_COCKTAIL
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_COCKTAIL
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_COCKTAIL
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_4WAY PORT_COCKTAIL
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_4WAY PORT_COCKTAIL
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START1 )

	PORT_START_TAG("DSW0")      /* Sound board */
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_4WAY
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_DIPNAME( 0x30, 0x00, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x00, "3" )
	PORT_DIPSETTING(    0x10, "4" )
	PORT_DIPSETTING(    0x20, "5" )
	PORT_DIPSETTING(    0x30, "255" )
	PORT_DIPNAME( 0x08, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0x06, 0x00, DEF_STR( Coinage ) )			// Mode 1
	PORT_DIPSETTING(    0x06, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 1C_2C ) )
	/*
    PORT_DIPNAME( 0x06, 0x00, DEF_STR( Coinage ) )          // Mode 2
    PORT_DIPSETTING(    0x02, DEF_STR( 2C_1C ) )
    PORT_DIPSETTING(    0x04, "A 2C/1C  B 1C/3C" )
    PORT_DIPSETTING(    0x00, DEF_STR( 1C_1C ) )
    PORT_DIPSETTING(    0x06, "A 1C/1C  B 1C/6C" )
    */
	PORT_DIPNAME( 0x01, 0x00, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x00, "10k, 80k then every 100k" )
	PORT_DIPSETTING(    0x01, "20k, 80k then every 100k" )

	PORT_START_TAG("DSW1")      /* CPU board */
	PORT_DIPNAME( 0x01, 0x00, "Coin Mode" )
	PORT_DIPSETTING(    0x00, "Mode 1" )
	PORT_DIPSETTING(    0x01, "Mode 2" )
	PORT_DIPNAME( 0x02, 0x00, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x02, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x00, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x04, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x00, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x08, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x00, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x10, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x00, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x20, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x00, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x40, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x00, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x80, DEF_STR( On ) )
INPUT_PORTS_END


INPUT_PORTS_START( commsega )
	PORT_START_TAG("IN0")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_4WAY PORT_COCKTAIL
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_4WAY
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_4WAY
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN1 )

	PORT_START_TAG("IN1")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_4WAY
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_4WAY PORT_COCKTAIL
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_COCKTAIL
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_COCKTAIL
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_4WAY PORT_COCKTAIL
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_4WAY PORT_COCKTAIL
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START1 )

	PORT_START_TAG("DSW0")      /* (sound board) */
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_4WAY
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unknown ) )			// "Infinite Lives" - See notes
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x07, 0x07, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x05, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x06, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x07, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Free_Play ) )

	PORT_START_TAG("DSW1")      /* (CPU board) */
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x03, "3" )
	PORT_DIPSETTING(    0x02, "4" )
	PORT_DIPSETTING(    0x01, "5" )
	PORT_DIPSETTING(    0x00, "6" )
	PORT_DIPNAME( 0x1c, 0x1c, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 4C_1C ) )			// Bonus Life : 50000 points
	PORT_DIPSETTING(    0x14, DEF_STR( 3C_1C ) )			// Bonus Life : 50000 points
	PORT_DIPSETTING(    0x18, DEF_STR( 2C_1C ) )			// Bonus Life : 30000 points
	PORT_DIPSETTING(    0x1c, DEF_STR( 1C_1C ) )			// Bonus Life : 30000 points
	PORT_DIPSETTING(    0x04, DEF_STR( 2C_3C ) )			// Bonus Life : 50000 points
	PORT_DIPSETTING(    0x0c, DEF_STR( 1C_2C ) )			// Bonus Life : 30000 points
	PORT_DIPSETTING(    0x08, DEF_STR( 1C_3C ) )			// Bonus Life : 30000 points
	PORT_DIPSETTING(    0x00, DEF_STR( Free_Play ) )		// Bonus Life : 50000 points
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Difficulty ) )		// Check code at 0x1fc5
	PORT_DIPSETTING(    0x40, DEF_STR( Easy ) )				// 16 flying enemies to kill
	PORT_DIPSETTING(    0x00, DEF_STR( Hard ) )				// 24 flying enemies to kill
	PORT_DIPNAME( 0x80, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Cocktail ) )
INPUT_PORTS_END



static const gfx_layout rallyx_charlayout =
{
	8,8,
	RGN_FRAC(1,1),
	2,
	{ 0, 4 },
	{ 8*8+0, 8*8+1, 8*8+2, 8*8+3, 0, 1, 2, 3 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	16*8
};

static const gfx_layout jungler_charlayout =
{
	8,8,
	RGN_FRAC(1,1),
	2,
	{ 4, 0 },
	{ 8*8+0, 8*8+1, 8*8+2, 8*8+3, 0, 1, 2, 3 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	16*8
};

static const gfx_layout rallyx_spritelayout =
{
	16,16,
	RGN_FRAC(1,1),
	2,
	{ 0, 4 },
	{ 8*8+0, 8*8+1, 8*8+2, 8*8+3, 16*8+0, 16*8+1, 16*8+2, 16*8+3,
			 24*8+0, 24*8+1, 24*8+2, 24*8+3, 0, 1, 2, 3 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8,
			32*8, 33*8, 34*8, 35*8, 36*8, 37*8, 38*8, 39*8 },
	64*8
};

static const gfx_layout jungler_spritelayout =
{
	16,16,
	RGN_FRAC(1,1),
	2,
	{ 4, 0 },
	{ 8*8, 8*8+1, 8*8+2, 8*8+3, 0, 1, 2, 3,
			24*8+0, 24*8+1, 24*8+2, 24*8+3, 16*8+0, 16*8+1, 16*8+2, 16*8+3 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8,
			32*8, 33*8, 34*8, 35*8, 36*8, 37*8, 38*8, 39*8 },
	64*8
};

static const gfx_layout dotlayout =
{
	4,4,
	8,
	2,
	{ 6, 7 },
	{ 0*8, 1*8, 2*8, 3*8 },
	{ 0*32, 1*32, 2*32, 3*32 },
	16*8
};

static const gfx_decode rallyx_gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &rallyx_charlayout,     0, 64 },
	{ REGION_GFX1, 0, &rallyx_spritelayout,   0, 64 },
	{ REGION_GFX2, 0, &dotlayout,	       64*4,  1 },
	{ -1 } /* end of array */
};

static const gfx_decode jungler_gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &jungler_charlayout,    0, 64 },
	{ REGION_GFX1, 0, &jungler_spritelayout,  0, 64 },
	{ REGION_GFX2, 0, &dotlayout,          64*4,  1 },
	{ -1 } /* end of array */
};


static struct namco_interface namco_interface =
{
	3,				/* number of voices */
	REGION_SOUND1	/* memory region */
};

static const char *rallyx_sample_names[] =
{
	"*rallyx",
	"bang.wav",
	0	/* end of array */
};

static struct Samplesinterface samples_interface =
{
	1,	/* 1 channel */
	rallyx_sample_names
};



static MACHINE_DRIVER_START( rallyx )

	/* basic machine hardware */
	MDRV_CPU_ADD(Z80, 18432000/6)	/* 3.072 MHz */
	MDRV_CPU_PROGRAM_MAP(rallyx_map,0)
	MDRV_CPU_IO_MAP(0,writeport)
	MDRV_CPU_VBLANK_INT(irq0_line_assert,1)

	MDRV_FRAMES_PER_SECOND(60.606060)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER | VIDEO_HAS_SHADOWS)
	MDRV_SCREEN_SIZE(36*8, 32*8)
	MDRV_VISIBLE_AREA(0*8, 36*8-1, 2*8, 30*8-1)
	MDRV_GFXDECODE(rallyx_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(32)
	MDRV_COLORTABLE_LENGTH(64*4+4)

	MDRV_PALETTE_INIT(rallyx)
	MDRV_VIDEO_START(rallyx)
	MDRV_VIDEO_UPDATE(rallyx)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")

	MDRV_SOUND_ADD(NAMCO, 18432000/6/32)
	MDRV_SOUND_CONFIG(namco_interface)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.0)

	MDRV_SOUND_ADD(SAMPLES, 0)
	MDRV_SOUND_CONFIG(samples_interface)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.80)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( tactcian )

	/* basic machine hardware */
	MDRV_CPU_ADD(Z80, 18432000/6)	/* 3.072 MHz */
	MDRV_CPU_PROGRAM_MAP(locomotn_map,0)
	MDRV_CPU_VBLANK_INT(nmi_line_pulse,1)

	MDRV_CPU_ADD(Z80, 14318180/8)	/* 1.789772727 MHz */
	/* audio CPU */
	MDRV_CPU_PROGRAM_MAP(locomotn_sound_readmem,locomotn_sound_writemem)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)	/* frames per second, vblank duration */

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER | VIDEO_HAS_SHADOWS)
	MDRV_SCREEN_SIZE(36*8, 32*8)
	MDRV_VISIBLE_AREA(0*8, 36*8-1, 2*8, 30*8-1)
	MDRV_GFXDECODE(jungler_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(32+64)
	MDRV_COLORTABLE_LENGTH(64*4+4)

	MDRV_PALETTE_INIT(rallyx)
	MDRV_VIDEO_START(rallyx)
	MDRV_VIDEO_UPDATE(rallyx)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")

	MDRV_SOUND_ADD(AY8910, 14318180/8)
	MDRV_SOUND_CONFIG(timeplt_ay8910_interface)
	MDRV_SOUND_ROUTE(0, "filter.0.0", 0.60)
	MDRV_SOUND_ROUTE(1, "filter.0.1", 0.60)
	MDRV_SOUND_ROUTE(2, "filter.0.2", 0.60)

	MDRV_SOUND_ADD(AY8910, 14318180/8)
	MDRV_SOUND_ROUTE(0, "filter.1.0", 0.60)
	MDRV_SOUND_ROUTE(1, "filter.1.1", 0.60)
	MDRV_SOUND_ROUTE(2, "filter.1.2", 0.60)

	MDRV_SOUND_ADD_TAG("filter.0.0", FILTER_RC, 0)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.0)
	MDRV_SOUND_ADD_TAG("filter.0.1", FILTER_RC, 0)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.0)
	MDRV_SOUND_ADD_TAG("filter.0.2", FILTER_RC, 0)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.0)

	MDRV_SOUND_ADD_TAG("filter.1.0", FILTER_RC, 0)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.0)
	MDRV_SOUND_ADD_TAG("filter.1.1", FILTER_RC, 0)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.0)
	MDRV_SOUND_ADD_TAG("filter.1.2", FILTER_RC, 0)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.0)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( locomotn )

	/* basic machine hardware */
	MDRV_IMPORT_FROM(tactcian)

	/* video hardware */
	MDRV_VISIBLE_AREA(0*8, 32*8-1, 2*8, 30*8-1)
MACHINE_DRIVER_END



/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START( rallyx )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )
	ROM_LOAD( "1b",           0x0000, 0x1000, CRC(5882700d) SHA1(b6029e9730f1694894fe8b729ac0ba8d6712dea9) )
	ROM_LOAD( "rallyxn.1e",   0x1000, 0x1000, CRC(ed1eba2b) SHA1(82d3a4b34b0ff5cfdb8ca7c18ad5c63d943b8484) )
	ROM_LOAD( "rallyxn.1h",   0x2000, 0x1000, CRC(4f98dd1c) SHA1(8a20fadcea76802d1c412ba62086abb846ad54a8) )
	ROM_LOAD( "rallyxn.1k",   0x3000, 0x1000, CRC(9aacccf0) SHA1(9b22079972c0f9970d62d62751db4783a87796d5) )

	ROM_REGION( 0x1000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "8e",           0x0000, 0x1000, CRC(277c1de5) SHA1(30bc57263e8dad870c501c76bce6f42d69ab9e00) )

	ROM_REGION( 0x0100, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "im5623.8m",    0x0000, 0x0100, CRC(3c16f62c) SHA1(7a3800be410e306cf85753b9953ffc5575afbcd6) )  /* dots */

	ROM_REGION( 0x0160, REGION_PROMS, 0 )
	ROM_LOAD( "m3-7603.11n",  0x0000, 0x0020, CRC(c7865434) SHA1(70c1c9610ba6f1ead77f347e7132958958bccb31) )  /* palette */
	ROM_LOAD( "im5623.8p",    0x0020, 0x0100, CRC(834d4fda) SHA1(617864d3df0917a513e8255ad8d96ae7a04da5a1) )  /* lookup table */
	ROM_LOAD( "r-2.4n",       0x0120, 0x0020, CRC(8f574815) SHA1(4f84162db9d58b64742c67dc689eb665b9862fb3) )  /* video layout (not used) */
	ROM_LOAD( "r-3.7k",       0x0140, 0x0020, CRC(b8861096) SHA1(26fad384ed7a1a1e0ba719b5578e2dbb09334a25) )  /* video timing (not used) */

	ROM_REGION( 0x0200, REGION_SOUND1, 0 ) /* sound proms */
	ROM_LOAD( "im5623.3p",    0x0000, 0x0100, CRC(4bad7017) SHA1(3e6da9d798f5e07fa18d6ce7d0b148be98c766d5) )
	ROM_LOAD( "im5623.2m",    0x0100, 0x0100, CRC(77245b66) SHA1(0c4d0bee858b97632411c440bea6948a74759746) )  /* timing - not used */
ROM_END

ROM_START( rallyxm )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )
	ROM_LOAD( "1b",           0x0000, 0x1000, CRC(5882700d) SHA1(b6029e9730f1694894fe8b729ac0ba8d6712dea9) )
	ROM_LOAD( "1e",           0x1000, 0x1000, CRC(786585ec) SHA1(8aa75f10d695f4b3483c4bf7030b733318fd3bf3) )
	ROM_LOAD( "1h",           0x2000, 0x1000, CRC(110d7dcd) SHA1(23e0855c2c9300f2068711d160fcdfaedd07832f) )
	ROM_LOAD( "1k",           0x3000, 0x1000, CRC(473ab447) SHA1(f0a37ccc48c97c53672f754ca2ac37dc0dc91a9f) )

	ROM_REGION( 0x1000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "8e",           0x0000, 0x1000, CRC(277c1de5) SHA1(30bc57263e8dad870c501c76bce6f42d69ab9e00) )

	ROM_REGION( 0x0100, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "im5623.8m",    0x0000, 0x0100, CRC(3c16f62c) SHA1(7a3800be410e306cf85753b9953ffc5575afbcd6) )  /* dots */

	ROM_REGION( 0x0160, REGION_PROMS, 0 )
	ROM_LOAD( "m3-7603.11n",  0x0000, 0x0020, CRC(c7865434) SHA1(70c1c9610ba6f1ead77f347e7132958958bccb31) )  /* palette */
	ROM_LOAD( "im5623.8p",    0x0020, 0x0100, CRC(834d4fda) SHA1(617864d3df0917a513e8255ad8d96ae7a04da5a1) )  /* lookup table */
	ROM_LOAD( "r-2.4n",       0x0120, 0x0020, CRC(8f574815) SHA1(4f84162db9d58b64742c67dc689eb665b9862fb3) )  /* video layout (not used) */
	ROM_LOAD( "r-3.7k",       0x0140, 0x0020, CRC(b8861096) SHA1(26fad384ed7a1a1e0ba719b5578e2dbb09334a25) )  /* video timing (not used) */

	ROM_REGION( 0x0200, REGION_SOUND1, 0 ) /* sound proms */
	ROM_LOAD( "im5623.3p",    0x0000, 0x0100, CRC(4bad7017) SHA1(3e6da9d798f5e07fa18d6ce7d0b148be98c766d5) )
	ROM_LOAD( "im5623.2m",    0x0100, 0x0100, CRC(77245b66) SHA1(0c4d0bee858b97632411c440bea6948a74759746) )  /* timing - not used */
ROM_END

ROM_START( nrallyx )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )
	ROM_LOAD( "nrallyx.1b",   0x0000, 0x1000, CRC(9404c8d6) SHA1(ee7e45c22a2fbf72d3ac5ac26ab1111a22623fc5) )
	ROM_LOAD( "nrallyx.1e",   0x1000, 0x1000, CRC(ac01bf3f) SHA1(8e1a7cce92ef709d18727db6ee7f89936f4b8df8) )
	ROM_LOAD( "nrallyx.1h",   0x2000, 0x1000, CRC(aeba29b5) SHA1(2a6e4568729b83c430bf70e43c4146ad6a556b1b) )
	ROM_LOAD( "nrallyx.1k",   0x3000, 0x1000, CRC(78f17da7) SHA1(1e035746a10f91e898166a58093d45bdb158ae47) )

	ROM_REGION( 0x1000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "nrallyx.8e",   0x0000, 0x1000, CRC(ca7a174a) SHA1(dc553df18c45ba399661122be75b71d6cb54d6a2) )

	ROM_REGION( 0x0100, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "im5623.8m",    0x0000, 0x0100, CRC(3c16f62c) SHA1(7a3800be410e306cf85753b9953ffc5575afbcd6) )  /* dots */

	ROM_REGION( 0x0160, REGION_PROMS, 0 )
	ROM_LOAD( "nrallyx.pr1",  0x0000, 0x0020, CRC(a0a49017) SHA1(494c920a157e9f876d533c1b0146275a366c4989) )  /* palette */
	ROM_LOAD( "nrallyx.pr2",  0x0020, 0x0100, CRC(b2b7ca15) SHA1(e604d58f2f20ebf042f28b01e74eddeacf5baba9) )  /* lookup table */
	ROM_LOAD( "r-2.4n",       0x0120, 0x0020, CRC(8f574815) SHA1(4f84162db9d58b64742c67dc689eb665b9862fb3) )  /* video layout (not used) */
	ROM_LOAD( "r-3.7k",       0x0140, 0x0020, CRC(b8861096) SHA1(26fad384ed7a1a1e0ba719b5578e2dbb09334a25) )  /* video timing (not used) */

	ROM_REGION( 0x0200, REGION_SOUND1, 0 ) /* sound proms */
	ROM_LOAD( "nrallyx.spr",  0x0000, 0x0100, CRC(b75c4e87) SHA1(450f79a5ae09e34f7624d37769815baf93c0028e) )
	ROM_LOAD( "im5623.2m",    0x0100, 0x0100, CRC(77245b66) SHA1(0c4d0bee858b97632411c440bea6948a74759746) )  /* timing - not used */
ROM_END


ROM_START( jungler )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )
	ROM_LOAD( "jungr1",       0x0000, 0x1000, CRC(5bd6ad15) SHA1(608de86e19c6726bb7d21e7dc0e936f00121a3f4) )
	ROM_LOAD( "jungr2",       0x1000, 0x1000, CRC(dc99f1e3) SHA1(942405f6c7d816139e36289126fe883a6a9a0a08) )
	ROM_LOAD( "jungr3",       0x2000, 0x1000, CRC(3dcc03da) SHA1(2c328a46511c4c9eec6515b9316a586de6503152) )
	ROM_LOAD( "jungr4",       0x3000, 0x1000, CRC(f92e9940) SHA1(d72a4d0a0ab7c9a1dcbb7925eb8530052640a234) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )
	ROM_LOAD( "1b",           0x0000, 0x1000, CRC(f86999c3) SHA1(4660bd7826219b1bad7d9178918823196d4fd8d6) )

	ROM_REGION( 0x1000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "5k",           0x0000, 0x0800, CRC(924262bf) SHA1(593f59630b3bd369aef0819992106b4e6e6a241f) )
	ROM_LOAD( "5m",           0x0800, 0x0800, CRC(131a08ac) SHA1(167a0710a2a153f7f7c6839d2340e5aa725ef039) )

	ROM_REGION( 0x0100, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "82s129.10g",   0x0000, 0x0100, CRC(c59c51b7) SHA1(e8ac60fed9ba16c61a4c3c09e27f8c3f4e254014) ) /* dots */

	ROM_REGION( 0x0160, REGION_PROMS, 0 )
	ROM_LOAD( "18s030.8b",    0x0000, 0x0020, CRC(55a7e6d1) SHA1(f9e4ff3b165235db2fd8dab94c43bc686c3ad29b) ) /* palette */
	ROM_LOAD( "tbp24s10.9d",  0x0020, 0x0100, CRC(d223f7b8) SHA1(87b62f09d4eda09c16d99d1554017d18e52b5886) ) /* loookup table */
	ROM_LOAD( "18s030.7a",    0x0120, 0x0020, CRC(8f574815) SHA1(4f84162db9d58b64742c67dc689eb665b9862fb3) ) /* video layout (not used) */
	ROM_LOAD( "6331-1.10a",   0x0140, 0x0020, CRC(b8861096) SHA1(26fad384ed7a1a1e0ba719b5578e2dbb09334a25) ) /* video timing (not used) */
ROM_END

ROM_START( junglers )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )
	ROM_LOAD( "5c",           0x0000, 0x1000, CRC(edd71b28) SHA1(6bdd85bc1c24ca57573252fd636e05759164de8a) )
	ROM_LOAD( "5a",           0x1000, 0x1000, CRC(61ea4d46) SHA1(575ffe9fc7d5777c8f2d2b449623c353f42a4249) )
	ROM_LOAD( "4d",           0x2000, 0x1000, CRC(557c7925) SHA1(84d8eb2fdb7ee9098805be9f457a37f51e4bc3b8) )
	ROM_LOAD( "4c",           0x3000, 0x1000, CRC(51aac9a5) SHA1(2c8a24b4ce8cec96c6e09332f3f63bd7d25ae4c6) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )
	ROM_LOAD( "1b",           0x0000, 0x1000, CRC(f86999c3) SHA1(4660bd7826219b1bad7d9178918823196d4fd8d6) )

	ROM_REGION( 0x1000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "5k",           0x0000, 0x0800, CRC(924262bf) SHA1(593f59630b3bd369aef0819992106b4e6e6a241f) )
	ROM_LOAD( "5m",           0x0800, 0x0800, CRC(131a08ac) SHA1(167a0710a2a153f7f7c6839d2340e5aa725ef039) )

	ROM_REGION( 0x0100, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "82s129.10g",   0x0000, 0x0100, CRC(c59c51b7) SHA1(e8ac60fed9ba16c61a4c3c09e27f8c3f4e254014) ) /* dots */

	ROM_REGION( 0x0160, REGION_PROMS, 0 )
	ROM_LOAD( "18s030.8b",    0x0000, 0x0020, CRC(55a7e6d1) SHA1(f9e4ff3b165235db2fd8dab94c43bc686c3ad29b) ) /* palette */
	ROM_LOAD( "tbp24s10.9d",  0x0020, 0x0100, CRC(d223f7b8) SHA1(87b62f09d4eda09c16d99d1554017d18e52b5886) ) /* loookup table */
	ROM_LOAD( "18s030.7a",    0x0120, 0x0020, CRC(8f574815) SHA1(4f84162db9d58b64742c67dc689eb665b9862fb3) ) /* video layout (not used) */
	ROM_LOAD( "6331-1.10a",   0x0140, 0x0020, CRC(b8861096) SHA1(26fad384ed7a1a1e0ba719b5578e2dbb09334a25) ) /* video timing (not used) */
ROM_END

ROM_START( tactcian )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )
	ROM_LOAD( "tacticia.001", 0x0000, 0x1000, CRC(99163e39) SHA1(0a863f358a0bb065a9e2c41fcf4c20d370001dfe) )
	ROM_LOAD( "tacticia.002", 0x1000, 0x1000, CRC(6d3e8a69) SHA1(2b4b3f2b7401064540f59070ef6742d1f44ca839) )
	ROM_LOAD( "tacticia.003", 0x2000, 0x1000, CRC(0f71d0fa) SHA1(cb55243853b8b33034af7a6438f9a7c85a774d71) )
	ROM_LOAD( "tacticia.004", 0x3000, 0x1000, CRC(5e15f3b3) SHA1(01979f64b281a958f0a4effe2be21bf0e0a812bf) )
	ROM_LOAD( "tacticia.005", 0x4000, 0x1000, CRC(76456106) SHA1(580428f3c8cf442ee5c0f56db973644229aa8093) )
	ROM_LOAD( "tacticia.006", 0x5000, 0x1000, CRC(b33ca9ea) SHA1(0299c1cb9a3c6368bbbacb60c6f5c6854035a7bf) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )
	ROM_LOAD( "tacticia.s2",  0x0000, 0x1000, CRC(97d145a7) SHA1(7aee9004287590a25e153d45b95dfaac89fbe996) )
	ROM_LOAD( "tacticia.s1",  0x1000, 0x1000, CRC(067f781b) SHA1(640bc7813c239e497644e53a080d81366fcd04df) )

	ROM_REGION( 0x2000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "tacticia.c1",  0x0000, 0x1000, CRC(5d3ee965) SHA1(654c033291f3d139fb94f7aacbc2d1917856deb6) )
	ROM_LOAD( "tacticia.c2",  0x1000, 0x1000, CRC(e8c59c4f) SHA1(e4881f2e2e08bb8af37cc679c4e2367528ac4804) )

	ROM_REGION( 0x0100, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "tact6301.004", 0x0000, 0x0100, CRC(88b0b511) SHA1(785eded1ba761cdb59db579eb8a786516ff58152) ) /* dots */	// tac.a7

	ROM_REGION( 0x0160, REGION_PROMS, 0 )
	ROM_LOAD( "tact6331.002", 0x0000, 0x0020, CRC(b7ef83b7) SHA1(5ffab25c2dc5be0856a43a93711d39c4aec6660b) ) /* palette */
	ROM_LOAD( "tact6301.003", 0x0020, 0x0100, CRC(a92796f2) SHA1(0faab2dc0f868f4023a34ecfcf972d1c86a224a0) ) /* loookup table */	// tac.b4
	ROM_LOAD( "tact6331.001", 0x0120, 0x0020, CRC(8f574815) SHA1(4f84162db9d58b64742c67dc689eb665b9862fb3) ) /* video layout (not used) */
//  ROM_LOAD( "10a.bpr",      0x0140, 0x0020, CRC(b8861096) SHA1(26fad384ed7a1a1e0ba719b5578e2dbb09334a25) ) /* video timing (not used) */
ROM_END

ROM_START( tactcan2 )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )
	ROM_LOAD( "tan1",         0x0000, 0x1000, CRC(ddf38b75) SHA1(bad66fd6ae0ab3b91989fca14a8696ed855dc852) )
	ROM_LOAD( "tan2",         0x1000, 0x1000, CRC(f065ee2e) SHA1(f2362c471981af3348465f3c8a5ffb38058432a5) )
	ROM_LOAD( "tan3",         0x2000, 0x1000, CRC(2dba64fe) SHA1(8d312a6db99d2248fef2bbc590ceba333b0fde8b) )
	ROM_LOAD( "tan4",         0x3000, 0x1000, CRC(2ba07847) SHA1(3cd7cd0621ed930cb5955fc2ffe3239f6e176321) )
	ROM_LOAD( "tan5",         0x4000, 0x1000, CRC(1dae4c61) SHA1(70283b8412b0725f1c2acc281625c582a4fae39d) )
	ROM_LOAD( "tan6",         0x5000, 0x1000, CRC(2b36a18d) SHA1(bea8f36ec98975438ab267509bd9d1d1eb605945) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )
	/* sound ROMs were missing - using the ones from the other set */
	ROM_LOAD( "tacticia.s2",  0x0000, 0x1000, CRC(97d145a7) SHA1(7aee9004287590a25e153d45b95dfaac89fbe996) )
	ROM_LOAD( "tacticia.s1",  0x1000, 0x1000, CRC(067f781b) SHA1(640bc7813c239e497644e53a080d81366fcd04df) )

	ROM_REGION( 0x2000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "c1",           0x0000, 0x1000, CRC(5399471f) SHA1(66aea0df982ccbd6caaa24c258b2ba364bc1ecfd) )
	ROM_LOAD( "c2",           0x1000, 0x1000, CRC(8e8861e8) SHA1(38728418b09df06356c1e45a26cf438b93517ce5) )

	ROM_REGION( 0x0100, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "tact6301.004", 0x0000, 0x0100, CRC(88b0b511) SHA1(785eded1ba761cdb59db579eb8a786516ff58152) ) /* dots */	// tac.a7

	ROM_REGION( 0x0160, REGION_PROMS, 0 )
	ROM_LOAD( "tact6331.002", 0x0000, 0x0020, CRC(b7ef83b7) SHA1(5ffab25c2dc5be0856a43a93711d39c4aec6660b) ) /* palette */
	ROM_LOAD( "tact6301.003", 0x0020, 0x0100, CRC(a92796f2) SHA1(0faab2dc0f868f4023a34ecfcf972d1c86a224a0) ) /* loookup table */	// tac.b4
	ROM_LOAD( "tact6331.001", 0x0120, 0x0020, CRC(8f574815) SHA1(4f84162db9d58b64742c67dc689eb665b9862fb3) ) /* video layout (not used) */
//  ROM_LOAD( "10a.bpr",      0x0140, 0x0020, CRC(b8861096) SHA1(26fad384ed7a1a1e0ba719b5578e2dbb09334a25) ) /* video timing (not used) */
ROM_END

ROM_START( locomotn )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )
	ROM_LOAD( "1a.cpu",       0x0000, 0x1000, CRC(b43e689a) SHA1(7f1a0fa1ea9ff95a9d51b23ea00792ba22024282) )
	ROM_LOAD( "2a.cpu",       0x1000, 0x1000, CRC(529c823d) SHA1(714ae0af254646eb6ebc5f47422246832e89ccfb) )
	ROM_LOAD( "3.cpu",        0x2000, 0x1000, CRC(c9dbfbd1) SHA1(10ec7403053ef52d0ce4aa6eab3e82a3ea5e57ff) )
	ROM_LOAD( "4.cpu",        0x3000, 0x1000, CRC(caf6431c) SHA1(f013d8846fad9f64367b69febeb7512029a639c0) )
	ROM_LOAD( "5.cpu",        0x4000, 0x1000, CRC(64cf8dd6) SHA1(8fa1b5c4a7f136cb74833425a565fa558eeee083) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )
	ROM_LOAD( "1b_s1.bin",    0x0000, 0x1000, CRC(a1105714) SHA1(6e2e264748ab90bc5e8e8167f17ff91677ef6ae7) )

	ROM_REGION( 0x2000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "5l_c1.bin",    0x0000, 0x1000, CRC(5732eda9) SHA1(451de30946a9c8198c5ec83cc5c50e3ac2f9f56b) )
	ROM_LOAD( "c2.cpu",       0x1000, 0x1000, CRC(c3035300) SHA1(ddb1d658a28b973b60e2ce72fd7662537e147860) )

	ROM_REGION( 0x0100, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "10g.bpr",      0x0000, 0x0100, CRC(2ef89356) SHA1(5ed33386bab5d583358709c92f21ad9ad1a1bce9) ) /* dots */

	ROM_REGION( 0x0160, REGION_PROMS, 0 )
	ROM_LOAD( "8b.bpr",       0x0000, 0x0020, CRC(75b05da0) SHA1(aee98f5389e42332f30a6882ee22ff23f37e0573) ) /* palette */
	ROM_LOAD( "9d.bpr",       0x0020, 0x0100, CRC(aa6cf063) SHA1(08c1c9ab03eb168954b0170d40e95eed81022acd) ) /* loookup table */
	ROM_LOAD( "7a.bpr",       0x0120, 0x0020, CRC(48c8f094) SHA1(61592209720fddc8991751edf08b6950388af42e) ) /* video layout (not used) */
	ROM_LOAD( "10a.bpr",      0x0140, 0x0020, CRC(b8861096) SHA1(26fad384ed7a1a1e0ba719b5578e2dbb09334a25) ) /* video timing (not used) */
ROM_END

ROM_START( gutangtn )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )
	ROM_LOAD( "3d_1.bin",     0x0000, 0x1000, CRC(e9757395) SHA1(78e2f8988ed39d2ecfe1f874be370f603d5eecc1) )
	ROM_LOAD( "3e_2.bin",     0x1000, 0x1000, CRC(11d21d2e) SHA1(fd17dd481bb7bb39234fa7e9946b1cb4fa18109e) )
	ROM_LOAD( "3f_3.bin",     0x2000, 0x1000, CRC(4d80f895) SHA1(7d83f4ee34226636012a84f46af01991a28b96f6) )
	ROM_LOAD( "3h_4.bin",     0x3000, 0x1000, CRC(aa258ddf) SHA1(0f01ac0d72d8bb5a55c91a6fba3e55ed1c038b86) )
	ROM_LOAD( "3j_5.bin",     0x4000, 0x1000, CRC(52aec87e) SHA1(6516724c4e570972f070f6dab5b066ea92f56be0) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )
	ROM_LOAD( "1b_s1.bin",    0x0000, 0x1000, CRC(a1105714) SHA1(6e2e264748ab90bc5e8e8167f17ff91677ef6ae7) )

	ROM_REGION( 0x2000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "5l_c1.bin",    0x0000, 0x1000, CRC(5732eda9) SHA1(451de30946a9c8198c5ec83cc5c50e3ac2f9f56b) )
	ROM_LOAD( "5m_c2.bin",    0x1000, 0x1000, CRC(51c542fd) SHA1(1437f8cba15811361b2c5b46085587ea3598fc88) )

	ROM_REGION( 0x0100, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "10g.bpr",      0x0000, 0x0100, CRC(2ef89356) SHA1(5ed33386bab5d583358709c92f21ad9ad1a1bce9) ) /* dots */

	ROM_REGION( 0x0160, REGION_PROMS, 0 )
	ROM_LOAD( "8b.bpr",       0x0000, 0x0020, CRC(75b05da0) SHA1(aee98f5389e42332f30a6882ee22ff23f37e0573) ) /* palette */
	ROM_LOAD( "9d.bpr",       0x0020, 0x0100, CRC(aa6cf063) SHA1(08c1c9ab03eb168954b0170d40e95eed81022acd) ) /* loookup table */
	ROM_LOAD( "7a.bpr",       0x0120, 0x0020, CRC(48c8f094) SHA1(61592209720fddc8991751edf08b6950388af42e) ) /* video layout (not used) */
	ROM_LOAD( "10a.bpr",      0x0140, 0x0020, CRC(b8861096) SHA1(26fad384ed7a1a1e0ba719b5578e2dbb09334a25) ) /* video timing (not used) */
ROM_END

ROM_START( cottong )
	ROM_REGION( 0x10000, REGION_CPU1, 0 ) /* 64k for code */
	ROM_LOAD( "c1",           0x0000, 0x1000, CRC(2c256fe6) SHA1(115594c616497eec998e4e3255ec6ab6299346fa) )
	ROM_LOAD( "c2",           0x1000, 0x1000, CRC(1de5e6a0) SHA1(8bb3408a510662ff3b9b7201d2d06fe70685bf7f) )
	ROM_LOAD( "c3",           0x2000, 0x1000, CRC(01f909fe) SHA1(c80295e9f91ce25bfd28e72823b20ee6f6524a5c) )
	ROM_LOAD( "c4",           0x3000, 0x1000, CRC(a89eb3e3) SHA1(058928ade909faba06f177750f914cf1dabaefc3) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 ) /* 64k for the audio CPU */
	ROM_LOAD( "c7",           0x0000, 0x1000, CRC(3d83f6d3) SHA1(e10ed6b6ce7280697c1bc9dbe6c6e6018e1d8be4) )
	ROM_LOAD( "c8",           0x1000, 0x1000, CRC(323e1937) SHA1(75499d6c8a9032fac090a13cd4f36bd350f52dab) )

	ROM_REGION( 0x2000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "c5",           0x0000, 0x1000, CRC(992d079c) SHA1(b5acd30f2e8700cc4cd852b190bd1f4163b137e8) )
	ROM_LOAD( "c6",           0x1000, 0x1000, CRC(0149ef46) SHA1(58f684a9b7b9410236b3c54ea6c0fa9853a078c5) )

	ROM_REGION( 0x0100, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "5.bpr",        0x0000, 0x0100, CRC(21fb583f) SHA1(b8c65fbdd5d8b70bf51341cd60fc2efeaab8bb82) ) /* dots */

	ROM_REGION( 0x0160, REGION_PROMS, 0 )
	ROM_LOAD( "2.bpr",        0x0000, 0x0020, CRC(26f42e6f) SHA1(f51578216a5d588c4d0143ce7a23d695a15a3914) ) /* palette */
	ROM_LOAD( "3.bpr",        0x0020, 0x0100, CRC(4aecc0c8) SHA1(3c1086a598d84b4bcb277556b716fd18c76c4364) ) /* loookup table */
	ROM_LOAD( "7a.bpr",       0x0120, 0x0020, CRC(48c8f094) SHA1(61592209720fddc8991751edf08b6950388af42e) ) /* video layout (not used) */
	ROM_LOAD( "10a.bpr",      0x0140, 0x0020, CRC(b8861096) SHA1(26fad384ed7a1a1e0ba719b5578e2dbb09334a25) ) /* video timing (not used) */
ROM_END

ROM_START( commsega )
	ROM_REGION( 0x10000, REGION_CPU1, 0 ) /* 64k for code */
	ROM_LOAD( "csega1",       0x0000, 0x1000, CRC(92de3405) SHA1(81ef4274b13f92d6274a0a037d7dc77ba0f67a1b) )
	ROM_LOAD( "csega2",       0x1000, 0x1000, CRC(f14e2f9a) SHA1(c1a7ec1c306e07bac0bbf19b60f756650f63ae29) )
	ROM_LOAD( "csega3",       0x2000, 0x1000, CRC(941dbf48) SHA1(01d2d64fb662af423aa04507ba97997772130c54) )
	ROM_LOAD( "csega4",       0x3000, 0x1000, CRC(e0ac69b4) SHA1(3a52b2a6204b7310cfe321c582352b437de16660) )
	ROM_LOAD( "csega5",       0x4000, 0x1000, CRC(bc56ebd0) SHA1(a178cd5ba381b107e720e18f3549247477037998) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 ) /* 64k for the audio CPU */
	ROM_LOAD( "csega8",       0x0000, 0x1000, CRC(588b4210) SHA1(43bac1bdac721567e4b5d56e9e4488165872bd6a) )

	ROM_REGION( 0x2000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "csega7",       0x0000, 0x1000, CRC(e8e374f9) SHA1(442cc6b7e7d5b9472a5c16d6f78db0c03e651e98) )
	ROM_LOAD( "csega6",       0x1000, 0x1000, CRC(cf07fd5e) SHA1(4fe351c3d093f8f5fcf95e3e921a06e44d14d2a7) )

	ROM_REGION( 0x0100, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "gg3.bpr",      0x0000, 0x0100, CRC(ae7fd962) SHA1(118359cffb2ad3fdf09456a484aa730cb1b85a5d) ) /* dots */

	ROM_REGION( 0x0160, REGION_PROMS, 0 )
	ROM_LOAD( "gg1.bpr",      0x0000, 0x0020, CRC(f69e585a) SHA1(248740b154732b6bc6f772d4bb19d654798c3739) ) /* palette */
	ROM_LOAD( "gg2.bpr",      0x0020, 0x0100, CRC(0b756e30) SHA1(8890e305547df8df108af0f89074fb1c8bed8e6c) ) /* loookup table */
	ROM_LOAD( "gg0.bpr",      0x0120, 0x0020, CRC(48c8f094) SHA1(61592209720fddc8991751edf08b6950388af42e) ) /* video layout (not used) */
	ROM_LOAD( "tt3.bpr",      0x0140, 0x0020, CRC(b8861096) SHA1(26fad384ed7a1a1e0ba719b5578e2dbb09334a25) ) /* video timing (not used) */
ROM_END



GAME( 1980, rallyx,   0,        rallyx,   rallyx,   rallyx,   ROT0, "Namco", "Rally X", 0 )
GAME( 1980, rallyxm,  rallyx,   rallyx,   rallyx,   rallyx,   ROT0, "[Namco] (Midway license)", "Rally X (Midway)", 0 )
GAME( 1981, nrallyx,  0,        rallyx,   nrallyx,  rallyx,   ROT0, "Namco", "New Rally X", 0 )

GAME( 1981, jungler,  0,        tactcian, jungler,  jungler,  ROT90, "Konami", "Jungler", 0 )
GAME( 1981, junglers, jungler,  tactcian, jungler,  jungler,  ROT90, "[Konami] (Stern license)", "Jungler (Stern)", 0 )
GAME( 1982, tactcian, 0,        tactcian, tactcian, tactcian, ROT90, "[Konami] (Sega license)", "Tactician (set 1)", 0 )
GAME( 1981, tactcan2, tactcian, tactcian, tactcian, tactcian, ROT90, "[Konami] (Sega license)", "Tactician (set 2)", 0 )
GAME( 1982, locomotn, 0,        locomotn, locomotn, locomotn, ROT90, "Konami (Centuri license)", "Loco-Motion", 0 )
GAME( 1982, gutangtn, locomotn, locomotn, locomotn, locomotn, ROT90, "Konami (Sega license)", "Guttang Gottong", 0 )
GAME( 1982, cottong,  locomotn, locomotn, locomotn, locomotn, ROT90, "bootleg", "Cotocoto Cottong", 0 )
GAME( 1983, commsega, 0,        locomotn, commsega, commsega, ROT90, "Sega", "Commando (Sega)", GAME_IMPERFECT_SOUND )
