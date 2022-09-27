/***************************************************************************

Namco early 8080-based games

All these games run on the same hardware design, with minor differences.

Gee Bee was the very first videogame released by Namco, and runs on a b&w board.
The year after, the design was upgraded to add color and a second sound channel;
however, the b&w board was not abandoned, and more games were written for it with
the only difference of a larger gfx ROM (losing the ability to choose full or half
brightness for each character).



Gee Bee memory map
Navarone, Kaitei, SOS are the same but CGROM is twice as large

Address          Dir Data     Name      Description
---------------- --- -------- --------- -----------------------
-000xxxxxxxxxxxx R   xxxxxxxx ROM2/ROM0 program ROM
-001xxxxxxxxxxxx R   xxxxxxxx ROM1      program ROM
-010--xxxxxxxxxx R/W xxxxxxxx VRAM      tile code
-011--xxxxxxxxxx R   xxxxxxxx CGROM     Character Generator ROM
-100----xxxxxxxx R/W xxxxxxxx RAM       work RAM
-101----------00*R   --xxxxxx SW READ 0 switch inputs bank #1 (coins, buttons)
-101----------01*R   --xxxxxx SW READ 1 switch inputs bank #2 (optional, only kaiteik uses it)
-101----------10*R   --xxxxxx SW READ 2 dip switches
-101----------11*R   xxxxxxxx VOL READ  paddle input (digital joystick is hacked in for the other games)
-110----------00*  W xxxxxxxx BHD       ball horizontal position
-110----------01*  W xxxxxxxx BVD       ball vertical position
-110----------10*  W -------- n.c.
-110----------11*  W ----xxxx SOUND     select frequency and wave shape for sound
-111---------000*  W -------x LAMP 1    player 1 start lamp
-111---------001*  W -------x LAMP 2    player 2 start lamp
-111---------010*  W -------x LAMP 3    serve lamp
-111---------011*  W -------x COUNTER   coin counter
-111---------100*  W -------x LOCK OUT COIL  coin lock out
-111---------101*  W -------x BGW       invert CGROM data
-111---------110*  W -------x BALL ON   ball enable
-111---------111*  W -------x INV       screen flip

* = usually accessed using the in/out instructions

Bomb Bee / Cutie Q memory map
Warp Warp is the same but A13 and A15 are swapped to make space for more program ROM.

Address          Dir Data     Name      Description
---------------- --- -------- --------- -----------------------
-00xxxxxxxxxxxxx R   xxxxxxxx ROM       program ROM
-01---xxxxxxxxxx R/W xxxxxxxx RAM       work RAM
-10-00xxxxxxxxxx R/W xxxxxxxx V-RAM 1   tile code
-10-01xxxxxxxxxx R/W xxxxxxxx V-RAM 2   tile color
-10-1xxxxxxxxxxx R   xxxxxxxx CGROM     Character Generator ROM
-11-------00-xxx R   -------x SW READ   switch inputs (coins, buttons)
-11-------01---- R   xxxxxxxx VOL READ  paddle input (digital joystick is hacked in for warpwarp)
-11-------10-xxx R   -------x DIP SW 1  dip switches bank #1
-11-------11-xxx R   -------x DIP SW 2  dip switches bank #2 (optional, not used by any game)
-11-------00--00   W xxxxxxxx BHD       ball horizontal position
-11-------00--01   W xxxxxxxx BVD       ball vertical position
-11-------00--10   W ----xxxx SOUND     select frequency and wave shape for sound channel #1
-11-------00--11   W --------    WATCH DOG RESET
-11-------01----   W --xxxxxx MUSIC 1   sound channel #2 frequency
-11-------10----   W --xxxxxx MUSIC 2   sound channel #2 shape
-11-------11-000   W -------x LAMP 1    player 1 start lamp
-11-------11-001   W -------x LAMP 2    player 2 start lamp
-11-------11-010   W -------x LAMP 3    serve lamp (not used by warp warp)
-11-------11-011   W -------x n.c.
-11-------11-100   W -------x LOCK OUT  coin lock out
-11-------11-101   W -------x COUNTER   coin counter
-11-------11-110   W -------x BALL ON   ball enable + irq enable
-11-------11-111   W -------x INV       screen flip


Notes:
- Warp Warp Easter egg:
  - enter service mode
  - keep B1 pressed and enter the following sequence:
    2xR 6xD L 4xU
  (c) 1981 NAMCO LTD. will be added at the bottom of the screen.

- In the pinball games, there isn't a player 2 "serve" button - both players use
  the same input. I think this is correct behaviour, because there is nothing in
  the schematics suggesting otherwise (while there is a provision to switch from
  one paddle to the other). The Bomb Bee flyer shows that the cocktail cabinet
  did have one serve button per player, but I guess they were just wired together
  on the same input.

- The ball size changes from game to game. I have determined what I believe are
  the correct sizes by checking how the games handle the ball position in
  cocktail mode (the ball isn't automatically flipped by the hardware).

- kaitei and kaiteik are intriguing. The game is more or less the same, but the
  code is radically different, and the gfx ROMs are arranged differently.
  kaitei is by Namco and kaiteik is by "K'K-TOKKI". kaitei does a ROM/RAM
  test on startup, while kaiteik jumps straight into the game. kaitei is
  locked in cocktail mode, while kaiteik has support for a cabinet dip
  switch. The title screen in kaitei is nice and polished, while in kaiteik
  it's confused, with fishes going over the text. There are several other
  differences.
  The code in kaiteik is longer (0x1800 bytes vs. just 0x1000 in kaitei) and
  less efficient, while kaitei has some clever space optimizations.
  My opinion is that kaiteik is the prototype version, developed by a third
  party and sold to Namco, where it was rewritten.

- The coin counter doesn't work in kaiteik. This might be the expected behaviour.

- sos does a "coffee break" every 2000 points, showing a girl. The first times,
  she wears a bikini. If the "nudity" switch is on, after 6000 points she's
  topless and every 10000 points she's nude.

- The only difference between 'warpwarr' and 'warpwar2' is the copyright
  string on the first screen (when the scores are displayed) :
  * 'warpwarr' : "(c) 1981 ROCK-OLA MFG.CORP."  (text stored at 0x33ff to 0x3417)
  * 'warpwar2' : "(c) 1981 ROCK-OLA MFG.CO."    (text stored at 0x33ff to 0x3415)
  Note that the checksum at 0x37ff (used for checking ROM at 0x3000 to 0x37ff)
  is different of course.

- warpwarr doesn't have an explicit Namco copyright notice, but in the default
  high score table the names are NNN AAA MMM CCC OOO. warpwarp doesn't have an
  high score table at all.


TODO:
- I arbitrarily assigned a uniform blue overlay to sos. I don't know how it's
  supposed to be. navarone and kaitei are missing the overlay too.

***************************************************************************/

