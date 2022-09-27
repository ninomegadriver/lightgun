/***************************************************************************

    profiler.c

    Functions to manage profiling of MAME execution.

    Copyright (c) 1996-2006, Nicola Salmoria and the MAME Team.
    Visit http://mamedev.org for licensing and usage restrictions.

***************************************************************************/

#include "osdepend.h"
#include "driver.h"
#include "osinline.h"
#include "profiler.h"


/* in usrintf.c */
static int use_profiler;


/*
 * Versions of GNU C earlier that 2.7 have big problems with the UINT64
 * so we make it into an unsigned long here.
 */

#ifdef __GNUC__
#if (__GNUC__ < 2) || ((__GNUC__ == 2) && (__GNUC_MINOR__ <= 7))
#define UINT64		unsigned long
#endif
#endif

#define MEMORY 6

struct _profile_data
{
	UINT64 count[MEMORY][PROFILER_TOTAL];
	unsigned int cpu_context_switches[MEMORY];
};
typedef struct _profile_data profile_data;

static profile_data profile;
static int memory;


static int FILO_type[10];
static cycles_t FILO_start[10];
static int FILO_length;

void profiler_start(void)
{
	use_profiler = 1;
	FILO_length = 0;
}

void profiler_stop(void)
{
	use_profiler = 0;
}

void profiler_mark(int type)
{
	cycles_t curr_cycles;


	if (!use_profiler)
	{
		FILO_length = 0;
		return;
	}

	if (type >= PROFILER_CPU1 && type <= PROFILER_CPU8)
		profile.cpu_context_switches[memory]++;

	curr_cycles = osd_profiling_ticks();

	if (type != PROFILER_END)
	{
		if (FILO_length > 0)
		{
			if (FILO_length >= 10)
			{
logerror("Profiler error: FILO buffer overflow\n");
				return;
			}

			/* handle nested calls */
			profile.count[memory][FILO_type[FILO_length-1]] += curr_cycles - FILO_start[FILO_length-1];
		}
		FILO_type[FILO_length] = type;
		FILO_start[FILO_length] = curr_cycles;
		FILO_length++;
	}
	else
	{
		if (FILO_length <= 0)
		{
logerror("Profiler error: FILO buffer underflow\n");
			return;
		}

		FILO_length--;
		profile.count[memory][FILO_type[FILO_length]] += curr_cycles - FILO_start[FILO_length];
		if (FILO_length > 0)
		{
			/* handle nested calls */
			FILO_start[FILO_length-1] = curr_cycles;
		}
	}
}

const char *profiler_get_text(void)
{
	int i,j;
	UINT64 total,normalize;
	UINT64 computed;
	static const char *names[PROFILER_TOTAL] =
	{
		"CPU 1  ",
		"CPU 2  ",
		"CPU 3  ",
		"CPU 4  ",
		"CPU 5  ",
		"CPU 6  ",
		"CPU 7  ",
		"CPU 8  ",
		"Mem rd ",
		"Mem wr ",
		"Video  ",
		"drawgfx",
		"copybmp",
		"tmdraw ",
		"tmdrroz",
		"tmupdat",
		"Artwork",
		"Blit   ",
		"Sound  ",
		"Mixer  ",
		"Callbck",
		"Hiscore",
		"Input  ",
		"Movie  ",
		"Logerr ",
		"Extra  ",
		"User1  ",
		"User2  ",
		"User3  ",
		"User4  ",
		"Profilr",
		"Idle   ",
	};
	static int showdelay[PROFILER_TOTAL];
	static char buf[30*20];
	char *bufptr = buf;


	if (!use_profiler) return "";

	profiler_mark(PROFILER_PROFILER);

	computed = 0;
	i = 0;
	while (i < PROFILER_PROFILER)
	{
		for (j = 0;j < MEMORY;j++)
			computed += profile.count[j][i];
		i++;
	}
	normalize = computed;
	while (i < PROFILER_TOTAL)
	{
		for (j = 0;j < MEMORY;j++)
			computed += profile.count[j][i];
		i++;
	}
	total = computed;

	if (total == 0 || normalize == 0) return "";	/* we have been just reset */

	for (i = 0;i < PROFILER_TOTAL;i++)
	{
		computed = 0;
		{
			for (j = 0;j < MEMORY;j++)
				computed += profile.count[j][i];
		}
		if (computed || showdelay[i])
		{
			if (computed) showdelay[i] = Machine->refresh_rate;
			showdelay[i]--;

			if (i < PROFILER_PROFILER)
				bufptr += sprintf(bufptr,"%s%3d%%%3d%%\n",names[i],
						(int)((computed * 100 + total/2) / total),
						(int)((computed * 100 + normalize/2) / normalize));
			else
				bufptr += sprintf(bufptr,"%s%3d%%\n",names[i],
						(int)((computed * 100 + total/2) / total));
		}
	}

	i = 0;
	for (j = 0;j < MEMORY;j++)
		i += profile.cpu_context_switches[j];
	bufptr += sprintf(bufptr,"CPU switches%4d\n",i / MEMORY);

	/* reset the counters */
	memory = (memory + 1) % MEMORY;
	profile.cpu_context_switches[memory] = 0;
	for (i = 0;i < PROFILER_TOTAL;i++)
		profile.count[memory][i] = 0;

	profiler_mark(PROFILER_END);

	return buf;
}
