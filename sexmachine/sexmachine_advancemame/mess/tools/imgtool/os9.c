/****************************************************************************

	os9.c

	CoCo OS-9 disk images

****************************************************************************/

#include <time.h>

#include "imgtool.h"
#include "formats/coco_dsk.h"
#include "iflopimg.h"

typedef enum
{
	CREATE_NONE,
	CREATE_FILE,
	CREATE_DIR
} creation_policy_t;

struct os9_diskinfo
{
	UINT32 total_sectors;
	UINT32 sectors_per_track;
	UINT32 allocation_bitmap_bytes;
	UINT32 cluster_size;
	UINT32 root_dir_lsn;
	UINT32 owner_id;
	UINT32 attributes;
	UINT32 disk_id;
	UINT32 format_flags;
	UINT32 bootstrap_lsn;
	UINT32 bootstrap_size;
	UINT32 sector_size;

	unsigned int sides : 2;
	unsigned int double_density : 1;
	unsigned int double_track : 1;
	unsigned int quad_track_density : 1;
	unsigned int octal_track_density : 1;

	char name[33];
	UINT8 *allocation_bitmap;
};


struct os9_fileinfo
{
	unsigned int directory : 1;
	unsigned int non_sharable : 1;
	unsigned int public_execute : 1;
	unsigned int public_write : 1;
	unsigned int public_read : 1;
	unsigned int user_execute : 1;
	unsigned int user_write : 1;
	unsigned int user_read : 1;

	UINT32 lsn;
	UINT32 owner_id;
	UINT32 link_count;
	UINT32 file_size;

	struct
	{
		UINT32 lsn;
		UINT32 count;
	} sector_map[48];
};


struct os9_direnum
{
	struct os9_fileinfo dir_info;
	UINT32 index;
};



static void pick_string(const void *ptr, size_t offset, size_t length, char *dest)
{
	UINT8 b;

	while(length--)
	{
		b = ((UINT8 *) ptr)[offset++];
		*(dest++) = b & 0x7F;
		if (b & 0x80)
			length = 0;
	}
	*dest = '\0';
}



static void place_string(void *ptr, size_t offset, size_t length, const char *s)
{
	size_t i;
	UINT8 b;
	UINT8 *bptr;

	bptr = (UINT8 *) ptr;
	bptr += offset;

	bptr[0] = 0x80;

	for (i = 0; s[i] && (i < length); i++)
	{
		b = ((UINT8) s[i]) & 0x7F;
		if (s[i+1] == '\0')
			b |= 0x80;
		bptr[i] = b;
	}
}



static imgtoolerr_t os9_locate_lsn(imgtool_image *image, UINT32 lsn, UINT32 *head, UINT32 *track, UINT32 *sector)
{
	const struct os9_diskinfo *disk_info;

	disk_info = (const struct os9_diskinfo *) imgtool_floppy_extrabytes(image);
	if (lsn > disk_info->total_sectors)
		return IMGTOOLERR_CORRUPTIMAGE;

	*track = lsn / (disk_info->sectors_per_track * disk_info->sides);
	*head = (lsn / disk_info->sectors_per_track) % disk_info->sides;
	*sector = (lsn % disk_info->sectors_per_track) + 1;
	return IMGTOOLERR_SUCCESS;
}



static imgtoolerr_t os9_read_lsn(imgtool_image *img, UINT32 lsn, int offset, void *buffer, size_t buffer_len)
{
	imgtoolerr_t err;
	floperr_t ferr;
	UINT32 head, track, sector;

	err = os9_locate_lsn(img, lsn, &head, &track, &sector);
	if (err)
		return err;

	ferr = floppy_read_sector(imgtool_floppy(img), head, track, sector, offset, buffer, buffer_len);
	if (ferr)
		return imgtool_floppy_error(ferr);

	return IMGTOOLERR_SUCCESS;
}



static imgtoolerr_t os9_write_lsn(imgtool_image *img, UINT32 lsn, int offset, const void *buffer, size_t buffer_len)
{
	imgtoolerr_t err;
	floperr_t ferr;
	UINT32 head, track, sector;

	err = os9_locate_lsn(img, lsn, &head, &track, &sector);
	if (err)
		return err;

	ferr = floppy_write_sector(imgtool_floppy(img), head, track, sector, offset, buffer, buffer_len);
	if (ferr)
		return imgtool_floppy_error(ferr);

	return IMGTOOLERR_SUCCESS;
}



static UINT32 os9_lookup_lsn(imgtool_image *img,
	const struct os9_fileinfo *file_info, UINT32 *index)
{
	int i;
	UINT32 lsn;
	const struct os9_diskinfo *disk_info;

	disk_info = (const struct os9_diskinfo *) imgtool_floppy_extrabytes(img);
	lsn = *index / disk_info->sector_size;

	i = 0;
	while(lsn >= file_info->sector_map[i].count)
		lsn -= file_info->sector_map[i++].count;

	lsn = file_info->sector_map[i].lsn + lsn;
	*index %= disk_info->sector_size;
	return lsn;
}



