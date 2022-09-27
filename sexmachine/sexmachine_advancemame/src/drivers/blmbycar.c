/***************************************************************************

                              -= Blomby Car =-

                    driver by   Luca Elia (l.elia@tin.it)


Main  CPU    :  68000
Video Chips  :  TI TPC1020AFN-084 (= Actel A1020A PL84C 9548)
Sound Chips  :  K-665 9546 (= M6295)

To Do:

- Flip screen unused ?
- Better driving wheel(s) support

Blomby Car is said to be a bootleg of Gaelco's World Rally and uses many
of the same fonts

Waterball

Check game speed, it depends on a bit we toggle..

***************************************************************************/

#include "driver.h"
#include "sound/okim6295.h"

/* Variables defined in vidhrdw: */

extern UINT16 *blmbycar_vram_0, *blmbycar_scroll_0;
extern UINT16 *blmbycar_vram_1, *blmbycar_scroll_1;

/* Functions defined in vidhrdw: */

WRITE16_HANDLER( blmbycar_palette_w );

WRITE16_HANDLER( blmbycar_vram_0_w );
WRITE16_HANDLER( blmbycar_vram_1_w );

VIDEO_START( blmbycar );
VIDEO_UPDATE( blmbycar );


/***************************************************************************


                                Sound Banking


***************************************************************************/

/* The top 64k of samples are banked (16 banks total) */

WRITE16_HANDLER( blmbycar_okibank_w )
{
	if (ACCESSING_LSB)
	{
		unsigned char *RAM = memory_region(REGION_SOUND1);
		memcpy(&RAM[0x30000],&RAM[0x40000 + 0x10000*(data & 0xf)],0x10000);
	}
}

/***************************************************************************


                                Input Handling


***************************************************************************/

/* Preliminary potentiometric wheel support */

static UINT8 pot_wheel = 0;

static WRITE16_HANDLER( blmbycar_pot_wheel_reset_w )
{
	if (ACCESSING_LSB)
		pot_wheel = ~readinputport(2) & 0xff;
}

static WRITE16_HANDLER( blmbycar_pot_wheel_shift_w )
{
	if (ACCESSING_LSB)
	{
		static int old;
		if ( ((old & 0xff) == 0xff) && ((data & 0xff) == 0) )
			pot_wheel <<= 1;
		old = data;
	}
}

static READ16_HANDLER( blmbycar_pot_wheel_r )
{
	return	((pot_wheel & 0x80) ? 0x04 : 0) |
			(rand() & 0x08);
}


/* Preliminary optical wheel support */

static READ16_HANDLER( blmbycar_opt_wheel_r )
{
	return	(~readinputport(2) & 0xff) << 8;
}


/***************************************************************************


                                Memory Maps


***************************************************************************/

static ADDRESS_MAP_START( blmbycar_readmem, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x0fffff) AM_READ(MRA16_ROM					)	// ROM
	AM_RANGE(0xfec000, 0xfeffff) AM_READ(MRA16_RAM					)	// RAM
	AM_RANGE(0x200000, 0x2005ff) AM_READ(MRA16_RAM					)	// Palette
	AM_RANGE(0x200600, 0x203fff) AM_READ(MRA16_RAM					)	//
	AM_RANGE(0x204000, 0x2045ff) AM_READ(MRA16_RAM					)	// Palette
	AM_RANGE(0x204600, 0x207fff) AM_READ(MRA16_RAM					)	//
	AM_RANGE(0x104000, 0x105fff) AM_READ(MRA16_RAM					)	// Layer 1
	AM_RANGE(0x106000, 0x107fff) AM_READ(MRA16_RAM					)	// Layer 0
	AM_RANGE(0x440000, 0x441fff) AM_READ(MRA16_RAM					)	//
	AM_RANGE(0x444000, 0x445fff) AM_READ(MRA16_RAM					)	// Sprites (size?)
	AM_RANGE(0x700000, 0x700001) AM_READ(input_port_0_word_r		)	// 2 x DSW0
	AM_RANGE(0x700002, 0x700003) AM_READ(input_port_1_word_r		)	// Joystick + Buttons
	AM_RANGE(0x700004, 0x700005) AM_READ(blmbycar_opt_wheel_r		)	// Wheel (optical)
	AM_RANGE(0x700006, 0x700007) AM_READ(input_port_3_word_r		)	//
	AM_RANGE(0x700008, 0x700009) AM_READ(blmbycar_pot_wheel_r		)	// Wheel (potentiometer)
	AM_RANGE(0x70000e, 0x70000f) AM_READ(OKIM6295_status_0_lsb_r	)	// Sound
