/***************************************************************************

Block Hole (GX973) (c) 1989 Konami

driver by Nicola Salmoria

Notes:
Quarth works, but Block Hole crashes when it reaches the title screen. An
interrupt happens, and after rti the ROM bank is not the same as before so
it jumps to garbage code.
If you want to see this happen, place a breakpoint at 0x8612, and trace
after that.
The code is almost identical in the two versions, it looks like Quarth is
working just because luckily the interrupt doesn't happen at that point.
It seems that the interrupt handler trashes the selected ROM bank and forces
it to 0. To prevent crashes, I only generate interrupts when the ROM bank is
already 0. There might be another interrupt enable register, but I haven't
found it.

***************************************************************************/

#include "driver.h"
#include "cpu/konami/konami.h" /* for the callback and the firq irq definition */
#include "vidhrdw/konamiic.h"
#include "sound/2151intf.h"

/* prototypes */
static MACHINE_RESET( blockhl );
static void blockhl_banking( int lines );


VIDEO_START( blockhl );
VIDEO_UPDATE( blockhl );

static int palette_selected;
static unsigned char *ram;
static int rombank;

static INTERRUPT_GEN( blockhl_interrupt )
{
	if (K052109_is_IRQ_enabled() && rombank == 0)	/* kludge to prevent crashes */
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

WRITE8_HANDLER( blockhl_sh_irqtrigger_w )
{
	cpunum_set_input_line_and_vector(1, 0, HOLD_LINE, 0xff);
}



static ADDRESS_MAP_START( readmem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x1f94, 0x1f94) AM_READ(input_port_4_r)
	AM_RANGE(0x1f95, 0x1f95) AM_READ(input_port_0_r)
	AM_RANGE(0x1f96, 0x1f96) AM_READ(input_port_1_r)
	AM_RANGE(0x1f97, 0x1f97) AM_READ(input_port_2_r)
	AM_RANGE(0x1f98, 0x1f98) AM_READ(input_port_3_r)
	AM_RANGE(0x0000, 0x3fff) AM_READ(K052109_051960_r)
	AM_RANGE(0x4000, 0x57ff) AM_READ(MRA8_RAM)
	AM_RANGE(0x5800, 0x5fff) AM_READ(bankedram_r)
	AM_RANGE(0x6000, 0x7fff) AM_READ(MRA8_BANK1)
	AM_RANGE(0x8000, 0xffff) AM_READ(MRA8_ROM)
ADDRESS_MAP_END

static ADDRESS_MAP_START( writemem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x1f84, 0x1f84) AM_WRITE(soundlatch_w)
	AM_RANGE(0x1f88, 0x1f88) AM_WRITE(blockhl_sh_irqtrigger_w)
	AM_RANGE(0x1f8c, 0x1f8c) AM_WRITE(watchdog_reset_w)
	AM_RANGE(0x0000, 0x3fff) AM_WRITE(K052109_051960_w)
	AM_RANGE(0x4000, 0x57ff) AM_WRITE(MWA8_RAM)
	AM_RANGE(0x5800, 0x5fff) AM_WRITE(bankedram_w) AM_BASE(&ram)
	AM_RANGE(0x6000, 0x7fff) AM_WRITE(MWA8_ROM)
	AM_RANGE(0x8000, 0xffff) AM_WRITE(MWA8_ROM)
ADDRESS_MAP_END

static ADDRESS_MAP_START( sound_readmem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x7fff) AM_READ(MRA8_ROM)
	AM_RANGE(0x8000, 0x87ff) AM_READ(MRA8_RAM)
	AM_RANGE(0xa000, 0xa000) AM_READ(soundlatch_r)
	AM_RANGE(0xc001, 0xc001) AM_READ(YM2151_status_port_0_r)
ADDRESS_MAP_END

static ADDRESS_MAP_START( sound_writemem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x7fff) AM_WRITE(MWA8_ROM)
	AM_RANGE(0x8000, 0x87ff) AM_WRITE(MWA8_RAM)
	AM_RANGE(0xc000, 0xc000) AM_WRITE(YM2151_register_port_0_w)
	AM_RANGE(0xc001, 0xc001) AM_WRITE(YM2151_data_port_0_w)
	AM_RANGE(0xe00c, 0xe00d) AM_WRITE(MWA8_NOP)		/* leftover from missing 007232? */
ADDRESS_MAP_END

/***************************************************************************

    Input Ports

***************************************************************************/

INPUT_PORTS_START( blockhl )
	PORT_START	/* PLAYER 1 INPUTS */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(1)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(1)
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_PLAYER(1)
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START1 )

	PORT_START	/* PLAYER 2 INPUTS */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(2)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(2)
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_PLAYER(2)
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START2 )

	PORT_START	/* DSW #1 */
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

	PORT_START	/* DSW #2 */
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x01, "1" )
	PORT_DIPSETTING(    0x00, "2" )
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
	PORT_DIPNAME( 0x60, 0x40, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x60, DEF_STR( Easy ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Normal ) )
	PORT_DIPSETTING(    0x20, "Difficult" )
	PORT_DIPSETTING(    0x00, "Very Difficult" )
	PORT_DIPNAME( 0x80, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START	/* DSW #3 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_COIN3 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_SERVICE( 0x40, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
INPUT_PORTS_END


/***************************************************************************

    Machine Driver

***************************************************************************/

static MACHINE_DRIVER_START( blockhl )

	/* basic machine hardware */
	MDRV_CPU_ADD(KONAMI,3000000)		/* Konami custom 052526 */
	MDRV_CPU_PROGRAM_MAP(readmem,writemem)
	MDRV_CPU_VBLANK_INT(blockhl_interrupt,1)

	MDRV_CPU_ADD(Z80, 3579545)
	/* audio CPU */		/* ? */
	MDRV_CPU_PROGRAM_MAP(sound_readmem,sound_writemem)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)
	MDRV_MACHINE_RESET(blockhl)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER | VIDEO_HAS_SHADOWS)
	MDRV_SCREEN_SIZE(64*8, 32*8)
	MDRV_VISIBLE_AREA(14*8, (64-14)*8-1, 2*8, 30*8-1 )
	MDRV_PALETTE_LENGTH(1024)

	MDRV_VIDEO_START(blockhl)
	MDRV_VIDEO_UPDATE(blockhl)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")

	MDRV_SOUND_ADD(YM2151, 3579545)
	MDRV_SOUND_ROUTE(0, "mono", 0.60)
	MDRV_SOUND_ROUTE(1, "mono", 0.60)
