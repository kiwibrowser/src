/*
 * Copyright (c) 2014 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <string.h>

#include "native_client/src/include/nacl_macros.h"
#include "native_client/src/untrusted/irt/irt_dev.h"
#include "native_client/src/untrusted/irt/irt_extension.h"
#include "native_client/src/untrusted/nacl/nacl_irt.h"

/*
 * The declarations here are used as references for nacl_interface_ext_supply()
 * to store the interface table pointers into. They should all be declared as
 * __attribute__((weak)) so that the references to the pointers will be NULL
 * if the user is has not linked in the definition. This makes it so we don't
 * link in extra definitions that are not utilized.
 */

/* Declarations are listed in the same order as in irt.h. */
extern struct nacl_irt_basic __libnacl_irt_basic __attribute__((weak));
extern struct nacl_irt_fdio __libnacl_irt_fdio __attribute__((weak));
extern struct nacl_irt_memory __libnacl_irt_memory __attribute__((weak));
extern struct nacl_irt_thread __libnacl_irt_thread __attribute__((weak));
extern struct nacl_irt_futex __libnacl_irt_futex __attribute__((weak));
extern struct nacl_irt_clock __libnacl_irt_clock __attribute__((weak));

/* Declarations are listed in the same order as in irt_dev.h. */
extern struct nacl_irt_dev_fdio __libnacl_irt_dev_fdio __attribute__((weak));
extern struct nacl_irt_dev_filename __libnacl_irt_dev_filename
  __attribute__((weak));
extern struct nacl_irt_tls __libnacl_irt_tls __attribute__((weak));
extern struct nacl_irt_dev_getpid __libnacl_irt_dev_getpid
  __attribute__((weak));

/*
 * The following table provides us a simple way to keep track of all the
 * interfaces we currently support along with their reference and sizes.
 */
struct nacl_irt_ext_struct {
  const char *interface_ident;
  void *table;
  size_t tablesize;
};

static const struct nacl_irt_ext_struct nacl_irt_ext_structs[] = {
  /* Interfaces are listed in the same order as in irt.h. */
  {
    .interface_ident = NACL_IRT_BASIC_v0_1,
    .table = &__libnacl_irt_basic,
    .tablesize = sizeof(__libnacl_irt_basic)
  }, {
    .interface_ident = NACL_IRT_FDIO_v0_1,
    .table = &__libnacl_irt_fdio,
    .tablesize = sizeof(__libnacl_irt_fdio),
  }, {
    .interface_ident = NACL_IRT_MEMORY_v0_3,
    .table = &__libnacl_irt_memory,
    .tablesize = sizeof(__libnacl_irt_memory),
  }, {
    .interface_ident = NACL_IRT_THREAD_v0_1,
    .table = &__libnacl_irt_thread,
    .tablesize = sizeof(__libnacl_irt_thread),
  }, {
    .interface_ident = NACL_IRT_FUTEX_v0_1,
    .table = &__libnacl_irt_futex,
    .tablesize = sizeof(__libnacl_irt_futex),
  }, {
    .interface_ident = NACL_IRT_CLOCK_v0_1,
    .table = &__libnacl_irt_clock,
    .tablesize = sizeof(__libnacl_irt_clock)
  },

  /* Interfaces are listed in the same order as in irt_dev.h. */
  {
    .interface_ident = NACL_IRT_DEV_FDIO_v0_3,
    .table = &__libnacl_irt_dev_fdio,
    .tablesize = sizeof(__libnacl_irt_dev_fdio),
  }, {
    .interface_ident = NACL_IRT_DEV_FILENAME_v0_3,
    .table = &__libnacl_irt_dev_filename,
    .tablesize = sizeof(__libnacl_irt_dev_filename),
  }, {
    .interface_ident = NACL_IRT_DEV_GETPID_v0_1,
    .table = &__libnacl_irt_dev_getpid,
    .tablesize = sizeof(__libnacl_irt_dev_getpid)
  },
};

size_t nacl_interface_ext_supply(const char *interface_ident,
                               const void *table, size_t tablesize) {
  for (size_t i = 0; i < NACL_ARRAY_SIZE(nacl_irt_ext_structs); i++) {
    if (nacl_irt_ext_structs[i].tablesize == tablesize &&
        strcmp(nacl_irt_ext_structs[i].interface_ident, interface_ident) == 0) {
      /*
       * Since the table is pointing to weak references, it can be NULL which
       * signifies that the variable is not linked. In that case we should
       * return 0 signifying that the interface was not supplied.
       */
      if (nacl_irt_ext_structs[i].table == NULL)
        return 0;

      memcpy(nacl_irt_ext_structs[i].table, table, tablesize);
      return tablesize;
    }
  }

  return 0;
}
