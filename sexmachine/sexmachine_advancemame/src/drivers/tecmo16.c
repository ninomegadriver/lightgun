/******************************************************************************

  Ganbare Ginkun  (Japan)  (c)1995 TECMO
  Final StarForce (US)     (c)1992 TECMO
  Riot            (Japan)  (c)1992 NMK


--
driver by Eisuke Watanabe, Nicola Salmoria

special thanks to Nekomata, NTD & code-name'Siberia'

TODO:
- wrong background in fstarfrc title
- there could be some priorities problems in riot
  (more noticeable in level 2)

Notes:
- To enter into service mode in Final Star Force press and hold start
  buttons 1 and 2 during P.O.S.T.
- The games seem to be romswaps. At least you can swap Riot roms on the pcb
  with Final Star Force and it'll work without problems.

******************************************************************************/

#include "driver.h"
#include "cpu/m68000/m68000.h"
#include "cpu/z80/z80.h"
#include "sound/2151intf.h"
#include "sound/okim6295.h"


extern UINT16 *tecmo16_videoram;
extern UINT16 *tecmo16_colorram;
extern UINT16 *tecmo16_videoram2;
extern UINT16 *tecmo16_colorram2;
extern UINT16 *tecmo16_charram;

WRITE16_HANDLER( tecmo16_videoram_w );
WRITE16_HANDLER( tecmo16_colorram_w );
WRITE16_HANDLER( tecmo16_videoram2_w );
WRITE16_HANDLER( tecmo16_colorram2_w );
WRITE16_HANDLER( tecmo16_charram_w );
WRITE16_HANDLER( tecmo16_flipscreen_w );

WRITE16_HANDLER( tecmo16_scroll_x_w );
WRITE16_HANDLER( tecmo16_scroll_y_w );
WRITE16_HANDLER( tecmo16_scroll2_x_w );
WRITE16_HANDLER( tecmo16_scroll2_y_w );
WRITE16_HANDLER( tecmo16_scroll_char_x_w );
WRITE16_HANDLER( tecmo16_scroll_char_y_w );

VIDEO_START( fstarfrc );
VIDEO_START( ginkun );
VIDEO_START( riot );
VIDEO_UPDATE( tecmo16 );

/******************************************************************************/

static WRITE16_HANDLER( tecmo16_sound_command_w )
{
	if (ACCESSING_LSB)
	{
		soundlatch_w(0x00,data & 0xff);
		cpunum_set_input_line(1,INPUT_LINE_NMI,PULSE_LINE);
	}
}

/******************************************************************************/

static ADDRESS_MAP_START( fstarfrc_readmem, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x07ffff) AM_READ(MRA16_ROM)
	AM_RANGE(0x100000, 0x103fff) AM_READ(MRA16_RAM)
	AM_RANGE(0x110000, 0x110fff) AM_READ(MRA16_RAM)
	AM_RANGE(0x120000, 0x1207ff) AM_READ(MRA16_RAM)
	AM_RANGE(0x120800, 0x120fff) AM_READ(MRA16_RAM)
	AM_RANGE(0x121000, 0x1217ff) AM_READ(MRA16_RAM)
	AM_RANGE(0x121800, 0x121fff) AM_READ(MRA16_RAM)
	AM_RANGE(0x122000, 0x127fff) AM_READ(MRA16_RAM)
	AM_RANGE(0x130000, 0x130fff) AM_READ(MRA16_RAM)
	AM_RANGE(0x140000, 0x141fff) AM_READ(MRA16_RAM)
	AM_RANGE(0x150030, 0x150031) AM_READ(input_port_1_word_r)
	AM_RANGE(0x150040, 0x150041) AM_READ(input_port_0_word_r)
	AM_RANGE(0x150050, 0x150051) AM_READ(input_port_2_word_r)
ADDRESS_MAP_END

