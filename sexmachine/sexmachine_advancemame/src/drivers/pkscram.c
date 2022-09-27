/*

PCB# -   ANIMAL-01  Cosmo Electronics Corporation
68000 + OSC 8MHz
YM2203 + YM3014 + OSC 12MHz
DIPSw 8-position x2
RAM - 6264 (x2), TC5588 (x2), CXK5814 (x2)
3.6V battery

driver by David Haywood and few bits by Pierpaolo Prazzoli

*/

#include "driver.h"
#include "sound/2203intf.h"

static int out = 0;

static UINT16* pkscramble_fgtilemap_ram;
static UINT16* pkscramble_mdtilemap_ram;
static UINT16* pkscramble_bgtilemap_ram;

static tilemap *fg_tilemap, *md_tilemap, *bg_tilemap;

static WRITE16_HANDLER( pkscramble_fgtilemap_w )
{
	COMBINE_DATA(&pkscramble_fgtilemap_ram[offset]);
	tilemap_mark_tile_dirty(fg_tilemap, offset >> 1);
}

static WRITE16_HANDLER( pkscramble_mdtilemap_w )
{
	COMBINE_DATA(&pkscramble_mdtilemap_ram[offset]);
	tilemap_mark_tile_dirty(md_tilemap, offset >> 1);
}

static WRITE16_HANDLER( pkscramble_bgtilemap_w )
{
	COMBINE_DATA(&pkscramble_bgtilemap_ram[offset]);
	tilemap_mark_tile_dirty(bg_tilemap, offset >> 1);
}

static WRITE16_HANDLER( pkscramble_YM2203_w )
{
	switch (offset)
	{
		case 0: YM2203_control_port_0_w(0,data & 0xff);break;
		case 1: YM2203_write_port_0_w(0,data & 0xff);break;
	}
}

static READ16_HANDLER( pkscramble_YM2203_r )
{
	return YM2203_status_port_0_r(0);
}

// input bit 0x20 in port1 should stay low until bit 0x20 is written here, then
// it should stay high for some time (currently we cheat keeping the input always active)
static WRITE16_HANDLER( pkscramble_output_w )
{
	// OUTPUT
	// BIT
	// 0x0001 -> STL
	// 0x0002 -> SPL1
	// 0x0004 -> SPL2
	// 0x0008 -> SPL3
	// 0x0010 -> MSK
	// 0x0020 -> HPM
	// 0x0040 -> CNT1
	// 0x0080 -> CNT2
	// 0x0100 -> LED1
	// 0x0200 -> LED2
	// 0x0400 -> LED3
	// 0x0800 -> LED4
	// 0x1000 -> LED5

	// 0x2000 and 0x4000 are used too

	out = data;

	coin_counter_w(0, data & 0x80);
}

static ADDRESS_MAP_START( pkscramble_map, ADDRESS_SPACE_PROGRAM, 16 )
	ADDRESS_MAP_FLAGS( AMEF_ABITS(19) )
	AM_RANGE(0x000000, 0x01ffff) AM_ROM
	AM_RANGE(0x040000, 0x0400ff) AM_RAM AM_BASE(&generic_nvram16) AM_SIZE(&generic_nvram_size)
	AM_RANGE(0x041000, 0x043fff) AM_RAM // main ram
	AM_RANGE(0x044000, 0x044fff) AM_RAM AM_WRITE(pkscramble_fgtilemap_w) AM_BASE(&pkscramble_fgtilemap_ram) // fg tilemap
	AM_RANGE(0x045000, 0x045fff) AM_RAM AM_WRITE(pkscramble_mdtilemap_w) AM_BASE(&pkscramble_mdtilemap_ram) // md tilemap (just a copy of fg?)
	AM_RANGE(0x046000, 0x046fff) AM_RAM AM_WRITE(pkscramble_bgtilemap_w) AM_BASE(&pkscramble_bgtilemap_ram) // bg tilemap
	AM_RANGE(0x047000, 0x047fff) AM_RAM // unused
	AM_RANGE(0x048000, 0x048fff) AM_RAM AM_WRITE(paletteram16_xRRRRRGGGGGBBBBB_word_w) AM_BASE(&paletteram16)
	AM_RANGE(0x049000, 0x049001) AM_READ(input_port_0_word_r)
	AM_RANGE(0x049004, 0x049005) AM_READ(input_port_1_word_r)
	AM_RANGE(0x049008, 0x049009) AM_WRITE(pkscramble_output_w)
	AM_RANGE(0x049010, 0x049011) AM_WRITENOP
	AM_RANGE(0x049014, 0x049015) AM_WRITENOP
	AM_RANGE(0x049018, 0x049019) AM_WRITENOP
	AM_RANGE(0x04901c, 0x04901d) AM_WRITENOP
	AM_RANGE(0x049020, 0x049021) AM_WRITENOP
	AM_RANGE(0x04900c, 0x04900d) AM_READ(pkscramble_YM2203_r)
	AM_RANGE(0x04900c, 0x04900f) AM_WRITE(pkscramble_YM2203_w)
	AM_RANGE(0x052086, 0x052087) AM_WRITENOP
