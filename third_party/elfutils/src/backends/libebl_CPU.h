/* Common interface for libebl modules.
   Copyright (C) 2000, 2001, 2002, 2003, 2005, 2013 Red Hat, Inc.
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

#ifndef _LIBEBL_CPU_H
#define _LIBEBL_CPU_H 1

#include <dwarf.h>
#include <libeblP.h>

#define EBLHOOK(name)	EBLHOOK_1(BACKEND, name)
#define EBLHOOK_1(a, b)	EBLHOOK_2(a, b)
#define EBLHOOK_2(a, b)	a##b

/* Constructor.  */
extern const char *EBLHOOK(init) (Elf *elf, GElf_Half machine,
				  Ebl *eh, size_t ehlen);

#include "ebl-hooks.h"

#define HOOK(eh, name)	eh->name = EBLHOOK(name)

extern bool (*generic_debugscn_p) (const char *) attribute_hidden;

/* Helper for retval.  Return dwarf_tag (die), but calls return -1
   if there where previous errors that leave die NULL.  */
#define DWARF_TAG_OR_RETURN(die)  \
  ({ Dwarf_Die *_die = (die);	  \
     if (_die == NULL) return -1; \
     dwarf_tag (_die); })

/* Follow typedefs and qualifiers to get to the actual type.  */
static inline int
dwarf_peel_type (Dwarf_Die *typediep, Dwarf_Attribute *attrp)
{
  int tag = DWARF_TAG_OR_RETURN (typediep);
  while (tag == DW_TAG_typedef
	 || tag == DW_TAG_const_type || tag == DW_TAG_volatile_type
	 || tag == DW_TAG_restrict_type || tag == DW_TAG_mutable_type)
    {
      attrp = dwarf_attr_integrate (typediep, DW_AT_type, attrp);
      typediep = dwarf_formref_die (attrp, typediep);
      tag = DWARF_TAG_OR_RETURN (typediep);
    }

  return tag;
}

/* Get a type die corresponding to DIE.  Peel CV qualifiers off
   it.  */
static inline int
dwarf_peeled_die_type (Dwarf_Die *die, Dwarf_Die *result)
{
  Dwarf_Attribute attr_mem;
  Dwarf_Attribute *attr = dwarf_attr_integrate (die, DW_AT_type, &attr_mem);
  if (attr == NULL)
    /* The function has no return value, like a `void' function in C.  */
    return 0;

  if (dwarf_formref_die (attr, result) == NULL)
    return -1;

  return dwarf_peel_type (result, attr);
}

#endif	/* libebl_CPU.h */
