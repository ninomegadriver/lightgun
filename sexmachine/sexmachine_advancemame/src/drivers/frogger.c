/***************************************************************************

 Frogger hardware

***************************************************************************/

#include "driver.h"
#include "galaxian.h"
#include "sound/ay8910.h"



static ADDRESS_MAP_START( readmem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x3fff) AM_READ(MRA8_ROM)
	AM_RANGE(0x8000, 0x87ff) AM_READ(MRA8_RAM)
	AM_RANGE(0x8800, 0x8800) AM_READ(watchdog_reset_r)
	AM_RANGE(0xa800, 0xabff) AM_READ(MRA8_RAM)
	AM_RANGE(0xb000, 0xb0ff) AM_READ(MRA8_RAM)
	AM_RANGE(0xd000, 0xd007) AM_READ(frogger_ppi8255_1_r)
	AM_RANGE(0xe000, 0xe007) AM_READ(frogger_ppi8255_0_r)
ADDRESS_MAP_END

static ADDRESS_MAP_START( writemem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x3fff) AM_WRITE(MWA8_ROM)
	AM_RANGE(0x8000, 0x87ff) AM_WRITE(MWA8_RAM)
	AM_RANGE(0xa800, 0xabff) AM_WRITE(galaxian_videoram_w) AM_BASE(&galaxian_videoram)
	AM_RANGE(0xb000, 0xb03f) AM_WRITE(galaxian_attributesram_w) AM_BASE(&galaxian_attributesram)
	AM_RANGE(0xb040, 0xb05f) AM_WRITE(MWA8_RAM) AM_BASE(&galaxian_spriteram) AM_SIZE(&galaxian_spriteram_size)
	AM_RANGE(0xb060, 0xb0ff) AM_WRITE(MWA8_RAM)
	AM_RANGE(0xb808, 0xb808) AM_WRITE(galaxian_nmi_enable_w)
	AM_RANGE(0xb80c, 0xb80c) AM_WRITE(galaxian_flip_screen_y_w)
	AM_RANGE(0xb810, 0xb810) AM_WRITE(galaxian_flip_screen_x_w)
	AM_RANGE(0xb818, 0xb818) AM_WRITE(galaxian_coin_counter_0_w)
	AM_RANGE(0xb81c, 0xb81c) AM_WRITE(galaxian_coin_counter_1_w)
	AM_RANGE(0xd000, 0xd007) AM_WRITE(frogger_ppi8255_1_w)
	AM_RANGE(0xe000, 0xe007) AM_WRITE(frogger_ppi8255_0_w)
ADDRESS_MAP_END


ADDRESS_MAP_START( frogger_sound_readmem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x1fff) AM_READ(MRA8_ROM)
	AM_RANGE(0x4000, 0x43ff) AM_READ(MRA8_RAM)
ADDRESS_MAP_END

ADDRESS_MAP_START( frogger_sound_writemem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x1fff) AM_WRITE(MWA8_ROM)
	AM_RANGE(0x4000, 0x43ff) AM_WRITE(MWA8_RAM)
    AM_RANGE(0x6000, 0x6fff) AM_WRITE(frogger_filter_w)
ADDRESS_MAP_END


