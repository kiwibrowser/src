/*
 * Copyright (C) 2010 Google Inc. All Rights Reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE COMPUTER, INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#include "third_party/blink/renderer/core/workers/worker_event_queue.h"

#include "third_party/blink/public/platform/task_type.h"
#include "third_party/blink/renderer/core/dom/events/event.h"
#include "third_party/blink/renderer/core/probe/core_probes.h"
#include "third_party/blink/renderer/core/workers/worker_or_worklet_global_scope.h"
#include "third_party/blink/renderer/core/workers/worker_thread.h"

namespace blink {

WorkerEventQueue* WorkerEventQueue::Create(
    WorkerOrWorkletGlobalScope* global_scope) {
  return new WorkerEventQueue(global_scope);
}

WorkerEventQueue::WorkerEventQueue(WorkerOrWorkletGlobalScope* global_scope)
    : global_scope_(global_scope) {}

WorkerEventQueue::~WorkerEventQueue() {
  DCHECK(pending_events_.IsEmpty());
}

void WorkerEventQueue::Trace(blink::Visitor* visitor) {
  visitor->Trace(global_scope_);
  visitor->Trace(pending_events_);
  EventQueue::Trace(visitor);
}

bool WorkerEventQueue::EnqueueEvent(const base::Location& from_here,
                                    Event* event) {
  if (is_closed_)
    return false;
  probe::AsyncTaskScheduled(event->target()->GetExecutionContext(),
                            event->type(), event);
  pending_events_.insert(event);
  // This queue is unthrottled because throttling event tasks may break existing
  // web pages. For example, throttling IndexedDB events may break scenarios
  // where several tabs, some of which are backgrounded, access the same
  // database concurrently. See also comments in the ctor of
  // DOMWindowEventQueueTimer.
  // TODO(nhiroki): Callers of enqueueEvent() should specify the task type.
  global_scope_->GetTaskRunner(TaskType::kInternalWorker)
      ->PostTask(from_here,
                 WTF::Bind(&WorkerEventQueue::DispatchEvent,
                           WrapPersistent(this), WrapWeakPersistent(event)));
  return true;
}

bool WorkerEventQueue::CancelEvent(Event* event) {
  if (!RemoveEvent(event))
    return false;
  probe::AsyncTaskCanceled(event->target()->GetExecutionContext(), event);
  return true;
}

void WorkerEventQueue::Close() {
  is_closed_ = true;
  for (const auto& event : pending_events_)
    probe::AsyncTaskCanceled(event->target()->GetExecutionContext(), event);
  pending_events_.clear();
}

bool WorkerEventQueue::RemoveEvent(Event* event) {
  auto found = pending_events_.find(event);
  if (found == pending_events_.end())
    return false;
  pending_events_.erase(found);
  return true;
}

void WorkerEventQueue::DispatchEvent(Event* event) {
  if (!event || !RemoveEvent(event))
    return;

  probe::AsyncTask async_task(global_scope_, event);
  event->target()->DispatchEvent(event);
}

}  // namespace blink
