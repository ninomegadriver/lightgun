/***************************************************************************

Double Dribble(GX690) (c) Konami 1986

Driver by Manuel Abadia <manu@teleline.es>

***************************************************************************/

#include "driver.h"
#include "cpu/m6809/m6809.h"
#include "sound/2203intf.h"
#include "sound/vlm5030.h"
#include "sound/flt_rc.h"

int ddrible_int_enable_0;
int ddrible_int_enable_1;

static unsigned char *ddrible_sharedram;
static unsigned char *ddrible_snd_sharedram;

extern unsigned char *ddrible_spriteram_1;
extern unsigned char *ddrible_spriteram_2;
extern unsigned char *ddrible_fg_videoram;
extern unsigned char *ddrible_bg_videoram;

/* video hardware memory handlers */
WRITE8_HANDLER( ddrible_fg_videoram_w );
WRITE8_HANDLER( ddrible_bg_videoram_w );

/* video hardware functions */
PALETTE_INIT( ddrible );
VIDEO_START( ddrible );
VIDEO_UPDATE( ddrible );
WRITE8_HANDLER( K005885_0_w );
WRITE8_HANDLER( K005885_1_w );


static INTERRUPT_GEN( ddrible_interrupt_0 )
{
	if (ddrible_int_enable_0)
		cpunum_set_input_line(0, M6809_FIRQ_LINE, HOLD_LINE);
}

static INTERRUPT_GEN( ddrible_interrupt_1 )
{
	if (ddrible_int_enable_1)
		cpunum_set_input_line(1, M6809_FIRQ_LINE, HOLD_LINE);
}


static WRITE8_HANDLER( ddrible_bankswitch_w )
{
	int bankaddress;
	unsigned char *RAM = memory_region(REGION_CPU1);

	bankaddress = 0x10000 + (data & 0x0f)*0x2000;
	memory_set_bankptr(1,&RAM[bankaddress]);
}


static READ8_HANDLER( ddrible_sharedram_r )
{
	return ddrible_sharedram[offset];
}

static WRITE8_HANDLER( ddrible_sharedram_w )
{
	ddrible_sharedram[offset] = data;
}

static READ8_HANDLER( ddrible_snd_sharedram_r )
{
	return ddrible_snd_sharedram[offset];
}

static WRITE8_HANDLER( ddrible_snd_sharedram_w )
{
	ddrible_snd_sharedram[offset] = data;
}

static WRITE8_HANDLER( ddrible_coin_counter_w )
{
	/* b4-b7: unused */
	/* b2-b3: unknown */
	/* b1: coin counter 2 */
	/* b0: coin counter 1 */
	coin_counter_w(0,(data) & 0x01);
	coin_counter_w(1,(data >> 1) & 0x01);
}

static READ8_HANDLER( ddrible_vlm5030_busy_r )
{
	return rand(); /* patch */
	if (VLM5030_BSY()) return 1;
	else return 0;
}

static WRITE8_HANDLER( ddrible_vlm5030_ctrl_w )
{
	unsigned char *SPEECH_ROM = memory_region(REGION_SOUND1);
	/* b7 : vlm data bus OE   */
	/* b6 : VLM5030-RST       */
	/* b5 : VLM5030-ST        */
	/* b4 : VLM5300-VCU       */
	/* b3 : ROM bank select   */
	if (sndti_to_sndnum(SOUND_VLM5030, 0) >= 0)
	{
		VLM5030_RST( data & 0x40 ? 1 : 0 );
		VLM5030_ST(  data & 0x20 ? 1 : 0 );
		VLM5030_VCU( data & 0x10 ? 1 : 0 );
		VLM5030_set_rom(&SPEECH_ROM[data & 0x08 ? 0x10000 : 0]);
	}
	/* b2 : SSG-C rc filter enable */
	/* b1 : SSG-B rc filter enable */
	/* b0 : SSG-A rc filter enable */
	if (sndti_to_sndnum(SOUND_FILTER_RC, 2) >= 0)
	{
		filter_rc_set_RC(2,1000,2200,1000,data & 0x04 ? 150000 : 0); /* YM2203-SSG-C */
		filter_rc_set_RC(1,1000,2200,1000,data & 0x02 ? 150000 : 0); /* YM2203-SSG-B */
		filter_rc_set_RC(0,1000,2200,1000,data & 0x01 ? 150000 : 0); /* YM2203-SSG-A */
	}
}


