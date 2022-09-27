/***************************************************************************

    Atari Battlezone hardware

    Games supported:
        * Battlezone
        * Bradley Trainer
        * Red Baron

    Known bugs:
        * none at this time

****************************************************************************

    Battlezone memory map (preliminary)

    0000-04ff RAM
    0800      IN0
    0a00      IN1
    0c00      IN2

    1200      Vector generator start (write)
    1400
    1600      Vector generator reset (write)

    1800      Mathbox Status register
    1810      Mathbox value (lo-byte)
    1818      Mathbox value (hi-byte)
    1820-182f POKEY I/O
    1828      Control inputs
    1860-187f Mathbox RAM

    2000-2fff Vector generator RAM
    3000-37ff Mathbox ROM
    5000-7fff ROM

    Battlezone settings:

    0 = OFF  1 = ON  X = Don't Care  $ = Atari suggests

    ** IMPORTANT - BITS are INVERTED in the game itself **

    TOP 8 SWITCH DIP
    87654321
    --------
    XXXXXX11   Free Play
    XXXXXX10   1 coin for 2 plays
    XXXXXX01   1 coin for 1 play
    XXXXXX00   2 coins for 1 play
    XXXX11XX   Right coin mech x 1
    XXXX10XX   Right coin mech x 4
    XXXX01XX   Right coin mech x 5
    XXXX00XX   Right coin mech x 6
    XXX1XXXX   Center (or Left) coin mech x 1
    XXX0XXXX   Center (or Left) coin mech x 2
    111XXXXX   No bonus coin
    110XXXXX   For every 2 coins inserted, game logic adds 1 more
    101XXXXX   For every 4 coins inserted, game logic adds 1 more
    100XXXXX   For every 4 coins inserted, game logic adds 2 more
    011XXXXX   For every 5 coins inserted, game logic adds 1 more

    BOTTOM 8 SWITCH DIP
    87654321
    --------
    XXXXXX11   Game starts with 2 tanks
    XXXXXX10   Game starts with 3 tanks  $
    XXXXXX01   Game starts with 4 tanks
    XXXXXX00   Game starts with 5 tanks
    XXXX11XX   Missile appears after 5,000 points
    XXXX10XX   Missile appears after 10,000 points  $
    XXXX01XX   Missile appears after 20,000 points
    XXXX00XX   Missile appears after 30,000 points
    XX11XXXX   No bonus tank
    XX10XXXX   Bonus taks at 15,000 and 100,000 points  $
    XX01XXXX   Bonus taks at 20,000 and 100,000 points
    XX00XXXX   Bonus taks at 50,000 and 100,000 points
    11XXXXXX   English language
    10XXXXXX   French language
    01XXXXXX   German language
    00XXXXXX   Spanish language

    4 SWITCH DIP

    XX11   All coin mechanisms register on one coin counter
    XX01   Left and center coin mechanisms on one coin counter, right on second
    XX10   Center and right coin mechanisms on one coin counter, left on second
    XX00   Each coin mechanism has it's own counter

****************************************************************************

    Red Baron memory map (preliminary)

    0000-04ff RAM
    0800      COIN_IN
    0a00      IN1
    0c00      IN2

    1200      Vector generator start (write)
    1400
    1600      Vector generator reset (write)

    1800      Mathbox Status register
    1802      Button inputs
    1804      Mathbox value (lo-byte)
    1806      Mathbox value (hi-byte)
    1808      Red Baron Sound (bit 1 selects joystick pot to read also)
    1810-181f POKEY I/O
    1818      Joystick inputs
    1860-187f Mathbox RAM

    2000-2fff Vector generator RAM
    3000-37ff Mathbox ROM
    5000-7fff ROM

    RED BARON DIP SWITCH SETTINGS
    Donated by Dana Colbert


    $=Default
    "K" = 1,000

    Switch at position P10
                                      8    7    6    5    4    3    2    1
                                    _________________________________________
    English                        $|    |    |    |    |    |    |Off |Off |
    Spanish                         |    |    |    |    |    |    |Off | On |
    French                          |    |    |    |    |    |    | On |Off |
    German                          |    |    |    |    |    |    | On | On |
                                    |    |    |    |    |    |    |    |    |
     Bonus airplane granted at:     |    |    |    |    |    |    |    |    |
    Bonus at 2K, 10K and 30K        |    |    |    |    |Off |Off |    |    |
    Bonus at 4K, 15K and 40K       $|    |    |    |    |Off | On |    |    |
    Bonus at 6K, 20K and 50K        |    |    |    |    | On |Off |    |    |
    No bonus airplanes              |    |    |    |    | On | On |    |    |
                                    |    |    |    |    |    |    |    |    |
    2 aiplanes per game             |    |    |Off |Off |    |    |    |    |
    3 airplanes per game           $|    |    |Off | On |    |    |    |    |
    4 airplanes per game            |    |    | On |Off |    |    |    |    |
    5 airplanes per game            |    |    | On | On |    |    |    |    |
                                    |    |    |    |    |    |    |    |    |
    1-play minimum                 $|    |Off |    |    |    |    |    |    |
    2-play minimum                  |    | On |    |    |    |    |    |    |
                                    |    |    |    |    |    |    |    |    |
    Self-adj. game difficulty: on  $|Off |    |    |    |    |    |    |    |
    Self-adj. game difficulty: off  | On |    |    |    |    |    |    |    |
                                    -----------------------------------------

      If self-adjusting game difficulty feature is
    turned on, the program strives to maintain the
    following average game lengths (in seconds):

                                            Airplanes per game:
         Bonus airplane granted at:          2   3     4     5
    2,000, 10,000 and 30,000 points         90  105$  120   135
    4,000, 15,000 and 40,000 points         75   90   105   120
    6,000, 20,000 and 50,000 points         60   75    90   105
                 No bonus airplanes         45   60    75    90



    Switch at position M10
                                      8    7    6    5    4    3    2    1
                                    _________________________________________
        50  PER PLAY                |    |    |    |    |    |    |    |    |
     Straight 25  Door:             |    |    |    |    |    |    |    |    |
    No Bonus Coins                  |Off |Off |Off |Off |Off |Off | On | On |
    Bonus $1= 3 plays               |Off | On | On |Off |Off |Off | On | On |
    Bonus $1= 3 plays, 75 = 2 plays |Off |Off | On |Off |Off |Off | On | On |
                                    |    |    |    |    |    |    |    |    |
     25 /$1 Door or 25 /25 /$1 Door |    |    |    |    |    |    |    |    |
    No Bonus Coins                  |Off |Off |Off |Off |Off | On | On | On |
    Bonus $1= 3 plays               |Off | On | On |Off |Off | On | On | On |
    Bonus $1= 3 plays, 75 = 2 plays |Off |Off | On |Off |Off | On | On | On |
                                    |    |    |    |    |    |    |    |    |
        25  PER PLAY                |    |    |    |    |    |    |    |    |
     Straight 25  Door:             |    |    |    |    |    |    |    |    |
    No Bonus Coins                  |Off |Off |Off |Off |Off |Off | On |Off |
    Bonus 50 = 3 plays              |Off |Off | On |Off |Off |Off | On |Off |
    Bonus $1= 5 plays               |Off | On |Off |Off |Off |Off | On |Off |
                                    |    |    |    |    |    |    |    |    |
     25 /$1 Door or 25 /25 /$1 Door |    |    |    |    |    |    |    |    |
    No Bonus Coins                  |Off |Off |Off |Off |Off | On | On |Off |
    Bonus 50 = 3 plays              |Off |Off | On |Off |Off | On | On |Off |
    Bonus $1= 5 plays               |Off | On |Off |Off |Off | On | On |Off |
                                    -----------------------------------------

    Switch at position L11
                                                          1    2    3    4
                                                        _____________________
    All 3 mechs same denomination                       | On | On |    |    |
    Left and Center same, right different denomination  | On |Off |    |    |
    Right and Center same, left differnnt denomination  |Off | On |    |    |
    All different denominations                         |Off |Off |    |    |
                                                    ---------------------

***************************************************************************/

