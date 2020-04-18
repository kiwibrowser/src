/* Decompression support for libdwfl: zlib (gzip) and/or bzlib (bzip2).
   Copyright (C) 2009 Red Hat, Inc.
   This file is part of elfutils.

   This file is free software; you can redistribute it and/or modify
   it under the terms of either

     * the GNU Lesser General Public License as published by the Free
       Software Foundation; either version 3 of the License, or (at
       your option) any later version

   or

     * the GNU General Public License as published by the Free
       Software Foundation; either version 2 of the License, or (at
       your option) any later version

   or both in parallel, as here.

   elfutils is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.

   You should have received copies of the GNU General Public License and
   the GNU Lesser General Public License along with this program.  If
   not, see <http://www.gnu.org/licenses/>.  */

#include "libdwflP.h"
#include "system.h"

#include <unistd.h>

#ifdef LZMA
# define USE_INFLATE	1
# include <lzma.h>
# define unzip		__libdw_unlzma
# define DWFL_E_ZLIB	DWFL_E_LZMA
# define MAGIC		"\xFD" "7zXZ\0" /* XZ file format.  */
# define MAGIC2		"\x5d\0"	/* Raw LZMA format.  */
# define Z(what)	LZMA_##what
# define LZMA_ERRNO	LZMA_PROG_ERROR
# define z_stream	lzma_stream
# define inflateInit(z)	lzma_auto_decoder (z, 1 << 30, 0)
# define do_inflate(z)	lzma_code (z, LZMA_RUN)
# define inflateEnd(z)	lzma_end (z)
#elif defined BZLIB
# define USE_INFLATE	1
# include <bzlib.h>
# define unzip		__libdw_bunzip2
# define DWFL_E_ZLIB	DWFL_E_BZLIB
# define MAGIC		"BZh"
# define Z(what)	BZ_##what
# define BZ_ERRNO	BZ_IO_ERROR
# define z_stream	bz_stream
# define inflateInit(z)	BZ2_bzDecompressInit (z, 0, 0)
# define do_inflate(z)	BZ2_bzDecompress (z)
# define inflateEnd(z)	BZ2_bzDecompressEnd (z)
#else
# define USE_INFLATE	0
# define crc32		loser_crc32
# include <zlib.h>
# define unzip		__libdw_gunzip
# define MAGIC		"\037\213"
# define Z(what)	Z_##what
#endif

#define READ_SIZE		(1 << 20)

/* If this is not a compressed image, return DWFL_E_BADELF.
   If we uncompressed it into *WHOLE, *WHOLE_SIZE, return DWFL_E_NOERROR.
   Otherwise return an error for bad compressed data or I/O failure.
   If we return an error after reading the first part of the file,
   leave that portion malloc'd in *WHOLE, *WHOLE_SIZE.  If *WHOLE
   is not null on entry, we'll use it in lieu of repeating a read.  */

