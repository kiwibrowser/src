/* Test program for dwarf_lowpc and dwarf_highpc
   Copyright (C) 2012 Red Hat, Inc.
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
#include <assert.h>
#include <inttypes.h>
#include ELFUTILS_HEADER(dwfl)
#include <dwarf.h>
#include <argp.h>
#include <stdio.h>
#include <stdio_ext.h>
#include <locale.h>
#include <stdlib.h>
#include <error.h>
#include <string.h>
#include <fnmatch.h>

struct args
{
  Dwfl *dwfl;
  Dwarf_Die *cu;
  Dwarf_Addr dwbias;
  char **argv;
  const char *file;
};

static struct args *args;

static void
fail(Dwarf_Off off, const char *name, const char *msg)
{
  printf("%s: [%" PRIx64 "] '%s' %s\n", args->file, off, name, msg);
  exit(-1);
}

static int
handle_die (Dwarf_Die *die, void *arg)
{
  args = arg;
  Dwarf_Off off = dwarf_dieoffset (die);

  const char *name = dwarf_diename (die);
  if (name == NULL)
    fail (off, "<no name>", "die without a name");

  Dwarf_Addr lowpc = 0;
  Dwarf_Addr highpc = 0;
  if (dwarf_lowpc (die, &lowpc) != 0 && dwarf_hasattr (die, DW_AT_low_pc))
    fail (off, name, "has DW_AT_low_pc but dwarf_lowpc fails");
  if (dwarf_highpc (die, &highpc) != 0 && dwarf_hasattr (die, DW_AT_high_pc))
    fail (off, name, "has DW_AT_high_pc but dwarf_highpc fails");

  /* GCC < 4.7 had a bug where no code CUs got a highpc == lowpc.
     Allow that, because it is not the main purpose of this test.  */
  if (dwarf_hasattr (die, DW_AT_low_pc)
      && dwarf_hasattr (die, DW_AT_high_pc)
      && highpc <= lowpc
      && ! (dwarf_tag (die) == DW_TAG_compile_unit && highpc == lowpc))
    {
      printf("lowpc: %" PRIx64 ", highpc: %" PRIx64 "lx\n", lowpc, highpc);
      fail (off, name, "highpc <= lowpc");
    }

  return 0;
}


int
main (int argc, char *argv[])
{
  int remaining;

  /* Set locale.  */
  (void) setlocale (LC_ALL, "");

  struct args a = { .dwfl = NULL, .cu = NULL };

  (void) argp_parse (dwfl_standard_argp (), argc, argv, 0, &remaining,
		     &a.dwfl);
  assert (a.dwfl != NULL);
  a.argv = &argv[remaining];

  int result = 0;

  while ((a.cu = dwfl_nextcu (a.dwfl, a.cu, &a.dwbias)) != NULL)
    {
      a.file = dwarf_diename (a.cu);
      handle_die (a.cu, &a);
      dwarf_getfuncs (a.cu, &handle_die, &a, 0);
    }

  dwfl_end (a.dwfl);

  return result;
}
