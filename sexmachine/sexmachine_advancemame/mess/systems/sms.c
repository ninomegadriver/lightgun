/******************************************************************************
 Contributors:

	Marat Fayzullin (MG source)
	Charles Mac Donald
	Mathis Rosenhauer
	Brad Oliver
	Michael Luong

 To do:

 - PSG control for Game Gear (needs custom SN76489 with stereo output for each channel)
 - SIO interface for Game Gear (needs netplay, I guess)
 - SMS lightgun support
 - LCD persistence emulation for GG
 - SMS 3D glass support

 The Game Gear SIO and PSG hardware are not emulated but have some
 placeholders in 'machine/sms.c'

 Changes:
	Apr 02 - Added raster palette effects for SMS & GG (ML)
				 - Added sprite collision (ML)
				 - Added zoomed sprites (ML)
	May 02 - Fixed paging bug (ML)
				 - Fixed sprite and tile priority bug (ML)
				 - Fixed bug #66 (ML)
				 - Fixed bug #78 (ML)
				 - try to implement LCD persistence emulation for GG (ML)
	Jun 10, 02 - Added bios emulation (ML)
	Jun 12, 02 - Added PAL & NTSC systems (ML)
	Jun 25, 02 - Added border emulation (ML)
	Jun 27, 02 - Version bits for Game Gear (bits 6 of port 00) (ML)
	Nov-Dec, 05 - Numerous cleanups, fixes, updates (WP)

 ******************************************************************************/

#include "driver.h"
#include "sound/sn76496.h"
#include "sound/2413intf.h"
#include "vidhrdw/generic.h"
#include "includes/sms.h"
#include "devices/cartslot.h"

static ADDRESS_MAP_START( sms_mem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x03FF) AM_ROMBANK(1)					/* First 0x0400 part always points to first page */
	AM_RANGE(0x0400, 0x3FFF) AM_ROMBANK(2)					/* switchable rom bank */
	AM_RANGE(0x4000, 0x7FFF) AM_ROMBANK(3)					/* switchable rom bank */
	AM_RANGE(0x8000, 0xBFFF) AM_READWRITE(MRA8_BANK4, sms_cartram_w)	/* ROM bank / on-cart RAM */
	AM_RANGE(0xC000, 0xDFFB) AM_MIRROR(0x2000) AM_RAM			/* RAM (mirror at 0xE000) */
	AM_RANGE(0xDFFC, 0xDFFF) AM_RAM						/* RAM "underneath" frame registers */
	AM_RANGE(0xFFFC, 0xFFFF) AM_READWRITE(sms_mapper_r, sms_mapper_w)	/* Bankswitch control */
ADDRESS_MAP_END

