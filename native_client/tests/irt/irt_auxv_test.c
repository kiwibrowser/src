/*
 * Copyright (c) 2014 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <stdbool.h>
#include <stdio.h>
#include <unistd.h>

#include "native_client/src/include/elf32.h"
#include "native_client/src/include/elf_auxv.h"
#include "native_client/src/untrusted/nacl/nacl_irt.h"
#include "native_client/src/untrusted/nacl/nacl_startup.h"
#include "native_client/src/untrusted/nacl/tls.h"

void __libc_init_array(void);
void __libc_fini_array(void);

void _start(uint32_t *info) {
  Elf32_auxv_t *auxv = nacl_startup_auxv(info);

  __libnacl_irt_init(auxv);
  atexit(&__libc_fini_array);
  __pthread_initialize();
  __libc_init_array();

  int bad = 0;
  bool saw_sysinfo = false;
  uintptr_t sysinfo = 0;
  for (Elf32_auxv_t *av = auxv; av->a_type != AT_NULL; ++av) {
    switch (av->a_type) {
      case AT_IGNORE:
        printf("AT_IGNORE seen in slot %d\n", av - auxv);
        bad = 1;
        break;
      case AT_SYSINFO:
        if (saw_sysinfo) {
          printf("duplicate AT_SYSINFO seen in slot %d\n", av - auxv);
          bad = 1;
        } else {
          sysinfo = av->a_un.a_val;
          saw_sysinfo = true;
        }
        break;
      default:
        printf("unexpected auxv element {%u, %u}\n",
               av->a_type, av->a_un.a_val);
        bad = 1;
        break;
    }
  }

  if (!saw_sysinfo) {
    puts("AT_SYSINFO missing!");
    bad = 1;
  } else if (sysinfo != (uintptr_t) __nacl_irt_query) {
    printf("AT_SYSINFO has value %#x, expected %#x\n",
           sysinfo, (uintptr_t) __nacl_irt_query);
    bad = 1;
  }

  exit(bad);

  /*NOTREACHED*/
  __builtin_trap();
}

/*
 * This is never actually called, but there is an artificial undefined
 * reference to it.
 */
int main(void) {
}
