/*
    Intel 386 emulator

    Written by Ville Linde

    Currently supports:
        Intel 386
        Intel 486
        Intel Pentium
        Cyrix MediaGX
*/

#include "debugger.h"
#include "i386.h"
#include "i386intf.h"

#if defined(MAME_DEBUG) && defined(NEW_DEBUGGER)
#include "debug/debugcpu.h"
#endif

int i386_parity_table[256];
MODRM_TABLE i386_MODRM_table[256];

/*************************************************************************/

#define INT_DEBUG	1

static void i386_load_protected_mode_segment( I386_SREG *seg )
{
	UINT32 v1,v2;
	UINT32 base, limit;
	int entry;

	if( seg->selector & 0x4 ) {
			base = I.ldtr.base;
			limit = I.ldtr.limit;
		} else {
			base = I.gdtr.base;
			limit = I.gdtr.limit;
		}

		if (limit == 0)
			return;
	entry = (seg->selector % limit) & ~0x7;

		v1 = READ32( base + entry );
		v2 = READ32( base + entry + 4 );

	seg->base = (v2 & 0xff000000) | ((v2 & 0xff) << 16) | ((v1 >> 16) & 0xffff);
	seg->limit = ((v2 << 16) & 0xf0000) | (v1 & 0xffff);
	seg->d = ((v2 & 0x400000) && PROTECTED_MODE && !V8086_MODE) ? 1 : 0;
}

void i386_load_segment_descriptor( int segment )
{
	if (PROTECTED_MODE)
	{
		i386_load_protected_mode_segment( &I.sreg[segment] );
	}
	else
	{
		I.sreg[segment].base = I.sreg[segment].selector << 4;

		if( segment == CS && !I.performed_intersegment_jump )
			I.sreg[segment].base |= 0xfff00000;
	}
}

static UINT32 get_flags(void)
{
	UINT32 f = 0x2;
	f |= I.CF;
	f |= I.PF << 2;
	f |= I.AF << 4;
	f |= I.ZF << 6;
	f |= I.SF << 7;
	f |= I.TF << 8;
	f |= I.IF << 9;
	f |= I.DF << 10;
	f |= I.OF << 11;
	return (I.eflags & 0xFFFF0000) | (f & 0xFFFF);
}

static void set_flags( UINT32 f )
{
	I.CF = (f & 0x1) ? 1 : 0;
	I.PF = (f & 0x4) ? 1 : 0;
	I.AF = (f & 0x10) ? 1 : 0;
	I.ZF = (f & 0x40) ? 1 : 0;
	I.SF = (f & 0x80) ? 1 : 0;
	I.TF = (f & 0x100) ? 1 : 0;
	I.IF = (f & 0x200) ? 1 : 0;
	I.DF = (f & 0x400) ? 1 : 0;
	I.OF = (f & 0x800) ? 1 : 0;
}

static void sib_byte(UINT8 mod, UINT32* out_ea, UINT8* out_segment)
{
	UINT32 ea = 0;
	UINT8 segment = 0;
	UINT8 scale, i, base;
	UINT8 sib = FETCH();
	scale = (sib >> 6) & 0x3;
	i = (sib >> 3) & 0x7;
	base = sib & 0x7;

	switch( base )
	{
		case 0: ea = REG32(EAX); segment = DS; break;
		case 1: ea = REG32(ECX); segment = DS; break;
		case 2: ea = REG32(EDX); segment = DS; break;
		case 3: ea = REG32(EBX); segment = DS; break;
		case 4: ea = REG32(ESP); segment = SS; break;
		case 5:
			if( mod == 0 ) {
				ea = FETCH32();
				segment = DS;
			} else if( mod == 1 ) {
				ea = REG32(EBP);
				segment = SS;
			} else if( mod == 2 ) {
				ea = REG32(EBP);
				segment = SS;
			}
			break;
		case 6: ea = REG32(ESI); segment = DS; break;
		case 7: ea = REG32(EDI); segment = DS; break;
	}
	switch( i )
	{
		case 0: ea += REG32(EAX) * (1 << scale); break;
		case 1: ea += REG32(ECX) * (1 << scale); break;
		case 2: ea += REG32(EDX) * (1 << scale); break;
		case 3: ea += REG32(EBX) * (1 << scale); break;
		case 4: break;
		case 5: ea += REG32(EBP) * (1 << scale); break;
		case 6: ea += REG32(ESI) * (1 << scale); break;
		case 7: ea += REG32(EDI) * (1 << scale); break;
	}
	*out_ea = ea;
	*out_segment = segment;
}

static void modrm_to_EA(UINT8 mod_rm, UINT32* out_ea, UINT8* out_segment)
{
	INT8 disp8;
	INT16 disp16;
	INT32 disp32;
	UINT8 mod = (mod_rm >> 6) & 0x3;
	UINT8 rm = mod_rm & 0x7;
	UINT32 ea;
	UINT8 segment;

	if( mod_rm >= 0xc0 )
		fatalerror("i386: Called modrm_to_EA with modrm value %02X !",mod_rm);

	if( I.address_size ) {
		switch( rm )
		{
			default:
			case 0: ea = REG32(EAX); segment = DS; break;
			case 1: ea = REG32(ECX); segment = DS; break;
			case 2: ea = REG32(EDX); segment = DS; break;
			case 3: ea = REG32(EBX); segment = DS; break;
			case 4: sib_byte( mod, &ea, &segment ); break;
			case 5:
				if( mod == 0 ) {
					ea = FETCH32(); segment = DS;
				} else {
					ea = REG32(EBP); segment = SS;
				}
				break;
			case 6: ea = REG32(ESI); segment = DS; break;
			case 7: ea = REG32(EDI); segment = DS; break;
		}
		if( mod == 1 ) {
			disp8 = FETCH();
			ea += (INT32)disp8;
		} else if( mod == 2 ) {
			disp32 = FETCH32();
			ea += disp32;
		}

		if( I.segment_prefix )
			segment = I.segment_override;

		*out_ea = ea;
		*out_segment = segment;

	} else {
		switch( rm )
		{
			default:
			case 0: ea = REG16(BX) + REG16(SI); segment = DS; break;
			case 1: ea = REG16(BX) + REG16(DI); segment = DS; break;
			case 2: ea = REG16(BP) + REG16(SI); segment = SS; break;
			case 3: ea = REG16(BP) + REG16(DI); segment = SS; break;
			case 4: ea = REG16(SI); segment = DS; break;
			case 5: ea = REG16(DI); segment = DS; break;
			case 6:
				if( mod == 0 ) {
					ea = FETCH16(); segment = DS;
				} else {
					ea = REG16(BP); segment = SS;
				}
				break;
			case 7: ea = REG16(BX); segment = DS; break;
		}
		if( mod == 1 ) {
			disp8 = FETCH();
			ea += (INT32)disp8;
		} else if( mod == 2 ) {
			disp16 = FETCH16();
			ea += (INT32)disp16;
		}

		if( I.segment_prefix )
			segment = I.segment_override;

		*out_ea = ea & 0xffff;
		*out_segment = segment;
	}
}

static UINT32 GetNonTranslatedEA(UINT8 modrm)
{
	UINT8 segment;
	UINT32 ea;
	modrm_to_EA( modrm, &ea, &segment );
	return ea;
}

static UINT32 GetEA(UINT8 modrm)
{
	UINT8 segment;
	UINT32 ea;
	modrm_to_EA( modrm, &ea, &segment );
	return i386_translate( segment, ea );
}

static void i386_trap(int irq, int irq_gate)
{
	/*  I386 Interrupts/Traps/Faults:
     *
     *  0x00    Divide by zero
     *  0x01    Debug exception
     *  0x02    NMI
     *  0x03    Int3
     *  0x04    Overflow
     *  0x05    Array bounds check
     *  0x06    Illegal Opcode
     *  0x07    FPU not available
     *  0x08    Double fault
     *  0x09    Coprocessor segment overrun
     *  0x0a    Invalid task state
     *  0x0b    Segment not present
     *  0x0c    Stack exception
     *  0x0d    General Protection Fault
     *  0x0e    Page fault
     *  0x0f    Reserved
     *  0x10    Coprocessor error
     */
	UINT32 v1, v2;
	UINT32 offset;
	UINT16 segment;
	int entry = irq * (I.sreg[CS].d ? 8 : 4);

	/* Check if IRQ is out of IDTR's bounds */
	if( entry > I.idtr.limit ) {
		fatalerror("I386 Interrupt: IRQ out of IDTR bounds (IRQ: %d, IDTR Limit: %d)", irq, I.idtr.limit);
	}

	if( !I.sreg[CS].d )
	{
		/* 16-bit */
		PUSH16( get_flags() & 0xffff );
		PUSH16( I.sreg[CS].selector );
		PUSH16( I.eip );

		I.sreg[CS].selector = READ16( I.idtr.base + entry + 2 );
		I.eip = READ16( I.idtr.base + entry );
	}
	else
	{
		/* 32-bit */
		PUSH32( get_flags() & 0x00fcffff );
		PUSH32( I.sreg[CS].selector );
		PUSH32( I.eip );

		v1 = READ32( I.idtr.base + entry );
		v2 = READ32( I.idtr.base + entry + 4 );
		offset = (v2 & 0xffff0000) | (v1 & 0xffff);
		segment = (v1 >> 16) & 0xffff;

		I.sreg[CS].selector = segment;
		I.eip = offset;
	}

	if (irq_gate)
	{
		I.IF = 0;
	}

	i386_load_segment_descriptor(CS);
	CHANGE_PC(I.eip);
}