#include "driver.h"
#include "artwork.h"
#include "sound/custom.h"


/* from vidhrdw/warpwarp.c */
extern UINT8 *geebee_videoram,*warpwarp_videoram;
extern int geebee_bgw;
extern int warpwarp_ball_on;
extern int warpwarp_ball_h, warpwarp_ball_v;
extern int warpwarp_ball_sizex, warpwarp_ball_sizey;
extern int geebee_handleoverlay;
PALETTE_INIT( geebee );
PALETTE_INIT( navarone );
PALETTE_INIT( warpwarp );
VIDEO_START( geebee );
VIDEO_START( navarone );
VIDEO_START( warpwarp );
VIDEO_UPDATE( geebee );
VIDEO_UPDATE( warpwarp );
WRITE8_HANDLER( warpwarp_videoram_w );
WRITE8_HANDLER( geebee_videoram_w );

/* from sndhrdw/geebee.c */
WRITE8_HANDLER( geebee_sound_w );
void *geebee_sh_start(int clock, const struct CustomSound_interface *config);

/* from sndhrdw/warpwarp.c */
WRITE8_HANDLER( warpwarp_sound_w );
WRITE8_HANDLER( warpwarp_music1_w );
WRITE8_HANDLER( warpwarp_music2_w );
void *warpwarp_sh_start(int clock, const struct CustomSound_interface *config);


/*******************************************************
 *
 * Gee Bee overlay
 *
 *******************************************************/

#define PINK1	MAKE_ARGB(0x04,0xa0,0x00,0xe0)
#define PINK2 	MAKE_ARGB(0x04,0xe0,0x00,0xf0)
#define ORANGE	MAKE_ARGB(0x04,0xff,0xd0,0x00)
#define BLUE	MAKE_ARGB(0x04,0x00,0x00,0xff)

OVERLAY_START( geebee_overlay )
	OVERLAY_RECT(  1*8,  0*8,  4*8, 28*8, PINK2 )
	OVERLAY_RECT(  4*8,  0*8,  5*8,  4*8, PINK1 )
	OVERLAY_RECT(  4*8, 24*8,  5*8, 28*8, PINK1 )
	OVERLAY_RECT(  4*8,  4*8,  5*8, 24*8, ORANGE )
	OVERLAY_RECT(  5*8,  0*8, 28*8,  1*8, PINK1 )
	OVERLAY_RECT(  5*8, 27*8, 28*8, 28*8, PINK1 )
	OVERLAY_RECT(  5*8,  1*8, 28*8,  4*8, BLUE )
	OVERLAY_RECT(  5*8, 24*8, 28*8, 27*8, BLUE )
	OVERLAY_RECT( 12*8, 13*8, 13*8, 15*8, BLUE )
	OVERLAY_RECT( 21*8, 10*8, 23*8, 12*8, BLUE )
	OVERLAY_RECT( 21*8, 16*8, 23*8, 18*8, BLUE )
	OVERLAY_RECT( 28*8,  0*8, 29*8, 28*8, PINK2 )
	OVERLAY_RECT( 29*8,  0*8, 32*8, 28*8, PINK1 )
OVERLAY_END

OVERLAY_START( sos_overlay )
	OVERLAY_RECT(  1*8,  0*8,  33*8, 28*8, MAKE_ARGB(0x04,0x88,0x88,0xff) )
OVERLAY_END




static int handle_joystick;

READ8_HANDLER( geebee_in_r )
{
	int res;

	offset &= 3;
	res = readinputport(offset);
	if (offset == 3)
	{
		res = readinputport(3 + (flip_screen & 1));	// read player 2 input in cocktail mode
		if (handle_joystick)
		{
			/* map digital two-way joystick to two fixed VOLIN values */
			if (res & 2) return 0x9f;
			if (res & 1) return 0x0f;
			return 0x60;
		}
	}
	return res;
}

WRITE8_HANDLER( geebee_out6_w )
{
	switch (offset & 3)
	{
		case 0:
			warpwarp_ball_h = data;
			break;
		case 1:
			warpwarp_ball_v = data;
			break;
		case 2:
			/* n.c. */
			break;
		case 3:
			geebee_sound_w(0,data);
			break;
	}
}

WRITE8_HANDLER( geebee_out7_w )
{
	switch (offset & 7)
	{
		case 0:
			set_led_status(0,data & 1);
			break;
		case 1:
			set_led_status(1,data & 1);
			break;
		case 2:
			set_led_status(2,data & 1);
			break;
		case 3:
			coin_counter_w(0,data & 1);
			break;
		case 4:
			coin_lockout_global_w(~data & 1);
			break;
		case 5:
			if( geebee_bgw != (data & 1) )
				tilemap_mark_all_tiles_dirty(ALL_TILEMAPS);
			geebee_bgw = data & 1;
			break;
		case 6:
			warpwarp_ball_on = data & 1;
			break;
		case 7:
			flip_screen_set(data & 1);
			break;
	}
}


/* Read Switch Inputs */
static READ8_HANDLER( warpwarp_sw_r )
{
	return (readinputport(0) >> (offset & 7)) & 1;
}

/* Read Dipswitches */
static READ8_HANDLER( warpwarp_dsw1_r )
{
	return (readinputport(1) >> (offset & 7)) & 1;
}

/* Read mux Controller Inputs */
static READ8_HANDLER( warpwarp_vol_r )
{
	int res;

	res = readinputport(2 + (flip_screen & 1));
	if (handle_joystick)
	{
		if (res & 1) return 0x0f;
		if (res & 2) return 0x3f;
		if (res & 4) return 0x6f;
		if (res & 8) return 0x9f;
		return 0xff;
	}
	return res;
}

static WRITE8_HANDLER( warpwarp_out0_w )
{
	switch (offset & 3)
	{
		case 0:
			warpwarp_ball_h = data;
			break;
		case 1:
			warpwarp_ball_v = data;
			break;
		case 2:
			warpwarp_sound_w(0,data);
			break;
		case 3:
			watchdog_reset_w(0,data);
			break;
	}
}

static WRITE8_HANDLER( warpwarp_out3_w )
{
	switch (offset & 7)
	{
		case 0:
			set_led_status(0,data & 1);
			break;
		case 1:
			set_led_status(1,data & 1);
			break;
		case 2:
			set_led_status(2,data & 1);
			break;
		case 3:
			/* n.c. */
			break;
		case 4:
			coin_lockout_global_w(~data & 1);
			break;
		case 5:
			coin_counter_w(0,data & 1);
			break;
		case 6:
			warpwarp_ball_on = data & 1;
			cpu_interrupt_enable(0,data & 1);
			if (~data & 1)
				cpunum_set_input_line(0, 0, CLEAR_LINE);
			break;
		case 7:
			flip_screen_set(data & 1);
			break;
	}
}



