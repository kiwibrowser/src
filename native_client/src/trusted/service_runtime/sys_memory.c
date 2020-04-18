/*
 * Copyright (c) 2013 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "native_client/src/trusted/service_runtime/sys_memory.h"

#include <errno.h>
#include <string.h>

#include "native_client/src/include/build_config.h"
#include "native_client/src/include/nacl_platform.h"
#include "native_client/src/shared/platform/nacl_check.h"
#include "native_client/src/shared/platform/nacl_log.h"
#include "native_client/src/shared/platform/nacl_sync_checked.h"
#include "native_client/src/trusted/desc/nacl_desc_effector_trusted_mem.h"
#include "native_client/src/trusted/desc/nacl_desc_io.h"
#include "native_client/src/trusted/fault_injection/fault_injection.h"
#include "native_client/src/trusted/service_runtime/include/bits/mman.h"
#include "native_client/src/trusted/service_runtime/include/sys/errno.h"
#include "native_client/src/trusted/service_runtime/include/sys/fcntl.h"
#include "native_client/src/trusted/service_runtime/include/sys/stat.h"
#include "native_client/src/trusted/service_runtime/internal_errno.h"
#include "native_client/src/trusted/service_runtime/nacl_app_thread.h"
#include "native_client/src/trusted/service_runtime/nacl_copy.h"
#include "native_client/src/trusted/service_runtime/nacl_syscall_common.h"
#include "native_client/src/trusted/service_runtime/nacl_text.h"
#include "native_client/src/trusted/service_runtime/sel_ldr.h"
#include "native_client/src/trusted/service_runtime/sel_memory.h"
#include "native_client/src/trusted/validator/validation_metadata.h"

#if NACL_WINDOWS
# include "native_client/src/shared/platform/win/xlate_system_error.h"
#endif


static int32_t MunmapInternal(struct NaClApp *nap,
                              uintptr_t sysaddr, size_t length);

static const size_t kMaxUsableFileSize = (SIZE_T_MAX >> 1);


static INLINE size_t  size_min(size_t a, size_t b) {
  return (a < b) ? a : b;
}

int32_t NaClSysBrk(struct NaClAppThread *natp,
                   uintptr_t            new_break) {
  struct NaClApp        *nap = natp->nap;
  uintptr_t             break_addr;
  int32_t               rv = -NACL_ABI_EINVAL;
  struct NaClVmmapIter  iter;
  struct NaClVmmapEntry *ent;
  struct NaClVmmapEntry *next_ent;
  uintptr_t             sys_break;
  uintptr_t             sys_new_break;
  uintptr_t             usr_last_data_page;
  uintptr_t             usr_new_last_data_page;
  uintptr_t             last_internal_data_addr;
  uintptr_t             last_internal_page;
  uintptr_t             start_new_region;
  uintptr_t             region_size;

  /*
   * The sysbrk() IRT interface is deprecated and is not enabled for
   * ABI-stable PNaCl pexes, so for security hardening, disable the
   * syscall under PNaCl too.
   */
  if (nap->pnacl_mode)
    return -NACL_ABI_ENOSYS;

  break_addr = nap->break_addr;

  NaClLog(3, "Entered NaClSysBrk(new_break 0x%08"NACL_PRIxPTR")\n",
          new_break);

  sys_new_break = NaClUserToSysAddr(nap, new_break);
  NaClLog(3, "sys_new_break 0x%08"NACL_PRIxPTR"\n", sys_new_break);

  if (kNaClBadAddress == sys_new_break) {
    goto cleanup_no_lock;
  }
  if (NACL_SYNC_OK != NaClMutexLock(&nap->mu)) {
    NaClLog(LOG_ERROR, "Could not get app lock for 0x%08"NACL_PRIxPTR"\n",
            (uintptr_t) nap);
    goto cleanup_no_lock;
  }
  if (new_break < nap->data_end) {
    NaClLog(4, "new_break before data_end (0x%"NACL_PRIxPTR")\n",
            nap->data_end);
    goto cleanup;
  }
  if (new_break <= nap->break_addr) {
    /* freeing memory */
    NaClLog(4, "new_break before break (0x%"NACL_PRIxPTR"); freeing\n",
            nap->break_addr);
    nap->break_addr = new_break;
    break_addr = new_break;
  } else {
    /*
     * See if page containing new_break is in mem_map; if so, we are
     * essentially done -- just update break_addr.  Otherwise, we
     * extend the VM map entry from the page containing the current
     * break to the page containing new_break.
     */

    sys_break = NaClUserToSys(nap, nap->break_addr);

    usr_last_data_page = (nap->break_addr - 1) >> NACL_PAGESHIFT;

    usr_new_last_data_page = (new_break - 1) >> NACL_PAGESHIFT;

    last_internal_data_addr = NaClRoundAllocPage(new_break) - 1;
    last_internal_page = last_internal_data_addr >> NACL_PAGESHIFT;

    NaClLog(4, ("current break sys addr 0x%08"NACL_PRIxPTR", "
                "usr last data page 0x%"NACL_PRIxPTR"\n"),
            sys_break, usr_last_data_page);
    NaClLog(4, "new break usr last data page 0x%"NACL_PRIxPTR"\n",
            usr_new_last_data_page);
    NaClLog(4, "last internal data addr 0x%08"NACL_PRIxPTR"\n",
            last_internal_data_addr);

    if (NULL == NaClVmmapFindPageIter(&nap->mem_map,
                                      usr_last_data_page,
                                      &iter)
        || NaClVmmapIterAtEnd(&iter)) {
      NaClLog(LOG_FATAL, ("current break (0x%08"NACL_PRIxPTR", "
                          "sys 0x%08"NACL_PRIxPTR") "
                          "not in address map\n"),
              nap->break_addr, sys_break);
    }
    ent = NaClVmmapIterStar(&iter);
    NaClLog(4, ("segment containing current break"
                ": page_num 0x%08"NACL_PRIxPTR", npages 0x%"NACL_PRIxS"\n"),
            ent->page_num, ent->npages);
    if (usr_new_last_data_page < ent->page_num + ent->npages) {
      NaClLog(4, "new break within break segment, just bumping addr\n");
      nap->break_addr = new_break;
      break_addr = new_break;
    } else {
      NaClVmmapIterIncr(&iter);
      if (!NaClVmmapIterAtEnd(&iter)
          && ((next_ent = NaClVmmapIterStar(&iter))->page_num
              <= last_internal_page)) {
        /* ran into next segment! */
        NaClLog(4,
                ("new break request of usr address "
                 "0x%08"NACL_PRIxPTR" / usr page 0x%"NACL_PRIxPTR
                 " runs into next region, page_num 0x%"NACL_PRIxPTR", "
                 "npages 0x%"NACL_PRIxS"\n"),
                new_break, usr_new_last_data_page,
                next_ent->page_num, next_ent->npages);
        goto cleanup;
      }
      NaClLog(4,
              "extending segment: page_num 0x%08"NACL_PRIxPTR", "
              "npages 0x%"NACL_PRIxS"\n",
              ent->page_num, ent->npages);
      /* go ahead and extend ent to cover, and make pages accessible */
      start_new_region = (ent->page_num + ent->npages) << NACL_PAGESHIFT;
      ent->npages = (last_internal_page - ent->page_num + 1);
      region_size = (((last_internal_page + 1) << NACL_PAGESHIFT)
                     - start_new_region);
      if (0 != NaClMprotect((void *) NaClUserToSys(nap, start_new_region),
                            region_size,
                            PROT_READ | PROT_WRITE)) {
        NaClLog(LOG_FATAL,
                ("Could not mprotect(0x%08"NACL_PRIxPTR", "
                 "0x%08"NACL_PRIxPTR", "
                 "PROT_READ|PROT_WRITE)\n"),
                start_new_region,
                region_size);
      }
      NaClLog(4, "segment now: page_num 0x%08"NACL_PRIxPTR", "
              "npages 0x%"NACL_PRIxS"\n",
              ent->page_num, ent->npages);
      nap->break_addr = new_break;
      break_addr = new_break;
    }
    /*
     * Zero out memory between old break and new break.
     */
    CHECK(sys_new_break > sys_break);
    memset((void *) sys_break, 0, sys_new_break - sys_break);
  }

