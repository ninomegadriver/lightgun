/*
    Namco (Super) System 23
    Extremely preliminary driver by R. Belmont, thanks to Phil Stroffolino & Olivier Galibert

    Hardware: R4650 (MIPS III with IDT special instructions) main CPU @ 166 MHz
              H8/3002 MCU for sound/inputs
          Custom polygon hardware
      1 text tilemap
          Sprites?

    NOTES:
    - First 128k of main program ROM is the BIOS, and after that is a 64-bit MIPS ELF image.

    - Text layer is (almost?) identical to System 22 & Super System 22.

    TODO:
    - Text layer does not refresh properly in all cases, in spite of forced all-dirty.
      Why?

    - Palette is not right.  Games always write 16 bits at a time where the top 8 bits of each 16
      appear to always be 0 (some sort of gamma correct?).

    - H8/3002 does not handshake.  Protocol should be the same as System 12 where the MIPS
      writes 0x3163 to offset 0x3002 in the shared RAM and the H8/3002 notices and sets it to 0x7106.
      The H8 currently never reads that location (core bug?).

    - Hook up actual inputs via the 2 serial latches at d00004 and d00006.
      Works like this: write to d00004, then read d00004 12 times.  Ditto at
      d00006.  This gives 24 bits of inputs from the I/O board (?).

    - The entire 3D subsystem.  Is there a DSP living down there?  If not, why the 300k
      download on initial startup?

    - There are currently no differences seen between System 23 (Time Crisis 2) and
      Super System 23 (GP500, Final Furlong 2).  These will presumably appear when
      the 3D hardware is emulated.

    - Serial number data is at offset 0x201 in the BIOS.  Until the games are running
      and displaying it I'm not going to meddle with it though.
*/

