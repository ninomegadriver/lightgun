/**********************************************************************
Tatakae! Big Fighter (c)1989 Nichibutsu

 based on armedf.c

 TODO:
 - scroll
 - controls
 - dips

    $80600($80000 ?? ) - $80fff  = shared ram with 8751 MCU

    controls :

    02E3E8: move.w  $8c000.l, $8064a.l
    02E3F2: move.w  $8c002.l, $8064c.l
    02E3FC: move.b  #$1, $80640.l
    02E404: move.w  $400000.l, D0
    02E40A: tst.b   $80640.l
    02E410: bne     2e3fc
    02E412: move.w  $80642.l, $8064e.l
    02E41C: move.w  $80644.l, $80650.l
    02E426: move.w  $80646.l, $80652.l - input 1 (80646)
    02E430: move.w  $80648.l, $80654.l
    02E43A: rts


------------------------------------------------------------------------
Tatakae! Big Fighter
Nichibutsu, 1989

This is a horizontal shoot'em-up similar to R-Type.

It appears this PCB is re-used? Sticker says PCB number is 1706 and (C) 1989
On the PCB under the sticker is written 1605 and (C) 1988


PCB Layout
----------


1605A-1 (1706-1)
-------------------------------------------------------------------
|                     2018                                        |
|              6.15F  2018                                        |
|  PROM.13H    5.13F                                              |
|                                                                 |
|                                                                 |
|                            5814                                 |
|                            5814                                 |
|                                                                 |
|                     5814                                        |
|                     5814                                        |
|                                           7.11C                 |
|                                                                 |
|                     6264   6264                                 |
|                     6264   6264                                 |
|                   ----------------                              |
|                   |1706-3        |                              |
|                   |              |                  PAL         |
|                   | PAL  8751    |                              |
|                   |          PAL |                              |
|                   |              |                              |
|                   |              |                  PAL         |
|                   | 2.IC4  4.IC5 |                              |
|                   | 1.IC2  3.IC3 |                              |
| DSW2              |              |                              |
| DSW1              |    68000     |    16MHz         PAL         |
|                   ----------------                              |
-------------------------------------------------------------------


1605B (1706-2)
-------------------------------------------------------------------
|                                                                 |
| 8.17K  Z80A                2018                                 |
|                            2018                                 |
| YM3812  2018               2018                                 |
| Y3014B                                                          |
|                                                                 |
|                                                                 |
|                                                                 |
|                                                                 |
|                                                                 |
|                                                                 |
|                                                                 |
|                                  2018                           |
|                          10.9D   2018                           |
|                           9.8D                          12.8A   |
|                           PAL                           11.6A   |
|                                                                 |
|                                                                 |
|                                                                 |
|                                                                 |
| 2018  2018                                                      |
| 2018  2018                                                      |
|                                                                 |
|               24MHz                                             |
|                                                                 |
-------------------------------------------------------------------


Notes:
      Horizontal Sync: 15.08kHz
        Vertical Sync: 60Hz
            68K Clock: 7.998MHz
            Z80 Clock: ? (unstable, probably 6MHz or less)


**********************************************************************/
#include "driver.h"
#include "sound/dac.h"
#include "sound/okim6295.h"
#include "sound/3812intf.h"

static UINT16 vreg;

static UINT16 *text_videoram;
static UINT16 *bg_videoram;
static UINT16 *fg_videoram;
static UINT16 *sharedram16;
static UINT16 fg_scrollx,fg_scrolly;


static tilemap *bg_tilemap, *fg_tilemap, *tx_tilemap;

static void get_fg_tile_info( int tile_index )
{
	int data = fg_videoram[tile_index];
	SET_TILE_INFO(
			1,
			(data&0x7ff),
			data>>11,
			0)
}


static void get_bg_tile_info( int tile_index )
{
	int data = bg_videoram[tile_index];
	SET_TILE_INFO(
			2,
			(data&0x3ff),
			data>>11,
			0)
}

static void get_tx_tile_info(int tile_index)
{
	int tile_number = text_videoram[tile_index]&0xff;
	int attributes;

	attributes = text_videoram[tile_index+0x800]&0xff;

	SET_TILE_INFO(
			0,
			tile_number + 256 * (attributes & 0x3),
			attributes >> 4,
			0)
}

