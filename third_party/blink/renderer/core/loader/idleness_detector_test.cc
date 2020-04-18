// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/loader/idleness_detector.h"

#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/blink/renderer/core/testing/page_test_base.h"
#include "third_party/blink/renderer/platform/testing/testing_platform_support_with_mock_scheduler.h"

namespace blink {

class IdlenessDetectorTest : public PageTestBase {
 protected:
  void SetUp() override {
    platform_time_ = 1;
    platform_->AdvanceClockSeconds(platform_time_);
    PageTestBase::SetUp();
  }

  IdlenessDetector* Detector() { return GetFrame().GetIdlenessDetector(); }

  bool IsNetworkQuietTimerActive() {
    return Detector()->network_quiet_timer_.IsActive();
  }

  bool HadNetworkQuiet() {
    return !Detector()->in_network_2_quiet_period_ &&
           !Detector()->in_network_0_quiet_period_;
  }

  void WillProcessTask(double start_time) {
    DCHECK(start_time >= platform_time_);
    platform_->AdvanceClockSeconds(start_time - platform_time_);
    platform_time_ = start_time;
    Detector()->WillProcessTask(start_time);
  }

  void DidProcessTask(double start_time, double end_time) {
    DCHECK(start_time < end_time);
    platform_->AdvanceClockSeconds(end_time - start_time);
    platform_time_ = end_time;
    Detector()->DidProcessTask(start_time, end_time);
  }

 protected:
  ScopedTestingPlatformSupport<TestingPlatformSupportWithMockScheduler>
      platform_;

 private:
  double platform_time_;
};

TEST_F(IdlenessDetectorTest, NetworkQuietBasic) {
  EXPECT_TRUE(IsNetworkQuietTimerActive());

  WillProcessTask(1);
  DidProcessTask(1, 1.01);

  WillProcessTask(1.52);
  EXPECT_TRUE(HadNetworkQuiet());
  DidProcessTask(1.52, 1.53);
}

TEST_F(IdlenessDetectorTest, NetworkQuietWithLongTask) {
  EXPECT_TRUE(IsNetworkQuietTimerActive());

  WillProcessTask(1);
  DidProcessTask(1, 1.01);

  WillProcessTask(1.02);
  DidProcessTask(1.02, 1.6);
  EXPECT_FALSE(HadNetworkQuiet());

  WillProcessTask(2.11);
  EXPECT_TRUE(HadNetworkQuiet());
  DidProcessTask(2.11, 2.12);
}

TEST_F(IdlenessDetectorTest, NetworkQuietWatchdogTimerFired) {
  EXPECT_TRUE(IsNetworkQuietTimerActive());

  WillProcessTask(1);
  DidProcessTask(1, 1.01);

  platform_->RunForPeriodSeconds(3);
  EXPECT_FALSE(IsNetworkQuietTimerActive());
  EXPECT_TRUE(HadNetworkQuiet());
}

}  // namespace blink
