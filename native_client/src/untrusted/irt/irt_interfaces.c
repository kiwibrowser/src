/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <string.h>

#include "native_client/src/include/nacl_compiler_annotations.h"
#include "native_client/src/include/nacl_macros.h"
#include "native_client/src/public/irt_core.h"
#include "native_client/src/trusted/service_runtime/include/sys/unistd.h"
#include "native_client/src/untrusted/irt/irt.h"
#include "native_client/src/untrusted/irt/irt_dev.h"
#include "native_client/src/untrusted/irt/irt_interfaces.h"
#include "native_client/src/untrusted/nacl/syscall_bindings_trampoline.h"

static int file_access_filter(void) {
  static int nacl_file_access_enabled = -1;
  if (NACL_UNLIKELY(-1 == nacl_file_access_enabled)) {
    NACL_SYSCALL(sysconf)(NACL_ABI__SC_NACL_FILE_ACCESS_ENABLED,
                          &nacl_file_access_enabled);
  }
  return nacl_file_access_enabled;
}

static int list_mappings_filter(void) {
  static int nacl_list_mappings_enabled = -1;
  if (NACL_UNLIKELY(-1 == nacl_list_mappings_enabled)) {
    NACL_SYSCALL(sysconf)(NACL_ABI__SC_NACL_LIST_MAPPINGS_ENABLED,
                          &nacl_list_mappings_enabled);
  }
  return nacl_list_mappings_enabled;
}

static int non_pnacl_filter(void) {
  static int pnacl_mode = -1;
  if (NACL_UNLIKELY(-1 == pnacl_mode)) {
    NACL_SYSCALL(sysconf)(NACL_ABI__SC_NACL_PNACL_MODE, &pnacl_mode);
  }
  return !pnacl_mode;
}

