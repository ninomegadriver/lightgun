/***************************************************************************

    Kitco Crowns Golf hardware

    driver by Aaron Giles

    Games supported:
        * Crowns Golf (4 sets)
        * Crowns Golf in Hawaii

    Known bugs:
        * not sure if the analog inputs are handled correctly

Text Strings in sound CPU ROM read:
ARIES ELECA
1984JAN15 V-0

Text Strings in the bootleg sound CPU ROM read:
WHO AM I?      (In place of "ARIES ELECA")
1984JULY1 V-1  (In place of "1984JAN15 V-0")
1984 COPYRIGHT BY WHO

****************************************************************************

    Memory map (TBA)

***************************************************************************/

#include "driver.h"
#include "crgolf.h"
#include "sound/ay8910.h"
#include "sound/msm5205.h"


/* constants */
#define MASTER_CLOCK		18432000


/* local variables */
static UINT8 port_select;
static UINT8 main_to_sound_data, sound_to_main_data;
static UINT16 sample_offset;
static UINT8 sample_count;



/*************************************
 *
 *  ROM banking
 *
 *************************************/

static WRITE8_HANDLER( rom_bank_select_w )
{
	memory_set_bank(1, data & 15);
}


static MACHINE_START( crgolf )
{
	/* configure the banking */
	memory_configure_bank(1, 0, 16, memory_region(REGION_CPU1) + 0x10000, 0x2000);
	memory_set_bank(1, 0);

	/* register for save states */
	state_save_register_global(port_select);
	state_save_register_global(main_to_sound_data);
	state_save_register_global(sound_to_main_data);
	state_save_register_global(sample_offset);
	state_save_register_global(sample_count);
	return 0;
}



/*************************************
 *
 *  Input ports
 *
 *************************************/

static READ8_HANDLER( switch_input_r )
{
	return readinputport(port_select);
}


static READ8_HANDLER( analog_input_r )
{
	return ((readinputport(7) >> 4) | (readinputport(8) & 0xf0)) ^ 0x88;
}


static WRITE8_HANDLER( switch_input_select_w )
{
	if (!(data & 0x40)) port_select = 6;
	if (!(data & 0x20)) port_select = 5;
	if (!(data & 0x10)) port_select = 4;
	if (!(data & 0x08)) port_select = 3;
	if (!(data & 0x04)) port_select = 2;
	if (!(data & 0x02)) port_select = 1;
	if (!(data & 0x01)) port_select = 0;
}


static WRITE8_HANDLER( unknown_w )
{
	logerror("%04X:unknown_w = %02X\n", activecpu_get_pc(), data);
}



/*************************************
 *
 *  Main->Sound CPU communications
 *
 *************************************/

static void main_to_sound_callback(int param)
{
	cpunum_set_input_line(1, INPUT_LINE_NMI, ASSERT_LINE);
	main_to_sound_data = param;
}


static WRITE8_HANDLER( main_to_sound_w )
{
	timer_set(TIME_NOW, data, main_to_sound_callback);
}


static READ8_HANDLER( main_to_sound_r )
{
	cpunum_set_input_line(1, INPUT_LINE_NMI, CLEAR_LINE);
	return main_to_sound_data;
}



/*************************************
 *
 *  Sound->Main CPU communications
 *
 *************************************/

static void sound_to_main_callback(int param)
{
	cpunum_set_input_line(0, INPUT_LINE_NMI, ASSERT_LINE);
	sound_to_main_data = param;
}


static WRITE8_HANDLER( sound_to_main_w )
{
	timer_set(TIME_NOW, data, sound_to_main_callback);
}


static READ8_HANDLER( sound_to_main_r )
{
	cpunum_set_input_line(0, INPUT_LINE_NMI, CLEAR_LINE);
	return sound_to_main_data;
}



/*************************************
 *
 *  Hawaii auto-sample player
 *
 *************************************/

