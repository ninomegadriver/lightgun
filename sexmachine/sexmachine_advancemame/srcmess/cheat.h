/*********************************************************************

    cheat.h

    Cheat system.

    Copyright (c) 1996-2006, Nicola Salmoria and the MAME Team.
    Visit http://mamedev.org for licensing and usage restrictions.

*********************************************************************/

#ifndef __CHEAT_H__
#define __CHEAT_H__

extern int he_did_cheat;

void cheat_init(void);

int cheat_menu(int selection);

void cheat_display_watches(void);

#endif	/* __CHEAT_H__ */
