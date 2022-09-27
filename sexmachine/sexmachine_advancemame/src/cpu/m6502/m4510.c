/*****************************************************************************
 *
 *   m4510.c
 *   Portable 4510 emulator V1.0beta1
 *
 *   Copyright (c) 2000 Peter Trauner, all rights reserved
 *   documentation preliminary databook
 *   documentation by michael steil mist@c64.org
 *   available at ftp://ftp.funet.fi/pub/cbm/c65
 *
 *   - This source code is released as freeware for non-commercial purposes.
 *   - You are free to use and redistribute this code in modified or
 *     unmodified form, provided you list me in the credits.
 *   - If you modify this source code, you must add a notice to each modified
 *     source file that it has been changed.  If you're a nice person, you
 *     will clearly mark each change too.  :)
 *   - If you wish to use this for commercial purposes, please contact me at
 *     pullmoll@t-online.de
 *   - The author of this copywritten work reserves the right to change the
 *     terms of its usage and license at any time, including retroactively
 *   - This entire notice must remain in the source code.
 *
 *****************************************************************************/

/*
   c65 memory management
   (reset)
   c64 ff
   c65 20 (interface)

a   (c65 mode)
   a:00 x:e3 y:00 z:b3
   c65 64 (interface)
   c64 ff

b   (c65 dosmode?)
   c65 65 (interface, full colorram)
   a:00 x:11 y:80 z:31
   c64 ff

c   (?)
   c64 07
   a:00 x:00 y:00 z:00

   a c65 mode

   diskcontroller accesses


   monitor
   c64 ff
   a:a0 x:82 y:00 z:83

   c64 mode
   c65 0
   c65 2f:0 !
   c64 ff
   a:00 x:00 y:00 z:00

internal 8 mb to 64k switching (jmp routine in rom)
( seams to be incomplete, in chapter 1 1megabyte memory mapper )
         a  x  y  z
g      0 00 e0 00 f0
g  10000 00 e1 00 f1
g  20000 00 e2 00 f2
g  30000 00 e3 00 f3
g  40000 00 e4 00 f4
g  50000 00 e5 00 f5
g  60000 00 e6 00 f6
.
.
g  f0000 00 ef 00 ff
the same for 100000 .. 700000
g 800000 00 e3 00 b3

thesis:
a: ?0?0 0000
   ? ?       only in monitor mode set
x:      xxxx address bits a19 .. a16 for memory accesses with a15 0 ?
   0000      c64 mode
   0001      dosmode
   1110      c65 mode, plain ram access
             (0000-1fff contains the switching code, so not switchable!?)
   1000      monitor
   1         map 6000-7fff
    1        map 4000-5fff
     1       map 2000-3fff
      1      map 0000-1fff
y: ?000 0000
   ?         only in dos mode set
z:      xxxx address bits a19 .. a16 for memory accesses with a15 1 ?
   0000      c64 mode
   0011      dosmode
   1000      monitor
   1011      c65 mode
   1111      plain ram access
   1         map e000-ffff
    1        map c000-dfff
     1       map a000-bfff
      1      map 8000-9fff
 */

#include "debugger.h"
#include "m6502.h"
#include "m4510.h"

#include "ops02.h"
#include "opsc02.h"
#include "opsce02.h"
#include "ops4510.h"

#define M6502_NMI_VEC	0xfffa
#define M6502_RST_VEC	0xfffc
#define M6502_IRQ_VEC	0xfffe
#define M4510_RST_VEC	M6502_RST_VEC
#define M4510_IRQ_VEC	M6502_IRQ_VEC
#define M4510_NMI_VEC	M6502_NMI_VEC

#define VERBOSE 0

#if VERBOSE
#define LOG(x)	logerror x
#else
#define LOG(x)
#endif


/* Layout of the registers in the debugger */
static UINT8 m4510_reg_layout[] = {
	M4510_A,M4510_X,M4510_Y,M4510_Z,M4510_S,M4510_PC,
	M4510_MEM_LOW,
	-1,
	M4510_EA,M4510_ZP, M4510_B,
	M4510_P,
	M4510_MEM_HIGH,
	0
};

/* Layout of the debugger windows x,y,w,h */
static UINT8 m4510_win_layout[] = {
	25, 0,55, 2,	/* register window (top, right rows) */
	 0, 0,24,22,	/* disassembler window (left colums) */
	25, 3,55, 9,	/* memory #1 window (right, upper middle) */
	25,13,55, 9,	/* memory #2 window (right, lower middle) */
	 0,23,80, 1,	/* command line window (bottom rows) */
};

