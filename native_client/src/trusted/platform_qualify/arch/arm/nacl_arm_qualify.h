/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef NATIVE_CLIENT_SRC_TRUSTED_PLATFORM_QUALIFY_ARCH_ARM_NACL_ARM_QUALIFY_H_
#define NATIVE_CLIENT_SRC_TRUSTED_PLATFORM_QUALIFY_ARCH_ARM_NACL_ARM_QUALIFY_H_

#include "native_client/src/include/nacl_base.h"

EXTERN_C_BEGIN

/*
 * Returns 1 if the CPU supports the required VFP/vector instruction sets.
 */
int NaClQualifyFpu(void);

/*
 * Returns 1 if special sandbox instructions trap as expected.
 */
int NaClQualifySandboxInstrs(void);

/*
 * Returns 1 if unaligned load/stores work properly.
 */
int NaClQualifyUnaligned(void);

EXTERN_C_END

#endif
