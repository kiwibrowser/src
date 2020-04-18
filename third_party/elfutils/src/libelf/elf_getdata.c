/* Return the next data element from the section after possibly converting it.
   Copyright (C) 1998-2005, 2006, 2007 Red Hat, Inc.
   This file is part of elfutils.
   Written by Ulrich Drepper <drepper@redhat.com>, 1998.

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

#include <errno.h>
#include <stddef.h>
#include <string.h>
#include <unistd.h>

#include "libelfP.h"
#include <system.h>
#include "common.h"
#include "elf-knowledge.h"


#define TYPEIDX(Sh_Type) \
  (Sh_Type >= SHT_NULL && Sh_Type < SHT_NUM				      \
   ? Sh_Type								      \
   : (Sh_Type >= SHT_GNU_HASH && Sh_Type <= SHT_HISUNW			      \
      ? SHT_NUM + Sh_Type - SHT_GNU_HASH				      \
      : 0))

/* Associate section types with libelf types.  */
static const Elf_Type shtype_map[EV_NUM - 1][TYPEIDX (SHT_HISUNW) + 1] =
  {
    [EV_CURRENT - 1] =
    {
      [SHT_SYMTAB] = ELF_T_SYM,
      [SHT_RELA] = ELF_T_RELA,
      [SHT_HASH] = ELF_T_WORD,
      [SHT_DYNAMIC] = ELF_T_DYN,
      [SHT_REL] = ELF_T_REL,
      [SHT_DYNSYM] = ELF_T_SYM,
      [SHT_INIT_ARRAY] = ELF_T_ADDR,
      [SHT_FINI_ARRAY] = ELF_T_ADDR,
      [SHT_PREINIT_ARRAY] = ELF_T_ADDR,
      [SHT_GROUP] = ELF_T_WORD,
      [SHT_SYMTAB_SHNDX] = ELF_T_WORD,
      [SHT_NOTE] = ELF_T_NHDR,
      [TYPEIDX (SHT_GNU_verdef)] = ELF_T_VDEF,
      [TYPEIDX (SHT_GNU_verneed)] = ELF_T_VNEED,
      [TYPEIDX (SHT_GNU_versym)] = ELF_T_HALF,
      [TYPEIDX (SHT_SUNW_syminfo)] = ELF_T_SYMINFO,
      [TYPEIDX (SHT_SUNW_move)] = ELF_T_MOVE,
      [TYPEIDX (SHT_GNU_LIBLIST)] = ELF_T_LIB,
      [TYPEIDX (SHT_GNU_HASH)] = ELF_T_GNUHASH,
    }
  };

#if !ALLOW_UNALIGNED
/* Associate libelf types with their internal alignment requirements.  */
const uint_fast8_t __libelf_type_aligns[EV_NUM - 1][ELFCLASSNUM - 1][ELF_T_NUM] =
  {
# define TYPE_ALIGNS(Bits)						      \
    {									      \
      [ELF_T_ADDR] = __alignof__ (ElfW2(Bits,Addr)),			      \
      [ELF_T_HALF] = __alignof__ (ElfW2(Bits,Half)),			      \
      [ELF_T_WORD] = __alignof__ (ElfW2(Bits,Word)),			      \
      [ELF_T_SYM] = __alignof__ (ElfW2(Bits,Sym)),			      \
      [ELF_T_SYMINFO] = __alignof__ (ElfW2(Bits,Syminfo)),		      \
      [ELF_T_REL] = __alignof__ (ElfW2(Bits,Rel)),			      \
      [ELF_T_RELA] = __alignof__ (ElfW2(Bits,Rela)),			      \
      [ELF_T_DYN] = __alignof__ (ElfW2(Bits,Dyn)),			      \
      [ELF_T_VDEF] = __alignof__ (ElfW2(Bits,Verdef)),			      \
      [ELF_T_VDAUX] = __alignof__ (ElfW2(Bits,Verdaux)),		      \
      [ELF_T_VNEED] = __alignof__ (ElfW2(Bits,Verneed)),		      \
      [ELF_T_VNAUX] = __alignof__ (ElfW2(Bits,Vernaux)),		      \
      [ELF_T_MOVE] = __alignof__ (ElfW2(Bits,Move)),			      \
      [ELF_T_LIB] = __alignof__ (ElfW2(Bits,Lib)),			      \
      [ELF_T_NHDR] = __alignof__ (ElfW2(Bits,Nhdr)),			      \
    }
    [EV_CURRENT - 1] =
    {
      [ELFCLASS32 - 1] = TYPE_ALIGNS (32),
      [ELFCLASS64 - 1] = TYPE_ALIGNS (64),
    }
# undef TYPE_ALIGNS
  };
