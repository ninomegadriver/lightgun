/***************************************************************************

Dragon Buster (c) 1984 Namco
Sky Kid       (c) 1985 Namco

Driver by Manuel Abadia <manu@teleline.es>

Notes:
-----
- skykidd: the MCU external program expects a different version of the
  internal ROM, which hasn't been dumped. 0xffed should be 0xfd, while it is
  0xff in the available dump. To get the game running, we patch the ROM.
  Apart from the different MCU code (and the required checksum adjustments),
  this version is identical to skykido.

***************************************************************************/

#include "driver.h"
#include "cpu/m6800/m6800.h"
#include "sound/namco.h"

extern UINT8 *skykid_textram, *skykid_videoram, *skykid_spriteram;

/* from vidhrdw/skykid.c */
VIDEO_START( skykid );
READ8_HANDLER( skykid_videoram_r );
WRITE8_HANDLER( skykid_videoram_w );
READ8_HANDLER( skykid_textram_r );
WRITE8_HANDLER( skykid_textram_w );
WRITE8_HANDLER( skykid_scroll_x_w );
WRITE8_HANDLER( skykid_scroll_y_w );
WRITE8_HANDLER( skykid_flipscreen_priority_w );
VIDEO_UPDATE( skykid );
VIDEO_UPDATE( drgnbstr );
PALETTE_INIT( skykid );


static int inputport_selected;

static WRITE8_HANDLER( inputport_select_w )
{
	if ((data & 0xe0) == 0x60)
		inputport_selected = data & 0x07;
	else if ((data & 0xe0) == 0xc0)
	{
		coin_lockout_global_w(~data & 1);
		coin_counter_w(0,data & 2);
		coin_counter_w(1,data & 4);
	}
}

static READ8_HANDLER( inputport_r )
{
	switch (inputport_selected)
	{
		case 0x00:	/* DSW B (bits 0-4) */
			return (readinputport(1) & 0xf8) >> 3; break;
		case 0x01:	/* DSW B (bits 5-7), DSW A (bits 0-1) */
			return ((readinputport(1) & 0x07) << 2) | ((readinputport(0) & 0xc0) >> 6); break;
		case 0x02:	/* DSW A (bits 2-6) */
			return (readinputport(0) & 0x3e) >> 1; break;
		case 0x03:	/* DSW A (bit 7), DSW C (bits 0-3) */
			return ((readinputport(0) & 0x01) << 4) | (readinputport(2) & 0x0f); break;
		case 0x04:	/* coins, start */
			return readinputport(3); break;
		case 0x05:	/* 2P controls */
			return readinputport(5); break;
		case 0x06:	/* 1P controls */
			return readinputport(4); break;
		default:
			return 0xff;
	}
}

static WRITE8_HANDLER( skykid_led_w )
{
	set_led_status(0,data & 0x08);
	set_led_status(1,data & 0x10);
}

static WRITE8_HANDLER( skykid_subreset_w )
{
	int bit = !BIT(offset,11);
	cpunum_set_input_line(1, INPUT_LINE_RESET, bit ? CLEAR_LINE : ASSERT_LINE);
}

static WRITE8_HANDLER( skykid_bankswitch_w )
{
	int bit = !BIT(offset,11);
	UINT8 *rom = memory_region(REGION_CPU1) + 0x10000;

	memory_set_bankptr(1,rom + 0x2000 * bit);
}

static WRITE8_HANDLER( skykid_irq_1_ctrl_w )
{
	int bit = !BIT(offset,11);
	cpu_interrupt_enable(0,bit);
	if (!bit)
		cpunum_set_input_line(0, 0, CLEAR_LINE);
}

static WRITE8_HANDLER( skykid_irq_2_ctrl_w )
{
	int bit = !BIT(offset,13);
	cpu_interrupt_enable(1,bit);
	if (!bit)
		cpunum_set_input_line(1, 0, CLEAR_LINE);
}



