/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * NaCl Service Runtime memory allocation code
 */
#include "native_client/src/include/portability.h"
#include "native_client/src/include/win/mman.h"

#include <errno.h>
#include <windows.h>
#include <string.h>

#include "native_client/src/shared/platform/nacl_check.h"
#include "native_client/src/shared/platform/nacl_global_secure_random.h"
#include "native_client/src/shared/platform/nacl_host_desc.h"
#include "native_client/src/shared/platform/nacl_log.h"
#include "native_client/src/shared/platform/win/xlate_system_error.h"

#include "native_client/src/trusted/service_runtime/nacl_config.h"
#include "native_client/src/trusted/service_runtime/sel_memory.h"
#include "native_client/src/trusted/service_runtime/sel_util.h"

/*
 * NaClPageFree: free pages allocated with NaClPageAlloc.
 * Must start at allocation granularity (NACL_MAP_PAGESIZE) and
 * number of bytes must be a multiple of allocation granularity.
 */
void NaClPageFree(void *p, size_t num_bytes) {
  void  *end_addr;

  end_addr = (void *) (((char *) p) + num_bytes);
  while (p < end_addr) {
    if (!VirtualFree(p, 0, MEM_RELEASE)) {
      DWORD err = GetLastError();
      NaClLog(LOG_FATAL,
              "NaClPageFree: VirtualFree(0x%016"NACL_PRIxPTR
              ", 0, MEM_RELEASE) failed "
              "with error 0x%X\n",
              (uintptr_t) p, err);
    }
    p = (void *) (((char *) p) + NACL_MAP_PAGESIZE);
  }
}


int NaClPageAllocAtAddr(void **p, size_t num_bytes) {
  SYSTEM_INFO sys_info;

  int         attempt_count;

  void        *hint = *p;
  void        *addr;
  void        *end_addr;
  void        *chunk;
  void        *unroll;

  /*
   * We have to allocate every 64KB -- the windows allocation
   * granularity -- because VirtualFree will only accept an address
   * that was returned by a call to VirtualAlloc.  NB: memory pages
   * put into the address space via MapViewOfFile{,Ex} must be
   * released by UnmapViewOfFile.  Thus, in order for us to open up a
   * hole in the NaCl application's address space to map in a file, we
   * must allocate the entire address space in 64KB chunks, so we can
   * later pick an arbitrary range of addresses (in 64KB chunks) to
   * free up and map in a file later.
   *
   * First, we verify via GetSystemInfo that the allocation
   * granularity matches NACL_MAP_PAGESIZE.
   *
   * Next, we VirtualAlloc the entire chunk desired.  This essentially
   * asks the kernel where there is space in the virtual address
   * space.  Then, we free this back, and repeat the allocation
   * starting at the returned address, but in 64KB chunks.  If any of
   * these smaller allocations fail, we roll back and try again.
   */

  NaClLog(3, "NaClPageAlloc(*, 0x%"NACL_PRIxS")\n", num_bytes);
  GetSystemInfo(&sys_info);
  if (NACL_PAGESIZE != sys_info.dwPageSize) {
    NaClLog(2, "page size is 0x%x; expected 0x%x\n",
            sys_info.dwPageSize,
            NACL_PAGESIZE);
  }
  if (NACL_MAP_PAGESIZE != sys_info.dwAllocationGranularity) {
    NaClLog(LOG_ERROR, "allocation granularity is 0x%x; expected 0x%x\n",
            sys_info.dwAllocationGranularity,
            NACL_MAP_PAGESIZE);
  }

  /*
   * Round allocation request up to next NACL_MAP_PAGESIZE.  This is
   * assumed to have taken place in NaClPageFree.
   */
  num_bytes = NaClRoundAllocPage(num_bytes);

  for (attempt_count = 0;
       attempt_count < NACL_MEMORY_ALLOC_RETRY_MAX;
       ++attempt_count) {

    addr = VirtualAlloc(hint, num_bytes, MEM_RESERVE, PAGE_NOACCESS);
    if (addr == NULL) {
      NaClLog(LOG_ERROR,
              "NaClPageAlloc: VirtualAlloc(*,0x%"NACL_PRIxS") failed\n",
              num_bytes);
      return -ENOMEM;
    }
    NaClLog(3,
            ("NaClPageAlloc:"
             "  VirtualAlloc(*,0x%"NACL_PRIxS")"
             " succeeded, 0x%016"NACL_PRIxPTR","
             " releasing and re-allocating in 64K chunks....\n"),
            num_bytes, (uintptr_t) addr);
    (void) VirtualFree(addr, 0, MEM_RELEASE);
    /*
     * We now know [addr, addr + num_bytes) is available in our addr space.
     * Get that memory again, but in 64KB chunks.
     */
    end_addr = (void *) (((char *) addr) + num_bytes);
    for (chunk = addr;
         chunk < end_addr;
         chunk = (void *) (((char *) chunk) + NACL_MAP_PAGESIZE)) {
      if (NULL == VirtualAlloc(chunk,
                               NACL_MAP_PAGESIZE,
                               MEM_RESERVE,
                               PAGE_NOACCESS)) {
        NaClLog(LOG_ERROR,
                ("NaClPageAlloc: re-allocation failed at "
                 "0x%016"NACL_PRIxPTR","
                 " error %d\n"),
                (uintptr_t) chunk, GetLastError());
        for (unroll = addr;
             unroll < chunk;
             unroll = (void *) (((char *) unroll) + NACL_MAP_PAGESIZE)) {
          (void) VirtualFree(unroll, 0, MEM_RELEASE);
        }
        goto retry;
        /* double break */
      }
    }
    NaClLog(2,
            ("NaClPageAlloc: *p = 0x%016"NACL_PRIxPTR","
             " returning success.\n"),
            (uintptr_t) addr);
    *p = addr;
    return 0;
 retry:
    NaClLog(2, "NaClPageAllocAtAddr: retrying w/o hint\n");
    hint = NULL;
  }

  return -ENOMEM;
}

