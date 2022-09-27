/*************************************************************

    Namco ND-1 Driver - Mark McDougall
                        R. Belmont

        With contributions from:
            James Jenkins
            Walter Fath

    Currently Supported Games:
        Namco Classics Vol #1
        Namco Classics Vol #2

PCB Layout
----------

8655960101 (8655970101)
|----------------------------------------|
|    LA4705        NC1_MAIN0B.14D  68000 |
|   4558  LC78815  NC1_MAIN1B.13D        |
|J  POT1                                 |
|                      AT28C16           |
|A                                       |
|                                        |
|M                                       |
|                    NC1_CG0.10C    *    |
|M       M9524LT                         |
|               POT2                     |
|A                                       |
|                    YGV608-F            |
|                                        |
|                                        |
|        NC1_VOICE.7B    49.152MHz       |
|N                  25.326MHz            |
|A                                       |
|M       C352                            |
|C                 MACH210         C416  |
|O                                       |
|4                                       |
|8       H8/3002                 62256   |
|                  NC1_SUB.1C    62256   |
|----------------------------------------|
Notes:
      68000 clock  : 12.288MHz (49.152 / 2)
      H8/3002 clock: 16.384MHz (49.152 / 3)
      VSync        : 60Hz

      POT1    : Master volume
      POT2    : Brightness adjustment (video level)
      M9524LT : ? possibly some sort of RGB video output chip
      AT28C16 : 2K x8 EEPROM
      YGV608-F: Yamaha YVG608-F video controller
      C352    : Namco custom QFP100
      C416    : Namco custom QFP176
      H8/3002 : Hitachi H8/3002 HD6413002F16 QFP100 microcontroller (H8/3002 has no internal ROM capability)
      MACH210 : PLCC44 CPLD, Namco KEYCUS, stamped 'KC001'
      62256   : 32K x8 SOJ28 SRAM
      *       : Unpopulated position for SOP44 Mask ROM 'CG1'

      NC1_MAIN0B.14D: 512K x16 EPROM type 27C240
      NC1_MAIN1B.13D: 512K x16 EPROM type 27C240
      NC1_SUB.1C    : 512K x16 EPROM type 27C240
      NC1_CG0.10C   : 16MBit SOP44 Mask ROM
      NC1_VOICE.7B  : 16MBit SOP44 Mask ROM

 *************************************************************/

#include "driver.h"
#include "vidhrdw/ygv608.h"
#include "cpu/h83002/h83002.h"
#include "namcond1.h"
#include "sound/c352.h"
#include "machine/at28c16.h"

/*************************************************************/

static ADDRESS_MAP_START( namcond1_map, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x0fffff) AM_ROM
	AM_RANGE(0x400000, 0x40ffff) AM_READWRITE(namcond1_shared_ram_r,namcond1_shared_ram_w) AM_BASE(&namcond1_shared_ram)
	AM_RANGE(0x800000, 0x80000f) AM_READWRITE(ygv608_r,ygv608_w)
	AM_RANGE(0xa00000, 0xa00fff) AM_READWRITE(at28c16_16msb_0_r,at28c16_16msb_0_w)
#ifdef MAME_DEBUG
	AM_RANGE(0xb00000, 0xb00001) AM_READ(debug_trigger)
#endif
	AM_RANGE(0xc3ff00, 0xc3ffff) AM_READWRITE(namcond1_cuskey_r,namcond1_cuskey_w)
ADDRESS_MAP_END

/*************************************************************/

INPUT_PORTS_START( namcond1 )
	PORT_START      /* player 1 */
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN )
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_UP )
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_PLAYER(2)
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_PLAYER(2)
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_PLAYER(2)
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_PLAYER(2)
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(2)
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(2)
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_START2 )

	PORT_START  	/* dipswitches */
	PORT_DIPNAME( 0x0100, 0x0100, "Freeze" )
	PORT_DIPSETTING(    0x0100, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0200, 0x0200, DEF_STR( Test ) )
	PORT_DIPSETTING(    0x0200, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x0000, DEF_STR( On ) )
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_SERVICE ) PORT_NAME(DEF_STR( Test )) PORT_CODE(KEYCODE_F2) PORT_TOGGLE
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x00ff, IP_ACTIVE_LOW, IPT_UNUSED )
INPUT_PORTS_END


/* text-layer characters */

