
/*********************************************************************

    romload.c

    ROM loading functions.

    Copyright (c) 1996-2006, Nicola Salmoria and the MAME Team.
    Visit http://mamedev.org for licensing and usage restrictions.

*********************************************************************/

#include "osdepend.h"
#include "driver.h"
#include "hash.h"
#include "png.h"
#include "harddisk.h"
#include "artwork.h"
#include "config.h"
#include <stdarg.h>
#include <ctype.h>


//#define LOG_LOAD



/***************************************************************************

    Global variables

***************************************************************************/

/* disks */
static chd_file *disk_handle[4];

/* system BIOS */
int system_bios;

static int total_rom_load_warnings;



/***************************************************************************

    Hard disk handling

***************************************************************************/

chd_file *get_disk_handle(int diskindex)
{
	return disk_handle[diskindex];
}



/***************************************************************************

    ROM loading code

***************************************************************************/

/*-------------------------------------------------
    rom_first_region - return pointer to first ROM
    region
-------------------------------------------------*/

const rom_entry *rom_first_region(const game_driver *drv)
{
	return drv->rom;
}


/*-------------------------------------------------
    rom_next_region - return pointer to next ROM
    region
-------------------------------------------------*/

const rom_entry *rom_next_region(const rom_entry *romp)
{
	romp++;
	while (!ROMENTRY_ISREGIONEND(romp))
		romp++;
	return ROMENTRY_ISEND(romp) ? NULL : romp;
}


/*-------------------------------------------------
    rom_first_file - return pointer to first ROM
    file
-------------------------------------------------*/

const rom_entry *rom_first_file(const rom_entry *romp)
{
	romp++;
	while (!ROMENTRY_ISFILE(romp) && !ROMENTRY_ISREGIONEND(romp))
		romp++;
	return ROMENTRY_ISREGIONEND(romp) ? NULL : romp;
}


/*-------------------------------------------------
    rom_next_file - return pointer to next ROM
    file
-------------------------------------------------*/

const rom_entry *rom_next_file(const rom_entry *romp)
{
	romp++;
	while (!ROMENTRY_ISFILE(romp) && !ROMENTRY_ISREGIONEND(romp))
		romp++;
	return ROMENTRY_ISREGIONEND(romp) ? NULL : romp;
}


/*-------------------------------------------------
    rom_first_chunk - return pointer to first ROM
    chunk
-------------------------------------------------*/

const rom_entry *rom_first_chunk(const rom_entry *romp)
{
	return (ROMENTRY_ISFILE(romp)) ? romp : NULL;
}


/*-------------------------------------------------
    rom_next_chunk - return pointer to next ROM
    chunk
-------------------------------------------------*/

const rom_entry *rom_next_chunk(const rom_entry *romp)
{
	romp++;
	return (ROMENTRY_ISCONTINUE(romp)) ? romp : NULL;
}


/*-------------------------------------------------
    debugload - log data to a file
-------------------------------------------------*/

void CLIB_DECL debugload(const char *string, ...)
{
#ifdef LOG_LOAD
	static int opened;
	va_list arg;
	FILE *f;

	f = fopen("romload.log", opened++ ? "a" : "w");
	if (f)
	{
		va_start(arg, string);
		vfprintf(f, string, arg);
		va_end(arg);
		fclose(f);
	}
#endif
}


/*-------------------------------------------------
    determine_bios_rom - determine system_bios
    from SystemBios structure and options.bios
-------------------------------------------------*/

int determine_bios_rom(const bios_entry *bios)
{
	const bios_entry *firstbios = bios;

	/* set to default */
	int bios_no = 0;

	/* Not system_bios_0 and options.bios is set  */
	if(bios && (options.bios != NULL))
	{
		/* Allow '-bios n' to still be used */
		while(!BIOSENTRY_ISEND(bios))
		{
			char bios_number[3];
			sprintf(bios_number, "%d", bios->value);

			if(!strcmp(bios_number, options.bios))
				bios_no = bios->value;

			bios++;
		}

		bios = firstbios;

		/* Test for bios short names */
		while(!BIOSENTRY_ISEND(bios))
		{
			if(!strcmp(bios->_name, options.bios))
				bios_no = bios->value;

			bios++;
		}
	}

	debugload("Using System BIOS: %d\n", bios_no);

	return bios_no;
}


