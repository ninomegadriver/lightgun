/***************************************************************************

 Lasso and similar hardware

        driver by Phil Stroffolino, Nicola Salmoria, Luca Elia

---------------------------------------------------------------------------
Year + Game                 By              CPUs        Sound Chips
---------------------------------------------------------------------------
82  Lasso                   SNK             3 x 6502    2 x SN76489
83  Chameleon               Jaleco          2 x 6502    2 x SN76489
84  Wai Wai Jockey Gate-In! Jaleco/Casio    2 x 6502    2 x SN76489 + DAC
84  Pinbo                   Jaleco          6502 + Z80  2 x AY-8910
---------------------------------------------------------------------------

Notes:

- unknown CPU speeds (affect game timing)
- Lasso: fire button auto-repeats on high score entry screen (real behavior?)

***************************************************************************/

#include "driver.h"
#include "cpu/z80/z80.h"
#include "lasso.h"
#include "sound/dac.h"
#include "sound/sn76496.h"
#include "sound/ay8910.h"


/* IRQ = VBlank, NMI = Coin Insertion */

static INTERRUPT_GEN( lasso_interrupt )
{
	static int old;
	int new;

	// VBlank
	if (cpu_getiloops() == 0)
	{
		cpunum_set_input_line(0, 0, HOLD_LINE);
		return;
	}

	// Coins
	new = ~readinputport(3) & 0x30;

	if ( ((new & 0x10) && !(old & 0x10)) ||
		 ((new & 0x20) && !(old & 0x20)) )
		cpunum_set_input_line(0, INPUT_LINE_NMI, PULSE_LINE);

	old = new;
}


/* Shared RAM between Main CPU and sub CPU */

static UINT8 *lasso_sharedram;

static READ8_HANDLER( lasso_sharedram_r )
{
	return lasso_sharedram[offset];
}
static WRITE8_HANDLER( lasso_sharedram_w )
{
	lasso_sharedram[offset] = data;
}


/* Write to the sound latch and generate an IRQ on the sound CPU */

static WRITE8_HANDLER( sound_command_w )
{
	soundlatch_w(offset,data);
	cpunum_set_input_line( 1, 0, PULSE_LINE );
}

static READ8_HANDLER( sound_status_r )
{
	/*  0x01: chip#0 ready; 0x02: chip#1 ready */
	return 0x03;
}

static UINT8 lasso_chip_data;

static WRITE8_HANDLER( sound_data_w )
{
	lasso_chip_data = BITSWAP8(data,0,1,2,3,4,5,6,7);
}

static WRITE8_HANDLER( sound_select_w )
{
	if (~data & 0x01)	/* chip #0 */
		SN76496_0_w(0,lasso_chip_data);

	if (~data & 0x02)	/* chip #1 */
		SN76496_1_w(0,lasso_chip_data);
}



static ADDRESS_MAP_START( lasso_readmem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x0c7f) AM_READ(MRA8_RAM)
	AM_RANGE(0x1000, 0x17ff) AM_READ(lasso_sharedram_r	)
	AM_RANGE(0x1804, 0x1804) AM_READ(input_port_0_r)
	AM_RANGE(0x1805, 0x1805) AM_READ(input_port_1_r)
	AM_RANGE(0x1806, 0x1806) AM_READ(input_port_2_r)
	AM_RANGE(0x1807, 0x1807) AM_READ(input_port_3_r)
	AM_RANGE(0x8000, 0xffff) AM_READ(MRA8_ROM)
ADDRESS_MAP_END

static ADDRESS_MAP_START( lasso_writemem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x03ff) AM_WRITE(MWA8_RAM)
	AM_RANGE(0x0400, 0x07ff) AM_WRITE(lasso_videoram_w) AM_BASE(&lasso_videoram)
	AM_RANGE(0x0800, 0x0bff) AM_WRITE(lasso_colorram_w) AM_BASE(&lasso_colorram)
	AM_RANGE(0x0c00, 0x0c7f) AM_WRITE(MWA8_RAM) AM_BASE(&lasso_spriteram) AM_SIZE(&lasso_spriteram_size)
	AM_RANGE(0x1000, 0x17ff) AM_WRITE(lasso_sharedram_w	)
	AM_RANGE(0x1800, 0x1800) AM_WRITE(sound_command_w)
	AM_RANGE(0x1801, 0x1801) AM_WRITE(lasso_backcolor_w	)
	AM_RANGE(0x1802, 0x1802) AM_WRITE(lasso_video_control_w)
	AM_RANGE(0x1806, 0x1806) AM_WRITE(MWA8_NOP)	// games uses 'lsr' to read port
	AM_RANGE(0x8000, 0xffff) AM_WRITE(MWA8_ROM)
ADDRESS_MAP_END


static ADDRESS_MAP_START( chameleo_readmem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x10ff) AM_READ(MRA8_RAM)
	AM_RANGE(0x1804, 0x1804) AM_READ(input_port_0_r)
	AM_RANGE(0x1805, 0x1805) AM_READ(input_port_1_r)
	AM_RANGE(0x1806, 0x1806) AM_READ(input_port_2_r)
	AM_RANGE(0x1807, 0x1807) AM_READ(input_port_3_r)
	AM_RANGE(0x2000, 0xffff) AM_READ(MRA8_ROM)
ADDRESS_MAP_END

static ADDRESS_MAP_START( chameleo_writemem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x03ff) AM_WRITE(MWA8_RAM)
	AM_RANGE(0x0400, 0x07ff) AM_WRITE(lasso_videoram_w) AM_BASE(&lasso_videoram)
	AM_RANGE(0x0800, 0x0bff) AM_WRITE(lasso_colorram_w) AM_BASE(&lasso_colorram)
	AM_RANGE(0x0c00, 0x0fff) AM_WRITE(MWA8_RAM)	//
	AM_RANGE(0x1000, 0x107f) AM_WRITE(MWA8_RAM) AM_BASE(&lasso_spriteram) AM_SIZE(&lasso_spriteram_size)
	AM_RANGE(0x1080, 0x10ff) AM_WRITE(MWA8_RAM)
	AM_RANGE(0x1800, 0x1800) AM_WRITE(sound_command_w)
	AM_RANGE(0x1801, 0x1801) AM_WRITE(lasso_backcolor_w	)
	AM_RANGE(0x1802, 0x1802) AM_WRITE(lasso_video_control_w)
	AM_RANGE(0x2000, 0xffff) AM_WRITE(MWA8_ROM)
ADDRESS_MAP_END


