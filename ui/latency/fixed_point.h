// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_FRAME_METRICS_FIXED_POINT_H_
#define UI_FRAME_METRICS_FIXED_POINT_H_

#include <cstdint>

#include "base/macros.h"

namespace ui {
namespace frame_metrics {

// Use fixed point math so we can manage our precision explicitly and avoid
// accumulating error in our windowed accumulators.
// The 64-bit accumulators reserve 32-bits for weights, and 32-bits for values.
// The 32-bit values reserve 16 bits before and after the radix point.
constexpr int kFixedPointShift = 16;
constexpr int64_t kFixedPointMultiplier{1LL << kFixedPointShift};

// kFixedPointRootMultiplier is used to shift the bits before taking the square
// root and undoing that shift after squaring in the SMR calculation.
constexpr int kFixedPointRootShift = 32;
constexpr int64_t kFixedPointRootMultiplier{1LL << kFixedPointRootShift};
constexpr int64_t kFixedPointRootMultiplierSqrt{1LL
                                                << (kFixedPointRootShift / 2)};

// We need a huge range to accumulate values for RMS calculations, which
// need double the range internally compared to the range we are targeting
// after taking the square root of the accumulation.
// This efficiently emulates a 96-bit unsigned integer with weighted
// accumulation operations.
// 32-bits are reserved for weights and 64-bits for squared values.
// Overflow or underflow indicates something is seriously wrong with the higher
// level metrics logic, so  this class will DCHECK if it anticipates overflow
// or underflow:
// * It doesn't need to support OVERFLOW since the frame metric classes will
//   always reset the entire accumulator before the accumulated weights
//   overflow. The accumulated weights correspond to a maximum of the number of
//   microseconds since the last reset, which for a 32-bit weight is about
//   1 hour. We will gather and reset results much more often than every hour.
// * It doesn't need to support UNDERFLOW since only the windowed metrics use
//   Subtract, and those only subtract values it has already added.
class Accumulator96b {
 public:
  Accumulator96b() = default;
  Accumulator96b(uint32_t value_to_square, uint32_t weight);

  void Add(const Accumulator96b& rhs);
  void Subtract(const Accumulator96b& rhs);
  double ToDouble() const;

 public:
  uint64_t ms64b{0};
  uint32_t ls32b{0};
};

// Convenience function overloads for AsDouble, to help with templated code.
inline double AsDouble(const Accumulator96b& value) {
  return value.ToDouble();
}

inline double AsDouble(double value) {
  return value;
}

}  // namespace frame_metrics
}  // namespace ui

#endif  // UI_FRAME_METRICS_FIXED_POINT_H_
