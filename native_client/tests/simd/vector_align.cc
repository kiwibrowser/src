/*
 * Copyright (c) 2014 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * Make sure that all vector types supported by PNaCl can be loaded and stored
 * without faulting, even when their alignment is at the vector's element size
 * instead of the full vector's width.
 *
 * Care is taken in this test to prevent the compiler from propagating alignment
 * information or the actual address being loaded:
 *  - The test is performed in 4 steps:
 *     + Setup allocated aligned buffers, filled with a pseudo-random sequence.
 *     + Test misaligns those buffers (whose base alignment is unknown) and adds
 *       their values together using vector instructions. Adding is prefered to
 *       simply moving because the compiler could decide to use memcpy.
 *     + Check does the same but with scalar instructions, and also prints the
 *       vector to stdout to be verified against the golden file (in case the
 *       scalar loop gets improperly vectorized and does fault).
 *     + Cleanup frees the buffers.
 *  - Each step is run for all type+alignments before moving to the next step.
 *  - The test information is kept in a volatile datastructure.
 *  - Each step, for each type+alignment, is run in a thread, which the main
 *    thread waits on being completed before proceeding further. This is extra
 *    paranoia to prevent the compiler from being too smart.
 */

#include "native_client/tests/simd/vector.h"

// Arithmetic will be performed on that many vectors. More testing can be
// performed by increasing this.
volatile size_t NumVectorsToTest = 2;
// Number of vectors to allocate for tests. This leaves room for re-alignment
// within each vector-aligned section that's allocated.
volatile size_t NumVectorsToAllocate = NumVectorsToTest + 1;

// Check that the addition performed on vectors is the same as that performed on
// scalars.
template <size_t NumElements, typename T>
NOINLINE typename std::enable_if<std::is_floating_point<T>::value>::type
check_vector(const T *in0, const T *in1, const T *out) {
  for (size_t i = 0; i != NumElements; ++i)
    // Floating-point addition must be within one epsilon.
    CHECK(std::abs((in0[i] + in1[i]) - out[i]) <=
          std::numeric_limits<T>::epsilon());
}
template <size_t NumElements, typename T>
NOINLINE typename std::enable_if<std::is_unsigned<T>::value>::type
check_vector(const T *in0, const T *in1, const T *out) {
  for (size_t i = 0; i != NumElements; ++i)
    // Rely on unsigned type's max being a power of 2 minus one to mask sums
    // that overflow.
    CHECK(((in0[i] + in1[i]) & std::numeric_limits<T>::max()) == out[i]);
}

// Setup the memory which will hold the vector data. Align memory to the vector
// size. The test can then offset from this to get unaligned vectors.
template <size_t NumElements, typename T>
NOINLINE void *setup(void *arg) {
  volatile Test *t = (volatile Test *)arg;

  // Allocate the 2 input and 1 output buffers.
  for (size_t i = 0; i != Test::NumBufs; ++i) {
    T **buf_ptr = (T **)&t->bufs[i];
    size_t vectorBytes = NumElements * sizeof(T);
    size_t allocBytes = vectorBytes * NumVectorsToAllocate;
    int ret = posix_memalign((void **)buf_ptr, vectorBytes, allocBytes);
    CHECK(ret == 0);
    CHECK(((uintptr_t)*buf_ptr & ~(uintptr_t)(vectorBytes - 1)) ==
          (uintptr_t)*buf_ptr);  // Check alignment.
  }

  // Initialize the in buffers to a pseudo-random sequence, the output to zero.
  size_t initSize = NumElements * NumVectorsToAllocate;
  Rng r;
  for (size_t i = 0; i != Test::NumBufs; ++i) {
    T *buf = *(T **)&t->bufs[i];
    if (i == Test::OutBuf)
      memset(buf, 0, initSize * sizeof(T));
    else
      pseudoRandomInit(&r, buf, initSize);
  }

  return NULL;
}

template <size_t NumElementsMisaligned, size_t NumElements, typename T>
NOINLINE void *test(void *arg) {
  volatile Test *t = (volatile Test *)arg;

  // We're going to purposefully re-align the pointer so that it's not strictly
  // aligned to the vector size. Make sure that enough memory was allocated.
  CHECK(NumVectorsToAllocate > NumVectorsToTest);

  // Re-align.
  typedef typename VectorHelper<NumElements, T>::ElementAligned Vector;
  Vector *vec[Test::NumBufs];
  for (size_t i = 0; i != Test::NumBufs; ++i)
    vec[i] = (Vector *)((T *)t->bufs[i] + NumElementsMisaligned);

  // Load, add and store NumVectorsToTest times.
  for (size_t i = 0; i != NumVectorsToTest; ++i)
    *(vec[Test::OutBuf] + i) =
        *(vec[Test::InBuf0] + i) + *(vec[Test::InBuf1] + i);

  return NULL;
}

template <size_t NumElementsMisaligned, size_t NumElements, typename T>
NOINLINE void *check(void *arg) {
  volatile Test *t = (volatile Test *)arg;

  // Re-align the same way the test did, but this time go through scalars
  // instead of vectors.
  T *vec[Test::NumBufs];
  for (size_t i = 0; i != Test::NumBufs; ++i)
    vec[i] = (T *)t->bufs[i] + NumElementsMisaligned;

  // Print and check each vector, manually re-creating the indexing for
  // NumElements scalars instead of for a vector containing NumElements.
  for (size_t i = 0; i != NumVectorsToTest; ++i) {
    T *in0 = vec[Test::InBuf0] + i * NumElements;
    T *in1 = vec[Test::InBuf1] + i * NumElements;
    T *out = vec[Test::OutBuf] + i * NumElements;
    std::cout << "  ";
    print_vector<NumElements, T>(in0);
    std::cout << " + ";
    print_vector<NumElements, T>(in1);
    std::cout << " = ";
    print_vector<NumElements, T>(out);
    std::cout << '\n';

    check_vector<NumElements, T>(in0, in1, out);
  }

  return NULL;
}

NOINLINE void *cleanup(void *arg) {
  volatile Test *t = (volatile Test *)arg;
  for (size_t i = 0; i != Test::NumBufs; ++i)
    free(t->bufs[i]);
  return NULL;
}

#define SETUP_TEST(N, T, FMT)                                   \
  {{&setup<N, T>, &test<0, N, T>, &check<0, N, T>, &cleanup},   \
    {NULL, NULL, NULL}, "<" #N " x " #T "> misaligned by 0"},   \
  {{&setup<N, T>, &test<1, N, T>, &check<1, N, T>, &cleanup},   \
    {NULL, NULL, NULL}, "<" #N " x " #T "> misaligned by 1"},
volatile Test tests[] = {FOR_EACH_VECTOR_TYPE(SETUP_TEST)};

DEFINE_MAIN(tests)
