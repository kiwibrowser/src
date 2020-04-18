// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_UI_WS_ASYNC_EVENT_DISPATCHER_H_
#define SERVICES_UI_WS_ASYNC_EVENT_DISPATCHER_H_

#include <stdint.h>

#include <string>

#include "base/callback_forward.h"
#include "base/containers/flat_map.h"

namespace ui {
class Event;
}

namespace ui {
namespace mojom {
enum class EventReslut;
}
namespace ws {

class ServerWindow;

struct EventLocation;

// AsyncEventDispatchers dispatch events asynchronously. A callback is run once
// the accelerator or event is dispatched. This class allows the event
// dispatching code to be independent of WindowTree (for testing and
// modularity).
//
// If an AsyncEventDispatcher does not run the callback in a reasonable amount
// of time, Dispatch* may be called again.
class AsyncEventDispatcher {
 public:
  using DispatchEventCallback = base::OnceCallback<void(mojom::EventResult)>;
  // Dispatches |event| to |target|, running |callback| with the result.
  // |event_location| is only useful for located events.
  virtual void DispatchEvent(ServerWindow* target,
                             const Event& event,
                             const EventLocation& event_location,
                             DispatchEventCallback callback) = 0;

  // In addition to the result of the accelerator, AcceleratorCallback is
  // supplied arbitrary key-value pairs that are added to the event if
  // further processing is necessary (EventResult::UNHANDLED is supplied to the
  // callback). The intepretation of the key-value pairs is left to clients (see
  // ash/public/interfaces/event_properties.mojom for examples).
  using AcceleratorCallback = base::OnceCallback<void(
      mojom::EventResult,
      const base::flat_map<std::string, std::vector<uint8_t>>&)>;

  // Dispatches an accelerator that matches |event|, running the callback with
  // the result and key-value pairs. |accelerator_id| identifies the
  // accelerator.
  virtual void DispatchAccelerator(uint32_t accelerator_id,
                                   const Event& event,
                                   AcceleratorCallback callback) = 0;
};

}  // namespace ws
}  // namespace ui

#endif  // SERVICES_UI_WS_ASYNC_EVENT_DISPATCHER_H_
