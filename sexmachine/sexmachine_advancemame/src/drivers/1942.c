/***************************************************************************

1942

driver by Paul Leaman


MAIN CPU:

0000-bfff ROM (8000-bfff banked)
cc00-cc7f Sprites
d000-d3ff Video RAM
d400-d7ff Color RAM
d800-dbff Background RAM (groups of 32 bytes, 16 code, 16 color/attribute)
e000-efff RAM

read:
c000      IN0
c001      IN1
c002      IN2
c003      DSW0
c004      DSW1

write:
c800      command for the audio CPU
c802-c803 background scroll
c804      bit 7: flip screen
          bit 4: cpu B reset
          bit 0: coin counter
c805      background palette bank selector
c806      bit 0-1 ROM bank selector 00=1-N5.BIN
                                    01=1-N6.BIN
                                    10=1-N7.BIN



SOUND CPU:

0000-3fff ROM
4000-47ff RAM
6000      command from the main CPU
8000      8910 #1 control
8001      8910 #1 write
c000      8910 #2 control
c001      8910 #2 write



Game runs in interrupt mode 0 (the devices supply the interrupt number).

Two interrupts must be triggered per refresh for the game to function
correctly.

0x10 is the video retrace. This controls the speed of the game and generally
     drives the code. This must be triggerd for each video retrace.
0x08 is the sound card service interupt. The game uses this to throw sounds
     at the sound CPU.

***************************************************************************/

#include "driver.h"
#include "sound/ay8910.h"


extern UINT8 *c1942_fgvideoram;
extern UINT8 *c1942_bgvideoram;


extern WRITE8_HANDLER( c1942_fgvideoram_w );
extern WRITE8_HANDLER( c1942_bgvideoram_w );
extern WRITE8_HANDLER( c1942_scroll_w );
extern WRITE8_HANDLER( c1942_c804_w );
extern WRITE8_HANDLER( c1942_palette_bank_w );

extern PALETTE_INIT( 1942 );
extern VIDEO_START( 1942 );
extern VIDEO_UPDATE( 1942 );



static WRITE8_HANDLER( c1942_bankswitch_w )
{
	memory_set_bank(1, data & 0x03);
}



static INTERRUPT_GEN( c1942_interrupt )
{
	if (cpu_getiloops() != 0)
		cpunum_set_input_line_and_vector(0, 0, HOLD_LINE, 0xcf);/* RST 08h */
	else
		cpunum_set_input_line_and_vector(0, 0, HOLD_LINE, 0xd7);	/* RST 10h - vblank */
}



static ADDRESS_MAP_START( c1942_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x7fff) AM_ROM
	AM_RANGE(0x8000, 0xbfff) AM_ROMBANK(1)
	AM_RANGE(0xc000, 0xc000) AM_READ(input_port_0_r)	/* IN0 */
	AM_RANGE(0xc001, 0xc001) AM_READ(input_port_1_r)	/* IN1 */
	AM_RANGE(0xc002, 0xc002) AM_READ(input_port_2_r)	/* IN2 */
	AM_RANGE(0xc003, 0xc003) AM_READ(input_port_3_r)	/* DSW0 */
	AM_RANGE(0xc004, 0xc004) AM_READ(input_port_4_r)	/* DSW1 */
	AM_RANGE(0xc800, 0xc800) AM_WRITE(soundlatch_w)
	AM_RANGE(0xc802, 0xc803) AM_WRITE(c1942_scroll_w)
	AM_RANGE(0xc804, 0xc804) AM_WRITE(c1942_c804_w)
	AM_RANGE(0xc805, 0xc805) AM_WRITE(c1942_palette_bank_w)
	AM_RANGE(0xc806, 0xc806) AM_WRITE(c1942_bankswitch_w)
	AM_RANGE(0xcc00, 0xcc7f) AM_RAM AM_BASE(&spriteram) AM_SIZE(&spriteram_size)
	AM_RANGE(0xd000, 0xd7ff) AM_RAM AM_WRITE(c1942_fgvideoram_w) AM_BASE(&c1942_fgvideoram)
	AM_RANGE(0xd800, 0xdbff) AM_RAM AM_WRITE(c1942_bgvideoram_w) AM_BASE(&c1942_bgvideoram)
	AM_RANGE(0xe000, 0xefff) AM_RAM
