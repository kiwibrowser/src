/*
 * Copyright (c) 2010 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */


#include <windows.h>

#include "native_client/src/shared/platform/nacl_find_addrsp.h"

#include "native_client/src/include/build_config.h"
#include "native_client/src/include/nacl_base.h"
#include "native_client/src/include/portability.h"
#include "native_client/src/shared/platform/nacl_check.h"
#include "native_client/src/shared/platform/nacl_global_secure_random.h"
#include "native_client/src/shared/platform/nacl_log.h"

#if NACL_ARCH(NACL_BUILD_ARCH) == NACL_x86 && NACL_BUILD_SUBARCH == 64
/*
 * The x86-64 (currently) permits only a 48 bit virtual address space
 * (and requires that the upper 64-48=16 bits is sign extended from
 * bit 47).  Additionally, (empirically) Windows 8 disallows addresses
 * with bits 44 and above set for user-space code.  So instead of 48,
 * we get a maximum of 43 usable bits of virtual address.
 */
# define NUM_USER_ADDRESS_BITS 43
/*
 * Windows is P64 and not LP64
 */
# define NACL_POINTER_SIZE 64
#else
# define NUM_USER_ADDRESS_BITS 31
# define NACL_POINTER_SIZE 32
#endif


/* bool */
int NaClFindAddressSpaceRandomized(uintptr_t *addr, size_t memory_size,
                                   int max_tries) {
  void *map_addr = NULL;
  int tries_remaining;
  /*
   * Mask for keeping the low order NUM_USER_ADDRESS_BITS of a randomly
   * generated address.  VirtualAlloc will drop the low 16 bits for
   * MEM_RESERVE, so we do not need to mask those off.
   */
  uintptr_t addr_mask;
  uintptr_t suggested_addr;

  CHECK(max_tries >= 0);
  NaClLog(4,
          "NaClFindAddressSpaceRandomized: looking for %"NACL_PRIxS" bytes\n",
          memory_size);

  /* 64kB allocation size */
  addr_mask = ~(uintptr_t) 0;
#if NACL_POINTER_SIZE > NUM_USER_ADDRESS_BITS
  addr_mask &= (((uintptr_t) 1 << NUM_USER_ADDRESS_BITS) - 1);
#endif

  for (tries_remaining = max_tries; tries_remaining >= 0; --tries_remaining) {
    if (tries_remaining > 0) {
      /* Pick a random address for the first max_tries */
#if NACL_POINTER_SIZE > 32
      suggested_addr = (((uintptr_t) NaClGlobalSecureRngUint32() << 32) |
                        (uintptr_t) NaClGlobalSecureRngUint32());
#else
      suggested_addr = ((uintptr_t) NaClGlobalSecureRngUint32());
#endif
      suggested_addr &= addr_mask;
    } else {
      /* Give up picking randomly -- now just let the system pick */
      suggested_addr = 0;
    }

    NaClLog(4,
            ("NaClFindAddressSpaceRandomized: try %d: starting addr hint %"
             NACL_PRIxPTR"\n"),
            max_tries - tries_remaining,
            suggested_addr);
    map_addr = VirtualAlloc((void *) suggested_addr,
                            memory_size, MEM_RESERVE, PAGE_READWRITE);
    if (NULL != map_addr) {
      /* success */
      break;
    }
  }
  if (NULL == map_addr) {
    NaClLog(LOG_ERROR,
            ("NaClFindAddressSpaceRandomized: VirtualAlloc failed looking for"
             " 0x%"NACL_PRIxS" bytes\n"),
            memory_size);
    return 0;
  }
  if (!VirtualFree(map_addr, 0, MEM_RELEASE)) {
    NaClLog(LOG_FATAL,
            ("NaClFindAddressSpaceRandomized: VirtualFree of VirtualAlloc"
             " result (0x%"NACL_PRIxPTR"failed, GetLastError %d.\n"),
            (uintptr_t) map_addr,
            GetLastError());
  }
  NaClLog(4,
          "NaClFindAddressSpaceRandomized: got addr %"NACL_PRIxPTR"\n",
          (uintptr_t) map_addr);
  *addr = (uintptr_t) map_addr;
  return 1;
}

int NaClFindAddressSpace(uintptr_t *addr, size_t memory_size) {
  return NaClFindAddressSpaceRandomized(addr, memory_size, 0);
}