VIDEO_START( bigfghtr )
{
	bg_tilemap = tilemap_create(get_bg_tile_info,tilemap_scan_cols,TILEMAP_OPAQUE,16,16,64,32);
	fg_tilemap = tilemap_create(get_fg_tile_info,tilemap_scan_cols,TILEMAP_TRANSPARENT,16,16,64,32);
	tx_tilemap = tilemap_create(get_tx_tile_info,tilemap_scan_cols,TILEMAP_TRANSPARENT,8,8,64,32);

	tilemap_set_transparent_pen(fg_tilemap,0xf);
	tilemap_set_transparent_pen(tx_tilemap,0xf);

	return 0;
}

WRITE16_HANDLER(text_videoram_w )
{
	COMBINE_DATA(&text_videoram[offset]);
	tilemap_mark_tile_dirty(tx_tilemap,offset & 0x7ff);
}

WRITE16_HANDLER( fg_videoram_w )
{
	COMBINE_DATA(&fg_videoram[offset]);
	tilemap_mark_tile_dirty(fg_tilemap,offset);
}

WRITE16_HANDLER( bg_videoram_w )
{
	COMBINE_DATA(&bg_videoram[offset]);
	tilemap_mark_tile_dirty(bg_tilemap,offset);
}

WRITE16_HANDLER( fg_scrollx_w )
{
	COMBINE_DATA(&fg_scrollx);
}

WRITE16_HANDLER( fg_scrolly_w )
{
	COMBINE_DATA(&fg_scrolly);
}

WRITE16_HANDLER( bg_scrollx_w )
{
	static UINT16 scroll;
	COMBINE_DATA(&scroll);
	tilemap_set_scrollx(bg_tilemap,0,scroll);
}

WRITE16_HANDLER( bg_scrolly_w )
{
	static UINT16 scroll;
	COMBINE_DATA(&scroll);
	tilemap_set_scrolly(bg_tilemap,0,scroll);
}



static void draw_sprites( mame_bitmap *bitmap, const rectangle *cliprect, int priority )
{
	int offs;
	for (offs = 0;offs < spriteram_size/2;offs += 4)
	{
		int code = buffered_spriteram16[offs+1]; /* ??YX?TTTTTTTTTTT */
		int flipx = code & 0x2000;
		int flipy = code & 0x1000;
		int color = (buffered_spriteram16[offs+2]>>8)&0x1f;
		int sx = buffered_spriteram16[offs+3];
		int sy = 240-(buffered_spriteram16[offs+0]&0x1ff);

		if (((buffered_spriteram16[offs+0] & 0x3000) >> 12) == priority)
		{
			drawgfx(bitmap,Machine->gfx[3],
				code & 0xfff,
				color,
 				flipx,flipy,
				sx,sy,
				cliprect,TRANSPARENCY_PEN,15);
		}
	}
}


VIDEO_UPDATE( bigfghtr )
{
	int sprite_enable = vreg & 0x200;

	tilemap_set_enable( bg_tilemap, vreg&0x800 );
	tilemap_set_enable( fg_tilemap, vreg&0x400 );
	tilemap_set_enable( tx_tilemap, vreg&0x100 );

	tilemap_set_scrollx( fg_tilemap, 0, fg_scrollx );
	tilemap_set_scrolly( fg_tilemap, 0, fg_scrolly );

	if( vreg & 0x0800 )
	{
		tilemap_draw( bitmap, cliprect, bg_tilemap, 0, 0);
	}
	else
	{
		fillbitmap( bitmap, get_black_pen(), cliprect );
	}

	if( sprite_enable ) draw_sprites( bitmap, cliprect, 2 );
	tilemap_draw( bitmap, cliprect, fg_tilemap, 0, 0);
	if( sprite_enable ) draw_sprites( bitmap, cliprect, 1 );
	tilemap_draw( bitmap, cliprect, tx_tilemap, 0, 0);
	if( sprite_enable ) draw_sprites( bitmap, cliprect, 0 );

}



VIDEO_EOF( bigfghtr )
{
	buffer_spriteram16_w(0,0,0);
}


