/***************************************************************************

 h8priv.h : Private constants and other definitions for the H8/3002 emulator.

****************************************************************************/

#ifndef _H8PRIV_H_
#define _H8PRIV_H_

typedef struct
{
	// main CPU stuff
	UINT32 h8err;
	UINT32 regs[8];
	UINT32 pc, ppc;
	UINT32 h8_IRQrequestH, h8_IRQrequestL;

	UINT8  ccr;
	UINT8  h8nflag, h8vflag, h8cflag, h8zflag, h8iflag, h8hflag;
	UINT8  h8uflag, h8uiflag;

	int (*irq_cb)(int);

	// H8/3002 onboard peripherals stuff

	UINT8 per_regs[256];

	UINT16 h8TCNT0, h8TCNT1, h8TCNT2, h8TCNT3, h8TCNT4;
	UINT8 h8TSTR;

	void *timer[5];

	int cpu_number;

} h83002_state;

extern h83002_state h8;

void h8_itu_init(void);
void h8_itu_reset(void);
UINT8 h8_itu_read8(UINT8 reg);
void h8_itu_write8(UINT8 reg, UINT8 val);

#endif
