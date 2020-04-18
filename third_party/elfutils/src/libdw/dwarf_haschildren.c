/* Return string associated with given attribute.
   Copyright (C) 2003, 2005, 2008 Red Hat, Inc.
   This file is part of elfutils.
   Written by Ulrich Drepper <drepper@redhat.com>, 2003.

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
#include <string.h>


int
dwarf_haschildren (die)
     Dwarf_Die *die;
{
  /* Find the abbreviation entry.  */
  Dwarf_Abbrev *abbrevp = die->abbrev;
  if (abbrevp != DWARF_END_ABBREV)
    {
      const unsigned char *readp = (unsigned char *) die->addr;

      /* First we have to get the abbreviation code so that we can decode
	 the data in the DIE.  */
      unsigned int abbrev_code;
      get_uleb128 (abbrev_code, readp);

      abbrevp = __libdw_findabbrev (die->cu, abbrev_code);
      die->abbrev = abbrevp ?: DWARF_END_ABBREV;
    }
  if (unlikely (die->abbrev == DWARF_END_ABBREV))
    {
      __libdw_seterrno (DWARF_E_INVALID_DWARF);
      return -1;
    }

  return die->abbrev->has_children;
}
INTDEF (dwarf_haschildren)