static WRITE16_HANDLER( sound_command_w )
{
	if (ACCESSING_LSB)
		soundlatch_w(0,((data & 0x7f) << 1) | 1);
}

static WRITE16_HANDLER( io_w )
{
	COMBINE_DATA(&vreg);
	flip_screen_set(vreg & 0x1000);
}

static int read_latch=0;

static READ16_HANDLER(latch_r)
{
	read_latch=1;
	return 0;
}


static WRITE16_HANDLER(sharedram_w)
{

	COMBINE_DATA(&sharedram16[offset]);

}

static READ16_HANDLER(sharedram_r)
{
		switch(offset)
		{
			case 0x40/2:
				if(read_latch)
				{
					read_latch=0;
					return rand();
				}
			break;

			case 0x46/2:
				return (input_port_0_word_r(0,0)&0xffff)^0xffff;


		}
		return sharedram16[offset];
}

static ADDRESS_MAP_START( mainmem, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x07ffff) AM_ROM
	AM_RANGE(0x080000, 0x0805ff) AM_RAM AM_BASE(&spriteram16) AM_SIZE(&spriteram_size)
	AM_RANGE(0x080600, 0x080fff) AM_READ(sharedram_r) AM_WRITE(sharedram_w) AM_BASE(&sharedram16)
	AM_RANGE(0x081000, 0x085fff) AM_RAM //??
  AM_RANGE(0x086000, 0x086fff) AM_READ(MRA16_RAM) AM_WRITE(bg_videoram_w) AM_BASE(&bg_videoram)
	AM_RANGE(0x087000, 0x087fff) AM_READ(MRA16_RAM) AM_WRITE(fg_videoram_w) AM_BASE(&fg_videoram)
	AM_RANGE(0x088000, 0x089fff) AM_READ(MRA16_RAM) AM_WRITE(text_videoram_w) AM_BASE(&text_videoram)
	AM_RANGE(0x08a000, 0x08afff) AM_READ(MRA16_RAM) AM_WRITE(paletteram16_xxxxRRRRGGGGBBBB_word_w) AM_BASE(&paletteram16)
	AM_RANGE(0x08b000, 0x08bfff) AM_RAM //??
	AM_RANGE(0x08c000, 0x08c001) AM_READ(input_port_0_word_r)
	AM_RANGE(0x08c002, 0x08c003) AM_READ(input_port_1_word_r)
	AM_RANGE(0x08c004, 0x08c005) AM_READ(input_port_2_word_r)
	AM_RANGE(0x08c006, 0x08c007) AM_READ(input_port_3_word_r)

	AM_RANGE(0x08d000, 0x08d001) AM_WRITE(io_w) 				//807b0

	AM_RANGE(0x08d002, 0x08d003) AM_WRITE(bg_scrollx_w) //8069a
	AM_RANGE(0x08d004, 0x08d005) AM_WRITE(bg_scrolly_w) //80696
	AM_RANGE(0x08d006, 0x08d007) AM_WRITE(fg_scrollx_w) //80692
	AM_RANGE(0x08d008, 0x08d009) AM_WRITE(fg_scrolly_w) //8068e

	AM_RANGE(0x08d00a, 0x08d00b) AM_WRITE(sound_command_w)
	AM_RANGE(0x400000, 0x400001) AM_READ(latch_r)
ADDRESS_MAP_END

static ADDRESS_MAP_START( soundmem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0xf7ff) AM_ROM
	AM_RANGE(0xf800, 0xffff) AM_RAM
ADDRESS_MAP_END

static READ8_HANDLER( soundlatch_clear_r )
{
	soundlatch_clear_w(0,0);
	return 0;
}

