/*

Twins
Electronic Devices, 1994

PCB Layout
----------

This is a very tiny PCB,
only about 6 inches square.

|-----------------------|
|     6116   16MHz 62256|
|TEST 6116 24C02        |
|             PAL  62256|
|J   62256 |--------|   |
|A         |TPC1020 | 2 |
|M   62256 |AFN-084C|   |
|M         |        | 1 |
|A         |--------|   |
| AY3-8910              |
|                 D70116|
|-----------------------|
Notes:
    V30 clock      : 8.000MHz (16/2)
    AY3-8910 clock : 2.000MHz (16/8)
    VSync          : 50Hz



seems a similar board to hotblocks

same TPC1020 AFN-084C chip
same 24c02 eeprom
V30 instead of I88
AY3-8910 instead of YM2149 (compatible)

video is not banked in this case instead palette data is sent to the ports
strange palette format.

todo:
hook up eeprom
takes a long time to boot (eeprom?)


Electronic Devices was printed on rom labels
1994 date string is in ROM

*/

#include "driver.h"
#include "sound/ay8910.h"

static UINT8 *twins_videoram;
static UINT16 *twins_pal;
static UINT16 paloff = 0;

/* port 4 is eeprom */
static READ8_HANDLER( twins_port4_r )
{
	return 0xff;
}

static WRITE8_HANDLER( twins_port4_w )
{
}

static WRITE8_HANDLER( port6_pal0_w )
{
	twins_pal[paloff] = (twins_pal[paloff] & 0xff00) | data;
}

static WRITE8_HANDLER( port7_pal1_w )
{
	twins_pal[paloff] = (twins_pal[paloff] & 0x00ff) | (data<<8);
	paloff = (paloff + 1) & 0xff;

}

/* ??? weird ..*/
static WRITE8_HANDLER( porte_paloff0_w )
{
	paloff = 0;
}

/* ??? weird ..*/
static WRITE8_HANDLER( portf_paloff1_w )
{
	paloff = 0;
}

static ADDRESS_MAP_START( twins_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x00000, 0x0ffff) AM_RAM
	AM_RANGE(0x10000, 0x1ffff) AM_RAM AM_BASE(&twins_videoram)
	AM_RANGE(0x20000, 0xfffff) AM_ROM
ADDRESS_MAP_END

static ADDRESS_MAP_START( twins_io, ADDRESS_SPACE_IO, 8 )
	AM_RANGE(0x0000, 0x0000) AM_WRITE(AY8910_control_port_0_w)
	AM_RANGE(0x0002, 0x0002) AM_READ(AY8910_read_port_0_r) AM_WRITE(AY8910_write_port_0_w)
	AM_RANGE(0x0004, 0x0004) AM_READ(twins_port4_r) AM_WRITE(twins_port4_w)
	AM_RANGE(0x0006, 0x0006) AM_WRITE(port6_pal0_w)
	AM_RANGE(0x0007, 0x0007) AM_WRITE(port7_pal1_w)
	AM_RANGE(0x000e, 0x000e) AM_WRITE(porte_paloff0_w)
	AM_RANGE(0x000f, 0x000f) AM_WRITE(portf_paloff1_w)
ADDRESS_MAP_END

VIDEO_START(twins)
{
	twins_pal = auto_malloc(0x100*2);
	return 0;
}

VIDEO_UPDATE(twins)
{
	int y,x,count;
	int i;
	static int xxx=320,yyy=204;

	fillbitmap(bitmap, get_black_pen(), 0);

	for (i=0;i<0x100;i++)
	{
		int dat,r,g,b;
		dat = twins_pal[i];

		r = dat & 0x1f;
		r = BITSWAP8(r,7,6,5,0,1,2,3,4);

		g = (dat>>5) & 0x1f;
		g = BITSWAP8(g,7,6,5,0,1,2,3,4);

		b = (dat>>10) & 0x1f;
		b = BITSWAP8(b,7,6,5,0,1,2,3,4);

		palette_set_color(i, r*8,g*8,b*8);
	}

	count=0;
	for (y=0;y<yyy;y++)
	{
		for(x=0;x<xxx;x++)
		{
			plot_pixel(bitmap, x,y, twins_videoram[count]);
			count++;
		}
	}
}


INPUT_PORTS_START(twins)
	PORT_START	/* 8bit */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_PLAYER(1) PORT_8WAY
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_PLAYER(1) PORT_8WAY
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_PLAYER(1) PORT_8WAY
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_PLAYER(1) PORT_8WAY
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(1)
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(1)

	PORT_START	/* 8bit */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_PLAYER(2) PORT_8WAY
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_PLAYER(2) PORT_8WAY
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_PLAYER(2) PORT_8WAY
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_PLAYER(2) PORT_8WAY
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(2)
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(2)
INPUT_PORTS_END


static struct AY8910interface ay8910_interface =
{
	input_port_0_r,
	input_port_1_r
};

static MACHINE_DRIVER_START( twins )
	/* basic machine hardware */
	MDRV_CPU_ADD(V30, 8000000)
	MDRV_CPU_PROGRAM_MAP(twins_map, 0)
	MDRV_CPU_IO_MAP(twins_io,0)
	MDRV_CPU_VBLANK_INT(nmi_line_pulse,1)

	MDRV_FRAMES_PER_SECOND(50)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(320,256)
	MDRV_VISIBLE_AREA(0, 320-1, 0, 200-1)
	MDRV_PALETTE_LENGTH(0x100)

	MDRV_VIDEO_START(twins)
	MDRV_VIDEO_UPDATE(twins)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")

	MDRV_SOUND_ADD(AY8910, 2000000)
	MDRV_SOUND_CONFIG(ay8910_interface)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.0)
MACHINE_DRIVER_END

ROM_START( twins )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )
	ROM_LOAD16_BYTE( "1.bin", 0x000000, 0x080000, CRC(d5ef7b0d) SHA1(7261dca5bb0aef755b4f2b85a159b356e7ac8219) )
	ROM_LOAD16_BYTE( "2.bin", 0x000001, 0x080000, CRC(8a5392f4) SHA1(e6a2ecdb775138a87d27aa4ad267bdec33c26baa) )
ROM_END

GAME( 1994, twins, 0, twins, twins, 0, ROT0, "Electronic Devices", "Twins", 0 )
