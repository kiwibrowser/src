/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "native_client/src/include/nacl_platform.h"
#include "native_client/src/shared/platform/nacl_check.h"
#include "native_client/src/trusted/service_runtime/arch/sel_ldr_arch.h"
#include "native_client/src/trusted/service_runtime/nacl_error_code.h"
#include "native_client/src/trusted/service_runtime/sel_addrspace.h"
#include "native_client/src/trusted/service_runtime/sel_ldr.h"
#include "native_client/src/trusted/service_runtime/sel_memory.h"


/* NOTE: This routine is almost identical to the x86_32 version.
 */
NaClErrorCode NaClAllocateSpaceAslr(void **mem, size_t addrsp_size,
                                    enum NaClAslrMode aslr_mode) {
  int result;
  void *tmp_mem = (void *) NACL_TRAMPOLINE_START;

  UNREFERENCED_PARAMETER(aslr_mode);
  CHECK(NULL != mem);

  /*
   * On ARM, we cheat slightly: we add two pages to the requested
   * allocation!  This accomodates the guard region we require at the
   * top end of untrusted memory.
   */
  addrsp_size += NACL_ADDRSPACE_UPPER_GUARD_SIZE;

  NaClAddrSpaceBeforeAlloc(addrsp_size);

  /*
   * On 32 bit Linux, a 1 gigabyte block of address space may be reserved at
   * the zero-end of the address space during process creation, to address
   * sandbox layout requirements on ARM and performance issues on Intel ATOM.
   * Look for this prereserved block and if found, pass its address to the
   * page allocation function.
   */
  if (!NaClFindPrereservedSandboxMemory(mem, addrsp_size)) {
    /* On ARM, we should always have prereserved sandbox memory. */
    NaClLog(LOG_ERROR, "NaClAllocateSpace:"
            " Could not find correct amount of prereserved memory"
            " (looked for 0x%016"NACL_PRIxS" bytes).\n",
            addrsp_size);
    return LOAD_NO_MEMORY_FOR_ADDRESS_SPACE;
  }
  /*
   * When creating a zero-based sandbox, we do not allocate the first 64K of
   * pages beneath the trampolines, because -- on Linux at least -- we cannot.
   * Instead, we allocate starting at the trampolines, and then coerce the
   * "mem" out parameter.
   */
  CHECK(*mem == NULL);
  addrsp_size -= NACL_TRAMPOLINE_START;
  result = NaClPageAllocAtAddr(&tmp_mem, addrsp_size);

  if (0 != result) {
    NaClLog(2,
            "NaClAllocateSpace: NaClPageAllocAtAddr 0x%08"NACL_PRIxPTR
            " failed\n",
            (uintptr_t) tmp_mem);
    return LOAD_NO_MEMORY_FOR_ADDRESS_SPACE;
  }
  NaClLog(4, "NaClAllocateSpace: %"NACL_PRIxPTR", %"NACL_PRIxS"\n",
          (uintptr_t) *mem,
          addrsp_size);

  return LOAD_OK;
}

