/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <stdlib.h>
#include <sys/mman.h>
#include <unistd.h>

#include "native_client/src/untrusted/nacl/nacl_irt.h"
#include "native_client/src/untrusted/nacl/nacl_thread.h"
#include "native_client/src/untrusted/nacl/tls.h"

/*
 * Use a weak reference to malloc() so that we only use it if it is
 * going to be linked in anyway.  This reduces executable sizes for
 * very simple programs that don't use malloc().
 */
void *malloc(size_t size) __attribute__((weak));

/*
 * This initialization happens early in startup with or without libpthread.
 * It must make it safe for vanilla newlib code to run.
 */
void __pthread_initialize_minimal(size_t tdb_size) {
  size_t combined_size = __nacl_tls_combined_size(tdb_size);

  void *combined_area;
  if (malloc != NULL) {
    combined_area = malloc(combined_size);
  } else {
    /*
     * Fall back to mmap() when malloc() is not linked in.  This will
     * likely waste most of a 64k page, but that probably doesn't
     * matter.  If the program doesn't use malloc() it probably won't
     * need to allocate much else.
     *
     * Note that if this allocation fails, it might crash setting
     * errno, because TLS is not set up yet.
     */
    combined_area = mmap(NULL, combined_size, PROT_READ | PROT_WRITE,
                         MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  }

  /*
   * Fill in that memory with its initializer data.
   */
  void *tp = __nacl_tls_initialize_memory(combined_area, tdb_size);

  /*
   * Set %gs, r9, or equivalent platform-specific mechanism.  Requires
   * a syscall since certain bitfields of these registers are trusted.
   */
  nacl_tls_init(tp);

  /*
   * Initialize newlib's thread-specific pointer.
   */
  __newlib_thread_init();
}
