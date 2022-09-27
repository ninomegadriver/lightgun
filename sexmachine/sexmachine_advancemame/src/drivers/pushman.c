/***************************************************************************

    Pushman                         (c) 1990 Comad

    With 'Debug Mode' on button 2 advances a level, button 3 goes back.

    The microcontroller mainly controls the animation of the enemy robots,
    the communication between the 68000 and MCU is probably not emulated
    100% correct but it works.  Later levels (using the cheat mode) seem
    to have some corrupt tilemaps, I'm not sure if this is a driver bug
    or a game bug from using the cheat mode.

    Emulation by Bryan McPhail, mish@tendril.co.uk

    The hardware is actually very similar to F1-Dream and Tiger Road but
    with a 68705 for protection.

 **************************************************************************

    Bouncing Balls                      (c) 1991 Comad

***************************************************************************/

#include "driver.h"
#include "cpu/z80/z80.h"
#include "cpu/m6805/m6805.h"
#include "sound/2203intf.h"

VIDEO_UPDATE( pushman );
WRITE16_HANDLER( pushman_scroll_w );
WRITE16_HANDLER( pushman_videoram_w );
VIDEO_START( pushman );

static UINT8 shared_ram[8];
static UINT16 latch,new_latch=0;

/******************************************************************************/

static WRITE16_HANDLER( pushman_control_w )
{
	if (ACCESSING_MSB)
		soundlatch_w(0,(data>>8)&0xff);
}

static READ16_HANDLER( pushman_68705_r )
{
	if (offset==0)
		return latch;

	if (offset==3 && new_latch) { new_latch=0; return 0; }
	if (offset==3 && !new_latch) return 0xff;

	return (shared_ram[2*offset+1]<<8)+shared_ram[2*offset];
}

static WRITE16_HANDLER( pushman_68705_w )
{
	if (ACCESSING_MSB)
		shared_ram[2*offset]=data>>8;
	if (ACCESSING_LSB)
		shared_ram[2*offset+1]=data&0xff;

	if (offset==1)
	{
        cpunum_set_input_line(2,M68705_IRQ_LINE,HOLD_LINE);
		cpu_spin();
		new_latch=0;
	}
}

/* ElSemi - Bouncing balls protection. */
static READ16_HANDLER( bballs_68705_r )
{
	if (offset==0)
		return latch;
	if(offset==3 && new_latch)
	{
        	new_latch=0;
		return 0;
	}
	if(offset==3 && !new_latch)
		return 0xff;

	return (shared_ram[2*offset+1]<<8)+shared_ram[2*offset];
}

static WRITE16_HANDLER( bballs_68705_w )
{
	if (ACCESSING_MSB)
		shared_ram[2*offset]=data>>8;
	if (ACCESSING_LSB)
		shared_ram[2*offset+1]=data&0xff;

	if(offset==0)
	{
		latch=0;
		if(shared_ram[0]<=0xf)
		{
			latch=shared_ram[0]<<2;
			if(shared_ram[1])
				latch|=2;
			new_latch=1;
		}
		else if(shared_ram[0])
		{
			if(shared_ram[1])
				latch|=2;
			new_latch=1;
		}
	}
}


static READ8_HANDLER( pushman_68000_r )
{
	return shared_ram[offset];
}

static WRITE8_HANDLER( pushman_68000_w )
{
	if (offset==2 && (shared_ram[2]&2)==0 && data&2) {
		latch=(shared_ram[1]<<8)|shared_ram[0];
		new_latch=1;
	}
	shared_ram[offset]=data;
}

MACHINE_RESET( bballs )
{
	latch=0x400;
}

/******************************************************************************/

static ADDRESS_MAP_START( readmem, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x01ffff) AM_READ(MRA16_ROM)
	AM_RANGE(0x060000, 0x060007) AM_READ(pushman_68705_r)
	AM_RANGE(0xfe0800, 0xfe17ff) AM_READ(MRA16_RAM)
	AM_RANGE(0xfe4000, 0xfe4001) AM_READ(input_port_0_word_r)
	AM_RANGE(0xfe4002, 0xfe4003) AM_READ(input_port_1_word_r)
	AM_RANGE(0xfe4004, 0xfe4005) AM_READ(input_port_2_word_r)
	AM_RANGE(0xfec000, 0xfec7ff) AM_READ(MRA16_RAM)
	AM_RANGE(0xff8000, 0xff87ff) AM_READ(MRA16_RAM)
	AM_RANGE(0xffc000, 0xffffff) AM_READ(MRA16_RAM)