static void i386_check_irq_line(void)
{
	/* Check if the interrupts are enabled */
	if ( I.irq_line && I.IF )
	{
		i386_trap(I.irq_callback(0), 1);
	}
}

#include "cycles.h"

static UINT8 *cycle_table_rm[X86_NUM_CPUS];
static UINT8 *cycle_table_pm[X86_NUM_CPUS];

#define CYCLES_NUM(x)	(I.cycles -= (x))

INLINE void CYCLES(int x)
{
	if (PROTECTED_MODE)
	{
		I.cycles -= I.cycle_table_pm[x];
	}
	else
	{
		I.cycles -= I.cycle_table_rm[x];
	}
}

INLINE void CYCLES_RM(int modrm, int r, int m)
{
	if (modrm >= 0xc0)
	{
		if (PROTECTED_MODE)
		{
			I.cycles -= I.cycle_table_pm[r];
		}
		else
		{
			I.cycles -= I.cycle_table_rm[r];
		}
	}
	else
	{
		if (PROTECTED_MODE)
		{
			I.cycles -= I.cycle_table_pm[m];
		}
		else
		{
			I.cycles -= I.cycle_table_rm[m];
		}
	}
}

static void build_cycle_table(void)
{
	int i, j;
	for (j=0; j < X86_NUM_CPUS; j++)
	{
		cycle_table_rm[j] = auto_malloc(sizeof(UINT8) * CYCLES_NUM_OPCODES);
		cycle_table_pm[j] = auto_malloc(sizeof(UINT8) * CYCLES_NUM_OPCODES);

		for (i=0; i < sizeof(x86_cycle_table)/sizeof(X86_CYCLE_TABLE); i++)
		{
			int opcode = x86_cycle_table[i].op;
			cycle_table_rm[j][opcode] = x86_cycle_table[i].cpu_cycles[j][0];
			cycle_table_pm[j][opcode] = x86_cycle_table[i].cpu_cycles[j][1];
		}
	}
}



#include "i386ops.c"
#include "i386op16.c"
#include "i386op32.c"
#include "i486ops.c"
#include "pentops.c"
#include "x87ops.c"
#include "i386ops.h"

static void I386OP(decode_opcode)(void)
{
	I.opcode = FETCH();
	if( I.operand_size )
		I.opcode_table1_32[I.opcode]();
	else
		I.opcode_table1_16[I.opcode]();
}

/* Two-byte opcode prefix */
static void I386OP(decode_two_byte)(void)
{
	I.opcode = FETCH();
	if( I.operand_size )
		I.opcode_table2_32[I.opcode]();
	else
		I.opcode_table2_16[I.opcode]();
}

/*************************************************************************/

#if defined(MAME_DEBUG) && defined(NEW_DEBUGGER)

static UINT64 i386_debug_segbase(UINT32 ref, UINT32 params, UINT64 *param)
{
	UINT32 result;
	I386_SREG seg;

	if (PROTECTED_MODE)
	{
		memset(&seg, 0, sizeof(seg));
		seg.selector = (UINT16) param[0];
		i386_load_protected_mode_segment(&seg);
		result = seg.base;
	}
	else
	{
		result = param[0] << 4;
	}
	return result;
}

static UINT64 i386_debug_seglimit(UINT32 ref, UINT32 params, UINT64 *param)
{
	UINT32 result = 0;
	I386_SREG seg;

	if (PROTECTED_MODE)
	{
		memset(&seg, 0, sizeof(seg));
		seg.selector = (UINT16) param[0];
		i386_load_protected_mode_segment(&seg);
		result = seg.limit;
	}
	return result;
}

static void i386_debug_setup(void)
{
	symtable_add_function(global_symtable, "segbase", 0, 1, 1, i386_debug_segbase);
	symtable_add_function(global_symtable, "seglimit", 0, 1, 1, i386_debug_seglimit);
}

#endif /* defined(MAME_DEBUG) && defined(NEW_DEBUGGER) */

/*************************************************************************/

static void i386_postload(void)
{
	int i;
	for (i = 0; i < 6; i++)
		i386_load_segment_descriptor(i);
	CHANGE_PC(I.eip);
}

void i386_init(int index, int clock, const void *config, int (*irqcallback)(int))
{
	int i, j;
	static const int regs8[8] = {AL,CL,DL,BL,AH,CH,DH,BH};
	static const int regs16[8] = {AX,CX,DX,BX,SP,BP,SI,DI};
	static const int regs32[8] = {EAX,ECX,EDX,EBX,ESP,EBP,ESI,EDI};
	static const char state_type[] = "I386";

	build_cycle_table();

	for( i=0; i < 256; i++ ) {
		int c=0;
		for( j=0; j < 8; j++ ) {
			if( i & (1 << j) )
				c++;
		}
		i386_parity_table[i] = ~(c & 0x1) & 0x1;
	}

	for( i=0; i < 256; i++ ) {
		i386_MODRM_table[i].reg.b = regs8[(i >> 3) & 0x7];
		i386_MODRM_table[i].reg.w = regs16[(i >> 3) & 0x7];
		i386_MODRM_table[i].reg.d = regs32[(i >> 3) & 0x7];

		i386_MODRM_table[i].rm.b = regs8[i & 0x7];
		i386_MODRM_table[i].rm.w = regs16[i & 0x7];
		i386_MODRM_table[i].rm.d = regs32[i & 0x7];
	}

	I.irq_callback = irqcallback;

	state_save_register_item_array(state_type, index,	I.reg.d);
	state_save_register_item(state_type, index, I.sreg[ES].selector);
	state_save_register_item(state_type, index, I.sreg[CS].selector);
	state_save_register_item(state_type, index, I.sreg[SS].selector);
	state_save_register_item(state_type, index, I.sreg[DS].selector);
	state_save_register_item(state_type, index, I.sreg[FS].selector);
	state_save_register_item(state_type, index, I.sreg[GS].selector);
	state_save_register_item(state_type, index, I.eip);
	state_save_register_item(state_type, index, I.prev_eip);
	state_save_register_item(state_type, index, I.CF);
	state_save_register_item(state_type, index, I.DF);
	state_save_register_item(state_type, index, I.SF);
	state_save_register_item(state_type, index, I.OF);
	state_save_register_item(state_type, index, I.ZF);
	state_save_register_item(state_type, index, I.PF);
	state_save_register_item(state_type, index, I.AF);
	state_save_register_item(state_type, index, I.IF);
	state_save_register_item(state_type, index, I.TF);
	state_save_register_item_array(state_type, index,	I.cr);
	state_save_register_item_array(state_type, index,	I.dr);
	state_save_register_item_array(state_type, index,	I.tr);
	state_save_register_item(state_type, index, I.idtr.base);
	state_save_register_item(state_type, index, I.idtr.limit);
	state_save_register_item(state_type, index, I.gdtr.base);
	state_save_register_item(state_type, index, I.gdtr.limit);
	state_save_register_item(state_type, index,  I.irq_line);
	state_save_register_item(state_type, index, I.performed_intersegment_jump);
	state_save_register_func_postload(i386_postload);
}

static void build_opcode_table(UINT32 features)
{
	int i;
	for (i=0; i < 256; i++)
	{
		I.opcode_table1_16[i] = I386OP(invalid);
		I.opcode_table1_32[i] = I386OP(invalid);
		I.opcode_table2_16[i] = I386OP(invalid);
		I.opcode_table2_32[i] = I386OP(invalid);
	}

	for (i=0; i < sizeof(x86_opcode_table)/sizeof(X86_OPCODE); i++)
	{
		const X86_OPCODE *op = &x86_opcode_table[i];

		if ((op->flags & features))
		{
			if (op->flags & OP_2BYTE)
			{
				I.opcode_table2_32[op->opcode] = op->handler32;
				I.opcode_table2_16[op->opcode] = op->handler16;
			}
			else
			{
				I.opcode_table1_32[op->opcode] = op->handler32;
				I.opcode_table1_16[op->opcode] = op->handler16;
			}
		}
	}
}

