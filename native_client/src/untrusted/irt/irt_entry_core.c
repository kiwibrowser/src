/*
 * Copyright (c) 2014 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "native_client/src/public/irt_core.h"
#include "native_client/src/untrusted/irt/irt.h"
#include "native_client/src/untrusted/irt/irt_dev.h"
#include "native_client/src/untrusted/irt/irt_interfaces.h"

/*
 * This is a list of IRT interface implementations to include in the NaCl
 * core IRT, but not in IRTs built by embedders of NaCl (such as Chromium).
 *
 * Chromium provides its own implementation of open_resource().
 * open_resource() is disabled under PNaCl (see irt.h), but we don't need
 * to conditionalize its availability here since this definition is only
 * used for the core IRT.
 *
 * Similarly, Chromium provides its own implementations of the PNaCl
 * translator compile/link interfaces.
 */
static const struct nacl_irt_interface irt_interfaces[] = {
  { NACL_IRT_RESOURCE_OPEN_v0_1, &nacl_irt_resource_open,
    sizeof(nacl_irt_resource_open), NULL },
  { NACL_IRT_PRIVATE_PNACL_TRANSLATOR_LINK_v0_1,
    &nacl_irt_private_pnacl_translator_link,
    sizeof(nacl_irt_private_pnacl_translator_link), NULL },
  { NACL_IRT_PRIVATE_PNACL_TRANSLATOR_COMPILE_v0_1,
    &nacl_irt_private_pnacl_translator_compile,
    sizeof(nacl_irt_private_pnacl_translator_compile), NULL },
};

static size_t irt_query(const char *interface_ident,
                        void *table, size_t tablesize) {
  size_t result = nacl_irt_query_core(interface_ident, table, tablesize);
  if (result != 0)
    return result;
  return nacl_irt_query_list(interface_ident, table, tablesize,
                             irt_interfaces, sizeof(irt_interfaces));
}

void nacl_irt_start(uint32_t *info) {
  nacl_irt_init(info);
  nacl_irt_enter_user_code(info, irt_query);
}
