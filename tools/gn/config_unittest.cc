// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "testing/gtest/include/gtest/gtest.h"
#include "tools/gn/config.h"
#include "tools/gn/test_with_scope.h"

// Tests that the "resolved" values are the same as "own" values when there
// are no subconfigs.
TEST(Config, ResolvedNoSub) {
  TestWithScope setup;
  Err err;

  Config config(setup.settings(), Label(SourceDir("//foo/"), "bar"));
  config.own_values().defines().push_back("FOO");
  ASSERT_TRUE(config.OnResolved(&err));

  // The resolved values should be the same as the value we put in to
  // own_values().
  ASSERT_EQ(1u, config.resolved_values().defines().size());
  EXPECT_EQ("FOO", config.resolved_values().defines()[0]);

  // As an optimization, the string should actually refer to the original. This
  // isn't required to pass for semantic correctness, though.
  EXPECT_TRUE(&config.own_values() == &config.resolved_values());
}

// Tests that subconfigs are resolved in the correct order.
TEST(Config, ResolvedSub) {
  TestWithScope setup;
  Err err;

  Config sub1(setup.settings(), Label(SourceDir("//foo/"), "1"));
  sub1.own_values().defines().push_back("ONE");
  ASSERT_TRUE(sub1.OnResolved(&err));

  Config sub2(setup.settings(), Label(SourceDir("//foo/"), "2"));
  sub2.own_values().defines().push_back("TWO");
  ASSERT_TRUE(sub2.OnResolved(&err));

  Config config(setup.settings(), Label(SourceDir("//foo/"), "bar"));
  config.own_values().defines().push_back("FOO");
  config.configs().push_back(LabelConfigPair(&sub1));
  config.configs().push_back(LabelConfigPair(&sub2));
  ASSERT_TRUE(config.OnResolved(&err));

  // The resolved values should be the same as the value we put in to
  // own_values().
  ASSERT_EQ(3u, config.resolved_values().defines().size());
  EXPECT_EQ("FOO", config.resolved_values().defines()[0]);
  EXPECT_EQ("ONE", config.resolved_values().defines()[1]);
  EXPECT_EQ("TWO", config.resolved_values().defines()[2]);

  // The "own" values should be unchanged.
  ASSERT_EQ(1u, config.own_values().defines().size());
  EXPECT_EQ("FOO", config.own_values().defines()[0]);
}

// Tests that subconfigs of subconfigs are resolved properly.
TEST(Config, SubSub) {
  TestWithScope setup;
  Err err;

  // Set up first -> middle -> last configs.
  Config last(setup.settings(), Label(SourceDir("//foo/"), "last"));
  last.own_values().defines().push_back("LAST");
  ASSERT_TRUE(last.OnResolved(&err));

  Config middle(setup.settings(), Label(SourceDir("//foo/"), "middle"));
  middle.own_values().defines().push_back("MIDDLE");
  middle.configs().push_back(LabelConfigPair(&last));
  ASSERT_TRUE(middle.OnResolved(&err));

  Config first(setup.settings(), Label(SourceDir("//foo/"), "first"));
  first.own_values().defines().push_back("FIRST");
  first.configs().push_back(LabelConfigPair(&middle));
  ASSERT_TRUE(first.OnResolved(&err));

  // Check final resolved defines on "first".
  ASSERT_EQ(3u, first.resolved_values().defines().size());
  EXPECT_EQ("FIRST", first.resolved_values().defines()[0]);
  EXPECT_EQ("MIDDLE", first.resolved_values().defines()[1]);
  EXPECT_EQ("LAST", first.resolved_values().defines()[2]);
}
