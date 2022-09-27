/*  Konami Hornet System

    Driver by Ville Linde


    Hardware configurations:

    Game            | ID        | CPU PCB      | CG Board(s)   | LAN PCB
    ---------------------------------------------------------------------------
    Gradius 4       | GX837     | GN715(A)     | GN715(B)      |
    Silent Scope    | GQ830     | GN715(A)     | 2x GN715(B)   |
    Silent Scope 2  | GQ931     | GN715(A)     | 2x GQ871(B)   | GQ931(H)




    Top Board GN715 PWB(A)A
    |--------------------------------------------------------------|
    |                                                              |
    |                                                        PAL   |
    |               PAL               68EC000          837A08.7S   |
    |NE5532         PAL                                            |
    |                         DRM1M4SJ8                            |
    |NE5532                                                        |
    |     SM5877              RF5C400                              |
    |                                                              |
    |     SM5877     16.9344MHz                                    |
    |                SRAM256K               837A10.14P  837A05.14T |
    |                                                              |
    |                SRAM256K               837A09.16P  837A04.16T |
    |  ADC12138                                                    |
    |             056800                                           |
    |                                                              |
    |                      MACH111                                 |
    |                                                              |
    |                      DRAM16X16                      PPC403GA |
    |                                   837C01.27P                 |
    |                                                              |
    |                      DRAM16X16                               |
    | 4AK16                                                        |
    |                                                              |
    |                                                              |
    | 0038323  PAL                                        7.3728MHz|
    | E9825                                                        |
    |                                                     50.000MHz|
    |                                                              |
    |M48T58Y-70PC1                                        64.000MHz|
    |--------------------------------------------------------------|

    Notes:
        DRM1M4SJ8 = Fujitsu 81C4256 DRAM (SOJ24)
        SRAM256K = Cypress CY7C199 SRAM (SOJ28)
        DRAM16X16 = Fujitsu 8118160A-60 DRAM (SOJ42)
        0038323 E9825 = SOIC8, I've seen a similar chip in the security cart of System573
        M48T58Y-70PC1 = ST Timekeeper RAM


    Bottom Board GN715 PWB(B)A
    |--------------------------------------------------------------|
    |                                                              |
    |JP1                                          4M EDO   4M EDO  |
    |                                                              |
    |  4M EDO 4M EDO      TEXELFX                                  |
    |                                                       4M EDO |
    |  4M EDO 4M EDO                  PIXELFX               4M EDO |
    |                                                              |
    |  4M EDO 4M EDO                                KONAMI         |
    |                                   50MHz       0000033906     |
    |  4M EDO 4M EDO                                               |
    |                       256KSRAM 256KSRAM                      |
    |                                                              |
    |         AV9170                     1MSRAM 1MSRAM             |
    | MC44200                                                      |
    |                       256KSRAM 256KSRAM                      |
    |                                    1MSRAM 1MSRAM             |
    |                                               837A13.24U     |
    |  KONAMI    MACH111                                 837A15.24V|
    |  0000037122                        1MSRAM 1MSRAM             |
    |                       ADSP-21062                             |
    |                       SHARC    36.00MHz                      |
    |1MSRAM                              1MSRAM 1MSRAM             |
    |                                                              |
    |1MSRAM                  PAL  PAL                              |
    |           256KSRAM                            837A14.32U     |
    |1MSRAM     256KSRAM                                 837A16.32V|
    |           256KSRAM                                           |
    |1MSRAM                                                        |
    |           JP2                                                |
    |--------------------------------------------------------------|

    Notes:
        4M EDO = SM81C256K16CJ-35 RAM 66MHz
        1MSRAM = CY7C109-25VC
        256KSRAM = W24257AJ-15
        TEXELFX = 3DFX 500-0004-02 BD0665.1 TMU (QFP208)
        PIXELFX = 3DFX 500-0003-03 F001701.1 FBI (QFP240)
        JP1 = Jumper set to SCR. Alt. setting is TWN
        JP2 = Jumper set for MASTER, Alt. setting SLAVE



    LAN PCB: GQ931 PWB(H)      (C) 1999 Konami
    ------------------------------------------

    2 x LAN ports, LANC(1) & LANC(2)
    1 x 32.0000MHz Oscillator

         HYC2485S  SMC ARCNET Media Transceiver, RS485 5Mbps-2.5Mbps
    8E   931A19    Konami 32meg masked ROM, ROM0 (compressed GFX data?)
    6E   931A20    Konami 32meg masked ROM, ROM1 (compressed GFX data?)
    12F  XC9536    Xilinx  CPLD,  44 pin PLCC, Konami no. Q931H1
    12C  XCS10XL   Xilinx  FPGA, 100 pin PQFP, Konami no. 4C
    12B  CY7C199   Cypress 32kx8 SRAM
    8B   AT93C46   Atmel 1K serial EEPROM, 8 pin SOP
    16G  DS2401    Dallas Silicon Serial Number IC, 6 pin SOP



    GFX PCB:  GQ871 PWB(B)A    (C) 1999 Konami
    ------------------------------------------

    There are no ROMs on the two GFX PCBs, all sockets are empty.
    Prior to the game starting there is a message saying downloading data.


    Jumpers set on GFX PCB to main monitor:
    4A   set to TWN (twin GFX PCBs)
    37D  set to Master


    Jumpers set on GFX PCB to scope monitor:
    4A   set to TWN (twin GFX PCBs)
    37D  set to Slave



    1 x 64.0000MHz
    1 x 36.0000MHz  (to 27L, ADSP)

    21E  AV9170           ICS, Clock synchroniser and multiplier

    27L  ADSP-21062       Analog Devices SHARC ADSP, 512k flash, Konami no. 022M16C
    15T  0000033906       Konami Custom, 160 pin PQFP
    19R  W241024AI-20     Winbond, 1Meg SRAM
    22R  W241024AI-20     Winbond, 1Meg SRAM
    25R  W241024AI-20     Winbond, 1Meg SRAM
    29R  W241024AI-20     Winbond, 1Meg SRAM
    19P  W241024AI-20     Winbond, 1Meg SRAM
    22P  W241024AI-20     Winbond, 1Meg SRAM
    25P  W241024AI-20     Winbond, 1Meg SRAM
    29P  W241024AI-20     Winbond, 1Meg SRAM
    18N  W24257AJ-15      Winbond, 256K SRAM
    14N  W24257AJ-15      Winbond, 256K SRAM
    18M  W24257AJ-15      Winbond, 256K SRAM
    14M  W24257AJ-15      Winbond, 256K SRAM

    28D  000037122        Konami Custom, 208 pin PQFP
    33E  W24257AJ-15      Winbond, 256K SRAM
    33D  W24257AJ-15      Winbond, 256K SRAM
    33C  W24257AJ-15      Winbond, 256K SRAM
    27A  W241024AI-20     Winbond, 1Meg SRAM
    30A  W241024AI-20     Winbond, 1Meg SRAM
    32A  W241024AI-20     Winbond, 1Meg SRAM
    35A  W241024AI-20     Winbond, 1Meg SRAM

    7K   500-0010-01      3DFX, Texture processor
    16F  SM81C256K16CJ-25 Silicon Magic 100MHz EDO RAM, 4Meg
    13F  SM81C256K16CJ-25 Silicon Magic 100MHz EDO RAM, 4Meg
    9F   SM81C256K16CJ-25 Silicon Magic 100MHz EDO RAM, 4Meg
    5F   SM81C256K16CJ-25 Silicon Magic 100MHz EDO RAM, 4Meg
    16D  SM81C256K16CJ-25 Silicon Magic 100MHz EDO RAM, 4Meg
    13D  SM81C256K16CJ-25 Silicon Magic 100MHz EDO RAM, 4Meg
    9D   SM81C256K16CJ-25 Silicon Magic 100MHz EDO RAM, 4Meg
    5D   SM81C256K16CJ-25 Silicon Magic 100MHz EDO RAM, 4Meg

    9P   500-0009-01      3DFX, Pixel processor
    10U  SM81C256K16CJ-25 Silicon Magic 100MHz EDO RAM, 4Meg
    7U   SM81C256K16CJ-25 Silicon Magic 100MHz EDO RAM, 4Meg
    3S   SM81C256K16CJ-25 Silicon Magic 100MHz EDO RAM, 4Meg
    3R   SM81C256K16CJ-25 Silicon Magic 100MHz EDO RAM, 4Meg

    27G  XC9536           Xilinx, CPLD, Konami no. Q830B1
    21C  MC44200FT        Motorola, 3 Channel video D/A converter
*/

