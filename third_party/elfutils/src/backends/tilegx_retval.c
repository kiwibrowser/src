/* Function return value location for Linux/TILE-Gx ABI.
   Copyright (C) 2012 Tilera Corporation
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

#include <assert.h>
#include <dwarf.h>

#define BACKEND tilegx_
#include "libebl_CPU.h"


/* r0.  */
static const Dwarf_Op loc_intreg[] =
  {
    { .atom = DW_OP_reg0 }
  };
#define nloc_intreg	1

/* The return value is a structure and is actually stored in stack space
   passed in a hidden argument by the caller.  But, the compiler
   helpfully returns the address of that space in r0.  */
static const Dwarf_Op loc_aggregate[] =
  {
    { .atom = DW_OP_breg0, .number = 0 }
  };
#define nloc_aggregate 1

int
tilegx_return_value_location (Dwarf_Die *functypedie, const Dwarf_Op **locp)
{
  /* Start with the function's type, and get the DW_AT_type attribute,
     which is the type of the return value.  */

  Dwarf_Attribute attr_mem;
  Dwarf_Attribute *attr = dwarf_attr_integrate (functypedie, DW_AT_type,
						&attr_mem);
  if (attr == NULL)
    /* The function has no return value, like a `void' function in C.  */
    return 0;

  Dwarf_Die die_mem;
  Dwarf_Die *typedie = dwarf_formref_die (attr, &die_mem);
  int tag = DWARF_TAG_OR_RETURN (typedie);

  /* Follow typedefs and qualifiers to get to the actual type.  */
  while (tag == DW_TAG_typedef
	 || tag == DW_TAG_const_type || tag == DW_TAG_volatile_type
	 || tag == DW_TAG_restrict_type || tag == DW_TAG_mutable_type)
    {
      attr = dwarf_attr_integrate (typedie, DW_AT_type, &attr_mem);
      typedie = dwarf_formref_die (attr, &die_mem);
      tag = DWARF_TAG_OR_RETURN (typedie);
    }

  Dwarf_Word size;
  switch (tag)
    {
    case -1:
      return -1;

    case DW_TAG_subrange_type:
      if (! dwarf_hasattr_integrate (typedie, DW_AT_byte_size))
	{
	  attr = dwarf_attr_integrate (typedie, DW_AT_type, &attr_mem);
	  typedie = dwarf_formref_die (attr, &die_mem);
	  tag = DWARF_TAG_OR_RETURN (typedie);
	}
      /* Fall through.  */

    case DW_TAG_base_type:
    case DW_TAG_enumeration_type:
    case DW_TAG_pointer_type:
    case DW_TAG_ptr_to_member_type:
      if (dwarf_formudata (dwarf_attr_integrate (typedie, DW_AT_byte_size,
						 &attr_mem), &size) != 0)
	{
	  if (tag == DW_TAG_pointer_type || tag == DW_TAG_ptr_to_member_type)
	    size = 8;
	  else
	    return -1;
	}
      if (tag == DW_TAG_base_type)
	{
	  Dwarf_Word encoding;
	  if (dwarf_formudata (dwarf_attr_integrate (typedie, DW_AT_encoding,
						     &attr_mem),
			       &encoding) != 0)
	    return -1;
        }

      /* Small enough structs are passed directly in registers R0 ... R7.  */
      if (size <= 8)
	{
	intreg:
	  *locp = loc_intreg;
	  return nloc_intreg;
	}

      /* Else fall through.  */
    case DW_TAG_structure_type:
    case DW_TAG_class_type:
    case DW_TAG_union_type:
    aggregate:
      *locp = loc_aggregate;
      return nloc_aggregate;

    case DW_TAG_array_type:
    case DW_TAG_string_type:
      if (dwarf_aggregate_size (typedie, &size) == 0 && size <= 8)
	{
	  if (tag == DW_TAG_array_type)
	    {
	      /* Check if it's a character array.  */
	      attr = dwarf_attr_integrate (typedie, DW_AT_type, &attr_mem);
	      typedie = dwarf_formref_die (attr, &die_mem);
	      tag = DWARF_TAG_OR_RETURN (typedie);
	      if (tag != DW_TAG_base_type)
		goto aggregate;
	      if (dwarf_formudata (dwarf_attr_integrate (typedie,
							 DW_AT_byte_size,
							 &attr_mem),
				   &size) != 0)
		return -1;
	      if (size != 1)
		goto aggregate;
	    }
	  goto intreg;
	}
      goto aggregate;
    }

  /* XXX We don't have a good way to return specific errors from ebl calls.
     This value means we do not understand the type, but it is well-formed
     DWARF and might be valid.  */
  return -2;
}
