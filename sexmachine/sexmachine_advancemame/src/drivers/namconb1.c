/*
Namco System NB-1

Notes:
- tilemap system is identical to Namco System2

ToDo:
- gunbulet force feedback
- MCU simulation (coin/inputs) is incomplete; it doesn't handle coinage
- sound support

Main CPU : Motorola 68020 32-bit processor @ 25MHz
Secondary CPUs : C329 + 137 (both custom)
Custom Graphics Chips : GFX:123,145,156,C116 - Motion Objects:C355,187,C347
Sound CPU : C351 (custom)
PCM Sound chip : C352 (custom)
I/O Chip : 160 (custom)
Board composition : Single board

Known games using this hardware:
- Great Sluggers '93
- Great Sluggers '94
- Gun Bullet / Point Blank
- Nebulas Ray
- Super World Stadium '95
- Super World Stadium '96
- Super World Stadium '97
- OutFoxies
- Mach Breakers

*****************************************************

Gun Bullet (JPN Ver.)
(c)1994 Namco
KeyCus.:C386

Note: CPU - Main PCB  (NB-1)
      MEM - MEMEXT OBJ8 PCB at J103 on the main PCB
      SPR - MEMEXT SPR PCB at location 5B on the main PCB

          - Namco main PCB:  NB-1  8634961101, (8634963101)
          - MEMEXT OBJ8 PCB:       8635902201, (8635902301)

        * - Surface mounted ROMs
        # - 32 pin DIP Custom IC (16bytes x 16-bit)

Brief hardware overview
-----------------------

Main processor    - MC68EC020FG25 25MHz   (100 pin PQFP)
                  - C329 custom           (100 pin PQFP)
                  - 137  custom PLD       (28 pin NDIP)
                  - C366 Key Custom

Sound processor   - C351 custom           (160 pin PQFP)
 (PCM)            - C352 custom           (100 pin PQFP)
 (control inputs) - 160  custom           (80 pin PQFP)

GFX               - 123  custom           (80 pin PQFP)
                  - 145  custom           (80 pin PQFP)
                  - 156  custom           (64 pin PQFP)
                  - C116 custom           (80 pin PQFP)

Motion Objects    - C355 custom           (160 pin PQFP)
                  - 187  custom           (160 pin PQFP)
                  - C347 custom           (80 pin PQFP)

PCB Jumper Settings
-------------------

Location      Setting       Alt. Setting
----------------------------------------
  JP1           1M              4M
  JP2           4M              1M
  JP5          <1M              1M
  JP6           8M             >8M
  JP7           4M              1M
  JP8           4M              1M
  JP9           cON             cOFF
  JP10          24M (MHz)       12M (MHz)
  JP11          24M (MHz)       28M (MHz)
  JP12          355             F32

*****************************************************

Super World Stadium '95 (JPN Ver.)
(c)1986-1993 1995 Namco

Namco NB-1 system

KeyCus.:C393

*****************************************************

Super World Stadium '96 (JPN Ver.)
(c)1986-1993 1995 1996 Namco

Namco NB-1 system

KeyCus.:C426

*****************************************************

Super World Stadium '97 (JPN Ver.)
(c)1986-1993 1995 1996 1997 Namco

Namco NB-1 system

KeyCus.:C434

*****************************************************

Great Sluggers Featuring 1994 Team Rosters
(c)1993 1994 Namco / 1994 MLBPA

Namco NB-1 system

KeyCus.:C359

*****************************************************

--------------------------
Nebulasray by NAMCO (1994)
--------------------------
Location      Device        File ID      Checksum
-------------------------------------------------
CPU 13B         27C402      NR1-MPRU       B0ED      [  MAIN PROG  ]
CPU 15B         27C240      NR1-MPRL       90C4      [  MAIN PROG  ]
SPR            27C1024      NR1-SPR0       99A6      [  SOUND PRG  ]
CPU 5M       MB834000B      NR1-SHA0       DD59      [    SHAPE    ]
CPU 8J       MB838000B      NR1-CHR0       22A4      [  CHARACTER  ]
CPU 9J       MB838000B      NR1-CHR1       19D0      [  CHARACTER  ]
CPU 10J      MB838000B      NR1-CHR2       B524      [  CHARACTER  ]
CPU 11J      MB838000B      NR1-CHR3       0AF4      [  CHARACTER  ]
CPU 5J       KM2316000      NR1-VOI0       8C41      [    VOICE    ]
MEM IC1     MT26C1600 *     NR1-OBJ0L      FD7C      [ MOTION OBJL ]
MEM IC3     MT26C1600 *     NR1-OBJ1L      7069      [ MOTION OBJL ]
MEM IC5     MT26C1600 *     NR1-OBJ2L      07DC      [ MOTION OBJL ]
MEM IC7     MT26C1600 *     NR1-OBJ3L      A61E      [ MOTION OBJL ]
MEM IC2     MT26C1600 *     NR1-OBJ0U      44D3      [ MOTION OBJU ]
MEM IC4     MT26C1600 *     NR1-OBJ1U      F822      [ MOTION OBJU ]
MEM IC6     MT26C1600 *     NR1-OBJ2U      DD24      [ MOTION OBJU ]
MEM IC8     MT26C1600 *     NR1-OBJ3U      F750      [ MOTION OBJU ]
CPU 11D        Custom #      C366.BIN      1C93      [  KEYCUSTUM  ]

Note: CPU - Main PCB  (NB-1)
      MEM - MEMEXT OBJ8 PCB at J103 on the main PCB
      SPR - MEMEXT SPR PCB at location 5B on the main PCB

          - Namco main PCB:  NB-1  8634961101, (8634963101)
          - MEMEXT OBJ8 PCB:       8635902201, (8635902301)

        * - Surface mounted ROMs
        # - 32 pin DIP Custom IC (16bytes x 16-bit)

Brief hardware overview
-----------------------

Main processor    - MC68EC020FG25 25MHz   (100 pin PQFP)
                  - C329 custom           (100 pin PQFP)
                  - 137  custom PLD       (28 pin NDIP)
                  - C366 Key Custom

Sound processor   - C351 custom           (160 pin PQFP)
 (PCM)            - C352 custom           (100 pin PQFP)
 (control inputs) - 160  custom           (80 pin PQFP)

GFX               - 123  custom           (80 pin PQFP)
                  - 145  custom           (80 pin PQFP)
                  - 156  custom           (64 pin PQFP)
                  - C116 custom           (80 pin PQFP)

Motion Objects    - C355 custom           (160 pin PQFP)
                  - 187  custom           (160 pin PQFP)
                  - C347 custom           (80 pin PQFP)

PCB Jumper Settings
-------------------

Location      Setting       Alt. Setting
----------------------------------------
  JP1           1M              4M
  JP2           4M              1M
  JP5          <1M              1M
  JP6           8M             >8M
  JP7           4M              1M
  JP8           4M              1M
  JP9           cON             cOFF
  JP10          24M (MHz)       12M (MHz)
  JP11          24M (MHz)       28M (MHz)
  JP12          355             F32


Namco System NB2

Games running on this hardware:
- Outfoxies
- Mach Breakers

Changes from Namcon System NB1 include:
- different memory map
- more complex sprite and tile banking
- 2 additional ROZ layers

-----------------------------
The Outfoxies by NAMCO (1994)
-----------------------------
Location            Device     File ID     Checksum
----------------------------------------------------
CPU 11C PRGL       27C4002     OU2-MPRL      166F
CPU 11D PRGU       27C4002     OU2-MPRU      F4C1
CPU 5B  SPR0        27C240     OU1-SPR0      7361
CPU 20A DAT0       27C4002     OU1-DAT0      FCD1
CPU 20B DAT1       27C4002     OU1-DAT1      0973
CPU 18S SHAPE-R    MB83800     OU1-SHAR      C922
CPU 12S SHAPE-S    MB83400     OU1-SHAS      2820
CPU 6N  VOICE0    MB831600     OU1-VOI0      4132
ROM 4C  OBJ0L    16Meg SMD     OU1-OBJ0L     171B
ROM 8C  OBJ0U    16Meg SMD     OU1-OBJ0U     F961
ROM 4B  OBJ1L    16Meg SMD     OU1-OBJ1L     1579
ROM 8B  OBJ1U    16Meg SMD     OU1-OBJ1U     E8DF
ROM 4A  OBJ2L    16Meg SMD     OU1-OBJ2L     AE7B
ROM 8A  OBJ2U    16Meg SMD     OU1-OBJ2U     6588
ROM 6C  OBJ3L    16Meg SMD     OU1-OBJ3L     9ED3
ROM 9C  OBJ3U    16Meg SMD     OU1-OBJ3U     ED3B
ROM 6B  OBJ4L    16Meg SMD     OU1-OBJ4L     59D4
ROM 9B  OBJ4U    16Meg SMD     OU1-OBJ4U     56CA
ROM 3D  ROT0     16Meg SMD     OU1-ROT0      A615
ROM 3C  ROT1     16Meg SMD     OU1-ROT1      6C0A
ROM 3B  ROT2     16Meg SMD     OU1-ROT2      313E
ROM 1D  SCR0     16Meg SMD     OU1-SCR0      751A

CPU 8B  DEC75     PAL16L8A        NB1-2
CPU 16N MIXER     PAL16V8H        NB2-1
CPU 11E SIZE      PAL16L8A        NB2-2
CPU 22C KEYCUS   KeyCustom         C390

CPU  -  Namco NB-2 Main PCB        8639960102 (8639970102)
ROM  -  Namco NB-2 Mask ROM PCB    8639969800 (8639979800)

     -  Audio out is Stereo

Jumper Settings:

     Setting     Alternate
JP1    4M           1M
JP2    GND          A20
JP3    GND          A20
JP6    4M           1M
JP8    GND          A20
JP9    CON          COFF
JP10   GND          A20

Hardware info:

Main CPU:           MC68EC020FG25
                    Custom C383    (100 pin PQFP)
                    Custom C385    (144 pin PQFP)

Slave CPU:         ?Custom C382    (160 pin PQFP)
                    Custom 160     ( 80 pin PQFP)
                    Custom C352    (100 pin PQFP)

GFX:                Custom 145     ( 80 pin PQFP)
                    Custom 156     ( 64 pin PQFP)
                    Custom 123     ( 64 pin PQFP)
                 3x Custom 384     ( 48 pin PQFP)
                    Custom C355    (160 pin PQFP)
                    Custom 187     (120 pin PQFP)
                    Custom 169     (120 pin PQFP)
*/
#include "driver.h"
#include "namconb1.h"
#include "namcos2.h"
#include "namcoic.h"
#include "sndhrdw/namcoc7x.h"

#define NB1_NVMEM_SIZE (0x800)
static UINT32 *nvmem32;

UINT32 *namconb1_workram32;
UINT32 *namconb1_spritebank32;
UINT32 *namconb1_tilebank32;

/****************************************************************************/

