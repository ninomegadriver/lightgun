/*****************************************************************************
 *
 *	 tms7000.h (c header file)
 *	 Portable TMS7000 emulator (Texas Instruments 7000)
 *
 *	 Copyright (c) 2001 tim lindner, all rights reserved.
 *
 *	 - This source code is released as freeware for non-commercial purposes.
 *	 - You are free to use and redistribute this code in modified or
 *	   unmodified form, provided you list me in the credits.
 *	 - If you modify this source code, you must add a notice to each modified
 *	   source file that it has been changed.  If you're a nice person, you
 *	   will clearly mark each change too.  :)
 *	 - If you wish to use this for commercial purposes, please contact me at
 *	   tlindner@ix.netcom.com
 *   - This entire notice must remain in the source code.
 *
 *****************************************************************************/

#ifndef _TMS7000_H
#define _TMS7000_H

#include "memory.h"

enum { TMS7000_PC=1, TMS7000_SP, TMS7000_ST, TMS7000_IDLE, TMS7000_T1_CL, TMS7000_T1_PS, TMS7000_T1_DEC };

enum { TMS7000_VCC, TMS7000_VSS };

enum { TMS7000_NMOS, TMS7000_CMOS };

enum {
	TMS7000_IRQ1_LINE = 0,   /* INT1 */
	TMS7000_IRQ2_LINE,       /* INT2 */
	TMS7000_IRQ3_LINE,       /* INT3 */
	TMS7000_IRQNONE = 255
};

enum {
	TMS7000_PORTA = 0,
	TMS7000_PORTB,
	TMS7000_PORTC,
	TMS7000_PORTD
};

/* PUBLIC FUNCTIONS */
extern void tms7000_A6EC1( void ); /* External event counter */
extern void tms7000_get_info(UINT32 state, union cpuinfo *info);
extern void tms7000_exl_get_info(UINT32 state, union cpuinfo *info);

extern WRITE8_HANDLER( tms70x0_pf_w );
extern  READ8_HANDLER( tms70x0_pf_r );

//SJE: Added these..
extern WRITE8_HANDLER( tms7000_internal_w );
extern  READ8_HANDLER( tms7000_internal_r );

#ifdef MAME_DEBUG
extern unsigned tms7000_dasm( char *buffer, offs_t pc, UINT8 *oprom, UINT8 *opram, int bytes );
#endif

#endif /* _TMS7000_H */

