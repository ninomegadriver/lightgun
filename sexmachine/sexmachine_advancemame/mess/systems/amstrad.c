/******************************************************************************

	amstrad.c
	system driver

		Amstrad Hardware:
			- 8255 connected to AY-3-8912 sound generator,
				keyboard, cassette, printer, crtc vsync output,
				and some PCB links
			- 6845 (either HD6845S, UM6845R or M6845) crtc graphics display
			  controller
			- NEC765 floppy disc controller (CPC664,CPC6128)
			- Z80 CPU running at 4Mhz (slowed by wait states on memory
			  access)
			- custom ASIC "Gate Array" controlling rom paging, ram paging,
				current display mode and colour palette

	Kevin Thacker [MESS driver]

  May-June 2004 - Yoann Courtois (aka Papagayo/ex GMC/ex PARADOX) - rewritting some code with hardware documentation from http://andercheran.aiind.upv.es/~amstrad
  
Some bugs left :
----------------
    - CRTC all type support (0,1,2,3,4) ?
    - Gate Array and CRTC aren't synchronised. (The Gate Array can change the color every microseconds?) So the multi-rasters in one line aren't supported (see yao demo p007's part)!
    - Implement full Asic for CPC+ emulation
 ******************************************************************************/
#include "driver.h"

#include "includes/centroni.h"
#include "machine/8255ppi.h"	/* for 8255 ppi */
#include "cpu/z80/z80.h"		/* for cycle tables */
#include "vidhrdw/m6845.h"		/* CRTC display */
#include "includes/amstrad.h"
#include "machine/nec765.h"	/* for floppy disc controller */
#include "devices/dsk.h"		/* for CPCEMU style disk images */
#include "devices/snapquik.h"
#include "devices/cartslot.h"
#include "devices/printer.h"
#include "devices/cassette.h"
#include "inputx.h"
#include "sound/ay8910.h"

#ifdef AMSTRAD_VIDEO_EVENT_LIST
/* for event list */
#include "eventlst.h"
#endif

#define MANUFACTURER_NAME 0x07
#define TV_REFRESH_RATE 0x10

//int selected_crtc6845_address = 0;

/* the hardware allows selection of 256 ROMs. Rom 0 is usually BASIC and Rom 7 is AMSDOS */
/* With the CPC hardware, if a expansion ROM is not connected, BASIC rom will be selected instead */
static unsigned char *Amstrad_ROM_Table[256];
/* data present on input of ppi, and data written to ppi output */
#define amstrad_ppi_PortA 0
#define amstrad_ppi_PortB 1
#define amstrad_ppi_PortC 2

static int ppi_port_inputs[3];
static int ppi_port_outputs[3];


/* keyboard line 0-9 */
static int amstrad_keyboard_line;

static unsigned char previous_amstrad_UpperRom_data;
static unsigned char previous_printer_data_byte;
/*------------------
  - Ram Management -
  ------------------*/
/* current selected upper rom */
static unsigned char *Amstrad_UpperRom;
/* There are 8 different ram configurations which work on the currently selected 64k logical block. 
   The following tables show the possible ram configurations :*/
static int RamConfigurations[8 * 4] =
{
	0, 1, 2, 3, 					   /* config 0 */
	0, 1, 2, 7, 					   /* config 1 */
	4, 5, 6, 7, 					   /* config 2 */
	0, 3, 2, 7, 					   /* config 3 */
	0, 4, 2, 3, 					   /* config 4 */
	0, 5, 2, 3, 					   /* config 5 */
	0, 6, 2, 3, 					   /* config 6 */
	0, 7, 2, 3					     /* config 7 */
};
/*-------------------
  - Gate Array data -
  -------------------*/
/* Pen selection */
static int amstrad_GateArray_PenSelected = 0;
/* Rom configuration */
static int amstrad_GateArray_ModeAndRomConfiguration = 0;
/* Ram configuration */
static int amstrad_GateArray_RamConfiguration = 0;
/* The gate array counts CRTC HSYNC pulses. (It has a internal 6-bit counter). */
extern int amstrad_CRTC_HS_Counter;
/*-------------
  - MULTIFACE -
  -------------*/
static void multiface_rethink_memory(void);
static WRITE8_HANDLER(multiface_io_write);
static void multiface_init(void);
static void multiface_stop(void);
static int multiface_hardware_enabled(void);
static void multiface_reset(void);
/* ---------------------------------------
   - 27.05.2004 - PSG function selection - 
   ---------------------------------------
The databus of the PSG is connected to PPI Port A.
Data is read from/written to the PSG through this port. 

The PSG function, defined by the BC1,BC2 and BDIR signals, is controlled by bit 7 and bit 6 of PPI Port C. 

PSG function selection:
-----------------------
Function          

BDIR = PPI Port C Bit 7 and BC1 = PPI Port C Bit 6

PPI Port C Bit | PSG Function 
BDIR BC1       | 
0    0         | Inactive 
0    1         | Read from selected PSG register. When function is set, the PSG will make the register data available to PPI Port A
1    0         | Write to selected PSG register. When set, the PSG will take the data at PPI Port A and write it into the selected PSG register
1    1         | Select PSG register. When set, the PSG will take the data at PPI Port A and select a register
*/
/* PSG function selected */
static unsigned char amstrad_Psg_FunctionSelected;

static void update_psg(void)
{
  switch (amstrad_Psg_FunctionSelected) {
  	case 0: {/* Inactive */
    } break;
  	case 1: {/* b6 = 1 ? : Read from selected PSG register and make the register data available to PPI Port A */
  		ppi_port_inputs[amstrad_ppi_PortA] = AY8910_read_port_0_r(0);
  	} break;
  	case 2: {/* b7 = 1 ? : Write to selected PSG register and write data to PPI Port A */
  		AY8910_write_port_0_w(0, ppi_port_outputs[amstrad_ppi_PortA]);
  	} break;
  	case 3: {/* b6 and b7 = 1 ? : The register will now be selected and the user can read from or write to it.  The register will remain selected until another is chosen.*/
  		AY8910_control_port_0_w(0, ppi_port_outputs[amstrad_ppi_PortA]);
  	} break;
  	default: {
    } break;
	}
}

/* Read/Write 8255 PPI port A (connected to AY-3-8912 databus) */
static READ8_HANDLER ( amstrad_ppi_porta_r )
{
	update_psg();
  return ppi_port_inputs[amstrad_ppi_PortA];
}
	
static WRITE8_HANDLER ( amstrad_ppi_porta_w )
{
  	ppi_port_outputs[amstrad_ppi_PortA] = data;
    update_psg();
}

/* - Read PPI Port B - 
   -------------------
Bit Description
7   Cassette read data
6   Parallel/Printer port ready signal ("1" = not ready, "0" = Ready)
5   /EXP signal on expansion port (note 6)  
4   50/60hz (link on PCB. For this MESS driver I have used the dipswitch feature) (note 5) 
3   | PCB links to define manufacturer name. For this MESS driver I have used the dipswitch feature. (note 1) (note 4) 
2   | (note 2) 
1   | (note 3) 
0   VSYNC State from 6845. "1" = VSYNC active, "0" = VSYNC inactive 

Note: 

1 On CPC464,CPC664,CPC6128 and GX4000 this is LK3 on the PCB. On the CPC464+ and CPC6128+ this is LK103 on the PCB. On the KC compact this is "1". 
2 On CPC464,CPC664,CPC6128 and GX4000 this is LK2 on the PCB. On the CPC464+ and CPC6128+ this is LK102 on the PCB. On the KC compact this is "0". 
3 On CPC464,CPC664,CPC6128 and GX4000 this is LK1 on the PCB. On the CPC464+ and CPC6128+ this is LK101 on the PCB. On the KC compact this is /TEST signal from the expansion port. 
4 On the CPC464,CPC664,CPC6128,CPC464+,CPC6128+ and GX4000 bits 3,2 and 1 define the manufacturer name. See below to see the options available. The manufacturer name is defined on the PCB and cannot be changed through software. 
5 On the CPC464,CPC664,CPC6128,CPC464+,CPC6128+ and GX4000 bit 4 defines the Screen refresh frequency. "1" = 50Hz, "0" = 60Hz. This is defined on the PCB and cannot be changed with software. On the KC compact bit 4 is "1" 
6 This bit is connected to /EXP signal on the expansion port. 
  On the KC Compact this bit is used to define bit 7 of the printer data. 
  On the CPC, it is possible to use this bit to define bit 7 of the printer data, so a 8-bit printer port is made, with a hardware modification, 
  On the CPC this can be used by a expansion device to report it's presence. "1" = device connected, "0" = device not connected. This is not always used by all expansion devices. 
*/

static READ8_HANDLER (amstrad_ppi_portb_r)
{
	int data = 0;
/* Set b7 with cassette tape input */
	if (cassette_input(image_from_devtype_and_index(IO_CASSETTE, 0)) > 0.03) {
		data |= (1<<7);
  }
/* Set b6 with Parallel/Printer port ready */
	if (printer_status(image_from_devtype_and_index(IO_PRINTER, 0), 0)==0 ) {
		data |= (1<<6);
  }
/* Set b4-b1 50hz/60hz state and manufacturer name defined by links on PCB */
	data |= (ppi_port_inputs[amstrad_ppi_PortB] & 0x1e);

/* 	Set b0 with VSync state from the CRTC */
	data |= amstrad_CRTC_VS; // crtc6845_vertical_sync_r(0);

	return data;
}

/* 26-May-2005 - PPI Port C
   -----------------------
Bit Description  Usage 
7   PSG BDIR     | PSG function selection 
6   PSG BC1      |
5                Cassette Write data   
4                Cassette Motor Control set bit to "1" for motor on, or "0" for motor off 
3                | Keyboard line Select keyboard line to be scanned (0-15) 
2                |
1                |
0                |*/

/* previous_ppi_portc_w value */
static int previous_ppi_portc_w;

