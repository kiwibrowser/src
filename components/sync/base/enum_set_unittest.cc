// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/sync/base/enum_set.h"

#include <stddef.h>

#include "testing/gtest/include/gtest/gtest.h"

namespace syncer {
namespace {

enum TestEnum {
  TEST_0,
  TEST_MIN = TEST_0,
  TEST_1,
  TEST_2,
  TEST_3,
  TEST_4,
  TEST_MAX = TEST_4,
  TEST_5
};

using TestEnumSet = EnumSet<TestEnum, TEST_MIN, TEST_MAX>;

class EnumSetTest : public ::testing::Test {};

TEST_F(EnumSetTest, ClassConstants) {
  TestEnumSet enums;
  EXPECT_EQ(TEST_MIN, TestEnumSet::kMinValue);
  EXPECT_EQ(TEST_MAX, TestEnumSet::kMaxValue);
  EXPECT_EQ(static_cast<size_t>(5), TestEnumSet::kValueCount);
}

// Use static_assert to check that functions we expect to be compile time
// evaluatable are really that way.
TEST_F(EnumSetTest, ConstexprsAreValid) {
  static_assert(TestEnumSet::All().Has(TEST_1),
                "expected All() to be integral constant expression");
  static_assert(TestEnumSet::FromRange(TEST_1, TEST_3).Has(TEST_1),
                "expected FromRange() to be integral constant expression");
  static_assert(TestEnumSet(TEST_1).Has(TEST_1),
                "expected TestEnumSet() to be integral constant expression");
}

TEST_F(EnumSetTest, DefaultConstructor) {
  const TestEnumSet enums;
  EXPECT_TRUE(enums.Empty());
  EXPECT_EQ(static_cast<size_t>(0), enums.Size());
  EXPECT_FALSE(enums.Has(TEST_0));
  EXPECT_FALSE(enums.Has(TEST_1));
  EXPECT_FALSE(enums.Has(TEST_2));
  EXPECT_FALSE(enums.Has(TEST_3));
  EXPECT_FALSE(enums.Has(TEST_4));
}

TEST_F(EnumSetTest, OneArgConstructor) {
  const TestEnumSet enums(TEST_3);
  EXPECT_FALSE(enums.Empty());
  EXPECT_EQ(static_cast<size_t>(1), enums.Size());
  EXPECT_FALSE(enums.Has(TEST_0));
  EXPECT_FALSE(enums.Has(TEST_1));
  EXPECT_FALSE(enums.Has(TEST_2));
  EXPECT_TRUE(enums.Has(TEST_3));
  EXPECT_FALSE(enums.Has(TEST_4));
}

TEST_F(EnumSetTest, TwoArgConstructor) {
  const TestEnumSet enums(TEST_3, TEST_1);
  EXPECT_FALSE(enums.Empty());
  EXPECT_EQ(static_cast<size_t>(2), enums.Size());
  EXPECT_FALSE(enums.Has(TEST_0));
  EXPECT_TRUE(enums.Has(TEST_1));
  EXPECT_FALSE(enums.Has(TEST_2));
  EXPECT_TRUE(enums.Has(TEST_3));
  EXPECT_FALSE(enums.Has(TEST_4));
}

TEST_F(EnumSetTest, ThreeArgConstructor) {
  const TestEnumSet enums(TEST_3, TEST_1, TEST_0);
  EXPECT_FALSE(enums.Empty());
  EXPECT_EQ(static_cast<size_t>(3), enums.Size());
  EXPECT_TRUE(enums.Has(TEST_0));
  EXPECT_TRUE(enums.Has(TEST_1));
  EXPECT_FALSE(enums.Has(TEST_2));
  EXPECT_TRUE(enums.Has(TEST_3));
  EXPECT_FALSE(enums.Has(TEST_4));
}

TEST_F(EnumSetTest, DuplicatesInConstructor) {
  EXPECT_EQ(TestEnumSet(TEST_3, TEST_1, TEST_0, TEST_3, TEST_1, TEST_3),
            TestEnumSet(TEST_0, TEST_1, TEST_3));
}

TEST_F(EnumSetTest, All) {
  const TestEnumSet enums(TestEnumSet::All());
  EXPECT_FALSE(enums.Empty());
  EXPECT_EQ(static_cast<size_t>(5), enums.Size());
  EXPECT_TRUE(enums.Has(TEST_0));
  EXPECT_TRUE(enums.Has(TEST_1));
  EXPECT_TRUE(enums.Has(TEST_2));
  EXPECT_TRUE(enums.Has(TEST_3));
  EXPECT_TRUE(enums.Has(TEST_4));
}

TEST_F(EnumSetTest, FromRange) {
  EXPECT_EQ(TestEnumSet(TEST_1, TEST_2, TEST_3),
            TestEnumSet::FromRange(TEST_1, TEST_3));
  EXPECT_EQ(TestEnumSet::All(), TestEnumSet::FromRange(TEST_0, TEST_4));
  EXPECT_EQ(TestEnumSet(TEST_1), TestEnumSet::FromRange(TEST_1, TEST_1));

  using RestrictedRangeSet = EnumSet<TestEnum, TEST_1, TEST_MAX>;
  EXPECT_EQ(RestrictedRangeSet(TEST_1, TEST_2, TEST_3),
            RestrictedRangeSet::FromRange(TEST_1, TEST_3));
  EXPECT_EQ(RestrictedRangeSet::All(),
            RestrictedRangeSet::FromRange(TEST_1, TEST_4));
}

TEST_F(EnumSetTest, Put) {
  TestEnumSet enums(TEST_3);
  enums.Put(TEST_2);
  EXPECT_EQ(TestEnumSet(TEST_2, TEST_3), enums);
  enums.Put(TEST_4);
  EXPECT_EQ(TestEnumSet(TEST_2, TEST_3, TEST_4), enums);
}

TEST_F(EnumSetTest, PutAll) {
  TestEnumSet enums(TEST_3, TEST_4);
  enums.PutAll(TestEnumSet(TEST_2, TEST_3));
  EXPECT_EQ(TestEnumSet(TEST_2, TEST_3, TEST_4), enums);
}

TEST_F(EnumSetTest, PutRange) {
  TestEnumSet enums;
  enums.PutRange(TEST_1, TEST_3);
  EXPECT_EQ(TestEnumSet(TEST_1, TEST_2, TEST_3), enums);
}

TEST_F(EnumSetTest, RetainAll) {
  TestEnumSet enums(TEST_3, TEST_4);
  enums.RetainAll(TestEnumSet(TEST_2, TEST_3));
  EXPECT_EQ(TestEnumSet(TEST_3), enums);
}

TEST_F(EnumSetTest, Remove) {
  TestEnumSet enums(TEST_3, TEST_4);
  enums.Remove(TEST_0);
  enums.Remove(TEST_2);
  EXPECT_EQ(TestEnumSet(TEST_3, TEST_4), enums);
  enums.Remove(TEST_3);
  EXPECT_EQ(TestEnumSet(TEST_4), enums);
  enums.Remove(TEST_4);
  enums.Remove(TEST_5);
  EXPECT_TRUE(enums.Empty());
}

TEST_F(EnumSetTest, RemoveAll) {
  TestEnumSet enums(TEST_3, TEST_4);
  enums.RemoveAll(TestEnumSet(TEST_2, TEST_3));
  EXPECT_EQ(TestEnumSet(TEST_4), enums);
}

TEST_F(EnumSetTest, Clear) {
  TestEnumSet enums(TEST_3, TEST_4);
  enums.Clear();
  EXPECT_TRUE(enums.Empty());
}

TEST_F(EnumSetTest, Has) {
  const TestEnumSet enums(TEST_3, TEST_4);
  EXPECT_FALSE(enums.Has(TEST_0));
  EXPECT_FALSE(enums.Has(TEST_1));
  EXPECT_FALSE(enums.Has(TEST_2));
  EXPECT_TRUE(enums.Has(TEST_3));
  EXPECT_TRUE(enums.Has(TEST_4));
  EXPECT_FALSE(enums.Has(TEST_5));
}

TEST_F(EnumSetTest, HasAll) {
  const TestEnumSet enums1(TEST_3, TEST_4);
  const TestEnumSet enums2(TEST_2, TEST_3);
  const TestEnumSet enums3 = Union(enums1, enums2);
  EXPECT_TRUE(enums1.HasAll(enums1));
  EXPECT_FALSE(enums1.HasAll(enums2));
  EXPECT_FALSE(enums1.HasAll(enums3));

  EXPECT_FALSE(enums2.HasAll(enums1));
  EXPECT_TRUE(enums2.HasAll(enums2));
  EXPECT_FALSE(enums2.HasAll(enums3));

  EXPECT_TRUE(enums3.HasAll(enums1));
  EXPECT_TRUE(enums3.HasAll(enums2));
  EXPECT_TRUE(enums3.HasAll(enums3));
}

TEST_F(EnumSetTest, Iterators) {
  const TestEnumSet enums1(TEST_3, TEST_4);
  TestEnumSet enums2;
  for (TestEnumSet::Iterator it = enums1.First(); it.Good(); it.Inc()) {
    enums2.Put(it.Get());
  }
  EXPECT_EQ(enums2, enums1);
}

TEST_F(EnumSetTest, Union) {
  const TestEnumSet enums1(TEST_3, TEST_4);
  const TestEnumSet enums2(TEST_2, TEST_3);
  const TestEnumSet enums3 = Union(enums1, enums2);

  EXPECT_EQ(TestEnumSet(TEST_2, TEST_3, TEST_4), enums3);
}

TEST_F(EnumSetTest, Intersection) {
  const TestEnumSet enums1(TEST_3, TEST_4);
  const TestEnumSet enums2(TEST_2, TEST_3);
  const TestEnumSet enums3 = Intersection(enums1, enums2);

  EXPECT_EQ(TestEnumSet(TEST_3), enums3);
}

TEST_F(EnumSetTest, Difference) {
  const TestEnumSet enums1(TEST_3, TEST_4);
  const TestEnumSet enums2(TEST_2, TEST_3);
  const TestEnumSet enums3 = Difference(enums1, enums2);

  EXPECT_EQ(TestEnumSet(TEST_4), enums3);
}

}  // namespace
}  // namespace syncer