static UINT32 *namconb_cpureg32;

static int
GetCPURegister(int which)
{
	return (namconb_cpureg32[which/4]<<((which&3)*8))>>24;
}

static void namconb1_TriggerPOSIRQ( int scanline )
{
	int irqlevel = GetCPURegister(0x04)>>4;
	force_partial_update(scanline);
	cpunum_set_input_line(0, irqlevel, PULSE_LINE);
}

static void namconb2_TriggerPOSIRQ( int scanline )
{
	int irqlevel = GetCPURegister(0x02);
	force_partial_update(scanline);
	cpunum_set_input_line(0, irqlevel, PULSE_LINE);
} /* namconb2_TriggerPOSIRQ */

static INTERRUPT_GEN( namconb2_interrupt )
{
	/**
     * f00000 0x01 // VBLANK irq level
     * f00001 0x00
     * f00002 0x05 // POSIRQ level
     * f00003 0x00
     *
     * f00004 VBLANK ack
     * f00005
     * f00006 POSIRQ ack
     * f00007
     *
     * f00008
     *
     * f00009 0x62
     * f0000a 0x0f
     * f0000b 0x41
     * f0000c 0x70
     * f0000d 0x70
     * f0000e 0x23
     * f0000f 0x50
     * f00010 0x00
     * f00011 0x64
     * f00012 0x18
     * f00013 0xe7
     * f00014 (watchdog)
     * f00016 0x00
     * f0001e 0x00
     * f0001f 0x01
     */
	int scanline = (paletteram32[0x1808/4]&0xffff)-32;
	int irqlevel = GetCPURegister(0x00);
	cpunum_set_input_line( 0, irqlevel, HOLD_LINE);

	if( scanline<0 )
	{
		scanline = 0;
	}
	if( scanline < NAMCONB1_ROWS*8 )
	{
		timer_set( cpu_getscanlinetime(scanline), scanline, namconb2_TriggerPOSIRQ );
	}
} /* namconb2_interrupt */
static INTERRUPT_GEN( namconb1_interrupt )
{
	/**
     * 400000 0x00
     * 400001 0x00
     * 400002 0x00
     * 400003 0x00
     * 400004 0x35 // irq levels
     * 400005 0x00
     * 400006 0x00
     * 400007 0x00
     * 400008 0x00
     * 400009 0x00 VBLANK ack
     * 40000a 0x00
     * 40000b 0x03
     * 40000c 0x07
     * 40000d 0x01
     * 40000e 0x10
     * 40000f 0x03
     * 400010 0x00
     * 400011 0x07
     * 400012 0x10
     * 400013 0x10
     * 400014 0x00
     * 400015 0x01
     * 400016 (watchdog)
     * 400017 0x00
     * 400018 0x01
     * 400019 0x00
     * 40001a 0x00
     * 40001b 0x00
     * 40001c 0x00
     * 40001d 0x00
     * 40001e 0x00
     * 40001f 0x00
     */
	int scanline = (paletteram32[0x1808/4]&0xffff)-32;
	int irqlevel = GetCPURegister(0x04)&0xf;
	cpunum_set_input_line( 0, irqlevel, HOLD_LINE);
	if( scanline<0 )
	{
		scanline = 0;
	}
	if( scanline < NAMCONB1_ROWS*8 )
	{
		timer_set( cpu_getscanlinetime(scanline), scanline, namconb1_TriggerPOSIRQ );
	}
} /* namconb1_interrupt */

static WRITE32_HANDLER( namconb_cpureg_w )
{
	COMBINE_DATA( &namconb_cpureg32[offset] );
} /* namconb_cpureg_w */

/****************************************************************************/

static NVRAM_HANDLER( namconb1 )
{
	int i;
	UINT8 data[4];
	if( read_or_write )
	{
		for( i=0; i<NB1_NVMEM_SIZE/4; i++ )
		{
			UINT32 dword = nvmem32[i];
			data[0] = dword>>24;
			data[1] = (dword&0x00ff0000)>>16;
			data[2] = (dword&0x0000ff00)>>8;
			data[3] = dword&0xff;
			mame_fwrite( file, data, 4 );
		}
	}
	else
	{
		if (file)
		{
			for( i=0; i<NB1_NVMEM_SIZE/4; i++ )
			{
				mame_fread( file, data, 4 );
				nvmem32[i] = (data[0]<<24)|(data[1]<<16)|(data[2]<<8)|data[3];
			}
		}
		else
		{
			memset( nvmem32, 0x00, NB1_NVMEM_SIZE );
			if( namcos2_gametype == NAMCONB1_GUNBULET )
			{
				nvmem32[0] = 0x0f260f26; /* default gun calibration */
			}
		}
	}
} /* namconb1 */

static DRIVER_INIT( nebulray )
{
	UINT8 *pMem = (UINT8 *)memory_region(NAMCONB1_TILEMASKREGION);
	size_t numBytes = (0xfe7-0xe6f)*8;
	memset( &pMem[0xe6f*8], 0, numBytes );

	namcos2_gametype = NAMCONB1_NEBULRAY;

	namcoc7x_on_driver_init();
} /* nebulray */

static DRIVER_INIT( gslgr94u )
{
	namcos2_gametype = NAMCONB1_GSLGR94U;
	namcoc7x_on_driver_init();
} /* gslgr94u */

static DRIVER_INIT( sws95 )
{
	namcos2_gametype = NAMCONB1_SWS95;
	namcoc7x_on_driver_init();
} /* sws95 */

static DRIVER_INIT( sws96 )
{
	namcos2_gametype = NAMCONB1_SWS96;
	namcoc7x_on_driver_init();
} /* sws96 */

static DRIVER_INIT( sws97 )
{
	namcos2_gametype = NAMCONB1_SWS97;
	namcoc7x_on_driver_init();
} /* sws97 */

static DRIVER_INIT( gunbulet )
{
	namcos2_gametype = NAMCONB1_GUNBULET;
	namcoc7x_on_driver_init();
} /* gunbulet */

static DRIVER_INIT( vshoot )
{
	namcos2_gametype = NAMCONB1_VSHOOT;
	namcoc7x_on_driver_init();
} /* vshoot */

static void
ShuffleDataROMs( void )
{
	size_t len = memory_region_length(REGION_USER1)/4;
	UINT8 *pMem8 = (UINT8 *)memory_region( REGION_USER1 );
	UINT32 *pMem32 = (UINT32 *)pMem8;
	int i;

	for( i=0; i<len; i++ )
	{
		pMem32[i] = (pMem8[0]<<16)|(pMem8[1]<<24)|(pMem8[2]<<0)|(pMem8[3]<<8);
		pMem8+=4;
	}
	memory_set_bankptr( 1, pMem32 );
}

static DRIVER_INIT( machbrkr )
{
	namcos2_gametype = NAMCONB2_MACH_BREAKERS;
	ShuffleDataROMs();
	namcoc7x_on_driver_init();
}

static DRIVER_INIT( outfxies )
{
	namcos2_gametype = NAMCONB2_OUTFOXIES;
	ShuffleDataROMs();
	namcoc7x_on_driver_init();
}

static READ32_HANDLER( custom_key_r )
{
	static UINT16 count;
	UINT16 old_count = count;

	do
	{ /* pick a random number, but don't pick the same twice in a row */
		count = mame_rand();
	} while( count==old_count );

	switch( namcos2_gametype )
	{
	case NAMCONB1_GUNBULET:
		return 0; /* no protection */

	case NAMCONB1_SWS95:
		switch( offset )
		{
		case 0: return 0x0189;
		case 1: return  count<<16;
		}
		break;

	case NAMCONB1_SWS96:
		switch( offset )
		{
		case 0: return 0x01aa<<16;
		case 4: return count<<16;
		}
		break;

	case NAMCONB1_SWS97:
		switch( offset )
		{
		case 2: return 0x1b2<<16;
		case 5: return count<<16;
		}
		break;

	case NAMCONB1_GSLGR94U:
		switch( offset )
		{
		case 0: return 0x0167;
		case 1: return count<<16;
		}
		break;

	case NAMCONB1_NEBULRAY:
		switch( offset )
		{
		case 1: return 0x016e;
		case 3: return count;
		}
		break;

	case NAMCONB1_VSHOOT:
		switch( offset )
		{
		case 2: return count<<16;
		case 3: return 0x0170<<16;
		}
		break;

	case NAMCONB2_OUTFOXIES:
		switch( offset )
		{
		case 0: return 0x0186;
		case 1: return count<<16;
		}
		break;

	case NAMCONB2_MACH_BREAKERS:
		break; /* no protection? */
	}

	logerror( "custom_key_r(%d); pc=%08x\n", offset, activecpu_get_pc() );
	return 0;
} /* custom_key_r */

/***************************************************************/

static const gfx_layout obj_layout =
{
	16,16,
	RGN_FRAC(1,1),
	8, /* bits per pixel */
	{
		/* plane offsets */
		0,1,2,3,4,5,6,7,
	},
	{
		0*16+8,1*16+8,0*16,1*16,
		2*16+8,3*16+8,2*16,3*16,
		4*16+8,5*16+8,4*16,5*16,
		6*16+8,7*16+8,6*16,7*16
	},
	{
		0x0*128,0x1*128,0x2*128,0x3*128,0x4*128,0x5*128,0x6*128,0x7*128,
		0x8*128,0x9*128,0xa*128,0xb*128,0xc*128,0xd*128,0xe*128,0xf*128
	},
	16*128
}; /* obj_layout */

static const gfx_layout tile_layout =
{
	8,8,
	RGN_FRAC(1,1),
	8,
	{ 0,1,2,3,4,5,6,7 },
	{ 0*8,1*8,2*8,3*8,4*8,5*8,6*8,7*8 },
	{ 0*64,1*64,2*64,3*64,4*64,5*64,6*64,7*64 },
	8*64
}; /* tile_layout */

static const gfx_layout roz_layout =
{
	16,16,
	RGN_FRAC(1,1),
	8,
	{ 0,1,2,3,4,5,6,7 },
	{ 0*8,1*8,2*8,3*8,4*8,5*8,6*8,7*8,8*8,9*8,10*8,11*8,12*8,13*8,14*8,15*8 },
	{
		0*128,1*128,2*128,3*128,4*128,5*128,6*128,7*128,8*128,9*128,10*128,11*128,12*128,13*128,14*128,15*128
	},
	16*128
}; /* roz_layout */

static const gfx_decode gfxdecodeinfo[] =
{
	{ NAMCONB1_TILEGFXREGION,	0, &tile_layout,	0x1000, 0x10 },
	{ NAMCONB1_SPRITEGFXREGION,	0, &obj_layout,		0x0000, 0x10 },
	{ -1 }
}; /* gfxdecodeinfo */

