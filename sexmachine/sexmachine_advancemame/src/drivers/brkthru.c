/***************************************************************************
Break Thru Doc. Data East (1986)

driver by Phil Stroffolino

UNK-1.1    (16Kb)  Code (4000-7FFF)
UNK-1.2    (32Kb)  Main 6809 (8000-FFFF)
UNK-1.3    (32Kb)  Mapped (2000-3FFF)
UNK-1.4    (32Kb)  Mapped (2000-3FFF)

UNK-1.5    (32Kb)  Sound 6809 (8000-FFFF)

Background has 4 banks, with 256 16x16x8 tiles in each bank.
UNK-1.6    (32Kb)  GFX Background
UNK-1.7    (32Kb)  GFX Background
UNK-1.8    (32Kb)  GFX Background

UNK-1.9    (32Kb)  GFX Sprites
UNK-1.10   (32Kb)  GFX Sprites
UNK-1.11   (32Kb)  GFX Sprites

Text has 256 8x8x4 characters.
UNK-1.12   (8Kb)   GFX Text

**************************************************************************
Memory Map for Main CPU by Carlos A. Lozano
**************************************************************************

MAIN CPU
0000-03FF                                   W                   Plane0
0400-0BFF                                  R/W                  RAM
0C00-0FFF                                   W                   Plane2(Background)
1000-10FF                                   W                   Plane1(sprites)
1100-17FF                                  R/W                  RAM
1800-180F                                  R/W                  In/Out
1810-1FFF                                  R/W                  RAM (unmapped?)
2000-3FFF                                   R                   ROM Mapped(*)
4000-7FFF                                   R                   ROM(UNK-1.1)
8000-FFFF                                   R                   ROM(UNK-1.2)

Interrupts: Reset, NMI, IRQ
The test routine is at F000

Sound: YM2203 and YM3526 driven by 6809.  Sound added by Bryan McPhail, 1/4/98.

***************************************************************************/

#include "driver.h"
#include "cpu/m6809/m6809.h"
#include "sound/2203intf.h"
#include "sound/3812intf.h"


unsigned char *brkthru_nmi_enable; /* needs to be tracked down */
extern unsigned char *brkthru_videoram;
extern size_t brkthru_videoram_size;

WRITE8_HANDLER( brkthru_1800_w );
WRITE8_HANDLER( brkthru_bgram_w );
WRITE8_HANDLER( brkthru_fgram_w );
VIDEO_START( brkthru );
PALETTE_INIT( brkthru );
VIDEO_UPDATE( brkthru );


static int nmi_enable;

WRITE8_HANDLER( brkthru_1803_w )
{
	/* bit 0 = NMI enable */
	nmi_enable = ~data & 1;

	/* bit 1 = ? maybe IRQ acknowledge */
}
WRITE8_HANDLER( darwin_0803_w )
{
	/* bit 0 = NMI enable */
	/*nmi_enable = ~data & 1;*/
	logerror("0803 %02X\n",data);
        nmi_enable = data;
	/* bit 1 = ? maybe IRQ acknowledge */
}

WRITE8_HANDLER( brkthru_soundlatch_w )
{
	soundlatch_w(offset,data);
	cpunum_set_input_line(1,INPUT_LINE_NMI,PULSE_LINE);
}



static ADDRESS_MAP_START( readmem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x03ff) AM_READ(MRA8_RAM)		/* Plane 0: Text */
	AM_RANGE(0x0400, 0x0bff) AM_READ(MRA8_RAM)
	AM_RANGE(0x0c00, 0x0fff) AM_READ(MRA8_RAM)		/* Plane 2  Background */
	AM_RANGE(0x1000, 0x10ff) AM_READ(MRA8_RAM)		/* Plane 1: Sprites */
	AM_RANGE(0x1100, 0x17ff) AM_READ(MRA8_RAM)
	AM_RANGE(0x1800, 0x1800) AM_READ(input_port_0_r)	/* player controls, player start */
	AM_RANGE(0x1801, 0x1801) AM_READ(input_port_1_r)	/* cocktail player controls */
	AM_RANGE(0x1802, 0x1802) AM_READ(input_port_3_r)	/* DSW 0 */
	AM_RANGE(0x1803, 0x1803) AM_READ(input_port_2_r)	/* coin input & DSW */
	AM_RANGE(0x2000, 0x3fff) AM_READ(MRA8_BANK1)
	AM_RANGE(0x4000, 0xffff) AM_READ(MRA8_ROM)
ADDRESS_MAP_END

