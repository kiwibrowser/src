/*
 * Copyright (c) 2013 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * This file defines a set of function for performing atomic accesses on
 * memory locations according to the GCCMM library interface, providing
 * library support for the C11/C++11 atomics.
 *
 * See: http://gcc.gnu.org/wiki/Atomic/GCCMM/LIbrary
 *
 * Clang often emits atomic instructions instead of library calls when
 * user code does atomics (e.g. by using the ``__sync_*``
 * primitives). Nonetheless library calls to the functions implemented
 * in this file are emitted by Clang in certain circumstances, and
 * user/library code sometimes calls these functions directly. This file
 * is linked into user code before the PNaCl toolchain creates a
 * portable executable, and implements GCCMM's functions as calls to
 * PNaCl's atomic intrinsics. Failing to link with this file would lead
 * to undefined reference errors at link time.
 *
 * This file doesn't assume that the underlying NaCl atomic intrinsics
 * are/aren't lock-free: it merely punts to the translator which may
 * implement some of the supported sizes as lock-free.
 *
 * Note that similar functions may be implemented by backends which
 * don't support some lockless atomic accesses, e.g. MIPS for 64-bit
 * atomics. These are entirely separate from this file and should not be
 * confused.
 *
 * TODO(jfb) We currently don't handle 16-bit atomics or wider at all in
 *           the PNaCl ABI. Implementing these here requires care
 *           because C11/C++11 atomics that are implemented with a lock
 *           all need to go through the same lock (or locks, if
 *           sharded), and this requires a contract with Clang's code
 *           generation if we are to implement these in the current
 *           file. Implementing them here is also not forward-looking
 *           because hardware may support lock-free accesses for these
 *           sizes.
 */

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>

/* Clang complains if we redefine builtins directly. */
#pragma redefine_extname __pnacl_atomic_load __atomic_load
#pragma redefine_extname __pnacl_atomic_store __atomic_store
#pragma redefine_extname __pnacl_atomic_exchange __atomic_exchange
#pragma redefine_extname __pnacl_atomic_compare_exchange \
  __atomic_compare_exchange

/*
 * Memory order from GCCMM.
 */
enum AtomicOrderingKind {
  AO_ABI_memory_order_relaxed = 0,
  AO_ABI_memory_order_consume = 1,
  AO_ABI_memory_order_acquire = 2,
  AO_ABI_memory_order_release = 3,
  AO_ABI_memory_order_acq_rel = 4,
  AO_ABI_memory_order_seq_cst = 5
};

/*
 * Part of the stable ABI from llvm/IR/NaClAtomicIntrinsics.h.
 */
enum AtomicRMWOperation {
  AtomicInvalid = 0, /* Invalid, keep first. */
  AtomicAdd,
  AtomicSub,
  AtomicOr,
  AtomicAnd,
  AtomicXor,
  AtomicExchange,
  AtomicNum /* Invalid, keep last. */
};
enum MemoryOrder {
  MemoryOrderInvalid = 0, /* Invalid, keep first. */
  MemoryOrderRelaxed,
  MemoryOrderConsume,
  MemoryOrderAcquire,
  MemoryOrderRelease,
  MemoryOrderAcquireRelease,
  MemoryOrderSequentiallyConsistent,
  MemoryOrderNum /* Invalid, keep last. */
};

/*
 * Map GCCMM's memory order (also used by LLVM) to NaCl's stable ABI
 * equivalent.
 */
static enum MemoryOrder map_mem(enum AtomicOrderingKind order) {
  /*
   * TODO(jfb) For now PNaCl only supports sequentially-consistent.
   *           When this changes we should map AO_ABI_(.*) to MemoryOrder\1.
   */
  (void) order;
  return MemoryOrderSequentiallyConsistent;
}

/*
 * All the atomic sizes currently handled by PNaCl.
 * Invokes DO_ATOMIC(BITS, BYTES).
 */
#define DO_FOR_ALL_ATOMIC_SIZES()               \
  DO_ATOMIC(8, 1)                               \
  DO_ATOMIC(16, 2)                              \
  DO_ATOMIC(32, 4)                              \
  DO_ATOMIC(64, 8)

/* Convenience type definitions. */
#define DO_ATOMIC(BITS, BYTES)                  \
  typedef uint##BITS##_t I##BYTES;
DO_FOR_ALL_ATOMIC_SIZES()
#undef DO_ATOMIC

/*
 * NaCl atomic intrinsics ABI from llvm/IR/Intrinsics.td:
 *  __llvm_nacl_atomic_{load,store,rmw,cmpxchg}_{1,2,4,8}
 * Used by the below optimized sized functions.
 * Note: these expect a memory order from the stable ABI.
 */
