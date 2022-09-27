/***************************************************************************

    TMP68301 basic emulation + Interrupt Handling

    The Toshiba TMP68301 is a 68HC000 + serial I/O, parallel I/O,
    3 timers, address decoder, wait generator, interrupt controller,
    all integrated in a single chip.

***************************************************************************/

#include "driver.h"
#include "machine/tmp68301.h"

UINT16 *tmp68301_regs;

static UINT8 tmp68301_IE[3];		// 3 External Interrupt Lines
static void *tmp68301_timer[3];		// 3 Timers

static int tmp68301_irq_vector[8];

void tmp68301_update_timer( int i );

int tmp68301_irq_callback(int int_level)
{
	int vector = tmp68301_irq_vector[int_level];
//  logerror("CPU #0 PC %06X: irq callback returns %04X for level %x\n",activecpu_get_pc(),vector,int_level);
	return vector;
}

void tmp68301_timer_callback(int i)
{
	UINT16 TCR	=	tmp68301_regs[(0x200 + i * 0x20)/2];
	UINT16 IMR	=	tmp68301_regs[0x94/2];		// Interrupt Mask Register (IMR)
	UINT16 ICR	=	tmp68301_regs[0x8e/2+i];	// Interrupt Controller Register (ICR7..9)
	UINT16 IVNR	=	tmp68301_regs[0x9a/2];		// Interrupt Vector Number Register (IVNR)

//  logerror("CPU #0 PC %06X: callback timer %04X, j = %d\n",activecpu_get_pc(),i,tcount);

	if	(	(TCR & 0x0004) &&	// INT
			!(IMR & (0x100<<i))
		)
	{
		int level = ICR & 0x0007;

		// Interrupt Vector Number Register (IVNR)
		tmp68301_irq_vector[level]	=	IVNR & 0x00e0;
		tmp68301_irq_vector[level]	+=	4+i;

		cpunum_set_input_line(0,level,HOLD_LINE);
	}

	if (TCR & 0x0080)	// N/1
	{
		// Repeat
		tmp68301_update_timer(i);
	}
	else
	{
		// One Shot
	}
}

void tmp68301_update_timer( int i )
{
	UINT16 TCR	=	tmp68301_regs[(0x200 + i * 0x20)/2];
	UINT16 MAX1	=	tmp68301_regs[(0x204 + i * 0x20)/2];
	UINT16 MAX2	=	tmp68301_regs[(0x206 + i * 0x20)/2];

	int max = 0;
	double duration = 0;

	timer_adjust(tmp68301_timer[i],TIME_NEVER,i,0);

	// timers 1&2 only
	switch( (TCR & 0x0030)>>4 )						// MR2..1
	{
	case 1:
		max = MAX1;
		break;
	case 2:
		max = MAX2;
		break;
	}

	switch ( (TCR & 0xc000)>>14 )					// CK2..1
	{
	case 0:	// System clock (CLK)
		if (max)
		{
			int scale = (TCR & 0x3c00)>>10;			// P4..1
			if (scale > 8) scale = 8;
			duration = Machine->drv->cpu[0].cpu_clock;
			duration /= 1 << scale;
			duration /= max;
		}
		break;
	}

//  logerror("CPU #0 PC %06X: TMP68301 Timer %d, duration %lf, max %04X\n",activecpu_get_pc(),i,duration,max);

	if (!(TCR & 0x0002))				// CS
	{
		if (duration)
			timer_adjust(tmp68301_timer[i],TIME_IN_HZ(duration),i,0);
		else
			logerror("CPU #0 PC %06X: TMP68301 error, timer %d duration is 0\n",activecpu_get_pc(),i);
	}
}

MACHINE_RESET( tmp68301 )
{
	int i;
	for (i = 0; i < 3; i++)
		tmp68301_timer[i] = timer_alloc(tmp68301_timer_callback);

	for (i = 0; i < 3; i++)
		tmp68301_IE[i] = 0;

	cpunum_set_irq_callback(0, tmp68301_irq_callback);
}

/* Update the IRQ state based on all possible causes */
static void update_irq_state(void)
{
	int i;

	/* Take care of external interrupts */

	UINT16 IMR	=	tmp68301_regs[0x94/2];		// Interrupt Mask Register (IMR)
	UINT16 IVNR	=	tmp68301_regs[0x9a/2];		// Interrupt Vector Number Register (IVNR)

	for (i = 0; i < 3; i++)
	{
		if	(	(tmp68301_IE[i]) &&
				!(IMR & (1<<i))
			)
		{
			UINT16 ICR	=	tmp68301_regs[0x80/2+i];	// Interrupt Controller Register (ICR0..2)

			// Interrupt Controller Register (ICR0..2)
			int level = ICR & 0x0007;

			// Interrupt Vector Number Register (IVNR)
			tmp68301_irq_vector[level]	=	IVNR & 0x00e0;
			tmp68301_irq_vector[level]	+=	i;

			tmp68301_IE[i] = 0;		// Interrupts are edge triggerred

			cpunum_set_input_line(0,level,HOLD_LINE);
		}
	}
}

WRITE16_HANDLER( tmp68301_regs_w )
{
	COMBINE_DATA(&tmp68301_regs[offset]);

	if (!ACCESSING_LSB)	return;

//  logerror("CPU #0 PC %06X: TMP68301 Reg %04X<-%04X & %04X\n",activecpu_get_pc(),offset*2,data,mem_mask^0xffff);

	switch( offset * 2 )
	{
		// Timers
		case 0x200:
		case 0x220:
		case 0x240:
		{
			int i = ((offset*2) >> 5) & 3;

			tmp68301_update_timer( i );
		}
		break;
	}
}

void tmp68301_external_interrupt_0()	{ tmp68301_IE[0] = 1;	update_irq_state(); }
void tmp68301_external_interrupt_1()	{ tmp68301_IE[1] = 1;	update_irq_state(); }
void tmp68301_external_interrupt_2()	{ tmp68301_IE[2] = 1;	update_irq_state(); }

