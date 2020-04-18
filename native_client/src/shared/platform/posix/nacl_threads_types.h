/*
 * Copyright 2008 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * NaCl Server Runtime threads implementation layer.
 */

#ifndef NATIVE_CLIENT_SRC_TRUSTED_PLATFORM_LINUX_NACL_THREADS_TYPES_H_
#define NATIVE_CLIENT_SRC_TRUSTED_PLATFORM_LINUX_NACL_THREADS_TYPES_H_

#include <pthread.h>

#include "native_client/src/include/nacl_base.h"

EXTERN_C_BEGIN

struct NaClThread {
  pthread_t tid;
};

EXTERN_C_END

#endif  /* NATIVE_CLIENT_SRC_TRUSTED_PLATFORM_LINUX_NACL_THREADS_TYPES_H_ */
