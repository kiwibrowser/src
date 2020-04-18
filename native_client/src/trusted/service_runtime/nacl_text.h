/*
 * Copyright (c) 2011 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef NATIVE_CLIENT_SRC_TRUSTED_SERVICE_RUNTIME_NACL_TEXT_H_
#define NATIVE_CLIENT_SRC_TRUSTED_SERVICE_RUNTIME_NACL_TEXT_H_

#include "native_client/src/include/portability.h"
#include "native_client/src/include/nacl_compiler_annotations.h"
#include "native_client/src/trusted/service_runtime/nacl_error_code.h"

EXTERN_C_BEGIN

struct NaClApp;
struct NaClAppThread;

/*
 * If/when we decide to support munmap of validated text regions
 * into the dynamic code region, we will need another bit per page
 * to know whether a page is backed by shm or by an mmapped file --
 * i.e., if it was a file, we will need to restore the shm mapping
 * (this is costly, so should be avoided if it's unnecessary;
 * additionally, if it wasn't, the munmap should fail, and
 * conversely, dyncode_delete should fail on an mmapped region).
 */

struct NaClDynamicRegion {
  uintptr_t start;
  size_t size;
  int delete_generation;
  int is_mmap;  /* cannot be deleted (for now) */
};

/*
 * Insert a new region into nap->dynamic regions, maintaining the sorted
 * ordering. Returns 1 on success, 0 if there is a conflicting region
 * Caller must hold nap->dynamic_load_mutex.
 * Invalidates all previous NaClDynamicRegion pointers.
 *
 * is_mmap is 1 if the region is backed by a memory mapped file (and thus
 * the shared memory view was unmapped), 0 otherwise.
 */
int NaClDynamicRegionCreate(struct NaClApp *nap,
                            uintptr_t start,
                            size_t size,
                            int is_mmap);
/*
 * Find the last region overlapping with the given memory range, return 0 if
 * region is unused.
 * caller must hold nap->dynamic_load_mutex, and must discard result
 * when lock is released.
 */
struct NaClDynamicRegion* NaClDynamicRegionFind(struct NaClApp *nap,
                                                uintptr_t ptr,
                                                size_t size);

/*
 * Delete a region from nap->dynamic_regions, maintaining the sorted ordering
 * Caller must hold nap->dynamic_load_mutex.
 * Invalidates all previous NaClDynamicRegion pointers.
 */
void NaClDynamicRegionDelete(struct NaClApp *nap, struct NaClDynamicRegion* r);

struct NaClValidationMetadata;

/*
 * Create a shared memory descriptor and map it into the text region
 * of the address space.  This implies that the text size must be a
 * multiple of NACL_MAP_PAGESIZE.
 */
NaClErrorCode NaClMakeDynamicTextShared(struct NaClApp *nap) NACL_WUR;

struct NaClDescEffectorShm;
int NaClDescEffectorShmCtor(struct NaClDescEffectorShm *self) NACL_WUR;

int NaClMinimumThreadGeneration(struct NaClApp *nap);

int32_t NaClTextDyncodeCreate(
    struct NaClApp *nap,
    uint32_t       dest,
    void           *code_copy,
    uint32_t       size,
    const struct NaClValidationMetadata *metadata) NACL_WUR;

int32_t NaClSysDyncodeCreate(struct NaClAppThread *natp,
                             uint32_t             dest,
                             uint32_t             src,
                             uint32_t             size) NACL_WUR;

int32_t NaClSysDyncodeModify(struct NaClAppThread *natp,
                             uint32_t             dest,
                             uint32_t             src,
                             uint32_t             size) NACL_WUR;

int32_t NaClSysDyncodeDelete(struct NaClAppThread *natp,
                             uint32_t             dest,
                             uint32_t             size) NACL_WUR;

void NaClDyncodeVisit(
    struct NaClApp *nap,
    void           (*fn)(void *state, struct NaClDynamicRegion *region),
    void           *state);

EXTERN_C_END

#endif
