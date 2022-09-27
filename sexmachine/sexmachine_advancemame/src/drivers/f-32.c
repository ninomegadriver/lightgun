/********************************************************************

 F-E1-32 driver


 Supported Games      PCB-ID
 ----------------------------------
 Mosaic               F-E1-32-009

 driver by Pierpaolo Prazzoli

   CPU: Hyperstone E1-32XN
 Video: QuickLogic QL2003-XPL84C
 Sound: OKI 6295, BS901 (YM2151) & BS902 (YM3012)
   OSC: 20MHz & 14.31818MHz
EEPROM: 93C46

F-E1-32-009
+------------------------------------------------------------------+
|            VOL                               +---------+         |
+-+                              YM3812        |   SND   |         |
  |                                            +---------+         |
+-+                              YM2151            OKI6295         |
|                                                                  |
|                                   +---------------+              |
|                                   |               |              |
|J                   +-------+      |               |              |
|A                   | VRAML |      | QuickLogic    |  14.31818MHz |
|M                   +-------+      | QL2003-XPL84C |              |
|M                   +-------+      | 9819 BA       |   +-----+    |
|A                   | VRAMU |      |               |   |93C46|    |
|                    +-------+      +---------------+   +-----+    |
|C                                                                 |
|O                                      +---------+   +---------+  |
|N                                      |   L00   |   |   U00   |  |
|N                                      |         |   |         |  |
|E                                      +---------+   +---------+  |
|C                   +------------+     +---------+   +---------+  |
|T                   |            |     |   L01   |   |   U01   |  |
|O                   |            |     |         |   |         |  |
|R                   | HyperStone |     +---------+   +---------+  |
|                    |  E1-32XN   |     +---------+   +---------+  |
|                    |            |     |   L02   |   |   U02   |  |
|          +-----+   |            |     |         |   |         |  |
|          |DRAML|   +------------+     +---------+   +---------+  |
+-+        +-----+                      +---------+   +---------+  |
  |        +-----+               20MHz  |   L03   |   |   U03   |  |
+-+        |DRAMU|                      |         |   |         |  |
|          +-----+    +----------+      +---------+   +---------+  |
|  +--+ +--+          |   ROM1   |                                 |
|  |S3| |S1|          +----------+                                 |
+------------------------------------------------------------------+

S3 is a reset button
S1 is the setup button

VRAML & VRAMU are KM6161002CJ-12
DRAML & DRAMU are GM71C18163CJ6

ROM1 & SND are stardard 27C040 and/or 27C020 eproms
L00-L03 & U00-U03 are 29F1610ML Flash roms

*********************************************************************/

#include "driver.h"
#include "machine/eeprom.h"
#include "sound/2151intf.h"
#include "sound/okim6295.h"

static UINT32 *vram;

static READ32_HANDLER( oki_32bit_r )
{
	return OKIM6295_status_0_r(0);
}

static WRITE32_HANDLER( oki_32bit_w )
{
	OKIM6295_data_0_w(0, data & 0xff);
}

static READ32_HANDLER( ym2151_status_32bit_r )
{
	return YM2151_status_port_0_r(0);
}

static WRITE32_HANDLER( ym2151_data_32bit_w )
{
	YM2151_data_port_0_w(0, data & 0xff);
}

static WRITE32_HANDLER( ym2151_register_32bit_w )
{
	YM2151_register_port_0_w(0,data & 0xff);
}

static WRITE32_HANDLER( vram_w )
{
	int x,y;

	COMBINE_DATA(&vram[offset]);

	y = offset >> 8;
	x = offset & 0xff;

	if(x < 320/2 && y < 224)
	{
		plot_pixel(tmpbitmap,x*2+0,y,(vram[offset] >> 16) & 0x7fff);
		plot_pixel(tmpbitmap,x*2+1,y,vram[offset] & 0x7fff);
	}
}

static READ32_HANDLER( eeprom_r )
{
	return EEPROM_read_bit();
}

