// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_VIEWS_EVENT_MONITOR_H_
#define UI_VIEWS_EVENT_MONITOR_H_

#include <memory>

#include "ui/gfx/geometry/point.h"
#include "ui/gfx/native_widget_types.h"
#include "ui/views/views_export.h"

namespace ui {
class EventHandler;
}

namespace views {

// RAII-style class that forwards events to |event_handler| before they are
// dispatched.
class VIEWS_EXPORT EventMonitor {
 public:
  virtual ~EventMonitor() {}

  // Create an instance for monitoring application events.
  // Events will be forwarded to |event_handler| before they are dispatched to
  // the application.
  static std::unique_ptr<EventMonitor> CreateApplicationMonitor(
      ui::EventHandler* event_handler);

  // Create an instance for monitoring events on a specific window.
  // Events will be forwarded to |event_handler| before they are dispatched to
  // |target_window|.
  // The EventMonitor instance must be destroyed before |target_window|.
  static std::unique_ptr<EventMonitor> CreateWindowMonitor(
      ui::EventHandler* event_handler,
      gfx::NativeWindow target_window);

  // Returns the last mouse location seen in a mouse event in screen
  // coordinates.
  static gfx::Point GetLastMouseLocation();
};

}  // namespace views

#endif  // UI_VIEWS_EVENT_MONITOR_H_