static ADDRESS_MAP_START( readmem_cpu0, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x1800, 0x187f) AM_READ(MRA8_RAM)			/* palette */
	AM_RANGE(0x2000, 0x3fff) AM_READ(MRA8_RAM)			/* Video RAM 1 + Object RAM 1 */
	AM_RANGE(0x4000, 0x5fff) AM_READ(MRA8_RAM)			/* shared RAM with CPU #1 */
	AM_RANGE(0x6000, 0x7fff) AM_READ(MRA8_RAM)			/* Video RAM 2 + Object RAM 2 */
	AM_RANGE(0x8000, 0x9fff) AM_READ(MRA8_BANK1)			/* banked ROM */
	AM_RANGE(0xa000, 0xffff) AM_READ(MRA8_ROM)			/* ROM */
ADDRESS_MAP_END


static ADDRESS_MAP_START( writemem_cpu0, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x0004) AM_WRITE(K005885_0_w)								/* video registers (005885 #1) */
	AM_RANGE(0x0800, 0x0804) AM_WRITE(K005885_1_w)								/* video registers (005885 #2) */
	AM_RANGE(0x1800, 0x187f) AM_WRITE(paletteram_xBBBBBGGGGGRRRRR_be_w) AM_BASE(&paletteram)/* seems wrong, MSB is used as well */
	AM_RANGE(0x2000, 0x2fff) AM_WRITE(ddrible_fg_videoram_w) AM_BASE(&ddrible_fg_videoram)/* Video RAM 1 */
	AM_RANGE(0x3000, 0x3fff) AM_WRITE(MWA8_RAM) AM_BASE(&ddrible_spriteram_1)				/* Object RAM 1 */
	AM_RANGE(0x4000, 0x5fff) AM_WRITE(MWA8_RAM) AM_BASE(&ddrible_sharedram)				/* shared RAM with CPU #1 */
	AM_RANGE(0x6000, 0x6fff) AM_WRITE(ddrible_bg_videoram_w) AM_BASE(&ddrible_bg_videoram)/* Video RAM 2 */
	AM_RANGE(0x7000, 0x7fff) AM_WRITE(MWA8_RAM) AM_BASE(&ddrible_spriteram_2)				/* Object RAM 2 + Work RAM */
	AM_RANGE(0x8000, 0x8000) AM_WRITE(ddrible_bankswitch_w)						/* bankswitch control */
	AM_RANGE(0x8000, 0xffff) AM_WRITE(MWA8_ROM)									/* ROM */
ADDRESS_MAP_END

static ADDRESS_MAP_START( readmem_cpu1, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x1fff) AM_READ(ddrible_sharedram_r)		/* shared RAM with CPU #0 */
	AM_RANGE(0x2000, 0x27ff) AM_READ(ddrible_snd_sharedram_r)	/* shared RAM with CPU #2 */
	AM_RANGE(0x2800, 0x2800) AM_READ(input_port_3_r)				/* DSW #1 */
	AM_RANGE(0x2801, 0x2801) AM_READ(input_port_0_r)				/* player 1 inputs */
	AM_RANGE(0x2802, 0x2802) AM_READ(input_port_1_r)				/* player 2 inputs */
	AM_RANGE(0x2803, 0x2803) AM_READ(input_port_2_r)				/* coinsw & start */
	AM_RANGE(0x2c00, 0x2c00) AM_READ(input_port_4_r)				/* DSW #2 */
	AM_RANGE(0x3000, 0x3000) AM_READ(input_port_5_r)				/* DSW #3 */
	AM_RANGE(0x8000, 0xffff) AM_READ(MRA8_ROM)					/* ROM */
