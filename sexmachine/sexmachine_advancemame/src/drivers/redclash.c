/***************************************************************************

Zero Hour / Red Clash

runs on hardware similar to Lady Bug

driver by inkling

Notes:
- In the Tehkan set (redclsha) the ship doesn't move during attract mode. Earlier version?
  Gameplay is different too.

TODO:
- Colors might be right, need screen shots to verify

- Some graphical problems in both games, but without screenshots its hard to
  know what we're aiming for

- Sound (analog, schematics available for Zero Hour)

***************************************************************************/

#include "driver.h"


extern WRITE8_HANDLER( redclash_videoram_w );
extern WRITE8_HANDLER( redclash_gfxbank_w );
extern WRITE8_HANDLER( redclash_flipscreen_w );

extern WRITE8_HANDLER( redclash_star0_w );
extern WRITE8_HANDLER( redclash_star1_w );
extern WRITE8_HANDLER( redclash_star2_w );
extern WRITE8_HANDLER( redclash_star_reset_w );

extern PALETTE_INIT( redclash );
extern VIDEO_START( redclash );
extern VIDEO_UPDATE( redclash );
extern VIDEO_EOF( redclash );

/*
  This game doesn't have VBlank interrupts.
  Interrupts are still used, but they are related to coin
  slots. Left slot generates an IRQ, Right slot a NMI.
*/
INTERRUPT_GEN( redclash_interrupt )
{
	if (readinputport(4) & 1)	/* Left Coin */
		cpunum_set_input_line(0,0,ASSERT_LINE);
	else if (readinputport(4) & 2)	/* Right Coin */
		cpunum_set_input_line(0, INPUT_LINE_NMI, PULSE_LINE);
}

static WRITE8_HANDLER( irqack_w )
{
	cpunum_set_input_line(0,0,CLEAR_LINE);
}



static ADDRESS_MAP_START( zero_readmem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x2fff) AM_READ(MRA8_ROM)
	AM_RANGE(0x3000, 0x37ff) AM_READ(MRA8_RAM)
	AM_RANGE(0x4800, 0x4800) AM_READ(input_port_0_r) /* IN0 */
	AM_RANGE(0x4801, 0x4801) AM_READ(input_port_1_r) /* IN1 */
	AM_RANGE(0x4802, 0x4802) AM_READ(input_port_2_r) /* DSW0 */
	AM_RANGE(0x4803, 0x4803) AM_READ(input_port_3_r) /* DSW1 */
	AM_RANGE(0x4000, 0x43ff) AM_READ(MRA8_RAM)  /* video RAM */
ADDRESS_MAP_END

static ADDRESS_MAP_START( zero_writemem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x2fff) AM_WRITE(MWA8_ROM)
	AM_RANGE(0x3000, 0x37ff) AM_WRITE(MWA8_RAM)
	AM_RANGE(0x3800, 0x3bff) AM_WRITE(MWA8_RAM) AM_BASE(&spriteram) AM_SIZE(&spriteram_size)
	AM_RANGE(0x4000, 0x43ff) AM_WRITE(redclash_videoram_w) AM_BASE(&videoram)
	AM_RANGE(0x5000, 0x5007) AM_WRITE(MWA8_NOP)	/* to sound board */
	AM_RANGE(0x5800, 0x5800) AM_WRITE(redclash_star0_w)
	AM_RANGE(0x5801, 0x5804) AM_WRITE(MWA8_NOP)	/* to sound board */
	AM_RANGE(0x5805, 0x5805) AM_WRITE(redclash_star1_w)
	AM_RANGE(0x5806, 0x5806) AM_WRITE(redclash_star2_w)
	AM_RANGE(0x5807, 0x5807) AM_WRITE(redclash_flipscreen_w)
	AM_RANGE(0x7000, 0x7000) AM_WRITE(redclash_star_reset_w)
	AM_RANGE(0x7800, 0x7800) AM_WRITE(irqack_w)
ADDRESS_MAP_END

static ADDRESS_MAP_START( readmem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x2fff) AM_READ(MRA8_ROM)
	AM_RANGE(0x4800, 0x4800) AM_READ(input_port_0_r) /* IN0 */
	AM_RANGE(0x4801, 0x4801) AM_READ(input_port_1_r) /* IN1 */
	AM_RANGE(0x4802, 0x4802) AM_READ(input_port_2_r) /* DSW0 */
	AM_RANGE(0x4803, 0x4803) AM_READ(input_port_3_r) /* DSW1 */
	AM_RANGE(0x4000, 0x43ff) AM_READ(MRA8_RAM)  /* video RAM */
	AM_RANGE(0x6000, 0x67ff) AM_READ(MRA8_RAM)
