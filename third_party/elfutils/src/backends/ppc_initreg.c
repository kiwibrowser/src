/* Fetch live process registers from TID.
   Copyright (C) 2013 Red Hat, Inc.
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

#include "system.h"
#include <stdlib.h>
#ifdef __powerpc__
# include <sys/user.h>
# include <sys/ptrace.h>
#endif

#define BACKEND ppc_
#include "libebl_CPU.h"

bool
ppc_dwarf_to_regno (Ebl *ebl __attribute__ ((unused)), unsigned *regno)
{
  switch (*regno)
  {
    case 108:
      // LR uses both 65 and 108 numbers, there is no consistency for it.
      *regno = 65;
      return true;
    case 0 ... 107:
    case 109 ... (114 - 1) -1:
      return true;
    case 1200 ... 1231:
      *regno = *regno - 1200 + (114 - 1);
      return true;
    default:
      return false;
  }
  abort ();
}

__typeof (ppc_dwarf_to_regno)
     ppc64_dwarf_to_regno
     __attribute__ ((alias ("ppc_dwarf_to_regno")));

bool
ppc_set_initial_registers_tid (pid_t tid __attribute__ ((unused)),
			  ebl_tid_registers_t *setfunc __attribute__ ((unused)),
			       void *arg __attribute__ ((unused)))
{
#ifndef __powerpc__
  return false;
#else /* __powerpc__ */
  union
    {
      struct pt_regs r;
      long l[sizeof (struct pt_regs) / sizeof (long)];
    }
  user_regs;
  eu_static_assert (sizeof (struct pt_regs) % sizeof (long) == 0);
  /* PTRACE_GETREGS is EIO on kernel-2.6.18-308.el5.ppc64.  */
  errno = 0;
  for (unsigned regno = 0; regno < sizeof (user_regs) / sizeof (long);
       regno++)
    {
      user_regs.l[regno] = ptrace (PTRACE_PEEKUSER, tid,
				   (void *) (uintptr_t) (regno
							 * sizeof (long)),
				   NULL);
      if (errno != 0)
	return false;
    }
  const size_t gprs = sizeof (user_regs.r.gpr) / sizeof (*user_regs.r.gpr);
  Dwarf_Word dwarf_regs[gprs];
  for (unsigned gpr = 0; gpr < gprs; gpr++)
    dwarf_regs[gpr] = user_regs.r.gpr[gpr];
  if (! setfunc (0, gprs, dwarf_regs, arg))
    return false;
  dwarf_regs[0] = user_regs.r.link;
  // LR uses both 65 and 108 numbers, there is no consistency for it.
  if (! setfunc (65, 1, dwarf_regs, arg))
    return false;
  /* Registers like msr, ctr, xer, dar, dsisr etc. are probably irrelevant
     for CFI.  */
  dwarf_regs[0] = user_regs.r.nip;
  return setfunc (-1, 1, dwarf_regs, arg);
#endif /* __powerpc__ */
}

__typeof (ppc_set_initial_registers_tid)
     ppc64_set_initial_registers_tid
     __attribute__ ((alias ("ppc_set_initial_registers_tid")));
