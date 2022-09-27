/***************************************************************************

Baraduke    (c) 1985 Namco
Metro-Cross (c) 1985 Namco

Driver by Manuel Abadia <manu@teleline.es>

The sprite and tilemap generator ICs are the same as in Namco System 86, but
System 86 has two tilemap generators instead of one.

Custom ICs:
----------
98XX     lamp/coin output
99XX     sound volume
CUS27    clock divider
CUS30    sound control
CUS31
CUS39    sprite generator
CUS41    address decoder
CUS42    dual scrolling tilemap address generator
CUS43    dual tilemap generator
CUS48    sprite address generator
CUS60    MCU (63701) [1]

[1] 60A1 found in at least one version


Baraduke
Namco, 1985

PCB Layout
----------

 2248961100
(2248963100)
|---------------------------------------------------|
|           BD1_8.4P                6116            |
|       43                                          |
|PR1.1N     BD1_7.4N       42       BD1_12.8N  2148 |
|                                                   |
|  PR2.2M   BD1_6.4M                BD1_11.8M  2148 |
|                                                   |
|                                   BD1_10.8L  2148 |
|2  DSWA DSWB                                       |
|2  31                    27        BD1_9.8K   39   |
|W      BD1_5.3J   49.152MHz                        |
|A                        6116                      |
|Y                                                  |
|       2148                                   48   |
|       2148     30                6116             |
|                                  6116             |
|                                  6116             |
|       6116                       6116   BD1_3.9C  |
|                                                   |
|       BD1_4B.3B                         BD1_2.9B  |
|                                                   |
|LA4460     60A1         41        6809   BD1_1.9A  |
|---------------------------------------------------|
Notes:
      Clocks:
      6809 clock : 1.536MHz (49.152 / 32)
      63701 clock: 1.536MHz (49.152 / 32)
      VSync      : 60.606060Hz

      RAMs:
      6116: 2K x8 SRAM
      2148: 1K x4 SRAM

      Namco Customs:
      27   : DIP40, manufactured by Fujitsu)
      30   : (SDIP64)
      31   : (DIP48, also written on chip is '218' and '5201')
      39   : (DIP42, manufactured by Fujitsu)
      41   : (DIP40, also written on chip is '8512MAD' and 'PHILIPPINES')
      42   : (QFP80, the only chip on the PCB that is surface-mounted)
      60A1 : (DIP40, known 63701 MCU)
      43   : (SDIP64)
      48   : (SDIP64)

      ROMs:
      PR1.1N is a PROM type MB7138E
      PR2.2M is a PROM type MB7128E
      BD1_3.9C & BD1_5.3J are 2764 EPROMs. All other ROMs are 27128 EPROMs.



Notes:
-----
- in floor 6 of baraduke, there are gaps in the big boss when the player moves.
  This is the correct behaviour, verified on the real board.

TODO:
----
- The two unknown writes for the MCU are probably watchdog reset and irq acknowledge,
  but they don't seem to work as expected. During the first few frames they are
  written out of order and hooking them up in the usual way causes the MCU to
  stop receiving interrupts.

- remove the sound kludge in Baraduke. This might actually be a feature of the
  CUS30 chip.

***************************************************************************/

#include "driver.h"
#include "cpu/m6800/m6800.h"
#include "sound/namco.h"

extern UINT8 *baraduke_textram, *baraduke_videoram, *baraduke_spriteram;

