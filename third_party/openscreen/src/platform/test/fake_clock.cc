// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "platform/test/fake_clock.h"

#include "platform/api/logging.h"

namespace openscreen {

FakeClock::FakeClock(platform::Clock::time_point start_time) {
  OSP_CHECK(!instance_) << "attempting to use multiple fake clocks!";
  instance_ = this;
  now_ = start_time;
}

FakeClock::~FakeClock() {
  instance_ = nullptr;
}

platform::Clock::time_point FakeClock::now() noexcept {
  OSP_CHECK(instance_);
  return now_;
}

void FakeClock::Advance(platform::Clock::duration delta) {
  now_ += delta;
}

// static
FakeClock* FakeClock::instance_ = nullptr;

// static
platform::Clock::time_point FakeClock::now_;

}  // namespace openscreen
