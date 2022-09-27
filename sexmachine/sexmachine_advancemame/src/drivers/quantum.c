/***************************************************************************

    Atari Quantum hardware

    driver by Paul Forgey, with some help from Aaron Giles

    Games supported:
        * Quantum

    Known bugs:
        * none at this time

****************************************************************************

    Memory map

****************************************************************************

    QUANTUM MEMORY MAP (per schem):

    000000-003FFF   ROM0
    004000-004FFF   ROM1
    008000-00BFFF   ROM2
    00C000-00FFFF   ROM3
    010000-013FFF   ROM4

    018000-01BFFF   RAM0
    01C000-01CFFF   RAM1

    940000          TRACKBALL
    948000          SWITCHES
    950000          COLORRAM
    958000          CONTROL (LED and coin control)
    960000-970000   RECALL (nvram read)
    968000          VGRST (vector reset)
    970000          VGGO (vector go)
    978000          WDCLR (watchdog)
    900000          NVRAM (nvram write)
    840000          I/OS (sound and dip switches)
    800000-801FFF   VMEM (vector display list)
    940000          I/O (shematic label really - covered above)
    900000          DTACK1

***************************************************************************/

#include "driver.h"
#include "vidhrdw/vector.h"
#include "vidhrdw/avgdvg.h"
#include "sound/pokey.h"



/*************************************
 *
 *  Inputs
 *
 *************************************/

static READ16_HANDLER( switches_r )
{
	return (readinputport(0) | (avgdvg_done() ? 1 : 0));
}


static READ16_HANDLER( trackball_r )
{
	return (readinputport(4) << 4) | readinputport(3);
}


static READ8_HANDLER( input_1_r )
{
	return (readinputport(1) << (7 - (offset - POT0_C))) & 0x80;
}


static READ8_HANDLER( input_2_r )
{
	return (readinputport(2) << (7 - (offset - POT0_C))) & 0x80;
}



/*************************************
 *
 *  LEDs/coin counters
 *
 *************************************/

static WRITE16_HANDLER( led_w )
{
	if (ACCESSING_LSB)
	{
		/* bits 0 and 1 are coin counters */
		coin_counter_w(0, data & 2);
		coin_counter_w(1, data & 1);

		/* bit 3 = select second trackball for cocktail mode? */

		/* bits 4 and 5 are LED controls */
		set_led_status(0, data & 0x10);
		set_led_status(1, data & 0x20);

		/* bits 6 and 7 flip screen */
		avg_set_flip_x (data & 0x40);
		avg_set_flip_y (data & 0x80);
	}
}



/*************************************
 *
 *  POKEY I/O
 *
 *************************************/

static WRITE16_HANDLER( pokey_word_w )
{
	if (offset & 0x10) /* A5 selects chip */
		pokey2_w(offset & 0x0f, data);
	else
		pokey1_w(offset & 0x0f, data);
}


static READ16_HANDLER( pokey_word_r )
{
	if (offset & 0x10)
		return pokey2_r(offset & 0x0f);
	else
		return pokey1_r(offset & 0x0f);
}



/*************************************
 *
 *  Main CPU memory handlers
 *
 *************************************/

ADDRESS_MAP_START( readmem, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x013fff) AM_READ(MRA16_ROM)
	AM_RANGE(0x018000, 0x01cfff) AM_READ(MRA16_RAM)
	AM_RANGE(0x800000, 0x801fff) AM_READ(MRA16_RAM)
	AM_RANGE(0x840000, 0x84003f) AM_READ(pokey_word_r)
	AM_RANGE(0x900000, 0x9001ff) AM_READ(MRA16_RAM)
	AM_RANGE(0x940000, 0x940001) AM_READ(trackball_r) /* trackball */
	AM_RANGE(0x948000, 0x948001) AM_READ(switches_r)
	AM_RANGE(0x978000, 0x978001) AM_READ(MRA16_NOP)	/* ??? */
ADDRESS_MAP_END


