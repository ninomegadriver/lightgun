/***************************************************************************

Naughty Boy driver by Sal and John Bugliarisi.
This driver is based largely on MAME's Phoenix driver, since Naughty Boy runs
on similar hardware as Phoenix. Phoenix driver provided by Brad Oliver.
Thanks to Richard Davies for his Phoenix emulator source.


Naughty Boy memory map

0000-3fff 16Kb Program ROM
4000-7fff 1Kb Work RAM (mirrored)
8000-87ff 2Kb Video RAM Charset A (lower priority, mirrored)
8800-8fff 2Kb Video RAM Charset b (higher priority, mirrored)
9000-97ff 2Kb Video Control write-only (mirrored)
9800-9fff 2Kb Video Scroll Register (mirrored)
a000-a7ff 2Kb Sound Control A (mirrored)
a800-afff 2Kb Sound Control B (mirrored)
b000-b7ff 2Kb 8bit Game Control read-only (mirrored)
b800-bfff 1Kb 8bit Dip Switch read-only (mirrored)
c000-0000 16Kb Unused

memory mapped ports:

read-only:
b000-b7ff IN
b800-bfff DSW


Naughty Boy Switch Settings
(C)1982 Cinematronics

 --------------------------------------------------------
|Option |Factory|Descrpt| 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 |
 ------------------------|-------------------------------
|Lives  |       |2      |on |on |   |   |   |   |   |   |
 ------------------------ -------------------------------
|       |   X   |3      |off|on |   |   |   |   |   |   |
 ------------------------ -------------------------------
|       |       |4      |on |off|   |   |   |   |   |   |
 ------------------------ -------------------------------
|       |       |5      |off|off|   |   |   |   |   |   |
 ------------------------ -------------------------------
|Extra  |       |10000  |   |   |on |on |   |   |   |   |
 ------------------------ -------------------------------
|       |   X   |30000  |   |   |off|on |   |   |   |   |
 ------------------------ -------------------------------
|       |       |50000  |   |   |on |off|   |   |   |   |
 ------------------------ -------------------------------
|       |       |70000  |   |   |off|off|   |   |   |   |
 ------------------------ -------------------------------
|Credits|       |2c, 1p |   |   |   |   |on |on |   |   |
 ------------------------ -------------------------------
|       |   X   |1c, 1p |   |   |   |   |off|on |   |   |
 ------------------------ -------------------------------
|       |       |1c, 2p |   |   |   |   |on |off|   |   |
 ------------------------ -------------------------------
|       |       |4c, 3p |   |   |   |   |off|off|   |   |
 ------------------------ -------------------------------
|Dffclty|   X   |Easier |   |   |   |   |   |   |on |   |
 ------------------------ -------------------------------
|       |       |Harder |   |   |   |   |   |   |off|   |
 ------------------------ -------------------------------
| Type  |       |Upright|   |   |   |   |   |   |   |on |
 ------------------------ -------------------------------
|       |       |Cktail |   |   |   |   |   |   |   |off|
 ------------------------ -------------------------------

*
* Pop Flamer
*

Pop Flamer appears to run on identical hardware as Naughty Boy.
The dipswitches are even identical. Spooky.

                        1   2   3   4   5   6   7   8
-------------------------------------------------------
Number of Mr. Mouse 2 |ON |ON |   |   |   |   |   |   |
                    3 |OFF|ON |   |   |   |   |   |   |
                    4 |ON |OFF|   |   |   |   |   |   |
                    5 |OFF|OFF|   |   |   |   |   |   |
-------------------------------------------------------
Extra Mouse    10,000 |   |   |ON |ON |   |   |   |   |
               30,000 |   |   |OFF|ON |   |   |   |   |
               50,000 |   |   |ON |OFF|   |   |   |   |
               70,000 |   |   |OFF|OFF|   |   |   |   |
-------------------------------------------------------
Credit  2 coin 1 play |   |   |   |   |ON |ON |   |   |
        1 coin 1 play |   |   |   |   |OFF|ON |   |   |
        1 coin 2 play |   |   |   |   |ON |OFF|   |   |
        1 coin 3 play |   |   |   |   |OFF|OFF|   |   |
-------------------------------------------------------
Skill          Easier |   |   |   |   |   |   |ON |   |
               Harder |   |   |   |   |   |   |OFF|   |
-------------------------------------------------------
Game style      Table |   |   |   |   |   |   |   |OFF|
              Upright |   |   |   |   |   |   |   |ON |


TODO:
    * sounds are a little skanky

 ***************************************************************************/

#include "driver.h"
#include "sound/custom.h"
#include "sound/tms36xx.h"
#include "includes/phoenix.h"

static READ8_HANDLER( in0_port_r )
{
	int in0 = input_port_0_r(0);

	if ( naughtyb_cocktail )
	{
		// cabinet == cocktail -AND- handling player 2

		in0 = ( in0 & 0x03 ) |				// start buttons
			  ( input_port_1_r(0) & 0xFC );	// cocktail inputs
	}

	return in0;
}

static READ8_HANDLER( dsw0_port_r )
{
	// vblank replaces the cabinet dip

	return ( ( input_port_2_r(0) & 0x7F ) |		// dsw0
   			 ( input_port_3_r(0) & 0x80 ) );	// vblank
}

/* Pop Flamer
   1st protection relies on reading values from a device at $9000 and writing to 400A-400D (See $26A9).
   Then value stored in 400C must be xxxx1001 (rrca x 3) or else reset
   2nd protection relies on the values stored in 400A-400D matching $2690+($400E) (Starts at $460)
   If the values all match then it will jump to 0x0011 instead of 0x0009 (refresh instead of reset)
   Paul Priest: tourniquet@mameworld.net */

//static int popflame_prot_count = 0;

