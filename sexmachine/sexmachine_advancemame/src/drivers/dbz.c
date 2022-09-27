/*
  Dragonball Z                  (c) 1993 Banpresto
  Dragonball Z 2 - Super Battle (c) 1994 Banpresto

  Driver by David Haywood, R. Belmont and Pierpaolo Prazzoli

  MC68000 + Konami Xexex-era video hardware and system controller ICs
  Z80 + YM2151 + OKIM6295 for sound

  Note: game has an extremely complete test mode, it's beautiful for emulation.
        flip the DIP and check it out!

  TODO:
    - Self Test Fails
    - Banpresto logo in DBZ has bad colors after 1 run of the attract mode because
      it's associated to the wrong logical tilemap and the same happens in DBZ2
      test mode. It should be a bug in K056832 emulation.

PCB Layout:

BP924-1  PWB250248D (note PCB is identical to DBZ2 also)
|-------------------------------------------------------|
| YM3014  Z80    32MHz  053252       222A05   222A07    |
|   YM2151 5168                      222A04   222A06    |
| 1.056kHz 5168                                         |
|  M6295   222A10                           5864        |
|   222A03     68000                 2018   5864 053246A|
|J          222A11  222A12           2018               |
|A 5864        *       *                                |
|M 5864     62256   62256                               |
|M                                               053247A|
|A                                                      |
|                                053936 053936     2018 |
|    053251  053251                                2018 |
|                                        CY7C128        |
|       054157  054156  5864     CY7C128 CY7C128 CY7C128|
|       222A01  222A02  5864     CY7C128                |
| DSW2  DSW1            5864     CY7C128 222A08  222A09 |
|                                           *       *   |
|-------------------------------------------------------|

Notes:
      68k clock: 16.000MHz
      Z80 clock: 4.000MHz
   YM2151 clock: 4.000MHz
    M6295 clock: 1.056MHz (sample rate = /132)
          Vsync: 55Hz
          Hsync: 15.36kHz
              *: unpopulated ROM positions on DBZ

*/

#include "driver.h"

#include "vidhrdw/konamiic.h"
#include "cpu/m68000/m68000.h"
#include "cpu/z80/z80.h"
#include "sound/2151intf.h"
#include "sound/okim6295.h"

/* BG LAYER */

extern UINT16 *dbz_bg1_videoram;
extern UINT16 *dbz_bg2_videoram;

WRITE16_HANDLER(dbz_bg1_videoram_w);
WRITE16_HANDLER(dbz_bg2_videoram_w);

static int dbz_control;

VIDEO_START(dbz);
VIDEO_UPDATE(dbz);

static INTERRUPT_GEN( dbz_interrupt )
{
	switch (cpu_getiloops())
	{
		case 0:
			cpunum_set_input_line(0, MC68000_IRQ_2, HOLD_LINE);
			break;

		case 1:
			if (K053246_is_IRQ_enabled())
				cpunum_set_input_line(0, MC68000_IRQ_4, HOLD_LINE);
			break;
	}
}

#if 0
static READ16_HANDLER( dbzcontrol_r )
{
	return dbz_control;
}
#endif

static WRITE16_HANDLER( dbzcontrol_w )
{
	/* bit 10 = enable '246 readback */

	COMBINE_DATA(&dbz_control);

	if (data & 0x400)
	{
		K053246_set_OBJCHA_line(ASSERT_LINE);
	}
	else
	{
		K053246_set_OBJCHA_line(CLEAR_LINE);
	}

	coin_counter_w(0, data & 1);
	coin_counter_w(1, data & 2);
}

static READ16_HANDLER( dbz_inp0_r )
{
	return readinputportbytag("IN0") | (readinputportbytag("IN1")<<8);
}

static READ16_HANDLER( dbz_inp1_r )
{
	return readinputportbytag("IN3") | (readinputportbytag("DSW1")<<8);
}

static READ16_HANDLER( dbz_inp2_r )
{
	return readinputportbytag("DSW2") | (readinputportbytag("DSW2")<<8);
}

static WRITE16_HANDLER( dbz_sound_command_w )
{
	soundlatch_w(0, data>>8);
}

static WRITE16_HANDLER( dbz_sound_cause_nmi )
{
	cpunum_set_input_line(1, INPUT_LINE_NMI, PULSE_LINE);
}

static void dbz_sound_irq(int irq)
{
	if (irq)
		cpunum_set_input_line(1, 0, ASSERT_LINE);
	else
		cpunum_set_input_line(1, 0, CLEAR_LINE);
}

