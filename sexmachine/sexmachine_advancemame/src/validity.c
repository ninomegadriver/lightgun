/***************************************************************************

    validity.c

    Validity checks on internal data structures.

    Copyright (c) 1996-2006, Nicola Salmoria and the MAME Team.
    Visit http://mamedev.org for licensing and usage restrictions.

***************************************************************************/

#include "osdepend.h"
#include "driver.h"
#include "hash.h"
#include <ctype.h>
#include <stdarg.h>
#include "ui_text.h"
#include <zlib.h>


/*************************************
 *
 *  Debugging
 *
 *************************************/

#define REPORT_TIMES		(0)



/*************************************
 *
 *  Constants
 *
 *************************************/

#define QUARK_HASH_SIZE		389



/*************************************
 *
 *  Type definitions
 *
 *************************************/

typedef struct _quark_entry quark_entry;
struct _quark_entry
{
	UINT32 crc;
	struct _quark_entry *next;
};


typedef struct _quark_table quark_table;
struct _quark_table
{
	UINT32 entries;
	UINT32 hashsize;
	quark_entry *entry;
	quark_entry **hash;
};



/*************************************
 *
 *  Local variables
 *
 *************************************/

static quark_table *source_table;
static quark_table *name_table;
static quark_table *description_table;
static quark_table *roms_table;
static quark_table *inputs_table;
static quark_table *defstr_table;
static int total_drivers;



/*************************************
 *
 *  Allocate an array of quark
 *  entries and a hash table
 *
 *************************************/

static quark_table *allocate_quark_table(UINT32 entries, UINT32 hashsize)
{
	quark_table *table;
	UINT32 total_bytes;

	/* determine how many total bytes we need */
	total_bytes = sizeof(*table) + entries * sizeof(table->entry[0]) + hashsize * sizeof(table->hash[0]);
	table = auto_malloc(total_bytes);

	/* fill in the details */
	table->entries = entries;
	table->hashsize = hashsize;

	/* compute the pointers */
	table->entry = (quark_entry *)((UINT8 *)table + sizeof(*table));
	table->hash = (quark_entry **)((UINT8 *)table->entry + entries * sizeof(table->entry[0]));

	/* reset the hash table */
	memset(table->hash, 0, hashsize * sizeof(table->hash[0]));
	return table;
}



/*************************************
 *
 *  Compute the CRC of a string
 *
 *************************************/

INLINE UINT32 quark_string_crc(const char *string)
{
	return crc32(0, (UINT8 *)string, strlen(string));
}



/*************************************
 *
 *  Add a quark to the table and
 *  connect it to the hash tables
 *
 *************************************/

INLINE void add_quark(quark_table *table, int index, UINT32 crc)
{
	quark_entry *entry = &table->entry[index];
	int hash = crc % table->hashsize;
	entry->crc = crc;
	entry->next = table->hash[hash];
	table->hash[hash] = entry;
}



/*************************************
 *
 *  Return a pointer to the first
 *  hash entry connected to a CRC
 *
 *************************************/

INLINE quark_entry *first_hash_entry(quark_table *table, UINT32 crc)
{
	return table->hash[crc % table->hashsize];
}



/*************************************
 *
 *  Build "quarks" for fast string
 *  operations
 *
 *************************************/

static void build_quarks(void)
{
	int drivnum, strnum;

	/* first count drivers */
	for (drivnum = 0; drivers[drivnum]; drivnum++)
		;

	/* allocate memory for the quark tables */
	source_table = allocate_quark_table(drivnum, QUARK_HASH_SIZE);
	name_table = allocate_quark_table(drivnum, QUARK_HASH_SIZE);
	description_table = allocate_quark_table(drivnum, QUARK_HASH_SIZE);
	roms_table = allocate_quark_table(drivnum, QUARK_HASH_SIZE);
	inputs_table = allocate_quark_table(drivnum, QUARK_HASH_SIZE);

	/* build the quarks and the hash tables */
	for (drivnum = 0; drivers[drivnum]; drivnum++)
	{
		const game_driver *driver = drivers[drivnum];

		/* fill in the quark with hashes of the strings */
		add_quark(source_table,      drivnum, quark_string_crc(driver->source_file));
		add_quark(name_table,        drivnum, quark_string_crc(driver->name));
		add_quark(description_table, drivnum, quark_string_crc(driver->description));

		/* only track actual driver ROM entries */
		if (driver->rom && (driver->flags & NOT_A_DRIVER) == 0)
			add_quark(roms_table,    drivnum, (UINT32)driver->rom);
	}

	/* allocate memory for a quark table of strings */
	defstr_table = allocate_quark_table(STR_TOTAL, 97);

	/* add all the default strings */
	for (strnum = 0; strnum < STR_TOTAL; strnum++)
		add_quark(defstr_table, strnum, quark_string_crc(input_port_default_strings[strnum]));
}



