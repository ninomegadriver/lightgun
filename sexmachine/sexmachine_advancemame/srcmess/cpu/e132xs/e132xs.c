/********************************************************************
 Hyperstone cpu emulator
 written by Pierpaolo Prazzoli

 All the types are compatible, but they have different IRAM size and cycles

 Hyperstone models:

 16 bits
 - E1-16T
 - E1-16XT
 - E1-16XS
 - E1-16XSR

 32bits
 - E1-32N   or  E1-32T
 - E1-32XN  or  E1-32XT
 - E1-32XS
 - E1-32XSR

 Hynix models:

 16 bits
 - GMS30C2116
 - GMS30C2216

 32bits
 - GMS30C2132
 - GMS30C2232

 TODO:
 - some wrong cycle counts
 - support more than 1 cpu when it'll be needed

 CHANGELOG:

 Pierpaolo Prazzoli
 - Fixed LDxx.N/P/S opcodes not to increment the destination register when
   it's the same as the source or "next source" one.

 Pierpaolo Prazzoli
 - Removed nested delays
 - Added better delay branch support
 - Fixed PC seen by a delay instruction, because a delay instruction
   should use the delayed PC (thus allowing the execution of software
   opcodes too)

 Tomasz Slanina
 - Fixed delayed branching for delay instructions longer than 2 bytes

 Pierpaolo Prazzoli
 - Added and fixed Timer without hack

 Tomasz Slanina
 - Fixed MULU/MULS
 - Fixed Carry in ADDC/SUBC

 Pierpaolo Prazzoli
 - Fixed software opcodes used as delay instructions
 - Added nested delays

 Tomasz Slanina
 - Added "undefined" C flag to shift left instructions

 Pierpaolo Prazzoli
 - Added interrupts-block for delay instructions
 - Fixed get_emu_code_addr
 - Added LDW.S and STW.S instructions
 - Fixed floating point opcodes

 Tomasz Slanina
 - interrputs after call and before frame are prohibited now
 - emulation of FCR register
 - Floating point opcodes (preliminary)
 - Fixed stack addressing in RET/FRAME opcodes
 - Fixed bug in SET_RS macro
 - Fixed bug in return opcode (S flag)
 - Added C/N flags calculation in add/adc/addi/adds/addsi and some shift opcodes
 - Added writeback to ROL
 - Fixed ROL/SAR/SARD/SHR/SHRD/SHL/SHLD opcode decoding (Local/Global regs)
 - Fixed I and T flag in RET opcode
 - Fixed XX/XM opcodes
 - Fixed MOV opcode, when RD = PC
 - Fixed execute_trap()
 - Fixed ST opcodes, when when RS = SR
 - Added interrupts
 - Fixed I/O addressing

 Pierpaolo Prazzoli
 - Fixed fetch
 - Fixed decode of hyperstone_xm opcode
 - Fixed 7 bits difference number in FRAME / RET instructions
 - Some debbugger fixes
 - Added generic registers decode function
 - Some other little fixes.

 MooglyGuy 29/03/2004
    - Changed MOVI to use unsigned values instead of signed, correcting
      an ugly glitch when loading 32-bit immediates.
 Pierpaolo Prazzoli
    - Same fix in get_const

 MooglyGuy - 02/27/04
    - Fixed delayed branching
    - const_val for CALL should always have bit 0 clear

 Pierpaolo Prazzoli - 02/25/04
    - Fixed some wrong addresses to address local registers instead of memory
    - Fixed FRAME and RET instruction
    - Added preliminary I/O space
    - Fixed some load / store instructions

 Pierpaolo Prazzoli - 02/20/04
    - Added execute_exception function
    - Added FL == 0 always interpreted as 16

 Pierpaolo Prazzoli - 02/19/04
    - Changed the reset to use the execute_trap(reset) which should be right to set
      the initiale state of the cpu
    - Added Trace exception
    - Set of T flag in RET instruction
    - Set I flag in interrupts entries and resetted by a RET instruction
    - Added correct set instruction for SR

 Pierpaolo Prazzoli - 10/26/03
    - Changed get_lrconst to get_const and changed it to use the removed GET_CONST_RR
      macro.
    - Removed the High flag used in some opcodes, it should be used only in
      MOV and MOVI instruction.
    - Fixed MOV and MOVI instruction.
    - Set to 1 FP is SR register at reset.
      (From the doc: A Call, Trap or Software instruction increments the FP and sets FL
      to 6, thus creating a new stack frame with the length of 6 registers).

 MooglyGuy - 10/25/03
    - Fixed CALL enough that it at least jumps to the right address, no word
      yet as to whether or not it's working enough to return.
    - Added get_lrconst() to get the const value for the CALL operand, since
      apparently using immediate_value() was wrong. The code is ugly, but it
      works properly. Vampire 1/2 now gets far enough to try to test its RAM.
    - Just from looking at it, CALL apparently doesn't frame properly. I'm not
      sure about FRAME, but perhaps it doesn't work properly - I'm not entirely
      positive. The return address when vamphalf's memory check routine is
      called at FFFFFD7E is stored in register L8, and then the RET instruction
      at the end of the routine uses L1 as the return address, so that might
      provide some clues as to how it works.
    - I'd almost be willing to bet money that there's no framing at all since
      the values in L0 - L15 as displayed by the debugger would change during a
      CALL or FRAME operation. I'll look when I'm in the mood.
    - The mood struck me, and I took a look at SET_L_REG and GET_L_REG.
      Apparently no matter what the current frame pointer is they'll always use
      local_regs[0] through local_regs[15].

 MooglyGuy - 08/20/03
    - Added H flag support for MOV and MOVI
    - Changed init routine to set S flag on boot. Apparently the CPU defaults to
      supervisor mode as opposed to user mode when it powers on, as shown by the
      vamphalf power-on routines. Makes sense, too, since if the machine booted
      in user mode, it would be impossible to get into supervisor mode.

 Pierpaolo Prazzoli - 08/19/03
    - Added check for D_BIT and S_BIT where PC or SR must or must not be denoted.
      (movd, divu, divs, ldxx1, ldxx2, stxx1, stxx2, mulu, muls, set, mul
      call, chk)

 MooglyGuy - 08/17/03
    - Working on support for H flag, nothing quite done yet
    - Added trap Range Error for CHK PC, PC
    - Fixed relative jumps, they have to be taken from the opcode following the
      jump minstead of the jump opcode itself.

 Pierpaolo Prazzoli - 08/17/03
    - Fixed get_pcrel() when OP & 0x80 is set.
    - Decremented PC by 2 also in MOV, ADD, ADDI, SUM, SUB and added the check if
      D_BIT is not set. (when pc is changed they are implicit branch)

 MooglyGuy - 08/17/03
    - Implemented a crude hack to set FL in the SR to 6, since according to the docs
      that's supposed to happen each time a trap occurs, apparently including when
      the processor starts up. The 3rd opcode executed in vamphalf checks to see if
      the FL flag in SR 6, so it's apparently the "correct" behaviour despite the
      docs not saying anything on it. If FL is not 6, the branch falls through and
      encounters a CHK PC, L2, which at that point will always throw a range trap.
      The range trap vector contains 00000000 (CHK PC, PC), which according to the
      docs will always throw a range trap (which would effectively lock the system).
      This revealed a bug: CHK PC, PC apparently does not throw a range trap, which
      needs to be fixed. Now that the "correct" behaviour is hacked in with the FL
      flags, it reveals yet another bug in that the branch is interpreted as being
      +0x8700. This means that the PC then wraps around to 000082B0, give or take
      a few bytes. While it does indeed branch to valid code, I highly doubt that
      this is the desired effect. Check for signed/unsigned relative branch, maybe?

 MooglyGuy - 08/16/03
    - Fixed the debugger at least somewhat so that it displays hex instead of decimal,
      and so that it disassembles opcodes properly.
    - Fixed hyperstone_execute() to increment PC *after* executing the opcode instead of
      before. This is probably why vamphalf was booting to fffffff8, but executing at
      fffffffa instead.
    - Changed execute_trap to decrement PC by 2 so that the next opcode isn't skipped
      after a trap
    - Changed execute_br to decrement PC by 2 so that the next opcode isn't skipped
      after a branch
    - Changed hyperstone_movi to decrement PC by 2 when G0 (PC) is modified so that the
      next opcode isn't skipped after a branch
    - Changed hyperstone_movi to default to a UINT32 being moved into the register
      as opposed to a UINT8. This is wrong, the bit width is quite likely to be
      dependent on the n field in the Rimm instruction type. However, vamphalf uses
      MOVI G0,[FFFF]FBAC (n=$13) since there's apparently no absolute branch opcode.
      What kind of CPU is this that it doesn't have an absolute jump in its branch
      instructions and you have to use an immediate MOV to do an abs. jump!?
    - Replaced usage of logerror() with smf's verboselog()

*********************************************************************/

#include "usrintrf.h"
#include "debugger.h"
#include "e132xs.h"

static UINT8  (*hyp_cpu_read_byte)(offs_t address);
static UINT16 (*hyp_cpu_read_half_word)(offs_t address);
static UINT32 (*hyp_cpu_read_word)(offs_t address);
static UINT32 (*hyp_cpu_read_io_word)(offs_t address);
static void (*hyp_cpu_write_byte)(offs_t address, UINT8 data);
static void (*hyp_cpu_write_half_word)(offs_t address, UINT16 data);
static void (*hyp_cpu_write_word)(offs_t address, UINT32 data);
static void (*hyp_cpu_write_io_word)(offs_t address, UINT32 data);
int hyp_type_16bit;

// set C in adds/addsi/subs/sums
#define SETCARRYS 0
#define MISSIONCRAFT_FLAGS 1

static int hyperstone_ICount;

static void hyperstone_chk(void);
static void hyperstone_movd(void);
static void hyperstone_divu(void);
static void hyperstone_divs(void);
static void hyperstone_xm(void);
static void hyperstone_mask(void);
static void hyperstone_sum(void);
static void hyperstone_sums(void);
static void hyperstone_cmp(void);
static void hyperstone_mov(void);
static void hyperstone_add(void);
static void hyperstone_adds(void);
static void hyperstone_cmpb(void);
static void hyperstone_andn(void);
static void hyperstone_or(void);
static void hyperstone_xor(void);
static void hyperstone_subc(void);
static void hyperstone_not(void);
static void hyperstone_sub(void);
static void hyperstone_subs(void);
static void hyperstone_addc(void);
static void hyperstone_and(void);
static void hyperstone_neg(void);
static void hyperstone_negs(void);
static void hyperstone_cmpi(void);
static void hyperstone_movi(void);
static void hyperstone_addi(void);
static void hyperstone_addsi(void);
static void hyperstone_cmpbi(void);
static void hyperstone_andni(void);
static void hyperstone_ori(void);
static void hyperstone_xori(void);
static void hyperstone_shrdi(void);
static void hyperstone_shrd(void);
static void hyperstone_shr(void);
static void hyperstone_sardi(void);
static void hyperstone_sard(void);
static void hyperstone_sar(void);
static void hyperstone_shldi(void);
static void hyperstone_shld(void);
static void hyperstone_shl(void);
static void reserved(void);
static void hyperstone_testlz(void);
static void hyperstone_rol(void);
static void hyperstone_ldxx1(void);
static void hyperstone_ldxx2(void);
static void hyperstone_stxx1(void);
static void hyperstone_stxx2(void);
static void hyperstone_shri(void);
static void hyperstone_sari(void);
static void hyperstone_shli(void);
static void hyperstone_mulu(void);
static void hyperstone_muls(void);
static void hyperstone_set(void);
static void hyperstone_mul(void);
static void hyperstone_fadd(void);
static void hyperstone_faddd(void);
static void hyperstone_fsub(void);
static void hyperstone_fsubd(void);
static void hyperstone_fmul(void);
static void hyperstone_fmuld(void);
static void hyperstone_fdiv(void);
static void hyperstone_fdivd(void);
static void hyperstone_fcmp(void);
static void hyperstone_fcmpd(void);
static void hyperstone_fcmpu(void);
static void hyperstone_fcmpud(void);
static void hyperstone_fcvt(void);
static void hyperstone_fcvtd(void);
static void hyperstone_extend(void);
static void hyperstone_do(void);
static void hyperstone_ldwr(void);
static void hyperstone_lddr(void);
static void hyperstone_ldwp(void);
static void hyperstone_lddp(void);
static void hyperstone_stwr(void);
static void hyperstone_stdr(void);
static void hyperstone_stwp(void);
static void hyperstone_stdp(void);
static void hyperstone_dbv(void);
static void hyperstone_dbnv(void);
static void hyperstone_dbe(void);
static void hyperstone_dbne(void);
static void hyperstone_dbc(void);
static void hyperstone_dbnc(void);
static void hyperstone_dbse(void);
static void hyperstone_dbht(void);
static void hyperstone_dbn(void);
static void hyperstone_dbnn(void);
static void hyperstone_dble(void);
static void hyperstone_dbgt(void);
static void hyperstone_dbr(void);
static void hyperstone_frame(void);
static void hyperstone_call(void);
static void hyperstone_bv(void);
static void hyperstone_bnv(void);
static void hyperstone_be(void);
static void hyperstone_bne(void);
static void hyperstone_bc(void);
static void hyperstone_bnc(void);
static void hyperstone_bse(void);
static void hyperstone_bht(void);
static void hyperstone_bn(void);
static void hyperstone_bnn(void);
static void hyperstone_ble(void);
static void hyperstone_bgt(void);
static void hyperstone_br(void);
static void hyperstone_trap(void);

/* Registers */

enum
{
	E132XS_PC = 1,
	E132XS_SR,
	E132XS_FER,
	E132XS_G3,
	E132XS_G4,
	E132XS_G5,
	E132XS_G6,
	E132XS_G7,
	E132XS_G8,
	E132XS_G9,
	E132XS_G10,
	E132XS_G11,
	E132XS_G12,
	E132XS_G13,
	E132XS_G14,
	E132XS_G15,
	E132XS_G16,
	E132XS_G17,
	E132XS_SP,
	E132XS_UB,
	E132XS_BCR,
	E132XS_TPR,
	E132XS_TCR,
	E132XS_TR,
	E132XS_WCR,
	E132XS_ISR,
	E132XS_FCR,
	E132XS_MCR,
	E132XS_G28,
	E132XS_G29,
	E132XS_G30,
	E132XS_G31,
	E132XS_CL0, E132XS_CL1, E132XS_CL2, E132XS_CL3,
	E132XS_CL4, E132XS_CL5, E132XS_CL6, E132XS_CL7,
	E132XS_CL8, E132XS_CL9, E132XS_CL10,E132XS_CL11,
	E132XS_CL12,E132XS_CL13,E132XS_CL14,E132XS_CL15,
	E132XS_L0,  E132XS_L1,  E132XS_L2,  E132XS_L3,
	E132XS_L4,  E132XS_L5,  E132XS_L6,  E132XS_L7,
	E132XS_L8,  E132XS_L9,  E132XS_L10, E132XS_L11,
	E132XS_L12, E132XS_L13, E132XS_L14, E132XS_L15,
	E132XS_L16, E132XS_L17, E132XS_L18, E132XS_L19,
	E132XS_L20, E132XS_L21, E132XS_L22, E132XS_L23,
	E132XS_L24, E132XS_L25, E132XS_L26, E132XS_L27,
	E132XS_L28, E132XS_L29, E132XS_L30, E132XS_L31,
	E132XS_L32, E132XS_L33, E132XS_L34, E132XS_L35,
	E132XS_L36, E132XS_L37, E132XS_L38, E132XS_L39,
	E132XS_L40, E132XS_L41, E132XS_L42, E132XS_L43,
	E132XS_L44, E132XS_L45, E132XS_L46, E132XS_L47,
	E132XS_L48, E132XS_L49, E132XS_L50, E132XS_L51,
	E132XS_L52, E132XS_L53, E132XS_L54, E132XS_L55,
	E132XS_L56, E132XS_L57, E132XS_L58, E132XS_L59,
	E132XS_L60, E132XS_L61, E132XS_L62, E132XS_L63
};

static UINT8 hyperstone_reg_layout[] =
{
	E132XS_PC,  E132XS_SR,  E132XS_FER, E132XS_G3,  -1,
	E132XS_G4,  E132XS_G5,  E132XS_G6,  E132XS_G7,  -1,
	E132XS_G8,  E132XS_G9,  E132XS_G10, E132XS_G11, -1,
	E132XS_G12, E132XS_G13, E132XS_G14,	E132XS_G15, -1,
	E132XS_G16,	E132XS_G17,	E132XS_SP,	E132XS_UB,  -1,
	E132XS_BCR,	E132XS_TPR,	E132XS_TCR,	E132XS_TR,  -1,
	E132XS_WCR,	E132XS_ISR,	E132XS_FCR,	E132XS_MCR, -1,
	E132XS_G28, E132XS_G29,	E132XS_G30,	E132XS_G31, -1,
	E132XS_CL0, E132XS_CL1, E132XS_CL2, E132XS_CL3, -1,
	E132XS_CL4, E132XS_CL5, E132XS_CL6, E132XS_CL7, -1,
	E132XS_CL8, E132XS_CL9, E132XS_CL10,E132XS_CL11,-1,
	E132XS_CL12,E132XS_CL13,E132XS_CL14,E132XS_CL15,-1,
	E132XS_L0,  E132XS_L1,  E132XS_L2,  E132XS_L3,  -1,
	E132XS_L4,  E132XS_L5,  E132XS_L6,  E132XS_L7,  -1,
	E132XS_L8,  E132XS_L9,  E132XS_L10, E132XS_L11, -1,
	E132XS_L12, E132XS_L13, E132XS_L14, E132XS_L15, -1,
	E132XS_L16, E132XS_L17, E132XS_L18, E132XS_L19, -1,
	E132XS_L20, E132XS_L21, E132XS_L22, E132XS_L23, -1,
	E132XS_L24, E132XS_L25, E132XS_L26, E132XS_L27, -1,
	E132XS_L28, E132XS_L29, E132XS_L30, E132XS_L31, -1,
	E132XS_L32, E132XS_L33, E132XS_L34, E132XS_L35, -1,
	E132XS_L36, E132XS_L37, E132XS_L38, E132XS_L39, -1,
	E132XS_L40, E132XS_L41, E132XS_L42, E132XS_L43, -1,
	E132XS_L44, E132XS_L45, E132XS_L46, E132XS_L47, -1,
	E132XS_L48, E132XS_L49, E132XS_L50, E132XS_L51, -1,
	E132XS_L52, E132XS_L53, E132XS_L54, E132XS_L55, -1,
	E132XS_L56, E132XS_L57, E132XS_L58, E132XS_L59, -1,
	E132XS_L60, E132XS_L61, E132XS_L62, E132XS_L63, 0
};

UINT8 hyperstone_win_layout[] =
{
	 0, 0,80, 8, /* register window (top rows) */
	 0, 9,34,13, /* disassembler window (left, middle columns) */
	35, 9,46, 6, /* memory #1 window (right, upper middle) */
	35,16,46, 6, /* memory #2 window (right, lower middle) */
	 0,23,80, 1  /* command line window (bottom row) */
};


/* Delay information */
struct _delay
{
	INT32	delay_cmd;
	UINT32	delay_pc;
};

/* Internal registers */
typedef struct
{
	UINT32	global_regs[32];
	UINT32	local_regs[64];

	/* internal stuff */
	UINT32	ppc;	// previous pc
	UINT16	op;		// opcode
	UINT32	trap_entry; // entry point to get trap address

	UINT8	n;
	void	*timer;
	double	time;
	INT32	delay_timer;

	struct _delay delay;

	int	(*irq_callback)(int irqline);

	INT32 h_clear;
	INT32 instruction_length;
	INT32 intblock;

} hyperstone_regs;

static hyperstone_regs hyperstone;

static struct regs_decode
{
	UINT8	src, dst;	    // destination and source register code
	UINT32	src_value;      // current source register value
	UINT32	next_src_value; // current next source register value
	UINT32	dst_value;      // current destination register value
	UINT32	next_dst_value; // current next destination register value
	UINT8	sub_type;		// sub type opcode (for DD and X_CODE bits)
	union
	{
			UINT32 u;
			INT32  s;
	} extra;				// extra value such as immediate value, const, pcrel, ...
	void	(*set_src_register)(UINT8 reg, UINT32 data);  // set local / global source register
	void	(*set_dst_register)(UINT8 reg, UINT32 data);  // set local / global destination register
	UINT8	src_is_pc;
	UINT8	dst_is_pc;
	UINT8	src_is_sr;
	UINT8	dst_is_sr;
	UINT8   same_src_dst;
	UINT8   same_src_dstf;
	UINT8   same_srcf_dst;
} current_regs;

#define SREG  current_regs.src_value
#define SREGF current_regs.next_src_value
#define DREG  current_regs.dst_value
#define DREGF current_regs.next_dst_value
#define EXTRA_U current_regs.extra.u
#define EXTRA_S current_regs.extra.s

#define SET_SREG( _data_ )  (*current_regs.set_src_register)(current_regs.src, _data_)
#define SET_SREGF( _data_ ) (*current_regs.set_src_register)(current_regs.src + 1, _data_)
#define SET_DREG( _data_ )  (*current_regs.set_dst_register)(current_regs.dst, _data_)
#define SET_DREGF( _data_ ) (*current_regs.set_dst_register)(current_regs.dst + 1, _data_)

#define SRC_IS_PC      current_regs.src_is_pc
#define DST_IS_PC      current_regs.dst_is_pc
#define SRC_IS_SR      current_regs.src_is_sr
#define DST_IS_SR      current_regs.dst_is_sr
#define SAME_SRC_DST   current_regs.same_src_dst
#define SAME_SRC_DSTF  current_regs.same_src_dstf
#define SAME_SRCF_DST  current_regs.same_srcf_dst

static UINT32 program_read_dword_16be(offs_t address)
{
	return (program_read_word_16be(address & ~1) << 16) | program_read_word_16be((address & ~1) + 2);
}

static void program_write_dword_16be(offs_t address, UINT32 data)
{
	program_write_word_16be(address & ~1, (data & 0xffff0000)>>16);
	program_write_word_16be((address & ~1)+2, data & 0xffff);
}

static UINT32 io_read_dword_16be(offs_t address)
{
	return (io_read_word_16be(address & ~1)<< 16) | io_read_word_16be((address & ~1) + 2);
}

static void io_write_dword_16be(offs_t address, UINT32 data)
{
	io_write_word_16be(address & ~1, (data & 0xffff0000)>>16);
	io_write_word_16be((address & ~1)+2, data & 0xffff);
}