static ADDRESS_MAP_START( fstarfrc_writemem, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x07ffff) AM_WRITE(MWA16_ROM)
	AM_RANGE(0x100000, 0x103fff) AM_WRITE(MWA16_RAM)	/* Main RAM */
	AM_RANGE(0x110000, 0x110fff) AM_WRITE(tecmo16_charram_w) AM_BASE(&tecmo16_charram)
	AM_RANGE(0x120000, 0x1207ff) AM_WRITE(tecmo16_videoram_w) AM_BASE(&tecmo16_videoram)
	AM_RANGE(0x120800, 0x120fff) AM_WRITE(tecmo16_colorram_w) AM_BASE(&tecmo16_colorram)
	AM_RANGE(0x121000, 0x1217ff) AM_WRITE(tecmo16_videoram2_w) AM_BASE(&tecmo16_videoram2)
	AM_RANGE(0x121800, 0x121fff) AM_WRITE(tecmo16_colorram2_w) AM_BASE(&tecmo16_colorram2)
	AM_RANGE(0x122000, 0x127fff) AM_WRITE(MWA16_RAM)	/* work area */
	AM_RANGE(0x130000, 0x130fff) AM_WRITE(MWA16_RAM) AM_BASE(&spriteram16) AM_SIZE(&spriteram_size)
	AM_RANGE(0x140000, 0x141fff) AM_WRITE(paletteram16_xxxxBBBBGGGGRRRR_word_w) AM_BASE(&paletteram16)
	AM_RANGE(0x150000, 0x150001) AM_WRITE(tecmo16_flipscreen_w)
	AM_RANGE(0x150010, 0x150011) AM_WRITE(tecmo16_sound_command_w)
	AM_RANGE(0x150030, 0x150031) AM_WRITE(MWA16_NOP)	/* ??? */
	AM_RANGE(0x160000, 0x160001) AM_WRITE(tecmo16_scroll_char_x_w)
	AM_RANGE(0x16000c, 0x16000d) AM_WRITE(tecmo16_scroll_x_w)
	AM_RANGE(0x160012, 0x160013) AM_WRITE(tecmo16_scroll_y_w)
	AM_RANGE(0x160018, 0x160019) AM_WRITE(tecmo16_scroll2_x_w)
	AM_RANGE(0x16001e, 0x16001f) AM_WRITE(tecmo16_scroll2_y_w)
ADDRESS_MAP_END

static ADDRESS_MAP_START( ginkun_readmem, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x07ffff) AM_READ(MRA16_ROM)
	AM_RANGE(0x100000, 0x103fff) AM_READ(MRA16_RAM)
	AM_RANGE(0x110000, 0x110fff) AM_READ(MRA16_RAM)
	AM_RANGE(0x120000, 0x120fff) AM_READ(MRA16_RAM)
	AM_RANGE(0x121000, 0x121fff) AM_READ(MRA16_RAM)
	AM_RANGE(0x122000, 0x122fff) AM_READ(MRA16_RAM)
	AM_RANGE(0x123000, 0x123fff) AM_READ(MRA16_RAM)
	AM_RANGE(0x124000, 0x124fff) AM_READ(MRA16_RAM)
	AM_RANGE(0x130000, 0x130fff) AM_READ(MRA16_RAM)
	AM_RANGE(0x140000, 0x141fff) AM_READ(MRA16_RAM)
	AM_RANGE(0x150020, 0x150021) AM_READ(input_port_3_word_r)
	AM_RANGE(0x150030, 0x150031) AM_READ(input_port_1_word_r)
	AM_RANGE(0x150040, 0x150041) AM_READ(input_port_0_word_r)
	AM_RANGE(0x150050, 0x150051) AM_READ(input_port_2_word_r)
ADDRESS_MAP_END

static ADDRESS_MAP_START( ginkun_writemem, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x07ffff) AM_WRITE(MWA16_ROM)
	AM_RANGE(0x100000, 0x103fff) AM_WRITE(MWA16_RAM)	/* Main RAM */
	AM_RANGE(0x110000, 0x110fff) AM_WRITE(tecmo16_charram_w) AM_BASE(&tecmo16_charram)
	AM_RANGE(0x120000, 0x120fff) AM_WRITE(tecmo16_videoram_w) AM_BASE(&tecmo16_videoram)
	AM_RANGE(0x121000, 0x121fff) AM_WRITE(tecmo16_colorram_w) AM_BASE(&tecmo16_colorram)
	AM_RANGE(0x122000, 0x122fff) AM_WRITE(tecmo16_videoram2_w) AM_BASE(&tecmo16_videoram2)
	AM_RANGE(0x123000, 0x123fff) AM_WRITE(tecmo16_colorram2_w) AM_BASE(&tecmo16_colorram2)
	AM_RANGE(0x124000, 0x124fff) AM_WRITE(MWA16_RAM)	/* extra RAM for Riot */
	AM_RANGE(0x130000, 0x130fff) AM_WRITE(MWA16_RAM) AM_BASE(&spriteram16) AM_SIZE(&spriteram_size)
	AM_RANGE(0x140000, 0x141fff) AM_WRITE(paletteram16_xxxxBBBBGGGGRRRR_word_w) AM_BASE(&paletteram16)
	AM_RANGE(0x150000, 0x150001) AM_WRITE(tecmo16_flipscreen_w)
	AM_RANGE(0x150010, 0x150011) AM_WRITE(tecmo16_sound_command_w)
	AM_RANGE(0x150020, 0x150021) AM_WRITE(MWA16_NOP)	/* ??? */
	AM_RANGE(0x150030, 0x150031) AM_WRITE(MWA16_NOP)	/* ??? */
	AM_RANGE(0x160000, 0x160001) AM_WRITE(tecmo16_scroll_char_x_w)
	AM_RANGE(0x160006, 0x160007) AM_WRITE(tecmo16_scroll_char_y_w)
	AM_RANGE(0x16000c, 0x16000d) AM_WRITE(tecmo16_scroll_x_w)
	AM_RANGE(0x160012, 0x160013) AM_WRITE(tecmo16_scroll_y_w)
	AM_RANGE(0x160018, 0x160019) AM_WRITE(tecmo16_scroll2_x_w)
	AM_RANGE(0x16001e, 0x16001f) AM_WRITE(tecmo16_scroll2_y_w)
