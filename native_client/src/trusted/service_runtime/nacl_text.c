/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <string.h>

#include "native_client/src/include/build_config.h"
#include "native_client/src/include/concurrency_ops.h"
#include "native_client/src/include/nacl_platform.h"
#include "native_client/src/include/portability.h"
#include "native_client/src/shared/platform/nacl_check.h"
#include "native_client/src/shared/platform/nacl_log.h"
#include "native_client/src/shared/platform/nacl_sync.h"
#include "native_client/src/shared/platform/nacl_sync_checked.h"
#include "native_client/src/trusted/desc/nacl_desc_base.h"
#include "native_client/src/trusted/desc/nacl_desc_effector.h"
#include "native_client/src/trusted/desc/nacl_desc_effector_trusted_mem.h"
#include "native_client/src/trusted/desc/nacl_desc_imc_shm.h"
#include "native_client/src/trusted/perf_counter/nacl_perf_counter.h"
#include "native_client/src/trusted/platform_qualify/nacl_os_qualify.h"
#include "native_client/src/trusted/service_runtime/arch/sel_ldr_arch.h"
#include "native_client/src/trusted/service_runtime/include/bits/mman.h"
#include "native_client/src/trusted/service_runtime/include/sys/errno.h"
#include "native_client/src/trusted/service_runtime/nacl_app_thread.h"
#include "native_client/src/trusted/service_runtime/nacl_error_code.h"
#include "native_client/src/trusted/service_runtime/nacl_text.h"
#include "native_client/src/trusted/service_runtime/sel_ldr.h"
#include "native_client/src/trusted/service_runtime/sel_memory.h"
#include "native_client/src/trusted/service_runtime/thread_suspension.h"

#if NACL_OSX
#include "native_client/src/trusted/desc/osx/nacl_desc_imc_shm_mach.h"
#endif

/* initial size of the malloced buffer for dynamic regions */
static const int kMinDynamicRegionsAllocated = 32;

static const int kBitsPerByte = 8;

static uint8_t *BitmapAllocate(uint32_t indexes) {
  uint32_t byte_count = (indexes + kBitsPerByte - 1) / kBitsPerByte;
  uint8_t *bitmap = malloc(byte_count);
  if (bitmap != NULL) {
    memset(bitmap, 0, byte_count);
  }
  return bitmap;
}

static int BitmapIsBitSet(uint8_t *bitmap, uint32_t index) {
  return (bitmap[index / kBitsPerByte] & (1 << (index % kBitsPerByte))) != 0;
}

static void BitmapSetBit(uint8_t *bitmap, uint32_t index) {
  bitmap[index / kBitsPerByte] |= 1 << (index % kBitsPerByte);
}

#if NACL_OSX
/*
 * Helper function for NaClMakeDynamicTextShared.
 */
static struct NaClDesc *MakeImcShmMachDesc(uintptr_t size) {
  struct NaClDescImcShmMach *shm =
      (struct NaClDescImcShmMach *) malloc(sizeof(struct NaClDescImcShmMach));
  CHECK(shm);

  if (!NaClDescImcShmMachAllocCtor(shm, size, /* executable= */ 1)) {
    free(shm);
    NaClLog(4, "NaClMakeDynamicTextShared: shm alloc ctor for text failed\n");
    return NULL;
  }

  return &shm->base;
}
#endif

/*
 * Helper function for NaClMakeDynamicTextShared.
 */
static struct NaClDesc *MakeImcShmDesc(uintptr_t size) {
#if NACL_OSX
  if (NaClOSX10Dot7OrLater())
    return MakeImcShmMachDesc(size);
#endif
  struct NaClDescImcShm *shm =
      (struct NaClDescImcShm *) malloc(sizeof(struct NaClDescImcShm));
  CHECK(shm);

  if (!NaClDescImcShmAllocCtor(shm, size, /* executable= */ 1)) {
    free(shm);
    NaClLog(4, "NaClMakeDynamicTextShared: shm alloc ctor for text failed\n");
    return NULL;
  }

  return &shm->base;
}

