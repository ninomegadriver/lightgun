/***************************************************************************

Blue Print memory map (preliminary)

driver by Nicola Salmoria


CPU #1
0000-4fff ROM
8000-87ff RAM
9000-93ff Video RAM
b000-b0ff Sprite RAM
f000-f3ff Color RAM

read:
c000      IN0
c001      IN1
c003      read dip switches from the second CPU

e000      Watchdog reset

write:
c000      bit 0,1 = coin counters
d000      command for the second CPU
e000      bit 1 = flip screen

CPU #2
0000-0fff ROM
2000-2fff ROM
4000-43ff RAM

read:
6002      8910 #0 read
8002      8910 #1 read

write:
6000      8910 #0 control
6001      8910 #0 write
8000      8910 #1 control
8001      8910 #1 write

***************************************************************************/

/*

    TODO:

    - the cross hatch pattern test in service mode is too fast, each phase
      should take 15 seconds according to the service manual
    - the two AY-3-8910's should be clocked independently at 2H and 4H

*/

#include "driver.h"
#include "cpu/z80/z80.h"
#include "sound/ay8910.h"

extern UINT8 *blueprnt_scrollram;

extern WRITE8_HANDLER( blueprnt_videoram_w );
extern WRITE8_HANDLER( blueprnt_colorram_w );
extern WRITE8_HANDLER( blueprnt_flipscreen_w );

extern PALETTE_INIT( blueprnt );
extern VIDEO_START( blueprnt );
extern VIDEO_UPDATE( blueprnt );

/* Read/Write Handlers */

static int dipsw;

static WRITE8_HANDLER( dipsw_w )
{
	dipsw = data;
}

static READ8_HANDLER( blueprnt_sh_dipsw_r )
{
	return dipsw;
}

static WRITE8_HANDLER( blueprnt_sound_command_w )
{
	soundlatch_w(offset, data);
	cpunum_set_input_line(1, INPUT_LINE_NMI, PULSE_LINE);
}

static WRITE8_HANDLER( blueprnt_coin_counter_w )
{
	coin_counter_w(0, data & 0x01);
	coin_counter_w(1, data & 0x02);
}

/* Memory Maps */

static ADDRESS_MAP_START( blueprnt_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x7fff) AM_ROM // service mode checks for 8 chips = 64K
	AM_RANGE(0x8000, 0x87ff) AM_RAM
	AM_RANGE(0x9000, 0x93ff) AM_RAM AM_MIRROR(0x400) AM_WRITE(blueprnt_videoram_w) AM_BASE(&videoram)
	AM_RANGE(0xa000, 0xa0ff) AM_RAM AM_BASE(&blueprnt_scrollram)
	AM_RANGE(0xb000, 0xb0ff) AM_RAM AM_BASE(&spriteram) AM_SIZE(&spriteram_size)
	AM_RANGE(0xc000, 0xc000) AM_READWRITE(input_port_0_r, blueprnt_coin_counter_w)
	AM_RANGE(0xc001, 0xc001) AM_READ(input_port_1_r)
	AM_RANGE(0xc003, 0xc003) AM_READ(blueprnt_sh_dipsw_r)
	AM_RANGE(0xd000, 0xd000) AM_WRITE(blueprnt_sound_command_w)
	AM_RANGE(0xe000, 0xe000) AM_READWRITE(watchdog_reset_r, blueprnt_flipscreen_w)
	AM_RANGE(0xf000, 0xf3ff) AM_RAM AM_WRITE(blueprnt_colorram_w) AM_BASE(&colorram)
ADDRESS_MAP_END

static ADDRESS_MAP_START( sound_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x0fff) AM_ROM
	AM_RANGE(0x2000, 0x2fff) AM_ROM
	AM_RANGE(0x4000, 0x43ff) AM_RAM
	AM_RANGE(0x6000, 0x6000) AM_WRITE(AY8910_control_port_0_w)
	AM_RANGE(0x6001, 0x6001) AM_WRITE(AY8910_write_port_0_w)
	AM_RANGE(0x6002, 0x6002) AM_READ(AY8910_read_port_0_r)
	AM_RANGE(0x8000, 0x8000) AM_WRITE(AY8910_control_port_1_w)
	AM_RANGE(0x8001, 0x8001) AM_WRITE(AY8910_write_port_1_w)
	AM_RANGE(0x8002, 0x8002) AM_READ(AY8910_read_port_1_r)
ADDRESS_MAP_END

/* Input Ports */

