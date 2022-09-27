/***************************************************************************

Atari Triple Hunt Driver

  Calibrate controls in service mode the first time you run this game.

To Do:
 The two 8-Tracks need to be found and recorded.  And then tape emulation
 needs to be added.  1 tape is for Witch Hunt.  The other is used for
 both Bear and Racoon Hunt.
 The 3 different overlays need to be found, scanned and added as artwork
 to really make the game complete.

***************************************************************************/

#include "driver.h"
#include "triplhnt.h"
#include "sound/discrete.h"

static UINT8 triplhnt_cmos[16];
static UINT8 triplhnt_da_latch;
static UINT8 triplhnt_misc_flags;
static UINT8 triplhnt_cmos_latch;
static UINT8 triplhnt_hit_code;


static DRIVER_INIT( triplhnt )
{
	generic_nvram = triplhnt_cmos;
	generic_nvram_size = sizeof triplhnt_cmos;
}


void triplhnt_hit_callback(int code)
{
	triplhnt_hit_code = code;

	cpunum_set_input_line(0, 0, HOLD_LINE);
}


static void triplhnt_update_misc(int offset)
{
	UINT8 bit = offset >> 1;

	/* BIT0 => UNUSED      */
	/* BIT1 => LAMP        */
	/* BIT2 => SCREECH     */
	/* BIT3 => LOCKOUT     */
	/* BIT4 => SPRITE ZOOM */
	/* BIT5 => CMOS WRITE  */
	/* BIT6 => TAPE CTRL   */
	/* BIT7 => SPRITE BANK */

	if (offset & 1)
	{
		triplhnt_misc_flags |= 1 << bit;

		if (bit == 5)
		{
			triplhnt_cmos[triplhnt_cmos_latch] = triplhnt_da_latch;
		}
	}
	else
	{
		triplhnt_misc_flags &= ~(1 << bit);
	}

	triplhnt_sprite_zoom = (triplhnt_misc_flags >> 4) & 1;
	triplhnt_sprite_bank = (triplhnt_misc_flags >> 7) & 1;

	set_led_status(0, triplhnt_misc_flags & 0x02);

	coin_lockout_w(0, !(triplhnt_misc_flags & 0x08));
	coin_lockout_w(1, !(triplhnt_misc_flags & 0x08));

	discrete_sound_w(TRIPLHNT_SCREECH_EN, triplhnt_misc_flags & 0x04);	// screech
	discrete_sound_w(TRIPLHNT_LAMP_EN, triplhnt_misc_flags & 0x02);	// Lamp is used to reset noise
	discrete_sound_w(TRIPLHNT_BEAR_EN, triplhnt_misc_flags & 0x80);	// bear
}


WRITE8_HANDLER( triplhnt_misc_w )
{
	triplhnt_update_misc(offset);
}


READ8_HANDLER( triplhnt_cmos_r )
{
	triplhnt_cmos_latch = offset;

	return triplhnt_cmos[triplhnt_cmos_latch] ^ 15;
}


READ8_HANDLER( triplhnt_input_port_4_r )
{
	watchdog_reset_w(0, 0);

	return readinputport(4);
}


READ8_HANDLER( triplhnt_misc_r )
{
	triplhnt_update_misc(offset);

	return readinputport(7) | triplhnt_hit_code;
}


READ8_HANDLER( triplhnt_da_latch_r )
{
	int cross_x = readinputport(8);
	int cross_y = readinputport(9);

	triplhnt_da_latch = offset;

	/* the following is a slight simplification */

	return (offset & 1) ? cross_x : cross_y;
}


static ADDRESS_MAP_START( triplhnt_readmem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x00ff) AM_READ(MRA8_RAM) AM_MIRROR(0x300)
	AM_RANGE(0x0c00, 0x0c00) AM_READ(input_port_0_r)
	AM_RANGE(0x0c08, 0x0c08) AM_READ(input_port_1_r)
	AM_RANGE(0x0c09, 0x0c09) AM_READ(input_port_2_r)
	AM_RANGE(0x0c0a, 0x0c0a) AM_READ(input_port_3_r)
	AM_RANGE(0x0c0b, 0x0c0b) AM_READ(triplhnt_input_port_4_r)
	AM_RANGE(0x0c10, 0x0c1f) AM_READ(triplhnt_da_latch_r)
	AM_RANGE(0x0c20, 0x0c2f) AM_READ(triplhnt_cmos_r)
	AM_RANGE(0x0c30, 0x0c3f) AM_READ(triplhnt_misc_r)
	AM_RANGE(0x0c40, 0x0c40) AM_READ(input_port_5_r)
	AM_RANGE(0x0c48, 0x0c48) AM_READ(input_port_6_r)
	AM_RANGE(0x7000, 0x7fff) AM_READ(MRA8_ROM) /* program */
	AM_RANGE(0xf800, 0xffff) AM_READ(MRA8_ROM) /* program mirror */
