/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * NaCl Simple/secure ELF loader (NaCl SEL).
 */

#include <errno.h>

#include "native_client/src/include/nacl_platform.h"
#include "native_client/src/include/portability.h"
#include "native_client/src/shared/platform/nacl_log.h"
#include "native_client/src/shared/platform/nacl_sync_checked.h"
#include "native_client/src/trusted/service_runtime/sel_addrspace.h"
#include "native_client/src/trusted/service_runtime/sel_ldr.h"
#include "native_client/src/trusted/service_runtime/sel_memory.h"
#include "native_client/src/trusted/service_runtime/sel_util.h"
#include "native_client/src/trusted/service_runtime/sys_memory.h"

#include "native_client/src/trusted/service_runtime/include/bits/mman.h"

NaClErrorCode NaClAllocAddrSpaceAslr(struct NaClApp *nap,
                                     enum NaClAslrMode aslr_mode) {
  void        *mem;
  int         rv;
  uintptr_t   hole_start;
  size_t      hole_size;
  uintptr_t   stack_start;

  NaClLog(2,
          "NaClAllocAddrSpace: calling NaClAllocateSpace(*,0x%016"
          NACL_PRIxS")\n",
          ((size_t) 1 << nap->addr_bits));

  rv = NaClAllocateSpaceAslr(&mem, (uintptr_t) 1U << nap->addr_bits,
                             aslr_mode);
  if (LOAD_OK != rv) {
    return rv;
  }

  nap->mem_start = (uintptr_t) mem;
  /*
   * The following should not be NaClLog(2, ...) because logging with
   * any detail level higher than LOG_INFO is disabled in the release
   * builds.  This was to reduce logging overhead, so as to eliminate
   * at least a function call as well as possibly a TLS/TSD read if
   * module-specific logging verbosity level comparisons are needed.
   */
  NaClLog(LOG_INFO,
          ("Native Client module will be loaded at"
           " base address 0x%016"NACL_PRIxPTR"\n"),
          nap->mem_start);

  hole_start = NaClRoundAllocPage(nap->data_end);

  if (nap->stack_size >= ((uintptr_t) 1U) << nap->addr_bits) {
    NaClLog(LOG_FATAL, "NaClAllocAddrSpace: stack too large!");
  }
  stack_start = (((uintptr_t) 1U) << nap->addr_bits) - nap->stack_size;
  stack_start = NaClTruncAllocPage(stack_start);

  if (stack_start < hole_start) {
    return LOAD_DATA_OVERLAPS_STACK_SECTION;
  }

  hole_size = stack_start - hole_start;
  hole_size = NaClTruncAllocPage(hole_size);

  /*
   * mprotect and madvise unused data space to "free" it up, but
   * retain mapping so no other memory can be mapped into those
   * addresses.
   */
  if (hole_size == 0) {
    NaClLog(2, ("NaClAllocAddrSpace: hole between end of data and"
                " the beginning of stack is zero size.\n"));
  } else {
    NaClLog(2,
            ("madvising 0x%08"NACL_PRIxPTR", 0x%08"NACL_PRIxS
             ", MADV_DONTNEED\n"),
            nap->mem_start + hole_start, hole_size);
    if (0 != NaClMadvise((void *) (nap->mem_start + hole_start),
                         hole_size,
                         MADV_DONTNEED)) {
      NaClLog(1, "madvise, errno %d\n", errno);
      return LOAD_MADVISE_FAIL;
    }
    NaClLog(2,
            "mprotecting 0x%08"NACL_PRIxPTR", 0x%08"NACL_PRIxS", PROT_NONE\n",
            nap->mem_start + hole_start, hole_size);
    if (0 != NaClMprotect((void *) (nap->mem_start + hole_start),
                          hole_size,
                          PROT_NONE)) {
      NaClLog(1, "mprotect, errno %d\n", errno);
      return LOAD_MPROTECT_FAIL;
    }
  }

  return LOAD_OK;
}

NaClErrorCode NaClAllocAddrSpace(struct NaClApp *nap) {
  return NaClAllocAddrSpaceAslr(nap, 1);
}

/*
 * Apply memory protection to memory regions.
 * Expects that "nap->mu" lock is already held.
 */
