// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <limits>

#include "base/logging.h"
#include "base/macros.h"
#include "build/build_config.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/blink/renderer/platform/wtf/saturated_arithmetic.h"

namespace WTF {

TEST(SaturatedArithmeticTest, SetSigned) {
  int int_max = std::numeric_limits<int>::max();
  int int_min = std::numeric_limits<int>::min();

  const int kFractionBits = 6;
  const int kIntMaxForLayoutUnit = int_max >> kFractionBits;
  const int kIntMinForLayoutUnit = int_min >> kFractionBits;

  EXPECT_EQ(0, SaturatedSet<kFractionBits>(0));

  // Internally the max number we can represent (without saturating)
  // is all the (non-sign) bits set except for the bottom n fraction bits
  const int max_internal_representation = int_max ^ ((1 << kFractionBits) - 1);
  EXPECT_EQ(max_internal_representation,
            SaturatedSet<kFractionBits>(kIntMaxForLayoutUnit));

  EXPECT_EQ(GetMaxSaturatedSetResultForTesting(kFractionBits),
            SaturatedSet<kFractionBits>(kIntMaxForLayoutUnit + 100));

  EXPECT_EQ((kIntMaxForLayoutUnit - 100) << kFractionBits,
            SaturatedSet<kFractionBits>(kIntMaxForLayoutUnit - 100));

  EXPECT_EQ(GetMinSaturatedSetResultForTesting(kFractionBits),
            SaturatedSet<kFractionBits>(kIntMinForLayoutUnit));

  EXPECT_EQ(GetMinSaturatedSetResultForTesting(kFractionBits),
            SaturatedSet<kFractionBits>(kIntMinForLayoutUnit - 100));

  // Shifting negative numbers left has undefined behavior, so use
  // multiplication instead of direct shifting here.
  EXPECT_EQ((kIntMinForLayoutUnit + 100) * (1 << kFractionBits),
            SaturatedSet<kFractionBits>(kIntMinForLayoutUnit + 100));
}

TEST(SaturatedArithmeticTest, SetUnsigned) {
  int int_max = std::numeric_limits<int>::max();

  const int kFractionBits = 6;
  const int kIntMaxForLayoutUnit = int_max >> kFractionBits;

  EXPECT_EQ(0, SaturatedSet<kFractionBits>((unsigned)0));

  EXPECT_EQ(GetMaxSaturatedSetResultForTesting(kFractionBits),
            SaturatedSet<kFractionBits>((unsigned)kIntMaxForLayoutUnit));

  const unsigned kOverflowed = kIntMaxForLayoutUnit + 100;
  EXPECT_EQ(GetMaxSaturatedSetResultForTesting(kFractionBits),
            SaturatedSet<kFractionBits>(kOverflowed));

  const unsigned kNotOverflowed = kIntMaxForLayoutUnit - 100;
  EXPECT_EQ((kIntMaxForLayoutUnit - 100) << kFractionBits,
            SaturatedSet<kFractionBits>(kNotOverflowed));
}

}  // namespace WTF
