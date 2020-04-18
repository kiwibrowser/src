// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/platform/scheduler/main_thread/main_thread_metrics_helper.h"

#include <memory>
#include "base/macros.h"
#include "base/test/histogram_tester.h"
#include "base/test/simple_test_tick_clock.h"
#include "components/viz/test/ordered_simple_task_runner.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/blink/public/common/page/launching_process_state.h"
#include "third_party/blink/renderer/platform/scheduler/base/test/task_queue_manager_for_test.h"
#include "third_party/blink/renderer/platform/scheduler/main_thread/main_thread_scheduler_impl.h"
#include "third_party/blink/renderer/platform/scheduler/public/frame_scheduler.h"
#include "third_party/blink/renderer/platform/scheduler/test/fake_frame_scheduler.h"
#include "third_party/blink/renderer/platform/scheduler/test/fake_page_scheduler.h"

using base::sequence_manager::TaskQueue;

namespace blink {
namespace scheduler {

namespace {
class MainThreadSchedulerImplForTest : public MainThreadSchedulerImpl {
 public:
  MainThreadSchedulerImplForTest(
      std::unique_ptr<base::sequence_manager::TaskQueueManager>
          task_queue_manager,
      base::Optional<base::Time> initial_virtual_time)
      : MainThreadSchedulerImpl(std::move(task_queue_manager),
                                initial_virtual_time){};

  using MainThreadSchedulerImpl::SetCurrentUseCaseForTest;
};
}  // namespace

using QueueType = MainThreadTaskQueue::QueueType;
using base::Bucket;
using testing::ElementsAre;
using testing::UnorderedElementsAre;

class MainThreadMetricsHelperTest : public testing::Test {
 public:
  MainThreadMetricsHelperTest() = default;
  ~MainThreadMetricsHelperTest() override = default;

  void SetUp() override {
    histogram_tester_.reset(new base::HistogramTester());
    clock_.Advance(base::TimeDelta::FromMilliseconds(1));
    mock_task_runner_ =
        base::MakeRefCounted<cc::OrderedSimpleTaskRunner>(&clock_, true);
    scheduler_ = std::make_unique<MainThreadSchedulerImplForTest>(
        base::sequence_manager::TaskQueueManagerForTest::Create(
            nullptr, mock_task_runner_, &clock_),
        base::nullopt);
    metrics_helper_ = &scheduler_->main_thread_only().metrics_helper;
  }

  void TearDown() override {
    scheduler_->Shutdown();
    scheduler_.reset();
  }

  void RunTask(MainThreadTaskQueue::QueueType queue_type,
               base::TimeTicks start,
               base::TimeDelta duration) {
    DCHECK_LE(clock_.NowTicks(), start);
    clock_.SetNowTicks(start + duration);
    scoped_refptr<MainThreadTaskQueueForTest> queue;
    if (queue_type != MainThreadTaskQueue::QueueType::kDetached) {
      queue = scoped_refptr<MainThreadTaskQueueForTest>(
          new MainThreadTaskQueueForTest(queue_type));
    }

    // Pass an empty task for recording.
    TaskQueue::PostedTask posted_task(base::OnceClosure(), FROM_HERE);
    TaskQueue::Task task(std::move(posted_task), base::TimeTicks());
    metrics_helper_->RecordTaskMetrics(queue.get(), task, start,
                                       start + duration, base::nullopt);
  }

  void RunTask(FrameScheduler* scheduler,
               base::TimeTicks start,
               base::TimeDelta duration) {
    DCHECK_LE(clock_.NowTicks(), start);
    clock_.SetNowTicks(start + duration);
    scoped_refptr<MainThreadTaskQueueForTest> queue(
        new MainThreadTaskQueueForTest(QueueType::kDefault));
    queue->SetFrameSchedulerForTest(scheduler);
    // Pass an empty task for recording.
    TaskQueue::PostedTask posted_task(base::OnceClosure(), FROM_HERE);
    TaskQueue::Task task(std::move(posted_task), base::TimeTicks());
    metrics_helper_->RecordTaskMetrics(queue.get(), task, start,
                                       start + duration, base::nullopt);
  }