static WRITE32_HANDLER( eeprom_bit_w )
{
	EEPROM_write_bit(data & 0x01);
}

static WRITE32_HANDLER( eeprom_cs_line_w )
{
	EEPROM_set_cs_line( (data & 0x01) ? CLEAR_LINE : ASSERT_LINE );
}

static WRITE32_HANDLER( eeprom_clock_line_w )
{
	EEPROM_set_clock_line( (~data & 0x01) ? ASSERT_LINE : CLEAR_LINE );
}


static ADDRESS_MAP_START( common_map, ADDRESS_SPACE_PROGRAM, 32 )
	AM_RANGE(0x00000000, 0x001fffff) AM_RAM
	AM_RANGE(0x40000000, 0x4003ffff) AM_WRITE(vram_w) AM_BASE(&vram)
	AM_RANGE(0x80000000, 0x80ffffff) AM_ROM AM_REGION(REGION_USER2,0)
	AM_RANGE(0xfff80000, 0xffffffff) AM_ROM AM_REGION(REGION_USER1,0)
ADDRESS_MAP_END

static ADDRESS_MAP_START( mosaicf2_io, ADDRESS_SPACE_IO, 32 )
	AM_RANGE(0x4000, 0x4003) AM_READ(oki_32bit_r)
	AM_RANGE(0x4810, 0x4813) AM_READ(ym2151_status_32bit_r)
	AM_RANGE(0x5000, 0x5003) AM_READ(input_port_0_dword_r)
	AM_RANGE(0x5200, 0x5203) AM_READ(input_port_1_dword_r)
	AM_RANGE(0x5400, 0x5403) AM_READ(eeprom_r)
	AM_RANGE(0x6000, 0x6003) AM_WRITE(oki_32bit_w)
	AM_RANGE(0x6800, 0x6803) AM_WRITE(ym2151_data_32bit_w)
	AM_RANGE(0x6810, 0x6813) AM_WRITE(ym2151_register_32bit_w)
	AM_RANGE(0x7000, 0x7003) AM_WRITE(eeprom_clock_line_w)
	AM_RANGE(0x7200, 0x7203) AM_WRITE(eeprom_cs_line_w)
	AM_RANGE(0x7400, 0x7403) AM_WRITE(eeprom_bit_w)
ADDRESS_MAP_END


