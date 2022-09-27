/***************************************************************************

    Atari Basketball hardware

    driver by Mike Balfour

    Games supported:
        * Basketball

    Known issues:
        * none at this time

****************************************************************************

    Note:  The original hardware uses the Player 1 and Player 2 Start buttons
    as the Jump/Shoot buttons.  I've taken button 1 and mapped it to the Start
    buttons to keep people from getting confused.

    If you have any questions about how this driver works, don't hesitate to
    ask.  - Mike Balfour (mab22@po.cwru.edu)

***************************************************************************/

#include "driver.h"
#include "bsktball.h"
#include "sound/discrete.h"


/*************************************
 *
 *  Palette generation
 *
 *************************************/

static unsigned short colortable_source[] =
{
	/* Playfield */
	0x01, 0x00, 0x00, 0x00,
	0x01, 0x03, 0x03, 0x03,

	/* Motion */
	0x01, 0x00, 0x00, 0x00,
	0x01, 0x00, 0x01, 0x00,
	0x01, 0x00, 0x02, 0x00,
	0x01, 0x00, 0x03, 0x00,

	0x01, 0x01, 0x00, 0x00,
	0x01, 0x01, 0x01, 0x00,
	0x01, 0x01, 0x02, 0x00,
	0x01, 0x01, 0x03, 0x00,

	0x01, 0x02, 0x00, 0x00,
	0x01, 0x02, 0x01, 0x00,
	0x01, 0x02, 0x02, 0x00,
	0x01, 0x02, 0x03, 0x00,

	0x01, 0x03, 0x00, 0x00,
	0x01, 0x03, 0x01, 0x00,
	0x01, 0x03, 0x02, 0x00,
	0x01, 0x03, 0x03, 0x00,

	0x01, 0x00, 0x00, 0x01,
	0x01, 0x00, 0x01, 0x01,
	0x01, 0x00, 0x02, 0x01,
	0x01, 0x00, 0x03, 0x01,

	0x01, 0x01, 0x00, 0x01,
	0x01, 0x01, 0x01, 0x01,
	0x01, 0x01, 0x02, 0x01,
	0x01, 0x01, 0x03, 0x01,

	0x01, 0x02, 0x00, 0x01,
	0x01, 0x02, 0x01, 0x01,
	0x01, 0x02, 0x02, 0x01,
	0x01, 0x02, 0x03, 0x01,

	0x01, 0x03, 0x00, 0x01,
	0x01, 0x03, 0x01, 0x01,
	0x01, 0x03, 0x02, 0x01,
	0x01, 0x03, 0x03, 0x01,

	0x01, 0x00, 0x00, 0x02,
	0x01, 0x00, 0x01, 0x02,
	0x01, 0x00, 0x02, 0x02,
	0x01, 0x00, 0x03, 0x02,

	0x01, 0x01, 0x00, 0x02,
	0x01, 0x01, 0x01, 0x02,
	0x01, 0x01, 0x02, 0x02,
	0x01, 0x01, 0x03, 0x02,

	0x01, 0x02, 0x00, 0x02,
	0x01, 0x02, 0x01, 0x02,
	0x01, 0x02, 0x02, 0x02,
	0x01, 0x02, 0x03, 0x02,

	0x01, 0x03, 0x00, 0x02,
	0x01, 0x03, 0x01, 0x02,
	0x01, 0x03, 0x02, 0x02,
	0x01, 0x03, 0x03, 0x02,

	0x01, 0x00, 0x00, 0x03,
	0x01, 0x00, 0x01, 0x03,
	0x01, 0x00, 0x02, 0x03,
	0x01, 0x00, 0x03, 0x03,

	0x01, 0x01, 0x00, 0x03,
	0x01, 0x01, 0x01, 0x03,
	0x01, 0x01, 0x02, 0x03,
	0x01, 0x01, 0x03, 0x03,

	0x01, 0x02, 0x00, 0x03,
	0x01, 0x02, 0x01, 0x03,
	0x01, 0x02, 0x02, 0x03,
	0x01, 0x02, 0x03, 0x03,

	0x01, 0x03, 0x00, 0x03,
	0x01, 0x03, 0x01, 0x03,
	0x01, 0x03, 0x02, 0x03,
	0x01, 0x03, 0x03, 0x03,
};

static PALETTE_INIT( bsktball )
{
	palette_set_color(0,0x00,0x00,0x00); /* BLACK */
	palette_set_color(1,0x80,0x80,0x80); /* LIGHT GREY */
	palette_set_color(2,0x50,0x50,0x50); /* DARK GREY */
	palette_set_color(3,0xff,0xff,0xff); /* WHITE */
	memcpy(colortable,colortable_source,sizeof(colortable_source));
}



/*************************************
 *
 *  Main CPU memory handlers
 *
 *************************************/