static ADDRESS_MAP_START( writemem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x03ff) AM_WRITE(brkthru_fgram_w) AM_BASE(&brkthru_videoram) AM_SIZE(&brkthru_videoram_size)
	AM_RANGE(0x0400, 0x0bff) AM_WRITE(MWA8_RAM)
	AM_RANGE(0x0c00, 0x0fff) AM_WRITE(brkthru_bgram_w) AM_BASE(&videoram) AM_SIZE(&videoram_size)
	AM_RANGE(0x1000, 0x10ff) AM_WRITE(MWA8_RAM) AM_BASE(&spriteram) AM_SIZE(&spriteram_size)
	AM_RANGE(0x1100, 0x17ff) AM_WRITE(MWA8_RAM)
	AM_RANGE(0x1800, 0x1801) AM_WRITE(brkthru_1800_w)	/* bg scroll and color, ROM bank selection, flip screen */
	AM_RANGE(0x1802, 0x1802) AM_WRITE(brkthru_soundlatch_w)
	AM_RANGE(0x1803, 0x1803) AM_WRITE(brkthru_1803_w)	/* NMI enable, + ? */
	AM_RANGE(0x2000, 0xffff) AM_WRITE(MWA8_ROM)
ADDRESS_MAP_END
static ADDRESS_MAP_START( darwin_readmem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x1000, 0x13ff) AM_READ(MRA8_RAM)		/* Plane 0: Text */
	AM_RANGE(0x0400, 0x07ff) AM_READ(MRA8_RAM)
	AM_RANGE(0x1c00, 0x1fff) AM_READ(MRA8_RAM)		/* Plane 2  Background */
	AM_RANGE(0x0000, 0x00ff) AM_READ(MRA8_RAM)		/* Plane 1: Sprites */
 	AM_RANGE(0x1400, 0x1bff) AM_READ(MRA8_RAM)
	AM_RANGE(0x0800, 0x0800) AM_READ(input_port_0_r)	/* player controls, player start */
	AM_RANGE(0x0801, 0x0801) AM_READ(input_port_1_r)	/* cocktail player controls */
	AM_RANGE(0x0802, 0x0802) AM_READ(input_port_3_r)	/* DSW 0 */
	AM_RANGE(0x0803, 0x0803) AM_READ(input_port_2_r)	/* coin input & DSW */
	AM_RANGE(0x2000, 0x3fff) AM_READ(MRA8_BANK1)
	AM_RANGE(0x4000, 0xffff) AM_READ(MRA8_ROM)
ADDRESS_MAP_END

static ADDRESS_MAP_START( darwin_writemem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x1000, 0x13ff) AM_WRITE(brkthru_fgram_w) AM_BASE(&brkthru_videoram) AM_SIZE(&brkthru_videoram_size)
	AM_RANGE(0x1c00, 0x1fff) AM_WRITE(brkthru_bgram_w) AM_BASE(&videoram) AM_SIZE(&videoram_size)
	AM_RANGE(0x0000, 0x00ff) AM_WRITE(MWA8_RAM) AM_BASE(&spriteram) AM_SIZE(&spriteram_size)
	AM_RANGE(0x1400, 0x1bff) AM_WRITE(MWA8_RAM)
	AM_RANGE(0x0100, 0x01ff) AM_WRITE(MWA8_NOP) /*tidyup, nothing realy here?*/
	AM_RANGE(0x0800, 0x0801) AM_WRITE(brkthru_1800_w)     /* bg scroll and color, ROM bank selection, flip screen */
	AM_RANGE(0x0802, 0x0802) AM_WRITE(brkthru_soundlatch_w)
	AM_RANGE(0x0803, 0x0803) AM_WRITE(darwin_0803_w)     /* NMI enable, + ? */
	AM_RANGE(0x2000, 0xffff) AM_WRITE(MWA8_ROM)
ADDRESS_MAP_END

static ADDRESS_MAP_START( sound_readmem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x1fff) AM_READ(MRA8_RAM)
	AM_RANGE(0x4000, 0x4000) AM_READ(soundlatch_r)
	AM_RANGE(0x6000, 0x6000) AM_READ(YM2203_status_port_0_r)
	AM_RANGE(0x8000, 0xffff) AM_READ(MRA8_ROM)
ADDRESS_MAP_END

static ADDRESS_MAP_START( sound_writemem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x1fff) AM_WRITE(MWA8_RAM)
	AM_RANGE(0x2000, 0x2000) AM_WRITE(YM3526_control_port_0_w)
	AM_RANGE(0x2001, 0x2001) AM_WRITE(YM3526_write_port_0_w)
	AM_RANGE(0x6000, 0x6000) AM_WRITE(YM2203_control_port_0_w)
	AM_RANGE(0x6001, 0x6001) AM_WRITE(YM2203_write_port_0_w)
	AM_RANGE(0x8000, 0xffff) AM_WRITE(MWA8_ROM)
ADDRESS_MAP_END



INTERRUPT_GEN( brkthru_interrupt )
{
	if (cpu_getiloops() == 0)
	{
		if (nmi_enable)
			cpunum_set_input_line(0, INPUT_LINE_NMI, PULSE_LINE);
	}
	else
	{
		/* generate IRQ on coin insertion */
		if ((readinputportbytag("IN2") & 0xe0) != 0xe0)
			cpunum_set_input_line(0, 0, HOLD_LINE);
	}
}

#define COMMON_IN0\
	PORT_START_TAG("IN0")\
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON1 )\
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON2 )\
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY\
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY\
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY\
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY\
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START1 )\
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START2 )

