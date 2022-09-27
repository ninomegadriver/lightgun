/***************************************************************************

                          -= Yun Sung 16 Bit Games =-

                    driver by   Luca Elia (l.elia@tin.it)


Main  CPU    :  MC68000
Sound CPU    :  Z80 [Optional]
Video Chips  :  Actel A1020B PL84C
Sound Chips  :  OKI M6295 + YM3812 [Optional]


---------------------------------------------------------------------------
Year + Game         Board#
---------------------------------------------------------------------------
19?? Magic Bubble     YS-1302
1997 Shocking
1998 Bomb Kick        YS-0211
---------------------------------------------------------------------------

- Screen flipping: not used!?

Original bugs:

- In shocking, service mode just shows the menu, with mangled graphics
  (sprites, but the charset they used is in the tiles ROMs!).
  In magicbub they used color 0 for tiles (all blacks, so you can't see
  most of it!). Again, color 0 for sprites would be ok. Some kind
  of sprites-tiles swapping, or unfinished leftovers?

***************************************************************************/

#include "driver.h"
#include "sound/okim6295.h"
#include "sound/3812intf.h"

/* Variables defined in vidhrdw: */

extern UINT16 *yunsun16_vram_0,   *yunsun16_vram_1;
extern UINT16 *yunsun16_scroll_0, *yunsun16_scroll_1;
extern UINT16 *yunsun16_priority;

/* Functions defined in vidhrdw: */

WRITE16_HANDLER( yunsun16_vram_0_w );
WRITE16_HANDLER( yunsun16_vram_1_w );

VIDEO_START( yunsun16 );
VIDEO_UPDATE( yunsun16 );


/***************************************************************************


                            Memory Maps - Main CPU


***************************************************************************/

static WRITE16_HANDLER( yunsun16_sound_bank_w )
{
	if (ACCESSING_LSB)
	{
		int bank = data & 3;
		unsigned char *dst	= memory_region(REGION_SOUND1);
		unsigned char *src	= dst + 0x80000 + 0x20000 * bank;
		memcpy(dst + 0x20000, src, 0x20000);
	}
}

static ADDRESS_MAP_START( yunsun16_readmem, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x07ffff) AM_READ(MRA16_ROM					)	// ROM
	AM_RANGE(0xff0000, 0xffffff) AM_READ(MRA16_RAM					)	// RAM
	AM_RANGE(0x800000, 0x800001) AM_READ(input_port_0_word_r		)	// P1 + P2
	AM_RANGE(0x800018, 0x800019) AM_READ(input_port_1_word_r		)	// Coins
	AM_RANGE(0x80001a, 0x80001b) AM_READ(input_port_2_word_r		)	// DSW1
	AM_RANGE(0x80001c, 0x80001d) AM_READ(input_port_3_word_r		)	// DSW2
	AM_RANGE(0x800188, 0x800189) AM_READ(OKIM6295_status_0_lsb_r	)	// Sound
	AM_RANGE(0x900000, 0x903fff) AM_READ(MRA16_RAM					)	// Palette
	AM_RANGE(0x908000, 0x90bfff) AM_READ(MRA16_RAM					)	// Layer 1
	AM_RANGE(0x90c000, 0x90ffff) AM_READ(MRA16_RAM					)	// Layer 0
	AM_RANGE(0x910000, 0x910fff) AM_READ(MRA16_RAM					)	// Sprites
ADDRESS_MAP_END

static ADDRESS_MAP_START( yunsun16_writemem, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x07ffff) AM_WRITE(MWA16_ROM					)	// ROM
	AM_RANGE(0xff0000, 0xffffff) AM_WRITE(MWA16_RAM					)	// RAM
	AM_RANGE(0x800030, 0x800031) AM_WRITE(MWA16_NOP					)	// ? (value: don't care)
	AM_RANGE(0x800100, 0x800101) AM_WRITE(MWA16_NOP					)	// ? $9100
	AM_RANGE(0x800102, 0x800103) AM_WRITE(MWA16_NOP					)	// ? $9080
	AM_RANGE(0x800104, 0x800105) AM_WRITE(MWA16_NOP					)	// ? $90c0
	AM_RANGE(0x80010a, 0x80010b) AM_WRITE(MWA16_NOP					)	// ? $9000
	AM_RANGE(0x80010c, 0x80010f) AM_WRITE(MWA16_RAM) AM_BASE(&yunsun16_scroll_1	)	// Scrolling
	AM_RANGE(0x800114, 0x800117) AM_WRITE(MWA16_RAM) AM_BASE(&yunsun16_scroll_0	)	//
	AM_RANGE(0x800154, 0x800155) AM_WRITE(MWA16_RAM) AM_BASE(&yunsun16_priority	)	// Priority
	AM_RANGE(0x800180, 0x800181) AM_WRITE(yunsun16_sound_bank_w		)	// Sound
	AM_RANGE(0x800188, 0x800189) AM_WRITE(OKIM6295_data_0_lsb_w		)	//
	AM_RANGE(0x8001fe, 0x8001ff) AM_WRITE(MWA16_NOP					)	// ? 0 (during int)
	AM_RANGE(0x900000, 0x903fff) AM_WRITE(paletteram16_xRRRRRGGGGGBBBBB_word_w) AM_BASE(&paletteram16	)	// Palette
	AM_RANGE(0x908000, 0x90bfff) AM_WRITE(yunsun16_vram_1_w) AM_BASE(&yunsun16_vram_1		)	// Layer 1
	AM_RANGE(0x90c000, 0x90ffff) AM_WRITE(yunsun16_vram_0_w) AM_BASE(&yunsun16_vram_0		)	// Layer 0
	AM_RANGE(0x910000, 0x910fff) AM_WRITE(MWA16_RAM) AM_BASE(&spriteram16) AM_SIZE(&spriteram_size	)	// Sprites