static const UINT32 pts_4bits_layout_xoffset[64] =
{
	STEP8( 0*256, 4 ), STEP8( 1*256, 4 ), STEP8( 4*256, 4 ), STEP8( 5*256, 4 ),
	STEP8( 16*256, 4 ), STEP8( 17*256, 4 ), STEP8( 20*256, 4 ), STEP8( 21*256, 4 )
};

static const UINT32 pts_4bits_layout_yoffset[64] =
{
	STEP8( 0*256, 8*4 ), STEP8( 2*256, 8*4 ), STEP8( 8*256, 8*4 ), STEP8( 10*256, 8*4 ),
	STEP8( 32*256, 8*4 ), STEP8( 34*256, 8*4 ), STEP8( 40*256, 8*4 ), STEP8( 42*256, 8*4 )
};

static const gfx_layout pts_8x8_4bits_layout =
{
	8,8,	      /* 8*8 pixels */
	RGN_FRAC(1,1),        /* 65536 patterns */
	4,	          /* 4 bits per pixel */
	{ 0, 1, 2, 3 },
	EXTENDED_XOFFS,
	EXTENDED_YOFFS,
	8*8*4,
	pts_4bits_layout_xoffset,
	pts_4bits_layout_yoffset
};

static const gfx_layout pts_16x16_4bits_layout =
{
	16,16,        /* 16*16 pixels */
	RGN_FRAC(1,1),        /* 16384 patterns */
	4,	          /* 4 bits per pixel */
	{ 0, 1, 2, 3 },
	EXTENDED_XOFFS,
	EXTENDED_YOFFS,
	16*16*4,
	pts_4bits_layout_xoffset,
	pts_4bits_layout_yoffset
};

static const gfx_layout pts_32x32_4bits_layout =
{
	32,32,        /* 32*32 pixels */
	RGN_FRAC(1,1),         /* 4096 patterns */
	4,	          /* 4 bits per pixel */
	{ 0, 1, 2, 3 },
	EXTENDED_XOFFS,
	EXTENDED_YOFFS,
	32*32*4,
	pts_4bits_layout_xoffset,
	pts_4bits_layout_yoffset
};

static const gfx_layout pts_64x64_4bits_layout =
{
	64,64,        /* 32*32 pixels */
	RGN_FRAC(1,1),         /* 1024 patterns */
	4,	          /* 4 bits per pixel */
	{ 0, 1, 2, 3 },
	EXTENDED_XOFFS,
	EXTENDED_YOFFS,
	64*64*4,
	pts_4bits_layout_xoffset,
	pts_4bits_layout_yoffset
};


static const gfx_layout pts_8x8_8bits_layout =
{
	8,8,	      /* 8*8 pixels */
	RGN_FRAC(1,1),        /* 32768 patterns */
	8,	          /* 8 bits per pixel */
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ STEP8( 0*512, 8 ) },
	{ STEP8( 0*512, 8*8 ) },
	8*8*8
};

static const gfx_layout pts_16x16_8bits_layout =
{
	16,16,        /* 16*16 pixels */
	RGN_FRAC(1,1),         /* 8192 patterns */
	8,	          /* 8 bits per pixel */
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ STEP8( 0*512, 8 ), STEP8( 1*512, 8 ) },
	{ STEP8( 0*512, 8*8 ), STEP8( 2*512, 8*8 ) },
	16*16*8
};

static const gfx_decode gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0x00000000, &pts_8x8_4bits_layout,    0,  16 },
	{ REGION_GFX1, 0x00000000, &pts_16x16_4bits_layout,  0,  16 },
	{ REGION_GFX1, 0x00000000, &pts_32x32_4bits_layout,  0,  16 },
	{ REGION_GFX1, 0x00000000, &pts_64x64_4bits_layout,  0,  16 },
	{ REGION_GFX1, 0x00000000, &pts_8x8_8bits_layout,    0, 256 },
	{ REGION_GFX1, 0x00000000, &pts_16x16_8bits_layout,  0, 256 },
	{ -1 }
};

static WRITE16_HANDLER( sharedram_sub_w )
{
	COMBINE_DATA(&namcond1_shared_ram[offset]);
}

static READ16_HANDLER( sharedram_sub_r )
{
	return namcond1_shared_ram[offset];
}

static int p8;

static READ8_HANDLER( mcu_p7_read )
{
	return 0xff;
}

static READ8_HANDLER( mcu_pa_read )
{
	return 0xff;
}

static WRITE8_HANDLER( mcu_pa_write )
{
	p8 = data;
}

