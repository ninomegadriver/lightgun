/**
Namco System 21

    There are at least three hardware variations, all of which are based on
    Namco System2:

    1. Winning Run was a mass-produced prototype; it uses 3 68k CPUs; all others use 2 68k CPUs.

    2. Cyber Sled uses 4xTMS320C25

    3. Starblade uses 5xTMS320C20

The main 68k CPUs populate a chunk of shared RAM with an display list describing a scene to be rendered.
The main CPUs also specify attributes for a master camera which provides additional global transformations.
The display list contains references to specific 3d objects and their position/orientation in 3d space.

The master DSP parses the display list and applies high level geometry, emitting matrices and object
references.

An intermediate device (*) expands object references to the primitive data from point ROMs interleaved with
local transforms, making this data available as input to the slave DSP.

The slave DSP applies the view transform to each primitive's vertices, projection, and clipping,
and outputs quad descriptors.  Each quad has a reference color (shared across vertices),
and for each vertex the tuple: (screenx,screeny,z-code).  The z-code scalar accounts for depth bias.
A zbuffer is used while rendering quads.

---------------------------------------------------------------------------

STATUS:
    Starblade:
        missing background objects on stage#1 (starships)
        - missing 'upload' data (dsp to 68k)?; check for unmapped ram use?
        - find object code for starship (compare to PSX port)

    Solvaou:
        no polys; why?

    CyberSled:
        graphics cropped
        three uploads?

    Air Combat:
        unemulated ram

    Winning Run
        ?

    Winning Run 92
        ?

    Driver's Eyes
        ?

TODO:   Unify memory maps.

TODO:   Fix emulation to support other games (likely caused by dspram16_r hacks).

TODO:   Determine purpose of additional (unemulated) DSPs.  I believe they are additional slave DSPs that
        run in parallel, but it is difficult to know for sure without having access to the DSP bios code.
        The single slave DSP isn't powerful enough to update the polygon layer at 60fps.

TODO:   (*) Extract master DSP's BIOS (via trojan if possible).
        Certain tasks currently implemented in C (see TransferDspData) are quite likely done explicitly
        by the master DSP.

TODO:   Determine how "point ram" (tested by main 68k cpu on boot) is actually used.

TODO:   namcoic.c: in StarBlade, the sprite list is stored at a different location during startup tests.
        Is there a register that controls this?

TODO:   Starblade has some missing background graphics (i.e. the spaceport runway at
    beginning of stage#1, various large startship cruisers drifting in space, etc.
        Even with (mostly complete) DSP emulation, it is a complete mystery why this isn't working.
    The main CPU doesn't write descriptors for these objects in its display list, nor does it
    write any obvious "high level commands" to control background graphics sequencing.

TODO:   CyberSled appears to use the "POSIRQ" feature seen in various Namco System 2 games for
        split screen effects.  This IRQ is triggered at a designated scanline.  Confirm that this
        works as expected.

TODO:   Map lamps/vibration outputs as used by StarBlade (and possibly other titles).
        These likely involve the MCU.

---------------------------------------------------------------------------

DSP RAM is shared with the 68000 CPUs and master DSP.
This memory map reflects DSP RAM as seen by the 68000 CPUs.

    0x200000..0x2000ff  populated with ASCII Text during self-tests:
        ROM:
        RAM:
        PTR:
        SMU:
        IDC:
        CPU:ABORT
        DSP:
        CRC:OK  from cpu
        CRC:    from dsp
        ID:
        B-M:    P-M:    S-M:    SET UP

    0x200100    status: 2=upload needed, 4=error (abort)
    0x200102    status
    0x200104    0x0002
    0x200106    addr written by main cpu
    0x20010a    checksum (starblade expects 0xed53)
    0x20010c    checksum (starblade expects 0xd5df)
    0x20010e    1 : upload-code-to-dsp request trigger
    0x200110    status
    0x200112    status
    0x200114    master dsp code size
    0x200116    slave dsp code size
    0x200120    upload source1 addr hi
    0x200122    upload source1 addr lo
    0x200124    upload source2 addr hi
    0x200126    upload source2 addr lo

    0x200200    enable
    0x200202    status
    0x200206    work page select

    0x208000..0x2080ff  camera attributes for page#0
    0x208200..0x208fff  3d object attribute display list for page#0

    0x20c000..0x20c0ff  camera attributes for page#1
    0x20c200..0x20cfff  3d object attribute display list for page#1

---------------------------------------------------------------------------

Thanks to Aaron Giles for originally making sense of the Point ROM data.

Point data in ROMS (signed 24 bit words) encodes the 3d primitives.

The first part of the Point ROMs is an address table.  The first entry serves
a special (unknown) purpose.

Given an object index, this table provides an address into the second part of
the ROM.

The second part of the ROM is a series of display lists.
This is a sequence of pointers to actual polygon data. There may be
more than one, and the list is terminated by $ffffff.

The remainder of the ROM is a series of polygon data. The first word of each
entry is the length of the entry (in words, not counting the length word).

The rest of the data in each entry is organized as follows:

length (1 word)
unknown value (1 word) - this increments with each entry
vertex count (1 word) - the number of vertices encoded
unknown value (1 word) - almost always 0; depth bias
vertex list (n x 3 words)
quad count (1 word) - the number of quads to draw
quad primitives (n x 5 words) - indices of four verticies plus color code

-----------------------------------------------------------------------
Board 1 : DSP Board - 1st PCB. (Uppermost)
DSP Type 1 : 4 x TMS320C25 connected x 4 x Namco Custom chip 67 (68 pin PLCC) (Cybersled)
DSP Type 2 : 5 x TMS320C20 (Starblade)
OSC: 40.000MHz
RAM: HM62832 x 2, M5M5189 x 4, ISSI IS61C68 x 16
ROMS: TMS27C040
Custom Chips:
4 x Namco Custom 327 (24 pin NDIP), each one located next to a chip 67.
4 x Namco Custom chip 342 (160 pin PQFP), there are 3 leds (red/green/yellow) connected to each 342 chip. (12 leds total)
2 x Namco Custom 197 (28 pin NDIP)
Namco Custom chip 317 IDC (180 pin PQFP)
Namco Custom chip 195 (160 pin PQFP)
-----------------------------------------------------------------------
Board 2 : Unknown Board - 2nd PCB (no roms)
OSC: 20.000MHz
RAM: HM62256 x 10, 84256 x 4, CY7C128 x 5, M5M5178 x 4
OTHER Chips:
MB8422-90LP
L7A0565 316 (111) x 1 (100 PIN PQFP)
150 (64 PIN PQFP)
167 (128 PIN PQFP)
L7A0564 x 2 (100 PIN PQFP)
157 x 16 (24 PIN NDIP)
-----------------------------------------------------------------------
Board 3 : CPU Board - 3rd PCB (looks very similar to Namco System 2 CPU PCB)
CPU: MC68000P12 x 2 @ 12 MHz (16-bit)
Sound CPU: MC68B09EP (3 MHz)
Sound Chips: C140 24-channel PCM (Sound Effects), YM2151 (Music), YM3012 (?)
XTAL: 3.579545 MHz
OSC: 49.152 MHz
RAM: MB8464 x 2, MCM2018 x 2, HM65256 x 4, HM62256 x 2

Other Chips:
Sharp PC900 - Opto-isolator
Sharp PC910 - Opto-isolator
HN58C65P (EEPROM)
MB3771
MB87077-SK x 2 (24 pin NDIP, located in sound section)
LB1760 (16 pin DIP, located next to SYS87B-2B)
CY7C132 (48 PIN DIP)

Namco Custom:
148 x 2 (64 pin PQFP)
C68 (64 pin PQFP)
139 (64 pin PQFP)
137 (28 pin NDIP)
149 (28 pin NDIP, near C68)
-----------------------------------------------------------------------
Board 4 : 4th PCB (bottom-most)
OSC: 38.76922 MHz
There is a 6 wire plug joining this PCB with the CPU PCB. It appears to be video cable (RGB, Sync etc..)
Jumpers:
JP7 INTERLACE = SHORTED (Other setting is NON-INTERLACE)
JP8 68000 = SHORTED (Other setting is 68020)
Namco Custom Chips:
C355 (160 pin PQFP)
187 (120 pin PQFP)
138 (64 pin PQFP)
165 (28 pin NDIP)
-----------------------------------------------------------------------

-------------------
Air Combat by NAMCO
-------------------
malcor


Location        Device     File ID      Checksum
-------------------------------------------------
CPU68  1J       27C4001    MPR-L.AC1      9859   [ main program ]  [ rev AC1 ]
CPU68  3J       27C4001    MPR-U.AC1      97F1   [ main program ]  [ rev AC1 ]
CPU68  1J       27C4001    MPR-L.AC2      C778   [ main program ]  [ rev AC2 ]
CPU68  3J       27C4001    MPR-U.AC2      6DD9   [ main program ]  [ rev AC2 ]
CPU68  1C      MB834000    EDATA1-L.AC1   7F77   [    data      ]
CPU68  3C      MB834000    EDATA1-U.AC1   FA2F   [    data      ]
CPU68  3A      MB834000    EDATA-U.AC1    20F2   [    data      ]
CPU68  1A      MB834000    EDATA-L.AC1    9E8A   [    data      ]
CPU68  8J        27C010    SND0.AC1       71A8   [  sound prog  ]
CPU68  12B     MB834000    VOI0.AC1       08CF   [   voice 0    ]
CPU68  12C     MB834000    VOI1.AC1       925D   [   voice 1    ]
CPU68  12D     MB834000    VOI2.AC1       C498   [   voice 2    ]
CPU68  12E     MB834000    VOI3.AC1       DE9F   [   voice 3    ]
CPU68  4C        27C010    SPR-L.AC1      473B   [ slave prog L ]  [ rev AC1 ]
CPU68  6C        27C010    SPR-U.AC1      CA33   [ slave prog U ]  [ rev AC1 ]
CPU68  4C        27C010    SPR-L.AC2      08CE   [ slave prog L ]  [ rev AC2 ]
CPU68  6C        27C010    SPR-U.AC2      A3F1   [ slave prog U ]  [ rev AC2 ]
OBJ(B) 5S       HN62344    OBJ0.AC1       CB72   [ object data  ]
OBJ(B) 5X       HN62344    OBJ1.AC1       85E2   [ object data  ]
OBJ(B) 3S       HN62344    OBJ2.AC1       89DC   [ object data  ]
OBJ(B) 3X       HN62344    OBJ3.AC1       58FF   [ object data  ]
OBJ(B) 4S       HN62344    OBJ4.AC1       46D6   [ object data  ]
OBJ(B) 4X       HN62344    OBJ5.AC1       7B91   [ object data  ]
OBJ(B) 2S       HN62344    OBJ6.AC1       5736   [ object data  ]
OBJ(B) 2X       HN62344    OBJ7.AC1       6D45   [ object data  ]
OBJ(B) 17N     PLHS18P8    3P0BJ3         4342
OBJ(B) 17N     PLHS18P8    3POBJ4         1143
DSP    2N       HN62344    AC1-POIL.L     8AAF   [   DSP data   ]
DSP    2K       HN62344    AC1-POIL.L     CF90   [   DSP data   ]
DSP    2E       HN62344    AC1-POIH       4D02   [   DSP data   ]
DSP    17D     GAL16V8A    3PDSP5         6C00

NOTE:  CPU68  - CPU board        2252961002  (2252971002)
       OBJ(B) - Object board     8623961803  (8623963803)
       DSP    - DSP board        8623961703  (8623963703)
       PGN(C) - PGN board        2252961300  (8623963600)

       Namco System 21 Hardware

       ROMs that have the same locations are different revisions
       of the same ROMs (AC1 or AC2).

Jumper settings:

Location    Position set    alt. setting
----------------------------------------

CPU68 PCB:

  JP2          /D-ST           /VBL
  JP3
*/
#include "driver.h"
#include "namcos2.h"
#include "cpu/m6809/m6809.h"
#include "cpu/tms32025/tms32025.h"
#include "namcoic.h"
#include "sound/2151intf.h"
#include "sound/c140.h"
#include "namcos21.h"

static UINT16 *master_dsp_rom;
static UINT16 *master_dsp_ram;
static UINT16 *slave_dsp_rom;
static UINT16 *slave_dsp_ram;

#define CPU_DSP_SLAVE_REGION	REGION_CPU6

#define DSP_BUF_MAX (4096*8)
static struct
{
	unsigned masterSourceAddr;
	UINT16 slaveInputBuffer[DSP_BUF_MAX];
	size_t slaveInputSize;
	unsigned slaveSourceAddr;

	UINT16 slaveOutputBuffer[DSP_BUF_MAX];
	unsigned slaveOutputSize;

	UINT16 masterDirectDrawBuffer[256];
	unsigned masterDirectDrawSize;
} *mpDspState;

/*static*/ void
UpdatePolyFrameBufferAndResetDSPs( void )
{
	memset( mpDspState, 0, sizeof(*mpDspState) );
	cpunum_set_input_line(4, INPUT_LINE_RESET, PULSE_LINE); /* DSP: master */
	cpunum_set_input_line(5, INPUT_LINE_RESET, PULSE_LINE); /* DSP: slave */
	namcos21_ClearPolyFrameBuffer();
}

/* globals (shared by videohrdw/namcos21.c) */
UINT16 *namcos21_dspram16;
UINT16 *namcos21_spritepos;

/* private data */
static UINT16	*mpDataROM;
static UINT16	*mpSharedRAM1;
static UINT8	*mpDualPortRAM;
static UINT16 namcos21_dsp_control[1];

/* memory handlers for shared DSP RAM (used to pass 3d parameters) */

static READ16_HANDLER( uploadhack_r )
{ /* used for memory reads from fake adddress 0xfffffe */
	return 0;
}

void namcos21_enabledsps( void )
{
	master_dsp_rom[1] = 0x0100;
	slave_dsp_rom[1] = 0x0100;
}

static READ16_HANDLER( dspram16_r )
{
	if( namcos21_dspram16[0] == 0x524f ) // "RO"(M)
	{ /* the check above is done so we don't interfere with the DSPRAM test */
		namcos21_dspram16[0x100/2] = 2; /* status to fake working DSP */
		namcos21_dspram16[0x102/2] = 2; /* status to fake working DSP */
		namcos21_dspram16[0x110/2] = 2; /* status to fake working DSP */
		namcos21_dspram16[0x112/2] = 0; /* status to fake working DSP */

		if( namcos2_gametype == NAMCOS21_SOLVALOU )
		{
			if( code_pressed(KEYCODE_A) ) namcos21_dspram16[0x94] = 0; // solvalou patch
		}

		/* The DSPs provide the 68k CPUs with a list of memory chunks to copy.
         * This is done at boot time, and doesn't appear to actually do anything
         * important.  For now, we pass artifically constructed parameters that
         * cause no memory copies to take place.
         */
		namcos21_dspram16[0x120/2] = 0xfffffe>>16;
		namcos21_dspram16[0x122/2] = 0xfffffe&0xffff;
		namcos21_dspram16[0x124/2] = 0xfffffe>>16;
		namcos21_dspram16[0x126/2] = 0xfffffe&0xffff;

		if( namcos21_dspram16[0x128/2] == 1 )
		{ /* ??? */
			namcos21_dspram16[0x102/2] = 1;
//          namcos21_dspram16[0x128/2] = 0; /* ? */
		}

		switch( offset )
		{
		case 0x010a/2:
		case 0x010c/2:
			logerror( "crc read: %d:0x%08x\n", cpu_getactivecpu(), activecpu_get_pc() );
			break;
		default:
			break;
		}

		/* The 68k CPUs perform some sort of checksum on the DSPs. The expected values are game-specific. */
		switch( namcos2_gametype )
		{
		case NAMCOS21_CYBERSLED:
			namcos21_dspram16[0x10a/2] = 0x4f4b;
			namcos21_dspram16[0x10c/2] = 0x759c;
			break;

		case NAMCOS21_STARBLADE:
			namcos21_dspram16[0x10a/2] = 0xed53;
			namcos21_dspram16[0x10c/2] = 0xd5df;
			break;

		case NAMCOS21_SOLVALOU:
			namcos21_dspram16[0x10a/2] = 0x05b2;
			namcos21_dspram16[0x10c/2] = 0x606e;
			break;

		case NAMCOS21_AIRCOMBAT:
			break;

		default:
			/* TODO: add 'checksum' values for other games */
			break;
		}

		if( namcos21_dspram16[0x10e/2] == 0x0001 )
		{
			int i;
			unsigned len;
			static int count;

			/* This signals that a large chunk of code/data has been written by the main CPU.
             * The 68k CPU waits for this flag to be cleared.
             */
			logerror( "%d:0x%x upload %d\n",
				cpu_getactivecpu(),
				activecpu_get_pc(),
				count++ );

			namcos21_dspram16[0x10e/2] = 0; /* ack */

			/* upload to master dsp */
			len = namcos21_dspram16[0x114/2];
			if( len )
			{
				logerror( "upload master\n" );
				for( i=0; i<len; i++ )
				{
					master_dsp_ram[i] = namcos21_dspram16[0x2000/2+i];
				}
				master_dsp_rom[1] = 0x0100;
			}

			/* upload to slave dsp(s) */
			len = namcos21_dspram16[0x116/2];
			if( len )
			{
				logerror( "upload slave\n" );
				for( i=0; i<len; i++ )
				{
					slave_dsp_ram[i] = namcos21_dspram16[0xa000/2+i];
				}
				slave_dsp_rom[1] = 0x0100;
			}
		}
	}
	return namcos21_dspram16[offset];
}