static WRITE8_HANDLER ( amstrad_ppi_portc_w )
{
	int changed_data;

	previous_ppi_portc_w = ppi_port_outputs[amstrad_ppi_PortC];
/* Write the data to Port C */
	changed_data = previous_ppi_portc_w^data;
	
	ppi_port_outputs[amstrad_ppi_PortC] = data;

/* get b7 and b6 (PSG Function Selected */
	amstrad_Psg_FunctionSelected = ((data & 0xC0)>>6);

/* Perform PSG function */
	update_psg();
	
/* b5 Cassette Write data */
	if ((changed_data & 0x20) != 0) {
		cassette_output(image_from_devtype_and_index(IO_CASSETTE, 0),
			((data & 0x20) ? -1.0 : +1.0));
	}
	
/* b4 Cassette Motor Control */
	if ((changed_data & 0x10) != 0) {
		cassette_change_state(image_from_devtype_and_index(IO_CASSETTE, 0),
			((data & 0x10) ? CASSETTE_MOTOR_ENABLED : CASSETTE_MOTOR_DISABLED),
			CASSETTE_MASK_MOTOR);
	}

/* b3-b0 Keyboard line Select keyboard line to be scanned */
  amstrad_keyboard_line = (data & 0x0F);
}

/* -----------------------------
   - amstrad_ppi8255_interface -
   -----------------------------*/
static ppi8255_interface amstrad_ppi8255_interface =
{
	1,                       /* number of PPIs to emulate */
	{amstrad_ppi_porta_r},   /* port A read */
	{amstrad_ppi_portb_r},   /* port B read */
	{NULL},                  /* port C read */
	{amstrad_ppi_porta_w},   /* port A write */
	{NULL},                  /* port B write */
	{amstrad_ppi_portc_w}    /* port C write */
};

/* Amstrad NEC765 interface doesn't use interrupts or DMA! */
static nec765_interface amstrad_nec765_interface =
{
	NULL,
	NULL
};

/* pointers to current ram configuration selected for banks */
static unsigned char *AmstradCPC_RamBanks[4];

/*--------------------------
  - Ram and Rom management -
  --------------------------*/ 
/*-----------------
  - Set Lower Rom -
  -----------------*/ 
void amstrad_setLowerRom(void)
{
	unsigned char *BankBase;
/* b2 : "1" Lower rom area disable or "0" Lower rom area enable */ 
		if ((amstrad_GateArray_ModeAndRomConfiguration & (1<<2)) == 0) {
			BankBase = &memory_region(REGION_CPU1)[0x010000];
		} else {
			BankBase = AmstradCPC_RamBanks[0];
		}
/* bank 0 - 0x0000..0x03fff */
		memory_set_bankptr(1, BankBase);
		memory_set_bankptr(2, BankBase+0x02000);
}		
/*-----------------
  - Set Upper Rom -
  -----------------*/ 
void amstrad_setUpperRom(void)
{
	unsigned char *BankBase;
/* b3 : "1" Upper rom area disable or "0" Upper rom area enable */
	if ((amstrad_GateArray_ModeAndRomConfiguration & (1<<3)) == 0) {
		BankBase = Amstrad_UpperRom;
	} else {
		BankBase = AmstradCPC_RamBanks[3];
	}

	if (BankBase)
	{
		memory_set_bankptr(7, BankBase);
		memory_set_bankptr(8, BankBase+0x2000);
	}
}		
void AmstradCPC_SetUpperRom(int Data)
{
	Amstrad_UpperRom = Amstrad_ROM_Table[Data & 0xFF];
	amstrad_setUpperRom();
}
/*------------------
  - Rethink Memory -
  ------------------*/ 
void amstrad_rethinkMemory(void)
{
	/* the following is used for banked memory read/writes and for setting up
	 * opcode and opcode argument reads */
/* bank 0 - 0x0000..0x03fff */
    amstrad_setLowerRom();
/* bank 1 - 0x04000..0x07fff */
		memory_set_bankptr(3, AmstradCPC_RamBanks[1]);
		memory_set_bankptr(4, AmstradCPC_RamBanks[1]+0x2000);
/* bank 2 - 0x08000..0x0bfff */
		memory_set_bankptr(5, AmstradCPC_RamBanks[2]);
		memory_set_bankptr(6, AmstradCPC_RamBanks[2]+0x2000);
/* bank 3 - 0x0c000..0x0ffff */
    amstrad_setUpperRom();
/* other banks */    
		memory_set_bankptr(9, AmstradCPC_RamBanks[0]);
		memory_set_bankptr(10, AmstradCPC_RamBanks[0]+0x2000);
		memory_set_bankptr(11, AmstradCPC_RamBanks[1]);
		memory_set_bankptr(12, AmstradCPC_RamBanks[1]+0x2000);
		memory_set_bankptr(13, AmstradCPC_RamBanks[2]);
		memory_set_bankptr(14, AmstradCPC_RamBanks[2]+0x2000);
		memory_set_bankptr(15, AmstradCPC_RamBanks[3]);
		memory_set_bankptr(16, AmstradCPC_RamBanks[3]+0x2000);

/* multiface hardware enabled? */
		if (multiface_hardware_enabled()) {
			multiface_rethink_memory();
		}
}
/* simplified ram configuration - e.g. only correct for 128k machines

RAM Expansion Bits 
                             7 6 5 4  3  2  1  0 
CPC6128                      1 1 - -  -  s2 s1 s0 
Dk'tronics 256K Silicon Disk 1 1 1 b1 b0 b2 -  - 

"-" - this bit is ignored. The value of this bit is not important. 
"0" - this bit must be set to "0" 
"1" - this bit must be set to "1" 
"b0,b1,b2" - this bit is used to define the logical 64k block that the ram configuration uses 
"s0,s1,s2" - this bit is used to define the ram configuration 

The CPC6128 has a 64k ram expansion built-in, giving 128K of RAM in this system. 
In the CPC464,CPC664 and KC compact if a ram expansion is not present, then writing to this port has no effect and the ram will be in the same arrangement as if configuration 0 had been selected. 
*/
static void AmstradCPC_GA_SetRamConfiguration(void)
{
	int ConfigurationIndex = amstrad_GateArray_RamConfiguration & 0x07;
	int BankIndex,i;
	unsigned char *BankAddr;
/* if b5 = 0 */
  if(((amstrad_GateArray_RamConfiguration) & (1<<5)) == 0)  {
    for (i=0;i<4;i++) {
    	BankIndex = RamConfigurations[(ConfigurationIndex << 2) + i];
    	BankAddr = mess_ram + (BankIndex << 14);
    	AmstradCPC_RamBanks[i] = BankAddr;
    }
  } else {/* Need to add the ram expansion configuration here ! */
  } 
  amstrad_rethinkMemory();
}
/* -------------------
   -  the Gate Array -
   -------------------
The gate array is controlled by I/O. The recommended I/O port address is &7Fxx. 
The gate array is selected when bit 15 of the I/O port address is set to "0" and bit 14 of the I/O port address is set to "1".
The values of the other bits are ignored.
However, to avoid conflict with other devices in the system, these bits should be set to "1". 

The function to be performed is selected by writing data to the Gate-Array, bit 7 and 6 of the data define the function selected (see table below).
It is not possible to read from the Gate-Array. 

Bit 7 Bit 6 Function 
0     0     Select pen 
0     1     Select colour for selected pen 
1     0     Select screen mode, rom configuration and interrupt control 
1     1     Ram Memory Management (note 1) 

Note 1 : This function is not available in the Gate-Array, but is performed by a device at the same I/O port address location. In the CPC464,CPC664 and KC compact, this function is performed in a memory-expansion (e.g. Dk'Tronics 64K Ram Expansion), if this expansion is not present then the function is not available. In the CPC6128, this function is performed by a PAL located on the main PCB, or a memory-expansion. In the 464+ and 6128+ this function is performed by the ASIC or a memory expansion. Please read the document on Ram Management for more information.*/

void amstrad_GateArray_write(int dataToGateArray)
{
/* Get Bit 7 and 6 of the dataToGateArray = Gate Array function selected */
	switch ((dataToGateArray & 0xc0)>>6) {
/* Pen selection
   -------------
Bit Value Function        Bit Value Function
5   x     not used        5   x     not used 
4   1     Select border   4   0     Select pen 
3   x     | ignored       3   x     | Pen Number 
2   x     |               2   x 	  |
1   x     |               1   x     |
0   x     |               0   x     |
*/
  	case 0x00:	{
/* Select Border Number, get b4 */
       amstrad_GateArray_PenSelected = dataToGateArray & (1<<4);
/* if b4 = 0 : Select Pen Number, get b3-b0 */
        if (amstrad_GateArray_PenSelected == 0) {
   		    amstrad_GateArray_PenSelected = dataToGateArray & 0x0F;
   		  }
    }	break;
/* Colour selection
   ----------------
Even though there is provision for 32 colours, only 27 are possible.
The remaining colours are duplicates of those already in the colour palette. 

Bit Value Function 
5   x     not used 
4   x     | Colour number 
3   x     |
2   x     |
1   x     |
0   x     |*/
  	case 0x01: {
#ifdef AMSTRAD_VIDEO_EVENT_LIST
    EventList_AddItemOffset((EVENT_LIST_CODE_GA_COLOUR<<6) | PenIndex, AmstradCPC_PenColours[PenIndex], TIME_TO_CYCLES(0,cpu_getscanline()*cpu_getscanlineperiod()));
#else
      amstrad_vh_update_colour(amstrad_GateArray_PenSelected, (dataToGateArray & 0x1F));
#endif
    } break;
/* Select screen mode and rom configuration
   ----------------------------------------
Bit Value Function 
5   x     not used 
4   x     Interrupt generation control 
3   1     Upper rom area disable or 0 Upper rom area enable 
2   1     Lower rom area disable or 0 Lower rom area enable 
1   x     | Mode selection 
0   x     |
   
Screen mode selection : The settings for bits 1 and 0 and the corresponding screen mode are given in the table below. 
-----------------------
b1 b0 Screen mode 
0  0  Mode 0, 160x200 resolution, 16 colours 
0  1  Mode 1, 320x200 resolution, 4 colours 
1  0  Mode 2, 640x200 resolution, 2 colours 
1  1  Mode 3, 160x200 resolution, 4 colours (note 1) 

This mode is not official. From the combinations possible, we can see that 4 modes can be defined, although the Amstrad only has 3. Mode 3 is similar to mode 0, because it has the same resolution, but it is limited to only 4 colours. 
Mode changing is synchronised with HSYNC. If the mode is changed, it will take effect from the next HSYNC. 

Rom configuration selection :
-----------------------------
Bit 2 is used to enable or disable the lower rom area. The lower rom area occupies memory addressess &0000-&3fff and is used to access the operating system rom. When the lower rom area is is enabled, reading from &0000-&3FFF will return data in the rom. When a value is written to &0000-&3FFF, it will be written to the ram underneath the rom. When it is disabled, data read from &0000-&3FFF will return the data in the ram. 
Similarly, bit 3 controls enabling or disabling of the upper rom area. The upper rom area occupies memory addressess &C000-&FFFF and is BASIC or any expansion roms which may be plugged into a rom board/box. See the document on upper rom selection for more details. When the upper rom area enabled, reading from &c000-&ffff, will return data in the rom. When data is written to &c000-&FFFF, it will be written to the ram at the same address as the rom. When the upper rom area is disabled, and data is read from &c000-&ffff the data returned will be the data in the ram. 

Bit 4 controls the interrupt generation. It can be used to delay interrupts.*/
  	case 0x02: {
      int Previous_GateArray_ModeAndRomConfiguration = amstrad_GateArray_ModeAndRomConfiguration;
    	amstrad_GateArray_ModeAndRomConfiguration = dataToGateArray;
/* If bit 4 of the "Select screen mode and rom configuration" register of the Gate-Array is set to "1" 
 then the interrupt request is cleared and the 6-bit counter is reset to "0".  */
  			if ((amstrad_GateArray_ModeAndRomConfiguration & (1<<4)) != 0) {
            amstrad_CRTC_HS_Counter = 0;
  			    cpunum_set_input_line(0,0, CLEAR_LINE);
  			}
/* b3b2 != 0 then change the state of upper or lower rom area and rethink memory */
        if (((amstrad_GateArray_ModeAndRomConfiguration & 0x0C)^(Previous_GateArray_ModeAndRomConfiguration & 0x0C)) != 0) {
          amstrad_setLowerRom();
          amstrad_setUpperRom();
        }
/* b1b0 mode change? */
  			if (((amstrad_GateArray_ModeAndRomConfiguration & 0x03)^(Previous_GateArray_ModeAndRomConfiguration & 0x03)) != 0) {
#ifdef AMSTRAD_VIDEO_EVENT_LIST
          EventList_AddItemOffset((EVENT_LIST_CODE_GA_MODE<<6) , amstrad_GateArray_ModeAndRomConfiguration & 0x03, TIME_TO_CYCLES(0,cpu_getscanline()*cpu_getscanlineperiod()));
#else
  				amstrad_vh_update_mode(amstrad_GateArray_ModeAndRomConfiguration & 0x03);
#endif
  			} 
      }  break;
/* Ram Memory Management
	 ---------------------	
This function is not available in the Gate-Array, but is performed by a device at the same I/O port address location.
In the CPC464,CPC664 and KC compact, this function is performed in a memory-expansion (e.g. Dk'Tronics 64K Ram Expansion), if this expansion is not present then the function is not available.
In the CPC6128, this function is performed by a PAL located on the main PCB, or a memory-expansion.
In the 464+ and 6128+ this function is performed by the ASIC or a memory expansion.
*/
  	case 0x03: {
  			amstrad_GateArray_RamConfiguration = dataToGateArray;
     } break;
  
    default: {
    } break;
  }
}

