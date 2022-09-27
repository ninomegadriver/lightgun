/****************************************************************************
*             real mode i286 emulator v1.4 by Fabrice Frances               *
*               (initial work based on David Hedley's pcemu)                *
****************************************************************************/

#include "host.h"
#include "debugger.h"


/* All post-i286 CPUs have a 16MB address space */
#define AMASK	I.amask


#define i86_ICount i286_ICount

#include "i286.h"
#include "i286intf.h"


static UINT8 i286_reg_layout[] =
{
	I286_FLAGS,
	I286_MSW,
	I286_TR,
	I286_TR_2,
	I286_GDTR,
	I286_GDTR_2,
	-1,
	I286_AX,
	I286_BP,
	I286_LDTR,
	I286_LDTR_2,
	I286_IDTR,
	I286_IDTR_2,
	-1,
	I286_BX,
	I286_SP,
	I286_SS,
	I286_SS_2,
	I286_VECTOR,
	-1,
	I286_CX,
	I286_IP,
	I286_CS,
	I286_CS_2,
	-1,
	I286_DX,
	I286_SI,
	I286_DS,
	I286_DS_2,
	-1,
	I286_EMPTY,
	I286_DI,
	I286_ES,
	I286_ES_2,
	0
};

/* Layout of the debugger windows x,y,w,h */
static UINT8 i286_win_layout[] = {
     0, 0,80, 6,    /* register window (top rows) */
	 0, 7,40,15,	/* disassembler window (left colums) */
	41, 7,39, 7,	/* memory #1 window (right, upper middle) */
	41,15,39, 7,	/* memory #2 window (right, lower middle) */
     0,23,80, 1,    /* command line window (bottom rows) */
};


#include "i86time.c"

/***************************************************************************/
/* cpu state                                                               */
/***************************************************************************/
/* I86 registers */
typedef union
{                   /* eight general registers */
    UINT16 w[8];    /* viewed as 16 bits registers */
    UINT8  b[16];   /* or as 8 bit registers */
} i286basicregs;

typedef struct
{
    i286basicregs regs;
	UINT32 	amask;			/* address mask */
    UINT32  pc;
    UINT32  prevpc;
	UINT16	flags;
	UINT16	msw;
	UINT32	base[4];
	UINT16	sregs[4];
	UINT16	limit[4];
	UINT8 rights[4];
	struct {
		UINT32 base;
		UINT16 limit;
	} gdtr, idtr;
	struct {
		UINT16 sel;
		UINT32 base;
		UINT16 limit;
		UINT8 rights;
	} ldtr, tr;
    int     (*irq_callback)(int irqline);
    INT32     AuxVal, OverVal, SignVal, ZeroVal, CarryVal, DirVal; /* 0 or non-0 valued flags */
    UINT8	ParityVal;
	UINT8	TF, IF; 	/* 0 or 1 valued flags */
	UINT8	int_vector;
	INT8	nmi_state;
	INT8	irq_state;
	INT8	test_state;		/* PJB 03/05 */
	INT32 	extra_cycles;       /* extra cycles for interrupts */
} i286_Regs;

int i286_ICount;

static i286_Regs I;
static unsigned prefix_base;	/* base address of the latest prefix segment */
static char seg_prefix;         /* prefix segment indicator */

#define INT_IRQ 0x01
#define NMI_IRQ 0x02

static UINT8 parity_table[256];

static struct i86_timing cycles;

/***************************************************************************/

#define I286
#define PREFIX(fname) i286##fname
#define PREFIX86(fname) i286##fname
#define PREFIX186(fname) i286##fname
#define PREFIX286(fname) i286##fname

#include "ea.h"
#include "modrm.h"
#include "instr86.h"
#include "instr186.h"
#include "instr286.h"
#include "table286.h"
#include "instr86.c"
#include "instr186.c"
#include "instr286.c"

