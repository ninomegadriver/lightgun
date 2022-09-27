/* Aquarium (c)1996 Excellent Systems */

/* the hardware is similar to gcpinbal.c, probably should merge it at some point */

/*

AQUARIUM
EXCELLENT SYSTEMS
ES-9206
                                   3
                      14.318MHz                 7
       ES 9207
                                        8
       ES 9303
                                       ES 9208  2

AQUARF1              68000-16            6

                                                 SW1
   YM2151  M6295  4  32MHz               1
                     Z80-6  5                    SW2Q

*/

/* To Do (top are higher priorities)

Controls + Dipswitches *done* - stephh
Verify Z80 program banking
Fix Priority Problems *done* - Pierpaolo Prazzoli
Merge with gcpinbal.c (and clean up gcpinbal.c)


Note, a bug in the program code causes the OKI to be reset on the very
first coin inserted.


Stephh's notes (based on the game M68000 code and some tests) :

  - The current set is a Japan set (0x0a5c.w = 0x0000), so there must exist
    a non-Japan set which is AFAIK not dumped at the moment.
    I have however no clue about what should be the value in the non-Japan set.
  - Use AQUARIUS_HACK to be able to change the game to English language
    via the FAKE Dip Switch.
    Change the Dip Switch then reset the game to make the changes effective.
  - I haven't found a way to enter sort of "test mode", even if there seems
    to be code (or at least data) for it.


*/


#include "driver.h"
#include "sound/2151intf.h"
#include "sound/okim6295.h"

#define AQUARIUS_HACK	0

static int aquarium_snd_ack;

UINT16 *aquarium_scroll, *aquarium_priority;
UINT16 *aquarium_txt_videoram;
UINT16 *aquarium_mid_videoram;
UINT16 *aquarium_bak_videoram;

WRITE16_HANDLER( aquarium_txt_videoram_w );
WRITE16_HANDLER( aquarium_mid_videoram_w );
WRITE16_HANDLER( aquarium_bak_videoram_w );

VIDEO_START(aquarium);
VIDEO_UPDATE(aquarium);

#if AQUARIUS_HACK
static MACHINE_RESET( aquarium )
{
	UINT16 *RAM = (UINT16 *)memory_region(REGION_CPU1);
	int data = readinputportbytag("FAKE");

	/* Language : 0x0000 = Japanese - Other value = English */

	RAM[0x000a5c/2] = data;

}
#endif

static READ16_HANDLER( aquarium_coins_r )
{
	int data;
	data = (input_port_2_word_r(0,mem_mask) & 0x7fff);	/* IN1 */
	data |= aquarium_snd_ack;
	aquarium_snd_ack = 0;
	return data;
}

static WRITE8_HANDLER( aquarium_snd_ack_w )
{
	aquarium_snd_ack = 0x8000;
}

static WRITE16_HANDLER( aquarium_sound_w )
{
//  ui_popup("sound write %04x",data);

	soundlatch_w(1,data&0xff);
	cpunum_set_input_line( 1, INPUT_LINE_NMI, PULSE_LINE );
}

static WRITE8_HANDLER( aquarium_z80_bank_w )
{
	int soundbank = ((data & 0x7) + 1) * 0x8000;
	UINT8 *Z80 = (UINT8 *)memory_region(REGION_CPU2);

	memory_set_bankptr(1, &Z80[soundbank + 0x10000]);
}

static UINT8 aquarium_snd_bitswap(UINT8 scrambled_data)
{
	UINT8 data = 0;

	data |= ((scrambled_data & 0x01) << 7);
	data |= ((scrambled_data & 0x02) << 5);
	data |= ((scrambled_data & 0x04) << 3);
	data |= ((scrambled_data & 0x08) << 1);
	data |= ((scrambled_data & 0x10) >> 1);
	data |= ((scrambled_data & 0x20) >> 3);
	data |= ((scrambled_data & 0x40) >> 5);
	data |= ((scrambled_data & 0x80) >> 7);

	return data;
}

static READ8_HANDLER( aquarium_oki_r )
{
	return (aquarium_snd_bitswap(OKIM6295_status_0_r(0)) );
}

