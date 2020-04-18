/* Copyright (C) 2005, 2013 Red Hat, Inc.
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

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <fcntl.h>
#include ELFUTILS_HEADER(dw)
#include <stdio.h>
#include <unistd.h>


static int
cb (Dwarf_Die *func, void *arg __attribute__ ((unused)))
{
  const char *file = dwarf_decl_file (func);
  int line = -1;
  dwarf_decl_line (func, &line);
  const char *fct = dwarf_diename (func);

  printf ("%s:%d:%s\n", file, line, fct);

  return DWARF_CB_ABORT;
}


int
main (int argc, char *argv[])
{
  for (int i = 1; i < argc; ++i)
    {
      int fd = open (argv[i], O_RDONLY);

      Dwarf *dbg = dwarf_begin (fd, DWARF_C_READ);
      if (dbg != NULL)
	{
	  Dwarf_Off off = 0;
	  size_t cuhl;
	  Dwarf_Off noff;

	  while (dwarf_nextcu (dbg, off, &noff, &cuhl, NULL, NULL, NULL) == 0)
	    {
	      Dwarf_Die die_mem;
	      Dwarf_Die *die = dwarf_offdie (dbg, off + cuhl, &die_mem);

	      /* Explicitly stop in the callback and then resume each time.  */
	      ptrdiff_t doff = 0;
	      do
		{
		  doff = dwarf_getfuncs (die, cb, NULL, doff);
		}
	      while (doff != 0 && dwarf_errno () == 0);

	      off = noff;
	    }

	  dwarf_end (dbg);
	}

      close (fd);
    }
}