typedef struct {
	void	(**insn)(void); /* pointer to the function pointer table */
	PAIR	ppc;			/* previous program counter */
	PAIR	pc; 			/* program counter */
	PAIR	sp; 			/* stack pointer (always 100 - 1FF) */
	PAIR	zp; 			/* zero page address */
	/* contains B register zp.b.h */
	PAIR	ea; 			/* effective address */
	UINT8	a;				/* Accumulator */
	UINT8	x;				/* X index register */
	UINT8	y;				/* Y index register */
	UINT8	z;				/* Z index register */
	UINT8	p;				/* Processor status */
	UINT8	pending_irq;	/* nonzero if an IRQ is pending */
	UINT8	after_cli;		/* pending IRQ and last insn cleared I */
	UINT8	nmi_state;
	UINT8	irq_state;
	UINT16  low, high;
	UINT32	mem[8];
	int 	(*irq_callback)(int irqline);	/* IRQ callback */
	read8_handler rdmem_id;					/* readmem callback for indexed instructions */
	write8_handler wrmem_id;				/* writemem callback for indexed instructions */

	UINT8    ddr;
	UINT8    port;
	UINT8 (*port_read)(void);
	void (*port_write)(UINT8 data);
}	m4510_Regs;

static int m4510_ICount = 0;

static m4510_Regs m4510;

/***************************************************************
 * include the opcode macros, functions and tables
 ***************************************************************/

INLINE int m4510_cpu_readop(void)
{
	register UINT16 t=m4510.pc.w.l++;
	return cpu_readop(M4510_MEM(t));
}

INLINE int m4510_cpu_readop_arg(void)
{
	register UINT16 t=m4510.pc.w.l++;
	return cpu_readop_arg(M4510_MEM(t));
}

#define M4510
#include "t65ce02.c"

static void m4510_init(int index, int clock, const void *config, int (*irqcallback)(int))
{
	m4510.rdmem_id = program_read_byte_8;
	m4510.wrmem_id = program_write_byte_8;
	m4510.irq_callback = irqcallback;
}

static void m4510_reset (void)
{
	m4510.insn = insn4510;

	/* wipe out the rest of the m65ce02 structure */
	/* read the reset vector into PC */
	/* reset z index and b bank */
	PCL = RDMEM(M4510_RST_VEC);
	PCH = RDMEM(M4510_RST_VEC+1);

	/* after reset in 6502 compatibility mode */
	m4510.sp.d = 0x01ff; /* high byte descriped in databook */
	m4510.z = 0;
	B = 0;
	m4510.p = F_E|F_B|F_I|F_Z;	/* set E, I and Z flags */
	m4510.pending_irq = 0;	/* nonzero if an IRQ is pending */
	m4510.after_cli = 0;		/* pending IRQ and last insn cleared I */
	m4510.irq_callback = NULL;

	/* don't know */
	m4510.high=0x8200;
	m4510.mem[7]=0x20000;

	CHANGE_PC;

	m4510.port = 0xff;
	m4510.ddr = 0x00;
}

static void m4510_exit(void)
{
	/* nothing to do yet */
}

static void m4510_get_context (void *dst)
{
	if( dst )
		*(m4510_Regs*)dst = m4510;
}

static void m4510_set_context (void *src)
{
	if( src )
	{
		m4510 = *(m4510_Regs*)src;
		CHANGE_PC;
	}
}


INLINE void m4510_take_irq(void)
{
	if( !(P & F_I) )
	{
		EAD = M4510_IRQ_VEC;
		m4510_ICount -= 7;
		PUSH(PCH);
		PUSH(PCL);
		PUSH(P & ~F_B);
		P = (P & ~F_D) | F_I;		/* knock out D and set I flag */
		PCL = RDMEM(EAD);
		PCH = RDMEM(EAD+1);
		LOG((errorlog,"M4510#%d takes IRQ ($%04x)\n", cpu_getactivecpu(), PCD));
		/* call back the cpuintrf to let it clear the line */
		if (m4510.irq_callback) (*m4510.irq_callback)(0);
		CHANGE_PC;
	}
	m4510.pending_irq = 0;
}

