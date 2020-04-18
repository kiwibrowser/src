// Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <gtest/gtest.h>

#include "gestures/include/map.h"

namespace gestures {

typedef vector<int, 10> test_vector;

class VectorTest : public ::testing::Test {};

void ExpectGrowingVectorOfSize(const test_vector& vector, int expected_size) {
  int count = 0;
  for (const int& v : vector) {
    ++count;
    EXPECT_EQ(count, v);
  }
  EXPECT_EQ(expected_size, count);
}

TEST(VectorTest, PushFrontBackTest) {
  test_vector vector;
  vector.push_back(3);
  vector.insert(vector.begin(), 2);
  vector.push_back(4);
  vector.insert(vector.begin(), 1);
  vector.push_back(5);
  ExpectGrowingVectorOfSize(vector, 5);
}

TEST(VectorTest, InsertTest) {
  test_vector vector;
  vector.push_back(1);
  vector.push_back(2);
  vector.push_back(4);
  vector.push_back(5);
  vector.insert(vector.find(4), 3);
  ExpectGrowingVectorOfSize(vector, 5);
}

bool EvenFilter(const int& v) { return (v % 2) == 0; }

TEST(VectorTest, FilterTest) {
  test_vector vector;
  vector.push_back(1);
  vector.push_back(2);
  vector.push_back(3);
  vector.push_back(4);
  vector.push_back(5);
  vector.push_back(6);

  int count = 0;
  FilteredRange<int> range(vector.begin(), vector.end(), EvenFilter);
  for (const int& v : range) {
    EXPECT_TRUE((v % 2) == 0);
    ++count;
  }
  EXPECT_EQ(count, 3);
}

TEST(VectorTest, FilterEditTest) {
  test_vector vector;
  vector.push_back(1);
  vector.push_back(2);
  vector.push_back(4);
  vector.push_back(7);
  FilteredRange<int> range(vector.begin(), vector.end(), EvenFilter);
  for (int& v : range)
    v = v + 1;
  // vector = [1, 3, 5, 7]
  vector.insert(vector.find(3), 2);
  vector.insert(vector.find(5), 4);
  vector.insert(vector.find(7), 6);
  ExpectGrowingVectorOfSize(vector, 7);
}

}  // namespace gestures