void i386_reset(void)
{
	int (*save_irqcallback)(int);

	save_irqcallback = I.irq_callback;
	memset( &I, 0, sizeof(I386_REGS) );
	I.irq_callback = save_irqcallback;

	I.sreg[CS].selector = 0xf000;
	I.sreg[CS].base		= 0xffff0000;
	I.sreg[CS].limit	= 0xffff;

	I.idtr.base = 0;
	I.idtr.limit = 0x3ff;

	I.a20_mask = ~0;

	I.cr[0] = 0;
	I.eflags = 0;
	I.eip = 0xfff0;

	REG32(EAX) = 0x0308;	// Intel 386, stepping D1
	REG32(EDX) = 0;

	build_opcode_table(OP_I386);
	I.cycle_table_rm = cycle_table_rm[CPU_CYCLES_I386];
	I.cycle_table_pm = cycle_table_pm[CPU_CYCLES_I386];

	CHANGE_PC(I.eip);
}

static void i386_get_context(void *dst)
{
	if(dst) {
		*(I386_REGS *)dst = I;
	}
}

static void i386_set_context(void *src)
{
	if(src) {
		I = *(I386_REGS *)src;
	}

	CHANGE_PC(I.eip);
}

static void i386_set_irq_line(int irqline, int state)
{
	if (I.halted)
	{
		I.halted = 0;
	}

	if ( irqline == INPUT_LINE_NMI )
	{
		/* NMI (I do not think that this is 100% right) */
		if ( state )
			i386_trap(2, 1);
	}
	else
	{
		I.irq_line = state;
		i386_check_irq_line();
	}
}

static void i386_set_a20_line(int state)
{
	if (state)
	{
		I.a20_mask = ~0;
	}
	else
	{
		I.a20_mask = ~(1 << 20);
	}
}

int i386_execute(int num_cycles)
{
	I.cycles = num_cycles;
	I.base_cycles = num_cycles;
	CHANGE_PC(I.eip);

	if (I.halted)
	{
		I.tsc += num_cycles;
		return num_cycles;
	}

	while( I.cycles > 0 )
	{
		I.operand_size = I.sreg[CS].d;
		I.address_size = I.sreg[CS].d;
		I.segment_prefix = 0;
		I.prev_eip = I.eip;

		CALL_MAME_DEBUG;

		I386OP(decode_opcode)();
	}
	I.tsc += (num_cycles - I.cycles);

	return num_cycles - I.cycles;
}

/*************************************************************************/

static UINT8 i386_reg_layout[] =
{
	I386_EIP,		I386_ESP,		-1,
	I386_EAX,		I386_EBP,		-1,
	I386_EBX,		I386_ESI,		-1,
	I386_ECX,		I386_EDI,		-1,
	I386_EDX,		-1,
	I386_CS,		I386_CR0,		-1,
	I386_SS,		I386_CR1,		-1,
	I386_DS,		I386_CR2,		-1,
	I386_ES,		I386_CR3,		-1,
	I386_FS,		I386_TR6,		-1,
	I386_GS,		I386_TR7,		-1,
	I386_DR0,		I386_DR1,		-1,
	I386_DR2,		I386_DR3,		-1,
	I386_DR4,		I386_DR5,		-1,
	I386_DR6,		I386_DR7,		0
};

static UINT8 i386_win_layout[] =
{
	 0, 0,32,12,	/* register window (top rows) */
	33, 0,46,15,	/* disassembler window (left colums) */
	33,10,46,12,	/* memory #2 window (right, lower middle) */
	 0,19,32, 3,	/* memory #1 window (right, upper middle) */
	 0,23,80, 1,	/* command line window (bottom rows) */
};

/*************************************************************************/

static int translate_address_cb(int space, offs_t *addr)
{
	int result = 1;
	if (space == ADDRESS_SPACE_PROGRAM)
	{
		if (I.cr[0] & 0x80000000)
			result = translate_address(addr);
		*addr &= I.a20_mask;
	}
	return result;
}

static offs_t i386_dasm(char *buffer, offs_t pc, UINT8 *oprom, UINT8 *opram, int bytes)
{
#ifdef MAME_DEBUG
	return i386_dasm_one(buffer, pc, oprom, I.sreg[CS].d, I.sreg[CS].d);
#else
	sprintf( buffer, "$%02X", *oprom );
	return 1;
#endif
}

static void i386_set_info(UINT32 state, union cpuinfo *info)
{
	if (state == CPUINFO_INT_INPUT_STATE+INPUT_LINE_A20)
	{
		i386_set_a20_line(info->i);
		return;
	}
	if (state >= CPUINFO_INT_INPUT_STATE && state <= CPUINFO_INT_INPUT_STATE + 1)
	{
		i386_set_irq_line(state-CPUINFO_INT_INPUT_STATE, info->i);
		return;
	}

	switch (state)
	{
		/* --- the following bits of info are set as 64-bit signed integers --- */
		case CPUINFO_INT_PC:
		case CPUINFO_INT_REGISTER + I386_PC:			I.pc = info->i;break;
		case CPUINFO_INT_REGISTER + I386_EIP:			I.eip = info->i; CHANGE_PC(I.eip); break;
		case CPUINFO_INT_REGISTER + I386_AL:			REG8(AL) = info->i; break;
		case CPUINFO_INT_REGISTER + I386_AH:			REG8(AH) = info->i; break;
		case CPUINFO_INT_REGISTER + I386_BL:			REG8(BL) = info->i; break;
		case CPUINFO_INT_REGISTER + I386_BH:			REG8(BH) = info->i; break;
		case CPUINFO_INT_REGISTER + I386_CL:			REG8(CL) = info->i; break;
		case CPUINFO_INT_REGISTER + I386_CH:			REG8(CH) = info->i; break;
		case CPUINFO_INT_REGISTER + I386_DL:			REG8(DL) = info->i; break;
		case CPUINFO_INT_REGISTER + I386_DH:			REG8(DH) = info->i; break;
		case CPUINFO_INT_REGISTER + I386_AX:			REG16(AX) = info->i; break;
		case CPUINFO_INT_REGISTER + I386_BX:			REG16(BX) = info->i; break;
		case CPUINFO_INT_REGISTER + I386_CX:			REG16(CX) = info->i; break;
		case CPUINFO_INT_REGISTER + I386_DX:			REG16(DX) = info->i; break;
		case CPUINFO_INT_REGISTER + I386_SI:			REG16(SI) = info->i; break;
		case CPUINFO_INT_REGISTER + I386_DI:			REG16(DI) = info->i; break;
		case CPUINFO_INT_REGISTER + I386_BP:			REG16(BP) = info->i; break;
		case CPUINFO_INT_REGISTER + I386_SP:			REG16(SP) = info->i; break;
		case CPUINFO_INT_REGISTER + I386_IP:			I.eip = (I.eip & ~0xFFFF) | (info->i & 0xFFFF); CHANGE_PC(I.eip); break;
		case CPUINFO_INT_REGISTER + I386_EAX:			REG32(EAX) = info->i; break;
		case CPUINFO_INT_REGISTER + I386_EBX:			REG32(EBX) = info->i; break;
		case CPUINFO_INT_REGISTER + I386_ECX:			REG32(ECX) = info->i; break;
		case CPUINFO_INT_REGISTER + I386_EDX:			REG32(EDX) = info->i; break;
		case CPUINFO_INT_REGISTER + I386_EBP:			REG32(EBP) = info->i; break;
		case CPUINFO_INT_SP:
		case CPUINFO_INT_REGISTER + I386_ESP:			REG32(ESP) = info->i; break;
		case CPUINFO_INT_REGISTER + I386_ESI:			REG32(ESI) = info->i; break;
		case CPUINFO_INT_REGISTER + I386_EDI:			REG32(EDI) = info->i; break;
		case CPUINFO_INT_REGISTER + I386_EFLAGS:		I.eflags = info->i; break;
		case CPUINFO_INT_REGISTER + I386_CS:			I.sreg[CS].selector = info->i & 0xffff; break;
		case CPUINFO_INT_REGISTER + I386_SS:			I.sreg[SS].selector = info->i & 0xffff; break;
		case CPUINFO_INT_REGISTER + I386_DS:			I.sreg[DS].selector = info->i & 0xffff; break;
		case CPUINFO_INT_REGISTER + I386_ES:			I.sreg[ES].selector = info->i & 0xffff; break;
		case CPUINFO_INT_REGISTER + I386_FS:			I.sreg[FS].selector = info->i & 0xffff; break;
		case CPUINFO_INT_REGISTER + I386_GS:			I.sreg[GS].selector = info->i & 0xffff; break;
		case CPUINFO_INT_REGISTER + I386_CR0:			I.cr[0] = info->i; break;
		case CPUINFO_INT_REGISTER + I386_CR1:			I.cr[1] = info->i; break;
		case CPUINFO_INT_REGISTER + I386_CR2:			I.cr[2] = info->i; break;
		case CPUINFO_INT_REGISTER + I386_CR3:			I.cr[3] = info->i; break;
		case CPUINFO_INT_REGISTER + I386_DR0:			I.dr[0] = info->i; break;
		case CPUINFO_INT_REGISTER + I386_DR1:			I.dr[1] = info->i; break;
		case CPUINFO_INT_REGISTER + I386_DR2:			I.dr[2] = info->i; break;
		case CPUINFO_INT_REGISTER + I386_DR3:			I.dr[3] = info->i; break;
		case CPUINFO_INT_REGISTER + I386_DR4:			I.dr[4] = info->i; break;
		case CPUINFO_INT_REGISTER + I386_DR5:			I.dr[5] = info->i; break;
		case CPUINFO_INT_REGISTER + I386_DR6:			I.dr[6] = info->i; break;
		case CPUINFO_INT_REGISTER + I386_DR7:			I.dr[7] = info->i; break;
		case CPUINFO_INT_REGISTER + I386_TR6:			I.tr[6] = info->i; break;
		case CPUINFO_INT_REGISTER + I386_TR7:			I.tr[7] = info->i; break;
	}
}

