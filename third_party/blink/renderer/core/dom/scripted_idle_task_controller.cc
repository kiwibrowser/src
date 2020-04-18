// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/dom/scripted_idle_task_controller.h"

#include "base/location.h"
#include "third_party/blink/public/platform/platform.h"
#include "third_party/blink/renderer/core/dom/idle_request_options.h"
#include "third_party/blink/renderer/core/execution_context/execution_context.h"
#include "third_party/blink/renderer/core/inspector/inspector_trace_events.h"
#include "third_party/blink/renderer/core/probe/core_probes.h"
#include "third_party/blink/renderer/platform/histogram.h"
#include "third_party/blink/renderer/platform/instrumentation/tracing/trace_event.h"
#include "third_party/blink/renderer/platform/scheduler/public/thread_scheduler.h"
#include "third_party/blink/renderer/platform/wtf/functional.h"
#include "third_party/blink/renderer/platform/wtf/ref_counted.h"
#include "third_party/blink/renderer/platform/wtf/time.h"

namespace blink {

namespace internal {

class IdleRequestCallbackWrapper
    : public RefCounted<IdleRequestCallbackWrapper> {
 public:
  static scoped_refptr<IdleRequestCallbackWrapper> Create(
      ScriptedIdleTaskController::CallbackId id,
      ScriptedIdleTaskController* controller) {
    return base::AdoptRef(new IdleRequestCallbackWrapper(id, controller));
  }
  virtual ~IdleRequestCallbackWrapper() = default;

  static void IdleTaskFired(
      scoped_refptr<IdleRequestCallbackWrapper> callback_wrapper,
      double deadline_seconds) {
    if (ScriptedIdleTaskController* controller =
            callback_wrapper->Controller()) {
      // If we are going to yield immediately, reschedule the callback for
      // later.
      if (Platform::Current()
              ->CurrentThread()
              ->Scheduler()
              ->ShouldYieldForHighPriorityWork()) {
        controller->ScheduleCallback(std::move(callback_wrapper),
                                     /* timeout_millis */ 0);
        return;
      }
      controller->CallbackFired(callback_wrapper->Id(), deadline_seconds,
                                IdleDeadline::CallbackType::kCalledWhenIdle);
    }
    callback_wrapper->Cancel();
  }

  static void TimeoutFired(
      scoped_refptr<IdleRequestCallbackWrapper> callback_wrapper) {
    if (ScriptedIdleTaskController* controller =
            callback_wrapper->Controller()) {
      controller->CallbackFired(callback_wrapper->Id(),
                                CurrentTimeTicksInSeconds(),
                                IdleDeadline::CallbackType::kCalledByTimeout);
    }
    callback_wrapper->Cancel();
  }

  void Cancel() { controller_ = nullptr; }

  ScriptedIdleTaskController::CallbackId Id() const { return id_; }
  ScriptedIdleTaskController* Controller() const { return controller_; }

 private:
  IdleRequestCallbackWrapper(ScriptedIdleTaskController::CallbackId id,
                             ScriptedIdleTaskController* controller)
      : id_(id), controller_(controller) {}

  ScriptedIdleTaskController::CallbackId id_;
  WeakPersistent<ScriptedIdleTaskController> controller_;
};

}  // namespace internal

ScriptedIdleTaskController::V8IdleTask::V8IdleTask(
    V8IdleRequestCallback* callback)
    : callback_(callback) {}

void ScriptedIdleTaskController::V8IdleTask::Trace(blink::Visitor* visitor) {
  visitor->Trace(callback_);
  ScriptedIdleTaskController::IdleTask::Trace(visitor);
}

void ScriptedIdleTaskController::V8IdleTask::TraceWrappers(
    ScriptWrappableVisitor* visitor) const {
  visitor->TraceWrappers(callback_);
  ScriptedIdleTaskController::IdleTask::TraceWrappers(visitor);
}

void ScriptedIdleTaskController::V8IdleTask::invoke(IdleDeadline* deadline) {
  callback_->InvokeAndReportException(nullptr, deadline);
}

ScriptedIdleTaskController::ScriptedIdleTaskController(
    ExecutionContext* context)
    : PausableObject(context),
      scheduler_(Platform::Current()->CurrentThread()->Scheduler()),
      next_callback_id_(0),
      paused_(false) {
  PauseIfNeeded();
}

ScriptedIdleTaskController::~ScriptedIdleTaskController() = default;

void ScriptedIdleTaskController::Trace(blink::Visitor* visitor) {
  visitor->Trace(idle_tasks_);
  PausableObject::Trace(visitor);
}

void ScriptedIdleTaskController::TraceWrappers(
    ScriptWrappableVisitor* visitor) const {
  for (const auto& idle_task : idle_tasks_.Values()) {
    visitor->TraceWrappers(idle_task);
  }
}

int ScriptedIdleTaskController::NextCallbackId() {
  while (true) {
    ++next_callback_id_;

    if (!IsValidCallbackId(next_callback_id_))
      next_callback_id_ = 1;

    if (!idle_tasks_.Contains(next_callback_id_))
      return next_callback_id_;
  }
}

ScriptedIdleTaskController::CallbackId
ScriptedIdleTaskController::RegisterCallback(
    IdleTask* idle_task,
    const IdleRequestOptions& options) {
  DCHECK(idle_task);

  CallbackId id = NextCallbackId();
  idle_tasks_.Set(id, idle_task);
  long long timeout_millis = options.timeout();

  probe::AsyncTaskScheduled(GetExecutionContext(), "requestIdleCallback",
                            idle_task);

  scoped_refptr<internal::IdleRequestCallbackWrapper> callback_wrapper =
      internal::IdleRequestCallbackWrapper::Create(id, this);
  ScheduleCallback(std::move(callback_wrapper), timeout_millis);
  TRACE_EVENT_INSTANT1("devtools.timeline", "RequestIdleCallback",
                       TRACE_EVENT_SCOPE_THREAD, "data",
                       InspectorIdleCallbackRequestEvent::Data(
                           GetExecutionContext(), id, timeout_millis));
  return id;
}

void ScriptedIdleTaskController::ScheduleCallback(
    scoped_refptr<internal::IdleRequestCallbackWrapper> callback_wrapper,
    long long timeout_millis) {
  scheduler_->PostIdleTask(
      FROM_HERE, WTF::Bind(&internal::IdleRequestCallbackWrapper::IdleTaskFired,
                           callback_wrapper));
  if (timeout_millis > 0) {
    GetExecutionContext()
        ->GetTaskRunner(TaskType::kIdleTask)
        ->PostDelayedTask(
            FROM_HERE,
            WTF::Bind(&internal::IdleRequestCallbackWrapper::TimeoutFired,
                      callback_wrapper),
            TimeDelta::FromMilliseconds(timeout_millis));
  }
}

void ScriptedIdleTaskController::CancelCallback(CallbackId id) {
  TRACE_EVENT_INSTANT1(
      "devtools.timeline", "CancelIdleCallback", TRACE_EVENT_SCOPE_THREAD,
      "data",
      InspectorIdleCallbackCancelEvent::Data(GetExecutionContext(), id));
  if (!IsValidCallbackId(id))
    return;

  idle_tasks_.erase(id);
}

void ScriptedIdleTaskController::CallbackFired(
    CallbackId id,
    double deadline_seconds,
    IdleDeadline::CallbackType callback_type) {
  if (!idle_tasks_.Contains(id))
    return;

  if (paused_) {
    if (callback_type == IdleDeadline::CallbackType::kCalledByTimeout) {
      // Queue for execution when we are resumed.
      pending_timeouts_.push_back(id);
    }
    // Just drop callbacks called while suspended, these will be reposted on the
    // idle task queue when we are resumed.
    return;
  }

  RunCallback(id, deadline_seconds, callback_type);
}

void ScriptedIdleTaskController::RunCallback(
    CallbackId id,
    double deadline_seconds,
    IdleDeadline::CallbackType callback_type) {
  DCHECK(!paused_);

  // Keep the idle task in |idle_tasks_| so that it's still wrapper-traced.
  // TODO(https://crbug.com/796145): Remove this hack once on-stack objects
  // get supported by either of wrapper-tracing or unified GC.
  auto idle_task_iter = idle_tasks_.find(id);
  if (idle_task_iter == idle_tasks_.end())
    return;
  IdleTask* idle_task = idle_task_iter->value;
  DCHECK(idle_task);

  double allotted_time_millis =
      std::max((deadline_seconds - CurrentTimeTicksInSeconds()) * 1000, 0.0);

  probe::AsyncTask async_task(GetExecutionContext(), idle_task);
  probe::UserCallback probe(GetExecutionContext(), "requestIdleCallback",
                            AtomicString(), true);

  TRACE_EVENT1(
      "devtools.timeline", "FireIdleCallback", "data",
      InspectorIdleCallbackFireEvent::Data(
          GetExecutionContext(), id, allotted_time_millis,
          callback_type == IdleDeadline::CallbackType::kCalledByTimeout));
  idle_task->invoke(IdleDeadline::Create(deadline_seconds, callback_type));

  // Finally there is no need to keep the idle task alive.
  //
  // Do not use the iterator because the idle task might update |idle_tasks_|.
  idle_tasks_.erase(id);
}

void ScriptedIdleTaskController::ContextDestroyed(ExecutionContext*) {
  idle_tasks_.clear();
}

void ScriptedIdleTaskController::Pause() {
  paused_ = true;
}

void ScriptedIdleTaskController::Unpause() {
  DCHECK(paused_);
  paused_ = false;

  // Run any pending timeouts.
  Vector<CallbackId> pending_timeouts;
  pending_timeouts_.swap(pending_timeouts);
  for (auto& id : pending_timeouts)
    RunCallback(id, CurrentTimeTicksInSeconds(),
                IdleDeadline::CallbackType::kCalledByTimeout);

  // Repost idle tasks for any remaining callbacks.
  for (auto& idle_task : idle_tasks_) {
    scoped_refptr<internal::IdleRequestCallbackWrapper> callback_wrapper =
        internal::IdleRequestCallbackWrapper::Create(idle_task.key, this);
    scheduler_->PostIdleTask(
        FROM_HERE,
        WTF::Bind(&internal::IdleRequestCallbackWrapper::IdleTaskFired,
                  callback_wrapper));
  }
}

}  // namespace blink
