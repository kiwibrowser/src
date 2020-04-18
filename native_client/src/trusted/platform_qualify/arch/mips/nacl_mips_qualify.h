/*
 * Copyright (c) 2013 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef NATIVE_CLIENT_SRC_TRUSTED_PLATFORM_QUALIFY_ARCH_MIPS_NACL_MIPS_QUALIFY_H
#define NATIVE_CLIENT_SRC_TRUSTED_PLATFORM_QUALIFY_ARCH_MIPS_NACL_MIPS_QUALIFY_H

#include "native_client/src/include/nacl_base.h"

EXTERN_C_BEGIN

/*
 * Returns 1 if the CPU has FPU unit and floating point registers are in
 * 32-bit mode.
 */
int NaClQualifyFpu(void);

EXTERN_C_END

#endif