void i386_get_info(UINT32 state, union cpuinfo *info)
{
	switch (state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case CPUINFO_INT_CONTEXT_SIZE:				info->i = sizeof(I386_REGS);			break;
		case CPUINFO_INT_INPUT_LINES:				info->i = 32;							break;
		case CPUINFO_INT_DEFAULT_IRQ_VECTOR:		info->i = 0;							break;
		case CPUINFO_INT_ENDIANNESS:				info->i = CPU_IS_LE;					break;
		case CPUINFO_INT_CLOCK_DIVIDER:				info->i = 1;							break;
		case CPUINFO_INT_MIN_INSTRUCTION_BYTES:		info->i = 1;							break;
		case CPUINFO_INT_MAX_INSTRUCTION_BYTES:		info->i = 8;							break;
		case CPUINFO_INT_MIN_CYCLES:				info->i = 1;							break;
		case CPUINFO_INT_MAX_CYCLES:				info->i = 40;							break;

		case CPUINFO_INT_DATABUS_WIDTH + ADDRESS_SPACE_PROGRAM:	info->i = 32;					break;
		case CPUINFO_INT_ADDRBUS_WIDTH + ADDRESS_SPACE_PROGRAM: info->i = 32;					break;
		case CPUINFO_INT_ADDRBUS_SHIFT + ADDRESS_SPACE_PROGRAM: info->i = 0;					break;
		case CPUINFO_INT_LOGADDR_WIDTH + ADDRESS_SPACE_PROGRAM: info->i = 32;					break;
		case CPUINFO_INT_PAGE_SHIFT + ADDRESS_SPACE_PROGRAM: 	info->i = 12;					break;
		case CPUINFO_INT_DATABUS_WIDTH + ADDRESS_SPACE_DATA:	info->i = 0;					break;
		case CPUINFO_INT_ADDRBUS_WIDTH + ADDRESS_SPACE_DATA: 	info->i = 0;					break;
		case CPUINFO_INT_ADDRBUS_SHIFT + ADDRESS_SPACE_DATA: 	info->i = 0;					break;
		case CPUINFO_INT_DATABUS_WIDTH + ADDRESS_SPACE_IO:		info->i = 32;					break;
		case CPUINFO_INT_ADDRBUS_WIDTH + ADDRESS_SPACE_IO: 		info->i = 32;					break;
		case CPUINFO_INT_ADDRBUS_SHIFT + ADDRESS_SPACE_IO: 		info->i = 0;					break;

		case CPUINFO_INT_INPUT_STATE:			info->i = CLEAR_LINE;	break;

		case CPUINFO_INT_PREVIOUSPC:				/* not implemented */					break;

		case CPUINFO_INT_PC:
		case CPUINFO_INT_REGISTER + I386_PC:			info->i = I.pc; break;
		case CPUINFO_INT_REGISTER + I386_EIP:			info->i = I.eip; break;
		case CPUINFO_INT_REGISTER + I386_AL:			info->i = REG8(AL); break;
		case CPUINFO_INT_REGISTER + I386_AH:			info->i = REG8(AH); break;
		case CPUINFO_INT_REGISTER + I386_BL:			info->i = REG8(BL); break;
		case CPUINFO_INT_REGISTER + I386_BH:			info->i = REG8(BH); break;
		case CPUINFO_INT_REGISTER + I386_CL:			info->i = REG8(CL); break;
		case CPUINFO_INT_REGISTER + I386_CH:			info->i = REG8(CH); break;
		case CPUINFO_INT_REGISTER + I386_DL:			info->i = REG8(DL); break;
		case CPUINFO_INT_REGISTER + I386_DH:			info->i = REG8(DH); break;
		case CPUINFO_INT_REGISTER + I386_AX:			info->i = REG16(AX); break;
		case CPUINFO_INT_REGISTER + I386_BX:			info->i = REG16(BX); break;
		case CPUINFO_INT_REGISTER + I386_CX:			info->i = REG16(CX); break;
		case CPUINFO_INT_REGISTER + I386_DX:			info->i = REG16(DX); break;
		case CPUINFO_INT_REGISTER + I386_SI:			info->i = REG16(SI); break;
		case CPUINFO_INT_REGISTER + I386_DI:			info->i = REG16(DI); break;
		case CPUINFO_INT_REGISTER + I386_BP:			info->i = REG16(BP); break;
		case CPUINFO_INT_REGISTER + I386_SP:			info->i = REG16(SP); break;
		case CPUINFO_INT_REGISTER + I386_IP:			info->i = I.eip & 0xFFFF; break;
		case CPUINFO_INT_REGISTER + I386_EAX:			info->i = REG32(EAX); break;
		case CPUINFO_INT_REGISTER + I386_EBX:			info->i = REG32(EBX); break;
		case CPUINFO_INT_REGISTER + I386_ECX:			info->i = REG32(ECX); break;
		case CPUINFO_INT_REGISTER + I386_EDX:			info->i = REG32(EDX); break;
		case CPUINFO_INT_REGISTER + I386_EBP:			info->i = REG32(EBP); break;
		case CPUINFO_INT_REGISTER + I386_ESP:			info->i = REG32(ESP); break;
		case CPUINFO_INT_REGISTER + I386_ESI:			info->i = REG32(ESI); break;
		case CPUINFO_INT_REGISTER + I386_EDI:			info->i = REG32(EDI); break;
		case CPUINFO_INT_REGISTER + I386_EFLAGS:		info->i = I.eflags; break;
		case CPUINFO_INT_REGISTER + I386_CS:			info->i = I.sreg[CS].selector; break;
		case CPUINFO_INT_REGISTER + I386_SS:			info->i = I.sreg[SS].selector; break;
		case CPUINFO_INT_REGISTER + I386_DS:			info->i = I.sreg[DS].selector; break;
		case CPUINFO_INT_REGISTER + I386_ES:			info->i = I.sreg[ES].selector; break;
		case CPUINFO_INT_REGISTER + I386_FS:			info->i = I.sreg[FS].selector; break;
		case CPUINFO_INT_REGISTER + I386_GS:			info->i = I.sreg[GS].selector; break;
		case CPUINFO_INT_REGISTER + I386_CR0:			info->i = I.cr[0]; break;
		case CPUINFO_INT_REGISTER + I386_CR1:			info->i = I.cr[1]; break;
		case CPUINFO_INT_REGISTER + I386_CR2:			info->i = I.cr[2]; break;
		case CPUINFO_INT_REGISTER + I386_CR3:			info->i = I.cr[3]; break;
		case CPUINFO_INT_REGISTER + I386_DR0:			info->i = I.dr[0]; break;
		case CPUINFO_INT_REGISTER + I386_DR1:			info->i = I.dr[1]; break;
		case CPUINFO_INT_REGISTER + I386_DR2:			info->i = I.dr[2]; break;
		case CPUINFO_INT_REGISTER + I386_DR3:			info->i = I.dr[3]; break;
		case CPUINFO_INT_REGISTER + I386_DR4:			info->i = I.dr[4]; break;
		case CPUINFO_INT_REGISTER + I386_DR5:			info->i = I.dr[5]; break;
		case CPUINFO_INT_REGISTER + I386_DR6:			info->i = I.dr[6]; break;
		case CPUINFO_INT_REGISTER + I386_DR7:			info->i = I.dr[7]; break;
		case CPUINFO_INT_REGISTER + I386_TR6:			info->i = I.tr[6]; break;
		case CPUINFO_INT_REGISTER + I386_TR7:			info->i = I.tr[7]; break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case CPUINFO_PTR_SET_INFO:	      				info->setinfo = i386_set_info;		break;
		case CPUINFO_PTR_GET_CONTEXT:					info->getcontext = i386_get_context;	break;
		case CPUINFO_PTR_SET_CONTEXT:					info->setcontext = i386_set_context;	break;
		case CPUINFO_PTR_INIT:		      				info->init = i386_init;			break;
		case CPUINFO_PTR_RESET:		      				info->reset = i386_reset;		break;
		case CPUINFO_PTR_EXECUTE:	      				info->execute = i386_execute;		break;
		case CPUINFO_PTR_BURN:		      				info->burn = NULL;			break;
		case CPUINFO_PTR_DISASSEMBLE_NEW:				info->disassemble_new = i386_dasm;		break;
		case CPUINFO_PTR_INSTRUCTION_COUNTER: 			info->icount = &I.cycles;		break;
		case CPUINFO_PTR_REGISTER_LAYOUT:				info->p = i386_reg_layout;		break;
		case CPUINFO_PTR_WINDOW_LAYOUT:					info->p = i386_win_layout;		break;
		case CPUINFO_PTR_TRANSLATE:						info->translate = translate_address_cb;	break;
#if defined(MAME_DEBUG) && defined(NEW_DEBUGGER)
		case CPUINFO_PTR_DEBUG_SETUP_COMMANDS:			info->setup_commands = i386_debug_setup; break;
#endif

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case CPUINFO_STR_NAME:							strcpy(info->s = cpuintrf_temp_str(), "I386"); break;
		case CPUINFO_STR_CORE_FAMILY:					strcpy(info->s = cpuintrf_temp_str(), "Intel 386"); break;
		case CPUINFO_STR_CORE_VERSION:					strcpy(info->s = cpuintrf_temp_str(), "1.0"); break;
		case CPUINFO_STR_CORE_FILE:						strcpy(info->s = cpuintrf_temp_str(), __FILE__); break;
		case CPUINFO_STR_CORE_CREDITS:					strcpy(info->s = cpuintrf_temp_str(), "Copyright (C) 2003-2004 Ville Linde"); break;

		case CPUINFO_STR_FLAGS:	   				sprintf(info->s = cpuintrf_temp_str(), "%08X", get_flags()); break;

		case CPUINFO_STR_REGISTER + I386_PC:			sprintf(info->s = cpuintrf_temp_str(), "PC: %08X", I.pc); break;
		case CPUINFO_STR_REGISTER + I386_EIP:			sprintf(info->s = cpuintrf_temp_str(), "EIP: %08X", I.eip); break;
		case CPUINFO_STR_REGISTER + I386_AL:			sprintf(info->s = cpuintrf_temp_str(), "~AL: %02X", REG8(AL)); break;
		case CPUINFO_STR_REGISTER + I386_AH:			sprintf(info->s = cpuintrf_temp_str(), "~AH: %02X", REG8(AH)); break;
		case CPUINFO_STR_REGISTER + I386_BL:			sprintf(info->s = cpuintrf_temp_str(), "~BL: %02X", REG8(BL)); break;
		case CPUINFO_STR_REGISTER + I386_BH:			sprintf(info->s = cpuintrf_temp_str(), "~BH: %02X", REG8(BH)); break;
		case CPUINFO_STR_REGISTER + I386_CL:			sprintf(info->s = cpuintrf_temp_str(), "~CL: %02X", REG8(CL)); break;
		case CPUINFO_STR_REGISTER + I386_CH:			sprintf(info->s = cpuintrf_temp_str(), "~CH: %02X", REG8(CH)); break;
		case CPUINFO_STR_REGISTER + I386_DL:			sprintf(info->s = cpuintrf_temp_str(), "~DL: %02X", REG8(DL)); break;
		case CPUINFO_STR_REGISTER + I386_DH:			sprintf(info->s = cpuintrf_temp_str(), "~DH: %02X", REG8(DH)); break;
		case CPUINFO_STR_REGISTER + I386_AX:			sprintf(info->s = cpuintrf_temp_str(), "~AX: %04X", REG16(AX)); break;
		case CPUINFO_STR_REGISTER + I386_BX:			sprintf(info->s = cpuintrf_temp_str(), "~BX: %04X", REG16(BX)); break;
		case CPUINFO_STR_REGISTER + I386_CX:			sprintf(info->s = cpuintrf_temp_str(), "~CX: %04X", REG16(CX)); break;
		case CPUINFO_STR_REGISTER + I386_DX:			sprintf(info->s = cpuintrf_temp_str(), "~DX: %04X", REG16(DX)); break;
		case CPUINFO_STR_REGISTER + I386_SI:			sprintf(info->s = cpuintrf_temp_str(), "~SI: %04X", REG16(SI)); break;
		case CPUINFO_STR_REGISTER + I386_DI:			sprintf(info->s = cpuintrf_temp_str(), "~DI: %04X", REG16(DI)); break;
		case CPUINFO_STR_REGISTER + I386_BP:			sprintf(info->s = cpuintrf_temp_str(), "~BP: %04X", REG16(BP)); break;
		case CPUINFO_STR_REGISTER + I386_SP:			sprintf(info->s = cpuintrf_temp_str(), "~SP: %04X", REG16(SP)); break;
		case CPUINFO_STR_REGISTER + I386_IP:			sprintf(info->s = cpuintrf_temp_str(), "~IP: %04X", I.eip & 0xFFFF); break;
		case CPUINFO_STR_REGISTER + I386_EAX:			sprintf(info->s = cpuintrf_temp_str(), "EAX: %08X", I.reg.d[EAX]); break;
		case CPUINFO_STR_REGISTER + I386_EBX:			sprintf(info->s = cpuintrf_temp_str(), "EBX: %08X", I.reg.d[EBX]); break;
		case CPUINFO_STR_REGISTER + I386_ECX:			sprintf(info->s = cpuintrf_temp_str(), "ECX: %08X", I.reg.d[ECX]); break;
		case CPUINFO_STR_REGISTER + I386_EDX:			sprintf(info->s = cpuintrf_temp_str(), "EDX: %08X", I.reg.d[EDX]); break;
		case CPUINFO_STR_REGISTER + I386_EBP:			sprintf(info->s = cpuintrf_temp_str(), "EBP: %08X", I.reg.d[EBP]); break;
		case CPUINFO_STR_REGISTER + I386_ESP:			sprintf(info->s = cpuintrf_temp_str(), "ESP: %08X", I.reg.d[ESP]); break;
		case CPUINFO_STR_REGISTER + I386_ESI:			sprintf(info->s = cpuintrf_temp_str(), "ESI: %08X", I.reg.d[ESI]); break;
		case CPUINFO_STR_REGISTER + I386_EDI:			sprintf(info->s = cpuintrf_temp_str(), "EDI: %08X", I.reg.d[EDI]); break;
		case CPUINFO_STR_REGISTER + I386_EFLAGS:		sprintf(info->s = cpuintrf_temp_str(), "EFLAGS: %08X", I.eflags); break;
		case CPUINFO_STR_REGISTER + I386_CS:			sprintf(info->s = cpuintrf_temp_str(), "CS: %04X:%08X", I.sreg[CS].selector, I.sreg[CS].base); break;
		case CPUINFO_STR_REGISTER + I386_SS:			sprintf(info->s = cpuintrf_temp_str(), "SS: %04X:%08X", I.sreg[SS].selector, I.sreg[SS].base); break;
		case CPUINFO_STR_REGISTER + I386_DS:			sprintf(info->s = cpuintrf_temp_str(), "DS: %04X:%08X", I.sreg[DS].selector, I.sreg[DS].base); break;
		case CPUINFO_STR_REGISTER + I386_ES:			sprintf(info->s = cpuintrf_temp_str(), "ES: %04X:%08X", I.sreg[ES].selector, I.sreg[ES].base); break;
		case CPUINFO_STR_REGISTER + I386_FS:			sprintf(info->s = cpuintrf_temp_str(), "FS: %04X:%08X", I.sreg[FS].selector, I.sreg[FS].base); break;
		case CPUINFO_STR_REGISTER + I386_GS:			sprintf(info->s = cpuintrf_temp_str(), "GS: %04X:%08X", I.sreg[GS].selector, I.sreg[GS].base); break;
		case CPUINFO_STR_REGISTER + I386_CR0:			sprintf(info->s = cpuintrf_temp_str(), "CR0: %08X", I.cr[0]); break;
		case CPUINFO_STR_REGISTER + I386_CR1:			sprintf(info->s = cpuintrf_temp_str(), "CR1: %08X", I.cr[1]); break;
		case CPUINFO_STR_REGISTER + I386_CR2:			sprintf(info->s = cpuintrf_temp_str(), "CR2: %08X", I.cr[2]); break;
		case CPUINFO_STR_REGISTER + I386_CR3:			sprintf(info->s = cpuintrf_temp_str(), "CR3: %08X", I.cr[3]); break;
		case CPUINFO_STR_REGISTER + I386_DR0:			sprintf(info->s = cpuintrf_temp_str(), "DR0: %08X", I.dr[0]); break;
		case CPUINFO_STR_REGISTER + I386_DR1:			sprintf(info->s = cpuintrf_temp_str(), "DR1: %08X", I.dr[1]); break;
		case CPUINFO_STR_REGISTER + I386_DR2:			sprintf(info->s = cpuintrf_temp_str(), "DR2: %08X", I.dr[2]); break;
		case CPUINFO_STR_REGISTER + I386_DR3:			sprintf(info->s = cpuintrf_temp_str(), "DR3: %08X", I.dr[3]); break;
		case CPUINFO_STR_REGISTER + I386_DR4:			sprintf(info->s = cpuintrf_temp_str(), "DR4: %08X", I.dr[4]); break;
		case CPUINFO_STR_REGISTER + I386_DR5:			sprintf(info->s = cpuintrf_temp_str(), "DR5: %08X", I.dr[5]); break;
		case CPUINFO_STR_REGISTER + I386_DR6:			sprintf(info->s = cpuintrf_temp_str(), "DR6: %08X", I.dr[6]); break;
		case CPUINFO_STR_REGISTER + I386_DR7:			sprintf(info->s = cpuintrf_temp_str(), "DR7: %08X", I.dr[7]); break;
		case CPUINFO_STR_REGISTER + I386_TR6:			sprintf(info->s = cpuintrf_temp_str(), "TR6: %08X", I.tr[6]); break;
		case CPUINFO_STR_REGISTER + I386_TR7:			sprintf(info->s = cpuintrf_temp_str(), "TR7: %08X", I.tr[7]); break;
	}
}

