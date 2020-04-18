/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef SERVICE_RUNTIME_ARCH_MIPS_SEL_LDR_H__
#define SERVICE_RUNTIME_ARCH_MIPS_SEL_LDR_H__ 1

#if !defined(__ASSEMBLER__)
# include "native_client/src/include/portability.h"
#endif

#include "native_client/src/trusted/service_runtime/nacl_config.h"

#define NACL_MAX_ADDR_BITS      30

/*
 * Guard region is 32KB wide for MIPS32. This comes from the fact that load
 * and store instructions can use signed 16-bit offset from a register, so
 * the guard region is [0x40000000-0x40007FFF]. The guard size of 32KB is
 * sufficient only in case this memory is mapped as PROT_NONE. Otherwise,
 * we need to extend the guard region.
 *
 * Example of lw/sw instruction on MIPS: lw $a0, 0x7FFF($a0)
 *
 * Lower guard size is zero, as there is no need to have guard region in the
 * address range [0xFFFF8000-0xFFFFFFFF] since this address space belongs to
 * kernel virtual space, and user space already does not have privilege to
 * access it.
 */
#define NACL_ADDRSPACE_LOWER_GUARD_SIZE 0
#define NACL_ADDRSPACE_UPPER_GUARD_SIZE 0x8000

/* Must be synced with irt_compatible_rodata_addr in SConstruct */
#define NACL_DATA_SEGMENT_START 0x10000000

#define NACL_THREAD_MAX         8192

#endif /* SERVICE_RUNTIME_ARCH_MIPS_SEL_LDR_H__ */
