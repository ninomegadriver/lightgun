/***************************************************************************

    Atari Asteroids hardware

    Games supported:
        * Asteroids
        * Asteroids Deluxe
        * Lunar Lander

    Known bugs:
        * the ERROR message in Asteroids Deluxe self test is related to a pokey problem

    Asteroids-deluxe state-prom added by HIGHWAYMAN.
    The prom pcb location is:C8 and is 256x4
    (i need to update the dump, this one is read in 8bit-mode)
****************************************************************************

    Asteroids Memory Map (preliminary)

    Asteroids settings:

    0 = OFF  1 = ON  X = Don't Care  $ = Atari suggests


    8 SWITCH DIP
    87654321
    --------
    XXXXXX11   English
    XXXXXX10   German
    XXXXXX01   French
    XXXXXX00   Spanish
    XXXXX1XX   4-ship game
    XXXXX0XX   3-ship game
    11XXXXXX   Free Play
    10XXXXXX   1 Coin  for 2 Plays
    01XXXXXX   1 Coin  for 1 Play
    00XXXXXX   2 Coins for 1 Play

    Asteroids Deluxe settings:

    0 = OFF  1 = ON  X = Don't Care  $ = Atari suggests


    8 SWITCH DIP (R5)
    87654321
    --------
    XXXXXX11   English $
    XXXXXX10   German
    XXXXXX01   French
    XXXXXX00   Spanish
    XXXX11XX   2-4 ships
    XXXX10XX   3-5 ships $
    XXXX01XX   4-6 ships
    XXXX00XX   5-7 ships
    XXX1XXXX   1-play minimum $
    XXX0XXXX   2-play minimum
    XX1XXXXX   Easier gameplay for first 30000 points +
    XX0XXXXX   Hard gameplay throughout the game      +
    11XXXXXX   Bonus ship every 10,000 points $ !
    10XXXXXX   Bonus ship every 12,000 points !
    01XXXXXX   Bonus ship every 15,000 points !
    00XXXXXX   No bonus ships (adds one ship at game start)

    + only with the newer romset
    ! not "every", but "at", e.g. only once.

    Thanks to Gregg Woodcock for the info.

    8 SWITCH DIP (L8)
    87654321
    --------
    XXXXXX11   Free Play
    XXXXXX10   1 Coin = 2 Plays
    XXXXXX01   1 Coin = 1 Play
    XXXXXX00   2 Coins = 1 Play $
    XXXX11XX   Right coin mech * 1 $
    XXXX10XX   Right coin mech * 4
    XXXX01XX   Right coin mech * 5
    XXXX00XX   Right coin mech * 6
    XXX1XXXX   Center coin mech * 1 $
    XXX0XXXX   Center coin mech * 2
    111XXXXX   No bonus coins
    110XXXXX   For every 2 coins inserted, game logic adds 1 more coin
    101XXXXX   For every 4 coins inserted, game logic adds 1 more coin
    100XXXXX   For every 4 coins inserted, game logic adds 2 more coins $
    011XXXXX   For every 5 coins inserted, game logic adds 1 more coin

****************************************************************************

    Lunar Lander Memory Map (preliminary)

    Lunar Lander settings:

    0 = OFF  1 = ON  x = Don't Care  $ = Atari suggests


    8 SWITCH DIP (P8) with -01 ROMs on PCB
    87654321
    --------
    11xxxxxx   450 fuel units per coin
    10xxxxxx   600 fuel units per coin
    01xxxxxx   750 fuel units per coin  $
    00xxxxxx   900 fuel units per coin
    xxx0xxxx   Free play
    xxx1xxxx   Coined play as determined by toggles 7 & 8  $
    xxxx00xx   German instructions
    xxxx01xx   Spanish instructions
    xxxx10xx   French instructions
    xxxx11xx   English instructions  $
    xxxxxx11   Right coin == 1 credit/coin  $
    xxxxxx10   Right coin == 4 credit/coin
    xxxxxx01   Right coin == 5 credit/coin
    xxxxxx00   Right coin == 6 credit/coin
               (Left coin always registers 1 credit/coin)


    8 SWITCH DIP (P8) with -02 ROMs on PCB
    87654321
    --------
    11x1xxxx   450 fuel units per coin
    10x1xxxx   600 fuel units per coin
    01x1xxxx   750 fuel units per coin  $
    00x1xxxx   900 fuel units per coin
    11x0xxxx   1100 fuel units per coin
    10x0xxxx   1300 fuel units per coin
    01x0xxxx   1550 fuel units per coin
    00x0xxxx   1800 fuel units per coin
    xx0xxxxx   Free play
    xx1xxxxx   Coined play as determined by toggles 5, 7, & 8  $
    xxxx00xx   German instructions
    xxxx01xx   Spanish instructions
    xxxx10xx   French instructions
    xxxx11xx   English instructions  $
    xxxxxx11   Right coin == 1 credit/coin  $
    xxxxxx10   Right coin == 4 credit/coin
    xxxxxx01   Right coin == 5 credit/coin
    xxxxxx00   Right coin == 6 credit/coin
               (Left coin always registers 1 credit/coin)

***************************************************************************/

#include "driver.h"
#include "vidhrdw/vector.h"
#include "vidhrdw/avgdvg.h"
#include "machine/atari_vg.h"
#include "asteroid.h"
#include "sound/discrete.h"
#include "sound/pokey.h"



/*************************************
 *
 *  Coin counters
 *
 *************************************/

static WRITE8_HANDLER( astdelux_coin_counter_w )
{
	coin_counter_w(offset,data);
}