static void i286_urinit(void)
{
	unsigned int i,j,c;
	static const BREGS reg_name[8]={ AL, CL, DL, BL, AH, CH, DH, BH };

	for (i = 0;i < 256; i++)
	{
		for (j = i, c = 0; j > 0; j >>= 1)
			if (j & 1) c++;

		parity_table[i] = !(c & 1);
	}

	for (i = 0; i < 256; i++)
	{
		Mod_RM.reg.b[i] = reg_name[(i & 0x38) >> 3];
		Mod_RM.reg.w[i] = (WREGS) ( (i & 0x38) >> 3) ;
	}

	for (i = 0xc0; i < 0x100; i++)
	{
		Mod_RM.RM.w[i] = (WREGS)( i & 7 );
		Mod_RM.RM.b[i] = (BREGS)reg_name[i & 7];
	}
}

static void i286_set_a20_line(int state)
{
	I.amask = state ? 0x00ffffff : 0x000fffff;
}

static void i286_reset (void)
{
	static int urinit=1;

	/* in my docu not all registers are initialized! */
	if (urinit) {
		i286_urinit();
		urinit=0;

	}

	I.sregs[CS] = 0xf000;
	I.base[CS] = 0xff0000;
	/* temporary, until I have the right reset vector working */
	I.base[CS] = I.sregs[CS] << 4;
	I.pc = 0xffff0;
	I.limit[CS]=I.limit[SS]=I.limit[DS]=I.limit[ES]=0xffff;
	I.sregs[DS]=I.sregs[SS]=I.sregs[ES]=0;
	I.base[DS]=I.base[SS]=I.base[ES]=0;
	I.msw=0xfff0;
	I.flags=2;
	ExpandFlags(I.flags);
	I.idtr.base=0;I.idtr.limit=0x3ff;

	CHANGE_PC(I.pc);

}

/****************************************************************************/

/* ASG 971222 -- added these interface functions */

static void i286_get_context(void *dst)
{
	if( dst )
		*(i286_Regs*)dst = I;
}

static void i286_set_context(void *src)
{
	if( src )
	{
		I = *(i286_Regs*)src;
		if (PM) {

		} else {
			I.base[CS] = SegBase(CS);
			I.base[DS] = SegBase(DS);
			I.base[ES] = SegBase(ES);
			I.base[SS] = SegBase(SS);
		}
		CHANGE_PC(I.pc);
	}
}

static void set_irq_line(int irqline, int state)
{
	if (irqline == INPUT_LINE_NMI)
	{
		if (I.nmi_state == state)
			return;
		I.nmi_state = state;

		/* on a rising edge, signal the NMI */
		if (state != CLEAR_LINE)
			PREFIX(_interrupt)(I86_NMI_INT_VECTOR);
	}
	else
	{
		I.irq_state = state;

		/* if the IF is set, signal an interrupt */
		if (state != CLEAR_LINE && I.IF)
			PREFIX(_interrupt)(-1);
	}
}

static int i286_execute(int num_cycles)
{
	/* copy over the cycle counts if they're not correct */
	if (cycles.id != 80286)
		cycles = i286_cycles;

	/* adjust for any interrupts that came in */
	i286_ICount = num_cycles;
	i286_ICount -= I.extra_cycles;
	I.extra_cycles = 0;

	/* run until we're out */
	while(i286_ICount>0)
	{

//#define VERBOSE_DEBUG
#ifdef VERBOSE_DEBUG
		printf("[%04x:%04x]=%02x\tF:%04x\tAX=%04x\tBX=%04x\tCX=%04x\tDX=%04x %d%d%d%d%d%d%d%d%d\n",I.sregs[CS],I.pc - I.base[CS],ReadByte(I.pc),I.flags,I.regs.w[AX],I.regs.w[BX],I.regs.w[CX],I.regs.w[DX], I.AuxVal?1:0, I.OverVal?1:0, I.SignVal?1:0, I.ZeroVal?1:0, I.CarryVal?1:0, I.ParityVal?1:0,I.TF, I.IF, I.DirVal<0?1:0);
#endif

		CALL_MAME_DEBUG;

		seg_prefix=FALSE;
		I.prevpc = I.pc;

		TABLE286 // call instruction
    }

	/* adjust for any interrupts that came in */
	i286_ICount -= I.extra_cycles;
	I.extra_cycles = 0;

	return num_cycles - i286_ICount;
}