READ8_HANDLER( popflame_protection_r ) /* Not used by bootleg/hack */
{
	static int values[4] = { 0x78, 0x68, 0x48, 0x38|0x80 };
	static int count;

	count = (count + 1) % 4;
	return values[count];

#if 0
	if ( activecpu_get_pc() == (0x26F2 + 0x03) )
	{
		popflame_prot_count = 0;
		return 0x01;
	} /* Must not carry when rotated left */

	if ( activecpu_get_pc() == (0x26F9 + 0x03) )
		return 0x80; /* Must carry when rotated left */

	if ( activecpu_get_pc() == (0x270F + 0x03) )
	{
		switch( popflame_prot_count++ )
		{
			case 0: return 0x78; /* x111 1xxx, matches 0x0F at $2690, stored in $400A */
			case 1: return 0x68; /* x110 1xxx, matches 0x0D at $2691, stored in $400B */
			case 2: return 0x48; /* x100 1xxx, matches 0x09 at $2692, stored in $400C */
			case 3: return 0x38; /* x011 1xxx, matches 0x07 at $2693, stored in $400D */
		}
	}
	logerror("CPU #0 PC %06x: unmapped protection read\n", activecpu_get_pc());
	return 0x00;
#endif
}



static ADDRESS_MAP_START( readmem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x3fff) AM_READ(MRA8_ROM)
	AM_RANGE(0x4000, 0x8fff) AM_READ(MRA8_RAM)
	AM_RANGE(0xb000, 0xb7ff) AM_READ(in0_port_r)	// IN0
	AM_RANGE(0xb800, 0xbfff) AM_READ(dsw0_port_r)	// DSW0
ADDRESS_MAP_END

static ADDRESS_MAP_START( writemem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x3fff) AM_WRITE(MWA8_ROM)
	AM_RANGE(0x4000, 0x7fff) AM_WRITE(MWA8_RAM)
	AM_RANGE(0x8000, 0x87ff) AM_WRITE(videoram_w) AM_BASE(&videoram) AM_SIZE(&videoram_size)
	AM_RANGE(0x8800, 0x8fff) AM_WRITE(naughtyb_videoram2_w) AM_BASE(&naughtyb_videoram2)
	AM_RANGE(0x9000, 0x97ff) AM_WRITE(naughtyb_videoreg_w)
	AM_RANGE(0x9800, 0x9fff) AM_WRITE(MWA8_RAM) AM_BASE(&naughtyb_scrollreg)
	AM_RANGE(0xa000, 0xa7ff) AM_WRITE(pleiads_sound_control_a_w)
	AM_RANGE(0xa800, 0xafff) AM_WRITE(pleiads_sound_control_b_w)
ADDRESS_MAP_END

static ADDRESS_MAP_START( popflame_writemem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x3fff) AM_WRITE(MWA8_ROM)
	AM_RANGE(0x4000, 0x7fff) AM_WRITE(MWA8_RAM)
	AM_RANGE(0x8000, 0x87ff) AM_WRITE(videoram_w) AM_BASE(&videoram) AM_SIZE(&videoram_size)
	AM_RANGE(0x8800, 0x8fff) AM_WRITE(naughtyb_videoram2_w) AM_BASE(&naughtyb_videoram2)
	AM_RANGE(0x9000, 0x97ff) AM_WRITE(popflame_videoreg_w)
	AM_RANGE(0x9800, 0x9fff) AM_WRITE(MWA8_RAM) AM_BASE(&naughtyb_scrollreg)
	AM_RANGE(0xa000, 0xa7ff) AM_WRITE(pleiads_sound_control_a_w)
	AM_RANGE(0xa800, 0xafff) AM_WRITE(pleiads_sound_control_b_w)
ADDRESS_MAP_END



/***************************************************************************

  Naughty Boy doesn't have VBlank interrupts.
  Interrupts are still used by the game: but they are related to coin
  slots.

***************************************************************************/

INTERRUPT_GEN( naughtyb_interrupt )
{
	if (readinputport(3) & 1)
		cpunum_set_input_line(0, INPUT_LINE_NMI, PULSE_LINE);
}