INPUT_PORTS_START( blueprnt )
	PORT_START_TAG("PORT 0")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_TILT )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_BUTTON1 )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT  ) PORT_8WAY
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT ) PORT_8WAY
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP    ) PORT_8WAY
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN  ) PORT_8WAY

	PORT_START_TAG("PORT 1")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_COIN2 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_SERVICE( 0x04, IP_ACTIVE_HIGH )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_BUTTON1 ) PORT_COCKTAIL
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT  ) PORT_8WAY PORT_COCKTAIL
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT ) PORT_8WAY PORT_COCKTAIL
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP    ) PORT_8WAY PORT_COCKTAIL
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN  ) PORT_8WAY PORT_COCKTAIL

	PORT_START_TAG("DILSW1")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_DIPNAME( 0x06, 0x02, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x00, "20K" )
	PORT_DIPSETTING(    0x02, "30K" )
	PORT_DIPSETTING(    0x04, "40K" )
	PORT_DIPSETTING(    0x06, "50K" )
	PORT_DIPNAME( 0x08, 0x00, DEF_STR( Free_Play ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x08, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x00, "Maze Monster Appears In" )
	PORT_DIPSETTING(    0x00, "2nd Maze" )
	PORT_DIPSETTING(    0x10, "3rd Maze" )
	PORT_DIPNAME( 0x20, 0x00, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x20, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_1C ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x40, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_5C ) )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START_TAG("DILSW2")
	PORT_DIPNAME( 0x03, 0x01, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x00, "2" )
	PORT_DIPSETTING(    0x01, "3" )
	PORT_DIPSETTING(    0x02, "4" )
	PORT_DIPSETTING(    0x03, "5" )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_DIPNAME( 0x08, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0x30, 0x10, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x00, "Level 1" )
	PORT_DIPSETTING(    0x10, "Level 2" )
	PORT_DIPSETTING(    0x20, "Level 3" )
	PORT_DIPSETTING(    0x30, "Level 4" )
	PORT_BIT( 0xc0, IP_ACTIVE_HIGH, IPT_UNUSED )
INPUT_PORTS_END

INPUT_PORTS_START( saturn )
	PORT_START_TAG("IN0")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_BUTTON2 )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_BUTTON1 )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT  ) PORT_8WAY
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT ) PORT_8WAY
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP    ) PORT_8WAY
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN  ) PORT_8WAY

	PORT_START_TAG("IN1")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_COIN2 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_BUTTON2 ) PORT_COCKTAIL
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_BUTTON1 ) PORT_COCKTAIL
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT  ) PORT_8WAY PORT_COCKTAIL
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT ) PORT_8WAY PORT_COCKTAIL
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP    ) PORT_8WAY PORT_COCKTAIL
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN  ) PORT_8WAY PORT_COCKTAIL

	PORT_START_TAG("DSW0")
	PORT_DIPNAME( 0x01, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x01, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0x04, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x04, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x08, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x10, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x20, DEF_STR( On ) )
	PORT_DIPNAME( 0xc0, 0x00, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x00, "3" )
	PORT_DIPSETTING(    0x40, "4" )
	PORT_DIPSETTING(    0x80, "5" )
	PORT_DIPSETTING(    0xc0, "6" )

	PORT_START_TAG("DSW1")
	PORT_DIPNAME( 0x01, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x01, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x00, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0x02, "A 2/1 B 1/3" )
	PORT_DIPSETTING(    0x00, "A 1/1 B 1/6" )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x04, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x08, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x10, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x20, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x40, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x80, DEF_STR( On ) )
INPUT_PORTS_END

/* Graphics Layouts */

static const gfx_layout charlayout =
{
	8,8,	/* 8*8 characters */
	512,	/* 512 characters */
	2,	/* 2 bits per pixel */
	{ 512*8*8, 0 },	/* the bitplanes are separated */
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8	/* every char takes 8 consecutive bytes */
};
static const gfx_layout spritelayout =
{
	8,16,	/* 8*16 sprites */
	256,	/* 256 sprites */
	3,	/* 3 bits per pixel */
	{ 2*128*16*16, 128*16*16, 0 },	/* the bitplanes are separated */
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8,
			8*8, 9*8, 10*8, 11*8, 12*8, 13*8, 14*8, 15*8 },
	16*8	/* every sprite takes 16 consecutive bytes */
};

/* Graphics Decode Info */

static const gfx_decode gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &charlayout,       0, 128 },
	{ REGION_GFX2, 0, &spritelayout, 128*4,   1 },
	{ -1 }
};

/* Sound Interfaces */

static struct AY8910interface ay8910_interface_1 =
{
	0,
	soundlatch_r,
	dipsw_w,
	0
};

static struct AY8910interface ay8910_interface_2 =
{
	input_port_2_r,
	input_port_3_r
};