ADDRESS_MAP_END

static ADDRESS_MAP_START( writemem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x2fff) AM_WRITE(MWA8_ROM)
//  AM_RANGE(0x3000, 0x3000) AM_WRITE(MWA8_NOP)
//  AM_RANGE(0x3800, 0x3800) AM_WRITE(MWA8_NOP)
	AM_RANGE(0x4000, 0x43ff) AM_WRITE(redclash_videoram_w) AM_BASE(&videoram)
	AM_RANGE(0x5000, 0x5007) AM_WRITE(MWA8_NOP)	/* to sound board */
	AM_RANGE(0x5800, 0x5800) AM_WRITE(redclash_star0_w)
	AM_RANGE(0x5801, 0x5801) AM_WRITE(redclash_gfxbank_w)
	AM_RANGE(0x5805, 0x5805) AM_WRITE(redclash_star1_w)
	AM_RANGE(0x5806, 0x5806) AM_WRITE(redclash_star2_w)
	AM_RANGE(0x5807, 0x5807) AM_WRITE(redclash_flipscreen_w)
	AM_RANGE(0x6000, 0x67ff) AM_WRITE(MWA8_RAM)
	AM_RANGE(0x6800, 0x6bff) AM_WRITE(MWA8_RAM) AM_BASE(&spriteram) AM_SIZE(&spriteram_size)
	AM_RANGE(0x7000, 0x7000) AM_WRITE(redclash_star_reset_w)
	AM_RANGE(0x7800, 0x7800) AM_WRITE(irqack_w)
ADDRESS_MAP_END



INPUT_PORTS_START( redclash )
	PORT_START	/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY PORT_COCKTAIL
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY PORT_COCKTAIL
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY PORT_COCKTAIL
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY PORT_COCKTAIL
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_COCKTAIL
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	/* Note that there are TWO VBlank inputs, one is active low, the other active */
	/* high. There are probably other differencies in the hardware, but emulating */
	/* them this way is enough to get the game running. */
	PORT_BIT( 0xc0, 0x40, IPT_VBLANK )

	PORT_START	/* DSW0 */
	PORT_DIPNAME( 0x03, 0x03, "Difficulty?" )
	PORT_DIPSETTING(    0x03, "Easy?" )
	PORT_DIPSETTING(    0x02, "Medium?" )
	PORT_DIPSETTING(    0x01, "Hard?" )
	PORT_DIPSETTING(    0x00, "Hardest?" )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0x10, 0x10, "High Score" )
	PORT_DIPSETTING(    0x00, "0" )
	PORT_DIPSETTING(    0x10, "10000" )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0xc0, 0xc0, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x00, "1" )
	PORT_DIPSETTING(    0xc0, "3" )
	PORT_DIPSETTING(    0x80, "5" )
	PORT_DIPSETTING(    0x40, "7" )

	PORT_START	/* DSW1 */
	PORT_DIPNAME( 0x0f, 0x0f, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 6C_1C ) )
	PORT_DIPSETTING(    0x05, DEF_STR( 5C_1C ) )
	PORT_DIPSETTING(    0x06, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x0a, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x07, DEF_STR( 3C_2C ) )
	PORT_DIPSETTING(    0x0f, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x09, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(    0x0e, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x0d, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x0c, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x0b, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 1C_6C ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 1C_7C ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 1C_8C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_9C ) )
	PORT_DIPNAME( 0xf0, 0xf0, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x40, DEF_STR( 6C_1C ) )
	PORT_DIPSETTING(    0x50, DEF_STR( 5C_1C ) )
	PORT_DIPSETTING(    0x60, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x80, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0xa0, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x70, DEF_STR( 3C_2C ) )
	PORT_DIPSETTING(    0xf0, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x90, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(    0xe0, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0xd0, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0xc0, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0xb0, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(    0x30, DEF_STR( 1C_6C ) )
	PORT_DIPSETTING(    0x20, DEF_STR( 1C_7C ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 1C_8C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_9C ) )

	PORT_START	/* FAKE */
	/* The coin slots are not memory mapped. Coin Left causes a NMI, */
	/* Coin Right an IRQ. This fake input port is used by the interrupt */
	/* handler to be notified of coin insertions. We use IMPULSE to */
	/* trigger exactly one interrupt, without having to check when the */
	/* user releases the key. */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_COIN1 ) PORT_IMPULSE(1)
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_COIN2 ) PORT_IMPULSE(1)
INPUT_PORTS_END

