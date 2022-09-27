/***************************************************************************

Aliens (c) 1990 Konami Co. Ltd

Preliminary driver by:
    Manuel Abadia <manu@teleline.es>

***************************************************************************/

#include "driver.h"
#include "cpu/konami/konami.h" /* for the callback and the firq irq definition */
#include "vidhrdw/konamiic.h"
#include "sound/k007232.h"
#include "sound/2151intf.h"

/* prototypes */
static MACHINE_RESET( aliens );
static void aliens_banking( int lines );


VIDEO_START( aliens );
VIDEO_UPDATE( aliens );


static int palette_selected;
static unsigned char *ram;


static INTERRUPT_GEN( aliens_interrupt )
{
	if (K051960_is_IRQ_enabled())
		cpunum_set_input_line(0, KONAMI_IRQ_LINE, HOLD_LINE);
}

static READ8_HANDLER( bankedram_r )
{
	if (palette_selected)
		return paletteram_r(offset);
	else
		return ram[offset];
}

static WRITE8_HANDLER( bankedram_w )
{
	if (palette_selected)
		paletteram_xBBBBBGGGGGRRRRR_be_w(offset,data);
	else
		ram[offset] = data;
}

static WRITE8_HANDLER( aliens_coin_counter_w )
{
	/* bits 0-1 = coin counters */
	coin_counter_w(0,data & 0x01);
	coin_counter_w(1,data & 0x02);

	/* bit 5 = select work RAM or palette */
	palette_selected = data & 0x20;

	/* bit 6 = enable char ROM reading through the video RAM */
	K052109_set_RMRD_line((data & 0x40) ? ASSERT_LINE : CLEAR_LINE);

	/* other bits unknown */
#if 0
{
	char baf[40];
	sprintf(baf,"%02x",data);
	ui_popup(baf);
}
#endif
}

WRITE8_HANDLER( aliens_sh_irqtrigger_w )
{
	soundlatch_w(offset,data);
	cpunum_set_input_line_and_vector(1, 0, HOLD_LINE, 0xff);
}

static WRITE8_HANDLER( aliens_snd_bankswitch_w )
{
	/* b1: bank for chanel A */
	/* b0: bank for chanel B */

	int bank_A = ((data >> 1) & 0x01);
	int bank_B = ((data) & 0x01);

	K007232_set_bank( 0, bank_A, bank_B );
}


static ADDRESS_MAP_START( aliens_readmem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x03ff) AM_READ(bankedram_r)			/* palette + work RAM */
	AM_RANGE(0x0400, 0x1fff) AM_READ(MRA8_RAM)
	AM_RANGE(0x2000, 0x3fff) AM_READ(MRA8_BANK1)				/* banked ROM */
	AM_RANGE(0x5f80, 0x5f80) AM_READ(input_port_2_r)			/* DIPSW #3 */
	AM_RANGE(0x5f81, 0x5f81) AM_READ(input_port_3_r)			/* Player 1 inputs */
	AM_RANGE(0x5f82, 0x5f82) AM_READ(input_port_4_r)			/* Player 2 inputs */
	AM_RANGE(0x5f83, 0x5f83) AM_READ(input_port_1_r)			/* DIPSW #2 */
	AM_RANGE(0x5f84, 0x5f84) AM_READ(input_port_0_r)			/* DIPSW #1 */
	AM_RANGE(0x5f88, 0x5f88) AM_READ(watchdog_reset_r)
	AM_RANGE(0x4000, 0x7fff) AM_READ(K052109_051960_r)
	AM_RANGE(0x8000, 0xffff) AM_READ(MRA8_ROM)				/* ROM e24_j02.bin */
ADDRESS_MAP_END

