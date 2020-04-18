/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <errno.h>
#include <sys/mman.h>

#include "native_client/src/include/build_config.h"

#if NACL_LINUX
/*
 * For getrlimit.
 */
# include <sys/resource.h>
#endif

#include "native_client/src/include/nacl_platform.h"
#include "native_client/src/shared/platform/nacl_check.h"
#include "native_client/src/shared/platform/nacl_find_addrsp.h"
#include "native_client/src/trusted/service_runtime/arch/sel_ldr_arch.h"
#include "native_client/src/trusted/service_runtime/sel_addrspace.h"
#include "native_client/src/trusted/service_runtime/sel_ldr.h"
#include "native_client/src/trusted/service_runtime/sel_memory.h"


#define FOURGIG     (((size_t) 1) << 32)
#define ALIGN_BITS  32
#define MAX_ADDRESS_RANDOMIZATION_ATTEMPTS  8
#define MSGWIDTH    "25"


/*
 * NaClAllocatePow2AlignedMemory is for allocating a large amount of
 * memory of mem_sz bytes that must be address aligned, so that
 * log_alignment low-order address bits must be zero.
 *
 * Returns the aligned region on success, or NULL on failure.
 */
static void *NaClAllocatePow2AlignedMemory(size_t mem_sz,
                                           size_t log_alignment,
                                           enum NaClAslrMode aslr_mode) {
  uintptr_t pow2align;
  size_t request_size;
  uintptr_t unrounded_addr;
  uintptr_t rounded_addr;
  size_t extra;
  int found_memory;

  pow2align = ((uintptr_t) 1) << log_alignment;
  request_size = mem_sz + pow2align;

  NaClLog(4,
          "%"MSGWIDTH"s %016"NACL_PRIxS"\n",
          " Ask:",
          request_size);
  if (NACL_ENABLE_ASLR == aslr_mode) {
    found_memory = NaClFindAddressSpaceRandomized(
        &unrounded_addr,
        request_size,
        MAX_ADDRESS_RANDOMIZATION_ATTEMPTS);
  } else {
    found_memory = NaClFindAddressSpace(&unrounded_addr, request_size);
  }
  if (!found_memory) {
    NaClLog(LOG_FATAL,
            "NaClAllocatePow2AlignedMemory: Failed to reserve %"NACL_PRIxS
            " bytes of address space\n",
            request_size);
  }

  NaClLog(4,
          "%"MSGWIDTH"s %016"NACL_PRIxPTR"\n",
          "orig memory at",
          unrounded_addr);

  rounded_addr = (unrounded_addr + (pow2align - 1)) & ~(pow2align - 1);
  extra = rounded_addr - unrounded_addr;

  if (0 != extra) {
    NaClLog(4,
            "%"MSGWIDTH"s %016"NACL_PRIxPTR", %016"NACL_PRIxS"\n",
            "Freeing front:",
            unrounded_addr,
            extra);
    if (-1 == munmap((void *) unrounded_addr, extra)) {
      perror("munmap (front)");
      NaClLog(LOG_FATAL,
              "NaClAllocatePow2AlignedMemory: munmap front failed\n");
    }
  }

  extra = pow2align - extra;
  if (0 != extra) {
    NaClLog(4,
            "%"MSGWIDTH"s %016"NACL_PRIxPTR", %016"NACL_PRIxS"\n",
            "Freeing tail:",
            rounded_addr + mem_sz,
            extra);
    if (-1 == munmap((void *) (rounded_addr + mem_sz),
         extra)) {
      perror("munmap (end)");
      NaClLog(LOG_FATAL,
              "NaClAllocatePow2AlignedMemory: munmap tail failed\n");
    }
  }
  NaClLog(4,
          "%"MSGWIDTH"s %016"NACL_PRIxPTR"\n",
          "Aligned memory:",
          rounded_addr);

  /*
   * we could also mmap again at rounded_addr w/o MAP_NORESERVE etc to
   * ensure that we have the memory, but that's better done in another
   * utility function.  the semantics here is no paging space
   * reserved, as in Windows MEM_RESERVE without MEM_COMMIT.
   */

  return (void *) rounded_addr;
}

