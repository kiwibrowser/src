/*
 * Copyright 2008 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * Implementation of dynamic arrays.
 */

#include "native_client/src/include/portability.h"

#define DYN_ARRAY_DEBUG 1

#include <stdlib.h>
#include <string.h>

#if DYN_ARRAY_DEBUG
# include "native_client/src/shared/platform/nacl_log.h"
#endif

#include "native_client/src/trusted/service_runtime/dyn_array.h"


static int const kBitsPerWord = 32;
static int const kWordIndexShift = 5;  /* 2**kWordIndexShift==kBitsPerWord */


static INLINE size_t BitsToAllocWords(size_t nbits) {
  return (nbits + kBitsPerWord - 1) >> kWordIndexShift;
}


static INLINE size_t BitsToIndex(size_t nbits) {
  return nbits >> kWordIndexShift;
}


static INLINE size_t BitsToOffset(size_t nbits) {
  return nbits & (kBitsPerWord - 1);
}


int DynArrayCtor(struct DynArray  *dap,
                 size_t           initial_size) {
  if (initial_size == 0) {
    initial_size = 32;
  }
  dap->num_entries = 0u;
  /* calloc should check internally, but we're paranoid */
  if (SIZE_T_MAX / sizeof *dap->ptr_array < initial_size ||
      SIZE_T_MAX/ sizeof *dap->available < BitsToAllocWords(initial_size)) {
    /* would integer overflow */
    return 0;
  }
  dap->ptr_array = calloc(initial_size, sizeof *dap->ptr_array);
  if (NULL == dap->ptr_array) {
    return 0;
  }
  dap->available = calloc(BitsToAllocWords(initial_size),
                          sizeof *dap->available);
  if (NULL == dap->available) {
    free(dap->ptr_array);
    dap->ptr_array = NULL;
    return 0;
  }
  dap->avail_ix = 0;  /* hint */

  dap->ptr_array_space = initial_size;
  return 1;
}


void DynArrayDtor(struct DynArray *dap) {
  dap->num_entries = 0;  /* assume user has freed entries */
  free(dap->ptr_array);
  dap->ptr_array = NULL;
  dap->ptr_array_space = 0;
  free(dap->available);
  dap->available = NULL;
}


void *DynArrayGet(struct DynArray *dap,
                  size_t          idx) {
  if (idx < dap->num_entries) {
    return dap->ptr_array[idx];
  }
  return NULL;
}


int DynArraySet(struct DynArray *dap,
                size_t          idx,
                void            *ptr) {
  size_t desired_space;
  size_t tmp;
  size_t ix;

  for (desired_space = dap->ptr_array_space;
       idx >= desired_space;
       desired_space = tmp) {
    tmp = 2 * desired_space;
    if (tmp < desired_space) {
      return 0;  /* failed */
    }
  }
  if (desired_space != dap->ptr_array_space) {
    /* need to grow */

    void      **new_space;
    uint32_t  *new_avail;
    size_t    new_avail_nwords;
    size_t    old_avail_nwords;

    new_space = realloc(dap->ptr_array, desired_space * sizeof *new_space);
    if (NULL == new_space) {
      return 0;
    }
    memset((void *) (new_space + dap->ptr_array_space),
           0,
           (desired_space - dap->ptr_array_space) * sizeof *new_space);
    dap->ptr_array = new_space;

    old_avail_nwords = BitsToAllocWords(dap->ptr_array_space);
    new_avail_nwords = BitsToAllocWords(desired_space);

    new_avail = realloc(dap->available,
                        new_avail_nwords * sizeof *new_avail);
    if (NULL == new_avail) {
      return 0;
    }
    memset((void *) &new_avail[old_avail_nwords],
           0,
           (new_avail_nwords - old_avail_nwords) * sizeof *new_avail);
    dap->available = new_avail;

    dap->ptr_array_space = desired_space;
  }
  dap->ptr_array[idx] = ptr;
  ix = BitsToIndex(idx);
#if DYN_ARRAY_DEBUG
  NaClLog(4, "Set(%"NACL_PRIuS",%p) @ix %"NACL_PRIuS": 0x%08x\n",
          idx, ptr, ix, dap->available[ix]);
#endif
  if (NULL != ptr) {
    dap->available[ix] |= (1 << BitsToOffset(idx));
  } else {
    dap->available[ix] &= ~(1 << BitsToOffset(idx));
    if (ix < dap->avail_ix) {
      dap->avail_ix = ix;
    }
  }
#if DYN_ARRAY_DEBUG
  NaClLog(4, "After @ix %"NACL_PRIuS": 0x%08x, avail_ix %"NACL_PRIuS"\n",
          ix, dap->available[ix], dap->avail_ix);
#endif
  if (dap->num_entries <= idx) {
    dap->num_entries = idx + 1;
  }
  return 1;
}


size_t DynArrayFirstAvail(struct DynArray *dap) {
  size_t ix;
  size_t last_ix;
  size_t avail_pos;

  last_ix = BitsToAllocWords(dap->ptr_array_space);

#if DYN_ARRAY_DEBUG
  for (ix = 0; ix < last_ix; ++ix) {
    NaClLog(4, "ix %"NACL_PRIuS": 0x%08x\n", ix, dap->available[ix]);
  }
#endif
  for (ix = dap->avail_ix; ix < last_ix; ++ix) {
    if (0U != ~dap->available[ix]) {
#if DYN_ARRAY_DEBUG
      NaClLog(4, "found first not-all-ones ix %"NACL_PRIuS"\n", ix);
#endif
      dap->avail_ix = ix;
      break;
    }
  }
  if (ix < last_ix) {
    avail_pos = ffs(~dap->available[ix]) - 1;
    avail_pos += ix << kWordIndexShift;
  } else {
    avail_pos = dap->ptr_array_space;
  }
  return avail_pos;
}
