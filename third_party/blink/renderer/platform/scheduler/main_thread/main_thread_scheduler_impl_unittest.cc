// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/platform/scheduler/main_thread/main_thread_scheduler_impl.h"

#include <memory>
#include <utility>

#include "base/callback.h"
#include "base/macros.h"
#include "base/message_loop/message_loop.h"
#include "base/run_loop.h"
#include "base/single_thread_task_runner.h"
#include "base/test/histogram_tester.h"
#include "base/test/scoped_feature_list.h"
#include "base/test/simple_test_tick_clock.h"
#include "build/build_config.h"
#include "components/viz/common/frame_sinks/begin_frame_args.h"
#include "components/viz/test/ordered_simple_task_runner.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/blink/public/common/page/launching_process_state.h"
#include "third_party/blink/renderer/platform/scheduler/base/real_time_domain.h"
#include "third_party/blink/renderer/platform/scheduler/base/test/task_queue_manager_for_test.h"
#include "third_party/blink/renderer/platform/scheduler/child/features.h"
#include "third_party/blink/renderer/platform/scheduler/common/throttling/budget_pool.h"
#include "third_party/blink/renderer/platform/scheduler/main_thread/auto_advancing_virtual_time_domain.h"
#include "third_party/blink/renderer/platform/scheduler/main_thread/frame_scheduler_impl.h"
#include "third_party/blink/renderer/platform/testing/runtime_enabled_features_test_helpers.h"

using base::sequence_manager::TaskQueue;

namespace blink {
namespace scheduler {
// To avoid symbol collisions in jumbo builds.
namespace main_thread_scheduler_impl_unittest {

using testing::Mock;
using InputEventState = WebMainThreadScheduler::InputEventState;

class FakeInputEvent : public blink::WebInputEvent {
 public:
  explicit FakeInputEvent(blink::WebInputEvent::Type event_type,
                          int modifiers = WebInputEvent::kNoModifiers)
      : WebInputEvent(sizeof(FakeInputEvent),
                      event_type,
                      modifiers,
                      WebInputEvent::GetStaticTimeStampForTests()) {}
};

void AppendToVectorTestTask(std::vector<std::string>* vector,
                            std::string value) {
  vector->push_back(value);
}

void AppendToVectorIdleTestTask(std::vector<std::string>* vector,
                                std::string value,
                                base::TimeTicks deadline) {
  AppendToVectorTestTask(vector, value);
}

void NullTask() {}

void AppendToVectorReentrantTask(base::SingleThreadTaskRunner* task_runner,
                                 std::vector<int>* vector,
                                 int* reentrant_count,
                                 int max_reentrant_count) {
  vector->push_back((*reentrant_count)++);
  if (*reentrant_count < max_reentrant_count) {
    task_runner->PostTask(FROM_HERE,
                          base::BindOnce(AppendToVectorReentrantTask,
                                         base::Unretained(task_runner), vector,
                                         reentrant_count, max_reentrant_count));
  }
}

void IdleTestTask(int* run_count,
                  base::TimeTicks* deadline_out,
                  base::TimeTicks deadline) {
  (*run_count)++;
  *deadline_out = deadline;
}

int g_max_idle_task_reposts = 2;

void RepostingIdleTestTask(SingleThreadIdleTaskRunner* idle_task_runner,
                           int* run_count,
                           base::TimeTicks deadline) {
  if ((*run_count + 1) < g_max_idle_task_reposts) {
    idle_task_runner->PostIdleTask(
        FROM_HERE,
        base::BindOnce(&RepostingIdleTestTask,
                       base::Unretained(idle_task_runner), run_count));
  }
  (*run_count)++;
}

void RepostingUpdateClockIdleTestTask(
    SingleThreadIdleTaskRunner* idle_task_runner,
    int* run_count,
    base::SimpleTestTickClock* clock,
    base::TimeDelta advance_time,
    std::vector<base::TimeTicks>* deadlines,
    base::TimeTicks deadline) {
  if ((*run_count + 1) < g_max_idle_task_reposts) {
    idle_task_runner->PostIdleTask(
        FROM_HERE, base::BindOnce(&RepostingUpdateClockIdleTestTask,
                                  base::Unretained(idle_task_runner), run_count,
                                  clock, advance_time, deadlines));
  }
  deadlines->push_back(deadline);
  (*run_count)++;
  clock->Advance(advance_time);
}

void WillBeginFrameIdleTask(WebMainThreadScheduler* scheduler,
                            uint64_t sequence_number,
                            base::SimpleTestTickClock* clock,
                            base::TimeTicks deadline) {
  scheduler->WillBeginFrame(viz::BeginFrameArgs::Create(
      BEGINFRAME_FROM_HERE, 0, sequence_number, clock->NowTicks(),
      base::TimeTicks(), base::TimeDelta::FromMilliseconds(1000),
      viz::BeginFrameArgs::NORMAL));
}

void UpdateClockToDeadlineIdleTestTask(base::SimpleTestTickClock* clock,
                                       int* run_count,
                                       base::TimeTicks deadline) {
  clock->Advance(deadline - clock->NowTicks());
  (*run_count)++;
}

void PostingYieldingTestTask(MainThreadSchedulerImpl* scheduler,
                             base::SingleThreadTaskRunner* task_runner,
                             bool simulate_input,
                             bool* should_yield_before,
                             bool* should_yield_after) {
  *should_yield_before = scheduler->ShouldYieldForHighPriorityWork();
  task_runner->PostTask(FROM_HERE, base::BindOnce(NullTask));
  if (simulate_input) {
    scheduler->DidHandleInputEventOnCompositorThread(
        FakeInputEvent(blink::WebInputEvent::kTouchMove),
        InputEventState::EVENT_CONSUMED_BY_COMPOSITOR);
  }
  *should_yield_after = scheduler->ShouldYieldForHighPriorityWork();
}

enum class SimulateInputType {
  kNone,
  kTouchStart,
  kTouchEnd,
  kGestureScrollBegin,
  kGestureScrollEnd
};

void AnticipationTestTask(MainThreadSchedulerImpl* scheduler,
                          SimulateInputType simulate_input,
                          bool* is_anticipated_before,
                          bool* is_anticipated_after) {
  *is_anticipated_before = scheduler->IsHighPriorityWorkAnticipated();
  switch (simulate_input) {
    case SimulateInputType::kNone:
      break;

    case SimulateInputType::kTouchStart:
      scheduler->DidHandleInputEventOnCompositorThread(
          FakeInputEvent(blink::WebInputEvent::kTouchStart),
          InputEventState::EVENT_CONSUMED_BY_COMPOSITOR);
      break;

    case SimulateInputType::kTouchEnd:
      scheduler->DidHandleInputEventOnCompositorThread(
          FakeInputEvent(blink::WebInputEvent::kTouchEnd),
          InputEventState::EVENT_CONSUMED_BY_COMPOSITOR);
      break;

    case SimulateInputType::kGestureScrollBegin:
      scheduler->DidHandleInputEventOnCompositorThread(
          FakeInputEvent(blink::WebInputEvent::kGestureScrollBegin),
          InputEventState::EVENT_CONSUMED_BY_COMPOSITOR);
      break;

    case SimulateInputType::kGestureScrollEnd:
      scheduler->DidHandleInputEventOnCompositorThread(
          FakeInputEvent(blink::WebInputEvent::kGestureScrollEnd),
          InputEventState::EVENT_CONSUMED_BY_COMPOSITOR);
      break;
  }
  *is_anticipated_after = scheduler->IsHighPriorityWorkAnticipated();
}

// RAII helper class to enable auto advancing of time inside mock task runner.
// Automatically disables auto-advancement when destroyed.
class ScopedAutoAdvanceNowEnabler {
 public:
  ScopedAutoAdvanceNowEnabler(
      scoped_refptr<cc::OrderedSimpleTaskRunner> task_runner)
      : task_runner_(task_runner) {
    task_runner_->SetAutoAdvanceNowToPendingTasks(true);
  }

  ~ScopedAutoAdvanceNowEnabler() {
    task_runner_->SetAutoAdvanceNowToPendingTasks(false);
  }

 private:
  scoped_refptr<cc::OrderedSimpleTaskRunner> task_runner_;

  DISALLOW_COPY_AND_ASSIGN(ScopedAutoAdvanceNowEnabler);
};

class MainThreadSchedulerImplForTest : public MainThreadSchedulerImpl {
 public:
  using MainThreadSchedulerImpl::EstimateLongestJankFreeTaskDuration;
  using MainThreadSchedulerImpl::OnIdlePeriodEnded;
  using MainThreadSchedulerImpl::OnIdlePeriodStarted;
  using MainThreadSchedulerImpl::OnPendingTasksChanged;
  using MainThreadSchedulerImpl::CompositorTaskQueue;
  using MainThreadSchedulerImpl::ControlTaskQueue;
  using MainThreadSchedulerImpl::DefaultTaskQueue;
  using MainThreadSchedulerImpl::InputTaskQueue;
  using MainThreadSchedulerImpl::V8TaskQueue;
  using MainThreadSchedulerImpl::VirtualTimeControlTaskQueue;

  MainThreadSchedulerImplForTest(
      std::unique_ptr<base::sequence_manager::TaskQueueManager> manager,
      base::Optional<base::Time> initial_virtual_time)
      : MainThreadSchedulerImpl(std::move(manager), initial_virtual_time),
        update_policy_count_(0) {}

  void UpdatePolicyLocked(UpdateType update_type) override {
    update_policy_count_++;
    MainThreadSchedulerImpl::UpdatePolicyLocked(update_type);

    std::string use_case = MainThreadSchedulerImpl::UseCaseToString(
        main_thread_only().current_use_case);
    if (main_thread_only().touchstart_expected_soon) {
      use_cases_.push_back(use_case + " touchstart expected");
    } else {
      use_cases_.push_back(use_case);
    }
  }

  void EnsureUrgentPolicyUpdatePostedOnMainThread() {
    base::AutoLock lock(any_thread_lock_);
    MainThreadSchedulerImpl::EnsureUrgentPolicyUpdatePostedOnMainThread(
        FROM_HERE);
  }

  void ScheduleDelayedPolicyUpdate(base::TimeTicks now, base::TimeDelta delay) {
    delayed_update_policy_runner_.SetDeadline(FROM_HERE, delay, now);
  }

  bool BeginMainFrameOnCriticalPath() {
    base::AutoLock lock(any_thread_lock_);
    return any_thread().begin_main_frame_on_critical_path;
  }

  bool waiting_for_meaningful_paint() const {
    base::AutoLock lock(any_thread_lock_);
    return any_thread().waiting_for_meaningful_paint;
  }

  VirtualTimePolicy virtual_time_policy() const {
    return main_thread_only().virtual_time_policy;
  }

  int update_policy_count_;
  std::vector<std::string> use_cases_;
};

// Lets gtest print human readable Policy values.
::std::ostream& operator<<(::std::ostream& os, const UseCase& use_case) {
  return os << MainThreadSchedulerImpl::UseCaseToString(use_case);
}

class MainThreadSchedulerImplTest : public testing::Test {
 public:
  MainThreadSchedulerImplTest()
      : fake_task_(TaskQueue::PostedTask(base::BindOnce([] {}), FROM_HERE),
                   base::TimeTicks()) {
    feature_list_.InitAndEnableFeature(kHighPriorityInput);
    clock_.Advance(base::TimeDelta::FromMicroseconds(5000));
  }

  MainThreadSchedulerImplTest(base::MessageLoop* message_loop)
      : fake_task_(TaskQueue::PostedTask(base::BindOnce([] {}), FROM_HERE),
                   base::TimeTicks()),
        message_loop_(message_loop) {
    clock_.Advance(base::TimeDelta::FromMicroseconds(5000));
  }

  ~MainThreadSchedulerImplTest() override = default;

  void SetUp() override {
    if (!message_loop_) {
      mock_task_runner_ =
          base::MakeRefCounted<cc::OrderedSimpleTaskRunner>(&clock_, false);
    }
    Initialize(std::make_unique<MainThreadSchedulerImplForTest>(
        base::sequence_manager::TaskQueueManagerForTest::Create(
            message_loop_.get(),
            message_loop_ ? message_loop_->task_runner() : mock_task_runner_,
            &clock_),
        base::nullopt));
  }

  void Initialize(std::unique_ptr<MainThreadSchedulerImplForTest> scheduler) {
    scheduler_ = std::move(scheduler);
    if (kLaunchingProcessIsBackgrounded) {
      scheduler_->SetRendererBackgrounded(false);
      // Reset the policy count as foregrounding would force an initial update.
      scheduler_->update_policy_count_ = 0;
      scheduler_->use_cases_.clear();
    }
    default_task_runner_ = scheduler_->DefaultTaskQueue();
    compositor_task_runner_ = scheduler_->CompositorTaskQueue();
    input_task_runner_ = scheduler_->InputTaskQueue();
    loading_task_runner_ = scheduler_->NewLoadingTaskQueue(
        MainThreadTaskQueue::QueueType::kFrameLoading, nullptr);
    loading_control_task_runner_ = scheduler_->NewLoadingTaskQueue(
        MainThreadTaskQueue::QueueType::kFrameLoadingControl, nullptr);
    idle_task_runner_ = scheduler_->IdleTaskRunner();
    timer_task_runner_ = scheduler_->NewTimerTaskQueue(
        MainThreadTaskQueue::QueueType::kFrameThrottleable, nullptr);
    v8_task_runner_ = scheduler_->V8TaskQueue();
    fake_queue_ = scheduler_->NewLoadingTaskQueue(
        MainThreadTaskQueue::QueueType::kFrameLoading, nullptr);
  }

  void TearDown() override {
    DCHECK(!mock_task_runner_.get() || !message_loop_.get());
    scheduler_->Shutdown();
    if (mock_task_runner_.get()) {
      // Check that all tests stop posting tasks.
      mock_task_runner_->SetAutoAdvanceNowToPendingTasks(true);
      while (mock_task_runner_->RunUntilIdle()) {
      }
    } else {
      base::RunLoop().RunUntilIdle();
    }
    scheduler_.reset();
  }

  void RunUntilIdle() {
    // Only one of mock_task_runner_ or message_loop_ should be set.
    DCHECK(!mock_task_runner_.get() || !message_loop_.get());
    if (mock_task_runner_.get()) {
      mock_task_runner_->RunUntilIdle();
    } else {
      base::RunLoop().RunUntilIdle();
    }
  }

  void DoMainFrame() {
    viz::BeginFrameArgs begin_frame_args = viz::BeginFrameArgs::Create(
        BEGINFRAME_FROM_HERE, 0, next_begin_frame_number_++, clock_.NowTicks(),
        base::TimeTicks(), base::TimeDelta::FromMilliseconds(16),
        viz::BeginFrameArgs::NORMAL);
    begin_frame_args.on_critical_path = false;
    scheduler_->WillBeginFrame(begin_frame_args);
    scheduler_->DidCommitFrameToCompositor();
  }

  void DoMainFrameOnCriticalPath() {
    viz::BeginFrameArgs begin_frame_args = viz::BeginFrameArgs::Create(
        BEGINFRAME_FROM_HERE, 0, next_begin_frame_number_++, clock_.NowTicks(),
        base::TimeTicks(), base::TimeDelta::FromMilliseconds(16),
        viz::BeginFrameArgs::NORMAL);
    begin_frame_args.on_critical_path = true;
    scheduler_->WillBeginFrame(begin_frame_args);
  }

  void ForceTouchStartToBeExpectedSoon() {
    scheduler_->DidHandleInputEventOnCompositorThread(
        FakeInputEvent(blink::WebInputEvent::kGestureScrollUpdate),
        InputEventState::EVENT_CONSUMED_BY_COMPOSITOR);
    scheduler_->DidHandleInputEventOnCompositorThread(
        FakeInputEvent(blink::WebInputEvent::kGestureScrollEnd),
        InputEventState::EVENT_CONSUMED_BY_COMPOSITOR);
    clock_.Advance(priority_escalation_after_input_duration() * 2);
    scheduler_->ForceUpdatePolicy();
  }

  void SimulateExpensiveTasks(
      const scoped_refptr<base::SingleThreadTaskRunner>& task_runner) {
    // RunUntilIdle won't actually run all of the SimpleTestTickClock::Advance
    // tasks unless we set AutoAdvanceNow to true :/
    ScopedAutoAdvanceNowEnabler enable_auto_advance_now(mock_task_runner_);

    // Simulate a bunch of expensive tasks
    for (int i = 0; i < 10; i++) {
      task_runner->PostTask(
          FROM_HERE, base::BindOnce(&base::SimpleTestTickClock::Advance,
                                    base::Unretained(&clock_),
                                    base::TimeDelta::FromMilliseconds(500)));
    }

    RunUntilIdle();
  }

  enum class TouchEventPolicy {
    kSendTouchStart,
    kDontSendTouchStart,
  };

  void SimulateCompositorGestureStart(TouchEventPolicy touch_event_policy) {
    if (touch_event_policy == TouchEventPolicy::kSendTouchStart) {
      scheduler_->DidHandleInputEventOnCompositorThread(
          FakeInputEvent(blink::WebInputEvent::kTouchStart),
          InputEventState::EVENT_FORWARDED_TO_MAIN_THREAD);
      scheduler_->DidHandleInputEventOnCompositorThread(
          FakeInputEvent(blink::WebInputEvent::kTouchMove),
          InputEventState::EVENT_FORWARDED_TO_MAIN_THREAD);
      scheduler_->DidHandleInputEventOnCompositorThread(
          FakeInputEvent(blink::WebInputEvent::kTouchMove),
          InputEventState::EVENT_FORWARDED_TO_MAIN_THREAD);
    }
    scheduler_->DidHandleInputEventOnCompositorThread(
        FakeInputEvent(blink::WebInputEvent::kGestureScrollBegin),
        InputEventState::EVENT_CONSUMED_BY_COMPOSITOR);
    scheduler_->DidHandleInputEventOnCompositorThread(
        FakeInputEvent(blink::WebInputEvent::kGestureScrollUpdate),
        InputEventState::EVENT_CONSUMED_BY_COMPOSITOR);
  }

  // Simulate a gesture where there is an active compositor scroll, but no
  // scroll updates are generated. Instead, the main thread handles
  // non-canceleable touch events, making this an effectively main thread
  // driven gesture.
  void SimulateMainThreadGestureWithoutScrollUpdates() {
    scheduler_->DidHandleInputEventOnCompositorThread(
        FakeInputEvent(blink::WebInputEvent::kTouchStart),
        InputEventState::EVENT_FORWARDED_TO_MAIN_THREAD);
    scheduler_->DidHandleInputEventOnCompositorThread(
        FakeInputEvent(blink::WebInputEvent::kTouchMove),
        InputEventState::EVENT_FORWARDED_TO_MAIN_THREAD);
    scheduler_->DidHandleInputEventOnCompositorThread(
        FakeInputEvent(blink::WebInputEvent::kGestureScrollBegin),
        InputEventState::EVENT_CONSUMED_BY_COMPOSITOR);
    scheduler_->DidHandleInputEventOnCompositorThread(
        FakeInputEvent(blink::WebInputEvent::kTouchMove),
        InputEventState::EVENT_FORWARDED_TO_MAIN_THREAD);
  }

  // Simulate a gesture where the main thread handles touch events but does not
  // preventDefault(), allowing the gesture to turn into a compositor driven
  // gesture. This function also verifies the necessary policy updates are
  // scheduled.
  void SimulateMainThreadGestureWithoutPreventDefault() {
    scheduler_->DidHandleInputEventOnCompositorThread(
        FakeInputEvent(blink::WebInputEvent::kTouchStart),
        InputEventState::EVENT_FORWARDED_TO_MAIN_THREAD);

    // Touchstart policy update.
    EXPECT_TRUE(scheduler_->PolicyNeedsUpdateForTesting());
    EXPECT_EQ(UseCase::kTouchstart, ForceUpdatePolicyAndGetCurrentUseCase());
    EXPECT_FALSE(scheduler_->PolicyNeedsUpdateForTesting());

    scheduler_->DidHandleInputEventOnCompositorThread(
        FakeInputEvent(blink::WebInputEvent::kTouchMove),
        InputEventState::EVENT_FORWARDED_TO_MAIN_THREAD);
    scheduler_->DidHandleInputEventOnCompositorThread(
        FakeInputEvent(blink::WebInputEvent::kGestureTapCancel),
        InputEventState::EVENT_FORWARDED_TO_MAIN_THREAD);
    scheduler_->DidHandleInputEventOnCompositorThread(
        FakeInputEvent(blink::WebInputEvent::kGestureScrollBegin),
        InputEventState::EVENT_CONSUMED_BY_COMPOSITOR);

    // Main thread gesture policy update.
    EXPECT_TRUE(scheduler_->PolicyNeedsUpdateForTesting());
    EXPECT_EQ(UseCase::kMainThreadCustomInputHandling,
              ForceUpdatePolicyAndGetCurrentUseCase());
    EXPECT_FALSE(scheduler_->PolicyNeedsUpdateForTesting());

    scheduler_->DidHandleInputEventOnCompositorThread(
        FakeInputEvent(blink::WebInputEvent::kGestureScrollUpdate),
        InputEventState::EVENT_CONSUMED_BY_COMPOSITOR);
    scheduler_->DidHandleInputEventOnCompositorThread(
        FakeInputEvent(blink::WebInputEvent::kTouchScrollStarted),
        InputEventState::EVENT_FORWARDED_TO_MAIN_THREAD);
    scheduler_->DidHandleInputEventOnCompositorThread(
        FakeInputEvent(blink::WebInputEvent::kTouchMove),
        InputEventState::EVENT_FORWARDED_TO_MAIN_THREAD);

    // Compositor thread gesture policy update.
    EXPECT_TRUE(scheduler_->PolicyNeedsUpdateForTesting());
    EXPECT_EQ(UseCase::kCompositorGesture,
              ForceUpdatePolicyAndGetCurrentUseCase());
    EXPECT_FALSE(scheduler_->PolicyNeedsUpdateForTesting());
  }

  void SimulateMainThreadGestureStart(TouchEventPolicy touch_event_policy,
                                      blink::WebInputEvent::Type gesture_type) {
    if (touch_event_policy == TouchEventPolicy::kSendTouchStart) {
      scheduler_->DidHandleInputEventOnCompositorThread(
          FakeInputEvent(blink::WebInputEvent::kTouchStart),
          InputEventState::EVENT_FORWARDED_TO_MAIN_THREAD);
      scheduler_->DidHandleInputEventOnMainThread(
          FakeInputEvent(blink::WebInputEvent::kTouchStart),
          WebInputEventResult::kHandledSystem);

      scheduler_->DidHandleInputEventOnCompositorThread(
          FakeInputEvent(blink::WebInputEvent::kTouchMove),
          InputEventState::EVENT_FORWARDED_TO_MAIN_THREAD);
      scheduler_->DidHandleInputEventOnMainThread(
          FakeInputEvent(blink::WebInputEvent::kTouchMove),
          WebInputEventResult::kHandledSystem);

      scheduler_->DidHandleInputEventOnCompositorThread(
          FakeInputEvent(blink::WebInputEvent::kTouchMove),
          InputEventState::EVENT_FORWARDED_TO_MAIN_THREAD);
      scheduler_->DidHandleInputEventOnMainThread(
          FakeInputEvent(blink::WebInputEvent::kTouchMove),
          WebInputEventResult::kHandledSystem);
    }
    if (gesture_type != blink::WebInputEvent::kUndefined) {
      scheduler_->DidHandleInputEventOnCompositorThread(
          FakeInputEvent(gesture_type),
          InputEventState::EVENT_FORWARDED_TO_MAIN_THREAD);
      scheduler_->DidHandleInputEventOnMainThread(
          FakeInputEvent(gesture_type), WebInputEventResult::kHandledSystem);
    }
  }

