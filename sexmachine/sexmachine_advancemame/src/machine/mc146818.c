/*********************************************************************

    mc146818.c

    Implementation of the MC146818 chip

    Real time clock chip with battery buffered ram (or CMOS)
    Used in IBM PC/AT, several PC clones, Amstrad NC200

    Nathan Woods  (npwoods@mess.org)
    Peter Trauner (peter.trauner@jk.uni-linz.ac.at)

    PC CMOS info (based on info from Padgett Peterson):

    Clock Related:
        0x00 Seconds       (BCD 00-59, Hex 00-3B) Note: Bit 7 is read only
        0x01 Second Alarm  (BCD 00-59, Hex 00-3B; "don't care" if C0-FF)
        0x02 Minutes       (BCD 00-59, Hex 00-3B)
        0x03 Minute Alarm  (BCD 00-59, Hex 00-3B; "don't care" if C0-FF))
        0x04 Hours         (BCD 00-23, Hex 00-17 if 24 hr mode)
                        (BCD 01-12, Hex 01-0C if 12 hr am)
                        (BCD 81-92. Hex 81-8C if 12 hr pm)
        0x05 Hour Alarm    (same as hours; "don't care" if C0-FF))
        0x06 Day of Week   (01-07 Sunday=1)
        0x07 Date of Month (BCD 01-31, Hex 01-1F)
        0x08 Month         (BCD 01-12, Hex 01-0C)
        0x09 Year          (BCD 00-99, Hex 00-63)
        0x0B Status Register B (read/write)
            Bit 7 - 1 enables cycle update, 0 disables
            Bit 6 - 1 enables periodic interrupt
            Bit 5 - 1 enables alarm interrupt
            Bit 4 - 1 enables update-ended interrupt
            Bit 3 - 1 enables square wave output
            Bit 2 - Data Mode - 0: BCD, 1: Binary
            Bit 1 - 24/12 hour selection - 1 enables 24 hour mode
            Bit 0 - Daylight Savings Enable - 1 enables
        0x0C Status Register C (Read only)
            Bit 7 - Interrupt request flag - 1 when any or all of bits 6-4 are
                        1 and appropriate enables (Register B) are set to 1. Generates
                        IRQ 8 when triggered.
            Bit 6 - Periodic Interrupt flag
            Bit 5 - Alarm Interrupt flag
            Bit 4 - Update-Ended Interrupt Flag
            Bit 3-0 ???
        0x0D Status Register D (read only)
            Bit 7 - Valid RAM - 1 indicates batery power good, 0 if dead or
                        disconnected.
            Bit 6-0 ???

    Non-clock related:
        0x0E (PS/2) Diagnostic Status Byte
            Bit 7 - When set (1) indicates clock has lost power
            Bit 6 - (1) indicates incorrect checksum
            Bit 5 - (1) indicates that equipment configuration is incorrect
                            power-on check requires that atleast one floppy be installed
            Bit 4 - (1) indicates error in memory size
            Bit 3 - (1) indicates that controller or disk drive failed initialization
            Bit 2 - (1) indicates that time is invalid
            Bit 1 - (1) indicates installed adaptors do not match configuration
            Bit 0 - (1) indicates a time-out while reading adaptor ID
        0x0E (AMSTRAD) 6  BYTEs time and date machine last used
        0x0F Reset Code (IBM PS/2 "Shutdown Status Byte")
            0x00-0x03   perform power-on reset
            0x04        INT 19h reboot
            0x05        flush keyboard and jump via 0040:0067
            0x06-0x07   reserved
            0x08        used by POST during protected-mode RAM test
            0x09        used for INT 15/87h (block move) support
            0x0A        jump via 0040:0067
            0x0B-0xFF   perform power-on reset

*********************************************************************/

#include "machine/mc146818.h"
#include "memconv.h"



#define LOG_MC146818		0
#define MC146818_DATA_SIZE	0x80

struct mc146818_chip
{
	MC146818_TYPE type;

	UINT8 index;
	UINT8 data[MC146818_DATA_SIZE];

	UINT16 eindex;
	UINT8 edata[0x2000];

	double last_refresh;
};

static struct mc146818_chip *mc146818;



