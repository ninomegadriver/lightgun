/***************************************************************************

  jackal.c

  Written by Kenneth Lin (kenneth_lin@ai.vancouver.bc.ca)

Notes:
- This game uses two 005885 gfx chip in parallel. The unique thing about it is
  that the two 4bpp tilemaps from the two chips are merged to form a single
  8bpp tilemap.
- topgunbl is derived from a completely different version, which supports gun
  turret rotation. The copyright year is also deiffrent, but this doesn't
  necessarily mean anything.

TODO:
- The high score table colors are wrong, are there proms missing?
- Sprite lag
- Coin counters don't work correctly, because the register is overwritten by
  other routines and the coin counter bits rapidly toggle between 0 and 1.

***************************************************************************/

#include "driver.h"
#include "cpu/m6809/m6809.h"
#include "sound/2151intf.h"


extern UINT8 *jackal_videoctrl;

extern MACHINE_RESET( jackal );

extern READ8_HANDLER( jackal_zram_r );
extern READ8_HANDLER( jackal_voram_r );
extern READ8_HANDLER( jackal_spriteram_r );

extern WRITE8_HANDLER( jackal_rambank_w );
extern WRITE8_HANDLER( jackal_zram_w );
extern WRITE8_HANDLER( jackal_voram_w );
extern WRITE8_HANDLER( jackal_spriteram_w );

extern PALETTE_INIT( jackal );
extern VIDEO_START( jackal );
extern VIDEO_UPDATE( jackal );

static int irq_enable;

/* Read/Write Handlers */

static READ8_HANDLER( topgunbl_rotary_r )
{
	return (1 << (readinputport(5 + offset) * 8 / 256)) ^ 0xff;
}

static WRITE8_HANDLER( jackal_flipscreen_w )
{
	irq_enable = data & 0x02;
	flip_screen_set(data & 0x08);
}

/* Memory Maps */

static ADDRESS_MAP_START( master_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x0003) AM_RAM AM_BASE(&jackal_videoctrl)	// scroll + other things
	AM_RANGE(0x0004, 0x0004) AM_WRITE(jackal_flipscreen_w)
	AM_RANGE(0x0010, 0x0010) AM_READ(input_port_0_r)
	AM_RANGE(0x0011, 0x0011) AM_READ(input_port_1_r)
	AM_RANGE(0x0012, 0x0012) AM_READ(input_port_2_r)
	AM_RANGE(0x0013, 0x0013) AM_READ(input_port_3_r)
	AM_RANGE(0x0014, 0x0015) AM_READ(topgunbl_rotary_r)
	AM_RANGE(0x0018, 0x0018) AM_READ(input_port_4_r)
	AM_RANGE(0x0019, 0x0019) AM_WRITE(watchdog_reset_w)
	AM_RANGE(0x001c, 0x001c) AM_WRITE(jackal_rambank_w)
	AM_RANGE(0x0020, 0x005f) AM_READWRITE(jackal_zram_r, jackal_zram_w)				// MAIN   Z RAM,SUB    Z RAM
	AM_RANGE(0x0060, 0x1fff) AM_RAM AM_SHARE(1)										// M COMMON RAM,S COMMON RAM
	AM_RANGE(0x2000, 0x2fff) AM_READWRITE(jackal_voram_r, jackal_voram_w)			// MAIN V O RAM,SUB  V O RAM
	AM_RANGE(0x3000, 0x3fff) AM_READWRITE(jackal_spriteram_r, jackal_spriteram_w)	// MAIN V O RAM,SUB  V O RAM
	AM_RANGE(0x4000, 0xbfff) AM_ROMBANK(1)
	AM_RANGE(0xc000, 0xffff) AM_ROM
ADDRESS_MAP_END

