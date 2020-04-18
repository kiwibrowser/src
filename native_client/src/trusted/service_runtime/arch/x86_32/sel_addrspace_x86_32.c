/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "native_client/src/include/build_config.h"

#if NACL_LINUX
#include <errno.h>
#include <sys/mman.h>
#endif

#include "native_client/src/include/nacl_platform.h"
#include "native_client/src/shared/platform/nacl_check.h"
#include "native_client/src/trusted/service_runtime/include/sys/errno.h"
#include "native_client/src/trusted/service_runtime/sel_addrspace.h"
#include "native_client/src/trusted/service_runtime/sel_memory.h"
#include "native_client/src/trusted/service_runtime/sel_ldr.h"


NaClErrorCode NaClAllocateSpaceAslr(void **mem, size_t addrsp_size,
                                    enum NaClAslrMode aslr_mode) {
  int result;
  int (*allocator)(void **, size_t) = ((NACL_ENABLE_ASLR == aslr_mode) ?
                                       NaClPageAllocRandomized :
                                       NaClPageAlloc);

  CHECK(NULL != mem);

  NaClAddrSpaceBeforeAlloc(addrsp_size);

#if NACL_LINUX
  /*
   * On 32 bit Linux, a 1 gigabyte block of address space may be reserved at
   * the zero-end of the address space during process creation, to address
   * sandbox layout requirements on ARM and performance issues on Intel ATOM.
   * Look for this prereserved block and if found, pass its address to the
   * page allocation function.
   */
  if (NaClFindPrereservedSandboxMemory(mem, addrsp_size)) {
    void *tmp_mem = (void *) NACL_TRAMPOLINE_START;
    CHECK(*mem == 0);
    addrsp_size -= NACL_TRAMPOLINE_START;
    result = NaClPageAllocAtAddr(&tmp_mem, addrsp_size);
  } else {
    /* Zero-based sandbox not prereserved. Attempt to allocate anyway. */
    result = (*allocator)(mem, addrsp_size);
  }
#elif NACL_WINDOWS
  /*
   * On 32 bit Windows, a 1 gigabyte block of address space is reserved before
   * starting up this process to make sure we can create the sandbox. Look for
   * this prereserved block and if found, pass its address to the page
   * allocation function.
   */
  if (0 == NaClFindPrereservedSandboxMemory(mem, addrsp_size)) {
    result = NaClPageAllocAtAddr(mem, addrsp_size);
  } else {
    result = (*allocator)(mem, addrsp_size);
  }
#else
  result = (*allocator)(mem, addrsp_size);
#endif

  if (0 != result) {
    NaClLog(2,
        "NaClAllocateSpace: NaClPageAlloc 0x%08"NACL_PRIxPTR
        " failed\n",
        (uintptr_t) *mem);
    return LOAD_NO_MEMORY_FOR_ADDRESS_SPACE;
  }
  NaClLog(4, "NaClAllocateSpace: %"NACL_PRIxPTR", %"NACL_PRIxS"\n",
          (uintptr_t) *mem,
          addrsp_size);

  return LOAD_OK;
}