/* Opcodes table */
static void (*hyperstone_op[0x100])(void) = {
	hyperstone_chk,	 hyperstone_chk,  hyperstone_chk,   hyperstone_chk,		/* CHK - CHKZ - NOP */
	hyperstone_movd, hyperstone_movd, hyperstone_movd,  hyperstone_movd,	/* MOVD - RET */
	hyperstone_divu, hyperstone_divu, hyperstone_divu,  hyperstone_divu,	/* DIVU */
	hyperstone_divs, hyperstone_divs, hyperstone_divs,  hyperstone_divs,	/* DIVS */
	hyperstone_xm,	 hyperstone_xm,   hyperstone_xm,    hyperstone_xm,		/* XMx - XXx */
	hyperstone_mask, hyperstone_mask, hyperstone_mask,  hyperstone_mask,	/* MASK */
	hyperstone_sum,  hyperstone_sum,  hyperstone_sum,   hyperstone_sum,		/* SUM */
	hyperstone_sums, hyperstone_sums, hyperstone_sums,  hyperstone_sums,	/* SUMS */
	hyperstone_cmp,  hyperstone_cmp,  hyperstone_cmp,   hyperstone_cmp,		/* CMP */
	hyperstone_mov,  hyperstone_mov,  hyperstone_mov,   hyperstone_mov,		/* MOV */
	hyperstone_add,  hyperstone_add,  hyperstone_add,   hyperstone_add,		/* ADD */
	hyperstone_adds, hyperstone_adds, hyperstone_adds,  hyperstone_adds,	/* ADDS */
	hyperstone_cmpb, hyperstone_cmpb, hyperstone_cmpb,  hyperstone_cmpb,	/* CMPB */
	hyperstone_andn, hyperstone_andn, hyperstone_andn,  hyperstone_andn,	/* ANDN */
	hyperstone_or,   hyperstone_or,   hyperstone_or,    hyperstone_or,		/* OR */
	hyperstone_xor,  hyperstone_xor,  hyperstone_xor,   hyperstone_xor,		/* XOR */
	hyperstone_subc, hyperstone_subc, hyperstone_subc,  hyperstone_subc,	/* SUBC */
	hyperstone_not,  hyperstone_not,  hyperstone_not,   hyperstone_not,		/* NOT */
	hyperstone_sub,  hyperstone_sub,  hyperstone_sub,   hyperstone_sub,		/* SUB */
	hyperstone_subs, hyperstone_subs, hyperstone_subs,  hyperstone_subs,	/* SUBS */
	hyperstone_addc, hyperstone_addc, hyperstone_addc,  hyperstone_addc,	/* ADDC */
	hyperstone_and,  hyperstone_and,  hyperstone_and,   hyperstone_and,		/* AND */
	hyperstone_neg,  hyperstone_neg,  hyperstone_neg,   hyperstone_neg,		/* NEG */
	hyperstone_negs, hyperstone_negs, hyperstone_negs,  hyperstone_negs,	/* NEGS */
	hyperstone_cmpi, hyperstone_cmpi, hyperstone_cmpi,  hyperstone_cmpi,	/* CMPI */
	hyperstone_movi, hyperstone_movi, hyperstone_movi,  hyperstone_movi,	/* MOVI */
	hyperstone_addi, hyperstone_addi, hyperstone_addi,  hyperstone_addi,	/* ADDI */
	hyperstone_addsi,hyperstone_addsi,hyperstone_addsi, hyperstone_addsi,	/* ADDSI */
	hyperstone_cmpbi,hyperstone_cmpbi,hyperstone_cmpbi, hyperstone_cmpbi,	/* CMPBI */
	hyperstone_andni,hyperstone_andni,hyperstone_andni, hyperstone_andni,	/* ANDNI */
	hyperstone_ori,  hyperstone_ori,  hyperstone_ori,   hyperstone_ori,		/* ORI */
	hyperstone_xori, hyperstone_xori, hyperstone_xori,  hyperstone_xori,	/* XORI */
	hyperstone_shrdi,hyperstone_shrdi,hyperstone_shrd,  hyperstone_shr,		/* SHRDI, SHRD, SHR */
	hyperstone_sardi,hyperstone_sardi,hyperstone_sard,  hyperstone_sar,		/* SARDI, SARD, SAR */
	hyperstone_shldi,hyperstone_shldi,hyperstone_shld,  hyperstone_shl,		/* SHLDI, SHLD, SHL */
	reserved,		 reserved,		  hyperstone_testlz,hyperstone_rol,		/* RESERVED, TESTLZ, ROL */
	hyperstone_ldxx1,hyperstone_ldxx1,hyperstone_ldxx1, hyperstone_ldxx1,	/* LDxx.D/A/IOD/IOA */
	hyperstone_ldxx2,hyperstone_ldxx2,hyperstone_ldxx2, hyperstone_ldxx2,	/* LDxx.N/S */
	hyperstone_stxx1,hyperstone_stxx1,hyperstone_stxx1, hyperstone_stxx1,	/* STxx.D/A/IOD/IOA */
	hyperstone_stxx2,hyperstone_stxx2,hyperstone_stxx2, hyperstone_stxx2,	/* STxx.N/S */
	hyperstone_shri, hyperstone_shri, hyperstone_shri,  hyperstone_shri,	/* SHRI */
	hyperstone_sari, hyperstone_sari, hyperstone_sari,  hyperstone_sari,	/* SARI */
	hyperstone_shli, hyperstone_shli, hyperstone_shli,  hyperstone_shli,	/* SHLI */
	reserved,		 reserved,		  reserved,			reserved,			/* RESERVED */
	hyperstone_mulu, hyperstone_mulu, hyperstone_mulu,  hyperstone_mulu,	/* MULU */
	hyperstone_muls, hyperstone_muls, hyperstone_muls,  hyperstone_muls,	/* MULS */
	hyperstone_set,  hyperstone_set,  hyperstone_set,   hyperstone_set,		/* SETxx - SETADR - FETCH */
	hyperstone_mul,  hyperstone_mul,  hyperstone_mul,   hyperstone_mul,		/* MUL */
	hyperstone_fadd, hyperstone_faddd,hyperstone_fsub,  hyperstone_fsubd,	/* FADD, FADDD, FSUB, FSUBD */
	hyperstone_fmul, hyperstone_fmuld,hyperstone_fdiv,  hyperstone_fdivd,	/* FMUL, FMULD, FDIV, FDIVD */
	hyperstone_fcmp, hyperstone_fcmpd,hyperstone_fcmpu, hyperstone_fcmpud,	/* FCMP, FCMPD, FCMPU, FCMPUD */
	hyperstone_fcvt, hyperstone_fcvtd,hyperstone_extend,hyperstone_do,		/* FCVT, FCVTD, EXTEND, DO */
	hyperstone_ldwr, hyperstone_ldwr, hyperstone_lddr,  hyperstone_lddr,	/* LDW.R, LDD.R */
	hyperstone_ldwp, hyperstone_ldwp, hyperstone_lddp,  hyperstone_lddp,	/* LDW.P, LDD.P */
	hyperstone_stwr, hyperstone_stwr, hyperstone_stdr,  hyperstone_stdr,	/* STW.R, STD.R */
	hyperstone_stwp, hyperstone_stwp, hyperstone_stdp,  hyperstone_stdp,	/* STW.P, STD.P */
	hyperstone_dbv,  hyperstone_dbnv, hyperstone_dbe,   hyperstone_dbne,	/* DBV, DBNV, DBE, DBNE */
	hyperstone_dbc,  hyperstone_dbnc, hyperstone_dbse,  hyperstone_dbht,	/* DBC, DBNC, DBSE, DBHT */
	hyperstone_dbn,  hyperstone_dbnn, hyperstone_dble,  hyperstone_dbgt,	/* DBN, DBNN, DBLE, DBGT */
	hyperstone_dbr,  hyperstone_frame,hyperstone_call,  hyperstone_call,	/* DBR, FRAME, CALL */
	hyperstone_bv,   hyperstone_bnv,  hyperstone_be,    hyperstone_bne,		/* BV, BNV, BE, BNE */
	hyperstone_bc,   hyperstone_bnc,  hyperstone_bse,   hyperstone_bht,		/* BC, BNC, BSE, BHT */
	hyperstone_bn,   hyperstone_bnn,  hyperstone_ble,   hyperstone_bgt,		/* BN, BNN, BLE, BGT */
 	hyperstone_br,   hyperstone_trap, hyperstone_trap,  hyperstone_trap		/* BR, TRAPxx - TRAP */
};

// 4Kb IRAM (On-Chip Memory)
#if (HAS_E116T || HAS_GMS30C2116)

static ADDRESS_MAP_START( e116_4k_iram_map, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0xc0000000, 0xc0000fff) AM_RAM AM_MIRROR(0x1ffff000)
ADDRESS_MAP_END

#endif

#if (HAS_E132N || HAS_E132T || HAS_GMS30C2132)

static ADDRESS_MAP_START( e132_4k_iram_map, ADDRESS_SPACE_PROGRAM, 32 )
	AM_RANGE(0xc0000000, 0xc0000fff) AM_RAM AM_MIRROR(0x1ffff000)
ADDRESS_MAP_END

#endif

// 8Kb IRAM (On-Chip Memory)

#if (HAS_E116XT || HAS_GMS30C2216)

static ADDRESS_MAP_START( e116_8k_iram_map, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0xc0000000, 0xc0001fff) AM_RAM AM_MIRROR(0x1fffe000)
ADDRESS_MAP_END

#endif

#if (HAS_E132XN || HAS_E132XT || HAS_GMS30C2232)

static ADDRESS_MAP_START( e132_8k_iram_map, ADDRESS_SPACE_PROGRAM, 32 )
	AM_RANGE(0xc0000000, 0xc0001fff) AM_RAM AM_MIRROR(0x1fffe000)
ADDRESS_MAP_END

#endif

// 16Kb IRAM (On-Chip Memory)

#if (HAS_E116XS || HAS_E116XSR)

static ADDRESS_MAP_START( e116_16k_iram_map, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0xc0000000, 0xc0003fff) AM_RAM AM_MIRROR(0x1fffc000)
ADDRESS_MAP_END

#endif

#if (HAS_E132XS || HAS_E132XSR)

static ADDRESS_MAP_START( e132_16k_iram_map, ADDRESS_SPACE_PROGRAM, 32 )
	AM_RANGE(0xc0000000, 0xc0003fff) AM_RAM AM_MIRROR(0x1fffc000)
ADDRESS_MAP_END

#endif

/* Return the entry point for a determinated trap */
static UINT32 get_trap_addr(UINT8 trapno)
{
	UINT32 addr;
	if( hyperstone.trap_entry == 0xffffff00 ) /* @ MEM3 */
	{
		addr = trapno * 4;
	}
	else
	{
		addr = (63 - trapno) * 4;
	}
	addr |= hyperstone.trap_entry;

	return addr;
}

/* Return the entry point for a determinated emulated code (the one for "extend" opcode is reserved) */
static UINT32 get_emu_code_addr(UINT8 num) /* num is OP */
{
	UINT32 addr;
	if( hyperstone.trap_entry == 0xffffff00 ) /* @ MEM3 */
	{
		addr = (hyperstone.trap_entry - 0x100) | ((num & 0xf) << 4);
	}
	else
	{
		addr = hyperstone.trap_entry | (0x10c | ((0xcf - num) << 4));
	}
	return addr;
}

static void hyperstone_set_trap_entry(int which)
{
	switch( which )
	{
		case E132XS_ENTRY_MEM0:
			hyperstone.trap_entry = 0x00000000;
			break;

		case E132XS_ENTRY_MEM1:
			hyperstone.trap_entry = 0x40000000;
			break;

		case E132XS_ENTRY_MEM2:
			hyperstone.trap_entry = 0x80000000;
			break;

		case E132XS_ENTRY_MEM3:
			hyperstone.trap_entry = 0xffffff00;
			break;

		case E132XS_ENTRY_IRAM:
			hyperstone.trap_entry = 0xc0000000;
			break;

		default:
			logerror("Set entry point to a reserved value: %d\n", which);
			break;
	}
}

#define OP				hyperstone.op

#define PPC				hyperstone.ppc //previous pc
#define PC				hyperstone.global_regs[0] //Program Counter
#define SR				hyperstone.global_regs[1] //Status Register
#define FER				hyperstone.global_regs[2] //Floating-Point Exception Register
// 03 - 15  General Purpose Registers
// 16 - 17  Reserved
#define SP				hyperstone.global_regs[18] //Stack Pointer
#define UB				hyperstone.global_regs[19] //Upper Stack Bound
#define BCR				hyperstone.global_regs[20] //Bus Control Register
#define TPR				hyperstone.global_regs[21] //Timer Prescaler Register
#define TCR				hyperstone.global_regs[22] //Timer Compare Register
#define TR				hyperstone.global_regs[23] //Timer Register
#define WCR				hyperstone.global_regs[24] //Watchdog Compare Register
#define ISR				hyperstone.global_regs[25] //Input Status Register
#define FCR				hyperstone.global_regs[26] //Function Control Register
#define MCR				hyperstone.global_regs[27] //Memory Control Register
// 28 - 31  Reserved

/* SR flags */
#define GET_C					( SR & 0x00000001)      // bit 0 //CARRY
#define GET_Z					((SR & 0x00000002)>>1)  // bit 1 //ZERO
#define GET_N					((SR & 0x00000004)>>2)  // bit 2 //NEGATIVE
#define GET_V					((SR & 0x00000008)>>3)  // bit 3 //OVERFLOW
#define GET_M					((SR & 0x00000010)>>4)  // bit 4 //CACHE-MODE
#define GET_H					((SR & 0x00000020)>>5)  // bit 5 //HIGHGLOBAL
// bit 6 RESERVED (always 0)
#define GET_I					((SR & 0x00000080)>>7)  // bit 7 //INTERRUPT-MODE
#define GET_FTE					((SR & 0x00001f00)>>8)  // bits 12 - 8  //Floating-Point Trap Enable
#define GET_FRM					((SR & 0x00006000)>>13) // bits 14 - 13 //Floating-Point Rounding Mode
#define GET_L					((SR & 0x00008000)>>15) // bit 15 //INTERRUPT-LOCK
#define GET_T					((SR & 0x00010000)>>16) // bit 16 //TRACE-MODE
#define GET_P					((SR & 0x00020000)>>17) // bit 17 //TRACE PENDING
#define GET_S					((SR & 0x00040000)>>18) // bit 18 //SUPERVISOR STATE
#define GET_ILC					((SR & 0x00180000)>>19) // bits 20 - 19 //INSTRUCTION-LENGTH
/* if FL is zero it is always interpreted as 16 */
#define GET_FL					((SR & 0x01e00000) ? ((SR & 0x01e00000)>>21) : 16) // bits 24 - 21 //FRAME LENGTH
#define GET_FP					((SR & 0xfe000000)>>25) // bits 31 - 25 //FRAME POINTER

#define SET_C(val)				(SR = (SR & ~0x00000001) | (val))
#define SET_Z(val)				(SR = (SR & ~0x00000002) | ((val) << 1))
#define SET_N(val)				(SR = (SR & ~0x00000004) | ((val) << 2))
#define SET_V(val)				(SR = (SR & ~0x00000008) | ((val) << 3))
#define SET_M(val)				(SR = (SR & ~0x00000010) | ((val) << 4))
#define SET_H(val)				(SR = (SR & ~0x00000020) | ((val) << 5))
#define SET_I(val)				(SR = (SR & ~0x00000080) | ((val) << 7))
#define SET_FTE(val)			(SR = (SR & ~0x00001f00) | ((val) << 8))
#define	SET_FRM(val)			(SR = (SR & ~0x00006000) | ((val) << 13))
#define SET_L(val)				(SR = (SR & ~0x00008000) | ((val) << 15))
#define SET_T(val)				(SR = (SR & ~0x00010000) | ((val) << 16))
#define SET_P(val)				(SR = (SR & ~0x00020000) | ((val) << 17))
#define SET_S(val)				(SR = (SR & ~0x00040000) | ((val) << 18))
#define SET_ILC(val)			(SR = (SR & ~0x00180000) | ((val) << 19))
#define SET_FL(val)				(SR = (SR & ~0x01e00000) | ((val) << 21))
#define SET_FP(val)				(SR = (SR & ~0xfe000000) | ((val) << 25))

#define SET_PC(val)				PC = ((val) & 0xfffffffe) //PC(0) = 0
#define SET_SP(val)				SP = ((val) & 0xfffffffc) //SP(0) = SP(1) = 0
#define SET_UB(val)				UB = ((val) & 0xfffffffc) //UB(0) = UB(1) = 0

#define SET_LOW_SR(val)			(SR = (SR & 0xffff0000) | ((val) & 0x0000ffff)) // when SR is addressed, only low 16 bits can be changed


#define CHECK_C(x) 				(SR = (SR & ~0x00000001) | (((x) & (((UINT64)1) << 32)) ? 1 : 0 ))
#define CHECK_VADD(x,y,z)		(SR = (SR & ~0x00000008) | ((((x) ^ (z)) & ((y) ^ (z)) & 0x80000000) ? 8: 0))
#define CHECK_VADD3(x,y,w,z)	(SR = (SR & ~0x00000008) | ((((x) ^ (z)) & ((y) ^ (z)) & ((w) ^ (z)) & 0x80000000) ? 8: 0))
#define CHECK_VSUB(x,y,z)		(SR = (SR & ~0x00000008) | ((((z) ^ (y)) & ((y) ^ (x)) & 0x80000000) ? 8: 0))


/* FER flags */
#define GET_ACCRUED				(FER & 0x0000001f) //bits  4 - 0 //Floating-Point Accrued Exceptions
#define GET_ACTUAL				(FER & 0x00001f00) //bits 12 - 8 //Floating-Point Actual  Exceptions
//other bits are reversed, in particular 7 - 5 for the operating system.
//the user program can only changes the above 2 flags

/* Timer */
static void hyperstone_timer(int num)
{
	TR++; // wrap around to 0 modulo 32 bits

	if( hyperstone.delay_timer )
	{
		hyperstone.delay_timer = 0;
		timer_adjust(hyperstone.timer,TIME_IN_HZ(hyperstone.time),0,TIME_IN_HZ(hyperstone.time));
	}
}

static UINT32 get_global_register(UINT8 code)
{
/*
    if( code >= 16 )
    {
        switch( code )
        {
        case 16:
        case 17:
        case 28:
        case 29:
        case 30:
        case 31:
            printf("read _Reserved_ Global Register %d @ %08X\n",code,PC);
            break;

        case BCR_REGISTER:
            printf("read write-only BCR register @ %08X\n",PC);
            return 0;

        case TPR_REGISTER:
            printf("read write-only TPR register @ %08X\n",PC);
            return 0;

        case FCR_REGISTER:
            printf("read write-only FCR register @ %08X\n",PC);
            return 0;

        case MCR_REGISTER:
            printf("read write-only MCR register @ %08X\n",PC);
            return 0;
        }
    }
*/
	return hyperstone.global_regs[code];
}

static void set_global_register(UINT8 code, UINT32 val)
{
	//TODO: add correct FER set instruction

	if( code == PC_REGISTER )
	{
		SET_PC(val);
		change_pc(PC);
	}
	else if( code == SR_REGISTER )
	{
		SET_LOW_SR(val); // only a RET instruction can change the full content of SR

		SR &= ~0x40; //reserved bit 6 always zero
	}
	else
	{
		if( code != ISR_REGISTER )
			hyperstone.global_regs[code] = val;
		else
			logerror("Written to ISR register. PC = %08X\n", PC);

		//are these set only when privilege bit is set?
		if( code >= 16 )
		{
			switch( code )
			{
			case 18:
				SET_SP(val);
				break;

			case 19:
				SET_UB(val);
				break;
/*
            case ISR_REGISTER:
                printf("written %08X to read-only ISR register\n",val);
                break;

            case 22:
//              printf("written %08X to TCR register\n",val);
                break;

            case 23:
//              printf("written %08X to TR register\n",val);
                break;

            case 24:
//              printf("written %08X to WCR register\n",val);
                break;

            case 16:
            case 17:
            case 28:
            case 29:
            case 30:
            case 31:
                printf("written %08X to _Reserved_ Global Register %d\n",val,code);
                break;

            case BCR_REGISTER:
                break;
*/
			case TPR_REGISTER:

				hyperstone.n = (val & 0xff0000) >> 16;
				hyperstone.time = cpunum_get_clock(0) / (hyperstone.n + 2);

				if(!(val & 0x80000000)) /* change immediately */
				{
					hyperstone.delay_timer = 0;
					timer_adjust(hyperstone.timer,TIME_IN_HZ(hyperstone.time),0,TIME_IN_HZ(hyperstone.time));
				}
				else
				{
					hyperstone.delay_timer = 1;
				}

				break;
/*
            case FCR_REGISTER:
                switch((val & 0x3000)>>12)
                {
                case 0:
                    printf("IO3 interrupt mode\n");
                    break;
                case 1:
                    printf("IO3 timing mode\n");
                    break;
                case 2:
                    printf("watchdog mode\n");
                    break;
                case 3:
                    // IO3 standard mode
                    break;
                }
                break;
*/
			case MCR_REGISTER:
				// bits 14..12 EntryTableMap
				hyperstone_set_trap_entry((val & 0x7000) >> 12);

				break;
			}
		}
	}
}

static void set_local_register(UINT8 code, UINT32 val)
{
	UINT8 new_code = (code + GET_FP) % 64;

	hyperstone.local_regs[new_code] = val;
}

#define GET_ABS_L_REG(code)			hyperstone.local_regs[code]
#define SET_L_REG(code, val)	    set_local_register(code, val)
#define SET_ABS_L_REG(code, val)	hyperstone.local_regs[code] = val
#define GET_G_REG(code)				get_global_register(code)
#define SET_G_REG(code, val)	    set_global_register(code, val)

#define S_BIT					((OP & 0x100) >> 8)
#define N_BIT					S_BIT
#define D_BIT					((OP & 0x200) >> 9)
#define N_VALUE					((N_BIT << 4) | (OP & 0x0f))
#define DST_CODE				((OP & 0xf0) >> 4)
#define SRC_CODE				(OP & 0x0f)
#define SIGN_BIT(val)			((val & 0x80000000) >> 31)

#define LOCAL  1

static void decode_source(int local, int hflag)
{
	if(local)
	{
		UINT8 code = current_regs.src;
		current_regs.set_src_register = set_local_register;
		code = (current_regs.src + GET_FP) % 64; // registers offset by frame pointer
		SREG = hyperstone.local_regs[code];
		code = (current_regs.src + 1 + GET_FP) % 64;
		SREGF = hyperstone.local_regs[code];
	}
	else
	{
		current_regs.set_src_register = set_global_register;

		if(hflag)
			current_regs.src += hflag * 16;

		SREG = hyperstone.global_regs[current_regs.src];

		/* bound safe */
		if(current_regs.src != 15 && current_regs.src != 31)
			SREGF = hyperstone.global_regs[current_regs.src + 1];

		if(current_regs.src == PC_REGISTER)
			SRC_IS_PC = 1;
		else if(current_regs.src == SR_REGISTER)
			SRC_IS_SR = 1;
		else if( current_regs.src == BCR_REGISTER || current_regs.src == TPR_REGISTER ||
				 current_regs.src == FCR_REGISTER || current_regs.src == MCR_REGISTER )
		{
			SREG = 0; // write-only registers
		}
		else if( current_regs.src == ISR_REGISTER )
			printf("read src ISR. PC = %08X\n",PPC);
	}
}

static void decode_dest(int local, int hflag)
{
	if(local)
	{
		UINT8 code = current_regs.dst;
		current_regs.set_dst_register = set_local_register;
		code = (current_regs.dst + GET_FP) % 64; // registers offset by frame pointer
		DREG = hyperstone.local_regs[code];
		code = (current_regs.dst + 1 + GET_FP) % 64;
		DREGF = hyperstone.local_regs[code];
	}
	else
	{
		current_regs.set_dst_register = set_global_register;

		if(hflag)
			current_regs.dst += hflag * 16;

		DREG = hyperstone.global_regs[current_regs.dst];

		/* bound safe */
		if(current_regs.dst != 15 && current_regs.dst != 31)
			DREGF = hyperstone.global_regs[current_regs.dst + 1];

		if(current_regs.dst == PC_REGISTER)
			DST_IS_PC = 1;
		else if(current_regs.dst == SR_REGISTER)
			DST_IS_SR = 1;
		else if( current_regs.src == ISR_REGISTER )
			printf("read dst ISR. PC = %08X\n",PPC);
	}
}

INLINE void decode_RR(void)
{
	current_regs.src = SRC_CODE;
	current_regs.dst = DST_CODE;
	decode_source(S_BIT, 0);
	decode_dest(D_BIT, 0);

	if( SRC_CODE == DST_CODE && S_BIT == D_BIT )
		SAME_SRC_DST = 1;

	if( S_BIT == LOCAL && D_BIT == LOCAL )
	{
		if( SRC_CODE == ((DST_CODE + 1) % 64) )
			SAME_SRC_DSTF = 1;

		if( ((SRC_CODE + 1) % 64) == DST_CODE )
			SAME_SRCF_DST = 1;
	}
	else if( S_BIT == 0 && D_BIT == 0 )
	{
		if( SRC_CODE == (DST_CODE + 1) )
			SAME_SRC_DSTF = 1;

		if( (SRC_CODE + 1) == DST_CODE )
			SAME_SRCF_DST = 1;
	}
}

INLINE void decode_LL(void)
{
	current_regs.src = SRC_CODE;
	current_regs.dst = DST_CODE;
	decode_source(LOCAL, 0);
	decode_dest(LOCAL, 0);

	if( SRC_CODE == DST_CODE )
		SAME_SRC_DST = 1;

	if( SRC_CODE == ((DST_CODE + 1) % 64) )
		SAME_SRC_DSTF = 1;
}

INLINE void decode_LR(void)
{
	current_regs.src = SRC_CODE;
	current_regs.dst = DST_CODE;
	decode_source(S_BIT, 0);
	decode_dest(LOCAL, 0);

	if( ((SRC_CODE + 1) % 64) == DST_CODE && S_BIT == LOCAL )
		SAME_SRCF_DST = 1;
}

INLINE void check_delay_PC(void)
{
	// if PC is used in a delay instruction, the delayed PC should be used
	if( hyperstone.delay.delay_cmd == DELAY_EXECUTE )
	{
		PC = hyperstone.delay.delay_pc;
		hyperstone.delay.delay_cmd = NO_DELAY;
	}
}

INLINE void decode_immediate(void)
{
	switch( N_VALUE )
	{
		case 0:	case 1:  case 2:  case 3:  case 4:  case 5:  case 6:  case 7: case 8:
		case 9:	case 10: case 11: case 12: case 13: case 14: case 15: case 16:
			EXTRA_U = N_VALUE;
			break;

		case 17:
			hyperstone.instruction_length = 3;
			EXTRA_U = (READ_OP(PC) << 16) | READ_OP(PC + 2);
			PC += 4;
			break;

		case 18:
			hyperstone.instruction_length = 2;
			EXTRA_U = READ_OP(PC);
			PC += 2;
			break;

		case 19:
			hyperstone.instruction_length = 2;
			EXTRA_U = 0xffff0000 | READ_OP(PC);
			PC += 2;
			break;

		case 20:
			EXTRA_U = 32;	// bit 5 = 1, others = 0
			break;

		case 21:
			EXTRA_U = 64;	// bit 6 = 1, others = 0
			break;

		case 22:
			EXTRA_U = 128; // bit 7 = 1, others = 0
			break;

		case 23:
			EXTRA_U = 0x80000000; // bit 31 = 1, others = 0 (2 at the power of 31)
			break;

		case 24:
			EXTRA_U = -8;
			break;

		case 25:
			EXTRA_U = -7;
			break;

		case 26:
			EXTRA_U = -6;
			break;

		case 27:
			EXTRA_U = -5;
			break;

		case 28:
			EXTRA_U = -4;
			break;

		case 29:
			EXTRA_U = -3;
			break;

		case 30:
			EXTRA_U = -2;
			break;

		case 31:
			EXTRA_U = -1;
			break;
	}
}

INLINE void decode_const(void)
{
	UINT16 imm_1 = READ_OP(PC);

	PC += 2;
	hyperstone.instruction_length = 2;

	if( E_BIT(imm_1) )
	{
		UINT16 imm_2 = READ_OP(PC);

		PC += 2;
		hyperstone.instruction_length = 3;

		EXTRA_S = imm_2;
		EXTRA_S |= ((imm_1 & 0x3fff) << 16);

		if( S_BIT_CONST(imm_1) )
		{
			EXTRA_S |= 0xc0000000;
		}
	}
	else
	{
		EXTRA_S = imm_1 & 0x3fff;

		if( S_BIT_CONST(imm_1) )
		{
			EXTRA_S |= 0xffffc000;
		}
	}
}

