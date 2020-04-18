/* Find the debuginfo file for a module from its build ID.
   Copyright (C) 2007, 2009 Red Hat, Inc.
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

#include "libdwflP.h"
#include <unistd.h>


int
dwfl_build_id_find_debuginfo (Dwfl_Module *mod,
			      void **userdata __attribute__ ((unused)),
			      const char *modname __attribute__ ((unused)),
			      Dwarf_Addr base __attribute__ ((unused)),
			      const char *file __attribute__ ((unused)),
			      const char *debuglink __attribute__ ((unused)),
			      GElf_Word crc __attribute__ ((unused)),
			      char **debuginfo_file_name)
{
  int fd = -1;
  const unsigned char *bits;
  GElf_Addr vaddr;
  if (INTUSE(dwfl_module_build_id) (mod, &bits, &vaddr) > 0)
    fd = __libdwfl_open_by_build_id (mod, true, debuginfo_file_name);
  if (fd >= 0)
    {
      /* We need to open an Elf handle on the file so we can check its
	 build ID note for validation.  Backdoor the handle into the
	 module data structure since we had to open it early anyway.  */
      Dwfl_Error error = __libdw_open_file (&fd, &mod->debug.elf, true, false);
      if (error != DWFL_E_NOERROR)
	__libdwfl_seterrno (error);
      else if (likely (__libdwfl_find_build_id (mod, false,
						mod->debug.elf) == 2))
	{
	  /* Also backdoor the gratuitous flag.  */
	  mod->debug.valid = true;
	  return fd;
	}
      else
	{
	  /* A mismatch!  */
	  elf_end (mod->debug.elf);
	  mod->debug.elf = NULL;
	  close (fd);
	  fd = -1;
	}
      free (*debuginfo_file_name);
      *debuginfo_file_name = NULL;
      errno = 0;
    }
  return fd;
}
INTDEF (dwfl_build_id_find_debuginfo)
