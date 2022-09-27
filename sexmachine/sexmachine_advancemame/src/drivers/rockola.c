/***************************************************************************

    Sasuke vs. Commander
    SNK/Rock-Ola

    driver by ?

    Games supported:
        * Sasuke vs. Commander
        * Satan of Saturn
        * Zarzon
        * Vanguard
        * Fantasy                   G-202
        * Pioneer Balloon               G-204
        * Nibbler                   G-208

****************************************************************************

Vanguard memory map (preliminary)

0000-03ff RAM
0400-07ff Video RAM 1
0800-0bff Video RAM 2
0c00-0fff Color RAM (3 bits for video RAM 1 and 3 bits for video RAM 2)
1000-1fff Character generator RAM
4000-bfff ROM

read:
3104      IN0
3105      IN1
3106      DSW ??
3107      IN2

write
3100      Sound Port 0
3101      Sound Port 1
3103      bit 7 = flip screen
3200      y scroll register
3300      x scroll register

****************************************************************************

Fantasy and Nibbler memory map (preliminary)

0000-03ff RAM
0400-07ff Video RAM 1
0800-0bff Video RAM 2
0c00-0fff Color RAM (3 bits for video RAM 1 and 3 bits for video RAM 2)
1000-1fff Character generator RAM
3000-bfff ROM

read:
2104      IN0
2105      IN1
2106      DSW
2107      IN2

write
2000-2001 To the HD46505S video controller
2100      Sound Port 0
2101      Sound Port 1
2103      bit 7 = flip screen
          bit 4-6 = music 2
          bit 3 = char bank selector
          bit 0-2 = background color
2200      y scroll register
2300      x scroll register

Interrupts: VBlank causes an IRQ. Coin insertion causes a NMI.

****************************************************************************

Pioneer Balloon memory map (preliminary)

0000-03ff RAM          IC13 cpu
0400-07ff Video RAM 1  IC67 video
0800-0bff Video RAM 2  ???? video
0c00-0fff Color RAM    IC68 (3 bits for VRAM 1 and 3 bits for VRAM 2)
1000-1fff RAM          ???? Character generator
3000-3fff ROM 4/5      IC12
4000-4fff ROM 1        IC07
5000-5fff ROM 2        IC08
6000-6fff ROM 3        IC09
7000-7fff ROM 4        IC10
8000-8fff ROM 5        IC14
9000-9fff ROM 6        IC15
read:
b104      IN0
b105      IN1
b106      DSW
b107      IN2

write
b000      Sound Port 0
b001      Sound Port 1
b100      ????
b103      bit 7 = flip screen
          bit 4-6 = music 2
          bit 3 = char bank selector
          bit 0-2 = background color
b106      ????
b200      y scroll register
b300      x scroll register

Interrupts: VBlank causes an IRQ. Coin insertion causes a NMI.

***************************************************************************/

/*

    TODO:

    - sasuke/satansat/vanguard discrete sound
    - vanguard/fantasy speech (hd38880/hd38882 emulation)
    - music freq (Satan of Saturn and clone)
    - correct music waveform/volume control
    - clean up dips/inputs for all games
    - correct ROM names
    - fantasy is German? (the continue text is in German)

*/

#include "driver.h"
#include "cpu/m6502/m6502.h"
#include "vidhrdw/crtc6845.h"
#include "sound/sn76477.h"
#include "sound/custom.h"
#include "sound/samples.h"

#ifndef M_LN2
#define M_LN2		0.69314718055994530942
#endif


/* vidhrdw */

extern UINT8 *rockola_videoram2;
extern UINT8 *rockola_charram;

extern WRITE8_HANDLER( rockola_videoram_w );
extern WRITE8_HANDLER( rockola_videoram2_w );
extern WRITE8_HANDLER( rockola_colorram_w );
extern WRITE8_HANDLER( rockola_charram_w );
extern WRITE8_HANDLER( rockola_flipscreen_w );
extern WRITE8_HANDLER( rockola_scrollx_w );
extern WRITE8_HANDLER( rockola_scrolly_w );

extern PALETTE_INIT( rockola );
extern VIDEO_START( rockola );
extern VIDEO_UPDATE( rockola );

extern WRITE8_HANDLER( satansat_charram_w );
extern WRITE8_HANDLER( satansat_b002_w );
extern WRITE8_HANDLER( satansat_backcolor_w );

extern PALETTE_INIT( satansat );
extern VIDEO_START( satansat );

/* sndhrdw */

extern const char *sasuke_sample_names[];
extern const char *vanguard_sample_names[];
extern const char *fantasy_sample_names[];

extern WRITE8_HANDLER( sasuke_sound_w );
extern WRITE8_HANDLER( satansat_sound_w );
extern WRITE8_HANDLER( vanguard_sound_w );
extern WRITE8_HANDLER( vanguard_speech_w );
extern WRITE8_HANDLER( fantasy_sound_w );
extern WRITE8_HANDLER( fantasy_speech_w );

void *rockola_sh_start(int clock, const struct CustomSound_interface *config);
void rockola_set_music_clock(double clock_time);
void rockola_set_music_freq(int freq);
int rockola_music0_playing(void);


/* binary counter (1.4MHz update) */
static UINT8 sasuke_counter;
static void *sasuke_timer;

static void sasuke_update_counter(int param)
{
	sasuke_counter += 0x10;
}

static void sasuke_start_counter(void)
{
	sasuke_counter = 0;

	sasuke_timer = timer_alloc(sasuke_update_counter);
	timer_adjust(sasuke_timer, TIME_NOW, 0, TIME_IN_HZ(11289000/8));	// 1.4 MHz
}


/* IN1 + music0 playing */
static READ8_HANDLER( sasuke_port_1_r )
{
	return readinputport(1) | (rockola_music0_playing() ? 0x80 : 0x00);
}

/* IN2 + binary counter */
static READ8_HANDLER( sasuke_port_3_r )
{
	return readinputport(3) | sasuke_counter;
}

/* IN2 + music0 playing */
static READ8_HANDLER( vanguard_port_3_r )
{
	return readinputport(3) | (rockola_music0_playing() ? 0x10 : 0x00);
}


/* Memory Maps */

static ADDRESS_MAP_START( sasuke_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x03ff) AM_RAM
	AM_RANGE(0x0400, 0x07ff) AM_RAM AM_WRITE(rockola_videoram2_w) AM_BASE(&rockola_videoram2)
	AM_RANGE(0x0800, 0x0bff) AM_RAM AM_WRITE(rockola_videoram_w) AM_BASE(&videoram)
	AM_RANGE(0x0c00, 0x0fff) AM_RAM AM_WRITE(rockola_colorram_w) AM_BASE(&colorram)
	AM_RANGE(0x1000, 0x1fff) AM_RAM AM_WRITE(rockola_charram_w) AM_BASE(&rockola_charram)
	AM_RANGE(0x3000, 0x3000) AM_WRITE(crtc6845_address_w)
	AM_RANGE(0x3001, 0x3001) AM_WRITE(crtc6845_register_w)
	AM_RANGE(0x4000, 0x8fff) AM_ROM
	AM_RANGE(0xb000, 0xb001) AM_WRITE(sasuke_sound_w)
	AM_RANGE(0xb002, 0xb002) AM_WRITE(satansat_b002_w)	/* flip screen & irq enable */
	AM_RANGE(0xb003, 0xb003) AM_WRITE(satansat_backcolor_w)
	AM_RANGE(0xb004, 0xb004) AM_READ(input_port_0_r)	// IN0
	AM_RANGE(0xb005, 0xb005) AM_READ(sasuke_port_1_r)	// IN1 + music0 playing
	AM_RANGE(0xb006, 0xb006) AM_READ(input_port_2_r)	/* DSW */
	AM_RANGE(0xb007, 0xb007) AM_READ(sasuke_port_3_r)	// IN2 + binary counter
	AM_RANGE(0xf800, 0xffff) AM_ROM
ADDRESS_MAP_END

static ADDRESS_MAP_START( satansat_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x03ff) AM_RAM
	AM_RANGE(0x0400, 0x07ff) AM_RAM AM_WRITE(rockola_videoram2_w) AM_BASE(&rockola_videoram2)
	AM_RANGE(0x0800, 0x0bff) AM_RAM AM_WRITE(rockola_videoram_w) AM_BASE(&videoram)
	AM_RANGE(0x0c00, 0x0fff) AM_RAM AM_WRITE(rockola_colorram_w) AM_BASE(&colorram)
	AM_RANGE(0x1000, 0x1fff) AM_RAM AM_WRITE(rockola_charram_w) AM_BASE(&rockola_charram)
	AM_RANGE(0x3000, 0x3000) AM_WRITE(crtc6845_address_w)
	AM_RANGE(0x3001, 0x3001) AM_WRITE(crtc6845_register_w)
	AM_RANGE(0x4000, 0x97ff) AM_ROM
	AM_RANGE(0xb000, 0xb001) AM_WRITE(satansat_sound_w)
	AM_RANGE(0xb002, 0xb002) AM_WRITE(satansat_b002_w)	/* flip screen & irq enable */
	AM_RANGE(0xb003, 0xb003) AM_WRITE(satansat_backcolor_w)
	AM_RANGE(0xb004, 0xb004) AM_READ(input_port_0_r)	// IN0
	AM_RANGE(0xb005, 0xb005) AM_READ(sasuke_port_1_r)	// IN1 + music0 playing
	AM_RANGE(0xb006, 0xb006) AM_READ(input_port_2_r)	/* DSW */
	AM_RANGE(0xb007, 0xb007) AM_READ(sasuke_port_3_r)	// IN2 + binary counter
	AM_RANGE(0xf800, 0xffff) AM_ROM
ADDRESS_MAP_END

static ADDRESS_MAP_START( vanguard_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x03ff) AM_RAM
	AM_RANGE(0x0400, 0x07ff) AM_RAM AM_WRITE(rockola_videoram2_w) AM_BASE(&rockola_videoram2)
	AM_RANGE(0x0800, 0x0bff) AM_RAM AM_WRITE(rockola_videoram_w) AM_BASE(&videoram)
	AM_RANGE(0x0c00, 0x0fff) AM_RAM AM_WRITE(rockola_colorram_w) AM_BASE(&colorram)
	AM_RANGE(0x1000, 0x1fff) AM_RAM AM_WRITE(rockola_charram_w) AM_BASE(&rockola_charram)
	AM_RANGE(0x3000, 0x3000) AM_WRITE(crtc6845_address_w)
	AM_RANGE(0x3001, 0x3001) AM_WRITE(crtc6845_register_w)
	AM_RANGE(0x3100, 0x3102) AM_WRITE(vanguard_sound_w)
	AM_RANGE(0x3103, 0x3103) AM_WRITE(rockola_flipscreen_w)
	AM_RANGE(0x3104, 0x3104) AM_READ(input_port_0_r)	// IN0
	AM_RANGE(0x3105, 0x3105) AM_READ(input_port_1_r)	// IN1
	AM_RANGE(0x3106, 0x3106) AM_READ(input_port_2_r)	/* DSW */
	AM_RANGE(0x3107, 0x3107) AM_READ(vanguard_port_3_r)	// IN2 + music0 playing
	AM_RANGE(0x3200, 0x3200) AM_WRITE(rockola_scrollx_w)
	AM_RANGE(0x3300, 0x3300) AM_WRITE(rockola_scrolly_w)
	AM_RANGE(0x3400, 0x3400) AM_WRITE(vanguard_speech_w)	// speech
	AM_RANGE(0x4000, 0xbfff) AM_ROM
	AM_RANGE(0xf000, 0xffff) AM_ROM	/* for the reset / interrupt vectors */
ADDRESS_MAP_END

