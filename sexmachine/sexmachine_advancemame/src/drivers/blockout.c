/***************************************************************************

Block Out

driver by Nicola Salmoria

***************************************************************************/

#include "driver.h"
#include "cpu/z80/z80.h"
#include "sound/2151intf.h"
#include "sound/okim6295.h"


extern UINT16 *blockout_videoram;
extern UINT16 *blockout_frontvideoram;
extern unsigned char *blockout_frontcolor;

WRITE16_HANDLER( blockout_videoram_w );
WRITE16_HANDLER( blockout_paletteram_w );
WRITE16_HANDLER( blockout_frontcolor_w );
VIDEO_START( blockout );
VIDEO_UPDATE( blockout );


static INTERRUPT_GEN( blockout_interrupt )
{
	/* interrupt 6 is vblank */
	/* interrupt 5 reads coin inputs - might have to be triggered only */
	/* when a coin is inserted */
	cpunum_set_input_line(0, 6 - cpu_getiloops(), HOLD_LINE);
}

static WRITE16_HANDLER( blockout_sound_command_w )
{
	if (ACCESSING_LSB)
	{
		soundlatch_w(offset,data & 0xff);
		cpunum_set_input_line(1, INPUT_LINE_NMI, PULSE_LINE);
	}
}


static ADDRESS_MAP_START( readmem, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x03ffff) AM_READ(MRA16_ROM)
	AM_RANGE(0x100000, 0x100001) AM_READ(input_port_0_word_r)
	AM_RANGE(0x100002, 0x100003) AM_READ(input_port_1_word_r)
	AM_RANGE(0x100004, 0x100005) AM_READ(input_port_2_word_r)
	AM_RANGE(0x100006, 0x100007) AM_READ(input_port_3_word_r)
	AM_RANGE(0x100008, 0x100009) AM_READ(input_port_4_word_r)
	AM_RANGE(0x180000, 0x1bffff) AM_READ(MRA16_RAM)
	AM_RANGE(0x1d4000, 0x1dffff) AM_READ(MRA16_RAM)
	AM_RANGE(0x1f4000, 0x1fffff) AM_READ(MRA16_RAM)
	AM_RANGE(0x200000, 0x207fff) AM_READ(MRA16_RAM)
	AM_RANGE(0x208000, 0x21ffff) AM_READ(MRA16_RAM)
	AM_RANGE(0x280200, 0x2805ff) AM_READ(MRA16_RAM)
ADDRESS_MAP_END

static ADDRESS_MAP_START( writemem, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x03ffff) AM_WRITE(MWA16_ROM)
	AM_RANGE(0x100014, 0x100015) AM_WRITE(blockout_sound_command_w)
	AM_RANGE(0x100016, 0x100017) AM_WRITE(MWA16_NOP)	/* don't know, maybe reset sound CPU */
	AM_RANGE(0x180000, 0x1bffff) AM_WRITE(blockout_videoram_w) AM_BASE(&blockout_videoram)
	AM_RANGE(0x1d4000, 0x1dffff) AM_WRITE(MWA16_RAM)	/* work RAM */
	AM_RANGE(0x1f4000, 0x1fffff) AM_WRITE(MWA16_RAM)	/* work RAM */
	AM_RANGE(0x200000, 0x207fff) AM_WRITE(MWA16_RAM) AM_BASE(&blockout_frontvideoram)
	AM_RANGE(0x208000, 0x21ffff) AM_WRITE(MWA16_RAM)	/* ??? */
	AM_RANGE(0x280002, 0x280003) AM_WRITE(blockout_frontcolor_w)
	AM_RANGE(0x280200, 0x2805ff) AM_WRITE(blockout_paletteram_w) AM_BASE(&paletteram16)
ADDRESS_MAP_END

static ADDRESS_MAP_START( sound_readmem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x7fff) AM_READ(MRA8_ROM)
	AM_RANGE(0x8000, 0x87ff) AM_READ(MRA8_RAM)
	AM_RANGE(0x8801, 0x8801) AM_READ(YM2151_status_port_0_r)
	AM_RANGE(0x9800, 0x9800) AM_READ(OKIM6295_status_0_r)
	AM_RANGE(0xa000, 0xa000) AM_READ(soundlatch_r)
ADDRESS_MAP_END

static ADDRESS_MAP_START( sound_writemem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x7fff) AM_WRITE(MWA8_ROM)
	AM_RANGE(0x8000, 0x87ff) AM_WRITE(MWA8_RAM)
	AM_RANGE(0x8800, 0x8800) AM_WRITE(YM2151_register_port_0_w)
	AM_RANGE(0x8801, 0x8801) AM_WRITE(YM2151_data_port_0_w)
	AM_RANGE(0x9800, 0x9800) AM_WRITE(OKIM6295_data_0_w)
