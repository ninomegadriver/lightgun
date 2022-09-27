/***************************************************************************

    Bogey Manor               (c) 1985 Technos Japan

    This game runs on Data East designed hardware.

    Emulation by Bryan McPhail, mish@tendril.co.uk and T.Nogi

***************************************************************************/

#include "driver.h"
#include "cpu/m6502/m6502.h"
#include "sound/ay8910.h"
extern UINT8 *bogeyman_videoram2, *bogeyman_colorram2;

extern WRITE8_HANDLER( bogeyman_videoram_w );
extern WRITE8_HANDLER( bogeyman_colorram_w );
extern WRITE8_HANDLER( bogeyman_videoram2_w );
extern WRITE8_HANDLER( bogeyman_colorram2_w );
extern WRITE8_HANDLER( bogeyman_paletteram_w );

extern PALETTE_INIT( bogeyman );
extern VIDEO_START( bogeyman );
extern VIDEO_UPDATE( bogeyman );

/* Read/Write Handlers */

// Sound section is copied from Mysterious Stones driver by Nicola, Mike, Brad

static int psg_latch;

static WRITE8_HANDLER( bogeyman_8910_latch_w )
{
	psg_latch = data;
}

static WRITE8_HANDLER( bogeyman_8910_control_w )
{
	static int last;

	// bit 0 is flipscreen
	flip_screen_set(data & 0x01);

	// bit 5 goes to 8910 #0 BDIR pin
	if ((last & 0x20) == 0x20 && (data & 0x20) == 0x00)
	{
		// bit 4 goes to the 8910 #0 BC1 pin
		if (last & 0x10)
			AY8910_control_port_0_w(0,psg_latch);
		else
			AY8910_write_port_0_w(0,psg_latch);
	}

	// bit 7 goes to 8910 #1 BDIR pin
	if ((last & 0x80) == 0x80 && (data & 0x80) == 0x00)
	{
		// bit 6 goes to the 8910 #1 BC1 pin
		if (last & 0x40)
			AY8910_control_port_1_w(0,psg_latch);
		else
			AY8910_write_port_1_w(0,psg_latch);
	}

	last = data;
}

/* Memory Map */

static ADDRESS_MAP_START( bogeyman_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x17ff) AM_RAM
	AM_RANGE(0x1800, 0x1bff) AM_RAM AM_WRITE(bogeyman_videoram2_w) AM_BASE(&bogeyman_videoram2)
	AM_RANGE(0x1c00, 0x1fff) AM_RAM AM_WRITE(bogeyman_colorram2_w) AM_BASE(&bogeyman_colorram2)
  	AM_RANGE(0x2000, 0x20ff) AM_RAM AM_WRITE(bogeyman_videoram_w) AM_BASE(&videoram)
  	AM_RANGE(0x2100, 0x21ff) AM_RAM AM_WRITE(bogeyman_colorram_w) AM_BASE(&colorram)
	AM_RANGE(0x2800, 0x2bff) AM_RAM AM_BASE(&spriteram) AM_SIZE(&spriteram_size)
	AM_RANGE(0x3000, 0x300f) AM_RAM AM_WRITE(bogeyman_paletteram_w) AM_BASE(&paletteram)
	AM_RANGE(0x3800, 0x3800) AM_READWRITE(input_port_0_r, bogeyman_8910_control_w)
	AM_RANGE(0x3801, 0x3801) AM_READWRITE(input_port_1_r, bogeyman_8910_latch_w)
	AM_RANGE(0x3802, 0x3802) AM_READ(input_port_2_r)
	AM_RANGE(0x3803, 0x3803) AM_READ(input_port_3_r) AM_WRITENOP // ??? sound
	AM_RANGE(0x4000, 0xffff) AM_ROM
ADDRESS_MAP_END

/* Input Ports */

