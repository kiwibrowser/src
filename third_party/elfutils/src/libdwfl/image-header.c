/* Linux kernel image support for libdwfl.
   Copyright (C) 2009-2011 Red Hat, Inc.
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
#include <endian.h>

#if BYTE_ORDER == LITTLE_ENDIAN
# define LE16(x)	(x)
#else
# define LE16(x)	bswap_16 (x)
#endif

/* See Documentation/x86/boot.txt in Linux kernel sources
   for an explanation of these format details.  */

#define MAGIC1			0xaa55
#define MAGIC2			0x53726448 /* "HdrS" little-endian */
#define MIN_VERSION		0x0208

#define H_START			(H_SETUP_SECTS & -4)
#define H_SETUP_SECTS		0x1f1
#define H_MAGIC1		0x1fe
#define H_MAGIC2		0x202
#define H_VERSION		0x206
#define H_PAYLOAD_OFFSET	0x248
#define H_PAYLOAD_LENGTH	0x24c
#define H_END			0x250
#define H_READ_SIZE		(H_END - H_START)

Dwfl_Error
internal_function
__libdw_image_header (int fd, off64_t *start_offset,
		      void *mapped, size_t mapped_size)
{
  if (likely (mapped_size > H_END))
    {
      const void *header = mapped;
      char header_buffer[H_READ_SIZE];
      if (header == NULL)
	{
	  ssize_t n = pread_retry (fd, header_buffer, H_READ_SIZE,
				   *start_offset + H_START);
	  if (n < 0)
	    return DWFL_E_ERRNO;
	  if (n < H_READ_SIZE)
	    return DWFL_E_BADELF;

	  header = header_buffer - H_START;
	}

      if (*(uint16_t *) (header + H_MAGIC1) == LE16 (MAGIC1)
	  && *(uint32_t *) (header + H_MAGIC2) == LE32 (MAGIC2)
	  && LE16 (*(uint16_t *) (header + H_VERSION)) >= MIN_VERSION)
	{
	  /* The magic numbers match and the version field is sufficient.
	     Extract the payload bounds.  */

	  uint32_t offset = LE32 (*(uint32_t *) (header + H_PAYLOAD_OFFSET));
	  uint32_t length = LE32 (*(uint32_t *) (header + H_PAYLOAD_LENGTH));

	  offset += ((*(uint8_t *) (header + H_SETUP_SECTS) ?: 4) + 1) * 512;

	  if (offset > H_END && offset < mapped_size
	      && mapped_size - offset >= length)
	    {
	      /* It looks kosher.  Use it!  */
	      *start_offset += offset;
	      return DWFL_E_NOERROR;
	    }
	}
    }
  return DWFL_E_BADELF;
}