ADDRESS_MAP_END

static ADDRESS_MAP_START( sound_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x3fff) AM_ROM
	AM_RANGE(0x4000, 0x47ff) AM_RAM
	AM_RANGE(0x6000, 0x6000) AM_READ(soundlatch_r)
	AM_RANGE(0x8000, 0x8000) AM_WRITE(AY8910_control_port_0_w)
	AM_RANGE(0x8001, 0x8001) AM_WRITE(AY8910_write_port_0_w)
	AM_RANGE(0xc000, 0xc000) AM_WRITE(AY8910_control_port_1_w)
	AM_RANGE(0xc001, 0xc001) AM_WRITE(AY8910_write_port_1_w)
ADDRESS_MAP_END


INPUT_PORTS_START( 1942 )
	PORT_START_TAG("IN0")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x0c, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN1 )

	PORT_START_TAG("IN1")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  ) PORT_8WAY
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  ) PORT_8WAY
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    ) PORT_8WAY
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0xc0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START_TAG("IN2")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY PORT_COCKTAIL
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  ) PORT_8WAY PORT_COCKTAIL
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  ) PORT_8WAY PORT_COCKTAIL
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    ) PORT_8WAY PORT_COCKTAIL
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_COCKTAIL
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_COCKTAIL
	PORT_BIT( 0xc0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START_TAG("DSWA")
	PORT_DIPNAME( 0x07, 0x07, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x07, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(    0x06, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x05, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Free_Play ) )
	PORT_DIPNAME( 0x08, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0x30, 0x30, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x30, "20K 80K 80K+" )
	PORT_DIPSETTING(    0x20, "20K 100K 100K+" )
	PORT_DIPSETTING(    0x10, "30K 80K 80K+" )
	PORT_DIPSETTING(    0x00, "30K 100K 100K+" )
	PORT_DIPNAME( 0xc0, 0x40, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x80, "1" )
	PORT_DIPSETTING(    0x40, "2" )
	PORT_DIPSETTING(    0xc0, "3" )
	PORT_DIPSETTING(    0x00, "5" )

	PORT_START_TAG("DSWB")
	PORT_DIPNAME( 0x07, 0x07, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x07, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(    0x06, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x05, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Free_Play ) )
	PORT_SERVICE( 0x08, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x60, 0x60, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Easy ) )
	PORT_DIPSETTING(    0x60, DEF_STR( Normal ) )
	PORT_DIPSETTING(    0x20, "Difficult" )
	PORT_DIPSETTING(    0x00, "Very Difficult" )
	PORT_DIPNAME( 0x80, 0x80, "Freeze" )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
INPUT_PORTS_END



static const gfx_layout charlayout =
{
	8,8,
	RGN_FRAC(1,1),
	2,
	{ 4, 0 },
	{ 0, 1, 2, 3, 8+0, 8+1, 8+2, 8+3 },
	{ 0*16, 1*16, 2*16, 3*16, 4*16, 5*16, 6*16, 7*16 },
	16*8
};

static const gfx_layout tilelayout =
{
	16,16,
	RGN_FRAC(1,3),
	3,
	{ RGN_FRAC(0,3), RGN_FRAC(1,3), RGN_FRAC(2,3) },
	{ 0, 1, 2, 3, 4, 5, 6, 7,
			16*8+0, 16*8+1, 16*8+2, 16*8+3, 16*8+4, 16*8+5, 16*8+6, 16*8+7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8,
			8*8, 9*8, 10*8, 11*8, 12*8, 13*8, 14*8, 15*8 },
	32*8
};