ADDRESS_MAP_END

static ADDRESS_MAP_START( sound_readmem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0xefff) AM_READ(MRA8_ROM)
	AM_RANGE(0xf000, 0xfbff) AM_READ(MRA8_RAM)	/* Sound RAM */
	AM_RANGE(0xfc00, 0xfc00) AM_READ(OKIM6295_status_0_r)
	AM_RANGE(0xfc05, 0xfc05) AM_READ(YM2151_status_port_0_r)
	AM_RANGE(0xfc08, 0xfc08) AM_READ(soundlatch_r)
	AM_RANGE(0xfc0c, 0xfc0c) AM_READ(MRA8_NOP)
	AM_RANGE(0xfffe, 0xffff) AM_READ(MRA8_RAM)
ADDRESS_MAP_END

static ADDRESS_MAP_START( sound_writemem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0xefff) AM_WRITE(MWA8_ROM)
	AM_RANGE(0xf000, 0xfbff) AM_WRITE(MWA8_RAM)	/* Sound RAM */
	AM_RANGE(0xfc00, 0xfc00) AM_WRITE(OKIM6295_data_0_w)
	AM_RANGE(0xfc04, 0xfc04) AM_WRITE(YM2151_register_port_0_w)
	AM_RANGE(0xfc05, 0xfc05) AM_WRITE(YM2151_data_port_0_w)
	AM_RANGE(0xfc0c, 0xfc0c) AM_WRITE(MWA8_NOP)
	AM_RANGE(0xfffe, 0xffff) AM_WRITE(MWA8_RAM)
ADDRESS_MAP_END

/******************************************************************************/

