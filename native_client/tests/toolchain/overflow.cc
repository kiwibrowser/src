/*
 * Copyright 2015 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * Test the overflow builtins.
 *
 * This test is run on multiple compilers and makes sure they all output the
 * same golden output values for overflow builtins. This is important to test
 * that PNaCl (which doesn't support these builtins as-is) still generates
 * correct code.
 *
 * The test performs addition/subtraction/multiplication of the minimum/maximum
 * numbers that can be represented for each datatype supported by the
 * overflow builtins.
 *
 * Documentation:
 *   clang.llvm.org/docs/LanguageExtensions.html#checked-arithmetic-builtins
 */

#include <iostream>
#include <iomanip>
#include <limits>

using namespace std;

// The compiler can do further optimizations when it sees values coming in to
// the overflow builtins. Run this test with and without inlining to make sure
// there's test coverage for overflow with constant, and overflow with variable.
#if SHOULD_INLINE
# define MAYBE_INLINE __attribute__((always_inline))
#else
# define MAYBE_INLINE __attribute__((noinline))
#endif

// Datatypes supported by the overflow builtins.
// DO(TYPE, SIGN, LENGTH)
#define FOR_EACH_TYPE(DO)       \
  DO(unsigned, u, )             \
  DO(unsigned long long, u, ll) \
  DO(int, s, )                  \
  DO(long long, s, ll)
static_assert(((sizeof(int) != sizeof(long)) ||
               (sizeof(long) != sizeof(long long))),
              "This test avoids testing long because it'll produce the same "
              "output as either int or long long (depending on the platform). "
              "If this weren't the case for some compilers then the golden "
              "output would be different.");

// The builtins have C-style names, create C++ overloads for them.
template <typename T> MAYBE_INLINE bool add(T, T, T*);
template <typename T> MAYBE_INLINE bool sub(T, T, T*);
template <typename T> MAYBE_INLINE bool mul(T, T, T*);

#define DECLARE_OVERLOADS(TYPE, SIGN, LENGTH)                       \
  template <>                                                       \
  MAYBE_INLINE bool add<TYPE>(TYPE lhs, TYPE rhs, TYPE * res) {     \
    return __builtin_##SIGN##add##LENGTH##_overflow(lhs, rhs, res); \
  }                                                                 \
  template <>                                                       \
  MAYBE_INLINE bool sub<TYPE>(TYPE lhs, TYPE rhs, TYPE * res) {     \
    return __builtin_##SIGN##sub##LENGTH##_overflow(lhs, rhs, res); \
  }                                                                 \
  template <>                                                       \
  MAYBE_INLINE bool mul<TYPE>(TYPE lhs, TYPE rhs, TYPE * res) {     \
    return __builtin_##SIGN##mul##LENGTH##_overflow(lhs, rhs, res); \
  }
FOR_EACH_TYPE(DECLARE_OVERLOADS)

#define TEST_SINGLE(OP, REP, LHS, RHS)                                     \
  do {                                                                     \
    const auto w = numeric_limits<T>::digits10 + 2;                        \
    T res;                                                                 \
    cout << setw(w) << LHS << ' ' << REP << ' ' << setw(w) << RHS << " = " \
         << setw(w);                                                       \
    if (OP(LHS, RHS, &res))                                                \
      cout << "overflow";                                                  \
    else                                                                   \
      cout << res;                                                         \
    cout << '\n';                                                          \
  } while (0)

// Test the combinations of OP(LHS, RHS). We start at the *_START values and
// iterate for `iterations` repetitions using the *_OP operator on values at
// each repetition.
#define ITERATE(OP, REP, LHS_START, RHS_START, LHS_OP, RHS_OP, ITERS) \
  do {                                                                \
    T lhs = LHS_START;                                                \
    for (int lhs_values = 0; lhs_values != ITERS; ++lhs_values) {     \
      T rhs = RHS_START;                                              \
      for (int rhs_values = 0; rhs_values != ITERS; ++rhs_values) {   \
        TEST_SINGLE(OP<T>, REP, lhs, rhs);                            \
        RHS_OP rhs;                                                   \
      }                                                               \
      LHS_OP lhs;                                                     \
    }                                                                 \
  } while (0)

template <typename T>
void test_type() {
  int iterations = 3;
  bool is_signed = numeric_limits<T>::is_signed;
  T min = numeric_limits<T>::min();
  T max = numeric_limits<T>::max();
  cout << "\nTesting " << sizeof(T) << " byte " << (is_signed ? "" : "un")
       << "signed integer:\n";
  ITERATE(add, '+', min, min, ++, ++, iterations);
  ITERATE(add, '+', min, max, ++, --, iterations);
  ITERATE(add, '+', max, max, --, --, iterations);
  ITERATE(add, '+', max, min, --, ++, iterations);
  ITERATE(sub, '-', min, min, ++, ++, iterations);
  ITERATE(sub, '-', min, max, ++, --, iterations);
  ITERATE(sub, '-', max, max, --, --, iterations);
  ITERATE(sub, '-', max, min, --, ++, iterations);
  ITERATE(mul, '*', min, min, ++, ++, iterations);
  ITERATE(mul, '*', min, max, ++, --, iterations);
  ITERATE(mul, '*', max, max, --, --, iterations);
  ITERATE(mul, '*', max, min, --, ++, iterations);
  if (is_signed) {
    ITERATE(add, '+', 0, min, (void), ++, iterations);
    ITERATE(add, '+', 0, max, (void), --, iterations);
    ITERATE(add, '+', min, 0, ++, (void), iterations);
    ITERATE(add, '+', max, 0, --, (void), iterations);
    ITERATE(sub, '-', 0, min, (void), ++, iterations);
    ITERATE(sub, '-', 0, max, (void), --, iterations);
    ITERATE(sub, '-', min, 0, ++, (void), iterations);
    ITERATE(sub, '-', max, 0, --, (void), iterations);
    ITERATE(mul, '*', 0, min, (void), ++, iterations);
    ITERATE(mul, '*', 0, max, (void), --, iterations);
    ITERATE(mul, '*', min, 0, ++, (void), iterations);
    ITERATE(mul, '*', max, 0, --, (void), iterations);
  }
}

int main() {
#define TEST_TYPE(TYPE, SIGN, LENGTH) test_type<TYPE>();
  FOR_EACH_TYPE(TEST_TYPE)

  return 0;
}
