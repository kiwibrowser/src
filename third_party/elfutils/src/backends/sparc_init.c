/* Initialization of SPARC specific backend library.
   Copyright (C) 2002, 2005, 2006, 2007, 2008 Red Hat, Inc.
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

#define BACKEND		sparc_
#define RELOC_PREFIX	R_SPARC_
#include "libebl_CPU.h"

/* This defines the common reloc hooks based on sparc_reloc.def.  */
#include "common-reloc.c"

extern __typeof (EBLHOOK (core_note)) sparc64_core_note attribute_hidden;

const char *
sparc_init (elf, machine, eh, ehlen)
     Elf *elf __attribute__ ((unused));
     GElf_Half machine __attribute__ ((unused));
     Ebl *eh;
     size_t ehlen;
{
  /* Check whether the Elf_BH object has a sufficent size.  */
  if (ehlen < sizeof (Ebl))
    return NULL;

  /* We handle it.  */
  if (machine == EM_SPARCV9)
    eh->name = "SPARC v9";
  else if (machine == EM_SPARC32PLUS)
    eh->name = "SPARC v8+";
  else
    eh->name = "SPARC";
  sparc_init_reloc (eh);
  HOOK (eh, reloc_simple_type);
  HOOK (eh, machine_flag_check);
  HOOK (eh, check_special_section);
  HOOK (eh, symbol_type_name);
  HOOK (eh, dynamic_tag_name);
  HOOK (eh, dynamic_tag_check);
  if (eh->class == ELFCLASS64)
    eh->core_note = sparc64_core_note;
  else
    HOOK (eh, core_note);
  HOOK (eh, auxv_info);
  HOOK (eh, register_info);
  HOOK (eh, return_value_location);

  return MODVERSION;
}