/* from vidhrdw/baraduke.c */
VIDEO_START( baraduke );
VIDEO_UPDATE( baraduke );
VIDEO_EOF( baraduke );
READ8_HANDLER( baraduke_videoram_r );
WRITE8_HANDLER( baraduke_videoram_w );
READ8_HANDLER( baraduke_textram_r );
WRITE8_HANDLER( baraduke_textram_w );
WRITE8_HANDLER( baraduke_scroll0_w );
WRITE8_HANDLER( baraduke_scroll1_w );
READ8_HANDLER( baraduke_spriteram_r );
WRITE8_HANDLER( baraduke_spriteram_w );
PALETTE_INIT( baraduke );

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
		case 0x00:	/* DSW A (bits 0-4) */
			return (readinputport(0) & 0xf8) >> 3; break;
		case 0x01:	/* DSW A (bits 5-7), DSW B (bits 0-1) */
			return ((readinputport(0) & 0x07) << 2) | ((readinputport(1) & 0xc0) >> 6); break;
		case 0x02:	/* DSW B (bits 2-6) */
			return (readinputport(1) & 0x3e) >> 1; break;
		case 0x03:	/* DSW B (bit 7), DSW C (bits 0-3) */
			return ((readinputport(1) & 0x01) << 4) | (readinputport(2) & 0x0f); break;
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

static WRITE8_HANDLER( baraduke_lamps_w )
{
	set_led_status(0,data & 0x08);
	set_led_status(1,data & 0x10);
}

static WRITE8_HANDLER( baraduke_irq_ack_w )
{
	cpunum_set_input_line(0, 0, CLEAR_LINE);
}



static ADDRESS_MAP_START( baraduke_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x1fff) AM_READWRITE(baraduke_spriteram_r,baraduke_spriteram_w) AM_BASE(&baraduke_spriteram)	/* Sprite RAM */
	AM_RANGE(0x2000, 0x3fff) AM_READWRITE(baraduke_videoram_r,baraduke_videoram_w) AM_BASE(&baraduke_videoram)	/* Video RAM */
	AM_RANGE(0x4000, 0x43ff) AM_READWRITE(namcos1_cus30_r,namcos1_cus30_w)		/* PSG device, shared RAM */
	AM_RANGE(0x4800, 0x4fff) AM_READWRITE(baraduke_textram_r,baraduke_textram_w) AM_BASE(&baraduke_textram)/* video RAM (text layer) */
	AM_RANGE(0x8000, 0x8000) AM_WRITE(watchdog_reset_w)			/* watchdog reset */
	AM_RANGE(0x8800, 0x8800) AM_WRITE(baraduke_irq_ack_w)		/* irq acknowledge */
	AM_RANGE(0xb000, 0xb002) AM_WRITE(baraduke_scroll0_w)		/* scroll (layer 0) */
	AM_RANGE(0xb004, 0xb006) AM_WRITE(baraduke_scroll1_w)		/* scroll (layer 1) */
	AM_RANGE(0x6000, 0xffff) AM_READ(MRA8_ROM)				/* ROM */
ADDRESS_MAP_END

static READ8_HANDLER( soundkludge_r )
{
	static int counter;

	return ((counter++) >> 4) & 0xff;
}

static ADDRESS_MAP_START( mcu_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x001f) AM_READWRITE(hd63701_internal_registers_r,hd63701_internal_registers_w)/* internal registers */
	AM_RANGE(0x0080, 0x00ff) AM_RAM								/* built in RAM */
	AM_RANGE(0x1105, 0x1105) AM_READ(soundkludge_r)				/* cures speech */
	AM_RANGE(0x1000, 0x13ff) AM_READWRITE(namcos1_cus30_r,namcos1_cus30_w) AM_BASE(&namco_wavedata)/* PSG device, shared RAM */
	AM_RANGE(0x8000, 0xbfff) AM_READ(MRA8_ROM)					/* MCU external ROM */
	AM_RANGE(0x8000, 0x8000) AM_WRITE(MWA8_NOP)					/* watchdog reset? */
	AM_RANGE(0x8800, 0x8800) AM_WRITE(MWA8_NOP)					/* irq acknoledge? */
	AM_RANGE(0xc000, 0xc7ff) AM_RAM								/* RAM */
	AM_RANGE(0xf000, 0xffff) AM_READ(MRA8_ROM)					/* MCU internal ROM */
ADDRESS_MAP_END


static READ8_HANDLER( readFF )
{
	return 0xff;
}

