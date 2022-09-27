/***************************************************************************

	stream.h

	Code for implementing Imgtool streams

***************************************************************************/

#ifndef STREAM_H
#define STREAM_H

#include "imgterrs.h"
#include "mamecore.h"

typedef struct _imgtool_stream imgtool_stream;

imgtool_stream *stream_open(const char *fname, int read_or_write);	/* similar params to mame_fopen */
imgtool_stream *stream_open_write_stream(int filesize);
imgtool_stream *stream_open_mem(void *buf, size_t sz);
void stream_close(imgtool_stream *stream);
size_t stream_read(imgtool_stream *stream, void *buf, size_t sz);
size_t stream_write(imgtool_stream *stream, const void *buf, size_t sz);
UINT64 stream_size(imgtool_stream *stream);
int stream_seek(imgtool_stream *stream, INT64 pos, int where);
size_t stream_tell(imgtool_stream *stream);
void stream_clear(imgtool_stream *stream);
void *stream_getptr(imgtool_stream *stream);
size_t stream_putc(imgtool_stream *stream, char c);
size_t stream_puts(imgtool_stream *stream, const char *s);
size_t stream_printf(imgtool_stream *stream, const char *fmt, ...) ATTR_PRINTF(2,3);

/* Transfers sz bytes from source to dest */
size_t stream_transfer(imgtool_stream *dest, imgtool_stream *source, size_t sz);
size_t stream_transfer_all(imgtool_stream *dest, imgtool_stream *source);

/* Fills sz bytes with b */
size_t stream_fill(imgtool_stream *f, unsigned char b, UINT64 sz);

/* Returns the CRC of a file */
int stream_crc(imgtool_stream *f, unsigned long *result);
int file_crc(const char *fname,  unsigned long *result);

/* Returns whether a stream is read only or not */
int stream_isreadonly(imgtool_stream *f);

#endif /* STREAM_H */