ADDRESS_MAP_END

static ADDRESS_MAP_START( blmbycar_writemem, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x0fffff) AM_WRITE(MWA16_ROM								)	// ROM
	AM_RANGE(0xfec000, 0xfeffff) AM_WRITE(MWA16_RAM								)	// RAM
	AM_RANGE(0x100000, 0x103fff) AM_WRITE(MWA16_RAM								)	//
	AM_RANGE(0x104000, 0x105fff) AM_WRITE(blmbycar_vram_1_w) AM_BASE(&blmbycar_vram_1	)	// Layer 1
	AM_RANGE(0x106000, 0x107fff) AM_WRITE(blmbycar_vram_0_w) AM_BASE(&blmbycar_vram_0	)	// Layer 0
	AM_RANGE(0x108000, 0x10bfff) AM_WRITE(MWA16_RAM								)	//
	AM_RANGE(0x10c000, 0x10c003) AM_WRITE(MWA16_RAM) AM_BASE(&blmbycar_scroll_1			)	// Scroll 1
	AM_RANGE(0x10c004, 0x10c007) AM_WRITE(MWA16_RAM) AM_BASE(&blmbycar_scroll_0			)	// Scroll 0
	AM_RANGE(0x200000, 0x2005ff) AM_WRITE(blmbycar_palette_w					)	// Palette
	AM_RANGE(0x200600, 0x203fff) AM_WRITE(MWA16_RAM								)	//
	AM_RANGE(0x204000, 0x2045ff) AM_WRITE(blmbycar_palette_w) AM_BASE(&paletteram16		)	// Palette
	AM_RANGE(0x204600, 0x207fff) AM_WRITE(MWA16_RAM								)	//
	AM_RANGE(0x440000, 0x441fff) AM_WRITE(MWA16_RAM								)	//
	AM_RANGE(0x444000, 0x445fff) AM_WRITE(MWA16_RAM) AM_BASE(&spriteram16) AM_SIZE(&spriteram_size	)	// Sprites (size?)
	AM_RANGE(0x70000a, 0x70000b) AM_WRITE(MWA16_NOP								)	// ? Wheel
	AM_RANGE(0x70000c, 0x70000d) AM_WRITE(blmbycar_okibank_w					)	// Sound
	AM_RANGE(0x70000e, 0x70000f) AM_WRITE(OKIM6295_data_0_lsb_w					)	//
	AM_RANGE(0x70006a, 0x70006b) AM_WRITE(blmbycar_pot_wheel_reset_w			)	// Wheel (potentiometer)
	AM_RANGE(0x70007a, 0x70007b) AM_WRITE(blmbycar_pot_wheel_shift_w			)	//
ADDRESS_MAP_END

READ16_HANDLER( waterball_unk_r )
{
	static int retvalue = 0;

	retvalue ^= 0x0008; // must toggle.. but not vblank?

	return retvalue;
}

static ADDRESS_MAP_START( watrball_readmem, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x0fffff) AM_READ(MRA16_ROM					)	// ROM
	AM_RANGE(0xfec000, 0xfeffff) AM_READ(MRA16_RAM					)	// RAM
	AM_RANGE(0x200000, 0x2005ff) AM_READ(MRA16_RAM					)	// Palette
	AM_RANGE(0x200600, 0x203fff) AM_READ(MRA16_RAM					)	//
	AM_RANGE(0x204000, 0x2045ff) AM_READ(MRA16_RAM					)	// Palette
	AM_RANGE(0x204600, 0x207fff) AM_READ(MRA16_RAM					)	//
	AM_RANGE(0x104000, 0x105fff) AM_READ(MRA16_RAM					)	// Layer 1
	AM_RANGE(0x106000, 0x107fff) AM_READ(MRA16_RAM					)	// Layer 0
	AM_RANGE(0x440000, 0x441fff) AM_READ(MRA16_RAM					)	//
	AM_RANGE(0x444000, 0x445fff) AM_READ(MRA16_RAM					)	// Sprites (size?)
	AM_RANGE(0x700000, 0x700001) AM_READ(input_port_0_word_r		)
	AM_RANGE(0x700002, 0x700003) AM_READ(input_port_1_word_r		)
	AM_RANGE(0x700006, 0x700007) AM_READ(MRA16_NOP	            	)   // read
	AM_RANGE(0x700008, 0x700009) AM_READ(waterball_unk_r	     	)   // 0x0008 must toggle
	AM_RANGE(0x70000e, 0x70000f) AM_READ(OKIM6295_status_0_lsb_r	)	// Sound
