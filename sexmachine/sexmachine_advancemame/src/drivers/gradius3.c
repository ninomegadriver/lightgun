/***************************************************************************

Gradius 3 (GX945) (c) 1989 Konami

driver by Nicola Salmoria

This board uses the well known 052109 051962 custom gfx chips, however unlike
all other games they fetch gfx data from RAM. The gfx ROMs are memory mapped
on cpu B and the needed parts are copied to RAM at run time.
To handle this efficiently in MAME, some changes would be required to the
tilemap system and to vidhrdw/konamiic.c. For the time being, I'm kludging
my way in.
There's also something wrong in the way tile banks are implemented in
konamiic.c. They don't seem to be used by this game.

***************************************************************************/

#include "driver.h"
#include "vidhrdw/konamiic.h"
#include "cpu/m68000/m68000.h"
#include "cpu/z80/z80.h"
#include "sound/2151intf.h"
#include "sound/k007232.h"


extern UINT16 *gradius3_gfxram;
extern int gradius3_priority;
VIDEO_START( gradius3 );
READ16_HANDLER( gradius3_gfxrom_r );
READ16_HANDLER( gradius3_gfxram_r );
WRITE16_HANDLER( gradius3_gfxram_w );
VIDEO_UPDATE( gradius3 );



static READ16_HANDLER( K052109_halfword_r )
{
	return K052109_r(offset);
}

static WRITE16_HANDLER( K052109_halfword_w )
{
	if (ACCESSING_LSB)
		K052109_w(offset,data & 0xff);

	/* is this a bug in the game or something else? */
	if (!ACCESSING_LSB)
		K052109_w(offset,(data >> 8) & 0xff);
//      logerror("%06x half %04x = %04x\n",activecpu_get_pc(),offset,data);
}

static READ16_HANDLER( K051937_halfword_r )
{
	return K051937_r(offset);
}

static WRITE16_HANDLER( K051937_halfword_w )
{
	if (ACCESSING_LSB)
		K051937_w(offset,data & 0xff);
}

static READ16_HANDLER( K051960_halfword_r )
{
	return K051960_r(offset);
}

static WRITE16_HANDLER( K051960_halfword_w )
{
	if (ACCESSING_LSB)
		K051960_w(offset,data & 0xff);
}



static int irqAen,irqBmask;


static MACHINE_RESET( gradius3 )
{
	/* start with cpu B halted */
	cpunum_set_input_line(1, INPUT_LINE_RESET, ASSERT_LINE);
	irqAen = 0;
	irqBmask = 0;
}

static UINT16 *sharedram;

static READ16_HANDLER( sharedram_r )
{
	return sharedram[offset];
}

static WRITE16_HANDLER( sharedram_w )
{
	COMBINE_DATA(&sharedram[offset]);
}

static WRITE16_HANDLER( cpuA_ctrl_w )
{
	if (ACCESSING_MSB)
	{
		data >>= 8;

		/* bits 0-1 are coin counters */
		coin_counter_w(0,data & 0x01);
		coin_counter_w(1,data & 0x02);

		/* bit 2 selects layer priority */
		gradius3_priority = data & 0x04;

		/* bit 3 enables cpu B */
		cpunum_set_input_line(1, INPUT_LINE_RESET, (data & 0x08) ? CLEAR_LINE : ASSERT_LINE);

		/* bit 5 enables irq */
		irqAen = data & 0x20;

		/* other bits unknown */
//logerror("%06x: write %04x to c0000\n",activecpu_get_pc(),data);
	}
}

static WRITE16_HANDLER( cpuB_irqenable_w )
{
	if (ACCESSING_MSB)
		irqBmask = (data >> 8) & 0x07;
}

static INTERRUPT_GEN( cpuA_interrupt )
{
	if (irqAen)
		cpunum_set_input_line(0, 2, HOLD_LINE);
}

static INTERRUPT_GEN( cpuB_interrupt )
{
	if (cpu_getiloops() & 1)	/* ??? */
	{
		if (irqBmask & 2)
			cpunum_set_input_line(1, 2, HOLD_LINE);
	}
	else
	{
		if (irqBmask & 1)
			cpunum_set_input_line(1, 1, HOLD_LINE);
	}
}

