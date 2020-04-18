/*
 * Copyright (c) 2015 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <stdint.h>

#include "native_client/src/include/elf_auxv.h"
#include "native_client/src/untrusted/irt/irt.h"
#include "native_client/src/untrusted/nacl/nacl_startup.h"

/*
 * This file is built with no C library, in PIC mode, and turned into an
 * ET_DYN object.  The code here must be purely PIC and make no references
 * that would require a dynamic reloc.  Marking _start as "hidden" allows
 * the compiler to use a pure PIC calculation for its runtime address.
 *
 * This code acts in place of a dynamic linker.  Its entry point gets
 * called with auxv bits pointing to the main program (if there is one).
 */

__attribute__((visibility("hidden"))) void _start(uint32_t info[]) {
  const Elf32_auxv_t *auxv = nacl_startup_auxv(info);

  TYPE_nacl_irt_query irt_query = 0;
  uintptr_t entry = 0;
  for (const Elf32_auxv_t *av = auxv; av->a_type != AT_NULL; ++av) {
    switch (av->a_type) {
      case AT_SYSINFO:
        irt_query = (TYPE_nacl_irt_query) av->a_un.a_val;
        break;
      case AT_ENTRY:
        entry = av->a_un.a_val;
        break;
    }
  }

  if (irt_query == 0 || entry == 0)
    __builtin_trap();

  if (entry == (uintptr_t) &_start) {
    /*
     * The loader gave us our own entrypoint in AT_ENTRY.
     * This means we're the main program, not the PT_INTERP.
     *
     * Just exit with a special status value to identify this case.
     * It tests elf_loader with an ET_DYN main program, as would be
     * the case for running the dynamic linker standalone.
     */

    struct nacl_irt_basic basic;
    if ((*irt_query)(NACL_IRT_BASIC_v0_1, &basic,
                     sizeof(basic)) != sizeof(basic)) {
      __builtin_trap();
    }

    (*basic.exit)(TEST_EXIT);
    /*NOTREACHED*/
    while (1)
      __builtin_trap();
  }

  /*
   * Tail-call into the main program's entry point.
   */
  ((void (*)(uint32_t info[])) entry)(info);
}
