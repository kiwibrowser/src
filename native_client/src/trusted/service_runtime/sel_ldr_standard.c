/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * NaCl Simple/secure ELF loader (NaCl SEL).
 */

#include "native_client/src/include/build_config.h"
#include "native_client/src/include/portability.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "native_client/src/include/elf_constants.h"
#include "native_client/src/include/elf.h"
#include "native_client/src/include/nacl_macros.h"
#include "native_client/src/include/win/mman.h"
#include "native_client/src/shared/platform/nacl_check.h"
#include "native_client/src/shared/platform/nacl_log.h"
#include "native_client/src/shared/platform/nacl_sync_checked.h"
#include "native_client/src/shared/platform/nacl_time.h"

#include "native_client/src/trusted/desc/nacl_desc_base.h"
#include "native_client/src/trusted/perf_counter/nacl_perf_counter.h"

#include "native_client/src/trusted/service_runtime/include/sys/errno.h"
#include "native_client/src/trusted/service_runtime/include/sys/fcntl.h"

#include "native_client/src/trusted/service_runtime/arch/sel_ldr_arch.h"
#include "native_client/src/trusted/service_runtime/elf_util.h"
#include "native_client/src/trusted/service_runtime/nacl_app_thread.h"
#include "native_client/src/trusted/service_runtime/nacl_signal.h"
#include "native_client/src/trusted/service_runtime/nacl_switch_to_app.h"
#include "native_client/src/trusted/service_runtime/nacl_syscall_common.h"
#include "native_client/src/trusted/service_runtime/nacl_text.h"
#include "native_client/src/trusted/service_runtime/sel_memory.h"
#include "native_client/src/trusted/service_runtime/sel_ldr.h"
#include "native_client/src/trusted/service_runtime/sel_util.h"
#include "native_client/src/trusted/service_runtime/sel_addrspace.h"


/*
 * Fill from static_text_end to end of that page with halt
 * instruction, which is at least NACL_HALT_LEN in size when no
 * dynamic text is present.  Does not touch dynamic text region, which
 * should be pre-filled with HLTs.
 *
 * By adding NACL_HALT_SLED_SIZE, we ensure that the code region ends
 * with HLTs, just in case the CPU has a bug in which it fails to
 * check for running off the end of the x86 code segment.
 */
void NaClFillEndOfTextRegion(struct NaClApp *nap) {
  size_t page_pad;

  /*
   * NOTE: make sure we are not silently overwriting data.  It is the
   * toolchain's responsibility to ensure that a NACL_HALT_SLED_SIZE
   * gap exists.
   */
  if (0 != nap->data_start &&
      nap->static_text_end + NACL_HALT_SLED_SIZE >
      NaClTruncAllocPage(nap->data_start)) {
    NaClLog(LOG_FATAL, "Missing gap between text and data for halt_sled\n");
  }
  if (0 != nap->rodata_start &&
      nap->static_text_end + NACL_HALT_SLED_SIZE > nap->rodata_start) {
    NaClLog(LOG_FATAL, "Missing gap between text and rodata for halt_sled\n");
  }

  if (NULL == nap->text_shm) {
    /*
     * No dynamic text exists.  Space for NACL_HALT_SLED_SIZE must
     * exist.
     */
    page_pad = (NaClRoundAllocPage(nap->static_text_end + NACL_HALT_SLED_SIZE)
                - nap->static_text_end);
    CHECK(page_pad >= NACL_HALT_SLED_SIZE);
    CHECK(page_pad < NACL_MAP_PAGESIZE + NACL_HALT_SLED_SIZE);
  } else {
    /*
     * Dynamic text exists; the halt sled resides in the dynamic text
     * region, so all we need to do here is to round out the last
     * static text page with HLT instructions.  It doesn't matter if
     * the size of this region is smaller than NACL_HALT_SLED_SIZE --
     * this is just to fully initialize the page, rather than (later)
     * decoding/validating zero-filled memory as instructions.
     */
    page_pad = NaClRoundAllocPage(nap->static_text_end) - nap->static_text_end;
  }

  NaClLog(4,
          "Filling with halts: %08"NACL_PRIxPTR", %08"NACL_PRIxS" bytes\n",
          nap->mem_start + nap->static_text_end,
          page_pad);

  NaClFillMemoryRegionWithHalt((void *)(nap->mem_start + nap->static_text_end),
                               page_pad);

  nap->static_text_end += page_pad;
}

