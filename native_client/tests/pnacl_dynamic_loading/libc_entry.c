/*
 * Copyright (c) 2016 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <stdlib.h>
#include <unistd.h>

#include "native_client/src/include/elf32.h"
#include "native_client/src/untrusted/nacl/nacl_irt.h"
#include "native_client/src/untrusted/nacl/start.h"
#include "native_client/src/untrusted/nacl/tls.h"


/*
 * Provide dummy definitions of _init() and _fini() because they are
 * referenced by libc.a's __libc_init_array(), which we do not use in this
 * context.
 */

void _init(void) {
  abort();
}

void _fini(void) {
  abort();
}

/*
 * Provide dummy definitions of the TLS layout functions because they are
 * referenced by libnacl.a's tls.c, which we do not use in this context.
 */

ptrdiff_t __nacl_tp_tls_offset(size_t tls_size) {
  abort();
}

ptrdiff_t __nacl_tp_tdb_offset(size_t tdb_size) {
  abort();
}

void __libc_start(int argc, char **argv, char **envp, Elf32_auxv_t *auxv) {
  environ = envp;

  __libnacl_irt_init(auxv);

  /*
   * We don't support TLS or pthreads yet, so we skip
   * __pthread_initialize() for now.
   */
  __newlib_thread_init();

  exit(main(argc, argv, envp));

  __builtin_trap();
}
