/******************************************************************************

Super Locomotive

driver by Zsolt Vasvari

TODO:
- Bit 5 in suprloco_control_w is pulsed when loco turns "super". This is supposed
  to make red parts of sprites blink to purple, it's not clear how this is
  implemented in hardware, there's a hack to support it.

******************************************************************************/

#include "driver.h"
#include "vidhrdw/system1.h"
#include "cpu/z80/z80.h"
#include "machine/segacrpt.h"
#include "sound/sn76496.h"

extern unsigned char *suprloco_videoram;

PALETTE_INIT( suprloco );
VIDEO_START( suprloco );
VIDEO_UPDATE( suprloco );
WRITE8_HANDLER( suprloco_videoram_w );
WRITE8_HANDLER( suprloco_scrollram_w );
READ8_HANDLER( suprloco_scrollram_r );
WRITE8_HANDLER( suprloco_control_w );
READ8_HANDLER( suprloco_control_r );


static WRITE8_HANDLER( suprloco_soundport_w )
{
	soundlatch_w(0,data);
	cpunum_set_input_line(1,INPUT_LINE_NMI,PULSE_LINE);
	/* spin for a while to let the Z80 read the command (fixes hanging sound in Regulus) */
	cpu_spinuntil_time(TIME_IN_USEC(50));
}


static ADDRESS_MAP_START( readmem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x7fff) AM_READ(MRA8_ROM)
	AM_RANGE(0x8000, 0xbfff) AM_READ(MRA8_ROM)
	AM_RANGE(0xc000, 0xc1ff) AM_READ(MRA8_RAM)
	AM_RANGE(0xc800, 0xc800) AM_READ(input_port_0_r)
	AM_RANGE(0xd000, 0xd000) AM_READ(input_port_1_r)
	AM_RANGE(0xd800, 0xd800) AM_READ(input_port_2_r)
	AM_RANGE(0xe000, 0xe000) AM_READ(input_port_3_r)
	AM_RANGE(0xe001, 0xe001) AM_READ(input_port_4_r)
	AM_RANGE(0xe801, 0xe801) AM_READ(suprloco_control_r)
	AM_RANGE(0xf000, 0xf6ff) AM_READ(MRA8_RAM)
	AM_RANGE(0xf7e0, 0xf7ff) AM_READ(suprloco_scrollram_r)
	AM_RANGE(0xf800, 0xffff) AM_READ(MRA8_RAM)
ADDRESS_MAP_END

static ADDRESS_MAP_START( writemem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x7fff) AM_WRITE(MWA8_ROM)
	AM_RANGE(0x8000, 0xbfff) AM_WRITE(MWA8_ROM)
	AM_RANGE(0xc000, 0xc1ff) AM_WRITE(MWA8_RAM) AM_BASE(&spriteram) AM_SIZE(&spriteram_size)
	AM_RANGE(0xe800, 0xe800) AM_WRITE(suprloco_soundport_w)
	AM_RANGE(0xe801, 0xe801) AM_WRITE(suprloco_control_w)
	AM_RANGE(0xf000, 0xf6ff) AM_WRITE(suprloco_videoram_w) AM_BASE(&suprloco_videoram)
	AM_RANGE(0xf7e0, 0xf7ff) AM_WRITE(suprloco_scrollram_w)
	AM_RANGE(0xf800, 0xffff) AM_WRITE(MWA8_RAM)
ADDRESS_MAP_END


static ADDRESS_MAP_START( sound_readmem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x7fff) AM_READ(MRA8_ROM)
	AM_RANGE(0x8000, 0x87ff) AM_READ(MRA8_RAM)
	AM_RANGE(0xe000, 0xe000) AM_READ(soundlatch_r)
ADDRESS_MAP_END

static ADDRESS_MAP_START( sound_writemem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x7fff) AM_WRITE(MWA8_ROM)
	AM_RANGE(0x8000, 0x87ff) AM_WRITE(MWA8_RAM)
	AM_RANGE(0xa000, 0xa003) AM_WRITE(SN76496_0_w)
	AM_RANGE(0xc000, 0xc003) AM_WRITE(SN76496_1_w)