#include "driver.h"
#include "vidhrdw/vector.h"
#include "vidhrdw/avgdvg.h"
#include "machine/mathbox.h"
#include "machine/atari_vg.h"
#include "sound/pokey.h"
#include "sound/custom.h"
#include "artwork.h"
#include "bzone.h"

#define IN0_3KHZ (1<<7)
#define IN0_VG_HALT (1<<6)


static UINT8 analog_data;

UINT8 rb_input_select;



/*************************************
 *
 *  Save state registration
 *
 *************************************/

static MACHINE_START( bzone )
{
	state_save_register_global(analog_data);
	return 0;
}


static MACHINE_START( redbaron )
{
	state_save_register_global(analog_data);
	state_save_register_global(rb_input_select);
	return 0;
}



/*************************************
 *
 *  Interrupt handling
 *
 *************************************/

static INTERRUPT_GEN( bzone_interrupt )
{
	if (readinputport(0) & 0x10)
		cpunum_set_input_line(0, INPUT_LINE_NMI, PULSE_LINE);
}



/*************************************
 *
 *  Battlezone input ports
 *
 *************************************/

READ8_HANDLER( bzone_IN0_r )
{
	int res;

	res = readinputportbytag("IN0");

	if (activecpu_gettotalcycles() & 0x100)
		res |= IN0_3KHZ;
	else
		res &= ~IN0_3KHZ;

	if (avgdvg_done())
		res |= IN0_VG_HALT;
	else
		res &= ~IN0_VG_HALT;

	return res;
}


static WRITE8_HANDLER( bzone_coin_counter_w )
{
	coin_counter_w(offset,data);
}



/*************************************
 *
 *  Red Baron input ports
 *
 *************************************/

