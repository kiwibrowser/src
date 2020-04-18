/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef NATIVE_CLIENT_SRC_TRUSTED_SERVICE_RUNTIME_ARCH_X86_SEL_LDR_X86__H_
#define NATIVE_CLIENT_SRC_TRUSTED_SERVICE_RUNTIME_ARCH_X86_SEL_LDR_X86__H_

#include "native_client/src/include/build_config.h"

/* to make LDT_ENTRIES available */
#if NACL_WINDOWS
# define LDT_ENTRIES 8192
#elif NACL_OSX
# define LDT_ENTRIES 8192
#elif NACL_LINUX
# include <asm/ldt.h>
#endif

#include "native_client/src/trusted/service_runtime/arch/x86/nacl_ldt_x86.h"

#if NACL_BUILD_SUBARCH == 32
# define NACL_MAX_ADDR_BITS  (30)
# define NACL_THREAD_MAX     LDT_ENTRIES  /* cannot be larger */
/* No guard regions on x86-32 */
# define NACL_ADDRSPACE_LOWER_GUARD_SIZE 0
# define NACL_ADDRSPACE_UPPER_GUARD_SIZE 0
#elif NACL_BUILD_SUBARCH == 64
# define NACL_MAX_ADDR_BITS  (32)
# define NACL_THREAD_MAX     LDT_ENTRIES  /* can be larger */
# define NACL_ADDRSPACE_LOWER_GUARD_SIZE ((size_t) 40 << 30)  /* 40GB guard */
# define NACL_ADDRSPACE_UPPER_GUARD_SIZE ((size_t) 40 << 30)  /* 40GB guard */
#else
# error "Did Intel or AMD introduce the 128-bit x86?"
#endif

#endif /* SERVICE_RUNTIME_ARCH_X86_SEL_LDR_H__ */
