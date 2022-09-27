/*********************************************************************

	lisa.h
	
	MESS Lisa driver declarations

*********************************************************************/

#ifndef LISA_H
#define LISA_H

#include "osdepend.h"

extern UINT8 *lisa_fdc_rom;
extern UINT8 *lisa_fdc_ram;

VIDEO_START( lisa );
VIDEO_UPDATE( lisa );

extern NVRAM_HANDLER(lisa);

void init_lisa2(void);
void init_lisa210(void);
void init_mac_xl(void);

MACHINE_RESET( lisa );

void lisa_interrupt(void);

READ8_HANDLER ( lisa_fdc_io_r );
WRITE8_HANDLER ( lisa_fdc_io_w );
READ8_HANDLER ( lisa_fdc_r );
READ8_HANDLER ( lisa210_fdc_r );
WRITE8_HANDLER ( lisa_fdc_w );
WRITE8_HANDLER ( lisa210_fdc_w );
READ16_HANDLER ( lisa_r );
WRITE16_HANDLER ( lisa_w );

#endif /* LISA_H */