static ADDRESS_MAP_START( main_map, ADDRESS_SPACE_PROGRAM, 8 )
	ADDRESS_MAP_FLAGS( AMEF_ABITS(14) )
	AM_RANGE(0x0000, 0x01ff) AM_RAM /* Zero Page RAM */
	AM_RANGE(0x0800, 0x0800) AM_READ(bsktball_in0_r)
	AM_RANGE(0x0802, 0x0802) AM_READ(input_port_5_r)
	AM_RANGE(0x0803, 0x0803) AM_READ(input_port_6_r)
	AM_RANGE(0x1000, 0x1000) AM_WRITE(MWA8_NOP) /* Timer Reset */
	AM_RANGE(0x1010, 0x1010) AM_WRITE(bsktball_bounce_w) /* Crowd Amp / Bounce */
	AM_RANGE(0x1022, 0x1023) AM_WRITE(MWA8_NOP) /* Coin Counter */
	AM_RANGE(0x1024, 0x1025) AM_WRITE(bsktball_led1_w) /* LED 1 */
	AM_RANGE(0x1026, 0x1027) AM_WRITE(bsktball_led2_w) /* LED 2 */
	AM_RANGE(0x1028, 0x1029) AM_WRITE(bsktball_ld1_w) /* LD 1 */
	AM_RANGE(0x102a, 0x102b) AM_WRITE(bsktball_ld2_w) /* LD 2 */
	AM_RANGE(0x102c, 0x102d) AM_WRITE(bsktball_noise_reset_w) /* Noise Reset */
	AM_RANGE(0x102e, 0x102f) AM_WRITE(bsktball_nmion_w) /* NMI On */
	AM_RANGE(0x1030, 0x1030) AM_WRITE(bsktball_note_w) /* Music Ckt Note Dvsr */
	AM_RANGE(0x1800, 0x1bbf) AM_READWRITE(MRA8_RAM, bsktball_videoram_w) AM_BASE(&videoram) /* DISPLAY */
	AM_RANGE(0x1bc0, 0x1bff) AM_RAM AM_BASE(&bsktball_motion)
	AM_RANGE(0x1c00, 0x1cff) AM_RAM
	AM_RANGE(0x2000, 0x3fff) AM_ROM /* PROGRAM */
ADDRESS_MAP_END



/*************************************
 *
 *  Port definitions
 *
 *************************************/