/* used for loading snapshot only ! */
void AmstradCPC_PALWrite(int data)
{
	if ((data & 0x0c0)==0x0c0)	{
		amstrad_GateArray_RamConfiguration = data;
		AmstradCPC_GA_SetRamConfiguration();
	}
}

/* CRTC Differences
   ----------------
The following tables list the functions that can be accessed for each type: 

Type 0
b1 b0 Function Read/Write 
0  0  Select internal 6845 register Write Only 
0  1  Write to selected internal 6845 register Write Only 
1  0  - - 
1  1  Read from selected internal 6845 register Read only 

Type 1
b1 b0 Function Read/Write 
0  0  Select internal 6845 register Write Only 
0  1  Write to selected internal 6845 register Write Only 
1  0  Read Status Register Read Only 
1  1  Read from selected internal 6845 register Read only 

Type 2
b1 b0 Function Read/Write 
0  0  Select internal 6845 register Write Only 
0  1  Write to selected internal 6845 register Write Only 
1  0  - - 
1  1  Read from selected internal 6845 register Read only 

Type 3 and 4
b1 b0 Function Read/Write 
0  0  Select internal 6845 register Write Only 
0  1  Write to selected internal 6845 register Write Only 
1  0  Read from selected internal 6845 register Read Only 
1  1  Read from selected internal 6845 register Read only 
*/

/* I/O port allocation
   -------------------

Many thanks to Mark Rison for providing the original information. Thankyou to Richard Wilson for his discoveries concerning RAM management I/O decoding. 

This document will explain the decoding of the I/O ports. The port address is not decoded fully which means a hardware device can be accessed through more than one address, in addition, using some addressess can access more than one element of the hardware at the same time. The CPC IN/OUT design differs from the norm in that port numbers are defined using 16 bits, as opposed to the traditional 8 bits. 

IN r,(C)/OUT (C),r instructions: Bits b15-b8 come from the B register, bits b7-b0 come from "r" 
IN A,(n)/OUT (n),A instructions: Bits b15-b8 come from the A register, bits b7-b0 come from "n" 
Listed below are the internal hardware devices and the bit fields to which they respond. In the table: 

"-" means this bit is ignored, 
"0" means the bit must be set to "0" for the hardware device to respond, 
"1" means the bit must be set to "1" for the hardware device to respond. 
"r1" and "r0" mean a bit used to define a register 

Hardware device       Read/Write Port bits 
                                 b15 b14 b13 b12 b11 b10 b9  b8  b7  b6  b5  b4  b3  b2  b1  b0 
Gate-Array            Write Only 0   1   -   -   -   -   -   -   -   -   -   -   -   -   -   - 
RAM Configuration     Write Only 0   -   -   -   -   -   -   -   -   -   -   -   -   -   -   - 
CRTC                  Read/Write -   0   -   -   -   -   r1  r0  -   -   -   -   -   -   -   - 
ROM select            Write only -   -   0   -   -   -   -   -   -   -   -   -   -   -   -   - 
Printer port          Write only -   -   -   0   -   -   -   -   -   -   -   -   -   -   -   - 
8255 PPI              Read/Write -   -   -   -   0   -   r1  r0  -   -   -   -   -   -   -   - 
Expansion Peripherals Read/Write -   -   -   -   -   0   -   -   -   -   -   -   -   -   -   - 

*/

static READ8_HANDLER ( AmstradCPC_ReadPortHandler )
{
	unsigned char data = 0xFF;
	unsigned int r1r0 = (unsigned int)((offset & 0x0300) >> 8);
	m6845_personality_t crtc_type;

	crtc_type = readinputportbytag_safe("crtc", 0);
	crtc6845_set_personality(crtc_type);

	/* if b14 = 0 : CRTC Read selected */
	if ((offset & (1<<14)) == 0)
	{
		switch(r1r0) {
		case 0x02:
			/* CRTC Type 1 : Read Status Register
			   CRTC Type 3 or 4 : Read from selected internal 6845 register */
			switch(crtc_type) {
			case M6845_PERSONALITY_UM6845R:
				data = amstrad_CRTC_CR; /* Read Status Register */
				break;
			case M6845_PERSONALITY_AMS40489:
			case M6845_PERSONALITY_PREASIC:
				data = crtc6845_register_r(0);
				break;
			default:
				break;
			}
			break;
		case 0x03:
			/* All CRTC type : Read from selected internal 6845 register Read only */
			data = crtc6845_register_r(0);
			break;
		}
	}

/* if b11 = 0 : 8255 PPI Read selected - bits 9 and 8 then define the PPI function access as shown below: 

b9 b8 | PPI Function Read/Write status 
0  0  | Port A data  Read/Write 
0  1  | Port B data  Read/Write 
1  0  | Port C data  Read/Write 
1  1  | Control      Write Only
*/    
	if ((offset & (1<<11)) == 0)
	{
		if (r1r0 < 0x03 )
			data = ppi8255_r(0, r1r0);
	}
/* if b10 = 0 : Expansion Peripherals Read selected

bits b7-b5 are used to select an expansion peripheral. Again, this is done by resetting the peripheral's bit:
Bit | Device 
b7  | FDC 
b6  | Reserved (was it ever used?) 
b5  | Serial port 

In the case of the FDC, bits b8 and b0 are used to select the specific mode of operation; all the other bits (b9,b4-b1) are ignored:
b8 b0 Function Read/Write state 
0 0 FDD motor control Write Only 
0 1 not used N/A 
1 0 Main Status register of FDC (MSR) Read Only 
1 1 Data register of FDC Read/Write

If b10 is reset but none of b7-b5 are reset, user expansion peripherals are selected.
The exception is the case where none of b7-b0 are reset (i.e. port &FBFF), which causes the expansion peripherals to reset. 
 */
 	if ((offset & (1<<10)) == 0)	{
		if ((offset & (1<<10)) == 0) {
			int b8b0 = ((offset & (1<<8)) >> (8 - 1)) | (offset & (0x01));
			switch (b8b0) {
  			case 0x02: {
  					data = nec765_status_r(0);
  			} break;
  			case 0x03: {
  					data = nec765_data_r(0);
  			} break;
  			default: {
  			} break;
			}
		}
	}
  return data;
}