cleanup:
  NaClXMutexUnlock(&nap->mu);
cleanup_no_lock:

  /*
   * This cast is safe because the incoming value (new_break) cannot
   * exceed the user address space--even though its type (uintptr_t)
   * theoretically allows larger values.
   */
  rv = (int32_t) break_addr;

  NaClLog(3, "NaClSysBrk: returning 0x%08"NACL_PRIx32"\n", rv);
  return rv;
}

int NaClSysCommonAddrRangeContainsExecutablePages(struct NaClApp *nap,
                                                  uintptr_t usraddr,
                                                  size_t length) {
  /*
   * NOTE: currently only trampoline and text region are executable,
   * and they are at the beginning of the address space, so this code
   * is fine.  We will probably never allow users to mark other pages
   * as executable; but if so, we will have to revisit how this check
   * is implemented.
   *
   * nap->static_text_end is a multiple of 4K, the memory protection
   * granularity.  Since this routine is used for checking whether
   * memory map adjustments / allocations -- which has 64K granularity
   * -- is okay, usraddr must be an allocation granularity value.  Our
   * callers (as of this writing) does this, but we truncate it down
   * to an allocation boundary to be sure.
   */
  UNREFERENCED_PARAMETER(length);
  usraddr = NaClTruncAllocPage(usraddr);
  return usraddr < nap->dynamic_text_end;
}

int NaClSysCommonAddrRangeInAllowedDynamicCodeSpace(struct NaClApp *nap,
                                                    uintptr_t usraddr,
                                                    size_t length) {
  uintptr_t usr_region_end = usraddr + length;

  if (usr_region_end < usraddr) {
    /* Check for unsigned addition overflow */
    return 0;
  }
  usr_region_end = NaClRoundAllocPage(usr_region_end);
  if (usr_region_end < usraddr) {
    /* 32-bit systems only, rounding caused uint32_t overflow */
    return 0;
  }
  return (nap->dynamic_text_start <= usraddr &&
          usr_region_end <= nap->dynamic_text_end);
}

