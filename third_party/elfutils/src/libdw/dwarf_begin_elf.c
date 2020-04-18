/* Create descriptor from ELF descriptor for processing file.
   Copyright (C) 2002-2011 Red Hat, Inc.
   This file is part of elfutils.
   Written by Ulrich Drepper <drepper@redhat.com>, 2002.

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
#include <inttypes.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "libdwP.h"

#if USE_ZLIB
# include <endian.h>
# define crc32		loser_crc32
# include <zlib.h>
# undef crc32
#endif


/* Section names.  */
static const char dwarf_scnnames[IDX_last][17] =
{
  [IDX_debug_info] = ".debug_info",
  [IDX_debug_types] = ".debug_types",
  [IDX_debug_abbrev] = ".debug_abbrev",
  [IDX_debug_aranges] = ".debug_aranges",
  [IDX_debug_line] = ".debug_line",
  [IDX_debug_frame] = ".debug_frame",
  [IDX_debug_loc] = ".debug_loc",
  [IDX_debug_pubnames] = ".debug_pubnames",
  [IDX_debug_str] = ".debug_str",
  [IDX_debug_macinfo] = ".debug_macinfo",
  [IDX_debug_macro] = ".debug_macro",
  [IDX_debug_ranges] = ".debug_ranges"
};
#define ndwarf_scnnames (sizeof (dwarf_scnnames) / sizeof (dwarf_scnnames[0]))

#ifdef ENABLE_DWZ
internal_function int
__check_build_id (Dwarf *dw, const uint8_t *build_id, const size_t id_len)
{
  if (dw == NULL)
    return -1;

  Elf *elf = dw->elf;
  Elf_Scn *scn = elf_nextscn (elf, NULL);
  if (scn == NULL)
    return -1;

  do
    {
      GElf_Shdr shdr_mem;
      GElf_Shdr *shdr = gelf_getshdr (scn, &shdr_mem);
      if (likely (shdr != NULL) && shdr->sh_type == SHT_NOTE)
	{
	  size_t pos = 0;
	  GElf_Nhdr nhdr;
	  size_t name_pos;
	  size_t desc_pos;
	  Elf_Data *data = elf_getdata (scn, NULL);
	  while ((pos = gelf_getnote (data, pos, &nhdr, &name_pos,
				      &desc_pos)) > 0)
	    if (nhdr.n_type == NT_GNU_BUILD_ID
	        && nhdr.n_namesz == sizeof "GNU"
		&& ! memcmp (data->d_buf + name_pos, "GNU", sizeof "GNU"))
	      return (nhdr.n_descsz == id_len
		      && ! memcmp (data->d_buf + desc_pos,
				   build_id, id_len)) ? 0 : 1;
        }
      }
    while ((scn = elf_nextscn (elf, scn)) != NULL);

  return -1;
}

/* Try to open an debug alt link by name, checking build_id.
   Marks free_alt on success, return NULL on failure.  */
static Dwarf *
try_debugaltlink (Dwarf *result, const char *try_name,
		   const uint8_t *build_id, const size_t id_len)
{
  int fd = open (try_name, O_RDONLY);
  if (fd > 0)
    {
      result->alt_dwarf = INTUSE (dwarf_begin) (fd, DWARF_C_READ);
      if (result->alt_dwarf != NULL)
	{
	  Elf *elf = result->alt_dwarf->elf;
	  if (__check_build_id (result->alt_dwarf, build_id, id_len) == 0
	      && elf_cntl (elf, ELF_C_FDREAD) == 0)
	    {
	      close (fd);
	      result->free_alt = 1;
	      return result;
	    }
	  INTUSE (dwarf_end) (result->alt_dwarf);
	}
      close (fd);
    }
  return NULL;
}

