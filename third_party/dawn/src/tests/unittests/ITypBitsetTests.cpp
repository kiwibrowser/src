// Copyright 2020 The Dawn Authors
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

#include "common/TypedInteger.h"
#include "common/ityp_bitset.h"

#include <set>

class ITypBitsetTest : public testing::Test {
  protected:
    using Key = TypedInteger<struct KeyT, size_t>;
    using Bitset = ityp::bitset<Key, 9>;

    // Test that the expected bitset methods can be constexpr
    struct ConstexprTest {
        static constexpr Bitset kBitset = {1 << 0 | 1 << 3 | 1 << 7 | 1 << 8};

        static_assert(kBitset[Key(0)] == true, "");
        static_assert(kBitset[Key(1)] == false, "");
        static_assert(kBitset[Key(2)] == false, "");
        static_assert(kBitset[Key(3)] == true, "");
        static_assert(kBitset[Key(4)] == false, "");
        static_assert(kBitset[Key(5)] == false, "");
        static_assert(kBitset[Key(6)] == false, "");
        static_assert(kBitset[Key(7)] == true, "");
        static_assert(kBitset[Key(8)] == true, "");

        static_assert(kBitset.size() == 9, "");
    };

    void ExpectBits(const Bitset& bits, std::set<size_t> indices) {
        size_t mask = 0;

        for (size_t i = 0; i < bits.size(); ++i) {
            if (indices.count(i) == 0) {
                ASSERT_FALSE(bits[Key(i)]) << i;
                ASSERT_FALSE(bits.test(Key(i))) << i;
            } else {
                mask |= (size_t(1) << i);
                ASSERT_TRUE(bits[Key(i)]) << i;
                ASSERT_TRUE(bits.test(Key(i))) << i;
            }
        }

        ASSERT_EQ(bits.to_ullong(), mask);
        ASSERT_EQ(bits.to_ulong(), mask);
        ASSERT_EQ(bits.count(), indices.size());
        ASSERT_EQ(bits.all(), indices.size() == bits.size());
        ASSERT_EQ(bits.any(), indices.size() != 0);
        ASSERT_EQ(bits.none(), indices.size() == 0);
    }
};

// Test that by default no bits are set
TEST_F(ITypBitsetTest, DefaultZero) {
    Bitset bits;
    ExpectBits(bits, {});
}

// Test the bitset can be initialized with a bitmask
TEST_F(ITypBitsetTest, InitializeByBits) {
    Bitset bits = {1 << 1 | 1 << 2 | 1 << 7};
    ExpectBits(bits, {1, 2, 7});
}

// Test that bits can be set at an index and retrieved from the same index.
TEST_F(ITypBitsetTest, Indexing) {
    Bitset bits;
    ExpectBits(bits, {});

    bits[Key(2)] = true;
    bits[Key(4)] = false;
    bits.set(Key(1));
    bits.set(Key(7), true);
    bits.set(Key(8), false);

    ExpectBits(bits, {1, 2, 7});

    bits.reset(Key(2));
    bits.reset(Key(7));
    ExpectBits(bits, {1});
}

// Test that bits can be flipped
TEST_F(ITypBitsetTest, Flip) {
    Bitset bits = {1 << 1 | 1 << 2 | 1 << 7};
    ExpectBits(bits, {1, 2, 7});

    bits.flip(Key(4));
    bits.flip(Key(1));  // false
    bits.flip(Key(6));
    bits.flip(Key(5));
    ExpectBits(bits, {2, 4, 5, 6, 7});

    bits.flip();
    ExpectBits(bits, {0, 1, 3, 8});

    ExpectBits(~bits, {2, 4, 5, 6, 7});
}

// Test that all the bits can be set/reset.
TEST_F(ITypBitsetTest, SetResetAll) {
    Bitset bits;

    bits.set();

    ASSERT_EQ(bits.count(), 9u);
    ASSERT_TRUE(bits.all());
    ASSERT_TRUE(bits.any());
    ASSERT_FALSE(bits.none());

    for (Key i(0); i < Key(9); ++i) {
        ASSERT_TRUE(bits[i]);
    }

    bits.reset();

    ASSERT_EQ(bits.count(), 0u);
    ASSERT_FALSE(bits.all());
    ASSERT_FALSE(bits.any());
    ASSERT_TRUE(bits.none());

    for (Key i(0); i < Key(9); ++i) {
        ASSERT_FALSE(bits[i]);
    }
}

// Test And operations
TEST_F(ITypBitsetTest, And) {
    Bitset bits = {1 << 1 | 1 << 2 | 1 << 7};
    ExpectBits(bits, {1, 2, 7});

    Bitset bits2 = bits & Bitset{1 << 0 | 1 << 3 | 1 << 7};
    ExpectBits(bits2, {7});
    ExpectBits(bits, {1, 2, 7});

    bits &= Bitset{1 << 1 | 1 << 6};
    ExpectBits(bits, {1});
}

// Test Or operations
TEST_F(ITypBitsetTest, Or) {
    Bitset bits = {1 << 1 | 1 << 2 | 1 << 7};
    ExpectBits(bits, {1, 2, 7});

    Bitset bits2 = bits | Bitset{1 << 0 | 1 << 3 | 1 << 7};
    ExpectBits(bits2, {0, 1, 2, 3, 7});
    ExpectBits(bits, {1, 2, 7});

    bits |= Bitset{1 << 1 | 1 << 6};
    ExpectBits(bits, {1, 2, 6, 7});
}

// Test xor operations
TEST_F(ITypBitsetTest, Xor) {
    Bitset bits = {1 << 1 | 1 << 2 | 1 << 7};
    ExpectBits(bits, {1, 2, 7});

    Bitset bits2 = bits ^ Bitset { 1 << 0 | 1 << 3 | 1 << 7 };
    ExpectBits(bits2, {0, 1, 2, 3});
    ExpectBits(bits, {1, 2, 7});

    bits ^= Bitset{1 << 1 | 1 << 6};
    ExpectBits(bits, {2, 6, 7});
}