/*************************************
 *
 *  Validate basic driver info
 *
 *************************************/

static int validate_driver(int drivnum, const machine_config *drv)
{
	const game_driver *driver = drivers[drivnum];
	const game_driver *clone_of;
	quark_entry *entry;
	int error = FALSE;
	const char *s;
	UINT32 crc;

	/* determine the clone */
	clone_of = driver_get_clone(driver);

	/* if we have at least 100 drivers, validate the clone */
	/* (100 is arbitrary, but tries to avoid tiny.mak dependencies) */
	if (total_drivers > 100 && !clone_of && strcmp(driver->parent, "0"))
	{
		printf("%s: %s is a non-existant clone\n", driver->source_file, driver->parent);
		error = TRUE;
	}

	/* look for recursive cloning */
	if (clone_of == driver)
	{
		printf("%s: %s is set as a clone of itself\n", driver->source_file, driver->name);
		error = TRUE;
	}

	/* look for clones that are too deep */
	if (clone_of != NULL && (clone_of = driver_get_clone(clone_of)) != NULL && (clone_of->flags & NOT_A_DRIVER) == 0)
	{
		printf("%s: %s is a clone of a clone\n", driver->source_file, driver->name);
		error = TRUE;
	}

	/* make sure the driver name is 8 chars or less */
	if (strlen(driver->name) > 8)
	{
		printf("%s: %s driver name must be 8 characters or less\n", driver->source_file, driver->name);
		error = TRUE;
	}

	/* make sure the year is only digits, '?' or '+' */
	for (s = driver->year; *s; s++)
		if (!isdigit(*s) && *s != '?' && *s != '+')
		{
			printf("%s: %s has an invalid year '%s'\n", driver->source_file, driver->name, driver->year);
			error = TRUE;
			break;
		}

#ifndef MESS
	/* make sure sound-less drivers are flagged */
	if ((driver->flags & NOT_A_DRIVER) == 0 && drv->sound[0].sound_type == 0 && (driver->flags & GAME_NO_SOUND) == 0 && strcmp(driver->name, "minivadr"))
	{
		printf("%s: %s missing GAME_NO_SOUND flag\n", driver->source_file, driver->name);
		error = TRUE;
	}
#endif

	/* find duplicate driver names */
	crc = quark_string_crc(driver->name);
	for (entry = first_hash_entry(name_table, crc); entry; entry = entry->next)
		if (entry->crc == crc && entry != &name_table->entry[drivnum])
		{
			const game_driver *match = drivers[entry - name_table->entry];
			if (!strcmp(match->name, driver->name))
			{
				printf("%s: %s is a duplicate name (%s, %s)\n", driver->source_file, driver->name, match->source_file, match->name);
				error = TRUE;
			}
		}

	/* find duplicate descriptions */
	crc = quark_string_crc(driver->description);
	for (entry = first_hash_entry(description_table, crc); entry; entry = entry->next)
		if (entry->crc == crc && entry != &description_table->entry[drivnum])
		{
			const game_driver *match = drivers[entry - description_table->entry];
			if (!strcmp(match->description, driver->description))
			{
				printf("%s: %s is a duplicate description (%s, %s)\n", driver->source_file, driver->description, match->source_file, match->name);
				error = TRUE;
			}
		}

	/* find shared ROM entries */
#ifndef MESS
	if (driver->rom && (driver->flags & NOT_A_DRIVER) == 0)
	{
		crc = (UINT32)driver->rom;
		for (entry = first_hash_entry(roms_table, crc); entry; entry = entry->next)
			if (entry->crc == crc && entry != &roms_table->entry[drivnum])
			{
				const game_driver *match = drivers[entry - roms_table->entry];
				if (match->rom == driver->rom)
				{
					printf("%s: %s uses the same ROM set as (%s, %s)\n", driver->source_file, driver->description, match->source_file, match->name);
					error = TRUE;
				}
			}
	}
#endif /* MESS */

	return error;
}



/*************************************
 *
 *  Validate ROM definitions
 *
 *************************************/

