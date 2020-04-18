/* Return string pointer from string section.
   Copyright (C) 1998, 1999, 2000, 2001, 2002, 2004, 2008, 2009 Red Hat, Inc.
   This file is part of elfutils.
   Contributed by Ulrich Drepper <drepper@redhat.com>, 1998.

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

#include <libelf.h>
#include <stddef.h>

#include "libelfP.h"


char *
elf_strptr (elf, idx, offset)
     Elf *elf;
     size_t idx;
     size_t offset;
{
  if (elf == NULL)
    return NULL;

  if (elf->kind != ELF_K_ELF)
    {
      __libelf_seterrno (ELF_E_INVALID_HANDLE);
      return NULL;
    }

  rwlock_rdlock (elf->lock);

  char *result = NULL;
  Elf_Scn *strscn;

  /* Find the section in the list.  */
  Elf_ScnList *runp = (elf->class == ELFCLASS32
		       || (offsetof (struct Elf, state.elf32.scns)
			   == offsetof (struct Elf, state.elf64.scns))
		       ? &elf->state.elf32.scns : &elf->state.elf64.scns);
  while (1)
    {
      if (idx < runp->max)
	{
	  if (idx < runp->cnt)
	    strscn = &runp->data[idx];
	  else
	    {
	      __libelf_seterrno (ELF_E_INVALID_INDEX);
	      goto out;
	    }
	  break;
	}

      idx -= runp->max;

      runp = runp->next;
      if (runp == NULL)
	{
	  __libelf_seterrno (ELF_E_INVALID_INDEX);
	  goto out;
	}
    }

  if (elf->class == ELFCLASS32)
    {
      if (unlikely (strscn->shdr.e32->sh_type != SHT_STRTAB))
	{
	  /* This is no string section.  */
	  __libelf_seterrno (ELF_E_INVALID_SECTION);
	  goto out;
	}

      if (unlikely (offset >= strscn->shdr.e32->sh_size))
	{
	  /* The given offset is too big, it is beyond this section.  */
	  __libelf_seterrno (ELF_E_OFFSET_RANGE);
	  goto out;
	}
    }
  else
    {
      if (unlikely (strscn->shdr.e64->sh_type != SHT_STRTAB))
	{
	  /* This is no string section.  */
	  __libelf_seterrno (ELF_E_INVALID_SECTION);
	  goto out;
	}

      if (unlikely (offset >= strscn->shdr.e64->sh_size))
	{
	  /* The given offset is too big, it is beyond this section.  */
	  __libelf_seterrno (ELF_E_OFFSET_RANGE);
	  goto out;
	}
    }

  if (strscn->rawdata_base == NULL && ! strscn->data_read)
    {
      rwlock_unlock (elf->lock);
      rwlock_wrlock (elf->lock);
      if (strscn->rawdata_base == NULL && ! strscn->data_read
	/* Read the section data.  */
	  && __libelf_set_rawdata_wrlock (strscn) != 0)
	goto out;
    }

  if (likely (strscn->rawdata_base != NULL))
    // XXX Is this correct if a file is read and then new data is added
    // XXX to the string section?  Likely needs to check offset against
    // XXX size of rawdata_base buffer and then iterate over rest of the
    // XXX list.
    result = &strscn->rawdata_base[offset];
  else
    {
      /* This is a file which is currently created.  Use the list of
	 data blocks.  */
      struct Elf_Data_List *dl = &strscn->data_list;
      while (dl != NULL)
	{
	  if (offset >= (size_t) dl->data.d.d_off
	      && offset < dl->data.d.d_off + dl->data.d.d_size)
	    {
	      result = (char *) dl->data.d.d_buf + (offset - dl->data.d.d_off);
	      break;
	    }

	  dl = dl->next;
	}
    }

 out:
  rwlock_unlock (elf->lock);

  return result;
}
INTDEF(elf_strptr)