ADDRESS_MAP_END

static ADDRESS_MAP_START( writemem_cpu1, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x1fff) AM_WRITE(ddrible_sharedram_w)		/* shared RAM with CPU #0 */
	AM_RANGE(0x2000, 0x27ff) AM_WRITE(ddrible_snd_sharedram_w)	/* shared RAM with CPU #2 */
	AM_RANGE(0x3400, 0x3400) AM_WRITE(ddrible_coin_counter_w)		/* coin counters */
	AM_RANGE(0x3c00, 0x3c00) AM_WRITE(watchdog_reset_w)			/* watchdog reset */
	AM_RANGE(0x8000, 0xffff) AM_WRITE(MWA8_ROM)					/* ROM */
ADDRESS_MAP_END

static ADDRESS_MAP_START( readmem_cpu2, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x07ff) AM_READ(MRA8_RAM)					/* shared RAM with CPU #1 */
	AM_RANGE(0x1000, 0x1000) AM_READ(YM2203_status_port_0_r)		/* YM2203 */
	AM_RANGE(0x1001, 0x1001) AM_READ(YM2203_read_port_0_r)		/* YM2203 */
	AM_RANGE(0x8000, 0xffff) AM_READ(MRA8_ROM)					/* ROM */
ADDRESS_MAP_END

static ADDRESS_MAP_START( writemem_cpu2, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x07ff) AM_WRITE(MWA8_RAM) AM_BASE(&ddrible_snd_sharedram)	/* shared RAM with CPU #1 */
	AM_RANGE(0x1000, 0x1000) AM_WRITE(YM2203_control_port_0_w)			/* YM2203 */
	AM_RANGE(0x1001, 0x1001) AM_WRITE(YM2203_write_port_0_w)				/* YM2203 */
	AM_RANGE(0x3000, 0x3000) AM_WRITE(VLM5030_data_w)						/* Speech data */
	AM_RANGE(0x8000, 0xffff) AM_WRITE(MWA8_ROM)							/* ROM */
ADDRESS_MAP_END