#define HOURS_24	(mc146818->data[0xb]&2)
#define BCD_MODE	!(mc146818->data[0xb]&4) // book has other description!
#define CENTURY		mc146818->data[50]
#define YEAR		mc146818->data[9]
#define MONTH		mc146818->data[8]
#define DAY			mc146818->data[7]
#define WEEK_DAY	mc146818->data[6]



static void mc146818_timer(int param)
{
	int year, month;

	if (BCD_MODE)
	{
		mc146818->data[0]=bcd_adjust(mc146818->data[0]+1);
		if (mc146818->data[0]>=0x60)
		{
			mc146818->data[0]=0;
			mc146818->data[2]=bcd_adjust(mc146818->data[2]+1);
			if (mc146818->data[2]>=0x60)
			{
				mc146818->data[2]=0;
				mc146818->data[4]=bcd_adjust(mc146818->data[4]+1);
				// different handling of hours
				if (mc146818->data[4]>=0x24)
				{
					mc146818->data[4]=0;
					WEEK_DAY=bcd_adjust(WEEK_DAY+1)%7;
					DAY=bcd_adjust(DAY+1);
					month=bcd_2_dec(MONTH);
					year=bcd_2_dec(YEAR);
					if (mc146818->type!=MC146818_IGNORE_CENTURY) year+=bcd_2_dec(CENTURY)*100;
					else year+=2000; // save for julian_days_in_month calculation
					DAY=bcd_adjust(DAY+1);
					if (DAY>gregorian_days_in_month(MONTH, year))
					{
						DAY=1;
						MONTH=bcd_adjust(MONTH+1);
						if (MONTH>0x12)
						{
							MONTH=1;
							YEAR=year=bcd_adjust(YEAR+1);
							if (mc146818->type!=MC146818_IGNORE_CENTURY)
							{
								if (year>=0x100)
								{
									CENTURY=bcd_adjust(CENTURY+1);
								}
							}
						}
					}
				}
			}
		}
	}
	else
	{
		mc146818->data[0]=mc146818->data[0]+1;
		if (mc146818->data[0]>=60)
		{
			mc146818->data[0]=0;
			mc146818->data[2]=mc146818->data[2]+1;
			if (mc146818->data[2]>=60) {
				mc146818->data[2]=0;
				mc146818->data[4]=mc146818->data[4]+1;
				// different handling of hours //?
				if (mc146818->data[4]>=24) {
					mc146818->data[4]=0;
					WEEK_DAY=(WEEK_DAY+1)%7;
					year=YEAR;
					if (mc146818->type!=MC146818_IGNORE_CENTURY) year+=CENTURY*100;
					else year+=2000; // save for julian_days_in_month calculation
					if (++DAY>gregorian_days_in_month(MONTH, year)) {
						DAY=1;
						if (++MONTH>12) {
							MONTH=1;
							YEAR++;
							if (mc146818->type!=MC146818_IGNORE_CENTURY) {
								if (YEAR>=100) { CENTURY++;YEAR=0; }
							} else {
								YEAR%=100;
							}
                       }
					}
				}
			}
		}
	}

	mc146818->last_refresh = timer_get_time();
}



void mc146818_init(MC146818_TYPE type)
{
	mc146818 = auto_malloc(sizeof(*mc146818));
	memset(mc146818, 0, sizeof(*mc146818));
	mc146818->type = type;
	mc146818->last_refresh = timer_get_time();
    timer_pulse(TIME_IN_HZ(1.0), 0, mc146818_timer);
}



void mc146818_load(void)
{
	mame_file *file;

	file = mame_fopen(Machine->gamedrv->name, 0, FILETYPE_NVRAM, 0);
	if (file)
	{
		mc146818_load_stream(file);
		mame_fclose(file);
	}
}



void mc146818_load_stream(mame_file *file)
{
	mame_fread(file, mc146818->data, sizeof(mc146818->data));
}



