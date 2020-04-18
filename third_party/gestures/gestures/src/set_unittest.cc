// Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <gtest/gtest.h>

#include "gestures/include/set.h"

namespace gestures {

class SetTest : public ::testing::Test {};

TEST(SetTest, SimpleTest) {
  const int kMax = 5;
  set<short, kMax> set_a;
  set<short, kMax> set_b;

  EXPECT_EQ(0, set_a.size());
  EXPECT_EQ(0, set_b.size());
  EXPECT_TRUE(set_a.empty());
  set<short, kMax>::iterator a_end = set_a.end();
  set<short, kMax>::iterator a_f = set_a.find(1);
  EXPECT_EQ(a_end, a_f);
  EXPECT_EQ(set_a.end(), set_a.find(1));
  set_a.insert(1);
  EXPECT_EQ(1, set_a.size());
  EXPECT_FALSE(set_a.empty());
  EXPECT_NE(set_a.end(), set_a.find(1));
  set_b.insert(3);
  EXPECT_EQ(1, set_b.size());
  set_b.insert(3);
  EXPECT_EQ(1, set_b.size());
  EXPECT_TRUE(set_a != set_b);
  set_b.erase(3);
  set_b.insert(2);
  set_b.insert(1);
  EXPECT_EQ(2, set_b.size());
  EXPECT_NE(set_b.end(), set_b.find(1));
  EXPECT_NE(set_b.end(), set_b.find(2));
  EXPECT_TRUE(set_b != set_a);
  set_a.insert(2);
  EXPECT_EQ(2, set_a.size());
  EXPECT_TRUE(set_b == set_a);
  set_a.insert(3);
  EXPECT_EQ(3, set_a.size());
  set_b.insert(3);
  EXPECT_EQ(3, set_b.size());
  EXPECT_TRUE(set_b == set_a);
  EXPECT_EQ(0, set_a.erase(4));
  EXPECT_EQ(3, set_a.size());
  EXPECT_TRUE(set_b == set_a);
  EXPECT_EQ(1, set_a.erase(1));
  EXPECT_EQ(2, set_a.size());
  EXPECT_EQ(1, set_b.erase(1));
  EXPECT_EQ(2, set_b.size());
  EXPECT_TRUE(set_b == set_a);
  set_a.clear();
  EXPECT_EQ(0, set_a.size());
  EXPECT_TRUE(set_b != set_a);
  set_b.clear();
  EXPECT_EQ(0, set_b.size());
  EXPECT_TRUE(set_b == set_a);
}

TEST(SetTest, OverflowTest) {
  const int kMax = 3;
  set<short, kMax> the_set;  // holds 3 elts
  the_set.insert(4);
  the_set.insert(5);
  the_set.insert(6);
  the_set.insert(7);
  EXPECT_EQ(kMax, the_set.size());
  EXPECT_NE(the_set.end(), the_set.find(4));
  EXPECT_NE(the_set.end(), the_set.find(5));
  EXPECT_NE(the_set.end(), the_set.find(6));
  EXPECT_EQ(the_set.end(), the_set.find(7));
}

TEST(SetTest, SizeTest) {
  set<short, 2> small;
  set<short, 3> big;
  EXPECT_TRUE(small == big);
  EXPECT_FALSE(small != big);

  small.insert(3);
  big = small;
  EXPECT_TRUE(small == big);

  big.insert(2);
  big.insert(1);
  small = big;
}

template<typename ReducedSet, typename RequiredSet>
void DoSetRemoveMissingTest(ReducedSet* reduced,
                            RequiredSet* required,
                            bool revserse_insert_order) {
  if (!revserse_insert_order) {
    reduced->insert(10);
    reduced->insert(11);
    required->insert(11);
    required->insert(12);
  } else {
    required->insert(12);
    required->insert(11);
    reduced->insert(11);
    reduced->insert(10);
  }
  SetRemoveMissing(reduced, *required);
  EXPECT_EQ(1, reduced->size());
  EXPECT_EQ(2, required->size());
  EXPECT_TRUE(SetContainsValue(*reduced, 11));
  EXPECT_TRUE(SetContainsValue(*required, 11));
  EXPECT_TRUE(SetContainsValue(*required, 12));
}

TEST(SetTest, SetRemoveMissingTest) {
  for (size_t i = 0; i < 2; i++) {
    set<short, 3> small;
    set<short, 4> big;
    DoSetRemoveMissingTest(&small, &big, i == 0);
  }

  for (size_t i = 0; i < 2; i++) {
    set<short, 3> small;
    set<short, 4> big;
    DoSetRemoveMissingTest(&big, &small, i == 0);
  }

  for (size_t i = 0; i < 2; i++) {
    set<short, 2> small;
    set<short, 3> big;
    DoSetRemoveMissingTest(&small, &big, i == 0);
  }

  for (size_t i = 0; i < 2; i++) {
    set<short, 2> small;
    set<short, 3> big;
    DoSetRemoveMissingTest(&big, &small, i == 0);
  }

  for (size_t i = 0; i < 2; i++) {
    std::set<short> small;
    std::set<short> big;
    DoSetRemoveMissingTest(&small, &big, i == 0);
  }
}

template<typename LeftSet, typename RightSet>
void DoSetSubtractTest() {
  LeftSet left;
  RightSet right;
  left.insert(4);
  left.insert(2);
  right.insert(1);
  right.insert(2);
  LeftSet out = SetSubtract(left, right);
  EXPECT_EQ(1, out.size());
  EXPECT_EQ(4, *out.begin());
  EXPECT_EQ(2, left.size());

  left.clear();
  EXPECT_EQ(0, SetSubtract(left, right).size());
  left.insert(5);
  EXPECT_EQ(1, SetSubtract(left, right).size());
  right.clear();
  EXPECT_EQ(1, SetSubtract(left, right).size());
}

TEST(SetTest, SetSubtractTest) {
  DoSetSubtractTest<std::set<short>, std::set<short>>();
  DoSetSubtractTest<set<short, 3>, set<short, 4>>();
  DoSetSubtractTest<set<short, 2>, set<short, 2>>();
  DoSetSubtractTest<set<short, 4>, set<short, 2>>();
}

}  // namespace gestures
