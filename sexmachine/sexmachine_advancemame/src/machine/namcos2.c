/***************************************************************************

Namco System II

  machine.c

  Functions to emulate general aspects of the machine (RAM, ROM, interrupts,
  I/O ports)

***************************************************************************/

#include "driver.h"
#include "cpu/m6809/m6809.h"
#include "cpu/m6805/m6805.h"
#include "namcos2.h"

int namcos2_gametype;

static unsigned mFinalLapProtCount;

READ16_HANDLER( namcos2_flap_prot_r )
{
	static const UINT16 table0[8] = { 0x0000,0x0040,0x0440,0x2440,0x2480,0xa080,0x8081,0x8041 };
	static const UINT16 table1[8] = { 0x0040,0x0060,0x0060,0x0860,0x0864,0x08e4,0x08e5,0x08a5 };
	UINT16 data;

	switch( offset )
	{
	case 0:
		data = 0x0101;
		break;

	case 1:
		data = 0x3e55;
		break;

	case 2:
		data = table1[mFinalLapProtCount&7];
		data = (data&0xff00)>>8;
		break;

	case 3:
		data = table1[mFinalLapProtCount&7];
		mFinalLapProtCount++;
		data = data&0x00ff;
		break;

	case 0x3fffc/2:
		data = table0[mFinalLapProtCount&7];
		data = data&0xff00;
		break;

	case 0x3fffe/2:
		data = table0[mFinalLapProtCount&7];
		mFinalLapProtCount++;
		data = (data&0x00ff)<<8;
		break;

	default:
		data = 0;
	}
	return data;
}

/*************************************************************/
/* Perform basic machine initialisation                      */
/*************************************************************/

MACHINE_RESET( namcos2 ){
	int loop;

	mFinalLapProtCount = 0;

	/* Initialise the bank select in the sound CPU */
	namcos2_sound_bankselect_w(0,0); /* Page in bank 0 */

	/* Place CPU2 & CPU3 into the reset condition */
	cpunum_set_input_line(CPU_SOUND, INPUT_LINE_RESET, ASSERT_LINE);
	cpunum_set_input_line(CPU_SLAVE, INPUT_LINE_RESET, ASSERT_LINE);
	cpunum_set_input_line(CPU_MCU, INPUT_LINE_RESET, ASSERT_LINE);

	/* Initialise interrupt handlers */
	for(loop=0;loop<20;loop++)
	{
		namcos2_68k_master_C148[loop]=0;
		namcos2_68k_slave_C148[loop]=0;
	}
}

/*************************************************************/
/* EEPROM Load/Save and read/write handling                  */
/*************************************************************/

UINT16 *namcos2_eeprom;
size_t namcos2_eeprom_size;

NVRAM_HANDLER( namcos2 )
{
	if( read_or_write )
	{
		mame_fwrite( file, namcos2_eeprom, namcos2_eeprom_size );
	}
	else
	{
		if( file )
		{
			mame_fread( file, namcos2_eeprom, namcos2_eeprom_size );
		}
		else
		{
			int pat = 0xff; /* default */
			if( namcos2_gametype == NAMCOS21_STARBLADE )
			{
				pat = 0x00;
			}
			memset( namcos2_eeprom, pat, namcos2_eeprom_size );
		}
	}
}

WRITE16_HANDLER( namcos2_68k_eeprom_w ){
	COMBINE_DATA( &namcos2_eeprom[offset] );
}

READ16_HANDLER( namcos2_68k_eeprom_r ){
	return namcos2_eeprom[offset];
}

/*************************************************************/
/* 68000 Shared memory area - Data ROM area                  */
/*************************************************************/
READ16_HANDLER( namcos2_68k_data_rom_r ){
	UINT16 *ROM = (UINT16 *)memory_region(REGION_USER1);
	return ROM[offset];
}



/**************************************************************/
/* 68000 Shared serial communications processor (CPU5?)       */
/**************************************************************/