static int validate_roms(int drivnum, const machine_config *drv, UINT32 *region_length)
{
	const game_driver *driver = drivers[drivnum];
	const rom_entry *romp;
	const char *last_name = "???";
	int cur_region = -1;
	int error = FALSE;
	int items_since_region = 1;

	/* reset region info */
	memset(region_length, 0, REGION_MAX * sizeof(*region_length));

	/* scan the ROM entries */
	for (romp = driver->rom; romp && !ROMENTRY_ISEND(romp); romp++)
	{
		/* if this is a region, make sure it's valid, and record the length */
		if (ROMENTRY_ISREGION(romp))
		{
			int type = ROMREGION_GETTYPE(romp);

			/* if we haven't seen any items since the last region, print a warning */
			if (items_since_region == 0)
				printf("%s: %s has empty ROM region (warning)\n", driver->source_file, driver->name);
			items_since_region = (ROMREGION_ISERASE(romp) || ROMREGION_ISDISPOSE(romp)) ? 1 : 0;

			/* check for an invalid region */
			if (type >= REGION_MAX || type <= REGION_INVALID)
			{
				printf("%s: %s has invalid ROM_REGION type %x\n", driver->source_file, driver->name, type);
				error = TRUE;
				cur_region = -1;
			}

			/* check for a duplicate */
			else if (region_length[type] != 0)
			{
				printf("%s: %s has duplicate ROM_REGION type %x\n", driver->source_file, driver->name, type);
				error = TRUE;
				cur_region = -1;
			}

			/* if all looks good, remember the length and note the region */
			else
			{
				cur_region = type;
				region_length[type] = ROMREGION_GETLENGTH(romp);
			}
		}

		/* if this is a file, make sure it is properly formatted */
		else if (ROMENTRY_ISFILE(romp))
		{
			const char *hash;
			const char *s;

			items_since_region++;

			/* track the last filename we found */
			last_name = ROM_GETNAME(romp);

			/* make sure it's all lowercase */
			for (s = last_name; *s; s++)
				if (tolower(*s) != *s)
				{
					printf("%s: %s has upper case ROM name %s\n", driver->source_file, driver->name, last_name);
					error = TRUE;
					break;
				}

			/* make sure the has is valid */
			hash = ROM_GETHASHDATA(romp);
			if (!hash_verify_string(hash))
			{
				printf("%s: rom '%s' has an invalid hash string '%s'\n", driver->name, last_name, hash);
				error = TRUE;
			}
		}

		/* for any non-region ending entries, make sure they don't extend past the end */
		if (!ROMENTRY_ISREGIONEND(romp) && cur_region != -1)
		{
			items_since_region++;

			if (ROM_GETOFFSET(romp) + ROM_GETLENGTH(romp) > region_length[cur_region])
			{
				printf("%s: %s has ROM %s extending past the defined memory region\n", driver->source_file, driver->name, last_name);
				error = TRUE;
			}
		}
	}

	/* final check for empty regions */
	if (items_since_region == 0)
		printf("%s: %s has empty ROM region (warning)\n", driver->source_file, driver->name);

	return error;
}



/*************************************
 *
 *  Validate CPUs and memory maps
 *
 *************************************/

