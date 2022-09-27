/***************************************************************************

                              -= Paradise / Target Ball / Torus =-

                    driver by   Luca Elia (l.elia@tin.it)


CPU          :  Z8400B
Video Chips  :  TPC1024AFN-084C
Sound Chips  :  2 x AR17961 (OKI M6295) (only 1 in Torus)

---------------------------------------------------------------------------
Year + Game          Board#
---------------------------------------------------------------------------
94+ Paradise         YS-1600
94+ Paradise Deluxe  YS-1604
95  Target Ball      YS-2002
96  Torus            ?
98  Mad Ball         YS-0402
---------------------------------------------------------------------------

Notes:

paradise: I'm not sure it's working correctly:

- The high scores table can't be entered !?
- The chance to play a bonus game is very slim. I think I got to play
  a couple in total. Is there a way to trigger them !?

Target Ball
Yunsung, 1995

PCB Layout
----------

YS-2002 YUNSUNG
|---------------------------------------------------------|
|  M6295  M6295   Z80              4MHz       YUNSUNG.110 |
| YUNSUNG.113   YUNSUNG.128                   YUNSUNG.111 |
| YUNSUNG.85     6264                                     |
|                                             YUNSUNG.92  |
|                                             YUNSUNG.93  |
|                                                         |
|                6116                                     |
|                6116              6116                   |
|J               6116                                     |
|A                                                        |
|M                                            6116        |
|M                                            6116        |
|A                            |-------|                   |
|                             | ACTEL |                   |
|                             |A1020B |                   |
|DSW1(8)                      |PLCC84 |                   |
|         12MHz               |-------|                   |
|                           4464                          |
|                           4464     YUNSUNG.114          |
|                           4464     YUNSUNG.115          |
|DSW2(8)                    4464                          |
|                                                         |
|---------------------------------------------------------|
Notes:
      Z80 clock: 6.000MHz
     6295 clock: 1.000MHz (both), sample rate = 1000000/132 (both)
          VSync: 54Hz

 note even with these settings game runs slightly faster in Mame than real PCB

***************************************************************************/

#include "driver.h"
#include "paradise.h"
#include "sound/okim6295.h"

/***************************************************************************

                                Memory Maps

***************************************************************************/

static WRITE8_HANDLER( paradise_rombank_w )
{
	int bank = data;
	int bank_n = memory_region_length(REGION_CPU1)/0x4000 - 1;
	if (bank >= bank_n)
	{
		logerror("PC %04X - invalid rom bank %x\n",activecpu_get_pc(),bank);
		bank %= bank_n;
	}

	if (bank >= 3)	bank+=1;
	memory_set_bankptr(1, memory_region(REGION_CPU1) + bank * 0x4000);
}

static WRITE8_HANDLER( paradise_okibank_w )
{
	if (data & ~0x02)	logerror("CPU #0 - PC %04X: unknown oki bank bits %02X\n",activecpu_get_pc(),data);

	if (sndti_to_sndnum(SOUND_OKIM6295, 1) >= 0)
		OKIM6295_set_bank_base(1, (data & 0x02) ? 0x40000 : 0);
}

static WRITE8_HANDLER( torus_coin_counter_w )
{
	coin_counter_w(0, data ^ 0xff);
}

#define STANDARD_MAP	\
	AM_RANGE(0x0000, 0x7fff) AM_ROM	/* ROM */	\
	AM_RANGE(0x8000, 0xbfff) AM_ROMBANK(1)	/* ROM (banked) */ \
	AM_RANGE(0xc000, 0xc7ff) AM_RAM AM_WRITE(paradise_vram_2_w) AM_BASE(&paradise_vram_2	)	/* Background */ \
	AM_RANGE(0xc800, 0xcfff) AM_RAM AM_WRITE(paradise_vram_1_w) AM_BASE(&paradise_vram_1	)	/* Midground */ \
	AM_RANGE(0xd000, 0xd7ff) AM_RAM AM_WRITE(paradise_vram_0_w) AM_BASE(&paradise_vram_0	)	/* Foreground */ \


static ADDRESS_MAP_START( paradise_map, ADDRESS_SPACE_PROGRAM, 8 )
	STANDARD_MAP
	AM_RANGE(0xd800, 0xd8ff) AM_RAM	// RAM
	AM_RANGE(0xd900, 0xe0ff) AM_RAM AM_BASE(&spriteram) AM_SIZE(&spriteram_size	)	// Sprites
	AM_RANGE(0xe100, 0xffff) AM_RAM	// RAM
ADDRESS_MAP_END

static ADDRESS_MAP_START( tgtball_map, ADDRESS_SPACE_PROGRAM, 8 )
	STANDARD_MAP
	AM_RANGE(0xd800, 0xd8ff) AM_RAM	// RAM
	AM_RANGE(0xd900, 0xd9ff) AM_RAM AM_BASE(&spriteram) AM_SIZE(&spriteram_size	)	// Sprites
	AM_RANGE(0xda00, 0xffff) AM_RAM	// RAM
ADDRESS_MAP_END

static ADDRESS_MAP_START( torus_map, ADDRESS_SPACE_PROGRAM, 8 )
	STANDARD_MAP
	AM_RANGE(0xd800, 0xdfff) AM_RAM AM_BASE(&spriteram) AM_SIZE(&spriteram_size	)	// Sprites
	AM_RANGE(0xe000, 0xffff) AM_RAM	// RAM