ADDRESS_MAP_END

static ADDRESS_MAP_START( triplhnt_writemem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x00ff) AM_WRITE(MWA8_RAM) AM_MIRROR(0x300)
	AM_RANGE(0x0400, 0x04ff) AM_WRITE(MWA8_RAM) AM_BASE(&triplhnt_playfield_ram)
	AM_RANGE(0x0800, 0x080f) AM_WRITE(MWA8_RAM) AM_BASE(&triplhnt_vpos_ram)
	AM_RANGE(0x0810, 0x081f) AM_WRITE(MWA8_RAM) AM_BASE(&triplhnt_hpos_ram)
	AM_RANGE(0x0820, 0x082f) AM_WRITE(MWA8_RAM) AM_BASE(&triplhnt_orga_ram)
	AM_RANGE(0x0830, 0x083f) AM_WRITE(MWA8_RAM) AM_BASE(&triplhnt_code_ram)
	AM_RANGE(0x0c30, 0x0c3f) AM_WRITE(triplhnt_misc_w)
	AM_RANGE(0x7000, 0x7fff) AM_WRITE(MWA8_ROM) /* program */
	AM_RANGE(0xf800, 0xffff) AM_WRITE(MWA8_ROM) /* program mirror */
ADDRESS_MAP_END


INPUT_PORTS_START( triplhnt )
	PORT_START /* 0C00 */
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_COIN1 )

	PORT_START /* 0C08 */
	PORT_DIPNAME( 0xc0, 0x00, "Play Time" )
	PORT_DIPSETTING( 0x00, "32 seconds / 16 raccoons" )
	PORT_DIPSETTING( 0x40, "64 seconds / 32 raccoons" )
	PORT_DIPSETTING( 0x80, "96 seconds / 48 raccoons" )
	PORT_DIPSETTING( 0xc0, "128 seconds / 64 raccoons" )

	PORT_START /* 0C09 */
	PORT_DIPNAME( 0xc0, 0x40, "Game Select" )
	PORT_DIPSETTING( 0x00, "Hit the Bear" )
	PORT_DIPSETTING( 0x40, "Witch Hunt" )
	PORT_DIPSETTING( 0xc0, "Raccoon Hunt" )

	PORT_START /* 0C0A */
	PORT_DIPNAME( 0xc0, 0x00, DEF_STR( Coinage ))
	PORT_DIPSETTING( 0x40, DEF_STR( 2C_1C ))
	PORT_DIPSETTING( 0x00, DEF_STR( 1C_1C ))
	PORT_DIPSETTING( 0x80, DEF_STR( 1C_2C ))

	PORT_START /* 0C0B */
	PORT_DIPNAME( 0x80, 0x00, "Extended Play" )
	PORT_DIPSETTING( 0x80, DEF_STR( Off ))
	PORT_DIPSETTING( 0x00, DEF_STR( On ))

	PORT_START /* 0C40 */
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_TILT )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_COIN2 )

	PORT_START /* 0C48 */
// default to service enabled to make users calibrate gun
//  PORT_SERVICE( 0x40, IP_ACTIVE_LOW )
	PORT_BIT(    0x40, 0x00, IPT_DIPSWITCH_NAME ) PORT_NAME( DEF_STR( Service_Mode )) PORT_TOGGLE PORT_CODE(KEYCODE_F2)
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON1 )

	PORT_START
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_VBLANK )

	PORT_START
	PORT_BIT( 0xff, 0x80, IPT_LIGHTGUN_X ) PORT_MINMAX(0x00,0xff) PORT_SENSITIVITY(25) PORT_KEYDELTA(15)

	PORT_START
	PORT_BIT( 0xff, 0x78, IPT_LIGHTGUN_Y ) PORT_MINMAX(0x00,0xef) PORT_SENSITIVITY(25) PORT_KEYDELTA(15)

	PORT_START		/* 10 */
	PORT_ADJUSTER( 35, "Bear Roar Frequency" )