static const gfx_decode gfxdecodeinfo2[] =
{
	{ NAMCONB1_TILEGFXREGION,	0, &tile_layout,	0x1000, 0x08 },
	{ NAMCONB1_SPRITEGFXREGION,	0, &obj_layout,		0x0000, 0x10 },
	{ NAMCONB1_ROTGFXREGION,	0, &roz_layout,		0x1800, 0x08 },
	{ -1 }
}; /* gfxdecodeinfo2 */

/***************************************************************/

static READ32_HANDLER( gunbulet_gun_r )
{
	int result = 0;

	switch( offset )
	{
	case 0: case 1: result = (UINT8)(0x0f+readinputport(7)*224/255); break; /* Y (p2) */
	case 2: case 3: result = (UINT8)(0x26+readinputport(6)*288/314); break; /* X (p2) */
	case 4: case 5: result = (UINT8)(0x0f+readinputport(5)*224/255); break; /* Y (p1) */
	case 6: case 7: result = (UINT8)(0x26+readinputport(4)*288/314); break; /* X (p1) */
	}
	return result<<24;
} /* gunbulet_gun_r */

static
READ32_HANDLER( randgen_r )
{
	return mame_rand();
} /* randgen_r */

static
WRITE32_HANDLER( srand_w )
{
	/**
     * Used to seed the hardware random number generator.
     * We don't yet know the algorithm that is used, so for now this is a NOP.
     */
} /* srand_w */

static WRITE32_HANDLER( sharedram_w )
{
	if (offset < 0xb0)
	{
		if (mem_mask == 0x0000ffff)
		{
			namcoc7x_sound_write16((data>>16), offset*2);
		}
		else if (mem_mask == 0xffff0000)
		{
			namcoc7x_sound_write16((data&0xffff), (offset*2)+1);
		}
	}

	COMBINE_DATA(&namconb1_workram32[offset]);
}

static ADDRESS_MAP_START( namconb1_am, ADDRESS_SPACE_PROGRAM, 32 )
	AM_RANGE(0x000000, 0x0fffff) AM_READ(MRA32_ROM) AM_WRITE(MWA32_ROM)
	AM_RANGE(0x100000, 0x10001f) AM_READ(gunbulet_gun_r)
	AM_RANGE(0x1c0000, 0x1cffff) AM_READ(MRA32_RAM) AM_WRITE(MWA32_RAM)
	AM_RANGE(0x1e4000, 0x1e4003) AM_READWRITE(randgen_r,srand_w)
	AM_RANGE(0x200000, 0x2fffff) AM_READ(MRA32_RAM) AM_WRITE(sharedram_w) AM_BASE(&namconb1_workram32) /* shared with MCU) */
	AM_RANGE(0x400000, 0x40001f) AM_READ(MRA32_RAM) AM_WRITE(namconb_cpureg_w) AM_BASE(&namconb_cpureg32)
	AM_RANGE(0x580000, 0x5807ff) AM_READ(MRA32_RAM) AM_WRITE(MWA32_RAM) AM_BASE(&nvmem32)
	AM_RANGE(0x600000, 0x61ffff) AM_READWRITE(namco_obj32_r,namco_obj32_w)
	AM_RANGE(0x620000, 0x620007) AM_READWRITE(namco_spritepos32_r,namco_spritepos32_w)
	AM_RANGE(0x640000, 0x64ffff) AM_READWRITE(namco_tilemapvideoram32_r,namco_tilemapvideoram32_w )
	AM_RANGE(0x660000, 0x66003f) AM_READWRITE(namco_tilemapcontrol32_r,namco_tilemapcontrol32_w)
	AM_RANGE(0x680000, 0x68000f) AM_READ(MRA32_RAM) AM_WRITE(MWA32_RAM) AM_BASE(&namconb1_spritebank32)
	AM_RANGE(0x6e0000, 0x6e001f) AM_READ(custom_key_r) AM_WRITE(MWA32_NOP)
	AM_RANGE(0x700000, 0x707fff) AM_READ(MRA32_RAM) AM_WRITE(MWA32_RAM) AM_BASE(&paletteram32)
ADDRESS_MAP_END

static ADDRESS_MAP_START( namconb2_am, ADDRESS_SPACE_PROGRAM, 32 )
	AM_RANGE(0x000000, 0x0fffff) AM_READ(MRA32_ROM) AM_WRITE(MWA32_ROM)
	AM_RANGE(0x1c0000, 0x1cffff) AM_READ(MRA32_RAM) AM_WRITE(MWA32_RAM)
	AM_RANGE(0x1e4000, 0x1e4003) AM_READWRITE(randgen_r,srand_w)
	AM_RANGE(0x200000, 0x2fffff) AM_READ(MRA32_RAM) AM_WRITE(sharedram_w) AM_BASE(&namconb1_workram32) /* shared with MCU */
	AM_RANGE(0x400000, 0x4fffff) AM_READ(MRA32_BANK1)/* data ROMs */
	AM_RANGE(0x600000, 0x61ffff) AM_READWRITE(namco_obj32_r,namco_obj32_w)
	AM_RANGE(0x620000, 0x620007) AM_READWRITE(namco_spritepos32_r,namco_spritepos32_w)
	AM_RANGE(0x640000, 0x64000f) AM_READ(MRA32_RAM) AM_WRITE(MWA32_RAM) /* unknown xy offset */
	AM_RANGE(0x680000, 0x68ffff) AM_READWRITE(namco_tilemapvideoram32_r, namco_tilemapvideoram32_w )
	AM_RANGE(0x6c0000, 0x6c003f) AM_READWRITE(namco_tilemapcontrol32_r, namco_tilemapcontrol32_w )
	AM_RANGE(0x700000, 0x71ffff) AM_READWRITE(namco_rozvideoram32_r,namco_rozvideoram32_w)
	AM_RANGE(0x740000, 0x74001f) AM_READWRITE(namco_rozcontrol32_r,namco_rozcontrol32_w)
	AM_RANGE(0x800000, 0x807fff) AM_READ(MRA32_RAM) AM_WRITE(MWA32_RAM) AM_BASE(&paletteram32)
	AM_RANGE(0x900008, 0x90000f) AM_READ(MRA32_RAM) AM_WRITE(MWA32_RAM) AM_BASE(&namconb1_spritebank32)
	AM_RANGE(0x940000, 0x94000f) AM_READ(MRA32_RAM) AM_WRITE(MWA32_RAM) AM_BASE(&namconb1_tilebank32)
	AM_RANGE(0x980000, 0x98000f) AM_READ(namco_rozbank32_r) AM_WRITE(namco_rozbank32_w)
	AM_RANGE(0xa00000, 0xa007ff) AM_READ(MRA32_RAM) AM_WRITE(MWA32_RAM) AM_BASE(&nvmem32)
	AM_RANGE(0xc00000, 0xc0001f) AM_READ(custom_key_r) AM_WRITE(MWA32_NOP)
	AM_RANGE(0xf00000, 0xf0001f) AM_READ(MRA32_RAM) AM_WRITE(MWA32_RAM) AM_BASE(&namconb_cpureg32)
ADDRESS_MAP_END /* namconb2_readmem */

#define MASTER_CLOCK_HZ 48384000

NAMCO_C7X_HARDWARE

static MACHINE_DRIVER_START( namconb1 )
	MDRV_CPU_ADD(M68EC020,MASTER_CLOCK_HZ/2)
	MDRV_CPU_PROGRAM_MAP(namconb1_am,0)
	MDRV_CPU_VBLANK_INT(namconb1_interrupt,1)

	NAMCO_C7X_MCU(MASTER_CLOCK_HZ/3)

	MDRV_FRAMES_PER_SECOND(59.7)
	MDRV_VBLANK_DURATION(DEFAULT_REAL_60HZ_VBLANK_DURATION)
	MDRV_NVRAM_HANDLER(namconb1)
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER|VIDEO_HAS_SHADOWS)
	MDRV_SCREEN_SIZE(NAMCONB1_COLS*8, NAMCONB1_ROWS*8) /* 288x224 pixels */
	MDRV_VISIBLE_AREA(0*8, NAMCONB1_COLS*8-1, 0*8, NAMCONB1_ROWS*8-1)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(0x2000)
	MDRV_VIDEO_START(namconb1)
	MDRV_VIDEO_UPDATE(namconb1)

	NAMCO_C7X_SOUND(MASTER_CLOCK_HZ/3)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( namconb2 )
	MDRV_CPU_ADD(M68EC020,MASTER_CLOCK_HZ/2)
	MDRV_CPU_PROGRAM_MAP(namconb2_am,0)
	MDRV_CPU_VBLANK_INT(namconb2_interrupt,1)

	NAMCO_C7X_MCU(MASTER_CLOCK_HZ/3)

	MDRV_FRAMES_PER_SECOND(59.7)
	MDRV_VBLANK_DURATION(DEFAULT_REAL_60HZ_VBLANK_DURATION)
	MDRV_NVRAM_HANDLER(namconb1)
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER|VIDEO_HAS_SHADOWS)
	MDRV_SCREEN_SIZE(NAMCONB1_COLS*8, NAMCONB1_ROWS*8) /* 288x224 pixels */
	MDRV_VISIBLE_AREA(0*8, NAMCONB1_COLS*8-1, 0*8, NAMCONB1_ROWS*8-1)
	MDRV_GFXDECODE(gfxdecodeinfo2)
	MDRV_PALETTE_LENGTH(0x2000)
	MDRV_VIDEO_START(namconb2)
	MDRV_VIDEO_UPDATE(namconb2)

	NAMCO_C7X_SOUND(MASTER_CLOCK_HZ/3)
MACHINE_DRIVER_END

/***************************************************************/