static void vck_callback(int data)
{
	/* only play back if we have data remaining */
	if (sample_count != 0xff)
	{
		UINT8 data = memory_region(REGION_SOUND1)[sample_offset >> 1];

		/* write the next nibble and advance */
		MSM5205_data_w(0, (data >> (4 * (~sample_offset & 1))) & 0x0f);
		sample_offset++;

		/* every 256 clocks, we decrement the length */
		if (!(sample_offset & 0xff))
		{
			sample_count--;

			/* if we hit 0xff, automatically turn off playback */
			if (sample_count == 0xff)
				MSM5205_reset_w(0, 1);
		}
	}
}


static WRITE8_HANDLER( crgolfhi_sample_w )
{
	switch (offset)
	{
		/* offset 0 holds the MSM5205 in reset */
		case 0:
			MSM5205_reset_w(0, 1);
			break;

		/* offset 1 is the length/256 nibbles */
		case 1:
			sample_count = data;
			break;

		/* offset 2 is the offset/256 nibbles */
		case 2:
			sample_offset = data << 8;
			break;

		/* offset 3 turns on playback */
		case 3:
			MSM5205_reset_w(0, 0);
			break;
	}
}



/*************************************
 *
 *  Main CPU memory handlers
 *
 *************************************/

static ADDRESS_MAP_START( main_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x3fff) AM_ROM
	AM_RANGE(0x4000, 0x5fff) AM_RAM
	AM_RANGE(0x6000, 0x7fff) AM_ROMBANK(1)
	AM_RANGE(0x8003, 0x8003) AM_WRITE(MWA8_RAM) AM_BASE(&crgolf_color_select)
	AM_RANGE(0x8004, 0x8004) AM_WRITE(MWA8_RAM) AM_BASE(&crgolf_screen_flip)
	AM_RANGE(0x8005, 0x8005) AM_WRITE(MWA8_RAM) AM_BASE(&crgolf_screen_select)
	AM_RANGE(0x8006, 0x8006) AM_WRITE(MWA8_RAM) AM_BASE(&crgolf_screenb_enable)
	AM_RANGE(0x8007, 0x8007) AM_WRITE(MWA8_RAM) AM_BASE(&crgolf_screena_enable)
	AM_RANGE(0x8800, 0x8800) AM_READWRITE(sound_to_main_r, main_to_sound_w)
	AM_RANGE(0x9000, 0x9000) AM_WRITE(rom_bank_select_w)
	AM_RANGE(0xa000, 0xbfff) AM_READWRITE(crgolf_videoram_bit1_r, crgolf_videoram_bit1_w)
	AM_RANGE(0xc000, 0xdfff) AM_READWRITE(crgolf_videoram_bit0_r, crgolf_videoram_bit0_w)
	AM_RANGE(0xe000, 0xffff) AM_READWRITE(crgolf_videoram_bit2_r, crgolf_videoram_bit2_w)
ADDRESS_MAP_END



/*************************************
 *
 *  Sound CPU memory handlers
 *
 *************************************/

static ADDRESS_MAP_START( sound_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x7fff) AM_ROM
	AM_RANGE(0x8000, 0x87ff) AM_RAM
	AM_RANGE(0xc000, 0xc000) AM_WRITE(AY8910_control_port_0_w)
	AM_RANGE(0xc001, 0xc001) AM_WRITE(AY8910_write_port_0_w)
	AM_RANGE(0xc002, 0xc002) AM_WRITE(MWA8_NOP)
	AM_RANGE(0xe000, 0xe000) AM_READWRITE(switch_input_r, switch_input_select_w)
	AM_RANGE(0xe001, 0xe001) AM_READWRITE(analog_input_r, unknown_w)
	AM_RANGE(0xe003, 0xe003) AM_READWRITE(main_to_sound_r, sound_to_main_w)
ADDRESS_MAP_END



/*************************************
 *
 *  Port definitions
 *
 *************************************/