Dwfl_Error internal_function
unzip (int fd, off64_t start_offset,
       void *mapped, size_t mapped_size,
       void **whole, size_t *whole_size)
{
  void *buffer = NULL;
  size_t size = 0;
  inline bool bigger_buffer (size_t start)
  {
    size_t more = size ? size * 2 : start;
    char *b = realloc (buffer, more);
    while (unlikely (b == NULL) && more >= size + 1024)
      b = realloc (buffer, more -= 1024);
    if (unlikely (b == NULL))
      return false;
    buffer = b;
    size = more;
    return true;
  }
  inline void smaller_buffer (size_t end)
  {
    buffer = realloc (buffer, end) ?: end == 0 ? NULL : buffer;
    size = end;
  }

  void *input_buffer = NULL;
  off_t input_pos = 0;

  inline Dwfl_Error fail (Dwfl_Error failure)
  {
    if (input_pos == (off_t) mapped_size)
      *whole = input_buffer;
    else
      {
	free (input_buffer);
	*whole = NULL;
      }
    free (buffer);
    return failure;
  }

  inline Dwfl_Error zlib_fail (int result)
  {
    switch (result)
      {
      case Z (MEM_ERROR):
	return fail (DWFL_E_NOMEM);
      case Z (ERRNO):
	return fail (DWFL_E_ERRNO);
      default:
	return fail (DWFL_E_ZLIB);
      }
  }

  if (mapped == NULL)
    {
      if (*whole == NULL)
	{
	  input_buffer = malloc (READ_SIZE);
	  if (unlikely (input_buffer == NULL))
	    return DWFL_E_NOMEM;

	  ssize_t n = pread_retry (fd, input_buffer, READ_SIZE, start_offset);
	  if (unlikely (n < 0))
	    return zlib_fail (Z (ERRNO));

	  input_pos = n;
	  mapped = input_buffer;
	  mapped_size = n;
	}
      else
	{
	  input_buffer = *whole;
	  input_pos = mapped_size = *whole_size;
	}
    }

#define NOMAGIC(magic) \
  (mapped_size <= sizeof magic || memcmp (mapped, magic, sizeof magic - 1))

  /* First, look at the header.  */
  if (NOMAGIC (MAGIC)
#ifdef MAGIC2
      && NOMAGIC (MAGIC2)
#endif
      )
    /* Not a compressed file.  */
    return DWFL_E_BADELF;

#if USE_INFLATE

  /* This style actually only works with bzlib and liblzma.
     The stupid zlib interface has nothing to grok the
     gzip file headers except the slow gzFile interface.  */

  z_stream z = { .next_in = mapped, .avail_in = mapped_size };
  int result = inflateInit (&z);
  if (result != Z (OK))
    {
      inflateEnd (&z);
      return zlib_fail (result);
    }

  do
    {
      if (z.avail_in == 0 && input_buffer != NULL)
	{
	  ssize_t n = pread_retry (fd, input_buffer, READ_SIZE,
				   start_offset + input_pos);
	  if (unlikely (n < 0))
	    {
	      inflateEnd (&z);
	      return zlib_fail (Z (ERRNO));
	    }
	  z.next_in = input_buffer;
	  z.avail_in = n;
	  input_pos += n;
	}
      if (z.avail_out == 0)
	{
	  ptrdiff_t pos = (void *) z.next_out - buffer;
	  if (!bigger_buffer (z.avail_in))
	    {
	      result = Z (MEM_ERROR);
	      break;
	    }
	  z.next_out = buffer + pos;
	  z.avail_out = size - pos;
	}
    }
  while ((result = do_inflate (&z)) == Z (OK));

#ifdef BZLIB
  uint64_t total_out = (((uint64_t) z.total_out_hi32 << 32)
			| z.total_out_lo32);
  smaller_buffer (total_out);
#else
  smaller_buffer (z.total_out);
#endif

  inflateEnd (&z);

  if (result != Z (STREAM_END))
    return zlib_fail (result);

#else  /* gzip only.  */

  /* Let the decompression library read the file directly.  */

  gzFile zf;
  Dwfl_Error open_stream (void)
  {
    int d = dup (fd);
    if (unlikely (d < 0))
      return DWFL_E_BADELF;
    if (start_offset != 0)
      {
	off64_t off = lseek (d, start_offset, SEEK_SET);
	if (off != start_offset)
	  {
	    close (d);
	    return DWFL_E_BADELF;
	  }
      }
    zf = gzdopen (d, "r");
    if (unlikely (zf == NULL))
      {
	close (d);
	return zlib_fail (Z (MEM_ERROR));
      }

    /* From here on, zlib will close D.  */

    return DWFL_E_NOERROR;
  }

  Dwfl_Error result = open_stream ();

  if (result == DWFL_E_NOERROR && gzdirect (zf))
    {
      gzclose (zf);
      return fail (DWFL_E_BADELF);
    }

  if (result != DWFL_E_NOERROR)
    return fail (result);

  ptrdiff_t pos = 0;
  while (1)
    {
      if (!bigger_buffer (1024))
	{
	  gzclose (zf);
	  return zlib_fail (Z (MEM_ERROR));
	}
      int n = gzread (zf, buffer + pos, size - pos);
      if (n < 0)
	{
	  int code;
	  gzerror (zf, &code);
	  gzclose (zf);
	  return zlib_fail (code);
	}
      if (n == 0)
	break;
      pos += n;
    }

  gzclose (zf);
  smaller_buffer (pos);
#endif

  free (input_buffer);

  *whole = buffer;
  *whole_size = size;

  return DWFL_E_NOERROR;
}
