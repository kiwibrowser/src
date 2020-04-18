/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef SERVICE_RUNTIME_ARCH_ARM_SEL_LDR_H__
#define SERVICE_RUNTIME_ARCH_ARM_SEL_LDR_H__ 1

#include "native_client/src/trusted/service_runtime/nacl_config.h"

/* NOTE: we hope to unify this among archtectures */
#define NACL_MAX_ADDR_BITS      30

#define NACL_ADDRSPACE_LOWER_GUARD_SIZE 0
#define NACL_ADDRSPACE_UPPER_GUARD_SIZE 0x2000

#define NACL_THREAD_MAX         8192  /* arbitrary, can be larger */

#endif /* SERVICE_RUNTIME_ARCH_ARM_SEL_LDR_H__ */
