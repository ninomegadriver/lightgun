/***************************************************************************

Shaolin's Road

driver by Allard Van Der Bas

***************************************************************************/

#include "driver.h"
#include "sound/sn76496.h"


UINT8 shaolins_nmi_enable;

extern WRITE8_HANDLER( shaolins_videoram_w );
extern WRITE8_HANDLER( shaolins_colorram_w );
extern WRITE8_HANDLER( shaolins_palettebank_w );
extern WRITE8_HANDLER( shaolins_scroll_w );
extern WRITE8_HANDLER( shaolins_nmi_w );

extern PALETTE_INIT( shaolins );
extern VIDEO_START( shaolins );
extern VIDEO_UPDATE( shaolins );


INTERRUPT_GEN( shaolins_interrupt )
{
	if (cpu_getiloops() == 0) cpunum_set_input_line(0, 0, HOLD_LINE);
	else if (cpu_getiloops() % 2)
	{
		if (shaolins_nmi_enable & 0x02) cpunum_set_input_line(0, INPUT_LINE_NMI, PULSE_LINE);
	}
}



static ADDRESS_MAP_START( readmem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0500, 0x0500) AM_READ(input_port_3_r)	/* Dipswitch settings */
	AM_RANGE(0x0600, 0x0600) AM_READ(input_port_4_r)	/* Dipswitch settings */
	AM_RANGE(0x0700, 0x0700) AM_READ(input_port_0_r)	/* coins + service */
	AM_RANGE(0x0701, 0x0701) AM_READ(input_port_1_r)	/* player 1 controls */
	AM_RANGE(0x0702, 0x0702) AM_READ(input_port_2_r)	/* player 2 controls */
	AM_RANGE(0x0703, 0x0703) AM_READ(input_port_5_r)	/* selftest */
	AM_RANGE(0x2800, 0x2bff) AM_READ(MRA8_RAM)	/* RAM BANK 2 */
	AM_RANGE(0x3000, 0x33ff) AM_READ(MRA8_RAM)	/* RAM BANK 1 */
	AM_RANGE(0x3800, 0x3fff) AM_READ(MRA8_RAM)	/* video RAM */
	AM_RANGE(0x4000, 0x5fff) AM_READ(MRA8_ROM)    /* Machine checks for extra rom */
	AM_RANGE(0x6000, 0xffff) AM_READ(MRA8_ROM)
ADDRESS_MAP_END

static ADDRESS_MAP_START( writemem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x0000) AM_WRITE(shaolins_nmi_w)	/* bit 0 = flip screen, bit 1 = nmi enable, bit 2 = ? */
										/* bit 3, bit 4 = coin counters */
	AM_RANGE(0x0100, 0x0100) AM_WRITE(watchdog_reset_w)
	AM_RANGE(0x0300, 0x0300) AM_WRITE(SN76496_0_w) 	/* trigger chip to read from latch. The program always */
	AM_RANGE(0x0400, 0x0400) AM_WRITE(SN76496_1_w) 	/* writes the same number as the latch, so we don't */
										/* bother emulating them. */
	AM_RANGE(0x0800, 0x0800) AM_WRITE(MWA8_NOP)	/* latch for 76496 #0 */
	AM_RANGE(0x1000, 0x1000) AM_WRITE(MWA8_NOP)	/* latch for 76496 #1 */
	AM_RANGE(0x1800, 0x1800) AM_WRITE(shaolins_palettebank_w)
	AM_RANGE(0x2000, 0x2000) AM_WRITE(shaolins_scroll_w)
	AM_RANGE(0x2800, 0x2bff) AM_WRITE(MWA8_RAM)	/* RAM BANK 2 */
	AM_RANGE(0x3000, 0x30ff) AM_WRITE(MWA8_RAM)	/* RAM BANK 1 */
	AM_RANGE(0x3100, 0x33ff) AM_WRITE(MWA8_RAM) AM_BASE(&spriteram) AM_SIZE(&spriteram_size)
	AM_RANGE(0x3800, 0x3bff) AM_WRITE(shaolins_colorram_w) AM_BASE(&colorram)
	AM_RANGE(0x3c00, 0x3fff) AM_WRITE(shaolins_videoram_w) AM_BASE(&videoram)
	AM_RANGE(0x6000, 0xffff) AM_WRITE(MWA8_ROM)
ADDRESS_MAP_END



