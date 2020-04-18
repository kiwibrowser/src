// Copyright (c) 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/logging.h"
#include "base/test/gtest_util.h"
#include "chrome/test/chromedriver/net/timeout.h"
#include "testing/gtest/include/gtest/gtest.h"

using base::TimeDelta;

TEST(TimeoutTest, Basics) {
  Timeout timeout;
  EXPECT_FALSE(timeout.is_set());
  EXPECT_FALSE(timeout.IsExpired());
  EXPECT_EQ(TimeDelta::Max(), timeout.GetDuration());
  EXPECT_EQ(TimeDelta::Max(), timeout.GetRemainingTime());

  timeout.SetDuration(TimeDelta());
  EXPECT_TRUE(timeout.is_set());
  EXPECT_TRUE(timeout.IsExpired());
  EXPECT_EQ(TimeDelta(), timeout.GetDuration());
  EXPECT_GE(TimeDelta(), timeout.GetRemainingTime());
}

TEST(TimeoutTest, SetDuration) {
  Timeout timeout(TimeDelta::FromSeconds(1));

  // It's ok to set the same duration again, since nothing changes.
  timeout.SetDuration(TimeDelta::FromSeconds(1));

  EXPECT_DCHECK_DEATH(timeout.SetDuration(TimeDelta::FromMinutes(30)));
}

TEST(TimeoutTest, Derive) {
  Timeout timeout(TimeDelta::FromMinutes(5));
  EXPECT_TRUE(timeout.is_set());
  EXPECT_FALSE(timeout.IsExpired());
  EXPECT_EQ(TimeDelta::FromMinutes(5), timeout.GetDuration());
  EXPECT_GE(TimeDelta::FromMinutes(5), timeout.GetRemainingTime());

  Timeout small = Timeout(TimeDelta::FromSeconds(10), &timeout);
  EXPECT_TRUE(small.is_set());
  EXPECT_FALSE(small.IsExpired());
  EXPECT_EQ(TimeDelta::FromSeconds(10), small.GetDuration());

  Timeout large = Timeout(TimeDelta::FromMinutes(30), &timeout);
  EXPECT_TRUE(large.is_set());
  EXPECT_FALSE(large.IsExpired());
  EXPECT_GE(timeout.GetDuration(), large.GetDuration());
}

TEST(TimeoutTest, DeriveExpired) {
  Timeout timeout((TimeDelta()));
  EXPECT_TRUE(timeout.is_set());
  EXPECT_TRUE(timeout.IsExpired());

  Timeout derived = Timeout(TimeDelta::FromSeconds(10), &timeout);
  EXPECT_TRUE(derived.is_set());
  EXPECT_TRUE(derived.IsExpired());
  EXPECT_GE(TimeDelta(), derived.GetDuration());
}