NaClErrorCode NaClMakeDynamicTextShared(struct NaClApp *nap) {
  uintptr_t                   dynamic_text_size;
  uintptr_t                   shm_vaddr_base;
  int                         mmap_protections;
  uintptr_t                   mmap_ret;

  uintptr_t                   shm_upper_bound;
  uintptr_t                   text_sysaddr;
  struct NaClDesc *           shm;

  shm_vaddr_base = NaClEndOfStaticText(nap);
  NaClLog(4,
          "NaClMakeDynamicTextShared: shm_vaddr_base = %08"NACL_PRIxPTR"\n",
          shm_vaddr_base);
  shm_vaddr_base = NaClRoundAllocPage(shm_vaddr_base);
  NaClLog(4,
          "NaClMakeDynamicTextShared: shm_vaddr_base = %08"NACL_PRIxPTR"\n",
          shm_vaddr_base);

  /*
   * Default is that there is no usable dynamic code area.
   */
  nap->dynamic_text_start = shm_vaddr_base;
  nap->dynamic_text_end = shm_vaddr_base;
  if (!nap->use_shm_for_dynamic_text) {
    NaClLog(4,
            "NaClMakeDynamicTextShared:"
            "  rodata / data segments not allocation aligned\n");
    NaClLog(4,
            " not using shm for text\n");
    return LOAD_OK;
  }

  /*
   * Allocate a shm region the size of which is nap->rodata_start -
   * end-of-text.  This implies that the "core" text will not be
   * backed by shm.
   */
  shm_upper_bound = nap->rodata_start;
  if (0 == shm_upper_bound) {
    shm_upper_bound = NaClTruncAllocPage(nap->data_start);
  }
  if (0 == shm_upper_bound) {
    shm_upper_bound = shm_vaddr_base;
  }

  NaClLog(4, "shm_upper_bound = %08"NACL_PRIxPTR"\n", shm_upper_bound);

  dynamic_text_size = shm_upper_bound - shm_vaddr_base;
  NaClLog(4,
          "NaClMakeDynamicTextShared: dynamic_text_size = %"NACL_PRIxPTR"\n",
          dynamic_text_size);

  if (0 == dynamic_text_size) {
    NaClLog(4, "Empty JITtable region\n");
    return LOAD_OK;
  }

  shm = MakeImcShmDesc(dynamic_text_size);
  if (!shm) {
    return LOAD_NO_MEMORY_FOR_DYNAMIC_TEXT;
  }

  text_sysaddr = NaClUserToSys(nap, shm_vaddr_base);

  /*
   * On Windows, we must unmap this range before the OS will let us remap
   * it.  This involves opening up an address space hole, which is risky
   * because another thread might call mmap() and receive an allocation
   * inside that hole.  We don't need to take that risk on Unix, where
   * MAP_FIXED overwrites mappings atomically.
   *
   * We use NaClPageFree() here because the existing memory was mapped
   * using VirtualAlloc().
   */
  if (NACL_WINDOWS) {
    NaClPageFree((void *) text_sysaddr, dynamic_text_size);
  }

  /*
   * Unix allows us to map pages with PROT_NONE initially and later
   * increase the mapping permissions with mprotect().
   *
   * Windows does not allow this, however: the initial permissions are
   * an upper bound on what the permissions may later be changed to
   * with VirtualProtect() or VirtualAlloc().  Given this, using
   * PROT_NONE at this point does not even make sense.  On Windows,
   * the pages start off as uncommitted, which makes them inaccessible
   * regardless of the page permissions they are mapped with.
   *
   * Write permissions are included here for nacl64-gdb to set
   * breakpoints.
   */
#if NACL_WINDOWS
  mmap_protections =
    NACL_ABI_PROT_READ | NACL_ABI_PROT_EXEC | NACL_ABI_PROT_WRITE;
#else
  mmap_protections = NACL_ABI_PROT_NONE;
#endif
  NaClLog(4,
          "NaClMakeDynamicTextShared: Map(,,0x%"NACL_PRIxPTR",size = 0x%x,"
          " prot=0x%x, flags=0x%x, offset=0)\n",
          text_sysaddr,
          (int) dynamic_text_size,
          mmap_protections,
          NACL_ABI_MAP_SHARED | NACL_ABI_MAP_FIXED);
  mmap_ret = (*NACL_VTBL(NaClDesc, shm)->
              Map)(shm,
                   NaClDescEffectorTrustedMem(),
                   (void *) text_sysaddr,
                   dynamic_text_size,
                   mmap_protections,
                   NACL_ABI_MAP_SHARED | NACL_ABI_MAP_FIXED,
                   0);
  if (text_sysaddr != mmap_ret) {
    NaClLog(LOG_FATAL, "Could not map in shm for dynamic text region\n");
  }

  nap->dynamic_page_bitmap =
    BitmapAllocate((uint32_t) (dynamic_text_size / NACL_MAP_PAGESIZE));
  if (NULL == nap->dynamic_page_bitmap) {
    NaClLog(LOG_FATAL, "NaClMakeDynamicTextShared: BitmapAllocate() failed\n");
  }

  nap->dynamic_text_start = shm_vaddr_base;
  nap->dynamic_text_end = shm_upper_bound;
  nap->text_shm = shm;
  return LOAD_OK;
}

/*
 * Binary search nap->dynamic_regions to find the maximal region with start<=ptr
 * caller must hold nap->dynamic_load_mutex, and must discard result
 * when lock is released.
 */
struct NaClDynamicRegion* NaClDynamicRegionFindClosestLEQ(struct NaClApp *nap,
                                                          uintptr_t ptr) {
  const int kBinarySearchToScanCutoff = 16;
  int begin = 0;
  int end = nap->num_dynamic_regions;
  if (0 == nap->num_dynamic_regions) {
    return NULL;
  }
  /* as an optimization, check the last region first */
  if (nap->dynamic_regions[nap->num_dynamic_regions-1].start <= ptr) {
    return nap->dynamic_regions + nap->num_dynamic_regions-1;
  }
  /* comes before everything */
  if (ptr < nap->dynamic_regions[0].start) {
    return NULL;
  }
  /* binary search, until range is small */
  while (begin + kBinarySearchToScanCutoff + 1 < end) {
    int mid = begin + (end - begin)/2;
    if (nap->dynamic_regions[mid].start <= ptr) {
      begin = mid;
    } else {
      end = mid;
    }
  }
  /* linear scan, faster for small ranges */
  while (begin + 1 < end && nap->dynamic_regions[begin + 1].start <= ptr) {
    begin++;
  }
  return nap->dynamic_regions + begin;
}