static int os9_interpret_dirent(void *entry, char **filename, UINT32 *lsn, int *corrupt)
{
	int i;
	char *entry_b = (char *) entry;

	*filename = NULL;
	*lsn = 0;
	if (corrupt)
		*corrupt = FALSE;

	if (entry_b[28] != '\0')
	{
		if (corrupt)
			*corrupt = TRUE;
	}

	for (i = 0; (i < 28) && !(entry_b[i] & 0x80); i++)
		;
	entry_b[i] &= 0x7F;
	entry_b[i+1] = '\0';

	*lsn = pick_integer_be(entry, 29, 3);
	if (strcmp(entry_b, ".") && strcmp(entry_b, ".."))
		*filename = entry_b;
	return *filename != NULL;
}



static imgtoolerr_t os9_decode_file_header(imgtool_image *image,
	int lsn, struct os9_fileinfo *info)
{
	imgtoolerr_t err;
	UINT32 attributes, count;
	int max_entries, i;
	const struct os9_diskinfo *disk_info;
	UINT8 header[256];

	disk_info = (const struct os9_diskinfo *) imgtool_floppy_extrabytes(image);

	err = os9_read_lsn(image, lsn, 0, header, sizeof(header));
	if (err)
		return err;

	info->lsn				= lsn;
	attributes				= pick_integer_be(header, 0, 1);
	info->owner_id			= pick_integer_be(header, 1, 2);
	info->link_count		= pick_integer_be(header, 8, 1);
	info->file_size			= pick_integer_be(header, 9, 4);
	info->directory			= (attributes & 0x80) ? 1 : 0;
	info->non_sharable		= (attributes & 0x40) ? 1 : 0;
	info->public_execute	= (attributes & 0x20) ? 1 : 0;
	info->public_write		= (attributes & 0x10) ? 1 : 0;
	info->public_read		= (attributes & 0x08) ? 1 : 0;
	info->user_execute		= (attributes & 0x04) ? 1 : 0;
	info->user_write		= (attributes & 0x02) ? 1 : 0;
	info->user_read			= (attributes & 0x01) ? 1 : 0;

	if (info->directory && (info->file_size % 32 != 0))
		return IMGTOOLERR_CORRUPTIMAGE;

	/* read all sector map entries */
	max_entries = (disk_info->sector_size - 16) / 5;
	max_entries = MIN(max_entries, sizeof(info->sector_map) / sizeof(info->sector_map[0]) - 1);
	for (i = 0; i < max_entries; i++)
	{
		lsn = pick_integer_be(header, 16 + (i * 5) + 0, 3);
		count = pick_integer_be(header, 16 + (i * 5) + 3, 2);
		if (count <= 0)
			break;

		info->sector_map[i].lsn = lsn;
		info->sector_map[i].count = count;
	}
	info->sector_map[i].lsn = 0;
	info->sector_map[i].count = 0;
	return IMGTOOLERR_SUCCESS;
}



static imgtoolerr_t os9_allocate_lsn(imgtool_image *image, UINT32 *lsn)
{
	UINT32 i;
	struct os9_diskinfo *disk_info;
	UINT8 b, mask;

	disk_info = (struct os9_diskinfo *) imgtool_floppy_extrabytes(image);

	for (i = 0; i < disk_info->total_sectors; i++)
	{
		b = disk_info->allocation_bitmap[i / 8];
		mask = 1 << (7 - (i % 8));

		if ((b & mask) == 0)
		{
			disk_info->allocation_bitmap[i / 8] |= mask;
			*lsn = i;
			return os9_write_lsn(image, 1, 0, disk_info->allocation_bitmap, disk_info->allocation_bitmap_bytes);
		}
	}
	return IMGTOOLERR_NOSPACE;
}



static imgtoolerr_t os9_deallocate_lsn(imgtool_image *image, UINT32 lsn)
{
	UINT8 mask;
	struct os9_diskinfo *disk_info;

	disk_info = (struct os9_diskinfo *) imgtool_floppy_extrabytes(image);
	mask = 1 << (7 - (lsn % 8));
	disk_info->allocation_bitmap[lsn / 8] &= ~mask;
	return os9_write_lsn(image, 1, 0, disk_info->allocation_bitmap, disk_info->allocation_bitmap_bytes);
}



static UINT32 os9_get_free_lsns(imgtool_image *image)
{
	const struct os9_diskinfo *disk_info;
	UINT32 i, free_lsns;
	UINT8 b;

	disk_info = (const struct os9_diskinfo *) imgtool_floppy_extrabytes(image);
	free_lsns = 0;

	for (i = 0; i < disk_info->total_sectors; i++)
	{
		b = disk_info->allocation_bitmap[i / 8];
		b >>= (7 - (i % 8));
		free_lsns += ~b & 1;
	}

	return free_lsns;
}

	

