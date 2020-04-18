/* Register names and numbers for TILE-Gx DWARF.
   Copyright (C) 2012 Tilera Corporation
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

#include <stdio.h>
#include <string.h>
#include <dwarf.h>

#define BACKEND tilegx_
#include "libebl_CPU.h"

ssize_t
tilegx_register_info (Ebl *ebl __attribute__ ((unused)),
		    int regno, char *name, size_t namelen,
		    const char **prefix, const char **setname,
		    int *bits, int *type)
{
  if (name == NULL)
    return 65;

  if (regno < 0 || regno > 64 || namelen < 5)
    return -1;

  *prefix = "";
  *setname = "integer";
  *bits = 64;

  switch (regno)
    {
    case 0 ... 9:
      *type = DW_ATE_signed;
      name[0] = 'r';
      name[1] = regno + '0';
      namelen = 2;
      break;

    case 10 ... 52:
      *type = DW_ATE_signed;
      name[0] = 'r';
      name[1] = regno / 10 + '0';
      name[2] = regno % 10 + '0';
      namelen = 3;
      break;

    case 53:
      *type = DW_ATE_address;
      return stpcpy (name, "tp") + 1 - name;

    case 54:
      *type = DW_ATE_address;
      return stpcpy (name, "sp") + 1 - name;

    case 55:
      *type = DW_ATE_address;
      return stpcpy (name, "lr") + 1 - name;

    case 56:
      *type = DW_ATE_unsigned;
      return stpcpy (name, "sn") + 1 - name;

    case 57:
      *type = DW_ATE_unsigned;
      return stpcpy (name, "idn0") + 1 - name;

    case 58:
      *type = DW_ATE_unsigned;
      return stpcpy (name, "idn1") + 1 - name;

    case 59:
      *type = DW_ATE_unsigned;
      return stpcpy (name, "udn0") + 1 - name;

    case 60:
      *type = DW_ATE_unsigned;
      return stpcpy (name, "udn1") + 1 - name;

    case 61:
      *type = DW_ATE_unsigned;
      return stpcpy (name, "udn2") + 1 - name;

    case 62:
      *type = DW_ATE_unsigned;
      return stpcpy (name, "udn3") + 1 - name;

    case 63:
      *type = DW_ATE_unsigned;
      return stpcpy (name, "zero") + 1 - name;

    case 64:
      *type = DW_ATE_address;
      return stpcpy (name, "pc") + 1 - name;

    /* Can't happen.  */
    default:
      *setname = NULL;
      return 0;
    }

  name[namelen++] = '\0';
  return namelen;
}