ADDRESS_MAP_END



INPUT_PORTS_START( suprloco )
	PORT_START_TAG("IN0")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_SERVICE_NO_TOGGLE(0x04, IP_ACTIVE_LOW)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_COIN3 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0xc0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START_TAG("IN1")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0xc0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START_TAG("IN2")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY PORT_COCKTAIL
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY PORT_COCKTAIL
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY PORT_COCKTAIL
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY PORT_COCKTAIL
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_COCKTAIL
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_COCKTAIL
	PORT_BIT( 0xc0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START_TAG("DSW1")
	PORT_DIPNAME( 0x07, 0x00, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x07, DEF_STR( 5C_1C ) )
	PORT_DIPSETTING(    0x06, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x05, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 1C_6C ) )
	PORT_DIPNAME( 0x38, 0x00, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x38, DEF_STR( 5C_1C ) )
	PORT_DIPSETTING(    0x30, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x28, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x20, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x18, DEF_STR( 1C_6C ) )
	PORT_DIPNAME( 0xc0, 0x40, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x00, "2" )
	PORT_DIPSETTING(    0x40, "3" )
	PORT_DIPSETTING(    0x80, "4" )
	PORT_DIPSETTING(    0xc0, "5" )

	PORT_START_TAG("DSW2")
	PORT_DIPNAME( 0x03, 0x00, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x00, "20000" )
	PORT_DIPSETTING(    0x01, "30000" )
	PORT_DIPSETTING(    0x02, "40000" )
	PORT_DIPSETTING(    0x03, "50000" )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Free_Play ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Easy ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Hard ) )
	PORT_DIPNAME( 0x20, 0x20, "Infinite Lives (Cheat)")
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, "Initial Entry" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x40, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Cocktail ) )

INPUT_PORTS_END


static const gfx_layout charlayout =
{
	8,8,	/* 8 by 8 */
	1024,	/* 1024 characters */
	4,		/* 4 bits per pixel */
	{ 0, 1024*8*8, 2*1024*8*8, 3*1024*8*8 },			/* plane */
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8
};


static const gfx_decode gfxdecodeinfo[] =
{
	/* sprites use colors 256-511 + 512-767 */
	{ REGION_GFX1, 0x6000, &charlayout, 0, 16 },
	{ -1 } /* end of array */
};



static MACHINE_DRIVER_START( suprloco )

	/* basic machine hardware */
	MDRV_CPU_ADD(Z80, 4000000)	/* 4 MHz (?) */
	MDRV_CPU_PROGRAM_MAP(readmem,writemem)
	MDRV_CPU_VBLANK_INT(irq0_line_hold,1)

	MDRV_CPU_ADD(Z80, 4000000)
	/* audio CPU */
	MDRV_CPU_PROGRAM_MAP(sound_readmem,sound_writemem)
	MDRV_CPU_VBLANK_INT(irq0_line_hold,4)			/* NMIs are caused by the main CPU */

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(5000)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(32*8, 32*8)
	MDRV_VISIBLE_AREA(1*8, 31*8-1, 0*8, 28*8-1)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(512+256)

	MDRV_PALETTE_INIT(suprloco)
	MDRV_VIDEO_START(suprloco)
	MDRV_VIDEO_UPDATE(suprloco)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")

	MDRV_SOUND_ADD(SN76496, 4000000)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.0)

	MDRV_SOUND_ADD(SN76496, 2000000)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.0)
MACHINE_DRIVER_END


