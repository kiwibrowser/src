/*
 * Copyright 2008 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */


/* @file
 *
 * NaCl utility for a dynamically sized array where the first unused
 * position can be discovered quickly.  This is used for I/O
 * descriptors -- the number of elements in an array is typically not
 * large, and the common access is finding an entry by its index, with
 * occasional need to allocate the last numbered slot for a new I/O
 * descriptor.
 *
 * A dynamic array containing void pointers is created using
 * DynArrayCtor in a placement-new manner with the initial size as
 * paramter.  The caller is responsible for the memory.  The dynamic
 * array is destroyed with DynArrayDtor, which destroys the contents
 * of the dynamic array; again, the caller is responsible for
 * destroying the DynArray object itself.  Since the array contains
 * void pointers, if these pointers should be in turn freed or have
 * destructors invoked on them, it is the responsible of the user to
 * do so.
 *
 * Accessors DynArrayGet and DynArraySet does what you would expect.
 * DynArrayFirstAvail returns the index of the first slot that is
 * unused.  Note that DynArraySet will grow the array as needed to set
 * the element, even if the value is a NULL pointer.  Such an entry is
 * still considerd to be unused.
 */

#ifndef SERVICE_RUNTIME_DYN_ARRAY_H__
#define SERVICE_RUNTIME_DYN_ARRAY_H__ 1

#include "native_client/src/include/portability.h"

struct DynArray {
  /* public */
  size_t    num_entries;
  /*
   * note num_entries has a somewhat unusual property.  if valid
   * entries are written to index 10, the num_entries is 11 even if
   * there have never been any writes to indices 0...9.
   */

  /* protected */
  void      **ptr_array;  /* we *could* sort/bsearch this */
  size_t    ptr_array_space;
  uint32_t  *available;
  size_t    avail_ix;
};

int DynArrayCtor(struct DynArray  *dap,
                 size_t           initial_size);

void DynArrayDtor(struct DynArray *dap);

void *DynArrayGet(struct DynArray *dap,
                  size_t          idx);

int DynArraySet(struct DynArray *dap,
                size_t          idx,
                void            *ptr);

size_t DynArrayFirstAvail(struct DynArray *dap);

#endif
