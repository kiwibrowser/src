/*
 *  Copyright (c) 2018 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#ifndef API_UNITS_DATA_SIZE_H_
#define API_UNITS_DATA_SIZE_H_

#include <stdint.h>
#include <cmath>
#include <limits>
#include <string>

#include "rtc_base/checks.h"

namespace webrtc {
namespace data_size_impl {
constexpr int64_t kPlusInfinityVal = std::numeric_limits<int64_t>::max();
}  // namespace data_size_impl

// DataSize is a class represeting a count of bytes.
class DataSize {
 public:
  DataSize() = delete;
  static DataSize Zero() { return DataSize(0); }
  static DataSize Infinity() {
    return DataSize(data_size_impl::kPlusInfinityVal);
  }
  static DataSize bytes(int64_t bytes) {
    RTC_DCHECK_GE(bytes, 0);
    return DataSize(bytes);
  }
  int64_t bytes() const {
    RTC_DCHECK(IsFinite());
    return bytes_;
  }
  int64_t kilobytes() const { return (bytes() + 500) / 1000; }
  bool IsZero() const { return bytes_ == 0; }
  bool IsInfinite() const { return bytes_ == data_size_impl::kPlusInfinityVal; }
  bool IsFinite() const { return !IsInfinite(); }
  DataSize operator-(const DataSize& other) const {
    return DataSize::bytes(bytes() - other.bytes());
  }
  DataSize operator+(const DataSize& other) const {
    return DataSize::bytes(bytes() + other.bytes());
  }
  DataSize& operator-=(const DataSize& other) {
    bytes_ -= other.bytes();
    return *this;
  }
  DataSize& operator+=(const DataSize& other) {
    bytes_ += other.bytes();
    return *this;
  }
  bool operator==(const DataSize& other) const {
    return bytes_ == other.bytes_;
  }
  bool operator!=(const DataSize& other) const {
    return bytes_ != other.bytes_;
  }
  bool operator<=(const DataSize& other) const {
    return bytes_ <= other.bytes_;
  }
  bool operator>=(const DataSize& other) const {
    return bytes_ >= other.bytes_;
  }
  bool operator>(const DataSize& other) const { return bytes_ > other.bytes_; }
  bool operator<(const DataSize& other) const { return bytes_ < other.bytes_; }

 private:
  explicit DataSize(int64_t bytes) : bytes_(bytes) {}
  int64_t bytes_;
};
inline DataSize operator*(const DataSize& size, const double& scalar) {
  return DataSize::bytes(std::round(size.bytes() * scalar));
}
inline DataSize operator*(const double& scalar, const DataSize& size) {
  return size * scalar;
}
inline DataSize operator*(const DataSize& size, const int64_t& scalar) {
  return DataSize::bytes(size.bytes() * scalar);
}
inline DataSize operator*(const int64_t& scalar, const DataSize& size) {
  return size * scalar;
}
inline DataSize operator*(const DataSize& size, const int32_t& scalar) {
  return DataSize::bytes(size.bytes() * scalar);
}
inline DataSize operator*(const int32_t& scalar, const DataSize& size) {
  return size * scalar;
}
inline DataSize operator/(const DataSize& size, const int64_t& scalar) {
  return DataSize::bytes(size.bytes() / scalar);
}

std::string ToString(const DataSize& value);

}  // namespace webrtc

#endif  // API_UNITS_DATA_SIZE_H_