extern int i386_dasm_one(char *buffer, UINT32 eip, UINT8 *oprom, int addr_size, int op_size);

static offs_t i286_dasm(char *buffer, offs_t pc, UINT8 *oprom, UINT8 *opram, int bytes)
{
#ifdef MAME_DEBUG
	return i386_dasm_one(buffer, pc, oprom, 0, 0);
#else
	sprintf( buffer, "$%02X", cpu_readop(pc) );
	return 1;
#endif
}

static void i286_init(int index, int clock, const void *config, int (*irqcallback)(int))
{
	static const char type[] = "I286";
	state_save_register_item_array(type, index, I.regs.w);
	state_save_register_item(type, index, I.amask);
	state_save_register_item(type, index, I.pc);
	state_save_register_item(type, index, I.prevpc);
	state_save_register_item(type, index, I.msw);
	state_save_register_item_array(type, index, I.base);
	state_save_register_item_array(type, index, I.sregs);
	state_save_register_item_array(type, index, I.limit);
	state_save_register_item_array(type, index, I.rights);
	state_save_register_item(type, index, I.gdtr.base);
	state_save_register_item(type, index, I.gdtr.limit);
	state_save_register_item(type, index, I.idtr.base);
	state_save_register_item(type, index, I.idtr.limit);
	state_save_register_item(type, index, I.ldtr.sel);
	state_save_register_item(type, index, I.ldtr.base);
	state_save_register_item(type, index, I.ldtr.limit);
	state_save_register_item(type, index, I.ldtr.rights);
	state_save_register_item(type, index, I.tr.sel);
	state_save_register_item(type, index, I.tr.base);
	state_save_register_item(type, index, I.tr.limit);
	state_save_register_item(type, index, I.tr.rights);
	state_save_register_item(type, index, I.AuxVal);
	state_save_register_item(type, index, I.OverVal);
	state_save_register_item(type, index, I.SignVal);
	state_save_register_item(type, index, I.ZeroVal);
	state_save_register_item(type, index, I.CarryVal);
	state_save_register_item(type, index, I.DirVal);
	state_save_register_item(type, index, I.ParityVal);
	state_save_register_item(type, index, I.TF);
	state_save_register_item(type, index, I.IF);
	state_save_register_item(type, index, I.int_vector);
	state_save_register_item(type, index, I.nmi_state);
	state_save_register_item(type, index, I.irq_state);
	state_save_register_item(type, index, I.extra_cycles);

	I.irq_callback = irqcallback;

	/* If a reset parameter is given, take it as pointer to an address mask */
	if( config )
		I.amask = *(unsigned*)config;
	else
		I.amask = 0x00ffff;
}





/**************************************************************************
 * Generic set_info
 **************************************************************************/

