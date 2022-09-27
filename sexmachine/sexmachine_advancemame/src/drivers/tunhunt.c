/***************************************************************************

    Atari Tunnel Hunt hardware

    Games supported:
        * Tunnel Hunt

    Known issues:
        * see below

****************************************************************************

    MAME driver for Tunnel Hunt (C)1981
        (aka Tube Chase)
        Developed by Atari
        Hardware by Dave Sherman
        Game Programmed by Owen Rubin
        Licensed and Distributed by Centuri

    Many thanks to Owen Rubin for invaluable hardware information and
    game description!

    Known Issues:

    Coin Input seems unresponsive.

    Colors:
    - Hues are hardcoded.  There doesn't appear to be any logical way to
        map the color proms so that the correct colors appear.
        See last page of schematics for details.  Are color proms bad?
        (shouldn't be, both sets were the same)

    Alphanumeric Layer:
    - placement for some characters seems strange (but may well be correct).

    Shell Objects:
    - vstretch/placement/color handling isn't confirmed
    - two bitplanes per character or two banks?

    Motion Object:
    - enemy ships look funny when they get close (to ram player)
    - stretch probably isn't implemented correctly (see splash screen
        with zooming "TUNNEL HUNT" logo.
    - colors may not be mapped correctly.

    Square Generator:
    - needs optimization

***************************************************************************/

#include "driver.h"
#include "sound/pokey.h"


UINT8 *tunhunt_ram;

extern WRITE8_HANDLER( tunhunt_videoram_w );
extern WRITE8_HANDLER( tunhunt_mott_w );

extern PALETTE_INIT( tunhunt );
extern VIDEO_START( tunhunt );
extern VIDEO_UPDATE( tunhunt );


/*************************************
 *
 *  Output ports
 *
 *************************************/

UINT8 tunhunt_control;

WRITE8_HANDLER( tunhunt_control_w )
{
	/*
        0x01    coin counter#2  "right counter"
        0x02    coin counter#1  "center counter"
        0x04    "left counter"
        0x08    cover screen (shell0 hstretch)
        0x10    cover screen (shell1 hstretch)
        0x40    start LED
        0x80    in-game
    */
	tunhunt_control = data;
	coin_counter_w( 0,data&0x01 );
	coin_counter_w( 1,data&0x02 );
	set_led_status( 0, data&0x40 ); /* start */
}



/*************************************
 *
 *  Input ports
 *
 *************************************/

static READ8_HANDLER( tunhunt_button_r )
{
	int data = readinputportbytag("IN0");
	return ((data>>offset)&1)?0x00:0x80;
}


static READ8_HANDLER( dsw1_r )
{
	return readinputportbytag("DSW")&0xff;
}


static READ8_HANDLER( dsw2_0r )
{
	return (readinputportbytag("DSW")&0x0100)?0x80:0x00;
}


static READ8_HANDLER( dsw2_1r )
{
	return (readinputportbytag("DSW")&0x0200)?0x80:0x00;
}


static READ8_HANDLER( dsw2_2r )
{
	return (readinputportbytag("DSW")&0x0400)?0x80:0x00;
}


static READ8_HANDLER( dsw2_3r )
{
	return (readinputportbytag("DSW")&0x0800)?0x80:0x00;
}


static READ8_HANDLER( dsw2_4r )
{
	return (readinputportbytag("DSW")&0x1000)?0x80:0x00;
}



/*************************************
 *
 *  Main CPU memory handlers
 *
 *************************************/

static ADDRESS_MAP_START( readmem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x03ff) AM_READ(MRA8_RAM) /* Work RAM */
	AM_RANGE(0x2000, 0x2007) AM_READ(tunhunt_button_r)
	AM_RANGE(0x3000, 0x300f) AM_READ(pokey1_r)
	AM_RANGE(0x4000, 0x400f) AM_READ(pokey2_r)
	AM_RANGE(0x5000, 0x7fff) AM_READ(MRA8_ROM)
	AM_RANGE(0xfffa, 0xffff) AM_READ(MRA8_ROM)
