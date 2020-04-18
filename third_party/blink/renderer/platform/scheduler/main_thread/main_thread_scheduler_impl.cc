// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/platform/scheduler/main_thread/main_thread_scheduler_impl.h"

#include <algorithm>
#include <utility>

#include "base/bind.h"
#include "base/debug/stack_trace.h"
#include "base/feature_list.h"
#include "base/logging.h"
#include "base/metrics/field_trial_params.h"
#include "base/metrics/histogram.h"
#include "base/metrics/histogram_functions.h"
#include "base/metrics/histogram_macros.h"
#include "base/strings/string_number_conversions.h"
#include "base/threading/thread_task_runner_handle.h"
#include "base/trace_event/trace_event.h"
#include "base/trace_event/trace_event_argument.h"
#include "build/build_config.h"
#include "components/viz/common/frame_sinks/begin_frame_args.h"
#include "services/metrics/public/cpp/ukm_builders.h"
#include "third_party/blink/public/common/page/launching_process_state.h"
#include "third_party/blink/public/platform/platform.h"
#include "third_party/blink/public/platform/scheduler/renderer_process_type.h"
#include "third_party/blink/renderer/platform/instrumentation/resource_coordinator/blink_resource_coordinator_base.h"
#include "third_party/blink/renderer/platform/instrumentation/resource_coordinator/renderer_resource_coordinator.h"
#include "third_party/blink/renderer/platform/runtime_enabled_features.h"
#include "third_party/blink/renderer/platform/scheduler/base/real_time_domain.h"
#include "third_party/blink/renderer/platform/scheduler/base/task_queue_impl.h"
#include "third_party/blink/renderer/platform/scheduler/base/task_queue_selector.h"
#include "third_party/blink/renderer/platform/scheduler/base/virtual_time_domain.h"
#include "third_party/blink/renderer/platform/scheduler/child/features.h"
#include "third_party/blink/renderer/platform/scheduler/child/process_state.h"
#include "third_party/blink/renderer/platform/scheduler/common/throttling/task_queue_throttler.h"
#include "third_party/blink/renderer/platform/scheduler/main_thread/auto_advancing_virtual_time_domain.h"
#include "third_party/blink/renderer/platform/scheduler/main_thread/page_scheduler_impl.h"
#include "third_party/blink/renderer/platform/scheduler/renderer/webthread_impl_for_renderer_scheduler.h"