struct NaClDynamicRegion* NaClDynamicRegionFind(struct NaClApp *nap,
                                                uintptr_t ptr,
                                                size_t size) {
  struct NaClDynamicRegion *p =
      NaClDynamicRegionFindClosestLEQ(nap, ptr + size - 1);
  return (p != NULL && ptr < p->start + p->size) ? p : NULL;
}

int NaClDynamicRegionCreate(struct NaClApp *nap,
                            uintptr_t start,
                            size_t size,
                            int is_mmap) {
  struct NaClDynamicRegion item, *regionp, *end;
  item.start = start;
  item.size = size;
  item.delete_generation = -1;
  item.is_mmap = is_mmap;
  if (nap->dynamic_regions_allocated == nap->num_dynamic_regions) {
    /* out of space, double buffer size */
    nap->dynamic_regions_allocated *= 2;
    if (nap->dynamic_regions_allocated < kMinDynamicRegionsAllocated) {
      nap->dynamic_regions_allocated = kMinDynamicRegionsAllocated;
    }
    nap->dynamic_regions = realloc(nap->dynamic_regions,
                sizeof(struct NaClDynamicRegion) *
                   nap->dynamic_regions_allocated);
    if (NULL == nap->dynamic_regions) {
      NaClLog(LOG_FATAL, "NaClDynamicRegionCreate: realloc failed");
      return 0;
    }
  }
  /* find preceding entry */
  regionp = NaClDynamicRegionFindClosestLEQ(nap, start + size - 1);
  if (regionp != NULL && start < regionp->start + regionp->size) {
    /* target already in use */
    return 0;
  }
  if (NULL == regionp) {
    /* start at beginning if we couldn't find predecessor */
    regionp = nap->dynamic_regions;
  }
  end = nap->dynamic_regions + nap->num_dynamic_regions;
  /* scroll to insertion point (this should scroll at most 1 element) */
  for (; regionp != end && regionp->start < item.start; ++regionp);
  /* insert and shift everything forward by 1 */
  for (; regionp != end; ++regionp) {
    /* swap(*i, item); */
    struct NaClDynamicRegion t = *regionp;
    *regionp = item;
    item = t;
  }
  *regionp = item;
  nap->num_dynamic_regions++;
  return 1;
}

void NaClDynamicRegionDelete(struct NaClApp *nap, struct NaClDynamicRegion* r) {
  struct NaClDynamicRegion *end = nap->dynamic_regions
                                + nap->num_dynamic_regions;
  /* shift everything down */
  for (; r + 1 < end; ++r) {
    r[0] = r[1];
  }
  nap->num_dynamic_regions--;

  if ( nap->dynamic_regions_allocated > kMinDynamicRegionsAllocated
     && nap->dynamic_regions_allocated/4 > nap->num_dynamic_regions) {
    /* too much waste, shrink buffer*/
    nap->dynamic_regions_allocated /= 2;
    nap->dynamic_regions = realloc(nap->dynamic_regions,
                sizeof(struct NaClDynamicRegion) *
                   nap->dynamic_regions_allocated);
    if (NULL == nap->dynamic_regions) {
      NaClLog(LOG_FATAL, "NaClDynamicRegionCreate: realloc failed");
      return;
    }
  }
}


void NaClSetThreadGeneration(struct NaClAppThread *natp, int generation) {
  /*
   * outer check handles fast case (no change)
   * since threads only set their own generation it is safe
   */
  if (natp->dynamic_delete_generation != generation)  {
    NaClXMutexLock(&natp->mu);
    CHECK(natp->dynamic_delete_generation <= generation);
    natp->dynamic_delete_generation = generation;
    NaClXMutexUnlock(&natp->mu);
  }
}

int NaClMinimumThreadGeneration(struct NaClApp *nap) {
  size_t index;
  int rv = INT_MAX;
  NaClXMutexLock(&nap->threads_mu);
  for (index = 0; index < nap->threads.num_entries; ++index) {
    struct NaClAppThread *thread = NaClGetThreadMu(nap, (int) index);
    if (thread != NULL) {
      NaClXMutexLock(&thread->mu);
      if (rv > thread->dynamic_delete_generation) {
        rv = thread->dynamic_delete_generation;
      }
      NaClXMutexUnlock(&thread->mu);
    }
  }
  NaClXMutexUnlock(&nap->threads_mu);
  return rv;
}

