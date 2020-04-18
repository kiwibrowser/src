// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/prefs/pref_value_map.h"

#include "base/values.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace base {
namespace {

TEST(PrefValueMapTest, SetValue) {
  PrefValueMap map;
  const Value* result = nullptr;
  EXPECT_FALSE(map.GetValue("key", &result));
  EXPECT_FALSE(result);

  EXPECT_TRUE(map.SetValue("key", std::make_unique<Value>("test")));
  EXPECT_FALSE(map.SetValue("key", std::make_unique<Value>("test")));
  EXPECT_TRUE(map.SetValue("key", std::make_unique<Value>("hi mom!")));

  EXPECT_TRUE(map.GetValue("key", &result));
  EXPECT_TRUE(Value("hi mom!").Equals(result));
}

TEST(PrefValueMapTest, GetAndSetIntegerValue) {
  PrefValueMap map;
  ASSERT_TRUE(map.SetValue("key", std::make_unique<Value>(5)));

  int int_value = 0;
  EXPECT_TRUE(map.GetInteger("key", &int_value));
  EXPECT_EQ(5, int_value);

  map.SetInteger("key", -14);
  EXPECT_TRUE(map.GetInteger("key", &int_value));
  EXPECT_EQ(-14, int_value);
}

TEST(PrefValueMapTest, SetDoubleValue) {
  PrefValueMap map;
  ASSERT_TRUE(map.SetValue("key", std::make_unique<Value>(5.5)));

  const Value* result = nullptr;
  ASSERT_TRUE(map.GetValue("key", &result));
  double double_value = 0.;
  EXPECT_TRUE(result->GetAsDouble(&double_value));
  EXPECT_DOUBLE_EQ(5.5, double_value);
}

TEST(PrefValueMapTest, RemoveValue) {
  PrefValueMap map;
  EXPECT_FALSE(map.RemoveValue("key"));

  EXPECT_TRUE(map.SetValue("key", std::make_unique<Value>("test")));
  EXPECT_TRUE(map.GetValue("key", nullptr));

  EXPECT_TRUE(map.RemoveValue("key"));
  EXPECT_FALSE(map.GetValue("key", nullptr));

  EXPECT_FALSE(map.RemoveValue("key"));
}

TEST(PrefValueMapTest, Clear) {
  PrefValueMap map;
  EXPECT_TRUE(map.SetValue("key", std::make_unique<Value>("test")));
  EXPECT_TRUE(map.GetValue("key", nullptr));

  map.Clear();

  EXPECT_FALSE(map.GetValue("key", nullptr));
}

TEST(PrefValueMapTest, GetDifferingKeys) {
  PrefValueMap reference;
  EXPECT_TRUE(reference.SetValue("b", std::make_unique<Value>("test")));
  EXPECT_TRUE(reference.SetValue("c", std::make_unique<Value>("test")));
  EXPECT_TRUE(reference.SetValue("e", std::make_unique<Value>("test")));

  PrefValueMap check;
  std::vector<std::string> differing_paths;
  std::vector<std::string> expected_differing_paths;

  reference.GetDifferingKeys(&check, &differing_paths);
  expected_differing_paths.push_back("b");
  expected_differing_paths.push_back("c");
  expected_differing_paths.push_back("e");
  EXPECT_EQ(expected_differing_paths, differing_paths);

  EXPECT_TRUE(check.SetValue("a", std::make_unique<Value>("test")));
  EXPECT_TRUE(check.SetValue("c", std::make_unique<Value>("test")));
  EXPECT_TRUE(check.SetValue("d", std::make_unique<Value>("test")));

  reference.GetDifferingKeys(&check, &differing_paths);
  expected_differing_paths.clear();
  expected_differing_paths.push_back("a");
  expected_differing_paths.push_back("b");
  expected_differing_paths.push_back("d");
  expected_differing_paths.push_back("e");
  EXPECT_EQ(expected_differing_paths, differing_paths);
}

TEST(PrefValueMapTest, SwapTwoMaps) {
  PrefValueMap first_map;
  EXPECT_TRUE(first_map.SetValue("a", std::make_unique<Value>("test")));
  EXPECT_TRUE(first_map.SetValue("b", std::make_unique<Value>("test")));
  EXPECT_TRUE(first_map.SetValue("c", std::make_unique<Value>("test")));

  PrefValueMap second_map;
  EXPECT_TRUE(second_map.SetValue("d", std::make_unique<Value>("test")));
  EXPECT_TRUE(second_map.SetValue("e", std::make_unique<Value>("test")));
  EXPECT_TRUE(second_map.SetValue("f", std::make_unique<Value>("test")));

  first_map.Swap(&second_map);

  EXPECT_TRUE(first_map.GetValue("d", nullptr));
  EXPECT_TRUE(first_map.GetValue("e", nullptr));
  EXPECT_TRUE(first_map.GetValue("f", nullptr));

  EXPECT_TRUE(second_map.GetValue("a", nullptr));
  EXPECT_TRUE(second_map.GetValue("b", nullptr));
  EXPECT_TRUE(second_map.GetValue("c", nullptr));
}

}  // namespace
}  // namespace base
