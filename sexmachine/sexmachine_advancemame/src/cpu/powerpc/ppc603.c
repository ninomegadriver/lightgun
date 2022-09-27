void ppc603_exception(int exception)
{
	switch( exception )
	{
		case EXCEPTION_IRQ:		/* External Interrupt */
			if( ppc_get_msr() & MSR_EE ) {
				UINT32 msr = ppc_get_msr();

				SRR0 = ppc.npc;
				SRR1 = msr & 0xff73;

				msr &= ~(MSR_POW | MSR_EE | MSR_PR | MSR_FP | MSR_FE0 | MSR_SE | MSR_BE | MSR_FE1 | MSR_IR | MSR_DR | MSR_RI);
				if( msr & MSR_ILE )
					msr |= MSR_LE;
				else
					msr &= ~MSR_LE;
				ppc_set_msr(msr);

				if( msr & MSR_IP )
					ppc.npc = 0xfff00000 | 0x0500;
				else
					ppc.npc = 0x00000000 | 0x0500;

				ppc.interrupt_pending &= ~0x1;
				change_pc(ppc.npc);
			}
			break;

		case EXCEPTION_DECREMENTER:		/* Decrementer overflow exception */
			if( ppc_get_msr() & MSR_EE ) {
				UINT32 msr = ppc_get_msr();

				SRR0 = ppc.npc;
				SRR1 = msr & 0xff73;

				msr &= ~(MSR_POW | MSR_EE | MSR_PR | MSR_FP | MSR_FE0 | MSR_SE | MSR_BE | MSR_FE1 | MSR_IR | MSR_DR | MSR_RI);
				if( msr & MSR_ILE )
					msr |= MSR_LE;
				else
					msr &= ~MSR_LE;
				ppc_set_msr(msr);

				if( msr & MSR_IP )
					ppc.npc = 0xfff00000 | 0x0900;
				else
					ppc.npc = 0x00000000 | 0x0900;

				ppc.interrupt_pending &= ~0x2;
				change_pc(ppc.npc);
			}
			break;

		case EXCEPTION_TRAP:			/* Program exception / Trap */
			{
				UINT32 msr = ppc_get_msr();

				SRR0 = ppc.pc;
				SRR1 = (msr & 0xff73) | 0x20000;	/* 0x20000 = TRAP bit */

				msr &= ~(MSR_POW | MSR_EE | MSR_PR | MSR_FP | MSR_FE0 | MSR_SE | MSR_BE | MSR_FE1 | MSR_IR | MSR_DR | MSR_RI);
				if( msr & MSR_ILE )
					msr |= MSR_LE;
				else
					msr &= ~MSR_LE;
				ppc_set_msr(msr);

				if( msr & MSR_IP )
					ppc.npc = 0xfff00000 | 0x0700;
				else
					ppc.npc = 0x00000000 | 0x0700;
				change_pc(ppc.npc);
			}
			break;

		case EXCEPTION_SYSTEM_CALL:		/* System call */
			{
				UINT32 msr = ppc_get_msr();

				SRR0 = ppc.npc;
				SRR1 = (msr & 0xff73);

				msr &= ~(MSR_POW | MSR_EE | MSR_PR | MSR_FP | MSR_FE0 | MSR_SE | MSR_BE | MSR_FE1 | MSR_IR | MSR_DR | MSR_RI);
				if( msr & MSR_ILE )
					msr |= MSR_LE;
				else
					msr &= ~MSR_LE;
				ppc_set_msr(msr);

				if( msr & MSR_IP )
					ppc.npc = 0xfff00000 | 0x0c00;
				else
					ppc.npc = 0x00000000 | 0x0c00;
				change_pc(ppc.npc);
			}
			break;

		case EXCEPTION_SMI:
			if( ppc_get_msr() & MSR_EE ) {
				UINT32 msr = ppc_get_msr();

				SRR0 = ppc.npc;
				SRR1 = msr & 0xff73;

				msr &= ~(MSR_POW | MSR_EE | MSR_PR | MSR_FP | MSR_FE0 | MSR_SE | MSR_BE | MSR_FE1 | MSR_IR | MSR_DR | MSR_RI);
				if( msr & MSR_ILE )
					msr |= MSR_LE;
				else
					msr &= ~MSR_LE;
				ppc_set_msr(msr);

				if( msr & MSR_IP )
					ppc.npc = 0xfff00000 | 0x1400;
				else
					ppc.npc = 0x00000000 | 0x1400;

				ppc.interrupt_pending &= ~0x4;
				change_pc(ppc.npc);
			}
			break;

		case EXCEPTION_DSI:
			{
				UINT32 msr = ppc_get_msr();

				SRR0 = ppc.npc;
				SRR1 = msr & 0xff73;

				msr &= ~(MSR_POW | MSR_EE | MSR_PR | MSR_FP | MSR_FE0 | MSR_SE | MSR_BE | MSR_FE1 | MSR_IR | MSR_DR | MSR_RI);
				if( msr & MSR_ILE )
					msr |= MSR_LE;
				else
					msr &= ~MSR_LE;
				ppc_set_msr(msr);

				if( msr & MSR_IP )
					ppc.npc = 0xfff00000 | 0x0300;
				else
					ppc.npc = 0x00000000 | 0x0300;

				ppc.interrupt_pending &= ~0x4;
				change_pc(ppc.npc);
			}
			break;

		case EXCEPTION_ISI:
			{
				UINT32 msr = ppc_get_msr();

				SRR0 = ppc.npc;
				SRR1 = msr & 0xff73;

				msr &= ~(MSR_POW | MSR_EE | MSR_PR | MSR_FP | MSR_FE0 | MSR_SE | MSR_BE | MSR_FE1 | MSR_IR | MSR_DR | MSR_RI);
				if( msr & MSR_ILE )
					msr |= MSR_LE;
				else
					msr &= ~MSR_LE;
				ppc_set_msr(msr);

				if( msr & MSR_IP )
					ppc.npc = 0xfff00000 | 0x0400;
				else
					ppc.npc = 0x00000000 | 0x0400;

				ppc.interrupt_pending &= ~0x4;
				change_pc(ppc.npc);
			}
			break;

		default:
			fatalerror("ppc: Unhandled exception %d", exception);
			break;
	}
}

