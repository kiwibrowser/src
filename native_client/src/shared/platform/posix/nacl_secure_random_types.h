/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * NaCl Service Runtime.  Secure RNG abstraction.
 */

#ifndef NATIVE_CLIENT_SRC_TRUSTED_PLATFORM_LINUX_NACL_SECURE_RANDOM_TYPES_H__
#define NATIVE_CLIENT_SRC_TRUSTED_PLATFORM_LINUX_NACL_SECURE_RANDOM_TYPES_H__

#include "native_client/src/include/nacl_base.h"

#include "native_client/src/shared/platform/nacl_secure_random_base.h"

EXTERN_C_BEGIN

# define  NACL_RANDOM_BUFFER_SIZE  1024

struct NaClSecureRng {
  struct NaClSecureRngIf  base;
  unsigned char           buf[NACL_RANDOM_BUFFER_SIZE];
  int                     nvalid;
};

EXTERN_C_END

#endif
/* NATIVE_CLIENT_SRC_TRUSTED_PLATFORM_LINUX_NACL_SECURE_RANDOM_TYPES_H__ */