void mc146818_set_gmtime(struct tm *tmtime)
{
	int second, minute, day, month, year;

	if (BCD_MODE)
	{
		if (HOURS_24||(tmtime->tm_hour<12))
			mc146818->data[4]=dec_2_bcd(tmtime->tm_hour);
		else
			mc146818->data[4]=dec_2_bcd(tmtime->tm_hour-12)|0x80;

		if (mc146818->type!=MC146818_IGNORE_CENTURY)
			CENTURY=dec_2_bcd((tmtime->tm_year+1900)/100);
	}
	else
	{
		if (HOURS_24||(tmtime->tm_hour<12))
			mc146818->data[4]=tmtime->tm_hour;
		else
			mc146818->data[4]=(tmtime->tm_hour-12)|0x80;

		if (mc146818->type!=MC146818_IGNORE_CENTURY)
			CENTURY=(tmtime->tm_year+1900)/100;
	}

	second	= tmtime->tm_sec;
	minute	= tmtime->tm_min;
	day		= tmtime->tm_mday;
	month	= tmtime->tm_mon + 1;
	year	= tmtime->tm_year % 100;

	mc146818->data[0]	= BCD_MODE ? dec_2_bcd(second)	: second;
	mc146818->data[2]	= BCD_MODE ? dec_2_bcd(minute)	: minute;
	DAY					= BCD_MODE ? dec_2_bcd(day)		: day;
	MONTH				= BCD_MODE ? dec_2_bcd(month)	: month;
	YEAR				= BCD_MODE ? dec_2_bcd(year)	: year;

	WEEK_DAY = tmtime->tm_wday;
	if (tmtime->tm_isdst)
		mc146818->data[0xb] |= 1;
	else
		mc146818->data[0xb] &= ~1;
}



void mc146818_set_time(void)
{
	time_t t;
	struct tm *tmtime;

	t=time(NULL);
	if (t==-1) return;

	tmtime=gmtime(&t);

	mc146818_set_gmtime(tmtime);
	// freeing of gmtime??
}



void mc146818_save(void)
{
	mame_file *file;

	file = mame_fopen(Machine->gamedrv->name, 0, FILETYPE_NVRAM, 1);
	if (file)
	{
		mame_fwrite(file, mc146818->data, sizeof(mc146818->data));
		mame_fclose(file);
	}
}



void mc146818_save_stream(mame_file *file)
{
	mame_fwrite(file, mc146818->data, sizeof(mc146818->data));
}



NVRAM_HANDLER( mc146818 )
{
	if (file==NULL) {
		mc146818_set_time();
		// init only
	} else if (read_or_write) {
		mc146818_save_stream(file);
	} else {
		mc146818_load_stream(file);
	}
}



READ8_HANDLER(mc146818_port_r)
{
	UINT8 data = 0;
	switch (offset) {
	case 0:
		data = mc146818->index;
		break;

	case 1:
		switch (mc146818->index % MC146818_DATA_SIZE) {
		case 0xa:
			data = mc146818->data[mc146818->index  % MC146818_DATA_SIZE];
			if (timer_get_time()-mc146818->last_refresh<TIME_IN_SEC(1.0/32768.0f))
				data |= 0x80;
#if 0
			/* for pc1512 bios realtime clock test */
			mc146818->data[mc146818->index % MC146818_DATA_SIZE] ^= 0x80; /* 0x80 update in progress */
#endif
			break;

		case 0xd:
			/* battery ok */
			data = mc146818->data[mc146818->index % MC146818_DATA_SIZE] | 0x80;
			break;

		default:
			data = mc146818->data[mc146818->index % MC146818_DATA_SIZE];
			break;
		}
		break;
	}

	if (LOG_MC146818)
		logerror("mc146818_port_r(): index=0x%02x data=0x%02x\n", mc146818->index, data);
	return data;
}



WRITE8_HANDLER(mc146818_port_w)
{
	if (LOG_MC146818)
		logerror("mc146818_port_w(): index=0x%02x data=0x%02x\n", mc146818->index, data);

	switch (offset) {
	case 0:
		mc146818->index = data;
		break;

	case 1:
		mc146818->data[mc146818->index % MC146818_DATA_SIZE] = data;
		break;
	}
}



READ32_HANDLER(mc146818_port32le_r)
{
	return read32le_with_read8_handler(mc146818_port_r, offset, mem_mask);
}

WRITE32_HANDLER(mc146818_port32le_w)
{
	write32le_with_write8_handler(mc146818_port_w, offset, data, mem_mask);
}

READ64_HANDLER(mc146818_port64be_r)
{
	return read64be_with_read8_handler(mc146818_port_r, offset, mem_mask);
}

WRITE64_HANDLER(mc146818_port64be_w)
{
	write64be_with_write8_handler(mc146818_port_w, offset, data, mem_mask);
}