/*

PCB Layouts
-----------

SYSTEM23 MAIN PCB 8660962302 8660971105 (8660961105)
|----------------------------------------------------------------------------|
|       J5                    3V_BATT                                        |
|                     LED1-8         *R4543     *MAX734          ADM485JR    |
|  |-------| LED10-11         LC35256                 CXA1779P  *3414 *3414  |
|  |H8/3002|          *2061ASC-1                                             |
|  |       |       SW4                                *LM358                 |
|  |-------|                    DS8921                *MB88347  *MB87078     |
|J18                                                  *LC78832  *LC78832     |
|              SW3  14.7456MHz                  |----|   |----|  CXD1178Q    |
|             |------| |----| |----||---------| |C435|   |C435|              |
|    N341256  | C416 | |C422| |IDT ||         | |----|   |----|              |
|             |      | |----| |7200||   C403  |                              |
|    N341256  |      |        |----||         |                 |---------|  |
|             |------|        |----||         | PAL(2)  N341256 |         |  |
|   |----| *PST575            |IDT ||---------|                 |  C417   |  |
|   |C352|           CY7C182  |7200|                            |         |  |
|   |----| LED9               |----|                    N341256 |         |  |
|                            J12                                |---------|  |
|     KM416S1020            |-------|   PAL(3)  M5M4V4265                    |
|                           |XILINX |                                        |
|J16                        |XC95108|                                     J17|
|     KM416S1020            |-------|       |---------|  |---------| N341256 |
|                                           |         |  |         |         |
|                                           |   C421  |  |   C404  | N341256 |
|       |---------|              N341256    |         |  |         |         |
|       |         |                         |         |  |         | N341256 |
|       |   C413  |              N341256    |---------|  |---------|         |
|       |         |                                                          |
|       |         |                           M5M4V4265                      |
|       |---------| SW2    LC321664                                          |
|               *PST575                                                      |
|                                                             *KM681000      |
|       |----------|    |---------|                        |-------------|   |
|       |NKK       |    |         |                        |             |   |
|       |NR4650-138|    |   C361  |           CY2291       |    C412     |   |
|J14    |          |    |         |                        |             |J15|
|       |          |    |         |           14.31818MHz  |             |   |
|       |----------|    |---------|  PAL(1)                |-------------|   |
|                                                             *KM681000      |
|                                                       HM5216165  HM5216165 |
|----------------------------------------------------------------------------|


SystemSuper23 MAIN(1) PCB 8672960904 8672960104 (8672970104)
|----------------------------------------------------------------------------|
|       J5                    3V_BATT                                        |
|                     LED1-8         *R4543     *MAX734          ADM485JR    |
|  |-------| LED10-11         LC35256                 CXA1779P   3414  3414  |
|  |H8/3002|          *2061ASC-1                                             |
|  |       |       SW4                                *LM358                 |
|  |-------|                    DS8921                *MB88347  *MB87078     |
|J18                                                  *LC78832  *LC78832     |
|               SW3  14.7456MHz                |----|    |----|  CXD1178Q    |
|              |------|  |----|   |---------|  |C435|    |C435|              |
|    N341256   | C416 |  |C422|   |         |  |----|    |----|              |
|              |      |  |----|   |   C444  |                                |
|    N341256   |      |           |         |                   |---------|  |
|              |------|           |         |  PAL(2) CY7C1399  |         |  |
|   |----| *PST575                |---------|                   |  C417   |  |
|   |C352|           CY7C182                                    |         |  |
|   |----| LED9                                       CY7C1399  |         |  |
|                                                               |---------|  |
|     KM416S1020           EPM7064      PAL(3)  KM416V2540                   |
|                                                                            |
|J16                                                                      J17|
|     KM416S1020                            |---------|  |---------| N341256 |
|                                CY7C1399   |         |  |         |         |
|                                           |   C421  |  |   C404  | N341256 |
|       |---------|              CY7C1399   |         |  |         |         |
|       |         |                         |         |  |         | N341256 |
|       |   C413  |                         |---------|  |---------|         |
|       |         |                                                          |
|       |         |                             KM416V2540                   |
|       |---------| SW2    LC321664                                          |
|               *PST575                                                      |
|                                                           *KM416S1020      |
|       |----------|    |---------|                        |-------------|   |
|       |NKK       |    |         |                        |             |   |
|       |NR4650-167|    |   C361  |           CY2291       |    C447     |   |
|J14    |          |    |         |                        |             |J15|
|       |          |    |         |           14.31818MHz  |             |   |
|       |----------|    |---------|  PAL(1)                |-------------|   |
|                                                           *KM416S1020      |
|                                                          71V124   71V124   |
|----------------------------------------------------------------------------|
Notes:
      * - These parts are underneath the PCB.

      Main Parts List:

      CPU
      ---
          NKK NR4650 - R4600-based 64bit RISC CPU (Main CPU, QFP208, clock input source = CY2291)
          H8/3002    - Hitachi H8/3002 HD6413002F17 (Sound CPU, QFP100, running at 14.7456MHz)

      RAM
      ---
          N341256    - NKK 32K x8 SRAM (x5, SOJ28)
          LC35256    - Sanyo 32K x8 SRAM (SOP28)
          KM416S1020 - Samsung 16M SDRAM (x4, TSOP48)
          KM416V2540 - Samsung 256K x16 EDO DRAM (x2, TSOP40/44)
          LC321664   - Sanyo 64K x16 EDO DRAM (SOJ40)
          71V124     - IDT 128K x8 SRAM (x2, SOJ32)
          CY7C1399   - Cypress 32K x8 SRAM (x4, SOJ28)
          CY7C182    - Cypress 8K x9 SRAM (SOJ28)
          M5M4V4265  - Mitsubishi 256K x16 DRAM (x2, TSOP40/44)
          HM5216165  - Hitachi 1M x16 SDRAM (x2, TSOP48)
          KM681000   - Samsung 128K x8 SRAM (x2, SOP32)

      Namco Customs
      -------------
                    C352 (QFP100)
                    C361 (QFP120)
                    C403 (QFP136)
                    C404 (QFP208)
                    C412 (QFP256)
                    C413 (QFP208)
                    C416 (QFP176)
                    C417 (QFP208)
                    C421 (QFP208)
                    C422 (QFP64)
                    C435 (x2, QFP144)
                    C444 (QFP136)
                    C447 (QFP256)

      Other ICs
      ---------
               XC95108  - Xilinx XC95108 In-System Programmable CPLD (QFP100, labelled 'S23MA9B')
               EPM7064  - Altera MAX EPM7064STC100-10 CPLD (TQFP100, labelled 'SS23MA4A')
               DS8921   - National RS422/423 Differential Line Driver and Receiver Pair (SOIC8)
               CXD1178Q - SONY CXD1178Q  8-bit RGB 3-channel D/A converter (QFP48)
               PAL(1)   - PALCE16V8H (PLCC20, stamped 'PAD23')
               PAL(2)   - PALCE22V10H (PLCC28, stamped 'S23MA5')
               PAL(3)   - PALCE22V10H (PLCC28, stamped 'SS23MA6B')
               PAL(1)   - PALCE16V8H (PLCC20, stamped 'SS23MA1B')
               PAL(2)   - PALCE22V10H (PLCC28, stamped 'SS23MA2A')
               PAL(3)   - PALCE22V10H (PLCC28, stamped 'SS23MA3A')
               MAX734   - MAX734 +12V 120mA Flash Memory Programming Supply Switching Regulator (SOIC8)
               PST575   - PST575 System Reset IC (SOIC4)
               3414     - NJM3414 70mA Dual Op Amp (x2, SOIC8)
               LM358    - National LM358 Low Power Dual Operational Amplifier (SOIC8)
               MB87078  - Fujitsu MB87078 Electronic Volume Control IC (SOIC24)
               MB88347  - Fujitsu MB88347 8bit 8 channel D/A converter with OP AMP output buffers (SOIC16)
               ADM485   - Analog Devices Low Power EIA RS485 transceiver (SOIC8)
               CXA1779P - SONY CXA1779P TV/Video circuit RGB Pre-Driver (DIP28)
               CY2291   - Cypress CY2291 Three-PLL General Purpose EPROM Programmable Clock Generator (SOIC20)
               2061ASC-1- IC Designs 2061ASC-1 clock generator IC (SOIC16, also found on Namco System 11 PCBs)
               R4543    - EPSON Real Time Clock Module (SOIC14)
               IDT7200  - Integrated Devices Technology IDT7200 256 x9 CMOS Asynchronous FIFO

      Misc
      ----
          J18   - Connector for MSPM(FRA) PCB
          J5    - Connector for EMI PCB
          J14 \
          J15 \
          J16 \
          J17 \ - Connectors for MEM(M) PCB
          SW2   - 2 position DIP Switch
          SW3   - 2 position DIP Switch
          SW4   - 8 position DIP Switch


Program ROM PCB
---------------

MSPM(FRA) PCB 8699017500 (8699017400)
|--------------------------|
|            J1            |
|                          |
|  IC3               IC1   |
|                          |
|                          |
|                    IC2   |
|--------------------------|
Notes:
      J1 -  Connector to plug into Main PCB
      IC1 \
      IC2 / Main Program  (Fujitsu 29F016 16MBit FlashROM, TSOP48)
      IC3 - Sound Program (Fujitsu 29F400T 4MBit FlashROM, TSOP48)

      Games that use this PCB include...

      Game           Code and revision
      --------------------------------
      Time Crisis 2  TSS3 Ver.B


MSPM(FRA) PCB 8699017501 (8699017401)
|--------------------------|
|            J1            |
|                          |
|  IC2               IC3   |
|                          |
|                          |
|  IC1                     |
|--------------------------|
Notes:
      J1 -  Connector to plug into Main PCB
      IC1 \
      IC2 / Main Program  (Fujitsu 29F016 16MBit FlashROM, TSOP48)
      IC3 - Sound Program (ST M29F400T 4MBit FlashROM, TSOP48)

      Games that use this PCB include...

      Game           Code and revision
      --------------------------------
      GP500          5GP3 Ver.C


ROM PCB
-------

Printed on the PCB      - 8660960601 (8660970601) SYSTEM23 MEM(M) PCB
Sticker (GP500)         - 8672961100
Sticker (Time Crisis 2) - 8660962302
|----------------------------------------------------------------------------|
| KEYCUS    MTBH.2M      CGLL.4M        CGLL.5M         CCRL.7M       PAL(3) |
|                                                                            |
|                                                                            |
|J1         MTAH.2J      CGLM.4K        CGLM.5K         CCRH.7K            J4|
|                                                                            |
|   PAL(4)                                            JP5                    |
|                        CGUM.4J        CGUM.5J       JP4                    |
|           MTAL.2H                                   JP3                    |
|                                                     JP2                    |
|                        CGUU.4F        CGUU.5F                              |
|                                                       CCRL.7F              |
|           MTBL.2F                                                          |
|                            PAL(1)    PAL(2)                                |
|                                                       CCRH.7E              |
|                                                                            |
|         JP1                                                                |
|                                                                            |
|       WAVEL.2C      PT3L.3C      PT2L.4C      PT1L.5C      PT0L.7C         |
|J2                                                                        J3|
|                                                                            |
|       WAVEH.2A      PT3H.3A      PT2H.4A      PT1H.5A      PT0H.7A         |
|                                                                            |
|                                                                            |
|----------------------------------------------------------------------------|
Notes:
      J1   \
      J2   \
      J3   \
      J4   \   - Connectors to main PCB
      JP1      - ROM size configuration jumper for WAVE ROMs. Set to 64M, alt. setting 32M
      JP2      - ROM size configuration jumper for CG ROMs. Set to 64M, alt. setting 32M
      JP3      - ROM size configuration jumper for CG ROMs. Set to 64M, alt. setting 32M
      JP4      - ROM size configuration jumper for CG ROMs. Set to 64M, alt. setting 32M
      JP5      - ROM size configuration jumper for CG ROMs. Set to 64M, alt. setting 32M
                 Other ROMs
                           CCRL - size fixed at 32M
                           CCRH - size fixed at 16M
                           PT*  - size fixed at 32M
                           MT*  - size fixed at 64M

      KEYCUS   - Mach211 CPLD (PLCC44)
      PAL(1)   - PALCE20V8H  (PLCC28, stamped 'SS22M2')  \ Both identical
      PAL(2)   - PALCE20V8H  (PLCC28, stamped 'SS22M2')  /
      PAL(3)   - PALCE16V8H  (PLCC20, stamped 'SS22M1')
      PAL(4)   - PALCE16V8H  (PLCC20, labelled 'SS23MM1')
                 Note this PAL is not populated when used on Super System 23

      All ROMs are SOP44 MaskROMs
      Note: ROMs at locations 7M, 7K, 5M, 5K, 5J & 5F are not included in the archive since they're copies of
            other ROMs which are included in the archive.
            Each ROM is stamped with the Namco game code, then the ROM-use code (such as CCRL, CCRH, PT* or MT*).

                           MaskROM
            Game           Code     Keycus
            -------------------------------
            GP500          5GP1     KC029
            Time Crisis 2  TSS1     KC010
        Final Furlong 2 ????     ?????
*/


