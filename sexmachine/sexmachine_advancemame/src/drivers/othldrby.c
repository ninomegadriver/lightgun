/***************************************************************************

Othello Derby

driver by Nicola Salmoria

Video IC is S951060-VGP

Notes:
- Sprite/tile priorities are NOT orthogonal to sprite/sprite priorities:
  sprites with a higher priority appear over sprites with a lower priority,
  regardless of their order in the sprite list. Therefore, the current
  implementation is correct.

***************************************************************************/

#include "driver.h"
#include "sound/okim6295.h"
#include <time.h>


WRITE16_HANDLER( othldrby_videoram_addr_w );
READ16_HANDLER( othldrby_videoram_r );
WRITE16_HANDLER( othldrby_videoram_w );
WRITE16_HANDLER( othldrby_vreg_addr_w );
WRITE16_HANDLER( othldrby_vreg_w );

VIDEO_START( othldrby );
VIDEO_EOF( othldrby );
VIDEO_UPDATE( othldrby );



static READ16_HANDLER( pip )
{
	static int toggle = 0xff;
	return toggle ^= 1;
}

static READ16_HANDLER( pap )
{
	return rand();
}


static WRITE16_HANDLER( oki_bankswitch_w )
{
	if (ACCESSING_LSB)
		OKIM6295_set_bank_base(0, (data & 1) * 0x40000);
}

static WRITE16_HANDLER( coinctrl_w )
{
	if (ACCESSING_LSB)
	{
		coin_counter_w(0,data & 1);
		coin_counter_w(1,data & 2);
		coin_lockout_w(0,~data & 4);
		coin_lockout_w(1,~data & 8);
	}
}

static WRITE16_HANDLER( calendar_w )
{
}

static READ16_HANDLER( calendar_r )
{
	time_t ltime;
	struct tm *today;


	time(&ltime);
	today = localtime(&ltime);

	switch (offset)
	{
		case 0:
			return ((today->tm_sec/10)<<4) + (today->tm_sec%10);
		case 1:
			return ((today->tm_min/10)<<4) + (today->tm_min%10);
		case 2:
			return ((today->tm_hour/10)<<4) + (today->tm_hour%10);
		case 3:
			return today->tm_wday;
		case 4:
			return ((today->tm_mday/10)<<4) + (today->tm_mday%10);
		case 5:
			return (today->tm_mon + 1);
		case 6:
			return (((today->tm_year%100)/10)<<4) + (today->tm_year%10);
		case 7:
		default:
			return 0;	/* status? the other registers are read only when bit 0 is clear */
	}
}



static ADDRESS_MAP_START( readmem, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x07ffff) AM_READ(MRA16_ROM)
	AM_RANGE(0x100000, 0x10ffff) AM_READ(MRA16_RAM)
	AM_RANGE(0x200000, 0x20000f) AM_READ(calendar_r)
	AM_RANGE(0x300004, 0x300007) AM_READ(othldrby_videoram_r)
	AM_RANGE(0x30000c, 0x30000d) AM_READ(pip)	// vblank?
	AM_RANGE(0x400000, 0x400fff) AM_READ(paletteram16_word_r)
	AM_RANGE(0x600000, 0x600001) AM_READ(OKIM6295_status_0_lsb_r)
	AM_RANGE(0x700000, 0x700001) AM_READ(pap)	// scanline???
	AM_RANGE(0x700004, 0x700005) AM_READ(input_port_0_word_r)
	AM_RANGE(0x700008, 0x700009) AM_READ(input_port_1_word_r)
	AM_RANGE(0x70000c, 0x70000d) AM_READ(input_port_2_word_r)
	AM_RANGE(0x700010, 0x700011) AM_READ(input_port_3_word_r)
	AM_RANGE(0x70001c, 0x70001d) AM_READ(input_port_4_word_r)
ADDRESS_MAP_END

static ADDRESS_MAP_START( writemem, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x07ffff) AM_WRITE(MWA16_ROM)
	AM_RANGE(0x100000, 0x10ffff) AM_WRITE(MWA16_RAM)
	AM_RANGE(0x200000, 0x20000f) AM_WRITE(calendar_w)
	AM_RANGE(0x300000, 0x300001) AM_WRITE(othldrby_videoram_addr_w)
	AM_RANGE(0x300004, 0x300007) AM_WRITE(othldrby_videoram_w)
	AM_RANGE(0x300008, 0x300009) AM_WRITE(othldrby_vreg_addr_w)
	AM_RANGE(0x30000c, 0x30000f) AM_WRITE(othldrby_vreg_w)
	AM_RANGE(0x400000, 0x400fff) AM_WRITE(paletteram16_xBBBBBGGGGGRRRRR_word_w) AM_BASE(&paletteram16)
	AM_RANGE(0x600000, 0x600001) AM_WRITE(OKIM6295_data_0_lsb_w)
	AM_RANGE(0x700030, 0x700031) AM_WRITE(oki_bankswitch_w)
	AM_RANGE(0x700034, 0x700035) AM_WRITE(coinctrl_w)
