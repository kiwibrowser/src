/* Internal definitions for interface for libebl.
   Copyright (C) 2000-2009, 2013 Red Hat, Inc.
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

#ifndef _LIBEBLP_H
#define _LIBEBLP_H 1

#include <gelf.h>
#include <libasm.h>
#include <libebl.h>
#include <libintl.h>


/* Backend handle.  */
struct ebl
{
  /* Machine name.  */
  const char *name;

  /* Emulation name.  */
  const char *emulation;

  /* ELF machine, class, and data encoding.  */
  uint_fast16_t machine;
  uint_fast8_t class;
  uint_fast8_t data;

  /* The libelf handle (if known).  */
  Elf *elf;

  /* See ebl-hooks.h for the declarations of the hook functions.  */
# define EBLHOOK(name) (*name)
# include "ebl-hooks.h"
# undef EBLHOOK

  /* Size of entry in Sysv-style hash table.  */
  int sysvhash_entrysize;

  /* Number of registers to allocate for ebl_set_initial_registers_tid.
     Ebl architecture can unwind iff FRAME_NREGS > 0.  */
  size_t frame_nregs;

  /* Function descriptor load address and table as used by
     ebl_resolve_sym_value if available for this arch.  */
  GElf_Addr fd_addr;
  Elf_Data *fd_data;

  /* Internal data.  */
  void *dlhandle;
};


/* Type of the initialization functions in the backend modules.  */
typedef const char *(*ebl_bhinit_t) (Elf *, GElf_Half, Ebl *, size_t);


/* gettext helper macros.  */
#undef _
#define _(Str) dgettext ("elfutils", Str)


/* LEB128 constant helper macros.  */
#define ULEB128_7(x)	(BUILD_BUG_ON_ZERO ((x) >= (1U << 7)) + (x))

#define BUILD_BUG_ON_ZERO(x) (sizeof (char [(x) ? -1 : 1]) - 1)

#endif	/* libeblP.h */