INPUT_PORTS_START( bogeyman )
	PORT_START_TAG("IN0")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  ) PORT_8WAY
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    ) PORT_8WAY
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  ) PORT_8WAY
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START2 )

	PORT_START_TAG("IN1")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_COCKTAIL
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_COCKTAIL
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  ) PORT_8WAY PORT_COCKTAIL
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    ) PORT_8WAY PORT_COCKTAIL
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  ) PORT_8WAY PORT_COCKTAIL
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY PORT_COCKTAIL
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_VBLANK )

	PORT_START_TAG("DSW1")
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
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x10, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Cocktail ) )
	PORT_SERVICE( 0x40, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Allow_Continue ) )
	PORT_DIPSETTING(    0x00, DEF_STR( No ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Yes ) )

	PORT_START_TAG("DSW2")
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x01, "3" )
	PORT_DIPSETTING(    0x00, "5" )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x02, "50K" )
	PORT_DIPSETTING(    0x00, DEF_STR( None ) )
	PORT_DIPNAME( 0x0c, 0x0c, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Easy ) )
	PORT_DIPSETTING(    0x0c, DEF_STR( Normal ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Hard ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Hardest ) )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_SERVICE1 )
INPUT_PORTS_END

/* Graphics Layouts */

static const gfx_layout charlayout1 =
{
	8, 8,
	512,
	3,
	{ 0x8000*8+4, 0, 4 },
	{ 0x2000*8+3, 0x2000*8+2, 0x2000*8+1, 0x2000*8+0, 3, 2, 1, 0 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8
};

static const gfx_layout charlayout2 =
{
	8, 8,
	512,
	3,
	{ 0x8000*8, 0+0x1000*8, 4+0x1000*8, 0 },
	{ 0x2000*8+3, 0x2000*8+2, 0x2000*8+1, 0x2000*8+0, 3, 2, 1, 0 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8
};

static const gfx_layout tiles1a =
{
	16, 16,
	128,
	3,
	{ 0x8000*8+4, 0, 4 },
	{ 1024*8*8+3, 1024*8*8+2, 1024*8*8+1, 1024*8*8+0, 3, 2, 1, 0,
	  1024*8*8+3+64, 1024*8*8+2+64, 1024*8*8+1+64, 1024*8*8+0+64, 3+64,2+64,1+64,0+64 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8,
	  0*8+16*8, 1*8+16*8, 2*8+16*8, 3*8+16*8, 4*8+16*8, 5*8+16*8, 6*8+16*8, 7*8+16*8 },
	32*8
};

static const gfx_layout tiles1b =
{
	16, 16,
	128,
	3,
	{ 0x8000*8+0, 0+0x1000*8+0, 4+0x1000*8 },
	{ 1024*8*8+3, 1024*8*8+2, 1024*8*8+1, 1024*8*8+0, 3, 2, 1, 0,
	  1024*8*8+3+64, 1024*8*8+2+64, 1024*8*8+1+64, 1024*8*8+0+64, 3+64,2+64, 1+64,0+64 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8,
	  0*8+16*8, 1*8+16*8, 2*8+16*8, 3*8+16*8, 4*8+16*8, 5*8+16*8, 6*8+16*8, 7*8+16*8 },
	32*8
};

static const gfx_layout sprites =
{
	16, 16,
	512,
	3,
 	{ 0x8000*8, 0x4000*8, 0 },
	{ 16*8, 1+(16*8), 2+(16*8), 3+(16*8), 4+(16*8), 5+(16*8), 6+(16*8), 7+(16*8),
	  0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8,
	  8*8, 9*8, 10*8, 11*8, 12*8, 13*8, 14*8, 15*8 },
	16*16
};

/* Graphics Decode Information */

static const gfx_decode gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0x00000, &charlayout1,     16, 32 },
	{ REGION_GFX1, 0x00000, &charlayout2,     16, 32 },
	{ REGION_GFX2, 0x00000, &sprites,          0,  2 },
	{ REGION_GFX3, 0x00000, &tiles1a,     16+128,  8 },
	{ REGION_GFX3, 0x00000, &tiles1b,     16+128,  8 },
	{ REGION_GFX3, 0x04000, &tiles1a,     16+128,  8 },
	{ REGION_GFX3, 0x04000, &tiles1b,     16+128,  8 },
	// colors 16+192 to 16+255 are currently unassigned
	{ -1 }
};


/* Machine Driver */

