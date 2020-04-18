/*
 * Copyright (c) 2013 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "native_client/src/untrusted/irt/irt.h"
#include "native_client/src/untrusted/irt/irt_dev.h"
#include "native_client/src/untrusted/nacl/syscall_bindings_trampoline.h"

static int nacl_irt_list_mappings(struct NaClMemMappingInfo *regions,
                                  size_t count, size_t *result_count) {
  int ret = NACL_SYSCALL(list_mappings)(regions, count);
  if (ret < 0) {
    return ret;
  } else {
    *result_count = ret;
    return 0;
  }
}

const struct nacl_irt_dev_list_mappings nacl_irt_dev_list_mappings = {
  nacl_irt_list_mappings,
};