namespace blink {
namespace scheduler {

using base::sequence_manager::TaskQueue;
using base::sequence_manager::TaskTimeObserver;
using base::sequence_manager::TimeDomain;

namespace {
// The run time of loading tasks is strongly bimodal.  The vast majority are
// very cheap, but there are usually a handful of very expensive tasks (e.g ~1
// second on a mobile device) so we take a very pessimistic view when estimating
// the cost of loading tasks.
const int kLoadingTaskEstimationSampleCount = 1000;
const double kLoadingTaskEstimationPercentile = 99;
const int kTimerTaskEstimationSampleCount = 1000;
const double kTimerTaskEstimationPercentile = 99;
const int kShortIdlePeriodDurationSampleCount = 10;
const double kShortIdlePeriodDurationPercentile = 50;
// Amount of idle time left in a frame (as a ratio of the vsync interval) above
// which main thread compositing can be considered fast.
const double kFastCompositingIdleTimeThreshold = .2;
constexpr base::TimeDelta kQueueingTimeWindowDuration =
    base::TimeDelta::FromSeconds(1);
const double kSamplingRateForTaskUkm = 0.0001;

// Field trial name.
const char kWakeUpThrottlingTrial[] = "RendererSchedulerWakeUpThrottling";
const char kWakeUpDurationParam[] = "wake_up_duration_ms";

constexpr base::TimeDelta kDefaultWakeUpDuration =
    base::TimeDelta::FromMilliseconds(3);

base::TimeDelta GetWakeUpDuration() {
  int duration_ms;
  if (!base::StringToInt(base::GetFieldTrialParamValue(kWakeUpThrottlingTrial,
                                                       kWakeUpDurationParam),
                         &duration_ms))
    return kDefaultWakeUpDuration;
  return base::TimeDelta::FromMilliseconds(duration_ms);
}

const char* BackgroundStateToString(bool is_backgrounded) {
  if (is_backgrounded) {
    return "renderer_backgrounded";
  } else {
    return "renderer_visible";
  }
}

const char* HiddenStateToString(bool is_hidden) {
  if (is_hidden) {
    return "hidden";
  } else {
    return "visible";
  }
}

const char* AudioPlayingStateToString(bool is_audio_playing) {
  if (is_audio_playing) {
    return "playing";
  } else {
    return "silent";
  }
}

const char* RendererProcessTypeToString(RendererProcessType process_type) {
  switch (process_type) {
    case RendererProcessType::kRenderer:
      return "normal";
    case RendererProcessType::kExtensionRenderer:
      return "extension";
  }
  NOTREACHED();
  return "";  // MSVC needs that.
}

const char* TaskTypeToString(TaskType task_type) {
  switch (task_type) {
    case TaskType::kDeprecatedNone:
      return "None";
    case TaskType::kDOMManipulation:
      return "DOMManipultion";
    case TaskType::kUserInteraction:
      return "UserInteraction";
    case TaskType::kNetworking:
      return "Networking";
    case TaskType::kNetworkingControl:
      return "NetworkingControl";
    case TaskType::kHistoryTraversal:
      return "HistoryTraversal";
    case TaskType::kEmbed:
      return "Embed";
    case TaskType::kMediaElementEvent:
      return "MediaElementEvent";
    case TaskType::kCanvasBlobSerialization:
      return "CanvasBlobSerialization";
    case TaskType::kMicrotask:
      return "Microtask";
    case TaskType::kJavascriptTimer:
      return "JavascriptTimer";
    case TaskType::kRemoteEvent:
      return "RemoteEvent";
    case TaskType::kWebSocket:
      return "WebSocket";
    case TaskType::kPostedMessage:
      return "PostedMessage";
    case TaskType::kUnshippedPortMessage:
      return "UnshipedPortMessage";
    case TaskType::kFileReading:
      return "FileReading";
    case TaskType::kDatabaseAccess:
      return "DatabaseAccess";
    case TaskType::kPresentation:
      return "Presentation";
    case TaskType::kSensor:
      return "Sensor";
    case TaskType::kPerformanceTimeline:
      return "PerformanceTimeline";
    case TaskType::kWebGL:
      return "WebGL";
    case TaskType::kIdleTask:
      return "IdleTask";
    case TaskType::kMiscPlatformAPI:
      return "MiscPlatformAPI";
    case TaskType::kInternalDefault:
      return "InternalDefault";
    case TaskType::kInternalLoading:
      return "InternalLoading";
    case TaskType::kUnthrottled:
      return "Unthrottled";
    case TaskType::kInternalTest:
      return "InternalTest";
    case TaskType::kInternalWebCrypto:
      return "InternalWebCrypto";
    case TaskType::kInternalIndexedDB:
      return "InternalIndexedDB";
    case TaskType::kInternalMedia:
      return "InternalMedia";
    case TaskType::kInternalMediaRealTime:
      return "InternalMediaRealTime";
    case TaskType::kInternalIPC:
      return "InternalIPC";
    case TaskType::kInternalUserInteraction:
      return "InternalUserInteraction";
    case TaskType::kInternalInspector:
      return "InternalInspector";
    case TaskType::kInternalWorker:
      return "InternalWorker";
    case TaskType::kMainThreadTaskQueueV8:
      return "MainThreadTaskQueueV8";
    case TaskType::kMainThreadTaskQueueCompositor:
      return "MainThreadTaskQueueCompositor";
    case TaskType::kMainThreadTaskQueueDefault:
      return "MainThreadTaskQueueDefault";
    case TaskType::kMainThreadTaskQueueInput:
      return "MainThreadTaskQueueInput";
    case TaskType::kMainThreadTaskQueueIdle:
      return "MainThreadTaskQueueIdle";
    case TaskType::kMainThreadTaskQueueIPC:
      return "MainThreadTaskQueueIPC";
    case TaskType::kMainThreadTaskQueueControl:
      return "MainThreadTaskQueueControl";
    case TaskType::kInternalIntersectionObserver:
      return "InternalIntersectionObserver";
    case TaskType::kCompositorThreadTaskQueueDefault:
      return "CompositorThreadTaskQueueDefault";
    case TaskType::kWorkerThreadTaskQueueDefault:
      return "WorkerThreadTaskQueueDefault";
    case TaskType::kCount:
      return "Count";
  }
  NOTREACHED();
  return "";
}

const char* OptionalTaskDescriptionToString(
    base::Optional<MainThreadSchedulerImpl::TaskDescriptionForTracing> desc) {
  if (!desc)
    return nullptr;
  if (desc->task_type != TaskType::kDeprecatedNone)
    return TaskTypeToString(desc->task_type);
  if (!desc->queue_type)
    return "detached_tq";
  return MainThreadTaskQueue::NameForQueueType(desc->queue_type.value());
}

const char* OptionalTaskPriorityToString(
    base::Optional<TaskQueue::QueuePriority> priority) {
  if (!priority)
    return nullptr;
  return TaskQueue::PriorityToString(priority.value());
}

bool IsUnconditionalHighPriorityInputEnabled() {
  return base::FeatureList::IsEnabled(kHighPriorityInput);
}

}  // namespace

MainThreadSchedulerImpl::MainThreadSchedulerImpl(
    std::unique_ptr<base::sequence_manager::TaskQueueManager>
        task_queue_manager,
    base::Optional<base::Time> initial_virtual_time)
    : helper_(std::move(task_queue_manager), this),
      idle_helper_(&helper_,
                   this,
                   "MainThreadSchedulerIdlePeriod",
                   base::TimeDelta(),
                   helper_.NewTaskQueue(
                       MainThreadTaskQueue::QueueCreationParams(
                           MainThreadTaskQueue::QueueType::kIdle)
                           .SetFixedPriority(
                               TaskQueue::QueuePriority::kBestEffortPriority))),
      idle_canceled_delayed_task_sweeper_(&helper_,
                                          idle_helper_.IdleTaskRunner()),
      render_widget_scheduler_signals_(this),
      control_task_queue_(helper_.ControlMainThreadTaskQueue()),
      compositor_task_queue_(
          helper_.NewTaskQueue(MainThreadTaskQueue::QueueCreationParams(
                                   MainThreadTaskQueue::QueueType::kCompositor)
                                   .SetShouldMonitorQuiescence(true))),
      input_task_queue_(helper_.NewTaskQueue(
          MainThreadTaskQueue::QueueCreationParams(
              MainThreadTaskQueue::QueueType::kInput)
              .SetShouldMonitorQuiescence(true)
              .SetFixedPriority(
                  IsUnconditionalHighPriorityInputEnabled()
                      ? base::make_optional(
                            TaskQueue::QueuePriority::kHighestPriority)
                      : base::nullopt))),
      compositor_task_queue_enabled_voter_(
          compositor_task_queue_->CreateQueueEnabledVoter()),
      input_task_queue_enabled_voter_(
          input_task_queue_->CreateQueueEnabledVoter()),
      delayed_update_policy_runner_(
          base::BindRepeating(&MainThreadSchedulerImpl::UpdatePolicy,
                              base::Unretained(this)),
          TaskQueueWithTaskType::Create(helper_.ControlMainThreadTaskQueue(),
                                        TaskType::kMainThreadTaskQueueControl)),
      queueing_time_estimator_(this, kQueueingTimeWindowDuration, 20),
      main_thread_only_(this,
                        compositor_task_queue_,
                        helper_.GetClock(),
                        helper_.NowTicks()),
      any_thread_(this),
      policy_may_need_update_(&any_thread_lock_),
      weak_factory_(this) {
  task_queue_throttler_.reset(
      new TaskQueueThrottler(this, &tracing_controller_));
  update_policy_closure_ = base::BindRepeating(
      &MainThreadSchedulerImpl::UpdatePolicy, weak_factory_.GetWeakPtr());
  end_renderer_hidden_idle_period_closure_.Reset(base::BindRepeating(
      &MainThreadSchedulerImpl::EndIdlePeriod, weak_factory_.GetWeakPtr()));

  // Compositor task queue and default task queue should be managed by
  // WebMainThreadScheduler. Control task queue should not.
  task_runners_.insert(
      std::make_pair(helper_.DefaultMainThreadTaskQueue(), nullptr));
  task_runners_.insert(
      std::make_pair(compositor_task_queue_,
                     compositor_task_queue_->CreateQueueEnabledVoter()));
  task_runners_.insert(std::make_pair(
      input_task_queue_, input_task_queue_->CreateQueueEnabledVoter()));

  v8_task_queue_ = NewTaskQueue(MainThreadTaskQueue::QueueCreationParams(
      MainThreadTaskQueue::QueueType::kV8));
  ipc_task_queue_ = NewTaskQueue(MainThreadTaskQueue::QueueCreationParams(
      MainThreadTaskQueue::QueueType::kIPC));

  v8_task_runner_ = TaskQueueWithTaskType::Create(
      v8_task_queue_, TaskType::kMainThreadTaskQueueV8);
  compositor_task_runner_ = TaskQueueWithTaskType::Create(
      compositor_task_queue_, TaskType::kMainThreadTaskQueueCompositor);

  TRACE_EVENT_OBJECT_CREATED_WITH_ID(
      TRACE_DISABLED_BY_DEFAULT("renderer.scheduler"), "MainThreadScheduler",
      this);

  helper_.SetObserver(this);

  // Register a tracing state observer unless we're running in a test without a
  // task runner. Note that it's safe to remove a non-existent observer.
  if (base::ThreadTaskRunnerHandle::IsSet()) {
    base::trace_event::TraceLog::GetInstance()->AddAsyncEnabledStateObserver(
        weak_factory_.GetWeakPtr());
  }

  int32_t delay_for_background_tab_freezing_millis;
  if (!base::StringToInt(
          base::GetFieldTrialParamValue("BackgroundTabFreezing",
                                        "DelayForBackgroundTabFreezingMills"),
          &delay_for_background_tab_freezing_millis)) {
    delay_for_background_tab_freezing_millis =
        kDelayForBackgroundTabFreezingMillis;
  }
  delay_for_background_tab_freezing_ = base::TimeDelta::FromMilliseconds(
      delay_for_background_tab_freezing_millis);

  internal::ProcessState::Get()->is_process_backgrounded =
      main_thread_only().renderer_backgrounded;

  if (initial_virtual_time) {
    main_thread_only().initial_virtual_time = *initial_virtual_time;
    // The real uptime of the machine is irrelevant if we're using virtual time
    // we choose an arbitrary initial offset.
    main_thread_only().initial_virtual_time_ticks =
        base::TimeTicks() + base::TimeDelta::FromSeconds(10);
    EnableVirtualTime(BaseTimeOverridePolicy::OVERRIDE);
    SetVirtualTimePolicy(VirtualTimePolicy::kPause);
  }
}

MainThreadSchedulerImpl::~MainThreadSchedulerImpl() {
  TRACE_EVENT_OBJECT_DELETED_WITH_ID(
      TRACE_DISABLED_BY_DEFAULT("renderer.scheduler"), "MainThreadScheduler",
      this);

  for (auto& pair : task_runners_) {
    TaskCostEstimator* observer = nullptr;
    switch (pair.first->queue_class()) {
      case MainThreadTaskQueue::QueueClass::kLoading:
        observer = &main_thread_only().loading_task_cost_estimator;
        break;
      case MainThreadTaskQueue::QueueClass::kTimer:
        observer = &main_thread_only().timer_task_cost_estimator;
        break;
      default:
        observer = nullptr;
    }

    if (observer)
      pair.first->RemoveTaskObserver(observer);

    pair.first->ShutdownTaskQueue();
  }

  if (virtual_time_domain_)
    UnregisterTimeDomain(virtual_time_domain_.get());

  if (virtual_time_control_task_queue_)
    virtual_time_control_task_queue_->ShutdownTaskQueue();

  base::trace_event::TraceLog::GetInstance()->RemoveAsyncEnabledStateObserver(
      this);

  // Ensure the renderer scheduler was shut down explicitly, because otherwise
  // we could end up having stale pointers to the Blink heap which has been
  // terminated by this point.
  DCHECK(was_shutdown_);
}

MainThreadSchedulerImpl::MainThreadOnly::MainThreadOnly(
    MainThreadSchedulerImpl* main_thread_scheduler_impl,
    const scoped_refptr<MainThreadTaskQueue>& compositor_task_runner,
    const base::TickClock* time_source,
    base::TimeTicks now)
    : loading_task_cost_estimator(time_source,
                                  kLoadingTaskEstimationSampleCount,
                                  kLoadingTaskEstimationPercentile),
      timer_task_cost_estimator(time_source,
                                kTimerTaskEstimationSampleCount,
                                kTimerTaskEstimationPercentile),
      idle_time_estimator(compositor_task_runner,
                          time_source,
                          kShortIdlePeriodDurationSampleCount,
                          kShortIdlePeriodDurationPercentile),
      current_use_case(UseCase::kNone,
                       "Scheduler.UseCase",
                       main_thread_scheduler_impl,
                       &main_thread_scheduler_impl->tracing_controller_,
                       UseCaseToString),
      longest_jank_free_task_duration(
          base::TimeDelta(),
          "Scheduler.LongestJankFreeTaskDuration",
          main_thread_scheduler_impl,
          &main_thread_scheduler_impl->tracing_controller_,
          TimeDeltaToMilliseconds),
      renderer_pause_count(0,
                           "Scheduler.PauseCount",
                           main_thread_scheduler_impl,
                           &main_thread_scheduler_impl->tracing_controller_),
      expensive_task_policy(ExpensiveTaskPolicy::kRun,
                            "Scheduler.ExpensiveTaskPolicy",
                            main_thread_scheduler_impl,
                            &main_thread_scheduler_impl->tracing_controller_,
                            ExpensiveTaskPolicyToString),
      rail_mode_for_tracing(current_policy.rail_mode(),
                            "Scheduler.RAILMode",
                            main_thread_scheduler_impl,
                            &main_thread_scheduler_impl->tracing_controller_,
                            RAILModeToString),
      renderer_hidden(false,
                      "Scheduler.Hidden",
                      main_thread_scheduler_impl,
                      &main_thread_scheduler_impl->tracing_controller_,
                      HiddenStateToString),
      renderer_backgrounded(kLaunchingProcessIsBackgrounded,
                            "RendererVisibility",
                            main_thread_scheduler_impl,
                            &main_thread_scheduler_impl->tracing_controller_,
                            BackgroundStateToString),
      keep_active_fetch_or_worker(
          false,
          "Scheduler.KeepRendererActive",
          main_thread_scheduler_impl,
          &main_thread_scheduler_impl->tracing_controller_,
          YesNoStateToString),
      freezing_when_backgrounded_enabled(
          false,
          "MainThreadScheduler.FreezingWhenBackgroundedEnabled",
          main_thread_scheduler_impl,
          &main_thread_scheduler_impl->tracing_controller_,
          YesNoStateToString),
      frozen_when_backgrounded(false,
                               "MainThreadScheduler.FrozenWhenBackgrounded",
                               main_thread_scheduler_impl,
                               &main_thread_scheduler_impl->tracing_controller_,
                               YesNoStateToString),
      loading_task_estimated_cost(
          base::TimeDelta(),
          "Scheduler.LoadingTaskEstimatedCostMs",
          main_thread_scheduler_impl,
          &main_thread_scheduler_impl->tracing_controller_,
          TimeDeltaToMilliseconds),
      timer_task_estimated_cost(
          base::TimeDelta(),
          "Scheduler.TimerTaskEstimatedCostMs",
          main_thread_scheduler_impl,
          &main_thread_scheduler_impl->tracing_controller_,
          TimeDeltaToMilliseconds),
      loading_tasks_seem_expensive(
          false,
          "Scheduler.LoadingTasksSeemExpensive",
          main_thread_scheduler_impl,
          &main_thread_scheduler_impl->tracing_controller_,
          YesNoStateToString),
      timer_tasks_seem_expensive(
          false,
          "Scheduler.TimerTasksSeemExpensive",
          main_thread_scheduler_impl,
          &main_thread_scheduler_impl->tracing_controller_,
          YesNoStateToString),
      touchstart_expected_soon(false,
                               "Scheduler.TouchstartExpectedSoon",
                               main_thread_scheduler_impl,
                               &main_thread_scheduler_impl->tracing_controller_,
                               YesNoStateToString),
      have_seen_a_begin_main_frame(
          false,
          "Scheduler.HasSeenBeginMainFrame",
          main_thread_scheduler_impl,
          &main_thread_scheduler_impl->tracing_controller_,
          YesNoStateToString),
      have_reported_blocking_intervention_in_current_policy(
          false,
          "Scheduler.HasReportedBlockingInterventionInCurrentPolicy",
          main_thread_scheduler_impl,
          &main_thread_scheduler_impl->tracing_controller_,
          YesNoStateToString),
      have_reported_blocking_intervention_since_navigation(
          false,
          "Scheduler.HasReportedBlockingInterventionSinceNavigation",
          main_thread_scheduler_impl,
          &main_thread_scheduler_impl->tracing_controller_,
          YesNoStateToString),
      has_visible_render_widget_with_touch_handler(
          false,
          "Scheduler.HasVisibleRenderWidgetWithTouchHandler",
          main_thread_scheduler_impl,
          &main_thread_scheduler_impl->tracing_controller_,
          YesNoStateToString),
      begin_frame_not_expected_soon(
          false,
          "Scheduler.BeginFrameNotExpectedSoon",
          main_thread_scheduler_impl,
          &main_thread_scheduler_impl->tracing_controller_,
          YesNoStateToString),
      in_idle_period_for_testing(
          false,
          "Scheduler.InIdlePeriod",
          main_thread_scheduler_impl,
          &main_thread_scheduler_impl->tracing_controller_,
          YesNoStateToString),
      use_virtual_time(false,
                       "Scheduler.UseVirtualTime",
                       main_thread_scheduler_impl,
                       &main_thread_scheduler_impl->tracing_controller_,
                       YesNoStateToString),
      is_audio_playing(false,
                       "RendererAudioState",
                       main_thread_scheduler_impl,
                       &main_thread_scheduler_impl->tracing_controller_,
                       AudioPlayingStateToString),
      compositor_will_send_main_frame_not_expected(
          false,
          "Scheduler.CompositorWillSendMainFrameNotExpected",
          main_thread_scheduler_impl,
          &main_thread_scheduler_impl->tracing_controller_,
          YesNoStateToString),
      has_navigated(false,
                    "Scheduler.HasNavigated",
                    main_thread_scheduler_impl,
                    &main_thread_scheduler_impl->tracing_controller_,
                    YesNoStateToString),
      pause_timers_for_webview(false,
                               "Scheduler.PauseTimersForWebview",
                               main_thread_scheduler_impl,
                               &main_thread_scheduler_impl->tracing_controller_,
                               YesNoStateToString),
      background_status_changed_at(now),
      rail_mode_observer(nullptr),
      wake_up_budget_pool(nullptr),
      metrics_helper(main_thread_scheduler_impl, now, renderer_backgrounded),
      process_type(RendererProcessType::kRenderer,
                   "RendererProcessType",
                   main_thread_scheduler_impl,
                   &main_thread_scheduler_impl->tracing_controller_,
                   RendererProcessTypeToString),
      task_description_for_tracing(
          base::nullopt,
          "Scheduler.MainThreadTask",
          main_thread_scheduler_impl,
          &main_thread_scheduler_impl->tracing_controller_,
          OptionalTaskDescriptionToString),
      task_priority_for_tracing(
          base::nullopt,
          "Scheduler.TaskPriority",
          main_thread_scheduler_impl,
          &main_thread_scheduler_impl->tracing_controller_,
          OptionalTaskPriorityToString),
      virtual_time_policy(VirtualTimePolicy::kAdvance),
      virtual_time_pause_count(0),
      max_virtual_time_task_starvation_count(0),
      virtual_time_stopped(false),
      nested_runloop(false),
      uniform_distribution(0.0f, 1.0f) {}

MainThreadSchedulerImpl::MainThreadOnly::~MainThreadOnly() = default;

MainThreadSchedulerImpl::AnyThread::AnyThread(
    MainThreadSchedulerImpl* main_thread_scheduler_impl)
    : awaiting_touch_start_response(
          false,
          "Scheduler.AwaitingTouchstartResponse",
          main_thread_scheduler_impl,
          &main_thread_scheduler_impl->tracing_controller_,
          YesNoStateToString),
      in_idle_period(false,
                     "Scheduler.InIdlePeriod",
                     main_thread_scheduler_impl,
                     &main_thread_scheduler_impl->tracing_controller_,
                     YesNoStateToString),
      begin_main_frame_on_critical_path(
          false,
          "Scheduler.BeginMainFrameOnCriticalPath",
          main_thread_scheduler_impl,
          &main_thread_scheduler_impl->tracing_controller_,
          YesNoStateToString),
      last_gesture_was_compositor_driven(
          false,
          "Scheduler.LastGestureWasCompositorDriven",
          main_thread_scheduler_impl,
          &main_thread_scheduler_impl->tracing_controller_,
          YesNoStateToString),
      default_gesture_prevented(
          true,
          "Scheduler.DefaultGesturePrevented",
          main_thread_scheduler_impl,
          &main_thread_scheduler_impl->tracing_controller_,
          YesNoStateToString),
      have_seen_a_potentially_blocking_gesture(
          false,
          "Scheduler.HaveSeenPotentiallyBlockingGesture",
          main_thread_scheduler_impl,
          &main_thread_scheduler_impl->tracing_controller_,
          YesNoStateToString),
      waiting_for_meaningful_paint(
          false,
          "Scheduler.WaitingForMeaningfulPaint",
          main_thread_scheduler_impl,
          &main_thread_scheduler_impl->tracing_controller_,
          YesNoStateToString),
      have_seen_input_since_navigation(
          false,
          "Scheduler.HaveSeenInputSinceNavigation",
          main_thread_scheduler_impl,
          &main_thread_scheduler_impl->tracing_controller_,
          YesNoStateToString) {}

MainThreadSchedulerImpl::AnyThread::~AnyThread() = default;

MainThreadSchedulerImpl::CompositorThreadOnly::CompositorThreadOnly()
    : last_input_type(blink::WebInputEvent::kUndefined) {}

MainThreadSchedulerImpl::CompositorThreadOnly::~CompositorThreadOnly() =
    default;

MainThreadSchedulerImpl::RendererPauseHandleImpl::RendererPauseHandleImpl(
    MainThreadSchedulerImpl* scheduler)
    : scheduler_(scheduler) {
  scheduler_->PauseRendererImpl();
}

MainThreadSchedulerImpl::RendererPauseHandleImpl::~RendererPauseHandleImpl() {
  scheduler_->ResumeRendererImpl();
}

void MainThreadSchedulerImpl::Shutdown() {
  if (was_shutdown_)
    return;

  base::TimeTicks now = tick_clock()->NowTicks();
  main_thread_only().metrics_helper.OnRendererShutdown(now);

  task_queue_throttler_.reset();
  idle_helper_.Shutdown();
  helper_.Shutdown();
  main_thread_only().rail_mode_observer = nullptr;
  was_shutdown_ = true;
}

std::unique_ptr<blink::WebThread> MainThreadSchedulerImpl::CreateMainThread() {
  return std::make_unique<WebThreadImplForRendererScheduler>(this);
}

scoped_refptr<base::SingleThreadTaskRunner>
MainThreadSchedulerImpl::ControlTaskRunner() {
  return TaskQueueWithTaskType::Create(ControlTaskQueue(),
                                       TaskType::kMainThreadTaskQueueControl);
}

scoped_refptr<base::SingleThreadTaskRunner>
MainThreadSchedulerImpl::DefaultTaskRunner() {
  return TaskQueueWithTaskType::Create(helper_.DefaultMainThreadTaskQueue(),
                                       TaskType::kMainThreadTaskQueueDefault);
}

scoped_refptr<base::SingleThreadTaskRunner>
MainThreadSchedulerImpl::InputTaskRunner() {
  helper_.CheckOnValidThread();
  return TaskQueueWithTaskType::Create(input_task_queue_,
                                       TaskType::kMainThreadTaskQueueInput);
}

scoped_refptr<SingleThreadIdleTaskRunner>
MainThreadSchedulerImpl::IdleTaskRunner() {
  return idle_helper_.IdleTaskRunner();
}

scoped_refptr<base::SingleThreadTaskRunner>
MainThreadSchedulerImpl::IPCTaskRunner() {
  return TaskQueueWithTaskType::Create(ipc_task_queue_,
                                       TaskType::kMainThreadTaskQueueIPC);
}

scoped_refptr<base::SingleThreadTaskRunner>
MainThreadSchedulerImpl::VirtualTimeControlTaskRunner() {
  return virtual_time_control_task_queue_;
}

scoped_refptr<MainThreadTaskQueue>
MainThreadSchedulerImpl::CompositorTaskQueue() {
  helper_.CheckOnValidThread();
  return compositor_task_queue_;
}

scoped_refptr<MainThreadTaskQueue> MainThreadSchedulerImpl::InputTaskQueue() {
  helper_.CheckOnValidThread();
  return input_task_queue_;
}

scoped_refptr<MainThreadTaskQueue> MainThreadSchedulerImpl::V8TaskQueue() {
  helper_.CheckOnValidThread();
  return v8_task_queue_;
}

scoped_refptr<MainThreadTaskQueue> MainThreadSchedulerImpl::ControlTaskQueue() {
  return helper_.ControlMainThreadTaskQueue();
}

scoped_refptr<MainThreadTaskQueue> MainThreadSchedulerImpl::DefaultTaskQueue() {
  return helper_.DefaultMainThreadTaskQueue();
}

scoped_refptr<MainThreadTaskQueue>
MainThreadSchedulerImpl::VirtualTimeControlTaskQueue() {
  helper_.CheckOnValidThread();
  return virtual_time_control_task_queue_;
}

scoped_refptr<MainThreadTaskQueue> MainThreadSchedulerImpl::NewTaskQueue(
    const MainThreadTaskQueue::QueueCreationParams& params) {
  helper_.CheckOnValidThread();
  scoped_refptr<MainThreadTaskQueue> task_queue(helper_.NewTaskQueue(params));

  std::unique_ptr<TaskQueue::QueueEnabledVoter> voter;
  if (params.can_be_deferred || params.can_be_paused || params.can_be_frozen)
    voter = task_queue->CreateQueueEnabledVoter();

  auto insert_result =
      task_runners_.insert(std::make_pair(task_queue, std::move(voter)));
  auto queue_class = task_queue->queue_class();
  if (queue_class == MainThreadTaskQueue::QueueClass::kTimer) {
    task_queue->AddTaskObserver(&main_thread_only().timer_task_cost_estimator);
  } else if (queue_class == MainThreadTaskQueue::QueueClass::kLoading) {
    task_queue->AddTaskObserver(
        &main_thread_only().loading_task_cost_estimator);
  }

  ApplyTaskQueuePolicy(
      task_queue.get(), insert_result.first->second.get(), TaskQueuePolicy(),
      main_thread_only().current_policy.GetQueuePolicy(queue_class));

  if (task_queue->CanBeThrottled())
    AddQueueToWakeUpBudgetPool(task_queue.get());

  // If this is a timer queue, and virtual time is enabled and paused, it should
  // be suspended by adding a fence to prevent immediate tasks from running when
  // they're not supposed to.
  if (queue_class == MainThreadTaskQueue::QueueClass::kTimer &&
      main_thread_only().virtual_time_stopped &&
      main_thread_only().use_virtual_time) {
    task_queue->InsertFence(TaskQueue::InsertFencePosition::kNow);
  }

  return task_queue;
}

scoped_refptr<MainThreadTaskQueue> MainThreadSchedulerImpl::NewLoadingTaskQueue(
    MainThreadTaskQueue::QueueType queue_type,
    FrameSchedulerImpl* frame_scheduler) {
  DCHECK_EQ(MainThreadTaskQueue::QueueClassForQueueType(queue_type),
            MainThreadTaskQueue::QueueClass::kLoading);
  return NewTaskQueue(
      MainThreadTaskQueue::QueueCreationParams(queue_type)
          .SetCanBePaused(true)
          .SetCanBeFrozen(
              RuntimeEnabledFeatures::StopLoadingInBackgroundEnabled())
          .SetCanBeDeferred(true)
          .SetUsedForImportantTasks(
              queue_type ==
              MainThreadTaskQueue::QueueType::kFrameLoadingControl)
          .SetFrameScheduler(frame_scheduler));
}

scoped_refptr<MainThreadTaskQueue> MainThreadSchedulerImpl::NewTimerTaskQueue(
    MainThreadTaskQueue::QueueType queue_type,
    FrameSchedulerImpl* frame_scheduler) {
  DCHECK_EQ(MainThreadTaskQueue::QueueClassForQueueType(queue_type),
            MainThreadTaskQueue::QueueClass::kTimer);
  return NewTaskQueue(MainThreadTaskQueue::QueueCreationParams(queue_type)
                          .SetCanBePaused(true)
                          .SetCanBeFrozen(true)
                          .SetCanBeDeferred(true)
                          .SetCanBeThrottled(true)
                          .SetFrameScheduler(frame_scheduler));
}

std::unique_ptr<WebRenderWidgetSchedulingState>
MainThreadSchedulerImpl::NewRenderWidgetSchedulingState() {
  return render_widget_scheduler_signals_.NewRenderWidgetSchedulingState();
}

void MainThreadSchedulerImpl::OnShutdownTaskQueue(
    const scoped_refptr<MainThreadTaskQueue>& task_queue) {
  if (was_shutdown_)
    return;

  if (task_queue_throttler_)
    task_queue_throttler_->ShutdownTaskQueue(task_queue.get());

  if (task_runners_.erase(task_queue)) {
    switch (task_queue->queue_class()) {
      case MainThreadTaskQueue::QueueClass::kTimer:
        task_queue->RemoveTaskObserver(
            &main_thread_only().timer_task_cost_estimator);
        break;
      case MainThreadTaskQueue::QueueClass::kLoading:
        task_queue->RemoveTaskObserver(
            &main_thread_only().loading_task_cost_estimator);
        break;
      default:
        break;
    }
  }
}

bool MainThreadSchedulerImpl::CanExceedIdleDeadlineIfRequired() const {
  return idle_helper_.CanExceedIdleDeadlineIfRequired();
}

void MainThreadSchedulerImpl::AddTaskObserver(
    base::MessageLoop::TaskObserver* task_observer) {
  helper_.AddTaskObserver(task_observer);
}

void MainThreadSchedulerImpl::RemoveTaskObserver(
    base::MessageLoop::TaskObserver* task_observer) {
  helper_.RemoveTaskObserver(task_observer);
}

void MainThreadSchedulerImpl::WillBeginFrame(const viz::BeginFrameArgs& args) {
  TRACE_EVENT1(TRACE_DISABLED_BY_DEFAULT("renderer.scheduler"),
               "MainThreadSchedulerImpl::WillBeginFrame", "args",
               args.AsValue());
  helper_.CheckOnValidThread();
  if (helper_.IsShutdown())
    return;

  EndIdlePeriod();
  main_thread_only().estimated_next_frame_begin =
      args.frame_time + args.interval;
  main_thread_only().have_seen_a_begin_main_frame = true;
  main_thread_only().begin_frame_not_expected_soon = false;
  main_thread_only().compositor_frame_interval = args.interval;
  {
    base::AutoLock lock(any_thread_lock_);
    any_thread().begin_main_frame_on_critical_path = args.on_critical_path;
  }
}

void MainThreadSchedulerImpl::DidCommitFrameToCompositor() {
  TRACE_EVENT0(TRACE_DISABLED_BY_DEFAULT("renderer.scheduler"),
               "MainThreadSchedulerImpl::DidCommitFrameToCompositor");
  helper_.CheckOnValidThread();
  if (helper_.IsShutdown())
    return;

  base::TimeTicks now(helper_.NowTicks());
  if (now < main_thread_only().estimated_next_frame_begin) {
    // TODO(rmcilroy): Consider reducing the idle period based on the runtime of
    // the next pending delayed tasks (as currently done in for long idle times)
    idle_helper_.StartIdlePeriod(
        IdleHelper::IdlePeriodState::kInShortIdlePeriod, now,
        main_thread_only().estimated_next_frame_begin);
  }

  main_thread_only().idle_time_estimator.DidCommitFrameToCompositor();
}

void MainThreadSchedulerImpl::BeginFrameNotExpectedSoon() {
  TRACE_EVENT0(TRACE_DISABLED_BY_DEFAULT("renderer.scheduler"),
               "MainThreadSchedulerImpl::BeginFrameNotExpectedSoon");
  helper_.CheckOnValidThread();
  if (helper_.IsShutdown())
    return;

  main_thread_only().begin_frame_not_expected_soon = true;
  idle_helper_.EnableLongIdlePeriod();
  {
    base::AutoLock lock(any_thread_lock_);
    any_thread().begin_main_frame_on_critical_path = false;
  }
}

void MainThreadSchedulerImpl::BeginMainFrameNotExpectedUntil(
    base::TimeTicks time) {
  helper_.CheckOnValidThread();
  if (helper_.IsShutdown())
    return;

  base::TimeTicks now(helper_.NowTicks());
  TRACE_EVENT1(TRACE_DISABLED_BY_DEFAULT("renderer.scheduler"),
               "MainThreadSchedulerImpl::BeginMainFrameNotExpectedUntil",
               "time_remaining", (time - now).InMillisecondsF());

  if (now < time) {
    // End any previous idle period.
    EndIdlePeriod();

    // TODO(rmcilroy): Consider reducing the idle period based on the runtime of
    // the next pending delayed tasks (as currently done in for long idle times)
    idle_helper_.StartIdlePeriod(
        IdleHelper::IdlePeriodState::kInShortIdlePeriod, now, time);
  }
}

void MainThreadSchedulerImpl::SetAllRenderWidgetsHidden(bool hidden) {
  TRACE_EVENT1(TRACE_DISABLED_BY_DEFAULT("renderer.scheduler"),
               "MainThreadSchedulerImpl::SetAllRenderWidgetsHidden", "hidden",
               hidden);

  helper_.CheckOnValidThread();

  if (helper_.IsShutdown() || main_thread_only().renderer_hidden == hidden)
    return;

  end_renderer_hidden_idle_period_closure_.Cancel();

  if (hidden) {
    idle_helper_.EnableLongIdlePeriod();

    // Ensure that we stop running idle tasks after a few seconds of being
    // hidden.
    base::TimeDelta end_idle_when_hidden_delay =
        base::TimeDelta::FromMilliseconds(kEndIdleWhenHiddenDelayMillis);
    control_task_queue_->PostDelayedTask(
        FROM_HERE, end_renderer_hidden_idle_period_closure_.GetCallback(),
        end_idle_when_hidden_delay);
    main_thread_only().renderer_hidden = true;
  } else {
    main_thread_only().renderer_hidden = false;
    EndIdlePeriod();
  }

  // TODO(alexclarke): Should we update policy here?
  CreateTraceEventObjectSnapshot();
}

void MainThreadSchedulerImpl::SetHasVisibleRenderWidgetWithTouchHandler(
    bool has_visible_render_widget_with_touch_handler) {
  helper_.CheckOnValidThread();
  if (has_visible_render_widget_with_touch_handler ==
      main_thread_only().has_visible_render_widget_with_touch_handler)
    return;

  main_thread_only().has_visible_render_widget_with_touch_handler =
      has_visible_render_widget_with_touch_handler;

  base::AutoLock lock(any_thread_lock_);
  UpdatePolicyLocked(UpdateType::kForceUpdate);
}

void MainThreadSchedulerImpl::SetRendererHidden(bool hidden) {
  if (hidden) {
    TRACE_EVENT0(TRACE_DISABLED_BY_DEFAULT("renderer.scheduler"),
                 "MainThreadSchedulerImpl::OnRendererHidden");
  } else {
    TRACE_EVENT0(TRACE_DISABLED_BY_DEFAULT("renderer.scheduler"),
                 "MainThreadSchedulerImpl::OnRendererVisible");
  }
  helper_.CheckOnValidThread();
  main_thread_only().renderer_hidden = hidden;
}

void MainThreadSchedulerImpl::SetRendererBackgrounded(bool backgrounded) {
  helper_.CheckOnValidThread();
  if (helper_.IsShutdown() ||
      main_thread_only().renderer_backgrounded == backgrounded)
    return;
  if (backgrounded) {
    TRACE_EVENT0(TRACE_DISABLED_BY_DEFAULT("renderer.scheduler"),
                 "MainThreadSchedulerImpl::OnRendererBackgrounded");
    MainThreadMetricsHelper::RecordBackgroundedTransition(
        BackgroundedRendererTransition::kBackgrounded);
  } else {
    TRACE_EVENT0(TRACE_DISABLED_BY_DEFAULT("renderer.scheduler"),
                 "MainThreadSchedulerImpl::OnRendererForegrounded");
    MainThreadMetricsHelper::RecordBackgroundedTransition(
        BackgroundedRendererTransition::kForegrounded);
  }

  main_thread_only().renderer_backgrounded = backgrounded;
  internal::ProcessState::Get()->is_process_backgrounded = backgrounded;

  main_thread_only().background_status_changed_at = tick_clock()->NowTicks();
  queueing_time_estimator_.OnRendererStateChanged(
      backgrounded, main_thread_only().background_status_changed_at);

  UpdatePolicy();

  base::TimeTicks now = tick_clock()->NowTicks();
  if (backgrounded) {
    main_thread_only().metrics_helper.OnRendererBackgrounded(now);
  } else {
    main_thread_only().metrics_helper.OnRendererForegrounded(now);
  }
}

void MainThreadSchedulerImpl::SetSchedulerKeepActive(bool keep_active) {
  main_thread_only().keep_active_fetch_or_worker = keep_active;
  for (PageSchedulerImpl* page_scheduler : main_thread_only().page_schedulers) {
    page_scheduler->SetKeepActive(keep_active);
  }
}

bool MainThreadSchedulerImpl::SchedulerKeepActive() {
  return main_thread_only().keep_active_fetch_or_worker;
}

#if defined(OS_ANDROID)
void MainThreadSchedulerImpl::PauseTimersForAndroidWebView() {
  main_thread_only().pause_timers_for_webview = true;
  UpdatePolicy();
}

void MainThreadSchedulerImpl::ResumeTimersForAndroidWebView() {
  main_thread_only().pause_timers_for_webview = false;
  UpdatePolicy();
}
#endif

void MainThreadSchedulerImpl::OnAudioStateChanged() {
  bool is_audio_playing = false;
  for (PageSchedulerImpl* page_scheduler : main_thread_only().page_schedulers) {
    is_audio_playing = is_audio_playing || page_scheduler->IsAudioPlaying();
  }

  if (is_audio_playing == main_thread_only().is_audio_playing)
    return;

  main_thread_only().is_audio_playing = is_audio_playing;
}

std::unique_ptr<ThreadScheduler::RendererPauseHandle>
MainThreadSchedulerImpl::PauseRenderer() {
  return std::make_unique<RendererPauseHandleImpl>(this);
}

void MainThreadSchedulerImpl::PauseRendererImpl() {
  helper_.CheckOnValidThread();
  if (helper_.IsShutdown())
    return;

  ++main_thread_only().renderer_pause_count;
  UpdatePolicy();
}

void MainThreadSchedulerImpl::ResumeRendererImpl() {
  helper_.CheckOnValidThread();
  if (helper_.IsShutdown())
    return;
  --main_thread_only().renderer_pause_count;
  DCHECK_GE(main_thread_only().renderer_pause_count.value(), 0);
  UpdatePolicy();
}

void MainThreadSchedulerImpl::EndIdlePeriod() {
  if (main_thread_only().in_idle_period_for_testing)
    return;
  TRACE_EVENT0(TRACE_DISABLED_BY_DEFAULT("renderer.scheduler"),
               "MainThreadSchedulerImpl::EndIdlePeriod");
  helper_.CheckOnValidThread();
  idle_helper_.EndIdlePeriod();
}

void MainThreadSchedulerImpl::EndIdlePeriodForTesting(
    base::OnceClosure callback,
    base::TimeTicks time_remaining) {
  main_thread_only().in_idle_period_for_testing = false;
  EndIdlePeriod();
  std::move(callback).Run();
}

bool MainThreadSchedulerImpl::PolicyNeedsUpdateForTesting() {
  return policy_may_need_update_.IsSet();
}

// static
bool MainThreadSchedulerImpl::ShouldPrioritizeInputEvent(
    const blink::WebInputEvent& web_input_event) {
  // We regard MouseMove events with the left mouse button down as a signal
  // that the user is doing something requiring a smooth frame rate.
  if ((web_input_event.GetType() == blink::WebInputEvent::kMouseDown ||
       web_input_event.GetType() == blink::WebInputEvent::kMouseMove) &&
      (web_input_event.GetModifiers() &
       blink::WebInputEvent::kLeftButtonDown)) {
    return true;
  }
  // Ignore all other mouse events because they probably don't signal user
  // interaction needing a smooth framerate. NOTE isMouseEventType returns false
  // for mouse wheel events, hence we regard them as user input.
  // Ignore keyboard events because it doesn't really make sense to enter
  // compositor priority for them.
  if (blink::WebInputEvent::IsMouseEventType(web_input_event.GetType()) ||
      blink::WebInputEvent::IsKeyboardEventType(web_input_event.GetType())) {
    return false;
  }
  return true;
}

void MainThreadSchedulerImpl::DidHandleInputEventOnCompositorThread(
    const blink::WebInputEvent& web_input_event,
    InputEventState event_state) {
  TRACE_EVENT0(
      TRACE_DISABLED_BY_DEFAULT("renderer.scheduler"),
      "MainThreadSchedulerImpl::DidHandleInputEventOnCompositorThread");
  if (!ShouldPrioritizeInputEvent(web_input_event))
    return;

  UpdateForInputEventOnCompositorThread(web_input_event.GetType(), event_state);
}

void MainThreadSchedulerImpl::DidAnimateForInputOnCompositorThread() {
  TRACE_EVENT0(TRACE_DISABLED_BY_DEFAULT("renderer.scheduler"),
               "MainThreadSchedulerImpl::DidAnimateForInputOnCompositorThread");
  base::AutoLock lock(any_thread_lock_);
  any_thread().fling_compositor_escalation_deadline =
      helper_.NowTicks() +
      base::TimeDelta::FromMilliseconds(kFlingEscalationLimitMillis);
}

void MainThreadSchedulerImpl::UpdateForInputEventOnCompositorThread(
    blink::WebInputEvent::Type type,
    InputEventState input_event_state) {
  base::AutoLock lock(any_thread_lock_);
  base::TimeTicks now = helper_.NowTicks();

  // TODO(alexclarke): Move WebInputEventTraits where we can access it from here
  // and record the name rather than the integer representation.
  TRACE_EVENT2(TRACE_DISABLED_BY_DEFAULT("renderer.scheduler"),
               "MainThreadSchedulerImpl::UpdateForInputEventOnCompositorThread",
               "type", static_cast<int>(type), "input_event_state",
               InputEventStateToString(input_event_state));

  base::TimeDelta unused_policy_duration;
  UseCase previous_use_case =
      ComputeCurrentUseCase(now, &unused_policy_duration);
  bool was_awaiting_touch_start_response =
      any_thread().awaiting_touch_start_response;

  any_thread().user_model.DidStartProcessingInputEvent(type, now);
  any_thread().have_seen_input_since_navigation = true;

  if (input_event_state == InputEventState::EVENT_CONSUMED_BY_COMPOSITOR)
    any_thread().user_model.DidFinishProcessingInputEvent(now);

  switch (type) {
    case blink::WebInputEvent::kTouchStart:
      any_thread().awaiting_touch_start_response = true;
      // This is just a fail-safe to reset the state of
      // |last_gesture_was_compositor_driven| to the default. We don't know
      // yet where the gesture will run.
      any_thread().last_gesture_was_compositor_driven = false;
      any_thread().have_seen_a_potentially_blocking_gesture = true;
      // Assume the default gesture is prevented until we see evidence
      // otherwise.
      any_thread().default_gesture_prevented = true;
      break;

    case blink::WebInputEvent::kTouchMove:
      // Observation of consecutive touchmoves is a strong signal that the
      // page is consuming the touch sequence, in which case touchstart
      // response prioritization is no longer necessary. Otherwise, the
      // initial touchmove should preserve the touchstart response pending
      // state.
      if (any_thread().awaiting_touch_start_response &&
          GetCompositorThreadOnly().last_input_type ==
              blink::WebInputEvent::kTouchMove) {
        any_thread().awaiting_touch_start_response = false;
      }
      break;

    case blink::WebInputEvent::kGesturePinchUpdate:
    case blink::WebInputEvent::kGestureScrollUpdate:
      // If we see events for an established gesture, we can lock it to the
      // appropriate thread as the gesture can no longer be cancelled.
      any_thread().last_gesture_was_compositor_driven =
          input_event_state == InputEventState::EVENT_CONSUMED_BY_COMPOSITOR;
      any_thread().awaiting_touch_start_response = false;
      any_thread().default_gesture_prevented = false;
      break;

    case blink::WebInputEvent::kGestureFlingCancel:
      any_thread().fling_compositor_escalation_deadline = base::TimeTicks();
      break;

    case blink::WebInputEvent::kGestureTapDown:
    case blink::WebInputEvent::kGestureShowPress:
    case blink::WebInputEvent::kGestureScrollEnd:
      // With no observable effect, these meta events do not indicate a
      // meaningful touchstart response and should not impact task priority.
      break;

    case blink::WebInputEvent::kMouseDown:
      // Reset tracking state at the start of a new mouse drag gesture.
      any_thread().last_gesture_was_compositor_driven = false;
      any_thread().default_gesture_prevented = true;
      break;

    case blink::WebInputEvent::kMouseMove:
      // Consider mouse movement with the left button held down (see
      // ShouldPrioritizeInputEvent) similarly to a touch gesture.
      any_thread().last_gesture_was_compositor_driven =
          input_event_state == InputEventState::EVENT_CONSUMED_BY_COMPOSITOR;
      any_thread().awaiting_touch_start_response = false;
      break;

    case blink::WebInputEvent::kMouseWheel:
      any_thread().last_gesture_was_compositor_driven =
          input_event_state == InputEventState::EVENT_CONSUMED_BY_COMPOSITOR;
      any_thread().awaiting_touch_start_response = false;
      any_thread().have_seen_a_potentially_blocking_gesture = true;
      // If the event was sent to the main thread, assume the default gesture is
      // prevented until we see evidence otherwise.
      any_thread().default_gesture_prevented =
          !any_thread().last_gesture_was_compositor_driven;
      break;

    case blink::WebInputEvent::kUndefined:
      break;

    default:
      any_thread().awaiting_touch_start_response = false;
      break;
  }

  // Avoid unnecessary policy updates if the use case did not change.
  UseCase use_case = ComputeCurrentUseCase(now, &unused_policy_duration);

  if (use_case != previous_use_case ||
      was_awaiting_touch_start_response !=
          any_thread().awaiting_touch_start_response) {
    EnsureUrgentPolicyUpdatePostedOnMainThread(FROM_HERE);
  }
  GetCompositorThreadOnly().last_input_type = type;
}

void MainThreadSchedulerImpl::DidHandleInputEventOnMainThread(
    const WebInputEvent& web_input_event,
    WebInputEventResult result) {
  TRACE_EVENT0(TRACE_DISABLED_BY_DEFAULT("renderer.scheduler"),
               "MainThreadSchedulerImpl::DidHandleInputEventOnMainThread");
  helper_.CheckOnValidThread();
  if (ShouldPrioritizeInputEvent(web_input_event)) {
    base::AutoLock lock(any_thread_lock_);
    any_thread().user_model.DidFinishProcessingInputEvent(helper_.NowTicks());

    // If we were waiting for a touchstart response and the main thread has
    // prevented the default gesture, consider the gesture established. This
    // ensures single-event gestures such as button presses are promptly
    // detected.
    if (any_thread().awaiting_touch_start_response &&
        result == WebInputEventResult::kHandledApplication) {
      any_thread().awaiting_touch_start_response = false;
      any_thread().default_gesture_prevented = true;
      UpdatePolicyLocked(UpdateType::kMayEarlyOutIfPolicyUnchanged);
    }
  }
}

bool MainThreadSchedulerImpl::IsHighPriorityWorkAnticipated() {
  helper_.CheckOnValidThread();
  if (helper_.IsShutdown())
    return false;

  MaybeUpdatePolicy();
  // The touchstart, synchronized gesture and main-thread gesture use cases
  // indicate a strong likelihood of high-priority work in the near future.
  UseCase use_case = main_thread_only().current_use_case;
  return main_thread_only().touchstart_expected_soon ||
         use_case == UseCase::kTouchstart ||
         use_case == UseCase::kMainThreadGesture ||
         use_case == UseCase::kMainThreadCustomInputHandling ||
         use_case == UseCase::kSynchronizedGesture;
}

bool MainThreadSchedulerImpl::ShouldYieldForHighPriorityWork() {
  helper_.CheckOnValidThread();
  if (helper_.IsShutdown())
    return false;

  MaybeUpdatePolicy();
  // We only yield if there's a urgent task to be run now, or we are expecting
  // one soon (touch start).
  // Note: even though the control queue has the highest priority we don't yield
  // for it since these tasks are not user-provided work and they are only
  // intended to run before the next task, not interrupt the tasks.
  switch (main_thread_only().current_use_case) {
    case UseCase::kCompositorGesture:
    case UseCase::kNone:
      return main_thread_only().touchstart_expected_soon;

    case UseCase::kMainThreadGesture:
    case UseCase::kMainThreadCustomInputHandling:
    case UseCase::kSynchronizedGesture:
      return compositor_task_queue_->HasTaskToRunImmediately() ||
             main_thread_only().touchstart_expected_soon;

    case UseCase::kTouchstart:
      return true;

    case UseCase::kLoading:
      return false;

    default:
      NOTREACHED();
      return false;
  }
}

base::TimeTicks MainThreadSchedulerImpl::CurrentIdleTaskDeadlineForTesting()
    const {
  return idle_helper_.CurrentIdleTaskDeadline();
}

void MainThreadSchedulerImpl::RunIdleTasksForTesting(
    base::OnceClosure callback) {
  main_thread_only().in_idle_period_for_testing = true;
  IdleTaskRunner()->PostIdleTask(
      FROM_HERE,
      base::BindOnce(&MainThreadSchedulerImpl::EndIdlePeriodForTesting,
                     weak_factory_.GetWeakPtr(), std::move(callback)));
  idle_helper_.EnableLongIdlePeriod();
}

void MainThreadSchedulerImpl::MaybeUpdatePolicy() {
  helper_.CheckOnValidThread();
  if (policy_may_need_update_.IsSet()) {
    UpdatePolicy();
  }
}

void MainThreadSchedulerImpl::EnsureUrgentPolicyUpdatePostedOnMainThread(
    const base::Location& from_here) {
  // TODO(scheduler-dev): Check that this method isn't called from the main
  // thread.
  any_thread_lock_.AssertAcquired();
  if (!policy_may_need_update_.IsSet()) {
    policy_may_need_update_.SetWhileLocked(true);
    control_task_queue_->PostTask(from_here, update_policy_closure_);
  }
}

void MainThreadSchedulerImpl::UpdatePolicy() {
  base::AutoLock lock(any_thread_lock_);
  UpdatePolicyLocked(UpdateType::kMayEarlyOutIfPolicyUnchanged);
}

void MainThreadSchedulerImpl::ForceUpdatePolicy() {
  base::AutoLock lock(any_thread_lock_);
  UpdatePolicyLocked(UpdateType::kForceUpdate);
}

namespace {

void UpdatePolicyDuration(base::TimeTicks now,
                          base::TimeTicks policy_expiration,
                          base::TimeDelta* policy_duration) {
  if (policy_expiration <= now)
    return;

  if (policy_duration->is_zero()) {
    *policy_duration = policy_expiration - now;
    return;
  }

  *policy_duration = std::min(*policy_duration, policy_expiration - now);
}

}  // namespace

void MainThreadSchedulerImpl::UpdatePolicyLocked(UpdateType update_type) {
  helper_.CheckOnValidThread();
  any_thread_lock_.AssertAcquired();
  if (helper_.IsShutdown())
    return;

  base::TimeTicks now = helper_.NowTicks();
  policy_may_need_update_.SetWhileLocked(false);

  base::TimeDelta expected_use_case_duration;
  UseCase use_case = ComputeCurrentUseCase(now, &expected_use_case_duration);
  main_thread_only().current_use_case = use_case;

  base::TimeDelta touchstart_expected_flag_valid_for_duration;
  // TODO(skyostil): Consider handlers for all types of blocking gestures (e.g.,
  // mouse wheel) instead of just touchstart.
  bool touchstart_expected_soon = false;
  if (main_thread_only().has_visible_render_widget_with_touch_handler) {
    touchstart_expected_soon = any_thread().user_model.IsGestureExpectedSoon(
        now, &touchstart_expected_flag_valid_for_duration);
  }
  main_thread_only().touchstart_expected_soon = touchstart_expected_soon;

  base::TimeDelta longest_jank_free_task_duration =
      EstimateLongestJankFreeTaskDuration();
  main_thread_only().longest_jank_free_task_duration =
      longest_jank_free_task_duration;

  main_thread_only().loading_task_estimated_cost =
      main_thread_only().loading_task_cost_estimator.expected_task_duration();
  bool loading_tasks_seem_expensive =
      main_thread_only().loading_task_estimated_cost >
      longest_jank_free_task_duration;

  main_thread_only().timer_task_estimated_cost =
      main_thread_only().timer_task_cost_estimator.expected_task_duration();
  bool timer_tasks_seem_expensive =
      main_thread_only().timer_task_estimated_cost >
      longest_jank_free_task_duration;

  main_thread_only().timer_tasks_seem_expensive = timer_tasks_seem_expensive;
  main_thread_only().loading_tasks_seem_expensive =
      loading_tasks_seem_expensive;

  // The |new_policy_duration| is the minimum of |expected_use_case_duration|
  // and |touchstart_expected_flag_valid_for_duration| unless one is zero in
  // which case we choose the other.
  base::TimeDelta new_policy_duration = expected_use_case_duration;
  if (new_policy_duration.is_zero() ||
      (touchstart_expected_flag_valid_for_duration > base::TimeDelta() &&
       new_policy_duration > touchstart_expected_flag_valid_for_duration)) {
    new_policy_duration = touchstart_expected_flag_valid_for_duration;
  }

  bool previously_frozen_when_backgrounded =
      main_thread_only().frozen_when_backgrounded;
  bool newly_frozen = false;
  if (main_thread_only().renderer_backgrounded &&
      main_thread_only().freezing_when_backgrounded_enabled) {
    base::TimeTicks stop_at = main_thread_only().background_status_changed_at +
                              delay_for_background_tab_freezing_;

    newly_frozen = !main_thread_only().frozen_when_backgrounded;
    main_thread_only().frozen_when_backgrounded = now >= stop_at;
    newly_frozen &= main_thread_only().frozen_when_backgrounded;

    if (!main_thread_only().frozen_when_backgrounded)
      UpdatePolicyDuration(now, stop_at, &new_policy_duration);
  } else {
    main_thread_only().frozen_when_backgrounded = false;
  }

  if (new_policy_duration > base::TimeDelta()) {
    main_thread_only().current_policy_expiration_time =
        now + new_policy_duration;
    delayed_update_policy_runner_.SetDeadline(FROM_HERE, new_policy_duration,
                                              now);
  } else {
    main_thread_only().current_policy_expiration_time = base::TimeTicks();
  }

  // Avoid prioritizing main thread compositing (e.g., rAF) if it is extremely
  // slow, because that can cause starvation in other task sources.
  bool main_thread_compositing_is_fast =
      main_thread_only().idle_time_estimator.GetExpectedIdleDuration(
          main_thread_only().compositor_frame_interval) >
      main_thread_only().compositor_frame_interval *
          kFastCompositingIdleTimeThreshold;

  Policy new_policy;
  ExpensiveTaskPolicy expensive_task_policy = ExpensiveTaskPolicy::kRun;
  new_policy.rail_mode() = v8::PERFORMANCE_ANIMATION;

  switch (use_case) {
    case UseCase::kCompositorGesture:
      if (touchstart_expected_soon) {
        new_policy.rail_mode() = v8::PERFORMANCE_RESPONSE;
        expensive_task_policy = ExpensiveTaskPolicy::kBlock;
        new_policy.compositor_queue_policy().priority =
            TaskQueue::kHighestPriority;
      } else {
        // What we really want to do is priorize loading tasks, but that doesn't
        // seem to be safe. Instead we do that by proxy by deprioritizing
        // compositor tasks. This should be safe since we've already gone to the
        // pain of fixing ordering issues with them.
        new_policy.compositor_queue_policy().priority = TaskQueue::kLowPriority;
      }
      break;

    case UseCase::kSynchronizedGesture:
      new_policy.compositor_queue_policy().priority =
          main_thread_compositing_is_fast ? TaskQueue::kHighestPriority
                                          : TaskQueue::kNormalPriority;
      if (touchstart_expected_soon) {
        new_policy.rail_mode() = v8::PERFORMANCE_RESPONSE;
        expensive_task_policy = ExpensiveTaskPolicy::kBlock;
      } else {
        expensive_task_policy = ExpensiveTaskPolicy::kThrottle;
      }
      break;

    case UseCase::kMainThreadCustomInputHandling:
      // In main thread input handling scenarios we don't have perfect knowledge
      // about which things we should be prioritizing, so we don't attempt to
      // block expensive tasks because we don't know whether they were integral
      // to the page's functionality or not.
      new_policy.compositor_queue_policy().priority =
          main_thread_compositing_is_fast ? TaskQueue::kHighestPriority
                                          : TaskQueue::kNormalPriority;
      break;

    case UseCase::kMainThreadGesture:
      // A main thread gesture is for example a scroll gesture which is handled
      // by the main thread. Since we know the established gesture type, we can
      // be a little more aggressive about prioritizing compositing and input
      // handling over other tasks.
      new_policy.compositor_queue_policy().priority =
          TaskQueue::kHighestPriority;
      if (touchstart_expected_soon) {
        new_policy.rail_mode() = v8::PERFORMANCE_RESPONSE;
        expensive_task_policy = ExpensiveTaskPolicy::kBlock;
      } else {
        expensive_task_policy = ExpensiveTaskPolicy::kThrottle;
      }
      break;

    case UseCase::kTouchstart:
      new_policy.rail_mode() = v8::PERFORMANCE_RESPONSE;
      new_policy.compositor_queue_policy().priority =
          TaskQueue::kHighestPriority;
      new_policy.loading_queue_policy().is_blocked = true;
      new_policy.timer_queue_policy().is_blocked = true;
      // NOTE this is a nop due to the above.
      expensive_task_policy = ExpensiveTaskPolicy::kBlock;
      break;

    case UseCase::kNone:
      // It's only safe to block tasks that if we are expecting a compositor
      // driven gesture.
      if (touchstart_expected_soon &&
          any_thread().last_gesture_was_compositor_driven) {
        new_policy.rail_mode() = v8::PERFORMANCE_RESPONSE;
        expensive_task_policy = ExpensiveTaskPolicy::kBlock;
      }
      break;

    case UseCase::kLoading:
      new_policy.rail_mode() = v8::PERFORMANCE_LOAD;
      // TODO(skyostil): Experiment with increasing loading and default queue
      // priorities and throttling rendering frame rate.
      break;

    default:
      NOTREACHED();
  }

  // TODO(skyostil): Add an idle state for foreground tabs too.
  if (main_thread_only().renderer_hidden)
    new_policy.rail_mode() = v8::PERFORMANCE_IDLE;

  if (expensive_task_policy == ExpensiveTaskPolicy::kBlock &&
      !main_thread_only().have_seen_a_begin_main_frame) {
    expensive_task_policy = ExpensiveTaskPolicy::kRun;
  }

  switch (expensive_task_policy) {
    case ExpensiveTaskPolicy::kRun:
      break;

    case ExpensiveTaskPolicy::kBlock:
      if (loading_tasks_seem_expensive)
        new_policy.loading_queue_policy().is_blocked = true;
      if (timer_tasks_seem_expensive)
        new_policy.timer_queue_policy().is_blocked = true;
      break;

    case ExpensiveTaskPolicy::kThrottle:
      if (loading_tasks_seem_expensive) {
        new_policy.loading_queue_policy().is_throttled = true;
      }
      if (timer_tasks_seem_expensive) {
        new_policy.timer_queue_policy().is_throttled = true;
      }
      break;
  }
  main_thread_only().expensive_task_policy = expensive_task_policy;

  if (main_thread_only().frozen_when_backgrounded) {
    // TODO(panicker): Remove this, as it is controlled at
    // FrameScheduler. This is currently needed to avoid early out.
    new_policy.timer_queue_policy().is_frozen = true;
  }

  if (main_thread_only().renderer_pause_count != 0) {
    new_policy.loading_queue_policy().is_paused = true;
    new_policy.timer_queue_policy().is_paused = true;
  }
  if (main_thread_only().pause_timers_for_webview) {
    new_policy.timer_queue_policy().is_paused = true;
  }

  if (main_thread_only().renderer_backgrounded &&
      RuntimeEnabledFeatures::TimerThrottlingForBackgroundTabsEnabled()) {
    new_policy.timer_queue_policy().is_throttled = true;
  }

  if (main_thread_only().use_virtual_time) {
    new_policy.compositor_queue_policy().use_virtual_time = true;
    new_policy.default_queue_policy().use_virtual_time = true;
    new_policy.loading_queue_policy().use_virtual_time = true;
    new_policy.timer_queue_policy().use_virtual_time = true;
  }

  new_policy.should_disable_throttling() = main_thread_only().use_virtual_time;

  // Tracing is done before the early out check, because it's quite possible we
  // will otherwise miss this information in traces.
  CreateTraceEventObjectSnapshotLocked();

  // TODO(alexclarke): Can we get rid of force update now?
  if (update_type == UpdateType::kMayEarlyOutIfPolicyUnchanged &&
      new_policy == main_thread_only().current_policy) {
    return;
  }

  for (const auto& pair : task_runners_) {
    MainThreadTaskQueue::QueueClass queue_class = pair.first->queue_class();

    ApplyTaskQueuePolicy(
        pair.first.get(), pair.second.get(),
        main_thread_only().current_policy.GetQueuePolicy(queue_class),
        new_policy.GetQueuePolicy(queue_class));
  }

  main_thread_only().rail_mode_for_tracing = new_policy.rail_mode();
  if (main_thread_only().rail_mode_observer &&
      new_policy.rail_mode() != main_thread_only().current_policy.rail_mode()) {
    main_thread_only().rail_mode_observer->OnRAILModeChanged(
        new_policy.rail_mode());
  }

  // TODO(skyostil): send these notifications after releasing the scheduler
  // lock.
  if (main_thread_only().freezing_when_backgrounded_enabled) {
    if (main_thread_only().frozen_when_backgrounded !=
        previously_frozen_when_backgrounded) {
      SetFrozenInBackground(main_thread_only().frozen_when_backgrounded);
      MainThreadMetricsHelper::RecordBackgroundedTransition(
          main_thread_only().frozen_when_backgrounded
              ? BackgroundedRendererTransition::kFrozenAfterDelay
              : BackgroundedRendererTransition::kResumed);
    }
  }

  if (new_policy.should_disable_throttling() !=
      main_thread_only().current_policy.should_disable_throttling()) {
    if (new_policy.should_disable_throttling()) {
      task_queue_throttler()->DisableThrottling();
    } else {
      task_queue_throttler()->EnableThrottling();
    }
  }

  DCHECK(compositor_task_queue_->IsQueueEnabled());
  main_thread_only().current_policy = new_policy;

  if (newly_frozen)
    Platform::Current()->RequestPurgeMemory();
}

void MainThreadSchedulerImpl::ApplyTaskQueuePolicy(
    MainThreadTaskQueue* task_queue,
    TaskQueue::QueueEnabledVoter* task_queue_enabled_voter,
    const TaskQueuePolicy& old_task_queue_policy,
    const TaskQueuePolicy& new_task_queue_policy) const {
  DCHECK(old_task_queue_policy.IsQueueEnabled(task_queue) ||
         task_queue_enabled_voter);
  if (task_queue_enabled_voter) {
    task_queue_enabled_voter->SetQueueEnabled(
        new_task_queue_policy.IsQueueEnabled(task_queue));
  }

  // Make sure if there's no voter that the task queue is enabled.
  DCHECK(task_queue_enabled_voter ||
         old_task_queue_policy.IsQueueEnabled(task_queue));

  task_queue->SetQueuePriority(new_task_queue_policy.GetPriority(task_queue));

  TimeDomainType old_time_domain_type =
      old_task_queue_policy.GetTimeDomainType(task_queue);
  TimeDomainType new_time_domain_type =
      new_task_queue_policy.GetTimeDomainType(task_queue);

  if (old_time_domain_type != new_time_domain_type) {
    if (old_time_domain_type == TimeDomainType::kThrottled) {
      task_queue_throttler_->DecreaseThrottleRefCount(task_queue);
    } else if (new_time_domain_type == TimeDomainType::kThrottled) {
      task_queue_throttler_->IncreaseThrottleRefCount(task_queue);
    }
    if (new_time_domain_type == TimeDomainType::kVirtual) {
      DCHECK(virtual_time_domain_);
      task_queue->SetTimeDomain(virtual_time_domain_.get());
    } else {
      task_queue->SetTimeDomain(real_time_domain());
    }
  }
}

UseCase MainThreadSchedulerImpl::ComputeCurrentUseCase(
    base::TimeTicks now,
    base::TimeDelta* expected_use_case_duration) const {
  any_thread_lock_.AssertAcquired();
  // Special case for flings. This is needed because we don't get notification
  // of a fling ending (although we do for cancellation).
  if (any_thread().fling_compositor_escalation_deadline > now &&
      !any_thread().awaiting_touch_start_response) {
    *expected_use_case_duration =
        any_thread().fling_compositor_escalation_deadline - now;
    return UseCase::kCompositorGesture;
  }
  // Above all else we want to be responsive to user input.
  *expected_use_case_duration =
      any_thread().user_model.TimeLeftInUserGesture(now);
  if (*expected_use_case_duration > base::TimeDelta()) {
    // Has a gesture been fully established?
    if (any_thread().awaiting_touch_start_response) {
      // No, so arrange for compositor tasks to be run at the highest priority.
      return UseCase::kTouchstart;
    }

    // Yes a gesture has been established.  Based on how the gesture is handled
    // we need to choose between one of four use cases:
    // 1. kCompositorGesture where the gesture is processed only on the
    //    compositor thread.
    // 2. MAIN_THREAD_GESTURE where the gesture is processed only on the main
    //    thread.
    // 3. MAIN_THREAD_CUSTOM_INPUT_HANDLING where the main thread processes a
    //    stream of input events and has prevented a default gesture from being
    //    started.
    // 4. SYNCHRONIZED_GESTURE where the gesture is processed on both threads.
    if (any_thread().last_gesture_was_compositor_driven) {
      if (any_thread().begin_main_frame_on_critical_path) {
        return UseCase::kSynchronizedGesture;
      } else {
        return UseCase::kCompositorGesture;
      }
    }
    if (any_thread().default_gesture_prevented) {
      return UseCase::kMainThreadCustomInputHandling;
    } else {
      return UseCase::kMainThreadGesture;
    }
  }

  // Occasionally the meaningful paint fails to be detected, so as a fallback we
  // treat the presence of input as an indirect signal that there is meaningful
  // content on the page.
  if (any_thread().waiting_for_meaningful_paint &&
      !any_thread().have_seen_input_since_navigation) {
    return UseCase::kLoading;
  }
  return UseCase::kNone;
}

base::TimeDelta MainThreadSchedulerImpl::EstimateLongestJankFreeTaskDuration()
    const {
  switch (main_thread_only().current_use_case) {
    case UseCase::kTouchstart:
    case UseCase::kCompositorGesture:
    case UseCase::kLoading:
    case UseCase::kNone:
      return base::TimeDelta::FromMilliseconds(kRailsResponseTimeMillis);

    case UseCase::kMainThreadCustomInputHandling:
    case UseCase::kMainThreadGesture:
    case UseCase::kSynchronizedGesture:
      return main_thread_only().idle_time_estimator.GetExpectedIdleDuration(
          main_thread_only().compositor_frame_interval);

    default:
      NOTREACHED();
      return base::TimeDelta::FromMilliseconds(kRailsResponseTimeMillis);
  }
}

bool MainThreadSchedulerImpl::CanEnterLongIdlePeriod(
    base::TimeTicks now,
    base::TimeDelta* next_long_idle_period_delay_out) {
  helper_.CheckOnValidThread();

  MaybeUpdatePolicy();
  if (main_thread_only().current_use_case == UseCase::kTouchstart) {
    // Don't start a long idle task in touch start priority, try again when
    // the policy is scheduled to end.
    *next_long_idle_period_delay_out =
        std::max(base::TimeDelta(),
                 main_thread_only().current_policy_expiration_time - now);
    return false;
  }
  return true;
}

void MainThreadSchedulerImpl::SetFrozenInBackground(bool frozen) const {
  for (PageSchedulerImpl* page_scheduler : main_thread_only().page_schedulers) {
    // This moves the page to FROZEN lifecycle state.
    page_scheduler->SetPageFrozen(frozen);
  }
}

MainThreadSchedulerHelper*
MainThreadSchedulerImpl::GetSchedulerHelperForTesting() {
  return &helper_;
}

TaskCostEstimator*
MainThreadSchedulerImpl::GetLoadingTaskCostEstimatorForTesting() {
  return &main_thread_only().loading_task_cost_estimator;
}

TaskCostEstimator*
MainThreadSchedulerImpl::GetTimerTaskCostEstimatorForTesting() {
  return &main_thread_only().timer_task_cost_estimator;
}

IdleTimeEstimator* MainThreadSchedulerImpl::GetIdleTimeEstimatorForTesting() {
  return &main_thread_only().idle_time_estimator;
}

WakeUpBudgetPool* MainThreadSchedulerImpl::GetWakeUpBudgetPoolForTesting() {
  InitWakeUpBudgetPoolIfNeeded();
  return main_thread_only().wake_up_budget_pool;
}

base::TimeTicks MainThreadSchedulerImpl::EnableVirtualTime() {
  return EnableVirtualTime(main_thread_only().initial_virtual_time.is_null()
                               ? BaseTimeOverridePolicy::DO_NOT_OVERRIDE
                               : BaseTimeOverridePolicy::OVERRIDE);
}

base::TimeTicks MainThreadSchedulerImpl::EnableVirtualTime(
    BaseTimeOverridePolicy policy) {
  if (main_thread_only().use_virtual_time)
    return main_thread_only().initial_virtual_time_ticks;
  main_thread_only().use_virtual_time = true;
  DCHECK(!virtual_time_domain_);
  if (main_thread_only().initial_virtual_time.is_null())
    main_thread_only().initial_virtual_time = base::Time::Now();
  if (main_thread_only().initial_virtual_time_ticks.is_null())
    main_thread_only().initial_virtual_time_ticks = tick_clock()->NowTicks();
  virtual_time_domain_.reset(new AutoAdvancingVirtualTimeDomain(
      main_thread_only().initial_virtual_time +
          main_thread_only().initial_virtual_time_offset,
      main_thread_only().initial_virtual_time_ticks +
          main_thread_only().initial_virtual_time_offset,
      &helper_, policy));
  RegisterTimeDomain(virtual_time_domain_.get());
  virtual_time_domain_->SetObserver(this);

  DCHECK(!virtual_time_control_task_queue_);
  virtual_time_control_task_queue_ =
      helper_.NewTaskQueue(MainThreadTaskQueue::QueueCreationParams(
          MainThreadTaskQueue::QueueType::kControl));
  virtual_time_control_task_queue_->SetQueuePriority(
      TaskQueue::kControlPriority);
  virtual_time_control_task_queue_->SetTimeDomain(virtual_time_domain_.get());

  main_thread_only().use_virtual_time = true;
  ForceUpdatePolicy();

  virtual_time_domain_->SetCanAdvanceVirtualTime(
      !main_thread_only().virtual_time_stopped);

  if (main_thread_only().virtual_time_stopped)
    VirtualTimePaused();
  return main_thread_only().initial_virtual_time_ticks;
}

bool MainThreadSchedulerImpl::IsVirtualTimeEnabled() const {
  return main_thread_only().use_virtual_time;
}

void MainThreadSchedulerImpl::DisableVirtualTimeForTesting() {
  if (!main_thread_only().use_virtual_time)
    return;
  // Reset virtual time and all tasks queues back to their initial state.
  main_thread_only().use_virtual_time = false;

  if (main_thread_only().virtual_time_stopped) {
    main_thread_only().virtual_time_stopped = false;
    VirtualTimeResumed();
  }

  ForceUpdatePolicy();

  virtual_time_control_task_queue_->ShutdownTaskQueue();
  virtual_time_control_task_queue_ = nullptr;
  UnregisterTimeDomain(virtual_time_domain_.get());
  virtual_time_domain_.reset();
  virtual_time_control_task_queue_ = nullptr;
  ApplyVirtualTimePolicy();

  // Reset the MetricsHelper because it gets confused by time going backwards.
  base::TimeTicks now = tick_clock()->NowTicks();
  main_thread_only().metrics_helper.ResetForTest(now);
}

void MainThreadSchedulerImpl::SetVirtualTimeStopped(bool virtual_time_stopped) {
  if (main_thread_only().virtual_time_stopped == virtual_time_stopped)
    return;
  main_thread_only().virtual_time_stopped = virtual_time_stopped;

  if (!main_thread_only().use_virtual_time)
    return;

  virtual_time_domain_->SetCanAdvanceVirtualTime(!virtual_time_stopped);

  if (virtual_time_stopped) {
    VirtualTimePaused();
  } else {
    VirtualTimeResumed();
  }
}

void MainThreadSchedulerImpl::VirtualTimePaused() {
  for (const auto& pair : task_runners_) {
    if (pair.first->queue_class() == MainThreadTaskQueue::QueueClass::kTimer) {
      DCHECK(!task_queue_throttler_->IsThrottled(pair.first.get()));
      pair.first->InsertFence(TaskQueue::InsertFencePosition::kNow);
    }
  }
  for (auto& observer : main_thread_only().virtual_time_observers) {
    observer.OnVirtualTimePaused(virtual_time_domain_->Now() -
                                 main_thread_only().initial_virtual_time_ticks);
  }
}

void MainThreadSchedulerImpl::VirtualTimeResumed() {
  for (const auto& pair : task_runners_) {
    if (pair.first->queue_class() == MainThreadTaskQueue::QueueClass::kTimer) {
      DCHECK(!task_queue_throttler_->IsThrottled(pair.first.get()));
      DCHECK(pair.first->HasActiveFence());
      pair.first->RemoveFence();
    }
  }
}

bool MainThreadSchedulerImpl::VirtualTimeAllowedToAdvance() const {
  return !main_thread_only().virtual_time_stopped;
}

base::TimeTicks MainThreadSchedulerImpl::IncrementVirtualTimePauseCount() {
  main_thread_only().virtual_time_pause_count++;
  ApplyVirtualTimePolicy();

  if (virtual_time_domain_)
    return virtual_time_domain_->Now();
  return tick_clock()->NowTicks();
}

void MainThreadSchedulerImpl::DecrementVirtualTimePauseCount() {
  main_thread_only().virtual_time_pause_count--;
  DCHECK_GE(main_thread_only().virtual_time_pause_count, 0);
  ApplyVirtualTimePolicy();
}

void MainThreadSchedulerImpl::MaybeAdvanceVirtualTime(
    base::TimeTicks new_virtual_time) {
  if (virtual_time_domain_)
    virtual_time_domain_->MaybeAdvanceVirtualTime(new_virtual_time);
}

void MainThreadSchedulerImpl::SetVirtualTimePolicy(VirtualTimePolicy policy) {
  main_thread_only().virtual_time_policy = policy;
  ApplyVirtualTimePolicy();
}

void MainThreadSchedulerImpl::SetInitialVirtualTime(base::Time time) {
  main_thread_only().initial_virtual_time = time;
}

void MainThreadSchedulerImpl::SetInitialVirtualTimeOffset(
    base::TimeDelta offset) {
  main_thread_only().initial_virtual_time_offset = offset;
}

void MainThreadSchedulerImpl::AddVirtualTimeObserver(
    VirtualTimeObserver* observer) {
  main_thread_only().virtual_time_observers.AddObserver(observer);
}

void MainThreadSchedulerImpl::RemoveVirtualTimeObserver(
    VirtualTimeObserver* observer) {
  main_thread_only().virtual_time_observers.RemoveObserver(observer);
}

void MainThreadSchedulerImpl::OnVirtualTimeAdvanced() {
  for (auto& observer : main_thread_only().virtual_time_observers) {
    observer.OnVirtualTimeAdvanced(
        virtual_time_domain_->Now() -
        main_thread_only().initial_virtual_time_ticks);
  }
}

void MainThreadSchedulerImpl::ApplyVirtualTimePolicy() {
  switch (main_thread_only().virtual_time_policy) {
    case VirtualTimePolicy::kAdvance:
      if (virtual_time_domain_) {
        virtual_time_domain_->SetMaxVirtualTimeTaskStarvationCount(
            main_thread_only().nested_runloop
                ? 0
                : main_thread_only().max_virtual_time_task_starvation_count);
        virtual_time_domain_->SetVirtualTimeFence(base::TimeTicks());
      }
      SetVirtualTimeStopped(false);
      break;
    case VirtualTimePolicy::kPause:
      if (virtual_time_domain_) {
        virtual_time_domain_->SetMaxVirtualTimeTaskStarvationCount(0);
        virtual_time_domain_->SetVirtualTimeFence(virtual_time_domain_->Now());
      }
      SetVirtualTimeStopped(true);
      break;
    case VirtualTimePolicy::kDeterministicLoading:
      if (virtual_time_domain_) {
        virtual_time_domain_->SetMaxVirtualTimeTaskStarvationCount(
            main_thread_only().nested_runloop
                ? 0
                : main_thread_only().max_virtual_time_task_starvation_count);
      }

      // We pause virtual time while the run loop is nested because that implies
      // something modal is happening such as the DevTools debugger pausing the
      // system. We also pause while the renderer is waiting for various
      // asynchronous things e.g. resource load or navigation.
      SetVirtualTimeStopped(main_thread_only().virtual_time_pause_count != 0 ||
                            main_thread_only().nested_runloop);
      break;
  }
}

void MainThreadSchedulerImpl::SetMaxVirtualTimeTaskStarvationCount(
    int max_task_starvation_count) {
  main_thread_only().max_virtual_time_task_starvation_count =
      max_task_starvation_count;
  ApplyVirtualTimePolicy();
}

void MainThreadSchedulerImpl::SetFreezingWhenBackgroundedEnabled(bool enabled) {
  // Note that this will only take effect for the next backgrounded signal.
  main_thread_only().freezing_when_backgrounded_enabled = enabled;
}

std::unique_ptr<base::trace_event::ConvertableToTraceFormat>
MainThreadSchedulerImpl::AsValue(base::TimeTicks optional_now) const {
  base::AutoLock lock(any_thread_lock_);
  return AsValueLocked(optional_now);
}

void MainThreadSchedulerImpl::CreateTraceEventObjectSnapshot() const {
  TRACE_EVENT_OBJECT_SNAPSHOT_WITH_ID(
      TRACE_DISABLED_BY_DEFAULT("renderer.scheduler.debug"),
      "MainThreadScheduler", this, AsValue(helper_.NowTicks()));
}

void MainThreadSchedulerImpl::CreateTraceEventObjectSnapshotLocked() const {
  TRACE_EVENT_OBJECT_SNAPSHOT_WITH_ID(
      TRACE_DISABLED_BY_DEFAULT("renderer.scheduler.debug"),
      "MainThreadScheduler", this, AsValueLocked(helper_.NowTicks()));
}

// static
const char* MainThreadSchedulerImpl::ExpensiveTaskPolicyToString(
    ExpensiveTaskPolicy expensive_task_policy) {
  switch (expensive_task_policy) {
    case ExpensiveTaskPolicy::kRun:
      return "run";
    case ExpensiveTaskPolicy::kBlock:
      return "block";
    case ExpensiveTaskPolicy::kThrottle:
      return "throttle";
    default:
      NOTREACHED();
      return nullptr;
  }
}

std::unique_ptr<base::trace_event::ConvertableToTraceFormat>
MainThreadSchedulerImpl::AsValueLocked(base::TimeTicks optional_now) const {
  helper_.CheckOnValidThread();
  any_thread_lock_.AssertAcquired();

  if (optional_now.is_null())
    optional_now = helper_.NowTicks();
  std::unique_ptr<base::trace_event::TracedValue> state(
      new base::trace_event::TracedValue());
  state->SetBoolean(
      "has_visible_render_widget_with_touch_handler",
      main_thread_only().has_visible_render_widget_with_touch_handler);
  state->SetString("current_use_case",
                   UseCaseToString(main_thread_only().current_use_case));
  state->SetBoolean("loading_tasks_seem_expensive",
                    main_thread_only().loading_tasks_seem_expensive);
  state->SetBoolean("timer_tasks_seem_expensive",
                    main_thread_only().timer_tasks_seem_expensive);
  state->SetBoolean("begin_frame_not_expected_soon",
                    main_thread_only().begin_frame_not_expected_soon);
  state->SetBoolean(
      "compositor_will_send_main_frame_not_expected",
      main_thread_only().compositor_will_send_main_frame_not_expected);
  state->SetBoolean("touchstart_expected_soon",
                    main_thread_only().touchstart_expected_soon);
  state->SetString("idle_period_state",
                   IdleHelper::IdlePeriodStateToString(
                       idle_helper_.SchedulerIdlePeriodState()));
  state->SetBoolean("renderer_hidden", main_thread_only().renderer_hidden);
  state->SetBoolean("have_seen_a_begin_main_frame",
                    main_thread_only().have_seen_a_begin_main_frame);
  state->SetBoolean("waiting_for_meaningful_paint",
                    any_thread().waiting_for_meaningful_paint);
  state->SetBoolean("have_seen_input_since_navigation",
                    any_thread().have_seen_input_since_navigation);
  state->SetBoolean(
      "have_reported_blocking_intervention_in_current_policy",
      main_thread_only().have_reported_blocking_intervention_in_current_policy);
  state->SetBoolean(
      "have_reported_blocking_intervention_since_navigation",
      main_thread_only().have_reported_blocking_intervention_since_navigation);
  state->SetBoolean("renderer_backgrounded",
                    main_thread_only().renderer_backgrounded);
  state->SetBoolean("keep_active_fetch_or_worker",
                    main_thread_only().keep_active_fetch_or_worker);
  state->SetBoolean("frozen_when_backgrounded",
                    main_thread_only().frozen_when_backgrounded);
  state->SetDouble("now", (optional_now - base::TimeTicks()).InMillisecondsF());
  state->SetDouble(
      "fling_compositor_escalation_deadline",
      (any_thread().fling_compositor_escalation_deadline - base::TimeTicks())
          .InMillisecondsF());
  state->SetDouble("last_idle_period_end_time",
                   (any_thread().last_idle_period_end_time - base::TimeTicks())
                       .InMillisecondsF());
  state->SetBoolean("awaiting_touch_start_response",
                    any_thread().awaiting_touch_start_response);
  state->SetBoolean("begin_main_frame_on_critical_path",
                    any_thread().begin_main_frame_on_critical_path);
  state->SetBoolean("last_gesture_was_compositor_driven",
                    any_thread().last_gesture_was_compositor_driven);
  state->SetBoolean("default_gesture_prevented",
                    any_thread().default_gesture_prevented);
  state->SetDouble("expected_loading_task_duration",
                   main_thread_only()
                       .loading_task_cost_estimator.expected_task_duration()
                       .InMillisecondsF());
  state->SetDouble("expected_timer_task_duration",
                   main_thread_only()
                       .timer_task_cost_estimator.expected_task_duration()
                       .InMillisecondsF());
  state->SetBoolean("is_audio_playing", main_thread_only().is_audio_playing);
  state->SetBoolean("virtual_time_stopped",
                    main_thread_only().virtual_time_stopped);
  state->SetDouble("virtual_time_pause_count",
                   main_thread_only().virtual_time_pause_count);
  state->SetString(
      "virtual_time_policy",
      VirtualTimePolicyToString(main_thread_only().virtual_time_policy));
  state->SetBoolean("virtual_time", main_thread_only().use_virtual_time);

  state->BeginDictionary("page_schedulers");
  for (PageSchedulerImpl* page_scheduler : main_thread_only().page_schedulers) {
    state->BeginDictionaryWithCopiedName(PointerToString(page_scheduler));
    page_scheduler->AsValueInto(state.get());
    state->EndDictionary();
  }
  state->EndDictionary();

  state->BeginDictionary("policy");
  main_thread_only().current_policy.AsValueInto(state.get());
  state->EndDictionary();

  // TODO(skyostil): Can we somehow trace how accurate these estimates were?
  state->SetDouble(
      "longest_jank_free_task_duration",
      main_thread_only().longest_jank_free_task_duration->InMillisecondsF());
  state->SetDouble(
      "compositor_frame_interval",
      main_thread_only().compositor_frame_interval.InMillisecondsF());
  state->SetDouble(
      "estimated_next_frame_begin",
      (main_thread_only().estimated_next_frame_begin - base::TimeTicks())
          .InMillisecondsF());
  state->SetBoolean("in_idle_period", any_thread().in_idle_period);

  state->SetString(
      "expensive_task_policy",
      ExpensiveTaskPolicyToString(main_thread_only().expensive_task_policy));

  any_thread().user_model.AsValueInto(state.get());
  render_widget_scheduler_signals_.AsValueInto(state.get());

  state->BeginDictionary("task_queue_throttler");
  task_queue_throttler_->AsValueInto(state.get(), optional_now);
  state->EndDictionary();

  return std::move(state);
}

bool MainThreadSchedulerImpl::TaskQueuePolicy::IsQueueEnabled(
    MainThreadTaskQueue* task_queue) const {
  if (!is_enabled)
    return false;
  if (is_paused && task_queue->CanBePaused())
    return false;
  if (is_blocked && task_queue->CanBeDeferred())
    return false;
  // TODO(panicker): Remove this, as it is redundant as we stop per-frame
  // task_queues in WebFrameScheduler
  if (is_frozen && task_queue->CanBeFrozen())
    return false;
  return true;
}

TaskQueue::QueuePriority MainThreadSchedulerImpl::TaskQueuePolicy::GetPriority(
    MainThreadTaskQueue* task_queue) const {
  base::Optional<TaskQueue::QueuePriority> fixed_priority =
      task_queue->FixedPriority();
  if (fixed_priority)
    return fixed_priority.value();
  return task_queue->UsedForImportantTasks() ? TaskQueue::kHighestPriority
                                             : priority;
}

MainThreadSchedulerImpl::TimeDomainType
MainThreadSchedulerImpl::TaskQueuePolicy::GetTimeDomainType(
    MainThreadTaskQueue* task_queue) const {
  if (use_virtual_time)
    return TimeDomainType::kVirtual;
  if (is_throttled && task_queue->CanBeThrottled())
    return TimeDomainType::kThrottled;
  return TimeDomainType::kReal;
}

void MainThreadSchedulerImpl::TaskQueuePolicy::AsValueInto(
    base::trace_event::TracedValue* state) const {
  state->SetBoolean("is_enabled", is_enabled);
  state->SetBoolean("is_paused", is_paused);
  state->SetBoolean("is_throttled", is_throttled);
  state->SetBoolean("is_blocked", is_blocked);
  state->SetBoolean("is_frozen", is_frozen);
  state->SetBoolean("use_virtual_time", use_virtual_time);
  state->SetString("priority", TaskQueue::PriorityToString(priority));
}

void MainThreadSchedulerImpl::Policy::AsValueInto(
    base::trace_event::TracedValue* state) const {
  state->BeginDictionary("compositor_queue_policy");
  compositor_queue_policy().AsValueInto(state);
  state->EndDictionary();

  state->BeginDictionary("loading_queue_policy");
  loading_queue_policy().AsValueInto(state);
  state->EndDictionary();

  state->BeginDictionary("timer_queue_policy");
  timer_queue_policy().AsValueInto(state);
  state->EndDictionary();

  state->BeginDictionary("default_queue_policy");
  default_queue_policy().AsValueInto(state);
  state->EndDictionary();

  state->SetString("rail_mode", RAILModeToString(rail_mode()));
  state->SetBoolean("should_disable_throttling", should_disable_throttling());
}

void MainThreadSchedulerImpl::OnIdlePeriodStarted() {
  base::AutoLock lock(any_thread_lock_);
  any_thread().in_idle_period = true;
  UpdatePolicyLocked(UpdateType::kMayEarlyOutIfPolicyUnchanged);
}

void MainThreadSchedulerImpl::OnIdlePeriodEnded() {
  base::AutoLock lock(any_thread_lock_);
  any_thread().last_idle_period_end_time = helper_.NowTicks();
  any_thread().in_idle_period = false;
  UpdatePolicyLocked(UpdateType::kMayEarlyOutIfPolicyUnchanged);
}

void MainThreadSchedulerImpl::OnPendingTasksChanged(bool has_tasks) {
  if (has_tasks ==
      main_thread_only().compositor_will_send_main_frame_not_expected.get())
    return;

  // Dispatch RequestBeginMainFrameNotExpectedSoon notifications asynchronously.
  // This is needed because idle task can be posted (and OnPendingTasksChanged
  // called) at any moment, including in the middle of allocating an object,
  // when state is not consistent. Posting a task to dispatch notifications
  // minimizes the amount of code that runs and sees an inconsistent state .
  control_task_queue_->PostTask(
      FROM_HERE,
      base::BindOnce(
          &MainThreadSchedulerImpl::DispatchRequestBeginMainFrameNotExpected,
          weak_factory_.GetWeakPtr(), has_tasks));
}

void MainThreadSchedulerImpl::DispatchRequestBeginMainFrameNotExpected(
    bool has_tasks) {
  if (has_tasks ==
      main_thread_only().compositor_will_send_main_frame_not_expected.get())
    return;
  main_thread_only().compositor_will_send_main_frame_not_expected = has_tasks;

  TRACE_EVENT1(
      TRACE_DISABLED_BY_DEFAULT("renderer.scheduler"),
      "MainThreadSchedulerImpl::DispatchRequestBeginMainFrameNotExpected",
      "has_tasks", has_tasks);
  for (PageSchedulerImpl* page_scheduler : main_thread_only().page_schedulers) {
    page_scheduler->RequestBeginMainFrameNotExpected(has_tasks);
  }
}

std::unique_ptr<base::SingleSampleMetric>
MainThreadSchedulerImpl::CreateMaxQueueingTimeMetric() {
  return base::SingleSampleMetricsFactory::Get()->CreateCustomCountsMetric(
      "RendererScheduler.MaxQueueingTime", 1, 10000, 50);
}

void MainThreadSchedulerImpl::DidStartProvisionalLoad(bool is_main_frame) {
  TRACE_EVENT0(TRACE_DISABLED_BY_DEFAULT("renderer.scheduler"),
               "MainThreadSchedulerImpl::DidStartProvisionalLoad");
  if (is_main_frame) {
    base::AutoLock lock(any_thread_lock_);
    ResetForNavigationLocked();
  }
}

void MainThreadSchedulerImpl::DidCommitProvisionalLoad(
    bool is_web_history_inert_commit,
    bool is_reload,
    bool is_main_frame) {
  TRACE_EVENT0(TRACE_DISABLED_BY_DEFAULT("renderer.scheduler"),
               "MainThreadSchedulerImpl::DidCommitProvisionalLoad");
  // Initialize |max_queueing_time_metric| lazily so that
  // |SingleSampleMetricsFactory::SetFactory()| is called before
  // |SingleSampleMetricsFactory::Get()|
  if (!main_thread_only().max_queueing_time_metric) {
    main_thread_only().max_queueing_time_metric = CreateMaxQueueingTimeMetric();
  }
  main_thread_only().max_queueing_time_metric.reset();
  main_thread_only().max_queueing_time = base::TimeDelta();
  main_thread_only().has_navigated = true;

  // If this either isn't a history inert commit or it's a reload then we must
  // reset the task cost estimators.
  if (is_main_frame && (!is_web_history_inert_commit || is_reload)) {
    base::AutoLock lock(any_thread_lock_);
    ResetForNavigationLocked();
  }
}

void MainThreadSchedulerImpl::OnFirstMeaningfulPaint() {
  TRACE_EVENT0(TRACE_DISABLED_BY_DEFAULT("renderer.scheduler"),
               "MainThreadSchedulerImpl::OnFirstMeaningfulPaint");
  base::AutoLock lock(any_thread_lock_);
  any_thread().waiting_for_meaningful_paint = false;
  UpdatePolicyLocked(UpdateType::kMayEarlyOutIfPolicyUnchanged);
}

void MainThreadSchedulerImpl::ResetForNavigationLocked() {
  TRACE_EVENT0(TRACE_DISABLED_BY_DEFAULT("renderer.scheduler"),
               "MainThreadSchedulerImpl::ResetForNavigationLocked");
  helper_.CheckOnValidThread();
  any_thread_lock_.AssertAcquired();
  any_thread().user_model.Reset(helper_.NowTicks());
  any_thread().have_seen_a_potentially_blocking_gesture = false;
  any_thread().waiting_for_meaningful_paint = true;
  any_thread().have_seen_input_since_navigation = false;
  main_thread_only().loading_task_cost_estimator.Clear();
  main_thread_only().timer_task_cost_estimator.Clear();
  main_thread_only().idle_time_estimator.Clear();
  main_thread_only().have_seen_a_begin_main_frame = false;
  main_thread_only().have_reported_blocking_intervention_since_navigation =
      false;
  for (PageSchedulerImpl* page_scheduler : main_thread_only().page_schedulers) {
    page_scheduler->OnNavigation();
  }
  UpdatePolicyLocked(UpdateType::kMayEarlyOutIfPolicyUnchanged);

  UMA_HISTOGRAM_COUNTS_100("RendererScheduler.WebViewsPerScheduler",
                           main_thread_only().page_schedulers.size());

  size_t frame_count = 0;
  for (PageSchedulerImpl* page_scheduler : main_thread_only().page_schedulers) {
    frame_count += page_scheduler->FrameCount();
  }
  UMA_HISTOGRAM_COUNTS_100("RendererScheduler.WebFramesPerScheduler",
                           frame_count);
}

void MainThreadSchedulerImpl::SetTopLevelBlameContext(
    base::trace_event::BlameContext* blame_context) {
  // Any task that runs in the default task runners belongs to the context of
  // all frames (as opposed to a particular frame). Note that the task itself
  // may still enter a more specific blame context if necessary.
  //
  // Per-frame task runners (loading, timers, etc.) are configured with a more
  // specific blame context by FrameSchedulerImpl.
  //
  // TODO(altimin): automatically enter top-level for all task queues associated
  // with renderer scheduler which do not have a corresponding frame.
  control_task_queue_->SetBlameContext(blame_context);
  DefaultTaskQueue()->SetBlameContext(blame_context);
  compositor_task_queue_->SetBlameContext(blame_context);
  idle_helper_.IdleTaskRunner()->SetBlameContext(blame_context);
  v8_task_queue_->SetBlameContext(blame_context);
  ipc_task_queue_->SetBlameContext(blame_context);
}

void MainThreadSchedulerImpl::SetRAILModeObserver(RAILModeObserver* observer) {
  main_thread_only().rail_mode_observer = observer;
}

void MainThreadSchedulerImpl::SetRendererProcessType(RendererProcessType type) {
  main_thread_only().process_type = type;
}

WebScopedVirtualTimePauser
MainThreadSchedulerImpl::CreateWebScopedVirtualTimePauser(
    const char* name,
    WebScopedVirtualTimePauser::VirtualTaskDuration duration) {
  return WebScopedVirtualTimePauser(this, duration,
                                    WebString(WTF::String(name)));
}

void MainThreadSchedulerImpl::RunIdleTask(WebThread::IdleTask task,
                                          base::TimeTicks deadline) {
  std::move(task).Run((deadline - base::TimeTicks()).InSecondsF());
}

void MainThreadSchedulerImpl::PostIdleTask(const base::Location& location,
                                           WebThread::IdleTask task) {
  IdleTaskRunner()->PostIdleTask(
      location,
      base::BindOnce(&MainThreadSchedulerImpl::RunIdleTask, std::move(task)));
}

void MainThreadSchedulerImpl::PostNonNestableIdleTask(
    const base::Location& location,
    WebThread::IdleTask task) {
  IdleTaskRunner()->PostNonNestableIdleTask(
      location,
      base::BindOnce(&MainThreadSchedulerImpl::RunIdleTask, std::move(task)));
}

scoped_refptr<base::SingleThreadTaskRunner>
MainThreadSchedulerImpl::V8TaskRunner() {
  return v8_task_runner_;
}

scoped_refptr<base::SingleThreadTaskRunner>
MainThreadSchedulerImpl::CompositorTaskRunner() {
  return compositor_task_runner_;
}

std::unique_ptr<PageScheduler> MainThreadSchedulerImpl::CreatePageScheduler(
    PageScheduler::Delegate* delegate) {
  return std::make_unique<PageSchedulerImpl>(
      delegate, this,
      !RuntimeEnabledFeatures::TimerThrottlingForBackgroundTabsEnabled());
}

std::unique_ptr<ThreadScheduler::RendererPauseHandle>
MainThreadSchedulerImpl::PauseScheduler() {
  return PauseRenderer();
}

base::TimeTicks MainThreadSchedulerImpl::MonotonicallyIncreasingVirtualTime() {
  return GetActiveTimeDomain()->Now();
}

WebMainThreadScheduler*
MainThreadSchedulerImpl::GetWebMainThreadSchedulerForTest() {
  return this;
}

void MainThreadSchedulerImpl::RegisterTimeDomain(TimeDomain* time_domain) {
  helper_.RegisterTimeDomain(time_domain);
}

void MainThreadSchedulerImpl::UnregisterTimeDomain(TimeDomain* time_domain) {
  helper_.UnregisterTimeDomain(time_domain);
}

const base::TickClock* MainThreadSchedulerImpl::GetTickClock() {
  return tick_clock();
}

const base::TickClock* MainThreadSchedulerImpl::tick_clock() const {
  return helper_.GetClock();
}

void MainThreadSchedulerImpl::AddPageScheduler(
    PageSchedulerImpl* page_scheduler) {
  main_thread_only().page_schedulers.insert(page_scheduler);
}

void MainThreadSchedulerImpl::RemovePageScheduler(
    PageSchedulerImpl* page_scheduler) {
  DCHECK(main_thread_only().page_schedulers.find(page_scheduler) !=
         main_thread_only().page_schedulers.end());
  main_thread_only().page_schedulers.erase(page_scheduler);
}

void MainThreadSchedulerImpl::BroadcastIntervention(
    const std::string& message) {
  helper_.CheckOnValidThread();
  for (auto* page_scheduler : main_thread_only().page_schedulers)
    page_scheduler->ReportIntervention(message);
}

void MainThreadSchedulerImpl::OnTaskStarted(MainThreadTaskQueue* queue,
                                            const TaskQueue::Task& task,
                                            base::TimeTicks start) {
  main_thread_only().current_task_start_time = start;
  queueing_time_estimator_.OnTopLevelTaskStarted(start, queue);
  main_thread_only().task_description_for_tracing = TaskDescriptionForTracing{
      static_cast<TaskType>(task.task_type()),
      queue
          ? base::Optional<MainThreadTaskQueue::QueueType>(queue->queue_type())
          : base::nullopt};

  main_thread_only().task_priority_for_tracing =
      queue
          ? base::Optional<TaskQueue::QueuePriority>(queue->GetQueuePriority())
          : base::nullopt;
}

void MainThreadSchedulerImpl::OnTaskCompleted(
    MainThreadTaskQueue* queue,
    const TaskQueue::Task& task,
    base::TimeTicks start,
    base::TimeTicks end,
    base::Optional<base::TimeDelta> thread_time) {
  DCHECK_LE(start, end);
  queueing_time_estimator_.OnTopLevelTaskCompleted(end);

  if (queue)
    task_queue_throttler()->OnTaskRunTimeReported(queue, start, end);

  // TODO(altimin): Per-page metrics should also be considered.
  main_thread_only().metrics_helper.RecordTaskMetrics(queue, task, start, end,
                                                      thread_time);
  main_thread_only().task_description_for_tracing = base::nullopt;

  // Unset the state of |task_priority_for_tracing|.
  main_thread_only().task_priority_for_tracing = base::nullopt;

  RecordTaskUkm(queue, task, start, end, thread_time);
}

void MainThreadSchedulerImpl::RecordTaskUkm(
    MainThreadTaskQueue* queue,
    const TaskQueue::Task& task,
    base::TimeTicks start,
    base::TimeTicks end,
    base::Optional<base::TimeDelta> thread_time) {
  if (!ShouldRecordTaskUkm(thread_time.has_value()))
    return;

  if (queue && queue->GetFrameScheduler()) {
    RecordTaskUkmImpl(queue, task, start, end, thread_time,
                      static_cast<PageSchedulerImpl*>(
                          queue->GetFrameScheduler()->GetPageScheduler()),
                      1);
    return;
  }

  for (PageSchedulerImpl* page_scheduler : main_thread_only().page_schedulers) {
    RecordTaskUkmImpl(queue, task, start, end, thread_time, page_scheduler,
                      main_thread_only().page_schedulers.size());
  }
}

void MainThreadSchedulerImpl::RecordTaskUkmImpl(
    MainThreadTaskQueue* queue,
    const TaskQueue::Task& task,
    base::TimeTicks start,
    base::TimeTicks end,
    base::Optional<base::TimeDelta> thread_time,
    PageSchedulerImpl* page_scheduler,
    size_t page_schedulers_to_attribute) {
  // Skip tasks which have deleted the page scheduler.
  if (!page_scheduler)
    return;

  ukm::UkmRecorder* ukm_recorder = page_scheduler->GetUkmRecorder();
  // OOPIFs are not supported.
  if (!ukm_recorder)
    return;

  ukm::builders::RendererSchedulerTask builder(
      page_scheduler->GetUkmSourceId());

  builder.SetPageSchedulers(page_schedulers_to_attribute);
  builder.SetRendererBackgrounded(main_thread_only().renderer_backgrounded);
  builder.SetRendererHidden(main_thread_only().renderer_hidden);
  builder.SetRendererAudible(main_thread_only().is_audio_playing);
  builder.SetUseCase(
      static_cast<int>(main_thread_only().current_use_case.get()));
  builder.SetTaskType(task.task_type());
  builder.SetQueueType(static_cast<int>(
      queue ? queue->queue_type() : MainThreadTaskQueue::QueueType::kDetached));
  builder.SetFrameStatus(static_cast<int>(
      GetFrameStatus(queue ? queue->GetFrameScheduler() : nullptr)));
  builder.SetTaskDuration((end - start).InMicroseconds());

  if (thread_time) {
    builder.SetTaskCPUDuration(thread_time->InMicroseconds());
  }

  builder.Record(ukm_recorder);
}

void MainThreadSchedulerImpl::OnBeginNestedRunLoop() {
  queueing_time_estimator_.OnBeginNestedRunLoop();

  main_thread_only().nested_runloop = true;
  ApplyVirtualTimePolicy();
}

void MainThreadSchedulerImpl::OnExitNestedRunLoop() {
  main_thread_only().nested_runloop = false;
  ApplyVirtualTimePolicy();
}

void MainThreadSchedulerImpl::AddTaskTimeObserver(
    TaskTimeObserver* task_time_observer) {
  helper_.AddTaskTimeObserver(task_time_observer);
}

void MainThreadSchedulerImpl::RemoveTaskTimeObserver(
    TaskTimeObserver* task_time_observer) {
  helper_.RemoveTaskTimeObserver(task_time_observer);
}

bool MainThreadSchedulerImpl::ContainsLocalMainFrame() {
  for (auto* page_scheduler : main_thread_only().page_schedulers) {
    if (page_scheduler->IsMainFrameLocal())
      return true;
  }
  return false;
}

void MainThreadSchedulerImpl::OnQueueingTimeForWindowEstimated(
    base::TimeDelta queueing_time,
    bool is_disjoint_window) {
  if (main_thread_only().has_navigated) {
    if (main_thread_only().max_queueing_time < queueing_time) {
      if (!main_thread_only().max_queueing_time_metric) {
        main_thread_only().max_queueing_time_metric =
            CreateMaxQueueingTimeMetric();
      }
      main_thread_only().max_queueing_time_metric->SetSample(
          queueing_time.InMilliseconds());
      main_thread_only().max_queueing_time = queueing_time;
    }
  }

  if (!is_disjoint_window || !ContainsLocalMainFrame())
    return;

  UMA_HISTOGRAM_TIMES("RendererScheduler.ExpectedTaskQueueingDuration",
                      queueing_time);
  UMA_HISTOGRAM_CUSTOM_COUNTS(
      "RendererScheduler.ExpectedTaskQueueingDuration3",
      queueing_time.InMicroseconds(), kMinExpectedQueueingTimeBucket,
      kMaxExpectedQueueingTimeBucket, kNumberExpectedQueueingTimeBuckets);
  TRACE_COUNTER1(TRACE_DISABLED_BY_DEFAULT("renderer.scheduler"),
                 "estimated_queueing_time_for_window",
                 queueing_time.InMillisecondsF());

  if (::resource_coordinator::IsResourceCoordinatorEnabled()) {
    RendererResourceCoordinator::Get().SetExpectedTaskQueueingDuration(
        queueing_time);
  }
}

void MainThreadSchedulerImpl::OnReportFineGrainedExpectedQueueingTime(
    const char* split_description,
    base::TimeDelta queueing_time) {
  if (!ContainsLocalMainFrame())
    return;

  base::UmaHistogramCustomCounts(
      split_description, queueing_time.InMicroseconds(),
      kMinExpectedQueueingTimeBucket, kMaxExpectedQueueingTimeBucket,
      kNumberExpectedQueueingTimeBuckets);
}

AutoAdvancingVirtualTimeDomain*
MainThreadSchedulerImpl::GetVirtualTimeDomain() {
  return virtual_time_domain_.get();
}

void MainThreadSchedulerImpl::AddQueueToWakeUpBudgetPool(
    MainThreadTaskQueue* queue) {
  InitWakeUpBudgetPoolIfNeeded();
  main_thread_only().wake_up_budget_pool->AddQueue(tick_clock()->NowTicks(),
                                                   queue);
}

void MainThreadSchedulerImpl::InitWakeUpBudgetPoolIfNeeded() {
  if (main_thread_only().wake_up_budget_pool)
    return;

  main_thread_only().wake_up_budget_pool =
      task_queue_throttler()->CreateWakeUpBudgetPool("renderer_wake_up_pool");
  main_thread_only().wake_up_budget_pool->SetWakeUpRate(1);
  main_thread_only().wake_up_budget_pool->SetWakeUpDuration(
      GetWakeUpDuration());
}

TimeDomain* MainThreadSchedulerImpl::GetActiveTimeDomain() {
  if (main_thread_only().use_virtual_time) {
    return GetVirtualTimeDomain();
  } else {
    return real_time_domain();
  }
}

void MainThreadSchedulerImpl::OnTraceLogEnabled() {
  CreateTraceEventObjectSnapshot();
  tracing_controller_.OnTraceLogEnabled();
  for (PageSchedulerImpl* page_scheduler : main_thread_only().page_schedulers) {
    page_scheduler->OnTraceLogEnabled();
  }
}

void MainThreadSchedulerImpl::OnTraceLogDisabled() {}

base::WeakPtr<MainThreadSchedulerImpl> MainThreadSchedulerImpl::GetWeakPtr() {
  return weak_factory_.GetWeakPtr();
}

bool MainThreadSchedulerImpl::IsAudioPlaying() const {
  return main_thread_only().is_audio_playing;
}

bool MainThreadSchedulerImpl::ShouldIgnoreTaskForUkm(bool has_thread_time,
                                                     double* sampling_rate) {
  const double thread_time_sampling_rate =
      helper_.GetSamplingRateForRecordingCPUTime();
  if (thread_time_sampling_rate && *sampling_rate < thread_time_sampling_rate) {
    if (!has_thread_time)
      return true;
    *sampling_rate /= thread_time_sampling_rate;
  }
  return false;
}

bool MainThreadSchedulerImpl::ShouldRecordTaskUkm(bool has_thread_time) {
  double sampling_rate = kSamplingRateForTaskUkm;

  // If thread_time is sampled as well, try to align UKM sampling with it so
  // that we only record UKMs for tasks that also record thread_time.
  if (ShouldIgnoreTaskForUkm(has_thread_time, &sampling_rate)) {
    return false;
  }

  return main_thread_only().uniform_distribution(
             main_thread_only().random_generator) < sampling_rate;
}

// static
const char* MainThreadSchedulerImpl::UseCaseToString(UseCase use_case) {
  switch (use_case) {
    case UseCase::kNone:
      return "none";
    case UseCase::kCompositorGesture:
      return "compositor_gesture";
    case UseCase::kMainThreadCustomInputHandling:
      return "main_thread_custom_input_handling";
    case UseCase::kSynchronizedGesture:
      return "synchronized_gesture";
    case UseCase::kTouchstart:
      return "touchstart";
    case UseCase::kLoading:
      return "loading";
    case UseCase::kMainThreadGesture:
      return "main_thread_gesture";
    default:
      NOTREACHED();
      return nullptr;
  }
}

// static
const char* MainThreadSchedulerImpl::RAILModeToString(v8::RAILMode rail_mode) {
  switch (rail_mode) {
    case v8::PERFORMANCE_RESPONSE:
      return "response";
    case v8::PERFORMANCE_ANIMATION:
      return "animation";
    case v8::PERFORMANCE_IDLE:
      return "idle";
    case v8::PERFORMANCE_LOAD:
      return "load";
    default:
      NOTREACHED();
      return nullptr;
  }
}

// static
const char* MainThreadSchedulerImpl::TimeDomainTypeToString(
    TimeDomainType domain_type) {
  switch (domain_type) {
    case TimeDomainType::kReal:
      return "real";
    case TimeDomainType::kThrottled:
      return "throttled";
    case TimeDomainType::kVirtual:
      return "virtual";
    default:
      NOTREACHED();
      return nullptr;
  }
}

// static
const char* MainThreadSchedulerImpl::VirtualTimePolicyToString(
    VirtualTimePolicy virtual_time_policy) {
  switch (virtual_time_policy) {
    case VirtualTimePolicy::kAdvance:
      return "ADVANCE";
    case VirtualTimePolicy::kPause:
      return "PAUSE";
    case VirtualTimePolicy::kDeterministicLoading:
      return "DETERMINISTIC_LOADING";
    default:
      NOTREACHED();
      return nullptr;
  }
}

}  // namespace scheduler
}  // namespace blink