ROM_START( ptblank )
	ROM_REGION( 0x100000, REGION_CPU1, 0 ) /* main program */
	ROM_LOAD32_WORD( "gn2mprlb.15b", 0x00002, 0x80000, CRC(fe2d9425) SHA1(51b166a629cbb522720d63720558816b496b6b76) )
	ROM_LOAD32_WORD( "gn2mprub.13b", 0x00000, 0x80000, CRC(3bf4985a) SHA1(f559e0d5f55d23d886fe61bd7d5ca556acc7f87c) )

	ROM_REGION16_LE( 0x100000, REGION_USER4, 0 ) /* sound data and MCU BIOS */
	ROM_LOAD( "gn1-spr0.bin", 0, 0x20000, CRC(6836ba38) SHA1(6ea17ea4bbb59be108e8887acd7871409580732f) )
	NAMCO_C7X_BIOS

	ROM_REGION( 0x200000, REGION_SOUND1, 0 )
	ROM_LOAD( "gn1-voi0.bin", 0, 0x200000, CRC(05477eb7) SHA1(f2eaacb5dbac06c37c56b9b131230c9cf6602221) )

	ROM_REGION( 0x800000, NAMCONB1_SPRITEGFXREGION, ROMREGION_DISPOSE )
	ROM_LOAD16_BYTE( "gn1obj0l.bin", 0x000001, 0x200000, CRC(06722dc8) SHA1(56fee4e17ed707fa6dbc6bad0d0281fc8cdf72d1) )
	ROM_LOAD16_BYTE( "gn1obj0u.bin", 0x000000, 0x200000, CRC(fcefc909) SHA1(48c19b6032096dd80777aa6d5eb5f90463095cbe) )
	ROM_LOAD16_BYTE( "gn1obj1u.bin", 0x400000, 0x200000, CRC(3109a071) SHA1(4bb16df5a3aecdf37baf843edfc82952d46f5227) )
	ROM_LOAD16_BYTE( "gn1obj1l.bin", 0x400001, 0x200000, CRC(48468df7) SHA1(c5fb9082c84ac2ffceb6f5f4cbc1d40047c55e3d) )

	ROM_REGION( 0x400000, NAMCONB1_TILEGFXREGION, ROMREGION_DISPOSE )
	ROM_LOAD( "gn1-chr0.bin", 0x000000, 0x100000, CRC(a5c61246) SHA1(d1d9f286b93b5b9880160029c53384d13c08dd8a) )
	ROM_LOAD( "gn1-chr1.bin", 0x100000, 0x100000, CRC(c8c59772) SHA1(91de633a300e3b25a919579eaada5549640ab6f0) )
	ROM_LOAD( "gn1-chr2.bin", 0x200000, 0x100000, CRC(dc96d999) SHA1(d006a401762b57fef6716f56eb3a7edcb3d3c00e) )
	ROM_LOAD( "gn1-chr3.bin", 0x300000, 0x100000, CRC(4352c308) SHA1(785c13df219dceac2f940519141665b630a29f86) )

	ROM_REGION( 0x80000, NAMCONB1_TILEMASKREGION, 0 )
	ROM_LOAD( "gn1-sha0.bin", 0, 0x80000, CRC(86d4ff85) SHA1(a71056b2bcbba50c834fe28269ebda9719df354a) )
ROM_END

ROM_START( gunbulet )
	ROM_REGION( 0x100000, REGION_CPU1, 0 ) /* main program */
	ROM_LOAD32_WORD( "gn1-mprl.bin", 0x00002, 0x80000, CRC(f99e309e) SHA1(3fe0ddf756e6849f8effc7672456cbe32f65c98a) )
	ROM_LOAD32_WORD( "gn1-mpru.bin", 0x00000, 0x80000, CRC(72a4db07) SHA1(8c5e1e51cd961b311d03f7b21f36a5bd5e8e9104) )

	ROM_REGION16_LE( 0x100000, REGION_USER4, 0 ) /* sound data and MCU BIOS */
	ROM_LOAD( "gn1-spr0.bin", 0, 0x20000, CRC(6836ba38) SHA1(6ea17ea4bbb59be108e8887acd7871409580732f) )
	NAMCO_C7X_BIOS

	ROM_REGION( 0x200000, REGION_SOUND1, 0 )
	ROM_LOAD( "gn1-voi0.bin", 0, 0x200000, CRC(05477eb7) SHA1(f2eaacb5dbac06c37c56b9b131230c9cf6602221) )

	ROM_REGION( 0x800000, NAMCONB1_SPRITEGFXREGION, ROMREGION_DISPOSE )
	ROM_LOAD16_BYTE( "gn1obj0l.bin", 0x000001, 0x200000, CRC(06722dc8) SHA1(56fee4e17ed707fa6dbc6bad0d0281fc8cdf72d1) )
	ROM_LOAD16_BYTE( "gn1obj0u.bin", 0x000000, 0x200000, CRC(fcefc909) SHA1(48c19b6032096dd80777aa6d5eb5f90463095cbe) )
	ROM_LOAD16_BYTE( "gn1obj1u.bin", 0x400000, 0x200000, CRC(3109a071) SHA1(4bb16df5a3aecdf37baf843edfc82952d46f5227) )
	ROM_LOAD16_BYTE( "gn1obj1l.bin", 0x400001, 0x200000, CRC(48468df7) SHA1(c5fb9082c84ac2ffceb6f5f4cbc1d40047c55e3d) )

	ROM_REGION( 0x400000, NAMCONB1_TILEGFXREGION, ROMREGION_DISPOSE )
	ROM_LOAD( "gn1-chr0.bin", 0x000000, 0x100000, CRC(a5c61246) SHA1(d1d9f286b93b5b9880160029c53384d13c08dd8a) )
	ROM_LOAD( "gn1-chr1.bin", 0x100000, 0x100000, CRC(c8c59772) SHA1(91de633a300e3b25a919579eaada5549640ab6f0) )
	ROM_LOAD( "gn1-chr2.bin", 0x200000, 0x100000, CRC(dc96d999) SHA1(d006a401762b57fef6716f56eb3a7edcb3d3c00e) )
	ROM_LOAD( "gn1-chr3.bin", 0x300000, 0x100000, CRC(4352c308) SHA1(785c13df219dceac2f940519141665b630a29f86) )

	ROM_REGION( 0x80000, NAMCONB1_TILEMASKREGION, 0 )
	ROM_LOAD( "gn1-sha0.bin", 0, 0x80000, CRC(86d4ff85) SHA1(a71056b2bcbba50c834fe28269ebda9719df354a) )
ROM_END

ROM_START( nebulray )
	ROM_REGION( 0x100000, REGION_CPU1, 0 ) /* main program */
	ROM_LOAD32_WORD( "nr2-mpru.13b", 0x00000, 0x80000, CRC(049b97cb) SHA1(0e344b29a4d4bdc854fa9849589772df2eeb0a05) )
	ROM_LOAD32_WORD( "nr2-mprl.15b", 0x00002, 0x80000, CRC(0431b6d4) SHA1(54c96e8ac9e753956c31bdef79d390f1c20e10ff) )

	ROM_REGION16_LE( 0x100000, REGION_USER4, 0 ) /* sound data and MCU BIOS */
	ROM_LOAD( "nr1-spr0", 0, 0x20000, CRC(1cc2b44b) SHA1(161f4ed39fabe89d7ee1d539f8b9f08cd0ff3111) )
	NAMCO_C7X_BIOS

	ROM_REGION( 0x200000, REGION_SOUND1, 0 )
	ROM_LOAD( "nr1-voi0", 0, 0x200000, CRC(332d5e26) SHA1(9daddac3fbe0709e25ed8e0b456bac15bfae20d7) )

	ROM_REGION( 0x1000000, NAMCONB1_SPRITEGFXREGION, ROMREGION_DISPOSE )
	ROM_LOAD16_BYTE( "nr1obj0u", 0x000000, 0x200000, CRC(fb82a881) SHA1(c9fa0728a37376a5c85bff1f6e8400c13ce15769) )
	ROM_LOAD16_BYTE( "nr1obj0l", 0x000001, 0x200000, CRC(0e99ef46) SHA1(450fe61e448270b633f312361bd5ca89bb9684dd) )
	ROM_LOAD16_BYTE( "nr1obj1u", 0x400000, 0x200000, CRC(49d9dbd7) SHA1(2dbd842c192d65888f931cdb5c9387127b1ab632) )
	ROM_LOAD16_BYTE( "nr1obj1l", 0x400001, 0x200000, CRC(f7a898f0) SHA1(a25a134a42adeb9088019bde42a96d120f20407e) )
	ROM_LOAD16_BYTE( "nr1obj2u", 0x800000, 0x200000, CRC(8c8205b1) SHA1(2c5fb9392d8cd5f8d1f9aba6ddbbafd061271cd4) )
	ROM_LOAD16_BYTE( "nr1obj2l", 0x800001, 0x200000, CRC(b39871d1) SHA1(a8f910702bb88a001f2bfd1b33ad355aa3b0f429) )
	ROM_LOAD16_BYTE( "nr1obj3u", 0xc00000, 0x200000, CRC(d5918c9e) SHA1(530781fb44d7bbf01669bb265b658cb60e27bcd7) )
	ROM_LOAD16_BYTE( "nr1obj3l", 0xc00001, 0x200000, CRC(c90d13ae) SHA1(675f7b8b3325aac91b2bae1cbebe274a65aedc43) )

	ROM_REGION( 0x400000, NAMCONB1_TILEGFXREGION, ROMREGION_DISPOSE )
	ROM_LOAD( "nr1-chr0", 0x000000, 0x100000,CRC(8d5b54ea) SHA1(616d5729f474da91da19a8246066280652da998c) )
	ROM_LOAD( "nr1-chr1", 0x100000, 0x100000,CRC(cd21630c) SHA1(9974c0eb1051ca52f001e6631264a1936bb50620) )
	ROM_LOAD( "nr1-chr2", 0x200000, 0x100000,CRC(70a11023) SHA1(bead486a86bd96c6fdfd2ea4d4d37c38bbe9bfbb) )
	ROM_LOAD( "nr1-chr3", 0x300000, 0x100000,CRC(8f4b1d51) SHA1(b48fb2c8ccd9105a5b48be44dd3fe4309769efa4) )

	ROM_REGION( 0x80000, NAMCONB1_TILEMASKREGION, 0 )
	ROM_LOAD( "nr1-sha0", 0, 0x80000,CRC(ca667e13) SHA1(685032603224cb81bcb85361921477caec570d5e) )

	ROM_REGION( 0x20, REGION_PROMS, 0 ) /* custom key data? */
	ROM_LOAD( "c366.bin", 0, 0x20, CRC(8c96f31d) SHA1(d186859cfc19a63266084372080d0a5bee687ae2) )
ROM_END