ADDRESS_MAP_END


static ADDRESS_MAP_START( writemem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x03ff) AM_WRITE(MWA8_RAM) AM_BASE(&tunhunt_ram) /* Work RAM */
	AM_RANGE(0x1080, 0x10ff) AM_WRITE(MWA8_RAM)
	AM_RANGE(0x1200, 0x12ff) AM_WRITE(MWA8_RAM)
	AM_RANGE(0x1400, 0x14ff) AM_WRITE(MWA8_RAM)
	AM_RANGE(0x1600, 0x160f) AM_WRITE(MWA8_RAM) AM_BASE(&paletteram) /* COLRAM (D7-D4 SHADE; D3-D0 COLOR) */
	AM_RANGE(0x1800, 0x1800) AM_WRITE(MWA8_RAM) /* SHEL0H */
	AM_RANGE(0x1a00, 0x1a00) AM_WRITE(MWA8_RAM) /* SHEL1H */
	AM_RANGE(0x1c00, 0x1c00) AM_WRITE(MWA8_RAM) /* MOBJV */
	AM_RANGE(0x1e00, 0x1eff) AM_WRITE(tunhunt_videoram_w) AM_BASE(&videoram)	/* ALPHA */
	AM_RANGE(0x2c00, 0x2fff) AM_WRITE(tunhunt_mott_w) AM_BASE(&spriteram)
	AM_RANGE(0x2000, 0x2000) AM_WRITE(MWA8_NOP) /* watchdog */
	AM_RANGE(0x2400, 0x2400) AM_WRITE(MWA8_NOP) /* INT ACK */
	AM_RANGE(0x2800, 0x2800) AM_WRITE(tunhunt_control_w)
	AM_RANGE(0x3000, 0x300f) AM_WRITE(pokey1_w)
	AM_RANGE(0x4000, 0x400f) AM_WRITE(pokey2_w)
	AM_RANGE(0x5000, 0xffff) AM_WRITE(MWA8_ROM)
ADDRESS_MAP_END



/*************************************
 *
 *  Port definitions
 *
 *************************************/

