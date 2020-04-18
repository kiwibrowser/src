/*
 * Copyright 2008 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * NaCl Server Runtime threads implementation layer.
 */

#ifndef NATIVE_CLIENT_SRC_TRUSTED_PLATFORM_WIN_NACL_THREADS_TYPES_H_
#define NATIVE_CLIENT_SRC_TRUSTED_PLATFORM_WIN_NACL_THREADS_TYPES_H_

#include "native_client/src/include/portability.h"
#include <windows.h>

struct NaClThread {
  HANDLE tid; /* thread handle */
};

#endif  /* NATIVE_CLIENT_SRC_TRUSTED_PLATFORM_WIN_NACL_THREADS_TYPES_H_ */