#endif


/* Convert the data in the current section.  */
static void
convert_data (Elf_Scn *scn, int version __attribute__ ((unused)), int eclass,
	      int data, size_t size, Elf_Type type)
{
  const size_t align = __libelf_type_align (eclass, type);

  if (data == MY_ELFDATA)
    {
      if (((((size_t) (char *) scn->rawdata_base)) & (align - 1)) == 0)
	/* No need to copy, we can use the raw data.  */
	scn->data_base = scn->rawdata_base;
      else
	{
	  scn->data_base = (char *) malloc (size);
	  if (scn->data_base == NULL)
	    {
	      __libelf_seterrno (ELF_E_NOMEM);
	      return;
	    }

	  /* The copy will be appropriately aligned for direct access.  */
	  memcpy (scn->data_base, scn->rawdata_base, size);
	}
    }
  else
    {
      xfct_t fp;

      scn->data_base = (char *) malloc (size);
      if (scn->data_base == NULL)
	{
	  __libelf_seterrno (ELF_E_NOMEM);
	  return;
	}

      /* Get the conversion function.  */
#if EV_NUM != 2
      fp = __elf_xfctstom[version - 1][__libelf_version - 1][eclass - 1][type];
#else
      fp = __elf_xfctstom[0][0][eclass - 1][type];
#endif

      fp (scn->data_base, scn->rawdata_base, size, 0);
    }

  scn->data_list.data.d.d_buf = scn->data_base;
  scn->data_list.data.d.d_size = size;
  scn->data_list.data.d.d_type = type;
  scn->data_list.data.d.d_off = scn->rawdata.d.d_off;
  scn->data_list.data.d.d_align = scn->rawdata.d.d_align;
  scn->data_list.data.d.d_version = scn->rawdata.d.d_version;

  scn->data_list.data.s = scn;
}