INPUT_PORTS_START( fstarfrc )
	PORT_START			/* DIP SW 1 */
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_4C ) )
	PORT_DIPNAME( 0x0c, 0x0c, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x0c, DEF_STR( 1C_1C ) )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x20, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Allow_Continue ) )
	PORT_DIPSETTING(    0x00, DEF_STR( No ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Free_Play ) )	//flagged as "unused" in the manual
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START			/* DIP SW 2 */
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x00, "2" )
	PORT_DIPSETTING(    0x03, "3" )
	PORT_DIPSETTING(    0x02, "4" )
	PORT_DIPSETTING(    0x01, "5" )
	PORT_DIPNAME( 0x0c, 0x0c, DEF_STR( Difficulty ) )	//enemy shot speed
	PORT_DIPSETTING(    0x04, DEF_STR( Easy ) )
	PORT_DIPSETTING(    0x0c, DEF_STR( Medium )  )
	PORT_DIPSETTING(    0x08, DEF_STR( Hard ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Hardest )  )
	PORT_DIPNAME( 0x30, 0x30, "Level Up Speed" )	//rate of power-up
	PORT_DIPSETTING(    0x30, "Fast" )
	PORT_DIPSETTING(    0x20, "Fastest" )
	PORT_DIPSETTING(    0x10, "Slow" )
	PORT_DIPSETTING(    0x00, "Slowest" )
	PORT_DIPNAME( 0xc0, 0xc0, DEF_STR( Bonus_Life ))
	PORT_DIPSETTING(    0xc0, "200000,1000000" )
	PORT_DIPSETTING(    0x80, "220000,1200000" )
	PORT_DIPSETTING(    0x40, "240000,1400000" )
	PORT_DIPSETTING(    0x00, "every 500000,once at highest score" )	//beating the hi-score gives you an extra life

	PORT_START
	PORT_BIT( 0x0001, IP_ACTIVE_LOW,  IPT_JOYSTICK_RIGHT ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x0002, IP_ACTIVE_LOW,  IPT_JOYSTICK_LEFT ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x0004, IP_ACTIVE_LOW,  IPT_JOYSTICK_DOWN ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x0008, IP_ACTIVE_LOW,  IPT_JOYSTICK_UP ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x0010, IP_ACTIVE_LOW,  IPT_BUTTON1 ) PORT_PLAYER(1)
	PORT_BIT( 0x0020, IP_ACTIVE_LOW,  IPT_BUTTON2 ) PORT_PLAYER(1)
	PORT_BIT( 0x0040, IP_ACTIVE_LOW,  IPT_START1 )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW,  IPT_START2 )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW,  IPT_JOYSTICK_RIGHT ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x0200, IP_ACTIVE_LOW,  IPT_JOYSTICK_LEFT ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x0400, IP_ACTIVE_LOW,  IPT_JOYSTICK_DOWN ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x0800, IP_ACTIVE_LOW,  IPT_JOYSTICK_UP ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x1000, IP_ACTIVE_LOW,  IPT_BUTTON1 ) PORT_PLAYER(2)
	PORT_BIT( 0x2000, IP_ACTIVE_LOW,  IPT_BUTTON2 ) PORT_PLAYER(2)
	PORT_BIT( 0x4000, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BIT( 0x8000, IP_ACTIVE_HIGH, IPT_COIN2 )
INPUT_PORTS_END

INPUT_PORTS_START( ginkun )
	PORT_START			/* DIP SW 1 */
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_4C ) )
	PORT_DIPNAME( 0x0c, 0x0c, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x0c, DEF_STR( 1C_1C ) )
	PORT_DIPNAME( 0x10, 0x10, "Continue Plus 1up" )
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

	PORT_START			/* DIP SW 2 */
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x00, "2" )
	PORT_DIPSETTING(    0x03, "3" )
	PORT_DIPSETTING(    0x02, "4" )
	PORT_DIPSETTING(    0x01, "5" )
	PORT_DIPNAME( 0x0c, 0x0c, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Easy ) )
	PORT_DIPSETTING(    0x0c, DEF_STR( Normal ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Hard ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Hardest ) )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )		/* Doesn't work? */
	PORT_DIPSETTING(    0x20, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START
	PORT_BIT( 0x0001, IP_ACTIVE_LOW,  IPT_JOYSTICK_RIGHT ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x0002, IP_ACTIVE_LOW,  IPT_JOYSTICK_LEFT ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x0004, IP_ACTIVE_LOW,  IPT_JOYSTICK_DOWN ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x0008, IP_ACTIVE_LOW,  IPT_JOYSTICK_UP ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x0010, IP_ACTIVE_LOW,  IPT_BUTTON1 ) PORT_PLAYER(1)
	PORT_BIT( 0x0020, IP_ACTIVE_LOW,  IPT_BUTTON2 ) PORT_PLAYER(1)
	PORT_BIT( 0x0040, IP_ACTIVE_LOW,  IPT_START1 )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW,  IPT_START2 )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW,  IPT_JOYSTICK_RIGHT ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x0200, IP_ACTIVE_LOW,  IPT_JOYSTICK_LEFT ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x0400, IP_ACTIVE_LOW,  IPT_JOYSTICK_DOWN ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x0800, IP_ACTIVE_LOW,  IPT_JOYSTICK_UP ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x1000, IP_ACTIVE_LOW,  IPT_BUTTON1 ) PORT_PLAYER(2)
	PORT_BIT( 0x2000, IP_ACTIVE_LOW,  IPT_BUTTON2 ) PORT_PLAYER(2)
	PORT_BIT( 0x4000, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BIT( 0x8000, IP_ACTIVE_HIGH, IPT_COIN2 )

	PORT_START
	/* Not used */
INPUT_PORTS_END

INPUT_PORTS_START( riot )
	PORT_START			/* DIP SW 1 */
	PORT_DIPNAME( 0x07, 0x07, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 5C_1C ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x07, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x06, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x05, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 1C_4C ) )
	PORT_DIPNAME( 0x38, 0x38, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 5C_1C ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x20, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x38, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x30, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x28, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x18, DEF_STR( 1C_4C ) )
	PORT_DIPNAME( 0x40, 0x40, "Starting Coins" )
	PORT_DIPSETTING(    0x40, "1" )
	PORT_DIPSETTING(    0x00, "2" )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START			/* DIP SW 2 */
	PORT_DIPNAME( 0x03, 0x00, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x03, "1" )
	PORT_DIPSETTING(    0x02, "2" )
	PORT_DIPSETTING(    0x01, "3" )
	PORT_DIPSETTING(    0x00, "4" )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x20, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START
	PORT_BIT( 0x0001, IP_ACTIVE_LOW,  IPT_JOYSTICK_RIGHT ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x0002, IP_ACTIVE_LOW,  IPT_JOYSTICK_LEFT ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x0004, IP_ACTIVE_LOW,  IPT_JOYSTICK_DOWN ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x0008, IP_ACTIVE_LOW,  IPT_JOYSTICK_UP ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x0010, IP_ACTIVE_LOW,  IPT_BUTTON2 ) PORT_PLAYER(1)
	PORT_BIT( 0x0020, IP_ACTIVE_LOW,  IPT_BUTTON3 ) PORT_PLAYER(1)
	PORT_BIT( 0x0040, IP_ACTIVE_LOW,  IPT_START1 )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW,  IPT_START2 )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW,  IPT_JOYSTICK_RIGHT ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x0200, IP_ACTIVE_LOW,  IPT_JOYSTICK_LEFT ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x0400, IP_ACTIVE_LOW,  IPT_JOYSTICK_DOWN ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x0800, IP_ACTIVE_LOW,  IPT_JOYSTICK_UP ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x1000, IP_ACTIVE_LOW,  IPT_BUTTON2 ) PORT_PLAYER(2)
	PORT_BIT( 0x2000, IP_ACTIVE_LOW,  IPT_BUTTON3 ) PORT_PLAYER(2)
	PORT_BIT( 0x4000, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BIT( 0x8000, IP_ACTIVE_HIGH, IPT_COIN2 )

	PORT_START
	PORT_BIT( 0x0002, IP_ACTIVE_LOW,  IPT_BUTTON1 ) PORT_PLAYER(1)
	PORT_BIT( 0x0020, IP_ACTIVE_LOW,  IPT_BUTTON1 ) PORT_PLAYER(2)
	PORT_BIT( 0xffdd, IP_ACTIVE_LOW,  IPT_UNKNOWN )
INPUT_PORTS_END

/******************************************************************************/

static const gfx_layout charlayout =
{
	8,8,	/* 8*8 characters */
	4096,	/* 4096 characters */
	4,	/* 4 bits per pixel */
	{ 0, 1, 2, 3 },	/* the bitplanes are packed in one nibble */
	{ 0*4, 1*4, 2*4, 3*4, 4*4, 5*4, 6*4, 7*4 },
	{ 0*32, 1*32, 2*32, 3*32, 4*32, 5*32, 6*32, 7*32 },
	32*8	/* every char takes 32 consecutive bytes */
};

static const gfx_layout tilelayout =
{
	16,16,	/* 16*16 tiles */
	8192,	/* 8192 tiles */
	4,	/* 4 bits per pixel */
	{ 0, 1, 2, 3 },	/* the bitplanes are packed in one nibble */
	{ 0*4, 1*4, 2*4, 3*4, 4*4, 5*4, 6*4, 7*4,
			32*8+0*4, 32*8+1*4, 32*8+2*4, 32*8+3*4, 32*8+4*4, 32*8+5*4, 32*8+6*4, 32*8+7*4 },
	{ 0*32, 1*32, 2*32, 3*32, 4*32, 5*32, 6*32, 7*32,
			16*32, 17*32, 18*32, 19*32, 20*32, 21*32, 22*32, 23*32 },
	128*8	/* every sprite takes 128 consecutive bytes */
};

static const gfx_layout spritelayout =
{
	8,8,	/* 8*8 sprites */
	32768,	/* 32768 sprites */
	4,		/* 4 bits per pixel */
	{ 0, 1, 2, 3 },	/* the bitplanes are packed in one nibble */
	{ 0*4, 1*4, 2*4, 3*4, 4*4, 5*4, 6*4, 7*4 },
	{ 0*32, 1*32, 2*32, 3*32, 4*32, 5*32, 6*32, 7*32 },
	32*8	/* every sprite takes 32 consecutive bytes */
};

static const gfx_decode gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &charlayout,   1*16*16, 16   },
	{ REGION_GFX2, 0, &tilelayout,   2*16*16, 16*2 },
	{ REGION_GFX3, 0, &spritelayout, 0*16*16, 16   },
	{ -1 } /* end of array */
};