static ADDRESS_MAP_START( aliens_writemem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x03ff) AM_WRITE(bankedram_w) AM_BASE(&ram)			/* palette + work RAM */
	AM_RANGE(0x0400, 0x1fff) AM_WRITE(MWA8_RAM)
	AM_RANGE(0x2000, 0x3fff) AM_WRITE(MWA8_ROM)					/* banked ROM */
	AM_RANGE(0x5f88, 0x5f88) AM_WRITE(aliens_coin_counter_w)		/* coin counters */
	AM_RANGE(0x5f8c, 0x5f8c) AM_WRITE(aliens_sh_irqtrigger_w)		/* cause interrupt on audio CPU */
	AM_RANGE(0x4000, 0x7fff) AM_WRITE(K052109_051960_w)
	AM_RANGE(0x8000, 0xffff) AM_WRITE(MWA8_ROM)					/* ROM e24_j02.bin */
ADDRESS_MAP_END

static ADDRESS_MAP_START( aliens_readmem_sound, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x7fff) AM_READ(MRA8_ROM)				/* ROM g04_b03.bin */
	AM_RANGE(0x8000, 0x87ff) AM_READ(MRA8_RAM)				/* RAM */
	AM_RANGE(0xa001, 0xa001) AM_READ(YM2151_status_port_0_r)
	AM_RANGE(0xc000, 0xc000) AM_READ(soundlatch_r)			/* soundlatch_r */
	AM_RANGE(0xe000, 0xe00d) AM_READ(K007232_read_port_0_r)
ADDRESS_MAP_END

static ADDRESS_MAP_START( aliens_writemem_sound, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x7fff) AM_WRITE(MWA8_ROM)					/* ROM g04_b03.bin */
	AM_RANGE(0x8000, 0x87ff) AM_WRITE(MWA8_RAM)					/* RAM */
	AM_RANGE(0xa000, 0xa000) AM_WRITE(YM2151_register_port_0_w)
	AM_RANGE(0xa001, 0xa001) AM_WRITE(YM2151_data_port_0_w)
	AM_RANGE(0xe000, 0xe00d) AM_WRITE(K007232_write_port_0_w)
ADDRESS_MAP_END

/***************************************************************************

    Input Ports

***************************************************************************/

INPUT_PORTS_START( aliens )
	PORT_START_TAG("DSW1")
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
//  PORT_DIPSETTING(    0x00, "Invalid" )

	PORT_START_TAG("DSW2")
	PORT_DIPNAME( 0x03, 0x02, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x03, "1" )
	PORT_DIPSETTING(    0x02, "2" )
	PORT_DIPSETTING(    0x01, "3" )
	PORT_DIPSETTING(    0x00, "5" )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x60, 0x40, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(	0x60, DEF_STR( Easy ) )
	PORT_DIPSETTING(	0x40, DEF_STR( Normal ) )
	PORT_DIPSETTING(	0x20, "Difficult" )
	PORT_DIPSETTING(	0x00, "Very Difficult" )
	PORT_DIPNAME( 0x80, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(	0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )

	PORT_START_TAG("DSW3")
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_SERVICE( 0x04, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_COIN3 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START_TAG("IN0")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START1 )

	PORT_START_TAG("IN1")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(2)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(2)
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START2 )
INPUT_PORTS_END


/***************************************************************************

    Machine Driver

***************************************************************************/

static void volume_callback(int v)
{
	K007232_set_volume(0,0,(v & 0x0f) * 0x11,0);
	K007232_set_volume(0,1,0,(v >> 4) * 0x11);
}

static struct K007232_interface k007232_interface =
{
	REGION_SOUND1,	/* memory regions */
	volume_callback	/* external port callback */
};

static struct YM2151interface ym2151_interface =
{
	0,
	aliens_snd_bankswitch_w
};