#include "driver.h"
#include "cpu/powerpc/ppc.h"
#include "cpu/sharc/sharc.h"
#include "machine/konppc.h"
#include "vidhrdw/voodoo.h"
#include "machine/timekpr.h"
#include "sound/rf5c400.h"

static UINT8 led_reg0 = 0x7f, led_reg1 = 0x7f;

/* K037122 Tilemap chip (move to konamiic.c ?) */

static UINT32 *K037122_tile_ram;
static UINT32 *K037122_char_ram;
static UINT8 *K037122_dirty_map;
static int K037122_gfx_index, K037122_char_dirty;
static tilemap *K037122_layer[2];
static UINT32 K037122_reg[256];

#define K037122_NUM_TILES		16384

static const gfx_layout K037122_char_layout =
{
	8, 8,
	K037122_NUM_TILES,
	8,
	{ 0,1,2,3,4,5,6,7 },
	{ 1*16, 0*16, 3*16, 2*16, 5*16, 4*16, 7*16, 6*16 },
	{ 0*128, 1*128, 2*128, 3*128, 4*128, 5*128, 6*128, 7*128 },
	8*128
};

static void K037122_tile_info_layer0(int tile_index)
{
	UINT32 val = K037122_tile_ram[tile_index + (0x8000/4)];
	int color = (val >> 17) & 0x1f;
	int tile = val & 0x3fff;
	int flags = 0;

	if (val & 0x400000)
		flags |= TILE_FLIPX;
	if (val & 0x800000)
		flags |= TILE_FLIPY;

	SET_TILE_INFO(K037122_gfx_index, tile, color, flags);
}

static void K037122_tile_info_layer1(int tile_index)
{
	UINT32 val = K037122_tile_ram[tile_index];
	int color = (val >> 17) & 0x1f;
	int tile = val & 0x3fff;
	int flags = 0;

	if (val & 0x400000)
		flags |= TILE_FLIPX;
	if (val & 0x800000)
		flags |= TILE_FLIPY;

	SET_TILE_INFO(K037122_gfx_index, tile, color, flags);
}

int K037122_vh_start(void)
{
	for(K037122_gfx_index = 0; K037122_gfx_index < MAX_GFX_ELEMENTS; K037122_gfx_index++)
		if (Machine->gfx[K037122_gfx_index] == 0)
			break;
	if(K037122_gfx_index == MAX_GFX_ELEMENTS)
		return 1;

	K037122_char_ram = auto_malloc(0x200000);

	K037122_tile_ram = auto_malloc(0x20000);

	K037122_dirty_map = auto_malloc(K037122_NUM_TILES);

	K037122_layer[0] = tilemap_create(K037122_tile_info_layer0, tilemap_scan_rows, TILEMAP_TRANSPARENT, 8, 8, 256, 64);
	K037122_layer[1] = tilemap_create(K037122_tile_info_layer1, tilemap_scan_rows, TILEMAP_TRANSPARENT, 8, 8, 128, 64);

	if( !K037122_layer[0] || !K037122_layer[1] ) {
		return 1;
	}

	tilemap_set_transparent_pen(K037122_layer[0], 0);
	tilemap_set_transparent_pen(K037122_layer[1], 0);

	memset(K037122_char_ram, 0, 0x200000);
	memset(K037122_tile_ram, 0, 0x20000);
	memset(K037122_dirty_map, 0, K037122_NUM_TILES);

	Machine->gfx[K037122_gfx_index] = allocgfx(&K037122_char_layout);
	decodegfx(Machine->gfx[K037122_gfx_index], (UINT8*)K037122_char_ram, 0, Machine->gfx[K037122_gfx_index]->total_elements);
	if( !Machine->gfx[K037122_gfx_index] ) {
		return 1;
	}

	if (Machine->drv->color_table_len)
	{
		Machine->gfx[K037122_gfx_index]->colortable = Machine->remapped_colortable;
		Machine->gfx[K037122_gfx_index]->total_colors = Machine->drv->color_table_len / 16;
	}
	else
	{
		Machine->gfx[K037122_gfx_index]->colortable = Machine->pens;
		Machine->gfx[K037122_gfx_index]->total_colors = Machine->drv->total_colors / 16;
	}

	return 0;
}

void K037122_tile_update(void)
{
	if(K037122_char_dirty) {
		int i;
		for(i=0; i<K037122_NUM_TILES; i++) {
			if(K037122_dirty_map[i]) {
				K037122_dirty_map[i] = 0;
				decodechar(Machine->gfx[K037122_gfx_index], i, (UINT8 *)K037122_char_ram, &K037122_char_layout);
			}
		}
		tilemap_mark_all_tiles_dirty(K037122_layer[0]);
		tilemap_mark_all_tiles_dirty(K037122_layer[1]);
		K037122_char_dirty = 0;
	}
}

void K037122_tile_draw(mame_bitmap *bitmap, const rectangle *cliprect)
{
	if (K037122_reg[0xc] & 0x10000)
	{
		tilemap_draw(bitmap, cliprect, K037122_layer[1], 0,0);
	}
	else
	{
		tilemap_draw(bitmap, cliprect, K037122_layer[0], 0,0);
	}
}

static void update_palette_color(UINT32 palette_base, int color)
{
	int r,g,b;
	UINT32 data = K037122_tile_ram[(palette_base/4) + color];

	r = ((data >> 6) & 0x1f);
	g = ((data >> 0) & 0x3f);
	b = ((data >> 11) & 0x1f);

	b = (b << 3) | (b >> 2);
	g = (g << 2) | (g >> 3);
	r = (r << 3) | (r >> 2);

	palette_set_color(color, r, g, b);
}

READ32_HANDLER(K037122_sram_r)
{
	return K037122_tile_ram[offset];
}

