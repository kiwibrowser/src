/* Return DIE associated with a location expression op.
   Copyright (C) 2013 Red Hat, Inc.
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

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <dwarf.h>
#include <libdwP.h>

int
dwarf_getlocation_die (attr, op, result)
     Dwarf_Attribute *attr;
     const Dwarf_Op *op;
     Dwarf_Die *result;
{
  if (attr == NULL)
    return -1;

  Dwarf_Off dieoff;
  switch (op->atom)
    {
    case DW_OP_GNU_implicit_pointer:
    case DW_OP_call_ref:
      dieoff = op->number;
      break;

    case DW_OP_GNU_parameter_ref:
    case DW_OP_GNU_convert:
    case DW_OP_GNU_reinterpret:
    case DW_OP_GNU_const_type:
    case DW_OP_call2:
    case DW_OP_call4:
      dieoff = attr->cu->start + op->number;
      break;

    case DW_OP_GNU_regval_type:
    case DW_OP_GNU_deref_type:
      dieoff = attr->cu->start + op->number2;
      break;

    default:
      __libdw_seterrno (DWARF_E_INVALID_ACCESS);
      return -1;
    }

  if (__libdw_offdie (attr->cu->dbg, dieoff, result,
                     attr->cu->type_offset != 0) == NULL)
    return -1;

  return 0;
}
INTDEF(dwarf_getlocation_die);
