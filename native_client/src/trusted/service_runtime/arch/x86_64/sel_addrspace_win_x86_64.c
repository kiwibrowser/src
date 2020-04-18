/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "native_client/src/shared/platform/nacl_check.h"
#include "native_client/src/shared/platform/nacl_find_addrsp.h"
#include "native_client/src/trusted/service_runtime/arch/sel_ldr_arch.h"
#include "native_client/src/trusted/service_runtime/sel_ldr.h"


#define FOURGIG     (((size_t) 1) << 32)
#define ALIGN_BITS  32

#define MAX_ADDRESS_RANDOMIZATION_ATTEMPTS  8


static uintptr_t NaClFindAddressSpacePow2Aligned(size_t mem_sz,
                                                 size_t log_alignment,
                                                 enum NaClAslrMode aslr_mode) {
  uintptr_t pow2align = ((uintptr_t) 1) << log_alignment;
  size_t request_size = mem_sz + pow2align;
  uintptr_t unrounded_addr;
  int found_memory;

  if (NACL_ENABLE_ASLR == aslr_mode) {
    found_memory = NaClFindAddressSpaceRandomized(
        &unrounded_addr, request_size,
        MAX_ADDRESS_RANDOMIZATION_ATTEMPTS);
  } else {
    found_memory = NaClFindAddressSpace(&unrounded_addr, request_size);
  }
  if (!found_memory) {
    NaClLog(LOG_FATAL,
            "NaClFindAddressSpacePow2Aligned: Failed to reserve %"NACL_PRIxS
            " bytes of address space\n",
            request_size);
  }
  return (unrounded_addr + (pow2align - 1)) & ~(pow2align - 1);
}

NaClErrorCode NaClAllocateSpaceAslr(void **mem, size_t addrsp_size,
                                    enum NaClAslrMode aslr_mode) {
  /* 40G guard on each side */
  size_t mem_sz = (NACL_ADDRSPACE_LOWER_GUARD_SIZE + FOURGIG +
                   NACL_ADDRSPACE_UPPER_GUARD_SIZE);
  uintptr_t rounded_addr;
  void *request_addr;
  void *allocated;
  uintptr_t mem_ptr;

  CHECK(addrsp_size == FOURGIG);

  rounded_addr = NaClFindAddressSpacePow2Aligned(mem_sz, ALIGN_BITS,
                                                 aslr_mode);

  /*
   * Reserve the guard regions.  We can map these with a single
   * VirtualAlloc() each because we do not need to remap any pages
   * inside the guard regions.
   */
  request_addr = (void *) rounded_addr;
  allocated = VirtualAlloc(request_addr, NACL_ADDRSPACE_LOWER_GUARD_SIZE,
                           MEM_RESERVE, PAGE_NOACCESS);
  if (allocated != request_addr) {
    NaClLog(LOG_FATAL,
            "NaClAllocateSpace: Failed to reserve first guard region\n");
  }
  request_addr = (void *) (rounded_addr + NACL_ADDRSPACE_LOWER_GUARD_SIZE +
                           FOURGIG);
  allocated = VirtualAlloc(request_addr, NACL_ADDRSPACE_UPPER_GUARD_SIZE,
                           MEM_RESERVE, PAGE_NOACCESS);
  if (allocated != request_addr) {
    NaClLog(LOG_FATAL,
            "NaClAllocateSpace: Failed to reserve second guard region\n");
  }
  /*
   * Reserve the main part of address space.  Each page needs to be
   * mapped with a separate VirtualAlloc() call so that it can be
   * independently remapped later.
   */
  for (mem_ptr = rounded_addr + NACL_ADDRSPACE_LOWER_GUARD_SIZE;
       mem_ptr < rounded_addr + NACL_ADDRSPACE_LOWER_GUARD_SIZE + FOURGIG;
       mem_ptr += NACL_MAP_PAGESIZE) {
    allocated = VirtualAlloc((void *) mem_ptr,
                             NACL_MAP_PAGESIZE,
                             MEM_RESERVE,
                             PAGE_NOACCESS);
    if ((uintptr_t) allocated != mem_ptr) {
      NaClLog(LOG_FATAL,
              "NaClAllocateSpace: Failed to reserve page at %"NACL_PRIxPTR"\n",
              mem_ptr);
    }
  }
  *mem = (void *) (rounded_addr + NACL_ADDRSPACE_LOWER_GUARD_SIZE);
  return LOAD_OK;
}