/*************************************
 *
 *  Land Lander LEDs/lamps
 *
 *************************************/

static WRITE8_HANDLER( llander_led_w )
{
	static const char *lampname[] =
	{
		"lamp0", "lamp1", "lamp2", "lamp3", "lamp4"
	};
    int i;

    for (i = 0; i < 5; i++)
		artwork_show(lampname[i], (data >> (4 - i)) & 1);
}




/*************************************
 *
 *  Main CPU memory handlers
 *
 *************************************/

static ADDRESS_MAP_START( asteroid_map, ADDRESS_SPACE_PROGRAM, 8 )
	ADDRESS_MAP_FLAGS( AMEF_ABITS(15) )
	AM_RANGE(0x0000, 0x03ff) AM_RAM
	AM_RANGE(0x2000, 0x2007) AM_READ(asteroid_IN0_r) /* IN0 */
	AM_RANGE(0x2400, 0x2407) AM_READ(asteroid_IN1_r) /* IN1 */
	AM_RANGE(0x2800, 0x2803) AM_READ(asteroid_DSW1_r) /* DSW1 */
	AM_RANGE(0x3000, 0x3000) AM_WRITE(avgdvg_go_w)
	AM_RANGE(0x3200, 0x3200) AM_WRITE(asteroid_bank_switch_w)
	AM_RANGE(0x3400, 0x3400) AM_WRITE(watchdog_reset_w)
	AM_RANGE(0x3600, 0x3600) AM_WRITE(asteroid_explode_w)
	AM_RANGE(0x3a00, 0x3a00) AM_WRITE(asteroid_thump_w)
	AM_RANGE(0x3c00, 0x3c05) AM_WRITE(asteroid_sounds_w)
	AM_RANGE(0x3e00, 0x3e00) AM_WRITE(asteroid_noise_reset_w)
	AM_RANGE(0x4000, 0x47ff) AM_RAM AM_BASE(&vectorram) AM_SIZE(&vectorram_size) AM_REGION(REGION_CPU1, 0x4000)
	AM_RANGE(0x5000, 0x57ff) AM_ROM				/* vector rom */
	AM_RANGE(0x6800, 0x7fff) AM_ROM
ADDRESS_MAP_END


static ADDRESS_MAP_START( astdelux_map, ADDRESS_SPACE_PROGRAM, 8 )
	ADDRESS_MAP_FLAGS( AMEF_ABITS(15) )
	AM_RANGE(0x0000, 0x03ff) AM_RAM
	AM_RANGE(0x2000, 0x2007) AM_READ(asteroid_IN0_r) /* IN0 */
	AM_RANGE(0x2400, 0x2407) AM_READ(asteroid_IN1_r) /* IN1 */
	AM_RANGE(0x2800, 0x2803) AM_READ(asteroid_DSW1_r) /* DSW1 */
	AM_RANGE(0x2c00, 0x2c0f) AM_READWRITE(pokey1_r, pokey1_w)
	AM_RANGE(0x2c40, 0x2c7f) AM_READ(atari_vg_earom_r)
	AM_RANGE(0x3000, 0x3000) AM_WRITE(avgdvg_go_w)
	AM_RANGE(0x3200, 0x323f) AM_WRITE(atari_vg_earom_w)
	AM_RANGE(0x3400, 0x3400) AM_WRITE(watchdog_reset_w)
	AM_RANGE(0x3600, 0x3600) AM_WRITE(asteroid_explode_w)
	AM_RANGE(0x3a00, 0x3a00) AM_WRITE(atari_vg_earom_ctrl_w)
	AM_RANGE(0x3c00, 0x3c01) AM_WRITE(astdelux_led_w)
	AM_RANGE(0x3c03, 0x3c03) AM_WRITE(astdelux_sounds_w)
	AM_RANGE(0x3c04, 0x3c04) AM_WRITE(astdelux_bank_switch_w)
	AM_RANGE(0x3c05, 0x3c07) AM_WRITE(astdelux_coin_counter_w)
	AM_RANGE(0x3e00, 0x3e00) AM_WRITE(asteroid_noise_reset_w)
	AM_RANGE(0x4000, 0x47ff) AM_RAM AM_BASE(&vectorram) AM_SIZE(&vectorram_size) AM_REGION(REGION_CPU1, 0x4000)
	AM_RANGE(0x4800, 0x57ff) AM_ROM				/* vector rom */
	AM_RANGE(0x6000, 0x7fff) AM_ROM
ADDRESS_MAP_END


static ADDRESS_MAP_START( llander_map, ADDRESS_SPACE_PROGRAM, 8 )
	ADDRESS_MAP_FLAGS( AMEF_ABITS(15) )
	AM_RANGE(0x0000, 0x00ff) AM_MIRROR(0x100) AM_RAM
	AM_RANGE(0x2000, 0x2000) AM_READ(llander_IN0_r) /* IN0 */
	AM_RANGE(0x2400, 0x2407) AM_READ(asteroid_IN1_r) /* IN1 */
	AM_RANGE(0x2800, 0x2803) AM_READ(asteroid_DSW1_r) /* DSW1 */
	AM_RANGE(0x2c00, 0x2c00) AM_READ(input_port_3_r) /* IN3 */
	AM_RANGE(0x3000, 0x3000) AM_WRITE(avgdvg_go_w)
	AM_RANGE(0x3200, 0x3200) AM_WRITE(llander_led_w)
	AM_RANGE(0x3400, 0x3400) AM_WRITE(watchdog_reset_w)
	AM_RANGE(0x3c00, 0x3c00) AM_WRITE(llander_sounds_w)
	AM_RANGE(0x3e00, 0x3e00) AM_WRITE(llander_snd_reset_w)
	AM_RANGE(0x4000, 0x47ff) AM_RAM AM_BASE(&vectorram) AM_SIZE(&vectorram_size) AM_REGION(REGION_CPU1, 0x4000)
	AM_RANGE(0x4800, 0x5fff) AM_ROM				/* vector rom */
	AM_RANGE(0x6000, 0x7fff) AM_ROM