/*
 * Basic address space layout sanity check.
 */
NaClErrorCode NaClCheckAddressSpaceLayoutSanity(struct NaClApp *nap,
                                                uintptr_t rodata_end,
                                                uintptr_t data_end,
                                                uintptr_t max_vaddr) {
  if (0 != nap->data_start) {
    if (data_end != max_vaddr) {
      NaClLog(LOG_INFO, "data segment is not last\n");
      return LOAD_DATA_NOT_LAST_SEGMENT;
    }
  } else if (0 != nap->rodata_start) {
    if (NaClRoundAllocPage(rodata_end) != max_vaddr) {
      /*
       * This should be unreachable, but we include it just for
       * completeness.
       *
       * Here is why it is unreachable:
       *
       * NaClPhdrChecks checks the test segment starting address.  The
       * only allowed loaded segments are text, data, and rodata.
       * Thus unless the rodata is in the trampoline region, it must
       * be after the text.  And NaClElfImageValidateProgramHeaders
       * ensures that all segments start after the trampoline region.
       */
      NaClLog(LOG_INFO, "no data segment, but rodata segment is not last\n");
      return LOAD_NO_DATA_BUT_RODATA_NOT_LAST_SEGMENT;
    }
  }
  if (0 != nap->rodata_start && 0 != nap->data_start) {
    if (rodata_end > NaClTruncAllocPage(nap->data_start)) {
      NaClLog(LOG_INFO, "rodata_overlaps data.\n");
      return LOAD_RODATA_OVERLAPS_DATA;
    }
  }
  if (0 != nap->rodata_start) {
    if (NaClRoundAllocPage(NaClEndOfStaticText(nap)) > nap->rodata_start) {
      return LOAD_TEXT_OVERLAPS_RODATA;
    }
  } else if (0 != nap->data_start) {
    if (NaClRoundAllocPage(NaClEndOfStaticText(nap)) >
        NaClTruncAllocPage(nap->data_start)) {
      return LOAD_TEXT_OVERLAPS_DATA;
    }
  }
  if (0 != nap->rodata_start &&
      NaClRoundAllocPage(nap->rodata_start) != nap->rodata_start) {
    NaClLog(LOG_INFO, "rodata_start not a multiple of allocation size\n");
    return LOAD_BAD_RODATA_ALIGNMENT;
  }
#if NACL_ARCH(NACL_BUILD_ARCH) == NACL_mips
  /*
   * This check is necessary to make MIPS sandbox secure, as there is no NX page
   * protection support on MIPS.
   */
  if (nap->rodata_start < NACL_DATA_SEGMENT_START) {
    NaClLog(LOG_INFO,
            "rodata_start is below NACL_DATA_SEGMENT_START (0x%X) address\n",
            NACL_DATA_SEGMENT_START);
    return LOAD_SEGMENT_BAD_LOC;
  }
#endif
  return LOAD_OK;
}

