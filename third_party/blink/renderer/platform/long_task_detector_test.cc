// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/platform/long_task_detector.h"

#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/blink/renderer/platform/cross_thread_functional.h"
#include "third_party/blink/renderer/platform/testing/testing_platform_support_with_mock_scheduler.h"
#include "third_party/blink/renderer/platform/web_task_runner.h"
#include "third_party/blink/renderer/platform/wtf/time.h"

namespace blink {

namespace {
class TestLongTaskObserver :
    // This has to be garbage collected since LongTaskObserver uses
    // GarbageCollectedMixin.
    public GarbageCollectedFinalized<TestLongTaskObserver>,
    public LongTaskObserver {
  USING_GARBAGE_COLLECTED_MIXIN(TestLongTaskObserver);

 public:
  TimeTicks last_long_task_start;
  TimeTicks last_long_task_end;

  // LongTaskObserver implementation.
  void OnLongTaskDetected(TimeTicks start_time, TimeTicks end_time) override {
    last_long_task_start = start_time;
    last_long_task_end = end_time;
  }
};  // Anonymous namespace

}  // namespace
class LongTaskDetectorTest : public testing::Test {
 public:
  // Public because it's executed on a task queue.
  void DummyTaskWithDuration(double duration_seconds) {
    dummy_task_start_time_ = CurrentTimeTicks();
    platform_->AdvanceClockSeconds(duration_seconds);
    dummy_task_end_time_ = CurrentTimeTicks();
  }

 protected:
  void SetUp() override {
    // For some reason, platform needs to run for non-zero seconds before we
    // start posting tasks to it. Otherwise TaskTimeObservers don't get notified
    // of tasks.
    platform_->RunForPeriodSeconds(1);
  }
  TimeTicks DummyTaskStartTime() { return dummy_task_start_time_; }

  TimeTicks DummyTaskEndTime() { return dummy_task_end_time_; }

  void SimulateTask(double duration_seconds) {
    PostCrossThreadTask(
        *platform_->CurrentThread()->GetTaskRunner(), FROM_HERE,
        CrossThreadBind(&LongTaskDetectorTest::DummyTaskWithDuration,
                        CrossThreadUnretained(this), duration_seconds));
    platform_->RunUntilIdle();
  }

  ScopedTestingPlatformSupport<TestingPlatformSupportWithMockScheduler>
      platform_;

 private:
  TimeTicks dummy_task_start_time_;
  TimeTicks dummy_task_end_time_;
};

TEST_F(LongTaskDetectorTest, DeliversLongTaskNotificationOnlyWhenRegistered) {
  TestLongTaskObserver* long_task_observer = new TestLongTaskObserver();
  SimulateTask(LongTaskDetector::kLongTaskThresholdSeconds + 0.01);
  EXPECT_EQ(long_task_observer->last_long_task_end, TimeTicks());

  LongTaskDetector::Instance().RegisterObserver(long_task_observer);
  SimulateTask(LongTaskDetector::kLongTaskThresholdSeconds + 0.01);
  TimeTicks long_task_end_when_registered = DummyTaskEndTime();
  EXPECT_EQ(long_task_observer->last_long_task_start, DummyTaskStartTime());
  EXPECT_EQ(long_task_observer->last_long_task_end,
            long_task_end_when_registered);

  LongTaskDetector::Instance().UnregisterObserver(long_task_observer);
  SimulateTask(LongTaskDetector::kLongTaskThresholdSeconds + 0.01);
  // Check that we have a long task after unregistering observer.
  ASSERT_FALSE(long_task_end_when_registered == DummyTaskEndTime());
  EXPECT_EQ(long_task_observer->last_long_task_end,
            long_task_end_when_registered);
}

TEST_F(LongTaskDetectorTest, DoesNotGetNotifiedOfShortTasks) {
  TestLongTaskObserver* long_task_observer = new TestLongTaskObserver();
  LongTaskDetector::Instance().RegisterObserver(long_task_observer);
  SimulateTask(LongTaskDetector::kLongTaskThresholdSeconds - 0.01);
  EXPECT_EQ(long_task_observer->last_long_task_end, TimeTicks());

  SimulateTask(LongTaskDetector::kLongTaskThresholdSeconds + 0.01);
  EXPECT_EQ(long_task_observer->last_long_task_end, DummyTaskEndTime());
  LongTaskDetector::Instance().UnregisterObserver(long_task_observer);
}

TEST_F(LongTaskDetectorTest, RegisterSameObserverTwice) {
  TestLongTaskObserver* long_task_observer = new TestLongTaskObserver();
  LongTaskDetector::Instance().RegisterObserver(long_task_observer);
  LongTaskDetector::Instance().RegisterObserver(long_task_observer);

  SimulateTask(LongTaskDetector::kLongTaskThresholdSeconds + 0.01);
  TimeTicks long_task_end_when_registered = DummyTaskEndTime();
  EXPECT_EQ(long_task_observer->last_long_task_start, DummyTaskStartTime());
  EXPECT_EQ(long_task_observer->last_long_task_end,
            long_task_end_when_registered);

  LongTaskDetector::Instance().UnregisterObserver(long_task_observer);
  // Should only need to unregister once even after we called RegisterObserver
  // twice.
  SimulateTask(LongTaskDetector::kLongTaskThresholdSeconds + 0.01);
  ASSERT_FALSE(long_task_end_when_registered == DummyTaskEndTime());
  EXPECT_EQ(long_task_observer->last_long_task_end,
            long_task_end_when_registered);
}

}  // namespace blink