NaClErrorCode NaClMemoryProtection(struct NaClApp *nap) {
  uintptr_t start_addr;
  size_t    region_size;
  int       err;

  /*
   * The first NACL_SYSCALL_START_ADDR bytes are mapped as PROT_NONE.
   * This enables NULL pointer checking, and provides additional protection
   * against addr16/data16 prefixed operations being used for attacks.
   */

  NaClLog(3, "Protecting guard pages for 0x%08"NACL_PRIxPTR"\n",
          nap->mem_start);
  /* Add the zero page to the mmap */
  NaClVmmapAdd(&nap->mem_map,
               0,
               NACL_SYSCALL_START_ADDR >> NACL_PAGESHIFT,
               PROT_NONE,
               NACL_ABI_MAP_PRIVATE,
               NULL,
               0,
               0);

  start_addr = nap->mem_start + NACL_SYSCALL_START_ADDR;
  /*
   * The next pages up to NACL_TRAMPOLINE_END are the trampolines.
   * Immediately following that is the loaded text section.
   * These are collectively marked as PROT_READ | PROT_EXEC.
   */
  region_size = NaClRoundPage(nap->static_text_end - NACL_SYSCALL_START_ADDR);
  NaClLog(3,
          ("Trampoline/text region start 0x%08"NACL_PRIxPTR","
           " size 0x%08"NACL_PRIxS", end 0x%08"NACL_PRIxPTR"\n"),
          start_addr, region_size,
          start_addr + region_size);
  if (0 != (err = NaClMprotect((void *) start_addr,
                               region_size,
                               PROT_READ | PROT_EXEC))) {
    NaClLog(LOG_ERROR,
            ("NaClMemoryProtection: "
             "NaClMprotect(0x%08"NACL_PRIxPTR", "
             "0x%08"NACL_PRIxS", 0x%x) failed, "
             "error %d (trampoline + code)\n"),
            start_addr, region_size, PROT_READ | PROT_EXEC,
            err);
    return LOAD_MPROTECT_FAIL;
  }
  NaClVmmapAdd(&nap->mem_map,
               NaClSysToUser(nap, start_addr) >> NACL_PAGESHIFT,
               region_size >> NACL_PAGESHIFT,
               NACL_ABI_PROT_READ | NACL_ABI_PROT_EXEC,
               NACL_ABI_MAP_PRIVATE,
               NULL,
               0,
               0);

  start_addr = NaClUserToSys(nap, nap->dynamic_text_start);
  region_size = nap->dynamic_text_end - nap->dynamic_text_start;
  NaClLog(3,
          ("shm txt region start 0x%08"NACL_PRIxPTR", size 0x%08"NACL_PRIxS","
           " end 0x%08"NACL_PRIxPTR"\n"),
          start_addr, region_size,
          start_addr + region_size);
  if (0 != region_size) {
    /*
     * Page protections for this region have already been set up by
     * nacl_text.c.
     *
     * We record the mapping for consistency with other fixed
     * mappings, but the record is not actually used.  Overmapping is
     * prevented by a separate range check, which is done by
     * NaClSysCommonAddrRangeContainsExecutablePages_mu().
     */
    NaClVmmapAdd(&nap->mem_map,
                 NaClSysToUser(nap, start_addr) >> NACL_PAGESHIFT,
                 region_size >> NACL_PAGESHIFT,
                 NACL_ABI_PROT_READ | NACL_ABI_PROT_EXEC,
                 NACL_ABI_MAP_PRIVATE,
                 nap->text_shm,
                 0,
                 region_size);
  }

  if (0 != nap->rodata_start) {
    uintptr_t rodata_end;
    /*
     * TODO(mseaborn): Could reduce the number of cases by ensuring
     * that nap->data_start is always non-zero, even if
     * nap->rodata_start == nap->data_start == nap->break_addr.
     */
    if (0 != nap->data_start) {
      rodata_end = NaClTruncAllocPage(nap->data_start);
    }
    else {
      rodata_end = nap->break_addr;
    }

    start_addr = NaClUserToSys(nap, nap->rodata_start);
    region_size = NaClRoundPage(NaClRoundAllocPage(rodata_end)
                                - NaClSysToUser(nap, start_addr));
    NaClLog(3,
            ("RO data region start 0x%08"NACL_PRIxPTR", size 0x%08"NACL_PRIxS","
             " end 0x%08"NACL_PRIxPTR"\n"),
            start_addr, region_size,
            start_addr + region_size);
    if (0 != (err = NaClMprotect((void *) start_addr,
                                 region_size,
                                 PROT_READ))) {
      NaClLog(LOG_ERROR,
              ("NaClMemoryProtection: "
               "NaClMprotect(0x%08"NACL_PRIxPTR", "
               "0x%08"NACL_PRIxS", 0x%x) failed, "
               "error %d (rodata)\n"),
              start_addr, region_size, PROT_READ,
              err);
      return LOAD_MPROTECT_FAIL;
    }
    NaClVmmapAdd(&nap->mem_map,
                 NaClSysToUser(nap, start_addr) >> NACL_PAGESHIFT,
                 region_size >> NACL_PAGESHIFT,
                 NACL_ABI_PROT_READ,
                 NACL_ABI_MAP_PRIVATE,
                 NULL,
                 0,
                 0);
  }

  /*
   * data_end is max virtual addr seen, so start_addr <= data_end
   * must hold.
   */

  if (0 != nap->data_start) {
    start_addr = NaClUserToSys(nap, NaClTruncAllocPage(nap->data_start));
    region_size = NaClRoundPage(NaClRoundAllocPage(nap->data_end)
                                - NaClSysToUser(nap, start_addr));
    NaClLog(3,
            ("RW data region start 0x%08"NACL_PRIxPTR", size 0x%08"NACL_PRIxS","
             " end 0x%08"NACL_PRIxPTR"\n"),
            start_addr, region_size,
            start_addr + region_size);
    if (0 != (err = NaClMprotect((void *) start_addr,
                                 region_size,
                                 PROT_READ | PROT_WRITE))) {
      NaClLog(LOG_ERROR,
              ("NaClMemoryProtection: "
               "NaClMprotect(0x%08"NACL_PRIxPTR", "
               "0x%08"NACL_PRIxS", 0x%x) failed, "
               "error %d (data)\n"),
              start_addr, region_size, PROT_READ | PROT_WRITE,
              err);
      return LOAD_MPROTECT_FAIL;
    }
    NaClVmmapAdd(&nap->mem_map,
                 NaClSysToUser(nap, start_addr) >> NACL_PAGESHIFT,
                 region_size >> NACL_PAGESHIFT,
                 NACL_ABI_PROT_READ | NACL_ABI_PROT_WRITE,
                 NACL_ABI_MAP_PRIVATE,
                 NULL,
                 0,
                 0);
  }

  /* stack is read/write but not execute */
  region_size = nap->stack_size;
  /*
   * "start_addr" must not be converted to "sys" address -- it is being passed
   * to NaClSysMmapIntern, which can also be called through user code.
   */
  start_addr = NaClTruncAllocPage(((uintptr_t) 1U << nap->addr_bits)
                                  - nap->stack_size);
  NaClLog(3,
          ("RW stack region start 0x%08"NACL_PRIxPTR", size 0x%08"NACL_PRIxS","
           " end 0x%08"NACL_PRIxPTR"\n"),
          start_addr, region_size,
          start_addr + region_size);
  /*
   * It is necessary to unlock the NaClApp mutex here, as the NaClSysMmapIntern
   * function both claims and releases the lock itself.
   *
   * Additionally, the call to NaClSysMmapIntern requires the flag
   * NACL_ABI_MAP_FIXED -- without it, NaClSysMmapIntern looks for free space
   * between mappings. Since the stack is being initialized here, there is no
   * mapping above (or at) start_addr.
   */
  NaClXMutexUnlock(&nap->mu);
  if ((int32_t) start_addr !=
      (err = NaClSysMmapIntern(nap,
                               (void *) start_addr,
                               region_size,
                               NACL_ABI_PROT_READ | NACL_ABI_PROT_WRITE,
                               NACL_ABI_MAP_PRIVATE | NACL_ABI_MAP_ANONYMOUS |
                               NACL_ABI_MAP_FIXED,
                               -1,
                               0))) {
    NaClXMutexLock(&nap->mu);
    NaClLog(LOG_ERROR,
            ("NaClMemoryProtection: "
             "NaClSysMmapIntern(nap,"
             "0x%08"NACL_PRIxPTR", "
             "0x%08"NACL_PRIxS") failed, "
             "error %d (stack)\n"),
            start_addr, region_size,
            err);
    return LOAD_MPROTECT_FAIL;
  }
  NaClXMutexLock(&nap->mu);
  return LOAD_OK;
}

NaClErrorCode NaClAllocateSpace(void **mem, size_t addrsp_size) {
  return NaClAllocateSpaceAslr(mem, addrsp_size, 1);
}