ADDRESS_MAP_START( frogger_sound_readport, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_FLAGS( AMEF_ABITS(8) )
	AM_RANGE(0x40, 0x40) AM_READ(AY8910_read_port_0_r)
ADDRESS_MAP_END

ADDRESS_MAP_START( frogger_sound_writeport, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_FLAGS( AMEF_ABITS(8) )
	AM_RANGE(0x40, 0x40) AM_WRITE(AY8910_write_port_0_w)
	AM_RANGE(0x80, 0x80) AM_WRITE(AY8910_control_port_0_w)
ADDRESS_MAP_END



INPUT_PORTS_START( frogger )
	PORT_START_TAG("IN0")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_4WAY PORT_COCKTAIL
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN ) /* 1P shoot2 - unused */
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN ) /* 1P shoot1 - unused */
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_4WAY
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_4WAY
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN1 )

	PORT_START_TAG("IN1")
	PORT_DIPNAME( 0x03, 0x00, DEF_STR( Lives ) )
	PORT_DIPSETTING(	0x00, "3" )
	PORT_DIPSETTING(	0x01, "5" )
	PORT_DIPSETTING(	0x02, "7" )
	PORT_DIPSETTING(	0x03, "256 (Cheat)")
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN ) /* 2P shoot2 - unused */
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN ) /* 2P shoot1 - unused */
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_4WAY PORT_COCKTAIL
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_4WAY PORT_COCKTAIL
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START1 )

	PORT_START_TAG("IN2")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_4WAY PORT_COCKTAIL
	PORT_DIPNAME( 0x06, 0x00, DEF_STR( Coinage ) )
	PORT_DIPSETTING(	0x02, "A 2/1 B 2/1 C 2/1" )
	PORT_DIPSETTING(	0x04, "A 2/1 B 1/3 C 2/1" )
	PORT_DIPSETTING(	0x00, "A 1/1 B 1/1 C 1/1" )
	PORT_DIPSETTING(	0x06, "A 1/1 B 1/6 C 1/1" )
	PORT_DIPNAME( 0x08, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(	0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(	0x08, DEF_STR( Cocktail ) )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_4WAY
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_4WAY
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNUSED )
INPUT_PORTS_END


struct AY8910interface frogger_ay8910_interface =
{
	soundlatch_r,
	frogger_portB_r
};


static MACHINE_DRIVER_START( frogger )

	/* basic machine hardware */
	MDRV_IMPORT_FROM(galaxian_base)
	MDRV_CPU_MODIFY("main")
	MDRV_CPU_PROGRAM_MAP(readmem,writemem)

	MDRV_CPU_ADD(Z80,14318000/8)
	/* audio CPU */ /* 1.78975 MHz */
	MDRV_CPU_PROGRAM_MAP(frogger_sound_readmem,frogger_sound_writemem)
	MDRV_CPU_IO_MAP(frogger_sound_readport,frogger_sound_writeport)

	MDRV_MACHINE_RESET(scramble)

	/* video hardware */
	MDRV_PALETTE_LENGTH(32+64+2+1)	/* 32 for characters, 64 for stars, 2 for bullets, 1 for background */	\

	MDRV_PALETTE_INIT(frogger)
	MDRV_VIDEO_START(frogger)

	/* sound hardware */
	MDRV_SOUND_ADD(AY8910, 14318000/8)
	MDRV_SOUND_CONFIG(frogger_ay8910_interface)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.33)
MACHINE_DRIVER_END


