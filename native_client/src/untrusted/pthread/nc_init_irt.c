/*
 * Copyright (c) 2011 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "native_client/src/untrusted/nacl/nacl_irt.h"
#include "native_client/src/untrusted/pthread/pthread_internal.h"

struct nacl_irt_futex __libnacl_irt_futex;

void __nc_initialize_interfaces(void) {
  __libnacl_mandatory_irt_query(NACL_IRT_THREAD_v0_1,
                                &__libnacl_irt_thread,
                                sizeof(__libnacl_irt_thread));
  __libnacl_mandatory_irt_query(NACL_IRT_FUTEX_v0_1,
                                &__libnacl_irt_futex,
                                sizeof(__libnacl_irt_futex));
}