/*-------------------------------------------------
    count_roms - counts the total number of ROMs
    that will need to be loaded
-------------------------------------------------*/

static int count_roms(const rom_entry *romp)
{
	const rom_entry *region, *rom;
	int count = 0;

	/* determine the correct biosset to load based on options.bios string */
	int this_bios = determine_bios_rom(Machine->gamedrv->bios);

	/* loop over regions, then over files */
	for (region = romp; region; region = rom_next_region(region))
		for (rom = rom_first_file(region); rom; rom = rom_next_file(rom))
			if (!ROM_GETBIOSFLAGS(romp) || (ROM_GETBIOSFLAGS(romp) == (this_bios+1))) /* alternate bios sets */
				count++;

	/* return the total count */
	return count;
}


/*-------------------------------------------------
    fill_random - fills an area of memory with
    random data
-------------------------------------------------*/

static void fill_random(UINT8 *base, UINT32 length)
{
	while (length--)
		*base++ = rand();
}


/*-------------------------------------------------
    handle_missing_file - handles error generation
    for missing files
-------------------------------------------------*/

static void handle_missing_file(rom_load_data *romdata, const rom_entry *romp)
{
	/* optional files are okay */
	if (ROM_ISOPTIONAL(romp))
	{
		sprintf(&romdata->errorbuf[strlen(romdata->errorbuf)], "OPTIONAL %-12s NOT FOUND\n", ROM_GETNAME(romp));
		romdata->warnings++;
	}

	/* no good dumps are okay */
	else if (ROM_NOGOODDUMP(romp))
	{
		sprintf(&romdata->errorbuf[strlen(romdata->errorbuf)], "%-12s NOT FOUND (NO GOOD DUMP KNOWN)\n", ROM_GETNAME(romp));
		romdata->warnings++;
	}

	/* anything else is bad */
	else
	{
		sprintf(&romdata->errorbuf[strlen(romdata->errorbuf)], "%-12s NOT FOUND\n", ROM_GETNAME(romp));
		romdata->errors++;
	}
}


/*-------------------------------------------------
    dump_wrong_and_correct_checksums - dump an
    error message containing the wrong and the
    correct checksums for a given ROM
-------------------------------------------------*/

static void dump_wrong_and_correct_checksums(rom_load_data* romdata, const char* hash, const char* acthash)
{
	unsigned i;
	char chksum[256];
	unsigned found_functions;
	unsigned wrong_functions;

	found_functions = hash_data_used_functions(hash) & hash_data_used_functions(acthash);

	hash_data_print(hash, found_functions, chksum);
	sprintf(&romdata->errorbuf[strlen(romdata->errorbuf)], "    EXPECTED: %s\n", chksum);

	/* We dump informations only of the functions for which MAME provided
        a correct checksum. Other functions we might have calculated are
        useless here */
	hash_data_print(acthash, found_functions, chksum);
	sprintf(&romdata->errorbuf[strlen(romdata->errorbuf)], "       FOUND: %s\n", chksum);

	/* For debugging purposes, we check if the checksums available in the
       driver are correctly specified or not. This can be done by checking
       the return value of one of the extract functions. Maybe we want to
       activate this only in debug buils, but many developers only use
       release builds, so I keep it as is for now. */
	wrong_functions = 0;
	for (i=0;i<HASH_NUM_FUNCTIONS;i++)
		if (hash_data_extract_printable_checksum(hash, 1<<i, chksum) == 2)
			wrong_functions |= 1<<i;

	if (wrong_functions)
	{
		for (i=0;i<HASH_NUM_FUNCTIONS;i++)
			if (wrong_functions & (1<<i))
			{
				sprintf(&romdata->errorbuf[strlen(romdata->errorbuf)],
					"\tInvalid %s checksum treated as 0 (check leading zeros)\n",
					hash_function_name(1<<i));

				romdata->warnings++;
			}
	}
}