WRITE32_HANDLER(K037122_sram_w)
{
	COMBINE_DATA(K037122_tile_ram + offset);

	if (K037122_reg[0xc] & 0x10000)
	{
		if (offset < 0x8000/4)
		{
			tilemap_mark_tile_dirty(K037122_layer[1], offset);
		}
		else if (offset >= 0x8000/4 && offset < 0x18000/4)
		{
			tilemap_mark_tile_dirty(K037122_layer[0], offset - (0x8000/4));
		}
		else if (offset >= 0x18000/4)
		{
			update_palette_color(0x18000, offset - (0x18000/4));
		}
	}
	else
	{
		if (offset < 0x8000/4)
		{
			update_palette_color(0, offset);
		}
		else if (offset >= 0x8000/4 && offset < 0x18000/4)
		{
			tilemap_mark_tile_dirty(K037122_layer[0], offset - (0x8000/4));
		}
		else if (offset >= 0x18000/4)
		{
			tilemap_mark_tile_dirty(K037122_layer[1], offset - (0x18000/4));
		}
	}
}


READ32_HANDLER(K037122_char_r)
{
	UINT32 addr;
	int bank = K037122_reg[0x30/4] & 0x7;

	addr = offset + (bank * (0x40000/4));

	return K037122_char_ram[addr];
}

WRITE32_HANDLER(K037122_char_w)
{
	UINT32 addr;
	int bank = K037122_reg[0x30/4] & 0x7;

	addr = offset + (bank * (0x40000/4));

	COMBINE_DATA(K037122_char_ram + addr);
	K037122_dirty_map[addr / 32] = 1;
	K037122_char_dirty = 1;
}

READ32_HANDLER(K037122_reg_r)
{
	switch (offset)
	{
		case 0x14/4:
		{
			return 0x000003fa;
		}
	}
	return K037122_reg[offset];
}

WRITE32_HANDLER(K037122_reg_w)
{
	COMBINE_DATA( K037122_reg + offset );
}

static int voodoo_version = 0;

VIDEO_START( hornet )
{
	if (voodoo_version == 0)
	{
		if (voodoo_start(0, VOODOO_1, 2, 4, 0))
			return 1;
	}
	else
	{
		if (voodoo_start(0, VOODOO_2, 2, 4, 0))
			return 1;
	}

	return K037122_vh_start();
}

VIDEO_START( hornet_2board )
{
	if (voodoo_version == 0)
	{
		if (voodoo_start(0, VOODOO_1, 2, 4, 0) ||
		    voodoo_start(1, VOODOO_1, 2, 4, 0))
			return 1;
	}
	else
	{
		if (voodoo_start(0, VOODOO_2, 2, 4, 0) ||
			voodoo_start(1, VOODOO_2, 2, 4, 0))
			return 1;
	}

	return K037122_vh_start();
}


VIDEO_UPDATE( hornet )
{
	voodoo_update(0, bitmap, cliprect);

	K037122_tile_update();
	K037122_tile_draw(bitmap, cliprect);

	draw_7segment_led(bitmap, 3, 3, led_reg0);
	draw_7segment_led(bitmap, 9, 3, led_reg1);
}

VIDEO_UPDATE( hornet_2board )
{
	voodoo_update(0, bitmap, cliprect);
	voodoo_update(1, bitmap, cliprect);

	K037122_tile_update();
	K037122_tile_draw(bitmap, cliprect);

	draw_7segment_led(bitmap, 3, 3, led_reg0);
	draw_7segment_led(bitmap, 9, 3, led_reg1);
}

/*****************************************************************************/

static READ32_HANDLER( sysreg_r )
{
	UINT32 r = 0;
	if (offset == 0)
	{
		if (!(mem_mask & 0xff000000))
		{
			//printf("read sysreg 0\n");
			r |= readinputport(0) << 24;
		}
		if (!(mem_mask & 0x00ff0000))
		{
			r |= readinputport(1) << 16;
		}
		if (!(mem_mask & 0x0000ff00))
		{
			r |= readinputport(2) << 8;
		}
		if (!(mem_mask & 0x000000ff))
		{
			r |= 0xf7;
		}
	}
	else if (offset == 1)
	{
		if (!(mem_mask & 0xff000000))
		{
			r |= readinputport(3) << 24;
		}
	}
	return r;
}

static WRITE32_HANDLER( sysreg_w )
{
	if( offset == 0 ) {
		if (!(mem_mask & 0xff000000))
		{
			led_reg0 = (data >> 24) & 0xff;
		}
		if (!(mem_mask & 0x00ff0000))
		{
			led_reg1 = (data >> 16) & 0xff;
		}
		return;
	}
	if( offset == 1 )
	{
		if (!(mem_mask & 0xff000000))
		{
		}
		if (!(mem_mask & 0x000000ff))
		{
			if (data & 0x80)	/* CG Board 1 IRQ Ack */
			{
				cpunum_set_input_line(0, INPUT_LINE_IRQ1, CLEAR_LINE);
			}
			if (data & 0x40)	/* CG Board 0 IRQ Ack */
			{
				cpunum_set_input_line(0, INPUT_LINE_IRQ0, CLEAR_LINE);
			}
			set_cgboard_id((data >> 4) & 0x3);
		}
		return;
	}
}

static UINT8 sndto68k[16], sndtoppc[2];	/* read/write split mapping */

static READ32_HANDLER( ppc_sound_r )
{
	UINT32 r = 0;

	if (!(mem_mask & 0xff000000))
	{
		r |= sndtoppc[0] << 24;
	}
	if (!(mem_mask & 0x00ff0000))
	{
		r |= sndtoppc[1] << 16;
	}

//  printf("ppc_sound_r: %08X, %08X\n", offset, mem_mask);
	return r;
}

INLINE void write_snd_ppc(int reg, int val)
{
	sndto68k[reg] = val;

	if (reg == 7)
	{
		cpunum_set_input_line(1, INPUT_LINE_IRQ2, PULSE_LINE);
	}
}

static WRITE32_HANDLER( ppc_sound_w )
{
	int reg=0, val=0;

	//printf("ppc_sound_w: %08X, %08X, %08X\n", data, offset, mem_mask);

	if (!(mem_mask & 0xff000000))
	{
		reg = offset * 4;
		val = data >> 24;
		write_snd_ppc(reg, val);
	}

	if (!(mem_mask & 0x00ff0000))
	{
		reg = (offset * 4) + 1;
		val = (data >> 16) & 0xff;
		write_snd_ppc(reg, val);
	}

	if (!(mem_mask & 0x0000ff00))
	{
		reg = (offset * 4) + 2;
		val = (data >> 8) & 0xff;
		write_snd_ppc(reg, val);
	}

	if (!(mem_mask & 0x000000ff))
	{
		reg = (offset * 4) + 3;
		val = (data >> 0) & 0xff;
		write_snd_ppc(reg, val);
	}
}

static int comm_rombank = 0;

static WRITE32_HANDLER( comm1_w )
{
	printf("comm1_w: %08X, %08X, %08X\n", offset, data, mem_mask);
}

static WRITE32_HANDLER( comm_rombank_w )
{
	int bank = data >> 24;
	if (memory_region(REGION_USER3))
		if( bank != comm_rombank ) {
			comm_rombank = bank & 0x7f;
			memory_set_bankptr(1, memory_region(REGION_USER3) + (comm_rombank * 0x10000));
		}
}