ADDRESS_MAP_END

static ADDRESS_MAP_START( writemem, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x01ffff) AM_WRITE(MWA16_ROM)
	AM_RANGE(0x060000, 0x060007) AM_WRITE(pushman_68705_w)
	AM_RANGE(0xfe0800, 0xfe17ff) AM_WRITE(MWA16_RAM) AM_BASE(&spriteram16)
	AM_RANGE(0xfe4002, 0xfe4003) AM_WRITE(pushman_control_w)
	AM_RANGE(0xfe8000, 0xfe8003) AM_WRITE(pushman_scroll_w)
	AM_RANGE(0xfe800e, 0xfe800f) AM_WRITE(MWA16_NOP) /* ? */
	AM_RANGE(0xfec000, 0xfec7ff) AM_WRITE(pushman_videoram_w) AM_BASE(&videoram16)
	AM_RANGE(0xff8000, 0xff87ff) AM_WRITE(paletteram16_xxxxRRRRGGGGBBBB_word_w) AM_BASE(&paletteram16)
	AM_RANGE(0xffc000, 0xffffff) AM_WRITE(MWA16_RAM)
ADDRESS_MAP_END

static ADDRESS_MAP_START( mcu_readmem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x0007) AM_READ(pushman_68000_r)
	AM_RANGE(0x0010, 0x007f) AM_READ(MRA8_RAM)
	AM_RANGE(0x0080, 0x0fff) AM_READ(MRA8_ROM)
ADDRESS_MAP_END

static ADDRESS_MAP_START( mcu_writemem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x0007) AM_WRITE(pushman_68000_w)
	AM_RANGE(0x0010, 0x007f) AM_WRITE(MWA8_RAM)
	AM_RANGE(0x0080, 0x0fff) AM_WRITE(MWA8_ROM)
ADDRESS_MAP_END

static ADDRESS_MAP_START( sound_readmem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x7fff) AM_READ(MRA8_ROM)
	AM_RANGE(0xc000, 0xc7ff) AM_READ(MRA8_RAM)
	AM_RANGE(0xe000, 0xe000) AM_READ(soundlatch_r)
ADDRESS_MAP_END

static ADDRESS_MAP_START( sound_writemem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x7fff) AM_WRITE(MWA8_ROM)
	AM_RANGE(0xc000, 0xc7ff) AM_WRITE(MWA8_RAM)
ADDRESS_MAP_END