static imgtoolerr_t os9_corrupt_file_error(const struct os9_fileinfo *file_info)
{
	imgtoolerr_t err;
	if (file_info->directory)
		err = IMGTOOLERR_CORRUPTDIR;
	else
		err = IMGTOOLERR_CORRUPTFILE;
	return err;
}



static imgtoolerr_t os9_set_file_size(imgtool_image *image,
	struct os9_fileinfo *file_info, UINT32 new_size)
{
	imgtoolerr_t err;
	const struct os9_diskinfo *disk_info;
	UINT32 new_lsn_count, current_lsn_count;
	UINT32 free_lsns, lsn, i;
	int sector_map_length = -1;
	UINT8 header[256];

	/* easy way out? */
	if (file_info->file_size == new_size)
		return IMGTOOLERR_SUCCESS;

	disk_info = (const struct os9_diskinfo *) imgtool_floppy_extrabytes(image);

	free_lsns = os9_get_free_lsns(image);
	current_lsn_count = (file_info->file_size + disk_info->sector_size - 1) / disk_info->sector_size;
	new_lsn_count = (new_size + disk_info->sector_size - 1) / disk_info->sector_size;

	/* check to see if the file is growing and we do not have enough space */
	if ((new_lsn_count > current_lsn_count) && (new_lsn_count - current_lsn_count > free_lsns))
		return IMGTOOLERR_NOSPACE;

	if (current_lsn_count != new_lsn_count)
	{
		/* first find out the size of our sector map */
		sector_map_length = 0;
		lsn = 0;
		while((lsn < current_lsn_count) && (sector_map_length < sizeof(file_info->sector_map) / sizeof(file_info->sector_map[0])))
		{
			if (file_info->sector_map[sector_map_length].count == 0)
				return os9_corrupt_file_error(file_info);

			lsn += file_info->sector_map[sector_map_length].count;
			sector_map_length++;
		}

		/* keep in mind that the sector_map might not parallel our expected
		 * current LSN count; we should tolerate this abnormality */
		current_lsn_count = lsn;

		while(current_lsn_count > new_lsn_count)
		{
			/* shrink this file */
			lsn = file_info->sector_map[sector_map_length - 1].lsn +
				file_info->sector_map[sector_map_length - 1].count - 1;

			err = os9_deallocate_lsn(image, lsn);
			if (err)
				return err;

			file_info->sector_map[sector_map_length - 1].count--;
			while(sector_map_length > 0 && (file_info->sector_map[sector_map_length - 1].count == 0))
				sector_map_length--;
			current_lsn_count--;
		}

		while(current_lsn_count < new_lsn_count)
		{
			/* grow this file */
			err = os9_allocate_lsn(image, &lsn);
			if (err)
				return err;

			if ((sector_map_length > 0) && ((file_info->sector_map[sector_map_length - 1].lsn +
				file_info->sector_map[sector_map_length - 1].count) == lsn))
			{
				file_info->sector_map[sector_map_length - 1].count++;
			}
			else if (sector_map_length >= sizeof(file_info->sector_map) / sizeof(file_info->sector_map[0]))
			{
				return IMGTOOLERR_NOSPACE;
			}
			else
			{
				file_info->sector_map[sector_map_length].lsn = lsn;
				file_info->sector_map[sector_map_length].count = 1;
				sector_map_length++;
				file_info->sector_map[sector_map_length].lsn = 0;
				file_info->sector_map[sector_map_length].count = 0;
			}
			current_lsn_count++;
		}
	}

	/* now lets write back the sector */
	err = os9_read_lsn(image, file_info->lsn, 0, header, sizeof(header));
	if (err)
		return err;

	file_info->file_size = new_size;
	place_integer_be(header, 9, 4, file_info->file_size);

	/* do we have to write the sector map? */
	if (sector_map_length >= 0)
	{
		for (i = 0; i < MIN(sector_map_length + 1, sizeof(file_info->sector_map) / sizeof(file_info->sector_map[0])); i++)
		{
			place_integer_be(header, 16 + (i * 5) + 0, 3, file_info->sector_map[i].lsn);
			place_integer_be(header, 16 + (i * 5) + 3, 2, file_info->sector_map[i].count);
		}
	}

	err = os9_write_lsn(image, file_info->lsn, 0, header, disk_info->sector_size);
	if (err)
		return err;

	return IMGTOOLERR_SUCCESS;
}