static READ8_HANDLER( redbaron_joy_r )
{
	return readinputport(rb_input_select ? 5 : 6);
}



/*************************************
 *
 *  Battle Zone overlay
 *
 *************************************/

OVERLAY_START( bzone_overlay )
	OVERLAY_RECT( 0.0, 0.0, 1.0, 0.2, MAKE_ARGB(0x04,0xff,0x20,0x20) )
	OVERLAY_RECT( 0.0, 0.2, 1.0, 1.0, MAKE_ARGB(0x04,0x20,0xff,0x20) )
OVERLAY_END



/*************************************
 *
 *  Main CPU memory handlers
 *
 *************************************/

static ADDRESS_MAP_START( bzone_map, ADDRESS_SPACE_PROGRAM, 8 )
	ADDRESS_MAP_FLAGS( AMEF_ABITS(15) )
	AM_RANGE(0x0000, 0x03ff) AM_RAM
	AM_RANGE(0x0800, 0x0800) AM_READ(bzone_IN0_r)    /* IN0 */
	AM_RANGE(0x0a00, 0x0a00) AM_READ(input_port_1_r)	/* DSW1 */
	AM_RANGE(0x0c00, 0x0c00) AM_READ(input_port_2_r)	/* DSW2 */
	AM_RANGE(0x1000, 0x1000) AM_WRITE(bzone_coin_counter_w)
	AM_RANGE(0x1200, 0x1200) AM_WRITE(avgdvg_go_w)
	AM_RANGE(0x1400, 0x1400) AM_WRITE(watchdog_reset_w)
	AM_RANGE(0x1600, 0x1600) AM_WRITE(avgdvg_reset_w)
	AM_RANGE(0x1800, 0x1800) AM_READ(mb_status_r)
	AM_RANGE(0x1810, 0x1810) AM_READ(mb_lo_r)
	AM_RANGE(0x1818, 0x1818) AM_READ(mb_hi_r)
	AM_RANGE(0x1820, 0x182f) AM_READWRITE(pokey1_r, pokey1_w)
	AM_RANGE(0x1840, 0x1840) AM_WRITE(bzone_sounds_w)
	AM_RANGE(0x1860, 0x187f) AM_WRITE(mb_go_w)
	AM_RANGE(0x2000, 0x2fff) AM_RAM AM_BASE(&vectorram) AM_SIZE(&vectorram_size) AM_REGION(REGION_CPU1, 0x2000)
	AM_RANGE(0x3000, 0x7fff) AM_ROM
ADDRESS_MAP_END

static ADDRESS_MAP_START( redbaron_map, ADDRESS_SPACE_PROGRAM, 8 )
	ADDRESS_MAP_FLAGS( AMEF_ABITS(15) )
	AM_RANGE(0x0000, 0x03ff) AM_RAM
	AM_RANGE(0x0800, 0x0800) AM_READ(bzone_IN0_r)    /* IN0 */
	AM_RANGE(0x0a00, 0x0a00) AM_READ(input_port_1_r)	/* DSW1 */
	AM_RANGE(0x0c00, 0x0c00) AM_READ(input_port_2_r)	/* DSW2 */
	AM_RANGE(0x1000, 0x1000) AM_WRITE(MWA8_NOP)			/* coin out */
	AM_RANGE(0x1200, 0x1200) AM_WRITE(avgdvg_go_w)
	AM_RANGE(0x1400, 0x1400) AM_WRITE(watchdog_reset_w)
	AM_RANGE(0x1600, 0x1600) AM_WRITE(avgdvg_reset_w)
	AM_RANGE(0x1800, 0x1800) AM_READ(mb_status_r)
	AM_RANGE(0x1802, 0x1802) AM_READ(input_port_4_r)	/* IN4 */
	AM_RANGE(0x1804, 0x1804) AM_READ(mb_lo_r)
	AM_RANGE(0x1806, 0x1806) AM_READ(mb_hi_r)
	AM_RANGE(0x1808, 0x1808) AM_WRITE(redbaron_sounds_w)	/* and select joystick pot also */
	AM_RANGE(0x180a, 0x180a) AM_WRITE(MWA8_NOP)			/* sound reset, yet todo */
	AM_RANGE(0x180c, 0x180c) AM_WRITE(atari_vg_earom_ctrl_w)
	AM_RANGE(0x1810, 0x181f) AM_READWRITE(pokey1_r, pokey1_w)
	AM_RANGE(0x1820, 0x185f) AM_READWRITE(atari_vg_earom_r, atari_vg_earom_w)
	AM_RANGE(0x1860, 0x187f) AM_WRITE(mb_go_w)
	AM_RANGE(0x2000, 0x2fff) AM_RAM AM_BASE(&vectorram) AM_SIZE(&vectorram_size) AM_REGION(REGION_CPU1, 0x2000)
	AM_RANGE(0x3000, 0x7fff) AM_ROM
ADDRESS_MAP_END



