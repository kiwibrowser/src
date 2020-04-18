/*
 * Copyright (c) 2010 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <stdio.h>
#include <stdint.h>
#include <sys/mman.h>
#include <sys/types.h>

#include "native_client/src/shared/platform/nacl_find_addrsp.h"

#include "native_client/src/include/build_config.h"
#include "native_client/src/include/nacl_base.h"
#include "native_client/src/include/nacl_platform.h"
#include "native_client/src/include/portability.h"
#include "native_client/src/shared/platform/nacl_check.h"
#include "native_client/src/shared/platform/nacl_global_secure_random.h"
#include "native_client/src/shared/platform/nacl_log.h"


#if NACL_ARCH(NACL_BUILD_ARCH) == NACL_x86 && NACL_BUILD_SUBARCH == 64
/*
 * The x86-64 (currently) permits only a 48 bit virtual address
 * space (and requires that the upper 64-48=16 bits is sign extended
 * from bit 47).  Additionally, the linux kernel disallows negative
 * addresses for user-space code.  So instead of 48, we get a
 * maximum of 47 usable bits of virtual address.
 */
# define NUM_USER_ADDRESS_BITS 47
/*
 * Don't assume __LP64__ is defined, even though we're POSIX and thus
 * we are compiling with gcc or gcc-compatible compilers.
 */
# define NACL_POINTER_SIZE 64

#elif (NACL_ARCH(NACL_BUILD_ARCH) == NACL_x86 && NACL_BUILD_SUBARCH == 32) || \
      NACL_ARCH(NACL_BUILD_ARCH) == NACL_arm
/*
 * The x86-32 / ARM architectures use a 32-bit address space, but it is common
 * for the kernel to reserve the high 1 or 2 GB. In order to allow the start
 * address to be in the 2GB-3GB range, we choose from all possible 32-bit
 * addresses.
 */
# define NUM_USER_ADDRESS_BITS 32
# define NACL_POINTER_SIZE 32

#elif NACL_ARCH(NACL_BUILD_ARCH) == NACL_mips
/*
 * The MIPS architecture uses a 32-bit address space, but it is common for the
 * kernel to reserve the high 1 or 2 GB. The current MIPS platforms we target
 * will not be making use of the address space in the 2GB-4GB range.
 */
# define NUM_USER_ADDRESS_BITS 31
# define NACL_POINTER_SIZE 32

#else
# error "Is this a 32-bit or 64-bit architecture?"
#endif

#define NACL_VALGRIND_MAP_FAILED_COUNT  10000