static ADDRESS_MAP_START( soundport, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_FLAGS( AMEF_ABITS(8) )
	AM_RANGE(0x0, 0x0) AM_WRITE(YM3812_control_port_0_w)
	AM_RANGE(0x1, 0x1) AM_WRITE(YM3812_write_port_0_w)
	AM_RANGE(0x2, 0x2) AM_WRITE(DAC_0_signed_data_w)
	AM_RANGE(0x3, 0x3) AM_WRITE(DAC_1_signed_data_w)
	AM_RANGE(0x4, 0x4) AM_READ(soundlatch_clear_r)
	AM_RANGE(0x6, 0x6) AM_READ(soundlatch_r)
ADDRESS_MAP_END

static const gfx_layout char_layout =
{
	8,8,
	RGN_FRAC(1,1),
	4,
	{ 0, 1, 2, 3 },
	{ 4, 0, 12, 8, 20, 16, 28, 24},
	{ 0*32, 1*32, 2*32, 3*32, 4*32, 5*32, 6*32, 7*32 },
	32*8
};

static const gfx_layout tile_layout =
{
	16,16,
	RGN_FRAC(1,1),
	4,
	{ 0, 1, 2, 3 },
	{ 4, 0, 12, 8, 20, 16, 28, 24,
			32+4, 32+0, 32+12, 32+8, 32+20, 32+16, 32+28, 32+24 },
	{ 0*64, 1*64, 2*64, 3*64, 4*64, 5*64, 6*64, 7*64,
			8*64, 9*64, 10*64, 11*64, 12*64, 13*64, 14*64, 15*64 },
	128*8
};

static const gfx_layout sprite_layout =
{
	16,16,
	RGN_FRAC(1,2),
	4,
	{ 0, 1, 2, 3 },
	{ 4, 0, RGN_FRAC(1,2)+4, RGN_FRAC(1,2)+0, 12, 8, RGN_FRAC(1,2)+12, RGN_FRAC(1,2)+8,
			20, 16, RGN_FRAC(1,2)+20, RGN_FRAC(1,2)+16, 28, 24, RGN_FRAC(1,2)+28, RGN_FRAC(1,2)+24 },
	{ 0*32, 1*32, 2*32, 3*32, 4*32, 5*32, 6*32, 7*32,
			8*32, 9*32, 10*32, 11*32, 12*32, 13*32, 14*32, 15*32 },
	64*8
};

static const gfx_decode gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &char_layout,		 0*16,	32 },
	{ REGION_GFX2, 0, &tile_layout,		64*16,	32 },
	{ REGION_GFX3, 0, &tile_layout,		96*16,	32 },
	{ REGION_GFX4, 0, &sprite_layout,	32*16,	32 },
	{ -1 } /* end of array */
};



static MACHINE_DRIVER_START( bigfghtr )
	MDRV_CPU_ADD(M68000, 8000000) /* 8 MHz?? */
	MDRV_CPU_PROGRAM_MAP(mainmem,0)

	MDRV_CPU_VBLANK_INT(irq1_line_hold,1)

	MDRV_CPU_ADD(Z80, 3072000)
	/* audio CPU */	/* 3.072 MHz???? */
	MDRV_CPU_PROGRAM_MAP(soundmem,0)
	MDRV_CPU_IO_MAP(soundport,0)
	MDRV_CPU_VBLANK_INT(irq0_line_hold,128)

	MDRV_FRAMES_PER_SECOND(57)
	MDRV_VBLANK_DURATION(DEFAULT_REAL_60HZ_VBLANK_DURATION)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER | VIDEO_BUFFERS_SPRITERAM)
	MDRV_SCREEN_SIZE(64*8, 32*8)
	MDRV_VISIBLE_AREA(12*8, (64-12)*8-1, 1*8, 31*8-1 )
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(2048)

	MDRV_VIDEO_EOF(bigfghtr)
	MDRV_VIDEO_START(bigfghtr)
	MDRV_VIDEO_UPDATE(bigfghtr)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")

	MDRV_SOUND_ADD(YM3812, 4000000)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.50)

	MDRV_SOUND_ADD(DAC, 0)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.0)

	MDRV_SOUND_ADD(DAC, 0)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.0)
MACHINE_DRIVER_END

