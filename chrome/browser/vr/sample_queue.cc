// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <stdint.h>

#include "chrome/browser/vr/sample_queue.h"

namespace vr {

SampleQueue::SampleQueue(size_t window_size) : window_size_(window_size) {
  samples_.reserve(window_size);
}

SampleQueue::~SampleQueue() = default;

void SampleQueue::AddSample(int64_t value) {
  sum_ += value;

  if (samples_.size() < window_size_) {
    samples_.push_back(value);
  } else {
    sum_ -= samples_[current_index_];
    samples_[current_index_] = value;
  }

  ++current_index_;
  if (current_index_ >= window_size_) {
    current_index_ = 0;
  }
}

}  // namespace vr