/* For dwz multifile support, ignore if it looks wrong.  */
static Dwarf *
open_debugaltlink (Dwarf *result, const char *alt_name,
		   const uint8_t *build_id, const size_t id_len)
{
  /* First try the name itself, it is either an absolute path or
     a relative one.  Sadly we don't know relative from where at
     this point.  */
  if (try_debugaltlink (result, alt_name, build_id, id_len) != NULL)
    return result;

  /* Lets try based on the build-id.  This is somewhat distro specific,
     we are following the Fedora implementation described at
  https://fedoraproject.org/wiki/Releases/FeatureBuildId#Find_files_by_build_ID
   */
#define DEBUG_PREFIX "/usr/lib/debug/.build-id/"
#define PREFIX_LEN sizeof (DEBUG_PREFIX)
  char id_name[PREFIX_LEN + 1 + id_len * 2 + sizeof ".debug" - 1];
  strcpy (id_name, DEBUG_PREFIX);
  int n = snprintf (&id_name[PREFIX_LEN  - 1],
		    4, "%02" PRIx8 "/", (uint8_t) build_id[0]);
  assert (n == 3);
  for (size_t i = 1; i < id_len; ++i)
    {
      n = snprintf (&id_name[PREFIX_LEN - 1 + 3 + (i - 1) * 2],
		    3, "%02" PRIx8, (uint8_t) build_id[i]);
      assert (n == 2);
    }
  strcpy (&id_name[PREFIX_LEN - 1 + 3 + (id_len - 1) * 2],
	  ".debug");

  if (try_debugaltlink (result, id_name, build_id, id_len))
    return result;

  /* Everything failed, mark this Dwarf as not having an alternate,
     but don't fail the load.  The user may want to set it by hand
     before usage.  */
  result->alt_dwarf = NULL;
  return result;
}
#endif /* ENABLE_DWZ */

