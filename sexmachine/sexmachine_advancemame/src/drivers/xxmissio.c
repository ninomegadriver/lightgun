/*****************************************************************************

XX Mission (c) 1986 UPL

    Driver by Uki

    31/Mar/2001 -

*****************************************************************************/

#include "driver.h"
#include "sound/2203intf.h"

VIDEO_UPDATE( xxmissio );

extern UINT8 *xxmissio_fgram;
extern size_t xxmissio_fgram_size;

static UINT8 *shared_workram;

static UINT8 xxmissio_status;


WRITE8_HANDLER( xxmissio_scroll_x_w );
WRITE8_HANDLER( xxmissio_scroll_y_w );
WRITE8_HANDLER( xxmissio_flipscreen_w );

READ8_HANDLER( xxmissio_videoram_r );
WRITE8_HANDLER( xxmissio_videoram_w );
READ8_HANDLER( xxmissio_fgram_r );
WRITE8_HANDLER( xxmissio_fgram_w );

WRITE8_HANDLER( xxmissio_paletteram_w );

static WRITE8_HANDLER( shared_workram_w )
{
	shared_workram[offset ^ 0x1000] = data;
}

static READ8_HANDLER( shared_workram_r )
{
	return shared_workram[offset ^ 0x1000];
}

WRITE8_HANDLER( xxmissio_bank_sel_w )
{
	UINT8 *BANK = memory_region(REGION_USER1);
	UINT32 bank_address = (data & 0x07) * 0x4000;
	memory_set_bankptr(1, &BANK[bank_address]);
}

READ8_HANDLER( xxmissio_status_r )
{
	xxmissio_status = (xxmissio_status | 2) & ( readinputportbytag("IN2") | 0xfd );
	return xxmissio_status;
}

WRITE8_HANDLER ( xxmissio_status_m_w )
{
	switch (data)
	{
		case 0x00:
			xxmissio_status |= 0x20;
			break;

		case 0x40:
			xxmissio_status &= ~0x08;
			cpunum_set_input_line_and_vector(1,0,HOLD_LINE,0x10);
			break;

		case 0x80:
			xxmissio_status |= 0x04;
			break;
	}
}

WRITE8_HANDLER ( xxmissio_status_s_w )
{
	switch (data)
	{
		case 0x00:
			xxmissio_status |= 0x10;
			break;

		case 0x40:
			xxmissio_status |= 0x08;
			break;

		case 0x80:
			xxmissio_status &= ~0x04;
			cpunum_set_input_line_and_vector(0,0,HOLD_LINE,0x10);
			break;
	}
}

INTERRUPT_GEN( xxmissio_interrupt_m )
{
	xxmissio_status &= ~0x20;
	cpunum_set_input_line(0, 0, HOLD_LINE);
}

INTERRUPT_GEN( xxmissio_interrupt_s )
{
	xxmissio_status &= ~0x10;
	cpunum_set_input_line(1, 0, HOLD_LINE);
}

/****************************************************************************/

static ADDRESS_MAP_START( readmem1, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x7fff) AM_READ(MRA8_ROM)

	AM_RANGE(0x8000, 0x8000) AM_READ(YM2203_status_port_0_r)
	AM_RANGE(0x8001, 0x8001) AM_READ(YM2203_read_port_0_r)
	AM_RANGE(0x8002, 0x8002) AM_READ(YM2203_status_port_1_r)
	AM_RANGE(0x8003, 0x8003) AM_READ(YM2203_read_port_1_r)

	AM_RANGE(0xa000, 0xa000) AM_READ(input_port_0_r)
	AM_RANGE(0xa001, 0xa001) AM_READ(input_port_1_r)

	AM_RANGE(0xa002, 0xa002) AM_READ(xxmissio_status_r)

	AM_RANGE(0xc000, 0xc7ff) AM_READ(xxmissio_fgram_r)
	AM_RANGE(0xc800, 0xcfff) AM_READ(xxmissio_videoram_r)
	AM_RANGE(0xd000, 0xd7ff) AM_READ(spriteram_r)

	AM_RANGE(0xd800, 0xdaff) AM_READ(paletteram_r)

	AM_RANGE(0xe000, 0xffff) AM_READ(MRA8_RAM)
ADDRESS_MAP_END