static ADDRESS_MAP_START( fantasy_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x03ff) AM_RAM
	AM_RANGE(0x0400, 0x07ff) AM_RAM AM_WRITE(rockola_videoram2_w) AM_BASE(&rockola_videoram2)
	AM_RANGE(0x0800, 0x0bff) AM_RAM AM_WRITE(rockola_videoram_w) AM_BASE(&videoram)
	AM_RANGE(0x0c00, 0x0fff) AM_RAM AM_WRITE(rockola_colorram_w) AM_BASE(&colorram)
	AM_RANGE(0x1000, 0x1fff) AM_RAM AM_WRITE(rockola_charram_w) AM_BASE(&rockola_charram)
	AM_RANGE(0x2000, 0x2000) AM_WRITE(crtc6845_address_w)
	AM_RANGE(0x2001, 0x2001) AM_WRITE(crtc6845_register_w)
	AM_RANGE(0x2100, 0x2103) AM_WRITE(fantasy_sound_w)
	AM_RANGE(0x2104, 0x2104) AM_READ(input_port_0_r)	// IN0
	AM_RANGE(0x2105, 0x2105) AM_READ(input_port_1_r)	// IN1
	AM_RANGE(0x2106, 0x2106) AM_READ(input_port_2_r)	/* DSW */
	AM_RANGE(0x2107, 0x2107) AM_READ(input_port_3_r)	// IN2
	AM_RANGE(0x2200, 0x2200) AM_WRITE(rockola_scrollx_w)
	AM_RANGE(0x2300, 0x2300) AM_WRITE(rockola_scrolly_w)
	AM_RANGE(0x2400, 0x2400) AM_WRITE(fantasy_speech_w)	// speech
	AM_RANGE(0x3000, 0xbfff) AM_ROM
	AM_RANGE(0xf000, 0xffff) AM_ROM
ADDRESS_MAP_END

static ADDRESS_MAP_START( pballoon_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x03ff) AM_RAM
	AM_RANGE(0x0400, 0x07ff) AM_RAM AM_WRITE(rockola_videoram2_w) AM_BASE(&rockola_videoram2)
	AM_RANGE(0x0800, 0x0bff) AM_RAM AM_WRITE(rockola_videoram_w) AM_BASE(&videoram)
	AM_RANGE(0x0c00, 0x0fff) AM_RAM AM_WRITE(rockola_colorram_w) AM_BASE(&colorram)
	AM_RANGE(0x1000, 0x1fff) AM_RAM AM_WRITE(rockola_charram_w) AM_BASE(&rockola_charram)
	AM_RANGE(0x3000, 0x9fff) AM_ROM
	AM_RANGE(0xb000, 0xb000) AM_WRITE(crtc6845_address_w)
	AM_RANGE(0xb001, 0xb001) AM_WRITE(crtc6845_register_w)
	AM_RANGE(0xb100, 0xb103) AM_WRITE(fantasy_sound_w)
	AM_RANGE(0xb104, 0xb104) AM_READ(input_port_0_r)	// IN0
	AM_RANGE(0xb105, 0xb105) AM_READ(input_port_1_r)	// IN1
	AM_RANGE(0xb106, 0xb106) AM_READ(input_port_2_r)	/* DSW */
	AM_RANGE(0xb107, 0xb107) AM_READ(input_port_3_r)	// IN2
	AM_RANGE(0xb200, 0xb200) AM_WRITE(rockola_scrollx_w)
	AM_RANGE(0xb300, 0xb300) AM_WRITE(rockola_scrolly_w)
	AM_RANGE(0xf000, 0xffff) AM_ROM
ADDRESS_MAP_END

/* Input Ports */

/* Derived from Zarzon. Might not reflect the actual hardware. */
INPUT_PORTS_START( sasuke )
	PORT_START  // IN0
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT ) PORT_2WAY
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT ) PORT_2WAY
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_BUTTON1 )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT ) PORT_2WAY PORT_COCKTAIL
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT ) PORT_2WAY PORT_COCKTAIL
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_BUTTON1 ) PORT_COCKTAIL
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_START2 )

	PORT_START	// IN1
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x7C, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_SPECIAL )	/* music0 playing */

	PORT_START  /* DSW */
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Upright ))
	PORT_DIPSETTING(    0x00, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0x02, 0x00, DEF_STR( Coinage ) )
	PORT_DIPSETTING (   0x00, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING (   0x02, DEF_STR( 1C_2C ) )
	PORT_DIPNAME( 0x04, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING (   0x00, DEF_STR( Off ) )
	PORT_DIPSETTING (   0x04, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING (   0x00, DEF_STR( Off ) )
	PORT_DIPSETTING (   0x08, DEF_STR( On ) )
	PORT_DIPNAME( 0x30, 0x00, DEF_STR( Lives ) )
	PORT_DIPSETTING (   0x00, "3" )
	PORT_DIPSETTING (   0x10, "4" )
	PORT_DIPSETTING (   0x20, "5" )
	/* 0x30 gives 3 again */
	PORT_DIPNAME( 0x40, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING (   0x00, DEF_STR( Off ) )
	PORT_DIPSETTING (   0x40, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x00, "RAM Test" )
	PORT_DIPSETTING (   0x00, DEF_STR( Off ) )
	PORT_DIPSETTING (   0x80, DEF_STR( On ) )

	PORT_START  // IN2
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_COIN1 ) PORT_IMPULSE(1)
	PORT_BIT( 0x0e, IP_ACTIVE_HIGH, IPT_UNKNOWN )	/* NC */
	PORT_BIT( 0xf0, IP_ACTIVE_HIGH, IPT_SPECIAL )	/* connected to a binary counter */
INPUT_PORTS_END

INPUT_PORTS_START( satansat )
	PORT_START  // IN0
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT ) PORT_2WAY
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT ) PORT_2WAY
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_BUTTON1 )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT ) PORT_2WAY PORT_COCKTAIL
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT ) PORT_2WAY PORT_COCKTAIL
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_BUTTON1 ) PORT_COCKTAIL
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_BUTTON2 )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_BUTTON2 ) PORT_COCKTAIL

	PORT_START	// IN1
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_BIT( 0x7C, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_SPECIAL )	/* music0 playing */

	PORT_START  /* DSW */
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Upright ))
	PORT_DIPSETTING(    0x00, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0x0a, 0x00, DEF_STR( Coinage ) )
	PORT_DIPSETTING (   0x08, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING (   0x00, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING (   0x02, DEF_STR( 1C_2C ) )
	/* 0x0a gives 2/1 again */
	PORT_DIPNAME( 0x04, 0x00, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING (   0x00, "5000" )
	PORT_DIPSETTING (   0x04, "10000" )
	PORT_DIPNAME( 0x30, 0x00, DEF_STR( Lives ) )
	PORT_DIPSETTING (   0x00, "3" )
	PORT_DIPSETTING (   0x10, "4" )
	PORT_DIPSETTING (   0x20, "5" )
	/* 0x30 gives 3 again */
	PORT_DIPNAME( 0x40, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING (   0x00, DEF_STR( Off ) )
	PORT_DIPSETTING (   0x40, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x00, "RAM Test" )
	PORT_DIPSETTING (   0x00, DEF_STR( Off ) )
	PORT_DIPSETTING (   0x80, DEF_STR( On ) )

	PORT_START  // IN2
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_COIN1 ) PORT_IMPULSE(1)
	PORT_BIT( 0x0e, IP_ACTIVE_HIGH, IPT_UNKNOWN )	/* NC */
	PORT_BIT( 0xf0, IP_ACTIVE_HIGH, IPT_SPECIAL )	/* connected to a binary counter */
INPUT_PORTS_END

INPUT_PORTS_START( vanguard )
	PORT_START	// IN0
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_BUTTON3 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_BUTTON4 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_BUTTON2 )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_BUTTON1 )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN ) PORT_8WAY
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP ) PORT_8WAY
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT ) PORT_8WAY
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT ) PORT_8WAY

	PORT_START	// IN1
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_BUTTON3 ) PORT_COCKTAIL
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_BUTTON4 ) PORT_COCKTAIL
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_BUTTON2 ) PORT_COCKTAIL
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_BUTTON1 ) PORT_COCKTAIL
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN ) PORT_8WAY PORT_COCKTAIL
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP ) PORT_8WAY PORT_COCKTAIL
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT ) PORT_8WAY PORT_COCKTAIL
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT ) PORT_8WAY PORT_COCKTAIL

	PORT_START	// DSW0
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0x4e, 0x02, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0x42, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x02, "1 Coin/1 Credit 2/3" )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x48, "1 Coin/2 Credits 2/5" )
	PORT_DIPSETTING(    0x08, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x44, "1 Coin/3 Credits 2/7" )
	PORT_DIPSETTING(    0x04, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x4c, "1 Coin/6 Credits 2/13" )
	PORT_DIPSETTING(    0x0c, DEF_STR( 1C_7C ) )
/*
    PORT_DIPSETTING(    0x06, DEF_STR( 1C_2C ) )
    PORT_DIPSETTING(    0x0a, DEF_STR( 1C_2C ) )
    PORT_DIPSETTING(    0x0e, DEF_STR( 1C_2C ) )
    PORT_DIPSETTING(    0x40, "1 Coin/1 Credit + Bonus" )
    PORT_DIPSETTING(    0x46, "1 Coin/1 Credit + Bonus" )
    PORT_DIPSETTING(    0x4a, "1 Coin/1 Credit + Bonus" )
    PORT_DIPSETTING(    0x4e, "1 Coin/1 Credit + Bonus" )
*/
	PORT_DIPNAME( 0x30, 0x00, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x00, "3" )
	PORT_DIPSETTING(    0x10, "4" )
	PORT_DIPSETTING(    0x20, "5" )
/*  PORT_DIPSETTING(    0x30, "3" ) */
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNUSED )

	PORT_START	// IN2
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_COIN2 ) PORT_IMPULSE(1)
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_COIN1 ) PORT_IMPULSE(1)
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_SPECIAL )	/* music0 playing */
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_START1 )
INPUT_PORTS_END

INPUT_PORTS_START( fantasy )
	PORT_START	// IN0
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN) PORT_8WAY
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP) PORT_8WAY
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT ) PORT_8WAY
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT ) PORT_8WAY

	PORT_START	// IN1
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN ) PORT_8WAY PORT_COCKTAIL
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP ) PORT_8WAY PORT_COCKTAIL
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT ) PORT_8WAY PORT_COCKTAIL
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT ) PORT_8WAY PORT_COCKTAIL

	PORT_START	// DSW0
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0x4e, 0x02, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0x42, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x02, "1 Coin/1 Credit 2/3" )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x48, "1 Coin/2 Credits 2/5" )
	PORT_DIPSETTING(    0x08, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x44, "1 Coin/3 Credits 2/7" )
	PORT_DIPSETTING(    0x04, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x4c, "1 Coin/6 Credits 2/13" )
	PORT_DIPSETTING(    0x0c, DEF_STR( 1C_7C ) )
/*
    PORT_DIPSETTING(    0x06, DEF_STR( 1C_2C ) )
    PORT_DIPSETTING(    0x0a, DEF_STR( 1C_2C ) )
    PORT_DIPSETTING(    0x0e, DEF_STR( 1C_2C ) )
    PORT_DIPSETTING(    0x40, "1 Coin/1 Credit + Bonus" )
    PORT_DIPSETTING(    0x46, "1 Coin/1 Credit + Bonus" )
    PORT_DIPSETTING(    0x4a, "1 Coin/1 Credit + Bonus" )
    PORT_DIPSETTING(    0x4e, "1 Coin/1 Credit + Bonus" )
*/
	PORT_DIPNAME( 0x30, 0x00, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x00, "3" )
	PORT_DIPSETTING(    0x10, "4" )
	PORT_DIPSETTING(    0x20, "5" )
/*  PORT_DIPSETTING(    0x30, "3" ) */
	PORT_DIPNAME( 0x80, 0x00, DEF_STR( Allow_Continue ) )
	PORT_DIPSETTING(    0x80, DEF_STR( No ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Yes ) )

	PORT_START	// IN2
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_COIN2 ) PORT_IMPULSE(1)
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_COIN1 ) PORT_IMPULSE(1)
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_START1 )
INPUT_PORTS_END