static WRITE16_HANDLER( dspram16_w )
{
	COMBINE_DATA( &namcos21_dspram16[offset] );
	if( offset == 0x103 )
	{
		cpu_yield(); /* hack; synchronization for solvalou */
	}
} /* dspram16_w */

/************************************************************************************/

static int
InitDSP( void )
{
	UINT16 *pMem;
	UINT16 pc, loop;

	mpDspState = auto_malloc( sizeof(*mpDspState) );

	memset( mpDspState, 0, sizeof(*mpDspState) );

	pMem = master_dsp_rom;
	pc = 0;
	pMem[pc++] = 0xff80; /* b */
	pMem[pc++] = 0; /* 0x0100; */
	pc = 2; /* IRQ0 */
	pMem[pc++] = 0xCE26; /* ret */
	pc = 4; /* IRQ1 */
	pMem[pc++] = 0xCE26; /* ret */
	pc = 6; /* IRQ2 */
	pMem[pc++] = 0xCE26; /* ret */
	pc = 0x18; /* timer IRQ */
	pMem[pc++] = 0xCE26; /* ret */
	pc = 0x1a; /* serial receive IRQ */
	pMem[pc++] = 0xCE26; /* ret */
	pc = 0x1b; /* serial send IRQ */
	pMem[pc++] = 0xCE26; /* ret */
	pc = 0x100;
	pMem[pc] = 0xc804;pc++; /* ldpk 004h */
	pMem[pc] = 0xca00;pc++; /* zac */
	pMem[pc] = 0x6000;pc++; /* sacl $00,0 */
	pMem[pc] = 0xca01;pc++; /* lack 01h */
	pMem[pc] = 0x6001;pc++; /* sacl $01,0 */
	loop = pc;
	pMem[pc++] = 0xfe80; /* call */
	pMem[pc++] = 0x8002;
	pMem[pc++] = 0xff80; /* b */
	pMem[pc++] = loop;

	pMem = slave_dsp_rom;
	pc = 0;
	pMem[pc++] = 0xff80; /* b */
	pMem[pc++] = 0;
	pMem[pc++] = 0xff80; /* b */
	pMem[pc++] = 0; /* 0x0100; */
	pc = 2; /* IRQ0 */
	pMem[pc++] = 0xCE26; /* ret */
	pc = 4; /* IRQ1 */
	pMem[pc++] = 0xCE26; /* ret */
	pc = 6; /* IRQ2 */
	pMem[pc++] = 0xCE26; /* ret */
	pc = 0x18; /* timer IRQ */
	pMem[pc++] = 0xCE26; /* ret */
	pc = 0x1a; /* serial receive IRQ */
	pMem[pc++] = 0xCE26; /* ret */
	pc = 0x1b; /* serial send IRQ */
	pMem[pc++] = 0xCE26; /* ret */
	pc = 0x100;
	pMem[pc] = 0xc804;pc++; /* ldpk 004h */
	pMem[pc] = 0xca00;pc++; /* zac */
	pMem[pc] = 0x6000;pc++; /* sacl $00,0 */
	pMem[pc] = 0xca01;pc++; /* lack 01h */
	pMem[pc] = 0x6001;pc++; /* sacl $01,0 */
	loop = pc;
	pMem[pc++] = 0xfe80; /* call */
	pMem[pc++] = 0x8010;
	pMem[pc++] = 0xff80; /* b */
	pMem[pc++] = loop;

	return 0;
}

/***********************************************************/

static READ16_HANDLER( dsp_port1_r )
{ /* PDP status */
	return 0x8000;
}
static READ16_HANDLER( dsp_port2_r )
{ /* PDP kick */
	return 0;
}

static WRITE16_HANDLER( dsp_port2_w )
{ /* PDP addr set */
	mpDspState->masterSourceAddr = data;
	logerror( "port2_w(PDP ADDR): 0x%04x\n", data );
}
static READ16_HANDLER( dsp_port3_r )
{ /* trigger; XF-related? */
	logerror( "port3_r(PDP trigger)\n" );
	return 0;
}
static WRITE16_HANDLER( dsp_port4_w )
{ /* PDP mode */
	logerror( "port4_w(0x%04x)\n", data );
}
static READ16_HANDLER( dsp_port8_r )
{ /* slave status? */
	return 0x0001;
}
static READ16_HANDLER( dsp_port9_r )
{ /* unk status; pre-transmit */
	return 0x0000;
}
static WRITE16_HANDLER(dsp_porta_w)
{ /* unk enable; 0 or 1 */
	logerror( "porta_w(0x%04x)\n", data );
}
static WRITE16_HANDLER(dsp_portb_w)
{
	logerror( "portb_w(0x%04x)\n", data );
	if( data!=1 )
	{ /* only 0->1 transition triggers */
		return;
	}
	if( mpDspState->masterDirectDrawSize == 13 )
	{
		int i;
		int sx[4], sy[4], zcode[4];
		int color  = mpDspState->masterDirectDrawBuffer[0];
		for( i=0; i<4; i++ )
		{
			sx[i] = NAMCOS21_POLY_FRAME_WIDTH/2 + (INT16)mpDspState->masterDirectDrawBuffer[i*3+1];
			sy[i] = NAMCOS21_POLY_FRAME_HEIGHT/2 + (INT16)mpDspState->masterDirectDrawBuffer[i*3+2];
			zcode[i] = mpDspState->masterDirectDrawBuffer[i*3+3];
			logerror( "#%d:{%4d,%4d,0x%04x},color=0x%x\n", i,sx[i],sy[i],zcode[i],color );
		}
		if( color&0x8000 )
		{
			namcos21_DrawQuad( sx, sy, zcode, color );
		}
		else
		{
			logerror( "indirection used w/ direct draw?\n" );
		}
	}
	mpDspState->masterDirectDrawSize = 0;
}
static WRITE16_HANDLER(dsp_portc_w)
{ /* enqueue data for direct-drawn poly */
	logerror( "portc_w(0x%04x)\n", data );
	if( mpDspState->masterDirectDrawSize < DSP_BUF_MAX )
	{
		mpDspState->masterDirectDrawBuffer[mpDspState->masterDirectDrawSize++] = data;
	}
	else
	{
		logerror( "portc overflow\n" );
	}
} /* dsp_portc_w */

/***********************************************************/

static UINT16 mCusKeyState;

static WRITE16_HANDLER(dspcuskey_w)
{
	logerror( "0x%08x:cuskey_w(%d)\n", activecpu_get_pc(), data );

	if( namcos2_gametype == NAMCOS21_SOLVALOU )
	{
		if( data == 0x8000 )
		{
			mCusKeyState = 0xffff;
		}
		else if( data == 0x0000 )
		{
			mCusKeyState = 0x0145;
		}
	}
}
static READ16_HANDLER(dspcuskey_r)
{
	UINT16 result = 0;

	if( namcos2_gametype == NAMCOS21_SOLVALOU )
	{
		switch( offset )
		{
		case 0:
			result = mCusKeyState;
			break;
		case 2:
			mCusKeyState = 0xfeba;
			break;
		}
	}
	logerror( "0x%08x:cuskey_r()==0x%04x\n", activecpu_get_pc(), result );
	return result;
}

static ADDRESS_MAP_START( master_dsp_program, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x0000, 0x01ff) AM_READ(MRA16_ROM) AM_BASE(&master_dsp_rom) /* internal ROM */
	AM_RANGE(0x8000, 0x8fff) AM_READ(MRA16_ROM) AM_BASE(&master_dsp_ram) /* uploaded code */
ADDRESS_MAP_END

static ADDRESS_MAP_START( master_dsp_data, ADDRESS_SPACE_DATA, 16 )
	AM_RANGE(0x2000, 0x200f) AM_READ(dspcuskey_r) AM_WRITE(dspcuskey_w)
	AM_RANGE(0x8000, 0xffff) AM_READ(dspram16_r) AM_WRITE(dspram16_w) /* 0x8000 words */
ADDRESS_MAP_END

static ADDRESS_MAP_START( master_dsp_io, ADDRESS_SPACE_IO, 16 )
	AM_RANGE(0x01,0x01) AM_READ(dsp_port1_r)
	AM_RANGE(0x02,0x02) AM_READ(dsp_port2_r) AM_WRITE(dsp_port2_w)
	AM_RANGE(0x03,0x03) AM_READ(dsp_port3_r)
	AM_RANGE(0x04,0x04) AM_WRITE(dsp_port4_w)
	AM_RANGE(0x08,0x08) AM_READ(dsp_port8_r)
	AM_RANGE(0x09,0x09) AM_READ(dsp_port9_r)
	AM_RANGE(0x0a,0x0a) AM_WRITE(dsp_porta_w)
	AM_RANGE(0x0b,0x0b) AM_WRITE(dsp_portb_w)
	AM_RANGE(0x0c,0x0c) AM_WRITE(dsp_portc_w)
	AM_RANGE(TMS32025_HOLD,  TMS32025_HOLD)  AM_READ( MRA16_NOP )
	AM_RANGE(TMS32025_HOLDA, TMS32025_HOLDA) AM_WRITE( MWA16_NOP )
	AM_RANGE(TMS32025_XF,    TMS32025_XF)    AM_WRITE( MWA16_NOP )
ADDRESS_MAP_END

/************************************************************************************/

static void
TransmitWordToSlave( UINT16 data )
{
	if( mpDspState->slaveInputSize < DSP_BUF_MAX )
	{
		mpDspState->slaveInputBuffer[mpDspState->slaveInputSize++] = data;
	}
	else
	{
		logerror( "slave input buffer overflow\n" );
	}
}

/**
 * On real hardware, the behavior below is almost certainly implemented
 * in DSP bios code, triggered from timer interrupts.
 */
static void
TransferDspData( void )
{
	if( mpDspState->masterSourceAddr >= 0x8000 &&
		mpDspState->masterSourceAddr &&
		mpDspState->slaveSourceAddr >= mpDspState->slaveInputSize )
	{
		const INT32 *pPointData = (INT32 *)memory_region( REGION_USER2 );
		const UINT16 *pSource = &namcos21_dspram16[mpDspState->masterSourceAddr - 0x8000];

		mpDspState->slaveInputSize = 0;
		mpDspState->slaveSourceAddr = 0;

		for(;;)
		{
			int i;
			UINT16 code = *pSource++;
			if( code == 0xffff )
			{
				UINT16 addr = *pSource++;
				mpDspState->masterSourceAddr = addr;
				return;
			}
			else if( code == 1 )
			{
				logerror( "PDP NOP?\n" );
				pSource += code;
			}
			else if( code == 0x18 )
			{
				TransmitWordToSlave( code );
				for( i=0; i<code; i++ )
				{
					TransmitWordToSlave( pSource[i] );
				}
				pSource += code;
			}
			else
			{
				UINT16 len = *pSource++;
				INT32 masterAddr = pPointData[code];
				for(;;)
				{
					INT32 subAddr = pPointData[masterAddr++];
					if( subAddr==0xffffff )
					{
						break;
					}
					else
					{
						int primWords = (UINT16)(pPointData[subAddr++]);
						TransmitWordToSlave( len );

						for( i=0; i<len; i++ )
						{
							TransmitWordToSlave( pSource[i] );
						}

						TransmitWordToSlave( primWords );
						for( i=0; i<primWords; i++ )
						{
							TransmitWordToSlave( (UINT16)pPointData[subAddr+i] );
						}
					}
				} /* for(;;) */
				pSource += len;
			} /* if( code ... ) */
		} /* for(;;) */
	}
} /* TransferDspData */

static const INT32 *
FindPrimitiveDataBySurfaceID( int surfID )
{
	const INT32 *pPointData = (INT32 *)memory_region( REGION_USER2 );
	INT32 last = pPointData[0];
	int i;
	for( i=1; i<last; i++ )
	{
		INT32 masterAddr = pPointData[i];
		for(;;)
		{
			INT32 subAddr = pPointData[masterAddr++];
			if( subAddr == 0xffffff )
			{
				break;
			}
			else
			{
				UINT16 len = pPointData[subAddr+0];
				if( len>2 )
				{
					int id = pPointData[subAddr+1];
					UINT16 vertexCount = pPointData[subAddr+2];
					if( id == surfID )
					{
						unsigned offs = subAddr + 4 + vertexCount*3;
						return &pPointData[offs];
					}
				}
			}
		}
	}
	return NULL;
} /* FindPrimitiveDataBySurfaceID */

static void
RenderSlaveOutput( UINT16 data )
{
	if( mpDspState->slaveOutputSize >= 4096 )
	{
		logerror( "FATAL ERROR: SLAVE OVERFLOW\n" );
		return;
	}
	mpDspState->slaveOutputBuffer[mpDspState->slaveOutputSize++] = data;
	if(1)
	{
		UINT16 *pSource = mpDspState->slaveOutputBuffer;
		UINT16 count = *pSource++;
		if( count && mpDspState->slaveOutputSize > count )
		{
			UINT16 surfaceID = *pSource++;
			UINT16 *pVertexList = pSource;
			int sx[4], sy[4],zcode[4];
			int j;
			if( surfaceID&0x8000 )
			{
				for( j=0; j<4; j++ )
				{
					sx[j] = NAMCOS21_POLY_FRAME_WIDTH/2 + (INT16)pVertexList[3*j+0];
					sy[j] = NAMCOS21_POLY_FRAME_HEIGHT/2 + (INT16)pVertexList[3*j+1];
					zcode[j] = pVertexList[3*j+3];
				}
				namcos21_DrawQuad( sx, sy, zcode, surfaceID&0x7fff );
			}
			else if( surfaceID )
			{
				const INT32 *pSurf = FindPrimitiveDataBySurfaceID( surfaceID );
				if( pSurf )
				{
					UINT16 surfCount = *pSurf++;
					while( surfCount-- )
					{
						UINT16 color;
						for( j=0; j<4; j++ )
						{
							UINT16 vi = *pSurf++;
							sx[j] = NAMCOS21_POLY_FRAME_WIDTH/2  + (INT16)pVertexList[3*vi+0];
							sy[j] = NAMCOS21_POLY_FRAME_HEIGHT/2 + (INT16)pVertexList[3*vi+1];
							zcode[j] = pVertexList[3*vi+2];
						}
						color = *pSurf++;
						namcos21_DrawQuad( sx, sy, zcode, color );
					} /* while( surfCount) */
				} /* if( pSurf ) */
			} /* if( surfaceID ) */
			mpDspState->slaveOutputSize = 0;
		}
	}
} /* RenderSlaveOutput */

