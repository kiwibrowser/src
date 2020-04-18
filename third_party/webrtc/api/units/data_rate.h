/*
 *  Copyright (c) 2018 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#ifndef API_UNITS_DATA_RATE_H_
#define API_UNITS_DATA_RATE_H_
#include <stdint.h>
#include <cmath>
#include <limits>
#include <string>

#include "rtc_base/checks.h"

#include "api/units/data_size.h"
#include "api/units/time_delta.h"

namespace webrtc {
namespace data_rate_impl {
constexpr int64_t kPlusInfinityVal = std::numeric_limits<int64_t>::max();

inline int64_t Microbits(const DataSize& size) {
  constexpr int64_t kMaxBeforeConversion =
      std::numeric_limits<int64_t>::max() / 8000000;
  RTC_DCHECK_LE(size.bytes(), kMaxBeforeConversion)
      << "size is too large to be expressed in microbytes";
  return size.bytes() * 8000000;
}
}  // namespace data_rate_impl

// DataRate is a class that represents a given data rate. This can be used to
// represent bandwidth, encoding bitrate, etc. The internal storage is bits per
// second (bps).
class DataRate {
 public:
  DataRate() = delete;
  static DataRate Zero() { return DataRate(0); }
  static DataRate Infinity() {
    return DataRate(data_rate_impl::kPlusInfinityVal);
  }
  static DataRate bits_per_second(int64_t bits_per_sec) {
    RTC_DCHECK_GE(bits_per_sec, 0);
    return DataRate(bits_per_sec);
  }
  static DataRate bps(int64_t bits_per_sec) {
    return DataRate::bits_per_second(bits_per_sec);
  }
  static DataRate kbps(int64_t kilobits_per_sec) {
    return DataRate::bits_per_second(kilobits_per_sec * 1000);
  }
  int64_t bits_per_second() const {
    RTC_DCHECK(IsFinite());
    return bits_per_sec_;
  }
  int64_t bps() const { return bits_per_second(); }
  int64_t kbps() const { return (bps() + 500) / 1000; }
  bool IsZero() const { return bits_per_sec_ == 0; }
  bool IsInfinite() const {
    return bits_per_sec_ == data_rate_impl::kPlusInfinityVal;
  }
  bool IsFinite() const { return !IsInfinite(); }

  bool operator==(const DataRate& other) const {
    return bits_per_sec_ == other.bits_per_sec_;
  }
  bool operator!=(const DataRate& other) const {
    return bits_per_sec_ != other.bits_per_sec_;
  }
  bool operator<=(const DataRate& other) const {
    return bits_per_sec_ <= other.bits_per_sec_;
  }
  bool operator>=(const DataRate& other) const {
    return bits_per_sec_ >= other.bits_per_sec_;
  }
  bool operator>(const DataRate& other) const {
    return bits_per_sec_ > other.bits_per_sec_;
  }
  bool operator<(const DataRate& other) const {
    return bits_per_sec_ < other.bits_per_sec_;
  }

 private:
  // Bits per second used internally to simplify debugging by making the value
  // more recognizable.
  explicit DataRate(int64_t bits_per_second) : bits_per_sec_(bits_per_second) {}
  int64_t bits_per_sec_;
};

inline DataRate operator*(const DataRate& rate, const double& scalar) {
  return DataRate::bits_per_second(std::round(rate.bits_per_second() * scalar));
}
inline DataRate operator*(const double& scalar, const DataRate& rate) {
  return rate * scalar;
}
inline DataRate operator*(const DataRate& rate, const int64_t& scalar) {
  return DataRate::bits_per_second(rate.bits_per_second() * scalar);
}
inline DataRate operator*(const int64_t& scalar, const DataRate& rate) {
  return rate * scalar;
}
inline DataRate operator*(const DataRate& rate, const int32_t& scalar) {
  return DataRate::bits_per_second(rate.bits_per_second() * scalar);
}
inline DataRate operator*(const int32_t& scalar, const DataRate& rate) {
  return rate * scalar;
}

inline DataRate operator/(const DataSize& size, const TimeDelta& duration) {
  return DataRate::bits_per_second(data_rate_impl::Microbits(size) /
                                   duration.us());
}
inline TimeDelta operator/(const DataSize& size, const DataRate& rate) {
  return TimeDelta::us(data_rate_impl::Microbits(size) /
                       rate.bits_per_second());
}
inline DataSize operator*(const DataRate& rate, const TimeDelta& duration) {
  int64_t microbits = rate.bits_per_second() * duration.us();
  return DataSize::bytes((microbits + 4000000) / 8000000);
}
inline DataSize operator*(const TimeDelta& duration, const DataRate& rate) {
  return rate * duration;
}

std::string ToString(const DataRate& value);

}  // namespace webrtc

#endif  // API_UNITS_DATA_RATE_H_