ADDRESS_MAP_END


static ADDRESS_MAP_START( paradise_readport, ADDRESS_SPACE_IO, 8 )
	AM_RANGE(0x0000, 0x17ff) AM_READ(paletteram_r			)	// Palette
	AM_RANGE(0x2010, 0x2010) AM_READ(OKIM6295_status_0_r	)	// OKI 0
	AM_RANGE(0x2030, 0x2030) AM_READ(OKIM6295_status_1_r	)	// OKI 1
	AM_RANGE(0x2020, 0x2020) AM_READ(input_port_0_r		)	// DSW 1
	AM_RANGE(0x2021, 0x2021) AM_READ(input_port_1_r		)	// DSW 2
	AM_RANGE(0x2022, 0x2022) AM_READ(input_port_2_r		)	// P1
	AM_RANGE(0x2023, 0x2023) AM_READ(input_port_3_r		)	// P2
	AM_RANGE(0x2024, 0x2024) AM_READ(input_port_4_r		)	// Coins
	AM_RANGE(0x8000, 0xffff) AM_READ(videoram_r			)	// Pixmap
ADDRESS_MAP_END

static ADDRESS_MAP_START( paradise_writeport, ADDRESS_SPACE_IO, 8 )
	AM_RANGE(0x0000, 0x17ff) AM_WRITE(paradise_palette_w	)	// Palette
	AM_RANGE(0x1800, 0x1800) AM_WRITE(paradise_priority_w	)	// Layers priority
	AM_RANGE(0x2001, 0x2001) AM_WRITE(paradise_flipscreen_w	)	// Flip Screen
	AM_RANGE(0x2004, 0x2004) AM_WRITE(paradise_palbank_w	)	// Layers palette bank
	AM_RANGE(0x2006, 0x2006) AM_WRITE(paradise_rombank_w	)	// ROM bank
	AM_RANGE(0x2007, 0x2007) AM_WRITE(paradise_okibank_w	)	// OKI 1 samples bank
	AM_RANGE(0x2010, 0x2010) AM_WRITE(OKIM6295_data_0_w		)	// OKI 0
	AM_RANGE(0x2030, 0x2030) AM_WRITE(OKIM6295_data_1_w		)	// OKI 1
	AM_RANGE(0x8000, 0xffff) AM_WRITE(paradise_pixmap_w		)	// Pixmap
ADDRESS_MAP_END


/***************************************************************************

                                Input Ports

***************************************************************************/

/***************************************************************************
                                Paradise
***************************************************************************/