ADDRESS_MAP_START( writemem, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x013fff) AM_WRITE(MWA16_ROM)
	AM_RANGE(0x018000, 0x01cfff) AM_WRITE(MWA16_RAM)
	AM_RANGE(0x800000, 0x801fff) AM_WRITE(MWA16_RAM) AM_BASE((UINT16 **)&vectorram) AM_SIZE(&vectorram_size)
	AM_RANGE(0x840000, 0x84003f) AM_WRITE(pokey_word_w)
	AM_RANGE(0x900000, 0x9001ff) AM_WRITE(MWA16_RAM) AM_BASE(&generic_nvram16) AM_SIZE(&generic_nvram_size)
	AM_RANGE(0x950000, 0x95001f) AM_WRITE(quantum_colorram_w)
	AM_RANGE(0x958000, 0x958001) AM_WRITE(led_w)
	AM_RANGE(0x960000, 0x960001) AM_WRITE(MWA16_NOP)	/* enable NVRAM? */
	AM_RANGE(0x968000, 0x968001) AM_WRITE(avgdvg_reset_word_w)
//  AM_RANGE(0x970000, 0x970001) AM_WRITE(avgdvg_go_w)
//  AM_RANGE(0x978000, 0x978001) AM_WRITE(watchdog_reset_w)
	/* the following is wrong, but it's the only way I found to fix the service mode */
	AM_RANGE(0x978000, 0x978001) AM_WRITE(avgdvg_go_word_w)
ADDRESS_MAP_END



/*************************************
 *
 *  Port definitions
 *
 *************************************/

INPUT_PORTS_START( quantum )
	PORT_START		/* IN0 */
	/* YHALT here MUST BE ALWAYS 0  */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH,IPT_SPECIAL )	/* vg YHALT */
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN3 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_SERVICE( 0x80, IP_ACTIVE_LOW )

/* first POKEY is SW2, second is SW1 -- more confusion! */
	PORT_START 		/* DSW0 */
	PORT_DIPNAME( 0xc0, 0x00, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0x80, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0xc0, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Free_Play ) )
	PORT_DIPNAME( 0x30, 0x00, "Right Coin" )
	PORT_DIPSETTING(    0x00, "*1" )
	PORT_DIPSETTING(    0x20, "*4" )
	PORT_DIPSETTING(    0x10, "*5" )
	PORT_DIPSETTING(    0x30, "*6" )
	PORT_DIPNAME( 0x08, 0x00, "Left Coin" )
	PORT_DIPSETTING(    0x00, "*1" )
	PORT_DIPSETTING(    0x08, "*2" )
	PORT_DIPNAME( 0x07, 0x00, "Bonus Coins" )
	PORT_DIPSETTING(    0x00, DEF_STR( None ) )
	PORT_DIPSETTING(    0x01, "1 each 5" )
	PORT_DIPSETTING(    0x02, "1 each 4" )
	PORT_DIPSETTING(    0x05, "1 each 3" )
	PORT_DIPSETTING(    0x06, "2 each 4" )

	PORT_START		/* DSW1 */
	PORT_BIT( 0xff, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START      /* IN2 */
	PORT_BIT( 0x0f, 0, IPT_TRACKBALL_Y ) PORT_SENSITIVITY(10) PORT_KEYDELTA(10) PORT_REVERSE

	PORT_START      /* IN3 */
	PORT_BIT( 0x0f, 0, IPT_TRACKBALL_X ) PORT_SENSITIVITY(10) PORT_KEYDELTA(10)
INPUT_PORTS_END



/*************************************
 *
 *  Sound definitions
 *
 *************************************/

static struct POKEYinterface pokey_interface_1 =
{
	{ input_1_r,input_1_r,input_1_r,input_1_r,input_1_r,input_1_r,input_1_r,input_1_r }
};

static struct POKEYinterface pokey_interface_2 =
{
	{ input_2_r,input_2_r,input_2_r,input_2_r,input_2_r,input_2_r,input_2_r,input_2_r }
};



/*************************************
 *
 *  Machine driver
 *
 *************************************/

static MACHINE_DRIVER_START( quantum )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000,6000000)
	MDRV_CPU_PROGRAM_MAP(readmem,writemem)
	MDRV_CPU_VBLANK_INT(irq1_line_hold,3)	/* IRQ rate = 750kHz/4096 */

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_NVRAM_HANDLER(generic_1fill)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_VECTOR | VIDEO_RGB_DIRECT)
	MDRV_SCREEN_SIZE(400, 300)
	MDRV_VISIBLE_AREA(0, 900, 0, 600)
	MDRV_PALETTE_LENGTH(32768)

	MDRV_PALETTE_INIT(avg_multi)
	MDRV_VIDEO_START(avg_quantum)
	MDRV_VIDEO_UPDATE(vector)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")

	MDRV_SOUND_ADD(POKEY, 600000)
	MDRV_SOUND_CONFIG(pokey_interface_1)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.50)

	MDRV_SOUND_ADD(POKEY, 600000)
	MDRV_SOUND_CONFIG(pokey_interface_2)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.50)
