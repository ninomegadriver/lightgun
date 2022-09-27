/******************************************************************************************

Sengoku Mahjong (c) 1991 Sigma

driver by Angelo Salese & Pierpaolo Prazzoli

Based on the D-Con driver.

TODO:
- Find tilemap enable registers (needed especially when you coin up at the intro).
- Missing NVRAM emulation. (?)
  At startup a "Warning : Data in stock is wrong check ram" message appears because of that.

Notes:
- Some strings written in the sound rom:
  "SENGOKU-MAHJONG Z80 PROGRAM ROM VERSION 1.00 WRITTEN BY K.SAEKI" at location 0x00c0-0x00ff.
  "Copyright 1990/1991 Sigma" at location 0x770-0x789.

- To bypass the startup message, toggle "Reset" dip-switch or reset with F3.


CPU:    uPD70116C-8 (V30)
Sound:  Z80-A
        YM3812
        M6295
OSC:    14.31818MHz
        16.000MHz
Chips:  SEI0100
        SEI0160
        SEI0200
        SEI0210
        SEI0220


MAH1-1-1.915  samples

MAH1-2-1.013  sound prg. (ic1013:27c512)

MM01-1-1.21   main prg.
MM01-2-1.24

RS006.89      video timing?

RSSENGO0.64   chr.
RSSENGO1.68

RSSENGO2.72   chr.

*******************************************************************************************/

#include "driver.h"
#include "sndhrdw/seibu.h"
#include "sound/3812intf.h"

extern UINT8 *sengokmj_bgvram,*sengokmj_mdvram,*sengokmj_fgvram,*sengokmj_txvram;
static UINT8 sengokumj_mux_data;

WRITE8_HANDLER( sengokmj_bgvram_w );
WRITE8_HANDLER( sengokmj_fgvram_w );
WRITE8_HANDLER( sengokmj_mdvram_w );
WRITE8_HANDLER( sengokmj_txvram_w );
VIDEO_START( sengokmj );
VIDEO_UPDATE( sengokmj );

/* Multiplexer device for the mahjong panel */
static READ8_HANDLER( mahjong_panel_r )
{
	if(!offset)
	{
		switch(sengokumj_mux_data)
		{
			case 1:    return readinputport(3);
			case 2:    return readinputport(4);
			case 4:    return readinputport(5);
			case 8:    return readinputport(6);
			case 0x10: return readinputport(7);
			case 0x20: return readinputport(8);
		}
	}

	return 0xff;
}

static WRITE8_HANDLER( mahjong_panel_w )
{
	if(offset)	{ sengokumj_mux_data = data; }
}

static WRITE8_HANDLER( sengokmj_out_w )
{
	static UINT8 old = 0;
	static int coins_used = 0;

	if(!offset)
	{
		if((old & 4) == 0 && (data & 4) == 4)
		{
			coins_used++;
			//printf("coins used = %d\n",coins_used);
		}

		old = data;

	// data & 2 -> hopper related ?
	}
}


static ADDRESS_MAP_START( sengokmj_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x00000, 0x003ff) AM_RAM
	AM_RANGE(0x01000, 0x011ff) AM_RAM
	AM_RANGE(0x01234, 0x01239) AM_NOP // ?
	AM_RANGE(0x06000, 0x067ff) AM_RAM
	AM_RANGE(0x07800, 0x097ff) AM_RAM
	AM_RANGE(0x09800, 0x099ff) AM_RAM
	AM_RANGE(0x0c000, 0x0c7ff) AM_RAM AM_WRITE(sengokmj_bgvram_w) AM_BASE(&sengokmj_bgvram)
	AM_RANGE(0x0c800, 0x0cfff) AM_RAM AM_WRITE(sengokmj_fgvram_w) AM_BASE(&sengokmj_fgvram)
	AM_RANGE(0x0d000, 0x0d7ff) AM_RAM AM_WRITE(sengokmj_mdvram_w) AM_BASE(&sengokmj_mdvram)
	AM_RANGE(0x0d800, 0x0e7ff) AM_RAM AM_WRITE(sengokmj_txvram_w) AM_BASE(&sengokmj_txvram)
	AM_RANGE(0x0e800, 0x0f7ff) AM_RAM AM_WRITE(paletteram_xBBBBBGGGGGRRRRR_le_w) AM_BASE(&paletteram)
	AM_RANGE(0x0f800, 0x0ffff) AM_RAM AM_BASE(&spriteram)
	AM_RANGE(0xc0000, 0xfffff) AM_ROM