static WRITE16_HANDLER( cpuB_irqtrigger_w )
{
	if (irqBmask & 4)
	{
logerror("%04x trigger cpu B irq 4 %02x\n",activecpu_get_pc(),data);
		cpunum_set_input_line(1,4,HOLD_LINE);
	}
	else
logerror("%04x MISSED cpu B irq 4 %02x\n",activecpu_get_pc(),data);
}

static WRITE16_HANDLER( sound_command_w )
{
	if (ACCESSING_MSB)
		soundlatch_w(0,(data >> 8) & 0xff);
}

static WRITE16_HANDLER( sound_irq_w )
{
	cpunum_set_input_line_and_vector(2,0,HOLD_LINE,0xff);
}

static WRITE8_HANDLER( sound_bank_w )
{
	int bank_A, bank_B;

	/* banks # for the 007232 (chip 1) */
	bank_A = ((data >> 0) & 0x03);
	bank_B = ((data >> 2) & 0x03);
	K007232_set_bank( 0, bank_A, bank_B );
}



static ADDRESS_MAP_START( gradius3_readmem, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x03ffff) AM_READ(MRA16_ROM)
	AM_RANGE(0x040000, 0x043fff) AM_READ(MRA16_RAM)
	AM_RANGE(0x080000, 0x080fff) AM_READ(MRA16_RAM)
	AM_RANGE(0x0c8000, 0x0c8001) AM_READ(input_port_0_word_r)
	AM_RANGE(0x0c8002, 0x0c8003) AM_READ(input_port_1_word_r)
	AM_RANGE(0x0c8004, 0x0c8005) AM_READ(input_port_2_word_r)
	AM_RANGE(0x0c8006, 0x0c8007) AM_READ(input_port_5_word_r)
	AM_RANGE(0x0d0000, 0x0d0001) AM_READ(input_port_3_word_r)
	AM_RANGE(0x0d0002, 0x0d0003) AM_READ(input_port_4_word_r)
	AM_RANGE(0x100000, 0x103fff) AM_READ(sharedram_r)
	AM_RANGE(0x14c000, 0x153fff) AM_READ(K052109_halfword_r)
	AM_RANGE(0x180000, 0x19ffff) AM_READ(gradius3_gfxram_r)
ADDRESS_MAP_END

static ADDRESS_MAP_START( gradius3_writemem, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x03ffff) AM_WRITE(MWA16_ROM)
	AM_RANGE(0x040000, 0x043fff) AM_WRITE(MWA16_RAM)
	AM_RANGE(0x080000, 0x080fff) AM_WRITE(paletteram16_xRRRRRGGGGGBBBBB_word_w) AM_BASE(&paletteram16)
	AM_RANGE(0x0c0000, 0x0c0001) AM_WRITE(cpuA_ctrl_w)	/* halt cpu B, irq enable, priority, coin counters, other? */
	AM_RANGE(0x0d8000, 0x0d8001) AM_WRITE(cpuB_irqtrigger_w)
	AM_RANGE(0x0e0000, 0x0e0001) AM_WRITE(watchdog_reset16_w)
	AM_RANGE(0x0e8000, 0x0e8001) AM_WRITE(sound_command_w)
	AM_RANGE(0x0f0000, 0x0f0001) AM_WRITE(sound_irq_w)
	AM_RANGE(0x100000, 0x103fff) AM_WRITE(sharedram_w) AM_BASE(&sharedram)
	AM_RANGE(0x14c000, 0x153fff) AM_WRITE(K052109_halfword_w)
	AM_RANGE(0x180000, 0x19ffff) AM_WRITE(gradius3_gfxram_w) AM_BASE(&gradius3_gfxram)
ADDRESS_MAP_END


