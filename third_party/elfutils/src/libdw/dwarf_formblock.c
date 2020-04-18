/* Return block represented by attribute.
   Copyright (C) 2004-2010 Red Hat, Inc.
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

#include <dwarf.h>
#include "libdwP.h"


int
dwarf_formblock (attr, return_block)
     Dwarf_Attribute *attr;
     Dwarf_Block *return_block;
{
  if (attr == NULL)
    return -1;

  const unsigned char *datap;

  switch (attr->form)
    {
    case DW_FORM_block1:
      return_block->length = *(uint8_t *) attr->valp;
      return_block->data = attr->valp + 1;
      break;

    case DW_FORM_block2:
      return_block->length = read_2ubyte_unaligned (attr->cu->dbg, attr->valp);
      return_block->data = attr->valp + 2;
      break;

    case DW_FORM_block4:
      return_block->length = read_4ubyte_unaligned (attr->cu->dbg, attr->valp);
      return_block->data = attr->valp + 4;
      break;

    case DW_FORM_block:
    case DW_FORM_exprloc:
      datap = attr->valp;
      get_uleb128 (return_block->length, datap);
      return_block->data = (unsigned char *) datap;
      break;

    default:
      __libdw_seterrno (DWARF_E_NO_BLOCK);
      return -1;
    }

  if (unlikely (cu_data (attr->cu)->d_size
		- (return_block->data
		   - (unsigned char *) cu_data (attr->cu)->d_buf)
		< return_block->length))
    {
      /* Block does not fit.  */
      __libdw_seterrno (DWARF_E_INVALID_DWARF);
      return -1;
    }

  return 0;
}
INTDEF(dwarf_formblock)