void NaClLogAddressSpaceLayout(struct NaClApp *nap) {
  NaClLog(2, "NaClApp addr space layout:\n");
  NaClLog(2, "nap->static_text_end    = 0x%016"NACL_PRIxPTR"\n",
          nap->static_text_end);
  NaClLog(2, "nap->dynamic_text_start = 0x%016"NACL_PRIxPTR"\n",
          nap->dynamic_text_start);
  NaClLog(2, "nap->dynamic_text_end   = 0x%016"NACL_PRIxPTR"\n",
          nap->dynamic_text_end);
  NaClLog(2, "nap->rodata_start       = 0x%016"NACL_PRIxPTR"\n",
          nap->rodata_start);
  NaClLog(2, "nap->data_start         = 0x%016"NACL_PRIxPTR"\n",
          nap->data_start);
  NaClLog(2, "nap->data_end           = 0x%016"NACL_PRIxPTR"\n",
          nap->data_end);
  NaClLog(2, "nap->break_addr         = 0x%016"NACL_PRIxPTR"\n",
          nap->break_addr);
  NaClLog(2, "nap->initial_entry_pt   = 0x%016"NACL_PRIxPTR"\n",
          nap->initial_entry_pt);
  NaClLog(2, "nap->user_entry_pt      = 0x%016"NACL_PRIxPTR"\n",
          nap->user_entry_pt);
  NaClLog(2, "nap->bundle_size        = 0x%x\n", nap->bundle_size);
}

/*
 * Expects that "nap->mu" lock is already held.
 */
