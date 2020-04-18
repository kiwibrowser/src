/* Return high PC attribute of DIE.
   Copyright (C) 2003, 2005, 2012 Red Hat, Inc.
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

#include <dwarf.h>
#include "libdwP.h"


int
dwarf_highpc (die, return_addr)
     Dwarf_Die *die;
     Dwarf_Addr *return_addr;
{
  Dwarf_Attribute attr_high_mem;
  Dwarf_Attribute *attr_high = INTUSE(dwarf_attr) (die, DW_AT_high_pc,
						   &attr_high_mem);
  if (attr_high == NULL)
    return -1;

  if (attr_high->form == DW_FORM_addr)
    return INTUSE(dwarf_formaddr) (attr_high, return_addr);

  /* DWARF 4 allows high_pc to be a constant offset from low_pc. */
  Dwarf_Attribute attr_low_mem;
  if (INTUSE(dwarf_formaddr) (INTUSE(dwarf_attr) (die, DW_AT_low_pc,
						  &attr_low_mem),
			      return_addr) == 0)
    {
      Dwarf_Word uval;
      if (INTUSE(dwarf_formudata) (attr_high, &uval) == 0)
	{
	  *return_addr += uval;
	  return 0;
	}
      __libdw_seterrno (DWARF_E_NO_ADDR);
    }
  return -1;
}
INTDEF(dwarf_highpc)
