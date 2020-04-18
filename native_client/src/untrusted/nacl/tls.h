/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <stddef.h>

#ifndef NATIVE_CLIENT_SRC_UNTRUSTED_NACL_TLS_H
#define NATIVE_CLIENT_SRC_UNTRUSTED_NACL_TLS_H

void __pthread_initialize(void);

/*
 * Allocates (using sbrk) and initializes the combined area for the
 * main thread.  Always called, whether or not pthreads is in use.
 */
void __pthread_initialize_minimal(size_t tdb_size);

/*
 * Initializes the thread-local data of newlib (which achieves
 * reentrancy by making heavy use of TLS).
 */
void __newlib_thread_init(void);

void __newlib_thread_exit(void);


/*
 * Returns the allocation size of the combined area, including TLS
 * (data + bss), TDB, and padding required to guarantee alignment.
 * Since the TDB layout is platform-specific, its size is passed as a
 * parameter.
 */
size_t __nacl_tls_combined_size(size_t tdb_size);

/*
 * Initializes the TLS of a combined area by copying the TLS data area
 * from the template (ELF image .tdata) and zeroing the TLS BSS area.
 * Returns the pointer that should go in the thread register.
 */
void *__nacl_tls_initialize_memory(void *combined_area, size_t tdb_size);

/*
 * Read the per-thread pointer.  Note that callers should call
 * __nacl_read_tp_inline() instead.
 */
void *__nacl_read_tp(void);

#if defined(__pnacl__)
/*
 * Reading the thread pointer using PNaCl's LLVM intrinisic can be
 * faster because the read can be inlined.
 *
 * Note that we can't add the __asm__ attribute below to the
 * declaration of __nacl_read_tp() because that would interfere with
 * definitions of __nacl_read_tp().
 */
void *__nacl_read_tp_inline(void) __asm__("llvm.nacl.read.tp");
#else
static inline void *__nacl_read_tp_inline(void) {
  return __nacl_read_tp();
}
#endif

/* Read the per-thread pointer and add an offset.  */
void *__nacl_add_tp(ptrdiff_t);


#endif /* NATIVE_CLIENT_SRC_UNTRUSTED_NACL_TLS_H */
