/*
 * Copyright (c) 2013 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <assert.h>
#include <errno.h>
#include <stdio.h>

#include "native_client/src/trusted/service_runtime/include/sys/unistd.h"
#include "native_client/src/untrusted/irt/irt.h"
#include "native_client/src/untrusted/irt/irt_dev.h"
#include "native_client/src/untrusted/nacl/syscall_bindings_trampoline.h"

/*
 * Check that the dev interfaces are not available when running in
 * non debug mode.
 */
void test_dev_interfaces(void) {
  struct nacl_irt_dev_filename_v0_2 filename;
  struct nacl_irt_dev_list_mappings list_mappings;
  int nacl_file_access_enabled;
  int nacl_list_mappings_enabled;
  int rc;

  /*
   * We cannot call sysconf libc wrapper because glibc ignores
   * uknown sysconf values and always returns -1.
   */

  rc = NACL_SYSCALL(sysconf)(NACL_ABI__SC_NACL_FILE_ACCESS_ENABLED,
                             &nacl_file_access_enabled);
  assert(rc == 0);

  rc = nacl_interface_query(NACL_IRT_DEV_FILENAME_v0_2,
                            &filename, sizeof filename);
  assert(rc == (nacl_file_access_enabled ? sizeof filename : 0));

  rc = NACL_SYSCALL(sysconf)(NACL_ABI__SC_NACL_LIST_MAPPINGS_ENABLED,
                             &nacl_list_mappings_enabled);
  assert(rc == 0);

  rc = nacl_interface_query(NACL_IRT_DEV_LIST_MAPPINGS_v0_1,
                            &list_mappings, sizeof list_mappings);
  assert(rc == (nacl_list_mappings_enabled ? sizeof list_mappings : 0));
}

int main(void) {
  test_dev_interfaces();

  return 0;
}