/* Offset handler for write */
static WRITE8_HANDLER ( AmstradCPC_WritePortHandler )
{
  if ((offset & (1<<15)) == 0) {
/* if b15 = 0 and b14 = 1 : Gate-Array Write Selected*/
    if ((offset & (1<<14)) != 0) {
	   	amstrad_GateArray_write(data);
    }
/* if b15 = 0 : RAM Configuration Write Selected*/
      AmstradCPC_GA_SetRamConfiguration();
  }
/*The Gate-Array and CRTC can't be selected simultaneously, which would otherwise cause potential display corruption.*/
/* if b14 = 0 : CRTC Write Selected*/
  if ((offset & (1<<14)) == 0) {
		switch ((offset & 0x0300) >> 8) { // r1r0
  		case 0x00: {/* Select internal 6845 register Write Only */
#ifdef AMSTRAD_VIDEO_EVENT_LIST
  			EventList_AddItemOffset((EVENT_LIST_CODE_CRTC_INDEX_WRITE<<6), data, TIME_TO_CYCLES(0,cpu_getscanline()*cpu_getscanlineperiod()));
#endif
        crtc6845_address_w(0,data);
//          selected_crtc6845_address = (data & 0x1F);
      } break;
  		case 0x01: {/* Write to selected internal 6845 register Write Only */
#ifdef AMSTRAD_VIDEO_EVENT_LIST
				EventList_AddItemOffset((EVENT_LIST_CODE_CRTC_WRITE<<6), data, TIME_TO_CYCLES(0,cpu_getscanline()*cpu_getscanlineperiod()));
#endif
//       	  logerror("crtc6845 register (%02d : %04x)\n", selected_crtc6845_address, (data&0x3F));
        crtc6845_register_w(0,data);
  		} break;
  		default: {
  		} break;
		}
	}
/* ROM select before GateArrayWrite ?*/
/* b13 = 0 : ROM select Write Selected*/
	if ((offset & (1<<13)) == 0) {
	  if (previous_amstrad_UpperRom_data != (data & 0xff)) {
      AmstradCPC_SetUpperRom(data);
    }
      previous_amstrad_UpperRom_data = (data & 0xff);
	}
/* b12 = 0 : Printer port Write Selected*/
	if ((offset & (1<<12)) == 0) {
		/* on CPC, write to printer through LS chip */
		/* the amstrad is crippled with a 7-bit port :( */
		/* bit 7 of the data is the printer /strobe */

		/* strobe state changed? */
		if (((previous_printer_data_byte^data) & (1<<7)) !=0 ) {
			/* check for only one transition */
			if ((data & (1<<7)) == 0)  {
				/* output data to printer */
				printer_output(image_from_devtype_and_index(IO_PRINTER, 0), data & 0x07f);
			}
		}
		previous_printer_data_byte = data;
	}
/* if b11 = 0 : 8255 PPI Write selected - bits 9 and 8 then define the PPI function access as shown below: 
b9 b8 | PPI Function Read/Write status 
0  0  | Port A data  Read/Write 
0  1  | Port B data  Read/Write 
1  0  | Port C data  Read/Write 
1  1  | Control      Write Only
*/    
	if ((offset & (1<<11)) == 0) {
		unsigned int Index = ((offset & 0x0300) >> 8);
		ppi8255_w(0, Index, data);
	}
/* if b10 = 0 : Expansion Peripherals Write selected */
	if ((offset & (1<<10)) == 0) {
/* bits b7-b5 are used to select an expansion peripheral. This is done by resetting the peripheral's bit:
Bit | Device 
b7  | FDC 
b6  | Reserved (was it ever used?) 
b5  | Serial port 

In the case of the FDC, bits b8 and b0 are used to select the specific mode of operation; 
all the other bits (b9,b4-b1) are ignored:
b8 b0 Function Read/Write state 
0 0 FDD motor control Write Only 
0 1 not used N/A 
1 0 Main Status register of FDC (MSR) Read Only 
1 1 Data register of FDC Read/Write 

If b10 is reset but none of b7-b5 are reset, user expansion peripherals are selected.
The exception is the case where none of b7-b0 are reset (i.e. port &FBFF), which causes the expansion peripherals to reset. 
*/
    if ((offset & (1<<7)) == 0) {
			unsigned int b8b0 = ((offset & 0x0100) >> (8 - 1)) | (offset & 0x01);

			switch (b8b0) {
			case 0x00:
        /* FDC Motor Control - Bit 0 defines the state of the FDD motor: 
         * "1" the FDD motor will be active. 
         * "0" the FDD motor will be in-active.*/
				floppy_drive_set_motor_state(image_from_devtype_and_index(IO_FLOPPY, 0), (data & 0x01));
				floppy_drive_set_motor_state(image_from_devtype_and_index(IO_FLOPPY, 1), (data & 0x01));
				floppy_drive_set_ready_state(image_from_devtype_and_index(IO_FLOPPY, 0), 1,1);
				floppy_drive_set_ready_state(image_from_devtype_and_index(IO_FLOPPY, 1), 1,1);
		  break;

      case 0x03: /* Write Data register of FDC */
				nec765_data_w(0,data);
			break;
			default:
			break;
			}
		}
	}
	multiface_io_write(offset,data);
}

/******************************************************************************************
	Multiface emulation
  ****************************************************************************************/

static unsigned char *multiface_ram;
static unsigned long multiface_flags;

/* stop button has been pressed */
#define MULTIFACE_STOP_BUTTON_PRESSED	0x0001
/* ram/rom is paged into memory space */
#define MULTIFACE_RAM_ROM_ENABLED		0x0002
/* when visible OUT commands are performed! */
#define MULTIFACE_VISIBLE				0x0004

/* multiface traps calls to 0x0065 when it is active.
This address has a RET and so executes no code.

It is believed that it is used to make multiface invisible to programs */

/*#define MULTIFACE_0065_TOGGLE 				  0x0008*/


/* used to setup computer if a snapshot was specified */
static OPBASE_HANDLER( amstrad_multiface_opbaseoverride )
{
		int pc;

		pc = activecpu_get_pc();

		/* there are two places where CALL &0065 can be found
		in the multiface rom. At this address there is a RET.

		To disable the multiface from being detected, the multiface
		stop button must be pressed, then the program that was stopped
		must be returned to. When this is done, the multiface cannot
		be detected and the out operations to page the multiface
		ram/rom into the address space will not work! */

		/* I assume that the hardware in the multiface detects
		the PC set to 0x065 and uses this to enable/disable the multiface
		*/

		/* I also use this to allow the stop button to be pressed again */
		if (pc==0x0164)
		{
			/* first call? */
			multiface_flags |= MULTIFACE_VISIBLE;
		}
		else if (pc==0x0c98)
		{
		  /* second call */

		  /* no longer visible */
		  multiface_flags &= ~(MULTIFACE_VISIBLE|MULTIFACE_STOP_BUTTON_PRESSED);

		 /* clear op base override */
				memory_set_opbase_handler(0,0);
		}

		return pc;
}

void multiface_init(void)
{
	/* after a reset the multiface is visible */
	multiface_flags = MULTIFACE_VISIBLE;

	/* allocate ram */
	multiface_ram = (unsigned char *)auto_malloc(8192);
}

/* call when a system reset is done */
void multiface_reset(void)
{
		/* stop button not pressed and ram/rom disabled */
		multiface_flags &= ~(MULTIFACE_STOP_BUTTON_PRESSED |
						MULTIFACE_RAM_ROM_ENABLED);
		/* as on the real hardware the multiface is visible after a reset! */
		multiface_flags |= MULTIFACE_VISIBLE;
}

int multiface_hardware_enabled(void)
{
		if (multiface_ram!=NULL)
		{
				if ((readinputportbytag("multiface") & 0x01)!=0)
				{
						return 1;
				}
		}

		return 0;
}

/* simulate the stop button has been pressed */
void	multiface_stop(void)
{
	/* multiface hardware enabled? */
		if (!multiface_hardware_enabled())
		return;

	/* if stop button not already pressed, do press action */
	/* pressing stop button while multiface is running has no effect */
	if ((multiface_flags & MULTIFACE_STOP_BUTTON_PRESSED)==0)
	{
		/* initialise 0065 toggle */
				/*multiface_flags &= ~MULTIFACE_0065_TOGGLE;*/

		multiface_flags |= MULTIFACE_RAM_ROM_ENABLED;

		/* stop button has been pressed, furthur pressess will not issue a NMI */
		multiface_flags |= MULTIFACE_STOP_BUTTON_PRESSED;

		amstrad_GateArray_ModeAndRomConfiguration &=~0x04;

		/* page rom into memory */
		multiface_rethink_memory();

		/* pulse the nmi line */
		cpunum_set_input_line(0, INPUT_LINE_NMI, PULSE_LINE);

		/* initialise 0065 override to monitor calls to 0065 */
		memory_set_opbase_handler(0,amstrad_multiface_opbaseoverride);
	}

}

static void multiface_rethink_memory(void)
{
		unsigned char *multiface_rom;

	/* multiface hardware enabled? */
		if (!multiface_hardware_enabled())
		return;

		multiface_rom = &memory_region(REGION_CPU1)[0x01C000];

	if (
		((multiface_flags & MULTIFACE_RAM_ROM_ENABLED)!=0) &&
		((amstrad_GateArray_ModeAndRomConfiguration & 0x04) == 0)
		)
	{

		/* set bank addressess */
		memory_set_bankptr(1, multiface_rom);
		memory_set_bankptr(2, multiface_ram);
		memory_set_bankptr(9, multiface_rom);
		memory_set_bankptr(10, multiface_ram);
	}
}

/* any io writes are passed through here */
static WRITE8_HANDLER(multiface_io_write)
{
	/* multiface hardware enabled? */
		if (!multiface_hardware_enabled())
		return;

		/* visible? */
	if (multiface_flags & MULTIFACE_VISIBLE)
	{
		if (offset==0x0fee8)
		{
			multiface_flags |= MULTIFACE_RAM_ROM_ENABLED;
			amstrad_rethinkMemory();
		}

		if (offset==0x0feea)
		{
			multiface_flags &= ~MULTIFACE_RAM_ROM_ENABLED;
			amstrad_rethinkMemory();
		}
	}

	/* update multiface ram with data */
		/* these are decoded fully! */
		switch ((offset>>8) & 0x0ff)
		{
				/* gate array */
				case 0x07f:
				{
						switch (data & 0x0c0)
						{
								/* pen index */
								case 0x00:
								{
									multiface_ram[0x01fcf] = data;

								}
								break;

								/* pen colour */
								case 0x040:
								{
									int pen_index;

									pen_index = multiface_ram[0x01fcf] & 0x0f;

									if (multiface_ram[0x01fcf] & 0x010)
									{

										multiface_ram[0x01fdf + pen_index] = data;
									}
									else
									{
										multiface_ram[0x01f90 + pen_index] = data & 0x01f;
									}

								}
								break;

								/* rom/mode selection */
								case 0x080:
								{

									multiface_ram[0x01fef] = data;

								}
								break;

								/* ram configuration */
								case 0x0c0:
								{

									multiface_ram[0x01fff] = data;

								}
								break;

								default:
								  break;

						}

				}
				break;


				/* crtc register index */
				case 0x0bc:
				{
						multiface_ram[0x01cff] = data;
				}
				break;

				/* crtc register write */
				case 0x0bd:
				{
						int reg_index;

						reg_index = multiface_ram[0x01cff] & 0x0f;

						multiface_ram[0x01db0 + reg_index] = data;
				}
				break;


				/* 8255 ppi control */
				case 0x0f7:
				{
				  multiface_ram[0x017ff] = data;

				}
				break;

				/* rom select */
				case 0x0df:
				{
				   multiface_ram[0x01aac] = data;
				}
				break;

				default:
				   break;

		 }

}
/* called when cpu acknowledges int */
/* reset top bit of interrupt line counter */
/* this ensures that the next interrupt is no closer than 32 lines */
static int 	amstrad_cpu_acknowledge_int(int cpu)
{
  cpunum_set_input_line(0,0, CLEAR_LINE);
	amstrad_CRTC_HS_Counter &= 0x1F;
	return 0xFF;
}