INPUT_PORTS_START( nibbler )
	PORT_START	// IN0
	PORT_BIT(0x01, IP_ACTIVE_HIGH, IPT_SERVICE ) PORT_NAME("Debug 0") PORT_CODE(KEYCODE_Z) // slow down
	PORT_BIT(0x02, IP_ACTIVE_HIGH, IPT_SERVICE ) PORT_NAME("Debug 1") PORT_CODE(KEYCODE_X)
	PORT_BIT(0x04, IP_ACTIVE_HIGH, IPT_SERVICE ) PORT_NAME("Debug 2") PORT_CODE(KEYCODE_C)
	PORT_BIT(0x08, IP_ACTIVE_HIGH, IPT_SERVICE ) PORT_NAME("Debug 3") PORT_CODE(KEYCODE_V)
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN) PORT_8WAY
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP) PORT_8WAY
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT ) PORT_8WAY
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT ) PORT_8WAY

	PORT_START	// IN1
	PORT_BIT(0x01, IP_ACTIVE_HIGH, IPT_SERVICE ) PORT_NAME("Debug 4") PORT_CODE(KEYCODE_B) // pause
	PORT_BIT(0x02, IP_ACTIVE_HIGH, IPT_SERVICE ) PORT_NAME("Debug 5") PORT_CODE(KEYCODE_N) // unpause
	PORT_BIT(0x04, IP_ACTIVE_HIGH, IPT_SERVICE ) PORT_NAME("Debug 6") PORT_CODE(KEYCODE_M) // end game
	PORT_BIT(0x08, IP_ACTIVE_HIGH, IPT_SERVICE ) PORT_NAME("Debug 7") PORT_CODE(KEYCODE_K)
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN ) PORT_8WAY PORT_COCKTAIL
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP ) PORT_8WAY PORT_COCKTAIL
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT ) PORT_8WAY PORT_COCKTAIL
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT ) PORT_8WAY PORT_COCKTAIL

	PORT_START	// DSW0
	PORT_DIPNAME( 0x03, 0x00, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x00, "3" )
	PORT_DIPSETTING(    0x01, "4" )
	PORT_DIPSETTING(    0x02, "5" )
	PORT_DIPSETTING(    0x03, "6" )
	PORT_DIPNAME( 0x04, 0x00, "Pause at Corners" )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Cocktail ) )
	PORT_SERVICE( 0x10, IP_ACTIVE_HIGH )
	PORT_DIPNAME( 0x20, 0x00, DEF_STR( Free_Play ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x20, DEF_STR( On ) )
	PORT_DIPNAME( 0xc0, 0x00, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0x40, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0xc0, DEF_STR( 4C_3C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x80, DEF_STR( 2C_3C ) )

	PORT_START	// IN2
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_COIN2 ) PORT_IMPULSE(1)
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_COIN1 ) PORT_IMPULSE(1)
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_START1 )
INPUT_PORTS_END

INPUT_PORTS_START( pballoon )
	PORT_START	// IN0
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_BUTTON1 )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN ) PORT_8WAY
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP ) PORT_8WAY
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT ) PORT_8WAY
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT ) PORT_8WAY

	PORT_START	// IN1
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_BUTTON1 ) PORT_COCKTAIL
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN ) PORT_8WAY PORT_COCKTAIL
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP ) PORT_8WAY PORT_COCKTAIL
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT ) PORT_8WAY PORT_COCKTAIL
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT ) PORT_8WAY PORT_COCKTAIL

	PORT_START	// DSW0
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0x4e, 0x02, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0x42, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x02, "1 Coin/1 Credit 2/3" )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x48, "1 Coin/2 Credits 2/5" )
	PORT_DIPSETTING(    0x08, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x44, "1 Coin/3 Credits 2/7" )
	PORT_DIPSETTING(    0x04, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x4c, "1 Coin/6 Credits 2/11" )
	PORT_DIPSETTING(    0x0c, DEF_STR( 1C_7C ) )
/*
    PORT_DIPSETTING(    0x06, DEF_STR( 1C_2C ) )
    PORT_DIPSETTING(    0x0a, DEF_STR( 1C_2C ) )
    PORT_DIPSETTING(    0x0e, DEF_STR( 1C_2C ) )
    PORT_DIPSETTING(    0x40, "1 Coin/1 Credit + Bonus" )
    PORT_DIPSETTING(    0x46, "1 Coin/1 Credit + Bonus" )
    PORT_DIPSETTING(    0x4a, "1 Coin/1 Credit + Bonus" )
    PORT_DIPSETTING(    0x4e, "1 Coin/1 Credit + Bonus" )
*/
	PORT_DIPNAME( 0x30, 0x00, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x00, "3" )
	PORT_DIPSETTING(    0x10, "4" )
	PORT_DIPSETTING(    0x20, "5" )
/*  PORT_DIPSETTING(    0x30, "3" ) */
	PORT_DIPNAME( 0x80, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START	// IN2
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_COIN2 ) PORT_IMPULSE(1)
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_COIN1 ) PORT_IMPULSE(1)
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_START1 )
INPUT_PORTS_END

/* Graphics Layouts */

static const gfx_layout swapcharlayout =
{
	8,8,    /* 8*8 characters */
	256,	/* 256 characters */
	2,      /* 2 bits per pixel */
	{ 256*8*8, 0 }, /* the two bitplanes are separated */
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8     /* every char takes 8 consecutive bytes */
};

static const gfx_layout charlayout =
{
	8,8,    /* 8*8 characters */
	RGN_FRAC(1,2),
	2,      /* 2 bits per pixel */
	{ 0, RGN_FRAC(1,2) }, /* the two bitplanes are separated */
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8     /* every char takes 8 consecutive bytes */
};

static const gfx_layout charlayout_memory =
{
	8,8,    /* 8*8 characters */
	256,	/* 256 characters */
	2,      /* 2 bits per pixel */
	{ 0, 256*8*8 }, /* the two bitplanes are separated */
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8     /* every char takes 8 consecutive bytes */
};

/* Graphics Decode Information */

static const gfx_decode sasuke_gfxdecodeinfo[] =
{
	{ 0,           0x1000, &swapcharlayout,      0, 4 },	/* the game dynamically modifies this */
	{ REGION_GFX1, 0x0000, &swapcharlayout,    4*4, 4 },
	{ -1 }
};

static const gfx_decode satansat_gfxdecodeinfo[] =
{
	{ 0,           0x1000, &charlayout_memory,   0, 4 },	/* the game dynamically modifies this */
	{ REGION_GFX1, 0x0000, &charlayout,        4*4, 4 },
	{ -1 }
};

static const gfx_decode vanguard_gfxdecodeinfo[] =
{
	{ 0,           0x1000, &charlayout_memory,   0, 8 },	/* the game dynamically modifies this */
	{ REGION_GFX1, 0x0000, &charlayout,        8*4, 8 },
	{ -1 }
};

/* Sound Interfaces */

static struct CustomSound_interface custom_interface =
{
	rockola_sh_start
};

static struct Samplesinterface sasuke_samples_interface =
{
	4,	/* 4 channels */
	sasuke_sample_names
};

static struct Samplesinterface vanguard_samples_interface =
{
	3,	/* 3 channel */
	vanguard_sample_names
};

static struct Samplesinterface fantasy_samples_interface =
{
	1,	/* 1 channel */
	fantasy_sample_names
};


static struct SN76477interface sasuke_sn76477_intf_1 =
{
	RES_K(470),		/*  4  noise_res     */
	RES_K(150),		/*  5  filter_res    */
	CAP_P(4700),	/*  6  filter_cap    */
	RES_K(22),		/*  7  decay_res     */
	CAP_U(10),		/*  8  attack_decay_cap  */
	RES_K(10),		/* 10  attack_res    */
	RES_K(100),		/* 11  amplitude_res     */
	RES_K(47),		/* 12  feedback_res      */
	0 /* NC */,		/* 16  vco_voltage   */
	0 /* NC */,		/* 17  vco_cap       */
	0 /* NC */,		/* 18  vco_res       */
	0 /* NC */,		/* 19  pitch_voltage     */
	RES_K(10),		/* 20  slf_res       */
	0 /* NC */,		/* 21  slf_cap       */
	CAP_U(2.2),		/* 23  oneshot_cap   */
	RES_K(100)		/* 24  oneshot_res   */

	// ic48     GND: 2,22,26,27,28  +5V: 1,15,25
};

static struct SN76477interface sasuke_sn76477_intf_2 =
{
	RES_K(340),		/*  4  noise_res     */
	RES_K(47),		/*  5  filter_res    */
	CAP_P(100),		/*  6  filter_cap    */
	RES_K(470),		/*  7  decay_res     */
	CAP_U(4.7),		/*  8  attack_decay_cap  */
	RES_K(10),		/* 10  attack_res    */
	RES_K(100),		/* 11  amplitude_res     */
	RES_K(47),		/* 12  feedback_res      */
	0 /* NC */,		/* 16  vco_voltage   */
	CAP_P(220),		/* 17  vco_cap       */
	RES_K(1000),	/* 18  vco_res       */
	0 /* NC */,		/* 19  pitch_voltage     */
	RES_K(220),		/* 20  slf_res       */
	0 /* NC */,		/* 21  slf_cap       */
	CAP_U(22),		/* 23  oneshot_cap   */
	RES_K(47)		/* 24  oneshot_res   */

	// ic51     GND: 2,26,27        +5V: 1,15,22,25,28
};

static struct SN76477interface sasuke_sn76477_intf_3 =
{
	RES_K(330),		/*  4  noise_res     */
	RES_K(47),		/*  5  filter_res    */
	CAP_P(100),		/*  6  filter_cap    */
	RES_K(1),		/*  7  decay_res     */
	0 /* NC */,		/*  8  attack_decay_cap  */
	RES_K(1),		/* 10  attack_res    */
	RES_K(100),		/* 11  amplitude_res     */
	RES_K(47),		/* 12  feedback_res      */
	0 /* NC */,		/* 16  vco_voltage   */
	CAP_P(1000),	/* 17  vco_cap       */
	RES_K(1000),	/* 18  vco_res       */
	0 /* NC */,		/* 19  pitch_voltage     */
	RES_K(10),		/* 20  slf_res       */
	CAP_U(1),		/* 21  slf_cap       */
	CAP_U(2.2),		/* 23  oneshot_cap   */
	RES_K(150)		/* 24  oneshot_res   */

	// ic52     GND: 2,22,27,28     +5V: 1,15,25,26
};

static struct SN76477interface satansat_sn76477_intf =
{
	RES_K(470),		/*  4  noise_res     */
	RES_M(1.5),		/*  5  filter_res    */
	CAP_P(220),		/*  6  filter_cap    */
	0,				/*  7  decay_res     */
	0,				/*  8  attack_decay_cap  */
	0,				/* 10  attack_res    */
	RES_K(47),		/* 11  amplitude_res     */
	RES_K(47),		/* 12  feedback_res      */
	0,				/* 16  vco_voltage   */
	0,				/* 17  vco_cap       */
	0,				/* 18  vco_res       */
	0,				/* 19  pitch_voltage     */
	0,				/* 20  slf_res       */
	0,				/* 21  slf_cap       */
	0,				/* 23  oneshot_cap   */
	0				/* 24  oneshot_res   */

	// ???      GND: 2,26,27        +5V: 15,25
};

static struct SN76477interface vanguard_sn76477_intf_1 =
{
	RES_K(470),		/*  4  noise_res     */
	RES_M(1.5),		/*  5  filter_res    */
	CAP_P(220),		/*  6  filter_cap    */
	0,				/*  7  decay_res     */
	0,				/*  8  attack_decay_cap  */
	0,				/* 10  attack_res    */
	RES_K(47),		/* 11  amplitude_res     */
	RES_K(4.7),		/* 12  feedback_res      */
	0,				/* 16  vco_voltage   */
	0,				/* 17  vco_cap       */
	0,				/* 18  vco_res       */
	0,				/* 19  pitch_voltage     */
	0,				/* 20  slf_res       */
	0,				/* 21  slf_cap       */
	0,				/* 23  oneshot_cap   */
	0				/* 24  oneshot_res   */

	// SHOT A   GND: 2,9,26,27  +5V: 15,25
};

static struct SN76477interface vanguard_sn76477_intf_2 =
{
	RES_K(10),		/*  4  noise_res     */
	RES_K(30),		/*  5  filter_res    */
	0,				/*  6  filter_cap    */
	0,				/*  7  decay_res     */
	0,				/*  8  attack_decay_cap  */
	0,				/* 10  attack_res    */
	RES_K(47),		/* 11  amplitude_res     */
	RES_K(4.7),		/* 12  feedback_res      */
	0,				/* 16  vco_voltage   */
	0,				/* 17  vco_cap       */
	0,				/* 18  vco_res       */
	0,				/* 19  pitch_voltage     */
	0,				/* 20  slf_res       */
	0,				/* 21  slf_cap       */
	0,				/* 23  oneshot_cap   */
	0				/* 24  oneshot_res   */

	// SHOT B   GND: 1,2,26,27  +5V: 15,25,28
};

static struct SN76477interface fantasy_sn76477_intf =
{
	RES_K(470),		/*  4  noise_res     */
	RES_M(1.5),		/*  5  filter_res    */
	CAP_P(220),		/*  6  filter_cap    */
	0,				/*  7  decay_res     */
	0,				/*  8  attack_decay_cap  */
	0,				/* 10  attack_res    */
	RES_K(470),		/* 11  amplitude_res     */
	RES_K(4.7),		/* 12  feedback_res      */
	0,				/* 16  vco_voltage   */
	0,				/* 17  vco_cap       */
	0,				/* 18  vco_res       */
	0,				/* 19  pitch_voltage     */
	0,				/* 20  slf_res       */
	0,				/* 21  slf_cap       */
	0,				/* 23  oneshot_cap   */
	0				/* 24  oneshot_res   */