static WRITE8_HANDLER( aquarium_oki_w )
{
	logerror("Z80-PC:%04x Writing %04x to the OKI M6295\n",activecpu_get_previouspc(),aquarium_snd_bitswap(data));
	OKIM6295_data_0_w( 0, (aquarium_snd_bitswap(data)) );
}




static ADDRESS_MAP_START( readmem, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x07ffff) AM_READ(MRA16_ROM)
	AM_RANGE(0xc00000, 0xc03fff) AM_READ(MRA16_RAM)
	AM_RANGE(0xc80000, 0xc81fff) AM_READ(MRA16_RAM)	/* sprite ram */
	AM_RANGE(0xd00000, 0xd00fff) AM_READ(MRA16_RAM)
	AM_RANGE(0xd80080, 0xd80081) AM_READ(input_port_0_word_r)
	AM_RANGE(0xd80082, 0xd80083) AM_READ(MRA16_NOP)	/* stored but not read back ? check code at 0x01f440 */
	AM_RANGE(0xd80084, 0xd80085) AM_READ(input_port_1_word_r)
	AM_RANGE(0xd80086, 0xd80087) AM_READ(aquarium_coins_r)
	AM_RANGE(0xff0000, 0xffffff) AM_READ(MRA16_RAM)	/* RAM */
ADDRESS_MAP_END

static ADDRESS_MAP_START( writemem, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x07ffff) AM_WRITE(MWA16_ROM)
	AM_RANGE(0xc00000, 0xc00fff) AM_WRITE(aquarium_mid_videoram_w) AM_BASE(&aquarium_mid_videoram)
	AM_RANGE(0xc01000, 0xc01fff) AM_WRITE(aquarium_bak_videoram_w) AM_BASE(&aquarium_bak_videoram)
	AM_RANGE(0xc02000, 0xc03fff) AM_WRITE(aquarium_txt_videoram_w) AM_BASE(&aquarium_txt_videoram)
	AM_RANGE(0xc80000, 0xc81fff) AM_WRITE(MWA16_RAM) AM_BASE(&spriteram16) AM_SIZE(&spriteram_size)
	AM_RANGE(0xd00000, 0xd00fff) AM_WRITE(paletteram16_RRRRGGGGBBBBRGBx_word_w) AM_BASE(&paletteram16)
	AM_RANGE(0xd80014, 0xd8001f) AM_WRITE(MWA16_RAM) AM_BASE(&aquarium_scroll)
	AM_RANGE(0xd80068, 0xd80069) AM_WRITENOP  /* probably not used */
	AM_RANGE(0xd80088, 0xd80089) AM_WRITENOP /* ?? video related */
	AM_RANGE(0xd8008a, 0xd8008b) AM_WRITE(aquarium_sound_w)
	AM_RANGE(0xff0000, 0xffffff) AM_WRITE(MWA16_RAM)
ADDRESS_MAP_END

static ADDRESS_MAP_START( snd_readmem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x3fff) AM_READ(MRA8_ROM)
	AM_RANGE(0x7800, 0x7fff) AM_READ(MRA8_RAM)
	AM_RANGE(0x8000, 0xffff) AM_READ(MRA8_BANK1)
ADDRESS_MAP_END

static ADDRESS_MAP_START( snd_writemem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x3fff) AM_WRITE(MWA8_ROM)
	AM_RANGE(0x7800, 0x7fff) AM_WRITE(MWA8_RAM)
ADDRESS_MAP_END