NaClErrorCode NaClAppLoadFileAslr(struct NaClDesc *ndp,
                                  struct NaClApp *nap,
                                  enum NaClAslrMode aslr_mode) {
  NaClErrorCode       ret = LOAD_INTERNAL;
  NaClErrorCode       subret = LOAD_INTERNAL;
  uintptr_t           rodata_end;
  uintptr_t           data_end;
  uintptr_t           max_vaddr;
  struct NaClElfImage *image = NULL;
  struct NaClPerfCounter  time_load_file;
  struct NaClElfImageInfo info;

  NaClPerfCounterCtor(&time_load_file, "NaClAppLoadFile");

  /* NACL_MAX_ADDR_BITS < 32 */
  if (nap->addr_bits > NACL_MAX_ADDR_BITS) {
    ret = LOAD_ADDR_SPACE_TOO_BIG;
    goto done;
  }

  nap->stack_size = NaClRoundAllocPage(nap->stack_size);

  /* temporay object will be deleted at end of function */
  image = NaClElfImageNew(ndp, &subret);
  if (NULL == image || LOAD_OK != subret) {
    ret = subret;
    goto done;
  }

  subret = NaClElfImageValidateProgramHeaders(image,
                                              nap->addr_bits,
                                              &info);
  if (LOAD_OK != subret) {
    ret = subret;
    goto done;
  }

  if (nap->initial_nexe_max_code_bytes != 0) {
    size_t code_segment_size = info.static_text_end - NACL_TRAMPOLINE_END;
    if (code_segment_size > nap->initial_nexe_max_code_bytes) {
      NaClLog(LOG_ERROR, "NaClAppLoadFileAslr: "
              "Code segment size (%"NACL_PRIuS" bytes) exceeds limit (%"
              NACL_PRId32" bytes)\n",
              code_segment_size, nap->initial_nexe_max_code_bytes);
      ret = LOAD_CODE_SEGMENT_TOO_LARGE;
      goto done;
    }
  }

  nap->static_text_end = info.static_text_end;
  nap->rodata_start = info.rodata_start;
  rodata_end = info.rodata_end;
  nap->data_start = info.data_start;
  data_end = info.data_end;
  max_vaddr = info.max_vaddr;

  if (0 == nap->data_start) {
    if (0 == nap->rodata_start) {
      if (NaClRoundAllocPage(max_vaddr) - max_vaddr < NACL_HALT_SLED_SIZE) {
        /*
         * if no rodata and no data, we make sure that there is space for
         * the halt sled.
         */
        max_vaddr += NACL_MAP_PAGESIZE;
      }
    } else {
      /*
       * no data, but there is rodata.  this means max_vaddr is just
       * where rodata ends.  this might not be at an allocation
       * boundary, and in this the page would not be writable.  round
       * max_vaddr up to the next allocation boundary so that bss will
       * be at the next writable region.
       */
      ;
    }
    max_vaddr = NaClRoundAllocPage(max_vaddr);
  }
  /*
   * max_vaddr -- the break or the boundary between data (initialized
   * and bss) and the address space hole -- does not have to be at a
   * page boundary.
   *
   * Memory allocation will use NaClRoundPage(nap->break_addr), but
   * the system notion of break is always an exact address.  Even
   * though we must allocate and make accessible multiples of pages,
   * the linux-style brk system call (which returns current break on
   * failure) permits a non-aligned address as argument.
   */
  nap->break_addr = max_vaddr;
  nap->data_end = max_vaddr;

  NaClLog(4, "Values from NaClElfImageValidateProgramHeaders:\n");
  NaClLog(4, "rodata_start = 0x%08"NACL_PRIxPTR"\n", nap->rodata_start);
  NaClLog(4, "rodata_end   = 0x%08"NACL_PRIxPTR"\n", rodata_end);
  NaClLog(4, "data_start   = 0x%08"NACL_PRIxPTR"\n", nap->data_start);
  NaClLog(4, "data_end     = 0x%08"NACL_PRIxPTR"\n", data_end);
  NaClLog(4, "max_vaddr    = 0x%08"NACL_PRIxPTR"\n", max_vaddr);

  /* We now support only one bundle size.  */
  nap->bundle_size = NACL_INSTR_BLOCK_SIZE;

  nap->initial_entry_pt = NaClElfImageGetEntryPoint(image);
  NaClLogAddressSpaceLayout(nap);

  if (!NaClAddrIsValidEntryPt(nap, nap->initial_entry_pt)) {
    ret = LOAD_BAD_ENTRY;
    goto done;
  }

  subret = NaClCheckAddressSpaceLayoutSanity(nap, rodata_end, data_end,
                                             max_vaddr);
  if (LOAD_OK != subret) {
    ret = subret;
    goto done;
  }

  NaClLog(2, "Allocating address space\n");
  NaClPerfCounterMark(&time_load_file, "PreAllocAddrSpace");
  NaClPerfCounterIntervalLast(&time_load_file);
  subret = NaClAllocAddrSpaceAslr(nap, aslr_mode);
  NaClPerfCounterMark(&time_load_file,
                      NACL_PERF_IMPORTANT_PREFIX "AllocAddrSpace");
  NaClPerfCounterIntervalLast(&time_load_file);
  if (LOAD_OK != subret) {
    ret = subret;
    goto done;
  }

  /*
   * Make sure the static image pages are marked writable before we try
   * to write them.
   */
  NaClLog(2, "Loading into memory\n");
  ret = NaClMprotect((void *) (nap->mem_start + NACL_TRAMPOLINE_START),
                     NaClRoundAllocPage(nap->data_end) - NACL_TRAMPOLINE_START,
                     PROT_READ | PROT_WRITE);
  if (0 != ret) {
    NaClLog(LOG_FATAL,
            "NaClAppLoadFile: Failed to make image pages writable. "
            "Error code 0x%x\n",
            ret);
  }
  subret = NaClElfImageLoad(image, ndp, nap);
  NaClPerfCounterMark(&time_load_file,
                      NACL_PERF_IMPORTANT_PREFIX "NaClElfImageLoad");
  NaClPerfCounterIntervalLast(&time_load_file);
  if (LOAD_OK != subret) {
    ret = subret;
    goto done;
  }

  /*
   * NB: mem_map object has been initialized, but is empty.
   * NaClMakeDynamicTextShared does not touch it.
   *
   * NaClMakeDynamicTextShared also fills the dynamic memory region
   * with the architecture-specific halt instruction.  If/when we use
   * memory mapping to save paging space for the dynamic region and
   * lazily halt fill the memory as the pages become
   * readable/executable, we must make sure that the *last*
   * NACL_MAP_PAGESIZE chunk is nonetheless mapped and written with
   * halts.
   */
  NaClLog(2,
          ("Replacing gap between static text and"
           " (ro)data with shareable memory\n"));
  subret = NaClMakeDynamicTextShared(nap);
  NaClPerfCounterMark(&time_load_file,
                      NACL_PERF_IMPORTANT_PREFIX "MakeDynText");
  NaClPerfCounterIntervalLast(&time_load_file);
  if (LOAD_OK != subret) {
    ret = subret;
    goto done;
  }

  /*
   * NaClFillEndOfTextRegion will fill with halt instructions the
   * padding space after the static text region.
   *
   * Shm-backed dynamic text space was filled with halt instructions
   * in NaClMakeDynamicTextShared.  This extends to the rodata.  For
   * non-shm-backed text space, this extend to the next page (and not
   * allocation page).  static_text_end is updated to include the
   * padding.
   */
  NaClFillEndOfTextRegion(nap);

  if (nap->main_exe_prevalidated) {
    NaClLog(2, "Main executable segment hit validation cache and mapped in,"
            " skipping validation.\n");
    subret = LOAD_OK;
  } else {
    NaClLog(2, "Validating image\n");
    subret = NaClValidateImage(nap);
  }
  NaClPerfCounterMark(&time_load_file,
                      NACL_PERF_IMPORTANT_PREFIX "ValidateImg");
  NaClPerfCounterIntervalLast(&time_load_file);
  if (LOAD_OK != subret) {
    ret = subret;
    goto done;
  }

  NaClLog(2, "Initializing arch switcher\n");
  NaClInitSwitchToApp(nap);

  NaClLog(2, "Installing trampoline\n");
  NaClLoadTrampoline(nap, aslr_mode);

  NaClLog(2, "Installing springboard\n");
  NaClLoadSpringboard(nap);

  /*
   * NaClMemoryProtection also initializes the mem_map w/ information
   * about the memory pages and their current protection value.
   *
   * The contents of the dynamic text region will get remapped as
   * non-writable.
   */
  NaClLog(2, "Applying memory protection\n");
  subret = NaClMemoryProtection(nap);
  if (LOAD_OK != subret) {
    ret = subret;
    goto done;
  }

  NaClLog(2, "NaClAppLoadFile done; ");
  NaClLogAddressSpaceLayout(nap);
  ret = LOAD_OK;
done:
  NaClElfImageDelete(image);

  NaClPerfCounterMark(&time_load_file, "EndLoadFile");
  NaClPerfCounterIntervalTotal(&time_load_file);
  return ret;
}