	// BOMB     GND:    2,9,26,27       +5V: 15,25
};

/* Interrupt Generators */

static INTERRUPT_GEN( satansat_interrupt )
{
	if (cpu_getiloops() != 0)
	{
		UINT8 val = readinputport(3);

		coin_counter_w(0, val & 1);

		/* user asks to insert coin: generate a NMI interrupt. */
		if (val & 0x01)
			cpunum_set_input_line(0, INPUT_LINE_NMI, PULSE_LINE);
	}
	else
		cpunum_set_input_line(0, M6502_IRQ_LINE, HOLD_LINE);	/* one IRQ per frame */
}

static INTERRUPT_GEN( rockola_interrupt )
{
	if (cpu_getiloops() != 0)
	{
		UINT8 val = readinputport(3);

		coin_counter_w(0, val & 1);
		coin_counter_w(1, val & 2);

		/* user asks to insert coin: generate a NMI interrupt. */
		if (val & 0x03)
			cpunum_set_input_line(0, INPUT_LINE_NMI, PULSE_LINE);
	}
	else
		cpunum_set_input_line(0, M6502_IRQ_LINE, HOLD_LINE);	/* one IRQ per frame */
}

/* Machine Initialization */

static MACHINE_RESET( sasuke )
{
	//rockola_set_music_clock(M_LN2 * (RES_K(1) + RES_K(10) * 2) * CAP_U(1));
	// adjusted
	rockola_set_music_clock(1 / 72.1);

	// adjusted
	rockola_set_music_freq(38000);

	// ic48
	SN76477_envelope_1_w(0, 1);	// pin 1
	SN76477_envelope_2_w(0, 0);	// pin 28
	SN76477_vco_w(0, 0);		// pin 22
	SN76477_mixer_a_w(0, 0);	// pin 26
	SN76477_mixer_b_w(0, 1);	// pin 25
	SN76477_mixer_c_w(0, 0);	// pin 27

	// ic51
	SN76477_envelope_1_w(1, 1);	// pin 1
	SN76477_envelope_2_w(1, 1);	// pin 28
	SN76477_vco_w(1, 1);		// pin 22
	SN76477_mixer_a_w(1, 0);	// pin 26
	SN76477_mixer_b_w(1, 1);	// pin 25
	SN76477_mixer_c_w(1, 0);	// pin 27

	// ic52
	SN76477_envelope_1_w(2, 1);	// pin 1
	SN76477_envelope_2_w(2, 0);	// pin 28
	SN76477_vco_w(2, 0);		// pin 22
	SN76477_mixer_a_w(2, 1);	// pin 26
	SN76477_mixer_b_w(2, 1);	// pin 25
	SN76477_mixer_c_w(2, 0);	// pin 27

	sasuke_start_counter();
}

static MACHINE_RESET( satansat )
{
	// same as sasuke
	rockola_set_music_freq(38000);

	// ???
	SN76477_mixer_a_w(0, 0);
	SN76477_mixer_b_w(0, 1);
	SN76477_mixer_c_w(0, 0);

	sasuke_start_counter();
}

static MACHINE_RESET( vanguard )
{
	// 41.6 Hz update (measured)
	rockola_set_music_clock(1 / 41.6);

	// SHOT A
	SN76477_mixer_a_w(0, 0);
	SN76477_mixer_b_w(0, 1);
	SN76477_mixer_c_w(0, 0);

	// SHOT B
	SN76477_envelope_1_w(1, 0);
	SN76477_envelope_2_w(1, 1);
	SN76477_mixer_a_w(1, 0);
	SN76477_mixer_b_w(1, 1);
	SN76477_mixer_c_w(1, 0);
}

static MACHINE_RESET( fantasy )
{
	// BOMB
	SN76477_mixer_a_w(0, 0);
	SN76477_mixer_b_w(0, 1);
	SN76477_mixer_c_w(0, 0);
}

static MACHINE_RESET( pballoon )
{
	// 40.3 Hz update (measured)
	rockola_set_music_clock(1 / 40.3);

	machine_reset_fantasy();
}

/* Machine Drivers */

static MACHINE_DRIVER_START( sasuke )
	// basic machine hardware
	MDRV_CPU_ADD_TAG("main", M6502, 11289000/16) // 700 kHz
	MDRV_CPU_PROGRAM_MAP(sasuke_map, 0)
	MDRV_CPU_VBLANK_INT(satansat_interrupt, 2)

	MDRV_FRAMES_PER_SECOND((11289000.0 / 16) / (45 * 32 * 8))
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	MDRV_MACHINE_RESET(sasuke)

	// video hardware
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(32*8, 32*8)
	MDRV_VISIBLE_AREA(0*8, 32*8-1, 0*8, 28*8-1)
	MDRV_GFXDECODE(sasuke_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(32)
	MDRV_COLORTABLE_LENGTH(4*4 + 4*4)

	MDRV_PALETTE_INIT(satansat)
	MDRV_VIDEO_START(satansat)
	MDRV_VIDEO_UPDATE(rockola)

	// sound hardware
	MDRV_SPEAKER_STANDARD_MONO("mono")

	MDRV_SOUND_ADD(CUSTOM, 0)
	MDRV_SOUND_CONFIG(custom_interface)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.50)

	MDRV_SOUND_ADD_TAG("samples", SAMPLES, 0)
	MDRV_SOUND_CONFIG(sasuke_samples_interface)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.12)

	MDRV_SOUND_ADD_TAG("SN76477.1", SN76477, 0)
	MDRV_SOUND_CONFIG(sasuke_sn76477_intf_1)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.50)

	MDRV_SOUND_ADD_TAG("SN76477.2", SN76477, 0)
	MDRV_SOUND_CONFIG(sasuke_sn76477_intf_2)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.50)

	MDRV_SOUND_ADD_TAG("SN76477.3", SN76477, 0)
	MDRV_SOUND_CONFIG(sasuke_sn76477_intf_3)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.50)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( satansat )
	// basic machine hardware
	MDRV_IMPORT_FROM(sasuke)
	MDRV_CPU_MODIFY("main")
	MDRV_CPU_PROGRAM_MAP(satansat_map, 0)

	MDRV_MACHINE_RESET(satansat)

	// video hardware
	MDRV_GFXDECODE(satansat_gfxdecodeinfo)

	// sound hardware
	MDRV_SOUND_REPLACE("samples", SAMPLES, 0)
	MDRV_SOUND_CONFIG(vanguard_samples_interface)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.25)

	MDRV_SOUND_REPLACE("SN76477.1", SN76477, 0)
	MDRV_SOUND_CONFIG(satansat_sn76477_intf)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.0)

	MDRV_SOUND_REMOVE("SN76477.2")
	MDRV_SOUND_REMOVE("SN76477.3")
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( vanguard )
	// basic machine hardware
	//MDRV_CPU_ADD_TAG("main", M6502, 11289000/8)   // 1.4 MHz
	MDRV_CPU_ADD_TAG("main", M6502, 930000)		// adjusted
	MDRV_CPU_PROGRAM_MAP(vanguard_map, 0)
	MDRV_CPU_VBLANK_INT(rockola_interrupt, 2)

	MDRV_FRAMES_PER_SECOND((11289000.0 / 16) / (45 * 32 * 8))
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	MDRV_MACHINE_RESET(vanguard)

	// video hardware
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(32*8, 32*8)
	MDRV_VISIBLE_AREA(0*8, 32*8-1, 0*8, 28*8-1)
	MDRV_GFXDECODE(vanguard_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(64)
	MDRV_COLORTABLE_LENGTH(16*4)

	MDRV_PALETTE_INIT(rockola)
	MDRV_VIDEO_START(rockola)
	MDRV_VIDEO_UPDATE(rockola)

	// sound hardware
	MDRV_SPEAKER_STANDARD_MONO("mono")

	MDRV_SOUND_ADD(CUSTOM, 0)
	MDRV_SOUND_CONFIG(custom_interface)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.50)

	MDRV_SOUND_ADD_TAG("samples", SAMPLES, 0)
	MDRV_SOUND_CONFIG(vanguard_samples_interface)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.25)

	MDRV_SOUND_ADD_TAG("SN76477.1", SN76477, 0)
	MDRV_SOUND_CONFIG(vanguard_sn76477_intf_1)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.50)

	MDRV_SOUND_ADD_TAG("SN76477.2", SN76477, 0)
	MDRV_SOUND_CONFIG(vanguard_sn76477_intf_2)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.25)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( fantasy )
	// basic machine hardware
	MDRV_IMPORT_FROM(vanguard)
	MDRV_CPU_MODIFY("main")
	MDRV_CPU_PROGRAM_MAP(fantasy_map, 0)

	MDRV_MACHINE_RESET(fantasy)

	// sound hardware
	MDRV_SOUND_REPLACE("samples", SAMPLES, 0)
	MDRV_SOUND_CONFIG(fantasy_samples_interface)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.25)

	MDRV_SOUND_REPLACE("SN76477.1", SN76477, 0)
	MDRV_SOUND_CONFIG(fantasy_sn76477_intf)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.0)

	MDRV_SOUND_REMOVE("SN76477.2")
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( nibbler )
	// basic machine hardware
	MDRV_IMPORT_FROM(fantasy)

	// sound hardware
	MDRV_SOUND_REMOVE("samples")
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( pballoon )
	// basic machine hardware
	MDRV_IMPORT_FROM(nibbler)
	MDRV_CPU_MODIFY("main")
	MDRV_CPU_PROGRAM_MAP(pballoon_map, 0)

	MDRV_MACHINE_RESET(pballoon)

	// video hardware
	MDRV_VISIBLE_AREA(0*8, 32*8-1, 2*8, 30*8-1)
MACHINE_DRIVER_END

/* ROMs */

ROM_START( sasuke )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "sc1",          0x4000, 0x0800, CRC(34cbbe03) SHA1(3d643e11370e61dde0c42c7761a856c5cf53d621) )
	ROM_LOAD( "sc2",          0x4800, 0x0800, CRC(38cc14f0) SHA1(d60df67f2a32c131e8957e225b79618d6262463d) )
	ROM_LOAD( "sc3",          0x5000, 0x0800, CRC(54c41285) SHA1(5618c2ac745bbde96bfda7f01f7aee7e2b643d7e) )
	ROM_LOAD( "sc4",          0x5800, 0x0800, CRC(23edafcf) SHA1(bda3bcb506f6e23f422aafd7ca9b95bfb4d1d8e1) )
	ROM_LOAD( "sc5",          0x6000, 0x0800, CRC(ca410e4f) SHA1(0d09422d01b4359853c173a4cb18c9b5fbc7fe7c) )
	ROM_LOAD( "sc6",          0x6800, 0x0800, CRC(80406afb) SHA1(2c4a34a7450fa7258c5e6ead9b1fd6c6b973f081) )
	ROM_LOAD( "sc7",          0x7000, 0x0800, CRC(04d0f104) SHA1(73ed501f70d2a9e8994f8392f617450eafef39b3) )
	ROM_LOAD( "sc8",          0x7800, 0x0800, CRC(0219104b) SHA1(fd5c43304d59bc34e9ae6ef7576d75cf319d823e) )
	ROM_RELOAD(               0xf800, 0x0800 ) /* for the reset/interrupt vectors */
	ROM_LOAD( "sc9",          0x8000, 0x0800, CRC(d6ff889a) SHA1(1eea0366205dd0d9bffb5d093f259edc1d51cbe0) )
	ROM_LOAD( "sc10",         0x8800, 0x0800, CRC(19df6b9a) SHA1(95e904251c39dcef227a4c125fc573e958ee78b7) )

	ROM_REGION( 0x1000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "mcs_c",        0x0000, 0x0800, CRC(aff9743d) SHA1(a968a193ca551d92f79e09d1761dd2ccebc76eee) )
	ROM_LOAD( "mcs_d",        0x0800, 0x0800, CRC(9c805120) SHA1(74b83daa3ce3c9f7d96ad872b9134edd6f1bcb8a) )

	ROM_REGION( 0x0020, REGION_PROMS, 0 )
	ROM_LOAD( "sasuke.clr",   0x0000, 0x0020, CRC(b70f34c1) SHA1(890cfbb25e14112713ba7900b9cd56554a8bc1ec) )

	ROM_REGION( 0x1000, REGION_SOUND1, 0 )  /* sound data for Vanguard-style audio section */
	ROM_LOAD( "sc11",         0x0000, 0x0800, CRC(24a0e121) SHA1(e3cde355309de6678026d595955297258f069946) )