static ADDRESS_MAP_START( wwjgtin_readmem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x10ff) AM_READ(MRA8_RAM)
	AM_RANGE(0x1804, 0x1804) AM_READ(input_port_0_r)
	AM_RANGE(0x1805, 0x1805) AM_READ(input_port_1_r)
	AM_RANGE(0x1806, 0x1806) AM_READ(input_port_2_r)
	AM_RANGE(0x1807, 0x1807) AM_READ(input_port_3_r)
	AM_RANGE(0x5000, 0xbfff) AM_READ(MRA8_ROM)
	AM_RANGE(0xfffa, 0xffff) AM_READ(MRA8_ROM)
ADDRESS_MAP_END

static ADDRESS_MAP_START( wwjgtin_writemem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x07ff) AM_WRITE(MWA8_RAM)
	AM_RANGE(0x0800, 0x0bff) AM_WRITE(lasso_videoram_w) AM_BASE(&lasso_videoram)
	AM_RANGE(0x0c00, 0x0fff) AM_WRITE(lasso_colorram_w) AM_BASE(&lasso_colorram)
	AM_RANGE(0x1000, 0x10ff) AM_WRITE(MWA8_RAM) AM_BASE(&lasso_spriteram) AM_SIZE(&lasso_spriteram_size)
	AM_RANGE(0x1800, 0x1800) AM_WRITE(sound_command_w)
	AM_RANGE(0x1801, 0x1801) AM_WRITE(lasso_backcolor_w	)
	AM_RANGE(0x1802, 0x1802) AM_WRITE(wwjgtin_video_control_w	)
	AM_RANGE(0x1c00, 0x1c03) AM_WRITE(wwjgtin_lastcolor_w)
	AM_RANGE(0x1c04, 0x1c07) AM_WRITE(MWA8_RAM) AM_BASE(&wwjgtin_track_scroll)
	AM_RANGE(0x5000, 0xbfff) AM_WRITE(MWA8_ROM)
	AM_RANGE(0xfffa, 0xffff) AM_WRITE(MWA8_ROM)
ADDRESS_MAP_END


static ADDRESS_MAP_START( pinbo_writemem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x03ff) AM_WRITE(MWA8_RAM)
	AM_RANGE(0x0400, 0x07ff) AM_WRITE(lasso_videoram_w) AM_BASE(&lasso_videoram)
	AM_RANGE(0x0800, 0x0bff) AM_WRITE(lasso_colorram_w) AM_BASE(&lasso_colorram)
	AM_RANGE(0x1000, 0x10ff) AM_WRITE(MWA8_RAM) AM_BASE(&lasso_spriteram) AM_SIZE(&lasso_spriteram_size)
	AM_RANGE(0x1800, 0x1800) AM_WRITE(sound_command_w)
	AM_RANGE(0x1802, 0x1802) AM_WRITE(pinbo_video_control_w)
	AM_RANGE(0x2000, 0xffff) AM_WRITE(MWA8_ROM)
ADDRESS_MAP_END


static ADDRESS_MAP_START( lasso_coprocessor_readmem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x07ff) AM_READ(MRA8_RAM)
	AM_RANGE(0x2000, 0x3fff) AM_READ(MRA8_RAM)
	AM_RANGE(0x8000, 0x8fff) AM_READ(MRA8_ROM)
	AM_RANGE(0xf000, 0xffff) AM_READ(MRA8_ROM)
ADDRESS_MAP_END

static ADDRESS_MAP_START( lasso_coprocessor_writemem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x07ff) AM_WRITE(MWA8_RAM) AM_BASE(&lasso_sharedram)
	AM_RANGE(0x2000, 0x3fff) AM_WRITE(MWA8_RAM) AM_BASE(&lasso_bitmap_ram)
	AM_RANGE(0x8000, 0x8fff) AM_WRITE(MWA8_ROM)
	AM_RANGE(0xf000, 0xffff) AM_WRITE(MWA8_ROM)
ADDRESS_MAP_END


static ADDRESS_MAP_START( lasso_sound_readmem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x01ff) AM_READ(MRA8_RAM)
	AM_RANGE(0x5000, 0x7fff) AM_READ(MRA8_ROM)
	AM_RANGE(0xb004, 0xb004) AM_READ(sound_status_r)
	AM_RANGE(0xb005, 0xb005) AM_READ(soundlatch_r)
	AM_RANGE(0xf000, 0xffff) AM_READ(MRA8_ROM)
ADDRESS_MAP_END

static ADDRESS_MAP_START( lasso_sound_writemem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x01ff) AM_WRITE(MWA8_RAM)
	AM_RANGE(0x5000, 0x7fff) AM_WRITE(MWA8_ROM)
	AM_RANGE(0xb000, 0xb000) AM_WRITE(sound_data_w)
	AM_RANGE(0xb001, 0xb001) AM_WRITE(sound_select_w)
	AM_RANGE(0xf000, 0xffff) AM_WRITE(MWA8_ROM)
ADDRESS_MAP_END


static ADDRESS_MAP_START( chameleo_sound_readmem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x01ff) AM_READ(MRA8_RAM)
	AM_RANGE(0x1000, 0x1fff) AM_READ(MRA8_ROM)
	AM_RANGE(0x6000, 0x7fff) AM_READ(MRA8_ROM)
	AM_RANGE(0xb004, 0xb004) AM_READ(sound_status_r)
	AM_RANGE(0xb005, 0xb005) AM_READ(soundlatch_r)
	AM_RANGE(0xfffa, 0xffff) AM_READ(MRA8_ROM)
ADDRESS_MAP_END

static ADDRESS_MAP_START( chameleo_sound_writemem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x01ff) AM_WRITE(MWA8_RAM)
	AM_RANGE(0x1000, 0x1fff) AM_WRITE(MWA8_ROM)
	AM_RANGE(0x6000, 0x7fff) AM_WRITE(MWA8_ROM)
	AM_RANGE(0xb000, 0xb000) AM_WRITE(sound_data_w)
	AM_RANGE(0xb001, 0xb001) AM_WRITE(sound_select_w)
	AM_RANGE(0xfffa, 0xffff) AM_WRITE(MWA8_ROM)
ADDRESS_MAP_END


static ADDRESS_MAP_START( wwjgtin_sound_writemem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x01ff) AM_WRITE(MWA8_RAM)
	AM_RANGE(0x5000, 0x7fff) AM_WRITE(MWA8_ROM)
	AM_RANGE(0xb000, 0xb000) AM_WRITE(sound_data_w)
	AM_RANGE(0xb001, 0xb001) AM_WRITE(sound_select_w)
	AM_RANGE(0xb003, 0xb003) AM_WRITE(DAC_0_data_w)
	AM_RANGE(0xfffa, 0xffff) AM_WRITE(MWA8_ROM)
