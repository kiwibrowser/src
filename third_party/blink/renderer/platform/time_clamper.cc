// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/platform/time_clamper.h"

#include "base/bit_cast.h"
#include "base/rand_util.h"
#include "third_party/blink/renderer/platform/wtf/assertions.h"

#include <cmath>

namespace blink {

TimeClamper::TimeClamper() : secret_(base::RandUint64()) {}

double TimeClamper::ClampTimeResolution(double time_seconds) const {
  DCHECK_GE(time_seconds, 0);
  double interval = floor(time_seconds / kResolutionSeconds);
  double clamped_time = interval * kResolutionSeconds;
  double tick_threshold = ThresholdFor(clamped_time);

  if (time_seconds >= tick_threshold)
    return (interval + 1) * kResolutionSeconds;
  return clamped_time;
}

inline double TimeClamper::ThresholdFor(double clamped_time) const {
  uint64_t time_hash = MurmurHash3(bit_cast<int64_t>(clamped_time) ^ secret_);
  return clamped_time + kResolutionSeconds * ToDouble(time_hash);
}

// static
inline double TimeClamper::ToDouble(uint64_t value) {
  // Exponent for double values for [1.0 .. 2.0]
  static const uint64_t kExponentBits = uint64_t{0x3FF0000000000000};
  static const uint64_t kMantissaMask = uint64_t{0x000FFFFFFFFFFFFF};
  uint64_t random = (value & kMantissaMask) | kExponentBits;
  return bit_cast<double>(random) - 1;
}

// static
inline uint64_t TimeClamper::MurmurHash3(uint64_t value) {
  value ^= value >> 33;
  value *= uint64_t{0xFF51AFD7ED558CCD};
  value ^= value >> 33;
  value *= uint64_t{0xC4CEB9FE1A85EC53};
  value ^= value >> 33;
  return value;
}

}  // namespace blink