INPUT_PORTS_START( naughtyb )
	PORT_START_TAG(	"IN0" )
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_4WAY
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_4WAY
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_4WAY
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_4WAY

	PORT_START_TAG(	"IN0_COCKTAIL" )
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN ) // IPT_START1
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN ) // IPT_START2
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_COCKTAIL
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_4WAY PORT_COCKTAIL
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_4WAY PORT_COCKTAIL
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_4WAY PORT_COCKTAIL
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_4WAY PORT_COCKTAIL

	PORT_START_TAG( "DSW0" )
	PORT_DIPNAME( 0x03, 0x01, DEF_STR( Lives ) )
	PORT_DIPSETTING(	0x00, "2" )
	PORT_DIPSETTING(	0x01, "3" )
	PORT_DIPSETTING(	0x02, "4" )
	PORT_DIPSETTING(	0x03, "5" )
	PORT_DIPNAME( 0x0c, 0x04, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(	0x00, "10000" )
	PORT_DIPSETTING(	0x04, "30000" )
	PORT_DIPSETTING(	0x08, "50000" )
	PORT_DIPSETTING(	0x0c, "70000" )
	PORT_DIPNAME( 0x30, 0x10, DEF_STR( Coinage ) )
	PORT_DIPSETTING(	0x00, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(	0x10, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(	0x20, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(	0x30, DEF_STR( 1C_3C ) )
	PORT_DIPNAME( 0x40, 0x00, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(	0x00, DEF_STR( Easy ) )
	PORT_DIPSETTING(	0x40, DEF_STR( Hard ) )
	PORT_DIPNAME( 0x80, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Cocktail ) )

	PORT_START_TAG( "FAKE" )
	// The coin slots are not memory mapped.
	// This fake input port is used by the interrupt
	// handler to be notified of coin insertions. We use IMPULSE to
	// trigger exactly one interrupt, without having to check when the
	// user releases the key.
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_COIN1 ) PORT_IMPULSE(1)
	// when reading DSW0, bit 7 doesn't read cabinet, but vblank
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_VBLANK )
INPUT_PORTS_END

INPUT_PORTS_START( trvmstr )
	PORT_START_TAG(	"IN0" )
	PORT_SERVICE(0x0f, IP_ACTIVE_LOW )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON4 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON3 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON1 )

	PORT_START_TAG(	"IN0_COCKTAIL" )
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON4 ) PORT_COCKTAIL
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_COCKTAIL
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_COCKTAIL
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_COCKTAIL

	PORT_START_TAG( "DSW0" )
	PORT_DIPNAME( 0x03, 0x00, "Screen Orientation" )
	PORT_DIPSETTING(	0x00, "0'" )
	PORT_DIPSETTING(	0x02, "90'" )
	PORT_DIPSETTING(	0x01, "180'" )
	PORT_DIPSETTING(	0x03, "270'" )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unknown ) )
	PORT_DIPSETTING(	0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x00, "Show Correct Answer" )
	PORT_DIPSETTING(	0x08, DEF_STR( No ) )
	PORT_DIPSETTING(	0x00, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x30, 0x00, DEF_STR( Coinage ) )
	PORT_DIPSETTING(	0x10, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(	0x00, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(	0x20, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(	0x30, DEF_STR( Free_Play ) )
	PORT_DIPNAME( 0x40, 0x40, "Number of Questions" )
	PORT_DIPSETTING(	0x00, "5" )
	PORT_DIPSETTING(	0x40, "7" )
	PORT_DIPNAME( 0x80, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Cocktail ) )

	PORT_START_TAG( "FAKE" )
	// The coin slots are not memory mapped.
	// This fake input port is used by the interrupt
	// handler to be notified of coin insertions. We use IMPULSE to
	// trigger exactly one interrupt, without having to check when the
	// user releases the key.
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_COIN1 ) PORT_IMPULSE(1)
	// when reading DSW0, bit 7 doesn't read cabinet, but vblank
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_VBLANK )
INPUT_PORTS_END



static const gfx_layout charlayout =
{
	8,8,	/* 8*8 characters */
	512,	/* 512 characters */
	2,		/* 2 bits per pixel */
	{ 512*8*8, 0 }, /* the two bitplanes are separated */
	{ 7, 6, 5, 4, 3, 2, 1, 0 }, /* pretty straightforward layout */
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8 	/* every char takes 8 consecutive bytes */
};



static const gfx_decode gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &charlayout,	  0, 32 },
	{ REGION_GFX2, 0, &charlayout, 32*4, 32 },
	{ -1 } /* end of array */
};



static struct CustomSound_interface naughtyb_custom_interface =
{
	naughtyb_sh_start
};

static struct CustomSound_interface popflame_custom_interface =
{
	popflame_sh_start
};

static struct TMS36XXinterface tms3615_interface =
{
	TMS3615,	/* TMS36xx subtype */
	/*
     * Decay times of the voices; NOTE: it's unknown if
     * the the TMS3615 mixes more than one voice internally.
     * A wav taken from Pop Flamer sounds like there
     * are at least no 'odd' harmonics (5 1/3' and 2 2/3')
     */
	{0.15,0.20,0,0,0,0}

};



static MACHINE_DRIVER_START( naughtyb )

	/* basic machine hardware */
	MDRV_CPU_ADD(Z80, 1500000)	/* 3 MHz ? */
	MDRV_CPU_PROGRAM_MAP(readmem,writemem)
	MDRV_CPU_VBLANK_INT(naughtyb_interrupt,1)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_REAL_60HZ_VBLANK_DURATION)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(36*8, 28*8)
	MDRV_VISIBLE_AREA(0*8, 36*8-1, 0*8, 28*8-1)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(256)
	MDRV_COLORTABLE_LENGTH(32*4+32*4)

	MDRV_PALETTE_INIT(naughtyb)
	MDRV_VIDEO_START(naughtyb)
	MDRV_VIDEO_UPDATE(naughtyb)

	/* sound hardware */
	/* uses the TMS3615NS for sound */
	MDRV_SPEAKER_STANDARD_MONO("mono")

	MDRV_SOUND_ADD(TMS36XX, 350)
	MDRV_SOUND_CONFIG(tms3615_interface)
	MDRV_SOUND_ROUTE(0, "mono", 0.60)

	MDRV_SOUND_ADD(CUSTOM, 0)
	MDRV_SOUND_CONFIG(naughtyb_custom_interface)
	MDRV_SOUND_ROUTE(0, "mono", 0.40)
MACHINE_DRIVER_END


/* Exactly the same but for the writemem handler */
static MACHINE_DRIVER_START( popflame )

	/* basic machine hardware */
	MDRV_CPU_ADD(Z80, 1500000)	/* 3 MHz ? */
	MDRV_CPU_PROGRAM_MAP(readmem,popflame_writemem)
	MDRV_CPU_VBLANK_INT(naughtyb_interrupt,1)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_REAL_60HZ_VBLANK_DURATION)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(36*8, 28*8)
	MDRV_VISIBLE_AREA(0*8, 36*8-1, 0*8, 28*8-1)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(256)
	MDRV_COLORTABLE_LENGTH(32*4+32*4)

	MDRV_PALETTE_INIT(naughtyb)
	MDRV_VIDEO_START(naughtyb)
	MDRV_VIDEO_UPDATE(naughtyb)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")

	MDRV_SOUND_ADD(TMS36XX, 350)
	MDRV_SOUND_CONFIG(tms3615_interface)
	MDRV_SOUND_ROUTE(0, "mono", 0.60)

	MDRV_SOUND_ADD(CUSTOM, 0)
	MDRV_SOUND_CONFIG(popflame_custom_interface)
	MDRV_SOUND_ROUTE(0, "mono", 1.0)
MACHINE_DRIVER_END



