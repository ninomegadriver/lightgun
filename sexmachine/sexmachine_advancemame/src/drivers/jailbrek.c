/***************************************************************************

Jailbreak - (c) 1986 Konami

Ernesto Corvi
ernesto@imagina.com

***************************************************************************/

/*

    TODO:

    - coin counters

*/

#include "driver.h"
#include "cpu/m6809/m6809.h"
#include "sound/sn76496.h"
#include "sound/vlm5030.h"


void konami1_decode(void);

extern UINT8 *jailbrek_scroll_x;
extern UINT8 *jailbrek_scroll_dir;

extern WRITE8_HANDLER( jailbrek_videoram_w );
extern WRITE8_HANDLER( jailbrek_colorram_w );

extern PALETTE_INIT( jailbrek );
extern VIDEO_START( jailbrek );
extern VIDEO_UPDATE( jailbrek );


static int irq_enable,nmi_enable;


static WRITE8_HANDLER( ctrl_w )
{
	nmi_enable = data & 0x01;
	irq_enable = data & 0x02;
	flip_screen_set(data & 0x08);
}

static INTERRUPT_GEN( jb_interrupt )
{
	if (irq_enable)
		cpunum_set_input_line(0, 0, HOLD_LINE);
}

static INTERRUPT_GEN( jb_interrupt_nmi )
{
	if (nmi_enable)
		cpunum_set_input_line(0, INPUT_LINE_NMI, PULSE_LINE);
}


static READ8_HANDLER( jailbrek_speech_r ) {
	return ( VLM5030_BSY() ? 1 : 0 );
}

static WRITE8_HANDLER( jailbrek_speech_w ) {
	/* bit 0 could be latch direction like in yiear */
	VLM5030_ST( ( data >> 1 ) & 1 );
	VLM5030_RST( ( data >> 2 ) & 1 );
}

static ADDRESS_MAP_START( jailbrek_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x07ff) AM_RAM AM_WRITE(jailbrek_colorram_w) AM_BASE(&colorram)
    AM_RANGE(0x0800, 0x0fff) AM_RAM AM_WRITE(jailbrek_videoram_w) AM_BASE(&videoram)
    AM_RANGE(0x1000, 0x10bf) AM_RAM AM_BASE(&spriteram) AM_SIZE(&spriteram_size)
	AM_RANGE(0x10c0, 0x14ff) AM_RAM /* ??? */
	AM_RANGE(0x1500, 0x1fff) AM_RAM /* work ram */
    AM_RANGE(0x2000, 0x203f) AM_RAM AM_BASE(&jailbrek_scroll_x)
    AM_RANGE(0x2040, 0x2040) AM_WRITENOP /* ??? */
    AM_RANGE(0x2041, 0x2041) AM_WRITENOP /* ??? */
    AM_RANGE(0x2042, 0x2042) AM_RAM AM_BASE(&jailbrek_scroll_dir) /* bit 2 = scroll direction */
    AM_RANGE(0x2043, 0x2043) AM_WRITENOP /* ??? */
    AM_RANGE(0x2044, 0x2044) AM_WRITE(ctrl_w) /* irq, nmi enable, screen flip */
	AM_RANGE(0x3000, 0x307f) AM_RAM /* related to sprites? */
	AM_RANGE(0x3100, 0x3100) AM_READWRITE(input_port_4_r, SN76496_0_w) /* DSW1 */
	AM_RANGE(0x3200, 0x3200) AM_READ(input_port_5_r) AM_WRITENOP /* DSW2 */ /* mirror of the previous? */
	AM_RANGE(0x3300, 0x3300) AM_READWRITE(input_port_0_r, watchdog_reset_w) /* coins, start */
	AM_RANGE(0x3301, 0x3301) AM_READ(input_port_1_r) /* joy1 */
	AM_RANGE(0x3302, 0x3302) AM_READ(input_port_2_r) /* joy2 */
	AM_RANGE(0x3303, 0x3303) AM_READ(input_port_3_r) /* DSW0 */
	AM_RANGE(0x4000, 0x4000) AM_WRITE(jailbrek_speech_w) /* speech pins */
	AM_RANGE(0x5000, 0x5000) AM_WRITE(VLM5030_data_w) /* speech data */
	AM_RANGE(0x6000, 0x6000) AM_READ(jailbrek_speech_r)
	AM_RANGE(0x8000, 0xffff) AM_ROM
ADDRESS_MAP_END