static imgtoolerr_t os9_lookup_path(imgtool_image *img, const char *path,
	creation_policy_t create, struct os9_fileinfo *file_info,
	UINT32 *parent_lsn, UINT32 *dirent_lsn, UINT32 *dirent_index)
{
	imgtoolerr_t err = IMGTOOLERR_SUCCESS;
	struct os9_fileinfo dir_info;
	UINT32 index, current_lsn, dir_size;
	UINT32 entry_index = 0;
	UINT32 entry_lsn = 0;
	UINT32 allocated_lsn = 0;
	UINT8 entry[32];
	UINT8 block[64];
	char *filename;
	const struct os9_diskinfo *disk_info;

	disk_info = (const struct os9_diskinfo *) imgtool_floppy_extrabytes(img);
	current_lsn = disk_info->root_dir_lsn;

	if (parent_lsn)
		*parent_lsn = 0;

	while(*path)
	{
		if (parent_lsn)
			*parent_lsn = current_lsn;

		err = os9_decode_file_header(img, current_lsn, &dir_info);
		if (err)
			goto done;
		dir_size = dir_info.file_size;

		/* sanity check directory */
		if (!dir_info.directory)
		{
			err = (current_lsn == disk_info->root_dir_lsn) ? IMGTOOLERR_CORRUPTIMAGE : IMGTOOLERR_INVALIDPATH;
			goto done;
		}

		for (index = 0; index < dir_size; index += 32)
		{
			entry_index = index;
			entry_lsn = os9_lookup_lsn(img, &dir_info, &entry_index);
			
			err = os9_read_lsn(img, entry_lsn, entry_index, entry, sizeof(entry));
			if (err)
				goto done;

			if (os9_interpret_dirent(entry, &filename, &current_lsn, NULL))
			{
				if (!strcmp(path, filename))
					break;
			}
		}

		/* at the end of the file? */
		if (index >= dir_size)
		{
			if (!create || path[strlen(path) + 1])
			{
				err = IMGTOOLERR_PATHNOTFOUND;
				goto done;
			}

			err = os9_allocate_lsn(img, &allocated_lsn);
			if (err)
				goto done;
			
			/* write the file */
			memset(block, 0, sizeof(block));
			place_integer_be(block, 0, 1, 0x1B | ((create == CREATE_DIR) ? 0x80 : 0x00));
			err = os9_write_lsn(img, allocated_lsn, 0, block, sizeof(block));
			if (err)
				goto done;

			/* expand the directory to hold the new entry */
			err = os9_set_file_size(img, &dir_info, dir_size + 32);
			if (err)
				goto done;

			/* now prepare the directory entry */
			memset(entry, 0, sizeof(entry));
			place_string(entry, 0, 28, path);
			place_integer_be(entry, 29, 3, allocated_lsn);

			/* now write the directory entry */
			entry_index = dir_size;
			entry_lsn = os9_lookup_lsn(img, &dir_info, &entry_index);
			err = os9_write_lsn(img, entry_lsn, entry_index, entry, 32);
			if (err)
				goto done;

			/* directory entry append complete; no need to hold this lsn */
			current_lsn = allocated_lsn;
			allocated_lsn = 0;
		}
		path += strlen(path) + 1;
	}

	if (file_info)
	{
		err = os9_decode_file_header(img, current_lsn, file_info);
		if (err)
			goto done;
	}

	if (dirent_lsn)
		*dirent_lsn = entry_lsn;
	if (dirent_index)
		*dirent_index = entry_index;

done:
	if (allocated_lsn != 0)
		os9_deallocate_lsn(img, allocated_lsn);
	return err;
}