int NaClPageAlloc(void **p, size_t num_bytes) {
  *p = NULL;
  return NaClPageAllocAtAddr(p, num_bytes);
}

/*
 * Pick a "hint" address that is random.
 */
int NaClPageAllocRandomized(void **p, size_t num_bytes) {
  uintptr_t addr;
  int       neg_errno = -ENOMEM;  /* in case we change kNumTries to 0 */
  int       tries;
  const int kNumTries = 4;

  for (tries = 0; tries < kNumTries; ++tries) {
#if NACL_HOST_WORDSIZE == 32
    addr = NaClGlobalSecureRngUint32();
    NaClLog(2, "NaClPageAllocRandomized: 0x%"NACL_PRIxPTR"\n", addr);
    /*
     * Windows permits 2-3 GB of user address space, depending on
     * 4-gigabyte tuning (4GT) parameter.  We ask for somewhere in the
     * lower 2G.
     */
    *p = (void *) (addr & ~((uintptr_t) NACL_MAP_PAGESIZE - 1)
                   & ((~(uintptr_t) 0) >> 1));
#elif NACL_HOST_WORDSIZE == 64
    addr = NaClGlobalSecureRngUint32();
    NaClLog(2, "NaClPageAllocRandomized: 0x%"NACL_PRIxPTR"\n", addr);
    /*
     * 64-bit windows permits 8 TB of user address space, and we keep
     * the low 16 bits free (64K alignment), so we just have 43-16=27
     * bits of entropy.
     */
    *p = (void *) ((addr << NACL_MAP_PAGESHIFT)  /* bits [47:16] are random */
                   & ((((uintptr_t) 1) << 43) - 1));  /* now bits [42:16] */
#else
# error "where am i?"
#endif

    NaClLog(2, "NaClPageAllocRandomized: hint 0x%"NACL_PRIxPTR"\n",
            (uintptr_t) *p);
    neg_errno = NaClPageAllocAtAddr(p, num_bytes);
    if (0 == neg_errno) {
      break;
    }
  }
  if (0 != neg_errno) {
    NaClLog(LOG_INFO,
            "NaClPageAllocRandomized: failed (%d), dropping hints\n",
            -neg_errno);
    *p = 0;
    neg_errno = NaClPageAllocAtAddr(p, num_bytes);
  }
  return neg_errno;
}

