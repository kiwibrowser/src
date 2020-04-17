// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef PLATFORM_API_TIME_H_
#define PLATFORM_API_TIME_H_

#include <chrono>

namespace openscreen {
namespace platform {

// For proper behavior of the OpenScreen library, the Clock implementation must
// tick at least 10000 times per second.
using kRequiredClockResolution = std::ratio<1, 10000>;

// A monotonic clock that meets all the C++14 requirements of a TrivialClock,
// for use with the chrono library. The default platform implementation bases
// this on std::chrono::steady_clock or std::chrono::high_resolution_clock, but
// a custom implementation may use a different source of time (e.g., an
// embedder's time library, a simulated time source, or a mock).
class Clock {
 public:
  // TrivialClock named requirements: std::chrono templates can/may use these.
  using duration = std::chrono::microseconds;
  using rep = duration::rep;
  using period = duration::period;
  using time_point = std::chrono::time_point<Clock, duration>;
  static constexpr bool is_steady = true;

  static time_point now() noexcept;

  // In <chrono>, a Clock is just some type properties plus a static now()
  // function. So, there's nothing to instantiate here.
  Clock() = delete;
  ~Clock() = delete;

  // "Trivially copyable" is necessary for use in std::atomic<>.
  static_assert(std::is_trivially_copyable<duration>(),
                "duration is not trivially copyable");
  static_assert(std::is_trivially_copyable<time_point>(),
                "time_point is not trivially copyable");
};

// Convenience, for injecting Clocks into classes. Note: The 'noexcept' keyword
// is dropped here to avoid a well-known Clang compiler warning (about an
// upcoming C++20 ABI change).
using ClockNowFunctionPtr = Clock::time_point (*)();

// Returns the number of seconds since UNIX epoch (1 Jan 1970, midnight)
// according to the wall clock, which is subject to adjustments (e.g., via
// NTP). Note that this is NOT the same time source as Clock::now() above, and
// is NOT guaranteed to be monotonically non-decreasing.
std::chrono::seconds GetWallTimeSinceUnixEpoch() noexcept;

}  // namespace platform
}  // namespace openscreen

#endif  // PLATFORM_API_TIME_H_
