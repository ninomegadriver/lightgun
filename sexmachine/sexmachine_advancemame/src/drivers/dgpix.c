/********************************************************************

 "dgPIX" games driver

 Games Supported:
 ---------------------------------------------------------------------------
 - X-Files                             (c) 1999 dgPIX Entertainment Inc.
 - King of Dynast Gear (version 1.8)   (c) 1999 EZ Graphics [*]
 - Fishing Maniac 3                    (c) 2002 Saero Entertainment

 [*] the version number is written at the beginning of the game settings


 Games Needed:
 ---------------------------------------------------------------------------
 - Elfin                               (c) 1999 dgPIX Entertainment Inc.


 Original bugs:
 - In King of Dynast Gear, Roger's fast attack shows some blank lines
   in the sword "shadow" if you do it in the left direction


 driver by Pierpaolo Prazzoli & Tomasz Slanina

 - Pierpaolo Prazzoli 2006.05.06
    - added Fishing Maniac 3

 - Pierpaolo Prazzoli 2006.01.16
   - added King of Dynast Gear (protection patched by Tomasz)
   - fixed frame buffer drawing
   - fixed flash roms reading
   - added game settings writing to flash roms
   - added coin counters

 - TS 2005.02.15
   Added double buffering

 - TS 2005.02.06
   Preliminary emulation of X-Files. VRender0- is probably just framebuffer.
   Patch in DRIVER_INIT removes call at RAM adr $8f30 - protection ?
   (without fix, game freezes int one of startup screens - like on real
   board  with  protection PIC removed)

*********************************************************************/

#include "driver.h"

static UINT32 *vram;
static int vbuffer = 0;
static mame_bitmap *bitmaps[2];

static int flash_roms;
static UINT32 flash_cmd = 0;

static READ32_HANDLER( flash_r )
{
	UINT32 *ROM = (UINT32 *)memory_region(REGION_USER1);

	if(offset >= (0x2000000 - flash_roms * 0x400000) / 4)
	{
		if(flash_cmd == 0x90900000)
		{
			//read maker ID and chip ID
			return 0x00890014;
		}
		else if(flash_cmd == 0x00700000)
		{
			//read status
			return 0x80<<16;
		}
		else if(flash_cmd == 0x70700000)
		{
			//read status and ?
			return 0x82<<16;
		}
		else if(flash_cmd == 0xe8e80000)
		{
			//read status ?
			return 0x80<<16;
		}
	}

	return ROM[offset];
}

static WRITE32_HANDLER( flash_w )
{
	if(flash_cmd == 0x20200000)
	{
		// erase game settings
		if(data == 0xd0d00000)
		{
			// point to game settings
			UINT8 *rom = (UINT8 *)memory_region(REGION_USER1) + 0x1c00000 + 0x380000;
			memset(rom, 0xff, 0x10000);

			flash_cmd = 0;
		}
	}
	else if(flash_cmd == 0x0f0f0000)
	{
		if(data == 0xd0d00000)
		{
			// finished
			flash_cmd = 0;
		}
		else
		{
			UINT16 *rom = (UINT16 *)memory_region(REGION_USER1);

			// write game settings

			if(ACCESSING_LSW32)
				rom[BYTE_XOR_BE(offset*2 + 1)] = data & 0xffff;
			else
				rom[BYTE_XOR_BE(offset*2 + 0)] = (data & 0xffff0000) >> 16;
		}
	}
	else
	{
		flash_cmd = data;
	}
}

static void plot_pixel_rgb(int x, int y, int color)
{
	if (Machine->color_depth == 32)
	{
		UINT32 b = (color & 0x001f) << 3;
		UINT32 g = (color & 0x03e0) >> 2;
		UINT32 r = (color & 0x7c00) >> 7;
		((UINT32 *)bitmaps[vbuffer]->line[y])[x] = b | (g<<8) | (r<<16);
	}
	else
	{
		/* color is BGR; convert to RGB */
		color = ((color & 0x1f) << 10) | (color & 0x3e0) | ((color & 0x7c00) >> 10);
		((UINT16 *)bitmaps[vbuffer]->line[y])[x] = color;
	}
}

