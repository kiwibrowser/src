// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/power/ml/recent_events_counter.h"

#include "base/time/time.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace chromeos {
namespace power {
namespace ml {

TEST(RecentEventsCounterTest, TimeTest) {
  base::TimeDelta minute = base::TimeDelta::FromMinutes(1);
  RecentEventsCounter counter(base::TimeDelta::FromHours(1), 60);
  ASSERT_EQ(counter.GetTotal(minute), 0);

  counter.Log(5 * minute);
  ASSERT_EQ(counter.GetTotal(10 * minute), 1);

  counter.Log(5 * minute);
  ASSERT_EQ(counter.GetTotal(10 * minute), 2);

  counter.Log(25.4 * minute);

  ASSERT_EQ(counter.GetTotal(30 * minute), 3);
  ASSERT_EQ(counter.GetTotal(70 * minute), 1);
  // Event at 25.4 minutes is counted 59 minutes later.
  ASSERT_EQ(counter.GetTotal(84.4 * minute), 1);
  // Event at 25.4 minutes is not counted 59.7 minutes later at 85.1 minutes. An
  // an event logged at 85.1 minutes would wipe out the event at 25.4 minutes,
  // so the event at 25.4 minutes cannot be counted to ensure consistency.
  ASSERT_EQ(counter.GetTotal(85.1 * minute), 0);

  counter.Log(75 * minute);
  ASSERT_EQ(counter.GetTotal(80 * minute), 2);
  ASSERT_EQ(counter.GetTotal(90 * minute), 1);

  // Overwrite the 25.4 minute logging.
  counter.Log(85.1 * minute);
  ASSERT_EQ(counter.GetTotal(90 * minute), 2);

  counter.Log(200 * minute);
  ASSERT_EQ(counter.GetTotal(210 * minute), 1);

  ASSERT_EQ(counter.GetTotal(300 * minute), 0);
}

}  // namespace ml
}  // namespace power
}  // namespace chromeos