INPUT_PORTS_START( crgolf )
	PORT_START	/* CREDIT */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_SERVICE1 )
	PORT_BIT( 0xfc, IP_ACTIVE_HIGH, IPT_UNUSED )

	PORT_START	/* SELECT */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_START3 )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_START4 )
	PORT_BIT( 0xe0, IP_ACTIVE_HIGH, IPT_UNUSED )

	PORT_START	/* PLAY1 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_BUTTON6 ) PORT_PLAYER(1)			/* club select */
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_BUTTON2 ) PORT_PLAYER(1)			/* backward address */
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_BUTTON3 ) PORT_PLAYER(1)			/* forward address */
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_BUTTON4 ) PORT_PLAYER(1)			/* open stance */
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON5 ) PORT_PLAYER(1)			/* closed stance */
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT ) PORT_PLAYER(1)	/* direction left */
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT ) PORT_PLAYER(1)	/* direction right */
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_BUTTON1 ) PORT_PLAYER(1)			/* shot switch */

	PORT_START	/* PLAY2 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_BUTTON6 ) PORT_COCKTAIL		/* club select */
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_BUTTON2 ) PORT_COCKTAIL		/* backward address */
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_BUTTON3 ) PORT_COCKTAIL		/* forward address */
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_BUTTON4 ) PORT_COCKTAIL		/* open stance */
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON5 ) PORT_COCKTAIL		/* closed stance */
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT ) PORT_COCKTAIL	/* direction left */
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT ) PORT_COCKTAIL	/* direction right */
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_BUTTON1 ) PORT_COCKTAIL		/* shot switch */

	PORT_START	/* DIPSW */
	PORT_DIPNAME( 0x01, 0x00, DEF_STR( Difficulty ))
	PORT_DIPSETTING(    0x00, DEF_STR( Easy ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Hard ) )
	PORT_DIPNAME( 0x06, 0x04, "Half-Round Play" )
	PORT_DIPSETTING(    0x00, "4 Coins" )
	PORT_DIPSETTING(    0x02, "5 Coins" )
	PORT_DIPSETTING(    0x04, "6 Coins" )
	PORT_DIPSETTING(    0x06, "10 Coins" )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Cabinet ))
	PORT_DIPSETTING(    0x08, DEF_STR( Upright ))
	PORT_DIPSETTING(    0x00, DEF_STR( Cocktail ))
	PORT_DIPNAME( 0x10, 0x00, "Clear High Scores" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ))
	PORT_DIPSETTING(    0x10, DEF_STR( On ))
	PORT_DIPNAME( 0x20, 0x00, DEF_STR( Coinage ))
	PORT_DIPSETTING(    0x20, DEF_STR( 2C_1C ))
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_1C ))
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_SERVICE( 0x80, IP_ACTIVE_HIGH )

	PORT_START
	PORT_BIT( 0xff, IP_ACTIVE_HIGH, IPT_UNUSED )

	PORT_START
	PORT_BIT( 0xff, IP_ACTIVE_HIGH, IPT_UNUSED )

	PORT_START
	PORT_BIT( 0xff, 0x80, IPT_AD_STICK_Y ) PORT_MINMAX(0,255) PORT_SENSITIVITY(70) PORT_KEYDELTA(16) PORT_REVERSE

	PORT_START
	PORT_BIT( 0xff, 0x80, IPT_AD_STICK_Y ) PORT_MINMAX(0,255) PORT_SENSITIVITY(70) PORT_KEYDELTA(16) PORT_REVERSE PORT_COCKTAIL
INPUT_PORTS_END



/*************************************
 *
 *  Sound definitions
 *
 *************************************/

static struct MSM5205interface msm5205_intf =
{
	vck_callback,
	MSM5205_S64_4B
};



/*************************************
 *
 *  Machine driver
 *
 *************************************/

static MACHINE_DRIVER_START( crgolf )

	/* basic machine hardware */
	MDRV_CPU_ADD(Z80,MASTER_CLOCK/3/2)
	MDRV_CPU_PROGRAM_MAP(main_map,0)
	MDRV_CPU_VBLANK_INT(irq0_line_hold,1)

	MDRV_CPU_ADD(Z80,MASTER_CLOCK/3/2)
	MDRV_CPU_PROGRAM_MAP(sound_map,0)
	MDRV_CPU_VBLANK_INT(irq0_line_hold,1)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_REAL_60HZ_VBLANK_DURATION)

	MDRV_MACHINE_START(crgolf)
	MDRV_INTERLEAVE(100)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(256, 256)
	MDRV_VISIBLE_AREA(0, 255, 8, 247)
	MDRV_PALETTE_LENGTH(32)

	MDRV_PALETTE_INIT(crgolf)
	MDRV_VIDEO_START(crgolf)
	MDRV_VIDEO_UPDATE(crgolf)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")
	MDRV_SOUND_ADD(AY8910, MASTER_CLOCK/3/2/2)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.0)