INPUT_PORTS_END


static const gfx_layout triplhnt_small_sprite_layout =
{
	32, 32,   /* width, height */
	16,       /* total         */
	2,        /* planes        */
	          /* plane offsets */
	{ 0x0000, 0x4000 },
	{
		0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
		0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F,
		0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
		0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F
	},
	{
		0x000, 0x020, 0x040, 0x060, 0x080, 0x0A0, 0x0C0, 0x0E0,
		0x100, 0x120, 0x140, 0x160, 0x180, 0x1A0, 0x1C0, 0x1E0,
		0x200, 0x220, 0x240, 0x260, 0x280, 0x2A0, 0x2C0, 0x2E0,
		0x300, 0x320, 0x340, 0x360, 0x380, 0x3A0, 0x3C0, 0x3E0
	},
	0x400     /* increment */
};


static const UINT32 triplhnt_large_sprite_layout_xoffset[64] =
{
		0x00, 0x00, 0x01, 0x01, 0x02, 0x02, 0x03, 0x03,
		0x04, 0x04, 0x05, 0x05, 0x06, 0x06, 0x07, 0x07,
		0x08, 0x08, 0x09, 0x09, 0x0A, 0x0A, 0x0B, 0x0B,
		0x0C, 0x0C, 0x0D, 0x0D, 0x0E, 0x0E, 0x0F, 0x0F,
		0x10, 0x10, 0x11, 0x11, 0x12, 0x12, 0x13, 0x13,
		0x14, 0x14, 0x15, 0x15, 0x16, 0x16, 0x17, 0x17,
		0x18, 0x18, 0x19, 0x19, 0x1A, 0x1A, 0x1B, 0x1B,
		0x1C, 0x1C, 0x1D, 0x1D, 0x1E, 0x1E, 0x1F, 0x1F
};

static const UINT32 triplhnt_large_sprite_layout_yoffset[64] =
{
		0x000, 0x000, 0x020, 0x020, 0x040, 0x040, 0x060, 0x060,
		0x080, 0x080, 0x0A0, 0x0A0, 0x0C0, 0x0C0, 0x0E0, 0x0E0,
		0x100, 0x100, 0x120, 0x120, 0x140, 0x140, 0x160, 0x160,
		0x180, 0x180, 0x1A0, 0x1A0, 0x1C0, 0x1C0, 0x1E0, 0x1E0,
		0x200, 0x200, 0x220, 0x220, 0x240, 0x240, 0x260, 0x260,
		0x280, 0x280, 0x2A0, 0x2A0, 0x2C0, 0x2C0, 0x2E0, 0x2E0,
		0x300, 0x300, 0x320, 0x320, 0x340, 0x340, 0x360, 0x360,
		0x380, 0x380, 0x3A0, 0x3A0, 0x3C0, 0x3C0, 0x3E0, 0x3E0
};

static const gfx_layout triplhnt_large_sprite_layout =
{
	64, 64,   /* width, height */
	16,       /* total         */
	2,        /* planes        */
	{ 0x0000, 0x4000 },
	EXTENDED_XOFFS,
	EXTENDED_YOFFS,
	0x400,
	triplhnt_large_sprite_layout_xoffset,
	triplhnt_large_sprite_layout_yoffset
};


static const gfx_layout triplhnt_tile_layout =
{
	16, 16,   /* width, height */
	64,       /* total         */
	1,        /* planes        */
	{ 0 },    /* plane offsets */
	{
		0, 0, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6, 6, 7, 7
	},
	{
		0x00, 0x00, 0x08, 0x08, 0x10, 0x10, 0x18, 0x18,
		0x20, 0x20, 0x28, 0x28, 0x30, 0x30, 0x38, 0x38
	},
	0x40      /* increment */
};


static const gfx_decode triplhnt_gfx_decode_info[] =
{
	{ REGION_GFX1, 0, &triplhnt_small_sprite_layout, 0, 1 },
	{ REGION_GFX1, 0, &triplhnt_large_sprite_layout, 0, 1 },
	{ REGION_GFX2, 0, &triplhnt_tile_layout, 4, 2 },
	{ -1 } /* end of array */
};


