/****************************************************************************************

Competition Golf Final Round (c) 1986 / 1985 Data East

Driver by Angelo Salese, Bryan McPhail and Pierpaolo Prazzoli
Thanks to David Haywood for the bg roms expansion

Nb:  The black border around the player sprite in attract mode happens on the real pcb
as well.

****************************************************************************************/

#include "driver.h"
#include "sound/2203intf.h"

extern UINT8 *compgolf_bg_ram;
extern int compgolf_scrollx_lo, compgolf_scrolly_lo, compgolf_scrollx_hi, compgolf_scrolly_hi;

extern WRITE8_HANDLER( compgolf_video_w );
extern WRITE8_HANDLER( compgolf_back_w );
extern PALETTE_INIT ( compgolf );
extern VIDEO_START  ( compgolf );
extern VIDEO_UPDATE ( compgolf );

static WRITE8_HANDLER( compgolf_scrollx_lo_w )
{
	compgolf_scrollx_lo = data;
}

static WRITE8_HANDLER( compgolf_scrolly_lo_w )
{
	compgolf_scrolly_lo = data;
}

static WRITE8_HANDLER( compgolf_ctrl_w )
{
	/* bit 4 and 6 are always set */

	static int bank = -1;
	int new_bank = (data & 4) >> 2;

	if( bank != new_bank )
	{
		bank = new_bank;
		memory_set_bankptr(1, memory_region(REGION_USER1) + 0x4000 * bank);
	}

	compgolf_scrollx_hi = (data & 1) << 8;
	compgolf_scrolly_hi = (data & 2) << 7;
}

static ADDRESS_MAP_START( readmem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x07ff) AM_READ(MRA8_RAM)
	AM_RANGE(0x1000, 0x17ff) AM_READ(MRA8_RAM)
	AM_RANGE(0x1800, 0x1fff) AM_READ(MRA8_RAM)
	AM_RANGE(0x2000, 0x2060) AM_READ(MRA8_RAM)
	AM_RANGE(0x3000, 0x3000) AM_READ(input_port_0_r) //player 1 + start buttons
	AM_RANGE(0x3001, 0x3001) AM_READ(input_port_1_r) //player 2 + vblank
	AM_RANGE(0x3002, 0x3002) AM_READ(input_port_2_r) //dip-switches
	AM_RANGE(0x3003, 0x3003) AM_READ(input_port_3_r) //coins
	AM_RANGE(0x3800, 0x3800) AM_READ(YM2203_status_port_0_r)
	AM_RANGE(0x4000, 0x7fff) AM_READ(MRA8_BANK1)
	AM_RANGE(0x8000, 0xffff) AM_READ(MRA8_ROM)
ADDRESS_MAP_END

static ADDRESS_MAP_START( writemem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x07ff) AM_WRITE(MWA8_RAM)
	AM_RANGE(0x1000, 0x17ff) AM_WRITE(compgolf_video_w) AM_BASE(&videoram)
	AM_RANGE(0x1800, 0x1fff) AM_WRITE(compgolf_back_w) AM_BASE(&compgolf_bg_ram)
	AM_RANGE(0x2000, 0x2060) AM_WRITE(MWA8_RAM) AM_BASE(&spriteram)
	AM_RANGE(0x2061, 0x2061) AM_WRITE(MWA8_NOP)
	AM_RANGE(0x3001, 0x3001) AM_WRITE(compgolf_ctrl_w)
	AM_RANGE(0x3800, 0x3800) AM_WRITE(YM2203_control_port_0_w)
	AM_RANGE(0x3801, 0x3801) AM_WRITE(YM2203_write_port_0_w)
	AM_RANGE(0x4000, 0x7fff) AM_WRITE(MWA8_ROM)
	AM_RANGE(0x8000, 0xffff) AM_WRITE(MWA8_ROM)
ADDRESS_MAP_END

/***************************************************************************/