ADDRESS_MAP_END


static WRITE16_HANDLER( magicbub_sound_command_w )
{
	if (ACCESSING_LSB)
	{
/*
HACK: the game continuously sends this. It'll play the oki sample
number 0 on each voice. That sample is 00000-00000.
*/
		if ((data&0xff)!=0x3a)
		{
		soundlatch_w(0,data & 0xff);
		cpunum_set_input_line(1, INPUT_LINE_NMI, PULSE_LINE);
		}
	}
}

DRIVER_INIT( magicbub )
{
//  remove_mem_write16_handler (0, 0x800180, 0x800181 );
	memory_install_write16_handler(0, ADDRESS_SPACE_PROGRAM, 0x800188, 0x800189, 0, 0, magicbub_sound_command_w);
}

/***************************************************************************


                            Memory Maps - Sound CPU


***************************************************************************/

static ADDRESS_MAP_START( yunsun16_sound_readmem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0xdfff) AM_READ(MRA8_ROM		)	// ROM
	AM_RANGE(0xe000, 0xe7ff) AM_READ(MRA8_RAM		)	// RAM
ADDRESS_MAP_END

static ADDRESS_MAP_START( yunsun16_sound_writemem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0xdfff) AM_WRITE(MWA8_ROM		)	// ROM
	AM_RANGE(0xe000, 0xe7ff) AM_WRITE(MWA8_RAM		)	// RAM
ADDRESS_MAP_END