UINT16  namcos2_68k_serial_comms_ctrl[0x8];
UINT16 *namcos2_68k_serial_comms_ram;

READ16_HANDLER( namcos2_68k_serial_comms_ram_r ){
	return namcos2_68k_serial_comms_ram[offset];
}

WRITE16_HANDLER( namcos2_68k_serial_comms_ram_w ){
	COMBINE_DATA( &namcos2_68k_serial_comms_ram[offset] );
}

READ16_HANDLER( namcos2_68k_serial_comms_ctrl_r )
{
	UINT16 retval = namcos2_68k_serial_comms_ctrl[offset];

	switch(offset){
	case 0x00:
		retval |= 0x0004; 	/* Set READY? status bit */
		break;

	default:
		break;
	}
	return retval;
}

WRITE16_HANDLER( namcos2_68k_serial_comms_ctrl_w )
{
	COMBINE_DATA( &namcos2_68k_serial_comms_ctrl[offset] );
}

/*************************************************************/
/* 68000 Shared protection/random key generator              */
/*************************************************************

Custom chip ID numbers:

Game        Year    ID (dec)    ID (hex)
--------    ----    ---         -----
finallap    1987
assault     1988    unused
metlhawk    1988
ordyne      1988    176         $00b0
mirninja    1988    177         $00b1
phelios     1988    178         $00b2   readme says 179
dirtfoxj    1989    180         $00b4
fourtrax    1989
valkyrie    1989
finehour    1989    188         $00bc
burnforc    1989    189         $00bd
marvland    1989    190         $00be
kyukaidk    1990    191         $00bf
dsaber      1990    192         $00c0
finalap2    1990    318         $013e
rthun2      1990    319         $013f
gollygho    1990                $0143
cosmogng    1991    330         $014a
sgunner2    1991    346         $015a   ID out of order; gfx board is not standard
finalap3    1992    318         $013e   same as finalap2
suzuka8h    1992
sws92       1992    331         $014b
sws92g      1992    332         $014c
suzuk8h2    1993
sws93       1993    334         $014e
 *************************************************************/
static int sendval = 0;
READ16_HANDLER( namcos2_68k_key_r )
{
	switch (namcos2_gametype)
	{
	case NAMCOS2_ORDYNE:
		switch(offset)
		{
		case 2: return 0x1001;
		case 3: return 0x1;
		case 4: return 0x110;
		case 5: return 0x10;
		case 6: return 0xB0;
		case 7: return 0xB0;
		}
		break;

	case NAMCOS2_STEEL_GUNNER_2:
		switch( offset )
		{
			case 4: return 0x15a;
		}
	    break;

	case NAMCOS2_MIRAI_NINJA:
		switch(offset)
		{
		case 7: return 0xB1;
		}
	    break;

	case NAMCOS2_PHELIOS:
		switch(offset)
		{
		case 0: return 0xF0;
		case 1: return 0xFF0;
		case 2: return 0xB2;
		case 3: return 0xB2;
		case 4: return 0xF;
		case 5: return 0xF00F;
		case 7: return 0xB2;
		}
	    break;

	case NAMCOS2_DIRT_FOX_JP:
		switch(offset)
		{
		case 1: return 0xB4;
		}
		break;

	case NAMCOS2_FINEST_HOUR:
		switch(offset)
		{
		case 7: return 0xBC;
		}
	    break;

	case NAMCOS2_BURNING_FORCE:
		switch(offset)
		{
		case 1: return 0xBD;
		case 7: return 0xBD;
		}
		break;

	case NAMCOS2_MARVEL_LAND:
		switch(offset)
		{
		case 0: return 0x10;
		case 1: return 0x110;
		case 4: return 0xBE;
		case 6: return 0x1001;
		case 7: return (sendval==1)?0xBE:1;
		}
	    break;

	case NAMCOS2_DRAGON_SABER:
		switch(offset)
		{
		case 2: return 0xC0;
		}
		break;

	case NAMCOS2_ROLLING_THUNDER_2:
		switch(offset)
		{
		case 4:
			if( sendval == 1 ){
		        sendval = 0;
		        return 0x13F;
			}
			break;
		case 7:
			if( sendval == 1 ){
			    sendval = 0;
			    return 0x13F;
			}
			break;
		case 2: return 0;
		}
		break;

	case NAMCOS2_COSMO_GANG:
		switch(offset)
		{
		case 3: return 0x14A;
		}
		break;

	case NAMCOS2_SUPER_WSTADIUM:
		switch(offset)
		{
	//  case 3: return 0x142;
		case 4: return 0x142;
	//  case 3: ui_popup("blah %08x",activecpu_get_pc());
		default: return mame_rand();
		}
		break;

	case NAMCOS2_SUPER_WSTADIUM_92:
		switch(offset)
		{
		case 3: return 0x14B;
		}
		break;

	case NAMCOS2_SUPER_WSTADIUM_92T:
		switch(offset)
		{
		case 3: return 0x14C;
		}
		break;

	case NAMCOS2_SUPER_WSTADIUM_93:
		switch(offset)
		{
		case 3: return 0x14E;
		}
		break;

	case NAMCOS2_SUZUKA_8_HOURS_2:
		switch(offset)
		{
		case 3: return 0x14D;
		case 2: return 0;
		}
		break;

	case NAMCOS2_GOLLY_GHOST:
		switch(offset)
		{
		case 0: return 2;
		case 1: return 2;
		case 2: return 0;
		case 4: return 0x143;
		}
		break;


	case NAMCOS2_BUBBLE_TROUBLE:
		switch(offset)
		{
		case 0: return 2; // not verified
		case 1: return 2; // not verified
		case 2: return 0; // not verified
		case 4: return 0x141;
		}
		break;
	}



	return mame_rand()&0xffff;
}