INLINE void decode_pcrel(void)
{
	if( OP & 0x80 )
	{
		UINT16 next = READ_OP(PC);

		PC += 2;
		hyperstone.instruction_length = 2;

		EXTRA_S = (OP & 0x7f) << 16;
		EXTRA_S |= (next & 0xfffe);

		if( next & 1 )
			EXTRA_S |= 0xff800000;
	}
	else
	{
		EXTRA_S = OP & 0x7e;

		if( OP & 1 )
			EXTRA_S |= 0xffffff80;
	}
}

INLINE void decode_dis(void)
{
	UINT16 next_1 = READ_OP(PC);

	PC += 2;
	hyperstone.instruction_length = 2;

	current_regs.sub_type = DD(next_1);

	if( E_BIT(next_1) )
	{
		UINT16 next_2 = READ_OP(PC);

		PC += 2;
		hyperstone.instruction_length = 3;

		EXTRA_S = next_2;
		EXTRA_S |= ((next_1 & 0xfff) << 16);

		if( S_BIT_CONST(next_1) )
		{
			EXTRA_S |= 0xf0000000;
		}
	}
	else
	{
		EXTRA_S = next_1 & 0xfff;

		if( S_BIT_CONST(next_1) )
		{
			EXTRA_S |= 0xfffff000;
		}
	}
}

INLINE void decode_lim(void)
{
	UINT32 next = READ_OP(PC);
	PC += 2;
	hyperstone.instruction_length = 2;

	current_regs.sub_type = X_CODE(next);

	if( E_BIT(next) )
	{
		EXTRA_U = ((next & 0xfff) << 16) | READ_OP(PC);
		PC += 2;
		hyperstone.instruction_length = 3;
	}
	else
	{
		EXTRA_U = next & 0xfff;
	}
}

static void decode_registers(void)
{
	memset(&current_regs, 0, sizeof(struct regs_decode));

	switch((OP & 0xff00) >> 8)
	{
		// RR decode
		case 0x00: case 0x01: case 0x02: case 0x03: // CHK - CHKZ - NOP
		case 0x04: case 0x05: case 0x06: case 0x07: // MOVD - RET
		case 0x08: case 0x09: case 0x0a: case 0x0b: // DIVU
		case 0x0c: case 0x0d: case 0x0e: case 0x0f: // DIVS
		case 0x20: case 0x21: case 0x22: case 0x23: // CMP
		case 0x28: case 0x29: case 0x2a: case 0x2b: // ADD
		case 0x2c: case 0x2d: case 0x2e: case 0x2f: // ADDS
		case 0x30: case 0x31: case 0x32: case 0x33: // CMPB
		case 0x34: case 0x35: case 0x36: case 0x37: // ANDN
		case 0x38: case 0x39: case 0x3a: case 0x3b: // OR
		case 0x3c: case 0x3d: case 0x3e: case 0x3f: // XOR
		case 0x40: case 0x41: case 0x42: case 0x43: // SUBC
		case 0x44: case 0x45: case 0x46: case 0x47: // NOT
		case 0x48: case 0x49: case 0x4a: case 0x4b: // SUB
		case 0x4c: case 0x4d: case 0x4e: case 0x4f: // SUBS
		case 0x50: case 0x51: case 0x52: case 0x53: // ADDC
		case 0x54: case 0x55: case 0x56: case 0x57: // AND
		case 0x58: case 0x59: case 0x5a: case 0x5b: // NEG
		case 0x5c: case 0x5d: case 0x5e: case 0x5f: // NEGS
		case 0xb0: case 0xb1: case 0xb2: case 0xb3: // MULU
		case 0xb4: case 0xb5: case 0xb6: case 0xb7: // MULS
		case 0xbc: case 0xbd: case 0xbe: case 0xbf: // MUL

			check_delay_PC();
			decode_RR();

		break;

		// RRlim decode
		case 0x10: case 0x11: case 0x12: case 0x13: // XMx - XXx

			decode_lim();
			check_delay_PC();
			decode_RR();

		break;

		// RRconst decode
		case 0x14: case 0x15: case 0x16: case 0x17: // MASK
		case 0x18: case 0x19: case 0x1a: case 0x1b: // SUM
		case 0x1c: case 0x1d: case 0x1e: case 0x1f: // SUMS

			decode_const();
			check_delay_PC();
			decode_RR();

		break;

		// RRdis decode
		case 0x90: case 0x91: case 0x92: case 0x93: // LDxx.D/A/IOD/IOA
		case 0x94: case 0x95: case 0x96: case 0x97: // LDxx.N/S
		case 0x98: case 0x99: case 0x9a: case 0x9b: // STxx.D/A/IOD/IOA
		case 0x9c: case 0x9d: case 0x9e: case 0x9f: // STxx.N/S

			decode_dis();
			check_delay_PC();
			decode_RR();

		break;

		// RR decode with H flag
		case 0x24: case 0x25: case 0x26: case 0x27: // MOV

			check_delay_PC();

			current_regs.src = SRC_CODE;
			current_regs.dst = DST_CODE;
			decode_source(S_BIT, GET_H);
			decode_dest(D_BIT, GET_H);

			if(GET_H)
				if(S_BIT == 0 && D_BIT == 0)
					ui_popup("MOV with hflag and 2 GRegs! PC = %08X\n",PPC);

		break;

		// Rimm decode
		case 0x60: case 0x61: case 0x62: case 0x63: // CMPI
		case 0x68: case 0x69: case 0x6a: case 0x6b: // ADDI
		case 0x6c: case 0x6d: case 0x6e: case 0x6f: // ADDSI
		case 0x70: case 0x71: case 0x72: case 0x73: // CMPBI
		case 0x74: case 0x75: case 0x76: case 0x77: // ANDNI
		case 0x78: case 0x79: case 0x7a: case 0x7b: // ORI
		case 0x7c: case 0x7d: case 0x7e: case 0x7f: // XORI

			decode_immediate();
			check_delay_PC();

			current_regs.dst = DST_CODE;
			decode_dest(D_BIT, 0);

		break;

		// Rn decode
		case 0xa0: case 0xa1: case 0xa2: case 0xa3: // SHRI
		case 0xa4: case 0xa5: case 0xa6: case 0xa7: // SARI
		case 0xa8: case 0xa9: case 0xaa: case 0xab: // SHLI
		case 0xb8: case 0xb9: case 0xba: case 0xbb: // SETxx - SETADR - FETCH

			check_delay_PC();

			current_regs.dst = DST_CODE;
			decode_dest(D_BIT, 0);

		break;

		// Rimm decode with H flag
		case 0x64: case 0x65: case 0x66: case 0x67: // MOVI

			decode_immediate();
			check_delay_PC();

			current_regs.dst = DST_CODE;
			decode_dest(D_BIT, GET_H);

		break;

		// Ln decode
		case 0x80: case 0x81: // SHRDI
		case 0x84: case 0x85: // SARDI
		case 0x88: case 0x89: // SHLDI

			check_delay_PC();

			current_regs.dst = DST_CODE;
			decode_dest(LOCAL, 0);

		break;

		// LL decode
		case 0x82: // SHRD
		case 0x83: // SHR
		case 0x86: // SARD
		case 0x87: // SAR
		case 0x8a: // SHLD
		case 0x8b: // SHL
		case 0x8e: // TESTLZ
		case 0x8f: // ROL
		case 0xc0: // FADD
		case 0xc1: // FADDD
		case 0xc2: // FSUB
		case 0xc3: // FSUBD
		case 0xc4: // FMUL
		case 0xc5: // FMULD
		case 0xc6: // FDIV
		case 0xc7: // FDIVD
		case 0xc8: // FCMP
		case 0xc9: // FCMPD
		case 0xca: // FCMPU
		case 0xcb: // FCMPUD
		case 0xcc: // FCVT
		case 0xcd: // FCVTD
		case 0xcf: // DO
		case 0xed: // FRAME

			check_delay_PC();
			decode_LL();

		break;

		// LLext decode
		case 0xce: // EXTEND

			hyperstone.instruction_length = 2;
			EXTRA_U = READ_OP(PC);
			PC += 2;
			check_delay_PC();
			decode_LL();

		break;

		// LR decode
		case 0xd0: case 0xd1: // LDW.R
		case 0xd2: case 0xd3: // LDD.R
		case 0xd4: case 0xd5: // LDW.P
		case 0xd6: case 0xd7: // LDD.P
		case 0xd8: case 0xd9: // STW.R
		case 0xda: case 0xdb: // STD.R
		case 0xdc: case 0xdd: // STW.P
		case 0xde: case 0xdf: // STD.P

			check_delay_PC();
			decode_LR();

		break;

		// LRconst decode
		case 0xee: case 0xef: // CALL

			decode_const();
			check_delay_PC();
			decode_LR();

		break;


        // PCrel decode
        case 0xe0: // DBV
        case 0xe1: // DBNV
        case 0xe2: // DBE
        case 0xe3: // DBNE
        case 0xe4: // DBC
        case 0xe5: // DBNC
        case 0xe6: // DBSE
        case 0xe7: // DBHT
        case 0xe8: // DBN
        case 0xe9: // DBNN
        case 0xea: // DBLE
        case 0xeb: // DBGT
        case 0xec: // DBR
        case 0xf0: // BV
        case 0xf1: // BNV
        case 0xf2: // BE
        case 0xf3: // BNE
        case 0xf4: // BC
        case 0xf5: // BNC
        case 0xf6: // BSE
        case 0xf7: // BHT
        case 0xf8: // BN
        case 0xf9: // BNN
        case 0xfa: // BLE
        case 0xfb: // BGT
        case 0xfc: // BR

			decode_pcrel();
			check_delay_PC();

        break;

        // PCadr decode
        case 0xfd: case 0xfe: case 0xff: // TRAPxx - TRAP

			check_delay_PC();

		break;
/*
        // RESERVED
        case 0x8c: case 0x8d:
        case 0xac: case 0xad: case 0xae: case 0xaf:
            break;
*/
	}
}



INLINE void execute_br(void)
{
	PPC = PC;
	PC += EXTRA_S;
	change_pc(PC);
	SET_M(0);

	hyperstone_ICount -= 2;
}

INLINE void execute_dbr(void)
{
	hyperstone.delay.delay_cmd = DELAY_TAKEN;
	hyperstone.delay.delay_pc  = PC + EXTRA_S;

	hyperstone.intblock = 3;
}


static void execute_trap(UINT32 addr)
{
	UINT8 reg;
	UINT32 oldSR;
	reg = GET_FP + GET_FL;

	SET_ILC(hyperstone.instruction_length & 3);

	oldSR = SR;

	SET_FL(6);
	SET_FP(reg);

	SET_L_REG(0, (PC & 0xfffffffe) | GET_S);
	SET_L_REG(1, oldSR);

	SET_M(0);
	SET_T(0);
	SET_L(1);
	SET_S(1);

	PPC = PC;
	PC = addr;
	change_pc(PC);

	hyperstone_ICount -= 2;
}


static void execute_int(UINT32 addr)
{
	UINT8 reg;
	UINT32 oldSR;
	reg = GET_FP + GET_FL;

	SET_ILC(hyperstone.instruction_length & 3);

	oldSR = SR;

	SET_FL(2);
	SET_FP(reg);

	SET_L_REG(0, (PC & 0xfffffffe) | GET_S);
	SET_L_REG(1, oldSR);

	SET_M(0);
	SET_T(0);
	SET_L(1);
	SET_S(1);
	SET_I(1);

	PPC = PC;
	PC = addr;
	change_pc(PC);

	hyperstone_ICount -= 2;
}

/* TODO: mask Parity Error and Extended Overflow exceptions */
static void execute_exception(UINT32 addr)
{
	UINT8 reg;
	UINT32 oldSR;
	reg = GET_FP + GET_FL;

	SET_ILC(hyperstone.instruction_length & 3);

	oldSR = SR;

	SET_FP(reg);
	SET_FL(2);

	SET_L_REG(0, (PC & 0xfffffffe) | GET_S);
	SET_L_REG(1, oldSR);

	SET_M(0);
	SET_T(0);
	SET_L(1);
	SET_S(1);

	PPC = PC;
	PC = addr;
	change_pc(PC);

	printf("EXCEPTION! PPC = %08X PC = %08X\n",PPC-2,PC-2);
	hyperstone_ICount -= 2;
}

static void execute_software(void)
{
	UINT8 reg;
	UINT32 oldSR;
	UINT32 addr;
	UINT32 stack_of_dst;

	SET_ILC(1);

	addr = get_emu_code_addr((OP & 0xff00) >> 8);
	reg = GET_FP + GET_FL;

	//since it's sure the register is in the register part of the stack,
	//set the stack address to a value above the highest address
	//that can be set by a following frame instruction
	stack_of_dst = (SP & ~0xff) + 64*4 + (((GET_FP + current_regs.dst) % 64) * 4); //converted to 32bits offset

	oldSR = SR;

	SET_FL(6);
	SET_FP(reg);

	SET_L_REG(0, stack_of_dst);
	SET_L_REG(1, SREG);
	SET_L_REG(2, SREGF);
	SET_L_REG(3, (PC & 0xfffffffe) | GET_S);
	SET_L_REG(4, oldSR);

	SET_M(0);
	SET_T(0);
	SET_L(1);

	PPC = PC;
	PC = addr;
	change_pc(PC);
}


/*
    IRQ lines :
        0 - IO2     (trap 48)
        1 - IO1     (trap 49)
        2 - INT4    (trap 50)
        3 - INT3    (trap 51)
        4 - INT2    (trap 52)
        5 - INT1    (trap 53)
        6 - IO3     (trap 54)
        7 - TIMER   (trap 55)
*/
static void set_irq_line(int irqline, int state)
{
	/* Interrupt-Lock flag isn't set */

	if(hyperstone.intblock)
		return;

	if( !GET_L && state )
	{
		int execint=0;
		switch(irqline)
		{
			case 0:	 if( (FCR&(1<<6)) && (!(FCR&(1<<4))) ) execint=1; // IO2
				break;
			case 1:	 if( (FCR&(1<<2)) && (!(FCR&(1<<0))) ) execint=1; // IO1
				break;
			case 2:	 if( !(FCR&(1<<31)) ) execint=1; //  int 4
				break;
			case 3:	 if( !(FCR&(1<<30)) ) execint=1; //  int 3
				break;
			case 4:	 if( !(FCR&(1<<29)) ) execint=1; //  int 2
				break;
			case 5:	 if( !(FCR&(1<<28)) ) execint=1; //  int 1
				break;
			case 6:	 if( (FCR&(1<<10)) && (!(FCR&(1<<8))) ) execint=1; // IO3
				break;
			case 7:	 if( !(FCR&(1<<23)) ) execint=1; //  timer
				break;
		}
/*
        if( (FCR&(1<<6)) && (!(FCR&(1<<4))) ) printf("IO2 en\n"); // IO2

        if( (FCR&(1<<2)) && (!(FCR&(1<<0))) ) printf("IO1 en\n"); // IO1

        if( !(FCR&(1<<31)) ) printf("int4 en\n"); //  int 4

        if( !(FCR&(1<<30)) ) printf("int3 en\n"); //  int 3

        if( !(FCR&(1<<29)) ) printf("int2 en\n"); //  int 2

        if( !(FCR&(1<<28)) ) printf("int1 en\n"); //  int 1

        if( (FCR&(1<<10)) && (!(FCR&(1<<8))) ) printf("IO3 en\n"); // IO3

      //  if( !(FCR&(1<<23)) ) printf("timer irq!\n"); //  timer
*/
		if(execint)
		{
			execute_int(get_trap_addr(irqline + 48));
			(*hyperstone.irq_callback)(irqline);
		}
	}
}

static void hyperstone_init(int index, int clock, const void *config, int (*irqcallback)(int))
{
	state_save_register_item_array("E132XS", index, hyperstone.global_regs);
	state_save_register_item_array("E132XS", index, hyperstone.local_regs);
	state_save_register_item("E132XS", index, hyperstone.ppc);
	state_save_register_item("E132XS", index, hyperstone.trap_entry);
	state_save_register_item("E132XS", index, hyperstone.delay.delay_pc);
	state_save_register_item("E132XS", index, hyperstone.op);
	state_save_register_item("E132XS", index, hyperstone.n);
	state_save_register_item("E132XS", index, hyperstone.h_clear);
	state_save_register_item("E132XS", index, hyperstone.instruction_length);
	state_save_register_item("E132XS", index, hyperstone.intblock);
	state_save_register_item("E132XS", index, hyperstone.delay_timer);
	state_save_register_item("E132XS", index, hyperstone.delay.delay_cmd);
	state_save_register_item("E132XS", index, hyperstone.time);

	hyperstone.timer = timer_alloc(hyperstone_timer);
	timer_adjust(hyperstone.timer, TIME_NEVER, 0, 0);

	hyperstone.irq_callback = irqcallback;
}

static void e116_init(int index, int clock, const void *config, int (*irqcallback)(int))
{
	hyp_cpu_read_byte      = program_read_byte_16be;
	hyp_cpu_read_half_word = program_read_word_16be;
	hyp_cpu_read_word      = program_read_dword_16be;
	hyp_cpu_read_io_word   = io_read_dword_16be;

	hyp_cpu_write_byte      = program_write_byte_16be;
	hyp_cpu_write_half_word = program_write_word_16be;
	hyp_cpu_write_word      = program_write_dword_16be;
	hyp_cpu_write_io_word   = io_write_dword_16be;

	hyp_type_16bit = 1;

	hyperstone_init(index, clock, config, irqcallback);
}

static void e132_init(int index, int clock, const void *config, int (*irqcallback)(int))
{
	hyp_cpu_read_byte      = program_read_byte_32be;
	hyp_cpu_read_half_word = program_read_word_32be;
	hyp_cpu_read_word      = program_read_dword_32be;
	hyp_cpu_read_io_word   = io_read_dword_32be;

	hyp_cpu_write_byte      = program_write_byte_32be;
	hyp_cpu_write_half_word = program_write_word_32be;
	hyp_cpu_write_word      = program_write_dword_32be;
	hyp_cpu_write_io_word   = io_write_dword_32be;

	hyp_type_16bit = 0;

	hyperstone_init(index, clock, config, irqcallback);
}

static void hyperstone_reset(void)
{
	//TODO: Add different reset initializations for BCR, MCR, FCR, TPR

	int (*save_irqcallback)(int);
	void *hyp_timer;

	save_irqcallback = hyperstone.irq_callback;
	hyp_timer = hyperstone.timer;
	memset(&hyperstone, 0, sizeof(hyperstone_regs));
	hyperstone.timer = hyp_timer;
	hyperstone.irq_callback = save_irqcallback;

	hyperstone.h_clear = 0;
	hyperstone.instruction_length = 0;

	hyperstone_set_trap_entry(E132XS_ENTRY_MEM3); /* default entry point @ MEM3 */

	set_global_register(BCR_REGISTER, ~0);
	set_global_register(MCR_REGISTER, ~0);
	set_global_register(FCR_REGISTER, ~0);
	set_global_register(TPR_REGISTER, 0xc000000);

	PC = get_trap_addr(RESET);
	change_pc(PC);

	SET_FP(0);
	SET_FL(2);

	SET_M(0);
	SET_T(0);
	SET_L(1);
	SET_S(1);

	SET_L_REG(0, (PC & 0xfffffffe) | GET_S);
	SET_L_REG(1, SR);

	hyperstone_ICount -= 2;
}

static void hyperstone_exit(void)
{
	// nothing to do
}

static int hyperstone_execute(int cycles)
{
	hyperstone_ICount = cycles;

	do
	{
		PPC = PC;	/* copy PC to previous PC */

		CALL_MAME_DEBUG;

		OP = READ_OP(PC);

		PC += 2;

		if(GET_H)
		{
			hyperstone.h_clear = 1;
		}

		hyperstone.instruction_length = 1;

		decode_registers();

		hyperstone_op[(OP & 0xff00) >> 8]();

		SET_ILC(hyperstone.instruction_length & 3);

		if(hyperstone.h_clear)
		{
			SET_H(0);
			hyperstone.h_clear = 0;
		}

		if( GET_T && GET_P && hyperstone.delay.delay_cmd == NO_DELAY ) /* Not in a Delayed Branch instructions */
		{
			UINT32 addr = get_trap_addr(TRACE_EXCEPTION);
			execute_exception(addr);
		}

		if( hyperstone.delay.delay_cmd == DELAY_TAKEN )
		{
			hyperstone.delay.delay_cmd = DELAY_EXECUTE;
		}

		if(hyperstone.intblock > 0)
		{
			hyperstone.intblock--;
		}
		else
		{
			if( !((TR - TCR) & 0x80000000) )
			{
				/* generate a timer interrupt */
				set_irq_line(7, PULSE_LINE);
			}
		}

	} while( hyperstone_ICount > 0 );

	return cycles - hyperstone_ICount;
}

static void hyperstone_get_context(void *regs)
{
	/* copy the context */
	if( regs )
		*(hyperstone_regs *)regs = hyperstone;
}

static void hyperstone_set_context(void *regs)
{
	/* copy the context */
	if (regs)
		hyperstone = *(hyperstone_regs *)regs;
}

static offs_t hyperstone_dasm(char *buffer, offs_t pc)
{
#ifdef MAME_DEBUG
	return dasm_hyperstone( buffer, pc, GET_H, GET_FP );
#else
	sprintf(buffer, "$%08x", READ_OP(pc));
	return 1;
#endif
}

/* Opcodes */

static void hyperstone_chk(void)
{
	UINT32 addr = get_trap_addr(RANGE_ERROR);

	if( SRC_IS_SR )
	{
		if( DREG == 0 )
			execute_exception(addr);
	}
	else
	{
		if( SRC_IS_PC )
		{
			if( DREG >= SREG )
				execute_exception(addr);
		}
		else
		{
			if( DREG > SREG )
				execute_exception(addr);
		}
	}

	hyperstone_ICount -= 1;
}

static void hyperstone_movd(void)
{
	if( DST_IS_PC ) // Rd denotes PC
	{
		// RET instruction

		UINT8 old_s, old_l;
		INT8 difference; // really it's 7 bits

		if( SRC_IS_PC || SRC_IS_SR )
		{
			logerror("Denoted PC or SR in RET instruction. PC = %08X\n", PC);
		}
		else
		{
			old_s = GET_S;
			old_l = GET_L;
			PPC = PC;

			PC = SET_PC(SREG);
			change_pc(PC);
			SR = (SREGF & 0xffe00000) | ((SREG & 0x01) << 18 ) | (SREGF & 0x3ffff);

			hyperstone.instruction_length = 0; // undefined

			if( (!old_s && GET_S) || (!GET_S && !old_l && GET_L))
			{
				UINT32 addr = get_trap_addr(PRIVILEGE_ERROR);
				execute_exception(addr);
			}

			difference = GET_FP - ((SP & 0x1fc) >> 2);

			/* convert to 8 bits */
			if(difference > 63)
				difference = (INT8)(difference|0x80);
			else if( difference < -64 )
				difference = difference & 0x7f;

			if( difference < 0 ) //else it's finished
			{
				do
				{
					SP -= 4;
					SET_ABS_L_REG(((SP & 0xfc) >> 2), READ_W(SP));
					difference++;

				} while(difference != 0);
			}
		}

		//TODO: no 1!
		hyperstone_ICount -= 1;
	}
	else if( SRC_IS_SR ) // Rd doesn't denote PC and Rs denotes SR
	{
		SET_DREG(0);
		SET_DREGF(0);
		SET_Z(1);
		SET_N(0);

		hyperstone_ICount -= 2;
	}
	else // Rd doesn't denote PC and Rs doesn't denote SR
	{
		UINT64 tmp;

		SET_DREG(SREG);
		SET_DREGF(SREGF);

		tmp = COMBINE_U64_U32_U32(SREG, SREGF);
		SET_Z( tmp == 0 ? 1 : 0 );
		SET_N( SIGN_BIT(SREG) );

		hyperstone_ICount -= 2;
	}
}

static void hyperstone_divu(void)
{
	if( SAME_SRC_DST || SAME_SRC_DSTF )
	{
		logerror("Denoted the same register code in hyperstone_divu instruction. PC = %08X\n", PC);
	}
	else
	{
		if( SRC_IS_PC || SRC_IS_SR )
		{
			logerror("Denoted PC or SR as source register in hyperstone_divu instruction. PC = %08X\n", PC);
		}
		else
		{
			UINT64 dividend;

			dividend = COMBINE_U64_U32_U32(DREG, DREGF);

			if( SREG == 0 )
			{
				//Rd//Rdf -> undefined
				//Z -> undefined
				//N -> undefined
				UINT32 addr;
				SET_V(1);
				addr = get_trap_addr(RANGE_ERROR);
				execute_exception(addr);
			}
			else
			{
				UINT32 quotient, remainder;

				/* TODO: add quotient overflow */
				quotient = dividend / SREG;
				remainder = dividend % SREG;

				SET_DREG(remainder);
				SET_DREGF(quotient);

				SET_Z( quotient == 0 ? 1 : 0 );
				SET_N( SIGN_BIT(quotient) );
				SET_V(0);
			}
		}
	}

	hyperstone_ICount -= 36;
}

static void hyperstone_divs(void)
{
	if( SAME_SRC_DST || SAME_SRC_DSTF )
	{
		logerror("Denoted the same register code in hyperstone_divs instruction. PC = %08X\n", PC);
	}
	else
	{
		if( SRC_IS_PC || SRC_IS_SR )
		{
			logerror("Denoted PC or SR as source register in hyperstone_divs instruction. PC = %08X\n", PC);
		}
		else
		{
			INT64 dividend;

			dividend = (INT64) COMBINE_64_32_32(DREG, DREGF);

			if( SREG == 0 || (DREG & 0x80000000) )
			{
				//Rd//Rdf -> undefined
				//Z -> undefined
				//N -> undefined
				UINT32 addr;
				SET_V(1);
				addr = get_trap_addr(RANGE_ERROR);
				execute_exception(addr);
			}
			else
			{
				INT32 quotient, remainder;

				/* TODO: add quotient overflow */
				quotient = dividend / ((INT32)(SREG));
				remainder = dividend % ((INT32)(SREG));

				SET_DREG(remainder);
				SET_DREGF(quotient);

				SET_Z( quotient == 0 ? 1 : 0 );
				SET_N( SIGN_BIT(quotient) );
				SET_V(0);
			}
		}
	}

	hyperstone_ICount -= 36;
}