static MACHINE_DRIVER_START( aliens )

	/* basic machine hardware */
	MDRV_CPU_ADD(KONAMI, 3000000)		/* ? */
	MDRV_CPU_PROGRAM_MAP(aliens_readmem,aliens_writemem)
	MDRV_CPU_VBLANK_INT(aliens_interrupt,1)

	MDRV_CPU_ADD(Z80, 3579545)
	/* audio CPU */		/* ? */
	MDRV_CPU_PROGRAM_MAP(aliens_readmem_sound,aliens_writemem_sound)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	MDRV_MACHINE_RESET(aliens)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER | VIDEO_HAS_SHADOWS)
	MDRV_SCREEN_SIZE(64*8, 32*8)
	MDRV_VISIBLE_AREA(14*8, (64-14)*8-1, 2*8, 30*8-1 )
	MDRV_PALETTE_LENGTH(512)

	MDRV_VIDEO_START(aliens)
	MDRV_VIDEO_UPDATE(aliens)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")

	MDRV_SOUND_ADD(YM2151, 3579545)
	MDRV_SOUND_CONFIG(ym2151_interface)
	MDRV_SOUND_ROUTE(0, "mono", 0.60)
	MDRV_SOUND_ROUTE(1, "mono", 0.60)

	MDRV_SOUND_ADD(K007232, 3579545)
	MDRV_SOUND_CONFIG(k007232_interface)
	MDRV_SOUND_ROUTE(0, "mono", 0.20)
	MDRV_SOUND_ROUTE(1, "mono", 0.20)
MACHINE_DRIVER_END


/***************************************************************************

  Game ROMs

***************************************************************************/

ROM_START( aliens )
	ROM_REGION( 0x38000, REGION_CPU1, 0 ) /* code + banked roms */
	ROM_LOAD( "e24_j02.bin", 0x10000, 0x08000, CRC(56c20971) SHA1(af272e146705e97342466a208c64d823ebc83d83) )
	ROM_CONTINUE(            0x08000, 0x08000 )
	ROM_LOAD( "c24_j01.bin", 0x18000, 0x20000, CRC(6a529cd6) SHA1(bff6dee33141d8ed2b2c28813cf49f52dceac364) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 ) /* 64k for the sound CPU */
	ROM_LOAD( "g04_b03.bin", 0x00000, 0x08000, CRC(1ac4d283) SHA1(2253f1f39c7edb6cef438b3d97f3af67a1f491ff) )

	ROM_REGION( 0x200000, REGION_GFX1, 0 ) /* graphics */
	ROM_LOAD( "k13_b11.bin", 0x000000, 0x80000, CRC(89c5c885) SHA1(02a1581579b6ef816e04bec312a7b3ae7c7e84f8) )	/* characters (set 1) */
	ROM_LOAD( "j13_b07.bin", 0x080000, 0x40000, CRC(e9c56d66) SHA1(1f58949d5391aef002a6e1ee7034e57bf99cee61) )	/* characters (set 2) */
	/* second half empty */
	ROM_LOAD( "k19_b12.bin", 0x100000, 0x80000, CRC(ea6bdc17) SHA1(a7c22370f8adc5b479283f1ff831f493df78282f) )	/* characters (set 1) */
	ROM_LOAD( "j19_b08.bin", 0x180000, 0x40000, CRC(f9387966) SHA1(470ecc4a5a3edd08d5e0ab10b0c590db1968fb0a) )	/* characters (set 2) */
	/* second half empty */

	ROM_REGION( 0x200000, REGION_GFX2, 0 ) /* graphics */
	ROM_LOAD( "k08_b10.bin", 0x000000, 0x80000, CRC(0b1035b1) SHA1(db04020761386e79249762cd1540208375c38c7f) )	/* sprites (set 1) */
	ROM_LOAD( "j08_b06.bin", 0x080000, 0x40000, CRC(081a0566) SHA1(3a4aa14178fe76a030224743c9e9cd974e08bd79) )	/* sprites (set 2) */
	/* second half empty */
	ROM_LOAD( "k02_b09.bin", 0x100000, 0x80000, CRC(e76b3c19) SHA1(6838e07460b3eaaeb129208ad0696c8019bd63d9) )	/* sprites (set 1) */
	ROM_LOAD( "j02_b05.bin", 0x180000, 0x40000, CRC(19a261f2) SHA1(b0518fad833b3e613e0201d5d9cab73dc5e78e1d) )	/* sprites (set 2) */
	/* second half empty */

	ROM_REGION( 0x0100, REGION_PROMS, 0 )
	ROM_LOAD( "821a08.h14",  0x0000, 0x0100, CRC(7da55800) SHA1(3826f73569c8ae0431510a355bdfa082152b74a5) )	/* priority encoder (not used) */

	ROM_REGION( 0x40000, REGION_SOUND1, 0 ) /* samples for 007232 */
	ROM_LOAD( "875b04.bin",  0x00000, 0x40000, CRC(4e209ac8) SHA1(09d9eaae61bfd04bf318555ccd44d7371571d86d) )