ADDRESS_MAP_END


static ADDRESS_MAP_START( pinbo_sound_readmem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x1fff) AM_READ(MRA8_ROM)
	AM_RANGE(0xf000, 0xffff) AM_READ(MRA8_RAM)
ADDRESS_MAP_END

static ADDRESS_MAP_START( pinbo_sound_writemem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x1fff) AM_WRITE(MWA8_ROM)
	AM_RANGE(0xf000, 0xffff) AM_WRITE(MWA8_RAM)
ADDRESS_MAP_END

static ADDRESS_MAP_START( pinbo_sound_readport, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_FLAGS( AMEF_ABITS(8) )
	AM_RANGE(0x02, 0x02) AM_READ(AY8910_read_port_0_r)
	AM_RANGE(0x06, 0x06) AM_READ(AY8910_read_port_1_r)
	AM_RANGE(0x08, 0x08) AM_READ(soundlatch_r)
ADDRESS_MAP_END

static ADDRESS_MAP_START( pinbo_sound_writeport, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_FLAGS( AMEF_ABITS(8) )
	AM_RANGE(0x00, 0x00) AM_WRITE(AY8910_control_port_0_w)
	AM_RANGE(0x01, 0x01) AM_WRITE(AY8910_write_port_0_w)
	AM_RANGE(0x04, 0x04) AM_WRITE(AY8910_control_port_1_w)
	AM_RANGE(0x05, 0x05) AM_WRITE(AY8910_write_port_1_w)
	AM_RANGE(0x08, 0x08) AM_WRITE(MWA8_NOP)	/* ??? */
	AM_RANGE(0x14, 0x14) AM_WRITE(MWA8_NOP)	/* ??? */
ADDRESS_MAP_END



INPUT_PORTS_START( lasso )
	PORT_START_TAG("1804")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT ) PORT_4WAY
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT ) PORT_4WAY
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP ) PORT_4WAY
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN ) PORT_4WAY
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 ) /* lasso */
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_BUTTON2 ) /* shoot */
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNUSED  )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNUSED  )

	PORT_START_TAG("1805")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT ) PORT_4WAY PORT_COCKTAIL
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT ) PORT_4WAY PORT_COCKTAIL
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP ) PORT_4WAY PORT_COCKTAIL
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN ) PORT_4WAY PORT_COCKTAIL
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 ) PORT_COCKTAIL
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_BUTTON2	 ) PORT_COCKTAIL
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNUSED  )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNUSED  )

	PORT_START_TAG("1806")
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(	0x01, DEF_STR( Upright ) )
	PORT_DIPSETTING(	0x00, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0x0e, 0x0e, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(	0x02, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(	0x0e, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(	0x08, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(	0x04, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(	0x0c, DEF_STR( 1C_6C ) )
//  PORT_DIPSETTING(    0x06, DEF_STR( 1C_1C ) )
//  PORT_DIPSETTING(    0x00, DEF_STR( 1C_1C ) )
//  PORT_DIPSETTING(    0x0a, DEF_STR( 1C_1C ) )
	PORT_DIPNAME( 0x30, 0x30, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x30, "3" )
	PORT_DIPSETTING(    0x10, "4" )
	PORT_DIPSETTING(    0x20, "5" )
//  PORT_DIPSETTING(    0x00, "3" )
	PORT_DIPNAME( 0x40, 0x00, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(	0x40, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(	0x00, DEF_STR( 1C_1C ) )
	PORT_DIPNAME( 0x80, 0x80, "Warm-Up Instructions" )
	PORT_DIPSETTING(	0x00, DEF_STR( No ) )
	PORT_DIPSETTING(	0x80, DEF_STR( Yes ) )

	PORT_START_TAG("1807")
	PORT_DIPNAME( 0x01, 0x00, "Warm-Up" )
	PORT_DIPSETTING(    0x01, DEF_STR( No ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x02, 0x00, "Warm-Up Language" )
	PORT_DIPSETTING(    0x00, DEF_STR( English ) )
	PORT_DIPSETTING(    0x02, DEF_STR( German ) )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unknown ) )	/* used */
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x00, "Invulnerability (Cheat)")
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x08, DEF_STR( On ) )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_COIN2    )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_COIN1    )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_START2  )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_START1  )
INPUT_PORTS_END

INPUT_PORTS_START( chameleo )
	PORT_START_TAG("1804")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT ) PORT_4WAY
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT ) PORT_4WAY
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP ) PORT_4WAY
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN ) PORT_4WAY
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNUSED )

	PORT_START_TAG("1805")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT ) PORT_4WAY PORT_COCKTAIL
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT ) PORT_4WAY PORT_COCKTAIL
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP ) PORT_4WAY PORT_COCKTAIL
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN ) PORT_4WAY PORT_COCKTAIL
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 ) PORT_COCKTAIL
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNUSED )

	PORT_START_TAG("1806")
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(	0x01, DEF_STR( Upright ) )
	PORT_DIPSETTING(	0x00, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0x0e, 0x0e, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(	0x02, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(	0x0e, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(	0x08, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(	0x04, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(	0x0c, DEF_STR( 1C_6C ) )
//  PORT_DIPSETTING(    0x06, DEF_STR( 1C_1C ) )
//  PORT_DIPSETTING(    0x00, DEF_STR( 1C_1C ) )
//  PORT_DIPSETTING(    0x0a, DEF_STR( 1C_1C ) )
	PORT_DIPNAME( 0x30, 0x00, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x00, "3" )
	PORT_DIPSETTING(    0x30, "5" )
//  PORT_DIPSETTING(    0x10, "5" )
	PORT_DIPSETTING(    0x20, "Infinite (Cheat)")
	PORT_DIPNAME( 0x40, 0x00, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(	0x40, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(	0x00, DEF_STR( 1C_1C ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START_TAG("1807")
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Unknown ) )	/* probably unused */
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Unknown ) )	/* probably unused */
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unknown ) )	/* probably unused */
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x08, DEF_STR( On ) )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_COIN2    )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_COIN1    )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_START2  )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_START1  )
INPUT_PORTS_END