static ADDRESS_MAP_START( skykid_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x1fff) AM_READ(MRA8_BANK1)				/* banked ROM */
	AM_RANGE(0x2000, 0x2fff) AM_READWRITE(skykid_videoram_r,skykid_videoram_w) AM_BASE(&skykid_videoram)/* Video RAM (background) */
	AM_RANGE(0x4000, 0x47ff) AM_READWRITE(skykid_textram_r,skykid_textram_w) AM_BASE(&skykid_textram)	/* video RAM (text layer) */
	AM_RANGE(0x4800, 0x5fff) AM_RAM AM_BASE(&skykid_spriteram)	/* RAM + Sprite RAM */
	AM_RANGE(0x6000, 0x60ff) AM_WRITE(skykid_scroll_y_w)		/* Y scroll register map */
	AM_RANGE(0x6200, 0x63ff) AM_WRITE(skykid_scroll_x_w)		/* X scroll register map */
	AM_RANGE(0x6800, 0x6bff) AM_READWRITE(namcos1_cus30_r,namcos1_cus30_w) AM_BASE(&namco_wavedata)/* PSG device, shared RAM */
	AM_RANGE(0x7000, 0x7fff) AM_WRITE(skykid_irq_1_ctrl_w)		/* IRQ control */
	AM_RANGE(0x7800, 0x7fff) AM_READ(watchdog_reset_r)			/* watchdog reset */
	AM_RANGE(0x8000, 0xffff) AM_READ(MRA8_ROM)					/* ROM */
	AM_RANGE(0x8000, 0x8fff) AM_WRITE(skykid_subreset_w)		/* MCU control */
	AM_RANGE(0x9000, 0x9fff) AM_WRITE(skykid_bankswitch_w)		/* Bankswitch control */
	AM_RANGE(0xa000, 0xa001) AM_WRITE(skykid_flipscreen_priority_w)	/* flip screen & priority */
ADDRESS_MAP_END

static ADDRESS_MAP_START( mcu_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x001f) AM_READWRITE(hd63701_internal_registers_r, hd63701_internal_registers_w)
	AM_RANGE(0x0080, 0x00ff) AM_RAM
	AM_RANGE(0x1000, 0x13ff) AM_READWRITE(namcos1_cus30_r, namcos1_cus30_w) AM_BASE(&namco_wavedata)		/* PSG device, shared RAM */
	AM_RANGE(0x2000, 0x3fff) AM_WRITE(watchdog_reset_w)		/* watchdog? */
	AM_RANGE(0x4000, 0x7fff) AM_WRITE(skykid_irq_2_ctrl_w)
	AM_RANGE(0x8000, 0xbfff) AM_READ(MRA8_ROM)
	AM_RANGE(0xc000, 0xc7ff) AM_RAM
	AM_RANGE(0xf000, 0xffff) AM_READ(MRA8_ROM)
ADDRESS_MAP_END


static READ8_HANDLER( readFF )
{
	return 0xff;
}

static ADDRESS_MAP_START( mcu_port_map, ADDRESS_SPACE_IO, 8 )
	AM_RANGE(HD63701_PORT1, HD63701_PORT1) AM_READ(inputport_r)			/* input ports read */
	AM_RANGE(HD63701_PORT1, HD63701_PORT1) AM_WRITE(inputport_select_w)	/* input port select */
	AM_RANGE(HD63701_PORT2, HD63701_PORT2) AM_READ(readFF)	/* leds won't work otherwise */
	AM_RANGE(HD63701_PORT2, HD63701_PORT2) AM_WRITE(skykid_led_w)			/* lamps */
ADDRESS_MAP_END