/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START( naughtyb )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )		/* 64k for code */
	ROM_LOAD( "1.30",	   0x0000, 0x0800, CRC(f6e1178e) SHA1(5cd428e1f085ff82d7237b3e261b33ff876fd4cb) )
	ROM_LOAD( "2.29",	   0x0800, 0x0800, CRC(b803eb8c) SHA1(c21b781eb329195e36e6fd1d7467bd9b0d9cbc5b) )
	ROM_LOAD( "3.28",	   0x1000, 0x0800, CRC(004d0ba7) SHA1(5c182fa6f65f7caa3459fcc5cdc3b7faa8b34769) )
	ROM_LOAD( "4.27",	   0x1800, 0x0800, CRC(3c7bcac6) SHA1(ef291cd5b2f8a64999dc015e16d3ea479fefaf8f) )
	ROM_LOAD( "5.26",	   0x2000, 0x0800, CRC(ea80f39b) SHA1(f05cc4ca48245053a8b35b594fb4c0c3b19304e0) )
	ROM_LOAD( "6.25",	   0x2800, 0x0800, CRC(66d9f942) SHA1(756b188836e9e9d86f8be59c9505288339b91899) )
	ROM_LOAD( "7.24",	   0x3000, 0x0800, CRC(00caf9be) SHA1(0599b28dfe8dd9c18564202af56ba8f272d7ac54) )
	ROM_LOAD( "8.23",	   0x3800, 0x0800, CRC(17c3b6fb) SHA1(c01c8ae27f5b9be90778f7c459c5ba0dddf443ba) )

	ROM_REGION( 0x2000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "15.44",	   0x0000, 0x0800, CRC(d692f9c7) SHA1(3573c518868690b140340d19f88c670026a6696d) )
	ROM_LOAD( "16.43",	   0x0800, 0x0800, CRC(d3ba8b27) SHA1(0ff14b8b983ab75870fb19b64327070ccd0888d6) )
	ROM_LOAD( "13.46",	   0x1000, 0x0800, CRC(c1669cd5) SHA1(9b4370ed54424e3615fa2e4d07cadae37ab8cd10) )
	ROM_LOAD( "14.45",	   0x1800, 0x0800, CRC(eef2c8e5) SHA1(5077c4052342958ee26c25047704c62eed44eb89) )

	ROM_REGION( 0x2000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "11.48",	   0x0000, 0x0800, CRC(75ec9710) SHA1(b41606930eff79ccf5bfcad01362251d7bab114a) )
	ROM_LOAD( "12.47",	   0x0800, 0x0800, CRC(ef0706c3) SHA1(0e0b82d29d710d1244384db84344bfba2e867b2e) )
	ROM_LOAD( "9.50",	   0x1000, 0x0800, CRC(8c8db764) SHA1(2641a1b8bc30896293ebd9396e304ce5eb7eb705) )
	ROM_LOAD( "10.49",	   0x1800, 0x0800, CRC(c97c97b9) SHA1(5da7fb378e85b6c9d5ab6e75544f1e64fae9997a) )

	ROM_REGION( 0x0200, REGION_PROMS, 0 )
	ROM_LOAD( "6301-1.63", 0x0000, 0x0100, CRC(98ad89a1) SHA1(ddee7dcb003b66fbc7d6d6e90d499ed090c59227) ) /* palette low bits */
	ROM_LOAD( "6301-1.64", 0x0100, 0x0100, CRC(909107d4) SHA1(138ace7845424bc3ca86b0889be634943c8c2d19) ) /* palette high bits */
ROM_END

ROM_START( naughtya )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )		/* 64k for code */
	ROM_LOAD( "91", 	   0x0000, 0x0800, CRC(42b14bc7) SHA1(a5890834105b83f6761a5ea819e94533473f0e44) )
	ROM_LOAD( "92", 	   0x0800, 0x0800, CRC(a24674b4) SHA1(2d93981c2f0dea190745cbc3926b012cfd561ec3) )
	ROM_LOAD( "3.28",	   0x1000, 0x0800, CRC(004d0ba7) SHA1(5c182fa6f65f7caa3459fcc5cdc3b7faa8b34769) )
	ROM_LOAD( "4.27",	   0x1800, 0x0800, CRC(3c7bcac6) SHA1(ef291cd5b2f8a64999dc015e16d3ea479fefaf8f) )
	ROM_LOAD( "95", 	   0x2000, 0x0800, CRC(e282f1b8) SHA1(9eb7b2fed75cd23f3c90e445021f23648503c96f) )
	ROM_LOAD( "96", 	   0x2800, 0x0800, CRC(61178ff2) SHA1(2a7fb894e7fc5ec170d00d24300f1e23307f9687) )
	ROM_LOAD( "97", 	   0x3000, 0x0800, CRC(3cafde88) SHA1(c77f03e81128341522d46056aad77e73c2818069) )
	ROM_LOAD( "8.23",	   0x3800, 0x0800, CRC(17c3b6fb) SHA1(c01c8ae27f5b9be90778f7c459c5ba0dddf443ba) )

	ROM_REGION( 0x2000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "15.44",	   0x0000, 0x0800, CRC(d692f9c7) SHA1(3573c518868690b140340d19f88c670026a6696d) )
	ROM_LOAD( "16.43",	   0x0800, 0x0800, CRC(d3ba8b27) SHA1(0ff14b8b983ab75870fb19b64327070ccd0888d6) )
	ROM_LOAD( "13.46",	   0x1000, 0x0800, CRC(c1669cd5) SHA1(9b4370ed54424e3615fa2e4d07cadae37ab8cd10) )
	ROM_LOAD( "14.45",	   0x1800, 0x0800, CRC(eef2c8e5) SHA1(5077c4052342958ee26c25047704c62eed44eb89) )

	ROM_REGION( 0x2000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "11.48",	   0x0000, 0x0800, CRC(75ec9710) SHA1(b41606930eff79ccf5bfcad01362251d7bab114a) )
	ROM_LOAD( "12.47",	   0x0800, 0x0800, CRC(ef0706c3) SHA1(0e0b82d29d710d1244384db84344bfba2e867b2e) )
	ROM_LOAD( "9.50",	   0x1000, 0x0800, CRC(8c8db764) SHA1(2641a1b8bc30896293ebd9396e304ce5eb7eb705) )
	ROM_LOAD( "10.49",	   0x1800, 0x0800, CRC(c97c97b9) SHA1(5da7fb378e85b6c9d5ab6e75544f1e64fae9997a) )

	ROM_REGION( 0x0200, REGION_PROMS, 0 )
	ROM_LOAD( "6301-1.63", 0x0000, 0x0100, CRC(98ad89a1) SHA1(ddee7dcb003b66fbc7d6d6e90d499ed090c59227) ) /* palette low bits */
	ROM_LOAD( "6301-1.64", 0x0100, 0x0100, CRC(909107d4) SHA1(138ace7845424bc3ca86b0889be634943c8c2d19) ) /* palette high bits */