INPUT_PORTS_START( zerohour )
	PORT_START	/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY PORT_COCKTAIL
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY PORT_COCKTAIL
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY PORT_COCKTAIL
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY PORT_COCKTAIL
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_COCKTAIL
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	/* Note that there are TWO VBlank inputs, one is active low, the other active */
	/* high. There are probably other differencies in the hardware, but emulating */
	/* them this way is enough to get the game running. */
	PORT_BIT( 0xc0, 0x40, IPT_VBLANK )

	PORT_START	/* DSW0 */
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
	PORT_DIPNAME( 0x08, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0xc0, 0xc0, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x00, "2" )
	PORT_DIPSETTING(    0xc0, "3" )
	PORT_DIPSETTING(    0x80, "4" )
	PORT_DIPSETTING(    0x40, "5" )

	PORT_START	/* DSW1 */
	PORT_DIPNAME( 0x0f, 0x0f, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x06, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x0a, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x07, DEF_STR( 3C_2C ) )
	PORT_DIPSETTING(    0x0f, DEF_STR( 1C_1C ) ) 	/* all other combinations give 1C_1C */
	PORT_DIPSETTING(    0x09, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(    0x0e, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x0d, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x0c, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x0b, DEF_STR( 1C_5C ) )
	PORT_DIPNAME( 0xf0, 0xf0, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x60, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x80, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0xa0, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x70, DEF_STR( 3C_2C ) )
	PORT_DIPSETTING(    0xf0, DEF_STR( 1C_1C ) )	/* all other combinations give 1C_1C */
	PORT_DIPSETTING(    0x90, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(    0xe0, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0xd0, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0xc0, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0xb0, DEF_STR( 1C_5C ) )

	PORT_START	/* FAKE */
	/* The coin slots are not memory mapped. Coin Left causes a NMI, */
	/* Coin Right an IRQ. This fake input port is used by the interrupt */
	/* handler to be notified of coin insertions. We use IMPULSE to */
	/* trigger exactly one interrupt, without having to check when the */
	/* user releases the key. */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_COIN1 ) PORT_IMPULSE(1)
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_COIN2 ) PORT_IMPULSE(1)
INPUT_PORTS_END

static const gfx_layout charlayout =
{
	8,8,
	RGN_FRAC(1,1),
	2,
	{ 8*8, 0 },
	{ 7, 6, 5, 4, 3, 2, 1, 0 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	16*8
};

static const gfx_layout spritelayout8x8 =
{
	8,8,
	RGN_FRAC(1,1),
	2,
	{ 1, 0 },
	{ STEP8(0,2) },
	{ STEP8(7*16,-16) },
	16*8
};

static const gfx_layout spritelayout16x16 =
{
	16,16,
	RGN_FRAC(1,1),
	2,
	{ 1, 0 },
	{ STEP8(24*2,2), STEP8(8*64+24*2,2) },
	{ STEP8(23*64,-64), STEP8(7*64,-64) },
	64*32
};

static const gfx_layout spritelayout24x24 =
{
	24,24,
	RGN_FRAC(1,1),
	2,
	{ 1, 0 },
	{ STEP8(0,2), STEP8(8*2,2), STEP8(16*2,2) },
	{ STEP8(23*64,-64), STEP8(15*64,-64), STEP8(7*64,-64) },
	64*32
};

static const gfx_layout spritelayout16x16bis =
{
	16,16,
	RGN_FRAC(1,1),
	2,
	{ 1, 0 },
	{ STEP8(0,2), STEP8(8*2,2) },
	{ STEP8(15*64,-64), STEP8(7*64,-64) },
	32*32
};

static const gfx_decode gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0x0000, &charlayout,          0,  8 },
	{ REGION_GFX3, 0x0000, &spritelayout8x8,   4*8, 16 },
	{ REGION_GFX2, 0x0000, &spritelayout16x16, 4*8, 16 },
	{ REGION_GFX2, 0x0000, &spritelayout24x24, 4*8, 16 },
	{ REGION_GFX2, 0x0000, &spritelayout16x16bis, 4*8, 16 },
	{ REGION_GFX2, 0x0004, &spritelayout16x16bis, 4*8, 16 },
	{ -1 } /* end of array */
};