/*************************************
 *
 *  Port definitions
 *
 *************************************/

#define BZONEIN0\
	PORT_START_TAG("IN0")\
	PORT_BIT ( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )\
	PORT_BIT ( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )\
	PORT_BIT ( 0x0c, IP_ACTIVE_LOW, IPT_UNUSED )\
	PORT_SERVICE( 0x10, IP_ACTIVE_LOW )\
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_SERVICE ) PORT_NAME("Diagnostic Step") PORT_CODE(KEYCODE_F1)\
 	/* bit 6 is the VG HALT bit. We set it to "low" */\
	/* per default (busy vector processor). */\
 	/* handled by bzone_IN0_r() */\
	PORT_BIT ( 0x40, IP_ACTIVE_HIGH, IPT_SPECIAL )\
	/* bit 7 is tied to a 3kHz clock */\
 	/* handled by bzone_IN0_r() */\
  	PORT_BIT ( 0x80, IP_ACTIVE_HIGH, IPT_SPECIAL )


#define BZONEDSW0\
	PORT_START_TAG("DSW0")\
	PORT_DIPNAME(0x03, 0x01, DEF_STR( Lives ) )\
	PORT_DIPSETTING (  0x00, "2" )\
	PORT_DIPSETTING (  0x01, "3" )\
	PORT_DIPSETTING (  0x02, "4" )\
	PORT_DIPSETTING (  0x03, "5" )\
	PORT_DIPNAME(0x0c, 0x04, "Missile appears at" )\
	PORT_DIPSETTING (  0x00, "5000" )\
	PORT_DIPSETTING (  0x04, "10000" )\
	PORT_DIPSETTING (  0x08, "20000" )\
	PORT_DIPSETTING (  0x0c, "30000" )\
	PORT_DIPNAME(0x30, 0x10, DEF_STR( Bonus_Life ) )\
	PORT_DIPSETTING (  0x10, "15k and 100k" )\
	PORT_DIPSETTING (  0x20, "20k and 100k" )\
	PORT_DIPSETTING (  0x30, "50k and 100k" )\
	PORT_DIPSETTING (  0x00, DEF_STR( None ) )\
	PORT_DIPNAME(0xc0, 0x00, DEF_STR( Language ) )\
	PORT_DIPSETTING (  0x00, DEF_STR( English ))\
	PORT_DIPSETTING (  0x40, DEF_STR( German ))\
	PORT_DIPSETTING (  0x80, DEF_STR( French ))\
  	PORT_DIPSETTING (  0xc0, DEF_STR( Spanish ))

#define BZONEDSW1\
	PORT_START_TAG("DSW1")\
	PORT_DIPNAME(0x03, 0x02, DEF_STR( Coinage ) )\
	PORT_DIPSETTING (  0x03, DEF_STR( 2C_1C ) )\
	PORT_DIPSETTING (  0x02, DEF_STR( 1C_1C ) )\
	PORT_DIPSETTING (  0x01, DEF_STR( 1C_2C ) )\
	PORT_DIPSETTING (  0x00, DEF_STR( Free_Play ) )\
	PORT_DIPNAME(0x0c, 0x00, DEF_STR( Coin_B ) )\
	PORT_DIPSETTING (  0x00, "*1" )\
	PORT_DIPSETTING (  0x04, "*4" )\
	PORT_DIPSETTING (  0x08, "*5" )\
	PORT_DIPSETTING (  0x0c, "*6" )\
	PORT_DIPNAME(0x10, 0x00, DEF_STR( Coin_A ) )\
	PORT_DIPSETTING (  0x00, "*1" )\
	PORT_DIPSETTING (  0x10, "*2" )\
	PORT_DIPNAME(0xe0, 0x00, "Bonus Coins" )\
	PORT_DIPSETTING (  0x00, DEF_STR( None ) )\
	PORT_DIPSETTING (  0x20, "3 credits/2 coins" )\
	PORT_DIPSETTING (  0x40, "5 credits/4 coins" )\
	PORT_DIPSETTING (  0x60, "6 credits/4 coins" )\
  	PORT_DIPSETTING (  0x80, "6 credits/5 coins" )

INPUT_PORTS_START( bzone )
BZONEIN0
BZONEDSW0
BZONEDSW1

	PORT_START_TAG("IN3")
  	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICKRIGHT_DOWN ) PORT_2WAY
  	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICKRIGHT_UP ) PORT_2WAY
  	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICKLEFT_DOWN ) PORT_2WAY
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICKLEFT_UP ) PORT_2WAY
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON3 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNUSED )
INPUT_PORTS_END