#define COMMON_IN1\
	PORT_START_TAG("IN1")\
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_COCKTAIL\
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_COCKTAIL\
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY PORT_COCKTAIL\
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY PORT_COCKTAIL\
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY PORT_COCKTAIL\
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY PORT_COCKTAIL\
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )\
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_VBLANK )	/* used only by the self test */

INPUT_PORTS_START( brkthru )
COMMON_IN0
COMMON_IN1

	PORT_START_TAG("IN2")
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x02, "2" )
	PORT_DIPSETTING(    0x03, "3" )
	PORT_DIPSETTING(    0x01, "5" )
	PORT_DIPSETTING(    0x00, "99 (Cheat)")
	PORT_DIPNAME( 0x0c, 0x0c, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x00, "20000 Points Only" )
	PORT_DIPSETTING(    0x04, "10000/20000 Points" )
	PORT_DIPSETTING(    0x0c, "20000/30000 Points" )
	PORT_DIPSETTING(    0x08, "20000/40000 Points" )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Allow_Continue ) )
	PORT_DIPSETTING(    0x00, DEF_STR( No ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Yes ) )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_COIN1 ) PORT_IMPULSE(2)	/* Manual says Picture Flip=On, Normal=Off */
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_COIN2 ) PORT_IMPULSE(2)
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_SERVICE1 ) PORT_IMPULSE(2)

	PORT_START_TAG("DSW0")
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 1C_3C ) )
	PORT_DIPNAME( 0x0c, 0x0c, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x0c, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 1C_3C ) )
	PORT_DIPNAME( 0x10, 0x10, "Enemy Vehicles" )
	PORT_DIPSETTING(    0x10, "Slow" )
	PORT_DIPSETTING(    0x00, "Fast" )
	PORT_DIPNAME( 0x20, 0x20, "Enemy Bullets" )
	PORT_DIPSETTING(    0x20, "Slow" )
	PORT_DIPSETTING(    0x00, "Fast" )
	PORT_DIPNAME( 0x40, 0x00, "Control Panel" )
	PORT_DIPSETTING(    0x40, DEF_STR( Upright ) )  /* 1 Player  */
	PORT_DIPSETTING(    0x00, DEF_STR( Cocktail ) ) /* 2 Players */
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0xc0, DEF_STR( Cocktail ) )
INPUT_PORTS_END

INPUT_PORTS_START( brkthruj )
COMMON_IN0
COMMON_IN1

	PORT_START_TAG("IN2")
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x02, "2" )
	PORT_DIPSETTING(    0x03, "3" )
	PORT_DIPSETTING(    0x01, "5" )
	PORT_DIPSETTING(    0x00, "99 (Cheat)")
	PORT_DIPNAME( 0x0c, 0x0c, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x00, "20000 Points Only" )
	PORT_DIPSETTING(    0x04, "10000/20000 Points" )
	PORT_DIPSETTING(    0x0c, "20000/30000 Points" )
	PORT_DIPSETTING(    0x08, "20000/40000 Points" )
	PORT_SERVICE( 0x10, IP_ACTIVE_LOW )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_COIN1 ) PORT_IMPULSE(2)	/* Manual says Pitcure Flip=On, Normal=Off */
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_COIN2 ) PORT_IMPULSE(2)
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_SERVICE1 ) PORT_IMPULSE(2)

	PORT_START_TAG("DSW0")
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 1C_3C ) )
	PORT_DIPNAME( 0x0c, 0x0c, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x0c, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 1C_3C ) )
	PORT_DIPNAME( 0x10, 0x10, "Enemy Vehicles" )
	PORT_DIPSETTING(    0x10, "Slow" )
	PORT_DIPSETTING(    0x00, "Fast" )
	PORT_DIPNAME( 0x20, 0x20, "Enemy Bullets" )
	PORT_DIPSETTING(    0x20, "Slow" )
	PORT_DIPSETTING(    0x00, "Fast" )
	PORT_DIPNAME( 0x40, 0x00, "Control Panel" )
	PORT_DIPSETTING(    0x40, DEF_STR( Upright ) )  /* 1 Player  */
	PORT_DIPSETTING(    0x00, DEF_STR( Cocktail ) ) /* 2 Players */
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0xc0, DEF_STR( Cocktail ) )
INPUT_PORTS_END

INPUT_PORTS_START( darwin )
COMMON_IN0
COMMON_IN1

	PORT_START_TAG("IN2")	/* modified by Shingo Suzuki 1999/11/02 */
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x01, "3" )
	PORT_DIPSETTING(    0x00, "5" )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x02, "20k, 50k and every 50k" )
	PORT_DIPSETTING(    0x00, "30k, 80k and every 80k" )
	PORT_DIPNAME( 0x0c, 0x0c, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x0c, DEF_STR( Easy ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Medium ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Hard ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Hardest ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_COIN1 ) PORT_IMPULSE(2)
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_COIN2 ) PORT_IMPULSE(2)
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_SERVICE1 ) PORT_IMPULSE(2)

	PORT_START_TAG("DSW0")
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 1C_3C ) )
	PORT_DIPNAME( 0x0c, 0x0c, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x0c, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 1C_3C ) )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x40, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