static imgtoolerr_t os9_diskimage_open(imgtool_image *image, imgtool_stream *stream)
{
	imgtoolerr_t err;
	floperr_t ferr;
	struct os9_diskinfo *info;
	UINT32 track_size_in_sectors, attributes, i;
	UINT8 header[256];
	UINT32 allocation_bitmap_lsns;
	UINT8 b, mask;

	info = (struct os9_diskinfo *) imgtool_floppy_extrabytes(image);

	ferr = floppy_read_sector(imgtool_floppy(image), 0, 0, 1, 0, header, sizeof(header));
	if (ferr)
		return imgtool_floppy_error(ferr);

	info->total_sectors				= pick_integer_be(header,   0, 3);
	track_size_in_sectors			= pick_integer_be(header,   3, 1);
	info->allocation_bitmap_bytes	= pick_integer_be(header,   4, 2);
	info->cluster_size				= pick_integer_be(header,   6, 2);
	info->root_dir_lsn				= pick_integer_be(header,   8, 3);
	info->owner_id					= pick_integer_be(header,  11, 2);
	attributes						= pick_integer_be(header,  13, 1);
	info->disk_id					= pick_integer_be(header,  14, 2);
	info->format_flags				= pick_integer_be(header,  16, 1);
	info->sectors_per_track			= pick_integer_be(header,  17, 2);
	info->bootstrap_lsn				= pick_integer_be(header,  21, 3);
	info->bootstrap_size			= pick_integer_be(header,  24, 2);
	info->sector_size				= pick_integer_be(header, 104, 2);

	info->sides					= (info->format_flags & 0x01) ? 2 : 1;
	info->double_density		= (info->format_flags & 0x02) ? 1 : 0;
	info->double_track			= (info->format_flags & 0x04) ? 1 : 0;
	info->quad_track_density	= (info->format_flags & 0x08) ? 1 : 0;
	info->octal_track_density	= (info->format_flags & 0x10) ? 1 : 0;

	pick_string(header, 31, 32, info->name);

	if (info->sector_size == 0)
		info->sector_size = 256;

	/* does the root directory and allocation bitmap collide? */
	allocation_bitmap_lsns = (info->allocation_bitmap_bytes + info->sector_size - 1) / info->sector_size;
	if (1 + allocation_bitmap_lsns > info->root_dir_lsn)
		return IMGTOOLERR_CORRUPTIMAGE;

	/* is the allocation bitmap too big? */
	info->allocation_bitmap = img_malloc(image, info->allocation_bitmap_bytes);
	if (!info->allocation_bitmap)
		return IMGTOOLERR_OUTOFMEMORY;
	memset(info->allocation_bitmap, 0, info->allocation_bitmap_bytes);

	/* sectors per track and track size dont jive? */
	if (info->sectors_per_track != track_size_in_sectors)
		return IMGTOOLERR_CORRUPTIMAGE;

	/* zero sectors per track? */
	if (info->sectors_per_track == 0)
		return IMGTOOLERR_CORRUPTIMAGE;

	/* do we have an odd number of sectors? */
	if (info->total_sectors % info->sectors_per_track)
		return IMGTOOLERR_CORRUPTIMAGE;

	/* read the allocation bitmap */
	for (i = 0; i < allocation_bitmap_lsns; i++)
	{
		err = os9_read_lsn(image, 1 + i, 0, &info->allocation_bitmap[i * info->sector_size],
			MIN(info->allocation_bitmap_bytes - (i * info->sector_size), info->sector_size));
		if (err)
			return err;
	}

	/* check to make sure that the allocation bitmap and root sector are reserved */
	for (i = 0; i <= allocation_bitmap_lsns; i++)
	{
		b = info->allocation_bitmap[i / 8];
		mask = 1 << (7 - (i % 8));
		if ((b & mask) == 0)
			return IMGTOOLERR_CORRUPTIMAGE;
	}

	return IMGTOOLERR_SUCCESS;
}