/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START( frogger )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "frogger.26",   0x0000, 0x1000, CRC(597696d6) SHA1(e7e021776cad00f095a1ebbef407b7c0a8f5d835) )
	ROM_LOAD( "frogger.27",   0x1000, 0x1000, CRC(b6e6fcc3) SHA1(5e8692f2b0c7f4b3642b3ee6670e1c3b20029cdc) )
	ROM_LOAD( "frsm3.7",      0x2000, 0x1000, CRC(aca22ae0) SHA1(5a99060ea2506a3ac7d61ca5876ce5cb3e493565) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* 64k for the audio CPU */
	ROM_LOAD( "frogger.608",  0x0000, 0x0800, CRC(e8ab0256) SHA1(f090afcfacf5f13cdfa0dfda8e3feb868c6ce8bc) )
	ROM_LOAD( "frogger.609",  0x0800, 0x0800, CRC(7380a48f) SHA1(75582a94b696062cbdb66a4c5cf0bc0bb94f81ee) )
	ROM_LOAD( "frogger.610",  0x1000, 0x0800, CRC(31d7eb27) SHA1(2e1d34ae4da385fd7cac94707d25eeddf4604e1a) )

	ROM_REGION( 0x1000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "frogger.607",  0x0000, 0x0800, CRC(05f7d883) SHA1(78831fd287da18928651a8adb7e578d291493eff) )
	ROM_LOAD( "frogger.606",  0x0800, 0x0800, CRC(f524ee30) SHA1(dd768967add61467baa08d5929001f157d6cd911) )

	ROM_REGION( 0x0020, REGION_PROMS, 0 )
	ROM_LOAD( "pr-91.6l",     0x0000, 0x0020, CRC(413703bf) SHA1(66648b2b28d3dcbda5bdb2605d1977428939dd3c) )
ROM_END

ROM_START( frogseg1 )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "frogger.26",   0x0000, 0x1000, CRC(597696d6) SHA1(e7e021776cad00f095a1ebbef407b7c0a8f5d835) )
	ROM_LOAD( "frogger.27",   0x1000, 0x1000, CRC(b6e6fcc3) SHA1(5e8692f2b0c7f4b3642b3ee6670e1c3b20029cdc) )
	ROM_LOAD( "frogger.34",   0x2000, 0x1000, CRC(ed866bab) SHA1(24e1bbde44eb5480b7a0570fa0dc1de388cb95ba) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* 64k for the audio CPU */
	ROM_LOAD( "frogger.608",  0x0000, 0x0800, CRC(e8ab0256) SHA1(f090afcfacf5f13cdfa0dfda8e3feb868c6ce8bc) )
	ROM_LOAD( "frogger.609",  0x0800, 0x0800, CRC(7380a48f) SHA1(75582a94b696062cbdb66a4c5cf0bc0bb94f81ee) )
	ROM_LOAD( "frogger.610",  0x1000, 0x0800, CRC(31d7eb27) SHA1(2e1d34ae4da385fd7cac94707d25eeddf4604e1a) )

	ROM_REGION( 0x1000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "frogger.607",  0x0000, 0x0800, CRC(05f7d883) SHA1(78831fd287da18928651a8adb7e578d291493eff) )
	ROM_LOAD( "frogger.606",  0x0800, 0x0800, CRC(f524ee30) SHA1(dd768967add61467baa08d5929001f157d6cd911) )

	ROM_REGION( 0x0020, REGION_PROMS, 0 )
	ROM_LOAD( "pr-91.6l",     0x0000, 0x0020, CRC(413703bf) SHA1(66648b2b28d3dcbda5bdb2605d1977428939dd3c) )
ROM_END

ROM_START( frogseg2 )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "frogger.ic5",  0x0000, 0x1000, CRC(efab0c79) SHA1(68c99b6cdcb9396bb473739a62ffc009b4bf57d5) )
	ROM_LOAD( "frogger.ic6",  0x1000, 0x1000, CRC(aeca9c13) SHA1(cdf560adbd7f2813e86e378da7781cccf7928a44) )
	ROM_LOAD( "frogger.ic7",  0x2000, 0x1000, CRC(dd251066) SHA1(4612e1fe1ab7182a277140b1a1976cc17e0746a5) )
	ROM_LOAD( "frogger.ic8",  0x3000, 0x1000, CRC(bf293a02) SHA1(be94e9f5caa74c3de6fd95bd20928f4a9c514227) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* 64k for the audio CPU */
	ROM_LOAD( "frogger.608",  0x0000, 0x0800, CRC(e8ab0256) SHA1(f090afcfacf5f13cdfa0dfda8e3feb868c6ce8bc) )
	ROM_LOAD( "frogger.609",  0x0800, 0x0800, CRC(7380a48f) SHA1(75582a94b696062cbdb66a4c5cf0bc0bb94f81ee) )
	ROM_LOAD( "frogger.610",  0x1000, 0x0800, CRC(31d7eb27) SHA1(2e1d34ae4da385fd7cac94707d25eeddf4604e1a) )

	ROM_REGION( 0x1000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "frogger.607",  0x0000, 0x0800, CRC(05f7d883) SHA1(78831fd287da18928651a8adb7e578d291493eff) )
	ROM_LOAD( "frogger.606",  0x0800, 0x0800, CRC(f524ee30) SHA1(dd768967add61467baa08d5929001f157d6cd911) )

	ROM_REGION( 0x0020, REGION_PROMS, 0 )
	ROM_LOAD( "pr-91.6l",     0x0000, 0x0020, CRC(413703bf) SHA1(66648b2b28d3dcbda5bdb2605d1977428939dd3c) )
ROM_END



GAME( 1981, frogger,  0,	   frogger,  frogger,  frogger,  ROT90, "Konami", "Frogger", GAME_SUPPORTS_SAVE )
GAME( 1981, frogseg1, frogger, frogger,  frogger,  frogger,  ROT90, "[Konami] (Sega license)", "Frogger (Sega set 1)", GAME_SUPPORTS_SAVE )
GAME( 1981, frogseg2, frogger, frogger,  frogger,  frogger,  ROT90, "[Konami] (Sega license)", "Frogger (Sega set 2)", GAME_SUPPORTS_SAVE )