static ADDRESS_MAP_START( dbz_readmem, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x0fffff) AM_READ(MRA16_ROM)
	AM_RANGE(0x480000, 0x48ffff) AM_READ(MRA16_RAM)
	AM_RANGE(0x490000, 0x491fff) AM_READ(K056832_ram_word_r)	// '157 RAM is mirrored twice
	AM_RANGE(0x492000, 0x493fff) AM_READ(K056832_ram_word_r)
	AM_RANGE(0x498000, 0x49ffff) AM_READ(K056832_rom_word_8000_r)	// code near a60 in dbz2, subroutine at 730 in dbz
	AM_RANGE(0x4a0000, 0x4a0fff) AM_READ(K053247_word_r)
	AM_RANGE(0x4a1000, 0x4a3fff) AM_READ(MRA16_RAM)
	AM_RANGE(0x4a8000, 0x4abfff) AM_READ(MRA16_RAM)			// palette
	AM_RANGE(0x4c0000, 0x4c0001) AM_READ(K053246_word_r)
	AM_RANGE(0x4e0000, 0x4e0001) AM_READ(dbz_inp0_r)
	AM_RANGE(0x4e0002, 0x4e0003) AM_READ(dbz_inp1_r)
	AM_RANGE(0x4e4000, 0x4e4001) AM_READ(dbz_inp2_r)
	AM_RANGE(0x500000, 0x501fff) AM_READ(MRA16_RAM)
	AM_RANGE(0x508000, 0x509fff) AM_READ(MRA16_RAM)
	AM_RANGE(0x510000, 0x513fff) AM_READ(MRA16_RAM)
	AM_RANGE(0x518000, 0x51bfff) AM_READ(MRA16_RAM)
	AM_RANGE(0x600000, 0x6fffff) AM_READ(MRA16_NOP) 			// PSAC 1 ROM readback window
	AM_RANGE(0x700000, 0x7fffff) AM_READ(MRA16_NOP) 			// PSAC 2 ROM readback window
ADDRESS_MAP_END

static ADDRESS_MAP_START( dbz_writemem, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x480000, 0x48ffff) AM_WRITE(MWA16_RAM)
	AM_RANGE(0x490000, 0x491fff) AM_WRITE(K056832_ram_word_w)
	AM_RANGE(0x492000, 0x493fff) AM_WRITE(K056832_ram_word_w)
	AM_RANGE(0x4a0000, 0x4a0fff) AM_WRITE(K053247_word_w)
	AM_RANGE(0x4a1000, 0x4a3fff) AM_WRITE(MWA16_RAM)
	AM_RANGE(0x4a8000, 0x4abfff) AM_WRITE(paletteram16_xRRRRRGGGGGBBBBB_word_w) AM_BASE(&paletteram16)
	AM_RANGE(0x4c0000, 0x4c0007) AM_WRITE(K053246_word_w)
	AM_RANGE(0x4c4000, 0x4c4007) AM_WRITE(K053246_word_w)
	AM_RANGE(0x4c8000, 0x4c8007) AM_WRITE(K056832_b_word_w)
	AM_RANGE(0x4cc000, 0x4cc03f) AM_WRITE(K056832_word_w)
	AM_RANGE(0x4ec000, 0x4ec001) AM_WRITE(dbzcontrol_w)
	AM_RANGE(0x4d0000, 0x4d001f) AM_WRITE(MWA16_RAM) AM_BASE(&K053936_0_ctrl)
	AM_RANGE(0x4d4000, 0x4d401f) AM_WRITE(MWA16_RAM) AM_BASE(&K053936_1_ctrl)
	AM_RANGE(0x4e8000, 0x4e8001) AM_WRITE(MWA16_NOP)
	AM_RANGE(0x4f0000, 0x4f0001) AM_WRITE(dbz_sound_command_w)
	AM_RANGE(0x4f4000, 0x4f4001) AM_WRITE(dbz_sound_cause_nmi)
	AM_RANGE(0x4f8000, 0x4f801f) AM_WRITE(MWA16_NOP)		// 251 #1
	AM_RANGE(0x4fc000, 0x4fc01f) AM_WRITE(K053251_lsb_w)	// 251 #2
	AM_RANGE(0x500000, 0x501fff) AM_WRITE(dbz_bg2_videoram_w) AM_BASE(&dbz_bg2_videoram)
	AM_RANGE(0x508000, 0x509fff) AM_WRITE(dbz_bg1_videoram_w) AM_BASE(&dbz_bg1_videoram)
	AM_RANGE(0x510000, 0x513fff) AM_WRITE(MWA16_RAM) AM_BASE(&K053936_0_linectrl) // ?? guess, it might not be
	AM_RANGE(0x518000, 0x51bfff) AM_WRITE(MWA16_RAM) AM_BASE(&K053936_1_linectrl) // ?? guess, it might not be
