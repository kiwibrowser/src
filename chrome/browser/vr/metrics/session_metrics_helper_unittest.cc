// Copyright (c) 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/vr/metrics/session_metrics_helper.h"
#include "components/ukm/content/source_url_recorder.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace vr {

class FakeUkmEvent {
  void Record(ukm::UkmRecorder* recorder) {}
};

class SessionMetricsHelperTest : public testing::Test {
 public:
  SessionMetricsHelperTest() {}
  DISALLOW_COPY_AND_ASSIGN(SessionMetricsHelperTest);
};

TEST_F(SessionMetricsHelperTest, SessionTrackerGetRoundedDurationInSeconds) {
  SessionTracker<FakeUkmEvent> tracker(std::make_unique<FakeUkmEvent>());
  base::Time now = base::Time::Now();

  tracker.SetSessionEnd(now + base::TimeDelta::FromSeconds(8));
  EXPECT_EQ(tracker.GetRoundedDurationInSeconds(), 8);

  // 3 min and 7 seconds, round down to nearest minute
  tracker.SetSessionEnd(now + base::TimeDelta::FromSeconds(187));
  EXPECT_EQ(tracker.GetRoundedDurationInSeconds(), 180);

  // 22 minutes 34 seconds, rounds down to nearest 10 minutes
  tracker.SetSessionEnd(now + base::TimeDelta::FromSeconds(1254));
  EXPECT_EQ(tracker.GetRoundedDurationInSeconds(), 1200);

  // 2 hours, 10 minutes, 22 seconds, rounds down nearest hour
  tracker.SetSessionEnd(now + base::TimeDelta::FromSeconds(7822));
  EXPECT_EQ(tracker.GetRoundedDurationInSeconds(), 7200);
}

}  // namespace vr
