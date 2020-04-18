/* AArch64 specific symbolic name handling.
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

#include <elf.h>
#include <stddef.h>
#include <string.h>

#define BACKEND		aarch64_
#include "libebl_CPU.h"


/* Check for the simple reloc types.  */
Elf_Type
aarch64_reloc_simple_type (Ebl *ebl __attribute__ ((unused)), int type)
{
  switch (type)
    {
    case R_AARCH64_ABS64:
      return ELF_T_XWORD;
    case R_AARCH64_ABS32:
      return ELF_T_WORD;
    case R_AARCH64_ABS16:
      return ELF_T_HALF;

    default:
      return ELF_T_NUM;
    }
}

/* If this is the _GLOBAL_OFFSET_TABLE_ symbol, then it should point to
   .got[0] even if there is a .got.plt section.  */
bool
aarch64_check_special_symbol (Elf *elf, GElf_Ehdr *ehdr, const GElf_Sym *sym,
                              const char *name, const GElf_Shdr *destshdr)
{
  if (name != NULL
      && strcmp (name, "_GLOBAL_OFFSET_TABLE_") == 0)
    {
      const char *sname = elf_strptr (elf, ehdr->e_shstrndx, destshdr->sh_name);
      if (sname != NULL && strcmp (sname, ".got.plt") == 0)
	{
	  Elf_Scn *scn = NULL;
	  while ((scn = elf_nextscn (elf, scn)) != NULL)
	    {
	      GElf_Shdr shdr_mem;
	      GElf_Shdr *shdr = gelf_getshdr (scn, &shdr_mem);
	      sname = elf_strptr (elf, ehdr->e_shstrndx, shdr->sh_name);
	      if (sname != NULL && strcmp (sname, ".got") == 0)
		return sym->st_value == shdr->sh_addr;
	    }
	}
    }

  return false;
}