WRITE16_HANDLER( namcos2_68k_key_w )
{
	if( namcos2_gametype == NAMCOS2_MARVEL_LAND && offset == 5 )
	{
		if (data == 0x615E) sendval = 1;
	}
	if( namcos2_gametype == NAMCOS2_ROLLING_THUNDER_2 && offset == 4 )
	{
		if (data == 0x13EC) sendval = 1;
	}
	if( namcos2_gametype == NAMCOS2_ROLLING_THUNDER_2 && offset == 7 )
	{
		if (data == 0x13EC) sendval = 1;
	}
	if( namcos2_gametype == NAMCOS2_MARVEL_LAND && offset == 6 )
	{
		if (data == 0x1001) sendval = 0;
	}
}

/*************************************************************/
/* 68000 Interrupt/IO Handlers - CUSTOM 148 - NOT SHARED     */
/*************************************************************/

#define NO_OF_LINES 	256
#define FRAME_TIME		(1.0/60.0)
#define LINE_LENGTH 	(FRAME_TIME/NO_OF_LINES)

UINT16  namcos2_68k_master_C148[0x20];
UINT16  namcos2_68k_slave_C148[0x20];

static UINT16
ReadWriteC148( int cpu, offs_t offset, UINT16 data, int bWrite )
{
	offs_t addr = ((offset*2)+0x1c0000)&0x1fe000;
	UINT16 *pC148Reg   = (cpu==CPU_MASTER)?namcos2_68k_master_C148:namcos2_68k_slave_C148;
	UINT16 *pC148RegAlt = (cpu==CPU_SLAVE)?namcos2_68k_master_C148:namcos2_68k_slave_C148;
	UINT16 result = pC148Reg[(addr>>13)&0x1f];
	int altCPU = (cpu==CPU_MASTER)?CPU_SLAVE:CPU_MASTER;

	if( bWrite )
	{
		pC148Reg[(addr>>13)&0x1f] = data&0x0007;
	}

	switch(addr)
	{
	case 0x1c0000: break; /* NAMCOS2_C148_0 level */
	case 0x1c2000: break; /* NAMCOS2_C148_1 level */
	case 0x1c4000: break; /* NAMCOS2_C148_2 level */
	case 0x1c6000: break; /* NAMCOS2_C148_CPUIRQ level */
	case 0x1c8000: break; /* NAMCOS2_C148_EXIRQ level */
	case 0x1ca000: break; /* NAMCOS2_C148_POSIRQ level */
	case 0x1cc000: break; /* NAMCOS2_C148_SERIRQ level */
	case 0x1ce000: break; /* NAMCOS2_C148_VBLANKIRQ level */

	case 0x1d0000:
		if( bWrite )
		{ /* DSP "render display list now" trigger */
		}
		cpunum_set_input_line(cpu, pC148Reg[NAMCOS2_C148_0], CLEAR_LINE);
		break;

	case 0x1d2000:
		if( bWrite )
		{
		//  printf( "cpu(%d) RAM[0x%06x] = 0x%x\n", cpu, addr, data );
		}
		cpunum_set_input_line(cpu, pC148Reg[NAMCOS2_C148_1], CLEAR_LINE);
		break;

	case 0x1d4000:
		if( bWrite )
		{
			cpunum_set_input_line(altCPU, pC148RegAlt[NAMCOS2_C148_CPUIRQ], ASSERT_LINE);
		}
		cpunum_set_input_line(cpu, pC148Reg[NAMCOS2_C148_2], CLEAR_LINE);
		break;

	case 0x1d6000:
		if( bWrite )
		{
		//  printf( "cpu(%d) RAM[0x%06x] = 0x%x\n", cpu, addr, data );
		}
		cpunum_set_input_line(cpu, pC148Reg[NAMCOS2_C148_CPUIRQ], CLEAR_LINE);
		break;

	case 0x1d8000: /* ack EXIRQ */
		if( bWrite )
		{
		//  printf( "cpu(%d) RAM[0x%06x] = 0x%x\n", cpu, addr, data );
		}
		cpunum_set_input_line(cpu, pC148Reg[NAMCOS2_C148_EXIRQ], CLEAR_LINE);
		break;

	case 0x1da000: /* ack POSIRQ */
		if( bWrite )
		{
		//  printf( "cpu(%d) RAM[0x%06x] = 0x%x\n", cpu, addr, data );
		}
		cpunum_set_input_line(cpu, pC148Reg[NAMCOS2_C148_POSIRQ], CLEAR_LINE);
		break;

	case 0x1dc000: /* ack SCIRQ */
		if( bWrite )
		{
		//  printf( "cpu(%d) RAM[0x%06x] = 0x%x\n", cpu, addr, data );
		}
		cpunum_set_input_line(cpu, pC148Reg[NAMCOS2_C148_SERIRQ], CLEAR_LINE);
		break;

	case 0x1de000: /* ack VBLANK */
		cpunum_set_input_line(cpu, pC148Reg[NAMCOS2_C148_VBLANKIRQ], CLEAR_LINE);
		break;

	case 0x1e0000: /* EEPROM Status Register */
		result = ~0; /* Only BIT0 used: 1=EEPROM READY 0=EEPROM BUSY */
		break;

	case 0x1e2000: /* Sound CPU Reset control */
		if( cpu == CPU_MASTER ) /* ? */
		{
			if( data&0x01 )
			{
				/* Resume execution */
				cpunum_set_input_line(CPU_SOUND, INPUT_LINE_RESET, CLEAR_LINE);
				cpu_yield();
			}
			else
			{
				/* Suspend execution */
				cpunum_set_input_line(CPU_SOUND, INPUT_LINE_RESET, ASSERT_LINE);
			}
		}
		break;

	case 0x1e4000: /* Alt 68000 & IO CPU Reset */
		if( cpu == CPU_MASTER ) /* ? */
		{
			if( data&0x01 )
			{
				/* Resume execution */
				cpunum_set_input_line(altCPU, INPUT_LINE_RESET, CLEAR_LINE);
				cpunum_set_input_line(CPU_MCU, INPUT_LINE_RESET, CLEAR_LINE);
				/* Give the new CPU an immediate slice of the action */
				cpu_yield();
			}
			else
			{
				/* Suspend execution */
				cpunum_set_input_line(altCPU, INPUT_LINE_RESET, ASSERT_LINE);
				cpunum_set_input_line(CPU_MCU, INPUT_LINE_RESET, ASSERT_LINE);
			}
		}
		break;

	case 0x1e6000: /* Watchdog reset kicker */
		/* watchdog_reset_w(0,0); */
		break;

	default:
		break;
	}
	return result;
}