INPUT_PORTS_START( skykid )
	PORT_START	/* DSW A */
	PORT_SERVICE( 0x80, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0x60, 0x60, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x20, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x60, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x40, DEF_STR( 1C_2C ) )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x10, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, "Round Select" )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, "Freeze" )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 1C_2C ) )

	PORT_START	/* DSW B */
	PORT_DIPNAME( 0xc0, 0xc0, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x80, "1" )
	PORT_DIPSETTING(    0x40, "2" )
	PORT_DIPSETTING(    0xc0, "3" )
	PORT_DIPSETTING(    0x00, "5" )
	PORT_DIPNAME( 0x30, 0x30, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x00, "20k every 80k" )
	PORT_DIPSETTING(    0x10, "20k and 80k" )
	PORT_DIPSETTING(    0x20, "30k every 90k" )
	PORT_DIPSETTING(    0x30, "30k and 90k" )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, "Allow Buy In" )
	PORT_DIPSETTING(    0x00, DEF_STR( No ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(2)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(1)
	PORT_BIT( 0xf0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* IN 0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0xe0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* IN 1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(1)
	PORT_BIT( 0xe0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* IN 2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(2)
	PORT_BIT( 0xe0, IP_ACTIVE_LOW, IPT_UNUSED )
INPUT_PORTS_END

INPUT_PORTS_START( drgnbstr )
	PORT_START	/* DSW A */
	PORT_SERVICE( 0x80, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0x60, 0x60, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x20, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x60, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x40, DEF_STR( 1C_2C ) )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x10, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, "Round Skip" )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, "Freeze" )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 1C_2C ) )

	PORT_START	/* DSW B */
	PORT_DIPNAME( 0x80, 0x80, "Spurt Time" )
	PORT_DIPSETTING(    0x80, DEF_STR( Normal ) )
	PORT_DIPSETTING(    0x00, "Difficult" )
	PORT_DIPNAME( 0x40, 0x40, "Level of Monster" )
	PORT_DIPSETTING(    0x40, DEF_STR( Normal ) )
	PORT_DIPSETTING(    0x00, "Difficult" )
	PORT_DIPNAME( 0x30, 0x30, "Starting Vitality" )
	PORT_DIPSETTING(    0x00, "160" )
	PORT_DIPSETTING(    0x30, "128" )
	PORT_DIPSETTING(    0x10, "96" )
	PORT_DIPSETTING(    0x20, "64" )
	PORT_DIPNAME( 0x0c, 0x0c, "Bonus Vitality" ) /* Clear Mountain, Tower, ect... */
	PORT_DIPSETTING(    0x00, "64" )
	PORT_DIPSETTING(    0x08, "48/64" )
	PORT_DIPSETTING(    0x04, "32/64" )
	PORT_DIPSETTING(    0x0c, DEF_STR( None ) )
	PORT_DIPNAME( 0x02, 0x02, "Bonus Level" )    /* Clear Round */
	PORT_DIPSETTING(    0x02, "Full" )
	PORT_DIPSETTING(    0x00, "Partial" )
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Allow_Continue ) )
	PORT_DIPSETTING(    0x01, DEF_STR( No ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Yes ) )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Cocktail ) )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(2)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(1)
	PORT_BIT( 0xf0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* IN 0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0xe0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* IN 1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_4WAY
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_4WAY
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_4WAY
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_4WAY
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(1)
	PORT_BIT( 0xe0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* IN 2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_4WAY PORT_COCKTAIL
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_4WAY PORT_COCKTAIL
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_4WAY PORT_COCKTAIL
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_4WAY PORT_COCKTAIL
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_COCKTAIL
	PORT_BIT( 0xe0, IP_ACTIVE_LOW, IPT_UNUSED )
INPUT_PORTS_END



static const gfx_layout text_layout =
{
	8,8,
	RGN_FRAC(1,1),
	2,
	{ 0, 4 },
	{ 8*8, 8*8+1, 8*8+2, 8*8+3, 0, 1, 2, 3 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	16*8
};

static const gfx_layout tile_layout =
{
	8,8,
	RGN_FRAC(1,1),
	2,
	{ 0, 4 },
	{ 0, 1, 2, 3, 8+0, 8+1, 8+2, 8+3 },
	{ 0*8, 2*8, 4*8, 6*8, 8*8, 10*8, 12*8, 14*8 },
	16*8
};

static const gfx_layout sprite_layout =
{
	16,16,
	RGN_FRAC(1,2),
	3,
	{ RGN_FRAC(1,2)+4, 0, 4 },
	{ 0, 1, 2, 3, 8*8, 8*8+1, 8*8+2, 8*8+3,
			16*8+0, 16*8+1, 16*8+2, 16*8+3, 24*8+0, 24*8+1, 24*8+2, 24*8+3 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8,
			32*8, 33*8, 34*8, 35*8, 36*8, 37*8, 38*8, 39*8 },
	64*8
};

static const gfx_decode gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &text_layout,   0, 64 },
	{ REGION_GFX2, 0, &tile_layout,   64*4, 128 },
	{ REGION_GFX3, 0, &sprite_layout, 64*4+128*4, 64 },
	{-1 }
};



static struct namco_interface namco_interface =
{
	8,					/* number of voices */
	-1,					/* memory region */
	0					/* stereo */
};



static MACHINE_DRIVER_START( skykid )

	/* basic machine hardware */
	MDRV_CPU_ADD(M6809,49152000/32)
	MDRV_CPU_PROGRAM_MAP(skykid_map,0)
	MDRV_CPU_VBLANK_INT(irq0_line_assert,1)

	MDRV_CPU_ADD(HD63701,49152000/32)	/* or compatible 6808 with extra instructions */
	MDRV_CPU_PROGRAM_MAP(mcu_map,0)
	MDRV_CPU_IO_MAP(mcu_port_map,0)
	MDRV_CPU_VBLANK_INT(irq0_line_assert,1)

	MDRV_FRAMES_PER_SECOND(60.606060)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)
	MDRV_INTERLEAVE(100)	/* we need heavy synch */

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(36*8, 28*8)
	MDRV_VISIBLE_AREA(0*8, 36*8-1, 0*8, 28*8-1)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(256)
	MDRV_COLORTABLE_LENGTH(64*4+128*4+64*8)

	MDRV_PALETTE_INIT(skykid)
	MDRV_VIDEO_START(skykid)
	MDRV_VIDEO_UPDATE(skykid)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")

	MDRV_SOUND_ADD(NAMCO_CUS30, 49152000/2048)
	MDRV_SOUND_CONFIG(namco_interface)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.0)