static ADDRESS_MAP_START( yunsun16_sound_readport, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_FLAGS( AMEF_ABITS(8) )
	AM_RANGE(0x10, 0x10) AM_READ(YM3812_status_port_0_r	)	// YM3812
	AM_RANGE(0x18, 0x18) AM_READ(soundlatch_r				)	// From Main CPU
	AM_RANGE(0x1c, 0x1c) AM_READ(OKIM6295_status_0_r		)	// M6295
ADDRESS_MAP_END

static ADDRESS_MAP_START( yunsun16_sound_writeport, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_FLAGS( AMEF_ABITS(8) )
	AM_RANGE(0x10, 0x10) AM_WRITE(YM3812_control_port_0_w	)	// YM3812
	AM_RANGE(0x11, 0x11) AM_WRITE(YM3812_write_port_0_w		)
	AM_RANGE(0x1c, 0x1c) AM_WRITE(OKIM6295_data_0_w			)	// M6295
ADDRESS_MAP_END


/***************************************************************************


                                Input Ports


***************************************************************************/


/***************************************************************************
                                Magic Bubble
***************************************************************************/

INPUT_PORTS_START( magicbub )

	PORT_START	// IN0 - $800000.w
	PORT_BIT(  0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_PLAYER(1)
	PORT_BIT(  0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_PLAYER(1)
	PORT_BIT(  0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_PLAYER(1)
	PORT_BIT(  0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_PLAYER(1)
	PORT_BIT(  0x0010, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(1)
	PORT_BIT(  0x0020, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x0040, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x0080, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_BIT(  0x0100, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_PLAYER(2)
	PORT_BIT(  0x0200, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_PLAYER(2)
	PORT_BIT(  0x0400, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_PLAYER(2)
	PORT_BIT(  0x0800, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_PLAYER(2)
	PORT_BIT(  0x1000, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(2)
	PORT_BIT(  0x2000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x4000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x8000, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	// IN1 - $800019.b
	PORT_BIT(  0x0001, IP_ACTIVE_LOW, IPT_COIN1   )
	PORT_BIT(  0x0002, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x0004, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x0008, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x0010, IP_ACTIVE_LOW, IPT_START1  )
	PORT_BIT(  0x0020, IP_ACTIVE_LOW, IPT_START2  )
	PORT_BIT(  0x0040, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x0080, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	// IN2 - $80001b.b
	PORT_DIPNAME( 0x0007, 0x0007, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(      0x0002, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(      0x0004, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0x0001, DEF_STR( 3C_2C ) )
	PORT_DIPSETTING(      0x0007, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x0003, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(      0x0006, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(      0x0005, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( Free_Play ) )
	PORT_DIPNAME( 0x0018, 0x0010, "Unknown 1-4&5" )
	PORT_DIPSETTING(      0x0010, "0" )
	PORT_DIPSETTING(      0x0018, "2" )
	PORT_DIPSETTING(      0x0008, "4" )
	PORT_DIPSETTING(      0x0000, "8" )
	PORT_DIPNAME( 0x0020, 0x0020, "Unknown 1-5" )
	PORT_DIPSETTING(      0x0020, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0040, 0x0000, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(      0x0040, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_SERVICE( 0x0080, IP_ACTIVE_LOW )

	PORT_START	// IN3 - $80001d.b
	PORT_DIPNAME( 0x0001, 0x0001, "Unknown 2-0" )
	PORT_DIPSETTING(      0x0001, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0002, 0x0002, "Unknown 2-1" )
	PORT_DIPSETTING(      0x0002, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x000c, 0x000c, "Lives (Vs Mode)" )
	PORT_DIPSETTING(      0x0008, "1" )
	PORT_DIPSETTING(      0x000c, "2" )
	PORT_DIPSETTING(      0x0004, "3" )
	PORT_DIPSETTING(      0x0000, "4" )
	PORT_DIPNAME( 0x0010, 0x0010, "Unknown 2-4*" )
	PORT_DIPSETTING(      0x0000, "1" )
	PORT_DIPSETTING(      0x0010, "2" )
	PORT_DIPNAME( 0x0020, 0x0020, "Unknown 2-5" )
	PORT_DIPSETTING(      0x0020, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0040, 0x0040, "Unknown 2-6" )
	PORT_DIPSETTING(      0x0040, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0080, 0x0080, "Unknown 2-7" )
	PORT_DIPSETTING(      0x0080, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )

INPUT_PORTS_END

/***************************************************************************
                        Magic Bubble (Adult version)
***************************************************************************/

INPUT_PORTS_START( magicbua )

	PORT_START	// IN0 - $800000.w
	PORT_BIT(  0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_PLAYER(1)
	PORT_BIT(  0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_PLAYER(1)
	PORT_BIT(  0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_PLAYER(1)
	PORT_BIT(  0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_PLAYER(1)
	PORT_BIT(  0x0010, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(1)
	PORT_BIT(  0x0020, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x0040, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x0080, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_BIT(  0x0100, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_PLAYER(2)
	PORT_BIT(  0x0200, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_PLAYER(2)
	PORT_BIT(  0x0400, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_PLAYER(2)
	PORT_BIT(  0x0800, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_PLAYER(2)
	PORT_BIT(  0x1000, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(2)
	PORT_BIT(  0x2000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x4000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x8000, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	// IN1 - $800019.b
	PORT_BIT(  0x0001, IP_ACTIVE_LOW, IPT_COIN1   )
	PORT_BIT(  0x0002, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x0004, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x0008, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x0010, IP_ACTIVE_LOW, IPT_START1  )
	PORT_BIT(  0x0020, IP_ACTIVE_LOW, IPT_START2  )
	PORT_BIT(  0x0040, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x0080, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	// IN2 - $80001b.b
	PORT_DIPNAME( 0x0007, 0x0007, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(      0x0002, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(      0x0004, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0x0001, DEF_STR( 3C_2C ) )
	PORT_DIPSETTING(      0x0007, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x0003, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(      0x0006, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(      0x0005, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( Free_Play ) )
	PORT_DIPNAME( 0x0018, 0x0010, "Unknown 1-4&5" )
	PORT_DIPSETTING(      0x0010, "0" )
	PORT_DIPSETTING(      0x0018, "2" )
	PORT_DIPSETTING(      0x0008, "4" )
	PORT_DIPSETTING(      0x0000, "8" )
	PORT_DIPNAME( 0x0020, 0x0020, "Unknown 1-5" )
	PORT_DIPSETTING(      0x0020, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0040, 0x0000, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(      0x0040, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_SERVICE( 0x0080, IP_ACTIVE_LOW )

	PORT_START	// IN3 - $80001d.b
	PORT_DIPNAME( 0x0003, 0x0003, "Nudity" )
	PORT_DIPSETTING(      0x0003, DEF_STR( Low ) )
	PORT_DIPSETTING(      0x0001, DEF_STR( Medium ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( High ) )
	PORT_DIPNAME( 0x000c, 0x000c, "Lives (Vs Mode)" )
	PORT_DIPSETTING(      0x0008, "1" )
	PORT_DIPSETTING(      0x000c, "2" )
	PORT_DIPSETTING(      0x0004, "3" )
	PORT_DIPSETTING(      0x0000, "4" )
	PORT_DIPNAME( 0x0010, 0x0010, "Unknown 2-4*" )
	PORT_DIPSETTING(      0x0000, "1" )
	PORT_DIPSETTING(      0x0010, "2" )
	PORT_DIPNAME( 0x0020, 0x0020, "Unknown 2-5" )
	PORT_DIPSETTING(      0x0020, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0040, 0x0040, "Unknown 2-6" )
	PORT_DIPSETTING(      0x0040, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0080, 0x0080, "Unknown 2-7" )
	PORT_DIPSETTING(      0x0080, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )

INPUT_PORTS_END

/***************************************************************************
                                Shocking
***************************************************************************/

INPUT_PORTS_START( shocking )

	PORT_START	// IN0 - $800000.w
	PORT_BIT(  0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_PLAYER(1)
	PORT_BIT(  0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_PLAYER(1)
	PORT_BIT(  0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_PLAYER(1)
	PORT_BIT(  0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_PLAYER(1)
	PORT_BIT(  0x0010, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(1)
	PORT_BIT(  0x0020, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x0040, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x0080, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_BIT(  0x0100, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_PLAYER(2)
	PORT_BIT(  0x0200, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_PLAYER(2)
	PORT_BIT(  0x0400, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_PLAYER(2)
	PORT_BIT(  0x0800, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_PLAYER(2)
	PORT_BIT(  0x1000, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(2)
	PORT_BIT(  0x2000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x4000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x8000, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	// IN1 - $800019.b
	PORT_BIT(  0x0001, IP_ACTIVE_LOW, IPT_COIN1   )
	PORT_BIT(  0x0002, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x0004, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x0008, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x0010, IP_ACTIVE_LOW, IPT_START1  )
	PORT_BIT(  0x0020, IP_ACTIVE_LOW, IPT_START2  )
	PORT_BIT(  0x0040, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x0080, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	// IN2 - $80001b.b
	PORT_DIPNAME( 0x0007, 0x0007, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(      0x0001, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(      0x0002, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0x0003, DEF_STR( 3C_2C ) )
	PORT_DIPSETTING(      0x0007, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x0004, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(      0x0006, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(      0x0005, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( Free_Play ) )
	PORT_DIPNAME( 0x0008, 0x0008, "Unknown 1-3" )		// rest unused
	PORT_DIPSETTING(      0x0008, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0010, 0x0010, "Unknown 1-4" )
	PORT_DIPSETTING(      0x0010, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0020, 0x0020, "Unknown 1-5" )
	PORT_DIPSETTING(      0x0020, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0040, 0x0040, "Unknown 1-6" )
	PORT_DIPSETTING(      0x0040, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0080, 0x0080, "Unknown 1-7" )
	PORT_DIPSETTING(      0x0080, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )

	PORT_START	// IN3 - $80001d.b
	PORT_DIPNAME( 0x0007, 0x0007, "Unknown 2-0&1&2" )
	PORT_DIPSETTING(      0x0004, "0" )
	PORT_DIPSETTING(      0x0005, "1" )
	PORT_DIPSETTING(      0x0006, "2" )
	PORT_DIPSETTING(      0x0007, "3" )
	PORT_DIPSETTING(      0x0003, "4" )
	PORT_DIPSETTING(      0x0002, "5" )
	PORT_DIPSETTING(      0x0001, "6" )
	PORT_DIPSETTING(      0x0000, "7" )
	PORT_DIPNAME( 0x0008, 0x0008, "Unknown 2-3" )
	PORT_DIPSETTING(      0x0008, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0030, 0x0030, DEF_STR( Lives ) )
	PORT_DIPSETTING(      0x0020, "2" )
	PORT_DIPSETTING(      0x0030, "3" )
	PORT_DIPSETTING(      0x0010, "4" )
	PORT_DIPSETTING(      0x0000, "5" )
	PORT_DIPNAME( 0x0040, 0x0000, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(      0x0040, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_SERVICE( 0x0080, IP_ACTIVE_LOW )

INPUT_PORTS_END

/***************************************************************************
                                bomb Kick
***************************************************************************/

INPUT_PORTS_START( bombkick )

	PORT_START	// IN0 - $800000.w
	PORT_BIT(  0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_PLAYER(1)
	PORT_BIT(  0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_PLAYER(1)
	PORT_BIT(  0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_PLAYER(1)
	PORT_BIT(  0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_PLAYER(1)
	PORT_BIT(  0x0010, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(1)
	PORT_BIT(  0x0020, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(1)
	PORT_BIT(  0x0040, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_PLAYER(1)
	PORT_BIT(  0x0080, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_BIT(  0x0100, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_PLAYER(2)
	PORT_BIT(  0x0200, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_PLAYER(2)
	PORT_BIT(  0x0400, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_PLAYER(2)
	PORT_BIT(  0x0800, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_PLAYER(2)
	PORT_BIT(  0x1000, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(2)
	PORT_BIT(  0x2000, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(2)
	PORT_BIT(  0x4000, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_PLAYER(2)
	PORT_BIT(  0x8000, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	// IN1 - $800019.b
	PORT_BIT(  0x0001, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT(  0x0002, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x0004, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x0008, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x0010, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT(  0x0020, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT(  0x0040, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x0080, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0xff00, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	// IN2 - $80001b.b
	PORT_DIPNAME( 0x0007, 0x0007, DEF_STR( Coinage ) )
	PORT_DIPSETTING(      0x0002, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(      0x0004, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0x0001, DEF_STR( 3C_2C ) )
	PORT_DIPSETTING(      0x0007, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x0003, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(      0x0006, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(      0x0005, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( Free_Play ) )
	PORT_DIPNAME( 0x0018, 0x0018, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( Very_Hard ) )
	PORT_DIPSETTING(      0x0008, DEF_STR( Hard ) )
	PORT_DIPSETTING(      0x0018, DEF_STR( Normal ) )
	PORT_DIPSETTING(      0x0010, DEF_STR( Easy ) )
	PORT_DIPNAME( 0x0020, 0x0020, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0020, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0040, 0x0000, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(      0x0040, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_SERVICE( 0x0080, IP_ACTIVE_LOW )
	PORT_BIT(  0xff00, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	// IN3 - $80001d.b
	PORT_DIPNAME( 0x0003, 0x0003, DEF_STR( Lives ) )
	PORT_DIPSETTING(      0x0001, "5" )
	PORT_DIPSETTING(      0x0002, "4" )
	PORT_DIPSETTING(      0x0003, "3" )
	PORT_DIPSETTING(      0x0000, "2" )
	PORT_DIPNAME( 0x0004, 0x0004, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0004, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_BIT(  0x0008, IP_ACTIVE_LOW,  IPT_UNKNOWN ) // keep it off --> otherwise it hangs when you select some levels
	PORT_BIT(  0x0010, IP_ACTIVE_HIGH, IPT_UNKNOWN ) // keep it on  /
	PORT_DIPNAME( 0x0020, 0x0020, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0020, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_BIT(  0x0040, IP_ACTIVE_HIGH, IPT_UNKNOWN ) // keep it on  --> otherwise it doesn't boot
	PORT_BIT(  0x0080, IP_ACTIVE_LOW,  IPT_UNKNOWN ) // keep it off /
	PORT_BIT(  0xff00, IP_ACTIVE_LOW,  IPT_UNUSED )

INPUT_PORTS_END


/***************************************************************************


                            Graphics Layouts


***************************************************************************/


/* 16x16x4 */
static const gfx_layout layout_16x16x4 =
{
	16,16,
	RGN_FRAC(1,4),
	4,
	{ RGN_FRAC(3,4), RGN_FRAC(1,4), RGN_FRAC(2,4), RGN_FRAC(0,4) },
	{ STEP16(0,1) },
	{ STEP16(0,16) },
	16*16
};

/* 16x16x8 */
static const gfx_layout layout_16x16x8 =
{
	16,16,
	RGN_FRAC(1,1),
	8,
	{ 6*8,4*8, 2*8,0*8, 7*8,5*8, 3*8,1*8 },
	{ STEP8(0,1),STEP8(8*8,1) },
	{ STEP16(0,16*8) },
	16*16*8
};


static const gfx_decode yunsun16_gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &layout_16x16x8, 0x1000, 0x10 }, // [0] Layers
	{ REGION_GFX2, 0, &layout_16x16x4, 0x0000, 0x20 }, // [1] Sprites
	{ -1 }
};


/***************************************************************************


                                Machine Drivers


***************************************************************************/

/***************************************************************************
                                Magic Bubble
***************************************************************************/

static void soundirq(int state)
{
	cpunum_set_input_line(1, 0, state);
}

static struct YM3812interface magicbub_ym3812_intf =
{
	soundirq	/* IRQ Line */
};

static MACHINE_DRIVER_START( magicbub )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000, 16000000)
	MDRV_CPU_PROGRAM_MAP(yunsun16_readmem,yunsun16_writemem)
	MDRV_CPU_VBLANK_INT(irq2_line_hold,1)

	MDRV_CPU_ADD(Z80, 3000000)
	/* audio CPU */	/* ? */
	MDRV_CPU_PROGRAM_MAP(yunsun16_sound_readmem,yunsun16_sound_writemem)
	MDRV_CPU_IO_MAP(yunsun16_sound_readport,yunsun16_sound_writeport)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(0x180, 0xe0)
	MDRV_VISIBLE_AREA(0+0x20, 0x180-1-0x20, 0, 0xe0-1)
	MDRV_GFXDECODE(yunsun16_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(8192)

	MDRV_VIDEO_START(yunsun16)
	MDRV_VIDEO_UPDATE(yunsun16)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_STEREO("left", "right")

	MDRV_SOUND_ADD(YM3812, 4000000)
	MDRV_SOUND_CONFIG(magicbub_ym3812_intf)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "left", 0.20)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "right", 0.20)

	MDRV_SOUND_ADD(OKIM6295, 8000)
	MDRV_SOUND_CONFIG(okim6295_interface_region_1)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "left", 0.80)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "right", 0.80)
MACHINE_DRIVER_END


/***************************************************************************
                                Shocking
***************************************************************************/

static MACHINE_DRIVER_START( shocking )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000, 16000000)
	MDRV_CPU_PROGRAM_MAP(yunsun16_readmem,yunsun16_writemem)
	MDRV_CPU_VBLANK_INT(irq2_line_hold,1)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(0x180, 0xe0)
	MDRV_VISIBLE_AREA(0, 0x180-1-4, 0, 0xe0-1)
	MDRV_GFXDECODE(yunsun16_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(8192)

	MDRV_VIDEO_START(yunsun16)
	MDRV_VIDEO_UPDATE(yunsun16)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_STEREO("left", "right")

	MDRV_SOUND_ADD(OKIM6295, 1000000 / 132)
	MDRV_SOUND_CONFIG(okim6295_interface_region_1)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "left", 1.0)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "right", 1.0)
MACHINE_DRIVER_END



/***************************************************************************


                                ROMs Loading


***************************************************************************/

/***************************************************************************

                                Magic Bubble

by Yun Sung YS102

U143 -------------------27c512
U23, 21, 22, 20, 131 ---27c010
U67, 68, 69, 70 --------27c040
U32, 33 ----------------27c020

U143, 131 .......most likely sound
U32, 33 .........most likely program
U20-23 ..........most likely sprites
U67-70 ..........most likely BG

Actel A1020b is close to U67-70

68HC000 p16 is close to  U32,33

16.000000 MHz

Sound section:
 SMD Z80
  "KS8001" (YM3812)
  "KS8002" (YM3014)
  "AD-65"  (OKI M6295 and is SMD)

***************************************************************************/

ROM_START( magicbub )

	ROM_REGION( 0x080000, REGION_CPU1, 0 )		/* 68000 Code */
	ROM_LOAD16_BYTE( "magbuble.u33", 0x000000, 0x040000, CRC(18fdd582) SHA1(89f4c52ec0e213285a04743da88f6e39408b573d) )
	ROM_LOAD16_BYTE( "magbuble.u32", 0x000001, 0x040000, CRC(f6ea7004) SHA1(069541e37b60370810451616ee66bbd05dc10137) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )		/* Z80 Code */
	ROM_LOAD( "magbuble.143", 0x00000, 0x10000, CRC(04192753) SHA1(9c56ba70e1d074906ea1dc593c2a8516c6ba2074) )

	ROM_REGION( 0x200000*8, REGION_GFX1, ROMREGION_ERASEFF | ROMREGION_DISPOSE )	/* 16x16x8 */
	ROMX_LOAD( "magbuble.u67", 0x000000, 0x080000, CRC(6355e57d) SHA1(5e9234dd474ddcf0a9e1001080f3de11c7d0ee55) , ROM_GROUPWORD | ROM_SKIP(6))
	ROMX_LOAD( "magbuble.u68", 0x000002, 0x080000, CRC(53ae6c2b) SHA1(43c02aa4cfdfa5bc009b42cd4be633787a35cb59) , ROM_GROUPWORD | ROM_SKIP(6))
	ROMX_LOAD( "magbuble.u69", 0x000004, 0x080000, CRC(b892e64c) SHA1(b1156c8f02371ee2c5d6c930483c50eef5da10b5) , ROM_GROUPWORD | ROM_SKIP(6))
	ROMX_LOAD( "magbuble.u70", 0x000006, 0x080000, CRC(37794837) SHA1(11597614e1e048544326fbbe281b364278d6350d) , ROM_GROUPWORD | ROM_SKIP(6))

	ROM_REGION( 0x080000, REGION_GFX2, ROMREGION_DISPOSE )	/* 16x16x4 */
	ROM_LOAD( "magbuble.u20", 0x000000, 0x020000, CRC(f70e3b8c) SHA1(d925c27bbd0f915228d22589a98e3ea7181a87ca) )
	ROM_LOAD( "magbuble.u21", 0x020000, 0x020000, CRC(ad082cf3) SHA1(0bc3cf6c54d47be4f1940192fc1585cb48767e97) )
	ROM_LOAD( "magbuble.u22", 0x040000, 0x020000, CRC(7c68df7a) SHA1(88acf9dd43892a790415b418f77d88c747aa84f5) )
	ROM_LOAD( "magbuble.u23", 0x060000, 0x020000, CRC(c7763fc1) SHA1(ed68b3c3c5155073afb7b55d6d92d3057e40df6c) )

	ROM_REGION( 0x020000, REGION_SOUND1, 0 )	/* Samples */
	ROM_LOAD( "magbuble.131", 0x000000, 0x020000, CRC(03e04e89) SHA1(7d80e6a7be2322e32e40acae72bedd8d7e90ad33) )

ROM_END

ROM_START( magicbua )

	ROM_REGION( 0x080000, REGION_CPU1, 0 )		/* 68000 Code */
	ROM_LOAD16_BYTE( "u33.bin", 0x000000, 0x040000, CRC(a8164a02) SHA1(7275209d5d73881839f7fa3ac7d362194ef2cfd9) )
	ROM_LOAD16_BYTE( "u32.bin", 0x000001, 0x040000, CRC(58f885ad) SHA1(e66f5bb1ac0acd9abc2def439af7f932c3a09cbd) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )		/* Z80 Code */
	ROM_LOAD( "magbuble.143", 0x00000, 0x10000, CRC(04192753) SHA1(9c56ba70e1d074906ea1dc593c2a8516c6ba2074) )

	ROM_REGION( 0x200000*8, REGION_GFX1, ROMREGION_ERASEFF | ROMREGION_DISPOSE )	/* 16x16x8 */
	ROMX_LOAD( "u67.bin", 0x000000, 0x080000, CRC(89523dcd) SHA1(edea2bbec615aa253d940bbc3bbdb33f6873a8ee) , ROM_GROUPWORD | ROM_SKIP(6))
	ROMX_LOAD( "u68.bin", 0x000002, 0x080000, CRC(30e01a70) SHA1(3a98c2ef61307b44bf4e155663117199587ff4a4) , ROM_GROUPWORD | ROM_SKIP(6))
	ROMX_LOAD( "u69.bin", 0x000004, 0x080000, CRC(fe357f52) SHA1(5aff9a0bf70fc8a78820c4d13838ad238852c594) , ROM_GROUPWORD | ROM_SKIP(6))
	ROMX_LOAD( "u70.bin", 0x000006, 0x080000, CRC(1398a473) SHA1(f58bda6cbf5f553a9632d910b2ffef5d5bfedf18) , ROM_GROUPWORD | ROM_SKIP(6))
	ROMX_LOAD( "u71.bin", 0x200000, 0x080000, CRC(0844e017) SHA1(2ae5c9da521fea7aa5811627d7b3eca82cdc0821) , ROM_GROUPWORD | ROM_SKIP(6))
	ROMX_LOAD( "u72.bin", 0x200002, 0x080000, CRC(591db1cb) SHA1(636fbfe9e048d6418d43f947004b281f61081fd8) , ROM_GROUPWORD | ROM_SKIP(6))
	ROMX_LOAD( "u73.bin", 0x200004, 0x080000, CRC(cb4f3c3c) SHA1(fbd804bb70f09c2471557675af4c5b4abedea3b2) , ROM_GROUPWORD | ROM_SKIP(6))
	ROMX_LOAD( "u74.bin", 0x200006, 0x080000, CRC(81ff4910) SHA1(69241fe2d20b53984aa67f17d8da32e1b74ce696) , ROM_GROUPWORD | ROM_SKIP(6))

	ROM_REGION( 0x080000, REGION_GFX2, ROMREGION_DISPOSE )	/* 16x16x4 */
	ROM_LOAD( "magbuble.u20", 0x000000, 0x020000, CRC(f70e3b8c) SHA1(d925c27bbd0f915228d22589a98e3ea7181a87ca) )
	ROM_LOAD( "magbuble.u21", 0x020000, 0x020000, CRC(ad082cf3) SHA1(0bc3cf6c54d47be4f1940192fc1585cb48767e97) )
	ROM_LOAD( "magbuble.u22", 0x040000, 0x020000, CRC(7c68df7a) SHA1(88acf9dd43892a790415b418f77d88c747aa84f5) )
	ROM_LOAD( "magbuble.u23", 0x060000, 0x020000, CRC(c7763fc1) SHA1(ed68b3c3c5155073afb7b55d6d92d3057e40df6c) )

	ROM_REGION( 0x020000, REGION_SOUND1, 0 )	/* Samples */
	ROM_LOAD( "magbuble.131", 0x000000, 0x020000, CRC(03e04e89) SHA1(7d80e6a7be2322e32e40acae72bedd8d7e90ad33) )

ROM_END


/***************************************************************************

                                Shocking
Yunsung, 1997

PCB Layout
----------

|-------------------------------------------------|
|UPC1242  ROM.U131           ROM.U20     6116     |
|     VOL M6295              ROM.U22     6116     |
|  PAL 6264                  ROM.U21              |
|      6264                  ROM.U23     6116     |
|                                        6116     |
|                  PAL                      PAL   |
|J                 PAL                      PAL   |
|A  DSW1                       PAL                |
|M                                                |
|M  DSW2                PAL   ACTEL         PAL   |
|A        PAL           PAL   A1020B  6116  6116  |
|         PAL  PAL      PAL           6116  6116  |
|              PAL                    ROM.U70     |
|                62256                ROM.U69     |
|        68000   62256                ROM.U68     |
|                ROM.U32              ROM.U67     |
|16MHz           ROM.U33           62256  62256   |
|-------------------------------------------------|
Notes:
      68000 clock - 16MHz
      M6295 clock - 1.000MHz, sample rate 1000000Hz / 132
      HSync - 14.84kHz
      VSync - 60Hz

***************************************************************************/

ROM_START( shocking )

	ROM_REGION( 0x080000, REGION_CPU1, 0 )		/* 68000 Code */
	ROM_LOAD16_BYTE( "yunsun16.u33", 0x000000, 0x040000, CRC(8a155521) SHA1(000c9095558e6cae30ce43a885c3fbcf55713f40) )
	ROM_LOAD16_BYTE( "yunsun16.u32", 0x000001, 0x040000, CRC(c4998c10) SHA1(431ae1f9982a70421650e1bfe4bf87152e2fe85c) )

	ROM_REGION( 0x200000*8, REGION_GFX1, ROMREGION_ERASEFF | ROMREGION_DISPOSE )	/* 16x16x8 */
	ROMX_LOAD( "yunsun16.u67", 0x000000, 0x080000, CRC(e30fb2c4) SHA1(0d33a1593d7ebcd5da6971a04c3300c0b4eef219) , ROM_GROUPWORD | ROM_SKIP(6))
	ROMX_LOAD( "yunsun16.u68", 0x000002, 0x080000, CRC(7d702538) SHA1(ae4c8ca6f172e204589f2f70ca114f7c38e7cabd) , ROM_GROUPWORD | ROM_SKIP(6))
	ROMX_LOAD( "yunsun16.u69", 0x000004, 0x080000, CRC(97447fec) SHA1(e52184f96b2337ccbef130ada21a959c8bc1d73b) , ROM_GROUPWORD | ROM_SKIP(6))
	ROMX_LOAD( "yunsun16.u70", 0x000006, 0x080000, CRC(1b1f7895) SHA1(939c386dbef82e4833b7038e7c603d2ec67fa23e) , ROM_GROUPWORD | ROM_SKIP(6))

	ROM_REGION( 0x100000, REGION_GFX2, ROMREGION_DISPOSE )	/* 16x16x4 */
	ROM_LOAD( "yunsun16.u20", 0x000000, 0x040000, CRC(124699d0) SHA1(e55c8fb35f193abf98b1df07b94b99bf33bb5207) )
	ROM_LOAD( "yunsun16.u21", 0x040000, 0x040000, CRC(4eea29a2) SHA1(c8173eeef0228a7635a96251ae3776726ffaf0f4) )
	ROM_LOAD( "yunsun16.u22", 0x080000, 0x040000, CRC(d6db0388) SHA1(f5d8f7740b602c402a8dd6c4ebd357cf15a0dfac) )
	ROM_LOAD( "yunsun16.u23", 0x0c0000, 0x040000, CRC(1fa33b2e) SHA1(4aa0dee8d34aac19cf6b7ba3f79ca022ad8d7760) )

	ROM_REGION( 0x080000 * 2, REGION_SOUND1, 0 )	/* Samples */
	ROM_LOAD( "yunsun16.131", 0x000000, 0x080000, CRC(d0a1bb8c) SHA1(10f33521bd6031ed73ee5c7be1382165925aa8f8) )
	ROM_RELOAD(               0x080000, 0x080000             )

ROM_END


/***************************************************************************

                                Bomb Kick

YunSung YS-0211

  CPU: 68HC000 P16
Video: Actel A1020B PL84C
  OSC: 16.000000 MHz
Sound: Unmarked OKI M6295

***************************************************************************/

ROM_START( bombkick )

	ROM_REGION( 0x080000, REGION_CPU1, 0 )		/* 68000 Code */
	ROM_LOAD16_BYTE( "bk_u33",       0x000000, 0x040000, CRC(d6eb50bf) SHA1(a24c31f212f86f066c35d39da137ef0933323e43) )
	ROM_LOAD16_BYTE( "bk_u32",       0x000001, 0x040000, CRC(d55388a2) SHA1(928f1a8933b986cf099e184002660e30ee1aeb0a) )

	ROM_REGION( 0x200000*8, REGION_GFX1, ROMREGION_ERASEFF | ROMREGION_DISPOSE )	/* 16x16x8 */
	ROMX_LOAD( "bk_u67",       0x000000, 0x080000, CRC(1962f536) SHA1(36d3c73a322330058e963efcb9b81324724382cc) , ROM_GROUPWORD | ROM_SKIP(6))
	ROMX_LOAD( "bk_u68",       0x000002, 0x080000, CRC(d80c75a4) SHA1(330c20d126b9f1f61f17750028c92843be55ec78) , ROM_GROUPWORD | ROM_SKIP(6))
	ROMX_LOAD( "bk_u69",       0x000004, 0x080000, CRC(615e1e6f) SHA1(73875313010514ff5ca9e0bc96d6f93baaee391e) , ROM_GROUPWORD | ROM_SKIP(6))
	ROMX_LOAD( "bk_u70",       0x000006, 0x080000, CRC(59817ef1) SHA1(d23df30b34223575d6a9c814f2ec3db990b18679) , ROM_GROUPWORD | ROM_SKIP(6))

	ROM_REGION( 0x100000, REGION_GFX2, ROMREGION_DISPOSE )	/* 16x16x4 */
	ROM_LOAD( "bk_u20",       0x000000, 0x040000, CRC(c2b83e3f) SHA1(8bcd862dbf56cf579058d045f89f900ebfea2f1d) )
	ROM_LOAD( "bk_u21",       0x040000, 0x040000, CRC(d6890192) SHA1(3c26a08580ceecf2f61f008861a459e175c99ed9) )
	ROM_LOAD( "bk_u22",       0x080000, 0x040000, CRC(9538c46c) SHA1(d7d0e167d5abc2ee81eae6fde152b2f5cc716c0e) )
	ROM_LOAD( "bk_u23",       0x0c0000, 0x040000, CRC(e3831f3d) SHA1(096658ee5a7b83d774b671c0a38113533c8751d1) )

	ROM_REGION( 0x080000 * 2, REGION_SOUND1, 0 )	/* Samples */
	ROM_LOAD( "bk_u131",      0x000000, 0x080000, CRC(22cc5732) SHA1(38aefa4e543ea54e004eee428ee087121eb20905) )
	ROM_RELOAD(               0x080000, 0x080000             )

ROM_END



/***************************************************************************


                                Game Drivers


***************************************************************************/

GAME( 19??, magicbub, 0,        magicbub, magicbub, magicbub, ROT0, "Yun Sung", "Magic Bubble", GAME_NO_COCKTAIL )
GAME( 19??, magicbua, magicbub, magicbub, magicbua, magicbub, ROT0, "Yun Sung", "Magic Bubble (Adult version)", GAME_NO_COCKTAIL )
GAME( 1997, shocking, 0,        shocking, shocking, 0,        ROT0, "Yun Sung", "Shocking",     GAME_NO_COCKTAIL )
GAME( 1998, bombkick, 0,        shocking, bombkick, 0,        ROT0, "Yun Sung", "Bomb Kick",    GAME_NO_COCKTAIL )
