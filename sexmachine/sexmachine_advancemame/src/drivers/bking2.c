/***************************************************************************


Birdie King / Birdie King II / Birdie King III Memory Map
---------------------------------------------------------

0000-7fff ROM
8000-83ff Scratch RAM
8400-8fff (Scratch RAM again, address lines AB10, AB11 ignored)
9000-97ff Playfield RAM
a000-bfff Unused?


NOTE:
ROM DM03 is missing from all known ROM sets.  This is a color palette.
* is this note out of date?, DM_03.d1 in bking.zip = 82s141.2d in bking2.zip

***************************************************************************/

#include "driver.h"
#include "sound/ay8910.h"
#include "sound/dac.h"

PALETTE_INIT( bking2 );

VIDEO_START( bking2 );
VIDEO_UPDATE( bking2 );
VIDEO_EOF( bking2 );

WRITE8_HANDLER( bking2_xld1_w );
WRITE8_HANDLER( bking2_yld1_w );
WRITE8_HANDLER( bking2_xld2_w );
WRITE8_HANDLER( bking2_yld2_w );
WRITE8_HANDLER( bking2_xld3_w );
WRITE8_HANDLER( bking2_yld3_w );
WRITE8_HANDLER( bking2_msk_w );
WRITE8_HANDLER( bking2_cont1_w );
WRITE8_HANDLER( bking2_cont2_w );
WRITE8_HANDLER( bking2_cont3_w );
WRITE8_HANDLER( bking2_hitclr_w );
WRITE8_HANDLER( bking2_playfield_w );

READ8_HANDLER( bking2_input_port_5_r );
READ8_HANDLER( bking2_input_port_6_r );
READ8_HANDLER( bking2_pos_r );

UINT8* bking2_playfield_ram;


static int sndnmi_enable = 1;

static READ8_HANDLER( bking2_sndnmi_disable_r )
{
	sndnmi_enable = 0;
	return 0;
}

static WRITE8_HANDLER( bking2_sndnmi_enable_w )
{
	sndnmi_enable = 1;
}

static WRITE8_HANDLER( bking2_soundlatch_w )
{
	int i,code;

	code = 0;
	for (i = 0;i < 8;i++)
		if (data & (1 << i)) code |= 0x80 >> i;

	soundlatch_w(offset,code);
	if (sndnmi_enable) cpunum_set_input_line(1, INPUT_LINE_NMI, PULSE_LINE);
}


static int bk3_l, bk3_h;

static WRITE8_HANDLER( bk3_l_w)
{
	bk3_l = data;
}

static WRITE8_HANDLER( bk3_h_w)
{
	bk3_h = data;
}

static READ8_HANDLER( bk3_r )
{
	unsigned char *rom = memory_region(REGION_USER2);
	return rom[bk3_h*256+bk3_l];
}

static WRITE8_HANDLER( unk_w )
{
/*
    0 = finished reading extra rom
    1 = started reading extra rom
*/
}

static READ8_HANDLER( mcu_status_r )
{
	static int res = 3;

	return res;//cpu data / MCU ready
}


/*
Birdie King 3 MCU simulation
Nothing really special to report,just the typical protection HW test and another
protection routine with 0x30 command when the player have to shot on the green.

Todo:
\-How to handle the reads at port($6f)?
\-In-depth game untested.
*/
static unsigned char mcu_val;

static WRITE8_HANDLER( mcu_data_w )
{
#ifdef MAME_DEBUG
	logerror("mcu_data_w = %x\n",data);
#endif
	mcu_val = data;
	/* HW test */
	/* all bits of port ($02) except the MSB are connected to the command,
       in all cases it should return 0x5e.This one is here to avoid to get a big
       switch-case statement in the mcu_data_r function... */
	if(mcu_val >= 0x80)
		mcu_val = 0x5e;
}

static READ8_HANDLER( mcu_data_r )
{
//  ui_popup("MCU-r1 PC = %04x %02x",activecpu_get_pc(),mcu_val);
	switch(mcu_val)
	{
		/* Shot counter control at the green (check $bdf and afterwards in that condition)*/
		case 0x30: return (mcu_val-0x1e);
		default:   return (mcu_val);
	}
}

static READ8_HANDLER( mcu_data_r2 )
{
//  ui_popup("MCU-r2 PC = %04x %02x",activecpu_get_pc(),mcu_val);
	return 0x31; //no "bad rom.", no "bad ext."
}

static ADDRESS_MAP_START( readmem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x7fff) AM_READ(MRA8_ROM)
	AM_RANGE(0x8000, 0x83ff) AM_READ(MRA8_RAM)
	AM_RANGE(0x9000, 0x97ff) AM_READ(MRA8_RAM)
ADDRESS_MAP_END

static ADDRESS_MAP_START( writemem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x7fff) AM_WRITE(MWA8_ROM)
	AM_RANGE(0x8000, 0x83ff) AM_WRITE(MWA8_RAM)
	AM_RANGE(0x9000, 0x97ff) AM_WRITE(bking2_playfield_w) AM_BASE(&bking2_playfield_ram)
ADDRESS_MAP_END

