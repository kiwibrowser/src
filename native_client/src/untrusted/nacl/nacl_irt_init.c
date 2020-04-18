/*
 * Copyright (c) 2013 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "native_client/src/include/elf_auxv.h"
#include "native_client/src/include/elf32.h"
#include "native_client/src/untrusted/nacl/nacl_irt.h"

static int __libnacl_irt_mprotect(void *addr, size_t len, int prot) {
  return ENOSYS;
}

/*
 * Scan the auxv for AT_SYSINFO, which is the pointer to the IRT query function.
 * Stash that for later use.
 */
static void grok_auxv(const Elf32_auxv_t *auxv) {
  const Elf32_auxv_t *av;
  for (av = auxv; av->a_type != AT_NULL; ++av) {
    if (av->a_type == AT_SYSINFO) {
      __nacl_irt_query = (TYPE_nacl_irt_query) av->a_un.a_val;
    }
  }
}

#define DO_QUERY(ident, name)                                   \
  __libnacl_mandatory_irt_query(ident, &__libnacl_irt_##name,   \
                                sizeof(__libnacl_irt_##name))

/*
 * Initialize all our IRT function tables using the query function.
 * The query function's address is passed via AT_SYSINFO in auxv.
 */
void __libnacl_irt_init(Elf32_auxv_t *auxv) {
  grok_auxv(auxv);

  /*
   * The "fdio" interface doesn't do anything useful in Chromium (with the
   * exception that write() sometimes produces useful debugging output for
   * stdout/stderr), so don't abort if the it is not available.
   *
   * We query for "fdio" early on so that write() can produce a useful
   * debugging message if any other IRT queries fail.
   */
  if (!__libnacl_irt_query(NACL_IRT_FDIO_v0_1, &__libnacl_irt_fdio,
                           sizeof(__libnacl_irt_fdio))) {
    __libnacl_irt_query(NACL_IRT_DEV_FDIO_v0_1, &__libnacl_irt_fdio,
                        sizeof(__libnacl_irt_fdio));
  }

  DO_QUERY(NACL_IRT_BASIC_v0_1, basic);

  if (!__libnacl_irt_query(NACL_IRT_MEMORY_v0_3,
                           &__libnacl_irt_memory,
                           sizeof(__libnacl_irt_memory))) {
    /* Fall back to trying the old version, before sysbrk() was removed. */
    struct nacl_irt_memory_v0_2 old_irt_memory;
    if (!__libnacl_irt_query(NACL_IRT_MEMORY_v0_2,
                             &old_irt_memory,
                             sizeof(old_irt_memory))) {
      /* Fall back to trying an older version, before mprotect() was added. */
      __libnacl_mandatory_irt_query(NACL_IRT_MEMORY_v0_1,
                                    &old_irt_memory,
                                    sizeof(struct nacl_irt_memory_v0_1));
      __libnacl_irt_memory.mprotect = __libnacl_irt_mprotect;
    }
    __libnacl_irt_memory.mmap = old_irt_memory.mmap;
    __libnacl_irt_memory.munmap = old_irt_memory.munmap;
  }

  DO_QUERY(NACL_IRT_TLS_v0_1, tls);
}
