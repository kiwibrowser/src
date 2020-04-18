// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/time/time.h"
#include "media/base/moving_average.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace media {

TEST(MovingAverageTest, AverageAndDeviation) {
  const int kSamples = 5;
  MovingAverage moving_average(kSamples);
  moving_average.AddSample(base::TimeDelta::FromSeconds(1));
  EXPECT_EQ(base::TimeDelta::FromSeconds(1), moving_average.Average());
  EXPECT_EQ(base::TimeDelta(), moving_average.Deviation());

  for (int i = 0; i < kSamples - 1; ++i)
    moving_average.AddSample(base::TimeDelta::FromSeconds(1));
  EXPECT_EQ(base::TimeDelta::FromSeconds(1), moving_average.Average());
  EXPECT_EQ(base::TimeDelta(), moving_average.Deviation());

  base::TimeDelta expect_deviation[] = {
      base::TimeDelta::FromMicroseconds(200000),
      base::TimeDelta::FromMicroseconds(244948),
      base::TimeDelta::FromMicroseconds(244948),
      base::TimeDelta::FromMicroseconds(200000),
      base::TimeDelta::FromMilliseconds(0),
  };
  for (int i = 0; i < kSamples; ++i) {
    moving_average.AddSample(base::TimeDelta::FromMilliseconds(500));
    EXPECT_EQ(base::TimeDelta::FromMilliseconds(1000 - (i + 1) * 100),
              moving_average.Average());
    EXPECT_EQ(expect_deviation[i], moving_average.Deviation());
  }
}

TEST(MovingAverageTest, Reset) {
  MovingAverage moving_average(2);
  moving_average.AddSample(base::TimeDelta::FromSeconds(1));
  EXPECT_EQ(base::TimeDelta::FromSeconds(1), moving_average.Average());
  moving_average.Reset();
  moving_average.AddSample(base::TimeDelta());
  EXPECT_EQ(base::TimeDelta(), moving_average.Average());
  EXPECT_EQ(base::TimeDelta(), moving_average.Deviation());
}

}  // namespace media
