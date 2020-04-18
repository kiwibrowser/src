// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/ui/ws/event_dispatcher_impl_test_api.h"

#include "services/ui/ws/event_dispatcher_impl.h"
#include "services/ui/ws/window_tree.h"

namespace ui {
namespace ws {

EventDispatcherImplTestApi::EventDispatcherImplTestApi(
    EventDispatcherImpl* event_dispatcher)
    : event_dispatcher_(event_dispatcher) {}

EventDispatcherImplTestApi::~EventDispatcherImplTestApi() = default;

void EventDispatcherImplTestApi::DispatchInputEventToWindow(
    ServerWindow* target,
    ClientSpecificId client_id,
    const EventLocation& event_location,
    const Event& event,
    Accelerator* accelerator) {
  event_dispatcher_->DispatchInputEventToWindow(
      target, client_id, event_location, event, accelerator);
}

WindowTree* EventDispatcherImplTestApi::GetTreeThatWillAckEvent() {
  return event_dispatcher_->in_flight_event_dispatch_details_
             ? static_cast<WindowTree*>(
                   event_dispatcher_->in_flight_event_dispatch_details_
                       ->async_event_dispatcher)
             : nullptr;
}

bool EventDispatcherImplTestApi::is_event_tasks_empty() {
  return event_dispatcher_->event_tasks_.empty();
}

bool EventDispatcherImplTestApi::OnDispatchInputEventDone(
    mojom::EventResult result) {
  if (!event_dispatcher_->GetInFlightEvent())
    return false;
  event_dispatcher_->OnDispatchInputEventDone(result);
  return true;
}

void EventDispatcherImplTestApi::OnDispatchInputEventTimeout() {
  if (event_dispatcher_->GetInFlightEvent())
    event_dispatcher_->OnDispatchInputEventTimeout();
}

}  // namespace ws
}  // namespace ui