/*****************************************************************************/
/* Intel 486 */

#if (HAS_I486)

static UINT8 i486_reg_layout[] =
{
	I386_EIP,		I386_ESP,		-1,
	I386_EAX,		I386_EBP,		-1,
	I386_EBX,		I386_ESI,		-1,
	I386_ECX,		I386_EDI,		-1,
	I386_EDX,		-1,
	I386_CS,		I386_CR0,		-1,
	I386_SS,		I386_CR1,		-1,
	I386_DS,		I386_CR2,		-1,
	I386_ES,		I386_CR3,		-1,
	I386_FS,		I386_TR6,		-1,
	I386_GS,		I386_TR7,		-1,
	I386_DR0,		I386_DR1,		-1,
	I386_DR2,		I386_DR3,		-1,
	I386_DR4,		I386_DR5,		-1,
	I386_DR6,		I386_DR7,		-1,
	X87_CTRL,		X87_STATUS,		-1,
	X87_ST0,		X87_ST1,		-1,
	X87_ST2,		X87_ST3,		-1,
	X87_ST4,		X87_ST5,		-1,
	X87_ST6,		X87_ST7,		0
};

static UINT8 i486_win_layout[] =
{
	 0, 0,32,12,	/* register window (top rows) */
	33, 0,46,15,	/* disassembler window (left colums) */
	33,10,46,12,	/* memory #2 window (right, lower middle) */
	 0,19,32, 3,	/* memory #1 window (right, upper middle) */
	 0,23,80, 1,	/* command line window (bottom rows) */
};

