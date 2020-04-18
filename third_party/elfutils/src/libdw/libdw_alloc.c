/* Memory handling for libdw.
   Copyright (C) 2003, 2004, 2006 Red Hat, Inc.
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

#include <error.h>
#include <errno.h>
#include <stdlib.h>
#include <sys/param.h>
#include "libdwP.h"


void *
__libdw_allocate (Dwarf *dbg, size_t minsize, size_t align)
{
  size_t size = MAX (dbg->mem_default_size,
		     (align - 1 +
		      2 * minsize + offsetof (struct libdw_memblock, mem)));
  struct libdw_memblock *newp = malloc (size);
  if (newp == NULL)
    dbg->oom_handler ();

  uintptr_t result = ((uintptr_t) newp->mem + align - 1) & ~(align - 1);

  newp->size = size - offsetof (struct libdw_memblock, mem);
  newp->remaining = (uintptr_t) newp + size - (result + minsize);

  newp->prev = dbg->mem_tail;
  dbg->mem_tail = newp;

  return (void *) result;
}


Dwarf_OOM
dwarf_new_oom_handler (Dwarf *dbg, Dwarf_OOM handler)
{
  Dwarf_OOM old = dbg->oom_handler;
  dbg->oom_handler = handler;
  return old;
}


void
__attribute ((noreturn, visibility ("hidden")))
__libdw_oom (void)
{
  while (1)
    error (EXIT_FAILURE, ENOMEM, "libdw");
}
