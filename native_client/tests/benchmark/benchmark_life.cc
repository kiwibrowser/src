// Copyright 2014 The Native Client Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "native_client/tests/benchmark/framework.h"
#include "native_client/tests/benchmark/thread_pool.h"


using sdk_util::ThreadPool;  // For sdk_util::ThreadPool

namespace {

const int kCellAlignment = 0x10;
const int kWidth = 2048;
const int kHeight = 2048;

#if defined(HAVE_SIMD)
// 128 bit vector types
typedef uint8_t u8x16_t __attribute__((vector_size(16)))
                        __attribute__((aligned(1)));
// TODO(dschuff): remove aligned(1) attribute above once nacl-clang has
// same vector alignment rules as pnacl.

// Helper function to broadcast x across 16 element vector.
INLINE u8x16_t broadcast(uint8_t x) {
  u8x16_t r = {x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x};
  return r;
}
#endif  // HAVE_SIMD

class Life {
 public:
  Life();
  virtual ~Life();
  void Reset();
  void SimulateFrame();
 private:
  void wSimulate(int y);
  static void wSimulateEntry(int y, void* data);

  uint8_t* cell_in_;
  uint8_t* cell_out_;
  int32_t cell_stride_;
  size_t size_;
  ThreadPool* workers_;
};

Life::Life() :
    cell_in_(NULL),
    cell_out_(NULL),
    cell_stride_(0) {
  // Query system for number of processors via sysconf()
  int num_threads = sysconf(_SC_NPROCESSORS_ONLN);
  workers_ = num_threads < 2 ? NULL : new ThreadPool(num_threads);
  cell_stride_ = (kWidth + kCellAlignment - 1) &
      ~(kCellAlignment - 1);
  size_ = cell_stride_ * kHeight;

  // Create a new context
  void* in_buffer = NULL;
  void* out_buffer = NULL;
  // alloc buffers aligned on 16 bytes
  posix_memalign(&in_buffer, kCellAlignment, size_);
  posix_memalign(&out_buffer, kCellAlignment, size_);
  cell_in_ = (uint8_t*) in_buffer;
  cell_out_ = (uint8_t*) out_buffer;

  Reset();
}

Life::~Life() {
  delete workers_;
}

void Life::wSimulate(int y) {
  // These represent the new health value of a cell based on its neighboring
  // values.  The health is binary: either alive or dead.
  const uint8_t kIsAlive[] = {
      0, 0, 0, 0, 0, 1, 1, 1, 0,
      0, 0, 0, 0, 0, 0, 0, 0, 0
  };

  // Don't run simulation on top and bottom borders
  if (y < 1 || y >= kHeight - 1)
    return;

  // Do neighbor summation; apply rules, output pixel color. Note that a 1 cell
  // wide perimeter is excluded from the simulation update; only cells from
  // x = 1 to x < width - 1 and y = 1 to y < height - 1 are updated.
  uint8_t *src0 = (cell_in_ + (y - 1) * cell_stride_);
  uint8_t *src1 = src0 + cell_stride_;
  uint8_t *src2 = src1 + cell_stride_;
  uint8_t *dst = (cell_out_ + y * cell_stride_) + 1;
  int32_t x = 1;

#if defined(HAVE_SIMD)
  const u8x16_t kOne = broadcast(1);
  const u8x16_t kFour = broadcast(4);
  const u8x16_t kEight = broadcast(8);

  // Prime the src
  u8x16_t src00 = *reinterpret_cast<u8x16_t*>(&src0[0]);
  u8x16_t src01 = *reinterpret_cast<u8x16_t*>(&src0[16]);
  u8x16_t src10 = *reinterpret_cast<u8x16_t*>(&src1[0]);
  u8x16_t src11 = *reinterpret_cast<u8x16_t*>(&src1[16]);
  u8x16_t src20 = *reinterpret_cast<u8x16_t*>(&src2[0]);
  u8x16_t src21 = *reinterpret_cast<u8x16_t*>(&src2[16]);

  // This inner loop is SIMD - each loop iteration will process 16 cells.
  for (; (x + 15) < (kWidth - 1); x += 16) {
    // Construct jittered source temps, using __builtin_shufflevector(..) to
    // extract a shifted 16 element vector from the 32 element concatenation
    // of two source vectors.
    u8x16_t src0j0 = src00;
    u8x16_t src0j1 = __builtin_shufflevector(src00, src01,
        1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16);
    u8x16_t src0j2 = __builtin_shufflevector(src00, src01,
        2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17);
    u8x16_t src1j0 = src10;
    u8x16_t src1j1 = __builtin_shufflevector(src10, src11,
        1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16);
    u8x16_t src1j2 = __builtin_shufflevector(src10, src11,
        2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17);
    u8x16_t src2j0 = src20;
    u8x16_t src2j1 = __builtin_shufflevector(src20, src21,
        1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16);
    u8x16_t src2j2 = __builtin_shufflevector(src20, src21,
        2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17);

    // Sum the jittered sources to construct neighbor count.
    u8x16_t count = src0j0 + src0j1 +  src0j2 +
                    src1j0 +        +  src1j2 +
                    src2j0 + src2j1 +  src2j2;
    // Add the center cell.
    count = count + count + src1j1;
    // If count > 4 and < 8, center cell will be alive in the next frame.
    u8x16_t alive1 = count > kFour;
    u8x16_t alive2 = count < kEight;
    // Intersect the two comparisons from above.
    u8x16_t alive = alive1 & alive2;

    // Convert alive mask to 1 or 0 and store in destination cell array.
    *reinterpret_cast<u8x16_t*>(dst) = alive & kOne;

    // Increment pointers.
    dst += 16;
    src0 += 16;
    src1 += 16;
    src2 += 16;

    // Shift source over by 16 cells and read the next 16 cells.
    src00 = src01;
    src01 = *reinterpret_cast<u8x16_t*>(&src0[16]);
    src10 = src11;
    src11 = *reinterpret_cast<u8x16_t*>(&src1[16]);
    src20 = src21;
    src21 = *reinterpret_cast<u8x16_t*>(&src2[16]);
  }
#endif  // HAVE_SIMD

  // The SIMD loop above does 16 cells at a time.  The loop below is the
  // regular version which processes one cell at a time.  It is used to
  // finish the remainder of the scanline not handled by the SIMD loop.
  for (; x < (kWidth - 1); ++x) {
    // Sum the jittered sources to construct neighbor count.
    int count = src0[0] + src0[1] + src0[2] +
                src1[0] +         + src1[2] +
                src2[0] + src2[1] + src2[2];
    // Add the center cell.
    count = count + count + src1[1];
    // Use table lookup indexed by count to determine pixel & alive state.
    *dst++ = kIsAlive[count];
    ++src0;
    ++src1;
    ++src2;
  }
}

// Static entry point for worker thread.
void Life::wSimulateEntry(int slice, void* thiz) {
  static_cast<Life*>(thiz)->wSimulate(slice);
}

void Life::SimulateFrame() {
  if (workers_) {
    // If multi-threading enabled, dispatch tasks to pool of worker threads.
    workers_->Dispatch(kHeight, wSimulateEntry, this);
  } else {
    // Else manually simulate each line on this thread.
    for (int y = 0; y < kHeight; y++) {
      wSimulateEntry(y, this);
    }
  }
  std::swap(cell_in_, cell_out_);
}

void Life::Reset() {
  memset(cell_out_, 0, size_);
  for (size_t index = 0; index < size_; index++) {
    cell_in_[index] = rand() & 1;
  }
}


// Wrap life in benchmark harness
class BenchmarkLife : public Benchmark {
 public:
  virtual int Run() {
    const int kFramesToBenchmark = 100;
    life_.Reset();
    for (int i = 0; i < kFramesToBenchmark; ++i)
      life_.SimulateFrame();
    // TODO(nfullagar): make simulation deterministic & compute a checksum on
    // the last frame.  Return success or failure based on the checksum.
    return 0;
  }
  virtual const std::string Name() { return "Life"; }
  virtual const std::string Notes() {
#if defined(HAVE_SIMD)
      return "SIMD version";
#else
      return "scalar version";
#endif
  }
 private:
  Life life_;
};

}  // namespace

// Register an instance to the list of benchmarks to be run.
RegisterBenchmark<BenchmarkLife> benchmark_life;