NaClErrorCode NaClAppLoadFile(struct NaClDesc *ndp,
                              struct NaClApp *nap) {
  return NaClAppLoadFileAslr(ndp, nap, NACL_ENABLE_ASLR);
}

NaClErrorCode NaClAppLoadFileDynamically(
    struct NaClApp *nap,
    struct NaClDesc *ndp,
    struct NaClValidationMetadata *metadata) {
  struct NaClElfImage *image = NULL;
  NaClErrorCode ret = LOAD_INTERNAL;

  image = NaClElfImageNew(ndp, &ret);
  if (NULL == image || LOAD_OK != ret) {
    goto done;
  }
  ret = NaClElfImageLoadDynamically(image, nap, ndp, metadata);
  if (LOAD_OK != ret) {
    goto done;
  }
  nap->user_entry_pt = nap->initial_entry_pt;
  nap->initial_entry_pt = NaClElfImageGetEntryPoint(image);
  if (!NaClAddrIsValidIrtEntryPt(nap, nap->initial_entry_pt)) {
    ret = LOAD_BAD_ENTRY;
    goto done;
  }

 done:
  NaClElfImageDelete(image);
  return ret;
}

int NaClAddrIsValidEntryPt(struct NaClApp *nap,
                           uintptr_t      addr) {
  if (0 != (addr & (nap->bundle_size - 1))) {
    return 0;
  }

  return addr < nap->static_text_end;
}