ADDRESS_MAP_END



/*************************************
 *
 *  Port definitions
 *
 *************************************/

INPUT_PORTS_START( asteroid )
	PORT_START_TAG("IN0")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	/* Bit 2 and 3 are handled in the machine dependent part. */
	/* Bit 2 is the 3 KHz source and Bit 3 the VG_HALT bit    */
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_BUTTON3 )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 )
	PORT_BIT(0x20, IP_ACTIVE_HIGH, IPT_SERVICE ) PORT_NAME("Diagnostic Step") PORT_CODE(KEYCODE_F1)
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_TILT )
	PORT_SERVICE( 0x80, IP_ACTIVE_HIGH )

	PORT_START_TAG("IN1")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_COIN3 )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_BUTTON2 )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT ) PORT_2WAY
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT ) PORT_2WAY

	PORT_START_TAG("DSW")
	PORT_DIPNAME( 0x03, 0x00, DEF_STR( Language ) )
	PORT_DIPSETTING (	0x00, DEF_STR( English ) )
	PORT_DIPSETTING (	0x01, DEF_STR( German ) )
	PORT_DIPSETTING (	0x02, DEF_STR( French ) )
	PORT_DIPSETTING (	0x03, DEF_STR( Spanish ) )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Lives ) )
	PORT_DIPSETTING (	0x04, "3" )
	PORT_DIPSETTING (	0x00, "4" )
	PORT_DIPNAME( 0x08, 0x00, "Center Mech" ) /*Left same for 2-door mech*/
	PORT_DIPSETTING (	0x00, "X 1" )
	PORT_DIPSETTING (	0x08, "X 2" )
	PORT_DIPNAME( 0x30, 0x00, "Right Mech" )
	PORT_DIPSETTING (	0x00, "X 1" )
	PORT_DIPSETTING (	0x10, "X 4" )
	PORT_DIPSETTING (	0x20, "X 5" )
	PORT_DIPSETTING (	0x30, "X 6" )
	PORT_DIPNAME( 0xc0, 0x80, DEF_STR( Coinage ) )
	PORT_DIPSETTING (	0xc0, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING (	0x80, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING (	0x40, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING (	0x00, DEF_STR( Free_Play ) )
INPUT_PORTS_END


INPUT_PORTS_START( asteroib )
	PORT_START_TAG("IN0")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* resets */
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* resets */
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_TILT )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	/* Bit 7 is VG_HALT, handled in the machine dependent part */
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START_TAG("IN1")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_BUTTON2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_BUTTON1 )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT ) PORT_2WAY
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT ) PORT_2WAY

	PORT_START_TAG("DSW")
	PORT_DIPNAME( 0x03, 0x00, DEF_STR( Language ) )
	PORT_DIPSETTING (	0x00, DEF_STR( English ) )
	PORT_DIPSETTING (	0x01, DEF_STR( German ) )
	PORT_DIPSETTING (	0x02, DEF_STR( French ) )
	PORT_DIPSETTING (	0x03, DEF_STR( Spanish ) )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Lives ) )
	PORT_DIPSETTING (	0x04, "3" )
	PORT_DIPSETTING (	0x00, "4" )
	PORT_DIPNAME( 0x08, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING (	0x00, DEF_STR( Off ) )
	PORT_DIPSETTING (	0x08, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING (	0x00, DEF_STR( Off ) )
	PORT_DIPSETTING (	0x10, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING (	0x00, DEF_STR( Off ) )
	PORT_DIPSETTING (	0x20, DEF_STR( On ) )
	PORT_DIPNAME( 0xc0, 0x80, DEF_STR( Coinage ) )
	PORT_DIPSETTING (	0xc0, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING (	0x80, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING (	0x40, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING (	0x00, DEF_STR( Free_Play ) )

	PORT_START_TAG("HS") /* hyperspace */
	PORT_BIT( 0x7f, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON3 )
INPUT_PORTS_END


INPUT_PORTS_START( asterock )
	PORT_START_TAG("IN0")
	/* Bit 0 is VG_HALT, handled in the machine dependent part */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	/* Bit 2 is the 3 KHz source */
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON3 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_SERVICE ) PORT_NAME("Diagnostic Step") PORT_CODE(KEYCODE_F1)
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_TILT )
	PORT_SERVICE( 0x80, IP_ACTIVE_LOW )

	PORT_START_TAG("IN1")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_COIN3 )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_BUTTON2 )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT ) PORT_2WAY
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT ) PORT_2WAY

	PORT_START_TAG("DSW")
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Language ) )
	PORT_DIPSETTING(    0x00, DEF_STR( English ) )
	PORT_DIPSETTING(    0x01, DEF_STR( French ) )
	PORT_DIPSETTING(    0x02, DEF_STR( German ) )
	PORT_DIPSETTING(    0x03, DEF_STR( Italian ) )
	PORT_DIPNAME( 0x0c, 0x04, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x00, "2" )
	PORT_DIPSETTING(    0x04, "3" )
	PORT_DIPSETTING(    0x08, "4" )
	PORT_DIPSETTING(    0x0c, "5" )
	PORT_DIPNAME( 0x10, 0x00, "Records Table" )
	PORT_DIPSETTING(    0x00, DEF_STR( Normal ) )
	PORT_DIPSETTING(    0x10, "Special" )
	PORT_DIPNAME( 0x20, 0x00, "Coin Mode" )
	PORT_DIPSETTING (	0x00, DEF_STR( Normal ) )
	PORT_DIPSETTING (	0x20, "Special" )
	PORT_DIPNAME( 0xc0, 0x80, DEF_STR( Coinage ) )
	PORT_DIPSETTING (	0xc0, DEF_STR( 2C_1C ) )					PORT_CONDITION("DSW",0x20,PORTCOND_EQUALS,0x00)
	PORT_DIPSETTING (	0x80, DEF_STR( 1C_1C ) )					PORT_CONDITION("DSW",0x20,PORTCOND_EQUALS,0x00)
	PORT_DIPSETTING (	0x40, DEF_STR( 1C_2C ) )					PORT_CONDITION("DSW",0x20,PORTCOND_EQUALS,0x00)