INPUT_PORTS_START( wwjgtin )
	PORT_START_TAG("1804")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT ) PORT_4WAY
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT ) PORT_4WAY
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP ) PORT_4WAY
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN ) PORT_4WAY
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_BUTTON2 )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNUSED )

	PORT_START_TAG("1805")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT ) PORT_4WAY PORT_PLAYER(2)
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT ) PORT_4WAY PORT_PLAYER(2)
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP ) PORT_4WAY PORT_PLAYER(2)
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN ) PORT_4WAY PORT_PLAYER(2)
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 ) PORT_PLAYER(2)
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_BUTTON2 ) PORT_PLAYER(2)
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNUSED )

	PORT_START_TAG("1806")
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Unknown ) )	/* used - has to do with the controls */
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x0e, 0x0e, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(	0x02, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(	0x0e, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(	0x08, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(	0x04, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(	0x0c, DEF_STR( 1C_6C ) )
//  PORT_DIPSETTING(    0x06, DEF_STR( 1C_1C ) )
//  PORT_DIPSETTING(    0x00, DEF_STR( 1C_1C ) )
//  PORT_DIPSETTING(    0x0a, DEF_STR( 1C_1C ) )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unknown ) )	/* probably unused */
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unknown ) )	/* probably unused */
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x00, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(	0x40, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(	0x00, DEF_STR( 1C_1C ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )	/* probably unused */
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START_TAG("1807")
	PORT_DIPNAME( 0x01, 0x00, DEF_STR( Bonus_Life) )
	PORT_DIPSETTING(    0x00, "20k" )
	PORT_DIPSETTING(    0x01, "50k" )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x02, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unknown ) )	/* probably unused */
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unknown ) )	/* probably unused */
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_BIT( 0x10, IP_ACTIVE_LOW,  IPT_COIN2   )
	PORT_BIT( 0x20, IP_ACTIVE_LOW,  IPT_COIN1   )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_START1  )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_START2  )
INPUT_PORTS_END

INPUT_PORTS_START( pinbo )
	PORT_START_TAG("1804")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT ) PORT_8WAY
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT ) PORT_8WAY
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP ) PORT_8WAY
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN ) PORT_8WAY
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON2 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_BUTTON1 )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START_TAG("1805")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT ) PORT_8WAY PORT_COCKTAIL
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT ) PORT_8WAY PORT_COCKTAIL
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP ) PORT_8WAY PORT_COCKTAIL
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN ) PORT_8WAY PORT_COCKTAIL
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON2 ) PORT_COCKTAIL
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_BUTTON1 ) PORT_COCKTAIL
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START_TAG("1806")
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0x0e, 0x0e, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(	0x02, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(	0x0e, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(	0x08, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(	0x04, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(	0x0c, DEF_STR( 1C_6C ) )
//  PORT_DIPSETTING(    0x06, DEF_STR( 1C_1C ) )
//  PORT_DIPSETTING(    0x00, DEF_STR( 1C_1C ) )
//  PORT_DIPSETTING(    0x0a, DEF_STR( 1C_1C ) )
	PORT_DIPNAME( 0x30, 0x00, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x00, "3" )
	PORT_DIPSETTING(    0x10, "4" )
	PORT_DIPSETTING(    0x20, "5" )
	PORT_DIPSETTING(    0x30, "70 (Cheat)")
	PORT_DIPNAME( 0x40, 0x00, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x40, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_1C ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )	/* probably unused */
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START_TAG("1807")
	PORT_DIPNAME( 0x01, 0x00, DEF_STR( Bonus_Life) )
	PORT_DIPSETTING(    0x00, "500000,1000000" )
	PORT_DIPSETTING(    0x01, DEF_STR( None ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Controls ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Normal ) )
	PORT_DIPSETTING(    0x00, "Reversed" )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unknown ) )	/* probably unused */
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x08, DEF_STR( On ) )
	PORT_BIT( 0x10, IP_ACTIVE_LOW,  IPT_COIN2   )
	PORT_BIT( 0x20, IP_ACTIVE_LOW,  IPT_COIN1   )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_START1 )
INPUT_PORTS_END


static const gfx_layout lasso_charlayout =
{
	8,8,
	RGN_FRAC(1,4),
	2,
	{ RGN_FRAC(0,4), RGN_FRAC(2,4) },
	{ STEP8(0,1) },
	{ STEP8(0,8) },
	8*8
};

static const gfx_layout lasso_spritelayout =
{
	16,16,
	RGN_FRAC(1,4),
	2,
	{ RGN_FRAC(1,4), RGN_FRAC(3,4) },
	{ STEP8(0,1), STEP8(8*8*1,1) },
	{ STEP8(0,8), STEP8(8*8*2,8) },
	16*16
};

static const gfx_layout wwjgtin_tracklayout =
{
	16,16,
	RGN_FRAC(1,4),
	4,
	{ RGN_FRAC(1,4), RGN_FRAC(3,4), RGN_FRAC(0,4), RGN_FRAC(2,4) },
	{ STEP8(0,1), STEP8(8*8*1,1) },
	{ STEP8(0,8), STEP8(8*8*2,8) },
	16*16
};

/* Pinbo is 3bpp, otherwise the same */
static const gfx_layout pinbo_charlayout =
{
	8,8,
	RGN_FRAC(1,6),
	3,
	{ RGN_FRAC(0,6), RGN_FRAC(2,6), RGN_FRAC(4,6) },
	{ STEP8(0,1) },
	{ STEP8(0,8) },
	8*8
};

static const gfx_layout pinbo_spritelayout =
{
	16,16,
	RGN_FRAC(1,6),
	3,
	{ RGN_FRAC(1,6), RGN_FRAC(3,6), RGN_FRAC(5,6) },
	{ STEP8(0,1), STEP8(8*8*1,1) },
	{ STEP8(0,8), STEP8(8*8*2,8) },
	16*16
};


static const gfx_decode lasso_gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &lasso_charlayout,   0, 16 },
	{ REGION_GFX1, 0, &lasso_spritelayout, 0, 16 },
	{ -1 }
};

static const gfx_decode wwjgtin_gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &lasso_charlayout,       0, 16 },
	{ REGION_GFX1, 0, &lasso_spritelayout,     0, 16 },
	{ REGION_GFX2, 0, &wwjgtin_tracklayout,	4*16, 16 },
	{ -1 }
};

static const gfx_decode pinbo_gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &pinbo_charlayout,   0, 16 },
	{ REGION_GFX1, 0, &pinbo_spritelayout, 0, 16 },
	{ -1 }
};