  void SimulateMainThreadInputHandlingCompositorTask(
      base::TimeDelta begin_main_frame_duration) {
    scheduler_->DidHandleInputEventOnCompositorThread(
        FakeInputEvent(blink::WebInputEvent::kTouchMove),
        InputEventState::EVENT_FORWARDED_TO_MAIN_THREAD);
    clock_.Advance(begin_main_frame_duration);
    scheduler_->DidHandleInputEventOnMainThread(
        FakeInputEvent(blink::WebInputEvent::kTouchMove),
        WebInputEventResult::kHandledApplication);
    scheduler_->DidCommitFrameToCompositor();
  }

  void SimulateMainThreadCompositorTask(
      base::TimeDelta begin_main_frame_duration) {
    clock_.Advance(begin_main_frame_duration);
    scheduler_->DidCommitFrameToCompositor();
    simulate_compositor_task_ran_ = true;
  }

  bool SimulatedCompositorTaskPending() const {
    return !simulate_compositor_task_ran_;
  }

  void SimulateTimerTask(base::TimeDelta duration) {
    clock_.Advance(duration);
    simulate_timer_task_ran_ = true;
  }

  void EnableIdleTasks() { DoMainFrame(); }

  UseCase CurrentUseCase() {
    return scheduler_->main_thread_only().current_use_case;
  }

  UseCase ForceUpdatePolicyAndGetCurrentUseCase() {
    scheduler_->ForceUpdatePolicy();
    return scheduler_->main_thread_only().current_use_case;
  }

  v8::RAILMode GetRAILMode() {
    return scheduler_->main_thread_only().current_policy.rail_mode();
  }

  bool BeginFrameNotExpectedSoon() {
    return scheduler_->main_thread_only().begin_frame_not_expected_soon;
  }

  bool TouchStartExpectedSoon() {
    return scheduler_->main_thread_only().touchstart_expected_soon;
  }

  bool HaveSeenABeginMainframe() {
    return scheduler_->main_thread_only().have_seen_a_begin_main_frame;
  }

  bool LoadingTasksSeemExpensive() {
    return scheduler_->main_thread_only().loading_tasks_seem_expensive;
  }

  bool TimerTasksSeemExpensive() {
    return scheduler_->main_thread_only().timer_tasks_seem_expensive;
  }

  base::TimeTicks EstimatedNextFrameBegin() {
    return scheduler_->main_thread_only().estimated_next_frame_begin;
  }

  void AdvanceTimeWithTask(double duration) {
    base::TimeTicks start = clock_.NowTicks();
    scheduler_->OnTaskStarted(fake_queue_.get(), fake_task_, start);
    clock_.Advance(base::TimeDelta::FromSecondsD(duration));
    base::TimeTicks end = clock_.NowTicks();
    scheduler_->OnTaskCompleted(fake_queue_.get(), fake_task_, start, end,
                                base::nullopt);
  }

  void RunSlowCompositorTask() {
    // Run a long compositor task so that compositor tasks appear to be running
    // slow and thus compositor tasks will not be prioritized.
    compositor_task_runner_->PostTask(
        FROM_HERE,
        base::BindOnce(
            &MainThreadSchedulerImplTest::SimulateMainThreadCompositorTask,
            base::Unretained(this), base::TimeDelta::FromMilliseconds(1000)));
    RunUntilIdle();
  }

  // Helper for posting several tasks of specific types. |task_descriptor| is a
  // string with space delimited task identifiers. The first letter of each
  // task identifier specifies the task type:
  // - 'D': Default task
  // - 'C': Compositor task
  // - 'P': Input task
  // - 'L': Loading task
  // - 'M': Loading Control task
  // - 'I': Idle task
  // - 'T': Timer task
  // - 'V': kV8 task
  void PostTestTasks(std::vector<std::string>* run_order,
                     const std::string& task_descriptor) {
    std::istringstream stream(task_descriptor);
    while (!stream.eof()) {
      std::string task;
      stream >> task;
      switch (task[0]) {
        case 'D':
          default_task_runner_->PostTask(
              FROM_HERE,
              base::BindOnce(&AppendToVectorTestTask, run_order, task));
          break;
        case 'C':
          compositor_task_runner_->PostTask(
              FROM_HERE,
              base::BindOnce(&AppendToVectorTestTask, run_order, task));
          break;
        case 'P':
          input_task_runner_->PostTask(
              FROM_HERE,
              base::BindOnce(&AppendToVectorTestTask, run_order, task));
          break;
        case 'L':
          loading_task_runner_->PostTask(
              FROM_HERE,
              base::BindOnce(&AppendToVectorTestTask, run_order, task));
          break;
        case 'M':
          loading_control_task_runner_->PostTask(
              FROM_HERE,
              base::BindOnce(&AppendToVectorTestTask, run_order, task));
          break;
        case 'I':
          idle_task_runner_->PostIdleTask(
              FROM_HERE,
              base::BindOnce(&AppendToVectorIdleTestTask, run_order, task));
          break;
        case 'T':
          timer_task_runner_->PostTask(
              FROM_HERE,
              base::BindOnce(&AppendToVectorTestTask, run_order, task));
          break;
        case 'V':
          v8_task_runner_->PostTask(
              FROM_HERE,
              base::BindOnce(&AppendToVectorTestTask, run_order, task));
          break;
        default:
          NOTREACHED();
      }
    }
  }

 protected:
  static base::TimeDelta priority_escalation_after_input_duration() {
    return base::TimeDelta::FromMilliseconds(
        UserModel::kGestureEstimationLimitMillis);
  }

  static base::TimeDelta subsequent_input_expected_after_input_duration() {
    return base::TimeDelta::FromMilliseconds(
        UserModel::kExpectSubsequentGestureMillis);
  }

  static base::TimeDelta maximum_idle_period_duration() {
    return base::TimeDelta::FromMilliseconds(
        IdleHelper::kMaximumIdlePeriodMillis);
  }

  static base::TimeDelta end_idle_when_hidden_delay() {
    return base::TimeDelta::FromMilliseconds(
        MainThreadSchedulerImpl::kEndIdleWhenHiddenDelayMillis);
  }

  static base::TimeDelta delay_for_background_tab_freezing() {
    return base::TimeDelta::FromMilliseconds(
        MainThreadSchedulerImpl::kDelayForBackgroundTabFreezingMillis);
  }

  static base::TimeDelta rails_response_time() {
    return base::TimeDelta::FromMilliseconds(
        MainThreadSchedulerImpl::kRailsResponseTimeMillis);
  }

  template <typename E>
  static void CallForEachEnumValue(E first,
                                   E last,
                                   const char* (*function)(E)) {
    for (E val = first; val < last;
         val = static_cast<E>(static_cast<int>(val) + 1)) {
      (*function)(val);
    }
  }

  static void CheckAllUseCaseToString() {
    CallForEachEnumValue<UseCase>(UseCase::kFirstUseCase, UseCase::kCount,
                                  &MainThreadSchedulerImpl::UseCaseToString);
  }

  static scoped_refptr<TaskQueue> ThrottableTaskQueue(
      FrameSchedulerImpl* scheduler) {
    return scheduler->ThrottleableTaskQueue();
  }

  base::test::ScopedFeatureList feature_list_;
  base::SimpleTestTickClock clock_;
  TaskQueue::Task fake_task_;
  scoped_refptr<MainThreadTaskQueue> fake_queue_;
  // Only one of mock_task_runner_ or message_loop_ will be set.
  scoped_refptr<cc::OrderedSimpleTaskRunner> mock_task_runner_;
  std::unique_ptr<base::MessageLoop> message_loop_;

  std::unique_ptr<MainThreadSchedulerImplForTest> scheduler_;
  scoped_refptr<base::SingleThreadTaskRunner> default_task_runner_;
  scoped_refptr<base::SingleThreadTaskRunner> compositor_task_runner_;
  scoped_refptr<base::SingleThreadTaskRunner> input_task_runner_;
  scoped_refptr<TaskQueue> loading_task_runner_;
  scoped_refptr<base::SingleThreadTaskRunner> loading_control_task_runner_;
  scoped_refptr<SingleThreadIdleTaskRunner> idle_task_runner_;
  scoped_refptr<TaskQueue> timer_task_runner_;
  scoped_refptr<base::SingleThreadTaskRunner> v8_task_runner_;
  bool simulate_timer_task_ran_;
  bool simulate_compositor_task_ran_;
  uint64_t next_begin_frame_number_ = viz::BeginFrameArgs::kStartingFrameNumber;

