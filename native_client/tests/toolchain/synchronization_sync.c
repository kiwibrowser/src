/*
 * Copyright (c) 2013 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * This test ensures that the PNaCl backends can deal with compiler
 * intrinsics as would be written by regular users, including PNaCl's
 * ABI stabilization and target lowering.
 *
 * See other synchronization_* tests in this directory.
 *
 * This is a syntactical check as we do not run this multithreaded. Some
 * real testing is done here: tests/threads/thread_test.c
 *
 * PNaCl's atomic support is based on C11/C++11's, and also supports
 * GCC's legacy __sync_* intrinsics which internally map to the same IR
 * as C11/C++11's atomics. PNaCl only supports 8, 16, 32 and 64-bit
 * atomics, and should be able to handle any type that can map to these
 * (e.g. 32- and 64-bit atomic floating point load/store).
 *
 * GCC Legacy __sync Built-in Functions for Atomic Memory Access.
 * See:
 * http://gcc.gnu.org/onlinedocs/gcc-4.8.1/gcc/_005f_005fsync-Builtins.html
 *
 * Note: nand is ignored because it isn't in C11/C++11.
 */

#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>


#define CONCAT_(A, B) A ## B
#define CONCAT(A, B) CONCAT_(A, B)
#define STR_(A) #A
#define STR(A) STR_(A)

#define CHECK_EQ(LHS, RHS, MSG) do {            \
    printf("\t" MSG ":\t" STR(LHS) "=%" PRIu64  \
           " and " STR(RHS) "=%" PRIu64 "\n",   \
           (uint64_t)(LHS), (uint64_t)(RHS));   \
    if ((LHS) != (RHS)) {                       \
      fprintf(stderr, "ERROR: " MSG ": `"       \
              STR(LHS) " != " STR(RHS) "` "     \
              "\n");                            \
      exit(1);                                  \
    }                                           \
  } while (0)

/*
 * Test that using some synchronization primitives without using the
 * return value works. Do this by modifying volatile globals.
 */
volatile uint8_t g_uint8_t = 0;
volatile uint16_t g_uint16_t = 0;
volatile uint32_t g_uint32_t = 0;
volatile uint64_t g_uint64_t = 0;
#define SINK_ADDR(TYPE) (&CONCAT(g_, TYPE))

/*
 * Test that base+displacement as synchronization address also works,
 * with small and big displacements (which should affect unrolling).
 */
/* Small displacement. */
enum { gsmall_ArrSize = 8 };
volatile struct GS8 { void *ptr; uint8_t arr[gsmall_ArrSize]; }
  gsmall_uint8_t = { 0, { 0 } };
volatile struct GS16 { void *ptr; uint16_t arr[gsmall_ArrSize]; }
  gsmall_uint16_t = { 0, { 0 } };
volatile struct GS32 { void *ptr; uint32_t arr[gsmall_ArrSize]; }
  gsmall_uint32_t = { 0, { 0 } };
volatile struct GS64 { void *ptr; uint64_t arr[gsmall_ArrSize]; }
  gsmall_uint64_t = { 0, { 0 } };
#define SMALL_SINK_ADDR(TYPE) (                                         \
    CONCAT(gsmall_, TYPE).ptr = (void*) CONCAT(gsmall_, TYPE).arr,      \
    (TYPE*) CONCAT(gsmall_, TYPE).ptr)
/* Big displacement. */
enum { gbig_ArrSize = 128 };
volatile struct GB8 { void *ptr; uint8_t arr[gbig_ArrSize]; }
  gbig_uint8_t = { 0, { 0 } };
volatile struct GB16 { void *ptr; uint16_t arr[gbig_ArrSize]; }
  gbig_uint16_t = { 0, { 0 } };
volatile struct GB32 { void *ptr; uint32_t arr[gbig_ArrSize]; }
  gbig_uint32_t = { 0, { 0 } };
volatile struct GB64 { void *ptr; uint64_t arr[gbig_ArrSize]; }
  gbig_uint64_t = { 0, { 0 } };
#define BIG_SINK_ADDR(TYPE) (                                   \
    CONCAT(gbig_, TYPE).ptr = (void*) CONCAT(gbig_, TYPE).arr,  \
    (TYPE*) CONCAT(gbig_, TYPE).ptr)


/*
 * fetch_and_*
 *
 * These built-in functions perform the operation suggested by the name,
 * and returns the value that had previously been in memory:
 *   { tmp = *ptr; *ptr op= value; return tmp; }
 *
 * *_and_fetch
 *
 * These built-in functions perform the operation suggested by the name,
 * and return the new value:
 *   { *ptr op= value; return *ptr; }
 */