static void CopyBundleTails(uint8_t *dest,
                            uint8_t *src,
                            int32_t size,
                            int     bundle_size) {
  /*
   * The order in which these locations are written does not matter:
   * none of the locations will be reachable, because the bundle heads
   * still contains HLTs.
   */
  int       bundle_mask = bundle_size - 1;
  uint32_t  *src_ptr;
  uint32_t  *dest_ptr;
  uint32_t  *end_ptr;

  CHECK(0 == ((uintptr_t) dest & 3));

  src_ptr = (uint32_t *) src;
  dest_ptr = (uint32_t *) dest;
  end_ptr = (uint32_t *) (dest + size);
  while (dest_ptr < end_ptr) {
    if ((((uintptr_t) dest_ptr) & bundle_mask) != 0) {
      *dest_ptr = *src_ptr;
    }
    dest_ptr++;
    src_ptr++;
  }
}

static void CopyBundleHeads(uint8_t  *dest,
                            uint8_t  *src,
                            uint32_t size,
                            int      bundle_size) {
  /* Again, the order in which these locations are written does not matter. */
  uint8_t *src_ptr;
  uint8_t *dest_ptr;
  uint8_t *end_ptr;

  /* dest must be aligned for the writes to be atomic. */
  CHECK(0 == ((uintptr_t) dest & 3));

  src_ptr = src;
  dest_ptr = dest;
  end_ptr = dest + size;
  while (dest_ptr < end_ptr) {
    /*
     * We assume that writing the 32-bit int here is atomic, which is
     * the case on x86 and ARM as long as the address is word-aligned.
     * The read does not have to be atomic.
     */
    *(uint32_t *) dest_ptr = *(uint32_t *) src_ptr;
    dest_ptr += bundle_size;
    src_ptr += bundle_size;
  }
}

static void ReplaceBundleHeadsWithHalts(uint8_t  *dest,
                                        uint32_t size,
                                        int      bundle_size) {
  uint32_t *dest_ptr = (uint32_t*) dest;
  uint32_t *end_ptr = (uint32_t*) (dest + size);
  while (dest_ptr < end_ptr) {
    /* dont assume 1-byte halt, write entire NACL_HALT_WORD */
    *dest_ptr = NACL_HALT_WORD;
    dest_ptr += bundle_size / sizeof(uint32_t);
  }
  NaClWriteMemoryBarrier();
}

static INLINE void CopyCodeSafelyInitial(uint8_t  *dest,
                                  uint8_t  *src,
                                  uint32_t size,
                                  int      bundle_size) {
  CopyBundleTails(dest, src, size, bundle_size);
  NaClWriteMemoryBarrier();
  CopyBundleHeads(dest, src, size, bundle_size);
}

static void MakeDynamicCodePagesVisible(struct NaClApp *nap,
                                        uint32_t page_index_min,
                                        uint32_t page_index_max,
                                        uint8_t *writable_addr) {
  void *user_addr;
  uint32_t index;
  size_t size = (page_index_max - page_index_min) * NACL_MAP_PAGESIZE;

  for (index = page_index_min; index < page_index_max; index++) {
    CHECK(!BitmapIsBitSet(nap->dynamic_page_bitmap, index));
    BitmapSetBit(nap->dynamic_page_bitmap, index);
  }
  user_addr = (void *) NaClUserToSys(nap, nap->dynamic_text_start
                                     + page_index_min * NACL_MAP_PAGESIZE);

#if NACL_WINDOWS
  NaClUntrustedThreadsSuspendAll(nap, /* save_registers= */ 0);

  /*
   * The VirtualAlloc() call here has two effects:
   *
   *  1) It commits the page in the shared memory (SHM) object,
   *     allocating swap space and making the page accessible.  This
   *     affects our writable mapping of the shared memory object too.
   *     Before the VirtualAlloc() call, dereferencing writable_addr
   *     would fault.
   *  2) It changes the page permissions of the mapping to
   *     read+execute.  Since this exposes the page in its unsafe,
   *     non-HLT-filled state, this must be done with untrusted
   *     threads suspended.
   */
  {
    uintptr_t offset;
    for (offset = 0; offset < size; offset += NACL_MAP_PAGESIZE) {
      void *user_page_addr = (char *) user_addr + offset;
      if (VirtualAlloc(user_page_addr, NACL_MAP_PAGESIZE,
                       MEM_COMMIT, PAGE_EXECUTE_READ) != user_page_addr) {
        NaClLog(LOG_FATAL, "MakeDynamicCodePagesVisible: "
                "VirtualAlloc() failed -- probably out of swap space\n");
      }
    }
  }
#endif

  /* Sanity check:  Ensure the page is not already in use. */
  CHECK(*writable_addr == 0);

  NaClFillMemoryRegionWithHalt(writable_addr, size);

#if NACL_WINDOWS
  NaClUntrustedThreadsResumeAll(nap);
#else
  if (NaClMprotect(user_addr, size, PROT_READ | PROT_EXEC) != 0) {
    NaClLog(LOG_FATAL, "MakeDynamicCodePageVisible: NaClMprotect() failed\n");
  }
#endif
}

/*
 * Maps a writable version of the code at [offset, offset+size) and returns a
 * pointer to the new mapping. Internally caches the last mapping between
 * calls. Pass offset=0,size=0 to clear cache.
 * Caller must hold nap->dynamic_load_mutex.
 */