static READ32_HANDLER( comm0_unk_r )
{
	printf("comm0_unk_r: %08X, %08X\n", offset, mem_mask);
	return 0xffffffff;
}

/*****************************************************************************/

static ADDRESS_MAP_START( hornet_map, ADDRESS_SPACE_PROGRAM, 32 )
	AM_RANGE(0x00000000, 0x003fffff) AM_MIRROR(0x80000000) AM_RAM		/* Work RAM */
	AM_RANGE(0x74000000, 0x740000ff) AM_MIRROR(0x80000000) AM_READWRITE(K037122_reg_r, K037122_reg_w)
	AM_RANGE(0x74020000, 0x7403ffff) AM_MIRROR(0x80000000) AM_READWRITE(K037122_sram_r, K037122_sram_w)
	AM_RANGE(0x74040000, 0x7407ffff) AM_MIRROR(0x80000000) AM_READWRITE(K037122_char_r, K037122_char_w)
	AM_RANGE(0x78000000, 0x7800ffff) AM_MIRROR(0x80000000) AM_READWRITE(cgboard_dsp_shared_r_ppc, cgboard_dsp_shared_w_ppc)
	AM_RANGE(0x780c0000, 0x780c0003) AM_MIRROR(0x80000000) AM_READWRITE(cgboard_dsp_comm_r_ppc, cgboard_dsp_comm_w_ppc)
	AM_RANGE(0x7d000000, 0x7d00ffff) AM_MIRROR(0x80000000) AM_READ(sysreg_r)
	AM_RANGE(0x7d010000, 0x7d01ffff) AM_MIRROR(0x80000000) AM_WRITE(sysreg_w)
	AM_RANGE(0x7d020000, 0x7d021fff) AM_MIRROR(0x80000000) AM_READWRITE(timekeeper_0_32be_r, timekeeper_0_32be_w)	/* M48T58Y RTC/NVRAM */
	AM_RANGE(0x7d030000, 0x7d030007) AM_MIRROR(0x80000000) AM_READWRITE(ppc_sound_r, ppc_sound_w)
	AM_RANGE(0x7d042000, 0x7d043fff) AM_MIRROR(0x80000000) AM_RAM				/* COMM BOARD 0 */
	AM_RANGE(0x7d044000, 0x7d044007) AM_MIRROR(0x80000000) AM_READ(comm0_unk_r)
	AM_RANGE(0x7d048000, 0x7d048003) AM_MIRROR(0x80000000) AM_WRITE(comm1_w)
	AM_RANGE(0x7d04a000, 0x7d04a003) AM_MIRROR(0x80000000) AM_WRITE(comm_rombank_w)
	AM_RANGE(0x7d050000, 0x7d05ffff) AM_MIRROR(0x80000000) AM_ROMBANK(1)		/* COMM BOARD 1 */
	AM_RANGE(0x7e000000, 0x7e7fffff) AM_MIRROR(0x80000000) AM_ROM AM_REGION(REGION_USER2, 0)		/* Data ROM */
	AM_RANGE(0x7f000000, 0x7f1fffff) AM_MIRROR(0x80000000) AM_ROM AM_SHARE(2)
	AM_RANGE(0x7fe00000, 0x7fffffff) AM_MIRROR(0x80000000) AM_ROM AM_REGION(REGION_USER1, 0) AM_SHARE(2)	/* Program ROM */
ADDRESS_MAP_END

/*****************************************************************************/

static void *m68k_timer;
static int m68k_irq1_enable;

static void m68k_timer_tick(int param)
{
	if (m68k_irq1_enable)
	{
		cpunum_set_input_line(1, INPUT_LINE_IRQ1, PULSE_LINE);
	}
}

static READ16_HANDLER( sndcomm68k_r )
{
	return sndto68k[offset];
}

static WRITE16_HANDLER( sndcomm68k_w )
{
	if (offset == 4)
    {
    	m68k_irq1_enable = data & 0x1;
    }

	logerror("sndcomm68k_w: %08X, %08X, %08X at %08X\n", data, offset, mem_mask, activecpu_get_pc());
	sndtoppc[offset] = data;
}

static ADDRESS_MAP_START( sound_memmap, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x07ffff) AM_ROM
	AM_RANGE(0x100000, 0x10ffff) AM_RAM		/* Work RAM */
	AM_RANGE(0x200000, 0x200fff) AM_READWRITE(RF5C400_0_r, RF5C400_0_w)		/* Ricoh RF5C400 */
	AM_RANGE(0x300000, 0x30000f) AM_READ(sndcomm68k_r)
	AM_RANGE(0x300000, 0x30000f) AM_WRITE(sndcomm68k_w)
ADDRESS_MAP_END

/*****************************************************************************/

static UINT32 *sharc_dataram[2];

static READ32_HANDLER( dsp_dataram0_r )
{
	return sharc_dataram[0][offset] & 0xffff;
}

static WRITE32_HANDLER( dsp_dataram0_w )
{
	sharc_dataram[0][offset] = data;
}

static READ32_HANDLER( dsp_dataram1_r )
{
	return sharc_dataram[1][offset] & 0xffff;
}

static WRITE32_HANDLER( dsp_dataram1_w )
{
	sharc_dataram[1][offset] = data;
}

/* Konami 033906 seems to be a PCI bridge chip between SHARC and 3dfx chips */

static UINT32 pci_3dfx_reg[256];

READ32_HANDLER(pci_3dfx_r)
{
	switch(offset)
	{
		case 0x00:		return 0x0001121a;				// PCI Vendor ID (0x121a = 3dfx), Device ID (0x0001 = Voodoo)
		case 0x02:		return 0x04000000;				// Revision ID
		case 0x04:		return pci_3dfx_reg[0x04];		// memBaseAddr
		case 0x0f:		return pci_3dfx_reg[0x0f];		// interrupt_line, interrupt_pin, min_gnt, max_lat

		default:
			fatalerror("pci_3dfx_r: %08X at %08X", offset, activecpu_get_pc());
	}
	return 0;
}

WRITE32_HANDLER(pci_3dfx_w)
{
	switch(offset)
	{
		case 0x00:
			break;

		case 0x01:		// command register
			break;

		case 0x04:		// memBaseAddr
		{
			if (data == 0xffffffff)
			{
				pci_3dfx_reg[0x04] = 0xff000000;
			}
			else
			{
				pci_3dfx_reg[0x04] = data & 0xff000000;
			}
			break;
		}

		case 0x0f:		// interrupt_line, interrupt_pin, min_gnt, max_lat
		{
			pci_3dfx_reg[0x0f] = data;
			break;
		}

		case 0x10:		// initEnable
		{
			voodoo_set_init_enable(0, data);
			break;
		}

		case 0x11:		// busSnoop0
		case 0x12:		// busSnoop1
			break;

		case 0x38:		// ???
			break;

		default:
			fatalerror("pci_3dfx_w: %08X, %08X at %08X", data, offset, activecpu_get_pc());
	}
}