#include "driver.h"
#include "cpu/mips/mips3.h"
#include "cpu/h83002/h83002.h"
#include "sound/c352.h"
#include <time.h>

static int ss23_vstat = 0, hstat = 0, vstate = 0;
static tilemap *bgtilemap;
static UINT32 *namcos23_textram, *namcos23_shared_ram;
static UINT32 *namcos23_charram;

static UINT16 nthword( const UINT32 *pSource, int offs )
{
	pSource += offs/2;
	return (pSource[0]<<((offs&1)*16))>>16;
}

static void TextTilemapGetInfo( int tile_index )
{
	UINT16 data = nthword( namcos23_textram,tile_index );
  /**
    * x---.----.----.---- blend
    * xxxx.----.----.---- palette select
    * ----.xx--.----.---- flip
    * ----.--xx.xxxx.xxxx code
    */
	SET_TILE_INFO( 0, data&0x03ff, data>>12, TILE_FLIPYX((data&0x0c00)>>10) );
	if( data&0x8000 )
	{
		tile_info.priority = 1;
	}
} /* TextTilemapGetInfo */

READ32_HANDLER( namcos23_textram_r )
{
	return namcos23_textram[offset];
}

WRITE32_HANDLER( namcos23_textram_w )
{
	COMBINE_DATA( &namcos23_textram[offset] );
//  tilemap_mark_tile_dirty( bgtilemap, offset*2 );
//  tilemap_mark_tile_dirty( bgtilemap, offset*2+1 );
}

VIDEO_START( ss23 )
{
	bgtilemap = tilemap_create( TextTilemapGetInfo,tilemap_scan_rows,TILEMAP_TRANSPARENT,16,16,64,64 );
	tilemap_set_transparent_pen( bgtilemap, 0xf );

	return 0;
}

#if 0
static double
Normalize( UINT32 data )
{
	data &=  0xffffff;
	if( data&0x800000 )
	{
		data |= 0xff000000;
	}
	return (INT32)data;
}

static void
DrawLine( mame_bitmap *bitmap, int x0, int y0, int x1, int y1 )
{
	if( x0>=0 && x0<bitmap->width &&
		x1>=0 && x1<bitmap->width &&
		y0>=0 && y0<bitmap->height &&
		y1>=0 && y1<bitmap->height )
	{
		int sx,sy,dy;
		if( x0>x1 )
		{
			int temp = x0;
			x0 = x1;
			x1 = temp;

			temp = y0;
			y0 = y1;
			y1 = temp;
		}

		if( x1>x0 )
		{
			sy = y0<<16;
			dy = ((y1-y0)<<16)/(x1-x0);
			for( sx=x0; sx<x1; sx++ )
			{
				UINT16 *pDest = (UINT16 *)bitmap->line[sy>>16];
				pDest[sx] = 1;
				sy += dy;
			}
		}
	}
} /* DrawLine */

static void
DrawPoly( mame_bitmap *bitmap, const UINT32 *pSource, int n, int bNew )
{
	UINT32 flags = *pSource++;
	UINT32 unk = *pSource++;
	UINT32 intensity = *pSource++;
	double x[4],y[4],z[4];
	int i;
	if( bNew )
	{
		printf( "polydata: 0x%08x 0x%08x 0x%08x\n", flags, unk, intensity );
	}
	for( i=0; i<n; i++ )
	{
		x[i] = Normalize(*pSource++);
		y[i] = Normalize(*pSource++);
		z[i] = Normalize(*pSource++);

		if( bNew )
		{
			printf( "\t(%f,%f,%f)\n", x[i], y[i], z[i] );
		}
	}
	for( i=0; i<n; i++ )
	{
		#define KDIST 0x400
		int j = (i+1)%n;
		double z0 = z[i]+KDIST;
		double z1 = z[j]+KDIST;

		if( z0>0 && z1>0 )
		{
			int x0 = bitmap->width*x[i]/z0 + bitmap->width/2;
			int y0 = bitmap->width*y[i]/z0 + bitmap->height/2;
			int x1 = bitmap->width*x[j]/z1 + bitmap->width/2;
			int y1 = bitmap->width*y[j]/z1 + bitmap->height/2;
			if( bNew )
			{
				printf( "[%d,%d]..[%d,%d]\n", x0,y0,x1,y1 );
			}
			DrawLine( bitmap, x0,y0,x1,y1 );
		}
	}
}
#endif

VIDEO_UPDATE( ss23 )
{
	fillbitmap(bitmap, get_black_pen(), cliprect);
	fillbitmap(priority_bitmap, 0, cliprect);

	tilemap_mark_all_tiles_dirty(bgtilemap);
	tilemap_draw( bitmap, cliprect, bgtilemap, 0/*flags*/, 0x1/*priority*/ ); /* opaque */

#if 0
	static int bNew = 1;
	static int code = 0x80;
	const UINT32 *pSource = (UINT32 *)memory_region(REGION_GFX4);

	pSource = pSource + pSource[code];

	fillbitmap( bitmap, 0, 0 );
	for(;;)
	{
		UINT32 opcode = *pSource++;
		int bDone = (opcode&0x10000);

		switch( opcode&0x0f00 )
		{
		case 0x0300:
			DrawPoly( bitmap, pSource, 3, bNew );
			pSource += 3 + 3*3;
			break;

		case 0x0400:
			DrawPoly( bitmap, pSource, 4, bNew );
			pSource += 3 + 4*3;
			break;

		default:
			printf( "unk opcode: 0x%x\n", opcode );
			bDone = 1;
			break;
		}


		if( bDone )
		{
			break;
		}
	}

	bNew = 0;

	if( keyboard_pressed(KEYCODE_SPACE) )
	{
		while( keyboard_pressed(KEYCODE_SPACE) ){}
		code++;
		bNew = 1;
	}
#endif
}

static const gfx_layout namcos23_cg_layout;

static WRITE32_HANDLER( s23_txtchar_w )
{
	COMBINE_DATA(&namcos23_charram[offset]	);

	decodechar( Machine->gfx[0],offset/32,(UINT8 *)namcos23_charram,&namcos23_cg_layout );

	tilemap_mark_all_tiles_dirty(bgtilemap);
}

static READ32_HANDLER( ss23_vstat_r )
{
	if (offset == 1)
	{
		hstat ^= 0xffffffff;
		return hstat;
	}

	vstate++;
	if (vstate >= 2)
	{
	ss23_vstat ^= 0xffffffff;
		vstate = 0;
	}
	return ss23_vstat;
}

static UINT8 nthbyte( const UINT32 *pSource, int offs )
{
	pSource += offs/4;
	return (pSource[0]<<((offs&3)*8))>>24;
}

INLINE void UpdatePalette( int entry )
{
         int j;

	for( j=0; j<1; j++ )
	{
		int which = (entry*2)+(j*2);
		int r = nthbyte(paletteram32,which+0x00001);
		int g = nthbyte(paletteram32,which+0x08001);
		int b = nthbyte(paletteram32,which+0x18001);
		palette_set_color( which,r,g,b );
	}
}

READ32_HANDLER( namcos23_paletteram_r )
{
	return paletteram32[offset];
}

/* each LONGWORD is 2 colors.  each OFFSET is 2 colors */

WRITE32_HANDLER( namcos23_paletteram_w )
{
	COMBINE_DATA( &paletteram32[offset] );

	UpdatePalette((offset % (0x8000/4))*2);
}