static uintptr_t CachedMapWritableText(struct NaClApp *nap,
                                       uint32_t offset,
                                       uint32_t size) {
  /*
   * The nap->* variables used in this function can be in two states:
   *
   * 1)
   * nap->dynamic_mapcache_size == 0
   * nap->dynamic_mapcache_ret == 0
   *
   * Initial state, nothing is cached.
   *
   * 2)
   * nap->dynamic_mapcache_size != 0
   * nap->dynamic_mapcache_ret != 0
   *
   * We have a cached mmap result stored, that must be unmapped.
   */
  struct NaClDesc            *shm = nap->text_shm;

  if (offset != nap->dynamic_mapcache_offset
          || size != nap->dynamic_mapcache_size) {
    /*
     * cache miss, first clear the old cache if needed
     */
    if (nap->dynamic_mapcache_size > 0) {
      NaClHostDescUnmapUnsafe((void *) nap->dynamic_mapcache_ret,
                              nap->dynamic_mapcache_size);
      nap->dynamic_mapcache_offset = 0;
      nap->dynamic_mapcache_size = 0;
      nap->dynamic_mapcache_ret = 0;
    }

    /*
     * update that cached version
     */
    if (size > 0) {
      uint32_t current_page_index;
      uint32_t end_page_index;

      uintptr_t mapping = (*((struct NaClDescVtbl const *)
            shm->base.vtbl)->
              Map)(shm,
                   NaClDescEffectorTrustedMem(),
                   NULL,
                   size,
                   NACL_ABI_PROT_READ | NACL_ABI_PROT_WRITE,
                   NACL_ABI_MAP_SHARED,
                   offset);
      if (NaClPtrIsNegErrno(&mapping)) {
        return 0;
      }

      /*
       * To reduce the number of mprotect() system calls, we coalesce
       * MakeDynamicCodePagesVisible() calls for adjacent pages that
       * have yet not been allocated.
       */
      current_page_index = offset / NACL_MAP_PAGESIZE;
      end_page_index = (offset + size) / NACL_MAP_PAGESIZE;
      while (current_page_index < end_page_index) {
        uint32_t start_page_index = current_page_index;
        /* Find the end of this block of unallocated pages. */
        while (current_page_index < end_page_index &&
               !BitmapIsBitSet(nap->dynamic_page_bitmap, current_page_index)) {
          current_page_index++;
        }
        if (current_page_index > start_page_index) {
          uintptr_t writable_addr =
              mapping + (start_page_index * NACL_MAP_PAGESIZE - offset);
          MakeDynamicCodePagesVisible(nap, start_page_index, current_page_index,
                                      (uint8_t *) writable_addr);
        }
        current_page_index++;
      }

      nap->dynamic_mapcache_offset = offset;
      nap->dynamic_mapcache_size = size;
      nap->dynamic_mapcache_ret = mapping;
    }
  }
  return nap->dynamic_mapcache_ret;
}

/*
 * A wrapper around CachedMapWritableText that performs common address
 * calculations.
 * Outputs *mmapped_addr.
 * Caller must hold nap->dynamic_load_mutex.
 * Returns boolean, true on success
 */
static INLINE int NaClTextMapWrapper(struct NaClApp *nap,
                                    uint32_t dest,
                                    uint32_t size,
                                    uint8_t  **mapped_addr) {
  uint32_t  shm_offset;
  uint32_t  shm_map_offset;
  uint32_t  within_page_offset;
  uint32_t  shm_map_offset_end;
  uint32_t  shm_map_size;
  uintptr_t mmap_ret;
  uint8_t   *mmap_result;

  shm_offset = dest - (uint32_t) nap->dynamic_text_start;
  shm_map_offset = shm_offset & ~(NACL_MAP_PAGESIZE - 1);
  within_page_offset = shm_offset & (NACL_MAP_PAGESIZE - 1);
  shm_map_offset_end =
    (shm_offset + size + NACL_MAP_PAGESIZE - 1) & ~(NACL_MAP_PAGESIZE - 1);
  shm_map_size = shm_map_offset_end - shm_map_offset;

  mmap_ret = CachedMapWritableText(nap,
                                   shm_map_offset,
                                   shm_map_size);
  if (0 == mmap_ret) {
    return 0;
  }
  mmap_result = (uint8_t *) mmap_ret;
  *mapped_addr = mmap_result + within_page_offset;
  return 1;
}

/*
 * Clear the mmap cache if multiple pages were mapped.
 * Caller must hold nap->dynamic_load_mutex.
 */
static INLINE void NaClTextMapClearCacheIfNeeded(struct NaClApp *nap,
                                                 uint32_t dest,
                                                 uint32_t size) {
  uint32_t                    shm_offset;
  uint32_t                    shm_map_offset;
  uint32_t                    shm_map_offset_end;
  uint32_t                    shm_map_size;
  shm_offset = dest - (uint32_t) nap->dynamic_text_start;
  shm_map_offset = shm_offset & ~(NACL_MAP_PAGESIZE - 1);
  shm_map_offset_end =
    (shm_offset + size + NACL_MAP_PAGESIZE - 1) & ~(NACL_MAP_PAGESIZE - 1);
  shm_map_size = shm_map_offset_end - shm_map_offset;
  if (shm_map_size > NACL_MAP_PAGESIZE) {
    /* call with size==offset==0 to clear cache */
    CachedMapWritableText(nap, 0, 0);
  }
}