  DISALLOW_COPY_AND_ASSIGN(MainThreadSchedulerImplTest);
};

TEST_F(MainThreadSchedulerImplTest, TestPostDefaultTask) {
  std::vector<std::string> run_order;
  PostTestTasks(&run_order, "D1 D2 D3 D4");

  RunUntilIdle();
  EXPECT_THAT(run_order,
              testing::ElementsAre(std::string("D1"), std::string("D2"),
                                   std::string("D3"), std::string("D4")));
}

TEST_F(MainThreadSchedulerImplTest, TestPostDefaultAndCompositor) {
  std::vector<std::string> run_order;
  PostTestTasks(&run_order, "D1 C1 P1");
  RunUntilIdle();
  EXPECT_THAT(run_order, testing::Contains("D1"));
  EXPECT_THAT(run_order, testing::Contains("C1"));
  EXPECT_THAT(run_order, testing::Contains("P1"));
}

TEST_F(MainThreadSchedulerImplTest, TestRentrantTask) {
  int count = 0;
  std::vector<int> run_order;
  default_task_runner_->PostTask(
      FROM_HERE, base::BindOnce(AppendToVectorReentrantTask,
                                base::RetainedRef(default_task_runner_),
                                &run_order, &count, 5));
  RunUntilIdle();

  EXPECT_THAT(run_order, testing::ElementsAre(0, 1, 2, 3, 4));
}

TEST_F(MainThreadSchedulerImplTest, TestPostIdleTask) {
  int run_count = 0;
  base::TimeTicks expected_deadline =
      clock_.NowTicks() + base::TimeDelta::FromMilliseconds(2300);
  base::TimeTicks deadline_in_task;

  clock_.Advance(base::TimeDelta::FromMilliseconds(100));
  idle_task_runner_->PostIdleTask(
      FROM_HERE, base::BindOnce(&IdleTestTask, &run_count, &deadline_in_task));

  RunUntilIdle();
  EXPECT_EQ(0, run_count);  // Shouldn't run yet as no WillBeginFrame.

  scheduler_->WillBeginFrame(viz::BeginFrameArgs::Create(
      BEGINFRAME_FROM_HERE, 0, next_begin_frame_number_++, clock_.NowTicks(),
      base::TimeTicks(), base::TimeDelta::FromMilliseconds(1000),
      viz::BeginFrameArgs::NORMAL));
  RunUntilIdle();
  EXPECT_EQ(0, run_count);  // Shouldn't run as no DidCommitFrameToCompositor.

  clock_.Advance(base::TimeDelta::FromMilliseconds(1200));
  scheduler_->DidCommitFrameToCompositor();
  RunUntilIdle();
  EXPECT_EQ(0, run_count);  // We missed the deadline.

  scheduler_->WillBeginFrame(viz::BeginFrameArgs::Create(
      BEGINFRAME_FROM_HERE, 0, next_begin_frame_number_++, clock_.NowTicks(),
      base::TimeTicks(), base::TimeDelta::FromMilliseconds(1000),
      viz::BeginFrameArgs::NORMAL));
  clock_.Advance(base::TimeDelta::FromMilliseconds(800));
  scheduler_->DidCommitFrameToCompositor();
  RunUntilIdle();
  EXPECT_EQ(1, run_count);
  EXPECT_EQ(expected_deadline, deadline_in_task);
}

TEST_F(MainThreadSchedulerImplTest, TestRepostingIdleTask) {
  int run_count = 0;

  g_max_idle_task_reposts = 2;
  idle_task_runner_->PostIdleTask(
      FROM_HERE,
      base::BindOnce(&RepostingIdleTestTask,
                     base::RetainedRef(idle_task_runner_), &run_count));
  EnableIdleTasks();
  RunUntilIdle();
  EXPECT_EQ(1, run_count);

  // Reposted tasks shouldn't run until next idle period.
  RunUntilIdle();
  EXPECT_EQ(1, run_count);

  EnableIdleTasks();
  RunUntilIdle();
  EXPECT_EQ(2, run_count);
}

TEST_F(MainThreadSchedulerImplTest, TestIdleTaskExceedsDeadline) {
  ScopedAutoAdvanceNowEnabler enable_auto_advance_now(mock_task_runner_);
  int run_count = 0;

  // Post two UpdateClockToDeadlineIdleTestTask tasks.
  idle_task_runner_->PostIdleTask(
      FROM_HERE,
      base::BindOnce(&UpdateClockToDeadlineIdleTestTask, &clock_, &run_count));
  idle_task_runner_->PostIdleTask(
      FROM_HERE,
      base::BindOnce(&UpdateClockToDeadlineIdleTestTask, &clock_, &run_count));

  EnableIdleTasks();
  RunUntilIdle();
  // Only the first idle task should execute since it's used up the deadline.
  EXPECT_EQ(1, run_count);

  EnableIdleTasks();
  RunUntilIdle();
  // Second task should be run on the next idle period.
  EXPECT_EQ(2, run_count);
}

TEST_F(MainThreadSchedulerImplTest, TestDelayedEndIdlePeriodCanceled) {
  int run_count = 0;

  base::TimeTicks deadline_in_task;
  idle_task_runner_->PostIdleTask(
      FROM_HERE, base::BindOnce(&IdleTestTask, &run_count, &deadline_in_task));

  // Trigger the beginning of an idle period for 1000ms.
  scheduler_->WillBeginFrame(viz::BeginFrameArgs::Create(
      BEGINFRAME_FROM_HERE, 0, next_begin_frame_number_++, clock_.NowTicks(),
      base::TimeTicks(), base::TimeDelta::FromMilliseconds(1000),
      viz::BeginFrameArgs::NORMAL));
  DoMainFrame();

  // End the idle period early (after 500ms), and send a WillBeginFrame which
  // specifies that the next idle period should end 1000ms from now.
  clock_.Advance(base::TimeDelta::FromMilliseconds(500));
  scheduler_->WillBeginFrame(viz::BeginFrameArgs::Create(
      BEGINFRAME_FROM_HERE, 0, next_begin_frame_number_++, clock_.NowTicks(),
      base::TimeTicks(), base::TimeDelta::FromMilliseconds(1000),
      viz::BeginFrameArgs::NORMAL));

  RunUntilIdle();
  EXPECT_EQ(0, run_count);  // Not currently in an idle period.

  // Trigger the start of the idle period before the task to end the previous
  // idle period has been triggered.
  clock_.Advance(base::TimeDelta::FromMilliseconds(400));
  scheduler_->DidCommitFrameToCompositor();

  // Post a task which simulates running until after the previous end idle
  // period delayed task was scheduled for
  scheduler_->DefaultTaskQueue()->PostTask(FROM_HERE, base::BindOnce(NullTask));
  clock_.Advance(base::TimeDelta::FromMilliseconds(300));

  RunUntilIdle();
  EXPECT_EQ(1, run_count);  // We should still be in the new idle period.
}

TEST_F(MainThreadSchedulerImplTest, TestDefaultPolicy) {
  std::vector<std::string> run_order;
  PostTestTasks(&run_order, "L1 I1 D1 P1 C1 D2 P2 C2");

  EnableIdleTasks();
  RunUntilIdle();
  // High-priority input is enabled and input tasks are processed first.
  EXPECT_THAT(run_order,
              testing::ElementsAre(std::string("P1"), std::string("P2"),
                                   std::string("L1"), std::string("D1"),
                                   std::string("C1"), std::string("D2"),
                                   std::string("C2"), std::string("I1")));
  EXPECT_EQ(UseCase::kNone, CurrentUseCase());
}

TEST_F(MainThreadSchedulerImplTest, TestDefaultPolicyWithSlowCompositor) {
  RunSlowCompositorTask();

  std::vector<std::string> run_order;
  PostTestTasks(&run_order, "L1 I1 D1 C1 P1 D2 C2");

  EnableIdleTasks();
  RunUntilIdle();
  // Even with slow compositor input tasks are handled first.
  EXPECT_THAT(run_order,
              testing::ElementsAre(std::string("P1"), std::string("L1"),
                                   std::string("D1"), std::string("C1"),
                                   std::string("D2"), std::string("C2"),
                                   std::string("I1")));
  EXPECT_EQ(UseCase::kNone, CurrentUseCase());
}

TEST_F(MainThreadSchedulerImplTest,
       TestCompositorPolicy_CompositorHandlesInput_WithTouchHandler) {
  std::vector<std::string> run_order;
  PostTestTasks(&run_order, "L1 I1 D1 C1 D2 C2");

  scheduler_->SetHasVisibleRenderWidgetWithTouchHandler(true);
  EnableIdleTasks();
  SimulateCompositorGestureStart(TouchEventPolicy::kSendTouchStart);
  RunUntilIdle();
  EXPECT_THAT(run_order,
              testing::ElementsAre(std::string("L1"), std::string("D1"),
                                   std::string("D2"), std::string("C1"),
                                   std::string("C2"), std::string("I1")));
  EXPECT_EQ(UseCase::kCompositorGesture, CurrentUseCase());
}

TEST_F(MainThreadSchedulerImplTest,
       TestCompositorPolicy_MainThreadHandlesInput_WithoutScrollUpdates) {
  std::vector<std::string> run_order;
  PostTestTasks(&run_order, "L1 I1 D1 C1 D2 C2");

  scheduler_->SetHasVisibleRenderWidgetWithTouchHandler(true);
  EnableIdleTasks();
  SimulateMainThreadGestureWithoutScrollUpdates();
  RunUntilIdle();
  EXPECT_THAT(run_order,
              testing::ElementsAre(std::string("C1"), std::string("C2"),
                                   std::string("L1"), std::string("D1"),
                                   std::string("D2"), std::string("I1")));
  EXPECT_EQ(UseCase::kMainThreadCustomInputHandling, CurrentUseCase());
}

TEST_F(MainThreadSchedulerImplTest,
       TestCompositorPolicy_MainThreadHandlesInput_WithoutPreventDefault) {
  std::vector<std::string> run_order;
  PostTestTasks(&run_order, "L1 I1 D1 C1 D2 C2");

  scheduler_->SetHasVisibleRenderWidgetWithTouchHandler(true);
  EnableIdleTasks();
  SimulateMainThreadGestureWithoutPreventDefault();
  RunUntilIdle();
  EXPECT_THAT(run_order,
              testing::ElementsAre(std::string("L1"), std::string("D1"),
                                   std::string("D2"), std::string("C1"),
                                   std::string("C2"), std::string("I1")));
  EXPECT_EQ(UseCase::kCompositorGesture, CurrentUseCase());
}

TEST_F(MainThreadSchedulerImplTest,
       TestCompositorPolicy_CompositorHandlesInput_LongGestureDuration) {
  scheduler_->SetHasVisibleRenderWidgetWithTouchHandler(true);
  EnableIdleTasks();
  SimulateCompositorGestureStart(TouchEventPolicy::kSendTouchStart);

  base::TimeTicks loop_end_time =
      clock_.NowTicks() + base::TimeDelta::FromMilliseconds(
                              UserModel::kMedianGestureDurationMillis * 2);

  // The UseCase::kCompositorGesture usecase initially deprioritizes
  // compositor tasks (see
  // TestCompositorPolicy_CompositorHandlesInput_WithTouchHandler) but if the
  // gesture is long enough, compositor tasks get prioritized again.
  while (clock_.NowTicks() < loop_end_time) {
    scheduler_->DidHandleInputEventOnCompositorThread(
        FakeInputEvent(blink::WebInputEvent::kTouchMove),
        InputEventState::EVENT_CONSUMED_BY_COMPOSITOR);
    clock_.Advance(base::TimeDelta::FromMilliseconds(16));
    RunUntilIdle();
  }

  std::vector<std::string> run_order;
  PostTestTasks(&run_order, "L1 I1 D1 C1 D2 C2");

  RunUntilIdle();
  EXPECT_THAT(run_order,
              testing::ElementsAre(std::string("C1"), std::string("C2"),
                                   std::string("L1"), std::string("D1"),
                                   std::string("D2")));
  EXPECT_EQ(UseCase::kCompositorGesture, CurrentUseCase());
}

TEST_F(MainThreadSchedulerImplTest,
       TestCompositorPolicy_CompositorHandlesInput_WithoutTouchHandler) {
  std::vector<std::string> run_order;
  PostTestTasks(&run_order, "L1 I1 D1 C1 D2 C2");

  EnableIdleTasks();
  SimulateCompositorGestureStart(TouchEventPolicy::kDontSendTouchStart);
  RunUntilIdle();
  EXPECT_THAT(run_order,
              testing::ElementsAre(std::string("L1"), std::string("D1"),
                                   std::string("D2"), std::string("C1"),
                                   std::string("C2"), std::string("I1")));
  EXPECT_EQ(UseCase::kCompositorGesture, CurrentUseCase());
}

TEST_F(MainThreadSchedulerImplTest,
       TestCompositorPolicy_MainThreadHandlesInput_WithTouchHandler) {
  std::vector<std::string> run_order;
  PostTestTasks(&run_order, "L1 I1 D1 C1 D2 C2");

  scheduler_->SetHasVisibleRenderWidgetWithTouchHandler(true);
  EnableIdleTasks();
  SimulateMainThreadGestureStart(TouchEventPolicy::kSendTouchStart,
                                 blink::WebInputEvent::kGestureScrollBegin);
  RunUntilIdle();
  EXPECT_THAT(run_order,
              testing::ElementsAre(std::string("C1"), std::string("C2"),
                                   std::string("L1"), std::string("D1"),
                                   std::string("D2"), std::string("I1")));
  EXPECT_EQ(UseCase::kMainThreadCustomInputHandling, CurrentUseCase());
  scheduler_->DidHandleInputEventOnMainThread(
      FakeInputEvent(blink::WebInputEvent::kGestureFlingStart),
      WebInputEventResult::kHandledSystem);
}

TEST_F(MainThreadSchedulerImplTest,
       TestCompositorPolicy_MainThreadHandlesInput_WithoutTouchHandler) {
  std::vector<std::string> run_order;
  PostTestTasks(&run_order, "L1 I1 D1 C1 D2 C2");

  EnableIdleTasks();
  SimulateMainThreadGestureStart(TouchEventPolicy::kDontSendTouchStart,
                                 blink::WebInputEvent::kGestureScrollBegin);
  RunUntilIdle();
  EXPECT_THAT(run_order,
              testing::ElementsAre(std::string("C1"), std::string("C2"),
                                   std::string("L1"), std::string("D1"),
                                   std::string("D2"), std::string("I1")));
  EXPECT_EQ(UseCase::kMainThreadCustomInputHandling, CurrentUseCase());
  scheduler_->DidHandleInputEventOnMainThread(
      FakeInputEvent(blink::WebInputEvent::kGestureFlingStart),
      WebInputEventResult::kHandledSystem);
}

TEST_F(MainThreadSchedulerImplTest,
       TestCompositorPolicy_MainThreadHandlesInput_SingleEvent_PreventDefault) {
  std::vector<std::string> run_order;
  PostTestTasks(&run_order, "L1 I1 D1 C1 D2 C2");

  scheduler_->SetHasVisibleRenderWidgetWithTouchHandler(true);
  EnableIdleTasks();
  scheduler_->DidHandleInputEventOnCompositorThread(
      FakeInputEvent(blink::WebInputEvent::kTouchStart),
      InputEventState::EVENT_FORWARDED_TO_MAIN_THREAD);
  scheduler_->DidHandleInputEventOnMainThread(
      FakeInputEvent(blink::WebInputEvent::kTouchStart),
      WebInputEventResult::kHandledApplication);
  RunUntilIdle();
  // Because the main thread is performing custom input handling, we let all
  // tasks run. However compositing tasks are still given priority.
  EXPECT_THAT(run_order,
              testing::ElementsAre(std::string("C1"), std::string("C2"),
                                   std::string("L1"), std::string("D1"),
                                   std::string("D2"), std::string("I1")));
  EXPECT_EQ(UseCase::kMainThreadCustomInputHandling, CurrentUseCase());
}

TEST_F(
    MainThreadSchedulerImplTest,
    TestCompositorPolicy_MainThreadHandlesInput_SingleEvent_NoPreventDefault) {
  std::vector<std::string> run_order;
  PostTestTasks(&run_order, "L1 I1 D1 C1 D2 C2");

  scheduler_->SetHasVisibleRenderWidgetWithTouchHandler(true);
  EnableIdleTasks();
  scheduler_->DidHandleInputEventOnCompositorThread(
      FakeInputEvent(blink::WebInputEvent::kTouchStart),
      InputEventState::EVENT_FORWARDED_TO_MAIN_THREAD);
  scheduler_->DidHandleInputEventOnMainThread(
      FakeInputEvent(blink::WebInputEvent::kTouchStart),
      WebInputEventResult::kHandledSystem);
  RunUntilIdle();
  // Because we are still waiting for the touchstart to be processed,
  // non-essential tasks like loading tasks are blocked.
  EXPECT_THAT(run_order,
              testing::ElementsAre(std::string("C1"), std::string("C2"),
                                   std::string("D1"), std::string("D2"),
                                   std::string("I1")));
  EXPECT_EQ(UseCase::kTouchstart, CurrentUseCase());
}

TEST_F(MainThreadSchedulerImplTest, TestCompositorPolicy_DidAnimateForInput) {
  std::vector<std::string> run_order;
  PostTestTasks(&run_order, "I1 D1 C1 D2 C2");

  scheduler_->SetHasVisibleRenderWidgetWithTouchHandler(true);
  scheduler_->DidAnimateForInputOnCompositorThread();
  // Note DidAnimateForInputOnCompositorThread does not by itself trigger a
  // policy update.
  EXPECT_EQ(UseCase::kCompositorGesture,
            ForceUpdatePolicyAndGetCurrentUseCase());
  EnableIdleTasks();
  RunUntilIdle();
  EXPECT_THAT(run_order,
              testing::ElementsAre(std::string("D1"), std::string("D2"),
                                   std::string("C1"), std::string("C2"),
                                   std::string("I1")));
  EXPECT_EQ(UseCase::kCompositorGesture, CurrentUseCase());
}

TEST_F(MainThreadSchedulerImplTest, Navigation_ResetsTaskCostEstimations) {
  std::vector<std::string> run_order;

  scheduler_->SetHasVisibleRenderWidgetWithTouchHandler(true);
  SimulateExpensiveTasks(timer_task_runner_);
  DoMainFrame();
  // A navigation occurs which creates a new Document thus resetting the task
  // cost estimations.
  scheduler_->DidStartProvisionalLoad(true);
  SimulateMainThreadGestureStart(TouchEventPolicy::kSendTouchStart,
                                 blink::WebInputEvent::kGestureScrollUpdate);

  PostTestTasks(&run_order, "C1 T1");

  RunUntilIdle();

  EXPECT_THAT(run_order,
              testing::ElementsAre(std::string("C1"), std::string("T1")));
}

TEST_F(MainThreadSchedulerImplTest,
       ExpensiveTimersDontRunWhenMainThreadScrolling) {
  std::vector<std::string> run_order;

  scheduler_->SetHasVisibleRenderWidgetWithTouchHandler(true);
  SimulateExpensiveTasks(timer_task_runner_);
  DoMainFrame();
  SimulateMainThreadGestureStart(TouchEventPolicy::kSendTouchStart,
                                 blink::WebInputEvent::kGestureScrollUpdate);

  PostTestTasks(&run_order, "C1 T1");

  RunUntilIdle();
  EXPECT_FALSE(TouchStartExpectedSoon());
  EXPECT_EQ(UseCase::kMainThreadGesture, CurrentUseCase());

  EXPECT_THAT(run_order, testing::ElementsAre(std::string("C1")));
}

TEST_F(MainThreadSchedulerImplTest,
       ExpensiveTimersDoRunWhenMainThreadInputHandling) {
  std::vector<std::string> run_order;

  scheduler_->SetHasVisibleRenderWidgetWithTouchHandler(true);
  SimulateExpensiveTasks(timer_task_runner_);
  DoMainFrame();
  SimulateMainThreadGestureStart(TouchEventPolicy::kSendTouchStart,
                                 blink::WebInputEvent::kUndefined);

  PostTestTasks(&run_order, "C1 T1");

  RunUntilIdle();
  EXPECT_FALSE(TouchStartExpectedSoon());
  EXPECT_EQ(UseCase::kMainThreadCustomInputHandling, CurrentUseCase());

  EXPECT_THAT(run_order,
              testing::ElementsAre(std::string("C1"), std::string("T1")));
}

TEST_F(MainThreadSchedulerImplTest,
       ExpensiveTimersDoRunWhenMainThreadScrolling_AndOnCriticalPath) {
  std::vector<std::string> run_order;

  scheduler_->SetHasVisibleRenderWidgetWithTouchHandler(true);
  SimulateExpensiveTasks(timer_task_runner_);
  DoMainFrameOnCriticalPath();
  SimulateMainThreadGestureStart(TouchEventPolicy::kSendTouchStart,
                                 blink::WebInputEvent::kGestureScrollBegin);

  PostTestTasks(&run_order, "C1 T1");

  RunUntilIdle();
  EXPECT_FALSE(TouchStartExpectedSoon());
  EXPECT_EQ(UseCase::kMainThreadCustomInputHandling, CurrentUseCase());

  EXPECT_THAT(run_order,
              testing::ElementsAre(std::string("C1"), std::string("T1")));
}

TEST_F(MainThreadSchedulerImplTest, TestTouchstartPolicy_Compositor) {
  std::vector<std::string> run_order;
  PostTestTasks(&run_order, "L1 D1 C1 D2 C2 T1 T2");

  // Observation of touchstart should defer execution of timer, idle and loading
  // tasks.
  scheduler_->DidHandleInputEventOnCompositorThread(
      FakeInputEvent(blink::WebInputEvent::kTouchStart),
      InputEventState::EVENT_CONSUMED_BY_COMPOSITOR);
  EnableIdleTasks();
  RunUntilIdle();
  EXPECT_THAT(run_order,
              testing::ElementsAre(std::string("C1"), std::string("C2"),
                                   std::string("D1"), std::string("D2")));

  // Animation or meta events like TapDown/FlingCancel shouldn't affect the
  // priority.
  run_order.clear();
  scheduler_->DidAnimateForInputOnCompositorThread();
  scheduler_->DidHandleInputEventOnCompositorThread(
      FakeInputEvent(blink::WebInputEvent::kGestureFlingCancel),
      InputEventState::EVENT_CONSUMED_BY_COMPOSITOR);
  scheduler_->DidHandleInputEventOnCompositorThread(
      FakeInputEvent(blink::WebInputEvent::kGestureTapDown),
      InputEventState::EVENT_CONSUMED_BY_COMPOSITOR);
  RunUntilIdle();
  EXPECT_THAT(run_order, testing::ElementsAre());

  // Action events like ScrollBegin will kick us back into compositor priority,
  // allowing service of the timer, loading and idle queues.
  run_order.clear();
  scheduler_->DidHandleInputEventOnCompositorThread(
      FakeInputEvent(blink::WebInputEvent::kGestureScrollBegin),
      InputEventState::EVENT_CONSUMED_BY_COMPOSITOR);
  RunUntilIdle();

  EXPECT_THAT(run_order,
              testing::ElementsAre(std::string("L1"), std::string("T1"),
                                   std::string("T2")));
}

TEST_F(MainThreadSchedulerImplTest, TestTouchstartPolicy_MainThread) {
  std::vector<std::string> run_order;
  PostTestTasks(&run_order, "L1 D1 C1 D2 C2 T1 T2");

  // Observation of touchstart should defer execution of timer, idle and loading
  // tasks.
  scheduler_->DidHandleInputEventOnCompositorThread(
      FakeInputEvent(blink::WebInputEvent::kTouchStart),
      InputEventState::EVENT_FORWARDED_TO_MAIN_THREAD);
  scheduler_->DidHandleInputEventOnMainThread(
      FakeInputEvent(blink::WebInputEvent::kTouchStart),
      WebInputEventResult::kHandledSystem);
  EnableIdleTasks();
  RunUntilIdle();
  EXPECT_THAT(run_order,
              testing::ElementsAre(std::string("C1"), std::string("C2"),
                                   std::string("D1"), std::string("D2")));

  // Meta events like TapDown/FlingCancel shouldn't affect the priority.
  run_order.clear();
  scheduler_->DidHandleInputEventOnCompositorThread(
      FakeInputEvent(blink::WebInputEvent::kGestureFlingCancel),
      InputEventState::EVENT_FORWARDED_TO_MAIN_THREAD);
  scheduler_->DidHandleInputEventOnMainThread(
      FakeInputEvent(blink::WebInputEvent::kGestureFlingCancel),
      WebInputEventResult::kHandledSystem);
  scheduler_->DidHandleInputEventOnCompositorThread(
      FakeInputEvent(blink::WebInputEvent::kGestureTapDown),
      InputEventState::EVENT_FORWARDED_TO_MAIN_THREAD);
  scheduler_->DidHandleInputEventOnMainThread(
      FakeInputEvent(blink::WebInputEvent::kGestureTapDown),
      WebInputEventResult::kHandledSystem);
  RunUntilIdle();
  EXPECT_THAT(run_order, testing::ElementsAre());

  // Action events like ScrollBegin will kick us back into compositor priority,
  // allowing service of the timer, loading and idle queues.
  run_order.clear();
  scheduler_->DidHandleInputEventOnCompositorThread(
      FakeInputEvent(blink::WebInputEvent::kGestureScrollBegin),
      InputEventState::EVENT_FORWARDED_TO_MAIN_THREAD);
  scheduler_->DidHandleInputEventOnMainThread(
      FakeInputEvent(blink::WebInputEvent::kGestureScrollBegin),
      WebInputEventResult::kHandledSystem);
  RunUntilIdle();

  EXPECT_THAT(run_order,
              testing::ElementsAre(std::string("L1"), std::string("T1"),
                                   std::string("T2")));
}

// TODO(alexclarke): Reenable once we've reinstaed the Loading
// UseCase.
TEST_F(MainThreadSchedulerImplTest, DISABLED_LoadingUseCase) {
  std::vector<std::string> run_order;
  PostTestTasks(&run_order, "I1 D1 C1 T1 L1 D2 C2 T2 L2");

  scheduler_->DidStartProvisionalLoad(true);
  EnableIdleTasks();
  RunUntilIdle();

  // In loading policy, loading tasks are prioritized other others.
  std::string loading_policy_expected[] = {
      std::string("D1"), std::string("L1"), std::string("D2"),
      std::string("L2"), std::string("C1"), std::string("T1"),
      std::string("C2"), std::string("T2"), std::string("I1")};
  EXPECT_THAT(run_order, testing::ElementsAreArray(loading_policy_expected));
  EXPECT_EQ(UseCase::kLoading, CurrentUseCase());

  // Advance 15s and try again, the loading policy should have ended and the
  // task order should return to the NONE use case where loading tasks are no
  // longer prioritized.
  clock_.Advance(base::TimeDelta::FromMilliseconds(150000));
  run_order.clear();
  PostTestTasks(&run_order, "I1 D1 C1 T1 L1 D2 C2 T2 L2");
  EnableIdleTasks();
  RunUntilIdle();

  std::string default_order_expected[] = {
      std::string("D1"), std::string("C1"), std::string("T1"),
      std::string("L1"), std::string("D2"), std::string("C2"),
      std::string("T2"), std::string("L2"), std::string("I1")};
  EXPECT_THAT(run_order, testing::ElementsAreArray(default_order_expected));
  EXPECT_EQ(UseCase::kNone, CurrentUseCase());
}

TEST_F(MainThreadSchedulerImplTest,
       EventConsumedOnCompositorThread_IgnoresMouseMove_WhenMouseUp) {
  RunSlowCompositorTask();

  std::vector<std::string> run_order;
  PostTestTasks(&run_order, "I1 D1 C1 D2 C2");

  EnableIdleTasks();
  scheduler_->DidHandleInputEventOnCompositorThread(
      FakeInputEvent(blink::WebInputEvent::kMouseMove),
      InputEventState::EVENT_CONSUMED_BY_COMPOSITOR);
  RunUntilIdle();
  // Note compositor tasks are not prioritized.
  EXPECT_EQ(UseCase::kNone, CurrentUseCase());
  EXPECT_THAT(run_order,
              testing::ElementsAre(std::string("D1"), std::string("C1"),
                                   std::string("D2"), std::string("C2"),
                                   std::string("I1")));
}

TEST_F(MainThreadSchedulerImplTest,
       EventForwardedToMainThread_IgnoresMouseMove_WhenMouseUp) {
  RunSlowCompositorTask();

  std::vector<std::string> run_order;
  PostTestTasks(&run_order, "I1 D1 C1 D2 C2");

  EnableIdleTasks();
  scheduler_->DidHandleInputEventOnCompositorThread(
      FakeInputEvent(blink::WebInputEvent::kMouseMove),
      InputEventState::EVENT_FORWARDED_TO_MAIN_THREAD);
  RunUntilIdle();
  // Note compositor tasks are not prioritized.
  EXPECT_EQ(UseCase::kNone, CurrentUseCase());
  EXPECT_THAT(run_order,
              testing::ElementsAre(std::string("D1"), std::string("C1"),
                                   std::string("D2"), std::string("C2"),
                                   std::string("I1")));
}

TEST_F(MainThreadSchedulerImplTest,
       EventConsumedOnCompositorThread_MouseMove_WhenMouseDown) {
  std::vector<std::string> run_order;
  PostTestTasks(&run_order, "I1 D1 C1 D2 C2");

  // Note that currently the compositor will never consume mouse move events,
  // but this test reflects what should happen if that was the case.
  EnableIdleTasks();
  scheduler_->DidHandleInputEventOnCompositorThread(
      FakeInputEvent(blink::WebInputEvent::kMouseMove,
                     blink::WebInputEvent::kLeftButtonDown),
      InputEventState::EVENT_CONSUMED_BY_COMPOSITOR);
  RunUntilIdle();
  // Note compositor tasks deprioritized.
  EXPECT_EQ(UseCase::kCompositorGesture, CurrentUseCase());
  EXPECT_THAT(run_order,
              testing::ElementsAre(std::string("D1"), std::string("D2"),
                                   std::string("C1"), std::string("C2"),
                                   std::string("I1")));
}

TEST_F(MainThreadSchedulerImplTest,
       EventForwardedToMainThread_MouseMove_WhenMouseDown) {
  std::vector<std::string> run_order;
  PostTestTasks(&run_order, "I1 D1 C1 D2 C2");

  EnableIdleTasks();
  scheduler_->DidHandleInputEventOnCompositorThread(
      FakeInputEvent(blink::WebInputEvent::kMouseMove,
                     blink::WebInputEvent::kLeftButtonDown),
      InputEventState::EVENT_FORWARDED_TO_MAIN_THREAD);
  RunUntilIdle();
  // Note compositor tasks are prioritized.
  EXPECT_THAT(run_order,
              testing::ElementsAre(std::string("C1"), std::string("C2"),
                                   std::string("D1"), std::string("D2"),
                                   std::string("I1")));
  scheduler_->DidHandleInputEventOnMainThread(
      FakeInputEvent(blink::WebInputEvent::kMouseMove,
                     blink::WebInputEvent::kLeftButtonDown),
      WebInputEventResult::kHandledSystem);
}

TEST_F(MainThreadSchedulerImplTest,
       EventForwardedToMainThread_MouseMove_WhenMouseDown_AfterMouseWheel) {
  // Simulate a main thread driven mouse wheel scroll gesture.
  SimulateMainThreadGestureStart(TouchEventPolicy::kSendTouchStart,
                                 blink::WebInputEvent::kGestureScrollUpdate);
  RunUntilIdle();
  EXPECT_FALSE(TouchStartExpectedSoon());
  EXPECT_EQ(UseCase::kMainThreadGesture, CurrentUseCase());

  // Now start a main thread mouse touch gesture. It should be detected as main
  // thread custom input handling.
  std::vector<std::string> run_order;
  PostTestTasks(&run_order, "I1 D1 C1 D2 C2");
  EnableIdleTasks();

  scheduler_->DidHandleInputEventOnCompositorThread(
      FakeInputEvent(blink::WebInputEvent::kMouseDown,
                     blink::WebInputEvent::kLeftButtonDown),
      InputEventState::EVENT_FORWARDED_TO_MAIN_THREAD);
  scheduler_->DidHandleInputEventOnCompositorThread(
      FakeInputEvent(blink::WebInputEvent::kMouseMove,
                     blink::WebInputEvent::kLeftButtonDown),
      InputEventState::EVENT_FORWARDED_TO_MAIN_THREAD);
  RunUntilIdle();

  EXPECT_EQ(UseCase::kMainThreadCustomInputHandling, CurrentUseCase());

  // Note compositor tasks are prioritized.
  EXPECT_THAT(run_order,
              testing::ElementsAre(std::string("C1"), std::string("C2"),
                                   std::string("D1"), std::string("D2"),
                                   std::string("I1")));
}

TEST_F(MainThreadSchedulerImplTest, EventForwardedToMainThread_MouseClick) {
  // A mouse click should be detected as main thread input handling, which means
  // we won't try to defer expensive tasks because of one. We can, however,
  // prioritize compositing/input handling.
  std::vector<std::string> run_order;
  PostTestTasks(&run_order, "I1 D1 C1 D2 C2");
  EnableIdleTasks();

  scheduler_->DidHandleInputEventOnCompositorThread(
      FakeInputEvent(blink::WebInputEvent::kMouseDown,
                     blink::WebInputEvent::kLeftButtonDown),
      InputEventState::EVENT_FORWARDED_TO_MAIN_THREAD);
  scheduler_->DidHandleInputEventOnCompositorThread(
      FakeInputEvent(blink::WebInputEvent::kMouseUp,
                     blink::WebInputEvent::kLeftButtonDown),
      InputEventState::EVENT_FORWARDED_TO_MAIN_THREAD);
  RunUntilIdle();

  EXPECT_EQ(UseCase::kMainThreadCustomInputHandling, CurrentUseCase());

  // Note compositor tasks are prioritized.
  EXPECT_THAT(run_order,
              testing::ElementsAre(std::string("C1"), std::string("C2"),
                                   std::string("D1"), std::string("D2"),
                                   std::string("I1")));
}

TEST_F(MainThreadSchedulerImplTest,
       EventConsumedOnCompositorThread_MouseWheel) {
  std::vector<std::string> run_order;
  PostTestTasks(&run_order, "I1 D1 C1 D2 C2");

  EnableIdleTasks();
  scheduler_->DidHandleInputEventOnCompositorThread(
      FakeInputEvent(blink::WebInputEvent::kMouseWheel),
      InputEventState::EVENT_CONSUMED_BY_COMPOSITOR);
  RunUntilIdle();
  // Note compositor tasks are not prioritized.
  EXPECT_THAT(run_order,
              testing::ElementsAre(std::string("D1"), std::string("D2"),
                                   std::string("C1"), std::string("C2"),
                                   std::string("I1")));
  EXPECT_EQ(UseCase::kCompositorGesture, CurrentUseCase());
}

TEST_F(MainThreadSchedulerImplTest,
       EventForwardedToMainThread_MouseWheel_PreventDefault) {
  std::vector<std::string> run_order;
  PostTestTasks(&run_order, "I1 D1 C1 D2 C2");

  EnableIdleTasks();
  scheduler_->DidHandleInputEventOnCompositorThread(
      FakeInputEvent(blink::WebInputEvent::kMouseWheel),
      InputEventState::EVENT_FORWARDED_TO_MAIN_THREAD);
  RunUntilIdle();
  // Note compositor tasks are prioritized (since they are fast).
  EXPECT_THAT(run_order,
              testing::ElementsAre(std::string("C1"), std::string("C2"),
                                   std::string("D1"), std::string("D2"),
                                   std::string("I1")));
  EXPECT_EQ(UseCase::kMainThreadCustomInputHandling, CurrentUseCase());
}

TEST_F(MainThreadSchedulerImplTest,
       EventForwardedToMainThread_NoPreventDefault) {
  std::vector<std::string> run_order;
  PostTestTasks(&run_order, "I1 D1 C1 D2 C2");

  EnableIdleTasks();
  scheduler_->DidHandleInputEventOnCompositorThread(
      FakeInputEvent(blink::WebInputEvent::kMouseWheel),
      InputEventState::EVENT_FORWARDED_TO_MAIN_THREAD);
  scheduler_->DidHandleInputEventOnCompositorThread(
      FakeInputEvent(blink::WebInputEvent::kGestureScrollBegin),
      InputEventState::EVENT_FORWARDED_TO_MAIN_THREAD);
  scheduler_->DidHandleInputEventOnCompositorThread(
      FakeInputEvent(blink::WebInputEvent::kGestureScrollUpdate),
      InputEventState::EVENT_FORWARDED_TO_MAIN_THREAD);
  scheduler_->DidHandleInputEventOnCompositorThread(
      FakeInputEvent(blink::WebInputEvent::kGestureScrollUpdate),
      InputEventState::EVENT_FORWARDED_TO_MAIN_THREAD);
  RunUntilIdle();
  // Note compositor tasks are prioritized.
  EXPECT_THAT(run_order,
              testing::ElementsAre(std::string("C1"), std::string("C2"),
                                   std::string("D1"), std::string("D2"),
                                   std::string("I1")));
  EXPECT_EQ(UseCase::kMainThreadGesture, CurrentUseCase());
}

TEST_F(
    MainThreadSchedulerImplTest,
    EventForwardedToMainThreadAndBackToCompositor_MouseWheel_NoPreventDefault) {
  std::vector<std::string> run_order;
  PostTestTasks(&run_order, "I1 D1 C1 D2 C2");

  EnableIdleTasks();
  scheduler_->DidHandleInputEventOnCompositorThread(
      FakeInputEvent(blink::WebInputEvent::kMouseWheel),
      InputEventState::EVENT_FORWARDED_TO_MAIN_THREAD);
  scheduler_->DidHandleInputEventOnCompositorThread(
      FakeInputEvent(blink::WebInputEvent::kGestureScrollBegin),
      InputEventState::EVENT_CONSUMED_BY_COMPOSITOR);
  scheduler_->DidHandleInputEventOnCompositorThread(
      FakeInputEvent(blink::WebInputEvent::kGestureScrollUpdate),
      InputEventState::EVENT_CONSUMED_BY_COMPOSITOR);
  scheduler_->DidHandleInputEventOnCompositorThread(
      FakeInputEvent(blink::WebInputEvent::kGestureScrollUpdate),
      InputEventState::EVENT_CONSUMED_BY_COMPOSITOR);
  RunUntilIdle();
  // Note compositor tasks are not prioritized.
  EXPECT_THAT(run_order,
              testing::ElementsAre(std::string("D1"), std::string("D2"),
                                   std::string("C1"), std::string("C2"),
                                   std::string("I1")));
  EXPECT_EQ(UseCase::kCompositorGesture, CurrentUseCase());
}

TEST_F(MainThreadSchedulerImplTest,
       EventConsumedOnCompositorThread_IgnoresKeyboardEvents) {
  RunSlowCompositorTask();

  std::vector<std::string> run_order;
  PostTestTasks(&run_order, "I1 D1 C1 D2 C2");

  EnableIdleTasks();
  scheduler_->DidHandleInputEventOnCompositorThread(
      FakeInputEvent(blink::WebInputEvent::kKeyDown),
      InputEventState::EVENT_CONSUMED_BY_COMPOSITOR);
  RunUntilIdle();
  // Note compositor tasks are not prioritized.
  EXPECT_THAT(run_order,
              testing::ElementsAre(std::string("D1"), std::string("C1"),
                                   std::string("D2"), std::string("C2"),
                                   std::string("I1")));
  EXPECT_EQ(UseCase::kNone, CurrentUseCase());
}

TEST_F(MainThreadSchedulerImplTest,
       EventForwardedToMainThread_IgnoresKeyboardEvents) {
  RunSlowCompositorTask();

  std::vector<std::string> run_order;
  PostTestTasks(&run_order, "I1 D1 C1 D2 C2");

  EnableIdleTasks();
  scheduler_->DidHandleInputEventOnCompositorThread(
      FakeInputEvent(blink::WebInputEvent::kKeyDown),
      InputEventState::EVENT_FORWARDED_TO_MAIN_THREAD);
  RunUntilIdle();
  // Note compositor tasks are not prioritized.
  EXPECT_THAT(run_order,
              testing::ElementsAre(std::string("D1"), std::string("C1"),
                                   std::string("D2"), std::string("C2"),
                                   std::string("I1")));
  EXPECT_EQ(UseCase::kNone, CurrentUseCase());
  // Note compositor tasks are not prioritized.
  scheduler_->DidHandleInputEventOnMainThread(
      FakeInputEvent(blink::WebInputEvent::kKeyDown),
      WebInputEventResult::kHandledSystem);
}

TEST_F(MainThreadSchedulerImplTest,
       TestMainthreadScrollingUseCaseDoesNotStarveDefaultTasks) {
  SimulateMainThreadGestureStart(TouchEventPolicy::kDontSendTouchStart,
                                 blink::WebInputEvent::kGestureScrollBegin);
  EnableIdleTasks();

  std::vector<std::string> run_order;
  PostTestTasks(&run_order, "D1 C1");

  for (int i = 0; i < 20; i++) {
    compositor_task_runner_->PostTask(FROM_HERE, base::BindOnce(&NullTask));
  }
  PostTestTasks(&run_order, "C2");

  scheduler_->DidHandleInputEventOnCompositorThread(
      FakeInputEvent(blink::WebInputEvent::kGestureFlingStart),
      InputEventState::EVENT_CONSUMED_BY_COMPOSITOR);
  RunUntilIdle();
  // Ensure that the default D1 task gets to run at some point before the final
  // C2 compositor task.
  EXPECT_THAT(run_order,
              testing::ElementsAre(std::string("C1"), std::string("D1"),
                                   std::string("C2")));
}

TEST_F(MainThreadSchedulerImplTest,
       TestCompositorPolicyEnds_CompositorHandlesInput) {
  SimulateCompositorGestureStart(TouchEventPolicy::kDontSendTouchStart);
  EXPECT_EQ(UseCase::kCompositorGesture,
            ForceUpdatePolicyAndGetCurrentUseCase());

  clock_.Advance(base::TimeDelta::FromMilliseconds(1000));
  EXPECT_EQ(UseCase::kNone, ForceUpdatePolicyAndGetCurrentUseCase());
}

TEST_F(MainThreadSchedulerImplTest,
       TestCompositorPolicyEnds_MainThreadHandlesInput) {
  SimulateMainThreadGestureStart(TouchEventPolicy::kDontSendTouchStart,
                                 blink::WebInputEvent::kGestureScrollBegin);
  EXPECT_EQ(UseCase::kMainThreadCustomInputHandling,
            ForceUpdatePolicyAndGetCurrentUseCase());

  clock_.Advance(base::TimeDelta::FromMilliseconds(1000));
  EXPECT_EQ(UseCase::kNone, ForceUpdatePolicyAndGetCurrentUseCase());
}

TEST_F(MainThreadSchedulerImplTest, TestTouchstartPolicyEndsAfterTimeout) {
  std::vector<std::string> run_order;
  PostTestTasks(&run_order, "L1 D1 C1 D2 C2");

  scheduler_->DidHandleInputEventOnCompositorThread(
      FakeInputEvent(blink::WebInputEvent::kTouchStart),
      InputEventState::EVENT_CONSUMED_BY_COMPOSITOR);
  RunUntilIdle();
  EXPECT_THAT(run_order,
              testing::ElementsAre(std::string("C1"), std::string("C2"),
                                   std::string("D1"), std::string("D2")));

  run_order.clear();
  clock_.Advance(base::TimeDelta::FromMilliseconds(1000));

  // Don't post any compositor tasks to simulate a very long running event
  // handler.
  PostTestTasks(&run_order, "D1 D2");

  // Touchstart policy mode should have ended now that the clock has advanced.
  RunUntilIdle();
  EXPECT_THAT(run_order,
              testing::ElementsAre(std::string("L1"), std::string("D1"),
                                   std::string("D2")));
}

TEST_F(MainThreadSchedulerImplTest,
       TestTouchstartPolicyEndsAfterConsecutiveTouchmoves) {
  std::vector<std::string> run_order;
  PostTestTasks(&run_order, "L1 D1 C1 D2 C2");

  // Observation of touchstart should defer execution of idle and loading tasks.
  scheduler_->DidHandleInputEventOnCompositorThread(
      FakeInputEvent(blink::WebInputEvent::kTouchStart),
      InputEventState::EVENT_CONSUMED_BY_COMPOSITOR);
  RunUntilIdle();
  EXPECT_THAT(run_order,
              testing::ElementsAre(std::string("C1"), std::string("C2"),
                                   std::string("D1"), std::string("D2")));

  // Receiving the first touchmove will not affect scheduler priority.
  run_order.clear();
  scheduler_->DidHandleInputEventOnCompositorThread(
      FakeInputEvent(blink::WebInputEvent::kTouchMove),
      InputEventState::EVENT_CONSUMED_BY_COMPOSITOR);
  RunUntilIdle();
  EXPECT_THAT(run_order, testing::ElementsAre());

  // Receiving the second touchmove will kick us back into compositor priority.
  run_order.clear();
  scheduler_->DidHandleInputEventOnCompositorThread(
      FakeInputEvent(blink::WebInputEvent::kTouchMove),
      InputEventState::EVENT_CONSUMED_BY_COMPOSITOR);
  RunUntilIdle();
  EXPECT_THAT(run_order, testing::ElementsAre(std::string("L1")));
}

TEST_F(MainThreadSchedulerImplTest, TestIsHighPriorityWorkAnticipated) {
  bool is_anticipated_before = false;
  bool is_anticipated_after = false;

  scheduler_->SetHasVisibleRenderWidgetWithTouchHandler(true);
  default_task_runner_->PostTask(
      FROM_HERE, base::BindOnce(&AnticipationTestTask, scheduler_.get(),
                                SimulateInputType::kNone,
                                &is_anticipated_before, &is_anticipated_after));
  RunUntilIdle();
  // In its default state, without input receipt, the scheduler should indicate
  // that no high-priority is anticipated.
  EXPECT_FALSE(is_anticipated_before);
  EXPECT_FALSE(is_anticipated_after);

  default_task_runner_->PostTask(
      FROM_HERE, base::BindOnce(&AnticipationTestTask, scheduler_.get(),
                                SimulateInputType::kTouchStart,
                                &is_anticipated_before, &is_anticipated_after));
  bool dummy;
  default_task_runner_->PostTask(
      FROM_HERE, base::BindOnce(&AnticipationTestTask, scheduler_.get(),
                                SimulateInputType::kTouchEnd, &dummy, &dummy));
  default_task_runner_->PostTask(
      FROM_HERE,
      base::BindOnce(&AnticipationTestTask, scheduler_.get(),
                     SimulateInputType::kGestureScrollBegin, &dummy, &dummy));
  default_task_runner_->PostTask(
      FROM_HERE,
      base::BindOnce(&AnticipationTestTask, scheduler_.get(),
                     SimulateInputType::kGestureScrollEnd, &dummy, &dummy));

  RunUntilIdle();
  // When input is received, the scheduler should indicate that high-priority
  // work is anticipated.
  EXPECT_FALSE(is_anticipated_before);
  EXPECT_TRUE(is_anticipated_after);

  clock_.Advance(priority_escalation_after_input_duration() * 2);
  default_task_runner_->PostTask(
      FROM_HERE, base::BindOnce(&AnticipationTestTask, scheduler_.get(),
                                SimulateInputType::kNone,
                                &is_anticipated_before, &is_anticipated_after));
  RunUntilIdle();
  // Without additional input, the scheduler should go into NONE
  // use case but with scrolling expected where high-priority work is still
  // anticipated.
  EXPECT_EQ(UseCase::kNone, CurrentUseCase());
  EXPECT_TRUE(TouchStartExpectedSoon());
  EXPECT_TRUE(is_anticipated_before);
  EXPECT_TRUE(is_anticipated_after);

  clock_.Advance(subsequent_input_expected_after_input_duration() * 2);
  default_task_runner_->PostTask(
      FROM_HERE, base::BindOnce(&AnticipationTestTask, scheduler_.get(),
                                SimulateInputType::kNone,
                                &is_anticipated_before, &is_anticipated_after));
  RunUntilIdle();
  // Eventually the scheduler should go into the default use case where
  // high-priority work is no longer anticipated.
  EXPECT_EQ(UseCase::kNone, CurrentUseCase());
  EXPECT_FALSE(TouchStartExpectedSoon());
  EXPECT_FALSE(is_anticipated_before);
  EXPECT_FALSE(is_anticipated_after);
}

TEST_F(MainThreadSchedulerImplTest, TestShouldYield) {
  bool should_yield_before = false;
  bool should_yield_after = false;

  default_task_runner_->PostTask(
      FROM_HERE, base::BindOnce(&PostingYieldingTestTask, scheduler_.get(),
                                base::RetainedRef(default_task_runner_), false,
                                &should_yield_before, &should_yield_after));
  RunUntilIdle();
  // Posting to default runner shouldn't cause yielding.
  EXPECT_FALSE(should_yield_before);
  EXPECT_FALSE(should_yield_after);

  default_task_runner_->PostTask(
      FROM_HERE,
      base::BindOnce(&PostingYieldingTestTask, scheduler_.get(),
                     base::RetainedRef(compositor_task_runner_), false,
                     &should_yield_before, &should_yield_after));
  RunUntilIdle();
  // Posting while not mainthread scrolling shouldn't cause yielding.
  EXPECT_FALSE(should_yield_before);
  EXPECT_FALSE(should_yield_after);

  default_task_runner_->PostTask(
      FROM_HERE,
      base::BindOnce(&PostingYieldingTestTask, scheduler_.get(),
                     base::RetainedRef(compositor_task_runner_), true,
                     &should_yield_before, &should_yield_after));
  RunUntilIdle();
  // We should be able to switch to compositor priority mid-task.
  EXPECT_FALSE(should_yield_before);
  EXPECT_TRUE(should_yield_after);
}

TEST_F(MainThreadSchedulerImplTest, TestShouldYield_TouchStart) {
  // Receiving a touchstart should immediately trigger yielding, even if
  // there's no immediately pending work in the compositor queue.
  EXPECT_FALSE(scheduler_->ShouldYieldForHighPriorityWork());
  scheduler_->DidHandleInputEventOnCompositorThread(
      FakeInputEvent(blink::WebInputEvent::kTouchStart),
      InputEventState::EVENT_CONSUMED_BY_COMPOSITOR);
  EXPECT_TRUE(scheduler_->ShouldYieldForHighPriorityWork());
  RunUntilIdle();
}

TEST_F(MainThreadSchedulerImplTest, SlowMainThreadInputEvent) {
  EXPECT_EQ(UseCase::kNone, CurrentUseCase());

  // An input event should bump us into input priority.
  scheduler_->DidHandleInputEventOnCompositorThread(
      FakeInputEvent(blink::WebInputEvent::kGestureFlingStart),
      InputEventState::EVENT_FORWARDED_TO_MAIN_THREAD);
  RunUntilIdle();
  EXPECT_EQ(UseCase::kMainThreadCustomInputHandling, CurrentUseCase());

  // Simulate the input event being queued for a very long time. The compositor
  // task we post here represents the enqueued input task.
  clock_.Advance(priority_escalation_after_input_duration() * 2);
  scheduler_->DidHandleInputEventOnMainThread(
      FakeInputEvent(blink::WebInputEvent::kGestureFlingStart),
      WebInputEventResult::kHandledSystem);
  RunUntilIdle();

  // Even though we exceeded the input priority escalation period, we should
  // still be in main thread gesture since the input remains queued.
  EXPECT_EQ(UseCase::kMainThreadCustomInputHandling, CurrentUseCase());

  // After the escalation period ends we should go back into normal mode.
  clock_.Advance(priority_escalation_after_input_duration() * 2);
  RunUntilIdle();
  EXPECT_EQ(UseCase::kNone, CurrentUseCase());
}

class MainThreadSchedulerImplWithMockSchedulerTest
    : public MainThreadSchedulerImplTest {
 public:
  void SetUp() override {
    mock_task_runner_ =
        base::MakeRefCounted<cc::OrderedSimpleTaskRunner>(&clock_, false);
    mock_scheduler_ = new MainThreadSchedulerImplForTest(
        base::sequence_manager::TaskQueueManagerForTest::Create(
            nullptr, mock_task_runner_, &clock_),
        base::nullopt);
    Initialize(base::WrapUnique(mock_scheduler_));
  }

 protected:
  MainThreadSchedulerImplForTest* mock_scheduler_;
};

TEST_F(MainThreadSchedulerImplWithMockSchedulerTest,
       OnlyOnePendingUrgentPolicyUpdatey) {
  mock_scheduler_->EnsureUrgentPolicyUpdatePostedOnMainThread();
  mock_scheduler_->EnsureUrgentPolicyUpdatePostedOnMainThread();
  mock_scheduler_->EnsureUrgentPolicyUpdatePostedOnMainThread();
  mock_scheduler_->EnsureUrgentPolicyUpdatePostedOnMainThread();

  RunUntilIdle();

  EXPECT_EQ(1, mock_scheduler_->update_policy_count_);
}

TEST_F(MainThreadSchedulerImplWithMockSchedulerTest,
       OnePendingDelayedAndOneUrgentUpdatePolicy) {
  ScopedAutoAdvanceNowEnabler enable_auto_advance_now(mock_task_runner_);

  mock_scheduler_->ScheduleDelayedPolicyUpdate(
      clock_.NowTicks(), base::TimeDelta::FromMilliseconds(1));
  mock_scheduler_->EnsureUrgentPolicyUpdatePostedOnMainThread();

  RunUntilIdle();

  // We expect both the urgent and the delayed updates to run.
  EXPECT_EQ(2, mock_scheduler_->update_policy_count_);
}

TEST_F(MainThreadSchedulerImplWithMockSchedulerTest,
       OneUrgentAndOnePendingDelayedUpdatePolicy) {
  ScopedAutoAdvanceNowEnabler enable_auto_advance_now(mock_task_runner_);

  mock_scheduler_->EnsureUrgentPolicyUpdatePostedOnMainThread();
  mock_scheduler_->ScheduleDelayedPolicyUpdate(
      clock_.NowTicks(), base::TimeDelta::FromMilliseconds(1));

  RunUntilIdle();

  // We expect both the urgent and the delayed updates to run.
  EXPECT_EQ(2, mock_scheduler_->update_policy_count_);
}

TEST_F(MainThreadSchedulerImplWithMockSchedulerTest,
       UpdatePolicyCountTriggeredByOneInputEvent) {
  // We expect DidHandleInputEventOnCompositorThread to post an urgent policy
  // update.
  scheduler_->DidHandleInputEventOnCompositorThread(
      FakeInputEvent(blink::WebInputEvent::kTouchStart),
      InputEventState::EVENT_FORWARDED_TO_MAIN_THREAD);
  EXPECT_EQ(0, mock_scheduler_->update_policy_count_);
  mock_task_runner_->RunPendingTasks();
  EXPECT_EQ(1, mock_scheduler_->update_policy_count_);

  scheduler_->DidHandleInputEventOnMainThread(
      FakeInputEvent(blink::WebInputEvent::kTouchStart),
      WebInputEventResult::kHandledSystem);
  EXPECT_EQ(1, mock_scheduler_->update_policy_count_);

  clock_.Advance(base::TimeDelta::FromMilliseconds(1000));
  RunUntilIdle();

  // We finally expect a delayed policy update 100ms later.
  EXPECT_EQ(2, mock_scheduler_->update_policy_count_);
}

TEST_F(MainThreadSchedulerImplWithMockSchedulerTest,
       UpdatePolicyCountTriggeredByThreeInputEvents) {
  // We expect DidHandleInputEventOnCompositorThread to post an urgent policy
  // update.
  scheduler_->DidHandleInputEventOnCompositorThread(
      FakeInputEvent(blink::WebInputEvent::kTouchStart),
      InputEventState::EVENT_FORWARDED_TO_MAIN_THREAD);
  EXPECT_EQ(0, mock_scheduler_->update_policy_count_);
  mock_task_runner_->RunPendingTasks();
  EXPECT_EQ(1, mock_scheduler_->update_policy_count_);

  scheduler_->DidHandleInputEventOnMainThread(
      FakeInputEvent(blink::WebInputEvent::kTouchStart),
      WebInputEventResult::kHandledSystem);
  EXPECT_EQ(1, mock_scheduler_->update_policy_count_);

  // The second call to DidHandleInputEventOnCompositorThread should not post a
  // policy update because we are already in compositor priority.
  scheduler_->DidHandleInputEventOnCompositorThread(
      FakeInputEvent(blink::WebInputEvent::kTouchMove),
      InputEventState::EVENT_FORWARDED_TO_MAIN_THREAD);
  mock_task_runner_->RunPendingTasks();
  EXPECT_EQ(1, mock_scheduler_->update_policy_count_);

  // We expect DidHandleInputEvent to trigger a policy update.
  scheduler_->DidHandleInputEventOnMainThread(
      FakeInputEvent(blink::WebInputEvent::kTouchMove),
      WebInputEventResult::kHandledSystem);
  EXPECT_EQ(1, mock_scheduler_->update_policy_count_);

  // The third call to DidHandleInputEventOnCompositorThread should post a
  // policy update because the awaiting_touch_start_response_ flag changed.
  scheduler_->DidHandleInputEventOnCompositorThread(
      FakeInputEvent(blink::WebInputEvent::kTouchMove),
      InputEventState::EVENT_FORWARDED_TO_MAIN_THREAD);
  EXPECT_EQ(1, mock_scheduler_->update_policy_count_);
  mock_task_runner_->RunPendingTasks();
  EXPECT_EQ(2, mock_scheduler_->update_policy_count_);

  // We expect DidHandleInputEvent to trigger a policy update.
  scheduler_->DidHandleInputEventOnMainThread(
      FakeInputEvent(blink::WebInputEvent::kTouchMove),
      WebInputEventResult::kHandledSystem);
  EXPECT_EQ(2, mock_scheduler_->update_policy_count_);

  clock_.Advance(base::TimeDelta::FromMilliseconds(1000));
  RunUntilIdle();

  // We finally expect a delayed policy update.
  EXPECT_EQ(3, mock_scheduler_->update_policy_count_);
}

TEST_F(MainThreadSchedulerImplWithMockSchedulerTest,
       UpdatePolicyCountTriggeredByTwoInputEventsWithALongSeparatingDelay) {
  // We expect DidHandleInputEventOnCompositorThread to post an urgent policy
  // update.
  scheduler_->DidHandleInputEventOnCompositorThread(
      FakeInputEvent(blink::WebInputEvent::kTouchStart),
      InputEventState::EVENT_FORWARDED_TO_MAIN_THREAD);
  EXPECT_EQ(0, mock_scheduler_->update_policy_count_);
  mock_task_runner_->RunPendingTasks();
  EXPECT_EQ(1, mock_scheduler_->update_policy_count_);

  scheduler_->DidHandleInputEventOnMainThread(
      FakeInputEvent(blink::WebInputEvent::kTouchStart),
      WebInputEventResult::kHandledSystem);
  EXPECT_EQ(1, mock_scheduler_->update_policy_count_);

  clock_.Advance(base::TimeDelta::FromMilliseconds(1000));
  RunUntilIdle();
  // We expect a delayed policy update.
  EXPECT_EQ(2, mock_scheduler_->update_policy_count_);

  // We expect the second call to DidHandleInputEventOnCompositorThread to post
  // an urgent policy update because we are no longer in compositor priority.
  scheduler_->DidHandleInputEventOnCompositorThread(
      FakeInputEvent(blink::WebInputEvent::kTouchMove),
      InputEventState::EVENT_FORWARDED_TO_MAIN_THREAD);
  EXPECT_EQ(2, mock_scheduler_->update_policy_count_);
  mock_task_runner_->RunPendingTasks();
  EXPECT_EQ(3, mock_scheduler_->update_policy_count_);

  scheduler_->DidHandleInputEventOnMainThread(
      FakeInputEvent(blink::WebInputEvent::kTouchMove),
      WebInputEventResult::kHandledSystem);
  EXPECT_EQ(3, mock_scheduler_->update_policy_count_);

  clock_.Advance(base::TimeDelta::FromMilliseconds(1000));
  RunUntilIdle();

  // We finally expect a delayed policy update.
  EXPECT_EQ(4, mock_scheduler_->update_policy_count_);
}

TEST_F(MainThreadSchedulerImplWithMockSchedulerTest,
       EnsureUpdatePolicyNotTriggeredTooOften) {
  ScopedAutoAdvanceNowEnabler enable_auto_advance_now(mock_task_runner_);

  EXPECT_EQ(0, mock_scheduler_->update_policy_count_);
  scheduler_->SetHasVisibleRenderWidgetWithTouchHandler(true);
  EXPECT_EQ(1, mock_scheduler_->update_policy_count_);

  SimulateCompositorGestureStart(TouchEventPolicy::kSendTouchStart);

  // We expect the first call to IsHighPriorityWorkAnticipated to be called
  // after receiving an input event (but before the UpdateTask was processed) to
  // call UpdatePolicy.
  EXPECT_EQ(1, mock_scheduler_->update_policy_count_);
  scheduler_->IsHighPriorityWorkAnticipated();
  EXPECT_EQ(2, mock_scheduler_->update_policy_count_);
  // Subsequent calls should not call UpdatePolicy.
  scheduler_->IsHighPriorityWorkAnticipated();
  scheduler_->IsHighPriorityWorkAnticipated();
  scheduler_->IsHighPriorityWorkAnticipated();
  scheduler_->ShouldYieldForHighPriorityWork();
  scheduler_->ShouldYieldForHighPriorityWork();
  scheduler_->ShouldYieldForHighPriorityWork();
  scheduler_->ShouldYieldForHighPriorityWork();

  scheduler_->DidHandleInputEventOnCompositorThread(
      FakeInputEvent(blink::WebInputEvent::kGestureScrollEnd),
      InputEventState::EVENT_CONSUMED_BY_COMPOSITOR);
  scheduler_->DidHandleInputEventOnCompositorThread(
      FakeInputEvent(blink::WebInputEvent::kTouchEnd),
      InputEventState::EVENT_FORWARDED_TO_MAIN_THREAD);

  scheduler_->DidHandleInputEventOnMainThread(
      FakeInputEvent(blink::WebInputEvent::kTouchStart),
      WebInputEventResult::kHandledSystem);
  scheduler_->DidHandleInputEventOnMainThread(
      FakeInputEvent(blink::WebInputEvent::kTouchMove),
      WebInputEventResult::kHandledSystem);
  scheduler_->DidHandleInputEventOnMainThread(
      FakeInputEvent(blink::WebInputEvent::kTouchMove),
      WebInputEventResult::kHandledSystem);
  scheduler_->DidHandleInputEventOnMainThread(
      FakeInputEvent(blink::WebInputEvent::kTouchEnd),
      WebInputEventResult::kHandledSystem);

  EXPECT_EQ(2, mock_scheduler_->update_policy_count_);

  // We expect both the urgent and the delayed updates to run in addition to the
  // earlier updated cause by IsHighPriorityWorkAnticipated, a final update
  // transitions from 'not_scrolling touchstart expected' to 'not_scrolling'.
  RunUntilIdle();
  EXPECT_THAT(
      mock_scheduler_->use_cases_,
      testing::ElementsAre(
          std::string("none"), std::string("compositor_gesture"),
          std::string("compositor_gesture touchstart expected"),
          std::string("none touchstart expected"), std::string("none")));
}

class MainThreadSchedulerImplWithMessageLoopTest
    : public MainThreadSchedulerImplTest {
 public:
  MainThreadSchedulerImplWithMessageLoopTest()
      : MainThreadSchedulerImplTest(new base::MessageLoop()) {}
  ~MainThreadSchedulerImplWithMessageLoopTest() override = default;

  void PostFromNestedRunloop(
      std::vector<std::pair<SingleThreadIdleTaskRunner::IdleTask, bool>>*
          tasks) {
    for (std::pair<SingleThreadIdleTaskRunner::IdleTask, bool>& pair : *tasks) {
      if (pair.second) {
        idle_task_runner_->PostIdleTask(FROM_HERE, std::move(pair.first));
      } else {
        idle_task_runner_->PostNonNestableIdleTask(FROM_HERE,
                                                   std::move(pair.first));
      }
    }
    EnableIdleTasks();
    base::RunLoop(base::RunLoop::Type::kNestableTasksAllowed).RunUntilIdle();
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(MainThreadSchedulerImplWithMessageLoopTest);
};

TEST_F(MainThreadSchedulerImplWithMessageLoopTest,
       NonNestableIdleTaskDoesntExecuteInNestedLoop) {
  std::vector<std::string> order;
  idle_task_runner_->PostIdleTask(
      FROM_HERE,
      base::BindOnce(&AppendToVectorIdleTestTask, &order, std::string("1")));
  idle_task_runner_->PostIdleTask(
      FROM_HERE,
      base::BindOnce(&AppendToVectorIdleTestTask, &order, std::string("2")));

  std::vector<std::pair<SingleThreadIdleTaskRunner::IdleTask, bool>>
      tasks_to_post_from_nested_loop;
  tasks_to_post_from_nested_loop.push_back(std::make_pair(
      base::BindOnce(&AppendToVectorIdleTestTask, &order, std::string("3")),
      false));
  tasks_to_post_from_nested_loop.push_back(std::make_pair(
      base::BindOnce(&AppendToVectorIdleTestTask, &order, std::string("4")),
      true));
  tasks_to_post_from_nested_loop.push_back(std::make_pair(
      base::BindOnce(&AppendToVectorIdleTestTask, &order, std::string("5")),
      true));

  default_task_runner_->PostTask(
      FROM_HERE,
      base::BindOnce(
          &MainThreadSchedulerImplWithMessageLoopTest::PostFromNestedRunloop,
          base::Unretained(this),
          base::Unretained(&tasks_to_post_from_nested_loop)));

  EnableIdleTasks();
  RunUntilIdle();
  // Note we expect task 3 to run last because it's non-nestable.
  EXPECT_THAT(order, testing::ElementsAre(std::string("1"), std::string("2"),
                                          std::string("4"), std::string("5"),
                                          std::string("3")));
}

TEST_F(MainThreadSchedulerImplTest, TestBeginMainFrameNotExpectedUntil) {
  base::TimeDelta ten_millis(base::TimeDelta::FromMilliseconds(10));
  base::TimeTicks expected_deadline = clock_.NowTicks() + ten_millis;
  base::TimeTicks deadline_in_task;
  int run_count = 0;

  idle_task_runner_->PostIdleTask(
      FROM_HERE, base::BindOnce(&IdleTestTask, &run_count, &deadline_in_task));

  RunUntilIdle();
  EXPECT_EQ(0, run_count);  // Shouldn't run yet as no idle period.

  base::TimeTicks now = clock_.NowTicks();
  base::TimeTicks frame_time = now + ten_millis;
  // No main frame is expected until frame_time, so short idle work can be
  // scheduled in the mean time.
  scheduler_->BeginMainFrameNotExpectedUntil(frame_time);
  RunUntilIdle();
  EXPECT_EQ(1, run_count);  // Should have run in a long idle time.
  EXPECT_EQ(expected_deadline, deadline_in_task);
}

TEST_F(MainThreadSchedulerImplTest, TestLongIdlePeriod) {
  base::TimeTicks expected_deadline =
      clock_.NowTicks() + maximum_idle_period_duration();
  base::TimeTicks deadline_in_task;
  int run_count = 0;

  idle_task_runner_->PostIdleTask(
      FROM_HERE, base::BindOnce(&IdleTestTask, &run_count, &deadline_in_task));

  RunUntilIdle();
  EXPECT_EQ(0, run_count);  // Shouldn't run yet as no idle period.

  scheduler_->BeginFrameNotExpectedSoon();
  RunUntilIdle();
  EXPECT_EQ(1, run_count);  // Should have run in a long idle time.
  EXPECT_EQ(expected_deadline, deadline_in_task);
}

TEST_F(MainThreadSchedulerImplTest, TestLongIdlePeriodWithPendingDelayedTask) {
  base::TimeDelta pending_task_delay = base::TimeDelta::FromMilliseconds(30);
  base::TimeTicks expected_deadline = clock_.NowTicks() + pending_task_delay;
  base::TimeTicks deadline_in_task;
  int run_count = 0;

  idle_task_runner_->PostIdleTask(
      FROM_HERE, base::BindOnce(&IdleTestTask, &run_count, &deadline_in_task));
  default_task_runner_->PostDelayedTask(FROM_HERE, base::BindOnce(&NullTask),
                                        pending_task_delay);

  scheduler_->BeginFrameNotExpectedSoon();
  RunUntilIdle();
  EXPECT_EQ(1, run_count);  // Should have run in a long idle time.
  EXPECT_EQ(expected_deadline, deadline_in_task);
}

TEST_F(MainThreadSchedulerImplTest,
       TestLongIdlePeriodWithLatePendingDelayedTask) {
  base::TimeDelta pending_task_delay = base::TimeDelta::FromMilliseconds(10);
  base::TimeTicks deadline_in_task;
  int run_count = 0;

  default_task_runner_->PostDelayedTask(FROM_HERE, base::BindOnce(&NullTask),
                                        pending_task_delay);

  // Advance clock until after delayed task was meant to be run.
  clock_.Advance(base::TimeDelta::FromMilliseconds(20));

  // Post an idle task and BeginFrameNotExpectedSoon to initiate a long idle
  // period. Since there is a late pending delayed task this shouldn't actually
  // start an idle period.
  idle_task_runner_->PostIdleTask(
      FROM_HERE, base::BindOnce(&IdleTestTask, &run_count, &deadline_in_task));
  scheduler_->BeginFrameNotExpectedSoon();
  RunUntilIdle();
  EXPECT_EQ(0, run_count);

  // After the delayed task has been run we should trigger an idle period.
  clock_.Advance(maximum_idle_period_duration());
  RunUntilIdle();
  EXPECT_EQ(1, run_count);
}

TEST_F(MainThreadSchedulerImplTest, TestLongIdlePeriodRepeating) {
  ScopedAutoAdvanceNowEnabler enable_auto_advance_now(mock_task_runner_);
  std::vector<base::TimeTicks> actual_deadlines;
  int run_count = 0;

  g_max_idle_task_reposts = 3;
  base::TimeTicks clock_before(clock_.NowTicks());
  base::TimeDelta idle_task_runtime(base::TimeDelta::FromMilliseconds(10));
  idle_task_runner_->PostIdleTask(
      FROM_HERE,
      base::BindOnce(&RepostingUpdateClockIdleTestTask,
                     base::RetainedRef(idle_task_runner_), &run_count, &clock_,
                     idle_task_runtime, &actual_deadlines));
  scheduler_->BeginFrameNotExpectedSoon();
  RunUntilIdle();
  EXPECT_EQ(3, run_count);
  EXPECT_THAT(
      actual_deadlines,
      testing::ElementsAre(clock_before + maximum_idle_period_duration(),
                           clock_before + 2 * maximum_idle_period_duration(),
                           clock_before + 3 * maximum_idle_period_duration()));

  // Check that idle tasks don't run after the idle period ends with a
  // new BeginMainFrame.
  g_max_idle_task_reposts = 5;
  idle_task_runner_->PostIdleTask(
      FROM_HERE,
      base::BindOnce(&RepostingUpdateClockIdleTestTask,
                     base::RetainedRef(idle_task_runner_), &run_count, &clock_,
                     idle_task_runtime, &actual_deadlines));
  idle_task_runner_->PostIdleTask(
      FROM_HERE, base::BindOnce(&WillBeginFrameIdleTask,
                                base::Unretained(scheduler_.get()),
                                next_begin_frame_number_++, &clock_));
  RunUntilIdle();
  EXPECT_EQ(4, run_count);
}

TEST_F(MainThreadSchedulerImplTest, TestLongIdlePeriodInTouchStartPolicy) {
  base::TimeTicks deadline_in_task;
  int run_count = 0;

  idle_task_runner_->PostIdleTask(
      FROM_HERE, base::BindOnce(&IdleTestTask, &run_count, &deadline_in_task));

  // Observation of touchstart should defer the start of the long idle period.
  scheduler_->DidHandleInputEventOnCompositorThread(
      FakeInputEvent(blink::WebInputEvent::kTouchStart),
      InputEventState::EVENT_CONSUMED_BY_COMPOSITOR);
  scheduler_->BeginFrameNotExpectedSoon();
  RunUntilIdle();
  EXPECT_EQ(0, run_count);

  // The long idle period should start after the touchstart policy has finished.
  clock_.Advance(priority_escalation_after_input_duration());
  RunUntilIdle();
  EXPECT_EQ(1, run_count);
}

void TestCanExceedIdleDeadlineIfRequiredTask(ThreadScheduler* scheduler,
                                             bool* can_exceed_idle_deadline_out,
                                             int* run_count,
                                             base::TimeTicks deadline) {
  *can_exceed_idle_deadline_out = scheduler->CanExceedIdleDeadlineIfRequired();
  (*run_count)++;
}

TEST_F(MainThreadSchedulerImplTest, CanExceedIdleDeadlineIfRequired) {
  int run_count = 0;
  bool can_exceed_idle_deadline = false;

  // Should return false if not in an idle period.
  EXPECT_FALSE(scheduler_->CanExceedIdleDeadlineIfRequired());

  // Should return false for short idle periods.
  idle_task_runner_->PostIdleTask(
      FROM_HERE,
      base::BindOnce(&TestCanExceedIdleDeadlineIfRequiredTask, scheduler_.get(),
                     &can_exceed_idle_deadline, &run_count));
  EnableIdleTasks();
  RunUntilIdle();
  EXPECT_EQ(1, run_count);
  EXPECT_FALSE(can_exceed_idle_deadline);

  // Should return false for a long idle period which is shortened due to a
  // pending delayed task.
  default_task_runner_->PostDelayedTask(FROM_HERE, base::BindOnce(&NullTask),
                                        base::TimeDelta::FromMilliseconds(10));
  idle_task_runner_->PostIdleTask(
      FROM_HERE,
      base::BindOnce(&TestCanExceedIdleDeadlineIfRequiredTask, scheduler_.get(),
                     &can_exceed_idle_deadline, &run_count));
  scheduler_->BeginFrameNotExpectedSoon();
  RunUntilIdle();
  EXPECT_EQ(2, run_count);
  EXPECT_FALSE(can_exceed_idle_deadline);

  // Next long idle period will be for the maximum time, so
  // CanExceedIdleDeadlineIfRequired should return true.
  clock_.Advance(maximum_idle_period_duration());
  idle_task_runner_->PostIdleTask(
      FROM_HERE,
      base::BindOnce(&TestCanExceedIdleDeadlineIfRequiredTask, scheduler_.get(),
                     &can_exceed_idle_deadline, &run_count));
  RunUntilIdle();
  EXPECT_EQ(3, run_count);
  EXPECT_TRUE(can_exceed_idle_deadline);

  // Next long idle period will be for the maximum time, so
  // CanExceedIdleDeadlineIfRequired should return true.
  scheduler_->WillBeginFrame(viz::BeginFrameArgs::Create(
      BEGINFRAME_FROM_HERE, 0, next_begin_frame_number_++, clock_.NowTicks(),
      base::TimeTicks(), base::TimeDelta::FromMilliseconds(1000),
      viz::BeginFrameArgs::NORMAL));
  EXPECT_FALSE(scheduler_->CanExceedIdleDeadlineIfRequired());
}

TEST_F(MainThreadSchedulerImplTest, TestRendererHiddenIdlePeriod) {
  ScopedAutoAdvanceNowEnabler enable_auto_advance_now(mock_task_runner_);

  int run_count = 0;

  g_max_idle_task_reposts = 2;
  idle_task_runner_->PostIdleTask(
      FROM_HERE,
      base::BindOnce(&RepostingIdleTestTask,
                     base::RetainedRef(idle_task_runner_), &run_count));

  // Renderer should start in visible state.
  RunUntilIdle();
  EXPECT_EQ(0, run_count);

  // When we hide the renderer it should start a max deadline idle period, which
  // will run an idle task and then immediately start a new idle period, which
  // runs the second idle task.
  scheduler_->SetAllRenderWidgetsHidden(true);
  RunUntilIdle();
  EXPECT_EQ(2, run_count);

  // Advance time by amount of time by the maximum amount of time we execute
  // idle tasks when hidden (plus some slack) - idle period should have ended.
  g_max_idle_task_reposts = 3;
  idle_task_runner_->PostIdleTask(
      FROM_HERE,
      base::BindOnce(&RepostingIdleTestTask,
                     base::RetainedRef(idle_task_runner_), &run_count));
  clock_.Advance(end_idle_when_hidden_delay() +
                 base::TimeDelta::FromMilliseconds(10));
  RunUntilIdle();
  EXPECT_EQ(2, run_count);
}

TEST_F(MainThreadSchedulerImplTest, TimerQueueEnabledByDefault) {
  std::vector<std::string> run_order;
  PostTestTasks(&run_order, "T1 T2");
  RunUntilIdle();
  EXPECT_THAT(run_order,
              testing::ElementsAre(std::string("T1"), std::string("T2")));
}

TEST_F(MainThreadSchedulerImplTest, StopAndResumeRenderer) {
  std::vector<std::string> run_order;
  PostTestTasks(&run_order, "T1 T2");

  auto pause_handle = scheduler_->PauseRenderer();
  RunUntilIdle();
  EXPECT_THAT(run_order, testing::ElementsAre());

  pause_handle.reset();
  RunUntilIdle();
  EXPECT_THAT(run_order,
              testing::ElementsAre(std::string("T1"), std::string("T2")));
}

TEST_F(MainThreadSchedulerImplTest, StopAndThrottleTimerQueue) {
  std::vector<std::string> run_order;
  PostTestTasks(&run_order, "T1 T2");

  auto pause_handle = scheduler_->PauseRenderer();
  RunUntilIdle();
  scheduler_->task_queue_throttler()->IncreaseThrottleRefCount(
      static_cast<TaskQueue*>(timer_task_runner_.get()));
  RunUntilIdle();
  EXPECT_THAT(run_order, testing::ElementsAre());
}

TEST_F(MainThreadSchedulerImplTest, ThrottleAndPauseRenderer) {
  std::vector<std::string> run_order;
  PostTestTasks(&run_order, "T1 T2");

  scheduler_->task_queue_throttler()->IncreaseThrottleRefCount(
      static_cast<TaskQueue*>(timer_task_runner_.get()));
  RunUntilIdle();
  auto pause_handle = scheduler_->PauseRenderer();
  RunUntilIdle();
  EXPECT_THAT(run_order, testing::ElementsAre());
}

TEST_F(MainThreadSchedulerImplTest, MultipleStopsNeedMultipleResumes) {
  std::vector<std::string> run_order;
  PostTestTasks(&run_order, "T1 T2");

  auto pause_handle1 = scheduler_->PauseRenderer();
  auto pause_handle2 = scheduler_->PauseRenderer();
  auto pause_handle3 = scheduler_->PauseRenderer();
  RunUntilIdle();
  EXPECT_THAT(run_order, testing::ElementsAre());

  pause_handle1.reset();
  RunUntilIdle();
  EXPECT_THAT(run_order, testing::ElementsAre());

  pause_handle2.reset();
  RunUntilIdle();
  EXPECT_THAT(run_order, testing::ElementsAre());

  pause_handle3.reset();
  RunUntilIdle();
  EXPECT_THAT(run_order,
              testing::ElementsAre(std::string("T1"), std::string("T2")));
}

TEST_F(MainThreadSchedulerImplTest, PauseRenderer) {
  // Tasks in some queues don't fire when the renderer is paused.
  std::vector<std::string> run_order;
  PostTestTasks(&run_order, "D1 C1 L1 I1 T1");
  auto pause_handle = scheduler_->PauseRenderer();
  EnableIdleTasks();
  RunUntilIdle();
  EXPECT_THAT(run_order,
              testing::ElementsAre(std::string("D1"), std::string("C1"),
                                   std::string("I1")));

  // Tasks are executed when renderer is resumed.
  run_order.clear();
  pause_handle.reset();
  RunUntilIdle();
  EXPECT_THAT(run_order,
              testing::ElementsAre(std::string("L1"), std::string("T1")));
}

TEST_F(MainThreadSchedulerImplTest, UseCaseToString) {
  CheckAllUseCaseToString();
}

TEST_F(MainThreadSchedulerImplTest, MismatchedDidHandleInputEventOnMainThread) {
  // This should not DCHECK because there was no corresponding compositor side
  // call to DidHandleInputEventOnCompositorThread with
  // INPUT_EVENT_ACK_STATE_NOT_CONSUMED. There are legitimate reasons for the
  // compositor to not be there and we don't want to make debugging impossible.
  scheduler_->DidHandleInputEventOnMainThread(
      FakeInputEvent(blink::WebInputEvent::kGestureFlingStart),
      WebInputEventResult::kHandledSystem);
}

TEST_F(MainThreadSchedulerImplTest, BeginMainFrameOnCriticalPath) {
  ASSERT_FALSE(scheduler_->BeginMainFrameOnCriticalPath());

  viz::BeginFrameArgs begin_frame_args = viz::BeginFrameArgs::Create(
      BEGINFRAME_FROM_HERE, 0, next_begin_frame_number_++, clock_.NowTicks(),
      base::TimeTicks(), base::TimeDelta::FromMilliseconds(1000),
      viz::BeginFrameArgs::NORMAL);
  scheduler_->WillBeginFrame(begin_frame_args);
  ASSERT_TRUE(scheduler_->BeginMainFrameOnCriticalPath());

  begin_frame_args.on_critical_path = false;
  scheduler_->WillBeginFrame(begin_frame_args);
  ASSERT_FALSE(scheduler_->BeginMainFrameOnCriticalPath());
}

TEST_F(MainThreadSchedulerImplTest, ShutdownPreventsPostingOfNewTasks) {
  scheduler_->Shutdown();
  std::vector<std::string> run_order;
  PostTestTasks(&run_order, "D1 C1");
  RunUntilIdle();
  EXPECT_THAT(run_order, testing::ElementsAre());
}

TEST_F(MainThreadSchedulerImplTest, TestRendererBackgroundedTimerSuspension) {
  scheduler_->SetFreezingWhenBackgroundedEnabled(true);

  std::vector<std::string> run_order;
  PostTestTasks(&run_order, "T1 T2");

  base::TimeTicks now;

  // The background signal will not immediately suspend the timer queue.
  scheduler_->SetRendererBackgrounded(true);
  now += base::TimeDelta::FromMilliseconds(1100);
  clock_.SetNowTicks(now);
  RunUntilIdle();
  EXPECT_THAT(run_order,
              testing::ElementsAre(std::string("T1"), std::string("T2")));

  run_order.clear();
  PostTestTasks(&run_order, "T3");

  now += base::TimeDelta::FromSeconds(1);
  clock_.SetNowTicks(now);
  RunUntilIdle();
  EXPECT_THAT(run_order, testing::ElementsAre(std::string("T3")));

  // Advance the time until after the scheduled timer queue suspension.
  now = base::TimeTicks() + delay_for_background_tab_freezing() +
        base::TimeDelta::FromMilliseconds(10);
  run_order.clear();
  clock_.SetNowTicks(now);
  RunUntilIdle();
  ASSERT_TRUE(run_order.empty());

  // Timer tasks should be paused until the foregrounded signal.
  PostTestTasks(&run_order, "T4 T5 V1");
  now += base::TimeDelta::FromSeconds(10);
  clock_.SetNowTicks(now);
  RunUntilIdle();
  EXPECT_THAT(run_order, testing::ElementsAre(std::string("V1")));

  run_order.clear();
  scheduler_->SetRendererBackgrounded(false);
  RunUntilIdle();
  EXPECT_THAT(run_order,
              testing::ElementsAre(std::string("T4"), std::string("T5")));

  // Subsequent timer tasks should fire as usual.
  run_order.clear();
  PostTestTasks(&run_order, "T6");
  RunUntilIdle();
  EXPECT_THAT(run_order, testing::ElementsAre(std::string("T6")));
}

TEST_F(MainThreadSchedulerImplTest,
       ExpensiveLoadingTasksNotBlockedTillFirstBeginMainFrame) {
  std::vector<std::string> run_order;

  scheduler_->SetHasVisibleRenderWidgetWithTouchHandler(true);
  SimulateExpensiveTasks(loading_task_runner_);
  ForceTouchStartToBeExpectedSoon();
  PostTestTasks(&run_order, "L1 D1");
  RunUntilIdle();

  EXPECT_EQ(UseCase::kNone, ForceUpdatePolicyAndGetCurrentUseCase());
  EXPECT_FALSE(HaveSeenABeginMainframe());
  EXPECT_TRUE(LoadingTasksSeemExpensive());
  EXPECT_FALSE(TimerTasksSeemExpensive());
  EXPECT_TRUE(TouchStartExpectedSoon());
  EXPECT_THAT(run_order,
              testing::ElementsAre(std::string("L1"), std::string("D1")));

  // Emit a BeginMainFrame, and the loading task should get blocked.
  DoMainFrame();
  run_order.clear();

  PostTestTasks(&run_order, "L1 D1");
  RunUntilIdle();

  EXPECT_EQ(UseCase::kNone, CurrentUseCase());
  EXPECT_TRUE(HaveSeenABeginMainframe());
  EXPECT_TRUE(LoadingTasksSeemExpensive());
  EXPECT_FALSE(TimerTasksSeemExpensive());
  EXPECT_TRUE(TouchStartExpectedSoon());
  EXPECT_THAT(run_order, testing::ElementsAre(std::string("D1")));
  EXPECT_EQ(v8::PERFORMANCE_RESPONSE, GetRAILMode());
}

TEST_F(MainThreadSchedulerImplTest,
       ExpensiveLoadingTasksNotBlockedIfNoTouchHandler) {
  std::vector<std::string> run_order;

  scheduler_->SetHasVisibleRenderWidgetWithTouchHandler(false);
  DoMainFrame();
  SimulateExpensiveTasks(loading_task_runner_);
  ForceTouchStartToBeExpectedSoon();
  PostTestTasks(&run_order, "L1 D1");
  RunUntilIdle();

  EXPECT_EQ(UseCase::kNone, ForceUpdatePolicyAndGetCurrentUseCase());
  EXPECT_TRUE(HaveSeenABeginMainframe());
  EXPECT_TRUE(LoadingTasksSeemExpensive());
  EXPECT_FALSE(TimerTasksSeemExpensive());
  EXPECT_FALSE(TouchStartExpectedSoon());
  EXPECT_THAT(run_order,
              testing::ElementsAre(std::string("L1"), std::string("D1")));
  EXPECT_EQ(v8::PERFORMANCE_ANIMATION, GetRAILMode());
}

TEST_F(MainThreadSchedulerImplTest,
       ExpensiveTimerTaskBlocked_UseCase_NONE_PreviousCompositorGesture) {
  std::vector<std::string> run_order;

  scheduler_->SetHasVisibleRenderWidgetWithTouchHandler(true);
  DoMainFrame();
  SimulateExpensiveTasks(timer_task_runner_);
  ForceTouchStartToBeExpectedSoon();

  PostTestTasks(&run_order, "T1 D1");
  RunUntilIdle();

  EXPECT_EQ(UseCase::kNone, ForceUpdatePolicyAndGetCurrentUseCase());
  EXPECT_TRUE(HaveSeenABeginMainframe());
  EXPECT_FALSE(LoadingTasksSeemExpensive());
  EXPECT_TRUE(TimerTasksSeemExpensive());
  EXPECT_TRUE(TouchStartExpectedSoon());
  EXPECT_THAT(run_order, testing::ElementsAre(std::string("D1")));
  EXPECT_EQ(v8::PERFORMANCE_RESPONSE, GetRAILMode());
}

TEST_F(MainThreadSchedulerImplTest,
       ExpensiveTimerTaskNotBlocked_UseCase_NONE_PreviousMainThreadGesture) {
  std::vector<std::string> run_order;

  scheduler_->SetHasVisibleRenderWidgetWithTouchHandler(true);
  DoMainFrame();
  SimulateExpensiveTasks(timer_task_runner_);

  SimulateMainThreadGestureStart(TouchEventPolicy::kSendTouchStart,
                                 blink::WebInputEvent::kGestureScrollBegin);
  EXPECT_EQ(UseCase::kMainThreadCustomInputHandling,
            ForceUpdatePolicyAndGetCurrentUseCase());

  scheduler_->DidHandleInputEventOnCompositorThread(
      FakeInputEvent(blink::WebInputEvent::kTouchEnd),
      InputEventState::EVENT_FORWARDED_TO_MAIN_THREAD);
  scheduler_->DidHandleInputEventOnMainThread(
      FakeInputEvent(blink::WebInputEvent::kTouchEnd),
      WebInputEventResult::kHandledSystem);

  clock_.Advance(priority_escalation_after_input_duration() * 2);
  EXPECT_EQ(UseCase::kNone, ForceUpdatePolicyAndGetCurrentUseCase());

  PostTestTasks(&run_order, "T1 D1");
  RunUntilIdle();

  EXPECT_EQ(UseCase::kNone, ForceUpdatePolicyAndGetCurrentUseCase());
  EXPECT_TRUE(HaveSeenABeginMainframe());
  EXPECT_FALSE(LoadingTasksSeemExpensive());
  EXPECT_TRUE(TimerTasksSeemExpensive());
  EXPECT_TRUE(TouchStartExpectedSoon());
  EXPECT_THAT(run_order,
              testing::ElementsAre(std::string("T1"), std::string("D1")));
  EXPECT_EQ(v8::PERFORMANCE_ANIMATION, GetRAILMode());
}

TEST_F(MainThreadSchedulerImplTest,
       ExpensiveTimerTaskBlocked_UseCase_kCompositorGesture) {
  std::vector<std::string> run_order;

  scheduler_->SetHasVisibleRenderWidgetWithTouchHandler(true);
  DoMainFrame();
  SimulateExpensiveTasks(timer_task_runner_);
  ForceTouchStartToBeExpectedSoon();
  scheduler_->DidAnimateForInputOnCompositorThread();

  PostTestTasks(&run_order, "T1 D1");
  RunUntilIdle();

  EXPECT_EQ(UseCase::kCompositorGesture,
            ForceUpdatePolicyAndGetCurrentUseCase());
  EXPECT_TRUE(HaveSeenABeginMainframe());
  EXPECT_FALSE(LoadingTasksSeemExpensive());
  EXPECT_TRUE(TimerTasksSeemExpensive());
  EXPECT_TRUE(TouchStartExpectedSoon());
  EXPECT_THAT(run_order, testing::ElementsAre(std::string("D1")));
  EXPECT_EQ(v8::PERFORMANCE_RESPONSE, GetRAILMode());
}

TEST_F(MainThreadSchedulerImplTest,
       ExpensiveTimerTaskBlocked_EvenIfBeginMainFrameNotExpectedSoon) {
  std::vector<std::string> run_order;

  scheduler_->SetHasVisibleRenderWidgetWithTouchHandler(true);
  DoMainFrame();
  SimulateExpensiveTasks(timer_task_runner_);
  ForceTouchStartToBeExpectedSoon();
  scheduler_->BeginFrameNotExpectedSoon();

  PostTestTasks(&run_order, "T1 D1");
  RunUntilIdle();

  EXPECT_EQ(UseCase::kNone, ForceUpdatePolicyAndGetCurrentUseCase());
  EXPECT_TRUE(HaveSeenABeginMainframe());
  EXPECT_FALSE(LoadingTasksSeemExpensive());
  EXPECT_TRUE(TimerTasksSeemExpensive());
  EXPECT_TRUE(TouchStartExpectedSoon());
  EXPECT_THAT(run_order, testing::ElementsAre(std::string("D1")));
  EXPECT_EQ(v8::PERFORMANCE_RESPONSE, GetRAILMode());
}

TEST_F(MainThreadSchedulerImplTest,
       ExpensiveLoadingTasksBlockedIfChildFrameNavigationExpected) {
  std::vector<std::string> run_order;

  DoMainFrame();
  scheduler_->SetHasVisibleRenderWidgetWithTouchHandler(true);
  SimulateExpensiveTasks(loading_task_runner_);
  ForceTouchStartToBeExpectedSoon();

  PostTestTasks(&run_order, "L1 D1");
  RunUntilIdle();

  // The expensive loading task gets blocked.
  EXPECT_THAT(run_order, testing::ElementsAre(std::string("D1")));
  EXPECT_EQ(v8::PERFORMANCE_RESPONSE, GetRAILMode());
}

TEST_F(MainThreadSchedulerImplTest,
       ExpensiveLoadingTasksNotBlockedDuringMainThreadGestures) {
  std::vector<std::string> run_order;

  SimulateExpensiveTasks(loading_task_runner_);

  // Loading tasks should not be disabled during main thread user interactions.
  PostTestTasks(&run_order, "C1 L1");

  // Trigger main_thread_gesture UseCase
  SimulateMainThreadGestureStart(TouchEventPolicy::kSendTouchStart,
                                 blink::WebInputEvent::kGestureScrollBegin);
  RunUntilIdle();
  EXPECT_EQ(UseCase::kMainThreadCustomInputHandling, CurrentUseCase());

  EXPECT_TRUE(LoadingTasksSeemExpensive());
  EXPECT_FALSE(TimerTasksSeemExpensive());
  EXPECT_THAT(run_order,
              testing::ElementsAre(std::string("C1"), std::string("L1")));
  EXPECT_EQ(v8::PERFORMANCE_ANIMATION, GetRAILMode());
}

TEST_F(MainThreadSchedulerImplTest, ModeratelyExpensiveTimer_NotBlocked) {
  scheduler_->SetHasVisibleRenderWidgetWithTouchHandler(true);
  SimulateMainThreadGestureStart(TouchEventPolicy::kSendTouchStart,
                                 blink::WebInputEvent::kTouchMove);
  RunUntilIdle();
  for (int i = 0; i < 20; i++) {
    simulate_timer_task_ran_ = false;

    viz::BeginFrameArgs begin_frame_args = viz::BeginFrameArgs::Create(
        BEGINFRAME_FROM_HERE, 0, next_begin_frame_number_++, clock_.NowTicks(),
        base::TimeTicks(), base::TimeDelta::FromMilliseconds(16),
        viz::BeginFrameArgs::NORMAL);
    begin_frame_args.on_critical_path = false;
    scheduler_->WillBeginFrame(begin_frame_args);

    compositor_task_runner_->PostTask(
        FROM_HERE,
        base::BindOnce(&MainThreadSchedulerImplTest::
                           SimulateMainThreadInputHandlingCompositorTask,
                       base::Unretained(this),
                       base::TimeDelta::FromMilliseconds(8)));
    timer_task_runner_->PostTask(
        FROM_HERE,
        base::BindOnce(&MainThreadSchedulerImplTest::SimulateTimerTask,
                       base::Unretained(this),
                       base::TimeDelta::FromMilliseconds(4)));

    RunUntilIdle();
    EXPECT_TRUE(simulate_timer_task_ran_) << " i = " << i;
    EXPECT_EQ(UseCase::kMainThreadCustomInputHandling, CurrentUseCase())
        << " i = " << i;
    EXPECT_FALSE(LoadingTasksSeemExpensive()) << " i = " << i;
    EXPECT_FALSE(TimerTasksSeemExpensive()) << " i = " << i;

    base::TimeDelta time_till_next_frame =
        EstimatedNextFrameBegin() - clock_.NowTicks();
    if (time_till_next_frame > base::TimeDelta())
      clock_.Advance(time_till_next_frame);
  }
}

TEST_F(MainThreadSchedulerImplTest,
       FourtyMsTimer_NotBlocked_CompositorScrolling) {
  scheduler_->SetHasVisibleRenderWidgetWithTouchHandler(true);
  RunUntilIdle();
  for (int i = 0; i < 20; i++) {
    simulate_timer_task_ran_ = false;

    viz::BeginFrameArgs begin_frame_args = viz::BeginFrameArgs::Create(
        BEGINFRAME_FROM_HERE, 0, next_begin_frame_number_++, clock_.NowTicks(),
        base::TimeTicks(), base::TimeDelta::FromMilliseconds(16),
        viz::BeginFrameArgs::NORMAL);
    begin_frame_args.on_critical_path = false;
    scheduler_->WillBeginFrame(begin_frame_args);
    scheduler_->DidAnimateForInputOnCompositorThread();

    compositor_task_runner_->PostTask(
        FROM_HERE,
        base::BindOnce(
            &MainThreadSchedulerImplTest::SimulateMainThreadCompositorTask,
            base::Unretained(this), base::TimeDelta::FromMilliseconds(8)));
    timer_task_runner_->PostTask(
        FROM_HERE,
        base::BindOnce(&MainThreadSchedulerImplTest::SimulateTimerTask,
                       base::Unretained(this),
                       base::TimeDelta::FromMilliseconds(40)));

    RunUntilIdle();
    EXPECT_TRUE(simulate_timer_task_ran_) << " i = " << i;
    EXPECT_EQ(UseCase::kCompositorGesture, CurrentUseCase()) << " i = " << i;
    EXPECT_FALSE(LoadingTasksSeemExpensive()) << " i = " << i;
    EXPECT_FALSE(TimerTasksSeemExpensive()) << " i = " << i;

    base::TimeDelta time_till_next_frame =
        EstimatedNextFrameBegin() - clock_.NowTicks();
    if (time_till_next_frame > base::TimeDelta())
      clock_.Advance(time_till_next_frame);
  }
}

TEST_F(MainThreadSchedulerImplTest,
       ExpensiveTimer_NotBlocked_UseCase_MAIN_THREAD_CUSTOM_INPUT_HANDLING) {
  scheduler_->SetHasVisibleRenderWidgetWithTouchHandler(true);
  SimulateMainThreadGestureStart(TouchEventPolicy::kSendTouchStart,
                                 blink::WebInputEvent::kTouchMove);
  RunUntilIdle();
  for (int i = 0; i < 20; i++) {
    simulate_timer_task_ran_ = false;

    viz::BeginFrameArgs begin_frame_args = viz::BeginFrameArgs::Create(
        BEGINFRAME_FROM_HERE, 0, next_begin_frame_number_++, clock_.NowTicks(),
        base::TimeTicks(), base::TimeDelta::FromMilliseconds(16),
        viz::BeginFrameArgs::NORMAL);
    begin_frame_args.on_critical_path = false;
    scheduler_->WillBeginFrame(begin_frame_args);

    compositor_task_runner_->PostTask(
        FROM_HERE,
        base::BindOnce(&MainThreadSchedulerImplTest::
                           SimulateMainThreadInputHandlingCompositorTask,
                       base::Unretained(this),
                       base::TimeDelta::FromMilliseconds(8)));
    timer_task_runner_->PostTask(
        FROM_HERE,
        base::BindOnce(&MainThreadSchedulerImplTest::SimulateTimerTask,
                       base::Unretained(this),
                       base::TimeDelta::FromMilliseconds(10)));

    RunUntilIdle();
    EXPECT_EQ(UseCase::kMainThreadCustomInputHandling, CurrentUseCase())
        << " i = " << i;
    EXPECT_FALSE(LoadingTasksSeemExpensive()) << " i = " << i;
    if (i == 0) {
      EXPECT_FALSE(TimerTasksSeemExpensive()) << " i = " << i;
    } else {
      EXPECT_TRUE(TimerTasksSeemExpensive()) << " i = " << i;
    }
    EXPECT_TRUE(simulate_timer_task_ran_) << " i = " << i;

    base::TimeDelta time_till_next_frame =
        EstimatedNextFrameBegin() - clock_.NowTicks();
    if (time_till_next_frame > base::TimeDelta())
      clock_.Advance(time_till_next_frame);
  }
}

TEST_F(MainThreadSchedulerImplTest,
       EstimateLongestJankFreeTaskDuration_UseCase_NONE) {
  EXPECT_EQ(UseCase::kNone, CurrentUseCase());
  EXPECT_EQ(rails_response_time(),
            scheduler_->EstimateLongestJankFreeTaskDuration());
}

TEST_F(MainThreadSchedulerImplTest,
       EstimateLongestJankFreeTaskDuration_UseCase_kCompositorGesture) {
  SimulateCompositorGestureStart(TouchEventPolicy::kDontSendTouchStart);
  EXPECT_EQ(UseCase::kCompositorGesture,
            ForceUpdatePolicyAndGetCurrentUseCase());
  EXPECT_EQ(rails_response_time(),
            scheduler_->EstimateLongestJankFreeTaskDuration());
}

// TODO(alexclarke): Reenable once we've reinstaed the Loading
// UseCase.
TEST_F(MainThreadSchedulerImplTest,
       DISABLED_EstimateLongestJankFreeTaskDuration_UseCase_) {
  scheduler_->DidStartProvisionalLoad(true);
  EXPECT_EQ(UseCase::kLoading, ForceUpdatePolicyAndGetCurrentUseCase());
  EXPECT_EQ(rails_response_time(),
            scheduler_->EstimateLongestJankFreeTaskDuration());
}

TEST_F(MainThreadSchedulerImplTest,
       EstimateLongestJankFreeTaskDuration_UseCase_MAIN_THREAD_GESTURE) {
  SimulateMainThreadGestureStart(TouchEventPolicy::kSendTouchStart,
                                 blink::WebInputEvent::kGestureScrollUpdate);
  viz::BeginFrameArgs begin_frame_args = viz::BeginFrameArgs::Create(
      BEGINFRAME_FROM_HERE, 0, next_begin_frame_number_++, clock_.NowTicks(),
      base::TimeTicks(), base::TimeDelta::FromMilliseconds(16),
      viz::BeginFrameArgs::NORMAL);
  begin_frame_args.on_critical_path = false;
  scheduler_->WillBeginFrame(begin_frame_args);

  compositor_task_runner_->PostTask(
      FROM_HERE,
      base::BindOnce(&MainThreadSchedulerImplTest::
                         SimulateMainThreadInputHandlingCompositorTask,
                     base::Unretained(this),
                     base::TimeDelta::FromMilliseconds(5)));

  RunUntilIdle();
  EXPECT_EQ(UseCase::kMainThreadGesture, CurrentUseCase());

  // 16ms frame - 5ms compositor work = 11ms for other stuff.
  EXPECT_EQ(base::TimeDelta::FromMilliseconds(11),
            scheduler_->EstimateLongestJankFreeTaskDuration());
}

TEST_F(
    MainThreadSchedulerImplTest,
    EstimateLongestJankFreeTaskDuration_UseCase_MAIN_THREAD_CUSTOM_INPUT_HANDLING) {
  viz::BeginFrameArgs begin_frame_args = viz::BeginFrameArgs::Create(
      BEGINFRAME_FROM_HERE, 0, next_begin_frame_number_++, clock_.NowTicks(),
      base::TimeTicks(), base::TimeDelta::FromMilliseconds(16),
      viz::BeginFrameArgs::NORMAL);
  begin_frame_args.on_critical_path = false;
  scheduler_->WillBeginFrame(begin_frame_args);

  compositor_task_runner_->PostTask(
      FROM_HERE,
      base::BindOnce(&MainThreadSchedulerImplTest::
                         SimulateMainThreadInputHandlingCompositorTask,
                     base::Unretained(this),
                     base::TimeDelta::FromMilliseconds(5)));

  RunUntilIdle();
  EXPECT_EQ(UseCase::kMainThreadCustomInputHandling, CurrentUseCase());

  // 16ms frame - 5ms compositor work = 11ms for other stuff.
  EXPECT_EQ(base::TimeDelta::FromMilliseconds(11),
            scheduler_->EstimateLongestJankFreeTaskDuration());
}

TEST_F(MainThreadSchedulerImplTest,
       EstimateLongestJankFreeTaskDuration_UseCase_SYNCHRONIZED_GESTURE) {
  SimulateCompositorGestureStart(TouchEventPolicy::kDontSendTouchStart);

  viz::BeginFrameArgs begin_frame_args = viz::BeginFrameArgs::Create(
      BEGINFRAME_FROM_HERE, 0, next_begin_frame_number_++, clock_.NowTicks(),
      base::TimeTicks(), base::TimeDelta::FromMilliseconds(16),
      viz::BeginFrameArgs::NORMAL);
  begin_frame_args.on_critical_path = true;
  scheduler_->WillBeginFrame(begin_frame_args);

  compositor_task_runner_->PostTask(
      FROM_HERE,
      base::BindOnce(
          &MainThreadSchedulerImplTest::SimulateMainThreadCompositorTask,
          base::Unretained(this), base::TimeDelta::FromMilliseconds(5)));

  RunUntilIdle();
  EXPECT_EQ(UseCase::kSynchronizedGesture, CurrentUseCase());

  // 16ms frame - 5ms compositor work = 11ms for other stuff.
  EXPECT_EQ(base::TimeDelta::FromMilliseconds(11),
            scheduler_->EstimateLongestJankFreeTaskDuration());
}

class PageSchedulerImplForTest : public PageSchedulerImpl {
 public:
  explicit PageSchedulerImplForTest(MainThreadSchedulerImpl* scheduler)
      : PageSchedulerImpl(nullptr, scheduler, false) {}
  ~PageSchedulerImplForTest() override = default;

  void ReportIntervention(const std::string& message) override {
    interventions_.push_back(message);
  }

  const std::vector<std::string>& Interventions() const {
    return interventions_;
  }

  MOCK_METHOD1(RequestBeginMainFrameNotExpected, void(bool));

 private:
  std::vector<std::string> interventions_;

  DISALLOW_COPY_AND_ASSIGN(PageSchedulerImplForTest);
};

namespace {
void SlowCountingTask(size_t* count,
                      base::SimpleTestTickClock* clock,
                      int task_duration,
                      scoped_refptr<base::SingleThreadTaskRunner> timer_queue) {
  clock->Advance(base::TimeDelta::FromMilliseconds(task_duration));
  if (++(*count) < 500) {
    timer_queue->PostTask(
        FROM_HERE, base::BindOnce(SlowCountingTask, count, clock, task_duration,
                                  timer_queue));
  }
}
}  // namespace

TEST_F(MainThreadSchedulerImplTest,
       SYNCHRONIZED_GESTURE_TimerTaskThrottling_task_expensive) {
  SimulateCompositorGestureStart(TouchEventPolicy::kSendTouchStart);

  base::TimeTicks first_throttled_run_time =
      TaskQueueThrottler::AlignedThrottledRunTime(clock_.NowTicks());

  size_t count = 0;
  // With the compositor task taking 10ms, there is not enough time to run this
  // 7ms timer task in the 16ms frame.
  timer_task_runner_->PostTask(
      FROM_HERE,
      base::BindOnce(SlowCountingTask, &count, &clock_, 7, timer_task_runner_));

  for (int i = 0; i < 1000; i++) {
    viz::BeginFrameArgs begin_frame_args = viz::BeginFrameArgs::Create(
        BEGINFRAME_FROM_HERE, 0, next_begin_frame_number_++, clock_.NowTicks(),
        base::TimeTicks(), base::TimeDelta::FromMilliseconds(16),
        viz::BeginFrameArgs::NORMAL);
    begin_frame_args.on_critical_path = true;
    scheduler_->WillBeginFrame(begin_frame_args);
    scheduler_->DidHandleInputEventOnCompositorThread(
        FakeInputEvent(blink::WebInputEvent::kGestureScrollUpdate),
        InputEventState::EVENT_CONSUMED_BY_COMPOSITOR);

    simulate_compositor_task_ran_ = false;
    compositor_task_runner_->PostTask(
        FROM_HERE,
        base::BindOnce(
            &MainThreadSchedulerImplTest::SimulateMainThreadCompositorTask,
            base::Unretained(this), base::TimeDelta::FromMilliseconds(10)));

    mock_task_runner_->RunTasksWhile(base::BindRepeating(
        &MainThreadSchedulerImplTest::SimulatedCompositorTaskPending,
        base::Unretained(this)));
    EXPECT_EQ(UseCase::kSynchronizedGesture, CurrentUseCase()) << "i = " << i;

    // We expect the queue to get throttled on the second iteration which is
    // when the system realizes the task is expensive.
    bool expect_queue_throttled = (i > 0);
    EXPECT_EQ(expect_queue_throttled,
              scheduler_->task_queue_throttler()->IsThrottled(
                  timer_task_runner_.get()))
        << "i = " << i;

    if (expect_queue_throttled) {
      EXPECT_GE(count, 2u);
    } else {
      EXPECT_LE(count, 2u);
    }

    // The task runs twice before the system realizes it's too expensive.
    bool throttled_task_has_run = count > 2;
    bool throttled_task_expected_to_have_run =
        (clock_.NowTicks() > first_throttled_run_time);
    EXPECT_EQ(throttled_task_expected_to_have_run, throttled_task_has_run)
        << "i = " << i << " count = " << count;
  }

  // Task is throttled but not completely blocked.
  EXPECT_EQ(12u, count);
}

TEST_F(MainThreadSchedulerImplTest,
       SYNCHRONIZED_GESTURE_TimerTaskThrottling_TimersStopped) {
  SimulateCompositorGestureStart(TouchEventPolicy::kSendTouchStart);

  base::TimeTicks first_throttled_run_time =
      TaskQueueThrottler::AlignedThrottledRunTime(clock_.NowTicks());

  size_t count = 0;
  // With the compositor task taking 10ms, there is not enough time to run this
  // 7ms timer task in the 16ms frame.
  timer_task_runner_->PostTask(
      FROM_HERE,
      base::BindOnce(SlowCountingTask, &count, &clock_, 7, timer_task_runner_));

  std::unique_ptr<WebMainThreadScheduler::RendererPauseHandle> paused;
  for (int i = 0; i < 1000; i++) {
    viz::BeginFrameArgs begin_frame_args = viz::BeginFrameArgs::Create(
        BEGINFRAME_FROM_HERE, 0, next_begin_frame_number_++, clock_.NowTicks(),
        base::TimeTicks(), base::TimeDelta::FromMilliseconds(16),
        viz::BeginFrameArgs::NORMAL);
    begin_frame_args.on_critical_path = true;
    scheduler_->WillBeginFrame(begin_frame_args);
    scheduler_->DidHandleInputEventOnCompositorThread(
        FakeInputEvent(blink::WebInputEvent::kGestureScrollUpdate),
        InputEventState::EVENT_CONSUMED_BY_COMPOSITOR);

    simulate_compositor_task_ran_ = false;
    compositor_task_runner_->PostTask(
        FROM_HERE,
        base::BindOnce(
            &MainThreadSchedulerImplTest::SimulateMainThreadCompositorTask,
            base::Unretained(this), base::TimeDelta::FromMilliseconds(10)));

    mock_task_runner_->RunTasksWhile(base::BindRepeating(
        &MainThreadSchedulerImplTest::SimulatedCompositorTaskPending,
        base::Unretained(this)));
    EXPECT_EQ(UseCase::kSynchronizedGesture, CurrentUseCase()) << "i = " << i;

    // Before the policy is updated the queue will be enabled. Subsequently it
    // will be disabled until the throttled queue is pumped.
    bool expect_queue_enabled =
        (i == 0) || (clock_.NowTicks() > first_throttled_run_time);
    if (paused)
      expect_queue_enabled = false;
    EXPECT_EQ(expect_queue_enabled, timer_task_runner_->IsQueueEnabled())
        << "i = " << i;

    // After we've run any expensive tasks suspend the queue.  The throttling
    // helper should /not/ re-enable this queue under any circumstances while
    // timers are paused.
    if (count > 0 && !paused) {
      EXPECT_EQ(2u, count);
      paused = scheduler_->PauseRenderer();
    }
  }

  // Make sure the timer queue stayed paused!
  EXPECT_EQ(2u, count);
}

TEST_F(MainThreadSchedulerImplTest,
       SYNCHRONIZED_GESTURE_TimerTaskThrottling_task_not_expensive) {
  SimulateCompositorGestureStart(TouchEventPolicy::kSendTouchStart);

  size_t count = 0;
  // With the compositor task taking 10ms, there is enough time to run this 6ms
  // timer task in the 16ms frame.
  timer_task_runner_->PostTask(
      FROM_HERE,
      base::BindOnce(SlowCountingTask, &count, &clock_, 6, timer_task_runner_));

  for (int i = 0; i < 1000; i++) {
    viz::BeginFrameArgs begin_frame_args = viz::BeginFrameArgs::Create(
        BEGINFRAME_FROM_HERE, 0, next_begin_frame_number_++, clock_.NowTicks(),
        base::TimeTicks(), base::TimeDelta::FromMilliseconds(16),
        viz::BeginFrameArgs::NORMAL);
    begin_frame_args.on_critical_path = true;
    scheduler_->WillBeginFrame(begin_frame_args);
    scheduler_->DidHandleInputEventOnCompositorThread(
        FakeInputEvent(blink::WebInputEvent::kGestureScrollUpdate),
        InputEventState::EVENT_CONSUMED_BY_COMPOSITOR);

    simulate_compositor_task_ran_ = false;
    compositor_task_runner_->PostTask(
        FROM_HERE,
        base::BindOnce(
            &MainThreadSchedulerImplTest::SimulateMainThreadCompositorTask,
            base::Unretained(this), base::TimeDelta::FromMilliseconds(10)));

    mock_task_runner_->RunTasksWhile(base::BindRepeating(
        &MainThreadSchedulerImplTest::SimulatedCompositorTaskPending,
        base::Unretained(this)));
    EXPECT_EQ(UseCase::kSynchronizedGesture, CurrentUseCase()) << "i = " << i;
    EXPECT_TRUE(timer_task_runner_->IsQueueEnabled()) << "i = " << i;
  }

  // Task is not throttled.
  EXPECT_EQ(500u, count);
}

TEST_F(MainThreadSchedulerImplTest,
       ExpensiveTimerTaskBlocked_SYNCHRONIZED_GESTURE_TouchStartExpected) {
  SimulateExpensiveTasks(timer_task_runner_);
  SimulateCompositorGestureStart(TouchEventPolicy::kSendTouchStart);
  scheduler_->SetHasVisibleRenderWidgetWithTouchHandler(true);
  ForceTouchStartToBeExpectedSoon();

  // Bump us into SYNCHRONIZED_GESTURE.
  scheduler_->DidHandleInputEventOnCompositorThread(
      FakeInputEvent(blink::WebInputEvent::kGestureScrollUpdate),
      InputEventState::EVENT_CONSUMED_BY_COMPOSITOR);

  viz::BeginFrameArgs begin_frame_args = viz::BeginFrameArgs::Create(
      BEGINFRAME_FROM_HERE, 0, next_begin_frame_number_++, clock_.NowTicks(),
      base::TimeTicks(), base::TimeDelta::FromMilliseconds(16),
      viz::BeginFrameArgs::NORMAL);
  begin_frame_args.on_critical_path = true;
  scheduler_->WillBeginFrame(begin_frame_args);

  EXPECT_EQ(UseCase::kSynchronizedGesture,
            ForceUpdatePolicyAndGetCurrentUseCase());

  EXPECT_TRUE(TimerTasksSeemExpensive());
  EXPECT_TRUE(TouchStartExpectedSoon());
  EXPECT_FALSE(timer_task_runner_->IsQueueEnabled());
}

TEST_F(MainThreadSchedulerImplTest, DenyLongIdleDuringTouchStart) {
  scheduler_->DidHandleInputEventOnCompositorThread(
      FakeInputEvent(blink::WebInputEvent::kTouchStart),
      InputEventState::EVENT_CONSUMED_BY_COMPOSITOR);
  EXPECT_EQ(UseCase::kTouchstart, ForceUpdatePolicyAndGetCurrentUseCase());

  // First check that long idle is denied during the TOUCHSTART use case.
  IdleHelper::Delegate* idle_delegate = scheduler_.get();
  base::TimeTicks now;
  base::TimeDelta next_time_to_check;
  EXPECT_FALSE(idle_delegate->CanEnterLongIdlePeriod(now, &next_time_to_check));
  EXPECT_GE(next_time_to_check, base::TimeDelta());

  // Check again at a time past the TOUCHSTART expiration. We should still get a
  // non-negative delay to when to check again.
  now += base::TimeDelta::FromMilliseconds(500);
  EXPECT_FALSE(idle_delegate->CanEnterLongIdlePeriod(now, &next_time_to_check));
  EXPECT_GE(next_time_to_check, base::TimeDelta());
}

TEST_F(MainThreadSchedulerImplTest,
       TestCompositorPolicy_TouchStartDuringFling) {
  scheduler_->SetHasVisibleRenderWidgetWithTouchHandler(true);
  scheduler_->DidAnimateForInputOnCompositorThread();
  // Note DidAnimateForInputOnCompositorThread does not by itself trigger a
  // policy update.
  EXPECT_EQ(UseCase::kCompositorGesture,
            ForceUpdatePolicyAndGetCurrentUseCase());

  // Make sure TouchStart causes a policy change.
  scheduler_->DidHandleInputEventOnCompositorThread(
      FakeInputEvent(blink::WebInputEvent::kTouchStart),
      InputEventState::EVENT_FORWARDED_TO_MAIN_THREAD);
  EXPECT_EQ(UseCase::kTouchstart, ForceUpdatePolicyAndGetCurrentUseCase());
}

TEST_F(MainThreadSchedulerImplTest, SYNCHRONIZED_GESTURE_CompositingExpensive) {
  SimulateCompositorGestureStart(TouchEventPolicy::kSendTouchStart);

  // With the compositor task taking 20ms, there is not enough time to run
  // other tasks in the same 16ms frame. To avoid starvation, compositing tasks
  // should therefore not get prioritized.
  std::vector<std::string> run_order;
  for (int i = 0; i < 1000; i++)
    PostTestTasks(&run_order, "T1");

  for (int i = 0; i < 100; i++) {
    viz::BeginFrameArgs begin_frame_args = viz::BeginFrameArgs::Create(
        BEGINFRAME_FROM_HERE, 0, next_begin_frame_number_++, clock_.NowTicks(),
        base::TimeTicks(), base::TimeDelta::FromMilliseconds(16),
        viz::BeginFrameArgs::NORMAL);
    begin_frame_args.on_critical_path = true;
    scheduler_->WillBeginFrame(begin_frame_args);
    scheduler_->DidHandleInputEventOnCompositorThread(
        FakeInputEvent(blink::WebInputEvent::kGestureScrollUpdate),
        InputEventState::EVENT_CONSUMED_BY_COMPOSITOR);

    simulate_compositor_task_ran_ = false;
    compositor_task_runner_->PostTask(
        FROM_HERE,
        base::BindOnce(
            &MainThreadSchedulerImplTest::SimulateMainThreadCompositorTask,
            base::Unretained(this), base::TimeDelta::FromMilliseconds(20)));

    mock_task_runner_->RunTasksWhile(base::BindRepeating(
        &MainThreadSchedulerImplTest::SimulatedCompositorTaskPending,
        base::Unretained(this)));
    EXPECT_EQ(UseCase::kSynchronizedGesture, CurrentUseCase()) << "i = " << i;
  }

  // Timer tasks should not have been starved by the expensive compositor
  // tasks.
  EXPECT_EQ(TaskQueue::kNormalPriority,
            scheduler_->CompositorTaskQueue()->GetQueuePriority());
  EXPECT_EQ(1000u, run_order.size());
}

TEST_F(MainThreadSchedulerImplTest, MAIN_THREAD_CUSTOM_INPUT_HANDLING) {
  SimulateMainThreadGestureStart(TouchEventPolicy::kSendTouchStart,
                                 blink::WebInputEvent::kGestureScrollBegin);

  // With the compositor task taking 20ms, there is not enough time to run
  // other tasks in the same 16ms frame. To avoid starvation, compositing tasks
  // should therefore not get prioritized.
  std::vector<std::string> run_order;
  for (int i = 0; i < 1000; i++)
    PostTestTasks(&run_order, "T1");

  for (int i = 0; i < 100; i++) {
    viz::BeginFrameArgs begin_frame_args = viz::BeginFrameArgs::Create(
        BEGINFRAME_FROM_HERE, 0, next_begin_frame_number_++, clock_.NowTicks(),
        base::TimeTicks(), base::TimeDelta::FromMilliseconds(16),
        viz::BeginFrameArgs::NORMAL);
    begin_frame_args.on_critical_path = true;
    scheduler_->WillBeginFrame(begin_frame_args);
    scheduler_->DidHandleInputEventOnCompositorThread(
        FakeInputEvent(blink::WebInputEvent::kTouchMove),
        InputEventState::EVENT_FORWARDED_TO_MAIN_THREAD);

    simulate_compositor_task_ran_ = false;
    compositor_task_runner_->PostTask(
        FROM_HERE,
        base::BindOnce(
            &MainThreadSchedulerImplTest::SimulateMainThreadCompositorTask,
            base::Unretained(this), base::TimeDelta::FromMilliseconds(20)));

    mock_task_runner_->RunTasksWhile(base::BindRepeating(
        &MainThreadSchedulerImplTest::SimulatedCompositorTaskPending,
        base::Unretained(this)));
    EXPECT_EQ(UseCase::kMainThreadCustomInputHandling, CurrentUseCase())
        << "i = " << i;
  }

  // Timer tasks should not have been starved by the expensive compositor
  // tasks.
  EXPECT_EQ(TaskQueue::kNormalPriority,
            scheduler_->CompositorTaskQueue()->GetQueuePriority());
  EXPECT_EQ(1000u, run_order.size());
}

TEST_F(MainThreadSchedulerImplTest, MAIN_THREAD_GESTURE) {
  SimulateMainThreadGestureStart(TouchEventPolicy::kDontSendTouchStart,
                                 blink::WebInputEvent::kGestureScrollBegin);

  // With the compositor task taking 20ms, there is not enough time to run
  // other tasks in the same 16ms frame. However because this is a main thread
  // gesture instead of custom main thread input handling, we allow the timer
  // tasks to be starved.
  std::vector<std::string> run_order;
  for (int i = 0; i < 1000; i++)
    PostTestTasks(&run_order, "T1");

  for (int i = 0; i < 100; i++) {
    viz::BeginFrameArgs begin_frame_args = viz::BeginFrameArgs::Create(
        BEGINFRAME_FROM_HERE, 0, next_begin_frame_number_++, clock_.NowTicks(),
        base::TimeTicks(), base::TimeDelta::FromMilliseconds(16),
        viz::BeginFrameArgs::NORMAL);
    begin_frame_args.on_critical_path = true;
    scheduler_->WillBeginFrame(begin_frame_args);
    scheduler_->DidHandleInputEventOnCompositorThread(
        FakeInputEvent(blink::WebInputEvent::kGestureScrollUpdate),
        InputEventState::EVENT_FORWARDED_TO_MAIN_THREAD);

    simulate_compositor_task_ran_ = false;
    compositor_task_runner_->PostTask(
        FROM_HERE,
        base::BindOnce(
            &MainThreadSchedulerImplTest::SimulateMainThreadCompositorTask,
            base::Unretained(this), base::TimeDelta::FromMilliseconds(20)));

    mock_task_runner_->RunTasksWhile(base::BindRepeating(
        &MainThreadSchedulerImplTest::SimulatedCompositorTaskPending,
        base::Unretained(this)));
    EXPECT_EQ(UseCase::kMainThreadGesture, CurrentUseCase()) << "i = " << i;
  }

  EXPECT_EQ(TaskQueue::kHighestPriority,
            scheduler_->CompositorTaskQueue()->GetQueuePriority());
  EXPECT_EQ(279u, run_order.size());
}

class MockRAILModeObserver : public WebMainThreadScheduler::RAILModeObserver {
 public:
  MOCK_METHOD1(OnRAILModeChanged, void(v8::RAILMode rail_mode));
};

TEST_F(MainThreadSchedulerImplTest, TestResponseRAILMode) {
  MockRAILModeObserver observer;
  scheduler_->SetRAILModeObserver(&observer);
  EXPECT_CALL(observer, OnRAILModeChanged(v8::PERFORMANCE_RESPONSE));

  scheduler_->SetHasVisibleRenderWidgetWithTouchHandler(true);
  ForceTouchStartToBeExpectedSoon();
  EXPECT_EQ(UseCase::kNone, ForceUpdatePolicyAndGetCurrentUseCase());
  EXPECT_EQ(v8::PERFORMANCE_RESPONSE, GetRAILMode());
  scheduler_->SetRAILModeObserver(nullptr);
}

TEST_F(MainThreadSchedulerImplTest, TestAnimateRAILMode) {
  MockRAILModeObserver observer;
  scheduler_->SetRAILModeObserver(&observer);
  EXPECT_CALL(observer, OnRAILModeChanged(v8::PERFORMANCE_ANIMATION)).Times(0);

  EXPECT_FALSE(BeginFrameNotExpectedSoon());
  EXPECT_EQ(UseCase::kNone, ForceUpdatePolicyAndGetCurrentUseCase());
  EXPECT_EQ(v8::PERFORMANCE_ANIMATION, GetRAILMode());
  scheduler_->SetRAILModeObserver(nullptr);
}

TEST_F(MainThreadSchedulerImplTest, TestIdleRAILMode) {
  MockRAILModeObserver observer;
  scheduler_->SetRAILModeObserver(&observer);
  EXPECT_CALL(observer, OnRAILModeChanged(v8::PERFORMANCE_ANIMATION));
  EXPECT_CALL(observer, OnRAILModeChanged(v8::PERFORMANCE_IDLE));

  scheduler_->SetAllRenderWidgetsHidden(true);
  EXPECT_EQ(UseCase::kNone, ForceUpdatePolicyAndGetCurrentUseCase());
  EXPECT_EQ(v8::PERFORMANCE_IDLE, GetRAILMode());
  scheduler_->SetAllRenderWidgetsHidden(false);
  EXPECT_EQ(UseCase::kNone, ForceUpdatePolicyAndGetCurrentUseCase());
  EXPECT_EQ(v8::PERFORMANCE_ANIMATION, GetRAILMode());
  scheduler_->SetRAILModeObserver(nullptr);
}

TEST_F(MainThreadSchedulerImplTest, TestLoadRAILMode) {
  MockRAILModeObserver observer;
  scheduler_->SetRAILModeObserver(&observer);
  EXPECT_CALL(observer, OnRAILModeChanged(v8::PERFORMANCE_ANIMATION));
  EXPECT_CALL(observer, OnRAILModeChanged(v8::PERFORMANCE_LOAD));

  scheduler_->DidStartProvisionalLoad(true);
  EXPECT_EQ(v8::PERFORMANCE_LOAD, GetRAILMode());
  EXPECT_EQ(UseCase::kLoading, ForceUpdatePolicyAndGetCurrentUseCase());
  scheduler_->OnFirstMeaningfulPaint();
  EXPECT_EQ(UseCase::kNone, ForceUpdatePolicyAndGetCurrentUseCase());
  EXPECT_EQ(v8::PERFORMANCE_ANIMATION, GetRAILMode());
  scheduler_->SetRAILModeObserver(nullptr);
}

TEST_F(MainThreadSchedulerImplTest, InputTerminatesLoadRAILMode) {
  MockRAILModeObserver observer;
  scheduler_->SetRAILModeObserver(&observer);
  EXPECT_CALL(observer, OnRAILModeChanged(v8::PERFORMANCE_ANIMATION));
  EXPECT_CALL(observer, OnRAILModeChanged(v8::PERFORMANCE_LOAD));

  scheduler_->DidStartProvisionalLoad(true);
  EXPECT_EQ(v8::PERFORMANCE_LOAD, GetRAILMode());
  EXPECT_EQ(UseCase::kLoading, ForceUpdatePolicyAndGetCurrentUseCase());
  scheduler_->DidHandleInputEventOnCompositorThread(
      FakeInputEvent(blink::WebInputEvent::kGestureScrollBegin),
      InputEventState::EVENT_CONSUMED_BY_COMPOSITOR);
  scheduler_->DidHandleInputEventOnCompositorThread(
      FakeInputEvent(blink::WebInputEvent::kGestureScrollUpdate),
      InputEventState::EVENT_CONSUMED_BY_COMPOSITOR);
  EXPECT_EQ(UseCase::kCompositorGesture,
            ForceUpdatePolicyAndGetCurrentUseCase());
  EXPECT_EQ(v8::PERFORMANCE_ANIMATION, GetRAILMode());
  scheduler_->SetRAILModeObserver(nullptr);
}

TEST_F(MainThreadSchedulerImplTest, UnthrottledTaskRunner) {
  // Ensure neither suspension nor timer task throttling affects an unthrottled
  // task runner.
  SimulateCompositorGestureStart(TouchEventPolicy::kSendTouchStart);
  scoped_refptr<TaskQueue> unthrottled_task_runner =
      scheduler_->NewTaskQueue(MainThreadTaskQueue::QueueCreationParams(
          MainThreadTaskQueue::QueueType::kUnthrottled));

  size_t timer_count = 0;
  size_t unthrottled_count = 0;
  timer_task_runner_->PostTask(
      FROM_HERE, base::BindOnce(SlowCountingTask, &timer_count, &clock_, 7,
                                timer_task_runner_));
  unthrottled_task_runner->PostTask(
      FROM_HERE, base::BindOnce(SlowCountingTask, &unthrottled_count, &clock_,
                                7, unthrottled_task_runner));
  auto handle = scheduler_->PauseRenderer();

  for (int i = 0; i < 1000; i++) {
    viz::BeginFrameArgs begin_frame_args = viz::BeginFrameArgs::Create(
        BEGINFRAME_FROM_HERE, 0, next_begin_frame_number_++, clock_.NowTicks(),
        base::TimeTicks(), base::TimeDelta::FromMilliseconds(16),
        viz::BeginFrameArgs::NORMAL);
    begin_frame_args.on_critical_path = true;
    scheduler_->WillBeginFrame(begin_frame_args);
    scheduler_->DidHandleInputEventOnCompositorThread(
        FakeInputEvent(blink::WebInputEvent::kGestureScrollUpdate),
        InputEventState::EVENT_CONSUMED_BY_COMPOSITOR);

    simulate_compositor_task_ran_ = false;
    compositor_task_runner_->PostTask(
        FROM_HERE,
        base::BindOnce(
            &MainThreadSchedulerImplTest::SimulateMainThreadCompositorTask,
            base::Unretained(this), base::TimeDelta::FromMilliseconds(10)));

    mock_task_runner_->RunTasksWhile(base::BindRepeating(
        &MainThreadSchedulerImplTest::SimulatedCompositorTaskPending,
        base::Unretained(this)));
    EXPECT_EQ(UseCase::kSynchronizedGesture, CurrentUseCase()) << "i = " << i;
  }

  EXPECT_EQ(0u, timer_count);
  EXPECT_EQ(500u, unthrottled_count);
}

TEST_F(MainThreadSchedulerImplTest,
       VirtualTimePolicyDoesNotAffectNewTimerTaskQueueIfVirtualTimeNotEnabled) {
  scheduler_->SetVirtualTimePolicy(
      PageSchedulerImpl::VirtualTimePolicy::kPause);
  scoped_refptr<MainThreadTaskQueue> timer_tq = scheduler_->NewTimerTaskQueue(
      MainThreadTaskQueue::QueueType::kFrameThrottleable, nullptr);
  EXPECT_FALSE(timer_tq->HasActiveFence());
}

TEST_F(MainThreadSchedulerImplTest, EnableVirtualTime) {
  EXPECT_FALSE(scheduler_->IsVirtualTimeEnabled());
  scheduler_->EnableVirtualTime(
      MainThreadSchedulerImpl::BaseTimeOverridePolicy::DO_NOT_OVERRIDE);
  EXPECT_TRUE(scheduler_->IsVirtualTimeEnabled());
  scoped_refptr<MainThreadTaskQueue> loading_tq =
      scheduler_->NewLoadingTaskQueue(
          MainThreadTaskQueue::QueueType::kFrameLoading, nullptr);
  scoped_refptr<TaskQueue> loading_control_tq = scheduler_->NewLoadingTaskQueue(
      MainThreadTaskQueue::QueueType::kFrameLoadingControl, nullptr);
  scoped_refptr<MainThreadTaskQueue> timer_tq = scheduler_->NewTimerTaskQueue(
      MainThreadTaskQueue::QueueType::kFrameThrottleable, nullptr);
  scoped_refptr<MainThreadTaskQueue> unthrottled_tq =
      scheduler_->NewTaskQueue(MainThreadTaskQueue::QueueCreationParams(
          MainThreadTaskQueue::QueueType::kUnthrottled));

  EXPECT_EQ(scheduler_->DefaultTaskQueue()->GetTimeDomain(),
            scheduler_->GetVirtualTimeDomain());
  EXPECT_EQ(scheduler_->CompositorTaskQueue()->GetTimeDomain(),
            scheduler_->GetVirtualTimeDomain());
  EXPECT_EQ(loading_task_runner_->GetTimeDomain(),
            scheduler_->GetVirtualTimeDomain());
  EXPECT_EQ(timer_task_runner_->GetTimeDomain(),
            scheduler_->GetVirtualTimeDomain());
  EXPECT_EQ(scheduler_->VirtualTimeControlTaskQueue()->GetTimeDomain(),
            scheduler_->GetVirtualTimeDomain());
  EXPECT_EQ(scheduler_->V8TaskQueue()->GetTimeDomain(),
            scheduler_->GetVirtualTimeDomain());

  // The main control task queue remains in the real time domain.
  EXPECT_EQ(scheduler_->ControlTaskQueue()->GetTimeDomain(),
            scheduler_->real_time_domain());

  EXPECT_EQ(loading_tq->GetTimeDomain(), scheduler_->GetVirtualTimeDomain());
  EXPECT_EQ(loading_control_tq->GetTimeDomain(),
            scheduler_->GetVirtualTimeDomain());
  EXPECT_EQ(timer_tq->GetTimeDomain(), scheduler_->GetVirtualTimeDomain());
  EXPECT_EQ(unthrottled_tq->GetTimeDomain(),
            scheduler_->GetVirtualTimeDomain());

  EXPECT_EQ(scheduler_
                ->NewLoadingTaskQueue(
                    MainThreadTaskQueue::QueueType::kFrameLoading, nullptr)
                ->GetTimeDomain(),
            scheduler_->GetVirtualTimeDomain());
  EXPECT_EQ(scheduler_
                ->NewTimerTaskQueue(
                    MainThreadTaskQueue::QueueType::kFrameThrottleable, nullptr)
                ->GetTimeDomain(),
            scheduler_->GetVirtualTimeDomain());
  EXPECT_EQ(scheduler_
                ->NewTaskQueue(MainThreadTaskQueue::QueueCreationParams(
                    MainThreadTaskQueue::QueueType::kUnthrottled))
                ->GetTimeDomain(),
            scheduler_->GetVirtualTimeDomain());
  EXPECT_EQ(scheduler_
                ->NewTaskQueue(MainThreadTaskQueue::QueueCreationParams(
                    MainThreadTaskQueue::QueueType::kTest))
                ->GetTimeDomain(),
            scheduler_->GetVirtualTimeDomain());
}

TEST_F(MainThreadSchedulerImplTest, EnableVirtualTimeAfterThrottling) {
  std::unique_ptr<PageSchedulerImpl> page_scheduler = base::WrapUnique(
      new PageSchedulerImpl(nullptr, scheduler_.get(),
                            false /* disable_background_timer_throttling */));
  scheduler_->AddPageScheduler(page_scheduler.get());

  std::unique_ptr<FrameSchedulerImpl> frame_scheduler =
      page_scheduler->CreateFrameSchedulerImpl(
          nullptr, FrameScheduler::FrameType::kSubframe);

  TaskQueue* timer_tq = ThrottableTaskQueue(frame_scheduler.get()).get();

  frame_scheduler->SetCrossOrigin(true);
  frame_scheduler->SetFrameVisible(false);
  EXPECT_TRUE(scheduler_->task_queue_throttler()->IsThrottled(timer_tq));

  scheduler_->EnableVirtualTime(
      MainThreadSchedulerImpl::BaseTimeOverridePolicy::DO_NOT_OVERRIDE);
  EXPECT_EQ(timer_tq->GetTimeDomain(), scheduler_->GetVirtualTimeDomain());
  EXPECT_FALSE(scheduler_->task_queue_throttler()->IsThrottled(timer_tq));
}

TEST_F(MainThreadSchedulerImplTest, DisableVirtualTimeForTesting) {
  scheduler_->EnableVirtualTime(
      MainThreadSchedulerImpl::BaseTimeOverridePolicy::DO_NOT_OVERRIDE);

  scoped_refptr<MainThreadTaskQueue> timer_tq = scheduler_->NewTimerTaskQueue(
      MainThreadTaskQueue::QueueType::kFrameThrottleable, nullptr);
  scoped_refptr<MainThreadTaskQueue> unthrottled_tq =
      scheduler_->NewTaskQueue(MainThreadTaskQueue::QueueCreationParams(
          MainThreadTaskQueue::QueueType::kUnthrottled));

  scheduler_->DisableVirtualTimeForTesting();
  EXPECT_EQ(scheduler_->DefaultTaskQueue()->GetTimeDomain(),
            scheduler_->real_time_domain());
  EXPECT_EQ(scheduler_->CompositorTaskQueue()->GetTimeDomain(),
            scheduler_->real_time_domain());
  EXPECT_EQ(loading_task_runner_->GetTimeDomain(),
            scheduler_->real_time_domain());
  EXPECT_EQ(timer_task_runner_->GetTimeDomain(),
            scheduler_->real_time_domain());
  EXPECT_EQ(scheduler_->ControlTaskQueue()->GetTimeDomain(),
            scheduler_->real_time_domain());
  EXPECT_EQ(scheduler_->V8TaskQueue()->GetTimeDomain(),
            scheduler_->real_time_domain());
  EXPECT_FALSE(scheduler_->VirtualTimeControlTaskQueue());
}

TEST_F(MainThreadSchedulerImplTest, VirtualTimePauser) {
  scheduler_->EnableVirtualTime(
      MainThreadSchedulerImpl::BaseTimeOverridePolicy::DO_NOT_OVERRIDE);
  scheduler_->SetVirtualTimePolicy(
      PageSchedulerImpl::VirtualTimePolicy::kDeterministicLoading);

  WebScopedVirtualTimePauser pauser =
      scheduler_->CreateWebScopedVirtualTimePauser(
          "test", WebScopedVirtualTimePauser::VirtualTaskDuration::kInstant);

  base::TimeTicks before = scheduler_->GetVirtualTimeDomain()->Now();
  EXPECT_TRUE(scheduler_->VirtualTimeAllowedToAdvance());
  pauser.PauseVirtualTime();
  EXPECT_FALSE(scheduler_->VirtualTimeAllowedToAdvance());

  pauser.UnpauseVirtualTime();
  EXPECT_TRUE(scheduler_->VirtualTimeAllowedToAdvance());
  base::TimeTicks after = scheduler_->GetVirtualTimeDomain()->Now();
  EXPECT_EQ(after, before);
}

TEST_F(MainThreadSchedulerImplTest, VirtualTimePauserNonInstantTask) {
  scheduler_->EnableVirtualTime(
      MainThreadSchedulerImpl::BaseTimeOverridePolicy::DO_NOT_OVERRIDE);
  scheduler_->SetVirtualTimePolicy(
      PageSchedulerImpl::VirtualTimePolicy::kDeterministicLoading);

  WebScopedVirtualTimePauser pauser =
      scheduler_->CreateWebScopedVirtualTimePauser(
          "test", WebScopedVirtualTimePauser::VirtualTaskDuration::kNonInstant);

  base::TimeTicks before = scheduler_->GetVirtualTimeDomain()->Now();
  pauser.PauseVirtualTime();
  pauser.UnpauseVirtualTime();
  base::TimeTicks after = scheduler_->GetVirtualTimeDomain()->Now();
  EXPECT_GT(after, before);
}

TEST_F(MainThreadSchedulerImplTest, Tracing) {
  // This test sets renderer scheduler to some non-trivial state
  // (by posting tasks, creating child schedulers, etc) and converts it into a
  // traced value. This test checks that no internal checks fire during this.

  std::unique_ptr<PageSchedulerImpl> page_scheduler1 =
      base::WrapUnique(new PageSchedulerImpl(nullptr, scheduler_.get(), false));
  scheduler_->AddPageScheduler(page_scheduler1.get());

  std::unique_ptr<FrameSchedulerImpl> frame_scheduler =
      page_scheduler1->CreateFrameSchedulerImpl(
          nullptr, FrameScheduler::FrameType::kSubframe);

  std::unique_ptr<PageSchedulerImpl> page_scheduler2 =
      base::WrapUnique(new PageSchedulerImpl(nullptr, scheduler_.get(), false));
  scheduler_->AddPageScheduler(page_scheduler2.get());

  CPUTimeBudgetPool* time_budget_pool =
      scheduler_->task_queue_throttler()->CreateCPUTimeBudgetPool("test");

  time_budget_pool->AddQueue(base::TimeTicks(), timer_task_runner_.get());

  timer_task_runner_->PostTask(FROM_HERE, base::BindOnce(NullTask));

  loading_task_runner_->PostDelayedTask(FROM_HERE, base::BindOnce(NullTask),
                                        base::TimeDelta::FromMilliseconds(10));

  std::unique_ptr<base::trace_event::ConvertableToTraceFormat> value =
      scheduler_->AsValue(base::TimeTicks());
  EXPECT_TRUE(value);
}

void RecordingTimeTestTask(std::vector<base::TimeTicks>* run_times,
                           base::SimpleTestTickClock* clock) {
  run_times->push_back(clock->NowTicks());
}

// TODO(altimin@): Re-enable after splitting the timer policy into separate
// policies.
TEST_F(MainThreadSchedulerImplTest,
       DISABLED_DefaultTimerTasksAreThrottledWhenBackgrounded) {
  ScopedAutoAdvanceNowEnabler enable_auto_advance_now(mock_task_runner_);

  scheduler_->SetRendererBackgrounded(true);

  std::vector<base::TimeTicks> run_times;

  timer_task_runner_->PostTask(
      FROM_HERE, base::BindOnce(&RecordingTimeTestTask, &run_times, &clock_));

  mock_task_runner_->RunUntilTime(base::TimeTicks() +
                                  base::TimeDelta::FromMilliseconds(1100));

  EXPECT_THAT(run_times, testing::ElementsAre(base::TimeTicks() +
                                              base::TimeDelta::FromSeconds(1)));
  run_times.clear();

  timer_task_runner_->PostDelayedTask(
      FROM_HERE, base::BindOnce(&RecordingTimeTestTask, &run_times, &clock_),
      base::TimeDelta::FromMilliseconds(200));

  scheduler_->SetRendererBackgrounded(false);

  mock_task_runner_->RunUntilTime(base::TimeTicks() +
                                  base::TimeDelta::FromMilliseconds(1500));

  EXPECT_THAT(run_times,
              testing::ElementsAre(base::TimeTicks() +
                                   base::TimeDelta::FromMilliseconds(1300)));
}

//                  Nav Start     Nav Start            assert
//                     |             |                   |
//                     v             v                   v
//    ------------------------------------------------------------>
//     |---long task---|---1s task---|-----long task ----|
//
//                     (---MaxEQT1---)
//                                   (---MaxEQT2---)
//
// --- EQT untracked---|             |---EQT unflushed-----
//
// MaxEQT1 = 500ms is recorded and observed in histogram.
// MaxEQT2 is recorded but not yet in histogram for not being flushed.
TEST_F(MainThreadSchedulerImplTest,
       MaxQueueingTimeMetricRecordedOnlyDuringNavigation) {
  base::HistogramTester tester;
  // Start with a long task whose queueing time will be ignored.
  AdvanceTimeWithTask(10);
  // Navigation start.
  scheduler_->DidCommitProvisionalLoad(false, false, false);
  // The max queueing time of the following task will be recorded.
  AdvanceTimeWithTask(1);
  // The smaller queuing time will be ignored.
  AdvanceTimeWithTask(0.5);
  scheduler_->DidCommitProvisionalLoad(false, false, false);
  // Add another long task after navigation start but without navigation end.
  // This value won't be recorded as there is not navigation.
  AdvanceTimeWithTask(10);
  // The expected queueing time of 1s task in 1s window is 500ms.
  tester.ExpectUniqueSample("RendererScheduler.MaxQueueingTime", 500, 1);
}

// Only the max of all the queueing times is recorded.
TEST_F(MainThreadSchedulerImplTest, MaxQueueingTimeMetricRecordTheMax) {
  base::HistogramTester tester;
  scheduler_->DidCommitProvisionalLoad(false, false, false);
  // The smaller queuing time will be ignored.
  AdvanceTimeWithTask(0.5);
  // The max queueing time of the following task will be recorded.
  AdvanceTimeWithTask(1);
  // The smaller queuing time will be ignored.
  AdvanceTimeWithTask(0.5);
  scheduler_->DidCommitProvisionalLoad(false, false, false);
  tester.ExpectUniqueSample("RendererScheduler.MaxQueueingTime", 500, 1);
}

TEST_F(MainThreadSchedulerImplTest, DidCommitProvisionalLoad) {
  scheduler_->OnFirstMeaningfulPaint();
  EXPECT_FALSE(scheduler_->waiting_for_meaningful_paint());

  // Check that we only clear state for main frame navigations that are either
  // not history inert or are reloads.
  scheduler_->DidCommitProvisionalLoad(false /* is_web_history_inert_commit */,
                                       false /* is_reload */,
                                       false /* is_main_frame */);
  EXPECT_FALSE(scheduler_->waiting_for_meaningful_paint());

  scheduler_->OnFirstMeaningfulPaint();
  scheduler_->DidCommitProvisionalLoad(false /* is_web_history_inert_commit */,
                                       false /* is_reload */,
                                       true /* is_main_frame */);
  EXPECT_TRUE(scheduler_->waiting_for_meaningful_paint());  // State cleared.

  scheduler_->OnFirstMeaningfulPaint();
  scheduler_->DidCommitProvisionalLoad(false /* is_web_history_inert_commit */,
                                       true /* is_reload */,
                                       false /* is_main_frame */);
  EXPECT_FALSE(scheduler_->waiting_for_meaningful_paint());

  scheduler_->OnFirstMeaningfulPaint();
  scheduler_->DidCommitProvisionalLoad(false /* is_web_history_inert_commit */,
                                       true /* is_reload */,
                                       true /* is_main_frame */);
  EXPECT_TRUE(scheduler_->waiting_for_meaningful_paint());  // State cleared.

  scheduler_->OnFirstMeaningfulPaint();
  scheduler_->DidCommitProvisionalLoad(true /* is_web_history_inert_commit */,
                                       false /* is_reload */,
                                       false /* is_main_frame */);
  EXPECT_FALSE(scheduler_->waiting_for_meaningful_paint());

  scheduler_->OnFirstMeaningfulPaint();
  scheduler_->DidCommitProvisionalLoad(true /* is_web_history_inert_commit */,
                                       false /* is_reload */,
                                       true /* is_main_frame */);
  EXPECT_FALSE(scheduler_->waiting_for_meaningful_paint());

  scheduler_->OnFirstMeaningfulPaint();
  scheduler_->DidCommitProvisionalLoad(true /* is_web_history_inert_commit */,
                                       true /* is_reload */,
                                       false /* is_main_frame */);
  EXPECT_FALSE(scheduler_->waiting_for_meaningful_paint());

  scheduler_->OnFirstMeaningfulPaint();
  scheduler_->DidCommitProvisionalLoad(true /* is_web_history_inert_commit */,
                                       true /* is_reload */,
                                       true /* is_main_frame */);
  EXPECT_TRUE(scheduler_->waiting_for_meaningful_paint());  // State cleared.
}

TEST_F(MainThreadSchedulerImplTest, LoadingControlTasks) {
  // Expect control loading tasks (M) to jump ahead of any regular loading
  // tasks (L).
  std::vector<std::string> run_order;
  PostTestTasks(&run_order, "L1 L2 M1 L3 L4 M2 L5 L6");
  RunUntilIdle();
  EXPECT_THAT(run_order,
              testing::ElementsAre(std::string("M1"), std::string("M2"),
                                   std::string("L1"), std::string("L2"),
                                   std::string("L3"), std::string("L4"),
                                   std::string("L5"), std::string("L6")));
}

TEST_F(MainThreadSchedulerImplTest, RequestBeginMainFrameNotExpected) {
  std::unique_ptr<PageSchedulerImplForTest> page_scheduler =
      std::make_unique<PageSchedulerImplForTest>(scheduler_.get());
  scheduler_->AddPageScheduler(page_scheduler.get());

  scheduler_->OnPendingTasksChanged(true);
  EXPECT_CALL(*page_scheduler, RequestBeginMainFrameNotExpected(true)).Times(1);
  RunUntilIdle();

  Mock::VerifyAndClearExpectations(page_scheduler.get());

  scheduler_->OnPendingTasksChanged(false);
  EXPECT_CALL(*page_scheduler, RequestBeginMainFrameNotExpected(false))
      .Times(1);
  RunUntilIdle();

  Mock::VerifyAndClearExpectations(page_scheduler.get());
}

TEST_F(MainThreadSchedulerImplTest,
       RequestBeginMainFrameNotExpected_MultipleCalls) {
  std::unique_ptr<PageSchedulerImplForTest> page_scheduler =
      std::make_unique<PageSchedulerImplForTest>(scheduler_.get());
  scheduler_->AddPageScheduler(page_scheduler.get());

  scheduler_->OnPendingTasksChanged(true);
  scheduler_->OnPendingTasksChanged(true);
  // Multiple calls should result in only one call.
  EXPECT_CALL(*page_scheduler, RequestBeginMainFrameNotExpected(true)).Times(1);
  RunUntilIdle();

  Mock::VerifyAndClearExpectations(page_scheduler.get());
}

#if defined(OS_ANDROID)
TEST_F(MainThreadSchedulerImplTest, PauseTimersForAndroidWebView) {
  ScopedAutoAdvanceNowEnabler enable_auto_advance_now(mock_task_runner_);
  // Tasks in some queues don't fire when the timers are paused.
  std::vector<std::string> run_order;
  PostTestTasks(&run_order, "D1 C1 L1 I1 T1");
  scheduler_->PauseTimersForAndroidWebView();
  EnableIdleTasks();
  RunUntilIdle();
  EXPECT_THAT(run_order,
              testing::ElementsAre(std::string("D1"), std::string("C1"),
                                   std::string("L1"), std::string("I1")));
  // The rest queued tasks fire when the timers are resumed.
  run_order.clear();
  scheduler_->ResumeTimersForAndroidWebView();
  RunUntilIdle();
  EXPECT_THAT(run_order, testing::ElementsAre(std::string("T1")));
}
#endif  // defined(OS_ANDROID)

class MainThreadSchedulerImplWithInitalVirtualTimeTest
    : public MainThreadSchedulerImplTest {
 public:
  void SetUp() override {
    if (!message_loop_) {
      mock_task_runner_ =
          base::MakeRefCounted<cc::OrderedSimpleTaskRunner>(&clock_, false);
    }
    Initialize(std::make_unique<MainThreadSchedulerImplForTest>(
        base::sequence_manager::TaskQueueManagerForTest::Create(
            message_loop_.get(),
            message_loop_ ? message_loop_->task_runner() : mock_task_runner_,
            &clock_),
        base::Time::FromJsTime(1000000.0)));
  }
};

TEST_F(MainThreadSchedulerImplWithInitalVirtualTimeTest, VirtualTimeOverride) {
  EXPECT_TRUE(scheduler_->IsVirtualTimeEnabled());
  EXPECT_EQ(PageSchedulerImpl::VirtualTimePolicy::kPause,
            scheduler_->virtual_time_policy());
  EXPECT_EQ(base::Time::Now(), base::Time::FromJsTime(1000000.0));
}

TEST_F(MainThreadSchedulerImplTest, ShouldIgnoreTaskForUkm) {
  bool supports_thread_ticks = base::ThreadTicks::IsSupported();
  double sampling_rate;

  sampling_rate = 0.0001;
  EXPECT_FALSE(scheduler_->ShouldIgnoreTaskForUkm(true, &sampling_rate));
  if (supports_thread_ticks) {
    EXPECT_EQ(0.01, sampling_rate);
  } else {
    EXPECT_EQ(0.0001, sampling_rate);
  }

  sampling_rate = 0.0001;
  if (supports_thread_ticks) {
    EXPECT_TRUE(scheduler_->ShouldIgnoreTaskForUkm(false, &sampling_rate));
  } else {
    EXPECT_FALSE(scheduler_->ShouldIgnoreTaskForUkm(false, &sampling_rate));
    EXPECT_EQ(0.0001, sampling_rate);
  }
}

}  // namespace main_thread_scheduler_impl_unittest
}  // namespace scheduler
}  // namespace blink
