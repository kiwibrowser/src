// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_UI_WS_EVENT_DISPATCHER_DELEGATE_H_
#define SERVICES_UI_WS_EVENT_DISPATCHER_DELEGATE_H_

#include <stdint.h>

#include "services/ui/ws/ids.h"

namespace ui {

class Event;

namespace ws {

class AsyncEventDispatcher;
class ServerWindow;

struct EventLocation;

// Called at interesting stages during event dispatch.
class EventDispatcherDelegate {
 public:
  // Called immediately before |event| is handed to EventProcessor for
  // processing
  virtual void OnWillProcessEvent(const ui::Event& event,
                                  const EventLocation& event_location) = 0;

  // Called before dispatching an event to an AsyncEventDispatcher. The delegate
  // may return a different ServerWindow to send the event to. Typically the
  // delegate will return |target|.
  virtual ServerWindow* OnWillDispatchInputEvent(
      ServerWindow* target,
      ClientSpecificId client_id,
      const EventLocation& event_location,
      const Event& event) = 0;

  // Called when |async_event_dispatcher| did not complete processing in a
  // reasonable amount of time.
  virtual void OnEventDispatchTimedOut(
      AsyncEventDispatcher* async_event_dispatcher) = 0;

  // Called when an AsyncEventDispatcher handles an event that mapped to an
  // accelerator.
  virtual void OnAsyncEventDispatcherHandledAccelerator(const Event& event,
                                                        int64_t display_id) = 0;

 protected:
  virtual ~EventDispatcherDelegate() {}
};

}  // namespace ws
}  // namespace ui

#endif  // SERVICES_UI_WS_EVENT_DISPATCHER_DELEGATE_H_