ADDRESS_MAP_END

/* dbz sound */
/* IRQ: from YM2151.  NMI: from 68000.  Port 0: write to ack NMI */

static ADDRESS_MAP_START( dbz_sound_readmem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x7fff) AM_READ(MRA8_ROM)
	AM_RANGE(0x8000, 0xbfff) AM_READ(MRA8_RAM)
	AM_RANGE(0xc000, 0xc001) AM_READ(YM2151_status_port_0_r)
	AM_RANGE(0xd000, 0xd002) AM_READ(OKIM6295_status_0_r)
	AM_RANGE(0xe000, 0xe001) AM_READ(soundlatch_r)
ADDRESS_MAP_END

static ADDRESS_MAP_START( dbz_sound_writemem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x7fff) AM_WRITE(MWA8_ROM)
	AM_RANGE(0x8000, 0xbfff) AM_WRITE(MWA8_RAM)
	AM_RANGE(0xc000, 0xc000) AM_WRITE(YM2151_register_port_0_w)
	AM_RANGE(0xc001, 0xc001) AM_WRITE(YM2151_data_port_0_w)
	AM_RANGE(0xd000, 0xd001) AM_WRITE(OKIM6295_data_0_w)
ADDRESS_MAP_END

static ADDRESS_MAP_START( dbz_sound_writeport, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_FLAGS( AMEF_ABITS(8) )
	AM_RANGE(0x00, 0x00) AM_WRITE(MWA8_NOP)
ADDRESS_MAP_END

/**********************************************************************************/


INPUT_PORTS_START( dbz )
	PORT_START_TAG("IN0")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_PLAYER(1)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(1)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(1)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START_TAG("IN1")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_PLAYER(2)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(2)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(2)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START_TAG("DSW1")
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Difficulty ) ) // I think this is right, but can't stomach the game long enough to check
	PORT_DIPSETTING(    0x01, DEF_STR( Easy ) )
	PORT_DIPSETTING(    0x03, DEF_STR( Normal ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Hard ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Hardest ) )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unknown ) ) // seems unused
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x04, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Flip_Screen ) ) // Definitely correct
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR(Service_Mode) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x00, DEF_STR( Language ) )
	PORT_DIPSETTING(    0x00, DEF_STR( English ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Japanese ) )
	PORT_DIPNAME( 0x80, 0x00, "Mask ROM Test" ) //NOP'd
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x80, DEF_STR( On ) )

	PORT_START_TAG("IN3")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_SERVICE ) PORT_NAME("Test Switch") PORT_CODE(KEYCODE_F2)				// "Test"
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON4 ) PORT_PLAYER(2)
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON4 ) PORT_PLAYER(1)

	PORT_START_TAG("DSW2")
	PORT_DIPNAME( 0x0f, 0x0f, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x05, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 3C_2C ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 4C_3C ) )
	PORT_DIPSETTING(    0x0f, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 3C_4C ) )
	PORT_DIPSETTING(    0x07, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(    0x0e, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x06, DEF_STR( 2C_5C ) )
	PORT_DIPSETTING(    0x0d, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x0c, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x0b, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(    0x0a, DEF_STR( 1C_6C ) )
	PORT_DIPSETTING(    0x09, DEF_STR( 1C_7C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Free_Play ) )
	PORT_DIPNAME( 0xf0, 0xf0, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x20, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x50, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x80, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x40, DEF_STR( 3C_2C ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 4C_3C ) )
	PORT_DIPSETTING(    0xf0, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x30, DEF_STR( 3C_4C ) )
	PORT_DIPSETTING(    0x70, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(    0xe0, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x60, DEF_STR( 2C_5C ) )
	PORT_DIPSETTING(    0xd0, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0xc0, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0xb0, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(    0xa0, DEF_STR( 1C_6C ) )
	PORT_DIPSETTING(    0x90, DEF_STR( 1C_7C ) )
//  PORT_DIPSETTING(    0x00, "Disabled" )
INPUT_PORTS_END

INPUT_PORTS_START( dbz2 )
	PORT_START_TAG("IN0")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_PLAYER(1)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(1)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(1)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START_TAG("IN1")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_PLAYER(2)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(2)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(2)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START_TAG("DSW1")
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Easy ) )
	PORT_DIPSETTING(    0x03, DEF_STR( Normal ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Hard ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Hardest ) )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x08, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Level_Select ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR(Service_Mode ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x00, DEF_STR( Language ) )
	PORT_DIPSETTING(    0x00, DEF_STR( English ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Japanese ) )
	PORT_DIPNAME( 0x80, 0x00, "Mask ROM Test" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x80, DEF_STR( On ) )

	PORT_START_TAG("IN3")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_SERVICE ) PORT_NAME("Test Switch") PORT_CODE(KEYCODE_F2)		// "Test"
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON4 ) PORT_PLAYER(2)
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON4 ) PORT_PLAYER(1)

	PORT_START_TAG("DSW2")
	PORT_DIPNAME( 0x0f, 0x0f, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x05, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 3C_2C ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 4C_3C ) )
	PORT_DIPSETTING(    0x0f, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 3C_4C ) )
	PORT_DIPSETTING(    0x07, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(    0x0e, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x06, DEF_STR( 2C_5C ) )
	PORT_DIPSETTING(    0x0d, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x0c, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x0b, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(    0x0a, DEF_STR( 1C_6C ) )
	PORT_DIPSETTING(    0x09, DEF_STR( 1C_7C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Free_Play ) )
	PORT_DIPNAME( 0xf0, 0xf0, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x20, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x50, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x80, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x40, DEF_STR( 3C_2C ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 4C_3C ) )
	PORT_DIPSETTING(    0xf0, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x30, DEF_STR( 3C_4C ) )
	PORT_DIPSETTING(    0x70, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(    0xe0, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x60, DEF_STR( 2C_5C ) )
	PORT_DIPSETTING(    0xd0, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0xc0, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0xb0, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(    0xa0, DEF_STR( 1C_6C ) )
	PORT_DIPSETTING(    0x90, DEF_STR( 1C_7C ) )
//  PORT_DIPSETTING(    0x00, "Disabled" )
INPUT_PORTS_END

/**********************************************************************************/

static struct YM2151interface ym2151_interface =
{
	dbz_sound_irq
};

/**********************************************************************************/

static const gfx_layout bglayout =
{
	16,16,
	RGN_FRAC(1,1),
	4,
	{ 0, 1, 2, 3 },
	{ 0*4, 1*4, 2*4, 3*4, 4*4, 5*4, 6*4, 7*4, 8*4,
	  9*4, 10*4, 11*4, 12*4, 13*4, 14*4, 15*4 },
	{ 0*64, 1*64, 2*64, 3*64, 4*64, 5*64, 6*64, 7*64,
			8*64, 9*64, 10*64, 11*64, 12*64, 13*64, 14*64, 15*64 },
	128*8
};

static const gfx_decode gfxdecodeinfo[] =
{
	{ REGION_GFX3, 0, &bglayout, 0, 512 },
	{ REGION_GFX4, 0, &bglayout, 0, 512 },
	{ -1 } /* end of array */
};

/**********************************************************************************/

static MACHINE_DRIVER_START( dbz )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000, 16000000)
	MDRV_CPU_PROGRAM_MAP(dbz_readmem,dbz_writemem)
	MDRV_CPU_VBLANK_INT(dbz_interrupt,2)

	MDRV_CPU_ADD(Z80, 4000000)
	/* audio CPU */
	MDRV_CPU_PROGRAM_MAP(dbz_sound_readmem, dbz_sound_writemem)
	MDRV_CPU_IO_MAP(0,dbz_sound_writeport)

	MDRV_FRAMES_PER_SECOND(55)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	MDRV_GFXDECODE(gfxdecodeinfo)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER | VIDEO_NEEDS_6BITS_PER_GUN | VIDEO_HAS_SHADOWS)
	MDRV_SCREEN_SIZE(64*8, 32*8)
	MDRV_VISIBLE_AREA(0, 48*8-1, 0, 32*8-1)
	MDRV_VIDEO_START(dbz)
	MDRV_VIDEO_UPDATE(dbz)
	MDRV_PALETTE_LENGTH(0x4000/2)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_STEREO("left", "right")

	MDRV_SOUND_ADD(YM2151, 4000000)
	MDRV_SOUND_CONFIG(ym2151_interface)
	MDRV_SOUND_ROUTE(0, "left", 1.0)
	MDRV_SOUND_ROUTE(1, "right", 1.0)

	MDRV_SOUND_ADD(OKIM6295, 1056000/132)
	MDRV_SOUND_CONFIG(okim6295_interface_region_1)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "left", 1.0)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "right", 1.0)