/* Warning: sizeof(nacl_abi_off_t)!=sizeof(off_t) on OSX */
int32_t NaClSysMmapIntern(struct NaClApp        *nap,
                          void                  *start,
                          size_t                length,
                          int                   prot,
                          int                   flags,
                          int                   d,
                          nacl_abi_off_t        offset) {
  int                         allowed_flags;
  struct NaClDesc             *ndp;
  uintptr_t                   usraddr;
  uintptr_t                   usrpage;
  uintptr_t                   sysaddr;
  uintptr_t                   endaddr;
  int                         mapping_code;
  uintptr_t                   map_result;
  int                         holding_app_lock;
  struct nacl_abi_stat        stbuf;
  size_t                      alloc_rounded_length;
  nacl_off64_t                file_size;
  nacl_off64_t                file_bytes;
  nacl_off64_t                host_rounded_file_bytes;
  size_t                      alloc_rounded_file_bytes;
  uint32_t                    val_flags;

  holding_app_lock = 0;
  ndp = NULL;

  allowed_flags = (NACL_ABI_MAP_FIXED | NACL_ABI_MAP_SHARED
                   | NACL_ABI_MAP_PRIVATE | NACL_ABI_MAP_ANONYMOUS);

  usraddr = (uintptr_t) start;

  if (0 != (flags & ~allowed_flags)) {
    NaClLog(2, "invalid mmap flags 0%o, ignoring extraneous bits\n", flags);
    flags &= allowed_flags;
  }

  if (0 != (flags & NACL_ABI_MAP_ANONYMOUS)) {
    /*
     * anonymous mmap, so backing store is just swap: no descriptor is
     * involved, and no memory object will be created to represent the
     * descriptor.
     */
    ndp = NULL;
  } else {
    ndp = NaClAppGetDesc(nap, d);
    if (NULL == ndp) {
      map_result = (uintptr_t) -NACL_ABI_EBADF;
      goto cleanup;
    }
  }

  mapping_code = 0;
  /*
   * Check if application is trying to do dynamic code loading by
   * mmaping a file.
   */
  if (0 != (NACL_ABI_PROT_EXEC & prot) &&
      0 != (NACL_ABI_MAP_FIXED & flags) &&
      NULL != ndp &&
      NaClSysCommonAddrRangeInAllowedDynamicCodeSpace(nap, usraddr, length)) {
    if (!nap->enable_dyncode_syscalls) {
      NaClLog(LOG_WARNING,
              "NaClSysMmap: PROT_EXEC when dyncode syscalls are disabled.\n");
      map_result = (uintptr_t) -NACL_ABI_EINVAL;
      goto cleanup;
    }
    if (0 != (NACL_ABI_PROT_WRITE & prot)) {
      NaClLog(3,
              "NaClSysMmap: asked for writable and executable code pages?!?\n");
      map_result = (uintptr_t) -NACL_ABI_EINVAL;
      goto cleanup;
    }
    mapping_code = 1;
  } else if (0 != (prot & NACL_ABI_PROT_EXEC)) {
    map_result = (uintptr_t) -NACL_ABI_EINVAL;
    goto cleanup;
  }

  /*
   * Starting address must be aligned to worst-case allocation
   * granularity.  (Windows.)
   */
  if (!NaClIsAllocPageMultiple(usraddr)) {
    if ((NACL_ABI_MAP_FIXED & flags) != 0) {
      NaClLog(2, "NaClSysMmap: address not allocation granularity aligned\n");
      map_result = (uintptr_t) -NACL_ABI_EINVAL;
      goto cleanup;
    } else {
      NaClLog(2, "NaClSysMmap: Force alignment of misaligned hint address\n");
      usraddr = NaClTruncAllocPage(usraddr);
    }
  }
  /*
   * Offset should be non-negative (nacl_abi_off_t is signed).  This
   * condition is caught when the file is stat'd and checked, and
   * offset is ignored for anonymous mappings.
   */
  if (offset < 0) {
    NaClLog(1,  /* application bug */
            "NaClSysMmap: negative file offset: %"NACL_PRId64"\n",
            (int64_t) offset);
    map_result = (uintptr_t) -NACL_ABI_EINVAL;
    goto cleanup;
  }
  /*
   * And offset must be a multiple of the allocation unit.
   */
  if (!NaClIsAllocPageMultiple((uintptr_t) offset)) {
    NaClLog(1,
            ("NaClSysMmap: file offset 0x%08"NACL_PRIxPTR" not multiple"
             " of allocation size\n"),
            (uintptr_t) offset);
    map_result = (uintptr_t) -NACL_ABI_EINVAL;
    goto cleanup;
  }

  /*
   * Round up to a page size multiple.
   *
   * Note that if length > 0xffff0000 (i.e. -NACL_MAP_PAGESIZE), rounding
   * up the length will wrap around to 0.  We check for length == 0 *after*
   * rounding up the length to simultaneously check for the length
   * originally being 0 and check for the wraparound.
   */
  alloc_rounded_length = NaClRoundAllocPage(length);
  if (alloc_rounded_length != length) {
    if (mapping_code) {
      NaClLog(3, "NaClSysMmap: length not a multiple of allocation size\n");
      map_result = (uintptr_t) -NACL_ABI_EINVAL;
      goto cleanup;
    }
    NaClLog(1,
            "NaClSysMmap: rounded length to 0x%"NACL_PRIxS"\n",
            alloc_rounded_length);
  }
  if (0 == (uint32_t) alloc_rounded_length) {
    map_result = (uintptr_t) -NACL_ABI_EINVAL;
    goto cleanup;
  }
  /*
   * Sanity check in case any later code behaves badly if
   * |alloc_rounded_length| is >=4GB.  This check shouldn't fail
   * because |length| was <4GB and we've already checked for overflow
   * when rounding it up.
   * TODO(mseaborn): Remove the need for this by using uint32_t for
   * untrusted sizes more consistently.
   */
  CHECK(alloc_rounded_length == (uint32_t) alloc_rounded_length);

  if (NULL == ndp) {
    /*
     * Note: sentinel values are bigger than the NaCl module addr space.
     */
    file_size                = kMaxUsableFileSize;
    file_bytes               = kMaxUsableFileSize;
    host_rounded_file_bytes  = kMaxUsableFileSize;
    alloc_rounded_file_bytes = kMaxUsableFileSize;
  } else {
    /*
     * We stat the file to figure out its actual size.
     *
     * This is necessary because the POSIXy interface we provide
     * allows mapping beyond the extent of a file but Windows'
     * interface does not.  We simulate the POSIX behaviour on
     * Windows.
     */
    map_result = (*((struct NaClDescVtbl const *) ndp->base.vtbl)->
                  Fstat)(ndp, &stbuf);
    if (0 != map_result) {
      goto cleanup;
    }

    /*
     * Preemptively refuse to map anything that's not a regular file or
     * shared memory segment.  Other types usually report st_size of zero,
     * which the code below will handle by just doing a dummy PROT_NONE
     * mapping for the requested size and never attempting the underlying
     * NaClDesc Map operation.  So without this check, the host OS never
     * gets the chance to refuse the mapping operation on an object that
     * can't do it.
     */
    if (!NACL_ABI_S_ISREG(stbuf.nacl_abi_st_mode) &&
        !NACL_ABI_S_ISSHM(stbuf.nacl_abi_st_mode)) {
      map_result = (uintptr_t) -NACL_ABI_ENODEV;
      goto cleanup;
    }

    /*
     * BUG(bsy): there's a race between this fstat and the actual mmap
     * below.  It's probably insoluble.  Even if we fstat again after
     * mmap and compared, the mmap could have "seen" the file with a
     * different size, after which the racing thread restored back to
     * the same value before the 2nd fstat takes place.
     */
    file_size = stbuf.nacl_abi_st_size;

    if (file_size < offset) {
      map_result = (uintptr_t) -NACL_ABI_EINVAL;
      goto cleanup;
    }

    file_bytes = file_size - offset;
    if ((nacl_off64_t) kMaxUsableFileSize < file_bytes) {
      host_rounded_file_bytes = kMaxUsableFileSize;
    } else {
      host_rounded_file_bytes = NaClRoundHostAllocPage((size_t) file_bytes);
    }

    CHECK(host_rounded_file_bytes <= (nacl_off64_t) kMaxUsableFileSize);
    /*
     * We need to deal with NaClRoundHostAllocPage rounding up to zero
     * from ~0u - n, where n < 4096 or 65536 (== 1 alloc page).
     *
     * Luckily, file_bytes is at most kMaxUsableFileSize which is
     * smaller than SIZE_T_MAX, so it should never happen, but we
     * leave the explicit check below as defensive programming.
     */
    alloc_rounded_file_bytes =
      NaClRoundAllocPage((size_t) host_rounded_file_bytes);

    if (0 == alloc_rounded_file_bytes && 0 != host_rounded_file_bytes) {
      map_result = (uintptr_t) -NACL_ABI_ENOMEM;
      goto cleanup;
    }

    /*
     * NB: host_rounded_file_bytes and alloc_rounded_file_bytes can be
     * zero.  Such an mmap just makes memory (offset relative to
     * usraddr) in the range [0, alloc_rounded_length) inaccessible.
     */
  }

  /*
   * host_rounded_file_bytes is how many bytes we can map from the
   * file, given the user-supplied starting offset.  It is at least
   * one page.  If it came from a real file, it is a multiple of
   * host-OS allocation size.  it cannot be larger than
   * kMaxUsableFileSize.
   */
  if (mapping_code && (size_t) file_bytes < alloc_rounded_length) {
    NaClLog(3,
            "NaClSysMmap: disallowing partial allocation page extension for"
            " short files\n");
    map_result = (uintptr_t) -NACL_ABI_EINVAL;
    goto cleanup;
  }
  length = size_min(alloc_rounded_length, (size_t) host_rounded_file_bytes);

  /*
   * Lock the addr space.
   */
  NaClXMutexLock(&nap->mu);

  NaClVmHoleOpeningMu(nap);

  holding_app_lock = 1;

  if (0 == (flags & NACL_ABI_MAP_FIXED)) {
    /*
     * The user wants us to pick an address range.
     */
    if (0 == usraddr) {
      /*
       * Pick a hole in addr space of appropriate size, anywhere.
       * We pick one that's best for the system.
       */
      usrpage = NaClVmmapFindMapSpace(&nap->mem_map,
                                      alloc_rounded_length >> NACL_PAGESHIFT);
      NaClLog(4, "NaClSysMmap: FindMapSpace: page 0x%05"NACL_PRIxPTR"\n",
              usrpage);
      if (0 == usrpage) {
        map_result = (uintptr_t) -NACL_ABI_ENOMEM;
        goto cleanup;
      }
      usraddr = usrpage << NACL_PAGESHIFT;
      NaClLog(4, "NaClSysMmap: new starting addr: 0x%08"NACL_PRIxPTR
              "\n", usraddr);
    } else {
      /*
       * user supplied an addr, but it's to be treated as a hint; we
       * find a hole of the right size in the app's address space,
       * according to the usual mmap semantics.
       */
      usrpage = NaClVmmapFindMapSpaceAboveHint(&nap->mem_map,
                                               usraddr,
                                               (alloc_rounded_length
                                                >> NACL_PAGESHIFT));
      NaClLog(4, "NaClSysMmap: FindSpaceAboveHint: page 0x%05"NACL_PRIxPTR"\n",
              usrpage);
      if (0 == usrpage) {
        NaClLog(4, "NaClSysMmap: hint failed, doing generic allocation\n");
        usrpage = NaClVmmapFindMapSpace(&nap->mem_map,
                                        alloc_rounded_length >> NACL_PAGESHIFT);
      }
      if (0 == usrpage) {
        map_result = (uintptr_t) -NACL_ABI_ENOMEM;
        goto cleanup;
      }
      usraddr = usrpage << NACL_PAGESHIFT;
      NaClLog(4, "NaClSysMmap: new starting addr: 0x%08"NACL_PRIxPTR"\n",
              usraddr);
    }
  }

  /*
   * Validate [usraddr, endaddr) is okay.
   */
  if (usraddr >= ((uintptr_t) 1 << nap->addr_bits)) {
    NaClLog(2,
            ("NaClSysMmap: start address (0x%08"NACL_PRIxPTR") outside address"
             " space\n"),
            usraddr);
    map_result = (uintptr_t) -NACL_ABI_EINVAL;
    goto cleanup;
  }
  endaddr = usraddr + alloc_rounded_length;
  if (endaddr < usraddr) {
    NaClLog(0,
            ("NaClSysMmap: integer overflow -- "
             "NaClSysMmap(0x%08"NACL_PRIxPTR",0x%"NACL_PRIxS",0x%x,0x%x,%d,"
             "0x%08"NACL_PRIxPTR"\n"),
            usraddr, length, prot, flags, d, (uintptr_t) offset);
    map_result = (uintptr_t) -NACL_ABI_EINVAL;
    goto cleanup;
  }
  /*
   * NB: we use > instead of >= here.
   *
   * endaddr is the address of the first byte beyond the target region
   * and it can equal the address space limit.  (of course, normally
   * the main thread's stack is there.)
   */
  if (endaddr > ((uintptr_t) 1 << nap->addr_bits)) {
    NaClLog(2,
            ("NaClSysMmap: end address (0x%08"NACL_PRIxPTR") is beyond"
             " the end of the address space\n"),
            endaddr);
    map_result = (uintptr_t) -NACL_ABI_EINVAL;
    goto cleanup;
  }

  if (mapping_code) {
    NaClLog(4,
            "NaClSysMmap: PROT_EXEC requested, usraddr 0x%08"NACL_PRIxPTR
            ", length %"NACL_PRIxS"\n",
            usraddr, length);
    if (!NACL_FI("MMAP_BYPASS_DESCRIPTOR_SAFETY_CHECK",
                 NaClDescIsSafeForMmap(ndp),
                 1) ||
        NACL_FI("MMAP_FORCE_DESCRIPTOR_SAFETY_CHECK_FAIL", 0, 1)) {
      NaClLog(4, "NaClSysMmap: descriptor not blessed\n");
      map_result = (uintptr_t) -NACL_ABI_EINVAL;
      goto cleanup;
    }
    NaClLog(4, "NaClSysMmap: allowed\n");
  } else if (NaClSysCommonAddrRangeContainsExecutablePages(nap,
                                                           usraddr,
                                                           length)) {
    NaClLog(2, "NaClSysMmap: region contains executable pages\n");
    map_result = (uintptr_t) -NACL_ABI_EINVAL;
    goto cleanup;
  }

  NaClVmIoPendingCheck_mu(nap,
                          (uint32_t) usraddr,
                          (uint32_t) (usraddr + length - 1));

  /*
   * Force NACL_ABI_MAP_FIXED, since we are specifying address in NaCl
   * app address space.
   */
  flags |= NACL_ABI_MAP_FIXED;

  /*
   * Turn off PROT_EXEC -- normal user mmapped pages should not be
   * executable.  This is primarily for the service runtime's own
   * bookkeeping -- prot is used in NaClVmmapAddWithOverwrite and will
   * be needed for remapping data pages on Windows if page protection
   * is set to PROT_NONE and back.
   *
   * NB: we've captured the notion of mapping executable memory for
   * dynamic library loading etc in mapping_code, so when we do map
   * text we will explicitly OR in NACL_ABI_PROT_EXEC as needed.
   */
  prot &= ~NACL_ABI_PROT_EXEC;

  /*
   * Exactly one of NACL_ABI_MAP_SHARED and NACL_ABI_MAP_PRIVATE is set.
   */
  if ((0 == (flags & NACL_ABI_MAP_SHARED)) ==
      (0 == (flags & NACL_ABI_MAP_PRIVATE))) {
    map_result = (uintptr_t) -NACL_ABI_EINVAL;
    goto cleanup;
  }

  sysaddr = NaClUserToSys(nap, usraddr);

  /* [0, length) */
  if (length > 0) {
    if (NULL == ndp) {
      NaClLog(4,
              ("NaClSysMmap: NaClDescIoDescMap(,,0x%08"NACL_PRIxPTR","
               "0x%08"NACL_PRIxS",0x%x,0x%x,0x%08"NACL_PRIxPTR")\n"),
              sysaddr, length, prot, flags, (uintptr_t) offset);
      map_result = NaClDescIoDescMapAnon(nap->effp,
                                         (void *) sysaddr,
                                         length,
                                         prot,
                                         flags,
                                         (off_t) offset);
    } else if (mapping_code) {
      /*
       * Map a read-only view in trusted memory, ask validator if
       * valid without patching; if okay, then map in untrusted
       * executable memory.  Fallback to using the dyncode_create
       * interface otherwise.
       *
       * On Windows, threads are already stopped by the
       * NaClVmHoleOpeningMu invocation above.
       *
       * For mmap, stopping threads on Windows is needed to ensure
       * that nothing gets allocated into the temporary address space
       * hole.  This would otherwise have been particularly dangerous,
       * since the hole is in an executable region.  We must abort the
       * program if some other trusted thread (or injected thread)
       * allocates into this space.  We also need interprocessor
       * interrupts to flush the icaches associated other cores, since
       * they may contain stale data.  NB: mmap with PROT_EXEC should
       * do this for us, since otherwise loading shared libraries in a
       * multithreaded environment cannot work in a portable fashion.
       * (Mutex locks only ensure dcache coherency.)
       *
       * For eventual munmap, stopping threads also involve looking at
       * their registers to make sure their %rip/%eip/%ip are not
       * inside the region being modified (impossible for initial
       * insertion).  This is needed because mmap->munmap->mmap could
       * cause problems due to scheduler races.
       *
       * Use NaClDynamicRegionCreate to mark region as allocated.
       *
       * See NaClElfFileMapSegment in elf_util.c for corresponding
       * mmap-based main executable loading.
       */
      uintptr_t image_sys_addr;
      NaClValidationStatus validator_status = NaClValidationFailed;
      struct NaClValidationMetadata metadata;
      int sys_ret;  /* syscall return convention */
      int ret;

      NaClLog(4, "NaClSysMmap: checking descriptor type\n");
      if (NACL_VTBL(NaClDesc, ndp)->typeTag != NACL_DESC_HOST_IO) {
        NaClLog(4, "NaClSysMmap: not supported type, got %d\n",
                NACL_VTBL(NaClDesc, ndp)->typeTag);
        map_result = (uintptr_t) -NACL_ABI_EINVAL;
        goto cleanup;
      }

      /*
       * First, try to mmap.  Check if target address range is
       * available.  It must be neither in use by NaClText interface,
       * nor used by previous mmap'd code.  We record mmap'd code
       * regions in the NaClText's data structures to avoid having to
       * deal with looking in two data structures.
       *
       * This mapping is PROT_READ | PROT_WRITE, MAP_PRIVATE so that
       * if validation fails in read-only mode, we can re-run the
       * validator to patch in place.
       */

      image_sys_addr = (*NACL_VTBL(NaClDesc, ndp)->
                        Map)(ndp,
                             NaClDescEffectorTrustedMem(),
                             (void *) NULL,
                             length,
                             NACL_ABI_PROT_READ | NACL_ABI_PROT_WRITE,
                             NACL_ABI_MAP_PRIVATE,
                             offset);
      if (NaClPtrIsNegErrno(&image_sys_addr)) {
        map_result = image_sys_addr;
        goto cleanup;
      }

      val_flags = nap->pnacl_mode ? NACL_DISABLE_NONTEMPORALS_X86 : 0;
      /* Ask validator / validation cache */
      NaClMetadataFromNaClDescCtor(&metadata, ndp);
      validator_status = NACL_FI("MMAP_FORCE_MMAP_VALIDATION_FAIL",
                                 (*nap->validator->
                                  Validate)(usraddr,
                                            (uint8_t *) image_sys_addr,
                                            length,
                                            0,  /* stubout_mode: no */
                                            val_flags,
                                            1,  /* readonly_text: yes */
                                            nap->cpu_features,
                                            &metadata,
                                            nap->validation_cache),
                                 NaClValidationFailed);
      NaClLog(3, "NaClSysMmap: prot_exec, validator_status %d\n",
              validator_status);
      NaClMetadataDtor(&metadata);

      if (NaClValidationSucceeded == validator_status) {
        /*
         * Check if target address range is actually available.  It
         * must be neither in use by NaClText interface, nor used by
         * previous mmap'd code.  We record mmap'd code regions in the
         * NaClText's data structures to avoid lo both having to deal
         * with looking in two data structures.  We could do this
         * first since this is a cheaper check, but it shouldn't
         * matter since application errors ought to be rare and we
         * shouldn't optimize for error handling, and this makes the
         * code simpler (releasing a created region is more code).
         */
        NaClXMutexLock(&nap->dynamic_load_mutex);
        ret = NaClDynamicRegionCreate(nap, NaClUserToSys(nap, usraddr), length,
                                      1);
        NaClXMutexUnlock(&nap->dynamic_load_mutex);
        if (!ret) {
          NaClLog(3, "NaClSysMmap: PROT_EXEC region"
                  " overlaps other dynamic code\n");
          map_result = (uintptr_t) -NACL_ABI_EINVAL;
          goto cleanup;
        }
        /*
         * Remove scratch mapping.
         */
        NaClHostDescUnmapUnsafe((void *) image_sys_addr, length);
        /*
         * We must succeed in mapping into the untrusted executable
         * space, since otherwise it would mean that the temporary
         * hole (for Windows) was filled by some other thread, and
         * that's unrecoverable.  For Linux and OSX, this should never
         * happen, since it's an atomic overmap.
         */
        NaClLog(3, "NaClSysMmap: mapping into executable memory\n");
        image_sys_addr = (*NACL_VTBL(NaClDesc, ndp)->
                          Map)(ndp,
                               nap->effp,
                               (void *) sysaddr,
                               length,
                               NACL_ABI_PROT_READ | NACL_ABI_PROT_EXEC,
                               NACL_ABI_MAP_PRIVATE | NACL_ABI_MAP_FIXED,
                               offset);
        if (image_sys_addr != sysaddr) {
          NaClLog(LOG_FATAL,
                  "NaClSysMmap: map into executable memory failed:"
                  " got 0x%"NACL_PRIxPTR"\n", image_sys_addr);
        }
        map_result = (int32_t) usraddr;
        goto cleanup;
      }

      NaClLog(3,
              "NaClSysMmap: did not validate in readonly_text mode;"
              " attempting to use dyncode interface.\n");

      if (holding_app_lock) {
        NaClVmHoleClosingMu(nap);
        NaClXMutexUnlock(&nap->mu);
      }

      if (NACL_FI("MMAP_STUBOUT_EMULATION", 0, 1)) {
        NaClLog(3, "NaClSysMmap: emulating stubout mode by touching memory\n");
        *(volatile uint8_t *) image_sys_addr =
            *(volatile uint8_t *) image_sys_addr;
      }

      /*
       * Fallback implementation.  Use the mapped memory as source for
       * the dynamic code insertion interface.
       */
      sys_ret = NaClTextDyncodeCreate(nap,
                                      (uint32_t) usraddr,
                                      (uint8_t *) image_sys_addr,
                                      (uint32_t) length,
                                      NULL);
      if (sys_ret < 0) {
        map_result = sys_ret;
      } else {
        map_result = (int32_t) usraddr;
      }

      NaClHostDescUnmapUnsafe((void *) image_sys_addr, length);
      goto cleanup_no_locks;
    } else {
      /*
       * This is a fix for Windows, where we cannot pass a size that
       * goes beyond the non-page-rounded end of the file.
       */
      size_t length_to_map = size_min(length, (size_t) file_bytes);

      NaClLog(4,
              ("NaClSysMmap: (*ndp->Map)(,,0x%08"NACL_PRIxPTR","
               "0x%08"NACL_PRIxS",0x%x,0x%x,0x%08"NACL_PRIxPTR")\n"),
              sysaddr, length, prot, flags, (uintptr_t) offset);

      map_result = (*((struct NaClDescVtbl const *) ndp->base.vtbl)->
                    Map)(ndp,
                         nap->effp,
                         (void *) sysaddr,
                         length_to_map,
                         prot,
                         flags,
                         (off_t) offset);
    }
    /*
     * "Small" negative integers are errno values.  Larger ones are
     * virtual addresses.
     */
    if (NaClPtrIsNegErrno(&map_result)) {
      if ((uintptr_t) -NACL_ABI_E_MOVE_ADDRESS_SPACE == map_result) {
        NaClLog(LOG_FATAL,
                ("NaClSysMmap: Map failed, but we"
                 " cannot handle address space move, error %"NACL_PRIuS"\n"),
                (size_t) map_result);
      }
      /*
       * Propagate all other errors to user code.
       */
      goto cleanup;
    }
    if (map_result != sysaddr) {
      NaClLog(LOG_FATAL, "system mmap did not honor NACL_ABI_MAP_FIXED\n");
    }
  }
  /*
   * If we are mapping beyond the end of the file, we fill this space
   * with PROT_NONE pages.
   *
   * Windows forces us to expose a mixture of 64k and 4k pages, and we
   * expose the same mappings on other platforms.  For example,
   * suppose untrusted code requests to map 0x40000 bytes from a file
   * of extent 0x100.  We will create the following regions:
   *
   *       0-  0x100  A: Bytes from the file
   *   0x100- 0x1000  B: The rest of the 4k page is accessible but undefined
   *  0x1000-0x10000  C: The rest of the 64k page is inaccessible (PROT_NONE)
   * 0x10000-0x40000  D: Further 64k pages are also inaccessible (PROT_NONE)
   *
   * On Windows, a single MapViewOfFileEx() call creates A, B and C.
   * This call will not accept a size greater than 0x100, so we have
   * to create D separately.  The hardware requires B to be accessible
   * (whenever A is accessible), but Windows does not allow C to be
   * mapped as accessible.  This is unfortunate because it interferes
   * with how ELF dynamic linkers usually like to set up an ELF
   * object's BSS.
   */
  /* inaccessible: [length, alloc_rounded_length) */
  if (length < alloc_rounded_length) {
    /*
     * On Unix, this maps regions C and D as inaccessible.  On
     * Windows, it just maps region D; region C has already been made
     * inaccessible.
     */
    size_t map_len = alloc_rounded_length - length;
    map_result = MunmapInternal(nap, sysaddr + length, map_len);
    if (map_result != 0) {
      goto cleanup;
    }
  }

  if (alloc_rounded_length > 0) {
    NaClVmmapAddWithOverwrite(&nap->mem_map,
                              NaClSysToUser(nap, sysaddr) >> NACL_PAGESHIFT,
                              alloc_rounded_length >> NACL_PAGESHIFT,
                              prot,
                              flags,
                              ndp,
                              offset,
                              file_size);
  }

  map_result = usraddr;

 cleanup:
  if (holding_app_lock) {
    NaClVmHoleClosingMu(nap);
    NaClXMutexUnlock(&nap->mu);
  }
 cleanup_no_locks:
  if (NULL != ndp) {
    NaClDescUnref(ndp);
  }

  /*
   * Check to ensure that map_result will fit into a 32-bit value. This is
   * a bit tricky because there are two valid ranges: one is the range from
   * 0 to (almost) 2^32, the other is from -1 to -4096 (our error range).
   * For a 32-bit value these ranges would overlap, but if the value is 64-bit
   * they will be disjoint.
   */
  if (map_result > UINT32_MAX
      && !NaClPtrIsNegErrno(&map_result)) {
    NaClLog(LOG_FATAL, "Overflow in NaClSysMmap: return address is "
                       "0x%"NACL_PRIxPTR"\n", map_result);
  }
  NaClLog(3, "NaClSysMmap: returning 0x%08"NACL_PRIxPTR"\n", map_result);

  return (int32_t) map_result;
}