INPUT_PORTS_START( redbaron )
BZONEIN0
	PORT_START_TAG("DSW0")
	/* See the table above if you are really interested */
	PORT_DIPNAME(0xff, 0xfd, DEF_STR( Coinage ) )
	PORT_DIPSETTING (  0xfd, DEF_STR( Normal ) )

	PORT_START_TAG("DSW1")
	PORT_DIPNAME(0x03, 0x03, DEF_STR( Language ) )
	PORT_DIPSETTING (  0x00, DEF_STR( German ) )
	PORT_DIPSETTING (  0x01, DEF_STR( French ) )
	PORT_DIPSETTING (  0x02, DEF_STR( Spanish ) )
	PORT_DIPSETTING (  0x03, DEF_STR( English ) )
	PORT_DIPNAME(0x0c, 0x04, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING (  0x0c, "2k 10k 30k" )
	PORT_DIPSETTING (  0x08, "4k 15k 40k" )
	PORT_DIPSETTING (  0x04, "6k 20k 50k" )
	PORT_DIPSETTING (  0x00, DEF_STR( None ) )
	PORT_DIPNAME(0x30, 0x20, DEF_STR( Lives ) )
	PORT_DIPSETTING (  0x30, "2" )
	PORT_DIPSETTING (  0x20, "3" )
	PORT_DIPSETTING (  0x10, "4" )
	PORT_DIPSETTING (  0x00, "5" )
	PORT_DIPNAME(0x40, 0x40, "One Play Minimum" )
	PORT_DIPSETTING (  0x40, DEF_STR( Off ) )
	PORT_DIPSETTING (  0x00, DEF_STR( On ) )
	PORT_DIPNAME(0x80, 0x80, "Self Adjust Diff" )
	PORT_DIPSETTING (  0x80, DEF_STR( Off ) )
	PORT_DIPSETTING (  0x00, DEF_STR( On ) )

	/* IN3 - the real machine reads either the X or Y axis from this port */
	/* Instead, we use the two fake 5 & 6 ports and bank-switch the proper */
	/* value based on the lsb of the byte written to the sound port */
	PORT_START_TAG("IN3")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT ) PORT_4WAY
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT ) PORT_4WAY
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP ) PORT_4WAY
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN ) PORT_4WAY

	PORT_START_TAG("IN4")	/* Misc controls */
	PORT_BIT( 0x3f, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_BUTTON1 )

	/* These 2 are fake - they are bank-switched from reads to IN3 */
	/* Red Baron doesn't seem to use the full 0-255 range. */
	PORT_START_TAG("FAKE1")	/* IN5 */
	PORT_BIT( 0xff, 0x80, IPT_AD_STICK_X ) PORT_MINMAX(64,192) PORT_SENSITIVITY(25) PORT_KEYDELTA(10)

	PORT_START_TAG("FAKE2")	/* IN6 */
	PORT_BIT( 0xff, 0x80, IPT_AD_STICK_Y ) PORT_MINMAX(64,192) PORT_SENSITIVITY(25) PORT_KEYDELTA(10)
INPUT_PORTS_END


INPUT_PORTS_START( bradley )
BZONEIN0
BZONEDSW0
BZONEDSW1

	PORT_START_TAG("IN3")
	PORT_BIT( 0x1f, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT( 0xc0, IP_ACTIVE_HIGH, IPT_UNUSED )

	PORT_START_TAG("1808")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_NAME("Armor Piercing (Single Shot)") PORT_CODE(KEYCODE_A)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_NAME("High Explosive (Single Shot)") PORT_CODE(KEYCODE_Z)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON4 ) PORT_NAME("Armor Piercing (Low Rate)") PORT_CODE(KEYCODE_S)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON5 ) PORT_NAME("High Explosive (Low Rate)") PORT_CODE(KEYCODE_X)
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON6 ) PORT_NAME("Armor Piercing (High Rate)") PORT_CODE(KEYCODE_D)
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON7 ) PORT_NAME("High Explosive (High Rate)") PORT_CODE(KEYCODE_C)

	PORT_START_TAG("1809")
	PORT_BIT( 0x03, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON9 ) PORT_NAME("Select TOW Missiles") PORT_CODE(KEYCODE_T)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON8 ) PORT_NAME("7.62 mm Machine Gun") PORT_CODE(KEYCODE_V)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON10 ) PORT_NAME("Magnification Toggle") PORT_CODE(KEYCODE_M) PORT_TOGGLE
	PORT_BIT( 0xe0, IP_ACTIVE_HIGH, IPT_UNUSED )

	PORT_START_TAG("AN0")	/* analog 0 = turret rotation */
	PORT_BIT( 0xff, 0x88, IPT_AD_STICK_X ) PORT_MINMAX(0x48,0xc8) PORT_SENSITIVITY(25) PORT_KEYDELTA(10)

	PORT_START_TAG("AN1")	/* analog 1 = turret elevation */
	PORT_BIT( 0xff, 0x86, IPT_AD_STICK_Y ) PORT_MINMAX(0x46,0xc6) PORT_SENSITIVITY(25) PORT_KEYDELTA(10)

	PORT_START_TAG("AN2")	/* analog 2 = shell firing range hack removed, now uses Z */
	PORT_BIT( 0xff, 0x80, IPT_AD_STICK_Z ) PORT_MINMAX(0x10,0xf0) PORT_SENSITIVITY(25) PORT_KEYDELTA(10) PORT_CENTERDELTA(0) PORT_REVERSE