static ADDRESS_MAP_START( gradius3_readmem2, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x0fffff) AM_READ(MRA16_ROM)
	AM_RANGE(0x100000, 0x103fff) AM_READ(MRA16_RAM)
	AM_RANGE(0x200000, 0x203fff) AM_READ(sharedram_r)
	AM_RANGE(0x24c000, 0x253fff) AM_READ(K052109_halfword_r)
	AM_RANGE(0x280000, 0x29ffff) AM_READ(gradius3_gfxram_r)
	AM_RANGE(0x2c0000, 0x2c000f) AM_READ(K051937_halfword_r)
	AM_RANGE(0x2c0800, 0x2c0fff) AM_READ(K051960_halfword_r)
	AM_RANGE(0x400000, 0x5fffff) AM_READ(gradius3_gfxrom_r)		/* gfx ROMs are mapped here, and copied to RAM */
ADDRESS_MAP_END

static ADDRESS_MAP_START( gradius3_writemem2, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x0fffff) AM_WRITE(MWA16_ROM)
	AM_RANGE(0x100000, 0x103fff) AM_WRITE(MWA16_RAM)
	AM_RANGE(0x140000, 0x140001) AM_WRITE(cpuB_irqenable_w)
	AM_RANGE(0x200000, 0x203fff) AM_WRITE(sharedram_w)
	AM_RANGE(0x24c000, 0x253fff) AM_WRITE(K052109_halfword_w)
	AM_RANGE(0x280000, 0x29ffff) AM_WRITE(gradius3_gfxram_w)
	AM_RANGE(0x2c0000, 0x2c000f) AM_WRITE(K051937_halfword_w)
	AM_RANGE(0x2c0800, 0x2c0fff) AM_WRITE(K051960_halfword_w)
ADDRESS_MAP_END


static ADDRESS_MAP_START( gradius3_s_readmem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0xefff) AM_READ(MRA8_ROM)
	AM_RANGE(0xf010, 0xf010) AM_READ(soundlatch_r)
	AM_RANGE(0xf020, 0xf02d) AM_READ(K007232_read_port_0_r)
	AM_RANGE(0xf031, 0xf031) AM_READ(YM2151_status_port_0_r)
	AM_RANGE(0xf800, 0xffff) AM_READ(MRA8_RAM)
ADDRESS_MAP_END

static ADDRESS_MAP_START( gradius3_s_writemem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0xefff) AM_WRITE(MWA8_ROM)
	AM_RANGE(0xf000, 0xf000) AM_WRITE(sound_bank_w)				/* 007232 bankswitch */
	AM_RANGE(0xf020, 0xf02d) AM_WRITE(K007232_write_port_0_w)
	AM_RANGE(0xf030, 0xf030) AM_WRITE(YM2151_register_port_0_w)
	AM_RANGE(0xf031, 0xf031) AM_WRITE(YM2151_data_port_0_w)
	AM_RANGE(0xf800, 0xffff) AM_WRITE(MWA8_RAM)
ADDRESS_MAP_END



