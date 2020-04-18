/*
 * Copyright 2014 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <string.h>

#include "native_client/src/public/irt_core.h"

size_t nacl_irt_query_list(const char *interface_ident,
                           void *table, size_t tablesize,
                           const struct nacl_irt_interface *available,
                           size_t available_size) {
  unsigned available_count = available_size / sizeof(*available);
  unsigned i;
  for (i = 0; i < available_count; ++i) {
    if (0 == strcmp(interface_ident, available[i].name)) {
      if (NULL == available[i].filter || available[i].filter()) {
        const size_t size = available[i].size;
        if (size <= tablesize) {
          memcpy(table, available[i].table, size);
          return size;
        }
      }
      break;
    }
  }
  return 0;
}