static MACHINE_DRIVER_START( zerohour )

	/* basic machine hardware */
	MDRV_CPU_ADD(Z80, 4000000)  /* 4 MHz */
	MDRV_CPU_PROGRAM_MAP(zero_readmem,zero_writemem)
	MDRV_CPU_VBLANK_INT(redclash_interrupt,1)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_REAL_60HZ_VBLANK_DURATION)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(32*8, 32*8)
	MDRV_VISIBLE_AREA(1*8, 31*8-1, 4*8, 28*8-1)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(64)
	MDRV_COLORTABLE_LENGTH(4*24)

	MDRV_PALETTE_INIT(redclash)
	MDRV_VIDEO_START(redclash)
	MDRV_VIDEO_UPDATE(redclash)
	MDRV_VIDEO_EOF(redclash)

	/* sound hardware */
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( redclash )

	/* basic machine hardware */
	MDRV_CPU_ADD(Z80, 4000000)  /* 4 MHz */
	MDRV_CPU_PROGRAM_MAP(readmem,writemem)
	MDRV_CPU_VBLANK_INT(redclash_interrupt,1)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_REAL_60HZ_VBLANK_DURATION)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(32*8, 32*8)
	MDRV_VISIBLE_AREA(1*8, 31*8-1, 4*8, 28*8-1)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(64)
	MDRV_COLORTABLE_LENGTH(4*24)

	MDRV_PALETTE_INIT(redclash)
	MDRV_VIDEO_START(redclash)
	MDRV_VIDEO_UPDATE(redclash)
	MDRV_VIDEO_EOF(redclash)

	/* sound hardware */
MACHINE_DRIVER_END

/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START( zerohour )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "zerohour.1",   0x0000, 0x0800, CRC(0dff4b48) SHA1(4911255f953851d0e5c2b66090b95254ac59ac9e) )
	ROM_LOAD( "zerohour.2",   0x0800, 0x0800, CRC(cf41b6ac) SHA1(263794e6be22c20e2b10fe9099e475097475df7b) )
	ROM_LOAD( "zerohour.3",	  0x1000, 0x0800, CRC(5ef48b67) SHA1(ae291aa84b109e6a51eebdd5526abca1d901b7b9) )
	ROM_LOAD( "zerohour.4",	  0x1800, 0x0800, CRC(25c5872d) SHA1(df008db607b72a92c4284d6a8127eafec2432ca4) )
	ROM_LOAD( "zerohour.5",	  0x2000, 0x0800, CRC(d7ce3add) SHA1(d8dd7ad98e7a0a4f35de181549b2e88a9e0a73d6) )
	ROM_LOAD( "zerohour.6",	  0x2800, 0x0800, CRC(8a93ae6e) SHA1(a66f05bb27e67b755c64ac8b68fa38ffe4cd961c) )

	ROM_REGION( 0x0800, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "zerohour.9",   0x0000, 0x0800, CRC(17ae6f13) SHA1(ce7a02f4e1aa2e5292d3807a0cfed6d92752fc7a) )

	ROM_REGION( 0x1000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "zerohour.7",	  0x0000, 0x0800, CRC(4c12f59d) SHA1(b99a21415bff0e59b6130df60182f05b1a5d0811) )
	ROM_LOAD( "zerohour.8",	  0x0800, 0x0800, CRC(6b9a6b6e) SHA1(f80d893b1b26c75c297e1da1c20db04e7129c92a) )

	ROM_REGION( 0x1000, REGION_GFX3, ROMREGION_DISPOSE )
	/* gfx data will be rearranged here for 8x8 sprites */

	ROM_REGION( 0x0600, REGION_PROMS, 0 )
	ROM_LOAD( "zerohour.ic2", 0x0000, 0x0020, CRC(b55aee56) SHA1(33e4767c8afbb7b3af67517ea1dfd69bf692cac7) ) /* palette */
	ROM_LOAD( "zerohour.n2",  0x0020, 0x0020, CRC(9adabf46) SHA1(f3538fdbc4280b6be46a4d7ebb4c34bd1a1ce2b7) ) /* sprite color lookup table */
	ROM_LOAD( "zerohour.u6",  0x0040, 0x0020, CRC(27fa3a50) SHA1(7cf59b7a37c156640d6ea91554d1c4276c1780e0) ) /* ?? */
ROM_END

