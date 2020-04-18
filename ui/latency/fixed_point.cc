// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/latency/fixed_point.h"

#include <cmath>
#include <limits>

#include "base/logging.h"

namespace ui {
namespace frame_metrics {

namespace {

constexpr uint64_t k2Pow32{1ULL << 32};
constexpr uint64_t k32LsbMask{0xFFFFFFFF};

}  // namespace

Accumulator96b::Accumulator96b(uint32_t value_to_square, uint32_t weight) {
  uint64_t square = static_cast<uint64_t>(value_to_square) * value_to_square;
  uint64_t ms64b_temp = (square >> 32) * weight;
  uint64_t ls32b_temp = (square & k32LsbMask) * weight;
  ms64b = ms64b_temp + (ls32b_temp >> 32);
  ls32b = ls32b_temp & k32LsbMask;
}

void Accumulator96b::Add(const Accumulator96b& rhs) {
  uint64_t ls32b_temp = static_cast<uint64_t>(ls32b) + rhs.ls32b;
  DCHECK_LT((ls32b_temp >> 32),
            std::numeric_limits<decltype(ms64b)>::max() - rhs.ms64b)
      << "Accumulator96b overflow.";
  uint64_t ms64b_add = rhs.ms64b + (ls32b_temp >> 32);
  DCHECK_LT(ms64b_add, std::numeric_limits<decltype(ms64b)>::max() - ms64b)
      << "Accumulator96b overflow.";
  ms64b += ms64b_add;
  ls32b = ls32b_temp & k32LsbMask;
}

void Accumulator96b::Subtract(const Accumulator96b& rhs) {
  uint64_t ls32b_temp = ls32b;
  if (ls32b < rhs.ls32b) {
    // Borrow from ms64b to ls32b.
    ms64b--;
    ls32b_temp |= k2Pow32;
  }
  DCHECK_GE(ms64b, rhs.ms64b) << "Accumulator96b underflow.";
  DCHECK_GE(ls32b_temp, static_cast<uint64_t>(rhs.ls32b))
      << "Accumulator96b underflow.";
  ms64b -= rhs.ms64b;
  ls32b = ls32b_temp - rhs.ls32b;
}

double Accumulator96b::ToDouble() const {
  return (static_cast<double>(ms64b) * k2Pow32) + ls32b;
}

}  // namespace frame_metrics
}  // namespace ui
