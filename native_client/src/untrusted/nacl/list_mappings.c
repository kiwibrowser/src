/*
 * Copyright (c) 2013 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <errno.h>
#include <unistd.h>

#include "native_client/src/untrusted/irt/irt.h"
#include "native_client/src/untrusted/irt/irt_dev.h"

/*
 * ABI table for underyling NaCl list_mappings interface.
 * Setup on demand.
 */
static struct nacl_irt_dev_list_mappings irt_list_mappings;

/*
 * We don't do any locking here, but simultaneous calls are harmless enough.
 * They'll all be writing the same values to the same words.
 * Returns 0 for success.
 */
static int set_up_irt_list_mappings(void) {
  if (NULL == irt_list_mappings.list_mappings) {
    if (nacl_interface_query(
          NACL_IRT_DEV_LIST_MAPPINGS_v0_1, &irt_list_mappings,
          sizeof(irt_list_mappings)) != sizeof(irt_list_mappings)) {
       return 1;
    }
  }
  return 0;
}

int nacl_list_mappings(struct NaClMemMappingInfo *regions, size_t count,
                       size_t *result_count) {
  if (set_up_irt_list_mappings()) {
    errno = ENOSYS;
    return -1;
  }
  int error = irt_list_mappings.list_mappings(regions, count, result_count);
  if (error) {
    errno = -error;
    return -1;
  }
  return 0;
}