void i486_init(int index, int clock, const void *config, int (*irqcallback)(int))
{
	i386_init(index, clock, config, irqcallback);
}

static void i486_reset(void)
{
	int (*save_irqcallback)(int);

	save_irqcallback = I.irq_callback;
	memset( &I, 0, sizeof(I386_REGS) );
	I.irq_callback = save_irqcallback;
	I.sreg[CS].selector = 0xf000;
	I.sreg[CS].base		= 0xffff0000;
	I.sreg[CS].limit	= 0xffff;

	I.idtr.base = 0;
	I.idtr.limit = 0x3ff;

	I.a20_mask = ~0;

	I.cr[0] = 0;
	I.eflags = 0;
	I.eip = 0xfff0;

	REG32(EAX) = 0x0308;	// Intel 386, stepping D1
	REG32(EDX) = 0;

	build_opcode_table(OP_I386 | OP_FPU | OP_I486);
	I.cycle_table_rm = cycle_table_rm[CPU_CYCLES_I486];
	I.cycle_table_pm = cycle_table_pm[CPU_CYCLES_I486];

	CHANGE_PC(I.eip);
}

static void i486_exit(void)
{

}

static void i486_set_info(UINT32 state, union cpuinfo *info)
{
	switch (state)
	{
		case CPUINFO_INT_REGISTER + X87_CTRL:			I.fpu_control_word = info->i; break;
		case CPUINFO_INT_REGISTER + X87_STATUS:			I.fpu_status_word = info->i; break;
		case CPUINFO_INT_REGISTER + X87_ST0:			ST(0).f = info->i; break;
		case CPUINFO_INT_REGISTER + X87_ST1:			ST(1).f = info->i; break;
		case CPUINFO_INT_REGISTER + X87_ST2:			ST(2).f = info->i; break;
		case CPUINFO_INT_REGISTER + X87_ST3:			ST(3).f = info->i; break;
		case CPUINFO_INT_REGISTER + X87_ST4:			ST(4).f = info->i; break;
		case CPUINFO_INT_REGISTER + X87_ST5:			ST(5).f = info->i; break;
		case CPUINFO_INT_REGISTER + X87_ST6:			ST(6).f = info->i; break;
		case CPUINFO_INT_REGISTER + X87_ST7:			ST(7).f = info->i; break;

		default: i386_set_info(state, info); break;
	}
}

void i486_get_info(UINT32 state, union cpuinfo *info)
{
	switch (state)
	{
		case CPUINFO_PTR_SET_INFO:	      				info->setinfo = i486_set_info;		break;
		case CPUINFO_PTR_INIT:		      				info->init = i486_init;				break;
		case CPUINFO_PTR_RESET:		      				info->reset = i486_reset;			break;
		case CPUINFO_PTR_EXIT:		      				info->exit = i486_exit;				break;
		case CPUINFO_PTR_REGISTER_LAYOUT:				info->p = i486_reg_layout;			break;
		case CPUINFO_PTR_WINDOW_LAYOUT:					info->p = i486_win_layout;			break;

		case CPUINFO_INT_REGISTER + X87_CTRL:			info->i = I.fpu_control_word; break;
		case CPUINFO_INT_REGISTER + X87_STATUS:			info->i = I.fpu_status_word; break;
		case CPUINFO_INT_REGISTER + X87_ST0:			info->i = ST(0).f; break;
		case CPUINFO_INT_REGISTER + X87_ST1:			info->i = ST(1).f; break;
		case CPUINFO_INT_REGISTER + X87_ST2:			info->i = ST(2).f; break;
		case CPUINFO_INT_REGISTER + X87_ST3:			info->i = ST(3).f; break;
		case CPUINFO_INT_REGISTER + X87_ST4:			info->i = ST(4).f; break;
		case CPUINFO_INT_REGISTER + X87_ST5:			info->i = ST(5).f; break;
		case CPUINFO_INT_REGISTER + X87_ST6:			info->i = ST(6).f; break;
		case CPUINFO_INT_REGISTER + X87_ST7:			info->i = ST(7).f; break;

		case CPUINFO_STR_NAME:							strcpy(info->s = cpuintrf_temp_str(), "I486"); break;
		case CPUINFO_STR_CORE_FAMILY:					strcpy(info->s = cpuintrf_temp_str(), "Intel 486"); break;
		case CPUINFO_STR_REGISTER + X87_CTRL:			sprintf(info->s = cpuintrf_temp_str(), "FPU_CW: %04X", I.fpu_control_word); break;
		case CPUINFO_STR_REGISTER + X87_STATUS:			sprintf(info->s = cpuintrf_temp_str(), "FPU_SW: %04X", I.fpu_status_word); break;
		case CPUINFO_STR_REGISTER + X87_ST0:			sprintf(info->s = cpuintrf_temp_str(), "ST0: %f", ST(0).f); break;
		case CPUINFO_STR_REGISTER + X87_ST1:			sprintf(info->s = cpuintrf_temp_str(), "ST1: %f", ST(1).f); break;
		case CPUINFO_STR_REGISTER + X87_ST2:			sprintf(info->s = cpuintrf_temp_str(), "ST2: %f", ST(2).f); break;
		case CPUINFO_STR_REGISTER + X87_ST3:			sprintf(info->s = cpuintrf_temp_str(), "ST3: %f", ST(3).f); break;
		case CPUINFO_STR_REGISTER + X87_ST4:			sprintf(info->s = cpuintrf_temp_str(), "ST4: %f", ST(4).f); break;
		case CPUINFO_STR_REGISTER + X87_ST5:			sprintf(info->s = cpuintrf_temp_str(), "ST5: %f", ST(5).f); break;
		case CPUINFO_STR_REGISTER + X87_ST6:			sprintf(info->s = cpuintrf_temp_str(), "ST6: %f", ST(6).f); break;
		case CPUINFO_STR_REGISTER + X87_ST7:			sprintf(info->s = cpuintrf_temp_str(), "ST7: %f", ST(7).f); break;

		default:	i386_get_info(state, info); break;
	}
}
#endif