ROM_START( redclash )
	ROM_REGION(0x10000, REGION_CPU1, 0 ) /* 64k for code */
	ROM_LOAD( "11.11c",       0x0000, 0x1000, CRC(695e070e) SHA1(8d0451a05572f62e0f282ab96bdd26d08b77a6c9) )
	ROM_LOAD( "13.7c",        0x1000, 0x1000, CRC(c2090318) SHA1(71725cdf51aedf5f29fa1dd1a41ad5e62c9a580d) )
	ROM_LOAD( "12.9c",        0x2000, 0x1000, CRC(b60e5ada) SHA1(37440f382c5e8852d804fa9837c36cc1e9d94d1d) )

	ROM_REGION(0x0800, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "6.12a",        0x0000, 0x0800, CRC(da9bbcc2) SHA1(4cbe03c7f5e99cc2f124e0089ea3c392156b5d92) )

	ROM_REGION( 0x2000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "14.3e",        0x0000, 0x0800, CRC(483a1293) SHA1(e7812475c7509389bcf8fee35598e9894428eb37) )
	ROM_CONTINUE(             0x1000, 0x0800 )
	ROM_LOAD( "15.3d",        0x0800, 0x0800, CRC(c45d9601) SHA1(2f156ad61161d65284df0cc55eb1b3b990eb41cb) )
	ROM_CONTINUE(             0x1800, 0x0800 )

	ROM_REGION( 0x2000, REGION_GFX3, ROMREGION_DISPOSE )
	/* gfx data will be rearranged here for 8x8 sprites */

	ROM_REGION( 0x0060, REGION_PROMS, 0 )
	ROM_LOAD( "1.12f",        0x0000, 0x0020, CRC(43989681) SHA1(0d471e6f499294f2f62f27392b8370e2af8e38a3) ) /* palette */
	ROM_LOAD( "2.4a",         0x0020, 0x0020, CRC(9adabf46) SHA1(f3538fdbc4280b6be46a4d7ebb4c34bd1a1ce2b7) ) /* sprite color lookup table */
	ROM_LOAD( "3.11e",        0x0040, 0x0020, CRC(27fa3a50) SHA1(7cf59b7a37c156640d6ea91554d1c4276c1780e0) ) /* ?? */
ROM_END

ROM_START( redclsha )
	ROM_REGION(0x10000, REGION_CPU1, 0 ) /* 64k for code */
	ROM_LOAD( "rc1.11c",      0x0000, 0x1000, CRC(5b62ff5a) SHA1(981d3c72f28b7d136a0bad9243d39fd1ba3abc97) )
	ROM_LOAD( "rc3.7c",       0x1000, 0x1000, CRC(409c4ee7) SHA1(15c03a4093d7695751a143aa749229fcb7721f46) )
	ROM_LOAD( "rc2.9c",       0x2000, 0x1000, CRC(5f215c9a) SHA1(c305f7be19f6a052c08feb0b63a0326b6a1bd808) )

	ROM_REGION(0x0800, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "rc6.12a",      0x0000, 0x0800, CRC(da9bbcc2) SHA1(4cbe03c7f5e99cc2f124e0089ea3c392156b5d92) )

	ROM_REGION( 0x2000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "rc4.3e",       0x0000, 0x0800, CRC(64ca8b63) SHA1(5fd1ca9b81f66b4d2041674900718dc8c94c2a97) )
	ROM_CONTINUE(             0x1000, 0x0800 )
	ROM_LOAD( "rc5.3d",       0x0800, 0x0800, CRC(fce610a2) SHA1(0be829c6f6f5c3a19056ba1594141c1965c7aa2a) )
	ROM_CONTINUE(             0x1800, 0x0800 )

	ROM_REGION( 0x2000, REGION_GFX3, ROMREGION_DISPOSE )
	/* gfx data will be rearranged here for 8x8 sprites */

	ROM_REGION( 0x0060, REGION_PROMS, 0 )
	ROM_LOAD( "1.12f",        0x0000, 0x0020, CRC(43989681) SHA1(0d471e6f499294f2f62f27392b8370e2af8e38a3) ) /* palette */
	ROM_LOAD( "2.4a",         0x0020, 0x0020, CRC(9adabf46) SHA1(f3538fdbc4280b6be46a4d7ebb4c34bd1a1ce2b7) ) /* sprite color lookup table */
	ROM_LOAD( "3.11e",        0x0040, 0x0020, CRC(27fa3a50) SHA1(7cf59b7a37c156640d6ea91554d1c4276c1780e0) ) /* ?? */
