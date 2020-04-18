/* Look up the DIE in a reference-form attribute.
   Copyright (C) 2005-2010 Red Hat, Inc.
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

#include <string.h>
#include "libdwP.h"
#include <dwarf.h>


Dwarf_Die *
dwarf_formref_die (attr, result)
     Dwarf_Attribute *attr;
     Dwarf_Die *result;
{
  if (attr == NULL)
    return NULL;

  struct Dwarf_CU *cu = attr->cu;

  Dwarf_Off offset;
  if (attr->form == DW_FORM_ref_addr || attr->form == DW_FORM_GNU_ref_alt)
    {
      /* This has an absolute offset.  */

      uint8_t ref_size = (cu->version == 2 && attr->form == DW_FORM_ref_addr
			  ? cu->address_size
			  : cu->offset_size);

      Dwarf *dbg_ret = (attr->form == DW_FORM_GNU_ref_alt
			? cu->dbg->alt_dwarf : cu->dbg);

      if (dbg_ret == NULL)
	{
	  __libdw_seterrno (DWARF_E_NO_ALT_DEBUGLINK);
	  return NULL;
	}

      if (__libdw_read_offset (cu->dbg, dbg_ret, IDX_debug_info, attr->valp,
			       ref_size, &offset, IDX_debug_info, 0))
	return NULL;

      return INTUSE(dwarf_offdie) (dbg_ret, offset, result);
    }

  Elf_Data *data;
  if (attr->form == DW_FORM_ref_sig8)
    {
      /* This doesn't have an offset, but instead a value we
	 have to match in the .debug_types type unit headers.  */

      uint64_t sig = read_8ubyte_unaligned (cu->dbg, attr->valp);
      cu = Dwarf_Sig8_Hash_find (&cu->dbg->sig8_hash, sig, NULL);
      if (cu == NULL)
	/* Not seen before.  We have to scan through the type units.  */
	do
	  {
	    cu = __libdw_intern_next_unit (attr->cu->dbg, true);
	    if (cu == NULL)
	      {
		__libdw_seterrno (INTUSE(dwarf_errno) ()
				  ?: DWARF_E_INVALID_REFERENCE);
		return NULL;
	      }
	  }
	while (cu->type_sig8 != sig);

      data = cu->dbg->sectiondata[IDX_debug_types];
      offset = cu->type_offset;
    }
  else
    {
      /* Other forms produce an offset from the CU.  */
      if (unlikely (__libdw_formref (attr, &offset) != 0))
	return NULL;

      data = cu_data (cu);
    }

  if (unlikely (data->d_size - cu->start <= offset))
    {
      __libdw_seterrno (DWARF_E_INVALID_DWARF);
      return NULL;
    }

  memset (result, '\0', sizeof (Dwarf_Die));
  result->addr = (char *) data->d_buf + cu->start + offset;
  result->cu = cu;
  return result;
}
INTDEF (dwarf_formref_die)