MACHINE_DRIVER_END

/**********************************************************************************/

#define ROM_LOAD64_WORD(name,offset,length,crc)		ROMX_LOAD(name, offset, length, crc, ROM_GROUPWORD | ROM_SKIP(6))

ROM_START( dbz )
	/* main program */
	ROM_REGION( 0x400000, REGION_CPU1, 0)
	ROM_LOAD16_BYTE( "222a11.9e", 0x000000, 0x80000, CRC(60c7d9b2) SHA1(718ef89e89b3943845e91bedfc5c1d26229f9fe5) )
	ROM_LOAD16_BYTE( "222a12.9f", 0x000001, 0x80000, CRC(6ebc6853) SHA1(e9b2068246228968cc6b8554215563cacaa5ba9f) )

	ROM_REGION( 0x400000, REGION_USER1, 0)
	ROM_LOAD16_BYTE( "222a11.9e", 0x000000, 0x80000, CRC(60c7d9b2) SHA1(718ef89e89b3943845e91bedfc5c1d26229f9fe5) )
	ROM_LOAD16_BYTE( "222a12.9f", 0x000001, 0x80000, CRC(6ebc6853) SHA1(e9b2068246228968cc6b8554215563cacaa5ba9f) )

	/* sound program */
	ROM_REGION( 0x010000, REGION_CPU2, 0 )
	ROM_LOAD("222a10.5e", 0x000000, 0x08000, CRC(1c93e30a) SHA1(8545a0ac5126b3c855e1901b186f57820699895d) )

	/* tiles */
	ROM_REGION( 0x400000, REGION_GFX1, 0)
	ROM_LOAD( "222a01.27c", 0x000000, 0x200000, CRC(9fce4ed4) SHA1(81e19375b351ee247f066434dd595149333d73c5) )
	ROM_LOAD( "222a02.27e", 0x200000, 0x200000, CRC(651acaa5) SHA1(33942a90fb294b5da6a48e5bfb741b31babca188) )

	/* sprites */
	ROM_REGION( 0x800000, REGION_GFX2, 0)
	ROM_LOAD64_WORD( "222a04.3j", 0x000000, 0x200000, CRC(2533b95a) SHA1(35910836b6030130d742eae6c4bf1cdf1ff43fa4) )
	ROM_LOAD64_WORD( "222a05.1j", 0x000002, 0x200000, CRC(731b7f93) SHA1(b676fff2ede5aa72c49fe12736cd60766462fe0b) )
	ROM_LOAD64_WORD( "222a06.3l", 0x000004, 0x200000, CRC(97b767d3) SHA1(3d879c431586da2f88c632ab1a531b4a5ec96939) )
	ROM_LOAD64_WORD( "222a07.1l", 0x000006, 0x200000, CRC(430bc873) SHA1(ea483195bb7f20ef3df7cfba153e5f6f8d53e5f9) )

	/* K053536 PSAC-2 #1 */
	ROM_REGION( 0x200000, REGION_GFX3, 0)
	ROM_LOAD( "222a08.25k", 0x000000, 0x200000, CRC(6410ee1b) SHA1(2296aafd3ba25f63a12130f7b58de53e88f14e92) )

	/* K053536 PSAC-2 #2 */
	ROM_REGION( 0x200000, REGION_GFX4, 0)
	ROM_LOAD( "222a09.25l", 0x000000, 0x200000, CRC(f7b3f070) SHA1(50ebd8cfcda292a3df5664de50f9212108d58923) )

	/* sound data */
	ROM_REGION( 0x40000, REGION_SOUND1, 0)
	ROM_LOAD( "222a03.7c", 0x000000, 0x40000, CRC(1924467b) SHA1(57922090509bcc63b4783e8f2c5e95afd2090b87) )