static WRITE32_HANDLER( vram_w )
{
	int x,y;

	switch(mem_mask)
	{
		case 0:
			vram_w(offset,data,0x0000ffff);
			vram_w(offset,data,0xffff0000);
			return;

		case 0xffff:
			if(data & 0x80000000)
				return;
		break;

		case 0xffff0000:
			if(data & 0x8000)
				return;
		break;
	}

	COMBINE_DATA(&vram[offset+(0x40000/4)*vbuffer]);

	y = offset >> 8;
	x = offset & 0xff;

	if(x < 320/2 && y < 240)
	{
		plot_pixel_rgb(x*2,  y,(vram[offset+(0x40000/4)*vbuffer]>>16) & 0x7fff);
		plot_pixel_rgb(x*2+1,y, vram[offset+(0x40000/4)*vbuffer] & 0x7fff);
	}
}

static READ32_HANDLER( vram_r )
{
	return vram[offset+(0x40000/4)*vbuffer];
}

static WRITE32_HANDLER( vbuffer_w )
{
	static int old_vbuf = 3;

	if(old_vbuf == 3 && (data & 3) == 2)
	{
		vbuffer ^= 1;
	}

	old_vbuf = data & 3;
}

static WRITE32_HANDLER( coin_w )
{
	coin_counter_w(0, data & 1);
	coin_counter_w(1, data & 2);
}

static ADDRESS_MAP_START( cpu_map, ADDRESS_SPACE_PROGRAM, 32 )
	AM_RANGE(0x00000000, 0x007fffff) AM_RAM
	AM_RANGE(0x40000000, 0x4003ffff) AM_READWRITE(vram_r, vram_w)
	AM_RANGE(0xe0000000, 0xe1ffffff) AM_READWRITE(flash_r, flash_w)
	AM_RANGE(0xe2000000, 0xe3ffffff) AM_READWRITE(flash_r, flash_w)
	AM_RANGE(0xffc00000, 0xffffffff) AM_ROM AM_REGION(REGION_USER1, 0x1c00000)
ADDRESS_MAP_END

static ADDRESS_MAP_START( io_map, ADDRESS_SPACE_IO, 32 )
	AM_RANGE(0x0200, 0x0203) AM_READNOP // used to sync with the protecion PIC? tested bits 0 and 1
	AM_RANGE(0x0400, 0x0403) AM_READWRITE(input_port_0_dword_r, vbuffer_w)
	AM_RANGE(0x0a10, 0x0a13) AM_READ(input_port_1_dword_r)
	AM_RANGE(0x0200, 0x0203) AM_WRITE(coin_w)
	AM_RANGE(0x0c00, 0x0c03) AM_WRITENOP // writes only: 1, 0, 1 at startup
	AM_RANGE(0x0c80, 0x0c83) AM_WRITENOP // sound commands / latches
	AM_RANGE(0x0c80, 0x0c83) AM_READNOP //read at startup -> cmp 0xFE
	AM_RANGE(0x0c84, 0x0c87) AM_READNOP // sound status, checks bit 0x40 and 0x80
ADDRESS_MAP_END


void nvram_handler_flashroms(mame_file *file,int read_or_write)
{
	if (read_or_write)
	{
		// point to game settings
		UINT8 *rom = (UINT8 *)memory_region(REGION_USER1) + 0x1c00000 + 0x380000;
		UINT8 tmp[0x10000];
		int i;

		// save the new settings
		for( i = 0; i < 0x10000; i++ )
			tmp[i] = rom[WORD_XOR_BE(i)];

		mame_fwrite( file, tmp, 0x10000 );
	}
	else if (file)
	{
		// point to game settings
		UINT8 *rom = (UINT8 *)memory_region(REGION_USER1) + 0x1c00000 + 0x380000;
		UINT8 tmp[0x10000];
		int i;

		mame_fread( file, tmp, 0x10000 );

		// overlap the default settings with the saved ones
		for( i = 0; i < 0x10000; i++ )
			rom[WORD_XOR_BE(i)] = tmp[i];
	}
}

