// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/platform/testing/wtf/scoped_mock_clock.h"

namespace WTF {

ScopedMockClock* ScopedMockClock::top_ = nullptr;

ScopedMockClock::ScopedMockClock()
    : next_(top_), next_time_function_(GetTimeFunctionForTesting()) {
  top_ = this;
  SetTimeFunctionsForTesting(&Now);
}

ScopedMockClock::~ScopedMockClock() {
  top_ = next_;
  SetTimeFunctionsForTesting(next_time_function_);
}

double ScopedMockClock::Now() {
  return top_->now_.since_origin().InSecondsF();
}

void ScopedMockClock::Advance(TimeDelta delta) {
  DCHECK_GT(delta, base::TimeDelta())
      << "Monotonically increasing time may not go backwards";
  now_ += delta;
}

}  // namespace WTF