MACHINE_DRIVER_END


MACHINE_DRIVER_START( crgolfhi )
	MDRV_IMPORT_FROM(crgolf)

	MDRV_SOUND_ADD(MSM5205, 384000)
	MDRV_SOUND_CONFIG(msm5205_intf)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.0)
MACHINE_DRIVER_END



/*************************************
 *
 *  ROM definition(s)
 *
 *************************************/

ROM_START( crgolf ) // 834-5419-03
	ROM_REGION( 0x30000, REGION_CPU1, 0 )
	ROM_LOAD( "epr6143.1c", 0x00000, 0x2000, CRC(4b301360) SHA1(2a7dd4876f4448b4b59b6dd02e55eb2d0126b777) )
	ROM_LOAD( "epr6142.1a", 0x02000, 0x2000, CRC(8fc5e67f) SHA1(6563db94c55cfc7d2270daccaab57fc7b422b9f9) )
	ROM_LOAD( "epr5880.6b", 0x10000, 0x2000, CRC(4d6d8dad) SHA1(1530f81ad0097eadc75884ff8690b60b85ae451b) )
	ROM_LOAD( "epr5885.5e", 0x1e000, 0x2000, CRC(fac6d56c) SHA1(67dc1918d5ab2443e967359e51d49dd134cdf25d) )
	ROM_LOAD( "epr5881.6f", 0x20000, 0x2000, CRC(dd48dc1f) SHA1(d4560a88d872bd5f401344e3adb25f8486caca11) )
	ROM_LOAD( "epr5886.5f", 0x22000, 0x2000, CRC(a09b27b8) SHA1(8b2d8322b633f6c7174bdb1fff0f6cef2d5a86de) )
	ROM_LOAD( "epr5882.6h", 0x24000, 0x2000, CRC(fb86a168) SHA1(a679c9f50ac952da6c65f6593dce805023b8fc45) )
	ROM_LOAD( "epr5887.5h", 0x26000, 0x2000, CRC(981f03ef) SHA1(42f686b970902bc42ac0f81bd2fc93dbdf766b1a) )
	ROM_LOAD( "epr5883.6j", 0x28000, 0x2000, CRC(e64125ff) SHA1(ae2014d1039f4ed02c55053519bdeddd2f60a77a) )
	ROM_LOAD( "epr5888.5j", 0x2a000, 0x2000, CRC(efc0e15a) SHA1(ba5772830f921004a2d9c90f557c04c799c755b9) )
	ROM_LOAD( "epr5884.6k", 0x2c000, 0x2000, CRC(eb455966) SHA1(14278b598ac1d4007d5357cb40899c92a052417f) )
	ROM_LOAD( "epr5889.5k", 0x2e000, 0x2000, CRC(88357391) SHA1(afdb5ed6555adf60bd64808413fc72fa5c67b6ec) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )
	ROM_LOAD( "epr6144.1f",  0x0000, 0x2000, CRC(b677f818) SHA1(d2f4c913a41ec7a935135fb08587257f9e939436) )
	ROM_LOAD( "epr5892.1e",  0x2000, 0x2000, CRC(608dc2e2) SHA1(d906537cffd3e055f52f37a0490b3bb63107b2f9) )
	ROM_LOAD( "epr5891a.1d", 0x4000, 0x2000, CRC(f353b585) SHA1(f09dcd0240131f872ceef5ddc9c89ab2fc92d117) )
	ROM_LOAD( "epr5890a.1c", 0x6000, 0x2000, CRC(b737c2e8) SHA1(8596abbdff74300230b5ec5bf8acfe222eb3414f) )

	ROM_REGION( 0x0020,  REGION_PROMS, 0 )
	ROM_LOAD( "pr5877.1s", 0x0000, 0x0020, CRC(f880b95d) SHA1(5ad0ee39e2b9befaf3895ec635d5865b7b1e562b) )