void test_fetch(void) {
# define FETCH_TEST(OPERATION_NAME, OPERATION, TYPE)  do {              \
    TYPE res, loc, value, res_, loc_, value_;                           \
    printf("Testing " STR(OPERATION_NAME) " with " STR(TYPE) "\n");     \
    /* Test __sync_fetch_and_* */                                       \
    loc = loc_ = 5;                                                     \
    value = value_ = 41;                                                \
    res = CONCAT(__sync_fetch_and_, OPERATION_NAME)(&loc, value);       \
    res_ = loc_; loc_ = loc_ OPERATION value_;                          \
    CHECK_EQ(res, res_, "__sync_fetch_and_" STR(OPERATION_NAME));       \
    /* Test __sync_*_and_fetch */                                       \
    loc = loc_ = 5;                                                     \
    value = value_ = 41;                                                \
    res = CONCAT(CONCAT(__sync_, OPERATION_NAME), _and_fetch)(          \
        &loc, value);                                                   \
    loc_ = loc_ OPERATION value_; res_ = loc_;                          \
    CHECK_EQ(res, res_, "__sync_" STR(OPERATION_NAME) "_and_fetch");    \
    /* Test the above two variants, without reading the result back. */ \
    { /* In just one variable. */                                       \
      volatile TYPE *sink = SINK_ADDR(TYPE);                            \
      (void) CONCAT(__sync_fetch_and_, OPERATION_NAME)(sink, value);    \
      (void) CONCAT(CONCAT(__sync_, OPERATION_NAME), _and_fetch)(       \
          sink, value);                                                 \
    }                                                                   \
    { /* In a small array. */                                           \
      volatile TYPE *small_sink = SMALL_SINK_ADDR(TYPE);                \
      size_t i;                                                         \
      for (i = 0; i != gsmall_ArrSize; ++i) {                           \
        (void) CONCAT(__sync_fetch_and_, OPERATION_NAME)(               \
            &small_sink[i], value);                                     \
        (void) CONCAT(CONCAT(__sync_, OPERATION_NAME), _and_fetch)(     \
            &small_sink[i], value);                                     \
      }                                                                 \
    }                                                                   \
    { /* In a big array. */                                             \
      volatile TYPE *big_sink = BIG_SINK_ADDR(TYPE);                    \
      size_t i;                                                         \
      for (i = 0; i != gbig_ArrSize; ++i) {                             \
        (void) CONCAT(__sync_fetch_and_, OPERATION_NAME)(               \
            &big_sink[i], value);                                       \
        (void) CONCAT(CONCAT(__sync_, OPERATION_NAME), _and_fetch)(     \
            &big_sink[i], value);                                       \
      }                                                                 \
    }                                                                   \
  } while (0)

# define FETCH_TEST_ALL_TYPES(OPERATION_NAME, OPERATION) do {   \
    FETCH_TEST(OPERATION_NAME, OPERATION, uint8_t);             \
    FETCH_TEST(OPERATION_NAME, OPERATION, uint16_t);            \
    FETCH_TEST(OPERATION_NAME, OPERATION, uint32_t);            \
    FETCH_TEST(OPERATION_NAME, OPERATION, uint64_t);            \
  } while (0)

  FETCH_TEST_ALL_TYPES(add, +);
  FETCH_TEST_ALL_TYPES(sub, -);
  FETCH_TEST_ALL_TYPES(or, |);
  FETCH_TEST_ALL_TYPES(and, &);
  FETCH_TEST_ALL_TYPES(xor, ^);
}

/*
 * *_compare_and_swap
 *
 * These built-in functions perform an atomic compare and swap. That is,
 * if the current value of *ptr is oldval, then write newval into
 * *ptr. The “bool” version returns true if the comparison is successful
 * and newval is written. The "val" version returns the contents of *ptr
 * before the operation.
 *
 * Note: LLVM also has __sync_swap, which is ignored because it isn't
 * C11/C++11 and isn't in GCC.
 */
