// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/ui/ws/event_injector.h"

#include "services/ui/ws/display.h"
#include "services/ui/ws/display_manager.h"
#include "services/ui/ws/platform_display.h"
#include "services/ui/ws/window_manager_state.h"
#include "services/ui/ws/window_server.h"

namespace ui {
namespace ws {

EventInjector::EventInjector(WindowServer* server) : window_server_(server) {}

EventInjector::~EventInjector() {}

void EventInjector::AdjustEventLocationForPixelLayout(Display* display,
                                                      ui::LocatedEvent* event) {
  WindowManagerState* window_manager_state =
      window_server_->GetWindowManagerState();
  if (!window_manager_state)
    return;

  // Only need to adjust the location of events when there is capture.
  PlatformDisplay* platform_display_with_capture =
      window_manager_state->platform_display_with_capture();
  if (!platform_display_with_capture ||
      display->platform_display() == platform_display_with_capture) {
    return;
  }

  // The event is from a display other than the display with capture. On device
  // events originate from the display with capture and are in terms of the
  // pixel layout (see comments in EventLocation). Convert the location to be
  // relative to the display with capture in terms of the pixel layout to match
  // what happens on device.
  gfx::PointF capture_relative_location = event->location_f();
  capture_relative_location += display->GetViewportMetrics()
                                   .bounds_in_pixels.origin()
                                   .OffsetFromOrigin();
  capture_relative_location -=
      platform_display_with_capture->GetViewportMetrics()
          .bounds_in_pixels.origin()
          .OffsetFromOrigin();
  event->set_location_f(capture_relative_location);
  event->set_root_location_f(capture_relative_location);
}

void EventInjector::InjectEvent(int64_t display_id,
                                std::unique_ptr<ui::Event> event,
                                InjectEventCallback cb) {
  DisplayManager* manager = window_server_->display_manager();
  if (!manager) {
    DVLOG(1) << "No display manager in InjectEvent.";
    std::move(cb).Run(false);
    return;
  }

  Display* display = manager->GetDisplayById(display_id);
  if (!display) {
    DVLOG(1) << "Invalid display_id in InjectEvent.";
    std::move(cb).Run(false);
    return;
  }

  if (event->IsLocatedEvent()) {
    LocatedEvent* located_event = event->AsLocatedEvent();
    if (located_event->root_location_f() != located_event->location_f()) {
      DVLOG(1) << "EventInjector::InjectEvent locations must match";
      std::move(cb).Run(false);
      return;
    }

    AdjustEventLocationForPixelLayout(display, located_event);

    // If this is a mouse pointer event, then we have to also update the
    // location of the cursor on the screen.
    if (event->IsMousePointerEvent())
      display->platform_display()->MoveCursorTo(located_event->location());
  }
  display->ProcessEvent(event.get(), base::BindOnce(std::move(cb), true));
}

}  // namespace ws
}  // namespace ui