INPUT_PORTS_START( mosaicf2 )
	PORT_START
	PORT_BIT( 0x0000ffff, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x00010000, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    ) PORT_PLAYER(1)
	PORT_BIT( 0x00020000, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  ) PORT_PLAYER(1)
	PORT_BIT( 0x00040000, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  ) PORT_PLAYER(1)
	PORT_BIT( 0x00080000, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_PLAYER(1)
	PORT_BIT( 0x00100000, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(1)
	PORT_BIT( 0x00200000, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(1)
	PORT_BIT( 0x00400000, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_PLAYER(1)
	PORT_BIT( 0x00800000, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0xff000000, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START
	PORT_BIT( 0x000000ff, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x00000100, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x00000200, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_SERVICE_NO_TOGGLE( 0x00000400, IP_ACTIVE_LOW )
	PORT_BIT( 0x00007800, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x00008000, IP_ACTIVE_LOW, IPT_VBLANK )
	PORT_BIT( 0x00010000, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    ) PORT_PLAYER(2)
	PORT_BIT( 0x00020000, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  ) PORT_PLAYER(2)
	PORT_BIT( 0x00040000, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  ) PORT_PLAYER(2)
	PORT_BIT( 0x00080000, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_PLAYER(2)
	PORT_BIT( 0x00100000, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(2)
	PORT_BIT( 0x00200000, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(2)
	PORT_BIT( 0x00400000, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_PLAYER(2)
	PORT_BIT( 0x00800000, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0xff000000, IP_ACTIVE_LOW, IPT_UNUSED )
INPUT_PORTS_END


static MACHINE_DRIVER_START( common )
	MDRV_CPU_ADD_TAG("main", E132XT, 20000000) // E132XN actually! /* 20 MHz */
	MDRV_CPU_PROGRAM_MAP(common_map,0)
	MDRV_CPU_VBLANK_INT(irq5_line_pulse, 1)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_REAL_60HZ_VBLANK_DURATION)

	MDRV_NVRAM_HANDLER(93C46)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER | VIDEO_RGB_DIRECT)
	MDRV_SCREEN_SIZE(512, 512)
	MDRV_VISIBLE_AREA(0, 319, 0, 223)
	MDRV_PALETTE_LENGTH(32768)

	MDRV_VIDEO_START(generic_bitmapped)
	MDRV_VIDEO_UPDATE(generic_bitmapped)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_STEREO("left", "right")

	MDRV_SOUND_ADD(YM2151, 14318180/4)
	MDRV_SOUND_ROUTE(0, "left", 1.0)
	MDRV_SOUND_ROUTE(1, "right", 1.0)

	MDRV_SOUND_ADD(OKIM6295, 1789772.5 / 132)
	MDRV_SOUND_CONFIG(okim6295_interface_region_1)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "left", 1.0)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "right", 1.0)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( mosaicf2 )
	MDRV_IMPORT_FROM(common)
	MDRV_CPU_MODIFY("main")
	MDRV_CPU_IO_MAP(mosaicf2_io,0)
MACHINE_DRIVER_END


ROM_START( mosaicf2 )
	ROM_REGION32_BE( 0x80000, REGION_USER1, 0 ) /* Hyperstone CPU Code */
	ROM_LOAD( "rom1.bin",            0x000000, 0x080000, CRC(fceb6f83) SHA1(b98afb477627c3b2d584c0f0fb26c4dd5b1a31e2) )

	ROM_REGION32_BE( 0x1000000, REGION_USER2, 0 )  /* gfx data */
	ROM_LOAD32_WORD_SWAP( "u00.bin", 0x000000, 0x200000, CRC(a2329675) SHA1(bff8974fab9120274821c9c9646744317f47c79c) )
	ROM_LOAD32_WORD_SWAP( "l00.bin", 0x000002, 0x200000, CRC(d96fe93b) SHA1(005d9889077825fc0e308d2981f6fca5e6b51fe8) )
	ROM_LOAD32_WORD_SWAP( "u01.bin", 0x400000, 0x200000, CRC(6379e73f) SHA1(fe5abafbcbd828795cb06a08763fae1bbe2a75ad) )
	ROM_LOAD32_WORD_SWAP( "l01.bin", 0x400002, 0x200000, CRC(a269ea82) SHA1(d962a8b3293c6f46dbefa49859b2b3e594e7a386) )
	ROM_LOAD32_WORD_SWAP( "u02.bin", 0x800000, 0x200000, CRC(c17f95cd) SHA1(1c701185be138b615d2851866288647f40809c28) )
	ROM_LOAD32_WORD_SWAP( "l02.bin", 0x800002, 0x200000, CRC(69cd9c5c) SHA1(6b4d204a6ab5f36dfba9053bb3be2d094fcfdd00) )
	ROM_LOAD32_WORD_SWAP( "u03.bin", 0xc00000, 0x200000, CRC(0e47df20) SHA1(6f6c3e7fc8c99db7ddc73d8d10a661373bb72a1a) )
	ROM_LOAD32_WORD_SWAP( "l03.bin", 0xc00002, 0x200000, CRC(d79f6ca8) SHA1(4735dda9269aa05ba1251d335dc73914f5cb43b0) )

	ROM_REGION( 0x40000, REGION_SOUND1, 0 ) /* Oki Samples */
	ROM_LOAD( "snd.bin",             0x000000, 0x040000, CRC(4584589c) SHA1(5f9824724f840767c3dc1dc04b203ddf3d78b84c) )
ROM_END


GAME( 1999, mosaicf2, 0, mosaicf2, mosaicf2, 0, ROT0, "F2 System", "Mosaic (F2 System)", GAME_SUPPORTS_SAVE )
