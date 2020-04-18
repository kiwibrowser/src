// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/platform/scheduler/base/test/test_count_uses_time_source.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace base {
namespace sequence_manager {

TestCountUsesTimeSource::TestCountUsesTimeSource() : now_calls_count_(0) {}

TestCountUsesTimeSource::~TestCountUsesTimeSource() = default;

TimeTicks TestCountUsesTimeSource::NowTicks() const {
  now_calls_count_++;
  // Don't return 0, as it triggers some assertions.
  return TimeTicks() + TimeDelta::FromSeconds(1);
}

}  // namespace sequence_manager
}  // namespace base
