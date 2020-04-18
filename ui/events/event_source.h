// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_EVENTS_EVENT_SOURCE_H_
#define UI_EVENTS_EVENT_SOURCE_H_

#include <vector>

#include "base/macros.h"
#include "ui/events/event_dispatcher.h"
#include "ui/events/events_export.h"

namespace ui {

class Event;
class EventSink;
class EventRewriter;

// EventSource receives events from the native platform (e.g. X11, win32 etc.)
// and sends the events to an EventSink.
class EVENTS_EXPORT EventSource {
 public:
  EventSource();
  virtual ~EventSource();

  virtual EventSink* GetEventSink() = 0;

  // Adds a rewriter to modify events before they are sent to the
  // EventSink. The rewriter must be explicitly removed from the
  // EventSource before the rewriter is destroyed. The EventSource
  // does not take ownership of the rewriter.
  void AddEventRewriter(EventRewriter* rewriter);
  void RemoveEventRewriter(EventRewriter* rewriter);

 protected:
  // Sends the event through all rewriters and onto the source's EventSink.
  EventDispatchDetails SendEventToSink(Event* event);

  // Sends the event through the rewriters and onto the source's EventSink.
  // If |rewriter| is valid, |event| is only sent to the subsequent rewriters.
  // This is used for asynchronous reposting of events processed by |rewriter|.
  EventDispatchDetails SendEventToSinkFromRewriter(
      Event* event,
      const EventRewriter* rewriter);

 private:
  friend class EventRewriter;
  friend class EventSourceTestApi;

  EventDispatchDetails DeliverEventToSink(Event* event);

  typedef std::vector<EventRewriter*> EventRewriterList;
  EventRewriterList rewriter_list_;

  DISALLOW_COPY_AND_ASSIGN(EventSource);
};

}  // namespace ui

#endif // UI_EVENTS_EVENT_SOURCE_H_