static VIDEO_EOF( amstrad )
{
	if (readinputportbytag_safe("multiface", 0) & 0x02)
	{
		multiface_stop();
	}
}
/* sets up for a machine reset
All hardware is reset and the firmware is completely initialized
Once all tables and jumpblocks have been set up,
control is passed to the default entry in rom 0*/
void amstrad_reset_machine(void)
{
	/* enable lower rom (OS rom) */
	amstrad_GateArray_write(0x089);

	/* set ram config 0 */
	amstrad_GateArray_write(0x0c0);

  // Get manufacturer name and TV refresh rate from PCB link (dipswitch for mess emulation)
	ppi_port_inputs[amstrad_ppi_PortB] = (((readinputport(10)&MANUFACTURER_NAME)<<1) | (readinputport(10)&TV_REFRESH_RATE));

	multiface_reset();
}

/* the following timings have been measured! */
static UINT8 amstrad_cycle_table_op[256] = {
	 4, 12,  8,  8,  4,  4,  8,  4,  4, 12,  8,  8,  4,  4,  8,  4,
	12, 12,  8,  8,  4,  4,  8,  4, 12, 12,  8,  8,  4,  4,  8,  4,
	 8, 12, 20,  8,  4,  4,  8,  4,  8, 12, 20,  8,  4,  4,  8,  4,
	 8, 12, 16,  8, 12, 12, 12,  4,  8, 12, 16,  8,  4,  4,  8,  4,
	 4,  4,  4,  4,  4,  4,  8,  4,  4,  4,  4,  4,  4,  4,  8,  4,
	 4,  4,  4,  4,  4,  4,  8,  4,  4,  4,  4,  4,  4,  4,  8,  4,
	 4,  4,  4,  4,  4,  4,  8,  4,  4,  4,  4,  4,  4,  4,  8,  4,
	 8,  8,  8,  8,  8,  8,  4,  8,  4,  4,  4,  4,  4,  4,  8,  4,
	 4,  4,  4,  4,  4,  4,  8,  4,  4,  4,  4,  4,  4,  4,  8,  4,
	 4,  4,  4,  4,  4,  4,  8,  4,  4,  4,  4,  4,  4,  4,  8,  4,
	 4,  4,  4,  4,  4,  4,  8,  4,  4,  4,  4,  4,  4,  4,  8,  4,
	 4,  4,  4,  4,  4,  4,  8,  4,  4,  4,  4,  4,  4,  4,  8,  4,
	 8, 12, 12, 12, 12, 16,  8, 16,  8, 12, 12,  4, 12, 20,  8, 16,
	 8, 12, 12, 12, 12, 16,  8, 16,  8,  4, 12, 12, 12,  4,  8, 16,
	 8, 12, 12, 24, 12, 16,  8, 16,  8,  4, 12,  4, 12,  4,  8, 16,
	 8, 12, 12,  4, 12, 16,  8, 16,  8,  8, 12,  4, 12,  4,  8, 16
};

static UINT8 amstrad_cycle_table_cb[256]=
{
	 4,  4,  4,  4,  4,  4, 12,  4,  4,  4,  4,  4,  4,  4, 12,  4,
	 4,  4,  4,  4,  4,  4, 12,  4,  4,  4,  4,  4,  4,  4, 12,  4,
	 4,  4,  4,  4,  4,  4, 12,  4,  4,  4,  4,  4,  4,  4, 12,  4,
	 4,  4,  4,  4,  4,  4, 12,  4,  4,  4,  4,  4,  4,  4, 12,  4,
	 4,  4,  4,  4,  4,  4,  8,  4,  4,  4,  4,  4,  4,  4,  8,  4,
	 4,  4,  4,  4,  4,  4,  8,  4,  4,  4,  4,  4,  4,  4,  8,  4,
	 4,  4,  4,  4,  4,  4,  8,  4,  4,  4,  4,  4,  4,  4,  8,  4,
	 4,  4,  4,  4,  4,  4,  8,  4,  4,  4,  4,  4,  4,  4,  8,  4,
	 4,  4,  4,  4,  4,  4, 12,  4,  4,  4,  4,  4,  4,  4, 12,  4,
	 4,  4,  4,  4,  4,  4, 12,  4,  4,  4,  4,  4,  4,  4, 12,  4,
	 4,  4,  4,  4,  4,  4, 12,  4,  4,  4,  4,  4,  4,  4, 12,  4,
	 4,  4,  4,  4,  4,  4, 12,  4,  4,  4,  4,  4,  4,  4, 12,  4,
	 4,  4,  4,  4,  4,  4, 12,  4,  4,  4,  4,  4,  4,  4, 12,  4,
	 4,  4,  4,  4,  4,  4, 12,  4,  4,  4,  4,  4,  4,  4, 12,  4,
	 4,  4,  4,  4,  4,  4, 12,  4,  4,  4,  4,  4,  4,  4, 12,  4,
	 4,  4,  4,  4,  4,  4, 12,  4,  4,  4,  4,  4,  4,  4, 12,  4
};


static UINT8 amstrad_cycle_table_ed[256]=
{
	 4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,
	 4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,
	 4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,
	 4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,
	12, 12, 12, 20,  4, 12,  4,  8, 12, 12, 12, 20,  4, 12,  4,  8,
	12, 12, 12, 20,  4, 12,  4,  8, 12, 12, 12, 20,  4, 12,  4,  8,
	12, 12, 12, 20,  4, 12,  4, 16, 12, 12, 12, 20,  4, 12,  4, 16,
	12, 12, 12, 20,  4, 12,  4,  4, 12, 12, 12, 20,  4, 12,  4,  4,
	 4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,
	 4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,
	16, 12, 16, 16,  4,  4,  4,  4, 16, 12, 16, 16,  4,  4,  4,  4,
	16, 12, 16, 16,  4,  4,  4,  4, 16, 12, 16, 16,  4,  4,  4,  4,
	 4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,
	 4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,
	 4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,
	 4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4
};


static UINT8 amstrad_cycle_table_xy[256]=
{
	 4, 12,  8,  8,  4,  4,  8,  4,  4, 12,  8,  8,  4,  4,  8,  4,
	12, 12,  8,  8,  4,  4,  8,  4, 12, 12,  8,  8,  4,  4,  8,  4,
	 8, 12, 20,  8,  4,  4,  8,  4,  8, 12, 20,  8,  4,  4,  8,  4,
	 8, 12, 16,  8, 20, 20, 20,  4,  8, 12, 16,  8,  4,  4,  8,  4,
	 4,  4,  4,  4,  4,  4, 16,  4,  4,  4,  4,  4,  4,  4, 16,  4,
	 4,  4,  4,  4,  4,  4, 16,  4,  4,  4,  4,  4,  4,  4, 16,  4,
	 4,  4,  4,  4,  4,  4, 16,  4,  4,  4,  4,  4,  4,  4, 16,  4,
	16, 16, 16, 16, 16, 16,  4, 16,  4,  4,  4,  4,  4,  4, 16,  4,
	 4,  4,  4,  4,  4,  4, 16,  4,  4,  4,  4,  4,  4,  4, 16,  4,
	 4,  4,  4,  4,  4,  4, 16,  4,  4,  4,  4,  4,  4,  4, 16,  4,
	 4,  4,  4,  4,  4,  4, 16,  4,  4,  4,  4,  4,  4,  4, 16,  4,
	 4,  4,  4,  4,  4,  4, 16,  4,  4,  4,  4,  4,  4,  4, 16,  4,
	 8, 12, 12, 12, 12, 16,  8, 16,  8, 12, 12,  4, 12, 20,  8, 16,
	 8, 12, 12, 12, 12, 16,  8, 16,  8,  4, 12, 12, 12,  4,  8, 16,
	 8, 12, 12, 24, 12, 16,  8, 16,  8,  4, 12,  4, 12,  4,  8, 16,
	 8, 12, 12,  4, 12, 16,  8, 16,  8,  8, 12,  4, 12,  4,  8, 16
};

static UINT8 amstrad_cycle_table_xycb[256]=
{
	20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20,
	20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20,
	20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20,
	20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20,
	16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16,
	16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16,
	16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16,
	16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16,
	20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20,
	20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20,
	20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20,
	20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20,
	20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20,
	20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20,
	20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20,
	20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20
};

static UINT8 amstrad_cycle_table_ex[256]=
{
	 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
	 4,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
	 4,  0,  0,  0,  0,  0,  0,  0,  4,  0,  0,  0,  0,  0,  0,  0,
	 4,  0,  0,  0,  0,  0,  0,  0,  4,  0,  0,  0,  0,  0,  0,  0,
	 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
	 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
	 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
	 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
	 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
	 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
	 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
	 4,  8,  4,  4,  0,  0,  0,  0,  4,  8,  4,  4,  0,  0,  0,  0,
	 8,  0,  0,  0,  8,  0,  0,  0,  8,  0,  0,  0,  8,  0,  0,  0,
	 8,  0,  0,  0,  8,  0,  0,  0,  8,  0,  0,  0,  8,  0,  0,  0,
	 8,  0,  0,  0,  8,  0,  0,  0,  8,  0,  0,  0,  8,  0,  0,  0,
	 8,  0,  0,  0,  8,  0,  0,  0,  8,  0,  0,  0,  8,  0,  0,  0
};

/* every 2us let's the crtc do the job !*/
static void amstrad_update_video_1(int dummy)
{
	amstrad_vh_execute_crtc_cycles(2);
} 