ROM_END

ROM_START( aliens2 )
	ROM_REGION( 0x38000, REGION_CPU1, 0 ) /* code + banked roms */
	ROM_LOAD( "e24_p02.bin", 0x10000, 0x08000, CRC(4edd707d) SHA1(02b39068e5fd99ecb5b35a586335b65a20fde490) )
	ROM_CONTINUE(            0x08000, 0x08000 )
	ROM_LOAD( "c24_n01.bin", 0x18000, 0x20000, CRC(106cf59c) SHA1(78622adc02055d31cd587c83b23a6cde30c9bc22) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 ) /* 64k for the sound CPU */
	ROM_LOAD( "g04_b03.bin", 0x00000, 0x08000, CRC(1ac4d283) SHA1(2253f1f39c7edb6cef438b3d97f3af67a1f491ff) )

	ROM_REGION( 0x200000, REGION_GFX1, 0 ) /* graphics */
	ROM_LOAD( "k13_b11.bin", 0x000000, 0x80000, CRC(89c5c885) SHA1(02a1581579b6ef816e04bec312a7b3ae7c7e84f8) )	/* characters (set 1) */
	ROM_LOAD( "j13_b07.bin", 0x080000, 0x40000, CRC(e9c56d66) SHA1(1f58949d5391aef002a6e1ee7034e57bf99cee61) )	/* characters (set 2) */
	/* second half empty */
	ROM_LOAD( "k19_b12.bin", 0x100000, 0x80000, CRC(ea6bdc17) SHA1(a7c22370f8adc5b479283f1ff831f493df78282f) )	/* characters (set 1) */
	ROM_LOAD( "j19_b08.bin", 0x180000, 0x40000, CRC(f9387966) SHA1(470ecc4a5a3edd08d5e0ab10b0c590db1968fb0a) )	/* characters (set 2) */
	/* second half empty */

	ROM_REGION( 0x200000, REGION_GFX2, 0 ) /* graphics */
	ROM_LOAD( "k08_b10.bin", 0x000000, 0x80000, CRC(0b1035b1) SHA1(db04020761386e79249762cd1540208375c38c7f) )	/* sprites (set 1) */
	ROM_LOAD( "j08_b06.bin", 0x080000, 0x40000, CRC(081a0566) SHA1(3a4aa14178fe76a030224743c9e9cd974e08bd79) )	/* sprites (set 2) */
	/* second half empty */
	ROM_LOAD( "k02_b09.bin", 0x100000, 0x80000, CRC(e76b3c19) SHA1(6838e07460b3eaaeb129208ad0696c8019bd63d9) )	/* sprites (set 1) */
	ROM_LOAD( "j02_b05.bin", 0x180000, 0x40000, CRC(19a261f2) SHA1(b0518fad833b3e613e0201d5d9cab73dc5e78e1d) )	/* sprites (set 2) */
	/* second half empty */

	ROM_REGION( 0x0100, REGION_PROMS, 0 )
	ROM_LOAD( "821a08.h14",  0x0000, 0x0100, CRC(7da55800) SHA1(3826f73569c8ae0431510a355bdfa082152b74a5) )	/* priority encoder (not used) */

	ROM_REGION( 0x40000, REGION_SOUND1, 0 ) /* samples for 007232 */
	ROM_LOAD( "875b04.bin",  0x00000, 0x40000, CRC(4e209ac8) SHA1(09d9eaae61bfd04bf318555ccd44d7371571d86d) )