// must return this magic number
static READ32_HANDLER(s23_unk_r)
{
	return 0x008e008e;
}

// this & 8 and this & 4 are checked
// offset = 1 for magic latch
static READ32_HANDLER(sysctl_stat_r)
{
	if (offset == 1) return 0x0000ffff;	// all inputs in

	return 0xffffffff;
}

// as with System 22, we need to halt the MCU while checking shared RAM
static WRITE32_HANDLER( s23_mcuen_w )
{
	printf("mcuen_w: mask %08x, data %08x\n", mem_mask, data);
	if (mem_mask == 0xffff0000)
	{
		if (data)
		{
			logerror("S23: booting H8/3002\n");
			cpunum_set_input_line(1, INPUT_LINE_RESET, CLEAR_LINE);
		}
		else
		{
			logerror("S23: stopping H8/3002\n");
			cpunum_set_input_line(1, INPUT_LINE_RESET, ASSERT_LINE);
		}
	}
}

static ADDRESS_MAP_START( ss23_map, ADDRESS_SPACE_PROGRAM, 32 )
	AM_RANGE(0x00000000, 0x003fffff) AM_RAM
	AM_RANGE(0x02000000, 0x02000003) AM_READ( s23_unk_r )
	AM_RANGE(0x04400000, 0x0440ffff) AM_BASE(&namcos23_shared_ram)
	AM_RANGE(0x04c3ff08, 0x04c3ff0b) AM_WRITE( s23_mcuen_w )
	AM_RANGE(0x04c3ff0c, 0x04c3ff0f) AM_RAM				// 3d FIFO
	AM_RANGE(0x06800000, 0x06800fff) AM_RAM 			// text layer palette
	AM_RANGE(0x06800000, 0x06803fff) AM_WRITE( s23_txtchar_w ) AM_BASE(&namcos23_charram)	// text layer characters
	AM_RANGE(0x06804000, 0x0681dfff) AM_RAM
	AM_RANGE(0x0681e000, 0x0681ffff) AM_READ(namcos23_textram_r) AM_WRITE(namcos23_textram_w) AM_BASE(&namcos23_textram)
	AM_RANGE(0x06a08000, 0x06a2ffff) AM_READ(namcos23_paletteram_r) AM_WRITE(namcos23_paletteram_w) AM_BASE(&paletteram32)
	AM_RANGE(0x06a30000, 0x06a3ffff) AM_RAM
	AM_RANGE(0x06820008, 0x0682000f) AM_READ( ss23_vstat_r )	// vblank status?
	AM_RANGE(0x08000000, 0x08017fff) AM_RAM
	AM_RANGE(0x0d000000, 0x0d000007) AM_READ(sysctl_stat_r)
	AM_RANGE(0x0fc00000, 0x0fffffff) AM_WRITENOP AM_ROM AM_REGION(REGION_USER1, 0)
	AM_RANGE(0x1fc00000, 0x1fffffff) AM_WRITENOP AM_ROM AM_REGION(REGION_USER1, 0)
ADDRESS_MAP_END

static WRITE16_HANDLER( sharedram_sub_w )
{
	UINT16 *shared16 = (UINT16 *)namcos23_shared_ram;

	COMBINE_DATA(&shared16[BYTE_XOR_BE(offset)]);
}

static READ16_HANDLER( sharedram_sub_r )
{
	UINT16 *shared16 = (UINT16 *)namcos23_shared_ram;

	return shared16[BYTE_XOR_BE(offset)];
}

/* H8/3002 MCU stuff */
static ADDRESS_MAP_START( s23h8rwmap, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x07ffff) AM_READ(MRA16_ROM)
	AM_RANGE(0x080000, 0x08ffff) AM_READWRITE( sharedram_sub_r, sharedram_sub_w )
	AM_RANGE(0x280000, 0x287fff) AM_READWRITE( c352_0_r, c352_0_w )
	AM_RANGE(0x300000, 0x300001) AM_READNOP //( input_port_1_word_r )
	AM_RANGE(0x300002, 0x300003) AM_READNOP //( input_port_2_word_r )
	AM_RANGE(0x300010, 0x300011) AM_NOP
	AM_RANGE(0x300030, 0x300031) AM_NOP
ADDRESS_MAP_END

static READ8_HANDLER( s23_mcu_p8_r )
{
	return 0x02;
}

// emulation of the Epson R4543 real time clock
// in System 12, bit 0 of H8/3002 port A is connected to it's chip enable
// the actual I/O takes place through the H8/3002's serial port B.

static int s23_porta = 0, s23_rtcstate = 0;

static READ8_HANDLER( s23_mcu_pa_r )
{
	return s23_porta;
}

static WRITE8_HANDLER( s23_mcu_pa_w )
{
	// bit 0 = chip enable for the RTC
	// reset the state on the rising edge of the bit
	if ((!(s23_porta & 1)) && (data & 1))
	{
		s23_rtcstate = 0;
	}

	s23_porta = data;
}

INLINE UINT8 make_bcd(UINT8 data)
{
	return ((data / 10) << 4) | (data % 10);
}

static READ8_HANDLER( s23_mcu_rtc_r )
{
	UINT8 ret = 0;
	time_t curtime;
	struct tm *exptime;
	static int weekday[7] = { 7, 1, 2, 3, 4, 5, 6 };

	time(&curtime);
	exptime = localtime(&curtime);

	switch (s23_rtcstate)
	{
		case 0:
			ret = make_bcd(exptime->tm_sec);	// seconds (BCD, 0-59) in bits 0-6, bit 7 = battery low
			break;
		case 1:
			ret = make_bcd(exptime->tm_min);	// minutes (BCD, 0-59)
			break;
		case 2:
			ret = make_bcd(exptime->tm_hour);	// hour (BCD, 0-23)
			break;
		case 3:
			ret = make_bcd(weekday[exptime->tm_wday]); // day of the week (1 = Monday, 7 = Sunday)
			break;
		case 4:
			ret = make_bcd(exptime->tm_mday);	// day (BCD, 1-31)
			break;
		case 5:
			ret = make_bcd(exptime->tm_mon + 1);	// month (BCD, 1-12)
			break;
		case 6:
			ret = make_bcd(exptime->tm_year % 100);	// year (BCD, 0-99)
			break;
	}

	s23_rtcstate++;

	return ret;
}

static int s23_lastpB = 0x50, s23_setstate = 0, s23_setnum, s23_settings[8];

static READ8_HANDLER( s23_mcu_portB_r )
{
	s23_lastpB ^= 0x80;
	return s23_lastpB;
}

static WRITE8_HANDLER( s23_mcu_portB_w )
{
	// bit 7 = chip enable for the video settings controller
	if (data & 0x80)
	{
		s23_setstate = 0;
	}

	s23_lastpB = data;
}

static WRITE8_HANDLER( s23_mcu_settings_w )
{
	if (s23_setstate)
	{
		// data
		s23_settings[s23_setnum] = data;

		if (s23_setnum == 7)
		{
			logerror("S23 video settings: Contrast: %02x  R: %02x  G: %02x  B: %02x\n",
				BITSWAP8(s23_settings[0], 0, 1, 2, 3, 4, 5, 6, 7),
				BITSWAP8(s23_settings[1], 0, 1, 2, 3, 4, 5, 6, 7),
				BITSWAP8(s23_settings[2], 0, 1, 2, 3, 4, 5, 6, 7),
				BITSWAP8(s23_settings[3], 0, 1, 2, 3, 4, 5, 6, 7));
		}
	}
	else
	{	// setting number
		s23_setnum = (data >> 4)-1;
	}

	s23_setstate ^= 1;
}

