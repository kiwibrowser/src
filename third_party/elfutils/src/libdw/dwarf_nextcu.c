/* Advance to next CU header.
   Copyright (C) 2002-2010 Red Hat, Inc.
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

#include <libdwP.h>
#include <dwarf.h>


int
dwarf_next_unit (dwarf, off, next_off, header_sizep, versionp, abbrev_offsetp,
		 address_sizep, offset_sizep, type_signaturep, type_offsetp)
     Dwarf *dwarf;
     Dwarf_Off off;
     Dwarf_Off *next_off;
     size_t *header_sizep;
     Dwarf_Half *versionp;
     Dwarf_Off *abbrev_offsetp;
     uint8_t *address_sizep;
     uint8_t *offset_sizep;
     uint64_t *type_signaturep;
     Dwarf_Off *type_offsetp;
{
  const bool debug_types = type_signaturep != NULL;
  const size_t sec_idx = debug_types ? IDX_debug_types : IDX_debug_info;

  /* Maybe there has been an error before.  */
  if (dwarf == NULL)
    return -1;

  /* If we reached the end before don't do anything.  */
  if (off == (Dwarf_Off) -1l
      || unlikely (dwarf->sectiondata[sec_idx] == NULL)
      /* Make sure there is enough space in the .debug_info section
	 for at least the initial word.  We cannot test the rest since
	 we don't know yet whether this is a 64-bit object or not.  */
      || unlikely (off + 4 >= dwarf->sectiondata[sec_idx]->d_size))
    {
      *next_off = (Dwarf_Off) -1l;
      return 1;
    }

  /* This points into the .debug_info section to the beginning of the
     CU entry.  */
  const unsigned char *data = dwarf->sectiondata[sec_idx]->d_buf;
  const unsigned char *bytes = data + off;

  /* The format of the CU header is described in dwarf2p1 7.5.1:

     1.  A 4-byte or 12-byte unsigned integer representing the length
	 of the .debug_info contribution for that compilation unit, not
	 including the length field itself. In the 32-bit DWARF format,
	 this is a 4-byte unsigned integer (which must be less than
	 0xfffffff0); in the 64-bit DWARF format, this consists of the
	 4-byte value 0xffffffff followed by an 8-byte unsigned integer
	 that gives the actual length (see Section 7.2.2).

      2. A 2-byte unsigned integer representing the version of the
	 DWARF information for that compilation unit. For DWARF Version
	 2.1, the value in this field is 2.

      3. A 4-byte or 8-byte unsigned offset into the .debug_abbrev
	 section. This offset associates the compilation unit with a
	 particular set of debugging information entry abbreviations. In
	 the 32-bit DWARF format, this is a 4-byte unsigned length; in
	 the 64-bit DWARF format, this is an 8-byte unsigned length (see
	 Section 7.4).

      4. A 1-byte unsigned integer representing the size in bytes of
	 an address on the target architecture. If the system uses
	 segmented addressing, this value represents the size of the
	 offset portion of an address.  */
  uint64_t length = read_4ubyte_unaligned_inc (dwarf, bytes);
  size_t offset_size = 4;
  /* Lengths of 0xfffffff0 - 0xffffffff are escape codes.  Oxffffffff is
     used to indicate that 64-bit dwarf information is being used, the
     other values are currently reserved.  */
  if (length == DWARF3_LENGTH_64_BIT)
    offset_size = 8;
  else if (unlikely (length >= DWARF3_LENGTH_MIN_ESCAPE_CODE
		     && length <= DWARF3_LENGTH_MAX_ESCAPE_CODE))
    {
    invalid:
      __libdw_seterrno (DWARF_E_INVALID_DWARF);
      return -1;
    }

  /* Now we know how large the header is.  */
  if (unlikely (DIE_OFFSET_FROM_CU_OFFSET (off, offset_size, debug_types)
		>= dwarf->sectiondata[sec_idx]->d_size))
    {
      *next_off = -1;
      return 1;
    }

  if (length == DWARF3_LENGTH_64_BIT)
    /* This is a 64-bit DWARF format.  */
    length = read_8ubyte_unaligned_inc (dwarf, bytes);

  /* Read the version stamp.  Always a 16-bit value.  */
  uint_fast16_t version = read_2ubyte_unaligned_inc (dwarf, bytes);

  /* Get offset in .debug_abbrev.  Note that the size of the entry
     depends on whether this is a 32-bit or 64-bit DWARF definition.  */
  uint64_t abbrev_offset;
  if (__libdw_read_offset_inc (dwarf, sec_idx, &bytes, offset_size,
			       &abbrev_offset, IDX_debug_abbrev, 0))
    return -1;

  /* The address size.  Always an 8-bit value.  */
  uint8_t address_size = *bytes++;

  if (debug_types)
    {
      uint64_t type_sig8 = read_8ubyte_unaligned_inc (dwarf, bytes);

      Dwarf_Off type_offset;
      if (__libdw_read_offset_inc (dwarf, sec_idx, &bytes, offset_size,
				   &type_offset, sec_idx, 0))
	return -1;

      /* Validate that the TYPE_OFFSET points past the header.  */
      if (unlikely (type_offset < (size_t) (bytes - (data + off))))
	goto invalid;

      *type_signaturep = type_sig8;
      if (type_offsetp != NULL)
	*type_offsetp = type_offset;
    }

  /* Store the header length.  */
  if (header_sizep != NULL)
    *header_sizep = bytes - (data + off);

  if (versionp != NULL)
    *versionp = version;

  if (abbrev_offsetp != NULL)
    *abbrev_offsetp = abbrev_offset;

  if (address_sizep != NULL)
    *address_sizep = address_size;

  /* Store the offset size.  */
  if (offset_sizep != NULL)
    *offset_sizep = offset_size;

  /* See definition of DIE_OFFSET_FROM_CU_OFFSET macro
     for an explanation of the trick in this expression.  */
  *next_off = off + 2 * offset_size - 4 + length;

  return 0;
}
INTDEF(dwarf_next_unit)

int
dwarf_nextcu (dwarf, off, next_off, header_sizep, abbrev_offsetp,
	      address_sizep, offset_sizep)
     Dwarf *dwarf;
     Dwarf_Off off;
     Dwarf_Off *next_off;
     size_t *header_sizep;
     Dwarf_Off *abbrev_offsetp;
     uint8_t *address_sizep;
     uint8_t *offset_sizep;
{
  return INTUSE(dwarf_next_unit) (dwarf, off, next_off, header_sizep, NULL,
				  abbrev_offsetp, address_sizep, offset_sizep,
				  NULL, NULL);
}
INTDEF(dwarf_nextcu)