static ADDRESS_MAP_START( sharc0_map, ADDRESS_SPACE_DATA, 32 )
	AM_RANGE(0x0400000, 0x041ffff) AM_READWRITE(cgboard_dsp_shared_r_sharc, cgboard_dsp_shared_w_sharc)
	AM_RANGE(0x0500000, 0x05fffff) AM_READWRITE(dsp_dataram0_r, dsp_dataram0_w)
	AM_RANGE(0x1400000, 0x14fffff) AM_RAM
	AM_RANGE(0x2400000, 0x27fffff) AM_READWRITE(voodoo_0_r, voodoo_0_w)
	AM_RANGE(0x3400000, 0x34000ff) AM_READWRITE(cgboard_dsp_comm_r_sharc, cgboard_dsp_comm_w_sharc)
	AM_RANGE(0x3500000, 0x35000ff) AM_READWRITE(pci_3dfx_r, pci_3dfx_w)
	AM_RANGE(0x3600000, 0x37fffff) AM_ROMBANK(5)
ADDRESS_MAP_END

static ADDRESS_MAP_START( sharc1_map, ADDRESS_SPACE_DATA, 32 )
	AM_RANGE(0x0400000, 0x041ffff) AM_READWRITE(cgboard_dsp_shared_r_sharc, cgboard_dsp_shared_w_sharc)
	AM_RANGE(0x0500000, 0x05fffff) AM_READWRITE(dsp_dataram1_r, dsp_dataram1_w)
	AM_RANGE(0x1400000, 0x14fffff) AM_RAM
	AM_RANGE(0x2400000, 0x27fffff) AM_READWRITE(voodoo_1_r, voodoo_1_w)
	AM_RANGE(0x3400000, 0x34000ff) AM_READWRITE(cgboard_dsp_comm_r_sharc, cgboard_dsp_comm_w_sharc)
	AM_RANGE(0x3500000, 0x35000ff) AM_READWRITE(pci_3dfx_r, pci_3dfx_w)
	AM_RANGE(0x3600000, 0x37fffff) AM_ROMBANK(5)
ADDRESS_MAP_END

/*****************************************************************************/

INPUT_PORTS_START( hornet )
	PORT_START
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(1)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(1)
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_PLAYER(1)

	PORT_START
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(2)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(2)
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_PLAYER(2)

	PORT_START
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_SERVICE ) PORT_NAME("Service Button") PORT_CODE(KEYCODE_7)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_SERVICE ) PORT_NAME( DEF_STR( Service_Mode )) PORT_CODE(KEYCODE_F2)
	PORT_BIT( 0x0f, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START
	PORT_DIPNAME( 0x80, 0x00, "Test Mode" )
	PORT_DIPSETTING( 0x00, DEF_STR( Off ) )
	PORT_DIPSETTING( 0x80, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, "Screen Flip (H)" )
	PORT_DIPSETTING( 0x40, DEF_STR( Off ) )
	PORT_DIPSETTING( 0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, "Screen Flip (V)" )
	PORT_DIPSETTING( 0x20, DEF_STR( Off ) )
	PORT_DIPSETTING( 0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, "DIP4" )
	PORT_DIPSETTING( 0x10, DEF_STR( Off ) )
	PORT_DIPSETTING( 0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, "DIP5" )
	PORT_DIPSETTING( 0x08, DEF_STR( Off ) )
	PORT_DIPSETTING( 0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x00, "Harness" )
	PORT_DIPSETTING( 0x04, "JVS" )
	PORT_DIPSETTING( 0x00, "JAMMA" )
	PORT_DIPNAME( 0x02, 0x02, "DIP7" )
	PORT_DIPSETTING( 0x02, DEF_STR( Off ) )
	PORT_DIPSETTING( 0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x01, 0x01, "Monitor Type" )
	PORT_DIPSETTING( 0x01, "24KHz" )
	PORT_DIPSETTING( 0x00, "15KHz" )

INPUT_PORTS_END

static struct RF5C400interface rf5c400_interface =
{
	REGION_SOUND1
};

static ppc_config hornet_ppc_cfg =
{
	PPC_MODEL_403GA
};

static sharc_config sharc_cfg =
{
	BOOT_MODE_EPROM
};

/* PowerPC interrupts

    IRQ0:   Vblank
    IRQ2:   LANC
    DMA0
    NMI:    SCI

*/

static INTERRUPT_GEN( hornet_vblank )
{
	cpunum_set_input_line(0, INPUT_LINE_IRQ0, ASSERT_LINE);
}

static MACHINE_RESET( hornet )
{
	if (memory_region(REGION_USER3))
		memory_set_bankptr(1, memory_region(REGION_USER3));
	cpunum_set_input_line(2, INPUT_LINE_RESET, ASSERT_LINE);

	if (memory_region(REGION_USER5))
		memory_set_bankptr(5, memory_region(REGION_USER5));
}

static MACHINE_DRIVER_START( hornet )

	/* basic machine hardware */
	MDRV_CPU_ADD_TAG("main", PPC403, 64000000/2)	/* PowerPC 403GA 32MHz */
	MDRV_CPU_CONFIG(hornet_ppc_cfg)
	MDRV_CPU_PROGRAM_MAP(hornet_map, 0)
	MDRV_CPU_VBLANK_INT(hornet_vblank, 1)

	MDRV_CPU_ADD(M68000, 64000000/4)	/* 16MHz */
	MDRV_CPU_PROGRAM_MAP(sound_memmap, 0)

	MDRV_CPU_ADD(ADSP21062, 36000000)
	MDRV_CPU_CONFIG(sharc_cfg)
	MDRV_CPU_DATA_MAP(sharc0_map, 0)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_REAL_60HZ_VBLANK_DURATION)

	MDRV_MACHINE_RESET( hornet )

	MDRV_NVRAM_HANDLER( timekeeper_0 )

 	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER | VIDEO_NEEDS_6BITS_PER_GUN | VIDEO_RGB_DIRECT)
	MDRV_SCREEN_SIZE(64*8, 48*8)
	MDRV_VISIBLE_AREA(0*8, 64*8-1, 0*8, 48*8-1)
	MDRV_PALETTE_LENGTH(65536)

	MDRV_VIDEO_START(hornet)
	MDRV_VIDEO_UPDATE(hornet)

	MDRV_SPEAKER_STANDARD_STEREO("left", "right")

	MDRV_SOUND_ADD(RF5C400, 0)
	MDRV_SOUND_CONFIG(rf5c400_interface)
	MDRV_SOUND_ROUTE(0, "left", 1.0)
	MDRV_SOUND_ROUTE(1, "right", 1.0)

MACHINE_DRIVER_END

static int vblank=0;
static INTERRUPT_GEN( hornet_2board_vblank )
{
	if (vblank == 0)
	{
		cpunum_set_input_line(0, INPUT_LINE_IRQ0, ASSERT_LINE);
	}
	else
	{
		cpunum_set_input_line(0, INPUT_LINE_IRQ1, ASSERT_LINE);
	}
	vblank++;
	vblank &= 1;
}

static MACHINE_RESET( hornet_2board )
{
	if (memory_region(REGION_USER3))
		memory_set_bankptr(1, memory_region(REGION_USER3));
	cpunum_set_input_line(2, INPUT_LINE_RESET, ASSERT_LINE);
	cpunum_set_input_line(3, INPUT_LINE_RESET, ASSERT_LINE);

	if (memory_region(REGION_USER5))
		memory_set_bankptr(5, memory_region(REGION_USER5));
}

