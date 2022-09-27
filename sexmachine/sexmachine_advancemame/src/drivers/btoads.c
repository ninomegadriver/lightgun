/*************************************************************************

    BattleToads

    driver by Aaron Giles

**************************************************************************/

#include "driver.h"
#include "cpu/tms34010/tms34010.h"
#include "vidhrdw/tlc34076.h"
#include "btoads.h"
#include "sound/bsmt2000.h"



/*************************************
 *
 *  Global variables
 *
 *************************************/

static UINT8 main_to_sound_data;
static UINT8 main_to_sound_ready;

static UINT8 sound_to_main_data;
static UINT8 sound_to_main_ready;
static UINT8 sound_int_state;



/*************************************
 *
 *  Machine init
 *
 *************************************/

static MACHINE_RESET( btoads )
{
	tlc34076_reset(6);
}



/*************************************
 *
 *  Main -> sound CPU communication
 *
 *************************************/

static void delayed_sound_w(int param)
{
	main_to_sound_data = param;
	main_to_sound_ready = 1;
	cpu_triggerint(1);

	/* use a timer to make long transfers faster */
	timer_set(TIME_IN_USEC(50), 0, 0);
}


static WRITE16_HANDLER( main_sound_w )
{
	if (ACCESSING_LSB)
		timer_set(TIME_NOW, data & 0xff, delayed_sound_w);
}


static READ16_HANDLER( special_port_4_r )
{
	int result = readinputport(4) & ~0x81;

	if (sound_to_main_ready)
		result |= 0x01;
	if (main_to_sound_ready)
		result |= 0x80;
	return result;
}


static READ16_HANDLER( main_sound_r )
{
	sound_to_main_ready = 0;
	return sound_to_main_data;
}



/*************************************
 *
 *  Sound -> main CPU communication
 *
 *************************************/

static WRITE8_HANDLER( sound_data_w )
{
	sound_to_main_data = data;
	sound_to_main_ready = 1;
}


static READ8_HANDLER( sound_data_r )
{
	main_to_sound_ready = 0;
	return main_to_sound_data;
}


static READ8_HANDLER( sound_ready_to_send_r )
{
	return sound_to_main_ready ? 0x00 : 0x80;
}


static READ8_HANDLER( sound_data_ready_r )
{
	if (activecpu_get_pc() == 0xd50 && !main_to_sound_ready)
		cpu_spinuntil_int();
	return main_to_sound_ready ? 0x00 : 0x80;
}



/*************************************
 *
 *  Sound CPU interrupt generation
 *
 *************************************/

static INTERRUPT_GEN( sound_interrupt )
{
	if (sound_int_state & 0x80)
	{
		cpunum_set_input_line(1, 0, ASSERT_LINE);
		sound_int_state = 0x00;
	}
}


static WRITE8_HANDLER( sound_int_state_w )
{
	if (!(sound_int_state & 0x80) && (data & 0x80))
		cpunum_set_input_line(1, 0, CLEAR_LINE);

	sound_int_state = data;
}



/*************************************
 *
 *  Sound CPU BSMT2000 communication
 *
 *************************************/

static READ8_HANDLER( bsmt_ready_r )
{
	return 0x80;
}


static WRITE8_HANDLER( bsmt2000_port_w )
{
	UINT16 reg = offset >> 8;
	UINT16 val = ((offset & 0xff) << 8) | data;
	BSMT2000_data_0_w(reg, val, 0);
}



/*************************************
 *
 *  Main CPU memory map
 *
 *************************************/