ADDRESS_MAP_END



INPUT_PORTS_START( othldrby )
	PORT_START
	PORT_DIPNAME( 0x01, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x00, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x02, DEF_STR( On ) )
	PORT_SERVICE( 0x04, IP_ACTIVE_HIGH )
	PORT_DIPNAME( 0x08, 0x00, DEF_STR( Allow_Continue ) )
	PORT_DIPSETTING(    0x08, DEF_STR( No ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x30, 0x00, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x30, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x20, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 1C_2C ) )
	PORT_DIPNAME( 0xc0, 0x00, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0xc0, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x80, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x40, DEF_STR( 1C_2C ) )

	PORT_START
	PORT_DIPNAME( 0x03, 0x00, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Easy ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Normal ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Hard ) )
	PORT_DIPSETTING(    0x03, DEF_STR( Very_Hard ) )
	PORT_DIPNAME( 0x04, 0x00, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x04, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x00, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x08, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x00, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x10, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x00, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x20, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x00, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x40, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x00, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x80, DEF_STR( On ) )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 ) PORT_PLAYER(1)
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_BUTTON2 ) PORT_PLAYER(1)
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_BUTTON3 ) PORT_PLAYER(1)
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 ) PORT_PLAYER(2)
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_BUTTON2 ) PORT_PLAYER(2)
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_BUTTON3 ) PORT_PLAYER(2)
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_SERVICE1 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_SERVICE2 )	/* TEST */
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_COIN1 ) PORT_IMPULSE(1)
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_COIN2 ) PORT_IMPULSE(1)
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )
INPUT_PORTS_END



static const gfx_layout spritelayout =
{
	8,8,
	RGN_FRAC(1,2),
	4,
	{ RGN_FRAC(1,2)+8, RGN_FRAC(1,2)+0, RGN_FRAC(0,2)+8, RGN_FRAC(0,2)+0 },
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*16, 1*16, 2*16, 3*16, 4*16, 5*16, 6*16, 7*16 },
	16*8
};

static const gfx_layout tilelayout =
{
	16,16,
	RGN_FRAC(1,2),
	4,
	{ RGN_FRAC(1,2)+8, RGN_FRAC(1,2)+0, RGN_FRAC(0,2)+8, RGN_FRAC(0,2)+0 },
	{ 0, 1, 2, 3, 4, 5, 6, 7,
			16*8+0, 16*8+1, 16*8+2, 16*8+3, 16*8+4, 16*8+5, 16*8+6, 16*8+7 },
	{ 0*16, 1*16, 2*16, 3*16, 4*16, 5*16, 6*16, 7*16,
			16*16, 17*16, 18*16, 19*16, 20*16, 21*16, 22*16, 23*16 },
	16*32
};

static const gfx_decode gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &spritelayout, 0, 0x80 },
	{ REGION_GFX1, 0, &tilelayout,   0, 0x80 },
	{ -1 } /* end of array */
};



static MACHINE_DRIVER_START( othldrby )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000, 16000000)
	MDRV_CPU_PROGRAM_MAP(readmem,writemem)
	MDRV_CPU_VBLANK_INT(irq4_line_hold,1)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(64*8, 32*8)
	MDRV_VISIBLE_AREA(12*8, (64-12)*8-1, 1*8, 31*8-1 )
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(0x800)

	MDRV_VIDEO_START(othldrby)
	MDRV_VIDEO_EOF(othldrby)
	MDRV_VIDEO_UPDATE(othldrby)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")

	MDRV_SOUND_ADD(OKIM6295, 12000)
	MDRV_SOUND_CONFIG(okim6295_interface_region_1)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.0)
MACHINE_DRIVER_END



/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START( othldrby )
	ROM_REGION( 0x080000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "db0.1",        0x00000, 0x80000, CRC(6b4008d3) SHA1(4cf838c47563ba482be8364b2e115569a4a06c83) )

	ROM_REGION( 0x400000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "db0-r2",       0x000000, 0x200000, CRC(4efff265) SHA1(4cd239ff42f532495946cb52bd1fee412f84e192) )
	ROM_LOAD( "db0-r3",       0x200000, 0x200000, CRC(5c142b38) SHA1(5466a8b061a0f2545493de0f96fd4387beea276a) )

	ROM_REGION( 0x080000, REGION_SOUND1, 0 )	/* OKIM6295 samples */
	ROM_LOAD( "db0.4",        0x00000, 0x80000, CRC(a9701868) SHA1(9ee89556666d358e8d3915622573b3ba660048b8) )
ROM_END


GAME( 1995, othldrby, 0, othldrby, othldrby, 0, ROT0, "Sunwise", "Othello Derby (Japan)", 0 )