ROM_END


ROM_START( crgolfa )
	ROM_REGION( 0x30000, REGION_CPU1, 0 )
	ROM_LOAD( "epr5879b.1c", 0x00000, 0x2000, CRC(927be359) SHA1(d534f7e3ef4ced8eea882ae2b8425df4c5842833) )
	ROM_LOAD( "epr5878.1a",  0x02000, 0x2000, CRC(65fd0fa0) SHA1(de95ff95c9f981cd9eadf8b028ee5373bc69007b) )
	ROM_LOAD( "epr5880.6b",  0x10000, 0x2000, CRC(4d6d8dad) SHA1(1530f81ad0097eadc75884ff8690b60b85ae451b) )
	ROM_LOAD( "epr5885.5e",  0x1e000, 0x2000, CRC(fac6d56c) SHA1(67dc1918d5ab2443e967359e51d49dd134cdf25d) )
	ROM_LOAD( "epr5881.6f",  0x20000, 0x2000, CRC(dd48dc1f) SHA1(d4560a88d872bd5f401344e3adb25f8486caca11) )
	ROM_LOAD( "epr5886.5f",  0x22000, 0x2000, CRC(a09b27b8) SHA1(8b2d8322b633f6c7174bdb1fff0f6cef2d5a86de) )
	ROM_LOAD( "epr5882.6h",  0x24000, 0x2000, CRC(fb86a168) SHA1(a679c9f50ac952da6c65f6593dce805023b8fc45) )
	ROM_LOAD( "epr5887.5h",  0x26000, 0x2000, CRC(981f03ef) SHA1(42f686b970902bc42ac0f81bd2fc93dbdf766b1a) )
	ROM_LOAD( "epr5883.6j",  0x28000, 0x2000, CRC(e64125ff) SHA1(ae2014d1039f4ed02c55053519bdeddd2f60a77a) )
	ROM_LOAD( "epr5888.5j",  0x2a000, 0x2000, CRC(efc0e15a) SHA1(ba5772830f921004a2d9c90f557c04c799c755b9) )
	ROM_LOAD( "epr5884.6k",  0x2c000, 0x2000, CRC(eb455966) SHA1(14278b598ac1d4007d5357cb40899c92a052417f) )
	ROM_LOAD( "epr5889.5k",  0x2e000, 0x2000, CRC(88357391) SHA1(afdb5ed6555adf60bd64808413fc72fa5c67b6ec) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )
	ROM_LOAD( "epr5893c.1f", 0x0000, 0x1000, CRC(5011646d) SHA1(1bbf83107396d69c17580d4b1b38d93f741a608f) )
	ROM_LOAD( "epr5892.1e",  0x2000, 0x2000, CRC(608dc2e2) SHA1(d906537cffd3e055f52f37a0490b3bb63107b2f9) )
	ROM_LOAD( "epr5891a.1d", 0x4000, 0x2000, CRC(f353b585) SHA1(f09dcd0240131f872ceef5ddc9c89ab2fc92d117) )
	ROM_LOAD( "epr5890a.1c", 0x6000, 0x2000, CRC(b737c2e8) SHA1(8596abbdff74300230b5ec5bf8acfe222eb3414f) )

	ROM_REGION( 0x0020,  REGION_PROMS, 0 )
	ROM_LOAD( "pr5877.1s",   0x0000, 0x0020, CRC(f880b95d) SHA1(5ad0ee39e2b9befaf3895ec635d5865b7b1e562b) )
ROM_END