//  PORT_DIPSETTING (   0x00, DEF_STR( 1C_1C ) )                    PORT_CONDITION("DSW",0x20,PORTCOND_EQUALS,0x00)
	PORT_DIPSETTING (	0xc0, "Coin A 2/1 Coin B 2/1 Coin C 1/1" )	PORT_CONDITION("DSW",0x20,PORTCOND_NOTEQUALS,0x00)
	PORT_DIPSETTING (	0x80, "Coin A 1/1 Coin B 1/1 Coin C 1/2" )	PORT_CONDITION("DSW",0x20,PORTCOND_NOTEQUALS,0x00)
	PORT_DIPSETTING (	0x40, "Coin A 1/2 Coin B 1/2 Coin C 1/4" )	PORT_CONDITION("DSW",0x20,PORTCOND_NOTEQUALS,0x00)
//  PORT_DIPSETTING (   0x00, "Coin A 1/1 Coin B 1/1 Coin C 1/2" )  PORT_CONDITION("DSW",0x20,PORTCOND_NOTEQUALS,0x00)
INPUT_PORTS_END


INPUT_PORTS_START( astdelux )
	PORT_START_TAG("IN0")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	/* Bit 2 and 3 are handled in the machine dependent part. */
	/* Bit 2 is the 3 KHz source and Bit 3 the VG_HALT bit    */
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_BUTTON3 )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_SERVICE ) PORT_NAME("Diagnostic Step") PORT_CODE(KEYCODE_F1)
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_TILT )
	PORT_SERVICE( 0x80, IP_ACTIVE_HIGH )

	PORT_START_TAG("IN1")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_COIN3 )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_BUTTON2 )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT ) PORT_2WAY
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT ) PORT_2WAY

	PORT_START_TAG("DSW1")
	PORT_DIPNAME( 0x03, 0x00, DEF_STR( Language ) )
	PORT_DIPSETTING (	0x00, DEF_STR( English ) )
	PORT_DIPSETTING (	0x01, DEF_STR( German ) )
	PORT_DIPSETTING (	0x02, DEF_STR( French ) )
	PORT_DIPSETTING (	0x03, DEF_STR( Spanish ) )
	PORT_DIPNAME( 0x0c, 0x04, DEF_STR( Lives ) )
	PORT_DIPSETTING (	0x00, "2-4" )
	PORT_DIPSETTING (	0x04, "3-5" )
	PORT_DIPSETTING (	0x08, "4-6" )
	PORT_DIPSETTING (	0x0c, "5-7" )
	PORT_DIPNAME( 0x10, 0x00, "Minimum plays" )
	PORT_DIPSETTING (	0x00, "1" )
	PORT_DIPSETTING (	0x10, "2" )
	PORT_DIPNAME( 0x20, 0x00, DEF_STR( Difficulty ) )
	PORT_DIPSETTING (	0x00, DEF_STR( Hard ) )
	PORT_DIPSETTING (	0x20, DEF_STR( Easy ) )
	PORT_DIPNAME( 0xc0, 0x00, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING (	0x00, "10000" )
	PORT_DIPSETTING (	0x40, "12000" )
	PORT_DIPSETTING (	0x80, "15000" )
	PORT_DIPSETTING (	0xc0, DEF_STR( None ) )

	PORT_START_TAG("DSW2")
	PORT_DIPNAME( 0x03, 0x01, DEF_STR( Coinage ) )
	PORT_DIPSETTING (	0x00, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING (	0x01, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING (	0x02, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING (	0x03, DEF_STR( Free_Play ) )
	PORT_DIPNAME( 0x0c, 0x0c, "Right Coin" )
	PORT_DIPSETTING (	0x00, "*6" )
	PORT_DIPSETTING (	0x04, "*5" )
	PORT_DIPSETTING (	0x08, "*4" )
	PORT_DIPSETTING (	0x0c, "*1" )
	PORT_DIPNAME( 0x10, 0x10, "Center Coin" )
	PORT_DIPSETTING (	0x00, "*2" )
	PORT_DIPSETTING (	0x10, "*1" )
	PORT_DIPNAME( 0xe0, 0x80, "Bonus Coins" )
	PORT_DIPSETTING (	0x60, "1 each 5" )
	PORT_DIPSETTING (	0x80, "2 each 4" )
	PORT_DIPSETTING (	0xa0, "1 each 4" )
	PORT_DIPSETTING (	0xc0, "1 each 2" )
	PORT_DIPSETTING (	0xe0, DEF_STR( None ) )
INPUT_PORTS_END


INPUT_PORTS_START( llander )
	PORT_START_TAG("IN0")
	/* Bit 0 is VG_HALT, handled in the machine dependent part */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_SERVICE( 0x02, IP_ACTIVE_LOW )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_TILT )
	/* Of the rest, Bit 6 is the 3KHz source. 3,4 and 5 are unknown */
	PORT_BIT( 0x78, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_SERVICE ) PORT_NAME("Diagnostic Step") PORT_CODE(KEYCODE_F1)

	PORT_START_TAG("IN1")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_COIN2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_COIN3 )
	PORT_BIT(0x10, IP_ACTIVE_HIGH, IPT_START2 ) PORT_NAME("Select Game")
	PORT_BIT(0x20, IP_ACTIVE_HIGH, IPT_BUTTON1 ) PORT_NAME("Abort")
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT ) PORT_2WAY
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT ) PORT_2WAY

	PORT_START_TAG("DSW")
	PORT_DIPNAME( 0x03, 0x01, "Right Coin" )
	PORT_DIPSETTING (	0x00, "*1" )
	PORT_DIPSETTING (	0x01, "*4" )
	PORT_DIPSETTING (	0x02, "*5" )
	PORT_DIPSETTING (	0x03, "*6" )
	PORT_DIPNAME( 0x0c, 0x00, DEF_STR( Language ) )
	PORT_DIPSETTING (	0x00, DEF_STR( English ) )
	PORT_DIPSETTING (	0x04, DEF_STR( French ) )
	PORT_DIPSETTING (	0x08, DEF_STR( Spanish ) )
	PORT_DIPSETTING (	0x0c, DEF_STR( German ) )
	PORT_DIPNAME( 0x20, 0x00, DEF_STR( Coinage ) )
	PORT_DIPSETTING (	0x00, DEF_STR( Normal ) )
	PORT_DIPSETTING (	0x20, DEF_STR( Free_Play ) )
	PORT_DIPNAME( 0xd0, 0x80, "Fuel units" )
	PORT_DIPSETTING (	0x00, "450" )
	PORT_DIPSETTING (	0x40, "600" )
	PORT_DIPSETTING (	0x80, "750" )
	PORT_DIPSETTING (	0xc0, "900" )
	PORT_DIPSETTING (	0x10, "1100" )
	PORT_DIPSETTING (	0x50, "1300" )
	PORT_DIPSETTING (	0x90, "1550" )
	PORT_DIPSETTING (	0xd0, "1800" )

	/* The next one is a potentiometer */
	PORT_START_TAG("IN3")
	PORT_BIT( 0xff, 0x00, IPT_PADDLE ) PORT_MINMAX(0,255) PORT_SENSITIVITY(100) PORT_KEYDELTA(10) PORT_CODE_DEC(KEYCODE_UP) PORT_CODE_DEC(JOYCODE_1_UP) PORT_CODE_INC(KEYCODE_DOWN) PORT_CODE_INC(JOYCODE_1_DOWN) PORT_REVERSE