int NaClAddrIsValidIrtEntryPt(struct NaClApp *nap,
                              uintptr_t      addr) {
  if (0 != (addr & (nap->bundle_size - 1))) {
    return 0;
  }

  return addr < nap->dynamic_text_end;
}

int NaClReportExitStatus(struct NaClApp *nap, int exit_status) {
  NaClXMutexLock(&nap->mu);
  /*
   * If several threads are exiting/reporting signals at once, we should
   * let only one thread to pass through. This way we can use exit code
   * without synchronization once we know that running==0.
   */
  if (!nap->running) {
    NaClXMutexUnlock(&nap->mu);
    return 0;
  }

  nap->exit_status = exit_status;
  nap->running = 0;
  NaClXCondVarSignal(&nap->cv);

  NaClXMutexUnlock(&nap->mu);

  return 0;
}

uintptr_t NaClGetInitialStackTop(struct NaClApp *nap) {
  /*
   * We keep the top of useful memory a page below the top of the
   * sandbox region so that compilers can do tricks like computing a
   * base register of sp + constant and then using a
   * register-minus-constant addressing mode, which comes up at least
   * on ARM where the compiler is trying to optimize given the limited
   * size of immediate offsets available.  The maximum such negative
   * constant on ARM will be -4095, but we use page size (64k) for
   * good measure and do it on all machines just for uniformity.
   */
  return ((uintptr_t) 1U << nap->addr_bits) - NACL_MAP_PAGESIZE;
}

/*
 * preconditions:
 *  * argc is the length of the argv array
 *  * envv may be NULL (this happens on MacOS/Cocoa and in tests)
 *  * if envv is non-NULL it is 'consistent', null terminated etc.
 */