static imgtoolerr_t os9_diskimage_create(imgtool_image *img, imgtool_stream *stream, option_resolution *opts)
{
	imgtoolerr_t err;
	UINT8 *header;
	UINT32 heads, tracks, sectors, sector_bytes, first_sector_id;
	UINT32 cluster_size, owner_id;
	UINT32 allocation_bitmap_bits, allocation_bitmap_lsns;
	UINT32 attributes, format_flags, disk_id;
	UINT32 i;
	const char *title;
	time_t t;
	struct tm *ltime;

	time(&t);
	ltime = localtime(&t);

	heads = option_resolution_lookup_int(opts, 'H');
	tracks = option_resolution_lookup_int(opts, 'T');
	sectors = option_resolution_lookup_int(opts, 'S');
	sector_bytes = option_resolution_lookup_int(opts, 'L');
	first_sector_id = option_resolution_lookup_int(opts, 'F');
	title = "";

	header = malloc(sector_bytes);
	if (!header)
	{
		err = IMGTOOLERR_OUTOFMEMORY;
		goto done;
	}

	if (sector_bytes > 256)
		sector_bytes = 256;
	cluster_size = 1;
	owner_id = 1;
	disk_id = 1;
	attributes = 0;
	allocation_bitmap_bits = heads * tracks * sectors / cluster_size;
	allocation_bitmap_lsns = (allocation_bitmap_bits / 8 + sector_bytes - 1) / sector_bytes;
	format_flags = ((heads > 1) ? 0x01 : 0x00) | ((tracks > 40) ? 0x02 : 0x00);

	memset(header, 0, sector_bytes);
	place_integer_be(header,   0,  3, heads * tracks * sectors);
	place_integer_be(header,   3,  1, sectors);
	place_integer_be(header,   4,  2, (allocation_bitmap_bits + 7) / 8);
	place_integer_be(header,   6,  2, cluster_size);
	place_integer_be(header,   8,  3, 1 + allocation_bitmap_lsns);
	place_integer_be(header,  11,  2, owner_id);
	place_integer_be(header,  13,  1, attributes);
	place_integer_be(header,  14,  2, disk_id);
	place_integer_be(header,  16,  1, format_flags);
	place_integer_be(header,  17,  2, sectors);
	place_string(header,   31, 32, title);
	place_integer_be(header, 100,  4, 1);
	place_integer_be(header, 104,  4, (sector_bytes == 256) ? 0 : sector_bytes);

	err = floppy_write_sector(imgtool_floppy(img), 0, 0, first_sector_id, 0, header, sector_bytes);
	if (err)
		goto done;

	memset(header, 0, sector_bytes);
	for (i = 0; i < allocation_bitmap_lsns; i++)
	{
		if (i == 0)
		{
			header[0] = 0xFF;
			header[1] = 0xC0;
		}
		else if (i == 1)
		{
			header[0] = 0x00;
			header[1] = 0x00;
		}

		err = floppy_write_sector(imgtool_floppy(img), 0, 0, first_sector_id + 1 + i, 0, header, sector_bytes);
		if (err)
			goto done;
	}

	memset(header, 0, sector_bytes);
	header[0x00] = 0xBF;
	header[0x01] = 0x00;
	header[0x02] = 0x00;
	header[0x03] = (UINT8) ltime->tm_year;
	header[0x04] = (UINT8) ltime->tm_mon + 1;
	header[0x05] = (UINT8) ltime->tm_mday;
	header[0x06] = (UINT8) ltime->tm_hour;
	header[0x07] = (UINT8) ltime->tm_min;
	header[0x08] = 0x02;
	header[0x09] = 0x00;
	header[0x0A] = 0x00;
	header[0x0B] = 0x00;
	header[0x0C] = 0x40;
	header[0x0D] = (UINT8) (ltime->tm_year % 100);
	header[0x0E] = (UINT8) ltime->tm_mon;
	header[0x0F] = (UINT8) ltime->tm_mday;
	header[0x10] = 0x00;
	header[0x11] = 0x00;
	header[0x12] = 0x03;
	header[0x13] = 0x00;
	header[0x14] = 0x07;
	err = floppy_write_sector(imgtool_floppy(img), 0, 0, first_sector_id + 1 + allocation_bitmap_lsns, 0, header, sector_bytes);
	if (err)
		goto done;

	memset(header, 0, sector_bytes);
	header[0x00] = 0x2E;
	header[0x01] = 0xAE;
	header[0x1F] = 1 + allocation_bitmap_lsns;
	header[0x20] = 0xAE;
	header[0x3F] = 1 + allocation_bitmap_lsns;
	err = floppy_write_sector(imgtool_floppy(img), 0, 0, first_sector_id + 2 + allocation_bitmap_lsns, 0, header, sector_bytes);
	if (err)
		goto done;

done:
	if (header)
		free(header);
	return err;
}



static imgtoolerr_t os9_diskimage_beginenum(imgtool_imageenum *enumeration, const char *path)
{
	imgtoolerr_t err = IMGTOOLERR_SUCCESS;
	struct os9_direnum *os9enum;
	imgtool_image *image;

	image = img_enum_image(enumeration);
	os9enum = (struct os9_direnum *) img_enum_extrabytes(enumeration);

	err = os9_lookup_path(image, path, CREATE_NONE, &os9enum->dir_info, NULL, NULL, NULL);
	if (err)
		goto done;

	/* this had better be a directory */
	if (!os9enum->dir_info.directory)
	{
		err = IMGTOOLERR_CORRUPTIMAGE;
		goto done;
	}

done:
	return err;
}



static imgtoolerr_t os9_diskimage_nextenum(imgtool_imageenum *enumeration, imgtool_dirent *ent)
{
	struct os9_direnum *os9enum;
	UINT32 lsn, index;
	imgtoolerr_t err;
	UINT8 dir_entry[32];
	char filename[29];
	struct os9_fileinfo file_info;
	imgtool_image *image;

	image = img_enum_image(enumeration);
	os9enum = (struct os9_direnum *) img_enum_extrabytes(enumeration);

	do
	{
		/* check for EOF */
		if (os9enum->index >= os9enum->dir_info.file_size)
		{
			ent->eof = 1;
			return IMGTOOLERR_SUCCESS;
		}

		index = os9enum->index;
		lsn = os9_lookup_lsn(image, &os9enum->dir_info, &index);
		os9enum->index += 32;

		err = os9_read_lsn(image, lsn, index, dir_entry, sizeof(dir_entry));
		if (err)
			return err;

		if (dir_entry[0])
			pick_string(dir_entry, 0, 28, filename);
		else
			filename[0] = '\0';
	}
	while(!filename[0] || !strcmp(filename, ".") || !strcmp(filename, ".."));

	/* read file attributes */
	lsn = pick_integer_be(dir_entry, 29, 3);
	err = os9_decode_file_header(img_enum_image(enumeration), lsn, &file_info);
	if (err)
		return err;

	/* fill out imgtool_dirent structure */
	snprintf(ent->filename, sizeof(ent->filename) / sizeof(ent->filename[0]), "%s", filename);
	snprintf(ent->attr, sizeof(ent->attr) / sizeof(ent->attr[0]), "%c%c%c%c%c%c%c%c", 
		file_info.directory      ? 'd' : '-',
		file_info.non_sharable   ? 's' : '-',
		file_info.public_execute ? 'x' : '-',
		file_info.public_write   ? 'w' : '-',
		file_info.public_read    ? 'r' : '-',
		file_info.user_execute   ? 'x' : '-',
		file_info.user_write     ? 'w' : '-',
		file_info.user_read      ? 'r' : '-');

	ent->directory = file_info.directory;
	ent->corrupt = (dir_entry[28] != 0);
	ent->filesize = file_info.file_size;
	return IMGTOOLERR_SUCCESS;
}