ROM_START( crgolfb )
	ROM_REGION( 0x30000, REGION_CPU1, 0 )
	ROM_LOAD( "epr5879b.1c", 0x00000, 0x2000, CRC(927be359) SHA1(d534f7e3ef4ced8eea882ae2b8425df4c5842833) )
	ROM_LOAD( "epr5878.1a",  0x02000, 0x2000, CRC(65fd0fa0) SHA1(de95ff95c9f981cd9eadf8b028ee5373bc69007b) )
	ROM_LOAD( "cg.1",        0x10000, 0x2000, CRC(ad7d537a) SHA1(deff74074a8b16ea91a0fa72d97ec36336c87b97) )
	ROM_LOAD( "cg.6",        0x1e000, 0x2000, CRC(fac6d56c) SHA1(67dc1918d5ab2443e967359e51d49dd134cdf25d) )
	ROM_LOAD( "cg.2",        0x20000, 0x2000, CRC(dd48dc1f) SHA1(d4560a88d872bd5f401344e3adb25f8486caca11) )
	ROM_LOAD( "cg.7",        0x22000, 0x2000, CRC(a09b27b8) SHA1(8b2d8322b633f6c7174bdb1fff0f6cef2d5a86de) )
	ROM_LOAD( "cg.3",        0x24000, 0x2000, CRC(fb86a168) SHA1(a679c9f50ac952da6c65f6593dce805023b8fc45) )
	ROM_LOAD( "cg.8",        0x26000, 0x2000, CRC(981f03ef) SHA1(42f686b970902bc42ac0f81bd2fc93dbdf766b1a) )
	ROM_LOAD( "cg.4",        0x28000, 0x2000, CRC(e64125ff) SHA1(ae2014d1039f4ed02c55053519bdeddd2f60a77a) )
	ROM_LOAD( "cg.9",        0x2a000, 0x2000, CRC(efc0e15a) SHA1(ba5772830f921004a2d9c90f557c04c799c755b9) )
	ROM_LOAD( "cg.5",        0x2c000, 0x2000, CRC(eb455966) SHA1(14278b598ac1d4007d5357cb40899c92a052417f) )
	ROM_LOAD( "cg.10",       0x2e000, 0x2000, CRC(88357391) SHA1(afdb5ed6555adf60bd64808413fc72fa5c67b6ec) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )
	ROM_LOAD( "cg.14",       0x0000, 0x1000, CRC(07156cd9) SHA1(8907cf9228d6de117b24969d4e039cee330f9b1e) )
	ROM_LOAD( "cg.13",       0x2000, 0x2000, CRC(608dc2e2) SHA1(d906537cffd3e055f52f37a0490b3bb63107b2f9) )
	ROM_LOAD( "cg.12",       0x4000, 0x2000, CRC(f353b585) SHA1(f09dcd0240131f872ceef5ddc9c89ab2fc92d117) )
	ROM_LOAD( "cg.11",       0x6000, 0x2000, CRC(b737c2e8) SHA1(8596abbdff74300230b5ec5bf8acfe222eb3414f) )

	ROM_REGION( 0x0020,  REGION_PROMS, 0 )
	ROM_LOAD( "pr5877.1s",   0x0000, 0x0020, CRC(f880b95d) SHA1(5ad0ee39e2b9befaf3895ec635d5865b7b1e562b) )
ROM_END