static ADDRESS_MAP_START( mcu_port_map, ADDRESS_SPACE_IO, 8 )
	AM_RANGE(HD63701_PORT1, HD63701_PORT1) AM_READ(inputport_r)			/* input ports read */
	AM_RANGE(HD63701_PORT1, HD63701_PORT1) AM_WRITE(inputport_select_w)	/* input port select */
	AM_RANGE(HD63701_PORT2, HD63701_PORT2) AM_READ(readFF)	/* leds won't work otherwise */
	AM_RANGE(HD63701_PORT2, HD63701_PORT2) AM_WRITE(baraduke_lamps_w)		/* lamps */
ADDRESS_MAP_END



INPUT_PORTS_START( baraduke )
	PORT_START	/* DSW A */
	PORT_SERVICE( 0x80, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0x60, 0x60, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x40, "2" )
	PORT_DIPSETTING(    0x60, "3" )
	PORT_DIPSETTING(    0x20, "4" )
	PORT_DIPSETTING(    0x00, "5" )
	PORT_DIPNAME( 0x18, 0x18, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x18, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 1C_2C ) )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x04, DEF_STR( On ) )
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 1C_2C ) )

	PORT_START	/* DSW B */
	PORT_DIPNAME( 0xc0, 0xc0, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x80, "Every 10k" )
	PORT_DIPSETTING(    0xc0, "10k and every 20k" )
	PORT_DIPSETTING(    0x40, "Every 20k" )
	PORT_DIPSETTING(    0x00, DEF_STR( None ) )
	PORT_DIPNAME( 0x30, 0x30, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Easy ) )
	PORT_DIPSETTING(    0x30, DEF_STR( Normal ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Hard ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Very_Hard ) )
	PORT_DIPNAME( 0x08, 0x08, "Round Select" )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, "Freeze" )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, "Allow continue from last level" )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_SERVICE ) /* service switch from the edge connector */
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Cocktail ) )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0xf0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* IN 0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0xe0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* IN 1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY
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

INPUT_PORTS_START( metrocrs )
	PORT_START	/* DSW A */
	PORT_SERVICE( 0x80, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0x60, 0x60, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x20, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x60, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x40, DEF_STR( 1C_2C ) )
	PORT_DIPNAME( 0x18, 0x18, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Easy ) )
	PORT_DIPSETTING(    0x18, DEF_STR( Normal ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Hard ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Very_Hard ) )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Allow_Continue ) )
	PORT_DIPSETTING(    0x00, DEF_STR( No ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 1C_2C ) )

	PORT_START	/* DSW B */
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x80, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, "Round Select" )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, "Freeze" )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_SERVICE ) /* service switch from the edge connector */
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Cocktail ) )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0xf0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* IN 0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0xe0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* IN 1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY
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
	1024,
	3,
	{ 0x8000*8, 0, 4 },
	{ 0, 1, 2, 3, 8+0, 8+1, 8+2, 8+3 },
	{ 0*8, 2*8, 4*8, 6*8, 8*8, 10*8, 12*8, 14*8 },
	16*8
};

static const gfx_layout spritelayout =
{
	16,16,
	RGN_FRAC(1,1),
	4,
	{ 0, 1, 2, 3 },
	{ 0*4, 1*4, 2*4, 3*4, 4*4, 5*4, 6*4, 7*4,
		8*4, 9*4, 10*4, 11*4, 12*4, 13*4, 14*4, 15*4 },
    { 8*8*0, 8*8*1, 8*8*2, 8*8*3, 8*8*4, 8*8*5, 8*8*6, 8*8*7,
	8*8*8, 8*8*9, 8*8*10, 8*8*11, 8*8*12, 8*8*13, 8*8*14, 8*8*15 },
	128*8
};

static const gfx_decode gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0,      &text_layout,  0, 512 },
	{ REGION_GFX2, 0x0000, &tile_layout,  0, 256 },
	{ REGION_GFX2, 0x4000, &tile_layout,  0, 256 },
	{ REGION_GFX3, 0,      &spritelayout, 0, 128 },
	{ -1 }
};



