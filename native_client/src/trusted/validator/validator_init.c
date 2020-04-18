/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "native_client/src/include/build_config.h"
#include "native_client/src/shared/platform/nacl_log.h"
#include "native_client/src/trusted/validator/ncvalidate.h"


const struct NaClValidatorInterface *NaClCreateValidator(void) {
#if NACL_ARCH(NACL_BUILD_ARCH) == NACL_arm
  return NaClValidatorCreateArm();
#elif NACL_ARCH(NACL_BUILD_ARCH) == NACL_mips
  return NaClValidatorCreateMips();
#elif NACL_ARCH(NACL_BUILD_ARCH) == NACL_x86
# if NACL_BUILD_SUBARCH == 64
  return NaClDfaValidatorCreate_x86_64();
# elif NACL_BUILD_SUBARCH == 32
  return NaClDfaValidatorCreate_x86_32();
# else
#  error "Invalid sub-architecture!"
# endif
#else  /* NACL_x86 */
# error "There is no validator for this architecture!"
#endif
}