/* H8/3002 MCU stuff */
static ADDRESS_MAP_START( nd1h8rwmap, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x07ffff) AM_READ(MRA16_ROM)
	AM_RANGE(0x200000, 0x20ffff) AM_READWRITE( sharedram_sub_r, sharedram_sub_w )
	AM_RANGE(0xa00000, 0xa07fff) AM_READWRITE( c352_0_r, c352_0_w )
	AM_RANGE(0xc00000, 0xc00001) AM_READ( input_port_1_word_r )
	AM_RANGE(0xc00002, 0xc00003) AM_READ( input_port_0_word_r )
	AM_RANGE(0xc00010, 0xc00011) AM_NOP
	AM_RANGE(0xc00030, 0xc00031) AM_NOP
	AM_RANGE(0xc00040, 0xc00041) AM_NOP
ADDRESS_MAP_END

static ADDRESS_MAP_START( nd1h8iomap, ADDRESS_SPACE_IO, 8 )
	AM_RANGE(H8_PORT7, H8_PORT7) AM_READ( mcu_p7_read )
	AM_RANGE(H8_PORTA, H8_PORTA) AM_READWRITE( mcu_pa_read, mcu_pa_write )
	AM_RANGE(H8_ADC_0_L, H8_ADC_3_H) AM_NOP // MCU reads these, but the games have no analog controls
ADDRESS_MAP_END

static struct C352interface c352_interface =
{
	REGION_SOUND1
};

static INTERRUPT_GEN( mcu_interrupt )
{
    if( namcond1_h8_irq5_enabled )
    {
    	cpunum_set_input_line(1, H8_IRQ5, PULSE_LINE);
    }
}

/******************************************
  ND-1 Master clock = 49.152MHz
  - 680000  = 12288000 (CLK/4)
  - H8/3002 = 16666667 (CLK/3) ??? huh?
  - H8/3002 = 16384000 (CLK/3)
  - The level 1 interrupt to the 68k has been measured at 60Hz.
*******************************************/

static MACHINE_DRIVER_START( namcond1 )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000, 12288000)
	MDRV_CPU_PROGRAM_MAP(namcond1_map,0)
	MDRV_CPU_VBLANK_INT(irq1_line_hold, 1)
	MDRV_CPU_PERIODIC_INT(ygv608_timed_interrupt, TIME_IN_HZ(1000))

	MDRV_CPU_ADD(H83002, 16384000 )
	MDRV_CPU_PROGRAM_MAP( nd1h8rwmap, 0 )
	MDRV_CPU_IO_MAP( nd1h8iomap, 0 )
	MDRV_CPU_VBLANK_INT( mcu_interrupt, 1 );

	MDRV_FRAMES_PER_SECOND(60.0)
	MDRV_VBLANK_DURATION(DEFAULT_REAL_60HZ_VBLANK_DURATION)
	MDRV_INTERLEAVE(100)

	MDRV_MACHINE_START(namcond1)
	MDRV_MACHINE_RESET(namcond1)
	MDRV_NVRAM_HANDLER(at28c16_0)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER | VIDEO_NEEDS_6BITS_PER_GUN)
	MDRV_SCREEN_SIZE(288, 224)   // maximum display resolution (512x512 in theory)
	MDRV_VISIBLE_AREA(0, 287, 0, 223)   // default visible area
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(256)

	MDRV_VIDEO_START(ygv608)
	MDRV_VIDEO_UPDATE(ygv608)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_STEREO("left", "right")

	MDRV_SOUND_ADD(C352, 16384000)
	MDRV_SOUND_CONFIG(c352_interface)
	MDRV_SOUND_ROUTE(0, "right", 1.00)
	MDRV_SOUND_ROUTE(1, "left", 1.00)
	MDRV_SOUND_ROUTE(2, "right", 1.00)
	MDRV_SOUND_ROUTE(3, "left", 1.00)
MACHINE_DRIVER_END

