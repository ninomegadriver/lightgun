#include "driver.h"

extern UINT8 *kopunch_videoram2;

extern WRITE8_HANDLER( kopunch_videoram_w );
extern WRITE8_HANDLER( kopunch_videoram2_w );
extern WRITE8_HANDLER( kopunch_scroll_x_w );
extern WRITE8_HANDLER( kopunch_scroll_y_w );
extern WRITE8_HANDLER( kopunch_gfxbank_w );

extern PALETTE_INIT( kopunch );
extern VIDEO_START( kopunch );
extern VIDEO_UPDATE( kopunch );


INTERRUPT_GEN( kopunch_interrupt )
{
	if (cpu_getiloops() == 0)
	{
		if (~input_port_1_r(0) & 0x80)	/* coin 1 */
		{
			cpunum_set_input_line_and_vector(0,0,HOLD_LINE,0xf7);	/* RST 30h */
			return;
		}
		else if (~input_port_1_r(0) & 0x08)	/* coin 2 */
		{
			cpunum_set_input_line_and_vector(0,0,HOLD_LINE,0xef);	/* RST 28h */
			return;
		}
	}

	cpunum_set_input_line_and_vector(0,0,HOLD_LINE,0xff);	/* RST 38h */
}

static READ8_HANDLER( kopunch_in_r )
{
	/* port 31 + low 3 bits of port 32 contain the punch strength */
	if (offset == 0)
		return rand();
	else
		return (rand() & 0x07) | input_port_1_r(0);
}

static WRITE8_HANDLER( kopunch_lamp_w )
{
	set_led_status(0,~data & 0x80);

//  if ((data & 0x7f) != 0x7f)
//      ui_popup("port 38 = %02x",data);
}

static WRITE8_HANDLER( kopunch_coin_w )
{
	coin_counter_w(0,~data & 0x80);
	coin_counter_w(1,~data & 0x40);

//  if ((data & 0x3f) != 0x3f)
//      ui_popup("port 34 = %02x",data);
}



static ADDRESS_MAP_START( kopunch_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x1fff) AM_ROM
	AM_RANGE(0x2000, 0x23ff) AM_RAM
	AM_RANGE(0x6000, 0x63ff) AM_RAM AM_WRITE(kopunch_videoram_w) AM_BASE(&videoram)
	AM_RANGE(0x7000, 0x70ff) AM_RAM AM_WRITE(kopunch_videoram2_w) AM_BASE(&kopunch_videoram2)
	AM_RANGE(0x7100, 0x7aff) AM_RAM	// ???
ADDRESS_MAP_END

static ADDRESS_MAP_START( kopunch_io_map, ADDRESS_SPACE_IO, 8 )
	AM_RANGE(0x30, 0x30) AM_READ(input_port_0_r)
	AM_RANGE(0x31, 0x32) AM_READ(kopunch_in_r)
	AM_RANGE(0x33, 0x33) AM_WRITENOP
	AM_RANGE(0x34, 0x34) AM_WRITE(kopunch_coin_w)
	AM_RANGE(0x35, 0x35) AM_WRITENOP
	AM_RANGE(0x36, 0x36) AM_WRITENOP
	AM_RANGE(0x37, 0x37) AM_WRITENOP
	AM_RANGE(0x38, 0x38) AM_WRITE(kopunch_lamp_w)
	AM_RANGE(0x39, 0x39) AM_WRITENOP
	AM_RANGE(0x3a, 0x3a) AM_READ(input_port_2_r)
	AM_RANGE(0x3b, 0x3b) AM_WRITENOP
	AM_RANGE(0x3c, 0x3c) AM_WRITE(kopunch_scroll_x_w)
	AM_RANGE(0x3d, 0x3d) AM_WRITE(kopunch_scroll_y_w)
	AM_RANGE(0x3e, 0x3e) AM_READWRITE(input_port_3_r, kopunch_gfxbank_w)
	AM_RANGE(0x3f, 0x3f) AM_WRITENOP
ADDRESS_MAP_END


INPUT_PORTS_START( kopunch )
	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_4WAY
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_4WAY
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_4WAY
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_4WAY
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON3 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON4 )

	PORT_START
	PORT_BIT( 0x07, IP_ACTIVE_HIGH, IPT_SPECIAL )	/* punch strength (high 3 bits) */
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_COIN2 ) PORT_IMPULSE(1)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON3 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN1 ) PORT_IMPULSE(1)

	PORT_START
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_4WAY PORT_COCKTAIL
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_4WAY PORT_COCKTAIL
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_4WAY PORT_COCKTAIL
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_4WAY PORT_COCKTAIL
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_COCKTAIL
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_COCKTAIL
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_COCKTAIL
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_SERVICE1 )
INPUT_PORTS_END