static ADDRESS_MAP_START( main_map, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x00000000, 0x003fffff) AM_RAM
	AM_RANGE(0x20000000, 0x2000007f) AM_READ(input_port_0_word_r)
	AM_RANGE(0x20000080, 0x200000ff) AM_READ(input_port_1_word_r)
	AM_RANGE(0x20000100, 0x2000017f) AM_READ(input_port_2_word_r)
	AM_RANGE(0x20000180, 0x200001ff) AM_READ(input_port_3_word_r)
	AM_RANGE(0x20000200, 0x2000027f) AM_READ(special_port_4_r)
	AM_RANGE(0x20000280, 0x200002ff) AM_READ(input_port_5_word_r)
	AM_RANGE(0x20000000, 0x200000ff) AM_WRITE(MWA16_RAM) AM_BASE(&btoads_sprite_scale)
	AM_RANGE(0x20000100, 0x2000017f) AM_WRITE(MWA16_RAM) AM_BASE(&btoads_sprite_control)
	AM_RANGE(0x20000180, 0x200001ff) AM_WRITE(btoads_display_control_w)
	AM_RANGE(0x20000200, 0x2000027f) AM_WRITE(btoads_scroll0_w)
	AM_RANGE(0x20000280, 0x200002ff) AM_WRITE(btoads_scroll1_w)
	AM_RANGE(0x20000300, 0x2000037f) AM_READWRITE(btoads_paletteram_r, btoads_paletteram_w)
	AM_RANGE(0x20000380, 0x200003ff) AM_READWRITE(main_sound_r, main_sound_w)
	AM_RANGE(0x20000400, 0x2000047f) AM_WRITE(btoads_misc_control_w)
	AM_RANGE(0x40000000, 0x4000000f) AM_WRITE(MWA16_NOP)	/* watchdog? */
	AM_RANGE(0x60000000, 0x6003ffff) AM_RAM AM_BASE(&generic_nvram16) AM_SIZE(&generic_nvram_size)
	AM_RANGE(0xa0000000, 0xa03fffff) AM_READWRITE(btoads_vram_fg_display_r, btoads_vram_fg_display_w) AM_BASE(&btoads_vram_fg0)
	AM_RANGE(0xa4000000, 0xa43fffff) AM_READWRITE(btoads_vram_fg_draw_r, btoads_vram_fg_draw_w) AM_BASE(&btoads_vram_fg1)
	AM_RANGE(0xa8000000, 0xa87fffff) AM_RAM AM_BASE(&btoads_vram_fg_data)
	AM_RANGE(0xa8800000, 0xa8ffffff) AM_WRITE(MWA16_NOP)
	AM_RANGE(0xb0000000, 0xb03fffff) AM_READWRITE(btoads_vram_bg0_r, btoads_vram_bg0_w) AM_BASE(&btoads_vram_bg0)
	AM_RANGE(0xb4000000, 0xb43fffff) AM_READWRITE(btoads_vram_bg1_r, btoads_vram_bg1_w) AM_BASE(&btoads_vram_bg1)
	AM_RANGE(0xc0000000, 0xc00003ff) AM_READWRITE(tms34020_io_register_r, tms34020_io_register_w)
	AM_RANGE(0xfc000000, 0xffffffff) AM_ROM AM_REGION(REGION_USER1, 0)
ADDRESS_MAP_END



/*************************************
 *
 *  Sound CPU memory map
 *
 *************************************/

static ADDRESS_MAP_START( sound_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x7fff) AM_ROM
	AM_RANGE(0x8000, 0xffff) AM_RAM
ADDRESS_MAP_END

static ADDRESS_MAP_START( sound_io_map, ADDRESS_SPACE_IO, 8 )
	AM_RANGE(0x0000, 0x7fff) AM_WRITE(bsmt2000_port_w)
	AM_RANGE(0x8000, 0x8000) AM_READWRITE(sound_data_r, sound_data_w)
	AM_RANGE(0x8002, 0x8002) AM_WRITE(sound_int_state_w)
	AM_RANGE(0x8004, 0x8004) AM_READ(sound_data_ready_r)
	AM_RANGE(0x8005, 0x8005) AM_READ(sound_ready_to_send_r)
	AM_RANGE(0x8006, 0x8006) AM_READ(bsmt_ready_r)
ADDRESS_MAP_END



/*************************************
 *
 *  Input ports
 *
 *************************************/