static ADDRESS_MAP_START( slave_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x2000, 0x2000) AM_WRITE(YM2151_register_port_0_w)
	AM_RANGE(0x2001, 0x2001) AM_READWRITE(YM2151_status_port_0_r, YM2151_data_port_0_w)
	AM_RANGE(0x4000, 0x43ff) AM_RAM AM_WRITE(paletteram_xBBBBBGGGGGRRRRR_le_w) AM_BASE(&paletteram)	// COLOR RAM (Self test only check 0x4000-0x423f)
	AM_RANGE(0x6000, 0x605f) AM_RAM																	// SOUND RAM (Self test check 0x6000-605f, 0x7c00-0x7fff)
	AM_RANGE(0x6060, 0x7fff) AM_RAM AM_SHARE(1)
	AM_RANGE(0x8000, 0xffff) AM_ROM
ADDRESS_MAP_END

/* Input Ports */

INPUT_PORTS_START( jackal )
	PORT_START_TAG("DSW1")
	PORT_DIPNAME( 0x0f, 0x0f, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x05, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 3C_2C ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 4C_3C ) )
	PORT_DIPSETTING(    0x0f, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 3C_4C ) )
	PORT_DIPSETTING(    0x07, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(    0x0e, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x06, DEF_STR( 2C_5C ) )
	PORT_DIPSETTING(    0x0d, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x0c, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x0b, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(    0x0a, DEF_STR( 1C_6C ) )
	PORT_DIPSETTING(    0x09, DEF_STR( 1C_7C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Free_Play ) )
	PORT_DIPNAME( 0xf0, 0xf0, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x20, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x50, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x80, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x40, DEF_STR( 3C_2C ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 4C_3C ) )
	PORT_DIPSETTING(    0xf0, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x30, DEF_STR( 3C_4C ) )
	PORT_DIPSETTING(    0x70, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(    0xe0, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x60, DEF_STR( 2C_5C ) )
	PORT_DIPSETTING(    0xd0, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0xc0, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0xb0, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(    0xa0, DEF_STR( 1C_6C ) )
	PORT_DIPSETTING(    0x90, DEF_STR( 1C_7C ) )

	PORT_START_TAG("IN1")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_NAME("P1 Fire") PORT_PLAYER(1)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_NAME("P1 Bomb") PORT_PLAYER(1)
	PORT_BIT( 0xc0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START_TAG("IN2")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_NAME("P2 Fire") PORT_PLAYER(2)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_NAME("P2 Bomb") PORT_PLAYER(2)
	PORT_BIT( 0xc0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START_TAG("IN0")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_START2 )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x00, "Sound Adjustment" )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0x80, 0x00, "Sound Mode" )
	PORT_DIPSETTING(    0x80, DEF_STR( Mono ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Stereo ) )

	PORT_START_TAG("DSW2")
	PORT_DIPNAME( 0x03, 0x02, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x03, "2" )
	PORT_DIPSETTING(    0x02, "3" )
	PORT_DIPSETTING(    0x01, "4" )
	PORT_DIPSETTING(    0x00, "7" )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_DIPNAME( 0x18, 0x18, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x18, "30K 150K" )
	PORT_DIPSETTING(    0x10, "50K 200K" )
	PORT_DIPSETTING(    0x08, "30K" )
	PORT_DIPSETTING(    0x00, "50K" )
	PORT_DIPNAME( 0x60, 0x40, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x60, DEF_STR( Easy ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Normal ) )
	PORT_DIPSETTING(    0x20, "Difficult" )
	PORT_DIPSETTING(    0x00, "Very Difficult" )
	PORT_DIPNAME( 0x80, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
INPUT_PORTS_END

INPUT_PORTS_START( topgunbl )
	PORT_INCLUDE(jackal)

	PORT_MODIFY("IN0")
	PORT_BIT( 0xc0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	// player 1 8-way rotary control - converted in topgunbl_rotary_r()
	PORT_BIT( 0xff, 0x00, IPT_DIAL ) PORT_SENSITIVITY(25) PORT_KEYDELTA(10) PORT_CODE_DEC(KEYCODE_Z) PORT_CODE_INC(KEYCODE_X)

	PORT_START	// player 2 8-way rotary control - converted in topgunbl_rotary_r()
	PORT_BIT( 0xff, 0x00, IPT_DIAL ) PORT_SENSITIVITY(25) PORT_KEYDELTA(10) PORT_CODE_DEC(KEYCODE_N) PORT_CODE_INC(KEYCODE_M) PORT_PLAYER(2)
INPUT_PORTS_END

/* Graphics Layouts */

static const gfx_layout charlayout =
{
	8, 8,
	RGN_FRAC(1,4),
	8,	/* 8 bits per pixel (!) */
	{ 0, 1, 2, 3, RGN_FRAC(1,2)+0, RGN_FRAC(1,2)+1, RGN_FRAC(1,2)+2, RGN_FRAC(1,2)+3 },
	{ 0*4, 1*4, 2*4, 3*4, 4*4, 5*4, 6*4, 7*4 },
	{ 0*32, 1*32, 2*32, 3*32, 4*32, 5*32, 6*32, 7*32 },
	32*8
};

static const gfx_layout spritelayout =
{
	16, 16,
	RGN_FRAC(1,4),
	4,
	{ 0, 1, 2, 3 },
	{ 0*4, 1*4, 2*4, 3*4, 4*4, 5*4, 6*4, 7*4,
			32*8+0*4, 32*8+1*4, 32*8+2*4, 32*8+3*4, 32*8+4*4, 32*8+5*4, 32*8+6*4, 32*8+7*4 },
	{ 0*32, 1*32, 2*32, 3*32, 4*32, 5*32, 6*32, 7*32,
			16*32, 17*32, 18*32, 19*32, 20*32, 21*32, 22*32, 23*32 },
	32*32
};

static const gfx_layout spritelayout8 =
{
	8, 8,
	RGN_FRAC(1,4),
	4,
	{ 0, 1, 2, 3 },
	{ 0*4, 1*4, 2*4, 3*4, 4*4, 5*4, 6*4, 7*4 },
	{ 0*32, 1*32, 2*32, 3*32, 4*32, 5*32, 6*32, 7*32 },
	32*8
};

/* Graphics Decode Information */

static const gfx_decode gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0x00000, &charlayout,               0, 16 },	// colors 256-511 with lookup
	{ REGION_GFX1, 0x20000, &spritelayout,        256*16, 16 },	// colors   0- 15 with lookup
	{ REGION_GFX1, 0x20000, &spritelayout8,       256*16, 16 },	// to handle 8x8 sprites
	{ REGION_GFX1, 0x60000, &spritelayout,  256*16+16*16, 16 },	// colors  16- 31 with lookup
	{ REGION_GFX1, 0x60000, &spritelayout8, 256*16+16*16, 16 },	// to handle 8x8 sprites
	{ -1 }
};

/* Interrupt Generator */

INTERRUPT_GEN( jackal_interrupt )
{
	if (irq_enable)
		cpunum_set_input_line(0, 0, HOLD_LINE);
}

/* Machine Driver */

static MACHINE_DRIVER_START( jackal )
	// basic machine hardware
	MDRV_CPU_ADD(M6809, 2000000)	// ???
	MDRV_CPU_PROGRAM_MAP(master_map, 0)
	MDRV_CPU_VBLANK_INT(jackal_interrupt, 1)

	MDRV_CPU_ADD(M6809, 2000000)	// ???
	MDRV_CPU_PROGRAM_MAP(slave_map, 0)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)
	MDRV_INTERLEAVE(10)	// 10 CPU slices per frame - seems enough to keep the CPUs in sync

	MDRV_MACHINE_RESET(jackal)

	// video hardware
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(32*8, 32*8)
	MDRV_VISIBLE_AREA(1*8, 31*8-1, 2*8, 30*8-1)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(512)
	MDRV_COLORTABLE_LENGTH(256*16+16*16+16*16)

	MDRV_PALETTE_INIT(jackal)
	MDRV_VIDEO_START(jackal)
	MDRV_VIDEO_UPDATE(jackal)

	// sound hardware
	MDRV_SPEAKER_STANDARD_STEREO("left", "right")

	MDRV_SOUND_ADD(YM2151, 3580000)
	MDRV_SOUND_ROUTE(0, "left", 0.50)
	MDRV_SOUND_ROUTE(1, "right", 0.50)
MACHINE_DRIVER_END

/* ROMs */

ROM_START( jackal )
	ROM_REGION( 0x20000, REGION_CPU1, 0 )	/* Banked 64k for 1st CPU */
	ROM_LOAD( "j-v02.rom",    0x04000, 0x8000, CRC(0b7e0584) SHA1(e4019463345a4c020d5a004c9a400aca4bdae07b) )
	ROM_CONTINUE(             0x14000, 0x8000 )
	ROM_LOAD( "j-v03.rom",    0x0c000, 0x4000, CRC(3e0dfb83) SHA1(5ba7073751eee33180e51143b348256597909516) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )     /* 64k for 2nd cpu (Graphics & Sound)*/
	ROM_LOAD( "631t01.bin",   0x8000, 0x8000, CRC(b189af6a) SHA1(f7df996c394fdd6f2ce128a8df38d7838f7ec6d6) )

	ROM_REGION( 0x80000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD16_BYTE( "631t04.bin",   0x00000, 0x20000, CRC(457f42f0) SHA1(08413a13d128875dddcf4f6ad302363096bf1d41) )
	ROM_LOAD16_BYTE( "631t05.bin",   0x00001, 0x20000, CRC(732b3fc1) SHA1(7e89650b9e5e2b7ae82f8c55ac9995740f6fdfe1) )
	ROM_LOAD16_BYTE( "631t06.bin",   0x40000, 0x20000, CRC(2d10e56e) SHA1(447b464ea725fb9ef87da067a41bcf463b427cce) )
	ROM_LOAD16_BYTE( "631t07.bin",   0x40001, 0x20000, CRC(4961c397) SHA1(b430df58fc3bb722d6fb23bed7d04afdb7e5d9c1) )

	ROM_REGION( 0x0200, REGION_PROMS, 0 )	/* color lookup tables */
	ROM_LOAD( "631r08.bpr",   0x0000, 0x0100, CRC(7553a172) SHA1(eadf1b4157f62c3af4602da764268df954aa0018) )
	ROM_LOAD( "631r09.bpr",   0x0100, 0x0100, CRC(a74dd86c) SHA1(571f606f8fc0fd3d98d26761de79ccb4cc9ab044) )
ROM_END

ROM_START( topgunr )
	ROM_REGION( 0x20000, REGION_CPU1, 0 )	/* Banked 64k for 1st CPU */
	ROM_LOAD( "tgnr15d.bin",  0x04000, 0x8000, CRC(f7e28426) SHA1(db2d5f252a574b8aa4d8406a8e93b423fd2a7fef) )
	ROM_CONTINUE(             0x14000, 0x8000 )
	ROM_LOAD( "tgnr16d.bin",  0x0c000, 0x4000, CRC(c086844e) SHA1(4d6f27ac3aabb4b2d673aa619e407e417ad89337) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )     /* 64k for 2nd cpu (Graphics & Sound)*/
	ROM_LOAD( "631t01.bin",   0x8000, 0x8000, CRC(b189af6a) SHA1(f7df996c394fdd6f2ce128a8df38d7838f7ec6d6) )

	ROM_REGION( 0x80000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD16_BYTE( "tgnr7h.bin",   0x00000, 0x20000, CRC(50122a12) SHA1(c9e0132a3a40d9d28685c867c70231947d8a9cb7) )
	ROM_LOAD16_BYTE( "tgnr8h.bin",   0x00001, 0x20000, CRC(6943b1a4) SHA1(40de2b434600ea4c8fb42e6b21be2c3705a55d67) )
	ROM_LOAD16_BYTE( "tgnr12h.bin",  0x40000, 0x20000, CRC(37dbbdb0) SHA1(f94db780d69e7dd40231a75629af79469d957378) )
	ROM_LOAD16_BYTE( "tgnr13h.bin",  0x40001, 0x20000, CRC(22effcc8) SHA1(4d174b0ce64def32050f87343c4b1424e0fef6f7) )

	ROM_REGION( 0x0200, REGION_PROMS, 0 )	/* color lookup tables */
	ROM_LOAD( "631r08.bpr",   0x0000, 0x0100, CRC(7553a172) SHA1(eadf1b4157f62c3af4602da764268df954aa0018) )
	ROM_LOAD( "631r09.bpr",   0x0100, 0x0100, CRC(a74dd86c) SHA1(571f606f8fc0fd3d98d26761de79ccb4cc9ab044) )
ROM_END

ROM_START( jackalj )
	ROM_REGION( 0x20000, REGION_CPU1, 0 )	/* Banked 64k for 1st CPU */
	ROM_LOAD( "631t02.bin",   0x04000, 0x8000, CRC(14db6b1a) SHA1(b469ea50aa94a2bda3bd0442300aa1272e5f30c4) )
	ROM_CONTINUE(             0x14000, 0x8000 )
	ROM_LOAD( "631t03.bin",   0x0c000, 0x4000, CRC(fd5f9624) SHA1(2520c1ff54410ef498ecbf52877f011900baed4c) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )     /* 64k for 2nd cpu (Graphics & Sound)*/
	ROM_LOAD( "631t01.bin",   0x8000, 0x8000, CRC(b189af6a) SHA1(f7df996c394fdd6f2ce128a8df38d7838f7ec6d6) )

	ROM_REGION( 0x80000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD16_BYTE( "631t04.bin",   0x00000, 0x20000, CRC(457f42f0) SHA1(08413a13d128875dddcf4f6ad302363096bf1d41) )
	ROM_LOAD16_BYTE( "631t05.bin",   0x00001, 0x20000, CRC(732b3fc1) SHA1(7e89650b9e5e2b7ae82f8c55ac9995740f6fdfe1) )
	ROM_LOAD16_BYTE( "631t06.bin",   0x40000, 0x20000, CRC(2d10e56e) SHA1(447b464ea725fb9ef87da067a41bcf463b427cce) )
	ROM_LOAD16_BYTE( "631t07.bin",   0x40001, 0x20000, CRC(4961c397) SHA1(b430df58fc3bb722d6fb23bed7d04afdb7e5d9c1) )

	ROM_REGION( 0x0200, REGION_PROMS, 0 )	/* color lookup tables */
	ROM_LOAD( "631r08.bpr",   0x0000, 0x0100, CRC(7553a172) SHA1(eadf1b4157f62c3af4602da764268df954aa0018) )
	ROM_LOAD( "631r09.bpr",   0x0100, 0x0100, CRC(a74dd86c) SHA1(571f606f8fc0fd3d98d26761de79ccb4cc9ab044) )
ROM_END

ROM_START( topgunbl )
	ROM_REGION( 0x20000, REGION_CPU1, 0 )	/* Banked 64k for 1st CPU */
	ROM_LOAD( "t-3.c5",       0x04000, 0x8000, CRC(7826ad38) SHA1(875e87867924905b9b83bc203eb7ffe81cf72233) )
	ROM_LOAD( "t-4.c4",       0x14000, 0x8000, CRC(976c8431) SHA1(c199f57c25380d741aec85b0e0bfb6acf383e6a6) )
	ROM_LOAD( "t-2.c6",       0x0c000, 0x4000, CRC(d53172e5) SHA1(44b7f180c17f9a121a2f06f2d3471920a8989e21) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )     /* 64k for 2nd cpu (Graphics & Sound)*/
	ROM_LOAD( "t-1.c14",      0x8000, 0x8000, CRC(54aa2d29) SHA1(ebc6b3a5db5120cc33d62e3213d0e881f658282d) )

	ROM_REGION( 0x80000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD16_BYTE( "tgnr7h.bin",   0x00000, 0x20000, CRC(50122a12) SHA1(c9e0132a3a40d9d28685c867c70231947d8a9cb7) )
	ROM_LOAD16_BYTE( "tgnr8h.bin",   0x00001, 0x20000, CRC(6943b1a4) SHA1(40de2b434600ea4c8fb42e6b21be2c3705a55d67) )
	ROM_LOAD16_BYTE( "tgnr12h.bin",  0x40000, 0x20000, CRC(37dbbdb0) SHA1(f94db780d69e7dd40231a75629af79469d957378) )
	ROM_LOAD16_BYTE( "tgnr13h.bin",  0x40001, 0x20000, CRC(22effcc8) SHA1(4d174b0ce64def32050f87343c4b1424e0fef6f7) )

#if 0
	same data, different layout (and one bad ROM)
	ROM_LOAD16_WORD_SWAP( "t-17.n12",     0x00000, 0x08000, CRC(e8875110) )
	ROM_LOAD16_WORD_SWAP( "t-18.n13",     0x08000, 0x08000, CRC(cf14471d) )
	ROM_LOAD16_WORD_SWAP( "t-19.n14",     0x10000, 0x08000, CRC(46ee5dd2) )
	ROM_LOAD16_WORD_SWAP( "t-20.n15",     0x18000, 0x08000, CRC(3f472344) )
	ROM_LOAD16_WORD_SWAP( "t-6.n1",       0x20000, 0x08000, CRC(539cc48c) )
	ROM_LOAD16_WORD_SWAP( "t-5.m1",       0x28000, 0x08000, BAD_DUMP CRC(2dd9a5e9) )
	ROM_LOAD16_WORD_SWAP( "t-7.n2",       0x30000, 0x08000, CRC(0ecd31b1) )
	ROM_LOAD16_WORD_SWAP( "t-8.n3",       0x38000, 0x08000, CRC(f946ada7) )
	ROM_LOAD16_WORD_SWAP( "t-13.n8",      0x40000, 0x08000, CRC(5d669abb) )
	ROM_LOAD16_WORD_SWAP( "t-14.n9",      0x48000, 0x08000, CRC(f349369b) )
	ROM_LOAD16_WORD_SWAP( "t-15.n10",     0x50000, 0x08000, CRC(7c5a91dd) )
	ROM_LOAD16_WORD_SWAP( "t-16.n11",     0x58000, 0x08000, CRC(5ec46d8e) )
	ROM_LOAD16_WORD_SWAP( "t-9.n4",       0x60000, 0x08000, CRC(8269caca) )
	ROM_LOAD16_WORD_SWAP( "t-10.n5",      0x68000, 0x08000, CRC(25393e4f) )
	ROM_LOAD16_WORD_SWAP( "t-11.n6",      0x70000, 0x08000, CRC(7895c22d) )
	ROM_LOAD16_WORD_SWAP( "t-12.n7",      0x78000, 0x08000, CRC(15606dfc) )
#endif

	ROM_REGION( 0x0200, REGION_PROMS, 0 )	/* color lookup tables */
	ROM_LOAD( "631r08.bpr",   0x0000, 0x0100, CRC(7553a172) SHA1(eadf1b4157f62c3af4602da764268df954aa0018) )
	ROM_LOAD( "631r09.bpr",   0x0100, 0x0100, CRC(a74dd86c) SHA1(571f606f8fc0fd3d98d26761de79ccb4cc9ab044) )
ROM_END

/* Game Drivers */

GAME( 1986, jackal,   0,      jackal, jackal,   0, ROT90, "Konami", "Jackal (World)", GAME_IMPERFECT_COLORS )
GAME( 1986, topgunr,  jackal, jackal, jackal,   0, ROT90, "Konami", "Top Gunner (US)", GAME_IMPERFECT_COLORS )
GAME( 1986, jackalj,  jackal, jackal, jackal,   0, ROT90, "Konami", "Tokushu Butai Jackal (Japan)", GAME_IMPERFECT_COLORS )
GAME( 1986, topgunbl, jackal, jackal, topgunbl, 0, ROT90, "bootleg", "Top Gunner (bootleg)", GAME_IMPERFECT_COLORS )