static void ppc603_set_irq_line(int irqline, int state)
{
	if( state ) {
		ppc.interrupt_pending |= 0x1;
		if (ppc.irq_callback)
		{
			ppc.irq_callback(irqline);
		}
	}
}

static void ppc603_set_smi_line(int state)
{
	if( state ) {
		ppc.interrupt_pending |= 0x4;
	}
}

INLINE void ppc603_check_interrupts(void)
{
	if (MSR & MSR_EE)
	{
		if (ppc.interrupt_pending != 0)
		{
			if (ppc.interrupt_pending & 0x1)
			{
				ppc603_exception(EXCEPTION_IRQ);
			}
			else if (ppc.interrupt_pending & 0x2)
			{
				ppc603_exception(EXCEPTION_DECREMENTER);
			}
			else if (ppc.interrupt_pending & 0x4)
			{
				ppc603_exception(EXCEPTION_SMI);
			}
		}
	}
}

static void ppc603_reset(void)
{
	ppc.pc = ppc.npc = 0xfff00100;

	ppc_set_msr(0x40);
	change_pc(ppc.pc);

	ppc.hid0 = 1;

	ppc.interrupt_pending = 0;
}


static int ppc603_execute(int cycles)
{
	int exception_type;
	UINT32 opcode;
	ppc_icount = cycles;
	ppc_tb_base_icount = cycles;
	ppc_dec_base_icount = cycles + ppc.dec_frac;

	// check if decrementer exception occurs during execution
	if ((UINT32)(DEC - ppc_icount) > (UINT32)(DEC))
	{
		ppc_dec_trigger_cycle = ppc_icount - DEC;
	}
	else
	{
		ppc_dec_trigger_cycle = 0x7fffffff;
	}

	change_pc(ppc.npc);

#ifdef __GNUC__
	// MinGW's optimizer kills setjmp()/longjmp()
	(void)__builtin_return_address(1);
#endif

	exception_type = setjmp(ppc.exception_jmpbuf);
	if (exception_type)
	{
		ppc.npc = ppc.pc;
		ppc603_exception(exception_type);
	}

	while( ppc_icount > 0 )
	{
		ppc.pc = ppc.npc;
		CALL_MAME_DEBUG;

		if (MSR & MSR_IR)
			opcode = ppc_readop_translated(ppc.pc);
		else
		opcode = ROPCODE64(ppc.pc);

		ppc.npc = ppc.pc + 4;
		switch(opcode >> 26)
		{
			case 19:	optable19[(opcode >> 1) & 0x3ff](opcode); break;
			case 31:	optable31[(opcode >> 1) & 0x3ff](opcode); break;
			case 59:	optable59[(opcode >> 1) & 0x3ff](opcode); break;
			case 63:	optable63[(opcode >> 1) & 0x3ff](opcode); break;
			default:	optable[opcode >> 26](opcode); break;
		}

		ppc_icount--;

		if(ppc_icount == ppc_dec_trigger_cycle) {
			ppc.interrupt_pending |= 0x2;
		}

		ppc603_check_interrupts();
	}

	// update timebase
	// timebase is incremented once every four core clock cycles, so adjust the cycles accordingly
	ppc.tb += ((ppc_tb_base_icount - ppc_icount) / 4);

	// update decrementer
	ppc.dec_frac = ((ppc_dec_base_icount - ppc_icount) % (bus_freq_multiplier * 2));
	DEC -= ((ppc_dec_base_icount - ppc_icount) / (bus_freq_multiplier * 2));

	return cycles - ppc_icount;
}
