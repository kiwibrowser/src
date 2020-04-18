/* PPC specific symbolic name handling.
   Copyright (C) 2004, 2005, 2007 Red Hat, Inc.
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

#include <assert.h>
#include <elf.h>
#include <stddef.h>
#include <string.h>

#define BACKEND		ppc_
#include "libebl_CPU.h"


/* Check for the simple reloc types.  */
Elf_Type
ppc_reloc_simple_type (Ebl *ebl __attribute__ ((unused)), int type)
{
  switch (type)
    {
    case R_PPC_ADDR32:
    case R_PPC_UADDR32:
      return ELF_T_WORD;
    case R_PPC_UADDR16:
      return ELF_T_HALF;
    default:
      return ELF_T_NUM;
    }
}


const char *
ppc_dynamic_tag_name (int64_t tag, char *buf __attribute__ ((unused)),
		      size_t len __attribute__ ((unused)))
{
  switch (tag)
    {
    case DT_PPC_GOT:
      return "PPC_GOT";
    default:
      break;
    }
  return NULL;
}


bool
ppc_dynamic_tag_check (int64_t tag)
{
  return tag == DT_PPC_GOT;
}


/* Look for DT_PPC_GOT.  */
static bool
find_dyn_got (Elf *elf, GElf_Ehdr *ehdr, GElf_Addr *addr)
{
  for (int i = 0; i < ehdr->e_phnum; ++i)
    {
      GElf_Phdr phdr_mem;
      GElf_Phdr *phdr = gelf_getphdr (elf, i, &phdr_mem);
      if (phdr == NULL || phdr->p_type != PT_DYNAMIC)
	continue;

      Elf_Scn *scn = gelf_offscn (elf, phdr->p_offset);
      GElf_Shdr shdr_mem;
      GElf_Shdr *shdr = gelf_getshdr (scn, &shdr_mem);
      Elf_Data *data = elf_getdata (scn, NULL);
      if (shdr != NULL && shdr->sh_type == SHT_DYNAMIC && data != NULL)
	for (unsigned int j = 0; j < shdr->sh_size / shdr->sh_entsize; ++j)
	  {
	    GElf_Dyn dyn_mem;
	    GElf_Dyn *dyn = gelf_getdyn (data, j, &dyn_mem);
	    if (dyn != NULL && dyn->d_tag == DT_PPC_GOT)
	      {
		*addr = dyn->d_un.d_ptr;
		return true;
	      }
	  }

      /* There is only one PT_DYNAMIC entry.  */
      break;
    }

  return false;
}


/* Check whether given symbol's st_value and st_size are OK despite failing
   normal checks.  */
bool
ppc_check_special_symbol (Elf *elf, GElf_Ehdr *ehdr, const GElf_Sym *sym,
			  const char *name, const GElf_Shdr *destshdr)
{
  if (name == NULL)
    return false;

  if (strcmp (name, "_GLOBAL_OFFSET_TABLE_") == 0)
    {
      /* In -msecure-plt mode, DT_PPC_GOT is present and must match.  */
      GElf_Addr gotaddr;
      if (find_dyn_got (elf, ehdr, &gotaddr))
	return sym->st_value == gotaddr;

      /* In -mbss-plt mode, any place in the section is valid.  */
      return true;
    }

  const char *sname = elf_strptr (elf, ehdr->e_shstrndx, destshdr->sh_name);
  if (sname == NULL)
    return false;

  if (strcmp (name, "_SDA_BASE_") == 0)
    return (strcmp (sname, ".sdata") == 0
	    && sym->st_value == destshdr->sh_addr + 0x8000
	    && sym->st_size == 0);

  if (strcmp (name, "_SDA2_BASE_") == 0)
    return (strcmp (sname, ".sdata2") == 0
	    && sym->st_value == destshdr->sh_addr + 0x8000
	    && sym->st_size == 0);

  return false;
}


/* Check if backend uses a bss PLT in this file.  */
bool
ppc_bss_plt_p (Elf *elf, GElf_Ehdr *ehdr)
{
  GElf_Addr addr;
  return ! find_dyn_got (elf, ehdr, &addr);
}