static int m4510_execute(int cycles)
{
	m4510_ICount = cycles;

	CHANGE_PC;

	do
	{
		UINT8 op;
		PPC = PCD;

		CALL_MAME_DEBUG;

		/* if an irq is pending, take it now */
		if( m4510.pending_irq )
			m4510_take_irq();

		op = RDOP();
		(*insn4510[op])();

		/* check if the I flag was just reset (interrupts enabled) */
		if( m4510.after_cli )
		{
			LOG((errorlog,"M4510#%d after_cli was >0", cpu_getactivecpu()));
			m4510.after_cli = 0;
			if (m4510.irq_state != CLEAR_LINE)
			{
				LOG((errorlog,": irq line is asserted: set pending IRQ\n"));
				m4510.pending_irq = 1;
			}
			else
			{
				LOG((errorlog,": irq line is clear\n"));
			}
		}
		else
		if( m4510.pending_irq )
			m4510_take_irq();

	} while (m4510_ICount > 0);

	return cycles - m4510_ICount;
}

static void m4510_set_irq_line(int irqline, int state)
{
	if (irqline == INPUT_LINE_NMI)
	{
		if (m4510.nmi_state == state) return;
		m4510.nmi_state = state;
		if( state != CLEAR_LINE )
		{
			LOG((errorlog, "M4510#%d set_nmi_line(ASSERT)\n", cpu_getactivecpu()));
			EAD = M4510_NMI_VEC;
			m4510_ICount -= 7;
			PUSH(PCH);
			PUSH(PCL);
			PUSH(P & ~F_B);
			P = (P & ~F_D) | F_I;		/* knock out D and set I flag */
			PCL = RDMEM(EAD);
			PCH = RDMEM(EAD+1);
			LOG((errorlog,"M4510#%d takes NMI ($%04x)\n", cpu_getactivecpu(), PCD));
			CHANGE_PC;
		}
	}
	else
	{
		m4510.irq_state = state;
		if( state != CLEAR_LINE )
		{
			LOG((errorlog, "M4510#%d set_irq_line(ASSERT)\n", cpu_getactivecpu()));
			m4510.pending_irq = 1;
		}
	}
}

static UINT8 m4510_get_port(void)
{
	return (m4510.port & m4510.ddr) | (m4510.ddr ^ 0xff);
}

static READ8_HANDLER( m4510_read_0000 )
{
	UINT8 result = 0x00;

	switch(offset)
	{
		case 0x0000:	/* DDR */
			result = m4510.ddr;
			break;
		case 0x0001:	/* Data Port */
			if (m4510.port_read)
				result = m4510.port_read();
			result = (m4510.ddr & m4510.port) | (~m4510.ddr & result);
			break;
	}
	return result;
}

static WRITE8_HANDLER( m4510_write_0000 )
{
	switch(offset)
	{
		case 0x0000:	/* DDR */
			m4510.ddr = data;
			break;
		case 0x0001:	/* Data Port */
			m4510.port = data;
			break;
	}

	if (m4510.port_write)
		m4510.port_write(m4510_get_port());
}

static ADDRESS_MAP_START(m4510_mem, ADDRESS_SPACE_PROGRAM, 8)
	AM_RANGE(0x0000, 0x0001) AM_READWRITE(m4510_read_0000, m4510_write_0000)
ADDRESS_MAP_END

static int m4510_translate(int space, offs_t *addr)
{
	if (space == ADDRESS_SPACE_PROGRAM)
		*addr = M4510_MEM(*addr);
	return 1;
}

/**************************************************************************
 * Generic set_info
 **************************************************************************/