ADDRESS_MAP_END


INPUT_PORTS_START( blockout )
	PORT_START      /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON4 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON3 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START1 )

	PORT_START      /* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON4 ) PORT_PLAYER(2)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(2)
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_PLAYER(2)
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START2 )

	PORT_START
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_COIN3 )

	PORT_START
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 1C_2C ) )
	/* the following two are supposed to control Coin 2, but they don't work. */
	/* This happens on the original board too. */
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unknown ) )	/* unused? */
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unknown ) )	/* unused? */
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, "1 Coin to Continue" )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x20, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unknown ) )	/* unused? */
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )	/* unused? */
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Easy ) )
	PORT_DIPSETTING(    0x03, DEF_STR( Normal ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Hard ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Very_Hard ) )
	PORT_DIPNAME( 0x04, 0x04, "Rotate Buttons" )
	PORT_DIPSETTING(    0x00, "2" )
	PORT_DIPSETTING(    0x04, "3" )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(2)
INPUT_PORTS_END

INPUT_PORTS_START( blckoutj )
	PORT_START      /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON3 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START1 )

	PORT_START      /* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(2)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(2)
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_PLAYER(2)
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START2 )

	PORT_START
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_COIN3 )

	PORT_START
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 1C_2C ) )
	/* the following two are supposed to control Coin 2, but they don't work. */
	/* This happens on the original board too. */
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unknown ) )	/* unused? */
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unknown ) )	/* unused? */
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, "1 Coin to Continue" )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x20, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unknown ) )	/* unused? */
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )	/* unused? */
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Easy ) )
	PORT_DIPSETTING(    0x03, DEF_STR( Normal ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Hard ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Very_Hard ) )
	/* The following switch is 2/3 rotate buttons on the other sets, option doesn't exist in the Japan version */
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
	/* these can still be used on the difficutly select even if they can't be used for rotating pieces in this version */
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON4 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON4 ) PORT_PLAYER(2)
INPUT_PORTS_END

INPUT_PORTS_START( agress )
	PORT_START      /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON4 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON3 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START1 )

	PORT_START      /* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON4 ) PORT_PLAYER(2)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(2)
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_PLAYER(2)
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START2 )

	PORT_START
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_COIN3 )

	/* factory shipment setting is all dips OFF */

	PORT_START	/* DSW 1 */
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 1C_2C ) )
	PORT_DIPNAME( 0x04, 0x04, "Opening Cut" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x04, DEF_STR( On ) )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x20, DEF_STR( On ) )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* DSW 2 */
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Easiest ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Easy ) )
	PORT_DIPSETTING(    0x03, DEF_STR( Normal ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Hard ) )
	/* Engrish here. The manual says "Number of Prayers". Maybe related to lives? */
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Players ) )
	PORT_DIPSETTING(    0x04, "1" )
	PORT_DIPSETTING(    0x00, "2" )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNUSED )
INPUT_PORTS_END

/* handler called by the 2151 emulator when the internal timers cause an IRQ */
static void blockout_irq_handler(int irq)
{
	cpunum_set_input_line_and_vector(1,0,irq ? ASSERT_LINE : CLEAR_LINE,0xff);
}

static struct YM2151interface ym2151_interface =
{
	blockout_irq_handler
};


static MACHINE_DRIVER_START( blockout )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000, 10000000)       /* MRH - 8.76 makes gfx/adpcm samples sync better -- but 10 is correct speed*/
	MDRV_CPU_PROGRAM_MAP(readmem,writemem)
	MDRV_CPU_VBLANK_INT(blockout_interrupt,2)

	MDRV_CPU_ADD(Z80, 3579545)
	/* audio CPU */	/* 3.579545 MHz */
	MDRV_CPU_PROGRAM_MAP(sound_readmem,sound_writemem)

	MDRV_FRAMES_PER_SECOND(58)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(320, 256)
	MDRV_VISIBLE_AREA(0, 319, 8, 247)
	MDRV_PALETTE_LENGTH(513)

	MDRV_VIDEO_START(blockout)
	MDRV_VIDEO_UPDATE(blockout)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_STEREO("left", "right")

	MDRV_SOUND_ADD(YM2151, 3579545)
	MDRV_SOUND_CONFIG(ym2151_interface)
	MDRV_SOUND_ROUTE(0, "left", 0.60)
	MDRV_SOUND_ROUTE(1, "right", 0.60)

	MDRV_SOUND_ADD(OKIM6295, 1056000 / 132)
	MDRV_SOUND_CONFIG(okim6295_interface_region_1)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "left", 0.50)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "right", 0.50)
MACHINE_DRIVER_END