  void RunTask(UseCase use_case,
               base::TimeTicks start,
               base::TimeDelta duration) {
    DCHECK_LE(clock_.NowTicks(), start);
    clock_.SetNowTicks(start + duration);
    scoped_refptr<MainThreadTaskQueueForTest> queue(
        new MainThreadTaskQueueForTest(QueueType::kDefault));
    scheduler_->SetCurrentUseCaseForTest(use_case);
    // Pass an empty task for recording.
    TaskQueue::PostedTask posted_task(base::OnceClosure(), FROM_HERE);
    TaskQueue::Task task(std::move(posted_task), base::TimeTicks());
    metrics_helper_->RecordTaskMetrics(queue.get(), task, start,
                                       start + duration, base::nullopt);
  }

  base::TimeTicks Milliseconds(int milliseconds) {
    return base::TimeTicks() + base::TimeDelta::FromMilliseconds(milliseconds);
  }

  void ForceUpdatePolicy() { scheduler_->ForceUpdatePolicy(); }

  std::unique_ptr<FakeFrameScheduler> CreateFakeFrameSchedulerWithType(
      FrameStatus frame_status) {
    FakeFrameScheduler::Builder builder;
    switch (frame_status) {
      case FrameStatus::kNone:
      case FrameStatus::kDetached:
        return nullptr;
      case FrameStatus::kMainFrameVisible:
        builder.SetFrameType(FrameScheduler::FrameType::kMainFrame)
            .SetIsPageVisible(true)
            .SetIsFrameVisible(true);
        break;
      case FrameStatus::kMainFrameVisibleService:
        builder.SetFrameType(FrameScheduler::FrameType::kMainFrame)
            .SetPageScheduler(playing_view_.get())
            .SetIsFrameVisible(true);
        break;
      case FrameStatus::kMainFrameHidden:
        builder.SetFrameType(FrameScheduler::FrameType::kMainFrame)
            .SetIsPageVisible(true);
        break;
      case FrameStatus::kMainFrameHiddenService:
        builder.SetFrameType(FrameScheduler::FrameType::kMainFrame)
            .SetPageScheduler(playing_view_.get());
        break;
      case FrameStatus::kMainFrameBackground:
        builder.SetFrameType(FrameScheduler::FrameType::kMainFrame);
        break;
      case FrameStatus::kMainFrameBackgroundExemptSelf:
        builder.SetFrameType(FrameScheduler::FrameType::kMainFrame)
            .SetIsExemptFromThrottling(true);
        break;
      case FrameStatus::kMainFrameBackgroundExemptOther:
        builder.SetFrameType(FrameScheduler::FrameType::kMainFrame)
            .SetPageScheduler(throtting_exempt_view_.get());
        break;
      case FrameStatus::kSameOriginVisible:
        builder.SetFrameType(FrameScheduler::FrameType::kSubframe)
            .SetIsPageVisible(true)
            .SetIsFrameVisible(true);
        break;
      case FrameStatus::kSameOriginVisibleService:
        builder.SetFrameType(FrameScheduler::FrameType::kSubframe)
            .SetPageScheduler(playing_view_.get())
            .SetIsFrameVisible(true);
        break;
      case FrameStatus::kSameOriginHidden:
        builder.SetFrameType(FrameScheduler::FrameType::kSubframe)
            .SetIsPageVisible(true);
        break;
      case FrameStatus::kSameOriginHiddenService:
        builder.SetFrameType(FrameScheduler::FrameType::kSubframe)
            .SetPageScheduler(playing_view_.get());
        break;
      case FrameStatus::kSameOriginBackground:
        builder.SetFrameType(FrameScheduler::FrameType::kSubframe);
        break;
      case FrameStatus::kSameOriginBackgroundExemptSelf:
        builder.SetFrameType(FrameScheduler::FrameType::kSubframe)
            .SetIsExemptFromThrottling(true);
        break;
      case FrameStatus::kSameOriginBackgroundExemptOther:
        builder.SetFrameType(FrameScheduler::FrameType::kSubframe)
            .SetPageScheduler(throtting_exempt_view_.get());
        break;
      case FrameStatus::kCrossOriginVisible:
        builder.SetFrameType(FrameScheduler::FrameType::kSubframe)
            .SetIsCrossOrigin(true)
            .SetIsPageVisible(true)
            .SetIsFrameVisible(true);
        break;
      case FrameStatus::kCrossOriginVisibleService:
        builder.SetFrameType(FrameScheduler::FrameType::kSubframe)
            .SetIsCrossOrigin(true)
            .SetPageScheduler(playing_view_.get())
            .SetIsFrameVisible(true);
        break;
      case FrameStatus::kCrossOriginHidden:
        builder.SetFrameType(FrameScheduler::FrameType::kSubframe)
            .SetIsCrossOrigin(true)
            .SetIsPageVisible(true);
        break;
      case FrameStatus::kCrossOriginHiddenService:
        builder.SetFrameType(FrameScheduler::FrameType::kSubframe)
            .SetIsCrossOrigin(true)
            .SetPageScheduler(playing_view_.get());
        break;
      case FrameStatus::kCrossOriginBackground:
        builder.SetFrameType(FrameScheduler::FrameType::kSubframe)
            .SetIsCrossOrigin(true);
        break;
      case FrameStatus::kCrossOriginBackgroundExemptSelf:
        builder.SetFrameType(FrameScheduler::FrameType::kSubframe)
            .SetIsCrossOrigin(true)
            .SetIsExemptFromThrottling(true);
        break;
      case FrameStatus::kCrossOriginBackgroundExemptOther:
        builder.SetFrameType(FrameScheduler::FrameType::kSubframe)
            .SetIsCrossOrigin(true)
            .SetPageScheduler(throtting_exempt_view_.get());
        break;
      case FrameStatus::kCount:
        NOTREACHED();
        return nullptr;
    }
    return builder.Build();
  }

