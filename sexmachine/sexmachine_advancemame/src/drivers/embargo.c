/***************************************************************************

Cinematronics Embargo driver

***************************************************************************/

#include "driver.h"
#include "cpu/s2650/s2650.h"

static int dial_enable_1;
static int dial_enable_2;

static int input_select;



static VIDEO_UPDATE( embargo )
{
	copybitmap(bitmap, tmpbitmap, 0, 0, 0, 0, cliprect, TRANSPARENCY_NONE, 0);
}


static WRITE8_HANDLER( embargo_videoram_w )
{
	int col = offset % 32;
	int row = offset / 32;

	int i;

	for (i = 0; i < 8; i++)
	{
		plot_pixel(tmpbitmap, 8 * col + i, row, (data >> i) & 1);
	}

	videoram[offset] = data;
}


static READ8_HANDLER( embargo_input_r )
{
	return (readinputport(1) << (7 - input_select)) & 0x80;
}


static READ8_HANDLER( embargo_dial_r )
{
	UINT8 lo = 0;
	UINT8 hi = 0;

	UINT8 mapped_lo = 0;
	UINT8 mapped_hi = 0;

	int i;

	/* game reads 4 bits per dial and maps them onto clock directions */

	static const UINT8 map[] =
	{
		0x0, 0xB, 0x1, 0x2, 0x4, 0x4, 0x2, 0x3,
		0x9, 0xA, 0x8, 0x9, 0x8, 0x5, 0x7, 0x6
	};

	if (dial_enable_1 && !dial_enable_2)
	{
		lo = readinputport(3);
		hi = readinputport(4);
	}
	if (dial_enable_2 && !dial_enable_1)
	{
		lo = readinputport(5);
		hi = readinputport(6);
	}

	lo = 12 * lo / 256;
	hi = 12 * hi / 256;

	for (i = 0; i < 16; i++)
	{
		if (map[i] == lo)
		{
			mapped_lo = i;
		}
		if (map[i] == hi)
		{
			mapped_hi = i;
		}
	}

	return 16 * mapped_hi + mapped_lo;
}


static WRITE8_HANDLER( embargo_port1_w )
{
	dial_enable_1 = data & 1; /* other bits unknown */
}
static WRITE8_HANDLER( embargo_port2_w )
{
	dial_enable_2 = data & 1; /* other bits unknown */
}


static WRITE8_HANDLER( embargo_input_w )
{
	input_select = data & 7;
}


static ADDRESS_MAP_START( readmem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x0fff) AM_READ(MRA8_ROM)
	AM_RANGE(0x1e00, 0x3dff) AM_READ(MRA8_RAM)
ADDRESS_MAP_END


static ADDRESS_MAP_START( writemem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x0fff) AM_WRITE(MWA8_ROM)
	AM_RANGE(0x1e00, 0x1fff) AM_WRITE(MWA8_RAM)
	AM_RANGE(0x2000, 0x3dff) AM_WRITE(embargo_videoram_w) AM_BASE(&videoram) AM_SIZE(&videoram_size)
ADDRESS_MAP_END


static ADDRESS_MAP_START( readport, ADDRESS_SPACE_IO, 8 )
	AM_RANGE(0x01, 0x01) AM_READ(input_port_0_r)
	AM_RANGE(0x02, 0x02) AM_READ(embargo_dial_r)
	AM_RANGE(S2650_DATA_PORT, S2650_DATA_PORT) AM_READ(input_port_2_r)
	AM_RANGE(S2650_CTRL_PORT, S2650_CTRL_PORT) AM_READ(embargo_input_r)
ADDRESS_MAP_END


static ADDRESS_MAP_START( writeport, ADDRESS_SPACE_IO, 8 )
	AM_RANGE(0x01, 0x01) AM_WRITE(embargo_port1_w)
	AM_RANGE(0x02, 0x02) AM_WRITE(embargo_port2_w)
	AM_RANGE(0x03, 0x03) AM_WRITE(MWA8_NOP) /* always 0xFE */
	AM_RANGE(S2650_CTRL_PORT, S2650_CTRL_PORT) AM_WRITE(embargo_input_w)