static uintptr_t NaClProtectChunkSize(uintptr_t start,
                                      uintptr_t end) {
  uintptr_t chunk_end;

  chunk_end = NaClRoundAllocPage(start + 1);
  if (chunk_end > end) {
    chunk_end = end;
  }
  return chunk_end - start;
}


/*
 * Change memory protection.
 *
 * This is critical to make the text region non-writable, and the data
 * region read/write but no exec.  Of course, some kernels do not
 * respect the lack of PROT_EXEC.
 *
 * NB: this function signature is likely to need to change.  In
 * particular, when on Windows, if the memory originated from a memory
 * mapping, whether the file was MAP_SHARED or MAP_PRIVATE and thus
 * whether the dwDesiredAccess used in the MapViewOfFileEx contained
 * FILE_MAP_WRITE or FILE_MAP_COPY determines whether the right
 * newProtection below should be PAGE_READWRITE or PAGE_WRITECOPY.
 * So, we need to have either extend prot to include a PROT_CoW, or
 * otherwise some way to look up whether the region had what level of
 * access when MapViewOfFileEx was invoked.
 *
 * For now, we implement a "fallback" mechanism on windows, where if
 * PAGE_READWRITE fails, we retry with PAGE_WRITECOPY.  Fortunately,
 * this is only needed when setting up memory protection for the BSS
 * segment during program loading, when the executable was deemed to
 * be safe-for-mapping.
 */
int NaClMprotect(void *addr, size_t len, int prot) {
  uintptr_t start_addr;
  uintptr_t end_addr;
  uintptr_t cur_addr;
  uintptr_t cur_chunk_size;
  DWORD     newProtection, oldProtection;
  BOOL      res;
  int       xlated_res;

  NaClLog(2, "NaClMprotect(0x%016"NACL_PRIxPTR", 0x%"NACL_PRIxS", 0x%x)\n",
          (uintptr_t) addr, len, prot);

  if (len == 0) {
    return 0;
  }

  start_addr = (uintptr_t) addr;
  if (!NaClIsPageMultiple(start_addr)) {
    NaClLog(2, "NaClMprotect: start address not a multiple of page size\n");
    return -EINVAL;
  }
  if (!NaClIsPageMultiple(len)) {
    NaClLog(2, "NaClMprotect: length not a multiple of page size\n");
    return -EINVAL;
  }
  end_addr = start_addr + len;
  newProtection = NaClflProtectMap(prot);
  /*
   * VirtualProtect region cannot span allocations: all addresses from
   * [lpAddress, lpAddress+dwSize) must be in one region of memory
   * returned from VirtualAlloc or VirtualAllocEx
   */
  for (cur_addr = start_addr,
           cur_chunk_size = NaClProtectChunkSize(cur_addr, end_addr);
       cur_addr < end_addr;
       cur_addr += cur_chunk_size,
           cur_chunk_size = NaClProtectChunkSize(cur_addr, end_addr)) {
    NaClLog(7,
            "NaClMprotect: VirtualProtect(0x%016"NACL_PRIxPTR","
            " 0x%"NACL_PRIxPTR", 0x%x, *)\n",
            cur_addr, cur_chunk_size, newProtection);

    if (newProtection != PAGE_NOACCESS) {
      res = VirtualProtect((void *) cur_addr,
                           cur_chunk_size,
                           newProtection,
                           &oldProtection);

      if (!res && newProtection == PAGE_READWRITE) {
        NaClLog(4,
                "NaClMprotect: VirtualProtect(0x%016"NACL_PRIxPTR","
                " 0x%"NACL_PRIxPTR", 0x%x, *) failed,"
                " trying CoW fallback\n",
                cur_addr, cur_chunk_size, newProtection);
        res = VirtualProtect((void *) cur_addr,
                             cur_chunk_size,
                             PAGE_WRITECOPY,
                             &oldProtection);
        if (!res) {
          NaClLog(4,
                  "NaClMprotect: VirtualProtect with PAGE_WRITECOPY failed,"
                  " trying VirtualAlloc\n");
        }
      }

      if (!res) {
        void *p;
        p = VirtualAlloc((void*) cur_addr,
                         cur_chunk_size,
                         MEM_COMMIT,
                         newProtection);
        if (p != (void*) cur_addr) {
          NaClLog(2,
                  "NaClMprotect: VirtualAlloc(0x%016"NACL_PRIxPTR","
                  " 0x%"NACL_PRIxPTR", MEM_COMMIT, 0x%x) failed\n",
                  cur_addr, cur_chunk_size, newProtection);
          return -NaClXlateSystemError(GetLastError());
        }
      }
    } else {
      /*
       * decommit the memory--this has the same effect as setting the protection
       * level to PAGE_NOACCESS, with the added benefit of not taking up any
       * swap space.
       */
      if (!VirtualFree((void *) cur_addr, cur_chunk_size, MEM_DECOMMIT)) {
        xlated_res = NaClXlateSystemError(GetLastError());
        NaClLog(2, "NaClMprotect: VirtualFree failed: 0x%x\n", xlated_res);
        return -xlated_res;
      }
    }
  }
  NaClLog(2, "NaClMprotect: done\n");
  return 0;
}

