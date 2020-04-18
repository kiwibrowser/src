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

#ifndef THIRD_PARTY_BLINK_RENDERER_CORE_DOM_EVENTS_MEDIA_ELEMENT_EVENT_QUEUE_H_
#define THIRD_PARTY_BLINK_RENDERER_CORE_DOM_EVENTS_MEDIA_ELEMENT_EVENT_QUEUE_H_

#include "base/location.h"
#include "base/memory/scoped_refptr.h"
#include "third_party/blink/renderer/core/core_export.h"
#include "third_party/blink/renderer/core/dom/events/event_queue.h"
#include "third_party/blink/renderer/core/dom/events/event_target.h"
#include "third_party/blink/renderer/platform/timer.h"
#include "third_party/blink/renderer/platform/wtf/vector.h"

namespace blink {

// Queue for events originating in MediaElement and having
// "media element event" task type according to the spec.
class CORE_EXPORT MediaElementEventQueue final : public EventQueue {
 public:
  static MediaElementEventQueue* Create(EventTarget*, ExecutionContext*);
  ~MediaElementEventQueue() override;

  // EventQueue
  void Trace(blink::Visitor*) override;
  bool EnqueueEvent(const base::Location&, Event*) override;
  bool CancelEvent(Event*) override;
  void Close() override;

  void CancelAllEvents();
  bool HasPendingEvents() const;

 private:
  MediaElementEventQueue(EventTarget*, ExecutionContext*);
  void TimerFired(TimerBase*);

  Member<EventTarget> owner_;
  HeapVector<Member<Event>> pending_events_;
  TaskRunnerTimer<MediaElementEventQueue> timer_;

  bool is_closed_;
};

}  // namespace blink

#endif
