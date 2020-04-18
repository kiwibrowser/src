/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * NaCl Service Runtime memory allocation code
 */

#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "native_client/src/include/build_config.h"
#include "native_client/src/include/nacl_platform.h"
#include "native_client/src/include/portability.h"
#include "native_client/src/shared/platform/nacl_exit.h"
#include "native_client/src/shared/platform/nacl_global_secure_random.h"
#include "native_client/src/shared/platform/nacl_log.h"
#include "native_client/src/trusted/service_runtime/sel_memory.h"
#include "native_client/src/trusted/service_runtime/nacl_config.h"
#include "native_client/src/trusted/service_runtime/include/machine/_types.h"

void NaClPageFree(void *p, size_t size) {
  if (p == 0 || size == 0)
    return;
  if (munmap(p, size) != 0) {
    NaClLog(LOG_FATAL, "NaClPageFree: munmap() failed");
  }
}

static int NaClPageAllocInternalFlags(void **p, size_t size, int map_flags) {
  void *addr;

  map_flags |= MAP_PRIVATE | MAP_ANONYMOUS;

  addr = mmap(*p, size, PROT_NONE, map_flags, -1, (off_t) 0);
  if (MAP_FAILED == addr) {
    addr = NULL;
  }
  if (NULL != addr) {
    *p = addr;
  }
  return (NULL == addr) ? -ENOMEM : 0;
}

/*
 * Note that NaClPageAlloc does not allocate pages that satisify
 * NaClIsAllocPageMultiple.  On linux/osx, the system does not impose
 * any such restrictions, and we only need to enforce the restriction
 * on NaCl app code to ensure that the app code is portable across all
 * host OSes.
 */
static int NaClPageAllocInternal(void **p, size_t size) {
  int map_flags = 0;

  if (NULL != *p) {
    map_flags |= MAP_FIXED;
  }
 #if NACL_LINUX
  /*
   * Indicate to the kernel that we just want these pages allocated, not
   * committed.  This is important for systems with relatively little RAM+swap.
   * See bug 251.
   */
  map_flags |= MAP_NORESERVE;
#elif NACL_OSX
  /*
   * TODO(cbiffle): This file is used by Mac OS X as well as Linux.
   * An equivalent fix may require this to stop, since we might have
   * to drop to the xnu layer and use vm_allocate.
   *
   * Currently this code is not guaranteed to work for non-x86-32 Mac OS X.
   */
#else
# error This file should be included only by Linux and (surprisingly) OS X.
#endif
  return NaClPageAllocInternalFlags(p, size, map_flags);
}

/*
 * Pick a "hint" address that is random.
 */
int NaClPageAllocRandomized(void **p, size_t size) {
  uintptr_t       addr;
  int             neg_errno = -ENOMEM;  /* in case we change kNumTries to 0 */
  int             tries;
  const int       kNumTries = 4;
  /*
   * linux permits 128 TB of user address space.
   */

  for (tries = 0; tries < kNumTries; ++tries) {
#if NACL_HOST_WORDSIZE == 32
    addr = NaClGlobalSecureRngUint32();
    NaClLog(2, "NaClPageAllocRandomized: 0x%"NACL_PRIxPTR"\n", addr);
    /* linux permits 3-4 GB of user address space */
    *p = (void *) (addr & ~((uintptr_t) NACL_MAP_PAGESIZE - 1)
                   & ((~(uintptr_t) 0) >> 1));
#elif NACL_HOST_WORDSIZE == 64
    addr = NaClGlobalSecureRngUint32();
    NaClLog(2, "NaClPageAllocRandomized: 0x%"NACL_PRIxPTR"\n", addr);
    /*
     * linux permits 128 TB of user address space, and we keep the low
     * 16 bits free (64K alignment to match Windows), so we have
     * 47-16=31 bits of entropy.
     */
    *p = (void *) ((addr << NACL_MAP_PAGESHIFT)  /* bits [47:16] are random */
                   & ((((uintptr_t) 1) << 47) - 1));  /* now bits [46:16] */
#else
# error "where am i?"
#endif

    NaClLog(2, "NaClPageAllocRandomized: hint 0x%"NACL_PRIxPTR"\n",
            (uintptr_t) *p);
    neg_errno = NaClPageAllocInternalFlags(p, size, 0);
    if (0 == neg_errno) {
      break;
    }
  }
  if (0 != neg_errno) {
    NaClLog(LOG_INFO,
            "NaClPageAllocRandomized: failed (%d), dropping hints\n",
            -neg_errno);
    *p = 0;
    neg_errno = NaClPageAllocInternalFlags(p, size, 0);
  }
  return neg_errno;
}

int NaClPageAlloc(void **p, size_t size) {
  void *addr = NULL;
  int rv;

  if (0 == (rv = NaClPageAllocInternal(&addr, size))) {
    *p = addr;
  }

  return rv;
}

int NaClPageAllocAtAddr(void **p, size_t size) {
  return NaClPageAllocInternal(p, size);
}

/*
* This is critical to make the text region non-writable, and the data
* region read/write but no exec.  Of course, some kernels do not
* respect the lack of PROT_EXEC.
*/
int NaClMprotect(void *addr, size_t len, int prot) {
  int  ret = mprotect(addr, len, prot);

  return ret == -1 ? -errno : ret;
}


int NaClMadvise(void *start, size_t length, int advice) {
  int ret = madvise(start, length, advice);

  /*
   * MADV_DONTNEED and MADV_NORMAL are needed
   */
  return ret == -1 ? -errno : ret;
}
