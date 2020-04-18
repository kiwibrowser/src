/*
 * Copyright (c) 2011 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */
#ifndef NATIVE_CLIENT_SRC_INCLUDE_NACL_ATOMIC_OPS_H_
#define NATIVE_CLIENT_SRC_INCLUDE_NACL_ATOMIC_OPS_H_ 1

#include "native_client/src/include/portability.h"
#include <stdint.h>

typedef int32_t Atomic32;

static INLINE Atomic32 CompareAndSwap(volatile Atomic32* ptr,
                                      Atomic32 old_value,
                                      Atomic32 new_value) {
  return __sync_val_compare_and_swap(ptr, old_value, new_value);
}

static INLINE Atomic32 AtomicExchange(volatile Atomic32* ptr,
                                      Atomic32 new_value) {
  return __sync_lock_test_and_set(ptr, new_value);
}

static INLINE Atomic32 AtomicIncrement(volatile Atomic32* ptr,
                                       Atomic32 increment) {
  return __sync_add_and_fetch(ptr, increment);
}

#endif  /* NATIVE_CLIENT_SRC_INCLUDE_NACL_ATOMIC_OPS_H_ */