int32_t NaClTextDyncodeCreate(struct NaClApp *nap,
                              uint32_t       dest,
                              void           *code_copy,
                              uint32_t       size,
                              const struct NaClValidationMetadata *metadata) {
  uintptr_t                   dest_addr;
  uint8_t                     *mapped_addr;
  int32_t                     retval = -NACL_ABI_EINVAL;
  int                         validator_result;
  struct NaClPerfCounter      time_dyncode_create;
  NaClPerfCounterCtor(&time_dyncode_create, "NaClTextDyncodeCreate");

  if (NULL == nap->text_shm) {
    NaClLog(1, "NaClTextDyncodeCreate: Dynamic loading not enabled\n");
    return -NACL_ABI_EINVAL;
  }
  if (0 != (dest & (nap->bundle_size - 1)) ||
      0 != (size & (nap->bundle_size - 1))) {
    NaClLog(1, "NaClTextDyncodeCreate: Non-bundle-aligned address or size\n");
    return -NACL_ABI_EINVAL;
  }
  dest_addr = NaClUserToSysAddrRange(nap, dest, size);
  if (kNaClBadAddress == dest_addr) {
    NaClLog(1, "NaClTextDyncodeCreate: Dest address out of range\n");
    return -NACL_ABI_EFAULT;
  }
  if (dest < nap->dynamic_text_start) {
    NaClLog(1, "NaClTextDyncodeCreate: Below dynamic code area\n");
    return -NACL_ABI_EFAULT;
  }
  /*
   * We ensure that the final HLTs of the dynamic code region cannot
   * be overwritten, just in case of CPU bugs.
   */
  if (dest + size > nap->dynamic_text_end - NACL_HALT_SLED_SIZE) {
    NaClLog(1, "NaClTextDyncodeCreate: Above dynamic code area\n");
    return -NACL_ABI_EFAULT;
  }
  if (0 == size) {
    /* Nothing to load.  Succeed trivially. */
    return 0;
  }

  NaClXMutexLock(&nap->dynamic_load_mutex);

  /*
   * Validate the code before trying to create the region.  This avoids the need
   * to delete the region if validation fails.
   * See: http://code.google.com/p/nativeclient/issues/detail?id=2566
   */
  if (!nap->skip_validator) {
    validator_result = NaClValidateCode(nap, dest, code_copy, size, metadata);
  } else {
    NaClLog(LOG_ERROR, "VALIDATION SKIPPED.\n");
    validator_result = LOAD_OK;
  }

  NaClPerfCounterMark(&time_dyncode_create,
                      NACL_PERF_IMPORTANT_PREFIX "DynRegionValidate");
  NaClPerfCounterIntervalLast(&time_dyncode_create);

  if (validator_result != LOAD_OK
      && nap->ignore_validator_result) {
    NaClLog(LOG_ERROR, "VALIDATION FAILED for dynamically-loaded code: "
            "continuing anyway...\n");
    validator_result = LOAD_OK;
  }

  if (validator_result != LOAD_OK) {
    NaClLog(1, "NaClTextDyncodeCreate: "
            "Validation of dynamic code failed\n");
    retval = -NACL_ABI_EINVAL;
    goto cleanup_unlock;
  }

  if (NaClDynamicRegionCreate(nap, dest_addr, size, 0) != 1) {
    /* target addr is in use */
    NaClLog(1, "NaClTextDyncodeCreate: Code range already allocated\n");
    retval = -NACL_ABI_EINVAL;
    goto cleanup_unlock;
  }

  if (!NaClTextMapWrapper(nap, dest, size, &mapped_addr)) {
    retval = -NACL_ABI_ENOMEM;
    goto cleanup_unlock;
  }

  CopyCodeSafelyInitial(mapped_addr, code_copy, size, nap->bundle_size);
  /*
   * Flush the processor's instruction cache.  This is not necessary
   * for security, because any old cached instructions will just be
   * safe halt instructions.  It is only necessary to ensure that
   * untrusted code runs correctly when it tries to execute the
   * dynamically-loaded code.
   */
  NaClFlushCacheForDoublyMappedCode(mapped_addr, (uint8_t *) dest_addr, size);

  retval = 0;

  NaClTextMapClearCacheIfNeeded(nap, dest, size);

 cleanup_unlock:
  NaClXMutexUnlock(&nap->dynamic_load_mutex);
  return retval;
}

