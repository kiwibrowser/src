/* Get macro information.
   Copyright (C) 2002-2009 Red Hat, Inc.
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

#include <dwarf.h>
#include <string.h>

#include <libdwP.h>


ptrdiff_t
dwarf_getmacros (die, callback, arg, offset)
     Dwarf_Die *die;
     int (*callback) (Dwarf_Macro *, void *);
     void *arg;
     ptrdiff_t offset;
{
  if (die == NULL)
    return -1;

  Elf_Data *d = die->cu->dbg->sectiondata[IDX_debug_macinfo];
  if (unlikely (d == NULL) || unlikely (d->d_buf == NULL))
    {
      __libdw_seterrno (DWARF_E_NO_ENTRY);
      return -1;
    }

  if (offset == 0)
    {
      /* Get the appropriate attribute.  */
      Dwarf_Attribute attr;
      if (INTUSE(dwarf_attr) (die, DW_AT_macro_info, &attr) == NULL)
	return -1;

      /* Offset into the .debug_macinfo section.  */
      Dwarf_Word macoff;
      if (INTUSE(dwarf_formudata) (&attr, &macoff) != 0)
	return -1;

      offset = macoff;
    }
  if (unlikely (offset > (ptrdiff_t) d->d_size))
    goto invalid;

  const unsigned char *readp = d->d_buf + offset;
  const unsigned char *readendp = d->d_buf + d->d_size;

  if (readp == readendp)
    return 0;

  while (readp < readendp)
    {
      unsigned int opcode = *readp++;
      unsigned int u128;
      unsigned int u128_2 = 0;
      const char *str = NULL;
      const unsigned char *endp;

      switch (opcode)
	{
	case DW_MACINFO_define:
	case DW_MACINFO_undef:
	case DW_MACINFO_vendor_ext:
	  /*  For the first two opcodes the parameters are
	        line, string
	      For the latter
	        number, string.
	      We can treat these cases together.  */
	  get_uleb128 (u128, readp);

	  endp = memchr (readp, '\0', readendp - readp);
	  if (endp == NULL)
	    goto invalid;

	  str = (char *) readp;
	  readp = endp + 1;
	  break;

	case DW_MACINFO_start_file:
	  /* The two parameters are line and file index.  */
	  get_uleb128 (u128, readp);
	  get_uleb128 (u128_2, readp);
	  break;

	case DW_MACINFO_end_file:
	  /* No parameters for this one.  */
	  u128 = 0;
	  break;

	case 0:
	  /* Nothing more to do.  */
	  return 0;

	default:
	  goto invalid;
	}

      Dwarf_Macro mac;
      mac.opcode = opcode;
      mac.param1 = u128;
      if (str == NULL)
	mac.param2.u = u128_2;
      else
	mac.param2.s = str;

      if (callback (&mac, arg) != DWARF_CB_OK)
	return readp - (const unsigned char *) d->d_buf;
    }

  /* If we come here the termination of the data for the CU is not
     present.  */
 invalid:
  __libdw_seterrno (DWARF_E_INVALID_DWARF);
  return -1;
}
