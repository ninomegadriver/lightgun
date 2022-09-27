/***************************************************************************

    MAME Compressed Hunks of Data file format

    Copyright (c) 1996-2006, Nicola Salmoria and the MAME Team.
    Visit http://mamedev.org for licensing and usage restrictions.

***************************************************************************/

#ifndef __CHD_H__
#define __CHD_H__

#include "mamecore.h"


/***************************************************************************

    Compressed Hunks of Data header format. All numbers are stored in
    Motorola (big-endian) byte ordering. The header is 76 (V1) or 80 (V2)
    bytes long.

    V1 header:

    [  0] char   tag[8];        // 'MComprHD'
    [  8] UINT32 length;        // length of header (including tag and length fields)
    [ 12] UINT32 version;       // drive format version
    [ 16] UINT32 flags;         // flags (see below)
    [ 20] UINT32 compression;   // compression type
    [ 24] UINT32 hunksize;      // 512-byte sectors per hunk
    [ 28] UINT32 totalhunks;    // total # of hunks represented
    [ 32] UINT32 cylinders;     // number of cylinders on hard disk
    [ 36] UINT32 heads;         // number of heads on hard disk
    [ 40] UINT32 sectors;       // number of sectors on hard disk
    [ 44] UINT8  md5[16];       // MD5 checksum of raw data
    [ 60] UINT8  parentmd5[16]; // MD5 checksum of parent file
    [ 76] (V1 header length)

    V2 header:

    [  0] char   tag[8];        // 'MComprHD'
    [  8] UINT32 length;        // length of header (including tag and length fields)
    [ 12] UINT32 version;       // drive format version
    [ 16] UINT32 flags;         // flags (see below)
    [ 20] UINT32 compression;   // compression type
    [ 24] UINT32 hunksize;      // seclen-byte sectors per hunk
    [ 28] UINT32 totalhunks;    // total # of hunks represented
    [ 32] UINT32 cylinders;     // number of cylinders on hard disk
    [ 36] UINT32 heads;         // number of heads on hard disk
    [ 40] UINT32 sectors;       // number of sectors on hard disk
    [ 44] UINT8  md5[16];       // MD5 checksum of raw data
    [ 60] UINT8  parentmd5[16]; // MD5 checksum of parent file
    [ 76] UINT32 seclen;        // number of bytes per sector
    [ 80] (V2 header length)

    V3 header:

    [  0] char   tag[8];        // 'MComprHD'
    [  8] UINT32 length;        // length of header (including tag and length fields)
    [ 12] UINT32 version;       // drive format version
    [ 16] UINT32 flags;         // flags (see below)
    [ 20] UINT32 compression;   // compression type
    [ 24] UINT32 totalhunks;    // total # of hunks represented
    [ 28] UINT64 logicalbytes;  // logical size of the data (in bytes)
    [ 36] UINT64 metaoffset;    // offset to the first blob of metadata
    [ 44] UINT8  md5[16];       // MD5 checksum of raw data
    [ 60] UINT8  parentmd5[16]; // MD5 checksum of parent file
    [ 76] UINT32 hunkbytes;     // number of bytes per hunk
    [ 80] UINT8  sha1[20];      // SHA1 checksum of raw data
    [100] UINT8  parentsha1[20];// SHA1 checksum of parent file
    [120] (V3 header length)

    Flags:
        0x00000001 - set if this drive has a parent
        0x00000002 - set if this drive allows writes

***************************************************************************/

/*************************************
 *
 *  Constants
 *
 *************************************/

#define CHD_HEADER_VERSION			3
#define CHD_V1_HEADER_SIZE			76
#define CHD_V2_HEADER_SIZE			80
#define CHD_V3_HEADER_SIZE			120
#define CHD_MAX_HEADER_SIZE			CHD_V3_HEADER_SIZE

#define CHD_MD5_BYTES				16
#define CHD_SHA1_BYTES				20

#define CHDFLAGS_HAS_PARENT			0x00000001
#define CHDFLAGS_IS_WRITEABLE		0x00000002
#define CHDFLAGS_UNDEFINED			0xfffffffc

#define CHDCOMPRESSION_NONE			0
#define CHDCOMPRESSION_ZLIB			1
#define CHDCOMPRESSION_ZLIB_PLUS	2
#define CHDCOMPRESSION_MAX			3

#define CHD_MAX_METADATA_SIZE		4096
#define CHDMETATAG_WILDCARD			0
#define CHD_METAINDEX_APPEND		((UINT32)-1)

#define HARD_DISK_STANDARD_METADATA	0x47444444
#define HARD_DISK_METADATA_FORMAT	"CYLS:%d,HEADS:%d,SECS:%d,BPS:%d"

#define CDROM_STANDARD_METADATA		0x43484344	/* 'CHCD' */

