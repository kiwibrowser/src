/* Return source file information of CU.
   Copyright (C) 2004, 2005, 2013 Red Hat, Inc.
   This file is part of elfutils.
   Written by Ulrich Drepper <drepper@redhat.com>, 2004.

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

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <assert.h>
#include <dwarf.h>
#include "libdwP.h"


int
dwarf_getsrcfiles (Dwarf_Die *cudie, Dwarf_Files **files, size_t *nfiles)
{
  if (unlikely (cudie == NULL
		|| (INTUSE(dwarf_tag) (cudie) != DW_TAG_compile_unit
		    && INTUSE(dwarf_tag) (cudie) != DW_TAG_partial_unit)))
    return -1;

  int res = -1;

  /* Get the information if it is not already known.  */
  struct Dwarf_CU *const cu = cudie->cu;
  if (cu->lines == NULL)
    {
      Dwarf_Lines *lines;
      size_t nlines;

      /* Let the more generic function do the work.  It'll create more
	 data but that will be needed in an real program anyway.  */
      res = INTUSE(dwarf_getsrclines) (cudie, &lines, &nlines);
    }
  else if (cu->files != (void *) -1l)
    /* We already have the information.  */
    res = 0;

  if (likely (res == 0))
    {
      assert (cu->files != NULL && cu->files != (void *) -1l);
      *files = cu->files;
      if (nfiles != NULL)
	*nfiles = cu->files->nfiles;
    }

  // XXX Eventually: unlocking here.

  return res;
}
INTDEF (dwarf_getsrcfiles)
