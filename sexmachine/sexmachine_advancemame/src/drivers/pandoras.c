/***************************************************************************

Pandora's Palace(GX328) (c) 1984 Konami/Interlogic

Driver by Manuel Abadia <manu@teleline.es>

Notes:
- Press 1P and 2P together to enter test mode.

TODO:
- CPU B continuously reads from 1e00. It seems to be important, could be a
  scanline counter or something like that.

***************************************************************************/

#include "driver.h"
#include "cpu/m6809/m6809.h"
#include "cpu/z80/z80.h"
#include "cpu/i8039/i8039.h"
#include "sound/ay8910.h"
#include "sound/dac.h"

static int irq_enable_a, irq_enable_b;
static int firq_old_data_a, firq_old_data_b;
static int i8039_status;

unsigned char *pandoras_sharedram;
static unsigned char *pandoras_sharedram2;

/* from vidhrdw */
PALETTE_INIT( pandoras );
READ8_HANDLER( pandoras_vram_r );
READ8_HANDLER( pandoras_cram_r );
WRITE8_HANDLER( pandoras_vram_w );
WRITE8_HANDLER( pandoras_cram_w );
WRITE8_HANDLER( pandoras_flipscreen_w );
WRITE8_HANDLER( pandoras_scrolly_w );
VIDEO_START( pandoras );
VIDEO_UPDATE( pandoras );

static INTERRUPT_GEN( pandoras_interrupt_a ){
	if (irq_enable_a)
		cpunum_set_input_line(0, M6809_IRQ_LINE, HOLD_LINE);
}

static INTERRUPT_GEN( pandoras_interrupt_b ){
	if (irq_enable_b)
		cpunum_set_input_line(1, M6809_IRQ_LINE, HOLD_LINE);
}

static READ8_HANDLER( pandoras_sharedram_r ){
	return pandoras_sharedram[offset];
}

static WRITE8_HANDLER( pandoras_sharedram_w ){
	pandoras_sharedram[offset] = data;
}

static READ8_HANDLER( pandoras_sharedram2_r ){
	return pandoras_sharedram2[offset];
}

static WRITE8_HANDLER( pandoras_sharedram2_w ){
	pandoras_sharedram2[offset] = data;
}

static WRITE8_HANDLER( pandoras_int_control_w ){
	/*  byte 0: irq enable (CPU A)
        byte 2: coin counter 1
        byte 3: coin counter 2
        byte 5: flip screen
        byte 6: irq enable (CPU B)
        byte 7: NMI to CPU B

        other bytes unknown */

	switch (offset){
		case 0x00:	if (!data) cpunum_set_input_line(0, M6809_IRQ_LINE, CLEAR_LINE);
					irq_enable_a = data;
					break;
		case 0x02:	coin_counter_w(0,data & 0x01);
					break;
		case 0x03:	coin_counter_w(1,data & 0x01);
					break;
		case 0x05:	pandoras_flipscreen_w(0, data);
					break;
		case 0x06:	if (!data) cpunum_set_input_line(1, M6809_IRQ_LINE, CLEAR_LINE);
					irq_enable_b = data;
					break;
		case 0x07:	cpunum_set_input_line(1,INPUT_LINE_NMI,PULSE_LINE);
					break;

		default:
			logerror("%04x: (irq_ctrl) write %02x to %02x\n",activecpu_get_pc(), data, offset);
	}
}

WRITE8_HANDLER( pandoras_cpua_irqtrigger_w ){
	if (!firq_old_data_a && data){
		cpunum_set_input_line(0,M6809_FIRQ_LINE,HOLD_LINE);
	}

	firq_old_data_a = data;
}

WRITE8_HANDLER( pandoras_cpub_irqtrigger_w ){
	if (!firq_old_data_b && data){
		cpunum_set_input_line(1,M6809_FIRQ_LINE,HOLD_LINE);
	}

	firq_old_data_b = data;
}

WRITE8_HANDLER( pandoras_i8039_irqtrigger_w )
{
	cpunum_set_input_line(3, 0, ASSERT_LINE);
}

static WRITE8_HANDLER( i8039_irqen_and_status_w )
{
	/* bit 7 enables IRQ */
	if ((data & 0x80) == 0)
		cpunum_set_input_line(3, 0, CLEAR_LINE);

	/* bit 5 goes to 8910 port A */
	i8039_status = (data & 0x20) >> 5;
}