/*-------------------------------------------------
    verify_length_and_hash - verify the length
    and hash signatures of a file
-------------------------------------------------*/

static void verify_length_and_hash(rom_load_data *romdata, const char *name, UINT32 explength, const char* hash)
{
	UINT32 actlength;
	const char* acthash;

	/* we've already complained if there is no file */
	if (!romdata->file)
		return;

	/* get the length and CRC from the file */
	actlength = mame_fsize(romdata->file);
	acthash = mame_fhash(romdata->file);

	/* verify length */
	if (explength != actlength)
	{
		sprintf(&romdata->errorbuf[strlen(romdata->errorbuf)], "%-12s WRONG LENGTH (expected: %08x found: %08x)\n", name, explength, actlength);
		romdata->warnings++;
	}

	/* If there is no good dump known, write it */
	if (hash_data_has_info(hash, HASH_INFO_NO_DUMP))
	{
			sprintf(&romdata->errorbuf[strlen(romdata->errorbuf)], "%-12s NO GOOD DUMP KNOWN\n", name);
		romdata->warnings++;
	}
	/* verify checksums */
	else if (!hash_data_is_equal(hash, acthash, 0))
	{
		/* otherwise, it's just bad */
		sprintf(&romdata->errorbuf[strlen(romdata->errorbuf)], "%-12s WRONG CHECKSUMS:\n", name);

		dump_wrong_and_correct_checksums(romdata, hash, acthash);

		romdata->warnings++;
	}
	/* If it matches, but it is actually a bad dump, write it */
	else if (hash_data_has_info(hash, HASH_INFO_BAD_DUMP))
	{
		sprintf(&romdata->errorbuf[strlen(romdata->errorbuf)], "%-12s ROM NEEDS REDUMP\n",name);
		romdata->warnings++;
	}
}


/*-------------------------------------------------
    display_rom_load_results - display the final
    results of ROM loading
-------------------------------------------------*/

static int display_rom_load_results(rom_load_data *romdata)
{
	int region;

	/* final status display */
	osd_display_loading_rom_message(NULL, romdata);

	/* if we had errors, they are fatal */
	if (romdata->errors)
	{
		strcat(romdata->errorbuf, "ERROR: required files are missing, the game cannot be run.");
		fatalerror("%s", romdata->errorbuf);
	}

	/* if we had warnings, output them, but continue */
	if (romdata->warnings)
	{
		strcat(romdata->errorbuf, "WARNING: the game might not run correctly.");
		/* AdvanceMAME: Display the message in the ui */
		osd_display_loading_rom_message(romdata->errorbuf, 0);
	}

	/* clean up any regions */
	if (romdata->errors)
		for (region = 0; region < MAX_MEMORY_REGIONS; region++)
			free_memory_region(region);

	/* return true if we had any errors */
	return (romdata->errors != 0);
}


/*-------------------------------------------------
    region_post_process - post-process a region,
    byte swapping and inverting data as necessary
-------------------------------------------------*/