static ADDRESS_MAP_START( readmem_geebee, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x1fff) AM_READ(MRA8_ROM)
	AM_RANGE(0x2000, 0x23ff) AM_READ(MRA8_RAM)
	AM_RANGE(0x3000, 0x37ff) AM_READ(MRA8_ROM)	/* 3000-33ff in GeeBee */
	AM_RANGE(0x4000, 0x40ff) AM_READ(MRA8_RAM)
	AM_RANGE(0x5000, 0x53ff) AM_READ(geebee_in_r)
ADDRESS_MAP_END

static ADDRESS_MAP_START( writemem_geebee, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x1fff) AM_WRITE(MWA8_ROM)
	AM_RANGE(0x2000, 0x23ff) AM_WRITE(geebee_videoram_w) AM_BASE(&geebee_videoram)
	AM_RANGE(0x2400, 0x27ff) AM_WRITE(geebee_videoram_w) /* mirror used by kaiteik due to a bug */
	AM_RANGE(0x3000, 0x37ff) AM_WRITE(MWA8_ROM)
    AM_RANGE(0x4000, 0x40ff) AM_WRITE(MWA8_RAM)
	AM_RANGE(0x6000, 0x6fff) AM_WRITE(geebee_out6_w)
	AM_RANGE(0x7000, 0x7fff) AM_WRITE(geebee_out7_w)
ADDRESS_MAP_END

static ADDRESS_MAP_START( readport_geebee, ADDRESS_SPACE_IO, 8 )
	AM_RANGE(0x50, 0x53) AM_READ(geebee_in_r)
ADDRESS_MAP_END

static ADDRESS_MAP_START( writeport_geebee, ADDRESS_SPACE_IO, 8 )
	AM_RANGE(0x60, 0x6f) AM_WRITE(geebee_out6_w)
	AM_RANGE(0x70, 0x7f) AM_WRITE(geebee_out7_w)
ADDRESS_MAP_END


static ADDRESS_MAP_START( readmem_bombbee, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x1fff) AM_READ(MRA8_ROM)
	AM_RANGE(0x2000, 0x23ff) AM_READ(MRA8_RAM)
	AM_RANGE(0x4000, 0x47ff) AM_READ(MRA8_RAM)
	AM_RANGE(0x4800, 0x4fff) AM_READ(MRA8_ROM)
	AM_RANGE(0x6000, 0x600f) AM_READ(warpwarp_sw_r)
	AM_RANGE(0x6010, 0x601f) AM_READ(warpwarp_vol_r)
	AM_RANGE(0x6020, 0x602f) AM_READ(warpwarp_dsw1_r)
ADDRESS_MAP_END

static ADDRESS_MAP_START( readmem_warpwarp, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x3fff) AM_READ(MRA8_ROM)
	AM_RANGE(0x8000, 0x83ff) AM_READ(MRA8_RAM)
	AM_RANGE(0x4000, 0x47ff) AM_READ(MRA8_RAM)
	AM_RANGE(0x4800, 0x4fff) AM_READ(MRA8_ROM)
	AM_RANGE(0xc000, 0xc00f) AM_READ(warpwarp_sw_r)
	AM_RANGE(0xc010, 0xc01f) AM_READ(warpwarp_vol_r)
	AM_RANGE(0xc020, 0xc02f) AM_READ(warpwarp_dsw1_r)
ADDRESS_MAP_END

static ADDRESS_MAP_START( writemem_bombbee, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x1fff) AM_WRITE(MWA8_ROM)
	AM_RANGE(0x2000, 0x23ff) AM_WRITE(MWA8_RAM)
	AM_RANGE(0x4000, 0x47ff) AM_WRITE(warpwarp_videoram_w) AM_BASE(&warpwarp_videoram)
	AM_RANGE(0x4800, 0x4fff) AM_WRITE(MWA8_ROM)
	AM_RANGE(0x6000, 0x600f) AM_WRITE(warpwarp_out0_w)
	AM_RANGE(0x6010, 0x601f) AM_WRITE(warpwarp_music1_w)
	AM_RANGE(0x6020, 0x602f) AM_WRITE(warpwarp_music2_w)
	AM_RANGE(0x6030, 0x603f) AM_WRITE(warpwarp_out3_w)
ADDRESS_MAP_END

static ADDRESS_MAP_START( writemem_warpwarp, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x3fff) AM_WRITE(MWA8_ROM)
	AM_RANGE(0x8000, 0x83ff) AM_WRITE(MWA8_RAM)
	AM_RANGE(0x4000, 0x47ff) AM_WRITE(warpwarp_videoram_w) AM_BASE(&warpwarp_videoram)
	AM_RANGE(0x4800, 0x4fff) AM_WRITE(MWA8_ROM)
	AM_RANGE(0xc000, 0xc00f) AM_WRITE(warpwarp_out0_w)
	AM_RANGE(0xc010, 0xc01f) AM_WRITE(warpwarp_music1_w)
	AM_RANGE(0xc020, 0xc02f) AM_WRITE(warpwarp_music2_w)
	AM_RANGE(0xc030, 0xc03f) AM_WRITE(warpwarp_out3_w)
ADDRESS_MAP_END