static ADDRESS_MAP_START( sms_io, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_FLAGS( AMEF_ABITS(8) )
	ADDRESS_MAP_FLAGS( AMEF_UNMAP(1) )
	AM_RANGE(0x00, 0x00) AM_MIRROR(0x3e) AM_WRITE(sms_bios_w) 
	AM_RANGE(0x01, 0x01) AM_MIRROR(0x3e) AM_WRITE(sms_version_w)
	AM_RANGE(0x40, 0x7F)                 AM_READWRITE(sms_vdp_curline_r, SN76496_0_w)
	AM_RANGE(0x80, 0x80) AM_MIRROR(0x3e) AM_READWRITE(sms_vdp_data_r, sms_vdp_data_w)
	AM_RANGE(0x80, 0x81) AM_MIRROR(0x3e) AM_READWRITE(sms_vdp_ctrl_r, sms_vdp_ctrl_w)
	AM_RANGE(0xC0, 0xC0) AM_MIRROR(0x1e) AM_READ(sms_input_port_0_r)
	AM_RANGE(0xC1, 0xC1) AM_MIRROR(0x1e) AM_READ(sms_version_r)
	AM_RANGE(0xE0, 0xE0) AM_MIRROR(0x0e) AM_READ(sms_input_port_0_r)
	AM_RANGE(0xE1, 0xE1) AM_MIRROR(0x0e) AM_READ(sms_version_r)
	AM_RANGE(0xF0, 0xF0)				 AM_READWRITE(sms_input_port_0_r, sms_YM2413_register_port_0_w)
	AM_RANGE(0xF1, 0xF1)				 AM_READWRITE(sms_version_r, sms_YM2413_data_port_0_w)
	AM_RANGE(0xF2, 0xF2)				 AM_READWRITE(sms_fm_detect_r, sms_fm_detect_w)
	AM_RANGE(0xF3, 0xF3)				 AM_READ(sms_version_r)
	AM_RANGE(0xF4, 0xF4) AM_MIRROR(0x02) AM_READ(sms_input_port_0_r)
	AM_RANGE(0xF5, 0xF5) AM_MIRROR(0x02) AM_READ(sms_version_r)
	AM_RANGE(0xF8, 0xF8) AM_MIRROR(0x06) AM_READ(sms_input_port_0_r)
	AM_RANGE(0xF9, 0xF9) AM_MIRROR(0x06) AM_READ(sms_version_r)
ADDRESS_MAP_END

static ADDRESS_MAP_START( gg_io, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_FLAGS( AMEF_ABITS(8) )
	ADDRESS_MAP_FLAGS( AMEF_UNMAP(1) )
	AM_RANGE(0x00, 0x00)				 AM_READ(gg_input_port_2_r)
	AM_RANGE(0x01, 0x05)				 AM_READWRITE(gg_sio_r, gg_sio_w)
	AM_RANGE(0x06, 0x06)				 AM_READWRITE(gg_psg_r, gg_psg_w)
	AM_RANGE(0x07, 0x07) 				 AM_WRITE(sms_version_w)
	AM_RANGE(0x08, 0x08) AM_MIRROR(0x06) AM_WRITE(sms_bios_w) 
	AM_RANGE(0x09, 0x09) AM_MIRROR(0x06) AM_WRITE(sms_version_w)
	AM_RANGE(0x10, 0x10) AM_MIRROR(0x0e) AM_WRITE(sms_bios_w) 
	AM_RANGE(0x11, 0x11) AM_MIRROR(0x0e) AM_WRITE(sms_version_w)
	AM_RANGE(0x20, 0x20) AM_MIRROR(0x1e) AM_WRITE(sms_bios_w) 
	AM_RANGE(0x21, 0x21) AM_MIRROR(0x1e) AM_WRITE(sms_version_w)
	AM_RANGE(0x40, 0x7F)                 AM_READWRITE(sms_vdp_curline_r, SN76496_0_w)
	AM_RANGE(0x80, 0x80) AM_MIRROR(0x3e) AM_READWRITE(sms_vdp_data_r, sms_vdp_data_w)
	AM_RANGE(0x80, 0x81) AM_MIRROR(0x3e) AM_READWRITE(sms_vdp_ctrl_r, sms_vdp_ctrl_w)
	AM_RANGE(0xC0, 0xC0)				 AM_READ(input_port_0_r)
	AM_RANGE(0xC1, 0xC1)				 AM_READ(input_port_1_r)
	AM_RANGE(0xDC, 0xDC)				 AM_READ(input_port_0_r)
	AM_RANGE(0xDD, 0xDD)				 AM_READ(input_port_1_r)
ADDRESS_MAP_END


INPUT_PORTS_START( sms )

	PORT_START	/* IN0 */
	PORT_BIT ( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP) PORT_PLAYER(1) PORT_8WAY
	PORT_BIT ( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN) PORT_PLAYER(1) PORT_8WAY
	PORT_BIT ( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT) PORT_PLAYER(1) PORT_8WAY
	PORT_BIT ( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT) PORT_PLAYER(1) PORT_8WAY
	PORT_BIT ( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1) PORT_PLAYER(1)
	PORT_BIT ( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2) PORT_PLAYER(1)
	PORT_BIT ( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_UP) PORT_PLAYER(2) PORT_8WAY
	PORT_BIT ( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN) PORT_PLAYER(2) PORT_8WAY

	PORT_START	/* IN1 */
	PORT_BIT ( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT) PORT_PLAYER(2) PORT_8WAY
	PORT_BIT ( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT) PORT_PLAYER(2) PORT_8WAY
	PORT_BIT ( 0x04, IP_ACTIVE_LOW, IPT_BUTTON1) PORT_PLAYER(2)
	PORT_BIT ( 0x08, IP_ACTIVE_LOW, IPT_BUTTON2) PORT_PLAYER(2)
	PORT_BIT ( 0x10, IP_ACTIVE_LOW, IPT_UNUSED ) /* Software Reset bit */
	PORT_BIT ( 0x20, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT ( 0x40, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT ( 0x80, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* IN2 */
	PORT_BIT ( 0x01, IP_ACTIVE_LOW, IPT_UNUSED)
	PORT_BIT ( 0x02, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT ( 0x04, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT ( 0x08, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT ( 0x10, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT ( 0x20, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT ( 0x40, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT ( 0x80, IP_ACTIVE_LOW, IPT_START ) /* Game Gear START */

INPUT_PORTS_END



static MACHINE_DRIVER_START(sms)
	/* basic machine hardware */
	MDRV_CPU_ADD_TAG("main", Z80, 3597545)
	MDRV_CPU_PROGRAM_MAP(sms_mem, 0)
	MDRV_CPU_IO_MAP(sms_io, 0)
	MDRV_CPU_VBLANK_INT(sms, NTSC_Y_PIXELS)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_REAL_60HZ_VBLANK_DURATION)
	MDRV_INTERLEAVE(1)

	MDRV_MACHINE_RESET(sms)
	MDRV_NVRAM_HANDLER(sms)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER | VIDEO_RGB_DIRECT)
	MDRV_SCREEN_SIZE(NTSC_X_PIXELS, NTSC_Y_PIXELS)
	MDRV_VISIBLE_AREA( LBORDER_X_PIXELS, LBORDER_X_PIXELS + 255, TBORDER_Y_PIXELS, TBORDER_Y_PIXELS + 239 )
	MDRV_PALETTE_LENGTH(32)
	MDRV_COLORTABLE_LENGTH(0)
	/*MDRV_PALETTE_INIT(sms)*/

	MDRV_VIDEO_START(sms)
	MDRV_VIDEO_UPDATE(sms)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")
	MDRV_SOUND_ADD(SN76496, 4194304)	/* 4.194304 MHz */
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.00)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START(smspal)
	/* basic machine hardware */
	MDRV_CPU_ADD_TAG("main", Z80, 3597545)
	MDRV_CPU_PROGRAM_MAP(sms_mem, 0)
	MDRV_CPU_IO_MAP(sms_io, 0)
	MDRV_CPU_VBLANK_INT(sms, PAL_Y_PIXELS)

	MDRV_FRAMES_PER_SECOND(50)
	MDRV_VBLANK_DURATION(DEFAULT_REAL_60HZ_VBLANK_DURATION)
	MDRV_INTERLEAVE(1)
        
	MDRV_MACHINE_RESET(sms)
	MDRV_NVRAM_HANDLER(sms)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER | VIDEO_RGB_DIRECT)
	MDRV_SCREEN_SIZE(PAL_X_PIXELS, PAL_Y_PIXELS)
	MDRV_VISIBLE_AREA( LBORDER_X_PIXELS, LBORDER_X_PIXELS + 255, TBORDER_Y_PIXELS, TBORDER_Y_PIXELS + 239 )
	MDRV_PALETTE_LENGTH(32)
	MDRV_COLORTABLE_LENGTH(0)
	/*MDRV_PALETTE_INIT(sms)*/
 
	MDRV_VIDEO_START(sms)
	MDRV_VIDEO_UPDATE(sms)
 
	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")
	MDRV_SOUND_ADD(SN76496, 4194304)	/* 4.194304 MHz */
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.00)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START(smsfm)
	MDRV_IMPORT_FROM(sms)

	MDRV_SOUND_ADD(YM2413, 8000000)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.50)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START(gamegear)
	/* basic machine hardware */
	MDRV_CPU_ADD_TAG("main", Z80, 3597545)
	MDRV_CPU_PROGRAM_MAP(sms_mem, 0)
	MDRV_CPU_IO_MAP(gg_io, 0)
	MDRV_CPU_VBLANK_INT(sms, NTSC_Y_PIXELS)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_REAL_60HZ_VBLANK_DURATION)
	MDRV_INTERLEAVE(1)

	MDRV_MACHINE_RESET(sms)
	MDRV_NVRAM_HANDLER(sms)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER | VIDEO_RGB_DIRECT)
	MDRV_SCREEN_SIZE(NTSC_X_PIXELS, NTSC_Y_PIXELS)
	MDRV_VISIBLE_AREA(6*8, 26*8-1, 3*8, 21*8-1)
	MDRV_PALETTE_LENGTH(32)
	MDRV_COLORTABLE_LENGTH(0)

	MDRV_VIDEO_START(sms)
	MDRV_VIDEO_UPDATE(sms)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")
	MDRV_SOUND_ADD(SN76496, 4194304)	/* 4.194304 MHz */
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.00)
MACHINE_DRIVER_END

/*
  sms (no bios, fake driver)
  smsumd3d (1986)
*/
SYSTEM_BIOS_START( sms )
	SYSTEM_BIOS_ADD( 0, "bios13", "US/European BIOS v1.3 (1986)" ) /* smsu13 */
/*	SYSTEM_BIOS_ADD( 1, "missiled", "US/European BIOS with Missile Defense 3D (1986)" ) smsumd3d */
	SYSTEM_BIOS_ADD( 1, "hangonsh", "US/European BIOS v2.4 with Hang On and Safari Hunt (1988)" ) /* smsuhs24 */
	SYSTEM_BIOS_ADD( 2, "hangon", "US/European BIOS v3.4 with Hang On (1988)" ) /* smseh34 */
	SYSTEM_BIOS_ADD( 3, "alexkidd", "US/European BIOS with Alex Kidd in Miracle World (1990)" ) /* smsuam */
SYSTEM_BIOS_END

SYSTEM_BIOS_START( smsj )
        SYSTEM_BIOS_ADD( 0, "jbios21", "Sega Mark III/Sega Master System/Samsung Gamboy - Japanese SMS BIOS v2.1" ) /* smsj21 (+fm) (1985), smsm3 (1984), smsss (+fm) (198?) */
SYSTEM_BIOS_END

/*
  smsemd3d (1986)
  smspal (no bios, fake driver)
*/
SYSTEM_BIOS_START( smspal )
        SYSTEM_BIOS_ADD( 0, "bios13", "Sega Master System - US/European BIOS v1.3 (1986)" ) /* smse13 */
/*	SYSTEM_BIOS_ADD( 1, "missiled", "US/European BIOS with Missile Defense 3D (1986)" ) smsemd3d */
	SYSTEM_BIOS_ADD( 1, "hangonsh", "Sega Master System Plus - US/European BIOS v2.4 with Hang On and Safari Hunt (1988)" ) /* smsehs24 */
	SYSTEM_BIOS_ADD( 2, "hangon", "Sega Master System - US/European BIOS v3.4 with Hang On (1988)" ) /* smseh34 */
        SYSTEM_BIOS_ADD( 3, "alexkidd", "Sega Master System II - US/European BIOS with Alex Kidd in Miracle World (1990)" ) /* smseam */
        SYSTEM_BIOS_ADD( 4, "sonic", "Sega Master System II/Tec Toy Master System III Compact (Brazil) - European/Brazilian BIOS with Sonic the Hedgehog (1991)" ) /* smsesh, smsbsh */
SYSTEM_BIOS_END

ROM_START(sms)
	ROM_REGION(CPU_ADDRESSABLE_SIZE, REGION_CPU1, 0)
	ROM_REGION(0x40000, REGION_USER1, 0)
	ROMX_LOAD("bios13fx.rom", 0x0000, 0x2000, CRC(0072ED54) SHA1(c315672807d8ddb8d91443729405c766dd95cae7), ROM_BIOS(1))
        ROMX_LOAD("sonbios.rom", 0x0000, 0x40000, CRC(81C3476B) SHA1(6aca0e3dffe461ba1cb11a86cd4caf5b97e1b8df), ROM_BIOS(2))
        ROMX_LOAD("hshbios.rom", 0x0000, 0x20000, CRC(91E93385) SHA1(9e179392cd416af14024d8f31c981d9ee9a64517), ROM_BIOS(3))
        ROMX_LOAD("hangbios.rom", 0x0000, 0x20000, CRC(8EDF7AC6) SHA1(51fd6d7990f62cd9d18c9ecfc62ed7936169107e), ROM_BIOS(4))
	ROMX_LOAD("akbios.rom", 0x0000, 0x20000, CRC(CF4A09EA) SHA1(3af7b66248d34eb26da40c92bf2fa4c73a46a051), ROM_BIOS(5))
ROM_END

ROM_START(smspal)
	ROM_REGION(CPU_ADDRESSABLE_SIZE, REGION_CPU1, 0)
	ROM_REGION(0x40000, REGION_USER1, 0)
	ROMX_LOAD("bios13fx.rom", 0x0000, 0x2000, CRC(0072ED54) SHA1(c315672807d8ddb8d91443729405c766dd95cae7), ROM_BIOS(1))
	ROMX_LOAD("hshbios.rom", 0x0000, 0x20000, CRC(91E93385) SHA1(9e179392cd416af14024d8f31c981d9ee9a64517), ROM_BIOS(2))
	ROMX_LOAD("hangbios.rom", 0x0000, 0x20000, CRC(8EDF7AC6) SHA1(51fd6d7990f62cd9d18c9ecfc62ed7936169107e), ROM_BIOS(3))
	ROMX_LOAD("akbios.rom", 0x0000, 0x20000, CRC(CF4A09EA) SHA1(3af7b66248d34eb26da40c92bf2fa4c73a46a051), ROM_BIOS(4))
	ROMX_LOAD("sonbios.rom", 0x0000, 0x40000, CRC(81C3476B) SHA1(6aca0e3dffe461ba1cb11a86cd4caf5b97e1b8df), ROM_BIOS(5))
ROM_END

ROM_START(smsm3)
	ROM_REGION(CPU_ADDRESSABLE_SIZE, REGION_CPU1, 0)
	ROM_REGION(0x4000, REGION_USER1, 0)
	ROMX_LOAD("jbios21.rom", 0x0000, 0x2000, CRC(48D44A13) SHA1(a8c1b39a2e41137835eda6a5de6d46dd9fadbaf2), ROM_BIOS(1))
ROM_END

#define rom_smsj21 rom_smsm3

SYSTEM_BIOS_START( gamegear )
	SYSTEM_BIOS_ADD( 0, "none", "No BIOS" ) /* gamegear */
	SYSTEM_BIOS_ADD( 1, "majesco", "Majesco BIOS" ) /* gamg */
SYSTEM_BIOS_END

ROM_START(gamegear)
	ROM_REGION(CPU_ADDRESSABLE_SIZE, REGION_CPU1,0)
	ROMX_LOAD("majbios.rom", 0x0000, 0x0400, CRC(0EBEA9D4) SHA1(914aa165e3d879f060be77870d345b60cfeb4ede), ROM_BIOS(2))
ROM_END

#define rom_gamegeaj rom_gamegear

static void sms_cartslot_getinfo(const device_class *devclass, UINT32 state, union devinfo *info)
{
	/* cartslot */
	switch(state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case DEVINFO_INT_COUNT:				info->i = 1; break;
		case DEVINFO_INT_MUST_BE_LOADED:		info->i = 0; break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case DEVINFO_PTR_INIT:				info->init = device_init_sms_cart; break;
		case DEVINFO_PTR_LOAD:				info->load = device_load_sms_cart; break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case DEVINFO_STR_FILE_EXTENSIONS:		strcpy(info->s = device_temp_str(), "sms,bin,ic1"); break;

		default:					cartslot_device_getinfo(devclass, state, info); break;
	}
}

SYSTEM_CONFIG_START(sms)
	CONFIG_DEVICE(sms_cartslot_getinfo)
SYSTEM_CONFIG_END

static void gamegear_cartslot_getinfo(const device_class *devclass, UINT32 state, union devinfo *info)
{
	switch(state)
	{
		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case DEVINFO_INT_COUNT:				info->i = 1; break;
		case DEVINFO_INT_MUST_BE_LOADED:		info->i = 1; break;
		case DEVINFO_STR_FILE_EXTENSIONS:		strcpy(info->s = device_temp_str(), "gg"); break;

		default:					sms_cartslot_getinfo(devclass, state, info); break;
	}
}

SYSTEM_CONFIG_START(gamegear)
	CONFIG_DEVICE(gamegear_cartslot_getinfo)
SYSTEM_CONFIG_END

static void gamegearo_cartslot_getinfo(const device_class *devclass, UINT32 state, union devinfo *info)
{
	switch(state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case DEVINFO_INT_MUST_BE_LOADED:				info->i = 0; break;

		default:										gamegear_cartslot_getinfo(devclass, state, info); break;
	}
}

SYSTEM_CONFIG_START(gamegearo)
	CONFIG_DEVICE(gamegearo_cartslot_getinfo)
SYSTEM_CONFIG_END

/***************************************************************************

  Game driver(s)

***************************************************************************/

/*		YEAR	NAME		PARENT		COMPATIBLE	MACHINE		INPUT	INIT	CONFIG		COMPANY			FULLNAME */
CONSB(	1986,	sms,	0, sms,			0,			sms,		sms,	0,		sms,		"Sega",			"Sega Master System" , FLAG_BIOS_FULL )
CONSB(	1986,	smspal,	sms, smspal,	0,			smspal,		sms,	0,		sms,		"Sega",			"Sega Master System (PAL)" , FLAG_BIOS_FULL )
CONSB(  1984,   smsm3,	0, smsj,              0,                      sms,            sms,    0,              sms,            "Sega",                 "Sega Mark III" , FLAG_REGION_JAPAN | FLAG_BIOS_2000 )
CONSB(	1985,	smsj21,		smsm3, smsj,		0,			smsfm,		sms,	0,		sms,		"Sega",			"Samsung Gamboy" , FLAG_REGION_JAPAN | FLAG_BIOS_2000 | FLAG_FM )

CONSB(	1990,	gamegear,	0, gamegear,			sms,		gamegear,	sms,	0,		gamegear,	"Sega",			"Sega Game Gear (European/American)" , FLAG_GAMEGEAR | FLAG_BIOS_0400 )
CONSB(	1990,	gamegeaj,	gamegear, gamegear,	0,			gamegear,	sms,	0,		gamegear,	"Sega",			"Sega Game Gear (Japanese)" , FLAG_REGION_JAPAN | FLAG_GAMEGEAR | FLAG_BIOS_0400 )