ADDRESS_MAP_END


INPUT_PORTS_START( pkscramble )
	PORT_START	/* Dips */
	PORT_DIPNAME( 0x0007, 0x0003, "Level" )
	PORT_DIPSETTING(      0x0000, "0" )
	PORT_DIPSETTING(      0x0001, "1" )
	PORT_DIPSETTING(      0x0002, "2" )
	PORT_DIPSETTING(      0x0003, "3" )
	PORT_DIPSETTING(      0x0004, "4" )
	PORT_DIPSETTING(      0x0005, "5" )
	PORT_DIPSETTING(      0x0006, "6" )
	PORT_DIPSETTING(      0x0007, "7" )
	PORT_DIPNAME( 0x0008, 0x0008, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0008, DEF_STR( On ) )
	PORT_DIPNAME( 0x0010, 0x0010, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0010, DEF_STR( On ) )
	PORT_DIPNAME( 0x0020, 0x0020, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0020, DEF_STR( On ) )
	PORT_DIPNAME( 0x0040, 0x0040, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0040, DEF_STR( On ) )
	PORT_DIPNAME( 0x0080, 0x0080, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0080, DEF_STR( On ) )
	PORT_DIPNAME( 0x0700, 0x0100, "Coin to Start" )
	PORT_DIPSETTING(      0x0100, "1" )
	PORT_DIPSETTING(      0x0200, "2" )
	PORT_DIPSETTING(      0x0300, "3" )
	PORT_DIPSETTING(      0x0400, "4" )
	PORT_DIPSETTING(      0x0500, "5" )
	PORT_DIPSETTING(      0x0600, "6" )
	PORT_DIPSETTING(      0x0700, "7" )
	PORT_DIPSETTING(      0x0000, DEF_STR( Free_Play ) )
	PORT_DIPNAME( 0x3800, 0x0800, DEF_STR( Coinage ) )
	PORT_DIPSETTING(      0x0800, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x1000, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(      0x1800, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(      0x2000, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(      0x2800, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(      0x3000, DEF_STR( 1C_6C ) )
	PORT_DIPSETTING(      0x3800, DEF_STR( 1C_7C ) )
	PORT_DIPSETTING(      0x0000, "No Credit" )
	PORT_DIPNAME( 0x4000, 0x4000, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x4000, DEF_STR( On ) )
	PORT_SERVICE( 0x8000, IP_ACTIVE_HIGH )

	PORT_START	/* 16bit */
	PORT_BIT( 0x0001, IP_ACTIVE_HIGH, IPT_BUTTON1 ) // Kick
	PORT_BIT( 0x0002, IP_ACTIVE_HIGH, IPT_BUTTON2 ) // Left
	PORT_BIT( 0x0004, IP_ACTIVE_HIGH, IPT_BUTTON3 ) // Center
	PORT_BIT( 0x0008, IP_ACTIVE_HIGH, IPT_BUTTON4 ) // Right
	PORT_BIT( 0x0010, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BIT( 0x0020, IP_ACTIVE_LOW,  IPT_SPECIAL ) // Hopper status
	PORT_DIPNAME( 0x0040, 0x0000, "Coin Test" )
	PORT_DIPSETTING(      0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0040, DEF_STR( On ) )
	PORT_BIT( 0x0080, IP_ACTIVE_HIGH, IPT_SERVICE1 )
	PORT_BIT( 0xff00, IP_ACTIVE_HIGH, IPT_UNKNOWN )
INPUT_PORTS_END


static void get_bg_tile_info(int tile_index)
{
	int tile  = pkscramble_bgtilemap_ram[tile_index*2];
	int color = pkscramble_bgtilemap_ram[tile_index*2 + 1] & 0x7f;

	SET_TILE_INFO(0,tile,color,0)
}

static void get_md_tile_info(int tile_index)
{
	int tile  = pkscramble_mdtilemap_ram[tile_index*2];
	int color = pkscramble_mdtilemap_ram[tile_index*2 + 1] & 0x7f;

	SET_TILE_INFO(0,tile,color,0)
}

static void get_fg_tile_info(int tile_index)
{
	int tile  = pkscramble_fgtilemap_ram[tile_index*2];
	int color = pkscramble_fgtilemap_ram[tile_index*2 + 1] & 0x7f;

	SET_TILE_INFO(0,tile,color,0)
}

VIDEO_START( pkscramble )
{
	bg_tilemap = tilemap_create(get_bg_tile_info, tilemap_scan_rows, TILEMAP_OPAQUE,      8, 8,32,32);
	md_tilemap = tilemap_create(get_md_tile_info, tilemap_scan_rows, TILEMAP_TRANSPARENT, 8, 8,32,32);
	fg_tilemap = tilemap_create(get_fg_tile_info, tilemap_scan_rows, TILEMAP_TRANSPARENT, 8, 8,32,32);

	tilemap_set_transparent_pen(md_tilemap,15);
	tilemap_set_transparent_pen(fg_tilemap,15);

	return 0;
}

VIDEO_UPDATE( pkscramble )
{
	tilemap_draw(bitmap,cliprect,bg_tilemap,0,0);
	tilemap_draw(bitmap,cliprect,md_tilemap,0,0);
	tilemap_draw(bitmap,cliprect,fg_tilemap,0,0);
}

static const gfx_layout tiles8x8_layout =
{
	8,8,
	RGN_FRAC(1,1),
	4,
	{ 0, 1, 2, 3 },
	{ 12, 8, 4, 0, 28, 24, 20, 16 },
	{ 0*32, 1*32, 2*32, 3*32, 4*32, 5*32, 6*32, 7*32 },
	32*8
};


static const gfx_decode gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &tiles8x8_layout, 0, 0x80 },
	{ -1 }
};

static void irqhandler(int irq)
{
	if(out & 0x10)
		cpunum_set_input_line(0,2,irq ? ASSERT_LINE : CLEAR_LINE);
}

static struct YM2203interface ym2203_interface =
{
	0,0,0,0,
	irqhandler
};

static MACHINE_DRIVER_START( pkscramble )
	/* basic machine hardware */
	MDRV_CPU_ADD(M68000, 8000000 )
	MDRV_CPU_PROGRAM_MAP(pkscramble_map,0)
	MDRV_CPU_VBLANK_INT(irq1_line_hold,1) /* only valid irq */

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	MDRV_NVRAM_HANDLER(generic_0fill)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(32*8, 32*8)
	MDRV_VISIBLE_AREA(0*8, 32*8-1, 0*8, 24*8-1)
	MDRV_PALETTE_LENGTH(0x800)
	MDRV_GFXDECODE(gfxdecodeinfo)

	MDRV_VIDEO_START(pkscramble)
	MDRV_VIDEO_UPDATE(pkscramble)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")

	MDRV_SOUND_ADD(YM2203, 12000000/4)
	MDRV_SOUND_CONFIG(ym2203_interface)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.60)
MACHINE_DRIVER_END


ROM_START( pkscram )
	ROM_REGION( 0x20000, REGION_CPU1, 0 ) /* 68k */
	ROM_LOAD16_BYTE( "pk1.6e", 0x00000, 0x10000, CRC(80e972e5) SHA1(cbbc6e1e3fbb65b40164e140f368d8fff85c1521) )
	ROM_LOAD16_BYTE( "pk2.6j", 0x00001, 0x10000, CRC(752c86d1) SHA1(2e0c669307bed6f9eab957b0e1316416e653a72f) )

	ROM_REGION( 0x40000, REGION_GFX1, 0 ) /* gfx */
	ROM_LOAD16_BYTE( "pk3.1c", 0x00000, 0x20000, CRC(0b18f2bc) SHA1(32892589442884ba02a1c6059ecb94e4ef516b86) )
	ROM_LOAD16_BYTE( "pk4.1e", 0x00001, 0x20000, CRC(a232d993) SHA1(1b7b15cf0fabf3b2b2e429506a78ff4c08f4f7a5) )
ROM_END


GAME( 1993, pkscram, 0, pkscramble, pkscramble, 0, ROT0, "Cosmo Electronics Corporation", "PK Scramble", 0)