INPUT_PORTS_START( geebee )
	PORT_START_TAG("SW0")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1   )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2   )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_START1  )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_START2  )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_SERVICE( 0x20, IP_ACTIVE_LOW )
	PORT_BIT( 0xc0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START_TAG("SW1")
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START_TAG("DSW2")
	PORT_DIPNAME( 0x01, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Cocktail) )
	PORT_DIPNAME( 0x32, 0x10, "Lives/Bonus Life" )
	PORT_DIPSETTING(    0x10, "3/40k 80k" )
	PORT_DIPSETTING(    0x20, "3/70k 140k" )
	PORT_DIPSETTING(    0x30, "3/100k 200k" )
	PORT_DIPSETTING(    0x00, "3/None" )
	PORT_DIPSETTING(    0x12, "5/60k 120k" )
	PORT_DIPSETTING(    0x22, "5/100k 200k" )
	PORT_DIPSETTING(    0x32, "5/150k 300k" )
	PORT_DIPSETTING(    0x02, "5/None" )
	PORT_DIPNAME( 0x0c, 0x00, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x0c, DEF_STR( Free_Play ) )
	PORT_BIT( 0xc0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START_TAG("VOLIN")
	PORT_BIT( 0xff, 0x58, IPT_PADDLE ) PORT_MINMAX(0x10,0xa0) PORT_SENSITIVITY(30) PORT_KEYDELTA(15) PORT_CENTERDELTA(0) PORT_REVERSE

	PORT_START_TAG("VOLINC") //Cocktail
	PORT_BIT( 0xff, 0x58, IPT_PADDLE ) PORT_MINMAX(0x10,0xa0) PORT_SENSITIVITY(30) PORT_KEYDELTA(15) PORT_CENTERDELTA(0) PORT_REVERSE PORT_COCKTAIL
INPUT_PORTS_END

INPUT_PORTS_START( navarone )
	PORT_START_TAG("SW0")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1   )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_COCKTAIL
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_START1  )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_START2  )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0xc0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START_TAG("SW1")
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START_TAG("DSW2")
	PORT_DIPNAME( 0x01, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Cocktail) )
	PORT_DIPNAME( 0x0e, 0x06, "Lives/Bonus Life" )
	PORT_DIPSETTING(	0x04, "2/5000" )
	PORT_DIPSETTING(	0x08, "2/6000" )
	PORT_DIPSETTING(	0x0c, "2/7000" )
	PORT_DIPSETTING(	0x00, "2/None" )
	PORT_DIPSETTING(	0x06, "3/6000" )
	PORT_DIPSETTING(	0x0a, "3/7000" )
	PORT_DIPSETTING(	0x0e, "3/8000" )
	PORT_DIPSETTING(	0x02, "3/None" )
	PORT_DIPNAME( 0x30, 0x10, DEF_STR( Coinage ) )
	PORT_DIPSETTING(	0x30, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(	0x20, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(	0x00, DEF_STR( Free_Play ) )
	PORT_BIT( 0xc0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START_TAG("FAKE1")	/* Fake input port to support digital joystick */
	PORT_BIT( 0x01, 0x00, IPT_JOYSTICK_RIGHT )
	PORT_BIT( 0x02, 0x00, IPT_JOYSTICK_LEFT )

	PORT_START_TAG("FAKE2")	/* Fake input port to support digital joystick */
	PORT_BIT( 0x01, 0x00, IPT_JOYSTICK_RIGHT ) PORT_COCKTAIL
	PORT_BIT( 0x02, 0x00, IPT_JOYSTICK_LEFT ) PORT_COCKTAIL
INPUT_PORTS_END

INPUT_PORTS_START( kaitei )
	PORT_START_TAG("SW0")
	PORT_BIT( 0x01, IP_ACTIVE_LOW,	IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW,	IPT_BUTTON1 ) PORT_COCKTAIL
	PORT_BIT( 0x04, IP_ACTIVE_LOW,	IPT_START1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW,	IPT_START2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW,	IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW,	IPT_SERVICE1 )
	PORT_BIT( 0xc0, IP_ACTIVE_LOW,	IPT_UNUSED )

	PORT_START_TAG("SW1")
	PORT_BIT( 0xff, IP_ACTIVE_LOW,	IPT_UNUSED )

	PORT_START_TAG("DSW2")
	PORT_DIPNAME( 0x03, 0x01, DEF_STR( Lives ) )
	PORT_DIPSETTING(	0x00, "2" )
	PORT_DIPSETTING(	0x01, "3" )
	PORT_DIPSETTING(	0x02, "4" )
	PORT_DIPSETTING(	0x03, "5" )
	PORT_DIPNAME( 0x0c, 0x04, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x04, "2000" )
	PORT_DIPSETTING(    0x08, "4000" )
	PORT_DIPSETTING(    0x0c, "6000" )
	PORT_DIPSETTING(    0x00, DEF_STR( None ) )
	PORT_DIPNAME( 0x30, 0x10, DEF_STR( Coinage ) )
	PORT_DIPSETTING(	0x30, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(	0x20, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Free_Play ) )
	PORT_BIT( 0xc0, IP_ACTIVE_LOW,	IPT_UNUSED )

	PORT_START_TAG("FAKE1")	/* Fake input port to support digital joystick */
	PORT_BIT( 0x01, 0x00, IPT_JOYSTICK_RIGHT )
	PORT_BIT( 0x02, 0x00, IPT_JOYSTICK_LEFT )

	PORT_START_TAG("FAKE")	/* Fake input port to support digital joystick */
	PORT_BIT( 0x01, 0x00, IPT_JOYSTICK_RIGHT ) PORT_COCKTAIL
	PORT_BIT( 0x02, 0x00, IPT_JOYSTICK_LEFT ) PORT_COCKTAIL
INPUT_PORTS_END

INPUT_PORTS_START( kaiteik )
	PORT_START_TAG("SW0")
	PORT_BIT( 0x01, IP_ACTIVE_LOW,	IPT_COIN1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW,	IPT_START1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW,	IPT_START2 )
	PORT_BIT( 0xf2, 0xa0, IPT_UNKNOWN )	// game verifies these bits and freezes if they don't match

	PORT_START_TAG("SW1")
	PORT_BIT( 0x01, IP_ACTIVE_LOW,	IPT_JOYSTICK_RIGHT )
	PORT_BIT( 0x02, IP_ACTIVE_LOW,	IPT_JOYSTICK_LEFT )
	PORT_BIT( 0x04, IP_ACTIVE_LOW,	IPT_BUTTON1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW,	IPT_JOYSTICK_RIGHT ) PORT_COCKTAIL
	PORT_BIT( 0x10, IP_ACTIVE_LOW,	IPT_JOYSTICK_LEFT ) PORT_COCKTAIL
	PORT_BIT( 0x20, IP_ACTIVE_LOW,	IPT_BUTTON1 ) PORT_COCKTAIL
	PORT_BIT( 0xc0, 0x80, IPT_UNKNOWN )	// game verifies these two bits and freezes if they don't match

	PORT_START_TAG("DSW2")
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(	0x01, DEF_STR( Upright ) )
	PORT_DIPSETTING(	0x00, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0x06, 0x06, DEF_STR( Lives ) )
	PORT_DIPSETTING(	0x06, "4" )
	PORT_DIPSETTING(    0x04, "5" )
	PORT_DIPSETTING(	0x02, "6" )
	PORT_DIPSETTING(    0x00, "7" )
	PORT_DIPNAME( 0x18, 0x18, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x18, "4000" )
	PORT_DIPSETTING(    0x10, "6000" )
	PORT_DIPSETTING(    0x08, "8000" )
	PORT_DIPSETTING(    0x00, "10000" )
	PORT_DIPNAME( 0x20, 0x00, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x20, DEF_STR( On ) )
	PORT_BIT( 0xc0, 0x80, IPT_UNKNOWN )	// game verifies these two bits and freezes if they don't match

	PORT_START_TAG("VOLIN1")
	PORT_BIT( 0x3f, 0x00, IPT_UNKNOWN )
	PORT_BIT( 0xc0, 0x80, IPT_UNKNOWN )	// game verifies these two bits and freezes if they don't match

	PORT_START_TAG("VOLIN2")
	PORT_BIT( 0x3f, 0x00, IPT_UNKNOWN )
	PORT_BIT( 0xc0, 0x80, IPT_UNKNOWN )	// game verifies these two bits and freezes if they don't match
INPUT_PORTS_END

INPUT_PORTS_START( sos )
	PORT_START_TAG("SW0")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1   )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_COCKTAIL
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_START1  )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_START2  )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_SERVICE1   )
	PORT_BIT( 0xc0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START_TAG("SW1")
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START_TAG("DSW2")
	PORT_DIPNAME( 0x01, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Cocktail) )
	PORT_DIPNAME( 0x06, 0x02, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x00, "2" )
	PORT_DIPSETTING(    0x02, "3" )
	PORT_DIPSETTING(    0x04, "4" )
	PORT_DIPSETTING(    0x06, "5" )
	PORT_DIPNAME( 0x18, 0x08, DEF_STR( Coinage ) )
	PORT_DIPSETTING(	0x18, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(	0x10, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Free_Play ) )
	PORT_DIPNAME( 0x20, 0x20, "Nudity" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x20, DEF_STR( On ) )
	PORT_BIT( 0xc0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START_TAG("FAKE1")	/* Fake input port to support digital joystick */
	PORT_BIT( 0x01, 0x00, IPT_JOYSTICK_RIGHT )
	PORT_BIT( 0x02, 0x00, IPT_JOYSTICK_LEFT )

	PORT_START_TAG("FAKE2")	/* Fake input port to support digital joystick */
	PORT_BIT( 0x01, 0x00, IPT_JOYSTICK_RIGHT ) PORT_COCKTAIL
	PORT_BIT( 0x02, 0x00, IPT_JOYSTICK_LEFT ) PORT_COCKTAIL
INPUT_PORTS_END

INPUT_PORTS_START( bombbee )
	PORT_START_TAG("IN0")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_SERVICE( 0x20, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(	0x40, DEF_STR( Upright ) )
	PORT_DIPSETTING(	0x00, DEF_STR( Cocktail ) )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_SERVICE1 )

	PORT_START_TAG("DSW1")
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Coinage ) )
	PORT_DIPSETTING(	0x02, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(	0x03, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(	0x01, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(	0x00, DEF_STR( Free_Play ) )
	PORT_DIPNAME( 0x0c, 0x00, DEF_STR( Lives ) )
	PORT_DIPSETTING(	0x00, "3" )
	PORT_DIPSETTING(	0x04, "4" )
//  PORT_DIPSETTING(    0x08, "4" )             // duplicated setting
	PORT_DIPSETTING(	0x0c, "5" )
	PORT_DIPNAME( 0x10, 0x00, DEF_STR( Unused ) )
	PORT_DIPSETTING(	0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0xe0, 0x00, "Replay" )		// awards 1 credit
	PORT_DIPSETTING(	0x00, "50000" )
	PORT_DIPSETTING(	0x20, "60000" )
	PORT_DIPSETTING(	0x40, "70000" )
	PORT_DIPSETTING(	0x60, "80000" )
	PORT_DIPSETTING(	0x80, "100000" )
	PORT_DIPSETTING(	0xa0, "120000" )
	PORT_DIPSETTING(	0xc0, "150000" )
	PORT_DIPSETTING(	0xe0, DEF_STR( None ) )

	PORT_START_TAG("VOLIN1")	/* Mux input - player 1 controller - handled by warpwarp_vol_r */
	PORT_BIT( 0xff, 0x60, IPT_PADDLE ) PORT_MINMAX(0x14,0xac) PORT_SENSITIVITY(30) PORT_KEYDELTA(10) PORT_CENTERDELTA(0) PORT_REVERSE

	PORT_START_TAG("VOLIN2")	/* Mux input - player 2 controller - handled by warpwarp_vol_r */
	PORT_BIT( 0xff, 0x60, IPT_PADDLE ) PORT_MINMAX(0x14,0xac) PORT_SENSITIVITY(30) PORT_KEYDELTA(10) PORT_CENTERDELTA(0) PORT_REVERSE PORT_COCKTAIL
INPUT_PORTS_END

INPUT_PORTS_START( cutieq )
	PORT_START_TAG("IN0")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_SERVICE( 0x20, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(	0x40, DEF_STR( Upright ) )
	PORT_DIPSETTING(	0x00, DEF_STR( Cocktail ) )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_SERVICE1 )

	PORT_START_TAG("DSW1")
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Coinage ) )
	PORT_DIPSETTING(	0x02, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(	0x03, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(	0x01, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(	0x00, DEF_STR( Free_Play ) )
	PORT_DIPNAME( 0x0c, 0x00, DEF_STR( Lives ) )
	PORT_DIPSETTING(	0x00, "3" )
	PORT_DIPSETTING(	0x04, "4" )
//  PORT_DIPSETTING(    0x08, "4" )             // duplicated setting
	PORT_DIPSETTING(	0x0c, "5" )
	PORT_DIPNAME( 0x10, 0x00, DEF_STR( Unused ) )
	PORT_DIPSETTING(	0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0xe0, 0x00, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(	0x00, "50000" )
	PORT_DIPSETTING(	0x20, "60000" )
	PORT_DIPSETTING(	0x40, "80000" )
	PORT_DIPSETTING(	0x60, "100000" )
	PORT_DIPSETTING(	0x80, "120000" )
	PORT_DIPSETTING(	0xa0, "150000" )
	PORT_DIPSETTING(	0xc0, "200000" )
	PORT_DIPSETTING(	0xe0, DEF_STR( None ) )

	PORT_START_TAG("VOLIN1")	/* Mux input - player 1 controller - handled by warpwarp_vol_r */
	PORT_BIT( 0xff, 0x60, IPT_PADDLE ) PORT_MINMAX(0x14,0xac) PORT_SENSITIVITY(30) PORT_KEYDELTA(10) PORT_CENTERDELTA(0) PORT_REVERSE

	PORT_START_TAG("VOLIN2")	/* Mux input - player 2 controller - handled by warpwarp_vol_r */
	PORT_BIT( 0xff, 0x60, IPT_PADDLE ) PORT_MINMAX(0x14,0xac) PORT_SENSITIVITY(30) PORT_KEYDELTA(10) PORT_CENTERDELTA(0) PORT_REVERSE PORT_COCKTAIL
INPUT_PORTS_END

INPUT_PORTS_START( warpwarp )
	PORT_START_TAG("IN0")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_COCKTAIL
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_SERVICE( 0x20, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(	0x40, DEF_STR( Upright ) )
	PORT_DIPSETTING(	0x00, DEF_STR( Cocktail ) )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_SERVICE1 )

	PORT_START_TAG("DSW1")
	PORT_DIPNAME( 0x03, 0x01, DEF_STR( Coinage ) )
	PORT_DIPSETTING(	0x03, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(	0x01, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(	0x02, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(	0x00, DEF_STR( Free_Play ) )
	PORT_DIPNAME( 0x0c, 0x04, DEF_STR( Lives ) )
	PORT_DIPSETTING(	0x00, "2" )
	PORT_DIPSETTING(	0x04, "3" )
	PORT_DIPSETTING(	0x08, "4" )
	PORT_DIPSETTING(	0x0c, "5" )
	/* Bonus Lives when "Lives" Dip Switch is set to "2", "3" or "4" */
	PORT_DIPNAME( 0x30, 0x00, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(	0x00, "8000 30000" )
	PORT_DIPSETTING(	0x10, "10000 40000" )
	PORT_DIPSETTING(	0x20, "15000 60000" )
	PORT_DIPSETTING(	0x30, DEF_STR( None ) )
	/* Bonus Lives when "Lives" Dip Switch is set to "5"
    PORT_DIPNAME( 0x30, 0x00, DEF_STR( Bonus_Life ) )
    PORT_DIPSETTING(    0x00, "30000" )
    PORT_DIPSETTING(    0x10, "40000" )
    PORT_DIPSETTING(    0x20, "60000" )
    PORT_DIPSETTING(    0x30, DEF_STR( None ) )
    */
	PORT_DIPNAME( 0x40, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(	0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )
	/* when level selection is On, press 1 to increase level */
	PORT_DIPNAME( 0x80, 0x80, "Level Selection (Cheat)")
	PORT_DIPSETTING(	0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )

	PORT_START_TAG("VOLIN1")	/* FAKE - input port to simulate an analog stick - handled by warpwarp_vol_r */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN ) PORT_4WAY
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP ) PORT_4WAY
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT ) PORT_4WAY
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT ) PORT_4WAY

	PORT_START_TAG("VOLIN2")	/* FAKE - input port to simulate an analog stick - handled by warpwarp_vol_r */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN ) PORT_4WAY PORT_COCKTAIL
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP ) PORT_4WAY PORT_COCKTAIL
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT ) PORT_4WAY PORT_COCKTAIL
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT ) PORT_4WAY PORT_COCKTAIL
INPUT_PORTS_END

/* has High Score Initials dip switch instead of rack test */
INPUT_PORTS_START( warpwarr )
	PORT_START_TAG("IN0")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_COCKTAIL
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_SERVICE( 0x20, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(	0x40, DEF_STR( Upright ) )
	PORT_DIPSETTING(	0x00, DEF_STR( Cocktail ) )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_SERVICE1 )

	PORT_START_TAG("DSW1")
	PORT_DIPNAME( 0x03, 0x01, DEF_STR( Coinage ) )
	PORT_DIPSETTING(	0x03, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(	0x01, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(	0x02, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(	0x00, DEF_STR( Free_Play ) )
	PORT_DIPNAME( 0x0c, 0x04, DEF_STR( Lives ) )
	PORT_DIPSETTING(	0x00, "2" )
	PORT_DIPSETTING(	0x04, "3" )
	PORT_DIPSETTING(	0x08, "4" )
	PORT_DIPSETTING(	0x0c, "5" )
	/* Bonus Lives when "Lives" Dip Switch is set to "2", "3" or "4" */
	PORT_DIPNAME( 0x30, 0x00, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(	0x00, "8000 30000" )
	PORT_DIPSETTING(	0x10, "10000 40000" )
	PORT_DIPSETTING(	0x20, "15000 60000" )
	PORT_DIPSETTING(	0x30, DEF_STR( None ) )
	/* Bonus Lives when "Lives" Dip Switch is set to "5"
    PORT_DIPNAME( 0x30, 0x00, DEF_STR( Bonus_Life ) )
    PORT_DIPSETTING(    0x00, "30000" )
    PORT_DIPSETTING(    0x10, "40000" )
    PORT_DIPSETTING(    0x20, "60000" )
    PORT_DIPSETTING(    0x30, DEF_STR( None ) )
    */
	PORT_DIPNAME( 0x40, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(	0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x00, "High Score Initials" )
	PORT_DIPSETTING(	0x80, DEF_STR( No ) )
	PORT_DIPSETTING(	0x00, DEF_STR( Yes ) )

	PORT_START_TAG("VOLIN1")	/* FAKE - input port to simulate an analog stick - handled by warpwarp_vol_r */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN ) PORT_4WAY
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP ) PORT_4WAY
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT ) PORT_4WAY
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT ) PORT_4WAY

	PORT_START_TAG("VOLIN2")	/* FAKE - input port to simulate an analog stick - handled by warpwarp_vol_r */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN ) PORT_4WAY PORT_COCKTAIL
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP ) PORT_4WAY PORT_COCKTAIL
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT ) PORT_4WAY PORT_COCKTAIL
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT ) PORT_4WAY PORT_COCKTAIL
INPUT_PORTS_END