static void region_post_process(rom_load_data *romdata, const rom_entry *regiondata)
{
	int type = ROMREGION_GETTYPE(regiondata);
	int datawidth = ROMREGION_GETWIDTH(regiondata) / 8;
	int littleendian = ROMREGION_ISLITTLEENDIAN(regiondata);
	UINT8 *base;
	int i, j;

	debugload("+ datawidth=%d little=%d\n", datawidth, littleendian);

	/* if this is a CPU region, override with the CPU width and endianness */
	if (type >= REGION_CPU1 && type < REGION_CPU1 + MAX_CPU)
	{
		int cputype = Machine->drv->cpu[type - REGION_CPU1].cpu_type;
		if (cputype != 0)
		{
			datawidth = cputype_databus_width(cputype, ADDRESS_SPACE_PROGRAM) / 8;
			littleendian = (cputype_endianness(cputype) == CPU_IS_LE);
			debugload("+ CPU region #%d: datawidth=%d little=%d\n", type - REGION_CPU1, datawidth, littleendian);
		}
	}

	/* if the region is inverted, do that now */
	if (ROMREGION_ISINVERTED(regiondata))
	{
		debugload("+ Inverting region\n");
		for (i = 0, base = romdata->regionbase; i < romdata->regionlength; i++)
			*base++ ^= 0xff;
	}

	/* swap the endianness if we need to */
#ifdef LSB_FIRST
	if (datawidth > 1 && !littleendian)
#else
	if (datawidth > 1 && littleendian)
#endif
	{
		debugload("+ Byte swapping region\n");
		for (i = 0, base = romdata->regionbase; i < romdata->regionlength; i += datawidth)
		{
			UINT8 temp[8];
			memcpy(temp, base, datawidth);
			for (j = datawidth - 1; j >= 0; j--)
				*base++ = temp[j];
		}
	}
}


/*-------------------------------------------------
    open_rom_file - open a ROM file, searching
    up the parent and loading by checksum
-------------------------------------------------*/

static int open_rom_file(rom_load_data *romdata, const rom_entry *romp)
{
	const game_driver *drv;

	++romdata->romsloaded;

	/* update status display */
	if (osd_display_loading_rom_message(ROM_GETNAME(romp), romdata))
       return 0;

	/* Attempt reading up the chain through the parents. It automatically also
       attempts any kind of load by checksum supported by the archives. */
	romdata->file = NULL;
	for (drv = Machine->gamedrv; !romdata->file && drv; drv = driver_get_clone(drv))
		if (drv->name && *drv->name)
			romdata->file = mame_fopen_rom(drv->name, ROM_GETNAME(romp), ROM_GETHASHDATA(romp));

	/* return the result */
	return (romdata->file != NULL);
}


/*-------------------------------------------------
    rom_fread - cheesy fread that fills with
    random data for a NULL file
-------------------------------------------------*/

static int rom_fread(rom_load_data *romdata, UINT8 *buffer, int length)
{
	/* files just pass through */
	if (romdata->file)
		return mame_fread(romdata->file, buffer, length);

	/* otherwise, fill with randomness */
	else
		fill_random(buffer, length);

	return length;
}


/*-------------------------------------------------
    read_rom_data - read ROM data for a single
    entry
-------------------------------------------------*/

