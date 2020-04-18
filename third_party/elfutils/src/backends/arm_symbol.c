/* Arm specific symbolic name handling.
   Copyright (C) 2002-2009 Red Hat, Inc.
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

#include <elf.h>
#include <stddef.h>

#define BACKEND		arm_
#include "libebl_CPU.h"


const char *
arm_segment_type_name (int segment, char *buf __attribute__ ((unused)),
		       size_t len __attribute__ ((unused)))
{
  switch (segment)
    {
    case PT_ARM_EXIDX:
      return "ARM_EXIDX";
    }
  return NULL;
}

/* Return symbolic representation of section type.  */
const char *
arm_section_type_name (int type,
		       char *buf __attribute__ ((unused)),
		       size_t len __attribute__ ((unused)))
{
  switch (type)
    {
    case SHT_ARM_EXIDX:
      return "ARM_EXIDX";
    case SHT_ARM_PREEMPTMAP:
      return "ARM_PREEMPTMAP";
    case SHT_ARM_ATTRIBUTES:
      return "ARM_ATTRIBUTES";
    }

  return NULL;
}

/* Check whether machine flags are valid.  */
bool
arm_machine_flag_check (GElf_Word flags)
{
  switch (flags & EF_ARM_EABIMASK)
    {
    case EF_ARM_EABI_UNKNOWN:
    case EF_ARM_EABI_VER1:
    case EF_ARM_EABI_VER2:
    case EF_ARM_EABI_VER3:
    case EF_ARM_EABI_VER4:
    case EF_ARM_EABI_VER5:
      break;
    default:
      return false;
    }

  return ((flags &~ (EF_ARM_EABIMASK
		     | EF_ARM_RELEXEC
		     | EF_ARM_HASENTRY
		     | EF_ARM_INTERWORK
		     | EF_ARM_APCS_26
		     | EF_ARM_APCS_FLOAT
		     | EF_ARM_PIC
		     | EF_ARM_ALIGN8
		     | EF_ARM_NEW_ABI
		     | EF_ARM_OLD_ABI
		     | EF_ARM_SOFT_FLOAT
		     | EF_ARM_VFP_FLOAT
		     | EF_ARM_MAVERICK_FLOAT
		     | EF_ARM_SYMSARESORTED
		     | EF_ARM_DYNSYMSUSESEGIDX
		     | EF_ARM_MAPSYMSFIRST
		     | EF_ARM_EABIMASK
		     | EF_ARM_BE8
		     | EF_ARM_LE8)) == 0);
}

/* Check for the simple reloc types.  */
Elf_Type
arm_reloc_simple_type (Ebl *ebl __attribute__ ((unused)), int type)
{
  switch (type)
    {
    case R_ARM_ABS32:
      return ELF_T_WORD;
    case R_ARM_ABS16:
      return ELF_T_HALF;
    case R_ARM_ABS8:
      return ELF_T_BYTE;
    default:
      return ELF_T_NUM;
    }
}