ADDRESS_MAP_END

static ADDRESS_MAP_START( watrball_writemem, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x0fffff) AM_WRITE(MWA16_ROM								)	// ROM
	AM_RANGE(0xfec000, 0xfeffff) AM_WRITE(MWA16_RAM								)	// RAM
	AM_RANGE(0x100000, 0x103fff) AM_WRITE(MWA16_RAM								)	//
	AM_RANGE(0x104000, 0x105fff) AM_WRITE(blmbycar_vram_1_w) AM_BASE(&blmbycar_vram_1	)	// Layer 1
	AM_RANGE(0x106000, 0x107fff) AM_WRITE(blmbycar_vram_0_w) AM_BASE(&blmbycar_vram_0	)	// Layer 0
	AM_RANGE(0x108000, 0x10bfff) AM_WRITE(MWA16_RAM								)	//
	AM_RANGE(0x10c000, 0x10c003) AM_WRITE(MWA16_RAM) AM_BASE(&blmbycar_scroll_1			)	// Scroll 1
	AM_RANGE(0x10c004, 0x10c007) AM_WRITE(MWA16_RAM) AM_BASE(&blmbycar_scroll_0			)	// Scroll 0
	AM_RANGE(0x200000, 0x2005ff) AM_WRITE(blmbycar_palette_w					)	// Palette
	AM_RANGE(0x200600, 0x203fff) AM_WRITE(MWA16_RAM								)	//
	AM_RANGE(0x204000, 0x2045ff) AM_WRITE(blmbycar_palette_w) AM_BASE(&paletteram16		)	// Palette
	AM_RANGE(0x204600, 0x207fff) AM_WRITE(MWA16_RAM								)	//
	AM_RANGE(0x440000, 0x441fff) AM_WRITE(MWA16_RAM								)	//
	AM_RANGE(0x444000, 0x445fff) AM_WRITE(MWA16_RAM) AM_BASE(&spriteram16) AM_SIZE(&spriteram_size	)	// Sprites (size?)
	AM_RANGE(0x70000a, 0x70000b) AM_WRITE(MWA16_NOP								)	// ?? busy
	AM_RANGE(0x70000c, 0x70000d) AM_WRITE(blmbycar_okibank_w					)	// Sound
	AM_RANGE(0x70000e, 0x70000f) AM_WRITE(OKIM6295_data_0_lsb_w					)	//
ADDRESS_MAP_END

/***************************************************************************


                                Input Ports


***************************************************************************/