int NaClCreateMainThread(struct NaClApp     *nap,
                         int                argc,
                         char               **argv,
                         char const *const  *envv) {
  /*
   * Compute size of string tables for argv and envv
   */
  int                   retval;
  int                   envc;
  size_t                size;
  int                   auxv_entries;
  size_t                ptr_tbl_size;
  int                   i;
  uint32_t              *p;
  char                  *strp;
  size_t                *argv_len;
  size_t                *envv_len;
  uintptr_t             stack_ptr;

  retval = 0;  /* fail */
  CHECK(argc >= 0);
  CHECK(NULL != argv || 0 == argc);

  envc = 0;
  if (NULL != envv) {
    char const *const *pp;
    for (pp = envv; NULL != *pp; ++pp) {
      ++envc;
    }
  }
  envv_len = 0;
  argv_len = malloc(argc * sizeof argv_len[0]);
  envv_len = malloc(envc * sizeof envv_len[0]);
  if (NULL == argv_len) {
    goto cleanup;
  }
  if (NULL == envv_len && 0 != envc) {
    goto cleanup;
  }

  size = 0;

  /*
   * The following two loops cannot overflow.  The reason for this is
   * that they are counting the number of bytes used to hold the
   * NUL-terminated strings that comprise the argv and envv tables.
   * If the entire address space consisted of just those strings, then
   * the size variable would overflow; however, since there's the code
   * space required to hold the code below (and we are not targetting
   * Harvard architecture machines), at least one page holds code, not
   * data.  We are assuming that the caller is non-adversarial and the
   * code does not look like string data....
   */
  for (i = 0; i < argc; ++i) {
    argv_len[i] = strlen(argv[i]) + 1;
    size += argv_len[i];
  }
  for (i = 0; i < envc; ++i) {
    envv_len[i] = strlen(envv[i]) + 1;
    size += envv_len[i];
  }

  /*
   * NaCl modules are ILP32, so the argv, envv pointers, as well as
   * the terminating NULL pointers at the end of the argv/envv tables,
   * are 32-bit values.  We also have the auxv to take into account.
   *
   * The argv and envv pointer tables came from trusted code and is
   * part of memory.  Thus, by the same argument above, adding in
   * "ptr_tbl_size" cannot possibly overflow the "size" variable since
   * it is a size_t object.  However, the extra pointers for auxv and
   * the space for argv could cause an overflow.  The fact that we
   * used stack to get here etc means that ptr_tbl_size could not have
   * overflowed.
   *
   * NB: the underlying OS would have limited the amount of space used
   * for argv and envv -- on linux, it is ARG_MAX, or 128KB -- and
   * hence the overflow check is for obvious auditability rather than
   * for correctness.
   */
  auxv_entries = 1;
  if (0 != nap->user_entry_pt) {
    auxv_entries++;
  }
  if (0 != nap->dynamic_text_start) {
    auxv_entries++;
  }
  ptr_tbl_size = 3 + argc + 1 + envc + 1 + auxv_entries * 2;
#if NACL_STACK_GETS_ARG
  ptr_tbl_size++;
#endif
  ptr_tbl_size *= sizeof(uint32_t);

  if (SIZE_T_MAX - size < ptr_tbl_size) {
    NaClLog(LOG_WARNING,
            "NaClCreateMainThread: ptr_tbl_size cause size of"
            " argv / environment copy to overflow!?!\n");
    retval = 0;
    goto cleanup;
  }
  size += ptr_tbl_size;

  size = (size + NACL_STACK_ALIGN_MASK) & ~NACL_STACK_ALIGN_MASK;

  if (size > nap->stack_size) {
    retval = 0;
    goto cleanup;
  }

  /*
   * Write strings and char * arrays to stack.
   */
  stack_ptr = NaClUserToSysAddrRange(nap, NaClGetInitialStackTop(nap) - size,
                                     size);
  if (stack_ptr == kNaClBadAddress) {
    retval = 0;
    goto cleanup;
  }

  NaClLog(2, "setting stack to : %016"NACL_PRIxPTR"\n", stack_ptr);

  VCHECK(0 == (stack_ptr & NACL_STACK_ALIGN_MASK),
         ("stack_ptr not aligned: %016"NACL_PRIxPTR"\n", stack_ptr));

  p = (uint32_t *) stack_ptr;
  strp = (char *) stack_ptr + ptr_tbl_size;

  /*
   * For x86-32, we push an initial argument that is the address of
   * the main argument block.  For other machines, this is passed
   * in a register and that's set in NaClStartThreadInApp.
   */
#if NACL_STACK_GETS_ARG
  {
    uint32_t *argloc = p++;  /* Prevent unsequenced access to p in next line. */
    *argloc = (uint32_t) NaClSysToUser(nap, (uintptr_t) p);
  }
#endif

  *p++ = 0;  /* Cleanup function pointer, always NULL.  */
  *p++ = envc;
  *p++ = argc;

  for (i = 0; i < argc; ++i) {
    *p++ = (uint32_t) NaClSysToUser(nap, (uintptr_t) strp);
    NaClLog(2, "copying arg %d  %p -> %p\n",
            i, (void *) argv[i], (void *) strp);
    strcpy(strp, argv[i]);
    strp += argv_len[i];
  }
  *p++ = 0;  /* argv[argc] is NULL.  */

  for (i = 0; i < envc; ++i) {
    *p++ = (uint32_t) NaClSysToUser(nap, (uintptr_t) strp);
    NaClLog(2, "copying env %d  %p -> %p\n",
            i, (void *) envv[i], (void *) strp);
    strcpy(strp, envv[i]);
    strp += envv_len[i];
  }
  *p++ = 0;  /* envp[envc] is NULL.  */

  /* Push an auxv */
  if (0 != nap->user_entry_pt) {
    *p++ = AT_ENTRY;
    *p++ = (uint32_t) nap->user_entry_pt;
  }
  if (0 != nap->dynamic_text_start) {
    *p++ = AT_BASE;
    *p++ = (uint32_t) nap->dynamic_text_start;
  }
  *p++ = AT_NULL;
  *p++ = 0;

  CHECK((char *) p == (char *) stack_ptr + ptr_tbl_size);

  /* now actually spawn the thread */
  NaClXMutexLock(&nap->mu);
  /*
   * Unreference the main nexe and irt at this point if no debug stub callbacks
   * have been registered, as these references to the main nexe and irt
   * descriptors are only used when providing file access to the debugger.
   * In the debug case, let shutdown take care of cleanup.
   */
  if (NULL == nap->debug_stub_callbacks) {
    if (NULL != nap->main_nexe_desc) {
      NaClDescUnref(nap->main_nexe_desc);
      nap->main_nexe_desc = NULL;
    }
    if (NULL != nap->irt_nexe_desc) {
      NaClDescUnref(nap->irt_nexe_desc);
      nap->irt_nexe_desc = NULL;
    }
  }
  nap->running = 1;
  NaClXMutexUnlock(&nap->mu);

  NaClVmHoleWaitToStartThread(nap);

  /*
   * For x86, we adjust the stack pointer down to push a dummy return
   * address.  This happens after the stack pointer alignment.
   * We avoid the otherwise harmless call for the zero case because
   * _FORTIFY_SOURCE memset can warn about zero-length calls.
   */
  if (NACL_STACK_PAD_BELOW_ALIGN != 0) {
    stack_ptr -= NACL_STACK_PAD_BELOW_ALIGN;
    memset((void *) stack_ptr, 0, NACL_STACK_PAD_BELOW_ALIGN);
  }

  NaClLog(2, "system stack ptr : %016"NACL_PRIxPTR"\n", stack_ptr);
  NaClLog(2, "  user stack ptr : %016"NACL_PRIxPTR"\n",
          NaClSysToUserStackAddr(nap, stack_ptr));

  /* e_entry is user addr */
  retval = NaClAppThreadSpawn(nap,
                              nap->initial_entry_pt,
                              NaClSysToUserStackAddr(nap, stack_ptr),
                              /* user_tls1= */ (uint32_t) nap->break_addr,
                              /* user_tls2= */ 0);

cleanup:
  free(argv_len);
  free(envv_len);

  return retval;
}

