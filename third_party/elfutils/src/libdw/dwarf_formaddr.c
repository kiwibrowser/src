/* Return address represented by attribute.
   Copyright (C) 2003-2010 Red Hat, Inc.
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
dwarf_formaddr (attr, return_addr)
     Dwarf_Attribute *attr;
     Dwarf_Addr *return_addr;
{
  if (attr == NULL)
    return -1;

  if (unlikely (attr->form != DW_FORM_addr))
    {
      __libdw_seterrno (DWARF_E_NO_ADDR);
      return -1;
    }

  if (__libdw_read_address (attr->cu->dbg,
			    cu_sec_idx (attr->cu), attr->valp,
			    attr->cu->address_size, return_addr))
    return -1;

  return 0;
}
INTDEF(dwarf_formaddr)
