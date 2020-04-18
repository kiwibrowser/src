/*
 * Copyright (c) 2013 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * This test ensures that the PNaCl backends can deal with C++11
 * synchronization primitives as would be written by regular users,
 * including PNaCl's ABI stabilization and target lowering.
 *
 * See other synchronization_* tests in this directory.
 *
 * This is a syntactical check as we do not run this multithreaded. Some
 * real testing is done here: tests/threads/thread_test.c
 */

#include <atomic>
#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>


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
 * ATOMIC_*_LOCK_FREE
 *
 * These macros must be compile-time constants, and for PNaCl the value
 * should be 1, which means that the corresponding type may be
 * lock-free: we can't guarantee that all our platforms are lock-free.
 */
void test_lock_free_macros() {
#if defined(__pnacl__)
  static_assert(ATOMIC_BOOL_LOCK_FREE == 1, "should be compile-time 1");
  static_assert(ATOMIC_CHAR_LOCK_FREE == 1, "should be compile-time 1");
  static_assert(ATOMIC_CHAR16_T_LOCK_FREE == 1, "should be compile-time 1");
  static_assert(ATOMIC_CHAR32_T_LOCK_FREE == 1, "should be compile-time 1");
  static_assert(ATOMIC_WCHAR_T_LOCK_FREE == 1, "should be compile-time 1");
  static_assert(ATOMIC_SHORT_LOCK_FREE == 1, "should be compile-time 1");
  static_assert(ATOMIC_INT_LOCK_FREE == 1, "should be compile-time 1");
  static_assert(ATOMIC_LONG_LOCK_FREE == 1, "should be compile-time 1");
  static_assert(ATOMIC_LLONG_LOCK_FREE == 1, "should be compile-time 1");
  static_assert(ATOMIC_POINTER_LOCK_FREE == 1, "should be compile-time 1");
#elif defined(__x86_64__) || defined(__i386__) || defined(__arm__)
  static_assert(ATOMIC_BOOL_LOCK_FREE == 2, "should be compile-time 2");
  static_assert(ATOMIC_CHAR_LOCK_FREE == 2, "should be compile-time 2");
  static_assert(ATOMIC_CHAR16_T_LOCK_FREE == 2, "should be compile-time 2");
  static_assert(ATOMIC_CHAR32_T_LOCK_FREE == 2, "should be compile-time 2");
  static_assert(ATOMIC_WCHAR_T_LOCK_FREE == 2, "should be compile-time 2");
  static_assert(ATOMIC_SHORT_LOCK_FREE == 2, "should be compile-time 2");
  static_assert(ATOMIC_INT_LOCK_FREE == 2, "should be compile-time 2");
  static_assert(ATOMIC_LONG_LOCK_FREE == 2, "should be compile-time 2");
  static_assert(ATOMIC_LLONG_LOCK_FREE == 2, "should be compile-time 2");
  static_assert(ATOMIC_POINTER_LOCK_FREE == 2, "should be compile-time 2");
#elif defined(__mips__)
  static_assert(ATOMIC_BOOL_LOCK_FREE == 2, "should be compile-time 2");
  static_assert(ATOMIC_CHAR_LOCK_FREE == 2, "should be compile-time 2");
  static_assert(ATOMIC_CHAR16_T_LOCK_FREE == 2, "should be compile-time 2");
  static_assert(ATOMIC_CHAR32_T_LOCK_FREE == 2, "should be compile-time 2");
  static_assert(ATOMIC_WCHAR_T_LOCK_FREE == 2, "should be compile-time 2");
  static_assert(ATOMIC_SHORT_LOCK_FREE == 2, "should be compile-time 2");
  static_assert(ATOMIC_INT_LOCK_FREE == 2, "should be compile-time 2");
  static_assert(ATOMIC_LONG_LOCK_FREE == 2, "should be compile-time 2");
  static_assert(ATOMIC_LLONG_LOCK_FREE == 1, "should be compile-time 1");
  static_assert(ATOMIC_POINTER_LOCK_FREE == 2, "should be compile-time 2");
# else
  #error "unknown arch"
#endif
}

#if defined(__mips__)
  bool isMips = true;
#else
  bool isMips = false;
#endif

#define TEST_IS_LOCK_FREE(TYPE) do {                              \
     bool expectLockFree = !((sizeof(TYPE) == 8) && isMips);      \
     CHECK_EQ(std::atomic<TYPE>().is_lock_free(), expectLockFree, \
              "expected lock-free for `" STR(TYPE) "`");          \
  } while (0)

void test_is_lock_free() {
  TEST_IS_LOCK_FREE(bool);
  // Table 145.
  TEST_IS_LOCK_FREE(char);
  TEST_IS_LOCK_FREE(signed char);
  TEST_IS_LOCK_FREE(unsigned char);
  TEST_IS_LOCK_FREE(short);
  TEST_IS_LOCK_FREE(unsigned short);
  TEST_IS_LOCK_FREE(int);
  TEST_IS_LOCK_FREE(unsigned int);
  TEST_IS_LOCK_FREE(long);
  TEST_IS_LOCK_FREE(unsigned long);
  TEST_IS_LOCK_FREE(long long);
  TEST_IS_LOCK_FREE(unsigned long long);
  TEST_IS_LOCK_FREE(char16_t);
  TEST_IS_LOCK_FREE(char32_t);
  TEST_IS_LOCK_FREE(wchar_t);

  // Table 146.
  TEST_IS_LOCK_FREE(int_least8_t);
  TEST_IS_LOCK_FREE(uint_least8_t);
  TEST_IS_LOCK_FREE(int_least16_t);
  TEST_IS_LOCK_FREE(uint_least16_t);
  TEST_IS_LOCK_FREE(int_least32_t);
  TEST_IS_LOCK_FREE(uint_least32_t);
  TEST_IS_LOCK_FREE(int_least64_t);
  TEST_IS_LOCK_FREE(uint_least64_t);
  TEST_IS_LOCK_FREE(int_fast8_t);
  TEST_IS_LOCK_FREE(uint_fast8_t);
  TEST_IS_LOCK_FREE(int_fast16_t);
  TEST_IS_LOCK_FREE(uint_fast16_t);
  TEST_IS_LOCK_FREE(int_fast32_t);
  TEST_IS_LOCK_FREE(uint_fast32_t);
  TEST_IS_LOCK_FREE(int_fast64_t);
  TEST_IS_LOCK_FREE(uint_fast64_t);
  TEST_IS_LOCK_FREE(intptr_t);
  TEST_IS_LOCK_FREE(uintptr_t);
  TEST_IS_LOCK_FREE(size_t);
  TEST_IS_LOCK_FREE(ptrdiff_t);
  TEST_IS_LOCK_FREE(intmax_t);
  TEST_IS_LOCK_FREE(uintmax_t);
}

// TODO(jfb) Test C++11 atomic features.
//   - std::atomic and their functions (including is_lock_free).
//   - std::atomic_flag.
//   - std::atomic_thread_fence (atomic_signal_fence currently unsupported).
//   - 6 memory orders.

int main() {
  test_lock_free_macros();
  test_is_lock_free();
  return 0;
}