ROM_END

ROM_START( satansat )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "ss1",          0x4000, 0x0800, CRC(549dd13a) SHA1(06b55d0b1da84bef30857faa398aabfd04365eb6) )
	ROM_LOAD( "ss2",          0x4800, 0x0800, CRC(04972fa8) SHA1(89833a7c893168acd5599ca7ad4b33a8f3df40c5) )
	ROM_LOAD( "ss3",          0x5000, 0x0800, CRC(9caf9057) SHA1(26d439678e5e4d375ffac60126f45de599575bfd) )
	ROM_LOAD( "ss4",          0x5800, 0x0800, CRC(e1bdcfe1) SHA1(c99457c18fae8de79bbbe6bc0471fdc83f1e9b19) )
	ROM_LOAD( "ss5",          0x6000, 0x0800, CRC(d454de19) SHA1(ae8abb8a9d999d11ba6ad341bf635ae822d5746f) )
	ROM_LOAD( "ss6",          0x6800, 0x0800, CRC(7fbd5d30) SHA1(be0554ade440bf255131466ee8bd2905d3f446a8) )
	ROM_LOAD( "zarz128.15",   0x7000, 0x0800, CRC(93ea2df9) SHA1(4f7d076deef1e14b568b06974194861d3789ab5c) )
	ROM_LOAD( "zarz129.16",   0x7800, 0x0800, CRC(e67ec873) SHA1(14158914f07cabe61abc400c371d742ceb61d165) )
	ROM_RELOAD(               0xf800, 0x0800 ) /* for the reset/interrupt vectors */
	ROM_LOAD( "zarz130.22",   0x8000, 0x0800, CRC(22c44650) SHA1(063915cde86aece8860db1df15497cde669e73bd) )
	ROM_LOAD( "ss10",         0x8800, 0x0800, CRC(8f1b313a) SHA1(0c7832505a1287533d9b2d7f2d54000b3b44e40d) )
	ROM_LOAD( "ss11",         0x9000, 0x0800, CRC(e74f98e0) SHA1(89a93de6105195e0e5d255bfa240538ded155fb9) )

	ROM_REGION( 0x1000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "zarz135.73",   0x0000, 0x0800, CRC(e837c62b) SHA1(97552b1e413a3934f4dc5a6fc9fc1fa8ba7a2e7e) )
	ROM_LOAD( "zarz136.75",   0x0800, 0x0800, CRC(83f61623) SHA1(4cb28f85f32d13bfa364c376ea3e30fd451b5884) )

	ROM_REGION( 0x0020, REGION_PROMS, 0 )
	ROM_LOAD( "zarz138.03",   0x0000, 0x0020, CRC(5dd6933a) SHA1(417d827d9e47b6db01fecc2164e5ef332d4cd70e) )

	ROM_REGION( 0x1000, REGION_SOUND1, 0 )  /* sound data for Vanguard-style audio section */
	ROM_LOAD( "ss12",         0x0000, 0x0800, CRC(dee01f24) SHA1(92c8545226a31412239dad4aa2715b51264ad22e) )
	ROM_LOAD( "zarz134.54",   0x0800, 0x0800, CRC(580934d2) SHA1(c1c7eba56bca2a0ea6a68c0245b071a3308f92bd) )
ROM_END

ROM_START( zarzon )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "zarz122.07",   0x4000, 0x0800, CRC(bdfa67e2) SHA1(0de06cf53ee21b8f14b933b61e6dc706338746c4) )
	ROM_LOAD( "zarz123.08",   0x4800, 0x0800, CRC(d034e61e) SHA1(dc802c3d7a9f7e473e323e3272fca406dab6d55d) )
	ROM_LOAD( "zarz124.09",   0x5000, 0x0800, CRC(296397ea) SHA1(3a1ad7f3c4453bb20768b3e3ce04cd76873aa0ee) )
	ROM_LOAD( "zarz125.10",   0x5800, 0x0800, CRC(26dc5e66) SHA1(07f47f3497bb85640e5e6a89ad7d6579108347fe) )
	ROM_LOAD( "zarz126.13",   0x6000, 0x0800, CRC(cee18d7f) SHA1(5a7e60b6be06b3038f2eb81e76fc54cb65d4877b) )
	ROM_LOAD( "zarz127.14",   0x6800, 0x0800, CRC(bbd2cc0d) SHA1(1128020b7c0f38f5ff2cc2da0fd2df5ebead4681) )
	ROM_LOAD( "zarz128.15",   0x7000, 0x0800, CRC(93ea2df9) SHA1(4f7d076deef1e14b568b06974194861d3789ab5c) )
	ROM_LOAD( "zarz129.16",   0x7800, 0x0800, CRC(e67ec873) SHA1(14158914f07cabe61abc400c371d742ceb61d165) )
	ROM_RELOAD(               0xf800, 0x0800 ) /* for the reset/interrupt vectors */
	ROM_LOAD( "zarz130.22",   0x8000, 0x0800, CRC(22c44650) SHA1(063915cde86aece8860db1df15497cde669e73bd) )
	ROM_LOAD( "zarz131.23",   0x8800, 0x0800, CRC(7be20678) SHA1(872de953df1f9f1725e14fdfd227ad872a813af8) )
	ROM_LOAD( "zarz132.24",   0x9000, 0x0800, CRC(72b2cb76) SHA1(d7a95c908fe2227e2a0820a8e9713b1709c9e5af) )

	ROM_REGION( 0x1000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "zarz135.73",   0x0000, 0x0800, CRC(e837c62b) SHA1(97552b1e413a3934f4dc5a6fc9fc1fa8ba7a2e7e) )
	ROM_LOAD( "zarz136.75",   0x0800, 0x0800, CRC(83f61623) SHA1(4cb28f85f32d13bfa364c376ea3e30fd451b5884) )

	ROM_REGION( 0x0020, REGION_PROMS, 0 )
	ROM_LOAD( "zarz138.03",   0x0000, 0x0020, CRC(5dd6933a) SHA1(417d827d9e47b6db01fecc2164e5ef332d4cd70e) )

	ROM_REGION( 0x1000, REGION_SOUND1, 0 )  /* sound data for Vanguard-style audio section */
	ROM_LOAD( "zarz133.53",   0x0000, 0x0800, CRC(b253cf78) SHA1(56a73b22ed2866222c407a3e9b51b8e0c92cf2aa) )
	ROM_LOAD( "zarz134.54",   0x0800, 0x0800, CRC(580934d2) SHA1(c1c7eba56bca2a0ea6a68c0245b071a3308f92bd) )
ROM_END

ROM_START( vanguard )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "sk4_ic07.bin", 0x4000, 0x1000, CRC(6a29e354) SHA1(ff953962ebc14a28cfc96f8e269cb1e1c188ed8a) )
	ROM_LOAD( "sk4_ic08.bin", 0x5000, 0x1000, CRC(302bba54) SHA1(1944f229481328a0635fafda65054106f42a532a) )
	ROM_LOAD( "sk4_ic09.bin", 0x6000, 0x1000, CRC(424755f6) SHA1(b4762b40c7ed70d4b90319a1a30983a41a096afb) )
	ROM_LOAD( "sk4_ic10.bin", 0x7000, 0x1000, CRC(54603274) SHA1(31571a560dbe300417b3ed5b114fa1d9ef742da9) )
	ROM_LOAD( "sk4_ic13.bin", 0x8000, 0x1000, CRC(fde157d0) SHA1(3f705fb6a410004f4f86283694e3694e49701af6) )
	ROM_RELOAD(               0xf000, 0x1000 )	/* for the reset and interrupt vectors */
	ROM_LOAD( "sk4_ic14.bin", 0x9000, 0x1000, CRC(0d5b47d0) SHA1(922621c23f33fe756cb6baa12e5465c4e64f2dda) )
	ROM_LOAD( "sk4_ic15.bin", 0xa000, 0x1000, CRC(8549b8f8) SHA1(375bc6f7e15564d5cf7e00c44e2651793c56d6ca) )
	ROM_LOAD( "sk4_ic16.bin", 0xb000, 0x1000, CRC(062e0be2) SHA1(45aaf315a62f37460e32d3ba99caaacf4c994810) )

	ROM_REGION( 0x1000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "sk5_ic50.bin", 0x0000, 0x0800, CRC(e7d4315b) SHA1(b99e4ea07292a0eabaa6098037c92a5678627cec) )
	ROM_LOAD( "sk5_ic51.bin", 0x0800, 0x0800, CRC(96e87858) SHA1(4e9ccb055919c8acf5837e062857647d5363af60) )

	ROM_REGION( 0x0040, REGION_PROMS, 0 )
	ROM_LOAD( "sk5_ic7.bin",  0x0000, 0x0020, CRC(ad782a73) SHA1(ddf44f74a20f10ed976c434a885857dade1f86d7) ) /* foreground colors */
	ROM_LOAD( "sk5_ic6.bin",  0x0020, 0x0020, CRC(7dc9d450) SHA1(9b2d1dfb3270a562d14bd54bfb3405a9095becc0) ) /* background colors */

	ROM_REGION( 0x1000, REGION_SOUND1, 0 )	/* sound ROMs */
	ROM_LOAD( "sk4_ic51.bin", 0x0000, 0x0800, CRC(d2a64006) SHA1(3f20b59ce1954f65535cd5603ca9271586428e35) )  /* sound ROM 1 */
	ROM_LOAD( "sk4_ic52.bin", 0x0800, 0x0800, CRC(cc4a0b6f) SHA1(251b24d60083d516c4ba686d75b41e04d10f7198) )  /* sound ROM 2 */

	ROM_REGION( 0x5800, REGION_SOUND2, 0 )	/* space for the speech ROMs (not supported) */
	//ROM_LOAD( "hd38882.bin",  0x0000, 0x4000, NO_DUMP )   /* HD38882 internal ROM */
	ROM_LOAD( "sk6_ic07.bin", 0x4000, 0x0800, CRC(2b7cbae9) SHA1(3d44a0232d7c94d8170cc06e90cc30bd57c99202) )
	ROM_LOAD( "sk6_ic08.bin", 0x4800, 0x0800, CRC(3b7e9d7c) SHA1(d9033188068b2aaa1502c89cf09f955eded8fa7a) )
	ROM_LOAD( "sk6_ic11.bin", 0x5000, 0x0800, CRC(c36df041) SHA1(8b51934229b961180d1edb99be3a4d337d37f66f) )
ROM_END

ROM_START( vangrdce )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "sk4_ic07.bin", 0x4000, 0x1000, CRC(6a29e354) SHA1(ff953962ebc14a28cfc96f8e269cb1e1c188ed8a) )
	ROM_LOAD( "sk4_ic08.bin", 0x5000, 0x1000, CRC(302bba54) SHA1(1944f229481328a0635fafda65054106f42a532a) )
	ROM_LOAD( "sk4_ic09.bin", 0x6000, 0x1000, CRC(424755f6) SHA1(b4762b40c7ed70d4b90319a1a30983a41a096afb) )
	ROM_LOAD( "4",            0x7000, 0x1000, CRC(770f9714) SHA1(4af37fc24e464681a8da6b184be0df32a4078f4f) )
	ROM_LOAD( "5",            0x8000, 0x1000, CRC(3445cba6) SHA1(6afe6dad79b53df58b53ef9c5d24bb4d91fa5e8e) )
	ROM_RELOAD(               0xf000, 0x1000 )	/* for the reset and interrupt vectors */
	ROM_LOAD( "sk4_ic14.bin", 0x9000, 0x1000, CRC(0d5b47d0) SHA1(922621c23f33fe756cb6baa12e5465c4e64f2dda) )
	ROM_LOAD( "sk4_ic15.bin", 0xa000, 0x1000, CRC(8549b8f8) SHA1(375bc6f7e15564d5cf7e00c44e2651793c56d6ca) )
	ROM_LOAD( "8",            0xb000, 0x1000, CRC(4b825bc8) SHA1(3fa32d9677e2cc3a1ebf52c0b9eed7dbf11201e9) )

	ROM_REGION( 0x1000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "sk5_ic50.bin", 0x0000, 0x0800, CRC(e7d4315b) SHA1(b99e4ea07292a0eabaa6098037c92a5678627cec) )
	ROM_LOAD( "sk5_ic51.bin", 0x0800, 0x0800, CRC(96e87858) SHA1(4e9ccb055919c8acf5837e062857647d5363af60) )

	ROM_REGION( 0x0040, REGION_PROMS, 0 )
	ROM_LOAD( "sk5_ic7.bin",  0x0000, 0x0020, CRC(ad782a73) SHA1(ddf44f74a20f10ed976c434a885857dade1f86d7) ) /* foreground colors */
	ROM_LOAD( "sk5_ic6.bin",  0x0020, 0x0020, CRC(7dc9d450) SHA1(9b2d1dfb3270a562d14bd54bfb3405a9095becc0) ) /* background colors */

	ROM_REGION( 0x1000, REGION_SOUND1, 0 )	/* sound ROMs */
	ROM_LOAD( "sk4_ic51.bin", 0x0000, 0x0800, CRC(d2a64006) SHA1(3f20b59ce1954f65535cd5603ca9271586428e35) )  /* confirmed, 6/21/05 */
	ROM_LOAD( "sk4_ic52.bin", 0x0800, 0x0800, CRC(cc4a0b6f) SHA1(251b24d60083d516c4ba686d75b41e04d10f7198) )  /* confirmed, 6/21/05 */

	ROM_REGION( 0x5800, REGION_SOUND2, 0 )	/* space for the speech ROMs (not supported) */
	//ROM_LOAD( "hd38882.bin",  0x0000, 0x4000, NO_DUMP )   /* HD38882 internal ROM */
	ROM_LOAD( "sk6_ic07.bin", 0x4000, 0x0800, CRC(2b7cbae9) SHA1(3d44a0232d7c94d8170cc06e90cc30bd57c99202) )
	ROM_LOAD( "sk6_ic08.bin", 0x4800, 0x0800, CRC(3b7e9d7c) SHA1(d9033188068b2aaa1502c89cf09f955eded8fa7a) )
	ROM_LOAD( "sk6_ic11.bin", 0x5000, 0x0800, CRC(c36df041) SHA1(8b51934229b961180d1edb99be3a4d337d37f66f) )