/* Machine Driver */

static MACHINE_DRIVER_START( blueprnt )
	// basic machine hardware
	MDRV_CPU_ADD(Z80, 7000000/2)	// 3.5 MHz
	MDRV_CPU_PROGRAM_MAP(blueprnt_map, 0)
	MDRV_CPU_VBLANK_INT(irq0_line_hold, 1)

	MDRV_CPU_ADD(Z80, 10000000/2/2/2)	// 1.25 MHz (2H)
	MDRV_CPU_PROGRAM_MAP(sound_map, 0)
	MDRV_CPU_VBLANK_INT(irq0_line_hold, 4)	// IRQs connected to 32V
											// NMIs are caused by the main CPU
	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	// video hardware
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(32*8, 32*8)
	MDRV_VISIBLE_AREA(0*8, 32*8-1, 2*8, 30*8-1)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(16)
	MDRV_COLORTABLE_LENGTH(128*4+8)

	MDRV_PALETTE_INIT(blueprnt)
	MDRV_VIDEO_START(blueprnt)
	MDRV_VIDEO_UPDATE(blueprnt)

	// sound hardware
	MDRV_SPEAKER_STANDARD_MONO("mono")

	MDRV_SOUND_ADD(AY8910, 10000000/2/2/2)
	MDRV_SOUND_CONFIG(ay8910_interface_1)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.25)

	MDRV_SOUND_ADD(AY8910, 10000000/2/2/2/2)
	MDRV_SOUND_CONFIG(ay8910_interface_2)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.25)
MACHINE_DRIVER_END

/* ROMs */