static void hyperstone_xm(void)
{
	if( SRC_IS_SR || DST_IS_SR || DST_IS_PC )
	{
		logerror("Denoted PC or SR in hyperstone_xm. PC = %08X\n", PC);
	}
	else
	{
		switch( current_regs.sub_type ) // x_code
		{
			case 0:
			case 1:
			case 2:
			case 3:
				if( !SRC_IS_PC && (SREG > EXTRA_U) )
				{
					UINT32 addr = get_trap_addr(RANGE_ERROR);
					execute_exception(addr);
				}
				else if( SRC_IS_PC && (SREG >= EXTRA_U) )
				{
					UINT32 addr = get_trap_addr(RANGE_ERROR);
					execute_exception(addr);
				}
				else
				{
					SREG <<= current_regs.sub_type;
				}

				break;

			case 4:
			case 5:
			case 6:
			case 7:
				current_regs.sub_type -= 4;
				SREG <<= current_regs.sub_type;

				break;
		}

		SET_DREG(SREG);
	}

	hyperstone_ICount -= 1;
}

static void hyperstone_mask(void)
{
	DREG = SREG & EXTRA_U;

	SET_DREG(DREG);
	SET_Z( DREG == 0 ? 1 : 0 );

	hyperstone_ICount -= 1;
}

static void hyperstone_sum(void)
{
	UINT64 tmp;

	if( SRC_IS_SR )
		SREG = GET_C;

	tmp = (UINT64)(SREG) + (UINT64)(EXTRA_U);
	CHECK_C(tmp);
	CHECK_VADD(SREG,EXTRA_U,tmp);

	DREG = SREG + EXTRA_U;

	SET_DREG(DREG);

	if( DST_IS_PC )
		SET_M(0);

	SET_Z( DREG == 0 ? 1 : 0 );
	SET_N( SIGN_BIT(DREG) );

	hyperstone_ICount -= 1;
}

static void hyperstone_sums(void)
{
	INT32 res;
	INT64 tmp;

	if( SRC_IS_SR )
		SREG = GET_C;

	tmp = (INT64)((INT32)(SREG)) + (INT64)(EXTRA_S);
	CHECK_VADD(SREG,EXTRA_S,tmp);

//#if SETCARRYS
//  CHECK_C(tmp);
//#endif

	res = (INT32)(SREG) + EXTRA_S;

	SET_DREG(res);

	SET_Z( res == 0 ? 1 : 0 );
	SET_N( SIGN_BIT(res) );

	hyperstone_ICount -= 1;

	if( GET_V && !SRC_IS_SR )
	{
		UINT32 addr = get_trap_addr(RANGE_ERROR);
		execute_exception(addr);
	}
}

static void hyperstone_cmp(void)
{
	UINT64 tmp;

	if( SRC_IS_SR )
		SREG = GET_C;

	if( DREG == SREG )
		SET_Z(1);
	else
		SET_Z(0);

	if( (INT32) DREG < (INT32) SREG )
		SET_N(1);
	else
		SET_N(0);

	tmp = (UINT64)(DREG) - (UINT64)(SREG);
	CHECK_VSUB(SREG,DREG,tmp);

	if( DREG < SREG )
		SET_C(1);
	else
		SET_C(0);

	hyperstone_ICount -= 1;
}

static void hyperstone_mov(void)
{
	if( !GET_S && current_regs.dst >= 16 )
	{
		UINT32 addr = get_trap_addr(PRIVILEGE_ERROR);
		execute_exception(addr);
	}

	SET_DREG(SREG);

	if( DST_IS_PC )
		SET_M(0);

	SET_Z( SREG == 0 ? 1 : 0 );
	SET_N( SIGN_BIT(SREG) );

	hyperstone_ICount -= 1;
}


static void hyperstone_add(void)
{
	UINT64 tmp;

	if( SRC_IS_SR )
		SREG = GET_C;

	tmp = (UINT64)(SREG) + (UINT64)(DREG);
	CHECK_C(tmp);
	CHECK_VADD(SREG,DREG,tmp);

	DREG = SREG + DREG;
	SET_DREG(DREG);

	if( DST_IS_PC )
		SET_M(0);

	SET_Z( DREG == 0 ? 1 : 0 );
	SET_N( SIGN_BIT(DREG) );

	hyperstone_ICount -= 1;
}

static void hyperstone_adds(void)
{
	INT32 res;
	INT64 tmp;

	if( SRC_IS_SR )
		SREG = GET_C;

	tmp = (INT64)((INT32)(SREG)) + (INT64)((INT32)(DREG));

	CHECK_VADD(SREG,DREG,tmp);

//#if SETCARRYS
//  CHECK_C(tmp);
//#endif

	res = (INT32)(SREG) + (INT32)(DREG);

	SET_DREG(res);
	SET_Z( res == 0 ? 1 : 0 );
	SET_N( SIGN_BIT(res) );

	hyperstone_ICount -= 1;

	if( GET_V )
	{
		UINT32 addr = get_trap_addr(RANGE_ERROR);
		execute_exception(addr);
	}
}

static void hyperstone_cmpb(void)
{
	SET_Z( (DREG & SREG) == 0 ? 1 : 0 );

	hyperstone_ICount -= 1;
}

static void hyperstone_andn(void)
{
	DREG = DREG & ~SREG;

	SET_DREG(DREG);
	SET_Z( DREG == 0 ? 1 : 0 );

	hyperstone_ICount -= 1;
}

static void hyperstone_or(void)
{
	DREG = DREG | SREG;

	SET_DREG(DREG);
	SET_Z( DREG == 0 ? 1 : 0 );

	hyperstone_ICount -= 1;
}

static void hyperstone_xor(void)
{
	DREG = DREG ^ SREG;

	SET_DREG(DREG);
	SET_Z( DREG == 0 ? 1 : 0 );

	hyperstone_ICount -= 1;
}

static void hyperstone_subc(void)
{
	UINT64 tmp;

	if( SRC_IS_SR )
	{
		tmp = (UINT64)(DREG) - (UINT64)(GET_C);
		CHECK_VSUB(GET_C,DREG,tmp);
	}
	else
	{
		tmp = (UINT64)(DREG) - ((UINT64)(SREG) + (UINT64)(GET_C));
		//CHECK!
		CHECK_VSUB(SREG + GET_C,DREG,tmp);
	}


	if( SRC_IS_SR )
	{
		DREG = DREG - GET_C;
	}
	else
	{
		DREG = DREG - (SREG + GET_C);
	}

	CHECK_C(tmp);

	SET_DREG(DREG);

	SET_Z( GET_Z & (DREG == 0 ? 1 : 0) );
	SET_N( SIGN_BIT(DREG) );

	hyperstone_ICount -= 1;
}

static void hyperstone_not(void)
{
	SET_DREG(~SREG);
	SET_Z( ~SREG == 0 ? 1 : 0 );

	hyperstone_ICount -= 1;
}

static void hyperstone_sub(void)
{
	UINT64 tmp;

	if( SRC_IS_SR )
		SREG = GET_C;

	tmp = (UINT64)(DREG) - (UINT64)(SREG);
	CHECK_C(tmp);
	CHECK_VSUB(SREG,DREG,tmp);

	DREG = DREG - SREG;
	SET_DREG(DREG);

	if( DST_IS_PC )
		SET_M(0);

	SET_Z( DREG == 0 ? 1 : 0 );
	SET_N( SIGN_BIT(DREG) );

	hyperstone_ICount -= 1;
}

static void hyperstone_subs(void)
{
	INT32 res;
	INT64 tmp;

	if( SRC_IS_SR )
		SREG = GET_C;

	tmp = (INT64)((INT32)(DREG)) - (INT64)((INT32)(SREG));

//#ifdef SETCARRYS
//  CHECK_C(tmp);
//#endif

	CHECK_VSUB(SREG,DREG,tmp);

	res = (INT32)(DREG) - (INT32)(SREG);

	SET_DREG(res);

	SET_Z( res == 0 ? 1 : 0 );
	SET_N( SIGN_BIT(res) );

	hyperstone_ICount -= 1;

	if( GET_V )
	{
		UINT32 addr = get_trap_addr(RANGE_ERROR);
		execute_exception(addr);
	}
}

static void hyperstone_addc(void)
{
	UINT64 tmp;

	if( SRC_IS_SR )
	{
		tmp = (UINT64)(DREG) + (UINT64)(GET_C);
		CHECK_VADD(DREG,GET_C,tmp);
	}
	else
	{
		tmp = (UINT64)(SREG) + (UINT64)(DREG) + (UINT64)(GET_C);

		//CHECK!
		//CHECK_VADD1: V = (DREG == 0x7FFF) && (C == 1);
		//OVERFLOW = CHECK_VADD1(DREG, C, DREG+C) | CHECK_VADD(SREG, DREG+C, SREG+DREG+C)
		/* check if DREG + GET_C overflows */
//      if( (DREG == 0x7FFFFFFF) && (GET_C == 1) )
//          SET_V(1);
//      else
//          CHECK_VADD(SREG,DREG + GET_C,tmp);

		CHECK_VADD3(SREG,DREG,GET_C,tmp);
	}



	if( SRC_IS_SR )
		DREG = DREG + GET_C;
	else
		DREG = SREG + DREG + GET_C;

	CHECK_C(tmp);

	SET_DREG(DREG);
	SET_Z( GET_Z & (DREG == 0 ? 1 : 0) );
	SET_N( SIGN_BIT(DREG) );

	hyperstone_ICount -= 1;
}

static void hyperstone_and(void)
{
	DREG = DREG & SREG;

	SET_DREG(DREG);
	SET_Z( DREG == 0 ? 1 : 0 );

	hyperstone_ICount -= 1;
}

static void hyperstone_neg(void)
{
	UINT64 tmp;

	if( SRC_IS_SR )
		SREG = GET_C;

	tmp = -(UINT64)(SREG);
	CHECK_C(tmp);
	CHECK_VSUB(SREG,0,tmp);

	DREG = -SREG;

	SET_DREG(DREG);

	SET_Z( DREG == 0 ? 1 : 0 );
	SET_N( SIGN_BIT(DREG) );

	hyperstone_ICount -= 1;
}

static void hyperstone_negs(void)
{
	INT32 res;
	INT64 tmp;

	if( SRC_IS_SR )
		SREG = GET_C;

	tmp = -(INT64)((INT32)(SREG));
	CHECK_VSUB(SREG,0,tmp);

//#if SETCARRYS
//  CHECK_C(tmp);
//#endif

	res = -(INT32)(SREG);

	SET_DREG(res);

	SET_Z( res == 0 ? 1 : 0 );
	SET_N( SIGN_BIT(res) );


	hyperstone_ICount -= 1;

	if( GET_V && !SRC_IS_SR ) //trap doesn't occur when source is SR
	{
		UINT32 addr = get_trap_addr(RANGE_ERROR);
		execute_exception(addr);
	}
}

static void hyperstone_cmpi(void)
{
	UINT64 tmp;

	tmp = (UINT64)(DREG) - (UINT64)(EXTRA_U);
	CHECK_VSUB(EXTRA_U,DREG,tmp);

	if( DREG == EXTRA_U )
		SET_Z(1);
	else
		SET_Z(0);

	if( (INT32) DREG < (INT32) EXTRA_U )
		SET_N(1);
	else
		SET_N(0);

	if( DREG < EXTRA_U )
		SET_C(1);
	else
		SET_C(0);

	hyperstone_ICount -= 1;
}

static void hyperstone_movi(void)
{
	if( !GET_S && current_regs.dst >= 16 )
	{
		UINT32 addr = get_trap_addr(PRIVILEGE_ERROR);
		execute_exception(addr);
	}

	SET_DREG(EXTRA_U);

	if( DST_IS_PC )
		SET_M(0);

	SET_Z( EXTRA_U == 0 ? 1 : 0 );
	SET_N( SIGN_BIT(EXTRA_U) );

#if MISSIONCRAFT_FLAGS
	SET_V(0); // or V undefined ?
#endif

	hyperstone_ICount -= 1;
}

static void hyperstone_addi(void)
{
	UINT32 imm;
	UINT64 tmp;

	if( N_VALUE )
		imm = EXTRA_U;
	else
		imm = GET_C & ((GET_Z == 0 ? 1 : 0) | (DREG & 0x01));


	tmp = (UINT64)(imm) + (UINT64)(DREG);
	CHECK_C(tmp);
	CHECK_VADD(imm,DREG,tmp);

	DREG = imm + DREG;
	SET_DREG(DREG);

	if( DST_IS_PC )
		SET_M(0);

	SET_Z( DREG == 0 ? 1 : 0 );
	SET_N( SIGN_BIT(DREG) );

	hyperstone_ICount -= 1;
}

static void hyperstone_addsi(void)
{
	INT32 imm, res;
	INT64 tmp;

	if( N_VALUE )
		imm = EXTRA_S;
	else
		imm = GET_C & ((GET_Z == 0 ? 1 : 0) | (DREG & 0x01));

	tmp = (INT64)(imm) + (INT64)((INT32)(DREG));
	CHECK_VADD(imm,DREG,tmp);

//#if SETCARRYS
//  CHECK_C(tmp);
//#endif

	res = imm + (INT32)(DREG);

	SET_DREG(res);

	SET_Z( res == 0 ? 1 : 0 );
	SET_N( SIGN_BIT(res) );

	hyperstone_ICount -= 1;

	if( GET_V )
	{
		UINT32 addr = get_trap_addr(RANGE_ERROR);
		execute_exception(addr);
	}
}

static void hyperstone_cmpbi(void)
{
	UINT32 imm;

	if( N_VALUE )
	{
		if( N_VALUE == 31 )
		{
			imm = 0x7fffffff; // bit 31 = 0, others = 1
		}
		else
		{
			imm = EXTRA_U;
		}

		SET_Z( (DREG & imm) == 0 ? 1 : 0 );
	}
	else
	{
		if( (DREG & 0xff000000) == 0 || (DREG & 0x00ff0000) == 0 ||
			(DREG & 0x0000ff00) == 0 || (DREG & 0x000000ff) == 0 )
			SET_Z(1);
		else
			SET_Z(0);
	}

	hyperstone_ICount -= 1;
}

static void hyperstone_andni(void)
{
	UINT32 imm;

	if( N_VALUE == 31 )
		imm = 0x7fffffff; // bit 31 = 0, others = 1
	else
		imm = EXTRA_U;

	DREG = DREG & ~imm;

	SET_DREG(DREG);
	SET_Z( DREG == 0 ? 1 : 0 );

	hyperstone_ICount -= 1;
}

static void hyperstone_ori(void)
{
	DREG = DREG | EXTRA_U;

	SET_DREG(DREG);
	SET_Z( DREG == 0 ? 1 : 0 );

	hyperstone_ICount -= 1;
}

static void hyperstone_xori(void)
{
	DREG = DREG ^ EXTRA_U;

	SET_DREG(DREG);
	SET_Z( DREG == 0 ? 1 : 0 );

	hyperstone_ICount -= 1;
}

static void hyperstone_shrdi(void)
{
	UINT32 low_order, high_order;
	UINT64 val;

	high_order = DREG;
	low_order  = DREGF;

	val = COMBINE_U64_U32_U32(high_order, low_order);

	if( N_VALUE )
		SET_C((val >> (N_VALUE - 1)) & 1);
	else
		SET_C(0);

	val >>= N_VALUE;

	high_order = HI32_32_64(val);
	low_order  = LO32_32_64(val);

	SET_DREG(high_order);
	SET_DREGF(low_order);
	SET_Z( val == 0 ? 1 : 0 );
	SET_N( SIGN_BIT(high_order) );

	hyperstone_ICount -= 2;
}

static void hyperstone_shrd(void)
{
	UINT32 low_order, high_order;
	UINT64 val;
	UINT8 n = SREG & 0x1f;

	// result undefined if Ls denotes the same register as Ld or Ldf
	if( SAME_SRC_DST || SAME_SRC_DSTF )
	{
		logerror("Denoted same registers in hyperstone_shrd. PC = %08X\n", PC);
	}
	else
	{
		high_order = DREG;
		low_order  = DREGF;

		val = COMBINE_U64_U32_U32(high_order, low_order);

		if( n )
			SET_C((val >> (n - 1)) & 1);
		else
			SET_C(0);

		val >>= n;

		high_order = HI32_32_64(val);
		low_order  = LO32_32_64(val);

		SET_DREG(high_order);
		SET_DREGF(low_order);

		SET_Z( val == 0 ? 1 : 0 );
		SET_N( SIGN_BIT(high_order) );
	}

	hyperstone_ICount -= 2;
}

static void hyperstone_shr(void)
{
	UINT32 ret;
	UINT8 n;

	n = SREG & 0x1f;
	ret = DREG;

	if( n )
		SET_C((ret >> (n - 1)) & 1);
	else
		SET_C(0);

	ret >>= n;

	SET_DREG(ret);
	SET_Z( ret == 0 ? 1 : 0 );
	SET_N( SIGN_BIT(ret) );

	hyperstone_ICount -= 1;
}

static void hyperstone_sardi(void)
{
	UINT32 low_order, high_order;
	UINT64 val;
	UINT8 sign_bit;

	high_order = DREG;
	low_order  = DREGF;

	val = COMBINE_64_32_32(high_order, low_order);

	if( N_VALUE )
		SET_C((val >> (N_VALUE - 1)) & 1);
	else
		SET_C(0);

	sign_bit = val >> 63;
	val >>= N_VALUE;

	if( sign_bit )
	{
		int i;
		for( i = 0; i < N_VALUE; i++ )
		{
			val |= (U64(0x8000000000000000) >> i);
		}
	}

	high_order = val >> 32;
	low_order  = val & 0xffffffff;

	SET_DREG(high_order);
	SET_DREGF(low_order);

	SET_Z( val == 0 ? 1 : 0 );
	SET_N( SIGN_BIT(high_order) );

	hyperstone_ICount -= 2;
}

static void hyperstone_sard(void)
{
	UINT32 low_order, high_order;
	UINT64 val;
	UINT8 n, sign_bit;

	n = SREG & 0x1f;

	// result undefined if Ls denotes the same register as Ld or Ldf
	if( SAME_SRC_DST || SAME_SRC_DSTF )
	{
		logerror("Denoted same registers in hyperstone_sard. PC = %08X\n", PC);
	}
	else
	{
		high_order = DREG;
		low_order  = DREGF;

		val = COMBINE_64_32_32(high_order, low_order);

		if( n )
			SET_C((val >> (n - 1)) & 1);
		else
			SET_C(0);

		sign_bit = val >> 63;

		val >>= n;

		if( sign_bit )
		{
			int i;
			for( i = 0; i < n; i++ )
			{
				val |= (U64(0x8000000000000000) >> i);
			}
		}

		high_order = val >> 32;
		low_order  = val & 0xffffffff;

		SET_DREG(high_order);
		SET_DREGF(low_order);
		SET_Z( val == 0 ? 1 : 0 );
		SET_N( SIGN_BIT(high_order) );
	}

	hyperstone_ICount -= 2;
}

static void hyperstone_sar(void)
{
	UINT32 ret;
	UINT8 n, sign_bit;

	n = SREG & 0x1f;
	ret = DREG;
	sign_bit = (ret & 0x80000000) >> 31;

	if( n )
		SET_C((ret >> (n - 1)) & 1);
	else
		SET_C(0);

	ret >>= n;

	if( sign_bit )
	{
		int i;
		for( i = 0; i < n; i++ )
		{
			ret |= (0x80000000 >> i);
		}
	}

	SET_DREG(ret);
	SET_Z( ret == 0 ? 1 : 0 );
	SET_N( SIGN_BIT(ret) );

	hyperstone_ICount -= 1;
}

static void hyperstone_shldi(void)
{
	UINT32 low_order, high_order, tmp;
	UINT64 val, mask;

	high_order = DREG;
	low_order  = DREGF;

	val  = COMBINE_U64_U32_U32(high_order, low_order);
	SET_C( (N_VALUE)?(((val<<(N_VALUE-1))&U64(0x8000000000000000))?1:0):0);
	mask = ((((UINT64)1) << (32 - N_VALUE)) - 1) ^ 0xffffffff;
	tmp  = high_order << N_VALUE;

	if( ((high_order & mask) && (!(tmp & 0x80000000))) ||
			(((high_order & mask) ^ mask) && (tmp & 0x80000000)) )
		SET_V(1);
	else
		SET_V(0);

	val <<= N_VALUE;

	high_order = HI32_32_64(val);
	low_order  = LO32_32_64(val);

	SET_DREG(high_order);
	SET_DREGF(low_order);

	SET_Z( val == 0 ? 1 : 0 );
	SET_N( SIGN_BIT(high_order) );

	hyperstone_ICount -= 2;
}

static void hyperstone_shld(void)
{
	UINT32 low_order, high_order, tmp, n;
	UINT64 val, mask;

	n = SREG & 0x1f;

	// result undefined if Ls denotes the same register as Ld or Ldf
	if( SAME_SRC_DST || SAME_SRC_DSTF )
	{
		logerror("Denoted same registers in hyperstone_shld. PC = %08X\n", PC);
	}
	else
	{
		high_order = DREG;
		low_order  = DREGF;

		mask = ((((UINT64)1) << (32 - n)) - 1) ^ 0xffffffff;

		val = COMBINE_U64_U32_U32(high_order, low_order);
		SET_C( (n)?(((val<<(n-1))&U64(0x8000000000000000))?1:0):0);
		tmp = high_order << n;

		if( ((high_order & mask) && (!(tmp & 0x80000000))) ||
				(((high_order & mask) ^ mask) && (tmp & 0x80000000)) )
			SET_V(1);
		else
			SET_V(0);

		val <<= n;

		high_order = HI32_32_64(val);
		low_order  = LO32_32_64(val);

		SET_DREG(high_order);
		SET_DREGF(low_order);

		SET_Z( val == 0 ? 1 : 0 );
		SET_N( SIGN_BIT(high_order) );
	}

	hyperstone_ICount -= 2;
}

static void hyperstone_shl(void)
{
	UINT32 base, ret, n;
	UINT64 mask;

	n    = SREG & 0x1f;
	base = DREG;
	mask = ((((UINT64)1) << (32 - n)) - 1) ^ 0xffffffff;
	SET_C( (n)?(((base<<(n-1))&0x80000000)?1:0):0);
	ret  = base << n;

	if( ((base & mask) && (!(ret & 0x80000000))) ||
			(((base & mask) ^ mask) && (ret & 0x80000000)) )
		SET_V(1);
	else
		SET_V(0);

	SET_DREG(ret);
	SET_Z( ret == 0 ? 1 : 0 );
	SET_N( SIGN_BIT(ret) );

	hyperstone_ICount -= 1;
}

static void reserved(void)
{
	logerror("Executed Reserved opcode. PC = %08X OP = %04X\n", PC, OP);
}

static void hyperstone_testlz(void)
{
	UINT8 zeros = 0;
	UINT32 mask;

	for( mask = 0x80000000; ; mask >>= 1 )
	{
		if( SREG & mask )
			break;
		else
			zeros++;

		if( zeros == 32 )
			break;
	}

	SET_DREG(zeros);

	hyperstone_ICount -= 2;
}

static void hyperstone_rol(void)
{
	UINT32 val, base;
	UINT8 n;
	UINT64 mask;

	n = SREG & 0x1f;

	val = base = DREG;

	mask = ((((UINT64)1) << (32 - n)) - 1) ^ 0xffffffff;

	while( n > 0 )
	{
		val = (val << 1) | ((val & 0x80000000) >> 31);
		n--;
	}

#ifdef MISSIONCRAFT_FLAGS

	if( ((base & mask) && (!(val & 0x80000000))) ||
			(((base & mask) ^ mask) && (val & 0x80000000)) )
		SET_V(1);
	else
		SET_V(0);

#endif

	SET_DREG(val);

	SET_Z( val == 0 ? 1 : 0 );
	SET_N( SIGN_BIT(val) );

	hyperstone_ICount -= 1;
}