INPUT_PORTS_START( btoads )
	PORT_START
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_PLAYER(1)
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_PLAYER(1)
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_PLAYER(1)
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_PLAYER(1)
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(1)
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(1)
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_COIN1 ) PORT_IMPULSE(2)
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0xff00, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_PLAYER(2)
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_PLAYER(2)
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_PLAYER(2)
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_PLAYER(2)
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(2)
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(2)
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_COIN2 ) PORT_IMPULSE(2)
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0xff00, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_PLAYER(3)
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_PLAYER(3)
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_PLAYER(3)
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_PLAYER(3)
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(3)
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(3)
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_COIN3 ) PORT_IMPULSE(2)
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_START3 )
	PORT_BIT( 0xff00, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START
	PORT_BIT( 0xffff, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START
	PORT_SERVICE_NO_TOGGLE( 0x0002, IP_ACTIVE_LOW )
	PORT_BIT( 0xfffd, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START
	PORT_DIPNAME( 0x0001, 0x0000, DEF_STR( Demo_Sounds ))
	PORT_DIPSETTING(      0x0001, DEF_STR( Off ))
	PORT_DIPSETTING(      0x0000, DEF_STR( On ))
	PORT_DIPNAME( 0x0002, 0x0000, DEF_STR( Stereo ))
	PORT_DIPSETTING(      0x0002, DEF_STR( Off ))
	PORT_DIPSETTING(      0x0000, DEF_STR( On ))
	PORT_DIPNAME( 0x0004, 0x0000, "Common Coin Mech")
	PORT_DIPSETTING(      0x0004, DEF_STR( Off ))
	PORT_DIPSETTING(      0x0000, DEF_STR( On ))
	PORT_DIPNAME( 0x0008, 0x0008, "Three Players")
	PORT_DIPSETTING(      0x0008, DEF_STR( Off ))
	PORT_DIPSETTING(      0x0000, DEF_STR( On ))
	PORT_DIPNAME( 0x0010, 0x0010, DEF_STR( Free_Play ))
	PORT_DIPSETTING(      0x0010, DEF_STR( Off ))
	PORT_DIPSETTING(      0x0000, DEF_STR( On ))
	PORT_DIPNAME( 0x0020, 0x0020, "Blood Free Mode")
	PORT_DIPSETTING(      0x0020, DEF_STR( Off ))
	PORT_DIPSETTING(      0x0000, DEF_STR( On ))
	PORT_DIPNAME( 0x0040, 0x0040, "Credit Retention")
	PORT_DIPSETTING(      0x0040, DEF_STR( Off ))
	PORT_DIPSETTING(      0x0000, DEF_STR( On ))
	PORT_DIPNAME( 0x0080, 0x0080, DEF_STR( Unknown ))
	PORT_DIPSETTING(      0x0080, DEF_STR( Off ))
	PORT_DIPSETTING(      0x0000, DEF_STR( On ))
	PORT_BIT( 0xff00, IP_ACTIVE_LOW, IPT_UNUSED )
INPUT_PORTS_END



/*************************************
 *
 *  34010 configuration
 *
 *************************************/

static struct tms34010_config tms_config =
{
	0,								/* halt on reset */
	NULL,							/* generate interrupt */
	btoads_to_shiftreg,				/* write to shiftreg function */
	btoads_from_shiftreg,			/* read from shiftreg function */
	0,								/* display address changed */
	0								/* display interrupt callback */
};



/*************************************
 *
 *  Sound definitions
 *
 *************************************/

static struct BSMT2000interface bsmt2000_interface =
{
	12,
	REGION_SOUND1
};



/*************************************
 *
 *  Machine drivers
 *
 *************************************/

static MACHINE_DRIVER_START( btoads )

	MDRV_CPU_ADD(TMS34020, 40000000/TMS34020_CLOCK_DIVIDER)
	MDRV_CPU_CONFIG(tms_config)
	MDRV_CPU_PROGRAM_MAP(main_map,0)

	MDRV_CPU_ADD(Z80, 4000000)
	MDRV_CPU_PROGRAM_MAP(sound_map,0)
	MDRV_CPU_IO_MAP(sound_io_map,0)
	MDRV_CPU_PERIODIC_INT(sound_interrupt,TIME_IN_HZ(183))

	MDRV_MACHINE_RESET(btoads)
	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION((1000000 * (256 - 224)) / (60 * 256))
	MDRV_NVRAM_HANDLER(generic_1fill)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(512,256)
	MDRV_VISIBLE_AREA(0,511, 0,223)
	MDRV_PALETTE_LENGTH(256)

	MDRV_VIDEO_START(btoads)
	MDRV_VIDEO_UPDATE(btoads)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_STEREO("left", "right")

	MDRV_SOUND_ADD(BSMT2000, 24000000)
	MDRV_SOUND_CONFIG(bsmt2000_interface)
	MDRV_SOUND_ROUTE(0, "left", 1.0)
	MDRV_SOUND_ROUTE(1, "right", 1.0)
MACHINE_DRIVER_END



/*************************************
 *
 *  ROM definitions
 *
 *************************************/

ROM_START( btoads )
	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* sound program */
	ROM_LOAD( "btu102.bin", 0x0000, 0x8000, CRC(a90b911a) SHA1(6ec25161e68df1c9870d48cc2b1f85cd1a49aba9) )

	ROM_REGION16_LE( 0x800000, REGION_USER1, 0 )	/* 34020 code */
	ROM_LOAD32_WORD( "btu120.bin",  0x000000, 0x400000, CRC(0dfd1e35) SHA1(733a0a4235bebd598c600f187ed2628f28cf9bd0) )
	ROM_LOAD32_WORD( "btu121.bin",  0x000002, 0x400000, CRC(df7487e1) SHA1(67151b900767bb2653b5261a98c81ff8827222c3) )

	ROM_REGION( 0x200000, REGION_SOUND1, 0 )	/* BSMT data */
	ROM_LOAD( "btu109.bin", 0x00000, 0x200000, CRC(d9612ddb) SHA1(f186dfb013e81abf81ba8ac5dc7eb731c1ad82b6) )
ROM_END



/*************************************
 *
 *  Game drivers
 *
 *************************************/

GAME( 1994, btoads, 0, btoads, btoads, 0,  ROT0, "Rare", "Battle Toads", 0 )
