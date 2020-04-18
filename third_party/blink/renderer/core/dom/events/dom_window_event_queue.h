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

#ifndef THIRD_PARTY_BLINK_RENDERER_CORE_DOM_EVENTS_DOM_WINDOW_EVENT_QUEUE_H_
#define THIRD_PARTY_BLINK_RENDERER_CORE_DOM_EVENTS_DOM_WINDOW_EVENT_QUEUE_H_

#include "third_party/blink/renderer/core/dom/events/event_queue.h"
#include "third_party/blink/renderer/platform/wtf/linked_hash_set.h"

namespace blink {

class Event;
class DOMWindowEventQueueTimer;
class ExecutionContext;

class DOMWindowEventQueue final : public EventQueue {
 public:
  static DOMWindowEventQueue* Create(ExecutionContext*);
  ~DOMWindowEventQueue() override;

  // EventQueue
  void Trace(blink::Visitor*) override;
  bool EnqueueEvent(const base::Location&, Event*) override;
  bool CancelEvent(Event*) override;
  void Close() override;

 private:
  explicit DOMWindowEventQueue(ExecutionContext*);

  void PendingEventTimerFired();
  void DispatchEvent(Event*);

  Member<DOMWindowEventQueueTimer> pending_event_timer_;
  HeapLinkedHashSet<Member<Event>> queued_events_;
  bool is_closed_;

  friend class DOMWindowEventQueueTimer;
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_CORE_DOM_EVENTS_DOM_WINDOW_EVENT_QUEUE_H_
