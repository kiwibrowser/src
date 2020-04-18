/*
 * Copyright 2009 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef NATIVE_CLIENT_SRC_SHARED_PLATFORM_LINUX_NACL_TIME_TYPES_H_
#define NATIVE_CLIENT_SRC_SHARED_PLATFORM_LINUX_NACL_TIME_TYPES_H_

#include "native_client/src/include/portability.h"
#include "native_client/src/include/nacl_base.h"

EXTERN_C_BEGIN

/*
 * NaClTimeState definition for Unix-like OSes.
 */

struct NaClTimeState {
  uint64_t time_resolution_ns;
};

EXTERN_C_END

#endif