ROM_START( crgolfc )
	ROM_REGION( 0x30000, REGION_CPU1, 0 )
	ROM_LOAD( "15.1a",      0x00000, 0x2000, CRC(e6194356) SHA1(78eec53a0658b552e6a8af109d9c9754e4ddadcb) )
	ROM_LOAD( "16.1c",      0x02000, 0x2000, CRC(f50412e2) SHA1(5a50fb1edfc26072e921447bd157fe996f707e05) )
	ROM_LOAD( "cg.1",       0x10000, 0x2000, CRC(ad7d537a) SHA1(deff74074a8b16ea91a0fa72d97ec36336c87b97) ) //  1.6a
	ROM_LOAD( "epr5885.5e", 0x1e000, 0x2000, CRC(fac6d56c) SHA1(67dc1918d5ab2443e967359e51d49dd134cdf25d) ) //  6.5a
	ROM_LOAD( "epr5881.6f", 0x20000, 0x2000, CRC(dd48dc1f) SHA1(d4560a88d872bd5f401344e3adb25f8486caca11) ) //  2.6b
	ROM_LOAD( "epr5886.5f", 0x22000, 0x2000, CRC(a09b27b8) SHA1(8b2d8322b633f6c7174bdb1fff0f6cef2d5a86de) ) //  7.5b
	ROM_LOAD( "3.6c",       0x24000, 0x2000, CRC(b7fcee1a) SHA1(47e9a2cee945c5f59490b73c475ec2512ea0f559) )
	ROM_LOAD( "epr5887.5h", 0x26000, 0x2000, CRC(981f03ef) SHA1(42f686b970902bc42ac0f81bd2fc93dbdf766b1a) ) //  8.5c
	ROM_LOAD( "epr5883.6j", 0x28000, 0x2000, CRC(e64125ff) SHA1(ae2014d1039f4ed02c55053519bdeddd2f60a77a) ) //  4.6d
	ROM_LOAD( "epr5888.5j", 0x2a000, 0x2000, CRC(efc0e15a) SHA1(ba5772830f921004a2d9c90f557c04c799c755b9) ) //  9.5d
	ROM_LOAD( "epr5884.6k", 0x2c000, 0x2000, CRC(eb455966) SHA1(14278b598ac1d4007d5357cb40899c92a052417f) ) //  5.6e
	ROM_LOAD( "epr5889.5k", 0x2e000, 0x2000, CRC(88357391) SHA1(afdb5ed6555adf60bd64808413fc72fa5c67b6ec) ) // 10.5e

	ROM_REGION( 0x10000, REGION_CPU2, 0 )
	ROM_LOAD( "11.1e",       0x0000, 0x1000, CRC(53295a1a) SHA1(ec6c4df9f32e4b3ffe48e823d90a9e6a671e6027) )
	ROM_LOAD( "epr5892.1e",  0x2000, 0x2000, CRC(608dc2e2) SHA1(d906537cffd3e055f52f37a0490b3bb63107b2f9) ) // 12.1d
	ROM_LOAD( "epr5891a.1d", 0x4000, 0x2000, CRC(f353b585) SHA1(f09dcd0240131f872ceef5ddc9c89ab2fc92d117) ) // 13.1c
	ROM_LOAD( "epr5890a.1c", 0x6000, 0x2000, CRC(b737c2e8) SHA1(8596abbdff74300230b5ec5bf8acfe222eb3414f) ) // 14.1b

	ROM_REGION( 0x0020,  REGION_PROMS, 0 )
	ROM_LOAD( "pr5877.1s", 0x0000, 0x0020, CRC(f880b95d) SHA1(5ad0ee39e2b9befaf3895ec635d5865b7b1e562b) )
ROM_END