static size_t
GetInputBytesAvailableForSlave( void )
{
	return mpDspState->slaveInputSize - mpDspState->slaveSourceAddr;
} /* GetInputBytesAvailableForSlave */

static READ16_HANDLER(slave_port2_r)
{
	UINT16 result;
	TransferDspData();
	result = GetInputBytesAvailableForSlave();
	if( result )
	{
		logerror( "%x : slave port2_r : 0x%04x\n", activecpu_get_pc(), result );
	}
	return result;
} /* slave_port2_r */

static READ16_HANDLER(slave_port0_r)
{ /* read data */
	UINT16 result;
	TransferDspData();
	if( GetInputBytesAvailableForSlave()>0 )
	{ /* data is available */
		result = mpDspState->slaveInputBuffer[mpDspState->slaveSourceAddr++];
	}
	else
	{
		result =0;
	}
	logerror( "%x : slave port0_r : 0x%04x\n", activecpu_get_pc(), result );
	return result;
} /* slave_port0_r */

static READ16_HANDLER(slave_port3_r)
{ /* number of bytes in output buffer (slave blocks if too large) */
	UINT16 result = 0;
	return result;
}

static WRITE16_HANDLER(slave_port0_w)
{
	logerror( "%x : slave port0_w : 0x%04x\n", activecpu_get_pc(), data );
	RenderSlaveOutput( data );
}

static WRITE16_HANDLER(slave_port3_w)
{ /* ack? */
	logerror( "%x : slave port3_w : 0x%04x\n", activecpu_get_pc(), data );
} /* slave_port3_w */

static WRITE16_HANDLER( slave_XF_output_w )
{
	/* Trigger update of framebuffer and reset slave/master.
     * This is probably NOT how real hardware works.
     */
	if( data==0 )
	{
		UpdatePolyFrameBufferAndResetDSPs();
	}
} /* slave_XF_output_w */

static ADDRESS_MAP_START( slave_dsp_program, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x0000, 0x01ff) AM_READ(MRA16_ROM) AM_BASE(&slave_dsp_rom) /* internal ROM */
	AM_RANGE(0x8000, 0x8fff) AM_READ(MRA16_ROM) AM_BASE(&slave_dsp_ram) /* uploaded code */
ADDRESS_MAP_END

static ADDRESS_MAP_START( slave_dsp_data, ADDRESS_SPACE_DATA, 16 )
ADDRESS_MAP_END

static ADDRESS_MAP_START( slave_dsp_io, ADDRESS_SPACE_IO, 16 )
	AM_RANGE(0x00,0x00) AM_READ(slave_port0_r) AM_WRITE(slave_port0_w)
	AM_RANGE(0x02,0x02) AM_READ(slave_port2_r)
	AM_RANGE(0x03,0x03) AM_READ(slave_port3_r) AM_WRITE(slave_port3_w)
	AM_RANGE(TMS32025_HOLD,  TMS32025_HOLD)  AM_READ(MRA16_NOP)
	AM_RANGE(TMS32025_HOLDA, TMS32025_HOLDA) AM_WRITE(MWA16_NOP)
	AM_RANGE(TMS32025_XF,    TMS32025_XF)    AM_WRITE(slave_XF_output_w)
ADDRESS_MAP_END

/************************************************************************************/

static WRITE16_HANDLER( namcos21_dsp_control_w )
{ /* serial output; used to pump bits and possibly to trigger IRQs on master DSP */
	static int code;
	static int prev;
	int cur;
	COMBINE_DATA( &namcos21_dsp_control[offset] );
	cur = namcos21_dsp_control[offset];
	if( (prev^cur)&4 )
	{
		code<<=1;
		code |= cur&1;
		code &= 0x1ff;
	}
}

static READ16_HANDLER( namcos21_dsp_control_r )
{
/*  logerror( "dsp control_r%x:%x[%x]==%04x\n",
        cpu_getactivecpu(),
        activecpu_get_pc(),
        offset,
        namcos21_dsp_control[offset] ); */
	return namcos21_dsp_control[offset];
}

#define PTRAM_SIZE 0x20000
static UINT8 *ptram;
static unsigned int ptram_addr;

static WRITE16_HANDLER( namcos21_dsp_data_w )
{
	ptram[ptram_addr++] = data;
	ptram_addr &= (PTRAM_SIZE-1);
}

static READ16_HANDLER( namcos21_dsp_data_r )
{
/*  logerror( "dsp data_r%x:%x[%x]==%04x\n",
        cpu_getactivecpu(),
        activecpu_get_pc(),
        offset,
        ptram[ptram_addr] ); */
	return ptram[ptram_addr];
}

/* dual port ram memory handlers */

static READ16_HANDLER( namcos2_68k_dualportram_word_r )
{
	return mpDualPortRAM[offset];
}

static WRITE16_HANDLER( namcos2_68k_dualportram_word_w )
{
	if( ACCESSING_LSB )
	{
		mpDualPortRAM[offset] = data&0xff;
	}
}

static READ8_HANDLER( namcos2_dualportram_byte_r )
{
	return mpDualPortRAM[offset];
}

static WRITE8_HANDLER( namcos2_dualportram_byte_w )
{
	mpDualPortRAM[offset] = data;
}

/* shared RAM memory handlers */

static READ16_HANDLER( shareram1_r )
{
	return mpSharedRAM1[offset];
}

static WRITE16_HANDLER( shareram1_w )
{
	COMBINE_DATA( &mpSharedRAM1[offset] );
}

/* some games have read-only areas where more ROMs are mapped */

static READ16_HANDLER( data_r )
{
	return mpDataROM[offset];
}

static READ16_HANDLER( data2_r )
{
	return mpDataROM[0x100000/2+offset];
}

/* palette memory handlers */

static READ16_HANDLER( paletteram16_r )
{
	return paletteram16[offset];
}

static WRITE16_HANDLER( paletteram16_w )
{
	COMBINE_DATA(&paletteram16[offset]);
}

/******************************************************************************/
static WRITE16_HANDLER( NAMCO_C139_SCI_buffer_w ){}
static READ16_HANDLER( NAMCO_C139_SCI_buffer_r ){ return 0; }

static WRITE16_HANDLER( NAMCO_C139_SCI_register_w ){}
static READ16_HANDLER( NAMCO_C139_SCI_register_r ){ return 0; }
/******************************************************************************/

static WRITE16_HANDLER( unk_48000X_w )
{
	/* 0x400 bytes - gamma table? */
}
static UINT16 namcos21_unk_reg;
WRITE16_HANDLER( namcos21_unk_reg_w )
{
	/* 0x40 or 0x00 */
	COMBINE_DATA( &namcos21_unk_reg );
}
static READ16_HANDLER( namcos21_unk_reg_r )
{
	/* 0x40 or 0x00 */
	return namcos21_unk_reg;
}

/*************************************************************/
/* MASTER 68000 CPU Memory declarations                      */
/*************************************************************/
static ADDRESS_MAP_START( namcos21_68k_master, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x0fffff) AM_READ(MRA16_ROM) AM_WRITE(MWA16_ROM)
	AM_RANGE(0x100000, 0x10ffff) AM_READ(MRA16_RAM) AM_WRITE(MWA16_RAM) /* private work RAM */
	AM_RANGE(0x180000, 0x183fff) AM_READ(NAMCOS2_68K_EEPROM_R) AM_WRITE(NAMCOS2_68K_EEPROM_W) AM_BASE(&namcos2_eeprom) AM_SIZE(&namcos2_eeprom_size)
	AM_RANGE(0x1c0000, 0x1fffff) AM_READ(namcos2_68k_master_C148_r) AM_WRITE(namcos2_68k_master_C148_w)
ADDRESS_MAP_END

static ADDRESS_MAP_START( namcos21_68k_slave, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x0fffff) AM_READ(MRA16_ROM) AM_WRITE(MWA16_ROM)
	AM_RANGE(0x100000, 0x13ffff) AM_READ(MRA16_RAM) AM_WRITE(MWA16_RAM) /* private work RAM */
	AM_RANGE(0x1c0000, 0x1fffff) AM_READ(namcos2_68k_slave_C148_r) AM_WRITE(namcos2_68k_slave_C148_w)
ADDRESS_MAP_END

static ADDRESS_MAP_START( namcos21_68k_common, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x200000, 0x20ffff) AM_READ(dspram16_r) AM_WRITE(dspram16_w) AM_BASE(&namcos21_dspram16)
	AM_RANGE(0x280000, 0x280001) AM_WRITE(MWA16_NOP) /* written once on startup */
	AM_RANGE(0x400000, 0x400001) AM_READ(namcos21_dsp_control_r) AM_WRITE(namcos21_dsp_control_w)
	AM_RANGE(0x440000, 0x440001) AM_READ(namcos21_dsp_data_r) AM_WRITE(namcos21_dsp_data_w)
//  AM_RANGE(0x440002, 0x44ffff) AM_WRITE(MWA16_NOP) /* (?) aircombj */
//  AM_RANGE(0x480000, 0x4807ff) AM_WRITE(unk_48000X_w)
	AM_RANGE(0x700000, 0x71ffff) AM_READ(namco_obj16_r) AM_WRITE(namco_obj16_w)
	AM_RANGE(0x720000, 0x720007) AM_READ(namco_spritepos16_r) AM_WRITE(namco_spritepos16_w)
	AM_RANGE(0x740000, 0x75ffff) AM_READ(paletteram16_r ) AM_WRITE(paletteram16_w) AM_BASE(&paletteram16)
	AM_RANGE(0x760000, 0x760001) AM_READ(namcos21_unk_reg_r) AM_WRITE(namcos21_unk_reg_w)
	AM_RANGE(0x800000, 0x8fffff) AM_READ(data_r)
	AM_RANGE(0x900000, 0x90ffff) AM_READ(shareram1_r) AM_WRITE(shareram1_w) AM_BASE(&mpSharedRAM1)
	AM_RANGE(0xa00000, 0xa00fff) AM_READ(namcos2_68k_dualportram_word_r) AM_WRITE(namcos2_68k_dualportram_word_w)
	AM_RANGE(0xb00000, 0xb03fff) AM_READ(NAMCO_C139_SCI_buffer_r) AM_WRITE(NAMCO_C139_SCI_buffer_w)
	AM_RANGE(0xb80000, 0xb8000f) AM_READ(NAMCO_C139_SCI_register_r) AM_WRITE(NAMCO_C139_SCI_register_w)
	AM_RANGE(0xc00000, 0xcfffff) AM_READ(data2_r) /* Cyber Sled */
	AM_RANGE(0xd00000, 0xdfffff) AM_READ(data2_r)
	AM_RANGE(0xfffffe, 0xffffff) AM_READ(uploadhack_r)
ADDRESS_MAP_END

/*************************************************************
 * Winning Run is prototype "System21" hardware that shares
 * very little with the final design.
 *************************************************************/

/* TBA: add support for more than two CPUs to C148 emulation */
static UINT16 gpu_vblank_irq_level;

static READ16_HANDLER( gpu_c148_r )
{
	offs_t addr = ((offset*2)+0x1c0000)&0x1fe000;
	switch( addr )
	{
	default:
		break;
	}
	return 0;
}

static WRITE16_HANDLER( gpu_c148_w )
{
	offs_t addr = ((offset*2)+0x1c0000)&0x1fe000;
	switch( addr )
	{
	case 0x1ca000:
		/* NAMCOS2_C148_POSIRQ level?*/
		break;

	case 0x1ce000:
		COMBINE_DATA( &gpu_vblank_irq_level );
		break;

	case 0x1e2000:
		break;

	default:
		break;
	}
}

static INTERRUPT_GEN( namcos2_gpu_vblank )
{
	cpunum_set_input_line(CPU_GPU, gpu_vblank_irq_level, HOLD_LINE);
}

static READ16_HANDLER( gpu_data_r )
{
	const UINT16 *pSrc = (UINT16 *)memory_region( REGION_USER3 );
	return pSrc[offset];
}

static ADDRESS_MAP_START( readmem_gpu_winrun, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x07ffff) AM_READ(MRA16_ROM)
	AM_RANGE(0x100000, 0x100001) AM_READ(MRA16_RAM) /* shareram? */
	AM_RANGE(0x180000, 0x19ffff) AM_READ(MRA16_RAM) /* private work RAM */
	AM_RANGE(0x1c0000, 0x1fffff) AM_READ(gpu_c148_r)
	AM_RANGE(0x200000, 0x20ffff) AM_READ(MRA16_RAM)
	AM_RANGE(00400000, 0x40ffff) AM_READ(MRA16_RAM)
	AM_RANGE(0x410000, 0x41ffff) AM_READ(MRA16_RAM)
	AM_RANGE(0x600000, 0x6fffff) AM_READ(gpu_data_r)
	AM_RANGE(0xc00000, 0xcfffff) AM_READ(MRA16_RAM) /* frame buffer */
	AM_RANGE(0xd00000, 0xd0000f) AM_READ(MRA16_RAM) /* ? */
ADDRESS_MAP_END

static ADDRESS_MAP_START( writemem_gpu_winrun, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x07ffff) AM_WRITE(MWA16_ROM)
	AM_RANGE(0x100000, 0x100001) AM_WRITE(MWA16_RAM) /* shareram? */
	AM_RANGE(0x180000, 0x19ffff) AM_WRITE(MWA16_RAM) /* private work RAM */
	AM_RANGE(0x1c0000, 0x1fffff) AM_WRITE(gpu_c148_w)
	AM_RANGE(0x200000, 0x20ffff) AM_WRITE(MWA16_RAM) AM_BASE(&namcos21_dspram16)
	AM_RANGE(00400000, 0x40ffff) AM_WRITE(MWA16_RAM)
	AM_RANGE(0x410000, 0x41ffff) AM_WRITE(MWA16_RAM)
	AM_RANGE(0xc00000, 0xcfffff) AM_WRITE(MWA16_RAM) AM_BASE(&videoram16)
	AM_RANGE(0xd00000, 0xd0000f) AM_WRITE(MWA16_RAM) /* ? */
	AM_RANGE(0xe0000c, 0xe0000d) AM_WRITE(MWA16_NOP) /* ? */
ADDRESS_MAP_END

static ADDRESS_MAP_START( readmem_master_winrun, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x03ffff) AM_READ(MRA16_ROM)
	AM_RANGE(0x100000, 0x10ffff) AM_READ(MRA16_RAM) /* private work RAM */
	AM_RANGE(0x180000, 0x183fff) AM_READ(NAMCOS2_68K_EEPROM_R)
	AM_RANGE(0x1c0000, 0x1fffff) AM_READ(namcos2_68k_master_C148_r)
	AM_RANGE(0x250000, 0x25ffff) AM_READ(MRA16_RAM) // sinetable?
	AM_RANGE(0x280000, 0x281fff) AM_READ(dspram16_r)
	AM_RANGE(0x380000, 0x38000f) AM_READ(MRA16_RAM)
	AM_RANGE(0x3c0000, 0x3c0fff) AM_READ(MRA16_RAM)
	AM_RANGE(0x400000, 0x400001) AM_READ(namcos21_dsp_control_r)
	AM_RANGE(0x440000, 0x440001) AM_READ(namcos21_dsp_data_r)
	AM_RANGE(0x600000, 0x600fff) AM_READ(MRA16_RAM)
	AM_RANGE(0x700000, 0x700001) AM_READ(MRA16_RAM)
	AM_RANGE(0x800000, 0x87ffff) AM_READ(data_r)
	AM_RANGE(0x900000, 0x90ffff) AM_READ(shareram1_r)
	AM_RANGE(0xa00000, 0xa00fff) AM_READ(namcos2_68k_dualportram_word_r)
	AM_RANGE(0xb00000, 0xb03fff) AM_READ(NAMCO_C139_SCI_buffer_r)
	AM_RANGE(0xb80000, 0xb8000f) AM_READ(NAMCO_C139_SCI_register_r)