static MACHINE_DRIVER_START( hornet_2board )

	MDRV_IMPORT_FROM(hornet)
	MDRV_CPU_MODIFY("main")
	MDRV_CPU_VBLANK_INT(hornet_2board_vblank, 2)

	MDRV_CPU_ADD(ADSP21062, 36000000)
	MDRV_CPU_CONFIG(sharc_cfg)
	MDRV_CPU_DATA_MAP(sharc1_map, 0)

	MDRV_MACHINE_RESET(hornet_2board)

	MDRV_VIDEO_START(hornet_2board)
	MDRV_VIDEO_UPDATE(hornet_2board)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER | VIDEO_NEEDS_6BITS_PER_GUN | VIDEO_RGB_DIRECT)
	MDRV_SCREEN_SIZE(128*8, 48*8)
	MDRV_VISIBLE_AREA(0*8, 128*8-1, 0*8, 48*8-1)
	MDRV_PALETTE_LENGTH(65536)
MACHINE_DRIVER_END


static UINT8 jamma_rdata[1024];
static void jamma_r(int length)
{
	int i;
	for (i=0; i < length; i++)
	{
		jamma_rdata[i] = 0;
	}
}

static UINT8 jamma_wdata[1024];
static void jamma_w(int length)
{
}

static UINT8 backup_ram[0x2000];
static DRIVER_INIT( hornet )
{
	init_konami_cgboard(0, CGBOARD_TYPE_HORNET);
	set_cgboard_texture_bank(5);
	sharc_dataram[0] = auto_malloc(0x100000);

	m68k_timer = timer_alloc(m68k_timer_tick);
	timer_adjust(m68k_timer, TIME_IN_MSEC(1000.0/(44100.0/128.0)), 0, TIME_IN_MSEC(1000.0/(44100.0/128.0)));

	timekeeper_init(0, TIMEKEEPER_M48T58, backup_ram);

	ppc403_install_spu_tx_dma_handler(jamma_w, jamma_wdata);
	ppc403_install_spu_rx_dma_handler(jamma_r, jamma_rdata);
}

static DRIVER_INIT( hornet_2board )
{
	init_konami_cgboard(0, CGBOARD_TYPE_HORNET);
	init_konami_cgboard(1, CGBOARD_TYPE_HORNET);

	set_cgboard_texture_bank(5);
	sharc_dataram[0] = auto_malloc(0x100000);
	sharc_dataram[1] = auto_malloc(0x100000);

	m68k_timer = timer_alloc(m68k_timer_tick);
	timer_adjust(m68k_timer, TIME_IN_MSEC(1000.0/(44100.0/128.0)), 0, TIME_IN_MSEC(1000.0/(44100.0/128.0)));

	timekeeper_init(0, TIMEKEEPER_M48T58, backup_ram);

	ppc403_install_spu_tx_dma_handler(jamma_w, jamma_wdata);
	ppc403_install_spu_rx_dma_handler(jamma_r, jamma_rdata);
}

static DRIVER_INIT(gradius4)
{
	/* RTC data */
	backup_ram[0x00] = 0x47;	// 'G'
	backup_ram[0x01] = 0x58;	// 'X'
	backup_ram[0x02] = 0x38;	// '8'
	backup_ram[0x03] = 0x33;	// '3'
	backup_ram[0x04] = 0x37;	// '7'
	backup_ram[0x05] = 0x00;	//
	backup_ram[0x06] = 0x11;	//
	backup_ram[0x07] = 0x06;	// 06 / 11
	backup_ram[0x08] = 0x19;	//
	backup_ram[0x09] = 0x98;	// 1998
	backup_ram[0x0a] = 0x4a;	// 'J'
	backup_ram[0x0b] = 0x41;	// 'A'
	backup_ram[0x0c] = 0x43;	// 'C'
	backup_ram[0x0d] = 0x00;	//
	backup_ram[0x0e] = 0x02;	// checksum
	backup_ram[0x0f] = 0xd7;	// checksum

	voodoo_version = 0;
	init_hornet();
}

static DRIVER_INIT(nbapbp)
{
	int i;
	UINT16 checksum;

	/* RTC data */
	backup_ram[0x00] = 0x47;	// 'G'
	backup_ram[0x01] = 0x58;	// 'X'
	backup_ram[0x02] = 0x37;	// '7'
	backup_ram[0x03] = 0x37;	// '7'
	backup_ram[0x04] = 0x38;	// '8'
	backup_ram[0x05] = 0x00;	//
	backup_ram[0x06] = 0x00;	//
	backup_ram[0x07] = 0x00;	//
	backup_ram[0x08] = 0x19;	//
	backup_ram[0x09] = 0x98;	// 1998
	backup_ram[0x0a] = 0x4a;	// 'J'
	backup_ram[0x0b] = 0x41;	// 'A'
	backup_ram[0x0c] = 0x41;	// 'A'
	backup_ram[0x0d] = 0x00;	//

	checksum = 0;
	for (i=0; i < 14; i++)
	{
		checksum += backup_ram[i];
		checksum &= 0xffff;
	}
	backup_ram[0x0e] = (checksum >> 8) & 0xff;	// checksum
	backup_ram[0x0f] = (checksum >> 0) & 0xff;	// checksum

	voodoo_version = 0;
	init_hornet();
}

static DRIVER_INIT(sscope)
{
	int i;
	UINT16 checksum;

	/* RTC data */
	backup_ram[0x00] = 0x47;	// 'G'
	backup_ram[0x01] = 0x51;	// 'Q'
	backup_ram[0x02] = 0x38;	// '8'
	backup_ram[0x03] = 0x33;	// '3'
	backup_ram[0x04] = 0x30;	// '0'
	backup_ram[0x05] = 0x00;	//
	backup_ram[0x06] = 0x00;	//
	backup_ram[0x07] = 0x00;	//
	backup_ram[0x08] = 0x20;	//
	backup_ram[0x09] = 0x00;	// 2000
	backup_ram[0x0a] = 0x55;	// 'U'
	backup_ram[0x0b] = 0x41;	// 'A'
	backup_ram[0x0c] = 0x41;	// 'A'
	backup_ram[0x0d] = 0x00;	//

	checksum = 0;
	for (i=0; i < 14; i+=2)
	{
		checksum += (backup_ram[i] << 8) | (backup_ram[i+1]);
	}
	checksum = ~checksum - 1;
	backup_ram[0x0e] = (checksum >> 8) & 0xff;	// checksum
	backup_ram[0x0f] = (checksum >> 0) & 0xff;	// checksum

	voodoo_version = 0;
	init_hornet_2board();
}