static const gfx_layout spritelayout =
{
	16,16,
	RGN_FRAC(1,2),
	4,
	{ RGN_FRAC(1,2)+4, RGN_FRAC(1,2)+0, 4, 0 },
	{ 0, 1, 2, 3, 8+0, 8+1, 8+2, 8+3,
			16*16+0, 16*16+1, 16*16+2, 16*16+3, 16*16+8+0, 16*16+8+1, 16*16+8+2, 16*16+8+3 },
	{ 0*16, 1*16, 2*16, 3*16, 4*16, 5*16, 6*16, 7*16,
			8*16, 9*16, 10*16, 11*16, 12*16, 13*16, 14*16, 15*16 },
	64*8
};

static const gfx_decode gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &charlayout,             0, 64 },
	{ REGION_GFX2, 0, &tilelayout,          64*4, 4*32 },
	{ REGION_GFX3, 0, &spritelayout, 64*4+4*32*8, 16 },
	{ -1 } /* end of array */
};



static MACHINE_DRIVER_START( 1942 )

	/* basic machine hardware */
	MDRV_CPU_ADD(Z80, 4000000)	/* 4 MHz (?) */
	MDRV_CPU_PROGRAM_MAP(c1942_map, 0)
	MDRV_CPU_VBLANK_INT(c1942_interrupt,2)

	MDRV_CPU_ADD(Z80, 3000000)
	/* audio CPU */	/* 3 MHz ??? */
	MDRV_CPU_PROGRAM_MAP(sound_map, 0)
	MDRV_CPU_VBLANK_INT(irq0_line_hold,4)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(32*8, 32*8)
	MDRV_VISIBLE_AREA(0*8, 32*8-1, 2*8, 30*8-1)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(256)
	MDRV_COLORTABLE_LENGTH(64*4+4*32*8+16*16)

	MDRV_PALETTE_INIT(1942)
	MDRV_VIDEO_START(1942)
	MDRV_VIDEO_UPDATE(1942)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")

	MDRV_SOUND_ADD(AY8910, 1500000)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.25)
	MDRV_SOUND_ADD(AY8910, 1500000)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.25)
MACHINE_DRIVER_END



