/*
 * Copyright (C) 2012 Victor Carbune (victor@rosedu.org)
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
 */

#include "third_party/blink/renderer/core/dom/events/media_element_event_queue.h"

#include "third_party/blink/public/platform/task_type.h"
#include "third_party/blink/renderer/core/dom/events/event.h"
#include "third_party/blink/renderer/core/probe/core_probes.h"
#include "third_party/blink/renderer/platform/instrumentation/tracing/trace_event.h"

namespace blink {

MediaElementEventQueue* MediaElementEventQueue::Create(
    EventTarget* owner,
    ExecutionContext* context) {
  return new MediaElementEventQueue(owner, context);
}

MediaElementEventQueue::MediaElementEventQueue(EventTarget* owner,
                                               ExecutionContext* context)
    : owner_(owner),
      timer_(context->GetTaskRunner(TaskType::kMediaElementEvent),
             this,
             &MediaElementEventQueue::TimerFired),
      is_closed_(false) {}

MediaElementEventQueue::~MediaElementEventQueue() = default;

void MediaElementEventQueue::Trace(blink::Visitor* visitor) {
  visitor->Trace(owner_);
  visitor->Trace(pending_events_);
  EventQueue::Trace(visitor);
}

bool MediaElementEventQueue::EnqueueEvent(const base::Location& from_here,
                                          Event* event) {
  if (is_closed_)
    return false;

  if (event->target() == owner_)
    event->SetTarget(nullptr);

  TRACE_EVENT_ASYNC_BEGIN1("event", "MediaElementEventQueue:enqueueEvent",
                           event, "type", event->type().Ascii());
  EventTarget* target = event->target() ? event->target() : owner_.Get();
  probe::AsyncTaskScheduled(target->GetExecutionContext(), event->type(),
                            event);
  pending_events_.push_back(event);

  if (!timer_.IsActive())
    timer_.StartOneShot(TimeDelta(), from_here);

  return true;
}

bool MediaElementEventQueue::CancelEvent(Event* event) {
  bool found = pending_events_.Contains(event);

  if (found) {
    EventTarget* target = event->target() ? event->target() : owner_.Get();
    probe::AsyncTaskCanceled(target->GetExecutionContext(), event);
    pending_events_.EraseAt(pending_events_.Find(event));
    TRACE_EVENT_ASYNC_END2("event", "MediaElementEventQueue:enqueueEvent",
                           event, "type", event->type().Ascii(), "status",
                           "cancelled");
  }

  if (pending_events_.IsEmpty())
    timer_.Stop();

  return found;
}

void MediaElementEventQueue::TimerFired(TimerBase*) {
  DCHECK(!timer_.IsActive());
  DCHECK(!pending_events_.IsEmpty());

  HeapVector<Member<Event>> pending_events;
  pending_events_.swap(pending_events);

  for (const auto& pending_event : pending_events) {
    Event* event = pending_event.Get();
    EventTarget* target = event->target() ? event->target() : owner_.Get();
    CString type(event->type().Ascii());
    probe::AsyncTask async_task(target->GetExecutionContext(), event);
    TRACE_EVENT_ASYNC_STEP_INTO1("event", "MediaElementEventQueue:enqueueEvent",
                                 event, "dispatch", "type", type);
    target->DispatchEvent(pending_event);
    TRACE_EVENT_ASYNC_END1("event", "MediaElementEventQueue:enqueueEvent",
                           event, "type", type);
  }
}

void MediaElementEventQueue::Close() {
  is_closed_ = true;
  CancelAllEvents();
}

void MediaElementEventQueue::CancelAllEvents() {
  timer_.Stop();

  for (const auto& pending_event : pending_events_) {
    Event* event = pending_event.Get();
    TRACE_EVENT_ASYNC_END2("event", "MediaElementEventQueue:enqueueEvent",
                           event, "type", event->type().Ascii(), "status",
                           "cancelled");
    EventTarget* target = event->target() ? event->target() : owner_.Get();
    probe::AsyncTaskCanceled(target->GetExecutionContext(), event);
  }
  pending_events_.clear();
}

bool MediaElementEventQueue::HasPendingEvents() const {
  return pending_events_.size();
}

}  // namespace blink
