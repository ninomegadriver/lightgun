/*
	rtc65271.h: include file for rtc65271.c
*/

#include "mame.h"

extern int rtc65271_file_load(mame_file *file);
extern int rtc65271_file_save(mame_file *file);
extern void rtc65271_init(UINT8 *xram, void (*interrupt_callback)(int state));
extern UINT8 rtc65271_r(int xramsel, offs_t offset);
extern void rtc65271_w(int xramsel, offs_t offset, UINT8 data);