int NaClWaitForMainThreadToExit(struct NaClApp  *nap) {
  NaClLog(3, "NaClWaitForMainThreadToExit: taking NaClApp lock\n");
  NaClXMutexLock(&nap->mu);
  NaClLog(3, " waiting for exit status\n");
  while (nap->running) {
    NaClXCondVarWait(&nap->cv, &nap->mu);
    NaClLog(3, " wakeup, nap->running %d, nap->exit_status %d\n",
            nap->running, nap->exit_status);
  }
  NaClXMutexUnlock(&nap->mu);
  /*
   * Some thread invoked the exit (exit_group) syscall.
   */

  if (NULL != nap->debug_stub_callbacks) {
    nap->debug_stub_callbacks->process_exit_hook();
  }

  return NACL_ABI_WEXITSTATUS(nap->exit_status);
}

/*
 * stack_ptr is from syscall, so a 32-bit address.
 */
int32_t NaClCreateAdditionalThread(struct NaClApp *nap,
                                   uintptr_t      prog_ctr,
                                   uintptr_t      sys_stack_ptr,
                                   uint32_t       user_tls1,
                                   uint32_t       user_tls2) {
  if (!NaClAppThreadSpawn(nap,
                          prog_ctr,
                          NaClSysToUserStackAddr(nap, sys_stack_ptr),
                          user_tls1,
                          user_tls2)) {
    NaClLog(LOG_WARNING,
            ("NaClCreateAdditionalThread: could not allocate thread."
             "  Returning EAGAIN per POSIX specs.\n"));
    return -NACL_ABI_EAGAIN;
  }
  return 0;
}
