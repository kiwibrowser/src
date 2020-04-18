/*
 * Copyright (c) 2014 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * Make sure that all vector types supported by PNaCl can be stack allocated.
 */

#include "native_client/tests/simd/vector.h"
#include <alloca.h>

volatile size_t NumRecursions = 32;

typedef void (*Recurse)(volatile void *, volatile void *, size_t);

// This function calls itself recursively (through a volatile pointer to avoid
// tail recursion optimization), overallocating space with alloca and copying
// the vector on the stack until there isn't any overallocation. It then simply
// prints out the vector.
template <size_t NumElements, typename T>
NOINLINE void test_alloca(volatile void *fn, volatile void *buf,
                          size_t overallocate) {
  typedef typename VectorHelper<NumElements, T>::VectorAligned VectorAligned;
  typedef typename VectorHelper<NumElements, T>::Unaligned VectorUnaligned;
  Recurse self = (Recurse)fn;
  if (overallocate > 1) {
    // Alloca doesn't guarantee alignment.
    VectorUnaligned *a =
        (VectorUnaligned *)alloca(NumElements * sizeof(T) * overallocate);
    VectorAligned *v = (VectorAligned *)buf;
    a[overallocate - 1] = *v;
    (*self)(fn, &a[overallocate - 1], overallocate - 1);
  } else {
    T *t = (T *)buf;
    std::cout << "  ";
    print_vector<NumElements, T>(t);
    std::cout << '\n';
  }
}

// Setup the memory which will hold the vector data. Align memory to the vector
// size. The test can then offset from this to get unaligned vectors.
template <size_t NumElements, typename T>
NOINLINE void *setup(void *arg) {
  volatile Test *t = (volatile Test *)arg;

  // Allocate the input buffer.
  T **buf_ptr = (T **)&t->bufs[Test::InBuf0];
  size_t vectorBytes = NumElements * sizeof(T);
  int ret = posix_memalign((void **)buf_ptr, vectorBytes, vectorBytes);
  CHECK(ret == 0);
  CHECK(((uintptr_t)*buf_ptr & ~(uintptr_t)(vectorBytes - 1)) ==
        (uintptr_t)*buf_ptr);  // Check alignment.

  // Initialize the in buffer.
  Rng r;
  T *buf = *(T **)&t->bufs[Test::InBuf0];
  pseudoRandomInit(&r, buf, NumElements);
  std::cout << "  ";
  print_vector<NumElements, T>(buf);
  std::cout << '\n';

  return NULL;
}

template <size_t NumElements, typename T>
NOINLINE void *test(void *arg) {
  volatile Test *t = (volatile Test *)arg;
  volatile void *buf = *(void **)&t->bufs[Test::InBuf0];
  volatile void *fn = (void *)&test_alloca<NumElements, T>;
  test_alloca<NumElements, T>(fn, buf, NumRecursions);
  return NULL;
}

NOINLINE void *check(void *arg) {
  (void)arg;
  return NULL;
}

NOINLINE void *cleanup(void *arg) {
  volatile Test *t = (volatile Test *)arg;
  free(t->bufs[Test::InBuf0]);
  return NULL;
}

#define SETUP_TEST(N, T, FMT)                                \
  {{&setup<N, T>, &test<N, T>, &check, &cleanup},            \
    {NULL, NULL, NULL}, "<" #N " x " #T ">"},
volatile Test tests[] = {FOR_EACH_VECTOR_TYPE(SETUP_TEST)};

DEFINE_MAIN(tests)
