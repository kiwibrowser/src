// Copyright 2017 The Dawn Authors
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <gtest/gtest.h>

#include "common/Math.h"

#include <cmath>

// Tests for ScanForward
TEST(Math, ScanForward) {
    // Test extrema
    ASSERT_EQ(ScanForward(1), 0u);
    ASSERT_EQ(ScanForward(0x80000000), 31u);

    // Test with more than one bit set.
    ASSERT_EQ(ScanForward(256), 8u);
    ASSERT_EQ(ScanForward(256 + 32), 5u);
    ASSERT_EQ(ScanForward(1024 + 256 + 32), 5u);
}

// Tests for Log2
TEST(Math, Log2) {
    // Test extrema
    ASSERT_EQ(Log2(1u), 0u);
    ASSERT_EQ(Log2(0xFFFFFFFFu), 31u);
    ASSERT_EQ(Log2(static_cast<uint64_t>(0xFFFFFFFFFFFFFFFF)), 63u);

    static_assert(ConstexprLog2(1u) == 0u, "");
    static_assert(ConstexprLog2(0xFFFFFFFFu) == 31u, "");
    static_assert(ConstexprLog2(static_cast<uint64_t>(0xFFFFFFFFFFFFFFFF)) == 63u, "");

    // Test boundary between two logs
    ASSERT_EQ(Log2(0x80000000u), 31u);
    ASSERT_EQ(Log2(0x7FFFFFFFu), 30u);
    ASSERT_EQ(Log2(static_cast<uint64_t>(0x8000000000000000)), 63u);
    ASSERT_EQ(Log2(static_cast<uint64_t>(0x7FFFFFFFFFFFFFFF)), 62u);

    static_assert(ConstexprLog2(0x80000000u) == 31u, "");
    static_assert(ConstexprLog2(0x7FFFFFFFu) == 30u, "");
    static_assert(ConstexprLog2(static_cast<uint64_t>(0x8000000000000000)) == 63u, "");
    static_assert(ConstexprLog2(static_cast<uint64_t>(0x7FFFFFFFFFFFFFFF)) == 62u, "");

    ASSERT_EQ(Log2(16u), 4u);
    ASSERT_EQ(Log2(15u), 3u);

    static_assert(ConstexprLog2(16u) == 4u, "");
    static_assert(ConstexprLog2(15u) == 3u, "");
}

// Tests for Log2Ceil
TEST(Math, Log2Ceil) {
    // Test extrema
    ASSERT_EQ(Log2Ceil(1u), 0u);
    ASSERT_EQ(Log2Ceil(0xFFFFFFFFu), 32u);
    ASSERT_EQ(Log2Ceil(static_cast<uint64_t>(0xFFFFFFFFFFFFFFFF)), 64u);

    static_assert(ConstexprLog2Ceil(1u) == 0u, "");
    static_assert(ConstexprLog2Ceil(0xFFFFFFFFu) == 32u, "");
    static_assert(ConstexprLog2Ceil(static_cast<uint64_t>(0xFFFFFFFFFFFFFFFF)) == 64u, "");

    // Test boundary between two logs
    ASSERT_EQ(Log2Ceil(0x80000001u), 32u);
    ASSERT_EQ(Log2Ceil(0x80000000u), 31u);
    ASSERT_EQ(Log2Ceil(0x7FFFFFFFu), 31u);
    ASSERT_EQ(Log2Ceil(static_cast<uint64_t>(0x8000000000000001)), 64u);
    ASSERT_EQ(Log2Ceil(static_cast<uint64_t>(0x8000000000000000)), 63u);
    ASSERT_EQ(Log2Ceil(static_cast<uint64_t>(0x7FFFFFFFFFFFFFFF)), 63u);

    static_assert(ConstexprLog2Ceil(0x80000001u) == 32u, "");
    static_assert(ConstexprLog2Ceil(0x80000000u) == 31u, "");
    static_assert(ConstexprLog2Ceil(0x7FFFFFFFu) == 31u, "");
    static_assert(ConstexprLog2Ceil(static_cast<uint64_t>(0x8000000000000001)) == 64u, "");
    static_assert(ConstexprLog2Ceil(static_cast<uint64_t>(0x8000000000000000)) == 63u, "");
    static_assert(ConstexprLog2Ceil(static_cast<uint64_t>(0x7FFFFFFFFFFFFFFF)) == 63u, "");

    ASSERT_EQ(Log2Ceil(17u), 5u);
    ASSERT_EQ(Log2Ceil(16u), 4u);
    ASSERT_EQ(Log2Ceil(15u), 4u);

    static_assert(ConstexprLog2Ceil(17u) == 5u, "");
    static_assert(ConstexprLog2Ceil(16u) == 4u, "");
    static_assert(ConstexprLog2Ceil(15u) == 4u, "");
}