static ADDRESS_MAP_START( s23h8iomap, ADDRESS_SPACE_IO, 8 )
	AM_RANGE(H8_PORT7, H8_PORT7) AM_READ( input_port_0_r )
	AM_RANGE(H8_PORT8, H8_PORT8) AM_READ( s23_mcu_p8_r ) AM_WRITENOP
	AM_RANGE(H8_PORTA, H8_PORTA) AM_READWRITE( s23_mcu_pa_r, s23_mcu_pa_w )
	AM_RANGE(H8_PORTB, H8_PORTB) AM_READWRITE( s23_mcu_portB_r, s23_mcu_portB_w )
	AM_RANGE(H8_SERIAL_B, H8_SERIAL_B) AM_READ( s23_mcu_rtc_r ) AM_WRITE( s23_mcu_settings_w )
	AM_RANGE(H8_ADC_0_H, H8_ADC_0_L) AM_NOP
	AM_RANGE(H8_ADC_1_H, H8_ADC_1_L) AM_NOP
	AM_RANGE(H8_ADC_2_H, H8_ADC_2_L) AM_NOP
	AM_RANGE(H8_ADC_3_H, H8_ADC_3_L) AM_NOP
ADDRESS_MAP_END

INPUT_PORTS_START( ss23 )
INPUT_PORTS_END


DRIVER_INIT(ss23)
{
    }

static const gfx_layout namcos23_cg_layout =
{
	16,16,
	0x400, /* 0x3c0 */
	4,
	{ 0,1,2,3 },
#ifdef LSB_FIRST
	{ 4*6,4*7,4*4,4*5,4*2,4*3,4*0,4*1,4*14,4*15,4*12,4*13,4*10,4*11,4*8,4*9 },
#else
	{ 4*0,4*1,4*2,4*3,4*4,4*5,4*6,4*7,4*8,4*9,4*10,4*11,4*12,4*13,4*14,4*15 },
#endif
	{ 64*0,64*1,64*2,64*3,64*4,64*5,64*6,64*7,64*8,64*9,64*10,64*11,64*12,64*13,64*14,64*15 },
	64*16
}; /* cg_layout */

#if 0
static const gfx_layout sprite_layout =
{
	32,32,
	RGN_FRAC(1,1),
	8,
	{ 0,1,2,3,4,5,6,7 },
	{
		0*8,1*8,2*8,3*8,4*8,5*8,6*8,7*8,
		8*8,9*8,10*8,11*8,12*8,13*8,14*8,15*8,
		16*8,17*8,18*8,19*8,20*8,21*8,22*8,23*8,
		24*8,25*8,26*8,27*8,28*8,29*8,30*8,31*8 },
	{
		0*32*8,1*32*8,2*32*8,3*32*8,4*32*8,5*32*8,6*32*8,7*32*8,
		8*32*8,9*32*8,10*32*8,11*32*8,12*32*8,13*32*8,14*32*8,15*32*8,
		16*32*8,17*32*8,18*32*8,19*32*8,20*32*8,21*32*8,22*32*8,23*32*8,
		24*32*8,25*32*8,26*32*8,27*32*8,28*32*8,29*32*8,30*32*8,31*32*8 },
	32*32*8
};
#endif

static const gfx_decode gfxdecodeinfo[] =
{
	{ 0, 0, &namcos23_cg_layout,  0, 0x80 },
	{ -1 },
};

static struct mips3_config config =
{
	8192,				/* code cache size - VERIFIED */
	8192				/* data cache size - VERIFIED */
};

static INTERRUPT_GEN( namcos23_interrupt )
{
}

static struct C352interface c352_interface =
{
	REGION_SOUND1
};

MACHINE_DRIVER_START( s23 )

	/* basic machine hardware */
	MDRV_CPU_ADD(R4650BE, 166000000)
	MDRV_CPU_CONFIG(config)
	MDRV_CPU_PROGRAM_MAP(ss23_map, 0)

	MDRV_CPU_ADD(H83002, 14745600 )
	MDRV_CPU_PROGRAM_MAP( s23h8rwmap, 0 )
	MDRV_CPU_IO_MAP( s23h8iomap, 0 )
	MDRV_CPU_VBLANK_INT( irq1_line_pulse, 1 );

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(64*16, 30*16)
	MDRV_VISIBLE_AREA(0, 64*16-1, 0, 30*16-1)
	MDRV_PALETTE_LENGTH(0x8000)

	MDRV_GFXDECODE(gfxdecodeinfo)

	MDRV_VIDEO_START(ss23)
	MDRV_VIDEO_UPDATE(ss23)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_STEREO("left", "right")

	MDRV_SOUND_ADD(C352, 14745600)
	MDRV_SOUND_CONFIG(c352_interface)
	MDRV_SOUND_ROUTE(0, "right", 1.00)
	MDRV_SOUND_ROUTE(1, "left", 1.00)
	MDRV_SOUND_ROUTE(2, "right", 1.00)
	MDRV_SOUND_ROUTE(3, "left", 1.00)
MACHINE_DRIVER_END

MACHINE_DRIVER_START( ss23 )

	/* basic machine hardware */
	MDRV_CPU_ADD(R4650BE, 166000000)
	MDRV_CPU_CONFIG(config)
	MDRV_CPU_PROGRAM_MAP(ss23_map, 0)
	MDRV_CPU_VBLANK_INT(namcos23_interrupt, 1)

	MDRV_CPU_ADD(H83002, 14745600 )
	MDRV_CPU_PROGRAM_MAP( s23h8rwmap, 0 )
	MDRV_CPU_IO_MAP( s23h8iomap, 0 )
	MDRV_CPU_VBLANK_INT( irq1_line_pulse, 1 );

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(48*16, 30*16)
	MDRV_VISIBLE_AREA(0, 48*16-1, 0, 30*16-1)
	MDRV_PALETTE_LENGTH(0x8000)

	MDRV_GFXDECODE(gfxdecodeinfo)

	MDRV_VIDEO_START(ss23)
	MDRV_VIDEO_UPDATE(ss23)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_STEREO("left", "right")

	MDRV_SOUND_ADD(C352, 14745600)
	MDRV_SOUND_CONFIG(c352_interface)
	MDRV_SOUND_ROUTE(0, "right", 1.00)
	MDRV_SOUND_ROUTE(1, "left", 1.00)
	MDRV_SOUND_ROUTE(2, "right", 1.00)
	MDRV_SOUND_ROUTE(3, "left", 1.00)
MACHINE_DRIVER_END