ROM_END

ROM_START( naughtyc )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )		/* 64k for code */
	ROM_LOAD( "nb1ic30",   0x0000, 0x0800, CRC(3f482fa3) SHA1(5c670ad37be5bed12a65b8b02330525cfe5ae303) )
	ROM_LOAD( "nb2ic29",   0x0800, 0x0800, CRC(7ddea141) SHA1(8a725614b156f1fdb249c2767ddb3f04681e7a3f) )
	ROM_LOAD( "nb3ic28",   0x1000, 0x0800, CRC(8c72a069) SHA1(648df992a5b118d0c48aa20e8621172f50ee5b4c) )
	ROM_LOAD( "nb4ic27",   0x1800, 0x0800, CRC(30feae51) SHA1(fa28942a58c2292147e33747feecad9817c2c8ea) )
	ROM_LOAD( "nb5ic26",   0x2000, 0x0800, CRC(05242fd0) SHA1(3436a18c021643959bd5d475eeb0b8ac6afaec74) )
	ROM_LOAD( "nb6ic25",   0x2800, 0x0800, CRC(7a12ffea) SHA1(4a34d6fcd0b6dc9319424d4122d88744ddc473c7) )
	ROM_LOAD( "nb7ic24",   0x3000, 0x0800, CRC(9cc287df) SHA1(507c551ca8044479e588bd2a3fff600c77ea2255) )
	ROM_LOAD( "nb8ic23",   0x3800, 0x0800, CRC(4d84ff2c) SHA1(66e51116bae787c67c10f282700a94069d7b9fe0) )

	ROM_REGION( 0x2000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "15.44",	   0x0000, 0x0800, CRC(d692f9c7) SHA1(3573c518868690b140340d19f88c670026a6696d) )
	ROM_LOAD( "16.43",	   0x0800, 0x0800, CRC(d3ba8b27) SHA1(0ff14b8b983ab75870fb19b64327070ccd0888d6) )
	ROM_LOAD( "13.46",	   0x1000, 0x0800, CRC(c1669cd5) SHA1(9b4370ed54424e3615fa2e4d07cadae37ab8cd10) )
	ROM_LOAD( "14.45",	   0x1800, 0x0800, CRC(eef2c8e5) SHA1(5077c4052342958ee26c25047704c62eed44eb89) )

	ROM_REGION( 0x2000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "nb11ic48",  0x0000, 0x0800, CRC(23271a13) SHA1(ba46fe9af0f6b6ab366469b9058d95477620e05c) )
	ROM_LOAD( "12.47",	   0x0800, 0x0800, CRC(ef0706c3) SHA1(0e0b82d29d710d1244384db84344bfba2e867b2e) )
	ROM_LOAD( "nb9ic50",   0x1000, 0x0800, CRC(d6949c27) SHA1(2076e76ef9f8f4c9beb3dfe863608151ffae3f3c) )
	ROM_LOAD( "10.49",	   0x1800, 0x0800, CRC(c97c97b9) SHA1(5da7fb378e85b6c9d5ab6e75544f1e64fae9997a) )

	ROM_REGION( 0x0200, REGION_PROMS, 0 )
	ROM_LOAD( "6301-1.63", 0x0000, 0x0100, CRC(98ad89a1) SHA1(ddee7dcb003b66fbc7d6d6e90d499ed090c59227) ) /* palette low bits */
	ROM_LOAD( "6301-1.64", 0x0100, 0x0100, CRC(909107d4) SHA1(138ace7845424bc3ca86b0889be634943c8c2d19) ) /* palette high bits */
ROM_END

ROM_START( popflame )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )		/* 64k for code */
	ROM_LOAD( "ic86.bin",	  0x0000, 0x1000, CRC(06397a4b) SHA1(12ef8aa60033161479ba2239b61a318cbf15b3cf) )
	ROM_LOAD( "ic80.pop",	  0x1000, 0x1000, CRC(b77abf3d) SHA1(8626af8fe7d10c52bea7570dd6237de60607bab6) )
	ROM_LOAD( "ic94.bin",	  0x2000, 0x1000, CRC(ae5248ae) SHA1(39a7feb94d0392a0eeeb506d2f52299151521692) )
	ROM_LOAD( "ic100.pop",	  0x3000, 0x1000, CRC(f9f2343b) SHA1(c019a5d838152417ec76be021d659f884928ef87) )

	ROM_REGION( 0x2000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "ic13.pop",	  0x0000, 0x1000, CRC(2367131e) SHA1(d7c15536e5e51f406f9b874333386251f4d3934f) )
	ROM_LOAD( "ic3.pop",	  0x1000, 0x1000, CRC(deed0a8b) SHA1(1aaa854f5c6ca3847726cb8a2f2f37f3eb4f621b) )

	ROM_REGION( 0x2000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "ic29.pop",	  0x0000, 0x1000, CRC(7b54f60f) SHA1(9ec979e67313351dd1165ff6cf591aadead30770) )
	ROM_LOAD( "ic38.pop",	  0x1000, 0x1000, CRC(dd2d9601) SHA1(7f495705d6b61c5416e87557a69da7e6457567ac) )

	ROM_REGION( 0x0200, REGION_PROMS, 0 )
	ROM_LOAD( "ic53",		  0x0000, 0x0100, CRC(6e66057f) SHA1(084d630f5e2f23e28a1f7839337ef608e086e8c4) ) /* palette low bits */
	ROM_LOAD( "ic54",		  0x0100, 0x0100, CRC(236bc771) SHA1(5c078eecdd9df2fbc791e440f96bc4c79476b211) ) /* palette high bits */