int32_t NaClSysMmap(struct NaClAppThread  *natp,
                    uint32_t              start,
                    size_t                length,
                    int                   prot,
                    int                   flags,
                    int                   d,
                    uint32_t              offp) {
  struct NaClApp  *nap = natp->nap;
  nacl_abi_off_t  offset;

  NaClLog(3,
          "Entered NaClSysMmap(0x%08"NACL_PRIx32",0x%"NACL_PRIxS","
          "0x%x,0x%x,%d,0x%08"NACL_PRIx32")\n",
          start, length, prot, flags, d, offp);

  if (!NaClCopyInFromUser(nap, &offset, offp, sizeof(offset)))
    return -NACL_ABI_EFAULT;
  NaClLog(4, " offset = 0x%08"NACL_PRIx64"\n", (uint64_t) offset);

  return NaClSysMmapIntern(nap, (void *) (uintptr_t) start, length, prot,
                           flags, d, offset);
}

#if NACL_WINDOWS
static int32_t MunmapInternal(struct NaClApp *nap,
                              uintptr_t sysaddr, size_t length) {
  uintptr_t addr;
  uintptr_t endaddr = sysaddr + length;
  uintptr_t usraddr;
  for (addr = sysaddr; addr < endaddr; addr += NACL_MAP_PAGESIZE) {
    struct NaClVmmapEntry const *entry;
    uintptr_t                   page_num;
    uintptr_t                   offset;

    usraddr = NaClSysToUser(nap, addr);

    entry = NaClVmmapFindPage(&nap->mem_map, usraddr >> NACL_PAGESHIFT);
    if (NULL == entry) {
      continue;
    }
    NaClLog(3,
            ("NaClSysMunmap: addr 0x%08x, desc 0x%08"NACL_PRIxPTR"\n"),
            addr, (uintptr_t) entry->desc);

    page_num = usraddr - (entry->page_num << NACL_PAGESHIFT);
    offset = (uintptr_t) entry->offset + page_num;

    if (NULL != entry->desc &&
        offset < (uintptr_t) entry->file_size) {
      if (!UnmapViewOfFile((void *) addr)) {
        NaClLog(LOG_FATAL,
                ("MunmapInternal: UnmapViewOfFile failed to at addr"
                 " 0x%08"NACL_PRIxPTR", error %d\n"),
                addr, GetLastError());
      }
      /*
      * Fill the address space hole that we opened
      * with UnmapViewOfFile().
      */
      if (!VirtualAlloc((void *) addr, NACL_MAP_PAGESIZE, MEM_RESERVE,
                        PAGE_READWRITE)) {
        NaClLog(LOG_FATAL, "MunmapInternal: "
                "failed to fill hole with VirtualAlloc(), error %d\n",
                GetLastError());
      }
    } else {
      /*
       * Anonymous memory; we just decommit it and thus
       * make it inaccessible.
       */
      if (!VirtualFree((void *) addr,
                       NACL_MAP_PAGESIZE,
                       MEM_DECOMMIT)) {
        int error = GetLastError();
        NaClLog(LOG_FATAL,
                ("MunmapInternal: Could not VirtualFree MEM_DECOMMIT"
                 " addr 0x%08x, error %d (0x%x)\n"),
                addr, error, error);
      }
    }
    NaClVmmapRemove(&nap->mem_map,
                    usraddr >> NACL_PAGESHIFT,
                    NACL_PAGES_PER_MAP);
  }
  return 0;
}
#else
static int32_t MunmapInternal(struct NaClApp *nap,
                              uintptr_t sysaddr, size_t length) {
  UNREFERENCED_PARAMETER(nap);
  NaClLog(3, "MunmapInternal(0x%08"NACL_PRIxPTR", 0x%"NACL_PRIxS")\n",
          (uintptr_t) sysaddr, length);
  /*
   * Overwrite current mapping with inaccessible, anonymous
   * zero-filled pages, which should be copy-on-write and thus
   * relatively cheap.  Do not open up an address space hole.
   */
  if (MAP_FAILED == mmap((void *) sysaddr,
                         length,
                         PROT_NONE,
                         MAP_PRIVATE | MAP_ANONYMOUS | MAP_FIXED,
                         -1,
                         (off_t) 0)) {
    NaClLog(4, "mmap to put in anonymous memory failed, errno = %d\n", errno);
    return -NaClXlateErrno(errno);
  }
  NaClVmmapRemove(&nap->mem_map,
                  NaClSysToUser(nap, (uintptr_t) sysaddr) >> NACL_PAGESHIFT,
                  length >> NACL_PAGESHIFT);
  return 0;
}
#endif