ADDRESS_MAP_END

static ADDRESS_MAP_START( writemem_master_winrun, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x03ffff) AM_WRITE(MWA16_ROM)
	AM_RANGE(0x100000, 0x10ffff) AM_WRITE(MWA16_RAM) /* private work RAM */
	AM_RANGE(0x180000, 0x183fff) AM_WRITE(NAMCOS2_68K_EEPROM_W) AM_BASE(&namcos2_eeprom) AM_SIZE(&namcos2_eeprom_size)
	AM_RANGE(0x1c0000, 0x1fffff) AM_WRITE(namcos2_68k_master_C148_w)
	AM_RANGE(0x250000, 0x25ffff) AM_WRITE(MWA16_RAM)
	AM_RANGE(0x280000, 0x281fff) AM_WRITE(dspram16_w)
	AM_RANGE(0x380000, 0x38000f) AM_WRITE(MWA16_RAM)
	AM_RANGE(0x3c0000, 0x3c0fff) AM_WRITE(MWA16_RAM)
	AM_RANGE(0x400000, 0x400001) AM_WRITE(namcos21_dsp_control_w)
	AM_RANGE(0x440000, 0x440001) AM_WRITE(namcos21_dsp_data_w)
//  AM_RANGE(0x600000, 0x600fff) AM_WRITE(MWA16_RAM)
	AM_RANGE(0x700000, 0x700001) AM_WRITE(MWA16_RAM)
	AM_RANGE(0x900000, 0x90ffff) AM_WRITE(shareram1_w) AM_BASE(&mpSharedRAM1)
	AM_RANGE(0xa00000, 0xa00fff) AM_WRITE(namcos2_68k_dualportram_word_w)
	AM_RANGE(0xb00000, 0xb03fff) AM_WRITE(NAMCO_C139_SCI_buffer_w)
	AM_RANGE(0xb80000, 0xb8000f) AM_WRITE(NAMCO_C139_SCI_register_w)
ADDRESS_MAP_END

static ADDRESS_MAP_START( readmem_slave_winrun, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x03ffff) AM_READ(MRA16_ROM)
	AM_RANGE(0x100000, 0x13ffff) AM_READ(MRA16_RAM)
	AM_RANGE(0x1c0000, 0x1fffff) AM_READ(namcos2_68k_slave_C148_r)
	AM_RANGE(0x280000, 0x281fff) AM_READ(dspram16_r)
	AM_RANGE(0x400000, 0x400001) AM_READ(namcos21_dsp_control_r)
	AM_RANGE(0x440000, 0x440001) AM_READ(namcos21_dsp_data_r)
//  AM_RANGE(0x600000, 0x600fff) AM_READ(MRA16_RAM)
	AM_RANGE(0x800000, 0x87ffff) AM_READ(data_r)
	AM_RANGE(0x900000, 0x90ffff) AM_READ(shareram1_r)
	AM_RANGE(0xa00000, 0xa00fff) AM_READ(namcos2_68k_dualportram_word_r)
	AM_RANGE(0xb00000, 0xb03fff) AM_READ(NAMCO_C139_SCI_buffer_r)
	AM_RANGE(0xb80000, 0xb8000f) AM_READ(NAMCO_C139_SCI_register_r)
ADDRESS_MAP_END

static ADDRESS_MAP_START( writemem_slave_winrun, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x100000, 0x13ffff) AM_WRITE(MWA16_RAM)
	AM_RANGE(0x1c0000, 0x1fffff) AM_WRITE(namcos2_68k_slave_C148_w)
	AM_RANGE(0x280000, 0x281fff) AM_WRITE(dspram16_w)
	AM_RANGE(0x400000, 0x400001) AM_WRITE(namcos21_dsp_control_w)
	AM_RANGE(0x440000, 0x440001) AM_WRITE(namcos21_dsp_data_w)
//  AM_RANGE(0x600000, 0x600fff) AM_WRITE(MWA16_RAM)
	AM_RANGE(0x900000, 0x90ffff) AM_WRITE(shareram1_w)
	AM_RANGE(0xa00000, 0xa00fff) AM_WRITE(namcos2_68k_dualportram_word_w)
	AM_RANGE(0xb00000, 0xb03fff) AM_WRITE(NAMCO_C139_SCI_buffer_w)
	AM_RANGE(0xb80000, 0xb8000f) AM_WRITE(NAMCO_C139_SCI_register_w)
ADDRESS_MAP_END

/*************************************************************/
/* SOUND 6809 CPU Memory declarations                        */
/*************************************************************/

static ADDRESS_MAP_START( readmem_sound, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x3fff) AM_READ(BANKED_SOUND_ROM_R) /* banked */
	AM_RANGE(0x4000, 0x4001) AM_READ(YM2151_status_port_0_r)
	AM_RANGE(0x5000, 0x6fff) AM_READ(C140_r)
	AM_RANGE(0x7000, 0x77ff) AM_READ(namcos2_dualportram_byte_r)
	AM_RANGE(0x7800, 0x7fff) AM_READ(namcos2_dualportram_byte_r)	/* mirror */
	AM_RANGE(0x8000, 0x9fff) AM_READ(MRA8_RAM)
	AM_RANGE(0xd000, 0xffff) AM_READ(MRA8_ROM)
ADDRESS_MAP_END

static ADDRESS_MAP_START( writemem_sound, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x3fff) AM_WRITE(MWA8_ROM)
	AM_RANGE(0x4000, 0x4000) AM_WRITE(YM2151_register_port_0_w)
	AM_RANGE(0x4001, 0x4001) AM_WRITE(YM2151_data_port_0_w)
	AM_RANGE(0x5000, 0x6fff) AM_WRITE(C140_w)
	AM_RANGE(0x7000, 0x77ff) AM_WRITE(namcos2_dualportram_byte_w) AM_BASE(&mpDualPortRAM)
	AM_RANGE(0x7800, 0x7fff) AM_WRITE(namcos2_dualportram_byte_w) /* mirror */
	AM_RANGE(0x8000, 0x9fff) AM_WRITE(MWA8_RAM)
	AM_RANGE(0xa000, 0xbfff) AM_WRITE(MWA8_NOP) /* amplifier enable on 1st write */
	AM_RANGE(0xc000, 0xc001) AM_WRITE(namcos2_sound_bankselect_w)
	AM_RANGE(0xd001, 0xd001) AM_WRITE(MWA8_NOP) /* watchdog */
	AM_RANGE(0xc000, 0xffff) AM_WRITE(MWA8_NOP) /* avoid debug log noise; games write frequently to 0xe000 */
ADDRESS_MAP_END

/*************************************************************/
/* I/O HD63705 MCU Memory declarations                       */
/*************************************************************/

static ADDRESS_MAP_START( readmem_mcu, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x0000) AM_READ(MRA8_NOP)
	AM_RANGE(0x0001, 0x0001) AM_READ(input_port_0_r)			/* p1,p2 start */
	AM_RANGE(0x0002, 0x0002) AM_READ(input_port_1_r)			/* coins */
	AM_RANGE(0x0003, 0x0003) AM_READ(namcos2_mcu_port_d_r)
	AM_RANGE(0x0007, 0x0007) AM_READ(input_port_10_r)		/* fire buttons */
	AM_RANGE(0x0010, 0x0010) AM_READ(namcos2_mcu_analog_ctrl_r)
	AM_RANGE(0x0011, 0x0011) AM_READ(namcos2_mcu_analog_port_r)
	AM_RANGE(0x0008, 0x003f) AM_READ(MRA8_RAM)
	AM_RANGE(0x0040, 0x01bf) AM_READ(MRA8_RAM)
	AM_RANGE(0x01c0, 0x1fff) AM_READ(MRA8_ROM)
	AM_RANGE(0x2000, 0x2000) AM_READ(input_port_11_r) /* dipswitches */
	AM_RANGE(0x3000, 0x3000) AM_READ(input_port_12_r) /* DIAL0 */
	AM_RANGE(0x3001, 0x3001) AM_READ(input_port_13_r) /* DIAL1 */
	AM_RANGE(0x3002, 0x3002) AM_READ(input_port_14_r) /* DIAL2 */
	AM_RANGE(0x3003, 0x3003) AM_READ(input_port_15_r) /* DIAL3 */
	AM_RANGE(0x5000, 0x57ff) AM_READ(namcos2_dualportram_byte_r)
	AM_RANGE(0x6000, 0x6fff) AM_READ(MRA8_NOP) /* watchdog */
	AM_RANGE(0x8000, 0xffff) AM_READ(MRA8_ROM)
ADDRESS_MAP_END

static ADDRESS_MAP_START( writemem_mcu, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0003, 0x0003) AM_WRITE(namcos2_mcu_port_d_w)
	AM_RANGE(0x0010, 0x0010) AM_WRITE(namcos2_mcu_analog_ctrl_w)
	AM_RANGE(0x0011, 0x0011) AM_WRITE(namcos2_mcu_analog_port_w)
	AM_RANGE(0x0000, 0x003f) AM_WRITE(MWA8_RAM)
	AM_RANGE(0x0040, 0x01bf) AM_WRITE(MWA8_RAM)
	AM_RANGE(0x01c0, 0x1fff) AM_WRITE(MWA8_ROM)
	AM_RANGE(0x5000, 0x57ff) AM_WRITE(namcos2_dualportram_byte_w) AM_BASE(&mpDualPortRAM)
	AM_RANGE(0x8000, 0xffff) AM_WRITE(MWA8_ROM)
ADDRESS_MAP_END

static const gfx_layout tile_layout =
{
	16,16,
	RGN_FRAC(1,4),	/* number of tiles */
	8,		/* bits per pixel */
	{		/* plane offsets */
		0,1,2,3,4,5,6,7
	},
	{ /* x offsets */
		0*8,RGN_FRAC(1,4)+0*8,RGN_FRAC(2,4)+0*8,RGN_FRAC(3,4)+0*8,
		1*8,RGN_FRAC(1,4)+1*8,RGN_FRAC(2,4)+1*8,RGN_FRAC(3,4)+1*8,
		2*8,RGN_FRAC(1,4)+2*8,RGN_FRAC(2,4)+2*8,RGN_FRAC(3,4)+2*8,
		3*8,RGN_FRAC(1,4)+3*8,RGN_FRAC(2,4)+3*8,RGN_FRAC(3,4)+3*8
	},
	{ /* y offsets */
		0*32,1*32,2*32,3*32,
		4*32,5*32,6*32,7*32,
		8*32,9*32,10*32,11*32,
		12*32,13*32,14*32,15*32
	},
	8*64 /* sprite offset */
};

static const gfx_decode gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0x000000, &tile_layout,  0x1000, 0x10 },
	{ -1 }
};

static struct C140interface C140_interface_typeA =
{
	C140_TYPE_SYSTEM21_A,
	REGION_SOUND1
};

static struct C140interface C140_interface_typeB =
{
	C140_TYPE_SYSTEM21_B,
	REGION_SOUND1
};

static MACHINE_DRIVER_START( s21base )
	MDRV_CPU_ADD(M68000,12288000) /* Master */
	MDRV_CPU_PROGRAM_MAP(namcos21_68k_master, namcos21_68k_common)
	MDRV_CPU_VBLANK_INT(namcos2_68k_master_vblank,1)

	MDRV_CPU_ADD(M68000,12288000) /* Slave */
	MDRV_CPU_PROGRAM_MAP(namcos21_68k_slave, namcos21_68k_common)
	MDRV_CPU_VBLANK_INT(namcos2_68k_slave_vblank,1)

	MDRV_CPU_ADD(M6809,3072000) /* Sound */
	MDRV_CPU_PROGRAM_MAP(readmem_sound,writemem_sound)
	MDRV_CPU_VBLANK_INT(irq0_line_hold,2)
	MDRV_CPU_PERIODIC_INT(irq1_line_hold,TIME_IN_HZ(120))

	MDRV_CPU_ADD(HD63705,2048000) /* IO */
	MDRV_CPU_PROGRAM_MAP(readmem_mcu,writemem_mcu)
	MDRV_CPU_VBLANK_INT(irq0_line_hold,1)

	MDRV_CPU_ADD(TMS32025,24000000) /* 24 MHz? overclocked */
	MDRV_CPU_PROGRAM_MAP(master_dsp_program,0)
	MDRV_CPU_DATA_MAP(master_dsp_data,0)
	MDRV_CPU_IO_MAP(master_dsp_io,0)

	MDRV_CPU_ADD(TMS32025,24000000) /* 24 MHz?; overclocked */
	MDRV_CPU_PROGRAM_MAP(slave_dsp_program,0)
	MDRV_CPU_DATA_MAP(slave_dsp_data,0)
	MDRV_CPU_IO_MAP(slave_dsp_io,0)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_REAL_60HZ_VBLANK_DURATION)
	MDRV_INTERLEAVE(10/*0*/)

	MDRV_MACHINE_RESET(namcos2)
	MDRV_NVRAM_HANDLER(namcos2)

	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER | VIDEO_NEEDS_6BITS_PER_GUN)
	MDRV_SCREEN_SIZE(NAMCOS21_POLY_FRAME_WIDTH,NAMCOS21_POLY_FRAME_HEIGHT)
	MDRV_VISIBLE_AREA(0,495,0,479)

	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(NAMCOS21_NUM_COLORS)
	MDRV_COLORTABLE_LENGTH(NAMCOS21_NUM_COLORS)

	MDRV_VIDEO_START(namcos21)
	MDRV_VIDEO_UPDATE(namcos21_default)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( poly_c140_typeA )
	MDRV_IMPORT_FROM(s21base)

	MDRV_SPEAKER_STANDARD_STEREO("left", "right")

	MDRV_SOUND_ADD(C140, 8000000/374)
	MDRV_SOUND_CONFIG(C140_interface_typeA)
	MDRV_SOUND_ROUTE(0, "left", 0.50)
	MDRV_SOUND_ROUTE(1, "right", 0.50)

	MDRV_SOUND_ADD(YM2151, 3579580)
	MDRV_SOUND_ROUTE(0, "left", 0.30)
	MDRV_SOUND_ROUTE(1, "right", 0.30)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( poly_c140_typeB )
	MDRV_IMPORT_FROM(s21base)

	MDRV_SPEAKER_STANDARD_STEREO("left", "right")

	MDRV_SOUND_ADD(C140, 8000000/374)
	MDRV_SOUND_CONFIG(C140_interface_typeB)
	MDRV_SOUND_ROUTE(0, "left", 0.50)
	MDRV_SOUND_ROUTE(1, "right", 0.50)

	MDRV_SOUND_ADD(YM2151, 3579580)
	MDRV_SOUND_ROUTE(0, "left", 0.30)
	MDRV_SOUND_ROUTE(1, "right", 0.30)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( winrun_c140_typeB )
	MDRV_CPU_ADD(M68000,12288000) /* Master */
	MDRV_CPU_PROGRAM_MAP(readmem_master_winrun,writemem_master_winrun)
	MDRV_CPU_VBLANK_INT(namcos2_68k_master_vblank,1)

	MDRV_CPU_ADD(M68000,12288000) /* Slave */
	MDRV_CPU_PROGRAM_MAP(readmem_slave_winrun,writemem_slave_winrun)
	MDRV_CPU_VBLANK_INT(namcos2_68k_slave_vblank,1)

	MDRV_CPU_ADD(M6809,3072000) /* Sound */
	MDRV_CPU_PROGRAM_MAP(readmem_sound,writemem_sound)
	MDRV_CPU_VBLANK_INT(irq0_line_hold,2)
	MDRV_CPU_PERIODIC_INT(irq1_line_hold,TIME_IN_HZ(120))

	MDRV_CPU_ADD(HD63705,2048000) /* IO */
	MDRV_CPU_PROGRAM_MAP(readmem_mcu,writemem_mcu)
	MDRV_CPU_VBLANK_INT(irq0_line_hold,1)

	MDRV_CPU_ADD(M68000,12288000) /* graphics coprocessor */
	MDRV_CPU_PROGRAM_MAP(readmem_gpu_winrun,writemem_gpu_winrun)
	MDRV_CPU_VBLANK_INT(namcos2_gpu_vblank,1)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_REAL_60HZ_VBLANK_DURATION)
	MDRV_INTERLEAVE(100) /* 100 CPU slices per frame */

	MDRV_MACHINE_RESET(namcos2)
	MDRV_NVRAM_HANDLER(namcos2)

	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER | VIDEO_NEEDS_6BITS_PER_GUN)
	MDRV_SCREEN_SIZE(512,512)
	MDRV_VISIBLE_AREA(0,511,0,511)

	MDRV_PALETTE_LENGTH(NAMCOS21_NUM_COLORS)
	MDRV_COLORTABLE_LENGTH(NAMCOS21_NUM_COLORS)

	MDRV_VIDEO_START(namcos21)
	MDRV_VIDEO_UPDATE(namcos21_winrun)

	MDRV_SPEAKER_STANDARD_STEREO("left", "right")

	MDRV_SOUND_ADD(C140, 8000000/374)
	MDRV_SOUND_CONFIG(C140_interface_typeB)
	MDRV_SOUND_ROUTE(0, "left", 0.50)
	MDRV_SOUND_ROUTE(1, "right", 0.50)

	MDRV_SOUND_ADD(YM2151, 3579580)
	MDRV_SOUND_ROUTE(0, "left", 0.30)
	MDRV_SOUND_ROUTE(1, "right", 0.30)