int32_t NaClSysDyncodeCreate(struct NaClAppThread *natp,
                             uint32_t             dest,
                             uint32_t             src,
                             uint32_t             size) {
  struct NaClApp              *nap = natp->nap;
  uintptr_t                   src_addr;
  uint8_t                     *code_copy;
  int32_t                     retval = -NACL_ABI_EINVAL;

  if (!nap->enable_dyncode_syscalls) {
    NaClLog(LOG_WARNING,
            "NaClSysDyncodeCreate: Dynamic code syscalls are disabled\n");
    return -NACL_ABI_ENOSYS;
  }

  src_addr = NaClUserToSysAddrRange(nap, src, size);
  if (kNaClBadAddress == src_addr) {
    NaClLog(1, "NaClSysDyncodeCreate: Source address out of range\n");
    return -NACL_ABI_EFAULT;
  }

  /*
   * Make a private copy of the code, so that we can validate it
   * without a TOCTTOU race condition.
   */
  code_copy = malloc(size);
  if (NULL == code_copy) {
    return -NACL_ABI_ENOMEM;
  }
  memcpy(code_copy, (uint8_t*) src_addr, size);

  /* Unknown data source, no metadata. */
  retval = NaClTextDyncodeCreate(nap, dest, code_copy, size, NULL);

  free(code_copy);
  return retval;
}

int32_t NaClSysDyncodeModify(struct NaClAppThread *natp,
                             uint32_t             dest,
                             uint32_t             src,
                             uint32_t             size) {
  struct NaClApp              *nap = natp->nap;
  uintptr_t                   dest_addr;
  uintptr_t                   src_addr;
  uintptr_t                   beginbundle;
  uintptr_t                   endbundle;
  uintptr_t                   offset;
  uint8_t                     *mapped_addr;
  uint8_t                     *code_copy = NULL;
  uint8_t                     code_copy_buf[NACL_INSTR_BLOCK_SIZE];
  int                         validator_result;
  int32_t                     retval = -NACL_ABI_EINVAL;
  struct NaClDynamicRegion    *region;

  if (!nap->validator->code_replacement) {
    NaClLog(LOG_WARNING,
            "NaClSysDyncodeModify: "
            "Dynamic code modification is not supported\n");
    return -NACL_ABI_ENOSYS;
  }

  if (!nap->enable_dyncode_syscalls) {
    NaClLog(LOG_WARNING,
            "NaClSysDyncodeModify: Dynamic code syscalls are disabled\n");
    return -NACL_ABI_ENOSYS;
  }

  if (NULL == nap->text_shm) {
    NaClLog(1, "NaClSysDyncodeModify: Dynamic loading not enabled\n");
    return -NACL_ABI_EINVAL;
  }

  if (0 == size) {
    /* Nothing to modify.  Succeed trivially. */
    return 0;
  }

  dest_addr = NaClUserToSysAddrRange(nap, dest, size);
  src_addr = NaClUserToSysAddrRange(nap, src, size);
  if (kNaClBadAddress == src_addr || kNaClBadAddress == dest_addr) {
    NaClLog(1, "NaClSysDyncodeModify: Address out of range\n");
    return -NACL_ABI_EFAULT;
  }

  NaClXMutexLock(&nap->dynamic_load_mutex);

  region = NaClDynamicRegionFind(nap, dest_addr, size);
  if (NULL == region ||
      region->start > dest_addr ||
      region->start + region->size < dest_addr + size ||
      region->is_mmap) {
    /*
     * target not a subregion of region or region is null, or came from a file.
     */
    NaClLog(1, "NaClSysDyncodeModify: Can't find region to modify\n");
    retval = -NACL_ABI_EFAULT;
    goto cleanup_unlock;
  }

  beginbundle = dest_addr & ~(nap->bundle_size - 1);
  endbundle   = (dest_addr + size - 1 + nap->bundle_size)
                  & ~(nap->bundle_size - 1);
  offset      = dest_addr &  (nap->bundle_size - 1);
  if (endbundle-beginbundle <= sizeof code_copy_buf) {
    /* usually patches are a single bundle, so stack allocate */
    code_copy = code_copy_buf;
  } else {
    /* in general case heap allocate */
    code_copy = malloc(endbundle-beginbundle);
    if (NULL == code_copy) {
      retval = -NACL_ABI_ENOMEM;
      goto cleanup_unlock;
    }
  }

  /* copy the bundles from already-inserted code */
  memcpy(code_copy, (uint8_t*) beginbundle, endbundle - beginbundle);

  /*
   * make the requested change in temporary location
   * this avoids TOTTOU race
   */
  memcpy(code_copy + offset, (uint8_t*) src_addr, size);

  /* update dest/size to refer to entire bundles */
  dest      &= ~(nap->bundle_size - 1);
  dest_addr &= ~((uintptr_t)nap->bundle_size - 1);
  /* since both are in sandbox memory this check should succeed */
  CHECK(endbundle-beginbundle < UINT32_MAX);
  size = (uint32_t)(endbundle - beginbundle);

  /* validate this code as a replacement */
  validator_result = NaClValidateCodeReplacement(nap,
                                                 dest,
                                                 (uint8_t*) dest_addr,
                                                 code_copy,
                                                 size);

  if (validator_result != LOAD_OK
      && nap->ignore_validator_result) {
    NaClLog(LOG_ERROR, "VALIDATION FAILED for dynamically-loaded code: "
                       "continuing anyway...\n");
    validator_result = LOAD_OK;
  }

  if (validator_result != LOAD_OK) {
    NaClLog(1, "NaClSysDyncodeModify: Validation of dynamic code failed\n");
    retval = -NACL_ABI_EINVAL;
    goto cleanup_unlock;
  }

  if (!NaClTextMapWrapper(nap, dest, size, &mapped_addr)) {
    retval = -NACL_ABI_ENOMEM;
    goto cleanup_unlock;
  }

  if (LOAD_OK != NaClCopyCode(nap, dest, mapped_addr, code_copy, size)) {
    NaClLog(1, "NaClSysDyncodeModify: Copying of replacement code failed\n");
    retval = -NACL_ABI_EINVAL;
    goto cleanup_unlock;
  }
  retval = 0;

  NaClTextMapClearCacheIfNeeded(nap, dest, size);

 cleanup_unlock:
  NaClXMutexUnlock(&nap->dynamic_load_mutex);

  if (code_copy != code_copy_buf) {
    free(code_copy);
  }

  return retval;
}