static int read_rom_data(rom_load_data *romdata, const rom_entry *romp)
{
	int datashift = ROM_GETBITSHIFT(romp);
	int datamask = ((1 << ROM_GETBITWIDTH(romp)) - 1) << datashift;
	int numbytes = ROM_GETLENGTH(romp);
	int groupsize = ROM_GETGROUPSIZE(romp);
	int skip = ROM_GETSKIPCOUNT(romp);
	int reversed = ROM_ISREVERSED(romp);
	int numgroups = (numbytes + groupsize - 1) / groupsize;
	UINT8 *base = romdata->regionbase + ROM_GETOFFSET(romp);
	int i;

	debugload("Loading ROM data: offs=%X len=%X mask=%02X group=%d skip=%d reverse=%d\n", ROM_GETOFFSET(romp), numbytes, datamask, groupsize, skip, reversed);

	/* make sure the length was an even multiple of the group size */
	if (numbytes % groupsize != 0)
	{
		printf("Error in RomModule definition: %s length not an even multiple of group size\n", ROM_GETNAME(romp));
		return -1;
	}

	/* make sure we only fill within the region space */
	if (ROM_GETOFFSET(romp) + numgroups * groupsize + (numgroups - 1) * skip > romdata->regionlength)
	{
		printf("Error in RomModule definition: %s out of memory region space\n", ROM_GETNAME(romp));
		return -1;
	}

	/* make sure the length was valid */
	if (numbytes == 0)
	{
		printf("Error in RomModule definition: %s has an invalid length\n", ROM_GETNAME(romp));
		return -1;
	}

	/* special case for simple loads */
	if (datamask == 0xff && (groupsize == 1 || !reversed) && skip == 0)
		return rom_fread(romdata, base, numbytes);

	/* chunky reads for complex loads */
	skip += groupsize;
	while (numbytes)
	{
		int evengroupcount = (sizeof(romdata->tempbuf) / groupsize) * groupsize;
		int bytesleft = (numbytes > evengroupcount) ? evengroupcount : numbytes;
		UINT8 *bufptr = romdata->tempbuf;

		/* read as much as we can */
		debugload("  Reading %X bytes into buffer\n", bytesleft);
		if (rom_fread(romdata, romdata->tempbuf, bytesleft) != bytesleft)
			return 0;
		numbytes -= bytesleft;

		debugload("  Copying to %08X\n", (int)base);

		/* unmasked cases */
		if (datamask == 0xff)
		{
			/* non-grouped data */
			if (groupsize == 1)
				for (i = 0; i < bytesleft; i++, base += skip)
					*base = *bufptr++;

			/* grouped data -- non-reversed case */
			else if (!reversed)
				while (bytesleft)
				{
					for (i = 0; i < groupsize && bytesleft; i++, bytesleft--)
						base[i] = *bufptr++;
					base += skip;
				}

			/* grouped data -- reversed case */
			else
				while (bytesleft)
				{
					for (i = groupsize - 1; i >= 0 && bytesleft; i--, bytesleft--)
						base[i] = *bufptr++;
					base += skip;
				}
		}

		/* masked cases */
		else
		{
			/* non-grouped data */
			if (groupsize == 1)
				for (i = 0; i < bytesleft; i++, base += skip)
					*base = (*base & ~datamask) | ((*bufptr++ << datashift) & datamask);

			/* grouped data -- non-reversed case */
			else if (!reversed)
				while (bytesleft)
				{
					for (i = 0; i < groupsize && bytesleft; i++, bytesleft--)
						base[i] = (base[i] & ~datamask) | ((*bufptr++ << datashift) & datamask);
					base += skip;
				}

			/* grouped data -- reversed case */
			else
				while (bytesleft)
				{
					for (i = groupsize - 1; i >= 0 && bytesleft; i--, bytesleft--)
						base[i] = (base[i] & ~datamask) | ((*bufptr++ << datashift) & datamask);
					base += skip;
				}
		}
	}
	debugload("  All done\n");
	return ROM_GETLENGTH(romp);
}


/*-------------------------------------------------
    fill_rom_data - fill a region of ROM space
-------------------------------------------------*/

static int fill_rom_data(rom_load_data *romdata, const rom_entry *romp)
{
	UINT32 numbytes = ROM_GETLENGTH(romp);
	UINT8 *base = romdata->regionbase + ROM_GETOFFSET(romp);

	/* make sure we fill within the region space */
	if (ROM_GETOFFSET(romp) + numbytes > romdata->regionlength)
	{
		printf("Error in RomModule definition: FILL out of memory region space\n");
		return 0;
	}

	/* make sure the length was valid */
	if (numbytes == 0)
	{
		printf("Error in RomModule definition: FILL has an invalid length\n");
		return 0;
	}

	/* fill the data (filling value is stored in place of the hashdata) */
	memset(base, (UINT32)ROM_GETHASHDATA(romp) & 0xff, numbytes);
	return 1;
}


/*-------------------------------------------------
    copy_rom_data - copy a region of ROM space
-------------------------------------------------*/