//TODO: add trap error
static void hyperstone_ldxx1(void)
{
	UINT32 load;

	if( DST_IS_SR )
	{
		switch( current_regs.sub_type )
		{
			case 0: // LDBS.A

				load = READ_B(EXTRA_S);
				load |= (load & 0x80) ? 0xffffff00 : 0;
				SET_SREG(load);

				break;

			case 1: // LDBU.A

				load = READ_B(EXTRA_S);
				SET_SREG(load);

				break;

			case 2:

				load = READ_HW(EXTRA_S & ~1);

				if( EXTRA_S & 1 ) // LDHS.A
				{
					load |= (load & 0x8000) ? 0xffff0000 : 0;
				}
				/*
                else          // LDHU.A
                {
                    // nothing more
                }
                */

				SET_SREG(load);

				break;

			case 3:

				if( (EXTRA_S & 3) == 3 )      // LDD.IOA
				{
					load = IO_READ_W(EXTRA_S & ~3);
					SET_SREG(load);

					load = IO_READ_W((EXTRA_S & ~3) + 4);
					SET_SREGF(load);

					hyperstone_ICount -= 1; // extra cycle
				}
				else if( (EXTRA_S & 3) == 2 ) // LDW.IOA
				{
					load = IO_READ_W(EXTRA_S & ~3);
					SET_SREG(load);
				}
				else if( (EXTRA_S & 3) == 1 ) // LDD.A
				{
					load = READ_W(EXTRA_S & ~1);
					SET_SREG(load);

					load = READ_W((EXTRA_S & ~1) + 4);
					SET_SREGF(load);

					hyperstone_ICount -= 1; // extra cycle
				}
				else                      // LDW.A
				{
					load = READ_W(EXTRA_S & ~1);
					SET_SREG(load);
				}

				break;
		}
	}
	else
	{
		switch( current_regs.sub_type )
		{
			case 0: // LDBS.D

				load = READ_B(DREG + EXTRA_S);
				load |= (load & 0x80) ? 0xffffff00 : 0;
				SET_SREG(load);

				break;

			case 1: // LDBU.D

				load = READ_B(DREG + EXTRA_S);
				SET_SREG(load);

				break;

			case 2:

				load = READ_HW(DREG + (EXTRA_S & ~1));

				if( EXTRA_S & 1 ) // LDHS.D
				{
					load |= (load & 0x8000) ? 0xffff0000 : 0;
				}
				/*
                else          // LDHU.D
                {
                    // nothing more
                }
                */

				SET_SREG(load);

				break;

			case 3:

				if( (EXTRA_S & 3) == 3 )      // LDD.IOD
				{
					load = IO_READ_W(DREG + (EXTRA_S & ~3));
					SET_SREG(load);

					load = IO_READ_W(DREG + (EXTRA_S & ~3) + 4);
					SET_SREGF(load);

					hyperstone_ICount -= 1; // extra cycle
				}
				else if( (EXTRA_S & 3) == 2 ) // LDW.IOD
				{
					load = IO_READ_W(DREG + (EXTRA_S & ~3));
					SET_SREG(load);
				}
				else if( (EXTRA_S & 3) == 1 ) // LDD.D
				{
					load = READ_W(DREG + (EXTRA_S & ~1));
					SET_SREG(load);

					load = READ_W(DREG + (EXTRA_S & ~1) + 4);
					SET_SREGF(load);

					hyperstone_ICount -= 1; // extra cycle
				}
				else                      // LDW.D
				{
					load = READ_W(DREG + (EXTRA_S & ~1));
					SET_SREG(load);
				}

				break;
		}
	}

	hyperstone_ICount -= 1;
}

static void hyperstone_ldxx2(void)
{
	UINT32 load;

	if( DST_IS_PC || DST_IS_SR )
	{
		logerror("Denoted PC or SR in hyperstone_ldxx2. PC = %08X\n", PC);
	}
	else
	{
		switch( current_regs.sub_type )
		{
			case 0: // LDBS.N

				if(SAME_SRC_DST)
					ui_popup("LDBS.N denoted same regs @ %08X",PPC);

				load = READ_B(DREG);
				load |= (load & 0x80) ? 0xffffff00 : 0;
				SET_SREG(load);

				if(!SAME_SRC_DST)
					SET_DREG(DREG + EXTRA_S);

				break;

			case 1: // LDBU.N

				if(SAME_SRC_DST)
					ui_popup("LDBU.N denoted same regs @ %08X",PPC);

				load = READ_B(DREG);
				SET_SREG(load);

				if(!SAME_SRC_DST)
					SET_DREG(DREG + EXTRA_S);

				break;

			case 2:

				load = READ_HW(DREG);

				if( EXTRA_S & 1 ) // LDHS.N
				{
					load |= (load & 0x8000) ? 0xffff0000 : 0;

					if(SAME_SRC_DST)
						ui_popup("LDHS.N denoted same regs @ %08X",PPC);
				}
				/*
                else          // LDHU.N
                {
                    // nothing more
                }
                */

				SET_SREG(load);

				if(!SAME_SRC_DST)
					SET_DREG(DREG + (EXTRA_S & ~1));

				break;

			case 3:

				if( (EXTRA_S & 3) == 3 )      // LDW.S
				{
					if(SAME_SRC_DST)
						ui_popup("LDW.S denoted same regs @ %08X",PPC);

					if(DREG < SP)
						SET_SREG(READ_W(DREG));
					else
						SET_SREG(GET_ABS_L_REG((DREG & 0xfc) >> 2));

					if(!SAME_SRC_DST)
						SET_DREG(DREG + (EXTRA_S & ~3));

					hyperstone_ICount -= 2; // extra cycles
				}
				else if( (EXTRA_S & 3) == 2 ) // Reserved
				{
					logerror("Executed Reserved instruction in hyperstone_ldxx2. PC = %08X\n", PC);
				}
				else if( (EXTRA_S & 3) == 1 ) // LDD.N
				{
					if(SAME_SRC_DST || SAME_SRCF_DST)
						ui_popup("LDD.N denoted same regs @ %08X",PPC);

					load = READ_W(DREG);
					SET_SREG(load);

					load = READ_W(DREG + 4);
					SET_SREGF(load);

					if(!SAME_SRC_DST && !SAME_SRCF_DST)
						SET_DREG(DREG + (EXTRA_S & ~1));

					hyperstone_ICount -= 1; // extra cycle
				}
				else                      // LDW.N
				{
					if(SAME_SRC_DST)
						ui_popup("LDW.N denoted same regs @ %08X",PPC);

					load = READ_W(DREG);
					SET_SREG(load);

					if(!SAME_SRC_DST)
						SET_DREG(DREG + (EXTRA_S & ~1));
				}

				break;
		}
	}

	hyperstone_ICount -= 1;
}

//TODO: add trap error
static void hyperstone_stxx1(void)
{
	if( SRC_IS_SR )
		SREG = SREGF = 0;

	if( DST_IS_SR )
	{
		switch( current_regs.sub_type )
		{
			case 0: // STBS.A

				/* TODO: missing trap on range error */
				WRITE_B(EXTRA_S, SREG & 0xff);

				break;

			case 1: // STBU.A

				WRITE_B(EXTRA_S, SREG & 0xff);

				break;

			case 2:

				WRITE_HW(EXTRA_S & ~1, SREG & 0xffff);

				/*
                if( EXTRA_S & 1 ) // STHS.A
                {
                    // TODO: missing trap on range error
                }
                else          // STHU.A
                {
                    // nothing more
                }
                */

				break;

			case 3:

				if( (EXTRA_S & 3) == 3 )      // STD.IOA
				{
					IO_WRITE_W(EXTRA_S & ~3, SREG);
					IO_WRITE_W((EXTRA_S & ~3) + 4, SREGF);

					hyperstone_ICount -= 1; // extra cycle
				}
				else if( (EXTRA_S & 3) == 2 ) // STW.IOA
				{
					IO_WRITE_W(EXTRA_S & ~3, SREG);
				}
				else if( (EXTRA_S & 3) == 1 ) // STD.A
				{
					WRITE_W(EXTRA_S & ~1, SREG);
					WRITE_W((EXTRA_S & ~1) + 4, SREGF);

					hyperstone_ICount -= 1; // extra cycle
				}
				else                      // STW.A
				{
					WRITE_W(EXTRA_S & ~1, SREG);
				}

				break;
		}
	}
	else
	{
		switch( current_regs.sub_type )
		{
			case 0: // STBS.D

				/* TODO: missing trap on range error */
				WRITE_B(DREG + EXTRA_S, SREG & 0xff);

				break;

			case 1: // STBU.D

				WRITE_B(DREG + EXTRA_S, SREG & 0xff);

				break;

			case 2:

				WRITE_HW(DREG + (EXTRA_S & ~1), SREG & 0xffff);

				/*
                if( EXTRA_S & 1 ) // STHS.D
                {
                    // TODO: missing trap on range error
                }
                else          // STHU.D
                {
                    // nothing more
                }
                */

				break;

			case 3:

				if( (EXTRA_S & 3) == 3 )      // STD.IOD
				{
					IO_WRITE_W(DREG + (EXTRA_S & ~3), SREG);
					IO_WRITE_W(DREG + (EXTRA_S & ~3) + 4, SREGF);

					hyperstone_ICount -= 1; // extra cycle
				}
				else if( (EXTRA_S & 3) == 2 ) // STW.IOD
				{
					IO_WRITE_W(DREG + (EXTRA_S & ~3), SREG);
				}
				else if( (EXTRA_S & 3) == 1 ) // STD.D
				{
					WRITE_W(DREG + (EXTRA_S & ~1), SREG);
					WRITE_W(DREG + (EXTRA_S & ~1) + 4, SREGF);

					hyperstone_ICount -= 1; // extra cycle
				}
				else                      // STW.D
				{
					WRITE_W(DREG + (EXTRA_S & ~1), SREG);
				}

				break;
		}
	}

	hyperstone_ICount -= 1;
}

static void hyperstone_stxx2(void)
{
	if( SRC_IS_SR )
		SREG = SREGF = 0;

	if( DST_IS_PC || DST_IS_SR )
	{
		logerror("Denoted PC or SR in hyperstone_stxx2. PC = %08X\n", PC);
	}
	else
	{
		switch( current_regs.sub_type )
		{
			case 0: // STBS.N

				/* TODO: missing trap on range error */
				WRITE_B(DREG, SREG & 0xff);
				SET_DREG(DREG + EXTRA_S);

				break;

			case 1: // STBU.N

				WRITE_B(DREG, SREG & 0xff);
				SET_DREG(DREG + EXTRA_S);

				break;

			case 2:

				WRITE_HW(DREG, SREG & 0xffff);
				SET_DREG(DREG + (EXTRA_S & ~1));

				/*
                if( EXTRA_S & 1 ) // STHS.N
                {
                    // TODO: missing trap on range error
                }
                else          // STHU.N
                {
                    // nothing more
                }
                */

				break;

			case 3:

				if( (EXTRA_S & 3) == 3 )      // STW.S
				{
					if(DREG < SP)
						WRITE_W(DREG, SREG);
					else
					{
						if(((DREG & 0xfc) >> 2) == ((current_regs.src + GET_FP) % 64) && S_BIT == LOCAL)
							ui_popup("STW.S denoted the same local register @ %08X\n",PPC);

						SET_ABS_L_REG((DREG & 0xfc) >> 2,SREG);
					}

					SET_DREG(DREG + (EXTRA_S & ~3));

					hyperstone_ICount -= 2; // extra cycles

				}
				else if( (EXTRA_S & 3) == 2 ) // Reserved
				{
					logerror("Executed Reserved instruction in hyperstone_stxx2. PC = %08X\n", PC);
				}
				else if( (EXTRA_S & 3) == 1 ) // STD.N
				{
					WRITE_W(DREG, SREG);
					SET_DREG(DREG + (EXTRA_S & ~1));

					if( SAME_SRCF_DST )
						WRITE_W(DREG + 4, SREGF + (EXTRA_S & ~1));  // because DREG == SREGF and DREG has been incremented
					else
						WRITE_W(DREG + 4, SREGF);

					hyperstone_ICount -= 1; // extra cycle
				}
				else                      // STW.N
				{
					WRITE_W(DREG, SREG);
					SET_DREG(DREG + (EXTRA_S & ~1));
				}

				break;
		}
	}

	hyperstone_ICount -= 1;
}

static void hyperstone_shri(void)
{
	UINT32 val;

	val = DREG;

	if( N_VALUE )
		SET_C((val >> (N_VALUE - 1)) & 1);
	else
		SET_C(0);

	val >>= N_VALUE;

	SET_DREG(val);
	SET_Z( val == 0 ? 1 : 0 );
	SET_N( SIGN_BIT(val) );

	hyperstone_ICount -= 1;
}

static void hyperstone_sari(void)
{
	UINT32 val;
	UINT8 sign_bit;

	val = DREG;
	sign_bit = (val & 0x80000000) >> 31;

	if( N_VALUE )
		SET_C((val >> (N_VALUE - 1)) & 1);
	else
		SET_C(0);

	val >>= N_VALUE;

	if( sign_bit )
	{
		int i;
		for( i = 0; i < N_VALUE; i++ )
		{
			val |= (0x80000000 >> i);
		}
	}

	SET_DREG(val);
	SET_Z( val == 0 ? 1 : 0 );
	SET_N( SIGN_BIT(val) );

	hyperstone_ICount -= 1;
}

static void hyperstone_shli(void)
{
	UINT32 val, val2;
	UINT64 mask;

	val  = DREG;
	SET_C( (N_VALUE)?(((val<<(N_VALUE-1))&0x80000000)?1:0):0);
	mask = ((((UINT64)1) << (32 - N_VALUE)) - 1) ^ 0xffffffff;
	val2 = val << N_VALUE;

	if( ((val & mask) && (!(val2 & 0x80000000))) ||
			(((val & mask) ^ mask) && (val2 & 0x80000000)) )
		SET_V(1);
	else
		SET_V(0);

	SET_DREG(val2);
	SET_Z( val2 == 0 ? 1 : 0 );
	SET_N( SIGN_BIT(val2) );

	hyperstone_ICount -= 1;
}

static void hyperstone_mulu(void)
{
	UINT32 low_order, high_order;
	UINT64 double_word;

	// PC or SR aren't denoted, else result is undefined
	if( SRC_IS_PC || SRC_IS_SR || DST_IS_PC || DST_IS_SR  )
	{
		logerror("Denoted PC or SR in hyperstone_mulu instruction. PC = %08X\n", PC);
	}
	else
	{
		double_word = (UINT64)SREG *(UINT64)DREG;

		low_order = double_word & 0xffffffff;
		high_order = double_word >> 32;

		SET_DREG(high_order);
		SET_DREGF(low_order);

		SET_Z( double_word == 0 ? 1 : 0 );
		SET_N( SIGN_BIT(high_order) );
	}

	if(SREG <= 0xffff && DREG <= 0xffff)
		hyperstone_ICount -= 4;
	else
		hyperstone_ICount -= 6;
}

static void hyperstone_muls(void)
{
	UINT32 low_order, high_order;
	INT64 double_word;

	// PC or SR aren't denoted, else result is undefined
	if( SRC_IS_PC || SRC_IS_SR || DST_IS_PC || DST_IS_SR  )
	{
		logerror("Denoted PC or SR in hyperstone_muls instruction. PC = %08X\n", PC);
	}
	else
	{
		double_word = (INT64)(INT32)(SREG) * (INT64)(INT32)(DREG);
		low_order = double_word & 0xffffffff;
		high_order = double_word >> 32;

		SET_DREG(high_order);
		SET_DREGF(low_order);

		SET_Z( double_word == 0 ? 1 : 0 );
		SET_N( SIGN_BIT(high_order) );
	}

	if((SREG >= 0xffff8000 && SREG <= 0x7fff) && (DREG >= 0xffff8000 && DREG <= 0x7fff))
		hyperstone_ICount -= 4;
	else
		hyperstone_ICount -= 6;
}

static void hyperstone_set(void)
{
	int n = N_VALUE;

	if( DST_IS_PC )
	{
		logerror("Denoted PC in hyperstone_set. PC = %08X\n", PC);
	}
	else if( DST_IS_SR )
	{
		//TODO: add fetch opcode when there's the pipeline

		//TODO: no 1!
		hyperstone_ICount -= 1;
	}
	else
	{
		switch( n )
		{
			// SETADR
			case 0:
			{
				UINT32 val;
				val =  (SP & 0xfffffe00) | (GET_FP << 2);

				//plus carry into bit 9
				val += (( (SP & 0x100) && (SIGN_BIT(SR) == 0) ) ? 1 : 0);

				SET_DREG(val);

				break;
			}
			// Reserved
			case 1:
			case 16:
			case 17:
			case 19:
				logerror("Used reserved N value (%d) in hyperstone_set. PC = %08X\n", n, PC);
				break;

			// SETxx
			case 2:
				SET_DREG(1);
				break;

			case 3:
				SET_DREG(0);
				break;

			case 4:
				if( GET_N || GET_Z )
				{
					SET_DREG(1);
				}
				else
				{
					SET_DREG(0);
				}

				break;

			case 5:
				if( !GET_N && !GET_Z )
				{
					SET_DREG(1);
				}
				else
				{
					SET_DREG(0);
				}

				break;

			case 6:
				if( GET_N )
				{
					SET_DREG(1);
				}
				else
				{
					SET_DREG(0);
				}

				break;

			case 7:
				if( !GET_N )
				{
					SET_DREG(1);
				}
				else
				{
					SET_DREG(0);
				}

				break;

			case 8:
				if( GET_C || GET_Z )
				{
					SET_DREG(1);
				}
				else
				{
					SET_DREG(0);
				}

				break;

			case 9:
				if( !GET_C && !GET_Z )
				{
					SET_DREG(1);
				}
				else
				{
					SET_DREG(0);
				}

				break;

			case 10:
				if( GET_C )
				{
					SET_DREG(1);
				}
				else
				{
					SET_DREG(0);
				}

				break;

			case 11:
				if( !GET_C )
				{
					SET_DREG(1);
				}
				else
				{
					SET_DREG(0);
				}

				break;

			case 12:
				if( GET_Z )
				{
					SET_DREG(1);
				}
				else
				{
					SET_DREG(0);
				}

				break;

			case 13:
				if( !GET_Z )
				{
					SET_DREG(1);
				}
				else
				{
					SET_DREG(0);
				}

				break;

			case 14:
				if( GET_V )
				{
					SET_DREG(1);
				}
				else
				{
					SET_DREG(0);
				}

				break;

			case 15:
				if( !GET_V )
				{
					SET_DREG(1);
				}
				else
				{
					SET_DREG(0);
				}

				break;

			case 18:
				SET_DREG(-1);
				break;

			case 20:
				if( GET_N || GET_Z )
				{
					SET_DREG(-1);
				}
				else
				{
					SET_DREG(0);
				}

				break;

			case 21:
				if( !GET_N && !GET_Z )
				{
					SET_DREG(-1);
				}
				else
				{
					SET_DREG(0);
				}

				break;

			case 22:
				if( GET_N )
				{
					SET_DREG(-1);
				}
				else
				{
					SET_DREG(0);
				}

				break;

			case 23:
				if( !GET_N )
				{
					SET_DREG(-1);
				}
				else
				{
					SET_DREG(0);
				}

				break;

			case 24:
				if( GET_C || GET_Z )
				{
					SET_DREG(-1);
				}
				else
				{
					SET_DREG(0);
				}

				break;

			case 25:
				if( !GET_C && !GET_Z )
				{
					SET_DREG(-1);
				}
				else
				{
					SET_DREG(0);
				}

				break;

			case 26:
				if( GET_C )
				{
					SET_DREG(-1);
				}
				else
				{
					SET_DREG(0);
				}

				break;

			case 27:
				if( !GET_C )
				{
					SET_DREG(-1);
				}
				else
				{
					SET_DREG(0);
				}

				break;

			case 28:
				if( GET_Z )
				{
					SET_DREG(-1);
				}
				else
				{
					SET_DREG(0);
				}

				break;

			case 29:
				if( !GET_Z )
				{
					SET_DREG(-1);
				}
				else
				{
					SET_DREG(0);
				}

				break;

			case 30:
				if( GET_V )
				{
					SET_DREG(-1);
				}
				else
				{
					SET_DREG(0);
				}

				break;

			case 31:
				if( !GET_V )
				{
					SET_DREG(-1);
				}
				else
				{
					SET_DREG(0);
				}

				break;
		}

		hyperstone_ICount -= 1;
	}
}

static void hyperstone_mul(void)
{
	UINT32 single_word;

	// PC or SR aren't denoted, else result is undefined
	if( SRC_IS_PC || SRC_IS_SR || DST_IS_PC || DST_IS_SR  )
	{
		logerror("Denoted PC or SR in hyperstone_mul instruction. PC = %08X\n", PC);
	}
	else
	{
		single_word = (SREG * DREG);// & 0xffffffff; // only the low-order word is taken

		SET_DREG(single_word);

		SET_Z( single_word == 0 ? 1 : 0 );
		SET_N( SIGN_BIT(single_word) );
	}

	if((SREG >= 0xffff8000 && SREG <= 0x7fff) && (DREG >= 0xffff8000 && DREG <= 0x7fff))
		hyperstone_ICount -= 3;
	else
		hyperstone_ICount -= 5;
}

static void hyperstone_fadd(void)
{
	execute_software();
	hyperstone_ICount -= 6;
}

static void hyperstone_faddd(void)
{
	execute_software();
	hyperstone_ICount -= 6;
}

static void hyperstone_fsub(void)
{
	execute_software();
	hyperstone_ICount -= 6;
}

static void hyperstone_fsubd(void)
{
	execute_software();
	hyperstone_ICount -= 6;
}

static void hyperstone_fmul(void)
{
	execute_software();
	hyperstone_ICount -= 6;
}

static void hyperstone_fmuld(void)
{
	execute_software();
	hyperstone_ICount -= 6;
}

static void hyperstone_fdiv(void)
{
	execute_software();
	hyperstone_ICount -= 6;
}

static void hyperstone_fdivd(void)
{
	execute_software();
	hyperstone_ICount -= 6;
}

static void hyperstone_fcmp(void)
{
	execute_software();
	hyperstone_ICount -= 6;
}

static void hyperstone_fcmpd(void)
{
	execute_software();
	hyperstone_ICount -= 6;
}

static void hyperstone_fcmpu(void)
{
	execute_software();
	hyperstone_ICount -= 6;
}

static void hyperstone_fcmpud(void)
{
	execute_software();
	hyperstone_ICount -= 6;
}

static void hyperstone_fcvt(void)
{
	execute_software();
	hyperstone_ICount -= 6;
}

static void hyperstone_fcvtd(void)
{
	execute_software();
	hyperstone_ICount -= 6;
}

static void hyperstone_extend(void)
{
	//TODO: add locks, overflow error and other things
	UINT32 vals, vald;

	vals = SREG;
	vald = DREG;

	switch( EXTRA_U ) // extended opcode
	{
		// signed or unsigned multiplication, single word product
		case EMUL:
		case 0x100: // used in "N" type cpu
		{
			UINT32 result;

			result = vals * vald;
			SET_G_REG(15, result);

			break;
		}
		// unsigned multiplication, double word product
		case EMULU:
		{
			UINT64 result;

			result = (UINT64)vals * (UINT64)vald;
			vals = result >> 32;
			vald = result & 0xffffffff;
			SET_G_REG(14, vals);
			SET_G_REG(15, vald);

			break;
		}
		// signed multiplication, double word product
		case EMULS:
		{
			INT64 result;

			result = (INT64)(INT32)(vals) * (INT64)(INT32)(vald);
			vals = result >> 32;
			vald = result & 0xffffffff;
			SET_G_REG(14, vals);
			SET_G_REG(15, vald);

			break;
		}
		// signed multiply/add, single word product sum
		case EMAC:
		{
			INT32 result;

			result = (INT32)GET_G_REG(15) + ((INT32)(vals) * (INT32)(vald));
			SET_G_REG(15, result);

			break;
		}
		// signed multiply/add, double word product sum
		case EMACD:
		{
			INT64 result;

			result = (INT64)COMBINE_64_32_32(GET_G_REG(14), GET_G_REG(15)) + (INT64)((INT64)(INT32)(vals) * (INT64)(INT32)(vald));

			vals = result >> 32;
			vald = result & 0xffffffff;
			SET_G_REG(14, vals);
			SET_G_REG(15, vald);

			break;
		}
		// signed multiply/substract, single word product difference
		case EMSUB:
		{
			INT32 result;

			result = (INT32)GET_G_REG(15) - ((INT32)(vals) * (INT32)(vald));
			SET_G_REG(15, result);

			break;
		}
		// signed multiply/substract, double word product difference
		case EMSUBD:
		{
			INT64 result;

			result = (INT64)COMBINE_64_32_32(GET_G_REG(14), GET_G_REG(15)) - (INT64)((INT64)(INT32)(vals) * (INT64)(INT32)(vald));

			vals = result >> 32;
			vald = result & 0xffffffff;
			SET_G_REG(14, vals);
			SET_G_REG(15, vald);

			break;
		}
		// signed half-word multiply/add, single word product sum
		case EHMAC:
		{
			INT32 result;

			result = (INT32)GET_G_REG(15) + ((INT32)((vald & 0xffff0000) >> 16) * (INT32)((vals & 0xffff0000) >> 16)) + ((INT32)(vald & 0xffff) * (INT32)(vals & 0xffff));
			SET_G_REG(15, result);

			break;
		}
		// signed half-word multiply/add, double word product sum
		case EHMACD:
		{
			INT64 result;

			result = (INT64)COMBINE_64_32_32(GET_G_REG(14), GET_G_REG(15)) + (INT64)((INT64)(INT32)((vald & 0xffff0000) >> 16) * (INT64)(INT32)((vals & 0xffff0000) >> 16)) + ((INT64)(INT32)(vald & 0xffff) * (INT64)(INT32)(vals & 0xffff));

			vals = result >> 32;
			vald = result & 0xffffffff;
			SET_G_REG(14, vals);
			SET_G_REG(15, vald);

			break;
		}
		// half-word complex multiply
		case EHCMULD:
		{
			UINT32 result;

			result = (((vald & 0xffff0000) >> 16) * ((vals & 0xffff0000) >> 16)) - ((vald & 0xffff) * (vals & 0xffff));
			SET_G_REG(14, result);

			result = (((vald & 0xffff0000) >> 16) * (vals & 0xffff)) + ((vald & 0xffff) * ((vals & 0xffff0000) >> 16));
			SET_G_REG(15, result);

			break;
		}
		// half-word complex multiply/add
		case EHCMACD:
		{
			UINT32 result;

			result = GET_G_REG(14) + (((vald & 0xffff0000) >> 16) * ((vals & 0xffff0000) >> 16)) - ((vald & 0xffff) * (vals & 0xffff));
			SET_G_REG(14, result);

			result = GET_G_REG(15) + (((vald & 0xffff0000) >> 16) * (vals & 0xffff)) + ((vald & 0xffff) * ((vals & 0xffff0000) >> 16));
			SET_G_REG(15, result);

			break;
		}
		// half-word (complex) add/substract
		// Ls is not used and should denote the same register as Ld
		case EHCSUMD:
		{
			UINT32 result;

			result = ((((vals & 0xffff0000) >> 16) + GET_G_REG(14)) << 16) & 0xffff0000;
			result |= ((vals & 0xffff) + GET_G_REG(15)) & 0xffff;
			SET_G_REG(14, result);

			result = ((((vals & 0xffff0000) >> 16) - GET_G_REG(14)) << 16) & 0xffff0000;
			result |= ((vals & 0xffff) - GET_G_REG(15)) & 0xffff;
			SET_G_REG(15, result);

			break;
		}
		// half-word (complex) add/substract with fixed point adjustment
		// Ls is not used and should denote the same register as Ld
		case EHCFFTD:
		{
			UINT32 result;

			result = ((((vals & 0xffff0000) >> 16) + (GET_G_REG(14) >> 15)) << 16) & 0xffff0000;
			result |= ((vals & 0xffff) + (GET_G_REG(15) >> 15)) & 0xffff;
			SET_G_REG(14, result);

			result = ((((vals & 0xffff0000) >> 16) - (GET_G_REG(14) >> 15)) << 16) & 0xffff0000;
			result |= ((vals & 0xffff) - (GET_G_REG(15) >> 15)) & 0xffff;
			SET_G_REG(15, result);

			break;
		}
		// half-word (complex) add/substract with fixed point adjustment and shift
		// Ls is not used and should denote the same register as Ld
		case EHCFFTSD:
		{
			UINT32 result;

			result = (((((vals & 0xffff0000) >> 16) + (GET_G_REG(14) >> 15)) >> 1) << 16) & 0xffff0000;
			result |= ((((vals & 0xffff) + (GET_G_REG(15) >> 15)) >> 1) & 0xffff);
			SET_G_REG(14, result);

			result = (((((vals & 0xffff0000) >> 16) - (GET_G_REG(14) >> 15)) >> 1) << 16) & 0xffff0000;
			result |= ((((vals & 0xffff) - (GET_G_REG(15) >> 15)) >> 1) & 0xffff);
			SET_G_REG(15, result);

			break;
		}
		default:
			logerror("Executed Illegal extended opcode (%X). PC = %08X\n", EXTRA_U, PC);
			break;
	}

	hyperstone_ICount -= 1; //TODO: with the latency it can change
}