static void amstrad_common_init(void)
{
	amstrad_GateArray_ModeAndRomConfiguration = 0;
	amstrad_CRTC_HS_Counter = 2;
	previous_amstrad_UpperRom_data = 0xff;

	memory_install_read8_handler(0, ADDRESS_SPACE_PROGRAM, 0x0000, 0x1fff, 0, 0, MRA8_BANK1);
	memory_install_read8_handler(0, ADDRESS_SPACE_PROGRAM, 0x2000, 0x3fff, 0, 0, MRA8_BANK2);
	memory_install_read8_handler(0, ADDRESS_SPACE_PROGRAM, 0x4000, 0x5fff, 0, 0, MRA8_BANK3);
	memory_install_read8_handler(0, ADDRESS_SPACE_PROGRAM, 0x6000, 0x7fff, 0, 0, MRA8_BANK4);
	memory_install_read8_handler(0, ADDRESS_SPACE_PROGRAM, 0x8000, 0x9fff, 0, 0, MRA8_BANK5);
	memory_install_read8_handler(0, ADDRESS_SPACE_PROGRAM, 0xa000, 0xbfff, 0, 0, MRA8_BANK6);
	memory_install_read8_handler(0, ADDRESS_SPACE_PROGRAM, 0xc000, 0xdfff, 0, 0, MRA8_BANK7);
	memory_install_read8_handler(0, ADDRESS_SPACE_PROGRAM, 0xe000, 0xffff, 0, 0, MRA8_BANK8);

	memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM, 0x0000, 0x1fff, 0, 0, MWA8_BANK9);
	memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM, 0x2000, 0x3fff, 0, 0, MWA8_BANK10);
	memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM, 0x4000, 0x5fff, 0, 0, MWA8_BANK11);
	memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM, 0x6000, 0x7fff, 0, 0, MWA8_BANK12);
	memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM, 0x8000, 0x9fff, 0, 0, MWA8_BANK13);
	memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM, 0xa000, 0xbfff, 0, 0, MWA8_BANK14);
	memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM, 0xc000, 0xdfff, 0, 0, MWA8_BANK15);
	memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM, 0xe000, 0xffff, 0, 0, MWA8_BANK16);

	cpunum_reset(0);
	cpunum_set_input_line_vector(0, 0,0x0ff);

	nec765_init(&amstrad_nec765_interface,NEC765A/*?*/);
	ppi8255_init(&amstrad_ppi8255_interface);

	floppy_drive_set_geometry(image_from_devtype_and_index(IO_FLOPPY, 0),  FLOPPY_DRIVE_SS_40);
	floppy_drive_set_geometry(image_from_devtype_and_index(IO_FLOPPY, 1),  FLOPPY_DRIVE_SS_40);

/* Every microsecond: 

The CRTC generates a memory address using it's MA and RA signal outputs 
The Gate-Array fetches two bytes for each address*/

//	timer_pulse(TIME_IN_USEC(AMSTRAD_US_PER_SCANLINE), 0, amstrad_vh_execute_crtc_cycles);
	timer_pulse(TIME_IN_USEC(1), 0, amstrad_vh_execute_crtc_cycles);

	/* The opcode timing in the Amstrad is different to the opcode
	timing in the core for the Z80 CPU.

	The Amstrad hardware issues a HALT for each memory fetch.
	This has the effect of stretching the timing for Z80 opcodes,
	so that they are all multiple of 4 T states long. All opcode
	timings are a multiple of 1us in length. */

	/* Using the cool code Juergen has provided, I will override
	the timing tables with the values for the amstrad */
	cpunum_set_info_ptr(0,CPUINFO_PTR_Z80_CYCLE_TABLE+Z80_TABLE_op, amstrad_cycle_table_op);
	cpunum_set_info_ptr(0,CPUINFO_PTR_Z80_CYCLE_TABLE+Z80_TABLE_cb, amstrad_cycle_table_cb);
	cpunum_set_info_ptr(0,CPUINFO_PTR_Z80_CYCLE_TABLE+Z80_TABLE_ed, amstrad_cycle_table_ed);
	cpunum_set_info_ptr(0,CPUINFO_PTR_Z80_CYCLE_TABLE+Z80_TABLE_xy, amstrad_cycle_table_xy);
	cpunum_set_info_ptr(0,CPUINFO_PTR_Z80_CYCLE_TABLE+Z80_TABLE_xycb, amstrad_cycle_table_xycb);
	cpunum_set_info_ptr(0,CPUINFO_PTR_Z80_CYCLE_TABLE+Z80_TABLE_ex, amstrad_cycle_table_ex);

	/* Juergen is a cool dude! */
	cpunum_set_irq_callback(0, amstrad_cpu_acknowledge_int);
}

static MACHINE_RESET( amstrad )
{
	int i;

	for (i=0; i<256; i++)
	{
		Amstrad_ROM_Table[i] = &memory_region(REGION_CPU1)[0x014000];
	}
	
	Amstrad_ROM_Table[7] = &memory_region(REGION_CPU1)[0x018000];
	amstrad_common_init();
	amstrad_reset_machine();

	multiface_init();
	
}

static MACHINE_RESET( kccomp )
{
	int i;

	for (i=0; i<256; i++)
	{
		Amstrad_ROM_Table[i] = &memory_region(REGION_CPU1)[0x014000];
	}

	amstrad_common_init();
	amstrad_reset_machine();

	/* bit 1 = /TEST. When 0, KC compact will enter data transfer
	sequence, where another system using the expansion port signals
	DATA2,DATA1, /STROBE and DATA7 can transfer 256 bytes of program.
	When the program has been transfered, it will be executed. This
	is not supported in the driver */
	/* bit 3,4 are tied to +5V, bit 2 is tied to 0V */
	ppi_port_inputs[amstrad_ppi_PortB] = (1<<4) | (1<<3) | 2;
}


/* Memory is banked in 16k blocks. However, the multiface
pages the memory in 8k blocks! The ROM can
be paged into bank 0 and bank 3. */
static ADDRESS_MAP_START(amstrad_mem, ADDRESS_SPACE_PROGRAM, 8)
	AM_RANGE(0x00000, 0x01fff) AM_READWRITE(MRA8_BANK1, MWA8_BANK9)
	AM_RANGE(0x02000, 0x03fff) AM_READWRITE(MRA8_BANK2, MWA8_BANK10)
	AM_RANGE(0x04000, 0x05fff) AM_READWRITE(MRA8_BANK3, MWA8_BANK11)
	AM_RANGE(0x06000, 0x07fff) AM_READWRITE(MRA8_BANK4, MWA8_BANK12)
	AM_RANGE(0x08000, 0x09fff) AM_READWRITE(MRA8_BANK5, MWA8_BANK13)
	AM_RANGE(0x0a000, 0x0bfff) AM_READWRITE(MRA8_BANK6, MWA8_BANK14)
	AM_RANGE(0x0c000, 0x0dfff) AM_READWRITE(MRA8_BANK7, MWA8_BANK15)
	AM_RANGE(0x0e000, 0x0ffff) AM_READWRITE(MRA8_BANK8, MWA8_BANK16)
ADDRESS_MAP_END

/* I've handled the I/O ports in this way, because the ports
are not fully decoded by the CPC h/w. Doing it this way means
I can decode it myself and a lot of  software should work */
static ADDRESS_MAP_START(amstrad_io, ADDRESS_SPACE_IO, 8)
	AM_RANGE(0x0000, 0xffff) AM_READWRITE(AmstradCPC_ReadPortHandler, AmstradCPC_WritePortHandler)
ADDRESS_MAP_END

/* Additional notes for the AY-3-8912 in the CPC design
Port A of the AY-3-8912 is connected to the keyboard.
The data for a selected keyboard line can be read through Port A, as long as it is defined as input.
The operating system and I believe all programs assume this port has been defined as input. (NWC has found a bug in the Multiface 2 software. The Multiface does not reprogram the input/output state of the AY-3-8912's registers, therefore if port A is programmed as output, the keyboard will be unresponsive and it will not be possible to use the Multiface functions.) 
When port B is defined as input (bit 7 of register 7 is set to "0"), a read of this port will return &FF. 
*/

/* read PSG port A */
static READ8_HANDLER ( amstrad_psg_porta_read )
{	
/* Read CPC Keyboard
   If keyboard matrix line 11-14 are selected, the byte is always &ff.
   After testing on a real CPC, it is found that these never change, they always return &FF. */

	if (amstrad_keyboard_line > 9) {
    return 0xFF;
  } else {
    int amstrad_read_keyboard_line = amstrad_keyboard_line;
    amstrad_keyboard_line = 0xFF;
    return (readinputport(amstrad_read_keyboard_line) & 0xFF);
  }
}