static ADDRESS_MAP_START( snd_readport, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_FLAGS( AMEF_ABITS(8) )
	AM_RANGE(0x01, 0x01) AM_READ(YM2151_status_port_0_r)
	AM_RANGE(0x02, 0x02) AM_READ(aquarium_oki_r)
	AM_RANGE(0x04, 0x04) AM_READ(soundlatch_r)
ADDRESS_MAP_END

static ADDRESS_MAP_START( snd_writeport, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_FLAGS( AMEF_ABITS(8) )
	AM_RANGE(0x00, 0x00) AM_WRITE(YM2151_register_port_0_w)
	AM_RANGE(0x01, 0x01) AM_WRITE(YM2151_data_port_0_w)
	AM_RANGE(0x02, 0x02) AM_WRITE(aquarium_oki_w)
	AM_RANGE(0x06, 0x06) AM_WRITE(aquarium_snd_ack_w)
	AM_RANGE(0x08, 0x08) AM_WRITE(aquarium_z80_bank_w)
ADDRESS_MAP_END

INPUT_PORTS_START( aquarium )
	PORT_START_TAG("DSW")
	PORT_DIPNAME( 0x0003, 0x0003, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(      0x0002, DEF_STR( Easy ) )
	PORT_DIPSETTING(      0x0003, DEF_STR( Normal ) )
	PORT_DIPSETTING(      0x0001, DEF_STR( Hard ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( Hardest ) )
	PORT_DIPNAME( 0x000c, 0x000c, "Winning Rounds (Player VS CPU)" )
	PORT_DIPSETTING(      0x000c, "1/1" )
	PORT_DIPSETTING(      0x0008, "2/3" )
	PORT_DIPSETTING(      0x0004, "3/5" )
//  PORT_DIPSETTING(      0x0000, "1/1" )
	PORT_DIPNAME( 0x0030, 0x0030, "Winning Rounds (Player VS Player)" )
	PORT_DIPSETTING(      0x0030, "1/1" )
	PORT_DIPSETTING(      0x0020, "2/3" )
	PORT_DIPSETTING(      0x0010, "3/5" )
//  PORT_DIPSETTING(      0x0000, "1/1" )
	PORT_DIPNAME( 0x0040, 0x0040, DEF_STR( Unused ) )
	PORT_DIPSETTING(      0x0040, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0080, 0x0080, DEF_STR( Unused ) )
	PORT_DIPSETTING(      0x0080, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0700, 0x0700, DEF_STR( Coinage ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( 5C_1C ) )
	PORT_DIPSETTING(      0x0100, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(      0x0200, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(      0x0300, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0x0700, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x0600, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(      0x0500, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(      0x0400, DEF_STR( 1C_4C ) )
	PORT_DIPNAME( 0x0800, 0x0800, DEF_STR( Unused ) )
	PORT_DIPSETTING(      0x0800, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x1000, 0x1000, DEF_STR( Flip_Screen ) )	// to be confirmed - code at 0x01f82c
	PORT_DIPSETTING(      0x1000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x2000, 0x2000, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x2000, DEF_STR( On ) )
	PORT_DIPNAME( 0x4000, 0x4000, DEF_STR( Unused ) )
	PORT_DIPSETTING(      0x4000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x8000, 0x8000, DEF_STR( Unused ) )
	PORT_DIPSETTING(      0x8000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )

	PORT_START_TAG("IN0")
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(2)
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(2)
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(1)
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(1)
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_START1 )

	PORT_START_TAG("IN1")
	PORT_BIT( 0x00ff, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* untested */
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* sound status */

#if AQUARIUS_HACK
	PORT_START_TAG("FAKE")	/* FAKE DSW to support language */
	PORT_DIPNAME( 0xffff, 0x0001, DEF_STR( Language ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( Japanese ) )
	PORT_DIPSETTING(      0x0001, DEF_STR( English ) )		// This is a guess of what should be the value
#endif
INPUT_PORTS_END

static const gfx_layout char5bpplayout =
{
	16,16,	/* 16*16 characters */
	RGN_FRAC(1,2),
	5,	/* 4 bits per pixel */
	{  RGN_FRAC(1,2), 0, 1, 2, 3 },
	{ 2*4, 3*4, 0*4, 1*4, 6*4, 7*4, 4*4, 5*4, 2*4+32, 3*4+32, 0*4+32, 1*4+32, 6*4+32, 7*4+32, 4*4+32, 5*4+32 },
	{ 0*64, 1*64, 2*64, 3*64, 4*64, 5*64, 6*64, 7*64, 8*64, 9*64, 10*64, 11*64, 12*64, 13*64, 14*64, 15*64 },
	128*8	/* every sprite takes 128 consecutive bytes */
};

static const gfx_layout char_8x8_layout =
{
	8,8,	/* 8*8 characters */
	RGN_FRAC(1,1),
	4,	/* 4 bits per pixel */
	{ 0, 1, 2, 3 },
	{ 2*4, 3*4, 0*4, 1*4, 6*4, 7*4, 4*4, 5*4 },
	{ 0*32, 1*32, 2*32, 3*32, 4*32, 5*32, 6*32, 7*32 },
	32*8	/* every sprite takes 32 consecutive bytes */
};

static const gfx_layout tilelayout =
{
	16,16,	/* 16*16 sprites */
	RGN_FRAC(1,1),
	4,	/* 4 bits per pixel */
	{ 48, 16, 32, 0 },
	{ 8, 9, 10, 11, 12, 13, 14, 15, 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*64, 1*64, 2*64, 3*64, 4*64, 5*64, 6*64, 7*64, 8*64, 9*64, 10*64, 11*64, 12*64, 13*64, 14*64, 15*64 },
	128*8	/* every sprite takes 128 consecutive bytes */
};

static DRIVER_INIT( aquarium )
{
	/* The BG tiles are 5bpp, this rearranges the data from
       the roms containing the 1bpp data so we can decode it
       correctly */

	UINT8 *DAT2 = memory_region(REGION_GFX1)+0x080000;
	UINT8 *DAT = memory_region(REGION_USER1);
	int len = 0x0200000;

	for (len = 0 ; len < 0x020000 ; len ++ )
	{
		DAT2[len*4+1] =  (DAT[len] & 0x80) << 0;
		DAT2[len*4+1] |= (DAT[len] & 0x40) >> 3;
		DAT2[len*4+0] =  (DAT[len] & 0x20) << 2;
		DAT2[len*4+0] |= (DAT[len] & 0x10) >> 1;
		DAT2[len*4+3] =  (DAT[len] & 0x08) << 4;
		DAT2[len*4+3] |= (DAT[len] & 0x04) << 1;
		DAT2[len*4+2] =  (DAT[len] & 0x02) << 6;
		DAT2[len*4+2] |= (DAT[len] & 0x01) << 3;
	}

	DAT2 = memory_region(REGION_GFX4)+0x080000;
	DAT = memory_region(REGION_USER2);

	for (len = 0 ; len < 0x020000 ; len ++ )
	{
		DAT2[len*4+1] =  (DAT[len] & 0x80) << 0;
		DAT2[len*4+1] |= (DAT[len] & 0x40) >> 3;
		DAT2[len*4+0] =  (DAT[len] & 0x20) << 2;
		DAT2[len*4+0] |= (DAT[len] & 0x10) >> 1;
		DAT2[len*4+3] =  (DAT[len] & 0x08) << 4;
		DAT2[len*4+3] |= (DAT[len] & 0x04) << 1;
		DAT2[len*4+2] =  (DAT[len] & 0x02) << 6;
		DAT2[len*4+2] |= (DAT[len] & 0x01) << 3;
	}

	/* reset the sound bank */
	aquarium_z80_bank_w(0, 0);
}


static const gfx_decode gfxdecodeinfo[] =
{
	{ REGION_GFX3, 0, &tilelayout,       0x300, 32 },
	{ REGION_GFX1, 0, &char5bpplayout,   0x400, 32 },
	{ REGION_GFX2, 0, &char_8x8_layout,  0x200, 32 },
	{ REGION_GFX4, 0, &char5bpplayout,   0x400, 32 },
	{ -1 } /* end of array */
};

static void irq_handler(int irq)
{
	cpunum_set_input_line( 1, 0 , irq ? ASSERT_LINE : CLEAR_LINE );
}

static struct YM2151interface ym2151_interface =
{
	irq_handler
};


static MACHINE_DRIVER_START( aquarium )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000, 32000000/2)
	MDRV_CPU_PROGRAM_MAP(readmem,writemem)
	MDRV_CPU_VBLANK_INT(irq1_line_hold,1)

	MDRV_CPU_ADD(Z80, 6000000)
	/* audio CPU */
	MDRV_CPU_PROGRAM_MAP(snd_readmem,snd_writemem)
	MDRV_CPU_IO_MAP(snd_readport,snd_writeport)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

#if AQUARIUS_HACK
	MDRV_MACHINE_RESET(aquarium)
#endif

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(64*8, 64*8)
	MDRV_VISIBLE_AREA(2*8, 42*8-1, 2*8, 34*8-1)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(0x1000/2)

	MDRV_VIDEO_START(aquarium)
	MDRV_VIDEO_UPDATE(aquarium)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_STEREO("left", "right")

	MDRV_SOUND_ADD(YM2151, 3579545)
	MDRV_SOUND_CONFIG(ym2151_interface)
	MDRV_SOUND_ROUTE(0, "left", 0.45)
	MDRV_SOUND_ROUTE(1, "right", 0.45)

	MDRV_SOUND_ADD(OKIM6295, 8500)
	MDRV_SOUND_CONFIG(okim6295_interface_region_1)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "left", 0.47)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "right", 0.47)
MACHINE_DRIVER_END

ROM_START( aquarium )
	ROM_REGION( 0x080000, REGION_CPU1, 0 )     /* 68000 code */
	ROM_LOAD16_WORD_SWAP( "aquar3",  0x000000, 0x080000, CRC(344509a1) SHA1(9deb610732dee5066b3225cd7b1929b767579235) )

	ROM_REGION( 0x50000, REGION_CPU2, 0 ) /* z80 (sound) code */
	ROM_LOAD( "aquar5",  0x000000, 0x40000, CRC(fa555be1) SHA1(07236f2b2ba67e92984b9ddf4a8154221d535245) )
	ROM_RELOAD( 		0x010000, 0x40000 )

	ROM_REGION( 0x100000, REGION_GFX1, ROMREGION_DISPOSE ) /* BG Tiles */
	ROM_LOAD( "aquar1",      0x000000, 0x080000, CRC(575df6ac) SHA1(071394273e512666fe124facdd8591a767ad0819) ) // 4bpp
	/* data is expanded here from USER1 */
	ROM_REGION( 0x100000, REGION_USER1, ROMREGION_DISPOSE ) /* BG Tiles */
	ROM_LOAD( "aquar6",      0x000000, 0x020000, CRC(9065b146) SHA1(befc218bbcd63453ea7eb8f976796d36f2b2d552) ) // 1bpp

	ROM_REGION( 0x100000, REGION_GFX4, ROMREGION_DISPOSE ) /* BG Tiles */
	ROM_LOAD( "aquar8",      0x000000, 0x080000, CRC(915520c4) SHA1(308207cb20f1ed6df365710c808644a6e4f07614) ) // 4bpp
	/* data is expanded here from USER2 */
	ROM_REGION( 0x100000, REGION_USER2, ROMREGION_DISPOSE ) /* BG Tiles */
	ROM_LOAD( "aquar7",      0x000000, 0x020000, CRC(b96b2b82) SHA1(2b719d0c185d1eca4cd9ea66bed7842b74062288) ) // 1bpp

	ROM_REGION( 0x060000, REGION_GFX2, ROMREGION_DISPOSE ) /* FG Tiles */
	ROM_LOAD( "aquar2",   0x000000, 0x020000, CRC(aa071b05) SHA1(517415bfd8e4dd51c6eb03a25c706f8613d34a09) )

	ROM_REGION( 0x200000, REGION_GFX3, ROMREGION_DISPOSE ) /* Sprites? */
	ROM_LOAD( "aquarf1",     0x000000, 0x0100000, CRC(14758b3c) SHA1(b372ccb42acb55a3dd15352a9d4ed576878a6731) )

	ROM_REGION( 0x100000, REGION_SOUND1, 0 ) /* Samples */
	ROM_LOAD( "aquar4",  0x000000, 0x80000, CRC(9a4af531) SHA1(bb201b7a6c9fd5924a0d79090257efffd8d4aba1) )
ROM_END

#if !AQUARIUS_HACK
GAME( 1996, aquarium, 0, aquarium, aquarium, aquarium, ROT0, "Excellent System", "Aquarium (Japan)", GAME_NO_COCKTAIL )
#else
GAME( 1996, aquarium, 0, aquarium, aquarium, aquarium, ROT0, "Excellent System", "Aquarium", GAME_NO_COCKTAIL )
#endif