ROM_START( nebulryj )
	ROM_REGION( 0x100000, REGION_CPU1, 0 ) /* main program */
	ROM_LOAD32_WORD( "nr1-mpru", 0x00000, 0x80000, CRC(42ef71f9) SHA1(20e3cb63e1fde293c60c404b378d901d635c4b79) )
	ROM_LOAD32_WORD( "nr1-mprl", 0x00002, 0x80000, CRC(fae5f62c) SHA1(143d716abbc834aac6270db3bbb89ec71ea3804d) )

	ROM_REGION16_LE( 0x100000, REGION_USER4, 0 ) /* sound data and MCU BIOS */
	ROM_LOAD( "nr1-spr0", 0, 0x20000, CRC(1cc2b44b) SHA1(161f4ed39fabe89d7ee1d539f8b9f08cd0ff3111) )
	NAMCO_C7X_BIOS

	ROM_REGION( 0x200000, REGION_SOUND1, 0 )
	ROM_LOAD( "nr1-voi0", 0, 0x200000, CRC(332d5e26) SHA1(9daddac3fbe0709e25ed8e0b456bac15bfae20d7) )

	ROM_REGION( 0x1000000, NAMCONB1_SPRITEGFXREGION, ROMREGION_DISPOSE )
	ROM_LOAD16_BYTE( "nr1obj0u", 0x000000, 0x200000, CRC(fb82a881) SHA1(c9fa0728a37376a5c85bff1f6e8400c13ce15769) )
	ROM_LOAD16_BYTE( "nr1obj0l", 0x000001, 0x200000, CRC(0e99ef46) SHA1(450fe61e448270b633f312361bd5ca89bb9684dd) )
	ROM_LOAD16_BYTE( "nr1obj1u", 0x400000, 0x200000, CRC(49d9dbd7) SHA1(2dbd842c192d65888f931cdb5c9387127b1ab632) )
	ROM_LOAD16_BYTE( "nr1obj1l", 0x400001, 0x200000, CRC(f7a898f0) SHA1(a25a134a42adeb9088019bde42a96d120f20407e) )
	ROM_LOAD16_BYTE( "nr1obj2u", 0x800000, 0x200000, CRC(8c8205b1) SHA1(2c5fb9392d8cd5f8d1f9aba6ddbbafd061271cd4) )
	ROM_LOAD16_BYTE( "nr1obj2l", 0x800001, 0x200000, CRC(b39871d1) SHA1(a8f910702bb88a001f2bfd1b33ad355aa3b0f429) )
	ROM_LOAD16_BYTE( "nr1obj3u", 0xc00000, 0x200000, CRC(d5918c9e) SHA1(530781fb44d7bbf01669bb265b658cb60e27bcd7) )
	ROM_LOAD16_BYTE( "nr1obj3l", 0xc00001, 0x200000, CRC(c90d13ae) SHA1(675f7b8b3325aac91b2bae1cbebe274a65aedc43) )

	ROM_REGION( 0x400000, NAMCONB1_TILEGFXREGION, ROMREGION_DISPOSE )
	ROM_LOAD( "nr1-chr0", 0x000000, 0x100000,CRC(8d5b54ea) SHA1(616d5729f474da91da19a8246066280652da998c) )
	ROM_LOAD( "nr1-chr1", 0x100000, 0x100000,CRC(cd21630c) SHA1(9974c0eb1051ca52f001e6631264a1936bb50620) )
	ROM_LOAD( "nr1-chr2", 0x200000, 0x100000,CRC(70a11023) SHA1(bead486a86bd96c6fdfd2ea4d4d37c38bbe9bfbb) )
	ROM_LOAD( "nr1-chr3", 0x300000, 0x100000,CRC(8f4b1d51) SHA1(b48fb2c8ccd9105a5b48be44dd3fe4309769efa4) )

	ROM_REGION( 0x80000, NAMCONB1_TILEMASKREGION, 0 )
	ROM_LOAD( "nr1-sha0", 0, 0x80000,CRC(ca667e13) SHA1(685032603224cb81bcb85361921477caec570d5e) )

	ROM_REGION( 0x20, REGION_PROMS, 0 ) /* custom key data? */
	ROM_LOAD( "c366.bin", 0, 0x20, CRC(8c96f31d) SHA1(d186859cfc19a63266084372080d0a5bee687ae2) )
ROM_END

ROM_START( gslgr94u )
	ROM_REGION( 0x100000, REGION_CPU1, 0 ) /* main program */
	ROM_LOAD32_WORD( "gse2mprl.bin", 0x00002, 0x80000, CRC(a514349c) SHA1(1f7ec81cd6193410d2f01e6f0f84878561fc8035) )
	ROM_LOAD32_WORD( "gse2mpru.bin", 0x00000, 0x80000, CRC(b6afd238) SHA1(438a3411ac8ce3d22d5da8c0800738cb8d2994a9) )

	ROM_REGION16_LE( 0x100000, REGION_USER4, 0 ) /* sound data and MCU BIOS */
	ROM_LOAD( "gse2spr0.bin", 0, 0x20000, CRC(17e87cfc) SHA1(9cbeadb6dfcb736e8c80eab344f70fc2f58469d6) )
	NAMCO_C7X_BIOS

	ROM_REGION( 0x200000, REGION_SOUND1, 0 )
	ROM_LOAD( "gse-voi0.bin", 0, 0x200000, CRC(d3480574) SHA1(0c468ed060769b36b7e41cf4919cb6d8691d64f6) )

	ROM_REGION( 0x400000, NAMCONB1_SPRITEGFXREGION, ROMREGION_DISPOSE )
	ROM_LOAD16_BYTE( "gseobj0l.bin", 0x000001, 0x200000, CRC(531520ca) SHA1(2a1a5282549c6f7a37d5fb8c0b342edb9dc45315) )
	ROM_LOAD16_BYTE( "gseobj0u.bin", 0x000000, 0x200000, CRC(fcc1283c) SHA1(fb44ed742f362e6737412cabf3f67d9506456a9e) )

	ROM_REGION( 0x400000, NAMCONB1_TILEGFXREGION, ROMREGION_DISPOSE )
	ROM_LOAD( "gse-chr0.bin", 0x000000, 0x100000, CRC(9314085d) SHA1(150e8ea908861337f9be2749aa7f9e1d52570586) )
	ROM_LOAD( "gse-chr1.bin", 0x100000, 0x100000, CRC(c128a887) SHA1(4faf78064dd48ec50684a7dc8d120f8c5985bf2a) )
	ROM_LOAD( "gse-chr2.bin", 0x200000, 0x100000, CRC(48f0a311) SHA1(e39adcce835542e64ca87f6019d4a85fcbe388c2) )
	ROM_LOAD( "gse-chr3.bin", 0x300000, 0x100000, CRC(adbd1f88) SHA1(3c7bb1a9a398412bd3c98cadf8ce63a16e2bfed5) )

	ROM_REGION( 0x80000, NAMCONB1_TILEMASKREGION, 0 )
	ROM_LOAD( "gse-sha0.bin", 0, 0x80000, CRC(6b2beabb) SHA1(815f7aef44735584edd4a9ca7e672471d07f225e) )
ROM_END

ROM_START( gslugrsj )
	ROM_REGION( 0x100000, REGION_CPU1, 0 ) /* main program */
	ROM_LOAD32_WORD( "gs1mprl.15b", 0x00002, 0x80000, CRC(1e6c3626) SHA1(56abe21884fd87df10996db19c49ce14214d4b73) )
	ROM_LOAD32_WORD( "gs1mpru.13b", 0x00000, 0x80000, CRC(ef355179) SHA1(0ab0ef4301a318681bb5827d35734a0732b35484) )

	ROM_REGION16_LE( 0x100000, REGION_USER4, 0 ) /* sound data and MCU BIOS */
	ROM_LOAD( "gs1spr0.5b", 0, 0x80000, CRC(561ea20f) SHA1(adac6b77effc3a82079a9b228bafca0fcef72ba5) )
	NAMCO_C7X_BIOS

	ROM_REGION( 0x200000, REGION_SOUND1, 0 )
	ROM_LOAD( "gs1voi-0.5j", 0, 0x200000, CRC(6f8262aa) SHA1(beea98d9f8b927a572eb0bfcf678e9d6e40fc68d) )

	ROM_REGION( 0x400000, NAMCONB1_SPRITEGFXREGION, ROMREGION_DISPOSE )
	ROM_LOAD16_BYTE( "gs1obj-0.ic1", 0x000001, 0x200000, CRC(9a55238f) SHA1(fc3fd4b8b6322bbe343edbcad7815b597562266b) )
	ROM_LOAD16_BYTE( "gs1obj-1.ic2", 0x000000, 0x200000, CRC(31c66f76) SHA1(8903e6586dff6f34a6ffca2d7c75343c0a5bff56) )

	ROM_REGION( 0x400000, NAMCONB1_TILEGFXREGION, ROMREGION_DISPOSE )
	ROM_LOAD( "gs1chr-0.8j", 0x000000, 0x100000, CRC(e7ced86a) SHA1(de90c2e3870b317431d3910f581660681b46ff9d) )
	ROM_LOAD( "gs1chr-1.9j", 0x100000, 0x100000, CRC(1fe46749) SHA1(f4c0ea666d52cb1c8b1da93e7486ade5eae336cc) )
	ROM_LOAD( "gs1chr-2.10j", 0x200000, 0x100000, CRC(f53afa20) SHA1(5c317e276ca2355e9737c1e8114dccbb5e11058a) )
	ROM_LOAD( "gs1chr-3.11j", 0x300000, 0x100000, CRC(b149d7da) SHA1(d50c6258db0ccdd69b563e880d1711aae811fbe3) )

	ROM_REGION( 0x80000, NAMCONB1_TILEMASKREGION, 0 )
	ROM_LOAD( "gs1sha-0.5m", 0, 0x80000, CRC(8a2832fe) SHA1(a1f54754fb01bbbc87274b1a0a4127fa9296ad1a) )
ROM_END

ROM_START( sws95 )
	ROM_REGION( 0x100000, REGION_CPU1, 0 ) /* main program */
	ROM_LOAD32_WORD( "ss51mprl.bin", 0x00002, 0x80000, CRC(c9e0107d) SHA1(0f10582416023a86ea1ef2679f3f06016c086e08) )
	ROM_LOAD32_WORD( "ss51mpru.bin", 0x00000, 0x80000, CRC(0d93d261) SHA1(5edef26e2c86dbc09727d910af92747d022e4fed) )

	ROM_REGION16_LE( 0x100000, REGION_USER4, 0 ) /* sound data and MCU BIOS */
	ROM_LOAD( "ss51spr0.bin", 0, 0x80000, CRC(71cb12f5) SHA1(6e13bd16a5ba14d6e47a21875db3663ada3c06a5) )
	NAMCO_C7X_BIOS

	ROM_REGION( 0x200000, REGION_SOUND1, 0 )
	ROM_LOAD( "ss51voi0.bin", 0, 0x200000, CRC(2740ec72) SHA1(9694a7378ea72771d2b1d43db6d74ed347ba27d3) )


	ROM_REGION( 0x400000, NAMCONB1_SPRITEGFXREGION, ROMREGION_DISPOSE )
	ROM_LOAD16_BYTE( "ss51ob0l.bin", 0x000001, 0x200000, CRC(e0395694) SHA1(e52045a7af4c4b0f9935695cfc5ff729bf9bd7c1) )
	ROM_LOAD16_BYTE( "ss51ob0u.bin", 0x000000, 0x200000, CRC(b0745ca0) SHA1(579ea7fd7b9a181fd9d08c50c6c5941264aa0b6d) )

	ROM_REGION( 0x400000, NAMCONB1_TILEGFXREGION, ROMREGION_DISPOSE )
	ROM_LOAD( "ss51chr0.bin", 0x000000, 0x100000, CRC(86dd3280) SHA1(07ba6d3edc5c38bf82ddaf8f6de7ef0f5d0788b2) )
	ROM_LOAD( "ss51chr1.bin", 0x100000, 0x100000, CRC(2ba0fb9e) SHA1(39ceddad7bc0073b361eb776762002a9fc61b337) )
	ROM_LOAD( "ss51chr2.bin", 0x200000, 0x100000, CRC(ca0e6c1a) SHA1(1221cd30894e97e2f7d456509c7b6732ec3d06a5) )
	ROM_LOAD( "ss51chr3.bin", 0x300000, 0x100000, CRC(73ca58f6) SHA1(44bdc943fb10dc53279662cd528169a27d57e478) )

	ROM_REGION( 0x80000, NAMCONB1_TILEMASKREGION, 0 )
	ROM_LOAD( "ss51sha0.bin", 0, 0x80000, CRC(3bf4d081) SHA1(7b07b86f753ea6bcd90eb7d152c12884a6fe785a) )
