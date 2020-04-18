/*
 * Copyright (c) 2011 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "native_client/src/untrusted/irt/irt.h"
#include "native_client/src/untrusted/nacl/nacl_irt.h"

size_t nacl_interface_query(const char *interface_ident,
                            void *table, size_t tablesize) {
  if (NULL == __nacl_irt_query)
    return 0;
  return (*__nacl_irt_query)(interface_ident, table, tablesize);
}
