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

#include "third_party/blink/renderer/core/dom/events/dom_window_event_queue.h"

#include "base/macros.h"
#include "third_party/blink/public/platform/task_type.h"
#include "third_party/blink/renderer/core/dom/events/event.h"
#include "third_party/blink/renderer/core/frame/local_dom_window.h"
#include "third_party/blink/renderer/core/frame/pausable_timer.h"
#include "third_party/blink/renderer/core/probe/core_probes.h"

namespace blink {

class DOMWindowEventQueueTimer final
    : public GarbageCollectedFinalized<DOMWindowEventQueueTimer>,
      public PausableTimer {
  USING_GARBAGE_COLLECTED_MIXIN(DOMWindowEventQueueTimer);

 public:
  DOMWindowEventQueueTimer(DOMWindowEventQueue* event_queue,
                           ExecutionContext* context)
      // This queue is unthrottled because throttling IndexedDB events may break
      // scenarios where several tabs, some of which are backgrounded, access
      // the same database concurrently.
      : PausableTimer(context, TaskType::kUnthrottled),
        event_queue_(event_queue) {}

  // Eager finalization is needed to promptly stop this timer object.
  // (see DOMTimer comment for more.)
  EAGERLY_FINALIZE();
  void Trace(blink::Visitor* visitor) override {
    visitor->Trace(event_queue_);
    PausableTimer::Trace(visitor);
  }

 private:
  void Fired() override { event_queue_->PendingEventTimerFired(); }

  Member<DOMWindowEventQueue> event_queue_;
  DISALLOW_COPY_AND_ASSIGN(DOMWindowEventQueueTimer);
};

DOMWindowEventQueue* DOMWindowEventQueue::Create(ExecutionContext* context) {
  return new DOMWindowEventQueue(context);
}

DOMWindowEventQueue::DOMWindowEventQueue(ExecutionContext* context)
    : pending_event_timer_(new DOMWindowEventQueueTimer(this, context)),
      is_closed_(false) {
  pending_event_timer_->PauseIfNeeded();
}

DOMWindowEventQueue::~DOMWindowEventQueue() = default;

void DOMWindowEventQueue::Trace(blink::Visitor* visitor) {
  visitor->Trace(pending_event_timer_);
  visitor->Trace(queued_events_);
  EventQueue::Trace(visitor);
}

bool DOMWindowEventQueue::EnqueueEvent(const base::Location& from_here,
                                       Event* event) {
  if (is_closed_)
    return false;

  DCHECK(event->target());
  probe::AsyncTaskScheduled(event->target()->GetExecutionContext(),
                            event->type(), event);

  bool was_added = queued_events_.insert(event).is_new_entry;
  DCHECK(was_added);  // It should not have already been in the list.

  if (!pending_event_timer_->IsActive())
    pending_event_timer_->StartOneShot(TimeDelta(), from_here);

  return true;
}

bool DOMWindowEventQueue::CancelEvent(Event* event) {
  auto it = queued_events_.find(event);
  bool found = it != queued_events_.end();
  if (found) {
    probe::AsyncTaskCanceled(event->target()->GetExecutionContext(), event);
    queued_events_.erase(it);
  }
  if (queued_events_.IsEmpty())
    pending_event_timer_->Stop();
  return found;
}

void DOMWindowEventQueue::Close() {
  is_closed_ = true;
  pending_event_timer_->Stop();
  for (const auto& queued_event : queued_events_) {
    if (queued_event) {
      probe::AsyncTaskCanceled(queued_event->target()->GetExecutionContext(),
                               queued_event);
    }
  }
  queued_events_.clear();
}

void DOMWindowEventQueue::PendingEventTimerFired() {
  DCHECK(!pending_event_timer_->IsActive());
  DCHECK(!queued_events_.IsEmpty());

  // Insert a marker for where we should stop.
  DCHECK(!queued_events_.Contains(nullptr));
  bool was_added = queued_events_.insert(nullptr).is_new_entry;
  DCHECK(was_added);  // It should not have already been in the list.

  while (!queued_events_.IsEmpty()) {
    auto it = queued_events_.begin();
    Event* event = *it;
    queued_events_.erase(it);
    if (!event)
      break;
    DispatchEvent(event);
  }
}

void DOMWindowEventQueue::DispatchEvent(Event* event) {
  EventTarget* event_target = event->target();
  probe::AsyncTask async_task(event_target->GetExecutionContext(), event);
  if (LocalDOMWindow* window = event_target->ToLocalDOMWindow())
    window->DispatchEvent(event, nullptr);
  else
    event_target->DispatchEvent(event);
}

}  // namespace blink