ROM_END

ROM_START( sws96 )
	ROM_REGION( 0x100000, REGION_CPU1, 0 ) /* main program */
	ROM_LOAD32_WORD( "ss61mprl.bin", 0x00002, 0x80000, CRC(06f55e73) SHA1(6be26f8a2ef600bf07c580f210d7b265ac464002) )
	ROM_LOAD32_WORD( "ss61mpru.bin", 0x00000, 0x80000, CRC(0abdbb83) SHA1(67e8b712291f9bcf2c3a52fbc451fad54679cab8) )

	ROM_REGION16_LE( 0x100000, REGION_USER4, 0 ) /* sound data and MCU BIOS */
	ROM_LOAD( "ss61spr0.bin", 0, 0x80000, CRC(71cb12f5) SHA1(6e13bd16a5ba14d6e47a21875db3663ada3c06a5) )
	NAMCO_C7X_BIOS

	ROM_REGION( 0x200000, REGION_SOUND1, 0 )
	ROM_LOAD( "ss61voi0.bin", 0, 0x200000, CRC(2740ec72) SHA1(9694a7378ea72771d2b1d43db6d74ed347ba27d3) )

	ROM_REGION( 0x400000, NAMCONB1_SPRITEGFXREGION, ROMREGION_DISPOSE )
	ROM_LOAD16_BYTE( "ss61ob0l.bin", 0x000001, 0x200000, CRC(579b19d4) SHA1(7f18097c683d2b1c532f54ee514dd499f5965165) )
	ROM_LOAD16_BYTE( "ss61ob0u.bin", 0x000000, 0x200000, CRC(a69bbd9e) SHA1(8f4c44e2caa31d25433a04c19c51904ec9461e2f) )

	ROM_REGION( 0x400000, NAMCONB1_TILEGFXREGION, ROMREGION_DISPOSE )
	ROM_LOAD( "ss61chr0.bin", 0x000000, 0x100000, CRC(9d2ae07b) SHA1(7d268f6c7d8145c913f80049369ae3106d69e939) )
	ROM_LOAD( "ss61chr1.bin", 0x100000, 0x100000, CRC(4dc75da6) SHA1(a29932b4fb39648e2c02df668f46cafb80c53619) )
	ROM_LOAD( "ss61chr2.bin", 0x200000, 0x100000, CRC(1240704b) SHA1(a24281681053cc6649f00ec5a31c7249101eaee1) )
	ROM_LOAD( "ss61chr3.bin", 0x300000, 0x100000, CRC(066581d4) SHA1(999cd478d9da452bb57793cd276c6c0d87e2825e) )

	ROM_REGION( 0x80000, NAMCONB1_TILEMASKREGION, 0 )
	ROM_LOAD( "ss61sha0.bin", 0, 0x80000, CRC(fceaa19c) SHA1(c9303a755ac7af19c4804a264d1a09d987f39e74) )
ROM_END

ROM_START( sws97 )
	ROM_REGION( 0x100000, REGION_CPU1, 0 ) /* main program */
	ROM_LOAD32_WORD( "ss71mprl.bin", 0x00002, 0x80000, CRC(bd60b50e) SHA1(9e00bacd506182ab2af2c0efdd5cc401b3e46485) )
	ROM_LOAD32_WORD( "ss71mpru.bin", 0x00000, 0x80000, CRC(3444f5a8) SHA1(8d0f35b3ba8f65dbc67c3b2d273833227a8b8b2a) )

	ROM_REGION16_LE( 0x100000, REGION_USER4, 0 ) /* sound data and MCU BIOS */
	ROM_LOAD( "ss71spr0.bin", 0, 0x80000, CRC(71cb12f5) SHA1(6e13bd16a5ba14d6e47a21875db3663ada3c06a5) )
	NAMCO_C7X_BIOS

	ROM_REGION( 0x200000, REGION_SOUND1, 0 )
	ROM_LOAD( "ss71voi0.bin", 0, 0x200000, CRC(2740ec72) SHA1(9694a7378ea72771d2b1d43db6d74ed347ba27d3) )

	ROM_REGION( 0x400000, NAMCONB1_SPRITEGFXREGION, ROMREGION_DISPOSE )
	ROM_LOAD16_BYTE( "ss71ob0l.bin", 0x000001, 0x200000, CRC(9559ad44) SHA1(fd56a8620f6958cc090f783d74cb38bba46d2423) )
	ROM_LOAD16_BYTE( "ss71ob0u.bin", 0x000000, 0x200000, CRC(4df4a722) SHA1(07eb94628ceeb7cbce2d39d479f33c37583a346a) )

	ROM_REGION( 0x400000, NAMCONB1_TILEGFXREGION, ROMREGION_DISPOSE )
	ROM_LOAD( "ss71chr0.bin", 0x000000, 0x100000, CRC(bd606356) SHA1(a62c55600e46f8821db0b84d79fc2588742ad7ad) )
	ROM_LOAD( "ss71chr1.bin", 0x100000, 0x100000, CRC(4dc75da6) SHA1(a29932b4fb39648e2c02df668f46cafb80c53619) )
	ROM_LOAD( "ss71chr2.bin", 0x200000, 0x100000, CRC(1240704b) SHA1(a24281681053cc6649f00ec5a31c7249101eaee1) )
	ROM_LOAD( "ss71chr3.bin", 0x300000, 0x100000, CRC(066581d4) SHA1(999cd478d9da452bb57793cd276c6c0d87e2825e) )

	ROM_REGION( 0x80000, NAMCONB1_TILEMASKREGION, 0 )
	ROM_LOAD( "ss71sha0.bin", 0, 0x80000, CRC(be8c2758) SHA1(0a1b6c03cdaec6103ae8483b67faf3840234f825) )
ROM_END

ROM_START( vshoot )
	ROM_REGION( 0x100000, REGION_CPU1, 0 ) /* main program */
	ROM_LOAD32_WORD( "vsj1mprl.15b", 0x00002, 0x80000, CRC(83a60d92) SHA1(c3db0c79f772a79418914353a3d6ecc4883ea54e) )
	ROM_LOAD32_WORD( "vsj1mpru.13b", 0x00000, 0x80000, CRC(c63eb92d) SHA1(f93bd4b91daee645677955020dc8df14dc9bfd27) )

	ROM_REGION16_LE( 0x100000, REGION_USER4, 0 ) /* sound data and MCU BIOS */
	ROM_LOAD( "vsj1spr0.5b", 0, 0x80000, CRC(b0c71aa6) SHA1(a94fae02b46a645ff728d2f98827c85ff155892b) )
	NAMCO_C7X_BIOS

	ROM_REGION( 0x200000, REGION_SOUND1, 0 )
	ROM_LOAD( "vsjvoi-0.5j", 0, 0x200000, CRC(0528c9ed) SHA1(52b67978fdeb97b77065575774a7ddeb49fe1d81) )

	ROM_REGION( 0x800000, NAMCONB1_SPRITEGFXREGION, ROMREGION_DISPOSE )
	ROM_LOAD16_BYTE( "vsjobj0l.ic1", 0x000001, 0x200000, CRC(e134faa7) SHA1(a844c8a5bd6d8907f9e5c7ba9e2ee8e9a886cd1e) )
	ROM_LOAD16_BYTE( "vsjobj0u.ic2", 0x000000, 0x200000, CRC(974d0714) SHA1(976050eaf82d4b66e13c1c579e5521eb867527fb) )
	ROM_LOAD16_BYTE( "vsjobj1l.ic3", 0x400001, 0x200000, CRC(ba46f967) SHA1(ddfb0ac7fba7369869e4df9a66d465a662eba2e6) )
	ROM_LOAD16_BYTE( "vsjobj1u.ic4", 0x400000, 0x200000, CRC(09da7e9c) SHA1(e98e07a886a4fe369748fc97f3cee6a4bb668385) )

	ROM_REGION( 0x400000, NAMCONB1_TILEGFXREGION, ROMREGION_DISPOSE )
	ROM_LOAD( "vsjchr-0.8j",  0x000000, 0x100000, CRC(2af8ba7c) SHA1(74f5a382425974a9b2167bb01672dd13dea882f5) )
	ROM_LOAD( "vsjchr-1.9j",  0x100000, 0x100000, CRC(b789d53e) SHA1(48b4cf956f9025e3c2b6f59b317596dfe0b6b142) )
	ROM_LOAD( "vsjchr-2.10j", 0x200000, 0x100000, CRC(7ef80758) SHA1(c7e6d14f0823607dfd8a13ea6f164ffa85b5563e) )
	ROM_LOAD( "vsjchr-3.11j", 0x300000, 0x100000, CRC(73ca58f6) SHA1(44bdc943fb10dc53279662cd528169a27d57e478) )

	ROM_REGION( 0x80000, NAMCONB1_TILEMASKREGION, 0 )
	ROM_LOAD( "vsjsha-0.5m", 0, 0x80000, CRC(78335ea4) SHA1(d4b9f179b1b456a866354ea308664c036de6414d) )
ROM_END