static const gfx_layout charlayout_1k =
{
	8,8,
	128,
	1,
	{ 0 },
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8
};

static const gfx_layout charlayout_2k =
{
	8,8,
	256,
	1,
	{ 0 },
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8
};

static const gfx_decode gfxdecodeinfo_1k[] =
{
	{ REGION_CPU1, 0x3000, &charlayout_1k, 0, 4 },
	{ -1 } /* end of array */
};

static const gfx_decode gfxdecodeinfo_2k[] =
{
	{ REGION_CPU1, 0x3000, &charlayout_2k, 0, 2 },
	{ -1 } /* end of array */
};

static const gfx_decode gfxdecodeinfo_color[] =
{
	{ REGION_CPU1, 0x4800, &charlayout_2k, 0, 256 },
	{ -1 } /* end of array */
};


static struct CustomSound_interface geebee_custom_interface =
{
	geebee_sh_start
};

static struct CustomSound_interface warpwarp_custom_interface =
{
	warpwarp_sh_start
};



static MACHINE_DRIVER_START( geebee )

	/* basic machine hardware */
	MDRV_CPU_ADD_TAG("main", 8080,18432000/9) 		/* 18.432 MHz / 9 */
	MDRV_CPU_PROGRAM_MAP(readmem_geebee,writemem_geebee)
	MDRV_CPU_IO_MAP(readport_geebee,writeport_geebee)
	MDRV_CPU_VBLANK_INT(irq0_line_pulse,1)	/* one interrupt per frame */

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_REAL_60HZ_VBLANK_DURATION)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(34*8, 28*8)
	MDRV_VISIBLE_AREA(0*8, 34*8-1, 0*8, 28*8-1)
	MDRV_GFXDECODE(gfxdecodeinfo_1k)
	MDRV_PALETTE_LENGTH(3)
	MDRV_COLORTABLE_LENGTH(4*2)

	MDRV_PALETTE_INIT(geebee)
	MDRV_VIDEO_START(geebee)
	MDRV_VIDEO_UPDATE(geebee)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")

	MDRV_SOUND_ADD(CUSTOM, 0)
	MDRV_SOUND_CONFIG(geebee_custom_interface)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.0)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( navarone )

	/* basic machine hardware */
	MDRV_IMPORT_FROM(geebee)

	MDRV_GFXDECODE(gfxdecodeinfo_2k)
	MDRV_COLORTABLE_LENGTH(2*2)

	MDRV_PALETTE_INIT(navarone)
	MDRV_VIDEO_START(navarone)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( bombbee )

	/* basic machine hardware */
	MDRV_CPU_ADD_TAG("main", 8080,18432000/9) 		/* 18.432 MHz / 9 */
	MDRV_CPU_PROGRAM_MAP(readmem_bombbee,writemem_bombbee)
	MDRV_CPU_VBLANK_INT(irq0_line_assert,1)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)	/* frames per second, vblank duration */

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(34*8, 28*8)
	MDRV_VISIBLE_AREA(0*8, 34*8-1, 0*8, 28*8-1)
	MDRV_GFXDECODE(gfxdecodeinfo_color)
	MDRV_PALETTE_LENGTH(256)
	MDRV_COLORTABLE_LENGTH(2*256)

	MDRV_PALETTE_INIT(warpwarp)
	MDRV_VIDEO_START(warpwarp)
	MDRV_VIDEO_UPDATE(warpwarp)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")

	MDRV_SOUND_ADD(CUSTOM, 0)
	MDRV_SOUND_CONFIG(warpwarp_custom_interface)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.0)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( warpwarp )

	/* basic machine hardware */
	MDRV_IMPORT_FROM(bombbee)
	MDRV_CPU_MODIFY("main")
	MDRV_CPU_PROGRAM_MAP(readmem_warpwarp,writemem_warpwarp)