static ADDRESS_MAP_START( readport, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_FLAGS( AMEF_ABITS(8) )
	AM_RANGE(0x00, 0x00) AM_READ(input_port_0_r)
	AM_RANGE(0x01, 0x01) AM_READ(input_port_1_r)
	AM_RANGE(0x02, 0x02) AM_READ(input_port_2_r)
	AM_RANGE(0x03, 0x03) AM_READ(input_port_3_r)
	AM_RANGE(0x04, 0x04) AM_READ(input_port_4_r)
	AM_RANGE(0x05, 0x05) AM_READ(bking2_input_port_5_r)
	AM_RANGE(0x06, 0x06) AM_READ(bking2_input_port_6_r)
	AM_RANGE(0x07, 0x1f) AM_READ(bking2_pos_r)

	AM_RANGE(0x2f, 0x2f) AM_READ(mcu_data_r)
	AM_RANGE(0x4f, 0x4f) AM_READ(mcu_status_r)
	AM_RANGE(0x60, 0x60) AM_READ(bk3_r)
	AM_RANGE(0x6f, 0x6f) AM_READ(mcu_data_r2)
ADDRESS_MAP_END

static ADDRESS_MAP_START( writeport, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_FLAGS( AMEF_ABITS(8) )
	AM_RANGE(0x00, 0x00) AM_WRITE(bking2_xld1_w)
	AM_RANGE(0x01, 0x01) AM_WRITE(bking2_yld1_w)
	AM_RANGE(0x02, 0x02) AM_WRITE(bking2_xld2_w)
	AM_RANGE(0x03, 0x03) AM_WRITE(bking2_yld2_w)
	AM_RANGE(0x04, 0x04) AM_WRITE(bking2_xld3_w)
	AM_RANGE(0x05, 0x05) AM_WRITE(bking2_yld3_w)
	AM_RANGE(0x06, 0x06) AM_WRITE(bking2_msk_w)
	AM_RANGE(0x07, 0x07) AM_WRITE(watchdog_reset_w)
	AM_RANGE(0x08, 0x08) AM_WRITE(bking2_cont1_w)
	AM_RANGE(0x09, 0x09) AM_WRITE(bking2_cont2_w)
	AM_RANGE(0x0a, 0x0a) AM_WRITE(bking2_cont3_w)
	AM_RANGE(0x0b, 0x0b) AM_WRITE(bking2_soundlatch_w)
//  AM_RANGE(0x0c, 0x0c) AM_WRITE(bking2_eport2_w)   this is not shown to be connected anywhere
	AM_RANGE(0x0d, 0x0d) AM_WRITE(bking2_hitclr_w)

	AM_RANGE(0x2f, 0x2f) AM_WRITE(mcu_data_w)
	AM_RANGE(0x4f, 0x4f) AM_WRITE(unk_w)
	AM_RANGE(0x6f, 0x6f) AM_WRITE(bk3_h_w)
	AM_RANGE(0x8f, 0x8f) AM_WRITE(bk3_l_w)
ADDRESS_MAP_END

static ADDRESS_MAP_START( sound_readmem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x1fff) AM_READ(MRA8_ROM)
	AM_RANGE(0x2000, 0x2fff) AM_READ(MRA8_ROM) //only bking3
	AM_RANGE(0x4000, 0x43ff) AM_READ(MRA8_RAM)
	AM_RANGE(0x4401, 0x4401) AM_READ(AY8910_read_port_0_r)
	AM_RANGE(0x4403, 0x4403) AM_READ(AY8910_read_port_1_r)
	AM_RANGE(0x4800, 0x4800) AM_READ(soundlatch_r)
	AM_RANGE(0x4802, 0x4802) AM_READ(bking2_sndnmi_disable_r)
	AM_RANGE(0xe000, 0xefff) AM_READ(MRA8_ROM)   /* space for some other ROM???
                                      It's checked if there is valid code there
                                      [Probably diagnostic ROM like other early Taito games -AS]*/
ADDRESS_MAP_END

static ADDRESS_MAP_START( sound_writemem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x1fff) AM_WRITE(MWA8_ROM)
	AM_RANGE(0x2000, 0x2fff) AM_WRITE(MWA8_ROM) //only bking3
	AM_RANGE(0x4000, 0x43ff) AM_WRITE(MWA8_RAM)
	AM_RANGE(0x4400, 0x4400) AM_WRITE(AY8910_control_port_0_w)
	AM_RANGE(0x4401, 0x4401) AM_WRITE(AY8910_write_port_0_w)
	AM_RANGE(0x4402, 0x4402) AM_WRITE(AY8910_control_port_1_w)
	AM_RANGE(0x4403, 0x4403) AM_WRITE(AY8910_write_port_1_w)
	AM_RANGE(0x4802, 0x4802) AM_WRITE(bking2_sndnmi_enable_w)
ADDRESS_MAP_END

INPUT_PORTS_START( bking )
	PORT_START  /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_COIN2 )
	PORT_BIT( 0xfc, IP_ACTIVE_HIGH, IPT_UNUSED )

	PORT_START  /* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_START1 ) /* Continue 1 */
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_START2 ) /* Continue 2 */
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_TILT )
	PORT_BIT( 0xc0, IP_ACTIVE_LOW, IPT_UNUSED ) /* Not Connected */

	PORT_START  /* IN2 - DIP Switch A */
	PORT_DIPNAME( 0x01, 0x00, "Bonus Holes Awarded" )
	PORT_DIPSETTING(    0x00, "Fewer" )
	PORT_DIPSETTING(    0x01, "More" )
	PORT_DIPNAME( 0x02, 0x02, "Holes Awarded for Hole-in-One" )
	PORT_DIPSETTING(    0x00, "3" )
	PORT_DIPSETTING(    0x02, "9" )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR(Free_Play) )
	PORT_DIPSETTING(    0x04, DEF_STR(Off))
	PORT_DIPSETTING(    0x00, DEF_STR(On))
	PORT_DIPNAME( 0x18, 0x18, "Holes (Lives)" )
	PORT_DIPSETTING(    0x18, "3" )
	PORT_DIPSETTING(    0x08, "4" )
	PORT_DIPSETTING(    0x10, "5" )
	PORT_DIPSETTING(    0x00, "9" )
	PORT_DIPNAME( 0x20, 0x20, "Self Test" )
	PORT_DIPSETTING(    0x20, DEF_STR(Off))
	PORT_DIPSETTING(    0x00, DEF_STR(On))
	PORT_DIPNAME( 0x40, 0x40, DEF_STR(Flip_Screen) )
	PORT_DIPSETTING(    0x40, DEF_STR(Off))
	PORT_DIPSETTING(    0x00, DEF_STR(On))
	PORT_DIPNAME( 0x80, 0x00, DEF_STR(Cabinet) )
	PORT_DIPSETTING(    0x00, DEF_STR(Upright) )
	PORT_DIPSETTING(    0x80, DEF_STR(Cocktail) )

	PORT_START  /* IN3 - DIP Switch B */
	PORT_DIPNAME( 0x0f, 0x00, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x0f, DEF_STR( 9C_1C ) )
	PORT_DIPSETTING(    0x0e, DEF_STR( 8C_1C ) )
	PORT_DIPSETTING(    0x0d, DEF_STR( 7C_1C ) )
	PORT_DIPSETTING(    0x0c, DEF_STR( 6C_1C ) )
	PORT_DIPSETTING(    0x0b, DEF_STR( 5C_1C ) )
	PORT_DIPSETTING(    0x0a, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x09, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(    0x05, DEF_STR( 1C_6C ) )
	PORT_DIPSETTING(    0x06, DEF_STR( 1C_7C ) )
	PORT_DIPSETTING(    0x07, DEF_STR( 1C_8C ) )
	PORT_DIPNAME( 0xf0, 0x00, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0xf0, DEF_STR( 9C_1C ) )
	PORT_DIPSETTING(    0xe0, DEF_STR( 8C_1C ) )
	PORT_DIPSETTING(    0xd0, DEF_STR( 7C_1C ) )
	PORT_DIPSETTING(    0xc0, DEF_STR( 6C_1C ) )
	PORT_DIPSETTING(    0xb0, DEF_STR( 5C_1C ) )
	PORT_DIPSETTING(    0xa0, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x90, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x80, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x20, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x30, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x40, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(    0x50, DEF_STR( 1C_6C ) )
	PORT_DIPSETTING(    0x60, DEF_STR( 1C_7C ) )
	PORT_DIPSETTING(    0x70, DEF_STR( 1C_8C ) )

	PORT_START  /* IN4 - DIP Switch C */
	PORT_DIPNAME( 0x01, 0x01, "Crow" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x01, DEF_STR( On ) )
	PORT_DIPNAME( 0x06, 0x04, "Crow Flight Pattern" )
	PORT_DIPSETTING(    0x00, "1" )
	PORT_DIPSETTING(    0x02, "2" )
	PORT_DIPSETTING(    0x04, "3" )
	PORT_DIPSETTING(    0x06, "4" )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR(Unused) )
	PORT_DIPSETTING(    0x00, DEF_STR(Off))
	PORT_DIPSETTING(    0x08, DEF_STR(On))
	PORT_DIPNAME( 0x10, 0x10, "Coinage Display" )
	PORT_DIPSETTING(    0x00, DEF_STR(Off))
	PORT_DIPSETTING(    0x10, DEF_STR(On))
	PORT_DIPNAME( 0x20, 0x20, "Year Display" )
	PORT_DIPSETTING(    0x00, DEF_STR(Off))
	PORT_DIPSETTING(    0x20, DEF_STR(On))
	PORT_DIPNAME( 0x40, 0x40, "Check" )
	PORT_DIPSETTING(    0x00, "Check" )
	PORT_DIPSETTING(    0x40, DEF_STR( Normal ) )
	PORT_DIPNAME( 0x80, 0x80, "Coin Chutes" )
	PORT_DIPSETTING(    0x00, "1" )
	PORT_DIPSETTING(    0x80, "2" )

	PORT_START  /* IN5 */
	PORT_BIT( 0xff, 0x00, IPT_TRACKBALL_X ) PORT_SENSITIVITY(25) PORT_KEYDELTA(10) /* Sensitivity, clip, min, max */

	PORT_START  /* IN6 */
	PORT_BIT( 0xff, 0x00, IPT_TRACKBALL_Y ) PORT_SENSITIVITY(25) PORT_KEYDELTA(10) PORT_REVERSE /* Sensitivity, clip, min, max */

	PORT_START  /* IN7 */
	PORT_BIT( 0xff, 0x00, IPT_TRACKBALL_X ) PORT_SENSITIVITY(25) PORT_KEYDELTA(10) PORT_COCKTAIL /* Sensitivity, clip, min, max */

	PORT_START  /* IN8 */
	PORT_BIT( 0xff, 0x00, IPT_TRACKBALL_Y ) PORT_SENSITIVITY(25) PORT_KEYDELTA(10) PORT_REVERSE PORT_COCKTAIL /* Sensitivity, clip, min, max */
INPUT_PORTS_END

INPUT_PORTS_START( bking2 )
	PORT_START  /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_COIN2 )
	PORT_BIT( 0xfc, IP_ACTIVE_HIGH, IPT_UNUSED )

	PORT_START  /* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_START1 ) /* Continue 1 */
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_START2 ) /* Continue 2 */
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_TILT )
	PORT_BIT( 0xc0, IP_ACTIVE_LOW, IPT_UNUSED ) /* Not Connected */

	PORT_START  /* IN2 - DIP Switch A */
	PORT_DIPNAME( 0x01, 0x00, "Bonus Holes Awarded" )
	PORT_DIPSETTING(    0x00, "Fewer" )
	PORT_DIPSETTING(    0x01, "More" )
	PORT_DIPNAME( 0x02, 0x02, "Holes Awarded for Hole-in-One" )
	PORT_DIPSETTING(    0x00, "3" )
	PORT_DIPSETTING(    0x02, "9" )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR(Free_Play) )
	PORT_DIPSETTING(    0x04, DEF_STR(Off))
	PORT_DIPSETTING(    0x00, DEF_STR(On))
	PORT_DIPNAME( 0x18, 0x18, "Holes (Lives)" )
	PORT_DIPSETTING(    0x18, "3" )
	PORT_DIPSETTING(    0x08, "4" )
	PORT_DIPSETTING(    0x10, "5" )
	PORT_DIPSETTING(    0x00, "9" )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR(Unused) )
	PORT_DIPSETTING(    0x20, DEF_STR(Off))
	PORT_DIPSETTING(    0x00, DEF_STR(On))
	PORT_DIPNAME( 0x40, 0x40, DEF_STR(Flip_Screen) )
	PORT_DIPSETTING(    0x40, DEF_STR(Off))
	PORT_DIPSETTING(    0x00, DEF_STR(On))
	PORT_DIPNAME( 0x80, 0x00, DEF_STR(Cabinet) )
	PORT_DIPSETTING(    0x00, DEF_STR(Upright) )
	PORT_DIPSETTING(    0x80, DEF_STR(Cocktail) )

	PORT_START  /* IN3 - DIP Switch B */
	PORT_DIPNAME( 0x0f, 0x00, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x0f, DEF_STR( 9C_1C ) )
	PORT_DIPSETTING(    0x0e, DEF_STR( 8C_1C ) )
	PORT_DIPSETTING(    0x0d, DEF_STR( 7C_1C ) )
	PORT_DIPSETTING(    0x0c, DEF_STR( 6C_1C ) )
	PORT_DIPSETTING(    0x0b, DEF_STR( 5C_1C ) )
	PORT_DIPSETTING(    0x0a, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x09, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(    0x05, DEF_STR( 1C_6C ) )
	PORT_DIPSETTING(    0x06, DEF_STR( 1C_7C ) )
	PORT_DIPSETTING(    0x07, DEF_STR( 1C_8C ) )
	PORT_DIPNAME( 0xf0, 0x00, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0xf0, DEF_STR( 9C_1C ) )
	PORT_DIPSETTING(    0xe0, DEF_STR( 8C_1C ) )
	PORT_DIPSETTING(    0xd0, DEF_STR( 7C_1C ) )
	PORT_DIPSETTING(    0xc0, DEF_STR( 6C_1C ) )
	PORT_DIPSETTING(    0xb0, DEF_STR( 5C_1C ) )
	PORT_DIPSETTING(    0xa0, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x90, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x80, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x20, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x30, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x40, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(    0x50, DEF_STR( 1C_6C ) )
	PORT_DIPSETTING(    0x60, DEF_STR( 1C_7C ) )
	PORT_DIPSETTING(    0x70, DEF_STR( 1C_8C ) )

	PORT_START  /* IN4 - DIP Switch C */
	PORT_DIPNAME( 0x01, 0x01, "Crow" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x01, DEF_STR( On ) )
	PORT_DIPNAME( 0x06, 0x04, "Crow Flight Pattern" )
	PORT_DIPSETTING(    0x00, "1" )
	PORT_DIPSETTING(    0x02, "2" )
	PORT_DIPSETTING(    0x04, "3" )
	PORT_DIPSETTING(    0x06, "4" )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR(Unused) )
	PORT_DIPSETTING(    0x00, DEF_STR(Off))
	PORT_DIPSETTING(    0x08, DEF_STR(On))
	PORT_DIPNAME( 0x10, 0x10, "Coinage Display" )
	PORT_DIPSETTING(    0x00, DEF_STR(Off))
	PORT_DIPSETTING(    0x10, DEF_STR(On))
	PORT_DIPNAME( 0x20, 0x20, "Year Display" )
	PORT_DIPSETTING(    0x00, DEF_STR(Off))
	PORT_DIPSETTING(    0x20, DEF_STR(On))
	PORT_DIPNAME( 0x40, 0x40, "Check" )
	PORT_DIPSETTING(    0x00, "Check" )
	PORT_DIPSETTING(    0x40, DEF_STR( Normal ) )
	PORT_DIPNAME( 0x80, 0x80, "Coin Chutes" )
	PORT_DIPSETTING(    0x00, "1" )
	PORT_DIPSETTING(    0x80, "2" )

	PORT_START  /* IN5 */
	PORT_BIT( 0xff, 0x00, IPT_TRACKBALL_X ) PORT_SENSITIVITY(25) PORT_KEYDELTA(10) /* Sensitivity, clip, min, max */

	PORT_START  /* IN6 */
	PORT_BIT( 0xff, 0x00, IPT_TRACKBALL_Y ) PORT_SENSITIVITY(25) PORT_KEYDELTA(10) PORT_REVERSE /* Sensitivity, clip, min, max */

	PORT_START  /* IN7 */
	PORT_BIT( 0xff, 0x00, IPT_TRACKBALL_X ) PORT_SENSITIVITY(25) PORT_KEYDELTA(10) PORT_COCKTAIL /* Sensitivity, clip, min, max */

	PORT_START  /* IN8 */
	PORT_BIT( 0xff, 0x00, IPT_TRACKBALL_Y ) PORT_SENSITIVITY(25) PORT_KEYDELTA(10) PORT_REVERSE PORT_COCKTAIL /* Sensitivity, clip, min, max */
INPUT_PORTS_END


static const gfx_layout charlayout =
{
	8,8,    /* 8*8 characters */
	1024,   /* 1024 characters */
	3,      /* 3 bits per pixel */
	{ 0*1024*8*8, 1*1024*8*8, 2*1024*8*8 }, /* the bitplanes are separated */
	{ 7, 6, 5, 4, 3, 2, 1, 0 }, /* reverse layout */
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8     /* every char takes 8 consecutive bytes */
};

static gfx_layout crowlayout =
{
	16,32,	/* 16*32 characters */
	16,		/* 16 characters */
	2,		/* 2 bits per pixel */
	{ 0, 4 },
	{ 3*32*8+3, 3*32*8+2, 3*32*8+1, 3*32*8+0,
	  2*32*8+3, 2*32*8+2, 2*32*8+1, 2*32*8+0,
	    32*8+3,   32*8+2,   32*8+1,   32*8+0,
		     3,        2,        1,        0 }, /* reverse layout */
	{ 31*8, 30*8, 29*8, 28*8, 27*8, 26*8, 25*8, 24*8,
	  23*8, 22*8, 21*8, 20*8, 19*8, 18*8, 17*8, 16*8,
	  15*8, 14*8, 13*8, 12*8, 11*8, 10*8,  9*8,  8*8,
	   7*8,  6*8,  5*8,  4*8,  3*8,  2*8,  1*8,  0*8 },
	128*8    /* every sprite takes 128 consecutive bytes */
};

static gfx_layout balllayout =
{
	8,16,  /* 8*16 sprites */
	8,     /* 8 sprites */
	1,  /* 1 bit per pixel */
	{ 0 },
	{ 7, 6, 5, 4, 3, 2, 1, 0 },   /* pretty straightforward layout */
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8,
	  8*8, 9*8, 10*8, 11*8, 12*8, 13*8, 14*8, 15*8 },
	16*8    /* every sprite takes 16 consecutive bytes */
};