static MACHINE_DRIVER_START( lasso )

	/* basic machine hardware */
	MDRV_CPU_ADD_TAG("main", M6502, 2000000)	/* 2 MHz (?) */
	MDRV_CPU_PROGRAM_MAP(lasso_readmem,lasso_writemem)
	MDRV_CPU_VBLANK_INT(lasso_interrupt,2)		/* IRQ = VBlank, NMI = Coin Insertion */

	MDRV_CPU_ADD_TAG("audio", M6502, 600000)
	/* audio CPU */
	MDRV_CPU_PROGRAM_MAP(lasso_sound_readmem,lasso_sound_writemem)

	MDRV_CPU_ADD_TAG("blitter", M6502, 2000000)	/* 2 MHz (?) */
	MDRV_CPU_PROGRAM_MAP(lasso_coprocessor_readmem,lasso_coprocessor_writemem)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_REAL_60HZ_VBLANK_DURATION)
	MDRV_INTERLEAVE(100)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(32*8, 32*8)
	MDRV_VISIBLE_AREA(0, 32*8-1, 2*8, 30*8-1)
	MDRV_GFXDECODE(lasso_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(0x40)

	MDRV_PALETTE_INIT(lasso)
	MDRV_VIDEO_START(lasso)
	MDRV_VIDEO_UPDATE(lasso)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")

	MDRV_SOUND_ADD_TAG("sn76496.1", SN76496, 2000000)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.0)

	MDRV_SOUND_ADD_TAG("sn76496.2", SN76496, 2000000)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.0)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( chameleo )

	/* basic machine hardware */
	MDRV_IMPORT_FROM(lasso)
	MDRV_CPU_MODIFY("main")
	MDRV_CPU_PROGRAM_MAP(chameleo_readmem,chameleo_writemem)

	MDRV_CPU_MODIFY("audio")
	MDRV_CPU_PROGRAM_MAP(chameleo_sound_readmem,chameleo_sound_writemem)

	MDRV_CPU_REMOVE("blitter")

	/* video hardware */
	MDRV_VIDEO_UPDATE(chameleo)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( wwjgtin )

	/* basic machine hardware */
	MDRV_IMPORT_FROM(lasso)
	MDRV_CPU_MODIFY("main")
	MDRV_CPU_PROGRAM_MAP(wwjgtin_readmem,wwjgtin_writemem)

	MDRV_CPU_MODIFY("audio")
	MDRV_CPU_PROGRAM_MAP(lasso_sound_readmem,wwjgtin_sound_writemem)

	MDRV_CPU_REMOVE("blitter")

	/* video hardware */
	MDRV_VISIBLE_AREA(1*8, 31*8-1, 2*8, 30*8-1)	// Smaller visible area?
	MDRV_GFXDECODE(wwjgtin_gfxdecodeinfo)	// Has 1 additional layer
	MDRV_PALETTE_LENGTH(0x40+1)
	MDRV_COLORTABLE_LENGTH(4*16 + 16*16)	// Reserve 1 color for black

	MDRV_PALETTE_INIT(wwjgtin)
	MDRV_VIDEO_START(wwjgtin)
	MDRV_VIDEO_UPDATE(wwjgtin)

	/* sound hardware */
	MDRV_SOUND_ADD(DAC, 0)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.0)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( pinbo )

	/* basic machine hardware */
	MDRV_IMPORT_FROM(lasso)
	MDRV_CPU_MODIFY("main")
	MDRV_CPU_PROGRAM_MAP(chameleo_readmem,pinbo_writemem)

	MDRV_CPU_REPLACE("audio", Z80, 3000000)
	MDRV_CPU_PROGRAM_MAP(pinbo_sound_readmem,pinbo_sound_writemem)
	MDRV_CPU_IO_MAP(pinbo_sound_readport,pinbo_sound_writeport)

	MDRV_CPU_REMOVE("blitter")

	/* video hardware */
	MDRV_GFXDECODE(pinbo_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(256)

	MDRV_PALETTE_INIT(RRRR_GGGG_BBBB)
	MDRV_VIDEO_START(pinbo)
	MDRV_VIDEO_UPDATE(chameleo)

	/* sound hardware */
	MDRV_SOUND_REMOVE("sn76496.1")
	MDRV_SOUND_REMOVE("sn76496.2")

	MDRV_SOUND_ADD(AY8910, 1250000)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.25)

	MDRV_SOUND_ADD(AY8910, 1250000)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.25)
MACHINE_DRIVER_END


ROM_START( lasso )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )
	ROM_LOAD( "wm3",       0x8000, 0x2000, CRC(f93addd6) SHA1(b0a1b263874da8608c3bab4e8785358e2aa19c2e) )
	ROM_RELOAD(            0xc000, 0x2000)
	ROM_LOAD( "wm4",       0xe000, 0x2000, CRC(77719859) SHA1(d206b6af9a567f70d69624866ae9973652527065) )
	ROM_RELOAD(            0xa000, 0x2000)

	ROM_REGION( 0x10000, REGION_CPU2, 0 )
	ROM_LOAD( "wmc",       0x5000, 0x1000, CRC(8b4eb242) SHA1(55ada50036abbaa10799f37e35254e6ff70ee947) )
	ROM_LOAD( "wmb",       0x6000, 0x1000, CRC(4658bcb9) SHA1(ecc83ef99edbe5f69a884a142478ff0f56edba12) )
	ROM_LOAD( "wma",       0x7000, 0x1000, CRC(2e7de3e9) SHA1(665a89b9914ca16b9c08b751e142cf7320aaf793) )
	ROM_RELOAD(            0xf000, 0x1000 )

	ROM_REGION( 0x10000, REGION_CPU3, 0 ) /* 6502 code (lasso image blitter) */
	ROM_LOAD( "wm5",       0xf000, 0x1000, CRC(7dc3ff07) SHA1(46aaa9186940d06fd679a573330e9ad3796aa647) )
	ROM_RELOAD(            0x8000, 0x1000)

	ROM_REGION( 0x4000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "wm1",       0x0000, 0x0800, CRC(7db77256) SHA1(d12305bdfb6923c32982982a5544ae9bd8dbc2cb) )	/* Tiles   */
	ROM_CONTINUE(          0x1000, 0x0800             )	/* Sprites */
	ROM_CONTINUE(          0x0800, 0x0800             )
	ROM_CONTINUE(          0x1800, 0x0800             )
	ROM_LOAD( "wm2",       0x2000, 0x0800, CRC(9e7d0b6f) SHA1(c82be332209bf7331718e51926004fe9aa6f5ebd) )	/* 2nd bitplane */
	ROM_CONTINUE(          0x3000, 0x0800             )
	ROM_CONTINUE(          0x2800, 0x0800             )
	ROM_CONTINUE(          0x3800, 0x0800             )

	ROM_REGION( 0x40, REGION_PROMS, 0 )
	ROM_LOAD( "82s123.69", 0x0000, 0x0020, CRC(1eabb04d) SHA1(3dc5b407bc1b1dea77337b4e913f1e945386d5c9) )
	ROM_LOAD( "82s123.70", 0x0020, 0x0020, CRC(09060f8c) SHA1(8f14b00bcfb7ab89d2e443cc82f7a65dc96ee819) )