INPUT_PORTS_START( dgpix )
	PORT_START
	PORT_BIT( 0x00000003, IP_ACTIVE_LOW, IPT_VBLANK ) //value 2 is used by fmaniac3
	PORT_BIT( 0xfffffffc, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START
	PORT_BIT( 0x00000001, IP_ACTIVE_LOW, IPT_JOYSTICK_UP	) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x00000002, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN	) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x00000004, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT	) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x00000008, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT	) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x00000010, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(1)
	PORT_BIT( 0x00000020, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(1)
	PORT_BIT( 0x00000040, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x00000080, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x00000100, IP_ACTIVE_LOW, IPT_JOYSTICK_UP	) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x00000200, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN	) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x00000400, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT	) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x00000800, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT	) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x00001000, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(2)
	PORT_BIT( 0x00002000, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(2)
	PORT_BIT( 0x00004000, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x00008000, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_SERVICE_NO_TOGGLE( 0x00010000, IP_ACTIVE_LOW )
	PORT_BIT( 0x00010000, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x00020000, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x00040000, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x00080000, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x00100000, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x00200000, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x00400000, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x00800000, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0xff000000, IP_ACTIVE_LOW, IPT_UNUSED )
INPUT_PORTS_END

VIDEO_START( dgpix )
{
	vram = auto_malloc(0x40000*2);
	bitmaps[0] = auto_bitmap_alloc(Machine->drv->screen_width,Machine->drv->screen_height);
	bitmaps[1] = auto_bitmap_alloc(Machine->drv->screen_width,Machine->drv->screen_height);

	if(!bitmaps[0] || !bitmaps[1])
		return 1;

	return 0;
}

VIDEO_UPDATE( dgpix )
{
	copybitmap(bitmap,bitmaps[vbuffer ^ 1],0,0,0,0,cliprect,TRANSPARENCY_NONE,0);
}

static MACHINE_DRIVER_START( dgpix )
	MDRV_CPU_ADD(E132XT, 20000000)
	MDRV_CPU_PROGRAM_MAP(cpu_map,0)
	MDRV_CPU_IO_MAP(io_map,0)

/*
    unknown 16bit sound cpu, embedded inside the KS0164 sound chip
    running at 16.9MHz
*/

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_REAL_60HZ_VBLANK_DURATION)

	MDRV_NVRAM_HANDLER(flashroms)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER | VIDEO_RGB_DIRECT)
	MDRV_SCREEN_SIZE(512, 256)
	MDRV_VISIBLE_AREA(0, 319, 0, 239)
	MDRV_PALETTE_LENGTH(32768)

	MDRV_VIDEO_START(dgpix)
	MDRV_VIDEO_UPDATE(dgpix)

	/* sound hardware */
	// KS0164 sound chip
MACHINE_DRIVER_END

/*

X-Files
dgPIX Entertainment Inc. 1999

Contrary to what you might think on first hearing the title, this game
is like Match It 2 etc. However, the quality of the graphics
is outstanding, perhaps the most high quality seen in this "type" of game.
At the end of the level, you are presented with a babe, where you can use
the joystick and buttons to scroll up and down and zoom in for erm...
a closer inspection of the 'merchandise' ;-))


PCB Layout
----------


VRenderOMinus Rev4
-------------------------------------------------------
|                                                     |
|   DA1545A             C-O-N-N-1                 C   |
|                                                 O   |
|  POT1    T2316162               SEC KS0164      N   |
|  POT2    T2316162                               N   |
|J                                    169NDK19:   3   |
|A     20MHz                           CONN2          |
|M  KA4558                                            |
|M                                                    |
|A                                SEC KM6161002CJ-12  |
|          E1-32XT                                    |
|                                 SEC KM6161002CJ-12  |
|                                                     |
|       ST7705C                   SEC KM6161002CJ-12  |
| B1             XCS05                                |
| B2 B3          14.31818MHz      SEC KM6161002CJ-12  |
-------------------------------------------------------


Notes
-----
ST7705C          : Reset/Watchdog IC (SOIC8)
E1-32XT          : Hyperstone E1-32XT CPU
169NDK19         : Xtal, 16.9MHz
CONN1,CONN2,CONN3: Connectors for small daughterboard containing
                   3x DA28F320J5 (32M surface mounted SSOP56 Flash ROM)
XCS05            : XILINX XCS05 PLD
B1,B2,B3         : Push Buttons for TEST, SERVICE and RESET
SEC KS0164       : Samsung Electronics KS0164 General Midi compliant 32-voice Wavetable Synthesizer Chip
                   with built-in 16bit CPU and MPU-401 compatibility. (QFP100)
T2316162         : Main program RAM (SOJ44)
SEC KM6161002    : Graphics RAM (SOJ44)

*/