static ADDRESS_MAP_START( writemem1, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x7fff) AM_WRITE(MWA8_ROM)

	AM_RANGE(0x8000, 0x8000) AM_WRITE(YM2203_control_port_0_w)
	AM_RANGE(0x8001, 0x8001) AM_WRITE(YM2203_write_port_0_w)
	AM_RANGE(0x8002, 0x8002) AM_WRITE(YM2203_control_port_1_w)
	AM_RANGE(0x8003, 0x8003) AM_WRITE(YM2203_write_port_1_w)

	AM_RANGE(0xa002, 0xa002) AM_WRITE(xxmissio_status_m_w)

	AM_RANGE(0xa003, 0xa003) AM_WRITE(xxmissio_flipscreen_w)

	AM_RANGE(0xc000, 0xc7ff) AM_WRITE(xxmissio_fgram_w) AM_BASE(&xxmissio_fgram) AM_SIZE(&xxmissio_fgram_size)
	AM_RANGE(0xc800, 0xcfff) AM_WRITE(xxmissio_videoram_w) AM_BASE(&videoram) AM_SIZE(&videoram_size)
	AM_RANGE(0xd000, 0xd7ff) AM_WRITE(MWA8_RAM) AM_BASE(&spriteram) AM_SIZE(&spriteram_size)

	AM_RANGE(0xd800, 0xdaff) AM_WRITE(xxmissio_paletteram_w) AM_BASE(&paletteram)

	AM_RANGE(0xe000, 0xffff) AM_WRITE(MWA8_RAM) AM_BASE(&shared_workram)
ADDRESS_MAP_END

static ADDRESS_MAP_START( readmem2, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x3fff) AM_READ(MRA8_ROM)
	AM_RANGE(0x4000, 0x7fff) AM_READ(MRA8_BANK1)

	AM_RANGE(0x8000, 0x8000) AM_READ(YM2203_status_port_0_r)
	AM_RANGE(0x8001, 0x8001) AM_READ(YM2203_read_port_0_r)
	AM_RANGE(0x8002, 0x8002) AM_READ(YM2203_status_port_1_r)
	AM_RANGE(0x8003, 0x8003) AM_READ(YM2203_read_port_1_r)

	AM_RANGE(0xa000, 0xa000) AM_READ(input_port_0_r)
	AM_RANGE(0xa001, 0xa001) AM_READ(input_port_1_r)

	AM_RANGE(0xa002, 0xa002) AM_READ(xxmissio_status_r)

	AM_RANGE(0xc000, 0xc7ff) AM_READ(xxmissio_fgram_r)
	AM_RANGE(0xc800, 0xcfff) AM_READ(xxmissio_videoram_r)
	AM_RANGE(0xd000, 0xd7ff) AM_READ(spriteram_r)

	AM_RANGE(0xd800, 0xdaff) AM_READ(paletteram_r)

	AM_RANGE(0xe000, 0xffff) AM_READ(shared_workram_r)

ADDRESS_MAP_END

static ADDRESS_MAP_START( writemem2, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x3fff) AM_WRITE(MWA8_ROM)
	AM_RANGE(0x4000, 0x7fff) AM_WRITE(MWA8_BANK1)

	AM_RANGE(0x8000, 0x8000) AM_WRITE(YM2203_control_port_0_w)
	AM_RANGE(0x8001, 0x8001) AM_WRITE(YM2203_write_port_0_w)
	AM_RANGE(0x8002, 0x8002) AM_WRITE(YM2203_control_port_1_w)
	AM_RANGE(0x8003, 0x8003) AM_WRITE(YM2203_write_port_1_w)

	AM_RANGE(0x8006, 0x8006) AM_WRITE(xxmissio_bank_sel_w)

	AM_RANGE(0xa002, 0xa002) AM_WRITE(xxmissio_status_s_w)

	AM_RANGE(0xa003, 0xa003) AM_WRITE(xxmissio_flipscreen_w)

	AM_RANGE(0xc000, 0xc7ff) AM_WRITE(xxmissio_fgram_w)
	AM_RANGE(0xc800, 0xcfff) AM_WRITE(xxmissio_videoram_w)
	AM_RANGE(0xd000, 0xd7ff) AM_WRITE(spriteram_w)

	AM_RANGE(0xd800, 0xdaff) AM_WRITE(xxmissio_paletteram_w)

	AM_RANGE(0xe000, 0xffff) AM_WRITE(shared_workram_w)
ADDRESS_MAP_END