/*****************************************************************************/
/* Pentium */

#if (HAS_PENTIUM)

static UINT8 pentium_reg_layout[] =
{
	I386_EIP,		I386_ESP,		-1,
	I386_EAX,		I386_EBP,		-1,
	I386_EBX,		I386_ESI,		-1,
	I386_ECX,		I386_EDI,		-1,
	I386_EDX,		-1,
	I386_CS,		I386_CR0,		-1,
	I386_SS,		I386_CR1,		-1,
	I386_DS,		I386_CR2,		-1,
	I386_ES,		I386_CR3,		-1,
	I386_FS,		I386_TR6,		-1,
	I386_GS,		I386_TR7,		-1,
	I386_DR0,		I386_DR1,		-1,
	I386_DR2,		I386_DR3,		-1,
	I386_DR4,		I386_DR5,		-1,
	I386_DR6,		I386_DR7,		-1,
	X87_CTRL,		X87_STATUS,		-1,
	X87_ST0,		X87_ST1,		-1,
	X87_ST2,		X87_ST3,		-1,
	X87_ST4,		X87_ST5,		-1,
	X87_ST6,		X87_ST7,		0
};

static UINT8 pentium_win_layout[] =
{
	 0, 0,32,12,	/* register window (top rows) */
	33, 0,46,15,	/* disassembler window (left colums) */
	33,10,46,12,	/* memory #2 window (right, lower middle) */
	 0,19,32, 3,	/* memory #1 window (right, upper middle) */
	 0,23,80, 1,	/* command line window (bottom rows) */
};

void pentium_init(int index, int clock, const void *config, int (*irqcallback)(int))
{
	i386_init(index, clock, config, irqcallback);
}

static void pentium_reset(void)
{
	int (*save_irqcallback)(int);

	save_irqcallback = I.irq_callback;
	memset( &I, 0, sizeof(I386_REGS) );
	I.irq_callback = save_irqcallback;
	I.sreg[CS].selector = 0xf000;
	I.sreg[CS].base		= 0xffff0000;
	I.sreg[CS].limit	= 0xffff;

	I.idtr.base = 0;
	I.idtr.limit = 0x3ff;

	I.a20_mask = ~0;

	I.cr[0] = 0;
	I.eflags = 0;
	I.eip = 0xfff0;

	REG32(EAX) = 0x0308;	// Intel 386, stepping D1
	REG32(EDX) = 0;

	build_opcode_table(OP_I386 | OP_FPU | OP_I486 | OP_PENTIUM);
	I.cycle_table_rm = cycle_table_rm[CPU_CYCLES_PENTIUM];
	I.cycle_table_pm = cycle_table_pm[CPU_CYCLES_PENTIUM];

	I.cpuid_id0 = 0x756e6547;	// Genu
	I.cpuid_id1 = 0x49656e69;	// ineI
	I.cpuid_id2 = 0x6c65746e;	// ntel

	I.cpuid_max_input_value_eax = 0x01;

	// [11:8] Family
	// [ 7:4] Model
	// [ 3:0] Stepping ID
	// Family 5 (Pentium), Model 2 (75 - 200MHz), Stepping 1
	I.cpu_version = (5 << 8) | (2 << 4) | (1);

	// [ 0:0] FPU on chip
	// [ 2:2] I/O breakpoints
	// [ 4:4] Time Stamp Counter
	// [ 5:5] Pentium CPU style model specific registers
	// [ 7:7] Machine Check Exception
	// [ 8:8] CMPXCHG8B instruction
	I.feature_flags = 0x00000000;

	CHANGE_PC(I.eip);
}

static void pentium_exit(void)
{

}

static void pentium_set_info(UINT32 state, union cpuinfo *info)
{
	switch (state)
	{
		case CPUINFO_INT_REGISTER + X87_CTRL:			I.fpu_control_word = info->i; break;
		case CPUINFO_INT_REGISTER + X87_STATUS:			I.fpu_status_word = info->i; break;
		case CPUINFO_INT_REGISTER + X87_ST0:			ST(0).f = info->i; break;
		case CPUINFO_INT_REGISTER + X87_ST1:			ST(1).f = info->i; break;
		case CPUINFO_INT_REGISTER + X87_ST2:			ST(2).f = info->i; break;
		case CPUINFO_INT_REGISTER + X87_ST3:			ST(3).f = info->i; break;
		case CPUINFO_INT_REGISTER + X87_ST4:			ST(4).f = info->i; break;
		case CPUINFO_INT_REGISTER + X87_ST5:			ST(5).f = info->i; break;
		case CPUINFO_INT_REGISTER + X87_ST6:			ST(6).f = info->i; break;
		case CPUINFO_INT_REGISTER + X87_ST7:			ST(7).f = info->i; break;

		default: i386_set_info(state, info); break;
	}
}

void pentium_get_info(UINT32 state, union cpuinfo *info)
{
	switch (state)
	{
		case CPUINFO_PTR_SET_INFO:	      				info->setinfo = pentium_set_info;		break;
		case CPUINFO_PTR_INIT:		      				info->init = pentium_init;				break;
		case CPUINFO_PTR_RESET:		      				info->reset = pentium_reset;			break;
		case CPUINFO_PTR_EXIT:		      				info->exit = pentium_exit;				break;
		case CPUINFO_PTR_REGISTER_LAYOUT:				info->p = pentium_reg_layout;			break;
		case CPUINFO_PTR_WINDOW_LAYOUT:					info->p = pentium_win_layout;			break;

		case CPUINFO_INT_REGISTER + X87_CTRL:			info->i = I.fpu_control_word; break;
		case CPUINFO_INT_REGISTER + X87_STATUS:			info->i = I.fpu_status_word; break;
		case CPUINFO_INT_REGISTER + X87_ST0:			info->i = ST(0).f; break;
		case CPUINFO_INT_REGISTER + X87_ST1:			info->i = ST(1).f; break;
		case CPUINFO_INT_REGISTER + X87_ST2:			info->i = ST(2).f; break;
		case CPUINFO_INT_REGISTER + X87_ST3:			info->i = ST(3).f; break;
		case CPUINFO_INT_REGISTER + X87_ST4:			info->i = ST(4).f; break;
		case CPUINFO_INT_REGISTER + X87_ST5:			info->i = ST(5).f; break;
		case CPUINFO_INT_REGISTER + X87_ST6:			info->i = ST(6).f; break;
		case CPUINFO_INT_REGISTER + X87_ST7:			info->i = ST(7).f; break;

		case CPUINFO_STR_NAME:							strcpy(info->s = cpuintrf_temp_str(), "PENTIUM"); break;
		case CPUINFO_STR_CORE_FAMILY:					strcpy(info->s = cpuintrf_temp_str(), "Intel Pentium"); break;
		case CPUINFO_STR_REGISTER + X87_CTRL:			sprintf(info->s = cpuintrf_temp_str(), "FPU_CW: %04X", I.fpu_control_word); break;
		case CPUINFO_STR_REGISTER + X87_STATUS:			sprintf(info->s = cpuintrf_temp_str(), "FPU_SW: %04X", I.fpu_status_word); break;
		case CPUINFO_STR_REGISTER + X87_ST0:			sprintf(info->s = cpuintrf_temp_str(), "ST0: %f", ST(0).f); break;
		case CPUINFO_STR_REGISTER + X87_ST1:			sprintf(info->s = cpuintrf_temp_str(), "ST1: %f", ST(1).f); break;
		case CPUINFO_STR_REGISTER + X87_ST2:			sprintf(info->s = cpuintrf_temp_str(), "ST2: %f", ST(2).f); break;
		case CPUINFO_STR_REGISTER + X87_ST3:			sprintf(info->s = cpuintrf_temp_str(), "ST3: %f", ST(3).f); break;
		case CPUINFO_STR_REGISTER + X87_ST4:			sprintf(info->s = cpuintrf_temp_str(), "ST4: %f", ST(4).f); break;
		case CPUINFO_STR_REGISTER + X87_ST5:			sprintf(info->s = cpuintrf_temp_str(), "ST5: %f", ST(5).f); break;
		case CPUINFO_STR_REGISTER + X87_ST6:			sprintf(info->s = cpuintrf_temp_str(), "ST6: %f", ST(6).f); break;
		case CPUINFO_STR_REGISTER + X87_ST7:			sprintf(info->s = cpuintrf_temp_str(), "ST7: %f", ST(7).f); break;

		default:	i386_get_info(state, info); break;
	}
}
#endif