static Dwarf *
check_section (Dwarf *result, GElf_Ehdr *ehdr, Elf_Scn *scn, bool inscngrp)
{
  GElf_Shdr shdr_mem;
  GElf_Shdr *shdr;

  /* Get the section header data.  */
  shdr = gelf_getshdr (scn, &shdr_mem);
  if (shdr == NULL)
    /* This should never happen.  If it does something is
       wrong in the libelf library.  */
    abort ();

  /* Ignore any SHT_NOBITS sections.  Debugging sections should not
     have been stripped, but in case of a corrupt file we won't try
     to look at the missing data.  */
  if (unlikely (shdr->sh_type == SHT_NOBITS))
    return result;

  /* Make sure the section is part of a section group only iff we
     really need it.  If we are looking for the global (= non-section
     group debug info) we have to ignore all the info in section
     groups.  If we are looking into a section group we cannot look at
     a section which isn't part of the section group.  */
  if (! inscngrp && (shdr->sh_flags & SHF_GROUP) != 0)
    /* Ignore the section.  */
    return result;


  /* We recognize the DWARF section by their names.  This is not very
     safe and stable but the best we can do.  */
  const char *scnname = elf_strptr (result->elf, ehdr->e_shstrndx,
				    shdr->sh_name);
  if (scnname == NULL)
    {
      /* The section name must be valid.  Otherwise is the ELF file
	 invalid.  */
      __libdw_free_zdata (result);
      Dwarf_Sig8_Hash_free (&result->sig8_hash);
      __libdw_seterrno (DWARF_E_INVALID_ELF);
      free (result);
      return NULL;
    }

#ifdef ENABLE_DWZ
  /* For dwz multifile support, ignore if it looks wrong.  */
  if (strcmp (scnname, ".gnu_debugaltlink") == 0)
    {
      Elf_Data *data = elf_getdata (scn, NULL);
      if (data != NULL && data->d_size != 0)
	{
	  const char *alt_name = data->d_buf;
	  const void *build_id = memchr (data->d_buf, '\0', data->d_size);
	  const int id_len = data->d_size - (build_id - data->d_buf + 1);
	  if (alt_name && build_id && id_len > 0)
	    return open_debugaltlink (result, alt_name, build_id + 1, id_len);
	}
    }
#endif /* ENABLE_DWZ */

  /* Recognize the various sections.  Most names start with .debug_.  */
  size_t cnt;
  for (cnt = 0; cnt < ndwarf_scnnames; ++cnt)
    if (strcmp (scnname, dwarf_scnnames[cnt]) == 0)
      {
	/* Found it.  Remember where the data is.  */
	if (unlikely (result->sectiondata[cnt] != NULL))
	  /* A section appears twice.  That's bad.  We ignore the section.  */
	  break;

	/* Get the section data.  */
	Elf_Data *data = elf_getdata (scn, NULL);
	if (data != NULL && data->d_size != 0)
	  /* Yep, there is actually data available.  */
	  result->sectiondata[cnt] = data;

	break;
      }
#if USE_ZLIB
    else if (scnname[0] == '.' && scnname[1] == 'z'
	     && strcmp (&scnname[2], &dwarf_scnnames[cnt][1]) == 0)
      {
	/* A compressed section.  */

	if (unlikely (result->sectiondata[cnt] != NULL))
	  /* A section appears twice.  That's bad.  We ignore the section.  */
	  break;

	/* Get the section data.  */
	Elf_Data *data = elf_getdata (scn, NULL);
	if (data != NULL && data->d_size != 0)
	  {
	    /* There is a 12-byte header of "ZLIB" followed by
	       an 8-byte big-endian size.  */

	    if (unlikely (data->d_size < 4 + 8)
		|| unlikely (memcmp (data->d_buf, "ZLIB", 4) != 0))
	      break;

	    uint64_t size;
	    memcpy (&size, data->d_buf + 4, sizeof size);
	    size = be64toh (size);

	    Elf_Data *zdata = malloc (sizeof (Elf_Data) + size);
	    if (unlikely (zdata == NULL))
	      break;

	    zdata->d_buf = &zdata[1];
	    zdata->d_type = ELF_T_BYTE;
	    zdata->d_version = EV_CURRENT;
	    zdata->d_size = size;
	    zdata->d_off = 0;
	    zdata->d_align = 1;

	    z_stream z =
	      {
		.next_in = data->d_buf + 4 + 8,
		.avail_in = data->d_size - 4 - 8,
		.next_out = zdata->d_buf,
		.avail_out = zdata->d_size
	      };
	    int zrc = inflateInit (&z);
	    while (z.avail_in > 0 && likely (zrc == Z_OK))
	      {
		z.next_out = zdata->d_buf + (zdata->d_size - z.avail_out);
		zrc = inflate (&z, Z_FINISH);
		if (unlikely (zrc != Z_STREAM_END))
		  {
		    zrc = Z_DATA_ERROR;
		    break;
		  }
		zrc = inflateReset (&z);
	      }
	    if (likely (zrc == Z_OK))
	      zrc = inflateEnd (&z);

	    if (unlikely (zrc != Z_OK) || unlikely (z.avail_out != 0))
	      free (zdata);
	    else
	      {
		result->sectiondata[cnt] = zdata;
		result->sectiondata_gzip_mask |= 1U << cnt;
	      }
	  }

	break;
      }
#endif

  return result;
}


/* Check whether all the necessary DWARF information is available.  */
static Dwarf *
valid_p (Dwarf *result)
{
  /* We looked at all the sections.  Now determine whether all the
     sections with debugging information we need are there.

     XXX Which sections are absolutely necessary?  Add tests if
     necessary.  For now we require only .debug_info.  Hopefully this
     is correct.  */
  if (likely (result != NULL)
      && unlikely (result->sectiondata[IDX_debug_info] == NULL))
    {
      __libdw_free_zdata (result);
      Dwarf_Sig8_Hash_free (&result->sig8_hash);
      __libdw_seterrno (DWARF_E_NO_DWARF);
      free (result);
      result = NULL;
    }

  return result;
}


static Dwarf *
global_read (Dwarf *result, Elf *elf, GElf_Ehdr *ehdr)
{
  Elf_Scn *scn = NULL;

  while (result != NULL && (scn = elf_nextscn (elf, scn)) != NULL)
    result = check_section (result, ehdr, scn, false);

  return valid_p (result);
}