ROM_END

ROM_START( dbz2 )
	/* main program */
	ROM_REGION( 0x400000, REGION_CPU1, 0)
	ROM_LOAD16_BYTE( "a9e.9e", 0x000000, 0x80000, CRC(e6a142c9) SHA1(7951c8f7036a67a0cd3260f434654820bf3e603f) )
	ROM_LOAD16_BYTE( "a9f.9f", 0x000001, 0x80000, CRC(76cac399) SHA1(af6daa1f8b87c861dc62adef5ca029190c3cb9ae) )

	/* sound program */
	ROM_REGION( 0x010000, REGION_CPU2, 0 )
	ROM_LOAD("s-001.5e", 0x000000, 0x08000, CRC(154e6d03) SHA1(db15c20982692271f40a733dfc3f2486221cd604) )

	/* tiles */
	ROM_REGION( 0x400000, REGION_GFX1, 0)
	ROM_LOAD( "ds-b01.27c", 0x000000, 0x200000, CRC(8dc39972) SHA1(c6e3d4e0ff069e08bdb68e2b0ad24cc7314e4e93) )
	ROM_LOAD( "ds-b02.27e", 0x200000, 0x200000, CRC(7552f8cd) SHA1(1f3beffe9733b1a18d44b5e8880ff1cc97e7a8ab) )

	/* sprites */
	ROM_REGION( 0x800000, REGION_GFX2, 0)
	ROM_LOAD64_WORD( "ds-o01.3j", 0x000000, 0x200000, CRC(d018531f) SHA1(d4082fe28e9f1f3f35aa75b4be650cadf1cef192) )
	ROM_LOAD64_WORD( "ds-o02.1j", 0x000002, 0x200000, CRC(5a0f1ebe) SHA1(3bb9e1389299dc046a24740ef1a1c543e44b5c37) )
	ROM_LOAD64_WORD( "ds-o03.3l", 0x000004, 0x200000, CRC(ddc3bef1) SHA1(69638ef53f627a238a12b6c206d57faadf894893) )
	ROM_LOAD64_WORD( "ds-o04.1l", 0x000006, 0x200000, CRC(b5df6676) SHA1(194cfce460ccd29e2cceec577aae4ec936ae88e5) )

	/* K053536 PSAC-2 #1 */
	ROM_REGION( 0x400000, REGION_GFX3, 0)
	ROM_LOAD( "ds-p01.25k", 0x000000, 0x200000, CRC(1c7aad68) SHA1(a5296cf12cec262eede55397ea929965576fea81) )
	ROM_LOAD( "ds-p02.27k", 0x200000, 0x200000, CRC(e4c3a43b) SHA1(f327f75fe82f8aafd2cfe6bdd3a426418615974b) )

	/* K053536 PSAC-2 #2 */
	ROM_REGION( 0x400000, REGION_GFX4, 0)
	ROM_LOAD( "ds-p03.25l", 0x000000, 0x200000, CRC(1eaa671b) SHA1(1875eefc6f2c3fc8feada56bfa6701144e8ef64b) )
	ROM_LOAD( "ds-p04.27l", 0x200000, 0x200000, CRC(5845ff98) SHA1(73b4c3f439321ce9c462119fe933e7cbda8cd498) )

	/* sound data */
	ROM_REGION( 0x40000, REGION_SOUND1, 0)
	ROM_LOAD( "pcm.7c", 0x000000, 0x40000, CRC(b58c884a) SHA1(0e2a7267e9dff29c9af25558081ec9d56629bc43) )
