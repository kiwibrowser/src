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
#include "common/ityp_array.h"

class ITypArrayTest : public testing::Test {
  protected:
    using Key = TypedInteger<struct KeyT, uint32_t>;
    using Val = TypedInteger<struct ValT, uint32_t>;
    using Array = ityp::array<Key, Val, 10>;

    // Test that the expected array methods can be constexpr
    struct ConstexprTest {
        static constexpr Array kArr = {Val(0), Val(1), Val(2), Val(3), Val(4),
                                       Val(5), Val(6), Val(7), Val(8), Val(9)};

        static_assert(kArr[Key(3)] == Val(3), "");
        static_assert(kArr.at(Key(7)) == Val(7), "");
        static_assert(kArr.size() == Key(10), "");
    };
};

// Test that values can be set at an index and retrieved from the same index.
TEST_F(ITypArrayTest, Indexing) {
    Array arr;
    {
        arr[Key(2)] = Val(5);
        arr[Key(1)] = Val(9);
        arr[Key(9)] = Val(2);

        ASSERT_EQ(arr[Key(2)], Val(5));
        ASSERT_EQ(arr[Key(1)], Val(9));
        ASSERT_EQ(arr[Key(9)], Val(2));
    }
    {
        arr.at(Key(4)) = Val(5);
        arr.at(Key(3)) = Val(8);
        arr.at(Key(1)) = Val(7);

        ASSERT_EQ(arr.at(Key(4)), Val(5));
        ASSERT_EQ(arr.at(Key(3)), Val(8));
        ASSERT_EQ(arr.at(Key(1)), Val(7));
    }
}

// Test that the array can be iterated in order with a range-based for loop
TEST_F(ITypArrayTest, RangeBasedIteration) {
    Array arr;

    // Assign in a non-const range-based for loop
    uint32_t i = 0;
    for (Val& val : arr) {
        val = Val(i);
    }

    // Check values in a const range-based for loop
    i = 0;
    for (Val val : static_cast<const Array&>(arr)) {
        ASSERT_EQ(val, arr[Key(i++)]);
    }
}

// Test that begin/end/front/back/data return pointers/references to the correct elements.
TEST_F(ITypArrayTest, BeginEndFrontBackData) {
    Array arr;

    // non-const versions
    ASSERT_EQ(&arr.front(), &arr[Key(0)]);
    ASSERT_EQ(&arr.back(), &arr[Key(9)]);
    ASSERT_EQ(arr.data(), &arr[Key(0)]);

    // const versions
    const Array& constArr = arr;
    ASSERT_EQ(&constArr.front(), &constArr[Key(0)]);
    ASSERT_EQ(&constArr.back(), &constArr[Key(9)]);
    ASSERT_EQ(constArr.data(), &constArr[Key(0)]);
}
