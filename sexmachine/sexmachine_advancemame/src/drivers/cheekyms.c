/*************************************************************************
 Universal Cheeky Mouse Driver
 (c)Lee Taylor May/June 1998, All rights reserved.

 For use only in offical Mame releases.
 Not to be distributed as part of any commerical work.
**************************************************************************/

#include "driver.h"
#include "sound/dac.h"



PALETTE_INIT( cheekyms );
VIDEO_UPDATE( cheekyms );
WRITE8_HANDLER( cheekyms_sprite_w );
WRITE8_HANDLER( cheekyms_port_40_w );
WRITE8_HANDLER( cheekyms_port_80_w );


static ADDRESS_MAP_START( readmem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x1fff) AM_READ(MRA8_ROM)
	AM_RANGE(0x3000, 0x33ff) AM_READ(MRA8_RAM)
	AM_RANGE(0x3800, 0x3bff) AM_READ(MRA8_RAM)	/* screen RAM */
ADDRESS_MAP_END


static ADDRESS_MAP_START( writemem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x1fff) AM_WRITE(MWA8_ROM)
	AM_RANGE(0x3000, 0x33ff) AM_WRITE(MWA8_RAM)
	AM_RANGE(0x3800, 0x3bff) AM_WRITE(videoram_w) AM_BASE(&videoram) AM_SIZE(&videoram_size)
ADDRESS_MAP_END

static ADDRESS_MAP_START( readport, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_FLAGS( AMEF_ABITS(8) )
	AM_RANGE(0x00, 0x00) AM_READ(input_port_0_r)
	AM_RANGE(0x01, 0x01) AM_READ(input_port_1_r)
ADDRESS_MAP_END

static ADDRESS_MAP_START( writeport, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_FLAGS( AMEF_ABITS(8) )
	AM_RANGE(0x20, 0x3f) AM_WRITE(cheekyms_sprite_w)
	AM_RANGE(0x40, 0x40) AM_WRITE(cheekyms_port_40_w)
	AM_RANGE(0x80, 0x80) AM_WRITE(cheekyms_port_80_w)
ADDRESS_MAP_END


static INTERRUPT_GEN( cheekyms_interrupt )
{
	if (readinputport(2) & 1)	/* Coin */
		nmi_line_pulse();
	else
		irq0_line_hold();
}


INPUT_PORTS_START( cheekyms )
	PORT_START      /* IN0 */
	PORT_DIPNAME( 0x03, 0x01, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x00, "2" )
	PORT_DIPSETTING(    0x01, "3" )
	PORT_DIPSETTING(    0x02, "4" )
	PORT_DIPSETTING(    0x03, "5" )
	PORT_DIPNAME( 0x0c, 0x04, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_2C ) )
//PORT_DIPSETTING(    0x0c, DEF_STR( 1C_1C ) )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x20, DEF_STR( On ) )
	PORT_DIPNAME( 0xc0, 0x40, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x40, "3000" )
	PORT_DIPSETTING(    0x80, "4500" )
	PORT_DIPSETTING(    0xc0, "6000" )
	PORT_DIPSETTING(    0x00, DEF_STR( None ) )

	PORT_START      /* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_COCKTAIL
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_COCKTAIL
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_COCKTAIL
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON1 )

	PORT_START	/* FAKE */
	/* The coin slots are not memory mapped. Coin  causes a NMI, */
	/* This fake input port is used by the interrupt */
	/* handler to be notified of coin insertions. We use IMPULSE to */
	/* trigger exactly one interrupt, without having to check when the */
	/* user releases the key. */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_COIN1 ) PORT_IMPULSE(1)
INPUT_PORTS_END



static const gfx_layout charlayout =
{
	8,8,	/* 16*16 sprites */
	256,	/* 64 sprites */
	2,	/* 2 bits per pixel */
	{ 0, 256*8*8 },	/* the two bitplanes are separated */
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8	/* every sprite takes 8 consecutive bytes */
};

