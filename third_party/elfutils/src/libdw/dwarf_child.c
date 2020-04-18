/* Return child of current DIE.
   Copyright (C) 2003-2011 Red Hat, Inc.
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

#include "libdwP.h"
#include <string.h>

/* Some arbitrary value not conflicting with any existing code.  */
#define INVALID 0xffffe444


unsigned char *
internal_function
__libdw_find_attr (Dwarf_Die *die, unsigned int search_name,
		   unsigned int *codep, unsigned int *formp)
{
  Dwarf *dbg = die->cu->dbg;
  const unsigned char *readp = (unsigned char *) die->addr;

  /* First we have to get the abbreviation code so that we can decode
     the data in the DIE.  */
  unsigned int abbrev_code;
  get_uleb128 (abbrev_code, readp);

  /* Find the abbreviation entry.  */
  Dwarf_Abbrev *abbrevp = die->abbrev;
  if (abbrevp == NULL)
    {
      abbrevp = __libdw_findabbrev (die->cu, abbrev_code);
      die->abbrev = abbrevp ?: DWARF_END_ABBREV;
    }
  if (unlikely (die->abbrev == DWARF_END_ABBREV))
    {
    invalid_dwarf:
      __libdw_seterrno (DWARF_E_INVALID_DWARF);
      return NULL;
    }

  /* Search the name attribute.  */
  unsigned char *const endp
    = ((unsigned char *) dbg->sectiondata[IDX_debug_abbrev]->d_buf
       + dbg->sectiondata[IDX_debug_abbrev]->d_size);

  const unsigned char *attrp = die->abbrev->attrp;
  while (1)
    {
      /* Are we still in bounds?  This test needs to be refined.  */
      if (unlikely (attrp + 1 >= endp))
	goto invalid_dwarf;

      /* Get attribute name and form.

	 XXX We don't check whether this reads beyond the end of the
	 section.  */
      unsigned int attr_name;
      get_uleb128 (attr_name, attrp);
      unsigned int attr_form;
      get_uleb128 (attr_form, attrp);

      /* We can stop if we found the attribute with value zero.  */
      if (attr_name == 0 && attr_form == 0)
	break;

      /* Is this the name attribute?  */
      if (attr_name == search_name && search_name != INVALID)
	{
	  if (codep != NULL)
	    *codep = attr_name;
	  if (formp != NULL)
	    *formp = attr_form;

	  return (unsigned char *) readp;
	}

      /* Skip over the rest of this attribute (if there is any).  */
      if (attr_form != 0)
	{
	  size_t len = __libdw_form_val_len (dbg, die->cu, attr_form, readp);

	  if (unlikely (len == (size_t) -1l))
	    {
	      readp = NULL;
	      break;
	    }

	  // XXX We need better boundary checks.
	  readp += len;
	}
    }

  // XXX Do we need other values?
  if (codep != NULL)
    *codep = INVALID;
  if (formp != NULL)
    *formp = INVALID;

  return (unsigned char *) readp;
}


int
dwarf_child (die, result)
     Dwarf_Die *die;
     Dwarf_Die *result;
{
  /* Ignore previous errors.  */
  if (die == NULL)
    return -1;

  /* Skip past the last attribute.  */
  void *addr = NULL;

  /* If we already know there are no children do not search.  */
  if (die->abbrev != DWARF_END_ABBREV
      && (die->abbrev == NULL || die->abbrev->has_children))
    addr = __libdw_find_attr (die, INVALID, NULL, NULL);
  if (unlikely (die->abbrev == (Dwarf_Abbrev *) -1l))
    return -1;

  /* Make sure the DIE really has children.  */
  if (! die->abbrev->has_children)
    /* There cannot be any children.  */
    return 1;

  if (addr == NULL)
    return -1;

  /* RESULT can be the same as DIE.  So preserve what we need.  */
  struct Dwarf_CU *cu = die->cu;
  Elf_Data *cu_sec = cu_data (cu);

  /* It's kosher (just suboptimal) to have a null entry first thing (7.5.3).
     So if this starts with ULEB128 of 0 (even with silly encoding of 0),
     it is a kosher null entry and we do not really have any children.  */
  const unsigned char *code = addr;
  const unsigned char *endp = (cu_sec->d_buf + cu_sec->d_size);
  while (1)
    {
      if (unlikely (code >= endp)) /* Truncated section.  */
	return 1;
      if (unlikely (*code == 0x80))
	++code;
      else
	break;
    }
  if (unlikely (*code == '\0'))
    return 1;

  /* Clear the entire DIE structure.  This signals we have not yet
     determined any of the information.  */
  memset (result, '\0', sizeof (Dwarf_Die));

  /* We have the address.  */
  result->addr = addr;

  /* Same CU as the parent.  */
  result->cu = cu;

  return 0;
}
INTDEF(dwarf_child)