MACHINE_DRIVER_END

ROM_START( aircombu )
	ROM_REGION( 0x100000, REGION_CPU1, 0 ) /* Master */
	ROM_LOAD16_BYTE( "ac2-mpr-u.3j",  0x000000, 0x80000, CRC(a7133f85) SHA1(9f1c99dd503f1fc81096170fd272e33ae8a7de2f) )
	ROM_LOAD16_BYTE( "ac2-mpr-l.1j",  0x000001, 0x80000, CRC(520a52e6) SHA1(74306e02abfe08aa1afbf325b74dbc0840c3ad3a) )

	ROM_REGION( 0x100000, REGION_CPU2, 0 ) /* Slave */
	ROM_LOAD16_BYTE( "ac2-spr-u.6c",  0x000000, 0x20000, CRC(42aca956) SHA1(10ea2400bb4d5b2d805e2de43ca0e0f54597f660) )
	ROM_LOAD16_BYTE( "ac2-spr-l.4c",  0x000001, 0x20000, CRC(3e15fa19) SHA1(65dbb33ab6b3c06c793613348ebb7b110b8bba0d) )

	ROM_REGION( 0x030000, REGION_CPU3, 0 ) /* Sound */
	ROM_LOAD( "ac1-snd0.8j",	0x00c000, 0x004000, CRC(5c1fb84b) SHA1(20e4d81289dbe58ffcfc947251a6ff1cc1e36436) )
	ROM_CONTINUE(      		0x010000, 0x01c000 )
	ROM_RELOAD(				0x010000, 0x020000 )

	ROM_REGION( 0x010000, REGION_CPU4, 0 ) /* I/O MCU */
	ROM_LOAD( "sys2mcpu.bin",  0x000000, 0x002000, CRC(a342a97e) SHA1(2c420d34dba21e409bf78ddca710fc7de65a6642) )
	ROM_LOAD( "sys2c65c.bin",  0x008000, 0x008000, CRC(a5b2a4ff) SHA1(068bdfcc71a5e83706e8b23330691973c1c214dc) )


	ROM_REGION( 0x400000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "ac2-obj0.5s",  0x000000, 0x80000, CRC(8327ff22) SHA1(16f6022dedb7a74590898bc8ed3e8a97993c4635) )
	ROM_LOAD( "ac2-obj4.4s",  0x080000, 0x80000, CRC(e433e344) SHA1(98ade550cf066fcb5c09fa905f441a1464d4d625) )
	ROM_LOAD( "ac2-obj1.5x",  0x100000, 0x80000, CRC(43af566d) SHA1(99f0d9f005e28040f5cc10de2198893946a31d09) )
	ROM_LOAD( "ac2-obj5.4x",  0x180000, 0x80000, CRC(ecb19199) SHA1(8e0aa1bc1141c4b09576ab08970d0c7629560643) )
	ROM_LOAD( "ac2-obj2.3s",  0x200000, 0x80000, CRC(dafbf489) SHA1(c53ccb3e1b4a6a660bd28c8abe52ccc3f85d111f) )
	ROM_LOAD( "ac2-obj6.2s",  0x280000, 0x80000, CRC(24cc3f36) SHA1(e50af176eb3034c9cab7613ca614f5cc2c62f95e) )
	ROM_LOAD( "ac2-obj3.3x",  0x300000, 0x80000, CRC(bd555a1d) SHA1(96e432b30da6f5f7ccb768c516b1f7186bc0d4c9) )
	ROM_LOAD( "ac2-obj7.2x",  0x380000, 0x80000, CRC(d561fbe3) SHA1(a23976e10bddf74d4a6b292f044dfd0affbab101) )

	ROM_REGION16_BE( 0x200000, REGION_USER1, 0 ) /* collision */
	ROM_LOAD16_BYTE( "ac1-data-u.3a",		0x000000, 0x80000, CRC(82320c71) SHA1(2be98d46853febb46e1cc728af2735c0e00ce303) )
	ROM_LOAD16_BYTE( "ac1-data-l.1a", 	0x000001, 0x80000, CRC(fd7947d3) SHA1(2696eeae37de6d256e626cc3f3cea7b0f6eff60e) )
	ROM_LOAD16_BYTE( "ac2-edata-1u.3c",	0x100000, 0x80000, CRC(40c07095) SHA1(5d9beaf5bc411ac66785d70980977b08446f46e3) )
	ROM_LOAD16_BYTE( "ac1-edata-1l.1c",	0x100001, 0x80000, CRC(a87087dd) SHA1(cd9b83a8f07886ab44e4ded68002b44338777e8c) )

	ROM_REGION32_BE( 0x400000, REGION_USER2, ROMREGION_ERASE )		/* 24bit signed point data */
	ROM_LOAD32_BYTE( "ac1-poih.2f",   0x000001, 0x80000, CRC(573bbc3b) SHA1(371be12b915db6872049f18980c1b55544cfc445) )	/* most significant */
	ROM_LOAD32_BYTE( "ac1-poil-u.2k", 0x000002, 0x80000, CRC(d99084b9) SHA1(c604d60a2162af7610e5ff7c1aa4195f7df82efe) )
	ROM_LOAD32_BYTE( "ac1-poil-l.2n", 0x000003, 0x80000, CRC(abb32307) SHA1(8e936ba99479215dd33a951d81ec2b04020dfd62) )	/* least significant */

	ROM_REGION( 0x200000, REGION_SOUND1, 0 ) /* sound samples */
	ROM_LOAD("ac1-voi0.12b",0x000000,0x80000,CRC(f427b119) SHA1(bd45bbe41c8be26d6c997fcdc226d080b416a2cf) )
	ROM_LOAD("ac1-voi1.12c",0x080000,0x80000,CRC(c9490667) SHA1(4b6fbe635c32469870a8e6f82742be6a9d4918c9) )
	ROM_LOAD("ac1-voi2.12d",0x100000,0x80000,CRC(1fcb51ba) SHA1(80fc815e5fad76d20c3795ab1d89b57d9abc3efd) )
	ROM_LOAD("ac1-voi3.12e",0x180000,0x80000,CRC(cd202e06) SHA1(72a18f5ba402caefef14b8d1304f337eaaa3eb1d) )
ROM_END

ROM_START( aircombj )
	ROM_REGION( 0x100000, REGION_CPU1, 0 ) /* Master */
	ROM_LOAD16_BYTE( "ac1-mpr-u.3j",  0x000000, 0x80000, CRC(a4dec813) SHA1(2ee8b3492d30db4c841f695151880925a5e205e0) )
	ROM_LOAD16_BYTE( "ac1-mpr-l.1j",  0x000001, 0x80000, CRC(8577b6a2) SHA1(32194e392fbd051754be88eb8c90688c65c65d85) )

	ROM_REGION( 0x100000, REGION_CPU2, 0 ) /* Slave */
	ROM_LOAD16_BYTE( "ac1-spr-u.6c",  0x000000, 0x20000, CRC(5810e219) SHA1(c312ffd8324670897871b12d521779570dc0f580) )
	ROM_LOAD16_BYTE( "ac1-spr-l.4c",  0x000001, 0x20000, CRC(175a7d6c) SHA1(9e31dde6646cd9b6dcdbdb3f2326177508559e56) )

	ROM_REGION( 0x030000, REGION_CPU3, 0 ) /* Sound */
	ROM_LOAD( "ac1-snd0.8j",	0x00c000, 0x004000, CRC(5c1fb84b) SHA1(20e4d81289dbe58ffcfc947251a6ff1cc1e36436) )
	ROM_CONTINUE(			0x010000, 0x01c000 )
	ROM_RELOAD(				0x010000, 0x020000 )

	ROM_REGION( 0x010000, REGION_CPU4, 0 ) /* I/O MCU */
	ROM_LOAD( "sys2mcpu.bin",  0x000000, 0x002000, CRC(a342a97e) SHA1(2c420d34dba21e409bf78ddca710fc7de65a6642) )
	ROM_LOAD( "sys2c65c.bin",  0x008000, 0x008000, CRC(a5b2a4ff) SHA1(068bdfcc71a5e83706e8b23330691973c1c214dc) )

	ROM_REGION( 0x400000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "ac1-obj0.5s",  0x000000, 0x80000, CRC(d2310c6a) SHA1(9bb8fdfc2c232574777248f4959975f9a20e3105) )
	ROM_LOAD( "ac1-obj4.4s",  0x080000, 0x80000, CRC(0c93b478) SHA1(a92ffbcf04b64e0eee5bcf37008e247700641b25) )
	ROM_LOAD( "ac1-obj1.5x",  0x100000, 0x80000, CRC(f5783a77) SHA1(0be1815ceb4ce4fa7ab75ba588e090f20ee0cac9) )
	ROM_LOAD( "ac1-obj5.4x",  0x180000, 0x80000, CRC(476aed15) SHA1(0e53fdf02e8ffe7852a1fa8bd2f64d0e58f3dc09) )
	ROM_LOAD( "ac1-obj2.3s",  0x200000, 0x80000, CRC(01343d5c) SHA1(64171fed1d1f8682b3d70d3233ea017719f4cc63) )
	ROM_LOAD( "ac1-obj6.2s",  0x280000, 0x80000, CRC(c67607b1) SHA1(df64ea7920cf64271fe742d3d0a57f842ee61e8d) )
	ROM_LOAD( "ac1-obj3.3x",  0x300000, 0x80000, CRC(7717f52e) SHA1(be1df3f4d0fdcaa5d3c81a724e5eb9d14136c6f5) )
	ROM_LOAD( "ac1-obj7.2x",  0x380000, 0x80000, CRC(cfa9fe5f) SHA1(0da25663b89d653c87ed32d15f7c82f3035702ab) )

	ROM_REGION16_BE( 0x200000, REGION_USER1, 0 )
	ROM_LOAD16_BYTE( "ac1-data-u.3a",   0x000000, 0x80000, CRC(82320c71) SHA1(2be98d46853febb46e1cc728af2735c0e00ce303) )
	ROM_LOAD16_BYTE( "ac1-data-l.1a",   0x000001, 0x80000, CRC(fd7947d3) SHA1(2696eeae37de6d256e626cc3f3cea7b0f6eff60e) )
	ROM_LOAD16_BYTE( "ac1-edata-1u.3c",  0x100000, 0x80000, CRC(a9547509) SHA1(1bc663cec03b60ad968896bbc2546f02efda135e) )
	ROM_LOAD16_BYTE( "ac1-edata-1l.1c",  0x100001, 0x80000, CRC(a87087dd) SHA1(cd9b83a8f07886ab44e4ded68002b44338777e8c) )

	ROM_REGION32_BE( 0x400000, REGION_USER2, ROMREGION_ERASE )		/* 24bit signed point data */
	ROM_LOAD32_BYTE( "ac1-poih.2f",   0x000001, 0x80000, CRC(573bbc3b) SHA1(371be12b915db6872049f18980c1b55544cfc445) )	/* most significant */
	ROM_LOAD32_BYTE( "ac1-poil-u.2k", 0x000002, 0x80000, CRC(d99084b9) SHA1(c604d60a2162af7610e5ff7c1aa4195f7df82efe) )
	ROM_LOAD32_BYTE( "ac1-poil-l.2n", 0x000003, 0x80000, CRC(abb32307) SHA1(8e936ba99479215dd33a951d81ec2b04020dfd62) )	/* least significant */

	ROM_REGION( 0x200000, REGION_SOUND1, 0 ) /* sound samples */
	ROM_LOAD("ac1-voi0.12b",0x000000,0x80000,CRC(f427b119) SHA1(bd45bbe41c8be26d6c997fcdc226d080b416a2cf) )
	ROM_LOAD("ac1-voi1.12c",0x080000,0x80000,CRC(c9490667) SHA1(4b6fbe635c32469870a8e6f82742be6a9d4918c9) )
	ROM_LOAD("ac1-voi2.12d",0x100000,0x80000,CRC(1fcb51ba) SHA1(80fc815e5fad76d20c3795ab1d89b57d9abc3efd) )
	ROM_LOAD("ac1-voi3.12e",0x180000,0x80000,CRC(cd202e06) SHA1(72a18f5ba402caefef14b8d1304f337eaaa3eb1d) )
ROM_END