NaClErrorCode NaClAllocateSpaceAslr(void **mem, size_t addrsp_size,
                                    enum NaClAslrMode aslr_mode) {
  /* 40G guard on each side */
  size_t        mem_sz = (NACL_ADDRSPACE_LOWER_GUARD_SIZE + FOURGIG +
                          NACL_ADDRSPACE_UPPER_GUARD_SIZE);
  size_t        log_align = ALIGN_BITS;
  void          *mem_ptr;
#if NACL_LINUX
  struct rlimit rlim;
#endif

  NaClLog(4, "NaClAllocateSpace(*, 0x%016"NACL_PRIxS" bytes).\n",
          addrsp_size);

  CHECK(addrsp_size == FOURGIG);

  if (NACL_X86_64_ZERO_BASED_SANDBOX) {
    mem_sz = 11 * FOURGIG;
    if (getenv("NACL_ENABLE_INSECURE_ZERO_BASED_SANDBOX") != NULL) {
      /*
       * For the zero-based 64-bit sandbox, we want to reserve 44GB of address
       * space: 4GB for the program plus 40GB of guard pages.  Due to a binutils
       * bug (see http://sourceware.org/bugzilla/show_bug.cgi?id=13400), the
       * amount of address space that the linker can pre-reserve is capped
       * at 4GB. For proper reservation, GNU ld version 2.22 or higher
       * needs to be used.
       *
       * Without the bug fix, trying to reserve 44GB will result in
       * pre-reserving the entire capped space of 4GB.  This tricks the run-time
       * into thinking that we can mmap up to 44GB.  This is unsafe as it can
       * overwrite the run-time program itself and/or other programs.
       *
       * For now, we allow a 4GB address space as a proof-of-concept insecure
       * sandboxing model.
       *
       * TODO(arbenson): remove this if block once the binutils bug is fixed
       */
      mem_sz = FOURGIG;
    }

    NaClAddrSpaceBeforeAlloc(mem_sz);
    if (NaClFindPrereservedSandboxMemory(mem, mem_sz)) {
      int result;
      void *tmp_mem = (void *) NACL_TRAMPOLINE_START;
      CHECK(*mem == 0);
      mem_sz -= NACL_TRAMPOLINE_START;
      result = NaClPageAllocAtAddr(&tmp_mem, mem_sz);
      if (0 != result) {
        NaClLog(2,
                "NaClAllocateSpace: NaClPageAlloc 0x%08"NACL_PRIxPTR
                " failed\n",
                (uintptr_t) *mem);
        return LOAD_NO_MEMORY_FOR_ADDRESS_SPACE;
      }
      NaClLog(4, "NaClAllocateSpace: %"NACL_PRIxPTR", %"NACL_PRIxS"\n",
              (uintptr_t) *mem,
              mem_sz);
      return LOAD_OK;
    }
    NaClLog(LOG_ERROR, "Failed to find prereserved memory\n");
    return LOAD_NO_MEMORY_FOR_ADDRESS_SPACE;
  }

  NaClAddrSpaceBeforeAlloc(mem_sz);

  errno = 0;
  mem_ptr = NaClAllocatePow2AlignedMemory(mem_sz, log_align, aslr_mode);
  if (NULL == mem_ptr) {
    if (0 != errno) {
      perror("NaClAllocatePow2AlignedMemory");
    }
    NaClLog(LOG_WARNING, "Memory allocation failed\n");
#if NACL_LINUX
    /*
     * Check with getrlimit whether RLIMIT_AS was likely to be the
     * problem with an allocation failure.  If so, generate a log
     * message.  Since this is a debugging aid and we don't know about
     * the memory requirement of the code that is embedding native
     * client, there is some slop.
     */
    if (0 != getrlimit(RLIMIT_AS, &rlim)) {
      perror("NaClAllocatePow2AlignedMemory::getrlimit");
    } else {
      if (rlim.rlim_cur < mem_sz) {
        /*
         * Developer hint/warning; this will show up in the crash log
         * and must be brief.
         */
        NaClLog(LOG_INFO,
                "Please run \"ulimit -v unlimited\" (bash)"
                " or \"limit vmemoryuse unlimited\" (tcsh)\n");
        NaClLog(LOG_INFO,
                "and restart the app.  NaCl requires at least %"NACL_PRIdS""
                " kilobytes of virtual\n",
                mem_sz / 1024);
        NaClLog(LOG_INFO,
                "address space. NB: Raising the hard limit requires"
                " root access.\n");
      }
    }
#elif NACL_OSX
    /*
     * In OSX, RLIMIT_AS and RLIMIT_RSS have the same value; i.e., OSX
     * conflates the notion of virtual address space used with the
     * resident set size.  In particular, the way NaCl uses virtual
     * address space is to allocate guard pages so that certain
     * addressing modes will not need to be explicitly masked; the
     * guard pages are allocated but inaccessible, never faulted so
     * not even zero-filled on demand, so they should not count
     * against the resident set -- which is supposed to be only the
     * frequently accessed pages in the first place.
     */
#endif

    return LOAD_NO_MEMORY_FOR_ADDRESS_SPACE;
  }
  /*
   * The module lives in the middle FOURGIG of the allocated region --
   * we skip over an initial 40G guard.
   */
  *mem = (void *) (((char *) mem_ptr) + NACL_ADDRSPACE_LOWER_GUARD_SIZE);
  NaClLog(4,
          "NaClAllocateSpace: addr space at 0x%016"NACL_PRIxPTR"\n",
          (uintptr_t) *mem);

  return LOAD_OK;
}