WRITE16_HANDLER( namcos2_68k_master_C148_w )
{
	(void)ReadWriteC148( CPU_MASTER, offset, data, 1 );
}

READ16_HANDLER( namcos2_68k_master_C148_r )
{
	return ReadWriteC148( CPU_MASTER, offset, 0, 0 );
}

WRITE16_HANDLER( namcos2_68k_slave_C148_w )
{
	(void)ReadWriteC148( CPU_SLAVE, offset, data, 1 );
}

READ16_HANDLER( namcos2_68k_slave_C148_r )
{
	return ReadWriteC148( CPU_SLAVE, offset, 0, 0 );
}

void namcos2_68k_master_posirq( int scanline )
{
	force_partial_update(scanline);
	cpunum_set_input_line(CPU_MASTER , namcos2_68k_master_C148[NAMCOS2_C148_POSIRQ] , ASSERT_LINE);
}

static int
GetPosIRQScanline( void )
{
	switch( namcos2_gametype )
	{
		case NAMCOS21_AIRCOMBAT:
		case NAMCOS21_STARBLADE:
		case NAMCOS21_CYBERSLED:
		case NAMCOS21_SOLVALOU:
		case NAMCOS21_WINRUN91:
		return 16; /* ? */

		default:
		return namcos2_GetPosIrqScanline();
	}
}