ROM_START( timecrs2 )
	ROM_REGION32_BE( 0x400000, REGION_USER1, 0 ) /* 4 megs for main R4650 code */
        ROM_LOAD16_BYTE( "tss3verb.2",   0x000000, 0x200000, CRC(c7be691f) SHA1(5e2e7a0db3d8ce6dfeb6c0d99e9fe6a9f9cab467) )
        ROM_LOAD16_BYTE( "tss3verb.1",   0x000001, 0x200000, CRC(6e3f232b) SHA1(8007d8f31a605a5df89938d7c9f9d3d209c934be) )

	ROM_REGION( 0x80000, REGION_CPU2, 0 )	/* Hitachi H8/3002 MCU code */
        ROM_LOAD16_WORD_SWAP( "tss3verb.3",   0x000000, 0x080000, CRC(41e41994) SHA1(eabc1a307c329070bfc6486cb68169c94ff8a162) )

	ROM_REGION( 0x2000000, REGION_GFX1, 0 )	/* sprite? tilemap? tiles */
        ROM_LOAD16_BYTE( "tss1mtal.2h",  0x0000000, 0x800000, CRC(bfc79190) SHA1(04bda00c4cc5660d27af4f3b0ee3550dea8d3805) )
        ROM_LOAD16_BYTE( "tss1mtah.2j",  0x0000001, 0x800000, CRC(697c26ed) SHA1(72f6f69e89496ba0c6183b35c3bde71f5a3c721f) )
        ROM_LOAD16_BYTE( "tss1mtbl.2f",  0x1000000, 0x800000, CRC(e648bea4) SHA1(3803d03e72b25fbcc124d5b25066d25629b76b94) )
        ROM_LOAD16_BYTE( "tss1mtbh.2m",  0x1000001, 0x800000, CRC(82582776) SHA1(7c790d09bac660ea1c62da3ffb21ab43f2461594) )

	ROM_REGION( 0x2000000, REGION_GFX2, 0 )	/* texture tiles */
        ROM_LOAD( "tss1cguu.4f",  0x0000000, 0x800000, CRC(76924e04) SHA1(751065d6ce658cbbcd88f854f6937ebd2204ec68) )
        ROM_LOAD( "tss1cgum.4j",  0x0800000, 0x800000, CRC(c22739e1) SHA1(8671ee047bb248033656c50befd1c35e5e478e1a) )
        ROM_LOAD( "tss1cgll.4m",  0x1000000, 0x800000, CRC(18433aaa) SHA1(08539beb2e66ec4e41062621fc098b121c669546) )
        ROM_LOAD( "tss1cglm.4k",  0x1800000, 0x800000, CRC(669974c2) SHA1(cfebe199631e38f547b38fcd35f1645b74e8dd0a) )

	ROM_REGION( 0x600000, REGION_GFX3, 0 )	/* texture tilemap */
        ROM_LOAD( "tss1ccrl.7f",  0x000000, 0x400000, CRC(3a325fe7) SHA1(882735dce7aeb36f9e88a983498360f5de901e9d) )
        ROM_LOAD( "tss1ccrh.7e",  0x400000, 0x200000, CRC(f998de1a) SHA1(371f540f505608297c5ffcfb623b983ca8310afb) )

	ROM_REGION32_LE( 0x2000000, REGION_GFX4, 0 )	/* 3D model data */
        ROM_LOAD32_WORD( "tss1pt0l.7c",  0x0000000, 0x400000, CRC(896f0fb4) SHA1(bdfa99eb21ce4fc8021f9d95a5558a34f9942c57) )
        ROM_LOAD32_WORD( "tss1pt0h.7a",  0x0000002, 0x400000, CRC(cdbe0ba8) SHA1(f8c6da31654c0a2a8024888ffb7fc1c783b2d629) )
        ROM_LOAD32_WORD( "tss1pt1l.5c",  0x0800000, 0x400000, CRC(5a09921f) SHA1(c23885708c7adf0b81c2c9346e21b869634a5b35) )
        ROM_LOAD32_WORD( "tss1pt1h.5a",  0x0800002, 0x400000, CRC(63647596) SHA1(833412be8f61686bd7e06c2738df740e0e585d0f) )
        ROM_LOAD32_WORD( "tss1pt2l.4c",  0x1000000, 0x400000, CRC(4b230d79) SHA1(794cee0a19993e90913f58507c53224f361e9663) )
        ROM_LOAD32_WORD( "tss1pt2h.4a",  0x1000002, 0x400000, CRC(9b06e22d) SHA1(cff5ed098112a4f0a2bc8937e226f50066e605b1) )

	ROM_REGION( 0x1000000, REGION_SOUND1, 0 ) /* C352 PCM samples */
        ROM_LOAD( "tss1wavel.2c", 0x000000, 0x800000, CRC(deaead26) SHA1(72dac0c3f41d4c3c290f9eb1b50236ae3040a472) )
        ROM_LOAD( "tss1waveh.2a", 0x800000, 0x800000, CRC(5c8758b4) SHA1(b85c8f6869900224ef83a2340b17f5bbb2801af9) )
ROM_END

ROM_START( gp500 )
	/* r4650-generic-xrom-generic: NMON 1.0.8-sys23-19990105 P for SYSTEM23 P1 */
	ROM_REGION32_BE( 0x400000, REGION_USER1, 0 ) /* 4 megs for main R4650 code */
        ROM_LOAD16_BYTE( "5gp3verc.2",   0x000000, 0x200000, CRC(e2d43468) SHA1(5e861dd223c7fa177febed9803ac353cba18e19d) )
        ROM_LOAD16_BYTE( "5gp3verc.1",   0x000001, 0x200000, CRC(f6efc94a) SHA1(785eee2bec5080d4e8ef836f28d446328c942b0e) )

	ROM_REGION( 0x80000, REGION_CPU2, 0 )	/* Hitachi H8/3002 MCU code */
        ROM_LOAD16_WORD_SWAP( "5gp3verc.3",   0x000000, 0x080000, CRC(b323abdf) SHA1(8962e39b48a7074a2d492afb5db3f5f3e5ae2389) )

	ROM_REGION( 0x2000000, REGION_GFX1, 0 )	/* sprite? tilemap? tiles */
	ROM_LOAD16_BYTE( "5gp1mtal.2h",  0x0000000, 0x800000, CRC(1bb00c7b) SHA1(922be45d57330c31853b2dc1642c589952b09188) )
        ROM_LOAD16_BYTE( "5gp1mtah.2j",  0x0000001, 0x800000, CRC(246e4b7a) SHA1(75743294b8f48bffb84f062febfbc02230d49ce9) )

		/* COMMON FUJII YASUI WAKAO KURE INOUE
         * 0x000000..0x57ffff: all 0xff
         */
        ROM_LOAD16_BYTE( "5gp1mtbl.2f",  0x1000000, 0x800000, CRC(66640606) SHA1(c69a0219748241c49315d7464f8156f8068e9cf5) )
        ROM_LOAD16_BYTE( "5gp1mtbh.2m",  0x1000001, 0x800000, CRC(352360e8) SHA1(d621dfac3385059c52d215f6623901589a8658a3) )

	ROM_REGION( 0x2000000, REGION_GFX2, 0 )	/* texture tiles */
        ROM_LOAD( "5gp1cguu.4f",  0x0000000, 0x800000, CRC(c411163b) SHA1(ae644d62357b8b806b160774043e41908fba5d05) )
        ROM_LOAD( "5gp1cgum.4j",  0x0800000, 0x800000, CRC(0265b701) SHA1(497a4c33311d3bb315100a78400cf2fa726f1483) )
        ROM_LOAD( "5gp1cgll.4m",  0x1000000, 0x800000, CRC(0cc5bf35) SHA1(b75510a94fa6b6d2ed43566e6e84c7ae62f68194) )
        ROM_LOAD( "5gp1cglm.4k",  0x1800000, 0x800000, CRC(31557d48) SHA1(b85c3db20b101ba6bdd77487af67c3324bea29d5) )

	ROM_REGION( 0x600000, REGION_GFX3, 0 )	/* texture tilemap */
        ROM_LOAD( "5gp1ccrl.7f",  0x000000, 0x400000, CRC(e7c77e1f) SHA1(0231ddbe2afb880099dfe2657c41236c74c730bb) )
        ROM_LOAD( "5gp1ccrh.7e",  0x400000, 0x200000, CRC(b2eba764) SHA1(5e09d1171f0afdeb9ed7337df1dbc924f23d3a0b) )

	ROM_REGION32_LE( 0x2000000, REGION_GFX4, 0 )	/* 3D model data */
        ROM_LOAD32_WORD( "5gp1pt0l.7c",  0x0000000, 0x400000, CRC(a0ece0a1) SHA1(b7aab2d78e1525f865214c7de387ccd585de5d34) )
        ROM_LOAD32_WORD( "5gp1pt0h.7a",  0x0000002, 0x400000, CRC(5746a8cd) SHA1(e70fc596ab9360f474f716c73d76cb9851370c76) )

        ROM_LOAD32_WORD( "5gp1pt1l.5c",  0x0800000, 0x400000, CRC(80b25ad2) SHA1(e9a03fe5bb4ce925f7218ab426ed2a1ca1a26a62) )
        ROM_LOAD32_WORD( "5gp1pt1h.5a",  0x0800002, 0x400000, CRC(b1feb5df) SHA1(45db259215511ac3e472895956f70204d4575482) )

	ROM_LOAD32_WORD( "5gp1pt2l.4c",  0x1000000, 0x400000, CRC(9289dbeb) SHA1(ec546ad3b1c90609591e599c760c70049ba3b581) )
        ROM_LOAD32_WORD( "5gp1pt2h.4a",  0x1000002, 0x400000, CRC(9a693771) SHA1(c988e04cd91c3b7e75b91376fd73be4a7da543e7) )

	ROM_LOAD32_WORD( "5gp1pt3l.3c",  0x1800000, 0x400000, CRC(480b120d) SHA1(6c703550faa412095d9633cf508050614e15fbae) )
        ROM_LOAD32_WORD( "5gp1pt3h.3a",  0x1800002, 0x400000, CRC(26eaa400) SHA1(0157b76fffe81b40eb970e84c98398807ced92c4) )

	ROM_REGION( 0x1000000, REGION_SOUND1, 0 ) /* C352 PCM samples */
        ROM_LOAD( "5gp1wavel.2c", 0x000000, 0x800000, CRC(aa634cc2) SHA1(e96f5c682039bc6ef22bf90e98f4da78486bd2b1) )
        ROM_LOAD( "5gp1waveh.2a", 0x800000, 0x800000, CRC(1e3523e8) SHA1(cb3d0d389fcbfb728fad29cfc36ef654d28d553a) )