ROM_START( xfiles )
	ROM_REGION32_BE( 0x2000000, REGION_USER1, ROMREGION_ERASE00 ) /* Hyperstone CPU Code & Data */
	/* 0 - 0x17fffff empty space */
	ROM_LOAD16_WORD_SWAP( "u8.bin",  0x1800000, 0x400000, CRC(3b2c2bc1) SHA1(1c07fb5bd8a8c9b5fb169e6400fef845f3aee7aa) )
	ROM_LOAD16_WORD_SWAP( "u9.bin",  0x1c00000, 0x400000, CRC(6ecdd1eb) SHA1(e26c9711e589865cc75ec693d382758fa52528b8) )

	ROM_REGION( 0x400000, REGION_CPU2, 0 ) /* sound rom */
	ROM_LOAD16_WORD_SWAP( "u10.bin", 0x0000000, 0x400000, CRC(f2ef1eb9) SHA1(d033d140fce6716d7d78509aa5387829f0a1404c) )
ROM_END

/*
King of Dynast Gear
EZ Graphics, 1999

This game runs on the same hardware as X-Files.

PCB Layout
----------

VRenderO Minus Rev5 dgPIX Entertainment Inc. 1999
|-----------------------------------------------------|
|TDA1515                C-O-N-N-1                     |
|   DA1545A                                       C   |
|                                                 O   |
|  VOL1    K4E151611                  KS0164      N   |
|  VOL2    K4E151611                              N   |
|J                                    169NDK19    3   |
|A     20MHz                           CONN2          |
|M  KA4558                                            |
|M                                                    |
|A                                          KM6161002 |
|          E1-32XT                                    |
|                                           KM6161002 |
|                                                     |
|       ST7705C                             KM6161002 |
| B1             XCS05                                |
| B2 B3          14.31818MHz  LED           KM6161002 |
|-----------------------------------------------------|
Notes:
      ST7705C      - Reset/Watchdog IC (SOIC8)
      E1-32XT      - Hyperstone E1-32XT CPU (QFP144)
      169NDK19     - Xtal, 16.9344MHz
      CONN1,CONN2, - Connectors for joining main board to small sub-board
      CONN3
      XCS05        - Xilinx Spartan XCS05 FPGA (QFP100)
      B1,B2,B3     - Push Buttons for TEST, SERVICE and RESET
      KS0164       - Samsung Electronics KS0164 General Midi compliant 32-voice Wavetable Synthesizer Chip
                     with built-in 16bit CPU and MPU-401 compatibility. (QFP100)
      K4E151611    - Samsung K4E151611C-JC60 1M x16Bit CMOS EDO DRAM (SOJ44)
      KM6161002    - Samsung KM6161002CJ-12 64k x16Bit High-Speed CMOS SRAM (SOJ44)

Sub-Board
---------

Flash Module Type-A REV2 dgPIX Entertainment Inc. 1999
|---------------------------------------|
|            C-O-N-N-1            U100  |
|C                FLASH.U3      FLASH.U5|
|O        FLASH.U2       FLASH.U4       |
|N FLASH.U10                            |
|N                                      |
|3                FLASH.U7      FLASH.U9|
|  CONN2  FLASH.U6       FLASH.U8       |
|---------------------------------------|
Notes:
      FLASH        - Intel DA28F320J5 32M x8 StrataFlash surface-mounted FlashROM (SSOP56)
      CONN1,CONN2,
      CONN3        - Connectors for joining small sub-board to main board
      U100         - A custom programmed PIC (Programmable Interrupt Controller), rebadged as 'dgPIX-PR1' (DIP18)

*/

