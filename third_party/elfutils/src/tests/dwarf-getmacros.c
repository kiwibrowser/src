/* Test program for dwfl_module_return_value_location.
   Copyright (C) 2009 Red Hat, Inc.
   This file is part of elfutils.

   This file is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   elfutils is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

#include <config.h>
#include ELFUTILS_HEADER(dw)
#include <dwarf.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

int
main (int argc __attribute__ ((unused)), char *argv[])
{
  const char *name = argv[1];
  ptrdiff_t cuoff = strtol (argv[2], NULL, 0);

  int fd = open (name, O_RDONLY);
  Dwarf *dbg = dwarf_begin (fd, DWARF_C_READ);

  Dwarf_Die cudie_mem, *cudie = dwarf_offdie (dbg, cuoff, &cudie_mem);
  int mac (Dwarf_Macro *macro, void *data __attribute__ ((unused)))
  {
    unsigned int opcode;
    dwarf_macro_opcode (macro, &opcode);
    if (opcode == DW_MACINFO_define)
      {
	const char *value;
	dwarf_macro_param2 (macro, NULL, &value);
	puts (value);
      }
    return DWARF_CB_ABORT;
  }

  ptrdiff_t off = 0;
  while ((off = dwarf_getmacros (cudie, mac, NULL, off)) > 0)
    ;

  return 0;
}
