/*
    Sega Model 3
    PowerPC 603e + tilemaps + Real3D 1000 + 68000 + 2x SCSP
    Preliminary driver by Andrew Gardiner, R. Belmont and Ville Linde

    Hardware info from Team Supermodel: Bart Trzynadlowski, Ville Linde, and Stefano Teso

    Hardware revisions
    ------------------
    Step 1.0: 66 MHz PPC
    Step 1.5: 100 MHz PPC, faster 3D engine
    Step 2.0: 166 MHz PPC, even faster 3D engine
    Step 2.1: 166 MHz PPC, same 3D engine as 2.0, differences unknown

===================================================================================

Tilemap generator notes:

    0xF1000000-0xF111FFFF       Tilegen VRAM
    0xF1180000-0xF11800FF (approx.) Tilegen regs

Offsets in tilemap VRAM:

    0       Tile pattern VRAM (can be 4 or 8 bpp, layout is normal chunky-pixel format)
    0xF8000 Layer 0 (top)
    0xFA000 Layer 1
    0xFC000 Layer 2
    0xFE000 Layer 3 (bottom)

    0x100000    Palette (1-5-5-5 A-G-B-R format)

Offsets in tilemap registers:

    0x10: IRQ ack for VBlank

    0x20: layer depths

    xxxxxxxxDCBAxxxxxxxxxxxxxxxxxxxx
    3  2   2   2   1   1   8   4   0
    1  8   4   0   6   2

    A: 0 = layer A is 8-bit, 1 = layer A is 4-bit
    B: 0 = layer B is 8-bit, 1 = layer B is 4-bit
    C: 0 = layer C is 8-bit, 1 = layer C is 4-bit
    D: 0 = layer D is 8-bit, 1 = layer D is 4-bit

Tilemap entry formats (16-bit wide):
15                                                              0
+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+
| T0|PHI|T14|T13|T12|T11|T10| T9| T8| T7| T6| T5| T4| T3| T2| T1|
+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+

    T0 - T14 = pattern name (multiply by 32 or 64 to get a byte offset
               into pattern VRAM.  Yes, bit 15 is T0.)
    To get the palette offset, use the top 7 (8 bpp) or 11 (4 bpp)
    bits of the tilemap entry, *excluding* bit 15.  So PHI-T9
    are the palette select bits for 8bpp and PHI-T5 for 4bpp.

===================================================================================

Harley Davidson (Rev.A)
Sega, 1997

This game runs on Sega Model3 Step2 hardware.

COMM Board
----------

171-7053B  837-11861  NEP-16T MODEL3 COMMUNICATION BOARD
|---------------------------------------------------------------------------------------------------|
|                                                                                                   |
|   LATTICE                     NKK N341256SJ-20     NKK N341256SJ-20     NKK N341256SJ-20    40MHz |
|   PLSI 2032   JP3                                                                      JP1        |
|               JP4             NKK N341256SJ-20     NKK N341256SJ-20     NKK N341256SJ-20          |
|                                                                                             JP2   |
|                                                                                                   |
|   PALCE16V8                                                                                       |
|   315-6075A      68000FN12       315-5804             315-5917              315-5917              |
|                                  (QFP144)             (QFP80)               (QFP80)               |
|   PALCE16V8                                                                                       |
|   315-6074                                                                                        |
|                                                                                                   |
|---------------------------------------------------------------------------------------------------|
JP1: 1-2
JP2: 2-3
JP3: not shorted
JP4: shorted


ROM Board
---------

171-7427B  837-13022 MODEL3 STEP2 ROM BOARD
|---------------------------------------------------------------------------------------------------|
|                                                                                                   |
|        MC88915FN70                                     GAL16V8B                                   |
|    JP13                                                315-6090A  JP1  JP11  JP2           JP7 2-3|
|     2-3                                                           1-2  1-2   2-3           JP8 2-3|
|                                                                                            JP9 2-3|
| VROM01.26   VROM00.27    CROM03.1    CROM02.2                           SROM0.21                  |
|                                                                                                   |
| VROM03.28   VROM02.29    CROM01.3    CROM00.4                                                     |
|                                                                                                   |
| VROM05.30   VROM04.31    CROM13.5    CROM12.6     CROM2.18 CROM0.20         SROM1.22  SROM3.24    |
|                                                                                                   |
| VROM07.32   VROM06.33    CROM11.7    CROM10.8                                                     |
|                                                                                                   |
| VROM11.34   VROM10.35    CROM23.9    CROM22.10                                                    |
|                                                                                                   |
| VROM13.36   VROM12.37    CROM21.11   CROM20.12                                                    |
|                                                                                                   |
| VROM15.38   VROM14.39    CROM33.13   CROM32.14    CROM3.17 CROM1.19         SROM2.23  SROM4.25    |
|                                                                                                   |
| VROM17.40   VROM16.41    CROM31.15   CROM30.16                                                    |
|                                                                                                   |
|---------------------------------------------------------------------------------------------------|

VROM00.27 mpr-20378 \
VROM01.26 mpr-20377 |
VROM02.29 mpr-20380 |
VROM03.28 mpr-20379 |
VROM04.31 mpr-20382 |
VROM05.30 mpr-20381 |
VROM06.33 mpr-20384 |
VROM07.32 mpr-20383  \ 32MBit DIP42 MASK ROM
VROM10.35 mpr-20386  /
VROM11.34 mpr-20385 |
VROM12.37 mpr-20388 |
VROM13.36 mpr-20387 |
VROM14.39 mpr-20390 |
VROM15.38 mpr-20389 |
VROM16.41 mpr-20392 |
VROM17.40 mpr-20391 /

CROM00.4  mpr-20364 \
CROM01.3  mpr-20363 |
CROM02.2  mpr-20362 |
CROM03.1  mpr-20361  \ 32MBit DIP42 MASK ROM
CROM10.8  mpr-20368  /
CROM11.7  mpr-20367 |
CROM12.6  mpr-20366 |
CROM13.5  mpr-20365 /
CROM20.12 not populated
CROM21.11 not populated
CROM22.10 not populated
CROM23.9  not populated
CROM30.16 epr-20412 \
CROM31.15 epr-20411 |  16MBit DIP42 EPROM (27C160)
CROM32.14 epr-20410 |
CROM33.13 epr-20409 /

CROM0.20  epr-20396A \
CROM1.19  epr-20395A | 16MBit DIP42 EPROM (27C160)
CROM2.18  epr-20394A |
CROM3.17  epr-20393A /

SROM0.21  epr-20397    4MBit DIP40 EPROM (27C4096)
SROM1.22  mpr-20373 \
SROM2.23  mpr-20374 |  32MBit DIP42 MASK ROM
SROM3.24  mpr-20375 |
SROM4.25  mpr-20376 /

The jumpers are used to configure the ROM types (All ROMs are 16 bit)
On Model 3 step 2.0 ROM boards, JP13 selects the size of the VROMs.
1-2 jumpered -> 16MBit (read as 27C160)
2-3 jumpered -> 32MBit (read as 27C322)

JP1, JP11 & JP2 seems to select the size of the CROMs. The jumpers are tied to +5V and ground and to PAL 315-6090A.
Many lines from the PAL are tied to the highest address line of the CROMs.
The contents of the PAL is unknown, so it's difficult to determine what the jumpers do exactly.
It's assumed the PAL does the size selection based on certain pins on the PAL being high or low which are set by the jumpers.
JP1  1-2 \
JP11 1-2 | Selects CROMs 00-03 & 10-13 as 32MBit
JP2  2-3 / and CROMs 30-33 as 16MBit
Alternative jumper configurations are not known.

JP7 selects the size of SROM0 (jumper pin 2 connected to pin 39(A17) of SROM0)
1-2 jumpered -> 1MBit (read as 27C1024)
2-3 jumpered -> 4MBit (read as 27C4096)

JP8 and JP9 seems to select the size of SROM1-4.
JP8 pin 2 is tied to pin 32 of SROM1-4 (highest address line A20)
JP9 pin 2 is tied to some logic.
JP8 & JP9 pin 3 are hardwired together.
JP8 and JP9 jumpered 2-3 selects SROM1-4 as 32MBit.
Alternative jumper configurations are not known.


CPU Board
---------

837-12715 171-7331C MODEL3 STEP2 CPU BOARD
|---------------------------------------------------------------------------------------------------|
| TC59S1616AFT-12  TC59S1616AFT-12                                                                  |
| (TSOP50)         (TSOP50)                                              GAL16V8D                   |
|                                                                        315-6089A                  |
| TC59S1616AFT-12  TC59S1616AFT-12                                       (PLCC20)                   |
| (TSOP50)         (TSOP50)                          45.158MHz        68000-12                      |
|                                                                     (PLCC68)                      |
|           MOTOROLA            PowerPC603ev                                      4066              |
|           XPC106ARX66CE       (QFP240,             SEGA             SEGA            TDA1386T      |
|           MPC106+               HEATSINKED)        315-5687 "SCSP"  315-5687 "SCSP"               |
|           (BGA304)                                 (QFP128)         (QFP128)              LMC6484 |
| PALCE16V8                                                                                         |
| 315-6102          33.333MHz                                                                       |
| (PLCC20)                     MPC950                HM514270CJ7      HM514270CJ7      TDA1386T     |
|                              (QFP32)                                                              |
|                                                                                                   |
| SEGA                            SEGA                                3771                          |
| 315-5894       SYMBIOS          315-5893                                                          |
| (QFP240)       53C810           (QFP240)                                                          |
|                (not populated)                                                                    |
|                                                                                                   |
|                                                    SEGA             SEGA       71AJC46A           |
|                                                    315-5296         315-5649   (SOIC8)            |
|                CY7C199          32MHz              (QFP100)         (QFP100)                      |
|                                                                                                   |
|                                                                                                   |
|                                                                                                   |
|                CY7C199                                                                            |
|                                                                             CN25                  |
|                                                    RTC72423             (Connector for )          |
| KM4132G271AQ-10                                                         (Protection PCB)          |
|                                                                         (   not used   )          |
|              32MHz                                 BATT_3V                                        |
|                          NEC D71051-10                                                            |
|                                                                                                   |
|              24.576MHz      SW1   SW2    LH52B256  LH52B256            DIPSW(8)   SW4    SW3      |
|---------------------------------------------------------------------------------------------------|


Video Board
-----------

837-12716 171-7332H MODEL3 STEP2 VIDEO BOARD
|---------------------------------------------------------------------------------------------------|
|                                                                                                   |
|  D4811650GF  D4811650GF                   33MHz   M5M4V4169  M5M4V4169  M5M4V4169  M5M4V4169      |
|                         D4811650GF                M5M4V4169  M5M4V4169  M5M4V4169  M5M4V4169      |
|                                              M5M410092FP                                          |
|                                              (TQFP128)                                  M5M4V4169 |
|                           SEGA                            SEGA                          M5M4V4169 |
|              SEGA         315-6058                        315-6060                                |
|              315-6057     (BGA)                           (BGA)                         M5M4V4169 |
|  SEGA        (BGA)                                                                      M5M4V4169 |
|  315-6022                                                                                         |
|  (QFP208)                KM4132G271AQ-10     SEGA                      SEGA                       |
|                          (QFP100)            315-6059                  315-6060         M5M4V4169 |
|                                              (BGA)                     (BGA)            M5M4V4169 |
|                                                                                                   |
|                                                                                                   |
|            M5M410092FP                       M5M410092FP                                M5M4V4169 |
|  SEGA      (TQFP128)                         (TQFP128)                                  M5M4V4169 |
|  315-6061              M5M410092FP                                                                |
|  (BGA)                 (TQFP128)                                                        M5M4V4169 |
|            M5M410092FP                                                                  M5M4V4169 |
|            (TQFP128)       M5M410092FP                                                            |
|                            (TQFP128)         SEGA                      SEGA             M5M4V4169 |
|     SEGA      M5M410092FP                    315-6059                  315-6060         M5M4V4169 |
|     315-5648  (TQFP128)      M5M410092FP     (BGA)                     (BGA)                      |
|     (QFP64)                  (TQFP128)                    SEGA                                    |
|                                                           315-6060                      M5M4V4169 |
|                                                           (BGA)                         M5M4V4169 |
|                                                                                                   |
|                                                                                         M5M4V4169 |
|                                                                                         M5M4V4169 |
|  ADV7120KP30                                                                                      |
|                                                                                                   |
|                                                                                                   |
|            M5M410092FP                       M5M410092FP                                M5M4V4169 |
|  SEGA      (TQFP128)                         (TQFP128)                                  M5M4V4169 |
|  315-6061              M5M410092FP                                                                |
|  (BGA)                 (TQFP128)                                                        M5M4V4169 |
|            M5M410092FP                                                                  M5M4V4169 |
|            (TQFP128)       M5M410092FP                                                            |
|                            (TQFP128)         SEGA                      SEGA             M5M4V4169 |
|     SEGA      M5M410092FP                    315-6059                  315-6060         M5M4V4169 |
|     315-5648  (TQFP128)      M5M410092FP     (BGA)                     (BGA)                      |
|     (QFP64)                  (TQFP128)                    SEGA                                    |
|                                                           315-6060                      M5M4V4169 |
|                                                           (BGA)                         M5M4V4169 |
|                                                                                                   |
|                                                                                         M5M4V4169 |
|                                                                                         M5M4V4169 |
|  ADV7120KP30                                                                                      |
|  (PLCC44)                                                                                         |
|                                                   M5M4V4169  M5M4V4169  M5M4V4169  M5M4V4169      |
|                                                   M5M4V4169  M5M4V4169  M5M4V4169  M5M4V4169      |
|                                                                                                   |
|---------------------------------------------------------------------------------------------------|

===================================================================================

Scud Race
Sega, 1996

Tis game runs on Sega Model 3 Step 1.5 hardware


Sound Board (bolted to outside of the metal box)
-----------

PCB Layout
----------

PCB Number: 837-10084 DIGITAL AUDIO BD (C) SEGA 1993
---------------------------------------------------------------
84256     D71051GU-10                 D6376    SM5840
epr-19612
Z80             16MHz     SEGA                D65654GF102
                          315-5762            (QFP100)
                          (PLCC68)
                     20MHz           KM68257
                                     KM68257          84256
                                     KM68257
                                          12.288MHz   mpr-19603
                                                      mpr-19604
                                                      mpr-19605
DSW1                                                  mpr-19606
----------------------------------------------------------------


Jumpers
JP1: 1-2 (Select 16M ROM on bank 1 - ic57 & ic58)
JP2: 1-2 (Select 16M ROM on bank 2 - ic59 & ic60)



ROM Board
---------

PCB Number: 837-11860 MODEL3 ROM BOARD (C) SEGA 1995

(For dumping reference)

Jumpers    centre pin joins
-------------------------------------------------------
JP3: 2-3   pin2 of ic 1 to 16 and pin 39 of ic 17 to 20
JP4: 2-3   pin2 of ic 1 to 16 and pin 39 of ic 17 to 20
JP5: 2-3   pin2 of ic 1 to 16 and pin 39 of ic 17 to 20
JP6: 2-3   pin2 of ic 1 to 16 and pin 39 of ic 17 to 20
JP7: 2-3   pin2 of ic 22 to 25 and pin 39 ic ic21
JP8: 2-3   pin32 of ic 22 to 25
JP9: 2-3   pin32 of ic 22 to 25
Jumper pos. 1 is +5V

JP1: 1-2   gnd
JP2: 2-3   +5v
Jumper pos. 1 is GND
Jumper pos. 3 is +5V

           pin1 joins
-------------------------------
JP10: 1-2  pin32 of ic 26 to 41

All CROM ROMs are 32M MASK
ALL VROM ROMs are 16M MASK

*/

#include "driver.h"
#include "cpu/powerpc/ppc.h"
#include "machine/eeprom.h"
#include "machine/53c810.h"

int model3_irq_enable;
int model3_irq_state;
int model3_step;
int model3_draw_crosshair = 0;
UINT32 *model3_vrom;

static UINT64 *work_ram;
static UINT64 *model3_backup;
static int model3_crom_bank = 0;
static int model3_controls_bank;
static UINT32 real3d_device_id;

extern UINT64 *paletteram64;

/* defined in machine/model3.c */
extern void model3_machine_init(int step);
extern int model3_tap_read(void);
extern void model3_tap_write(int tck, int tms, int tdi, int trst);
extern void model3_tap_reset(void);
extern READ32_HANDLER(rtc72421_r);
extern WRITE32_HANDLER(rtc72421_w);


/* defined in vidhrdw/model3.c */
READ64_HANDLER(model3_char_r);
WRITE64_HANDLER(model3_char_w);
READ64_HANDLER(model3_tile_r);
WRITE64_HANDLER(model3_tile_w);
READ64_HANDLER(model3_vid_reg_r);
WRITE64_HANDLER(model3_vid_reg_w);
READ64_HANDLER(model3_palette_r);
WRITE64_HANDLER(model3_palette_w);
VIDEO_START(model3);
VIDEO_UPDATE(model3);
WRITE64_HANDLER(real3d_cmd_w);
WRITE64_HANDLER(real3d_display_list_w);
WRITE64_HANDLER(real3d_polygon_ram_w);
void real3d_display_list_end(void);
void real3d_display_list1_dma(UINT32 src, UINT32 dst, int length, int byteswap);
void real3d_display_list2_dma(UINT32 src, UINT32 dst, int length, int byteswap);
void real3d_vrom_texture_dma(UINT32 src, UINT32 dst, int length, int byteswap);
void real3d_texture_fifo_dma(UINT32 src, int length, int byteswap);
void real3d_polygon_ram_dma(UINT32 src, UINT32 dst, int length, int byteswap);

static void real3d_dma_callback(UINT32 src, UINT32 dst, int length, int byteswap);

#define BYTE_REVERSE32(x)		(((x >> 24) & 0xff) | \
								((x >> 8) & 0xff00) | \
								((x << 8) & 0xff0000) | \
								((x << 24) & 0xff000000))

static UINT32 pci_device_get_reg(int device, int reg)
{
	switch(device)
	{
		case 11:		/* ??? */
			switch(reg)
			{
				case 0x14:	return 0;		/* ??? */
				default:
					fatalerror("pci_device_get_reg: Device 11, unknown reg %02X", reg);
					break;
			}

		case 13:		/* Real3D Controller chip */
			switch(reg)
			{
				case 0:		return real3d_device_id;	/* PCI Vendor ID & Device ID */
				default:
					fatalerror("pci_device_get_reg: Real3D controller, unknown reg %02X", reg);
					break;
			}
			break;

		case 14:		/* NCR 53C810 SCSI Controller */
			switch(reg)
			{
				case 0:		return 0x00011000;		/* PCI Vendor ID (0x1000 = LSI Logic) */
				default:
					fatalerror("pci_device_get_reg: SCSI Controller, unknown reg %02X", reg);
					break;
			}
			break;
		case 16:		/* ??? (Used by Daytona 2) */
			switch(reg)
			{
				case 0:		return 0x182711db;		/* PCI Vendor ID & Device ID, 315-6183 ??? */
				default:
					fatalerror("pci_device_get_reg: Device 16, unknown reg %02X", reg);
					break;
			}
			break;

		default:
			fatalerror("pci_device_get_reg: Unknown device %d, reg %02X", device, reg);
			break;
	}

	return 0;
}

static void pci_device_set_reg(int device, int reg, UINT32 value)
{
	switch(device)
	{
		case 11:		/* Unknown device for now !!! */
			switch(reg)
			{
				case 0x01:		/* ??? */
					break;
				case 0x04:		/* ??? */
					break;
				case 0x10:		/* ??? */
					break;
				case 0x11:
					break;
				case 0x14:		/* ??? */
					break;
				default:
					fatalerror("pci_device_set_reg: Unknown device (11), unknown reg %02X %08X", reg, value);
					break;
			}
			break;

		case 13:		/* Real3D Controller chip */
			switch(reg)
			{
				case 0x01:		/* ??? */
					break;
				case 0x03:
					break;
				case 0x04:		/* ??? */
					break;
				default:
					fatalerror("pci_device_set_reg: Real3D controller, unknown reg %02X %08X", reg, value);
					break;
			}
			break;

		case 14:		/* NCR 53C810 SCSI Controller */
			switch(reg)
			{
				case 0x04/4:	/* Status / Command */
					break;
				case 0x0c/4:	/* Header Type / Latency Timer / Cache Line Size */
					break;
				case 0x14/4:	/* Base Address One (Memory) */
					break;
				default:
					fatalerror("pci_device_set_reg: SCSI Controller, unknown reg %02X, %08X", reg, value);
					break;
			}
			break;

		case 16:		/* ??? (Used by Daytona 2) */
			switch(reg)
			{
				case 4:			/* Base address ? (set to 0xC3000000) */
					break;
				default:
					fatalerror("pci_device_set_reg: Device 16, unknown reg %02X, %08X", reg, value);
					break;
			}
			break;

		default:
			fatalerror("pci_device_set_reg: Unknown device %d, reg %02X, %08X", device, reg, value);
			break;
	}
}

/*****************************************************************************/
/* Motorola MPC105 PCI Bridge/Memory Controller */

static UINT32 mpc105_regs[0x40];
static UINT32 mpc105_addr;
static int pci_bus;
static int pci_device;
static int pci_function;
static int pci_reg;

static READ64_HANDLER( mpc105_addr_r )
{
	if (!(mem_mask & U64(0xffffffff00000000)))
	{
		return (UINT64)mpc105_addr << 32;
	}
	return 0;
}

static WRITE64_HANDLER( mpc105_addr_w )
{
	if (!(mem_mask & U64(0xffffffff00000000)))
	{
		UINT32 d = BYTE_REVERSE32((UINT32)(data >> 32));
		mpc105_addr = data >> 32;

		pci_bus = (d >> 16) & 0xff;
		pci_device = (d >> 11) & 0x1f;
		pci_function = (d >> 8) & 0x7;
		pci_reg = (d >> 2) & 0x3f;
	}
}

static READ64_HANDLER( mpc105_data_r )
{
	if(pci_device == 0) {
		return ((UINT64)(BYTE_REVERSE32(mpc105_regs[(pci_reg/2)+1])) << 32) |
			   ((UINT64)(BYTE_REVERSE32(mpc105_regs[(pci_reg/2)+0])));
	}
	return BYTE_REVERSE32(pci_device_get_reg(pci_device, pci_reg));
}

static WRITE64_HANDLER( mpc105_data_w )
{
	if(pci_device == 0) {
		mpc105_regs[(pci_reg/2)+1] = BYTE_REVERSE32((UINT32)(data >> 32));
		mpc105_regs[(pci_reg/2)+0] = BYTE_REVERSE32((UINT32)(data));
		return;
	}
	if (!(mem_mask & 0xffffffff))
	{
		pci_device_set_reg(pci_device, pci_reg, BYTE_REVERSE32((UINT32)data));
	}
}

static READ64_HANDLER( mpc105_reg_r )
{
	return ((UINT64)(mpc105_regs[(offset*2)+0]) << 32) |
			(UINT64)(mpc105_regs[(offset*2)+1]);
}

static WRITE64_HANDLER( mpc105_reg_w )
{
	mpc105_regs[(offset*2)+0] = (UINT32)(data >> 32);
	mpc105_regs[(offset*2)+1] = (UINT32)data;
}

static void mpc105_init(void)
{
	/* set reset values */
	memset(mpc105_regs, 0, sizeof(mpc105_regs));
	mpc105_regs[0x00/4] = 0x00011057;		/* Vendor ID & Device ID */
	mpc105_regs[0x04/4] = 0x00800006;		/* PCI Command & PCI Status */
	mpc105_regs[0x08/4] = 0x00060000;		/* Class code */
	mpc105_regs[0xa8/4] = 0x0010ff00;		/* Processor interface configuration 1 */
	mpc105_regs[0xac/4] = 0x060c000c;		/* Processor interface configuration 2 */
	mpc105_regs[0xb8/4] = 0x04000000;
	mpc105_regs[0xf0/4] = 0x0000ff02;		/* Memory control configuration 1 */
	mpc105_regs[0xf4/4] = 0x00030000;		/* Memory control configuration 2 */
	mpc105_regs[0xfc/4] = 0x00000010;		/* Memory control configuration 4 */
}

/*****************************************************************************/
/* Motorola MPC106 PCI Bridge/Memory Controller */

static UINT32 mpc106_regs[0x40];
static UINT32 mpc106_addr;

static READ64_HANDLER( mpc106_addr_r )
{
	if (!(mem_mask & U64(0xffffffff00000000)))
	{
		return (UINT64)mpc106_addr << 32;
	}
	return 0;
}

static WRITE64_HANDLER( mpc106_addr_w )
{
	if (!(mem_mask & U64(0xffffffff00000000)))
	{
		UINT32 d = BYTE_REVERSE32((UINT32)(data >> 32));

		if (((d >> 8) & 0xffffff) == 0x800000)
		{
			mpc106_addr = d & 0xff;
		}
		else
		{
			mpc106_addr = data >> 32;

			pci_bus = (d >> 16) & 0xff;
			pci_device = (d >> 11) & 0x1f;
			pci_function = (d >> 8) & 0x7;
			pci_reg = (d >> 2) & 0x3f;
		}
	}
}

static READ64_HANDLER( mpc106_data_r )
{
	if(pci_device == 0) {
		return ((UINT64)(BYTE_REVERSE32(mpc106_regs[(pci_reg/2)+1])) << 32) |
			   ((UINT64)(BYTE_REVERSE32(mpc106_regs[(pci_reg/2)+0])));
	}
	if (!(mem_mask & U64(0xffffffff00000000)))
	{
		return (UINT64)(BYTE_REVERSE32(pci_device_get_reg(pci_device, pci_reg))) << 32;
	}
	else
	{
		return (UINT64)(BYTE_REVERSE32(pci_device_get_reg(pci_device, pci_reg)));
	}
}

static WRITE64_HANDLER( mpc106_data_w )
{
	if(pci_device == 0) {
		mpc106_regs[(pci_reg/2)+1] = BYTE_REVERSE32((UINT32)(data >> 32));
		mpc106_regs[(pci_reg/2)+0] = BYTE_REVERSE32((UINT32)(data));
		return;
	}
	if (!(mem_mask & 0xffffffff))
	{
		pci_device_set_reg(pci_device, pci_reg, BYTE_REVERSE32((UINT32)data));
	}
}

static READ64_HANDLER( mpc106_reg_r )
{
	return ((UINT64)(mpc106_regs[(offset*2)+0]) << 32) |
			(UINT64)(mpc106_regs[(offset*2)+1]);
}

static WRITE64_HANDLER( mpc106_reg_w )
{
	mpc106_regs[(offset*2)+0] = (UINT32)(data >> 32);
	mpc106_regs[(offset*2)+1] = (UINT32)data;
}

static void mpc106_init(void)
{
	/* set reset values */
	memset(mpc106_regs, 0, sizeof(mpc106_regs));
	mpc106_regs[0x00/4] = 0x00021057;		/* Vendor ID & Device ID */
	mpc106_regs[0x04/4] = 0x00800006;		/* PCI Command & PCI Status */
	mpc106_regs[0x08/4] = 0x00060000;		/* Class code */
	mpc106_regs[0x0c/4] = 0x00000800;		/* Cache line size */
	mpc106_regs[0x70/4] = 0x00cd0000;		/* Output driver control */
	mpc106_regs[0xa8/4] = 0x0010ff00;		/* Processor interface configuration 1 */
	mpc106_regs[0xac/4] = 0x060c000c;		/* Processor interface configuration 2 */
	mpc106_regs[0xb8/4] = 0x04000000;
	mpc106_regs[0xc0/4] = 0x00000100;		/* Error enabling 1 */
	mpc106_regs[0xe0/4] = 0x00420fff;		/* Emulation support configuration 1 */
	mpc106_regs[0xe8/4] = 0x00200000;		/* Emulation support configuration 2 */
	mpc106_regs[0xf0/4] = 0x0000ff02;		/* Memory control configuration 1 */
	mpc106_regs[0xf4/4] = 0x00030000;		/* Memory control configuration 2 */
	mpc106_regs[0xfc/4] = 0x00000010;		/* Memory control configuration 4 */
}

/*****************************************************************************/

static READ64_HANDLER(scsi_r)
{
	int reg = offset*8;
	UINT64 r = 0;
	if (!(mem_mask & U64(0xff00000000000000))) {
		r |= (UINT64)lsi53c810_reg_r(reg+0) << 56;
	}
	if (!(mem_mask & U64(0x00ff000000000000))) {
		r |= (UINT64)lsi53c810_reg_r(reg+1) << 48;
	}
	if (!(mem_mask & U64(0x0000ff0000000000))) {
		r |= (UINT64)lsi53c810_reg_r(reg+2) << 40;
	}
	if (!(mem_mask & U64(0x000000ff00000000))) {
		r |= (UINT64)lsi53c810_reg_r(reg+3) << 32;
	}
	if (!(mem_mask & U64(0x00000000ff000000))) {
		r |= (UINT64)lsi53c810_reg_r(reg+4) << 24;
	}
	if (!(mem_mask & U64(0x0000000000ff0000))) {
		r |= (UINT64)lsi53c810_reg_r(reg+5) << 16;
	}
	if (!(mem_mask & U64(0x000000000000ff00))) {
		r |= (UINT64)lsi53c810_reg_r(reg+6) << 8;
	}
	if (!(mem_mask & U64(0x00000000000000ff))) {
		r |= (UINT64)lsi53c810_reg_r(reg+7) << 0;
	}

	return r;
}

static WRITE64_HANDLER(scsi_w)
{
	int reg = offset*8;
	if (!(mem_mask & U64(0xff00000000000000))) {
		lsi53c810_reg_w(reg+0, data >> 56);
	}
	if (!(mem_mask & U64(0x00ff000000000000))) {
		lsi53c810_reg_w(reg+1, data >> 48);
	}
	if (!(mem_mask & U64(0x0000ff0000000000))) {
		lsi53c810_reg_w(reg+2, data >> 40);
	}
	if (!(mem_mask & U64(0x000000ff00000000))) {
		lsi53c810_reg_w(reg+3, data >> 32);
	}
	if (!(mem_mask & U64(0x00000000ff000000))) {
		lsi53c810_reg_w(reg+4, data >> 24);
	}
	if (!(mem_mask & U64(0x0000000000ff0000))) {
		lsi53c810_reg_w(reg+5, data >> 16);
	}
	if (!(mem_mask & U64(0x000000000000ff00))) {
		lsi53c810_reg_w(reg+6, data >> 8);
	}
	if (!(mem_mask & U64(0x00000000000000ff))) {
		lsi53c810_reg_w(reg+7, data >> 0);
	}
}

static UINT32 scsi_fetch(UINT32 dsp)
{
	UINT32 result;
	result = program_read_dword_64be(dsp);
	return BYTE_REVERSE32(result);
}

static void scsi_irq_callback(void)
{
	model3_irq_state |= model3_irq_enable & ~0x60;	/* FIXME: enable only SCSI interrupt */
	cpunum_set_input_line(0, INPUT_LINE_IRQ1, ASSERT_LINE);
}

/*****************************************************************************/
/* Real3D DMA */

static UINT32 dma_data;
static UINT32 dma_status;
static UINT32 dma_source;
static UINT32 dma_dest;
static UINT32 dma_endian;
static UINT32 dma_irq;

static READ64_HANDLER( real3d_dma_r )
{
	switch(offset)
	{
		case 1:
			return (dma_irq << 24) | (dma_endian << 8);
		case 2:
			if(!(mem_mask & U64(0x00000000ffffffff))) {
				return dma_data;
			}
			break;
	}
	printf("real3d_dma_r: %08X, %08X%08X\n", offset, (UINT32)(mem_mask >> 32), (UINT32)(mem_mask));
	return 0;
}

static WRITE64_HANDLER( real3d_dma_w )
{
	switch(offset)
	{
		case 0:
			if(!(mem_mask & U64(0xffffffff00000000))) {		/* DMA source address */
				dma_source = BYTE_REVERSE32((UINT32)(data >> 32));
				return;
			}
			if(!(mem_mask & U64(0x00000000ffffffff))) {		/* DMA destination address */
				dma_dest = BYTE_REVERSE32((UINT32)(data));
				return;
			}
			break;
		case 1:
			if(!(mem_mask & U64(0xffffffff00000000)))		/* DMA length */
			{
				int length = BYTE_REVERSE32((UINT32)(data >> 32)) * 4;
				if (dma_endian & 0x80)
				{
					real3d_dma_callback(dma_source, dma_dest, length, 0);
				}
				else
				{
					real3d_dma_callback(dma_source, dma_dest, length, 1);
				}
				dma_irq |= 0x01;
				scsi_irq_callback();
				return;
			}
			else if(!(mem_mask & U64(0x0000000000ff0000)))
			{
				if(data & 0x10000) {
					dma_irq &= ~0x1;
				}
				return;
			}
			else if(!(mem_mask & U64(0x000000000000ff00)))
			{
				dma_endian = (data >> 8) & 0xff;
				return;
			}
			break;
		case 2:
			if(!(mem_mask & U64(0xffffffff00000000))) {		/* DMA command */
				UINT32 cmd = BYTE_REVERSE32((UINT32)(data >> 32));
				if(cmd & 0x20000000) {
					dma_data = BYTE_REVERSE32(real3d_device_id);	/* (PCI Vendor & Device ID) */
				}
				else if(cmd & 0x80000000) {
					dma_status ^= 0xffffffff;
					dma_data = dma_status;
				}
				return;
			}
			if(!(mem_mask & U64(0x00000000ffffffff))) {		/* ??? */
				dma_data = 0xffffffff;
				return;
			}
			return;
			break;
	}
	fatalerror("real3d_dma_w: %08X, %08X%08X, %08X%08X", offset, (UINT32)(data >> 32), (UINT32)(data), (UINT32)(mem_mask >> 32), (UINT32)(mem_mask));
}

static void real3d_dma_callback(UINT32 src, UINT32 dst, int length, int byteswap)
{
	switch(dst >> 24)
	{
		case 0x88:		/* Display List End Trigger */
			real3d_display_list_end();
			break;
		case 0x8c:		/* Display List RAM 2 */
			real3d_display_list2_dma(src, dst, length, byteswap);
			break;
		case 0x8e:		/* Display List RAM 1 */
			real3d_display_list1_dma(src, dst, length, byteswap);
			break;
		case 0x90:		/* VROM Texture Download */
			real3d_vrom_texture_dma(src, dst, length, byteswap);
			break;
		case 0x94:		/* Texture FIFO */
			real3d_texture_fifo_dma(src, length, byteswap);
			break;
		case 0x98:		/* Polygon RAM */
			real3d_polygon_ram_dma(src, dst, length, byteswap);
			break;
		case 0x9c:		/* Unknown */
			break;
		default:
			fatalerror("dma_callback: %08X, %08X, %d at %08X", src, dst, length, activecpu_get_pc());
			break;
	}
}

/*****************************************************************************/

/* this is a 93C46 but with reset delay that is needed by Lost World */
static struct EEPROM_interface eeprom_interface =
{
	6,				/* address bits */
	16,				/* data bits */
	"*110",			/*  read command */
	"*101",			/* write command */
	"*111",			/* erase command */
	"*10000xxxx",	/* lock command */
	"*10011xxxx",	/* unlock command */
	1,				/* enable_multi_read */
	5				/* reset_delay (Lost World needs this, very similar to wbeachvl in playmark.c) */
};

static void eeprom_handler(mame_file *file,int read_or_write)
{
	if (read_or_write)
		EEPROM_save(file);
	else
	{
		EEPROM_init(&eeprom_interface);
		if (file)	EEPROM_load(file);
	}
}

static NVRAM_HANDLER( model3 )
{
	if( mame_stricmp(Machine->gamedrv->name, "lostwsga") == 0 ||
		mame_stricmp(Machine->gamedrv->name, "dirtdvls") == 0 ||
		mame_stricmp(Machine->gamedrv->name, "dirtdvla") == 0 ||
		mame_stricmp(Machine->gamedrv->name, "lemans24") == 0 ||
		mame_stricmp(Machine->gamedrv->name, "von2") == 0 ||
		mame_stricmp(Machine->gamedrv->name, "von254g") == 0)
	{
		eeprom_handler(file, read_or_write);
	} else {
		nvram_handler_93C46(file, read_or_write);
	}

	if (read_or_write)
	{
		mame_fwrite(file, model3_backup, 0x1ffff);
	}
	else
	{
		if (file)
		{
			mame_fread(file, model3_backup, 0x1ffff);
		}
	}
}

static SCSIConfigTable scsi_dev_table =
{
	1, /* 1 SCSI device */
	{
		{ SCSI_ID_0, 0, SCSI_DEVICE_HARDDISK } /* SCSI ID 0, using HD 0, HD */
	}
};

static struct LSI53C810interface scsi_intf =
{
	&scsi_dev_table,  /* SCSI device table */
	&scsi_irq_callback,
	&real3d_dma_callback,
	&scsi_fetch,
};

static void model3_init(int step)
{
	model3_step = step;
	memory_set_bankptr( 1, memory_region( REGION_USER1 ) + 0x800000 ); /* banked CROM */

	model3_machine_init(step);	// step 1.5
	model3_tap_reset();

	if(step < 0x20) {
		if( mame_stricmp(Machine->gamedrv->name, "vs215") == 0 ||
			mame_stricmp(Machine->gamedrv->name, "vs29815") == 0 ||
			mame_stricmp(Machine->gamedrv->name, "bass") == 0 )
		{
			mpc106_init();
		} else {
			mpc105_init();
		}
		lsi53c810_init(&scsi_intf);
		real3d_device_id = 0x16c311db;	/* PCI Vendor ID (11db = SEGA), Device ID (16c3 = 315-5827) */
	}
	else {
		mpc106_init();
		real3d_device_id = 0x178611db;	/* PCI Vendor ID (11db = SEGA), Device ID (1786 = 315-6022) */
	}
}

static MACHINE_RESET(model3_10) { model3_init(0x10); }
static MACHINE_RESET(model3_15) { model3_init(0x15); }
static MACHINE_RESET(model3_20) { model3_init(0x20); }
static MACHINE_RESET(model3_21) { model3_init(0x21); }

static UINT32 eeprom_bit = 0;

static UINT64 controls_2;
static UINT64 controls_3;
static UINT8 model3_serial_fifo1;
static UINT8 model3_serial_fifo2;
static int lightgun_reg_sel;
static int adc_channel;

static READ64_HANDLER( model3_ctrl_r )
{
	switch( offset )
	{
		case 0:
			if (!(mem_mask & U64(0xff00000000000000)))
			{
				return (UINT64)model3_controls_bank << 56;
			}
			else if (!(mem_mask & 0xff000000))
			{
				if(model3_controls_bank & 0x1) {
					eeprom_bit = EEPROM_read_bit() << 5;
					return ((readinputport(1) & ~0x20) | eeprom_bit) << 24;
				}
				else {
					return (readinputport(0)) << 24;
				}
			}
			break;

		case 1:
			if (!(mem_mask & U64(0xff00000000000000)))
			{
				return (UINT64)readinputport(2) << 56;
			}
			else if (!(mem_mask & 0xff000000))
			{
				return readinputport(3) << 24;
			}
			break;

		case 2:
			return U64(0xffffffffffffffff);

		case 3:
			return U64(0xffffffffffffffff);		/* Dip switches */

		case 4:
			return U64(0xffffffffffffffff);

		case 5:
			if (!(mem_mask & 0xff000000))					/* Serial comm RX FIFO 1 */
			{
				return (UINT64)model3_serial_fifo1 << 24;
			}
			break;

		case 6:
			if (!(mem_mask & U64(0xff00000000000000)))		/* Serial comm RX FIFO 2 */
			{
				return (UINT64)model3_serial_fifo2 << 56;
			}
			else if (!(mem_mask & 0xff000000))				/* Serial comm full/empty flags */
			{
				return 0x0c << 24;
			}
			break;

		case 7:
			if (!(mem_mask & 0xff000000))		/* ADC Data read */
			{
				UINT8 adc_data = readinputport(5 + adc_channel);
				adc_channel++;
				adc_channel &= 0x7;
				return (UINT64)adc_data << 24;
			}
			break;
	}

	fatalerror("ctrl_r: %02X, %08X%08X", offset, (UINT32)(mem_mask >> 32), (UINT32)(mem_mask));
	return 0;
}

static WRITE64_HANDLER( model3_ctrl_w )
{
	switch(offset)
	{
		case 0:
			if (!(mem_mask & U64(0xff00000000000000)))
			{
				int reg = (data >> 56) & 0xff;
				EEPROM_write_bit((reg & 0x20) ? 1 : 0);
				EEPROM_set_clock_line((reg & 0x80) ? ASSERT_LINE : CLEAR_LINE);
				EEPROM_set_cs_line((reg & 0x40) ? CLEAR_LINE : ASSERT_LINE);
				model3_controls_bank = reg & 0xff;
			}
			return;

		case 2:
			COMBINE_DATA(&controls_2);
			return;

		case 3:
			COMBINE_DATA(&controls_3);
			return;

		case 4:
			if (!(mem_mask & U64(0xff00000000000000)))	/* Port 4 direction */
			{

			}
			if (!(mem_mask & 0xff000000))				/* Serial comm TX FIFO 1 */
			{											/* Used for reading the light gun in Lost World */
				switch(data >> 24)
				{
					case 0x00:
						lightgun_reg_sel = model3_serial_fifo2;
						break;
					case 0x87:
						model3_serial_fifo1 = 0;
						switch(lightgun_reg_sel)		/* read lightrun register */
						{
							case 0:		/* player 1 gun X-position, lower 8-bits */
								model3_serial_fifo2 = readinputport(6) & 0xff;
								break;
							case 1:		/* player 1 gun X-position, upper 2-bits */
								model3_serial_fifo2 = (readinputport(6) >> 8) & 0x3;
								break;
							case 2:		/* player 1 gun Y-position, lower 8-bits */
								model3_serial_fifo2 = readinputport(5) & 0xff;
								break;
							case 3:		/* player 1 gun Y-position, upper 2-bits */
								model3_serial_fifo2 = (readinputport(5) >> 8) & 0x3;
								break;
							case 4:		/* player 2 gun X-position, lower 8-bits */
								model3_serial_fifo2 = readinputport(8) & 0xff;
								break;
							case 5:		/* player 2 gun X-position, upper 2-bits */
								model3_serial_fifo2 = (readinputport(8) >> 8) & 0x3;
								break;
							case 6:		/* player 2 gun Y-position, lower 8-bits */
								model3_serial_fifo2 = readinputport(7) & 0xff;
								break;
							case 7:		/* player 2 gun Y-position, upper 2-bits */
								model3_serial_fifo2 = (readinputport(7) >> 8) & 0x3;
								break;
							case 8:		/* gun offscreen (bit set = gun offscreen, bit clear = gun on screen) */
								model3_serial_fifo2 = 0;	/* bit 0 = player 1, bit 1 = player 2 */
								if(readinputport(9) & 0x1) {
									model3_serial_fifo2 |= 0x01;
								}
								break;
						}
						break;
					default:
						//printf("Lightgun: Unknown command %02X at %08X\n", (UINT32)(data >> 24), activecpu_get_pc());
						break;
				}
			}
			return;

		case 5:
			if (!(mem_mask & U64(0xff00000000000000)))	/* Serial comm TX FIFO 2 */
			{
				model3_serial_fifo2 = data >> 56;
				return;
			}
			break;

		case 7:
			if (!(mem_mask & U64(0xff000000)))	/* ADC Channel selection */
			{
				adc_channel = (data >> 24) & 0xf;
			}
			return;
	}

	fatalerror("ctrl_w: %02X, %08X%08X, %08X%08X", offset, (UINT32)(data >> 32), (UINT32)(data), (UINT32)(mem_mask >> 32), (UINT32)(mem_mask));
}

static READ64_HANDLER( model3_sys_r )
{
	switch (offset)
	{
		case 0x08/8:
			if (!(mem_mask & U64(0xff00000000000000)))
			{
				return ((UINT64)model3_crom_bank << 56);
			}
			break;

		case 0x10/8:
			if (!(mem_mask & U64(0xff00000000000000)))
			{
				UINT64 res = model3_tap_read();

				return res<<61;
			}
			else if (!(mem_mask & 0xff000000))
			{
				return (model3_irq_enable<<24);
			}
			else logerror("m3_sys: Unk sys_r @ 0x10: mask = %x\n", (UINT32)mem_mask);
			break;
		case 0x18/8:
			return (UINT64)model3_irq_state<<56 | 0xff000000;
			break;
	}

	logerror("Unknown model3 sys_r: offs %08X mask %08X\n", offset, (UINT32)mem_mask);
	return 0;
}

static WRITE64_HANDLER( model3_sys_w )
{
	switch (offset)
	{
		case 0x10/8:
			if (!(mem_mask & 0xff000000))
			{
				model3_irq_enable = (data>>24)&0xff;
			}
			else logerror("m3_sys: unknown mask on IRQen write\n");
			break;
		case 0x08/8:
			if (!(mem_mask & U64(0xff00000000000000)))
			{
				model3_crom_bank = data >> 56;

				data >>= 56;
				data = (~data) & 0xf;
				memory_set_bankptr( 1, memory_region( REGION_USER1 ) + 0x800000 + (data * 0x800000)); /* banked CROM */
			}
			if (!(mem_mask & 0xff000000))
			{
				data >>= 24;
				model3_tap_write(
					(data >> 6) & 1,// TCK
					(data >> 2) & 1,// TMS
					(data >> 5) & 1,// TDI
					(data >> 7) & 1	// TRST
					);
			}
			break;
	}
}

static READ64_HANDLER( model3_rtc_r )
{
	UINT64 r = 0;
	if(!(mem_mask & U64(0xff00000000000000))) {
		r |= (UINT64)rtc72421_r((offset*2)+0, (UINT32)(mem_mask >> 32)) << 32;
	}
	if(!(mem_mask & U64(0x00000000ff000000))) {
		r |= (UINT64)rtc72421_r((offset*2)+1, (UINT32)(mem_mask));
	}
	return r;
}

static WRITE64_HANDLER( model3_rtc_w )
{
	if(!(mem_mask & U64(0xff00000000000000))) {
		rtc72421_w((offset*2)+0, (UINT32)(data >> 32), (UINT32)(mem_mask >> 32));
	}
	if(!(mem_mask & U64(0x00000000ff000000))) {
		rtc72421_w((offset*2)+1, (UINT32)(data), (UINT32)(mem_mask));
	}
}

static UINT64 real3d_status = 0;
static READ64_HANDLER(real3d_status_r)
{
	real3d_status ^= U64(0xffffffffffffffff);
	return real3d_status;
}

/* SCSP interface */
static READ64_HANDLER(model3_sound_r)
{
	return U64(0xffffffffffffffff);
}

static WRITE64_HANDLER(model3_sound_w)
{
	model3_irq_state &= ~0x40;
}



static UINT64 network_ram[0x10000];
static READ64_HANDLER(network_r)
{
	printf("network_r: %02X at %08X\n", offset, activecpu_get_pc());
	return network_ram[offset];
}

static WRITE64_HANDLER(network_w)
{
	COMBINE_DATA(network_ram + offset);
	printf("network_w: %02X, %08X%08X at %08X\n", offset, (UINT32)(data >> 32), (UINT32)(data), activecpu_get_pc());
}

static int prot_data_ptr = 0;

static UINT16 vs299_prot_data[] =
{
	0xc800, 0x4a20, 0x5041, 0x4e41, 0x4920, 0x4154, 0x594c, 0x4220,
	0x4152, 0x4953, 0x204c, 0x5241, 0x4547, 0x544e, 0x4e49, 0x2041,
	0x4547, 0x4d52, 0x4e41, 0x2059, 0x4e45, 0x4c47, 0x4e41, 0x2044,
	0x454e, 0x4854, 0x5245, 0x414c, 0x444e, 0x2053, 0x5246, 0x4e41,
	0x4543, 0x4320, 0x4c4f, 0x4d4f, 0x4942, 0x2041, 0x4150, 0x4152,
	0x5547, 0x5941, 0x4220, 0x4c55, 0x4147, 0x4952, 0x2041, 0x5053,
	0x4941, 0x204e, 0x5243, 0x414f, 0x4954, 0x2041, 0x4542, 0x474c,
	0x5549, 0x204d, 0x494e, 0x4547, 0x4952, 0x2041, 0x4153, 0x4455,
	0x2049, 0x4f4b, 0x4552, 0x2041, 0x4544, 0x4d4e, 0x5241, 0x204b,
	0x4f52, 0x414d, 0x494e, 0x2041, 0x4353, 0x544f, 0x414c, 0x444e,
	0x5520, 0x4153, 0x5320, 0x554f, 0x4854, 0x4641, 0x4952, 0x4143,
	0x4d20, 0x5845, 0x4349, 0x204f, 0x5559, 0x4f47, 0x4c53, 0x5641,
	0x4149, 0x4620, 0x5f43, 0x4553, 0x4147
};

static UINT16 swt_prot_data[] =
{
	0xffff,
	0x3d3d, 0x3d3d, 0x203d, 0x5453, 0x5241, 0x5720, 0x5241, 0x2053,
	0x3d3d, 0x3d3d, 0x0a3d, 0x6f43, 0x7970, 0x6952, 0x6867, 0x2074,
	0x4553, 0x4147, 0x4520, 0x746e, 0x7265, 0x7270, 0x7369, 0x7365,
	0x202c, 0x744c, 0x2e64, 0x410a, 0x756d, 0x6573, 0x656d, 0x746e,
	0x5220, 0x4426, 0x4420, 0x7065, 0x2e74, 0x2320, 0x3231, 0x4b0a,
	0x7461, 0x7573, 0x6179, 0x7573, 0x4120, 0x646e, 0x206f, 0x2026,
	0x614b, 0x6f79, 0x6f6b, 0x5920, 0x6d61, 0x6d61, 0x746f, 0x0a6f,
};

static UINT16 fvipers2_prot_data[] =
{
	0x2a2a,
	0x2a2a, 0x2a2a, 0x2a2a, 0x2a2a, 0x2a2a, 0x2a2a, 0x202a, 0x5b5b,
	0x4620, 0x6769, 0x7468, 0x6e69, 0x2067, 0x6956, 0x6570, 0x7372,
	0x3220, 0x5d20, 0x205d, 0x6e69, 0x3c20, 0x4d3c, 0x444f, 0x4c45,
	0x332d, 0x3e3e, 0x4320, 0x706f, 0x7279, 0x6769, 0x7468, 0x2820,
	0x2943, 0x3931, 0x3839, 0x5320, 0x4745, 0x2041, 0x6e45, 0x6574,
	0x7072, 0x6972, 0x6573, 0x2c73, 0x544c, 0x2e44, 0x2020, 0x4120,
	0x6c6c, 0x7220, 0x6769, 0x7468, 0x7220, 0x7365, 0x7265, 0x6576,
	0x2e64, 0x2a20, 0x2a2a, 0x2a2a, 0x2a2a, 0x2a2a, 0x2a2a, 0x2a2a,
};

static UINT16 spikeout_prot_data[] =
{
	0x0000,
	0x4f4d, 0x4544, 0x2d4c, 0x2033, 0x7953, 0x7473, 0x6d65, 0x5020,
	0x6f72, 0x7267, 0x6d61, 0x4320, 0x706f, 0x7279, 0x6769, 0x7468,
	0x2820, 0x2943, 0x3120, 0x3939, 0x2035, 0x4553, 0x4147, 0x4520,
	0x746e, 0x7265, 0x7270, 0x7369, 0x7365, 0x4c2c, 0x4454, 0x202e,
	0x6c41, 0x206c, 0x6972, 0x6867, 0x2074, 0x6572, 0x6573, 0x7672,
	0x6465, 0x202e, 0x2020, 0x0020
};

static UINT16 eca_prot_data[] =
{
	0x0000,
    0x2d2f, 0x202d, 0x4d45, 0x5245, 0x4547, 0x434e, 0x2059, 0x4143,
    0x4c4c, 0x4120, 0x424d, 0x4c55, 0x4e41, 0x4543, 0x2d20, 0x0a2d,
    0x6f43, 0x7970, 0x6952, 0x6867, 0x2074, 0x4553, 0x4147, 0x4520,
    0x746e, 0x7265, 0x7270, 0x7369, 0x7365, 0x202c, 0x744c, 0x2e64,
    0x530a, 0x666f, 0x7774, 0x7261, 0x2065, 0x2652, 0x2044, 0x6544,
    0x7470, 0x202e, 0x3123, 0x660a, 0x726f, 0x7420, 0x7365, 0x0a74,
};

static READ64_HANDLER(model3_security_r)
{
	switch(offset)
	{
		case 0x00/8:	return 0;		/* status */
		case 0x1c/8:					/* security board data read */
		{
			if (mame_stricmp(Machine->gamedrv->name, "vs299") == 0 ||
				mame_stricmp(Machine->gamedrv->name, "vs2v991") == 0)
			{
				return (UINT64)vs299_prot_data[prot_data_ptr++] << 48;
			}
			else if (mame_stricmp(Machine->gamedrv->name, "swtrilgy") == 0 ||
					 mame_stricmp(Machine->gamedrv->name, "swtrilga") == 0)
			{
				UINT64 data = (UINT64)swt_prot_data[prot_data_ptr++] << 16;
				if (prot_data_ptr > 0x38)
				{
					prot_data_ptr = 0;
				}
				return data;
			}
			else if (mame_stricmp(Machine->gamedrv->name, "fvipers2") == 0)
			{
				UINT64 data = (UINT64)fvipers2_prot_data[prot_data_ptr++] << 16;
				if (prot_data_ptr >= 0x41)
				{
					prot_data_ptr = 0;
				}
				return data;
			}
			else if (mame_stricmp(Machine->gamedrv->name, "spikeout") == 0 ||
					 mame_stricmp(Machine->gamedrv->name, "spikeofe") == 0)
			{
				UINT64 data = (UINT64)spikeout_prot_data[prot_data_ptr++] << 16;
				if (prot_data_ptr >= 0x55)
				{
					prot_data_ptr = 0;
				}
				return data;
			}
			else if (mame_stricmp(Machine->gamedrv->name, "eca") == 0)
			{
				UINT64 data = (UINT64)eca_prot_data[prot_data_ptr++] << 16;
				if (prot_data_ptr >= 0x31)
				{
					prot_data_ptr = 0;
				}
				return data;
			}
			else
			{
				return U64(0xffffffffffffffff);
			}
		}
	}
	return U64(0xffffffffffffffff);
}

static WRITE64_HANDLER(daytona2_rombank_w)
{
	if (!(mem_mask & U64(0xff00000000000000)))
	{
		data >>= 56;
		data = (~data) & 0xf;
		memory_set_bankptr( 1, memory_region( REGION_USER1 ) + 0x800000 + (data * 0x800000)); /* banked CROM */
	}
}

static ADDRESS_MAP_START( model3_mem, ADDRESS_SPACE_PROGRAM, 64)
	AM_RANGE(0x00000000, 0x007fffff) AM_RAM	AM_BASE(&work_ram)	/* work RAM */

	AM_RANGE(0x84000000, 0x8400003f) AM_READ( real3d_status_r )
	AM_RANGE(0x88000000, 0x88000007) AM_WRITE( real3d_cmd_w )
	AM_RANGE(0x8e000000, 0x8e0fffff) AM_WRITE( real3d_display_list_w )
	AM_RANGE(0x98000000, 0x980fffff) AM_WRITE( real3d_polygon_ram_w )

	AM_RANGE(0xf0040000, 0xf004003f) AM_READWRITE( model3_ctrl_r, model3_ctrl_w )
	AM_RANGE(0xf0080000, 0xf0080007) AM_READWRITE( model3_sound_r, model3_sound_w )
	AM_RANGE(0xf00c0000, 0xf00dffff) AM_RAM	AM_BASE(&model3_backup)	/* backup SRAM */
	AM_RANGE(0xf0100000, 0xf010003f) AM_READWRITE( model3_sys_r, model3_sys_w )
	AM_RANGE(0xf0140000, 0xf014003f) AM_READWRITE( model3_rtc_r, model3_rtc_w )

	AM_RANGE(0xf1000000, 0xf10f7fff) AM_READWRITE( model3_char_r, model3_char_w )	/* character RAM */
	AM_RANGE(0xf10f8000, 0xf10fffff) AM_READWRITE( model3_tile_r, model3_tile_w )	/* tilemaps */
	AM_RANGE(0xf1100000, 0xf111ffff) AM_READWRITE( model3_palette_r, model3_palette_w ) AM_BASE(&paletteram64) /* palette */
	AM_RANGE(0xf1180000, 0xf11800ff) AM_READWRITE( model3_vid_reg_r, model3_vid_reg_w )

	AM_RANGE(0xfe040000, 0xfe04003f) AM_READWRITE( model3_ctrl_r, model3_ctrl_w )
	AM_RANGE(0xfe080000, 0xfe080007) AM_READWRITE( model3_sound_r, model3_sound_w )
	AM_RANGE(0xfe0c0000, 0xfe0dffff) AM_RAM	AM_BASE(&model3_backup)	/* backup SRAM */
	AM_RANGE(0xfe100000, 0xfe10003f) AM_READWRITE( model3_sys_r, model3_sys_w )
	AM_RANGE(0xfe140000, 0xfe14003f) AM_READWRITE( model3_rtc_r, model3_rtc_w )

	AM_RANGE(0xfe180000, 0xfe19ffff) AM_RAM							/* Security Board RAM */
	AM_RANGE(0xfe1a0000, 0xfe1a003f) AM_READ( model3_security_r )	/* Security board */

	AM_RANGE(0xff800000, 0xffffffff) AM_ROM AM_REGION(REGION_USER1, 0)
ADDRESS_MAP_END

#define MODEL3_SYSTEM_CONTROLS_1 \
PORT_START_TAG("IN0") \
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 ) \
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 ) \
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_SERVICE ) PORT_NAME( DEF_STR( Service_Mode )) PORT_CODE(KEYCODE_F2) /* Test Button */ \
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_SERVICE1 ) \
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_START1 ) \
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_START2 ) \
	PORT_BIT( 0xc0, IP_ACTIVE_LOW, IPT_UNKNOWN )

#define MODEL3_SYSTEM_CONTROLS_2 \
PORT_START_TAG("IN1") \
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_SERVICE ) PORT_NAME("Service Button B") PORT_CODE(KEYCODE_8) \
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_SERVICE ) PORT_NAME("Test Button B") PORT_CODE(KEYCODE_7) \
	PORT_BIT( 0x3f, IP_ACTIVE_LOW, IPT_UNUSED )


INPUT_PORTS_START( model3 )
	MODEL3_SYSTEM_CONTROLS_1
	MODEL3_SYSTEM_CONTROLS_2

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(1)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(1)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_PLAYER(1)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON4 ) PORT_PLAYER(1)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY PORT_PLAYER(1)

	PORT_START
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )		/* Dip switches */
INPUT_PORTS_END

INPUT_PORTS_START( lostwsga )
	MODEL3_SYSTEM_CONTROLS_1
	MODEL3_SYSTEM_CONTROLS_2

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(1)
	PORT_BIT( 0xfe, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )		/* Dip switches */

	PORT_START	// lightgun X-axis
	PORT_BIT( 0x3ff, 0x200, IPT_LIGHTGUN_X ) PORT_MINMAX(0x00,0x3ff) PORT_SENSITIVITY(50) PORT_KEYDELTA(10) PORT_PLAYER(1)

	PORT_START	// lightgun Y-axis
	PORT_BIT( 0x3ff, 0x200, IPT_LIGHTGUN_Y ) PORT_MINMAX(0x00,0x3ff) PORT_SENSITIVITY(50) PORT_KEYDELTA(10) PORT_PLAYER(1)

	PORT_START	// lightgun X-axis
	PORT_BIT( 0x3ff, 0x200, IPT_LIGHTGUN_X ) PORT_MINMAX(0x00,0x3ff) PORT_SENSITIVITY(50) PORT_KEYDELTA(10) PORT_PLAYER(2)

	PORT_START	// lightgun Y-axis
	PORT_BIT( 0x3ff, 0x200, IPT_LIGHTGUN_Y ) PORT_MINMAX(0x00,0x3ff) PORT_SENSITIVITY(50) PORT_KEYDELTA(10) PORT_PLAYER(2)

	PORT_START	// fake button to shoot offscreen
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_BUTTON2 ) PORT_PLAYER(1)
INPUT_PORTS_END

INPUT_PORTS_START( scud )
	MODEL3_SYSTEM_CONTROLS_1
	MODEL3_SYSTEM_CONTROLS_2

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON1 )	/* View Button 1 */
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON2 )	/* View Button 2 */
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON3 )	/* View Button 3 */
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON4 )	/* View Button 4 */
	PORT_BIT( 0x50, IP_ACTIVE_LOW, IPT_BUTTON5 )	/* Shift 1 */
	PORT_BIT( 0x60, IP_ACTIVE_LOW, IPT_BUTTON6 )	/* Shift 2 */
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON7 )	/* Shift 3 */
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON8 )	/* Shift 4 */

	PORT_START
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )		/* Dip switches */

	PORT_START	// steering
	PORT_BIT( 0xff, 0x80, IPT_PADDLE ) PORT_MINMAX(0x00,0xff) PORT_SENSITIVITY(30) PORT_KEYDELTA(10) PORT_PLAYER(1)

	PORT_START	// accelerator
	PORT_BIT( 0xff, 0x00, IPT_PEDAL ) PORT_MINMAX(0x00,0xff) PORT_SENSITIVITY(30) PORT_KEYDELTA(10) PORT_PLAYER(1)

	PORT_START	// brake
	PORT_BIT( 0xff, 0x00, IPT_PEDAL2 ) PORT_MINMAX(0x00,0xff) PORT_SENSITIVITY(30) PORT_KEYDELTA(10) PORT_PLAYER(1)
INPUT_PORTS_END

INPUT_PORTS_START( bass )
	MODEL3_SYSTEM_CONTROLS_1
	MODEL3_SYSTEM_CONTROLS_2

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(1)		/* Cast */
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(1)		/* Select */
	PORT_BIT( 0xfc, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )		/* Dip switches */

	PORT_START		/* Rod Y */
	PORT_BIT( 0xff, 0x80, IPT_AD_STICK_Y ) PORT_MINMAX(0x00,0xff) PORT_SENSITIVITY(30) PORT_KEYDELTA(10) PORT_PLAYER(1)

	PORT_START		/* Rod X */
	PORT_BIT( 0xff, 0x80, IPT_AD_STICK_X ) PORT_MINMAX(0x00,0xff) PORT_SENSITIVITY(30) PORT_KEYDELTA(10) PORT_PLAYER(1)

	PORT_START
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START		/* Reel */
	PORT_BIT( 0xff, 0x00, IPT_PEDAL ) PORT_MINMAX(0x00,0xff) PORT_SENSITIVITY(30) PORT_KEYDELTA(10) PORT_PLAYER(1)

	PORT_START		/* Stick Y */
	PORT_BIT( 0xff, 0x80, IPT_PADDLE_V ) PORT_MINMAX(0x00,0xff) PORT_SENSITIVITY(30) PORT_KEYDELTA(10) PORT_PLAYER(1)

	PORT_START		/* Stick X */
	PORT_BIT( 0xff, 0x80, IPT_PADDLE ) PORT_MINMAX(0x00,0xff) PORT_SENSITIVITY(30) PORT_KEYDELTA(10) PORT_PLAYER(1)
INPUT_PORTS_END

INPUT_PORTS_START( harley )
	MODEL3_SYSTEM_CONTROLS_1
	MODEL3_SYSTEM_CONTROLS_2

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON1 )	/* View Button 1 */
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON2 )	/* View Button 2 */
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON3 )	/* Shift down */
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON4 )	/* Shift up */
	PORT_BIT( 0xcc, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )		/* Dip switches */

	PORT_START	// steering
	PORT_BIT( 0xff, 0x80, IPT_PADDLE ) PORT_MINMAX(0x00,0xff) PORT_SENSITIVITY(30) PORT_KEYDELTA(10) PORT_PLAYER(1)

	PORT_START	// accelerator
	PORT_BIT( 0xff, 0x00, IPT_PEDAL ) PORT_MINMAX(0x00,0xff) PORT_SENSITIVITY(30) PORT_KEYDELTA(10) PORT_PLAYER(1)

	PORT_START	// front brake
	PORT_BIT( 0xff, 0x00, IPT_PEDAL2 ) PORT_MINMAX(0x00,0xff) PORT_SENSITIVITY(30) PORT_KEYDELTA(10) PORT_PLAYER(1)

	PORT_START	// back brake
	PORT_BIT( 0xff, 0x00, IPT_PEDAL3 ) PORT_MINMAX(0x00,0xff) PORT_SENSITIVITY(30) PORT_KEYDELTA(10) PORT_PLAYER(1)
INPUT_PORTS_END

INPUT_PORTS_START( daytona2 )
	MODEL3_SYSTEM_CONTROLS_1
	MODEL3_SYSTEM_CONTROLS_2

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON1 )	/* View Button 1 */
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON2 )	/* View Button 2 */
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON3 )	/* View Button 3 */
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON4 )	/* View Button 4 */
	PORT_BIT( 0x50, IP_ACTIVE_LOW, IPT_BUTTON5 )	/* Shift 1 */
	PORT_BIT( 0x60, IP_ACTIVE_LOW, IPT_BUTTON6 )	/* Shift 2 */
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON7 )	/* Shift 3 */
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON8 )	/* Shift 4 */

	PORT_START
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )		/* Dip switches */

	PORT_START	// steering
	PORT_BIT( 0xff, 0x80, IPT_PADDLE ) PORT_MINMAX(0x00,0xff) PORT_SENSITIVITY(30) PORT_KEYDELTA(10) PORT_PLAYER(1)

	PORT_START	// accelerator
	PORT_BIT( 0xff, 0x00, IPT_PEDAL ) PORT_MINMAX(0x00,0xff) PORT_SENSITIVITY(30) PORT_KEYDELTA(10) PORT_PLAYER(1)

	PORT_START	// brake
	PORT_BIT( 0xff, 0x00, IPT_PEDAL2 ) PORT_MINMAX(0x00,0xff) PORT_SENSITIVITY(30) PORT_KEYDELTA(10) PORT_PLAYER(1)
INPUT_PORTS_END

INPUT_PORTS_START( swtrilgy )
	MODEL3_SYSTEM_CONTROLS_1
	MODEL3_SYSTEM_CONTROLS_2

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(1)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(1)
	PORT_BIT( 0xde, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )		/* Dip switches */

	PORT_START		/* Analog Stick Y */
	PORT_BIT( 0xff, 0x80, IPT_AD_STICK_Y ) PORT_MINMAX(0x00,0xff) PORT_SENSITIVITY(30) PORT_KEYDELTA(30) PORT_PLAYER(1)

	PORT_START		/* Analog Stick X */
	PORT_BIT( 0xff, 0x80, IPT_AD_STICK_X ) PORT_MINMAX(0x00,0xff) PORT_SENSITIVITY(30) PORT_KEYDELTA(30) PORT_PLAYER(1)

INPUT_PORTS_END

INPUT_PORTS_START( eca )
	MODEL3_SYSTEM_CONTROLS_1
	MODEL3_SYSTEM_CONTROLS_2

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON1 )	/* View Change */
	PORT_BIT( 0x0e, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x50, IP_ACTIVE_LOW, IPT_BUTTON2 )	/* Shift Up */
	PORT_BIT( 0x60, IP_ACTIVE_LOW, IPT_BUTTON3 )	/* Shift Down */

	PORT_START
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )		/* Dip switches */

	PORT_START	// steering
	PORT_BIT( 0xff, 0x80, IPT_PADDLE ) PORT_MINMAX(0x00,0xff) PORT_SENSITIVITY(30) PORT_KEYDELTA(10) PORT_PLAYER(1)

	PORT_START	// accelerator
	PORT_BIT( 0xff, 0x00, IPT_PEDAL ) PORT_MINMAX(0x00,0xff) PORT_SENSITIVITY(30) PORT_KEYDELTA(10) PORT_PLAYER(1)

	PORT_START	// brake
	PORT_BIT( 0xff, 0x00, IPT_PEDAL2 ) PORT_MINMAX(0x00,0xff) PORT_SENSITIVITY(30) PORT_KEYDELTA(10) PORT_PLAYER(1)
INPUT_PORTS_END


#define ROM_LOAD64_WORD(name,offset,length,hash)      ROMX_LOAD(name, offset, length, hash, ROM_GROUPWORD | ROM_SKIP(6))
#define ROM_LOAD64_WORD_SWAP(name,offset,length,hash) ROMX_LOAD(name, offset, length, hash, ROM_GROUPWORD | ROM_REVERSE| ROM_SKIP(6))
#define ROM_LOAD_VROM(name, offset, length, hash)     ROMX_LOAD(name, offset, length, hash, ROM_GROUPWORD | ROM_SKIP(14) )
#define ROM_REGION64_BE(length,type,flags)	      ROM_REGION(length, type, (flags) | ROMREGION_64BIT | ROMREGION_BE)

ROM_START( lemans24 )	/* step 1.5 */
	ROM_REGION64_BE( 0x4800000, REGION_USER1, 0 ) /* program + data ROMs */
	// CROM
	ROM_LOAD64_WORD_SWAP( "epr-19887.17", 0x600006, 0x080000, CRC(2842bb87) SHA1(2acabf3f7281acaf6bab4d3bae9701df3909cf81) )
        ROM_LOAD64_WORD_SWAP( "epr-19888.18", 0x600004, 0x080000, CRC(800d763d) SHA1(4f2865a64d6dda638840d359db3bd2f22b6d1404) )
        ROM_LOAD64_WORD_SWAP( "epr-19889.19", 0x600002, 0x080000, CRC(d1f7e44c) SHA1(5faad711cf39c0fb10c3b9ccce25f5219ddd5a17) )
        ROM_LOAD64_WORD_SWAP( "epr-19890.20", 0x600000, 0x080000, CRC(9c16c3cc) SHA1(f1fba0c7cbdf7ddc4224356d50fdabea612c129d) )

	// CROM0
        ROM_LOAD64_WORD_SWAP( "mpr-19857.01", 0x800006, 0x400000, CRC(82c9fcfc) SHA1(31c38483b33606b11d74da21dc49df8fa9f227db) )
        ROM_LOAD64_WORD_SWAP( "mpr-19858.02", 0x800004, 0x400000, CRC(993fa656) SHA1(9090156ece06b11f3f24fbb96240eca44122a805) )
        ROM_LOAD64_WORD_SWAP( "mpr-19859.03", 0x800002, 0x400000, CRC(15906869) SHA1(7ad216ac6d048718e1a6c99f16c1a2f98db065b8) )
        ROM_LOAD64_WORD_SWAP( "mpr-19860.04", 0x800000, 0x400000, CRC(19a1ddc7) SHA1(c13277a419ff8f1a8bc532f00375f015a99a1d7f) )

	// mirror CROM0 to CROM
	ROM_COPY(REGION_USER1, 0x800000, 0x000000, 0x600000)

	// CROM1
        ROM_LOAD64_WORD_SWAP( "mpr-19861.05", 0x1800006, 0x400000, CRC(6ddf21b3) SHA1(462f987068ac7a1c8caaf356f44e0452ef0e8238) )
        ROM_LOAD64_WORD_SWAP( "mpr-19862.06", 0x1800004, 0x400000, CRC(b0f69ae4) SHA1(a4e785cda6f0101e11cd82ac9aa9934926234ba1) )
        ROM_LOAD64_WORD_SWAP( "mpr-19863.07", 0x1800002, 0x400000, CRC(2b2619d0) SHA1(895c8090a8149429ca41d6b94822462d8482a533) )
        ROM_LOAD64_WORD_SWAP( "mpr-19864.08", 0x1800000, 0x400000, CRC(c7baab2b) SHA1(abc629abf677e8adb3b4eff4cb6e6cd21254bdba) )

	// CROM2
        ROM_LOAD64_WORD_SWAP( "mpr-19865.09", 0x2800006, 0x400000, CRC(b2749d2b) SHA1(4e2a5d07eae2a2b2f9149328c68886df16389c2c) )
        ROM_LOAD64_WORD_SWAP( "mpr-19866.10", 0x2800004, 0x400000, CRC(ede5fc78) SHA1(ff170fad7aaf1a6ba86d50022ad7586d0e785668) )
        ROM_LOAD64_WORD_SWAP( "mpr-19867.11", 0x2800002, 0x400000, CRC(ae610fc5) SHA1(b03c85cb661a67becf59b6bb29e52de736470add) )
        ROM_LOAD64_WORD_SWAP( "mpr-19868.12", 0x2800000, 0x400000, CRC(3c43d64f) SHA1(00e1bd91496a6b3f73343ef4ad24a0dd3cb6bcf5) )

	ROM_REGION( 0x1000000, REGION_USER3, ROMREGION_DISPOSE )  /* Video ROMs Part 1 */
        ROM_LOAD_VROM( "mpr-19871.26", 0x000002, 0x200000, CRC(5168e02b) SHA1(3572c748c8f1b70b194fcf27919d3e671c7a09a5) )
        ROM_LOAD_VROM( "mpr-19872.27", 0x000000, 0x200000, CRC(9e65fc06) SHA1(7de713cf1161d921d0b19aa4af6ec0790f043424) )
        ROM_LOAD_VROM( "mpr-19873.28", 0x000006, 0x200000, CRC(0b15d7ab) SHA1(d3f42e096b9d9bd3b7b5905d2ae6f3205caa3c82) )
        ROM_LOAD_VROM( "mpr-19874.29", 0x000004, 0x200000, CRC(6a28ec89) SHA1(cd5ed506cb08a7420729157dba912a3e8e7a9076) )
        ROM_LOAD_VROM( "mpr-19875.30", 0x00000a, 0x200000, CRC(a03e1173) SHA1(45a083a2aeecc0bc250f9e025940525dfdf607b1) )
        ROM_LOAD_VROM( "mpr-19876.31", 0x000008, 0x200000, CRC(c93bb036) SHA1(d317fc726c2c8e72234a507406b0e8b8b93fe85a) )
        ROM_LOAD_VROM( "mpr-19877.32", 0x00000e, 0x200000, CRC(b1e3df56) SHA1(f9784b33c49b30612f5f4415c7be47198457a9e7) )
        ROM_LOAD_VROM( "mpr-19878.33", 0x00000c, 0x200000, CRC(a2acc111) SHA1(f06205dbbf1fae5b20712270131c12ad8b014c82) )

	ROM_REGION( 0x1000000, REGION_USER4, ROMREGION_DISPOSE )  /* Video ROMs Part 2 */
        ROM_LOAD_VROM( "mpr-19879.34", 0x000002, 0x200000, CRC(90c1553f) SHA1(1424774c2c2bc126bcde985ee4acb62254357b90) )
        ROM_LOAD_VROM( "mpr-19880.35", 0x000000, 0x200000, CRC(42504e63) SHA1(ab4690e1ea701d391e731d8491e8769030bdc689) )
        ROM_LOAD_VROM( "mpr-19881.36", 0x000006, 0x200000, CRC(d06985cf) SHA1(daae76c0bad5fbcc610525a13aea39289021d929) )
        ROM_LOAD_VROM( "mpr-19882.37", 0x000004, 0x200000, CRC(a86f2e2f) SHA1(9b39f49618b86ccf5a80d84c5e3d4d349a3866cd) )
        ROM_LOAD_VROM( "mpr-19883.38", 0x00000a, 0x200000, CRC(12895d6e) SHA1(347a85d0236fb5fed1a6b848425e905e8d5a3ddd) )
        ROM_LOAD_VROM( "mpr-19884.39", 0x000008, 0x200000, CRC(711eebfb) SHA1(3a14251399265f5c3d29830dd713efb49c8c2f2e) )
        ROM_LOAD_VROM( "mpr-19885.40", 0x00000e, 0x200000, CRC(d1ae5473) SHA1(c225ad47175247b4cc0d3db57d2ecb68242639d5) )
        ROM_LOAD_VROM( "mpr-19886.41", 0x00000c, 0x200000, CRC(278aae0b) SHA1(471a74ca21d0394742d0275029642c712a6bc924) )

	ROM_REGION( 0x100000, REGION_CPU2, 0 )	/* 68000 code */
        ROM_LOAD16_WORD_SWAP( "epr-19891.21", 0x080000, 0x080000, CRC(c3ecd448) SHA1(875ee429872f3a851fa0239e5c781870fa3f4323) )

	// Samples
	ROM_REGION( 0x800000, REGION_SOUND1, 0 )	/* SCSP samples */
        ROM_LOAD32_WORD( "mpr-19869.22", 0x000000, 0x400000, CRC(ea1ef1cc) SHA1(399c43659d83673f83b551b30b3b1410a75d8f8c) )
        ROM_LOAD32_WORD( "mpr-19870.24", 0x000002, 0x400000, CRC(49c70296) SHA1(9bf88a63c38d318006a9c6c6b7b4452439df876c) )

	ROM_REGION( 0x20000, REGION_CPU3, 0 )	/* Z80 code */
		ROM_FILL( 0x000000, 0x20000, 0 )

	ROM_REGION( 0x800000, REGION_SOUND2, 0 )	/* DSB samples */
		ROM_FILL( 0x000000, 0x800000, 0 )
ROM_END

ROM_START( scud )	/* step 1.5 */
	ROM_REGION64_BE( 0x4800000, REGION_USER1, 0 ) /* program + data ROMs */
	// CROM
	ROM_LOAD64_WORD_SWAP( "epr-19731.17",  0x0600006,  0x80000,  CRC(3EE6447E) SHA1(124697791d90c1b352dd6e33bd3b45535aa92bb5) )
	ROM_LOAD64_WORD_SWAP( "epr-19732.18",  0x0600004,  0x80000,  CRC(23E864BB) SHA1(0f34d963ee681ca1006f3dec12b593d961e3e442) )
	ROM_LOAD64_WORD_SWAP( "epr-19733.19",  0x0600002,  0x80000,  CRC(6565E29A) SHA1(4fb4f3e77fa46a825900a63095307714e71c08f3) )
	ROM_LOAD64_WORD_SWAP( "epr-19734.20",  0x0600000,  0x80000,  CRC(BE897336) SHA1(690e17f2f9a5fbe63686d197552a298efcc8c8c5) )

	// CROM0
	ROM_LOAD64_WORD_SWAP( "mpr-19658.01",  0x0800006,  0x400000, CRC(d523235c) SHA1(0dbfe746b2bdc185768d82c50a329c4c58ad4a29) )
	ROM_LOAD64_WORD_SWAP( "mpr-19659.02",  0x0800004,  0x400000, CRC(c47e7002) SHA1(9644694e6d117564f92650f32f94ce4d7b5523fa) )
	ROM_LOAD64_WORD_SWAP( "mpr-19660.03",  0x0800002,  0x400000, CRC(d999c935) SHA1(ef5429e90314d7a789d8ccbad4d0efaeaff9741a) )
	ROM_LOAD64_WORD_SWAP( "mpr-19661.04",  0x0800000,  0x400000, CRC(8e3fd241) SHA1(df2596f483c759f068c75337320d369d80189ea1) )

	// mirror CROM0 to CROM
	ROM_COPY(REGION_USER1, 0x800000, 0x000000, 0x600000)

	// CROM1
	ROM_LOAD64_WORD_SWAP( "mpr-19662.05",  0x1800006,  0x400000, CRC(3c700eff) SHA1(2ebb149a3d8a9de95afe091b3a1776f4dc3fc579) )
	ROM_LOAD64_WORD_SWAP( "mpr-19663.06",  0x1800004,  0x400000, CRC(f6af1ca4) SHA1(c78237b8f568792202d927ba0af86df6df80f87a) )
	ROM_LOAD64_WORD_SWAP( "mpr-19664.07",  0x1800002,  0x400000, CRC(b9d11294) SHA1(69b6f5708f423fb11337184a3646597356554058) )
	ROM_LOAD64_WORD_SWAP( "mpr-19665.08",  0x1800000,  0x400000, CRC(f97c78f9) SHA1(39aa69e365bf597e5e9185aaf4a044b485ebad8d) )

	// CROM2
	ROM_LOAD64_WORD_SWAP( "mpr-19666.09",  0x2800006,  0x400000, CRC(b53dc97f) SHA1(a4fbc7aade153e6f5fc1dd40ba97d462f643c2c4) )
	ROM_LOAD64_WORD_SWAP( "mpr-19667.10",  0x2800004,  0x400000, CRC(a8676799) SHA1(78734b194e2797ac7efc40f3d0a2ff09dc93409e) )
	ROM_LOAD64_WORD_SWAP( "mpr-19668.11",  0x2800002,  0x400000, CRC(0b4dd8d5) SHA1(b5668ce7ac5a4ac844a0a5a07df9649df9ad9615) )
	ROM_LOAD64_WORD_SWAP( "mpr-19669.12",  0x2800000,  0x400000, CRC(cdc43c61) SHA1(b096d0eb302a9285a8ee396fdbd7b8c546049fd4) )

	ROM_REGION( 0x1000000, REGION_USER3, ROMREGION_DISPOSE )  /* Video ROMs Part 1 */
	ROM_LOAD_VROM( "mpr-19672.26", 0x0000002,  0x200000, CRC(588c29fd) SHA1(5f58c885b506592106aa15208fc1db9d55ab4481) )
	ROM_LOAD_VROM( "mpr-19673.27", 0x0000000,  0x200000, CRC(156abaa9) SHA1(6ef9c042e9ee34090192c1c99c98d19f18efcfba) )
	ROM_LOAD_VROM( "mpr-19674.28", 0x0000006,  0x200000, CRC(c7b0f98c) SHA1(632dbc4cb225d91c82f6a1874517ed0b03b7a0c5) )
	ROM_LOAD_VROM( "mpr-19675.29", 0x0000004,  0x200000, CRC(ff113396) SHA1(af90bb696a3c1585318150cb83ea2ed85cdb67a1) )
	ROM_LOAD_VROM( "mpr-19676.30", 0x000000a,  0x200000, CRC(fd852ead) SHA1(854204c33aec8fb9c014db06e4106be37ecdaf0d) )
	ROM_LOAD_VROM( "mpr-19677.31", 0x0000008,  0x200000, CRC(c6ac0347) SHA1(c792da72af8bf9d011305c9ab7a6230b9e2c5316) )
	ROM_LOAD_VROM( "mpr-19678.32", 0x000000e,  0x200000, CRC(b8819cfe) SHA1(b99f8d0626bc38c75058e94d2461dbec6029589d) )
	ROM_LOAD_VROM( "mpr-19679.33", 0x000000c,  0x200000, CRC(e126c3e3) SHA1(5440540c2432a9ff5bd8e36467af46c456d16844) )

	ROM_REGION( 0x1000000, REGION_USER4, ROMREGION_DISPOSE )  /* Video ROMs Part 2 */
	ROM_LOAD_VROM( "mpr-19680.34", 0x0000002,  0x200000, CRC(00ea5cef) SHA1(3aed46182c0e99c0b72b26c718e2fa20fa7d2e44) )
	ROM_LOAD_VROM( "mpr-19681.35", 0x0000000,  0x200000, CRC(c949325f) SHA1(146de7abf764adc1840b84294cbd473f191cbcb8) )
	ROM_LOAD_VROM( "mpr-19682.36", 0x0000006,  0x200000, CRC(ce5ca065) SHA1(2f518186b29e7cf5fa1c6b036427b8015cfb681e) )
	ROM_LOAD_VROM( "mpr-19683.37", 0x0000004,  0x200000, CRC(e5856419) SHA1(3f8f5b8b36d417090955d34553dcf6d8d9f34558) )
	ROM_LOAD_VROM( "mpr-19684.38", 0x000000a,  0x200000, CRC(56f6ec97) SHA1(dfd251dba77b39342457036fcbe4683d24029600) )
	ROM_LOAD_VROM( "mpr-19685.39", 0x0000008,  0x200000, CRC(42b49304) SHA1(4e185d0f97de44a25b5f982a46f0c3d1dab406c2) )
	ROM_LOAD_VROM( "mpr-19686.40", 0x000000e,  0x200000, CRC(84eed592) SHA1(cc03094770945096d81bc981bff77b540452b045) )
	ROM_LOAD_VROM( "mpr-19687.41", 0x000000c,  0x200000, CRC(776ce694) SHA1(d1e56ebd0011aa3a54a5829c6bd0f5343b283fa0) )

	ROM_REGION( 0x100000, REGION_CPU2, 0 )	/* 68000 code */
	ROM_LOAD16_WORD_SWAP( "epr-19692.21", 0x080000,  0x080000,  CRC(a94f5521) SHA1(22b6a17d44fec8bf796e1790bcabc41f34c89baf) )

	// Samples
	ROM_REGION( 0x800000, REGION_SOUND1, 0 )	/* SCSP samples */
	ROM_LOAD32_WORD( "mpr-19670.22", 0x000000, 0x400000, CRC(bd31cc06) SHA1(d1c85d0cf79b92de5bcbe20dfb8b626ad72de019) )
	ROM_LOAD32_WORD( "mpr-19671.24", 0x000002, 0x400000, CRC(8e8526ab) SHA1(3d2cbb09bd185660feea4dd80bee5af2e2a19aa6) )

	ROM_REGION( 0x20000, REGION_CPU3, 0 )	/* Z80 code */
	ROM_LOAD( "epr-19612.2", 0x000000,  0x20000,  CRC(13978fd4) SHA1(bb597914a34308376239afab6e04fc231e39e379) )

	ROM_REGION( 0x800000, REGION_SOUND2, 0 )	/* DSB samples */
        ROM_LOAD( "mpr-19603.57",  0x000000, 0x200000, CRC(b1b1765f) SHA1(cdcb4d6e6507322f84ac5153b386c3eb5d031e22) )
        ROM_LOAD( "mpr-19604.58",  0x200000, 0x200000, CRC(6ac85b49) SHA1(3e74ae6e9ac7b208e2cd5ebdf80bb3cee19d436d) )
        ROM_LOAD( "mpr-19605.59",  0x400000, 0x200000, CRC(bec891eb) SHA1(357849d2842ac77f9945eb4a0ca89253e474f617) )
        ROM_LOAD( "mpr-19606.60",  0x600000, 0x200000, CRC(adad46b2) SHA1(360b23870f1d15ab527fae1bb731da6e7a8b19c1) )
ROM_END

ROM_START( scuda )	/* step 1.5 */
	ROM_REGION64_BE( 0x4800000, REGION_USER1, 0 ) /* program + data ROMs */
	// CROM
	ROM_LOAD64_WORD_SWAP( "epr-19688.17",  0x0600006,  0x80000,  CRC(a4c85103) SHA1(b2e57f86d0a49e3e88fa7d6a77bbd99039c034bb) )
	ROM_LOAD64_WORD_SWAP( "epr-19689.18",  0x0600004,  0x80000,  CRC(cbce6d62) SHA1(b6051af013ee80406cfadb0c8acf24b8825ccaf2) )
	ROM_LOAD64_WORD_SWAP( "epr-19690.19",  0x0600002,  0x80000,  CRC(25f007fe) SHA1(d3814637f6278c30ea277e30e66ad06e37d37e15) )
	ROM_LOAD64_WORD_SWAP( "epr-19691.20",  0x0600000,  0x80000,  CRC(83523b89) SHA1(c881c71c961948c8ff3fab33e1db23cff4db0ed8) )

	// CROM0
	ROM_LOAD64_WORD_SWAP( "mpr-19658.01",  0x0800006,  0x400000, CRC(d523235c) SHA1(0dbfe746b2bdc185768d82c50a329c4c58ad4a29) )
	ROM_LOAD64_WORD_SWAP( "mpr-19659.02",  0x0800004,  0x400000, CRC(c47e7002) SHA1(9644694e6d117564f92650f32f94ce4d7b5523fa) )
	ROM_LOAD64_WORD_SWAP( "mpr-19660.03",  0x0800002,  0x400000, CRC(d999c935) SHA1(ef5429e90314d7a789d8ccbad4d0efaeaff9741a) )
	ROM_LOAD64_WORD_SWAP( "mpr-19661.04",  0x0800000,  0x400000, CRC(8e3fd241) SHA1(df2596f483c759f068c75337320d369d80189ea1) )

	// mirror CROM0 to CROM
	ROM_COPY(REGION_USER1, 0x800000, 0x000000, 0x600000)

	// CROM1
	ROM_LOAD64_WORD_SWAP( "mpr-19662.05",  0x1800006,  0x400000, CRC(3c700eff) SHA1(2ebb149a3d8a9de95afe091b3a1776f4dc3fc579) )
	ROM_LOAD64_WORD_SWAP( "mpr-19663.06",  0x1800004,  0x400000, CRC(f6af1ca4) SHA1(c78237b8f568792202d927ba0af86df6df80f87a) )
	ROM_LOAD64_WORD_SWAP( "mpr-19664.07",  0x1800002,  0x400000, CRC(b9d11294) SHA1(69b6f5708f423fb11337184a3646597356554058) )
	ROM_LOAD64_WORD_SWAP( "mpr-19665.08",  0x1800000,  0x400000, CRC(f97c78f9) SHA1(39aa69e365bf597e5e9185aaf4a044b485ebad8d) )

	// CROM2
	ROM_LOAD64_WORD_SWAP( "mpr-19666.09",  0x2800006,  0x400000, CRC(b53dc97f) SHA1(a4fbc7aade153e6f5fc1dd40ba97d462f643c2c4) )
	ROM_LOAD64_WORD_SWAP( "mpr-19667.10",  0x2800004,  0x400000, CRC(a8676799) SHA1(78734b194e2797ac7efc40f3d0a2ff09dc93409e) )
	ROM_LOAD64_WORD_SWAP( "mpr-19668.11",  0x2800002,  0x400000, CRC(0b4dd8d5) SHA1(b5668ce7ac5a4ac844a0a5a07df9649df9ad9615) )
	ROM_LOAD64_WORD_SWAP( "mpr-19669.12",  0x2800000,  0x400000, CRC(cdc43c61) SHA1(b096d0eb302a9285a8ee396fdbd7b8c546049fd4) )

	ROM_REGION( 0x1000000, REGION_USER3, ROMREGION_DISPOSE )  /* Video ROMs Part 1 */
	ROM_LOAD_VROM( "mpr-19672.26", 0x02,  0x200000, CRC(588c29fd) SHA1(5f58c885b506592106aa15208fc1db9d55ab4481) )
	ROM_LOAD_VROM( "mpr-19673.27", 0x00,  0x200000, CRC(156abaa9) SHA1(6ef9c042e9ee34090192c1c99c98d19f18efcfba) )
	ROM_LOAD_VROM( "mpr-19674.28", 0x06,  0x200000, CRC(c7b0f98c) SHA1(632dbc4cb225d91c82f6a1874517ed0b03b7a0c5) )
	ROM_LOAD_VROM( "mpr-19675.29", 0x04,  0x200000, CRC(ff113396) SHA1(af90bb696a3c1585318150cb83ea2ed85cdb67a1) )
	ROM_LOAD_VROM( "mpr-19676.30", 0x0a,  0x200000, CRC(fd852ead) SHA1(854204c33aec8fb9c014db06e4106be37ecdaf0d) )
	ROM_LOAD_VROM( "mpr-19677.31", 0x08,  0x200000, CRC(c6ac0347) SHA1(c792da72af8bf9d011305c9ab7a6230b9e2c5316) )
	ROM_LOAD_VROM( "mpr-19678.32", 0x0e,  0x200000, CRC(b8819cfe) SHA1(b99f8d0626bc38c75058e94d2461dbec6029589d) )
	ROM_LOAD_VROM( "mpr-19679.33", 0x0c,  0x200000, CRC(e126c3e3) SHA1(5440540c2432a9ff5bd8e36467af46c456d16844) )

	ROM_REGION( 0x1000000, REGION_USER4, ROMREGION_DISPOSE )  /* Video ROMs Part 2 */
	ROM_LOAD_VROM( "mpr-19680.34", 0x02,  0x200000, CRC(00ea5cef) SHA1(3aed46182c0e99c0b72b26c718e2fa20fa7d2e44) )
	ROM_LOAD_VROM( "mpr-19681.35", 0x00,  0x200000, CRC(c949325f) SHA1(146de7abf764adc1840b84294cbd473f191cbcb8) )
	ROM_LOAD_VROM( "mpr-19682.36", 0x06,  0x200000, CRC(ce5ca065) SHA1(2f518186b29e7cf5fa1c6b036427b8015cfb681e) )
	ROM_LOAD_VROM( "mpr-19683.37", 0x04,  0x200000, CRC(e5856419) SHA1(3f8f5b8b36d417090955d34553dcf6d8d9f34558) )
	ROM_LOAD_VROM( "mpr-19684.38", 0x0a,  0x200000, CRC(56f6ec97) SHA1(dfd251dba77b39342457036fcbe4683d24029600) )
	ROM_LOAD_VROM( "mpr-19685.39", 0x08,  0x200000, CRC(42b49304) SHA1(4e185d0f97de44a25b5f982a46f0c3d1dab406c2) )
	ROM_LOAD_VROM( "mpr-19686.40", 0x0e,  0x200000, CRC(84eed592) SHA1(cc03094770945096d81bc981bff77b540452b045) )
	ROM_LOAD_VROM( "mpr-19687.41", 0x0c,  0x200000, CRC(776ce694) SHA1(d1e56ebd0011aa3a54a5829c6bd0f5343b283fa0) )

	ROM_REGION( 0x100000, REGION_CPU2, 0 )	/* 68000 code */
	ROM_LOAD16_WORD_SWAP( "epr-19692.21", 0x080000,  0x080000,  CRC(a94f5521) SHA1(22b6a17d44fec8bf796e1790bcabc41f34c89baf) )

	// Samples
	ROM_REGION( 0x800000, REGION_SOUND1, 0 )	/* SCSP samples */
	ROM_LOAD32_WORD( "mpr-19670.22", 0x000000, 0x400000, CRC(bd31cc06) SHA1(d1c85d0cf79b92de5bcbe20dfb8b626ad72de019) )
	ROM_LOAD32_WORD( "mpr-19671.24", 0x000002, 0x400000, CRC(8e8526ab) SHA1(3d2cbb09bd185660feea4dd80bee5af2e2a19aa6) )

	ROM_REGION( 0x20000, REGION_CPU3, 0 )	/* Z80 code */
	ROM_LOAD( "epr-19612.2", 0x000000,  0x20000,  CRC(13978fd4) SHA1(bb597914a34308376239afab6e04fc231e39e379) )

	ROM_REGION( 0x800000, REGION_SOUND2, 0 )	/* DSB samples */
        ROM_LOAD( "mpr-19603.57",  0x000000, 0x200000, CRC(b1b1765f) SHA1(cdcb4d6e6507322f84ac5153b386c3eb5d031e22) )
        ROM_LOAD( "mpr-19604.58",  0x200000, 0x200000, CRC(6ac85b49) SHA1(3e74ae6e9ac7b208e2cd5ebdf80bb3cee19d436d) )
        ROM_LOAD( "mpr-19605.59",  0x400000, 0x200000, CRC(bec891eb) SHA1(357849d2842ac77f9945eb4a0ca89253e474f617) )
        ROM_LOAD( "mpr-19606.60",  0x600000, 0x200000, CRC(adad46b2) SHA1(360b23870f1d15ab527fae1bb731da6e7a8b19c1) )
ROM_END

ROM_START( scudp )	/* step 1.5 */
	ROM_REGION64_BE( 0x4800000, REGION_USER1, 0 ) /* program + data ROMs */
	// CROM
	ROM_LOAD64_WORD_SWAP( "epr-20092a.17",  0x0600006,  0x80000, CRC(a94ec57e) SHA1(bda1d9cb38e10a25f7cdde38f30ae13541fdbc5e) )
	ROM_LOAD64_WORD_SWAP( "epr-20093a.18",  0x0600004,  0x80000, CRC(4ed2e35d) SHA1(ac149b369db9fc80e63e1ed943d42ccd056dab1b) )
	ROM_LOAD64_WORD_SWAP( "epr-20094a.19",  0x0600002,  0x80000, CRC(dbf17a43) SHA1(fd719515c8ed78dea80fef20f3af9b72461f81d7) )
	ROM_LOAD64_WORD_SWAP( "epr-20095a.20",  0x0600000,  0x80000, CRC(58c7e393) SHA1(7e25101f72daa271f680a39042341d06249fd104) )

	// CROM0
	ROM_LOAD64_WORD_SWAP( "mpr-19658.01",  0x0800006,  0x400000, CRC(d523235c) SHA1(0dbfe746b2bdc185768d82c50a329c4c58ad4a29) )
	ROM_LOAD64_WORD_SWAP( "mpr-19659.02",  0x0800004,  0x400000, CRC(c47e7002) SHA1(9644694e6d117564f92650f32f94ce4d7b5523fa) )
	ROM_LOAD64_WORD_SWAP( "mpr-19660.03",  0x0800002,  0x400000, CRC(d999c935) SHA1(ef5429e90314d7a789d8ccbad4d0efaeaff9741a) )
	ROM_LOAD64_WORD_SWAP( "mpr-19661.04",  0x0800000,  0x400000, CRC(8e3fd241) SHA1(df2596f483c759f068c75337320d369d80189ea1) )

	// mirror CROM0 to CROM
	ROM_COPY(REGION_USER1, 0x800000, 0x000000, 0x600000)

	// CROM1
	ROM_LOAD64_WORD_SWAP( "mpr-19662.05",  0x1800006,  0x400000, CRC(3c700eff) SHA1(2ebb149a3d8a9de95afe091b3a1776f4dc3fc579) )
	ROM_LOAD64_WORD_SWAP( "mpr-19663.06",  0x1800004,  0x400000, CRC(f6af1ca4) SHA1(c78237b8f568792202d927ba0af86df6df80f87a) )
	ROM_LOAD64_WORD_SWAP( "mpr-19664.07",  0x1800002,  0x400000, CRC(b9d11294) SHA1(69b6f5708f423fb11337184a3646597356554058) )
	ROM_LOAD64_WORD_SWAP( "mpr-19665.08",  0x1800000,  0x400000, CRC(f97c78f9) SHA1(39aa69e365bf597e5e9185aaf4a044b485ebad8d) )

	// CROM2
	ROM_LOAD64_WORD_SWAP( "mpr-19666.09",  0x2800006,  0x400000, CRC(b53dc97f) SHA1(a4fbc7aade153e6f5fc1dd40ba97d462f643c2c4) )
	ROM_LOAD64_WORD_SWAP( "mpr-19667.10",  0x2800004,  0x400000, CRC(a8676799) SHA1(78734b194e2797ac7efc40f3d0a2ff09dc93409e) )
	ROM_LOAD64_WORD_SWAP( "mpr-19668.11",  0x2800002,  0x400000, CRC(0b4dd8d5) SHA1(b5668ce7ac5a4ac844a0a5a07df9649df9ad9615) )
	ROM_LOAD64_WORD_SWAP( "mpr-19669.12",  0x2800000,  0x400000, CRC(cdc43c61) SHA1(b096d0eb302a9285a8ee396fdbd7b8c546049fd4) )

	// CROM3
	ROM_LOAD64_WORD_SWAP( "mpr-20097.13",  0x3800006,  0x400000, CRC(269a9dbe) SHA1(df804ec1df87c1ab6387cd1bbd5dd224adc8d528) )
	ROM_LOAD64_WORD_SWAP( "mpr-20098.14",  0x3800004,  0x400000, CRC(8355fa41) SHA1(e6ac03fd7f1ab882a2861f65f6fea977c106dc15) )
	ROM_LOAD64_WORD_SWAP( "mpr-20099.15",  0x3800002,  0x400000, CRC(fc9bd7d9) SHA1(2dbcbb4b7f8fbeeeeacab51179003d8a4a9e771b) )
	ROM_LOAD64_WORD_SWAP( "mpr-20100.16",  0x3800000,  0x400000, CRC(c99e2c01) SHA1(074d4b5e85f00f5b7550e9944664fdfba8666569) )

	ROM_REGION( 0x1000000, REGION_USER3, ROMREGION_DISPOSE )  /* Video ROMs Part 1 */
	ROM_LOAD_VROM( "mpr-19672.26", 0x0000002,  0x200000, CRC(588c29fd) SHA1(5f58c885b506592106aa15208fc1db9d55ab4481) )
	ROM_LOAD_VROM( "mpr-19673.27", 0x0000000,  0x200000, CRC(156abaa9) SHA1(6ef9c042e9ee34090192c1c99c98d19f18efcfba) )
	ROM_LOAD_VROM( "mpr-19674.28", 0x0000006,  0x200000, CRC(c7b0f98c) SHA1(632dbc4cb225d91c82f6a1874517ed0b03b7a0c5) )
	ROM_LOAD_VROM( "mpr-19675.29", 0x0000004,  0x200000, CRC(ff113396) SHA1(af90bb696a3c1585318150cb83ea2ed85cdb67a1) )
	ROM_LOAD_VROM( "mpr-19676.30", 0x000000a,  0x200000, CRC(fd852ead) SHA1(854204c33aec8fb9c014db06e4106be37ecdaf0d) )
	ROM_LOAD_VROM( "mpr-19677.31", 0x0000008,  0x200000, CRC(c6ac0347) SHA1(c792da72af8bf9d011305c9ab7a6230b9e2c5316) )
	ROM_LOAD_VROM( "mpr-19678.32", 0x000000e,  0x200000, CRC(b8819cfe) SHA1(b99f8d0626bc38c75058e94d2461dbec6029589d) )
	ROM_LOAD_VROM( "mpr-19679.33", 0x000000c,  0x200000, CRC(e126c3e3) SHA1(5440540c2432a9ff5bd8e36467af46c456d16844) )

	ROM_REGION( 0x1000000, REGION_USER4, ROMREGION_DISPOSE )  /* Video ROMs Part 2 */
	ROM_LOAD_VROM( "mpr-19680.34", 0x0000002,  0x200000, CRC(00ea5cef) SHA1(3aed46182c0e99c0b72b26c718e2fa20fa7d2e44) )
	ROM_LOAD_VROM( "mpr-19681.35", 0x0000000,  0x200000, CRC(c949325f) SHA1(146de7abf764adc1840b84294cbd473f191cbcb8) )
	ROM_LOAD_VROM( "mpr-19682.36", 0x0000006,  0x200000, CRC(ce5ca065) SHA1(2f518186b29e7cf5fa1c6b036427b8015cfb681e) )
	ROM_LOAD_VROM( "mpr-19683.37", 0x0000004,  0x200000, CRC(e5856419) SHA1(3f8f5b8b36d417090955d34553dcf6d8d9f34558) )
	ROM_LOAD_VROM( "mpr-19684.38", 0x000000a,  0x200000, CRC(56f6ec97) SHA1(dfd251dba77b39342457036fcbe4683d24029600) )
	ROM_LOAD_VROM( "mpr-19685.39", 0x0000008,  0x200000, CRC(42b49304) SHA1(4e185d0f97de44a25b5f982a46f0c3d1dab406c2) )
	ROM_LOAD_VROM( "mpr-19686.40", 0x000000e,  0x200000, CRC(84eed592) SHA1(cc03094770945096d81bc981bff77b540452b045) )
	ROM_LOAD_VROM( "mpr-19687.41", 0x000000c,  0x200000, CRC(776ce694) SHA1(d1e56ebd0011aa3a54a5829c6bd0f5343b283fa0) )

	ROM_REGION( 0x100000, REGION_CPU2, 0 )	/* 68000 code */
	ROM_LOAD16_WORD_SWAP( "epr-20096a.21", 0x080000,  0x080000,  CRC(0fef288b) SHA1(d6842108d1baea5fffba679d81179c8ffaa87b93) )

	// Samples
	ROM_REGION( 0xc00000, REGION_SOUND1, 0 )	/* SCSP samples */
	ROM_LOAD32_WORD( "mpr-19670.22", 0x000000, 0x400000, CRC(bd31cc06) SHA1(d1c85d0cf79b92de5bcbe20dfb8b626ad72de019) )
	ROM_LOAD32_WORD( "mpr-19671.24", 0x000002, 0x400000, CRC(8e8526ab) SHA1(3d2cbb09bd185660feea4dd80bee5af2e2a19aa6) )
	ROM_LOAD( "mpr-20101.23",        0x800000, 0x400000, CRC(66d1e31f) SHA1(cbc06e9aebcdf82f14bef1c35cbb3203530ef6ae) )

	ROM_REGION( 0x20000, REGION_CPU3, 0 )	/* Z80 code */
	ROM_LOAD( "epr-19612.2", 0x000000,  0x20000,  CRC(13978fd4) SHA1(bb597914a34308376239afab6e04fc231e39e379) )

	ROM_REGION( 0x800000, REGION_SOUND2, 0 )	/* DSB samples */
        ROM_LOAD( "mpr-19603.57",  0x000000, 0x200000, CRC(b1b1765f) SHA1(cdcb4d6e6507322f84ac5153b386c3eb5d031e22) )
        ROM_LOAD( "mpr-19604.58",  0x200000, 0x200000, CRC(6ac85b49) SHA1(3e74ae6e9ac7b208e2cd5ebdf80bb3cee19d436d) )
        ROM_LOAD( "mpr-19605.59",  0x400000, 0x200000, CRC(bec891eb) SHA1(357849d2842ac77f9945eb4a0ca89253e474f617) )
        ROM_LOAD( "mpr-19606.60",  0x600000, 0x200000, CRC(adad46b2) SHA1(360b23870f1d15ab527fae1bb731da6e7a8b19c1) )
ROM_END

ROM_START( vf3 )	/* step 1.0 */
	ROM_REGION64_BE( 0x4800000, REGION_USER1, 0 ) /* program + data ROMs */
	// CROM
        ROM_LOAD64_WORD_SWAP( "epr-19227c.17",  0x600006, 0x080000, CRC(a7df4d75) SHA1(1b1186227f830556c5e2b6ca4c2bf20673b22f94) )
        ROM_LOAD64_WORD_SWAP( "epr-19228c.18",  0x600004, 0x080000, CRC(9c5727e2) SHA1(f9f8b8cf27fdce08ab2975dbaa8c7a03f5c064fb) )
        ROM_LOAD64_WORD_SWAP( "epr-19229c.19",  0x600002, 0x080000, CRC(731b6b78) SHA1(e39f92f721c2771f2d1f5b67625659e006f6fe0a) )
        ROM_LOAD64_WORD_SWAP( "epr-19230c.20",  0x600000, 0x080000, CRC(736a9431) SHA1(0f62a122f349c0b0aab43863a30284a8fe4b7ba9) )

	// CROM0
        ROM_LOAD64_WORD_SWAP( "mpr-19193.1",    0x800006, 0x400000, CRC(7bab33d2) SHA1(243a09959f3c4311070f1de760ee63958cd47660) )
        ROM_LOAD64_WORD_SWAP( "mpr-19194.2",    0x800004, 0x400000, CRC(66254702) SHA1(843ac4f6791f312f3138f8f38d38c8e4d2bab305) )
        ROM_LOAD64_WORD_SWAP( "mpr-19195.3",    0x800002, 0x400000, CRC(bd5e27a3) SHA1(778c67bf7b5c7e3ae52fe12308a81b095563f52b) )
        ROM_LOAD64_WORD_SWAP( "mpr-19196.4",    0x800000, 0x400000, CRC(f386b850) SHA1(168d21382359acb8f1d52d722de8c6b9a9210378) )

	// CROM1
        ROM_LOAD64_WORD_SWAP( "mpr-19197.5",   0x1800006, 0x400000, CRC(a22d76c9) SHA1(ad2d67a62436ccc6479e2a218ab09d2fc22c367d) )
        ROM_LOAD64_WORD_SWAP( "mpr-19198.6",   0x1800004, 0x400000, CRC(d8ee5032) SHA1(3e9274142874ace76dba2bc9b5351cfdfb3a50cd) )
        ROM_LOAD64_WORD_SWAP( "mpr-19199.7",   0x1800002, 0x400000, CRC(9f80d6fe) SHA1(97b9076d413e28d00e9c45fcc7dad6f534ca8874) )
        ROM_LOAD64_WORD_SWAP( "mpr-19200.8",   0x1800000, 0x400000, CRC(74941091) SHA1(914db3955f355779147d86446f5976121191ea6d) )

	// CROM2
        ROM_LOAD64_WORD_SWAP( "mpr-19201.9",   0x2800006, 0x400000, CRC(7c4a8c31) SHA1(473b7bef932d7d54a5dc06bd80d286f2e2e96d44) )
        ROM_LOAD64_WORD_SWAP( "mpr-19202.10",  0x2800004, 0x400000, CRC(aaa086c6) SHA1(01871c8e5454aed80e907fde199cfb23a57aa1c2) )
        ROM_LOAD64_WORD_SWAP( "mpr-19203.11",  0x2800002, 0x400000, CRC(0afa6334) SHA1(1bb70e823fb6e05df069cbfafed2e57bda8776b9) )
        ROM_LOAD64_WORD_SWAP( "mpr-19204.12",  0x2800000, 0x400000, CRC(2f93310a) SHA1(3dfc5b72a78967d7772da4098adb41f18b5294d4) )

	// CROM3
        ROM_LOAD64_WORD_SWAP( "mpr-19205.13",  0x3800006, 0x400000, CRC(199c328e) SHA1(1ef1f09ff1f5253bf03e06c5b6e42be9599b9ea5) )
        ROM_LOAD64_WORD_SWAP( "mpr-19206.14",  0x3800004, 0x400000, CRC(71a98d73) SHA1(dda617f9f5f986e3369fa3d3090c423eefdf913c) )
        ROM_LOAD64_WORD_SWAP( "mpr-19207.15",  0x3800002, 0x400000, CRC(2ce1612d) SHA1(736f559d460f0069c7a2d5ba7cddf9135737d6e2) )
        ROM_LOAD64_WORD_SWAP( "mpr-19208.16",  0x3800000, 0x400000, CRC(08f30f71) SHA1(393525b19cdecddfbd62c6209203db5f3edfd9a8) )

	// mirror CROM0 to CROM
	ROM_COPY(REGION_USER1, 0x800000, 0x000000, 0x600000)

	ROM_REGION( 0x1000000, REGION_USER3, ROMREGION_DISPOSE )  /* Video ROMs Part 1 */
        ROM_LOAD_VROM( "mpr-19211.26",   0x000002, 0x200000, CRC(9c8f5df1) SHA1(d47c8bd0189c8e617a3ed9f75ee3812a229f56c0) )
        ROM_LOAD_VROM( "mpr-19212.27",   0x000000, 0x200000, CRC(75036234) SHA1(01a20a6a62408017bff8f2e76dbd21c00275bc70) )
        ROM_LOAD_VROM( "mpr-19213.28",   0x000006, 0x200000, CRC(67b123cf) SHA1(b84c4f83c25edcc8ac929d3f9cf51da713045071) )
        ROM_LOAD_VROM( "mpr-19214.29",   0x000004, 0x200000, CRC(a6f5576b) SHA1(e994b3ef8e6eb07e8f3bbe474410c06d6c42354b) )
        ROM_LOAD_VROM( "mpr-19215.30",   0x00000a, 0x200000, CRC(c6fd9f0d) SHA1(1f3299706d6ac73836c069a7ed2866d412f60369) )
        ROM_LOAD_VROM( "mpr-19216.31",   0x000008, 0x200000, CRC(201bb1ed) SHA1(7ffd72ff56159529d74f01f8da0ba4798f109806) )
        ROM_LOAD_VROM( "mpr-19217.32",   0x00000e, 0x200000, CRC(4dadd41a) SHA1(7a1e0908962afcfc737132478c0e45d153d94ecb) )
        ROM_LOAD_VROM( "mpr-19218.33",   0x00000c, 0x200000, CRC(cff91953) SHA1(41e95704a65958377c3bbd9d00d90a5ad4552f66) )

	ROM_REGION( 0x1000000, REGION_USER4, ROMREGION_DISPOSE )  /* Video ROMs Part 2 */
        ROM_LOAD_VROM( "mpr-19219.34",   0x000002, 0x200000, CRC(c610d521) SHA1(cb146fe78d89176e9dd5c773644614cdc2ef57ce) )
        ROM_LOAD_VROM( "mpr-19220.35",   0x000000, 0x200000, CRC(e62924d0) SHA1(4d1ac11a5977a4e9cf942c9f1204960c0a895347) )
        ROM_LOAD_VROM( "mpr-19221.36",   0x000006, 0x200000, CRC(24f83e3c) SHA1(c587428fa47e849881bf45487af086db6b09264e) )
        ROM_LOAD_VROM( "mpr-19222.37",   0x000004, 0x200000, CRC(61a6aa7d) SHA1(cc26020b2f904f68822111073b595ee0cc8b2e0c) )
        ROM_LOAD_VROM( "mpr-19223.38",   0x00000a, 0x200000, CRC(1a8c1980) SHA1(43b8efb019c8a20fe38f95050fe60dfe9bf322f0) )
        ROM_LOAD_VROM( "mpr-19224.39",   0x000008, 0x200000, CRC(0a79a1bd) SHA1(1df71cf77ea8611462380a449eb99199664b3da3) )
        ROM_LOAD_VROM( "mpr-19225.40",   0x00000e, 0x200000, CRC(91a985eb) SHA1(5a842a260e4a78f5463222db44f13b068fa70b23) )
        ROM_LOAD_VROM( "mpr-19226.41",   0x00000c, 0x200000, CRC(00091722) SHA1(ef86db36b4b91a66b3e401c3c91735b9d28da2e2) )

	ROM_REGION( 0x80000, REGION_CPU2, 0 )	/* 68000 code */
        ROM_LOAD16_WORD_SWAP( "epr-19231.21",   0x000000, 0x020000, CRC(f2ae2558) SHA1(7ab5fc508ffd1b0c9f3e70f4d3edeae44e3a0dc4) )

	// Samples
	ROM_REGION( 0x800000, REGION_SOUND1, 0 )	/* SCSP samples */
        ROM_LOAD( "mpr-19209.22",   0x000000, 0x400000, CRC(3715e38c) SHA1(b11dbf8a5840990e9697c53b4796cd70ad91f6a1) )
        ROM_LOAD( "mpr-19210.24",   0x400000, 0x400000, CRC(c03d6502) SHA1(4ca49fe5dd5105ca5f78f4740477beb64137d4be) )

	ROM_REGION( 0x20000, REGION_CPU3, 0 )	/* Z80 code */
		ROM_FILL( 0x000000, 0x20000, 0 )

	ROM_REGION( 0x800000, REGION_SOUND2, 0 )	/* DSB samples */
		ROM_FILL( 0x000000, 0x800000, 0 )
ROM_END

ROM_START( vf3tb )	/* step 1.0? */
	ROM_REGION64_BE( 0x4800000, REGION_USER1, 0 ) /* program + data ROMs */
	// CROM
        ROM_LOAD64_WORD_SWAP( "epr-20126.17",  0x600006, 0x080000, CRC(27ecd3b0) SHA1(a9b913294ac925adb501d3b47f346006b70dfcd6) )
        ROM_LOAD64_WORD_SWAP( "epr-20127.18",  0x600004, 0x080000, CRC(5c0f694b) SHA1(ca346d6b249bb7a3015f016d25bfb3050360c8ec) )
        ROM_LOAD64_WORD_SWAP( "epr-20128.19",  0x600002, 0x080000, CRC(ffbdbdc5) SHA1(3cbcc5ce3fcb563f11dc87ac514de2325c6cc9f2) )
        ROM_LOAD64_WORD_SWAP( "epr-20129.20",  0x600000, 0x080000, CRC(0db897ce) SHA1(68f5005082c69fab254d43485669dd6b95a6cc9b) )

	// CROM0
        ROM_LOAD64_WORD_SWAP( "mpr-20130.1",   0x800006, 0x200000, CRC(19e1eaca) SHA1(e63afe7d8f5e653d0efd026fe20da0850f908d5e) )
        ROM_LOAD64_WORD_SWAP( "mpr-20131.2",   0x800004, 0x200000, CRC(d86ec71b) SHA1(da8ffb1a8000b8e656893c29dad8458e04c91df6) )
        ROM_LOAD64_WORD_SWAP( "mpr-20132.3",   0x800002, 0x200000, CRC(91ebc0fb) SHA1(0c37d29c15ec7137dd5398c010a289ffce9ee1e8) )
        ROM_LOAD64_WORD_SWAP( "mpr-20133.4",   0x800000, 0x200000, CRC(d9a10a89) SHA1(a1c1dff2cde54cd128690bb5896448786e5243b9) )

	// CROM1
        ROM_LOAD64_WORD_SWAP( "mpr-19197.5",  0x1800006, 0x400000, CRC(a22d76c9) SHA1(ad2d67a62436ccc6479e2a218ab09d2fc22c367d) )
        ROM_LOAD64_WORD_SWAP( "mpr-19198.6",  0x1800004, 0x400000, CRC(d8ee5032) SHA1(3e9274142874ace76dba2bc9b5351cfdfb3a50cd) )
        ROM_LOAD64_WORD_SWAP( "mpr-19199.7",  0x1800002, 0x400000, CRC(9f80d6fe) SHA1(97b9076d413e28d00e9c45fcc7dad6f534ca8874) )
        ROM_LOAD64_WORD_SWAP( "mpr-19200.8",  0x1800000, 0x400000, CRC(74941091) SHA1(914db3955f355779147d86446f5976121191ea6d) )

	// CROM2
        ROM_LOAD64_WORD_SWAP( "mpr-19201.9",  0x2800006, 0x400000, CRC(7c4a8c31) SHA1(473b7bef932d7d54a5dc06bd80d286f2e2e96d44) )
        ROM_LOAD64_WORD_SWAP( "mpr-19202.10", 0x2800004, 0x400000, CRC(aaa086c6) SHA1(01871c8e5454aed80e907fde199cfb23a57aa1c2) )
        ROM_LOAD64_WORD_SWAP( "mpr-19203.11", 0x2800002, 0x400000, CRC(0afa6334) SHA1(1bb70e823fb6e05df069cbfafed2e57bda8776b9) )
        ROM_LOAD64_WORD_SWAP( "mpr-19204.12", 0x2800000, 0x400000, CRC(2f93310a) SHA1(3dfc5b72a78967d7772da4098adb41f18b5294d4) )

	// CROM3
        ROM_LOAD64_WORD_SWAP( "mpr-19205.13", 0x3800006, 0x400000, CRC(199c328e) SHA1(1ef1f09ff1f5253bf03e06c5b6e42be9599b9ea5) )
        ROM_LOAD64_WORD_SWAP( "mpr-19206.14", 0x3800004, 0x400000, CRC(71a98d73) SHA1(dda617f9f5f986e3369fa3d3090c423eefdf913c) )
        ROM_LOAD64_WORD_SWAP( "mpr-19207.15", 0x3800002, 0x400000, CRC(2ce1612d) SHA1(736f559d460f0069c7a2d5ba7cddf9135737d6e2) )
        ROM_LOAD64_WORD_SWAP( "mpr-19208.16", 0x3800000, 0x400000, CRC(08f30f71) SHA1(393525b19cdecddfbd62c6209203db5f3edfd9a8) )

	// mirror CROM0 to CROM
	ROM_COPY(REGION_USER1, 0x800000, 0x000000, 0x600000)

	ROM_REGION( 0x1000000, REGION_USER3, ROMREGION_DISPOSE )  /* Video ROMs Part 1 */
        ROM_LOAD_VROM( "mpr-19211.26",   0x000002, 0x200000, CRC(9c8f5df1) SHA1(d47c8bd0189c8e617a3ed9f75ee3812a229f56c0) )
        ROM_LOAD_VROM( "mpr-19212.27",   0x000000, 0x200000, CRC(75036234) SHA1(01a20a6a62408017bff8f2e76dbd21c00275bc70) )
        ROM_LOAD_VROM( "mpr-19213.28",   0x000006, 0x200000, CRC(67b123cf) SHA1(b84c4f83c25edcc8ac929d3f9cf51da713045071) )
        ROM_LOAD_VROM( "mpr-19214.29",   0x000004, 0x200000, CRC(a6f5576b) SHA1(e994b3ef8e6eb07e8f3bbe474410c06d6c42354b) )
        ROM_LOAD_VROM( "mpr-19215.30",   0x00000a, 0x200000, CRC(c6fd9f0d) SHA1(1f3299706d6ac73836c069a7ed2866d412f60369) )
        ROM_LOAD_VROM( "mpr-19216.31",   0x000008, 0x200000, CRC(201bb1ed) SHA1(7ffd72ff56159529d74f01f8da0ba4798f109806) )
        ROM_LOAD_VROM( "mpr-19217.32",   0x00000e, 0x200000, CRC(4dadd41a) SHA1(7a1e0908962afcfc737132478c0e45d153d94ecb) )
        ROM_LOAD_VROM( "mpr-19218.33",   0x00000c, 0x200000, CRC(cff91953) SHA1(41e95704a65958377c3bbd9d00d90a5ad4552f66) )

	ROM_REGION( 0x1000000, REGION_USER4, ROMREGION_DISPOSE )  /* Video ROMs Part 2 */
        ROM_LOAD_VROM( "mpr-19219.34",   0x000002, 0x200000, CRC(c610d521) SHA1(cb146fe78d89176e9dd5c773644614cdc2ef57ce) )
        ROM_LOAD_VROM( "mpr-19220.35",   0x000000, 0x200000, CRC(e62924d0) SHA1(4d1ac11a5977a4e9cf942c9f1204960c0a895347) )
        ROM_LOAD_VROM( "mpr-19221.36",   0x000006, 0x200000, CRC(24f83e3c) SHA1(c587428fa47e849881bf45487af086db6b09264e) )
        ROM_LOAD_VROM( "mpr-19222.37",   0x000004, 0x200000, CRC(61a6aa7d) SHA1(cc26020b2f904f68822111073b595ee0cc8b2e0c) )
        ROM_LOAD_VROM( "mpr-19223.38",   0x00000a, 0x200000, CRC(1a8c1980) SHA1(43b8efb019c8a20fe38f95050fe60dfe9bf322f0) )
        ROM_LOAD_VROM( "mpr-19224.39",   0x000008, 0x200000, CRC(0a79a1bd) SHA1(1df71cf77ea8611462380a449eb99199664b3da3) )
        ROM_LOAD_VROM( "mpr-19225.40",   0x00000e, 0x200000, CRC(91a985eb) SHA1(5a842a260e4a78f5463222db44f13b068fa70b23) )
        ROM_LOAD_VROM( "mpr-19226.41",   0x00000c, 0x200000, CRC(00091722) SHA1(ef86db36b4b91a66b3e401c3c91735b9d28da2e2) )

	ROM_REGION( 0x80000, REGION_CPU2, 0 )	/* 68000 code */
        ROM_LOAD16_WORD_SWAP( "epr-19231.21",   0x000000, 0x020000, CRC(f2ae2558) SHA1(7ab5fc508ffd1b0c9f3e70f4d3edeae44e3a0dc4) )

	// Samples
	ROM_REGION( 0x800000, REGION_SOUND1, 0 )	/* SCSP samples */
        ROM_LOAD( "mpr-19209.22",   0x000000, 0x400000, CRC(3715e38c) SHA1(b11dbf8a5840990e9697c53b4796cd70ad91f6a1) )
        ROM_LOAD( "mpr-19210.24",   0x400000, 0x400000, CRC(c03d6502) SHA1(4ca49fe5dd5105ca5f78f4740477beb64137d4be) )

	ROM_REGION( 0x20000, REGION_CPU3, 0 )	/* Z80 code */
		ROM_FILL( 0x000000, 0x20000, 0 )

	ROM_REGION( 0x800000, REGION_SOUND2, 0 )	/* DSB samples */
		ROM_FILL( 0x000000, 0x800000, 0 )
ROM_END

ROM_START( bass )	/* step 1.0 */
	ROM_REGION64_BE( 0x4800000, REGION_USER1, 0 ) /* program + data ROMs */
	// CROM
        ROM_LOAD64_WORD_SWAP( "epr-20643.17",  0x600006, 0x080000, CRC(daf02716) SHA1(b968f8ca602c78b9ca49969ff01f9440f175049a) )
        ROM_LOAD64_WORD_SWAP( "epr-20644.18",  0x600004, 0x080000, CRC(c28db2b6) SHA1(0b12fe9e5189714b1aca79c4bba4be57a9e0d5fd) )
        ROM_LOAD64_WORD_SWAP( "epr-20645.19",  0x600002, 0x080000, CRC(8eefa2b0) SHA1(130ce2a58daa2eba6e56e1f49488017ab871bb7d) )
        ROM_LOAD64_WORD_SWAP( "epr-20646.20",  0x600000, 0x080000, CRC(d740ae06) SHA1(f6c00b9f3f8cf67c8bd8fff73c64bc0c14d364c4) )

	// CROM0
        ROM_LOAD64_WORD_SWAP( "mpr-20256.1",   0x800006, 0x400000, CRC(115302ac) SHA1(45f60aa9f91c9a5821a14647e5ac4d53caf71d5f) )
        ROM_LOAD64_WORD_SWAP( "mpr-20257.2",   0x800004, 0x400000, CRC(025bc06d) SHA1(e774021d8d884871e840100ba6f4c16299233a51) )
        ROM_LOAD64_WORD_SWAP( "mpr-20258.3",   0x800002, 0x400000, CRC(7b78b071) SHA1(f2b29a1238c9eae0a7a68c91a9728ac31f05ef7d) )
        ROM_LOAD64_WORD_SWAP( "mpr-20259.4",   0x800000, 0x400000, CRC(40052562) SHA1(2361fd299b76b1c0d112f1fed85bde16e1564382) )

	// CROM1
        ROM_LOAD64_WORD_SWAP( "mpr-20260.5",  0x1800006, 0x400000, CRC(c56b4c10) SHA1(a0a81d4f05df5b8584c2dca53993c01a35d38812) )
        ROM_LOAD64_WORD_SWAP( "mpr-20261.6",  0x1800004, 0x400000, CRC(b1e9d44a) SHA1(dfe8c5ed848afd48040775bb5a440c590188272c) )
        ROM_LOAD64_WORD_SWAP( "mpr-20262.7",  0x1800002, 0x400000, CRC(52b0674d) SHA1(c9f817b46dc7fcd04dfc2bbc4b1d82b1f41fe258) )
        ROM_LOAD64_WORD_SWAP( "mpr-20263.8",  0x1800000, 0x400000, CRC(1cf4cba9) SHA1(2884bb00990ab4bdad0d524937123c2936523cbb) )

	// CROM2
        ROM_LOAD64_WORD_SWAP( "mpr-20264.9",  0x2800006, 0x400000, CRC(8d995196) SHA1(ff410d80353956ee8e770d7b9e9dabac87ee76cc) )
        ROM_LOAD64_WORD_SWAP( "mpr-20265.10", 0x2800004, 0x400000, CRC(28f76e3e) SHA1(5446e0a0df60d77112bd71c726291fdbba7df284) )
        ROM_LOAD64_WORD_SWAP( "mpr-20266.11", 0x2800002, 0x400000, CRC(abd2db85) SHA1(ebe752071562c532e6ad494f285ff4d0b5050611) )
        ROM_LOAD64_WORD_SWAP( "mpr-20267.12", 0x2800000, 0x400000, CRC(48989191) SHA1(ddbf787ec5dae298ab29847f117eae2ce1ff935e) )

	// mirror CROM0 to CROM
	ROM_COPY(REGION_USER1, 0x800000, 0x000000, 0x600000)

	ROM_REGION( 0x1000000, REGION_USER3, ROMREGION_DISPOSE )  /* Video ROMs Part 1 */
        ROM_LOAD_VROM( "mpr-20270.26",  0x000002, 0x200000, CRC(df68a7a7) SHA1(5d610962dd87c010a094fe2ce8d13408595b4ae4) )
        ROM_LOAD_VROM( "mpr-20271.27",  0x000000, 0x200000, CRC(4b01c3a4) SHA1(8d47109e7f410c9d34d57b22adfe1c3092e70074) )
        ROM_LOAD_VROM( "mpr-20272.28",  0x000006, 0x200000, CRC(a658da23) SHA1(b96270c64cf75625960fa7c03411af595880353f) )
        ROM_LOAD_VROM( "mpr-20273.29",  0x000004, 0x200000, CRC(577e9ffa) SHA1(b004fa10a073e6f4715b417da817051752db5636) )
        ROM_LOAD_VROM( "mpr-20274.30",  0x00000a, 0x200000, CRC(7c7056ae) SHA1(79f6e0ac65f9e80875946b2e73cf9437ecf73407) )
        ROM_LOAD_VROM( "mpr-20275.31",  0x000008, 0x200000, CRC(e739f77a) SHA1(6547c4bc0925af6e07beab54377a174a9c17e9fa) )
        ROM_LOAD_VROM( "mpr-20276.32",  0x00000e, 0x200000, CRC(cbf966c0) SHA1(7c63506d01b52c8ab86fe0dc9ac774e2d540f7c5) )
        ROM_LOAD_VROM( "mpr-20277.33",  0x00000c, 0x200000, CRC(9c75200b) SHA1(b39f571eeab11a619ab964d78a2ba0aa7b1dd24f) )

	ROM_REGION( 0x1000000, REGION_USER4, ROMREGION_DISPOSE )  /* Video ROMs Part 2 */
        ROM_LOAD_VROM( "mpr-20278.34",  0x000002, 0x200000, CRC(db3991ba) SHA1(7854a047741042f087a146882e27f2624f1f9e98) )
        ROM_LOAD_VROM( "mpr-20279.35",  0x000000, 0x200000, CRC(995a11b8) SHA1(eddd32fc3688d12458c5ffb3b3e70459947889a2) )
        ROM_LOAD_VROM( "mpr-20280.36",  0x000006, 0x200000, CRC(c2c8f9f5) SHA1(dd30c1fbece0a3dc8dad2d9d87e58a9f3798f4a2) )
        ROM_LOAD_VROM( "mpr-20281.37",  0x000004, 0x200000, CRC(da84b967) SHA1(dfc13942adc9cf438e70470cb17f4d1f846c4c1a) )
        ROM_LOAD_VROM( "mpr-20282.38",  0x00000a, 0x200000, CRC(1869ff49) SHA1(1368123edfd9c93d1ee591bf40ea110deeac88cf) )
        ROM_LOAD_VROM( "mpr-20283.39",  0x000008, 0x200000, CRC(7d8fb469) SHA1(ad95fec786e9181d91a6ea18808bbf2772e9be6a) )
        ROM_LOAD_VROM( "mpr-20284.40",  0x00000e, 0x200000, CRC(5c7f3a6f) SHA1(d242bc7ad213a79203cd6a060229c356ec0867e7) )
        ROM_LOAD_VROM( "mpr-20285.41",  0x00000c, 0x200000, CRC(4aadc573) SHA1(65aef06c8c48196a0c1f630529ae2248323c5747) )

	ROM_REGION( 0x80000, REGION_CPU2, 0 )	/* 68000 code */
        ROM_LOAD16_WORD_SWAP( "epr-20313.21",  0x000000, 0x080000, CRC(863a7857) SHA1(72384dc6d7613806ab6bb84d935a3b0497e9e9d2) )

	ROM_REGION( 0x800000, REGION_SOUND1, 0 )	/* SCSP samples */
        ROM_LOAD( "mpr-20268.22",  0x000000, 0x400000, CRC(3631e93e) SHA1(3991d6cf03e4f39733d467c483857eac874505d1) )
        ROM_LOAD( "mpr-20269.24",  0x400000, 0x400000, CRC(105a3181) SHA1(022cbce1d01366461a584ff6225ded40bcb9000b) )

	ROM_REGION( 0x20000, REGION_CPU3, 0 )	/* Z80 code */
		ROM_FILL( 0x000000, 0x20000, 0 )

	ROM_REGION( 0x800000, REGION_SOUND2, 0 )	/* DSB samples */
		ROM_FILL( 0x000000, 0x800000, 0 )
ROM_END

ROM_START( getbass )	/* step 1.0 */
	ROM_REGION64_BE( 0x4800000, REGION_USER1, 0 ) /* program + data ROMs */
	// CROM
        ROM_LOAD64_WORD_SWAP( "epr-20309.17",  0x600006, 0x080000, CRC(a42e1033) SHA1(a834eb973e9529338413220a2d8e66ce98d6cb31) )
        ROM_LOAD64_WORD_SWAP( "epr-20310.18",  0x600004, 0x080000, CRC(4efcddc9) SHA1(d1362c2a844b605901083e875a6aad817401eb0a) )
        ROM_LOAD64_WORD_SWAP( "epr-20311.19",  0x600002, 0x080000, CRC(f721050d) SHA1(320a0bd29ac530760d941e54fec2dbc923d6d0f2) )
        ROM_LOAD64_WORD_SWAP( "epr-20312.20",  0x600000, 0x080000, CRC(9d8b8b58) SHA1(6e91aa6a56593e3df8909d149b0c8da189bfbe82) )

	// CROM0
        ROM_LOAD64_WORD_SWAP( "mpr-20256.1",   0x800006, 0x400000, CRC(115302ac) SHA1(45f60aa9f91c9a5821a14647e5ac4d53caf71d5f) )
        ROM_LOAD64_WORD_SWAP( "mpr-20257.2",   0x800004, 0x400000, CRC(025bc06d) SHA1(e774021d8d884871e840100ba6f4c16299233a51) )
        ROM_LOAD64_WORD_SWAP( "mpr-20258.3",   0x800002, 0x400000, CRC(7b78b071) SHA1(f2b29a1238c9eae0a7a68c91a9728ac31f05ef7d) )
        ROM_LOAD64_WORD_SWAP( "mpr-20259.4",   0x800000, 0x400000, CRC(40052562) SHA1(2361fd299b76b1c0d112f1fed85bde16e1564382) )

	// CROM1
        ROM_LOAD64_WORD_SWAP( "mpr-20260.5",  0x1800006, 0x400000, CRC(c56b4c10) SHA1(a0a81d4f05df5b8584c2dca53993c01a35d38812) )
        ROM_LOAD64_WORD_SWAP( "mpr-20261.6",  0x1800004, 0x400000, CRC(b1e9d44a) SHA1(dfe8c5ed848afd48040775bb5a440c590188272c) )
        ROM_LOAD64_WORD_SWAP( "mpr-20262.7",  0x1800002, 0x400000, CRC(52b0674d) SHA1(c9f817b46dc7fcd04dfc2bbc4b1d82b1f41fe258) )
        ROM_LOAD64_WORD_SWAP( "mpr-20263.8",  0x1800000, 0x400000, CRC(1cf4cba9) SHA1(2884bb00990ab4bdad0d524937123c2936523cbb) )

	// CROM2
        ROM_LOAD64_WORD_SWAP( "mpr-20264.9",  0x2800006, 0x400000, CRC(8d995196) SHA1(ff410d80353956ee8e770d7b9e9dabac87ee76cc) )
        ROM_LOAD64_WORD_SWAP( "mpr-20265.10", 0x2800004, 0x400000, CRC(28f76e3e) SHA1(5446e0a0df60d77112bd71c726291fdbba7df284) )
        ROM_LOAD64_WORD_SWAP( "mpr-20266.11", 0x2800002, 0x400000, CRC(abd2db85) SHA1(ebe752071562c532e6ad494f285ff4d0b5050611) )
        ROM_LOAD64_WORD_SWAP( "mpr-20267.12", 0x2800000, 0x400000, CRC(48989191) SHA1(ddbf787ec5dae298ab29847f117eae2ce1ff935e) )

	// mirror CROM0 to CROM
	ROM_COPY(REGION_USER1, 0x800000, 0x000000, 0x600000)

	ROM_REGION( 0x1000000, REGION_USER3, ROMREGION_DISPOSE )  /* Video ROMs Part 1 */
        ROM_LOAD_VROM( "mpr-20270.26",  0x000002, 0x200000, CRC(df68a7a7) SHA1(5d610962dd87c010a094fe2ce8d13408595b4ae4) )
        ROM_LOAD_VROM( "mpr-20271.27",  0x000000, 0x200000, CRC(4b01c3a4) SHA1(8d47109e7f410c9d34d57b22adfe1c3092e70074) )
        ROM_LOAD_VROM( "mpr-20272.28",  0x000006, 0x200000, CRC(a658da23) SHA1(b96270c64cf75625960fa7c03411af595880353f) )
        ROM_LOAD_VROM( "mpr-20273.29",  0x000004, 0x200000, CRC(577e9ffa) SHA1(b004fa10a073e6f4715b417da817051752db5636) )
        ROM_LOAD_VROM( "mpr-20274.30",  0x00000a, 0x200000, CRC(7c7056ae) SHA1(79f6e0ac65f9e80875946b2e73cf9437ecf73407) )
        ROM_LOAD_VROM( "mpr-20275.31",  0x000008, 0x200000, CRC(e739f77a) SHA1(6547c4bc0925af6e07beab54377a174a9c17e9fa) )
        ROM_LOAD_VROM( "mpr-20276.32",  0x00000e, 0x200000, CRC(cbf966c0) SHA1(7c63506d01b52c8ab86fe0dc9ac774e2d540f7c5) )
        ROM_LOAD_VROM( "mpr-20277.33",  0x00000c, 0x200000, CRC(9c75200b) SHA1(b39f571eeab11a619ab964d78a2ba0aa7b1dd24f) )

	ROM_REGION( 0x1000000, REGION_USER4, ROMREGION_DISPOSE )  /* Video ROMs Part 2 */
        ROM_LOAD_VROM( "mpr-20278.34",  0x000002, 0x200000, CRC(db3991ba) SHA1(7854a047741042f087a146882e27f2624f1f9e98) )
        ROM_LOAD_VROM( "mpr-20279.35",  0x000000, 0x200000, CRC(995a11b8) SHA1(eddd32fc3688d12458c5ffb3b3e70459947889a2) )
        ROM_LOAD_VROM( "mpr-20280.36",  0x000006, 0x200000, CRC(c2c8f9f5) SHA1(dd30c1fbece0a3dc8dad2d9d87e58a9f3798f4a2) )
        ROM_LOAD_VROM( "mpr-20281.37",  0x000004, 0x200000, CRC(da84b967) SHA1(dfc13942adc9cf438e70470cb17f4d1f846c4c1a) )
        ROM_LOAD_VROM( "mpr-20282.38",  0x00000a, 0x200000, CRC(1869ff49) SHA1(1368123edfd9c93d1ee591bf40ea110deeac88cf) )
        ROM_LOAD_VROM( "mpr-20283.39",  0x000008, 0x200000, CRC(7d8fb469) SHA1(ad95fec786e9181d91a6ea18808bbf2772e9be6a) )
        ROM_LOAD_VROM( "mpr-20284.40",  0x00000e, 0x200000, CRC(5c7f3a6f) SHA1(d242bc7ad213a79203cd6a060229c356ec0867e7) )
        ROM_LOAD_VROM( "mpr-20285.41",  0x00000c, 0x200000, CRC(4aadc573) SHA1(65aef06c8c48196a0c1f630529ae2248323c5747) )

	ROM_REGION( 0x80000, REGION_CPU2, 0 )	/* 68000 code */
        ROM_LOAD16_WORD_SWAP( "epr-20313.21",  0x000000, 0x080000, CRC(863a7857) SHA1(72384dc6d7613806ab6bb84d935a3b0497e9e9d2) )

	ROM_REGION( 0x800000, REGION_SOUND1, 0 )	/* SCSP samples */
        ROM_LOAD( "mpr-20268.22",  0x000000, 0x400000, CRC(3631e93e) SHA1(3991d6cf03e4f39733d467c483857eac874505d1) )
        ROM_LOAD( "mpr-20269.24",  0x400000, 0x400000, CRC(105a3181) SHA1(022cbce1d01366461a584ff6225ded40bcb9000b) )

	ROM_REGION( 0x20000, REGION_CPU3, 0 )	/* Z80 code */
		ROM_FILL( 0x000000, 0x20000, 0 )

	ROM_REGION( 0x800000, REGION_SOUND2, 0 )	/* DSB samples */
		ROM_FILL( 0x000000, 0x800000, 0 )
ROM_END

ROM_START( lostwsga )	/* Step 1.5 */
	ROM_REGION64_BE( 0x4800000, REGION_USER1, 0 ) /* program + data ROMs */
	// CROM
        ROM_LOAD64_WORD_SWAP( "epr-19939.17",  0x600006, 0x080000, CRC(8788b939) SHA1(30932057f763545568526f85977aa0afc4b66e7d) )
        ROM_LOAD64_WORD_SWAP( "epr-19938.18",  0x600004, 0x080000, CRC(38afe27a) SHA1(718a238ee246eeed9fa698b58493806932d0e7cb) )
        ROM_LOAD64_WORD_SWAP( "epr-19937.19",  0x600002, 0x080000, CRC(9dbf5712) SHA1(ca5923bb5a0b7702391dcacc20e863a7f615929d) )
        ROM_LOAD64_WORD_SWAP( "epr-19936.20",  0x600000, 0x080000, CRC(2f1ca664) SHA1(9138e28ad6c0219b4b3a4609136ed69f484de9a3) )

	// CROM0
        ROM_LOAD64_WORD_SWAP( "mpr-19918.1",   0x800006, 0x400000, CRC(95b690e9) SHA1(50b71aa41372b9acda1db2e0b7ac70707a5cca4b) )
        ROM_LOAD64_WORD_SWAP( "mpr-19919.2",   0x800004, 0x400000, CRC(ff119949) SHA1(2f2648c2eeb8be2838188c5ce65a932c5e6803a6) )
        ROM_LOAD64_WORD_SWAP( "mpr-19920.3",   0x800002, 0x400000, CRC(8df33574) SHA1(671cfee1e5f304a480d8f4a9d44d0315b6839f99) )
        ROM_LOAD64_WORD_SWAP( "mpr-19921.4",   0x800000, 0x400000, CRC(9af3227f) SHA1(f1232ee486b974409f25d94ade1b1f50258cc609) )

	// CROM1
        ROM_LOAD64_WORD_SWAP( "mpr-19922.5",  0x1800006, 0x400000, CRC(4dfd7fc6) SHA1(7fe7445dbf2c1a0b03c1aec40c4e27b391178472) )
        ROM_LOAD64_WORD_SWAP( "mpr-19923.6",  0x1800004, 0x400000, CRC(ed515cb2) SHA1(7604f7bd277a2fc0855effe24b03702878076c13) )
        ROM_LOAD64_WORD_SWAP( "mpr-19924.7",  0x1800002, 0x400000, CRC(4ee3ddc5) SHA1(37b1ca13a7442ae2dedd0ced6c222846c3690984) )
        ROM_LOAD64_WORD_SWAP( "mpr-19925.8",  0x1800000, 0x400000, CRC(cfa4bb49) SHA1(2584fe57f0e117ff64eb34bc4bb782912a379bd3) )

	// CROM2
        ROM_LOAD64_WORD_SWAP( "mpr-19926.9",  0x2800006, 0x400000, CRC(05a232e0) SHA1(712db7664be2efb6c93181194316a7436c40e638) )
        ROM_LOAD64_WORD_SWAP( "mpr-19927.10", 0x2800004, 0x400000, CRC(0c96ef11) SHA1(789878204f3b07def187a8e6c471530021a8504a) )
        ROM_LOAD64_WORD_SWAP( "mpr-19928.11", 0x2800002, 0x400000, CRC(9afd5d4a) SHA1(e81982b7ca21a4a76d3ca5e307a88ce3c8063a6c) )
        ROM_LOAD64_WORD_SWAP( "mpr-19929.12", 0x2800000, 0x400000, CRC(16491f63) SHA1(3ce750db13c6af5696dc14868487334cc7eb9276) )

	// CROM3
        ROM_LOAD64_WORD_SWAP( "mpr-19930.13", 0x3800006, 0x400000, CRC(b598c2f2) SHA1(bd9f3b729bec539a9f9b3e021efa9f4248337456) )
        ROM_LOAD64_WORD_SWAP( "mpr-19931.14", 0x3800004, 0x400000, CRC(448a5007) SHA1(d842d5552867026a1f5d6d22aa5c08e2209f5487) )
        ROM_LOAD64_WORD_SWAP( "mpr-19932.15", 0x3800002, 0x400000, CRC(04389385) SHA1(e645159672c6edbaab5bb3eda3bfbac98bd36210) )
        ROM_LOAD64_WORD_SWAP( "mpr-19933.16", 0x3800000, 0x400000, CRC(8e2acd3b) SHA1(9a87086c06d3d22ade96d6057709008663aa3cfa) )

	// mirror CROM0 to CROM
	ROM_COPY(REGION_USER1, 0x800000, 0x000000, 0x600000)

	ROM_REGION( 0x1000000, REGION_USER3, ROMREGION_DISPOSE )  /* Video ROMs Part 1 */
        ROM_LOAD_VROM( "mpr-19902.26",     0x02, 0x200000, CRC(178bd471) SHA1(dc2cb409081e4fd1176470869e025320449a8d02) )
        ROM_LOAD_VROM( "mpr-19903.27",     0x00, 0x200000, CRC(fe575871) SHA1(db7aec4997b0c9d9a77a611139d53bcfba4bf258) )
        ROM_LOAD_VROM( "mpr-19904.28",     0x06, 0x200000, CRC(57971d7d) SHA1(710705d53e499c5cec6374438d8393a31277f8b7) )
        ROM_LOAD_VROM( "mpr-19905.29",     0x04, 0x200000, CRC(6fa122ee) SHA1(d3e373e7c3f72cee0658820848993c5fd0d4752d) )
        ROM_LOAD_VROM( "mpr-19906.30",     0x0a, 0x200000, CRC(a5b16dd9) SHA1(956a3dbbb101effe92c7a9be3207b9882cf09882) )
        ROM_LOAD_VROM( "mpr-19907.31",     0x08, 0x200000, CRC(84a425cd) SHA1(156541d9cacc0c57ac8d4e60f5ed85a87c7608e7) )
        ROM_LOAD_VROM( "mpr-19908.32",     0x0e, 0x200000, CRC(7702aa7c) SHA1(0ef4f56c95a6779a14b7df7c4e7b83bd219cb67d) )
        ROM_LOAD_VROM( "mpr-19909.33",     0x0c, 0x200000, CRC(8fca65f9) SHA1(17350103ddf0a2efdcde1c1f17d28800334d723f) )

	ROM_REGION( 0x1000000, REGION_USER4, ROMREGION_DISPOSE )  /* Video ROMs Part 2 */
	ROM_LOAD_VROM( "mpr-19910.34",     0x02, 0x200000, CRC(1ef585e2) SHA1(69583635a52aace7986dd8e8139482d685547b7a) )
        ROM_LOAD_VROM( "mpr-19911.35",     0x00, 0x200000, CRC(ca26a48d) SHA1(407434d74cb205211261e08bb633f5fc1863e495) )
        ROM_LOAD_VROM( "mpr-19912.36",     0x06, 0x200000, CRC(ffe000e0) SHA1(d5a2fe8a6ddd5efb934af9f369b24e4508be3143) )
        ROM_LOAD_VROM( "mpr-19913.37",     0x04, 0x200000, CRC(c003049e) SHA1(d3e26531fac33e36c01cdbc0d66f41b918af4c4d) )
        ROM_LOAD_VROM( "mpr-19914.38",     0x0a, 0x200000, CRC(3c21a953) SHA1(1968ba68298e9e73840aa8737dd6c7ad7220cff0) )
        ROM_LOAD_VROM( "mpr-19915.39",     0x08, 0x200000, CRC(fd0f2a2b) SHA1(f47bcbc0a4564682578dde454b4f42ca1a8c6b87) )
        ROM_LOAD_VROM( "mpr-19916.40",     0x0e, 0x200000, CRC(10b0c52e) SHA1(1076352f9a0484815a4f14e66485337a6d5b565e) )
        ROM_LOAD_VROM( "mpr-19917.41",     0x0c, 0x200000, CRC(3035833b) SHA1(e55a225aa1268bcfcc3381d48fc7aaf75f6e1839) )

	ROM_REGION( 0x100000, REGION_CPU2, 0 )	/* 68000 code */
        ROM_LOAD16_WORD_SWAP( "epr-19940.21",     0x080000, 0x080000, CRC(b06ffe5f) SHA1(1b49c2fbc3f188168828daf7f7f56a04c394e832) )

	// Samples
	ROM_REGION( 0x800000, REGION_SOUND1, 0 )	/* SCSP samples */
        ROM_LOAD( "mpr-19934.22",     0x000000, 0x200000, CRC(661b09d0) SHA1(cc4a30aa9fe17964808a7ae967de8f64da36379a) )
        ROM_LOAD( "mpr-19935.24",     0x200000, 0x200000, CRC(6436e483) SHA1(f81b846e7898ec0172b50d74bb7f9062b81f38a8) )
        ROM_LOAD( "mpr-19934.22",     0x400000, 0x200000, CRC(661b09d0) SHA1(cc4a30aa9fe17964808a7ae967de8f64da36379a) )
        ROM_LOAD( "mpr-19935.24",     0x600000, 0x200000, CRC(6436e483) SHA1(f81b846e7898ec0172b50d74bb7f9062b81f38a8) )


	ROM_REGION( 0x20000, REGION_CPU3, 0 )	/* Z80 code */
		ROM_FILL( 0x000000, 0x20000, 0 )

	ROM_REGION( 0x800000, REGION_SOUND2, 0 )	/* DSB samples */
		ROM_FILL( 0x000000, 0x800000, 0 )
ROM_END

ROM_START( vs2 )	/* Step 2.0 */
	ROM_REGION64_BE( 0x4800000, REGION_USER1, 0 ) /* program + data ROMs */
	// CROM
        ROM_LOAD64_WORD_SWAP( "epr-20467.17",  0x400006, 0x100000, CRC(25d7ae73) SHA1(433a7c1dac1bd5524b018da2ed09f937d527ac3e) )
        ROM_LOAD64_WORD_SWAP( "epr-20468.18",  0x400004, 0x100000, CRC(f0f0b6ea) SHA1(b3f545e5a4dd45b97df938093251cc7845c2a1f9) )
        ROM_LOAD64_WORD_SWAP( "epr-20469.19",  0x400002, 0x100000, CRC(9d7521f6) SHA1(9efcb5e6a9add4331c5ea60998afce792b8d6623) )
        ROM_LOAD64_WORD_SWAP( "epr-20470.20",  0x400000, 0x100000, CRC(2f62b292) SHA1(b38db0d7b690e7f9344adf7ee36037196bb521d2) )

	// CROM0
        ROM_LOAD64_WORD_SWAP( "mpr-19769.1",   0x800006, 0x400000, CRC(dc020031) SHA1(35eba49a237c1c647dbf13024e664e2cb09f38b5) )
        ROM_LOAD64_WORD_SWAP( "mpr-19770.2",   0x800004, 0x400000, CRC(91f690b0) SHA1(e70482b255bdc37def897842313c2cb592dd3c6c) )
        ROM_LOAD64_WORD_SWAP( "mpr-19771.3",   0x800002, 0x400000, CRC(189c510f) SHA1(9ebe3c4d98fc57744104608feef7c4e00c0dfd15) )
        ROM_LOAD64_WORD_SWAP( "mpr-19772.4",   0x800000, 0x400000, CRC(6db7b9d0) SHA1(098daad081c8e3ab5dc88e5a8d453b82101c5fc4) )

	// CROM1
        ROM_LOAD64_WORD_SWAP( "mpr-19773.5",  0x1800006, 0x400000, CRC(4e381ae7) SHA1(8ada8de80e019d521d8a3dbdc832745478c84a3d) )
        ROM_LOAD64_WORD_SWAP( "mpr-19774.6",  0x1800004, 0x400000, CRC(1d61d287) SHA1(f0ab5f687570fa3e33a87da9130859f804c8fc01) )
        ROM_LOAD64_WORD_SWAP( "mpr-19775.7",  0x1800002, 0x400000, CRC(a6b32bd9) SHA1(5e2e1e779ff11620a3cf2a8756f2ea08e46d0839) )
        ROM_LOAD64_WORD_SWAP( "mpr-19776.8",  0x1800000, 0x400000, CRC(5b31c7c1) SHA1(7821c5af52a9551be5358aae7df8cfddd58f0fb6) )

	// CROM2
        ROM_LOAD64_WORD_SWAP( "mpr-19777.9",  0x2800006, 0x400000, CRC(c8f216a6) SHA1(0ec700ef15094f6746bbc886e7045329ccebb5d1) )
        ROM_LOAD64_WORD_SWAP( "mpr-19778.10", 0x2800004, 0x400000, CRC(2192b189) SHA1(63f81ab0bd099ef77470cc19fd6d218de72d7876) )
        ROM_LOAD64_WORD_SWAP( "mpr-19779.11", 0x2800002, 0x400000, CRC(2242b21b) SHA1(b5834c19a0a54fe38cac22cfbc2a1e14543aee9d) )
        ROM_LOAD64_WORD_SWAP( "mpr-19780.12", 0x2800000, 0x400000, CRC(38508791) SHA1(0e6d629a6e4d11368a367d74f44bc805805f4365) )

	// CROM3
        ROM_LOAD64_WORD_SWAP( "mpr-19781.13", 0x3800006, 0x400000, CRC(783213f4) SHA1(042c0c0d8604d9d73f581064b0b31234ec7b81b2) )
        ROM_LOAD64_WORD_SWAP( "mpr-19782.14", 0x3800004, 0x400000, CRC(43b43eef) SHA1(8a8b40581a13f56ab2da75f049ba3101a4d3adb4) )
        ROM_LOAD64_WORD_SWAP( "mpr-19783.15", 0x3800002, 0x400000, CRC(47c3d726) SHA1(99ca84b9318c45721ccc5053a909b6ef67a6671c) )
        ROM_LOAD64_WORD_SWAP( "mpr-19784.16", 0x3800000, 0x400000, CRC(a1cc70be) SHA1(c0bb9e78ae9a45c03945b9bc647da129f6171812) )

	// mirror CROM0 to CROM
	ROM_COPY(REGION_USER1, 0x800000, 0x000000, 0x400000)

	ROM_REGION( 0x1000000, REGION_USER3, ROMREGION_DISPOSE )  /* Video ROMs Part 1 */
        ROM_LOAD_VROM( "mpr-19787.26", 0x000002, 0x200000, CRC(856cc4ad) SHA1(a8856ee407c10b021f00f5fd2180dfae4cad2dad) )
        ROM_LOAD_VROM( "mpr-19788.27", 0x000000, 0x200000, CRC(72ef970a) SHA1(ee4af92444e7da61094b5eb5b8469b78aeeb8a32) )
        ROM_LOAD_VROM( "mpr-19789.28", 0x000006, 0x200000, CRC(076add9a) SHA1(117156fd7b802807ee2908dc8a1edb1cd79f1730) )
        ROM_LOAD_VROM( "mpr-19790.29", 0x000004, 0x200000, CRC(74ce238c) SHA1(f6662793d1f2f3c36d7548b912cd40b2ce58753e) )
        ROM_LOAD_VROM( "mpr-19791.30", 0x00000a, 0x200000, CRC(75a98f96) SHA1(9c4cdc5a782cac957fc43952e8f3e35d54f23d1c) )
        ROM_LOAD_VROM( "mpr-19792.31", 0x000008, 0x200000, CRC(85c81633) SHA1(e4311ca92e77502626b312d38a5069ceed9e679f) )
        ROM_LOAD_VROM( "mpr-19793.32", 0x00000e, 0x200000, CRC(7f288cc4) SHA1(71e42ac564c85402b35777e28fff2dce161cbf46) )
        ROM_LOAD_VROM( "mpr-19794.33", 0x00000c, 0x200000, CRC(e0c1c370) SHA1(8efc2940857109bbe74fb7ce1e6fb11d601630f2) )

	ROM_REGION( 0x1000000, REGION_USER4, ROMREGION_DISPOSE )  /* Video ROMs Part 2 */
        ROM_LOAD_VROM( "mpr-19795.34", 0x000002, 0x200000, CRC(90989b20) SHA1(bf96f5770ffaae3b625a906461b8ea755baf9756) )
        ROM_LOAD_VROM( "mpr-19796.35", 0x000000, 0x200000, CRC(5d1aab8d) SHA1(e004c30b9bc8fcad23459e162e9db0c1afd0d5b1) )
        ROM_LOAD_VROM( "mpr-19797.36", 0x000006, 0x200000, CRC(f5edc891) SHA1(e8a23be9a81568892f95e7c66c28ea8d7bd0f508) )
        ROM_LOAD_VROM( "mpr-19798.37", 0x000004, 0x200000, CRC(ae2da90f) SHA1(8243d1f81faa57d5ff5e1f64bcd33cff59219b69) )
        ROM_LOAD_VROM( "mpr-19799.38", 0x00000a, 0x200000, CRC(92b18ad7) SHA1(7be46d7ae2233337d866938ac803589156bfde94) )
        ROM_LOAD_VROM( "mpr-19800.39", 0x000008, 0x200000, CRC(4a57b16c) SHA1(341952460b2f7718e63d5f43a86f507de52bf421) )
        ROM_LOAD_VROM( "mpr-19801.40", 0x00000e, 0x200000, CRC(beb79a00) SHA1(63385ff70bf9ae223e6acfa1b6cb2d641afa2790) )
        ROM_LOAD_VROM( "mpr-19802.41", 0x00000c, 0x200000, CRC(f2c3a7b7) SHA1(d72fafe75baa3542ee27fed05230cd5da99aa459) )

	ROM_REGION( 0x80000, REGION_CPU2, 0 )	/* 68000 code */
        ROM_LOAD16_WORD_SWAP( "epr-19807.21", 0x000000, 0x080000, CRC(9641cbaf) SHA1(aaffde7678b40bc940be04fb107efc4d0d416ea1) )

	// Samples
	ROM_REGION( 0x800000, REGION_SOUND1, 0 )	/* SCSP samples */
        ROM_LOAD( "mpr-19785.22", 0x000000, 0x400000, CRC(e7d190e3) SHA1(f263af149e303429f469a3ab601b87461256aaa7) )
        ROM_LOAD( "mpr-19786.24", 0x400000, 0x400000, CRC(b08d889b) SHA1(790b5b2d62a28c39d43aeec9ffb365ccd9dc93af) )

	ROM_REGION( 0x20000, REGION_CPU3, 0 )	/* Z80 code */
		ROM_FILL( 0x000000, 0x20000, 0 )

	ROM_REGION( 0x800000, REGION_SOUND2, 0 )	/* DSB samples */
		ROM_FILL( 0x000000, 0x800000, 0 )
ROM_END

ROM_START( vs215 )	/* Step 1.5 */
	ROM_REGION64_BE( 0x4800000, REGION_USER1, 0 ) /* program + data ROMs */
	// CROM
        ROM_LOAD64_WORD_SWAP( "epr-19900.17",  0x600006, 0x080000, CRC(8fb6045d) SHA1(88497eafc23ba70ab4a43de552a16caccd8dccbe) )
        ROM_LOAD64_WORD_SWAP( "epr-19899.18",  0x600004, 0x080000, CRC(8cc2be9f) SHA1(ec82b1312c8d58adb200f4d7f6f9a9c8214415d5) )
        ROM_LOAD64_WORD_SWAP( "epr-19898.19",  0x600002, 0x080000, CRC(4389d9ce) SHA1(a5f412417484fdd70dc3dfb2f0cb5554ed4fc7f3) )
        ROM_LOAD64_WORD_SWAP( "epr-19897.20",  0x600000, 0x080000, CRC(25a722a9) SHA1(190b7d002009d91ac4521d180f7519cbeb49da30) )

	// CROM0
        ROM_LOAD64_WORD_SWAP( "mpr-19769.1",   0x800006, 0x400000, CRC(dc020031) SHA1(35eba49a237c1c647dbf13024e664e2cb09f38b5) )
        ROM_LOAD64_WORD_SWAP( "mpr-19770.2",   0x800004, 0x400000, CRC(91f690b0) SHA1(e70482b255bdc37def897842313c2cb592dd3c6c) )
        ROM_LOAD64_WORD_SWAP( "mpr-19771.3",   0x800002, 0x400000, CRC(189c510f) SHA1(9ebe3c4d98fc57744104608feef7c4e00c0dfd15) )
        ROM_LOAD64_WORD_SWAP( "mpr-19772.4",   0x800000, 0x400000, CRC(6db7b9d0) SHA1(098daad081c8e3ab5dc88e5a8d453b82101c5fc4) )

	// CROM1
        ROM_LOAD64_WORD_SWAP( "mpr-19773.5",  0x1800006, 0x400000, CRC(4e381ae7) SHA1(8ada8de80e019d521d8a3dbdc832745478c84a3d) )
        ROM_LOAD64_WORD_SWAP( "mpr-19774.6",  0x1800004, 0x400000, CRC(1d61d287) SHA1(f0ab5f687570fa3e33a87da9130859f804c8fc01) )
        ROM_LOAD64_WORD_SWAP( "mpr-19775.7",  0x1800002, 0x400000, CRC(a6b32bd9) SHA1(5e2e1e779ff11620a3cf2a8756f2ea08e46d0839) )
        ROM_LOAD64_WORD_SWAP( "mpr-19776.8",  0x1800000, 0x400000, CRC(5b31c7c1) SHA1(7821c5af52a9551be5358aae7df8cfddd58f0fb6) )

	// CROM2
        ROM_LOAD64_WORD_SWAP( "mpr-19777.9",  0x2800006, 0x400000, CRC(c8f216a6) SHA1(0ec700ef15094f6746bbc886e7045329ccebb5d1) )
        ROM_LOAD64_WORD_SWAP( "mpr-19778.10", 0x2800004, 0x400000, CRC(2192b189) SHA1(63f81ab0bd099ef77470cc19fd6d218de72d7876) )
        ROM_LOAD64_WORD_SWAP( "mpr-19779.11", 0x2800002, 0x400000, CRC(2242b21b) SHA1(b5834c19a0a54fe38cac22cfbc2a1e14543aee9d) )
        ROM_LOAD64_WORD_SWAP( "mpr-19780.12", 0x2800000, 0x400000, CRC(38508791) SHA1(0e6d629a6e4d11368a367d74f44bc805805f4365) )

	// CROM3
        ROM_LOAD64_WORD_SWAP( "mpr-19781.13", 0x3800006, 0x400000, CRC(783213f4) SHA1(042c0c0d8604d9d73f581064b0b31234ec7b81b2) )
        ROM_LOAD64_WORD_SWAP( "mpr-19782.14", 0x3800004, 0x400000, CRC(43b43eef) SHA1(8a8b40581a13f56ab2da75f049ba3101a4d3adb4) )
        ROM_LOAD64_WORD_SWAP( "mpr-19783.15", 0x3800002, 0x400000, CRC(47c3d726) SHA1(99ca84b9318c45721ccc5053a909b6ef67a6671c) )
        ROM_LOAD64_WORD_SWAP( "mpr-19784.16", 0x3800000, 0x400000, CRC(a1cc70be) SHA1(c0bb9e78ae9a45c03945b9bc647da129f6171812) )

	// mirror CROM0 to CROM
	ROM_COPY(REGION_USER1, 0x800000, 0x000000, 0x600000)

	ROM_REGION( 0x1000000, REGION_USER3, ROMREGION_DISPOSE )  /* Video ROMs Part 1 */
        ROM_LOAD_VROM( "mpr-19787.26", 0x000002, 0x200000, CRC(856cc4ad) SHA1(a8856ee407c10b021f00f5fd2180dfae4cad2dad) )
        ROM_LOAD_VROM( "mpr-19788.27", 0x000000, 0x200000, CRC(72ef970a) SHA1(ee4af92444e7da61094b5eb5b8469b78aeeb8a32) )
        ROM_LOAD_VROM( "mpr-19789.28", 0x000006, 0x200000, CRC(076add9a) SHA1(117156fd7b802807ee2908dc8a1edb1cd79f1730) )
        ROM_LOAD_VROM( "mpr-19790.29", 0x000004, 0x200000, CRC(74ce238c) SHA1(f6662793d1f2f3c36d7548b912cd40b2ce58753e) )
        ROM_LOAD_VROM( "mpr-19791.30", 0x00000a, 0x200000, CRC(75a98f96) SHA1(9c4cdc5a782cac957fc43952e8f3e35d54f23d1c) )
        ROM_LOAD_VROM( "mpr-19792.31", 0x000008, 0x200000, CRC(85c81633) SHA1(e4311ca92e77502626b312d38a5069ceed9e679f) )
        ROM_LOAD_VROM( "mpr-19793.32", 0x00000e, 0x200000, CRC(7f288cc4) SHA1(71e42ac564c85402b35777e28fff2dce161cbf46) )
        ROM_LOAD_VROM( "mpr-19794.33", 0x00000c, 0x200000, CRC(e0c1c370) SHA1(8efc2940857109bbe74fb7ce1e6fb11d601630f2) )

	ROM_REGION( 0x1000000, REGION_USER4, ROMREGION_DISPOSE )  /* Video ROMs Part 2 */
        ROM_LOAD_VROM( "mpr-19795.34", 0x000002, 0x200000, CRC(90989b20) SHA1(bf96f5770ffaae3b625a906461b8ea755baf9756) )
        ROM_LOAD_VROM( "mpr-19796.35", 0x000000, 0x200000, CRC(5d1aab8d) SHA1(e004c30b9bc8fcad23459e162e9db0c1afd0d5b1) )
        ROM_LOAD_VROM( "mpr-19797.36", 0x000006, 0x200000, CRC(f5edc891) SHA1(e8a23be9a81568892f95e7c66c28ea8d7bd0f508) )
        ROM_LOAD_VROM( "mpr-19798.37", 0x000004, 0x200000, CRC(ae2da90f) SHA1(8243d1f81faa57d5ff5e1f64bcd33cff59219b69) )
        ROM_LOAD_VROM( "mpr-19799.38", 0x00000a, 0x200000, CRC(92b18ad7) SHA1(7be46d7ae2233337d866938ac803589156bfde94) )
        ROM_LOAD_VROM( "mpr-19800.39", 0x000008, 0x200000, CRC(4a57b16c) SHA1(341952460b2f7718e63d5f43a86f507de52bf421) )
        ROM_LOAD_VROM( "mpr-19801.40", 0x00000e, 0x200000, CRC(beb79a00) SHA1(63385ff70bf9ae223e6acfa1b6cb2d641afa2790) )
        ROM_LOAD_VROM( "mpr-19802.41", 0x00000c, 0x200000, CRC(f2c3a7b7) SHA1(d72fafe75baa3542ee27fed05230cd5da99aa459) )

	ROM_REGION( 0x80000, REGION_CPU2, 0 )	/* 68000 code */
        ROM_LOAD16_WORD_SWAP( "epr-19807.21", 0x000000, 0x080000, CRC(9641cbaf) SHA1(aaffde7678b40bc940be04fb107efc4d0d416ea1) )

	// Samples
	ROM_REGION( 0x800000, REGION_SOUND1, 0 )	/* SCSP samples */
        ROM_LOAD( "mpr-19785.22", 0x000000, 0x400000, CRC(e7d190e3) SHA1(f263af149e303429f469a3ab601b87461256aaa7) )
        ROM_LOAD( "mpr-19786.24", 0x400000, 0x400000, CRC(b08d889b) SHA1(790b5b2d62a28c39d43aeec9ffb365ccd9dc93af) )

	ROM_REGION( 0x20000, REGION_CPU3, 0 )	/* Z80 code */
		ROM_FILL( 0x000000, 0x20000, 0 )

	ROM_REGION( 0x800000, REGION_SOUND2, 0 )	/* DSB samples */
		ROM_FILL( 0x000000, 0x800000, 0 )
ROM_END

ROM_START( vs298 )	/* Step 2.0 */
	ROM_REGION64_BE( 0x4800000, REGION_USER1, 0 ) /* program + data ROMs */
	// CROM
        ROM_LOAD64_WORD_SWAP( "epr-20917.17",  0x400006, 0x100000, CRC(c3bbb270) SHA1(16b2342031ff72408f2290e775df5c8aa344c2e4) )
        ROM_LOAD64_WORD_SWAP( "epr-20918.18",  0x400004, 0x100000, CRC(0e9cdc5b) SHA1(356816d0380c791b9d812ce17fa95123d15bb5e9) )
        ROM_LOAD64_WORD_SWAP( "epr-20919.19",  0x400002, 0x100000, CRC(7a0713d2) SHA1(595f962ae852e48fb24aa08d0b8603692acfb1b9) )
        ROM_LOAD64_WORD_SWAP( "epr-20920.20",  0x400000, 0x100000, CRC(428d05fc) SHA1(451e78c7b381e7d84dbac2a3d68ebbd6f1490bad) )

	// CROM0
        ROM_LOAD64_WORD_SWAP( "mpr-20891.1",   0x800006, 0x400000, CRC(9ecb0b39) SHA1(97b2ba2a863b7559923efff315aab04f7dca33b0) )
        ROM_LOAD64_WORD_SWAP( "mpr-20892.2",   0x800004, 0x400000, CRC(8e5d3fe7) SHA1(5fe7ad8577ce46fdd2ea741eb2a98028eee61a82) )
        ROM_LOAD64_WORD_SWAP( "mpr-20893.3",   0x800002, 0x400000, CRC(5c83dcaa) SHA1(2d4794bc6c3bfd4913ee045692b6aec5680825e0) )
        ROM_LOAD64_WORD_SWAP( "mpr-20894.4",   0x800000, 0x400000, CRC(09c065cc) SHA1(9a47c6fd45630549066f58b6872ad885908c6e38) )

	// CROM1
        ROM_LOAD64_WORD_SWAP( "mpr-19773.5",  0x1800006, 0x400000, CRC(4e381ae7) SHA1(8ada8de80e019d521d8a3dbdc832745478c84a3d) )
        ROM_LOAD64_WORD_SWAP( "mpr-19774.6",  0x1800004, 0x400000, CRC(1d61d287) SHA1(f0ab5f687570fa3e33a87da9130859f804c8fc01) )
        ROM_LOAD64_WORD_SWAP( "mpr-19775.7",  0x1800002, 0x400000, CRC(a6b32bd9) SHA1(5e2e1e779ff11620a3cf2a8756f2ea08e46d0839) )
        ROM_LOAD64_WORD_SWAP( "mpr-19776.8",  0x1800000, 0x400000, CRC(5b31c7c1) SHA1(7821c5af52a9551be5358aae7df8cfddd58f0fb6) )

	// CROM2
        ROM_LOAD64_WORD_SWAP( "mpr-20895.9",  0x2800006, 0x400000, CRC(9b51cbf5) SHA1(670cfee991b997d4f7c3d51c48dad1ee032f93ef) )
        ROM_LOAD64_WORD_SWAP( "mpr-20896.10", 0x2800004, 0x400000, CRC(bf1cbd5e) SHA1(a677247d7c94f8d36ffece7824026047db1188e1) )
        ROM_LOAD64_WORD_SWAP( "mpr-20897.11", 0x2800002, 0x400000, CRC(c5cf067a) SHA1(ee9503bee3af238434590f439c87219fe45c91b9) )
        ROM_LOAD64_WORD_SWAP( "mpr-20898.12", 0x2800000, 0x400000, CRC(94040d37) SHA1(4a35a0e1cdfdf18a8b01ea11ae88815bf8e92ff3) )

	// CROM3
        ROM_LOAD64_WORD_SWAP( "mpr-20899.13", 0x3800006, 0x400000, CRC(65422425) SHA1(ad4aba5996c2851f14741c5d0f3d7b65e7e765c5) )
        ROM_LOAD64_WORD_SWAP( "mpr-20900.14", 0x3800004, 0x400000, CRC(7a38b571) SHA1(664be2a614e3ce2cc75fdfb9baff55b6e9d77998) )
        ROM_LOAD64_WORD_SWAP( "mpr-20901.15", 0x3800002, 0x400000, CRC(3492ddc8) SHA1(2a36b91ca58b7cc1c8f7a337d2e40b671780ddeb) )
        ROM_LOAD64_WORD_SWAP( "mpr-20902.16", 0x3800000, 0x400000, CRC(f4d3ff3a) SHA1(da3ceba113bca0ea7bd8f67d39bdd7d0cbe5ff7f) )

	// mirror CROM0 to CROM
	ROM_COPY(REGION_USER1, 0x800000, 0x000000, 0x400000)

	ROM_REGION( 0x1000000, REGION_USER3, ROMREGION_DISPOSE )  /* Video ROMs Part 1 */
        ROM_LOAD_VROM( "mpr-19787.26", 0x000002, 0x200000, CRC(856cc4ad) SHA1(a8856ee407c10b021f00f5fd2180dfae4cad2dad) )
        ROM_LOAD_VROM( "mpr-19788.27", 0x000000, 0x200000, CRC(72ef970a) SHA1(ee4af92444e7da61094b5eb5b8469b78aeeb8a32) )
        ROM_LOAD_VROM( "mpr-19789.28", 0x000006, 0x200000, CRC(076add9a) SHA1(117156fd7b802807ee2908dc8a1edb1cd79f1730) )
        ROM_LOAD_VROM( "mpr-19790.29", 0x000004, 0x200000, CRC(74ce238c) SHA1(f6662793d1f2f3c36d7548b912cd40b2ce58753e) )
        ROM_LOAD_VROM( "mpr-19791.30", 0x00000a, 0x200000, CRC(75a98f96) SHA1(9c4cdc5a782cac957fc43952e8f3e35d54f23d1c) )
        ROM_LOAD_VROM( "mpr-19792.31", 0x000008, 0x200000, CRC(85c81633) SHA1(e4311ca92e77502626b312d38a5069ceed9e679f) )
        ROM_LOAD_VROM( "mpr-19793.32", 0x00000e, 0x200000, CRC(7f288cc4) SHA1(71e42ac564c85402b35777e28fff2dce161cbf46) )
        ROM_LOAD_VROM( "mpr-19794.33", 0x00000c, 0x200000, CRC(e0c1c370) SHA1(8efc2940857109bbe74fb7ce1e6fb11d601630f2) )

	ROM_REGION( 0x1000000, REGION_USER4, ROMREGION_DISPOSE )  /* Video ROMs Part 2 */
        ROM_LOAD_VROM( "mpr-19795.34", 0x000002, 0x200000, CRC(90989b20) SHA1(bf96f5770ffaae3b625a906461b8ea755baf9756) )
        ROM_LOAD_VROM( "mpr-19796.35", 0x000000, 0x200000, CRC(5d1aab8d) SHA1(e004c30b9bc8fcad23459e162e9db0c1afd0d5b1) )
        ROM_LOAD_VROM( "mpr-19797.36", 0x000006, 0x200000, CRC(f5edc891) SHA1(e8a23be9a81568892f95e7c66c28ea8d7bd0f508) )
        ROM_LOAD_VROM( "mpr-19798.37", 0x000004, 0x200000, CRC(ae2da90f) SHA1(8243d1f81faa57d5ff5e1f64bcd33cff59219b69) )
        ROM_LOAD_VROM( "mpr-19799.38", 0x00000a, 0x200000, CRC(92b18ad7) SHA1(7be46d7ae2233337d866938ac803589156bfde94) )
        ROM_LOAD_VROM( "mpr-19800.39", 0x000008, 0x200000, CRC(4a57b16c) SHA1(341952460b2f7718e63d5f43a86f507de52bf421) )
        ROM_LOAD_VROM( "mpr-19801.40", 0x00000e, 0x200000, CRC(beb79a00) SHA1(63385ff70bf9ae223e6acfa1b6cb2d641afa2790) )
        ROM_LOAD_VROM( "mpr-19802.41", 0x00000c, 0x200000, CRC(f2c3a7b7) SHA1(d72fafe75baa3542ee27fed05230cd5da99aa459) )

	ROM_REGION( 0x80000, REGION_CPU2, 0 )	/* 68000 code */
        ROM_LOAD16_WORD_SWAP( "epr-20921.21", 0x000000, 0x080000, CRC(30f032a7) SHA1(d29c9631bd50fabe3d86343f44c37ee535db14a0) )

	// Samples
	ROM_REGION( 0x800000, REGION_SOUND1, 0 )	/* SCSP samples */
        ROM_LOAD( "mpr-20903.22", 0x000000, 0x400000, CRC(e343e131) SHA1(cb144516e8c6f1e68bcb774a26cdc494383d3e1b) )
        ROM_LOAD( "mpr-20904.24", 0x400000, 0x400000, CRC(21a91b84) SHA1(cd2d7231b8652ff38376b672c47127ce054d1f32) )

	ROM_REGION( 0x20000, REGION_CPU3, 0 )	/* Z80 code */
		ROM_FILL( 0x000000, 0x20000, 0 )

	ROM_REGION( 0x800000, REGION_SOUND2, 0 )	/* DSB samples */
		ROM_FILL( 0x000000, 0x800000, 0 )
ROM_END

ROM_START( vs29815 )	/* Step 1.5 */
	ROM_REGION64_BE( 0x4800000, REGION_USER1, 0 ) /* program + data ROMs */
	// CROM
        ROM_LOAD64_WORD_SWAP( "epr-20909.17",  0x600006, 0x080000, CRC(3dff0d7e) SHA1(c6a6a103f499cd451796ae2480b8c38c3e87a143) )
        ROM_LOAD64_WORD_SWAP( "epr-20910.18",  0x600004, 0x080000, CRC(dc75a2e3) SHA1(f1b13674ae20b5b964be593171b9d6008d5a51b7) )
        ROM_LOAD64_WORD_SWAP( "epr-20911.19",  0x600002, 0x080000, CRC(acb8fd97) SHA1(2a0ae502283fc8b19ae2bb85b95f66bf80e1bcdf) )
        ROM_LOAD64_WORD_SWAP( "epr-20912.20",  0x600000, 0x080000, CRC(cd2c0538) SHA1(17b66f0cfa0530be3091f974ec959917f2805be1) )

	// CROM0
        ROM_LOAD64_WORD_SWAP( "mpr-20891.1",   0x800006, 0x400000, CRC(9ecb0b39) SHA1(97b2ba2a863b7559923efff315aab04f7dca33b0) )
        ROM_LOAD64_WORD_SWAP( "mpr-20892.2",   0x800004, 0x400000, CRC(8e5d3fe7) SHA1(5fe7ad8577ce46fdd2ea741eb2a98028eee61a82) )
        ROM_LOAD64_WORD_SWAP( "mpr-20893.3",   0x800002, 0x400000, CRC(5c83dcaa) SHA1(2d4794bc6c3bfd4913ee045692b6aec5680825e0) )
        ROM_LOAD64_WORD_SWAP( "mpr-20894.4",   0x800000, 0x400000, CRC(09c065cc) SHA1(9a47c6fd45630549066f58b6872ad885908c6e38) )

	// CROM1
        ROM_LOAD64_WORD_SWAP( "mpr-19773.5",  0x1800006, 0x400000, CRC(4e381ae7) SHA1(8ada8de80e019d521d8a3dbdc832745478c84a3d) )
        ROM_LOAD64_WORD_SWAP( "mpr-19774.6",  0x1800004, 0x400000, CRC(1d61d287) SHA1(f0ab5f687570fa3e33a87da9130859f804c8fc01) )
        ROM_LOAD64_WORD_SWAP( "mpr-19775.7",  0x1800002, 0x400000, CRC(a6b32bd9) SHA1(5e2e1e779ff11620a3cf2a8756f2ea08e46d0839) )
        ROM_LOAD64_WORD_SWAP( "mpr-19776.8",  0x1800000, 0x400000, CRC(5b31c7c1) SHA1(7821c5af52a9551be5358aae7df8cfddd58f0fb6) )

	// CROM2
        ROM_LOAD64_WORD_SWAP( "mpr-20895.9",  0x2800006, 0x400000, CRC(9b51cbf5) SHA1(670cfee991b997d4f7c3d51c48dad1ee032f93ef) )
        ROM_LOAD64_WORD_SWAP( "mpr-20896.10", 0x2800004, 0x400000, CRC(bf1cbd5e) SHA1(a677247d7c94f8d36ffece7824026047db1188e1) )
        ROM_LOAD64_WORD_SWAP( "mpr-20897.11", 0x2800002, 0x400000, CRC(c5cf067a) SHA1(ee9503bee3af238434590f439c87219fe45c91b9) )
        ROM_LOAD64_WORD_SWAP( "mpr-20898.12", 0x2800000, 0x400000, CRC(94040d37) SHA1(4a35a0e1cdfdf18a8b01ea11ae88815bf8e92ff3) )

	// CROM3
        ROM_LOAD64_WORD_SWAP( "mpr-20899.13", 0x3800006, 0x400000, CRC(65422425) SHA1(ad4aba5996c2851f14741c5d0f3d7b65e7e765c5) )
        ROM_LOAD64_WORD_SWAP( "mpr-20900.14", 0x3800004, 0x400000, CRC(7a38b571) SHA1(664be2a614e3ce2cc75fdfb9baff55b6e9d77998) )
        ROM_LOAD64_WORD_SWAP( "mpr-20901.15", 0x3800002, 0x400000, CRC(3492ddc8) SHA1(2a36b91ca58b7cc1c8f7a337d2e40b671780ddeb) )
        ROM_LOAD64_WORD_SWAP( "mpr-20902.16", 0x3800000, 0x400000, CRC(f4d3ff3a) SHA1(da3ceba113bca0ea7bd8f67d39bdd7d0cbe5ff7f) )

	// mirror CROM0 to CROM
	ROM_COPY(REGION_USER1, 0x800000, 0x000000, 0x600000)

	ROM_REGION( 0x1000000, REGION_USER3, ROMREGION_DISPOSE )  /* Video ROMs Part 1 */
        ROM_LOAD_VROM( "mpr-19787.26", 0x000002, 0x200000, CRC(856cc4ad) SHA1(a8856ee407c10b021f00f5fd2180dfae4cad2dad) )
        ROM_LOAD_VROM( "mpr-19788.27", 0x000000, 0x200000, CRC(72ef970a) SHA1(ee4af92444e7da61094b5eb5b8469b78aeeb8a32) )
        ROM_LOAD_VROM( "mpr-19789.28", 0x000006, 0x200000, CRC(076add9a) SHA1(117156fd7b802807ee2908dc8a1edb1cd79f1730) )
        ROM_LOAD_VROM( "mpr-19790.29", 0x000004, 0x200000, CRC(74ce238c) SHA1(f6662793d1f2f3c36d7548b912cd40b2ce58753e) )
        ROM_LOAD_VROM( "mpr-19791.30", 0x00000a, 0x200000, CRC(75a98f96) SHA1(9c4cdc5a782cac957fc43952e8f3e35d54f23d1c) )
        ROM_LOAD_VROM( "mpr-19792.31", 0x000008, 0x200000, CRC(85c81633) SHA1(e4311ca92e77502626b312d38a5069ceed9e679f) )
        ROM_LOAD_VROM( "mpr-19793.32", 0x00000e, 0x200000, CRC(7f288cc4) SHA1(71e42ac564c85402b35777e28fff2dce161cbf46) )
        ROM_LOAD_VROM( "mpr-19794.33", 0x00000c, 0x200000, CRC(e0c1c370) SHA1(8efc2940857109bbe74fb7ce1e6fb11d601630f2) )

	ROM_REGION( 0x1000000, REGION_USER4, ROMREGION_DISPOSE )  /* Video ROMs Part 2 */
        ROM_LOAD_VROM( "mpr-19795.34", 0x000002, 0x200000, CRC(90989b20) SHA1(bf96f5770ffaae3b625a906461b8ea755baf9756) )
        ROM_LOAD_VROM( "mpr-19796.35", 0x000000, 0x200000, CRC(5d1aab8d) SHA1(e004c30b9bc8fcad23459e162e9db0c1afd0d5b1) )
        ROM_LOAD_VROM( "mpr-19797.36", 0x000006, 0x200000, CRC(f5edc891) SHA1(e8a23be9a81568892f95e7c66c28ea8d7bd0f508) )
        ROM_LOAD_VROM( "mpr-19798.37", 0x000004, 0x200000, CRC(ae2da90f) SHA1(8243d1f81faa57d5ff5e1f64bcd33cff59219b69) )
        ROM_LOAD_VROM( "mpr-19799.38", 0x00000a, 0x200000, CRC(92b18ad7) SHA1(7be46d7ae2233337d866938ac803589156bfde94) )
        ROM_LOAD_VROM( "mpr-19800.39", 0x000008, 0x200000, CRC(4a57b16c) SHA1(341952460b2f7718e63d5f43a86f507de52bf421) )
        ROM_LOAD_VROM( "mpr-19801.40", 0x00000e, 0x200000, CRC(beb79a00) SHA1(63385ff70bf9ae223e6acfa1b6cb2d641afa2790) )
        ROM_LOAD_VROM( "mpr-19802.41", 0x00000c, 0x200000, CRC(f2c3a7b7) SHA1(d72fafe75baa3542ee27fed05230cd5da99aa459) )

	ROM_REGION( 0x80000, REGION_CPU2, 0 )	/* 68000 code */
        ROM_LOAD16_WORD_SWAP( "epr-20921.21", 0x000000, 0x080000, CRC(30f032a7) SHA1(d29c9631bd50fabe3d86343f44c37ee535db14a0) )

	// Samples
	ROM_REGION( 0x800000, REGION_SOUND1, 0 )	/* SCSP samples */
        ROM_LOAD( "mpr-20903.22", 0x000000, 0x400000, CRC(e343e131) SHA1(cb144516e8c6f1e68bcb774a26cdc494383d3e1b) )
        ROM_LOAD( "mpr-20904.24", 0x400000, 0x400000, CRC(21a91b84) SHA1(cd2d7231b8652ff38376b672c47127ce054d1f32) )

	ROM_REGION( 0x20000, REGION_CPU3, 0 )	/* Z80 code */
		ROM_FILL( 0x000000, 0x20000, 0 )

	ROM_REGION( 0x800000, REGION_SOUND2, 0 )	/* DSB samples */
		ROM_FILL( 0x000000, 0x800000, 0 )
ROM_END

ROM_START( vs2v991 )	/* Step 2.0 */
	ROM_REGION64_BE( 0x4800000, REGION_USER1, 0 ) /* program + data ROMs */
	// CROM
        ROM_LOAD64_WORD_SWAP( "epr-21535b.17", 0x400006, 0x100000, CRC(76c5fa8e) SHA1(862438198cb7fdd20beeba53e707a7c59e618ad9) )
        ROM_LOAD64_WORD_SWAP( "epr-21536b.18", 0x400004, 0x100000, CRC(1f2bd190) SHA1(19843e6c5626de03eba3cba79c03ce9f2471c183) )
        ROM_LOAD64_WORD_SWAP( "epr-21537b.19", 0x400002, 0x100000, CRC(a8b3fa5c) SHA1(884042590da9eef0fc2557f715c5d6811edb4ce1) )
        ROM_LOAD64_WORD_SWAP( "epr-21538b.20", 0x400000, 0x100000, CRC(b3f0ce2a) SHA1(940b76afe3e66bcd026e74f58a08b13c3925f449) )

	// CROM0
        ROM_LOAD64_WORD_SWAP( "mpr-21497.1",   0x800006, 0x400000, CRC(8ea759a1) SHA1(0d444fa360d93f48e5d6607362a231f97a7685d4) )
        ROM_LOAD64_WORD_SWAP( "mpr-21498.2",   0x800004, 0x400000, CRC(4f53d6e0) SHA1(c8cd14f46d4ac7afdf55035a20d2e9a5ce2b6cde) )
        ROM_LOAD64_WORD_SWAP( "mpr-21499.3",   0x800002, 0x400000, CRC(2cc4c1f1) SHA1(fd0fd747368e798095119a21d82f14778aeaa45e) )
        ROM_LOAD64_WORD_SWAP( "mpr-21500.4",   0x800000, 0x400000, CRC(8c43964b) SHA1(cf3a6e9402f9ba532fca73f6838478558fb9a3ba) )

	// CROM1
        ROM_LOAD64_WORD_SWAP( "mpr-21501.5",  0x1800006, 0x400000, CRC(08bc2185) SHA1(6c4c977f68a73d605bdacdc0d76ca89bc7030c04) )
        ROM_LOAD64_WORD_SWAP( "mpr-21502.6",  0x1800004, 0x400000, CRC(921486be) SHA1(bb1261272992cf86e83e0c788788765f05b43bbf) )
        ROM_LOAD64_WORD_SWAP( "mpr-21503.7",  0x1800002, 0x400000, CRC(c9e1de6b) SHA1(d200c3da2c9bc6d4ed60dfa60a77056d25b19037) )
        ROM_LOAD64_WORD_SWAP( "mpr-21504.8",  0x1800000, 0x400000, CRC(7aae557e) SHA1(2128d7dfa52e639858d37eb6100875b9ce3d056f) )

	// CROM2
        ROM_LOAD64_WORD_SWAP( "mpr-21505.9",  0x2800006, 0x400000, CRC(e169ff72) SHA1(9d407b424403261a224ea15b9476eba16406c4a4) )
        ROM_LOAD64_WORD_SWAP( "mpr-21506.10", 0x2800004, 0x400000, CRC(2c1477c7) SHA1(81ab7d9cef5127e1f0e16f9a94a9ea2acc4530a4) )
        ROM_LOAD64_WORD_SWAP( "mpr-21507.11", 0x2800002, 0x400000, CRC(1d8eb68b) SHA1(634693f066059c738526913498bb18be2f7cd086) )
        ROM_LOAD64_WORD_SWAP( "mpr-21508.12", 0x2800000, 0x400000, CRC(2e8f798e) SHA1(8298df90101dd5850db8fccb7661ca2bc6806b3f) )

	// CROM3
        ROM_LOAD64_WORD_SWAP( "mpr-21509.13", 0x3800006, 0x400000, CRC(9a65e6b4) SHA1(e96c4bc2782b73490dffd5dcb11b9020077b11a3) )
        ROM_LOAD64_WORD_SWAP( "mpr-21510.14", 0x3800004, 0x400000, CRC(f47489a4) SHA1(8412505002628d7ae3ab766a13e2068a018f3bf3) )
        ROM_LOAD64_WORD_SWAP( "mpr-21511.15", 0x3800002, 0x400000, CRC(5ad9660c) SHA1(da387449292322a89af1cb6746d0fb8cea17575f) )
        ROM_LOAD64_WORD_SWAP( "mpr-21512.16", 0x3800000, 0x400000, CRC(7cb2b05c) SHA1(16edc6642c74d9cef883559ca6ec562d985a43d6) )

	// mirror CROM0 to CROM
	ROM_COPY(REGION_USER1, 0x800000, 0x000000, 0x400000)

	ROM_REGION( 0x1000000, REGION_USER3, ROMREGION_DISPOSE )  /* Video ROMs Part 1 */
        ROM_LOAD_VROM( "mpr-21515.26",     0x000002, 0x200000, CRC(8ce9910b) SHA1(7a0d0696e4456d9ebf131041917c5214b7d2e3ec) )
        ROM_LOAD_VROM( "mpr-21516.27",     0x000000, 0x200000, CRC(8971a753) SHA1(00dfdb83a65f4fde337618c346157bb89f398531) )
        ROM_LOAD_VROM( "mpr-21517.28",     0x000006, 0x200000, CRC(55a4533b) SHA1(b5701bbf7780bb9fc386cef4c1835606ab792f91) )
        ROM_LOAD_VROM( "mpr-21518.29",     0x000004, 0x200000, CRC(4134026c) SHA1(2dfe1cbb354affe465c31a18c3ffb83a9bf555c9) )
        ROM_LOAD_VROM( "mpr-21519.30",     0x00000a, 0x200000, CRC(ef6757de) SHA1(d41bbfcc551a4589bac577e311c67f2cba0a49aa) )
        ROM_LOAD_VROM( "mpr-21520.31",     0x000008, 0x200000, CRC(c53be8cc) SHA1(b12dc0327a00b7e056254d2f11f96dbf396a0c91) )
        ROM_LOAD_VROM( "mpr-21521.32",     0x00000e, 0x200000, CRC(abb501dc) SHA1(88cb40b0f795e0de1ff56e1f31bf834fad0c7885) )
        ROM_LOAD_VROM( "mpr-21522.33",     0x00000c, 0x200000, CRC(e3b79973) SHA1(4b6ca16a23bb3e195ca60bee81b2d069f371ff70) )

	ROM_REGION( 0x1000000, REGION_USER4, ROMREGION_DISPOSE )  /* Video ROMs Part 2 */
        ROM_LOAD_VROM( "mpr-21523.34",     0x000002, 0x200000, CRC(fe4d1eac) SHA1(d222743d25ca92904ec212c66d03b3e3ff0ddbd9) )
        ROM_LOAD_VROM( "mpr-21524.35",     0x000000, 0x200000, CRC(8633b6e9) SHA1(65ec24eb29613831dd28e5338cac14696b0d975d) )
        ROM_LOAD_VROM( "mpr-21525.36",     0x000006, 0x200000, CRC(3c490167) SHA1(6fd46049723e0790b2231301cfa23071cd6ff1f6) )
        ROM_LOAD_VROM( "mpr-21526.37",     0x000004, 0x200000, CRC(5fe5f9b0) SHA1(c708918cfc60f5fd9f6ec49ec1cd3167f2876e30) )
        ROM_LOAD_VROM( "mpr-21527.38",     0x00000a, 0x200000, CRC(10d0fe7e) SHA1(63693b0de43e2eb6efbb3d2dfbe0e2f5bc6810dc) )
        ROM_LOAD_VROM( "mpr-21528.39",     0x000008, 0x200000, CRC(4e346a6c) SHA1(ae34038d5bf6f63ec5ad2e8dd8e06db66147c40e) )
        ROM_LOAD_VROM( "mpr-21529.40",     0x00000e, 0x200000, CRC(9a731a00) SHA1(eca98b142acc02fb28387675e1cb1bc7e4e59b86) )
        ROM_LOAD_VROM( "mpr-21530.41",     0x00000c, 0x200000, CRC(78400d5e) SHA1(9b4546848dbe213f33b02e8ea42743e60a0f763f) )

	ROM_REGION( 0x80000, REGION_CPU2, 0 )	/* 68000 code */
        ROM_LOAD16_WORD_SWAP( "epr-21539.21",    0x000000, 0x080000, CRC(a1d3e00e) SHA1(e03bb31967929a12de9ae21923914e0e3bd96aaa) )

	ROM_REGION( 0x800000, REGION_SOUND1, 0 )	/* SCSP samples */
        ROM_LOAD( "mpr-21513.22", 0x000000, 0x400000, CRC(cca1cc00) SHA1(ba1fa3b8ef3bff7e116901a0a4bd80d2ae4018bf) )
        ROM_LOAD( "mpr-21514.24", 0x400000, 0x400000, CRC(6cedd292) SHA1(c1f44715697a8bac9d39926bcd6558ec9a9b2319) )

	ROM_REGION( 0x20000, REGION_CPU3, 0 )	/* Z80 code */
		ROM_FILL( 0x000000, 0x20000, 0 )

	ROM_REGION( 0x800000, REGION_SOUND2, 0 )	/* DSB samples */
		ROM_FILL( 0x000000, 0x800000, 0 )
ROM_END

ROM_START( vs299 )	/* Step 2.0 */
	ROM_REGION64_BE( 0x4800000, REGION_USER1, 0 ) /* program + data ROMs */
	// CROM
        ROM_LOAD64_WORD_SWAP( "epr-21535.17",  0x400006, 0x100000, CRC(976a00bf) SHA1(d4be52ff59faa877b169f96ac509a2196cefb908) )
        ROM_LOAD64_WORD_SWAP( "epr-21536.18",  0x400004, 0x100000, CRC(9af2b0d5) SHA1(6ec296014228782f372611fe774014d252956b63) )
        ROM_LOAD64_WORD_SWAP( "epr-21537.19",  0x400002, 0x100000, CRC(fb37dc16) SHA1(205e7d6dae21ba2cd4c8e37c2acab680c3f5a9b4) )
        ROM_LOAD64_WORD_SWAP( "epr-21538.20",  0x400000, 0x100000, CRC(02df6ac8) SHA1(2259b528264dd30fa38ea06934e2d38b44b32981) )

	// CROM0
        ROM_LOAD64_WORD_SWAP( "mpr-21497.1",   0x800006, 0x400000, CRC(8ea759a1) SHA1(0d444fa360d93f48e5d6607362a231f97a7685d4) )
        ROM_LOAD64_WORD_SWAP( "mpr-21498.2",   0x800004, 0x400000, CRC(4f53d6e0) SHA1(c8cd14f46d4ac7afdf55035a20d2e9a5ce2b6cde) )
        ROM_LOAD64_WORD_SWAP( "mpr-21499.3",   0x800002, 0x400000, CRC(2cc4c1f1) SHA1(fd0fd747368e798095119a21d82f14778aeaa45e) )
        ROM_LOAD64_WORD_SWAP( "mpr-21500.4",   0x800000, 0x400000, CRC(8c43964b) SHA1(cf3a6e9402f9ba532fca73f6838478558fb9a3ba) )

	// CROM1
        ROM_LOAD64_WORD_SWAP( "mpr-21501.5",  0x1800006, 0x400000, CRC(08bc2185) SHA1(6c4c977f68a73d605bdacdc0d76ca89bc7030c04) )
        ROM_LOAD64_WORD_SWAP( "mpr-21502.6",  0x1800004, 0x400000, CRC(921486be) SHA1(bb1261272992cf86e83e0c788788765f05b43bbf) )
        ROM_LOAD64_WORD_SWAP( "mpr-21503.7",  0x1800002, 0x400000, CRC(c9e1de6b) SHA1(d200c3da2c9bc6d4ed60dfa60a77056d25b19037) )
        ROM_LOAD64_WORD_SWAP( "mpr-21504.8",  0x1800000, 0x400000, CRC(7aae557e) SHA1(2128d7dfa52e639858d37eb6100875b9ce3d056f) )

	// CROM2
        ROM_LOAD64_WORD_SWAP( "mpr-21505.9",  0x2800006, 0x400000, CRC(e169ff72) SHA1(9d407b424403261a224ea15b9476eba16406c4a4) )
        ROM_LOAD64_WORD_SWAP( "mpr-21506.10", 0x2800004, 0x400000, CRC(2c1477c7) SHA1(81ab7d9cef5127e1f0e16f9a94a9ea2acc4530a4) )
        ROM_LOAD64_WORD_SWAP( "mpr-21507.11", 0x2800002, 0x400000, CRC(1d8eb68b) SHA1(634693f066059c738526913498bb18be2f7cd086) )
        ROM_LOAD64_WORD_SWAP( "mpr-21508.12", 0x2800000, 0x400000, CRC(2e8f798e) SHA1(8298df90101dd5850db8fccb7661ca2bc6806b3f) )

	// CROM3
        ROM_LOAD64_WORD_SWAP( "mpr-21509.13", 0x3800006, 0x400000, CRC(9a65e6b4) SHA1(e96c4bc2782b73490dffd5dcb11b9020077b11a3) )
        ROM_LOAD64_WORD_SWAP( "mpr-21510.14", 0x3800004, 0x400000, CRC(f47489a4) SHA1(8412505002628d7ae3ab766a13e2068a018f3bf3) )
        ROM_LOAD64_WORD_SWAP( "mpr-21511.15", 0x3800002, 0x400000, CRC(5ad9660c) SHA1(da387449292322a89af1cb6746d0fb8cea17575f) )
        ROM_LOAD64_WORD_SWAP( "mpr-21512.16", 0x3800000, 0x400000, CRC(7cb2b05c) SHA1(16edc6642c74d9cef883559ca6ec562d985a43d6) )

	// mirror CROM0 to CROM
	ROM_COPY(REGION_USER1, 0x800000, 0x000000, 0x400000)

	ROM_REGION( 0x1000000, REGION_USER3, ROMREGION_DISPOSE )  /* Video ROMs Part 1 */
        ROM_LOAD_VROM( "mpr-21515.26",     0x000002, 0x200000, CRC(8ce9910b) SHA1(7a0d0696e4456d9ebf131041917c5214b7d2e3ec) )
        ROM_LOAD_VROM( "mpr-21516.27",     0x000000, 0x200000, CRC(8971a753) SHA1(00dfdb83a65f4fde337618c346157bb89f398531) )
        ROM_LOAD_VROM( "mpr-21517.28",     0x000006, 0x200000, CRC(55a4533b) SHA1(b5701bbf7780bb9fc386cef4c1835606ab792f91) )
        ROM_LOAD_VROM( "mpr-21518.29",     0x000004, 0x200000, CRC(4134026c) SHA1(2dfe1cbb354affe465c31a18c3ffb83a9bf555c9) )
        ROM_LOAD_VROM( "mpr-21519.30",     0x00000a, 0x200000, CRC(ef6757de) SHA1(d41bbfcc551a4589bac577e311c67f2cba0a49aa) )
        ROM_LOAD_VROM( "mpr-21520.31",     0x000008, 0x200000, CRC(c53be8cc) SHA1(b12dc0327a00b7e056254d2f11f96dbf396a0c91) )
        ROM_LOAD_VROM( "mpr-21521.32",     0x00000e, 0x200000, CRC(abb501dc) SHA1(88cb40b0f795e0de1ff56e1f31bf834fad0c7885) )
        ROM_LOAD_VROM( "mpr-21522.33",     0x00000c, 0x200000, CRC(e3b79973) SHA1(4b6ca16a23bb3e195ca60bee81b2d069f371ff70) )

	ROM_REGION( 0x1000000, REGION_USER4, ROMREGION_DISPOSE )  /* Video ROMs Part 2 */
        ROM_LOAD_VROM( "mpr-21523.34",     0x000002, 0x200000, CRC(fe4d1eac) SHA1(d222743d25ca92904ec212c66d03b3e3ff0ddbd9) )
        ROM_LOAD_VROM( "mpr-21524.35",     0x000000, 0x200000, CRC(8633b6e9) SHA1(65ec24eb29613831dd28e5338cac14696b0d975d) )
        ROM_LOAD_VROM( "mpr-21525.36",     0x000006, 0x200000, CRC(3c490167) SHA1(6fd46049723e0790b2231301cfa23071cd6ff1f6) )
        ROM_LOAD_VROM( "mpr-21526.37",     0x000004, 0x200000, CRC(5fe5f9b0) SHA1(c708918cfc60f5fd9f6ec49ec1cd3167f2876e30) )
        ROM_LOAD_VROM( "mpr-21527.38",     0x00000a, 0x200000, CRC(10d0fe7e) SHA1(63693b0de43e2eb6efbb3d2dfbe0e2f5bc6810dc) )
        ROM_LOAD_VROM( "mpr-21528.39",     0x000008, 0x200000, CRC(4e346a6c) SHA1(ae34038d5bf6f63ec5ad2e8dd8e06db66147c40e) )
        ROM_LOAD_VROM( "mpr-21529.40",     0x00000e, 0x200000, CRC(9a731a00) SHA1(eca98b142acc02fb28387675e1cb1bc7e4e59b86) )
        ROM_LOAD_VROM( "mpr-21530.41",     0x00000c, 0x200000, CRC(78400d5e) SHA1(9b4546848dbe213f33b02e8ea42743e60a0f763f) )

	ROM_REGION( 0x80000, REGION_CPU2, 0 )	/* 68000 code */
        ROM_LOAD16_WORD_SWAP( "epr-21539.21", 0x000000, 0x080000, CRC(a1d3e00e) SHA1(e03bb31967929a12de9ae21923914e0e3bd96aaa) )

	ROM_REGION( 0x800000, REGION_SOUND1, 0 )	/* SCSP samples */
        ROM_LOAD( "mpr-21513.22", 0x000000, 0x400000, CRC(cca1cc00) SHA1(ba1fa3b8ef3bff7e116901a0a4bd80d2ae4018bf) )
        ROM_LOAD( "mpr-21514.24", 0x400000, 0x400000, CRC(6cedd292) SHA1(c1f44715697a8bac9d39926bcd6558ec9a9b2319) )

	ROM_REGION( 0x20000, REGION_CPU3, 0 )	/* Z80 code */
		ROM_FILL( 0x000000, 0x20000, 0 )

	ROM_REGION( 0x800000, REGION_SOUND2, 0 )	/* DSB samples */
		ROM_FILL( 0x000000, 0x800000, 0 )
ROM_END

ROM_START( von2 )	/* Step 2.0 */
	ROM_REGION64_BE( 0x4800000, REGION_USER1, 0 ) /* program + data ROMs */
	// CROM
        ROM_LOAD64_WORD_SWAP( "epr-20683b.17", 0x000006, 0x200000, CRC(59d9c974) SHA1(c45594ed474a9e8fd074e0d9d5fa6662bc88dee6) )
        ROM_LOAD64_WORD_SWAP( "epr-20684b.18", 0x000004, 0x200000, CRC(1fc15431) SHA1(c68c77dfcf5e2702214d64095ce07076d3702a5e) )
        ROM_LOAD64_WORD_SWAP( "epr-20685b.19", 0x000002, 0x200000, CRC(ae82cb35) SHA1(b4563f325945cc943a46bdc094e0169fcf82023d) )
        ROM_LOAD64_WORD_SWAP( "epr-20686b.20", 0x000000, 0x200000, CRC(3ea4de9f) SHA1(0d09e0a256e531c1e4115355e9ce29fa8016c458) )

	// CROM0:
        ROM_LOAD64_WORD_SWAP( "mpr-20647.1",   0x800006, 0x400000, CRC(e8586380) SHA1(67dd49975b31ba2c3f889ff38a3bc4663145934a) )
        ROM_LOAD64_WORD_SWAP( "mpr-20648.2",   0x800004, 0x400000, CRC(107309e0) SHA1(61657814a30020c0d4ea77625cb8f11a1db7e866) )
        ROM_LOAD64_WORD_SWAP( "mpr-20649.3",   0x800002, 0x400000, CRC(b8fd56ba) SHA1(5e5051d4b752463e1da632f8294a6c8f9250dbc8) )
        ROM_LOAD64_WORD_SWAP( "mpr-20650.4",   0x800000, 0x400000, CRC(81f96649) SHA1(0d7aba7654237b68de6e43811832fafaf61e2bec) )

	// CROM1
        ROM_LOAD64_WORD_SWAP( "mpr-20651.5",  0x1800006, 0x400000, CRC(8373cab3) SHA1(1d36612668a3004e2448f99ab27d7184ff859478) )
        ROM_LOAD64_WORD_SWAP( "mpr-20652.6",  0x1800004, 0x400000, CRC(64c6fbb6) SHA1(c8682bda20d3119b4f95bbd2dbde301bfd036608) )
        ROM_LOAD64_WORD_SWAP( "mpr-20653.7",  0x1800002, 0x400000, CRC(858e6bba) SHA1(22b71826799249a577124a49d5a276908a53ce61) )
        ROM_LOAD64_WORD_SWAP( "mpr-20654.8",  0x1800000, 0x400000, CRC(763ef905) SHA1(4d5f6b1770cf9bf6cecd4d3a91a822e5cc658464) )

	// CROM2
        ROM_LOAD64_WORD_SWAP( "mpr-20655.9",  0x2800006, 0x400000, CRC(f0a471e9) SHA1(8a40c9381e8b3733be297738c825b82abcb476d0) )
        ROM_LOAD64_WORD_SWAP( "mpr-20656.10", 0x2800004, 0x400000, CRC(466bee13) SHA1(bc2087a138037188f462fa1cecc898e5efb3e8b8) )
        ROM_LOAD64_WORD_SWAP( "mpr-20657.11", 0x2800002, 0x400000, CRC(14bf8964) SHA1(84444f7c489344ad1dd980b860364b5a4ed53038) )
        ROM_LOAD64_WORD_SWAP( "mpr-20658.12", 0x2800000, 0x400000, CRC(b80175b9) SHA1(26dc97f6a6e8415cbb7e9e1f64389d80a2b761a1) )

	// CROM3
        ROM_LOAD64_WORD_SWAP( "mpr-20659.13", 0x3800006, 0x400000, CRC(edb63e7b) SHA1(761abcfc213e813967d053475c965459a9724a24) )
        ROM_LOAD64_WORD_SWAP( "mpr-20660.14", 0x3800004, 0x400000, CRC(d961d385) SHA1(7e341c2cf24715c5cecb276c42166bf426860819) )
        ROM_LOAD64_WORD_SWAP( "mpr-20661.15", 0x3800002, 0x400000, CRC(50e6189e) SHA1(04be5ff1379af4972edec3b320f148bdf09bfbb5) )
        ROM_LOAD64_WORD_SWAP( "mpr-20662.16", 0x3800000, 0x400000, CRC(7130cb61) SHA1(39de0e3c2086f339156bfd734a196b667df7f5ac) )

	ROM_REGION( 0x2000000, REGION_USER3, ROMREGION_DISPOSE )  /* Video ROMs Part 1 */
        ROM_LOAD_VROM( "mpr-20667.26",   0x000002, 0x400000, CRC(321e006f) SHA1(687165bd2d2d22f861cd79083adcab62eb827c0f) )
        ROM_LOAD_VROM( "mpr-20668.27",   0x000000, 0x400000, CRC(c2dd8053) SHA1(52bc88d172d335b47e3ae3d582233382e9608de2) )
        ROM_LOAD_VROM( "mpr-20669.28",   0x000006, 0x400000, CRC(63432497) SHA1(b072741fe9ba49f1a7eed03301c8b1956af94d26) )
        ROM_LOAD_VROM( "mpr-20670.29",   0x000004, 0x400000, CRC(f7b554fd) SHA1(84fb08413345e0f3afb6e20c723aa8aa8156fdc7) )
        ROM_LOAD_VROM( "mpr-20671.30",   0x00000a, 0x400000, CRC(fee1a49b) SHA1(a024a0564df65e065e8b1830e85513d17ebd8635) )
        ROM_LOAD_VROM( "mpr-20672.31",   0x000008, 0x400000, CRC(e4b8c6e6) SHA1(674d4d26285f2825050fd27dd3382ca6245d54c7) )
        ROM_LOAD_VROM( "mpr-20673.32",   0x00000e, 0x400000, CRC(e7b6403b) SHA1(0f74f7a916c091d49eed8222050981a6b73d4bdd) )
        ROM_LOAD_VROM( "mpr-20674.33",   0x00000c, 0x400000, CRC(9be22e13) SHA1(a00b0c69b6ed086f3f61d4f767df6c4ddea45052) )

	ROM_REGION( 0x2000000, REGION_USER4, ROMREGION_DISPOSE )  /* Video ROMs Part 2 */
        ROM_LOAD_VROM( "mpr-20675.34",   0x000002, 0x400000, CRC(6a7c3862) SHA1(f77145c2a5e373f567783cf5db70e25b71e77bf5) )
        ROM_LOAD_VROM( "mpr-20676.35",   0x000000, 0x400000, CRC(dd299648) SHA1(c222c10cb23753ac3d6d1c779b2d026a64c61bc4) )
        ROM_LOAD_VROM( "mpr-20677.36",   0x000006, 0x400000, CRC(3fc5f330) SHA1(778c1932b093a4de96c76ea704463b7c67cdcb33) )
        ROM_LOAD_VROM( "mpr-20678.37",   0x000004, 0x400000, CRC(62f794a1) SHA1(fc7adafb49056b23b6cc483978ffe4fd3635977d) )
        ROM_LOAD_VROM( "mpr-20679.38",   0x00000a, 0x400000, CRC(35a37c53) SHA1(cd727a8914c3c01e302378048e3998b4cd849c4a) )
        ROM_LOAD_VROM( "mpr-20680.39",   0x000008, 0x400000, CRC(81fec46e) SHA1(43b3fbb544d920a87f77437860e32a628ae2865b) )
        ROM_LOAD_VROM( "mpr-20681.40",   0x00000e, 0x400000, CRC(d517873b) SHA1(8e50dd149716ae6b0b8d7ac99cd425a17b3c0a46) )
        ROM_LOAD_VROM( "mpr-20682.41",   0x00000c, 0x400000, CRC(5b43250c) SHA1(fccb40cd03c096360ca3c565e8621d4110b273ab) )

	ROM_REGION( 0x80000, REGION_CPU2, 0 )	/* 68000 code */
        ROM_LOAD16_WORD_SWAP( "epr-20687.21", 0x000000, 0x080000, CRC(fa084de5) SHA1(8a760b76bc12d60d4727f93106830f19179c9046) )

	// Samples
	ROM_REGION( 0x1000000, REGION_SOUND1, 0 )	/* SCSP samples */
	/* WARNING: mpr- numbers here are a guess based on how other sets are ordered and may not be right.
       If restoring a real PCB, go by the IC numbers in the extension! (.22, .24) */
        ROM_LOAD( "mpr-20663.22",  0x000000, 0x400000, CRC(977eb6a4) SHA1(9dbba51630cbef2351d79b82ab6ae3af4aed99f0) )
        ROM_LOAD( "mpr-20665.24",  0x400000, 0x400000, CRC(0efc0ca8) SHA1(1414becad21eb7d03d816a8cba47506f941b3c29) )
        ROM_LOAD( "mpr-20664.23",  0x800000, 0x400000, CRC(89220782) SHA1(18a3585af960a76eb08f187223e9b69ad16809a1) )
        ROM_LOAD( "mpr-20666.25",  0xc00000, 0x400000, CRC(3ecb2606) SHA1(a38d1f61933c8873deaff0a913c657b768f9783d) )

	ROM_REGION( 0x20000, REGION_CPU3, 0 )	/* Z80 code */
		ROM_FILL( 0x000000, 0x20000, 0 )

	ROM_REGION( 0x800000, REGION_SOUND2, 0 )	/* DSB samples */
		ROM_FILL( 0x000000, 0x800000, 0 )
ROM_END

ROM_START( von254g )	/* Step 2.0 */
	ROM_REGION64_BE( 0x4800000, REGION_USER1, 0 ) /* program + data ROMs */
	// CROM
        ROM_LOAD64_WORD_SWAP( "epr-21788.17",  0x000006, 0x200000, CRC(97066bcf) SHA1(234c45ee1f23b22f61893825eebf31d867cf420f) )
        ROM_LOAD64_WORD_SWAP( "epr-21789.18",  0x000004, 0x200000, CRC(3069108f) SHA1(f4e82da677458423abcf07c9c5a837005ed8f1c4) )
        ROM_LOAD64_WORD_SWAP( "epr-21790.19",  0x000002, 0x200000, CRC(2ae1efd3) SHA1(fc3957a140d741138a8eeacc19eedbb237f629cd) )
        ROM_LOAD64_WORD_SWAP( "epr-21791.20",  0x000000, 0x200000, CRC(d0bb3ca3) SHA1(7d00205b5366d7a6f9ecc10f8f7dcf335789a043) )

	// CROM0:
        ROM_LOAD64_WORD_SWAP( "mpr-20647.1",   0x800006, 0x400000, CRC(e8586380) SHA1(67dd49975b31ba2c3f889ff38a3bc4663145934a) )
        ROM_LOAD64_WORD_SWAP( "mpr-20648.2",   0x800004, 0x400000, CRC(107309e0) SHA1(61657814a30020c0d4ea77625cb8f11a1db7e866) )
        ROM_LOAD64_WORD_SWAP( "mpr-20649.3",   0x800002, 0x400000, CRC(b8fd56ba) SHA1(5e5051d4b752463e1da632f8294a6c8f9250dbc8) )
        ROM_LOAD64_WORD_SWAP( "mpr-20650.4",   0x800000, 0x400000, CRC(81f96649) SHA1(0d7aba7654237b68de6e43811832fafaf61e2bec) )

	// CROM1
        ROM_LOAD64_WORD_SWAP( "mpr-20651.5",  0x1800006, 0x400000, CRC(8373cab3) SHA1(1d36612668a3004e2448f99ab27d7184ff859478) )
        ROM_LOAD64_WORD_SWAP( "mpr-20652.6",  0x1800004, 0x400000, CRC(64c6fbb6) SHA1(c8682bda20d3119b4f95bbd2dbde301bfd036608) )
        ROM_LOAD64_WORD_SWAP( "mpr-20653.7",  0x1800002, 0x400000, CRC(858e6bba) SHA1(22b71826799249a577124a49d5a276908a53ce61) )
        ROM_LOAD64_WORD_SWAP( "mpr-20654.8",  0x1800000, 0x400000, CRC(763ef905) SHA1(4d5f6b1770cf9bf6cecd4d3a91a822e5cc658464) )

	// CROM2
        ROM_LOAD64_WORD_SWAP( "mpr-20655.9",  0x2800006, 0x400000, CRC(f0a471e9) SHA1(8a40c9381e8b3733be297738c825b82abcb476d0) )
        ROM_LOAD64_WORD_SWAP( "mpr-20656.10", 0x2800004, 0x400000, CRC(466bee13) SHA1(bc2087a138037188f462fa1cecc898e5efb3e8b8) )
        ROM_LOAD64_WORD_SWAP( "mpr-20657.11", 0x2800002, 0x400000, CRC(14bf8964) SHA1(84444f7c489344ad1dd980b860364b5a4ed53038) )
        ROM_LOAD64_WORD_SWAP( "mpr-20658.12", 0x2800000, 0x400000, CRC(b80175b9) SHA1(26dc97f6a6e8415cbb7e9e1f64389d80a2b761a1) )

	// CROM3
        ROM_LOAD64_WORD_SWAP( "mpr-20659.13", 0x3800006, 0x400000, CRC(edb63e7b) SHA1(761abcfc213e813967d053475c965459a9724a24) )
        ROM_LOAD64_WORD_SWAP( "mpr-20660.14", 0x3800004, 0x400000, CRC(d961d385) SHA1(7e341c2cf24715c5cecb276c42166bf426860819) )
        ROM_LOAD64_WORD_SWAP( "mpr-20661.15", 0x3800002, 0x400000, CRC(50e6189e) SHA1(04be5ff1379af4972edec3b320f148bdf09bfbb5) )
        ROM_LOAD64_WORD_SWAP( "mpr-20662.16", 0x3800000, 0x400000, CRC(7130cb61) SHA1(39de0e3c2086f339156bfd734a196b667df7f5ac) )

	ROM_REGION( 0x2000000, REGION_USER3, ROMREGION_DISPOSE )  /* Video ROMs Part 1 */
        ROM_LOAD_VROM( "mpr-20667.26",   0x000002, 0x400000, CRC(321e006f) SHA1(687165bd2d2d22f861cd79083adcab62eb827c0f) )
        ROM_LOAD_VROM( "mpr-20668.27",   0x000000, 0x400000, CRC(c2dd8053) SHA1(52bc88d172d335b47e3ae3d582233382e9608de2) )
        ROM_LOAD_VROM( "mpr-20669.28",   0x000006, 0x400000, CRC(63432497) SHA1(b072741fe9ba49f1a7eed03301c8b1956af94d26) )
        ROM_LOAD_VROM( "mpr-20670.29",   0x000004, 0x400000, CRC(f7b554fd) SHA1(84fb08413345e0f3afb6e20c723aa8aa8156fdc7) )
        ROM_LOAD_VROM( "mpr-20671.30",   0x00000a, 0x400000, CRC(fee1a49b) SHA1(a024a0564df65e065e8b1830e85513d17ebd8635) )
        ROM_LOAD_VROM( "mpr-20672.31",   0x000008, 0x400000, CRC(e4b8c6e6) SHA1(674d4d26285f2825050fd27dd3382ca6245d54c7) )
        ROM_LOAD_VROM( "mpr-20673.32",   0x00000e, 0x400000, CRC(e7b6403b) SHA1(0f74f7a916c091d49eed8222050981a6b73d4bdd) )
        ROM_LOAD_VROM( "mpr-20674.33",   0x00000c, 0x400000, CRC(9be22e13) SHA1(a00b0c69b6ed086f3f61d4f767df6c4ddea45052) )

	ROM_REGION( 0x2000000, REGION_USER4, ROMREGION_DISPOSE )  /* Video ROMs Part 2 */
        ROM_LOAD_VROM( "mpr-20675.34",   0x000002, 0x400000, CRC(6a7c3862) SHA1(f77145c2a5e373f567783cf5db70e25b71e77bf5) )
        ROM_LOAD_VROM( "mpr-20676.35",   0x000000, 0x400000, CRC(dd299648) SHA1(c222c10cb23753ac3d6d1c779b2d026a64c61bc4) )
        ROM_LOAD_VROM( "mpr-20677.36",   0x000006, 0x400000, CRC(3fc5f330) SHA1(778c1932b093a4de96c76ea704463b7c67cdcb33) )
        ROM_LOAD_VROM( "mpr-20678.37",   0x000004, 0x400000, CRC(62f794a1) SHA1(fc7adafb49056b23b6cc483978ffe4fd3635977d) )
        ROM_LOAD_VROM( "mpr-20679.38",   0x00000a, 0x400000, CRC(35a37c53) SHA1(cd727a8914c3c01e302378048e3998b4cd849c4a) )
        ROM_LOAD_VROM( "mpr-20680.39",   0x000008, 0x400000, CRC(81fec46e) SHA1(43b3fbb544d920a87f77437860e32a628ae2865b) )
        ROM_LOAD_VROM( "mpr-20681.40",   0x00000e, 0x400000, CRC(d517873b) SHA1(8e50dd149716ae6b0b8d7ac99cd425a17b3c0a46) )
        ROM_LOAD_VROM( "mpr-20682.41",   0x00000c, 0x400000, CRC(5b43250c) SHA1(fccb40cd03c096360ca3c565e8621d4110b273ab) )

	ROM_REGION( 0x80000, REGION_CPU2, 0 )	/* 68000 code */
        ROM_LOAD16_WORD_SWAP( "epr-20687.21", 0x000000, 0x080000, CRC(fa084de5) SHA1(8a760b76bc12d60d4727f93106830f19179c9046) )

	// Samples
	ROM_REGION( 0x1000000, REGION_SOUND1, 0 )	/* SCSP samples */
	/* WARNING: mpr- numbers here are a guess based on how other sets are ordered and may not be right.
       If restoring a real PCB, go by the IC numbers in the extension! (.22, .24) */
        ROM_LOAD( "mpr-20663.22",  0x000000, 0x400000, CRC(977eb6a4) SHA1(9dbba51630cbef2351d79b82ab6ae3af4aed99f0) )
        ROM_LOAD( "mpr-20665.24",  0x400000, 0x400000, CRC(0efc0ca8) SHA1(1414becad21eb7d03d816a8cba47506f941b3c29) )
        ROM_LOAD( "mpr-20664.23",  0x800000, 0x400000, CRC(89220782) SHA1(18a3585af960a76eb08f187223e9b69ad16809a1) )
        ROM_LOAD( "mpr-20666.25",  0xc00000, 0x400000, CRC(3ecb2606) SHA1(a38d1f61933c8873deaff0a913c657b768f9783d) )

	ROM_REGION( 0x20000, REGION_CPU3, 0 )	/* Z80 code */
		ROM_FILL( 0x000000, 0x20000, 0 )

	ROM_REGION( 0x800000, REGION_SOUND2, 0 )	/* DSB samples */
		ROM_FILL( 0x000000, 0x800000, 0 )
ROM_END


ROM_START( swtrilgy )	/* Step 2.1 */
	ROM_REGION64_BE( 0x4800000, REGION_USER1, 0 ) /* program + data ROMs */
	// CROM
        ROM_LOAD64_WORD_SWAP( "epr-21379a.17",  0x000006, 0x200000, CRC(24dc1555) SHA1(0a4b458bb09238de0f38ba2805512b5dbee7d58e) )
        ROM_LOAD64_WORD_SWAP( "epr-21380a.18",  0x000004, 0x200000, CRC(780fb4e7) SHA1(6650e114bad0e4c3f67b744599dba9845da82f11) )
        ROM_LOAD64_WORD_SWAP( "epr-21381a.19",  0x000002, 0x200000, CRC(2dd34e28) SHA1(b9d2034aee6be2313f7286091f4660bbba87e376) )
        ROM_LOAD64_WORD_SWAP( "epr-21382a.20",  0x000000, 0x200000, CRC(69baf117) SHA1(ceba6b9b092953b00777e5309bbba527270b0c40) )

	// CROM0
        ROM_LOAD64_WORD_SWAP( "mpr-21339.01",   0x800006, 0x400000, CRC(c0ce5037) SHA1(1765339ac94b7e7e54217cc9703610f6a39eda4f) )
        ROM_LOAD64_WORD_SWAP( "mpr-21340.02",   0x800004, 0x400000, CRC(ad36040e) SHA1(d40b888f892aa40b1d7f375cf7523667f3b7ee12) )
        ROM_LOAD64_WORD_SWAP( "mpr-21341.03",   0x800002, 0x400000, CRC(b2a269e4) SHA1(b32769251118d3ae5c1c30864b58e27365b9602d) )
        ROM_LOAD64_WORD_SWAP( "mpr-21342.04",   0x800000, 0x400000, CRC(339525ce) SHA1(e93ee61705612ae7017d9d99b26a6b5c2d20a15c) )

	// CROM1
        ROM_LOAD64_WORD_SWAP( "mpr-21343.05",  0x1800006, 0x400000, CRC(12552d07) SHA1(7ed31feecda71f44ca7d3409d753d75c36a03ae8) )
        ROM_LOAD64_WORD_SWAP( "mpr-21344.06",  0x1800004, 0x400000, CRC(87453d76) SHA1(5793ac0b4ae02364821d82e8cf30baf676bc7649) )
        ROM_LOAD64_WORD_SWAP( "mpr-21345.07",  0x1800002, 0x400000, CRC(6c183a21) SHA1(416e56b76464df4aed552fe2e7262334e5841b17) )
        ROM_LOAD64_WORD_SWAP( "mpr-21346.08",  0x1800000, 0x400000, CRC(c8733594) SHA1(1196ec899c64c58f21d56bff56e432b156aacb31) )

	// CROM2
        ROM_LOAD64_WORD_SWAP( "mpr-21347.09",  0x2800006, 0x400000, CRC(ecb6b934) SHA1(49e394adc3c339a13df6679457b910b9e0a078c1) )
        ROM_LOAD64_WORD_SWAP( "mpr-21348.10",  0x2800004, 0x400000, CRC(1f7cc5f5) SHA1(6ac1bef009ba86e97541f4d6bbdb935fb8a22f5a) )
        ROM_LOAD64_WORD_SWAP( "mpr-21349.11",  0x2800002, 0x400000, CRC(3d39454b) SHA1(1c55339a0694fc817e7ee2f2087c7548361c3f8b) )
        ROM_LOAD64_WORD_SWAP( "mpr-21350.12",  0x2800000, 0x400000, CRC(486195e7) SHA1(b3725da2317561b8570666e459737022370256a8) )

	ROM_REGION( 0x2000000, REGION_USER3, ROMREGION_DISPOSE )  /* Video ROMs Part 1 */
        ROM_LOAD_VROM( "mpr-21359.26",  0x000002, 0x400000, CRC(34ef4122) SHA1(26e0726e1ab722ba0e12624efd01af3a40fc320b) )
        ROM_LOAD_VROM( "mpr-21360.27",  0x000000, 0x400000, CRC(2882b95e) SHA1(e553661c98da3e23318920576488b8ff97430f44) )
        ROM_LOAD_VROM( "mpr-21361.28",  0x000006, 0x400000, CRC(9b61c3c1) SHA1(93b4acb9340176b578f8222fcaf8fc67fd874556) )
        ROM_LOAD_VROM( "mpr-21362.29",  0x000004, 0x400000, CRC(01a92169) SHA1(0911d5a656a5b5de5fefab77ea34a1b495863610) )
        ROM_LOAD_VROM( "mpr-21363.30",  0x00000a, 0x400000, CRC(e7d18fed) SHA1(3e77e09db4f00780a5bcf6e644bfdc72b9d4ac83) )
        ROM_LOAD_VROM( "mpr-21364.31",  0x000008, 0x400000, CRC(cb6a5468) SHA1(3ca093646b565eb6298c3d66da83664f718fe76a) )
        ROM_LOAD_VROM( "mpr-21365.32",  0x00000e, 0x400000, CRC(ad5449d8) SHA1(e7f1b4b6ebbe578f292b5a71258c79767f57cf90) )
        ROM_LOAD_VROM( "mpr-21366.33",  0x00000c, 0x400000, CRC(defb6b95) SHA1(a5de55c8e4bcbf2aef93972e3aba22ba64e46fdb) )

	ROM_REGION( 0x2000000, REGION_USER4, ROMREGION_DISPOSE )  /* Video ROMs Part 2 */
        ROM_LOAD_VROM( "mpr-21367.34",  0x000002, 0x400000, CRC(dfd51029) SHA1(44c2e6c8e36217bb4fc0743912f0b046a1944a74) )
        ROM_LOAD_VROM( "mpr-21368.35",  0x000000, 0x400000, CRC(ae90fd21) SHA1(a519add40e29e6b737f50d9314a6009d4c696a9f) )
        ROM_LOAD_VROM( "mpr-21369.36",  0x000006, 0x400000, CRC(bf17eeb4) SHA1(12d5f0c9c6ad27a225dbecdc7b94ade0e90a8f00) )
        ROM_LOAD_VROM( "mpr-21370.37",  0x000004, 0x400000, CRC(2321592a) SHA1(d4270b872e1a5ff82220014c65b726309305ecb0) )
        ROM_LOAD_VROM( "mpr-21371.38",  0x00000a, 0x400000, CRC(a68782fd) SHA1(610530f804876206fdd2c2f9ff159db9813fabea) )
        ROM_LOAD_VROM( "mpr-21372.39",  0x000008, 0x400000, CRC(fc3f4e8b) SHA1(47240f14d81458e104452125eabf44619e026ff9) )
        ROM_LOAD_VROM( "mpr-21373.40",  0x00000e, 0x400000, CRC(b76ad261) SHA1(de5a39a23ac6b12b17f16f2b3e82d1f5470ae600) )
        ROM_LOAD_VROM( "mpr-21374.41",  0x00000c, 0x400000, CRC(ae6c4d28) SHA1(b57733cfaa63ba018b0c3c9c935c12c48cc7f184) )

	ROM_REGION( 0x80000, REGION_CPU2, 0 )	/* 68000 code */
        ROM_LOAD16_WORD_SWAP( "epr-21383.21",  0x000000, 0x080000, CRC(544d1e28) SHA1(8b4c99cf9ad0cf15d2d3da578bbc08705bafb829) )

	// Samples
	ROM_REGION( 0x800000, REGION_SOUND1, 0 )	/* SCSP samples */
        ROM_LOAD( "mpr-21355.22",  0x000000, 0x400000, CRC(c1b2d326) SHA1(118d9e02cdb9f500bd677b1de8331b29c57ca02f) )
        ROM_LOAD( "mpr-21357.24",  0x400000, 0x400000, CRC(02703fab) SHA1(c312f3d7967229660a7fb81b4fcd16c204d671cd) )

	ROM_REGION( 0x20000, REGION_CPU3, 0 )	/* Z80 code */
	ROM_LOAD16_WORD_SWAP( "ep21384.2", 0x000000, 0x20000, CRC(12fa4780) SHA1(a10ce82d81045cc49efcfba490693d06aeced3ae) )

	ROM_REGION( 0x1000000, REGION_SOUND2, 0 )	/* DSB samples */
	ROM_LOAD( "mp21375.18", 0x000000, 0x400000, CRC(735157a9) SHA1(d1ff5dc7a6be8c8b0b6ba33fdf353c2008507afc) )
	ROM_LOAD( "mp21376.20", 0x400000, 0x400000, CRC(e635f81e) SHA1(3eb4243fd275946ce0e85d074abd59b5ed31bbcd) )
	ROM_LOAD( "mp21377.22", 0x800000, 0x400000, CRC(720621f8) SHA1(191bd8159010c172a82159d0ebfa56637c2a8462) )
	ROM_LOAD( "mp21378.24", 0xc00000, 0x400000, CRC(1fcf715e) SHA1(9706f36e7a61d885d34a6974311a2410fe3d6760) )
ROM_END

ROM_START( swtrilga )	/* Step 2.1 */
	ROM_REGION64_BE( 0x4800000, REGION_USER1, 0 ) /* program + data ROMs */
	// CROM
        ROM_LOAD64_WORD_SWAP( "ep21379.17",  0x000006, 0x200000, CRC(61ad51d9) SHA1(b27ea929702bb94c86d03d6c1f479af32230b4d0) )
        ROM_LOAD64_WORD_SWAP( "ep21380.18",  0x000004, 0x200000, CRC(49b182f2) SHA1(9a4e3180f2661c95976963ab17e66a5184bca9a3) )
        ROM_LOAD64_WORD_SWAP( "ep21381.19",  0x000002, 0x200000, CRC(bb5757bf) SHA1(4e803f2a5fe09c82c0318d22240d3738e51e7e3b) )
        ROM_LOAD64_WORD_SWAP( "ep21382.20",  0x000000, 0x200000, CRC(0b9c44a0) SHA1(e7171bda684263a92746a84fd91ac32b788bfab4) )

	// CROM0
        ROM_LOAD64_WORD_SWAP( "mpr-21339.01",   0x800006, 0x400000, CRC(c0ce5037) SHA1(1765339ac94b7e7e54217cc9703610f6a39eda4f) )
        ROM_LOAD64_WORD_SWAP( "mpr-21340.02",   0x800004, 0x400000, CRC(ad36040e) SHA1(d40b888f892aa40b1d7f375cf7523667f3b7ee12) )
        ROM_LOAD64_WORD_SWAP( "mpr-21341.03",   0x800002, 0x400000, CRC(b2a269e4) SHA1(b32769251118d3ae5c1c30864b58e27365b9602d) )
        ROM_LOAD64_WORD_SWAP( "mpr-21342.04",   0x800000, 0x400000, CRC(339525ce) SHA1(e93ee61705612ae7017d9d99b26a6b5c2d20a15c) )

	// CROM1
        ROM_LOAD64_WORD_SWAP( "mpr-21343.05",  0x1800006, 0x400000, CRC(12552d07) SHA1(7ed31feecda71f44ca7d3409d753d75c36a03ae8) )
        ROM_LOAD64_WORD_SWAP( "mpr-21344.06",  0x1800004, 0x400000, CRC(87453d76) SHA1(5793ac0b4ae02364821d82e8cf30baf676bc7649) )
        ROM_LOAD64_WORD_SWAP( "mpr-21345.07",  0x1800002, 0x400000, CRC(6c183a21) SHA1(416e56b76464df4aed552fe2e7262334e5841b17) )
        ROM_LOAD64_WORD_SWAP( "mpr-21346.08",  0x1800000, 0x400000, CRC(c8733594) SHA1(1196ec899c64c58f21d56bff56e432b156aacb31) )

	// CROM2
        ROM_LOAD64_WORD_SWAP( "mpr-21347.09",  0x2800006, 0x400000, CRC(ecb6b934) SHA1(49e394adc3c339a13df6679457b910b9e0a078c1) )
        ROM_LOAD64_WORD_SWAP( "mpr-21348.10",  0x2800004, 0x400000, CRC(1f7cc5f5) SHA1(6ac1bef009ba86e97541f4d6bbdb935fb8a22f5a) )
        ROM_LOAD64_WORD_SWAP( "mpr-21349.11",  0x2800002, 0x400000, CRC(3d39454b) SHA1(1c55339a0694fc817e7ee2f2087c7548361c3f8b) )
        ROM_LOAD64_WORD_SWAP( "mpr-21350.12",  0x2800000, 0x400000, CRC(486195e7) SHA1(b3725da2317561b8570666e459737022370256a8) )

	ROM_REGION( 0x2000000, REGION_USER3, ROMREGION_DISPOSE )  /* Video ROMs Part 1 */
        ROM_LOAD_VROM( "mpr-21359.26",  0x000002, 0x400000, CRC(34ef4122) SHA1(26e0726e1ab722ba0e12624efd01af3a40fc320b) )
        ROM_LOAD_VROM( "mpr-21360.27",  0x000000, 0x400000, CRC(2882b95e) SHA1(e553661c98da3e23318920576488b8ff97430f44) )
        ROM_LOAD_VROM( "mpr-21361.28",  0x000006, 0x400000, CRC(9b61c3c1) SHA1(93b4acb9340176b578f8222fcaf8fc67fd874556) )
        ROM_LOAD_VROM( "mpr-21362.29",  0x000004, 0x400000, CRC(01a92169) SHA1(0911d5a656a5b5de5fefab77ea34a1b495863610) )
        ROM_LOAD_VROM( "mpr-21363.30",  0x00000a, 0x400000, CRC(e7d18fed) SHA1(3e77e09db4f00780a5bcf6e644bfdc72b9d4ac83) )
        ROM_LOAD_VROM( "mpr-21364.31",  0x000008, 0x400000, CRC(cb6a5468) SHA1(3ca093646b565eb6298c3d66da83664f718fe76a) )
        ROM_LOAD_VROM( "mpr-21365.32",  0x00000e, 0x400000, CRC(ad5449d8) SHA1(e7f1b4b6ebbe578f292b5a71258c79767f57cf90) )
        ROM_LOAD_VROM( "mpr-21366.33",  0x00000c, 0x400000, CRC(defb6b95) SHA1(a5de55c8e4bcbf2aef93972e3aba22ba64e46fdb) )

	ROM_REGION( 0x2000000, REGION_USER4, ROMREGION_DISPOSE )  /* Video ROMs Part 2 */
        ROM_LOAD_VROM( "mpr-21367.34",  0x000002, 0x400000, CRC(dfd51029) SHA1(44c2e6c8e36217bb4fc0743912f0b046a1944a74) )
        ROM_LOAD_VROM( "mpr-21368.35",  0x000000, 0x400000, CRC(ae90fd21) SHA1(a519add40e29e6b737f50d9314a6009d4c696a9f) )
        ROM_LOAD_VROM( "mpr-21369.36",  0x000006, 0x400000, CRC(bf17eeb4) SHA1(12d5f0c9c6ad27a225dbecdc7b94ade0e90a8f00) )
        ROM_LOAD_VROM( "mpr-21370.37",  0x000004, 0x400000, CRC(2321592a) SHA1(d4270b872e1a5ff82220014c65b726309305ecb0) )
        ROM_LOAD_VROM( "mpr-21371.38",  0x00000a, 0x400000, CRC(a68782fd) SHA1(610530f804876206fdd2c2f9ff159db9813fabea) )
        ROM_LOAD_VROM( "mpr-21372.39",  0x000008, 0x400000, CRC(fc3f4e8b) SHA1(47240f14d81458e104452125eabf44619e026ff9) )
        ROM_LOAD_VROM( "mpr-21373.40",  0x00000e, 0x400000, CRC(b76ad261) SHA1(de5a39a23ac6b12b17f16f2b3e82d1f5470ae600) )
        ROM_LOAD_VROM( "mpr-21374.41",  0x00000c, 0x400000, CRC(ae6c4d28) SHA1(b57733cfaa63ba018b0c3c9c935c12c48cc7f184) )

	ROM_REGION( 0x80000, REGION_CPU2, 0 )	/* 68000 code */
        ROM_LOAD16_WORD_SWAP( "epr-21383.21",  0x000000, 0x080000, CRC(544d1e28) SHA1(8b4c99cf9ad0cf15d2d3da578bbc08705bafb829) )

	// Samples
	ROM_REGION( 0x800000, REGION_SOUND1, 0 )	/* SCSP samples */
        ROM_LOAD( "mpr-21355.22",  0x000000, 0x400000, CRC(c1b2d326) SHA1(118d9e02cdb9f500bd677b1de8331b29c57ca02f) )
        ROM_LOAD( "mpr-21357.24",  0x400000, 0x400000, CRC(02703fab) SHA1(c312f3d7967229660a7fb81b4fcd16c204d671cd) )

	ROM_REGION( 0x20000, REGION_CPU3, 0 )	/* Z80 code */
	ROM_LOAD16_WORD_SWAP( "ep21384.2", 0x000000, 0x20000, CRC(12fa4780) SHA1(a10ce82d81045cc49efcfba490693d06aeced3ae) )

	ROM_REGION( 0x1000000, REGION_SOUND2, 0 )	/* DSB samples */
	ROM_LOAD( "mp21375.18", 0x000000, 0x400000, CRC(735157a9) SHA1(d1ff5dc7a6be8c8b0b6ba33fdf353c2008507afc) )
	ROM_LOAD( "mp21376.20", 0x400000, 0x400000, CRC(e635f81e) SHA1(3eb4243fd275946ce0e85d074abd59b5ed31bbcd) )
	ROM_LOAD( "mp21377.22", 0x800000, 0x400000, CRC(720621f8) SHA1(191bd8159010c172a82159d0ebfa56637c2a8462) )
	ROM_LOAD( "mp21378.24", 0xc00000, 0x400000, CRC(1fcf715e) SHA1(9706f36e7a61d885d34a6974311a2410fe3d6760) )
ROM_END

ROM_START( dirtdvls )	/* Step 2.1 */
	ROM_REGION64_BE( 0x4800000, REGION_USER1, 0 ) /* program + data ROMs */
	// CROM
        ROM_LOAD64_WORD_SWAP( "epr-21062a.17", 0x000006, 0x200000, CRC(64b55254) SHA1(0e5de3786edad77dde08652ac837dc9125e7851c) )
        ROM_LOAD64_WORD_SWAP( "epr-21063a.18", 0x000004, 0x200000, CRC(6ab7eb32) SHA1(3a4226d4c786e7b64688af3b8883b4039b8c8407) )
        ROM_LOAD64_WORD_SWAP( "epr-21064a.19", 0x000002, 0x200000, CRC(2a01f9ad) SHA1(d936d8eeecbcc502e35799d484c36f5da9457013) )
        ROM_LOAD64_WORD_SWAP( "epr-21065a.20", 0x000000, 0x200000, CRC(3223db1a) SHA1(ad34d16475a571ffed48539ef98736357cb327b0) )

	// CROM0
        ROM_LOAD64_WORD_SWAP( "mpr-21023.1",   0x800006, 0x400000, CRC(932a3724) SHA1(146dfe897caa8a4385c527bc7c649e9dbd2ce0c0) )
        ROM_LOAD64_WORD_SWAP( "mpr-21024.2",   0x800004, 0x400000, CRC(ede859b0) SHA1(cecd595a6ba60e248b7bf47778ba4da7658dcf93) )
        ROM_LOAD64_WORD_SWAP( "mpr-21025.3",   0x800002, 0x400000, CRC(6591c66e) SHA1(feaae431692a3bab867b79d52bc3934f77c4022b) )
        ROM_LOAD64_WORD_SWAP( "mpr-21026.4",   0x800000, 0x400000, CRC(f4937e3f) SHA1(21559ef991789ede4b4e7297e2a71f33f7cc7090) )

	// CROM1
        ROM_LOAD64_WORD_SWAP( "mpr-21027.5",  0x1800006, 0x400000, CRC(74e1496a) SHA1(0988058a109216e8b97045dde9d1099688193a13) )
        ROM_LOAD64_WORD_SWAP( "mpr-21028.6",  0x1800004, 0x400000, CRC(db11f50a) SHA1(78bf2418bcea1ed30da9af936e9f95e9c76ce919) )
        ROM_LOAD64_WORD_SWAP( "mpr-21029.7",  0x1800002, 0x400000, CRC(89867d8a) SHA1(89ebd5bc5d98fbd63d4cad407033419a39b1d60a) )
        ROM_LOAD64_WORD_SWAP( "mpr-21030.8",  0x1800000, 0x400000, CRC(f8e51bec) SHA1(fe8a06ef21dd646e3ad6fa382e3f3d30db4cbd91) )

	ROM_REGION( 0x2000000, REGION_USER3, ROMREGION_DISPOSE )  /* Video ROMs Part 1 */
        ROM_LOAD_VROM( "mpr-21034.26",  0x000002, 0x400000, CRC(acba5ca6) SHA1(be213ca40d17f18e725349585f95d677e53c1bfc) )
        ROM_LOAD_VROM( "mpr-21035.27",  0x000000, 0x400000, CRC(618b7d6a) SHA1(0968b72c8d7fc4b2635062647da5d36a58e69b08) )
        ROM_LOAD_VROM( "mpr-21036.28",  0x000006, 0x400000, CRC(0e665bb2) SHA1(3b18ea93ed1d71873ff635358c3143e4f515bab9) )
        ROM_LOAD_VROM( "mpr-21037.29",  0x000004, 0x400000, CRC(90b98493) SHA1(3f98855caec5895c8651ed88e07f2dcec5a6c66a) )
        ROM_LOAD_VROM( "mpr-21038.30",  0x00000a, 0x400000, CRC(9b59d2c2) SHA1(3f14cfc905a018e0aa2b2ad4918cd4ee2ef65c7b) )
        ROM_LOAD_VROM( "mpr-21039.31",  0x000008, 0x400000, CRC(61407b07) SHA1(d7676a03110ca694cc53c1d3a6c781d2f8cee98b) )
        ROM_LOAD_VROM( "mpr-21040.32",  0x00000e, 0x400000, CRC(b550c229) SHA1(b13ea462914bb13388e11bed9a9b2e696a8eb759) )
        ROM_LOAD_VROM( "mpr-21041.33",  0x00000c, 0x400000, CRC(8f1ac988) SHA1(11b628c85533a307298765641eb87c305bde64d1) )

	ROM_REGION( 0x2000000, REGION_USER4, ROMREGION_DISPOSE )  /* Video ROMs Part 2 */
        ROM_LOAD_VROM( "mpr-21042.34",  0x000002, 0x400000, CRC(1dab621d) SHA1(cf0e59be7b5a12146f5562e208009054074151cd) )
        ROM_LOAD_VROM( "mpr-21043.35",  0x000000, 0x400000, CRC(707015c8) SHA1(125ff08cc555a4c8d9863e7433fad7949230630d) )
        ROM_LOAD_VROM( "mpr-21044.36",  0x000006, 0x400000, CRC(776f9580) SHA1(0529532975d74da851a2fd1ce9810e218d751d5f) )
        ROM_LOAD_VROM( "mpr-21045.37",  0x000004, 0x400000, CRC(a28ad02f) SHA1(8734568153dbf304193491e746b19a423a547f0d) )
        ROM_LOAD_VROM( "mpr-21046.38",  0x00000a, 0x400000, CRC(05c995ae) SHA1(d96391360692d30c456324dcd51511bf095a58cb) )
        ROM_LOAD_VROM( "mpr-21047.39",  0x000008, 0x400000, CRC(06b7826f) SHA1(cfdeb56964bd31196fde01b1f5cc294c8b49c215) )
        ROM_LOAD_VROM( "mpr-21048.40",  0x00000e, 0x400000, CRC(96849974) SHA1(347e2216ea1225eda92693dcd80eb97df88caabf) )
        ROM_LOAD_VROM( "mpr-21049.41",  0x00000c, 0x400000, CRC(91e8161a) SHA1(1edc0bc856e5d72f714bd0544814727f4ff12e7a) )

	ROM_REGION( 0x80000, REGION_CPU2, 0 )	/* 68000 code */
        ROM_LOAD16_WORD_SWAP( "epr-21066.21",  0x000000, 0x080000, CRC(f7ed2582) SHA1(a4f80d5f82c86f0bdb74bcda5dc69b83b475c542) )

	ROM_REGION( 0x1000000, REGION_SOUND1, 0 )	/* SCSP samples */
        ROM_LOAD( "mpr-21031.22",  0x000000, 0x400000, CRC(32f6b23a) SHA1(8cd092733b85aecf607c2f4b683c42e388a70906) )
        ROM_LOAD( "mpr-21033.24",  0x400000, 0x400000, CRC(253d3c70) SHA1(bfbc42d08cf46d89c87505f53e31b8a53e8a729a) )
        ROM_LOAD( "mpr-21032.23",  0x800000, 0x400000, CRC(3d3ff407) SHA1(5e298e24cb3050f8683658cef41ce59948e79166) )

	ROM_REGION( 0x20000, REGION_CPU3, 0 )	/* Z80 code */
		ROM_FILL( 0x000000, 0x20000, 0 )

	ROM_REGION( 0x800000, REGION_SOUND2, 0 )	/* DSB samples */
		ROM_FILL( 0x000000, 0x800000, 0 )
ROM_END

ROM_START( dirtdvla )	/* Step 2.1 */
	ROM_REGION64_BE( 0x4800000, REGION_USER1, 0 ) /* program + data ROMs */
	// CROM
        ROM_LOAD64_WORD_SWAP( "epr-21058a", 0x000006, 0x200000, CRC(4d7fdc8d) SHA1(c45031b4e3ea65519de671e0e11f87e0965e3c93) )
        ROM_LOAD64_WORD_SWAP( "epr-21059a", 0x000004, 0x200000, CRC(f31a2aa4) SHA1(b7398db217372885f763efdb909f3e43ccbac34a) )
        ROM_LOAD64_WORD_SWAP( "epr-21060a", 0x000002, 0x200000, CRC(5ebe2816) SHA1(9ebbaf69f4a3b071d65ce3cbe6aabcd7547f1634) )
        ROM_LOAD64_WORD_SWAP( "epr-21061a", 0x000000, 0x200000, CRC(755ca612) SHA1(ba21cf7f445bf1c33962affd0400247e27268233) )

	// CROM0
        ROM_LOAD64_WORD_SWAP( "mpr-21023.1",   0x800006, 0x400000, CRC(932a3724) SHA1(146dfe897caa8a4385c527bc7c649e9dbd2ce0c0) )
        ROM_LOAD64_WORD_SWAP( "mpr-21024.2",   0x800004, 0x400000, CRC(ede859b0) SHA1(cecd595a6ba60e248b7bf47778ba4da7658dcf93) )
        ROM_LOAD64_WORD_SWAP( "mpr-21025.3",   0x800002, 0x400000, CRC(6591c66e) SHA1(feaae431692a3bab867b79d52bc3934f77c4022b) )
        ROM_LOAD64_WORD_SWAP( "mpr-21026.4",   0x800000, 0x400000, CRC(f4937e3f) SHA1(21559ef991789ede4b4e7297e2a71f33f7cc7090) )

	// CROM1
        ROM_LOAD64_WORD_SWAP( "mpr-21027.5",  0x1800006, 0x400000, CRC(74e1496a) SHA1(0988058a109216e8b97045dde9d1099688193a13) )
        ROM_LOAD64_WORD_SWAP( "mpr-21028.6",  0x1800004, 0x400000, CRC(db11f50a) SHA1(78bf2418bcea1ed30da9af936e9f95e9c76ce919) )
        ROM_LOAD64_WORD_SWAP( "mpr-21029.7",  0x1800002, 0x400000, CRC(89867d8a) SHA1(89ebd5bc5d98fbd63d4cad407033419a39b1d60a) )
        ROM_LOAD64_WORD_SWAP( "mpr-21030.8",  0x1800000, 0x400000, CRC(f8e51bec) SHA1(fe8a06ef21dd646e3ad6fa382e3f3d30db4cbd91) )

	ROM_REGION( 0x2000000, REGION_USER3, ROMREGION_DISPOSE )  /* Video ROMs Part 1 */
        ROM_LOAD_VROM( "mpr-21034.26",  0x000002, 0x400000, CRC(acba5ca6) SHA1(be213ca40d17f18e725349585f95d677e53c1bfc) )
        ROM_LOAD_VROM( "mpr-21035.27",  0x000000, 0x400000, CRC(618b7d6a) SHA1(0968b72c8d7fc4b2635062647da5d36a58e69b08) )
        ROM_LOAD_VROM( "mpr-21036.28",  0x000006, 0x400000, CRC(0e665bb2) SHA1(3b18ea93ed1d71873ff635358c3143e4f515bab9) )
        ROM_LOAD_VROM( "mpr-21037.29",  0x000004, 0x400000, CRC(90b98493) SHA1(3f98855caec5895c8651ed88e07f2dcec5a6c66a) )
        ROM_LOAD_VROM( "mpr-21038.30",  0x00000a, 0x400000, CRC(9b59d2c2) SHA1(3f14cfc905a018e0aa2b2ad4918cd4ee2ef65c7b) )
        ROM_LOAD_VROM( "mpr-21039.31",  0x000008, 0x400000, CRC(61407b07) SHA1(d7676a03110ca694cc53c1d3a6c781d2f8cee98b) )
        ROM_LOAD_VROM( "mpr-21040.32",  0x00000e, 0x400000, CRC(b550c229) SHA1(b13ea462914bb13388e11bed9a9b2e696a8eb759) )
        ROM_LOAD_VROM( "mpr-21041.33",  0x00000c, 0x400000, CRC(8f1ac988) SHA1(11b628c85533a307298765641eb87c305bde64d1) )

	ROM_REGION( 0x2000000, REGION_USER4, ROMREGION_DISPOSE )  /* Video ROMs Part 2 */
        ROM_LOAD_VROM( "mpr-21042.34",  0x000002, 0x400000, CRC(1dab621d) SHA1(cf0e59be7b5a12146f5562e208009054074151cd) )
        ROM_LOAD_VROM( "mpr-21043.35",  0x000000, 0x400000, CRC(707015c8) SHA1(125ff08cc555a4c8d9863e7433fad7949230630d) )
        ROM_LOAD_VROM( "mpr-21044.36",  0x000006, 0x400000, CRC(776f9580) SHA1(0529532975d74da851a2fd1ce9810e218d751d5f) )
        ROM_LOAD_VROM( "mpr-21045.37",  0x000004, 0x400000, CRC(a28ad02f) SHA1(8734568153dbf304193491e746b19a423a547f0d) )
        ROM_LOAD_VROM( "mpr-21046.38",  0x00000a, 0x400000, CRC(05c995ae) SHA1(d96391360692d30c456324dcd51511bf095a58cb) )
        ROM_LOAD_VROM( "mpr-21047.39",  0x000008, 0x400000, CRC(06b7826f) SHA1(cfdeb56964bd31196fde01b1f5cc294c8b49c215) )
        ROM_LOAD_VROM( "mpr-21048.40",  0x00000e, 0x400000, CRC(96849974) SHA1(347e2216ea1225eda92693dcd80eb97df88caabf) )
        ROM_LOAD_VROM( "mpr-21049.41",  0x00000c, 0x400000, CRC(91e8161a) SHA1(1edc0bc856e5d72f714bd0544814727f4ff12e7a) )

	ROM_REGION( 0x80000, REGION_CPU2, 0 )	/* 68000 code */
        ROM_LOAD16_WORD_SWAP( "epr-21066.21",  0x000000, 0x080000, CRC(f7ed2582) SHA1(a4f80d5f82c86f0bdb74bcda5dc69b83b475c542) )

	ROM_REGION( 0x1000000, REGION_SOUND1, 0 )	/* SCSP samples */
        ROM_LOAD( "mpr-21031.22",  0x000000, 0x400000, CRC(32f6b23a) SHA1(8cd092733b85aecf607c2f4b683c42e388a70906) )
        ROM_LOAD( "mpr-21033.24",  0x400000, 0x400000, CRC(253d3c70) SHA1(bfbc42d08cf46d89c87505f53e31b8a53e8a729a) )
        ROM_LOAD( "mpr-21032.23",  0x800000, 0x400000, CRC(3d3ff407) SHA1(5e298e24cb3050f8683658cef41ce59948e79166) )

	ROM_REGION( 0x20000, REGION_CPU3, 0 )	/* Z80 code */
		ROM_FILL( 0x000000, 0x20000, 0 )

	ROM_REGION( 0x800000, REGION_SOUND2, 0 )	/* DSB samples */
		ROM_FILL( 0x000000, 0x800000, 0 )
ROM_END

ROM_START( daytona2 )	/* Step 2.1 */
	ROM_REGION64_BE( 0x8800000, REGION_USER1, 0 ) /* program + data ROMs */
	// CROM
        ROM_LOAD64_WORD_SWAP( "epr-20861a.17", 0x000006, 0x200000, CRC(89ba8e78) SHA1(7d27124b976a63fdadd16551a664b2cc8cc08e79) )
        ROM_LOAD64_WORD_SWAP( "epr-20862a.18", 0x000004, 0x200000, CRC(e1b2ca61) SHA1(a5aa3416554b9d62469af3fefa9c2bacd69b4707) )
        ROM_LOAD64_WORD_SWAP( "epr-20863a.19", 0x000002, 0x200000, CRC(1deb4686) SHA1(dd6fcc95afa36148089b766c24c53a2af4d392d4) )
        ROM_LOAD64_WORD_SWAP( "epr-20864a.20", 0x000000, 0x200000, CRC(5250f3a8) SHA1(d75b86cb320b854fcf53e1b76331f87806b9ae84) )

	/* IC1-8 should be 64MBit */

	// CROM0
        ROM_LOAD64_WORD_SWAP( "mpr-20845.1",   0x800006, 0x400000, BAD_DUMP CRC(ab70d79e) SHA1(938c492f1ea7208d27dd0ea44c4d826625e652d8) )
        ROM_LOAD64_WORD_SWAP( "mpr-20846.2",   0x800004, 0x400000, BAD_DUMP CRC(aabf4392) SHA1(22d10c3e7896ff6e5cbbfd38e85ffa3daec895c0) )
        ROM_LOAD64_WORD_SWAP( "mpr-20847.3",   0x800002, 0x400000, BAD_DUMP CRC(96f3e97a) SHA1(4f6a4dd7d1eecbad65dc10c2f47db4bb949b0dfc) )
        ROM_LOAD64_WORD_SWAP( "mpr-20848.4",   0x800000, 0x400000, BAD_DUMP CRC(7a22a63b) SHA1(00c0b7c179f02936847d9e3fc2507d822efc7202) )

	// CROM1
        ROM_LOAD64_WORD_SWAP( "mpr-20849.5",  0x2800006, 0x400000, BAD_DUMP CRC(3c3cc882) SHA1(2e34b999155e3bac7978428abaf77be9fedcc306) )
        ROM_LOAD64_WORD_SWAP( "mpr-20850.6",  0x2800004, 0x400000, BAD_DUMP CRC(aa71a7a6) SHA1(27f160c6ecdc4dc99198879976ad1ddc46dd28ea) )
        ROM_LOAD64_WORD_SWAP( "mpr-20851.7",  0x2800002, 0x400000, BAD_DUMP CRC(ba128235) SHA1(e307addb5c172c248c0472574148d5b54f1e0b78) )
        ROM_LOAD64_WORD_SWAP( "mpr-20852.8",  0x2800000, 0x400000, BAD_DUMP CRC(e1000d4d) SHA1(5a09106616a7c116ee770025d1eedff8a0cda5ca) )

	// CROM2
        ROM_LOAD64_WORD_SWAP( "mpr-20853.9",  0x4800006, 0x400000, CRC(3245ee68) SHA1(f957df447bab0559076bc8a14eeaa080040acb85) )
        ROM_LOAD64_WORD_SWAP( "mpr-20854.10", 0x4800004, 0x400000, CRC(68d94cdf) SHA1(298236225ab5d0265ec615e71d084c79932d0438) )
        ROM_LOAD64_WORD_SWAP( "mpr-20855.11", 0x4800002, 0x400000, CRC(f1ff0794) SHA1(22782a01de0b699e7663aeb659169581830e69fd) )
        ROM_LOAD64_WORD_SWAP( "mpr-20856.12", 0x4800000, 0x400000, CRC(0367a242) SHA1(e3bb0ad7abddd8e81e4052dae169dfa245acbfa8) )

	// CROM3
        ROM_LOAD64_WORD_SWAP( "mpr-20857.13", 0x6800006, 0x400000, CRC(1eab9c62) SHA1(7c5cbab2761609cdf6ab20c2e198135fa9ae1067) )
        ROM_LOAD64_WORD_SWAP( "mpr-20858.14", 0x6800004, 0x400000, CRC(407fbad5) SHA1(594d45311d1daf00773b466c64ece4975df4dd5a) )
        ROM_LOAD64_WORD_SWAP( "mpr-20859.15", 0x6800002, 0x400000, CRC(e14f5c46) SHA1(d80ccd2c5ea1f34280241aaa5c947397b0344820) )
        ROM_LOAD64_WORD_SWAP( "mpr-20860.16", 0x6800000, 0x400000, CRC(e5ce2939) SHA1(7e5626cfb68402de80a05e4c3b18bdfd8bb40f85) )

	ROM_REGION( 0x2000000, REGION_USER3, ROMREGION_DISPOSE )  /* Video ROMs Part 1 */
        ROM_LOAD_VROM( "mpr-20870.26",  0x000002, 0x400000, CRC(7c9e573d) SHA1(0c488dcca8946b846a179dd66ca09d9eabedc21c) )
        ROM_LOAD_VROM( "mpr-20871.27",  0x000000, 0x400000, CRC(47a1b789) SHA1(8f817fefcfe99c27b30bb3bd9276b0a0947e5992) )
        ROM_LOAD_VROM( "mpr-20872.28",  0x000006, 0x400000, CRC(2f55b423) SHA1(4fa5b00715163b8f68a01627eb04dadbcbb6e89a) )
        ROM_LOAD_VROM( "mpr-20873.29",  0x000004, 0x400000, CRC(c9000e48) SHA1(a64a13e3116e36309ed37ec8651ef8a38b27b71f) )
        ROM_LOAD_VROM( "mpr-20874.30",  0x00000a, 0x400000, CRC(26a9cca2) SHA1(e3b68daaef95e004217c6f7cf3c6d256d083b994) )
        ROM_LOAD_VROM( "mpr-20875.31",  0x000008, 0x400000, CRC(bfefd21e) SHA1(8422b16569e029fead5cfca1ab31a04179a0f4b2) )
        ROM_LOAD_VROM( "mpr-20876.32",  0x00000e, 0x400000, CRC(fa701b87) SHA1(4134da01818cd6776a3c3396ac4f9b9e815722f2) )
        ROM_LOAD_VROM( "mpr-20877.33",  0x00000c, 0x400000, CRC(2cd072f1) SHA1(97881da77abc1aee309de64b53a0da3d22c4112c) )

	ROM_REGION( 0x2000000, REGION_USER4, ROMREGION_DISPOSE )  /* Video ROMs Part 2 */
        ROM_LOAD_VROM( "mpr-20878.34",  0x000002, 0x400000, CRC(e6d5bc01) SHA1(d8d2e202830693ac2f32304c484276193eb550a3) )
        ROM_LOAD_VROM( "mpr-20879.35",  0x000000, 0x400000, CRC(f1d727ec) SHA1(aecd2096bee590e8ccab9710845e8f0675bd06b0) )
        ROM_LOAD_VROM( "mpr-20880.36",  0x000006, 0x400000, CRC(8b370602) SHA1(2c461ae94e0bd0fea623724b82e08238f947e26b) )
        ROM_LOAD_VROM( "mpr-20881.37",  0x000004, 0x400000, CRC(397322e7) SHA1(9d1c24a063e3e00ca4ee02162dc0fc6de48de2d7) )
        ROM_LOAD_VROM( "mpr-20882.38",  0x00000a, 0x400000, CRC(9185be51) SHA1(b9f5ce29d9dda45733a649aa049124990f536fd9) )
        ROM_LOAD_VROM( "mpr-20883.39",  0x000008, 0x400000, CRC(d1e39e83) SHA1(8d8ccd7f23004e43ede71f73069979a3834bc010) )
        ROM_LOAD_VROM( "mpr-20884.40",  0x00000e, 0x400000, CRC(63c4639a) SHA1(d2b47f7bb8244e0a25c15d025d1bb295101f8875) )
        ROM_LOAD_VROM( "mpr-20885.41",  0x00000c, 0x400000, CRC(61c292ca) SHA1(a2c7e81a8a8ded8d0fd33ffea74a8d2cc8f22520) )

	ROM_REGION( 0x80000, REGION_CPU2, 0 )	/* 68000 code */
        ROM_LOAD16_WORD_SWAP( "epr-20865.21",  0x000000, 0x020000, CRC(b70c2699) SHA1(9ec3f59eda18c03530a5ab7a54c09c1e14cb1c4d) )

	ROM_REGION( 0x1000000, REGION_SOUND1, 0 )	/* SCSP samples */
        ROM_LOAD( "mpr-20866.22",  0x000000, 0x400000, CRC(91f40c1c) SHA1(0c9bee8e9a8bb5ffb2699922204ec26b489eee84) )
        ROM_LOAD( "mpr-20868.24",  0x400000, 0x400000, CRC(fa0c7ec0) SHA1(e3570318b67b9e9819830ad73529a627ac2f2821) )
        ROM_LOAD( "mpr-20867.23",  0x800000, 0x400000, CRC(a579c884) SHA1(ffa626381b1b2c0b963f8f0bad508c052364e657) )
        ROM_LOAD( "mpr-20869.25",  0xc00000, 0x400000, CRC(1f338832) SHA1(77160ea4d336aa88725b868c0035a267e92030b3) )

	ROM_REGION( 0x20000, REGION_CPU3, 0 )	/* Z80 code */
        ROM_LOAD( "epr-20886.ic2", 0x000000, 0x020000, CRC(65b05f98) SHA1(b83a2a6e7ec3d2fcd34ce701ffa66d99f6feb86d) )

	ROM_REGION( 0x1000000, REGION_SOUND2, 0 )	/* DSB samples */
        ROM_LOAD( "mpr-20887.ic18", 0x000000, 0x400000, CRC(a0757684) SHA1(9222527d90b3f462846a4fff95af83011871c277) )
        ROM_LOAD( "mpr-20888.ic20", 0x400000, 0x400000, CRC(b495fe65) SHA1(1573ced683534bb279a6dd69f6460745b728eac0) )
        ROM_LOAD( "mpr-20889.ic22", 0x800000, 0x400000, CRC(18eec79e) SHA1(341982d89952ed85c921c627c294609bf83ec44b) )
        ROM_LOAD( "mpr-20890.ic24", 0xc00000, 0x400000, CRC(aac96fa2) SHA1(bc68cd48eae50d3558d3c5a0302a3930639e3019) )
ROM_END

ROM_START( dayto2pe )	/* Step 2.1 */
	ROM_REGION64_BE( 0x8800000, REGION_USER1, 0 ) /* program + data ROMs */
	// CROM
        ROM_LOAD64_WORD_SWAP( "epr-21178.17", 0x000006, 0x200000, CRC(230bf8ac) SHA1(bc64c4f8a794ca59b5c488a34d1b5a2b67af8fec) )
        ROM_LOAD64_WORD_SWAP( "epr-21179.18", 0x000004, 0x200000, CRC(d5ffb4d6) SHA1(e1160bfcc180667463815b64e1815be38b6ef49d) )
        ROM_LOAD64_WORD_SWAP( "epr-21180.19", 0x000002, 0x200000, CRC(6e7b98ed) SHA1(67b0153dbc67c5df9231a968fd8a9714a1bb3f67) )
        ROM_LOAD64_WORD_SWAP( "epr-21181.20", 0x000000, 0x200000, CRC(bf0007ed) SHA1(4e044313e7abbd4d68c1c78ad9464688d7614590) )

	// CROM0
        ROM_LOAD64_WORD_SWAP( "mpr-21182.1",   0x800006, 0x400000, CRC(ba8e667f) SHA1(649f273cc612b014d9b04d301da018d248a73839) )
        ROM_LOAD64_WORD_SWAP( "mpr-21183.2",   0x800004, 0x400000, CRC(b4b44805) SHA1(8db3700207ec530ee3a83431fc891d7ab3d3bfec) )
        ROM_LOAD64_WORD_SWAP( "mpr-21184.3",   0x800002, 0x400000, CRC(25616403) SHA1(6607734479806a8812bff4d6a697f6a024156142) )
        ROM_LOAD64_WORD_SWAP( "mpr-21185.4",   0x800000, 0x400000, CRC(b6d5d2a1) SHA1(8e6548d65d926934c5ee3ee3669d1ce2a880bc82) )

	// CROM1
        ROM_LOAD64_WORD_SWAP( "mpr-21186.5",  0x2800006, 0x400000, CRC(a6128662) SHA1(52514e8ad978688e812a2a7d0fb54817814f2729) )
        ROM_LOAD64_WORD_SWAP( "mpr-21187.6",  0x2800004, 0x400000, CRC(3bd14ee6) SHA1(55d403c38439d9d489ce94bc34017ea43df27e91) )
        ROM_LOAD64_WORD_SWAP( "mpr-21188.7",  0x2800002, 0x400000, CRC(753fc2a5) SHA1(1d4bd2b971fb0b7897109f078e260eaae437a78c) )
        ROM_LOAD64_WORD_SWAP( "mpr-21189.8",  0x2800000, 0x400000, CRC(cb439c45) SHA1(a6326ae9cbbaa7235d7c9e45fa4dbc7267b681c9) )

	// CROM2
        ROM_LOAD64_WORD_SWAP( "mpr-21190.9",  0x4800006, 0x400000, CRC(984d56eb) SHA1(a04beeeadc8a11730b41ae66607777be3a7b1b5e) )
        ROM_LOAD64_WORD_SWAP( "mpr-21191.10", 0x4800004, 0x400000, CRC(a2bdcfe0) SHA1(c72896d0338a98efe83e70284c49b966d679e7ea) )
        ROM_LOAD64_WORD_SWAP( "mpr-21192.11", 0x4800002, 0x400000, CRC(60cbb1fa) SHA1(3b1d2da551a287efdc97e46ffeed56cf7e967bd5) )
        ROM_LOAD64_WORD_SWAP( "mpr-21193.12", 0x4800000, 0x400000, CRC(4638fef4) SHA1(0c880119c642c96b5f88eb024575f320d099d988) )

	// CROM3
        ROM_LOAD64_WORD_SWAP( "mpr-21194.13", 0x6800006, 0x400000, CRC(12c7a414) SHA1(3069ce4e50a82e243fa0c794faeafd735625df5c) )
        ROM_LOAD64_WORD_SWAP( "mpr-21195.14", 0x6800004, 0x400000, CRC(7f39761c) SHA1(e861e3c622bdc44510bd0ca7e2ef1284e4550d2e) )
        ROM_LOAD64_WORD_SWAP( "mpr-21196.15", 0x6800002, 0x400000, CRC(0ab46db5) SHA1(8c7ab959c677e6b8f1d1bc79cd8e04299aab56aa) )
        ROM_LOAD64_WORD_SWAP( "mpr-21197.16", 0x6800000, 0x400000, CRC(04015247) SHA1(a8b3d1f9572367e1c84475035ae0c6bb7726f01d) )

	ROM_REGION( 0x2000000, REGION_USER3, ROMREGION_DISPOSE )  /* Video ROMs Part 1 */
        ROM_LOAD_VROM( "mpr-21198.26",  0x000002, 0x400000, CRC(42ec9ed4) SHA1(bdc73050a5e51032478df77a02f7cd4b4ed9d337) )
        ROM_LOAD_VROM( "mpr-21199.27",  0x000000, 0x400000, CRC(fa28088c) SHA1(851f4fc5baa596f78bed21b0fe0e756865c19a9b) )
        ROM_LOAD_VROM( "mpr-21200.28",  0x000006, 0x400000, CRC(fbb5aa1d) SHA1(04b25e8fb8783e785fd471ffcf8330ca9f2e3ae7) )
        ROM_LOAD_VROM( "mpr-21201.29",  0x000004, 0x400000, CRC(e6b13469) SHA1(4968ab49aba86d6adcfb6200a5a02a410bbbd98c) )
        ROM_LOAD_VROM( "mpr-21202.30",  0x00000a, 0x400000, CRC(e6b4c2be) SHA1(d3209101793f44d4ece327f24cd3a8a52bfc8298) )
        ROM_LOAD_VROM( "mpr-21203.31",  0x000008, 0x400000, CRC(32d08d33) SHA1(dd9a3ac5a8306b861e32bbbfad2d4c5a7790c45d) )
        ROM_LOAD_VROM( "mpr-21204.32",  0x00000e, 0x400000, CRC(ef18fe0a) SHA1(81c8c87537eef6266b5ab464ba84cd12594e64bd) )
        ROM_LOAD_VROM( "mpr-21205.33",  0x00000c, 0x400000, CRC(4687bea6) SHA1(6633ad8ea96f8b3a89f49dbe52f92906be504ba1) )

	ROM_REGION( 0x2000000, REGION_USER4, ROMREGION_DISPOSE )  /* Video ROMs Part 2 */
        ROM_LOAD_VROM( "mpr-21206.34",  0x000002, 0x400000, CRC(ec2d6884) SHA1(8ec53005830902ebeb4f0ee36c5b2b73c2df5ebd) )
        ROM_LOAD_VROM( "mpr-21207.35",  0x000000, 0x400000, CRC(eeaa510b) SHA1(5f1b9e1d09f4866289ac4ad718cb90329a93353f) )
        ROM_LOAD_VROM( "mpr-21208.36",  0x000006, 0x400000, CRC(b222fef0) SHA1(88cf7ce17feef2da34a86217cced98a32051b22b) )
        ROM_LOAD_VROM( "mpr-21209.37",  0x000004, 0x400000, CRC(170a28ce) SHA1(ca0043d42e02f5913e4eb92686586704724a3e5e) )
        ROM_LOAD_VROM( "mpr-21210.38",  0x00000a, 0x400000, CRC(460cefe0) SHA1(820f3780e89173134792c62cc43f7c629bd555dd) )
        ROM_LOAD_VROM( "mpr-21211.39",  0x000008, 0x400000, CRC(c84759ce) SHA1(9b2de7fa26684457a61a2eeb0a69bd28125dd118) )
        ROM_LOAD_VROM( "mpr-21212.40",  0x00000e, 0x400000, CRC(6f8a75e0) SHA1(3b6cb238c29a6778be354882cd27b371ce96d332) )
        ROM_LOAD_VROM( "mpr-21213.41",  0x00000c, 0x400000, CRC(de75bec6) SHA1(f0d6f143ad227b221bf7f7c3d1b2edaa28ad4813) )

	ROM_REGION( 0x80000, REGION_CPU2, 0 )	/* 68000 code */
        ROM_LOAD16_WORD_SWAP( "epr-21325.21",  0x000000, 0x020000, CRC(004ad6ad) SHA1(3cedc58aaf40539325870c99ecedf51f161f4f4c) )

	ROM_REGION( 0x1000000, REGION_SOUND1, 0 )	/* SCSP samples */
        ROM_LOAD( "mpr-21285.22",  0x000000, 0x400000, CRC(7cdca6ac) SHA1(fda23365d5c40f4f9e810def914cd70418e5662e) )
        ROM_LOAD( "mpr-21287.24",  0x400000, 0x400000, CRC(06b66f17) SHA1(a7ce8f49d2db804429cedb5db53de8762b05845a) )
        ROM_LOAD( "mpr-21286.23",  0x800000, 0x400000, CRC(749dfef0) SHA1(43032b465f426188a6d718f22a11c6d9a79f7577) )
        ROM_LOAD( "mpr-21288.25",  0xc00000, 0x400000, CRC(14bee38e) SHA1(68300bf663ec3f597c73e7a39ca7057cf51a7a47) )

	ROM_REGION( 0x20000, REGION_CPU3, 0 )	/* Z80 code */
        ROM_LOAD( "epr-20886.ic2", 0x000000, 0x020000, CRC(65b05f98) SHA1(b83a2a6e7ec3d2fcd34ce701ffa66d99f6feb86d) )

	ROM_REGION( 0x1000000, REGION_SOUND2, 0 )	/* DSB samples */
        ROM_LOAD( "mpr-20887.ic18", 0x000000, 0x400000, CRC(a0757684) SHA1(9222527d90b3f462846a4fff95af83011871c277) )
        ROM_LOAD( "mpr-20888.ic20", 0x400000, 0x400000, CRC(b495fe65) SHA1(1573ced683534bb279a6dd69f6460745b728eac0) )
        ROM_LOAD( "mpr-20889.ic22", 0x800000, 0x400000, CRC(18eec79e) SHA1(341982d89952ed85c921c627c294609bf83ec44b) )
        ROM_LOAD( "mpr-20890.ic24", 0xc00000, 0x400000, CRC(aac96fa2) SHA1(bc68cd48eae50d3558d3c5a0302a3930639e3019) )
ROM_END

ROM_START( srally2 )	/* Step 2.0 */
	ROM_REGION64_BE( 0x4800000, REGION_USER1, 0 ) /* program + data ROMs */
	// CROM
        ROM_LOAD64_WORD_SWAP( "epr-20632.17",  0x000006, 0x200000, CRC(6829a801) SHA1(2aa3834f6a8c53f5db57ab52994b8ab3fde2d7c2) )
        ROM_LOAD64_WORD_SWAP( "epr-20633.18",  0x000004, 0x200000, CRC(f5a24f24) SHA1(6f741bc53d51ff4b5535dbee35aa490f159945ec) )
        ROM_LOAD64_WORD_SWAP( "epr-20634.19",  0x000002, 0x200000, CRC(45a09245) SHA1(1e7e844d38e9cba59c2a7e6f2e6ca2bba2c8b352) )
        ROM_LOAD64_WORD_SWAP( "epr-20635.20",  0x000000, 0x200000, CRC(7937473f) SHA1(d5bb57a08019a4523f976c418526efffc6ef988b) )

	// CROM0
        ROM_LOAD64_WORD_SWAP( "mpr-20602.1",   0x800006, 0x400000, CRC(60cfa72a) SHA1(2cb9e16d50979461d39e0493273ee38d23900b45) )
        ROM_LOAD64_WORD_SWAP( "mpr-20603.2",   0x800004, 0x400000, CRC(ad0d8eb8) SHA1(5bcd0a72b2d48d1834f296b7b37a56ae13c461fa) )
        ROM_LOAD64_WORD_SWAP( "mpr-20604.3",   0x800002, 0x400000, CRC(99c5f396) SHA1(dc586591a684bde224704f8356640343eb7651b3) )
        ROM_LOAD64_WORD_SWAP( "mpr-20605.4",   0x800000, 0x400000, CRC(00513401) SHA1(c6ac665cea999dcf483e9837daefd7fcdd755043) )

	// CROM1
        ROM_LOAD64_WORD_SWAP( "mpr-20606.5",  0x1800006, 0x400000, CRC(072498fd) SHA1(f7804a7c2139901367b06823bf0a6472e24078a6) )
        ROM_LOAD64_WORD_SWAP( "mpr-20607.6",  0x1800004, 0x400000, CRC(6da85aa3) SHA1(04fce70fa69aca615e5b009be99f7a657070b0c0) )
        ROM_LOAD64_WORD_SWAP( "mpr-20608.7",  0x1800002, 0x400000, CRC(0c9b0571) SHA1(2e13ab15d7fb6104316dc86770172269c8280306) )
        ROM_LOAD64_WORD_SWAP( "mpr-20609.8",  0x1800000, 0x400000, CRC(c03cc0e5) SHA1(8ee99dd37587dd1aa24d746ef14180515306e5d9) )

	// CROM2
        ROM_LOAD64_WORD_SWAP( "mpr-20610.9",  0x2800006, 0x400000, CRC(b6e0ff4e) SHA1(9b6ab7925b64b34f0d16ff0ba72dd0a0e6d23290) )
        ROM_LOAD64_WORD_SWAP( "mpr-20611.10", 0x2800004, 0x400000, CRC(5d9f8ba2) SHA1(42bec0ca6cf8d0192183dd08c4a92157c34d3651) )
        ROM_LOAD64_WORD_SWAP( "mpr-20612.11", 0x2800002, 0x400000, CRC(721a44b6) SHA1(151e4f9e2ed4095620c88bf54bf69f13f441e5ce) )
        ROM_LOAD64_WORD_SWAP( "mpr-20613.12", 0x2800000, 0x400000, CRC(2938c0d9) SHA1(95c444ef0b53afd129d512f8a8961938b0fd703e) )

	ROM_REGION( 0x2000000, REGION_USER3, ROMREGION_DISPOSE )  /* Video ROMs Part 1 */
        ROM_LOAD_VROM( "mpr-20616.26",  0x000002, 0x400000, CRC(e11dcf8b) SHA1(bbc384e95d52d883cbc68122f5a3c44dee98d54c) )
        ROM_LOAD_VROM( "mpr-20617.27",  0x000000, 0x400000, CRC(96acef3f) SHA1(a72ed7ac3f8132562c3d2e9bdf7b5c596082cb29) )
        ROM_LOAD_VROM( "mpr-20618.28",  0x000006, 0x400000, CRC(6c281281) SHA1(ddf056386e7e44f2d2d034f405c55bc5a3109848) )
        ROM_LOAD_VROM( "mpr-20619.29",  0x000004, 0x400000, CRC(0fa65819) SHA1(b9ce62a149f019b04eb72e0d6c9339472f403823) )
        ROM_LOAD_VROM( "mpr-20620.30",  0x00000a, 0x400000, CRC(ee79585f) SHA1(ea1797667a6ef6e38cf48543a63fd69a097b1e50) )
        ROM_LOAD_VROM( "mpr-20621.31",  0x000008, 0x400000, CRC(3a99148f) SHA1(0a2e5b4af85a4ce370c8defeb8d81def1279e693) )
        ROM_LOAD_VROM( "mpr-20622.32",  0x00000e, 0x400000, CRC(0618f056) SHA1(873063ab147b6b513683d4b4fe6a874f920ba818) )
        ROM_LOAD_VROM( "mpr-20623.33",  0x00000c, 0x400000, CRC(ccf31b85) SHA1(b25ea13706734a2e5c2f7587f494d59b6ad43c1f) )

	ROM_REGION( 0x2000000, REGION_USER4, ROMREGION_DISPOSE )  /* Video ROMs Part 2 */
        ROM_LOAD_VROM( "mpr-20624.34",  0x000002, 0x400000, CRC(90f30936) SHA1(43be3fe5a4200ab4200a49d571d9114e894db700) )
        ROM_LOAD_VROM( "mpr-20625.35",  0x000000, 0x400000, CRC(04f804fa) SHA1(6537733d6a0c12e084937366bbdcc1b3bea3b4aa) )
        ROM_LOAD_VROM( "mpr-20626.36",  0x000006, 0x400000, CRC(2d6c97d6) SHA1(201d01556e8381f21670b9637eb479e8f88e880e) )
        ROM_LOAD_VROM( "mpr-20627.37",  0x000004, 0x400000, CRC(a14ee871) SHA1(c846e39760570703ec97ae085992b2de0daef458) )
        ROM_LOAD_VROM( "mpr-20628.38",  0x00000a, 0x400000, CRC(bba829a3) SHA1(ad44d53e9cf5bc1f6a03fe961d5f13410261d912) )
        ROM_LOAD_VROM( "mpr-20629.39",  0x000008, 0x400000, CRC(ead2eb31) SHA1(cfb2ff20d5fcdaa9e0b075f84e94d07ff069d17e) )
        ROM_LOAD_VROM( "mpr-20630.40",  0x00000e, 0x400000, CRC(cc5881b8) SHA1(6d9b973c442c5d3bb872624412b8dfbef0677b34) )
        ROM_LOAD_VROM( "mpr-20631.41",  0x00000c, 0x400000, CRC(5cb69ffd) SHA1(654d8187634726c8218bf84304765a40c2d4b117) )

	ROM_REGION( 0x80000, REGION_CPU2, 0 )	/* 68000 code */
        ROM_LOAD16_WORD_SWAP( "epr-20636.21", 0x000000, 0x080000, CRC(7139ebf8) SHA1(3e06e8aa5c3eaf371073caa51e5fc5b42826f015) )

	// Samples
	ROM_REGION( 0x800000, REGION_SOUND1, 0 )	/* SCSP samples */
        ROM_LOAD( "mpr-20614.22", 0x000000, 0x400000, CRC(a3930e4a) SHA1(6a34f5b7817db8304454235997eaa453528bc655) )
        ROM_LOAD( "mpr-20615.24", 0x400000, 0x400000, CRC(62e8a94a) SHA1(abed71b1c6eb2563fe58e6598c10dd266340e5e0) )

	ROM_REGION( 0x20000, REGION_CPU3, 0 )	/* Z80 code */
        ROM_LOAD( "epr-20641.2", 0x000000, 0x020000, CRC(c9b82035) SHA1(1e438f8104f79c2956bb1aeb710b01b6dc59101e) )

	ROM_REGION( 0x1000000, REGION_SOUND2, 0 )	/* DSB samples */
        ROM_LOAD( "mpr-20637.57", 0x000000, 0x400000, CRC(d66e8a02) SHA1(f5d2bf4c97139fa56d14ffe2885a86e8f17ee965) )
        ROM_LOAD( "mpr-20638.58", 0x400000, 0x400000, CRC(d1513382) SHA1(b4d5b7680e2e73b361530d689ffdb0bab62e9ee4) )
        ROM_LOAD( "mpr-20639.59", 0x800000, 0x400000, CRC(f6603b7b) SHA1(9f31a2562168e5eba51864935e1c15db4e3114fb) )
        ROM_LOAD( "mpr-20640.60", 0xc00000, 0x400000, CRC(9eea07b7) SHA1(bdcf136f29e1435c9d82718730ef209d8cfe74d8) )
ROM_END

ROM_START( sraly2dx )	/* Step 2.0 */
	ROM_REGION64_BE( 0x4800000, REGION_USER1, 0 ) /* program + data ROMs */
	// CROM
        ROM_LOAD64_WORD_SWAP( "epr-20502.17",  0x000006, 0x200000, CRC(af16846d) SHA1(a0babc4dc3809ca1e71eaad4dc2f8c1597575e8b) )
        ROM_LOAD64_WORD_SWAP( "epr-20503.18",  0x000004, 0x200000, CRC(6e238b3d) SHA1(78da9abf39a2371d74d6b72b00f2467dfe86c4d5) )
        ROM_LOAD64_WORD_SWAP( "epr-20504.19",  0x000002, 0x200000, CRC(30bbc46d) SHA1(e6e2c76886cc740d009b4d7ac4412c0591caf34b) )
        ROM_LOAD64_WORD_SWAP( "epr-20505.20",  0x000000, 0x200000, CRC(c24a5097) SHA1(93ba7bd98333c8e7f2dd067ac10d4a231a38fa84) )

	// CROM0
        ROM_LOAD64_WORD_SWAP( "mpr-20472.1",   0x800006, 0x400000, CRC(db8d6a00) SHA1(67b6206361edd5714aad9dc54c43d79d8e649c7e) )
        ROM_LOAD64_WORD_SWAP( "mpr-20473.2",   0x800004, 0x400000, CRC(dd8e3131) SHA1(402d50fddfd2a5e7691bbccf309ce8522e3afea9) )
        ROM_LOAD64_WORD_SWAP( "mpr-20474.3",   0x800002, 0x400000, CRC(66cb4c8e) SHA1(21b47691011643b7560a3bc55b38eb559f164376) )
        ROM_LOAD64_WORD_SWAP( "mpr-20475.4",   0x800000, 0x400000, CRC(d0f059ee) SHA1(d6c15419f60306f11b0ff19ac0ee8c0052ac0b67) )

	// CROM1
        ROM_LOAD64_WORD_SWAP( "mpr-20476.5",  0x1800006, 0x400000, CRC(cc97d758) SHA1(3b9a83f1837a1b64ba70dfe4707edc738b489543) )
        ROM_LOAD64_WORD_SWAP( "mpr-20477.6",  0x1800004, 0x400000, CRC(0b5ac3ad) SHA1(87a8a983b3d020388240634a4598a8eae9896d3a) )
        ROM_LOAD64_WORD_SWAP( "mpr-20478.7",  0x1800002, 0x400000, CRC(5dfd59f7) SHA1(dd01fcac97cee9f2a7216f3f9b3135e60d8c4704) )
        ROM_LOAD64_WORD_SWAP( "mpr-20479.8",  0x1800000, 0x400000, CRC(82ec5488) SHA1(b126aece3dc841bf09a9c1e450d1f49ce6337b71) )

	// CROM2
        ROM_LOAD64_WORD_SWAP( "mpr-20480.9",  0x2800006, 0x400000, CRC(1e486a2e) SHA1(1da0f035f78a8e06c38291b3073c0d98b4603e45) )
        ROM_LOAD64_WORD_SWAP( "mpr-20481.10", 0x2800004, 0x400000, CRC(42acc4f9) SHA1(064b80116b24946cf638d1e3f3d608fe97a60815) )
        ROM_LOAD64_WORD_SWAP( "mpr-20482.11", 0x2800002, 0x400000, CRC(d21668d1) SHA1(94113533607d388003c4dc43fde0fe4aea4d9589) )
        ROM_LOAD64_WORD_SWAP( "mpr-20483.12", 0x2800000, 0x400000, CRC(7d487f3a) SHA1(0ef22b8ff6f05acf97cd8f792788467c47c4a2f9) )

	ROM_REGION( 0x2000000, REGION_USER3, ROMREGION_DISPOSE )  /* Video ROMs Part 1 */
        ROM_LOAD_VROM( "mpr-20486.26",  0x000002, 0x400000, CRC(dab1f70f) SHA1(37083dfd7a91618afccf3a70740de296e6b542cb) )
        ROM_LOAD_VROM( "mpr-20487.27",  0x000000, 0x400000, CRC(ffb38774) SHA1(0b8cd5fa26e7b70bd8e2e93e3be702194d119e52) )
        ROM_LOAD_VROM( "mpr-20488.28",  0x000006, 0x400000, CRC(0c25a1fb) SHA1(9ed754d8210aad3ac76416b5e7bd55d8e2f0a440) )
        ROM_LOAD_VROM( "mpr-20489.29",  0x000004, 0x400000, CRC(6e8a911a) SHA1(1d9aa42c81eb18cfae64c6df91e6cffb1e8f52fd) )
        ROM_LOAD_VROM( "mpr-20490.30",  0x00000a, 0x400000, CRC(93da0363) SHA1(1cf69103991dee4527dec490663ae8a2526fa12f) )
        ROM_LOAD_VROM( "mpr-20491.31",  0x000008, 0x400000, CRC(c4808e7a) SHA1(aa6af8a2338aa716b04f9e84fb7ce14d55bdd3bd) )
        ROM_LOAD_VROM( "mpr-20492.32",  0x00000e, 0x400000, CRC(d1b27b2b) SHA1(23c7b0c1c427ad420fbe208851d2ed14f5fbb723) )
        ROM_LOAD_VROM( "mpr-20493.33",  0x00000c, 0x400000, CRC(e43cc6af) SHA1(26489c60c7fccf2145ca4489717a4e0b1243b5ce) )

	ROM_REGION( 0x2000000, REGION_USER4, ROMREGION_DISPOSE )  /* Video ROMs Part 2 */
        ROM_LOAD_VROM( "mpr-20494.34",  0x000002, 0x400000, CRC(b997b531) SHA1(eb4568701d9540aeb681a2355cbb1f3f4adfd651) )
        ROM_LOAD_VROM( "mpr-20495.35",  0x000000, 0x400000, CRC(72480f09) SHA1(30466c8f57cf9109c1aebb6dc2fb1c580eca7a3c) )
        ROM_LOAD_VROM( "mpr-20496.36",  0x000006, 0x400000, CRC(96f6d3a8) SHA1(8b18dbd20567e6fa6dd07487b36f6ebba74a04c5) )
        ROM_LOAD_VROM( "mpr-20497.37",  0x000004, 0x400000, CRC(7dc700a3) SHA1(3bbc5517151067946c2ecefa03c8806249fbc7ff) )
        ROM_LOAD_VROM( "mpr-20498.38",  0x00000a, 0x400000, CRC(4e844081) SHA1(1e3acf84b4c5a85ac26cb4e3f6d5a31433e4b1f7) )
        ROM_LOAD_VROM( "mpr-20499.39",  0x000008, 0x400000, CRC(09d9c7d1) SHA1(78c81254eb0babdfbfc84612cae5037fce82b7fe) )
        ROM_LOAD_VROM( "mpr-20500.40",  0x00000e, 0x400000, CRC(3766fd87) SHA1(941ff6d89dbc8e59cc7a9a677c329aadb9068e5d) )
        ROM_LOAD_VROM( "mpr-20501.41",  0x00000c, 0x400000, CRC(741da4ac) SHA1(fa6e52b42b927bc659f139f4dd039204bda3b224) )

	ROM_REGION( 0x80000, REGION_CPU2, 0 )	/* 68000 code */
        ROM_LOAD16_WORD_SWAP( "epr-20506.21", 0x000000, 0x080000, CRC(855af67b) SHA1(a0359b8329c9c0746bc996b9272b7a1f2db07368) )

	// Samples
	ROM_REGION( 0x800000, REGION_SOUND1, 0 )	/* SCSP samples */
        ROM_LOAD( "mpr-20484.22", 0x000000, 0x400000, CRC(8ac3fbc4) SHA1(8b7624506ff00256a745bb4b7393cf17a081faa4) )
        ROM_LOAD( "mpr-20485.24", 0x400000, 0x400000, CRC(cfd8c19b) SHA1(3b8cc045cb02b93f9d35b81a48085d4d480d6bff) )

	ROM_REGION( 0x20000, REGION_CPU3, 0 )	/* Z80 code */
		ROM_FILL( 0x000000, 0x20000, 0 )

	ROM_REGION( 0x1000000, REGION_SOUND2, 0 )	/* DSB samples */
		ROM_FILL( 0x000000, 0x1000000, 0 )
ROM_END

ROM_START( harley )	/* Step 2.0 */
	ROM_REGION64_BE( 0x4800000, REGION_USER1, 0 ) /* program + data ROMs */
	// CROM
        ROM_LOAD64_WORD_SWAP( "epr-20393a.17", 0x000006, 0x200000, CRC(b5646556) SHA1(4bff0e140e1d1df7459f7194aa4a335bc4592203) )
        ROM_LOAD64_WORD_SWAP( "epr-20394a.18", 0x000004, 0x200000, CRC(ce29e2b6) SHA1(482aaf5480b219b8ac6e4e36a6d64359e1834f44) )
        ROM_LOAD64_WORD_SWAP( "epr-20395a.19", 0x000002, 0x200000, CRC(761f4976) SHA1(3e451a99b87f05d6fa35efdd944cd4c2033dd5fd) )
        ROM_LOAD64_WORD_SWAP( "epr-20396a.20", 0x000000, 0x200000, CRC(16b0106b) SHA1(3b8449805b73ff6ff1a1c8b134cd861b77b7f8d8) )

	// CROM0
        ROM_LOAD64_WORD_SWAP( "mpr-20361.1",   0x800006, 0x400000, CRC(ddb66c2f) SHA1(e47450cc38af99e2870aac1f598e057a0c6efe47) )
        ROM_LOAD64_WORD_SWAP( "mpr-20362.2",   0x800004, 0x400000, CRC(f7e60dfd) SHA1(e87b1a35513d6c9392643e8e97e2bc3eccdf21cb) )
        ROM_LOAD64_WORD_SWAP( "mpr-20363.3",   0x800002, 0x400000, CRC(3e3cc6ff) SHA1(ae0640ac00b6a09010984197a1aeb43817576181) )
        ROM_LOAD64_WORD_SWAP( "mpr-20364.4",   0x800000, 0x400000, CRC(a2a68ef2) SHA1(4d6ddd85d3350e56276c7cb11dd428d2c13a0374) )

	// CROM1
        ROM_LOAD64_WORD_SWAP( "mpr-20365.5",  0x1800006, 0x400000, CRC(7dd50361) SHA1(731ed1b660ebdaf1c9d583b374181ddc1496322b) )
        ROM_LOAD64_WORD_SWAP( "mpr-20366.6",  0x1800004, 0x400000, CRC(45e3850e) SHA1(54e6a285c594418355ab80cdfa4f5881cf74e39a) )
        ROM_LOAD64_WORD_SWAP( "mpr-20367.7",  0x1800002, 0x400000, CRC(6c3f9748) SHA1(0bec4c32c7e5fe715bb4572bf54c559583637170) )
        ROM_LOAD64_WORD_SWAP( "mpr-20368.8",  0x1800000, 0x400000, CRC(100c9846) SHA1(63b2ab000b5e4996c68724e3f3457a50e6185295) )

	// CROM3
        ROM_LOAD64_WORD_SWAP( "epr-20409.13", 0x4000006, 0x200000, CRC(58caaa75) SHA1(ce27ff9109f4fd69191b6aec3298013261f657e5) )
        ROM_LOAD64_WORD_SWAP( "epr-20410.14", 0x4000004, 0x200000, CRC(98b126f2) SHA1(f102ce6fdcd5c7eebc2c802b80a3ee861fc50f19) )
        ROM_LOAD64_WORD_SWAP( "epr-20411.15", 0x4000002, 0x200000, CRC(848daaf7) SHA1(85486e8fbac4237a2fed31120cc15239d5037d9a) )
        ROM_LOAD64_WORD_SWAP( "epr-20412.16", 0x4000000, 0x200000, CRC(0d51bb34) SHA1(24ce007e1abdd26d10b3d2a294503bf70a48db2d) )

	ROM_REGION( 0x2000000, REGION_USER3, ROMREGION_DISPOSE )  /* Video ROMs Part 1 */
        ROM_LOAD_VROM( "mpr-20377.26",  0x000002, 0x400000, CRC(4d2887e5) SHA1(6c965262bbeb8c5a10cdcf5ee31f2f50616ad6f0) )
        ROM_LOAD_VROM( "mpr-20378.27",  0x000000, 0x400000, CRC(5ad7c0ec) SHA1(19eee9a512abf857e2fea40261f90d727f79961a) )
        ROM_LOAD_VROM( "mpr-20379.28",  0x000006, 0x400000, CRC(1e51c9f0) SHA1(e62f6b4b5378f2724cd7ae9a0c87a9de31d0ff37) )
        ROM_LOAD_VROM( "mpr-20380.29",  0x000004, 0x400000, CRC(e10d35ae) SHA1(d99615b9d6b6c89cb1f9bfeba4f3aebefbaf857c) )
        ROM_LOAD_VROM( "mpr-20381.30",  0x00000a, 0x400000, CRC(76cd36a2) SHA1(026dfe1c996a923b8114defb96a3b200e2c72279) )
        ROM_LOAD_VROM( "mpr-20382.31",  0x000008, 0x400000, CRC(f089ae37) SHA1(d4366617fc956b2ad653409981227f238314c7eb) )
        ROM_LOAD_VROM( "mpr-20383.32",  0x00000e, 0x400000, CRC(9e96d3be) SHA1(a674544eb5e1792f82d2b58ab26e9be2cf3a4e25) )
        ROM_LOAD_VROM( "mpr-20384.33",  0x00000c, 0x400000, CRC(5bdfbb52) SHA1(31470f761170e489582db36124d7740d8dbb94aa) )

	ROM_REGION( 0x2000000, REGION_USER4, ROMREGION_DISPOSE )  /* Video ROMs Part 2 */
        ROM_LOAD_VROM( "mpr-20385.34",  0x000002, 0x400000, CRC(12db1729) SHA1(58fd946d8d63cf8b8da741a68d18c58f6894d661) )
        ROM_LOAD_VROM( "mpr-20386.35",  0x000000, 0x400000, CRC(db2ccaf8) SHA1(873fc1f293a50bb98e463449c8591d2cfc3cd93f) )
        ROM_LOAD_VROM( "mpr-20387.36",  0x000006, 0x400000, CRC(c5dde91b) SHA1(8155f49918cd57bc625fbc149ffdd9074f5906f0) )
        ROM_LOAD_VROM( "mpr-20388.37",  0x000004, 0x400000, CRC(aeaa862e) SHA1(e78a7e446df2f15570e1dc64a5f8b22ac09ac6a0) )
        ROM_LOAD_VROM( "mpr-20389.38",  0x00000a, 0x400000, CRC(49bb6593) SHA1(ffed9c3748405e4835998f644077cf8581bd00c7) )
        ROM_LOAD_VROM( "mpr-20390.39",  0x000008, 0x400000, CRC(1d4a8efe) SHA1(7a4515906a351737fb5b3a221bad839e61ea03a2) )
        ROM_LOAD_VROM( "mpr-20391.40",  0x00000e, 0x400000, CRC(5dc452dc) SHA1(203d5d8008bea8d2f43566fff6971b3f7add75bc) )
        ROM_LOAD_VROM( "mpr-20392.41",  0x00000c, 0x400000, CRC(892208cb) SHA1(12b5309f5f66d7c2165b285e0a9710ee0d9c99f4) )

	ROM_REGION( 0x80000, REGION_CPU2, 0 )	/* 68000 code */
        ROM_LOAD16_WORD_SWAP( "epr-20397.21",  0x000000, 0x080000, CRC(5b20b54a) SHA1(26fa5aedc6ccc37f2c0879e1a0f9fbac2331e12e) )

	// Samples
	ROM_REGION( 0x1000000, REGION_SOUND1, 0 )	/* SCSP samples */
        ROM_LOAD( "mpr-20373.22",  0x000000, 0x400000, CRC(c684e8a3) SHA1(2be1db24c3b221976cbcc6ad3d8cb7c6f4e3a13e) )
        ROM_LOAD( "mpr-20375.24",  0x400000, 0x400000, CRC(906ace86) SHA1(8edd2183d83897eda0578a34938b926672c21953) )
        ROM_LOAD( "mpr-20374.23",  0x800000, 0x400000, CRC(fcf6ea21) SHA1(9102323cf867f9a87fe362b78d8e1be8a2809fd3) )
        ROM_LOAD( "mpr-20376.25",  0xc00000, 0x400000, CRC(deeed366) SHA1(6d4809960c34865374d146605bb3e009394f7a8c) )

	ROM_REGION( 0x20000, REGION_CPU3, 0 )	/* Z80 code */
		ROM_FILL( 0x000000, 0x20000, 0 )

	ROM_REGION( 0x1000000, REGION_SOUND2, 0 )	/* DSB samples */
		ROM_FILL( 0x000000, 0x1000000, 0 )
ROM_END

ROM_START( fvipers2 )	/* Step 2.0 */
	ROM_REGION64_BE( 0x4800000, REGION_USER1, 0 ) /* program + data ROMs */
	// CROM
        ROM_LOAD64_WORD_SWAP( "epr-20596a.17", 0x000006, 0x200000, CRC(969ab801) SHA1(a7a2aa71204d1c38a6a8c0605331fd859cb224f1) )
        ROM_LOAD64_WORD_SWAP( "epr-20597a.18", 0x000004, 0x200000, CRC(6fcee322) SHA1(d65303f2551902ac5446a35656241628d67f4a63) )
        ROM_LOAD64_WORD_SWAP( "epr-20598a.19", 0x000002, 0x200000, CRC(87bd070f) SHA1(270840e828a59f74d41a852d26f0c44e74775100) )
        ROM_LOAD64_WORD_SWAP( "epr-20599a.20", 0x000000, 0x200000, CRC(9df02ab9) SHA1(772185e0cd8b481ac62fee0dd472a155126c1fac) )

	// CROM0
        ROM_LOAD64_WORD_SWAP( "mpr-20560.1",   0x800006, 0x400000, CRC(b0f6584d) SHA1(e776a47fae42e43189840418945d33cf23a1f5fe) )
        ROM_LOAD64_WORD_SWAP( "mpr-20561.2",   0x800004, 0x400000, CRC(38a0f112) SHA1(6ad9ee6a9f08ef379acce2360b3e861f574101f0) )
        ROM_LOAD64_WORD_SWAP( "mpr-20562.3",   0x800002, 0x400000, CRC(96e4942e) SHA1(2567671fe9d7e207634464d08e5789f4521ffed2) )
        ROM_LOAD64_WORD_SWAP( "mpr-20563.4",   0x800000, 0x400000, CRC(999848ac) SHA1(55b37ea0cfa7989871f88d89d9a13df5b1ec4e92) )

	// CROM1
        ROM_LOAD64_WORD_SWAP( "mpr-20564.5",  0x1800006, 0x400000, CRC(be69fca0) SHA1(8a0c4449b5742235f8e5eaf6d168e7512ed96fa6) )
        ROM_LOAD64_WORD_SWAP( "mpr-20565.6",  0x1800004, 0x400000, CRC(d6bbe638) SHA1(725f53b087c65d09e28ad3cec8634d5e7e63158a) )
        ROM_LOAD64_WORD_SWAP( "mpr-20566.7",  0x1800002, 0x400000, CRC(2901883b) SHA1(f086587578e4f4931da8a18ff53adf8b3d665a09) )
        ROM_LOAD64_WORD_SWAP( "mpr-20567.8",  0x1800000, 0x400000, CRC(80f4eba7) SHA1(ead30d57e3ca39dcd4195eeb2b4c6ee71c968769) )

	// CROM2
        ROM_LOAD64_WORD_SWAP( "mpr-20568.9",  0x2800006, 0x400000, CRC(ff23cf1c) SHA1(54b8a38c8deda1ddd0e26aa47504bfb0e2ed4b79) )
        ROM_LOAD64_WORD_SWAP( "mpr-20569.10", 0x2800004, 0x400000, CRC(136c014f) SHA1(c11b9b93ec6189ca745e6de9697bb274f07c6b1c) )
        ROM_LOAD64_WORD_SWAP( "mpr-20570.11", 0x2800002, 0x400000, CRC(2c0d91fc) SHA1(5ab896272da3650dbc482629ef7bdaccfa8eba54) )
        ROM_LOAD64_WORD_SWAP( "mpr-20571.12", 0x2800000, 0x400000, CRC(40b459af) SHA1(f2bca8783a787df45d4142436245d343a1a13fa7) )

	// CROM3
        ROM_LOAD64_WORD_SWAP( "mpr-20572.13", 0x3800006, 0x400000, CRC(d4a41a0b) SHA1(2ceb45e5ff22e2539d52a960713606742e11ed18) )
        ROM_LOAD64_WORD_SWAP( "mpr-20573.14", 0x3800004, 0x400000, CRC(e0dee793) SHA1(be80a5ca9d3cd910b15a4372883e047c2cc6b267) )
        ROM_LOAD64_WORD_SWAP( "mpr-20574.15", 0x3800002, 0x400000, CRC(68567771) SHA1(6183f588092d079c10e002adf641520183148143) )
        ROM_LOAD64_WORD_SWAP( "mpr-20575.16", 0x3800000, 0x400000, CRC(ebc99d8a) SHA1(66bd76738d76f42e32d3a27db4a8586dbb694f36) )

	ROM_REGION( 0x2000000, REGION_USER3, ROMREGION_DISPOSE )  /* Video ROMs Part 1 */
        ROM_LOAD_VROM( "mpr-20580.26",  0x000002, 0x400000, CRC(6d42775e) SHA1(9cd77ee6a317dcac67466920f59ac5cac98f67e2) )
        ROM_LOAD_VROM( "mpr-20581.27",  0x000000, 0x400000, CRC(ac9eec04) SHA1(0d4af895bc7cc100f9618c52e989aadbc0aa6d6e) )
        ROM_LOAD_VROM( "mpr-20582.28",  0x000006, 0x400000, CRC(b202f7bd) SHA1(6600f4bf49795c2821a2119f1251348782bcbdb2) )
        ROM_LOAD_VROM( "mpr-20583.29",  0x000004, 0x400000, CRC(0d6d508a) SHA1(bf7e419b097b0c90ee278766a3ee527beb860e71) )
        ROM_LOAD_VROM( "mpr-20584.30",  0x00000a, 0x400000, CRC(eccf4de6) SHA1(b5508aab6fd28f8b55aae495920382259b4c75e5) )
        ROM_LOAD_VROM( "mpr-20585.31",  0x000008, 0x400000, CRC(b383f4e5) SHA1(4a0d02de7fc41c66862917c35b0c2026ad06dfd0) )
        ROM_LOAD_VROM( "mpr-20586.32",  0x00000e, 0x400000, CRC(e7cd5dfb) SHA1(b33698792b3a1190712ad1c8c337f1f7fe67a1cb) )
        ROM_LOAD_VROM( "mpr-20587.33",  0x00000c, 0x400000, CRC(e2b2abe1) SHA1(32a6253948b852eb647a28438e8928684fc57b72) )

	ROM_REGION( 0x2000000, REGION_USER4, ROMREGION_DISPOSE )  /* Video ROMs Part 2 */
        ROM_LOAD_VROM( "mpr-20588.34",  0x000002, 0x400000, CRC(84f4162d) SHA1(efb7c632399616052e39f21dd399eba78563d83a) )
        ROM_LOAD_VROM( "mpr-20589.35",  0x000000, 0x400000, CRC(4e653d02) SHA1(120161ed82d882daa90b5637aa84714de7b26c95) )
        ROM_LOAD_VROM( "mpr-20590.36",  0x000006, 0x400000, CRC(527049be) SHA1(541bba0101baacfae3de7d998a5f29da61b2b956) )
        ROM_LOAD_VROM( "mpr-20591.37",  0x000004, 0x400000, CRC(3be20243) SHA1(83797b50f368843462dc46401940eda1a5c2acbd) )
        ROM_LOAD_VROM( "mpr-20592.38",  0x00000a, 0x400000, CRC(d7985b28) SHA1(a7a15757802d88e8ded363bad4d1d673bfc53a76) )
        ROM_LOAD_VROM( "mpr-20593.39",  0x000008, 0x400000, CRC(e670c4d3) SHA1(bc180de98757972b322f4f0d461a8c8d8094ac05) )
        ROM_LOAD_VROM( "mpr-20594.40",  0x00000e, 0x400000, CRC(35578240) SHA1(7902f41d376dd3449a17fb4d907e0bd84d70272b) )
        ROM_LOAD_VROM( "mpr-20595.41",  0x00000c, 0x400000, CRC(1d4a2cad) SHA1(0ee9eef0bb969e715a98ca1c212a02d413f36145) )

	ROM_REGION( 0x80000, REGION_CPU2, 0 )	/* 68000 code */
        ROM_LOAD16_WORD_SWAP( "epr-20600a.21",  0x000000, 0x080000, CRC(f0e7db7e) SHA1(980c1c6d3e8534c414d8b8016531e90f77e00f90) )

	// Samples
	/* Are these in the correct order ?? */
	ROM_REGION( 0x1000000, REGION_SOUND1, 0 )	/* SCSP samples */
        ROM_LOAD( "mpr-20576",  0x000000, 0x400000, CRC(1eeb540b) SHA1(f8de2ff302757aad38b61e2093e3783857d5d0fb) ) /* IC22 ?? */
        ROM_LOAD( "mpr-20578",  0x400000, 0x400000, CRC(d222f2d4) SHA1(f6128f8267e91242d7fc3d85beb2cd35124dd018) ) /* IC24 ?? */
        ROM_LOAD( "mpr-20577",  0x800000, 0x400000, CRC(3b236187) SHA1(0ba1513e0652f6686b306a4f600a565570b1ebcc) ) /* IC23 ?? */
        ROM_LOAD( "mpr-20579",  0xc00000, 0x400000, CRC(08788436) SHA1(6c9af2cf65e803882d6f4c0d57eb9e95cdeb5818) ) /* IC25 ?? */

	ROM_REGION( 0x20000, REGION_CPU3, 0 )	/* Z80 code */
		ROM_FILL( 0x000000, 0x20000, 0 )

	ROM_REGION( 0x1000000, REGION_SOUND2, 0 )	/* DSB samples */
		ROM_FILL( 0x000000, 0x1000000, 0 )
ROM_END

ROM_START( spikeout )	/* Step 2.1 */
	ROM_REGION64_BE( 0x8800000, REGION_USER1, 0 ) /* program + data ROMs */
	// CROM
        ROM_LOAD64_WORD_SWAP( "epr21214c.17", 0x000006, 0x200000, CRC(8dc0a85c) SHA1(c75088fd0594964a4ed78b80a2585d3d89c85464) )
        ROM_LOAD64_WORD_SWAP( "epr21215c.18", 0x000004, 0x200000, CRC(e2878221) SHA1(8c57efc942f6e091b7fbbed101c3a306b7368eed) )
        ROM_LOAD64_WORD_SWAP( "epr21216c.19", 0x000002, 0x200000, CRC(867d3a0f) SHA1(15ba12099eaa2cfd1d6cd985805df69c48441fb2) )
        ROM_LOAD64_WORD_SWAP( "epr21217c.20", 0x000000, 0x200000, CRC(ea8c30ce) SHA1(e076e32ba9134c63c7a5fb826eba4735774e3ab3) )

	/* IC1-8 should be 64MBit */

	// CROM0
        ROM_LOAD64_WORD_SWAP( "mpr21134.1",   0x800006, 0x400000, BAD_DUMP CRC(7bfe21a4) SHA1(accbb4272da313f4bd2b897f3f395f5523e7a3f4) )
        ROM_LOAD64_WORD_SWAP( "mpr21135.2",   0x800004, 0x400000, BAD_DUMP CRC(972f4b4f) SHA1(fa6b9d1ef3a213cedbe42705de015f6ae8c216b6) )
        ROM_LOAD64_WORD_SWAP( "mpr21136.3",   0x800002, 0x400000, BAD_DUMP CRC(5d563fba) SHA1(1f157b25e70438f1cfc04bbe8c83afa0e3858c2a) )
        ROM_LOAD64_WORD_SWAP( "mpr21137.4",   0x800000, 0x400000, BAD_DUMP CRC(707e5921) SHA1(a38ac947860d0cf7ad40b641135f1fb203e3df67) )

	// CROM1
        ROM_LOAD64_WORD_SWAP( "mpr21138.5",  0x2800006, 0x400000, BAD_DUMP CRC(48619c98) SHA1(34e0d10ea6c6a9dca3e1440e248ab0ec7bad0d8e) )
        ROM_LOAD64_WORD_SWAP( "mpr21139.6",  0x2800004, 0x400000, BAD_DUMP CRC(0425ba5d) SHA1(a2f347268279182e1be08a600d862adc49f3f3b4) )
        ROM_LOAD64_WORD_SWAP( "mpr21140.7",  0x2800002, 0x400000, BAD_DUMP CRC(df574950) SHA1(816f3c60f0cbcda3278e4acf99176af93e7c9b4b) )
        ROM_LOAD64_WORD_SWAP( "mpr21141.8",  0x2800000, 0x400000, BAD_DUMP CRC(d6a51bf1) SHA1(cb3196a7f9b378c755d4b29b01a97612d7d7a8d5) )

	// CROM2
	ROM_LOAD64_WORD_SWAP( "mpr21142.9",   0x4800006, 0x400000, CRC(da35cd51) SHA1(6b89c79a4e207d4bb3fe36e918f41d09915facaa) )
        ROM_LOAD64_WORD_SWAP( "mpr21143.10",  0x4800004, 0x400000, CRC(ddcada10) SHA1(72a4d7a1bcd7e6745b7076f2a7945e8c7c51e68a) )
        ROM_LOAD64_WORD_SWAP( "mpr21144.11",  0x4800002, 0x400000, CRC(d93d778c) SHA1(b6be5f37c1e40764f2831537603bd88afa872b27) )
        ROM_LOAD64_WORD_SWAP( "mpr21145.12",  0x4800000, 0x400000, CRC(0e6a3ae3) SHA1(718bad86461e592ce1d56bd3dc0090b83ed82355) )

	// CROM3
	ROM_LOAD64_WORD_SWAP( "mpr21146.13", 0x6800006, 0x400000, CRC(85f55311) SHA1(dc5529534e7702f05d48620fc9049289b7cbc9fb) )
        ROM_LOAD64_WORD_SWAP( "mpr21147.14", 0x6800004, 0x400000, CRC(a1f2b73f) SHA1(485bc11abd44826d00ac0c7e90cb72006cfa66e7) )
        ROM_LOAD64_WORD_SWAP( "mpr21148.15", 0x6800002, 0x400000, CRC(56d980ad) SHA1(f4175e97e68ca2c29ec6cb18d9ffd5ff170a6f6f) )
        ROM_LOAD64_WORD_SWAP( "mpr21149.16", 0x6800000, 0x400000, CRC(9e4ebe58) SHA1(130e2c09528d57522fec70fe93d2de55558ee87f) )

	ROM_REGION( 0x2000000, REGION_USER3, ROMREGION_DISPOSE )  /* Video ROMs Part 1 */
        ROM_LOAD_VROM( "mpr21154.26",  0x000002, 0x400000, CRC(3b76f8e8) SHA1(85773cdf1062ee0aed9b3cbc8a1a1b7dd2801afb) )
        ROM_LOAD_VROM( "mpr21155.27",  0x000000, 0x400000, CRC(aca19901) SHA1(d375defaa65e35aaf3f0f149bb1763f02d796c7a) )
        ROM_LOAD_VROM( "mpr21156.28",  0x000006, 0x400000, CRC(5c9df226) SHA1(00baf34ba2d4b57bf63b55386310ca6d6685fa96) )
        ROM_LOAD_VROM( "mpr21157.29",  0x000004, 0x400000, CRC(f6fb1279) SHA1(be61d8be27500c9af5f7dc6333fa37c667f337f8) )
        ROM_LOAD_VROM( "mpr21158.30",  0x00000a, 0x400000, CRC(61707554) SHA1(62b2de8ce88a361b272fde4998766f29e5beafc7) )
        ROM_LOAD_VROM( "mpr21159.31",  0x000008, 0x400000, CRC(fcc791f5) SHA1(03c66f65885f41766e860fdb5324226f4fb555f7) )
        ROM_LOAD_VROM( "mpr21160.32",  0x00000e, 0x400000, CRC(b40a38d3) SHA1(dec9590632b547a8eeb52528fa4f0b9f5f8bd769) )
        ROM_LOAD_VROM( "mpr21161.33",  0x00000c, 0x400000, CRC(559063f0) SHA1(f31a6d117550f1422bee76972c21bbdee6f3505d) )

	ROM_REGION( 0x2000000, REGION_USER4, ROMREGION_DISPOSE )  /* Video ROMs Part 2 */
        ROM_LOAD_VROM( "mpr21162.34",  0x000002, 0x400000, CRC(acc4b2e4) SHA1(b3ab137b0712c4088daeed1ac0fa2eaedca85940) )
        ROM_LOAD_VROM( "mpr21163.35",  0x000000, 0x400000, CRC(653c54c7) SHA1(cc5121215c3c511df03cfbddead06f6c09db849b) )
        ROM_LOAD_VROM( "mpr21164.36",  0x000006, 0x400000, CRC(902fd1e0) SHA1(930fc166beb87ca4801c66fb4c6e92e59050892b) )
        ROM_LOAD_VROM( "mpr21165.37",  0x000004, 0x400000, CRC(50b3be05) SHA1(016f5ba71f41b9cf358d22195b08319d86b95f78) )
        ROM_LOAD_VROM( "mpr21166.38",  0x00000a, 0x400000, CRC(8f87a782) SHA1(3631929f4d3dda71c3950a29a7d3135ecf4a9f57) )
        ROM_LOAD_VROM( "mpr21167.39",  0x000008, 0x400000, CRC(0f3994d0) SHA1(a5cf583c7ea4e05bd9ca493ca7ba9e61cdc803cd) )
        ROM_LOAD_VROM( "mpr21168.40",  0x00000e, 0x400000, CRC(c58be980) SHA1(77e3d5109c6659e1839b56ca2fdb157cdfba4b6a) )
        ROM_LOAD_VROM( "mpr21169.41",  0x00000c, 0x400000, CRC(aa3b2cc0) SHA1(1c804e602f9227a4e553cd1a5dfbfebd04930c61) )

	ROM_REGION( 0x80000, REGION_CPU2, 0 )	/* 68000 code */
        ROM_LOAD16_WORD_SWAP( "epr21218.21",  0x000000, 0x080000, CRC(5821001a) SHA1(f6bc416b77279670bc6c1c3a62f42faf9323387e) )

	// Samples
	ROM_REGION( 0x1000000, REGION_SOUND1, 0 )	/* SCSP samples */
        ROM_LOAD( "mpr21150.22",  0x000000, 0x400000, CRC(125201ce) SHA1(b6347042e1482561c6d468b05dfa3de261736485) )
        ROM_LOAD( "mpr21152.24",  0x400000, 0x400000, CRC(0afdee87) SHA1(2be20991a6d8fbee51dddc2dd0fbae7e43e5b3df) )
        ROM_LOAD( "mpr21151.23",  0x800000, 0x400000, CRC(599527b9) SHA1(bc124f916a72c85a1f2a10ababc4254adc951697) )
        ROM_LOAD( "mpr21153.25",  0xc00000, 0x400000, CRC(4155f307) SHA1(420a7b9d1a4aca9ff31ff7af7c8cea00963756af) )

	ROM_REGION( 0x20000, REGION_CPU3, 0 )	/* Z80 code */
	ROM_LOAD( "epr21219.ic2", 0x00000, 0x20000, CRC(4e042b21) SHA1(90937659702ddcda1bdbb623a38bf26c3b29f9d9) )

	ROM_REGION( 0x1000000, REGION_SOUND2, 0 )	/* DSB samples */
	ROM_LOAD( "mpr21170.ic18",  0x000000, 0x400000, CRC(f51f7ce3) SHA1(38b853b0545196e2c95822f572afb46a0a5d4c6c) )
        ROM_LOAD( "mpr21171.ic20",  0x400000, 0x400000, CRC(8d3bd5b6) SHA1(42167dd53e4562869382ec1c8a00b69d1fd4602a) )
        ROM_LOAD( "mpr21172.ic22",  0x800000, 0x400000, CRC(be221e27) SHA1(cf396d0145172a0492bf4203a7ff12c5c8480c0c) )
        ROM_LOAD( "mpr21173.ic24",  0xc00000, 0x400000, CRC(ca7226d6) SHA1(e15c6fb9dee91a42889cef350479b1964bf1e5df) )
ROM_END

ROM_START( spikeofe )	/* Step 2.1 */
	ROM_REGION64_BE( 0x8800000, REGION_USER1, 0 ) /* program + data ROMs */
	// CROM
        ROM_LOAD64_WORD_SWAP( "epr21653.17", 0x000006, 0x200000, CRC(f4bd9c3c) SHA1(de509c25226939d7a9c1b402ab6923844c12314e) )
        ROM_LOAD64_WORD_SWAP( "epr21654.18", 0x000004, 0x200000, CRC(5be245a3) SHA1(69cc84dd4ccc80b6453ace1944962b666e6fca3d) )
        ROM_LOAD64_WORD_SWAP( "epr21655.19", 0x000002, 0x200000, CRC(68a9e417) SHA1(8d5eb0d52eabde0d2cfb34ded4fc436f113cdcc0) )
        ROM_LOAD64_WORD_SWAP( "epr21656.20", 0x000000, 0x200000, CRC(bd2aaf64) SHA1(850c08f3e2fa269d94b4dc88c9d33854d5e38f21) )

	/* IC1-8 should be 64MBit */

	// CROM0
        ROM_LOAD64_WORD_SWAP( "mpr21613.1",   0x800006, 0x400000, BAD_DUMP CRC(39f181c0) SHA1(a0734b2aa5fde29ff7af4d29443f7e76f3323af1) )
        ROM_LOAD64_WORD_SWAP( "mpr21614.2",   0x800004, 0x400000, BAD_DUMP CRC(789a96e9) SHA1(6ac8908e9613cab1bf4b584c29ba6e2eec16b436) )
        ROM_LOAD64_WORD_SWAP( "mpr21615.3",   0x800002, 0x400000, BAD_DUMP CRC(50341c34) SHA1(155f6e73bad81f98817e635352c0972b5544e466) )
        ROM_LOAD64_WORD_SWAP( "mpr21616.4",   0x800000, 0x400000, BAD_DUMP CRC(d5980569) SHA1(22439b2ea5afdbb4cf20bfc5479f8e6c0664e4c5) )

	// CROM1
        ROM_LOAD64_WORD_SWAP( "mpr21617.5",  0x2800006, 0x400000, BAD_DUMP CRC(8306b52c) SHA1(4519e41ca621fa6cf4b8f880adbd89d10ef445dd) )
        ROM_LOAD64_WORD_SWAP( "mpr21618.6",  0x2800004, 0x400000, BAD_DUMP CRC(a9c0e6c4) SHA1(467b4ce1805d71a3b07b998816ce70ac8a3b678e) )
        ROM_LOAD64_WORD_SWAP( "mpr21619.7",  0x2800002, 0x400000, BAD_DUMP CRC(d2380847) SHA1(2f4860222da18fa560862e6fd730eef3551dfdce) )
        ROM_LOAD64_WORD_SWAP( "mpr21620.8",  0x2800000, 0x400000, BAD_DUMP CRC(3a1ecec4) SHA1(98eda2c0ad206aaf27c142198e8439424d585142) )

	// CROM2
	ROM_LOAD64_WORD_SWAP( "mpr21621.9",   0x4800006, 0x400000, CRC(eaa96639) SHA1(153307007debaf7e063d7599785736cbf53ec891) )
        ROM_LOAD64_WORD_SWAP( "mpr21622.10",  0x4800004, 0x400000, CRC(8483a0d0) SHA1(dff98e755112a3402a158b9d05b886dd9c0bffb0) )
        ROM_LOAD64_WORD_SWAP( "mpr21623.11",  0x4800002, 0x400000, CRC(8d37d83e) SHA1(6cff317b02215f6d9a16354e01767d978c0531c3) )
        ROM_LOAD64_WORD_SWAP( "mpr21624.12",  0x4800000, 0x400000, CRC(56994cc6) SHA1(f4be0e530226cf3df01d396e723bcb5db79e21d0) )

	// CROM3
        ROM_LOAD64_WORD_SWAP( "mpr21625.13", 0x6800006, 0x400000, CRC(72a34707) SHA1(107b9e1dfead1d04a59ba9fbe9800c703a1bca56) )
        ROM_LOAD64_WORD_SWAP( "mpr21626.14", 0x6800004, 0x400000, CRC(1861652e) SHA1(45793397523ae0c1af0b5b0746182f9193bb35df) )
        ROM_LOAD64_WORD_SWAP( "mpr21627.15", 0x6800002, 0x400000, CRC(efe94608) SHA1(1c04b1ea6517acee8b306ae52fadaf33341750bb) )
        ROM_LOAD64_WORD_SWAP( "mpr21628.16", 0x6800000, 0x400000, CRC(de3866ea) SHA1(43068f2af2b84ce2ee63e806cfc048e75c77a451) )

	ROM_REGION( 0x2000000, REGION_USER3, ROMREGION_DISPOSE )  /* Video ROMs Part 1 */
        ROM_LOAD_VROM( "mpr21633.26",  0x000002, 0x400000, CRC(735fb67d) SHA1(7f8d1880b0e5302f3cc7e49c3f0de1d8800bb6f8) )
        ROM_LOAD_VROM( "mpr21634.27",  0x000000, 0x400000, CRC(876e6788) SHA1(18f80b79e1da6bc6e6c1f1d368d0ff1ff6b1f468) )
        ROM_LOAD_VROM( "mpr21635.28",  0x000006, 0x400000, CRC(093534a8) SHA1(8ecbc3e4456f0453f8728d1ab7687b7d05d7957b) )
        ROM_LOAD_VROM( "mpr21636.29",  0x000004, 0x400000, CRC(2433f21c) SHA1(8c62cc42f6a4d795855ec75512a38e5a8a120f8e) )
        ROM_LOAD_VROM( "mpr21637.30",  0x00000a, 0x400000, CRC(093534a8) SHA1(8ecbc3e4456f0453f8728d1ab7687b7d05d7957b) )
        ROM_LOAD_VROM( "mpr21638.31",  0x000008, 0x400000, CRC(3773a215) SHA1(f36baf8e38bb1d853bc87b99db70efdcca217c93) )
        ROM_LOAD_VROM( "mpr21639.32",  0x00000e, 0x400000, CRC(313d1872) SHA1(9e74786fc17c38b7687be3abcd115e3b817991b1) )
        ROM_LOAD_VROM( "mpr21640.33",  0x00000c, 0x400000, CRC(271366be) SHA1(eab9ab8105f3d72ce797439c2c17f49aebbd8f07) )

	ROM_REGION( 0x2000000, REGION_USER4, ROMREGION_DISPOSE )  /* Video ROMs Part 2 */
        ROM_LOAD_VROM( "mpr21641.34",  0x000002, 0x400000, CRC(782147e4) SHA1(080f28daeee38c169b1bd5030800eca032a42da2) )
        ROM_LOAD_VROM( "mpr21642.35",  0x000000, 0x400000, CRC(844732c9) SHA1(20ee88b762eed38d1a6f16a0c1b375496187c676) )
        ROM_LOAD_VROM( "mpr21643.36",  0x000006, 0x400000, CRC(9e922e9d) SHA1(f66df4876672c0ee17525c4809d737523b04650d) )
        ROM_LOAD_VROM( "mpr21644.37",  0x000004, 0x400000, CRC(617aa65a) SHA1(eb856645a8e587207452ddc7e0e2bdc9b8642af5) )
        ROM_LOAD_VROM( "mpr21645.38",  0x00000a, 0x400000, CRC(71396f52) SHA1(ef0d3068104e663143b42b7fd5f4c65d01b12a6b) )
        ROM_LOAD_VROM( "mpr21646.39",  0x000008, 0x400000, CRC(90fd9c87) SHA1(75097e021b7b91502793e3e1833aa55d3adbb084) )
        ROM_LOAD_VROM( "mpr21647.40",  0x00000e, 0x400000, CRC(cf87991f) SHA1(8249e086d2ef5ded8a0544b0ddbd2328e3c64876) )
        ROM_LOAD_VROM( "mpr21648.41",  0x00000c, 0x400000, CRC(30f974a1) SHA1(03e2481b9af9083c3b97273819c0fcd5edc41bd4) )

	ROM_REGION( 0x80000, REGION_CPU2, 0 )	/* 68000 code */
        ROM_LOAD16_WORD_SWAP( "epr21657.21",  0x000000, 0x080000, CRC(7242e8fd) SHA1(9712972d821e2eca8db6666693340aca884f4393) )

	// Samples
	ROM_REGION( 0x1000000, REGION_SOUND1, 0 )	/* SCSP samples */
        ROM_LOAD( "mpr21629.22",  0x000000, 0x400000, CRC(bc9701c4) SHA1(df8b4deb16736461d5ae69a42404785f57a84d7f) )
        ROM_LOAD( "mpr21630.24",  0x400000, 0x400000, CRC(9f2deadd) SHA1(964dea6c8f5e34c5682cd7ca207a853aab80306e) )
        ROM_LOAD( "mpr21631.23",  0x800000, 0x400000, CRC(299036c5) SHA1(861f5d6579ee0fba1793140468194c2ef0fd0b7f) )
        ROM_LOAD( "mpr21632.25",  0xc00000, 0x400000, CRC(ff162f0d) SHA1(e62dcf68a4bfe8087d0ab508468fd016ace1f9c5) )

	ROM_REGION( 0x20000, REGION_CPU3, 0 )	/* Z80 code */
	ROM_LOAD( "epr21658.ic2", 0x00000, 0x20000, CRC(50bad8cb) SHA1(83947cbf8f074e6f15055917502a77f198123efe) )

	ROM_REGION( 0x1000000, REGION_SOUND2, 0 )	/* DSB samples */
	ROM_LOAD( "mpr21649.ic18",  0x000000, 0x400000, CRC(dac87f47) SHA1(429734ba3d97162e175a074249baf7c7a1aecdee) )
        ROM_LOAD( "mpr21650.ic20",  0x400000, 0x400000, CRC(86d90123) SHA1(1e90fb0242fbb032825684e763a5ebb329f63869) )
        ROM_LOAD( "mpr21651.ic22",  0x800000, 0x400000, CRC(81715565) SHA1(533deefed3565e7373a0aa2d043ea10e43e78b71) )
        ROM_LOAD( "mpr21652.ic24",  0xc00000, 0x400000, CRC(e7c8c9bf) SHA1(76d8fa89aed86fff4d1ba704aeef96fd2c326bc9) )
ROM_END

ROM_START( eca )	/* Step 2.1 */
	ROM_REGION64_BE( 0x4800000, REGION_USER1, 0 ) /* program + data ROMs */
	// CROM
        ROM_LOAD64_WORD_SWAP( "epr22895.17", 0x000006, 0x200000, CRC(07df16a0) SHA1(a9ad2b229854a5f4f761565141db738adde28720) )
        ROM_LOAD64_WORD_SWAP( "epr22896.18", 0x000004, 0x200000, CRC(0ff828a8) SHA1(2a74414891ceb5989e6ccb6e9d597f7d2e31fec4) )
        ROM_LOAD64_WORD_SWAP( "epr22897.19", 0x000002, 0x200000, CRC(9755dd8c) SHA1(41f27a303f4af179f17520d0c2d6a0aa4467aae8) )
        ROM_LOAD64_WORD_SWAP( "epr22898.20", 0x000000, 0x200000, CRC(efb96701) SHA1(168c7763740225f832728ab02b22185c9b7997c3) )

	// CROM0
        ROM_LOAD64_WORD_SWAP( "mpr22870.1",   0x800006, 0x400000, CRC(52054043) SHA1(f07c1f95a5847393c9e640c10cd14e2a3750b3ff) )
        ROM_LOAD64_WORD_SWAP( "mpr22871.2",   0x800004, 0x400000, CRC(cf5bb5b5) SHA1(fd056fcf9a48854faa357014c2cb4a6ed301c3a6) )
        ROM_LOAD64_WORD_SWAP( "mpr22872.3",   0x800002, 0x400000, CRC(4fde63a1) SHA1(72f88e514caea28eeee9e57e55c835c490465df0) )
        ROM_LOAD64_WORD_SWAP( "mpr22873.4",   0x800000, 0x400000, CRC(dd406330) SHA1(e17ed1814cb84820f19922eef27bbad65be1d355) )

	// CROM1
        ROM_LOAD64_WORD_SWAP( "mpr22874.5",  0x1800006, 0x400000, CRC(5e990497) SHA1(f0df52a19a11ef23f82e88eddbacdcb2a6357ded) )
        ROM_LOAD64_WORD_SWAP( "mpr22875.6",  0x1800004, 0x400000, CRC(1bb5c018) SHA1(5d0146c32f6a50613340ba0d2f5cc5430b595965) )
        ROM_LOAD64_WORD_SWAP( "mpr22876.7",  0x1800002, 0x400000, CRC(a7561249) SHA1(b4217cb088234831ca2e9486af849866790bf704) )
        ROM_LOAD64_WORD_SWAP( "mpr22877.8",  0x1800000, 0x400000, CRC(e53b8764) SHA1(03756ef0526aa0b56c2944336590918d8ff9a9b8) )

	// CROM3
        ROM_LOAD64_WORD_SWAP( "epr22882.13", 0x3800006, 0x400000, CRC(b161416f) SHA1(753810fa4db80a2d3a333a257dc92095a112d282) )
        ROM_LOAD64_WORD_SWAP( "epr22883.14", 0x3800004, 0x400000, CRC(86d90148) SHA1(4336efb9e3d62cc5e8073993c44cbe49141d987f) )
        ROM_LOAD64_WORD_SWAP( "epr22884.15", 0x3800002, 0x400000, CRC(254c3b63) SHA1(b5cd94ecbffb6f0da70fb2cb7541d5bfc96cb67c) )
        ROM_LOAD64_WORD_SWAP( "epr22885.16", 0x3800000, 0x400000, CRC(3525b46d) SHA1(d7914cd5c558b50b9115303f77bef63e04316b29) )

	ROM_REGION( 0x2000000, REGION_USER3, ROMREGION_DISPOSE )  /* Video ROMs Part 1 */
        ROM_LOAD_VROM( "mpr22854.26",  0x000002, 0x400000, CRC(97a23d16) SHA1(17a11a3e1b1806f7b994955ae22dd0e5333d47ea) )
        ROM_LOAD_VROM( "mpr22855.27",  0x000000, 0x400000, CRC(7249cdc9) SHA1(4bd07d911382cc569e09fb888858806d06809f9f) )
        ROM_LOAD_VROM( "mpr22856.28",  0x000006, 0x400000, CRC(9c0d1d1b) SHA1(f15c756fb262e15d784328ed3f731aea40797d98) )
        ROM_LOAD_VROM( "mpr22857.29",  0x000004, 0x400000, CRC(44e6ce2b) SHA1(03a33417dc96d3ddcbeb92422654f75f620ef265) )
        ROM_LOAD_VROM( "mpr22858.30",  0x00000a, 0x400000, CRC(0af40aae) SHA1(1af762e09af932d267b860712fa46eaf4b1500ac) )
        ROM_LOAD_VROM( "mpr22859.31",  0x000008, 0x400000, CRC(c64f0158) SHA1(b92dba07e52b4fdabc9bd4bbd26cc41cdab0414d) )
        ROM_LOAD_VROM( "mpr22860.32",  0x00000e, 0x400000, CRC(053af14b) SHA1(6d13609e52a4999d6dca3c4da695ebd973b06b7e) )
        ROM_LOAD_VROM( "mpr22861.33",  0x00000c, 0x400000, CRC(d26343da) SHA1(9d8c860c388cc2434cc8d753cde139096c12e79e) )

	ROM_REGION( 0x2000000, REGION_USER4, ROMREGION_DISPOSE )  /* Video ROMs Part 2 */
        ROM_LOAD_VROM( "mpr22862.34",  0x000002, 0x400000, CRC(38347c14) SHA1(6222a5001d3a6ed8e8ff3010284658dbc69edee6) )
        ROM_LOAD_VROM( "mpr22863.35",  0x000000, 0x400000, CRC(28b558e6) SHA1(9b4849499baebbd2ea81d00663dff0d40c9db602) )
        ROM_LOAD_VROM( "mpr22864.36",  0x000006, 0x400000, CRC(31ed02f6) SHA1(a669aa8a42ff70562c86f348fcf4be6f14c2f650) )
        ROM_LOAD_VROM( "mpr22865.37",  0x000004, 0x400000, CRC(3e3a211a) SHA1(422f960914a6604de59b110b18f3e67ed9116f22) )
        ROM_LOAD_VROM( "mpr22866.38",  0x00000a, 0x400000, CRC(a863a3c8) SHA1(52e13b76a3698deef05df4c607d047e6362d81c0) )
        ROM_LOAD_VROM( "mpr22867.39",  0x000008, 0x400000, CRC(1ce6c7b2) SHA1(d5fd49f9838d3dc636366c436c507ac2b4f2596e) )
        ROM_LOAD_VROM( "mpr22868.40",  0x00000e, 0x400000, CRC(2db40cf8) SHA1(be1e04aeb5034c2edc5c0ad153700c385a1b773a) )
        ROM_LOAD_VROM( "mpr22869.41",  0x00000c, 0x400000, CRC(c6d62634) SHA1(72d493653b50fb31333f70c7ee143a8c7531106f) )

	ROM_REGION( 0x80000, REGION_CPU2, 0 )	/* 68000 code */
        ROM_LOAD16_WORD_SWAP( "epr22886.21",  0x000000, 0x080000, CRC(374ec1c6) SHA1(b06e678db191971f6701bd1f739815d00d4cfb3e) )

	// Samples
	ROM_REGION( 0x1000000, REGION_SOUND1, 0 )	/* SCSP samples */
        ROM_LOAD( "mpr22887.22",  0x000000, 0x400000, CRC(7d04a867) SHA1(053de98105880188b4daff183710d7932617547f) )
        ROM_LOAD( "mpr22889.24",  0x400000, 0x400000, CRC(4f9ba45d) SHA1(d60314e852637edf6510be52b9b6576a1f3e1b7e) )
        ROM_LOAD( "mpr22888.23",  0x800000, 0x400000, CRC(018fcf22) SHA1(c5133358f591d699f177617463e7dfa22edf5369) )
        ROM_LOAD( "mpr22890.25",  0xc00000, 0x400000, CRC(b638bd7c) SHA1(ed9c69175fd0ca4c6f22e542b4e68398a6e4ad07) )

	ROM_REGION( 0x20000, REGION_CPU3, 0 )	/* Z80 code */
		ROM_FILL( 0x000000, 0x20000, 0 )

	ROM_REGION( 0x1000000, REGION_SOUND2, 0 )	/* DSB samples */
		ROM_FILL( 0x000000, 0x1000000, 0 )
ROM_END


/* IRQs */
/*
    0x80: Unused ?
    0x40: SCSP
    0x20: Unused ?
    0x10: Network
    0x08: V-blank end ?
    0x04: ???
    0x02: V-blank start
    0x01: Unused ?
*/
static int model3_vblank = 0;
static INTERRUPT_GEN(model3_interrupt)
{
	if (model3_vblank == 0) {
		model3_irq_state = 0x42;
	} else {
		model3_irq_state = 0x0d;
	}
	cpunum_set_input_line(0, INPUT_LINE_IRQ1, ASSERT_LINE);

	model3_vblank++;
	model3_vblank &= 1;
}

static ppc_config model3_10 =
{
	PPC_MODEL_603E,		/* 603e, Stretch, 1.3 */
	0x10,				/* Multiplier 1, Bus = 66MHz, Core = 66MHz */
	BUS_FREQUENCY_66MHZ
};

static ppc_config model3_15 =
{
	PPC_MODEL_603E,		/* 603e, Stretch, 1.3 */
	0x15,				/* Multiplier 1.5, Bus = 66MHz, Core = 100MHz */
	BUS_FREQUENCY_66MHZ
};

static ppc_config model3_2x =
{
	PPC_MODEL_603R,		/* 603e-PID7t, Goldeneye, 2.1 */
	0x25,				/* Multiplier 2.5, Bus = 66MHz, Core = 166MHz */
	BUS_FREQUENCY_66MHZ
};

static MACHINE_DRIVER_START( model3_10 )
	MDRV_CPU_ADD(PPC603, 66000000)
	MDRV_CPU_CONFIG(model3_10)
	MDRV_CPU_PROGRAM_MAP(model3_mem, 0)
 	MDRV_CPU_VBLANK_INT(model3_interrupt,2)

 	MDRV_INTERLEAVE(10)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	MDRV_MACHINE_RESET(model3_10)
	MDRV_NVRAM_HANDLER(model3)

	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER | VIDEO_RGB_DIRECT)
	MDRV_SCREEN_SIZE(496, 384)
	MDRV_VISIBLE_AREA(0, 495, 0, 383)
	MDRV_PALETTE_LENGTH(65536)

	MDRV_VIDEO_START(model3)
	MDRV_VIDEO_UPDATE(model3)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( model3_15 )
	MDRV_CPU_ADD(PPC603, 100000000)
	MDRV_CPU_CONFIG(model3_15)
	MDRV_CPU_PROGRAM_MAP(model3_mem, 0)
 	MDRV_CPU_VBLANK_INT(model3_interrupt,2)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	MDRV_MACHINE_RESET(model3_15)
	MDRV_NVRAM_HANDLER(model3)

	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER | VIDEO_RGB_DIRECT)
	MDRV_SCREEN_SIZE(496, 384)
	MDRV_VISIBLE_AREA(0, 495, 0, 383)
	MDRV_PALETTE_LENGTH(65536)

	MDRV_VIDEO_START(model3)
	MDRV_VIDEO_UPDATE(model3)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( model3_20 )
	MDRV_CPU_ADD(PPC603, 166000000)
	MDRV_CPU_CONFIG(model3_2x)
	MDRV_CPU_PROGRAM_MAP(model3_mem, 0)
 	MDRV_CPU_VBLANK_INT(model3_interrupt,2)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	MDRV_MACHINE_RESET(model3_20)
	MDRV_NVRAM_HANDLER(model3)

	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER | VIDEO_RGB_DIRECT)
	MDRV_SCREEN_SIZE(496, 384)
	MDRV_VISIBLE_AREA(0, 495, 0, 383)
	MDRV_PALETTE_LENGTH(65536)

	MDRV_VIDEO_START(model3)
	MDRV_VIDEO_UPDATE(model3)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( model3_21 )
	MDRV_CPU_ADD(PPC603, 166000000)
	MDRV_CPU_CONFIG(model3_2x)
	MDRV_CPU_PROGRAM_MAP(model3_mem, 0)
 	MDRV_CPU_VBLANK_INT(model3_interrupt,2)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	MDRV_MACHINE_RESET(model3_21)
	MDRV_NVRAM_HANDLER(model3)

	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER | VIDEO_RGB_DIRECT)
	MDRV_SCREEN_SIZE(496, 384)
	MDRV_VISIBLE_AREA(0, 495, 0, 383)
	MDRV_PALETTE_LENGTH(65536)

	MDRV_VIDEO_START(model3)
	MDRV_VIDEO_UPDATE(model3)
MACHINE_DRIVER_END

static void interleave_vroms(void)
{
	int start;
	int i,j,x;
	UINT16 *vrom1 = (UINT16*)memory_region(REGION_USER3);
	UINT16 *vrom2 = (UINT16*)memory_region(REGION_USER4);
	int vrom_length = memory_region_length(REGION_USER3);
	UINT16 *vrom;

	model3_vrom = auto_malloc(0x4000000);
	vrom = (UINT16 *)model3_vrom;

	if( vrom_length <= 0x1000000 ) {
		start = 0x1000000;
	} else {
		start = 0;
	}

	j=0;
	for(i=start; i < 0x2000000; i+=16) {
		for(x=0; x < 8; x++) {
			vrom[i+x+0] = vrom1[(j+x)^1];
		}
		for(x=0; x < 8; x++) {
			vrom[i+x+8] = vrom2[(j+x)^1];
		}
		j+=8;
	}
}

static DRIVER_INIT( model3_10 )
{
	interleave_vroms();

	memory_install_read64_handler( 0, ADDRESS_SPACE_PROGRAM, 0xc0000000, 0xc00000ff, 0, 0, scsi_r );
	memory_install_write64_handler( 0, ADDRESS_SPACE_PROGRAM, 0xc0000000, 0xc00000ff, 0, 0, scsi_w );

	memory_install_read64_handler( 0, ADDRESS_SPACE_PROGRAM, 0xff000000, 0xff7fffff, 0, 0, MRA64_BANK1 );

	memory_install_read64_handler( 0, ADDRESS_SPACE_PROGRAM, 0xf0800cf8, 0xf0800cff, 0, 0, mpc105_addr_r );
	memory_install_write64_handler( 0, ADDRESS_SPACE_PROGRAM, 0xf0800cf8, 0xf0800cff, 0, 0, mpc105_addr_w );
	memory_install_read64_handler( 0, ADDRESS_SPACE_PROGRAM, 0xf0c00cf8, 0xf0c00cff, 0, 0, mpc105_data_r );
	memory_install_write64_handler( 0, ADDRESS_SPACE_PROGRAM, 0xf0c00cf8, 0xf0c00cff, 0, 0, mpc105_data_w );
	memory_install_read64_handler( 0, ADDRESS_SPACE_PROGRAM, 0xf8fff000, 0xf8fff0ff, 0, 0, mpc105_reg_r );
	memory_install_write64_handler( 0, ADDRESS_SPACE_PROGRAM, 0xf8fff000, 0xf8fff0ff, 0, 0, mpc105_reg_w );
}

static DRIVER_INIT( model3_15 )
{
	interleave_vroms();
	memory_install_read64_handler( 0, ADDRESS_SPACE_PROGRAM, 0xff000000, 0xff7fffff, 0, 0, MRA64_BANK1 );

	memory_install_read64_handler( 0, ADDRESS_SPACE_PROGRAM, 0xf0800cf8, 0xf0800cff, 0, 0, mpc105_addr_r );
	memory_install_write64_handler( 0, ADDRESS_SPACE_PROGRAM, 0xf0800cf8, 0xf0800cff, 0, 0, mpc105_addr_w );
	memory_install_read64_handler( 0, ADDRESS_SPACE_PROGRAM, 0xf0c00cf8, 0xf0c00cff, 0, 0, mpc105_data_r );
	memory_install_write64_handler( 0, ADDRESS_SPACE_PROGRAM, 0xf0c00cf8, 0xf0c00cff, 0, 0, mpc105_data_w );
	memory_install_read64_handler( 0, ADDRESS_SPACE_PROGRAM, 0xf8fff000, 0xf8fff0ff, 0, 0, mpc105_reg_r );
	memory_install_write64_handler( 0, ADDRESS_SPACE_PROGRAM, 0xf8fff000, 0xf8fff0ff, 0, 0, mpc105_reg_w );
}

static DRIVER_INIT( model3_20 )
{
	interleave_vroms();
	memory_install_read64_handler( 0, ADDRESS_SPACE_PROGRAM, 0xff000000, 0xff7fffff, 0, 0, MRA64_BANK1 );

	memory_install_read64_handler( 0, ADDRESS_SPACE_PROGRAM, 0xc2000000, 0xc20000ff, 0, 0, real3d_dma_r );
	memory_install_write64_handler( 0, ADDRESS_SPACE_PROGRAM, 0xc2000000, 0xc20000ff, 0, 0, real3d_dma_w );

	memory_install_read64_handler( 0, ADDRESS_SPACE_PROGRAM, 0xfec00000, 0xfedfffff, 0, 0, mpc106_addr_r );
	memory_install_write64_handler( 0, ADDRESS_SPACE_PROGRAM, 0xfec00000, 0xfedfffff, 0, 0, mpc106_addr_w );
	memory_install_read64_handler( 0, ADDRESS_SPACE_PROGRAM, 0xfee00000, 0xfeffffff, 0, 0, mpc106_data_r );
	memory_install_write64_handler( 0, ADDRESS_SPACE_PROGRAM, 0xfee00000, 0xfeffffff, 0, 0, mpc106_data_w );
	memory_install_read64_handler( 0, ADDRESS_SPACE_PROGRAM, 0xf8fff000, 0xf8fff0ff, 0, 0, mpc106_reg_r );
	memory_install_write64_handler( 0, ADDRESS_SPACE_PROGRAM, 0xf8fff000, 0xf8fff0ff, 0, 0, mpc106_reg_w );
}

static DRIVER_INIT( lostwsga )
{
	UINT32 *rom = (UINT32*)memory_region(REGION_USER1);

	init_model3_15();
	/* TODO: there's an M68K device at 0xC0000000 - FF, maybe lightgun controls ? */
	memory_install_read64_handler( 0, ADDRESS_SPACE_PROGRAM, 0xc1000000, 0xc10000ff, 0, 0, scsi_r );
	memory_install_write64_handler( 0, ADDRESS_SPACE_PROGRAM, 0xc1000000, 0xc10000ff, 0, 0, scsi_w );

	rom[0x7374f0/4] = 0x38840004;		/* This seems to be an actual bug in the original code */

	model3_draw_crosshair = 1;
}

static DRIVER_INIT( scud )
{
	UINT32 *rom = (UINT32*)memory_region(REGION_USER1);

	init_model3_15();
	/* TODO: network device at 0xC0000000 - FF */
	memory_install_read64_handler( 0, ADDRESS_SPACE_PROGRAM, 0xf9000000, 0xf90000ff, 0, 0, scsi_r );
	memory_install_write64_handler( 0, ADDRESS_SPACE_PROGRAM, 0xf9000000, 0xf90000ff, 0, 0, scsi_w );

	rom[(0x71275c^4)/4] = 0x60000000;
	rom[(0x71277c^4)/4] = 0x60000000;
}

static DRIVER_INIT( scudp )
{
	UINT32 *rom = (UINT32*)memory_region(REGION_USER1);

	init_model3_15();
	/* TODO: network device at 0xC0000000 - FF */
	memory_install_read64_handler( 0, ADDRESS_SPACE_PROGRAM, 0xc1000000, 0xc10000ff, 0, 0, scsi_r );
	memory_install_write64_handler( 0, ADDRESS_SPACE_PROGRAM, 0xc1000000, 0xc10000ff, 0, 0, scsi_w );

	rom[(0x713724^4)/4] = 0x60000000;
	rom[(0x713744^4)/4] = 0x60000000;

	rom[(0x741f48^4)/4] = 0x60000000;

	rom[(0x741f68^4)/4] = 0x60000000;
	rom[(0x741efc^4)/4] = 0x60000000;
}

static DRIVER_INIT( lemans24 )
{
	UINT32 *rom = (UINT32*)memory_region(REGION_USER1);
	init_model3_15();

	memory_install_read64_handler( 0, ADDRESS_SPACE_PROGRAM, 0xc1000000, 0xc10000ff, 0, 0, scsi_r );
	memory_install_write64_handler( 0, ADDRESS_SPACE_PROGRAM, 0xc1000000, 0xc10000ff, 0, 0, scsi_w );

	rom[(0x73fe38^4)/4] = 0x38840004;		/* This seems to be an actual bug in the original code */

	rom[(0x73eb5c^4)/4] = 0x60000000;
	rom[(0x73edd0^4)/4] = 0x60000000;
	rom[(0x73edc4^4)/4] = 0x60000000;
}

static DRIVER_INIT( vf3 )
{
	UINT32 *rom = (UINT32*)memory_region(REGION_USER1);

	init_model3_10();

	rom[(0x713c7c^4)/4] = 0x60000000;
	rom[(0x713e54^4)/4] = 0x60000000;
	rom[(0x7125b0^4)/4] = 0x60000000;
	rom[(0x7125d0^4)/4] = 0x60000000;

}

static DRIVER_INIT( vs215 )
{
	UINT32 *rom = (UINT32*)memory_region(REGION_USER1);

	rom[(0x70dde0^4)/4] = 0x60000000;
	rom[(0x70e6f0^4)/4] = 0x60000000;
	rom[(0x70e710^4)/4] = 0x60000000;

	interleave_vroms();
	memory_install_read64_handler( 0, ADDRESS_SPACE_PROGRAM, 0xff000000, 0xff7fffff, 0, 0, MRA64_BANK1 );

	memory_install_read64_handler( 0, ADDRESS_SPACE_PROGRAM, 0xf9000000, 0xf90000ff, 0, 0, scsi_r );
	memory_install_write64_handler( 0, ADDRESS_SPACE_PROGRAM, 0xf9000000, 0xf90000ff, 0, 0, scsi_w );

	memory_install_read64_handler( 0, ADDRESS_SPACE_PROGRAM, 0xf0800cf8, 0xf0800cff, 0, 0, mpc106_addr_r );
	memory_install_write64_handler( 0, ADDRESS_SPACE_PROGRAM, 0xf0800cf8, 0xf0800cff, 0, 0, mpc106_addr_w );
	memory_install_read64_handler( 0, ADDRESS_SPACE_PROGRAM, 0xfec00000, 0xfedfffff, 0, 0, mpc106_addr_r );
	memory_install_write64_handler( 0, ADDRESS_SPACE_PROGRAM, 0xfec00000, 0xfedfffff, 0, 0, mpc106_addr_w );
	memory_install_read64_handler( 0, ADDRESS_SPACE_PROGRAM, 0xf0c00cf8, 0xf0c00cff, 0, 0, mpc106_data_r );
	memory_install_write64_handler( 0, ADDRESS_SPACE_PROGRAM, 0xf0c00cf8, 0xf0c00cff, 0, 0, mpc106_data_w );
	memory_install_read64_handler( 0, ADDRESS_SPACE_PROGRAM, 0xfee00000, 0xfeffffff, 0, 0, mpc106_data_r );
	memory_install_write64_handler( 0, ADDRESS_SPACE_PROGRAM, 0xfee00000, 0xfeffffff, 0, 0, mpc106_data_w );
	memory_install_read64_handler( 0, ADDRESS_SPACE_PROGRAM, 0xf8fff000, 0xf8fff0ff, 0, 0, mpc106_reg_r );
	memory_install_write64_handler( 0, ADDRESS_SPACE_PROGRAM, 0xf8fff000, 0xf8fff0ff, 0, 0, mpc106_reg_w );
}

static DRIVER_INIT( vs29815 )
{
	UINT32 *rom = (UINT32*)memory_region(REGION_USER1);

	rom[(0x6028ec^4)/4] = 0x60000000;
	rom[(0x60290c^4)/4] = 0x60000000;

	interleave_vroms();
	memory_install_read64_handler( 0, ADDRESS_SPACE_PROGRAM, 0xff000000, 0xff7fffff, 0, 0, MRA64_BANK1 );

	memory_install_read64_handler( 0, ADDRESS_SPACE_PROGRAM, 0xf9000000, 0xf90000ff, 0, 0, scsi_r );
	memory_install_write64_handler( 0, ADDRESS_SPACE_PROGRAM, 0xf9000000, 0xf90000ff, 0, 0, scsi_w );

	memory_install_read64_handler( 0, ADDRESS_SPACE_PROGRAM, 0xf0800cf8, 0xf0800cff, 0, 0, mpc106_addr_r );
	memory_install_write64_handler( 0, ADDRESS_SPACE_PROGRAM, 0xf0800cf8, 0xf0800cff, 0, 0, mpc106_addr_w );
	memory_install_read64_handler( 0, ADDRESS_SPACE_PROGRAM, 0xfec00000, 0xfedfffff, 0, 0, mpc106_addr_r );
	memory_install_write64_handler( 0, ADDRESS_SPACE_PROGRAM, 0xfec00000, 0xfedfffff, 0, 0, mpc106_addr_w );
	memory_install_read64_handler( 0, ADDRESS_SPACE_PROGRAM, 0xf0c00cf8, 0xf0c00cff, 0, 0, mpc106_data_r );
	memory_install_write64_handler( 0, ADDRESS_SPACE_PROGRAM, 0xf0c00cf8, 0xf0c00cff, 0, 0, mpc106_data_w );
	memory_install_read64_handler( 0, ADDRESS_SPACE_PROGRAM, 0xfee00000, 0xfeffffff, 0, 0, mpc106_data_r );
	memory_install_write64_handler( 0, ADDRESS_SPACE_PROGRAM, 0xfee00000, 0xfeffffff, 0, 0, mpc106_data_w );
	memory_install_read64_handler( 0, ADDRESS_SPACE_PROGRAM, 0xf8fff000, 0xf8fff0ff, 0, 0, mpc106_reg_r );
	memory_install_write64_handler( 0, ADDRESS_SPACE_PROGRAM, 0xf8fff000, 0xf8fff0ff, 0, 0, mpc106_reg_w );
}

static DRIVER_INIT( bass )
{
	UINT32 *rom = (UINT32*)memory_region(REGION_USER1);

	rom[(0x7999a8^4)/4] = 0x60000000;
	rom[(0x7999c8^4)/4] = 0x60000000;

	interleave_vroms();
	memory_install_read64_handler( 0, ADDRESS_SPACE_PROGRAM, 0xff000000, 0xff7fffff, 0, 0, MRA64_BANK1 );

	memory_install_read64_handler( 0, ADDRESS_SPACE_PROGRAM, 0xf9000000, 0xf90000ff, 0, 0, scsi_r );
	memory_install_write64_handler( 0, ADDRESS_SPACE_PROGRAM, 0xf9000000, 0xf90000ff, 0, 0, scsi_w );

	memory_install_read64_handler( 0, ADDRESS_SPACE_PROGRAM, 0xf0800cf8, 0xf0800cff, 0, 0, mpc106_addr_r );
	memory_install_write64_handler( 0, ADDRESS_SPACE_PROGRAM, 0xf0800cf8, 0xf0800cff, 0, 0, mpc106_addr_w );
	memory_install_read64_handler( 0, ADDRESS_SPACE_PROGRAM, 0xfec00000, 0xfedfffff, 0, 0, mpc106_addr_r );
	memory_install_write64_handler( 0, ADDRESS_SPACE_PROGRAM, 0xfec00000, 0xfedfffff, 0, 0, mpc106_addr_w );
	memory_install_read64_handler( 0, ADDRESS_SPACE_PROGRAM, 0xf0c00cf8, 0xf0c00cff, 0, 0, mpc106_data_r );
	memory_install_write64_handler( 0, ADDRESS_SPACE_PROGRAM, 0xf0c00cf8, 0xf0c00cff, 0, 0, mpc106_data_w );
	memory_install_read64_handler( 0, ADDRESS_SPACE_PROGRAM, 0xfee00000, 0xfeffffff, 0, 0, mpc106_data_r );
	memory_install_write64_handler( 0, ADDRESS_SPACE_PROGRAM, 0xfee00000, 0xfeffffff, 0, 0, mpc106_data_w );
	memory_install_read64_handler( 0, ADDRESS_SPACE_PROGRAM, 0xf8fff000, 0xf8fff0ff, 0, 0, mpc106_reg_r );
	memory_install_write64_handler( 0, ADDRESS_SPACE_PROGRAM, 0xf8fff000, 0xf8fff0ff, 0, 0, mpc106_reg_w );
}

static DRIVER_INIT( getbass )
{
	interleave_vroms();
	memory_install_read64_handler( 0, ADDRESS_SPACE_PROGRAM, 0xff000000, 0xff7fffff, 0, 0, MRA64_BANK1 );

	memory_install_read64_handler( 0, ADDRESS_SPACE_PROGRAM, 0xf9000000, 0xf90000ff, 0, 0, scsi_r );
	memory_install_write64_handler( 0, ADDRESS_SPACE_PROGRAM, 0xf9000000, 0xf90000ff, 0, 0, scsi_w );

	memory_install_read64_handler( 0, ADDRESS_SPACE_PROGRAM, 0xf0800cf8, 0xf0800cff, 0, 0, mpc105_addr_r );
	memory_install_write64_handler( 0, ADDRESS_SPACE_PROGRAM, 0xf0800cf8, 0xf0800cff, 0, 0, mpc105_addr_w );
	memory_install_read64_handler( 0, ADDRESS_SPACE_PROGRAM, 0xf0c00cf8, 0xf0c00cff, 0, 0, mpc105_data_r );
	memory_install_write64_handler( 0, ADDRESS_SPACE_PROGRAM, 0xf0c00cf8, 0xf0c00cff, 0, 0, mpc105_data_w );
	memory_install_read64_handler( 0, ADDRESS_SPACE_PROGRAM, 0xf8fff000, 0xf8fff0ff, 0, 0, mpc105_reg_r );
	memory_install_write64_handler( 0, ADDRESS_SPACE_PROGRAM, 0xf8fff000, 0xf8fff0ff, 0, 0, mpc105_reg_w );
}

static DRIVER_INIT( vs2 )
{
	UINT32 *rom = (UINT32*)memory_region(REGION_USER1);

	init_model3_20();

	rom[(0x705884^4)/4] = 0x60000000;
	rom[(0x7058a4^4)/4] = 0x60000000;
}

static DRIVER_INIT( vs298 )
{
	UINT32 *rom = (UINT32*)memory_region(REGION_USER1);

	init_model3_20();

	rom[(0x603868^4)/4] = 0x60000000;
	rom[(0x603888^4)/4] = 0x60000000;
}


static DRIVER_INIT( vs2v991 )
{
	UINT32 *rom = (UINT32*)memory_region(REGION_USER1);

	init_model3_20();

	rom[(0x603868^4)/4] = 0x60000000;
	rom[(0x603888^4)/4] = 0x60000000;
}

static DRIVER_INIT( vs299 )
{
	UINT32 *rom = (UINT32*)memory_region(REGION_USER1);

	init_model3_20();

	rom[(0x603868^4)/4] = 0x60000000;
	rom[(0x603888^4)/4] = 0x60000000;
}

static DRIVER_INIT( harley )
{
	UINT32 *rom = (UINT32*)memory_region(REGION_USER1);
	init_model3_20();

	memory_install_read64_handler( 0, ADDRESS_SPACE_PROGRAM, 0xc0000000, 0xc00fffff, 0, 0, network_r );
	memory_install_write64_handler( 0, ADDRESS_SPACE_PROGRAM, 0xc0000000, 0xc00fffff, 0, 0, network_w );

	rom[(0x50e8d4^4)/4] = 0x60000000;
	rom[(0x50e8f4^4)/4] = 0x60000000;
	rom[(0x50fb84^4)/4] = 0x60000000;
	rom[(0x4f736c^4)/4] = 0x60000000;
	rom[(0x4f738c^4)/4] = 0x60000000;
}

static DRIVER_INIT( srally2 )
{
	UINT32 *rom = (UINT32*)memory_region(REGION_USER1);
	init_model3_20();

	rom[(0x7c0c4^4)/4] = 0x60000000;
	rom[(0x7c0c8^4)/4] = 0x60000000;
	rom[(0x7c0cc^4)/4] = 0x60000000;
}

static DRIVER_INIT( swtrilgy )
{
	UINT32 *rom = (UINT32*)memory_region(REGION_USER1);
	init_model3_20();

	rom[(0xf0e48^4)/4] = 0x60000000;
    rom[(0x043dc^4)/4] = 0x48000090;
    rom[(0x029a0^4)/4] = 0x60000000;
    rom[(0x02a0c^4)/4] = 0x60000000;
}

static DRIVER_INIT( swtrilga )
{
	UINT32 *rom = (UINT32*)memory_region(REGION_USER1);
	init_model3_20();

	rom[(0xf6dd0^4)/4] = 0x60000000;
}

static DRIVER_INIT( von2 )
{
	UINT32 *rom = (UINT32*)memory_region(REGION_USER1);
	init_model3_20();

	rom[(0x189168^4)/4] = 0x60000000;
	rom[(0x1890ac^4)/4] = 0x60000000;
	rom[(0x1890b8^4)/4] = 0x60000000;
	rom[(0x1888a8^4)/4] = 0x60000000;
	rom[(0x1891c8^4)/4] = 0x60000000;
}

static DRIVER_INIT( dirtdvls )
{
	UINT32 *rom = (UINT32*)memory_region(REGION_USER1);
	init_model3_20();

	rom[(0x0600a0^4)/4] = 0x60000000;
	rom[(0x0608a4^4)/4] = 0x60000000;
	rom[(0x0608b0^4)/4] = 0x60000000;
	rom[(0x060960^4)/4] = 0x60000000;
	rom[(0x0609c0^4)/4] = 0x60000000;
	rom[(0x001e24^4)/4] = 0x60000000;
}

static DRIVER_INIT( daytona2 )
{
	UINT32 *rom = (UINT32*)memory_region(REGION_USER1);
	init_model3_20();

	memory_install_write64_handler( 0, ADDRESS_SPACE_PROGRAM, 0xc3800000, 0xc3800007, 0, 0, daytona2_rombank_w );

	//rom[(0x68468c^4)/4] = 0x60000000;
	rom[(0x6063c4^4)/4] = 0x60000000;
	rom[(0x616434^4)/4] = 0x60000000;
	rom[(0x69f4e4^4)/4] = 0x60000000;
}

static DRIVER_INIT( dayto2pe )
{
	UINT32 *rom = (UINT32*)memory_region(REGION_USER1);
	init_model3_20();

	memory_install_write64_handler( 0, ADDRESS_SPACE_PROGRAM, 0xc3800000, 0xc3800007, 0, 0, daytona2_rombank_w );

	rom[(0x606784^4)/4] = 0x60000000;
	rom[(0x69a3fc^4)/4] = 0x60000000;		// jump to encrypted code
	rom[(0x618b28^4)/4] = 0x60000000;		// jump to encrypted code

	rom[(0x64ca34^4)/4] = 0x60000000;		// dec
}

static DRIVER_INIT( spikeout )
{
	UINT32 *rom = (UINT32*)memory_region(REGION_USER1);
	init_model3_20();

	rom[(0x6059cc^4)/4] = 0x60000000;
	rom[(0x6059ec^4)/4] = 0x60000000;
}

static DRIVER_INIT( spikeofe )
{
	UINT32 *rom = (UINT32*)memory_region(REGION_USER1);
	init_model3_20();

	rom[(0x6059cc^4)/4] = 0x60000000;
	rom[(0x6059ec^4)/4] = 0x60000000;
}

static DRIVER_INIT( eca )
{
	UINT32 *rom = (UINT32*)memory_region(REGION_USER1);
	init_model3_20();

	rom[(0x535580^4)/4] = 0x60000000;
	rom[(0x5023b4^4)/4] = 0x60000000;
	rom[(0x5023d4^4)/4] = 0x60000000;
}





/* Model 3 Step 1.0 */
GAME( 1996, vf3,            0, model3_10, model3,        vf3, ROT0, "Sega", "Virtua Fighter 3", GAME_NOT_WORKING | GAME_IMPERFECT_GRAPHICS | GAME_NO_SOUND )
GAME( 1996, vf3tb,        vf3, model3_10, model3,  model3_10, ROT0, "Sega", "Virtua Fighter 3 Team Battle", GAME_NOT_WORKING | GAME_IMPERFECT_GRAPHICS | GAME_NO_SOUND )
GAME( 1997, bass,           0, model3_10, bass,         bass, ROT0, "Sega", "Sega Bass Fishing", GAME_NOT_WORKING | GAME_IMPERFECT_GRAPHICS | GAME_NO_SOUND )
GAME( 1997, getbass,     bass, model3_10, bass,      getbass, ROT0, "Sega", "Get Bass", GAME_NOT_WORKING | GAME_IMPERFECT_GRAPHICS | GAME_NO_SOUND )

/* Model 3 Step 1.5 */
GAME( 1996, scud,           0, model3_15, scud,         scud, ROT0, "Sega", "Scud Race (Australia)", GAME_NOT_WORKING | GAME_IMPERFECT_GRAPHICS | GAME_NO_SOUND )
GAME( 1996, scuda,       scud, model3_15, scud,         scud, ROT0, "Sega", "Scud Race (Export)", GAME_NOT_WORKING | GAME_IMPERFECT_GRAPHICS | GAME_NO_SOUND )
GAME( 1997, scudp,       scud, model3_15, scud,        scudp, ROT0, "Sega", "Scud Race Plus", GAME_NOT_WORKING | GAME_IMPERFECT_GRAPHICS | GAME_NO_SOUND )
GAME( 1997, lostwsga,       0, model3_15, lostwsga, lostwsga, ROT0, "Sega", "The Lost World", GAME_NOT_WORKING | GAME_IMPERFECT_GRAPHICS | GAME_NO_SOUND )
GAME( 1997, vs215,        vs2, model3_15, model3,      vs215, ROT0, "Sega", "Virtua Striker 2 (Step 1.5)", GAME_NOT_WORKING | GAME_IMPERFECT_GRAPHICS | GAME_NO_SOUND )
GAME( 1997, lemans24,       0, model3_15, scud,     lemans24, ROT0, "Sega", "LeMans 24", GAME_NOT_WORKING | GAME_IMPERFECT_GRAPHICS | GAME_NO_SOUND )
GAME( 1998, vs29815,    vs298, model3_15, model3,    vs29815, ROT0, "Sega", "Virtua Striker 2 '98 (Step 1.5)", GAME_NOT_WORKING | GAME_IMPERFECT_GRAPHICS | GAME_NO_SOUND )

/* Model 3 Step 2.0 */
GAME( 1997, vs2,            0, model3_20, model3,        vs2, ROT0, "Sega", "Virtua Striker 2 (Step 2.0)", GAME_NOT_WORKING | GAME_IMPERFECT_GRAPHICS | GAME_NO_SOUND )
GAME( 1997, harley,         0, model3_20, harley,     harley, ROT0, "Sega", "Harley-Davidson and L.A. Riders", GAME_NOT_WORKING | GAME_IMPERFECT_GRAPHICS | GAME_NO_SOUND )
GAME( 1998, srally2,        0, model3_20, scud,      srally2, ROT0, "Sega", "Sega Rally 2", GAME_NOT_WORKING | GAME_IMPERFECT_GRAPHICS | GAME_NO_SOUND )
GAME( 1998, sraly2dx,       0, model3_20, scud,      srally2, ROT0, "Sega", "Sega Rally 2 DX", GAME_NOT_WORKING | GAME_IMPERFECT_GRAPHICS | GAME_NO_SOUND )
GAME( 1998, von2,           0, model3_20, model3,       von2, ROT0, "Sega", "Virtual On 2: Oratorio Tangram", GAME_NOT_WORKING | GAME_IMPERFECT_GRAPHICS | GAME_NO_SOUND )
GAME( 1998, von254g,     von2, model3_20, model3,  model3_20, ROT0, "Sega", "Virtual On 2: Oratorio Tangram (ver 5.4g)", GAME_NOT_WORKING | GAME_IMPERFECT_GRAPHICS | GAME_NO_SOUND )
GAME( 1998, fvipers2,       0, model3_20, model3,  model3_20, ROT0, "Sega", "Fighting Vipers 2", GAME_NOT_WORKING | GAME_IMPERFECT_GRAPHICS | GAME_NO_SOUND )
GAME( 1998, vs298,          0, model3_20, model3,      vs298, ROT0, "Sega", "Virtua Striker 2 '98 (Step 2.0)", GAME_NOT_WORKING | GAME_IMPERFECT_GRAPHICS | GAME_NO_SOUND )
GAME( 1999, vs2v991,        0, model3_20, model3,    vs2v991, ROT0, "Sega", "Virtua Striker 2 '99.1", GAME_NOT_WORKING | GAME_IMPERFECT_GRAPHICS | GAME_NO_SOUND )
GAME( 1999, vs299,    vs2v991, model3_20, model3,      vs299, ROT0, "Sega", "Virtua Striker 2 '99", GAME_NOT_WORKING | GAME_IMPERFECT_GRAPHICS | GAME_NO_SOUND )

/* Model 3 Step 2.1 */
GAME( 1998, daytona2,       0, model3_21, daytona2, daytona2, ROT0, "Sega", "Daytona USA 2", GAME_NOT_WORKING | GAME_IMPERFECT_GRAPHICS | GAME_NO_SOUND )
GAME( 1998, dayto2pe,       0, model3_21, daytona2, dayto2pe, ROT0, "Sega", "Daytona USA 2 Power Edition", GAME_NOT_WORKING | GAME_IMPERFECT_GRAPHICS | GAME_NO_SOUND )
GAME( 1998, dirtdvls,       0, model3_21, model3,   dirtdvls, ROT0, "Sega", "Dirt Devils", GAME_NOT_WORKING | GAME_IMPERFECT_GRAPHICS | GAME_NO_SOUND )
GAME( 1998, dirtdvla,dirtdvls, model3_21, model3,   dirtdvls, ROT0, "Sega", "Dirt Devils (alt)", GAME_NOT_WORKING | GAME_IMPERFECT_GRAPHICS | GAME_NO_SOUND )
GAME( 1998, swtrilgy,       0, model3_21, swtrilgy, swtrilgy, ROT0, "Sega/LucasArts", "Star Wars Trilogy (ver A)", GAME_NOT_WORKING | GAME_IMPERFECT_GRAPHICS | GAME_NO_SOUND )
GAME( 1998, swtrilga,swtrilgy, model3_21, swtrilgy, swtrilga, ROT0, "Sega/LucasArts", "Star Wars Trilogy", GAME_NOT_WORKING | GAME_IMPERFECT_GRAPHICS | GAME_NO_SOUND )
GAME( 1998, spikeout,       0, model3_21, model3,   spikeout, ROT0, "Sega", "Spikeout", GAME_NOT_WORKING | GAME_IMPERFECT_GRAPHICS | GAME_NO_SOUND )
GAME( 1998, spikeofe,       0, model3_21, model3,   spikeofe, ROT0, "Sega", "Spikeout Final Edition", GAME_NOT_WORKING | GAME_IMPERFECT_GRAPHICS | GAME_NO_SOUND )
GAME( 1999, eca,            0, model3_21, eca,           eca, ROT0, "Sega", "Emergency Call Ambulance", GAME_NOT_WORKING | GAME_IMPERFECT_GRAPHICS | GAME_NO_SOUND )