static struct namco_interface namco_interface =
{
	8,					/* number of voices */
	-1,					/* memory region */
	0					/* stereo */
};



static MACHINE_DRIVER_START( baraduke )

	/* basic machine hardware */
	MDRV_CPU_ADD(M6809,49152000/32)
	MDRV_CPU_PROGRAM_MAP(baraduke_map,0)
	MDRV_CPU_VBLANK_INT(irq0_line_assert,1)

	MDRV_CPU_ADD(HD63701,49152000/32)
	MDRV_CPU_PROGRAM_MAP(mcu_map,0)
	MDRV_CPU_IO_MAP(mcu_port_map,0)
	MDRV_CPU_VBLANK_INT(irq0_line_hold,1)

	MDRV_FRAMES_PER_SECOND(60.606060)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)
	MDRV_INTERLEAVE(100)		/* we need heavy synch */

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(64*8, 32*8)
	MDRV_VISIBLE_AREA(0*8, 36*8-1, 2*8, 30*8-1)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(2048)

	MDRV_PALETTE_INIT(baraduke)
	MDRV_VIDEO_START(baraduke)
	MDRV_VIDEO_UPDATE(baraduke)
	MDRV_VIDEO_EOF(baraduke)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")

	MDRV_SOUND_ADD(NAMCO_CUS30, 49152000/2048)
	MDRV_SOUND_CONFIG(namco_interface)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.0)
MACHINE_DRIVER_END