static imgtoolerr_t os9_diskimage_freespace(imgtool_image *img, UINT64 *size)
{
	const struct os9_diskinfo *disk_info;
	UINT32 free_lsns;

	disk_info = (const struct os9_diskinfo *) imgtool_floppy_extrabytes(img);
	free_lsns = os9_get_free_lsns(img);

	*size = free_lsns * disk_info->sector_size;
	return IMGTOOLERR_SUCCESS;
}



static imgtoolerr_t os9_diskimage_readfile(imgtool_image *img, const char *filename, const char *fork, imgtool_stream *destf)
{
	imgtoolerr_t err;
	const struct os9_diskinfo *disk_info;
	struct os9_fileinfo file_info;
	UINT8 buffer[256];
	int i, j;
	UINT32 file_size;
	UINT32 used_size;

	disk_info = (const struct os9_diskinfo *) imgtool_floppy_extrabytes(img);

	err = os9_lookup_path(img, filename, CREATE_NONE, &file_info, NULL, NULL, NULL);
	if (err)
		return err;
	if (file_info.directory)
		return IMGTOOLERR_FILENOTFOUND;
	file_size = file_info.file_size;

	for (i = 0; file_info.sector_map[i].count > 0; i++)
	{
		for (j = 0; j < file_info.sector_map[i].count; j++)
		{
			used_size = MIN(file_size, disk_info->sector_size);
			err = os9_read_lsn(img, file_info.sector_map[i].lsn + j, 0,
				buffer, used_size);
			if (err)
				return err;
			stream_write(destf, buffer, used_size);
			file_size -= used_size;
		}
	}
	return IMGTOOLERR_SUCCESS;
}



static imgtoolerr_t os9_diskimage_writefile(imgtool_image *image, const char *path, const char *fork, imgtool_stream *sourcef, option_resolution *opts)
{
	imgtoolerr_t err;
	struct os9_fileinfo file_info;
	size_t write_size;
	void *buf = NULL;
	int i = -1;
	UINT32 lsn = 0;
	UINT32 count = 0;
	UINT32 sz;
	const struct os9_diskinfo *disk_info;

	disk_info = (const struct os9_diskinfo *) imgtool_floppy_extrabytes(image);

	buf = malloc(disk_info->sector_size);
	if (!buf)
	{
		err = IMGTOOLERR_OUTOFMEMORY;
		goto done;
	}

	err = os9_lookup_path(image, path, CREATE_FILE, &file_info, NULL, NULL, NULL);
	if (err)
		goto done;

	sz = (UINT32) stream_size(sourcef);

	err = os9_set_file_size(image, &file_info, sz);
	if (err)
		goto done;

	while(sz > 0)
	{
		write_size = (size_t) MIN(sz, (UINT64) disk_info->sector_size);

		stream_read(sourcef, buf, write_size);

		while(count == 0)
		{
			i++;
			lsn = file_info.sector_map[i].lsn;
			count = file_info.sector_map[i].count;
		}

		err = os9_write_lsn(image, lsn, 0, buf, write_size);
		if (err)
			goto done;

		lsn++;
		count--;
		sz -= write_size;
	}

done:
	if (buf)
		free(buf);
	return err;
}



