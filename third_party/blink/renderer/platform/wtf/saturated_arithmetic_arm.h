// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_PLATFORM_WTF_SATURATED_ARITHMETIC_ARM_H_
#define THIRD_PARTY_BLINK_RENDERER_PLATFORM_WTF_SATURATED_ARITHMETIC_ARM_H_

#include <limits>

namespace WTF {

inline int GetMaxSaturatedSetResultForTesting(int fractional_shift) {
  // For ARM Asm version the set function maxes out to the biggest
  // possible integer part with the fractional part zero'd out.
  // e.g. 0x7fffffc0.
  return std::numeric_limits<int>::max() & ~((1 << fractional_shift) - 1);
}

inline int GetMinSaturatedSetResultForTesting(int fractional_shift) {
  return std::numeric_limits<int>::min();
}

template <int fractional_shift>
inline int SaturatedSet(int value) {
  // Figure out how many bits are left for storing the integer part of
  // the fixed point number, and saturate our input to that
  enum { Saturate = 32 - fractional_shift };

  int result;

  // The following ARM code will Saturate the passed value to the number of
  // bits used for the whole part of the fixed point representation, then
  // shift it up into place. This will result in the low <FractionShift> bits
  // all being 0's. When the value saturates this gives a different result
  // to from the C++ case; in the C++ code a saturated value has all the low
  // bits set to 1 (for a +ve number at least). This cannot be done rapidly
  // in ARM ... we live with the difference, for the sake of speed.

  asm("ssat %[output],%[saturate],%[value]\n\t"
      "lsl  %[output],%[shift]"
      : [output] "=r"(result)
      : [value] "r"(value), [saturate] "n"(Saturate),
        [shift] "n"(fractional_shift));

  return result;
}

template <int fractional_shift>
inline int SaturatedSet(unsigned value) {
  // Here we are being passed an unsigned value to saturate,
  // even though the result is returned as a signed integer. The ARM
  // instruction for unsigned saturation therefore needs to be given one
  // less bit (i.e. the sign bit) for the saturation to work correctly; hence
  // the '31' below.
  enum { Saturate = 31 - fractional_shift };

  // The following ARM code will Saturate the passed value to the number of
  // bits used for the whole part of the fixed point representation, then
  // shift it up into place. This will result in the low <FractionShift> bits
  // all being 0's. When the value saturates this gives a different result
  // to from the C++ case; in the C++ code a saturated value has all the low
  // bits set to 1. This cannot be done rapidly in ARM, so we live with the
  // difference, for the sake of speed.

  int result;

  asm("usat %[output],%[saturate],%[value]\n\t"
      "lsl  %[output],%[shift]"
      : [output] "=r"(result)
      : [value] "r"(value), [saturate] "n"(Saturate),
        [shift] "n"(fractional_shift));

  return result;
}

}  // namespace WTF

#endif  // THIRD_PARTY_BLINK_RENDERER_PLATFORM_WTF_SATURATED_ARITHMETIC_ARM_H_