enum
{
	CHDERR_NONE,
	CHDERR_NO_INTERFACE,
	CHDERR_OUT_OF_MEMORY,
	CHDERR_INVALID_FILE,
	CHDERR_INVALID_PARAMETER,
	CHDERR_INVALID_DATA,
	CHDERR_FILE_NOT_FOUND,
	CHDERR_REQUIRES_PARENT,
	CHDERR_FILE_NOT_WRITEABLE,
	CHDERR_READ_ERROR,
	CHDERR_WRITE_ERROR,
	CHDERR_CODEC_ERROR,
	CHDERR_INVALID_PARENT,
	CHDERR_HUNK_OUT_OF_RANGE,
	CHDERR_DECOMPRESSION_ERROR,
	CHDERR_COMPRESSION_ERROR,
	CHDERR_CANT_CREATE_FILE,
	CHDERR_CANT_VERIFY,
	CHDERR_NOT_SUPPORTED,
	CHDERR_METADATA_NOT_FOUND,
	CHDERR_INVALID_METADATA_SIZE,
	CHDERR_UNSUPPORTED_VERSION
};



/*************************************
 *
 *  Type definitions
 *
 *************************************/

struct _chd_header
{
	UINT32	length;						/* length of header data */
	UINT32	version;					/* drive format version */
	UINT32	flags;						/* flags field */
	UINT32	compression;				/* compression type */
	UINT32	hunkbytes;					/* number of bytes per hunk */
	UINT32	totalhunks;					/* total # of hunks represented */
	UINT64	logicalbytes;				/* logical size of the data */
	UINT64	metaoffset;					/* offset in file of first metadata */
	UINT8	md5[CHD_MD5_BYTES];			/* MD5 checksum of raw data */
	UINT8	parentmd5[CHD_MD5_BYTES];	/* MD5 checksum of parent file */
	UINT8	sha1[CHD_SHA1_BYTES];		/* SHA1 checksum of raw data */
	UINT8	parentsha1[CHD_SHA1_BYTES];	/* SHA1 checksum of parent file */

	UINT32	obsolete_cylinders;			/* obsolete field -- do not use! */
	UINT32	obsolete_sectors;			/* obsolete field -- do not use! */
	UINT32	obsolete_heads;				/* obsolete field -- do not use! */
	UINT32	obsolete_hunksize;			/* obsolete field -- do not use! */
};
typedef struct _chd_header chd_header;


typedef struct _chd_exfile chd_exfile;
typedef struct _chd_interface_file chd_interface_file;


struct _chd_interface
{
	chd_interface_file *(*open)(const char *filename, const char *mode);
	void (*close)(chd_interface_file *file);
	UINT32 (*read)(chd_interface_file *file, UINT64 offset, UINT32 count, void *buffer);
	UINT32 (*write)(chd_interface_file *file, UINT64 offset, UINT32 count, const void *buffer);
	UINT64 (*length)(chd_interface_file *file);
};
typedef struct _chd_interface chd_interface;



/*************************************
 *
 *  Prototypes
 *
 *************************************/

void chd_set_interface(chd_interface *new_interface);
void chd_save_interface(chd_interface *interface_save);

int chd_create(const char *filename, UINT64 logicalbytes, UINT32 hunkbytes, UINT32 compression, chd_file *parent);
chd_file *chd_open(const char *filename, int writeable, chd_file *parent);
void chd_close(chd_file *chd);
void chd_close_all(void);

UINT32 chd_get_metadata(chd_file *chd, UINT32 *metatag, UINT32 metaindex, void *outputbuf, UINT32 outputlen);
int chd_set_metadata(chd_file *chd, UINT32 metatag, UINT32 metaindex, const void *inputbuf, UINT32 inputlen);

UINT32 chd_read(chd_file *chd, UINT32 hunknum, UINT32 hunkcount, void *buffer);
UINT32 chd_write(chd_file *chd, UINT32 hunknum, UINT32 hunkcount, const void *buffer);

int chd_get_last_error(void);
const chd_header *chd_get_header(chd_file *chd);
int chd_set_header(const char *filename, const chd_header *header);

int chd_compress(chd_file *chd, const char *rawfile, UINT32 offset, void (*progress)(const char *, ...));
int chd_verify(chd_file *chd, void (*progress)(const char *, ...), UINT8 actualmd5[CHD_MD5_BYTES], UINT8 actualsha1[CHD_SHA1_BYTES]);

chd_exfile *chd_start_compress_ex(chd_file *chd);
int chd_compress_ex(chd_exfile *chdex, const char *rawfile, UINT64 offset, UINT32 inpsecsize, UINT32 srcperhunk, UINT32 hunks_to_read, UINT32 hunksecsize, void (*progress)(const char *, ...));
int chd_end_compress_ex(chd_exfile *chdex, void (*progress)(const char *, ...));

#endif /* __CHD_H__ */
