// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "streaming/cast/ssrc.h"

#include <random>

#include "platform/api/time.h"

namespace openscreen {
namespace cast_streaming {

namespace {

// These ranges are arbitrary, but have been used for several years (in prior
// implementations of Cast Streaming).
constexpr int kHigherPriorityMin = 1;
constexpr int kHigherPriorityMax = 50000;
constexpr int kNormalPriorityMin = 50001;
constexpr int kNormalPriorityMax = 100000;

}  // namespace

Ssrc GenerateSsrc(bool higher_priority) {
  // Use a statically-allocated generator, instantiated upon first use, and
  // seeded with the current time tick count. This generator was chosen because
  // it is light-weight and does not need to produce unguessable (nor
  // crypto-secure) values.
  static std::minstd_rand generator(static_cast<std::minstd_rand::result_type>(
      platform::Clock::now().time_since_epoch().count()));

  std::uniform_int_distribution<int> distribution(
      higher_priority ? kHigherPriorityMin : kNormalPriorityMin,
      higher_priority ? kHigherPriorityMax : kNormalPriorityMax);
  return static_cast<Ssrc>(distribution(generator));
}

int ComparePriority(Ssrc ssrc_a, Ssrc ssrc_b) {
  return static_cast<int>(ssrc_a) - static_cast<int>(ssrc_b);
}

}  // namespace cast_streaming
}  // namespace openscreen