MACHINE_DRIVER_END


/***************************************************************************

  Game ROMs

***************************************************************************/

ROM_START( blockhl )
	ROM_REGION( 0x18800, REGION_CPU1, 0 ) /* code + banked roms + space for banked RAM */
	ROM_LOAD( "973l02.e21", 0x10000, 0x08000, CRC(e14f849a) SHA1(d44cf178cc98998b72ed32c6e20b6ebdf1f97579) )
	ROM_CONTINUE(           0x08000, 0x08000 )

	ROM_REGION( 0x10000, REGION_CPU2, 0 ) /* 64k for the sound CPU */
	ROM_LOAD( "973d01.g6",  0x0000, 0x8000, CRC(eeee9d92) SHA1(6c6c324b1f6f4fba0aa12e0d1fc5dbab133ef669) )

	ROM_REGION( 0x20000, REGION_GFX1, 0 ) /* graphics (addressable by the main CPU) */
	ROM_LOAD16_BYTE( "973f07.k15", 0x00000, 0x08000, CRC(1a8cd9b4) SHA1(7cb7944d24ac51fa6b610542d9dec68697cacf0f) )	/* tiles */
	ROM_LOAD16_BYTE( "973f08.k18", 0x00001, 0x08000, CRC(952b51a6) SHA1(017575738d444b688b137cad5611638d53be84f2) )
	ROM_LOAD16_BYTE( "973f09.k20", 0x10000, 0x08000, CRC(77841594) SHA1(e1bfdc5bb598d865868d578ef7faba8078becd7a) )
	ROM_LOAD16_BYTE( "973f10.k23", 0x10001, 0x08000, CRC(09039fab) SHA1(a9dea17aacf4484d21ef3b16470263447b51b6b5) )

	ROM_REGION( 0x20000, REGION_GFX2, 0 ) /* graphics (addressable by the main CPU) */
	ROM_LOAD16_BYTE( "973f06.k12", 0x00000, 0x08000, CRC(51acfdb6) SHA1(94d243f341b490684f5297d95d4835bd522ece35) )	/* sprites */
	ROM_LOAD16_BYTE( "973f05.k9",  0x00001, 0x08000, CRC(4cfea298) SHA1(4772b5b99f5fd8174d8884bd84173512e1edabf4) )
	ROM_LOAD16_BYTE( "973f04.k7",  0x10000, 0x08000, CRC(69ca41bd) SHA1(9b0b1c888efd2f2d5525f14778e18fb4a7353eb6) )
	ROM_LOAD16_BYTE( "973f03.k4",  0x10001, 0x08000, CRC(21e98472) SHA1(8c697d369a1f57be0825c33b4e9107ce1b02a130) )

	ROM_REGION( 0x0100, REGION_PROMS, 0 )	/* PROMs */
	ROM_LOAD( "973a11.h10", 0x0000, 0x0100, CRC(46d28fe9) SHA1(9d0811a928c8907785ef483bfbee5445506b3ec8) )	/* priority encoder (not used) */