static int copy_rom_data(rom_load_data *romdata, const rom_entry *romp)
{
	UINT8 *base = romdata->regionbase + ROM_GETOFFSET(romp);
	int srcregion = ROM_GETFLAGS(romp) >> 24;
	UINT32 numbytes = ROM_GETLENGTH(romp);
	UINT32 srcoffs = (UINT32)ROM_GETHASHDATA(romp);  /* srcoffset in place of hashdata */
	UINT8 *srcbase;

	/* make sure we copy within the region space */
	if (ROM_GETOFFSET(romp) + numbytes > romdata->regionlength)
	{
		printf("Error in RomModule definition: COPY out of target memory region space\n");
		return 0;
	}

	/* make sure the length was valid */
	if (numbytes == 0)
	{
		printf("Error in RomModule definition: COPY has an invalid length\n");
		return 0;
	}

	/* make sure the source was valid */
	srcbase = memory_region(srcregion);
	if (!srcbase)
	{
		printf("Error in RomModule definition: COPY from an invalid region\n");
		return 0;
	}

	/* make sure we find within the region space */
	if (srcoffs + numbytes > memory_region_length(srcregion))
	{
		printf("Error in RomModule definition: COPY out of source memory region space\n");
		return 0;
	}

	/* fill the data */
	memcpy(base, srcbase + srcoffs, numbytes);
	return 1;
}


/*-------------------------------------------------
    process_rom_entries - process all ROM entries
    for a region
-------------------------------------------------*/

static int process_rom_entries(rom_load_data *romdata, const rom_entry *romp)
{
	UINT32 lastflags = 0;

	/* loop until we hit the end of this region */
	while (!ROMENTRY_ISREGIONEND(romp))
	{
		/* if this is a continue entry, it's invalid */
		if (ROMENTRY_ISCONTINUE(romp))
		{
			printf("Error in RomModule definition: ROM_CONTINUE not preceded by ROM_LOAD\n");
			goto fatalerror;
		}

		/* if this is a reload entry, it's invalid */
		if (ROMENTRY_ISRELOAD(romp))
		{
			printf("Error in RomModule definition: ROM_RELOAD not preceded by ROM_LOAD\n");
			goto fatalerror;
		}

		/* handle fills */
		if (ROMENTRY_ISFILL(romp))
		{
			if (!fill_rom_data(romdata, romp++))
				goto fatalerror;
		}

		/* handle copies */
		else if (ROMENTRY_ISCOPY(romp))
		{
			if (!copy_rom_data(romdata, romp++))
				goto fatalerror;
		}

		/* handle files */
		else if (ROMENTRY_ISFILE(romp))
		{
			if (!ROM_GETBIOSFLAGS(romp) || (ROM_GETBIOSFLAGS(romp) == (system_bios+1))) /* alternate bios sets */
			{
				const rom_entry *baserom = romp;
				int explength = 0;

				/* open the file */
				debugload("Opening ROM file: %s\n", ROM_GETNAME(romp));
				if (!open_rom_file(romdata, romp))
					handle_missing_file(romdata, romp);

				/* loop until we run out of reloads */
				do
				{
					/* loop until we run out of continues */
					do
					{
						rom_entry modified_romp = *romp++;
						int readresult;

						/* handle flag inheritance */
						if (!ROM_INHERITSFLAGS(&modified_romp))
							lastflags = modified_romp._flags;
						else
							modified_romp._flags = (modified_romp._flags & ~ROM_INHERITEDFLAGS) | lastflags;

						explength += ROM_GETLENGTH(&modified_romp);

						/* attempt to read using the modified entry */
						readresult = read_rom_data(romdata, &modified_romp);
						if (readresult == -1)
							goto fatalerror;
					}
					while (ROMENTRY_ISCONTINUE(romp));

					/* if this was the first use of this file, verify the length and CRC */
					if (baserom)
					{
						debugload("Verifying length (%X) and checksums\n", explength);
						verify_length_and_hash(romdata, ROM_GETNAME(baserom), explength, ROM_GETHASHDATA(baserom));
						debugload("Verify finished\n");
					}

					/* reseek to the start and clear the baserom so we don't reverify */
					if (romdata->file)
						mame_fseek(romdata->file, 0, SEEK_SET);
					baserom = NULL;
					explength = 0;
				}
				while (ROMENTRY_ISRELOAD(romp));

				/* close the file */
				if (romdata->file)
				{
					debugload("Closing ROM file\n");
					mame_fclose(romdata->file);
					romdata->file = NULL;
				}
			}
			else
			{
				romp++; /* skip over file */
			}
		}
		else
		{
			romp++;	/* something else; skip */
		}
	}
	return 1;

	/* error case */
fatalerror:
	if (romdata->file)
		mame_fclose(romdata->file);
	romdata->file = NULL;
	return 0;
}