static void m4510_set_info(UINT32 state, union cpuinfo *info)
{
	switch (state)
	{
		/* --- the following bits of info are set as 64-bit signed integers --- */
		case CPUINFO_INT_INPUT_STATE + M4510_IRQ_LINE:	m4510_set_irq_line(M4510_IRQ_LINE, info->i); break;
		case CPUINFO_INT_INPUT_STATE + INPUT_LINE_NMI:	m4510_set_irq_line(INPUT_LINE_NMI, info->i); break;

		case CPUINFO_INT_PC:							PCW = info->i; change_pc(PCD);			break;
		case CPUINFO_INT_REGISTER + M4510_PC:			m4510.pc.w.l = info->i;					break;
		case CPUINFO_INT_SP:							S = info->i;							break;
		case CPUINFO_INT_REGISTER + M4510_S:			m4510.sp.b.l = info->i;					break;
		case CPUINFO_INT_REGISTER + M4510_P:			m4510.p = info->i;						break;
		case CPUINFO_INT_REGISTER + M4510_A:			m4510.a = info->i;						break;
		case CPUINFO_INT_REGISTER + M4510_X:			m4510.x = info->i;						break;
		case CPUINFO_INT_REGISTER + M4510_Y:			m4510.y = info->i;						break;
		case CPUINFO_INT_REGISTER + M4510_Z:			m4510.z = info->i;						break;
		case CPUINFO_INT_REGISTER + M4510_B:			m4510.zp.b.h = info->i;					break;
		case CPUINFO_INT_REGISTER + M4510_MEM_LOW:		m4510.low = info->i;					break;
		case CPUINFO_INT_REGISTER + M4510_MEM_HIGH:		m4510.high = info->i;					break;
		case CPUINFO_INT_REGISTER + M4510_EA:			m4510.ea.w.l = info->i;					break;
		case CPUINFO_INT_REGISTER + M4510_ZP:			m4510.zp.w.l = info->i;					break;
		case CPUINFO_INT_REGISTER + M4510_MEM0:			m4510.mem[0] = info->i;					break;
		case CPUINFO_INT_REGISTER + M4510_MEM1:			m4510.mem[1] = info->i;					break;
		case CPUINFO_INT_REGISTER + M4510_MEM2:			m4510.mem[2] = info->i;					break;
		case CPUINFO_INT_REGISTER + M4510_MEM3:			m4510.mem[3] = info->i;					break;
		case CPUINFO_INT_REGISTER + M4510_MEM4:			m4510.mem[4] = info->i;					break;
		case CPUINFO_INT_REGISTER + M4510_MEM5:			m4510.mem[5] = info->i;					break;
		case CPUINFO_INT_REGISTER + M4510_MEM6:			m4510.mem[6] = info->i;					break;
		case CPUINFO_INT_REGISTER + M4510_MEM7:			m4510.mem[7] = info->i;					break;

		/* --- the following bits of info are set as pointers to data or functions --- */
		case CPUINFO_PTR_M6502_READINDEXED_CALLBACK:	m4510.rdmem_id = (read8_handler) info->f;	break;
		case CPUINFO_PTR_M6502_WRITEINDEXED_CALLBACK:	m4510.wrmem_id = (write8_handler) info->f;	break;
		case CPUINFO_PTR_M6510_PORTREAD:	m4510.port_read = (UINT8 (*)(void)) info->f;	break;
		case CPUINFO_PTR_M6510_PORTWRITE:	m4510.port_write = (void (*)(UINT8)) info->f;	break;
	}
}



/**************************************************************************
 * Generic get_info
 **************************************************************************/

