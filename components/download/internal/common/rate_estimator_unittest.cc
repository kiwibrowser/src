// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/download/public/common/rate_estimator.h"

#include "testing/gtest/include/gtest/gtest.h"

using base::TimeDelta;

namespace download {

TEST(RateEstimatorTest, RateEstimator) {
  base::TimeTicks now;
  RateEstimator estimator(TimeDelta::FromSeconds(1), 10u, now);
  EXPECT_EQ(0u, estimator.GetCountPerSecond(now));

  estimator.Increment(50u, now);
  EXPECT_EQ(50u, estimator.GetCountPerSecond(now));

  now += TimeDelta::FromMilliseconds(800);
  estimator.Increment(50, now);
  EXPECT_EQ(100u, estimator.GetCountPerSecond(now));

  // Advance time.
  now += TimeDelta::FromSeconds(3);
  EXPECT_EQ(25u, estimator.GetCountPerSecond(now));
  estimator.Increment(60, now);
  EXPECT_EQ(40u, estimator.GetCountPerSecond(now));

  // Advance time again.
  now += TimeDelta::FromSeconds(4);
  EXPECT_EQ(20u, estimator.GetCountPerSecond(now));

  // Advance time to the end.
  now += TimeDelta::FromSeconds(2);
  EXPECT_EQ(16u, estimator.GetCountPerSecond(now));
  estimator.Increment(100, now);
  EXPECT_EQ(26u, estimator.GetCountPerSecond(now));

  // Now wrap around to the start.
  now += TimeDelta::FromSeconds(1);
  EXPECT_EQ(16u, estimator.GetCountPerSecond(now));
  estimator.Increment(100, now);
  EXPECT_EQ(26u, estimator.GetCountPerSecond(now));

  // Advance far into the future.
  now += TimeDelta::FromSeconds(40);
  EXPECT_EQ(0u, estimator.GetCountPerSecond(now));
  estimator.Increment(100, now);
  EXPECT_EQ(100u, estimator.GetCountPerSecond(now));

  // Pretend that there is timeticks wrap around.
  now = base::TimeTicks();
  EXPECT_EQ(0u, estimator.GetCountPerSecond(now));
}

}  // namespace download