// Tests for IsPowerOfTwo
TEST(Math, IsPowerOfTwo) {
    ASSERT_TRUE(IsPowerOfTwo(1));
    ASSERT_TRUE(IsPowerOfTwo(2));
    ASSERT_FALSE(IsPowerOfTwo(3));

    ASSERT_TRUE(IsPowerOfTwo(0x8000000));
    ASSERT_FALSE(IsPowerOfTwo(0x8000400));
}

// Tests for NextPowerOfTwo
TEST(Math, NextPowerOfTwo) {
    // Test extrema
    ASSERT_EQ(NextPowerOfTwo(0), 1ull);
    ASSERT_EQ(NextPowerOfTwo(0x7FFFFFFFFFFFFFFF), 0x8000000000000000);

    // Test boundary between powers-of-two.
    ASSERT_EQ(NextPowerOfTwo(31), 32ull);
    ASSERT_EQ(NextPowerOfTwo(33), 64ull);

    ASSERT_EQ(NextPowerOfTwo(32), 32ull);
}

// Tests for AlignPtr
TEST(Math, AlignPtr) {
    constexpr size_t kTestAlignment = 8;

    char buffer[kTestAlignment * 4];

    for (size_t i = 0; i < 2 * kTestAlignment; ++i) {
        char* unaligned = &buffer[i];
        char* aligned = AlignPtr(unaligned, kTestAlignment);

        ASSERT_GE(aligned - unaligned, 0);
        ASSERT_LT(static_cast<size_t>(aligned - unaligned), kTestAlignment);
        ASSERT_EQ(reinterpret_cast<uintptr_t>(aligned) & (kTestAlignment - 1), 0u);
    }
}

// Tests for Align
TEST(Math, Align) {
    // 0 aligns to 0
    ASSERT_EQ(Align(0, 4), 0u);
    ASSERT_EQ(Align(0, 256), 0u);
    ASSERT_EQ(Align(0, 512), 0u);

    // Multiples align to self
    ASSERT_EQ(Align(8, 8), 8u);
    ASSERT_EQ(Align(16, 8), 16u);
    ASSERT_EQ(Align(24, 8), 24u);
    ASSERT_EQ(Align(256, 256), 256u);
    ASSERT_EQ(Align(512, 256), 512u);
    ASSERT_EQ(Align(768, 256), 768u);

    // Alignment with 1 is self
    for (uint32_t i = 0; i < 128; ++i) {
        ASSERT_EQ(Align(i, 1), i);
    }

    // Everything in the range (align, 2*align] aligns to 2*align
    for (uint32_t i = 1; i <= 64; ++i) {
        ASSERT_EQ(Align(64 + i, 64), 128u);
    }
}

// Tests for IsPtrAligned
TEST(Math, IsPtrAligned) {
    constexpr size_t kTestAlignment = 8;

    char buffer[kTestAlignment * 4];

    for (size_t i = 0; i < 2 * kTestAlignment; ++i) {
        char* unaligned = &buffer[i];
        char* aligned = AlignPtr(unaligned, kTestAlignment);

        ASSERT_EQ(IsPtrAligned(unaligned, kTestAlignment), unaligned == aligned);
    }
}

