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
#include "common/ityp_span.h"

#include <array>

class ITypSpanTest : public testing::Test {
  protected:
    using Key = TypedInteger<struct KeyT, size_t>;
    using Val = TypedInteger<struct ValT, uint32_t>;
    using Span = ityp::span<Key, Val>;
};

// Test that values can be set at an index and retrieved from the same index.
TEST_F(ITypSpanTest, Indexing) {
    std::array<Val, 10> arr;
    Span span(arr.data(), Key(arr.size()));
    {
        span[Key(2)] = Val(5);
        span[Key(1)] = Val(9);
        span[Key(9)] = Val(2);

        ASSERT_EQ(span[Key(2)], Val(5));
        ASSERT_EQ(span[Key(1)], Val(9));
        ASSERT_EQ(span[Key(9)], Val(2));
    }
}

// Test that the span can be is iterated in order with a range-based for loop
TEST_F(ITypSpanTest, RangeBasedIteration) {
    std::array<Val, 10> arr;
    Span span(arr.data(), Key(arr.size()));

    // Assign in a non-const range-based for loop
    uint32_t i = 0;
    for (Val& val : span) {
        val = Val(i);
    }

    // Check values in a const range-based for loop
    i = 0;
    for (Val val : static_cast<const Span&>(span)) {
        ASSERT_EQ(val, span[Key(i++)]);
    }
}

// Test that begin/end/front/back/data return pointers/references to the correct elements.
TEST_F(ITypSpanTest, BeginEndFrontBackData) {
    std::array<Val, 10> arr;
    Span span(arr.data(), Key(arr.size()));

    // non-const versions
    ASSERT_EQ(span.begin(), &span[Key(0)]);
    ASSERT_EQ(span.end(), &span[Key(0)] + static_cast<size_t>(span.size()));
    ASSERT_EQ(&span.front(), &span[Key(0)]);
    ASSERT_EQ(&span.back(), &span[Key(9)]);
    ASSERT_EQ(span.data(), &span[Key(0)]);

    // const versions
    const Span& constSpan = span;
    ASSERT_EQ(constSpan.begin(), &constSpan[Key(0)]);
    ASSERT_EQ(constSpan.end(), &constSpan[Key(0)] + static_cast<size_t>(constSpan.size()));
    ASSERT_EQ(&constSpan.front(), &constSpan[Key(0)]);
    ASSERT_EQ(&constSpan.back(), &constSpan[Key(9)]);
    ASSERT_EQ(constSpan.data(), &constSpan[Key(0)]);
}