#define DO_ATOMIC(BITS, BYTES)                                  \
  I##BYTES                                                      \
  __llvm_nacl_atomic_load_##BYTES(                              \
      I##BYTES *mem, int model)                                 \
  asm("llvm.nacl.atomic.load.i" #BITS);                         \
  void                                                          \
  __llvm_nacl_atomic_store_##BYTES(                             \
      I##BYTES val, I##BYTES *mem, int model)                   \
      asm("llvm.nacl.atomic.store.i" #BITS);                    \
  I##BYTES                                                      \
  __llvm_nacl_atomic_rmw_##BYTES(                               \
      int op, I##BYTES *mem, I##BYTES val, int model)           \
  asm("llvm.nacl.atomic.rmw.i" #BITS);                          \
  I##BYTES                                                      \
  __llvm_nacl_atomic_cmpxchg_##BYTES(                           \
      I##BYTES *mem, I##BYTES expected, I##BYTES desired,       \
      int success, int failure)                                 \
  asm("llvm.nacl.atomic.cmpxchg.i" #BITS);
DO_FOR_ALL_ATOMIC_SIZES()
#undef DO_ATOMIC

/*
 * GCCMM optimized sized functions:
 *  - __atomic_fetch_{add,sub,or,and,xor}_{1,2,4,8}.
 *  - __atomic_{load,store,exchange,compare_exchange}_{1,2,4,8}.
 * User/library accessible, and used by some of the generic function below.
 */
#define DO_ATOMIC_RMW(BITS, BYTES, OP, OPCASE)          \
  I##BYTES                                              \
  __atomic_fetch_##OP##_##BYTES(                        \
      I##BYTES *mem, I##BYTES val, int model) {         \
    return __llvm_nacl_atomic_rmw_##BYTES(              \
        Atomic##OPCASE, mem, val, map_mem(model));      \
  }
#define DO_ATOMIC(BITS, BYTES)                                          \
  I##BYTES                                                              \
  __atomic_load_##BYTES(                                                \
      I##BYTES *mem, int model) {                                       \
    return __llvm_nacl_atomic_load_##BYTES(mem, map_mem(model));        \
  }                                                                     \
  void                                                                  \
  __atomic_store_##BYTES(                                               \
      I##BYTES *mem, I##BYTES val, int model) {                         \
    __llvm_nacl_atomic_store_##BYTES(val, mem, map_mem(model));         \
  }                                                                     \
  I##BYTES                                                              \
  __atomic_exchange_##BYTES(                                            \
      I##BYTES *mem, I##BYTES val, int model) {                         \
    return __llvm_nacl_atomic_rmw_##BYTES(                              \
        AtomicExchange, mem, val, map_mem(model));                      \
  }                                                                     \
  bool                                                                  \
  __atomic_compare_exchange_##BYTES(                                    \
      I##BYTES *mem, I##BYTES *expected, I##BYTES desired,              \
      int success, int failure) {                                       \
    I##BYTES e = *expected;                                             \
    I##BYTES old = __llvm_nacl_atomic_cmpxchg_##BYTES(                  \
        mem, e, desired, map_mem(success), map_mem(failure));           \
    bool succeeded = old == e;                                          \
    *expected = old;                                                    \
    return succeeded;                                                   \
  }                                                                     \
  DO_ATOMIC_RMW(BITS, BYTES, add, Add)                                  \
  DO_ATOMIC_RMW(BITS, BYTES, sub, Sub)                                  \
  DO_ATOMIC_RMW(BITS, BYTES, or, Or)                                    \
  DO_ATOMIC_RMW(BITS, BYTES, and, And)                                  \
  DO_ATOMIC_RMW(BITS, BYTES, xor, Xor)
DO_FOR_ALL_ATOMIC_SIZES()
#undef DO_ATOMIC
#undef DO_ATOMIC_RMW

__attribute__((noinline, noreturn))
static void unhandled_size(const char *error_string, size_t size) {
  write(2, error_string, size);
  abort();
}
#define UNHANDLED_SIZE(WHAT) do {                               \
    static const char msg[] = "Aborting: __atomic_" #WHAT       \
        " called with size greater than 8 bytes\n";             \
    unhandled_size(msg, sizeof(msg) - 1);                       \
  } while (0)

/*
 * Generic GCCMM functions which handle objects of arbitrary size.
 * Calls the optimized sized functions above.
 */
void __pnacl_atomic_load(size_t size, void *mem, void *ret, int model) {
#define DO_ATOMIC(BITS, BYTES)                  \
  case BYTES: {                                 \
    I##BYTES res = __atomic_load_##BYTES(       \
        (I##BYTES*)(mem), model);               \
    *(I##BYTES*)(ret) = res;                    \
  } break;
  switch(size) {
    DO_FOR_ALL_ATOMIC_SIZES();
    default: UNHANDLED_SIZE(load);
  }
#undef DO_ATOMIC
}
void __pnacl_atomic_store(size_t size, void *mem, void *val, int model) {
#define DO_ATOMIC(BITS, BYTES)                  \
  case BYTES: {                                 \
    I##BYTES v = *(I##BYTES*)(val);             \
    __atomic_store_##BYTES(                     \
        (I##BYTES*)(mem), v, model);            \
  } break;
  switch(size) {
    DO_FOR_ALL_ATOMIC_SIZES();
    default: UNHANDLED_SIZE(store);
  }
#undef DO_ATOMIC
}
void __pnacl_atomic_exchange(size_t size, void *mem, void *val, void *ret,
                             int model) {
#define DO_ATOMIC(BITS, BYTES)                  \
  case BYTES: {                                 \
    I##BYTES v = *(I##BYTES*)(val);             \
    I##BYTES res = __atomic_exchange_##BYTES(   \
        (I##BYTES*)(mem), v, model);            \
    *(I##BYTES*)(ret) = res;                    \
  } break;
  switch(size) {
    DO_FOR_ALL_ATOMIC_SIZES();
    default: UNHANDLED_SIZE(exchange);
  }
#undef DO_ATOMIC
}
bool __pnacl_atomic_compare_exchange(size_t size, void *mem, void *expected,
                                     void *desired, int success, int failure) {
#define DO_ATOMIC(BITS, BYTES)                          \
  case BYTES: {                                         \
    I##BYTES d = *(I##BYTES*)(desired);                 \
    return __atomic_compare_exchange_##BYTES(           \
        (I##BYTES*)(mem), (I##BYTES*)(expected), d,     \
        success, failure);                              \
  }
  switch(size) {
    DO_FOR_ALL_ATOMIC_SIZES();
    default: UNHANDLED_SIZE(compare_exchange);
  }
#undef DO_ATOMIC
}