ROM_END

ROM_START( vanguarj )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "sk4_ic07.bin", 0x4000, 0x1000, CRC(6a29e354) SHA1(ff953962ebc14a28cfc96f8e269cb1e1c188ed8a) )
	ROM_LOAD( "sk4_ic08.bin", 0x5000, 0x1000, CRC(302bba54) SHA1(1944f229481328a0635fafda65054106f42a532a) )
	ROM_LOAD( "sk4_ic09.bin", 0x6000, 0x1000, CRC(424755f6) SHA1(b4762b40c7ed70d4b90319a1a30983a41a096afb) )
	ROM_LOAD( "vgj4ic10.bin", 0x7000, 0x1000, CRC(0a91a5d1) SHA1(bef435e431e31179eb22a4c18ca1dedf6a4a0ab0) )
	ROM_LOAD( "vgj5ic13.bin", 0x8000, 0x1000, CRC(06601a40) SHA1(d1efcf75edf3892fe59d63e524f4880ffce67965) )
	ROM_RELOAD(               0xf000, 0x1000 )	/* for the reset and interrupt vectors */
	ROM_LOAD( "sk4_ic14.bin", 0x9000, 0x1000, CRC(0d5b47d0) SHA1(922621c23f33fe756cb6baa12e5465c4e64f2dda) )
	ROM_LOAD( "sk4_ic15.bin", 0xa000, 0x1000, CRC(8549b8f8) SHA1(375bc6f7e15564d5cf7e00c44e2651793c56d6ca) )
	ROM_LOAD( "sk4_ic16.bin", 0xb000, 0x1000, CRC(062e0be2) SHA1(45aaf315a62f37460e32d3ba99caaacf4c994810) )

	ROM_REGION( 0x1000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "sk5_ic50.bin", 0x0000, 0x0800, CRC(e7d4315b) SHA1(b99e4ea07292a0eabaa6098037c92a5678627cec) )
	ROM_LOAD( "sk5_ic51.bin", 0x0800, 0x0800, CRC(96e87858) SHA1(4e9ccb055919c8acf5837e062857647d5363af60) )

	ROM_REGION( 0x0040, REGION_PROMS, 0 )
	ROM_LOAD( "sk5_ic7.bin",  0x0000, 0x0020, CRC(ad782a73) SHA1(ddf44f74a20f10ed976c434a885857dade1f86d7) ) /* foreground colors */
	ROM_LOAD( "sk5_ic6.bin",  0x0020, 0x0020, CRC(7dc9d450) SHA1(9b2d1dfb3270a562d14bd54bfb3405a9095becc0) ) /* background colors */

	ROM_REGION( 0x1000, REGION_SOUND1, 0 )	/* sound ROMs */
	ROM_LOAD( "sk4_ic51.bin", 0x0000, 0x0800, CRC(d2a64006) SHA1(3f20b59ce1954f65535cd5603ca9271586428e35) )  /* sound ROM 1 */
	ROM_LOAD( "sk4_ic52.bin", 0x0800, 0x0800, CRC(cc4a0b6f) SHA1(251b24d60083d516c4ba686d75b41e04d10f7198) )  /* sound ROM 2 */

	ROM_REGION( 0x5800, REGION_SOUND2, 0 )	/* space for the speech ROMs (not supported) */
	//ROM_LOAD( "hd38882.bin",  0x0000, 0x4000, NO_DUMP )   /* HD38882 internal ROM */
	ROM_LOAD( "sk6_ic07.bin", 0x4000, 0x0800, CRC(2b7cbae9) SHA1(3d44a0232d7c94d8170cc06e90cc30bd57c99202) )
	ROM_LOAD( "sk6_ic08.bin", 0x4800, 0x0800, CRC(3b7e9d7c) SHA1(d9033188068b2aaa1502c89cf09f955eded8fa7a) )
	ROM_LOAD( "sk6_ic11.bin", 0x5000, 0x0800, CRC(c36df041) SHA1(8b51934229b961180d1edb99be3a4d337d37f66f) )
ROM_END

ROM_START( fantasyu )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "ic12.cpu",     0x3000, 0x1000, CRC(22cb2249) SHA1(6c43e3fa9638b6d2e069199968923e470bd5d18b) )
	ROM_LOAD( "ic07.cpu",     0x4000, 0x1000, CRC(0e2880b6) SHA1(666d6942864eb7a90178b3b6e2b0eb23aa3c967f) )
	ROM_LOAD( "ic08.cpu",     0x5000, 0x1000, CRC(4c331317) SHA1(800850f4e8bcfbbade54eb9e47a53941f8798641) )
	ROM_LOAD( "ic09.cpu",     0x6000, 0x1000, CRC(6ac1dbfc) SHA1(b9c7bf8d3b085db0b53646b5639c09f9ced2b1fe) )
	ROM_LOAD( "ic10.cpu",     0x7000, 0x1000, CRC(c796a406) SHA1(1b7f5f307a81b481a3e7791128a01d4c1a20c4bf) )
	ROM_LOAD( "ic14.cpu",     0x8000, 0x1000, CRC(6f1f0698) SHA1(05bd114dcd08c990d897518a8ea7965bc82279bf) )
	ROM_RELOAD(               0xf000, 0x1000 )	/* for the reset and interrupt vectors */
	ROM_LOAD( "ic15.cpu",     0x9000, 0x1000, CRC(5534d57e) SHA1(e564a3325766423b47de18d6adb61760cbbf88be) )
	ROM_LOAD( "ic16.cpu",     0xa000, 0x1000, CRC(6c2aeb6e) SHA1(fd0b913a663bf2a5f45fc3d342d7575a9c7dae46) )
	ROM_LOAD( "ic17.cpu",     0xb000, 0x1000, CRC(f6aa5de1) SHA1(ca53cf66cc6cdb21a60760102f35a5b0745ce09b) )

	ROM_REGION( 0x2000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "fs10ic50.bin", 0x0000, 0x1000, CRC(86a801c3) SHA1(c040b5807c25823072f7e8ceab57b95d4bed89fe) )
	ROM_LOAD( "fs11ic51.bin", 0x1000, 0x1000, CRC(9dfff71c) SHA1(7a7c017170f2ea903a730a4e5ab69db379a4fc61) )

	ROM_REGION( 0x0040, REGION_PROMS, 0 )
	ROM_LOAD( "fantasy.ic7",  0x0000, 0x0020, CRC(361a5e99) SHA1(b9777ce658549c03971bd476482d5cc0be27d3a9) ) /* foreground colors */
	ROM_LOAD( "fantasy.ic6",  0x0020, 0x0020, CRC(33d974f7) SHA1(a6f6a531dec3f454b477bfdda8e213e9cad42748) ) /* background colors */

	ROM_REGION( 0x1800, REGION_SOUND1, 0 )	/* sound ROMs */
	ROM_LOAD( "fs_b_51.bin",  0x0000, 0x0800, CRC(48094ec5) SHA1(7d6118133bc1eb8ebc5d8a95d10ef842daffef89) )
	ROM_LOAD( "fs_a_52.bin",  0x0800, 0x0800, CRC(1d0316e8) SHA1(6a3ab289b5fefef8663514bd1d5817c70fe58882) )
	ROM_LOAD( "fs_c_53.bin",  0x1000, 0x0800, CRC(49fd4ae8) SHA1(96ff1267c0ffab1e8a0769fa869516e2546ab640) )

	ROM_REGION( 0x5800, REGION_SOUND2, 0 )	/* space for the speech ROMs (not supported) */
	//ROM_LOAD( "hd38882.bin",  0x0000, 0x4000, NO_DUMP )   /* HD38882 internal ROM */
	ROM_LOAD( "fs_d_7.bin",   0x4000, 0x0800, CRC(a7ef4cc6) SHA1(8df71cb18fcfe9a2f592f83bc01cf2314ae30e32) )
	ROM_LOAD( "fs_e_8.bin",   0x4800, 0x0800, CRC(19b8fb3e) SHA1(271c76f68866c28bc6755238a71970d5f7c81ecb) )
	ROM_LOAD( "fs_f_11.bin",  0x5000, 0x0800, CRC(3a352e1f) SHA1(af880ce3daed0877d454421bd08c86ff71f6bf72) )
ROM_END

ROM_START( fantasy )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "5.12",         0x3000, 0x1000, CRC(0968ab50) SHA1(f09d03a171349895c5cb69e684901be63d272b32) )
	ROM_LOAD( "1.7",          0x4000, 0x1000, CRC(de83000e) SHA1(ede1dda46406b4d340f1efea3bc85b2227af9e1d) )
	ROM_LOAD( "2.8",          0x5000, 0x1000, CRC(90499b5a) SHA1(81a9d93a5655d2ff9504036bc764d8bb81e1470d) )
	ROM_LOAD( "3.9",          0x6000, 0x1000, CRC(6fbffeb6) SHA1(b36aeaf095da4957103c8921957ff4be658eddf5) )
	ROM_LOAD( "4.10",         0x7000, 0x1000, CRC(02e85884) SHA1(71fa6eb375fc417f92c049ec5118818b9ad48468) )
	ROM_LOAD( "ic14.cpu",     0x8000, 0x1000, CRC(6f1f0698) SHA1(05bd114dcd08c990d897518a8ea7965bc82279bf) )
	ROM_RELOAD(               0xf000, 0x1000 )	/* for the reset and interrupt vectors */
	ROM_LOAD( "ic15.cpu",     0x9000, 0x1000, CRC(5534d57e) SHA1(e564a3325766423b47de18d6adb61760cbbf88be) )
	ROM_LOAD( "8.16",         0xa000, 0x1000, CRC(371129fe) SHA1(c21759222aebcc9ea1292e367a41ac43a4dd3554) )
	ROM_LOAD( "9.17",         0xb000, 0x1000, CRC(56a7c8b8) SHA1(6c417644851c7b4b5291d9c5b2c808ff4a1ad048) )

	ROM_REGION( 0x2000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "fs10ic50.bin", 0x0000, 0x1000, CRC(86a801c3) SHA1(c040b5807c25823072f7e8ceab57b95d4bed89fe) )
	ROM_LOAD( "fs11ic51.bin", 0x1000, 0x1000, CRC(9dfff71c) SHA1(7a7c017170f2ea903a730a4e5ab69db379a4fc61) )

	ROM_REGION( 0x0040, REGION_PROMS, 0 )
	ROM_LOAD( "fantasy.ic7",  0x0000, 0x0020, CRC(361a5e99) SHA1(b9777ce658549c03971bd476482d5cc0be27d3a9) ) /* foreground colors */
	ROM_LOAD( "fantasy.ic6",  0x0020, 0x0020, CRC(33d974f7) SHA1(a6f6a531dec3f454b477bfdda8e213e9cad42748) ) /* background colors */

	ROM_REGION( 0x1800, REGION_SOUND1, 0 )	/* sound ROMs */
	ROM_LOAD( "fs_b_51.bin",  0x0000, 0x0800, CRC(48094ec5) SHA1(7d6118133bc1eb8ebc5d8a95d10ef842daffef89) )
	ROM_LOAD( "fs_a_52.bin",  0x0800, 0x0800, CRC(1d0316e8) SHA1(6a3ab289b5fefef8663514bd1d5817c70fe58882) )
	ROM_LOAD( "fs_c_53.bin",  0x1000, 0x0800, CRC(49fd4ae8) SHA1(96ff1267c0ffab1e8a0769fa869516e2546ab640) )

	ROM_REGION( 0x5800, REGION_SOUND2, 0 )	/* space for the speech ROMs (not supported) */
	//ROM_LOAD( "hd38882.bin",  0x0000, 0x4000, NO_DUMP )   /* HD38882 internal ROM */
	ROM_LOAD( "fs_d_7.bin",   0x4000, 0x0800, CRC(a7ef4cc6) SHA1(8df71cb18fcfe9a2f592f83bc01cf2314ae30e32) )
	ROM_LOAD( "fs_e_8.bin",   0x4800, 0x0800, CRC(19b8fb3e) SHA1(271c76f68866c28bc6755238a71970d5f7c81ecb) )
	ROM_LOAD( "fs_f_11.bin",  0x5000, 0x0800, CRC(3a352e1f) SHA1(af880ce3daed0877d454421bd08c86ff71f6bf72) )
