// Copyright 2014 The Native Client Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <algorithm>
#include <string>
#include <vector>

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>

#include "native_client/tests/benchmark/framework.h"

namespace {

timeval start_tv;
int start_tv_retv = gettimeofday(&start_tv, NULL);

// Timer helper for benchmarking.
double getseconds() {
  const double usec_to_sec = 0.000001;
  timeval tv;
  if ((0 == start_tv_retv) && (0 == gettimeofday(&tv, NULL)))
    return (tv.tv_sec - start_tv.tv_sec) + tv.tv_usec * usec_to_sec;
  return 0.0;
}

bool SortCompareFunction(Benchmark* a, Benchmark* b) {
  return a->Name() < b->Name();
}

}  // namespace

int BenchmarkSuite::Run(const char* description, BenchmarkCallback callback,
    void* data) {
  int ret = EXIT_SUCCESS;
  printf("Running suite of %d benchmarks:\n", Benchmarks().size());
  std::sort(Benchmarks().begin(), Benchmarks().end(), SortCompareFunction);
  for (int i = 0, total = Benchmarks().size(); i < total; ++i) {
    std::vector<double> times;
    std::string name = Benchmarks()[i]->Name();
    printf("Running Benchmark: %s", name.c_str());
    std::string notes = Benchmarks()[i]->Notes();
    if (notes.length())
      printf(" (%s)", notes.c_str());
    printf("...\n");
    int rv = EXIT_SUCCESS;
    // Run each benchmark 3 times, report median & range.
    for (int j = 0; j < 3; j++) {
      double start_time = getseconds();
      int r = Benchmarks()[i]->Run();
      double end_time = getseconds();
      double total_time = end_time - start_time;
      times.push_back(total_time);
      if (r < 0)
        rv = r;
    }
    if (rv != EXIT_SUCCESS) {
      printf("@@@STEP_TEXT@<br>Benchmark %s FAILED@@@\n", name.c_str());
      ret = EXIT_FAILURE;
    }
    std::sort(times.begin(), times.end());
    double median = times[1];
    double range = times[2] - times[0];
    printf("RESULT Benchmark%s: %s= {%.6f, %.6f} seconds\n",
        name.c_str(), description, median, range);
    printf("---------------------------------------------------------------\n");
    // Invoke an optional callback on each benchmark.
    if (callback)
      callback(Benchmarks()[i], median, range, data);
  }
  printf("Done running benchmark suite.\n");
  return ret;
}