ADDRESS_MAP_END


static ADDRESS_MAP_START( sengokmj_io_map, ADDRESS_SPACE_IO, 8 )
	AM_RANGE(0x4000, 0x400f) AM_READWRITE(seibu_main_v30_r, seibu_main_v30_w)
	AM_RANGE(0x8000, 0x800f) AM_MIRROR(0x30) AM_WRITE(seibu_main_v30_w)
	AM_RANGE(0x8040, 0x804f) AM_WRITE(seibu_main_v30_w)
	AM_RANGE(0x8100, 0x8101) AM_WRITENOP // always 0
	AM_RANGE(0x8180, 0x8181) AM_WRITE(sengokmj_out_w)
	AM_RANGE(0x8080, 0x8081) AM_WRITENOP // ?
	AM_RANGE(0x80c0, 0x80c1) AM_WRITENOP // ?
	AM_RANGE(0x8140, 0x8141) AM_WRITE(mahjong_panel_w)
	AM_RANGE(0xc000, 0xc000) AM_READ(input_port_1_r)
	AM_RANGE(0xc001, 0xc001) AM_READ(input_port_2_r)
	AM_RANGE(0xc002, 0xc003) AM_READ(mahjong_panel_r)
	AM_RANGE(0xc004, 0xc004) AM_READ(input_port_9_r)
	AM_RANGE(0xc005, 0xc005) AM_READ(input_port_10_r)
ADDRESS_MAP_END


INPUT_PORTS_START( sengokmj )
	/* Must be port 0: coin inputs read through sound cpu */
	SEIBU_COIN_INPUTS

	PORT_START
	PORT_DIPNAME( 0x01,   0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(	  0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(	  0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02,   0x02, "Re-start" )
	PORT_DIPSETTING(	  0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(	  0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04,   0x04, "Double G" )
	PORT_DIPSETTING(	  0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(	  0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08,   0x08, "Double L" )
	PORT_DIPSETTING(	  0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(	  0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10,   0x10, "Kamon" )
	PORT_DIPSETTING(	  0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(   	  0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20,   0x20, DEF_STR( Unused ) )
	PORT_DIPSETTING(	  0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(	  0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40,   0x40, "Out Sw" )
	PORT_DIPSETTING(	  0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(	  0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80,   0x80, "Hopper" )
	PORT_DIPSETTING(	  0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(	  0x00, DEF_STR( On ) )

	PORT_START
	PORT_BIT(  0xff, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_MAHJONG_A )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_MAHJONG_E )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_MAHJONG_I )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_MAHJONG_M )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_MAHJONG_KAN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_MAHJONG_B )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_MAHJONG_F )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_MAHJONG_J )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_MAHJONG_N )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_MAHJONG_REACH )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_MAHJONG_BET )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_MAHJONG_C )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_MAHJONG_G )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_MAHJONG_K )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_MAHJONG_CHI )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_MAHJONG_RON )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_MAHJONG_D )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_MAHJONG_H )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_MAHJONG_L )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_MAHJONG_PON )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_MAHJONG_LAST_CHANCE )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_MAHJONG_SCORE )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_MAHJONG_DOUBLE_UP )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START
	PORT_DIPNAME( 0x01, 0x01, "Door" )
	PORT_DIPSETTING(	0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )
	PORT_SERVICE_NO_TOGGLE( 0x02, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0x04, 0x04, "Opt. 1st" )
	PORT_DIPSETTING(	0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, "Reset" )
	PORT_DIPSETTING(	0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unknown ) )
	PORT_DIPSETTING(	0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, "Cash" )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	/* Used, causes "Hopper RunAway" message if you toggle it */
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, "Meter" )
	PORT_DIPSETTING(	0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )

	PORT_START
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )
INPUT_PORTS_END


static const gfx_layout tilelayout =
{
	16,16,	/* 16*16 sprites  */
	RGN_FRAC(1,1),
	4,	/* 4 bits per pixel */
	{ 8, 12, 0, 4 },
	{ 3, 2, 1, 0, 16+3, 16+2, 16+1, 16+0,
             3+32*16, 2+32*16, 1+32*16, 0+32*16, 16+3+32*16, 16+2+32*16, 16+1+32*16, 16+0+32*16 },
	{ 0*16, 2*16, 4*16, 6*16, 8*16, 10*16, 12*16, 14*16,
			16*16, 18*16, 20*16, 22*16, 24*16, 26*16, 28*16, 30*16 },
	128*8	/* every sprite takes 128 consecutive bytes */
};