WRITE8_HANDLER( pandoras_z80_irqtrigger_w )
{
	cpunum_set_input_line_and_vector(2,0,HOLD_LINE,0xff);
}



static ADDRESS_MAP_START( pandoras_readmem_a, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x0fff) AM_READ(pandoras_sharedram_r)	/* Work RAM (Shared with CPU B) */
	AM_RANGE(0x1000, 0x13ff) AM_READ(pandoras_cram_r)		/* Color RAM (shared with CPU B) */
	AM_RANGE(0x1400, 0x17ff) AM_READ(pandoras_vram_r)		/* Video RAM (shared with CPU B) */
	AM_RANGE(0x4000, 0x5fff) AM_READ(MRA8_ROM)				/* space for diagnostic ROM */
	AM_RANGE(0x6000, 0x67ff) AM_READ(pandoras_sharedram2_r)	/* Shared RAM with CPU B */
	AM_RANGE(0x8000, 0xffff) AM_READ(MRA8_ROM)				/* ROM */
ADDRESS_MAP_END

static ADDRESS_MAP_START( pandoras_writemem_a, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x0fff) AM_WRITE(pandoras_sharedram_w) AM_BASE(&pandoras_sharedram)	/* Work RAM (Shared with CPU B) */
	AM_RANGE(0x1000, 0x13ff) AM_WRITE(pandoras_cram_w) AM_BASE(&colorram)					/* Color RAM (shared with CPU B) */
	AM_RANGE(0x1400, 0x17ff) AM_WRITE(pandoras_vram_w) AM_BASE(&videoram)					/* Video RAM (shared with CPU B) */
	AM_RANGE(0x1800, 0x1807) AM_WRITE(pandoras_int_control_w)						/* INT control */
	AM_RANGE(0x1a00, 0x1a00) AM_WRITE(pandoras_scrolly_w)							/* bg scroll */
	AM_RANGE(0x1c00, 0x1c00) AM_WRITE(pandoras_z80_irqtrigger_w)					/* cause INT on the Z80 */
	AM_RANGE(0x1e00, 0x1e00) AM_WRITE(soundlatch_w)								/* sound command to the Z80 */
	AM_RANGE(0x2000, 0x2000) AM_WRITE(pandoras_cpub_irqtrigger_w)					/* cause FIRQ on CPU B */
	AM_RANGE(0x2001, 0x2001) AM_WRITE(watchdog_reset_w)							/* watchdog reset */
	AM_RANGE(0x4000, 0x5fff) AM_WRITE(MWA8_ROM)									/* see notes */
	AM_RANGE(0x6000, 0x67ff) AM_WRITE(pandoras_sharedram2_w) AM_BASE(&pandoras_sharedram2)/* Shared RAM with CPU B */
	AM_RANGE(0x8000, 0xffff) AM_WRITE(MWA8_ROM)									/* ROM */
ADDRESS_MAP_END

static ADDRESS_MAP_START( pandoras_readmem_b, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x0fff) AM_READ(pandoras_sharedram_r)	/* Work RAM (Shared with CPU A) */
	AM_RANGE(0x1000, 0x13ff) AM_READ(pandoras_cram_r)		/* Color RAM (shared with CPU A) */
	AM_RANGE(0x1400, 0x17ff) AM_READ(pandoras_vram_r)		/* Video RAM (shared with CPU A) */
	AM_RANGE(0x1800, 0x1800) AM_READ(input_port_0_r)			/* DIPSW #1 */
	AM_RANGE(0x1a00, 0x1a00) AM_READ(input_port_3_r)			/* COINSW */
	AM_RANGE(0x1a01, 0x1a01) AM_READ(input_port_4_r)			/* 1P inputs */
	AM_RANGE(0x1a02, 0x1a02) AM_READ(input_port_5_r)			/* 2P inputs */
	AM_RANGE(0x1a03, 0x1a03) AM_READ(input_port_2_r)			/* DIPSW #3 */
	AM_RANGE(0x1c00, 0x1c00) AM_READ(input_port_1_r)			/* DISPW #2 */