ROM_END

ROM_START( finfurl2 )
	ROM_REGION32_BE( 0x400000, REGION_USER1, 0 ) /* 4 megs for main R4650 code */
        ROM_LOAD16_BYTE( "29f016.ic2",   0x000000, 0x200000, CRC(13cbc545) SHA1(3e67a7bfbb1c1374e8e3996a0c09e4861b0dca14) )
        ROM_LOAD16_BYTE( "29f016.ic1",   0x000001, 0x200000, CRC(5b04e4f2) SHA1(8099fc3deab9ed14a2484a774666fbd928330de8) )

	ROM_REGION( 0x80000, REGION_CPU2, 0 )	/* Hitachi H8/3002 MCU code */
        ROM_LOAD16_WORD_SWAP( "m29f400.ic3",  0x000000, 0x080000, CRC(9fd69bbd) SHA1(53a9bf505de70495dcccc43fdc722b3381aad97c) )

	ROM_REGION( 0x2000000, REGION_GFX1, 0 )	/* sprite? tilemap? tiles */
        ROM_LOAD16_BYTE( "ffs1mtal.2h",  0x0000000, 0x800000, CRC(98730ad5) SHA1(9ba276ad88ec8730edbacab80cdacc34a99593e4) )
        ROM_LOAD16_BYTE( "ffs1mtah.2j",  0x0000001, 0x800000, CRC(f336d81d) SHA1(a9177091e1412dea1b6ea6c53530ae31361b32d0) )
        ROM_LOAD16_BYTE( "ffs1mtbl.2f",  0x1000000, 0x800000, CRC(0abc9e50) SHA1(be5e5e2b637811c59804ef9442c6da5a5a1315e2) )
        ROM_LOAD16_BYTE( "ffs1mtbh.2m",  0x1000001, 0x800000, CRC(0f42c93b) SHA1(26b313fc5c33afb0a1ee42243486e38f052c95c2) )

	ROM_REGION( 0x2000000, REGION_GFX2, 0 )	/* texture tiles */
        ROM_LOAD( "ffs1cguu.4f",  0x0000000, 0x800000, CRC(52c0a19f) SHA1(e6b4b90ff88da09cb2e653e450e7ae66942a719e) )
        ROM_LOAD( "ffs1cgum.4j",  0x0800000, 0x800000, CRC(77447199) SHA1(1eeae30b3dd1ac467bdbbdfe4be36ca0f0816496) )
        ROM_LOAD( "ffs1cgll.4m",  0x1000000, 0x800000, CRC(171bba76) SHA1(4a63a1f34de8f341a0ef9b499a21e8fec758e1cd) )
        ROM_LOAD( "ffs1cglm.4k",  0x1800000, 0x800000, CRC(48acf207) SHA1(ea902efdd94aba34dadb20762219d2d25441d199) )

	ROM_REGION( 0x600000, REGION_GFX3, 0 )	/* texture tilemap */
        ROM_LOAD( "ffs1ccrl.7f",  0x000000, 0x200000, CRC(ffbcfec1) SHA1(9ab25f1543da4b72784eec93985abaa2e1dafc83) )
        ROM_LOAD( "ffs1ccrh.7e",  0x200000, 0x200000, CRC(8be4aeb4) SHA1(ec344f6fba42092083e737e436451f5d7be12c15) )

	ROM_REGION32_LE( 0x2000000, REGION_GFX4, 0 )	/* 3D model data */
        ROM_LOAD32_WORD( "ffs1pt0l.7c",  0x0000000, 0x400000, CRC(383cbfba) SHA1(0784ac2d709bee6653c95f80fedf7f98ca79357f) )
        ROM_LOAD32_WORD( "ffs1pt0h.7a",  0x0000002, 0x400000, CRC(79b9b019) SHA1(ca2bbabd949fec91001a30b63f7343520028cde0) )
        ROM_LOAD32_WORD( "ffs1pt1l.5c",  0x0800000, 0x400000, CRC(ba0fff5b) SHA1(d5a6db4de60657d46228e85ed09ed7f0ecbc7975) )
        ROM_LOAD32_WORD( "ffs1pt1h.5a",  0x0800002, 0x400000, CRC(2dba59d0) SHA1(34d4c415b5635338511ff3578eb3c00e2b6cd7d4) )
        ROM_LOAD32_WORD( "ffs1pt2l.4c",  0x1000000, 0x400000, CRC(c5199c1b) SHA1(8f1a70c8edb2791a099b4911353af6250a5d0e8a) )
        ROM_LOAD32_WORD( "ffs1pt2h.4a",  0x1000002, 0x400000, CRC(26ea01e8) SHA1(9af096c99e6835e21b1b78dfce07040f50f8c922) )
        ROM_LOAD32_WORD( "ffs1pt3l.3c",  0x1800000, 0x400000, CRC(2381611a) SHA1(a3d948bf910dcfd9f47c65c56b9920f58c42fed5) )
        ROM_LOAD32_WORD( "ffs1pt3h.3a",  0x1800002, 0x400000, CRC(48226e9f) SHA1(f099b2929d49903a33b4dab80972c3ce0ddb6ca2) )

	ROM_REGION( 0x1000000, REGION_SOUND1, 0 ) /* C352 PCM samples */
        ROM_LOAD( "ffs1wavel.2c", 0x000000, 0x800000, CRC(67ba16cf) SHA1(00b38617c2185b9a3bf279962ad0c21a7287256f) )
        ROM_LOAD( "ffs1waveh.2a", 0x800000, 0x800000, CRC(178e8bd3) SHA1(8ab1a97003914f70b09e96c5924f3a839fe634c7) )