int32_t NaClSysDyncodeDelete(struct NaClAppThread *natp,
                             uint32_t             dest,
                             uint32_t             size) {
  struct NaClApp              *nap = natp->nap;
  uintptr_t                    dest_addr;
  uint8_t                     *mapped_addr;
  int32_t                     retval = -NACL_ABI_EINVAL;
  struct NaClDynamicRegion    *region;

  if (!nap->enable_dyncode_syscalls) {
    NaClLog(LOG_WARNING,
            "NaClSysDyncodeDelete: Dynamic code syscalls are disabled\n");
    return -NACL_ABI_ENOSYS;
  }

  if (NULL == nap->text_shm) {
    NaClLog(1, "NaClSysDyncodeDelete: Dynamic loading not enabled\n");
    return -NACL_ABI_EINVAL;
  }

  if (0 == size) {
    /* Nothing to delete.  Just update our generation. */
    int gen;
    /* fetch current generation */
    NaClXMutexLock(&nap->dynamic_load_mutex);
    gen = nap->dynamic_delete_generation;
    NaClXMutexUnlock(&nap->dynamic_load_mutex);
    /* set our generation */
    NaClSetThreadGeneration(natp, gen);
    return 0;
  }

  dest_addr = NaClUserToSysAddrRange(nap, dest, size);
  if (kNaClBadAddress == dest_addr) {
    NaClLog(1, "NaClSysDyncodeDelete: Address out of range\n");
    return -NACL_ABI_EFAULT;
  }

  NaClXMutexLock(&nap->dynamic_load_mutex);

  /*
   * this check ensures the to-be-deleted region is identical to a
   * previously inserted region, so no need to check for alignment/bounds/etc
   */
  region = NaClDynamicRegionFind(nap, dest_addr, size);
  if (NULL == region ||
      region->start != dest_addr ||
      region->size != size ||
      region->is_mmap) {
    NaClLog(1, "NaClSysDyncodeDelete: Can't find region to delete\n");
    retval = -NACL_ABI_EFAULT;
    goto cleanup_unlock;
  }


  if (region->delete_generation < 0) {
    /* first deletion request */

    if (nap->dynamic_delete_generation == INT32_MAX) {
      NaClLog(1, "NaClSysDyncodeDelete:"
                 "Overflow, can only delete INT32_MAX regions\n");
      retval = -NACL_ABI_EFAULT;
      goto cleanup_unlock;
    }

    if (!NaClTextMapWrapper(nap, dest, size, &mapped_addr)) {
      retval = -NACL_ABI_ENOMEM;
      goto cleanup_unlock;
    }

    /* make it so no new threads can enter target region */
    ReplaceBundleHeadsWithHalts(mapped_addr, size, nap->bundle_size);

    /*
     * Flush the instruction cache.  In principle this is needed for
     * security on ARM so that, when new code is loaded, it is not
     * possible for it to jump to stale code that remains in the
     * icache.
     */
    NaClFlushCacheForDoublyMappedCode(mapped_addr, (uint8_t *) dest_addr, size);

    NaClTextMapClearCacheIfNeeded(nap, dest, size);

    /* increment and record the generation deletion was requested */
    region->delete_generation = ++nap->dynamic_delete_generation;
  }

  /* update our own generation */
  NaClSetThreadGeneration(natp, nap->dynamic_delete_generation);

  if (region->delete_generation <= NaClMinimumThreadGeneration(nap)) {
    /*
     * All threads have checked in since we marked region for deletion.
     * It is safe to remove the region.
     *
     * No need to memset the region to hlt since bundle heads are hlt
     * and thus the bodies are unreachable.
     */
    NaClDynamicRegionDelete(nap, region);
    retval = 0;
  } else {
    /*
     * Still waiting for some threads to report in...
     */
    retval = -NACL_ABI_EAGAIN;
  }

 cleanup_unlock:
  NaClXMutexUnlock(&nap->dynamic_load_mutex);
  return retval;
}

void NaClDyncodeVisit(
    struct NaClApp *nap,
    void           (*fn)(void *state, struct NaClDynamicRegion *region),
    void           *state) {
  int            i;

  NaClXMutexLock(&nap->dynamic_load_mutex);
  for (i = 0; i < nap->num_dynamic_regions; ++i) {
    fn(state, &nap->dynamic_regions[i]);
  }
  NaClXMutexUnlock(&nap->dynamic_load_mutex);
}