static int validate_cpu(int drivnum, const machine_config *drv, const UINT32 *region_length)
{
	const game_driver *driver = drivers[drivnum];
	int error = FALSE;
	int cpunum;

	/* loop over all the CPUs */
	for (cpunum = 0; cpunum < MAX_CPU; cpunum++)
	{
		extern void dummy_get_info(UINT32 state, union cpuinfo *info);
		const cpu_config *cpu = &drv->cpu[cpunum];
		int spacenum;

		/* skip empty entries */
		if (cpu->cpu_type == 0)
			continue;

		/* checks to see if this driver is using a dummy CPU */
		if (cputype_get_interface(cpu->cpu_type)->get_info == dummy_get_info)
		{
			printf("%s: %s uses non-present CPU\n", driver->source_file, driver->name);
			error = TRUE;
			continue;
		}

		/* check the CPU for incompleteness */
		if (!cputype_get_info_fct(cpu->cpu_type, CPUINFO_PTR_GET_CONTEXT)
			|| !cputype_get_info_fct(cpu->cpu_type, CPUINFO_PTR_SET_CONTEXT)
			|| !cputype_get_info_fct(cpu->cpu_type, CPUINFO_PTR_RESET)
			|| !cputype_get_info_fct(cpu->cpu_type, CPUINFO_PTR_EXECUTE))
		{
			printf("%s: %s uses an incomplete CPU\n", driver->source_file, driver->name);
			error = TRUE;
			continue;
		}

		/* loop over all address spaces */
		for (spacenum = 0; spacenum < ADDRESS_SPACES; spacenum++)
		{
#define SPACE_SHIFT(a)		((addr_shift < 0) ? ((a) << -addr_shift) : ((a) >> addr_shift))
#define SPACE_SHIFT_END(a)	((addr_shift < 0) ? (((a) << -addr_shift) | ((1 << -addr_shift) - 1)) : ((a) >> addr_shift))
			static const char *spacename[] = { "program", "data", "I/O" };
			int databus_width = cputype_databus_width(cpu->cpu_type, spacenum);
			int addr_shift = cputype_addrbus_shift(cpu->cpu_type, spacenum);
			int alignunit = databus_width/8;
			address_map addrmap[MAX_ADDRESS_MAP_SIZE*2];
			address_map *map;
			UINT32 flags;

			/* if no map, skip */
			if (!cpu->construct_map[spacenum][0] && !cpu->construct_map[spacenum][1])
				continue;

			/* check to see that the same map is not used twice */
			if (cpu->construct_map[spacenum][0] && cpu->construct_map[spacenum][0] != construct_map_0 && cpu->construct_map[spacenum][0] == cpu->construct_map[spacenum][1])
			{
				printf("%s: %s uses identical memory maps for CPU #%d spacenum %d\n", driver->source_file, driver->name, cpunum, spacenum);
				error = TRUE;
			}

			/* reset the address map, resetting the base address to a non-NULL value */
			/* because the AM_REGION macro will query non-existant memory regions and */
			/* product valid NULL results */
			memset(addrmap, 0, sizeof(addrmap));

			/* construct the maps */
			map = addrmap;
			if (cpu->construct_map[spacenum][0])
				map = (*cpu->construct_map[spacenum][0])(map);
			if (cpu->construct_map[spacenum][1])
				map = (*cpu->construct_map[spacenum][1])(map);

			/* if this is an empty map, just skip it */
			map = addrmap;
			if (IS_AMENTRY_END(map))
				continue;

			/* make sure we start with a proper entry */
			if (!IS_AMENTRY_EXTENDED(map))
			{
				printf("%s: %s wrong MEMORY_READ_START for %s space\n", driver->source_file, driver->name, spacename[spacenum]);
				error = TRUE;
			}

			/* verify the type of memory handlers */
			flags = AM_EXTENDED_FLAGS(map);
			if (flags & AMEF_SPECIFIES_DBITS)
			{
				int val = (flags & AMEF_DBITS_MASK) >> AMEF_DBITS_SHIFT;
				val = (val + 1) * 8;
				if (val != databus_width)
				{
					printf("%s: %s cpu #%d uses wrong memory handlers for %s space! (width = %d, memory = %08x)\n", driver->source_file, driver->name, cpunum, spacename[spacenum], databus_width, AM_EXTENDED_FLAGS(map));
					error = TRUE;
				}
			}

			/* loop over entries and look for errors */
			for ( ; !IS_AMENTRY_END(map); map++)
				if (!IS_AMENTRY_EXTENDED(map))
				{
					/* the following checks only work for standard start/end ranges */
					if (!IS_AMENTRY_MATCH_MASK(map))
					{
						/* look for inverted start/end pairs */
						if (map->end < map->start)
						{
							printf("%s: %s wrong %s memory read handler start = %08x > end = %08x\n", driver->source_file, driver->name, spacename[spacenum], map->start, map->end);
							error = TRUE;
						}

						/* look for misaligned entries */
						if ((SPACE_SHIFT(map->start) & (alignunit-1)) != 0 || (SPACE_SHIFT_END(map->end) & (alignunit-1)) != (alignunit-1))
						{
							printf("%s: %s wrong %s memory read handler start = %08x, end = %08x ALIGN = %d\n", driver->source_file, driver->name, spacename[spacenum], map->start, map->end, alignunit);
							error = TRUE;
						}

						/* if this is a program space, auto-assign implicit ROM entries */
						if ((FPTR)map->read.handler == STATIC_ROM && !map->region)
						{
							map->region = REGION_CPU1 + cpunum;
							map->region_offs = map->start;
						}

						/* if this entry references a memory region, validate it */
						if (map->region)
							if (region_length[map->region] && map->region_offs + (SPACE_SHIFT_END(map->end) - SPACE_SHIFT(map->start) + 1) > region_length[map->region] && map->share == 0 && !map->base)
							{
								if (region_length[map->region] == 0)
									printf("%s: %s CPU %d space %d memory map entry %X-%X references non-existant region %d\n", driver->source_file, driver->name, cpunum, spacenum, map->start, map->end, map->region);
								else
									printf("%s: %s CPU %d space %d memory map entry %X-%X extends beyond region %d size (%X)\n", driver->source_file, driver->name, cpunum, spacenum, map->start, map->end, map->region, region_length[map->region]);
								error = TRUE;
							}
					}
				}
		}
	}

	return error;
}