ROM_START( blueprnt )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "bp-1.1m",   0x0000, 0x1000, CRC(b20069a6) SHA1(aa0a61c898ec58fc4872a24666f422e1abdc09f3) )
	ROM_LOAD( "bp-2.1n",   0x1000, 0x1000, CRC(4a30302e) SHA1(a3a22b78585cc9677bf03bbfeb20afb05f026075) )
	ROM_LOAD( "bp-3.1p",   0x2000, 0x1000, CRC(6866ca07) SHA1(a0df14eee9240fad42ceb6f926d34755e8442411) )
	ROM_LOAD( "bp-4.1r",   0x3000, 0x1000, CRC(5d3cfac3) SHA1(7e6ab8398d799aaf0fcaa0769a827471d8c872e9) )
	ROM_LOAD( "bp-5.1s",   0x4000, 0x1000, CRC(a556cac4) SHA1(0fe7070c70792d883c29f3d12a33238b5ed8af22) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* 64k for the audio CPU */
	ROM_LOAD( "snd-1.3u",  0x0000, 0x1000, CRC(fd38777a) SHA1(0ed230e0fa047d3171e7141e5620b4c750b07629) )
	ROM_LOAD( "snd-2.3v",  0x2000, 0x1000, CRC(33d5bf5b) SHA1(3ac684cd48559cd0eab32f9e7ce3ec6eca88dcd4) )

	ROM_REGION( 0x2000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "bg-1.3c",   0x0000, 0x1000, CRC(ac2a61bc) SHA1(e56708d261648478d1dae4769118546411299e59) )
	ROM_LOAD( "bg-2.3d",   0x1000, 0x1000, CRC(81fe85d7) SHA1(fa637631d25f7499d2325cce77d11e1d624f5e07) )

	ROM_REGION( 0x3000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "red.17d",   0x0000, 0x1000, CRC(a73b6483) SHA1(9f7756d032a8ffaa4aa236fc5117f476916986e0) )
	ROM_LOAD( "blue.18d",  0x1000, 0x1000, CRC(7d622550) SHA1(8283debff8253996513148629ec55831e48e8e92) )
	ROM_LOAD( "green.20d", 0x2000, 0x1000, CRC(2fcb4f26) SHA1(508cb2800737bad0a7dea0789d122b7c802aecfd) )
ROM_END

ROM_START( blueprnj )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "bp-1j.1m",   0x0000, 0x1000, CRC(2e746693) SHA1(4a9bb023f753ba792d1db86a0fb128d5261db685) )
	ROM_LOAD( "bp-2j.1n",   0x1000, 0x1000, CRC(a0eb0b8e) SHA1(b3c830b61172880fd2843a47350d8cb9461a25a4) )
	ROM_LOAD( "bp-3j.1p",   0x2000, 0x1000, CRC(c34981bb) SHA1(1c7fa9d599b3458f665e95d92cafda8851098c8f) )
	ROM_LOAD( "bp-4j.1r",   0x3000, 0x1000, CRC(525e77b5) SHA1(95c898be78881802b801f071d1d88062dcb1b798) )
	ROM_LOAD( "bp-5j.1s",   0x4000, 0x1000, CRC(431a015f) SHA1(e00912ac501bdd6750f63b53204553a40ad6605a) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* 64k for the audio CPU */
	ROM_LOAD( "snd-1.3u",  0x0000, 0x1000, CRC(fd38777a) SHA1(0ed230e0fa047d3171e7141e5620b4c750b07629) )
	ROM_LOAD( "snd-2.3v",  0x2000, 0x1000, CRC(33d5bf5b) SHA1(3ac684cd48559cd0eab32f9e7ce3ec6eca88dcd4) )

	ROM_REGION( 0x2000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "bg-1j.3c",   0x0000, 0x0800, CRC(43718c34) SHA1(5df4794a38866c7f03b264581c8555b9bec3969f) )
	ROM_LOAD( "bg-2j.3d",   0x1000, 0x0800, CRC(d3ce077d) SHA1(a9086b494437f9d4d3c0a6c36595a03d3a229a24) )

	ROM_REGION( 0x3000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "redj.17d",   0x0000, 0x1000, CRC(83da108f) SHA1(575d6505bd3d600324c4f656e28218deaaa470e4) )
	ROM_LOAD( "bluej.18d",  0x1000, 0x1000, CRC(b440f32f) SHA1(bd464ff324d4ef7c7c924886417b55bcb6f74fb9) )
	ROM_LOAD( "greenj.20d", 0x2000, 0x1000, CRC(23026765) SHA1(9b16de37922208f4f2d2afc94189f11f5e5011fa) )
ROM_END

ROM_START( saturn )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "r1",           0x0000, 0x1000, CRC(18a6d68e) SHA1(816baca24dd75c6f9d4c91c86f90825dbb9a1347) )
	ROM_LOAD( "r2",           0x1000, 0x1000, CRC(a7dd2665) SHA1(02d03fb436c704ccdbad751ccf034742fcd4ae43) )
	ROM_LOAD( "r3",           0x2000, 0x1000, CRC(b9cfa791) SHA1(4f4c7b1dd347c6f402124ddf235a02e812dc536d) )
	ROM_LOAD( "r4",           0x3000, 0x1000, CRC(c5a997e7) SHA1(134d2719bf9f14dc22365d03384271f6a6f3a448) )
	ROM_LOAD( "r5",           0x4000, 0x1000, CRC(43444d00) SHA1(3b58c9387eac75e713943a5ea9c8922634772a67) )
	ROM_LOAD( "r6",           0x5000, 0x1000, CRC(4d4821f6) SHA1(e414751d73c3f6e86d265540c1ebf69b95088b43) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* 64k for the audio CPU */
	ROM_LOAD( "r7",           0x0000, 0x1000, CRC(dd43e02f) SHA1(1d95a307cb4ef523f024cb9c60382a2ac8c17b1c) )
	ROM_LOAD( "r8",           0x2000, 0x1000, CRC(7f9d0877) SHA1(335b17d187089e91bd3002778821921e73ec59d2) )

	ROM_REGION( 0x2000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "r10",          0x0000, 0x1000, CRC(35987d61) SHA1(964503c3b17299b27b611943eebca9bc7c93a18c) )
	ROM_LOAD( "r9",           0x1000, 0x1000, CRC(ca6a7fda) SHA1(fac72535bb30d3527effa64900830403fb98c5c5) )

	ROM_REGION( 0x3000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "r11",          0x0000, 0x1000, CRC(6e4e6e5d) SHA1(f91188712e4b93d5676238c60d5a698891c3167a) )
	ROM_LOAD( "r12",          0x1000, 0x1000, CRC(46fc049e) SHA1(dc3027c2dcbf7a9b2eeeb165d0b99ce188f26d20) )
	ROM_LOAD( "r13",          0x2000, 0x1000, CRC(8b3e8c32) SHA1(65e2bf4a9f45be39419d85b2ee46b9c5eeff8f57) )
ROM_END

/* Game Drivers */

GAME( 1982, blueprnt, 0,        blueprnt, blueprnt, 0, ROT270, "[Zilec Electronics] Bally Midway", "Blue Print (Midway)", 0 )
GAME( 1982, blueprnj, blueprnt, blueprnt, blueprnt, 0, ROT270, "[Zilec Electronics] Jaleco", "Blue Print (Jaleco)", 0 )
GAME( 1983, saturn,   0,        blueprnt, saturn,   0, ROT270, "[Zilec Electronics] Jaleco", "Saturn", 0 )