static const gfx_layout charlayout =
{
	8,8,
	RGN_FRAC(1,3),
	3,
	{ RGN_FRAC(2,3), RGN_FRAC(1,3), RGN_FRAC(0,3) },
	{ 7, 6, 5, 4, 3, 2, 1, 0 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8
};

static const gfx_decode gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &charlayout, 0, 1 },
	{ REGION_GFX2, 0, &charlayout, 0, 1 },
	{ -1 } /* end of array */
};


static MACHINE_DRIVER_START( kopunch )

	/* basic machine hardware */
	MDRV_CPU_ADD(8080, 4000000)	/* 4 MHz ???? appears to use 8080 instructions, not z80 */
	MDRV_CPU_PROGRAM_MAP(kopunch_map,0)
	MDRV_CPU_IO_MAP(kopunch_io_map,0)
	MDRV_CPU_VBLANK_INT(kopunch_interrupt,4)	/* ??? */

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(32*8, 32*8)
	MDRV_VISIBLE_AREA(0*8, 32*8-1, 1*8, 31*8-1)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(8)

	MDRV_PALETTE_INIT(kopunch)
	MDRV_VIDEO_START(kopunch)
	MDRV_VIDEO_UPDATE(kopunch)

	/* sound hardware */
MACHINE_DRIVER_END


/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START( kopunch )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "epr1105.x",    0x0000, 0x1000, CRC(34ef5e79) SHA1(2827c68f4c902f447a304d3ab0258c7819a0e4ca) )
	ROM_LOAD( "epr1106.x",    0x1000, 0x1000, CRC(25a5c68b) SHA1(9761418c6f3903f8aaceece658739fe5bf5c0803) )

	ROM_REGION( 0x1800, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "epr1102",      0x0000, 0x0800, CRC(8a52de96) SHA1(5abdaa83c6bfea81395cb190f5364b72811927ba) )
	ROM_LOAD( "epr1103",      0x0800, 0x0800, CRC(bae5e054) SHA1(95373123ab64543cdffb7ee9e02d0613c5c494bf) )
	ROM_LOAD( "epr1104",      0x1000, 0x0800, CRC(7b119a0e) SHA1(454f01355fa9512a7442990cc92da7bc7a8d6b68) )

	ROM_REGION( 0x6000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "epr1107",      0x0000, 0x1000, CRC(ca00244d) SHA1(690931ea1bef9d80dcd7bd2ea2462b083c884a89) )
	ROM_LOAD( "epr1108",      0x1000, 0x1000, CRC(cc17c5ed) SHA1(693df076e16cc3a3dd54f6680691e658da3942fe) )
	ROM_LOAD( "epr1110",      0x2000, 0x1000, CRC(ae0aff15) SHA1(7f71c94bacdb444e5ed4f917c5a7de17012027a9) )
	ROM_LOAD( "epr1109",      0x3000, 0x1000, CRC(625446ba) SHA1(e4acedc8ddaf7e825930260d12601085ed89dced) )
	ROM_LOAD( "epr1112",      0x4000, 0x1000, CRC(ef6994df) SHA1(2d68650b6b875bcfdc9f977f96044c6867aa40a6) )
	ROM_LOAD( "epr1111",      0x5000, 0x1000, CRC(28530ec9) SHA1(1a8782d37128cdb43133fc891cde93d2bdd5476b) )

	ROM_REGION( 0x0060, REGION_PROMS, 0 )
	ROM_LOAD( "epr1101",      0x0000, 0x0020, CRC(15600f5d) SHA1(130179f79761cb16316c544e3c689bc10431db30) )	/* palette */
	ROM_LOAD( "epr1099",      0x0020, 0x0020, CRC(fc58c456) SHA1(f27c3ad669dfdc33bcd7e0481fa01bf34973e816) )	/* unknown */
	ROM_LOAD( "epr1100",      0x0040, 0x0020, CRC(bedb66b1) SHA1(8e78bb205d900075b761e1baa5f5813174ff28ba) )	/* unknown */
ROM_END


static DRIVER_INIT( kopunch )
{
//  UINT8 *rom = memory_region(REGION_CPU1);

	/* It looks like there is a security chip, that changes instruction of the form:
        0334: 3E 0C       ld   a,$0C
        0336: 30 FB       jr   nc,$0333
       into something else (maybe just a nop) with the effect of resuming execution
       from the operand of the JR  NC instruction (in the example above, 0337).
       For now, I'm just patching the affected instructions. */

/*  rom[0x119] = 0;
    rom[0x336] = 0;
    rom[0x381] = 0;
    rom[0xf0b] = 0;
    rom[0xf33] = 0;*/
}


GAME( 1981, kopunch, 0, kopunch, kopunch, kopunch, ROT270, "Sega", "KO Punch", GAME_NO_SOUND | GAME_NOT_WORKING)