/*-------------------------------------------------
    process_disk_entries - process all disk entries
    for a region
-------------------------------------------------*/

static int process_disk_entries(rom_load_data *romdata, const rom_entry *romp)
{
	/* loop until we hit the end of this region */
	while (!ROMENTRY_ISREGIONEND(romp))
	{
		/* handle files */
		if (ROMENTRY_ISFILE(romp))
		{
			chd_file *source, *diff = NULL;
			chd_header header;
			char filename[1024];
			char acthash[HASH_BUF_SIZE];
			int err;

			/* make the filename of the source */
			sprintf(filename, "%s.chd", ROM_GETNAME(romp));

			/* first open the source drive */
			debugload("Opening disk image: %s\n", filename);
			source = chd_open(filename, 0, NULL);
			if (!source)
			{
				if (chd_get_last_error() == CHDERR_UNSUPPORTED_VERSION)
					sprintf(&romdata->errorbuf[strlen(romdata->errorbuf)], "%-12s UNSUPPORTED CHD VERSION\n", filename);
				else
					sprintf(&romdata->errorbuf[strlen(romdata->errorbuf)], "%-12s NOT FOUND\n", filename);
				romdata->errors++;
				romp++;
				continue;
			}

			/* get the header and extract the MD5/SHA1 */
			header = *chd_get_header(source);
			hash_data_clear(acthash);
			hash_data_insert_binary_checksum(acthash, HASH_MD5, header.md5);
			hash_data_insert_binary_checksum(acthash, HASH_SHA1, header.sha1);

			/* verify the MD5 */
			if (!hash_data_is_equal(ROM_GETHASHDATA(romp), acthash, 0))
			{
				sprintf(&romdata->errorbuf[strlen(romdata->errorbuf)], "%-12s WRONG CHECKSUMS:\n", filename);
				dump_wrong_and_correct_checksums(romdata, ROM_GETHASHDATA(romp), acthash);
				romdata->warnings++;
			}

			/* if not read-only, make the diff file */
			if (!DISK_ISREADONLY(romp))
			{
				/* make the filename of the diff */
				sprintf(filename, "%s.dif", ROM_GETNAME(romp));

				/* try to open the diff */
				debugload("Opening differencing image: %s\n", filename);
				diff = chd_open(filename, 1, source);
				if (!diff)
				{
					/* didn't work; try creating it instead */
					debugload("Creating differencing image: %s\n", filename);
					err = chd_create(filename, 0, 0, CHDCOMPRESSION_NONE, source);
					if (err != CHDERR_NONE)
					{
						if (chd_get_last_error() == CHDERR_UNSUPPORTED_VERSION)
							sprintf(&romdata->errorbuf[strlen(romdata->errorbuf)], "%-12s UNSUPPORTED CHD VERSION\n", filename);
						else
							sprintf(&romdata->errorbuf[strlen(romdata->errorbuf)], "%-12s: CAN'T CREATE DIFF FILE\n", filename);
						romdata->errors++;
						romp++;
						continue;
					}

					/* open the newly-created diff file */
					debugload("Opening differencing image: %s\n", filename);
					diff = chd_open(filename, 1, source);
					if (!diff)
					{
						if (chd_get_last_error() == CHDERR_UNSUPPORTED_VERSION)
							sprintf(&romdata->errorbuf[strlen(romdata->errorbuf)], "%-12s UNSUPPORTED CHD VERSION\n", filename);
						else
							sprintf(&romdata->errorbuf[strlen(romdata->errorbuf)], "%-12s: CAN'T OPEN DIFF FILE\n", filename);
						romdata->errors++;
						romp++;
						continue;
					}
				}
			}

			/* we're okay, set the handle */
			debugload("Assigning to handle %d\n", DISK_GETINDEX(romp));
			disk_handle[DISK_GETINDEX(romp)] = DISK_ISREADONLY(romp) ? source : diff;
			romp++;
		}
	}
	return 1;
}


