// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_PLATFORM_SCHEDULER_BASE_TEST_TEST_COUNT_USES_TIME_SOURCE_H_
#define THIRD_PARTY_BLINK_RENDERER_PLATFORM_SCHEDULER_BASE_TEST_TEST_COUNT_USES_TIME_SOURCE_H_

#include "base/macros.h"
#include "base/time/tick_clock.h"

namespace base {
namespace sequence_manager {

class TestCountUsesTimeSource : public TickClock {
 public:
  explicit TestCountUsesTimeSource();
  ~TestCountUsesTimeSource() override;

  TimeTicks NowTicks() const override;
  int now_calls_count() const { return now_calls_count_; }

 private:
  DISALLOW_COPY_AND_ASSIGN(TestCountUsesTimeSource);

  mutable int now_calls_count_;
};

}  // namespace sequence_manager
}  // namespace base

#endif  // THIRD_PARTY_BLINK_RENDERER_PLATFORM_SCHEDULER_BASE_TEST_TEST_COUNT_USES_TIME_SOURCE_H_
