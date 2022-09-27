/*
    Boogie Wings (aka The Great Ragtime Show)
    Data East, 1992

    PCB No: DE-0379-1

    CPU: DE102
    Sound: HuC6280A, YM2151, YM3012, OKI M6295 (x2)
    OSC: 32.220MHz, 28.000MHz
    DIPs: 8 position (x2)
    PALs: (not dumped) VF-01 (PAL16L8)
                    VF-02 (22CV10P)
                    VF-03 (PAL16L8)
                    VF-04 (PAL 16L8)
                    VF-05 (PAL 16L8)

    PROM: MB7122 (compatible to 82S137, located near MBD-09)

    RAM: 62256 (x2), 6264 (x5)

    Data East Chips:   52 (x2)
                    141 (x2)
                    102
                    104
                    113
                    99
                    71 (x2)
                    200

    ROMs:
    kn00-2.2b   27c2001   \
    kn01-2.4b   27c2001    |  Main program
    kn02-2.2e   27c2001    |
    kn03-2.4e   27c2001   /
    kn04.8e     27c512    \
    kn05.9e     27c512    /   near 141's and MBD-00, MBD-01 and MBD-02
    kn06.18p    27c512        Sound Program
    mbd-00.8b   16M
    mbd-01.9b   16M
    mbd-02.10e  4M
    mbd-03.13b  16M
    mbd-04.14b  16M
    mbd-05.16b  16M
    mbd-06.17b  16M
    mbd-07.18b  16M
    mbd-08.19b  16M
    mbd-09.16p  4M         Oki Samples
    mbd-10.17p  4M         Oki Samples


    Driver by Bryan McPhail and David Haywood.

    Todo:
        Sprite priorities aren't verified to be 100% accurate.
        There may be some kind of fullscreen palette effect (controlled by bit 3 in priority
        word - used at end of each level, and on final boss).
        A shadow effect (used in level 1) is not implemented.
*/

#include "driver.h"
#include "cpu/m68000/m68000.h"
#include "cpu/h6280/h6280.h"
#include "decocrpt.h"
#include "deco16ic.h"
#include "decoprot.h"
#include "sound/2151intf.h"
#include "sound/okim6295.h"

extern void deco102_decrypt(int region, int address_xor, int data_select_xor, int opcode_select_xor);
VIDEO_START(boogwing);
VIDEO_UPDATE(boogwing);

/**********************************************************************************/