INPUT_PORTS_START( jailbrek )
	PORT_START	/* IN0 - $3300 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0xe0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* IN1 - $3301 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )	// shoot
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 )	// select
	PORT_BIT( 0xc0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* IN2 - $3302 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(2)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(2)
	PORT_BIT( 0xc0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* DSW0  - $3303 */
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
	PORT_DIPSETTING(    0x00, "Invalid" )

	PORT_START	/* DSW1  - $3100 */
	PORT_DIPNAME( 0x03, 0x01, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x03, "1" )
	PORT_DIPSETTING(    0x02, "2" )
	PORT_DIPSETTING(    0x01, "3" )
	PORT_DIPSETTING(    0x00, "5" )
	PORT_DIPNAME( 0x04, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x08, "30K 70K+" )
	PORT_DIPSETTING(    0x00, "40K 80K+" )
	PORT_DIPNAME( 0x30, 0x10, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x30, DEF_STR( Easy ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Normal ) )
	PORT_DIPSETTING(    0x10, "Difficult" )
	PORT_DIPSETTING(    0x00, "Very Difficult" )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_DIPNAME( 0x80, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START	/* DSW2  - $3200 */
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(	0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, "Upright Controls" )
	PORT_DIPSETTING(    0x02, DEF_STR( Single ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Dual ) )
	PORT_BIT( 0x0c, IP_ACTIVE_LOW, IPT_UNUSED )
INPUT_PORTS_END


static const gfx_layout charlayout =
{
	8,8,	/* 8*8 characters */
	1024,	/* 1024 characters */
	4,	/* 4 bits per pixel */
	{ 0, 1, 2, 3 },	/* the four bitplanes are packed in one nibble */
	{ 0*4, 1*4, 2*4, 3*4, 4*4, 5*4, 6*4, 7*4 },
	{ 0*32, 1*32, 2*32, 3*32, 4*32, 5*32, 6*32, 7*32 },
	32*8	/* every char takes 32 consecutive bytes */
};

static const gfx_layout spritelayout =
{
	16,16,	/* 16*16 sprites */
	512,	/* 512 sprites */
	4,	/* 4 bits per pixel */
	{ 0, 1, 2, 3 },	/* the bitplanes are packed in one nibble */
	{ 0*4, 1*4, 2*4, 3*4, 4*4, 5*4, 6*4, 7*4,
			32*8+0*4, 32*8+1*4, 32*8+2*4, 32*8+3*4, 32*8+4*4, 32*8+5*4, 32*8+6*4, 32*8+7*4 },
	{ 0*32, 1*32, 2*32, 3*32, 4*32, 5*32, 6*32, 7*32,
			16*32, 17*32, 18*32, 19*32, 20*32, 21*32, 22*32, 23*32 },
	128*8	/* every sprite takes 128 consecutive bytes */
};

static const gfx_decode gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &charlayout,   0, 16 }, /* characters */
	{ REGION_GFX2, 0, &spritelayout, 16*16, 16 }, /* sprites */
	{ -1 } /* end of array */
};



static struct VLM5030interface vlm5030_interface =
{
	REGION_SOUND1,	/* memory region of speech rom */
	0           /* memory size of speech rom */
};

static MACHINE_DRIVER_START( jailbrek )

	/* basic machine hardware */
	MDRV_CPU_ADD(M6809, 3000000)        /* 3 MHz ??? */
	MDRV_CPU_PROGRAM_MAP(jailbrek_map, 0)
	MDRV_CPU_VBLANK_INT(jb_interrupt,1)
	MDRV_CPU_PERIODIC_INT(jb_interrupt_nmi,TIME_IN_HZ(500)) /* ? */

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(32*8, 32*8)
	MDRV_VISIBLE_AREA(1*8, 31*8-1, 2*8, 30*8-1)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(32)
	MDRV_COLORTABLE_LENGTH(512)

	MDRV_PALETTE_INIT(jailbrek)
	MDRV_VIDEO_START(jailbrek)
	MDRV_VIDEO_UPDATE(jailbrek)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")

	MDRV_SOUND_ADD(SN76496, 1500000)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.0)

	MDRV_SOUND_ADD(VLM5030, 3580000)
	MDRV_SOUND_CONFIG(vlm5030_interface)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.0)
MACHINE_DRIVER_END