INPUT_PORTS_START( gradius3 )
	PORT_START      /* COINS */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_COIN3 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START      /* PLAYER 1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )	// power-up
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 )	// shoot
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON3 )	// missile
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START      /* PLAYER 2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY PORT_COCKTAIL
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY PORT_COCKTAIL
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY PORT_COCKTAIL
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY PORT_COCKTAIL
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_COCKTAIL
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_COCKTAIL
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_COCKTAIL
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* DSW1 */
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

	PORT_START	/* DSW2 */
	PORT_DIPNAME( 0x03, 0x02, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x03, "2" )
	PORT_DIPSETTING(    0x02, "3" )
	PORT_DIPSETTING(    0x01, "5" )
	PORT_DIPSETTING(    0x00, "7" )
	PORT_DIPNAME( 0x04, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0x18, 0x18, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x18, "20000 and every 70000" )
	PORT_DIPSETTING(    0x10, "100000 and every 100000" )
	PORT_DIPSETTING(    0x08, "50000" )
	PORT_DIPSETTING(    0x00, "100000" )
	PORT_DIPNAME( 0x60, 0x40, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x60, DEF_STR( Easy ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Normal ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Hard ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Hardest ) )
	PORT_DIPNAME( 0x80, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START	/* DSW3 */
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, "Upright Controls" )
	PORT_DIPSETTING(    0x02, DEF_STR( Single ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Dual ) )
	PORT_SERVICE( 0x04, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_BIT( 0xf0, IP_ACTIVE_LOW, IPT_UNUSED )
INPUT_PORTS_END



static void volume_callback(int v)
{
	K007232_set_volume(0,0,(v >> 4) * 0x11,0);
	K007232_set_volume(0,1,0,(v & 0x0f) * 0x11);
}

static struct K007232_interface k007232_interface =
{
	REGION_SOUND1,	/* memory regions */
	volume_callback	/* external port callback */
};



static MACHINE_DRIVER_START( gradius3 )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000, 10000000)	/* 10 MHz */
	MDRV_CPU_PROGRAM_MAP(gradius3_readmem,gradius3_writemem)
	MDRV_CPU_VBLANK_INT(cpuA_interrupt,1)

	MDRV_CPU_ADD(M68000, 10000000)	/* 10 MHz */
	MDRV_CPU_PROGRAM_MAP(gradius3_readmem2,gradius3_writemem2)
	MDRV_CPU_VBLANK_INT(cpuB_interrupt,2)	/* has three interrupt vectors, 1 2 and 4 */
								/* 4 is triggered by cpu A, the others are unknown but */
								/* required for the game to run. */
	MDRV_CPU_ADD(Z80, 3579545)
	/* audio CPU */	/* 3.579545 MHz */
	MDRV_CPU_PROGRAM_MAP(gradius3_s_readmem,gradius3_s_writemem)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)
	MDRV_INTERLEAVE(100)

	MDRV_MACHINE_RESET(gradius3)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER | VIDEO_HAS_SHADOWS)
	MDRV_SCREEN_SIZE(64*8, 32*8)
	MDRV_VISIBLE_AREA(12*8, (64-12)*8-1, 2*8, 30*8-1 )
	MDRV_PALETTE_LENGTH(2048)

	MDRV_VIDEO_START(gradius3)
	MDRV_VIDEO_UPDATE(gradius3)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_STEREO("left", "right")

	MDRV_SOUND_ADD(YM2151, 3579545)
	MDRV_SOUND_ROUTE(0, "left", 1.0)
	MDRV_SOUND_ROUTE(1, "right", 1.0)

	MDRV_SOUND_ADD(K007232, 3579545)
	MDRV_SOUND_CONFIG(k007232_interface)
	MDRV_SOUND_ROUTE(0, "left", 0.20)
	MDRV_SOUND_ROUTE(0, "right", 0.20)
	MDRV_SOUND_ROUTE(1, "left", 0.20)
	MDRV_SOUND_ROUTE(1, "right", 0.20)
MACHINE_DRIVER_END