ROM_END

ROM_START( popflama )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "ic86.pop",	  0x0000, 0x1000, CRC(5e32bbdf) SHA1(b75e3125301d05f5fb6bcef85d0028de2ee75fab) )
	ROM_LOAD( "ic80.pop",	  0x1000, 0x1000, CRC(b77abf3d) SHA1(8626af8fe7d10c52bea7570dd6237de60607bab6) )
	ROM_LOAD( "ic94.pop",	  0x2000, 0x1000, CRC(945a3c0f) SHA1(353fce8904d869bbf654b7be99e76cadf325b47d) )
	ROM_LOAD( "ic100.pop",	  0x3000, 0x1000, CRC(f9f2343b) SHA1(c019a5d838152417ec76be021d659f884928ef87) )

	ROM_REGION( 0x2000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "ic13.pop",	  0x0000, 0x1000, CRC(2367131e) SHA1(d7c15536e5e51f406f9b874333386251f4d3934f) )
	ROM_LOAD( "ic3.pop",	  0x1000, 0x1000, CRC(deed0a8b) SHA1(1aaa854f5c6ca3847726cb8a2f2f37f3eb4f621b) )

	ROM_REGION( 0x2000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "ic29.pop",	  0x0000, 0x1000, CRC(7b54f60f) SHA1(9ec979e67313351dd1165ff6cf591aadead30770) )
	ROM_LOAD( "ic38.pop",	  0x1000, 0x1000, CRC(dd2d9601) SHA1(7f495705d6b61c5416e87557a69da7e6457567ac) )

	ROM_REGION( 0x0200, REGION_PROMS, 0 )
	ROM_LOAD( "ic53",		  0x0000, 0x0100, CRC(6e66057f) SHA1(084d630f5e2f23e28a1f7839337ef608e086e8c4) ) /* palette low bits */
	ROM_LOAD( "ic54",		  0x0100, 0x0100, CRC(236bc771) SHA1(5c078eecdd9df2fbc791e440f96bc4c79476b211) ) /* palette high bits */
ROM_END

ROM_START( popflamb )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "popflama.30",	 0x0000, 0x1000, CRC(a9bb0e8a) SHA1(1948be9545401b8163e0f2fa8e962ea66c26d9e0) )
	ROM_LOAD( "popflama.28",	 0x1000, 0x1000, CRC(debe6d03) SHA1(2365c57a0a08563bea31ab150934dcfc1e6eba58) )
	ROM_LOAD( "popflama.26",	 0x2000, 0x1000, CRC(09df0d4d) SHA1(ddc0227035edd11bec045c09c535ad7a375698f1) )
	ROM_LOAD( "popflama.24",	 0x3000, 0x1000, CRC(f399d553) SHA1(c08c496fcb99370c344185af599e2ad57a327bc9) )

	ROM_REGION( 0x2000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "ic13.pop",	  0x0000, 0x1000, CRC(2367131e) SHA1(d7c15536e5e51f406f9b874333386251f4d3934f) )
	ROM_LOAD( "ic3.pop",	  0x1000, 0x1000, CRC(deed0a8b) SHA1(1aaa854f5c6ca3847726cb8a2f2f37f3eb4f621b) )

	ROM_REGION( 0x2000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "ic29.pop",	  0x0000, 0x1000, CRC(7b54f60f) SHA1(9ec979e67313351dd1165ff6cf591aadead30770) )
	ROM_LOAD( "ic38.pop",	  0x1000, 0x1000, CRC(dd2d9601) SHA1(7f495705d6b61c5416e87557a69da7e6457567ac) )

	ROM_REGION( 0x0200, REGION_PROMS, 0 )
	ROM_LOAD( "ic53",		  0x0000, 0x0100, CRC(6e66057f) SHA1(084d630f5e2f23e28a1f7839337ef608e086e8c4) ) /* palette low bits */
	ROM_LOAD( "ic54",		  0x0100, 0x0100, CRC(236bc771) SHA1(5c078eecdd9df2fbc791e440f96bc4c79476b211) ) /* palette high bits */
ROM_END

