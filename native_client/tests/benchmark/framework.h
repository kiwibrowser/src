// Copyright 2014 The Native Client Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <string>
#include <vector>

#include <stdint.h>

#define INLINE inline __attribute__((always_inline))

#undef HAVE_SIMD
#if defined(__has_builtin)
#if __has_builtin(__builtin_shufflevector)
#define HAVE_SIMD 1
#endif
#endif

// Derive each benchmark (NBody, Life, etc.) from Benchmark class,
// and provide Name() and Run() virtual functions.  Also provide optional
// Notes() function to annotate benchmark with additional info, such as
// scalar or SIMD version.
// The Run() method should perform the computation and return 0 for success. If
// it returns a non-zero value, the benchmark suite will fail and return
// EXIT_FAILURE from main().
class Benchmark {
 public:
  virtual const std::string Name() = 0;
  virtual const std::string Notes() { return ""; }
  virtual int Run() = 0;
};

typedef void (*BenchmarkCallback)
    (Benchmark* benchmark, double median, double range, void* data);

// Base class BenchmarkSuite singleton.
class BenchmarkSuite {
  typedef std::vector<Benchmark*> BenchmarkVector;
  static BenchmarkVector& Benchmarks() {
    static BenchmarkVector benchmarks;
    return benchmarks;
  }
 protected:
  static void Add(Benchmark* benchmark) { Benchmarks().push_back(benchmark); }
 public:
  static int Run(const char* description,
                 BenchmarkCallback callback,
                 void* data);
};

// RegisterBenchmark will add an instance of a benchmark to the suite.
template <class T>
class RegisterBenchmark : BenchmarkSuite {
 public:
  RegisterBenchmark() { Add(&benchmark_); }
 private:
  T benchmark_;
};