ADDRESS_MAP_END


INPUT_PORTS_START( embargo )

	PORT_START /* port 0x01 */
	PORT_DIPNAME( 0x03, 0x00, "Rounds" )
	PORT_DIPSETTING(    0x00, "3" )
	PORT_DIPSETTING(    0x01, "4" )
	PORT_DIPSETTING(    0x02, "5" )
	PORT_DIPSETTING(    0x03, "6" )

	PORT_START /* S2650_CONTROL_PORT */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_START3 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_START4 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(1)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(2)
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(3)
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(4)

	PORT_START /* S2650_DATA_PORT */
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_COIN1 )

	PORT_START
	PORT_BIT( 0xff, 0x80, IPT_DIAL ) PORT_SENSITIVITY(50) PORT_KEYDELTA(8) PORT_PLAYER(1)
	PORT_START
	PORT_BIT( 0xff, 0x80, IPT_DIAL ) PORT_SENSITIVITY(50) PORT_KEYDELTA(8) PORT_PLAYER(2)
	PORT_START
	PORT_BIT( 0xff, 0x80, IPT_DIAL ) PORT_SENSITIVITY(50) PORT_KEYDELTA(8) PORT_PLAYER(3)
	PORT_START
	PORT_BIT( 0xff, 0x80, IPT_DIAL ) PORT_SENSITIVITY(50) PORT_KEYDELTA(8) PORT_PLAYER(4)

INPUT_PORTS_END


static MACHINE_DRIVER_START( embargo )

	/* basic machine hardware */
	MDRV_CPU_ADD(S2650, 625000)
	MDRV_CPU_PROGRAM_MAP(readmem, writemem)
	MDRV_CPU_IO_MAP(readport, writeport)

	MDRV_FRAMES_PER_SECOND(60)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(256, 256)
	MDRV_VISIBLE_AREA(0, 255, 0, 239)
	MDRV_PALETTE_LENGTH(2)

	MDRV_PALETTE_INIT(black_and_white)
	MDRV_VIDEO_START(generic_bitmapped)
	MDRV_VIDEO_UPDATE(embargo)

	/* sound hardware */
MACHINE_DRIVER_END


ROM_START( embargo )
	ROM_REGION( 0x8000, REGION_CPU1, 0 )
	ROM_LOAD( "emb1", 0x0000, 0x0200, CRC(00dcbc24) SHA1(67018a20d7694618123499640f041fb518ea29fa) )
	ROM_LOAD( "emb2", 0x0200, 0x0200, CRC(e7069b11) SHA1(b933095087cd4fe10f12fd244606aaaed1c31bca) )
	ROM_LOAD( "emb3", 0x0400, 0x0200, CRC(1af7a966) SHA1(a8f6d1063927106f44c43f64c26b52c07c5450df) )
	ROM_LOAD( "emb4", 0x0600, 0x0200, CRC(d9c75da0) SHA1(895784ec543f1c73ced5f37751a26cb3305030f3) )
	ROM_LOAD( "emb5", 0x0800, 0x0200, CRC(15960b58) SHA1(2e6c196b240cef92799f83deef2b1c501c01f9c9) )
	ROM_LOAD( "emb6", 0x0a00, 0x0200, CRC(7ba23058) SHA1(ad3736ec7617ecb902ea686055e55203be1ea5fd) )
	ROM_LOAD( "emb7", 0x0c00, 0x0200, CRC(6d46a593) SHA1(5432ae1c167e774c47f06ffd0e8acf801891dee1) )
	ROM_LOAD( "emb8", 0x0e00, 0x0200, CRC(f0b00634) SHA1(317aacc9022596a2af0f3b399fe119fe9c8c1679) )
ROM_END


GAME( 1977, embargo, 0, embargo, embargo, 0, ROT0, "Cinematronics", "Embargo", GAME_NO_SOUND )