/*************************************
 *
 *  Validate display
 *
 *************************************/

static int validate_display(int drivnum, const machine_config *drv)
{
	const game_driver *driver = drivers[drivnum];
	int error = FALSE;

	/* check for empty palette */
	if (drv->total_colors == 0 && !(drv->video_attributes & VIDEO_RGB_DIRECT))
	{
		printf("%s: %s has zero palette entries\n", driver->source_file, driver->name);
		error = TRUE;
	}

	/* sanity check dimensions */
	if ((drv->screen_width <= 0) || (drv->screen_height <= 0))
	{
		printf("%s: %s has invalid display dimensions\n", driver->source_file, driver->name);
		error = TRUE;
	}

	/* sanity check display area */
	if (!(drv->video_attributes & VIDEO_TYPE_VECTOR))
	{
		if ((drv->default_visible_area.max_x < drv->default_visible_area.min_x)
			|| (drv->default_visible_area.max_y < drv->default_visible_area.min_y)
			|| (drv->default_visible_area.max_x >= drv->screen_width)
			|| (drv->default_visible_area.max_y >= drv->screen_height))
		{
			printf("%s: %s has an invalid display area\n", driver->source_file, driver->name);
			error = TRUE;
		}
	}

	return error;
}



/*************************************
 *
 *  Validate graphics
 *
 *************************************/

static int validate_gfx(int drivnum, const machine_config *drv, const UINT32 *region_length)
{
	const game_driver *driver = drivers[drivnum];
	int error = FALSE;
	int gfxnum;

	/* bail if no gfx */
	if (!drv->gfxdecodeinfo)
		return FALSE;

	/* iterate over graphics decoding entries */
	for (gfxnum = 0; gfxnum < MAX_GFX_ELEMENTS && drv->gfxdecodeinfo[gfxnum].memory_region != -1; gfxnum++)
	{
		const gfx_decode *gfx = &drv->gfxdecodeinfo[gfxnum];
		int region = gfx->memory_region;

		/* if we have a valid region, and we're not using auto-sizing, check the decode against the region length */
		if (region && !IS_FRAC(gfx->gfxlayout->total))
		{
			int len, avail, plane, start;

			/* determine which plane is the largest */
 			start = 0;
			for (plane = 0; plane < MAX_GFX_PLANES; plane++)
				if (gfx->gfxlayout->planeoffset[plane] > start)
					start = gfx->gfxlayout->planeoffset[plane];
			start &= ~(gfx->gfxlayout->charincrement - 1);

			/* determine the total length based on this info */
			len = gfx->gfxlayout->total * gfx->gfxlayout->charincrement;

			/* do we have enough space in the region to cover the whole decode? */
			avail = region_length[region] - (gfx->start & ~(gfx->gfxlayout->charincrement/8-1));

			/* if not, this is an error */
			if ((start + len) / 8 > avail)
			{
				printf("%s: %s has gfx[%d] extending past allocated memory\n", driver->source_file, driver->name, gfxnum);
				error = TRUE;
			}
		}
	}

	return error;
}



/*************************************
 *
 *  Validate input ports
 *
 *************************************/

