/* Linux/i386 system call ABI in DWARF register numbers.
   Copyright (C) 2008 Red Hat, Inc.
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

#define BACKEND i386_
#include "libebl_CPU.h"

int
i386_syscall_abi (Ebl *ebl __attribute__ ((unused)),
		  int *sp, int *pc, int *callno, int args[6])
{
  *sp = 4;			/* %esp */
  *pc = 8;			/* %eip */
  *callno = 0;			/* %eax */
  args[0] = 3;			/* %ebx */
  args[1] = 1;			/* %ecx */
  args[2] = 2;			/* %edx */
  args[3] = 6;			/* %esi */
  args[4] = 7;			/* %edi */
  args[5] = 5;			/* %ebp */
  return 0;
}