MACHINE_DRIVER_END


/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START( geebee )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )
	ROM_LOAD( "geebee.1k",    0x0000, 0x1000, CRC(8a5577e0) SHA1(356d33e19c6b4f519816ee4b65ff9b59d6c1b565) )
	ROM_LOAD( "geebee.3a",    0x3000, 0x0400, CRC(f257b21b) SHA1(c788fd923438f1bffbff9ff3cd4c5c8b547c0c14) )
ROM_END

ROM_START( geebeeg )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )
	ROM_LOAD( "geebee.1k",    0x0000, 0x1000, CRC(8a5577e0) SHA1(356d33e19c6b4f519816ee4b65ff9b59d6c1b565) )
	ROM_LOAD( "geebeeg.3a",   0x3000, 0x0400, CRC(a45932ba) SHA1(48f70742c42a9377f31fac3a1e43123751e57656) )
ROM_END

ROM_START( navarone )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )
	ROM_LOAD( "navalone.p1",  0x0000, 0x0800, CRC(5a32016b) SHA1(d856d069eba470a81341de0bf47eca2a629a69a6) )
	ROM_LOAD( "navalone.p2",  0x0800, 0x0800, CRC(b1c86fe3) SHA1(0293b742806c1517cb126443701115a3427fc60a) )
	ROM_LOAD( "navalone.chr", 0x3000, 0x0800, CRC(b26c6170) SHA1(ae0aec2b60e1fd3b212e311afb1c588b2b286433) )
