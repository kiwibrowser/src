/*
 * Copyright (c) 2013 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <string.h>

#include "native_client/src/include/nacl_macros.h"
#include "native_client/src/untrusted/irt/irt.h"
#include "native_client/src/untrusted/irt/irt_interfaces.h"

struct nacl_interface_table {
  const char *name;
  const void *table;
  size_t size;
};

static const struct nacl_interface_table irt_interfaces[] = {
  /*
   * We expect current NaCl programs to be able to work with this
   * minimal set of IRT interface versions.  Some of these are old
   * interface versions.
   */
  { NACL_IRT_BASIC_v0_1, &nacl_irt_basic, sizeof(nacl_irt_basic) },
  { NACL_IRT_MEMORY_v0_1, &nacl_irt_memory_v0_1, sizeof(nacl_irt_memory_v0_1) },
  { NACL_IRT_TLS_v0_1, &nacl_irt_tls, sizeof(nacl_irt_tls) },
#if ALLOW_DYNAMIC_LINKING
  { NACL_IRT_FILENAME_v0_1, &nacl_irt_filename, sizeof(nacl_irt_filename) },
  { NACL_IRT_DYNCODE_v0_1, &nacl_irt_dyncode, sizeof(nacl_irt_dyncode) },
#endif
  /*
   * Nexes should not necessarily require "fdio" at startup, but its
   * presence is necessary for hello_world to produce output and so
   * for the hello_world test to pass.
   */
  { NACL_IRT_FDIO_v0_1, &nacl_irt_fdio, sizeof(nacl_irt_fdio) },
};

size_t nacl_irt_query_core(const char *interface_ident,
                           void *table, size_t tablesize) {
  int i;
  for (i = 0; i < NACL_ARRAY_SIZE(irt_interfaces); ++i) {
    if (0 == strcmp(interface_ident, irt_interfaces[i].name)) {
      const size_t size = irt_interfaces[i].size;
      if (size <= tablesize) {
        memcpy(table, irt_interfaces[i].table, size);
        return size;
      }
      break;
    }
  }
  return 0;
}
