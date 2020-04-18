/*
 * Copyright (C) 2012 Intel Corporation
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "third_party/blink/renderer/platform/wtf/math_extras.h"

#include "testing/gtest/include/gtest/gtest.h"

namespace WTF {

TEST(MathExtrasTest, Lrint) {
  EXPECT_EQ(-8, lrint(-7.5));
  EXPECT_EQ(-8, lrint(-8.5));
  EXPECT_EQ(0, lrint(-0.5));
  EXPECT_EQ(0, lrint(0.5));
  EXPECT_EQ(0, lrint(-0.5));
  EXPECT_EQ(1, lrint(1.3));
  EXPECT_EQ(2, lrint(1.7));
  EXPECT_EQ(0, lrint(0));
  EXPECT_EQ(0, lrint(-0));
  if (sizeof(long int) == 8) {
    // Largest double number with 0.5 precision and one halfway rounding case
    // below.
    EXPECT_EQ(pow(2.0, 52), lrint(pow(2.0, 52) - 0.5));
    EXPECT_EQ(pow(2.0, 52) - 2, lrint(pow(2.0, 52) - 1.5));
    // Smallest double number with 0.5 precision and one halfway rounding case
    // above.
    EXPECT_EQ(-pow(2.0, 52), lrint(-pow(2.0, 52) + 0.5));
    EXPECT_EQ(-pow(2.0, 52) + 2, lrint(-pow(2.0, 52) + 1.5));
  }
}

TEST(MathExtrasTest, clampToIntLong) {
  if (sizeof(long) == sizeof(int))
    return;

  long max_int = std::numeric_limits<int>::max();
  long min_int = std::numeric_limits<int>::min();
  long overflow_int = max_int + 1;
  long underflow_int = min_int - 1;

  EXPECT_GT(overflow_int, max_int);
  EXPECT_LT(underflow_int, min_int);

  EXPECT_EQ(max_int, clampTo<int>(max_int));
  EXPECT_EQ(min_int, clampTo<int>(min_int));

  EXPECT_EQ(max_int, clampTo<int>(overflow_int));
  EXPECT_EQ(min_int, clampTo<int>(underflow_int));
}

TEST(MathExtrasTest, clampToIntLongLong) {
  long long max_int = std::numeric_limits<int>::max();
  long long min_int = std::numeric_limits<int>::min();
  long long overflow_int = max_int + 1;
  long long underflow_int = min_int - 1;

  EXPECT_GT(overflow_int, max_int);
  EXPECT_LT(underflow_int, min_int);

  EXPECT_EQ(max_int, clampTo<int>(max_int));
  EXPECT_EQ(min_int, clampTo<int>(min_int));

  EXPECT_EQ(max_int, clampTo<int>(overflow_int));
  EXPECT_EQ(min_int, clampTo<int>(underflow_int));
}

TEST(MathExtrasTest, clampToIntFloat) {
  float max_int = static_cast<float>(std::numeric_limits<int>::max());
  float min_int = static_cast<float>(std::numeric_limits<int>::min());
  float overflow_int = max_int * 1.1f;
  float underflow_int = min_int * 1.1f;

  EXPECT_GT(overflow_int, max_int);
  EXPECT_LT(underflow_int, min_int);

  EXPECT_EQ(max_int, clampTo<int>(max_int));
  EXPECT_EQ(min_int, clampTo<int>(min_int));

  EXPECT_EQ(max_int, clampTo<int>(overflow_int));
  EXPECT_EQ(min_int, clampTo<int>(underflow_int));

  // This value and the value one greater are typically represented the same
  // way when stored in a 32-bit float.  Make sure clamping does not cause us
  // to erroneously jump to the larger value.
  int near_float_precision_limit = 2147483520;
  EXPECT_EQ(near_float_precision_limit,
            clampTo<int>(static_cast<float>(near_float_precision_limit), 0,
                         near_float_precision_limit + 1));
  EXPECT_EQ(-near_float_precision_limit,
            clampTo<int>(static_cast<float>(-near_float_precision_limit),
                         -near_float_precision_limit - 1, 0));
}

TEST(MathExtrasTest, clampToIntDouble) {
  int max_int = std::numeric_limits<int>::max();
  int min_int = std::numeric_limits<int>::min();
  double almost_overflow_int = max_int - 0.5;
  double overflow_int = max_int + 0.5;
  double almost_underflow_int = min_int + 0.5;
  double underflow_int = min_int - 0.5;

  EXPECT_LT(almost_overflow_int, max_int);
  EXPECT_GT(overflow_int, max_int);
  EXPECT_GT(almost_underflow_int, min_int);
  EXPECT_LT(underflow_int, min_int);

  EXPECT_EQ(max_int, clampTo<int>(static_cast<double>(max_int)));
  EXPECT_EQ(min_int, clampTo<int>(static_cast<double>(min_int)));

  EXPECT_EQ(max_int - 1, clampTo<int>(almost_overflow_int));
  EXPECT_EQ(max_int, clampTo<int>(overflow_int));
  EXPECT_EQ(min_int + 1, clampTo<int>(almost_underflow_int));
  EXPECT_EQ(min_int, clampTo<int>(underflow_int));
}

TEST(MathExtrasTest, clampToFloatDouble) {
  double max_float = std::numeric_limits<float>::max();
  double min_float = -max_float;
  double overflow_float = max_float * 1.1;
  double underflow_float = min_float * 1.1;

  EXPECT_GT(overflow_float, max_float);
  EXPECT_LT(underflow_float, min_float);

  EXPECT_EQ(max_float, clampTo<float>(max_float));
  EXPECT_EQ(min_float, clampTo<float>(min_float));

  EXPECT_EQ(max_float, clampTo<float>(overflow_float));
  EXPECT_EQ(min_float, clampTo<float>(underflow_float));

  EXPECT_EQ(max_float, clampTo<float>(std::numeric_limits<float>::infinity()));
  EXPECT_EQ(min_float, clampTo<float>(-std::numeric_limits<float>::infinity()));
}

TEST(MathExtrasTest, clampToDouble) {
  EXPECT_EQ(0.0, clampTo<double>(0));
  EXPECT_EQ(0.0, clampTo<double>(0.0f));
  EXPECT_EQ(0.0, clampTo<double>(0ULL));
  EXPECT_EQ(3.5, clampTo<double>(std::numeric_limits<unsigned long long>::max(),
                                 0.0, 3.5));
}

TEST(MathExtrasText, clampToLongLongDouble) {
  double overflow_ll =
      static_cast<double>(std::numeric_limits<long long>::max()) * 2;
  EXPECT_EQ(std::numeric_limits<long long>::max(),
            clampTo<long long>(overflow_ll));
  EXPECT_EQ(std::numeric_limits<long long>::min(),
            clampTo<long long>(-overflow_ll));
}

TEST(MathExtrasText, clampToUnsignedLongLongDouble) {
  double overflow_ull =
      static_cast<double>(std::numeric_limits<unsigned long long>::max()) * 2;
  EXPECT_EQ(std::numeric_limits<unsigned long long>::max(),
            clampTo<unsigned long long>(overflow_ull));
  EXPECT_EQ(std::numeric_limits<unsigned long long>::min(),
            clampTo<unsigned long long>(-overflow_ull));
}

TEST(MathExtrasTest, clampToUnsignedUnsignedLong) {
  if (sizeof(unsigned long) == sizeof(unsigned))
    return;

  unsigned long max_unsigned = std::numeric_limits<unsigned>::max();
  unsigned long overflow_unsigned = max_unsigned + 1;

  EXPECT_GT(overflow_unsigned, max_unsigned);

  EXPECT_EQ(max_unsigned, clampTo<unsigned>(max_unsigned));

  EXPECT_EQ(max_unsigned, clampTo<unsigned>(overflow_unsigned));
  EXPECT_EQ(0u, clampTo<unsigned>(-1));
}

TEST(MathExtrasTest, clampToUnsignedUnsignedLongLong) {
  unsigned long long max_unsigned = std::numeric_limits<unsigned>::max();
  unsigned long long overflow_unsigned = max_unsigned + 1;

  EXPECT_GT(overflow_unsigned, max_unsigned);

  EXPECT_EQ(max_unsigned, clampTo<unsigned>(max_unsigned));

  EXPECT_EQ(max_unsigned, clampTo<unsigned>(overflow_unsigned));
  EXPECT_EQ(0u, clampTo<unsigned>(-1));
}

TEST(MathExtrasTest, clampToLongLongUnsignedLongLong) {
  long long max_long_long_ll = std::numeric_limits<long long>::max();
  unsigned long long max_long_long_ull = max_long_long_ll;
  unsigned long long overflow_long_long = max_long_long_ull + 1;

  EXPECT_GT(overflow_long_long, max_long_long_ull);

  EXPECT_EQ(max_long_long_ll, clampTo<long long>(max_long_long_ull));
  EXPECT_EQ(max_long_long_ll - 1, clampTo<long long>(max_long_long_ull - 1));
  EXPECT_EQ(max_long_long_ll, clampTo<long long>(overflow_long_long));

  EXPECT_EQ(-3LL, clampTo<long long>(2ULL, -5LL, -3LL));
}

TEST(MathExtrasTest, clampToUnsignedLongLongInt) {
  EXPECT_EQ(0ULL, clampTo<unsigned long long>(-1));
  EXPECT_EQ(0ULL, clampTo<unsigned long long>(0));
  EXPECT_EQ(1ULL, clampTo<unsigned long long>(1));
}

TEST(MathExtrasTest, clampToUnsignedLongLongUnsignedLongLong) {
  EXPECT_EQ(0ULL, clampTo<unsigned long long>(0ULL));
  EXPECT_EQ(1ULL, clampTo<unsigned long long>(0ULL, 1ULL, 2ULL));
  EXPECT_EQ(2ULL, clampTo<unsigned long long>(3ULL, 1ULL, 2ULL));
  EXPECT_EQ(0xFFFFFFFFFFFFFFF5ULL,
            clampTo<unsigned long long>(0xFFFFFFFFFFFFFFF5ULL));
}

// Make sure that various +-inf cases are handled properly (they aren't
// by default on VS).
TEST(MathExtrasTest, infinityMath) {
  double pos_inf = std::numeric_limits<double>::infinity();
  double neg_inf = -std::numeric_limits<double>::infinity();
  double nan = std::numeric_limits<double>::quiet_NaN();

  EXPECT_EQ(M_PI_4, atan2(pos_inf, pos_inf));
  EXPECT_EQ(3.0 * M_PI_4, atan2(pos_inf, neg_inf));
  EXPECT_EQ(-M_PI_4, atan2(neg_inf, pos_inf));
  EXPECT_EQ(-3.0 * M_PI_4, atan2(neg_inf, neg_inf));

  EXPECT_EQ(0.0, fmod(0.0, pos_inf));
  EXPECT_EQ(7.0, fmod(7.0, pos_inf));
  EXPECT_EQ(-7.0, fmod(-7.0, pos_inf));
  EXPECT_EQ(0.0, fmod(0.0, neg_inf));
  EXPECT_EQ(7.0, fmod(7.0, neg_inf));
  EXPECT_EQ(-7.0, fmod(-7.0, neg_inf));

  EXPECT_EQ(1.0, pow(5.0, 0.0));
  EXPECT_EQ(1.0, pow(-5.0, 0.0));
  EXPECT_EQ(1.0, pow(nan, 0.0));
}

}  // namespace WTF