/* Store the information for the raw data in the `rawdata' element.  */
int
internal_function
__libelf_set_rawdata_wrlock (Elf_Scn *scn)
{
  size_t offset;
  size_t size;
  size_t align;
  int type;
  Elf *elf = scn->elf;

  if (elf->class == ELFCLASS32)
    {
      Elf32_Shdr *shdr
	= scn->shdr.e32 ?: __elf32_getshdr_wrlock (scn);

      if (shdr == NULL)
	/* Something went terribly wrong.  */
	return 1;

      offset = shdr->sh_offset;
      size = shdr->sh_size;
      type = shdr->sh_type;
      align = shdr->sh_addralign;
    }
  else
    {
      Elf64_Shdr *shdr
	= scn->shdr.e64 ?: __elf64_getshdr_wrlock (scn);

      if (shdr == NULL)
	/* Something went terribly wrong.  */
	return 1;

      offset = shdr->sh_offset;
      size = shdr->sh_size;
      type = shdr->sh_type;
      align = shdr->sh_addralign;
    }

  /* If the section has no data (for whatever reason), leave the `d_buf'
     pointer NULL.  */
  if (size != 0 && type != SHT_NOBITS)
    {
      /* First a test whether the section is valid at all.  */
      size_t entsize;

      if (type == SHT_HASH)
	{
	  GElf_Ehdr ehdr_mem;
	  GElf_Ehdr *ehdr = __gelf_getehdr_rdlock (elf, &ehdr_mem);
	  entsize = SH_ENTSIZE_HASH (ehdr);
	}
      else
	{
	  Elf_Type t = shtype_map[LIBELF_EV_IDX][TYPEIDX (type)];
	  if (t == ELF_T_VDEF || t == ELF_T_NHDR
	      || (t == ELF_T_GNUHASH && elf->class == ELFCLASS64))
	    entsize = 1;
	  else
	    entsize = __libelf_type_sizes[LIBELF_EV_IDX][elf->class - 1][t];
	}

      /* We assume it is an array of bytes if it is none of the structured
	 sections we know of.  */
      if (entsize == 0)
	entsize = 1;

      if (unlikely (size % entsize != 0))
	{
	  __libelf_seterrno (ELF_E_INVALID_DATA);
	  return 1;
	}

      /* We can use the mapped or loaded data if available.  */
      if (elf->map_address != NULL)
	{
	  /* First see whether the information in the section header is
	     valid and it does not ask for too much.  */
	  if (unlikely (offset + size > elf->maximum_size))
	    {
	      /* Something is wrong.  */
	      __libelf_seterrno (ELF_E_INVALID_SECTION_HEADER);
	      return 1;
	    }

	  scn->rawdata_base = scn->rawdata.d.d_buf
	    = (char *) elf->map_address + elf->start_offset + offset;
	}
      else if (likely (elf->fildes != -1))
	{
	  /* We have to read the data from the file.  Allocate the needed
	     memory.  */
	  scn->rawdata_base = scn->rawdata.d.d_buf
	    = (char *) malloc (size);
	  if (scn->rawdata.d.d_buf == NULL)
	    {
	      __libelf_seterrno (ELF_E_NOMEM);
	      return 1;
	    }

	  ssize_t n = pread_retry (elf->fildes, scn->rawdata.d.d_buf, size,
				   elf->start_offset + offset);
	  if (unlikely ((size_t) n != size))
	    {
	      /* Cannot read the data.  */
	      free (scn->rawdata.d.d_buf);
	      scn->rawdata_base = scn->rawdata.d.d_buf = NULL;
	      __libelf_seterrno (ELF_E_READ_ERROR);
	      return 1;
	    }
	}
      else
	{
	  /* The file descriptor is already closed, we cannot get the data
	     anymore.  */
	  __libelf_seterrno (ELF_E_FD_DISABLED);
	  return 1;
	}
    }

  scn->rawdata.d.d_size = size;
  /* Some broken ELF ABI for 64-bit machines use the wrong hash table
     entry size.  See elf-knowledge.h for more information.  */
  if (type == SHT_HASH && elf->class == ELFCLASS64)
    {
      GElf_Ehdr ehdr_mem;
      GElf_Ehdr *ehdr = __gelf_getehdr_rdlock (elf, &ehdr_mem);
      scn->rawdata.d.d_type
	= (SH_ENTSIZE_HASH (ehdr) == 4 ? ELF_T_WORD : ELF_T_XWORD);
    }
  else
    scn->rawdata.d.d_type = shtype_map[LIBELF_EV_IDX][TYPEIDX (type)];
  scn->rawdata.d.d_off = 0;
  scn->rawdata.d.d_align = align;
  if (elf->class == ELFCLASS32
      || (offsetof (struct Elf, state.elf32.ehdr)
	  == offsetof (struct Elf, state.elf64.ehdr)))
    scn->rawdata.d.d_version =
      elf->state.elf32.ehdr->e_ident[EI_VERSION];
  else
    scn->rawdata.d.d_version =
      elf->state.elf64.ehdr->e_ident[EI_VERSION];

  scn->rawdata.s = scn;

  scn->data_read = 1;

  /* We actually read data from the file.  At least we tried.  */
  scn->flags |= ELF_F_FILEDATA;

  return 0;
}

int
internal_function
__libelf_set_rawdata (Elf_Scn *scn)
{
  int result;

  if (scn == NULL)
    return 1;

  rwlock_wrlock (scn->elf->lock);
  result = __libelf_set_rawdata_wrlock (scn);
  rwlock_unlock (scn->elf->lock);

  return result;
}

