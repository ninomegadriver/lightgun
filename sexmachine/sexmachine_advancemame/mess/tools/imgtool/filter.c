/***************************************************************************

	filter.c

	Imgtool filters

***************************************************************************/

#include <assert.h>
#include <string.h>
#include "imgtool.h"
#include "utils.h"

/* ----------------------------------------------------------------------- */

INT64 filter_get_info_int(filter_getinfoproc get_info, UINT32 state)
{
	union filterinfo info;
	info.i = 0;
	get_info(state, &info);
	return info.i;
}

void *filter_get_info_ptr(filter_getinfoproc get_info, UINT32 state)
{
	union filterinfo info;
	info.p = NULL;
	get_info(state, &info);
	return info.p;
}

genf *filter_get_info_fct(filter_getinfoproc get_info, UINT32 state)
{
	union filterinfo info;
	info.f = NULL;
	get_info(state, &info);
	return info.f;
}

const char *filter_get_info_string(filter_getinfoproc get_info, UINT32 state)
{
	union filterinfo info;
	info.s = NULL;
	get_info(state, &info);
	return info.s;
}

/* ----------------------------------------------------------------------- */

const filter_getinfoproc filters[] =
{
	filter_eoln_getinfo,
	filter_cocobas_getinfo,
	filter_dragonbas_getinfo,
	filter_macbinary_getinfo,
	filter_vzsnapshot_getinfo,
	filter_vzbas_getinfo,
	NULL
};



filter_getinfoproc filter_lookup(const char *name)
{
	int i;
	const char *filter_name;

	for (i = 0; filters[i]; i++)
	{
		filter_name = filter_get_info_string(filters[i], FILTINFO_STR_NAME);
		if (!strcmp(name, filter_name))
			return filters[i];
	}

	return NULL;
}