ROM_END

ROM_START( kaitei )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )
	ROM_LOAD( "kaitein.p1",   0x0000, 0x0800, CRC(d88e10ae) SHA1(76d6cd46b6e59e528e7a8fff9965375a1446a91d) )
	ROM_LOAD( "kaitein.p2",   0x0800, 0x0800, CRC(aa9b5763) SHA1(64a6c8f25b0510841dcce0b57505731aa0deeda7) )
	ROM_LOAD( "kaitein.chr",  0x3000, 0x0800, CRC(3125af4d) SHA1(9e6b161636665ee48d6bde2d5fc412fde382c687) )
ROM_END

ROM_START( kaiteik )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )
	ROM_LOAD( "kaitei_7.1k",  0x0000, 0x0800, CRC(32f70d48) SHA1(c5ae606df1d0e513daea909f5474309a176096c1) )
	ROM_RELOAD(               0x0800, 0x0800 )
    ROM_LOAD( "kaitei_1.1m",  0x1000, 0x0400, CRC(9a7ab3b9) SHA1(94a82ba66e51c8203ec61c9320edbddbb6462d33) )
	ROM_LOAD( "kaitei_2.1p",  0x1400, 0x0400, CRC(5eeb0fff) SHA1(91cb84a9af8e4df4e6c896e7655199328b7da30b) )
	ROM_LOAD( "kaitei_3.1s",  0x1800, 0x0400, CRC(5dff4df7) SHA1(c179c93a559a0d18db3092c842634de02f3f03ea) )
	ROM_LOAD( "kaitei_4.1t",  0x1c00, 0x0400, CRC(e5f303d6) SHA1(6dd57e0b17f51d101c6c5dbfeadb7418098cc440) )
	ROM_LOAD( "kaitei_5.bin", 0x3000, 0x0400, CRC(60fdb795) SHA1(723e635eed9937a28bee0b7978413984651ee87f) )
	ROM_LOAD( "kaitei_6.bin", 0x3400, 0x0400, CRC(21399ace) SHA1(0ad49be2c9bdab2f9dc41c7348d1d4b4b769e3c4) )
ROM_END

ROM_START( sos )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )
	ROM_LOAD( "sos.p1",       0x0000, 0x0800, CRC(f70bdafb) SHA1(e71d552ccc9adad48225bdb4d62c31c5741a3e95) )
	ROM_LOAD( "sos.p2",       0x0800, 0x0800, CRC(58e9c480) SHA1(0eeb5982183d0e9f9dbae04839b604a0c22b420e) )
	ROM_LOAD( "sos.chr",      0x3000, 0x0800, CRC(66f983e4) SHA1(b3cf8bff4ac6b554d3fc06eeb8227b3b2a0dd554) )
ROM_END

ROM_START( bombbee )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "bombbee.1k",   0x0000, 0x2000, CRC(9f8cd7af) SHA1(0d6e1ee5519660d1498eb7a093872ed5034423f2) )
	ROM_LOAD( "bombbee.4c",   0x4800, 0x0800, CRC(5f37d569) SHA1(d5e3fb4c5a1612a6e568c8970161b0290b88993f) )
ROM_END

