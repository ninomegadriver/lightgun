/***********************************************************

  utils.h

  Nifty utility code

***********************************************************/

#ifndef UTILS_H
#define UTILS_H

#include <string.h>
#include "mamecore.h"

/* -----------------------------------------------------------------------
 * osdutils.h is a header file that gives overrides for functions we
 * define below, so if a native implementation is available, we can use
 * it
 * ----------------------------------------------------------------------- */

#include "osdutils.h"

/* -----------------------------------------------------------------------
 * strncpyz
 * strncatz
 *
 * strncpy done right :-)
 * ----------------------------------------------------------------------- */
char *strncpyz(char *dest, const char *source, size_t len);
char *strncatz(char *dest, const char *source, size_t len);

/* -----------------------------------------------------------------------
 * rtrim
 *
 * Removes all trailing spaces from a string
 * ----------------------------------------------------------------------- */
void rtrim(char *buf);

/* -----------------------------------------------------------------------
 * strncmpi
 *
 * Case insensitive compares.  If your platform has this function then
 * #define it in "osdutils.h"
 * ----------------------------------------------------------------------- */

#ifndef strncmpi
int strncmpi(const char *dst, const char *src, size_t n);
#endif /* strncmpi */

/* -----------------------------------------------------------------------
 * osd_basename
 *
 * Given a pathname, returns the partially qualified path.  If your
 * platform has this function then #define it in "osdutils.h"
 * ----------------------------------------------------------------------- */

#ifndef osd_basename
char *osd_basename (char *name);
#endif /* osd_basename */



/* -----------------------------------------------------------------------
 * osd_mkdir
 * osd_rmdir
 * osd_rmfile
 * osd_copyfile
 * osd_getcurdir
 * osd_setcurdir
 *
 * Misc platform independent dir/file functions.
 * ----------------------------------------------------------------------- */

#ifndef osd_mkdir
void osd_mkdir(const char *dir);
#endif /* osd_mkdir */

#ifndef osd_rmdir
void osd_rmdir(const char *dir);
#endif /* osd_rmdir */

#ifndef osd_rmfile
void osd_rmfile(const char *filepath);
#endif /* osd_rmfile */

#ifndef osd_copyfile
void osd_copyfile(const char *destfile, const char *srcfile);
#endif /* osd_copyfile */

#ifndef osd_getcurdir
void osd_getcurdir(char *buffer, size_t buffer_len);
#endif /* osd_getcurdir */

#ifndef osd_getcurdir
void osd_setcurdir(const char *dir);
#endif /* osd_getcurdir */



/* -----------------------------------------------------------------------
 * memset16
 *
 * 16 bit memset
 * ----------------------------------------------------------------------- */
#ifndef memset16
void *memset16 (void *dest, int value, size_t size);
#endif /* memset16 */


/* -----------------------------------------------------------------------
 * CRC stuff
 * ----------------------------------------------------------------------- */
unsigned short ccitt_crc16(unsigned short crc, const unsigned char *buffer, size_t buffer_len);
unsigned short ccitt_crc16_one( unsigned short crc, const unsigned char data );

/* -----------------------------------------------------------------------
 * Alignment-friendly integer placement
 * ----------------------------------------------------------------------- */

INLINE void place_integer_be(void *ptr, size_t offset, size_t size, UINT64 value)
{
	UINT8 *byte_ptr = ((UINT8 *) ptr) + offset;
	UINT16 val16;
	UINT32 val32;

	switch(size)
	{
		case 2:
			val16 = BIG_ENDIANIZE_INT16((UINT16) value);
			memcpy(byte_ptr, &val16, sizeof(val16));
			break;

		case 4:
			val32 = BIG_ENDIANIZE_INT32((UINT32) value);
			memcpy(byte_ptr, &val32, sizeof(val32));
			break;

		default:
			if (size >= 1)	byte_ptr[0] = (UINT8) (value >> ((size - 1) * 8));
			if (size >= 2)	byte_ptr[1] = (UINT8) (value >> ((size - 2) * 8));
			if (size >= 3)	byte_ptr[2] = (UINT8) (value >> ((size - 3) * 8));
			if (size >= 4)	byte_ptr[3] = (UINT8) (value >> ((size - 4) * 8));
			if (size >= 5)	byte_ptr[4] = (UINT8) (value >> ((size - 5) * 8));
			if (size >= 6)	byte_ptr[5] = (UINT8) (value >> ((size - 6) * 8));
			if (size >= 7)	byte_ptr[6] = (UINT8) (value >> ((size - 7) * 8));
			if (size >= 8)	byte_ptr[7] = (UINT8) (value >> ((size - 8) * 8));
			break;
	}
}