INPUT_PORTS_END



/*************************************
 *
 *  Sound interfaces
 *
 *************************************/

static struct POKEYinterface bzone_pokey_interface =
{
	{ 0 },
	input_port_3_r
};


static struct CustomSound_interface bzone_custom_interface =
{
	bzone_sh_start
};


static struct POKEYinterface redbaron_pokey_interface =
{
	{ 0 },
	redbaron_joy_r
};


static struct CustomSound_interface redbaron_custom_interface =
{
	redbaron_sh_start
};



/*************************************
 *
 *  Machine driver
 *
 *************************************/

static MACHINE_DRIVER_START( bzone )

	/* basic machine hardware */
	MDRV_CPU_ADD_TAG("main", M6502, 1500000)
	MDRV_CPU_PROGRAM_MAP(bzone_map,0)
	MDRV_CPU_VBLANK_INT(bzone_interrupt,6) /* 4.1ms */

	MDRV_FRAMES_PER_SECOND(40)

	MDRV_MACHINE_START(bzone)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_VECTOR | VIDEO_RGB_DIRECT)
	MDRV_SCREEN_SIZE(400, 300)
	MDRV_VISIBLE_AREA(0, 580, 0, 400)
	MDRV_PALETTE_LENGTH(32768)

	MDRV_PALETTE_INIT(avg_white)
	MDRV_VIDEO_START(avg_bzone)
	MDRV_VIDEO_UPDATE(vector)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")

	MDRV_SOUND_ADD_TAG("pokey",  POKEY, 1500000)
	MDRV_SOUND_CONFIG(bzone_pokey_interface)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.0)

	MDRV_SOUND_ADD_TAG("custom", CUSTOM, 0)
	MDRV_SOUND_CONFIG(bzone_custom_interface)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.50)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( bradley )

	/* basic machine hardware */
	MDRV_IMPORT_FROM(bzone)

	/* sound hardware */
	MDRV_SOUND_REPLACE("pokey", POKEY, 1500000)
	MDRV_SOUND_CONFIG(bzone_pokey_interface)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.0)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( redbaron )

	/* basic machine hardware */
	MDRV_IMPORT_FROM(bzone)
	MDRV_CPU_MODIFY("main")
	MDRV_CPU_PROGRAM_MAP(redbaron_map,0)
	MDRV_CPU_VBLANK_INT(bzone_interrupt,4) /* 5.4ms */

	MDRV_MACHINE_START(redbaron)

	MDRV_FRAMES_PER_SECOND(45)
	MDRV_NVRAM_HANDLER(atari_vg)

	/* video hardware */
	MDRV_VISIBLE_AREA(0, 520, 0, 400)

	MDRV_VIDEO_START(avg_redbaron)

	/* sound hardware */
	MDRV_SOUND_REPLACE("pokey", POKEY, 1500000)
	MDRV_SOUND_CONFIG(redbaron_pokey_interface)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.0)

	MDRV_SOUND_REPLACE("custom", CUSTOM, 0)
	MDRV_SOUND_CONFIG(redbaron_custom_interface)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.50)
MACHINE_DRIVER_END



/*************************************
 *
 *  ROM definitions
 *
 *************************************/

ROM_START( bzone )
	ROM_REGION( 0x8000, REGION_CPU1, 0 )	/* 32k for code */
	ROM_LOAD( "036414.01",  0x5000, 0x0800, CRC(efbc3fa0) SHA1(6d284fab34b09dde8aa0df7088711d4723f07970) )
	ROM_LOAD( "036413.01",  0x5800, 0x0800, CRC(5d9d9111) SHA1(42638cff53a9791a0f18d316f62a0ea8eea4e194) )
	ROM_LOAD( "036412.01",  0x6000, 0x0800, CRC(ab55cbd2) SHA1(6bbb8316d9f8588ea0893932f9174788292b8edc) )
	ROM_LOAD( "036411.01",  0x6800, 0x0800, CRC(ad281297) SHA1(54c5e06b2e69eb731a6c9b1704e4340f493e7ea5) )
	ROM_LOAD( "036410.01",  0x7000, 0x0800, CRC(0b7bfaa4) SHA1(33ae0f68b4e2eae9f3aecbee2d0b29003ce460b2) )
	ROM_LOAD( "036409.01",  0x7800, 0x0800, CRC(1e14e919) SHA1(448fab30535e6fad7e0ab4427bc06bbbe075e797) )
	/* Mathbox ROMs */
	ROM_LOAD( "036422.01",  0x3000, 0x0800, CRC(7414177b) SHA1(147d97a3b475e738ce00b1a7909bbd787ad06eda) )
	ROM_LOAD( "036421.01",  0x3800, 0x0800, CRC(8ea8f939) SHA1(b71e0ab0e220c3e64dc2b094c701fb1a960b64e4) )