ROM_END

ROM_START( aliens3 )
	ROM_REGION( 0x38000, REGION_CPU1, 0 ) /* code + banked roms */
	ROM_LOAD( "875_w3_2.e24", 0x10000, 0x08000, CRC(f917f7b5) SHA1(ab95ad40c82f11572bbaa03d76dae35f76bacf0c) )
	ROM_CONTINUE(            0x08000, 0x08000 )
	ROM_LOAD( "875_w3_1.c24", 0x18000, 0x20000, CRC(3c0006fb) SHA1(e8730d50c358e7321dd676c74368fe44b9bbe5b2) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 ) /* 64k for the sound CPU */
	ROM_LOAD( "g04_b03.bin", 0x00000, 0x08000, CRC(1ac4d283) SHA1(2253f1f39c7edb6cef438b3d97f3af67a1f491ff) )

	ROM_REGION( 0x200000, REGION_GFX1, 0 ) /* graphics */
	ROM_LOAD( "k13_b11.bin", 0x000000, 0x80000, CRC(89c5c885) SHA1(02a1581579b6ef816e04bec312a7b3ae7c7e84f8) )	/* characters (set 1) */
	ROM_LOAD( "j13_b07.bin", 0x080000, 0x40000, CRC(e9c56d66) SHA1(1f58949d5391aef002a6e1ee7034e57bf99cee61) )	/* characters (set 2) */
	/* second half empty */
	ROM_LOAD( "k19_b12.bin", 0x100000, 0x80000, CRC(ea6bdc17) SHA1(a7c22370f8adc5b479283f1ff831f493df78282f) )	/* characters (set 1) */
	ROM_LOAD( "j19_b08.bin", 0x180000, 0x40000, CRC(f9387966) SHA1(470ecc4a5a3edd08d5e0ab10b0c590db1968fb0a) )	/* characters (set 2) */
	/* second half empty */

	ROM_REGION( 0x200000, REGION_GFX2, 0 ) /* graphics */
	ROM_LOAD( "k08_b10.bin", 0x000000, 0x80000, CRC(0b1035b1) SHA1(db04020761386e79249762cd1540208375c38c7f) )	/* sprites (set 1) */
	ROM_LOAD( "j08_b06.bin", 0x080000, 0x40000, CRC(081a0566) SHA1(3a4aa14178fe76a030224743c9e9cd974e08bd79) )	/* sprites (set 2) */
	/* second half empty */
	ROM_LOAD( "k02_b09.bin", 0x100000, 0x80000, CRC(e76b3c19) SHA1(6838e07460b3eaaeb129208ad0696c8019bd63d9) )	/* sprites (set 1) */
	ROM_LOAD( "j02_b05.bin", 0x180000, 0x40000, CRC(19a261f2) SHA1(b0518fad833b3e613e0201d5d9cab73dc5e78e1d) )	/* sprites (set 2) */
	/* second half empty */

	ROM_REGION( 0x0100, REGION_PROMS, 0 )
	ROM_LOAD( "821a08.h14",  0x0000, 0x0100, CRC(7da55800) SHA1(3826f73569c8ae0431510a355bdfa082152b74a5) )	/* priority encoder (not used) */

	ROM_REGION( 0x40000, REGION_SOUND1, 0 ) /* samples for 007232 */
	ROM_LOAD( "875b04.bin",  0x00000, 0x40000, CRC(4e209ac8) SHA1(09d9eaae61bfd04bf318555ccd44d7371571d86d) )
ROM_END