MACHINE_DRIVER_END



/*************************************
 *
 *  ROM definition(s)
 *
 *************************************/

ROM_START( quantum )
	ROM_REGION( 0x014000, REGION_CPU1, 0 )
    ROM_LOAD16_BYTE( "136016.201",   0x000000, 0x002000, CRC(7e7be63a) SHA1(11b2d0168cdbaa7a48656b77abc0bcbe9408fe84) )
    ROM_LOAD16_BYTE( "136016.206",   0x000001, 0x002000, CRC(2d8f5759) SHA1(54b0388ef44b5d34e621b48b465566aa16887e8f) )
    ROM_LOAD16_BYTE( "136016.102",   0x004000, 0x002000, CRC(408d34f4) SHA1(9a30debd1240b9c103134701943c94d6b48b926d) )
    ROM_LOAD16_BYTE( "136016.107",   0x004001, 0x002000, CRC(63154484) SHA1(c098cdbc339c9ea291c4c4fb203c60b3284e894a) )
    ROM_LOAD16_BYTE( "136016.203",   0x008000, 0x002000, CRC(bdc52fad) SHA1(c8ede54a4f7f555adffa5b4bfea6bf646a0d02d4) )
    ROM_LOAD16_BYTE( "136016.208",   0x008001, 0x002000, CRC(dab4066b) SHA1(dbb82df8e6de4e0f9f6e7ddd5f07618864fce8f9) )
    ROM_LOAD16_BYTE( "136016.104",   0x00C000, 0x002000, CRC(bf271e5c) SHA1(012edb947f1437932b9283e49d025a7794c45669) )
    ROM_LOAD16_BYTE( "136016.109",   0x00C001, 0x002000, CRC(d2894424) SHA1(5390025136b677b66d948c8cf6ea5e20203a4bae) )
    ROM_LOAD16_BYTE( "136016.105",   0x010000, 0x002000, CRC(13ec512c) SHA1(22a0395135b83ba47eacb5129f34fc97aa1b70a1) )
    ROM_LOAD16_BYTE( "136016.110",   0x010001, 0x002000, CRC(acb50363) SHA1(9efa9ca88efdd2d5e212bd537903892b67b4fe53) )
ROM_END