static imgtoolerr_t os9_diskimage_delete(imgtool_image *image, const char *path,
	unsigned int delete_directory)
{
	imgtoolerr_t err;
	const struct os9_diskinfo *disk_info;
	struct os9_fileinfo file_info;
	UINT32 dirent_lsn, dirent_index;
	UINT32 entry_lsn, entry_index;
	UINT32 i, j, lsn;
	UINT8 b;

	disk_info = (const struct os9_diskinfo *) imgtool_floppy_extrabytes(image);

	err = os9_lookup_path(image, path, CREATE_NONE, &file_info, NULL, &dirent_lsn, &dirent_index);
	if (err)
		return err;
	if (file_info.directory != delete_directory)
		return IMGTOOLERR_FILENOTFOUND;

	/* make sure that if we are deleting a directory, it is empty */
	if (delete_directory)
	{
		for (i = 64; i < file_info.file_size; i += 32)
		{
			entry_index = i;
			entry_lsn = os9_lookup_lsn(image, &file_info, &entry_index);

			err = os9_read_lsn(image, entry_lsn, entry_index, &b, 1);
			if (err)
				return err;

			/* this had better be a deleted file, if not we can't delete */
			if (b != 0)
				return IMGTOOLERR_DIRNOTEMPTY;
		}
	}

	/* zero out the file entry */
	b = '\0';
	err = os9_write_lsn(image, dirent_lsn, dirent_index, &b, 1);
	if (err)
		return err;

	/* get the link count */
	err = os9_read_lsn(image, file_info.lsn, 8, &b, 1);
	if (err)
		return err;

	if (b > 0)
		b--;
	if (b > 0)
	{
		/* link count is greater than zero */
		err = os9_write_lsn(image, file_info.lsn, 8, &b, 1);
		if (err)
			return err;
	}
	else
	{
		/* no more links; outright delete the file */
		err = os9_deallocate_lsn(image, file_info.lsn);
		if (err)
			return err;

		for (i = 0; (i < sizeof(file_info.sector_map) / sizeof(file_info.sector_map[0])) && file_info.sector_map[i].count; i++)
		{
			lsn = file_info.sector_map[i].lsn;
			for (j = 0;  j < file_info.sector_map[i].count; j++)
			{
				err = os9_deallocate_lsn(image, lsn + j);
				if (err)
					return err;
			}
		}
	}

	return IMGTOOLERR_SUCCESS;
}



static imgtoolerr_t os9_diskimage_deletefile(imgtool_image *image, const char *path)
{
	return os9_diskimage_delete(image, path, 0);
}



static imgtoolerr_t os9_diskimage_createdir(imgtool_image *image, const char *path)
{
	imgtoolerr_t err;
	struct os9_fileinfo file_info;
	UINT8 dir_data[64];
	UINT32 parent_lsn;

	err = os9_lookup_path(image, path, CREATE_DIR, &file_info, &parent_lsn, NULL, NULL);
	if (err)
		goto done;

	err = os9_set_file_size(image, &file_info, 64);
	if (err)
		goto done;

	memset(dir_data, 0, sizeof(dir_data));
	place_string(dir_data,   0, 32, ".");
	place_integer_be(dir_data, 29,  3, file_info.lsn);
	place_string(dir_data,  32, 32, "..");
	place_integer_be(dir_data, 29,  3, parent_lsn);

	err = os9_write_lsn(image, file_info.sector_map[0].lsn, 0, dir_data, sizeof(dir_data));
	if (err)
		goto done;

done:
	return err;
}



static imgtoolerr_t os9_diskimage_deletedir(imgtool_image *image, const char *path)
{
	return os9_diskimage_delete(image, path, 1);
}



static void coco_os9_module_populate(UINT32 state, union imgtoolinfo *info)
{
	switch(state)
	{
		case IMGTOOLINFO_INT_INITIAL_PATH_SEPARATOR:		info->i = 1; break;
		case IMGTOOLINFO_INT_OPEN_IS_STRICT:				info->i = 1; break;
		case IMGTOOLINFO_INT_IMAGE_EXTRA_BYTES:				info->i = sizeof(struct os9_diskinfo); break;
		case IMGTOOLINFO_INT_ENUM_EXTRA_BYTES:				info->i = sizeof(struct os9_direnum); break;
		case IMGTOOLINFO_STR_EOLN:							strcpy(info->s = imgtool_temp_str(), "\r"); break;
		case IMGTOOLINFO_INT_PATH_SEPARATOR:				info->i = '/'; break;
		case IMGTOOLINFO_PTR_CREATE:						info->create = os9_diskimage_create; break;
		case IMGTOOLINFO_PTR_OPEN:							info->open = os9_diskimage_open; break;
		case IMGTOOLINFO_PTR_BEGIN_ENUM:					info->begin_enum = os9_diskimage_beginenum; break;
		case IMGTOOLINFO_PTR_NEXT_ENUM:						info->next_enum = os9_diskimage_nextenum; break;
		case IMGTOOLINFO_PTR_FREE_SPACE:					info->free_space = os9_diskimage_freespace; break;
		case IMGTOOLINFO_PTR_READ_FILE:						info->read_file = os9_diskimage_readfile; break;
		case IMGTOOLINFO_PTR_WRITE_FILE:					info->write_file = os9_diskimage_writefile; break;
		case IMGTOOLINFO_PTR_DELETE_FILE:					info->delete_file = os9_diskimage_deletefile; break;
		case IMGTOOLINFO_PTR_CREATE_DIR:					info->create_dir = os9_diskimage_createdir; break;
		case IMGTOOLINFO_PTR_DELETE_DIR:					info->delete_dir = os9_diskimage_deletedir; break;
	}
}



FLOPPYMODULE(os9, "OS-9 format", coco, coco_os9_module_populate)