ROM_START( aliensu )
	ROM_REGION( 0x38000, REGION_CPU1, 0 ) /* code + banked roms */
	ROM_LOAD( "e24_n02.bin", 0x10000, 0x08000, CRC(24dd612e) SHA1(35bceb3045cd0bd9d107312b371fb60dcf3f1272) )
	ROM_CONTINUE(            0x08000, 0x08000 )
	ROM_LOAD( "c24_n01.bin", 0x18000, 0x20000, CRC(106cf59c) SHA1(78622adc02055d31cd587c83b23a6cde30c9bc22) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 ) /* 64k for the sound CPU */
	ROM_LOAD( "g04_b03.bin", 0x00000, 0x08000, CRC(1ac4d283) SHA1(2253f1f39c7edb6cef438b3d97f3af67a1f491ff) )

	ROM_REGION( 0x200000, REGION_GFX1, 0 ) /* graphics */
	ROM_LOAD( "k13_b11.bin", 0x000000, 0x80000, CRC(89c5c885) SHA1(02a1581579b6ef816e04bec312a7b3ae7c7e84f8) )	/* characters (set 1) */
	ROM_LOAD( "j13_b07.bin", 0x080000, 0x40000, CRC(e9c56d66) SHA1(1f58949d5391aef002a6e1ee7034e57bf99cee61) )	/* characters (set 2) */
	/* second half empty */
	ROM_LOAD( "k19_b12.bin", 0x100000, 0x80000, CRC(ea6bdc17) SHA1(a7c22370f8adc5b479283f1ff831f493df78282f) )	/* characters (set 1) */
	ROM_LOAD( "j19_b08.bin", 0x180000, 0x40000, CRC(f9387966) SHA1(470ecc4a5a3edd08d5e0ab10b0c590db1968fb0a) )	/* characters (set 2) */
	/* second half empty */

	ROM_REGION( 0x200000, REGION_GFX2, 0 ) /* graphics */
	ROM_LOAD( "k08_b10.bin", 0x000000, 0x80000, CRC(0b1035b1) SHA1(db04020761386e79249762cd1540208375c38c7f) )	/* sprites (set 1) */
	ROM_LOAD( "j08_b06.bin", 0x080000, 0x40000, CRC(081a0566) SHA1(3a4aa14178fe76a030224743c9e9cd974e08bd79) )	/* sprites (set 2) */
	/* second half empty */
	ROM_LOAD( "k02_b09.bin", 0x100000, 0x80000, CRC(e76b3c19) SHA1(6838e07460b3eaaeb129208ad0696c8019bd63d9) )	/* sprites (set 1) */
	ROM_LOAD( "j02_b05.bin", 0x180000, 0x40000, CRC(19a261f2) SHA1(b0518fad833b3e613e0201d5d9cab73dc5e78e1d) )	/* sprites (set 2) */
	/* second half empty */

	ROM_REGION( 0x0100, REGION_PROMS, 0 )
	ROM_LOAD( "821a08.h14",  0x0000, 0x0100, CRC(7da55800) SHA1(3826f73569c8ae0431510a355bdfa082152b74a5) )	/* priority encoder (not used) */

	ROM_REGION( 0x40000, REGION_SOUND1, 0 ) /* samples for 007232 */
	ROM_LOAD( "875b04.bin",  0x00000, 0x40000, CRC(4e209ac8) SHA1(09d9eaae61bfd04bf318555ccd44d7371571d86d) )
ROM_END