static ADDRESS_MAP_START( boogwing_map, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x0fffff) AM_ROM
	AM_RANGE(0x200000, 0x20ffff) AM_RAM

	AM_RANGE(0x220000, 0x220001) AM_WRITE(deco16_priority_w)
	AM_RANGE(0x220002, 0x22000f) AM_NOP

	AM_RANGE(0x240000, 0x240001) AM_WRITE(buffer_spriteram16_w)
	AM_RANGE(0x242000, 0x2427ff) AM_RAM AM_BASE(&spriteram16) AM_SIZE(&spriteram_size)
	AM_RANGE(0x244000, 0x244001) AM_WRITE(buffer_spriteram16_2_w)
	AM_RANGE(0x246000, 0x2467ff) AM_RAM AM_BASE(&spriteram16_2) AM_SIZE(&spriteram_2_size)

	AM_RANGE(0x24e6c0, 0x24e6c1) AM_READ(input_port_1_word_r)
	AM_RANGE(0x24e138, 0x24e139) AM_READ(input_port_0_word_r)
	AM_RANGE(0x24e344, 0x24e345) AM_READ(input_port_2_word_r)
	AM_RANGE(0x24e000, 0x24e7ff) AM_WRITE(deco16_104_prot_w) AM_BASE(&deco16_prot_ram)

	AM_RANGE(0x260000, 0x26000f) AM_WRITE(MWA16_RAM) AM_BASE(&deco16_pf12_control)
	AM_RANGE(0x264000, 0x265fff) AM_READ(MRA16_RAM) AM_WRITE(deco16_pf1_data_w) AM_BASE(&deco16_pf1_data)
	AM_RANGE(0x266000, 0x267fff) AM_READ(MRA16_RAM) AM_WRITE(deco16_pf2_data_w) AM_BASE(&deco16_pf2_data)
	AM_RANGE(0x268000, 0x268fff) AM_READ(MRA16_RAM) AM_WRITE(MWA16_RAM) AM_BASE(&deco16_pf1_rowscroll)
	AM_RANGE(0x26a000, 0x26afff) AM_READ(MRA16_RAM) AM_WRITE(MWA16_RAM) AM_BASE(&deco16_pf2_rowscroll)

	AM_RANGE(0x270000, 0x27000f) AM_WRITE(MWA16_RAM) AM_BASE(&deco16_pf34_control)
	AM_RANGE(0x274000, 0x275fff) AM_READ(MRA16_RAM) AM_WRITE(deco16_pf3_data_w) AM_BASE(&deco16_pf3_data)
	AM_RANGE(0x276000, 0x277fff) AM_READ(MRA16_RAM) AM_WRITE(deco16_pf4_data_w) AM_BASE(&deco16_pf4_data)
	AM_RANGE(0x278000, 0x278fff) AM_READ(MRA16_RAM) AM_WRITE(MWA16_RAM) AM_BASE(&deco16_pf3_rowscroll)
	AM_RANGE(0x27a000, 0x27afff) AM_READ(MRA16_RAM) AM_WRITE(MWA16_RAM) AM_BASE(&deco16_pf4_rowscroll)

	AM_RANGE(0x280000, 0x28000f) AM_NOP // ?
	AM_RANGE(0x282000, 0x282001) AM_NOP // Palette setup?
	AM_RANGE(0x282008, 0x282009) AM_WRITE(deco16_palette_dma_w)
	AM_RANGE(0x284000, 0x285fff) AM_WRITE(deco16_buffered_palette_w) AM_BASE(&paletteram16)

	AM_RANGE(0x3c0000, 0x3c004f) AM_RAM // ?
ADDRESS_MAP_END


static ADDRESS_MAP_START( sound_readmem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x000000, 0x00ffff) AM_READ(MRA8_ROM)
	AM_RANGE(0x100000, 0x100001) AM_READ(MRA8_NOP)
	AM_RANGE(0x110000, 0x110001) AM_READ(YM2151_status_port_0_r)
	AM_RANGE(0x120000, 0x120001) AM_READ(OKIM6295_status_0_r)
	AM_RANGE(0x130000, 0x130001) AM_READ(OKIM6295_status_1_r)
	AM_RANGE(0x140000, 0x140001) AM_READ(soundlatch_r)
	AM_RANGE(0x1f0000, 0x1f1fff) AM_READ(MRA8_BANK8)
ADDRESS_MAP_END

static ADDRESS_MAP_START( sound_writemem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x000000, 0x00ffff) AM_WRITE(MWA8_ROM)
	AM_RANGE(0x100000, 0x100001) AM_WRITE(MWA8_NOP)
	AM_RANGE(0x110000, 0x110001) AM_WRITE(YM2151_word_0_w)
	AM_RANGE(0x120000, 0x120001) AM_WRITE(OKIM6295_data_0_w)
	AM_RANGE(0x130000, 0x130001) AM_WRITE(OKIM6295_data_1_w)
	AM_RANGE(0x1f0000, 0x1f1fff) AM_WRITE(MWA8_BANK8)
	AM_RANGE(0x1fec00, 0x1fec01) AM_WRITE(H6280_timer_w)
	AM_RANGE(0x1ff400, 0x1ff403) AM_WRITE(H6280_irq_status_w)
ADDRESS_MAP_END

/**********************************************************************************/