ROM_START( cutieq )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "cutieq.1k",    0x0000, 0x2000, CRC(6486cdca) SHA1(914c36487fba2dd57c3fd1f011b2225d2baac2bf) )
	ROM_LOAD( "cutieq.4c",    0x4800, 0x0800, CRC(0e1618c9) SHA1(456e9b3d6bae8b4af7778a38e4f40bb6736b0690) )
ROM_END

ROM_START( warpwarp )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "g-n9601n.2r",  0x0000, 0x1000, CRC(f5262f38) SHA1(1c64d0282b0a209390a548ceeaaf8b7b55e50896) )
	ROM_LOAD( "g-09602n.2m",  0x1000, 0x1000, CRC(de8355dd) SHA1(133d137711d79aaeb45cd3ee041c0be3b73e1b2f) )
	ROM_LOAD( "g-09603n.1p",  0x2000, 0x1000, CRC(bdd1dec5) SHA1(bb3d9d1500e31bb271a394facaec7adc3c987e5e) )
	ROM_LOAD( "g-09613n.1t",  0x3000, 0x0800, CRC(af3d77ef) SHA1(5b79aabbe14c2997e0b1a9276c483ae76814a63a) )
	ROM_LOAD( "g-9611n.4c",   0x4800, 0x0800, CRC(380994c8) SHA1(0cdf6a05db52c423365bff9c9df6d93ac885794e) )
ROM_END

ROM_START( warpwarr )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "g-09601.2r",   0x0000, 0x1000, CRC(916ffa35) SHA1(bca2087f8b78a128cdffc55db9814854b72daab5) )
	ROM_LOAD( "g-09602.2m",   0x1000, 0x1000, CRC(398bb87b) SHA1(74373336288dc13d59e6f7e7c718aa51d857b087) )
	ROM_LOAD( "g-09603.1p",   0x2000, 0x1000, CRC(6b962fc4) SHA1(0291d0c574a1048e52121ca57e01098bff04da40) )
	ROM_LOAD( "g-09613.1t",   0x3000, 0x0800, CRC(60a67e76) SHA1(af65e7bf16a5e69fee05c0134e3b8d5bca142402) )
	ROM_LOAD( "g-9611.4c",    0x4800, 0x0800, CRC(00e6a326) SHA1(67b7ab5b7b2c9a97d4d690d88561da48b86bc66e) )
ROM_END

ROM_START( warpwar2 )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "g-09601.2r",   0x0000, 0x1000, CRC(916ffa35) SHA1(bca2087f8b78a128cdffc55db9814854b72daab5) )
	ROM_LOAD( "g-09602.2m",   0x1000, 0x1000, CRC(398bb87b) SHA1(74373336288dc13d59e6f7e7c718aa51d857b087) )
	ROM_LOAD( "g-09603.1p",   0x2000, 0x1000, CRC(6b962fc4) SHA1(0291d0c574a1048e52121ca57e01098bff04da40) )
	ROM_LOAD( "g-09612.1t",   0x3000, 0x0800, CRC(b91e9e79) SHA1(378323d83c550b3acabc83dba946ab089b9195cb) )
	ROM_LOAD( "g-9611.4c",    0x4800, 0x0800, CRC(00e6a326) SHA1(67b7ab5b7b2c9a97d4d690d88561da48b86bc66e) )
ROM_END



static DRIVER_INIT( geebee )
{
	handle_joystick = 0;

	artwork_set_overlay(geebee_overlay);
	// turn off overlay in cocktail mode; this assumes that the cabinet dip switch
	// is bit 0 of input port 2
	geebee_handleoverlay = 1;

	warpwarp_ball_sizex = 4;
	warpwarp_ball_sizey = 4;
}

static DRIVER_INIT( navarone )
{
	handle_joystick = 1;
	geebee_handleoverlay = 0;

	warpwarp_ball_sizex = 4;
	warpwarp_ball_sizey = 4;
}

static DRIVER_INIT( kaitei )
{
	handle_joystick = 1;
	geebee_handleoverlay = 0;

	warpwarp_ball_sizex = 1;
	warpwarp_ball_sizey = 16;
}

static DRIVER_INIT( kaiteik )
{
	handle_joystick = 0;
	geebee_handleoverlay = 0;

	warpwarp_ball_sizex = 1;
	warpwarp_ball_sizey = 16;
}

static DRIVER_INIT( sos )
{
	handle_joystick = 1;

	artwork_set_overlay(sos_overlay);
	geebee_handleoverlay = 0;

	warpwarp_ball_sizex = 4;
	warpwarp_ball_sizey = 2;
}

static DRIVER_INIT( bombbee )
{
	handle_joystick = 0;
	geebee_handleoverlay = 0;

	warpwarp_ball_sizex = 4;
	warpwarp_ball_sizey = 4;
}

static DRIVER_INIT( warpwarp )
{
	handle_joystick = 1;
	geebee_handleoverlay = 0;

	warpwarp_ball_sizex = 4;
	warpwarp_ball_sizey = 4;
}


/* B & W games */
GAME( 1978, geebee,   0,        geebee,   geebee,   geebee,   ROT90, "Namco", "Gee Bee", 0 )
GAME( 1978, geebeeg,  geebee,   geebee,   geebee,   geebee,   ROT90, "[Namco] (Gremlin license)", "Gee Bee (Gremlin)", 0 )
GAME( 1980, navarone, 0,        navarone, navarone, navarone, ROT90, "Namco", "Navarone", GAME_IMPERFECT_SOUND )
GAME( 1980, kaitei,   0,        navarone, kaitei,   kaitei,   ROT90, "Namco", "Kaitei Takara Sagashi", 0 )
GAME( 1980, kaiteik,  kaitei,   navarone, kaiteik,  kaiteik,  ROT90, "K.K. Tokki", "Kaitei Takara Sagashi (K'K-Tokki)", 0 )
GAME( 1980, sos,      0,        navarone, sos,      sos,      ROT90, "Namco", "SOS", GAME_IMPERFECT_SOUND )

/* Color games */
GAME( 1979, bombbee,  0,        bombbee,  bombbee,  bombbee,  ROT90, "Namco", "Bomb Bee", 0 )
GAME( 1979, cutieq,   0,        bombbee,  cutieq,   bombbee,  ROT90, "Namco", "Cutie Q", 0 )
GAME( 1981, warpwarp, 0,        warpwarp, warpwarp, warpwarp, ROT90, "Namco", "Warp & Warp", 0 )
GAME( 1981, warpwarr, warpwarp, warpwarp, warpwarr, warpwarp, ROT90, "[Namco] (Rock-ola license)", "Warp Warp (Rock-ola set 1)", 0 )
GAME( 1981, warpwar2, warpwarp, warpwarp, warpwarr, warpwarp, ROT90, "[Namco] (Rock-ola license)", "Warp Warp (Rock-ola set 2)", 0 )
