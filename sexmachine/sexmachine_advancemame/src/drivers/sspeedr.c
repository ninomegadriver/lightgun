/***************************************************************************

Taito Super Speed Race driver

***************************************************************************/

#include "driver.h"
#include "artwork.h"

extern WRITE8_HANDLER( sspeedr_driver_horz_w );
extern WRITE8_HANDLER( sspeedr_driver_horz_2_w );
extern WRITE8_HANDLER( sspeedr_driver_vert_w );
extern WRITE8_HANDLER( sspeedr_driver_pic_w );

extern WRITE8_HANDLER( sspeedr_drones_horz_w );
extern WRITE8_HANDLER( sspeedr_drones_horz_2_w );
extern WRITE8_HANDLER( sspeedr_drones_vert_w );
extern WRITE8_HANDLER( sspeedr_drones_mask_w );

extern WRITE8_HANDLER( sspeedr_track_horz_w );
extern WRITE8_HANDLER( sspeedr_track_horz_2_w );
extern WRITE8_HANDLER( sspeedr_track_vert_w );
extern WRITE8_HANDLER( sspeedr_track_ice_w );

extern VIDEO_START( sspeedr );
extern VIDEO_UPDATE( sspeedr );
extern VIDEO_EOF( sspeedr );

static UINT8 led_TIME[2];
static UINT8 led_SCORE[24];


static PALETTE_INIT( sspeedr )
{
	int i;

	for (i = 0; i < 16; i++)
	{
		int r = (i & 1) ? 0xb0 : 0x20;
		int g = (i & 2) ? 0xb0 : 0x20;
		int b = (i & 4) ? 0xb0 : 0x20;

		if (i & 8)
		{
			r += 0x4f;
			g += 0x4f;
			b += 0x4f;
		}

		palette_set_color(i, r, g, b);
	}
}


static READ8_HANDLER( sspeedr_steering_r )
{
	UINT8 val = readinputport(0);

	return 0x3f ^ (val >> 2) ^ (val >> 3);
}


static WRITE8_HANDLER( sspeedr_int_ack_w )
{
	cpunum_set_input_line(0, 0, CLEAR_LINE);
}


static WRITE8_HANDLER( sspeedr_lamp_w )
{
	artwork_show("lampGO",
		data & 1);
	artwork_show("lampEP",
		data & 2);

	coin_counter_w(0, data & 8);
}


static WRITE8_HANDLER( sspeedr_time_w )
{
	UINT8 prev = led_TIME[offset];

	char buf_old[8];
	char buf_new[8];

	data = data & 15;

	sprintf(buf_old, "LEDT%d-%c", offset, prev >= 10 ? 'X' : '0' + prev);
	sprintf(buf_new, "LEDT%d-%c", offset, data >= 10 ? 'X' : '0' + data);

	artwork_show(buf_old, 0);
	artwork_show(buf_new, 1);

	led_TIME[offset] = data;
}


static WRITE8_HANDLER( sspeedr_score_w )
{
	UINT8 prev = led_SCORE[offset];

	char buf_old[8];
	char buf_new[8];

	data = ~data & 15;

	sprintf(buf_old, "LED%02d-%c", offset, prev >= 10 ? 'X' : '0' + prev);
	sprintf(buf_new, "LED%02d-%c", offset, data >= 10 ? 'X' : '0' + data);

	artwork_show(buf_old, 0);
	artwork_show(buf_new, 1);

	led_SCORE[offset] = data;
}


static WRITE8_HANDLER( sspeedr_sound_w )
{
	/* not implemented */
}


static ADDRESS_MAP_START( readmem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x0fff) AM_READ(MRA8_ROM)
	AM_RANGE(0x2000, 0x21ff) AM_READ(MRA8_RAM)
ADDRESS_MAP_END