ROM_START( trvmstr )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "ic30.bin",     0x0000, 0x1000, CRC(4ccd0537) SHA1(f0991581c2efeb54626dd1f8acf33a28ed1b6f80) )
	ROM_LOAD( "ic28.bin",     0x1000, 0x1000, CRC(782a2b8c) SHA1(611be829470c2fcbb301f48f5e80ad97e51ef821) )
	ROM_LOAD( "ic26.bin",     0x2000, 0x1000, CRC(1362010a) SHA1(d721e051329b823e79515a631244eb77b77c731a) )

	ROM_REGION( 0x2000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "ic44.bin",     0x0000, 0x1000, CRC(dac8cff7) SHA1(21da2b2ceb4a726d03b2e49a2df75ca66b89a197) )
	ROM_LOAD( "ic46.bin",     0x1000, 0x1000, CRC(a97ab879) SHA1(67b86d056896f10e0c055fb58c97341cf75c3d17) )

	ROM_REGION( 0x2000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "ic48.bin",     0x0000, 0x1000, CRC(79952015) SHA1(8407c2bab476a60d945d82201f01bf59ae9e0dad) )
	ROM_LOAD( "ic50.bin",     0x1000, 0x1000, CRC(f09da428) SHA1(092d0eea41c8bbd48d7a3aff54c15f85262b21ff) )

	ROM_REGION( 0x0200, REGION_PROMS, 0 )
	ROM_LOAD( "ic64.bin",     0x0000, 0x0100, CRC(e9915da8) SHA1(7c64ea76e39eaff724179d52ff5482df363fcf56) )  /* palette low & high bits */
	ROM_RELOAD(				  0x0100, 0x0100 )

	ROM_REGION( 0x20000, REGION_USER1, 0 )	/* Questions roms */
	ROM_LOAD( "sport_lo.u2",  0x00000, 0x4000, CRC(24f30489) SHA1(b34ecd485bccb7b78332196e6dffd18721177ac3) )
	ROM_LOAD( "sport_hi.u1",  0x04000, 0x4000, CRC(d64a7480) SHA1(4239c7142d783cbd4242ff58d74e87d87f3535e6) )
	ROM_LOAD( "etain_lo.u4",  0x08000, 0x4000, CRC(a2af9709) SHA1(24858ab58a8a6577446215e261da877cb48c03df) )
	ROM_LOAD( "etain_hi.u3",  0x0c000, 0x4000, CRC(82a60dea) SHA1(2b03a67507c5a5c343804cf40b8b8147df070002) )
	ROM_LOAD( "sex_lo.u6",    0x10000, 0x4000, CRC(f2ecfa88) SHA1(15e9ce1be8b868a99b72426abbdf086fcf134517) )
	ROM_LOAD( "sex_hi.u5",    0x14000, 0x4000, CRC(de4a6c4b) SHA1(ba12193eabcee7e4d354678ddd780e1e338efbb1) )
	ROM_LOAD( "scien_lo.u8",  0x18000, 0x4000, CRC(01a01ff1) SHA1(d3b62ae466681ae01ab1beaf2958af94c9c4cbcb) )
	ROM_LOAD( "scien_hi.u7",  0x1c000, 0x4000, CRC(0bc68078) SHA1(910cd1a8ca68cff87c93a8ffa810d77338fc710b) )
ROM_END

ROM_START( trvmstra )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "ic30a.bin",    0x0000, 0x2000, CRC(4c175c45) SHA1(770e128ad30ef6ad9936cbf4da810c8b38c7b630) )
	ROM_LOAD( "ic28a.bin",    0x1000, 0x2000, CRC(3a8ca87d) SHA1(bf82ca226daa13eabf8db3cabe2c047b831188e8) )
	ROM_LOAD( "ic26a.bin",    0x2000, 0x2000, CRC(3c655400) SHA1(d536e7cf63834b0ce94fb4e597c370befa792f82) )

	ROM_REGION( 0x2000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "ic44.bin",     0x0000, 0x1000, CRC(dac8cff7) SHA1(21da2b2ceb4a726d03b2e49a2df75ca66b89a197) )
	ROM_LOAD( "ic46.bin",     0x1000, 0x1000, CRC(a97ab879) SHA1(67b86d056896f10e0c055fb58c97341cf75c3d17) )

	ROM_REGION( 0x2000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "ic48.bin",     0x0000, 0x1000, CRC(79952015) SHA1(8407c2bab476a60d945d82201f01bf59ae9e0dad) )
	ROM_LOAD( "ic50.bin",     0x1000, 0x1000, CRC(f09da428) SHA1(092d0eea41c8bbd48d7a3aff54c15f85262b21ff) )

	ROM_REGION( 0x0200, REGION_PROMS, 0 )
	ROM_LOAD( "ic64.bin",     0x0000, 0x0100, CRC(e9915da8) SHA1(7c64ea76e39eaff724179d52ff5482df363fcf56) )  /* palette low & high bits */
	ROM_RELOAD(				  0x0100, 0x0100 )

	ROM_REGION( 0x20000, REGION_USER1, 0 )	/* Questions roms */
	ROM_LOAD( "enter_lo.u2",  0x00000, 0x4000, CRC(a65b8f83) SHA1(a86bef07349a00aa977270e3504cf2698c7c6333) )
	ROM_LOAD( "enter_hi.u1",  0x04000, 0x4000, CRC(caede447) SHA1(ee6d015e3e7d338926296c69eab3e07dbb64a8e6) )
	ROM_LOAD( "sports_lo.u4", 0x08000, 0x4000, CRC(d5317b26) SHA1(8d93cf9c15b25687f224e01f332f53cac3180b83) )
	ROM_LOAD( "sports_hi.u3", 0x0c000, 0x4000, CRC(9f706db2) SHA1(171b5c490bd576d33355cfd3cd4d1b0c5cb90e00) )
	ROM_LOAD( "sex2_lo.u6",   0x10000, 0x4000, CRC(b73f2e31) SHA1(4390152e053118c31ed74fe850ea7124c0e7b731) )
	ROM_LOAD( "sex2_hi.u5",   0x14000, 0x4000, CRC(bf654110) SHA1(5229f5e6973a04c53572ea94c14d79a238c0e90f) )
	ROM_LOAD( "comic_lo.u8",  0x18000, 0x4000, CRC(109bd359) SHA1(ea8cb4b0a14a3ef4932947afdfa773ecc34c2b9b) )
	ROM_LOAD( "comic_hi.u7",  0x1c000, 0x4000, CRC(8e8b5f71) SHA1(71514af2af2468a13cf5cc4237fa2590d7a16b27) )
ROM_END

