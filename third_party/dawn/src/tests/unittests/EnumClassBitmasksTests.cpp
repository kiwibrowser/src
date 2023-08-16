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

#include "gtest/gtest.h"

#include "dawn/EnumClassBitmasks.h"

namespace wgpu {

    enum class Color : uint32_t {
        R = 1,
        G = 2,
        B = 4,
        A = 8,
    };

    template <>
    struct IsDawnBitmask<Color> {
        static constexpr bool enable = true;
    };

    TEST(BitmaskTests, BasicOperations) {
        Color test1 = Color::R | Color::G;
        ASSERT_EQ(1u | 2u, static_cast<uint32_t>(test1));

        Color test2 = test1 ^ (Color::R | Color::A);
        ASSERT_EQ(2u | 8u, static_cast<uint32_t>(test2));

        Color test3 = test2 & Color::A;
        ASSERT_EQ(8u, static_cast<uint32_t>(test3));

        Color test4 = ~test3;
        ASSERT_EQ(~uint32_t(8), static_cast<uint32_t>(test4));
    }

    TEST(BitmaskTests, AssignOperations) {
        Color test1 = Color::R;
        test1 |= Color::G;
        ASSERT_EQ(1u | 2u, static_cast<uint32_t>(test1));

        Color test2 = test1;
        test2 ^= (Color::R | Color::A);
        ASSERT_EQ(2u | 8u, static_cast<uint32_t>(test2));

        Color test3 = test2;
        test3 &= Color::A;
        ASSERT_EQ(8u, static_cast<uint32_t>(test3));
    }

    TEST(BitmaskTests, BoolConversion) {
        bool test1 = Color::R | Color::G;
        ASSERT_TRUE(test1);

        bool test2 = Color::R & Color::G;
        ASSERT_FALSE(test2);

        bool test3 = Color::R ^ Color::G;
        ASSERT_TRUE(test3);

        if (Color::R & ~Color::R) {
            ASSERT_TRUE(false);
        }
    }

    TEST(BitmaskTests, ThreeOrs) {
        Color c = Color::R | Color::G | Color::B;
        ASSERT_EQ(7u, static_cast<uint32_t>(c));
    }

    TEST(BitmaskTests, ZeroOrOneBits) {
        Color zero = static_cast<Color>(0);
        ASSERT_TRUE(wgpu::HasZeroOrOneBits(zero));
        ASSERT_TRUE(wgpu::HasZeroOrOneBits(Color::R));
        ASSERT_TRUE(wgpu::HasZeroOrOneBits(Color::G));
        ASSERT_TRUE(wgpu::HasZeroOrOneBits(Color::B));
        ASSERT_TRUE(wgpu::HasZeroOrOneBits(Color::A));
        ASSERT_FALSE(wgpu::HasZeroOrOneBits(static_cast<Color>(Color::R | Color::G)));
        ASSERT_FALSE(wgpu::HasZeroOrOneBits(static_cast<Color>(Color::G | Color::B)));
        ASSERT_FALSE(wgpu::HasZeroOrOneBits(static_cast<Color>(Color::B | Color::A)));
    }

}  // namespace wgpu