static void hyperstone_do(void)
{
	fatalerror("Executed hyperstone_do instruction. PC = %08X", PPC);
}

static void hyperstone_ldwr(void)
{
	SET_SREG(READ_W(DREG));

	hyperstone_ICount -= 1;
}

static void hyperstone_lddr(void)
{
	SET_SREG(READ_W(DREG));
	SET_SREGF(READ_W(DREG + 4));

	hyperstone_ICount -= 2;
}

static void hyperstone_ldwp(void)
{
	SET_SREG(READ_W(DREG));

	// post increment the destination register if it's different from the source one
	// (needed by Hidden Catch)
	if(!(current_regs.src == current_regs.dst && S_BIT == LOCAL))
		SET_DREG(DREG + 4);

	hyperstone_ICount -= 1;
}

static void hyperstone_lddp(void)
{
	SET_SREG(READ_W(DREG));
	SET_SREGF(READ_W(DREG + 4));

	// post increment the destination register if it's different from the source one
	// and from the "next source" one
	if(!(current_regs.src == current_regs.dst && S_BIT == LOCAL) &&	!SAME_SRCF_DST )
	{
		SET_DREG(DREG + 8);
	}
	else
	{
		ui_popup("LDD.P denoted same regs @ %08X",PPC);
	}

	hyperstone_ICount -= 2;
}

static void hyperstone_stwr(void)
{
	if( SRC_IS_SR )
		SREG = 0;

	WRITE_W(DREG, SREG);

	hyperstone_ICount -= 1;
}

static void hyperstone_stdr(void)
{
	if( SRC_IS_SR )
		SREG = SREGF = 0;

	WRITE_W(DREG, SREG);
	WRITE_W(DREG + 4, SREGF);

	hyperstone_ICount -= 2;
}

static void hyperstone_stwp(void)
{
	if( SRC_IS_SR )
		SREG = 0;

	WRITE_W(DREG, SREG);
	SET_DREG(DREG + 4);

	hyperstone_ICount -= 1;
}

static void hyperstone_stdp(void)
{
	if( SRC_IS_SR )
		SREG = SREGF = 0;

	WRITE_W(DREG, SREG);
	SET_DREG(DREG + 8);

	if( SAME_SRCF_DST )
		WRITE_W(DREG + 4, SREGF + 8); // because DREG == SREGF and DREG has been incremented
	else
		WRITE_W(DREG + 4, SREGF);

	hyperstone_ICount -= 2;
}

static void hyperstone_dbv(void)
{
	if( GET_V )
		execute_dbr();

	hyperstone_ICount -= 1;
}

static void hyperstone_dbnv(void)
{
	if( !GET_V )
		execute_dbr();

	hyperstone_ICount -= 1;
}

static void hyperstone_dbe(void) //or DBZ
{
	if( GET_Z )
		execute_dbr();

	hyperstone_ICount -= 1;
}

static void hyperstone_dbne(void) //or DBNZ
{
	if( !GET_Z )
		execute_dbr();

	hyperstone_ICount -= 1;
}

static void hyperstone_dbc(void) //or DBST
{
	if( GET_C )
		execute_dbr();

	hyperstone_ICount -= 1;
}

static void hyperstone_dbnc(void) //or DBHE
{
	if( !GET_C )
		execute_dbr();

	hyperstone_ICount -= 1;
}

static void hyperstone_dbse(void)
{
	if( GET_C || GET_Z )
		execute_dbr();

	hyperstone_ICount -= 1;
}

static void hyperstone_dbht(void)
{
	if( !GET_C && !GET_Z )
		execute_dbr();

	hyperstone_ICount -= 1;
}

static void hyperstone_dbn(void) //or DBLT
{
	if( GET_N )
		execute_dbr();

	hyperstone_ICount -= 1;
}

static void hyperstone_dbnn(void) //or DBGE
{
	if( !GET_N )
		execute_dbr();

	hyperstone_ICount -= 1;
}

static void hyperstone_dble(void)
{
	if( GET_N || GET_Z )
		execute_dbr();

	hyperstone_ICount -= 1;
}

static void hyperstone_dbgt(void)
{
	if( !GET_N && !GET_Z )
		execute_dbr();

	hyperstone_ICount -= 1;
}

static void hyperstone_dbr(void)
{
	execute_dbr();
}

static void hyperstone_frame(void)
{
	INT8 difference; // really it's 7 bits
	UINT8 realfp = GET_FP - SRC_CODE;

	SET_FP(realfp);
	SET_FL(DST_CODE);
	SET_M(0);

	difference = ((SP & 0x1fc) >> 2) + (64 - 10) - (realfp + GET_FL);

	/* convert to 8 bits */
	if(difference > 63)
		difference = (INT8)(difference|0x80);
	else if( difference < -64 )
		difference = difference & 0x7f;

	if( difference < 0 ) // else it's finished
	{
		UINT8 tmp_flag;

		tmp_flag = ( SP >= UB ? 1 : 0 );

		do
		{
			WRITE_W(SP, GET_ABS_L_REG((SP & 0xfc) >> 2));
			SP += 4;
			difference++;

		} while(difference != 0);

		if( tmp_flag )
		{
			UINT32 addr = get_trap_addr(FRAME_ERROR);
			execute_exception(addr);
		}
	}

	//TODO: no 1!
	hyperstone_ICount -= 1;
}

static void hyperstone_call(void)
{
	if( SRC_IS_SR )
		SREG = 0;

	if( !DST_CODE )
		current_regs.dst = 16;

	EXTRA_S = (EXTRA_S & ~1) + SREG;

	SET_ILC(hyperstone.instruction_length & 3);

	SET_DREG((PC & 0xfffffffe) | GET_S);
	SET_DREGF(SR);

	SET_FP(GET_FP + current_regs.dst);

	SET_FL(6); //default value for call
	SET_M(0);

	PPC = PC;
	PC = EXTRA_S; // const value
	change_pc(PC);

	hyperstone.intblock = 2;

	//TODO: add interrupt locks, errors, ....

	//TODO: no 1!
	hyperstone_ICount -= 1;
}

static void hyperstone_bv(void)
{
	if( GET_V )
		execute_br();
	else
		hyperstone_ICount -= 1;
}

static void hyperstone_bnv(void)
{
	if( !GET_V )
		execute_br();
	else
		hyperstone_ICount -= 1;
}

static void hyperstone_be(void) //or BZ
{
	if( GET_Z )
		execute_br();
	else
		hyperstone_ICount -= 1;
}

static void hyperstone_bne(void) //or BNZ
{
	if( !GET_Z )
		execute_br();
	else
		hyperstone_ICount -= 1;
}

static void hyperstone_bc(void) //or BST
{
	if( GET_C )
		execute_br();
	else
		hyperstone_ICount -= 1;
}

static void hyperstone_bnc(void) //or BHE
{
	if( !GET_C )
		execute_br();
	else
		hyperstone_ICount -= 1;
}

static void hyperstone_bse(void)
{
	if( GET_C || GET_Z )
		execute_br();
	else
		hyperstone_ICount -= 1;
}

static void hyperstone_bht(void)
{
	if( !GET_C && !GET_Z )
		execute_br();
	else
		hyperstone_ICount -= 1;
}

static void hyperstone_bn(void) //or BLT
{
	if( GET_N )
		execute_br();
	else
		hyperstone_ICount -= 1;
}

static void hyperstone_bnn(void) //or BGE
{
	if( !GET_N )
		execute_br();
	else
		hyperstone_ICount -= 1;
}

static void hyperstone_ble(void)
{
	if( GET_N || GET_Z )
		execute_br();
	else
		hyperstone_ICount -= 1;
}

static void hyperstone_bgt(void)
{
	if( !GET_N && !GET_Z )
		execute_br();
	else
		hyperstone_ICount -= 1;
}

static void hyperstone_br(void)
{
	execute_br();
}

static void hyperstone_trap(void)
{
	UINT8 code, trapno;
	UINT32 addr;

	trapno = (OP & 0xfc) >> 2;

	addr = get_trap_addr(trapno);
	code = ((OP & 0x300) >> 6) | (OP & 0x03);

	switch( code )
	{
		case TRAPLE:
			if( GET_N || GET_Z )
				execute_trap(addr);

			break;

		case TRAPGT:
			if( !GET_N && !GET_Z )
				execute_trap(addr);

			break;

		case TRAPLT:
			if( GET_N )
				execute_trap(addr);

			break;

		case TRAPGE:
			if( !GET_N )
				execute_trap(addr);

			break;

		case TRAPSE:
			if( GET_C || GET_Z )
				execute_trap(addr);

			break;

		case TRAPHT:
			if( !GET_C && !GET_Z )
				execute_trap(addr);

			break;

		case TRAPST:
			if( GET_C )
				execute_trap(addr);

			break;

		case TRAPHE:
			if( !GET_C )
				execute_trap(addr);

			break;

		case TRAPE:
			if( GET_Z )
				execute_trap(addr);

			break;

		case TRAPNE:
			if( !GET_Z )
				execute_trap(addr);

			break;

		case TRAPV:
			if( GET_V )
				execute_trap(addr);

			break;

		case TRAP:
			execute_trap(addr);

			break;
	}

	hyperstone_ICount -= 1;
}


/**************************************************************************
 * Generic set_info
 **************************************************************************/

static void hyperstone_set_info(UINT32 state, union cpuinfo *info)
{
	switch (state)
	{
		/* --- the following bits of info are set as 64-bit signed integers --- */

		case CPUINFO_INT_PC:
		case CPUINFO_INT_REGISTER + E132XS_PC:			PC = info->i; change_pc(PC);			break;
		case CPUINFO_INT_REGISTER + E132XS_SR:			SR = info->i;							break;
		case CPUINFO_INT_REGISTER + E132XS_FER:			FER = info->i;							break;
		case CPUINFO_INT_REGISTER + E132XS_G3:			hyperstone.global_regs[3] = info->i;		break;
		case CPUINFO_INT_REGISTER + E132XS_G4:			hyperstone.global_regs[4] = info->i;		break;
		case CPUINFO_INT_REGISTER + E132XS_G5:			hyperstone.global_regs[5] = info->i;		break;
		case CPUINFO_INT_REGISTER + E132XS_G6:			hyperstone.global_regs[6] = info->i;		break;
		case CPUINFO_INT_REGISTER + E132XS_G7:			hyperstone.global_regs[7] = info->i;		break;
		case CPUINFO_INT_REGISTER + E132XS_G8:			hyperstone.global_regs[8] = info->i;		break;
		case CPUINFO_INT_REGISTER + E132XS_G9:			hyperstone.global_regs[9] = info->i;		break;
		case CPUINFO_INT_REGISTER + E132XS_G10:			hyperstone.global_regs[10] = info->i;		break;
		case CPUINFO_INT_REGISTER + E132XS_G11:			hyperstone.global_regs[11] = info->i;		break;
		case CPUINFO_INT_REGISTER + E132XS_G12:			hyperstone.global_regs[12] = info->i;		break;
		case CPUINFO_INT_REGISTER + E132XS_G13:			hyperstone.global_regs[13] = info->i;		break;
		case CPUINFO_INT_REGISTER + E132XS_G14:			hyperstone.global_regs[14] = info->i;		break;
		case CPUINFO_INT_REGISTER + E132XS_G15:			hyperstone.global_regs[15] = info->i;		break;
		case CPUINFO_INT_REGISTER + E132XS_G16:			hyperstone.global_regs[16] = info->i;		break;
		case CPUINFO_INT_REGISTER + E132XS_G17:			hyperstone.global_regs[17] = info->i;		break;
		case CPUINFO_INT_SP:
		case CPUINFO_INT_REGISTER + E132XS_SP:			SP  = info->i;							break;
		case CPUINFO_INT_REGISTER + E132XS_UB:			UB  = info->i;							break;
		case CPUINFO_INT_REGISTER + E132XS_BCR:			BCR = info->i;							break;
		case CPUINFO_INT_REGISTER + E132XS_TPR:			TPR = info->i;							break;
		case CPUINFO_INT_REGISTER + E132XS_TCR:			TCR = info->i;							break;
		case CPUINFO_INT_REGISTER + E132XS_TR:			TR  = info->i;							break;
		case CPUINFO_INT_REGISTER + E132XS_WCR:			WCR = info->i;							break;
		case CPUINFO_INT_REGISTER + E132XS_ISR:			ISR = info->i;							break;
		case CPUINFO_INT_REGISTER + E132XS_FCR:			FCR = info->i;							break;
		case CPUINFO_INT_REGISTER + E132XS_MCR:			MCR = info->i;							break;
		case CPUINFO_INT_REGISTER + E132XS_G28:			hyperstone.global_regs[28] = info->i;		break;
		case CPUINFO_INT_REGISTER + E132XS_G29:			hyperstone.global_regs[29] = info->i;		break;
		case CPUINFO_INT_REGISTER + E132XS_G30:			hyperstone.global_regs[30] = info->i;		break;
		case CPUINFO_INT_REGISTER + E132XS_G31:			hyperstone.global_regs[31] = info->i;		break;
		case CPUINFO_INT_REGISTER + E132XS_CL0:			hyperstone.local_regs[(0 + GET_FP) % 64] = info->i; break;
		case CPUINFO_INT_REGISTER + E132XS_CL1:			hyperstone.local_regs[(1 + GET_FP) % 64] = info->i; break;
		case CPUINFO_INT_REGISTER + E132XS_CL2:			hyperstone.local_regs[(2 + GET_FP) % 64] = info->i; break;
		case CPUINFO_INT_REGISTER + E132XS_CL3:			hyperstone.local_regs[(3 + GET_FP) % 64] = info->i; break;
		case CPUINFO_INT_REGISTER + E132XS_CL4:			hyperstone.local_regs[(4 + GET_FP) % 64] = info->i; break;
		case CPUINFO_INT_REGISTER + E132XS_CL5:			hyperstone.local_regs[(5 + GET_FP) % 64] = info->i; break;
		case CPUINFO_INT_REGISTER + E132XS_CL6:			hyperstone.local_regs[(6 + GET_FP) % 64] = info->i; break;
		case CPUINFO_INT_REGISTER + E132XS_CL7:			hyperstone.local_regs[(7 + GET_FP) % 64] = info->i; break;
		case CPUINFO_INT_REGISTER + E132XS_CL8:			hyperstone.local_regs[(8 + GET_FP) % 64] = info->i; break;
		case CPUINFO_INT_REGISTER + E132XS_CL9:			hyperstone.local_regs[(9 + GET_FP) % 64] = info->i; break;
		case CPUINFO_INT_REGISTER + E132XS_CL10:		hyperstone.local_regs[(10 + GET_FP) % 64] = info->i; break;
		case CPUINFO_INT_REGISTER + E132XS_CL11:		hyperstone.local_regs[(11 + GET_FP) % 64] = info->i; break;
		case CPUINFO_INT_REGISTER + E132XS_CL12:		hyperstone.local_regs[(12 + GET_FP) % 64] = info->i; break;
		case CPUINFO_INT_REGISTER + E132XS_CL13:		hyperstone.local_regs[(13 + GET_FP) % 64] = info->i; break;
		case CPUINFO_INT_REGISTER + E132XS_CL14:		hyperstone.local_regs[(14 + GET_FP) % 64] = info->i; break;
		case CPUINFO_INT_REGISTER + E132XS_CL15:		hyperstone.local_regs[(15 + GET_FP) % 64] = info->i; break;
		case CPUINFO_INT_REGISTER + E132XS_L0:			hyperstone.local_regs[0] = info->i;		break;
		case CPUINFO_INT_REGISTER + E132XS_L1:			hyperstone.local_regs[1] = info->i;		break;
		case CPUINFO_INT_REGISTER + E132XS_L2:			hyperstone.local_regs[2] = info->i;		break;
		case CPUINFO_INT_REGISTER + E132XS_L3:			hyperstone.local_regs[3] = info->i;		break;
		case CPUINFO_INT_REGISTER + E132XS_L4:			hyperstone.local_regs[4] = info->i;		break;
		case CPUINFO_INT_REGISTER + E132XS_L5:			hyperstone.local_regs[5] = info->i;		break;
		case CPUINFO_INT_REGISTER + E132XS_L6:			hyperstone.local_regs[6] = info->i;		break;
		case CPUINFO_INT_REGISTER + E132XS_L7:			hyperstone.local_regs[7] = info->i;		break;
		case CPUINFO_INT_REGISTER + E132XS_L8:			hyperstone.local_regs[8] = info->i;		break;
		case CPUINFO_INT_REGISTER + E132XS_L9:			hyperstone.local_regs[9] = info->i;		break;
		case CPUINFO_INT_REGISTER + E132XS_L10:			hyperstone.local_regs[10] = info->i;		break;
		case CPUINFO_INT_REGISTER + E132XS_L11:			hyperstone.local_regs[11] = info->i;		break;
		case CPUINFO_INT_REGISTER + E132XS_L12:			hyperstone.local_regs[12] = info->i;		break;
		case CPUINFO_INT_REGISTER + E132XS_L13:			hyperstone.local_regs[13] = info->i;		break;
		case CPUINFO_INT_REGISTER + E132XS_L14:			hyperstone.local_regs[14] = info->i;		break;
		case CPUINFO_INT_REGISTER + E132XS_L15:			hyperstone.local_regs[15] = info->i;		break;
		case CPUINFO_INT_REGISTER + E132XS_L16:			hyperstone.local_regs[16] = info->i;		break;
		case CPUINFO_INT_REGISTER + E132XS_L17:			hyperstone.local_regs[17] = info->i;		break;
		case CPUINFO_INT_REGISTER + E132XS_L18:			hyperstone.local_regs[18] = info->i;		break;
		case CPUINFO_INT_REGISTER + E132XS_L19:			hyperstone.local_regs[19] = info->i;		break;
		case CPUINFO_INT_REGISTER + E132XS_L20:			hyperstone.local_regs[20] = info->i;		break;
		case CPUINFO_INT_REGISTER + E132XS_L21:			hyperstone.local_regs[21] = info->i;		break;
		case CPUINFO_INT_REGISTER + E132XS_L22:			hyperstone.local_regs[22] = info->i;		break;
		case CPUINFO_INT_REGISTER + E132XS_L23:			hyperstone.local_regs[23] = info->i;		break;
		case CPUINFO_INT_REGISTER + E132XS_L24:			hyperstone.local_regs[24] = info->i;		break;
		case CPUINFO_INT_REGISTER + E132XS_L25:			hyperstone.local_regs[25] = info->i;		break;
		case CPUINFO_INT_REGISTER + E132XS_L26:			hyperstone.local_regs[26] = info->i;		break;
		case CPUINFO_INT_REGISTER + E132XS_L27:			hyperstone.local_regs[27] = info->i;		break;
		case CPUINFO_INT_REGISTER + E132XS_L28:			hyperstone.local_regs[28] = info->i;		break;
		case CPUINFO_INT_REGISTER + E132XS_L29:			hyperstone.local_regs[29] = info->i;		break;
		case CPUINFO_INT_REGISTER + E132XS_L30:			hyperstone.local_regs[30] = info->i;		break;
		case CPUINFO_INT_REGISTER + E132XS_L31:			hyperstone.local_regs[31] = info->i;		break;
		case CPUINFO_INT_REGISTER + E132XS_L32:			hyperstone.local_regs[32] = info->i;		break;
		case CPUINFO_INT_REGISTER + E132XS_L33:			hyperstone.local_regs[33] = info->i;		break;
		case CPUINFO_INT_REGISTER + E132XS_L34:			hyperstone.local_regs[34] = info->i;		break;
		case CPUINFO_INT_REGISTER + E132XS_L35:			hyperstone.local_regs[35] = info->i;		break;
		case CPUINFO_INT_REGISTER + E132XS_L36:			hyperstone.local_regs[36] = info->i;		break;
		case CPUINFO_INT_REGISTER + E132XS_L37:			hyperstone.local_regs[37] = info->i;		break;
		case CPUINFO_INT_REGISTER + E132XS_L38:			hyperstone.local_regs[38] = info->i;		break;
		case CPUINFO_INT_REGISTER + E132XS_L39:			hyperstone.local_regs[39] = info->i;		break;
		case CPUINFO_INT_REGISTER + E132XS_L40:			hyperstone.local_regs[40] = info->i;		break;
		case CPUINFO_INT_REGISTER + E132XS_L41:			hyperstone.local_regs[41] = info->i;		break;
		case CPUINFO_INT_REGISTER + E132XS_L42:			hyperstone.local_regs[42] = info->i;		break;
		case CPUINFO_INT_REGISTER + E132XS_L43:			hyperstone.local_regs[43] = info->i;		break;
		case CPUINFO_INT_REGISTER + E132XS_L44:			hyperstone.local_regs[44] = info->i;		break;
		case CPUINFO_INT_REGISTER + E132XS_L45:			hyperstone.local_regs[45] = info->i;		break;
		case CPUINFO_INT_REGISTER + E132XS_L46:			hyperstone.local_regs[46] = info->i;		break;
		case CPUINFO_INT_REGISTER + E132XS_L47:			hyperstone.local_regs[47] = info->i;		break;
		case CPUINFO_INT_REGISTER + E132XS_L48:			hyperstone.local_regs[48] = info->i;		break;
		case CPUINFO_INT_REGISTER + E132XS_L49:			hyperstone.local_regs[49] = info->i;		break;
		case CPUINFO_INT_REGISTER + E132XS_L50:			hyperstone.local_regs[50] = info->i;		break;
		case CPUINFO_INT_REGISTER + E132XS_L51:			hyperstone.local_regs[51] = info->i;		break;
		case CPUINFO_INT_REGISTER + E132XS_L52:			hyperstone.local_regs[52] = info->i;		break;
		case CPUINFO_INT_REGISTER + E132XS_L53:			hyperstone.local_regs[53] = info->i;		break;
		case CPUINFO_INT_REGISTER + E132XS_L54:			hyperstone.local_regs[54] = info->i;		break;
		case CPUINFO_INT_REGISTER + E132XS_L55:			hyperstone.local_regs[55] = info->i;		break;
		case CPUINFO_INT_REGISTER + E132XS_L56:			hyperstone.local_regs[56] = info->i;		break;
		case CPUINFO_INT_REGISTER + E132XS_L57:			hyperstone.local_regs[57] = info->i;		break;
		case CPUINFO_INT_REGISTER + E132XS_L58:			hyperstone.local_regs[58] = info->i;		break;
		case CPUINFO_INT_REGISTER + E132XS_L59:			hyperstone.local_regs[59] = info->i;		break;
		case CPUINFO_INT_REGISTER + E132XS_L60:			hyperstone.local_regs[60] = info->i;		break;
		case CPUINFO_INT_REGISTER + E132XS_L61:			hyperstone.local_regs[61] = info->i;		break;
		case CPUINFO_INT_REGISTER + E132XS_L62:			hyperstone.local_regs[62] = info->i;		break;
		case CPUINFO_INT_REGISTER + E132XS_L63:			hyperstone.local_regs[63] = info->i;		break;

		case CPUINFO_INT_INPUT_STATE + 0:				set_irq_line(0, info->i);					break;
		case CPUINFO_INT_INPUT_STATE + 1:				set_irq_line(1, info->i);					break;
		case CPUINFO_INT_INPUT_STATE + 2:				set_irq_line(2, info->i);					break;
		case CPUINFO_INT_INPUT_STATE + 3:				set_irq_line(3, info->i);					break;
		case CPUINFO_INT_INPUT_STATE + 4:				set_irq_line(4, info->i);					break;
		case CPUINFO_INT_INPUT_STATE + 5:				set_irq_line(5, info->i);					break;
		case CPUINFO_INT_INPUT_STATE + 6:				set_irq_line(6, info->i);					break;
		case CPUINFO_INT_INPUT_STATE + 7:				set_irq_line(7, info->i);					break;
	}
}