static ADDRESS_MAP_START( writemem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x0fff) AM_WRITE(MWA8_ROM)
	AM_RANGE(0x2000, 0x21ff) AM_WRITE(MWA8_RAM)
	AM_RANGE(0x7f00, 0x7f17) AM_WRITE(sspeedr_score_w)
ADDRESS_MAP_END


static ADDRESS_MAP_START( readport, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_FLAGS( AMEF_ABITS(8) )
	AM_RANGE(0x00, 0x00) AM_READ(sspeedr_steering_r)
	AM_RANGE(0x01, 0x01) AM_READ(input_port_1_r)
	AM_RANGE(0x03, 0x03) AM_READ(input_port_2_r)
	AM_RANGE(0x04, 0x04) AM_READ(input_port_3_r)
ADDRESS_MAP_END


static ADDRESS_MAP_START( writeport, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_FLAGS( AMEF_ABITS(8) )
	AM_RANGE(0x00, 0x01) AM_WRITE(sspeedr_sound_w)
	AM_RANGE(0x02, 0x02) AM_WRITE(sspeedr_lamp_w)
	AM_RANGE(0x04, 0x05) AM_WRITE(sspeedr_time_w)
	AM_RANGE(0x06, 0x06) AM_WRITE(watchdog_reset_w)
	AM_RANGE(0x10, 0x10) AM_WRITE(sspeedr_driver_horz_w)
	AM_RANGE(0x11, 0x11) AM_WRITE(sspeedr_driver_pic_w)
	AM_RANGE(0x12, 0x12) AM_WRITE(sspeedr_driver_horz_2_w)
	AM_RANGE(0x13, 0x13) AM_WRITE(sspeedr_drones_horz_w)
	AM_RANGE(0x14, 0x14) AM_WRITE(sspeedr_drones_horz_2_w)
	AM_RANGE(0x15, 0x15) AM_WRITE(sspeedr_drones_mask_w)
	AM_RANGE(0x16, 0x16) AM_WRITE(sspeedr_driver_vert_w)
	AM_RANGE(0x17, 0x18) AM_WRITE(sspeedr_track_vert_w)
	AM_RANGE(0x19, 0x19) AM_WRITE(sspeedr_track_horz_w)
	AM_RANGE(0x1a, 0x1a) AM_WRITE(sspeedr_track_horz_2_w)
	AM_RANGE(0x1b, 0x1b) AM_WRITE(sspeedr_track_ice_w)
	AM_RANGE(0x1c, 0x1e) AM_WRITE(sspeedr_drones_vert_w)
	AM_RANGE(0x1f, 0x1f) AM_WRITE(sspeedr_int_ack_w)
ADDRESS_MAP_END


INPUT_PORTS_START( sspeedr )

	PORT_START_TAG("IN0")
	PORT_BIT( 0xff, 0x80, IPT_DIAL ) PORT_MINMAX(0x00,0xff) PORT_SENSITIVITY(25) PORT_KEYDELTA(10)

	PORT_START_TAG("IN1")
	PORT_BIT( 0x1f, 0x00, IPT_PEDAL ) PORT_MINMAX(0x00,0x1f) PORT_SENSITIVITY(25) PORT_KEYDELTA(20) PORT_REVERSE

	PORT_START_TAG("DSW")
	PORT_DIPNAME( 0x03, 0x00, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 1C_2C ) )
	PORT_DIPNAME( 0x0C, 0x08, "Play Time" )
	PORT_DIPSETTING(    0x00, "60 seconds")
	PORT_DIPSETTING(    0x04, "70 seconds")
	PORT_DIPSETTING(    0x08, "80 seconds")
	PORT_DIPSETTING(    0x0C, "90 seconds")
	PORT_DIPNAME( 0x10, 0x00, "Extended Play" )
	PORT_DIPSETTING(    0x00, "20 seconds" )
	PORT_DIPSETTING(    0x10, "30 seconds" )
	PORT_DIPNAME( 0xE0, 0x20, DEF_STR( Service_Mode ) )
	PORT_DIPSETTING(    0x20, "Play Mode" )
	PORT_DIPSETTING(    0xA0, "RAM/ROM Test" )
	PORT_DIPSETTING(    0xE0, "Accelerator Adjustment" )

	PORT_START_TAG("IN3")
	PORT_BIT ( 0x01, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT ( 0x02, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT ( 0x04, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_TOGGLE /* gear shift lever */
	PORT_BIT ( 0x08, IP_ACTIVE_LOW, IPT_COIN1 )

INPUT_PORTS_END


static const gfx_layout car_layout =
{
	32, 16,
	16,
	4,
	{ 0, 1, 2, 3 },
	{
		0x04, 0x04, 0x00, 0x00, 0x0c, 0x0c, 0x08, 0x08,
		0x14, 0x14, 0x10, 0x10, 0x1c, 0x1c, 0x18, 0x18,
		0x24, 0x24, 0x20, 0x20, 0x2c, 0x2c, 0x28, 0x28,
		0x34, 0x34, 0x30, 0x30, 0x3c, 0x3c, 0x38, 0x38
	},
	{
		0x000, 0x040, 0x080, 0x0c0, 0x100, 0x140, 0x180, 0x1c0,
		0x200, 0x240, 0x280, 0x2c0, 0x300, 0x340, 0x380, 0x3c0
	},
	0x400
};


static const gfx_decode gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &car_layout, 0, 1 },
	{ REGION_GFX2, 0, &car_layout, 0, 1 },
	{ -1 }
};


static MACHINE_DRIVER_START( sspeedr )

	/* basic machine hardware */
	MDRV_CPU_ADD(Z80, 19968000 / 8)
	MDRV_CPU_PROGRAM_MAP(readmem, writemem)
	MDRV_CPU_IO_MAP(readport, writeport)
	MDRV_CPU_VBLANK_INT(irq0_line_assert, 1)

	MDRV_FRAMES_PER_SECOND(59.39)
	MDRV_VBLANK_DURATION(16 * 1000000 / 15680)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(376, 256)
	MDRV_VISIBLE_AREA(0, 375, 0, 247)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(16)

	MDRV_PALETTE_INIT(sspeedr)
	MDRV_VIDEO_START(sspeedr)
	MDRV_VIDEO_UPDATE(sspeedr)
	MDRV_VIDEO_EOF(sspeedr)

	/* sound hardware */
MACHINE_DRIVER_END


ROM_START( sspeedr )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )
	ROM_LOAD( "ssr0000.pgm", 0x0000, 0x0800, CRC(bfc7069a) SHA1(2f7aa3d3c7cfd804ba4b625c6a8338534a204855) )
	ROM_LOAD( "ssr0800.pgm", 0x0800, 0x0800, CRC(ec46b59a) SHA1(d5727efecb32ad3d034b885e4a57d7373368ca9e) )

	ROM_REGION( 0x0800, REGION_GFX1, ROMREGION_DISPOSE ) /* driver */
	ROM_LOAD( "ssrm762a.f3", 0x0000, 0x0800, CRC(de4653a9) SHA1(a6bbffb7eb60581eee43c74d20ca00b50c9a6e07) )

	ROM_REGION( 0x0800, REGION_GFX2, ROMREGION_DISPOSE ) /* drone */
	ROM_LOAD( "ssrm762b.j3", 0x0000, 0x0800, CRC(ef6a1cd6) SHA1(77c31f14783e5ba90849bdc930b099c8360aeba7) )

	ROM_REGION( 0x0800, REGION_GFX3, 0 ) /* track */
	ROM_LOAD( "ssrm762c.l3", 0x0000, 0x0800, CRC(ebaad3ee) SHA1(54ac994b505d20c75cf07a4f68da12360ee00153) )
ROM_END


GAME( 1979, sspeedr, 0, sspeedr, sspeedr, 0, ROT270, "Midway", "Super Speed Race", GAME_NO_SOUND )