INPUT_PORTS_START( boogwing )
	PORT_START	/* 16bit */
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x0008, IP_ACTIVE_HIGH, IPT_VBLANK )

	PORT_START	/* 16bit */
	PORT_DIPNAME( 0x0007, 0x0007, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(      0x0001, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0x0007, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x0006, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(      0x0005, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(      0x0004, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(      0x0003, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(      0x0002, DEF_STR( 1C_6C ) )
	PORT_DIPNAME( 0x0038, 0x0038, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(      0x0008, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0x0038, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x0030, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(      0x0028, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(      0x0020, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(      0x0018, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(      0x0010, DEF_STR( 1C_6C ) )
	PORT_DIPNAME( 0x0040, 0x0040, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(      0x0040, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0080, 0x0080, "Continue Coin" )
	PORT_DIPSETTING(      0x0080, "Normal Coin Credit" )
	PORT_DIPSETTING(      0x0000, "2 Start/1 Continue" )
	PORT_DIPNAME( 0x0300, 0x0300, DEF_STR( Lives ) )
	PORT_DIPSETTING(      0x0100, "1" )
	PORT_DIPSETTING(      0x0300, "2" )
	PORT_DIPSETTING(      0x0200, "3" )
	PORT_DIPSETTING(      0x0000, "4" )
	PORT_DIPNAME( 0x0c00, 0x0c00, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(      0x0800, DEF_STR( Easy ) )
	PORT_DIPSETTING(      0x0c00, DEF_STR( Normal ) )
	PORT_DIPSETTING(      0x0400, DEF_STR( Hard ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( Hardest ) )
	PORT_DIPNAME( 0x1000, 0x1000, "Coin Slots" )
	PORT_DIPSETTING(      0x1000, "Common" )
	PORT_DIPSETTING(      0x0000, "Individual" )
	PORT_DIPNAME( 0x2000, 0x2000, "Stage Reset" ) /* At loss of life */
	PORT_DIPSETTING(      0x2000, "Point of Termination" )
	PORT_DIPSETTING(      0x0000, "Beginning of Stage" )
	PORT_DIPNAME( 0x4000, 0x4000, DEF_STR( Unknown ) ) /* Manual shows as OFF and states "Don't Change" */
	PORT_DIPSETTING(      0x4000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x8000, 0x0000, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(      0x8000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )

	PORT_START	/* 16bit */
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(1)
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(1)
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_PLAYER(1)
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(2)
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(2)
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_PLAYER(2)
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_START2 )
INPUT_PORTS_END

/**********************************************************************************/

static const gfx_layout tile_8x8_layout =
{
	8,8,
	RGN_FRAC(1,2),
	4,
	{ RGN_FRAC(1,2)+8,RGN_FRAC(1,2)+0,RGN_FRAC(0,2)+8,RGN_FRAC(0,2)+0 },
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*16, 1*16, 2*16, 3*16, 4*16, 5*16, 6*16, 7*16 },
	8*16
};

static const gfx_layout tile_16x16_layout_5bpp =
{
	16,16,
	RGN_FRAC(1,3),
	5,
	{ RGN_FRAC(2,3), RGN_FRAC(1,3)+8,RGN_FRAC(1,3)+0,RGN_FRAC(0,3)+8,RGN_FRAC(0,3)+0 },
	{ 256,257,258,259,260,261,262,263,0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*16, 1*16, 2*16, 3*16, 4*16, 5*16, 6*16, 7*16,8*16,9*16,10*16,11*16,12*16,13*16,14*16,15*16 },
	32*16
};

static const gfx_layout tile_16x16_layout =
{
	16,16,
	RGN_FRAC(1,2),
	4,
	{ RGN_FRAC(1,2)+8,RGN_FRAC(1,2)+0,RGN_FRAC(0,2)+8,RGN_FRAC(0,2)+0 },
	{ 256,257,258,259,260,261,262,263,0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*16, 1*16, 2*16, 3*16, 4*16, 5*16, 6*16, 7*16,8*16,9*16,10*16,11*16,12*16,13*16,14*16,15*16 },
	32*16
};

static const gfx_layout spritelayout =
{
	16,16,
	RGN_FRAC(1,1),
	4,
	{ 24,8,16,0 },
	{ 512,513,514,515,516,517,518,519, 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*32, 1*32, 2*32, 3*32, 4*32, 5*32, 6*32, 7*32,
	  8*32, 9*32,10*32,11*32,12*32,13*32,14*32,15*32},
	32*32
};


static const gfx_decode gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &tile_8x8_layout,            0, 16 },	/* Tiles (8x8) */
	{ REGION_GFX2, 0, &tile_16x16_layout_5bpp, 0x100, 16 },	/* Tiles (16x16) */
	{ REGION_GFX3, 0, &tile_16x16_layout,      0x300, 32 },	/* Tiles (16x16) */
	{ REGION_GFX4, 0, &spritelayout,           0x500, 32 },	/* Sprites (16x16) */
	{ REGION_GFX5, 0, &spritelayout,           0x700, 16 },	/* Sprites (16x16) */
	{ -1 } /* end of array */
};

/**********************************************************************************/

static void sound_irq(int state)
{
	cpunum_set_input_line(1,1,state); /* IRQ 2 */
}

static WRITE8_HANDLER( sound_bankswitch_w )
{
	OKIM6295_set_bank_base(1, ((data & 2)>>1) * 0x40000);
	OKIM6295_set_bank_base(0, (data & 1) * 0x40000);
}

static struct YM2151interface ym2151_interface =
{
	sound_irq,
	sound_bankswitch_w
};


static MACHINE_DRIVER_START( boogwing )
	/* basic machine hardware */
	MDRV_CPU_ADD(M68000, 14000000)	/* DE102 */
	MDRV_CPU_PROGRAM_MAP(boogwing_map,0)
	MDRV_CPU_VBLANK_INT(irq6_line_hold,1)

	MDRV_CPU_ADD(H6280,32220000/4)
	MDRV_CPU_PROGRAM_MAP(sound_readmem, sound_writemem)

	MDRV_FRAMES_PER_SECOND(58)
	MDRV_VBLANK_DURATION(DEFAULT_REAL_60HZ_VBLANK_DURATION)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER | VIDEO_NEEDS_6BITS_PER_GUN | VIDEO_BUFFERS_SPRITERAM | VIDEO_RGB_DIRECT)
	MDRV_SCREEN_SIZE(40*8, 32*8)
	MDRV_VISIBLE_AREA(0*8, 40*8-1, 1*8, 31*8-1)

	MDRV_PALETTE_LENGTH(2048)
	MDRV_GFXDECODE(gfxdecodeinfo)

	MDRV_VIDEO_START(boogwing)
	MDRV_VIDEO_UPDATE(boogwing)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_STEREO("left", "right")

	MDRV_SOUND_ADD(YM2151, 32220000/9)
	MDRV_SOUND_CONFIG(ym2151_interface)
	MDRV_SOUND_ROUTE(0, "left", 0.80)
	MDRV_SOUND_ROUTE(1, "right", 0.80)

	MDRV_SOUND_ADD(OKIM6295, 32220000/32/132)
	MDRV_SOUND_CONFIG(okim6295_interface_region_1)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "left", 1.40)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "right", 1.40)

	MDRV_SOUND_ADD(OKIM6295, 32220000/16/132)
	MDRV_SOUND_CONFIG(okim6295_interface_region_2)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "left", 0.30)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "right", 0.30)
MACHINE_DRIVER_END

/**********************************************************************************/

ROM_START( boogwing )
	ROM_REGION( 0x100000, REGION_CPU1, 0 ) /* DE102 code (encrypted) */
	ROM_LOAD16_BYTE( "kn00-2.2b",    0x000000, 0x040000, CRC(e38892b9) SHA1(49b5637965a43e0378e1258c5f0a780926f1f283) )
	ROM_LOAD16_BYTE( "kn02-2.2e",    0x000001, 0x040000, CRC(8426efef) SHA1(2ea33cbd58b638053d75668a484648dbf67dabb8) )
	ROM_LOAD16_BYTE( "kn01-2.4b",    0x080000, 0x040000, CRC(3ad4b54c) SHA1(5141001768266995078407851b445378b21453de) )
	ROM_LOAD16_BYTE( "kn03-2.4e",    0x080001, 0x040000, CRC(10b61f4a) SHA1(41d7f670defbd7dae89afafac9839a9e237814d5) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )
	ROM_LOAD( "kn06.18p",    0x00000, 0x10000, CRC(3e8bc4e1) SHA1(7e4c357afefa47b8f101727e06485eb9ebae635d) )

	ROM_REGION( 0x20000, REGION_GFX1, ROMREGION_DISPOSE ) /* Tiles 1 */
	ROM_LOAD16_BYTE( "kn05.9e",   0x00000, 0x010000, CRC(d10aef95) SHA1(a611a35ab312caee19c31da079c647679d31673d) )
	ROM_LOAD16_BYTE( "kn04.8e",   0x00001, 0x010000, CRC(329323a8) SHA1(e2ec7b059301c0a2e052dfc683e044c808ad9b33) )

	ROM_REGION( 0x300000, REGION_GFX2, ROMREGION_DISPOSE ) /* Tiles 2 */
	ROM_LOAD( "mbd-01.9b", 0x000000, 0x100000, CRC(d7de4f4b) SHA1(4747f8795e277ed8106667b6f68e1176d95db684) )
	ROM_LOAD( "mbd-00.8b", 0x100000, 0x100000, CRC(adb20ba9) SHA1(2ffa1dd19a438a4d2f5743b1050a8037183a3e7d) )
	/* 0x100000 bytes expanded from mbd-02.10e copied here later */

	ROM_REGION( 0x200000, REGION_GFX3, ROMREGION_DISPOSE ) /* Tiles 3 */
	ROM_LOAD( "mbd-03.13b",   0x000000, 0x100000, CRC(cf798f2c) SHA1(f484a22679d6a4d4b0dcac820de3f1a37cbc478f) )
	ROM_LOAD( "mbd-04.14b",   0x100000, 0x100000, CRC(d9764d0b) SHA1(74d6f09d65d073606a6e10556cedf740aa50ff08) )

	ROM_REGION( 0x400000, REGION_GFX4, ROMREGION_DISPOSE ) /* Sprites 1 */
	ROM_LOAD16_BYTE( "mbd-05.16b",    0x000001, 0x200000, CRC(1768c66a) SHA1(06bf3bb187c65db9dcce959a43a7231e2ac45c17) )
	ROM_LOAD16_BYTE( "mbd-06.17b",    0x000000, 0x200000, CRC(7750847a) SHA1(358266ed68a9816094e7aab0905d958284c8ce98) )

	ROM_REGION( 0x400000, REGION_GFX5, ROMREGION_DISPOSE ) /* Sprites 2 */
	ROM_LOAD16_BYTE( "mbd-07.18b",    0x000001, 0x200000, CRC(241faac1) SHA1(588be0cf2647c1d185a99c987a5a20ab7ad8dea8) )
	ROM_LOAD16_BYTE( "mbd-08.19b",    0x000000, 0x200000, CRC(f13b1e56) SHA1(f8f5e8c4e6c159f076d4e6505bd901ade5c6a0ca) )

	ROM_REGION( 0x0100000, REGION_GFX6, ROMREGION_DISPOSE ) /* 1bpp graphics */
	ROM_LOAD16_BYTE( "mbd-02.10e",    0x000000, 0x080000, CRC(b25aa721) SHA1(efe800759080bd1dac2da93bd79062a48c5da2b2) )

	ROM_REGION( 0x80000, REGION_SOUND1, 0 ) /* Oki samples 1 */
	ROM_LOAD( "mbd-10.17p",    0x000000, 0x080000, CRC(f159f76a) SHA1(0b1ea69fecdd151e2b1fa96a21eade492499691d) )

	ROM_REGION( 0x80000, REGION_SOUND2, 0 ) /* Oki samples 1 */
	ROM_LOAD( "mbd-09.16p",    0x000000, 0x080000, CRC(f44f2f87) SHA1(d941520bdfc9e6d88c45462bc1f697c18f33498e) )

	ROM_REGION( 0x000400, REGION_PROMS, ROMREGION_DISPOSE ) /* Priority (not used) */
	ROM_LOAD( "kj-00.15n",    0x000000, 0x00400, CRC(add4d50b) SHA1(080e5a8192a146d5141aef5c8d9996ddf8cd3ab4) )
ROM_END

ROM_START( boogwina )
	ROM_REGION( 0x100000, REGION_CPU1, 0 ) /* DE102 code (encrypted) */
	ROM_LOAD16_BYTE( "km00-2.2b",    0x000000, 0x040000, CRC(71ab71c6) SHA1(00bfd71dd9ae5f12c574ab0ecc07d85898930c4b) )
	ROM_LOAD16_BYTE( "km02-2.2e",    0x000001, 0x040000, CRC(e90f07f9) SHA1(1e8bd3983ed875f4752cbf2ab1c7e748d3df019c) )
	ROM_LOAD16_BYTE( "km01-2.4b",    0x080000, 0x040000, CRC(7fdce2d3) SHA1(5ce9b8ac26700f1c3bfb3ce4845f890b81241823) )
	ROM_LOAD16_BYTE( "km03-2.4e",    0x080001, 0x040000, CRC(0b582de3) SHA1(f5c58c7e0e8a227506a81e38c266356596dcda7b) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )
	ROM_LOAD( "km06.18p",    0x00000, 0x10000, CRC(3e8bc4e1) SHA1(7e4c357afefa47b8f101727e06485eb9ebae635d) )

	ROM_REGION( 0x20000, REGION_GFX1, ROMREGION_DISPOSE ) /* Tiles 1 */
	ROM_LOAD16_BYTE( "kn05.9e",   0x00000, 0x010000, CRC(d10aef95) SHA1(a611a35ab312caee19c31da079c647679d31673d) )
	ROM_LOAD16_BYTE( "kn04.8e",   0x00001, 0x010000, CRC(329323a8) SHA1(e2ec7b059301c0a2e052dfc683e044c808ad9b33) )

	ROM_REGION( 0x300000, REGION_GFX2, ROMREGION_DISPOSE ) /* Tiles 2 */
	ROM_LOAD( "mbd-01.9b", 0x000000, 0x100000, CRC(d7de4f4b) SHA1(4747f8795e277ed8106667b6f68e1176d95db684) )
	ROM_LOAD( "mbd-00.8b", 0x100000, 0x100000, CRC(adb20ba9) SHA1(2ffa1dd19a438a4d2f5743b1050a8037183a3e7d) )
	/* 0x100000 bytes expanded from mbd-02.10e copied here later */

	ROM_REGION( 0x200000, REGION_GFX3, ROMREGION_DISPOSE ) /* Tiles 3 */
	ROM_LOAD( "mbd-03.13b",   0x000000, 0x100000, CRC(cf798f2c) SHA1(f484a22679d6a4d4b0dcac820de3f1a37cbc478f) )
	ROM_LOAD( "mbd-04.14b",   0x100000, 0x100000, CRC(d9764d0b) SHA1(74d6f09d65d073606a6e10556cedf740aa50ff08) )

	ROM_REGION( 0x400000, REGION_GFX4, ROMREGION_DISPOSE ) /* Sprites 1 */
	ROM_LOAD16_BYTE( "mbd-05.16b",    0x000001, 0x200000, CRC(1768c66a) SHA1(06bf3bb187c65db9dcce959a43a7231e2ac45c17) )
	ROM_LOAD16_BYTE( "mbd-06.17b",    0x000000, 0x200000, CRC(7750847a) SHA1(358266ed68a9816094e7aab0905d958284c8ce98) )

	ROM_REGION( 0x400000, REGION_GFX5, ROMREGION_DISPOSE ) /* Sprites 2 */
	ROM_LOAD16_BYTE( "mbd-07.18b",    0x000001, 0x200000, CRC(241faac1) SHA1(588be0cf2647c1d185a99c987a5a20ab7ad8dea8) )
	ROM_LOAD16_BYTE( "mbd-08.19b",    0x000000, 0x200000, CRC(f13b1e56) SHA1(f8f5e8c4e6c159f076d4e6505bd901ade5c6a0ca) )

	ROM_REGION( 0x0100000, REGION_GFX6, ROMREGION_DISPOSE ) /* 1bpp graphics */
	ROM_LOAD16_BYTE( "mbd-02.10e",    0x000000, 0x080000, CRC(b25aa721) SHA1(efe800759080bd1dac2da93bd79062a48c5da2b2) )

	ROM_REGION( 0x80000, REGION_SOUND1, 0 ) /* Oki samples 1 */
	ROM_LOAD( "mbd-10.17p",    0x000000, 0x080000, CRC(f159f76a) SHA1(0b1ea69fecdd151e2b1fa96a21eade492499691d) )

	ROM_REGION( 0x80000, REGION_SOUND2, 0 ) /* Oki samples 1 */
	ROM_LOAD( "mbd-09.16p",    0x000000, 0x080000, CRC(f44f2f87) SHA1(d941520bdfc9e6d88c45462bc1f697c18f33498e) )

	ROM_REGION( 0x000400, REGION_PROMS, ROMREGION_DISPOSE ) /* Priority (not used) */
	ROM_LOAD( "kj-00.15n",    0x000000, 0x00400, CRC(add4d50b) SHA1(080e5a8192a146d5141aef5c8d9996ddf8cd3ab4) )
ROM_END

ROM_START( ragtime )
	ROM_REGION( 0x100000, REGION_CPU1, 0 ) /* DE102 code (encrypted) */
	ROM_LOAD16_BYTE( "kh00-1.2b",    0x000000, 0x040000, CRC(553e179f) SHA1(ab156d9eca4a74084da944989529fd8f5a147dfc) )
	ROM_LOAD16_BYTE( "kh02-1.2e",    0x000001, 0x040000, CRC(6c759ec0) SHA1(f503d225c31543a7cd975fc599811a31ff729251) )
	ROM_LOAD16_BYTE( "kh01-1.4b",    0x080000, 0x040000, CRC(12dfee70) SHA1(a7c8fd118f589ef13bcb43a6aa446ff81015f5b3) )
	ROM_LOAD16_BYTE( "kh03-1.4e",    0x080001, 0x040000, CRC(076fea18) SHA1(342ca71b6d8c8be92dbf221ada717bdbd0061226) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )
	ROM_LOAD( "km06.18p",    0x00000, 0x10000, CRC(3e8bc4e1) SHA1(7e4c357afefa47b8f101727e06485eb9ebae635d) )

	ROM_REGION( 0x20000, REGION_GFX1, ROMREGION_DISPOSE ) /* Tiles 1 */
	ROM_LOAD16_BYTE( "kn05.9e",   0x00000, 0x010000, CRC(d10aef95) SHA1(a611a35ab312caee19c31da079c647679d31673d) )
	ROM_LOAD16_BYTE( "kn04.8e",   0x00001, 0x010000, CRC(329323a8) SHA1(e2ec7b059301c0a2e052dfc683e044c808ad9b33) )

	ROM_REGION( 0x300000, REGION_GFX2, ROMREGION_DISPOSE ) /* Tiles 2 */
	ROM_LOAD( "mbd-01.9b", 0x000000, 0x100000, CRC(d7de4f4b) SHA1(4747f8795e277ed8106667b6f68e1176d95db684) )
	ROM_LOAD( "mbd-00.8b", 0x100000, 0x100000, CRC(adb20ba9) SHA1(2ffa1dd19a438a4d2f5743b1050a8037183a3e7d) )
	/* 0x100000 bytes expanded from mbd-02.10e copied here later */

	ROM_REGION( 0x200000, REGION_GFX3, ROMREGION_DISPOSE ) /* Tiles 3 */
	ROM_LOAD( "mbd-03.13b",   0x000000, 0x100000, CRC(cf798f2c) SHA1(f484a22679d6a4d4b0dcac820de3f1a37cbc478f) )
	ROM_LOAD( "mbd-04.14b",   0x100000, 0x100000, CRC(d9764d0b) SHA1(74d6f09d65d073606a6e10556cedf740aa50ff08) )

	ROM_REGION( 0x400000, REGION_GFX4, ROMREGION_DISPOSE ) /* Sprites 1 */
	ROM_LOAD16_BYTE( "mbd-05.16b",    0x000001, 0x200000, CRC(1768c66a) SHA1(06bf3bb187c65db9dcce959a43a7231e2ac45c17) )
	ROM_LOAD16_BYTE( "mbd-06.17b",    0x000000, 0x200000, CRC(7750847a) SHA1(358266ed68a9816094e7aab0905d958284c8ce98) )

	ROM_REGION( 0x400000, REGION_GFX5, ROMREGION_DISPOSE ) /* Sprites 2 */
	ROM_LOAD16_BYTE( "mbd-07.18b",    0x000001, 0x200000, CRC(241faac1) SHA1(588be0cf2647c1d185a99c987a5a20ab7ad8dea8) )
	ROM_LOAD16_BYTE( "mbd-08.19b",    0x000000, 0x200000, CRC(f13b1e56) SHA1(f8f5e8c4e6c159f076d4e6505bd901ade5c6a0ca) )

	ROM_REGION( 0x0100000, REGION_GFX6, ROMREGION_DISPOSE ) /* 1bpp graphics */
	ROM_LOAD16_BYTE( "mbd-02.10e",    0x000000, 0x080000, CRC(b25aa721) SHA1(efe800759080bd1dac2da93bd79062a48c5da2b2) )

	ROM_REGION( 0x80000, REGION_SOUND1, 0 ) /* Oki samples 1 */
	ROM_LOAD( "mbd-10.17p",    0x000000, 0x080000, CRC(f159f76a) SHA1(0b1ea69fecdd151e2b1fa96a21eade492499691d) )

	ROM_REGION( 0x80000, REGION_SOUND2, 0 ) /* Oki samples 1 */
	ROM_LOAD( "mbd-09.16p",    0x000000, 0x080000, CRC(f44f2f87) SHA1(d941520bdfc9e6d88c45462bc1f697c18f33498e) )

	ROM_REGION( 0x000400, REGION_PROMS, ROMREGION_DISPOSE ) /* Priority (not used) */
	ROM_LOAD( "kj-00.15n",    0x000000, 0x00400, CRC(add4d50b) SHA1(080e5a8192a146d5141aef5c8d9996ddf8cd3ab4) )
ROM_END

static DRIVER_INIT( boogwing )
{
	const UINT8* src=memory_region(REGION_GFX6);
	UINT8* dst=memory_region(REGION_GFX2) + 0x200000;

	deco56_decrypt(REGION_GFX1);
	deco56_decrypt(REGION_GFX2);
	deco56_decrypt(REGION_GFX3);
	deco56_remap(REGION_GFX6);
	deco102_decrypt(REGION_CPU1, 0x42ba, 0x00, 0x18);
	memcpy(dst, src, 0x100000);
}

GAME( 1992, boogwing, 0,        boogwing, boogwing,  boogwing,  ROT0, "Data East Corporation", "Boogie Wings (Euro v1.5, 92.12.07)", 0 )
GAME( 1992, boogwina, boogwing, boogwing, boogwing,  boogwing,  ROT0, "Data East Corporation", "Boogie Wings (Asia v1.5, 92.12.07)", 0 )
GAME( 1992, ragtime,  boogwing, boogwing, boogwing,  boogwing,  ROT0, "Data East Corporation", "The Great Ragtime Show (Japan v1.5, 92.12.07)", 0 )