INPUT_PORTS_START( bigfghtr )
	PORT_START_TAG("IN0")
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(1)
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(1)
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_PLAYER(1)
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0xf000, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START_TAG("IN1")
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(2)
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(2)
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_PLAYER(2)
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_SERVICE( 0x0200, IP_ACTIVE_LOW )
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_TILT )
	PORT_BIT( 0xf800, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START_TAG("DSW0")
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x03, "3" )
	PORT_DIPSETTING(    0x02, "4" )
	PORT_DIPSETTING(    0x01, "5" )
	PORT_DIPSETTING(    0x00, "6" )


	PORT_DIPNAME( 0x0c, 0x0c, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x0c, "20k then every 60k" )
	PORT_DIPSETTING(    0x04, "20k then every 80k" )
	PORT_DIPSETTING(    0x08, "40k then every 60k" )
	PORT_DIPSETTING(    0x00, "40k then every 80k" )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x10, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0xc0, 0xc0, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0xc0, DEF_STR( Easy ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Normal ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Hard ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Hardest ) )

	PORT_START_TAG("DSW1")
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Free_Play ) )
	PORT_DIPNAME( 0x0c, 0x0c, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x0c, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 1C_2C ) )
	PORT_DIPNAME( 0x30, 0x00, DEF_STR( Allow_Continue ) )
	PORT_DIPSETTING(    0x30, DEF_STR( No ) )
	PORT_DIPSETTING(    0x20, "3 Times" )
	PORT_DIPSETTING(    0x10, "5 Times" )
	PORT_DIPSETTING(    0x00, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
INPUT_PORTS_END


DRIVER_INIT( bigfghtr )
{
	//RAM TESTS
	UINT16 *RAM = (UINT16 *)memory_region(REGION_CPU1);
	RAM[0x2e8cc/2] = 0x4ef9;
	RAM[0x2e8ce/2] = 0x0002;
	RAM[0x2e8d0/2] = 0xea58;

}

ROM_START( bigfghtr )
	ROM_REGION( 0x80000, REGION_CPU1, 0 )

	ROM_LOAD16_BYTE( "3.ic3", 0x00000, 0x20000, CRC(e1e1f291) SHA1(dbbd707be6250d9ffcba3fee265869b72f790e26) )
	ROM_LOAD16_BYTE( "1.ic2", 0x00001, 0x20000, CRC(1100d991) SHA1(3c79398804b3a26b3df0c5734b270c37e1ba6a60) )
	ROM_LOAD16_BYTE( "4.ic5", 0x40000, 0x20000, CRC(2464a83b) SHA1(00f5ac81bc33148daafeab757647b63894e0e0ca) )
	ROM_LOAD16_BYTE( "2.ic4", 0x40001, 0x20000, CRC(b47bbcd5) SHA1(811bd4bc8fb662abf4734ab51e24c863d5cc3df3) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* Z80 code (sound) */
	ROM_LOAD( "8.17k", 0x00000, 0x10000, CRC(0aeab61e) SHA1(165e0ad58542b65383fef714578da21f62df7b74) )

	ROM_REGION( 0x08000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "7.11c", 0x00000, 0x08000, CRC(1809e79f) SHA1(730547771f803857acb552a84a8bc21bd3bda33f) )

	ROM_REGION( 0x30000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "5.13f", 0x00000, 0x20000, CRC(d440a29f) SHA1(9e6ea7c9903e5e3e8e10ac7680c6120e1aa27250) )
	ROM_LOAD( "6.15f", 0x20000, 0x10000, CRC(27469a76) SHA1(ebf2c60e1f70a589680c05adf10771ac2097b9d0) )

	ROM_REGION( 0x20000, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_LOAD( "12.8a", 0x00000, 0x10000, CRC(a5694ea9) SHA1(ea94174495b3a65b3797932074a94df3b55fa0a2) )
	ROM_LOAD( "11.6a", 0x10000, 0x10000, CRC(10b74e2c) SHA1(e3ec68726e7f277dc2043424f2e4d863eb01b3dc) )


	ROM_REGION( 0x40000, REGION_GFX4, ROMREGION_DISPOSE )
	ROM_LOAD( "9.8d",  0x00000, 0x20000, CRC(fe67800e) SHA1(0d3c4c3cb185270260fa691a97cddf082d6a056e) )
	ROM_LOAD( "10.9d", 0x20000, 0x20000, CRC(dcb828c4) SHA1(607bc86580a6fe6e15e91131532b0eecd8b7a0cb) )


ROM_END

GAME( 1989, bigfghtr,   0,     bigfghtr,   bigfghtr,   bigfghtr,   ROT0, "Nichibutsu", "Tatakae! Big Fighter",GAME_NOT_WORKING | GAME_UNEMULATED_PROTECTION)

