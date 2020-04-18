// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_EVENTS_EVENT_REWRITER_H_
#define UI_EVENTS_EVENT_REWRITER_H_

#include <memory>

#include "base/macros.h"
#include "ui/events/event_dispatcher.h"
#include "ui/events/events_export.h"

namespace ui {

class Event;
class EventSource;

// Return status of EventRewriter operations; see that class below.
enum EventRewriteStatus {
  // Nothing was done; no rewritten event returned. Pass the original
  // event to later rewriters, or send it to the EventSink if this
  // was the final rewriter.
  EVENT_REWRITE_CONTINUE,

  // The event has been rewritten. Send the rewritten event to the
  // EventSink instead of the original event (without sending
  // either to any later rewriters).
  EVENT_REWRITE_REWRITTEN,

  // The event should be discarded, neither passing it to any later
  // rewriters nor sending it to the EventSink.
  EVENT_REWRITE_DISCARD,

  // The event has been rewritten. As for EVENT_REWRITE_REWRITTEN,
  // send the rewritten event to the EventSink instead of the original
  // event (without sending either to any later rewriters).
  // In addition the rewriter has one or more additional new events
  // to be retrieved using |NextDispatchEvent()| and sent to the
  // EventSink.
  EVENT_REWRITE_DISPATCH_ANOTHER,
};

// EventRewriter provides a mechanism for Events to be rewritten
// before being dispatched from EventSource to EventSink.
class EVENTS_EXPORT EventRewriter {
 public:
  EventRewriter() = default;
  virtual ~EventRewriter() = default;

  // Potentially rewrites (replaces) an event, or requests it be discarded.
  // or discards an event. If the rewriter wants to rewrite an event, and
  // dispatch another event once the rewritten event is dispatched, it should
  // return EVENT_REWRITE_DISPATCH_ANOTHER, and return the next event to
  // dispatch from |NextDispatchEvent()|.
  virtual EventRewriteStatus RewriteEvent(
      const Event& event,
      std::unique_ptr<Event>* rewritten_event) = 0;

  // Supplies an additional event to be dispatched. It is only valid to
  // call this after the immediately previous call to |RewriteEvent()|
  // or |NextDispatchEvent()| has returned EVENT_REWRITE_DISPATCH_ANOTHER.
  // Should only return either EVENT_REWRITE_REWRITTEN or
  // EVENT_REWRITE_DISPATCH_ANOTHER; otherwise the previous call should not
  // have returned EVENT_REWRITE_DISPATCH_ANOTHER.
  virtual EventRewriteStatus NextDispatchEvent(
      const Event& last_event,
      std::unique_ptr<Event>* new_event) = 0;

 protected:
  // A helper that calls a protected EventSource function, which sends the event
  // to subsequent event rewriters on the source and onto its event sink.
  EventDispatchDetails SendEventToEventSource(EventSource* source,
                                              Event* event) const;

 private:
  DISALLOW_COPY_AND_ASSIGN(EventRewriter);
};

}  // namespace ui

#endif  // UI_EVENTS_EVENT_REWRITER_H_
