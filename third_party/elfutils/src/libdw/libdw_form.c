/* Helper functions for form handling.
   Copyright (C) 2003-2009 Red Hat, Inc.
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
#include <string.h>

#include "libdwP.h"


size_t
internal_function
__libdw_form_val_compute_len (Dwarf *dbg, struct Dwarf_CU *cu,
			      unsigned int form, const unsigned char *valp)
{
  const unsigned char *saved;
  Dwarf_Word u128;
  size_t result;

  /* NB: This doesn't cover constant form lengths, which are
     already handled by the inlined __libdw_form_val_len.  */
  switch (form)
    {
    case DW_FORM_addr:
      result = cu->address_size;
      break;

    case DW_FORM_ref_addr:
      result = cu->version == 2 ? cu->address_size : cu->offset_size;
      break;

    case DW_FORM_strp:
    case DW_FORM_sec_offset:
    case DW_FORM_GNU_ref_alt:
    case DW_FORM_GNU_strp_alt:
      result = cu->offset_size;
      break;

    case DW_FORM_block1:
      result = *valp + 1;
      break;

    case DW_FORM_block2:
      result = read_2ubyte_unaligned (dbg, valp) + 2;
      break;

    case DW_FORM_block4:
      result = read_4ubyte_unaligned (dbg, valp) + 4;
      break;

    case DW_FORM_block:
    case DW_FORM_exprloc:
      saved = valp;
      get_uleb128 (u128, valp);
      result = u128 + (valp - saved);
      break;

    case DW_FORM_string:
      result = strlen ((char *) valp) + 1;
      break;

    case DW_FORM_sdata:
    case DW_FORM_udata:
    case DW_FORM_ref_udata:
      saved = valp;
      get_uleb128 (u128, valp);
      result = valp - saved;
      break;

    case DW_FORM_indirect:
      saved = valp;
      get_uleb128 (u128, valp);
      // XXX Is this really correct?
      result = __libdw_form_val_len (dbg, cu, u128, valp);
      if (result != (size_t) -1)
	result += valp - saved;
      break;

    default:
      __libdw_seterrno (DWARF_E_INVALID_DWARF);
      result = (size_t) -1l;
      break;
    }

  return result;
}