INPUT_PORTS_START( blmbycar )

	PORT_START	// IN0 - $700000.w
	PORT_DIPNAME( 0x0003, 0x0003, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(      0x0002, DEF_STR( Easy )    )
	PORT_DIPSETTING(      0x0003, DEF_STR( Normal )  )
	PORT_DIPSETTING(      0x0001, DEF_STR( Hard )    )
	PORT_DIPSETTING(      0x0000, DEF_STR( Hardest ) )
	PORT_DIPNAME( 0x0004, 0x0004, "Joysticks" )
	PORT_DIPSETTING(      0x0000, "1" )
	PORT_DIPSETTING(      0x0004, "2" )
	PORT_DIPNAME( 0x0018, 0x0018, DEF_STR( Controls ) )
	PORT_DIPSETTING(      0x0018, DEF_STR( Joystick ) )
//  PORT_DIPSETTING(      0x0010, "Pot Wheel" ) // Preliminary
//  PORT_DIPSETTING(      0x0008, "Opt Wheel" ) // Preliminary
//  PORT_DIPSETTING(      0x0000, DEF_STR( Unused ) )   // Time goes to 0 rally fast!
	PORT_DIPNAME( 0x0020, 0x0000, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(      0x0020, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0040, 0x0040, "Unknown 1-6" )
	PORT_DIPSETTING(      0x0040, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_SERVICE( 0x0080, IP_ACTIVE_LOW )

	PORT_DIPNAME( 0x0700, 0x0700, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(      0x0700, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( 3C_4C ) )
	PORT_DIPSETTING(      0x0100, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(      0x0600, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(      0x0500, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(      0x0400, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(      0x0300, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(      0x0200, DEF_STR( 1C_6C ) )
	PORT_DIPNAME( 0x3800, 0x3800, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(      0x1000, DEF_STR( 6C_1C ) )
	PORT_DIPSETTING(      0x1800, DEF_STR( 5C_1C ) )
	PORT_DIPSETTING(      0x2000, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(      0x2800, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(      0x3000, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0x0800, DEF_STR( 3C_2C ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( 4C_3C ) )
	PORT_DIPSETTING(      0x3800, DEF_STR( 1C_1C ) )
	PORT_DIPNAME( 0x4000, 0x4000, "Credits To Start" )
	PORT_DIPSETTING(      0x4000, "1" )
	PORT_DIPSETTING(      0x0000, "2" )
	PORT_DIPNAME( 0x8000, 0x8000, DEF_STR( Free_Play ) )
	PORT_DIPSETTING(      0x8000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )

	PORT_START	// IN1 - $700002.w
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_PLAYER(1)
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN	 ) PORT_PLAYER(1)
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_PLAYER(1)
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_PLAYER(1)
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(1)
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(1)
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_COIN1  )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_COIN2  )

	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_PLAYER(2)
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN	 ) PORT_PLAYER(2)
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_PLAYER(2)
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_PLAYER(2)
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(2)
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(2)
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_START1  )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_START2  )

	PORT_START	// IN2 - $700004.w
	PORT_BIT ( 0x00ff, 0x0080, IPT_AD_STICK_X ) PORT_MINMAX(0x00,0xff) PORT_SENSITIVITY(30) PORT_KEYDELTA(1)

	PORT_START	// IN3 - $700006.w
	PORT_BIT( 0xffff, IP_ACTIVE_LOW, IPT_UNKNOWN )

INPUT_PORTS_END


INPUT_PORTS_START( watrball )
	PORT_START	/* dips */
	PORT_DIPNAME( 0x0001, 0x0001, "1" )
	PORT_DIPSETTING(      0x0001, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0002, 0x0002, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0002, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0004, 0x0004, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0004, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0008, 0x0008, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0008, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0010, 0x0010, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0010, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0020, 0x0020, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0020, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0040, 0x0040, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0040, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0080, 0x0080, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0080, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0100, 0x0100, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0100, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0200, 0x0200, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0200, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0400, 0x0400, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0400, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0800, 0x0800, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0800, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x1000, 0x1000, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x1000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x2000, 0x2000, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x2000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x4000, 0x4000, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x4000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x8000, 0x8000, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x8000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )

	PORT_START	/* 16bit */
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_PLAYER(1)
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN	 ) PORT_PLAYER(1)
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_PLAYER(1)
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_PLAYER(1)
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(1)
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(1)
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_COIN1  )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_COIN2  )

	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_PLAYER(2)
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN	 ) PORT_PLAYER(2)
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_PLAYER(2)
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_PLAYER(2)
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(2)
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(2)
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_START1  )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_START2  )
INPUT_PORTS_END

/***************************************************************************


                                Graphics Layouts


***************************************************************************/

/* 16x16x4 tiles (made of four 8x8 tiles) */
static const gfx_layout layout_16x16x4 =
{
	16,16,
	RGN_FRAC(1,4),
	4,
	{ RGN_FRAC(3,4),RGN_FRAC(2,4),RGN_FRAC(1,4),RGN_FRAC(0,4) },
	{ STEP8(0,1), STEP8(8*8*2,1) },
	{ STEP8(0,8), STEP8(8*8*1,8) },
	16*16
};

/* Layers both use the first $20 color codes. Sprites the next $10 */
static const gfx_decode blmbycar_gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &layout_16x16x4, 0x0, 0x30 }, // [0] Layers + Sprites
	{ -1 }
};



/***************************************************************************


                                Machine Drivers


***************************************************************************/