/******************************************************************************/

static void irqhandler(int irq)
{
	cpunum_set_input_line(1,0,irq ? ASSERT_LINE : CLEAR_LINE);
}

static struct YM2151interface ym2151_interface =
{
	irqhandler
};

/******************************************************************************/

static MACHINE_DRIVER_START( fstarfrc )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000,24000000/2)			/* 12MHz */
	MDRV_CPU_PROGRAM_MAP(fstarfrc_readmem,fstarfrc_writemem)
	MDRV_CPU_VBLANK_INT(irq5_line_hold,1)

	MDRV_CPU_ADD(Z80,8000000/2)
	/* audio CPU */			/* 4MHz */
	MDRV_CPU_PROGRAM_MAP(sound_readmem,sound_writemem)
								/* NMIs are triggered by the main CPU */
	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)
	MDRV_INTERLEAVE(10)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER | VIDEO_NEEDS_6BITS_PER_GUN | VIDEO_RGB_DIRECT)
	MDRV_SCREEN_SIZE(32*8, 32*8)
	MDRV_VISIBLE_AREA(0*8, 32*8-1, 2*8, 30*8-1)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(4096)

	MDRV_VIDEO_START(fstarfrc)
	MDRV_VIDEO_UPDATE(tecmo16)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_STEREO("left", "right")

	MDRV_SOUND_ADD(YM2151, 8000000/2)
	MDRV_SOUND_CONFIG(ym2151_interface)
	MDRV_SOUND_ROUTE(0, "left", 0.60)
	MDRV_SOUND_ROUTE(1, "right", 0.60)

	MDRV_SOUND_ADD(OKIM6295, 7575)
	MDRV_SOUND_CONFIG(okim6295_interface_region_1)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "left", 0.40)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "right", 0.40)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( ginkun )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000,24000000/2)			/* 12MHz */
	MDRV_CPU_PROGRAM_MAP(ginkun_readmem,ginkun_writemem)
	MDRV_CPU_VBLANK_INT(irq5_line_hold,1)

	MDRV_CPU_ADD(Z80,8000000/2)
	/* audio CPU */			/* 4MHz */
	MDRV_CPU_PROGRAM_MAP(sound_readmem,sound_writemem)
								/* NMIs are triggered by the main CPU */
	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)
	MDRV_INTERLEAVE(10)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER | VIDEO_NEEDS_6BITS_PER_GUN | VIDEO_RGB_DIRECT)
	MDRV_SCREEN_SIZE(32*8, 32*8)
	MDRV_VISIBLE_AREA(0*8, 32*8-1, 2*8, 30*8-1)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(4096)

	MDRV_VIDEO_START(ginkun)
	MDRV_VIDEO_UPDATE(tecmo16)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_STEREO("left", "right")

	MDRV_SOUND_ADD(YM2151, 8000000/2)
	MDRV_SOUND_CONFIG(ym2151_interface)
	MDRV_SOUND_ROUTE(0, "left", 0.60)
	MDRV_SOUND_ROUTE(1, "right", 0.60)

	MDRV_SOUND_ADD(OKIM6295, 7575)
	MDRV_SOUND_CONFIG(okim6295_interface_region_1)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "left", 0.40)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "right", 0.40)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( riot )
	/* basic machine hardware */
	MDRV_IMPORT_FROM(ginkun)
	MDRV_VIDEO_START(riot)