INPUT_PORTS_END


static const gfx_layout charlayout =
{
	8,8,	/* 8*8 chars */
	256,	/* 256 characters */
	3,	/* 3 bits per pixel */
	{ 512*8*8+4, 0, 4 },	/* plane offset */
	{ 256*8*8+0, 256*8*8+1, 256*8*8+2, 256*8*8+3, 0, 1, 2, 3 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8	/* every char takes 8 consecutive bytes */
};

static const gfx_layout tilelayout1 =
{
	16,16,	/* 16*16 tiles */
	128,	/* 128 tiles */
	3,	/* 3 bits per pixel */
	{ 0x4000*8+4, 0, 4 },	/* plane offset */
	{ 0, 1, 2, 3, 1024*8*8+0, 1024*8*8+1, 1024*8*8+2, 1024*8*8+3,
			16*8+0, 16*8+1, 16*8+2, 16*8+3, 16*8+1024*8*8+0, 16*8+1024*8*8+1, 16*8+1024*8*8+2, 16*8+1024*8*8+3 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8,
			8*8, 9*8, 10*8, 11*8, 12*8, 13*8, 14*8, 15*8 },
	32*8	/* every tile takes 32 consecutive bytes */
};

static const gfx_layout tilelayout2 =
{
	16,16,	/* 16*16 tiles */
	128,	/* 128 tiles */
	3,	/* 3 bits per pixel */
	{ 0x3000*8+0, 0, 4 },	/* plane offset */
	{ 0, 1, 2, 3, 1024*8*8+0, 1024*8*8+1, 1024*8*8+2, 1024*8*8+3,
			16*8+0, 16*8+1, 16*8+2, 16*8+3, 16*8+1024*8*8+0, 16*8+1024*8*8+1, 16*8+1024*8*8+2, 16*8+1024*8*8+3 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8,
			8*8, 9*8, 10*8, 11*8, 12*8, 13*8, 14*8, 15*8 },
	32*8	/* every tile takes 32 consecutive bytes */
};

static const gfx_layout spritelayout =
{
	16,16,	/* 16*16 sprites */
	1024,	/* 1024 sprites */
	3,	/* 3 bits per pixel */
	{ 2*1024*32*8, 1024*32*8, 0 },	/* plane offset */
	{ 16*8+0, 16*8+1, 16*8+2, 16*8+3, 16*8+4, 16*8+5, 16*8+6, 16*8+7,
			0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8,
			8*8, 9*8, 10*8, 11*8, 12*8, 13*8, 14*8, 15*8 },
	32*8	/* every sprite takes 32 consecutive bytes */
};

static const gfx_decode gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0x00000, &charlayout,   0x00,  1 },	/* use colors 0x00-0x07 */
	{ REGION_GFX2, 0x00000, &tilelayout1,  0x80, 16 },	/* use colors 0x80-0xff */
	{ REGION_GFX2, 0x01000, &tilelayout2,  0x80, 16 },
	{ REGION_GFX2, 0x08000, &tilelayout1,  0x80, 16 },
	{ REGION_GFX2, 0x09000, &tilelayout2,  0x80, 16 },
	{ REGION_GFX2, 0x10000, &tilelayout1,  0x80, 16 },
	{ REGION_GFX2, 0x11000, &tilelayout2,  0x80, 16 },
	{ REGION_GFX2, 0x18000, &tilelayout1,  0x80, 16 },
	{ REGION_GFX2, 0x19000, &tilelayout2,  0x80, 16 },
	{ REGION_GFX3, 0x00000, &spritelayout, 0x40,  8 },	/* use colors 0x40-0x7f */
	{ -1 } /* end of array */
};

/* handler called by the 3812 emulator when the internal timers cause an IRQ */
static void irqhandler(int linestate)
{
	cpunum_set_input_line(1,M6809_IRQ_LINE,linestate);
}

static struct YM3526interface ym3526_interface =
{
	irqhandler
};