/* bool */
int NaClFindAddressSpaceRandomized(uintptr_t *addr, size_t memory_size,
                                   int max_tries) {
  void *map_addr;
  int tries_remaining;
  /*
   * Mask for keeping the low order NUM_USER_ADDRESS_BITS of a randomly
   * generated address.
   */
  uintptr_t addr_mask;
  uintptr_t suggested_addr;
  int mmap_type;

  size_t map_failed_count_left = NACL_VALGRIND_MAP_FAILED_COUNT;

  CHECK(max_tries >= 0);
  NaClLog(4,
          "NaClFindAddressSpaceRandomized: looking for %"NACL_PRIxS" bytes\n",
          memory_size);
  NaClLog(4, "NaClFindAddressSpaceRandomized: max %d tries\n", max_tries);

  if (NACL_ARCH(NACL_BUILD_ARCH) != NACL_mips) {
    /* 4kB pages */
    addr_mask = ~(((uintptr_t) 1 << 12) - 1);
    mmap_type = MAP_PRIVATE;
  } else {
    /*
     * 8kB pages in userspace memory region.
     * Apart from system page size, MIPS shared memory mask in kernel can depend
     * on dcache attributes. Ideally, we could use kernel constant SHMLBA, but
     * it is too large. Common case is that 8kB is sufficient.
     * As shared memory mask alignment is more rigid on MIPS, we need to pass
     * MAP_SHARED type to mmap, so it can return value applicable both for
     * private and shared mapping.
     */
    addr_mask = ~(((uintptr_t) 1 << 13) - 1);
    mmap_type = MAP_SHARED;
  }
  /*
   * We cannot just do
   *
   *   if (NUM_USER_ADDRESS_BITS < sizeof(uintptr_t) * 8) {
   *     addr_mask &= (((uintptr_t) 1 << NUM_USER_ADDRESS_BITS) - 1);
   *   }
   *
   * since when NUM_USER_ADDRESS_BITS is defined to be 32, for
   * example, and sizeof(uintptr_t) is 4, then even though the
   * constant expression NUM_USER_ADDRESS_BITS < sizeof(uintptr_t) * 8
   * is false, the compiler still parses the block of code controlled
   * by the conditional.  And the warning for shifting by too many
   * bits would be produced because we'd venture into
   * undefined-behavior territory:
   *
   *   6.5.7.3: "The integer promotions are performed on each of the
   *   operands. The type of the result is that of the promoted left
   *   operand. If the value of the right operand is negative or is
   *   greater than or equal to the width of the promoted left
   *   operand, the behavior is undefined."
   *
   * Since we compile with -Wall and -Werror, this would lead to a
   * build failure.
   */
#if NACL_POINTER_SIZE > NUM_USER_ADDRESS_BITS
  addr_mask &= (((uintptr_t) 1 << NUM_USER_ADDRESS_BITS) - 1);
#endif

  tries_remaining = max_tries;
  for (;;) {
#if NACL_POINTER_SIZE > 32
    suggested_addr = (((uintptr_t) NaClGlobalSecureRngUint32() << 32) |
                      (uintptr_t) NaClGlobalSecureRngUint32());
#else
    suggested_addr = ((uintptr_t) NaClGlobalSecureRngUint32());
#endif
    suggested_addr &= addr_mask;

    NaClLog(4,
            ("NaClFindAddressSpaceRandomized: non-MAP_FAILED tries"
             " remaining %d, hint addr %"NACL_PRIxPTR"\n"),
            tries_remaining,
            suggested_addr);
    map_addr = mmap((void *) suggested_addr, memory_size,
                    PROT_NONE,
                    MAP_ANONYMOUS | MAP_NORESERVE | mmap_type,
                    -1,
                    0);
    /*
     * On most POSIX systems, the mmap syscall, without MAP_FIXED,
     * will use the first parameter (actual argument suggested_addr)
     * as a hint for where to find a hole in the address space for the
     * new memory -- often as a starting point for a search.
     *
     * However, when Valgrind is used (3.7.0 to 3.8.1 at least), the
     * mmap syscall is replaced with wrappers which do not behave
     * correctly: the Valgrind-provided mmap replacement will return
     * MAP_FAILED instead of ignoring the hint, until the "Warning:
     * set address range perms: large range [0x...., 0x....]
     * (noaccess)" message to warn about large allocations shows up.
     * So in order for this code to not fail when run under Valgrind,
     * we have to ignore MAP_FAILED and not count these attempts
     * against randomization failures.
     */
    if (MAP_FAILED == map_addr) {
      if (--map_failed_count_left != 0) {
        NaClLog(LOG_INFO, "NaClFindAddressSpaceRandomized: MAP_FAILED\n");
      } else {
        NaClLog(LOG_ERROR,
                "NaClFindAddressSpaceRandomized: too many MAP_FAILED\n");
        return 0;
      }
    } else {
      /*
       * mmap will use a system-dependent algorithm to find a starting
       * address if the hint location cannot work, e.g., if there is
       * already memory mapped there.  We do not trust that algorithm
       * to provide any randomness in the high-order bits, so we only
       * accept allocations that match the requested high-order bits
       * exactly.  If the algorithm is to scan the address space
       * starting near the hint address, then it would be acceptable;
       * if it is to use a default algorithm that is independent of
       * the supplied hint, then it would not be.
       */
      if ((addr_mask & ((uintptr_t) map_addr)) == suggested_addr) {
        NaClLog(5,
                "NaClFindAddressSpaceRandomized: high order bits matched.\n");
        /* success */
        break;
      }

      if (0 == tries_remaining--) {
        /* give up on retrying, and just use what was returned */
        NaClLog(5, "NaClFindAddressSpaceRandomized: last try, taking as is.\n");
        break;
      }
      /*
       * Remove undesirable mapping location before trying again.
       */
      if (-1 == munmap(map_addr, memory_size)) {
        NaClLog(LOG_FATAL,
                "NaClFindAddressSpaceRandomized: could not unmap non-random"
                " memory\n");
      }
    }
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