//  AM_RANGE(0x1e00, 0x1e00) AM_READ(MRA8_NOP)              /* ??? seems to be important */
	AM_RANGE(0xc000, 0xc7ff) AM_READ(pandoras_sharedram2_r)	/* Shared RAM with the CPU A */
	AM_RANGE(0xe000, 0xffff) AM_READ(MRA8_ROM)				/* ROM */
ADDRESS_MAP_END

static ADDRESS_MAP_START( pandoras_writemem_b, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x0fff) AM_WRITE(pandoras_sharedram_w)	/* Work RAM (Shared with CPU A) */
	AM_RANGE(0x1000, 0x13ff) AM_WRITE(pandoras_cram_w)		/* Color RAM (shared with CPU A) */
	AM_RANGE(0x1400, 0x17ff) AM_WRITE(pandoras_vram_w)		/* Video RAM (shared with CPU A) */
	AM_RANGE(0x1800, 0x1807) AM_WRITE(pandoras_int_control_w)	/* INT control */
	AM_RANGE(0x8000, 0x8000) AM_WRITE(watchdog_reset_w)		/* watchdog reset */
	AM_RANGE(0xa000, 0xa000) AM_WRITE(pandoras_cpua_irqtrigger_w)/* cause FIRQ on CPU A */
	AM_RANGE(0xc000, 0xc7ff) AM_WRITE(pandoras_sharedram2_w)	/* Shared RAM with the CPU A */
	AM_RANGE(0xe000, 0xffff) AM_WRITE(MWA8_ROM)				/* ROM */
ADDRESS_MAP_END

static ADDRESS_MAP_START( pandoras_readmem_snd, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x1fff) AM_READ(MRA8_ROM)				/* ROM */
	AM_RANGE(0x2000, 0x23ff) AM_READ(MRA8_RAM)				/* RAM */
	AM_RANGE(0x4000, 0x4000) AM_READ(soundlatch_r)			/* soundlatch_r */
	AM_RANGE(0x6001, 0x6001) AM_READ(AY8910_read_port_0_r)	/* AY-8910 */
ADDRESS_MAP_END

static ADDRESS_MAP_START( pandoras_writemem_snd, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x1fff) AM_WRITE(MWA8_ROM)				/* ROM */
	AM_RANGE(0x2000, 0x23ff) AM_WRITE(MWA8_RAM)				/* RAM */
	AM_RANGE(0x6000, 0x6000) AM_WRITE(AY8910_control_port_0_w)/* AY-8910 */
	AM_RANGE(0x6002, 0x6002) AM_WRITE(AY8910_write_port_0_w)	/* AY-8910 */
	AM_RANGE(0x8000, 0x8000) AM_WRITE(pandoras_i8039_irqtrigger_w)/* cause INT on the 8039 */
	AM_RANGE(0xa000, 0xa000) AM_WRITE(soundlatch2_w)			/* sound command to the 8039 */
ADDRESS_MAP_END

static ADDRESS_MAP_START( i8039_readmem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x0fff) AM_READ(MRA8_ROM)
ADDRESS_MAP_END

static ADDRESS_MAP_START( i8039_writemem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x0fff) AM_WRITE(MWA8_ROM)
ADDRESS_MAP_END

static ADDRESS_MAP_START( i8039_readport, ADDRESS_SPACE_IO, 8 )
	AM_RANGE(0x00, 0xff) AM_READ(soundlatch2_r)
ADDRESS_MAP_END

static ADDRESS_MAP_START( i8039_writeport, ADDRESS_SPACE_IO, 8 )
	AM_RANGE(I8039_p1, I8039_p1) AM_WRITE(DAC_0_data_w)
	AM_RANGE(I8039_p2, I8039_p2) AM_WRITE(i8039_irqen_and_status_w)
ADDRESS_MAP_END


/***************************************************************************

    Input Ports

***************************************************************************/

