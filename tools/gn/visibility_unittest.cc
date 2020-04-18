// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "testing/gtest/include/gtest/gtest.h"
#include "tools/gn/err.h"
#include "tools/gn/label.h"
#include "tools/gn/scope.h"
#include "tools/gn/value.h"
#include "tools/gn/visibility.h"

TEST(Visibility, CanSeeMe) {
  Value list(nullptr, Value::LIST);
  list.list_value().push_back(Value(nullptr, "//rec/*"));    // Recursive.
  list.list_value().push_back(Value(nullptr, "//dir:*"));    // One dir.
  list.list_value().push_back(Value(nullptr, "//my:name"));  // Exact match.

  Err err;
  Visibility vis;
  ASSERT_TRUE(vis.Set(SourceDir("//"), list, &err));

  EXPECT_FALSE(vis.CanSeeMe(Label(SourceDir("//random/"), "thing")));
  EXPECT_FALSE(vis.CanSeeMe(Label(SourceDir("//my/"), "notname")));

  EXPECT_TRUE(vis.CanSeeMe(Label(SourceDir("//my/"), "name")));
  EXPECT_TRUE(vis.CanSeeMe(Label(SourceDir("//rec/"), "anything")));
  EXPECT_TRUE(vis.CanSeeMe(Label(SourceDir("//rec/a/"), "anything")));
  EXPECT_TRUE(vis.CanSeeMe(Label(SourceDir("//rec/b/"), "anything")));
  EXPECT_TRUE(vis.CanSeeMe(Label(SourceDir("//dir/"), "anything")));
  EXPECT_FALSE(vis.CanSeeMe(Label(SourceDir("//dir/a/"), "anything")));
  EXPECT_FALSE(vis.CanSeeMe(Label(SourceDir("//directory/"), "anything")));
}

TEST(Visibility, Public) {
  Err err;
  Visibility vis;

  Value list(nullptr, Value::LIST);
  list.list_value().push_back(Value(nullptr, "*"));
  ASSERT_TRUE(vis.Set(SourceDir("//"), list, &err));

  EXPECT_TRUE(vis.CanSeeMe(Label(SourceDir("//random/"), "thing")));
  EXPECT_TRUE(vis.CanSeeMe(Label(SourceDir("//"), "")));
}

TEST(Visibility, Private) {
  Err err;
  Visibility vis;
  ASSERT_TRUE(vis.Set(SourceDir("//"), Value(nullptr, Value::LIST), &err));

  EXPECT_FALSE(vis.CanSeeMe(Label(SourceDir("//random/"), "thing")));
  EXPECT_FALSE(vis.CanSeeMe(Label(SourceDir("//"), "")));
}