ROM_START( crgolfhi )
	ROM_REGION( 0x30000, REGION_CPU1, 0 )
	ROM_LOAD( "cpu.c1",  0x00000, 0x2000, CRC(8b101085) SHA1(a59c369be3e7e645d8b20032998a778a2056b7d7) )
	ROM_LOAD( "cpu.a1",  0x02000, 0x2000, CRC(f48a8ee8) SHA1(cc07c7258caf251e9cb52f12be779cb02fca0b0a) )
	ROM_LOAD( "main.b6", 0x10000, 0x2000, CRC(5b0336c6) SHA1(86e2c197f23a2f2f7666448b74611150ca15a2af) )
	ROM_LOAD( "main.b5", 0x12000, 0x2000, CRC(7b80149a) SHA1(c802a79b1430b15d166f5fca11d2ed4e65bc65a9) )
	ROM_LOAD( "main.c6", 0x14000, 0x2000, CRC(7804cb1c) SHA1(487f979f47a0f40fa35331c71a66dc8428387a26) )
	ROM_LOAD( "main.c5", 0x16000, 0x2000, CRC(7721efc5) SHA1(9f3fb6845e5815ada1535da7800e175769fd46b1) )
	ROM_LOAD( "main.d6", 0x18000, 0x2000, CRC(f3ccdfaa) SHA1(c266737caf7222a971d0297b944c5710d3ec12be) )
	ROM_LOAD( "main.d5", 0x1a000, 0x2000, CRC(bef85c95) SHA1(516615975207209a4c649df7ffd451167fc40c45) )
	ROM_LOAD( "main.e6", 0x1c000, 0x2000, CRC(aa75e849) SHA1(226e7712e65f86422a1caebf3b95abcf39af2277) )
	ROM_LOAD( "main.e5", 0x1e000, 0x2000, CRC(e8eefbc4) SHA1(02393d3c0a1234ec51348d755725562cc7861285) )
	ROM_LOAD( "main.f6", 0x20000, 0x2000, CRC(e1130eec) SHA1(26a68f8af543983fcae73db59d075b11ee101ca8) )
	ROM_LOAD( "main.f5", 0x22000, 0x2000, CRC(090c21e3) SHA1(e5e0fc1e4ffd2a9c344cfc70a9e8e7cebb0821cc) )
	ROM_LOAD( "main.h6", 0x24000, 0x2000, CRC(33b8ada4) SHA1(73192108daa0724c30c1deea7d52538a49bfdf8f) )
	ROM_LOAD( "main.h5", 0x26000, 0x2000, CRC(16e5a26c) SHA1(7bb6e5d852f352331953058c17e753fee04d1cf9) )
	ROM_LOAD( "main.j6", 0x28000, 0x2000, CRC(22db8cce) SHA1(cd646830129bfdd2f5f10c8f6732e76f8a15b74f) )
	ROM_LOAD( "main.j5", 0x2a000, 0x2000, CRC(f757de30) SHA1(38330f10051735683f41ed425900b9f0f9ee01be) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )
	ROM_LOAD( "main.f1",  0x0000, 0x2000, CRC(e7c471de) SHA1(b953807bc714496363ca33ad0fc11a2d30aa7b7e) )

	ROM_REGION( 0x8000, REGION_SOUND1, 0 )
	ROM_LOAD( "sub.r1", 0x0000, 0x2000, CRC(9be85e38) SHA1(a108fe812d0518e7bef32fd76998c0c70b70723e) )
	ROM_LOAD( "sub.r2", 0x2000, 0x2000, CRC(d65b8e3a) SHA1(de6acffbe2d7078f0598857a6a3b2179e5c82a34) )
	ROM_LOAD( "sub.r3", 0x4000, 0x2000, CRC(65967250) SHA1(7620560ea57b8e5d259ea8881fb8d8ca46228014) )
	ROM_LOAD( "sub.r4", 0x6000, 0x2000, CRC(d3716776) SHA1(7e38437d255c5f28aac24f0943c10fc1ce998b60) )

	ROM_REGION( 0x0020,  REGION_PROMS, 0 )
	ROM_LOAD( "prom.s1", 0x0000, 0x0020, CRC(014427df) SHA1(85a5e660f9667e032b80152bbde351007e5c88df) )
ROM_END



/*************************************
 *
 *  Game-specific init
 *
 *************************************/

static DRIVER_INIT( crgolfhi )
{
	memory_install_write8_handler(1, ADDRESS_SPACE_PROGRAM, 0xa000, 0xa003, 0, 0, crgolfhi_sample_w);
}



/*************************************
 *
 *  Game driver(s)
 *
 *************************************/

GAME( 1984, crgolf,   0,      crgolf,   crgolf,  0,        ROT0, "Nasco Japan", "Crowns Golf (set 1)" , GAME_SUPPORTS_SAVE ) // 834-5419-03
GAME( 1984, crgolfa,  crgolf, crgolf,   crgolf,  0,        ROT0, "Nasco Japan", "Crowns Golf (set 2)", GAME_SUPPORTS_SAVE )
GAME( 1984, crgolfc,  crgolf, crgolf,   crgolf,  0,        ROT0, "Nasco Japan", "Champion Golf", GAME_SUPPORTS_SAVE )
GAME( 1984, crgolfb,  crgolf, crgolf,   crgolf,  0,        ROT0, "Nasco Japan", "Champion Golf (bootleg Set 1)", GAME_SUPPORTS_SAVE )
GAME( 1985, crgolfhi, 0,      crgolfhi, crgolf,  crgolfhi, ROT0, "Nasco Japan", "Crowns Golf in Hawaii" , GAME_SUPPORTS_SAVE )
