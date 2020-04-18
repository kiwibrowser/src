/* Get abbreviation at given offset.
   Copyright (C) 2003, 2004, 2005, 2006 Red Hat, Inc.
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

#include <assert.h>
#include <dwarf.h>
#include "libdwP.h"


Dwarf_Abbrev *
internal_function
__libdw_getabbrev (dbg, cu, offset, lengthp, result)
     Dwarf *dbg;
     struct Dwarf_CU *cu;
     Dwarf_Off offset;
     size_t *lengthp;
     Dwarf_Abbrev *result;
{
  /* Don't fail if there is not .debug_abbrev section.  */
  if (dbg->sectiondata[IDX_debug_abbrev] == NULL)
    return NULL;

  if (offset >= dbg->sectiondata[IDX_debug_abbrev]->d_size)
    {
      __libdw_seterrno (DWARF_E_INVALID_OFFSET);
      return NULL;
    }

  const unsigned char *abbrevp
    = (unsigned char *) dbg->sectiondata[IDX_debug_abbrev]->d_buf + offset;

  if (*abbrevp == '\0')
    /* We are past the last entry.  */
    return DWARF_END_ABBREV;

  /* 7.5.3 Abbreviations Tables

     [...] Each declaration begins with an unsigned LEB128 number
     representing the abbreviation code itself.  [...]  The
     abbreviation code is followed by another unsigned LEB128
     number that encodes the entry's tag.  [...]

     [...] Following the tag encoding is a 1-byte value that
     determines whether a debugging information entry using this
     abbreviation has child entries or not. [...]

     [...] Finally, the child encoding is followed by a series of
     attribute specifications. Each attribute specification
     consists of two parts. The first part is an unsigned LEB128
     number representing the attribute's name. The second part is
     an unsigned LEB128 number representing the attribute's form.  */
  const unsigned char *start_abbrevp = abbrevp;
  unsigned int code;
  get_uleb128 (code, abbrevp);

  /* Check whether this code is already in the hash table.  */
  bool foundit = false;
  Dwarf_Abbrev *abb = NULL;
  if (cu == NULL
      || (abb = Dwarf_Abbrev_Hash_find (&cu->abbrev_hash, code, NULL)) == NULL)
    {
      if (result == NULL)
	abb = libdw_typed_alloc (dbg, Dwarf_Abbrev);
      else
	abb = result;
    }
  else
    {
      foundit = true;

      assert (abb->offset == offset);

      /* If the caller doesn't need the length we are done.  */
      if (lengthp == NULL)
	goto out;
    }

  /* If there is already a value in the hash table we are going to
     overwrite its content.  This must not be a problem, since the
     content better be the same.  */
  abb->code = code;
  get_uleb128 (abb->tag, abbrevp);
  abb->has_children = *abbrevp++ == DW_CHILDREN_yes;
  abb->attrp = (unsigned char *) abbrevp;
  abb->offset = offset;

  /* Skip over all the attributes and count them while doing so.  */
  abb->attrcnt = 0;
  unsigned int attrname;
  unsigned int attrform;
  do
    {
      get_uleb128 (attrname, abbrevp);
      get_uleb128 (attrform, abbrevp);
    }
  while (attrname != 0 && attrform != 0 && ++abb->attrcnt);

  /* Return the length to the caller if she asked for it.  */
  if (lengthp != NULL)
    *lengthp = abbrevp - start_abbrevp;

  /* Add the entry to the hash table.  */
  if (cu != NULL && ! foundit)
    (void) Dwarf_Abbrev_Hash_insert (&cu->abbrev_hash, abb->code, abb);

 out:
  return abb;
}


Dwarf_Abbrev *
dwarf_getabbrev (die, offset, lengthp)
     Dwarf_Die *die;
     Dwarf_Off offset;
     size_t *lengthp;
{
  return __libdw_getabbrev (die->cu->dbg, die->cu,
			    die->cu->orig_abbrev_offset + offset, lengthp,
			    NULL);
}