static MACHINE_DRIVER_START( bogeyman )
	// basic machine hardware
	MDRV_CPU_ADD(M6502, 2000000)	// 12 MHz clock on board
	MDRV_CPU_PROGRAM_MAP(bogeyman_map, 0)
	MDRV_CPU_VBLANK_INT(irq0_line_hold, 16) // Controls sound

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_REAL_60HZ_VBLANK_DURATION)

	// video hardware
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER | VIDEO_UPDATE_BEFORE_VBLANK)
	MDRV_SCREEN_SIZE(32*8, 32*8)
	MDRV_VISIBLE_AREA(0*8, 32*8-1, 1*8, 31*8-1)

	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(16+256)
	MDRV_COLORTABLE_LENGTH(16+256)

	MDRV_PALETTE_INIT(bogeyman)
	MDRV_VIDEO_START(bogeyman)
	MDRV_VIDEO_UPDATE(bogeyman)

	// sound hardware
	MDRV_SPEAKER_STANDARD_MONO("mono")

	MDRV_SOUND_ADD(AY8910, 1500000)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.30)

	MDRV_SOUND_ADD(AY8910, 1500000)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.30)
MACHINE_DRIVER_END

/* ROMs */

ROM_START( bogeyman )
	ROM_REGION( 0x58000, REGION_CPU1, 0 )
 	ROM_LOAD( "j20.c14",  0x04000, 0x04000, CRC(ea90d637) SHA1(aa89bee806badb05119516d84e7674cd302aaf4e) )
	ROM_LOAD( "j10.c15",  0x08000, 0x04000, CRC(0a8f218d) SHA1(5e5958cccfe634e3d274d187a0a7fe4789f3a9c3) )
	ROM_LOAD( "j00.c17",  0x0c000, 0x04000, CRC(5d486de9) SHA1(40ea14a4a25f8f38d33a8844f627ba42503e1280) )

	ROM_REGION( 0x10000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "j70.h15",  0x00000, 0x04000, CRC(fdc787bf) SHA1(1f185a1927fff6ce793d673ebd882a852ac547e4) )	/* Characters */
	ROM_LOAD( "j60.c17",  0x08000, 0x01000, CRC(cc03ceb2) SHA1(0149eacac2c1469be6e19f7a43c13d1fe8790f2c) )
	ROM_CONTINUE(         0x0a000, 0x01000 )

	ROM_REGION( 0x0c000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "j30.c9",   0x00000, 0x04000, CRC(41af81c0) SHA1(d8465622cdf16bc906818641d7988fc412454a45) )	/* Sprites */
	ROM_LOAD( "j40.c7",   0x04000, 0x04000, CRC(8b438421) SHA1(295806c119f4ddc01afc15550e1ff397fbf5d862) )
	ROM_LOAD( "j50.c5",   0x08000, 0x04000, CRC(b507157f) SHA1(471f67eb5e7aedef52353581405d9613d2a86898) )

	ROM_REGION( 0x10000, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_LOAD( "j90.h12",  0x00000, 0x04000, CRC(46b2d4d0) SHA1(35cd320d4db7aa6a89f83ba4d9ff88925357d640) )	/* Tiles */
	ROM_LOAD( "j80.h13",  0x04000, 0x04000, CRC(77ebd0a4) SHA1(c6921ee59633eeeda97c73cb7833578fa8a84fa3) )
	ROM_LOAD( "ja0.h10",  0x08000, 0x01000, CRC(f2aa05ed) SHA1(e6df96e4128eff6de7e6483254608dd8a7b258b9) )
	ROM_CONTINUE(         0x0a000, 0x01000 )
	ROM_CONTINUE(         0x0c000, 0x01000 )
	ROM_CONTINUE(         0x0e000, 0x01000 )

	ROM_REGION( 0x0200, REGION_PROMS, 0 )
	ROM_LOAD( "82s129.5k",  0x0000, 0x0100, CRC(4a7c5367) SHA1(a67f5b90c18238cbfb1507230b4614191d37eef4) )	/* Colour prom 1 */
	ROM_LOAD( "82s129.6k",  0x0100, 0x0100, CRC(b6127713) SHA1(5bd8627453916ac6605af7d1193f79c748eab981) )	/* Colour prom 2 */
ROM_END

/* Game Driver */

GAME( 1985?, bogeyman, 0, bogeyman, bogeyman, 0, ROT0, "Technos Japan", "Bogey Manor", GAME_IMPERFECT_COLORS )