ROM_START( kdynastg )
	ROM_REGION32_BE( 0x2000000, REGION_USER1, ROMREGION_ERASE00 )  /* Hyperstone CPU Code & Data */
	/* 0 - 0x0ffffff empty space */
	ROM_LOAD16_WORD_SWAP( "flash.u6",  0x1000000, 0x400000, CRC(280dd64e) SHA1(0e23b227b1183fb5591c3a849b5a5fe7faa23cc8) )
	ROM_LOAD16_WORD_SWAP( "flash.u7",  0x1400000, 0x400000, CRC(f9125894) SHA1(abaad31f7a02143ea7029e47e6baf2976365f70c) )
	ROM_LOAD16_WORD_SWAP( "flash.u8",  0x1800000, 0x400000, CRC(1016b61c) SHA1(eab4934e1f41cc26259e5187a94ceebd45888a94) )
	ROM_LOAD16_WORD_SWAP( "flash.u9",  0x1c00000, 0x400000, CRC(093d9243) SHA1(2a643acc7144193aaa3606a84b0c67aadb4c543b) )

	ROM_REGION( 0x400000, REGION_CPU2, 0 ) /* sound rom */
	ROM_LOAD16_WORD_SWAP( "flash.u10", 0x0000000, 0x400000, CRC(3f103cb1) SHA1(2ff9bd73f3005f09d872018b81c915b01d6703f5) )
ROM_END

/*
Fishing Maniac 3
Saero Entertainment, 2002

This game runs on hardware that is identical to XFiles and King Of Dynast Gear
but with less ROMs and no PIC.

PCB Layout
----------

VRenderO Minus Rev4 dgPIX Entertainment Inc. 1999
|-----------------------------------------------------|
|TDA1515                C-O-N-N-1                     |
|   DA1545A                                       C   |
|                                                 O   |
|  VOL1    K4E151611                  KS0164      N   |
|  VOL2    K4E151611                              N   |
|J                                    169NDK19    3   |
|A     20MHz                           CONN2          |
|M  KA4558                                            |
|M                                                    |
|A                                          KM6161002 |
|          E1-32XT                                    |
|                                           KM6161002 |
|                                                     |
|       ST7705C                             KM6161002 |
| B1             XCS05                                |
| B2 B3          14.31818MHz  LED           KM6161002 |
|-----------------------------------------------------|
Notes:
      ST7705C      - Reset/Watchdog IC (SOIC8)
      E1-32XT      - Hyperstone E1-32XT CPU (QFP144)
      169NDK19     - Xtal, 16.9344MHz
      CONN1,CONN2, - Connectors for joining main board to small sub-board
      CONN3
      XCS05        - Xilinx Spartan XCS05 FPGA (QFP100)
      B1,B2,B3     - Push Buttons for TEST, SERVICE and RESET
      KS0164       - Samsung Electronics KS0164 General Midi compliant 32-voice Wavetable Synthesizer Chip
                     with built-in 16bit CPU and MPU-401 compatibility. (QFP100)
      K4E151611    - Samsung K4E151611C-JC60 1M x16 CMOS EDO DRAM (SOJ44)
      KM6161002    - Samsung KM6161002CJ-12 64k x16 High-Speed CMOS SRAM (SOJ44)

Sub-Board
---------

Flash Module Type-A REV2 dgPIX Entertainment Inc. 1999
|---------------------------------------|
|            C-O-N-N-1            U100  |
|C                FLASH.U3      FLASH.U5|
|O        FLASH.U2       FLASH.U4       |
|N FLASH.U10                            |
|N                                      |
|3                FLASH.U7      FLASH.U9|
|  CONN2  FLASH.U6       FLASH.U8       |
|---------------------------------------|
Notes:
      FLASH        - Intel DA28F320J5 32M x8 StrataFlash surface-mounted FlashROM (SSOP56)
                     Only U8, U9 & U10 are populated
      CONN1,CONN2,
      CONN3        - Connectors for joining small sub-board to main board
      U100         - Empty 18 pin socket
*/