static Dwarf *
scngrp_read (Dwarf *result, Elf *elf, GElf_Ehdr *ehdr, Elf_Scn *scngrp)
{
  /* SCNGRP is the section descriptor for a section group which might
     contain debug sections.  */
  Elf_Data *data = elf_getdata (scngrp, NULL);
  if (data == NULL)
    {
      /* We cannot read the section content.  Fail!  */
      __libdw_free_zdata (result);
      Dwarf_Sig8_Hash_free (&result->sig8_hash);
      free (result);
      return NULL;
    }

  /* The content of the section is a number of 32-bit words which
     represent section indices.  The first word is a flag word.  */
  Elf32_Word *scnidx = (Elf32_Word *) data->d_buf;
  size_t cnt;
  for (cnt = 1; cnt * sizeof (Elf32_Word) <= data->d_size; ++cnt)
    {
      Elf_Scn *scn = elf_getscn (elf, scnidx[cnt]);
      if (scn == NULL)
	{
	  /* A section group refers to a non-existing section.  Should
	     never happen.  */
	  __libdw_free_zdata (result);
	  Dwarf_Sig8_Hash_free (&result->sig8_hash);
	  __libdw_seterrno (DWARF_E_INVALID_ELF);
	  free (result);
	  return NULL;
	}

      result = check_section (result, ehdr, scn, true);
      if (result == NULL)
	break;
    }

  return valid_p (result);
}


Dwarf *
dwarf_begin_elf (elf, cmd, scngrp)
     Elf *elf;
     Dwarf_Cmd cmd;
     Elf_Scn *scngrp;
{
  GElf_Ehdr *ehdr;
  GElf_Ehdr ehdr_mem;

  /* Get the ELF header of the file.  We need various pieces of
     information from it.  */
  ehdr = gelf_getehdr (elf, &ehdr_mem);
  if (ehdr == NULL)
    {
      if (elf_kind (elf) != ELF_K_ELF)
	__libdw_seterrno (DWARF_E_NOELF);
      else
	__libdw_seterrno (DWARF_E_GETEHDR_ERROR);

      return NULL;
    }


  /* Default memory allocation size.  */
  size_t mem_default_size = sysconf (_SC_PAGESIZE) - 4 * sizeof (void *);

  /* Allocate the data structure.  */
  Dwarf *result = (Dwarf *) calloc (1, sizeof (Dwarf) + mem_default_size);
  if (unlikely (result == NULL)
      || unlikely (Dwarf_Sig8_Hash_init (&result->sig8_hash, 11) < 0))
    {
      free (result);
      __libdw_seterrno (DWARF_E_NOMEM);
      return NULL;
    }

  /* Fill in some values.  */
  if ((BYTE_ORDER == LITTLE_ENDIAN && ehdr->e_ident[EI_DATA] == ELFDATA2MSB)
      || (BYTE_ORDER == BIG_ENDIAN && ehdr->e_ident[EI_DATA] == ELFDATA2LSB))
    result->other_byte_order = true;

  result->elf = elf;

  /* Initialize the memory handling.  */
  result->mem_default_size = mem_default_size;
  result->oom_handler = __libdw_oom;
  result->mem_tail = (struct libdw_memblock *) (result + 1);
  result->mem_tail->size = (result->mem_default_size
			    - offsetof (struct libdw_memblock, mem));
  result->mem_tail->remaining = result->mem_tail->size;
  result->mem_tail->prev = NULL;

  if (cmd == DWARF_C_READ || cmd == DWARF_C_RDWR)
    {
      /* If the caller provides a section group we get the DWARF
	 sections only from this setion group.  Otherwise we search
	 for the first section with the required name.  Further
	 sections with the name are ignored.  The DWARF specification
	 does not really say this is allowed.  */
      if (scngrp == NULL)
	return global_read (result, elf, ehdr);
      else
	return scngrp_read (result, elf, ehdr, scngrp);
    }
  else if (cmd == DWARF_C_WRITE)
    {
      Dwarf_Sig8_Hash_free (&result->sig8_hash);
      __libdw_seterrno (DWARF_E_UNIMPL);
      free (result);
      return NULL;
    }

  Dwarf_Sig8_Hash_free (&result->sig8_hash);
  __libdw_seterrno (DWARF_E_INVALID_CMD);
  free (result);
  return NULL;
}
INTDEF(dwarf_begin_elf)