/*****************************************************************************/
/* Cyrix MediaGX */

#if (HAS_MEDIAGX)

static UINT8 mediagx_reg_layout[] =
{
	I386_EIP,		I386_ESP,		-1,
	I386_EAX,		I386_EBP,		-1,
	I386_EBX,		I386_ESI,		-1,
	I386_ECX,		I386_EDI,		-1,
	I386_EDX,		-1,
	I386_CS,		I386_CR0,		-1,
	I386_SS,		I386_CR1,		-1,
	I386_DS,		I386_CR2,		-1,
	I386_ES,		I386_CR3,		-1,
	I386_FS,		I386_TR6,		-1,
	I386_GS,		I386_TR7,		-1,
	I386_DR0,		I386_DR1,		-1,
	I386_DR2,		I386_DR3,		-1,
	I386_DR4,		I386_DR5,		-1,
	I386_DR6,		I386_DR7,		-1,
	X87_CTRL,		X87_STATUS,		-1,
	X87_ST0,		X87_ST1,		-1,
	X87_ST2,		X87_ST3,		-1,
	X87_ST4,		X87_ST5,		-1,
	X87_ST6,		X87_ST7,		0
};

static UINT8 mediagx_win_layout[] =
{
	 0, 0,32,12,	/* register window (top rows) */
	33, 0,46,15,	/* disassembler window (left colums) */
	33,10,46,12,	/* memory #2 window (right, lower middle) */
	 0,19,32, 3,	/* memory #1 window (right, upper middle) */
	 0,23,80, 1,	/* command line window (bottom rows) */
};

void mediagx_init(int index, int clock, const void *config, int (*irqcallback)(int))
{
	i386_init(index, clock, config, irqcallback);
}

static void mediagx_reset(void)
{
	int (*save_irqcallback)(int);

	save_irqcallback = I.irq_callback;
	memset( &I, 0, sizeof(I386_REGS) );
	I.irq_callback = save_irqcallback;
	I.sreg[CS].selector = 0xf000;
	I.sreg[CS].base		= 0xffff0000;
	I.sreg[CS].limit	= 0xffff;

	I.idtr.base = 0;
	I.idtr.limit = 0x3ff;

	I.a20_mask = ~0;

	I.cr[0] = 0;
	I.eflags = 0;
	I.eip = 0xfff0;

	REG32(EAX) = 0x0308;	// Intel 386, stepping D1
	REG32(EDX) = 0;

	build_opcode_table(OP_I386 | OP_FPU | OP_I486 | OP_PENTIUM | OP_CYRIX);
	I.cycle_table_rm = cycle_table_rm[CPU_CYCLES_MEDIAGX];
	I.cycle_table_pm = cycle_table_pm[CPU_CYCLES_MEDIAGX];

	I.cpuid_id0 = 0x69727943;	// Cyri
	I.cpuid_id1 = 0x736e4978;	// xIns
	I.cpuid_id2 = 0x6d616574;	// tead

	I.cpuid_max_input_value_eax = 0x01;

	// [11:8] Family
	// [ 7:4] Model
	// [ 3:0] Stepping ID
	// Family 4, Model 4 (MediaGX)
	I.cpu_version = (4 << 8) | (4 << 4) | (1);

	// [ 0:0] FPU on chip
	// [ 2:2] I/O breakpoints
	// [ 4:4] Time Stamp Counter
	// [ 5:5] Pentium CPU style model specific registers
	// [ 7:7] Machine Check Exception
	// [ 8:8] CMPXCHG8B instruction
	I.feature_flags = 0x00000001;

	CHANGE_PC(I.eip);
}

static void mediagx_exit(void)
{

}

static void mediagx_set_info(UINT32 state, union cpuinfo *info)
{
	switch (state)
	{
		case CPUINFO_INT_REGISTER + X87_CTRL:			I.fpu_control_word = info->i; break;
		case CPUINFO_INT_REGISTER + X87_STATUS:			I.fpu_status_word = info->i; break;
		case CPUINFO_INT_REGISTER + X87_ST0:			ST(0).f = info->i; break;
		case CPUINFO_INT_REGISTER + X87_ST1:			ST(1).f = info->i; break;
		case CPUINFO_INT_REGISTER + X87_ST2:			ST(2).f = info->i; break;
		case CPUINFO_INT_REGISTER + X87_ST3:			ST(3).f = info->i; break;
		case CPUINFO_INT_REGISTER + X87_ST4:			ST(4).f = info->i; break;
		case CPUINFO_INT_REGISTER + X87_ST5:			ST(5).f = info->i; break;
		case CPUINFO_INT_REGISTER + X87_ST6:			ST(6).f = info->i; break;
		case CPUINFO_INT_REGISTER + X87_ST7:			ST(7).f = info->i; break;

		default: i386_set_info(state, info); break;
	}
}

void mediagx_get_info(UINT32 state, union cpuinfo *info)
{
	switch (state)
	{
		case CPUINFO_PTR_SET_INFO:	      				info->setinfo = mediagx_set_info;		break;
		case CPUINFO_PTR_INIT:		      				info->init = mediagx_init;				break;
		case CPUINFO_PTR_RESET:		      				info->reset = mediagx_reset;			break;
		case CPUINFO_PTR_EXIT:		      				info->exit = mediagx_exit;				break;
		case CPUINFO_PTR_REGISTER_LAYOUT:				info->p = mediagx_reg_layout;			break;
		case CPUINFO_PTR_WINDOW_LAYOUT:					info->p = mediagx_win_layout;			break;

		case CPUINFO_INT_REGISTER + X87_CTRL:			info->i = I.fpu_control_word; break;
		case CPUINFO_INT_REGISTER + X87_STATUS:			info->i = I.fpu_status_word; break;
		case CPUINFO_INT_REGISTER + X87_ST0:			info->i = ST(0).f; break;
		case CPUINFO_INT_REGISTER + X87_ST1:			info->i = ST(1).f; break;
		case CPUINFO_INT_REGISTER + X87_ST2:			info->i = ST(2).f; break;
		case CPUINFO_INT_REGISTER + X87_ST3:			info->i = ST(3).f; break;
		case CPUINFO_INT_REGISTER + X87_ST4:			info->i = ST(4).f; break;
		case CPUINFO_INT_REGISTER + X87_ST5:			info->i = ST(5).f; break;
		case CPUINFO_INT_REGISTER + X87_ST6:			info->i = ST(6).f; break;
		case CPUINFO_INT_REGISTER + X87_ST7:			info->i = ST(7).f; break;

		case CPUINFO_STR_NAME:							strcpy(info->s = cpuintrf_temp_str(), "MEDIAGX"); break;
		case CPUINFO_STR_CORE_FAMILY:					strcpy(info->s = cpuintrf_temp_str(), "Cyrix MediaGX"); break;
		case CPUINFO_STR_REGISTER + X87_CTRL:			sprintf(info->s = cpuintrf_temp_str(), "FPU_CW: %04X", I.fpu_control_word); break;
		case CPUINFO_STR_REGISTER + X87_STATUS:			sprintf(info->s = cpuintrf_temp_str(), "FPU_SW: %04X", I.fpu_status_word); break;
		case CPUINFO_STR_REGISTER + X87_ST0:			sprintf(info->s = cpuintrf_temp_str(), "ST0: %f", ST(0).f); break;
		case CPUINFO_STR_REGISTER + X87_ST1:			sprintf(info->s = cpuintrf_temp_str(), "ST1: %f", ST(1).f); break;
		case CPUINFO_STR_REGISTER + X87_ST2:			sprintf(info->s = cpuintrf_temp_str(), "ST2: %f", ST(2).f); break;
		case CPUINFO_STR_REGISTER + X87_ST3:			sprintf(info->s = cpuintrf_temp_str(), "ST3: %f", ST(3).f); break;
		case CPUINFO_STR_REGISTER + X87_ST4:			sprintf(info->s = cpuintrf_temp_str(), "ST4: %f", ST(4).f); break;
		case CPUINFO_STR_REGISTER + X87_ST5:			sprintf(info->s = cpuintrf_temp_str(), "ST5: %f", ST(5).f); break;
		case CPUINFO_STR_REGISTER + X87_ST6:			sprintf(info->s = cpuintrf_temp_str(), "ST6: %f", ST(6).f); break;
		case CPUINFO_STR_REGISTER + X87_ST7:			sprintf(info->s = cpuintrf_temp_str(), "ST7: %f", ST(7).f); break;

		default:	i386_get_info(state, info); break;
	}
}
#endif
