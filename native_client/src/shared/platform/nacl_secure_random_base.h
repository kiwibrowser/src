/*
 * Copyright 2008 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * NaCl Service Runtime.  Secure RNG abstraction.  Base class.
 */

#ifndef NATIVE_CLIENT_SRC_TRUSTED_PLATFORM_NACL_SECURE_RANDOM_BASE_H_
#define NATIVE_CLIENT_SRC_TRUSTED_PLATFORM_NACL_SECURE_RANDOM_BASE_H_

#include "native_client/src/include/portability.h"

#include "native_client/src/include/nacl_base.h"

EXTERN_C_BEGIN

struct NaClSecureRngIf;  /* fwd: base interface class */

struct NaClSecureRngIfVtbl {
  void      (*Dtor)(struct NaClSecureRngIf  *self);
  /*
   * Generates a uniform random 8-bit byte (uint8_t).
   */
  uint8_t   (*GenByte)(struct NaClSecureRngIf *self);
  /*
   * Generates a uniform random 32-bit unsigned int.
   */
  uint32_t  (*GenUint32)(struct NaClSecureRngIf *self);
  /*
   * Generate an uniformly random number in [0, range_max).  May invoke
   * the provided generator multiple times.
   */
  void      (*GenBytes)(struct NaClSecureRngIf  *self,
                        uint8_t                 *buf,
                        size_t                  nbytes);
  uint32_t  (*Uniform)(struct NaClSecureRngIf *self,
                       uint32_t               range_max);
};

struct NaClSecureRngIf {
  struct NaClSecureRngIfVtbl const  *vtbl;
};

EXTERN_C_END

#endif  /* NATIVE_CLIENT_SRC_TRUSTED_PLATFORM_NACL_SECURE_RANDOM_BASE_H_ */