ROM_END


ROM_START( bzone2 )
	ROM_REGION( 0x8000, REGION_CPU1, 0 )	/* 32k for code */
	ROM_LOAD( "036414a.01", 0x5000, 0x0800, CRC(13de36d5) SHA1(40e356ddc5c042bc1ce0b71f51e8b6de72daf1e4) )
	ROM_LOAD( "036413.01",  0x5800, 0x0800, CRC(5d9d9111) SHA1(42638cff53a9791a0f18d316f62a0ea8eea4e194) )
	ROM_LOAD( "036412.01",  0x6000, 0x0800, CRC(ab55cbd2) SHA1(6bbb8316d9f8588ea0893932f9174788292b8edc) )
	ROM_LOAD( "036411.01",  0x6800, 0x0800, CRC(ad281297) SHA1(54c5e06b2e69eb731a6c9b1704e4340f493e7ea5) )
	ROM_LOAD( "036410.01",  0x7000, 0x0800, CRC(0b7bfaa4) SHA1(33ae0f68b4e2eae9f3aecbee2d0b29003ce460b2) )
	ROM_LOAD( "036409.01",  0x7800, 0x0800, CRC(1e14e919) SHA1(448fab30535e6fad7e0ab4427bc06bbbe075e797) )
	/* Mathbox ROMs */
	ROM_LOAD( "036422.01",  0x3000, 0x0800, CRC(7414177b) SHA1(147d97a3b475e738ce00b1a7909bbd787ad06eda) )
	ROM_LOAD( "036421.01",  0x3800, 0x0800, CRC(8ea8f939) SHA1(b71e0ab0e220c3e64dc2b094c701fb1a960b64e4) )
ROM_END


ROM_START( bzonec ) /* cocktail version */
	ROM_REGION( 0x8000, REGION_CPU1, 0 )	/* 32k for code */
	ROM_LOAD( "bz1g4800",   0x4800, 0x0800, CRC(e228dd64) SHA1(247c788b4ccadf6c1e9201ad4f31d55c0036ff0f) )
	ROM_LOAD( "bz1f5000",   0x5000, 0x0800, CRC(dddfac9a) SHA1(e6f2761902e1ffafba437a1117e9ba40f116087d) )
	ROM_LOAD( "bz1e5800",   0x5800, 0x0800, CRC(7e00e823) SHA1(008e491a8074dac16e56c3aedec32d4b340158ce) )
	ROM_LOAD( "bz1d6000",   0x6000, 0x0800, CRC(c0f8c068) SHA1(66fff6b493371f0015c21b06b94637db12deced2) )
	ROM_LOAD( "bz1c6800",   0x6800, 0x0800, CRC(5adc64bd) SHA1(4574e4fe375d4ab3151a988235efa11e8744e2c6) )
	ROM_LOAD( "bz1b7000",   0x7000, 0x0800, CRC(ed8a860e) SHA1(316a3c4870ba44bb3e9cb9fc5200eb081318facf) )
	ROM_LOAD( "bz1a7800",   0x7800, 0x0800, CRC(04babf45) SHA1(a59da5ff49fc398ca4a948e28f05250af776b898) )
	/* Mathbox ROMs */
	ROM_LOAD( "036422.01",  0x3000, 0x0800, CRC(7414177b) SHA1(147d97a3b475e738ce00b1a7909bbd787ad06eda) )	// bz3a3000
	ROM_LOAD( "bz3b3800",   0x3800, 0x0800, CRC(76cf57f6) SHA1(1b8f3fcd664ed04ce60d94fdf27e56b20d52bdbd) )
ROM_END


ROM_START( bradley )
	ROM_REGION( 0x8000, REGION_CPU1, 0 )	/* 32k for code */
	ROM_LOAD( "btc1.bin",   0x4000, 0x0800, CRC(0bb8e049) SHA1(158517ff9a4e8ae7270ccf7eab87bf77427a4a8c) )
	ROM_LOAD( "btd1.bin",   0x4800, 0x0800, CRC(9e0566d4) SHA1(f14aa5c3d14136c5e9a317004f82d44a8d5d6815) )
	ROM_LOAD( "bte1.bin",   0x5000, 0x0800, CRC(64ee6a42) SHA1(33d0713ed2a1f4c1c443dce1f053321f2c279293) )
	ROM_LOAD( "bth1.bin",   0x5800, 0x0800, CRC(baab67be) SHA1(77ad1935bf252b401bb6bbb57bd2ed66a85f0a6d) )
	ROM_LOAD( "btj1.bin",   0x6000, 0x0800, CRC(036adde4) SHA1(16a9fcf98a2aa287e0b7a665b88c9c67377a1203) )
	ROM_LOAD( "btk1.bin",   0x6800, 0x0800, CRC(f5c2904e) SHA1(f2cbf720c4f5ce0fc912dbc2f0445cb2c51ffac1) )
	ROM_LOAD( "btlm.bin",   0x7000, 0x0800, CRC(7d0313bf) SHA1(17e3d8df62b332cf889133f1943c8f27308df027) )
	ROM_LOAD( "btn1.bin",   0x7800, 0x0800, CRC(182c8c64) SHA1(511af60d86551291d2dc28442970b4863c62624a) )
	/* Mathbox ROMs */
	ROM_LOAD( "btb3.bin",   0x3000, 0x0800, CRC(88206304) SHA1(6a2e2ff35a929acf460f244db7968f3978b1d239) )
	ROM_LOAD( "bta3.bin",   0x3800, 0x0800, CRC(d669d796) SHA1(ad606882320cd13612c7962d4718680fe5a35dd3) )