int32_t NaClSysMunmap(struct NaClAppThread  *natp,
                      uint32_t              start,
                      uint32_t              length) {
  struct NaClApp *nap = natp->nap;
  int32_t   retval = -NACL_ABI_EINVAL;
  uintptr_t sysaddr;
  int       holding_app_lock = 0;
  size_t    alloc_rounded_length;

  NaClLog(3, "Entered NaClSysMunmap(0x%08"NACL_PRIxPTR", "
          "0x%08"NACL_PRIx32", 0x%"NACL_PRIx32")\n",
          (uintptr_t) natp, start, length);

  if (!NaClIsAllocPageMultiple(start)) {
    NaClLog(4, "start addr not allocation multiple\n");
    retval = -NACL_ABI_EINVAL;
    goto cleanup;
  }
  /*
   * Round up to a page size multiple.
   *
   * Note that if length > 0xffff0000 (i.e. -NACL_MAP_PAGESIZE), rounding
   * up the length will wrap around to 0.  We check for length == 0 *after*
   * rounding up the length to simultaneously check for the length
   * originally being 0 and check for the wraparound.
   */
  alloc_rounded_length = NaClRoundAllocPage(length);
  if (alloc_rounded_length != length) {
    length = (uint32_t) alloc_rounded_length;
    NaClLog(2, "munmap: rounded length to 0x%"NACL_PRIx32"\n", length);
  }
  if (0 == length) {
    /*
     * Without this check we would get the following inconsistent
     * behaviour:
     *  * On Linux, an mmap() of zero length yields a failure.
     *  * On Mac OS X, an mmap() of zero length returns no error,
     *    which would lead to a NaClVmmapUpdate() of zero pages, which
     *    should not occur.
     *  * On Windows we would iterate through the 64k pages and do
     *    nothing, which would not yield a failure.
     */
    retval = -NACL_ABI_EINVAL;
    goto cleanup;
  }
  sysaddr = NaClUserToSysAddrRange(nap, start, length);
  if (kNaClBadAddress == sysaddr) {
    NaClLog(4, "munmap: region not user addresses\n");
    retval = -NACL_ABI_EFAULT;
    goto cleanup;
  }

  NaClXMutexLock(&nap->mu);

  NaClVmHoleOpeningMu(nap);

  holding_app_lock = 1;

  /*
   * User should be unable to unmap any executable pages.  We check here.
   */
  if (NaClSysCommonAddrRangeContainsExecutablePages(nap, start, length)) {
    NaClLog(2, "NaClSysMunmap: region contains executable pages\n");
    retval = -NACL_ABI_EINVAL;
    goto cleanup;
  }

  NaClVmIoPendingCheck_mu(nap, start, start + length - 1);

  retval = MunmapInternal(nap, sysaddr, length);
cleanup:
  if (holding_app_lock) {
    NaClVmHoleClosingMu(nap);
    NaClXMutexUnlock(&nap->mu);
  }
  return retval;
}