static const gfx_layout charlayout =
{
	8,8,
	RGN_FRAC(1,1),
	4,	/* 4 bits per pixel */
	{ 8, 12, 0, 4 },
	{ 3, 2, 1, 0, 16+3, 16+2, 16+1, 16+0 },
	{ 0*16, 2*16, 4*16, 6*16, 8*16, 10*16, 12*16, 14*16 },
	128*8	/* every sprite takes 128 consecutive bytes */
};

static const gfx_decode gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &tilelayout, 0x000, 0x40 }, /* Sprites */
	{ REGION_GFX2, 0, &tilelayout, 0x400, 0x30 }, /* Tiles */
	{ REGION_GFX2, 0, &charlayout, 0x700, 0x10 }, /* Text */
	{ -1 }
};

SEIBU_SOUND_SYSTEM_YM3812_HARDWARE

static INTERRUPT_GEN( sengokmj_interrupt )
{
	cpunum_set_input_line_and_vector(0,0,HOLD_LINE,0xcb/4);
}

static MACHINE_DRIVER_START( sengokmj )

	/* basic machine hardware */
	MDRV_CPU_ADD(V30, 16000000/2) /* V30-8 */
	MDRV_CPU_PROGRAM_MAP(sengokmj_map,0)
	MDRV_CPU_IO_MAP(sengokmj_io_map,0)
	MDRV_CPU_VBLANK_INT(sengokmj_interrupt,1)

	SEIBU_SOUND_SYSTEM_CPU(14318180/4)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	MDRV_MACHINE_RESET(seibu_sound_1)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(64*8, 32*8)
	MDRV_VISIBLE_AREA(16*8, 56*8-1, 2*8, 32*8-1)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(0x800)

	MDRV_VIDEO_START(sengokmj)
	MDRV_VIDEO_UPDATE(sengokmj)

	/* sound hardware */
	SEIBU_SOUND_SYSTEM_YM3812_INTERFACE(14318180/4,8000,1)
MACHINE_DRIVER_END


ROM_START( sengokmj )
	ROM_REGION( 0x100000, REGION_CPU1, 0 ) /* V30 code */
	ROM_LOAD16_BYTE( "mm01-1-1.21",  0xc0000, 0x20000, CRC(74076b46) SHA1(64b0ed5a8c32e21157ae12fe40519e4c605b329c) )
	ROM_LOAD16_BYTE( "mm01-2-1.24",  0xc0001, 0x20000, CRC(f1a7c131) SHA1(d0fbbdedbff8f05da0e0296baa41369bc41a67e4) )

	ROM_REGION( 0x20000, REGION_CPU2, 0 ) /* 64k code for sound Z80 */
	ROM_LOAD( "mah1-2-1.013", 0x000000, 0x08000, CRC(6a4f31b8) SHA1(5e1d7ed299c1fd65c7a43faa02831220f4251733) )
	ROM_CONTINUE(             0x010000, 0x08000 )
	ROM_COPY( REGION_CPU2, 0, 0x018000, 0x08000 )

	ROM_REGION( 0x100000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "rssengo2.72", 0x00000, 0x100000, CRC(fb215ff8) SHA1(f98c0a53ad9b97d209dd1f85c994fc17ec585bd7) )

	ROM_REGION( 0x200000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "rssengo0.64", 0x000000, 0x100000, CRC(36924b71) SHA1(814b2c69ab9876ccc57774e5718c05059ea23150) )
	ROM_LOAD( "rssengo1.68", 0x100000, 0x100000, CRC(1bbd00e5) SHA1(86391323b8e0d3b7e09a5914d87fb2adc48e5af4) )

	ROM_REGION( 0x20000, REGION_SOUND1, 0 )	 /* ADPCM samples */
	ROM_LOAD( "mah1-1-1.915", 0x00000, 0x20000, CRC(d4612e95) SHA1(937c5dbd25c89d4f4178b0bed510307020c5f40e) )

	ROM_REGION( 0x200, REGION_USER1, ROMREGION_DISPOSE ) /* not used */
	ROM_LOAD( "rs006.89", 0x000, 0x200, CRC(96f7646e) SHA1(400a831b83d6ac4d2a46ef95b97b1ee237099e44) ) /* Priority */
ROM_END

GAME( 1991, sengokmj, 0, sengokmj, sengokmj, 0, ROT0, "Sigma", "Sengoku Mahjong (Japan)", GAME_IMPERFECT_GRAPHICS )
