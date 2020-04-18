/* Return string associated with given attribute.
   Copyright (C) 2003-2010, 2013 Red Hat, Inc.
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


const char *
dwarf_formstring (attrp)
     Dwarf_Attribute *attrp;
{
  /* Ignore earlier errors.  */
  if (attrp == NULL)
    return NULL;

  /* We found it.  Now determine where the string is stored.  */
  if (attrp->form == DW_FORM_string)
    /* A simple inlined string.  */
    return (const char *) attrp->valp;

  Dwarf *dbg = attrp->cu->dbg;
  Dwarf *dbg_ret = attrp->form == DW_FORM_GNU_strp_alt ? dbg->alt_dwarf : dbg;

  if (unlikely (dbg_ret == NULL))
    {
      __libdw_seterrno (DWARF_E_NO_ALT_DEBUGLINK);
      return NULL;
    }


  if (unlikely (attrp->form != DW_FORM_strp
		   && attrp->form != DW_FORM_GNU_strp_alt)
      || dbg_ret->sectiondata[IDX_debug_str] == NULL)
    {
      __libdw_seterrno (DWARF_E_NO_STRING);
      return NULL;
    }

  uint64_t off;
  if (__libdw_read_offset (dbg, dbg_ret, cu_sec_idx (attrp->cu), attrp->valp,
			   attrp->cu->offset_size, &off, IDX_debug_str, 1))
    return NULL;

  return (const char *) dbg_ret->sectiondata[IDX_debug_str]->d_buf + off;
}
INTDEF(dwarf_formstring)