INPUT_PORTS_START( bsktball )
	PORT_START	/* IN0 */
	PORT_BIT( 0xFF, 0x00, IPT_TRACKBALL_X ) PORT_SENSITIVITY(100) PORT_KEYDELTA(10) /* Sensitivity, clip, min, max */

	PORT_START	/* IN0 */
	PORT_BIT( 0xFF, 0x00, IPT_TRACKBALL_Y ) PORT_SENSITIVITY(100) PORT_KEYDELTA(10)

	PORT_START	/* IN0 */
	PORT_BIT( 0xFF, 0x00, IPT_TRACKBALL_X ) PORT_SENSITIVITY(100) PORT_KEYDELTA(10) PORT_PLAYER(2) /* Sensitivity, clip, min, max */

	PORT_START	/* IN0 */
	PORT_BIT( 0xFF, 0x00, IPT_TRACKBALL_Y ) PORT_SENSITIVITY(100) PORT_KEYDELTA(10) PORT_PLAYER(2)

	PORT_START		/* IN0 */
	PORT_BIT ( 0x01, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT ( 0x02, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT ( 0x04, IP_ACTIVE_LOW, IPT_BUTTON1 ) /* SPARE */
	PORT_BIT ( 0x08, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(2) /* SPARE */
	/* 0x10 - DR0 = PL2 H DIR */
	/* 0x20 - DR1 = PL2 V DIR */
	/* 0x40 - DR2 = PL1 H DIR */
	/* 0x80 - DR3 = PL1 V DIR */

	PORT_START		/* IN2 */
	PORT_BIT ( 0x01, IP_ACTIVE_HIGH, IPT_VBLANK )
	PORT_BIT ( 0x02, IP_ACTIVE_LOW, IPT_TILT )
	PORT_BIT ( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN ) /* SPARE */
	PORT_BIT ( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN ) /* TEST STEP */
	PORT_SERVICE( 0x10, IP_ACTIVE_LOW )
	PORT_BIT ( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN ) /* COIN 0 */
	PORT_BIT ( 0x40, IP_ACTIVE_LOW, IPT_COIN2 ) /* COIN 1 */
	PORT_BIT ( 0x80, IP_ACTIVE_LOW, IPT_COIN1 ) /* COIN 2 */

	PORT_START		/* DSW */
	PORT_DIPNAME( 0x07, 0x00, "Play Time per Credit" )
	PORT_DIPSETTING(	0x07, DEF_STR( Free_Play ) )
	PORT_DIPSETTING(	0x06, "2:30" )
	PORT_DIPSETTING(	0x05, "2:00" )
	PORT_DIPSETTING(	0x04, "1:30" )
	PORT_DIPSETTING(	0x03, "1:15" )
	PORT_DIPSETTING(	0x02, "0:45" )
	PORT_DIPSETTING(	0x01, "0:30" )
	PORT_DIPSETTING(	0x00, "1:00" )
	PORT_DIPNAME( 0x18, 0x00, DEF_STR( Coinage ) )
	PORT_DIPSETTING(	0x00, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(	0x10, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(	0x08, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(	0x18, DEF_STR( 1C_6C ) )
	PORT_DIPNAME( 0x20, 0x00, "Cost" )
	PORT_DIPSETTING(	0x20, "Two Coin Minimum" )
	PORT_DIPSETTING(	0x00, "One Coin Minimum" )
	PORT_DIPNAME( 0xC0, 0x00, DEF_STR( Language ) )
	PORT_DIPSETTING(	0xC0, DEF_STR( German ) )
	PORT_DIPSETTING(	0x80, DEF_STR( French ) )
	PORT_DIPSETTING(	0x40, DEF_STR( Spanish ) )
	PORT_DIPSETTING(	0x00, DEF_STR( English ) )
INPUT_PORTS_END



/*************************************
 *
 *  Graphics layouts
 *
 *************************************/

static const gfx_layout charlayout =
{
	8,8,
	64,
	2,
	{ 0, 8*0x800 },
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8
};


static const gfx_layout motionlayout =
{
	8,32,
	64,
	2,
	{ 0, 8*0x800 },
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{	0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8,
		8*8, 9*8, 10*8, 11*8, 12*8, 13*8, 14*8, 15*8,
		16*8, 17*8, 18*8, 19*8, 20*8, 21*8, 22*8, 23*8,
		24*8, 25*8, 26*8, 27*8, 28*8, 29*8, 30*8, 31*8 },
	32*8
};


static const gfx_decode gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0x0600, &charlayout,   0x00, 0x02 },
	{ REGION_GFX1, 0x0000, &motionlayout, 0x08, 0x40 },
	{ -1 }
};


/*************************************
 *
 *  Machine driver
 *
 *************************************/

static MACHINE_DRIVER_START( bsktball )

	/* basic machine hardware */
	MDRV_CPU_ADD(M6502,750000)
	MDRV_CPU_PROGRAM_MAP(main_map,0)
	MDRV_CPU_VBLANK_INT(bsktball_interrupt,8)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_REAL_60HZ_VBLANK_DURATION)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(32*8, 28*8)
	MDRV_VISIBLE_AREA(0*8, 32*8-1, 0*8, 28*8-1)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(4)
	MDRV_COLORTABLE_LENGTH(sizeof(colortable_source) / sizeof(colortable_source[0]))

	MDRV_PALETTE_INIT(bsktball)
	MDRV_VIDEO_START(bsktball)
	MDRV_VIDEO_UPDATE(bsktball)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")

	MDRV_SOUND_ADD(DISCRETE, 0)
	MDRV_SOUND_CONFIG(bsktball_discrete_interface)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.0)
MACHINE_DRIVER_END



/*************************************
 *
 *  ROM definitions
 *
 *************************************/

ROM_START( bsktball )
	ROM_REGION( 0x4000, REGION_CPU1, 0 ) /* 16k for code */
	ROM_LOAD( "034765.d1",    0x2000, 0x0800, CRC(798cea39) SHA1(b1b709a74258b01b21d7c2038a3b6abe879944c5) )
	ROM_LOAD( "034764.c1",    0x2800, 0x0800, CRC(a087109e) SHA1(f5d6dcccc4a54db35be3d8997bc51e73892747fb) )
	ROM_LOAD( "034766.f1",    0x3000, 0x0800, CRC(a82e9a9f) SHA1(9aca236c5145c04a8aaebb316179482bbdc9ddfc) )
	ROM_LOAD( "034763.b1",    0x3800, 0x0800, CRC(1fc69359) SHA1(a215ba3bb18ea2c57c443dfc4c4a0a3846bbedfe) )

	ROM_REGION( 0x1000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "034757.a6",    0x0000, 0x0800, CRC(010e8ad3) SHA1(43ce2c2089ec3011e2d28e8257a35efeed0e71c5) )
	ROM_LOAD( "034758.b6",    0x0800, 0x0800, CRC(f7bea344) SHA1(df544bff67bb0334f77cef11792199d9c3f5fdf4) )
ROM_END



/*************************************
 *
 *  Game drivers
 *
 *************************************/

GAME( 1979, bsktball, 0, bsktball, bsktball, 0, ROT0, "Atari", "Basketball", 0 )