static MACHINE_DRIVER_START( brkthru )

	/* basic machine hardware */
	MDRV_CPU_ADD(M6809, 1250000)        /* 1.25 MHz ? */
	MDRV_CPU_PROGRAM_MAP(readmem,writemem)
	MDRV_CPU_VBLANK_INT(brkthru_interrupt,2)

	MDRV_CPU_ADD(M6809, 1250000)
	/* audio CPU */        /* 1.25 MHz ? */
	MDRV_CPU_PROGRAM_MAP(sound_readmem,sound_writemem)

	MDRV_FRAMES_PER_SECOND(58)
	MDRV_VBLANK_DURATION(DEFAULT_REAL_60HZ_VBLANK_DURATION)	/* frames per second, vblank duration (not sure) */

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(32*8, 32*8)
	MDRV_VISIBLE_AREA(1*8, 31*8-1, 1*8, 31*8-1)	/* not sure */
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(256)

	MDRV_PALETTE_INIT(brkthru)
	MDRV_VIDEO_START(brkthru)
	MDRV_VIDEO_UPDATE(brkthru)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")

	MDRV_SOUND_ADD(YM2203, 1500000)
	MDRV_SOUND_ROUTE(0, "mono", 0.10)
	MDRV_SOUND_ROUTE(1, "mono", 0.10)
	MDRV_SOUND_ROUTE(2, "mono", 0.10)
	MDRV_SOUND_ROUTE(3, "mono", 0.50)

	MDRV_SOUND_ADD(YM3526, 3000000)
	MDRV_SOUND_CONFIG(ym3526_interface)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.0)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( darwin )

	/* basic machine hardware */
	MDRV_CPU_ADD(M6809, 1500000)        /* 1.25 MHz ? */
	MDRV_CPU_PROGRAM_MAP(darwin_readmem,darwin_writemem)
	MDRV_CPU_VBLANK_INT(brkthru_interrupt,2)

	MDRV_CPU_ADD(M6809, 1500000)
	/* audio CPU */        /* 1.25 MHz ? */
	MDRV_CPU_PROGRAM_MAP(sound_readmem,sound_writemem)

	MDRV_FRAMES_PER_SECOND(15625.0/272)
	MDRV_VBLANK_DURATION(DEFAULT_REAL_60HZ_VBLANK_DURATION)
	/* frames per second, vblank duration
        Horizontal video frequency:
            HSync = Dot Clock / Horizontal Frame Length
                  = Xtal /2   / (HDisplay + HBlank)
                  = 12MHz/2   / (240 + 144)
                  = 15.625kHz
        Vertical Video frequency:
            VSync = HSync / Vertical Frame Length
                  = HSync / (VDisplay + VBlank)
                  = 15.625kHz / (240 + 32)
                  = 57.444855Hz
    tuned by Shingo SUZUKI(VSyncMAME Project) 2000/10/19 */

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(32*8, 32*8)
	MDRV_VISIBLE_AREA(1*8, 31*8-1, 1*8, 31*8-1)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(256)

	MDRV_PALETTE_INIT(brkthru)
	MDRV_VIDEO_START(brkthru)
	MDRV_VIDEO_UPDATE(brkthru)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")

	MDRV_SOUND_ADD(YM2203, 1500000)
	MDRV_SOUND_ROUTE(0, "mono", 0.10)
	MDRV_SOUND_ROUTE(1, "mono", 0.10)
	MDRV_SOUND_ROUTE(2, "mono", 0.10)
	MDRV_SOUND_ROUTE(3, "mono", 0.50)

	MDRV_SOUND_ADD(YM3526, 3000000)
	MDRV_SOUND_CONFIG(ym3526_interface)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.0)
MACHINE_DRIVER_END