static const gfx_layout spritelayout =
{
	16,16,	/* 16*16 sprites */
	64,	/* 64 sprites */
	2,	/* 2 bits per pixel */
	{ 64*32*8, 0 },	/* the two bitplanes are separated */
	{ 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15 },
	{ 0*16, 1*16,  2*16,  3*16,  4*16,  5*16,  6*16,  7*16,
			8*16, 9*16, 10*16, 11*16, 12*16, 13*16, 14*16, 15*16 },
	32*8	/* every sprite takes 32 consecutive bytes */
};



static const gfx_decode gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &charlayout,   0,    32 },
	{ REGION_GFX2, 0, &spritelayout, 32*4, 16 },
	{ -1 } /* end of array */
};


static MACHINE_DRIVER_START( cheekyms )

	/* basic machine hardware */
	MDRV_CPU_ADD(Z80,5000000/2)  /* 2.5 MHz */
	MDRV_CPU_PROGRAM_MAP(readmem,writemem)
	MDRV_CPU_IO_MAP(readport,writeport)
	MDRV_CPU_VBLANK_INT(cheekyms_interrupt,1)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(32*8, 32*8)
	MDRV_VISIBLE_AREA(0*8, 32*8-1, 4*8, 28*8-1)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(64*3)
	MDRV_COLORTABLE_LENGTH(64*3)

	MDRV_PALETTE_INIT(cheekyms)
	MDRV_VIDEO_START(generic)
	MDRV_VIDEO_UPDATE(cheekyms)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")

	MDRV_SOUND_ADD(DAC, 0)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.0)
MACHINE_DRIVER_END




/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START( cheekyms )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "cm03.c5",       0x0000, 0x0800, CRC(1ad0cb40) SHA1(2a751395ac19a3218c22cfd3597f9a17b8e31527) )
	ROM_LOAD( "cm04.c6",       0x0800, 0x0800, CRC(2238f607) SHA1(35df9eb49f6e3c6351fae220d773442cf8536f90) )
	ROM_LOAD( "cm05.c7",       0x1000, 0x0800, CRC(4169eba8) SHA1(52a059f29c724d087483c7cd733f75d7b8a5b103) )
	ROM_LOAD( "cm06.c8",       0x1800, 0x0800, CRC(7031660c) SHA1(1370702e30897e45ee172609c1d983f8a4fdf157) )

	ROM_REGION( 0x1000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "cm01.c1",       0x0000, 0x0800, CRC(26f73bd7) SHA1(fa4db5df5be1a5f4531cba86a83f89b7eb7fa3ec) )
	ROM_LOAD( "cm02.c2",       0x0800, 0x0800, CRC(885887c3) SHA1(62ce8e39d27c0cfea9ebd51757ad31b0baf6b3cd) )

	ROM_REGION( 0x1000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "cm07.n5",       0x0000, 0x0800, CRC(2738c88d) SHA1(3ccd6c1d49bfe2c1b141854ec705e692252e8af8) )
	ROM_LOAD( "cm08.n6",       0x0800, 0x0800, CRC(b3fbd4ac) SHA1(9f45cc6d9e0bf580149e18de5c3e37d4de347b92) )

	ROM_REGION( 0x0060, REGION_PROMS, 0 )
	ROM_LOAD( "cm.m8",         0x0000, 0x0020, CRC(2386bc68) SHA1(6676082860cd8678a71339a352d2c6286e78ba44) )	 /* Character colors \ Selected by Bit 6 of Port 0x80 */
	ROM_LOAD( "cm.m9",         0x0020, 0x0020, CRC(db9c59a5) SHA1(357ed5ac8e954a4c8b4d78d36e57bf2de36c1d57) )	 /* Character colors /                                */
	ROM_LOAD( "cm.p3",         0x0040, 0x0020, CRC(6ac41516) SHA1(05bf40790a0de1e859362df892f7f158c183e247) )  /* Sprite colors */
ROM_END



GAME( 1980?, cheekyms, 0, cheekyms, cheekyms, 0, ROT270, "Universal", "Cheeky Mouse", GAME_WRONG_COLORS | GAME_IMPERFECT_SOUND )