Elf_Data *
internal_function
__elf_getdata_rdlock (scn, data)
     Elf_Scn *scn;
     Elf_Data *data;
{
  Elf_Data *result = NULL;
  Elf *elf;
  int locked = 0;

  if (scn == NULL)
    return NULL;

  if (unlikely (scn->elf->kind != ELF_K_ELF))
    {
      __libelf_seterrno (ELF_E_INVALID_HANDLE);
      return NULL;
    }

  /* We will need this multiple times later on.  */
  elf = scn->elf;

  /* If `data' is not NULL this means we are not addressing the initial
     data in the file.  But this also means this data is already read
     (since otherwise it is not possible to have a valid `data' pointer)
     and all the data structures are initialized as well.  In this case
     we can simply walk the list of data records.  */
  if (data != NULL)
    {
      Elf_Data_List *runp;

      /* It is not possible that if DATA is not NULL the first entry is
	 returned.  But this also means that there must be a first data
	 entry.  */
      if (scn->data_list_rear == NULL
	  /* The section the reference data is for must match the section
	     parameter.  */
	  || unlikely (((Elf_Data_Scn *) data)->s != scn))
	{
	  __libelf_seterrno (ELF_E_DATA_MISMATCH);
	  goto out;
	}

      /* We start searching with the first entry.  */
      runp = &scn->data_list;

      while (1)
	{
	  /* If `data' does not match any known record punt.  */
	  if (runp == NULL)
	    {
	      __libelf_seterrno (ELF_E_DATA_MISMATCH);
	      goto out;
	    }

	  if (&runp->data.d == data)
	    /* Found the entry.  */
	    break;

	  runp = runp->next;
	}

      /* Return the data for the next data record.  */
      result = runp->next ? &runp->next->data.d : NULL;
      goto out;
    }

  /* If the data for this section was not yet initialized do it now.  */
  if (scn->data_read == 0)
    {
      /* We cannot acquire a write lock while we are holding a read
         lock.  Therefore give up the read lock and then get the write
         lock.  But this means that the data could meanwhile be
         modified, therefore start the tests again.  */
      rwlock_unlock (elf->lock);
      rwlock_wrlock (elf->lock);
      locked = 1;

      /* Read the data from the file.  There is always a file (or
	 memory region) associated with this descriptor since
	 otherwise the `data_read' flag would be set.  */
      if (scn->data_read == 0 && __libelf_set_rawdata_wrlock (scn) != 0)
	/* Something went wrong.  The error value is already set.  */
	goto out;
    }

  /* At this point we know the raw data is available.  But it might be
     empty in case the section has size zero (for whatever reason).
     Now create the converted data in case this is necessary.  */
  if (scn->data_list_rear == NULL)
    {
      if (scn->rawdata.d.d_buf != NULL && scn->rawdata.d.d_size > 0)
	{
	  if (!locked)
	    {
	      rwlock_unlock (elf->lock);
	      rwlock_wrlock (elf->lock);
	      if (scn->data_list_rear != NULL)
		goto pass;
	    }

	  /* Convert according to the version and the type.   */
	  convert_data (scn, __libelf_version, elf->class,
			(elf->class == ELFCLASS32
			 || (offsetof (struct Elf, state.elf32.ehdr)
			     == offsetof (struct Elf, state.elf64.ehdr))
			 ? elf->state.elf32.ehdr->e_ident[EI_DATA]
			 : elf->state.elf64.ehdr->e_ident[EI_DATA]),
			scn->rawdata.d.d_size, scn->rawdata.d.d_type);
	}
      else
	{
	  /* This is an empty or NOBITS section.  There is no buffer but
	     the size information etc is important.  */
	  scn->data_list.data.d = scn->rawdata.d;
	  scn->data_list.data.s = scn;
	}

      scn->data_list_rear = &scn->data_list;
    }

  /* If no data is present we cannot return any.  */
  if (scn->data_list_rear != NULL)
  pass:
    /* Return the first data element in the list.  */
    result = &scn->data_list.data.d;

 out:
  return result;
}

Elf_Data *
elf_getdata (scn, data)
     Elf_Scn *scn;
     Elf_Data *data;
{
  Elf_Data *result;

  if (scn == NULL)
    return NULL;

  rwlock_rdlock (scn->elf->lock);
  result = __elf_getdata_rdlock (scn, data);
  rwlock_unlock (scn->elf->lock);

  return result;
}
INTDEF(elf_getdata)
