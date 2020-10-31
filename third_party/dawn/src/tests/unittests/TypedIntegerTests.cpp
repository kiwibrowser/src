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
#include "common/UnderlyingType.h"

class TypedIntegerTest : public testing::Test {
  protected:
    using Unsigned = TypedInteger<struct UnsignedT, uint32_t>;
    using Signed = TypedInteger<struct SignedT, int32_t>;
};

// Test that typed integers can be created and cast and the internal values are identical
TEST_F(TypedIntegerTest, ConstructionAndCast) {
    Signed svalue(2);
    EXPECT_EQ(static_cast<int32_t>(svalue), 2);

    Unsigned uvalue(7);
    EXPECT_EQ(static_cast<uint32_t>(uvalue), 7u);

    static_assert(static_cast<int32_t>(Signed(3)) == 3, "");
    static_assert(static_cast<uint32_t>(Unsigned(28)) == 28, "");
}

// Test typed integer comparison operators
TEST_F(TypedIntegerTest, Comparison) {
    Unsigned value(8);

    // Truthy usages of comparison operators
    EXPECT_TRUE(value < Unsigned(9));
    EXPECT_TRUE(value <= Unsigned(9));
    EXPECT_TRUE(value <= Unsigned(8));
    EXPECT_TRUE(value == Unsigned(8));
    EXPECT_TRUE(value >= Unsigned(8));
    EXPECT_TRUE(value >= Unsigned(7));
    EXPECT_TRUE(value > Unsigned(7));
    EXPECT_TRUE(value != Unsigned(7));

    // Falsy usages of comparison operators
    EXPECT_FALSE(value >= Unsigned(9));
    EXPECT_FALSE(value > Unsigned(9));
    EXPECT_FALSE(value > Unsigned(8));
    EXPECT_FALSE(value != Unsigned(8));
    EXPECT_FALSE(value < Unsigned(8));
    EXPECT_FALSE(value < Unsigned(7));
    EXPECT_FALSE(value <= Unsigned(7));
    EXPECT_FALSE(value == Unsigned(7));
}

TEST_F(TypedIntegerTest, Arithmetic) {
    // Postfix Increment
    {
        Signed value(0);
        EXPECT_EQ(value++, Signed(0));
        EXPECT_EQ(value, Signed(1));
    }

    // Prefix Increment
    {
        Signed value(0);
        EXPECT_EQ(++value, Signed(1));
        EXPECT_EQ(value, Signed(1));
    }

    // Postfix Decrement
    {
        Signed value(0);
        EXPECT_EQ(value--, Signed(0));
        EXPECT_EQ(value, Signed(-1));
    }

    // Prefix Decrement
    {
        Signed value(0);
        EXPECT_EQ(--value, Signed(-1));
        EXPECT_EQ(value, Signed(-1));
    }

    // Signed addition
    {
        Signed a(3);
        Signed b(-4);
        Signed c = a + b;
        EXPECT_EQ(a, Signed(3));
        EXPECT_EQ(b, Signed(-4));
        EXPECT_EQ(c, Signed(-1));
    }

    // Signed subtraction
    {
        Signed a(3);
        Signed b(-4);
        Signed c = a - b;
        EXPECT_EQ(a, Signed(3));
        EXPECT_EQ(b, Signed(-4));
        EXPECT_EQ(c, Signed(7));
    }

    // Unsigned addition
    {
        Unsigned a(9);
        Unsigned b(3);
        Unsigned c = a + b;
        EXPECT_EQ(a, Unsigned(9));
        EXPECT_EQ(b, Unsigned(3));
        EXPECT_EQ(c, Unsigned(12));
    }

    // Unsigned subtraction
    {
        Unsigned a(9);
        Unsigned b(2);
        Unsigned c = a - b;
        EXPECT_EQ(a, Unsigned(9));
        EXPECT_EQ(b, Unsigned(2));
        EXPECT_EQ(c, Unsigned(7));
    }

    // Negation
    {
        Signed a(5);
        Signed b = -a;
        EXPECT_EQ(a, Signed(5));
        EXPECT_EQ(b, Signed(-5));
    }
}