#if NACL_WINDOWS
static int32_t MprotectInternal(struct NaClApp *nap,
                                uintptr_t sysaddr, size_t length, int prot) {
  uintptr_t addr;
  uintptr_t endaddr = sysaddr + length;
  uintptr_t usraddr;
  DWORD     flProtect;
  DWORD     flOldProtect;

  /*
   * VirtualProtect region cannot span allocations, all addresses must be
   * in one region of memory returned from VirtualAlloc or VirtualAllocEx.
   */
  for (addr = sysaddr; addr < endaddr; addr += NACL_MAP_PAGESIZE) {
    struct NaClVmmapEntry const *entry;
    uintptr_t                   page_num;
    uintptr_t                   offset;

    usraddr = NaClSysToUser(nap, addr);

    entry = NaClVmmapFindPage(&nap->mem_map, usraddr >> NACL_PAGESHIFT);
    if (NULL == entry) {
      continue;
    }
    NaClLog(3, "MprotectInternal: addr 0x%08x, desc 0x%08"NACL_PRIxPTR"\n",
            addr, (uintptr_t) entry->desc);

    page_num = usraddr - (entry->page_num << NACL_PAGESHIFT);
    offset = (uintptr_t) entry->offset + page_num;

    if (NULL == entry->desc) {
      flProtect = NaClflProtectMap(prot);

      /* Change the page protection */
      if (!VirtualProtect((void *) addr,
                          NACL_MAP_PAGESIZE,
                          flProtect,
                          &flOldProtect)) {
        int error = GetLastError();
        NaClLog(LOG_FATAL, "MprotectInternal: "
                "failed to change the memory protection with VirtualProtect,"
                " addr 0x%08x, error %d (0x%x)\n",
                addr, error, error);
        return -NaClXlateSystemError(error);
      }
    } else if (offset < (uintptr_t) entry->file_size) {
      nacl_off64_t  file_bytes;
      size_t        chunk_size;
      size_t        rounded_chunk_size;
      int           desc_flags;
      char const    *err_msg;

      desc_flags = (*NACL_VTBL(NaClDesc, entry->desc)->GetFlags)(entry->desc);
      NaClflProtectAndDesiredAccessMap(prot,
                                       (entry->flags
                                        & NACL_ABI_MAP_PRIVATE) != 0,
                                       (desc_flags & NACL_ABI_O_ACCMODE),
                                       /* flMaximumProtect= */ NULL,
                                       &flProtect,
                                       /* dwDesiredAccess= */ NULL,
                                       &err_msg);
      if (0 == flProtect) {
        /*
         * This shouldn't really happen since we already checked the address
         * space using NaClVmmapCheckExistingMapping, but better be safe.
         */
        NaClLog(LOG_FATAL, "MprotectInternal: %s\n", err_msg);
      }

      file_bytes = entry->file_size - offset;
      chunk_size = size_min((size_t) file_bytes, NACL_MAP_PAGESIZE);
      rounded_chunk_size = NaClRoundPage(chunk_size);

      NaClLog(4, "VirtualProtect(0x%08x, 0x%"NACL_PRIxS", %x)\n",
              addr, rounded_chunk_size, flProtect);

      /* Change the page protection */
      if (!VirtualProtect((void *) addr,
                          rounded_chunk_size,
                          flProtect,
                          &flOldProtect)) {
        int error = GetLastError();
        NaClLog(LOG_FATAL, "MprotectInternal: "
                "failed to change the memory protection with VirtualProtect()"
                " addr 0x%08x, error %d (0x%x)\n",
                addr, error, error);
        return -NaClXlateSystemError(error);
      }
    }
  }

  return 0;
}
#else
static int32_t MprotectInternal(struct NaClApp *nap,
                                uintptr_t sysaddr, size_t length, int prot) {
  uintptr_t               usraddr;
  uintptr_t               last_page_num;
  int                     host_prot;
  struct NaClVmmapIter    iter;
  struct NaClVmmapEntry   *entry;

  host_prot = NaClProtMap(prot);

  usraddr = NaClSysToUser(nap, sysaddr);
  last_page_num = (usraddr + length) >> NACL_PAGESHIFT;

  NaClVmmapFindPageIter(&nap->mem_map,
                        usraddr >> NACL_PAGESHIFT,
                        &iter);
  entry = NaClVmmapIterStar(&iter);

  CHECK(usraddr == (entry->page_num << NACL_PAGESHIFT));

  for (; !NaClVmmapIterAtEnd(&iter) &&
           (NaClVmmapIterStar(&iter))->page_num < last_page_num;
       NaClVmmapIterIncr(&iter)) {
    uintptr_t addr;
    size_t    entry_len;

    entry = NaClVmmapIterStar(&iter);

    addr = NaClUserToSys(nap, entry->page_num << NACL_PAGESHIFT);
    entry_len = entry->npages << NACL_PAGESHIFT;

    NaClLog(3, "MprotectInternal: "
            "addr 0x%08"NACL_PRIxPTR", desc 0x%08"NACL_PRIxPTR"\n",
            addr, (uintptr_t) entry->desc);

    if (NULL == entry->desc) {
      if (0 != mprotect((void *) addr, entry_len, host_prot)) {
        NaClLog(LOG_FATAL, "MprotectInternal: "
                "mprotect on anonymous memory failed, errno = %d\n", errno);
        return -NaClXlateErrno(errno);
      }
    } else if (entry->offset < entry->file_size) {
      nacl_abi_off64_t  file_bytes;
      size_t            rounded_file_bytes;
      size_t            prot_len;

      file_bytes = entry->file_size - entry->offset;
      rounded_file_bytes = NaClRoundPage((size_t) file_bytes);
      prot_len = size_min(rounded_file_bytes, entry_len);

      if (0 != mprotect((void *) addr, prot_len, host_prot)) {
        NaClLog(LOG_FATAL, "MprotectInternal: "
                "mprotect on file-backed memory failed, errno = %d\n", errno);
        return -NaClXlateErrno(errno);
      }
    }
  }

  CHECK((entry->page_num + entry->npages) == last_page_num);

  return 0;
}
#endif

