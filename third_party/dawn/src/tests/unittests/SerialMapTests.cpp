// Copyright 2018 The Dawn Authors
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

#include "common/SerialMap.h"

using TestSerialMap = SerialMap<int>;

// A number of basic tests for SerialMap that are difficult to split from one another
TEST(SerialMap, BasicTest) {
    TestSerialMap map;

    // Map starts empty
    ASSERT_TRUE(map.Empty());

    // Iterating on empty map 1) works 2) doesn't produce any values
    for (int value : map.IterateAll()) {
        DAWN_UNUSED(value);
        ASSERT_TRUE(false);
    }

    // Enqueuing values as const ref or rvalue ref
    map.Enqueue(1, 0);
    map.Enqueue(2, 0);
    map.Enqueue(std::move(3), 1);

    // Iterating over a non-empty map produces the expected result
    std::vector<int> expectedValues = {1, 2, 3};
    for (int value : map.IterateAll()) {
        EXPECT_EQ(expectedValues.front(), value);
        ASSERT_FALSE(expectedValues.empty());
        expectedValues.erase(expectedValues.begin());
    }
    ASSERT_TRUE(expectedValues.empty());

    // Clear works and makes the map empty and iteration does nothing.
    map.Clear();
    ASSERT_TRUE(map.Empty());

    for (int value : map.IterateAll()) {
        DAWN_UNUSED(value);
        ASSERT_TRUE(false);
    }
}

// Test that items can be enqueued in an arbitrary order
TEST(SerialMap, EnqueueOrder) {
    TestSerialMap map;

    // Enqueue values in an arbitrary order
    map.Enqueue(3, 1);
    map.Enqueue(1, 0);
    map.Enqueue(4, 2);
    map.Enqueue(5, 2);
    map.Enqueue(2, 0);

    // Iterating over a non-empty map produces the expected result
    std::vector<int> expectedValues = {1, 2, 3, 4, 5};
    for (int value : map.IterateAll()) {
        EXPECT_EQ(expectedValues.front(), value);
        ASSERT_FALSE(expectedValues.empty());
        expectedValues.erase(expectedValues.begin());
    }
    ASSERT_TRUE(expectedValues.empty());
}

// Test enqueuing vectors works
TEST(SerialMap, EnqueueVectors) {
    TestSerialMap map;

    std::vector<int> vector1 = {1, 2, 3, 4};
    std::vector<int> vector2 = {5, 6, 7, 8};
    std::vector<int> vector3 = {9, 0};

    map.Enqueue(vector1, 0);
    map.Enqueue(std::move(vector2), 0);
    map.Enqueue(vector3, 1);

    std::vector<int> expectedValues = {1, 2, 3, 4, 5, 6, 7, 8, 9, 0};
    for (int value : map.IterateAll()) {
        EXPECT_EQ(expectedValues.front(), value);
        ASSERT_FALSE(expectedValues.empty());
        expectedValues.erase(expectedValues.begin());
    }
    ASSERT_TRUE(expectedValues.empty());
}

// Test IterateUpTo
TEST(SerialMap, IterateUpTo) {
    TestSerialMap map;

    std::vector<int> vector1 = {1, 2, 3, 4};
    std::vector<int> vector2 = {5, 6, 7, 8};
    std::vector<int> vector3 = {9, 0};

    map.Enqueue(vector1, 0);
    map.Enqueue(std::move(vector2), 1);
    map.Enqueue(vector3, 2);

    std::vector<int> expectedValues = {1, 2, 3, 4, 5, 6, 7, 8};
    for (int value : map.IterateUpTo(1)) {
        EXPECT_EQ(expectedValues.front(), value);
        ASSERT_FALSE(expectedValues.empty());
        expectedValues.erase(expectedValues.begin());
    }
    ASSERT_TRUE(expectedValues.empty());
}

// Test ClearUpTo
TEST(SerialMap, ClearUpTo) {
    TestSerialMap map;

    std::vector<int> vector1 = {1, 2, 3, 4};
    std::vector<int> vector2 = {5, 6, 7, 8};
    std::vector<int> vector3 = {9, 0};

    map.Enqueue(vector1, 0);
    map.Enqueue(std::move(vector2), 0);
    map.Enqueue(vector3, 1);

    map.ClearUpTo(0);

    std::vector<int> expectedValues = {9, 0};
    for (int value : map.IterateAll()) {
        EXPECT_EQ(expectedValues.front(), value);
        ASSERT_FALSE(expectedValues.empty());
        expectedValues.erase(expectedValues.begin());
    }
    ASSERT_TRUE(expectedValues.empty());
}

// Test FirstSerial
TEST(SerialMap, FirstSerial) {
    TestSerialMap map;

    std::vector<int> vector1 = {1, 2, 3, 4};
    std::vector<int> vector2 = {5, 6, 7, 8};
    std::vector<int> vector3 = {9, 0};

    map.Enqueue(vector1, 0);
    map.Enqueue(std::move(vector2), 1);
    map.Enqueue(vector3, 2);

    EXPECT_EQ(map.FirstSerial(), 0u);

    map.ClearUpTo(1);
    EXPECT_EQ(map.FirstSerial(), 2u);

    map.Clear();
    map.Enqueue(vector1, 6);
    EXPECT_EQ(map.FirstSerial(), 6u);
}
