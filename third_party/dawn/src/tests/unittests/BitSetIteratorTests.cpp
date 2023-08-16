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

#include "common/BitSetIterator.h"
#include "common/ityp_bitset.h"

// This is ANGLE's BitSetIterator_unittests.cpp file.

class BitSetIteratorTest : public testing::Test {
  protected:
    std::bitset<40> mStateBits;
};

// Simple iterator test.
TEST_F(BitSetIteratorTest, Iterator) {
    std::set<unsigned long> originalValues;
    originalValues.insert(2);
    originalValues.insert(6);
    originalValues.insert(8);
    originalValues.insert(35);

    for (unsigned long value : originalValues) {
        mStateBits.set(value);
    }

    std::set<unsigned long> readValues;
    for (unsigned long bit : IterateBitSet(mStateBits)) {
        EXPECT_EQ(1u, originalValues.count(bit));
        EXPECT_EQ(0u, readValues.count(bit));
        readValues.insert(bit);
    }

    EXPECT_EQ(originalValues.size(), readValues.size());
}

// Test an empty iterator.
TEST_F(BitSetIteratorTest, EmptySet) {
    // We don't use the FAIL gtest macro here since it returns immediately,
    // causing an unreachable code warning in MSVC
    bool sawBit = false;
    for (unsigned long bit : IterateBitSet(mStateBits)) {
        DAWN_UNUSED(bit);
        sawBit = true;
    }
    EXPECT_FALSE(sawBit);
}

// Test iterating a result of combining two bitsets.
TEST_F(BitSetIteratorTest, NonLValueBitset) {
    std::bitset<40> otherBits;

    mStateBits.set(1);
    mStateBits.set(2);
    mStateBits.set(3);
    mStateBits.set(4);

    otherBits.set(0);
    otherBits.set(1);
    otherBits.set(3);
    otherBits.set(5);

    std::set<unsigned long> seenBits;

    for (unsigned long bit : IterateBitSet(mStateBits & otherBits)) {
        EXPECT_EQ(0u, seenBits.count(bit));
        seenBits.insert(bit);
        EXPECT_TRUE(mStateBits[bit]);
        EXPECT_TRUE(otherBits[bit]);
    }

    EXPECT_EQ((mStateBits & otherBits).count(), seenBits.size());
}

class EnumBitSetIteratorTest : public testing::Test {
  protected:
    enum class TestEnum { A, B, C, D, E, F, G, H, I, J, EnumCount };

    static constexpr size_t kEnumCount = static_cast<size_t>(TestEnum::EnumCount);
    ityp::bitset<TestEnum, kEnumCount> mStateBits;
};

// Simple iterator test.
TEST_F(EnumBitSetIteratorTest, Iterator) {
    std::set<TestEnum> originalValues;
    originalValues.insert(TestEnum::B);
    originalValues.insert(TestEnum::F);
    originalValues.insert(TestEnum::C);
    originalValues.insert(TestEnum::I);

    for (TestEnum value : originalValues) {
        mStateBits.set(value);
    }

    std::set<TestEnum> readValues;
    for (TestEnum bit : IterateBitSet(mStateBits)) {
        EXPECT_EQ(1u, originalValues.count(bit));
        EXPECT_EQ(0u, readValues.count(bit));
        readValues.insert(bit);
    }

    EXPECT_EQ(originalValues.size(), readValues.size());
}

// Test an empty iterator.
TEST_F(EnumBitSetIteratorTest, EmptySet) {
    // We don't use the FAIL gtest macro here since it returns immediately,
    // causing an unreachable code warning in MSVC
    bool sawBit = false;
    for (TestEnum bit : IterateBitSet(mStateBits)) {
        DAWN_UNUSED(bit);
        sawBit = true;
    }
    EXPECT_FALSE(sawBit);
}

// Test iterating a result of combining two bitsets.
TEST_F(EnumBitSetIteratorTest, NonLValueBitset) {
    ityp::bitset<TestEnum, kEnumCount> otherBits;

    mStateBits.set(TestEnum::B);
    mStateBits.set(TestEnum::C);
    mStateBits.set(TestEnum::D);
    mStateBits.set(TestEnum::E);

    otherBits.set(TestEnum::A);
    otherBits.set(TestEnum::B);
    otherBits.set(TestEnum::D);
    otherBits.set(TestEnum::F);

    std::set<TestEnum> seenBits;

    for (TestEnum bit : IterateBitSet(mStateBits & otherBits)) {
        EXPECT_EQ(0u, seenBits.count(bit));
        seenBits.insert(bit);
        EXPECT_TRUE(mStateBits[bit]);
        EXPECT_TRUE(otherBits[bit]);
    }

    EXPECT_EQ((mStateBits & otherBits).count(), seenBits.size());
}

class ITypBitsetIteratorTest : public testing::Test {
  protected:
    using IntegerT = TypedInteger<struct Foo, uint32_t>;
    ityp::bitset<IntegerT, 40> mStateBits;
};

// Simple iterator test.
TEST_F(ITypBitsetIteratorTest, Iterator) {
    std::set<IntegerT> originalValues;
    originalValues.insert(IntegerT(2));
    originalValues.insert(IntegerT(6));
    originalValues.insert(IntegerT(8));
    originalValues.insert(IntegerT(35));

    for (IntegerT value : originalValues) {
        mStateBits.set(value);
    }

    std::set<IntegerT> readValues;
    for (IntegerT bit : IterateBitSet(mStateBits)) {
        EXPECT_EQ(1u, originalValues.count(bit));
        EXPECT_EQ(0u, readValues.count(bit));
        readValues.insert(bit);
    }

    EXPECT_EQ(originalValues.size(), readValues.size());
}

// Test an empty iterator.
TEST_F(ITypBitsetIteratorTest, EmptySet) {
    // We don't use the FAIL gtest macro here since it returns immediately,
    // causing an unreachable code warning in MSVC
    bool sawBit = false;
    for (IntegerT bit : IterateBitSet(mStateBits)) {
        DAWN_UNUSED(bit);
        sawBit = true;
    }
    EXPECT_FALSE(sawBit);
}

// Test iterating a result of combining two bitsets.
TEST_F(ITypBitsetIteratorTest, NonLValueBitset) {
    ityp::bitset<IntegerT, 40> otherBits;

    mStateBits.set(IntegerT(1));
    mStateBits.set(IntegerT(2));
    mStateBits.set(IntegerT(3));
    mStateBits.set(IntegerT(4));

    otherBits.set(IntegerT(0));
    otherBits.set(IntegerT(1));
    otherBits.set(IntegerT(3));
    otherBits.set(IntegerT(5));

    std::set<IntegerT> seenBits;

    for (IntegerT bit : IterateBitSet(mStateBits & otherBits)) {
        EXPECT_EQ(0u, seenBits.count(bit));
        seenBits.insert(bit);
        EXPECT_TRUE(mStateBits[bit]);
        EXPECT_TRUE(otherBits[bit]);
    }

    EXPECT_EQ((mStateBits & otherBits).count(), seenBits.size());
}