static INPUT_PORTS_START( amstrad_keyboard )
	/* keyboard row 0 */
	PORT_START
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("\x18") PORT_CODE(KEYCODE_UP) PORT_CHAR(UCHAR_MAMEKEY(UP))
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("\x1a") PORT_CODE(KEYCODE_RIGHT) PORT_CHAR(UCHAR_MAMEKEY(RIGHT))
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("\x19") PORT_CODE(KEYCODE_DOWN) PORT_CHAR(UCHAR_MAMEKEY(DOWN))
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Keypad 9") PORT_CODE(KEYCODE_9_PAD) PORT_CHAR(UCHAR_MAMEKEY(9_PAD))
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Keypad 6") PORT_CODE(KEYCODE_6_PAD) PORT_CHAR(UCHAR_MAMEKEY(6_PAD))
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Keypad 3") PORT_CODE(KEYCODE_3_PAD) PORT_CHAR(UCHAR_MAMEKEY(3_PAD))
	PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Keypad Enter") PORT_CODE(KEYCODE_ENTER_PAD) PORT_CHAR(UCHAR_MAMEKEY(ENTER_PAD))
	PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Keypad .") PORT_CODE(KEYCODE_DEL_PAD) PORT_CHAR(UCHAR_MAMEKEY(DEL_PAD))

	/* keyboard line 1 */
	PORT_START
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("\x1b") PORT_CODE(KEYCODE_LEFT) PORT_CHAR(UCHAR_MAMEKEY(LEFT))
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Copy") PORT_CODE(KEYCODE_LALT)
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Keypad 7") PORT_CODE(KEYCODE_7_PAD) PORT_CHAR(UCHAR_MAMEKEY(7_PAD))
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Keypad 8") PORT_CODE(KEYCODE_8_PAD) PORT_CHAR(UCHAR_MAMEKEY(8_PAD))
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Keypad 5") PORT_CODE(KEYCODE_5_PAD) PORT_CHAR(UCHAR_MAMEKEY(5_PAD))
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Keypad 1") PORT_CODE(KEYCODE_1_PAD) PORT_CHAR(UCHAR_MAMEKEY(1_PAD))
	PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Keypad 2") PORT_CODE(KEYCODE_2_PAD) PORT_CHAR(UCHAR_MAMEKEY(2_PAD))
	PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Keypad 0") PORT_CODE(KEYCODE_0_PAD) PORT_CHAR(UCHAR_MAMEKEY(0_PAD)) 

	/* keyboard row 2 */
	PORT_START
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Clr") PORT_CODE(KEYCODE_HOME) PORT_CHAR(UCHAR_MAMEKEY(HOME))
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_ASTERISK) PORT_CHAR('@') PORT_CHAR('|')
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Enter") PORT_CODE(KEYCODE_ENTER) PORT_CHAR(13)
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_CLOSEBRACE) PORT_CHAR(']')
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Keypad 4") PORT_CODE(KEYCODE_4_PAD) PORT_CHAR(UCHAR_MAMEKEY(4_PAD))
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Left Shift") PORT_CODE(KEYCODE_LSHIFT) PORT_CHAR(UCHAR_SHIFT_1)
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Right Shift") PORT_CODE(KEYCODE_RSHIFT) PORT_CHAR(UCHAR_SHIFT_1)
	PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_BACKSLASH) PORT_CHAR('\\')
	PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Ctrl") PORT_CODE(KEYCODE_LCONTROL) PORT_CHAR(UCHAR_SHIFT_2)

	/* keyboard row 3 */
	PORT_START
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("^ \xa3") PORT_CODE(KEYCODE_SLASH_PAD) PORT_CHAR('^') PORT_CHAR('\xa3')
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_MINUS) PORT_CHAR('-') PORT_CHAR('=')
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_OPENBRACE) PORT_CHAR('[')
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_P) PORT_CHAR('P')
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_QUOTE) PORT_CHAR(';') PORT_CHAR('+')
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_COLON) PORT_CHAR(':') PORT_CHAR('*')
	PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_SLASH) PORT_CHAR('/') PORT_CHAR('?')
	PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_STOP) PORT_CHAR('.') PORT_CHAR('>')

	/* keyboard line 4 */
	PORT_START
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_0) PORT_CHAR('0') PORT_CHAR('_')
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_9) PORT_CHAR('9') PORT_CHAR(')')
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_O) PORT_CHAR('O')
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_I) PORT_CHAR('I')
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_L) PORT_CHAR('L')
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_K) PORT_CHAR('K')
	PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_M) PORT_CHAR('M')
	PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_COMMA) PORT_CODE(',') PORT_CHAR('<')

	/* keyboard line 5 */
	PORT_START
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_8) PORT_CHAR('8') PORT_CHAR('(')
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_7) PORT_CHAR('7') PORT_CHAR('\'')
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_U) PORT_CHAR('U')
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_Y) PORT_CHAR('Y')
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_H) PORT_CHAR('H')
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_J) PORT_CHAR('J')
	PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_N) PORT_CHAR('N')
	PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Space") PORT_CODE(KEYCODE_SPACE) PORT_CHAR(' ')

	/* keyboard line 6 */
	PORT_START
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_6) PORT_CHAR('6') PORT_CHAR('&')
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_5) PORT_CHAR('5') PORT_CHAR('%')
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_R) PORT_CHAR('R')
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_T) PORT_CHAR('T')
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_G) PORT_CHAR('G')
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_F) PORT_CHAR('F')
	PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_B) PORT_CHAR('B')
	PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_V) PORT_CHAR('V')

	/* keyboard line 7 */
	PORT_START
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_4) PORT_CHAR('4') PORT_CHAR('$')
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_3) PORT_CHAR('3') PORT_CHAR('#')
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_E) PORT_CHAR('E')
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_W) PORT_CHAR('W')
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_S) PORT_CHAR('S')
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_D) PORT_CHAR('D')
	PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_C) PORT_CHAR('C')
	PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_X) PORT_CHAR('X')

	/* keyboard line 8 */
	PORT_START
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_1) PORT_CHAR('1') PORT_CHAR('!')
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_2) PORT_CHAR('2') PORT_CHAR('\"')
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Esc") PORT_CODE(KEYCODE_ESC) PORT_CHAR(26)
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_Q) PORT_CHAR('Q')
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Tab") PORT_CODE(KEYCODE_TAB) PORT_CHAR(9)
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_A) PORT_CHAR('A')
	PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Caps Lock") PORT_CODE(KEYCODE_CAPSLOCK)
	PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_Z) PORT_CHAR('Z')

	/* keyboard line 9 */
	PORT_START
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP) PORT_CODE(JOYCODE_1_UP)	 PORT_PLAYER(1)
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN) PORT_CODE(JOYCODE_1_DOWN)	 PORT_PLAYER(1)
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT) PORT_CODE(JOYCODE_1_LEFT)	 PORT_PLAYER(1)
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT) PORT_CODE(JOYCODE_1_RIGHT)	 PORT_PLAYER(1)
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_BUTTON1) PORT_CODE(JOYCODE_1_BUTTON1)	 PORT_PLAYER(1)
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_BUTTON2) PORT_CODE(JOYCODE_1_BUTTON2)	 PORT_PLAYER(1)
	PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_BUTTON3) PORT_CODE(JOYCODE_1_BUTTON3)	 PORT_PLAYER(1)
	PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Del") PORT_CODE(KEYCODE_BACKSPACE) PORT_CHAR(8)
INPUT_PORTS_END


/* Steph 2000-10-27	I remapped the 'Machine Name' Dip Switches (easier to understand) */

INPUT_PORTS_START(amstrad)

	PORT_INCLUDE( amstrad_keyboard )

/* the following are defined as dipswitches, but are in fact solder links on the
 * curcuit board. The links are open or closed when the PCB is made, and are set depending on which country
 * the Amstrad system was to go to

b3 b2 b1 Manufacturer Name (CPC and CPC+ only):
0  0  0  Isp 
0  0  1  Triumph 
0  1  0  Saisho 
0  1  1  Solavox 
1  0  0  Awa 
1  0  1  Schneider 
1  1  0  Orion 
1  1  1  Amstrad*/
PORT_START
	PORT_DIPNAME( 0x07, 0x07, "Manufacturer Name" )
	PORT_DIPSETTING(    0x00, "Isp" )
	PORT_DIPSETTING(    0x01, "Triumph" )
	PORT_DIPSETTING(    0x02, "Saisho" )
	PORT_DIPSETTING(    0x03, "Solavox" )
	PORT_DIPSETTING(    0x04, "Awa" )
	PORT_DIPSETTING(    0x05, "Schneider" )
	PORT_DIPSETTING(    0x06, "Orion" )
	PORT_DIPSETTING(    0x07, "Amstrad" )

	PORT_DIPNAME(    0x10, 0x10, "TV Refresh Rate" )
	PORT_DIPSETTING(    0x00, "60 Hz" )
	PORT_DIPSETTING(    0x10, "50 Hz" )

/* Part number Manufacturer Type number
   UM6845      UMC          0 
   HD6845S     Hitachi      0 
   UM6845R     UMC          1 
   MC6845      Motorola     2 
   AMS40489    Amstrad      3
   Pre-ASIC??? Amstrad?     4 In the "cost-down" CPC6128, the CRTC functionality is integrated into a single ASIC IC. This ASIC is often refered to as the "Pre-ASIC" because it preceeded the CPC+ ASIC
As far as I know, the KC compact used HD6845S only. 
*/
	PORT_START_TAG("crtc")
	PORT_DIPNAME( 0xFF, M6845_PERSONALITY_UM6845R, "CRTC Type" )
	PORT_DIPSETTING(M6845_PERSONALITY_UM6845, "Type 0 - UM6845" )
	PORT_DIPSETTING(M6845_PERSONALITY_HD6845S, "Type 0 - HD6845S" )
	PORT_DIPSETTING(M6845_PERSONALITY_UM6845R, "Type 1 - UM6845R" )
	PORT_DIPSETTING(M6845_PERSONALITY_GENUINE, "Type 2 - MC6845" )
	PORT_DIPSETTING(M6845_PERSONALITY_AMS40489, "Type 3 - AMS40489" )
	PORT_DIPSETTING(M6845_PERSONALITY_PREASIC, "Type 4 - Pre-ASIC???" )

	PORT_START_TAG("multiface")
	PORT_CONFNAME(0x01, 0x00, "Multiface Hardware" )
	PORT_CONFSETTING(0x00, DEF_STR( Off) )
	PORT_CONFSETTING(0x01, DEF_STR( On) )
	PORT_BIT(0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Multiface Stop") PORT_CODE(KEYCODE_F1)

	PORT_START_TAG("green_display")
	PORT_CONFNAME( 0x01, 0x00, "Color/Green Display" )
	PORT_CONFSETTING(0x00, "Color Display" )
	PORT_CONFSETTING(0x01, "Green Display" )

INPUT_PORTS_END

INPUT_PORTS_START(kccomp)
	PORT_INCLUDE( amstrad_keyboard )
INPUT_PORTS_END

/* --------------------
   - AY8910_interface -
   --------------------*/
static struct AY8910interface ay8912_interface =
{
	amstrad_psg_porta_read,	/* portA read */
	amstrad_psg_porta_read,	/* portB read */
	NULL,					/* portA write */
	NULL					/* portB write */
};



/* actual clock to CPU is 4Mhz, but it is slowed by memory
accessess. A HALT is used for every memory access by the CPU.
This stretches the timing for opcodes, and gives an effective
speed of 3.8Mhz */

/* Info about structures below:

	The Amstrad has a CPU running at 4Mhz, slowed with wait states.
	I have measured 19968 NOP instructions per frame, which gives,
	50.08 fps as the tv refresh rate.

  There are 312 lines on a PAL screen, giving 64us per line.

  There is only 50us visible per line, and 35*8 lines visible on the screen.

  This is the reason why the displayed area is not the same as the visible area.
 */

static MACHINE_DRIVER_START( amstrad )
	/* Machine hardware */
	MDRV_CPU_ADD(Z80, 4000000)
	MDRV_CPU_PROGRAM_MAP(amstrad_mem, 0)
	MDRV_CPU_IO_MAP(amstrad_io, 0)

	MDRV_FRAMES_PER_SECOND(AMSTRAD_FPS)
	MDRV_INTERLEAVE(1)
	MDRV_VBLANK_DURATION(19968)

	MDRV_MACHINE_RESET( amstrad )

    /* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER | VIDEO_PIXEL_ASPECT_RATIO_1_2)
	MDRV_SCREEN_SIZE(800, 312) 
	/* Amstrad Monitor Visible AREA : 768x272 */
	MDRV_VISIBLE_AREA(0, ((AMSTRAD_SCREEN_WIDTH-32) - 1), 0, ((AMSTRAD_SCREEN_HEIGHT-40) - 1))
	MDRV_PALETTE_LENGTH(32)
	MDRV_COLORTABLE_LENGTH(32)
	MDRV_PALETTE_INIT( amstrad_cpc )

	MDRV_VIDEO_START( amstrad )
	MDRV_VIDEO_UPDATE( amstrad )
	MDRV_VIDEO_EOF( amstrad )

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")
	MDRV_SOUND_ADD(WAVE, 0)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.50)
	MDRV_SOUND_ADD(AY8910, 1000000)
	MDRV_SOUND_CONFIG(ay8912_interface)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.25)	
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( kccomp )
	MDRV_IMPORT_FROM( amstrad )
	MDRV_FRAMES_PER_SECOND( AMSTRAD_FPS )
	MDRV_MACHINE_RESET( kccomp )
	MDRV_SCREEN_SIZE(800, 312)
	MDRV_PALETTE_INIT( kccomp )
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( cpcplus )
	MDRV_IMPORT_FROM( amstrad )
	MDRV_FRAMES_PER_SECOND( AMSTRAD_FPS )
	MDRV_SCREEN_SIZE(800, 312)
	MDRV_PALETTE_LENGTH(4096)
	MDRV_COLORTABLE_LENGTH(4096)
	MDRV_PALETTE_INIT( amstrad_plus )