/**************************************************************************
 * Generic get_info
 **************************************************************************/

void hyperstone_get_info(UINT32 state, union cpuinfo *info)
{
	switch (state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case CPUINFO_INT_CONTEXT_SIZE:					info->i = sizeof(hyperstone_regs);		break;
		case CPUINFO_INT_INPUT_LINES:					info->i = 8;							break;
		case CPUINFO_INT_DEFAULT_IRQ_VECTOR:			info->i = 0;							break;
		case CPUINFO_INT_ENDIANNESS:					info->i = CPU_IS_BE;					break;
		case CPUINFO_INT_CLOCK_DIVIDER:					info->i = 1;							break;
		case CPUINFO_INT_MIN_INSTRUCTION_BYTES:			info->i = 2;							break;
		case CPUINFO_INT_MAX_INSTRUCTION_BYTES:			info->i = 6;							break;
		case CPUINFO_INT_MIN_CYCLES:					info->i = 1;							break;
		case CPUINFO_INT_MAX_CYCLES:					info->i = 36;							break;

		case CPUINFO_INT_ADDRBUS_WIDTH + ADDRESS_SPACE_PROGRAM: info->i = 32;					break;
		case CPUINFO_INT_ADDRBUS_SHIFT + ADDRESS_SPACE_PROGRAM: info->i = 0;					break;
		case CPUINFO_INT_DATABUS_WIDTH + ADDRESS_SPACE_DATA:	info->i = 0;					break;
		case CPUINFO_INT_ADDRBUS_WIDTH + ADDRESS_SPACE_DATA: 	info->i = 0;					break;
		case CPUINFO_INT_ADDRBUS_SHIFT + ADDRESS_SPACE_DATA: 	info->i = 0;					break;
		case CPUINFO_INT_ADDRBUS_WIDTH + ADDRESS_SPACE_IO: 		info->i = 15;					break;
		case CPUINFO_INT_ADDRBUS_SHIFT + ADDRESS_SPACE_IO: 		info->i = 0;					break;

		case CPUINFO_INT_INPUT_STATE + 0:					/* not implemented */				break;

		case CPUINFO_INT_PREVIOUSPC:					info->i = PPC;							break;

		case CPUINFO_INT_PC:
		case CPUINFO_INT_REGISTER + E132XS_PC:			info->i =  PC;							break;
		case CPUINFO_INT_REGISTER + E132XS_SR:			info->i =  SR;							break;
		case CPUINFO_INT_REGISTER + E132XS_FER:			info->i =  FER;							break;
		case CPUINFO_INT_REGISTER + E132XS_G3:			info->i =  hyperstone.global_regs[3];	break;
		case CPUINFO_INT_REGISTER + E132XS_G4:			info->i =  hyperstone.global_regs[4];	break;
		case CPUINFO_INT_REGISTER + E132XS_G5:			info->i =  hyperstone.global_regs[5];	break;
		case CPUINFO_INT_REGISTER + E132XS_G6:			info->i =  hyperstone.global_regs[6];	break;
		case CPUINFO_INT_REGISTER + E132XS_G7:			info->i =  hyperstone.global_regs[7];	break;
		case CPUINFO_INT_REGISTER + E132XS_G8:			info->i =  hyperstone.global_regs[8];	break;
		case CPUINFO_INT_REGISTER + E132XS_G9:			info->i =  hyperstone.global_regs[9];	break;
		case CPUINFO_INT_REGISTER + E132XS_G10:			info->i =  hyperstone.global_regs[10];	break;
		case CPUINFO_INT_REGISTER + E132XS_G11:			info->i =  hyperstone.global_regs[11];	break;
		case CPUINFO_INT_REGISTER + E132XS_G12:			info->i =  hyperstone.global_regs[12];	break;
		case CPUINFO_INT_REGISTER + E132XS_G13:			info->i =  hyperstone.global_regs[13];	break;
		case CPUINFO_INT_REGISTER + E132XS_G14:			info->i =  hyperstone.global_regs[14];	break;
		case CPUINFO_INT_REGISTER + E132XS_G15:			info->i =  hyperstone.global_regs[15];	break;
		case CPUINFO_INT_REGISTER + E132XS_G16:			info->i =  hyperstone.global_regs[16];	break;
		case CPUINFO_INT_REGISTER + E132XS_G17:			info->i =  hyperstone.global_regs[17];	break;
		case CPUINFO_INT_SP:
		case CPUINFO_INT_REGISTER + E132XS_SP:			info->i =  SP;							break;
		case CPUINFO_INT_REGISTER + E132XS_UB:			info->i =  UB;							break;
		case CPUINFO_INT_REGISTER + E132XS_BCR:			info->i =  BCR;							break;
		case CPUINFO_INT_REGISTER + E132XS_TPR:			info->i =  TPR;							break;
		case CPUINFO_INT_REGISTER + E132XS_TCR:			info->i =  TCR;							break;
		case CPUINFO_INT_REGISTER + E132XS_TR:			info->i =  TR;							break;
		case CPUINFO_INT_REGISTER + E132XS_WCR:			info->i =  WCR;							break;
		case CPUINFO_INT_REGISTER + E132XS_ISR:			info->i =  ISR;							break;
		case CPUINFO_INT_REGISTER + E132XS_FCR:			info->i =  FCR;							break;
		case CPUINFO_INT_REGISTER + E132XS_MCR:			info->i =  MCR;							break;
		case CPUINFO_INT_REGISTER + E132XS_G28:			info->i =  hyperstone.global_regs[28];	break;
		case CPUINFO_INT_REGISTER + E132XS_G29:			info->i =  hyperstone.global_regs[29];	break;
		case CPUINFO_INT_REGISTER + E132XS_G30:			info->i =  hyperstone.global_regs[30];	break;
		case CPUINFO_INT_REGISTER + E132XS_G31:			info->i =  hyperstone.global_regs[31];	break;
		case CPUINFO_INT_REGISTER + E132XS_CL0:			info->i =  hyperstone.local_regs[(0 + GET_FP) % 64]; break;
		case CPUINFO_INT_REGISTER + E132XS_CL1:			info->i =  hyperstone.local_regs[(1 + GET_FP) % 64]; break;
		case CPUINFO_INT_REGISTER + E132XS_CL2:			info->i =  hyperstone.local_regs[(2 + GET_FP) % 64]; break;
		case CPUINFO_INT_REGISTER + E132XS_CL3:			info->i =  hyperstone.local_regs[(3 + GET_FP) % 64]; break;
		case CPUINFO_INT_REGISTER + E132XS_CL4:			info->i =  hyperstone.local_regs[(4 + GET_FP) % 64]; break;
		case CPUINFO_INT_REGISTER + E132XS_CL5:			info->i =  hyperstone.local_regs[(5 + GET_FP) % 64]; break;
		case CPUINFO_INT_REGISTER + E132XS_CL6:			info->i =  hyperstone.local_regs[(6 + GET_FP) % 64]; break;
		case CPUINFO_INT_REGISTER + E132XS_CL7:			info->i =  hyperstone.local_regs[(7 + GET_FP) % 64]; break;
		case CPUINFO_INT_REGISTER + E132XS_CL8:			info->i =  hyperstone.local_regs[(8 + GET_FP) % 64]; break;
		case CPUINFO_INT_REGISTER + E132XS_CL9:			info->i =  hyperstone.local_regs[(9 + GET_FP) % 64]; break;
		case CPUINFO_INT_REGISTER + E132XS_CL10:		info->i =  hyperstone.local_regs[(10 + GET_FP) % 64]; break;
		case CPUINFO_INT_REGISTER + E132XS_CL11:		info->i =  hyperstone.local_regs[(11 + GET_FP) % 64]; break;
		case CPUINFO_INT_REGISTER + E132XS_CL12:		info->i =  hyperstone.local_regs[(12 + GET_FP) % 64]; break;
		case CPUINFO_INT_REGISTER + E132XS_CL13:		info->i =  hyperstone.local_regs[(13 + GET_FP) % 64]; break;
		case CPUINFO_INT_REGISTER + E132XS_CL14:		info->i =  hyperstone.local_regs[(14 + GET_FP) % 64]; break;
		case CPUINFO_INT_REGISTER + E132XS_CL15:		info->i =  hyperstone.local_regs[(15 + GET_FP) % 64]; break;
		case CPUINFO_INT_REGISTER + E132XS_L0:			info->i =  hyperstone.local_regs[0];		break;
		case CPUINFO_INT_REGISTER + E132XS_L1:			info->i =  hyperstone.local_regs[1];		break;
		case CPUINFO_INT_REGISTER + E132XS_L2:			info->i =  hyperstone.local_regs[2];		break;
		case CPUINFO_INT_REGISTER + E132XS_L3:			info->i =  hyperstone.local_regs[3];		break;
		case CPUINFO_INT_REGISTER + E132XS_L4:			info->i =  hyperstone.local_regs[4];		break;
		case CPUINFO_INT_REGISTER + E132XS_L5:			info->i =  hyperstone.local_regs[5];		break;
		case CPUINFO_INT_REGISTER + E132XS_L6:			info->i =  hyperstone.local_regs[6];		break;
		case CPUINFO_INT_REGISTER + E132XS_L7:			info->i =  hyperstone.local_regs[7];		break;
		case CPUINFO_INT_REGISTER + E132XS_L8:			info->i =  hyperstone.local_regs[8];		break;
		case CPUINFO_INT_REGISTER + E132XS_L9:			info->i =  hyperstone.local_regs[9];		break;
		case CPUINFO_INT_REGISTER + E132XS_L10:			info->i =  hyperstone.local_regs[10];		break;
		case CPUINFO_INT_REGISTER + E132XS_L11:			info->i =  hyperstone.local_regs[11];		break;
		case CPUINFO_INT_REGISTER + E132XS_L12:			info->i =  hyperstone.local_regs[12];		break;
		case CPUINFO_INT_REGISTER + E132XS_L13:			info->i =  hyperstone.local_regs[13];		break;
		case CPUINFO_INT_REGISTER + E132XS_L14:			info->i =  hyperstone.local_regs[14];		break;
		case CPUINFO_INT_REGISTER + E132XS_L15:			info->i =  hyperstone.local_regs[15];		break;
		case CPUINFO_INT_REGISTER + E132XS_L16:			info->i =  hyperstone.local_regs[16];		break;
		case CPUINFO_INT_REGISTER + E132XS_L17:			info->i =  hyperstone.local_regs[17];		break;
		case CPUINFO_INT_REGISTER + E132XS_L18:			info->i =  hyperstone.local_regs[18];		break;
		case CPUINFO_INT_REGISTER + E132XS_L19:			info->i =  hyperstone.local_regs[19];		break;
		case CPUINFO_INT_REGISTER + E132XS_L20:			info->i =  hyperstone.local_regs[20];		break;
		case CPUINFO_INT_REGISTER + E132XS_L21:			info->i =  hyperstone.local_regs[21];		break;
		case CPUINFO_INT_REGISTER + E132XS_L22:			info->i =  hyperstone.local_regs[22];		break;
		case CPUINFO_INT_REGISTER + E132XS_L23:			info->i =  hyperstone.local_regs[23];		break;
		case CPUINFO_INT_REGISTER + E132XS_L24:			info->i =  hyperstone.local_regs[24];		break;
		case CPUINFO_INT_REGISTER + E132XS_L25:			info->i =  hyperstone.local_regs[25];		break;
		case CPUINFO_INT_REGISTER + E132XS_L26:			info->i =  hyperstone.local_regs[26];		break;
		case CPUINFO_INT_REGISTER + E132XS_L27:			info->i =  hyperstone.local_regs[27];		break;
		case CPUINFO_INT_REGISTER + E132XS_L28:			info->i =  hyperstone.local_regs[28];		break;
		case CPUINFO_INT_REGISTER + E132XS_L29:			info->i =  hyperstone.local_regs[29];		break;
		case CPUINFO_INT_REGISTER + E132XS_L30:			info->i =  hyperstone.local_regs[30];		break;
		case CPUINFO_INT_REGISTER + E132XS_L31:			info->i =  hyperstone.local_regs[31];		break;
		case CPUINFO_INT_REGISTER + E132XS_L32:			info->i =  hyperstone.local_regs[32];		break;
		case CPUINFO_INT_REGISTER + E132XS_L33:			info->i =  hyperstone.local_regs[33];		break;
		case CPUINFO_INT_REGISTER + E132XS_L34:			info->i =  hyperstone.local_regs[34];		break;
		case CPUINFO_INT_REGISTER + E132XS_L35:			info->i =  hyperstone.local_regs[35];		break;
		case CPUINFO_INT_REGISTER + E132XS_L36:			info->i =  hyperstone.local_regs[36];		break;
		case CPUINFO_INT_REGISTER + E132XS_L37:			info->i =  hyperstone.local_regs[37];		break;
		case CPUINFO_INT_REGISTER + E132XS_L38:			info->i =  hyperstone.local_regs[38];		break;
		case CPUINFO_INT_REGISTER + E132XS_L39:			info->i =  hyperstone.local_regs[39];		break;
		case CPUINFO_INT_REGISTER + E132XS_L40:			info->i =  hyperstone.local_regs[40];		break;
		case CPUINFO_INT_REGISTER + E132XS_L41:			info->i =  hyperstone.local_regs[41];		break;
		case CPUINFO_INT_REGISTER + E132XS_L42:			info->i =  hyperstone.local_regs[42];		break;
		case CPUINFO_INT_REGISTER + E132XS_L43:			info->i =  hyperstone.local_regs[43];		break;
		case CPUINFO_INT_REGISTER + E132XS_L44:			info->i =  hyperstone.local_regs[44];		break;
		case CPUINFO_INT_REGISTER + E132XS_L45:			info->i =  hyperstone.local_regs[45];		break;
		case CPUINFO_INT_REGISTER + E132XS_L46:			info->i =  hyperstone.local_regs[46];		break;
		case CPUINFO_INT_REGISTER + E132XS_L47:			info->i =  hyperstone.local_regs[47];		break;
		case CPUINFO_INT_REGISTER + E132XS_L48:			info->i =  hyperstone.local_regs[48];		break;
		case CPUINFO_INT_REGISTER + E132XS_L49:			info->i =  hyperstone.local_regs[49];		break;
		case CPUINFO_INT_REGISTER + E132XS_L50:			info->i =  hyperstone.local_regs[50];		break;
		case CPUINFO_INT_REGISTER + E132XS_L51:			info->i =  hyperstone.local_regs[51];		break;
		case CPUINFO_INT_REGISTER + E132XS_L52:			info->i =  hyperstone.local_regs[52];		break;
		case CPUINFO_INT_REGISTER + E132XS_L53:			info->i =  hyperstone.local_regs[53];		break;
		case CPUINFO_INT_REGISTER + E132XS_L54:			info->i =  hyperstone.local_regs[54];		break;
		case CPUINFO_INT_REGISTER + E132XS_L55:			info->i =  hyperstone.local_regs[55];		break;
		case CPUINFO_INT_REGISTER + E132XS_L56:			info->i =  hyperstone.local_regs[56];		break;
		case CPUINFO_INT_REGISTER + E132XS_L57:			info->i =  hyperstone.local_regs[57];		break;
		case CPUINFO_INT_REGISTER + E132XS_L58:			info->i =  hyperstone.local_regs[58];		break;
		case CPUINFO_INT_REGISTER + E132XS_L59:			info->i =  hyperstone.local_regs[59];		break;
		case CPUINFO_INT_REGISTER + E132XS_L60:			info->i =  hyperstone.local_regs[60];		break;
		case CPUINFO_INT_REGISTER + E132XS_L61:			info->i =  hyperstone.local_regs[61];		break;
		case CPUINFO_INT_REGISTER + E132XS_L62:			info->i =  hyperstone.local_regs[62];		break;
		case CPUINFO_INT_REGISTER + E132XS_L63:			info->i =  hyperstone.local_regs[63];		break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case CPUINFO_PTR_SET_INFO:						info->setinfo = hyperstone_set_info;		break;
		case CPUINFO_PTR_GET_CONTEXT:					info->getcontext = hyperstone_get_context;	break;
		case CPUINFO_PTR_SET_CONTEXT:					info->setcontext = hyperstone_set_context;	break;
		case CPUINFO_PTR_INIT:							info->init = hyperstone_init;				break;
		case CPUINFO_PTR_RESET:							info->reset = hyperstone_reset;				break;
		case CPUINFO_PTR_EXIT:							info->exit = hyperstone_exit;				break;
		case CPUINFO_PTR_EXECUTE:						info->execute = hyperstone_execute;			break;
		case CPUINFO_PTR_BURN:							info->burn = NULL;						    break;
		case CPUINFO_PTR_DISASSEMBLE:					info->disassemble = hyperstone_dasm;		break;
		case CPUINFO_PTR_INSTRUCTION_COUNTER:			info->icount = &hyperstone_ICount;			break;
		case CPUINFO_PTR_REGISTER_LAYOUT:				info->p = hyperstone_reg_layout;			break;
		case CPUINFO_PTR_WINDOW_LAYOUT:					info->p = hyperstone_win_layout;			break;

		case CPUINFO_PTR_INTERNAL_MEMORY_MAP + ADDRESS_SPACE_DATA:    info->internal_map = 0; break;
		case CPUINFO_PTR_INTERNAL_MEMORY_MAP + ADDRESS_SPACE_IO:      info->internal_map = 0; break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case CPUINFO_STR_CORE_FAMILY:					strcpy(info->s = cpuintrf_temp_str(), "Hyperstone CPU"); break;
		case CPUINFO_STR_CORE_VERSION:					strcpy(info->s = cpuintrf_temp_str(), "0.9"); break;
		case CPUINFO_STR_CORE_FILE:						strcpy(info->s = cpuintrf_temp_str(), __FILE__); break;
		case CPUINFO_STR_CORE_CREDITS:					strcpy(info->s = cpuintrf_temp_str(), "Copyright Pierpaolo Prazzoli and Ryan Holtz"); break;

		case CPUINFO_STR_FLAGS:
			sprintf(info->s = cpuintrf_temp_str(), "%c%c%c%c%c%c%c%c%c%c%c%c FTE:%X FRM:%X ILC:%d FL:%d FP:%d",
				GET_S ? 'S':'.',
				GET_P ? 'P':'.',
				GET_T ? 'T':'.',
				GET_L ? 'L':'.',
				GET_I ? 'I':'.',
				hyperstone.global_regs[1] & 0x00040 ? '?':'.',
				GET_H ? 'H':'.',
				GET_M ? 'M':'.',
				GET_V ? 'V':'.',
				GET_N ? 'N':'.',
				GET_Z ? 'Z':'.',
				GET_C ? 'C':'.',
				GET_FTE,
				GET_FRM,
				GET_ILC,
				GET_FL,
				GET_FP);
			break;

		case CPUINFO_STR_REGISTER + E132XS_PC:  		sprintf(info->s = cpuintrf_temp_str(), "PC  :%08X", hyperstone.global_regs[0]); break;
		case CPUINFO_STR_REGISTER + E132XS_SR:  		sprintf(info->s = cpuintrf_temp_str(), "SR  :%08X", hyperstone.global_regs[1]); break;
		case CPUINFO_STR_REGISTER + E132XS_FER: 		sprintf(info->s = cpuintrf_temp_str(), "FER :%08X", hyperstone.global_regs[2]); break;
		case CPUINFO_STR_REGISTER + E132XS_G3:  		sprintf(info->s = cpuintrf_temp_str(), "G3  :%08X", hyperstone.global_regs[3]); break;
		case CPUINFO_STR_REGISTER + E132XS_G4:  		sprintf(info->s = cpuintrf_temp_str(), "G4  :%08X", hyperstone.global_regs[4]); break;
		case CPUINFO_STR_REGISTER + E132XS_G5:  		sprintf(info->s = cpuintrf_temp_str(), "G5  :%08X", hyperstone.global_regs[5]); break;
		case CPUINFO_STR_REGISTER + E132XS_G6:  		sprintf(info->s = cpuintrf_temp_str(), "G6  :%08X", hyperstone.global_regs[6]); break;
		case CPUINFO_STR_REGISTER + E132XS_G7:  		sprintf(info->s = cpuintrf_temp_str(), "G7  :%08X", hyperstone.global_regs[7]); break;
		case CPUINFO_STR_REGISTER + E132XS_G8:  		sprintf(info->s = cpuintrf_temp_str(), "G8  :%08X", hyperstone.global_regs[8]); break;
		case CPUINFO_STR_REGISTER + E132XS_G9:  		sprintf(info->s = cpuintrf_temp_str(), "G9  :%08X", hyperstone.global_regs[9]); break;
		case CPUINFO_STR_REGISTER + E132XS_G10: 		sprintf(info->s = cpuintrf_temp_str(), "G10 :%08X", hyperstone.global_regs[10]); break;
		case CPUINFO_STR_REGISTER + E132XS_G11: 		sprintf(info->s = cpuintrf_temp_str(), "G11 :%08X", hyperstone.global_regs[11]); break;
		case CPUINFO_STR_REGISTER + E132XS_G12: 		sprintf(info->s = cpuintrf_temp_str(), "G12 :%08X", hyperstone.global_regs[12]); break;
		case CPUINFO_STR_REGISTER + E132XS_G13: 		sprintf(info->s = cpuintrf_temp_str(), "G13 :%08X", hyperstone.global_regs[13]); break;
		case CPUINFO_STR_REGISTER + E132XS_G14: 		sprintf(info->s = cpuintrf_temp_str(), "G14 :%08X", hyperstone.global_regs[14]); break;
		case CPUINFO_STR_REGISTER + E132XS_G15: 		sprintf(info->s = cpuintrf_temp_str(), "G15 :%08X", hyperstone.global_regs[15]); break;
		case CPUINFO_STR_REGISTER + E132XS_G16: 		sprintf(info->s = cpuintrf_temp_str(), "G16 :%08X", hyperstone.global_regs[16]); break;
		case CPUINFO_STR_REGISTER + E132XS_G17: 		sprintf(info->s = cpuintrf_temp_str(), "G17 :%08X", hyperstone.global_regs[17]); break;
		case CPUINFO_STR_REGISTER + E132XS_SP:  		sprintf(info->s = cpuintrf_temp_str(), "SP  :%08X", hyperstone.global_regs[18]); break;
		case CPUINFO_STR_REGISTER + E132XS_UB:  		sprintf(info->s = cpuintrf_temp_str(), "UB  :%08X", hyperstone.global_regs[19]); break;
		case CPUINFO_STR_REGISTER + E132XS_BCR: 		sprintf(info->s = cpuintrf_temp_str(), "BCR :%08X", hyperstone.global_regs[20]); break;
		case CPUINFO_STR_REGISTER + E132XS_TPR: 		sprintf(info->s = cpuintrf_temp_str(), "TPR :%08X", hyperstone.global_regs[21]); break;
		case CPUINFO_STR_REGISTER + E132XS_TCR: 		sprintf(info->s = cpuintrf_temp_str(), "TCR :%08X", hyperstone.global_regs[22]); break;
		case CPUINFO_STR_REGISTER + E132XS_TR:  		sprintf(info->s = cpuintrf_temp_str(), "TR  :%08X", hyperstone.global_regs[23]); break;
		case CPUINFO_STR_REGISTER + E132XS_WCR: 		sprintf(info->s = cpuintrf_temp_str(), "WCR :%08X", hyperstone.global_regs[24]); break;
		case CPUINFO_STR_REGISTER + E132XS_ISR: 		sprintf(info->s = cpuintrf_temp_str(), "ISR :%08X", hyperstone.global_regs[25]); break;
		case CPUINFO_STR_REGISTER + E132XS_FCR: 		sprintf(info->s = cpuintrf_temp_str(), "FCR :%08X", hyperstone.global_regs[26]); break;
		case CPUINFO_STR_REGISTER + E132XS_MCR: 		sprintf(info->s = cpuintrf_temp_str(), "MCR :%08X", hyperstone.global_regs[27]); break;
		case CPUINFO_STR_REGISTER + E132XS_G28: 		sprintf(info->s = cpuintrf_temp_str(), "G28 :%08X", hyperstone.global_regs[28]); break;
		case CPUINFO_STR_REGISTER + E132XS_G29: 		sprintf(info->s = cpuintrf_temp_str(), "G29 :%08X", hyperstone.global_regs[29]); break;
		case CPUINFO_STR_REGISTER + E132XS_G30: 		sprintf(info->s = cpuintrf_temp_str(), "G30 :%08X", hyperstone.global_regs[30]); break;
		case CPUINFO_STR_REGISTER + E132XS_G31: 		sprintf(info->s = cpuintrf_temp_str(), "G31 :%08X", hyperstone.global_regs[31]); break;
		case CPUINFO_STR_REGISTER + E132XS_CL0:  		sprintf(info->s = cpuintrf_temp_str(), "CL0 :%08X", hyperstone.local_regs[(0 + GET_FP) % 64]); break;
		case CPUINFO_STR_REGISTER + E132XS_CL1:  		sprintf(info->s = cpuintrf_temp_str(), "CL1 :%08X", hyperstone.local_regs[(1 + GET_FP) % 64]); break;
		case CPUINFO_STR_REGISTER + E132XS_CL2:  		sprintf(info->s = cpuintrf_temp_str(), "CL2 :%08X", hyperstone.local_regs[(2 + GET_FP) % 64]); break;
		case CPUINFO_STR_REGISTER + E132XS_CL3:  		sprintf(info->s = cpuintrf_temp_str(), "CL3 :%08X", hyperstone.local_regs[(3 + GET_FP) % 64]); break;
		case CPUINFO_STR_REGISTER + E132XS_CL4:  		sprintf(info->s = cpuintrf_temp_str(), "CL4 :%08X", hyperstone.local_regs[(4 + GET_FP) % 64]); break;
		case CPUINFO_STR_REGISTER + E132XS_CL5:  		sprintf(info->s = cpuintrf_temp_str(), "CL5 :%08X", hyperstone.local_regs[(5 + GET_FP) % 64]); break;
		case CPUINFO_STR_REGISTER + E132XS_CL6:  		sprintf(info->s = cpuintrf_temp_str(), "CL6 :%08X", hyperstone.local_regs[(6 + GET_FP) % 64]); break;
		case CPUINFO_STR_REGISTER + E132XS_CL7:  		sprintf(info->s = cpuintrf_temp_str(), "CL7 :%08X", hyperstone.local_regs[(7 + GET_FP) % 64]); break;
		case CPUINFO_STR_REGISTER + E132XS_CL8:  		sprintf(info->s = cpuintrf_temp_str(), "CL8 :%08X", hyperstone.local_regs[(8 + GET_FP) % 64]); break;
		case CPUINFO_STR_REGISTER + E132XS_CL9:  		sprintf(info->s = cpuintrf_temp_str(), "CL9 :%08X", hyperstone.local_regs[(9 + GET_FP) % 64]); break;
		case CPUINFO_STR_REGISTER + E132XS_CL10: 		sprintf(info->s = cpuintrf_temp_str(), "CL10:%08X", hyperstone.local_regs[(10 + GET_FP) % 64]); break;
		case CPUINFO_STR_REGISTER + E132XS_CL11: 		sprintf(info->s = cpuintrf_temp_str(), "CL11:%08X", hyperstone.local_regs[(11 + GET_FP) % 64]); break;
		case CPUINFO_STR_REGISTER + E132XS_CL12: 		sprintf(info->s = cpuintrf_temp_str(), "CL12:%08X", hyperstone.local_regs[(12 + GET_FP) % 64]); break;
		case CPUINFO_STR_REGISTER + E132XS_CL13: 		sprintf(info->s = cpuintrf_temp_str(), "CL13:%08X", hyperstone.local_regs[(13 + GET_FP) % 64]); break;
		case CPUINFO_STR_REGISTER + E132XS_CL14: 		sprintf(info->s = cpuintrf_temp_str(), "CL14:%08X", hyperstone.local_regs[(14 + GET_FP) % 64]); break;
		case CPUINFO_STR_REGISTER + E132XS_CL15: 		sprintf(info->s = cpuintrf_temp_str(), "CL15:%08X", hyperstone.local_regs[(15 + GET_FP) % 64]); break;
		case CPUINFO_STR_REGISTER + E132XS_L0:  		sprintf(info->s = cpuintrf_temp_str(), "L0  :%08X", hyperstone.local_regs[0]); break;
		case CPUINFO_STR_REGISTER + E132XS_L1:  		sprintf(info->s = cpuintrf_temp_str(), "L1  :%08X", hyperstone.local_regs[1]); break;
		case CPUINFO_STR_REGISTER + E132XS_L2:  		sprintf(info->s = cpuintrf_temp_str(), "L2  :%08X", hyperstone.local_regs[2]); break;
		case CPUINFO_STR_REGISTER + E132XS_L3:  		sprintf(info->s = cpuintrf_temp_str(), "L3  :%08X", hyperstone.local_regs[3]); break;
		case CPUINFO_STR_REGISTER + E132XS_L4:  		sprintf(info->s = cpuintrf_temp_str(), "L4  :%08X", hyperstone.local_regs[4]); break;
		case CPUINFO_STR_REGISTER + E132XS_L5:  		sprintf(info->s = cpuintrf_temp_str(), "L5  :%08X", hyperstone.local_regs[5]); break;
		case CPUINFO_STR_REGISTER + E132XS_L6:  		sprintf(info->s = cpuintrf_temp_str(), "L6  :%08X", hyperstone.local_regs[6]); break;
		case CPUINFO_STR_REGISTER + E132XS_L7:  		sprintf(info->s = cpuintrf_temp_str(), "L7  :%08X", hyperstone.local_regs[7]); break;
		case CPUINFO_STR_REGISTER + E132XS_L8:  		sprintf(info->s = cpuintrf_temp_str(), "L8  :%08X", hyperstone.local_regs[8]); break;
		case CPUINFO_STR_REGISTER + E132XS_L9:  		sprintf(info->s = cpuintrf_temp_str(), "L9  :%08X", hyperstone.local_regs[9]); break;
		case CPUINFO_STR_REGISTER + E132XS_L10: 		sprintf(info->s = cpuintrf_temp_str(), "L10 :%08X", hyperstone.local_regs[10]); break;
		case CPUINFO_STR_REGISTER + E132XS_L11: 		sprintf(info->s = cpuintrf_temp_str(), "L11 :%08X", hyperstone.local_regs[11]); break;
		case CPUINFO_STR_REGISTER + E132XS_L12: 		sprintf(info->s = cpuintrf_temp_str(), "L12 :%08X", hyperstone.local_regs[12]); break;
		case CPUINFO_STR_REGISTER + E132XS_L13: 		sprintf(info->s = cpuintrf_temp_str(), "L13 :%08X", hyperstone.local_regs[13]); break;
		case CPUINFO_STR_REGISTER + E132XS_L14: 		sprintf(info->s = cpuintrf_temp_str(), "L14 :%08X", hyperstone.local_regs[14]); break;
		case CPUINFO_STR_REGISTER + E132XS_L15: 		sprintf(info->s = cpuintrf_temp_str(), "L15 :%08X", hyperstone.local_regs[15]); break;
		case CPUINFO_STR_REGISTER + E132XS_L16: 		sprintf(info->s = cpuintrf_temp_str(), "L16 :%08X", hyperstone.local_regs[16]); break;
		case CPUINFO_STR_REGISTER + E132XS_L17: 		sprintf(info->s = cpuintrf_temp_str(), "L17 :%08X", hyperstone.local_regs[17]); break;
		case CPUINFO_STR_REGISTER + E132XS_L18: 		sprintf(info->s = cpuintrf_temp_str(), "L18 :%08X", hyperstone.local_regs[18]); break;
		case CPUINFO_STR_REGISTER + E132XS_L19: 		sprintf(info->s = cpuintrf_temp_str(), "L19 :%08X", hyperstone.local_regs[19]); break;
		case CPUINFO_STR_REGISTER + E132XS_L20: 		sprintf(info->s = cpuintrf_temp_str(), "L20 :%08X", hyperstone.local_regs[20]); break;
		case CPUINFO_STR_REGISTER + E132XS_L21: 		sprintf(info->s = cpuintrf_temp_str(), "L21 :%08X", hyperstone.local_regs[21]); break;
		case CPUINFO_STR_REGISTER + E132XS_L22: 		sprintf(info->s = cpuintrf_temp_str(), "L22 :%08X", hyperstone.local_regs[22]); break;
		case CPUINFO_STR_REGISTER + E132XS_L23: 		sprintf(info->s = cpuintrf_temp_str(), "L23 :%08X", hyperstone.local_regs[23]); break;
		case CPUINFO_STR_REGISTER + E132XS_L24: 		sprintf(info->s = cpuintrf_temp_str(), "L24 :%08X", hyperstone.local_regs[24]); break;
		case CPUINFO_STR_REGISTER + E132XS_L25: 		sprintf(info->s = cpuintrf_temp_str(), "L25 :%08X", hyperstone.local_regs[25]); break;
		case CPUINFO_STR_REGISTER + E132XS_L26: 		sprintf(info->s = cpuintrf_temp_str(), "L26 :%08X", hyperstone.local_regs[26]); break;
		case CPUINFO_STR_REGISTER + E132XS_L27: 		sprintf(info->s = cpuintrf_temp_str(), "L27 :%08X", hyperstone.local_regs[27]); break;
		case CPUINFO_STR_REGISTER + E132XS_L28: 		sprintf(info->s = cpuintrf_temp_str(), "L28 :%08X", hyperstone.local_regs[28]); break;
		case CPUINFO_STR_REGISTER + E132XS_L29: 		sprintf(info->s = cpuintrf_temp_str(), "L29 :%08X", hyperstone.local_regs[29]); break;
		case CPUINFO_STR_REGISTER + E132XS_L30: 		sprintf(info->s = cpuintrf_temp_str(), "L30 :%08X", hyperstone.local_regs[30]); break;
		case CPUINFO_STR_REGISTER + E132XS_L31: 		sprintf(info->s = cpuintrf_temp_str(), "L31 :%08X", hyperstone.local_regs[31]); break;
		case CPUINFO_STR_REGISTER + E132XS_L32: 		sprintf(info->s = cpuintrf_temp_str(), "L32 :%08X", hyperstone.local_regs[32]); break;
		case CPUINFO_STR_REGISTER + E132XS_L33: 		sprintf(info->s = cpuintrf_temp_str(), "L33 :%08X", hyperstone.local_regs[33]); break;
		case CPUINFO_STR_REGISTER + E132XS_L34: 		sprintf(info->s = cpuintrf_temp_str(), "L34 :%08X", hyperstone.local_regs[34]); break;
		case CPUINFO_STR_REGISTER + E132XS_L35: 		sprintf(info->s = cpuintrf_temp_str(), "L35 :%08X", hyperstone.local_regs[35]); break;
		case CPUINFO_STR_REGISTER + E132XS_L36: 		sprintf(info->s = cpuintrf_temp_str(), "L36 :%08X", hyperstone.local_regs[36]); break;
		case CPUINFO_STR_REGISTER + E132XS_L37: 		sprintf(info->s = cpuintrf_temp_str(), "L37 :%08X", hyperstone.local_regs[37]); break;
		case CPUINFO_STR_REGISTER + E132XS_L38: 		sprintf(info->s = cpuintrf_temp_str(), "L38 :%08X", hyperstone.local_regs[38]); break;
		case CPUINFO_STR_REGISTER + E132XS_L39: 		sprintf(info->s = cpuintrf_temp_str(), "L39 :%08X", hyperstone.local_regs[39]); break;
		case CPUINFO_STR_REGISTER + E132XS_L40: 		sprintf(info->s = cpuintrf_temp_str(), "L40 :%08X", hyperstone.local_regs[40]); break;
		case CPUINFO_STR_REGISTER + E132XS_L41: 		sprintf(info->s = cpuintrf_temp_str(), "L41 :%08X", hyperstone.local_regs[41]); break;
		case CPUINFO_STR_REGISTER + E132XS_L42: 		sprintf(info->s = cpuintrf_temp_str(), "L42 :%08X", hyperstone.local_regs[42]); break;
		case CPUINFO_STR_REGISTER + E132XS_L43: 		sprintf(info->s = cpuintrf_temp_str(), "L43 :%08X", hyperstone.local_regs[43]); break;
		case CPUINFO_STR_REGISTER + E132XS_L44: 		sprintf(info->s = cpuintrf_temp_str(), "L44 :%08X", hyperstone.local_regs[44]); break;
		case CPUINFO_STR_REGISTER + E132XS_L45: 		sprintf(info->s = cpuintrf_temp_str(), "L45 :%08X", hyperstone.local_regs[45]); break;
		case CPUINFO_STR_REGISTER + E132XS_L46: 		sprintf(info->s = cpuintrf_temp_str(), "L46 :%08X", hyperstone.local_regs[46]); break;
		case CPUINFO_STR_REGISTER + E132XS_L47: 		sprintf(info->s = cpuintrf_temp_str(), "L47 :%08X", hyperstone.local_regs[47]); break;
		case CPUINFO_STR_REGISTER + E132XS_L48: 		sprintf(info->s = cpuintrf_temp_str(), "L48 :%08X", hyperstone.local_regs[48]); break;
		case CPUINFO_STR_REGISTER + E132XS_L49: 		sprintf(info->s = cpuintrf_temp_str(), "L49 :%08X", hyperstone.local_regs[49]); break;
		case CPUINFO_STR_REGISTER + E132XS_L50: 		sprintf(info->s = cpuintrf_temp_str(), "L50 :%08X", hyperstone.local_regs[50]); break;
		case CPUINFO_STR_REGISTER + E132XS_L51: 		sprintf(info->s = cpuintrf_temp_str(), "L51 :%08X", hyperstone.local_regs[51]); break;
		case CPUINFO_STR_REGISTER + E132XS_L52: 		sprintf(info->s = cpuintrf_temp_str(), "L52 :%08X", hyperstone.local_regs[52]); break;
		case CPUINFO_STR_REGISTER + E132XS_L53: 		sprintf(info->s = cpuintrf_temp_str(), "L53 :%08X", hyperstone.local_regs[53]); break;
		case CPUINFO_STR_REGISTER + E132XS_L54: 		sprintf(info->s = cpuintrf_temp_str(), "L54 :%08X", hyperstone.local_regs[54]); break;
		case CPUINFO_STR_REGISTER + E132XS_L55: 		sprintf(info->s = cpuintrf_temp_str(), "L55 :%08X", hyperstone.local_regs[55]); break;
		case CPUINFO_STR_REGISTER + E132XS_L56: 		sprintf(info->s = cpuintrf_temp_str(), "L56 :%08X", hyperstone.local_regs[56]); break;
		case CPUINFO_STR_REGISTER + E132XS_L57: 		sprintf(info->s = cpuintrf_temp_str(), "L57 :%08X", hyperstone.local_regs[57]); break;
		case CPUINFO_STR_REGISTER + E132XS_L58: 		sprintf(info->s = cpuintrf_temp_str(), "L58 :%08X", hyperstone.local_regs[58]); break;
		case CPUINFO_STR_REGISTER + E132XS_L59: 		sprintf(info->s = cpuintrf_temp_str(), "L59 :%08X", hyperstone.local_regs[59]); break;
		case CPUINFO_STR_REGISTER + E132XS_L60: 		sprintf(info->s = cpuintrf_temp_str(), "L60 :%08X", hyperstone.local_regs[60]); break;
		case CPUINFO_STR_REGISTER + E132XS_L61: 		sprintf(info->s = cpuintrf_temp_str(), "L61 :%08X", hyperstone.local_regs[61]); break;
		case CPUINFO_STR_REGISTER + E132XS_L62: 		sprintf(info->s = cpuintrf_temp_str(), "L62 :%08X", hyperstone.local_regs[62]); break;
		case CPUINFO_STR_REGISTER + E132XS_L63: 		sprintf(info->s = cpuintrf_temp_str(), "L63 :%08X", hyperstone.local_regs[63]); break;
	}
}