void m4510_get_info(UINT32 state, union cpuinfo *info)
{
	switch (state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case CPUINFO_INT_CONTEXT_SIZE:					info->i = sizeof(m4510);				break;
		case CPUINFO_INT_INPUT_LINES:					info->i = 2;							break;
		case CPUINFO_INT_DEFAULT_IRQ_VECTOR:			info->i = 0;							break;
		case CPUINFO_INT_ENDIANNESS:					info->i = CPU_IS_LE;					break;
		case CPUINFO_INT_CLOCK_DIVIDER:					info->i = 1;							break;
		case CPUINFO_INT_MIN_INSTRUCTION_BYTES:			info->i = 1;							break;
		case CPUINFO_INT_MAX_INSTRUCTION_BYTES:			info->i = 3;							break;
		case CPUINFO_INT_MIN_CYCLES:					info->i = 1;							break;
		case CPUINFO_INT_MAX_CYCLES:					info->i = 10;							break;

		case CPUINFO_INT_DATABUS_WIDTH + ADDRESS_SPACE_PROGRAM:	info->i = 8;					break;
		case CPUINFO_INT_ADDRBUS_WIDTH + ADDRESS_SPACE_PROGRAM: info->i = 20;					break;
		case CPUINFO_INT_ADDRBUS_SHIFT + ADDRESS_SPACE_PROGRAM: info->i = 0;					break;
		case CPUINFO_INT_LOGADDR_WIDTH + ADDRESS_SPACE_PROGRAM: info->i = 16;					break;
		case CPUINFO_INT_PAGE_SHIFT + ADDRESS_SPACE_PROGRAM: 	info->i = 13;					break;
		case CPUINFO_INT_DATABUS_WIDTH + ADDRESS_SPACE_DATA:	info->i = 0;					break;
		case CPUINFO_INT_ADDRBUS_WIDTH + ADDRESS_SPACE_DATA: 	info->i = 0;					break;
		case CPUINFO_INT_ADDRBUS_SHIFT + ADDRESS_SPACE_DATA: 	info->i = 0;					break;
		case CPUINFO_INT_DATABUS_WIDTH + ADDRESS_SPACE_IO:		info->i = 0;					break;
		case CPUINFO_INT_ADDRBUS_WIDTH + ADDRESS_SPACE_IO: 		info->i = 0;					break;
		case CPUINFO_INT_ADDRBUS_SHIFT + ADDRESS_SPACE_IO: 		info->i = 0;					break;

		case CPUINFO_INT_INPUT_STATE + M4510_IRQ_LINE:	info->i = m4510.irq_state;				break;
		case CPUINFO_INT_INPUT_STATE + INPUT_LINE_NMI:	info->i = m4510.nmi_state;				break;

		case CPUINFO_INT_PREVIOUSPC:					info->i = m4510.ppc.w.l;				break;

		case CPUINFO_INT_PC:							info->i = PCD;							break;
		case CPUINFO_INT_REGISTER + M4510_PC:			info->i = m4510.pc.w.l;					break;
		case CPUINFO_INT_SP:							info->i = S;							break;
		case CPUINFO_INT_REGISTER + M4510_S:			info->i = m4510.sp.b.l;					break;
		case CPUINFO_INT_REGISTER + M4510_P:			info->i = m4510.p;						break;
		case CPUINFO_INT_REGISTER + M4510_A:			info->i = m4510.a;						break;
		case CPUINFO_INT_REGISTER + M4510_X:			info->i = m4510.x;						break;
		case CPUINFO_INT_REGISTER + M4510_Y:			info->i = m4510.y;						break;
		case CPUINFO_INT_REGISTER + M4510_Z:			info->i = m4510.z;						break;
		case CPUINFO_INT_REGISTER + M4510_B:			info->i = m4510.zp.b.h;					break;
		case CPUINFO_INT_REGISTER + M4510_MEM_LOW:		info->i = m4510.low;					break;
		case CPUINFO_INT_REGISTER + M4510_MEM_HIGH:		info->i = m4510.high;					break;
		case CPUINFO_INT_REGISTER + M4510_EA:			info->i = m4510.ea.w.l;					break;
		case CPUINFO_INT_REGISTER + M4510_ZP:			info->i = m4510.zp.w.l;					break;
		case CPUINFO_INT_REGISTER + M4510_MEM0:			info->i = m4510.mem[0];					break;
		case CPUINFO_INT_REGISTER + M4510_MEM1:			info->i = m4510.mem[1];					break;
		case CPUINFO_INT_REGISTER + M4510_MEM2:			info->i = m4510.mem[2];					break;
		case CPUINFO_INT_REGISTER + M4510_MEM3:			info->i = m4510.mem[3];					break;
		case CPUINFO_INT_REGISTER + M4510_MEM4:			info->i = m4510.mem[4];					break;
		case CPUINFO_INT_REGISTER + M4510_MEM5:			info->i = m4510.mem[5];					break;
		case CPUINFO_INT_REGISTER + M4510_MEM6:			info->i = m4510.mem[6];					break;
		case CPUINFO_INT_REGISTER + M4510_MEM7:			info->i = m4510.mem[7];					break;
		case CPUINFO_INT_M6510_PORT:					info->i = m4510_get_port();	break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case CPUINFO_PTR_SET_INFO:						info->setinfo = m4510_set_info;			break;
		case CPUINFO_PTR_GET_CONTEXT:					info->getcontext = m4510_get_context;	break;
		case CPUINFO_PTR_SET_CONTEXT:					info->setcontext = m4510_set_context;	break;
		case CPUINFO_PTR_INIT:							info->init = m4510_init;				break;
		case CPUINFO_PTR_RESET:							info->reset = m4510_reset;				break;
		case CPUINFO_PTR_EXIT:							info->exit = m4510_exit;				break;
		case CPUINFO_PTR_EXECUTE:						info->execute = m4510_execute;			break;
		case CPUINFO_PTR_BURN:							info->burn = NULL;						break;
#ifdef MAME_DEBUG
		case CPUINFO_PTR_DISASSEMBLE_NEW:				info->disassemble_new = m4510_dasm;			break;
#endif
		case CPUINFO_PTR_INSTRUCTION_COUNTER:			info->icount = &m4510_ICount;			break;
		case CPUINFO_PTR_REGISTER_LAYOUT:				info->p = m4510_reg_layout;				break;
		case CPUINFO_PTR_WINDOW_LAYOUT:					info->p = m4510_win_layout;				break;
		case CPUINFO_PTR_INTERNAL_MEMORY_MAP:			info->internal_map = construct_map_m4510_mem; break;
		case CPUINFO_PTR_TRANSLATE:						info->translate = m4510_translate;	break;
		case CPUINFO_PTR_M6502_READINDEXED_CALLBACK:	info->f = (genf *) m4510.rdmem_id;		break;
		case CPUINFO_PTR_M6502_WRITEINDEXED_CALLBACK:	info->f = (genf *) m4510.wrmem_id;		break;
		case CPUINFO_PTR_M6510_PORTREAD:				info->f = (genf *) m4510.port_read;		break;
		case CPUINFO_PTR_M6510_PORTWRITE:				info->f = (genf *) m4510.port_write;		break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case CPUINFO_STR_NAME:							strcpy(info->s = cpuintrf_temp_str(), "M4510"); break;
		case CPUINFO_STR_CORE_FAMILY:					strcpy(info->s = cpuintrf_temp_str(), "CBM Semiconductor Group CSG 65CE02"); break;
		case CPUINFO_STR_CORE_VERSION:					strcpy(info->s = cpuintrf_temp_str(), "1.0beta"); break;
		case CPUINFO_STR_CORE_FILE:						strcpy(info->s = cpuintrf_temp_str(), __FILE__); break;
		case CPUINFO_STR_CORE_CREDITS:					strcpy(info->s = cpuintrf_temp_str(), "Copyright (c) 1998 Juergen Buchmueller\nCopyright (c) 2000 Peter Trauner\nall rights reserved."); break;

		case CPUINFO_STR_FLAGS:
			sprintf(info->s = cpuintrf_temp_str(), "%c%c%c%c%c%c%c%c",
				m4510.p & 0x80 ? 'N':'.',
				m4510.p & 0x40 ? 'V':'.',
				m4510.p & 0x20 ? 'R':'.',
				m4510.p & 0x10 ? 'B':'.',
				m4510.p & 0x08 ? 'D':'.',
				m4510.p & 0x04 ? 'I':'.',
				m4510.p & 0x02 ? 'Z':'.',
				m4510.p & 0x01 ? 'C':'.');
			break;

		case CPUINFO_STR_REGISTER + M4510_PC:			sprintf(info->s = cpuintrf_temp_str(), "PC:%04X", m4510.pc.w.l); break;
		case CPUINFO_STR_REGISTER + M4510_S:			sprintf(info->s = cpuintrf_temp_str(), "S:%02X", m4510.sp.b.l); break;
		case CPUINFO_STR_REGISTER + M4510_P:			sprintf(info->s = cpuintrf_temp_str(), "P:%02X", m4510.p); break;
		case CPUINFO_STR_REGISTER + M4510_A:			sprintf(info->s = cpuintrf_temp_str(), "A:%02X", m4510.a); break;
		case CPUINFO_STR_REGISTER + M4510_X:			sprintf(info->s = cpuintrf_temp_str(), "X:%02X", m4510.x); break;
		case CPUINFO_STR_REGISTER + M4510_Y:			sprintf(info->s = cpuintrf_temp_str(), "Y:%02X", m4510.y); break;
		case CPUINFO_STR_REGISTER + M4510_Z:			sprintf(info->s = cpuintrf_temp_str(), "Z:%02X", m4510.z); break;
		case CPUINFO_STR_REGISTER + M4510_B:			sprintf(info->s = cpuintrf_temp_str(), "B:%02X", m4510.zp.b.h); break;
		case CPUINFO_STR_REGISTER + M4510_MEM_LOW:		sprintf(info->s = cpuintrf_temp_str(), "0:%01X", m4510.low); break;
		case CPUINFO_STR_REGISTER + M4510_MEM_HIGH:		sprintf(info->s = cpuintrf_temp_str(), "1:%01X", m4510.high); break;
		case CPUINFO_STR_REGISTER + M4510_EA:			sprintf(info->s = cpuintrf_temp_str(), "EA:%04X", m4510.ea.w.l); break;
		case CPUINFO_STR_REGISTER + M4510_ZP:			sprintf(info->s = cpuintrf_temp_str(), "ZP:%03X", m4510.zp.w.l); break;
	}
}