INTERRUPT_GEN( namcos2_68k_master_vblank )
{
	if( namcos2_68k_master_C148[NAMCOS2_C148_POSIRQ] )
	{
		int scanline = GetPosIRQScanline();
		timer_set( cpu_getscanlinetime(scanline), scanline, namcos2_68k_master_posirq );
	}
	cpunum_set_input_line( CPU_MASTER, namcos2_68k_master_C148[NAMCOS2_C148_VBLANKIRQ], HOLD_LINE);
}

void namcos2_68k_slave_posirq( int scanline )
{
	force_partial_update(scanline);
	cpunum_set_input_line(CPU_SLAVE , namcos2_68k_slave_C148[NAMCOS2_C148_POSIRQ] , ASSERT_LINE);
}

INTERRUPT_GEN( namcos2_68k_slave_vblank )
{
	if( namcos2_68k_slave_C148[NAMCOS2_C148_POSIRQ] )
	{
		int scanline = GetPosIRQScanline();
		timer_set( cpu_getscanlinetime(scanline), scanline, namcos2_68k_slave_posirq );
	}
	cpunum_set_input_line( CPU_SLAVE, namcos2_68k_slave_C148[NAMCOS2_C148_VBLANKIRQ], HOLD_LINE);
}

/**************************************************************/
/*  Sound sub-system                                          */
/**************************************************************/

WRITE8_HANDLER( namcos2_sound_bankselect_w )
{
	unsigned char *RAM=memory_region(REGION_CPU3);
	unsigned long max = (memory_region_length(REGION_CPU3) - 0x10000) / 0x4000;
	int bank = ( data >> 4 ) % max;	/* 991104.CAB */
	memory_set_bankptr( CPU3_ROM1, &RAM[ 0x10000 + ( 0x4000 * bank ) ] );
}

/**************************************************************/
/*                                                            */
/*  68705 IO CPU Support functions                            */
/*                                                            */
/**************************************************************/