/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START( blockout )
	ROM_REGION( 0x40000, REGION_CPU1, 0 )	/* 2*128k for 68000 code */
	ROM_LOAD16_BYTE( "bo29a0-2.bin", 0x00000, 0x20000, CRC(b0103427) SHA1(53cac2adc04783abbde21e9f3c0e655f22f68f69) )
	ROM_LOAD16_BYTE( "bo29a1-2.bin", 0x00001, 0x20000, CRC(5984d5a2) SHA1(4b350856d0313d40eaa3d8a8d9e310f74bc20398) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* 64k for the audio CPU */
	ROM_LOAD( "bo29e3-0.bin", 0x0000, 0x8000, CRC(3ea01f78) SHA1(5fc4ad4d9f03d7c26d2afc3e7ede75589e40b0d8) )

	ROM_REGION( 0x20000, REGION_SOUND1, 0 )	/* 128k for ADPCM samples - sound chip is OKIM6295 */
	ROM_LOAD( "bo29e2-0.bin", 0x0000, 0x20000, CRC(15c5a99d) SHA1(89091eda454a028fd1f17501584bd589baf6d523) )

	ROM_REGION( 0x0100, REGION_PROMS, 0 )
	ROM_LOAD( "mb7114h.25",   0x0000, 0x0100, CRC(b25bbda7) SHA1(840f1470886bd0019db3cd29e3d1d80205a65f48) )	/* unknown */
ROM_END

ROM_START( blckout2 )
	ROM_REGION( 0x40000, REGION_CPU1, 0 )	/* 2*128k for 68000 code */
	ROM_LOAD16_BYTE( "29a0",         0x00000, 0x20000, CRC(605f931e) SHA1(65fa7227dafde1fc8564e09fa949fe575b394d8a) )
	ROM_LOAD16_BYTE( "29a1",         0x00001, 0x20000, CRC(38f07000) SHA1(e4070e3067d77cc1b0d8d0c63786f2729c5c703a) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* 64k for the audio CPU */
	ROM_LOAD( "bo29e3-0.bin", 0x0000, 0x8000, CRC(3ea01f78) SHA1(5fc4ad4d9f03d7c26d2afc3e7ede75589e40b0d8) )

	ROM_REGION( 0x20000, REGION_SOUND1, 0 )	/* 128k for ADPCM samples - sound chip is OKIM6295 */
	ROM_LOAD( "bo29e2-0.bin", 0x0000, 0x20000, CRC(15c5a99d) SHA1(89091eda454a028fd1f17501584bd589baf6d523) )

	ROM_REGION( 0x0100, REGION_PROMS, 0 )
	ROM_LOAD( "mb7114h.25",   0x0000, 0x0100, CRC(b25bbda7) SHA1(840f1470886bd0019db3cd29e3d1d80205a65f48) )	/* unknown */
ROM_END

ROM_START( blckoutj )
	ROM_REGION( 0x40000, REGION_CPU1, 0 )	/* 2*128k for 68000 code */
	ROM_LOAD16_BYTE( "2.bin",         0x00000, 0x20000, CRC(e16cf065) SHA1(541b30b054cf08f10d6ca4746423759f4326c005) )
	ROM_LOAD16_BYTE( "1.bin",         0x00001, 0x20000, CRC(950b28a3) SHA1(7d1635ac2a3fc1efdd2f78cd6038bd7b4c907b1b) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* 64k for the audio CPU */
	ROM_LOAD( "bo29e3-0.bin", 0x0000, 0x8000, CRC(3ea01f78) SHA1(5fc4ad4d9f03d7c26d2afc3e7ede75589e40b0d8) )

	ROM_REGION( 0x20000, REGION_SOUND1, 0 )	/* 128k for ADPCM samples - sound chip is OKIM6295 */
	ROM_LOAD( "bo29e2-0.bin", 0x0000, 0x20000, CRC(15c5a99d) SHA1(89091eda454a028fd1f17501584bd589baf6d523) )

	ROM_REGION( 0x0100, REGION_PROMS, 0 )
	ROM_LOAD( "mb7114h.25",   0x0000, 0x0100, CRC(b25bbda7) SHA1(840f1470886bd0019db3cd29e3d1d80205a65f48) )	/* unknown */
ROM_END

/*

Agress
Palco System Corp., 1991

This game runs on an original unmodified Technos 'Block Out' PCB.
All of the Technos identifications are hidden under 'Palco' or 'Agress' stickers.

PCB Layout (Applies to both Agress and Block Out)
----------

PS-05307 (sticker)
TA-0029-P1-02 (printed on Block Out PCB under the sticker)
|--------------------------------------------------------|
| M51516            YM2151                    3.579545MHz|
|           YM3012     6116                              |
|   MB3615  1.056MHz   PALCO3.73            20MHz        |
|           M6295                                        |
|   MB3615                            82S129PR.25  28MHz |
|           PALCO4.78                                    |
|                      Z80            |-------|          |
|                                     |TECHNOS|          |
|J         2018     6264              |TJ-001 |          |
|A                                    |(QFP80)|          |
|M         2018                       |-------|          |
|M                                                       |
|A                  6264                                 |
|                               MB81461-12               |
|                               MB81461-12               |
|                               MB81461-12               |
|                               MB81461-12               |
|                               MB81461-12               |
|  DSW1(8)                      MB81461-12               |
|                               MB81461-12               |
|  DSW2(8)                      MB81461-12               |
|         PALCO2.91                                      |
|                 PALCO1.81               68000          |
|--------------------------------------------------------|
Notes:
      68000 clock : 10.000MHz
      Z80 clock   : 3.579545MHz
      M6295 clock : 1.056MHz, sample rate = 1056000 / 132
      YM2151 clock: 3.579545MHz
      VSync       : 58Hz

      PROM is used for video timing etc, without it, no graphics are displayed,
      only 'Insert Coin' and the manufacturer text/year on the title screen.

      palco1.81 \ Main program   27C010
      palco2.91 /                  "
      palco3.73   OKI samples    27C256
      palco4.78   Z80 program    27C010

*/

ROM_START( agress )
	ROM_REGION( 0x40000, REGION_CPU1, 0 )	/* 2*128k for 68000 code */
	ROM_LOAD16_BYTE( "palco1.81",         0x00000, 0x20000, CRC(3acc917a) SHA1(14960588673458d862daf14a8d7474af6c95c2ad) )
	ROM_LOAD16_BYTE( "palco2.91",         0x00001, 0x20000, CRC(abfd5bcc) SHA1(bf0ea8ba00750ea2ddf2b8afc96393bf8a730068) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* 64k for the audio CPU */
	ROM_LOAD( "palco3.73", 0x0000, 0x8000, CRC(2a21c97d) SHA1(7f71bf18db3e6ff9c69c589268450e66c6585cdd) )

	ROM_REGION( 0x20000, REGION_SOUND1, 0 )	/* 128k for ADPCM samples - sound chip is OKIM6295 */
	ROM_LOAD( "palco4.78", 0x0000, 0x20000, CRC(9dfd9cfe) SHA1(5ea8f98bc0cd117cde81c04f02aa33199afe8231) )

	ROM_REGION( 0x0100, REGION_PROMS, 0 )
	ROM_LOAD( "82s129pr.25",   0x0000, 0x0100, CRC(b25bbda7) SHA1(840f1470886bd0019db3cd29e3d1d80205a65f48) )	/* unknown */
ROM_END

// this is probably an original English version with copyright year hacked
ROM_START( agressb )
	ROM_REGION( 0x40000, REGION_CPU1, 0 )	/* 2*128k for 68000 code */
	ROM_LOAD16_BYTE( "palco1.ic81",  0x00000, 0x20000, CRC(a1875175) SHA1(6c9946bcd4fe7987d4f817ea25bfc76432188883) )
	ROM_LOAD16_BYTE( "palco2.ic91",  0x00001, 0x20000, CRC(ab3182c3) SHA1(788a3e7cf6ef889262f3d72af8be9ec951eb397b) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* 64k for the audio CPU */
	ROM_LOAD( "palco3.ic73",  0x000000, 0x08000, CRC(2a21c97d) SHA1(7f71bf18db3e6ff9c69c589268450e66c6585cdd) )

	ROM_REGION( 0x20000, REGION_SOUND1, 0 )	/* 128k for ADPCM samples - sound chip is OKIM6295 */
	ROM_LOAD( "palco4.ic78",  0x000000, 0x20000, CRC(9dfd9cfe) SHA1(5ea8f98bc0cd117cde81c04f02aa33199afe8231) )

	ROM_REGION( 0x0100, REGION_PROMS, 0 )
	ROM_LOAD( "prom29-mb7114h.ic25", 0x000000, 0x0100, CRC(b25bbda7) SHA1(840f1470886bd0019db3cd29e3d1d80205a65f48) )
ROM_END


GAME( 1989, blockout, 0,        blockout, blockout, 0, ROT0, "Technos + California Dreams", "Block Out (set 1)", 0 )
GAME( 1989, blckout2, blockout, blockout, blockout, 0, ROT0, "Technos + California Dreams", "Block Out (set 2)", 0 )
GAME( 1989, blckoutj, blockout, blockout, blckoutj, 0, ROT0, "Technos + California Dreams", "Block Out (Japan)", 0 )
GAME( 1991, agress,   0,        blockout, agress,   0, ROT0, "Palco", "Agress", GAME_IMPERFECT_GRAPHICS )
GAME( 2003, agressb,  agress,   blockout, agress,   0, ROT0, "Palco", "Agress (English bootleg)", GAME_IMPERFECT_GRAPHICS )