INPUT_PORTS_END


INPUT_PORTS_START( llander1 )
	PORT_START_TAG("IN0")
	/* Bit 0 is VG_HALT, handled in the machine dependent part */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_SERVICE( 0x02, IP_ACTIVE_LOW )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_TILT )
	/* Of the rest, Bit 6 is the 3KHz source. 3,4 and 5 are unknown */
	PORT_BIT( 0x78, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_SERVICE ) PORT_NAME("Diagnostic Step") PORT_CODE(KEYCODE_F1)

	PORT_START_TAG("IN1")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_COIN2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_COIN3 )
	PORT_BIT(0x10, IP_ACTIVE_HIGH, IPT_START2 ) PORT_NAME("Select Game")
	PORT_BIT(0x20, IP_ACTIVE_HIGH, IPT_BUTTON1 ) PORT_NAME("Abort")
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT ) PORT_2WAY
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT ) PORT_2WAY

	PORT_START_TAG("DSW")
	PORT_DIPNAME( 0x03, 0x01, "Right Coin" )
	PORT_DIPSETTING (	0x00, "*1" )
	PORT_DIPSETTING (	0x01, "*4" )
	PORT_DIPSETTING (	0x02, "*5" )
	PORT_DIPSETTING (	0x03, "*6" )
	PORT_DIPNAME( 0x0c, 0x00, DEF_STR( Language ) )
	PORT_DIPSETTING (	0x00, DEF_STR( English ) )
	PORT_DIPSETTING (	0x04, DEF_STR( French ) )
	PORT_DIPSETTING (	0x08, DEF_STR( Spanish ) )
	PORT_DIPSETTING (	0x0c, DEF_STR( German ) )
	PORT_DIPNAME( 0x10, 0x00, DEF_STR( Coinage ) )
	PORT_DIPSETTING (	0x00, DEF_STR( Normal ) )
	PORT_DIPSETTING (	0x10, DEF_STR( Free_Play ) )
	PORT_DIPNAME( 0xc0, 0x80, "Fuel units" )
	PORT_DIPSETTING (	0x00, "450" )
	PORT_DIPSETTING (	0x40, "600" )
	PORT_DIPSETTING (	0x80, "750" )
	PORT_DIPSETTING (	0xc0, "900" )

	/* The next one is a potentiometer */
	PORT_START_TAG("IN3")
	PORT_BIT( 0xff, 0x00, IPT_PADDLE ) PORT_MINMAX(0,255) PORT_SENSITIVITY(100) PORT_KEYDELTA(10) PORT_CODE_DEC(KEYCODE_UP) PORT_CODE_DEC(JOYCODE_1_UP) PORT_CODE_INC(KEYCODE_DOWN) PORT_CODE_INC(JOYCODE_1_DOWN) PORT_REVERSE
