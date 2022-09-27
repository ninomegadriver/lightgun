/*
 * mathbox.h: math box simulation (Battlezone/Red Baron/Tempest)
 *
 * Copyright 1991, 1992, 1993, 1996 Eric Smith
 *
 */

typedef short s16;
typedef int s32;

WRITE8_HANDLER( mb_go_w );
READ8_HANDLER( mb_status_r );
READ8_HANDLER( mb_lo_r );
READ8_HANDLER( mb_hi_r );

extern s16 mb_result;