static MACHINE_DRIVER_START( blmbycar )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000, 10000000)	/* ? */
	MDRV_CPU_PROGRAM_MAP(blmbycar_readmem,blmbycar_writemem)
	MDRV_CPU_VBLANK_INT(irq1_line_hold,1)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(0x180, 0x100)
	MDRV_VISIBLE_AREA(0, 0x180-1, 0, 0x100-1)
	MDRV_GFXDECODE(blmbycar_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(0x300)

	MDRV_VIDEO_START(blmbycar)
	MDRV_VIDEO_UPDATE(blmbycar)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_STEREO("left", "right")

	MDRV_SOUND_ADD(OKIM6295, 8000)
	MDRV_SOUND_CONFIG(okim6295_interface_region_1)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "left", 1.0)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "right", 1.0)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( watrball )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000, 10000000)	/* ? */
	MDRV_CPU_PROGRAM_MAP(watrball_readmem,watrball_writemem)
	MDRV_CPU_VBLANK_INT(irq1_line_hold,1)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_REAL_60HZ_VBLANK_DURATION)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(0x180, 0x100)
	MDRV_VISIBLE_AREA(0, 0x180-1, 16, 0x100-1)
	MDRV_GFXDECODE(blmbycar_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(0x300)

	MDRV_VIDEO_START(blmbycar)
	MDRV_VIDEO_UPDATE(blmbycar)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_STEREO("left", "right")

	MDRV_SOUND_ADD(OKIM6295, 8000)
	MDRV_SOUND_CONFIG(okim6295_interface_region_1)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "left", 1.0)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "right", 1.0)
MACHINE_DRIVER_END


/***************************************************************************


                                ROMs Loading


***************************************************************************/

/***************************************************************************

                                Blomby Car
Abm & Gecas, 1990.

CPU : 68000
SND : Oki M6295 (samples only)
OSC : 30.000 + 24.000
DSW : 2 x 8
GFX : TI TPC1020AFN-084

***************************************************************************/

ROM_START( blmbycar )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )		/* 68000 Code */
	ROM_LOAD16_BYTE( "bcrom4.bin", 0x000000, 0x080000, CRC(06d490ba) SHA1(6d113561b474bf613c6b91c9525a52025ae65ab7) )
	ROM_LOAD16_BYTE( "bcrom6.bin", 0x000001, 0x080000, CRC(33aca664) SHA1(04fff492654d3edac62e9d35808e5946bcc78cbb) )

	ROM_REGION( 0x200000, REGION_GFX1, ROMREGION_DISPOSE )	/* Sprites */
	ROM_LOAD( "bc_rom7",   0x000000, 0x080000, CRC(e55ca79b) SHA1(4453a6ae0518832f437ab701c28cb2f32920f8ba) )
	ROM_LOAD( "bc_rom8",   0x080000, 0x080000, CRC(cdf38c96) SHA1(3273c29b6a01a7296d06fc653120f8c615195d2c) )
	ROM_LOAD( "bc_rom9",   0x100000, 0x080000, CRC(0337ab3d) SHA1(18c72cd640c7b599390dffaeb670f5832202bf06) )
	ROM_LOAD( "bc_rom10",  0x180000, 0x080000, CRC(5458917e) SHA1(c8dd5a391cc20a573e27a140b185893a8c04859e) )

	ROM_REGION( 0x140000, REGION_SOUND1, 0 )	/* 8 bit adpcm (banked) */
	ROM_LOAD( "bc_rom1",     0x040000, 0x080000, CRC(ac6f8ba1) SHA1(69d2d47cdd331bde5a8973d29659b3f8520452e7) )
	ROM_LOAD( "bc_rom2",     0x0c0000, 0x080000, CRC(a4bc31bf) SHA1(f3d60141a91449a73f6cec9f4bc5d95ca7911e19) )
	ROM_COPY( REGION_SOUND1, 0x040000, 0x000000,   0x040000 )
ROM_END