INPUT_PORTS_END



/*************************************
 *
 *  Sound interfaces
 *
 *************************************/

static struct POKEYinterface pokey_interface =
{
	{ 0 },
	input_port_3_r
};



/*************************************
 *
 *  Machine drivers
 *
 *************************************/

static MACHINE_DRIVER_START( asteroid )

	/* basic machine hardware */
	MDRV_CPU_ADD_TAG("main", M6502, 1500000)
	MDRV_CPU_PROGRAM_MAP(asteroid_map,0)
	MDRV_CPU_VBLANK_INT(asteroid_interrupt,4)	/* 250 Hz */

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_MACHINE_RESET(asteroid)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_VECTOR | VIDEO_RGB_DIRECT)
	MDRV_SCREEN_SIZE(400,300)
	MDRV_VISIBLE_AREA(0, 1040, 70, 950)
	MDRV_PALETTE_LENGTH(32768)

	MDRV_PALETTE_INIT(avg_white)
	MDRV_VIDEO_START(dvg)
	MDRV_VIDEO_UPDATE(vector)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")

	MDRV_SOUND_ADD_TAG("disc", DISCRETE, 0)
	MDRV_SOUND_CONFIG(asteroid_discrete_interface)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.0)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( asterock )

	/* basic machine hardware */
	MDRV_IMPORT_FROM(asteroid)
	MDRV_CPU_MODIFY("main")
	MDRV_CPU_VBLANK_INT(asterock_interrupt,4)	/* 250 Hz */
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( astdelux )

	/* basic machine hardware */
	MDRV_IMPORT_FROM(asteroid)
	MDRV_CPU_MODIFY("main")
	MDRV_CPU_PROGRAM_MAP(astdelux_map,0)

	MDRV_NVRAM_HANDLER(atari_vg)

	/* sound hardware */
	MDRV_SOUND_REPLACE("disc", DISCRETE, 0)
	MDRV_SOUND_CONFIG(astdelux_discrete_interface)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.0)

	MDRV_SOUND_ADD(POKEY, 1500000)
	MDRV_SOUND_CONFIG(pokey_interface)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.0)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( llander )

	/* basic machine hardware */
	MDRV_IMPORT_FROM(asteroid)
	MDRV_CPU_MODIFY("main")
	MDRV_CPU_PROGRAM_MAP(llander_map,0)
	MDRV_CPU_VBLANK_INT(llander_interrupt,6)	/* 250 Hz */

	MDRV_FRAMES_PER_SECOND(40)
	MDRV_MACHINE_RESET(NULL)

	/* video hardware */
	MDRV_VISIBLE_AREA(0, 1050, 0, 900)

	MDRV_PALETTE_INIT(avg_white)
	MDRV_VIDEO_START(dvg)
	MDRV_VIDEO_UPDATE(vector)

	/* sound hardware */
	MDRV_SOUND_REPLACE("disc", DISCRETE, 0)
	MDRV_SOUND_CONFIG(llander_discrete_interface)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.0)
MACHINE_DRIVER_END



/*************************************
 *
 *  ROM definitions
 *
 *************************************/

ROM_START( asteroid )
	ROM_REGION( 0x8000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "035145.02",    0x6800, 0x0800, CRC(0cc75459) SHA1(2af85c9689b878155004da47fedbde5853a18723) )
	ROM_LOAD( "035144.02",    0x7000, 0x0800, CRC(096ed35c) SHA1(064d680ded7f30c543f93ae5ca85f90d550f73e5) )
	ROM_LOAD( "035143.02",    0x7800, 0x0800, CRC(312caa02) SHA1(1ce2eac1ab90b972e3f1fc3d250908f26328d6cb) )
	/* Vector ROM */
	ROM_LOAD( "035127.02",    0x5000, 0x0800, CRC(8b71fd9e) SHA1(8cd5005e531eafa361d6b7e9eed159d164776c70) )
ROM_END


ROM_START( asteroi1 )
	ROM_REGION( 0x8000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "035145.01",    0x6800, 0x0800, CRC(e9bfda64) SHA1(291dc567ebb31b35df83d9fb87f4080f251ff9c8) )
	ROM_LOAD( "035144.01",    0x7000, 0x0800, CRC(e53c28a9) SHA1(d9f081e73511ec43377f0c6457747f15a470d4dc) )
	ROM_LOAD( "035143.01",    0x7800, 0x0800, CRC(7d4e3d05) SHA1(d88000e904e158efde50e453e2889ecd2cb95f24) )
	/* Vector ROM */
	ROM_LOAD( "035127.01",    0x5000, 0x0800, CRC(99699366) SHA1(9b2828fc1cef7727f65fa65e1e11e309b7c98792) )
ROM_END


