/* Enumerate the PC ranges covered by a DIE.
   Copyright (C) 2005, 2007, 2009 Red Hat, Inc.
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

#include "libdwP.h"
#include <dwarf.h>
#include <assert.h>

/* Read up begin/end pair and increment read pointer.
    - If it's normal range record, set up `*beginp' and `*endp' and return 0.
    - If it's base address selection record, set up `*basep' and return 1.
    - If it's end of rangelist, don't set anything and return 2
    - If an error occurs, don't set anything and return -1.  */
internal_function int
__libdw_read_begin_end_pair_inc (Dwarf *dbg, int sec_index,
				 unsigned char **addrp, int width,
				 Dwarf_Addr *beginp, Dwarf_Addr *endp,
				 Dwarf_Addr *basep)
{
  Dwarf_Addr escape = (width == 8 ? (Elf64_Addr) -1
		       : (Elf64_Addr) (Elf32_Addr) -1);
  Dwarf_Addr begin;
  Dwarf_Addr end;

  unsigned char *addr = *addrp;
  bool begin_relocated = READ_AND_RELOCATE (__libdw_relocate_address, begin);
  bool end_relocated = READ_AND_RELOCATE (__libdw_relocate_address, end);
  *addrp = addr;

  /* Unrelocated escape for begin means base address selection.  */
  if (begin == escape && !begin_relocated)
    {
      if (unlikely (end == escape))
	{
	  __libdw_seterrno (DWARF_E_INVALID_DWARF);
	  return -1;
	}

      if (basep != NULL)
	*basep = end;
      return 1;
    }

  /* Unrelocated pair of zeroes means end of range list.  */
  if (begin == 0 && end == 0 && !begin_relocated && !end_relocated)
    return 2;

  /* Don't check for begin_relocated == end_relocated.  Serve the data
     to the client even though it may be buggy.  */
  *beginp = begin;
  *endp = end;

  return 0;
}

ptrdiff_t
dwarf_ranges (Dwarf_Die *die, ptrdiff_t offset, Dwarf_Addr *basep,
	      Dwarf_Addr *startp, Dwarf_Addr *endp)
{
  if (die == NULL)
    return -1;

  if (offset == 0
      /* Usually there is a single contiguous range.  */
      && INTUSE(dwarf_highpc) (die, endp) == 0
      && INTUSE(dwarf_lowpc) (die, startp) == 0)
    /* A offset into .debug_ranges will never be 1, it must be at least a
       multiple of 4.  So we can return 1 as a special case value to mark
       there are no ranges to look for on the next call.  */
    return 1;

  if (offset == 1)
    return 0;

  /* We have to look for a noncontiguous range.  */

  const Elf_Data *d = die->cu->dbg->sectiondata[IDX_debug_ranges];
  if (d == NULL && offset != 0)
    {
      __libdw_seterrno (DWARF_E_NO_DEBUG_RANGES);
      return -1;
    }

  unsigned char *readp;
  unsigned char *readendp;
  if (offset == 0)
    {
      Dwarf_Attribute attr_mem;
      Dwarf_Attribute *attr = INTUSE(dwarf_attr) (die, DW_AT_ranges,
						  &attr_mem);
      if (attr == NULL)
	/* No PC attributes in this DIE at all, so an empty range list.  */
	return 0;

      Dwarf_Word start_offset;
      if ((readp = __libdw_formptr (attr, IDX_debug_ranges,
				    DWARF_E_NO_DEBUG_RANGES,
				    &readendp, &start_offset)) == NULL)
	return -1;

      offset = start_offset;
      assert ((Dwarf_Word) offset == start_offset);

      /* Fetch the CU's base address.  */
      Dwarf_Die cudie = CUDIE (attr->cu);

      /* Find the base address of the compilation unit.  It will
	 normally be specified by DW_AT_low_pc.  In DWARF-3 draft 4,
	 the base address could be overridden by DW_AT_entry_pc.  It's
	 been removed, but GCC emits DW_AT_entry_pc and not DW_AT_lowpc
	 for compilation units with discontinuous ranges.  */
      if (unlikely (INTUSE(dwarf_lowpc) (&cudie, basep) != 0)
	  && INTUSE(dwarf_formaddr) (INTUSE(dwarf_attr) (&cudie,
							 DW_AT_entry_pc,
							 &attr_mem),
				     basep) != 0)
	{
	  if (INTUSE(dwarf_errno) () == 0)
	    {
	    invalid:
	      __libdw_seterrno (DWARF_E_INVALID_DWARF);
	    }
	  return -1;
	}
    }
  else
    {
      if (__libdw_offset_in_section (die->cu->dbg,
				     IDX_debug_ranges, offset, 1))
	return -1l;

      readp = d->d_buf + offset;
      readendp = d->d_buf + d->d_size;
    }

 next:
  if (readendp - readp < die->cu->address_size * 2)
    goto invalid;

  Dwarf_Addr begin;
  Dwarf_Addr end;

  switch (__libdw_read_begin_end_pair_inc (die->cu->dbg, IDX_debug_ranges,
					   &readp, die->cu->address_size,
					   &begin, &end, basep))
    {
    case 0:
      break;
    case 1:
      goto next;
    case 2:
      return 0;
    default:
      return -1l;
    }

  /* We have an address range entry.  */
  *startp = *basep + begin;
  *endp = *basep + end;
  return readp - (unsigned char *) d->d_buf;
}
INTDEF (dwarf_ranges)
