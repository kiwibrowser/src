// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_ZUCCHINI_ALGORITHM_H_
#define COMPONENTS_ZUCCHINI_ALGORITHM_H_

#include <stddef.h>

#include <algorithm>
#include <type_traits>
#include <vector>

#include "base/logging.h"

// Collection of simple utilities used in for low-level computation.

namespace zucchini {

// Safely determines whether |[begin, begin + size)| is in |[0, bound)|. Note:
// The special case |[bound, bound)| is not considered to be in |[0, bound)|.
template <typename T>
bool RangeIsBounded(T begin, T size, size_t bound) {
  static_assert(std::is_unsigned<T>::value, "Value type must be unsigned.");
  return begin < bound && size <= bound - begin;
}

// Safely determines whether |value| lies in |[begin, begin + size)|. Works
// properly even if |begin + size| overflows -- although such ranges are
// considered pathological, and should fail validation elsewhere.
template <typename T>
bool RangeCovers(T begin, T size, T value) {
  static_assert(std::is_unsigned<T>::value, "Value type must be unsigned.");
  return begin <= value && value - begin < size;
}

// Returns the integer in inclusive range |[lo, hi]| that's closest to |value|.
// This departs from the usual usage of semi-inclusive ranges, but is useful
// because (1) sentinels can use this, (2) a valid output always exists. It is
// assumed that |lo <= hi|.
template <class T>
T InclusiveClamp(T value, T lo, T hi) {
  static_assert(std::is_unsigned<T>::value, "Value type must be unsigned.");
  DCHECK_LE(lo, hi);
  return value <= lo ? lo : (value >= hi ? hi : value);
}

// Returns the minimum multiple of |m| that's no less than |x|. Assumes |m > 0|
// and |x| is sufficiently small so that no overflow occurs.
template <class T>
constexpr T ceil(T x, T m) {
  static_assert(std::is_unsigned<T>::value, "Value type must be unsigned.");
  return T((x + m - 1) / m) * m;
}

// Sorts values in |container| and removes duplicates.
template <class T>
void SortAndUniquify(std::vector<T>* container) {
  std::sort(container->begin(), container->end());
  container->erase(std::unique(container->begin(), container->end()),
                   container->end());
  container->shrink_to_fit();
}

// Copies bits at |pos| in |v| to all higher bits, and returns the result as the
// same int type as |v|.
template <typename T>
constexpr T SignExtend(int pos, T v) {
  int kNumBits = sizeof(T) * 8;
  int kShift = kNumBits - 1 - pos;
  return static_cast<typename std::make_signed<T>::type>(v << kShift) >> kShift;
}

// Optimized version where |pos| becomes a template parameter.
template <int pos, typename T>
constexpr T SignExtend(T v) {
  constexpr int kNumBits = sizeof(T) * 8;
  constexpr int kShift = kNumBits - 1 - pos;
  return static_cast<typename std::make_signed<T>::type>(v << kShift) >> kShift;
}

}  // namespace zucchini

#endif  // COMPONENTS_ZUCCHINI_ALGORITHM_H_
