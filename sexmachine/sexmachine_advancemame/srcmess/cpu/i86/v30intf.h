#ifndef __V30INTRF_H_
#define __V30INTRF_H_

#include "i86intf.h"

/* Public functions */
void v30_get_info(UINT32 state, union cpuinfo *info);

#ifdef MAME_DEBUG
extern unsigned DasmV30(char* buffer, unsigned pc);
#endif

#endif