INPUT_PORTS_START( tunhunt )
	PORT_START_TAG("IN0")
	PORT_BIT ( 0x01, IP_ACTIVE_HIGH, IPT_TILT )
	PORT_BIT ( 0x02, IP_ACTIVE_HIGH, IPT_BUTTON1 )
	PORT_DIPNAME( 0x04, 0x00, DEF_STR( Service_Mode ) )
	PORT_DIPSETTING (	0x00, DEF_STR( Off ) )
	PORT_DIPSETTING (	0x04, DEF_STR( On ) )
	//Should the above switch be described as:
	//PORT_SERVICE(0x04, IP_ACTIVE_HIGH)    ?
	PORT_BIT ( 0x08, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BIT ( 0x10, IP_ACTIVE_HIGH, IPT_COIN2 )
	PORT_BIT ( 0x20, IP_ACTIVE_HIGH, IPT_BUTTON2 )
	PORT_BIT ( 0x40, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT ( 0x80, IP_ACTIVE_LOW, IPT_VBLANK )

	PORT_START_TAG("IN1")
	PORT_BIT( 0xff, 0x00, IPT_AD_STICK_Y ) PORT_MINMAX(0x00,0xff) PORT_SENSITIVITY(100) PORT_KEYDELTA(4)

	PORT_START_TAG("IN2")
	PORT_BIT( 0xff, 0x00, IPT_AD_STICK_X ) PORT_MINMAX(0x00,0xff) PORT_SENSITIVITY(100) PORT_KEYDELTA(4) PORT_REVERSE

	PORT_START_TAG("DSW")
	PORT_DIPNAME (0x0003, 0x0002, DEF_STR( Coinage ) )
	PORT_DIPSETTING (     0x0003, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING (     0x0002, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING (     0x0001, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING (     0x0000, DEF_STR( Free_Play ) )
	PORT_DIPNAME (0x000c, 0x0000, DEF_STR( Coin_B ) )
	PORT_DIPSETTING (     0x0000, "*1" )
	PORT_DIPSETTING (     0x0004, "*4" )
	PORT_DIPSETTING (     0x0008, "*5" )
	PORT_DIPSETTING (     0x000c, "*6" )
	PORT_DIPNAME (0x0010, 0x0000, DEF_STR( Coin_A ) )
	PORT_DIPSETTING (     0x0000, "*1" )
	PORT_DIPSETTING (     0x0010, "*2" )
	PORT_DIPNAME (0x0060, 0x0000, "Bonus Credits" )
	PORT_DIPSETTING (     0x0000, DEF_STR( None ) )
	PORT_DIPSETTING (     0x0060, "5 credits, 1 bonus" )
	PORT_DIPSETTING (     0x0040, "4 credits, 1 bonus" )
	PORT_DIPSETTING (     0x0020, "2 credits, 1 bonus" )
	PORT_DIPNAME (0x0880, 0x0000, DEF_STR( Language ) )
	PORT_DIPSETTING (     0x0000, DEF_STR( English ) )
	PORT_DIPSETTING (     0x0080, DEF_STR( German ) )
	PORT_DIPSETTING (     0x0800, DEF_STR( French ) )
	PORT_DIPSETTING (     0x0880, DEF_STR( Spanish ) )
	PORT_DIPNAME (0x0100, 0x0000, DEF_STR( Unknown ) )
	PORT_DIPSETTING (     0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING (     0x0100, DEF_STR( On ) )
	PORT_DIPNAME (0x0600, 0x0200, DEF_STR( Lives ) )
	PORT_DIPSETTING (     0x0000, "2" )
	PORT_DIPSETTING (     0x0200, "3" )
	PORT_DIPSETTING (     0x0400, "4" )
	PORT_DIPSETTING (     0x0600, "5" )
	PORT_DIPNAME (0x1000, 0x1000, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING (     0x1000, "30000" )
	PORT_DIPSETTING (     0x0000, DEF_STR( None ) )
INPUT_PORTS_END



/*************************************
 *
 *  Graphics definitions
 *
 *************************************/

static const gfx_layout alpha_layout =
{
	8,8,
	0x40,
	1,
	{ 4 },
	{ 0,1,2,3,8,9,10,11 },
	{ 0x00,0x10,0x20,0x30,0x40,0x50,0x60,0x70 },
	0x80
};


static const gfx_layout obj_layout =
{
	16,16,
	8, /* number of objects */
	1, /* number of bitplanes */
	{ 4 }, /* plane offsets */
	{
		0x00+0,0x00+1,0x00+2,0x00+3,
		0x08+0,0x08+1,0x08+2,0x08+3,
		0x10+0,0x10+1,0x10+2,0x10+3,
		0x18+0,0x18+1,0x18+2,0x18+3
	 }, /* x offsets */
	{
		0x0*0x20, 0x1*0x20, 0x2*0x20, 0x3*0x20,
		0x4*0x20, 0x5*0x20, 0x6*0x20, 0x7*0x20,
		0x8*0x20, 0x9*0x20, 0xa*0x20, 0xb*0x20,
		0xc*0x20, 0xd*0x20, 0xe*0x20, 0xf*0x20
	}, /* y offsets */
	0x200
};


static const gfx_decode gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0x000, &alpha_layout, 0, 4 },
	{ REGION_GFX2, 0x200, &obj_layout,	 8, 1 },
	{ REGION_GFX2, 0x000, &obj_layout,	 8, 1 }, /* second bank, or second bitplane? */
	{ -1 }
};



/*************************************
 *
 *  Sound interfaces
 *
 *************************************/

static struct POKEYinterface pokey_interface_1 =
{
	{ 0 },
	dsw1_r
};

static struct POKEYinterface pokey_interface_2 =
{
	{ input_port_1_r,input_port_2_r,dsw2_0r,dsw2_1r,dsw2_2r,dsw2_3r,dsw2_4r }
};



/*************************************
 *
 *  Machine driver
 *
 *************************************/

static MACHINE_DRIVER_START( tunhunt )

	/* basic machine hardware */
	MDRV_CPU_ADD(M6502,2000000)		/* ??? */
	MDRV_CPU_PROGRAM_MAP(readmem,writemem)
	MDRV_CPU_VBLANK_INT(irq0_line_hold,2)	/* ? probably wrong */

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_REAL_60HZ_VBLANK_DURATION)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(256, 256-16)
	MDRV_VISIBLE_AREA(0, 255, 0, 255-16)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(16)
	MDRV_COLORTABLE_LENGTH(16)

	MDRV_PALETTE_INIT(tunhunt)
	MDRV_VIDEO_START(tunhunt)
	MDRV_VIDEO_UPDATE(tunhunt)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")

	MDRV_SOUND_ADD(POKEY, 1209600)
	MDRV_SOUND_CONFIG(pokey_interface_1)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.50)

	MDRV_SOUND_ADD(POKEY, 1209600)
	MDRV_SOUND_CONFIG(pokey_interface_2)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.50)
MACHINE_DRIVER_END



/*************************************
 *
 *  ROM definitions
 *
 *************************************/

/*

ATARI TUBE CHASE

136000-101      5000    6/16
136000-102      5800    6/16
136000-103      6000    6/16
136000-104      6800    6/16
136000-105      7000    6/16
136000-106      7800    6/16

136000-015      SYNC
136000-017      SW 1            B8
136000-016      SW 0            A8
136000-018      PRIORITY        H9
136000-019      A/N             C10
136000-013      RED             C11
136000-014      B/G             D11

*/

ROM_START( tunhunt )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )
	ROM_LOAD( "001.lm1",	0x5000, 0x800, CRC(2601a3a4) SHA1(939bafc54576fdaccf688b49cc9d201b03feec3a) )
	ROM_LOAD( "002.k1",		0x5800, 0x800, CRC(29bbf3df) SHA1(4a0ec4cfab362a976d3962b347f687db45095cfd) )
	ROM_LOAD( "136000.103",	0x6000, 0x800, CRC(1a6a60a4) SHA1(7c60cc92595f1b90f421eabbaa20f657181ed4f0) )
	ROM_LOAD( "004.fh1",	0x6800, 0x800, CRC(4d6c920e) SHA1(2ef274356f4b8a0170a267cd6a3758b2bda693b5) )
	ROM_LOAD( "005.ef1",	0x7000, 0x800, CRC(e17badf0) SHA1(6afbf517486340fe54b01fa26258877b2a8fc510) )
	ROM_LOAD( "006.d1",		0x7800, 0x800, CRC(c3ae8519) SHA1(2b2e49065bc38429894ef29a29ffc60f96e64840) )
	ROM_RELOAD( 		  	0xf800, 0x800 ) /* 6502 vectors  */

	ROM_REGION( 0x400, REGION_GFX1, ROMREGION_DISPOSE ) /* alphanumeric characters */
	ROM_LOAD( "019.c10",	0x000, 0x400, CRC(d6fd45a9) SHA1(c86ea3790c29c554199af8ad6f3d563dcb7723c7) )

	ROM_REGION( 0x400, REGION_GFX2, 0 ) /* "SHELL" objects (16x16 pixel sprites) */
	ROM_LOAD( "016.a8",		0x000, 0x200, CRC(830e6c34) SHA1(37a5eeb722dd80c4224c7f622b0edabb3ac1ca19) )
	ROM_LOAD( "017.b8",		0x200, 0x200, CRC(5bef8b5a) SHA1(bfd9c592a34ed4861a6ad76ef10ea0d9b76a92b2) )

	ROM_REGION( 0x540, REGION_PROMS, 0 )
	ROM_LOAD( "013.d11",	0x000, 0x020, CRC(66f1f5eb) SHA1(bcf5348ae328cf943d2bf6e38df727c0c4c466b7) )	/* hue: BBBBGGGG? */
	ROM_LOAD( "014.c11",	0x020, 0x020, CRC(662444b2) SHA1(2e510c1d9b7e34a3045048a46045e61fabaf918e) )	/* hue: RRRR----? */
	ROM_LOAD( "015.n4",		0x040, 0x100, CRC(00e224a0) SHA1(1a384ef488791c62566c91b18d6a1fb4a5def2ba) )	/* timing? */
	ROM_LOAD( "018.h9",		0x140, 0x400, CRC(6547c208) SHA1(f19c334f9b4a1cfcbc913c0920688db2730dded0) )	/* color lookup table? */
ROM_END

ROM_START( tunhuntc )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )
	ROM_LOAD( "001.lm1",	0x5000, 0x800, CRC(2601a3a4) SHA1(939bafc54576fdaccf688b49cc9d201b03feec3a) )
	ROM_LOAD( "002.k1",		0x5800, 0x800, CRC(29bbf3df) SHA1(4a0ec4cfab362a976d3962b347f687db45095cfd) )
	ROM_LOAD( "003.j1",		0x6000, 0x800, CRC(360c0f47) SHA1(8e3d815836504c7651812e0e26423b0c7045621c) ) /* bad crc? fails self-test */
	/* 0xcaa6bb2a: alternate prom (re)dumped by Al also fails, they simply modified the rom without fixing the checksum routine? */
	ROM_LOAD( "004.fh1",	0x6800, 0x800, CRC(4d6c920e) SHA1(2ef274356f4b8a0170a267cd6a3758b2bda693b5) )
	ROM_LOAD( "005.ef1",	0x7000, 0x800, CRC(e17badf0) SHA1(6afbf517486340fe54b01fa26258877b2a8fc510) )
	ROM_LOAD( "006.d1",		0x7800, 0x800, CRC(c3ae8519) SHA1(2b2e49065bc38429894ef29a29ffc60f96e64840) )
	ROM_RELOAD( 		  	0xf800, 0x800 ) /* 6502 vectors  */

	ROM_REGION( 0x400, REGION_GFX1, ROMREGION_DISPOSE ) /* alphanumeric characters */
	ROM_LOAD( "019.c10",	0x000, 0x400, CRC(d6fd45a9) SHA1(c86ea3790c29c554199af8ad6f3d563dcb7723c7) )

	ROM_REGION( 0x400, REGION_GFX2, 0 ) /* "SHELL" objects (16x16 pixel sprites) */
	ROM_LOAD( "016.a8",		0x000, 0x200, CRC(830e6c34) SHA1(37a5eeb722dd80c4224c7f622b0edabb3ac1ca19) )
	ROM_LOAD( "017.b8",		0x200, 0x200, CRC(5bef8b5a) SHA1(bfd9c592a34ed4861a6ad76ef10ea0d9b76a92b2) )

	ROM_REGION( 0x540, REGION_PROMS, 0 )
	ROM_LOAD( "013.d11",	0x000, 0x020, CRC(66f1f5eb) SHA1(bcf5348ae328cf943d2bf6e38df727c0c4c466b7) )	/* hue: BBBBGGGG? */
	ROM_LOAD( "014.c11",	0x020, 0x020, CRC(662444b2) SHA1(2e510c1d9b7e34a3045048a46045e61fabaf918e) )	/* hue: RRRR----? */
	ROM_LOAD( "015.n4",		0x040, 0x100, CRC(00e224a0) SHA1(1a384ef488791c62566c91b18d6a1fb4a5def2ba) )	/* timing? */
	ROM_LOAD( "018.h9",		0x140, 0x400, CRC(6547c208) SHA1(f19c334f9b4a1cfcbc913c0920688db2730dded0) )	/* color lookup table? */
ROM_END



/*************************************
 *
 *  Game drivers
 *
 *************************************/

/*         rom   parent  machine    inp         init */
GAME( 1979,tunhunt,  0,       tunhunt,   tunhunt,	0,  ORIENTATION_SWAP_XY, "Atari", "Tunnel Hunt", 0 )
GAME( 1981,tunhuntc, tunhunt, tunhunt,   tunhunt,	0,  ORIENTATION_SWAP_XY, "Atari (Centuri license)", "Tunnel Hunt (Centuri)", 0 )
