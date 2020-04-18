// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/events/ozone/events_ozone.h"

#include "ui/events/event.h"

namespace ui {

void DispatchEventFromNativeUiEvent(
    const PlatformEvent& event,
    base::OnceCallback<void(ui::Event*)> callback) {
  // NB: ui::Events are constructed here using the overload that takes a
  // const PlatformEvent& (here ui::Event* const&) rather than the copy
  // constructor. This has side effects and cannot be changed to use the
  // copy constructor or Event::Clone.
  if (event->IsKeyEvent()) {
    ui::KeyEvent key_event(event);
    std::move(callback).Run(&key_event);
  } else if (event->IsMouseWheelEvent()) {
    ui::MouseWheelEvent wheel_event(event);
    std::move(callback).Run(&wheel_event);
  } else if (event->IsMouseEvent()) {
    ui::MouseEvent mouse_event(event);
    std::move(callback).Run(&mouse_event);
  } else if (event->IsTouchEvent()) {
    ui::TouchEvent touch_event(event);
    std::move(callback).Run(&touch_event);
  } else if (event->IsScrollEvent()) {
    ui::ScrollEvent scroll_event(event);
    std::move(callback).Run(&scroll_event);
  } else if (event->IsGestureEvent()) {
    std::move(callback).Run(event);
    // TODO(mohsen): Use the same pattern for scroll/touch/wheel events.
    // Apparently, there is no need for them to wrap the |event|.
  } else {
    NOTREACHED();
  }
}

}  // namespace ui