ROM_END

ROM_START( redclask )
	ROM_REGION(0x10000, REGION_CPU1, 0 ) /* 64k for code */
	ROM_LOAD( "rc1.8c",       0x0000, 0x0800, CRC(fd90622a) SHA1(a65a32d519e7fee89b160f8152322df20b6af4ea) )
	ROM_LOAD( "rc2.7c",       0x0800, 0x0800, CRC(c8f33440) SHA1(60d1faee415faa13102b8e744f444f1480b8bd73) )
	ROM_LOAD( "rc3.6c",       0x1000, 0x0800, CRC(2172b1e9) SHA1(b6f7ee8924bda9f8da13baaa2db3ffb7d623236c) )
	ROM_LOAD( "rc4.5c",       0x1800, 0x0800, CRC(55c0d1b5) SHA1(2f1d729d29184de8f8bb914015352730325cda12) )
	ROM_LOAD( "rc5.4c",       0x2000, 0x0800, CRC(744b5261) SHA1(6c5de2f91f57c463230e0ea04b336347840161a3) )
	ROM_LOAD( "rc6.3c",       0x2800, 0x0800, CRC(fa507e17) SHA1(dd0e27b08e902b91c5e9552351206c671ed2f3c0) )

	ROM_REGION( 0x0800, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "rc6.12a",      0x0000, 0x0800, CRC(da9bbcc2) SHA1(4cbe03c7f5e99cc2f124e0089ea3c392156b5d92) ) /* rc9.7m */

	ROM_REGION( 0x2000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "rc4.4m",       0x0000, 0x0800, CRC(483a1293) SHA1(e7812475c7509389bcf8fee35598e9894428eb37) )
	ROM_CONTINUE(             0x1000, 0x0800 )
	ROM_LOAD( "rc5.5m",       0x0800, 0x0800, CRC(c45d9601) SHA1(2f156ad61161d65284df0cc55eb1b3b990eb41cb) )
	ROM_CONTINUE(             0x1800, 0x0800 )

	ROM_REGION( 0x2000, REGION_GFX3, ROMREGION_DISPOSE )
	/* gfx data will be rearranged here for 8x8 sprites */

	ROM_REGION( 0x0060, REGION_PROMS, 0 )
	ROM_LOAD( "1.12f",        0x0000, 0x0020, CRC(43989681) SHA1(0d471e6f499294f2f62f27392b8370e2af8e38a3) ) /* 6331.7e */
	ROM_LOAD( "2.4a",         0x0020, 0x0020, CRC(9adabf46) SHA1(f3538fdbc4280b6be46a4d7ebb4c34bd1a1ce2b7) ) /* 6331.2r */
	ROM_LOAD( "3.11e",        0x0040, 0x0020, CRC(27fa3a50) SHA1(7cf59b7a37c156640d6ea91554d1c4276c1780e0) ) /* 6331.6w */
ROM_END

static DRIVER_INIT( redclash )
{
	int i,j;

	/* rearrange the sprite graphics */
	for (i = 0;i < memory_region_length(REGION_GFX3);i++)
	{
		j = (i & ~0x003e) | ((i & 0x0e) << 2) | ((i & 0x30) >> 3);
		memory_region(REGION_GFX3)[i] = memory_region(REGION_GFX2)[j];
	}
}



GAME( 1980, zerohour, 0,        zerohour, zerohour, redclash, ROT270, "Universal", "Zero Hour",          GAME_NO_SOUND | GAME_WRONG_COLORS | GAME_IMPERFECT_GRAPHICS )
GAME( 1981, redclash, 0,        redclash, redclash, redclash, ROT270, "Tehkan",    "Red Clash (set 1)",  GAME_NO_SOUND | GAME_WRONG_COLORS | GAME_IMPERFECT_GRAPHICS )
GAME( 1981, redclsha, redclash, redclash, redclash, redclash, ROT270, "Tehkan",    "Red Clash (set 2)",  GAME_NO_SOUND | GAME_WRONG_COLORS | GAME_IMPERFECT_GRAPHICS )
GAME( 1981, redclask, redclash, redclash, redclash, redclash, ROT270, "Kaneko",    "Red Clash (Kaneko)", GAME_NO_SOUND | GAME_WRONG_COLORS | GAME_IMPERFECT_GRAPHICS )