ROM_START( aliensj )
	ROM_REGION( 0x38000, REGION_CPU1, 0 ) /* code + banked roms */
	ROM_LOAD( "875m02.e24",  0x10000, 0x08000, CRC(54a774e5) SHA1(b6413b2199f863cae1c6fcef766989162cd4b95e) )
	ROM_CONTINUE(            0x08000, 0x08000 )
	ROM_LOAD( "875m01.c24",  0x18000, 0x20000, CRC(1663d3dc) SHA1(706bdf3daa3bda372d94263f3405d67a7ef8dc69) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 ) /* 64k for the sound CPU */
	ROM_LOAD( "875k03.g4",   0x00000, 0x08000, CRC(bd86264d) SHA1(345fd666daf8a29ef314b14306c1a976cb159bed) )

	ROM_REGION( 0x200000, REGION_GFX1, 0 ) /* graphics */
	ROM_LOAD( "k13_b11.bin", 0x000000, 0x80000, CRC(89c5c885) SHA1(02a1581579b6ef816e04bec312a7b3ae7c7e84f8) )	/* characters (set 1) */
	ROM_LOAD( "j13_b07.bin", 0x080000, 0x40000, CRC(e9c56d66) SHA1(1f58949d5391aef002a6e1ee7034e57bf99cee61) )	/* characters (set 2) */
	/* second half empty */
	ROM_LOAD( "k19_b12.bin", 0x100000, 0x80000, CRC(ea6bdc17) SHA1(a7c22370f8adc5b479283f1ff831f493df78282f) )	/* characters (set 1) */
	ROM_LOAD( "j19_b08.bin", 0x180000, 0x40000, CRC(f9387966) SHA1(470ecc4a5a3edd08d5e0ab10b0c590db1968fb0a) )	/* characters (set 2) */
	/* second half empty */

	ROM_REGION( 0x200000, REGION_GFX2, 0 ) /* graphics */
	ROM_LOAD( "k08_b10.bin", 0x000000, 0x80000, CRC(0b1035b1) SHA1(db04020761386e79249762cd1540208375c38c7f) )	/* sprites (set 1) */
	ROM_LOAD( "j08_b06.bin", 0x080000, 0x40000, CRC(081a0566) SHA1(3a4aa14178fe76a030224743c9e9cd974e08bd79) )	/* sprites (set 2) */
	/* second half empty */
	ROM_LOAD( "k02_b09.bin", 0x100000, 0x80000, CRC(e76b3c19) SHA1(6838e07460b3eaaeb129208ad0696c8019bd63d9) )	/* sprites (set 1) */
	ROM_LOAD( "j02_b05.bin", 0x180000, 0x40000, CRC(19a261f2) SHA1(b0518fad833b3e613e0201d5d9cab73dc5e78e1d) )	/* sprites (set 2) */
	/* second half empty */

	ROM_REGION( 0x0100, REGION_PROMS, 0 )
	ROM_LOAD( "821a08.h14",  0x0000, 0x0100, CRC(7da55800) SHA1(3826f73569c8ae0431510a355bdfa082152b74a5) )	/* priority encoder (not used) */

	ROM_REGION( 0x40000, REGION_SOUND1, 0 ) /* samples for 007232 */
	ROM_LOAD( "875b04.bin",  0x00000, 0x40000, CRC(4e209ac8) SHA1(09d9eaae61bfd04bf318555ccd44d7371571d86d) )
ROM_END