/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START( gradius3 )
	ROM_REGION( 0x40000, REGION_CPU1, 0 )
	ROM_LOAD16_BYTE( "945_s13.f15",		0x00000, 0x20000, CRC(70c240a2) SHA1(82dc391572e1f61b0182cb031654d71adcdd5f6e) )
	ROM_LOAD16_BYTE( "945_s12.e15",		0x00001, 0x20000, CRC(bbc300d4) SHA1(e1ca98bc591575285d7bd2d4fefdf35fed10dcb6) )

	ROM_REGION( 0x100000, REGION_CPU2, 0 )
	ROM_LOAD16_BYTE( "945_m09.r17",		0x000000, 0x20000, CRC(b4a6df25) SHA1(85533cf140d28f6f81c0b49b8061bda0924a613a) )
	ROM_LOAD16_BYTE( "945_m08.n17",		0x000001, 0x20000, CRC(74e981d2) SHA1(e7b47a2da01ff73293d2100c48fdf00b33125af5) )
	ROM_LOAD16_BYTE( "945_l06b.r11",	0x040000, 0x20000, CRC(83772304) SHA1(a90c75a3de670b6ec5e0fc201876d463b4a76766) )
	ROM_LOAD16_BYTE( "945_l06a.n11",	0x040001, 0x20000, CRC(e1fd75b6) SHA1(6160d80a2f1bf550e85d6253cf521a96f5a644cc) )
	ROM_LOAD16_BYTE( "945_l07c.r15",	0x080000, 0x20000, CRC(c1e399b6) SHA1(e95bd478dd3beea0175bf9ee4cededb111c4ace1) )
	ROM_LOAD16_BYTE( "945_l07a.n15",	0x080001, 0x20000, CRC(96222d04) SHA1(b55700f683a556b0e73dbac9c7b4ce485420d21c) )
	ROM_LOAD16_BYTE( "945_l07d.r13",	0x0c0000, 0x20000, CRC(4c16d4bd) SHA1(01dcf169b78a1e495214b10181401d1920b0c924) )
	ROM_LOAD16_BYTE( "945_l07b.n13",	0x0c0001, 0x20000, CRC(5e209d01) SHA1(0efa1bbfdc7e2ba1e0bb96245e2bfe961258b446) )

	ROM_REGION( 0x10000, REGION_CPU3, 0 )	/* 64k for the audio CPU */
	ROM_LOAD( "945_m05.d9",				0x00000, 0x10000, CRC(c8c45365) SHA1(b9a7b736b52bca42c7b8c8ed64c8df73e0116158) )

	ROM_REGION( 0x20000, REGION_GFX1, ROMREGION_ERASE00 )	/* fake */
	/* gfx data is dynamically generated in RAM */

	ROM_REGION( 0x200000, REGION_GFX2, 0 )	/* graphics (addressable by the main CPU) */
	ROM_LOAD( "945_a02.l3",				0x000000, 0x80000, CRC(4dfffd74) SHA1(588210bac27448240ef08961f70b714b69cb3ffd) )
	ROM_LOAD16_BYTE( "945_l04a.k6",		0x080000, 0x20000, CRC(884e21ee) SHA1(ce86dd3a06775e5b1aa09db010dcb674e67828e7) )
	ROM_LOAD16_BYTE( "945_l04c.m6",		0x080001, 0x20000, CRC(45bcd921) SHA1(e51a8a71362a6fb55124aa1dce74519c0a3c6e3f) )
	ROM_LOAD16_BYTE( "945_l04b.k8",		0x0c0000, 0x20000, CRC(843bc67d) SHA1(cdf8421083f24ab27867ed5d08d8949da192b2b9) )
	ROM_LOAD16_BYTE( "945_l04d.m8",		0x0c0001, 0x20000, CRC(0a98d08e) SHA1(1e0ca51a2d45c01fa3f11950ddd387f41ddae691) )
	ROM_LOAD( "945_a01.h3",				0x100000, 0x80000, CRC(339d6dd2) SHA1(6a52b826aba92c75fc6a5926184948735dc20812) )
	ROM_LOAD16_BYTE( "945_l03a.e6",		0x180000, 0x20000, CRC(a67ef087) SHA1(fd63474f3bbde5dfc53ed4c1db25d6411a8b54d2) )
	ROM_LOAD16_BYTE( "945_l03c.h6",		0x180001, 0x20000, CRC(a56be17a) SHA1(1d387736144c30fcb5de54235331ab1ff70c356e) )
	ROM_LOAD16_BYTE( "945_l03b.e8",		0x1c0000, 0x20000, CRC(933e68b9) SHA1(f3a39446ca77d17fdbd938bd5f718ae9d5570879) )
	ROM_LOAD16_BYTE( "945_l03d.h8",		0x1c0001, 0x20000, CRC(f375e87b) SHA1(6427b966795c907c8e516244872fe52217da62c4) )

	ROM_REGION( 0x0100, REGION_PROMS, 0 )
	ROM_LOAD( "945l14.j28",				0x0000, 0x0100, CRC(c778c189) SHA1(847eaf379ba075c25911c6f83dd63ff390534f60) )	/* priority encoder (not used) */

	ROM_REGION( 0x80000, REGION_SOUND1, 0 )	/* 007232 samples */
	ROM_LOAD( "945_a10.b15",			0x00000, 0x40000, CRC(1d083e10) SHA1(b116f133a7647ef7a6c373aff00e9622d9954b61) )
	ROM_LOAD( "945_l11a.c18",			0x40000, 0x20000, CRC(6043f4eb) SHA1(1c2e9ace1cfdde504b7b6158e3c3f54dc5ae33d4) )
	ROM_LOAD( "945_l11b.c20",			0x60000, 0x20000, CRC(89ea3baf) SHA1(8edcbaa7969185cfac48c02559826d1b8b081f3f) )
