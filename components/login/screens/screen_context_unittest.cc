// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/login/screens/screen_context.h"

#include <algorithm>
#include <memory>
#include <vector>

#include "testing/gtest/include/gtest/gtest.h"

namespace login {

class ScreenContextTest : public testing::Test {
 public:
  ScreenContextTest() {}
  ~ScreenContextTest() override {}

  void SetUp() override { context_.reset(new ScreenContext()); }

  void TearDown() override {}

 protected:
  ScreenContext& context() { return *context_; }

 private:
  std::unique_ptr<ScreenContext> context_;
};

TEST_F(ScreenContextTest, Simple) {
  ASSERT_FALSE(context().HasChanges());

  ASSERT_FALSE(context().HasKey("key0"));

  bool rv = context().SetBoolean("key0", true);
  ASSERT_TRUE(rv);
  ASSERT_TRUE(context().HasKey("key0"));
  ASSERT_TRUE(context().GetBoolean("key0"));
  ASSERT_TRUE(context().GetBoolean("key0", false));
  ASSERT_TRUE(context().HasChanges());

  rv = context().SetBoolean("key0", true);
  ASSERT_FALSE(rv);

  rv = context().SetBoolean("key0", false);
  ASSERT_TRUE(rv);
  ASSERT_TRUE(context().HasKey("key0"));
  ASSERT_FALSE(context().GetBoolean("key0"));
  ASSERT_FALSE(context().GetBoolean("key0", true));
  ASSERT_TRUE(context().HasChanges());

  ASSERT_FALSE(context().HasKey("key1"));

  ASSERT_EQ(1, context().GetInteger("key1", 1));
  rv = context().SetInteger("key1", 2);
  ASSERT_TRUE(rv);
  ASSERT_TRUE(context().HasKey("key1"));
  ASSERT_EQ(2, context().GetInteger("key1"));
  ASSERT_EQ(2, context().GetInteger("key1", 1));
}

TEST_F(ScreenContextTest, Changes) {
  ASSERT_FALSE(context().HasChanges());

  bool rv = context().SetInteger("key0", 2);
  ASSERT_TRUE(rv);
  ASSERT_EQ(2, context().GetInteger("key0"));
  ASSERT_TRUE(context().HasChanges());

  base::DictionaryValue changes;
  context().GetChangesAndReset(&changes);
  ASSERT_FALSE(context().HasChanges());

  ASSERT_EQ(1u, changes.size());
  int value;
  rv = changes.GetInteger("key0", &value);
  ASSERT_TRUE(rv);
  ASSERT_EQ(2, value);

  rv = context().SetInteger("key0", 3);
  ASSERT_TRUE(rv);
  ASSERT_EQ(3, context().GetInteger("key0", 3));
  ASSERT_TRUE(context().HasChanges());

  rv = context().SetInteger("key0", 2);
  ASSERT_TRUE(rv);
  ASSERT_TRUE(context().HasChanges());
}

TEST_F(ScreenContextTest, ComplexChanges) {
  ASSERT_FALSE(context().HasChanges());

  context().SetString("key0", "value0");
  context().SetBoolean("key1", true);
  context().SetDouble("key2", 3.14159);
  ASSERT_TRUE(context().HasChanges());

  // Get all changes and verify them.
  base::DictionaryValue changes;
  context().GetChangesAndReset(&changes);
  ASSERT_FALSE(context().HasChanges());
  ASSERT_EQ(3u, changes.size());

  std::string string_value;
  bool bool_value;
  double double_value;
  bool rv = changes.GetString("key0", &string_value);
  ASSERT_TRUE(rv);
  rv = changes.GetBoolean("key1", &bool_value);
  ASSERT_TRUE(rv);
  rv = changes.GetDouble("key2", &double_value);
  ASSERT_TRUE(rv);
  ASSERT_EQ("value0", string_value);
  ASSERT_EQ(true, bool_value);
  ASSERT_DOUBLE_EQ(3.14159, double_value);

  context().SetString("key0", "value1");
  ASSERT_TRUE(context().HasChanges());

  context().SetString("key0", "value0");
  ASSERT_TRUE(context().HasChanges());

  context().GetChangesAndReset(&changes);
  ASSERT_FALSE(context().HasChanges());
  ASSERT_EQ(1u, changes.size());
  rv = changes.GetString("key0", &string_value);
  ASSERT_TRUE(rv);
  ASSERT_EQ("value0", string_value);
}

TEST_F(ScreenContextTest, ApplyChanges) {
  ASSERT_FALSE(context().HasChanges());

  base::DictionaryValue changes;
  changes.SetString("key0", "value0");
  changes.SetInteger("key1", 1);
  changes.SetBoolean("key2", true);

  std::vector<std::string> keys;
  context().ApplyChanges(changes, &keys);

  ASSERT_EQ(3u, keys.size());
  std::sort(keys.begin(), keys.end());
  ASSERT_EQ("key0", keys[0]);
  ASSERT_EQ("key1", keys[1]);
  ASSERT_EQ("key2", keys[2]);

  ASSERT_FALSE(context().HasChanges());
  ASSERT_EQ("value0", context().GetString("key0"));
  ASSERT_EQ(1, context().GetInteger("key1"));
  ASSERT_TRUE(context().GetBoolean("key2"));
}

}  // namespace login
