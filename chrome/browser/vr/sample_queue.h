// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_VR_SAMPLE_QUEUE_H_
#define CHROME_BROWSER_VR_SAMPLE_QUEUE_H_

#include <vector>

#include "base/macros.h"

namespace vr {

// Manages a fixed-size queue of samples including their current sum. Old
// samples are automatically dropped when an added sample would exceed the
// requested size.
class SampleQueue {
 public:
  explicit SampleQueue(size_t window_size);
  ~SampleQueue();

  int64_t GetSum() const { return sum_; }

  void AddSample(int64_t value);

  size_t GetCount() const { return samples_.size(); }

  // Get sliding window size for tests.
  size_t GetWindowSize() const { return window_size_; }

 private:
  int64_t sum_ = 0;
  size_t current_index_ = 0;
  size_t window_size_;
  std::vector<int64_t> samples_;
  DISALLOW_COPY_AND_ASSIGN(SampleQueue);
};

}  // namespace vr

#endif  // CHROME_BROWSER_VR_SAMPLE_QUEUE_H_