ROM_END

ROM_START( chameleo )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )		/* 6502 Code (Main CPU) */
	ROM_LOAD( "chamel4.bin", 0x4000, 0x2000, CRC(97379c47) SHA1(b29fa2318d4260c29fc95d22a461173dc960ad1a) )
	ROM_LOAD( "chamel5.bin", 0x6000, 0x2000, CRC(0a2cadfd) SHA1(1ccc43accd60ca15b8f03ed1c3fda76a840a2bb1) )
	ROM_LOAD( "chamel6.bin", 0x8000, 0x2000, CRC(b023c354) SHA1(0424ecf81ac9f0e055f9ff01cf0bd6d5c9ff866c) )
	ROM_LOAD( "chamel7.bin", 0xa000, 0x2000, CRC(a5a03375) SHA1(c1eac4596c2bda419f3c513ecd3df9fae49ae159) )
	ROM_RELOAD(              0xe000, 0x2000             )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )		/* 6502 Code (Sound CPU) */
	ROM_LOAD( "chamel3.bin", 0x1000, 0x1000, CRC(52eab9ec) SHA1(554c34134e3af970262da89fe82feeaf47fd30bc) )
	ROM_LOAD( "chamel2.bin", 0x6000, 0x1000, CRC(81dcc49c) SHA1(7e1b4351775f9c140a43f531da8b055271b7b28c) )
	ROM_LOAD( "chamel1.bin", 0x7000, 0x1000, CRC(96031d3b) SHA1(a143b54b98891423d355e0ba08c3b88d70fa0e23) )
	ROM_RELOAD(              0xf000, 0x1000             )

	ROM_REGION( 0x4000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "chamel8.bin", 0x0800, 0x0800, CRC(dc67916b) SHA1(8b3fad0d5d42925b44e51df7f88ea4b6a8dbb4f6) )	/* Tiles   */
	ROM_CONTINUE(            0x1800, 0x0800             )	/* Sprites */
	ROM_CONTINUE(            0x0000, 0x0800             )
	ROM_CONTINUE(            0x1000, 0x0800             )
	ROM_LOAD( "chamel9.bin", 0x2800, 0x0800, CRC(6b559bf1) SHA1(b7b8b8bccbd88ea868e2d3ccb42513615120d8e6) )	/* 2nd bitplane */
	ROM_CONTINUE(            0x3800, 0x0800             )
	ROM_CONTINUE(            0x2000, 0x0800             )
	ROM_CONTINUE(            0x3000, 0x0800             )

	ROM_REGION( 0x40, REGION_PROMS, ROMREGION_DISPOSE )	/* Colors */
	ROM_LOAD( "chambprm.bin", 0x0000, 0x0020, CRC(e3ad76df) SHA1(cd115cece4931bfcfc0f60147b942998a5c21bf7) )
	ROM_LOAD( "chamaprm.bin", 0x0020, 0x0020, CRC(c7063b54) SHA1(53baed3806848207ab3a8fafd182cabec3be4b04) )
ROM_END

ROM_START( wwjgtin )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )		/* 6502 Code (Main CPU) */
	ROM_LOAD( "ic2.6", 0x4000, 0x4000, CRC(744ba45b) SHA1(cccf3e2dd3c27bf54d2abd366cd9a044311aa031) )
	ROM_LOAD( "ic5.5", 0x8000, 0x4000, CRC(af751614) SHA1(fc0f0a3967524b1743a182c1da4f9b0c3097a157) )
	ROM_RELOAD(        0xc000, 0x4000             )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )		/* 6502 Code (Sound CPU) */
	ROM_LOAD( "ic59.9", 0x4000, 0x4000, CRC(2ecb4d98) SHA1(d5b0d447b24f64fca452dc13e6ff95b090fce2d7) )
	ROM_RELOAD(         0xc000, 0x4000             )

	ROM_REGION( 0x8000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "ic81.7", 0x0000, 0x0800, CRC(a27f1a63) SHA1(3c770424bd4996f648687afce4aecea252da83a7) )	/* Tiles   */
	ROM_CONTINUE(       0x2000, 0x0800             )	/* Sprites */
	ROM_CONTINUE(       0x0800, 0x0800             )
	ROM_CONTINUE(       0x2800, 0x0800             )
	ROM_CONTINUE(       0x1000, 0x0800             )
	ROM_CONTINUE(       0x3000, 0x0800             )
	ROM_CONTINUE(       0x1800, 0x0800             )
	ROM_CONTINUE(       0x3800, 0x0800             )
	ROM_LOAD( "ic82.8", 0x4000, 0x0800, CRC(ea2862b3) SHA1(f7604fd324560c54311c35f806a17e30e018032a) )	/* 2nd bitplane */
	ROM_CONTINUE(       0x6000, 0x0800             )	/* Sprites */
	ROM_CONTINUE(       0x4800, 0x0800             )
	ROM_CONTINUE(       0x6800, 0x0800             )
	ROM_CONTINUE(       0x5000, 0x0800             )
	ROM_CONTINUE(       0x7000, 0x0800             )
	ROM_CONTINUE(       0x5800, 0x0800             )
	ROM_CONTINUE(       0x7800, 0x0800             )

	ROM_REGION( 0x4000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "ic47.3", 0x0000, 0x2000, CRC(40594c59) SHA1(94533be8e267d9aa5bcdd52b45f6974436d3fed5) )	// 1xxxxxxxxxxxx = 0xFF
	ROM_LOAD( "ic46.4", 0x2000, 0x2000, CRC(d1921348) SHA1(8b5506ff80a31ce721aed515cad1b4a7e52e47a2) )

	ROM_REGION( 0x4000, REGION_USER1, 0 )				/* tilemap */
	ROM_LOAD( "ic48.2", 0x0000, 0x2000, CRC(a4a7df77) SHA1(476aab702346a402169ab404a8b06589e4932d37) )
	ROM_LOAD( "ic49.1", 0x2000, 0x2000, CRC(e480fbba) SHA1(197c86747ef8477040169f90eb6e04d928aedbe5) )	// FIXED BITS (1111xxxx)

	ROM_REGION( 0x40, REGION_PROMS, ROMREGION_DISPOSE )
	ROM_LOAD( "2.bpr",  0x0000, 0x0020, CRC(79adda5d) SHA1(e54de3eb02f744d49f524cd81e1cf993338916e3) )
	ROM_LOAD( "1.bpr",  0x0020, 0x0020, CRC(c1a93cc8) SHA1(805641ea2ce86589b968f1ff44e5d3ab9377769d) )