ROM_START( cybsled )
	ROM_REGION( 0x100000, REGION_CPU1, 0 ) /* Master */
	ROM_LOAD16_BYTE( "cy1-mpr-u.3j",  0x000000, 0x80000, CRC(cc5a2e83) SHA1(b794051b2c351e9ca43351603845e4e563f6740f) )
	ROM_LOAD16_BYTE( "cy1-mpr-l.1j",  0x000001, 0x80000, CRC(f7ee8b48) SHA1(6d36eb3dba9cf7f5f5e1a26c156e77a2dad3f257) )

	ROM_REGION( 0x100000, REGION_CPU2, 0 ) /* Slave */
	ROM_LOAD16_BYTE( "cy1-spr-u.6c",  0x000000, 0x80000, CRC(28dd707b) SHA1(11297ceae4fe78d170785a5cf9ad77833bbe7fff) )
	ROM_LOAD16_BYTE( "cy1-spr-l.4c",  0x000001, 0x80000, CRC(437029de) SHA1(3d275a2b0ce6909e77e657c371bd22597ea9d398) )

	ROM_REGION( 0x030000, REGION_CPU3, 0 ) /* Sound */
	ROM_LOAD( "cy1-snd0.8j",	0x00c000, 0x004000, CRC(3dddf83b) SHA1(e16119cbef176b6f8f8ace773fcbc201e987823f) )
	ROM_CONTINUE(			0x010000, 0x01c000 )
	ROM_RELOAD(				0x010000, 0x020000 )

	ROM_REGION( 0x010000, REGION_CPU4, 0 ) /* I/O MCU */
	ROM_LOAD( "sys2mcpu.bin",  0x000000, 0x002000, CRC(a342a97e) SHA1(2c420d34dba21e409bf78ddca710fc7de65a6642) )
	ROM_LOAD( "sys2c65c.bin",  0x008000, 0x008000, CRC(a5b2a4ff) SHA1(068bdfcc71a5e83706e8b23330691973c1c214dc) )

	ROM_REGION( 0x400000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "cy1-obj0.5s",  0x000000, 0x80000, CRC(5ae542d5) SHA1(99b1a3ed476da4a97cb864538909d7b831f0fd3b) )
	ROM_LOAD( "cy1-obj4.4s",  0x080000, 0x80000, CRC(57904076) SHA1(b1dc0d99543bc4b9584b37ffc12c6ebc59e30e3b) )
	ROM_LOAD( "cy1-obj1.5x",  0x100000, 0x80000, CRC(4aae3eff) SHA1(c80240bd2f4228a0261a14adb6b10560b31b5aa0) )
	ROM_LOAD( "cy1-obj5.4x",  0x180000, 0x80000, CRC(0e11ca47) SHA1(076a9a4cfddbee2d8aaa06110333090d8fdbefeb) )
	ROM_LOAD( "cy1-obj2.3s",  0x200000, 0x80000, CRC(d64ec4c3) SHA1(0bed1cafc21ed8cef3850fb81e30076977086eb0) )
	ROM_LOAD( "cy1-obj6.2s",  0x280000, 0x80000, CRC(7748b485) SHA1(adb4da419a6cdbefd0fef182d866a3479be379af) )
	ROM_LOAD( "cy1-obj3.3x",  0x300000, 0x80000, CRC(3d1f7168) SHA1(392dddcc79fe61dcc6514a91ac27b5e36825d8b7) )
	ROM_LOAD( "cy1-obj7.2x",  0x380000, 0x80000, CRC(b6eb6ad2) SHA1(85a660c5e44012491be7d4e783cce6ba12c135cb) )

	ROM_REGION16_BE( 0x200000, REGION_USER1, 0 )
	ROM_LOAD16_BYTE( "cy1-data-u.3a",   0x000000, 0x80000, CRC(570da15d) SHA1(9ebe756f10756c079a92fb522332e9e52ff715c3) )
	ROM_LOAD16_BYTE( "cy1-data-l.1a",   0x000001, 0x80000, CRC(9cf96f9e) SHA1(91783f48b93e03c778c6641ca8fb419c13b0d3c5) )
	ROM_LOAD16_BYTE( "cy1-edata0-u.3b", 0x100000, 0x80000, CRC(77452533) SHA1(48fc199bcc1beb23c714eebd9b09b153c980170b) )
	ROM_LOAD16_BYTE( "cy1-edata0-l.1b", 0x100001, 0x80000, CRC(e812e290) SHA1(719e0a026ae8ef63d0d0269b67669ea9b4d950dd) )

	ROM_REGION32_BE( 0x400000, REGION_USER2, ROMREGION_ERASE )		/* 24bit signed point data */
	ROM_LOAD32_BYTE( "cy1-poih-1.2f",  0x000001, 0x80000, CRC(eaf8bac3) SHA1(7a2caf6672af158b4a23ce4626342d1f17d1a4e4) )	/* most significant */
	ROM_LOAD32_BYTE( "cy1-poil-u1.2k", 0x000002, 0x80000, CRC(c544a8dc) SHA1(4cce5f2ab3519b4aa7edbdd15b2d79a7fdcade3c) )
	ROM_LOAD32_BYTE( "cy1-poil-l1.2n", 0x000003, 0x80000, CRC(30acb99b) SHA1(a28dcb3e5405f166644f6353a903c1143ee268f1) )	/* least significant */
	ROM_LOAD32_BYTE( "cy1-poih-2.2j",  0x200001, 0x80000, CRC(4079f342) SHA1(fa36aed1abbda54a42f29b183007474580870319) )
	ROM_LOAD32_BYTE( "cy1-poil-u2.2l", 0x200002, 0x80000, CRC(61d816d4) SHA1(7991957b910d32530151abc7f469fcf1de62d8f3) )
	ROM_LOAD32_BYTE( "cy1-poil-l2.2p", 0x200003, 0x80000, CRC(faf09158) SHA1(b56ebed6012362b1d599c396a43e90a1e4d9dc38) )

	ROM_REGION( 0x200000, REGION_SOUND1, 0 ) /* sound samples */
	ROM_LOAD("cy1-voi0.12b",0x000000,0x80000,CRC(99d7ce46) SHA1(b75f4055c3ce847daabfacda22df14e3f80c4fb9) )
	ROM_LOAD("cy1-voi1.12c",0x080000,0x80000,CRC(2b335f06) SHA1(2b2cd407c34388b56496f84a414daa153780b098) )
	ROM_LOAD("cy1-voi2.12d",0x100000,0x80000,CRC(10cd15f0) SHA1(9b721654ed97a13287373c1b2854ac9aeddc271f) )
	ROM_LOAD("cy1-voi3.12e",0x180000,0x80000,CRC(c902b4a4) SHA1(816357ec1a02a7ebf817ac1182e9c50ce5ca71f6) )
ROM_END

ROM_START( driveyes )
	ROM_REGION( 0x100000, REGION_CPU1, 0 ) /* 68k code */
	ROM_LOAD16_BYTE( "de2_mpub.3j",  0x000000, 0x20000, CRC(f9c86fb5) SHA1(b48d16e8f26e7a2cfecb30285b517c42e5585ac7) )
	ROM_LOAD16_BYTE( "de2_mplb.1j",  0x000001, 0x20000, CRC(11d8587a) SHA1(ecb1e8fe2ba56b6f6a71a5552d5663b597165786) )

	ROM_REGION( 0x100000, REGION_CPU2, 0 ) /* 68k code */
	ROM_LOAD16_BYTE( "de1_spub.6c",  0x000000, 0x20000, CRC(231b144f) SHA1(42518614cb083455dc5fec71e699403907ca784b) )
	ROM_LOAD16_BYTE( "de1_splb.4c",  0x000001, 0x20000, CRC(50cb9f59) SHA1(aec7fa080854f0297d9e90e3aaeb0f332fd579bd) )

	ROM_REGION( 0x30000, REGION_CPU3, 0 ) /* Sound */
/*
There are 3 seperate complete boards used for this 3 screen version....
"Set2" (center screen board?) has de1_snd0 while the other 2 sets have de1_snd0r (rear speakers??)
Only "Set2" has voice roms present/dumped?
We load the "r" set, then load set2's sound CPU code over it to keep the "r" rom in the set
*/
	ROM_LOAD( "de1_snd0r.8j",  0x00c000, 0x004000, CRC(7bbeda42) SHA1(fe840cc9069758928492bbeec79acded18daafd9) ) /* Sets 1 & 3 */
	ROM_CONTINUE(              0x010000, 0x01c000 )
	ROM_RELOAD(                0x010000, 0x020000 )
	ROM_LOAD( "de1_snd0.8j",   0x00c000, 0x004000, CRC(5474f203) SHA1(e0ae2f6978deb0c934d9311a334a6e36bb402aee) ) /* Set 2 */
	ROM_CONTINUE(              0x010000, 0x01c000 )
	ROM_RELOAD(                0x010000, 0x020000 )

	ROM_REGION( 0x10000, REGION_CPU4, 0 ) /* I/O MCU */
	ROM_LOAD( "sys2mcpu.bin",  0x000000, 0x002000, CRC(a342a97e) SHA1(2c420d34dba21e409bf78ddca710fc7de65a6642) )
	ROM_LOAD( "sys2c65c.bin",  0x008000, 0x008000, CRC(a5b2a4ff) SHA1(068bdfcc71a5e83706e8b23330691973c1c214dc) )

	ROM_REGION( 0x200000, REGION_GFX1, ROMREGION_DISPOSE ) /* sprites */
	ROM_LOAD( "de1_obj0.5s",  0x000000, 0x40000, CRC(7438bd53) SHA1(7619c4b56d5c466e845eb45e6157dcaf2a03ad94) )
	ROM_LOAD( "de1_obj1.5x",  0x040000, 0x40000, CRC(45f2334e) SHA1(95f277a4e43d6662ae44d6b69a57f65c72978319) )
	ROM_LOAD( "de1_obj2.3s",  0x080000, 0x40000, CRC(8f1a542c) SHA1(2cb59713607d8929815a9b28bf2a384b6a6c9db8) )
	ROM_LOAD( "de1_obj3.3x",  0x0c0000, 0x40000, CRC(fc94544c) SHA1(6297445c64784ee253716f6438d98e5fcd4e7520) )
	ROM_LOAD( "de1_obj4.4s",  0x100000, 0x40000, CRC(335f0ea4) SHA1(9ec065d99ad0874b262b372334179a7e7612558e) )
	ROM_LOAD( "de1_obj5.4x",  0x140000, 0x40000, CRC(9e22999c) SHA1(02624186c359b5e2c96cd3f0e2cb1598ea36dff7) )
	ROM_LOAD( "de1_obj6.2s",  0x180000, 0x40000, CRC(346df4d5) SHA1(edbadb9db93b7f5a3b064c7f6acb77001cdacce2) )
	ROM_LOAD( "de1_obj7.2x",  0x1c0000, 0x40000, CRC(9ce325d7) SHA1(de4d788bec14842507ed405244974b4fd4f07515) )

	ROM_REGION16_BE( 0x100000, REGION_USER1, 0 )
	ROM_LOAD16_BYTE( "de1_du.3a",  0x00001, 0x80000, CRC(fe65d2ab) SHA1(dbe962dda7efa60357fa3a684a265aaad49df5b5) )
	ROM_LOAD16_BYTE( "de1_dl.1a",  0x00000, 0x80000, CRC(9bb37aca) SHA1(7f5dffc95cadcf12f53ff7944920afc25ed3cf68) )

	ROM_REGION16_BE( 0xc0000, REGION_USER2, 0 ) /* 3d objects */
	ROM_LOAD16_BYTE( "de1pt0ub.8j", 0x00000, 0x20000, CRC(3b6b746d) SHA1(40c992ef4cf5187b30aba42c5fe7ce0f8f02bee0) )
	ROM_LOAD16_BYTE( "de1pt0lb.8d", 0x00001, 0x20000, CRC(9c5c477e) SHA1(c8ae8a663227d636d35bd5f432d23f05d6695942) )
	ROM_LOAD16_BYTE( "de1pt1u.8l",  0x40000, 0x20000, CRC(23bc72a1) SHA1(083e2955ae2f88d1ad461517b47054d64375b46e) )
	ROM_LOAD16_BYTE( "de1pt1l.8e",  0x40001, 0x20000, CRC(a05ee081) SHA1(1be4c61ad716abb809856e04d4bb450943706a55) )
	ROM_LOAD16_BYTE( "de1pt2u.5n",  0x80000, 0x20000, CRC(10e83d81) SHA1(446fedc3b1e258a39fb9467e5327c9f9a9f1ac3f) )
	ROM_LOAD16_BYTE( "de1pt2l.7n",  0x80001, 0x20000, CRC(3339a976) SHA1(c9eb9c04f7b3f2a85e5ab64ffb2fe4fcfb6c494b) )

	ROM_REGION( 0x100000, REGION_SOUND1, 0 ) /* sound samples */
	ROM_LOAD("de1_voi0.12b",  0x00000, 0x40000, CRC(fc44adbd) SHA1(4268bb1f025e47a94212351d1c1cfd0e5029221f) )
	ROM_LOAD("de1_voi1.12c",  0x40000, 0x40000, CRC(a71dc55a) SHA1(5e746184db9144ab4e3a97b20195b92b0f56c8cc) )
	ROM_LOAD("de1_voi2.12d",  0x80000, 0x40000, CRC(4d32879a) SHA1(eae65f4b98cee9efe4e5dad7298c3717cfb1e6bf) )
	ROM_LOAD("de1_voi3.12e",  0xc0000, 0x40000, CRC(e4832d18) SHA1(0460c79d3942aab89a765b0bd8bbddaf19a6d682) )
ROM_END

ROM_START( starblad )
	ROM_REGION( 0x100000, REGION_CPU1, 0 ) /* Master */
	ROM_LOAD16_BYTE( "st1_mpu.bin",  0x000000, 0x80000, CRC(483a311c) SHA1(dd9416b8d4b0f8b361630e312eac71c113064eae) )
	ROM_LOAD16_BYTE( "st1_mpl.bin",  0x000001, 0x80000, CRC(0a4dd661) SHA1(fc2b71a255a8613693c4d1c79ddd57a6d396165a) )

	ROM_REGION( 0x100000, REGION_CPU2, 0 ) /* Slave */
	ROM_LOAD16_BYTE( "st1_spu.bin",  0x000000, 0x40000, CRC(9f9a55db) SHA1(72bf5d6908cc57cc490fa2292b4993d796b2974d) )
	ROM_LOAD16_BYTE( "st1_spl.bin",  0x000001, 0x40000, CRC(acbe39c7) SHA1(ca48b7ea619b1caaf590eed33001826ce7ef36d8) )

	ROM_REGION( 0x030000, REGION_CPU3, 0 ) /* Sound */
	ROM_LOAD( "st1snd0.bin",		 0x00c000, 0x004000, CRC(c0e934a3) SHA1(678ed6705c6f494d7ecb801a4ef1b123b80979a5) )
	ROM_CONTINUE(					 0x010000, 0x01c000 )
	ROM_RELOAD( 					 0x010000, 0x020000 )

	ROM_REGION( 0x010000, REGION_CPU4, 0 ) /* I/O MCU */
	ROM_LOAD( "sys2mcpu.bin",  0x000000, 0x002000, CRC(a342a97e) SHA1(2c420d34dba21e409bf78ddca710fc7de65a6642) )
	ROM_LOAD( "sys2c65c.bin",  0x008000, 0x008000, CRC(a5b2a4ff) SHA1(068bdfcc71a5e83706e8b23330691973c1c214dc) )

	ROM_REGION( 0x200000, REGION_GFX1, ROMREGION_DISPOSE ) /* sprites */
	ROM_LOAD( "st1obj0.bin",  0x000000, 0x80000, CRC(5d42c71e) SHA1(f1aa2bb31bbbcdcac8e94334b1c78238cac1a0e7) )
	ROM_LOAD( "st1obj1.bin",  0x080000, 0x80000, CRC(c98011ad) SHA1(bc34c21428e0ef5887051c0eb0fdef5397823a82) )
	ROM_LOAD( "st1obj2.bin",  0x100000, 0x80000, CRC(6cf5b608) SHA1(c8537fbe97677c4c8a365b1cf86c4645db7a7d6b) )
	ROM_LOAD( "st1obj3.bin",  0x180000, 0x80000, CRC(cdc195bb) SHA1(91443917a6982c286b6f15381d441d061aefb138) )

	ROM_REGION16_BE( 0x100000, REGION_USER1, 0 )
	ROM_LOAD16_BYTE( "st1_du.bin",  0x000000, 0x20000, CRC(2433e911) SHA1(95f5f00d3bacda4996e055a443311fb9f9a5fe2f) )
	ROM_LOAD16_BYTE( "st1_dl.bin",  0x000001, 0x20000, CRC(4a2cc252) SHA1(d9da9992bac878f8a1f5e84cc3c6d457b4705e8f) )

	ROM_REGION32_BE( 0x400000, REGION_USER2, ROMREGION_ERASE ) /* 24bit signed point data */
	ROM_LOAD32_BYTE( "st1pt0h.bin", 0x000001, 0x80000, CRC(84eb355f) SHA1(89a248b8be2e0afcee29ba4c4c9cca65d5fb246a) )
	ROM_LOAD32_BYTE( "st1pt0u.bin", 0x000002, 0x80000, CRC(1956cd0a) SHA1(7d21b3a59f742694de472c545a1f30c3d92e3390) )
	ROM_LOAD32_BYTE( "st1pt0l.bin", 0x000003, 0x80000, CRC(ff577049) SHA1(1e1595174094e88d5788753d05ce296c1f7eca75) )

	ROM_LOAD32_BYTE( "st1pt1h.bin", 0x200001, 0x80000, CRC(96b1bd7d) SHA1(55da7896dda2aa4c35501a55c8605a065b02aa17) ) // bad?
	ROM_LOAD32_BYTE( "st1pt1u.bin", 0x200002, 0x80000, CRC(ecf21047) SHA1(ddb13f5a2e7d192f0662fa420b49f89e1e991e66) )
	ROM_LOAD32_BYTE( "st1pt1l.bin", 0x200003, 0x80000, CRC(01cb0407) SHA1(4b58860bbc353de8b4b8e83d12b919d9386846e8) )

	ROM_REGION( 0x200000, REGION_SOUND1, 0 ) /* sound samples */
	ROM_LOAD("st1voi0.bin",0x000000,0x80000,CRC(5b3d43a9) SHA1(cdc04f19dc91dca9fa88ba0c2fca72aa195a3694) )
	ROM_LOAD("st1voi1.bin",0x080000,0x80000,CRC(413e6181) SHA1(e827ec11f5755606affd2635718512aeac9354da) )
	ROM_LOAD("st1voi2.bin",0x100000,0x80000,CRC(067d0720) SHA1(a853b2d43027a46c5e707fc677afdaae00f450c7) )
	ROM_LOAD("st1voi3.bin",0x180000,0x80000,CRC(8b5aa45f) SHA1(e1214e639200758ad2045bde0368a2d500c1b84a) )