  base::SimpleTestTickClock clock_;
  scoped_refptr<cc::OrderedSimpleTaskRunner> mock_task_runner_;
  std::unique_ptr<MainThreadSchedulerImplForTest> scheduler_;
  MainThreadMetricsHelper* metrics_helper_;  // NOT OWNED
  std::unique_ptr<base::HistogramTester> histogram_tester_;
  std::unique_ptr<FakePageScheduler> playing_view_ =
      FakePageScheduler::Builder().SetIsAudioPlaying(true).Build();
  std::unique_ptr<FakePageScheduler> throtting_exempt_view_ =
      FakePageScheduler::Builder().SetIsThrottlingExempt(true).Build();

  DISALLOW_COPY_AND_ASSIGN(MainThreadMetricsHelperTest);
};

TEST_F(MainThreadMetricsHelperTest, Metrics_PerQueueType) {
  // QueueType::kDefault is checking sub-millisecond task aggregation,
  // FRAME_* tasks are checking normal task aggregation and other
  // queue types have a single task.

  // Make sure that it starts in a foregrounded state.
  if (kLaunchingProcessIsBackgrounded)
    scheduler_->SetRendererBackgrounded(false);

  RunTask(QueueType::kDefault, Milliseconds(1),
          base::TimeDelta::FromMicroseconds(700));
  RunTask(QueueType::kDefault, Milliseconds(2),
          base::TimeDelta::FromMicroseconds(700));
  RunTask(QueueType::kDefault, Milliseconds(3),
          base::TimeDelta::FromMicroseconds(700));

  RunTask(QueueType::kControl, Milliseconds(400),
          base::TimeDelta::FromMilliseconds(30));
  RunTask(QueueType::kFrameLoading, Milliseconds(800),
          base::TimeDelta::FromMilliseconds(70));
  RunTask(QueueType::kFramePausable, Milliseconds(1000),
          base::TimeDelta::FromMilliseconds(20));
  RunTask(QueueType::kCompositor, Milliseconds(1200),
          base::TimeDelta::FromMilliseconds(25));
  RunTask(QueueType::kTest, Milliseconds(1600),
          base::TimeDelta::FromMilliseconds(85));

  scheduler_->SetRendererBackgrounded(true);

  RunTask(QueueType::kControl, Milliseconds(2000),
          base::TimeDelta::FromMilliseconds(25));
  RunTask(QueueType::kFrameThrottleable, Milliseconds(2600),
          base::TimeDelta::FromMilliseconds(175));
  RunTask(QueueType::kUnthrottled, Milliseconds(2800),
          base::TimeDelta::FromMilliseconds(25));
  RunTask(QueueType::kFrameLoading, Milliseconds(3000),
          base::TimeDelta::FromMilliseconds(35));
  RunTask(QueueType::kFrameThrottleable, Milliseconds(3200),
          base::TimeDelta::FromMilliseconds(5));
  RunTask(QueueType::kCompositor, Milliseconds(3400),
          base::TimeDelta::FromMilliseconds(20));
  RunTask(QueueType::kIdle, Milliseconds(3600),
          base::TimeDelta::FromMilliseconds(50));
  RunTask(QueueType::kFrameLoadingControl, Milliseconds(4000),
          base::TimeDelta::FromMilliseconds(5));
  RunTask(QueueType::kControl, Milliseconds(4200),
          base::TimeDelta::FromMilliseconds(20));
  RunTask(QueueType::kFrameThrottleable, Milliseconds(4400),
          base::TimeDelta::FromMilliseconds(115));
  RunTask(QueueType::kFramePausable, Milliseconds(4600),
          base::TimeDelta::FromMilliseconds(175));
  RunTask(QueueType::kIdle, Milliseconds(5000),
          base::TimeDelta::FromMilliseconds(1600));

  RunTask(QueueType::kDetached, Milliseconds(8000),
          base::TimeDelta::FromMilliseconds(150));

  std::vector<base::Bucket> expected_samples = {
      {static_cast<int>(QueueType::kControl), 75},
      {static_cast<int>(QueueType::kDefault), 2},
      {static_cast<int>(QueueType::kUnthrottled), 25},
      {static_cast<int>(QueueType::kFrameLoading), 105},
      {static_cast<int>(QueueType::kCompositor), 45},
      {static_cast<int>(QueueType::kIdle), 1650},
      {static_cast<int>(QueueType::kTest), 85},
      {static_cast<int>(QueueType::kFrameLoadingControl), 5},
      {static_cast<int>(QueueType::kFrameThrottleable), 295},
      {static_cast<int>(QueueType::kFramePausable), 195},
      {static_cast<int>(QueueType::kDetached), 150},
  };
  EXPECT_THAT(histogram_tester_->GetAllSamples(
                  "RendererScheduler.TaskDurationPerQueueType2"),
              testing::ContainerEq(expected_samples));

  EXPECT_THAT(histogram_tester_->GetAllSamples(
                  "RendererScheduler.TaskDurationPerQueueType2.Foreground"),
              UnorderedElementsAre(
                  Bucket(static_cast<int>(QueueType::kControl), 30),
                  Bucket(static_cast<int>(QueueType::kDefault), 2),
                  Bucket(static_cast<int>(QueueType::kFrameLoading), 70),
                  Bucket(static_cast<int>(QueueType::kCompositor), 25),
                  Bucket(static_cast<int>(QueueType::kTest), 85),
                  Bucket(static_cast<int>(QueueType::kFramePausable), 20)));

  EXPECT_THAT(histogram_tester_->GetAllSamples(
                  "RendererScheduler.TaskDurationPerQueueType2.Background"),
              UnorderedElementsAre(
                  Bucket(static_cast<int>(QueueType::kControl), 45),
                  Bucket(static_cast<int>(QueueType::kUnthrottled), 25),
                  Bucket(static_cast<int>(QueueType::kFrameLoading), 35),
                  Bucket(static_cast<int>(QueueType::kFrameThrottleable), 295),
                  Bucket(static_cast<int>(QueueType::kFramePausable), 175),
                  Bucket(static_cast<int>(QueueType::kCompositor), 20),
                  Bucket(static_cast<int>(QueueType::kIdle), 1650),
                  Bucket(static_cast<int>(QueueType::kFrameLoadingControl), 5),
                  Bucket(static_cast<int>(QueueType::kDetached), 150)));
}

TEST_F(MainThreadMetricsHelperTest, Metrics_PerUseCase) {
  RunTask(UseCase::kNone, Milliseconds(500),
          base::TimeDelta::FromMilliseconds(4000));

  RunTask(UseCase::kTouchstart, Milliseconds(7000),
          base::TimeDelta::FromMilliseconds(25));
  RunTask(UseCase::kTouchstart, Milliseconds(7050),
          base::TimeDelta::FromMilliseconds(25));
  RunTask(UseCase::kTouchstart, Milliseconds(7100),
          base::TimeDelta::FromMilliseconds(25));

  RunTask(UseCase::kCompositorGesture, Milliseconds(7150),
          base::TimeDelta::FromMilliseconds(5));
  RunTask(UseCase::kCompositorGesture, Milliseconds(7200),
          base::TimeDelta::FromMilliseconds(30));

  RunTask(UseCase::kMainThreadCustomInputHandling, Milliseconds(7300),
          base::TimeDelta::FromMilliseconds(2));
  RunTask(UseCase::kSynchronizedGesture, Milliseconds(7400),
          base::TimeDelta::FromMilliseconds(250));
  RunTask(UseCase::kMainThreadCustomInputHandling, Milliseconds(7700),
          base::TimeDelta::FromMilliseconds(150));
  RunTask(UseCase::kLoading, Milliseconds(7900),
          base::TimeDelta::FromMilliseconds(50));
  RunTask(UseCase::kMainThreadGesture, Milliseconds(8000),
          base::TimeDelta::FromMilliseconds(60));
  EXPECT_THAT(
      histogram_tester_->GetAllSamples(
          "RendererScheduler.TaskDurationPerUseCase"),
      UnorderedElementsAre(
          Bucket(static_cast<int>(UseCase::kNone), 4000),
          Bucket(static_cast<int>(UseCase::kCompositorGesture), 35),
          Bucket(static_cast<int>(UseCase::kMainThreadCustomInputHandling),
                 152),
          Bucket(static_cast<int>(UseCase::kSynchronizedGesture), 250),
          Bucket(static_cast<int>(UseCase::kTouchstart), 75),
          Bucket(static_cast<int>(UseCase::kLoading), 50),
          Bucket(static_cast<int>(UseCase::kMainThreadGesture), 60)));
}

TEST_F(MainThreadMetricsHelperTest, GetFrameStatusTest) {
  DCHECK_EQ(GetFrameStatus(nullptr), FrameStatus::kNone);

  FrameStatus frame_statuses_tested[] = {
      FrameStatus::kMainFrameVisible,
      FrameStatus::kSameOriginHidden,
      FrameStatus::kCrossOriginHidden,
      FrameStatus::kSameOriginBackground,
      FrameStatus::kMainFrameBackgroundExemptSelf,
      FrameStatus::kSameOriginVisibleService,
      FrameStatus::kCrossOriginHiddenService,
      FrameStatus::kMainFrameBackgroundExemptOther};
  for (FrameStatus frame_status : frame_statuses_tested) {
    std::unique_ptr<FakeFrameScheduler> frame =
        CreateFakeFrameSchedulerWithType(frame_status);
    EXPECT_EQ(GetFrameStatus(frame.get()), frame_status);
  }
}

TEST_F(MainThreadMetricsHelperTest, BackgroundedRendererTransition) {
  scheduler_->SetFreezingWhenBackgroundedEnabled(true);
  typedef BackgroundedRendererTransition Transition;

  int backgrounding_transitions = 0;
  int foregrounding_transitions = 0;
  if (!kLaunchingProcessIsBackgrounded) {
    scheduler_->SetRendererBackgrounded(true);
    backgrounding_transitions++;
    EXPECT_THAT(
        histogram_tester_->GetAllSamples(
            "RendererScheduler.BackgroundedRendererTransition"),
        UnorderedElementsAre(Bucket(static_cast<int>(Transition::kBackgrounded),
                                    backgrounding_transitions)));
    scheduler_->SetRendererBackgrounded(false);
    foregrounding_transitions++;
    EXPECT_THAT(
        histogram_tester_->GetAllSamples(
            "RendererScheduler.BackgroundedRendererTransition"),
        UnorderedElementsAre(Bucket(static_cast<int>(Transition::kBackgrounded),
                                    backgrounding_transitions),
                             Bucket(static_cast<int>(Transition::kForegrounded),
                                    foregrounding_transitions)));
  } else {
    scheduler_->SetRendererBackgrounded(false);
    foregrounding_transitions++;
    EXPECT_THAT(
        histogram_tester_->GetAllSamples(
            "RendererScheduler.BackgroundedRendererTransition"),
        UnorderedElementsAre(Bucket(static_cast<int>(Transition::kForegrounded),
                                    foregrounding_transitions)));
  }

  scheduler_->SetRendererBackgrounded(true);
  backgrounding_transitions++;
  EXPECT_THAT(
      histogram_tester_->GetAllSamples(
          "RendererScheduler.BackgroundedRendererTransition"),
      UnorderedElementsAre(Bucket(static_cast<int>(Transition::kBackgrounded),
                                  backgrounding_transitions),
                           Bucket(static_cast<int>(Transition::kForegrounded),
                                  foregrounding_transitions)));

  // Waste 5+ minutes so that the delayed stop is triggered
  RunTask(QueueType::kDefault, Milliseconds(1),
          base::TimeDelta::FromSeconds(5 * 61));
  // Firing ForceUpdatePolicy multiple times to make sure that the
  // metric is only recorded upon an actual change.
  ForceUpdatePolicy();
  ForceUpdatePolicy();
  ForceUpdatePolicy();
  EXPECT_THAT(histogram_tester_->GetAllSamples(
                  "RendererScheduler.BackgroundedRendererTransition"),
              UnorderedElementsAre(
                  Bucket(static_cast<int>(Transition::kBackgrounded),
                         backgrounding_transitions),
                  Bucket(static_cast<int>(Transition::kForegrounded),
                         foregrounding_transitions),
                  Bucket(static_cast<int>(Transition::kFrozenAfterDelay), 1)));

  scheduler_->SetRendererBackgrounded(false);
  foregrounding_transitions++;
  ForceUpdatePolicy();
  ForceUpdatePolicy();
  EXPECT_THAT(histogram_tester_->GetAllSamples(
                  "RendererScheduler.BackgroundedRendererTransition"),
              UnorderedElementsAre(
                  Bucket(static_cast<int>(Transition::kBackgrounded),
                         backgrounding_transitions),
                  Bucket(static_cast<int>(Transition::kForegrounded),
                         foregrounding_transitions),
                  Bucket(static_cast<int>(Transition::kFrozenAfterDelay), 1),
                  Bucket(static_cast<int>(Transition::kResumed), 1)));
}

TEST_F(MainThreadMetricsHelperTest, TaskCountPerFrameStatus) {
  int task_count = 0;
  struct CountPerFrameStatus {
    FrameStatus frame_status;
    int count;
  };
  CountPerFrameStatus test_data[] = {
      {FrameStatus::kNone, 4},
      {FrameStatus::kMainFrameVisible, 8},
      {FrameStatus::kMainFrameBackgroundExemptSelf, 5},
      {FrameStatus::kCrossOriginHidden, 3},
      {FrameStatus::kCrossOriginHiddenService, 7},
      {FrameStatus::kCrossOriginVisible, 1},
      {FrameStatus::kMainFrameBackgroundExemptOther, 2},
      {FrameStatus::kSameOriginVisible, 10},
      {FrameStatus::kSameOriginBackground, 9},
      {FrameStatus::kSameOriginVisibleService, 6}};

  for (const auto& data : test_data) {
    std::unique_ptr<FakeFrameScheduler> frame =
        CreateFakeFrameSchedulerWithType(data.frame_status);
    for (int i = 0; i < data.count; ++i) {
      RunTask(frame.get(), Milliseconds(++task_count),
              base::TimeDelta::FromMicroseconds(100));
    }
  }

  EXPECT_THAT(
      histogram_tester_->GetAllSamples(
          "RendererScheduler.TaskCountPerFrameType"),
      UnorderedElementsAre(
          Bucket(static_cast<int>(FrameStatus::kNone), 4),
          Bucket(static_cast<int>(FrameStatus::kMainFrameVisible), 8),
          Bucket(static_cast<int>(FrameStatus::kMainFrameBackgroundExemptSelf),
                 5),
          Bucket(static_cast<int>(FrameStatus::kMainFrameBackgroundExemptOther),
                 2),
          Bucket(static_cast<int>(FrameStatus::kSameOriginVisible), 10),
          Bucket(static_cast<int>(FrameStatus::kSameOriginVisibleService), 6),
          Bucket(static_cast<int>(FrameStatus::kSameOriginBackground), 9),
          Bucket(static_cast<int>(FrameStatus::kCrossOriginVisible), 1),
          Bucket(static_cast<int>(FrameStatus::kCrossOriginHidden), 3),
          Bucket(static_cast<int>(FrameStatus::kCrossOriginHiddenService), 7)));
}

TEST_F(MainThreadMetricsHelperTest, TaskCountPerFrameTypeLongerThan) {
  int total_duration = 0;
  struct TasksPerFrameStatus {
    FrameStatus frame_status;
    std::vector<int> durations;
  };
  TasksPerFrameStatus test_data[] = {
      {FrameStatus::kSameOriginHidden,
       {2, 15, 16, 20, 25, 30, 49, 50, 73, 99, 100, 110, 140, 150, 800, 1000,
        1200}},
      {FrameStatus::kCrossOriginVisibleService,
       {5, 10, 18, 19, 20, 55, 75, 220}},
      {FrameStatus::kMainFrameBackground,
       {21, 31, 41, 51, 61, 71, 81, 91, 101, 1001}},
  };

  for (const auto& data : test_data) {
    std::unique_ptr<FakeFrameScheduler> frame =
        CreateFakeFrameSchedulerWithType(data.frame_status);
    for (size_t i = 0; i < data.durations.size(); ++i) {
      RunTask(frame.get(), Milliseconds(++total_duration),
              base::TimeDelta::FromMilliseconds(data.durations[i]));
      total_duration += data.durations[i];
    }
  }

  EXPECT_THAT(
      histogram_tester_->GetAllSamples(
          "RendererScheduler.TaskCountPerFrameType"),
      UnorderedElementsAre(
          Bucket(static_cast<int>(FrameStatus::kMainFrameBackground), 10),
          Bucket(static_cast<int>(FrameStatus::kSameOriginHidden), 17),
          Bucket(static_cast<int>(FrameStatus::kCrossOriginVisibleService),
                 8)));

  EXPECT_THAT(
      histogram_tester_->GetAllSamples(
          "RendererScheduler.TaskCountPerFrameType."
          "LongerThan16ms"),
      UnorderedElementsAre(
          Bucket(static_cast<int>(FrameStatus::kMainFrameBackground), 10),
          Bucket(static_cast<int>(FrameStatus::kSameOriginHidden), 15),
          Bucket(static_cast<int>(FrameStatus::kCrossOriginVisibleService),
                 6)));

  EXPECT_THAT(
      histogram_tester_->GetAllSamples(
          "RendererScheduler.TaskCountPerFrameType."
          "LongerThan50ms"),
      UnorderedElementsAre(
          Bucket(static_cast<int>(FrameStatus::kMainFrameBackground), 7),
          Bucket(static_cast<int>(FrameStatus::kSameOriginHidden), 10),
          Bucket(static_cast<int>(FrameStatus::kCrossOriginVisibleService),
                 3)));

  EXPECT_THAT(
      histogram_tester_->GetAllSamples(
          "RendererScheduler.TaskCountPerFrameType."
          "LongerThan100ms"),
      UnorderedElementsAre(
          Bucket(static_cast<int>(FrameStatus::kMainFrameBackground), 2),
          Bucket(static_cast<int>(FrameStatus::kSameOriginHidden), 7),
          Bucket(static_cast<int>(FrameStatus::kCrossOriginVisibleService),
                 1)));

  EXPECT_THAT(
      histogram_tester_->GetAllSamples(
          "RendererScheduler.TaskCountPerFrameType."
          "LongerThan150ms"),
      UnorderedElementsAre(
          Bucket(static_cast<int>(FrameStatus::kMainFrameBackground), 1),
          Bucket(static_cast<int>(FrameStatus::kSameOriginHidden), 4),
          Bucket(static_cast<int>(FrameStatus::kCrossOriginVisibleService),
                 1)));

  EXPECT_THAT(
      histogram_tester_->GetAllSamples(
          "RendererScheduler.TaskCountPerFrameType.LongerThan1s"),
      UnorderedElementsAre(
          Bucket(static_cast<int>(FrameStatus::kMainFrameBackground), 1),
          Bucket(static_cast<int>(FrameStatus::kSameOriginHidden), 2)));
}

// TODO(crbug.com/754656): Add tests for NthMinute and
// AfterNthMinute histograms.

// TODO(crbug.com/754656): Add tests for
// TaskDuration.Hidden/Visible histograms.

// TODO(crbug.com/754656): Add tests for non-TaskDuration
// histograms.

}  // namespace scheduler
}  // namespace blink