ROM_START( trvgns )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )
	ROM_LOAD( "trvgns.30",   0x0000, 0x1000, CRC(a17f172c) SHA1(b831673f860f6b7566e248b13b349d82379b5e72) )
	ROM_LOAD( "trvgns.28",   0x1000, 0x1000, CRC(681a1bff) SHA1(53da179185ae3bfb30502706cc623c2f4cc57128) )
	ROM_LOAD( "trvgns.26",   0x2000, 0x1000, CRC(5b4068b8) SHA1(3b424dd8e2a6fa1e4628790f60c51d44f9a535a1) )

	ROM_REGION( 0x2000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "trvgns.44",   0x0000, 0x1000, CRC(cd67f2cb) SHA1(22d9d8509fd44fbeb313f5120e692d7a30e3ca54) )
	ROM_LOAD( "trvgns.46",   0x1000, 0x1000, CRC(f4021941) SHA1(81a93b5b2bf46e2f5254a86b14e31b31b7821d4f) )

	ROM_REGION( 0x2000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "trvgns.48",   0x0000, 0x1000, NO_DUMP )
	ROM_LOAD( "trvgns.50",   0x1000, 0x1000, CRC(ac292be8) SHA1(41f95273907b27158af0631c716fdb9301852e27) )
	ROM_RELOAD(				 0x0000, 0x1000 )	//until the rom is redumped

	ROM_REGION( 0x0200, REGION_PROMS, 0 )
	/* wrong! from trvmstr just to see the game */
	ROM_LOAD( "ic64.bin",     0x0000, 0x0100, BAD_DUMP CRC(e9915da8) SHA1(7c64ea76e39eaff724179d52ff5482df363fcf56) )  /* palette low & high bits */
	ROM_RELOAD(				  0x0100, 0x0100 )

	ROM_REGION( 0x20000, REGION_USER1, 0 ) /* Question roms */
	ROM_LOAD( "trvgns.u2",   0x00000, 0x4000, CRC(109bd359) SHA1(ea8cb4b0a14a3ef4932947afdfa773ecc34c2b9b) )
	ROM_LOAD( "trvgns.u1",   0x04000, 0x4000, CRC(8e8b5f71) SHA1(71514af2af2468a13cf5cc4237fa2590d7a16b27) )
	ROM_LOAD( "trvgns.u4",   0x08000, 0x4000, CRC(b73f2e31) SHA1(4390152e053118c31ed74fe850ea7124c0e7b731) )
	ROM_LOAD( "trvgns.u3",   0x0c000, 0x4000, CRC(bf654110) SHA1(5229f5e6973a04c53572ea94c14d79a238c0e90f) )
	ROM_LOAD( "trvgns.u6",   0x10000, 0x4000, CRC(4a2263a7) SHA1(63f2f79261d508c9bba3d73d78f7dce5d348b6d4) )
	ROM_LOAD( "trvgns.u5",   0x14000, 0x4000, CRC(bd31f382) SHA1(ec04a5d4a5fc8be059abf3c21c65cd970e569d44) )
	ROM_LOAD( "trvgns.u8",   0x18000, 0x4000, CRC(dbfce45f) SHA1(5d96186c96dee810b0ef63964cb3614fd486aefa) )
	ROM_LOAD( "trvgns.u7",   0x1c000, 0x4000, CRC(c8f5a02d) SHA1(8a566f83f9bd39ab508085af942957a7ed941813) )
ROM_END


DRIVER_INIT( popflame )
{
	/* install a handler to catch protection checks */
	memory_install_read8_handler(0, ADDRESS_SPACE_PROGRAM, 0x9000, 0x9000, 0, 0, popflame_protection_r);
}

static int question_offset = 0;

static READ8_HANDLER( trvmstr_questions_r )
{
	return memory_region(REGION_USER1)[question_offset];
}

static WRITE8_HANDLER( trvmstr_questions_w )
{
	switch(offset)
	{
	case 0:
		question_offset = (question_offset & 0xffff00) | data;
		break;
	case 1:
		question_offset = (question_offset & 0xff00ff) | (data << 8);
		break;
	case 2:
		question_offset = (question_offset & 0x00ffff) | (data << 16);
		break;
	}
}

DRIVER_INIT( trvmstr )
{
	/* install questions' handlers  */
	memory_install_read8_handler(0, ADDRESS_SPACE_PROGRAM, 0xc000, 0xc000, 0, 0, trvmstr_questions_r);
	memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM, 0xc000, 0xc002, 0, 0, trvmstr_questions_w);
}


GAME( 1982, naughtyb, 0,		naughtyb, naughtyb, 0,        ROT90, "Jaleco", "Naughty Boy", 0 )
GAME( 1982, naughtya, naughtyb, naughtyb, naughtyb, 0,        ROT90, "bootleg", "Naughty Boy (bootleg)", 0 )
GAME( 1982, naughtyc, naughtyb, naughtyb, naughtyb, 0,        ROT90, "Jaleco (Cinematronics license)", "Naughty Boy (Cinematronics)", 0 )
GAME( 1982, popflame, 0,		popflame, naughtyb, popflame, ROT90, "Jaleco", "Pop Flamer (protected)", 0 )
GAME( 1982, popflama, popflame, popflame, naughtyb, 0,        ROT90, "Jaleco", "Pop Flamer (not protected)", 0 )
GAME( 1982, popflamb, popflame, popflame, naughtyb, 0,        ROT90, "Jaleco", "Pop Flamer (hack?)", 0 )
GAME( 1985, trvmstr,  0,		naughtyb, trvmstr,  trvmstr,  ROT90, "Enerdyne Technologies Inc.", "Trivia Master (set 1)", 0 )
GAME( 1985, trvmstra, trvmstr,  naughtyb, trvmstr,  trvmstr,  ROT90, "Enerdyne Technologies Inc.", "Trivia Master (set 2)", 0 )
GAME( 198?, trvgns,   0,		naughtyb, trvmstr,  trvmstr,  ROT90, "Enerdyne Technologies Inc.", "Trivia Genius", GAME_WRONG_COLORS )