ROM_END

ROM_START( grdius3a )
	ROM_REGION( 0x40000, REGION_CPU1, 0 )
	ROM_LOAD16_BYTE( "945_13.f15",		0x00000, 0x20000, CRC(9974fe6b) SHA1(c18ad8d7c93bf58d886715d8e210177cf49f220b) )
	ROM_LOAD16_BYTE( "945_12.e15",		0x00001, 0x20000, CRC(e9771b91) SHA1(c9f4610b897c13742b44b546e2bed8ee21945f61) )

	ROM_REGION( 0x100000, REGION_CPU2, 0 )
	ROM_LOAD16_BYTE( "945_m09.r17",		0x000000, 0x20000, CRC(b4a6df25) SHA1(85533cf140d28f6f81c0b49b8061bda0924a613a) )
	ROM_LOAD16_BYTE( "945_m08.n17",		0x000001, 0x20000, CRC(74e981d2) SHA1(e7b47a2da01ff73293d2100c48fdf00b33125af5) )
	ROM_LOAD16_BYTE( "945_l06b.r11",	0x040000, 0x20000, CRC(83772304) SHA1(a90c75a3de670b6ec5e0fc201876d463b4a76766) )
	ROM_LOAD16_BYTE( "945_l06a.n11",	0x040001, 0x20000, CRC(e1fd75b6) SHA1(6160d80a2f1bf550e85d6253cf521a96f5a644cc) )
	ROM_LOAD16_BYTE( "945_l07c.r15",	0x080000, 0x20000, CRC(c1e399b6) SHA1(e95bd478dd3beea0175bf9ee4cededb111c4ace1) )
	ROM_LOAD16_BYTE( "945_l07a.n15",	0x080001, 0x20000, CRC(96222d04) SHA1(b55700f683a556b0e73dbac9c7b4ce485420d21c) )
	ROM_LOAD16_BYTE( "945_l07d.r13",	0x0c0000, 0x20000, CRC(4c16d4bd) SHA1(01dcf169b78a1e495214b10181401d1920b0c924) )
	ROM_LOAD16_BYTE( "945_l07b.n13",	0x0c0001, 0x20000, CRC(5e209d01) SHA1(0efa1bbfdc7e2ba1e0bb96245e2bfe961258b446) )

	ROM_REGION( 0x10000, REGION_CPU3, 0 )	/* 64k for the audio CPU */
	ROM_LOAD( "945_m05.d9",				0x00000, 0x10000, CRC(c8c45365) SHA1(b9a7b736b52bca42c7b8c8ed64c8df73e0116158) )

	ROM_REGION( 0x20000, REGION_GFX1, ROMREGION_ERASE00 )	/* fake */
	/* gfx data is dynamically generated in RAM */

	ROM_REGION( 0x200000, REGION_GFX2, 0 )	/* graphics (addressable by the main CPU) */
	ROM_LOAD( "945_a02.l3",				0x000000, 0x80000, CRC(4dfffd74) SHA1(588210bac27448240ef08961f70b714b69cb3ffd) )
	ROM_LOAD16_BYTE( "945_l04a.k6",		0x080000, 0x20000, CRC(884e21ee) SHA1(ce86dd3a06775e5b1aa09db010dcb674e67828e7) )
	ROM_LOAD16_BYTE( "945_l04c.m6",		0x080001, 0x20000, CRC(45bcd921) SHA1(e51a8a71362a6fb55124aa1dce74519c0a3c6e3f) )
	ROM_LOAD16_BYTE( "945_l04b.k8",		0x0c0000, 0x20000, CRC(843bc67d) SHA1(cdf8421083f24ab27867ed5d08d8949da192b2b9) )
	ROM_LOAD16_BYTE( "945_l04d.m8",		0x0c0001, 0x20000, CRC(0a98d08e) SHA1(1e0ca51a2d45c01fa3f11950ddd387f41ddae691) )
	ROM_LOAD( "945_a01.h3",				0x100000, 0x80000, CRC(339d6dd2) SHA1(6a52b826aba92c75fc6a5926184948735dc20812) )
	ROM_LOAD16_BYTE( "945_l03a.e6",		0x180000, 0x20000, CRC(a67ef087) SHA1(fd63474f3bbde5dfc53ed4c1db25d6411a8b54d2) )
	ROM_LOAD16_BYTE( "945_l03c.h6",		0x180001, 0x20000, CRC(a56be17a) SHA1(1d387736144c30fcb5de54235331ab1ff70c356e) )
	ROM_LOAD16_BYTE( "945_l03b.e8",		0x1c0000, 0x20000, CRC(933e68b9) SHA1(f3a39446ca77d17fdbd938bd5f718ae9d5570879) )
	ROM_LOAD16_BYTE( "945_l03d.h8",		0x1c0001, 0x20000, CRC(f375e87b) SHA1(6427b966795c907c8e516244872fe52217da62c4) )

	ROM_REGION( 0x0100, REGION_PROMS, 0 )
	ROM_LOAD( "945l14.j28",				0x0000, 0x0100, CRC(c778c189) SHA1(847eaf379ba075c25911c6f83dd63ff390534f60) )	/* priority encoder (not used) */

	ROM_REGION( 0x80000, REGION_SOUND1, 0 )	/* 007232 samples */
	ROM_LOAD( "945_a10.b15",			0x00000, 0x40000, CRC(1d083e10) SHA1(b116f133a7647ef7a6c373aff00e9622d9954b61) )
	ROM_LOAD( "945_l11a.c18",			0x40000, 0x20000, CRC(6043f4eb) SHA1(1c2e9ace1cfdde504b7b6158e3c3f54dc5ae33d4) )
	ROM_LOAD( "945_l11b.c20",			0x60000, 0x20000, CRC(89ea3baf) SHA1(8edcbaa7969185cfac48c02559826d1b8b081f3f) )