INPUT_PORTS_START( pandoras )
	PORT_START	/* DSW #1 */
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
//  PORT_DIPSETTING(    0x00, "Invalid" )

	PORT_START	/* DSW #2 */
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x03, "3" )
	PORT_DIPSETTING(    0x02, "4" )
	PORT_DIPSETTING(    0x01, "5" )
	PORT_DIPSETTING(    0x00, "7" )
	PORT_DIPNAME( 0x04, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0x18, 0x18, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x18, "20k and every 60k" )
	PORT_DIPSETTING(    0x10, "30k and every 70k" )
	PORT_DIPSETTING(    0x08, "20k" )
	PORT_DIPSETTING(    0x00, "30k" )
	PORT_DIPNAME( 0x60, 0x40, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x60, DEF_STR( Easy ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Normal ) )
	PORT_DIPSETTING(    0x20, "Difficult" )
	PORT_DIPSETTING(    0x00, "Very Difficult" )
	PORT_DIPNAME( 0x80, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START	/* DSW #3 */
	PORT_DIPNAME( 0x01, 0x01, "Freeze" )
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
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START	/* COINSW */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_COIN3 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0xc0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* PLAYER 1 INPUTS */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0xe0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* PLAYER 2 INPUTS */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY PORT_COCKTAIL
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY PORT_COCKTAIL
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY PORT_COCKTAIL
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY PORT_COCKTAIL
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_COCKTAIL
	PORT_BIT( 0xe0, IP_ACTIVE_LOW, IPT_UNUSED )
INPUT_PORTS_END



static const gfx_layout charlayout =
{
	8,8,
	RGN_FRAC(1,1),
	4,
	{ 0, 1, 2, 3 },
	{ 0*4, 1*4, 2*4, 3*4, 4*4, 5*4, 6*4, 7*4 },
	{ 0*32, 1*32, 2*32, 3*32, 4*32, 5*32, 6*32, 7*32 },
	32*8
};

static const gfx_layout spritelayout =
{
	16,16,
	RGN_FRAC(1,1),
	4,
	{ 0, 1, 2, 3 },
	{ 15*4, 14*4, 13*4, 12*4, 11*4, 10*4, 9*4, 8*4,
			7*4, 6*4, 5*4, 4*4, 3*4, 2*4, 1*4, 0*4 },
	{ 15*4*16, 14*4*16, 13*4*16, 12*4*16, 11*4*16, 10*4*16, 9*4*16, 8*4*16,
			7*4*16, 6*4*16, 5*4*16, 4*4*16, 3*4*16, 2*4*16, 1*4*16, 0*4*16 },
	32*4*8
};

static const gfx_decode gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &charlayout,       0, 16 },
	{ REGION_GFX2, 0, &spritelayout, 16*16, 16 },
	{ -1 } /* end of array */
};

/***************************************************************************

    Machine Driver

***************************************************************************/

static MACHINE_RESET( pandoras )
{
	firq_old_data_a = firq_old_data_b = 0;
	irq_enable_a = irq_enable_b = 0;
}

static READ8_HANDLER( pandoras_portA_r )
{
	return i8039_status;
}

static READ8_HANDLER( pandoras_portB_r )
{
	return (activecpu_gettotalcycles() / 512) & 0x0f;
}

static struct AY8910interface ay8910_interface =
{
	pandoras_portA_r,	// not used
	pandoras_portB_r
};

static MACHINE_DRIVER_START( pandoras )

	/* basic machine hardware */
	MDRV_CPU_ADD(M6809,18432000/6)	/* CPU A */
	MDRV_CPU_PROGRAM_MAP(pandoras_readmem_a,pandoras_writemem_a)
	MDRV_CPU_VBLANK_INT(pandoras_interrupt_a,1)

	MDRV_CPU_ADD(M6809,18432000/6)	/* CPU B */
	MDRV_CPU_PROGRAM_MAP(pandoras_readmem_b,pandoras_writemem_b)
	MDRV_CPU_VBLANK_INT(pandoras_interrupt_b,1)

	MDRV_CPU_ADD(Z80,14318000/8)
	/* audio CPU */
	MDRV_CPU_PROGRAM_MAP(pandoras_readmem_snd,pandoras_writemem_snd)

	MDRV_CPU_ADD(I8039,(14318000/2)/I8039_CLOCK_DIVIDER)
	/* audio CPU */
	MDRV_CPU_PROGRAM_MAP(i8039_readmem,i8039_writemem)
	MDRV_CPU_IO_MAP(i8039_readport,i8039_writeport)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)
	MDRV_INTERLEAVE(50)	/* slices per frame */

	MDRV_MACHINE_RESET(pandoras)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(32*8, 32*8)
	MDRV_VISIBLE_AREA(0*8, 32*8-1, 2*8, 30*8-1)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(32)
	MDRV_COLORTABLE_LENGTH(16*16+16*16)

	MDRV_PALETTE_INIT(pandoras)
	MDRV_VIDEO_START(pandoras)
	MDRV_VIDEO_UPDATE(pandoras)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")

	MDRV_SOUND_ADD(AY8910, 14318000/8)
	MDRV_SOUND_CONFIG(ay8910_interface)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.40)

	MDRV_SOUND_ADD(DAC, 0)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.25)