static const struct nacl_irt_interface irt_interfaces[] = {
  { NACL_IRT_BASIC_v0_1, &nacl_irt_basic, sizeof(nacl_irt_basic), NULL },
  /*
   * "irt-fdio" is disabled under PNaCl because file descriptors in
   * general are not exposed in PNaCl's in-browser ABI (since
   * open_resource() is also disabled under PNaCl).  "irt-fdio" is
   * only exposed under PNaCl via the "dev" query string since writing
   * to stdout/stderr is useful for debugging.
   */
  { NACL_IRT_FDIO_v0_1, &nacl_irt_fdio, sizeof(nacl_irt_fdio),
    non_pnacl_filter },
  { NACL_IRT_DEV_FDIO_v0_1, &nacl_irt_fdio, sizeof(nacl_irt_fdio), NULL },
  { NACL_IRT_DEV_FDIO_v0_2, &nacl_irt_dev_fdio_v0_2,
    sizeof(nacl_irt_dev_fdio_v0_2), file_access_filter },
  { NACL_IRT_DEV_FDIO_v0_3, &nacl_irt_dev_fdio,
    sizeof(nacl_irt_dev_fdio), file_access_filter },
  /*
   * "irt-filename" is made available to non-PNaCl NaCl apps only for
   * compatibility, because existing nexes abort on startup if
   * "irt-filename" is not available.
   */
  { NACL_IRT_FILENAME_v0_1, &nacl_irt_filename, sizeof(nacl_irt_filename),
    non_pnacl_filter },
  { NACL_IRT_DEV_FILENAME_v0_2, &nacl_irt_dev_filename_v0_2,
    sizeof(nacl_irt_dev_filename_v0_2), file_access_filter },
  { NACL_IRT_DEV_FILENAME_v0_3, &nacl_irt_dev_filename,
    sizeof(nacl_irt_dev_filename), file_access_filter },
  /*
   * The old versions of "irt-memory", v0.1 and v0.2, which contain
   * the deprecated sysbrk() function, are disabled under PNaCl.  See:
   * https://code.google.com/p/nativeclient/issues/detail?id=3542
   */
  { NACL_IRT_MEMORY_v0_1, &nacl_irt_memory_v0_1, sizeof(nacl_irt_memory_v0_1),
    non_pnacl_filter },
  { NACL_IRT_MEMORY_v0_2, &nacl_irt_memory_v0_2, sizeof(nacl_irt_memory_v0_2),
    non_pnacl_filter },
  { NACL_IRT_MEMORY_v0_3, &nacl_irt_memory, sizeof(nacl_irt_memory), NULL },
  /*
   * "irt-dyncode" is not supported under PNaCl because dynamically
   * loading architecture-specific native code is not portable.
   */
  { NACL_IRT_DYNCODE_v0_1, &nacl_irt_dyncode, sizeof(nacl_irt_dyncode),
    non_pnacl_filter },
  { NACL_IRT_THREAD_v0_1, &nacl_irt_thread, sizeof(nacl_irt_thread), NULL },
  { NACL_IRT_FUTEX_v0_1, &nacl_irt_futex, sizeof(nacl_irt_futex), NULL },
  /*
   * "irt-mutex", "irt-cond" and "irt-sem" are deprecated and
   * superseded by the "irt-futex" interface, and so are disabled
   * under PNaCl.  See:
   * https://code.google.com/p/nativeclient/issues/detail?id=3484
   */
  { NACL_IRT_MUTEX_v0_1, &nacl_irt_mutex, sizeof(nacl_irt_mutex),
    non_pnacl_filter },
  { NACL_IRT_COND_v0_1, &nacl_irt_cond, sizeof(nacl_irt_cond),
    non_pnacl_filter },
  { NACL_IRT_SEM_v0_1, &nacl_irt_sem, sizeof(nacl_irt_sem),
    non_pnacl_filter },
  { NACL_IRT_TLS_v0_1, &nacl_irt_tls, sizeof(nacl_irt_tls), NULL },
  /*
   * "irt-blockhook" is deprecated.  It was provided for implementing
   * thread suspension for conservative garbage collection, but this
   * is probably not a portable use case under PNaCl, so this
   * interface is disabled under PNaCl.  See:
   * https://code.google.com/p/nativeclient/issues/detail?id=3539
   */
  { NACL_IRT_BLOCKHOOK_v0_1, &nacl_irt_blockhook, sizeof(nacl_irt_blockhook),
    non_pnacl_filter },
  { NACL_IRT_RANDOM_v0_1, &nacl_irt_random, sizeof(nacl_irt_random), NULL },
  { NACL_IRT_CLOCK_v0_1, &nacl_irt_clock, sizeof(nacl_irt_clock), NULL },
  { NACL_IRT_DEV_GETPID_v0_1, &nacl_irt_dev_getpid,
    sizeof(nacl_irt_dev_getpid), file_access_filter },
  /*
   * "irt-exception-handling" is not supported under PNaCl because it
   * exposes non-portable, architecture-specific register state.  See:
   * https://code.google.com/p/nativeclient/issues/detail?id=3444
   */
  { NACL_IRT_EXCEPTION_HANDLING_v0_1, &nacl_irt_exception_handling,
    sizeof(nacl_irt_exception_handling), non_pnacl_filter },
  { NACL_IRT_DEV_LIST_MAPPINGS_v0_1, &nacl_irt_dev_list_mappings,
    sizeof(nacl_irt_dev_list_mappings), list_mappings_filter },
  /*
   * "irt-code-data-alloc" is not supported under PNaCl.
   */
  { NACL_IRT_CODE_DATA_ALLOC_v0_1, &nacl_irt_code_data_alloc,
    sizeof(nacl_irt_code_data_alloc), non_pnacl_filter },
};

size_t nacl_irt_query_core(const char *interface_ident,
                           void *table, size_t tablesize) {
  return nacl_irt_query_list(interface_ident, table, tablesize,
                             irt_interfaces, sizeof(irt_interfaces));
}
