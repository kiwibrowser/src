// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CC_BENCHMARKS_MICRO_BENCHMARK_CONTROLLER_IMPL_H_
#define CC_BENCHMARKS_MICRO_BENCHMARK_CONTROLLER_IMPL_H_

#include <string>
#include <vector>

#include "base/macros.h"
#include "cc/benchmarks/micro_benchmark_impl.h"

namespace cc {

class LayerTreeHostImpl;
class CC_EXPORT MicroBenchmarkControllerImpl {
 public:
  explicit MicroBenchmarkControllerImpl(LayerTreeHostImpl* host);
  ~MicroBenchmarkControllerImpl();

  void DidCompleteCommit();

  void ScheduleRun(std::unique_ptr<MicroBenchmarkImpl> benchmark);

 private:
  void CleanUpFinishedBenchmarks();

  LayerTreeHostImpl* host_;
  std::vector<std::unique_ptr<MicroBenchmarkImpl>> benchmarks_;

  DISALLOW_COPY_AND_ASSIGN(MicroBenchmarkControllerImpl);
};

}  // namespace cc

#endif  // CC_BENCHMARKS_MICRO_BENCHMARK_CONTROLLER_IMPL_H_