static DRIVER_INIT(sscope2)
{
	int i;
	int checksum;

	/* RTC data */
	backup_ram[0x00] = 0x47;	// 'G'
	backup_ram[0x01] = 0x4b;	// 'K'
	backup_ram[0x02] = 0x39;	// '9'
	backup_ram[0x03] = 0x33;	// '3'
	backup_ram[0x04] = 0x31;	// '1'
	backup_ram[0x05] = 0x00;	//
	backup_ram[0x06] = 0x00;	//
	backup_ram[0x07] = 0x00;	//
	backup_ram[0x08] = 0x20;	//
	backup_ram[0x09] = 0x00;	// 2000
	backup_ram[0x0a] = 0x55;	// 'U'
	backup_ram[0x0b] = 0x41;	// 'A'
	backup_ram[0x0c] = 0x41;	// 'A'
	backup_ram[0x0d] = 0x00;	//

	checksum = 0;
	for (i=0; i < 14; i+=2)
	{
		checksum += (backup_ram[i] << 8) | (backup_ram[i+1]);
		checksum &= 0xffff;
	}
	checksum = (-1 - checksum) - 1;
	backup_ram[0x0e] = (checksum >> 8) & 0xff;	// checksum
	backup_ram[0x0f] = (checksum >> 0) & 0xff;	// checksum


	/* Silent Scope data */
	backup_ram[0x1f40] = 0x47;	// 'G'
	backup_ram[0x1f41] = 0x4b;	// 'Q'
	backup_ram[0x1f42] = 0x38;	// '8'
	backup_ram[0x1f43] = 0x33;	// '3'
	backup_ram[0x1f44] = 0x30;	// '0'
	backup_ram[0x1f45] = 0x00;	//
	backup_ram[0x1f46] = 0x00;	//
	backup_ram[0x1f47] = 0x00;	//
	backup_ram[0x1f48] = 0x20;	//
	backup_ram[0x1f49] = 0x00;	// 2000
	backup_ram[0x1f4a] = 0x55;	// 'U'
	backup_ram[0x1f4b] = 0x41;	// 'A'
	backup_ram[0x1f4c] = 0x41;	// 'A'
	backup_ram[0x1f4d] = 0x00;	//

	checksum = 0;
	for (i=0x1f40; i < 0x1f4e; i+=2)
	{
		checksum += (backup_ram[i] << 8) | (backup_ram[i+1]);
		checksum &= 0xffff;
	}
	checksum = (-1 - checksum) - 1;
	backup_ram[0x1f4e] = (checksum >> 8) & 0xff;	// checksum
	backup_ram[0x1f4f] = (checksum >> 0) & 0xff;	// checksum

	voodoo_version = 1;
	init_hornet_2board();
}

/*****************************************************************************/

ROM_START(sscope)
	ROM_REGION32_BE(0x200000, REGION_USER1, 0)	/* PowerPC program */
	ROM_LOAD16_WORD_SWAP("ss1-1.27p", 0x000000, 0x200000, CRC(3b6bb075) SHA1(babc134c3a20c7cdcaa735d5f1fd5cab38667a14))

	ROM_REGION32_BE(0x800000, REGION_USER2, ROMREGION_ERASE00)	/* Data roms */

	ROM_REGION(0x80000, REGION_CPU2, 0)		/* 68K Program */
	ROM_LOAD16_WORD_SWAP("ss1-1.7s", 0x000000, 0x80000, CRC(2805ea1d) SHA1(2556a51ee98cb8f59bf081e916c69a24532196f1))

	ROM_REGION(0x800000, REGION_USER5, 0)		/* CG Board texture roms */
    	ROM_LOAD32_WORD_SWAP( "ss1-3.u32",    0x000000, 0x400000, CRC(335793e1) SHA1(d582b53c3853abd59bc728f619a30c27cfc9497c) )
    	ROM_LOAD32_WORD_SWAP( "ss1-3.u24",    0x000002, 0x400000, CRC(d6e7877e) SHA1(b4d0e17ada7dd126ec564a20e7140775b4b3fdb7) )

	ROM_REGION(0x400000, REGION_SOUND1, 0)		/* PCM sample roms */
        ROM_LOAD( "ss1-1.16p",    0x000000, 0x200000, CRC(4503ff1e) SHA1(2c208a1e9a5633c97e8a8387b7fcc7460013bc2c) )
        ROM_LOAD( "ss1-1.14p",    0x200000, 0x200000, CRC(a5bd9a93) SHA1(c789a272b9f2b449b07fff1c04b6c9ef3ca6bfe0) )
ROM_END

ROM_START(sscopea)
	ROM_REGION32_BE(0x200000, REGION_USER1, 0)	/* PowerPC program */
	ROM_LOAD16_WORD_SWAP("830_a01.bin", 0x000000, 0x200000, CRC(39e353f1) SHA1(569b06969ae7a690f6d6e63cc3b5336061663a37))

	ROM_REGION32_BE(0x800000, REGION_USER2, ROMREGION_ERASE00)	/* Data roms */

	ROM_REGION(0x80000, REGION_CPU2, 0)		/* 68K Program */
	ROM_LOAD16_WORD_SWAP("ss1-1.7s", 0x000000, 0x80000, CRC(2805ea1d) SHA1(2556a51ee98cb8f59bf081e916c69a24532196f1))

	ROM_REGION(0x800000, REGION_USER5, 0)		/* CG Board texture roms */
    	ROM_LOAD32_WORD_SWAP( "ss1-3.u32",    0x000000, 0x400000, CRC(335793e1) SHA1(d582b53c3853abd59bc728f619a30c27cfc9497c) )
    	ROM_LOAD32_WORD_SWAP( "ss1-3.u24",    0x000002, 0x400000, CRC(d6e7877e) SHA1(b4d0e17ada7dd126ec564a20e7140775b4b3fdb7) )

	ROM_REGION(0x400000, REGION_SOUND1, 0)		/* PCM sample roms */
        ROM_LOAD( "ss1-1.16p",    0x000000, 0x200000, CRC(4503ff1e) SHA1(2c208a1e9a5633c97e8a8387b7fcc7460013bc2c) )
        ROM_LOAD( "ss1-1.14p",    0x200000, 0x200000, CRC(a5bd9a93) SHA1(c789a272b9f2b449b07fff1c04b6c9ef3ca6bfe0) )
ROM_END

ROM_START(sscope2)
	ROM_REGION32_BE(0x200000, REGION_USER1, 0)	/* PowerPC program */
	ROM_LOAD16_WORD_SWAP("931d01.bin", 0x000000, 0x200000, CRC(4065fde6) SHA1(84f2dedc3e8f61651b22c0a21433a64993e1b9e2))

	ROM_REGION32_BE(0x800000, REGION_USER2, 0)	/* Data roms */
		ROM_LOAD32_WORD_SWAP("931a04.bin", 0x000000, 0x200000, CRC(4f5917e6) SHA1(a63a107f1d6d9756e4ab0965d72ea446f0692814))

	ROM_REGION32_BE(0x800000, REGION_USER3, 0)	/* Comm board roms */
	ROM_LOAD("931a19.bin", 0x000000, 0x400000, CRC(8e8bb6af) SHA1(1bb399f7897fbcbe6852fda3215052b2810437d8))
	ROM_LOAD("931a20.bin", 0x400000, 0x400000, CRC(a14a7887) SHA1(daf0cbaf83e59680a0d3c4d66fcc48d02c9723d1))

	ROM_REGION(0x80000, REGION_CPU2, 0)		/* 68K Program */
	ROM_LOAD16_WORD_SWAP("931a08.bin", 0x000000, 0x80000, CRC(1597d604) SHA1(a1eab4d25907930b59ea558b484c3b6ddcb9303c))

	ROM_REGION(0xc00000, REGION_SOUND1, 0)		/* PCM sample roms */
        ROM_LOAD( "931a09.bin",   0x000000, 0x400000, CRC(694c354c) SHA1(42f54254a5959e1b341f2801f1ad032c4ed6f329) )
        ROM_LOAD( "931a10.bin",   0x400000, 0x400000, CRC(78ceb519) SHA1(e61c0d21b6dc37a9293e72814474f5aee59115ad) )
        ROM_LOAD( "931a11.bin",   0x800000, 0x400000, CRC(9c8362b2) SHA1(a8158c4db386e2bbd61dc9a600720f07a1eba294) )