INPUT_PORTS_START( compgolf )
	/* Player 1 Port */
	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START2 )

	/* Player 2 Port */
	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON1	 ) PORT_PLAYER(2)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON2	 ) PORT_PLAYER(2)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP	 ) PORT_PLAYER(2)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_PLAYER(2)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_PLAYER(2)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_PLAYER(2)
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_VBLANK )

	/* Dip-Switch Port */
	PORT_START
	PORT_DIPNAME( 0x03,   0x03, DEF_STR( Coinage ) )
	PORT_DIPSETTING(	  0x00, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(	  0x03, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(	  0x02, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(	  0x01, DEF_STR( 1C_3C ) )
	PORT_DIPNAME( 0x04,   0x04, DEF_STR( Unknown ) )
	PORT_DIPSETTING(	  0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(	  0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08,   0x08, DEF_STR( Unknown ) )
	PORT_DIPSETTING(	  0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(	  0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10,   0x10, DEF_STR( Unknown ) )
	PORT_DIPSETTING(	  0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(   	  0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20,   0x20, DEF_STR( Unknown ) )
	PORT_DIPSETTING(	  0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(	  0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40,   0x40, DEF_STR( Unknown ) )
	PORT_DIPSETTING(	  0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(	  0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80,   0x80, DEF_STR( Unknown ) )
	PORT_DIPSETTING(	  0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(	  0x00, DEF_STR( On ) )

	/* System Port */
	PORT_START
	PORT_DIPNAME( 0x01,   0x01, DEF_STR( Unknown ) )
	PORT_DIPSETTING(	  0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(	  0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02,   0x02, "Freeze" ) // this is more likely a switch...
	PORT_DIPSETTING(      0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04,   0x04, DEF_STR( Unknown ) )
	PORT_DIPSETTING(	  0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(	  0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08,   0x08, DEF_STR( Unknown ) )
	PORT_DIPSETTING(	  0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(	  0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10,   0x10, DEF_STR( Unknown ) )
	PORT_DIPSETTING(	  0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(   	  0x00, DEF_STR( On ) )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN2 )
INPUT_PORTS_END

/***************************************************************************/

static const gfx_layout spritelayout =
{
	16,16,
	RGN_FRAC(1,3),
	3,
	{ RGN_FRAC(0,3), RGN_FRAC(1,3), RGN_FRAC(2,3) },
	{ STEP8(8*8*2,1), STEP8(8*8*0,1) },
	{ STEP8(8*8*0,8), STEP8(8*8*1,8) },
	16*16
};

static const gfx_layout tilelayoutbg =
{
	16,16,
	RGN_FRAC(1,2),
	3,
	{ RGN_FRAC(1,2)+0, 0, 4 },
	{ 0, 1, 2, 3, 16*8+0, 16*8+1, 16*8+2, 16*8+3,
		2*16*8+0, 2*16*8+1, 2*16*8+2, 2*16*8+3, 3*16*8+0, 3*16*8+1, 3*16*8+2, 3*16*8+3 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8,
	  8*8, 9*8, 10*8, 11*8, 12*8, 13*8, 14*8, 15*8 },
	32*16
};

static const gfx_layout tilelayout8 =
{
	8,8,
	RGN_FRAC(1,2),
	3,
 	{ RGN_FRAC(1,2)+4, 0, 4 },
	{ 0, 1, 2, 3, 8*8+0, 8*8+1, 8*8+2, 8*8+3 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	16*8
};

static const gfx_decode gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &spritelayout, 0, 0x10 },
	{ REGION_GFX2, 0, &tilelayoutbg, 0, 0x20 },
	{ REGION_GFX3, 0, &tilelayout8,  0, 0x10 },
	{ -1 }
};

/***************************************************************************/

static void sound_irq(int linestate)
{
	cpunum_set_input_line(0,0,linestate);
}

static struct YM2203interface ym2203_interface =
{
	0,
	0,
	compgolf_scrollx_lo_w,
	compgolf_scrolly_lo_w,
	sound_irq
};

static MACHINE_DRIVER_START( compgolf )
	MDRV_CPU_ADD(M6809, 2000000)
	MDRV_CPU_PROGRAM_MAP(readmem,writemem)
	MDRV_CPU_VBLANK_INT(nmi_line_pulse,1)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_REAL_60HZ_VBLANK_DURATION)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(256, 256)
	MDRV_VISIBLE_AREA(1*8, 32*8-1, 1*8, 31*8-1)
	MDRV_PALETTE_LENGTH(0x100)
	MDRV_PALETTE_INIT(compgolf)
	MDRV_GFXDECODE(gfxdecodeinfo)

	MDRV_VIDEO_START(compgolf)
	MDRV_VIDEO_UPDATE(compgolf)

	MDRV_SPEAKER_STANDARD_MONO("mono")

	MDRV_SOUND_ADD(YM2203, 1500000)
	MDRV_SOUND_CONFIG(ym2203_interface)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.0)
MACHINE_DRIVER_END

/***************************************************************************/

ROM_START( compgolf )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )
	ROM_LOAD( "cv05-3.bin",   0x08000, 0x8000, CRC(af9805bf) SHA1(bdde482906bb267e76317067785ac0ab7816df63) )

	ROM_REGION( 0x8000, REGION_USER1, 0 ) // background data
	ROM_LOAD( "cv06.bin",     0x00000, 0x8000, CRC(8f76979d) SHA1(432f6a1402fd3276669f5f45f03fd12380900178) )

	ROM_REGION( 0x18000, REGION_GFX1, ROMREGION_DISPOSE ) // Sprites
	ROM_LOAD( "cv00.bin",     0x00000, 0x8000, CRC(aa3d3b99) SHA1(eb968e40bcc7e7dd1acc0bbe885fd3f7d70d4bb5) )
	ROM_LOAD( "cv01.bin",     0x08000, 0x8000, CRC(f68c2ff6) SHA1(dda9159fb59d3855025b98c272722b031617c89a) )
	ROM_LOAD( "cv02.bin",     0x10000, 0x8000, CRC(979cdb5a) SHA1(25c1f3e6ddf50168c7e1a967bfa2753bea6106ec) )

	ROM_REGION( 0x10000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "cv03.bin",     0x00000, 0x8000, CRC(cc7ed6d8) SHA1(4ffcfa3f720414e1b7e929bdf29359ebcd8717c3) )
	/* we expand rom cv04.bin to 0x8000 - 0xffff */

	ROM_REGION( 0x8000,  REGION_GFX3, ROMREGION_DISPOSE )
	ROM_LOAD( "cv07.bin",     0x00000, 0x8000, CRC(ed5441ba) SHA1(69d50695e8b92544f9857c6f3de0efb399899a2c) )

	ROM_REGION( 0x4000, REGION_GFX4, ROMREGION_DISPOSE )
	ROM_LOAD( "cv04.bin",     0x00000, 0x4000, CRC(df693a04) SHA1(45bef98c7e66881f8c62affecc1ab90dd2707240) )

	ROM_REGION( 0x100, REGION_PROMS, 0 )
	ROM_LOAD( "cv08-1.bpr",   0x00000, 0x0100, CRC(b7c43db9) SHA1(418b11e4c8a9bce6873b0624ac53a5011c5807d0) )
ROM_END

ROM_START( compglfo )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )
	ROM_LOAD( "cv05.bin",     0x08000, 0x8000, CRC(3cef62c9) SHA1(c4827b45faf7aa4c80ddd3c57f1ed6ba76b5c49b) )

	ROM_REGION( 0x8000, REGION_USER1, 0 ) // background data
	ROM_LOAD( "cv06.bin",     0x00000, 0x8000, CRC(8f76979d) SHA1(432f6a1402fd3276669f5f45f03fd12380900178) )

	ROM_REGION( 0x18000, REGION_GFX1, ROMREGION_DISPOSE ) // Sprites
	ROM_LOAD( "cv00.bin",     0x00000, 0x8000, CRC(aa3d3b99) SHA1(eb968e40bcc7e7dd1acc0bbe885fd3f7d70d4bb5) )
	ROM_LOAD( "cv01.bin",     0x08000, 0x8000, CRC(f68c2ff6) SHA1(dda9159fb59d3855025b98c272722b031617c89a) )
	ROM_LOAD( "cv02.bin",     0x10000, 0x8000, CRC(979cdb5a) SHA1(25c1f3e6ddf50168c7e1a967bfa2753bea6106ec) )

	ROM_REGION( 0x10000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "cv03.bin",     0x00000, 0x8000, CRC(cc7ed6d8) SHA1(4ffcfa3f720414e1b7e929bdf29359ebcd8717c3) )
	/* we expand rom cv04.bin to 0x8000 - 0xffff */

	ROM_REGION( 0x8000,  REGION_GFX3, ROMREGION_DISPOSE )
	ROM_LOAD( "cv07.bin",     0x00000, 0x8000, CRC(ed5441ba) SHA1(69d50695e8b92544f9857c6f3de0efb399899a2c) )

	ROM_REGION( 0x4000, REGION_GFX4, ROMREGION_DISPOSE )
	ROM_LOAD( "cv04.bin",     0x00000, 0x4000, CRC(df693a04) SHA1(45bef98c7e66881f8c62affecc1ab90dd2707240) )

	ROM_REGION( 0x100, REGION_PROMS, 0 )
	ROM_LOAD( "cv08-1.bpr",   0x00000, 0x0100, CRC(b7c43db9) SHA1(418b11e4c8a9bce6873b0624ac53a5011c5807d0) )
ROM_END

static void compgolf_expand_bg(void)
{
	UINT8 *GFXDST = memory_region(REGION_GFX2);
	UINT8 *GFXSRC = memory_region(REGION_GFX4);

	int x;

	for (x = 0; x < 0x4000; x++)
	{
		GFXDST[0x8000+x]  = (GFXSRC[x] & 0x0f) << 4;
		GFXDST[0xc000+x]  = (GFXSRC[x] & 0xf0);
	}
}

static DRIVER_INIT( compgolf )
{
	compgolf_expand_bg();
}

GAME( 1986, compgolf, 0,		compgolf, compgolf, compgolf, ROT0, "Data East", "Competition Golf Final Round (revision 3)", 0 )
GAME( 1985, compglfo, compgolf, compgolf, compgolf, compgolf, ROT0, "Data East", "Competition Golf Final Round (old version)", 0 )