ROM_END

static DRIVER_INIT( dbz )
{
	UINT16 *ROM;

	konami_rom_deinterleave_2(REGION_GFX1);

	ROM = (UINT16 *)memory_region(REGION_CPU1);

	// nop out dbz1's mask rom test
	// tile ROM test
	ROM[0x790/2] = 0x4e71;
	ROM[0x792/2] = 0x4e71;
	// PSAC2 ROM test
	ROM[0x982/2] = 0x4e71;
	ROM[0x984/2] = 0x4e71;
	ROM[0x986/2] = 0x4e71;
	ROM[0x988/2] = 0x4e71;
	ROM[0x98a/2] = 0x4e71;
	ROM[0x98c/2] = 0x4e71;
	ROM[0x98e/2] = 0x4e71;
	ROM[0x990/2] = 0x4e71;
}

static DRIVER_INIT( dbz2 )
{
	konami_rom_deinterleave_2(REGION_GFX1);
}

GAME( 1993, dbz,  0, dbz, dbz,  dbz,  ROT0, "Banpresto", "Dragonball Z", GAME_IMPERFECT_GRAPHICS )
GAME( 1994, dbz2, 0, dbz, dbz2, dbz2, ROT0, "Banpresto", "Dragonball Z 2 - Super Battle", GAME_IMPERFECT_GRAPHICS )