/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START( 1942 )
	ROM_REGION( 0x1c000, REGION_CPU1, 0 )	/* 64k for code + 3*16k for the banked ROMs images */
	ROM_LOAD( "1-n3a.bin",    0x00000, 0x4000, CRC(40201bab) SHA1(4886c07a4602223c21419118e10aadce9c99fa5a) )
	ROM_LOAD( "1-n4.bin",     0x04000, 0x4000, CRC(a60ac644) SHA1(f37862db3cf5e6cc9ab3276f3bc45fd629fd70dd) )
	ROM_LOAD( "1-n5.bin",     0x10000, 0x4000, CRC(835f7b24) SHA1(24b66827f08c43fbf5b9517d638acdfc38e1b1e7) )
	ROM_LOAD( "1-n6.bin",     0x14000, 0x2000, CRC(821c6481) SHA1(06becb6bf8b4bde3a458098498eecad566a87711) )
	ROM_LOAD( "1-n7.bin",     0x18000, 0x4000, CRC(5df525e1) SHA1(70cd2910e2945db76bd6ebfa0ff09a5efadc2d0b) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* 64k for the audio CPU */
	ROM_LOAD( "1-c11.bin",    0x0000, 0x4000, CRC(bd87f06b) SHA1(821f85cf157f81117eeaba0c3cf0337eac357e58) )

	ROM_REGION( 0x2000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "1-f2.bin",     0x0000, 0x2000, CRC(6ebca191) SHA1(0dbddadde54a0ab66994c4a8726be05c6ca88a0e) )	/* characters */

	ROM_REGION( 0xc000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "2-a1.bin",     0x0000, 0x2000, CRC(3884d9eb) SHA1(5cbd9215fa5ba5a61208b383700adc4428521aed) )	/* tiles */
	ROM_LOAD( "2-a2.bin",     0x2000, 0x2000, CRC(999cf6e0) SHA1(5b8b685038ec98b781908b92eb7fb9506db68544) )
	ROM_LOAD( "2-a3.bin",     0x4000, 0x2000, CRC(8edb273a) SHA1(85fdd4c690ed31e6396e3c16aa02140ee7ea2d61) )
	ROM_LOAD( "2-a4.bin",     0x6000, 0x2000, CRC(3a2726c3) SHA1(187c92ef591febdcbd1d42ab850e0cbb62c00873) )
	ROM_LOAD( "2-a5.bin",     0x8000, 0x2000, CRC(1bd3d8bb) SHA1(ef4dce605eb4dc8035985a415315ec61c21419c6) )
	ROM_LOAD( "2-a6.bin",     0xa000, 0x2000, CRC(658f02c4) SHA1(f087d69e49e38cf3107350cde18fcf85a8fa04f0) )

	ROM_REGION( 0x10000, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_LOAD( "2-l1.bin",     0x00000, 0x4000, CRC(2528bec6) SHA1(29f7719f18faad6bd1ec6735cc24e69168361470) )	/* sprites */
	ROM_LOAD( "2-l2.bin",     0x04000, 0x4000, CRC(f89287aa) SHA1(136fff6d2a4f48a488fc7c620213761459c3ada0) )
	ROM_LOAD( "2-n1.bin",     0x08000, 0x4000, CRC(024418f8) SHA1(145b8d5d6c8654cd090955a98f6dd8c8dbafe7c1) )
	ROM_LOAD( "2-n2.bin",     0x0c000, 0x4000, CRC(e2c7e489) SHA1(d4b5d575c021f58f6966df189df94e08c5b3621c) )

	ROM_REGION( 0x0a00, REGION_PROMS, 0 )
	ROM_LOAD( "08e_sb-5.bin", 0x0000, 0x0100, CRC(93ab8153) SHA1(a792f24e5c0c3c4a6b436102e7a98199f878ece1) )	/* red component */
	ROM_LOAD( "09e_sb-6.bin", 0x0100, 0x0100, CRC(8ab44f7d) SHA1(f74680a6a987d74b3acb32e6396f20e127874149) )	/* green component */
	ROM_LOAD( "10e_sb-7.bin", 0x0200, 0x0100, CRC(f4ade9a4) SHA1(62ad31d31d183cce213b03168daa035083b2f28e) )	/* blue component */
	ROM_LOAD( "f01_sb-0.bin", 0x0300, 0x0100, CRC(6047d91b) SHA1(1ce025f9524c1033e48c5294ee7d360f8bfebe8d) )	/* char lookup table */
	ROM_LOAD( "06d_sb-4.bin", 0x0400, 0x0100, CRC(4858968d) SHA1(20b5dbcaa1a4081b3139e7e2332d8fe3c9e55ed6) )	/* tile lookup table */
	ROM_LOAD( "03k_sb-8.bin", 0x0500, 0x0100, CRC(f6fad943) SHA1(b0a24ea7805272e8ebf72a99b08907bc00d5f82f) )	/* sprite lookup table */
	ROM_LOAD( "01d_sb-2.bin", 0x0600, 0x0100, CRC(8bb8b3df) SHA1(49de2819c4c92057fedcb20425282515d85829aa) )	/* tile palette selector? (not used) */
	ROM_LOAD( "02d_sb-3.bin", 0x0700, 0x0100, CRC(3b0c99af) SHA1(38f30ac1e48632634e409f328ee3051b987de7ad) )	/* tile palette selector? (not used) */
	ROM_LOAD( "k06_sb-1.bin", 0x0800, 0x0100, CRC(712ac508) SHA1(5349d722ab6733afdda65f6e0a98322f0d515e86) )	/* interrupt timing (not used) */
	ROM_LOAD( "01m_sb-9.bin", 0x0900, 0x0100, CRC(4921635c) SHA1(aee37d6cdc36acf0f11ff5f93e7b16e4b12f6c39) )	/* video timing? (not used) */
ROM_END

ROM_START( 1942a )
	ROM_REGION( 0x1c000, REGION_CPU1, 0 )	/* 64k for code + 3*16k for the banked ROMs images */
	ROM_LOAD( "1-n3.bin",     0x00000, 0x4000, CRC(612975f2) SHA1(f3744335862dd4c53925cc32792badd4a378c837) )
	ROM_LOAD( "1-n4.bin",     0x04000, 0x4000, CRC(a60ac644) SHA1(f37862db3cf5e6cc9ab3276f3bc45fd629fd70dd) )
	ROM_LOAD( "1-n5.bin",     0x10000, 0x4000, CRC(835f7b24) SHA1(24b66827f08c43fbf5b9517d638acdfc38e1b1e7) )
	ROM_LOAD( "1-n6.bin",     0x14000, 0x2000, CRC(821c6481) SHA1(06becb6bf8b4bde3a458098498eecad566a87711) )
	ROM_LOAD( "1-n7.bin",     0x18000, 0x4000, CRC(5df525e1) SHA1(70cd2910e2945db76bd6ebfa0ff09a5efadc2d0b) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* 64k for the audio CPU */
	ROM_LOAD( "1-c11.bin",    0x0000, 0x4000, CRC(bd87f06b) SHA1(821f85cf157f81117eeaba0c3cf0337eac357e58) )

	ROM_REGION( 0x2000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "1-f2.bin",     0x0000, 0x2000, CRC(6ebca191) SHA1(0dbddadde54a0ab66994c4a8726be05c6ca88a0e) )	/* characters */

	ROM_REGION( 0xc000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "2-a1.bin",     0x0000, 0x2000, CRC(3884d9eb) SHA1(5cbd9215fa5ba5a61208b383700adc4428521aed) )	/* tiles */
	ROM_LOAD( "2-a2.bin",     0x2000, 0x2000, CRC(999cf6e0) SHA1(5b8b685038ec98b781908b92eb7fb9506db68544) )
	ROM_LOAD( "2-a3.bin",     0x4000, 0x2000, CRC(8edb273a) SHA1(85fdd4c690ed31e6396e3c16aa02140ee7ea2d61) )
	ROM_LOAD( "2-a4.bin",     0x6000, 0x2000, CRC(3a2726c3) SHA1(187c92ef591febdcbd1d42ab850e0cbb62c00873) )
	ROM_LOAD( "2-a5.bin",     0x8000, 0x2000, CRC(1bd3d8bb) SHA1(ef4dce605eb4dc8035985a415315ec61c21419c6) )
	ROM_LOAD( "2-a6.bin",     0xa000, 0x2000, CRC(658f02c4) SHA1(f087d69e49e38cf3107350cde18fcf85a8fa04f0) )

	ROM_REGION( 0x10000, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_LOAD( "2-l1.bin",     0x00000, 0x4000, CRC(2528bec6) SHA1(29f7719f18faad6bd1ec6735cc24e69168361470) )	/* sprites */
	ROM_LOAD( "2-l2.bin",     0x04000, 0x4000, CRC(f89287aa) SHA1(136fff6d2a4f48a488fc7c620213761459c3ada0) )
	ROM_LOAD( "2-n1.bin",     0x08000, 0x4000, CRC(024418f8) SHA1(145b8d5d6c8654cd090955a98f6dd8c8dbafe7c1) )
	ROM_LOAD( "2-n2.bin",     0x0c000, 0x4000, CRC(e2c7e489) SHA1(d4b5d575c021f58f6966df189df94e08c5b3621c) )

	ROM_REGION( 0x0a00, REGION_PROMS, 0 )
	ROM_LOAD( "08e_sb-5.bin", 0x0000, 0x0100, CRC(93ab8153) SHA1(a792f24e5c0c3c4a6b436102e7a98199f878ece1) )	/* red component */
	ROM_LOAD( "09e_sb-6.bin", 0x0100, 0x0100, CRC(8ab44f7d) SHA1(f74680a6a987d74b3acb32e6396f20e127874149) )	/* green component */
	ROM_LOAD( "10e_sb-7.bin", 0x0200, 0x0100, CRC(f4ade9a4) SHA1(62ad31d31d183cce213b03168daa035083b2f28e) )	/* blue component */
	ROM_LOAD( "f01_sb-0.bin", 0x0300, 0x0100, CRC(6047d91b) SHA1(1ce025f9524c1033e48c5294ee7d360f8bfebe8d) )	/* char lookup table */
	ROM_LOAD( "06d_sb-4.bin", 0x0400, 0x0100, CRC(4858968d) SHA1(20b5dbcaa1a4081b3139e7e2332d8fe3c9e55ed6) )	/* tile lookup table */
	ROM_LOAD( "03k_sb-8.bin", 0x0500, 0x0100, CRC(f6fad943) SHA1(b0a24ea7805272e8ebf72a99b08907bc00d5f82f) )	/* sprite lookup table */
	ROM_LOAD( "01d_sb-2.bin", 0x0600, 0x0100, CRC(8bb8b3df) SHA1(49de2819c4c92057fedcb20425282515d85829aa) )	/* tile palette selector? (not used) */
	ROM_LOAD( "02d_sb-3.bin", 0x0700, 0x0100, CRC(3b0c99af) SHA1(38f30ac1e48632634e409f328ee3051b987de7ad) )	/* tile palette selector? (not used) */
	ROM_LOAD( "k06_sb-1.bin", 0x0800, 0x0100, CRC(712ac508) SHA1(5349d722ab6733afdda65f6e0a98322f0d515e86) )	/* interrupt timing (not used) */
	ROM_LOAD( "01m_sb-9.bin", 0x0900, 0x0100, CRC(4921635c) SHA1(aee37d6cdc36acf0f11ff5f93e7b16e4b12f6c39) )	/* video timing? (not used) */
ROM_END

ROM_START( 1942b )
	ROM_REGION( 0x1c000, REGION_CPU1, 0 )	/* 64k for code + 3*16k for the banked ROMs images */
	ROM_LOAD( "srb-03.n3",    0x00000, 0x4000, CRC(d9dafcc3) SHA1(a089a9bc55fb7d6d0ac53f91b258396d5d62677a) )
	ROM_LOAD( "srb-04.n4",    0x04000, 0x4000, CRC(da0cf924) SHA1(856fbb302c9a4ec7850a26ab23dab8467f79bba4) )
	ROM_LOAD( "srb-05.n5",    0x10000, 0x4000, CRC(d102911c) SHA1(35ba1d82bd901940f61d8619273463d02fc0a952) )
	ROM_LOAD( "srb-06.n6",    0x14000, 0x2000, CRC(466f8248) SHA1(2ccc8fc59962d3001fbc10e8d2f20a254a74f251) )
	ROM_LOAD( "srb-07.n7",    0x18000, 0x4000, CRC(0d31038c) SHA1(b588eaf6fddd66ecb2d9832dc197f286f1ccd846) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* 64k for the audio CPU */
	ROM_LOAD( "1-c11.bin",    0x0000, 0x4000, CRC(bd87f06b) SHA1(821f85cf157f81117eeaba0c3cf0337eac357e58) )

	ROM_REGION( 0x2000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "1-f2.bin",     0x0000, 0x2000, CRC(6ebca191) SHA1(0dbddadde54a0ab66994c4a8726be05c6ca88a0e) )	/* characters */

	ROM_REGION( 0xc000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "2-a1.bin",     0x0000, 0x2000, CRC(3884d9eb) SHA1(5cbd9215fa5ba5a61208b383700adc4428521aed) )	/* tiles */
	ROM_LOAD( "2-a2.bin",     0x2000, 0x2000, CRC(999cf6e0) SHA1(5b8b685038ec98b781908b92eb7fb9506db68544) )
	ROM_LOAD( "2-a3.bin",     0x4000, 0x2000, CRC(8edb273a) SHA1(85fdd4c690ed31e6396e3c16aa02140ee7ea2d61) )
	ROM_LOAD( "2-a4.bin",     0x6000, 0x2000, CRC(3a2726c3) SHA1(187c92ef591febdcbd1d42ab850e0cbb62c00873) )
	ROM_LOAD( "2-a5.bin",     0x8000, 0x2000, CRC(1bd3d8bb) SHA1(ef4dce605eb4dc8035985a415315ec61c21419c6) )
	ROM_LOAD( "2-a6.bin",     0xa000, 0x2000, CRC(658f02c4) SHA1(f087d69e49e38cf3107350cde18fcf85a8fa04f0) )

	ROM_REGION( 0x10000, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_LOAD( "2-l1.bin",     0x00000, 0x4000, CRC(2528bec6) SHA1(29f7719f18faad6bd1ec6735cc24e69168361470) )	/* sprites */
	ROM_LOAD( "2-l2.bin",     0x04000, 0x4000, CRC(f89287aa) SHA1(136fff6d2a4f48a488fc7c620213761459c3ada0) )
	ROM_LOAD( "2-n1.bin",     0x08000, 0x4000, CRC(024418f8) SHA1(145b8d5d6c8654cd090955a98f6dd8c8dbafe7c1) )
	ROM_LOAD( "2-n2.bin",     0x0c000, 0x4000, CRC(e2c7e489) SHA1(d4b5d575c021f58f6966df189df94e08c5b3621c) )

	ROM_REGION( 0x0a00, REGION_PROMS, 0 )
	ROM_LOAD( "08e_sb-5.bin", 0x0000, 0x0100, CRC(93ab8153) SHA1(a792f24e5c0c3c4a6b436102e7a98199f878ece1) )	/* red component */
	ROM_LOAD( "09e_sb-6.bin", 0x0100, 0x0100, CRC(8ab44f7d) SHA1(f74680a6a987d74b3acb32e6396f20e127874149) )	/* green component */
	ROM_LOAD( "10e_sb-7.bin", 0x0200, 0x0100, CRC(f4ade9a4) SHA1(62ad31d31d183cce213b03168daa035083b2f28e) )	/* blue component */
	ROM_LOAD( "f01_sb-0.bin", 0x0300, 0x0100, CRC(6047d91b) SHA1(1ce025f9524c1033e48c5294ee7d360f8bfebe8d) )	/* char lookup table */
	ROM_LOAD( "06d_sb-4.bin", 0x0400, 0x0100, CRC(4858968d) SHA1(20b5dbcaa1a4081b3139e7e2332d8fe3c9e55ed6) )	/* tile lookup table */
	ROM_LOAD( "03k_sb-8.bin", 0x0500, 0x0100, CRC(f6fad943) SHA1(b0a24ea7805272e8ebf72a99b08907bc00d5f82f) )	/* sprite lookup table */
	ROM_LOAD( "01d_sb-2.bin", 0x0600, 0x0100, CRC(8bb8b3df) SHA1(49de2819c4c92057fedcb20425282515d85829aa) )	/* tile palette selector? (not used) */
	ROM_LOAD( "02d_sb-3.bin", 0x0700, 0x0100, CRC(3b0c99af) SHA1(38f30ac1e48632634e409f328ee3051b987de7ad) )	/* tile palette selector? (not used) */
	ROM_LOAD( "k06_sb-1.bin", 0x0800, 0x0100, CRC(712ac508) SHA1(5349d722ab6733afdda65f6e0a98322f0d515e86) )	/* interrupt timing (not used) */
	ROM_LOAD( "01m_sb-9.bin", 0x0900, 0x0100, CRC(4921635c) SHA1(aee37d6cdc36acf0f11ff5f93e7b16e4b12f6c39) )	/* video timing? (not used) */
ROM_END


DRIVER_INIT( 1942 )
{
	UINT8 *ROM = memory_region(REGION_CPU1);
	memory_configure_bank(1, 0, 3, &ROM[0x10000], 0x4000);
}


GAME( 1984, 1942,  0,    1942, 1942, 1942, ROT270, "Capcom", "1942 (set 1)", 0 )
GAME( 1984, 1942a, 1942, 1942, 1942, 1942, ROT270, "Capcom", "1942 (set 2)", 0 )
GAME( 1984, 1942b, 1942, 1942, 1942, 1942, ROT270, "Capcom", "1942 (set 3)", 0 )
