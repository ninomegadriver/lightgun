/***************************************************************************

    fileio.h

    Core file I/O interface functions and definitions.

    Copyright (c) 1996-2006, Nicola Salmoria and the MAME Team.
    Visit http://mamedev.org for licensing and usage restrictions.

***************************************************************************/

#ifndef __FILEIO_H__
#define __FILEIO_H__

#include "mamecore.h"


/* file types */
enum
{
	FILETYPE_RAW = 0,
	FILETYPE_ROM,
	FILETYPE_IMAGE,
	FILETYPE_IMAGE_DIFF,
	FILETYPE_SAMPLE,
	FILETYPE_ARTWORK,
	FILETYPE_NVRAM,
	FILETYPE_HIGHSCORE,
	FILETYPE_HIGHSCORE_DB,
	FILETYPE_CONFIG,
	FILETYPE_INPUTLOG,
	FILETYPE_STATE,
	FILETYPE_MEMCARD,
	FILETYPE_SCREENSHOT,
	FILETYPE_MOVIE,
	FILETYPE_HISTORY,
	FILETYPE_CHEAT,
	FILETYPE_LANGUAGE,
	FILETYPE_CTRLR,
	FILETYPE_INI,
	FILETYPE_COMMENT,
	FILETYPE_DEBUGLOG,
	FILETYPE_HASH,	/* MESS-specific */
	FILETYPE_end 	/* dummy last entry */
};


/* gamename holds the driver name, filename is only used for ROMs and    */
/* samples. If 'write' is not 0, the file is opened for write. Otherwise */
/* it is opened for read. */

void fileio_init(void);
void fileio_exit(void);

int mame_faccess(const char *filename, int filetype);
mame_file *mame_fopen(const char *gamename, const char *filename, int filetype, int openforwrite);
mame_file *mame_fopen_error(const char *gamename, const char *filename, int filetype, int openforwrite, osd_file_error *error);
mame_file *mame_fopen_rom(const char *gamename, const char *filename, const char *exphash);
UINT32 mame_fread(mame_file *file, void *buffer, UINT32 length);
UINT32 mame_fwrite(mame_file *file, const void *buffer, UINT32 length);
UINT32 mame_fread_swap(mame_file *file, void *buffer, UINT32 length);
UINT32 mame_fwrite_swap(mame_file *file, const void *buffer, UINT32 length);
#ifdef LSB_FIRST
#define mame_fread_msbfirst mame_fread_swap
#define mame_fwrite_msbfirst mame_fwrite_swap
#define mame_fread_lsbfirst mame_fread
#define mame_fwrite_lsbfirst mame_fwrite
#else
#define mame_fread_msbfirst mame_fread
#define mame_fwrite_msbfirst mame_fwrite
#define mame_fread_lsbfirst mame_fread_swap
#define mame_fwrite_lsbfirst mame_fwrite_swap
#endif
int mame_fseek(mame_file *file, INT64 offset, int whence);
void mame_fclose(mame_file *file);
int mame_fchecksum(const char *gamename, const char *filename, unsigned int *length, char *hash);
UINT64 mame_fsize(mame_file *file);
const char *mame_fhash(mame_file *file);
int mame_fgetc(mame_file *file);
int mame_ungetc(int c, mame_file *file);
char *mame_fgets(char *s, int n, mame_file *file);
int mame_feof(mame_file *file);
UINT64 mame_ftell(mame_file *file);

int mame_fputs(mame_file *f, const char *s);
int CLIB_DECL mame_fprintf(mame_file *f, const char *fmt, ...) ATTR_PRINTF(2,3);

#endif	/* __FILEIO_H__ */