static int validate_inputs(int drivnum, const machine_config *drv, input_port_entry **memory)
{
	const input_port_entry *inp, *last_dipname_entry = NULL;
	const game_driver *driver = drivers[drivnum];
	int empty_string_found = FALSE;
	int last_strindex = -1;
	quark_entry *entry;
	int error = FALSE;
	UINT32 crc;

	/* skip if no ports */
	if (!driver->construct_ipt)
		return FALSE;

	/* skip if we already validated these ports */
	crc = (UINT32)driver->construct_ipt;
	for (entry = first_hash_entry(inputs_table, crc); entry; entry = entry->next)
		if (entry->crc == crc && driver->construct_ipt == drivers[entry - inputs_table->entry]->construct_ipt)
			return FALSE;

	/* otherwise, add ourself to the list */
	add_quark(inputs_table, drivnum, crc);

	/* allocate the input ports */
	*memory = input_port_allocate(driver->construct_ipt, *memory);

	/* iterate over the results */
	for (inp = *memory; inp->type != IPT_END; inp++)
	{
		quark_entry *entry;
		int strindex = -1;
		UINT32 crc;

		/* clear the DIP switch tracking when we hit the first non-DIP entry */
		if (last_dipname_entry && inp->type != IPT_DIPSWITCH_SETTING)
			last_dipname_entry = NULL;

		/* look for invalid (0) types which should be mapped to IPT_OTHER */
		if (inp->type == IPT_INVALID)
		{
			printf("%s: %s has an input port with an invalid type (0); use IPT_OTHER instead\n", driver->source_file, driver->name);
			error = TRUE;
		}

		/* if this entry doesn't have a name, we don't care about the rest of this stuff */
		if (!inp->name || inp->name == IP_NAME_DEFAULT)
		{
			/* not allowed for dipswitches */
			if (inp->type == IPT_DIPSWITCH_NAME || inp->type == IPT_DIPSWITCH_SETTING)
			{
				printf("%s: %s has a DIP switch name or setting with no name\n", driver->source_file, driver->name);
				error = TRUE;
			}
			last_strindex = -1;
			continue;
		}

		/* check for empty string */
		if (!inp->name[0] && !empty_string_found)
		{
			printf("%s: %s has an input with an empty string\n", driver->source_file, driver->name);
			error = TRUE;
			empty_string_found = TRUE;
		}

		/* check for trailing spaces */
		if (inp->name[0] && inp->name[strlen(inp->name) - 1] == ' ')
		{
			printf("%s: %s input '%s' has trailing spaces\n", driver->source_file, driver->name, inp->name);
			error = TRUE;
		}

		/* hash the string and look it up in the string table */
		crc = quark_string_crc(inp->name);
		for (entry = first_hash_entry(defstr_table, crc); entry; entry = entry->next)
			if (entry->crc == crc && !strcmp(inp->name, input_port_default_strings[entry - defstr_table->entry]))
			{
				strindex = entry - defstr_table->entry;
				break;
			}

		/* check for strings that should be DEF_STR */
		if (strindex != -1 && inp->name != input_port_default_strings[strindex])
		{
			printf("%s: %s must use DEF_STR( %s )\n", driver->source_file, driver->name, inp->name);
			error = TRUE;
		}

		/* track the last dipswitch we encountered */
		if (inp->type == IPT_DIPSWITCH_NAME)
			last_dipname_entry = inp;

		/* check for dipswitch ordering against the last entry */
		if (inp[0].type == IPT_DIPSWITCH_SETTING && inp[-1].type == IPT_DIPSWITCH_SETTING && last_strindex != -1 && strindex != -1)
		{
			/* check for inverted off/on dispswitch order */
			if (last_strindex == STR_On && strindex == STR_Off)
			{
				printf("%s: %s has inverted Off/On dipswitch order\n", driver->source_file, driver->name);
				error = TRUE;
			}

			/* check for inverted yes/no dispswitch order */
			else if (last_strindex == STR_Yes && strindex == STR_No)
			{
				printf("%s: %s has inverted No/Yes dipswitch order\n", driver->source_file, driver->name);
				error = TRUE;
			}

			/* check for inverted upright/cocktail dispswitch order */
			else if (last_strindex == STR_Cocktail && strindex == STR_Upright)
			{
				printf("%s: %s has inverted Upright/Cocktail dipswitch order\n", driver->source_file, driver->name);
				error = TRUE;
			}

			/* check for proper coin ordering */
			else if (last_strindex >= STR_9C_1C && last_strindex <= STR_1C_9C && strindex >= STR_9C_1C && strindex <= STR_1C_9C &&
					 last_strindex >= strindex && !memcmp(&inp[-1].condition, &inp[0].condition, sizeof(inp[-1].condition)))
			{
				printf("%s: %s has unsorted coinage %s > %s\n", driver->source_file, driver->name, inp[-1].name, inp[0].name);
				error = TRUE;
			}
		}

		/* check for invalid DIP switch entries */
		if (last_dipname_entry && inp->type == IPT_DIPSWITCH_SETTING)
		{
			/* make sure cabinet selections default to upright */
			if (last_dipname_entry->name == DEF_STR( Cabinet ) && strindex == STR_Upright &&
				last_dipname_entry->default_value != inp->default_value)
			{
				printf("%s: %s Cabinet must default to Upright\n", driver->source_file, driver->name);
				error = TRUE;
			}

			/* make sure demo sounds default to on */
			if (last_dipname_entry->name == DEF_STR( Demo_Sounds ) && strindex == STR_On &&
				last_dipname_entry->default_value != inp->default_value)
			{
				printf("%s: %s Demo Sounds must default to On\n", driver->source_file, driver->name);
				error = TRUE;
			}

			/* check for bad flip screen options */
			if (last_dipname_entry->name == DEF_STR( Flip_Screen ) && (strindex == STR_Yes || strindex == STR_No))
			{
				printf("%s: %s has wrong Flip Screen option %s (must be Off/On)\n", driver->source_file, driver->name, inp->name);
				error = TRUE;
			}

			/* check for bad demo sounds options */
			if (last_dipname_entry->name == DEF_STR( Demo_Sounds ) && (strindex == STR_Yes || strindex == STR_No))
			{
				printf("%s: %s has wrong Demo Sounds option %s (must be Off/On)\n", driver->source_file, driver->name, inp->name);
				error = TRUE;
			}
		}

		/* analog ports must have a valid sensitivity */
		if (port_type_is_analog(inp->type) && inp->analog.sensitivity == 0)
		{
			printf("%s: %s has an analog port with zero sensitivity\n", driver->source_file, driver->name);
			error = TRUE;
		}

		/* remember the last string index */
		last_strindex = strindex;
	}

	return error;
}



