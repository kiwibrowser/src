// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_PLATFORM_TESTING_WTF_SCOPED_MOCK_CLOCK_H_
#define THIRD_PARTY_BLINK_RENDERER_PLATFORM_TESTING_WTF_SCOPED_MOCK_CLOCK_H_

#include "third_party/blink/renderer/platform/wtf/time.h"

namespace WTF {

// Scoped helper to override WTF time functions for testing. Creating one on the
// stack resets mock time to the zero point for WTF::Time and WTF::TimeTicks.
// Mock time may only flow forwards, not backwards.
class ScopedMockClock {
 public:
  ScopedMockClock();
  ~ScopedMockClock();

  static double Now();

  void Advance(TimeDelta);

 private:
  static ScopedMockClock* top_;
  ScopedMockClock* next_;
  TimeFunction next_time_function_;
  base::TimeTicks now_;
};

}  // namespace WTF

#endif