ROM_END

ROM_START( solvalou )
	ROM_REGION( 0x100000, REGION_CPU1, 0 ) /* Master */
	ROM_LOAD16_BYTE( "sv1mpu.bin",  0x000000, 0x20000, CRC(b6f92762) SHA1(d177328b3da2ab0580e101478142bc8c373d6140) )
	ROM_LOAD16_BYTE( "sv1mpl.bin",  0x000001, 0x20000, CRC(28c54c42) SHA1(32fcca2eb4bb8ba8c2587b03d3cf59f072f7fac5) )

	ROM_REGION( 0x100000, REGION_CPU2, 0 ) /* Slave */
	ROM_LOAD16_BYTE( "sv1spu.bin",  0x000000, 0x20000, CRC(ebd4bf82) SHA1(67946360d680a675abcb3c131bac0502b2455573) )
	ROM_LOAD16_BYTE( "sv1spl.bin",  0x000001, 0x20000, CRC(7acab679) SHA1(764297c9601be99dbbffb75bbc6fe4a40ea38529) )

	ROM_REGION( 0x030000, REGION_CPU3, 0 ) /* Sound */
	ROM_LOAD( "sv1snd0.bin",    0x00c000, 0x004000, CRC(5e007864) SHA1(94da2d51544c6127056beaa251353038646da15f) )
	ROM_CONTINUE(				0x010000, 0x01c000 )
	ROM_RELOAD(					0x010000, 0x020000 )

	ROM_REGION( 0x010000, REGION_CPU4, 0 ) /* I/O MCU */
	ROM_LOAD( "sys2mcpu.bin",  0x000000, 0x002000, CRC(a342a97e) SHA1(2c420d34dba21e409bf78ddca710fc7de65a6642) )
	ROM_LOAD( "sys2c65c.bin",  0x008000, 0x008000, CRC(a5b2a4ff) SHA1(068bdfcc71a5e83706e8b23330691973c1c214dc) )

	ROM_REGION( 0x400000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "sv1obj0.bin",  0x000000, 0x80000, CRC(773798bb) SHA1(51ab76c95030bab834f1a74ae677b2f0afc18c52) )
	ROM_LOAD( "sv1obj4.bin",  0x080000, 0x80000, CRC(33a008a7) SHA1(4959a0ac24ad64f1367e2d8d63d39a0273c60f3e) )
	ROM_LOAD( "sv1obj1.bin",  0x100000, 0x80000, CRC(a36d9e79) SHA1(928d9995e97ee7509e23e6cc64f5e7bfb5c02d42) )
	ROM_LOAD( "sv1obj5.bin",  0x180000, 0x80000, CRC(31551245) SHA1(385452ea4830c466263ad5241313ac850dfef756) )
	ROM_LOAD( "sv1obj2.bin",  0x200000, 0x80000, CRC(c8672b8a) SHA1(8da037b27d2c2b178aab202781f162371458f788) )
	ROM_LOAD( "sv1obj6.bin",  0x280000, 0x80000, CRC(fe319530) SHA1(8f7e46c8f0b86c7515f6d763b795ce07d11c77bc) )
	ROM_LOAD( "sv1obj3.bin",  0x300000, 0x80000, CRC(293ef1c5) SHA1(f677883bfec16bbaeb0a01ac565d0e6cac679174) )
	ROM_LOAD( "sv1obj7.bin",  0x380000, 0x80000, CRC(95ed6dcb) SHA1(931706ce3fea630823ce0c79febec5eec0cc623d) )

	ROM_REGION16_BE( 0x100000, REGION_USER1, 0 )
	ROM_LOAD16_BYTE( "sv1du.bin",  0x000000, 0x80000, CRC(2e561996) SHA1(982158481e5649f21d5c2816fdc80cb725ed1419) )
	ROM_LOAD16_BYTE( "sv1dl.bin",  0x000001, 0x80000, CRC(495fb8dd) SHA1(813d1da4109652008d72b3bdb03032efc5c0c2d5) )

	ROM_REGION32_BE( 0x400000, REGION_USER2, ROMREGION_ERASE )		/* 24bit signed point data */
	ROM_LOAD32_BYTE( "sv1pt0h.bin", 0x000001, 0x80000, CRC(3be21115) SHA1(c9f30353c1216f64199f87cd34e787efd728e739) ) /* most significant */
	ROM_LOAD32_BYTE( "sv1pt0u.bin", 0x000002, 0x80000, CRC(4aacfc42) SHA1(f0e179e057183b41744ca429764f44306f0ce9bf) )
	ROM_LOAD32_BYTE( "sv1pt0l.bin", 0x000003, 0x80000, CRC(6a4dddff) SHA1(9ed182d21d328c6a684ee6658a9dfcf3f3dd8646) ) /* least significant */

	ROM_REGION( 0x200000, REGION_SOUND1, 0 ) /* sound samples */
	ROM_LOAD("sv1voi0.bin",0x000000,0x80000,CRC(7f61bbcf) SHA1(b3b7e66e24d9cb16ebd139237c1e51f5d60c1585) )
	ROM_LOAD("sv1voi1.bin",0x080000,0x80000,CRC(c732e66c) SHA1(14e75dd9bea4055f85eb2bcbf69cf6695a3f7ec4) )
	ROM_LOAD("sv1voi2.bin",0x100000,0x80000,CRC(51076298) SHA1(ec52c9ae3029118f3ea3732948d6de28f5fba561) )
	ROM_LOAD("sv1voi3.bin",0x180000,0x80000,CRC(33085ff3) SHA1(0a30b91618c250a5e7bd896a8ceeb3d16da178a9) )
ROM_END

ROM_START( winrun )
	ROM_REGION( 0x100000, REGION_CPU1, 0 ) /* 68k code */
	ROM_LOAD16_BYTE( "sg1mpub.3k",  0x000000, 0x20000, CRC(7f9b855a) SHA1(6d39a3a9959dbcd0047dbaab0fcd68adc81f5508) )
	ROM_LOAD16_BYTE( "sg1mplb.1k",  0x000001, 0x20000, CRC(a45e8543) SHA1(f9e583a988e4661026ee7873a48d078225778df3) )

	ROM_REGION( 0x40000, REGION_CPU2, 0 ) /* 68k code */
	ROM_LOAD16_BYTE( "sg1spu.6b",  0x000000, 0x20000, CRC(7c9c3a3f) SHA1(cacb45c9111ac66c6e60b7a0cacd8bf47fd00752) )
	ROM_LOAD16_BYTE( "sg1spl.4b",  0x000001, 0x20000, CRC(5068fc5d) SHA1(7f6e80f74985959509d824318a4a7ff2b11953da) )

	ROM_REGION( 0x30000, REGION_CPU3, 0 ) /* Sound */
	ROM_LOAD( "sg1snd0.7c",	0x00c000, 0x004000, CRC(de04b794) SHA1(191f4d79ac2375d7060f3d83ec753185e92f28ea) )
	ROM_CONTINUE(           0x010000, 0x01c000 )
	ROM_RELOAD(             0x010000, 0x020000 )

	ROM_REGION( 0x10000, REGION_CPU4, 0 ) /* I/O MCU */
	ROM_LOAD( "sys2mcpu.bin",  0x000000, 0x002000, CRC(a342a97e) SHA1(2c420d34dba21e409bf78ddca710fc7de65a6642) )
	ROM_LOAD( "sys2c65c.bin",  0x008000, 0x008000, CRC(a5b2a4ff) SHA1(068bdfcc71a5e83706e8b23330691973c1c214dc) )

	ROM_REGION( 0x80000, REGION_CPU5, 0 ) /* 68k code */
	ROM_LOAD16_BYTE( "sg1gp0u.1j",  0x00000, 0x20000, CRC(475da78a) SHA1(6e69bcc6caf2e3cd28fed75796c8992e754f9323) )
	ROM_LOAD16_BYTE( "sg1gp0l.3j",  0x00001, 0x20000, CRC(580479bf) SHA1(ba682190cba0d3cdc49aa4937c898ba7ed2a25f5) )
	ROM_LOAD16_BYTE( "sg1gp1u.1l",  0x40000, 0x20000, CRC(f5f2e927) SHA1(ebf709f16f01f1a634de9121454537cda74e891b) )
	ROM_LOAD16_BYTE( "sg1gp1l.3l",  0x40001, 0x20000, CRC(17ed90a5) SHA1(386bdcb11dcbe400f5be1fe4a7418158b46e50ef) )

	ROM_REGION16_BE( 0x80000, REGION_USER1, 0 )
	ROM_LOAD16_BYTE( "sg1d0u.3a",  0x00001, 0x20000, CRC(1dde2ac2) SHA1(2d20a434561c04e48b52a2137a8c9047e17c1013) )
	ROM_LOAD16_BYTE( "sg1d0l.1a",  0x00000, 0x20000, CRC(2afeb77e) SHA1(ac1552f6e2788158d3477b6a0981d001d6cbdf13) )
	ROM_LOAD16_BYTE( "sg1d1u.3b",  0x40001, 0x20000, CRC(5664b09e) SHA1(10c1c29614eee2cffcfd69085f0450d81ba2e25f) )
	ROM_LOAD16_BYTE( "sg1d1l.1b",  0x40000, 0x20000, CRC(2dbc7de4) SHA1(824304c95942c7296f8e8dcf8ee7e22bf56154b1) )

	ROM_REGION16_BE( 0x80000, REGION_USER2, 0 ) /* 3d objects */
	ROM_LOAD16_BYTE( "sg1pt0u.8j", 0x00001, 0x20000, CRC(160c3634) SHA1(485d20d6cc459f17d77682201dee07bdf76bf343) )
	ROM_LOAD16_BYTE( "sg1pt0l.8d", 0x00000, 0x20000, CRC(b5a665bf) SHA1(5af6ec492f31395c0492e14590b025b120067b8d) )
	ROM_LOAD16_BYTE( "sg1pt1u.8l", 0x40001, 0x20000, CRC(b63d3006) SHA1(78e78619766b0fd91b1e830cfb066495d6773981) )
	ROM_LOAD16_BYTE( "sg1pt1l.8e", 0x40000, 0x20000, CRC(6385e325) SHA1(d50bceb2e9c0d0a38d7b0f918f99c482649e260d) )

	ROM_REGION16_BE( 0x100000, REGION_USER3, 0 ) /* bitmapped graphics */
	ROM_LOAD16_BYTE( "sg1gd0u.1p",  0x00001, 0x40000, CRC(7838fcde) SHA1(45e31269eed1999b73c41c2f5d2c5bfbbdaf23df) )
	ROM_LOAD16_BYTE( "sg1gd0l.3p",  0x00000, 0x40000, CRC(4bd02b9a) SHA1(b2fdfd1c1325864aaad87f5358ab9bbdd79ff6ae) )
	ROM_LOAD16_BYTE( "sg1gd1u.1s",  0x80001, 0x40000, CRC(271db29b) SHA1(8b35fcf273b9aec28d4c606c41c0626dded697e1) )
	ROM_LOAD16_BYTE( "sg1gd1l.3s",  0x80000, 0x40000, CRC(a6c4da96) SHA1(377dbf21a1bede01de16708c96c112abab4417ce) )

	ROM_REGION( 0x200000, REGION_SOUND1, 0 ) /* sound samples */
	ROM_LOAD("sgvoi-1.11c",0x000000,0x80000,CRC(7dcccb31) SHA1(4441b37691434b13eae5dee2d04dc12a56b04d2a) )
	ROM_LOAD("sgboi-3.11e",0x080000,0x80000,CRC(a198141c) SHA1(b4ca352e6aedd9d7a7e5e39e840f1d3a7145900e) )
ROM_END