INLINE UINT64 pick_integer_be(const void *ptr, size_t offset, size_t size)
{
	UINT64 result = 0;
	const UINT8 *byte_ptr = ((const UINT8 *) ptr) + offset;
	UINT16 val16;
	UINT32 val32;

	switch(size)
	{
		case 1:
			result = *byte_ptr;
			break;

		case 2:
			memcpy(&val16, byte_ptr, sizeof(val16));
			result = BIG_ENDIANIZE_INT16(val16);
			break;

		case 4:
			memcpy(&val32, byte_ptr, sizeof(val32));
			result = BIG_ENDIANIZE_INT32(val32);
			break;

		default:
			if (size >= 1)	result |= ((UINT64) byte_ptr[0]) << ((size - 1) * 8);
			if (size >= 2)	result |= ((UINT64) byte_ptr[1]) << ((size - 2) * 8);
			if (size >= 3)	result |= ((UINT64) byte_ptr[2]) << ((size - 3) * 8);
			if (size >= 4)	result |= ((UINT64) byte_ptr[3]) << ((size - 4) * 8);
			if (size >= 5)	result |= ((UINT64) byte_ptr[4]) << ((size - 5) * 8);
			if (size >= 6)	result |= ((UINT64) byte_ptr[5]) << ((size - 6) * 8);
			if (size >= 7)	result |= ((UINT64) byte_ptr[6]) << ((size - 7) * 8);
			if (size >= 8)	result |= ((UINT64) byte_ptr[7]) << ((size - 8) * 8);
			break;
	}
	return result;
}

INLINE void place_integer_le(void *ptr, size_t offset, size_t size, UINT64 value)
{
	UINT8 *byte_ptr = ((UINT8 *) ptr) + offset;
	UINT16 val16;
	UINT32 val32;

	switch(size)
	{
		case 2:
			val16 = LITTLE_ENDIANIZE_INT16((UINT16) value);
			memcpy(byte_ptr, &val16, sizeof(val16));
			break;

		case 4:
			val32 = LITTLE_ENDIANIZE_INT32((UINT32) value);
			memcpy(byte_ptr, &val32, sizeof(val32));
			break;

		default:
			if (size >= 1)	byte_ptr[0] = (UINT8) (value >> (0 * 8));
			if (size >= 2)	byte_ptr[1] = (UINT8) (value >> (1 * 8));
			if (size >= 3)	byte_ptr[2] = (UINT8) (value >> (2 * 8));
			if (size >= 4)	byte_ptr[3] = (UINT8) (value >> (3 * 8));
			if (size >= 5)	byte_ptr[4] = (UINT8) (value >> (4 * 8));
			if (size >= 6)	byte_ptr[5] = (UINT8) (value >> (5 * 8));
			if (size >= 7)	byte_ptr[6] = (UINT8) (value >> (6 * 8));
			if (size >= 8)	byte_ptr[7] = (UINT8) (value >> (7 * 8));
			break;
	}
}

INLINE UINT64 pick_integer_le(const void *ptr, size_t offset, size_t size)
{
	UINT64 result = 0;
	const UINT8 *byte_ptr = ((const UINT8 *) ptr) + offset;
	UINT16 val16;
	UINT32 val32;

	switch(size)
	{
		case 1:
			result = *byte_ptr;
			break;

		case 2:
			memcpy(&val16, byte_ptr, sizeof(val16));
			result = LITTLE_ENDIANIZE_INT16(val16);
			break;

		case 4:
			memcpy(&val32, byte_ptr, sizeof(val32));
			result = LITTLE_ENDIANIZE_INT32(val32);
			break;

		default:
			if (size >= 1)	result |= ((UINT64) byte_ptr[0]) << (0 * 8);
			if (size >= 2)	result |= ((UINT64) byte_ptr[1]) << (1 * 8);
			if (size >= 3)	result |= ((UINT64) byte_ptr[2]) << (2 * 8);
			if (size >= 4)	result |= ((UINT64) byte_ptr[3]) << (3 * 8);
			if (size >= 5)	result |= ((UINT64) byte_ptr[4]) << (4 * 8);
			if (size >= 6)	result |= ((UINT64) byte_ptr[5]) << (5 * 8);
			if (size >= 7)	result |= ((UINT64) byte_ptr[6]) << (6 * 8);
			if (size >= 8)	result |= ((UINT64) byte_ptr[7]) << (7 * 8);
			break;
	}
	return result;
}

/* -----------------------------------------------------------------------
 * Miscellaneous
 * ----------------------------------------------------------------------- */

#ifndef PATH_SEPARATOR
#define PATH_SEPARATOR	'/'
#endif



/* miscellaneous functions */
char *stripspace(const char *src);
char *strip_extension(const char *filename);
int compute_log2(int val);
int findextension(const char *extensions, const char *ext);
int hexdigit(char c);

#endif /* UTILS_H */
