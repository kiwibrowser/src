/*
 * Copyright (c) 2013 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "native_client/src/untrusted/nacl/nacl_irt.h"

void __libnacl_irt_dev_filename_init(void) {
  /* Attempt to load the 'dev-filename' interface */
  if (!__libnacl_irt_query(NACL_IRT_DEV_FILENAME_v0_3,
                           &__libnacl_irt_dev_filename,
                           sizeof(__libnacl_irt_dev_filename))) {
    /*
     * Fall back to old 'filename' interface if the dev interface is
     * not found.
     */
    if (!__libnacl_irt_query(NACL_IRT_DEV_FILENAME_v0_2,
                             &__libnacl_irt_dev_filename,
                             sizeof(struct nacl_irt_dev_filename_v0_2))) {
      __libnacl_irt_query(NACL_IRT_FILENAME_v0_1,
                          &__libnacl_irt_dev_filename,
                          sizeof(struct nacl_irt_filename));
    }
  }
}