static int namcos2_mcu_analog_ctrl=0;
static int namcos2_mcu_analog_data=0xaa;
static int namcos2_mcu_analog_complete=0;

WRITE8_HANDLER( namcos2_mcu_analog_ctrl_w )
{
	namcos2_mcu_analog_ctrl=data&0xff;

	/* Check if this is a start of conversion */
	/* Input ports 2 thru 9 are the analog channels */
	if(data&0x40)
	{
		/* Set the conversion complete flag */
		namcos2_mcu_analog_complete=2;
		/* We convert instantly, good eh! */
		switch((data>>2)&0x07)
		{
		case 0:
			namcos2_mcu_analog_data=input_port_2_r(0);
			break;
		case 1:
			namcos2_mcu_analog_data=input_port_3_r(0);
			break;
		case 2:
			namcos2_mcu_analog_data=input_port_4_r(0);
			break;
		case 3:
			namcos2_mcu_analog_data=input_port_5_r(0);
			break;
		case 4:
			namcos2_mcu_analog_data=input_port_6_r(0);
			break;
		case 5:
			namcos2_mcu_analog_data=input_port_7_r(0);
			break;
		case 6:
			namcos2_mcu_analog_data=input_port_8_r(0);
			break;
		case 7:
			namcos2_mcu_analog_data=input_port_9_r(0);
			break;
		}
#if 0
		/* Perform the offset handling on the input port */
		/* this converts it to a twos complement number */
		if( namcos2_gametype==NAMCOS2_DIRT_FOX ||
			namcos2_gametype==NAMCOS2_DIRT_FOX_JP )
		{
			namcos2_mcu_analog_data^=0x80;
		}
#endif
		/* If the interrupt enable bit is set trigger an A/D IRQ */
		if(data&0x20)
		{
			cpunum_set_input_line( CPU_MCU, HD63705_INT_ADCONV , PULSE_LINE);
		}
	}
}

READ8_HANDLER( namcos2_mcu_analog_ctrl_r )
{
	int data=0;

	/* ADEF flag is only cleared AFTER a read from control THEN a read from DATA */
	if(namcos2_mcu_analog_complete==2) namcos2_mcu_analog_complete=1;
	if(namcos2_mcu_analog_complete) data|=0x80;

	/* Mask on the lower 6 register bits, Irq EN/Channel/Clock */
	data|=namcos2_mcu_analog_ctrl&0x3f;
	/* Return the value */
	return data;
}

WRITE8_HANDLER( namcos2_mcu_analog_port_w )
{
}

READ8_HANDLER( namcos2_mcu_analog_port_r )
{
	if(namcos2_mcu_analog_complete==1) namcos2_mcu_analog_complete=0;
	return namcos2_mcu_analog_data;
}

WRITE8_HANDLER( namcos2_mcu_port_d_w )
{
	/* Undefined operation on write */
}

READ8_HANDLER( namcos2_mcu_port_d_r )
{
	/* Provides a digital version of the analog ports */
	int threshold=0x7f;
	int data=0;

	/* Read/convert the bits one at a time */
	if(input_port_2_r(0)>threshold) data|=0x01;
	if(input_port_3_r(0)>threshold) data|=0x02;
	if(input_port_4_r(0)>threshold) data|=0x04;
	if(input_port_5_r(0)>threshold) data|=0x08;
	if(input_port_6_r(0)>threshold) data|=0x10;
	if(input_port_7_r(0)>threshold) data|=0x20;
	if(input_port_8_r(0)>threshold) data|=0x40;
	if(input_port_9_r(0)>threshold) data|=0x80;

	/* Return the result */
	return data;
}

READ8_HANDLER( namcos2_input_port_0_r )
{
	int data=readinputport(0);
	return data;
}

READ8_HANDLER( namcos2_input_port_10_r )
{
	int data=readinputport(10);
	return data;
}

READ8_HANDLER( namcos2_input_port_12_r )
{
	int data=readinputport(12);
	return data;
}