ROM_START( ncv1 )
	ROM_REGION( 0x100000,REGION_CPU1, 0 )		/* 16MB for Main CPU */
	ROM_LOAD16_WORD( "n1main0b.14d", 0x00000, 0x80000, CRC(4ffc530b) SHA1(23d622d0261a3584236a77b2cefa522a0f46490e) )
	ROM_LOAD16_WORD( "n1main1b.13d", 0x80000, 0x80000, CRC(26499a4e) SHA1(4af0c365713b4a51da684a3423b07cbb70d9599b) )

	ROM_REGION( 0x80000,REGION_CPU2, 0 )		/* sub CPU */
	ROM_LOAD( "nc1sub.1c",          0x00000, 0x80000, CRC(48ea0de2) SHA1(33e57c8d084a960ccbda462d18e355de44ec7ad9) )

	ROM_REGION( 0x200000,REGION_GFX1, ROMREGION_DISPOSE )	/* 2MB character generator */
	ROM_LOAD( "nc1cg0.10c",         0x000000, 0x200000, CRC(355e7f29) SHA1(47d92c4e28c3610a620d3c9b3be558199477f6d8) )

	ROM_REGION( 0x200000,REGION_SOUND1, 0 ) 	/* 2MB sound data */
    ROM_LOAD( "nc1voice.7b",     0x000000, 0x200000, CRC(91c85bd6) SHA1(c2af8b1518b2b601f2b14c3f327e7e3eae9e29fc) )
ROM_END

ROM_START( ncv1j )
	ROM_REGION( 0x100000,REGION_CPU1, 0 )		/* 16MB for Main CPU */
	ROM_LOAD16_WORD( "n1main0j.14d",  0x00000, 0x80000, CRC(48ce0b2b) SHA1(07dfca8ba935ee0151211f9eb4d453f2da1d4bd7) )
	ROM_LOAD16_WORD( "n1main1j.13d",  0x80000, 0x80000, CRC(49f99235) SHA1(97afde7f7dddd8538de78a74325d0038cb1217f7) )

	ROM_REGION( 0x80000,REGION_CPU2, 0 )		/* sub CPU */
	ROM_LOAD( "nc1sub.1c",          0x00000, 0x80000, CRC(48ea0de2) SHA1(33e57c8d084a960ccbda462d18e355de44ec7ad9) )

	ROM_REGION( 0x200000,REGION_GFX1, ROMREGION_DISPOSE )	/* 2MB character generator */
	ROM_LOAD( "nc1cg0.10c",         0x000000, 0x200000, CRC(355e7f29) SHA1(47d92c4e28c3610a620d3c9b3be558199477f6d8) )

	ROM_REGION( 0x200000,REGION_SOUND1, 0 ) 	/* 2MB sound data */
    ROM_LOAD( "nc1voice.7b",     0x000000, 0x200000, CRC(91c85bd6) SHA1(c2af8b1518b2b601f2b14c3f327e7e3eae9e29fc) )
ROM_END

ROM_START( ncv1j2 )
	ROM_REGION( 0x100000,REGION_CPU1, 0 )		/* 16MB for Main CPU */
	ROM_LOAD16_WORD( "1main0ja.14d", 0x00000, 0x80000, CRC(7207469d) SHA1(73faf1973a57c1bc2163e9ee3fe2febd3b8763a4) )
	ROM_LOAD16_WORD( "1main1ja.13d", 0x80000, 0x80000, CRC(52401b17) SHA1(60c9f20831d0101c02dafbc0bd15422f71f3ad81) )

	ROM_REGION( 0x80000,REGION_CPU2, 0 )		/* sub CPU */
	ROM_LOAD( "nc1sub.1c",          0x00000, 0x80000, CRC(48ea0de2) SHA1(33e57c8d084a960ccbda462d18e355de44ec7ad9) )

	ROM_REGION( 0x200000,REGION_GFX1, ROMREGION_DISPOSE )	/* 2MB character generator */
	ROM_LOAD( "nc1cg0.10c",         0x000000, 0x200000, CRC(355e7f29) SHA1(47d92c4e28c3610a620d3c9b3be558199477f6d8) )

	ROM_REGION( 0x200000,REGION_SOUND1, 0 ) 	/* 2MB sound data */
    ROM_LOAD( "nc1voice.7b",     0x000000, 0x200000, CRC(91c85bd6) SHA1(c2af8b1518b2b601f2b14c3f327e7e3eae9e29fc) )
ROM_END