static const gfx_decode gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &charlayout, 0,           4  }, /* playfield */
	{ REGION_GFX2, 0, &crowlayout, 4*8,         4  }, /* crow */
	{ REGION_GFX3, 0, &balllayout, 4*8+4*4,     4  }, /* ball 1 */
	{ REGION_GFX4, 0, &balllayout, 4*8+4*4+4*2, 4  }, /* ball 2 */
	{ -1 } /* end of array */
};


static WRITE8_HANDLER( portb_w )
{
	/* don't know what this is... could be a filter */
	if (data != 0x00) logerror("portB = %02x\n",data);
}

static struct AY8910interface ay8910_interface =
{
	0,
	0,
	DAC_0_signed_data_w,
	portb_w
};

static MACHINE_DRIVER_START( bking2 )

	/* basic machine hardware */
	MDRV_CPU_ADD(Z80, 4000000)	/* 4 MHz */
	MDRV_CPU_PROGRAM_MAP(readmem,writemem)
	MDRV_CPU_IO_MAP(readport,writeport)
	MDRV_CPU_VBLANK_INT(irq0_line_hold,1)

	MDRV_CPU_ADD(Z80, 3000000)	/* 3 MHz */
	/* audio CPU */
	MDRV_CPU_PROGRAM_MAP(sound_readmem,sound_writemem)
			/* interrupts (from Jungle King hardware, might be wrong): */
			/* - no interrupts synced with vblank */
			/* - NMI triggered by the main CPU */
			/* - periodic IRQ, with frequency 6000000/(4*16*16*10*16) = 36.621 Hz, */
			/*   that is a period of 27306666.6666 ns */
	MDRV_CPU_PERIODIC_INT(irq0_line_hold,TIME_IN_NSEC(27306667))

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(32*8, 32*8)
	MDRV_VISIBLE_AREA(0*8, 32*8-1, 2*8, 30*8-1)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(512)
	MDRV_COLORTABLE_LENGTH(4*8+4*4+4*2+4*2)

	MDRV_PALETTE_INIT(bking2)
	MDRV_VIDEO_START(bking2)
	MDRV_VIDEO_UPDATE(bking2)
	MDRV_VIDEO_EOF(bking2)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")

	MDRV_SOUND_ADD(AY8910, 2000000)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.25)

	MDRV_SOUND_ADD(AY8910, 2000000)
	MDRV_SOUND_CONFIG(ay8910_interface)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.25)

	MDRV_SOUND_ADD(DAC, 0)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.25)