ROM_START( asteroib )
	ROM_REGION( 0x8000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "035145ll.bin", 0x6800, 0x0800, CRC(605fc0f2) SHA1(8d897a3b75bd1f2537470f0a34a97a8c0853ee08) )
	ROM_LOAD( "035144ll.bin", 0x7000, 0x0800, CRC(e106de77) SHA1(003e99d095bd4df6fae243ea1dd5b12f3eb974f1) )
	ROM_LOAD( "035143ll.bin", 0x7800, 0x0800, CRC(6b1d8594) SHA1(ff3cd93f1bc5734bface285e442125b395602d7d) )
	/* Vector ROM */
	ROM_LOAD( "035127.02",    0x5000, 0x0800, CRC(8b71fd9e) SHA1(8cd5005e531eafa361d6b7e9eed159d164776c70) )
ROM_END


ROM_START( asterock )
	ROM_REGION( 0x8000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "sidamas.2",    0x6800, 0x0400, CRC(cdf720c6) SHA1(85fe748096478e28a06bd98ff3aad73ab21b22a4) )
	ROM_LOAD( "sidamas.3",    0x6c00, 0x0400, CRC(ee58bdf0) SHA1(80094cb5dafd327aff6658ede33106f0493a809d) )
	ROM_LOAD( "sidamas.4",    0x7000, 0x0400, CRC(8d3e421e) SHA1(5f5719ab84d4755e69bef205d313b455bc59c413) )
	ROM_LOAD( "sidamas.5",    0x7400, 0x0400, CRC(d2ce7672) SHA1(b6012e09b2439a614a55bcf23be0692c42830e21) )
	ROM_LOAD( "sidamas.6",    0x7800, 0x0400, CRC(74103c87) SHA1(e568b5ac573a6d0474cf672b3c62abfbd3320799) )
	ROM_LOAD( "sidamas.7",    0x7c00, 0x0400, CRC(75a39768) SHA1(bf22998fd692fb01964d8894e421435c55d746a0) )
	/* Vector ROM */
	ROM_LOAD( "sidamas.0",    0x5000, 0x0400, CRC(6bd2053f) SHA1(790f2858f44bbb1854e2d9d549e29f4815c4665b) )
	ROM_LOAD( "sidamas.1",    0x5400, 0x0400, CRC(231ce201) SHA1(710f4c19864d725ba1c9ea447a97e84001a679f7) )
ROM_END


ROM_START( meteorts )
	ROM_REGION( 0x8000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "m0_c1.bin",    0x6800, 0x0800, CRC(dff88688) SHA1(7f4148a580fb6f605499c99e7dde7068eca1651a) )
	ROM_LOAD( "m1_f1.bin",    0x7000, 0x0800, CRC(e53c28a9) SHA1(d9f081e73511ec43377f0c6457747f15a470d4dc) )
	ROM_LOAD( "m2_j1.bin",    0x7800, 0x0800, CRC(64bd0408) SHA1(141d053cb4cce3fece98293136928b527d3ade0f) )
	/* Vector ROM */
	ROM_LOAD( "mv_np3.bin",   0x5000, 0x0800, CRC(11d1c4ae) SHA1(433c2c05b92094bbe102c356d7f1a907db13da67) )
ROM_END


ROM_START( astdelux )
	ROM_REGION( 0x8000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "036430.02",    0x6000, 0x0800, CRC(a4d7a525) SHA1(abe262193ec8e1981be36928e9a89a8ac95cd0ad) )
	ROM_LOAD( "036431.02",    0x6800, 0x0800, CRC(d4004aae) SHA1(aa2099b8fc62a79879efeea70ea1e9ed77e3e6f0) )
	ROM_LOAD( "036432.02",    0x7000, 0x0800, CRC(6d720c41) SHA1(198218cd2f43f8b83e4463b1f3a8aa49da5015e4) )
	ROM_LOAD( "036433.03",    0x7800, 0x0800, CRC(0dcc0be6) SHA1(bf10ffb0c4870e777d6b509cbede35db8bb6b0b8) )
	/* Vector ROM */
	ROM_LOAD( "036800.02",    0x4800, 0x0800, CRC(bb8cabe1) SHA1(cebaa1b91b96e8b80f2b2c17c6fd31fa9f156386) )
	ROM_LOAD( "036799.01",    0x5000, 0x0800, CRC(7d511572) SHA1(1956a12bccb5d3a84ce0c1cc10c6ad7f64e30b40) )

	ROM_REGION( 0x0100, REGION_PROMS, 0 )
	ROM_LOAD( "034602.bin",   0x0000, 0x0100, CRC(97953db8) SHA1(8cbded64d1dd35b18c4d5cece00f77e7b2cab2ad) )
ROM_END


ROM_START( astdelu1 )
	ROM_REGION( 0x8000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "036430.01",    0x6000, 0x0800, CRC(8f5dabc6) SHA1(5d7543e19acab99ddb63c0ffd60f54d7a0f267f5) )
	ROM_LOAD( "036431.01",    0x6800, 0x0800, CRC(157a8516) SHA1(9041d8c2369d004f198681e02b59a923fa8f70c9) )
	ROM_LOAD( "036432.01",    0x7000, 0x0800, CRC(fdea913c) SHA1(ded0138a20d80317d67add5bb2a64e6274e0e409) )
	ROM_LOAD( "036433.02",    0x7800, 0x0800, CRC(d8db74e3) SHA1(52b64e867df98d14742eb1817b59931bb7f941d9) )
	/* Vector ROM */
	ROM_LOAD( "036800.01",    0x4800, 0x0800, CRC(3b597407) SHA1(344fea2e5d84acce365d76daed61e96b9b6b37cc) )
	ROM_LOAD( "036799.01",    0x5000, 0x0800, CRC(7d511572) SHA1(1956a12bccb5d3a84ce0c1cc10c6ad7f64e30b40) )

	ROM_REGION( 0x0100, REGION_PROMS, 0 )
	ROM_LOAD( "034602.bin",   0x0000, 0x0100, CRC(97953db8) SHA1(8cbded64d1dd35b18c4d5cece00f77e7b2cab2ad) )