#if (HAS_E116T)
void e116t_get_info(UINT32 state, union cpuinfo *info)
{
	switch (state)
	{

		case CPUINFO_INT_DATABUS_WIDTH + ADDRESS_SPACE_PROGRAM:	info->i = 16;					break;
		case CPUINFO_INT_DATABUS_WIDTH + ADDRESS_SPACE_IO:		info->i = 16;					break;

		case CPUINFO_PTR_INTERNAL_MEMORY_MAP + ADDRESS_SPACE_PROGRAM: info->internal_map = &construct_map_e116_4k_iram_map; break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case CPUINFO_PTR_INIT:							info->init = e116_init;				break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case CPUINFO_STR_NAME:							strcpy(info->s = cpuintrf_temp_str(), "E1-16T"); break;

		default:
			hyperstone_get_info(state, info);
	}
}
#endif

#if (HAS_E116XT)
void e116xt_get_info(UINT32 state, union cpuinfo *info)
{
	switch (state)
	{

		case CPUINFO_INT_DATABUS_WIDTH + ADDRESS_SPACE_PROGRAM:	info->i = 16;					break;
		case CPUINFO_INT_DATABUS_WIDTH + ADDRESS_SPACE_IO:		info->i = 16;					break;

		case CPUINFO_PTR_INTERNAL_MEMORY_MAP + ADDRESS_SPACE_PROGRAM: info->internal_map = &construct_map_e116_8k_iram_map; break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case CPUINFO_PTR_INIT:							info->init = e116_init;				break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case CPUINFO_STR_NAME:							strcpy(info->s = cpuintrf_temp_str(), "E1-16XT"); break;

		default:
			hyperstone_get_info(state, info);
	}
}
#endif

#if (HAS_E116XS)
void e116xs_get_info(UINT32 state, union cpuinfo *info)
{
	switch (state)
	{

		case CPUINFO_INT_DATABUS_WIDTH + ADDRESS_SPACE_PROGRAM:	info->i = 16;					break;
		case CPUINFO_INT_DATABUS_WIDTH + ADDRESS_SPACE_IO:		info->i = 16;					break;

		case CPUINFO_PTR_INTERNAL_MEMORY_MAP + ADDRESS_SPACE_PROGRAM: info->internal_map = &construct_map_e116_16k_iram_map; break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case CPUINFO_PTR_INIT:							info->init = e116_init;				break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case CPUINFO_STR_NAME:							strcpy(info->s = cpuintrf_temp_str(), "E1-16XS"); break;

		default:
			hyperstone_get_info(state, info);
	}
}
#endif

#if (HAS_E116XSR)
void e116xsr_get_info(UINT32 state, union cpuinfo *info)
{
	switch (state)
	{

		case CPUINFO_INT_DATABUS_WIDTH + ADDRESS_SPACE_PROGRAM:	info->i = 16;					break;
		case CPUINFO_INT_DATABUS_WIDTH + ADDRESS_SPACE_IO:		info->i = 16;					break;

		case CPUINFO_PTR_INTERNAL_MEMORY_MAP + ADDRESS_SPACE_PROGRAM: info->internal_map = &construct_map_e116_16k_iram_map; break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case CPUINFO_PTR_INIT:							info->init = e116_init;				break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case CPUINFO_STR_NAME:							strcpy(info->s = cpuintrf_temp_str(), "E1-16XSR"); break;

		default:
			hyperstone_get_info(state, info);
	}
}
#endif

#if (HAS_E132N)
void e132n_get_info(UINT32 state, union cpuinfo *info)
{
	switch (state)
	{

		case CPUINFO_INT_DATABUS_WIDTH + ADDRESS_SPACE_PROGRAM:	info->i = 32;					break;
		case CPUINFO_INT_DATABUS_WIDTH + ADDRESS_SPACE_IO:		info->i = 32;					break;

		case CPUINFO_PTR_INTERNAL_MEMORY_MAP + ADDRESS_SPACE_PROGRAM: info->internal_map = &construct_map_e132_4k_iram_map; break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case CPUINFO_PTR_INIT:							info->init = e132_init;				break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case CPUINFO_STR_NAME:							strcpy(info->s = cpuintrf_temp_str(), "E1-32N"); break;

		default:
			hyperstone_get_info(state, info);
	}
}
#endif

#if (HAS_E132T)
void e132t_get_info(UINT32 state, union cpuinfo *info)
{
	switch (state)
	{

		case CPUINFO_INT_DATABUS_WIDTH + ADDRESS_SPACE_PROGRAM:	info->i = 32;					break;
		case CPUINFO_INT_DATABUS_WIDTH + ADDRESS_SPACE_IO:		info->i = 32;					break;

		case CPUINFO_PTR_INTERNAL_MEMORY_MAP + ADDRESS_SPACE_PROGRAM: info->internal_map = &construct_map_e132_4k_iram_map; break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case CPUINFO_PTR_INIT:							info->init = e132_init;				break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case CPUINFO_STR_NAME:							strcpy(info->s = cpuintrf_temp_str(), "E1-32T"); break;

		default:
			hyperstone_get_info(state, info);
	}
}
#endif

#if (HAS_E132XN)
void e132xn_get_info(UINT32 state, union cpuinfo *info)
{
	switch (state)
	{

		case CPUINFO_INT_DATABUS_WIDTH + ADDRESS_SPACE_PROGRAM:	info->i = 32;					break;
		case CPUINFO_INT_DATABUS_WIDTH + ADDRESS_SPACE_IO:		info->i = 32;					break;

		case CPUINFO_PTR_INTERNAL_MEMORY_MAP + ADDRESS_SPACE_PROGRAM: info->internal_map = &construct_map_e132_8k_iram_map; break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case CPUINFO_PTR_INIT:							info->init = e132_init;				break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case CPUINFO_STR_NAME:							strcpy(info->s = cpuintrf_temp_str(), "E1-32XN"); break;

		default:
			hyperstone_get_info(state, info);
	}
}
#endif

#if (HAS_E132XT)
void e132xt_get_info(UINT32 state, union cpuinfo *info)
{
	switch (state)
	{

		case CPUINFO_INT_DATABUS_WIDTH + ADDRESS_SPACE_PROGRAM:	info->i = 32;					break;
		case CPUINFO_INT_DATABUS_WIDTH + ADDRESS_SPACE_IO:		info->i = 32;					break;

		case CPUINFO_PTR_INTERNAL_MEMORY_MAP + ADDRESS_SPACE_PROGRAM: info->internal_map = &construct_map_e132_8k_iram_map; break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case CPUINFO_PTR_INIT:							info->init = e132_init;				break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case CPUINFO_STR_NAME:							strcpy(info->s = cpuintrf_temp_str(), "E1-32XT"); break;

		default:
			hyperstone_get_info(state, info);
	}
}
#endif

#if (HAS_E132XS)
void e132xs_get_info(UINT32 state, union cpuinfo *info)
{
	switch (state)
	{

		case CPUINFO_INT_DATABUS_WIDTH + ADDRESS_SPACE_PROGRAM:	info->i = 32;					break;
		case CPUINFO_INT_DATABUS_WIDTH + ADDRESS_SPACE_IO:		info->i = 32;					break;

		case CPUINFO_PTR_INTERNAL_MEMORY_MAP + ADDRESS_SPACE_PROGRAM: info->internal_map = &construct_map_e132_16k_iram_map; break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case CPUINFO_PTR_INIT:							info->init = e132_init;				break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case CPUINFO_STR_NAME:							strcpy(info->s = cpuintrf_temp_str(), "E1-32XS"); break;

		default:
			hyperstone_get_info(state, info);
	}
}
#endif

#if (HAS_E132XSR)
void e132xsr_get_info(UINT32 state, union cpuinfo *info)
{
	switch (state)
	{

		case CPUINFO_INT_DATABUS_WIDTH + ADDRESS_SPACE_PROGRAM:	info->i = 32;					break;
		case CPUINFO_INT_DATABUS_WIDTH + ADDRESS_SPACE_IO:		info->i = 32;					break;

		case CPUINFO_PTR_INTERNAL_MEMORY_MAP + ADDRESS_SPACE_PROGRAM: info->internal_map = &construct_map_e132_16k_iram_map; break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case CPUINFO_PTR_INIT:							info->init = e132_init;				break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case CPUINFO_STR_NAME:							strcpy(info->s = cpuintrf_temp_str(), "E1-32XSR"); break;

		default:
			hyperstone_get_info(state, info);
	}
}
#endif

#if (HAS_GMS30C2116)
void gms30c2116_get_info(UINT32 state, union cpuinfo *info)
{
	switch (state)
	{

		case CPUINFO_INT_DATABUS_WIDTH + ADDRESS_SPACE_PROGRAM:	info->i = 16;					break;
		case CPUINFO_INT_DATABUS_WIDTH + ADDRESS_SPACE_IO:		info->i = 16;					break;

		case CPUINFO_PTR_INTERNAL_MEMORY_MAP + ADDRESS_SPACE_PROGRAM: info->internal_map = &construct_map_e116_4k_iram_map; break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case CPUINFO_PTR_INIT:							info->init = e116_init;				break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case CPUINFO_STR_NAME:							strcpy(info->s = cpuintrf_temp_str(), "GMS30C2116"); break;

		default:
			hyperstone_get_info(state, info);
	}
}
#endif

#if (HAS_GMS30C2132)
void gms30c2132_get_info(UINT32 state, union cpuinfo *info)
{
	switch (state)
	{

		case CPUINFO_INT_DATABUS_WIDTH + ADDRESS_SPACE_PROGRAM:	info->i = 32;					break;
		case CPUINFO_INT_DATABUS_WIDTH + ADDRESS_SPACE_IO:		info->i = 32;					break;

		case CPUINFO_PTR_INTERNAL_MEMORY_MAP + ADDRESS_SPACE_PROGRAM: info->internal_map = &construct_map_e132_4k_iram_map; break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case CPUINFO_PTR_INIT:							info->init = e132_init;				break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case CPUINFO_STR_NAME:							strcpy(info->s = cpuintrf_temp_str(), "GMS30C2132"); break;

		default:
			hyperstone_get_info(state, info);
	}
}
#endif

#if (HAS_GMS30C2216)
void gms30c2216_get_info(UINT32 state, union cpuinfo *info)
{
	switch (state)
	{

		case CPUINFO_INT_DATABUS_WIDTH + ADDRESS_SPACE_PROGRAM:	info->i = 16;					break;
		case CPUINFO_INT_DATABUS_WIDTH + ADDRESS_SPACE_IO:		info->i = 16;					break;

		case CPUINFO_PTR_INTERNAL_MEMORY_MAP + ADDRESS_SPACE_PROGRAM: info->internal_map = &construct_map_e116_8k_iram_map; break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case CPUINFO_PTR_INIT:							info->init = e116_init;				break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case CPUINFO_STR_NAME:							strcpy(info->s = cpuintrf_temp_str(), "GMS30C2216"); break;

		default:
			hyperstone_get_info(state, info);
	}
}
#endif

#if (HAS_GMS30C2232)
void gms30c2232_get_info(UINT32 state, union cpuinfo *info)
{
	switch (state)
	{

		case CPUINFO_INT_DATABUS_WIDTH + ADDRESS_SPACE_PROGRAM:	info->i = 32;					break;
		case CPUINFO_INT_DATABUS_WIDTH + ADDRESS_SPACE_IO:		info->i = 32;					break;

		case CPUINFO_PTR_INTERNAL_MEMORY_MAP + ADDRESS_SPACE_PROGRAM: info->internal_map = &construct_map_e132_8k_iram_map; break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case CPUINFO_PTR_INIT:							info->init = e132_init;				break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case CPUINFO_STR_NAME:							strcpy(info->s = cpuintrf_temp_str(), "GMS30C2232"); break;

		default:
			hyperstone_get_info(state, info);
	}
}
#endif