INPUT_PORTS_START( shaolins )
	PORT_START	/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_4WAY
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_4WAY
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_4WAY
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_4WAY
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_4WAY PORT_COCKTAIL
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_4WAY PORT_COCKTAIL
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_4WAY PORT_COCKTAIL
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_4WAY PORT_COCKTAIL
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_COCKTAIL
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_COCKTAIL
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* DSW0 */
	PORT_DIPNAME( 0x03, 0x02, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x03, "2" )
	PORT_DIPSETTING(    0x02, "3" )
	PORT_DIPSETTING(    0x01, "5" )
	PORT_DIPSETTING(    0x00, "7" )
	PORT_DIPNAME( 0x04, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0x18, 0x18, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x18, "30000 and every 70000" )
	PORT_DIPSETTING(    0x10, "40000 and every 80000" )
	PORT_DIPSETTING(    0x08, "40000" )
	PORT_DIPSETTING(    0x00, "50000" )
	PORT_DIPNAME( 0x60, 0x40, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x60, DEF_STR( Easy ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Medium ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Hard ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Hardest ) )
	PORT_DIPNAME( 0x80, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START	/* DSW1 */
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, "Upright Controls" )
	PORT_DIPSETTING(	0x02, DEF_STR( Single ) )
	PORT_DIPSETTING(	0x00, DEF_STR( Dual ) )
	PORT_SERVICE( 0x04, IP_ACTIVE_LOW )
	PORT_BIT ( 0x08, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_DIPNAME( 0x10, 0x10, "Unknown DSW2 5" )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, "Unknown DSW2 6" )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, "Unknown DSW2 7" )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, "Unknown DSW2 8" )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START	/* DSW2 */
	PORT_DIPNAME( 0x0f, 0x0f, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x05, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 3C_2C ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 4C_3C ) )
	PORT_DIPSETTING(    0x0f, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 3C_4C ) )
	PORT_DIPSETTING(    0x07, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(    0x0e, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x06, DEF_STR( 2C_5C ) )
	PORT_DIPSETTING(    0x0d, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x0c, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x0b, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(    0x0a, DEF_STR( 1C_6C ) )
	PORT_DIPSETTING(    0x09, DEF_STR( 1C_7C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Free_Play ) )
	PORT_DIPNAME( 0xf0, 0xf0, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x20, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x50, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x80, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x40, DEF_STR( 3C_2C ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 4C_3C ) )
	PORT_DIPSETTING(    0xf0, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x30, DEF_STR( 3C_4C ) )
	PORT_DIPSETTING(    0x70, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(    0xe0, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x60, DEF_STR( 2C_5C ) )
	PORT_DIPSETTING(    0xd0, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0xc0, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0xb0, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(    0xa0, DEF_STR( 1C_6C ) )
	PORT_DIPSETTING(    0x90, DEF_STR( 1C_7C ) )
INPUT_PORTS_END



static const gfx_layout shaolins_charlayout =
{
	8,8,	/* 8*8 chars */
	512,	/* 512 characters */
	4,	/* 4 bits per pixel */
	{ 512*16*8+4, 512*16*8+0, 4, 0 },
	{ 0, 1, 2, 3, 8*8+0, 8*8+1, 8*8+2, 8*8+3 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	16*8	/* every char takes 16 consecutive bytes */
};

static const gfx_layout shaolins_spritelayout =
{
	16,16,	/* 16*16 sprites */
	256,	/* 256 sprites */
	4,	/* 4 bits per pixel */
	{ 256*64*8+4, 256*64*8+0, 4, 0 },
	{ 0, 1, 2, 3, 8*8+0, 8*8+1, 8*8+2, 8*8+3,
			16*8+0, 16*8+1, 16*8+2, 16*8+3, 24*8+0, 24*8+1, 24*8+2, 24*8+3 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8,
			32*8, 33*8, 34*8, 35*8, 36*8, 37*8, 38*8, 39*8 },
	64*8	/* every sprite takes 64 consecutive bytes */
};

static const gfx_decode shaolins_gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &shaolins_charlayout,         0, 16*8 },
	{ REGION_GFX2, 0, &shaolins_spritelayout, 16*8*16, 16*8 },
	{ -1 } /* end of array */
};



static MACHINE_DRIVER_START( shaolins )

	/* basic machine hardware */
	MDRV_CPU_ADD(M6809, 1250000)        /* 1.25 MHz */
	MDRV_CPU_PROGRAM_MAP(readmem,writemem)
	MDRV_CPU_VBLANK_INT(shaolins_interrupt,16)	/* 1 IRQ + 8 NMI */

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(32*8, 32*8)
	MDRV_VISIBLE_AREA(0*8, 32*8-1, 2*8, 30*8-1)
	MDRV_GFXDECODE(shaolins_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(256)
	MDRV_COLORTABLE_LENGTH(16*8*16+16*8*16)

	MDRV_PALETTE_INIT(shaolins)
	MDRV_VIDEO_START(shaolins)
	MDRV_VIDEO_UPDATE(shaolins)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")

	MDRV_SOUND_ADD(SN76496, 1536000)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.0)

	MDRV_SOUND_ADD(SN76496, 3072000)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.0)
MACHINE_DRIVER_END


/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START( kicker )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )     /* 64k for code */
	ROM_LOAD( "kikrd8.bin",   0x6000, 0x2000, CRC(2598dfdd) SHA1(70a9d81b73bbd4ff6b627a3e4102d5328a946d20) )
	ROM_LOAD( "kikrd9.bin",   0x8000, 0x4000, CRC(0cf0351a) SHA1(a9da783b29a63a46912a29715e8d11dc4cd22265) )
	ROM_LOAD( "kikrd11.bin",  0xC000, 0x4000, CRC(654037f8) SHA1(52d098386fe87ae97d4dfefab0bd3a902f66d70b) )

	ROM_REGION( 0x4000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "kikra10.bin",  0x0000, 0x2000, CRC(4d156afc) SHA1(29eb66e2ebcf2f1c1d5ece5413d1ebf54663f9cf) )
	ROM_LOAD( "kikra11.bin",  0x2000, 0x2000, CRC(ff6ca5df) SHA1(dfcd445c8b233a0a4168eb249472e53784eda25d) )

	ROM_REGION( 0x8000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "kikrh14.bin",  0x0000, 0x4000, CRC(b94e645b) SHA1(65ae48134a0fe1e910a787714f7ae721734ded5b) )
	ROM_LOAD( "kikrh13.bin",  0x4000, 0x4000, CRC(61bbf797) SHA1(97d276099172975499f646f381a6fc587c022435) )

	ROM_REGION( 0x0500, REGION_PROMS, 0 )
	ROM_LOAD( "kicker.a12",   0x0000, 0x0100, CRC(b09db4b4) SHA1(d21176cdc7def760da109083eb52e5b6a515021f) ) /* palette red component */
	ROM_LOAD( "kicker.a13",   0x0100, 0x0100, CRC(270a2bf3) SHA1(c0aec04bd3bceccddf5f5a814a560a893b29ef6b) ) /* palette green component */
	ROM_LOAD( "kicker.a14",   0x0200, 0x0100, CRC(83e95ea8) SHA1(e0bfa20600488f5c66233e13ea6ad857f62acb7c) ) /* palette blue component */
	ROM_LOAD( "kicker.b8",    0x0300, 0x0100, CRC(aa900724) SHA1(c5343273d0a7101b8ba6876c4f22e43d77610c75) ) /* character lookup table */
	ROM_LOAD( "kicker.f16",   0x0400, 0x0100, CRC(80009cf5) SHA1(a367f3f55d75a9d5bf4d43f9d77272eb910a1344) ) /* sprite lookup table */
ROM_END

ROM_START( shaolins )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )     /* 64k for code */
	ROM_LOAD( "kikrd8.bin",   0x6000, 0x2000, CRC(2598dfdd) SHA1(70a9d81b73bbd4ff6b627a3e4102d5328a946d20) )
	ROM_LOAD( "kikrd9.bin",   0x8000, 0x4000, CRC(0cf0351a) SHA1(a9da783b29a63a46912a29715e8d11dc4cd22265) )
	ROM_LOAD( "kikrd11.bin",  0xC000, 0x4000, CRC(654037f8) SHA1(52d098386fe87ae97d4dfefab0bd3a902f66d70b) )

	ROM_REGION( 0x4000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "shaolins.6",   0x0000, 0x2000, CRC(ff18a7ed) SHA1(f28bfeff84bb6a08a8bee999a0b7a19e09a8dfc3) )
	ROM_LOAD( "shaolins.7",   0x2000, 0x2000, CRC(5f53ae61) SHA1(ad29e2255855c503295c6b63eb4cd6700a1e3f0e) )

	ROM_REGION( 0x8000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "kikrh14.bin",  0x0000, 0x4000, CRC(b94e645b) SHA1(65ae48134a0fe1e910a787714f7ae721734ded5b) )
	ROM_LOAD( "kikrh13.bin",  0x4000, 0x4000, CRC(61bbf797) SHA1(97d276099172975499f646f381a6fc587c022435) )

	ROM_REGION( 0x0500, REGION_PROMS, 0 )
	ROM_LOAD( "kicker.a12",   0x0000, 0x0100, CRC(b09db4b4) SHA1(d21176cdc7def760da109083eb52e5b6a515021f) ) /* palette red component */
	ROM_LOAD( "kicker.a13",   0x0100, 0x0100, CRC(270a2bf3) SHA1(c0aec04bd3bceccddf5f5a814a560a893b29ef6b) ) /* palette green component */
	ROM_LOAD( "kicker.a14",   0x0200, 0x0100, CRC(83e95ea8) SHA1(e0bfa20600488f5c66233e13ea6ad857f62acb7c) ) /* palette blue component */
	ROM_LOAD( "kicker.b8",    0x0300, 0x0100, CRC(aa900724) SHA1(c5343273d0a7101b8ba6876c4f22e43d77610c75) ) /* character lookup table */
	ROM_LOAD( "kicker.f16",   0x0400, 0x0100, CRC(80009cf5) SHA1(a367f3f55d75a9d5bf4d43f9d77272eb910a1344) ) /* sprite lookup table */
ROM_END



GAME( 1985, kicker,   0,      shaolins, shaolins, 0, ROT90, "Konami", "Kicker", 0 )
GAME( 1985, shaolins, kicker, shaolins, shaolins, 0, ROT90, "Konami", "Shao-Lin's Road", 0 )