MACHINE_DRIVER_END

/******************************************************************************/

ROM_START( fstarfrc )
	ROM_REGION( 0x80000, REGION_CPU1, 0 )
	ROM_LOAD16_BYTE( "fstarf01.rom", 0x00000, 0x40000, CRC(94c71de6) SHA1(7637aee89034d60ef74d0015db6fcbcc8689b88b) )
	ROM_LOAD16_BYTE( "fstarf02.rom", 0x00001, 0x40000, CRC(b1a07761) SHA1(efd580e06a134a8b6ed6e836eec3203c41ed03c5) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )
	ROM_LOAD( "fstarf07.rom", 0x00000, 0x10000, CRC(e0ad5de1) SHA1(677237341e837061b6cc02200c0752964caed907) )

	ROM_REGION( 0x20000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "fstarf03.rom", 0x00000, 0x20000, CRC(54375335) SHA1(d1af56a7c7fff877066dad3144d0b5147da28c6a) )

	ROM_REGION( 0x100000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD16_BYTE( "fstarf05.rom", 0x00000, 0x80000, CRC(77a281e7) SHA1(a87a90c2c856d45785cb56185b1a7dff3404b5cb) )
	ROM_LOAD16_BYTE( "fstarf04.rom", 0x00001, 0x80000, CRC(398a920d) SHA1(eecc167803f48517348d68ce70f15e87eac204bb) )

	ROM_REGION( 0x100000, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_LOAD16_BYTE( "fstarf09.rom", 0x00000, 0x80000, CRC(d51341d2) SHA1(e46c319158046d407d4387cb2d8f0b6cfd7be576) )
	ROM_LOAD16_BYTE( "fstarf06.rom", 0x00001, 0x80000, CRC(07e40e87) SHA1(22867e52a8267ae8ae0ff0dba6bb846cb3e1b63d) )

	ROM_REGION( 0x20000, REGION_SOUND1, 0 )
	ROM_LOAD( "fstarf08.rom", 0x00000, 0x20000, CRC(f0ad5693) SHA1(a0202801bb9f9c86175ca7989fbc9efa47183188) )
ROM_END

ROM_START( ginkun )
	ROM_REGION( 0x80000, REGION_CPU1, 0 )
	ROM_LOAD16_BYTE( "ginkun01.i01", 0x00000, 0x40000, CRC(98946fd5) SHA1(e0b496d1fa5201d94a2a22243fe4b37d9ff7bc90) )
	ROM_LOAD16_BYTE( "ginkun02.i02", 0x00001, 0x40000, CRC(e98757f6) SHA1(2310b5f00b9522d5a983c8686f7d5bcf2d885964) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )
	ROM_LOAD( "ginkun07.i17", 0x00000, 0x10000, CRC(8836b1aa) SHA1(22bd5258e5971aa69eaa516d7358d87fbb65bee4) )

	ROM_REGION( 0x20000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "ginkun03.i03", 0x00000, 0x20000, CRC(4456e0df) SHA1(1509474cfbb208502262b7039e28d37be1131a46) )

	ROM_REGION( 0x100000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD16_BYTE( "ginkun05.i09", 0x00000, 0x80000, CRC(1263bd42) SHA1(bff93633d42bae5b8273465e16bdb4db81bbd6e0) )
	ROM_LOAD16_BYTE( "ginkun04.i05", 0x00001, 0x80000, CRC(9e4cf611) SHA1(57242f0aac49e0569a57372e59ccc643924e9b44) )

	ROM_REGION( 0x100000, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_LOAD16_BYTE( "ginkun09.i22", 0x00000, 0x80000, CRC(233384b9) SHA1(031735b0fb2c89b0af26ba76061776767647c59c) )
	ROM_LOAD16_BYTE( "ginkun06.i16", 0x00001, 0x80000, CRC(f8589184) SHA1(b933265960742cb3505eb73631ec419b7e1d1d63) )

	ROM_REGION( 0x20000, REGION_SOUND1, 0 )
	ROM_LOAD( "ginkun08.i18", 0x00000, 0x20000, CRC(8b7583c7) SHA1(be7ce721504afb45e16eda146f12031d818fc94c) )
ROM_END


/*
Riot
NMK, 1992

This game runs on Tecmo hardware.

PCB Layouts -

MAIN BOARD
----------

8901A-4 TECMO
|--------------------------------------------------------------------|
|LA4460                                    |----------------------|  |
|           |-----|          6264          |----------------------|  |
|           |     |                   |--------|    |--------|       |
|           |     |          6264     |        |    |        | 62256 |
|   LM324   |68000|                   |TECMO-8 |    |TECMO-07|       |
|           |     |                   |        |    |        |       |
|   LM324   |     |                   |        |    |        |       |
|           |     |                   |--------|    |--------|       |
|           |     |                                                  |
|           |     |       |-----|                                |-| |
|   YM3012  |-----| PAL1  |TECMO|                   6116   6116  | | |
|J                        |-11  |                                | | |
|A                        |-----|                                | | |
|M                                  4464                         | | |
|M                 4464   4464      4464    4464    4464   4464  | | |
|A                 4464   4464      4464            4464   4464  | | |
|                                                                |-| |
|                           |--------|                   |--------|  |
|                           |        |     |-----|       |        |  |
|                           |TECMO-10|     |TECMO|       |TECMO-06|  |
|      24MHz        YM2151  |        |     |-12  |       |        |  |
|    |--------|             |        |     |-----|       |        |  |
|    |        |             |--------|                   |--------|  |
|    |TECMO-9 |       PAL2                                           |
|    |        |                                                      |
|    |        |       6264                        |-------------|    |
|    |--------|               M6295          8MHz |   TECMO-5   |    |
|                                                 |-------------|    |
|                                          |----------------------|  |
|  DSW2(8)  DSW1(8)   Z80                  |----------------------|  |
|--------------------------------------------------------------------|
Notes:
      68000 clock 12.000MHz [24/2]
      Z80 clock 4.000MHz [24/6]
      YM2151 clock 4.000MHz [24/6]
      M6295 clock 1.000MHz [8/8], sample rate 1000000/132
      VSync 60Hz
      PAL1 - AMI 18CV8 stamped 'T-11'
      PAL2 - AMI 18CV8 stamped 'T-12'

      Custom Tecmo IC's -
                         TECMO-5   MCU? clock input 6.000MHz on pin15 (SDIP64)
                         TECMO-06, also stamped 'YM6048' (QFP160)
                         TECMO-07, also stamped 'YM6621' (QFP160)
                         TECMO-8   (QFP136)
                         TECMO-9,  also stamped 'MN53030XTB' (QFP124)
                         TECMO-10  (QFP128)
                         TECMO-11, also stamped 'MN51005XTC' (QFP64)
                         TECMO-12, also stamped 'MN51005XTD' (QFP64)


ROM BOARD
---------

TECMO OBD ROM 8901B
B-82778 (sticker)
|---------------------------------|
|       |----------------------|  |
|       |----------------------|  |
|                                 |
|                                 |
|                                 |
|  7   8    *    *    *    9      |
|                                 |
|                                 |
|                                 |
|                                 |
|                                 |
|                                 |
|  *   *    *    *    *    6      |
|                             |-| |
|                             | | |
|                             | | |
|                             | | |
|                             | | |
|                             | | |
|  *   4    *    *    *    5  | | |
|                             |-| |
|                                 |
|                                 |
|                                 |
|                                 |
|           1    2         3      |
|                                 |
|                                 |
|       |----------------------|  |
|       |----------------------|  |
|---------------------------------|
Notes:
      * = empty solder location for ROM (no socket)

*/

ROM_START( riot )
	ROM_REGION( 0x80000, REGION_CPU1, 0 )
	ROM_LOAD16_BYTE( "1.ic1", 0x00000, 0x40000, CRC(9ef4232e) SHA1(b9dd3e0dc5785311ff2433b5eb94e327b51ef144) )
	ROM_LOAD16_BYTE( "2.ic2", 0x00001, 0x40000, CRC(f2c6fbbf) SHA1(114cc9ede8b6b4e94dad59f82f0232e9b7fa5025) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )
	ROM_LOAD( "7.ic17", 0x00000, 0x10000, CRC(0a95b8f3) SHA1(cc6bdeeeb184eb4f3867eb9c961b0b82743fac9f) )

	ROM_REGION( 0x20000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "3.ic3", 0x00000, 0x20000, CRC(f60f5c96) SHA1(56ea21f22d3cf47071bfb3555b331a676463b63e) )

	ROM_REGION( 0x100000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD16_BYTE( "5.ic9", 0x00000, 0x80000, CRC(056fce78) SHA1(25234fa0282fdbefefb06e6aa5a467f9d08ed534) )
	ROM_LOAD16_BYTE( "4.ic5", 0x00001, 0x80000, CRC(0894e7b4) SHA1(37a04476770942f292d836997c649a343f71e317) )

	ROM_REGION( 0x100000, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_LOAD16_BYTE( "9.ic22", 0x00000, 0x80000, CRC(0ead54f3) SHA1(4848eb158d9e2279332225e0b25f1c96a8a5a0c4) )
	ROM_LOAD16_BYTE( "6.ic16", 0x00001, 0x80000, CRC(96ef61da) SHA1(c306e4d1eee19af0229a47c2f115f98c74f33d33) )

	ROM_REGION( 0x20000, REGION_SOUND1, 0 )
	ROM_LOAD( "8.ic18", 0x00000, 0x20000, CRC(4b70e266) SHA1(4ed23de9223cc7359fbaff9dd500ef6daee00fb0) )
ROM_END

/******************************************************************************/

GAME( 1992, fstarfrc, 0, fstarfrc, fstarfrc, 0, ROT90, "Tecmo", "Final Star Force (US)", 0 )
GAME( 1992, riot,     0, riot,     riot,     0, ROT0,  "NMK",   "Riot", 0 )
GAME( 1995, ginkun,   0, ginkun,   ginkun,   0, ROT0,  "Tecmo", "Ganbare Ginkun", 0 )