MACHINE_DRIVER_END


/***************************************************************************

  Game ROMs

***************************************************************************/

ROM_START( pandoras )
	ROM_REGION( 0x10000, REGION_CPU1, 0 ) /* 64K for the CPU A */
	ROM_LOAD( "pand_j13.cpu",	0x08000, 0x02000, CRC(7a0fe9c5) SHA1(e68c8d76d1abb69ac72b0e2cd8c1dfc540064ee3) )
	ROM_LOAD( "pand_j12.cpu",	0x0a000, 0x02000, CRC(7dc4bfe1) SHA1(359c3051e5d7a34d0e49578e4c168fd19c73e202) )
	ROM_LOAD( "pand_j10.cpu",	0x0c000, 0x02000, CRC(be3af3b7) SHA1(91321b53e17e58b674104cb95b1c35ee8fecae22) )
	ROM_LOAD( "pand_j9.cpu",	0x0e000, 0x02000, CRC(e674a17a) SHA1(a4b096dc455425dd60298acf2203659ef6f8d857) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 ) /* 64K for the CPU B */
	ROM_LOAD( "pand_j5.cpu",	0x0e000, 0x02000, CRC(4aab190b) SHA1(d2204953d6b6b34cea851bfc9c2b31426e75f90b) )

	ROM_REGION( 0x10000, REGION_CPU3, 0 ) /* 64K for the Sound CPU */
	ROM_LOAD( "pand_6c.snd",	0x00000, 0x02000, CRC(0c1f109d) SHA1(4e6cdee99261764bd2fea5abbd49d800baba0dc5) )

	ROM_REGION( 0x1000, REGION_CPU4, 0 ) /* 4K for the Sound CPU 2 */
	ROM_LOAD( "pand_7e.snd",	0x00000, 0x01000, CRC(18b0f9d0) SHA1(2a6119423222577a4c2b99ed78f61ba387eec7f8) )

	ROM_REGION( 0x4000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "pand_a18.cpu",	0x00000, 0x02000, CRC(23706d4a) SHA1(cca92e6ff90e3006a79a214f1211fd659771de53) )	/* tiles */
	ROM_LOAD( "pand_a19.cpu",	0x02000, 0x02000, CRC(a463b3f9) SHA1(549b7ee6e47325b80186441da11879fb8b1b47be) )

	ROM_REGION( 0x6000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "pand_j18.cpu",	0x00000, 0x02000, CRC(99a696c5) SHA1(35a27cd5ecc51a9a1acf01eb8078a1028f03be32) )	/* sprites */
	ROM_LOAD( "pand_j17.cpu",	0x02000, 0x02000, CRC(38a03c21) SHA1(b0c8f642787bab3cd1d76657e56f07f4f6f9073c) )
	ROM_LOAD( "pand_j16.cpu",	0x04000, 0x02000, CRC(e0708a78) SHA1(9dbd08b6ca8a66a61e128d1806888696273de848) )

	ROM_REGION( 0x0220, REGION_PROMS, 0 )
	ROM_LOAD( "pandora.2a",		0x0000, 0x020, CRC(4d56f939) SHA1(a8dac604bfdaf4b153b75dbf165de113152b6daa) ) /* palette */
	ROM_LOAD( "pandora.17g",	0x0020, 0x100, CRC(c1a90cfc) SHA1(c6581f2d543e38f1de399774183cf0698e61dab5) ) /* sprite lookup table */
	ROM_LOAD( "pandora.16b",	0x0120, 0x100, CRC(c89af0c3) SHA1(4072c8d61521b34ce4dbce1d48f546402e9539cd) ) /* character lookup table */
ROM_END



GAME( 1984, pandoras, 0, pandoras, pandoras, 0, ROT90, "Konami/Interlogic", "Pandora's Palace", 0 )
