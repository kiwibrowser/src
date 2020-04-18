/* Return DWARF attribute associated with a location expression op.
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
dwarf_getlocation_attr (attr, op, result)
     Dwarf_Attribute *attr;
     const Dwarf_Op *op;
     Dwarf_Attribute *result;
{
  if (attr == NULL)
    return -1;

  result->cu = attr->cu;

  switch (op->atom)
    {
      case DW_OP_implicit_value:
	result->code = DW_AT_const_value;
	result->form = DW_FORM_block;
	result->valp = (unsigned char *) (uintptr_t) op->number2;
	break;

      case DW_OP_GNU_entry_value:
	result->code = DW_AT_location;
	result->form = DW_FORM_exprloc;
	result->valp = (unsigned char *) (uintptr_t) op->number2;
	break;

      case DW_OP_GNU_const_type:
	result->code = DW_AT_const_value;
	result->form = DW_FORM_block1;
	result->valp = (unsigned char *) (uintptr_t) op->number2;
	break;

      case DW_OP_call2:
      case DW_OP_call4:
      case DW_OP_call_ref:
	{
	  Dwarf_Die die;
	  if (INTUSE(dwarf_getlocation_die) (attr, op, &die) != 0)
	    return -1;
	  if (INTUSE(dwarf_attr) (&die, DW_AT_location, result) == NULL)
	    {
	      __libdw_empty_loc_attr (result, attr->cu);
	      return 0;
	    }
	}
	break;

      case DW_OP_GNU_implicit_pointer:
	{
	  Dwarf_Die die;
	  if (INTUSE(dwarf_getlocation_die) (attr, op, &die) != 0)
	    return -1;
	  if (INTUSE(dwarf_attr) (&die, DW_AT_location, result) == NULL
	      && INTUSE(dwarf_attr) (&die, DW_AT_const_value, result) == NULL)
	    {
	      __libdw_empty_loc_attr (result, attr->cu);
	      return 0;
	    }
	}
	break;

      default:
	__libdw_seterrno (DWARF_E_INVALID_ACCESS);
	return -1;
    }

  return 0;
}