ROM_END


ROM_START( llander )
	ROM_REGION( 0x8000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "034572.02",    0x6000, 0x0800, CRC(b8763eea) SHA1(5a15eaeaf825ccdf9ce013a6789cf51da20f785c) )
	ROM_LOAD( "034571.02",    0x6800, 0x0800, CRC(77da4b2f) SHA1(4be6cef5af38734d580cbfb7e4070fe7981ddfd6) )
	ROM_LOAD( "034570.01",    0x7000, 0x0800, CRC(2724e591) SHA1(ecf4430a0040c227c896aa2cd81ee03960b4d641) )
	ROM_LOAD( "034569.02",    0x7800, 0x0800, CRC(72837a4e) SHA1(9b21ba5e1518079c326ca6e15b9993e6c4483caa) )
	/* Vector ROM */
	ROM_LOAD( "034599.01",    0x4800, 0x0800, CRC(355a9371) SHA1(6ecb40169b797d9eb623bcb17872f745b1bf20fa) )
	ROM_LOAD( "034598.01",    0x5000, 0x0800, CRC(9c4ffa68) SHA1(eb4ffc289d254f699f821df3146aa2c6cd78597f) )
	/* This _should_ be the rom for international versions. */
	/* Unfortunately, is it not currently available. */
	ROM_LOAD( "034597.01",    0x5800, 0x0800, NO_DUMP )
ROM_END


ROM_START( llander1 )
	ROM_REGION( 0x8000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "034572.01",    0x6000, 0x0800, CRC(2aff3140) SHA1(4fc8aae640ce655417c11d9a3121aae9a1238e7c) )
	ROM_LOAD( "034571.01",    0x6800, 0x0800, CRC(493e24b7) SHA1(125a2c335338ccabababef12fd7096ef4b605a31) )
	ROM_LOAD( "034570.01",    0x7000, 0x0800, CRC(2724e591) SHA1(ecf4430a0040c227c896aa2cd81ee03960b4d641) )
	ROM_LOAD( "034569.01",    0x7800, 0x0800, CRC(b11a7d01) SHA1(8f2935dbe04ee68815d69ea9e71853b5a145d7c3) )
	/* Vector ROM */
	ROM_LOAD( "034599.01",    0x4800, 0x0800, CRC(355a9371) SHA1(6ecb40169b797d9eb623bcb17872f745b1bf20fa) )
	ROM_LOAD( "034598.01",    0x5000, 0x0800, CRC(9c4ffa68) SHA1(eb4ffc289d254f699f821df3146aa2c6cd78597f) )
	/* This _should_ be the rom for international versions. */
	/* Unfortunately, is it not currently available. */
	ROM_LOAD( "034597.01",    0x5800, 0x0800, NO_DUMP )
ROM_END



/*************************************
 *
 *  Driver initialization
 *
 *************************************/

static DRIVER_INIT( asteroib )
{
	memory_install_read8_handler(0, ADDRESS_SPACE_PROGRAM, 0x2000, 0x2000, 0, 0, asteroib_IN0_r);
	memory_install_read8_handler(0, ADDRESS_SPACE_PROGRAM, 0x2003, 0x2003, 0, 0, input_port_3_r);
}


static DRIVER_INIT( asterock )
{

	memory_install_read8_handler(0, ADDRESS_SPACE_PROGRAM, 0x2000, 0x2007, 0, 0, asterock_IN0_r);
}


static DRIVER_INIT( astdelux )
{
	OVERLAY_START( astdelux_overlay )
		OVERLAY_RECT( 0.0, 0.0, 1.0, 1.0, MAKE_ARGB(0x04,0x88,0xff,0xff) )
	OVERLAY_END

	artwork_set_overlay(astdelux_overlay);
}



/*************************************
 *
 *  Game drivers
 *
 *************************************/

GAME( 1979, asteroid, 0,        asteroid, asteroid, 0,        ROT0, "Atari", "Asteroids (rev 2)", 0 )
GAME( 1979, asteroi1, asteroid, asteroid, asteroid, 0,        ROT0, "Atari", "Asteroids (rev 1)", 0 )
GAME( 1979, asteroib, asteroid, asteroid, asteroib, asteroib, ROT0, "bootleg", "Asteroids (bootleg on Lunar Lander hardware)", 0 )
GAME( 1979, asterock, asteroid, asterock, asterock, asterock, ROT0, "Sidam", "Asterock", 0 )
GAME( 1979, meteorts, asteroid, asteroid, asteroid, 0,        ROT0, "VGG",   "Meteorites", 0 )
GAME( 1980, astdelux, 0,        astdelux, astdelux, astdelux, ROT0, "Atari", "Asteroids Deluxe (rev 2)", 0 )
GAME( 1980, astdelu1, astdelux, astdelux, astdelux, astdelux, ROT0, "Atari", "Asteroids Deluxe (rev 1)", 0 )
GAME( 1979, llander,  0,        llander,  llander,  0,        ROT0, "Atari", "Lunar Lander (rev 2)", 0 )
GAME( 1979, llander1, llander,  llander,  llander1, 0,        ROT0, "Atari", "Lunar Lander (rev 1)", 0 )