int32_t NaClSysMprotectInternal(struct NaClApp  *nap,
                                uint32_t        start,
                                size_t          length,
                                int             prot) {
  int32_t     retval = -NACL_ABI_EINVAL;
  uintptr_t   sysaddr;
  int         holding_app_lock = 0;

  if (!NaClIsAllocPageMultiple((uintptr_t) start)) {
    NaClLog(4, "mprotect: start addr not allocation multiple\n");
    retval = -NACL_ABI_EINVAL;
    goto cleanup;
  }
  length = NaClRoundAllocPage(length);
  sysaddr = NaClUserToSysAddrRange(nap, (uintptr_t) start, length);
  if (kNaClBadAddress == sysaddr) {
    NaClLog(4, "mprotect: region not user addresses\n");
    retval = -NACL_ABI_EFAULT;
    goto cleanup;
  }
  if (0 != (~(NACL_ABI_PROT_READ | NACL_ABI_PROT_WRITE) & prot)) {
    NaClLog(4, "mprotect: prot has other bits than PROT_{READ|WRITE}\n");
    retval = -NACL_ABI_EACCES;
    goto cleanup;
  }

  NaClXMutexLock(&nap->mu);

  holding_app_lock = 1;

  /*
   * User should be unable to change protection of any executable pages.
   */
  if (NaClSysCommonAddrRangeContainsExecutablePages(nap,
                                                    (uintptr_t) start,
                                                    length)) {
    NaClLog(2, "mprotect: region contains executable pages\n");
    retval = -NACL_ABI_EACCES;
    goto cleanup;
  }

  if (!NaClVmmapChangeProt(&nap->mem_map,
                           NaClSysToUser(nap, sysaddr) >> NACL_PAGESHIFT,
                           length >> NACL_PAGESHIFT,
                           prot)) {
    NaClLog(4, "mprotect: no such region\n");
    retval = -NACL_ABI_EACCES;
    goto cleanup;
  }

  NaClVmIoPendingCheck_mu(nap,
                          (uint32_t) (uintptr_t) start,
                          (uint32_t) ((uintptr_t) start + length - 1));

  retval = MprotectInternal(nap, sysaddr, length, prot);
cleanup:
  if (holding_app_lock) {
    NaClXMutexUnlock(&nap->mu);
  }
  return retval;
}

int32_t NaClSysMprotect(struct NaClAppThread  *natp,
                        uint32_t              start,
                        size_t                length,
                        int                   prot) {
  struct NaClApp  *nap = natp->nap;

  NaClLog(3, "Entered NaClSysMprotect(0x%08"NACL_PRIxPTR", "
          "0x%08"NACL_PRIxPTR", 0x%"NACL_PRIxS", 0x%x)\n",
          (uintptr_t) natp, (uintptr_t) start, length, prot);

  return NaClSysMprotectInternal(nap, start, length, prot);
}
