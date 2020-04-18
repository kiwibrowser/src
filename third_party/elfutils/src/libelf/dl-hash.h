/* Compute hash value for given string according to ELF standard.
   Copyright (C) 2006 Red Hat, Inc.
   This file is part of elfutils.
   Written by Ulrich Drepper <drepper@redhat.com>, 1995.

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

#ifndef _DL_HASH_H
#define _DL_HASH_H	1


/* This is the hashing function specified by the ELF ABI.  In the
   first five operations no overflow is possible so we optimized it a
   bit.  */
static inline unsigned int
__attribute__ ((__pure__))
_dl_elf_hash (const char *name)
{
  const unsigned char *iname = (const unsigned char *) name;
  unsigned int hash = (unsigned int) *iname++;
  if (*iname != '\0')
    {
      hash = (hash << 4) + (unsigned int) *iname++;
      if (*iname != '\0')
	{
	  hash = (hash << 4) + (unsigned int) *iname++;
	  if (*iname != '\0')
	    {
	      hash = (hash << 4) + (unsigned int) *iname++;
	      if (*iname != '\0')
		{
		  hash = (hash << 4) + (unsigned int) *iname++;
		  while (*iname != '\0')
		    {
		      unsigned int hi;
		      hash = (hash << 4) + (unsigned int) *iname++;
		      hi = hash & 0xf0000000;

		      /* The algorithm specified in the ELF ABI is as
			 follows:

			 if (hi != 0)
			 hash ^= hi >> 24;

			 hash &= ~hi;

			 But the following is equivalent and a lot
			 faster, especially on modern processors.  */

		      hash ^= hi;
		      hash ^= hi >> 24;
		    }
		}
	    }
	}
    }
  return hash;
}

#endif /* dl-hash.h */