/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START( suprloco )
	ROM_REGION( 2*0x10000, REGION_CPU1, 0 )	/* 64k for code + 64k for decrypted opcodes */
	ROM_LOAD( "ic37.bin",     0x0000, 0x4000, CRC(57f514dd) SHA1(707800b90a22547a56b01d1e11775e9ee5555d23) )	/* encrypted */
	ROM_LOAD( "ic15.bin",     0x4000, 0x4000, CRC(5a1d2fb0) SHA1(fdb9416e5530718245fd597073a63feddb233c3c) )	/* encrypted */
	ROM_LOAD( "ic28.bin",     0x8000, 0x4000, CRC(a597828a) SHA1(61004d112591fd2d752c39df71c1304d9308daae) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* 64k for sound cpu */
	ROM_LOAD( "ic64.bin",     0x0000, 0x2000, CRC(0aa57207) SHA1(b29b533505cb5b47c90534f2f610baeb7265d030) )

	ROM_REGION( 0xe000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "ic63.bin",     0x0000, 0x2000, CRC(e571fe81) SHA1(ac2b5914a445b89b7456b2c4290e4630b525f05d) )
	ROM_LOAD( "ic62.bin",     0x2000, 0x2000, CRC(6130f93c) SHA1(ae0657f46c10e75eec994e75359a89b5d61baf68) )
	ROM_LOAD( "ic61.bin",     0x4000, 0x2000, CRC(3b03004e) SHA1(805b51cb14d3ace97f2e0f306db28921b2f5e322) )
							/*0x6000- 0xe000 will be created by init_suprloco */

	ROM_REGION( 0x8000, REGION_GFX2, 0 )	/* 32k for sprites data used at runtime */
	ROM_LOAD( "ic55.bin",     0x0000, 0x4000, CRC(ee2d3ed3) SHA1(593f3cd5c4e7f20b5e31e6bac8864774442e4b75) )
	ROM_LOAD( "ic56.bin",     0x4000, 0x2000, CRC(f04a4b50) SHA1(80363f0c508fb2a755bf684f9a6862c1e7285495) )
							/*0x6000 empty */

	ROM_REGION( 0x0620, REGION_PROMS, 0 )
	ROM_LOAD( "ic100.bin",    0x0100, 0x0080, CRC(7b0c8ce5) SHA1(4e1ea5ce38198a3965dfeb609ba0c7e8211531c3) )  /* color PROM */
	ROM_CONTINUE(             0x0000, 0x0080 )
	ROM_CONTINUE(             0x0180, 0x0080 )
	ROM_CONTINUE(             0x0080, 0x0080 )
	ROM_LOAD( "ic89.bin",     0x0200, 0x0400, CRC(1d4b02cb) SHA1(00d822f1bc4f57f2f5d5a0615241f8136246a842) )  /* 3bpp to 4bpp table */
	ROM_LOAD( "ic7.bin",      0x0600, 0x0020, CRC(89ba674f) SHA1(17c87840c8011968675a5a6f55966467df02364b) )	/* unknown */
ROM_END



DRIVER_INIT( suprloco )
{
	/* convert graphics to 4bpp from 3bpp */

	int i, j, k, color_source, color_dest;
	unsigned char *source, *dest, *lookup;

	source = memory_region(REGION_GFX1);
	dest   = source + 0x6000;
	lookup = memory_region(REGION_PROMS) + 0x0200;

	for (i = 0; i < 0x80; i++, lookup += 8)
	{
		for (j = 0; j < 0x40; j++, source++, dest++)
		{
			dest[0] = dest[0x2000] = dest[0x4000] = dest[0x6000] = 0;

			for (k = 0; k < 8; k++)
			{
				color_source = (((source[0x0000] >> k) & 0x01) << 2) |
							   (((source[0x2000] >> k) & 0x01) << 1) |
							   (((source[0x4000] >> k) & 0x01) << 0);

				color_dest = lookup[color_source];

				dest[0x0000] |= (((color_dest >> 3) & 0x01) << k);
				dest[0x2000] |= (((color_dest >> 2) & 0x01) << k);
				dest[0x4000] |= (((color_dest >> 1) & 0x01) << k);
				dest[0x6000] |= (((color_dest >> 0) & 0x01) << k);
			}
		}
	}


	/* decrypt program ROMs */
	suprloco_decode();
}



GAME( 1982, suprloco, 0, suprloco, suprloco, suprloco, ROT0, "Sega", "Super Locomotive", 0 )
