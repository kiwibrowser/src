/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * Interval Set.
 *
 * The interval set abstraction is any data structure that supports
 * the following operations:
 *
 * - Add or remove a closed interval in uint32_t to the set.  Such
 *   intervals may overlap.
 *
 * - Query whether an interval overlaps with any intervals already in
 *   the set.
 *
 * The first and simplest implementation will be a simple list, where
 * the Query operation would require complete list traversal.
 * Interval trees can also support the required operations, except
 * that since an interval set does not need to return the intervals
 * that overlaps with the probe interval, it can be simpler.
 */

#ifndef NATIVE_CLIENT_SRC_TRUSTED_INTERVAL_MULTISET_NACL_INTERVAL_MULTISET_H_
#define NATIVE_CLIENT_SRC_TRUSTED_INTERVAL_MULTISET_NACL_INTERVAL_MULTISET_H_

#include "native_client/src/include/nacl_base.h"
#include "native_client/src/include/nacl_compiler_annotations.h"
#include "native_client/src/include/portability.h"

/*
 * This file contains the methods for accessing an abstract data
 * structure, the "Interval Multiset".  There are more than one
 * implementations of interval multisets, and we follow the NaCl TCB
 * placement-new style of constructors -- the subclass-specific Ctor
 * expects the caller to have allocated memory (possibly on-stack,
 * automatic storage, as well as heap allocated), and the Ctor
 * function constructs the object in-place.  The subclass-specific
 * Ctors are declared in the subclass-specific header files.
 *
 * Since we want to be able to insulate the users from knowledge about
 * the object sizes (actual subclasses in use), we also provide a
 * function following the factory pattern: NaClIntervalSetFactory
 * takes a string that names the particular subclass desired, and it
 * will (heap) allocate the appropriately sized memory and invoke the
 * appropriate Ctor on it.  A corresponding NaClIntervalSetDelete
 * method handles the invocation of the Dtor and freeing the
 * Factory-allocated memory.
 *
 * An interval multiset is used to keep track of closed intervals
 * [first_val, last_val].  An interval multiset contains a multiset of
 * these closed intervals, which may overlap.  The basic operations
 * are to add and remove closed intervals to/from the multiset.  Given
 * an interval multiset, it is also possible to query if a given
 * closed interval overlaps with any intervals currently in the
 * multiset.  (Different implementations of interval multisets have
 * different performance characteristics.)
 *
 * We use closed intervals so that, for the use of keeping track of
 * "in use" memory regions in a NaCl module's 32-bit address space,
 * uint32_t values can describe all possible intervals.  Note,
 * however, that gaps between intervals are open intervals, and that
 * it is impossible/illegal to have a zero-length interval.  first_val
 * <= last_val must hold.
 */

EXTERN_C_BEGIN

struct NaClIntervalMultisetVtbl;

/* base class */
struct NaClIntervalMultiset {
  struct NaClIntervalMultisetVtbl const *vtbl;
};

struct NaClIntervalMultiset *NaClIntervalMultisetFactory(char const *kind);
void NaClIntervalMultisetDelete(struct NaClIntervalMultiset *obj);

struct NaClIntervalMultisetVtbl {
  void (*Dtor)(struct NaClIntervalMultiset *self);

  /*
   * Insert the closed interval [first_val, last_val] to interval
   * multiset.  Aborts via LOG_FATAL if out of memory.
   */
  void (*AddInterval)(struct NaClIntervalMultiset *self,
                      uint32_t first_val, uint32_t last_val);

  /*
   * Remove the specified closed interval.  The result is UNDEFINED if
   * the interval [first_val, last_val] is not in the interval multiset.
   */
  void (*RemoveInterval)(struct NaClIntervalMultiset *self,
                         uint32_t first_val, uint32_t last_val);

  /*
   * Boolean predicate.  Might rebalance tree, etc.  Returns \exists x
   * \in self: x.left <= last_val \hat first_val <= x.right (i.e., the
   * intervals overlap).
   */
  int (*OverlapsWith)(struct NaClIntervalMultiset *self,
                      uint32_t first_val, uint32_t last_val) NACL_WUR;
};

EXTERN_C_END

#endif