ROM_END

ROM_START( quarth )
	ROM_REGION( 0x18800, REGION_CPU1, 0 ) /* code + banked roms + space for banked RAM */
	ROM_LOAD( "973j02.e21", 0x10000, 0x08000, CRC(27a90118) SHA1(51309385b93db29b9277d14252166c4ea1746303) )
	ROM_CONTINUE(           0x08000, 0x08000 )

	ROM_REGION( 0x10000, REGION_CPU2, 0 ) /* 64k for the sound CPU */
	ROM_LOAD( "973d01.g6",  0x0000, 0x8000, CRC(eeee9d92) SHA1(6c6c324b1f6f4fba0aa12e0d1fc5dbab133ef669) )

	ROM_REGION( 0x20000, REGION_GFX1, 0 ) /* graphics (addressable by the main CPU) */
	ROM_LOAD16_BYTE( "973e07.k15", 0x00000, 0x08000, CRC(0bd6b0f8) SHA1(6c59cf637354fe2df424eaa89feb9c1bc1f66a92) )	/* tiles */
	ROM_LOAD16_BYTE( "973e08.k18", 0x00001, 0x08000, CRC(104d0d5f) SHA1(595698911513113d01e5b565f5b073d1bd033d3f) )
	ROM_LOAD16_BYTE( "973e09.k20", 0x10000, 0x08000, CRC(bd3a6f24) SHA1(eb45db3a6a52bb2b25df8c2dace877e59b4130a6) )
	ROM_LOAD16_BYTE( "973e10.k23", 0x10001, 0x08000, CRC(cf5e4b86) SHA1(43348753894c1763b26dbfc70245dac92048db8f) )

	ROM_REGION( 0x20000, REGION_GFX2, 0 ) /* graphics (addressable by the main CPU) */
	ROM_LOAD16_BYTE( "973e06.k12", 0x00000, 0x08000, CRC(0d58af85) SHA1(2efd661d614fb305a14cfe1aa4fb17714f215d4f) )	/* sprites */
	ROM_LOAD16_BYTE( "973e05.k9",  0x00001, 0x08000, CRC(15d822cb) SHA1(70ecad5e0a461df0da6e6eb23f43a7b643297f0d) )
	ROM_LOAD16_BYTE( "973e04.k7",  0x10000, 0x08000, CRC(d70f4a2c) SHA1(25f835a17bacf2b8debb2eb8a3cff90cab3f402a) )
	ROM_LOAD16_BYTE( "973e03.k4",  0x10001, 0x08000, CRC(2c5a4b4b) SHA1(e2991dd78b9cd96cf93ebd6de0d4e060d346ab9c) )

	ROM_REGION( 0x0100, REGION_PROMS, 0 )	/* PROMs */
	ROM_LOAD( "973a11.h10", 0x0000, 0x0100, CRC(46d28fe9) SHA1(9d0811a928c8907785ef483bfbee5445506b3ec8) )	/* priority encoder (not used) */
ROM_END


/***************************************************************************

  Game driver(s)

***************************************************************************/

static void blockhl_banking( int lines )
{
	unsigned char *RAM = memory_region(REGION_CPU1);
	int offs;

	/* bits 0-1 = ROM bank */
	rombank = lines & 0x03;
	offs = 0x10000 + (lines & 0x03) * 0x2000;
	memory_set_bankptr(1,&RAM[offs]);

	/* bits 3/4 = coin counters */
	coin_counter_w(0,lines & 0x08);
	coin_counter_w(1,lines & 0x10);

	/* bit 5 = select palette RAM or work RAM at 5800-5fff */
	palette_selected = ~lines & 0x20;

	/* bit 6 = enable char ROM reading through the video RAM */
	K052109_set_RMRD_line( ( lines & 0x40 ) ? ASSERT_LINE : CLEAR_LINE );

	/* bit 7 used but unknown */

	/* other bits unknown */

	if ((lines & 0x84) != 0x80) logerror("%04x: setlines %02x\n",activecpu_get_pc(),lines);
}

static MACHINE_RESET( blockhl )
{
	unsigned char *RAM = memory_region(REGION_CPU1);

	cpunum_set_info_fct(0, CPUINFO_PTR_KONAMI_SETLINES_CALLBACK, (genf *)blockhl_banking);

	paletteram = &RAM[0x18000];
}


static DRIVER_INIT( blockhl )
{
	konami_rom_deinterleave_2(REGION_GFX1);
	konami_rom_deinterleave_2(REGION_GFX2);
}



GAME( 1989, blockhl, 0,       blockhl, blockhl, blockhl, ROT0, "Konami", "Block Hole", 0 )
GAME( 1989, quarth,  blockhl, blockhl, blockhl, blockhl, ROT0, "Konami", "Quarth (Japan)", 0 )
