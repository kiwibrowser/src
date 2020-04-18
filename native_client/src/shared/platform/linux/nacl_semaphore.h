/*
 * Copyright 2008 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * NaCl semaphore type (Linux)
 */
#ifndef NATIVE_CLIENT_SRC_TRUSTED_PLATFORM_LINUX_NACL_SEMAPHORE_H_
#define NATIVE_CLIENT_SRC_TRUSTED_PLATFORM_LINUX_NACL_SEMAPHORE_H_

#include <semaphore.h>

struct NaClSemaphore {
  sem_t sem_obj;
};

#endif  /* NATIVE_CLIENT_SRC_TRUSTED_PLATFORM_LINUX_NACL_SEMAPHORE_H_ */