ROM_START( blmbycau )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )		/* 68000 Code */
	ROM_LOAD16_BYTE( "bc_rom4", 0x000000, 0x080000, CRC(76f054a2) SHA1(198efd152b13033e5249119ca48b9e0f6351b0b9) )
	ROM_LOAD16_BYTE( "bc_rom6", 0x000001, 0x080000, CRC(2570b4c5) SHA1(706465950023a6ef7c85ceb9c76246d7556b3859) )

	ROM_REGION( 0x200000, REGION_GFX1, ROMREGION_DISPOSE )	/* Sprites */
	ROM_LOAD( "bc_rom7",   0x000000, 0x080000, CRC(e55ca79b) SHA1(4453a6ae0518832f437ab701c28cb2f32920f8ba) )
	ROM_LOAD( "bc_rom8",   0x080000, 0x080000, CRC(cdf38c96) SHA1(3273c29b6a01a7296d06fc653120f8c615195d2c) )
	ROM_LOAD( "bc_rom9",   0x100000, 0x080000, CRC(0337ab3d) SHA1(18c72cd640c7b599390dffaeb670f5832202bf06) )
	ROM_LOAD( "bc_rom10",  0x180000, 0x080000, CRC(5458917e) SHA1(c8dd5a391cc20a573e27a140b185893a8c04859e) )

	ROM_REGION( 0x140000, REGION_SOUND1, 0 )	/* 8 bit adpcm (banked) */
	ROM_LOAD( "bc_rom1",     0x040000, 0x080000, CRC(ac6f8ba1) SHA1(69d2d47cdd331bde5a8973d29659b3f8520452e7) )
	ROM_LOAD( "bc_rom2",     0x0c0000, 0x080000, CRC(a4bc31bf) SHA1(f3d60141a91449a73f6cec9f4bc5d95ca7911e19) )
	ROM_COPY( REGION_SOUND1, 0x040000, 0x000000,   0x040000 )
ROM_END

/*

Waterball by ABM (sticker on the pcb 12-3-96)
The pcb has some empty sockets, maybe it was used for other games since it has no markings.

The game has fonts identical to World rally and obiviously Blomby car ;)

1x 68k
1x oki 6295
1x OSC 30mhz
1x OSC 24mhz
1x FPGA
1x dispswitch

*/

ROM_START( watrball )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )		/* 68000 Code */
	ROM_LOAD16_BYTE( "rom4.bin", 0x000000, 0x020000, CRC(bfbfa720) SHA1(d6d14c0ba545eb7adee7190da2d3db1c6dd00d75) )
	ROM_LOAD16_BYTE( "rom6.bin", 0x000001, 0x020000, CRC(acff9b01) SHA1(b85671bcc4f03fdf05eb1c9b5d4143e33ec1d7db) )

	ROM_REGION( 0x200000, REGION_GFX1, ROMREGION_DISPOSE )	/* Sprites */
	ROM_LOAD( "rom7.bin",   0x000000, 0x080000, CRC(e7e5c311) SHA1(5af1a666bf23c5505d120d81fb942f5c49341861) )
	ROM_LOAD( "rom8.bin",   0x080000, 0x080000, CRC(fd27ce6e) SHA1(a472a8cc25818427d2870518649780146e51835b) )
	ROM_LOAD( "rom9.bin",   0x100000, 0x080000, CRC(122cc0ad) SHA1(27cdb19fa082089e47c5cdb44742cfd93aa23a00) )
	ROM_LOAD( "rom10.bin",  0x180000, 0x080000, CRC(22a2a706) SHA1(c7350a94a857e0007d7fc0076b44a3d62693cb6c) )

	ROM_REGION( 0x140000, REGION_SOUND1, 0 )	/* 8 bit adpcm (banked) */
	ROM_LOAD( "rom1.bin",     0x040000, 0x080000, CRC(7f88dee7) SHA1(d493b961fa4631185a33faee7f61786430707209))
//  ROM_LOAD( "rom2.bin",     0x0c0000, 0x080000, /* not populated for this game */ )
	ROM_COPY( REGION_SOUND1, 0x040000, 0x000000,   0x040000 )
ROM_END


DRIVER_INIT( blmbycar )
{
	UINT16 *RAM  = (UINT16 *) memory_region(REGION_CPU1);
	size_t    size = memory_region_length(REGION_CPU1) / 2;
	int i;

	for (i = 0; i < size; i++)
	{
		UINT16 x = RAM[i];
		x = (x & ~0x0606) | ((x & 0x0202) << 1) | ((x & 0x0404) >> 1);
		RAM[i] = x;
	}
}

/***************************************************************************


                                Game Drivers


***************************************************************************/

GAME( 1994, blmbycar, 0,        blmbycar, blmbycar, blmbycar, ROT0, "ABM & Gecas", "Blomby Car", 0 )
GAME( 1994, blmbycau, blmbycar, blmbycar, blmbycar, 0,        ROT0, "ABM & Gecas", "Blomby Car (not encrypted)", 0 )
GAME( 1996, watrball, 0,        watrball, watrball, 0,        ROT0, "ABM", "Water Balls", 0 )
