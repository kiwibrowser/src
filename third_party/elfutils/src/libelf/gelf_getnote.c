/* Get note information at the supplied offset.
   Copyright (C) 2007 Red Hat, Inc.
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
#include <string.h>

#include "libelfP.h"

size_t
gelf_getnote (data, offset, result, name_offset, desc_offset)
     Elf_Data *data;
     size_t offset;
     GElf_Nhdr *result;
     size_t *name_offset;
     size_t *desc_offset;
{
  if (data == NULL)
    return 0;

  if (unlikely (data->d_type != ELF_T_NHDR))
    {
      __libelf_seterrno (ELF_E_INVALID_HANDLE);
      return 0;
    }

  /* It's easy to handle this type.  It has the same size for 32 and
     64 bit objects.  */
  assert (sizeof (GElf_Nhdr) == sizeof (Elf32_Nhdr));
  assert (sizeof (GElf_Nhdr) == sizeof (Elf64_Nhdr));

  rwlock_rdlock (((Elf_Data_Scn *) data)->s->elf->lock);

  /* The data is already in the correct form.  Just make sure the
     offset is OK.  */
  if (unlikely (offset + sizeof (GElf_Nhdr) > data->d_size))
    {
      __libelf_seterrno (ELF_E_OFFSET_RANGE);
      offset = 0;
    }
  else
    {
      const GElf_Nhdr *n = data->d_buf + offset;
      offset += sizeof *n;

      GElf_Word namesz = NOTE_ALIGN (n->n_namesz);
      GElf_Word descsz = NOTE_ALIGN (n->n_descsz);

      if (unlikely (data->d_size - offset < namesz))
	offset = 0;
      else
	{
	  *name_offset = offset;
	  offset += namesz;
	  if (unlikely (data->d_size - offset < descsz))
	    offset = 0;
	  else
	    {
	      *desc_offset = offset;
	      offset += descsz;
	      *result = *n;
	    }
	}
    }

  rwlock_unlock (((Elf_Data_Scn *) data)->s->elf->lock);

  return offset;
}
