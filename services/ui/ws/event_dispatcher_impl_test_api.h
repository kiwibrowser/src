// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_UI_WS_EVENT_DISPATCHER_IMPL_TEST_API_H_
#define SERVICES_UI_WS_EVENT_DISPATCHER_IMPL_TEST_API_H_

#include "base/macros.h"
#include "services/ui/common/types.h"

namespace ui {

class Event;

namespace mojom {
enum class EventResult;
}

namespace ws {

class Accelerator;
class EventDispatcherImpl;
class ServerWindow;
class WindowTree;

struct EventLocation;

// Allows accessing internal functions of EventDispatcherImpl for testing. Most
// functions call through to a function of the same name on EventDispatcherImpl,
// see EventDispatcherImpl for details.
class EventDispatcherImplTestApi {
 public:
  explicit EventDispatcherImplTestApi(EventDispatcherImpl* event_dispatcher);
  ~EventDispatcherImplTestApi();

  void DispatchInputEventToWindow(ServerWindow* target,
                                  ClientSpecificId client_id,
                                  const EventLocation& event_location,
                                  const Event& event,
                                  Accelerator* accelerator);

  // Convenience for returning
  // |in_flight_event_dispatch_details_->async_event_dispatcher_| as a
  // WindowTree.
  WindowTree* GetTreeThatWillAckEvent();

  bool is_event_tasks_empty();

  bool OnDispatchInputEventDone(mojom::EventResult result);

  void OnDispatchInputEventTimeout();

 private:
  EventDispatcherImpl* event_dispatcher_;

  DISALLOW_COPY_AND_ASSIGN(EventDispatcherImplTestApi);
};

}  // namespace ws
}  // namespace ui

#endif  // SERVICES_UI_WS_EVENT_DISPATCHER_IMPL_TEST_API_H_
