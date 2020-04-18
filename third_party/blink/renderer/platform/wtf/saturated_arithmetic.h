/*
 * Copyright (c) 2012, Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef THIRD_PARTY_BLINK_RENDERER_PLATFORM_WTF_SATURATED_ARITHMETIC_H_
#define THIRD_PARTY_BLINK_RENDERER_PLATFORM_WTF_SATURATED_ARITHMETIC_H_

#include "base/compiler_specific.h"
#include "base/numerics/clamped_math.h"
#include "build/build_config.h"

#include <stdint.h>

#include <limits>

#if defined(ARCH_CPU_ARM_FAMILY) && defined(ARCH_CPU_32_BITS) && \
    defined(COMPILER_GCC) && !defined(OS_NACL) && __OPTIMIZE__

// If we're building ARM 32-bit on GCC we replace the C++ versions with some
// native ARM assembly for speed.
#include "third_party/blink/renderer/platform/wtf/saturated_arithmetic_arm.h"

#else

namespace WTF {

ALWAYS_INLINE int GetMaxSaturatedSetResultForTesting(int fractional_shift) {
  // For C version the set function maxes out to max int, this differs from
  // the ARM asm version, see saturated_arithmetic_arm.h for the equivalent asm
  // version.
  return std::numeric_limits<int>::max();
}

ALWAYS_INLINE int GetMinSaturatedSetResultForTesting(int fractional_shift) {
  return std::numeric_limits<int>::min();
}

template <int fractional_shift>
ALWAYS_INLINE int SaturatedSet(int value) {
  const int kIntMaxForLayoutUnit =
      std::numeric_limits<int>::max() >> fractional_shift;

  const int kIntMinForLayoutUnit =
      std::numeric_limits<int>::min() >> fractional_shift;

  if (value > kIntMaxForLayoutUnit)
    return std::numeric_limits<int>::max();

  if (value < kIntMinForLayoutUnit)
    return std::numeric_limits<int>::min();

  return static_cast<unsigned>(value) << fractional_shift;
}

template <int fractional_shift>
ALWAYS_INLINE int SaturatedSet(unsigned value) {
  const unsigned kIntMaxForLayoutUnit =
      std::numeric_limits<int>::max() >> fractional_shift;

  if (value >= kIntMaxForLayoutUnit)
    return std::numeric_limits<int>::max();

  return value << fractional_shift;
}

}  // namespace WTF.

#endif  // CPU(ARM) && COMPILER(GCC)

namespace WTF {
using base::ClampAdd;
using base::ClampSub;
using base::MakeClampedNum;
}  // namespace WTF.

using WTF::ClampAdd;
using WTF::ClampSub;
using WTF::MakeClampedNum;
using WTF::SaturatedSet;

#endif  // THIRD_PARTY_BLINK_RENDERER_PLATFORM_WTF_SATURATED_ARITHMETIC_H_
