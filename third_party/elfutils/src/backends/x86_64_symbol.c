/* x86_64 specific symbolic name handling.
   Copyright (C) 2002, 2005 Red Hat, Inc.
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

#include <assert.h>
#include <elf.h>
#include <stddef.h>

#define BACKEND		x86_64_
#include "libebl_CPU.h"

/* Check for the simple reloc types.  */
Elf_Type
x86_64_reloc_simple_type (Ebl *ebl __attribute__ ((unused)), int type)
{
  switch (type)
    {
    case R_X86_64_64:
      return ELF_T_XWORD;
    case R_X86_64_32:
      return ELF_T_WORD;
    case R_X86_64_32S:
      return ELF_T_SWORD;
    case R_X86_64_16:
      return ELF_T_HALF;
    case R_X86_64_8:
      return ELF_T_BYTE;
    default:
      return ELF_T_NUM;
    }
}