ROM_END

ROM_START( finfrl2j )
	ROM_REGION32_BE( 0x400000, REGION_USER1, 0 ) /* 4 megs for main R4650 code */
        ROM_LOAD16_BYTE( "29f016_jap1.ic2", 0x000000, 0x200000, CRC(0215125d) SHA1(a99f601441c152b0b00f4811e5752c71897b1ed4) )
        ROM_LOAD16_BYTE( "29f016_jap1.ic1", 0x000001, 0x200000, CRC(38c9ae96) SHA1(b50afc7276662267ff6460f82d0e5e8b00b341ea) )

	ROM_REGION( 0x80000, REGION_CPU2, 0 )	/* Hitachi H8/3002 MCU code */
        ROM_LOAD16_WORD_SWAP( "m29f400.ic3",  0x000000, 0x080000, CRC(9fd69bbd) SHA1(53a9bf505de70495dcccc43fdc722b3381aad97c) )

	ROM_REGION( 0x2000000, REGION_GFX1, 0 )	/* sprite? tilemap? tiles */
        ROM_LOAD16_BYTE( "ffs1mtal.2h",  0x0000000, 0x800000, CRC(98730ad5) SHA1(9ba276ad88ec8730edbacab80cdacc34a99593e4) )
        ROM_LOAD16_BYTE( "ffs1mtah.2j",  0x0000001, 0x800000, CRC(f336d81d) SHA1(a9177091e1412dea1b6ea6c53530ae31361b32d0) )
        ROM_LOAD16_BYTE( "ffs1mtbl.2f",  0x1000000, 0x800000, CRC(0abc9e50) SHA1(be5e5e2b637811c59804ef9442c6da5a5a1315e2) )
        ROM_LOAD16_BYTE( "ffs1mtbh.2m",  0x1000001, 0x800000, CRC(0f42c93b) SHA1(26b313fc5c33afb0a1ee42243486e38f052c95c2) )

	ROM_REGION( 0x2000000, REGION_GFX2, 0 )	/* texture tiles */
        ROM_LOAD( "ffs1cguu.4f",  0x0000000, 0x800000, CRC(52c0a19f) SHA1(e6b4b90ff88da09cb2e653e450e7ae66942a719e) )
        ROM_LOAD( "ffs1cgum.4j",  0x0800000, 0x800000, CRC(77447199) SHA1(1eeae30b3dd1ac467bdbbdfe4be36ca0f0816496) )
        ROM_LOAD( "ffs1cgll.4m",  0x1000000, 0x800000, CRC(171bba76) SHA1(4a63a1f34de8f341a0ef9b499a21e8fec758e1cd) )
        ROM_LOAD( "ffs1cglm.4k",  0x1800000, 0x800000, CRC(48acf207) SHA1(ea902efdd94aba34dadb20762219d2d25441d199) )

	ROM_REGION( 0x600000, REGION_GFX3, 0 )	/* texture tilemap */
        ROM_LOAD( "ffs1ccrl.7f",  0x000000, 0x200000, CRC(ffbcfec1) SHA1(9ab25f1543da4b72784eec93985abaa2e1dafc83) )
        ROM_LOAD( "ffs1ccrh.7e",  0x200000, 0x200000, CRC(8be4aeb4) SHA1(ec344f6fba42092083e737e436451f5d7be12c15) )

	ROM_REGION32_LE( 0x2000000, REGION_GFX4, 0 )	/* 3D model data */
        ROM_LOAD32_WORD( "ffs1pt0l.7c",  0x0000000, 0x400000, CRC(383cbfba) SHA1(0784ac2d709bee6653c95f80fedf7f98ca79357f) )
        ROM_LOAD32_WORD( "ffs1pt0h.7a",  0x0000002, 0x400000, CRC(79b9b019) SHA1(ca2bbabd949fec91001a30b63f7343520028cde0) )
        ROM_LOAD32_WORD( "ffs1pt1l.5c",  0x0800000, 0x400000, CRC(ba0fff5b) SHA1(d5a6db4de60657d46228e85ed09ed7f0ecbc7975) )
        ROM_LOAD32_WORD( "ffs1pt1h.5a",  0x0800002, 0x400000, CRC(2dba59d0) SHA1(34d4c415b5635338511ff3578eb3c00e2b6cd7d4) )
        ROM_LOAD32_WORD( "ffs1pt2l.4c",  0x1000000, 0x400000, CRC(c5199c1b) SHA1(8f1a70c8edb2791a099b4911353af6250a5d0e8a) )
        ROM_LOAD32_WORD( "ffs1pt2h.4a",  0x1000002, 0x400000, CRC(26ea01e8) SHA1(9af096c99e6835e21b1b78dfce07040f50f8c922) )
        ROM_LOAD32_WORD( "ffs1pt3l.3c",  0x1800000, 0x400000, CRC(2381611a) SHA1(a3d948bf910dcfd9f47c65c56b9920f58c42fed5) )
        ROM_LOAD32_WORD( "ffs1pt3h.3a",  0x1800002, 0x400000, CRC(48226e9f) SHA1(f099b2929d49903a33b4dab80972c3ce0ddb6ca2) )

	ROM_REGION( 0x1000000, REGION_SOUND1, 0 ) /* C352 PCM samples */
        ROM_LOAD( "ffs1wavel.2c", 0x000000, 0x800000, CRC(67ba16cf) SHA1(00b38617c2185b9a3bf279962ad0c21a7287256f) )
        ROM_LOAD( "ffs1waveh.2a", 0x800000, 0x800000, CRC(178e8bd3) SHA1(8ab1a97003914f70b09e96c5924f3a839fe634c7) )
ROM_END

/* Games */
GAME( 1997, timecrs2, 0,         s23, ss23, ss23, ROT0, "Namco", "Time Crisis 2", GAME_NOT_WORKING | GAME_UNEMULATED_PROTECTION | GAME_IMPERFECT_SOUND )
GAME( 1999, gp500,    0,        ss23, ss23, ss23, ROT0, "Namco", "GP500", GAME_NOT_WORKING | GAME_UNEMULATED_PROTECTION | GAME_IMPERFECT_SOUND )
GAME( 1999, finfurl2, 0,        ss23, ss23, ss23, ROT0, "Namco", "Final Furlong 2 (World)", GAME_NOT_WORKING | GAME_UNEMULATED_PROTECTION | GAME_IMPERFECT_SOUND )
GAME( 1999, finfrl2j, finfurl2, ss23, ss23, ss23, ROT0, "Namco", "Final Furlong 2 (Japan)", GAME_NOT_WORKING | GAME_UNEMULATED_PROTECTION | GAME_IMPERFECT_SOUND )