ROM_START( outfxies )
	ROM_REGION( 0x100000, REGION_CPU1, 0 ) /* main program */
	ROM_LOAD32_WORD( "ou2mprl.11c", 0x00002, 0x80000, CRC(f414a32e) SHA1(9733ab087cfde1b8fb5b676d8a2eb5325ebdbb56) )
	ROM_LOAD32_WORD( "ou2mpru.11d", 0x00000, 0x80000, CRC(ab5083fb) SHA1(cb2e7a4838c2b80057edb83ea63116bccb1394d3) )

	ROM_REGION16_LE( 0x100000, REGION_USER4, 0 ) /* sound data and MCU BIOS */
	ROM_LOAD( "ou1spr0.5b", 0, 0x80000, CRC(60cee566) SHA1(2f3b96793816d90011586e0f9f71c58b636b6d4c) )
	NAMCO_C7X_BIOS

	ROM_REGION( 0x200000, REGION_SOUND1, 0 )
	ROM_LOAD( "ou1voi0.6n", 0, 0x200000, CRC(2d8fb271) SHA1(bde9d45979728f5a2cd8ec89f5f81bf16b694cc2) )

	ROM_REGION( 0x200000, NAMCONB1_TILEMASKREGION, 0 )
	ROM_LOAD( "ou1shas.12s", 0, 0x200000,CRC(9bcb0397) SHA1(54a32b6394d0e6f51bfd281f8a4bafce6ddf6246) )

	ROM_REGION( 0x200000, NAMCONB1_ROTMASKREGION, 0 )
	ROM_LOAD( "ou1shar.18s", 0, 0x200000,	CRC(fbb48194) SHA1(2d3ec5bc519fad2b755018f83fadfe0cba13c292) )

	ROM_REGION( 0x2000000, NAMCONB1_SPRITEGFXREGION, ROMREGION_DISPOSE )
	ROM_LOAD16_BYTE( "ou1obj0l", 0x0000001, 0x200000, CRC(1b4f7184) SHA1(a05d67842fce92f321d1fdd3bd30aa3427775a0c) )
	ROM_LOAD16_BYTE( "ou1obj0u", 0x0000000, 0x200000, CRC(d0a69794) SHA1(07d449e54e9971abeb9cd5bb7b372270fafa8bac) )
	ROM_LOAD16_BYTE( "ou1obj1l", 0x0400001, 0x200000, CRC(48a93e84) SHA1(6935ec161a12237d4cec732d42070f381c23b47c) )
	ROM_LOAD16_BYTE( "ou1obj1u", 0x0400000, 0x200000, CRC(999de386) SHA1(d4780ab1929a3e2c2df464363d6451a2bcecb2a2) )
	ROM_LOAD16_BYTE( "ou1obj2l", 0x0800001, 0x200000, CRC(30386cd0) SHA1(3563c5378288da58136f102381373bd6fcaeec21) )
	ROM_LOAD16_BYTE( "ou1obj2u", 0x0800000, 0x200000, CRC(ccada5f8) SHA1(75ed95bb295780126879d67bba4d0ae1da63c928) )
	ROM_LOAD16_BYTE( "ou1obj3l", 0x0c00001, 0x200000, CRC(5f41b44e) SHA1(3f5376fcd3e15af772df65b8eda4d5ee07ee5664) )
	ROM_LOAD16_BYTE( "ou1obj3u", 0x0c00000, 0x200000, CRC(bc852c8e) SHA1(4863302c45ee16aaf2c36dac07aceaf287959c53) )
	ROM_LOAD16_BYTE( "ou1obj4l", 0x1000001, 0x200000, CRC(99a5f9d7) SHA1(b0f46f4ac357918137031a19c36a56a47b7aefd6) )
	ROM_LOAD16_BYTE( "ou1obj4u", 0x1000000, 0x200000, CRC(70ecaabb) SHA1(521c6849526fb271e6447f6c4f5bfa081f96b91e) )

	ROM_REGION( 0x600000, NAMCONB1_ROTGFXREGION, ROMREGION_DISPOSE )
	ROM_LOAD( "ou1-rot0", 0x000000, 0x200000, CRC(a50c67c8) SHA1(432b8451eb9eaa3078134fce1e5e2d58a8b64be3) )
	ROM_LOAD( "ou1-rot1", 0x200000, 0x200000, CRC(14866780) SHA1(4a54151fada4dfba7232e53e40623e5697eeb7db) )
	ROM_LOAD( "ou1-rot2", 0x400000, 0x200000, CRC(55ccf3af) SHA1(d98489aaa840cbffb21c47609961c1163b0336f3) )

	ROM_REGION( 0x200000, NAMCONB1_TILEGFXREGION, ROMREGION_DISPOSE )
	ROM_LOAD( "ou1-scr0", 0x000000, 0x200000, CRC(b3b3f2e9) SHA1(541bd7e9ba12aff4ec4033bd9c6bb19476acb3c4) )

	ROM_REGION( 0x100000, REGION_USER1, 0 )
	ROM_LOAD( "ou1dat0.20a", 0x00000, 0x80000, CRC(1a49aead) SHA1(df243aff1a6fb5bcf4d5d883c5af2374a4aff477) )
	ROM_LOAD( "ou1dat1.20b", 0x80000, 0x80000, CRC(63bb119d) SHA1(d4c2820243b84c3f5cdf7f9e66bb50f53d0efed2) )
ROM_END

ROM_START( outfxesj )
	ROM_REGION( 0x100000, REGION_CPU1, 0 ) /* main program */
	ROM_LOAD32_WORD( "ou1-mprl.11c", 0x00002, 0x80000, CRC(d3b9e530) SHA1(3f5fe5eea817a23dfe42e76f32912ce94d4c49c9) )
	ROM_LOAD32_WORD( "ou1-mpru.11d", 0x00000, 0x80000, CRC(d98308fb) SHA1(fdefeebf56464a20e3aaefd88df4eee9f7b5c4f3) )

	ROM_REGION16_LE( 0x100000, REGION_USER4, 0 ) /* sound data and MCU BIOS */
	ROM_LOAD( "ou1spr0.5b", 0, 0x80000, CRC(60cee566) SHA1(2f3b96793816d90011586e0f9f71c58b636b6d4c) )
	NAMCO_C7X_BIOS

	ROM_REGION( 0x200000, REGION_SOUND1, 0 )
	ROM_LOAD( "ou1voi0.6n", 0, 0x200000, CRC(2d8fb271) SHA1(bde9d45979728f5a2cd8ec89f5f81bf16b694cc2) )

	ROM_REGION( 0x200000, NAMCONB1_TILEMASKREGION, 0 )
	ROM_LOAD( "ou1shas.12s", 0, 0x200000,CRC(9bcb0397) SHA1(54a32b6394d0e6f51bfd281f8a4bafce6ddf6246) )

	ROM_REGION( 0x200000, NAMCONB1_ROTMASKREGION, 0 )
	ROM_LOAD( "ou1shar.18s", 0, 0x200000,	CRC(fbb48194) SHA1(2d3ec5bc519fad2b755018f83fadfe0cba13c292) )

	ROM_REGION( 0x2000000, NAMCONB1_SPRITEGFXREGION, ROMREGION_DISPOSE )
	ROM_LOAD16_BYTE( "ou1obj0l", 0x0000001, 0x200000, CRC(1b4f7184) SHA1(a05d67842fce92f321d1fdd3bd30aa3427775a0c) )
	ROM_LOAD16_BYTE( "ou1obj0u", 0x0000000, 0x200000, CRC(d0a69794) SHA1(07d449e54e9971abeb9cd5bb7b372270fafa8bac) )
	ROM_LOAD16_BYTE( "ou1obj1l", 0x0400001, 0x200000, CRC(48a93e84) SHA1(6935ec161a12237d4cec732d42070f381c23b47c) )
	ROM_LOAD16_BYTE( "ou1obj1u", 0x0400000, 0x200000, CRC(999de386) SHA1(d4780ab1929a3e2c2df464363d6451a2bcecb2a2) )
	ROM_LOAD16_BYTE( "ou1obj2l", 0x0800001, 0x200000, CRC(30386cd0) SHA1(3563c5378288da58136f102381373bd6fcaeec21) )
	ROM_LOAD16_BYTE( "ou1obj2u", 0x0800000, 0x200000, CRC(ccada5f8) SHA1(75ed95bb295780126879d67bba4d0ae1da63c928) )
	ROM_LOAD16_BYTE( "ou1obj3l", 0x0c00001, 0x200000, CRC(5f41b44e) SHA1(3f5376fcd3e15af772df65b8eda4d5ee07ee5664) )
	ROM_LOAD16_BYTE( "ou1obj3u", 0x0c00000, 0x200000, CRC(bc852c8e) SHA1(4863302c45ee16aaf2c36dac07aceaf287959c53) )
	ROM_LOAD16_BYTE( "ou1obj4l", 0x1000001, 0x200000, CRC(99a5f9d7) SHA1(b0f46f4ac357918137031a19c36a56a47b7aefd6) )
	ROM_LOAD16_BYTE( "ou1obj4u", 0x1000000, 0x200000, CRC(70ecaabb) SHA1(521c6849526fb271e6447f6c4f5bfa081f96b91e) )

	ROM_REGION( 0x600000, NAMCONB1_ROTGFXREGION, ROMREGION_DISPOSE )
	ROM_LOAD( "ou1-rot0", 0x000000, 0x200000, CRC(a50c67c8) SHA1(432b8451eb9eaa3078134fce1e5e2d58a8b64be3) )
	ROM_LOAD( "ou1-rot1", 0x200000, 0x200000, CRC(14866780) SHA1(4a54151fada4dfba7232e53e40623e5697eeb7db) )
	ROM_LOAD( "ou1-rot2", 0x400000, 0x200000, CRC(55ccf3af) SHA1(d98489aaa840cbffb21c47609961c1163b0336f3) )

	ROM_REGION( 0x200000, NAMCONB1_TILEGFXREGION, ROMREGION_DISPOSE )
	ROM_LOAD( "ou1-scr0", 0x000000, 0x200000, CRC(b3b3f2e9) SHA1(541bd7e9ba12aff4ec4033bd9c6bb19476acb3c4) )

	ROM_REGION( 0x100000, REGION_USER1, 0 )
	ROM_LOAD( "ou1dat0.20a", 0x00000, 0x80000, CRC(1a49aead) SHA1(df243aff1a6fb5bcf4d5d883c5af2374a4aff477) )
	ROM_LOAD( "ou1dat1.20b", 0x80000, 0x80000, CRC(63bb119d) SHA1(d4c2820243b84c3f5cdf7f9e66bb50f53d0efed2) )
ROM_END