ROM_END

ROM_START( fantasyj )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "fs5jic12.bin", 0x3000, 0x1000, CRC(dd1eac89) SHA1(d63078d4666e3c6db0c9b3f8b45ef81606ed5a4f) )
	ROM_LOAD( "fs1jic7.bin",  0x4000, 0x1000, CRC(7b8115ae) SHA1(6274f937c57ab9cbb7c6283022b81f70dad7c232) )
	ROM_LOAD( "fs2jic8.bin",  0x5000, 0x1000, CRC(61531dd1) SHA1(f3bc405bafc8ced6c6fce93ad2ad20ff6aa603e8) )
	ROM_LOAD( "fs3jic9.bin",  0x6000, 0x1000, CRC(36a12617) SHA1(dd74abb4cbaeb1a56ee466043997187ebe933612) )
	ROM_LOAD( "fs4jic10.bin", 0x7000, 0x1000, CRC(dbf7c347) SHA1(1bb3f924a7e1ec74ef68e237a0f68d62ce78532c) )
	ROM_LOAD( "fs6jic14.bin", 0x8000, 0x1000, CRC(bf59a33a) SHA1(bdbdd03199758069b904fdf0455193682c4d457f) )
	ROM_RELOAD(               0xf000, 0x1000 )	/* for the reset and interrupt vectors */
	ROM_LOAD( "fs7jic15.bin", 0x9000, 0x1000, CRC(cc18428e) SHA1(c7c0a031434cf9ce3c450b0c5dc2b154b08d19cf) )
	ROM_LOAD( "fs8jic16.bin", 0xa000, 0x1000, CRC(ae5bf727) SHA1(3c5eaaba3971f57a5687945a614dd0d6c9e007d6) )
	ROM_LOAD( "fs9jic17.bin", 0xb000, 0x1000, CRC(fa6903e2) SHA1(a5b9b7309ecaaeaba76e45610d5ab80415ecbdd0) )

	ROM_REGION( 0x2000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "fs10ic50.bin", 0x0000, 0x1000, CRC(86a801c3) SHA1(c040b5807c25823072f7e8ceab57b95d4bed89fe) )
	ROM_LOAD( "fs11ic51.bin", 0x1000, 0x1000, CRC(9dfff71c) SHA1(7a7c017170f2ea903a730a4e5ab69db379a4fc61) )

	ROM_REGION( 0x0040, REGION_PROMS, 0 )
	ROM_LOAD( "prom-8.bpr",   0x0000, 0x0020, CRC(1aa9285a) SHA1(d503aa76ca0cf032c7b1c962abc59677c41a2c62) ) /* foreground colors */
	ROM_LOAD( "prom-7.bpr",   0x0020, 0x0020, CRC(7a6f7dc3) SHA1(e15d898275d1cd205cc2d28f7dd9df653594039e) ) /* background colors */

	ROM_REGION( 0x1800, REGION_SOUND1, 0 )	/* sound ROMs */
	ROM_LOAD( "fs_b_51.bin",  0x0000, 0x0800, CRC(48094ec5) SHA1(7d6118133bc1eb8ebc5d8a95d10ef842daffef89) )
	ROM_LOAD( "fs_a_52.bin",  0x0800, 0x0800, CRC(1d0316e8) SHA1(6a3ab289b5fefef8663514bd1d5817c70fe58882) )
	ROM_LOAD( "fs_c_53.bin",  0x1000, 0x0800, CRC(49fd4ae8) SHA1(96ff1267c0ffab1e8a0769fa869516e2546ab640) )

	ROM_REGION( 0x5800, REGION_SOUND2, 0 )	/* space for the speech ROMs (not supported) */
	//ROM_LOAD( "hd38882.bin",  0x0000, 0x4000, NO_DUMP )   /* HD38882 internal ROM */
	ROM_LOAD( "fs_d_7.bin",   0x4000, 0x0800, CRC(a7ef4cc6) SHA1(8df71cb18fcfe9a2f592f83bc01cf2314ae30e32) )
	ROM_LOAD( "fs_e_8.bin",   0x4800, 0x0800, CRC(19b8fb3e) SHA1(271c76f68866c28bc6755238a71970d5f7c81ecb) )
	ROM_LOAD( "fs_f_11.bin",  0x5000, 0x0800, CRC(3a352e1f) SHA1(af880ce3daed0877d454421bd08c86ff71f6bf72) )
ROM_END

ROM_START( pballoon )
	ROM_REGION( 0x10000, REGION_CPU1, 0 ) /* 64k for code */
	ROM_LOAD( "sk7_ic12.bin", 0x3000, 0x1000, CRC(dfe2ae05) SHA1(21c98bef9d4d5fcb65ce5e9b20cde2259840459e) )
	ROM_LOAD( "sk7_ic07.bin", 0x4000, 0x1000, CRC(736e67df) SHA1(a58d9561f62d396ca90b0f69afe6240d809b10bb) )
	ROM_LOAD( "sk7_ic08.bin", 0x5000, 0x1000, CRC(7a2032b2) SHA1(79570943468d647cda67d94b20eac1b2d9eb371f) )
	ROM_LOAD( "sk7_ic09.bin", 0x6000, 0x1000, CRC(2d63cf3a) SHA1(8934af617229db445f9fd10e4028e1f8df4cfeb1) )
	ROM_LOAD( "sk7_ic10.bin", 0x7000, 0x1000, CRC(7b88cbd4) SHA1(1be3c484bd08c747f38389114c157e84319c48be) )
	ROM_LOAD( "sk7_ic14.bin", 0x8000, 0x1000, CRC(6a8817a5) SHA1(4cf8eda68d21b1fad0f12eedaeb88b256bba44da) )
	ROM_RELOAD(               0xf000, 0x1000 )  /* for the reset and interrupt vectors */
	ROM_LOAD( "sk7_ic15.bin", 0x9000, 0x1000, CRC(1f78d814) SHA1(7e618971f1bbf8859284531e94989c43c3285b4a) )

	ROM_REGION( 0x2000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "sk8_ic50.bin", 0x0000, 0x1000, CRC(560df07f) SHA1(e57945de829d22d39390a649eddaf78c989af679) )
	ROM_LOAD( "sk8_ic51.bin", 0x1000, 0x1000, CRC(d415de51) SHA1(257cf939efec8adee87baf827315c69fde90da4c) )

	ROM_REGION( 0x0040, REGION_PROMS, 0 )
	ROM_LOAD( "sk8_ic7.bin",  0x0000, 0x0020, CRC(ef6c82a0) SHA1(95b522d6389f25bf5fa2fca5f3f826ef43b2885b) ) /* foreground colors */
	ROM_LOAD( "sk8_ic6.bin",  0x0020, 0x0020, CRC(eabc6a00) SHA1(942af5e22e49e578c6a24651476e3b60d40e2076) ) /* background colors */

	ROM_REGION( 0x1800, REGION_SOUND1, 0 )	/* sound ROMs */
	ROM_LOAD( "sk7_ic51.bin", 0x0000, 0x0800, CRC(0345f8b7) SHA1(c00992dc7222cc53d9fdff4ab47a7abdf90c5116) )
	ROM_LOAD( "sk7_ic52.bin", 0x0800, 0x0800, CRC(5d6d68ea) SHA1(d3e03720eff5c85c1c2fb1d4bf960f45a99dc86a) )
	ROM_LOAD( "sk7_ic53.bin", 0x1000, 0x0800, CRC(a4c505cd) SHA1(47eea7e7ffa3dc8b35dc050ac1a1d77d6a5c4ece) )
ROM_END

ROM_START( nibbler )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "g-0960-52.ic12", 0x3000, 0x1000, CRC(ac6a802b) SHA1(ac1072e30994f13097663dc24d9d1dc35a95d874) )
	ROM_LOAD( "g-0960-48.ic7",  0x4000, 0x1000, CRC(35971364) SHA1(6430c7be9e5f47d3f1f2cc157d949246e4085e8b) )
	ROM_LOAD( "g-0960-49.ic8",  0x5000, 0x1000, CRC(6b33b806) SHA1(29444e45bf5a6ab1d86e0aa19dc6c1bc64ba633f) )
	ROM_LOAD( "g-0960-50.ic9",  0x6000, 0x1000, CRC(91a4f98d) SHA1(678c7e8c91a7fdba8dc2faff4192eb0964abdb3f) )
	ROM_LOAD( "g-0960-51.ic10", 0x7000, 0x1000, CRC(a151d934) SHA1(6681bdcd84cf62b40b2430ff530cb3c9aa36656c) )
	ROM_LOAD( "g-0960-53.ic14", 0x8000, 0x1000, CRC(063f05cc) SHA1(039ac1b007cb817ae0902484ca611ae7076930d6) )
	ROM_RELOAD(                 0xf000, 0x1000 )	/* for the reset and interrupt vectors */
	ROM_LOAD( "g-0960-54.ic15", 0x9000, 0x1000, CRC(7205fb8d) SHA1(bc341bc11a383aa8b8dd7b2be851907a3ec56f8b) )
	ROM_LOAD( "g-0960-55.ic16", 0xa000, 0x1000, CRC(4bb39815) SHA1(1755c28d7d300524ab839aedcc744254544e9c19) )
	ROM_LOAD( "g-0960-56.ic17", 0xb000, 0x1000, CRC(ed680f19) SHA1(b44203585f32ebe2a3bf0597eac7c0faa7e81a92) )

	ROM_REGION( 0x2000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "g-0960-57.ic50", 0x0000, 0x1000, CRC(01d4d0c2) SHA1(5a8026210a872351ce4e39e27f6479d3ca0689e2) )
	ROM_LOAD( "g-0960-58.ic51", 0x1000, 0x1000, CRC(feff7faf) SHA1(50005502578a4ea9b9c8f36998670b787d2d0b20) )

	ROM_REGION( 0x0040, REGION_PROMS, 0 )
	ROM_LOAD( "g-0708-05.ic7",  0x0000, 0x0020, CRC(a5709ff3) SHA1(fbd07b756235f2d03aea3d777ca741ade54be200) ) /* foreground colors */
	ROM_LOAD( "g-0708-04.ic6",  0x0020, 0x0020, CRC(dacd592d) SHA1(c7709c680e2764885a40bc256d07dffc9e827cd6) ) /* background colors */

	ROM_REGION( 0x1800, REGION_SOUND1, 0 )	/* sound ROMs */
	ROM_LOAD( "g-0959-43.ic51", 0x0000, 0x0800, CRC(0345f8b7) SHA1(c00992dc7222cc53d9fdff4ab47a7abdf90c5116) )
	ROM_LOAD( "g-0959-44.ic52", 0x0800, 0x0800, CRC(87d67dee) SHA1(bd292eab3671cb953279f3136a450deac3818367) )
	ROM_LOAD( "g-0959-45.ic53", 0x1000, 0x0800, CRC(33189917) SHA1(01a1b1693db0172609780daeb60430fa0c8bcec2) )
