/**********************************************************************

    MOS 6526/8520 CIA interface and emulation

    This function emulates all the functionality of up to 2 MOS6526 or
    MOS8520 complex interface adapters.

**********************************************************************/

#ifndef _6526CIA_H_
#define _6526CIA_H_

#include "memory.h"

typedef enum
{
	CIA6526,
	CIA8520
} cia_type_t;

typedef struct _cia6526_interface cia6526_interface;
struct _cia6526_interface
{
	cia_type_t type;
	void (*irq_func)(int state);
	double clock;
	double tod_clock;

	struct
	{
		UINT8	(*read)(void);
		void	(*write)(UINT8);
	} port[2];
};

/* configuration and reset */
void cia_config(int which, const cia6526_interface *intf);
void cia_reset(void);

/* reading and writing */
UINT8 cia_read(int which, offs_t offset);
void cia_write(int which, offs_t offset, UINT8 data);
void cia_clock_tod(int which);
void cia_issue_index(int which);

/* accessors */
UINT8 cia_get_output_a(int which);
UINT8 cia_get_output_b(int which);
int cia_get_irq(int which);

/* standard handlers */
READ8_HANDLER( cia_0_r );
READ8_HANDLER( cia_1_r );

WRITE8_HANDLER( cia_0_w );
WRITE8_HANDLER( cia_1_w );

#endif /* _6526CIA_H_ */
