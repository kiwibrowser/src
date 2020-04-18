/* Find line information for address.
   Copyright (C) 2004, 2005 Red Hat, Inc.
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

#include "libdwP.h"
#include <assert.h>


Dwarf_Line *
dwarf_getsrc_die (Dwarf_Die *cudie, Dwarf_Addr addr)
{
  Dwarf_Lines *lines;
  size_t nlines;

  if (INTUSE(dwarf_getsrclines) (cudie, &lines, &nlines) != 0)
    return NULL;

  /* The lines are sorted by address, so we can use binary search.  */
  size_t l = 0, u = nlines;
  while (l < u)
    {
      size_t idx = (l + u) / 2;
      if (addr < lines->info[idx].addr)
	u = idx;
      else if (addr > lines->info[idx].addr || lines->info[idx].end_sequence)
	l = idx + 1;
      else
	return &lines->info[idx];
    }

  if (nlines > 0)
    assert (lines->info[nlines - 1].end_sequence);

  /* If none were equal, the closest one below is what we want.  We
     never want the last one, because it's the end-sequence marker
     with an address at the high bound of the CU's code.  If the debug
     information is faulty and no end-sequence marker is present, we
     still ignore it.  */
  if (u > 0 && u < nlines && addr > lines->info[u - 1].addr)
    {
      while (lines->info[u - 1].end_sequence && u > 0)
	--u;
      if (u > 0)
	return &lines->info[u - 1];
    }

  __libdw_seterrno (DWARF_E_ADDR_OUTOFRANGE);
  return NULL;
}