ROM_END

ROM_START( nibblera )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "ic12",           0x3000, 0x1000, CRC(6dfa1be5) SHA1(bb265702a2f74cb7d5ba27081f9fb2fe01dd95a5) )
	ROM_LOAD( "ic7",            0x4000, 0x1000, CRC(808e1a03) SHA1(a747a16ee0c8cb803b72ac84e80f791b2bf1813a) )
	ROM_LOAD( "ic8",            0x5000, 0x1000, CRC(1571d4a2) SHA1(42cbaa262c2265d904fd5844c0d3c63d3beb67a8) )
	ROM_LOAD( "ic9",            0x6000, 0x1000, CRC(a599df10) SHA1(68ee8b5199ec24409fcbb40c887a1eec44c68dcf) )
	ROM_LOAD( "ic10",           0x7000, 0x1000, CRC(a6b5abe5) SHA1(a0f228dac801a54dfa1947d6b2f6b4e3d005e0b2) )
	ROM_LOAD( "ic14",           0x8000, 0x1000, CRC(9f537185) SHA1(619df63f4df38014dc229f614043f867e6a5aa51) )
	ROM_RELOAD(                 0xf000, 0x1000 )	/* for the reset and interrupt vectors */
	ROM_LOAD( "g-0960-54.ic15", 0x9000, 0x1000, CRC(7205fb8d) SHA1(bc341bc11a383aa8b8dd7b2be851907a3ec56f8b) )
	ROM_LOAD( "g-0960-55.ic16", 0xa000, 0x1000, CRC(4bb39815) SHA1(1755c28d7d300524ab839aedcc744254544e9c19) )
	ROM_LOAD( "g-0960-56.ic17", 0xb000, 0x1000, CRC(ed680f19) SHA1(b44203585f32ebe2a3bf0597eac7c0faa7e81a92) )

	ROM_REGION( 0x2000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "g-0960-57.ic50", 0x0000, 0x1000, CRC(01d4d0c2) SHA1(5a8026210a872351ce4e39e27f6479d3ca0689e2) )
	ROM_LOAD( "g-0960-58.ic51", 0x1000, 0x1000, CRC(feff7faf) SHA1(50005502578a4ea9b9c8f36998670b787d2d0b20) )

	ROM_REGION( 0x0040, REGION_PROMS, 0 )
	ROM_LOAD( "g-0708-05.ic7",  0x0000, 0x0020, CRC(a5709ff3) SHA1(fbd07b756235f2d03aea3d777ca741ade54be200) ) /* foreground colors */
	ROM_LOAD( "g-0708-04.ic6",  0x0020, 0x0020, CRC(dacd592d) SHA1(c7709c680e2764885a40bc256d07dffc9e827cd6) ) /* background colors */

	ROM_REGION( 0x1800, REGION_SOUND1, 0 )	/* sound ROMs */
	ROM_LOAD( "g-0959-43.ic51", 0x0000, 0x0800, CRC(0345f8b7) SHA1(c00992dc7222cc53d9fdff4ab47a7abdf90c5116) )
	ROM_LOAD( "g-0959-44.ic52", 0x0800, 0x0800, CRC(87d67dee) SHA1(bd292eab3671cb953279f3136a450deac3818367) )
	ROM_LOAD( "g-0959-45.ic53", 0x1000, 0x0800, CRC(33189917) SHA1(01a1b1693db0172609780daeb60430fa0c8bcec2) )
ROM_END

ROM_START( nibblerb )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "50-144.012",     0x3000, 0x1000, CRC(68af8f4b) SHA1(be6ddd3a9abb05563c927b1ec54dbaab44b65492) )
	ROM_LOAD( "50-140.007",     0x4000, 0x1000, CRC(c18b3009) SHA1(c3703d0300f5f1546417ecdc27ab747d9c7eb267) )
	ROM_LOAD( "50-141.008",     0x5000, 0x1000, CRC(b50fd79c) SHA1(cd9847bf8d570ca9411d1bbcbccb3c94220349f9) )
	ROM_LOAD( "ic9",            0x6000, 0x1000, CRC(a599df10) SHA1(68ee8b5199ec24409fcbb40c887a1eec44c68dcf) ) // 50-142.009
	ROM_LOAD( "ic10",           0x7000, 0x1000, CRC(a6b5abe5) SHA1(a0f228dac801a54dfa1947d6b2f6b4e3d005e0b2) ) // 50-143.010
	ROM_LOAD( "50-145.014",     0x8000, 0x1000, CRC(29ea246a) SHA1(bf1afbddbea5ab7e93e5ac69c6445749dd65ed3b) )
	ROM_RELOAD(                 0xf000, 0x1000 )	/* for the reset and interrupt vectors */
	ROM_LOAD( "g-0960-54.ic15", 0x9000, 0x1000, CRC(7205fb8d) SHA1(bc341bc11a383aa8b8dd7b2be851907a3ec56f8b) ) // 50-146.015
	ROM_LOAD( "g-0960-55.ic16", 0xa000, 0x1000, CRC(4bb39815) SHA1(1755c28d7d300524ab839aedcc744254544e9c19) ) // 50-147.016
	ROM_LOAD( "g-0960-56.ic17", 0xb000, 0x1000, CRC(ed680f19) SHA1(b44203585f32ebe2a3bf0597eac7c0faa7e81a92) ) // 50-148.017

	ROM_REGION( 0x2000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "g-0960-57.ic50", 0x0000, 0x1000, CRC(01d4d0c2) SHA1(5a8026210a872351ce4e39e27f6479d3ca0689e2) ) // 50-150.051
	ROM_LOAD( "g-0960-58.ic51", 0x1000, 0x1000, CRC(feff7faf) SHA1(50005502578a4ea9b9c8f36998670b787d2d0b20) ) // 50-149.050

	ROM_REGION( 0x0040, REGION_PROMS, 0 )
	ROM_LOAD( "g-0708-05.ic7",  0x0000, 0x0020, CRC(a5709ff3) SHA1(fbd07b756235f2d03aea3d777ca741ade54be200) ) /* foreground colors */
	ROM_LOAD( "g-0708-04.ic6",  0x0020, 0x0020, CRC(dacd592d) SHA1(c7709c680e2764885a40bc256d07dffc9e827cd6) ) /* background colors */

	ROM_REGION( 0x1800, REGION_SOUND1, 0 )	/* sound ROMs */
	ROM_LOAD( "g-0959-43.ic51", 0x0000, 0x0800, CRC(0345f8b7) SHA1(c00992dc7222cc53d9fdff4ab47a7abdf90c5116) ) // not in this set / board according to readme but it seems to be needed?!
	ROM_LOAD( "g-0959-44.ic52", 0x0800, 0x0800, CRC(87d67dee) SHA1(bd292eab3671cb953279f3136a450deac3818367) ) // 50-152.052
	ROM_LOAD( "g-0959-45.ic53", 0x1000, 0x0800, CRC(33189917) SHA1(01a1b1693db0172609780daeb60430fa0c8bcec2) ) // 50-151.053
ROM_END

ROM_START( nibblero )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "50-144g.012",    0x3000, 0x1000, CRC(1093f525) SHA1(6a63372300765acdbac1d2e30fd73af7773de80f) )
	ROM_LOAD( "50-140g.007",    0x4000, 0x1000, CRC(848651dd) SHA1(a5aafbcca42baca8d0d5d28546733aefc778ba99) )
	ROM_LOAD( "50-141.008",     0x5000, 0x1000, CRC(b50fd79c) SHA1(cd9847bf8d570ca9411d1bbcbccb3c94220349f9) )
	ROM_LOAD( "ic9",            0x6000, 0x1000, CRC(a599df10) SHA1(68ee8b5199ec24409fcbb40c887a1eec44c68dcf) )
	ROM_LOAD( "ic10",           0x7000, 0x1000, CRC(a6b5abe5) SHA1(a0f228dac801a54dfa1947d6b2f6b4e3d005e0b2) )
	ROM_LOAD( "50-145.014",     0x8000, 0x1000, CRC(29ea246a) SHA1(bf1afbddbea5ab7e93e5ac69c6445749dd65ed3b) )
	ROM_RELOAD(                 0xf000, 0x1000 )	/* for the reset and interrupt vectors */
	ROM_LOAD( "g-0960-54.ic15", 0x9000, 0x1000, CRC(7205fb8d) SHA1(bc341bc11a383aa8b8dd7b2be851907a3ec56f8b) )
	ROM_LOAD( "g-0960-55.ic16", 0xa000, 0x1000, CRC(4bb39815) SHA1(1755c28d7d300524ab839aedcc744254544e9c19) )
	ROM_LOAD( "g-0960-56.ic17", 0xb000, 0x1000, CRC(ed680f19) SHA1(b44203585f32ebe2a3bf0597eac7c0faa7e81a92) )

	ROM_REGION( 0x2000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "g-0960-57.ic50", 0x0000, 0x1000, CRC(01d4d0c2) SHA1(5a8026210a872351ce4e39e27f6479d3ca0689e2) )
	ROM_LOAD( "g-0960-58.ic51", 0x1000, 0x1000, CRC(feff7faf) SHA1(50005502578a4ea9b9c8f36998670b787d2d0b20) )

	ROM_REGION( 0x0040, REGION_PROMS, 0 )
	ROM_LOAD( "g-0708-05.ic7",  0x0000, 0x0020, CRC(a5709ff3) SHA1(fbd07b756235f2d03aea3d777ca741ade54be200) ) /* foreground colors */
	ROM_LOAD( "g-0708-04.ic6",  0x0020, 0x0020, CRC(dacd592d) SHA1(c7709c680e2764885a40bc256d07dffc9e827cd6) ) /* background colors */

	ROM_REGION( 0x1800, REGION_SOUND1, 0 )	/* sound ROMs */
	ROM_LOAD( "g-0959-43.ic51", 0x0000, 0x0800, CRC(0345f8b7) SHA1(c00992dc7222cc53d9fdff4ab47a7abdf90c5116) )
	ROM_LOAD( "g-0959-44.ic52", 0x0800, 0x0800, CRC(87d67dee) SHA1(bd292eab3671cb953279f3136a450deac3818367) )
	ROM_LOAD( "g-0959-45.ic53", 0x1000, 0x0800, CRC(33189917) SHA1(01a1b1693db0172609780daeb60430fa0c8bcec2) )
ROM_END

/* Game Drivers */

GAME( 1980, sasuke,   0,        sasuke,   sasuke,   0, ROT90, "SNK", "Sasuke vs. Commander", 0 )
GAME( 1981, satansat, 0,        satansat, satansat, 0, ROT90, "SNK", "Satan of Saturn", GAME_IMPERFECT_SOUND )
GAME( 1981, zarzon,   satansat, satansat, satansat, 0, ROT90, "[SNK] (Taito America license)", "Zarzon", GAME_IMPERFECT_SOUND )
GAME( 1981, vanguard, 0,        vanguard, vanguard, 0, ROT90, "SNK", "Vanguard (SNK)", 0 )
GAME( 1981, vangrdce, vanguard, vanguard, vanguard, 0, ROT90, "SNK (Centuri license)", "Vanguard (Centuri)", 0 )
GAME( 1981, vanguarj,  vanguard, vanguard, vanguard, 0, ROT90, "SNK", "Vanguard (Japan)", 0 )
GAME( 1981, fantasy,  0,        fantasy,  fantasy,  0, ROT90, "SNK", "Fantasy (World)", 0 )
GAME( 1981, fantasyu, fantasy,  fantasy,  fantasy,  0, ROT90, "[SNK] (Rock-Ola license)", "Fantasy (US)", 0 )
GAME( 1981, fantasyj, fantasy,  fantasy,  fantasy,  0, ROT90, "SNK", "Fantasy (Japan)", 0 )
GAME( 1982, pballoon, 0,        pballoon, pballoon, 0, ROT90, "SNK", "Pioneer Balloon", 0 )
GAME( 1982, nibbler,  0,        nibbler,  nibbler,  0, ROT90, "Rock-Ola", "Nibbler (set 1)", 0 )
GAME( 1982, nibblera, nibbler,  nibbler,  nibbler,  0, ROT90, "Rock-Ola", "Nibbler (set 2)", 0 )
GAME( 1982, nibblerb, nibbler,  nibbler,  nibbler,  0, ROT90, "Rock-Ola", "Nibbler (set 3)", 0 )
GAME( 1983, nibblero, nibbler,  nibbler,  nibbler,  0, ROT90, "Olympia",  "Nibbler (Olympia)", 0 )