/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START( jailbrek )
    ROM_REGION( 0x10000, REGION_CPU1, 0 )     /* 64k for code */
	ROM_LOAD( "jailb11d.bin", 0x8000, 0x4000, CRC(a0b88dfd) SHA1(f999e382b9d3b812fca41f4d0da3ea692fef6b19) )
	ROM_LOAD( "jailb9d.bin",  0xc000, 0x4000, CRC(444b7d8e) SHA1(c708b67c2d249448dae9a3d10c24d13ba6849597) )

    ROM_REGION( 0x08000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "jailb4f.bin",  0x00000, 0x4000, CRC(e3b7a226) SHA1(c19a02a2def65648bf198fccec98ebbd2fc7c0fb) )	/* characters */
    ROM_LOAD( "jailb5f.bin",  0x04000, 0x4000, CRC(504f0912) SHA1(b51a45dd5506bccdf0061dd6edd7f49ac86ed0f8) )

    ROM_REGION( 0x10000, REGION_GFX2, ROMREGION_DISPOSE )
    ROM_LOAD( "jailb3e.bin",  0x00000, 0x4000, CRC(0d269524) SHA1(a10ddb405e884bfec521a3c7a29d22f63e535b59) )	/* sprites */
    ROM_LOAD( "jailb4e.bin",  0x04000, 0x4000, CRC(27d4f6f4) SHA1(c42c064dbd7c5cf0b1d99651367e0bee1728a5b0) )
    ROM_LOAD( "jailb5e.bin",  0x08000, 0x4000, CRC(717485cb) SHA1(22609489186dcb3d7cd49b7ddfdc6f04d0739354) )
    ROM_LOAD( "jailb3f.bin",  0x0c000, 0x4000, CRC(e933086f) SHA1(c0fd1e8d23c0f7e14c0b75f629448034420cf8ef) )

	ROM_REGION( 0x0240, REGION_PROMS, 0 )
	ROM_LOAD( "jailbbl.cl2",  0x0000, 0x0020, CRC(f1909605) SHA1(91eaa865375b3bc052897732b64b1ff7df3f78f6) ) /* red & green */
	ROM_LOAD( "jailbbl.cl1",  0x0020, 0x0020, CRC(f70bb122) SHA1(bf77990260e8346faa3d3481718cbe46a4a27150) ) /* blue */
	ROM_LOAD( "jailbbl.bp2",  0x0040, 0x0100, CRC(d4fe5c97) SHA1(972e9dab6c53722545dd3a43e3ada7921e88708b) ) /* char lookup */
	ROM_LOAD( "jailbbl.bp1",  0x0140, 0x0100, CRC(0266c7db) SHA1(a8f21e86e6d974c9bfd92a147689d0e7316d66e2) ) /* sprites lookup */

	ROM_REGION( 0x2000, REGION_SOUND1, 0 ) /* speech rom */
	ROM_LOAD( "jailb8c.bin",  0x0000, 0x2000, CRC(d91d15e3) SHA1(475fe50aafbf8f2fb79880ef0e2c25158eda5270) )
ROM_END

ROM_START( manhatan )
    ROM_REGION( 0x10000, REGION_CPU1, 0 )     /* 64k for code */
	ROM_LOAD( "507n03.9d",    0x8000, 0x4000, CRC(e5039f7e) SHA1(0f12484ed40444d978e0405c27bdd027ae2e2a0b) )
	ROM_LOAD( "507n02.11d",   0xc000, 0x4000, CRC(143cc62c) SHA1(9520dbb1b6f1fa439e03d4caa9bed96ef8f805f2) )

    ROM_REGION( 0x08000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "507j08.4f",    0x00000, 0x4000, CRC(175e1b49) SHA1(4cfe982cdf7729bd05c6da803480571876320bf6) )	/* characters */
    ROM_LOAD( "jailb5f.bin",  0x04000, 0x4000, CRC(504f0912) SHA1(b51a45dd5506bccdf0061dd6edd7f49ac86ed0f8) )

    ROM_REGION( 0x10000, REGION_GFX2, ROMREGION_DISPOSE )
    ROM_LOAD( "jailb3e.bin",  0x00000, 0x4000, CRC(0d269524) SHA1(a10ddb405e884bfec521a3c7a29d22f63e535b59) )	/* sprites */
    ROM_LOAD( "jailb4e.bin",  0x04000, 0x4000, CRC(27d4f6f4) SHA1(c42c064dbd7c5cf0b1d99651367e0bee1728a5b0) )
    ROM_LOAD( "jailb5e.bin",  0x08000, 0x4000, CRC(717485cb) SHA1(22609489186dcb3d7cd49b7ddfdc6f04d0739354) )
    ROM_LOAD( "jailb3f.bin",  0x0c000, 0x4000, CRC(e933086f) SHA1(c0fd1e8d23c0f7e14c0b75f629448034420cf8ef) )

	ROM_REGION( 0x0240, REGION_PROMS, 0 )
	ROM_LOAD( "jailbbl.cl2",  0x0000, 0x0020, CRC(f1909605) SHA1(91eaa865375b3bc052897732b64b1ff7df3f78f6) ) /* red & green */
	ROM_LOAD( "jailbbl.cl1",  0x0020, 0x0020, CRC(f70bb122) SHA1(bf77990260e8346faa3d3481718cbe46a4a27150) ) /* blue */
	ROM_LOAD( "jailbbl.bp2",  0x0040, 0x0100, CRC(d4fe5c97) SHA1(972e9dab6c53722545dd3a43e3ada7921e88708b) ) /* char lookup */
	ROM_LOAD( "jailbbl.bp1",  0x0140, 0x0100, CRC(0266c7db) SHA1(a8f21e86e6d974c9bfd92a147689d0e7316d66e2) ) /* sprites lookup */

	ROM_REGION( 0x2000, REGION_SOUND1, 0 ) /* speech rom */
	ROM_LOAD( "507p01.8c",    0x0000, 0x2000, CRC(4a1da0b7) SHA1(e18987f0f7fa8740d5f91d1701ee11612c94e8e8) )
ROM_END


static DRIVER_INIT( jailbrek )
{
	konami1_decode();
}


GAME( 1986, jailbrek, 0,        jailbrek, jailbrek, jailbrek, ROT0, "Konami", "Jail Break", 0 )
GAME( 1986, manhatan, jailbrek, jailbrek, jailbrek, jailbrek, ROT0, "Konami", "Manhattan 24 Bunsyo (Japan)", 0 )
