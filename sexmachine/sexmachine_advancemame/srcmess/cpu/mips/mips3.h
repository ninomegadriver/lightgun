/***************************************************************************

    mips3.h
    Interface file for the portable MIPS III/IV emulator.
    Written by Aaron Giles

***************************************************************************/

#ifndef _MIPS3_H
#define _MIPS3_H

#include "cpuintrf.h"


/***************************************************************************
    REGISTER ENUMERATION
***************************************************************************/

enum
{
	MIPS3_PC=1,MIPS3_SR,
	MIPS3_R0,
	MIPS3_R1,
	MIPS3_R2,
	MIPS3_R3,
	MIPS3_R4,
	MIPS3_R5,
	MIPS3_R6,
	MIPS3_R7,
	MIPS3_R8,
	MIPS3_R9,
	MIPS3_R10,
	MIPS3_R11,
	MIPS3_R12,
	MIPS3_R13,
	MIPS3_R14,
	MIPS3_R15,
	MIPS3_R16,
	MIPS3_R17,
	MIPS3_R18,
	MIPS3_R19,
	MIPS3_R20,
	MIPS3_R21,
	MIPS3_R22,
	MIPS3_R23,
	MIPS3_R24,
	MIPS3_R25,
	MIPS3_R26,
	MIPS3_R27,
	MIPS3_R28,
	MIPS3_R29,
	MIPS3_R30,
	MIPS3_R31,
	MIPS3_HI,
	MIPS3_LO,
	MIPS3_EPC,   MIPS3_CAUSE,
	MIPS3_COUNT, MIPS3_COMPARE,
	MIPS3_INDEX, MIPS3_RANDOM,
	MIPS3_ENTRYHI,
	MIPS3_ENTRYLO0, MIPS3_ENTRYLO1,
	MIPS3_PAGEMASK,
	MIPS3_WIRED, MIPS3_BADVADDR
};

#define MIPS3_MAX_FASTRAM		4
#define MIPS3_MAX_HOTSPOTS		16

enum
{
	CPUINFO_INT_MIPS3_DRC_OPTIONS = CPUINFO_INT_CPU_SPECIFIC,

	CPUINFO_INT_MIPS3_FASTRAM_SELECT,
	CPUINFO_INT_MIPS3_FASTRAM_START,
	CPUINFO_INT_MIPS3_FASTRAM_END,
	CPUINFO_INT_MIPS3_FASTRAM_READONLY,

	CPUINFO_INT_MIPS3_HOTSPOT_SELECT,
	CPUINFO_INT_MIPS3_HOTSPOT_PC,
	CPUINFO_INT_MIPS3_HOTSPOT_OPCODE,
	CPUINFO_INT_MIPS3_HOTSPOT_CYCLES,

	CPUINFO_PTR_MIPS3_FASTRAM_BASE = CPUINFO_PTR_CPU_SPECIFIC
};


/***************************************************************************
    INTERRUPT CONSTANTS
***************************************************************************/

#define MIPS3_IRQ0		0		/* IRQ0 */
#define MIPS3_IRQ1		1		/* IRQ1 */
#define MIPS3_IRQ2		2		/* IRQ2 */
#define MIPS3_IRQ3		3		/* IRQ3 */
#define MIPS3_IRQ4		4		/* IRQ4 */
#define MIPS3_IRQ5		5		/* IRQ5 */


/***************************************************************************
    STRUCTURES
***************************************************************************/

struct mips3_config
{
	size_t		icache;							/* code cache size */
	size_t		dcache;							/* data cache size */
	UINT32		system_clock;					/* system clock rate */
};


/***************************************************************************
    PUBLIC FUNCTIONS
***************************************************************************/

#if (HAS_R4600)
void r4600be_get_info(UINT32 state, union cpuinfo *info);
void r4600le_get_info(UINT32 state, union cpuinfo *info);
#endif

#if (HAS_R4700)
void r4700be_get_info(UINT32 state, union cpuinfo *info);
void r4700le_get_info(UINT32 state, union cpuinfo *info);
#endif

#if (HAS_R5000)
void r5000be_get_info(UINT32 state, union cpuinfo *info);
void r5000le_get_info(UINT32 state, union cpuinfo *info);
#endif

#if (HAS_QED5271)
void qed5271be_get_info(UINT32 state, union cpuinfo *info);
void qed5271le_get_info(UINT32 state, union cpuinfo *info);
#endif

#if (HAS_RM7000)
void rm7000be_get_info(UINT32 state, union cpuinfo *info);
void rm7000le_get_info(UINT32 state, union cpuinfo *info);
#endif


/***************************************************************************
    COMPILER-SPECIFIC OPTIONS
***************************************************************************/

/* fix me -- how do we make this work?? */
#define MIPS3DRC_STRICT_VERIFY		0x0001			/* verify all instructions */
#define MIPS3DRC_STRICT_COP0		0x0002			/* validate all COP0 instructions */
#define MIPS3DRC_STRICT_COP1		0x0004			/* validate all COP1 instructions */
#define MIPS3DRC_STRICT_COP2		0x0008			/* validate all COP2 instructions */
#define MIPS3DRC_DIRECT_RAM			0x0010			/* allow direct RAM access (no bankswitching!) */
#define MIPS3DRC_FLUSH_PC			0x0020			/* flush the PC value before each memory access */
#define MIPS3DRC_CHECK_OVERFLOWS	0x0040			/* actually check overflows on add/sub instructions */

#define MIPS3DRC_COMPATIBLE_OPTIONS	(MIPS3DRC_STRICT_VERIFY | MIPS3DRC_STRICT_COP0 | MIPS3DRC_STRICT_COP1 | MIPS3DRC_STRICT_COP2 | MIPS3DRC_FLUSH_PC)
#define MIPS3DRC_FASTEST_OPTIONS	(MIPS3DRC_DIRECT_RAM)



#endif /* _MIPS3_H */
