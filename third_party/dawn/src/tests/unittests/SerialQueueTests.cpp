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

#include "common/SerialQueue.h"

using TestSerialQueue = SerialQueue<int>;

// A number of basic tests for SerialQueue that are difficult to split from one another
TEST(SerialQueue, BasicTest) {
    TestSerialQueue queue;

    // Queue starts empty
    ASSERT_TRUE(queue.Empty());

    // Iterating on empty queue 1) works 2) doesn't produce any values
    for (int value : queue.IterateAll()) {
        DAWN_UNUSED(value);
        ASSERT_TRUE(false);
    }

    // Enqueuing values as const ref or rvalue ref
    queue.Enqueue(1, 0);
    queue.Enqueue(2, 0);
    queue.Enqueue(std::move(3), 1);

    // Iterating over a non-empty queue produces the expected result
    std::vector<int> expectedValues = {1, 2, 3};
    for (int value : queue.IterateAll()) {
        EXPECT_EQ(expectedValues.front(), value);
        ASSERT_FALSE(expectedValues.empty());
        expectedValues.erase(expectedValues.begin());
    }
    ASSERT_TRUE(expectedValues.empty());

    // Clear works and makes the queue empty and iteration does nothing.
    queue.Clear();
    ASSERT_TRUE(queue.Empty());

    for (int value : queue.IterateAll()) {
        DAWN_UNUSED(value);
        ASSERT_TRUE(false);
    }
}

// Test enqueuing vectors works
TEST(SerialQueue, EnqueueVectors) {
    TestSerialQueue queue;

    std::vector<int> vector1 = {1, 2, 3, 4};
    std::vector<int> vector2 = {5, 6, 7, 8};
    std::vector<int> vector3 = {9, 0};

    queue.Enqueue(vector1, 0);
    queue.Enqueue(std::move(vector2), 0);
    queue.Enqueue(vector3, 1);

    std::vector<int> expectedValues = {1, 2, 3, 4, 5, 6, 7, 8, 9, 0};
    for (int value : queue.IterateAll()) {
        EXPECT_EQ(expectedValues.front(), value);
        ASSERT_FALSE(expectedValues.empty());
        expectedValues.erase(expectedValues.begin());
    }
    ASSERT_TRUE(expectedValues.empty());
}

// Test IterateUpTo
TEST(SerialQueue, IterateUpTo) {
    TestSerialQueue queue;

    std::vector<int> vector1 = {1, 2, 3, 4};
    std::vector<int> vector2 = {5, 6, 7, 8};
    std::vector<int> vector3 = {9, 0};

    queue.Enqueue(vector1, 0);
    queue.Enqueue(std::move(vector2), 1);
    queue.Enqueue(vector3, 2);

    std::vector<int> expectedValues = {1, 2, 3, 4, 5, 6, 7, 8};
    for (int value : queue.IterateUpTo(1)) {
        EXPECT_EQ(expectedValues.front(), value);
        ASSERT_FALSE(expectedValues.empty());
        expectedValues.erase(expectedValues.begin());
    }
    ASSERT_TRUE(expectedValues.empty());
    EXPECT_EQ(queue.LastSerial(), 2u);
}

// Test ClearUpTo
TEST(SerialQueue, ClearUpTo) {
    TestSerialQueue queue;

    std::vector<int> vector1 = {1, 2, 3, 4};
    std::vector<int> vector2 = {5, 6, 7, 8};
    std::vector<int> vector3 = {9, 0};

    queue.Enqueue(vector1, 0);
    queue.Enqueue(std::move(vector2), 0);
    queue.Enqueue(vector3, 1);

    queue.ClearUpTo(0);
    EXPECT_EQ(queue.LastSerial(), 1u);

    std::vector<int> expectedValues = {9, 0};
    for (int value : queue.IterateAll()) {
        EXPECT_EQ(expectedValues.front(), value);
        ASSERT_FALSE(expectedValues.empty());
        expectedValues.erase(expectedValues.begin());
    }
    ASSERT_TRUE(expectedValues.empty());
}

// Test FirstSerial
TEST(SerialQueue, FirstSerial) {
    TestSerialQueue queue;

    std::vector<int> vector1 = {1, 2, 3, 4};
    std::vector<int> vector2 = {5, 6, 7, 8};
    std::vector<int> vector3 = {9, 0};

    queue.Enqueue(vector1, 0);
    queue.Enqueue(std::move(vector2), 1);
    queue.Enqueue(vector3, 2);

    EXPECT_EQ(queue.FirstSerial(), 0u);

    queue.ClearUpTo(1);
    EXPECT_EQ(queue.FirstSerial(), 2u);

    queue.Clear();
    queue.Enqueue(vector1, 6);
    EXPECT_EQ(queue.FirstSerial(), 6u);
}

// Test LastSerial
TEST(SerialQueue, LastSerial) {
    TestSerialQueue queue;

    queue.Enqueue({1}, 0);
    EXPECT_EQ(queue.LastSerial(), 0u);

    queue.Enqueue({2}, 1);
    EXPECT_EQ(queue.LastSerial(), 1u);
}