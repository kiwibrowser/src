/* Return signed constant represented by attribute.
   Copyright (C) 2003, 2005 Red Hat, Inc.
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
dwarf_formsdata (attr, return_sval)
     Dwarf_Attribute *attr;
     Dwarf_Sword *return_sval;
{
  if (attr == NULL)
    return -1;

  const unsigned char *datap;

  switch (attr->form)
    {
    case DW_FORM_data1:
      *return_sval = *attr->valp;
      break;

    case DW_FORM_data2:
      *return_sval = read_2ubyte_unaligned (attr->cu->dbg, attr->valp);
      break;

    case DW_FORM_data4:
      *return_sval = read_4ubyte_unaligned (attr->cu->dbg, attr->valp);
      break;

    case DW_FORM_data8:
      *return_sval = read_8ubyte_unaligned (attr->cu->dbg, attr->valp);
      break;

    case DW_FORM_sdata:
      datap = attr->valp;
      get_sleb128 (*return_sval, datap);
      break;

    case DW_FORM_udata:
      datap = attr->valp;
      get_uleb128 (*return_sval, datap);
      break;

    default:
      __libdw_seterrno (DWARF_E_NO_CONSTANT);
      return -1;
    }

  return 0;
}
INTDEF(dwarf_formsdata)