/*************************************
 *
 *  Validate sound and speakers
 *
 *************************************/

static int validate_sound(int drivnum, const machine_config *drv)
{
	const game_driver *driver = drivers[drivnum];
	int speaknum, sndnum;
	int error = FALSE;

	/* make sure the speaker layout makes sense */
	for (speaknum = 0; speaknum < MAX_SPEAKER && drv->speaker[speaknum].tag; speaknum++)
	{
		int check;

		/* check for duplicate tags */
		for (check = 0; check < MAX_SPEAKER && drv->speaker[check].tag; check++)
			if (speaknum != check && drv->speaker[check].tag && !strcmp(drv->speaker[speaknum].tag, drv->speaker[check].tag))
			{
				printf("%s: %s has multiple speakers tagged as '%s'\n", driver->source_file, driver->name, drv->speaker[speaknum].tag);
				error = TRUE;
			}

		/* make sure there are no sound chips with the same tag */
		for (check = 0; check < MAX_SOUND && drv->sound[check].sound_type != SOUND_DUMMY; check++)
			if (drv->sound[check].tag && !strcmp(drv->speaker[speaknum].tag, drv->sound[check].tag))
			{
				printf("%s: %s has both a speaker and a sound chip tagged as '%s'\n", driver->source_file, driver->name, drv->speaker[speaknum].tag);
				error = TRUE;
			}
	}

	/* make sure the sounds are wired to the speakers correctly */
	for (sndnum = 0; sndnum < MAX_SOUND && drv->sound[sndnum].sound_type != SOUND_DUMMY; sndnum++)
	{
		int routenum;

		/* loop over all the routes */
		for (routenum = 0; routenum < drv->sound[sndnum].routes; routenum++)
		{
			/* find a speaker with the requested tag */
			for (speaknum = 0; speaknum < MAX_SPEAKER && drv->speaker[speaknum].tag; speaknum++)
				if (!strcmp(drv->sound[sndnum].route[routenum].target, drv->speaker[speaknum].tag))
					break;

			/* if we didn't find one, look for another sound chip with the tag */
			if (speaknum >= MAX_SPEAKER || !drv->speaker[speaknum].tag)
			{
				int check;

				for (check = 0; check < MAX_SOUND && drv->sound[check].sound_type != SOUND_DUMMY; check++)
					if (check != sndnum && drv->sound[check].tag && !strcmp(drv->sound[check].tag, drv->sound[sndnum].route[routenum].target))
						break;

				/* if we didn't find one, it's an error */
				if (check >= MAX_SOUND || drv->sound[check].sound_type == SOUND_DUMMY)
				{
					printf("%s: %s attempting to route sound to non-existant speaker '%s'\n", driver->source_file, driver->name, drv->sound[sndnum].route[routenum].target);
					error = TRUE;
				}
			}
		}
	}

	return error;
}



/*************************************
 *
 *  Master validity checker
 *
 *************************************/