MACHINE_DRIVER_END


/***************************************************************************

  Game driver(s)

***************************************************************************/

/* cpc6128.rom contains OS in first 16k, BASIC in second 16k */
/* cpcados.rom contains Amstrad DOS */


static void cpc6128_floppy_getinfo(const device_class *devclass, UINT32 state, union devinfo *info)
{
	/* floppy */
	switch(state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case DEVINFO_INT_COUNT:							info->i = 2; break;

		default:										legacydsk_device_getinfo(devclass, state, info); break;
	}
}

static void cpc6128_cassette_getinfo(const device_class *devclass, UINT32 state, union devinfo *info)
{
	/* cassette */
	switch(state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case DEVINFO_INT_COUNT:							info->i = 1; break;
		case DEVINFO_INT_CASSETTE_DEFAULT_STATE:		info->i = CASSETTE_STOPPED | CASSETTE_SPEAKER_ENABLED; break;

		default:										cassette_device_getinfo(devclass, state, info); break;
	}
}

static void cpc6128_printer_getinfo(const device_class *devclass, UINT32 state, union devinfo *info)
{
	/* printer */
	switch(state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case DEVINFO_INT_COUNT:							info->i = 1; break;

		default:										printer_device_getinfo(devclass, state, info); break;
	}
}

SYSTEM_CONFIG_START(cpc6128)
	CONFIG_RAM_DEFAULT(128 * 1024)
	CONFIG_DEVICE(cpc6128_floppy_getinfo)
	CONFIG_DEVICE(cpc6128_cassette_getinfo)
	CONFIG_DEVICE(cpc6128_printer_getinfo)
SYSTEM_CONFIG_END

static void cpcplus_cartslot_getinfo(const device_class *devclass, UINT32 state, union devinfo *info)
{
	/* cartslot */
	switch(state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case DEVINFO_INT_COUNT:							info->i = 1; break;
		case DEVINFO_INT_MUST_BE_LOADED:				info->i = 1; break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case DEVINFO_PTR_LOAD:							info->load = device_load_amstrad_plus_cartridge; break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case DEVINFO_STR_FILE_EXTENSIONS:				strcpy(info->s = device_temp_str(), "cpr"); break;

		default:										cartslot_device_getinfo(devclass, state, info); break;
	}
}

static void cpcplus_snapshot_getinfo(const device_class *devclass, UINT32 state, union devinfo *info)
{
	/* snapshot */
	switch(state)
	{
		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case DEVINFO_STR_FILE_EXTENSIONS:				strcpy(info->s = device_temp_str(), "sna"); break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case DEVINFO_PTR_SNAPSHOT_LOAD:					info->f = (genf *) snapshot_load_amstrad; break;

		default:										snapshot_device_getinfo(devclass, state, info); break;
	}
}

SYSTEM_CONFIG_START(cpcplus)
	CONFIG_IMPORT_FROM(cpc6128)
	CONFIG_DEVICE(cpcplus_cartslot_getinfo)
	CONFIG_DEVICE(cpcplus_snapshot_getinfo)
SYSTEM_CONFIG_END


/* ---------------------------------------------
   - Rom definition for the ROM loading system -
   ---------------------------------------------*/
/* I am loading the roms outside of the Z80 memory area, because they
are banked. */
ROM_START(cpc6128)
	/* this defines the total memory size - 64k ram, 16k OS, 16k BASIC, 16k DOS */
	ROM_REGION(0x020000, REGION_CPU1,0)
	/* load the os to offset 0x01000 from memory base */
	ROM_LOAD("cpc6128.rom", 0x10000, 0x8000, CRC(9e827fe1) SHA1(5977adbad3f7c1e0e082cd02fe76a700d9860c30))
	ROM_LOAD("cpcados.rom", 0x18000, 0x4000, CRC(1fe22ecd) SHA1(39102c8e9cb55fcc0b9b62098780ed4a3cb6a4bb))

	/* optional Multiface hardware */
		ROM_LOAD_OPTIONAL("multface.rom", 0x01c000, 0x02000, CRC(f36086de) SHA1(1431ec628d38f000715545dd2186b684c5fe5a6f))
ROM_END

ROM_START(cpc6128f)

/* this defines the total memory size (128kb))- 64k ram, 16k OS, 16k BASIC, 16k DOS +16k*/
	ROM_REGION(0x020000, REGION_CPU1,0)

/* load the os to offset 0x01000 from memory base */
	ROM_LOAD("cpc6128f.rom", 0x10000, 0x8000, CRC(1574923b) SHA1(200d59076dfef36db061d6d7d21d80021cab1237))
	ROM_LOAD("cpcados.rom", 0x018000, 0x4000, CRC(1fe22ecd) SHA1(39102c8e9cb55fcc0b9b62098780ed4a3cb6a4bb))

	/* optional Multiface hardware */
	ROM_LOAD_OPTIONAL("multface.rom", 0x01c000, 0x2000, CRC(f36086de) SHA1(1431ec628d38f000715545dd2186b684c5fe5a6f))
ROM_END

ROM_START(cpc464)
	/* this defines the total memory size - 64k ram, 16k OS, 16k BASIC, 16k DOS */
	ROM_REGION(0x01c000, REGION_CPU1,0)
	/* load the os to offset 0x01000 from memory base */
	ROM_LOAD("cpc464.rom", 0x10000, 0x8000, CRC(40852f25) SHA1(56d39c463da60968d93e58b4ba0e675829412a20))
	ROM_LOAD("cpcados.rom", 0x18000, 0x4000, CRC(1fe22ecd) SHA1(39102c8e9cb55fcc0b9b62098780ed4a3cb6a4bb))
ROM_END

ROM_START(cpc664)
	/* this defines the total memory size - 64k ram, 16k OS, 16k BASIC, 16k DOS */
	ROM_REGION(0x01c000, REGION_CPU1,0)
	/* load the os to offset 0x01000 from memory base */
	ROM_LOAD("cpc664.rom", 0x10000, 0x8000, CRC(9AB5A036) SHA1(073a7665527b5bd8a148747a3947dbd3328682c8))
	ROM_LOAD("cpcados.rom", 0x18000, 0x4000, CRC(1fe22ecd) SHA1(39102c8e9cb55fcc0b9b62098780ed4a3cb6a4bb))
ROM_END


ROM_START(kccomp)
	ROM_REGION(0x018000, REGION_CPU1,0)
	ROM_LOAD("kccos.rom", 0x10000, 0x04000, CRC(7f9ab3f7) SHA1(f828045e98e767f737fd93df0af03917f936ad08))
	ROM_LOAD("kccbas.rom", 0x14000, 0x04000, CRC(ca6af63d) SHA1(d7d03755099d0aff501fa5fffc9c0b14c0825448))
	ROM_REGION(0x018000+0x0800, REGION_PROMS, 0 )
	ROM_LOAD("farben.rom", 0x018000, 0x0800, CRC(a50fa3cf) SHA1(2f229ac9f62d56973139dad9992c208421bc0f51))

	/* fake region - required by graphics decode structure */
	/*ROM_REGION(0x0c00, REGION_GFX1) */
ROM_END


/* this system must have a cartridge installed to run */
ROM_START(cpc6128p)
	ROM_REGION(0, REGION_CPU1,0)
ROM_END


/* this system must have a cartridge installed to run */
ROM_START(cpc464p)
	ROM_REGION(0, REGION_CPU1,0)
ROM_END

/*      YEAR  NAME    PARENT	COMPAT  MACHINE    INPUT    INIT    CONFIG,  COMPANY               FULLNAME */
COMP( 1984, cpc464,   0,		0,		amstrad,  amstrad,	0,		cpc6128, "Amstrad plc", "Amstrad/Schneider CPC464", 0)
COMP( 1985, cpc664,   cpc464,	0,		amstrad,  amstrad,	0,	    cpc6128, "Amstrad plc", "Amstrad/Schneider CPC664", 0)
COMP( 1985, cpc6128,  cpc464,	0,		amstrad,  amstrad,	0,	    cpc6128, "Amstrad plc", "Amstrad/Schneider CPC6128", 0)
COMP( 1985, cpc6128f, cpc464,   0,      amstrad,  amstrad, 0, cpc6128, "Amstrad plc", "Amstrad/Schneider CPC6128 Azerty French Keyboard", 0)
COMP( 1990, cpc464p,  0,		0,		cpcplus,  amstrad,	0,	    cpcplus, "Amstrad plc", "Amstrad 464plus", 0)
COMP( 1990, cpc6128p, 0,		0,		cpcplus,  amstrad,	0,	    cpcplus, "Amstrad plc", "Amstrad 6128plus", 0)
COMP( 1989, kccomp,   cpc464,	0,		kccomp,   kccomp,	0,	    cpc6128, "VEB Mikroelektronik", "KC Compact", 0)