/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START( brkthru )
	ROM_REGION( 0x20000, REGION_CPU1, 0 )     /* 64k for main CPU + 64k for banked ROMs */
	ROM_LOAD( "brkthru.1",    0x04000, 0x4000, CRC(cfb4265f) SHA1(4cd748fa06fd2727de1694196912d605672d4883) )
	ROM_LOAD( "brkthru.2",    0x08000, 0x8000, CRC(fa8246d9) SHA1(d6da03b2a3d8a83411191351ee110b89352a3ead) )
	ROM_LOAD( "brkthru.4",    0x10000, 0x8000, CRC(8cabf252) SHA1(45e8847b2e6b278989f67e0b27b827a9b3b92581) )
	ROM_LOAD( "brkthru.3",    0x18000, 0x8000, CRC(2f2c40c2) SHA1(fcb78941453520a3a07f272127dae7c2cc1999ea) )

	ROM_REGION( 0x02000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "brkthru.12",   0x00000, 0x2000, CRC(58c0b29b) SHA1(9dc075f8afae7e8fe164a9fe325e9948cdc7e4bb) )	/* characters */

	ROM_REGION( 0x20000, REGION_GFX2, ROMREGION_DISPOSE )
	/* background */
	/* we do a lot of scatter loading here, to place the data in a format */
	/* which can be decoded by MAME's standard functions */
	ROM_LOAD( "brkthru.7",    0x00000, 0x4000, CRC(920cc56a) SHA1(c75806691073f1f3bd54dcaca4c14155ecf4471d) )	/* bitplanes 1,2 for bank 1,2 */
	ROM_CONTINUE(             0x08000, 0x4000 )				/* bitplanes 1,2 for bank 3,4 */
	ROM_LOAD( "brkthru.6",    0x10000, 0x4000, CRC(fd3cee40) SHA1(3308b96bb69e0fa6dffbdff296273fafa16d5e70) )	/* bitplanes 1,2 for bank 5,6 */
	ROM_CONTINUE(             0x18000, 0x4000 )				/* bitplanes 1,2 for bank 7,8 */
	ROM_LOAD( "brkthru.8",    0x04000, 0x1000, CRC(f67ee64e) SHA1(75634bd481ae44b8aa02acb4f9b4d7ff973a4c71) )	/* bitplane 3 for bank 1,2 */
	ROM_CONTINUE(             0x06000, 0x1000 )
	ROM_CONTINUE(             0x0c000, 0x1000 )				/* bitplane 3 for bank 3,4 */
	ROM_CONTINUE(             0x0e000, 0x1000 )
	ROM_CONTINUE(             0x14000, 0x1000 )				/* bitplane 3 for bank 5,6 */
	ROM_CONTINUE(             0x16000, 0x1000 )
	ROM_CONTINUE(             0x1c000, 0x1000 )				/* bitplane 3 for bank 7,8 */
	ROM_CONTINUE(             0x1e000, 0x1000 )

	ROM_REGION( 0x18000, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_LOAD( "brkthru.9",    0x00000, 0x8000, CRC(f54e50a7) SHA1(eccf4d859c26944271ec6586644b4730a72851fd) )	/* sprites */
	ROM_LOAD( "brkthru.10",   0x08000, 0x8000, CRC(fd156945) SHA1(a0575a4164217e63317886176ab7e59d255fc771) )
	ROM_LOAD( "brkthru.11",   0x10000, 0x8000, CRC(c152a99b) SHA1(f96133aa01219eda357b9e906bd9577dbfe359c0) )

	ROM_REGION( 0x0200, REGION_PROMS, 0 )
	ROM_LOAD( "brkthru.13",   0x0000, 0x0100, CRC(aae44269) SHA1(7c66aeb93577104109d264ee8b848254256c81eb) ) /* red and green component */
	ROM_LOAD( "brkthru.14",   0x0100, 0x0100, CRC(f2d4822a) SHA1(f535e91b87ff01f2a73662856fd3f72907ca62e9) ) /* blue component */

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* 64K for sound CPU */
	ROM_LOAD( "brkthru.5",    0x8000, 0x8000, CRC(c309435f) SHA1(82914004c2b169a7c31aa49af83a699ebbc7b33f) )
ROM_END

ROM_START( brkthruj )
	ROM_REGION( 0x20000, REGION_CPU1, 0 )     /* 64k for main CPU + 64k for banked ROMs */
	ROM_LOAD( "1",            0x04000, 0x4000, CRC(09bd60ee) SHA1(9591a4c89bb69d5615a5d6b29c47e6b17350c007) )
	ROM_LOAD( "2",            0x08000, 0x8000, CRC(f2b2cd1c) SHA1(dafccc74310876bc1c88de7f3c86f93ed8a0eb62) )
	ROM_LOAD( "4",            0x10000, 0x8000, CRC(b42b3359) SHA1(c1da550e0f7cc52721802c7c0f2770ef0087e28b) )
	ROM_LOAD( "brkthru.3",    0x18000, 0x8000, CRC(2f2c40c2) SHA1(fcb78941453520a3a07f272127dae7c2cc1999ea) )

	ROM_REGION( 0x02000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "12",   0x00000, 0x2000, CRC(3d9a7003) SHA1(2e5de982eb75ac75312fb29bb4cb2ed12ec0fd56) )	/* characters */

	ROM_REGION( 0x20000, REGION_GFX2, ROMREGION_DISPOSE )
	/* background */
	/* we do a lot of scatter loading here, to place the data in a format */
	/* which can be decoded by MAME's standard functions */
	ROM_LOAD( "brkthru.7",    0x00000, 0x4000, CRC(920cc56a) SHA1(c75806691073f1f3bd54dcaca4c14155ecf4471d) )	/* bitplanes 1,2 for bank 1,2 */
	ROM_CONTINUE(             0x08000, 0x4000 )				/* bitplanes 1,2 for bank 3,4 */
	ROM_LOAD( "6",            0x10000, 0x4000, CRC(cb47b395) SHA1(bf5459d696e863644f13c8b0786b8f45caf6ceb6) )	/* bitplanes 1,2 for bank 5,6 */
	ROM_CONTINUE(             0x18000, 0x4000 )				/* bitplanes 1,2 for bank 7,8 */
	ROM_LOAD( "8",            0x04000, 0x1000, CRC(5e5a2cd7) SHA1(f1782d67b924b4b89bcb6602e970c28fbeaab522) )	/* bitplane 3 for bank 1,2 */
	ROM_CONTINUE(             0x06000, 0x1000 )
	ROM_CONTINUE(             0x0c000, 0x1000 )				/* bitplane 3 for bank 3,4 */
	ROM_CONTINUE(             0x0e000, 0x1000 )
	ROM_CONTINUE(             0x14000, 0x1000 )				/* bitplane 3 for bank 5,6 */
	ROM_CONTINUE(             0x16000, 0x1000 )
	ROM_CONTINUE(             0x1c000, 0x1000 )				/* bitplane 3 for bank 7,8 */
	ROM_CONTINUE(             0x1e000, 0x1000 )

	ROM_REGION( 0x18000, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_LOAD( "brkthru.9",    0x00000, 0x8000, CRC(f54e50a7) SHA1(eccf4d859c26944271ec6586644b4730a72851fd) )	/* sprites */
	ROM_LOAD( "brkthru.10",   0x08000, 0x8000, CRC(fd156945) SHA1(a0575a4164217e63317886176ab7e59d255fc771) )
	ROM_LOAD( "brkthru.11",   0x10000, 0x8000, CRC(c152a99b) SHA1(f96133aa01219eda357b9e906bd9577dbfe359c0) )

	ROM_REGION( 0x0200, REGION_PROMS, 0 )
	ROM_LOAD( "brkthru.13",   0x0000, 0x0100, CRC(aae44269) SHA1(7c66aeb93577104109d264ee8b848254256c81eb) ) /* red and green component */
	ROM_LOAD( "brkthru.14",   0x0100, 0x0100, CRC(f2d4822a) SHA1(f535e91b87ff01f2a73662856fd3f72907ca62e9) ) /* blue component */

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* 64K for sound CPU */
	ROM_LOAD( "brkthru.5",    0x8000, 0x8000, CRC(c309435f) SHA1(82914004c2b169a7c31aa49af83a699ebbc7b33f) )
ROM_END

/* bootleg, changed prg rom fails test, probably just the japanese version modified to have english title */
ROM_START( forcebrk )
	ROM_REGION( 0x20000, REGION_CPU1, 0 )     /* 64k for main CPU + 64k for banked ROMs */
	ROM_LOAD( "1",            0x04000, 0x4000, CRC(09bd60ee) SHA1(9591a4c89bb69d5615a5d6b29c47e6b17350c007) )
	ROM_LOAD( "2",            0x08000, 0x8000, CRC(f2b2cd1c) SHA1(dafccc74310876bc1c88de7f3c86f93ed8a0eb62) )
	ROM_LOAD( "forcebrk4",    0x10000, 0x8000, CRC(b4838c19) SHA1(b32f183ee042872a6eb6689aab219108d37829e4) )
	ROM_LOAD( "brkthru.3",    0x18000, 0x8000, CRC(2f2c40c2) SHA1(fcb78941453520a3a07f272127dae7c2cc1999ea) )

	ROM_REGION( 0x02000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "12",           0x00000, 0x2000, CRC(3d9a7003) SHA1(2e5de982eb75ac75312fb29bb4cb2ed12ec0fd56) )	/* characters */

	ROM_REGION( 0x20000, REGION_GFX2, ROMREGION_DISPOSE )
	/* background */
	/* we do a lot of scatter loading here, to place the data in a format */
	/* which can be decoded by MAME's standard functions */
	ROM_LOAD( "brkthru.7",    0x00000, 0x4000, CRC(920cc56a) SHA1(c75806691073f1f3bd54dcaca4c14155ecf4471d) )	/* bitplanes 1,2 for bank 1,2 */
	ROM_CONTINUE(             0x08000, 0x4000 )				/* bitplanes 1,2 for bank 3,4 */
	ROM_LOAD( "forcebrk6",    0x10000, 0x4000, CRC(08bca16a) SHA1(d5dcf5cf68a5090f467c076abb1b9cf0baffe272) )	/* bitplanes 1,2 for bank 5,6 */
	ROM_CONTINUE(             0x18000, 0x4000 )				/* bitplanes 1,2 for bank 7,8 */
	ROM_LOAD( "forcebrk8",    0x04000, 0x1000, CRC(a3a1131e) SHA1(e0b73c8b2c8ea6b31418bc642830875c5985f800) )	/* bitplane 3 for bank 1,2 */
	ROM_CONTINUE(             0x06000, 0x1000 )
	ROM_CONTINUE(             0x0c000, 0x1000 )				/* bitplane 3 for bank 3,4 */
	ROM_CONTINUE(             0x0e000, 0x1000 )
	ROM_CONTINUE(             0x14000, 0x1000 )				/* bitplane 3 for bank 5,6 */
	ROM_CONTINUE(             0x16000, 0x1000 )
	ROM_CONTINUE(             0x1c000, 0x1000 )				/* bitplane 3 for bank 7,8 */
	ROM_CONTINUE(             0x1e000, 0x1000 )

	ROM_REGION( 0x18000, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_LOAD( "brkthru.9",    0x00000, 0x8000, CRC(f54e50a7) SHA1(eccf4d859c26944271ec6586644b4730a72851fd) )	/* sprites */
	ROM_LOAD( "brkthru.10",   0x08000, 0x8000, CRC(fd156945) SHA1(a0575a4164217e63317886176ab7e59d255fc771) )
	ROM_LOAD( "brkthru.11",   0x10000, 0x8000, CRC(c152a99b) SHA1(f96133aa01219eda357b9e906bd9577dbfe359c0) )

	ROM_REGION( 0x0200, REGION_PROMS, 0 )
	ROM_LOAD( "brkthru.13",   0x0000, 0x0100, CRC(aae44269) SHA1(7c66aeb93577104109d264ee8b848254256c81eb) ) /* red and green component */
	ROM_LOAD( "brkthru.14",   0x0100, 0x0100, CRC(f2d4822a) SHA1(f535e91b87ff01f2a73662856fd3f72907ca62e9) ) /* blue component */

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* 64K for sound CPU */
	ROM_LOAD( "brkthru.5",    0x8000, 0x8000, CRC(c309435f) SHA1(82914004c2b169a7c31aa49af83a699ebbc7b33f) )
ROM_END

ROM_START( darwin )
	ROM_REGION( 0x20000, REGION_CPU1, 0 )     /* 64k for main CPU + 64k for banked ROMs */
	ROM_LOAD( "darw_04.rom",  0x04000, 0x4000, CRC(0eabf21c) SHA1(ccad6b30fe9361e8a21b8aaf8116aa85f9e6bb19) )
	ROM_LOAD( "darw_05.rom",  0x08000, 0x8000, CRC(e771f864) SHA1(8ba9f97c6abf035ceaf9f5505495708506f1b0c5) )
	ROM_LOAD( "darw_07.rom",  0x10000, 0x8000, CRC(97ac052c) SHA1(8baa117472d46b99e5946f095b869de9b5c48f9a) )
	ROM_LOAD( "darw_06.rom",  0x18000, 0x8000, CRC(2a9fb208) SHA1(f04a5502600e49e2494a87ec65a44a2843441d37) )

	ROM_REGION( 0x02000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "darw_09.rom",  0x00000, 0x2000, CRC(067b4cf5) SHA1(fc752bb72e4850b71565afd1df0cbb4f732f131c) )   /* characters */

	ROM_REGION( 0x20000, REGION_GFX2, ROMREGION_DISPOSE )
	/* background */
	/* we do a lot of scatter loading here, to place the data in a format */
	/* which can be decoded by MAME's standard functions */
	ROM_LOAD( "darw_03.rom",  0x00000, 0x4000, CRC(57d0350d) SHA1(6f904047485e669afb5f4b590818743111f010c6) )	/* bitplanes 1,2 for bank 1,2 */
	ROM_CONTINUE(             0x08000, 0x4000 )				/* bitplanes 1,2 for bank 3,4 */
	ROM_LOAD( "darw_02.rom",  0x10000, 0x4000, CRC(559a71ab) SHA1(a28de25e89e0d68332f4095b988827a9cb72c675) )	/* bitplanes 1,2 for bank 5,6 */
	ROM_CONTINUE(             0x18000, 0x4000 )				/* bitplanes 1,2 for bank 7,8 */
	ROM_LOAD( "darw_01.rom",  0x04000, 0x1000, CRC(15a16973) SHA1(5eb978a32be88176936e5d37b6ec18820d9720d8) )	/* bitplane 3 for bank 1,2 */
	ROM_CONTINUE(             0x06000, 0x1000 )
	ROM_CONTINUE(             0x0c000, 0x1000 )				/* bitplane 3 for bank 3,4 */
	ROM_CONTINUE(             0x0e000, 0x1000 )
	ROM_CONTINUE(             0x14000, 0x1000 )				/* bitplane 3 for bank 5,6 */
	ROM_CONTINUE(             0x16000, 0x1000 )
	ROM_CONTINUE(             0x1c000, 0x1000 )				/* bitplane 3 for bank 7,8 */
	ROM_CONTINUE(             0x1e000, 0x1000 )

	ROM_REGION( 0x18000, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_LOAD( "darw_10.rom",  0x00000, 0x8000, CRC(487a014c) SHA1(c9543df8115088b02019e76a6473ecc5f645a836) )	/* sprites */
	ROM_LOAD( "darw_11.rom",  0x08000, 0x8000, CRC(548ce2d1) SHA1(3b1757c70346ab4ee19ec85e7ae5137f8ccf446f) )
	ROM_LOAD( "darw_12.rom",  0x10000, 0x8000, CRC(faba5fef) SHA1(848da4d4888f0218b737f1dc9b62944f68349a43) )

	ROM_REGION( 0x0200, REGION_PROMS, 0 )
	ROM_LOAD( "df.12",   0x0000, 0x0100, CRC(89b952ef) SHA1(77dc4020a2e25f81fae1182d58993cf09d13af00) ) /* red and green component */
	ROM_LOAD( "df.13",   0x0100, 0x0100, CRC(d595e91d) SHA1(5e9793f6602455c79afdc855cd13183a7f48ab1e) ) /* blue component */

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* 64K for sound CPU */
	ROM_LOAD( "darw_08.rom",  0x8000, 0x8000, CRC(6b580d58) SHA1(a70aebc6b4a291b4adddbb41d092b2682fc2d421) )
ROM_END



GAME( 1986, brkthru,  0,       brkthru, brkthru,  0, ROT0,   "Data East USA", "Break Thru (US)", 0 )
GAME( 1986, brkthruj, brkthru, brkthru, brkthruj, 0, ROT0,   "Data East Corporation", "Kyohkoh-Toppa (Japan)", 0 )
GAME( 1986, forcebrk, brkthru, brkthru, brkthruj, 0, ROT0,   "bootleg", "Force Break (bootleg)", 0 )
GAME( 1986, darwin,   0,       darwin,  darwin,   0, ROT270, "Data East Corporation", "Darwin 4078 (Japan)", 0 )