INPUT_PORTS_START( paradise )
	PORT_START	// IN0 - port $2020 - DSW 1
	PORT_DIPNAME( 0x03, 0x02, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x03, DEF_STR( Easy ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Normal ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Hard ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Hardest ) )
	PORT_DIPNAME( 0x0c, 0x08, "Fill Area" )
	PORT_DIPSETTING(    0x0c, "75%" )
	PORT_DIPSETTING(    0x08, "80%" )
	PORT_DIPSETTING(    0x04, "85%" )
	PORT_DIPSETTING(    0x00, "90%" )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x00, "3" )
	PORT_DIPSETTING(    0x10, "5" )
	PORT_DIPNAME( 0x60, 0x20, "Time" )
	PORT_DIPSETTING(    0x00, "45" )
	PORT_DIPSETTING(    0x20, "60" )
	PORT_DIPSETTING(    0x40, "75" )
	PORT_DIPSETTING(    0x60, "90" )
	PORT_DIPNAME( 0x80, 0x80, "Sound Test" )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START	// IN1 - port $2021 - DSW 2
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 1C_2C ) )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x20, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, "Characters Test" )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START	// IN2 - port $2022 - Player 1
	PORT_BIT(  0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_PLAYER(1)
	PORT_BIT(  0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_PLAYER(1)
	PORT_BIT(  0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_PLAYER(1)
	PORT_BIT(  0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_PLAYER(1)
	PORT_BIT(  0x10, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(1)
	PORT_BIT(  0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )	// alias for button1?
	PORT_BIT(  0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )	// alias for button1?
	PORT_BIT(  0x80, IP_ACTIVE_LOW, IPT_START1  )

	PORT_START	// IN3 - port $2023 - Player 2
	PORT_BIT(  0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_PLAYER(2)
	PORT_BIT(  0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_PLAYER(2)
	PORT_BIT(  0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_PLAYER(2)
	PORT_BIT(  0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_PLAYER(2)
	PORT_BIT(  0x10, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(2)
	PORT_BIT(  0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )	// alias for button1?
	PORT_BIT(  0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )	// alias for button1?
	PORT_BIT(  0x80, IP_ACTIVE_LOW, IPT_START2  )

	PORT_START	// IN4 - port $2024 - Coins
	PORT_BIT(  0x01, IP_ACTIVE_LOW, IPT_COIN1 ) PORT_IMPULSE(5)
	PORT_BIT(  0x02, IP_ACTIVE_LOW, IPT_COIN2 ) PORT_IMPULSE(5)
	PORT_BIT(  0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x40, IP_ACTIVE_LOW, IPT_VBLANK  )
	PORT_BIT(  0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )
INPUT_PORTS_END


INPUT_PORTS_START( tgtball )
	PORT_START	// IN0 - port $2020 - DSW 1
	PORT_DIPNAME( 0x03, 0x02, "Time" )
	PORT_DIPSETTING(    0x03, "60" )
	PORT_DIPSETTING(    0x02, "80" )
	PORT_DIPSETTING(    0x01, "100" )
	PORT_DIPSETTING(    0x00, "120" )
	PORT_DIPNAME( 0x0c, 0x08, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x0c, "15" )
	PORT_DIPSETTING(    0x08, "20" )
	PORT_DIPSETTING(    0x04, "25" )
	PORT_DIPSETTING(    0x00, "30" )
	PORT_DIPNAME( 0x30, 0x20, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x30, "4" )
	PORT_DIPSETTING(    0x20, "5" )
	PORT_DIPSETTING(    0x10, "6" )
	PORT_DIPSETTING(    0x00, "7" )
	PORT_DIPNAME( 0x40, 0x40, "Balls Sequence Length" )
	PORT_DIPSETTING(    0x40, "3" )
	PORT_DIPSETTING(    0x00, "5" )
	PORT_DIPNAME( 0x80, 0x80, "Game Goal" )
	PORT_DIPSETTING(    0x80, "Target Score" )
	PORT_DIPSETTING(    0x00, "Balls Sequence" )

	PORT_START	// IN1 - port $2021 - DSW 2
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 1C_2C ) )
	PORT_DIPNAME( 0x0c, 0x08, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x0c, "120" )
	PORT_DIPSETTING(    0x08, "160" )
	PORT_DIPSETTING(    0x04, "200" )
	PORT_DIPSETTING(    0x00, "240" )
	PORT_DIPNAME( 0x10, 0x10, "Vs. Matches" )
	PORT_DIPSETTING(    0x10, "1" )
	PORT_DIPSETTING(    0x00, "3" )
	PORT_DIPNAME( 0x20, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, "Characters Test" )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START	// IN2 - port $2022 - Player 1
	PORT_BIT(  0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_PLAYER(1)
	PORT_BIT(  0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_PLAYER(1)
	PORT_BIT(  0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_PLAYER(1)
	PORT_BIT(  0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_PLAYER(1)
	PORT_BIT(  0x10, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(1)
	PORT_BIT(  0x20, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(1)
	PORT_BIT(  0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x80, IP_ACTIVE_LOW, IPT_START1  )

	PORT_START	// IN3 - port $2023 - Player 2
	PORT_BIT(  0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_PLAYER(2)
	PORT_BIT(  0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_PLAYER(2)
	PORT_BIT(  0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_PLAYER(2)
	PORT_BIT(  0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_PLAYER(2)
	PORT_BIT(  0x10, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(2)
	PORT_BIT(  0x20, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(2)
	PORT_BIT(  0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x80, IP_ACTIVE_LOW, IPT_START2  )

	PORT_START	// IN4 - port $2024 - Coins
	PORT_BIT(  0x01, IP_ACTIVE_LOW, IPT_COIN1 ) PORT_IMPULSE(5)
	PORT_BIT(  0x02, IP_ACTIVE_LOW, IPT_COIN2 ) PORT_IMPULSE(5)
	PORT_BIT(  0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x08, IP_ACTIVE_LOW, IPT_VBLANK  )
	PORT_BIT(  0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )
INPUT_PORTS_END

INPUT_PORTS_START( torus )

	PORT_START	// IN0 - port $2020 - DSW 1
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 1C_2C ) )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START	// IN1 - port $2021 - DSW 2
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, "Characters Test" )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START	// IN2 - port $2022 - Player 1
	PORT_BIT(  0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_PLAYER(1)
	PORT_BIT(  0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_PLAYER(1)
	PORT_BIT(  0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_PLAYER(1)
	PORT_BIT(  0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_PLAYER(1)
	PORT_BIT(  0x10, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(1)
	PORT_BIT(  0x20, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(1)
	PORT_BIT(  0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x80, IP_ACTIVE_LOW, IPT_START1  )

	PORT_START	// IN3 - port $2023 - Player 2
	PORT_BIT(  0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_PLAYER(2)
	PORT_BIT(  0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_PLAYER(2)
	PORT_BIT(  0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_PLAYER(2)
	PORT_BIT(  0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_PLAYER(2)
	PORT_BIT(  0x10, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(2)
	PORT_BIT(  0x20, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(2)
	PORT_BIT(  0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x80, IP_ACTIVE_LOW, IPT_START2  )

	PORT_START	// IN4 - port $2024 - Coins
	PORT_BIT(  0x01, IP_ACTIVE_LOW, IPT_COIN1 ) PORT_IMPULSE(5)
	PORT_BIT(  0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x40, IP_ACTIVE_LOW, IPT_VBLANK  )
	PORT_BIT(  0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )
INPUT_PORTS_END


INPUT_PORTS_START( madball )
	PORT_START	/* 8bit DSW 1 */
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 1C_3C ) )
	PORT_DIPNAME( 0x0c, 0x08, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x0c, "3" )
	PORT_DIPSETTING(    0x08, "4" )
	PORT_DIPSETTING(    0x04, "5" )
	PORT_DIPSETTING(    0x00, "6" )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x00, "Control?" )
	PORT_DIPSETTING(    0x20, "Disable / Spinner?" )
	PORT_DIPSETTING(    0x00, DEF_STR( Joystick ) )
	PORT_DIPNAME( 0x40, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, "Slide Show" ) /* Use P1 button to advance */
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START	/* 8bit DSW 2 */
	PORT_DIPNAME( 0x03, 0x02, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Easy ) )
	PORT_DIPSETTING(    0x03, DEF_STR( Normal ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Hard ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Hardest ) )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START	// IN2 - port $2022 - Player 1
	PORT_BIT(  0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_PLAYER(1)
	PORT_BIT(  0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_PLAYER(1)
	PORT_BIT(  0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_PLAYER(1)
	PORT_BIT(  0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_PLAYER(1)
	PORT_BIT(  0x10, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(1)
	PORT_BIT(  0x20, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(1)
	PORT_BIT(  0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x80, IP_ACTIVE_LOW, IPT_START1  )

	PORT_START	// IN3 - port $2023 - Player 2
	PORT_BIT(  0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_PLAYER(2)
	PORT_BIT(  0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_PLAYER(2)
	PORT_BIT(  0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_PLAYER(2)
	PORT_BIT(  0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_PLAYER(2)
	PORT_BIT(  0x10, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(2)
	PORT_BIT(  0x20, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(2)
	PORT_BIT(  0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x80, IP_ACTIVE_LOW, IPT_START2  )

	PORT_START	// IN4 - port $2024 - Coins
	PORT_BIT(  0x01, IP_ACTIVE_LOW, IPT_COIN1 ) PORT_IMPULSE(5)
	PORT_BIT(  0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x40, IP_ACTIVE_LOW, IPT_VBLANK  )
	PORT_BIT(  0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )
INPUT_PORTS_END

/***************************************************************************

                                Graphics Layouts

***************************************************************************/

static const gfx_layout layout_8x8x4 =
{
	8,8,
	RGN_FRAC(1,1),
	4,
	{ STEP4(0,1) },
	{ STEP8(0,4) },
	{ STEP8(0,4*8) },
	8*8*4
};

static const gfx_layout layout_8x8x8 =
{
	8,8,
	RGN_FRAC(1,2),
	8,
	{ STEP4(RGN_FRAC(1,2),1), STEP4(RGN_FRAC(0,2),1) },
	{ STEP8(0,4) },
	{ STEP8(0,4*8) },
	8*8*4
};

static const gfx_layout layout_16x16x8 =
{
	16,16,
	RGN_FRAC(1,2),
	8,
	{ STEP4(RGN_FRAC(1,2),1), STEP4(RGN_FRAC(0,2),1) },
	{ STEP8(8*8*4*0,4), STEP8(8*8*4*1,4) },
	{ STEP8(8*8*4*0,4*8), STEP8(8*8*4*2,4*8) },
	16*16*4
};

static const gfx_layout torus_layout_16x16x8 =
{
	16,16,
	RGN_FRAC(1,2),
	8,
	{ STEP4(RGN_FRAC(1,2),1), STEP4(RGN_FRAC(0,2),1) },
	{ STEP8(0,4),STEP8(4*8,4) },
	{ STEP16(0,8*8) },
	128*8
};

static const gfx_decode paradise_gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &layout_16x16x8,	0x100, 1  }, // [0] Sprites
	{ REGION_GFX2, 0, &layout_8x8x4,	0x400, 16 }, // [1] Background
	{ REGION_GFX3, 0, &layout_8x8x8,	0x300, 1  }, // [2] Midground
	{ REGION_GFX4, 0, &layout_8x8x8,	0x000, 1  }, // [3] Foreground
	{ -1 }
};

static const gfx_decode torus_gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &torus_layout_16x16x8, 0x100, 1  }, // [0] Sprites
	{ REGION_GFX2, 0, &layout_8x8x4,	     0x400, 16 }, // [1] Background
	{ REGION_GFX3, 0, &layout_8x8x8,	     0x300, 1  }, // [2] Midground
	{ REGION_GFX4, 0, &layout_8x8x8,	     0x000, 1  }, // [3] Foreground
	{ -1 }
};

static const gfx_decode madball_gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &torus_layout_16x16x8, 0x500, 1  }, // [0] Sprites
	{ REGION_GFX2, 0, &layout_8x8x4,	     0x400, 16 }, // [1] Background
	{ REGION_GFX3, 0, &layout_8x8x8,	     0x300, 1  }, // [2] Midground
	{ REGION_GFX4, 0, &layout_8x8x8,	     0x000, 1  }, // [3] Foreground
	{ -1 }
};

/***************************************************************************

                                Machine Drivers

***************************************************************************/

static MACHINE_DRIVER_START( paradise )

	/* basic machine hardware */
	MDRV_CPU_ADD_TAG("main", Z80, 12000000/2)			/* Z8400B - 6mhz Verified */
	MDRV_CPU_PROGRAM_MAP(paradise_map,0)
	MDRV_CPU_IO_MAP(paradise_readport,paradise_writeport)
	MDRV_CPU_VBLANK_INT(irq0_line_hold,4)	/* No nmi routine */

	MDRV_FRAMES_PER_SECOND(54) /* 54 verified */
	MDRV_VBLANK_DURATION(DEFAULT_REAL_60HZ_VBLANK_DURATION)	/* we're using IPT_VBLANK */

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER | VIDEO_NEEDS_6BITS_PER_GUN)
	MDRV_SCREEN_SIZE(256, 256)
	MDRV_VISIBLE_AREA(0, 256-1, 0+16, 256-1-16)
	MDRV_GFXDECODE(paradise_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(0x800 + 16)

	MDRV_VIDEO_START(paradise)
	MDRV_VIDEO_UPDATE(paradise)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")

	MDRV_SOUND_ADD(OKIM6295, 1000000/132)
	MDRV_SOUND_CONFIG(okim6295_interface_region_1)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.50)

	MDRV_SOUND_ADD_TAG("oki2", OKIM6295, 1000000/132)
	MDRV_SOUND_CONFIG(okim6295_interface_region_2)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.50)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( tgtball )
	/* basic machine hardware */
	MDRV_IMPORT_FROM(paradise)
	MDRV_CPU_MODIFY("main")
	MDRV_CPU_PROGRAM_MAP(tgtball_map,0)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( torus )
	/* basic machine hardware */
	MDRV_IMPORT_FROM(paradise)
	MDRV_CPU_MODIFY("main")
	MDRV_CPU_PROGRAM_MAP(torus_map,0)

	MDRV_GFXDECODE(torus_gfxdecodeinfo)

	MDRV_VIDEO_UPDATE(torus)

	MDRV_SOUND_REMOVE("oki2")
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( madball )
	/* basic machine hardware */
	MDRV_IMPORT_FROM(paradise)
	MDRV_CPU_MODIFY("main")
	MDRV_CPU_PROGRAM_MAP(torus_map,0)

	MDRV_GFXDECODE(madball_gfxdecodeinfo)

	MDRV_VIDEO_UPDATE(madball)

	MDRV_SOUND_REMOVE("oki2")
MACHINE_DRIVER_END


/***************************************************************************

                                ROMs Loading

***************************************************************************/

/***************************************************************************

                          Paradise / Paradise Deluxe

(c) Yun Sung  year unkown

Another porn qix like game

YS-1600 / YS-1604
  CPU: Z8400B PS (Z80 6Mhz)
Sound: 2 x AR17961 or AD-65 (OKI M6295)
Video: TPC1020AFN-084C
  OSC: 12.000MHz & 4.000MHz

YS-1604
|---------------------------------------------------------|
|  AD-65  AD-65   Z80              4MHz       YUNSUNG.110 |
| YUNSUNG.113   YUNSUNG.128                   YUNSUNG.111 |
| YUNSUNG.85     6264                                     |
|                                             YUNSUNG.92  |
|                                             YUNSUNG.93  |
|                                             YUNSUNG.94  |
|                6116                                     |
|                6116              6116                   |
|J               6116                                     |
|A                                                        |
|M                                            6116        |
|M                                            6116        |
|A                            |-------|       6116        |
|                             | TI    |                   |
|                             |TPC1020|                   |
|DSW1(8)                      | 084C  |                   |
|         12MHz               |-------|                   |
|                           4464                          |
|                           4464     YUNSUNG.114          |
|                           4464     YUNSUNG.115          |
|DSW2(8)                    4464                          |
|                                                         |
|---------------------------------------------------------|

The year is not shown but must be >= 1994, since the development system
(cross compiler?) they used left a "1994.8-1989" in the rom

***************************************************************************/

ROM_START( paradise )
	ROM_REGION( 0x44000, REGION_CPU1, 0 )		/* Z80 Code */
	ROM_LOAD( "u128", 0x00000, 0x0c000, CRC(8e5b5a24) SHA1(a4e559d9329f8a7a9d12cd90d98d0525958085d8) )
	ROM_CONTINUE(     0x10000, 0x34000    )

	ROM_REGION( 0x80000, REGION_GFX1, ROMREGION_DISPOSE | ROMREGION_INVERT)	/* 16x16x8 Sprites */
	ROM_LOAD( "u114", 0x00000, 0x40000, CRC(c748ba3b) SHA1(ad23bda4e001ca539f849c1ca256de5daf7c233b) )
	ROM_LOAD( "u115", 0x40000, 0x40000, CRC(0d517bbb) SHA1(5bf7c5036f3d660901e26f14baaea1a3c0327dfe) )

	ROM_REGION( 0x20000, REGION_GFX2, ROMREGION_DISPOSE | ROMREGION_INVERT)	/* 8x8x4 Background */
	ROM_LOAD( "u94", 0x00000, 0x20000, CRC(e3a99209) SHA1(5db79dc1a38d93b458b043499a58516285c65aa8) )

	ROM_REGION( 0x100000, REGION_GFX3, ROMREGION_DISPOSE | ROMREGION_INVERT)	/* 8x8x8 Foreground */
	ROM_LOAD( "u92", 0x00000, 0x80000, CRC(633d24f0) SHA1(26b25ec1014fba1a3d0d2bdba0c867c57034647d) )
	ROM_LOAD( "u93", 0x80000, 0x80000, CRC(bbf5c632) SHA1(9d31e136f014c2dd7dd988c3aee0adfcfea91bc9) )

	ROM_REGION( 0x40000, REGION_GFX4, ROMREGION_DISPOSE | ROMREGION_INVERT)	/* 8x8x8 Midground */
	ROM_LOAD( "u110", 0x00000, 0x20000, CRC(9807a7e6) SHA1(30e2a741a93954cfe672c61c93a990d0c3b25145) )
	ROM_LOAD( "u111", 0x20000, 0x20000, CRC(bc9f93f0) SHA1(dd4cfc849a0c0f918ac0dfeb7f00a67aae5a1c13) )

	ROM_REGION( 0x40000, REGION_SOUND1, 0 )	/* Samples */
	ROM_LOAD( "u85", 0x00000, 0x40000, CRC(bf3c3065) SHA1(54dd7ffea2fb3f31ed575e982b82691cddc2581a) )

	ROM_REGION( 0x80000, REGION_SOUND2, 0 )	/* Samples (banked) */
	ROM_LOAD( "u113", 0x00000, 0x80000, CRC(53de6025) SHA1(c94b3778b57ff7f46ce4cff661841019fb187d5d) )
ROM_END

ROM_START( paradlx )
	ROM_REGION( 0x44000, REGION_CPU1, 0 )		/* Z80 Code */
	ROM_LOAD( "8.u128", 0x00000, 0x0c000, CRC(3a45ac9e) SHA1(24e1b508ef582c8429e09929fea387f3a137f0e3) )
	ROM_CONTINUE(     0x10000, 0x34000    )

	ROM_REGION( 0x80000, REGION_GFX1, ROMREGION_DISPOSE | ROMREGION_INVERT)	/* 16x16x8 Sprites */
	ROM_LOAD( "6.u114", 0x00000, 0x40000, CRC(d0341838) SHA1(fa400486968bd6b5a805fb79a970bb280ee24662) )
	ROM_LOAD( "7.u115", 0x40000, 0x40000, CRC(a6231efd) SHA1(2f484ce2081c692b48dbfd98e152b7a74de9c414) )

	ROM_REGION( 0x40000, REGION_GFX2, ROMREGION_DISPOSE | ROMREGION_INVERT)	/* 8x8x4 Background */
	ROM_LOAD( "5.u94", 0x00000, 0x40000, CRC(70560945) SHA1(f5f1f1779178cb3d1bb4789a135cd49a0d0fd99b) )

	ROM_REGION( 0x100000, REGION_GFX3, ROMREGION_DISPOSE | ROMREGION_INVERT)	/* 8x8x8 Foreground */
	ROM_LOAD( "3.u92", 0x00000, 0x80000, CRC(c61aa37b) SHA1(8f4235a6ff47209b5982aa1c143f3c877bfd1bae) )
	ROM_LOAD( "4.u93", 0x80000, 0x80000, CRC(658f855d) SHA1(73a9377633b53869c47c443898914b70238b591a) )

	ROM_REGION( 0x40000, REGION_GFX4, ROMREGION_DISPOSE | ROMREGION_INVERT)	/* 8x8x8 Midground */
	ROM_LOAD( "1.u110", 0x00000, 0x20000, CRC(6b7f9bb9) SHA1(fd150c8e5a560bff49c993b0b703d84f775ea0b0) )
	ROM_LOAD( "2.u111", 0x20000, 0x20000, CRC(eb291f96) SHA1(096f09894f4a319c30daa7a3051798902d4fd1eb) )

	ROM_REGION( 0x40000, REGION_SOUND1, 0 )	/* Samples */
	ROM_LOAD( "10.u85", 0x00000, 0x40000, CRC(ed20133e) SHA1(a6ab1ab3ca075a3b03fe96cd32a5c186ee26d095) )

	ROM_REGION( 0x80000, REGION_SOUND2, 0 )	/* Samples (banked) */
	ROM_LOAD( "9.u113", 0x00000, 0x80000, CRC(9c5337f0) SHA1(4d7a8069be4551aad9d7d32d835dcf91be079359) )
ROM_END

ROM_START( tgtball )
	ROM_REGION( 0x44000, REGION_CPU1, 0 )		/* Z80 Code */
	ROM_LOAD( "rom7.bin", 0x00000, 0x0c000, CRC(8dbeab12) SHA1(7181c23459990aecbe2d13377aaf19f65108eac6) )
	ROM_CONTINUE(         0x10000, 0x34000    )

	ROM_REGION( 0x80000, REGION_GFX1, ROMREGION_DISPOSE | ROMREGION_INVERT)	/* 16x16x8 Sprites */
	ROM_LOAD( "yunsung.114", 0x00000, 0x40000, CRC(3dbe1872) SHA1(754f90123a3944ca548fc66ee65a93615155bf30) )
	ROM_LOAD( "yunsung.115", 0x40000, 0x40000, CRC(30f49dac) SHA1(b70d37973bd03069c48641d6c0804be6f9aa6553) )

	ROM_REGION( 0x20000, REGION_GFX2, ROMREGION_DISPOSE | ROMREGION_ERASEFF)	/* 8x8x4 Background */
	/* not for this game? */

	ROM_REGION( 0x100000, REGION_GFX3, ROMREGION_DISPOSE | ROMREGION_INVERT)	/* 8x8x8 Foreground */
	ROM_LOAD( "rom2.bin", 0x00000, 0x80000, CRC(fe4004ec) SHA1(fde782665445ad465b8f8fb95df5f60cd24016ad) )
	ROM_LOAD( "rom1.bin", 0x80000, 0x80000, CRC(aef17762) SHA1(3dd8924695b67eec0f25549dbe2461b927268b8f) )

	ROM_REGION( 0x100000, REGION_GFX4, ROMREGION_DISPOSE | ROMREGION_INVERT)	/* 8x8x8 Midground */
	ROM_LOAD( "rom4.bin", 0x00000, 0x80000,  CRC(0a5abf62) SHA1(6900d598764300c81c90f5a7efb294639178bee6) )
	ROM_LOAD( "rom3.bin", 0x80000, 0x80000,  CRC(94822bbf) SHA1(9fa6595eb819f163b58181926c276346cfa5c332) )

	ROM_REGION( 0x40000, REGION_SOUND1, 0 )	/* Samples */
	ROM_LOAD( "yunsung.85", 0x00000, 0x20000, CRC(cdf3336b) SHA1(98029d6d5d8ffb3b24ae2bcf950618a7d5b404c3) )

	ROM_REGION( 0x80000, REGION_SOUND2, 0 )	/* Samples (banked) */
	ROM_LOAD( "yunsung.113", 0x00000, 0x40000, CRC(150a6cc6) SHA1(b435fcf8ba48006f506db6b63ba54a30a6b3eade) )
ROM_END

ROM_START( tgtballa )
	ROM_REGION( 0x44000, REGION_CPU1, 0 )		/* Z80 Code */
	ROM_LOAD( "yunsung.128", 0x00000, 0x0c000, CRC(cb0f3d46) SHA1(b56c4abbd4248074c1559a0f1902d2ea11cb01a8) )
	ROM_CONTINUE(            0x10000, 0x34000 )

	ROM_REGION( 0x80000, REGION_GFX1, ROMREGION_DISPOSE | ROMREGION_INVERT)	/* 16x16x8 Sprites */
	ROM_LOAD( "yunsung.114", 0x00000, 0x40000, CRC(3dbe1872) SHA1(754f90123a3944ca548fc66ee65a93615155bf30) )
	ROM_LOAD( "yunsung.115", 0x40000, 0x40000, CRC(30f49dac) SHA1(b70d37973bd03069c48641d6c0804be6f9aa6553) )

	ROM_REGION( 0x20000, REGION_GFX2, ROMREGION_DISPOSE | ROMREGION_ERASEFF)	/* 8x8x4 Background */
	/* not for this game? */

	ROM_REGION( 0x100000, REGION_GFX3, ROMREGION_DISPOSE | ROMREGION_INVERT)	/* 8x8x8 Foreground */
	ROM_LOAD( "yunsung.92", 0x00000, 0x80000, CRC(bcf206a9) SHA1(0db2cee21c025b7b8d2d5b898c7231c77e36904d) )
	ROM_LOAD( "yunsung.93", 0x80000, 0x80000, CRC(64edb93c) SHA1(94f8d4fd159c682d952d6a4c38dc50f2c0c0824d) )

	ROM_REGION( 0x100000, REGION_GFX4, ROMREGION_DISPOSE | ROMREGION_INVERT)	/* 8x8x8 Midground */
	ROM_LOAD( "yunsung.110", 0x00000, 0x80000, CRC(c209201e) SHA1(ba1cb3a204f689f9a3636834628d2265927e34f7) )
	ROM_LOAD( "yunsung.111", 0x80000, 0x80000, CRC(82334337) SHA1(4b2a07196027b190366131cd7b8eca87a1bd0b1c) )

	ROM_REGION( 0x40000, REGION_SOUND1, 0 )	/* Samples */
	ROM_LOAD( "yunsung.85", 0x00000, 0x20000, CRC(cdf3336b) SHA1(98029d6d5d8ffb3b24ae2bcf950618a7d5b404c3) )

	ROM_REGION( 0x80000, REGION_SOUND2, 0 )	/* Samples (banked) */
	ROM_LOAD( "yunsung.113", 0x00000, 0x40000, CRC(150a6cc6) SHA1(b435fcf8ba48006f506db6b63ba54a30a6b3eade) )
ROM_END

ROM_START( torus )
	ROM_REGION( 0x14000, REGION_CPU1, 0 )		/* Z80 Code */
	ROM_LOAD( "bc13.bin",     0x00000, 0xc000, CRC(55d3ef3e) SHA1(195463271fdb3f9f5c19068efd1c99105f761fe9) )
	ROM_CONTINUE(             0x10000, 0x4000 )

	ROM_REGION( 0x80000, REGION_GFX1, ROMREGION_DISPOSE | ROMREGION_INVERT)	/* 16x16x8 Sprites */
	ROM_LOAD( "bc5.bin",      0x00000, 0x40000, CRC(5b60ce9f) SHA1(d5c091145e0bae7cd776e642ea17895d086ed2b0) )
	ROM_LOAD( "bc6.bin",      0x40000, 0x40000, CRC(4caa0c50) SHA1(a971b6e87cd1162cf370d39cfeafefbb1557e14e) )

	ROM_REGION( 0x20000, REGION_GFX2, ROMREGION_DISPOSE | ROMREGION_ERASEFF)	/* 8x8x4 Background */
	/* not for this game */

	ROM_REGION( 0x100000, REGION_GFX3, ROMREGION_DISPOSE | ROMREGION_INVERT)	/* 8x8x8 Foreground */
	ROM_LOAD( "bc2.bin",      0x00000, 0x80000, CRC(67c5ba1a) SHA1(0e39752ddc5ee9469647140a3fc9e6bb69d6afa1) )
	ROM_LOAD( "bc1.bin",      0x80000, 0x80000, CRC(efb105e9) SHA1(7bfe6ff64b25797dd524a7077def5669f25f16ec) )

	ROM_REGION( 0x40000, REGION_GFX4, ROMREGION_DISPOSE | ROMREGION_INVERT)	/* 8x8x8 Midground */
	ROM_LOAD( "bc4.bin",      0x00000, 0x20000, CRC(ee914caf) SHA1(42f3d760a4c14658ac2eb0ba7f54fb9916368b50) )
	ROM_LOAD( "bc3.bin",      0x20000, 0x20000, CRC(aff1dab9) SHA1(ae488abd605c1e78b8b73452a2c1391cc0fe6b00) )

	ROM_REGION( 0x40000, REGION_SOUND1, 0 )	/* Samples */
	ROM_LOAD( "bc15.bin",     0x00000, 0x40000, CRC(12d84839) SHA1(840d82253c0651ebe6799ea2bb5bae334e963e12) )
ROM_END

/*

Yun Sung Mad Ball

PCB: YS-0402

Z80A
AD-65 (OKI M6295)
Actel A1020B PLC84C
OSC: 12.000 MHz, 4.000Mhz

RAM 4 Hyundai HY62256ALP-70
    1 Hyundai HY6264LP-10 (by P rom & Z80A)
DSW 2 8-switch DIP


P.u1  Intel i27C010A - Program (next to Z80A)
s.u28 ST M27C4001    - Sound (next to AD-65)

1.u66  ST M27C2001
2.u67  ST M27C2001
3.u92  TI 27C040
4.u93  TI 27C040
5.u105 TI 27C040
6.u106 TI 27C040

All roms read with manufacturer's IDs and routines

*/

ROM_START( madball )
	ROM_REGION( 0x24000, REGION_CPU1, 0 )		/* Z80 Code */
	ROM_LOAD( "p.u1",     0x00000, 0xc000, CRC(73008425) SHA1(6eded60fd5c637a63783247c858d999d5974d378) )
	ROM_CONTINUE(             0x10000, 0x14000 )

	ROM_REGION( 0x80000, REGION_GFX1, ROMREGION_DISPOSE | ROMREGION_INVERT)	/* 16x16x8 Sprites */
	ROM_LOAD( "2.u67",      0x00000, 0x40000, CRC(1f3a6cd5) SHA1(7a17549f2fff003605d91703c84a398488b2f74c) )
	ROM_LOAD( "1.u66",      0x40000, 0x40000, CRC(8637c7b4) SHA1(e0026e48f0e8f3554a5b448e0d1f9d1c5551dbfb) )

	ROM_REGION( 0x20000, REGION_GFX2, ROMREGION_DISPOSE | ROMREGION_ERASEFF)	/* 8x8x4 Background */
	/* not for this game */

	ROM_REGION( 0x100000, REGION_GFX3, ROMREGION_DISPOSE | ROMREGION_INVERT)	/* 8x8x8 Foreground */
	ROM_LOAD( "5.u105",      0x00000, 0x80000, CRC(f26aac1e) SHA1(50ad34ee70bf45fa4e1dc9281b83bcdd7c7db3f8) )
	ROM_LOAD( "6.u106",      0x80000, 0x80000, CRC(27b78907) SHA1(ab6645457adc0d17b141e366aac7e00e8ce4296b) )

	ROM_REGION( 0x100000, REGION_GFX4, ROMREGION_DISPOSE | ROMREGION_INVERT)	/* 8x8x8 Midground */
	ROM_LOAD( "4.u93",      0x80000, 0x80000, CRC(c3be56ad) SHA1(9cfa0b38c60798deccca74dc6b0ce0826ff7f467) )
	ROM_LOAD( "3.u92",      0x00000, 0x80000, CRC(846019a6) SHA1(571bfa299e13b96ca263bd7e62c760bdbe3438bd) )

	ROM_REGION( 0x80000, REGION_SOUND1, 0 )	/* Samples */
	ROM_LOAD( "s.u28",     0x00000, 0x80000, CRC(78f02584) SHA1(70542e126db73a573db9ef41399d3a07fb7ea94b) )
ROM_END

DRIVER_INIT (paradise)
{
	paradise_sprite_inc = 0x20;
}

// Inverted flipscreen and sprites are packed in less memory (same number though)
DRIVER_INIT (tgtball)
{
	paradise_sprite_inc = 4;
	memory_install_write8_handler(0, ADDRESS_SPACE_IO, 0x2001, 0x2001, 0, 0, tgtball_flipscreen_w );
}

DRIVER_INIT (torus)
{
	paradise_sprite_inc = 4;
	memory_install_write8_handler(0, ADDRESS_SPACE_IO, 0x2070, 0x2070, 0, 0, torus_coin_counter_w);
}


/***************************************************************************

                                Game Drivers

***************************************************************************/

GAME( 1994+, paradise, 0,       paradise, paradise, paradise, ROT90, "Yun Sung", "Paradise", 0 )
GAME( 1994+, paradlx,  0,       paradise, paradise, paradise, ROT90, "Yun Sung", "Paradise Deluxe", GAME_IMPERFECT_GRAPHICS )
GAME( 1995,  tgtball,  0,       tgtball,  tgtball,  tgtball,  ROT0,  "Yun Sung", "Target Ball (Nude)", 0 )
GAME( 1995,  tgtballa, tgtball, tgtball,  tgtball,  tgtball,  ROT0,  "Yun Sung", "Target Ball", 0 )
GAME( 1996,  torus,    0,       torus,    torus,    torus,    ROT90, "Yun Sung", "Torus", 0 )
GAME( 1998,  madball,  0,       madball,  madball,  tgtball,  ROT0,  "Yun Sung", "Mad Ball V2.0", 0 )