static void i286_set_info(UINT32 state, union cpuinfo *info)
{
	switch (state)
	{
		/* --- the following bits of info are set as 64-bit signed integers --- */
		case CPUINFO_INT_INPUT_STATE + 0:				set_irq_line(0, info->i);				break;
		case CPUINFO_INT_INPUT_STATE + INPUT_LINE_NMI:	set_irq_line(INPUT_LINE_NMI, info->i);	break;

		case CPUINFO_INT_INPUT_STATE + INPUT_LINE_A20:	i286_set_a20_line(info->i);				break;

		case CPUINFO_INT_PC:
		case CPUINFO_INT_REGISTER + I286_PC:
			if (PM)
			{
				/* protected mode NYI */
			}
			else
			{
				if (info->i - I.base[CS] >= 0x10000)
				{
					I.base[CS] = info->i & 0xffff0;
					I.sregs[CS] = I.base[CS] >> 4;
				}
				I.pc = info->i;
			}
			break;

		case CPUINFO_INT_REGISTER + I286_IP:			I.pc = I.base[CS] + info->i;			break;
		case CPUINFO_INT_SP:
			if (PM)
			{
				/* protected mode NYI */
			}
			else
			{
				if (info->i - I.base[SS] < 0x10000)
				{
					I.regs.w[SP] = info->i - I.base[SS];
				}
				else
				{
					I.base[SS] = info->i & 0xffff0;
					I.sregs[SS] = I.base[SS] >> 4;
					I.regs.w[SP] = info->i & 0x0000f;
				}
			}
			break;

		case CPUINFO_INT_REGISTER + I286_SP:			I.regs.w[SP] = info->i; 				break;
		case CPUINFO_INT_REGISTER + I286_FLAGS: 		I.flags = info->i;	ExpandFlags(info->i); break;
		case CPUINFO_INT_REGISTER + I286_AX:			I.regs.w[AX] = info->i; 				break;
		case CPUINFO_INT_REGISTER + I286_CX:			I.regs.w[CX] = info->i; 				break;
		case CPUINFO_INT_REGISTER + I286_DX:			I.regs.w[DX] = info->i; 				break;
		case CPUINFO_INT_REGISTER + I286_BX:			I.regs.w[BX] = info->i; 				break;
		case CPUINFO_INT_REGISTER + I286_BP:			I.regs.w[BP] = info->i; 				break;
		case CPUINFO_INT_REGISTER + I286_SI:			I.regs.w[SI] = info->i; 				break;
		case CPUINFO_INT_REGISTER + I286_DI:			I.regs.w[DI] = info->i; 				break;
		case CPUINFO_INT_REGISTER + I286_ES:			I.sregs[ES] = info->i;	I.base[ES] = SegBase(ES); break;
		case CPUINFO_INT_REGISTER + I286_CS:			I.sregs[CS] = info->i;	I.base[CS] = SegBase(CS); break;
		case CPUINFO_INT_REGISTER + I286_SS:			I.sregs[SS] = info->i;	I.base[SS] = SegBase(SS); break;
		case CPUINFO_INT_REGISTER + I286_DS:			I.sregs[DS] = info->i;	I.base[DS] = SegBase(DS); break;
		case CPUINFO_INT_REGISTER + I286_VECTOR:		I.int_vector = info->i; 				break;
	}
}



/****************************************************************************
 * Generic get_info
 ****************************************************************************/

