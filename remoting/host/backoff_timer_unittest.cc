// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "remoting/host/backoff_timer.h"

#include "base/memory/ptr_util.h"
#include "base/timer/mock_timer.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace remoting {

namespace {

void IncrementCounter(int* counter) {
  ++(*counter);
}

}  // namespace

TEST(BackoffTimer, Basic) {
  base::MockTimer* mock_timer = new base::MockTimer(false, false);
  BackoffTimer backoff_timer;
  backoff_timer.SetTimerForTest(base::WrapUnique(mock_timer));
  ASSERT_FALSE(backoff_timer.IsRunning());

  int counter = 0;
  backoff_timer.Start(FROM_HERE, base::TimeDelta::FromMilliseconds(10),
                      base::TimeDelta::FromMilliseconds(50),
                      base::Bind(&IncrementCounter, &counter));
  ASSERT_TRUE(backoff_timer.IsRunning());
  ASSERT_EQ(0, counter);
  ASSERT_NEAR(0, mock_timer->GetCurrentDelay().InMillisecondsF(), 1);

  mock_timer->Fire();
  ASSERT_TRUE(backoff_timer.IsRunning());
  ASSERT_EQ(1, counter);
  EXPECT_NEAR(10, mock_timer->GetCurrentDelay().InMillisecondsF(), 1);

  mock_timer->Fire();
  ASSERT_TRUE(backoff_timer.IsRunning());
  ASSERT_EQ(2, counter);
  EXPECT_NEAR(20, mock_timer->GetCurrentDelay().InMillisecondsF(), 1);

  mock_timer->Fire();
  ASSERT_TRUE(backoff_timer.IsRunning());
  ASSERT_EQ(3, counter);
  EXPECT_NEAR(40, mock_timer->GetCurrentDelay().InMillisecondsF(), 1);

  mock_timer->Fire();
  ASSERT_TRUE(backoff_timer.IsRunning());
  ASSERT_EQ(4, counter);
  EXPECT_NEAR(50, mock_timer->GetCurrentDelay().InMillisecondsF(), 1);

  mock_timer->Fire();
  ASSERT_TRUE(backoff_timer.IsRunning());
  ASSERT_EQ(5, counter);
  EXPECT_NEAR(50, mock_timer->GetCurrentDelay().InMillisecondsF(), 1);

  backoff_timer.Stop();
  ASSERT_FALSE(backoff_timer.IsRunning());
}

}  // namespace remoting