MACHINE_DRIVER_END

/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START( bking )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )
	ROM_LOAD( "dm_11.f13", 0x0000, 0x1000, CRC(d84fe4f7) SHA1(3ad1641d05e4faca2be28052ccae8f81bc2255bb) )
	ROM_LOAD( "dm_12.f11", 0x1000, 0x1000, CRC(e065bbe6) SHA1(8d6d3334977c1eea1bf238817d59c25acd9d99f0) )
	ROM_LOAD( "dm_13.f10", 0x2000, 0x1000, CRC(aac7cddd) SHA1(12a8887bd8d3334e0d740a7f54374b0e48021140) )
	ROM_LOAD( "dm_14.f8",  0x3000, 0x1000, CRC(1179d074) SHA1(23df9a7e3e1bf42d6ea3a2d85629d27bd68e9af4) )
	ROM_LOAD( "dm_15.f7",  0x4000, 0x1000, CRC(fda31475) SHA1(784ffa089b7bd4ab4cbd454f4c1c26553a11fc48) )
	ROM_LOAD( "dm_16.f5",  0x5000, 0x1000, CRC(b6c3c3ed) SHA1(6c7f67d5eba35e32b556b531e848ef375123de78) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 ) /* Sound ROMs */
	ROM_LOAD( "dm_17.f4",  0x0000, 0x1000, CRC(54840bc3) SHA1(225daf7ff8a4095b0e69ce6ccce6d8eab26ec1c8) )
	ROM_LOAD( "dm_18.d4",  0x1000, 0x1000, CRC(2abadd42) SHA1(d921d333ec9b9140a7d3ce7aaddab35f45fae018) )

	ROM_REGION( 0x6000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "dm_10.a5",  0x0000, 0x1000, CRC(fe96dd67) SHA1(11014602f926cf6edbf06e7b2acef92036b2f30a) )
	ROM_LOAD( "dm_09.a7",  0x1000, 0x1000, CRC(80c675d7) SHA1(e590a71a15ea485abf099eceaa16d5a1dbe0c3dc) )
	ROM_LOAD( "dm_08.a8",  0x2000, 0x1000, CRC(d9bd6b60) SHA1(3c790b6a69472e0a37f45baa00ce5c7d09e7b588) )
	ROM_LOAD( "dm_07.a10", 0x3000, 0x1000, CRC(65f7a0e4) SHA1(034dbf2fe384cb69963936e9f3029aa54e032e4a) )
	ROM_LOAD( "dm_06.a11", 0x4000, 0x1000, CRC(00fdbafc) SHA1(b2a8d9c96415fecee52f1c4691a5f10c96f484b1) )
	ROM_LOAD( "dm_05.a13", 0x5000, 0x1000, CRC(3e4fe925) SHA1(9ed73601c8b34ea8889717cbb3ee4a00ab7ab458) )

	ROM_REGION( 0x0800, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "dm_01.e10", 0x0000, 0x0800, CRC(e5663f0b) SHA1(b0fed8c4cdff7b12bb220e51d5b7188933934a34) ) /* crow graphics */

	ROM_REGION( 0x0800, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_LOAD( "dm_02.e7",  0x0000, 0x0800, CRC(fc9cec31) SHA1(5ab1c9b3b15334c6ec06826005ecb66b34d8879a) ) /* ball 1 graphics. Only the first 128 bytes used */

	ROM_REGION( 0x0800, REGION_GFX4, ROMREGION_DISPOSE )
	ROM_LOAD( "dm_02.e9",  0x0000, 0x0800, CRC(fc9cec31) SHA1(5ab1c9b3b15334c6ec06826005ecb66b34d8879a) ) /* ball 2 graphics. Only the first 128 bytes used */

	ROM_REGION( 0x0020, REGION_USER1, 0 )
	ROM_LOAD( "dm04.c2",   0x0000, 0x0020, CRC(4cb5bd32) SHA1(8851bae033ba67516d5ff6888e5daef10c2116ee) )  /* collision detection */

	ROM_REGION( 0x0200, REGION_PROMS, 0 )
	ROM_LOAD( "dm_03.d1",  0x0000, 0x0200, CRC(61b7a9ff) SHA1(4302de0c0dad2b871ad4719ad934beaee05a0c40) ) /* palette */
ROM_END

ROM_START( bking2 )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )
	ROM_LOAD( "01.13f",       0x0000, 0x1000, CRC(078ada3f) SHA1(5e82a6d27c65fe29d664dbfc2ede547c0f4869f0) )
	ROM_LOAD( "02.11f",       0x1000, 0x1000, CRC(c37d110a) SHA1(7aec6c949d1cf136c3037140bd86597feaf29108) )
	ROM_LOAD( "03.10f",       0x2000, 0x1000, CRC(2ba5c681) SHA1(d0df24f5e52e6162b40308d8aa38b0348a100f37) )
	ROM_LOAD( "04.8f",        0x3000, 0x1000, CRC(8fad54e8) SHA1(55edc185914686d42efd848a402f78884d42292b) )
	ROM_LOAD( "05.7f",        0x4000, 0x1000, CRC(b4de6b58) SHA1(f62bdc3128b226454b1f00a4cbe382e1219a11b0) )
	ROM_LOAD( "06.5f",        0x5000, 0x1000, CRC(9ac43b87) SHA1(dd562fee01c81317978d1bd8a0178e3d9be6145a) )
	ROM_LOAD( "07.4f",        0x6000, 0x1000, CRC(b3ed40b7) SHA1(d481094c0381234314f797928e3cdb22f36f4e32) )
	ROM_LOAD( "08.2f",        0x7000, 0x1000, CRC(8fddb2e8) SHA1(6ee5f09d154440851f370a97b35450e3726e14e7) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )         /* Sound ROMs */
	ROM_LOAD( "15",           0x0000, 0x1000, CRC(f045d0fe) SHA1(3b34081fa6cd0423236d09b6f23e8cf8cfd627c5) )
	ROM_LOAD( "16",           0x1000, 0x1000, CRC(92d50410) SHA1(e6f4c27031744bbc832a1eb121a7dba4da5286c4) )

	ROM_REGION( 0x6000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "14.5a",        0x0000, 0x1000, CRC(52636a94) SHA1(185c4455bd9bb23d14aa2f6f7baa74959da08fc2) )
	ROM_LOAD( "13.7a",        0x1000, 0x1000, CRC(6b9e0564) SHA1(6cdd3820caa3825e98b61fe260960cc05c04d032) )
	ROM_LOAD( "12.8a",        0x2000, 0x1000, CRC(c6d685d9) SHA1(2dd2fda365e6bdf9aa26de90650f4a2588ea0515) )
	ROM_LOAD( "11.10a",       0x3000, 0x1000, CRC(2b949987) SHA1(a94666c4f2fdc25399f7976ed2c25fd454387be6) )
	ROM_LOAD( "10.11a",       0x4000, 0x1000, CRC(eb96f948) SHA1(295ba5a620a8a85a121d3e823804adceeeef64d9) )
	ROM_LOAD( "09.13a",       0x5000, 0x1000, CRC(595e3dd4) SHA1(9dd3388ce704dd5473af034716cd8d48df3dc495) )

	ROM_REGION( 0x0800, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "17",           0x0000, 0x0800, CRC(e5663f0b) SHA1(b0fed8c4cdff7b12bb220e51d5b7188933934a34) )	/* crow graphics */

	ROM_REGION( 0x0800, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_LOAD( "18",           0x0000, 0x0800, CRC(fc9cec31) SHA1(5ab1c9b3b15334c6ec06826005ecb66b34d8879a) )	/* ball 1 graphics. Only the first 128 bytes used */

	ROM_REGION( 0x0800, REGION_GFX4, ROMREGION_DISPOSE )
	ROM_LOAD( "19",           0x0000, 0x0800, CRC(fc9cec31) SHA1(5ab1c9b3b15334c6ec06826005ecb66b34d8879a) )  /* ball 2 graphics. Only the first 128 bytes used */

	ROM_REGION( 0x0020, REGION_USER1, 0 )
	ROM_LOAD( "mb7051.2c",    0x0000, 0x0020, CRC(4cb5bd32) SHA1(8851bae033ba67516d5ff6888e5daef10c2116ee) )  /* collision detection */

	ROM_REGION( 0x0200, REGION_PROMS, 0 )
	ROM_LOAD( "82s141.2d",    0x0000, 0x0200, CRC(61b7a9ff) SHA1(4302de0c0dad2b871ad4719ad934beaee05a0c40) )	/* palette */
ROM_END

ROM_START( bking3 )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )
	ROM_LOAD( "a24-04.13f",   0x0000, 0x1000, CRC(a0c319a6) SHA1(6b79667288113fde43975fcfd05e93d8e45bf92d) )
	ROM_LOAD( "a24-05.11f",   0x1000, 0x1000, CRC(fedc9b4a) SHA1(3ac22c3ca09df9983f3c8c05e807ecf5999c9fc5) )
	ROM_LOAD( "a24-06.10f",   0x2000, 0x1000, CRC(6a116ebf) SHA1(e58b1f75eb75027749a900b27107930e9072ca5a) )
	ROM_LOAD( "a24-07.8f",    0x3000, 0x1000, CRC(75a74d2d) SHA1(d433e8fcf3819b845936e7e107fef414f72bfc16) )
	ROM_LOAD( "a24-08.7f",    0x4000, 0x1000, CRC(9fe07cf9) SHA1(23fdae48e519a171bf4adeeadf2fdfedfd56f4ea) )
	ROM_LOAD( "a24-09.5f",    0x5000, 0x1000, CRC(51545ced) SHA1(4addad527c6fd675506bf584ec8670a23767787c) )
	ROM_LOAD( "a24-01.4f",    0x6000, 0x1000, CRC(a86b3e62) SHA1(f97a13e31e622b5ac55c23458c65a49c2998196a) ) //another one: a24-10.4f
	ROM_LOAD( "a24-11.2f",    0x7000, 0x1000, CRC(b39db430) SHA1(4f48a34f3aaa1e998a4a5656bc3f399d9e6633c4) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )		/* Sound ROMs */
	ROM_LOAD( "a24-18.4f",    0x0000, 0x1000, CRC(fa3bfa98) SHA1(733924e154e301a9d692d80b485afc4ab0e200c1) )
	ROM_LOAD( "a24-19.4d",    0x1000, 0x1000, CRC(817f9c2a) SHA1(7365ecf2700e1fd13016408f5493f8d51aab5bbd) )
	ROM_LOAD( "a24-20.4b",    0x2000, 0x1000, CRC(0e9e16d6) SHA1(43c69602a8d9c34c527ce54472db84168acc4ef4) )

	ROM_REGION( 0x0800, REGION_CPU3, 0 )	/* 2k for the microcontroller */
	ROM_LOAD( "a24_22",    0x0000,  0x0800, NO_DUMP ) //M68705P5S uC 3MHz xtal

	ROM_REGION( 0x6000, REGION_GFX1, ROMREGION_DISPOSE ) /* Tiles */
	ROM_LOAD( "a24-12.5a",    0x0000, 0x1000, CRC(c5fe4817) SHA1(fbf82d9d85e18b76c7e939932df074a545e73f42) )
	ROM_LOAD( "a24-13.7a",    0x1000, 0x1000, CRC(728bac57) SHA1(3daa246f95b31c971e5418f55b821616d0bce25d) )
	ROM_LOAD( "a24-14.8a",    0x2000, 0x1000, CRC(63cd0009) SHA1(10fcfeec70b23e2206c4f4bf686dc6a48ecba1ce) )
	ROM_LOAD( "a24-15.10a",   0x3000, 0x1000, CRC(590275d0) SHA1(563bebb344c606ca3a2124fc7a8804935a011e90) )
	ROM_LOAD( "a24-16.11a",   0x4000, 0x1000, CRC(728d069e) SHA1(b4adb14281e4874bab7cec7f38ade70b5b7c6b8f) )
	ROM_LOAD( "a24-17.13a",   0x5000, 0x1000, CRC(4c04c4f2) SHA1(8e9eee6d89e91910b398d42ac86597ef91baad96) )

	ROM_REGION( 0x0800, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "a24-02.10e",   0x0000, 0x0800, CRC(8560da46) SHA1(56f249f0b56336daac1a3624ef9b71354bb8ca40) ) /* crow graphics */

	ROM_REGION( 0x0800, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_LOAD( "a24-01.7e",    0x0000, 0x0800, CRC(369c01e1) SHA1(196e12d0bcaf74cefe4cad3fccb69d104aab061e) ) /* ball 1 graphics. Only the first 128 bytes used */

	ROM_REGION( 0x0800, REGION_GFX4, ROMREGION_DISPOSE )
	ROM_LOAD( "a24-01.9e",    0x0000, 0x0800, CRC(369c01e1) SHA1(196e12d0bcaf74cefe4cad3fccb69d104aab061e) ) /* ball 2 graphics. Only the first 128 bytes used */

	ROM_REGION( 0x0020, REGION_USER1, 0 )
	ROM_LOAD( "82s123.2c",    0x0000, 0x0020, CRC(4cb5bd32) SHA1(8851bae033ba67516d5ff6888e5daef10c2116ee) ) /* collision detection */

	ROM_REGION( 0x0200, REGION_PROMS, 0 )
	ROM_LOAD( "82s141.2d",    0x0000, 0x0200, CRC(61b7a9ff) SHA1(4302de0c0dad2b871ad4719ad934beaee05a0c40) )	/* palette */

	ROM_REGION( 0x1000, REGION_USER2, 0 )
	ROM_LOAD( "a24-21.25",    0x0000, 0x1000, CRC(3106fcac) SHA1(08454adfb58e5df84140d86ed52fa4ef684df9f1) ) /* extra rom on the same SUB PCB where is the mcu */

//missing?
//"a24_03"  A24_03 - TI TBP28S46N - Not read
//"a24_04"  A24_04 - Bipolar PROM - Not read

ROM_END

GAME( 1982, bking,  0, bking2, bking,  0, ROT270, "Taito Corporation", "Birdie King", 0 )
GAME( 1983, bking2, 0, bking2, bking2, 0, ROT90,  "Taito Corporation", "Birdie King 2", 0 )
GAME( 1984, bking3, 0, bking2, bking2, 0, ROT90,  "Taito Corporation", "Birdie King 3", 0 )