ROM_START( aliensj2 )
	ROM_REGION( 0x38000, REGION_CPU1, 0 ) /* code + banked roms */
	ROM_LOAD( "875_j2_2.e24",  0x10000, 0x08000, CRC(4bb84952) SHA1(ca40a7181f11d6c34c26b65f8d4a1d1df2c7fb48) )
	ROM_CONTINUE(            0x08000, 0x08000 )
	ROM_LOAD( "875m01.c24",  0x18000, 0x20000, CRC(1663d3dc) SHA1(706bdf3daa3bda372d94263f3405d67a7ef8dc69) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 ) /* 64k for the sound CPU */
	ROM_LOAD( "875k03.g4",   0x00000, 0x08000, CRC(bd86264d) SHA1(345fd666daf8a29ef314b14306c1a976cb159bed) )

	ROM_REGION( 0x200000, REGION_GFX1, 0 ) /* graphics */
	ROM_LOAD( "k13_b11.bin", 0x000000, 0x80000, CRC(89c5c885) SHA1(02a1581579b6ef816e04bec312a7b3ae7c7e84f8) )	/* characters (set 1) */
	ROM_LOAD( "j13_b07.bin", 0x080000, 0x40000, CRC(e9c56d66) SHA1(1f58949d5391aef002a6e1ee7034e57bf99cee61) )	/* characters (set 2) */
	/* second half empty */
	ROM_LOAD( "k19_b12.bin", 0x100000, 0x80000, CRC(ea6bdc17) SHA1(a7c22370f8adc5b479283f1ff831f493df78282f) )	/* characters (set 1) */
	ROM_LOAD( "j19_b08.bin", 0x180000, 0x40000, CRC(f9387966) SHA1(470ecc4a5a3edd08d5e0ab10b0c590db1968fb0a) )	/* characters (set 2) */
	/* second half empty */

	ROM_REGION( 0x200000, REGION_GFX2, 0 ) /* graphics */
	ROM_LOAD( "k08_b10.bin", 0x000000, 0x80000, CRC(0b1035b1) SHA1(db04020761386e79249762cd1540208375c38c7f) )	/* sprites (set 1) */
	ROM_LOAD( "j08_b06.bin", 0x080000, 0x40000, CRC(081a0566) SHA1(3a4aa14178fe76a030224743c9e9cd974e08bd79) )	/* sprites (set 2) */
	/* second half empty */
	ROM_LOAD( "k02_b09.bin", 0x100000, 0x80000, CRC(e76b3c19) SHA1(6838e07460b3eaaeb129208ad0696c8019bd63d9) )	/* sprites (set 1) */
	ROM_LOAD( "j02_b05.bin", 0x180000, 0x40000, CRC(19a261f2) SHA1(b0518fad833b3e613e0201d5d9cab73dc5e78e1d) )	/* sprites (set 2) */
	/* second half empty */

	ROM_REGION( 0x0100, REGION_PROMS, 0 )
	ROM_LOAD( "821a08.h14",  0x0000, 0x0100, CRC(7da55800) SHA1(3826f73569c8ae0431510a355bdfa082152b74a5) )	/* priority encoder (not used) */

	ROM_REGION( 0x40000, REGION_SOUND1, 0 ) /* samples for 007232 */
	ROM_LOAD( "875b04.bin",  0x00000, 0x40000, CRC(4e209ac8) SHA1(09d9eaae61bfd04bf318555ccd44d7371571d86d) )
ROM_END

/***************************************************************************

  Game driver(s)

***************************************************************************/

static void aliens_banking( int lines )
{
	unsigned char *RAM = memory_region(REGION_CPU1);
	int offs = 0x18000;


	if (lines & 0x10) offs -= 0x8000;

	offs += (lines & 0x0f)*0x2000;
	memory_set_bankptr( 1, &RAM[offs] );
}

static MACHINE_RESET( aliens )
{
	unsigned char *RAM = memory_region(REGION_CPU1);

	cpunum_set_info_fct(0, CPUINFO_PTR_KONAMI_SETLINES_CALLBACK, (genf *)aliens_banking);

	/* init the default bank */
	memory_set_bankptr( 1, &RAM[0x10000] );
}



static DRIVER_INIT( aliens )
{
	konami_rom_deinterleave_2(REGION_GFX1);
	konami_rom_deinterleave_2(REGION_GFX2);
}



GAME( 1990, aliens,   0,      aliens, aliens, aliens, ROT0, "Konami", "Aliens (World set 1)", 0 )
GAME( 1990, aliens2,  aliens, aliens, aliens, aliens, ROT0, "Konami", "Aliens (World set 2)", 0 )
GAME( 1990, aliens3,  aliens, aliens, aliens, aliens, ROT0, "Konami", "Aliens (World set 3)", 0 )
GAME( 1990, aliensu,  aliens, aliens, aliens, aliens, ROT0, "Konami", "Aliens (US)", 0 )
GAME( 1990, aliensj,  aliens, aliens, aliens, aliens, ROT0, "Konami", "Aliens (Japan set 1)", 0 )
GAME( 1990, aliensj2, aliens, aliens, aliens, aliens, ROT0, "Konami", "Aliens (Japan set 2)", 0 )