ROM_START( machbrkr )
	ROM_REGION( 0x100000, REGION_CPU1, 0 ) /* main program */
	ROM_LOAD32_WORD( "mb1_mprl.11c", 0x00002, 0x80000, CRC(86cf0644) SHA1(07eeadda1d94c9be2f882edb6f2eb0b98292e500) )
	ROM_LOAD32_WORD( "mb1_mpru.11d", 0x00000, 0x80000, CRC(fb1ff916) SHA1(e0ba96c1f26a60f87d8050e582e164d91e132183) )

	ROM_REGION16_LE( 0x100000, REGION_USER4, 0 ) /* sound data and MCU BIOS */
	ROM_LOAD( "mb1_spr0.5b", 0, 0x80000, CRC(d10f6272) SHA1(cb99e06e050dbf86998ea51ef2ca130b2acfb2f6) )
	NAMCO_C7X_BIOS

	ROM_REGION( 0x1000000, REGION_SOUND1, 0 )
	ROM_LOAD( "mb1_voi0.6n", 0x000000, 0x200000, CRC(d363ca3b) SHA1(71650b66ca3eb00f6ad7d3f1df0f37210b77b942) )
	ROM_RELOAD( 0x400000, 0x200000)
	ROM_LOAD( "mb1_voi1.6p", 0x800000, 0x200000, CRC(7e1c2603) SHA1(533098a54fb897931f1d75be9e69a5c047e4c446) )
	ROM_RELOAD( 0xc00000, 0x200000)

	ROM_REGION( 0x200000, NAMCONB1_TILEMASKREGION, 0 )
	ROM_LOAD( "mb1_shas.12s", 0, 0x100000, CRC(c51c614b) SHA1(519ecad2e4543c05ec35a727f4c875ab006291af) )

	ROM_REGION( 0x200000, NAMCONB1_ROTMASKREGION, 0 )
	ROM_LOAD( "mb1_shar.18s", 0, 0x080000, CRC(d9329b10) SHA1(149c8804c07350f47af36bc7902371f1dfbed272) )

	ROM_REGION( 0x2000000, NAMCONB1_SPRITEGFXREGION, ROMREGION_DISPOSE )
	ROM_LOAD16_BYTE( "mb1obj0l.4c", 0x0000001, 0x200000, CRC(056e6b1c) SHA1(44e49de80c925c8fbe04bf9328a77a50a305a5a7) )
	ROM_LOAD16_BYTE( "mb1obj0u.8c", 0x0000000, 0x200000, CRC(e19b1714) SHA1(ff43bf3c8e8698934c4057c7b4c72db73929e2af) )
	ROM_LOAD16_BYTE( "mb1obj1l.4b", 0x0400001, 0x200000, CRC(af69f7f1) SHA1(414544ec1a9aaffb751beaf63d937ce78d0cf9c6) )
	ROM_LOAD16_BYTE( "mb1obj1u.8b", 0x0400000, 0x200000, CRC(e8ff9082) SHA1(a8c7feb33f6243f1f3bda00deffa695ac2b19171) )
	ROM_LOAD16_BYTE( "mb1obj2l.4a", 0x0800001, 0x200000, CRC(3a5c7379) SHA1(ffe9a229eb04a894e5f3bb8ac2fc4617b5413ac3) )
	ROM_LOAD16_BYTE( "mb1obj2u.8b", 0x0800000, 0x200000, CRC(b59cf5e0) SHA1(eee7511f117a4c1a24e4187e3f30e4d66f914a81) )
	ROM_LOAD16_BYTE( "mb1obj3l.6c", 0x0c00001, 0x200000, CRC(9a765d58) SHA1(2e9ea0f76f80383fcf093e947e1fe161743e33fb) )
	ROM_LOAD16_BYTE( "mb1obj3u.9c", 0x0c00000, 0x200000, CRC(5329c693) SHA1(955b3b8b9813826347a1211f71fa0a294b759ccd) )
	ROM_LOAD16_BYTE( "mb1obj4l.6b", 0x1000001, 0x200000, CRC(a650b05e) SHA1(b247699433c7bf4b6ae990fc06255cfd48a248dd) )
	ROM_LOAD16_BYTE( "mb1obj4u.9b", 0x1000000, 0x200000, CRC(6d0c37e9) SHA1(3a3feb74b890e0a933dcc791e5eee1fb4bdcbb69) )

	ROM_REGION( 0x400000, NAMCONB1_ROTGFXREGION, ROMREGION_DISPOSE )
	ROM_LOAD( "mb1_rot0.3d", 0x000000, 0x200000, CRC(bc353630) SHA1(2bbddda632298899716394ddcfe51412576ca74a) )
	ROM_LOAD( "mb1_rot1.3c", 0x200000, 0x200000, CRC(cf7688cb) SHA1(29a040ce2c4e3bf671cff1a7a1ade06103db236a) )

	ROM_REGION( 0x600000, NAMCONB1_TILEGFXREGION, ROMREGION_DISPOSE )
	ROM_LOAD( "mb1_scr0.1d", 0x000000, 0x200000, CRC(c678d5f3) SHA1(98d1523bef50d444be9485c4e7f6932cccbea191) )
	ROM_LOAD( "mb1_scr1.1c", 0x200000, 0x200000, CRC(fb2b1939) SHA1(bf9d7b93205e7012aa86693f3d2ba8f4d729bc97) )
	ROM_LOAD( "mb1_scr2.1b", 0x400000, 0x200000, CRC(0e6097a5) SHA1(b6c64b3e34ba913138b6b7c3d99d2be4f3ceda08) )

	ROM_REGION( 0x100000, REGION_USER1, 0 )
	ROM_LOAD( "mb1_dat0.20a", 0x00000, 0x80000, CRC(fb2e3cd1) SHA1(019b1d645a07619036522f42e0b9a537f39b6b93) )
ROM_END

/***************************************************************/

INPUT_PORTS_START( gunbulet )
	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_COIN2 )

	PORT_START
	PORT_DIPNAME( 0x01, 0x00, "DSW2 (Unused)" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x01, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x00, "DSW1 (Test)" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x02, DEF_STR( On ) )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_SERVICE1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNUSED ) /* C75 status */

	PORT_START
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_START1 )

	PORT_START
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 ) PORT_PLAYER(2)
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_START2 )

	PORT_START
	PORT_BIT( 0xff, 0x80, IPT_LIGHTGUN_X ) PORT_MINMAX(0,255) PORT_SENSITIVITY(50) PORT_KEYDELTA(4)
	PORT_START
	PORT_BIT( 0xff, 0x80, IPT_LIGHTGUN_Y ) PORT_MINMAX(0,255) PORT_SENSITIVITY(50) PORT_KEYDELTA(4)
	PORT_START
	PORT_BIT( 0xff, 0x80, IPT_LIGHTGUN_X ) PORT_MINMAX(0,255) PORT_SENSITIVITY(50) PORT_KEYDELTA(4) PORT_PLAYER(2)
	PORT_START
	PORT_BIT( 0xff, 0x80, IPT_LIGHTGUN_Y ) PORT_MINMAX(0,255) PORT_SENSITIVITY(50) PORT_KEYDELTA(4) PORT_PLAYER(2)
INPUT_PORTS_END

INPUT_PORTS_START( machbrkr )
	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_COIN2 )

	PORT_START
	PORT_DIPNAME( 0x01, 0x00, "Freeze Screen" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x01, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x00, "DSW1 (Test)" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x02, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x00, "Alternate Test Switch" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x04, DEF_STR( On ) )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_SERVICE1 )
	PORT_DIPNAME( 0x10, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x10, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x20, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x40, DEF_STR( On ) )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNUSED ) /* C75 status */

	PORT_START
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 ) // self test: up
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_BUTTON2 ) // self test: enter
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_BUTTON3 ) // self test: down
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_START1 )

	PORT_START
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 ) PORT_PLAYER(2)
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_BUTTON2 ) PORT_PLAYER(2)
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_BUTTON3 ) PORT_PLAYER(2)
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_START2 )

	PORT_START
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 ) PORT_PLAYER(3)
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_BUTTON2 ) PORT_PLAYER(3)
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_BUTTON3 ) PORT_PLAYER(3)
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_START3 )

	PORT_START
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 ) PORT_PLAYER(4)
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_BUTTON2 ) PORT_PLAYER(4)
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_BUTTON3 ) PORT_PLAYER(4)
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_START4 )
INPUT_PORTS_END

INPUT_PORTS_START( outfxies )
	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_COIN2 )

	PORT_START
	PORT_DIPNAME( 0x01, 0x00, "Freeze Screen" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x01, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x00, "DSW1 (Test)" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x02, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x00, "Alternate Test Switch" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x04, DEF_STR( On ) )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_SERVICE1 )
	PORT_DIPNAME( 0x10, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x10, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x20, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x40, DEF_STR( On ) )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNUSED ) /* C75 status */

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT ) PORT_8WAY
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT ) PORT_8WAY
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN ) PORT_8WAY
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP ) PORT_8WAY
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_BUTTON2 )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_BUTTON3 )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_START1 )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 ) PORT_PLAYER(2)
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_BUTTON2 ) PORT_PLAYER(2)
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_BUTTON3 ) PORT_PLAYER(2)
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_START2 )
INPUT_PORTS_END

INPUT_PORTS_START( namconb1 )
	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_COIN2 )

	PORT_START
	PORT_DIPNAME( 0x01, 0x00, "DSW2 (Unused)" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x01, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x00, "DSW1 (Test)" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x02, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x04, DEF_STR( On ) )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_SERVICE1 )
	PORT_DIPNAME( 0x10, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x10, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x20, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x40, DEF_STR( On ) )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNUSED ) /* C75 status */

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT ) PORT_8WAY
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT ) PORT_8WAY
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN ) PORT_8WAY
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP ) PORT_8WAY
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_BUTTON2 )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_BUTTON3 )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_START1 )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 ) PORT_PLAYER(2)
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_BUTTON2 ) PORT_PLAYER(2)
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_BUTTON3 ) PORT_PLAYER(2)
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_START2 )
INPUT_PORTS_END


GAME( 1994, nebulray, 0,        namconb1, namconb1, nebulray, ROT90, "Namco", "Nebulas Ray (World)", GAME_IMPERFECT_SOUND )
GAME( 1994, nebulryj, nebulray, namconb1, namconb1, nebulray, ROT90, "Namco", "Nebulas Ray (Japan)", GAME_IMPERFECT_SOUND )
GAME( 1994, ptblank,  0,        namconb1, gunbulet, gunbulet, ROT0,  "Namco", "Point Blank", GAME_IMPERFECT_SOUND )
GAME( 1994, gunbulet, ptblank,  namconb1, gunbulet, gunbulet, ROT0,  "Namco", "Gun Bullet (Japan)", GAME_IMPERFECT_SOUND )
GAME( 1993, gslugrsj, 0,        namconb1, namconb1, gslgr94u, ROT0,  "Namco", "Great Sluggers (Japan)", GAME_IMPERFECT_SOUND )
GAME( 1994, gslgr94u, 0,        namconb1, namconb1, gslgr94u, ROT0,  "Namco", "Great Sluggers '94", GAME_IMPERFECT_SOUND )
GAME( 1995, sws95,    0,        namconb1, namconb1, sws95,    ROT0,  "Namco", "Super World Stadium '95 (Japan)", GAME_IMPERFECT_SOUND )
GAME( 1996, sws96,    0,        namconb1, namconb1, sws96,    ROT0,  "Namco", "Super World Stadium '96 (Japan)", GAME_IMPERFECT_SOUND )
GAME( 1997, sws97,    0,        namconb1, namconb1, sws97,    ROT0,  "Namco", "Super World Stadium '97 (Japan)", GAME_IMPERFECT_SOUND )
GAME( 1994, vshoot,   0,        namconb1, namconb1, vshoot,   ROT0,  "Namco", "J-League Soccer V-Shoot", GAME_IMPERFECT_SOUND )

/*     YEAR, NAME,     PARENT,   MACHINE,  INPUT,    INIT,     MNTR,  COMPANY, FULLNAME,   FLAGS */
GAME( 1994, outfxies, 0,	 namconb2, outfxies, outfxies, ROT0, "Namco", "Outfoxies", GAME_IMPERFECT_SOUND )
GAME( 1994, outfxesj, outfxies, namconb2, outfxies, outfxies, ROT0, "Namco", "Outfoxies (Japan)", GAME_IMPERFECT_SOUND )
GAME( 1995, machbrkr, 0,	 namconb2, namconb1, machbrkr, ROT0, "Namco", "Mach Breakers (Japan)", GAME_IMPERFECT_SOUND )