static PALETTE_INIT( triplhnt )
{
	palette_set_color(0, 0xAF, 0xAF, 0xAF);  /* sprites */
	palette_set_color(1, 0x00, 0x00, 0x00);
	palette_set_color(2, 0xFF, 0xFF, 0xFF);
	palette_set_color(3, 0x50, 0x50, 0x50);
	palette_set_color(4, 0x00, 0x00, 0x00);  /* tiles */
	palette_set_color(5, 0x3F, 0x3F, 0x3F);
	palette_set_color(6, 0x00, 0x00, 0x00);
	palette_set_color(7, 0x3F, 0x3F, 0x3F);
}


static MACHINE_DRIVER_START( triplhnt )

/* basic machine hardware */
	MDRV_CPU_ADD(M6800, 800000)
	MDRV_CPU_PROGRAM_MAP(triplhnt_readmem, triplhnt_writemem)
	MDRV_CPU_VBLANK_INT(irq0_line_hold, 1)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION((int) ((22. * 1000000) / (262. * 60) + 0.5))

	MDRV_NVRAM_HANDLER(generic_0fill)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(256, 262)
	MDRV_VISIBLE_AREA(0, 255, 0, 239)
	MDRV_GFXDECODE(triplhnt_gfx_decode_info)
	MDRV_PALETTE_LENGTH(8)
	MDRV_PALETTE_INIT(triplhnt)
	MDRV_VIDEO_START(triplhnt)
	MDRV_VIDEO_UPDATE(triplhnt)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")

	MDRV_SOUND_ADD_TAG("discrete", DISCRETE, 0)
	MDRV_SOUND_CONFIG(triplhnt_discrete_interface)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.0)
MACHINE_DRIVER_END


ROM_START( triplhnt )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )
	ROM_LOAD_NIB_HIGH( "8404.f1", 0x7000, 0x400, CRC(abc8acd5) SHA1(bcef2abc5829829a01aa21776c3deb2e1bf1d4ac) )
	ROM_LOAD_NIB_LOW ( "8408.f2", 0x7000, 0x400, CRC(77fcdd3f) SHA1(ce0196abb8d6510aa9a5308f8efd6442e94272c4) )
	ROM_LOAD_NIB_HIGH( "8403.e1", 0x7400, 0x400, CRC(8d756fa1) SHA1(48a74f710b130d9af0c866483d6fc4ecce4a3ac5) )
	ROM_LOAD_NIB_LOW ( "8407.e2", 0x7400, 0x400, CRC(de268f4b) SHA1(937f97421ffb4f0f704402847892382ae8032b7c) )
	ROM_LOAD_NIB_HIGH( "8402.d1", 0x7800, 0x400, CRC(eb75c936) SHA1(48f9d4113a7ab8413a5aacd44b3506afc99d26ce) )
	ROM_LOAD_NIB_LOW ( "8406.d2", 0x7800, 0x400, CRC(e7ab1186) SHA1(7185fb837966bfb4aa70be3dd948d44f356b0452) )
	ROM_LOAD_NIB_HIGH( "8401.c1", 0x7C00, 0x400, CRC(7461b05e) SHA1(16573ae655c306a38ff0f29a3c3285d636907f38) )
	ROM_RELOAD(                   0xFC00, 0x400 )
	ROM_LOAD_NIB_LOW ( "8405.c2", 0x7C00, 0x400, CRC(ba370b97) SHA1(5d799ce6ae56c315ff0abedea7ad9204bacc266b) )
	ROM_RELOAD(                   0xFC00, 0x400 )

	ROM_REGION( 0x1000, REGION_GFX1, ROMREGION_DISPOSE )  /* sprites */
	ROM_LOAD( "8423.n1", 0x0000, 0x800, CRC(9937d0da) SHA1(abb906c2d9869b09be5172cc7639bb9cda38831b) )
	ROM_LOAD( "8422.r1", 0x0800, 0x800, CRC(803621dd) SHA1(ffbd7f87a86477e5eb94f12fc20a837128a02442) )

	ROM_REGION( 0x200, REGION_GFX2, ROMREGION_DISPOSE )   /* tiles */
	ROM_LOAD_NIB_HIGH( "8409.l3", 0x0000, 0x200, CRC(ec304172) SHA1(ccbf7e117fef7fa4288e3bf68f1a150b3a492ce6) )
	ROM_LOAD_NIB_LOW ( "8410.m3", 0x0000, 0x200, CRC(f75a1b08) SHA1(81b4733194462cd4cef7f4221ecb7abd1556b871) )
ROM_END


GAME( 1977, triplhnt, 0, triplhnt, triplhnt, triplhnt, 0, "Atari", "Triple Hunt", GAME_IMPERFECT_SOUND )