static ADDRESS_MAP_START( sound_writeport, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_FLAGS( AMEF_ABITS(8) )
	AM_RANGE(0x00, 0x00) AM_WRITE(YM2203_control_port_0_w)
	AM_RANGE(0x01, 0x01) AM_WRITE(YM2203_write_port_0_w)
	AM_RANGE(0x80, 0x80) AM_WRITE(YM2203_control_port_1_w)
	AM_RANGE(0x81, 0x81) AM_WRITE(YM2203_write_port_1_w)
ADDRESS_MAP_END

static ADDRESS_MAP_START( bballs_readmem, ADDRESS_SPACE_PROGRAM, 16 )
	ADDRESS_MAP_FLAGS( AMEF_ABITS(20) )
	AM_RANGE(0x00000, 0x1ffff) AM_READ(MRA16_ROM)
	AM_RANGE(0x60000, 0x60007) AM_READ(bballs_68705_r)
	AM_RANGE(0xe0800, 0xe17ff) AM_READ(MRA16_RAM)
	AM_RANGE(0xe4000, 0xe4001) AM_READ(input_port_0_word_r)
	AM_RANGE(0xe4002, 0xe4003) AM_READ(input_port_1_word_r)
	AM_RANGE(0xe4004, 0xe4005) AM_READ(input_port_2_word_r)
	AM_RANGE(0xec000, 0xec7ff) AM_READ(MRA16_RAM)
	AM_RANGE(0xf8000, 0xf87ff) AM_READ(MRA16_RAM)
	AM_RANGE(0xfc000, 0xfffff) AM_READ(MRA16_RAM)
ADDRESS_MAP_END

static ADDRESS_MAP_START( bballs_writemem, ADDRESS_SPACE_PROGRAM, 16 )
	ADDRESS_MAP_FLAGS( AMEF_ABITS(20) )
	AM_RANGE(0x00000, 0x1ffff) AM_WRITE(MWA16_ROM)
	AM_RANGE(0x60000, 0x60007) AM_WRITE(bballs_68705_w)
	AM_RANGE(0xe0800, 0xe17ff) AM_WRITE(MWA16_RAM) AM_BASE(&spriteram16)
	AM_RANGE(0xe4002, 0xe4003) AM_WRITE(pushman_control_w)
	AM_RANGE(0xe8000, 0xe8003) AM_WRITE(pushman_scroll_w)
	AM_RANGE(0xe800e, 0xe800f) AM_WRITE(MWA16_NOP) /* ? */
	AM_RANGE(0xec000, 0xec7ff) AM_WRITE(pushman_videoram_w) AM_BASE(&videoram16)
	AM_RANGE(0xf8000, 0xf87ff) AM_WRITE(paletteram16_xxxxRRRRGGGGBBBB_word_w) AM_BASE(&paletteram16)
	AM_RANGE(0xfc000, 0xfffff) AM_WRITE(MWA16_RAM)
ADDRESS_MAP_END

/******************************************************************************/

INPUT_PORTS_START( pushman )
	PORT_START_TAG("IN0")
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(1)
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(1)
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_PLAYER(1)
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(2)
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(2)
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_PLAYER(2)
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START_TAG("IN1")
	PORT_BIT( 0x00ff, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_VBLANK ) /* not sure, probably wrong */
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_COIN2 )

	PORT_START_TAG("IN2")
	PORT_DIPNAME( 0x0001, 0x0001, "Debug Mode (Cheat)") /* Listed as "Screen Skip" */
	PORT_DIPSETTING(      0x0001, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0002, 0x0002, "Pull Option" )
	PORT_DIPSETTING(      0x0002, "5" )
	PORT_DIPSETTING(      0x0000, "9" )
	PORT_DIPNAME( 0x0004, 0x0004, DEF_STR( Level_Select ) )
	PORT_DIPSETTING(      0x0004, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0008, 0x0008, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(      0x0008, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0010, 0x0010, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0010, DEF_STR( On ) )
	PORT_SERVICE( 0x0020, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0x0040, 0x0040, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0040, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0080, 0x0080, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0080, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0700, 0x0700, DEF_STR( Coinage ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( 5C_1C ) )
	PORT_DIPSETTING(      0x0100, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(      0x0200, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(      0x0300, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0x0700, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x0600, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(      0x0500, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(      0x0400, DEF_STR( 1C_4C ) )
	PORT_DIPNAME( 0x0800, 0x0800, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(      0x0800, DEF_STR( Easy ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( Hard ) )
	PORT_DIPNAME( 0x1000, 0x1000, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x1000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x2000, 0x2000, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x2000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x4000, 0x4000, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x4000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x8000, 0x8000, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x8000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
INPUT_PORTS_END

INPUT_PORTS_START( bballs )
	PORT_START_TAG("IN0")
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(1) PORT_NAME("P1 Open/Close Gate")	// Open/Close gate
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(1) PORT_NAME("P1 Zap")	// Use Zap
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_UNKNOWN )	// BUTTON3 in "test mode"
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(2) PORT_NAME("P2 Open/Close Gate")	// Open/Close gate
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(2) PORT_NAME("P2 Zap")	// Use Zap
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_UNKNOWN ) // BUTTON3 in "test mode"

	PORT_START_TAG("IN1")
	PORT_BIT( 0x00ff, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_VBLANK ) /* not sure, probably wrong */
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_COIN2 )

	PORT_START_TAG("IN2")
	PORT_DIPNAME( 0x0007, 0x0007, DEF_STR( Coinage ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( 5C_1C ) )
	PORT_DIPSETTING(      0x0001, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(      0x0002, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(      0x0003, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0x0007, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x0006, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(      0x0005, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(      0x0004, DEF_STR( 1C_4C ) )
	PORT_DIPNAME( 0x0008, 0x0008, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(      0x0008, DEF_STR( Easy ) )			// less bubbles before cycling
	PORT_DIPSETTING(      0x0000, DEF_STR( Hard ) )			// more bubbles before cycling
	PORT_DIPNAME( 0x0010, 0x0000, "Music (In-game)" )
	PORT_DIPSETTING(      0x0010, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0020, 0x0000, "Music (Attract Mode)" )
	PORT_DIPSETTING(      0x0020, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x00c0, 0x00c0, DEF_STR( Lives ) )
	PORT_DIPSETTING(      0x00c0, "1" )
	PORT_DIPSETTING(      0x0080, "2" )
	PORT_DIPSETTING(      0x0040, "3" )
	PORT_DIPSETTING(      0x0000, "4" )
	PORT_DIPNAME( 0x0100, 0x0100, "Zaps" )
	PORT_DIPSETTING(      0x0100, "1" )
	PORT_DIPSETTING(      0x0000, "2" )
	PORT_DIPNAME( 0x0200, 0x0000, "Display Next Ball" )
	PORT_DIPSETTING(      0x0200, DEF_STR( No ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x0400, 0x0400, DEF_STR( Unused ) )
	PORT_DIPSETTING(      0x0400, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0800, 0x0800, DEF_STR( Unused ) )
	PORT_DIPSETTING(      0x0800, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x1000, 0x1000, DEF_STR( Unused ) )
	PORT_DIPSETTING(      0x1000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x2000, 0x2000, DEF_STR( Unknown ) )	// code at 0x0054ac, 0x0054f2, 0x0056fc
	PORT_DIPSETTING(      0x2000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0xc000, 0xc000, DEF_STR( Service_Mode ) )
	PORT_DIPSETTING(      0xc000, DEF_STR( Off ) )
//  PORT_DIPSETTING(      0x8000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x4000, "Inputs/Outputs" )
	PORT_DIPSETTING(      0x0000, "Graphics" )
INPUT_PORTS_END

/******************************************************************************/

static const gfx_layout charlayout =
{
	8,8,
	RGN_FRAC(1,1),
	2,
	{ 4, 0 },
	{ 0, 1, 2, 3, 8+0, 8+1, 8+2, 8+3 },
	{ 0*16, 1*16, 2*16, 3*16, 4*16, 5*16, 6*16, 7*16 },
	16*8
};

static const gfx_layout spritelayout =
{
	16,16,
	RGN_FRAC(1,4),
	4,
	{ RGN_FRAC(0,4), RGN_FRAC(1,4), RGN_FRAC(2,4), RGN_FRAC(3,4) },
	{ 0, 1, 2, 3, 4, 5, 6, 7,
			16*8+0, 16*8+1, 16*8+2, 16*8+3, 16*8+4, 16*8+5, 16*8+6, 16*8+7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8,
			8*8, 9*8, 10*8, 11*8, 12*8, 13*8, 14*8, 15*8 },
	32*8
};

static const gfx_layout tilelayout =
{
	32,32,
	RGN_FRAC(1,2),
	4,
	{ 4, 0, RGN_FRAC(1,2)+4, RGN_FRAC(1,2)+0 },
	{ 0, 1, 2, 3, 8+0, 8+1, 8+2, 8+3,
			64*8+0, 64*8+1, 64*8+2, 64*8+3, 65*8+0, 65*8+1, 65*8+2, 65*8+3,
			128*8+0, 128*8+1, 128*8+2, 128*8+3, 129*8+0, 129*8+1, 129*8+2, 129*8+3,
			192*8+0, 192*8+1, 192*8+2, 192*8+3, 193*8+0, 193*8+1, 193*8+2, 193*8+3 },
	{ 0*16, 1*16, 2*16, 3*16, 4*16, 5*16, 6*16, 7*16,
			8*16, 9*16, 10*16, 11*16, 12*16, 13*16, 14*16, 15*16,
			16*16, 17*16, 18*16, 19*16, 20*16, 21*16, 22*16, 23*16,
			24*16, 25*16, 26*16, 27*16, 28*16, 29*16, 30*16, 31*16 },
	256*8
};

static const gfx_decode pushman_gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0x000000, &charlayout,   0x300, 16 },	/* colors 0x300-0x33f */
	{ REGION_GFX2, 0x000000, &spritelayout, 0x200, 16 },	/* colors 0x200-0x2ff */
	{ REGION_GFX3, 0x000000, &tilelayout,   0x100, 16 },	/* colors 0x100-0x1ff */
	{ -1 } /* end of array */
};

/******************************************************************************/

static void irqhandler(int irq)
{
    cpunum_set_input_line(1,0,irq ? ASSERT_LINE : CLEAR_LINE);
}

static struct YM2203interface ym2203_interface =
{
	0,0,0,0,irqhandler
};


static MACHINE_DRIVER_START( pushman )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000, 8000000)
	MDRV_CPU_PROGRAM_MAP(readmem,writemem)
	MDRV_CPU_VBLANK_INT(irq2_line_hold,1)

	MDRV_CPU_ADD(Z80, 4000000)
	/* audio CPU */
	MDRV_CPU_PROGRAM_MAP(sound_readmem,sound_writemem)
	MDRV_CPU_IO_MAP(0,sound_writeport)

	/* ElSemi. Reversed the CPU order so the sound callback works with bballs */
	MDRV_CPU_ADD(M68705, 400000)	/* No idea */
	MDRV_CPU_PROGRAM_MAP(mcu_readmem,mcu_writemem)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_REAL_60HZ_VBLANK_DURATION)
	MDRV_INTERLEAVE(60)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(32*8, 32*8)
	MDRV_VISIBLE_AREA(0*8, 32*8-1, 2*8, 30*8-1)
	MDRV_GFXDECODE(pushman_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(1024)

	MDRV_VIDEO_START(pushman)
	MDRV_VIDEO_UPDATE(pushman)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")

	MDRV_SOUND_ADD(YM2203, 2000000)
	MDRV_SOUND_CONFIG(ym2203_interface)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.40)

	MDRV_SOUND_ADD(YM2203, 2000000)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.40)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( bballs )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000, 8000000)
	MDRV_CPU_PROGRAM_MAP(bballs_readmem,bballs_writemem)
	MDRV_CPU_VBLANK_INT(irq2_line_hold,1)

	MDRV_CPU_ADD(Z80, 4000000)
	/* audio CPU */
	MDRV_CPU_PROGRAM_MAP(sound_readmem,sound_writemem)
	MDRV_CPU_IO_MAP(0,sound_writeport)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_REAL_60HZ_VBLANK_DURATION)
	MDRV_INTERLEAVE(60)

	MDRV_MACHINE_RESET(bballs)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(32*8, 32*8)
	MDRV_VISIBLE_AREA(0*8, 32*8-1, 2*8, 30*8-1)
	MDRV_GFXDECODE(pushman_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(1024)

	MDRV_VIDEO_START(pushman)
	MDRV_VIDEO_UPDATE(pushman)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")

	MDRV_SOUND_ADD(YM2203, 2000000)
	MDRV_SOUND_CONFIG(ym2203_interface)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.40)

	MDRV_SOUND_ADD(YM2203, 2000000)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.40)
MACHINE_DRIVER_END


/***************************************************************************/

ROM_START( pushman )
	ROM_REGION( 0x20000, REGION_CPU1, 0 )
	ROM_LOAD16_BYTE( "pushman.012", 0x000000, 0x10000, CRC(330762bc) SHA1(c769b68da40183e6eb84212636bfd1265e5ed2d8) )
	ROM_LOAD16_BYTE( "pushman.011", 0x000001, 0x10000, CRC(62636796) SHA1(1a205c1b0efff4158439bc9a21cfe3cd8834aef9) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )
	ROM_LOAD( "pushman.013", 0x00000, 0x08000,  CRC(adfe66c1) SHA1(fa4ed13d655c664b06e9b91292d2c0a88cb5a569) )

	ROM_REGION( 0x01000, REGION_CPU3, 0 ) /* not from this set, check behavior is the same */
	ROM_LOAD( "pushman.uc",  0x00000, 0x01000, BAD_DUMP CRC(d7916657) SHA1(89c14c6044f082fffe2a8f86d0a82336f4a110a2) )

	ROM_REGION( 0x10000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "pushman.001",  0x00000, 0x08000, CRC(626e5865) SHA1(4ab96c8512f439d18390094d71a898f5c576399c) )

	ROM_REGION( 0x40000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "pushman.004", 0x00000, 0x10000, CRC(87aafa70) SHA1(560661b23ddac106a3d2762fc32da666b31e7424) )
	ROM_LOAD( "pushman.005", 0x10000, 0x10000, CRC(7fd1200c) SHA1(15d6781a2d7e3ec2e8f85f8585b1e3fd9fe4fd1d) )
	ROM_LOAD( "pushman.002", 0x20000, 0x10000, CRC(0a094ab0) SHA1(2ff5dcf0d9439eeadd61601170c9767f4d81f022) )
	ROM_LOAD( "pushman.003", 0x30000, 0x10000, CRC(73d1f29d) SHA1(0a87fe02b1efd04c540f016b2626d32da70219db) )

	ROM_REGION( 0x40000, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_LOAD( "pushman.006", 0x00000, 0x10000, CRC(48ef3da6) SHA1(407d50c2030584bb17a4d4a1bb45e0b04e1a95a4) )
	ROM_LOAD( "pushman.008", 0x10000, 0x10000, CRC(4b6a3e88) SHA1(c57d0528e942dd77a13e5a4bf39053f52915d44c) )
	ROM_LOAD( "pushman.007", 0x20000, 0x10000, CRC(b70020bd) SHA1(218ca4a08b87b7dc5c1eed99960f4098c4fc7e0c) )
	ROM_LOAD( "pushman.009", 0x30000, 0x10000, CRC(cc555667) SHA1(6c79e14fc18d1d836392044779cb3219494a3447) )

	ROM_REGION( 0x10000, REGION_GFX4, 0 )	/* bg tilemaps */
	ROM_LOAD( "pushman.010", 0x00000, 0x08000, CRC(a500132d) SHA1(26b02c9fea69b51c5f7dc1b43b838cd336ebf862) )
ROM_END

ROM_START( pushmana )
	ROM_REGION( 0x20000, REGION_CPU1, 0 )
	ROM_LOAD16_BYTE( "pushmana.212", 0x000000, 0x10000, CRC(871d0858) SHA1(690ca554c8c6f19c0f26ccd8d948e3aa6e1b23c0) )
	ROM_LOAD16_BYTE( "pushmana.011", 0x000001, 0x10000, CRC(ae57761e) SHA1(63467d61c967f38e0fbb8130f08e09e03cdcce6c) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )
	ROM_LOAD( "pushman.013", 0x00000, 0x08000,  CRC(adfe66c1) SHA1(fa4ed13d655c664b06e9b91292d2c0a88cb5a569) ) // missing from this set?

	ROM_REGION( 0x01000, REGION_CPU3, 0 ) /* not from this set, check behavior is the same */
	ROM_LOAD( "pushman.uc",  0x00000, 0x01000, BAD_DUMP CRC(d7916657) SHA1(89c14c6044f082fffe2a8f86d0a82336f4a110a2) )

	ROM_REGION( 0x10000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "pushmana.130",  0x00000, 0x10000, CRC(f83f92e7) SHA1(37f337d7b496f8d81eed247c80390a6aabcf4b95) )

	ROM_REGION( 0x40000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "pushman.004", 0x00000, 0x10000, CRC(87aafa70) SHA1(560661b23ddac106a3d2762fc32da666b31e7424) ) // .58
	ROM_LOAD( "pushman.005", 0x10000, 0x10000, CRC(7fd1200c) SHA1(15d6781a2d7e3ec2e8f85f8585b1e3fd9fe4fd1d) ) // .59
	ROM_LOAD( "pushman.002", 0x20000, 0x10000, CRC(0a094ab0) SHA1(2ff5dcf0d9439eeadd61601170c9767f4d81f022) ) // .56
	ROM_LOAD( "pushman.003", 0x30000, 0x10000, CRC(73d1f29d) SHA1(0a87fe02b1efd04c540f016b2626d32da70219db) ) // .57

	ROM_REGION( 0x40000, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_LOAD( "pushman.006", 0x00000, 0x10000, CRC(48ef3da6) SHA1(407d50c2030584bb17a4d4a1bb45e0b04e1a95a4) ) // .131
	ROM_LOAD( "pushman.008", 0x10000, 0x10000, CRC(4b6a3e88) SHA1(c57d0528e942dd77a13e5a4bf39053f52915d44c) ) // .148
	ROM_LOAD( "pushman.007", 0x20000, 0x10000, CRC(b70020bd) SHA1(218ca4a08b87b7dc5c1eed99960f4098c4fc7e0c) ) // .132
	ROM_LOAD( "pushman.009", 0x30000, 0x10000, CRC(cc555667) SHA1(6c79e14fc18d1d836392044779cb3219494a3447) ) // .149

	ROM_REGION( 0x10000, REGION_GFX4, 0 )	/* bg tilemaps */
	ROM_LOAD( "pushmana.189", 0x00000, 0x10000, CRC(59f25598) SHA1(ace33afd6e6d07376ed01048db99b13bcec790d7) )
ROM_END

ROM_START( pushmans )
	ROM_REGION( 0x20000, REGION_CPU1, 0 )
	ROM_LOAD16_BYTE( "pman-12.212", 0x000000, 0x10000, CRC(4251109d) SHA1(d4b020e4ecc2005b3a4c1b34d88de82b09bf5a6b) )
	ROM_LOAD16_BYTE( "pman-11.197", 0x000001, 0x10000, CRC(1167ed9f) SHA1(ca0296950a75ef15ff6f9d3a776b02180b941d61) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )
	ROM_LOAD( "pman-13.216", 0x00000, 0x08000, CRC(bc03827a) SHA1(b4d6ae164bbb7ba19e4934392fe2ba29575f28b9) )

	ROM_REGION( 0x01000, REGION_CPU3, 0 )
	ROM_LOAD( "pushman.uc",  0x00000, 0x01000, CRC(d7916657) SHA1(89c14c6044f082fffe2a8f86d0a82336f4a110a2) )

	ROM_REGION( 0x10000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "pman-1.130",  0x00000, 0x08000, CRC(14497754) SHA1(a47d03c56add18c5d9aed221990550b18589ff43) )

	ROM_REGION( 0x40000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "pman-4.58", 0x00000, 0x10000, CRC(16e5ce6b) SHA1(cb9c6094a853abc550eae29c35083f26a0d1de94) )
	ROM_LOAD( "pman-5.59", 0x10000, 0x10000, CRC(b82140b8) SHA1(0a16b904eb2739bfa22a87d03266d3ff2b750b67) )
	ROM_LOAD( "pman-2.56", 0x20000, 0x10000, CRC(2cb2ac29) SHA1(165447ad7eb8593c0d4346096ec13ac386e905c9) )
	ROM_LOAD( "pman-3.57", 0x30000, 0x10000, CRC(8ab957c8) SHA1(143501819920c521353930f83e49f1e19fbba34f) )

	ROM_REGION( 0x40000, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_LOAD( "pman-6.131", 0x00000, 0x10000, CRC(bd0f9025) SHA1(7262410d4631f1b051c605d5cea5b91e9f68327e) )
	ROM_LOAD( "pman-8.148", 0x10000, 0x10000, CRC(591bd5c0) SHA1(6e0e18e0912fa38e113420ac31c7f36853b830ec) )
	ROM_LOAD( "pman-7.132", 0x20000, 0x10000, CRC(208cb197) SHA1(161633b6b0acf25447a5c0b3c6fbf18adc6e2243) )
	ROM_LOAD( "pman-9.149", 0x30000, 0x10000, CRC(77ee8577) SHA1(63d13683dd097d8e7cb71ad3abe04e11f2a58bd3) )

	ROM_REGION( 0x10000, REGION_GFX4, 0 )	/* bg tilemaps */
	ROM_LOAD( "pman-10.189", 0x00000, 0x08000, CRC(5f9ae9a1) SHA1(87619918c28c942780f6dbd3818d4cc69932eefc) )
ROM_END

ROM_START( bballs )
	ROM_REGION( 0x20000, REGION_CPU1, 0 )
	ROM_LOAD16_BYTE( "bb12.m17", 0x000000, 0x10000, CRC(4501c245) SHA1(b03ace135b077e8c226dd3be04fa8e86ad096770) )
	ROM_LOAD16_BYTE( "bb11.l17", 0x000001, 0x10000, CRC(55e45b60) SHA1(103d848ae74b59ac2f5a5c5300323bbf8b109752) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )
	ROM_LOAD( "bb13.n4", 0x00000, 0x08000, CRC(1ef78175) SHA1(2e7dcbab3a572c2a6bb67a36ba283a5faeb14a88) )

	ROM_REGION( 0x01000, REGION_CPU3, 0 )
	ROM_LOAD( "68705.uc",  0x00000, 0x01000, NO_DUMP )

	ROM_REGION( 0x10000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "bb1.g20",  0x00000, 0x08000, CRC(b62dbcb8) SHA1(121613f6d2bcd226e71d4ae71830b9b0d15c2331) )

	ROM_REGION( 0x40000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "bb4.d1", 0x00000, 0x10000, CRC(b77de5f8) SHA1(e966f982d712109c4402ca3a8cd2c19640d52bdb) )
	ROM_LOAD( "bb5.d2", 0x10000, 0x10000, CRC(ffffccbf) SHA1(3ac85c06c3dca1de8839fca73f5de3982a3baca0) )
	ROM_LOAD( "bb2.b1", 0x20000, 0x10000, CRC(a5b13236) SHA1(e2d21fa3c878b328238ba8b400f3ab00b0763f6b) )
	ROM_LOAD( "bb3.b2", 0x30000, 0x10000, CRC(e35b383d) SHA1(5312e80d786dc2ffe0f7b1038a64f8ec6e590e0c) )

	ROM_REGION( 0x40000, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_LOAD( "bb6.h1", 0x00000, 0x10000, CRC(0cada9ce) SHA1(5f2e85baf5f04e874e0857451946c8b1e1c8d209) )
	ROM_LOAD( "bb8.j1", 0x10000, 0x10000, CRC(d55fe7c1) SHA1(de5ba87c0f905e6f1abadde3af63884a8a130806) )
	ROM_LOAD( "bb7.h2", 0x20000, 0x10000, CRC(a352d53b) SHA1(c71e976b7c28630d7af11fffe0d1cfd7d611ee8b) )
	ROM_LOAD( "bb9.j2", 0x30000, 0x10000, CRC(78d185ac) SHA1(6ed6e1f5eeb93129eeeab6bae22b640c9782f7fc) )

	ROM_REGION( 0x10000, REGION_GFX4, 0 )	/* bg tilemaps */
	ROM_LOAD( "bb10.l6", 0x00000, 0x08000, CRC(d06498f9) SHA1(9f33bbc40ebe11c03aec29289f76f1c3ca5bf009) )

	ROM_REGION( 0x0100, REGION_PROMS, 0 ) /* this is the same as tiger road / f1-dream */
	ROM_LOAD( "bb_prom.e9",   0x0000, 0x0100, CRC(ec80ae36) SHA1(397ec8fc1b106c8b8d4bf6798aa429e8768a101a) )	/* priority (not used) */
ROM_END

GAME( 1990, pushman,  0,       pushman, pushman, 0, ROT0, "Comad", "Pushman (Korea, set 1)", 0 )
GAME( 1990, pushmana, pushman, pushman, pushman, 0, ROT0, "Comad", "Pushman (Korea, set 2)", 0 )
GAME( 1990, pushmans, pushman, pushman, pushman, 0, ROT0, "Comad (American Sammy license)", "Pushman (American Sammy license)", 0 )
GAME( 1991, bballs,   0,       bballs,  bballs,  0, ROT0, "Comad", "Bouncing Balls", 0 )