ROM_START( ncv2 )
	ROM_REGION( 0x100000,REGION_CPU1, 0 )		/* 16MB for Main CPU */
	ROM_LOAD16_WORD( "ncs1mn0.14e", 0x00000, 0x80000, CRC(fb8a4123) SHA1(47acdfe9b5441d0e3649aaa9780e676f760c4e42) )
	ROM_LOAD16_WORD( "ncs1mn1.13e", 0x80000, 0x80000, CRC(7a5ef23b) SHA1(0408742424a6abad512b5baff63409fe44353e10) )

	ROM_REGION( 0x80000,REGION_CPU2, 0 )		/* sub CPU */
	ROM_LOAD( "ncs1sub.1d",          0x00000, 0x80000, CRC(365cadbf) SHA1(7263220e1630239e3e88b828c00389d02628bd7d) )

	ROM_REGION( 0x400000,REGION_GFX1, ROMREGION_DISPOSE )	/* 4MB character generator */
	ROM_LOAD( "ncs1cg0.10e",         0x000000, 0x200000, CRC(fdd24dbe) SHA1(4dceaae3d853075f58a7408be879afc91d80292e) )
	ROM_LOAD( "ncs1cg1.10e",         0x200000, 0x200000, CRC(007b19de) SHA1(d3c093543511ec1dd2f8be6db45f33820123cabc) )

	ROM_REGION( 0x200000,REGION_SOUND1, 0 ) 	/* 2MB sound data */
    ROM_LOAD( "ncs1voic.7c",     0x000000, 0x200000, CRC(ed05fd88) SHA1(ad88632c89a9946708fc6b4c9247e1bae9b2944b) )
ROM_END

ROM_START( ncv2j )
	ROM_REGION( 0x100000,REGION_CPU1, 0 )		/* 16MB for Main CPU */
	ROM_LOAD16_WORD( "ncs1mn0j.14e", 0x00000, 0x80000, CRC(99991192) SHA1(e0b0e15ae23560b77119b3d3e4b2d2bb9d8b36c9) )
	ROM_LOAD16_WORD( "ncs1mn1j.13e", 0x80000, 0x80000, CRC(af4ba4f6) SHA1(ff5adfdd462cfd3f17fbe2401dfc88ff8c71b6f8) )

	ROM_REGION( 0x80000,REGION_CPU2, 0 )		/* sub CPU */
	ROM_LOAD("ncs1sub.1d",          0x00000, 0x80000, CRC(365cadbf) SHA1(7263220e1630239e3e88b828c00389d02628bd7d) )

	ROM_REGION( 0x400000,REGION_GFX1, ROMREGION_DISPOSE )	/* 4MB character generator */
	ROM_LOAD( "ncs1cg0.10e",         0x000000, 0x200000, CRC(fdd24dbe) SHA1(4dceaae3d853075f58a7408be879afc91d80292e) )
	ROM_LOAD( "ncs1cg1.10e",         0x200000, 0x200000, CRC(007b19de) SHA1(d3c093543511ec1dd2f8be6db45f33820123cabc) )

	ROM_REGION( 0x200000,REGION_SOUND1, 0 ) 	/* 2MB sound data */
    ROM_LOAD( "ncs1voic.7c",     0x000000, 0x200000, CRC(ed05fd88) SHA1(ad88632c89a9946708fc6b4c9247e1bae9b2944b) )
ROM_END

GAME( 1995, ncv1,      0, namcond1, namcond1, 0, ROT90, "Namco", "Namco Classics Collection Vol.1", GAME_IMPERFECT_SOUND | GAME_IMPERFECT_GRAPHICS | GAME_SUPPORTS_SAVE )
GAME( 1995, ncv1j,  ncv1, namcond1, namcond1, 0, ROT90, "Namco", "Namco Classics Collection Vol.1 (Japan, v1.00)", GAME_IMPERFECT_SOUND | GAME_IMPERFECT_GRAPHICS | GAME_SUPPORTS_SAVE )
GAME( 1995, ncv1j2, ncv1, namcond1, namcond1, 0, ROT90, "Namco", "Namco Classics Collection Vol.1 (Japan, v1.03)", GAME_IMPERFECT_SOUND | GAME_IMPERFECT_GRAPHICS | GAME_SUPPORTS_SAVE )
GAME( 1996, ncv2,      0, namcond1, namcond1, 0, ROT90, "Namco", "Namco Classics Collection Vol.2", GAME_IMPERFECT_SOUND | GAME_IMPERFECT_GRAPHICS | GAME_UNEMULATED_PROTECTION | GAME_SUPPORTS_SAVE )
GAME( 1996, ncv2j,  ncv2, namcond1, namcond1, 0, ROT90, "Namco", "Namco Classics Collection Vol.2 (Japan)", GAME_IMPERFECT_SOUND | GAME_IMPERFECT_GRAPHICS | GAME_UNEMULATED_PROTECTION | GAME_SUPPORTS_SAVE )