ROM_END

ROM_START(gradius4)
	ROM_REGION32_BE(0x200000, REGION_USER1, 0)	/* PowerPC program */
        ROM_LOAD16_WORD_SWAP( "837c01.27p",   0x000000, 0x200000, CRC(ce003123) SHA1(15e33997be2c1b3f71998627c540db378680a7a1) )

	ROM_REGION32_BE(0x800000, REGION_USER2, 0)	/* Data roms */
        ROM_LOAD32_WORD_SWAP( "837a04.16t",   0x000000, 0x200000, CRC(18453b59) SHA1(3c75a54d8c09c0796223b42d30fb3867a911a074) )
        ROM_LOAD32_WORD_SWAP( "837a05.14t",   0x000002, 0x200000, CRC(77178633) SHA1(ececdd501d0692390325c8dad6dbb068808a8b26) )

	ROM_REGION32_BE(0x1000000, REGION_USER5, 0)	/* CG Board texture roms */
        ROM_LOAD32_WORD_SWAP( "837a14.32u",   0x000002, 0x400000, CRC(ff1b5d18) SHA1(7a38362170133dcc6ea01eb62981845917b85c36) )
        ROM_LOAD32_WORD_SWAP( "837a13.24u",   0x000000, 0x400000, CRC(d86e10ff) SHA1(6de1179d7081d9a93ab6df47692d3efc190c38ba) )
        ROM_LOAD32_WORD_SWAP( "837a16.32v",   0x800002, 0x400000, CRC(bb7a7558) SHA1(8c8cc062793c2dcfa72657b6ea0813d7223a0b87) )
        ROM_LOAD32_WORD_SWAP( "837a15.24v",   0x800000, 0x400000, CRC(e0620737) SHA1(c14078cdb44f75c7c956b3627045d8494941d6b4) )

	ROM_REGION(0x80000, REGION_CPU2, 0)		/* 68K Program */
        ROM_LOAD16_WORD_SWAP( "837a08.7s",    0x000000, 0x080000, CRC(c3a7ff56) SHA1(9d8d033277d560b58da151338d14b4758a9235ea) )

	ROM_REGION(0x800000, REGION_SOUND1, 0)		/* PCM sample roms */
        ROM_LOAD( "837a09.16p",   0x000000, 0x400000, CRC(fb8f3dc2) SHA1(69e314ac06308c5a24309abc3d7b05af6c0302a8) )
        ROM_LOAD( "837a10.14p",   0x400000, 0x400000, CRC(1419cad2) SHA1(a6369a5c29813fa51e8246d0c091736f32994f3d) )
ROM_END

ROM_START(nbapbp)
	ROM_REGION32_BE(0x200000, REGION_USER1, 0)	/* PowerPC program */
        ROM_LOAD16_WORD_SWAP( "778a01.27p",   0x000000, 0x200000, CRC(e70019ce) SHA1(8b187b6e670fdc88771da08a56685cd621b139dc) )

	ROM_REGION32_BE(0x800000, REGION_USER2, 0)	/* Data roms */
        ROM_LOAD32_WORD_SWAP( "778a04.16t",   0x000000, 0x400000, CRC(62c70132) SHA1(405aed149fc51e0adfa3ace3c644e47d53cf1ee3) )
        ROM_LOAD32_WORD_SWAP( "778a05.14t",   0x000002, 0x400000, CRC(03249803) SHA1(f632a5f1dfa0a8500407214df0ec8d98ce09bc2b) )

	ROM_REGION32_BE(0x1000000, REGION_USER5, 0)	/* CG Board texture roms */
        ROM_LOAD32_WORD_SWAP( "778a14.32u",   0x000002, 0x400000, CRC(db0c278d) SHA1(bb9884b6cdcdb707fff7e56e92e2ede062abcfd3) )
        ROM_LOAD32_WORD_SWAP( "778a13.24u",   0x000000, 0x400000, CRC(47fda9cc) SHA1(4aae01c1f1861b4b12a3f9de6b39eb4d11a9736b) )
        ROM_LOAD32_WORD_SWAP( "778a16.32v",   0x800002, 0x400000, CRC(6c0f46ea) SHA1(c6b9fbe14e13114a91a5925a0b46496260539687) )
        ROM_LOAD32_WORD_SWAP( "778a15.24v",   0x800000, 0x400000, CRC(d176ad0d) SHA1(2be755dfa3f60379d396734809bbaaaad49e0db5) )

	ROM_REGION(0x80000, REGION_CPU2, 0)		/* 68K Program */
        ROM_LOAD16_WORD_SWAP( "778a08.7s",    0x000000, 0x080000, CRC(6259b4bf) SHA1(d0c38870495c9a07984b4b85e736d6477dd44832) )

	ROM_REGION(0x1000000, REGION_SOUND1, 0)		/* PCM sample roms */
        ROM_LOAD( "778a09.16p",   0x000000, 0x400000, CRC(e8c6fd93) SHA1(dd378b67b3b7dd932e4b39fbf4321e706522247f) )
        ROM_LOAD( "778a10.14p",   0x400000, 0x400000, CRC(c6a0857b) SHA1(976734ba56460fcc090619fbba043a3d888c4f4e) )
        ROM_LOAD( "778a11.12p",   0x800000, 0x400000, CRC(40199382) SHA1(bee268adf9b6634a4f6bb39278ecd02f2bdcb1f4) )
        ROM_LOAD( "778a12.9p",    0xc00000, 0x400000, CRC(27d0c724) SHA1(48e48cbaea6db0de8c3471a2eda6faaa16eed46e) )
ROM_END

/*************************************************************************/

GAME( 1998, gradius4,	0,		hornet,			hornet,	gradius4,	ROT0,	"Konami",	"Gradius 4: Fukkatsu", GAME_IMPERFECT_SOUND )
GAME( 1998, nbapbp,		0,		hornet,			hornet,	nbapbp,		ROT0,	"Konami",	"NBA Play By Play", GAME_IMPERFECT_SOUND )
GAME( 2000, sscope,		0,		hornet_2board,	hornet,	sscope,		ROT0,	"Konami",	"Silent Scope (ver UAB)", GAME_IMPERFECT_SOUND|GAME_NOT_WORKING )
GAME( 2000, sscopea,	sscope, hornet_2board,	hornet,	sscope,		ROT0,	"Konami",	"Silent Scope (ver UAA)", GAME_IMPERFECT_SOUND|GAME_NOT_WORKING )
GAME( 2000, sscope2,	0,		hornet_2board,	hornet,	sscope2,	ROT0,	"Konami",	"Silent Scope 2", GAME_IMPERFECT_SOUND|GAME_NOT_WORKING )