// Tests for IsAligned
TEST(Math, IsAligned) {
    // 0 is aligned
    ASSERT_TRUE(IsAligned(0, 4));
    ASSERT_TRUE(IsAligned(0, 256));
    ASSERT_TRUE(IsAligned(0, 512));

    // Multiples are aligned
    ASSERT_TRUE(IsAligned(8, 8));
    ASSERT_TRUE(IsAligned(16, 8));
    ASSERT_TRUE(IsAligned(24, 8));
    ASSERT_TRUE(IsAligned(256, 256));
    ASSERT_TRUE(IsAligned(512, 256));
    ASSERT_TRUE(IsAligned(768, 256));

    // Alignment with 1 is always aligned
    for (uint32_t i = 0; i < 128; ++i) {
        ASSERT_TRUE(IsAligned(i, 1));
    }

    // Everything in the range (align, 2*align) is not aligned
    for (uint32_t i = 1; i < 64; ++i) {
        ASSERT_FALSE(IsAligned(64 + i, 64));
    }
}

// Tests for float32 to float16 conversion
TEST(Math, Float32ToFloat16) {
    ASSERT_EQ(Float32ToFloat16(0.0f), 0x0000);
    ASSERT_EQ(Float32ToFloat16(-0.0f), 0x8000);

    ASSERT_EQ(Float32ToFloat16(INFINITY), 0x7C00);
    ASSERT_EQ(Float32ToFloat16(-INFINITY), 0xFC00);

    // Check that NaN is converted to a value in one of the float16 NaN ranges
    uint16_t nan16 = Float32ToFloat16(NAN);
    ASSERT_TRUE(nan16 > 0xFC00 || (nan16 < 0x8000 && nan16 > 0x7C00));

    ASSERT_EQ(Float32ToFloat16(1.0f), 0x3C00);
}

// Tests for IsFloat16NaN
TEST(Math, IsFloat16NaN) {
    ASSERT_FALSE(IsFloat16NaN(0u));
    ASSERT_FALSE(IsFloat16NaN(0u));
    ASSERT_FALSE(IsFloat16NaN(Float32ToFloat16(1.0f)));
    ASSERT_FALSE(IsFloat16NaN(Float32ToFloat16(INFINITY)));
    ASSERT_FALSE(IsFloat16NaN(Float32ToFloat16(-INFINITY)));

    ASSERT_TRUE(IsFloat16NaN(Float32ToFloat16(INFINITY) + 1));
    ASSERT_TRUE(IsFloat16NaN(Float32ToFloat16(-INFINITY) + 1));
    ASSERT_TRUE(IsFloat16NaN(0x7FFF));
    ASSERT_TRUE(IsFloat16NaN(0xFFFF));
}

// Tests for SRGBToLinear
TEST(Math, SRGBToLinear) {
    ASSERT_EQ(SRGBToLinear(0.0f), 0.0f);
    ASSERT_EQ(SRGBToLinear(1.0f), 1.0f);

    ASSERT_EQ(SRGBToLinear(-1.0f), 0.0f);
    ASSERT_EQ(SRGBToLinear(2.0f), 1.0f);

    ASSERT_FLOAT_EQ(SRGBToLinear(0.5f), 0.21404114f);
}

// Tests for RoundUp
TEST(Math, RoundUp) {
    ASSERT_EQ(RoundUp(2, 2), 2u);
    ASSERT_EQ(RoundUp(2, 4), 4u);
    ASSERT_EQ(RoundUp(6, 2), 6u);
    ASSERT_EQ(RoundUp(8, 4), 8u);
    ASSERT_EQ(RoundUp(12, 6), 12u);

    ASSERT_EQ(RoundUp(3, 3), 3u);
    ASSERT_EQ(RoundUp(3, 5), 5u);
    ASSERT_EQ(RoundUp(5, 3), 6u);
    ASSERT_EQ(RoundUp(9, 5), 10u);

    // Test extrema
    ASSERT_EQ(RoundUp(0x7FFFFFFFFFFFFFFFull, 0x8000000000000000ull), 0x8000000000000000ull);
    ASSERT_EQ(RoundUp(1, 1), 1u);
}