void i286_get_info(UINT32 state, union cpuinfo *info)
{
	switch (state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case CPUINFO_INT_CONTEXT_SIZE:					info->i = sizeof(I);					break;
		case CPUINFO_INT_INPUT_LINES:					info->i = 1;							break;
		case CPUINFO_INT_DEFAULT_IRQ_VECTOR:			info->i = 0xff;							break;
		case CPUINFO_INT_ENDIANNESS:					info->i = CPU_IS_LE;					break;
		case CPUINFO_INT_CLOCK_DIVIDER:					info->i = 1;							break;
		case CPUINFO_INT_MIN_INSTRUCTION_BYTES:			info->i = 1;							break;
		case CPUINFO_INT_MAX_INSTRUCTION_BYTES:			info->i = 8;							break;
		case CPUINFO_INT_MIN_CYCLES:					info->i = 1;							break;
		case CPUINFO_INT_MAX_CYCLES:					info->i = 50;							break;

		case CPUINFO_INT_DATABUS_WIDTH + ADDRESS_SPACE_PROGRAM:	info->i = 8;					break;
		case CPUINFO_INT_ADDRBUS_WIDTH + ADDRESS_SPACE_PROGRAM: info->i = 24;					break;
		case CPUINFO_INT_ADDRBUS_SHIFT + ADDRESS_SPACE_PROGRAM: info->i = 0;					break;
		case CPUINFO_INT_DATABUS_WIDTH + ADDRESS_SPACE_DATA:	info->i = 0;					break;
		case CPUINFO_INT_ADDRBUS_WIDTH + ADDRESS_SPACE_DATA: 	info->i = 0;					break;
		case CPUINFO_INT_ADDRBUS_SHIFT + ADDRESS_SPACE_DATA: 	info->i = 0;					break;
		case CPUINFO_INT_DATABUS_WIDTH + ADDRESS_SPACE_IO:		info->i = 8;					break;
		case CPUINFO_INT_ADDRBUS_WIDTH + ADDRESS_SPACE_IO: 		info->i = 16;					break;
		case CPUINFO_INT_ADDRBUS_SHIFT + ADDRESS_SPACE_IO: 		info->i = 0;					break;

		case CPUINFO_INT_INPUT_STATE + 0:				info->i = I.irq_state;					break;
		case CPUINFO_INT_INPUT_STATE + INPUT_LINE_NMI:	info->i = I.nmi_state;					break;

		case CPUINFO_INT_PREVIOUSPC:					info->i = I.prevpc;						break;

		case CPUINFO_INT_PC:
		case CPUINFO_INT_REGISTER + I286_PC:			info->i = I.pc;							break;
		case CPUINFO_INT_REGISTER + I286_IP:			info->i = I.pc - I.base[CS];			break;
		case CPUINFO_INT_SP:							info->i = I.base[SS] + I.regs.w[SP];	break;
		case CPUINFO_INT_REGISTER + I286_SP:			info->i = I.regs.w[SP];					break;
		case CPUINFO_INT_REGISTER + I286_FLAGS: 		CompressFlags(); info->i = I.flags;		break;
		case CPUINFO_INT_REGISTER + I286_AX:			info->i = I.regs.w[AX];					break;
		case CPUINFO_INT_REGISTER + I286_CX:			info->i = I.regs.w[CX];					break;
		case CPUINFO_INT_REGISTER + I286_DX:			info->i = I.regs.w[DX];					break;
		case CPUINFO_INT_REGISTER + I286_BX:			info->i = I.regs.w[BX];					break;
		case CPUINFO_INT_REGISTER + I286_BP:			info->i = I.regs.w[BP];					break;
		case CPUINFO_INT_REGISTER + I286_SI:			info->i = I.regs.w[SI];					break;
		case CPUINFO_INT_REGISTER + I286_DI:			info->i = I.regs.w[DI];					break;
		case CPUINFO_INT_REGISTER + I286_ES:			info->i = I.sregs[ES];					break;
		case CPUINFO_INT_REGISTER + I286_CS:			info->i = I.sregs[CS];					break;
		case CPUINFO_INT_REGISTER + I286_SS:			info->i = I.sregs[SS];					break;
		case CPUINFO_INT_REGISTER + I286_DS:			info->i = I.sregs[DS];					break;
		case CPUINFO_INT_REGISTER + I286_VECTOR:		info->i = I.int_vector;					break;
		case CPUINFO_INT_REGISTER + I286_MSW:			info->i = I.msw;						break;
		case CPUINFO_INT_REGISTER + I286_GDTR:			info->i = I.gdtr.base;					break;
		case CPUINFO_INT_REGISTER + I286_GDTR_2:		info->i = I.gdtr.limit;					break;
		case CPUINFO_INT_REGISTER + I286_IDTR:			info->i = I.idtr.base;					break;
		case CPUINFO_INT_REGISTER + I286_IDTR_2:		info->i = I.idtr.limit;					break;
		case CPUINFO_INT_REGISTER + I286_LDTR:			info->i = I.ldtr.base;					break;
		case CPUINFO_INT_REGISTER + I286_LDTR_2:		info->i = I.ldtr.limit;					break;
		case CPUINFO_INT_REGISTER + I286_TR:			info->i = I.tr.base;					break;
		case CPUINFO_INT_REGISTER + I286_TR_2:			info->i = I.tr.limit;					break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case CPUINFO_PTR_SET_INFO:						info->setinfo = i286_set_info;			break;
		case CPUINFO_PTR_GET_CONTEXT:					info->getcontext = i286_get_context;	break;
		case CPUINFO_PTR_SET_CONTEXT:					info->setcontext = i286_set_context;	break;
		case CPUINFO_PTR_INIT:							info->init = i286_init;					break;
		case CPUINFO_PTR_RESET:							info->reset = i286_reset;				break;
		case CPUINFO_PTR_EXIT:							info->exit = NULL;						break;
		case CPUINFO_PTR_EXECUTE:						info->execute = i286_execute;			break;
		case CPUINFO_PTR_BURN:							info->burn = NULL;						break;
		case CPUINFO_PTR_DISASSEMBLE_NEW:				info->disassemble_new = i286_dasm;		break;
		case CPUINFO_PTR_INSTRUCTION_COUNTER:			info->icount = &i286_ICount;			break;
		case CPUINFO_PTR_REGISTER_LAYOUT:				info->p = i286_reg_layout;				break;
		case CPUINFO_PTR_WINDOW_LAYOUT:					info->p = i286_win_layout;				break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case CPUINFO_STR_NAME:							strcpy(info->s = cpuintrf_temp_str(), "I80286"); break;
		case CPUINFO_STR_CORE_FAMILY:					strcpy(info->s = cpuintrf_temp_str(), "Intel 80286"); break;
		case CPUINFO_STR_CORE_VERSION:					strcpy(info->s = cpuintrf_temp_str(), "1.4"); break;
		case CPUINFO_STR_CORE_FILE:						strcpy(info->s = cpuintrf_temp_str(), __FILE__); break;
		case CPUINFO_STR_CORE_CREDITS:					strcpy(info->s = cpuintrf_temp_str(), "Real mode i286 emulator v1.4 by Fabrice Frances\n(initial work I.based on David Hedley's pcemu)"); break;

		case CPUINFO_STR_FLAGS:
			I.flags = CompressFlags();
			sprintf(info->s = cpuintrf_temp_str(), "%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c",
					I.flags & 0x8000 ? '?' : '.',
					I.flags & 0x4000 ? '?' : '.',
					I.flags & 0x2000 ? '?' : '.',
					I.flags & 0x1000 ? '?' : '.',
					I.flags & 0x0800 ? 'O' : '.',
					I.flags & 0x0400 ? 'D' : '.',
					I.flags & 0x0200 ? 'I' : '.',
					I.flags & 0x0100 ? 'T' : '.',
					I.flags & 0x0080 ? 'S' : '.',
					I.flags & 0x0040 ? 'Z' : '.',
					I.flags & 0x0020 ? '?' : '.',
					I.flags & 0x0010 ? 'A' : '.',
					I.flags & 0x0008 ? '?' : '.',
					I.flags & 0x0004 ? 'P' : '.',
					I.flags & 0x0002 ? 'N' : '.',
					I.flags & 0x0001 ? 'C' : '.');
			break;

		case CPUINFO_STR_REGISTER + I286_PC: 			sprintf(info->s = cpuintrf_temp_str(), "PC:%04X", I.pc); break;
		case CPUINFO_STR_REGISTER + I286_IP: 			sprintf(info->s = cpuintrf_temp_str(), "IP: %04X", I.pc - I.base[CS]); break;
		case CPUINFO_STR_REGISTER + I286_SP: 			sprintf(info->s = cpuintrf_temp_str(), "SP: %04X", I.regs.w[SP]);  break;
		case CPUINFO_STR_REGISTER + I286_FLAGS:			sprintf(info->s = cpuintrf_temp_str(), "F:%04X", I.flags);         break;
		case CPUINFO_STR_REGISTER + I286_AX: 			sprintf(info->s = cpuintrf_temp_str(), "AX:%04X", I.regs.w[AX]);   break;
		case CPUINFO_STR_REGISTER + I286_CX: 			sprintf(info->s = cpuintrf_temp_str(), "CX:%04X", I.regs.w[CX]);   break;
		case CPUINFO_STR_REGISTER + I286_DX: 			sprintf(info->s = cpuintrf_temp_str(), "DX:%04X", I.regs.w[DX]);   break;
		case CPUINFO_STR_REGISTER + I286_BX: 			sprintf(info->s = cpuintrf_temp_str(), "BX:%04X", I.regs.w[BX]);   break;
		case CPUINFO_STR_REGISTER + I286_BP: 			sprintf(info->s = cpuintrf_temp_str(), "BP:%04X", I.regs.w[BP]);   break;
		case CPUINFO_STR_REGISTER + I286_SI: 			sprintf(info->s = cpuintrf_temp_str(), "SI: %04X", I.regs.w[SI]);  break;
		case CPUINFO_STR_REGISTER + I286_DI: 			sprintf(info->s = cpuintrf_temp_str(), "DI: %04X", I.regs.w[DI]);  break;
		case CPUINFO_STR_REGISTER + I286_CS: 			sprintf(info->s = cpuintrf_temp_str(), "CS:  %04X %02X", I.sregs[CS], I.rights[CS]);    break;
		case CPUINFO_STR_REGISTER + I286_CS_2: 			sprintf(info->s = cpuintrf_temp_str(), "%06X %04X", I.base[CS], I.limit[CS]);			break;
		case CPUINFO_STR_REGISTER + I286_SS: 			sprintf(info->s = cpuintrf_temp_str(), "SS:  %04X %02X", I.sregs[SS], I.rights[SS]);    break;
		case CPUINFO_STR_REGISTER + I286_SS_2: 			sprintf(info->s = cpuintrf_temp_str(), "%06X %04X", I.base[SS], I.limit[SS]);			break;
		case CPUINFO_STR_REGISTER + I286_DS: 			sprintf(info->s = cpuintrf_temp_str(), "DS:  %04X %02X", I.sregs[DS], I.rights[DS]);    break;
		case CPUINFO_STR_REGISTER + I286_DS_2: 			sprintf(info->s = cpuintrf_temp_str(), "%06X %04X", I.base[DS], I.limit[DS]);			break;
		case CPUINFO_STR_REGISTER + I286_ES: 			sprintf(info->s = cpuintrf_temp_str(), "ES:  %04X %02X", I.sregs[ES], I.rights[ES]);    break;
		case CPUINFO_STR_REGISTER + I286_ES_2: 			sprintf(info->s = cpuintrf_temp_str(), "%06X %04X", I.base[ES], I.limit[ES]);			break;
		case CPUINFO_STR_REGISTER + I286_VECTOR:		sprintf(info->s = cpuintrf_temp_str(), "V:%02X", I.int_vector);    break;
		case CPUINFO_STR_REGISTER + I286_MSW:			sprintf(info->s = cpuintrf_temp_str(), "MSW:%04X", I.msw);					break;
		case CPUINFO_STR_REGISTER + I286_TR:			sprintf(info->s = cpuintrf_temp_str(), "GDTR: %06X", I.gdtr.base);			break;
		case CPUINFO_STR_REGISTER + I286_TR_2:			sprintf(info->s = cpuintrf_temp_str(), "%04X", I.gdtr.limit);				break;
		case CPUINFO_STR_REGISTER + I286_GDTR:			sprintf(info->s = cpuintrf_temp_str(), "IDTR: %06X", I.idtr.base);			break;
		case CPUINFO_STR_REGISTER + I286_GDTR_2:		sprintf(info->s = cpuintrf_temp_str(), "%04X", I.idtr.limit);				break;
		case CPUINFO_STR_REGISTER + I286_LDTR:			sprintf(info->s = cpuintrf_temp_str(), "LDTR:%04X %02X", I.ldtr.sel, I.ldtr.rights);    break;
		case CPUINFO_STR_REGISTER + I286_LDTR_2:		sprintf(info->s = cpuintrf_temp_str(), "%06X %04X", I.ldtr.base, I.ldtr.limit);			break;
		case CPUINFO_STR_REGISTER + I286_IDTR:			sprintf(info->s = cpuintrf_temp_str(), "IDTR: %06X", (unsigned) I.base);				break;
		case CPUINFO_STR_REGISTER + I286_IDTR_2:		sprintf(info->s = cpuintrf_temp_str(), "%04X", (unsigned) I.limit);						break;
		case CPUINFO_STR_REGISTER + I286_EMPTY:			sprintf(info->s = cpuintrf_temp_str(), " ");											break;
	}
}