TEST_F(TypedIntegerTest, NumericLimits) {
    EXPECT_EQ(std::numeric_limits<Unsigned>::max(), Unsigned(std::numeric_limits<uint32_t>::max()));
    EXPECT_EQ(std::numeric_limits<Unsigned>::min(), Unsigned(std::numeric_limits<uint32_t>::min()));
    EXPECT_EQ(std::numeric_limits<Signed>::max(), Signed(std::numeric_limits<int32_t>::max()));
    EXPECT_EQ(std::numeric_limits<Signed>::min(), Signed(std::numeric_limits<int32_t>::min()));
}

TEST_F(TypedIntegerTest, UnderlyingType) {
    static_assert(std::is_same<UnderlyingType<Unsigned>, uint32_t>::value, "");
    static_assert(std::is_same<UnderlyingType<Signed>, int32_t>::value, "");
}

// Tests for bounds assertions on arithmetic overflow and underflow.
#if defined(DAWN_ENABLE_ASSERTS)

TEST_F(TypedIntegerTest, IncrementUnsignedOverflow) {
    Unsigned value(std::numeric_limits<uint32_t>::max() - 1);

    value++;                    // Doesn't overflow.
    EXPECT_DEATH(value++, "");  // Overflows.
}

TEST_F(TypedIntegerTest, IncrementSignedOverflow) {
    Signed value(std::numeric_limits<int32_t>::max() - 1);

    value++;                    // Doesn't overflow.
    EXPECT_DEATH(value++, "");  // Overflows.
}

TEST_F(TypedIntegerTest, DecrementUnsignedUnderflow) {
    Unsigned value(std::numeric_limits<uint32_t>::min() + 1);

    value--;                    // Doesn't underflow.
    EXPECT_DEATH(value--, "");  // Underflows.
}

TEST_F(TypedIntegerTest, DecrementSignedUnderflow) {
    Signed value(std::numeric_limits<int32_t>::min() + 1);

    value--;                    // Doesn't underflow.
    EXPECT_DEATH(value--, "");  // Underflows.
}

TEST_F(TypedIntegerTest, UnsignedAdditionOverflow) {
    Unsigned value(std::numeric_limits<uint32_t>::max() - 1);

    value + Unsigned(1);                    // Doesn't overflow.
    EXPECT_DEATH(value + Unsigned(2), "");  // Overflows.
}

TEST_F(TypedIntegerTest, UnsignedSubtractionUnderflow) {
    Unsigned value(1);

    value - Unsigned(1);                    // Doesn't underflow.
    EXPECT_DEATH(value - Unsigned(2), "");  // Underflows.
}

TEST_F(TypedIntegerTest, SignedAdditionOverflow) {
    Signed value(std::numeric_limits<int32_t>::max() - 1);

    value + Signed(1);                    // Doesn't overflow.
    EXPECT_DEATH(value + Signed(2), "");  // Overflows.
}

TEST_F(TypedIntegerTest, SignedAdditionUnderflow) {
    Signed value(std::numeric_limits<int32_t>::min() + 1);

    value + Signed(-1);                    // Doesn't underflow.
    EXPECT_DEATH(value + Signed(-2), "");  // Underflows.
}

TEST_F(TypedIntegerTest, SignedSubtractionOverflow) {
    Signed value(std::numeric_limits<int32_t>::max() - 1);

    value - Signed(-1);                    // Doesn't overflow.
    EXPECT_DEATH(value - Signed(-2), "");  // Overflows.
}

TEST_F(TypedIntegerTest, SignedSubtractionUnderflow) {
    Signed value(std::numeric_limits<int32_t>::min() + 1);

    value - Signed(1);                    // Doesn't underflow.
    EXPECT_DEATH(value - Signed(2), "");  // Underflows.
}

TEST_F(TypedIntegerTest, NegationOverflow) {
    Signed maxValue(std::numeric_limits<int32_t>::max());
    -maxValue;  // Doesn't underflow.

    Signed minValue(std::numeric_limits<int32_t>::min());
    EXPECT_DEATH(-minValue, "");  // Overflows.
}

#endif  // defined(DAWN_ENABLE_ASSERTS)