int NaClMadvise(void *start, size_t length, int advice) {
  int       err;
  uintptr_t start_addr;
  uintptr_t end_addr;
  uintptr_t cur_addr;
  uintptr_t cur_chunk_size;

  /*
   * MADV_DONTNEED and MADV_NORMAL are needed
   */
  NaClLog(5, "NaClMadvise(0x%016"NACL_PRIxPTR", 0x%"NACL_PRIxS", 0x%x)\n",
          (uintptr_t) start, length, advice);
  switch (advice) {
    case MADV_DONTNEED:
      start_addr = (uintptr_t) start;
      if (!NaClIsPageMultiple(start_addr)) {
        NaClLog(2,
                "NaClMadvise: start address not a multiple of page size\n");
        return -EINVAL;
      }
      if (!NaClIsPageMultiple(length)) {
        NaClLog(2, "NaClMadvise: length not a multiple of page size\n");
        return -EINVAL;
      }
      end_addr = start_addr + length;
      for (cur_addr = start_addr,
               cur_chunk_size = NaClProtectChunkSize(cur_addr, end_addr);
           cur_addr < end_addr;
           cur_addr += cur_chunk_size,
               cur_chunk_size = NaClProtectChunkSize(cur_addr, end_addr)) {
        NaClLog(7,
                ("NaClMadvise: MADV_DONTNEED"
                 " -> VirtualAlloc(0x%016"NACL_PRIxPTR","
                 " 0x%"NACL_PRIxPTR", MEM_RESET, PAGE_NOACCESS)\n"),
                cur_addr, cur_chunk_size);

        /*
         * Decommit (but do not release) the page. This allows the kernel to
         * release backing store, but does not release the VA space. Should be
         * fairly close to the behavior we'd get from the Linux madvise()
         * function.
         */
        if (NULL == VirtualAlloc((void *) cur_addr,
                                 cur_chunk_size, MEM_RESET, PAGE_READONLY)) {
          err = NaClXlateSystemError(GetLastError());
          NaClLog(2, "NaClMadvise: VirtualFree failed: 0x%x\n", err);
          return -err;
        }
      }
      break;
    case MADV_NORMAL:
      memset(start, 0, length);
      break;
    default:
      return -EINVAL;
  }
  NaClLog(5, "NaClMadvise: done\n");
  return 0;
}
