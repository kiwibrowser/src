// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_UI_WS_EVENT_DISPATCHER_H_
#define SERVICES_UI_WS_EVENT_DISPATCHER_H_

#include <stdint.h>

#include "services/ui/common/types.h"

namespace ui {

class Event;

namespace ws {

class Accelerator;
class ServerWindow;

struct EventLocation;

// EventDispatcher is called from EventProcessor once it determines the target
// for events as well as accelerators.
class EventDispatcher {
 public:
  enum class AcceleratorPhase {
    kPre,
    kPost,
  };

  // Called when the target Window is found for |event|.
  // |post_target_accelerator| is the accelerator to run if the target doesn't
  // handle the event. See EventProcessor for details on event processing
  // phases. |client_id| is the id of tree the event should be sent to. See
  // EventLocation for details on |event_location|. |event_location| is only
  // useful for located events.
  virtual void DispatchInputEventToWindow(
      ServerWindow* target,
      ClientSpecificId client_id,
      const EventLocation& event_location,
      const ui::Event& event,
      Accelerator* post_target_accelerator) = 0;

  // A matching accelerator was found for the specified phase of processing.
  // |event| is the event that the accelerator matches and |display_id|
  // identifies the display the event came in on.
  virtual void OnAccelerator(uint32_t accelerator,
                             int64_t display_id,
                             const ui::Event& event,
                             AcceleratorPhase phase) = 0;

 protected:
  virtual ~EventDispatcher() {}
};

}  // namespace ws
}  // namespace ui

#endif  // SERVICES_UI_WS_EVENT_DISPATCHER_H_
