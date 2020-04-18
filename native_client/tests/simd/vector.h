/*
 * Copyright (c) 2014 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * Code that's reused in vector tests.
 */

#ifndef _NATIVE_CLIENT_TESTS_SIMD_VECTOR_H
#define _NATIVE_CLIENT_TESTS_SIMD_VECTOR_H 1

#include <cmath>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <limits>
#include <pthread.h>
#include <random>
#include <type_traits>

#define CHECK(EXPR)                                                      \
  do {                                                                   \
    if (!(EXPR)) {                                                       \
      std::cerr << #EXPR " returned false at line " << __LINE__ << '\n'; \
      exit(1);                                                           \
    }                                                                    \
  } while (0)

#define NOINLINE __attribute__((noinline))

// X-Macro invoking ACTION(NUM_ELEMENTS, ELEMENT_TYPE, FORMAT_AS) for each
// supported vector type.
//
// TODO(jfb) Add uint64_t, double and wider vectors once supported.
#define FOR_EACH_VECTOR_TYPE(ACTION) \
  ACTION(16, uint8_t, uint32_t)      \
  ACTION(8, uint16_t, uint16_t)      \
  ACTION(4, uint32_t, uint32_t)      \
  ACTION(4, float, float)

// X-Macro for each state of the test.
#define FOR_EACH_STATE(ACTION) \
  ACTION(SETUP) ACTION(TEST) ACTION(CHECK) ACTION(CLEANUP)

struct Test {
  typedef void *(*Func)(void *);  // POSIX thread function signature.
#define CREATE_ENUM(S) S,
  enum State { FOR_EACH_STATE(CREATE_ENUM) DONE };
  Func functions[DONE];  // One function per state.
  enum { InBuf0, InBuf1, OutBuf, NumBufs };
  void *bufs[NumBufs];
  const char *name;
};
#define CREATE_STRING(S) #S,
static const char *states[] = {FOR_EACH_STATE(CREATE_STRING)};

#define DEFINE_FORMAT(N, T, FMT) \
  FMT format(T v) { return v; }
FOR_EACH_VECTOR_TYPE(DEFINE_FORMAT)

template <size_t NumElements, typename T>
NOINLINE void print_vector(const T *vec) {
  std::cout << '{';
  for (size_t i = 0; i != NumElements; ++i)
    std::cout << format(vec[i]) << (i == NumElements - 1 ? '}' : ',');
}

// Declare vector types of a particular number of elements and underlying type,
// aligned to the element width (*not* the vector's width).
//
// Attributes must be integer constants, work around that limitation and allow
// using the following type for vectors within generic code:
//
//   typedef typename VectorHelper<NumElements, T>::Vector VectorAligned;
template <size_t NumElements, typename T>
struct VectorHelper;
#define SPECIALIZE_VECTOR_HELPER(N, T, FMT)                              \
  template <>                                                            \
  struct VectorHelper<N, T> {                                            \
    typedef T VectorAligned __attribute__((vector_size(N * sizeof(T)))); \
    typedef T Unaligned                                                  \
        __attribute__((vector_size(N * sizeof(T)), aligned(1)));         \
    typedef T ElementAligned                                             \
        __attribute__((vector_size(N * sizeof(T)), aligned(sizeof(T)))); \
  };
FOR_EACH_VECTOR_TYPE(SPECIALIZE_VECTOR_HELPER)

// Fill an array with a pseudo-random sequence.
//
// Always generate the same pseudo-random sequence from one run to another (to
// preserve golden output file), but different sequence for each type being
// tested.
typedef std::minstd_rand Rng;
template <typename T>
NOINLINE void pseudoRandomInit(Rng *r, T *buf, size_t size) {
  for (size_t i = 0; i != size; ++i)
    buf[i] = std::is_floating_point<T>::value ? (*r)() / (T)(*r).max() : (*r)();
}

// Define main() and run all the tests in the TEST_ARRAY, which is defined as:
//   volatile Test TESTS_ARRAY[] = { ... };
#define DEFINE_MAIN(TESTS_ARRAY)                                           \
  int main(void) {                                                         \
    std::cout << std::dec << std::fixed                                    \
              << std::setprecision(std::numeric_limits<double>::digits10); \
                                                                           \
    for (int state = Test::SETUP; state != Test::DONE; ++state)            \
      for (size_t i = 0; i != sizeof(tests) / sizeof(tests[0]); ++i) {     \
        int err;                                                           \
        pthread_t tid;                                                     \
        volatile Test *test = &TESTS_ARRAY[i];                             \
        Test::Func f = test->functions[state];                             \
        std::cout << states[state] << ' ' << test->name << '\n';           \
                                                                           \
        err = pthread_create(&tid, NULL, f, (void *)test);                 \
        CHECK(err == 0);                                                   \
        err = pthread_join(tid, NULL);                                     \
        CHECK(err == 0);                                                   \
      }                                                                    \
                                                                           \
    return 0;                                                              \
  }

#endif