ROM_START( winrun91 )
	ROM_REGION( 0x100000, REGION_CPU1, 0 ) /* 68k code */
	ROM_LOAD16_BYTE( "mpu.3k",  0x000000, 0x20000, CRC(80a0e5be) SHA1(6613b95e164c2032ea9043e4161130c6b3262492) )
	ROM_LOAD16_BYTE( "mpl.1k",  0x000001, 0x20000, CRC(942172d8) SHA1(21d8dfd2165b5ceb0399fdb53d9d0f51f1255803) )

	ROM_REGION( 0x40000, REGION_CPU2, 0 ) /* 68k code */
	ROM_LOAD16_BYTE( "spu.6b",  0x000000, 0x20000, CRC(0221d4b2) SHA1(65fd38b1cfaa6693d71248561d764a9ea1098c56) )
	ROM_LOAD16_BYTE( "spl.4b",  0x000001, 0x20000, CRC(288799e2) SHA1(2c4bf0cf9c71458fff4dd77e426a76685d9e1bab) )

	ROM_REGION( 0x30000, REGION_CPU3, 0 ) /* Sound */
	ROM_LOAD( "snd0.7c",	0x00c000, 0x004000, CRC(6a321e1e) SHA1(b2e77cac4ed7609593fa5a462c9d78526451e477) )
	ROM_CONTINUE(			0x010000, 0x01c000 )
	ROM_RELOAD(				0x010000, 0x020000 )

	ROM_REGION( 0x10000, REGION_CPU4, 0 ) /* I/O MCU */
	ROM_LOAD( "sys2mcpu.bin",  0x000000, 0x002000, CRC(a342a97e) SHA1(2c420d34dba21e409bf78ddca710fc7de65a6642) )
	ROM_LOAD( "sys2c65c.bin",  0x008000, 0x008000, CRC(a5b2a4ff) SHA1(068bdfcc71a5e83706e8b23330691973c1c214dc) )

	ROM_REGION( 0x80000, REGION_CPU5, 0 ) /* 68k code */
	ROM_LOAD16_BYTE( "gp0u.1j",  0x00000, 0x20000, CRC(f5469a29) SHA1(38b6ea1fbe482b69fbb0e2f44f44a0ca2a49f6bc) )
	ROM_LOAD16_BYTE( "gp0l.3j",  0x00001, 0x20000, CRC(5c18f596) SHA1(215cbda62254e31b4ff6431623384df1639bfdb7) )
	ROM_LOAD16_BYTE( "gp1u.1l",  0x40000, 0x20000, CRC(146ab6b8) SHA1(aefb89585bf311f8d33f18298fea326ef1f19f1e) )
	ROM_LOAD16_BYTE( "gp1l.3l",  0x40001, 0x20000, CRC(96c2463c) SHA1(e43db580e7b454af04c22e894108fbb56da0eeb5) )

	ROM_REGION16_BE( 0x80000, REGION_USER1, 0 )
	ROM_LOAD16_BYTE( "d0u.3a",  0x00001, 0x20000, CRC(dcb27da5) SHA1(ecd72397d10313fe8dcb8589bdc5d88d4298b26c) )
	ROM_LOAD16_BYTE( "d0l.1a",  0x00000, 0x20000, CRC(f692a8f3) SHA1(4c29f60400b18d9ef0425de149618da6cf762ca4) )
	ROM_LOAD16_BYTE( "d1u.3b",  0x40001, 0x20000, CRC(ac2afd1b) SHA1(510eb41931164b086c85ba0a86d6f10b88f5e534) )
	ROM_LOAD16_BYTE( "d1l.1b",  0x40000, 0x20000, CRC(ebb51af1) SHA1(87b7b64ee662bf652add1e1199e42391d0e2f7e8) )

	ROM_REGION16_BE( 0x80000, REGION_USER2, 0 ) /* 3d objects */
	ROM_LOAD16_BYTE( "pt0u.8j", 0x00001, 0x20000, CRC(abf512a6) SHA1(e86288039d6c4dedfa95b11cb7e4b87637f90c09) )
	ROM_LOAD16_BYTE( "pt0l.8d", 0x00000, 0x20000, CRC(ac8d468c) SHA1(d1b457a19a5d3259d0caf933f42b3a02b485867b) )
	ROM_LOAD16_BYTE( "pt1u.8l", 0x40001, 0x20000, CRC(7e5dab74) SHA1(5bde219d5b4305d38d17b494b2e759f05d05329f) )
	ROM_LOAD16_BYTE( "pt1l.8e", 0x40000, 0x20000, CRC(38a54ec5) SHA1(5c6017c98cae674868153ff2d64532027cf0ab83) )

	ROM_REGION16_BE( 0x100000, REGION_USER3, 0 ) /* bitmapped graphics */
	ROM_LOAD16_BYTE( "gd0u.1p",  0x00001, 0x40000, CRC(33f5a19b) SHA1(b1dbd242168007f80e13e11c78b34abc1668883e) )
	ROM_LOAD16_BYTE( "gd0l.3p",  0x00000, 0x40000, CRC(9a29500e) SHA1(c605f86b138e0a4c3163ffd967482e298a15fbe7) )
	ROM_LOAD16_BYTE( "gd1u.1s",  0x80001, 0x40000, CRC(17e5a61c) SHA1(272ebd7daa56847f1887809535362331b5465dec) )
	ROM_LOAD16_BYTE( "gd1l.3s",  0x80000, 0x40000, CRC(64df59a2) SHA1(1e9d0945b94780bb0be16803e767466d2cda07e8) )

	ROM_REGION( 0x200000, REGION_SOUND1, 0 ) /* sound samples */
	ROM_LOAD("avo1.11c",0x000000,0x80000,CRC(9fb33af3) SHA1(666630a8e5766ca4c3275961963c3e713dfdda2d) )
	ROM_LOAD("avo3.11e",0x080000,0x80000,CRC(76e22f92) SHA1(0e1b8d35a5b9c20cc3192d935f0c9da1e69679d2) )
ROM_END

static void namcos21_init( int game_type )
{
	namcos2_gametype = game_type;
	ptram = auto_malloc(PTRAM_SIZE);
	mpDataROM = (UINT16 *)memory_region( REGION_USER1 );
	InitDSP();
} /* namcos21_init */

static DRIVER_INIT( winrun )
{
	/* don't use the generic namcos21_init; hardware is different */
	namcos2_gametype = NAMCOS21_WINRUN91;
	mpDataROM = (UINT16 *)memory_region( REGION_USER1 );
}

static DRIVER_INIT( aircombt )
{
#if 0
	/* replace first four tests of aircombj to reveal special "hidden" tests */
	UINT16 *pMem = (UINT16 *)memory_region( REGION_CPU1 );
	pMem[0x2a32/2] = 0x90;
	pMem[0x2a34/2] = 0x94;
	pMem[0x2a36/2] = 0x88;
	pMem[0x2a38/2] = 0x8c;
#endif

	namcos21_init( NAMCOS21_AIRCOMBAT );
}

DRIVER_INIT( starblad )
{
	namcos21_init( NAMCOS21_STARBLADE );
}


DRIVER_INIT( cybsled )
{
	namcos21_init( NAMCOS21_CYBERSLED );
}

DRIVER_INIT( solvalou )
{
	namcos21_init( NAMCOS21_SOLVALOU );
}

DRIVER_INIT( driveyes )
{
	namcos2_gametype = NAMCOS21_DRIVERS_EYES;
}

/*************************************************************/
/*                                                           */
/*  NAMCO SYSTEM 21 INPUT PORTS                              */
/*                                                           */
/*************************************************************/

static INPUT_PORTS_START( s21default )
	PORT_START		/* 63B05Z0 - PORT B */
	PORT_BIT( 0x3f, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START1 )

	PORT_START		/* 63B05Z0 - PORT C & SCI */
	PORT_BIT( 0x0f, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_DIPNAME( 0x40, 0x40, "Test Switch")
	PORT_DIPSETTING(	0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN3 )

	PORT_START		/* 63B05Z0 - 8 CHANNEL ANALOG - CHANNEL 0 */
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_START      /* 63B05Z0 - 8 CHANNEL ANALOG - CHANNEL 1 */
	PORT_BIT( 0xff, 0x80, IPT_AD_STICK_X ) PORT_MINMAX(0x60,0x9f) PORT_SENSITIVITY(15) PORT_KEYDELTA(10)
	PORT_START      /* 63B05Z0 - 8 CHANNEL ANALOG - CHANNEL 2 */
	PORT_BIT( 0xff, 0x80, IPT_AD_STICK_Y ) PORT_MINMAX(0x60,0x9f) PORT_SENSITIVITY(20) PORT_KEYDELTA(10)
	PORT_START		/* 63B05Z0 - 8 CHANNEL ANALOG - CHANNEL 3 */
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_START		/* 63B05Z0 - 8 CHANNEL ANALOG - CHANNEL 4 */
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_START		/* 63B05Z0 - 8 CHANNEL ANALOG - CHANNEL 5 */
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_START		/* 63B05Z0 - 8 CHANNEL ANALOG - CHANNEL 6 */
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_START		/* 63B05Z0 - 8 CHANNEL ANALOG - CHANNEL 7 */
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START		/* 63B05Z0 - PORT H */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON4 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON3 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0xc0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START		/* 63B05Z0 - $2000 DIP SW */
	PORT_DIPNAME( 0x01, 0x01, "DSW1 (Test Mode)")
	PORT_DIPSETTING(	0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, "DSW2")
	PORT_DIPSETTING(	0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, "DSW3")
	PORT_DIPSETTING(	0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, "DSW4")
	PORT_DIPSETTING(	0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, "DSW5")
	PORT_DIPSETTING(	0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, "DSW6")
	PORT_DIPSETTING(	0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, "DSW7")
	PORT_DIPSETTING(	0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, "DSW8")
	PORT_DIPSETTING(	0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )

	PORT_START		/* 63B05Z0 - $3000 */
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_START		/* 63B05Z0 - $3001 */
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_START		/* 63B05Z0 - $3002 */
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_START		/* 63B05Z0 - $3003 */
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )
INPUT_PORTS_END

static INPUT_PORTS_START( cybsled )
	PORT_START		/* 63B05Z0 - PORT B */
	PORT_BIT( 0x3f, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START1 )

	PORT_START		/* 63B05Z0 - PORT C & SCI */
	PORT_BIT( 0x0f, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_DIPNAME( 0x40, 0x40, "Test Switch")
	PORT_DIPSETTING(	0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN3 )

	PORT_START		/* 63B05Z0 - 8 CHANNEL ANALOG - CHANNEL 0 */
	PORT_BIT( 0xff, 0x7f, IPT_AD_STICK_Y ) PORT_MINMAX(0x00,0xff) PORT_SENSITIVITY(100) PORT_KEYDELTA(10) PORT_PLAYER(2) /* right joystick: vertical */
	PORT_START      /* 63B05Z0 - 8 CHANNEL ANALOG - CHANNEL 1 */
	PORT_BIT( 0xff, 0x7f, IPT_AD_STICK_Y ) PORT_MINMAX(0x00,0xff) PORT_SENSITIVITY(100) PORT_KEYDELTA(10) PORT_PLAYER(1) /* left joystick: vertical */
	PORT_START      /* 63B05Z0 - 8 CHANNEL ANALOG - CHANNEL 2 */
	PORT_BIT( 0xff, 0x7f, IPT_AD_STICK_X ) PORT_MINMAX(0x00,0xff) PORT_SENSITIVITY(100) PORT_KEYDELTA(10) PORT_PLAYER(2) /* right joystick: horizontal */
	PORT_START		/* 63B05Z0 - 8 CHANNEL ANALOG - CHANNEL 3 */
	PORT_BIT( 0xff, 0x7f, IPT_AD_STICK_X ) PORT_MINMAX(0x00,0xff) PORT_SENSITIVITY(100) PORT_KEYDELTA(10) PORT_PLAYER(1) /* left joystick: horizontal */
	PORT_START		/* 63B05Z0 - 8 CHANNEL ANALOG - CHANNEL 4 */
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_START		/* 63B05Z0 - 8 CHANNEL ANALOG - CHANNEL 5 */
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_START		/* 63B05Z0 - 8 CHANNEL ANALOG - CHANNEL 6 */
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_START		/* 63B05Z0 - 8 CHANNEL ANALOG - CHANNEL 7 */
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START		/* 63B05Z0 - PORT H */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON4 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON3 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0xc0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START		/* 63B05Z0 - $2000 DIP SW */
	PORT_DIPNAME( 0x01, 0x01, "DSW1 (Test Mode)")
	PORT_DIPSETTING(	0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, "DSW2")
	PORT_DIPSETTING(	0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, "DSW3")
	PORT_DIPSETTING(	0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, "DSW4")
	PORT_DIPSETTING(	0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, "DSW5")
	PORT_DIPSETTING(	0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, "DSW6")
	PORT_DIPSETTING(	0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, "DSW7")
	PORT_DIPSETTING(	0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, "DSW8")
	PORT_DIPSETTING(	0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )

	PORT_START		/* 63B05Z0 - $3000 */
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_START		/* 63B05Z0 - $3001 */
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_START		/* 63B05Z0 - $3002 */
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_START		/* 63B05Z0 - $3003 */
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )
INPUT_PORTS_END

static INPUT_PORTS_START( aircombt )
	PORT_START		/* IN#0: 63B05Z0 - PORT B */
	PORT_BIT( 0x3f, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START1 )

	PORT_START		/* IN#1: 63B05Z0 - PORT C & SCI */
	PORT_BIT( 0x0f, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_DIPNAME( 0x40, 0x40, "Test Switch")
	PORT_DIPSETTING(	0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN3 )

	PORT_START		/* IN#2: 63B05Z0 - 8 CHANNEL ANALOG - CHANNEL 0 */
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START		/* IN#3: 63B05Z0 - 8 CHANNEL ANALOG - CHANNEL 1 */
	PORT_BIT( 0xff, 0x80, IPT_AD_STICK_X ) PORT_MINMAX(0x00,0xff) PORT_SENSITIVITY(100) PORT_KEYDELTA(4)

	PORT_START		/* IN#4: 63B05Z0 - 8 CHANNEL ANALOG - CHANNEL 2 */
	PORT_BIT( 0xff, 0x80, IPT_AD_STICK_Y ) PORT_MINMAX(0x00,0xff) PORT_SENSITIVITY(100) PORT_KEYDELTA(4)

	PORT_START		/* IN#5: 63B05Z0 - 8 CHANNEL ANALOG - CHANNEL 3 */
	PORT_BIT( 0xff, 0x80, IPT_AD_STICK_X ) PORT_MINMAX(0x00,0xff) PORT_SENSITIVITY(100) PORT_KEYDELTA(4) PORT_PLAYER(2)

	PORT_START		/* IN#6: 63B05Z0 - 8 CHANNEL ANALOG - CHANNEL 4 */
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_START		/* IN#7: 63B05Z0 - 8 CHANNEL ANALOG - CHANNEL 5 */
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_START		/* IN#8: 63B05Z0 - 8 CHANNEL ANALOG - CHANNEL 6 */
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_START		/* IN#9: 63B05Z0 - 8 CHANNEL ANALOG - CHANNEL 7 */
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START		/* IN#10: 63B05Z0 - PORT H */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON4 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON3 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON6 ) ///???
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON2 ) // prev color
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON5 ) // ???next color
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0xc0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START		/* IN#11: 63B05Z0 - $2000 DIP SW */
	PORT_DIPNAME( 0x01, 0x01, "DSW1 (Test Mode)")
	PORT_DIPSETTING(	0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, "DSW2")
	PORT_DIPSETTING(	0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, "DSW3")
	PORT_DIPSETTING(	0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, "DSW4")
	PORT_DIPSETTING(	0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, "DSW5")
	PORT_DIPSETTING(	0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, "DSW6")
	PORT_DIPSETTING(	0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, "DSW7")
	PORT_DIPSETTING(	0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, "DSW8")
	PORT_DIPSETTING(	0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )

	PORT_START		/* IN#12: 63B05Z0 - $3000 */
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_START		/* IN#13: 63B05Z0 - $3001 */
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_START		/* IN#14: 63B05Z0 - $3002 */
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_START		/* IN#15: 63B05Z0 - $3003 */
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )
INPUT_PORTS_END

/*    YEAR, NAME,     PARENT,   MACHINE,           INPUT,        INIT,     MONITOR, COMPANY, FULLNAME,             FLAGS */
GAME( 1992, aircombj, 0,       poly_c140_typeB,   aircombt,     aircombt, ROT0,    "Namco", "Air Combat (Japan)", GAME_NOT_WORKING ) /* mostly working */
GAME( 1992, aircombu, aircombj,poly_c140_typeB,   aircombt,     aircombt, ROT0,    "Namco", "Air Combat (US)",	  GAME_NOT_WORKING ) /* mostly working */
GAME( 1993, cybsled,  0,       poly_c140_typeA,   cybsled,      cybsled,  ROT0,    "Namco", "Cyber Sled",         GAME_NOT_WORKING ) /* mostly working */
GAME( 1999, driveyes, 0,       poly_c140_typeA,   s21default,   driveyes, ROT0,    "Namco", "Driver's Eyes",      GAME_NOT_WORKING )
/* 1992, ShimDrive */
GAME( 1991, solvalou, 0,       poly_c140_typeA,   s21default,   solvalou, ROT0,    "Namco", "Solvalou (Japan)",   GAME_NOT_WORKING|GAME_IMPERFECT_GRAPHICS ) /* working and playable */
GAME( 1991, starblad, 0,       poly_c140_typeA,   s21default,   starblad, ROT0,    "Namco", "Starblade",		  GAME_IMPERFECT_GRAPHICS ) /* working and playable */
GAME( 1988, winrun,   0,       winrun_c140_typeB, s21default,   winrun,   ROT0,    "Namco", "Winning Run",        GAME_NOT_WORKING )
/* 1989, Winning Run Suzuka Grand Prix */
GAME( 1991, winrun91, 0,       winrun_c140_typeB, s21default,   winrun,   ROT0,    "Namco", "Winning Run 91", 	  GAME_NOT_WORKING ) /* not working */