ROM_START( baraduke )
	ROM_REGION( 0x10000, REGION_CPU1, 0 ) /* 6809 code */
	ROM_LOAD( "prg1.9c",	0x6000, 0x02000, CRC(ea2ea790) SHA1(ab6f523803b2b0ea04b78f2f252de6c2d344a26c) )
	ROM_LOAD( "prg2.9a",	0x8000, 0x04000, CRC(9a0a9a87) SHA1(6d88fb5b443c822ede4884d4452e333834b16aca) )
	ROM_LOAD( "prg3.9b",	0xc000, 0x04000, CRC(383e5458) SHA1(091f25e287f0a81649c9a4fa196ebe4112a82295) )

	ROM_REGION(  0x10000 , REGION_CPU2, 0 ) /* MCU code */
	ROM_LOAD( "prg4.3b",	0x8000, 0x4000, CRC(abda0fe7) SHA1(f7edcb5f9fa47bb38a8207af5678cf4ccc243547) )	/* subprogram for the MCU */
	ROM_LOAD( "cus60",      0xf000, 0x1000, CRC(6ef08fb3) SHA1(4842590d60035a0059b0899eb2d5f58ae72c2529) )	/* The MCU internal code is missing */
															/* Using Pacland code (probably similar) */
	ROM_REGION( 0x02000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "ch1.3j",		0x00000, 0x2000, CRC(706b7fee) SHA1(e5694289bd4346c1a3a004feaa940710cea755c6) )	/* characters */

	ROM_REGION( 0x10000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "ch2.4p",		0x00000, 0x4000, CRC(b0bb0710) SHA1(797832aea59bf80342fd2a3505645f2766bde65b) )	/* tiles */
	ROM_LOAD( "ch3.4n",		0x04000, 0x4000, CRC(0d7ebec9) SHA1(6b86b476db61f5760bc8610b51adc1115cfdad96) )
	ROM_LOAD( "ch4.4m",		0x08000, 0x4000, CRC(e5da0896) SHA1(abb8bf7e9dc1c60bc0a20a691109fb145bb1d8e0) )
	/* 0xc000-0xffff  will be unpacked from 0x8000-0xbfff */

	ROM_REGION( 0x10000, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_LOAD( "obj1.8k",	0x00000, 0x4000, CRC(87a29acc) SHA1(3aa00efc95d1da50f6e4637d101640328287dea1) )	/* sprites */
	ROM_LOAD( "obj2.8l",	0x04000, 0x4000, CRC(72b6d20c) SHA1(e40b48dacefce4fd62ab28d3e6ff3932d4ff005b) )
	ROM_LOAD( "obj3.8m",	0x08000, 0x4000, CRC(3076af9c) SHA1(57ce09b298fd0bae94e4d8c817a34ce812c3ddfc) )
	ROM_LOAD( "obj4.8n",	0x0c000, 0x4000, CRC(8b4c09a3) SHA1(46e0ef39cb313c6780f6137769153dc4a054c77f) )

	ROM_REGION( 0x1000, REGION_PROMS, 0 )
	ROM_LOAD( "prmcolbg.1n",0x0000, 0x0800, CRC(0d78ebc6) SHA1(0a0c1e23eb4d1748c4e6c448284d785286c77911) )	/* Blue + Green palette */
	ROM_LOAD( "prmcolr.2m",	0x0800, 0x0800, CRC(03f7241f) SHA1(16ae059f084ba0ac4ddaa95dbeed113295f106ea) )	/* Red palette */
ROM_END

ROM_START( baraduka )
	ROM_REGION( 0x10000, REGION_CPU1, 0 ) /* 6809 code */
	ROM_LOAD( "prg1.9c",	0x6000, 0x02000, CRC(ea2ea790) SHA1(ab6f523803b2b0ea04b78f2f252de6c2d344a26c) )
	ROM_LOAD( "bd1_1.9a",	0x8000, 0x04000, CRC(4e9f2bdc) SHA1(bc6e71d4d3b2064e662a105c1a77d2731070d58e) )
	ROM_LOAD( "bd1_2.9b",	0xc000, 0x04000, CRC(40617fcd) SHA1(51d17f3a2fc96e13c8ef5952efece526e0fb33f4) )

	ROM_REGION(  0x10000 , REGION_CPU2, 0 ) /* MCU code */
	ROM_LOAD( "bd1_4b.3b",	0x8000, 0x4000, CRC(a47ecd32) SHA1(a2a75e65deb28224a5729ed134ee4d5ea8c50706) )	/* subprogram for the MCU */
	ROM_LOAD( "cus60",      0xf000, 0x1000, CRC(6ef08fb3) SHA1(4842590d60035a0059b0899eb2d5f58ae72c2529) )	/* The MCU internal code is missing */
															/* Using Pacland code (probably similar) */
	ROM_REGION( 0x02000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "ch1.3j",		0x00000, 0x2000, CRC(706b7fee) SHA1(e5694289bd4346c1a3a004feaa940710cea755c6) )	/* characters */

	ROM_REGION( 0x10000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "ch2.4p",		0x00000, 0x4000, CRC(b0bb0710) SHA1(797832aea59bf80342fd2a3505645f2766bde65b) )	/* tiles */
	ROM_LOAD( "ch3.4n",		0x04000, 0x4000, CRC(0d7ebec9) SHA1(6b86b476db61f5760bc8610b51adc1115cfdad96) )
	ROM_LOAD( "ch4.4m",		0x08000, 0x4000, CRC(e5da0896) SHA1(abb8bf7e9dc1c60bc0a20a691109fb145bb1d8e0) )
	/* 0xc000-0xffff  will be unpacked from 0x8000-0xbfff */

	ROM_REGION( 0x10000, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_LOAD( "obj1.8k",	0x00000, 0x4000, CRC(87a29acc) SHA1(3aa00efc95d1da50f6e4637d101640328287dea1) )	/* sprites */
	ROM_LOAD( "obj2.8l",	0x04000, 0x4000, CRC(72b6d20c) SHA1(e40b48dacefce4fd62ab28d3e6ff3932d4ff005b) )
	ROM_LOAD( "obj3.8m",	0x08000, 0x4000, CRC(3076af9c) SHA1(57ce09b298fd0bae94e4d8c817a34ce812c3ddfc) )
	ROM_LOAD( "obj4.8n",	0x0c000, 0x4000, CRC(8b4c09a3) SHA1(46e0ef39cb313c6780f6137769153dc4a054c77f) )

	ROM_REGION( 0x1000, REGION_PROMS, 0 )
	ROM_LOAD( "prmcolbg.1n",0x0000, 0x0800, CRC(0d78ebc6) SHA1(0a0c1e23eb4d1748c4e6c448284d785286c77911) )	/* Blue + Green palette */
	ROM_LOAD( "prmcolr.2m",	0x0800, 0x0800, CRC(03f7241f) SHA1(16ae059f084ba0ac4ddaa95dbeed113295f106ea) )	/* Red palette */
ROM_END

ROM_START( metrocrs )
	ROM_REGION( 0x10000, REGION_CPU1, 0 ) /* 6809 code */
	ROM_LOAD( "mc1-3.9c",	0x6000, 0x02000, CRC(3390b33c) SHA1(0733aece368acc913e2ff32e8817194cb4b630fb) )
	ROM_LOAD( "mc1-1.9a",	0x8000, 0x04000, CRC(10b0977e) SHA1(6266d173b55075da1f252092bf38185880bc4969) )
	ROM_LOAD( "mc1-2.9b",	0xc000, 0x04000, CRC(5c846f35) SHA1(3c98a0f1131f2e2477fc75a588123c57ff5350ad) )

	ROM_REGION(  0x10000 , REGION_CPU2, 0 ) /* MCU code */
	ROM_LOAD( "mc1-4.3b",	0x8000, 0x2000, CRC(9c88f898) SHA1(d6d0345002b70c5aca41c664f34181715cd87669) )	/* subprogram for the MCU */
	ROM_LOAD( "cus60",      0xf000, 0x1000, CRC(6ef08fb3) SHA1(4842590d60035a0059b0899eb2d5f58ae72c2529) )	/* The MCU internal code is missing */
															/* Using Pacland code (probably similar) */
	ROM_REGION( 0x02000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "mc1-5.3j",	0x00000, 0x2000, CRC(9b5ea33a) SHA1(a8108e71e3440b645ebdb5cdbd87712151299789) )	/* characters */

	ROM_REGION( 0x10000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "mc1-7.4p",	0x00000, 0x4000, CRC(c9dfa003) SHA1(86e8f9fc25de67691ce5385d93b723e7eb836b2b) )	/* tiles */
	ROM_LOAD( "mc1-6.4n",	0x04000, 0x4000, CRC(9686dc3c) SHA1(1caf712eedb1f70559169685e5421e11866e518c) )
	ROM_FILL(               0x08000, 0x4000, 0xff )
	/* 0xc000-0xffff  will be unpacked from 0x8000-0xbfff */

	ROM_REGION( 0x08000, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_LOAD( "mc1-8.8k",	0x00000, 0x4000, CRC(265b31fa) SHA1(d46e6db5d6f325954d2b6159157b11e10fe5838d) )	/* sprites */
	ROM_LOAD( "mc1-9.8l",	0x04000, 0x4000, CRC(541ec029) SHA1(a3096d8405b6bbc862b03773889f6cbd43739f5b) )

	ROM_REGION( 0x1000, REGION_PROMS, 0 )
	ROM_LOAD( "mc1-1.1n",	0x0000, 0x0800, CRC(32a78a8b) SHA1(545a59bc3c5868ac1749d2947210110205fb3da2) )	/* Blue + Green palette */
	ROM_LOAD( "mc1-2.2m",	0x0800, 0x0800, CRC(6f4dca7b) SHA1(781134c02853aded2cba63719c0e4c78b227da1c) )	/* Red palette */
ROM_END

ROM_START( metrocra )
	ROM_REGION( 0x10000, REGION_CPU1, 0 ) /* 6809 code */
	ROM_LOAD( "mc2-3.9b",	0x6000, 0x02000, CRC(ffe08075) SHA1(4e1341d5a9a58f171e1e6f9aa18092d5557a6947) )
	ROM_LOAD( "mc2-1.9a",	0x8000, 0x04000, CRC(05a239ea) SHA1(3e7c7d305d0f48e2431d60b176a0cb451ddc4637) )
	ROM_LOAD( "mc2-2.9a",	0xc000, 0x04000, CRC(db9b0e6d) SHA1(2772b59fe7dc0e78ee29dd001a6bba75b94e0334) )

	ROM_REGION(  0x10000 , REGION_CPU2, 0 ) /* MCU code */
	ROM_LOAD( "mc1-4.3b",	0x8000, 0x2000, CRC(9c88f898) SHA1(d6d0345002b70c5aca41c664f34181715cd87669) )	/* subprogram for the MCU */
	ROM_LOAD( "cus60",      0xf000, 0x1000, CRC(6ef08fb3) SHA1(4842590d60035a0059b0899eb2d5f58ae72c2529) )	/* The MCU internal code is missing */
															/* Using Pacland code (probably similar) */
	ROM_REGION( 0x02000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "mc1-5.3j",	0x00000, 0x2000, CRC(9b5ea33a) SHA1(a8108e71e3440b645ebdb5cdbd87712151299789) )	/* characters */

	ROM_REGION( 0x10000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "mc1-7.4p",	0x00000, 0x4000, CRC(c9dfa003) SHA1(86e8f9fc25de67691ce5385d93b723e7eb836b2b) )	/* tiles */
	ROM_LOAD( "mc1-6.4n",	0x04000, 0x4000, CRC(9686dc3c) SHA1(1caf712eedb1f70559169685e5421e11866e518c) )
	ROM_FILL(               0x08000, 0x4000, 0xff )
	/* 0xc000-0xffff  will be unpacked from 0x8000-0xbfff */

	ROM_REGION( 0x08000, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_LOAD( "mc1-8.8k",	0x00000, 0x4000, CRC(265b31fa) SHA1(d46e6db5d6f325954d2b6159157b11e10fe5838d) )	/* sprites */
	ROM_LOAD( "mc1-9.8l",	0x04000, 0x4000, CRC(541ec029) SHA1(a3096d8405b6bbc862b03773889f6cbd43739f5b) )

	ROM_REGION( 0x1000, REGION_PROMS, 0 )
	ROM_LOAD( "mc1-1.1n",	0x0000, 0x0800, CRC(32a78a8b) SHA1(545a59bc3c5868ac1749d2947210110205fb3da2) )	/* Blue + Green palette */
	ROM_LOAD( "mc1-2.2m",	0x0800, 0x0800, CRC(6f4dca7b) SHA1(781134c02853aded2cba63719c0e4c78b227da1c) )	/* Red palette */
ROM_END



static DRIVER_INIT( baraduke )
{
	UINT8 *rom;
	int i;

	/* unpack the third tile ROM */
	rom = memory_region(REGION_GFX2) + 0x8000;
	for (i = 0x2000;i < 0x4000;i++)
	{
		rom[i + 0x2000] = rom[i];
		rom[i + 0x4000] = rom[i] << 4;
	}
	for (i = 0;i < 0x2000;i++)
	{
		rom[i + 0x2000] = rom[i] << 4;
	}
}



GAME( 1985, metrocrs, 0,        baraduke, metrocrs, baraduke, ROT0, "Namco", "Metro-Cross (set 1)", 0 )
GAME( 1985, metrocra, metrocrs, baraduke, metrocrs, baraduke, ROT0, "Namco", "Metro-Cross (set 2)", 0 )
GAME( 1985, baraduke, 0,        baraduke, baraduke, baraduke, ROT0, "Namco", "Baraduke (set 1)", 0 )
GAME( 1985, baraduka, baraduke, baraduke, baraduke, baraduke, ROT0, "Namco", "Baraduke (set 2)", 0 )