ROM_END

ROM_START( pinbo )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "rom2.b7",     0x2000, 0x2000, CRC(9a185338) SHA1(4029cf927686b5e14ef7600b17ea3056cc58b15b) )
	ROM_LOAD( "rom3.e7",     0x6000, 0x2000, CRC(1cd1b3bd) SHA1(388ea72568f5bfd39856d872415327a2afaf7fad) )
	ROM_LOAD( "rom4.h7",     0x8000, 0x2000, CRC(ba043fa7) SHA1(ef3d67b6dab5c82035c58290879a3ca969a0256d) )
	ROM_LOAD( "rom5.j7",     0xa000, 0x2000, CRC(e71046c4) SHA1(f49133544c98df5f3e1a1d2ae92e17261b1504fc) )
	ROM_RELOAD(              0xe000, 0x2000 )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )  /* 64K for sound */
	ROM_LOAD( "rom1.s8",     0x0000, 0x2000, CRC(ca45a1be) SHA1(d0b2d8f1e6d01b60cba83d2bd458a57548549b4b) )

	ROM_REGION( 0xc000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "rom6.a1",     0x0000, 0x0800, CRC(74fe8e98) SHA1(3c9ac38d7054b2831a515786b6f204b1804aaea3) )	/* tiles   */
	ROM_CONTINUE(       	 0x2000, 0x0800             )	/* sprites */
	ROM_CONTINUE(       	 0x0800, 0x0800             )
	ROM_CONTINUE(       	 0x2800, 0x0800             )
	ROM_CONTINUE(       	 0x1000, 0x0800             )
	ROM_CONTINUE(       	 0x3000, 0x0800             )
	ROM_CONTINUE(       	 0x1800, 0x0800             )
	ROM_CONTINUE(       	 0x3800, 0x0800             )
	ROM_LOAD( "rom8.c1",     0x4000, 0x0800, CRC(5a800fe7) SHA1(375269ec73fab7f0cf017a79e002e31b006f5ad7) )	/* 2nd bitplane */
	ROM_CONTINUE(       	 0x6000, 0x0800             )
	ROM_CONTINUE(       	 0x4800, 0x0800             )
	ROM_CONTINUE(       	 0x6800, 0x0800             )
	ROM_CONTINUE(       	 0x5000, 0x0800             )
	ROM_CONTINUE(       	 0x7000, 0x0800             )
	ROM_CONTINUE(       	 0x5800, 0x0800             )
	ROM_CONTINUE(       	 0x7800, 0x0800             )
	ROM_LOAD( "rom7.d1",     0x8000, 0x0800, CRC(327a3c21) SHA1(e938915d28ac4ec033b20d33728788493e3f30f6) )	/* 3rd bitplane */
	ROM_CONTINUE(       	 0xa000, 0x0800             )
	ROM_CONTINUE(       	 0x8800, 0x0800             )
	ROM_CONTINUE(       	 0xa800, 0x0800             )
	ROM_CONTINUE(       	 0x9000, 0x0800             )
	ROM_CONTINUE(       	 0xb000, 0x0800             )
	ROM_CONTINUE(       	 0x9800, 0x0800             )
	ROM_CONTINUE(       	 0xb800, 0x0800             )

	ROM_REGION( 0x00300, REGION_PROMS, 0 ) /* Color PROMs */
	ROM_LOAD( "red.l10",     0x0000, 0x0100, CRC(e6c9ba52) SHA1(6ea96f9bd71de6181d675b0f2d59a8c5e1be5aa3) )
	ROM_LOAD( "green.k10",   0x0100, 0x0100, CRC(1bf2d335) SHA1(dcb074d3de939dfc652743e25bc66bd6fbdc3289) )
	ROM_LOAD( "blue.n10",    0x0200, 0x0100, CRC(e41250ad) SHA1(2e9a2babbacb1753057d46cf1dd6dc183611747e) )
ROM_END

ROM_START( pinboa )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "rom2.b7",     0x2000, 0x2000, CRC(9a185338) SHA1(4029cf927686b5e14ef7600b17ea3056cc58b15b) )
	ROM_LOAD( "6.bin",       0x6000, 0x2000, CRC(f80b204c) SHA1(ee9b4ae1d8ea2fc062022fcfae67df87ed7aff41) )
	ROM_LOAD( "5.bin",       0x8000, 0x2000, CRC(c57fe503) SHA1(11b7371c07c9b2c73ab61420a2cc609653c48d37) )
	ROM_LOAD( "4.bin",       0xa000, 0x2000, CRC(d632b598) SHA1(270a5a790a66eaf3d90bf8081ab144fd1af9db3d) )
	ROM_RELOAD(              0xe000, 0x2000 )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )  /* 64K for sound */
	ROM_LOAD( "8.bin",       0x0000, 0x2000, CRC(32d1df14) SHA1(c0d4181378bbd6f2c594e923e2f8b21647c7fb0e) )

	ROM_REGION( 0xc000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "rom6.a1",     0x0000, 0x0800, CRC(74fe8e98) SHA1(3c9ac38d7054b2831a515786b6f204b1804aaea3) )	/* tiles   */
	ROM_CONTINUE(       	 0x2000, 0x0800             )	/* sprites */
	ROM_CONTINUE(       	 0x0800, 0x0800             )
	ROM_CONTINUE(       	 0x2800, 0x0800             )
	ROM_CONTINUE(       	 0x1000, 0x0800             )
	ROM_CONTINUE(       	 0x3000, 0x0800             )
	ROM_CONTINUE(       	 0x1800, 0x0800             )
	ROM_CONTINUE(       	 0x3800, 0x0800             )
	ROM_LOAD( "rom8.c1",     0x4000, 0x0800, CRC(5a800fe7) SHA1(375269ec73fab7f0cf017a79e002e31b006f5ad7) )	/* 2nd bitplane */
	ROM_CONTINUE(       	 0x6000, 0x0800             )
	ROM_CONTINUE(       	 0x4800, 0x0800             )
	ROM_CONTINUE(       	 0x6800, 0x0800             )
	ROM_CONTINUE(       	 0x5000, 0x0800             )
	ROM_CONTINUE(       	 0x7000, 0x0800             )
	ROM_CONTINUE(       	 0x5800, 0x0800             )
	ROM_CONTINUE(       	 0x7800, 0x0800             )
	ROM_LOAD( "2.bin",       0x8000, 0x0800, CRC(33cac92e) SHA1(55d4ff3ae9c9519a59bd6021a53584c873b4d327) ) /* 3rd bitplane */
	ROM_CONTINUE(       	 0xa000, 0x0800             )
	ROM_CONTINUE(       	 0x8800, 0x0800             )
	ROM_CONTINUE(       	 0xa800, 0x0800             )
	ROM_CONTINUE(       	 0x9000, 0x0800             )
	ROM_CONTINUE(       	 0xb000, 0x0800             )
	ROM_CONTINUE(       	 0x9800, 0x0800             )
	ROM_CONTINUE(       	 0xb800, 0x0800             )

	ROM_REGION( 0x00300, REGION_PROMS, 0 ) /* Color PROMs */
	ROM_LOAD( "red.l10",     0x0000, 0x0100, CRC(e6c9ba52) SHA1(6ea96f9bd71de6181d675b0f2d59a8c5e1be5aa3) )
	ROM_LOAD( "green.k10",   0x0100, 0x0100, CRC(1bf2d335) SHA1(dcb074d3de939dfc652743e25bc66bd6fbdc3289) )
	ROM_LOAD( "blue.n10",    0x0200, 0x0100, CRC(e41250ad) SHA1(2e9a2babbacb1753057d46cf1dd6dc183611747e) )