ROM_START( fmaniac3 )
	ROM_REGION32_BE( 0x2000000, REGION_USER1, ROMREGION_ERASE00 ) /* Hyperstone CPU Code & Data */
	/* 0 - 0x17fffff empty space */
	ROM_LOAD16_WORD_SWAP( "flash.u8", 0x1800000, 0x400000, CRC(dc08a224) SHA1(4d14145eb84ad13674296f81e90b9d60403fa0de) )
	ROM_LOAD16_WORD_SWAP( "flash.u9", 0x1c00000, 0x400000, CRC(c1fee95f) SHA1(0ed5ed9fa18e7da9242a6df2c210c46de25a2281) )

	ROM_REGION( 0x400000, REGION_CPU2, 0 ) /* sound rom */
	ROM_LOAD16_WORD_SWAP( "flash.u10", 0x000000, 0x400000, CRC(dfeb91a0) SHA1(a4a79073c3f6135957ea8a4a66a9c71a3a39893c) )
ROM_END

static DRIVER_INIT( xfiles )
{
	UINT8 *rom = (UINT8 *)memory_region(REGION_USER1) + 0x1c00000;

	rom[BYTE4_XOR_BE(0x3aa92e)] = 3;
	rom[BYTE4_XOR_BE(0x3aa92f)] = 0;
	rom[BYTE4_XOR_BE(0x3aa930)] = 3;
	rom[BYTE4_XOR_BE(0x3aa931)] = 0;
	rom[BYTE4_XOR_BE(0x3aa932)] = 3;
	rom[BYTE4_XOR_BE(0x3aa933)] = 0;

//  protection related ?
//  memory_install_read32_handler(0, ADDRESS_SPACE_PROGRAM, 0xf0c8b440, 0xf0c8b447, 0, 0, MRA32_NOP );

	flash_roms = 2;
}

static DRIVER_INIT( kdynastg )
{
	UINT8 *rom = (UINT8 *)memory_region(REGION_USER1) + 0x1c00000;

	rom[BYTE4_XOR_BE(0x3aaa10)] = 3; // 129f0 - nopped call
	rom[BYTE4_XOR_BE(0x3aaa11)] = 0;
	rom[BYTE4_XOR_BE(0x3aaa12)] = 3;
	rom[BYTE4_XOR_BE(0x3aaa13)] = 0;
	rom[BYTE4_XOR_BE(0x3aaa14)] = 3;
	rom[BYTE4_XOR_BE(0x3aaa15)] = 0;

	rom[BYTE4_XOR_BE(0x3a45c8)] = 5; // c5a8 - added ret
	rom[BYTE4_XOR_BE(0x3a45c9)] = 0;

//  protection related ?
//  memory_install_read32_handler(0, ADDRESS_SPACE_PROGRAM, 0x12341234, 0x12341243, 0, 0, MRA32_NOP );

	flash_roms = 4;
}

static DRIVER_INIT( fmaniac3 )
{
	flash_roms = 2;
}

GAME( 1999, xfiles,   0, dgpix, dgpix, xfiles,   ROT0, "dgPIX Entertainment Inc.", "X-Files",                           GAME_NO_SOUND )
GAME( 1999, kdynastg, 0, dgpix, dgpix, kdynastg, ROT0, "EZ Graphics",              "King of Dynast Gear (version 1.8)", GAME_NO_SOUND )
GAME( 2002, fmaniac3, 0, dgpix, dgpix, fmaniac3, ROT0, "Saero Entertainment",      "Fishing Maniac 3",                  GAME_NO_SOUND )