/****************************************************************************/

INPUT_PORTS_START( xxmissio )
	PORT_START_TAG("IN0")
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY

	PORT_START_TAG("IN1")
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_COCKTAIL
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_COCKTAIL
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY PORT_COCKTAIL
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY PORT_COCKTAIL
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY PORT_COCKTAIL
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY PORT_COCKTAIL

	PORT_START_TAG("DSW1")
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 1C_2C ) )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Normal ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Hard ) )
	PORT_DIPNAME( 0x20, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0x40, 0x40, "Endless Game (Cheat)")
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_SERVICE( 0x80, IP_ACTIVE_LOW )

	PORT_START_TAG("DSW2")
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x01, "2" )
	PORT_DIPSETTING(    0x03, "3" )
	PORT_DIPSETTING(    0x02, "4" )
	PORT_DIPSETTING(    0x00, "5" )
	PORT_DIPNAME( 0x04, 0x04, "First Bonus" )
	PORT_DIPSETTING(    0x04, "30000" )
	PORT_DIPSETTING(    0x00, "40000" )
	PORT_DIPNAME( 0x18, 0x08, "Bonus Every" )
	PORT_DIPSETTING(    0x18, "50000" )
	PORT_DIPSETTING(    0x08, "70000" )
	PORT_DIPSETTING(    0x10, "90000" )
	PORT_DIPSETTING(    0x00, DEF_STR( None ) )
	PORT_DIPNAME( 0x20, 0x20, "Unknown 2-6" )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, "Unknown 2-7" )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, "Unknown 2-8" )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START_TAG("IN2")
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_VBLANK )
INPUT_PORTS_END

/****************************************************************************/

static const gfx_layout charlayout =
{
	16,8,   /* 16*8 characters */
	2048,   /* 2048 characters */
	4,      /* 4 bits per pixel */
	{0,1,2,3},
	{0,4,8,12,16,20,24,28,32,36,40,44,48,52,56,60},
	{64*0, 64*1, 64*2, 64*3, 64*4, 64*5, 64*6, 64*7},
	64*8
};

static const gfx_layout spritelayout =
{
	32,16,    /* 32*16 characters */
	512,	  /* 512 sprites */
	4,        /* 4 bits per pixel */
	{0,1,2,3},
	{0,4,8,12,16,20,24,28,
	 32,36,40,44,48,52,56,60,
	 8*64+0,8*64+4,8*64+8,8*64+12,8*64+16,8*64+20,8*64+24,8*64+28,
	 8*64+32,8*64+36,8*64+40,8*64+44,8*64+48,8*64+52,8*64+56,8*64+60},
	{64*0, 64*1, 64*2, 64*3, 64*4, 64*5, 64*6, 64*7,
	 64*16, 64*17, 64*18, 64*19, 64*20, 64*21, 64*22, 64*23},
	64*8*4
};

static const gfx_layout bglayout =
{
	16,8,   /* 16*8 characters */
	1024,   /* 1024 characters */
	4,      /* 4 bits per pixel */
	{0,1,2,3},
	{0,4,8,12,16,20,24,28,32,36,40,44,48,52,56,60},
	{64*0, 64*1, 64*2, 64*3, 64*4, 64*5, 64*6, 64*7},
	64*8
};

static const gfx_decode gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0x0000, &charlayout,   256,  8 }, /* FG */
	{ REGION_GFX1, 0x0000, &spritelayout,   0,  8 }, /* sprite */
	{ REGION_GFX2, 0x0000, &bglayout,     512, 16 }, /* BG */
	{ -1 } /* end of array */
};

/****************************************************************************/

static struct YM2203interface ym2203_interface_1 =
{
	input_port_2_r,
	input_port_3_r
};

static struct YM2203interface ym2203_interface_2 =
{
	0,
	0,
	xxmissio_scroll_x_w,
	xxmissio_scroll_y_w
};

