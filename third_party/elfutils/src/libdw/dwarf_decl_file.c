/* Return file name containing definition of the given function.
   Copyright (C) 2005, 2009 Red Hat, Inc.
   This file is part of elfutils.
   Written by Ulrich Drepper <drepper@redhat.com>, 2005.

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


const char *
dwarf_decl_file (Dwarf_Die *die)
{
  Dwarf_Attribute attr_mem;
  Dwarf_Sword idx = 0;

  if (INTUSE(dwarf_formsdata) (INTUSE(dwarf_attr_integrate)
			       (die, DW_AT_decl_file, &attr_mem),
			       &idx) != 0)
    return NULL;

  /* Zero means no source file information available.  */
  if (idx == 0)
    {
      __libdw_seterrno (DWARF_E_NO_ENTRY);
      return NULL;
    }

  /* Get the array of source files for the CU.  */
  struct Dwarf_CU *cu = die->cu;
  if (cu->lines == NULL)
    {
      Dwarf_Lines *lines;
      size_t nlines;

      /* Let the more generic function do the work.  It'll create more
	 data but that will be needed in an real program anyway.  */
      (void) INTUSE(dwarf_getsrclines) (&CUDIE (cu), &lines, &nlines);
      assert (cu->lines != NULL);
    }

  if (cu->lines == (void *) -1l)
    {
      /* If the file index is not zero, there must be file information
	 available.  */
      __libdw_seterrno (DWARF_E_INVALID_DWARF);
      return NULL;
    }

  assert (cu->files != NULL && cu->files != (void *) -1l);

  if (idx >= cu->files->nfiles)
    {
      __libdw_seterrno (DWARF_E_INVALID_DWARF);
      return NULL;
    }

  return cu->files->info[idx].name;
}
OLD_VERSION (dwarf_decl_file, ELFUTILS_0.122)
NEW_VERSION (dwarf_decl_file, ELFUTILS_0.143)