MACHINE_DRIVER_END



ROM_START( skykid )
	ROM_REGION( 0x14000, REGION_CPU1, 0 )	/* 6809 code */
	ROM_LOAD( "sk1_2.6c",     0x08000, 0x4000, CRC(ea8a5822) SHA1(5b13133410bcb7d647e662b476dbfd2edab8aac0) )
	ROM_LOAD( "sk1-1c.6b",    0x0c000, 0x4000, CRC(7abe6c6c) SHA1(7d2631cc6149fa3e02b1355cb899de5474ff5d0a) )
	ROM_LOAD( "sk1_3.6d",     0x10000, 0x4000, CRC(314b8765) SHA1(d90a8a853ce672fe5ee190f07bcb33262c73df3b) )	/* banked ROM */

	ROM_REGION( 0x10000, REGION_CPU2, 0 ) /* MCU code */
	ROM_LOAD( "sk1_4.3c",     0x8000, 0x2000, CRC(a460d0e0) SHA1(7124ffeb3b84b282940dcbf9421ae4934bcce1c8) )	/* subprogram for the MCU */
	ROM_LOAD( "cus60",        0xf000, 0x1000, CRC(6ef08fb3) SHA1(4842590d60035a0059b0899eb2d5f58ae72c2529) )	/* MCU internal code */
																/* Using Pacland code (probably similar) */
	ROM_REGION( 0x02000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "sk1_6.6l",     0x0000, 0x2000, CRC(58b731b9) SHA1(40f7be85914833ce02a734c20d68c0db8b77911d) )	/* chars */

	ROM_REGION( 0x02000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "sk1_5.7e",     0x0000, 0x2000, CRC(c33a498e) SHA1(9f89a514888418a9bebbca341a8cc66e41b58acb) )

	ROM_REGION( 0x10000, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_LOAD( "sk1_8.10n",    0x0000, 0x4000, CRC(44bb7375) SHA1(5b2fa6782671150bab5f3c3ac46b47bc23f3d7e0) )	/* sprites */
	ROM_LOAD( "sk1_7.10m",    0x4000, 0x4000, CRC(3454671d) SHA1(723b26a0f208addc2a22736457cb4be6ab6c69cc) )
	/* 0x8000-0xbfff  will be unpacked from 0x4000-0x5fff */
	ROM_FILL(                 0xc000, 0x4000, 0x00 )	// part of the gfx is 2bpp decoded as 3bpp

	ROM_REGION( 0x0700, REGION_PROMS, 0 )
	ROM_LOAD( "sk1-1.2n",     0x0000, 0x0100, CRC(0218e726) SHA1(8b766162a4783c058d9a1ecf8741673d7ef955fb) )	/* red component */
	ROM_LOAD( "sk1-2.2p",     0x0100, 0x0100, CRC(fc0d5b85) SHA1(d1b13e42e735b24594cf0b840dee8110de23369e) )	/* green component */
	ROM_LOAD( "sk1-3.2r",     0x0200, 0x0100, CRC(d06b620b) SHA1(968a2d62c65e201d521e9efa8fcf6ad15898e4b3) )	/* blue component */
	ROM_LOAD( "sk1-4.5n",     0x0300, 0x0200, CRC(c697ac72) SHA1(3b79755e6cbb22c14fc4affdbd3f4521da1d90e8) )	/* tiles lookup table */
	ROM_LOAD( "sk1-5.6n",     0x0500, 0x0200, CRC(161514a4) SHA1(4488ce60d12be6586e4a1ddbbfd06bf4e7dfaceb) )	/* sprites lookup table */
ROM_END

ROM_START( skykido )
	ROM_REGION( 0x14000, REGION_CPU1, 0 )	/* 6809 code */
	ROM_LOAD( "sk1_2.6c",     0x08000, 0x4000, CRC(ea8a5822) SHA1(5b13133410bcb7d647e662b476dbfd2edab8aac0) )
	ROM_LOAD( "sk1_1.6b",     0x0c000, 0x4000, CRC(070a49d4) SHA1(4b994bde3e34b574bd927843804d2fb1a08d1bdf) )
	ROM_LOAD( "sk1_3.6d",     0x10000, 0x4000, CRC(314b8765) SHA1(d90a8a853ce672fe5ee190f07bcb33262c73df3b) )	/* banked ROM */

	ROM_REGION( 0x10000, REGION_CPU2, 0 ) /* MCU code */
	ROM_LOAD( "sk1_4.3c",     0x8000, 0x2000, CRC(a460d0e0) SHA1(7124ffeb3b84b282940dcbf9421ae4934bcce1c8) )	/* subprogram for the MCU */
	ROM_LOAD( "cus60",        0xf000, 0x1000, CRC(6ef08fb3) SHA1(4842590d60035a0059b0899eb2d5f58ae72c2529) )	/* MCU internal code */
																/* Using Pacland code (probably similar) */
	ROM_REGION( 0x02000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "sk1_6.6l",     0x0000, 0x2000, CRC(58b731b9) SHA1(40f7be85914833ce02a734c20d68c0db8b77911d) )	/* chars */

	ROM_REGION( 0x02000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "sk1_5.7e",     0x0000, 0x2000, CRC(c33a498e) SHA1(9f89a514888418a9bebbca341a8cc66e41b58acb) )

	ROM_REGION( 0x10000, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_LOAD( "sk1_8.10n",    0x0000, 0x4000, CRC(44bb7375) SHA1(5b2fa6782671150bab5f3c3ac46b47bc23f3d7e0) )	/* sprites */
	ROM_LOAD( "sk1_7.10m",    0x4000, 0x4000, CRC(3454671d) SHA1(723b26a0f208addc2a22736457cb4be6ab6c69cc) )
	/* 0x8000-0xbfff  will be unpacked from 0x4000-0x5fff */
	ROM_FILL(                 0xc000, 0x4000, 0x00 )	// part of the gfx is 2bpp decoded as 3bpp

	ROM_REGION( 0x0700, REGION_PROMS, 0 )
	ROM_LOAD( "sk1-1.2n",     0x0000, 0x0100, CRC(0218e726) SHA1(8b766162a4783c058d9a1ecf8741673d7ef955fb) )	/* red component */
	ROM_LOAD( "sk1-2.2p",     0x0100, 0x0100, CRC(fc0d5b85) SHA1(d1b13e42e735b24594cf0b840dee8110de23369e) )	/* green component */
	ROM_LOAD( "sk1-3.2r",     0x0200, 0x0100, CRC(d06b620b) SHA1(968a2d62c65e201d521e9efa8fcf6ad15898e4b3) )	/* blue component */
	ROM_LOAD( "sk1-4.5n",     0x0300, 0x0200, CRC(c697ac72) SHA1(3b79755e6cbb22c14fc4affdbd3f4521da1d90e8) )	/* tiles lookup table */
	ROM_LOAD( "sk1-5.6n",     0x0500, 0x0200, CRC(161514a4) SHA1(4488ce60d12be6586e4a1ddbbfd06bf4e7dfaceb) )	/* sprites lookup table */
ROM_END

ROM_START( skykidd )
	ROM_REGION( 0x14000, REGION_CPU1, 0 )	/* 6809 code */
	ROM_LOAD( "sk1_2x.6c",    0x08000, 0x4000, CRC(8370671a) SHA1(7038f952ebfc4482440b73ee4027fa908561d122) )
	ROM_LOAD( "sk1_1.6b",     0x0c000, 0x4000, CRC(070a49d4) SHA1(4b994bde3e34b574bd927843804d2fb1a08d1bdf) )
	ROM_LOAD( "sk1_3.6d",     0x10000, 0x4000, CRC(314b8765) SHA1(d90a8a853ce672fe5ee190f07bcb33262c73df3b) )	/* banked ROM */

	ROM_REGION( 0x10000, REGION_CPU2, 0 ) /* MCU code */
	ROM_LOAD( "sk1_4x.3c",    0x8000, 0x2000, CRC(887137cc) SHA1(dd0f66afb78833c4da73539b692854346f448c0d) )	/* subprogram for the MCU */
	ROM_LOAD( "cus60a1",      0xf000, 0x1000, BAD_DUMP CRC(6ef08fb3) SHA1(4842590d60035a0059b0899eb2d5f58ae72c2529) )	/* MCU internal code */

	ROM_REGION( 0x02000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "sk1_6.6l",     0x0000, 0x2000, CRC(58b731b9) SHA1(40f7be85914833ce02a734c20d68c0db8b77911d) )	/* chars */

	ROM_REGION( 0x02000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "sk1_5.7e",     0x0000, 0x2000, CRC(c33a498e) SHA1(9f89a514888418a9bebbca341a8cc66e41b58acb) )

	ROM_REGION( 0x10000, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_LOAD( "sk1_8.10n",    0x0000, 0x4000, CRC(44bb7375) SHA1(5b2fa6782671150bab5f3c3ac46b47bc23f3d7e0) )	/* sprites */
	ROM_LOAD( "sk1_7.10m",    0x4000, 0x4000, CRC(3454671d) SHA1(723b26a0f208addc2a22736457cb4be6ab6c69cc) )
	/* 0x8000-0xbfff  will be unpacked from 0x4000-0x5fff */
	ROM_FILL(                 0xc000, 0x4000, 0x00 )	// part of the gfx is 2bpp decoded as 3bpp

	ROM_REGION( 0x0700, REGION_PROMS, 0 )
	ROM_LOAD( "sk1-1.2n",     0x0000, 0x0100, CRC(0218e726) SHA1(8b766162a4783c058d9a1ecf8741673d7ef955fb) )	/* red component */
	ROM_LOAD( "sk1-2.2p",     0x0100, 0x0100, CRC(fc0d5b85) SHA1(d1b13e42e735b24594cf0b840dee8110de23369e) )	/* green component */
	ROM_LOAD( "sk1-3.2r",     0x0200, 0x0100, CRC(d06b620b) SHA1(968a2d62c65e201d521e9efa8fcf6ad15898e4b3) )	/* blue component */
	ROM_LOAD( "sk1-4.5n",     0x0300, 0x0200, CRC(c697ac72) SHA1(3b79755e6cbb22c14fc4affdbd3f4521da1d90e8) )	/* tiles lookup table */
	ROM_LOAD( "sk1-5.6n",     0x0500, 0x0200, CRC(161514a4) SHA1(4488ce60d12be6586e4a1ddbbfd06bf4e7dfaceb) )	/* sprites lookup table */
ROM_END

ROM_START( drgnbstr )
	ROM_REGION( 0x14000, REGION_CPU1, 0 ) /* 6809 code */
	ROM_LOAD( "db1_2b.6c",    0x08000, 0x04000, CRC(0f11cd17) SHA1(691d853f4f08898ecf4bccfb70a568de309329f1) )
	ROM_LOAD( "db1_1.6b",     0x0c000, 0x04000, CRC(1c7c1821) SHA1(8b6111afc42e2996bdc2fc276be0c40556cd431e) )
	ROM_LOAD( "db1_3.6d",     0x10000, 0x04000, CRC(6da169ae) SHA1(235211c26562fef0660e3fde1e87f2e52626d119) )	/* banked ROM */

	ROM_REGION( 0x10000, REGION_CPU2, 0 ) /* MCU code */
	ROM_LOAD( "db1_4.3c",     0x8000, 0x02000, CRC(8a0b1fc1) SHA1(c2861d0da63e2d17f2d1ad46dccf753ecd902ce3) )	/* subprogram for the MCU */
	ROM_LOAD( "cus60",        0xf000, 0x01000, CRC(6ef08fb3) SHA1(4842590d60035a0059b0899eb2d5f58ae72c2529) )	/* The MCU internal code is missing */
																/* Using Pacland code (probably similar) */
	ROM_REGION( 0x02000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "db1_6.6l",     0x0000, 0x2000, CRC(c080b66c) SHA1(05dcd45274d0bd12ef8ae7fd10c8719e679b3e7b) )	/* tiles */

	ROM_REGION( 0x02000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "db1_5.7e",     0x0000, 0x2000, CRC(28129aed) SHA1(d7f52e871d97179ec88c142a1c70eb6ad09e534a) )

	ROM_REGION( 0x10000, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_LOAD( "db1_8.10n",    0x0000, 0x4000, CRC(11942c61) SHA1(0f065cb82cf83967e90b3c7326b36956f4fa9a52) )	/* sprites */
	ROM_LOAD( "db1_7.10m",    0x4000, 0x4000, CRC(cc130fe2) SHA1(4f5d4f21152b3b4e523a6d17dd5ff5cef52447f2) )
	/* 0x8000-0xbfff  will be unpacked from 0x4000-0x5fff */
	ROM_FILL(                 0xc000, 0x4000, 0x00 )	// part of the gfx is 2bpp decoded as 3bpp

	ROM_REGION( 0x0700, REGION_PROMS, 0 )
	ROM_LOAD( "db1-1.2n",     0x0000, 0x0100, CRC(3f8cce97) SHA1(027b3fb0f322a9d68b434b207a40b31799a8a8d6) )	/* red component */
	ROM_LOAD( "db1-2.2p",     0x0100, 0x0100, CRC(afe32436) SHA1(e405787f7f2aa992edd63078e3944334d8acddb1) )	/* green component */
	ROM_LOAD( "db1-3.2r",     0x0200, 0x0100, CRC(c95ff576) SHA1(861a7340d29e6a6a0d5ead93abd3f73cc3df0cc7) )	/* blue component */
	ROM_LOAD( "db1-4.5n",     0x0300, 0x0200, CRC(b2180c21) SHA1(a5d14c31d54f04494ea99c3d94bd1b5e072b612e) )	/* tiles lookup table */
	ROM_LOAD( "db1-5.6n",     0x0500, 0x0200, CRC(5e2b3f74) SHA1(ef58661fa12a52bc358e81179254d37de7551b38) )	/* sprites lookup table */
ROM_END



static DRIVER_INIT( skykid )
{
	UINT8 *rom;
	int i;

	/* unpack the third sprite ROM */
	rom = memory_region(REGION_GFX3) + 0x4000;
	for (i = 0;i < 0x2000;i++)
	{
		rom[i + 0x4000] = rom[i];		// sprite set #1, plane 3
		rom[i + 0x6000] = rom[i] >> 4;	// sprite set #2, plane 3

		rom[i] = rom[i + 0x2000];		// sprite set #3, planes 1&2 (plane 3 is empty)
	}
}

static DRIVER_INIT( skykidd )
{
	init_skykid();

	/* patch the MCU internal ROM to make the game boot */
	memory_region(REGION_CPU2)[0xffed] -= 2;	// value expected by the external ROM
	memory_region(REGION_CPU2)[0xffec] += 2;	// to keep the same chksum
}



GAME( 1984, drgnbstr, 0,      skykid, drgnbstr, skykid,  ROT0,   "Namco", "Dragon Buster", 0 )
GAME( 1985, skykid,   0,      skykid, skykid,   skykid,  ROT180, "Namco", "Sky Kid (New Ver.)", 0 )
GAME( 1985, skykido,  skykid, skykid, skykid,   skykid,  ROT180, "Namco", "Sky Kid (Old Ver.)", 0 )
GAME( 1985, skykidd,  skykid, skykid, skykid,   skykidd, ROT180, "Namco", "Sky Kid (60A1 Ver.)", 0 )
