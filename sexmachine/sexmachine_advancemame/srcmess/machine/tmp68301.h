#ifndef TMP68301_H
#define TMP68301_H

#include "driver.h"

// Machine init
MACHINE_RESET( tmp68301 );

// Hardware Registers
extern UINT16 *tmp68301_regs;
WRITE16_HANDLER( tmp68301_regs_w );

// Interrupts
void tmp68301_external_interrupt_0(void);
void tmp68301_external_interrupt_1(void);
void tmp68301_external_interrupt_2(void);

#endif