static MACHINE_DRIVER_START( xxmissio )

	/* basic machine hardware */
	MDRV_CPU_ADD(Z80,12000000/4)	/* 3.0MHz */
	MDRV_CPU_PROGRAM_MAP(readmem1,writemem1)
	MDRV_CPU_VBLANK_INT(xxmissio_interrupt_m,1)

	MDRV_CPU_ADD(Z80,12000000/4)	/* 3.0MHz */
	MDRV_CPU_PROGRAM_MAP(readmem2,writemem2)
	MDRV_CPU_VBLANK_INT(xxmissio_interrupt_s,2)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_REAL_60HZ_VBLANK_DURATION)
	MDRV_INTERLEAVE(100)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER | VIDEO_PIXEL_ASPECT_RATIO_1_2)
	MDRV_SCREEN_SIZE(64*8, 32*8)
	MDRV_VISIBLE_AREA(0*8, 64*8-1, 4*8, 28*8-1)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(768)

	MDRV_VIDEO_START(generic)
	MDRV_VIDEO_UPDATE(xxmissio)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")

	MDRV_SOUND_ADD(YM2203, 12000000/8)
	MDRV_SOUND_CONFIG(ym2203_interface_1)
	MDRV_SOUND_ROUTE(0, "mono", 0.15)
	MDRV_SOUND_ROUTE(1, "mono", 0.15)
	MDRV_SOUND_ROUTE(2, "mono", 0.15)
	MDRV_SOUND_ROUTE(3, "mono", 0.40)

	MDRV_SOUND_ADD(YM2203, 12000000/8)
	MDRV_SOUND_CONFIG(ym2203_interface_2)
	MDRV_SOUND_ROUTE(0, "mono", 0.15)
	MDRV_SOUND_ROUTE(1, "mono", 0.15)
	MDRV_SOUND_ROUTE(2, "mono", 0.15)
	MDRV_SOUND_ROUTE(3, "mono", 0.40)
MACHINE_DRIVER_END

/****************************************************************************/

ROM_START( xxmissio )
	ROM_REGION( 0x10000, REGION_CPU1, 0 ) /* CPU1 */
	ROM_LOAD( "xx1.4l", 0x0000,  0x8000, CRC(86e07709) SHA1(7bfb7540b6509f07a6388ca2da6b3892f5b1df74) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 ) /* CPU2 */
	ROM_LOAD( "xx2.4b", 0x0000,  0x4000, CRC(13fa7049) SHA1(e8974d9f271a966611b523496ba8cd910e227a23) )

	ROM_REGION( 0x18000, REGION_USER1, 0 ) /* BANK */
	ROM_LOAD( "xx3.6a", 0x00000,  0x8000, CRC(16fdacab) SHA1(2158ca9b14c52bc1cd5ef0f4c0180f0519224403) )
	ROM_LOAD( "xx4.6b", 0x08000,  0x8000, CRC(274bd4d2) SHA1(2ddf9b953584e26f221b1c86181d827bdc3dc81b) )
	ROM_LOAD( "xx5.6d", 0x10000,  0x8000, CRC(c5f35535) SHA1(6812b70beb73fc80cf20d2d51f747952ed106887) )

	ROM_REGION( 0x20000, REGION_GFX1, ROMREGION_DISPOSE ) /* FG/sprites */
	ROM_LOAD16_BYTE( "xx6.8j", 0x00001, 0x8000, CRC(dc954d01) SHA1(73ecbbc859da9db9fead91cd03bb90e5779916e2) )
	ROM_LOAD16_BYTE( "xx8.8f", 0x00000, 0x8000, CRC(a9587cc6) SHA1(5fbcb88505f89c4d8a2a228489612ff66fc5d3af) )
	ROM_LOAD16_BYTE( "xx7.8h", 0x10001, 0x8000, CRC(abe9cd68) SHA1(f3ce9b40e3d9cdc9b77a43f9d5d0411338d88833) )
	ROM_LOAD16_BYTE( "xx9.8e", 0x10000, 0x8000, CRC(854e0e5f) SHA1(b01d6a735b175c2f7ac3fc4053702c9da62c6a4e) )

	ROM_REGION( 0x10000, REGION_GFX2, ROMREGION_DISPOSE ) /* BG */
	ROM_LOAD16_BYTE( "xx10.4c", 0x0000,  0x8000, CRC(d27d7834) SHA1(60c24dc2ab7e2a33da4002f1f07eaf7898cf387f) )
	ROM_LOAD16_BYTE( "xx11.4b", 0x0001,  0x8000, CRC(d9dd827c) SHA1(aea3a5abd871adf7f75ad4d6cc57eff0833135c7) )
ROM_END

GAME( 1986, xxmissio, 0, xxmissio, xxmissio, 0, ROT90, "UPL", "XX Mission", 0 )
