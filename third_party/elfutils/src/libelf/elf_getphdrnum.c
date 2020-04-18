/* Return number of program headers in the ELF file.
   Copyright (C) 2010 Red Hat, Inc.
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
#include <gelf.h>
#include <stddef.h>

#include "libelfP.h"


int
__elf_getphdrnum_rdlock (elf, dst)
     Elf *elf;
     size_t *dst;
{
 if (unlikely (elf->state.elf64.ehdr == NULL))
   {
     /* Maybe no ELF header was created yet.  */
     __libelf_seterrno (ELF_E_WRONG_ORDER_EHDR);
     return -1;
   }

 *dst = (elf->class == ELFCLASS32
	 ? elf->state.elf32.ehdr->e_phnum
	 : elf->state.elf64.ehdr->e_phnum);

 if (*dst == PN_XNUM)
   {
     const Elf_ScnList *const scns = (elf->class == ELFCLASS32
				      ? &elf->state.elf32.scns
				      : &elf->state.elf64.scns);

     /* If there are no section headers, perhaps this is really just 65536
	written without PN_XNUM support.  Either that or it's bad data.  */

     if (likely (scns->cnt > 0))
       *dst = (elf->class == ELFCLASS32
	       ? scns->data[0].shdr.e32->sh_info
	       : scns->data[0].shdr.e64->sh_info);
   }

 return 0;
}

int
elf_getphdrnum (elf, dst)
     Elf *elf;
     size_t *dst;
{
  int result;

  if (elf == NULL)
    return -1;

  if (unlikely (elf->kind != ELF_K_ELF))
    {
      __libelf_seterrno (ELF_E_INVALID_HANDLE);
      return -1;
    }

  rwlock_rdlock (elf->lock);
  result = __elf_getphdrnum_rdlock (elf, dst);
  rwlock_unlock (elf->lock);

  return result;
}