/*-------------------------------------------------
    rom_init - new, more flexible ROM
    loading system
-------------------------------------------------*/

int rom_init(const rom_entry *romp)
{
	const rom_entry *regionlist[REGION_MAX];
	const rom_entry *region;
	static rom_load_data romdata;
	int regnum;

	/* if no roms, bail */
	if (romp == NULL)
		return 0;

	/* reset the region list */
	memset((void *)regionlist, 0, sizeof(regionlist));

	/* reset the romdata struct */
	memset(&romdata, 0, sizeof(romdata));
	romdata.romstotal = count_roms(romp);

	/* reset the disk list */
	memset(disk_handle, 0, sizeof(disk_handle));

	/* determine the correct biosset to load based on options.bios string */
	system_bios = determine_bios_rom(Machine->gamedrv->bios);

	/* loop until we hit the end */
	for (region = romp, regnum = 0; region; region = rom_next_region(region), regnum++)
	{
		int regiontype = ROMREGION_GETTYPE(region);

		debugload("Processing region %02X (length=%X)\n", regiontype, ROMREGION_GETLENGTH(region));

		/* the first entry must be a region */
		if (!ROMENTRY_ISREGION(region))
		{
			printf("Error: missing ROM_REGION header\n");
			return 1;
		}

		/* allocate memory for the region */
		if (new_memory_region(regiontype, ROMREGION_GETLENGTH(region), ROMREGION_GETFLAGS(region)) != 0)
		{
			printf("Error: unable to allocate memory for region %d\n", regiontype);
			return 1;
		}

		/* remember the base and length */
		romdata.regionlength = memory_region_length(regiontype);
		romdata.regionbase = memory_region(regiontype);
		debugload("Allocated %X bytes @ %08X\n", romdata.regionlength, (int)romdata.regionbase);

		/* clear the region if it's requested */
		if (ROMREGION_ISERASE(region))
			memset(romdata.regionbase, ROMREGION_GETERASEVAL(region), romdata.regionlength);

		/* or if it's sufficiently small (<= 4MB) */
		else if (romdata.regionlength <= 0x400000)
			memset(romdata.regionbase, 0, romdata.regionlength);

#ifdef MAME_DEBUG
		/* if we're debugging, fill region with random data to catch errors */
		else
			fill_random(romdata.regionbase, romdata.regionlength);
#endif

		/* now process the entries in the region */
		if (ROMREGION_ISROMDATA(region))
		{
			if (!process_rom_entries(&romdata, region + 1))
				return 1;
		}
		else if (ROMREGION_ISDISKDATA(region))
		{
			if (!process_disk_entries(&romdata, region + 1))
				return 1;
		}

		/* add this region to the list */
		if (regiontype < REGION_MAX)
			regionlist[regiontype] = region;
	}

	/* post-process the regions */
	for (regnum = 0; regnum < REGION_MAX; regnum++)
		if (regionlist[regnum])
		{
			debugload("Post-processing region %02X\n", regnum);
			romdata.regionlength = memory_region_length(regnum);
			romdata.regionbase = memory_region(regnum);
			region_post_process(&romdata, regionlist[regnum]);
		}

	/* display the results and exit */
	total_rom_load_warnings = romdata.warnings;

	add_exit_callback(rom_exit);
	return display_rom_load_results(&romdata);
}


/*-------------------------------------------------
    rom_exit - clean up after ourselves
-------------------------------------------------*/

void rom_exit(void)
{
	int i;

	/* free the memory allocated for various regions */
	for (i = 0; i < MAX_MEMORY_REGIONS; i++)
		free_memory_region(i);

	/* close all hard drives */
	chd_close_all();
}


int rom_load_warnings(void)
{
	return total_rom_load_warnings;
}
