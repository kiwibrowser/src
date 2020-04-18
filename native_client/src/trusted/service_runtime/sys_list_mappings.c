/*
 * Copyright (c) 2013 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * NaCl service run-time, list_mappings system call.
 */

#include <stdlib.h>

#include "native_client/src/trusted/service_runtime/sys_list_mappings.h"

#include "native_client/src/include/nacl_platform.h"
#include "native_client/src/include/portability_string.h"
#include "native_client/src/shared/platform/nacl_sync_checked.h"
#include "native_client/src/trusted/desc/nacl_desc_io.h"
#include "native_client/src/trusted/service_runtime/include/bits/mman.h"
#include "native_client/src/trusted/service_runtime/include/sys/errno.h"
#include "native_client/src/trusted/service_runtime/include/sys/nacl_list_mappings.h"
#include "native_client/src/trusted/service_runtime/nacl_app_thread.h"
#include "native_client/src/trusted/service_runtime/nacl_copy.h"
#include "native_client/src/trusted/service_runtime/nacl_text.h"
#include "native_client/src/trusted/service_runtime/sel_ldr.h"

struct NaClSysListMappingsState {
  struct NaClApp *nap;
  struct NaClMemMappingInfo *regions;
  uint32_t count;
  uint32_t capacity;
  int out_of_memory;
};

static const int kStartCapacity = 8;

static void NaClSysListMappingsAdd(struct NaClSysListMappingsState *state,
                                   uint32_t start,
                                   uint32_t size,
                                   uint32_t prot,
                                   uint32_t max_prot,
                                   uint32_t vmmap_type) {
  struct NaClMemMappingInfo *info;
  uint32_t new_capacity;

  if (state->out_of_memory) {
    return;
  }
  if (state->count == state->capacity) {
    if (state->capacity == 0) {
      new_capacity = kStartCapacity;
    } else {
      new_capacity = state->capacity * 2;
      if (UINT32_MAX / sizeof(*state->regions) < new_capacity) {
        /* state->capacity * sizeof(*state->regions) would overflow. */
        state->out_of_memory = 1;
        return;
      }
    }
    info = (struct NaClMemMappingInfo *) realloc(
        state->regions, new_capacity * sizeof(*state->regions));
    if (info == NULL) {
      state->out_of_memory = 1;
      return;
    }
    state->capacity = new_capacity;
    state->regions = info;
  }
  info = &state->regions[state->count];
  ++state->count;

  memset(info, 0, sizeof(*info));
  info->start = start;
  info->size = size;
  info->prot = prot;
  info->max_prot = max_prot;
  info->vmmap_type = vmmap_type;
}

static int NaClSysListMappingsOrder(const void *a, const void *b) {
  const struct NaClMemMappingInfo *am = (const struct NaClMemMappingInfo *) a;
  const struct NaClMemMappingInfo *bm = (const struct NaClMemMappingInfo *) b;

  if (am->start < bm->start) return -1;
  if (am->start > bm->start) return 1;
  return 0;
}

static void NaClSysListMappingsVisit(void *statev,
                                     struct NaClVmmapEntry *vmep) {
  struct NaClSysListMappingsState *state =
      (struct NaClSysListMappingsState *) statev;

  uint32_t start = (uint32_t) (vmep->page_num << NACL_PAGESHIFT);
  uint32_t size = (uint32_t) (vmep->npages << NACL_PAGESHIFT);
  uint32_t max_prot = NaClVmmapEntryMaxProt(vmep);
  /* Skip dynamic code region as its parts will be visited separately. */
  if (state->nap->dynamic_text_start == start &&
      state->nap->dynamic_text_end == start + size) {
    return;
  }
  NaClSysListMappingsAdd(
      state,
      /* start= */ start,
      /* size= */ size,
      /* prot= */ vmep->prot,
      /* max_prot= */ max_prot,
      /* vmmap_type= */ vmep->desc != NULL);
}

static void NaClSysListMappingsDyncodeVisit(void *statev,
                                            struct NaClDynamicRegion *rg) {
  struct NaClSysListMappingsState *state =
      (struct NaClSysListMappingsState *) statev;

  NaClSysListMappingsAdd(
      state,
      /* start= */ (uint32_t) NaClSysToUser(state->nap, rg->start),
      /* size= */ (uint32_t) rg->size,
      /* prot= */ NACL_ABI_PROT_READ | NACL_ABI_PROT_EXEC,
      /* max_prot= */ NACL_ABI_PROT_READ | NACL_ABI_PROT_EXEC,
      /* vmmap_type= */ 0);
}

int32_t NaClSysListMappings(struct NaClAppThread *natp,
                            uint32_t regions,
                            uint32_t count) {
  struct NaClApp *nap = natp->nap;
  struct NaClSysListMappingsState state;

  if (!nap->enable_list_mappings) {
    return -NACL_ABI_ENOSYS;
  }

  state.nap = nap;
  state.count = 0;
  state.capacity = 0;
  state.out_of_memory = 0;
  state.regions = NULL;

  NaClXMutexLock(&nap->mu);
  NaClVmmapVisit(&nap->mem_map, NaClSysListMappingsVisit, &state);
  NaClDyncodeVisit(nap, NaClSysListMappingsDyncodeVisit, &state);
  NaClXMutexUnlock(&nap->mu);

  if (state.out_of_memory) {
    NaClLog(3, "Out of memory while gathering memory map\n");
    return -NACL_ABI_ENOMEM;
  }

  qsort(state.regions, state.count, sizeof(*state.regions),
        NaClSysListMappingsOrder);

  if (state.count < count) {
    count = state.count;
  }
  if (!NaClCopyOutToUser(nap, regions, state.regions,
                         sizeof(*state.regions) * count)) {
    NaClLog(3, "Illegal address for ListMappings at 0x%08"NACL_PRIxPTR"\n",
            (uintptr_t) regions);
    return -NACL_ABI_EFAULT;
  }
  free(state.regions);

  return state.count;
}