ROM_END


ROM_START( redbaron )
	ROM_REGION( 0x8000, REGION_CPU1, 0 )	/* 32k for code */
	ROM_LOAD( "037587.01",  0x4800, 0x0800, CRC(60f23983) SHA1(7a9e5380bf49bf50a2d8ab0e0bd1ba3ac8efde24) )
	ROM_CONTINUE(           0x5800, 0x0800 )
	ROM_LOAD( "037000.01e", 0x5000, 0x0800, CRC(69bed808) SHA1(27d99efc74113cdcbbf021734b8a5a5fdb78c04c) )
	ROM_LOAD( "036998.01e", 0x6000, 0x0800, CRC(d1104dd7) SHA1(0eab47cb45ede9dcc4dd7498dcf3a8d8194460b4) )
	ROM_LOAD( "036997.01e", 0x6800, 0x0800, CRC(7434acb4) SHA1(c950c4c12ab556b5051ad356ab4a0ed6b779ba1f) )
	ROM_LOAD( "036996.01e", 0x7000, 0x0800, CRC(c0e7589e) SHA1(c1aedc95966afffd860d7e0009d5a43e8b292036) )
	ROM_LOAD( "036995.01e", 0x7800, 0x0800, CRC(ad81d1da) SHA1(8bd66e5f34fc1c75f31eb6b168607e52aa3aa4df) )
	/* Mathbox ROMs */
	ROM_LOAD( "037006.01e", 0x3000, 0x0800, CRC(9fcffea0) SHA1(69b76655ee75742fcaa0f39a4a1cf3aa58088343) )
	ROM_LOAD( "037007.01e", 0x3800, 0x0800, CRC(60250ede) SHA1(9c48952bd69863bee0c6dce09f3613149e0151ef) )
ROM_END



/*************************************
 *
 *  Driver initialization
 *
 *************************************/

static READ8_HANDLER( analog_data_r )
{
	return analog_data;
}


static WRITE8_HANDLER( analog_select_w )
{
	if (offset <= 2)
		analog_data = readinputport(6 + offset);
}


static DRIVER_INIT( bzone )
{
	artwork_set_overlay(bzone_overlay);
}


static DRIVER_INIT( bradley )
{
	memory_install_read8_handler(0, ADDRESS_SPACE_PROGRAM, 0x400, 0x7ff, 0, 0, MRA8_RAM);
	memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM, 0x400, 0x7ff, 0, 0, MWA8_RAM);

	memory_install_read8_handler(0, ADDRESS_SPACE_PROGRAM, 0x1808, 0x1808, 0, 0, input_port_4_r);
	memory_install_read8_handler(0, ADDRESS_SPACE_PROGRAM, 0x1809, 0x1809, 0, 0, input_port_5_r);
	memory_install_read8_handler(0, ADDRESS_SPACE_PROGRAM, 0x180a, 0x180a, 0, 0, analog_data_r);
	memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM, 0x1848, 0x1850, 0, 0, analog_select_w);
}


static DRIVER_INIT( redbaron )
{
	OVERLAY_START( redbaron_overlay )
		OVERLAY_RECT( 0.0, 0.0, 1.0, 1.0, MAKE_ARGB(0x04,0x88,0xff,0xff) )
	OVERLAY_END

	artwork_set_overlay(redbaron_overlay);
}



/*************************************
 *
 *  Game drivers
 *
 *************************************/

GAME( 1980, bzone,    0,     bzone,    bzone,    bzone,    ROT0, "Atari", "Battle Zone (set 1)", 0 )
GAME( 1980, bzone2,   bzone, bzone,    bzone,    bzone,    ROT0, "Atari", "Battle Zone (set 2)", 0 )
GAME( 1980, bzonec,   bzone, bzone,    bzone,    bzone,    ROT0, "Atari", "Battle Zone (cocktail)", GAME_NO_COCKTAIL )
GAME( 1980, bradley,  0,     bradley,  bradley,  bradley,  ROT0, "Atari", "Bradley Trainer", 0 )
GAME( 1980, redbaron, 0,     redbaron, redbaron, redbaron, ROT0, "Atari", "Red Baron", 0 )