ROM_END

ROM_START( pinbos )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "b4.bin",      0x2000, 0x2000, CRC(d9452d4f) SHA1(c744ee037275b880c0ddc2fd83b3c05eb0a53621) )
	ROM_LOAD( "b5.bin",      0x6000, 0x2000, CRC(f80b204c) SHA1(ee9b4ae1d8ea2fc062022fcfae67df87ed7aff41) )
	ROM_LOAD( "b6.bin",      0x8000, 0x2000, CRC(ae967d83) SHA1(e79db85917a31821d10f919c4c429da33e97894d) )
	ROM_LOAD( "b7.bin",      0xa000, 0x2000, CRC(7a584b4e) SHA1(2eb55b706815228b3b12ee5c0f6c415cd1d612e6) )
	ROM_RELOAD(              0xe000, 0x2000 )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )  /* 64K for sound */
	ROM_LOAD( "b8.bin",      0x0000, 0x2000, CRC(32d1df14) SHA1(c0d4181378bbd6f2c594e923e2f8b21647c7fb0e) )

	ROM_REGION( 0xc000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "rom6.a1",     0x0000, 0x0800, CRC(74fe8e98) SHA1(3c9ac38d7054b2831a515786b6f204b1804aaea3) )	/* tiles   */
	ROM_CONTINUE(       	 0x2000, 0x0800             )	/* sprites */
	ROM_CONTINUE(       	 0x0800, 0x0800             )
	ROM_CONTINUE(       	 0x2800, 0x0800             )
	ROM_CONTINUE(       	 0x1000, 0x0800             )
	ROM_CONTINUE(       	 0x3000, 0x0800             )
	ROM_CONTINUE(       	 0x1800, 0x0800             )
	ROM_CONTINUE(       	 0x3800, 0x0800             )
	ROM_LOAD( "rom8.c1",     0x4000, 0x0800, CRC(5a800fe7) SHA1(375269ec73fab7f0cf017a79e002e31b006f5ad7) )	/* 2nd bitplane */
	ROM_CONTINUE(       	 0x6000, 0x0800             )
	ROM_CONTINUE(       	 0x4800, 0x0800             )
	ROM_CONTINUE(       	 0x6800, 0x0800             )
	ROM_CONTINUE(       	 0x5000, 0x0800             )
	ROM_CONTINUE(       	 0x7000, 0x0800             )
	ROM_CONTINUE(       	 0x5800, 0x0800             )
	ROM_CONTINUE(       	 0x7800, 0x0800             )
	ROM_LOAD( "rom7.d1",     0x8000, 0x0800, CRC(327a3c21) SHA1(e938915d28ac4ec033b20d33728788493e3f30f6) )	/* 3rd bitplane */
	ROM_CONTINUE(       	 0xa000, 0x0800             )
	ROM_CONTINUE(       	 0x8800, 0x0800             )
	ROM_CONTINUE(       	 0xa800, 0x0800             )
	ROM_CONTINUE(       	 0x9000, 0x0800             )
	ROM_CONTINUE(       	 0xb000, 0x0800             )
	ROM_CONTINUE(       	 0x9800, 0x0800             )
	ROM_CONTINUE(       	 0xb800, 0x0800             )

	ROM_REGION( 0x00300, REGION_PROMS, 0 ) /* Color PROMs */
	ROM_LOAD( "red.l10",     0x0000, 0x0100, CRC(e6c9ba52) SHA1(6ea96f9bd71de6181d675b0f2d59a8c5e1be5aa3) )
	ROM_LOAD( "green.k10",   0x0100, 0x0100, CRC(1bf2d335) SHA1(dcb074d3de939dfc652743e25bc66bd6fbdc3289) )
	ROM_LOAD( "blue.n10",    0x0200, 0x0100, CRC(e41250ad) SHA1(2e9a2babbacb1753057d46cf1dd6dc183611747e) )
ROM_END



/***************************************************************************

                                Game Drivers

***************************************************************************/

GAME( 1982, lasso,    0,     lasso,    lasso,    0, ROT90, "SNK", "Lasso"                  , 0 )
GAME( 1983, chameleo, 0,     chameleo, chameleo, 0, ROT0,  "Jaleco", "Chameleon"              , 0 )
GAME( 1984, wwjgtin,  0,     wwjgtin,  wwjgtin,  0, ROT0,  "Jaleco / Casio", "Wai Wai Jockey Gate-In!", 0 )
GAME( 1984, pinbo,    0,     pinbo,    pinbo,    0, ROT90, "Jaleco", "Pinbo (set 1)", 0 )
GAME( 1984, pinboa,   pinbo, pinbo,    pinbo,   0, ROT90, "Jaleco", "Pinbo (set 2)", 0 )
GAME( 1985, pinbos,   pinbo, pinbo,    pinbo,   0, ROT90, "bootleg?", "Pinbo (Strike)", 0 )