INPUT_PORTS_START( ddribble )
	PORT_START	/* PLAYER 1 INPUTS */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON3 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* PLAYER 2 INPUTS */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(2)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_PLAYER(2)
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(2)
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* COINSW & START */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_COIN3 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* DSW #1 */
	PORT_DIPNAME( 0x0f, 0x0f, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x05, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 3C_2C ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 4C_3C ) )
	PORT_DIPSETTING(    0x0f, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 4C_5C ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 3C_4C ) )
	PORT_DIPSETTING(    0x07, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(    0x0e, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x06, DEF_STR( 2C_5C ) )
	PORT_DIPSETTING(    0x0d, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x0c, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x0b, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(    0x0a, DEF_STR( 1C_6C ) )
	PORT_DIPSETTING(    0x09, DEF_STR( 1C_7C ) )
	PORT_DIPNAME( 0xf0, 0xf0, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x20, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x50, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x80, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x40, DEF_STR( 3C_2C ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 4C_3C ) )
	PORT_DIPSETTING(    0xf0, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 4C_5C ) )
	PORT_DIPSETTING(    0x30, DEF_STR( 3C_4C ) )
	PORT_DIPSETTING(    0x70, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(    0xe0, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x60, DEF_STR( 2C_5C ) )
	PORT_DIPSETTING(    0xd0, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0xc0, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0xb0, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(    0xa0, DEF_STR( 1C_6C ) )
	PORT_DIPSETTING(    0x90, DEF_STR( 1C_7C ) )

	PORT_START	/* DSW #2 */
	PORT_DIPNAME( 0x01, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(	0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(	0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(	0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(	0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(	0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x60, 0x40, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(	0x60, DEF_STR( Easy ) )
	PORT_DIPSETTING(	0x40, DEF_STR( Normal ) )
	PORT_DIPSETTING(	0x20, DEF_STR( Hard ) )
	PORT_DIPSETTING(	0x00, DEF_STR( Hardest ) )
	PORT_DIPNAME( 0x80, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(	0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )

	PORT_START	/* DSW #3 */
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(	0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(	0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )
	PORT_SERVICE( 0x04, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0x08, 0x00, "Allow vs match with 1 Credit" )
	PORT_DIPSETTING(	0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNUSED )
INPUT_PORTS_END



static const gfx_layout charlayout =
{
	8,8,
	RGN_FRAC(1,2),
	4,
	{ 0, 1, 2, 3 },
	{ 0*4, 1*4, 2*4, 3*4, 4*4, 5*4, 6*4, 7*4 },
	{ 0*32, 1*32, 2*32, 3*32, 4*32, 5*32, 6*32, 7*32 },
	32*8
};

static const gfx_layout spritelayout =
{
	16,16,
	RGN_FRAC(1,2),
	4,
	{ 0, 1, 2, 3 },
	{ 0*4, 1*4, 2*4, 3*4, 4*4, 5*4, 6*4, 7*4,
			32*8+0*4, 32*8+1*4, 32*8+2*4, 32*8+3*4, 32*8+4*4, 32*8+5*4, 32*8+6*4, 32*8+7*4 },
	{ 0*32, 1*32, 2*32, 3*32, 4*32, 5*32, 6*32, 7*32,
			16*32, 17*32, 18*32, 19*32, 20*32, 21*32, 22*32, 23*32 },
	32*32
};

static const gfx_decode gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0x00000, &charlayout,    48,  1 },	/* colors 48-63 */
	{ REGION_GFX2, 0x00000, &charlayout,    16,  1 },	/* colors 16-31 */
	{ REGION_GFX1, 0x20000, &spritelayout,  32,  1 },	/* colors 32-47 */
	{ REGION_GFX2, 0x40000, &spritelayout,  64, 16 },	/* colors  0-15 but using lookup table */
	{ -1 } /* end of array */
};

static struct YM2203interface ym2203_interface =
{
	0,
	ddrible_vlm5030_busy_r,
	ddrible_vlm5030_ctrl_w,
	0
};

static struct VLM5030interface vlm5030_interface =
{
	REGION_SOUND1,/* memory region of speech rom */
	0x10000     /* memory size 64Kbyte * 2 bank */
};

static MACHINE_DRIVER_START( ddribble )

	/* basic machine hardware */
	MDRV_CPU_ADD(M6809,	1536000)	/* 18432000/12 MHz? */
	MDRV_CPU_PROGRAM_MAP(readmem_cpu0,writemem_cpu0)
	MDRV_CPU_VBLANK_INT(ddrible_interrupt_0,1)

	MDRV_CPU_ADD(M6809,	1536000)	/* 18432000/12 MHz? */
	MDRV_CPU_PROGRAM_MAP(readmem_cpu1,writemem_cpu1)
	MDRV_CPU_VBLANK_INT(ddrible_interrupt_1,1)

	MDRV_CPU_ADD(M6809,	1536000)	/* 18432000/12 MHz? */
	MDRV_CPU_PROGRAM_MAP(readmem_cpu2,writemem_cpu2)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)
	MDRV_INTERLEAVE(100)	/* we need heavy synch */

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(32*8, 32*8)
	MDRV_VISIBLE_AREA(0*8, 32*8-1, 2*8, 30*8-1)
/*  MDRV_SCREEN_SIZE(64*8, 32*8)
    MDRV_VISIBLE_AREA(0*8, 64*8-1, 2*8, 30*8-1) */
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(64)
	MDRV_COLORTABLE_LENGTH(64 + 256)

	MDRV_PALETTE_INIT(ddrible)
	MDRV_VIDEO_START(ddrible)
	MDRV_VIDEO_UPDATE(ddrible)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")

	MDRV_SOUND_ADD(YM2203, 3580000)
	MDRV_SOUND_CONFIG(ym2203_interface)
	MDRV_SOUND_ROUTE(0, "filter1", 0.25)
	MDRV_SOUND_ROUTE(1, "filter2", 0.25)
	MDRV_SOUND_ROUTE(2, "filter3", 0.25)
	MDRV_SOUND_ROUTE(3, "mono", 0.25)

	MDRV_SOUND_ADD(VLM5030, 3580000)
	MDRV_SOUND_CONFIG(vlm5030_interface)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.0)

	MDRV_SOUND_ADD_TAG("filter1", FILTER_RC, 0)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.0)
	MDRV_SOUND_ADD_TAG("filter2", FILTER_RC, 0)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.0)
	MDRV_SOUND_ADD_TAG("filter3", FILTER_RC, 0)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.0)
MACHINE_DRIVER_END


ROM_START( ddribble )
	ROM_REGION( 0x1a000, REGION_CPU1, 0 ) /* 64K CPU #0 + 40K for Banked ROMS */
	ROM_LOAD( "690c03.bin",	0x10000, 0x0a000, CRC(07975a58) SHA1(96fd1b2348bbdf560067d8ee3cd4c0514e263d7a) )
	ROM_CONTINUE(			0x0a000, 0x06000 )

	ROM_REGION( 0x10000, REGION_CPU2, 0 ) /* 64 for the CPU #1 */
	ROM_LOAD( "690c02.bin", 0x08000, 0x08000, CRC(f07c030a) SHA1(db96a10f8bb657bf285266db9e775fa6af82f38c) )

	ROM_REGION( 0x10000, REGION_CPU3, 0 )	/* 64k for the SOUND CPU */
	ROM_LOAD( "690b01.bin", 0x08000, 0x08000, CRC(806b8453) SHA1(3184772c5e5181438a17ac72129070bf164b2965) )

	ROM_REGION( 0x40000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD16_BYTE( "690a05.bin",	0x00000, 0x20000, CRC(6a816d0d) SHA1(73f2527d5f2b9d51b784be36e07e0d0c566a28d9) )	/* characters & objects */
	ROM_LOAD16_BYTE( "690a06.bin",	0x00001, 0x20000, CRC(46300cd0) SHA1(07197a546fff452a41575fcd481da64ac6bf601e) )

	ROM_REGION( 0x80000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD16_BYTE( "690a10.bin", 0x00000, 0x20000, CRC(61efa222) SHA1(bd7b993ad1c06d8f6ac29fbc07c4a987abe1ab42) )	/* characters */
	ROM_LOAD16_BYTE( "690a09.bin", 0x00001, 0x20000, CRC(ab682186) SHA1(a28982835042a07354557e1539b097cdf93fc466) )
	ROM_LOAD16_BYTE( "690a08.bin", 0x40000, 0x20000, CRC(9a889944) SHA1(ca96815aefb1e336bd2288841b00a5c21cacf90f) )	/* objects */
	ROM_LOAD16_BYTE( "690a07.bin", 0x40001, 0x20000, CRC(faf81b3f) SHA1(0bd647b4cdd3f2209472e303fd22eedd5533d1b1) )

	ROM_REGION( 0x0100, REGION_PROMS, 0 )
	ROM_LOAD( "690a11.i15", 0x0000, 0x0100, CRC(f34617ad) SHA1(79ceba6fe204472a5a659641ac4f14bb1f0ee3f6) )	/* sprite lookup table */

	ROM_REGION( 0x20000, REGION_SOUND1, 0 )	/* 128k for the VLM5030 data */
	ROM_LOAD( "690a04.bin", 0x00000, 0x20000, CRC(1bfeb763) SHA1(f3e9acb2a7a9b4c8dee6838c1344a7a65c27ff77) )
ROM_END



GAME( 1986, ddribble, 0, ddribble, ddribble, 0, ROT0, "Konami", "Double Dribble", 0)