ROM_START( quantum1 )
	ROM_REGION( 0x014000, REGION_CPU1, 0 )
    ROM_LOAD16_BYTE( "136016.101",   0x000000, 0x002000, CRC(5af0bd5b) SHA1(f6e46fbebbf52294e78ae240fe2628c6b29b8dea) )
    ROM_LOAD16_BYTE( "136016.106",   0x000001, 0x002000, CRC(f9724666) SHA1(1bb073135029c92bef9afc9ccd910e0ab3302c8a) )
    ROM_LOAD16_BYTE( "136016.102",   0x004000, 0x002000, CRC(408d34f4) SHA1(9a30debd1240b9c103134701943c94d6b48b926d) )
    ROM_LOAD16_BYTE( "136016.107",   0x004001, 0x002000, CRC(63154484) SHA1(c098cdbc339c9ea291c4c4fb203c60b3284e894a) )
    ROM_LOAD16_BYTE( "136016.103",   0x008000, 0x002000, CRC(948f228b) SHA1(878ac96173a793997cc88be469ec1ccdf833a7e8) )
    ROM_LOAD16_BYTE( "136016.108",   0x008001, 0x002000, CRC(e4c48e4e) SHA1(caaf9d20741fcb961d590b634250a44a166cc33a) )
    ROM_LOAD16_BYTE( "136016.104",   0x00C000, 0x002000, CRC(bf271e5c) SHA1(012edb947f1437932b9283e49d025a7794c45669) )
    ROM_LOAD16_BYTE( "136016.109",   0x00C001, 0x002000, CRC(d2894424) SHA1(5390025136b677b66d948c8cf6ea5e20203a4bae) )
    ROM_LOAD16_BYTE( "136016.105",   0x010000, 0x002000, CRC(13ec512c) SHA1(22a0395135b83ba47eacb5129f34fc97aa1b70a1) )
    ROM_LOAD16_BYTE( "136016.110",   0x010001, 0x002000, CRC(acb50363) SHA1(9efa9ca88efdd2d5e212bd537903892b67b4fe53) )
ROM_END


ROM_START( quantump )
	ROM_REGION( 0x014000, REGION_CPU1, 0 )
    ROM_LOAD16_BYTE( "quantump.2e",  0x000000, 0x002000, CRC(176d73d3) SHA1(b887ee50af5db6f6d43cc6ba57451173f996dedc) )
    ROM_LOAD16_BYTE( "quantump.3e",  0x000001, 0x002000, CRC(12fc631f) SHA1(327a44da897199536f43e5f792cb4a18d9055ac4) )
    ROM_LOAD16_BYTE( "quantump.2f",  0x004000, 0x002000, CRC(b64fab48) SHA1(d5a77a367d4f652261c381e6bdd55c2175ace857) )
    ROM_LOAD16_BYTE( "quantump.3f",  0x004001, 0x002000, CRC(a52a9433) SHA1(33787adb04864efebb04483353bbc96c966ec607) )
    ROM_LOAD16_BYTE( "quantump.2h",  0x008000, 0x002000, CRC(5b29cba3) SHA1(e83b68907bc397994ed51a39dfa241430a0adb0c) )
    ROM_LOAD16_BYTE( "quantump.3h",  0x008001, 0x002000, CRC(c64fc03a) SHA1(ab6cd710d01bc85432cc52021f27fd8f2a5e3168) )
    ROM_LOAD16_BYTE( "quantump.2k",  0x00C000, 0x002000, CRC(854f9c09) SHA1(d908b8c7f6837e511004cbd45a8883c6c7b155dd) )
    ROM_LOAD16_BYTE( "quantump.3k",  0x00C001, 0x002000, CRC(1aac576c) SHA1(28bdb5fcbd8cccc657d6e00ace3c083c21015564) )
    ROM_LOAD16_BYTE( "quantump.2l",  0x010000, 0x002000, CRC(1285b5e7) SHA1(0e01e361da2d9cf1fac1896f8f44c4c2e75a3061) )
    ROM_LOAD16_BYTE( "quantump.3l",  0x010001, 0x002000, CRC(e19de844) SHA1(cb4f9d80807b26d6b95405b2d830799984667f54) )
ROM_END



/*************************************
 *
 *  Game driver(s)
 *
 *************************************/

GAME( 1982, quantum,  0,       quantum, quantum, 0, ROT270, "Atari", "Quantum (rev 2)", 0 )
GAME( 1982, quantum1, quantum, quantum, quantum, 0, ROT270, "Atari", "Quantum (rev 1)", 0 )
GAME( 1982, quantump, quantum, quantum, quantum, 0, ROT270, "Atari", "Quantum (prototype)", 0 )