void test_cas(void) {
# define CAS_TEST(TYPE) do {                                            \
    TYPE res, loc, oldval, newval;                                      \
    printf("Testing CAS with " STR(TYPE) "\n");                         \
    /* Test __sync_bool_compare_and_swap */                             \
    loc = 5;                                                            \
    oldval = 29;                                                        \
    newval = 41;                                                        \
    res = __sync_bool_compare_and_swap(&loc, oldval, newval);           \
    CHECK_EQ(res, 0, "__sync_bool_compare_and_swap");                   \
    CHECK_EQ(loc, 5, "__sync_bool_compare_and_swap");                   \
    loc = oldval;                                                       \
    res = __sync_bool_compare_and_swap(&loc, oldval, newval);           \
    CHECK_EQ(res, 1, "__sync_bool_compare_and_swap");                   \
    CHECK_EQ(loc, 41, "__sync_bool_compare_and_swap");                  \
    /* Test __sync_val_compare_and_swap */                              \
    loc = 5;                                                            \
    oldval = 29;                                                        \
    newval = 41;                                                        \
    res = __sync_val_compare_and_swap(&loc, oldval, newval);            \
    CHECK_EQ(res, 5, "__sync_val_compare_and_swap");                    \
    CHECK_EQ(loc, 5, "__sync_val_compare_and_swap");                    \
    loc = oldval;                                                       \
    res = __sync_val_compare_and_swap(&loc, oldval, newval);            \
    CHECK_EQ(res, oldval, "__sync_val_compare_and_swap");               \
    CHECK_EQ(loc, 41, "__sync_val_compare_and_swap");                   \
    /* Test the above two variants, without reading the result back. */ \
    { /* In just one variable. */                                       \
      volatile TYPE *sink = SINK_ADDR(TYPE);                            \
      (void) __sync_bool_compare_and_swap(sink, oldval, newval);        \
      (void) __sync_val_compare_and_swap(sink, oldval, newval);         \
    }                                                                   \
    { /* In a small array. */                                           \
      volatile TYPE *small_sink = SMALL_SINK_ADDR(TYPE);                \
      size_t i;                                                         \
      for (i = 0; i != gsmall_ArrSize; ++i) {                           \
        (void) __sync_bool_compare_and_swap(                            \
            &small_sink[i], oldval, newval);                            \
        (void) __sync_val_compare_and_swap(                             \
            &small_sink[i], oldval, newval);                            \
      }                                                                 \
    }                                                                   \
    { /* In a big array. */                                             \
      volatile TYPE *big_sink = BIG_SINK_ADDR(TYPE);                    \
      size_t i;                                                         \
      for (i = 0; i != gbig_ArrSize; ++i) {                             \
        (void) __sync_bool_compare_and_swap(                            \
            &big_sink[i], oldval, newval);                              \
        (void) __sync_val_compare_and_swap(                             \
            &big_sink[i], oldval, newval);                              \
      }                                                                 \
    }                                                                   \
  } while (0)

  CAS_TEST(uint8_t);
  CAS_TEST(uint16_t);
  CAS_TEST(uint32_t);
  CAS_TEST(uint64_t);
}

/*
 * synchronize
 *
 * This built-in function issues a full memory barrier. Simply test that
 * it compiles fine.
 */
void test_synchronize(void) {
  __sync_synchronize();
}

/*
 * lock_test_and_set
 *
 * This built-in function, as described by Intel, is not a traditional
 * test-and-set operation, but rather an atomic exchange operation. It
 * writes value into *ptr, and returns the previous contents of *ptr.
 * Many targets have only minimal support for such locks, and do not
 * support a full exchange operation. In this case, a target may support
 * reduced functionality here by which the only valid value to store is
 * the immediate constant 1. The exact value actually stored in *ptr is
 * implementation defined.
 *
 * This built-in function is not a full barrier, but rather an acquire
 * barrier. This means that references after the operation cannot move
 * to (or be speculated to) before the operation, but previous memory
 * stores may not be globally visible yet, and previous memory loads may
 * not yet be satisfied.
 *
 * lock_release
 *
 * This built-in function releases the lock acquired by
 * __sync_lock_test_and_set. Normally this means writing the constant 0
 * to *ptr.
 *
 * This built-in function is not a full barrier, but rather a release
 * barrier. This means that all previous memory stores are globally
 * visible, and all previous memory loads have been satisfied, but
 * following memory reads are not prevented from being speculated to
 * before the barrier.
 */
void test_tas(void) {
# define TAS_TEST(TYPE) do {                                            \
    TYPE res, loc, value;                                               \
    printf("Testing TAS with " STR(TYPE) "\n");                         \
    /* Test __sync_lock_test_and_set. */                                \
    loc = 5;                                                            \
    value = 41;                                                         \
    res = __sync_lock_test_and_set(&loc, value);                        \
    CHECK_EQ(res, 5, "__sync_lock_test_and_set");                       \
    CHECK_EQ(loc, 41, "__sync_lock_test_and_set");                      \
    /* Test __sync_lock_release. */                                     \
    __sync_lock_release(&loc);                                          \
    CHECK_EQ(loc, 0,  "__sync_lock_release");                           \
    { /* In just one variable. */                                       \
      volatile TYPE *sink = SINK_ADDR(TYPE);                            \
      (void) __sync_lock_test_and_set(sink, value);                     \
      __sync_lock_release(sink);                                        \
    }                                                                   \
    /* Test the above two variants, without reading the result back. */ \
    { /* In a small array. */                                           \
      volatile TYPE *small_sink = SMALL_SINK_ADDR(TYPE);                \
      size_t i;                                                         \
      for (i = 0; i != gsmall_ArrSize; ++i) {                           \
        (void) __sync_lock_test_and_set(&small_sink[i], value);         \
        __sync_lock_release(&small_sink[i]);                            \
      }                                                                 \
    }                                                                   \
    { /* In a big array. */                                             \
      volatile TYPE *big_sink = BIG_SINK_ADDR(TYPE);                    \
      size_t i;                                                         \
      for (i = 0; i != gbig_ArrSize; ++i) {                             \
        (void) __sync_lock_test_and_set(&big_sink[i], value);           \
        __sync_lock_release(&big_sink[i]);                              \
      }                                                                 \
    }                                                                   \
  } while (0)

  TAS_TEST(uint8_t);
  TAS_TEST(uint16_t);
  TAS_TEST(uint32_t);
  TAS_TEST(uint64_t);
}


int main(void) {
  test_fetch();
  test_cas();
  test_synchronize();
  test_tas();
  return 0;
}