int mame_validitychecks(int game)
{
	cycles_t prep = 0;
	cycles_t expansion = 0;
	cycles_t driver_checks = 0;
	cycles_t rom_checks = 0;
	cycles_t cpu_checks = 0;
	cycles_t gfx_checks = 0;
	cycles_t display_checks = 0;
	cycles_t input_checks = 0;
	cycles_t sound_checks = 0;
#ifdef MESS
	cycles_t mess_checks = 0;
#endif

	input_port_entry *inputports = NULL;
	int drivnum;
	int error = FALSE;
	UINT8 a, b;

	/* basic system checks */
	a = 0xff;
	b = a + 1;
	if (b > a)	{ printf("UINT8 must be 8 bits\n"); error = TRUE; }

	if (sizeof(INT8)   != 1)	{ printf("INT8 must be 8 bits\n"); error = TRUE; }
	if (sizeof(UINT8)  != 1)	{ printf("UINT8 must be 8 bits\n"); error = TRUE; }
	if (sizeof(INT16)  != 2)	{ printf("INT16 must be 16 bits\n"); error = TRUE; }
	if (sizeof(UINT16) != 2)	{ printf("UINT16 must be 16 bits\n"); error = TRUE; }
	if (sizeof(INT32)  != 4)	{ printf("INT32 must be 32 bits\n"); error = TRUE; }
	if (sizeof(UINT32) != 4)	{ printf("UINT32 must be 32 bits\n"); error = TRUE; }
	if (sizeof(INT64)  != 8)	{ printf("INT64 must be 64 bits\n"); error = TRUE; }
	if (sizeof(UINT64) != 8)	{ printf("UINT64 must be 64 bits\n"); error = TRUE; }

	begin_resource_tracking();
	osd_profiling_ticks();

	/* prepare by pre-scanning all the drivers and adding their info to hash tables */
	prep -= osd_profiling_ticks();
	build_quarks();
	prep += osd_profiling_ticks();

	/* count drivers first */
	for (drivnum = 0; drivers[drivnum]; drivnum++) ;
	total_drivers = drivnum;

	/* iterate over all drivers */
	for (drivnum = 0; drivers[drivnum]; drivnum++)
	{
		const game_driver *driver = drivers[drivnum];
		UINT32 region_length[REGION_MAX];
		machine_config drv;

/* ASG -- trying this for a while to see if submission failures increase */
#if 1
		/* non-debug builds only care about games in the same driver */
		if (game != -1 && strcmp(drivers[game]->source_file, driver->source_file) != 0)
			continue;
#endif

		/* expand the machine driver */
		expansion -= osd_profiling_ticks();
		expand_machine_driver(driver->drv, &drv);
		expansion += osd_profiling_ticks();

		/* validate the driver entry */
		driver_checks -= osd_profiling_ticks();
		error = validate_driver(drivnum, &drv) || error;
		driver_checks += osd_profiling_ticks();

		/* validate the ROM information */
		rom_checks -= osd_profiling_ticks();
		error = validate_roms(drivnum, &drv, region_length) || error;
		rom_checks += osd_profiling_ticks();

		/* validate the CPU information */
		cpu_checks -= osd_profiling_ticks();
		error = validate_cpu(drivnum, &drv, region_length) || error;
		cpu_checks += osd_profiling_ticks();

		/* validate the display */
		display_checks -= osd_profiling_ticks();
		error = validate_display(drivnum, &drv) || error;
		display_checks += osd_profiling_ticks();

		/* validate the graphics decoding */
		gfx_checks -= osd_profiling_ticks();
		error = validate_gfx(drivnum, &drv, region_length) || error;
		gfx_checks += osd_profiling_ticks();

		/* validate input ports */
		input_checks -= osd_profiling_ticks();
		error = validate_inputs(drivnum, &drv, &inputports) || error;
		input_checks += osd_profiling_ticks();

		/* validate sounds and speakers */
		sound_checks -= osd_profiling_ticks();
		error = validate_sound(drivnum, &drv) || error;
		sound_checks += osd_profiling_ticks();
	}

#ifdef MESS
	mess_checks -= osd_profiling_ticks();
	if (mess_validitychecks())
		error = TRUE;
	mess_checks += osd_profiling_ticks();
#endif /* MESS */

#if (REPORT_TIMES)
	printf("Prep:      %8dm\n", (int)(prep / 1000000));
	printf("Expansion: %8dm\n", (int)(expansion / 1000000));
	printf("Driver:    %8dm\n", (int)(driver_checks / 1000000));
	printf("ROM:       %8dm\n", (int)(rom_checks / 1000000));
	printf("CPU:       %8dm\n", (int)(cpu_checks / 1000000));
	printf("Display:   %8dm\n", (int)(display_checks / 1000000));
	printf("Graphics:  %8dm\n", (int)(gfx_checks / 1000000));
	printf("Input:     %8dm\n", (int)(input_checks / 1000000));
	printf("Sound:     %8dm\n", (int)(sound_checks / 1000000));
#ifdef MESS
	printf("MESS:      %8dm\n", (int)(mess_checks / 1000000));
#endif
#endif

	end_resource_tracking();

	return error;
}
