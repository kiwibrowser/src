// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/power/ml/real_boot_clock.h"

#include "base/threading/platform_thread.h"
#include "base/time/time.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace chromeos {
namespace power {
namespace ml {

class RealBootClockTest : public testing::Test {
 public:
  ~RealBootClockTest() override = default;

 protected:
  RealBootClock boot_clock_;
};

TEST_F(RealBootClockTest, Basic) {
  const base::TimeDelta sleep_duration = base::TimeDelta::FromMilliseconds(10);
  const base::TimeDelta init_time_since_boot = boot_clock_.GetTimeSinceBoot();
  EXPECT_GE(init_time_since_boot, base::TimeDelta());
  const base::TimeDelta expected_end_time_since_boot =
      init_time_since_boot + sleep_duration;

  base::PlatformThread::Sleep(sleep_duration);
  EXPECT_GE(boot_clock_.GetTimeSinceBoot(), expected_end_time_since_boot);
}

}  // namespace ml
}  // namespace power
}  // namespace chromeos