ROM_END

ROM_START( grdius3e )
	ROM_REGION( 0x40000, REGION_CPU1, 0 )
	ROM_LOAD16_BYTE( "945_r13.f15",		0x00000, 0x20000, CRC(cffd103f) SHA1(6bd15e8c2e6e5223d7de9b0b375f36f3e81f60ba) )
	ROM_LOAD16_BYTE( "945_r12.e15",		0x00001, 0x20000, CRC(0b968ef6) SHA1(ba28d16d94b13aac791b11d3d91df26f78e2e477) )

	ROM_REGION( 0x100000, REGION_CPU2, 0 )
	ROM_LOAD16_BYTE( "945_m09.r17",		0x000000, 0x20000, CRC(b4a6df25) SHA1(85533cf140d28f6f81c0b49b8061bda0924a613a) )
	ROM_LOAD16_BYTE( "945_m08.n17",		0x000001, 0x20000, CRC(74e981d2) SHA1(e7b47a2da01ff73293d2100c48fdf00b33125af5) )
	ROM_LOAD16_BYTE( "945_l06b.r11",	0x040000, 0x20000, CRC(83772304) SHA1(a90c75a3de670b6ec5e0fc201876d463b4a76766) )
	ROM_LOAD16_BYTE( "945_l06a.n11",	0x040001, 0x20000, CRC(e1fd75b6) SHA1(6160d80a2f1bf550e85d6253cf521a96f5a644cc) )
	ROM_LOAD16_BYTE( "945_l07c.r15",	0x080000, 0x20000, CRC(c1e399b6) SHA1(e95bd478dd3beea0175bf9ee4cededb111c4ace1) )
	ROM_LOAD16_BYTE( "945_l07a.n15",	0x080001, 0x20000, CRC(96222d04) SHA1(b55700f683a556b0e73dbac9c7b4ce485420d21c) )
	ROM_LOAD16_BYTE( "945_l07d.r13",	0x0c0000, 0x20000, CRC(4c16d4bd) SHA1(01dcf169b78a1e495214b10181401d1920b0c924) )
	ROM_LOAD16_BYTE( "945_l07b.n13",	0x0c0001, 0x20000, CRC(5e209d01) SHA1(0efa1bbfdc7e2ba1e0bb96245e2bfe961258b446) )

	ROM_REGION( 0x10000, REGION_CPU3, 0 )	/* 64k for the audio CPU */
	ROM_LOAD( "945_m05.d9",				0x00000, 0x10000, CRC(c8c45365) SHA1(b9a7b736b52bca42c7b8c8ed64c8df73e0116158) ) /* 945_r05.d9 */

	ROM_REGION( 0x20000, REGION_GFX1, ROMREGION_ERASE00 )	/* fake */
	/* gfx data is dynamically generated in RAM */

	ROM_REGION( 0x200000, REGION_GFX2, 0 )	/* graphics (addressable by the main CPU) */
	ROM_LOAD( "945_a02.l3",				0x000000, 0x80000, CRC(4dfffd74) SHA1(588210bac27448240ef08961f70b714b69cb3ffd) )
	ROM_LOAD16_BYTE( "945_l04a.k6",		0x080000, 0x20000, CRC(884e21ee) SHA1(ce86dd3a06775e5b1aa09db010dcb674e67828e7) )
	ROM_LOAD16_BYTE( "945_l04c.m6",		0x080001, 0x20000, CRC(45bcd921) SHA1(e51a8a71362a6fb55124aa1dce74519c0a3c6e3f) )
	ROM_LOAD16_BYTE( "945_l04b.k8",		0x0c0000, 0x20000, CRC(843bc67d) SHA1(cdf8421083f24ab27867ed5d08d8949da192b2b9) )
	ROM_LOAD16_BYTE( "945_l04d.m8",		0x0c0001, 0x20000, CRC(0a98d08e) SHA1(1e0ca51a2d45c01fa3f11950ddd387f41ddae691) )
	ROM_LOAD( "945_a01.h3",				0x100000, 0x80000, CRC(339d6dd2) SHA1(6a52b826aba92c75fc6a5926184948735dc20812) )
	ROM_LOAD16_BYTE( "945_l03a.e6",		0x180000, 0x20000, CRC(a67ef087) SHA1(fd63474f3bbde5dfc53ed4c1db25d6411a8b54d2) )
	ROM_LOAD16_BYTE( "945_l03c.h6",		0x180001, 0x20000, CRC(a56be17a) SHA1(1d387736144c30fcb5de54235331ab1ff70c356e) )
	ROM_LOAD16_BYTE( "945_l03b.e8",		0x1c0000, 0x20000, CRC(933e68b9) SHA1(f3a39446ca77d17fdbd938bd5f718ae9d5570879) )
	ROM_LOAD16_BYTE( "945_l03d.h8",		0x1c0001, 0x20000, CRC(f375e87b) SHA1(6427b966795c907c8e516244872fe52217da62c4) )

	ROM_REGION( 0x0100, REGION_PROMS, 0 )
	ROM_LOAD( "945l14.j28",				0x0000, 0x0100, CRC(c778c189) SHA1(847eaf379ba075c25911c6f83dd63ff390534f60) )	/* priority encoder (not used) */

	ROM_REGION( 0x80000, REGION_SOUND1, 0 )	/* 007232 samples */
	ROM_LOAD( "945_a10.b15",			0x00000, 0x40000, CRC(1d083e10) SHA1(b116f133a7647ef7a6c373aff00e9622d9954b61) )
	ROM_LOAD( "945_l11a.c18",			0x40000, 0x20000, CRC(6043f4eb) SHA1(1c2e9ace1cfdde504b7b6158e3c3f54dc5ae33d4) )
	ROM_LOAD( "945_l11b.c20",			0x60000, 0x20000, CRC(89ea3baf) SHA1(8edcbaa7969185cfac48c02559826d1b8b081f3f) )
ROM_END


static DRIVER_INIT( gradius3 )
{
	konami_rom_deinterleave_2(REGION_GFX2);
}



GAME( 1989, gradius3, 0,        gradius3, gradius3, gradius3, ROT0, "Konami", "Gradius III (Japan)", 0 )
GAME( 1989, grdius3a, gradius3, gradius3, gradius3, gradius3, ROT0, "Konami", "Gradius III (Asia)", 0 )
GAME( 1989, grdius3e, gradius3, gradius3, gradius3, gradius3, ROT0, "Konami", "Gradius III (World ?)", 0 )
