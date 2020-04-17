// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/views/event_monitor_android.h"

#include "base/logging.h"
#include "base/memory/ptr_util.h"
#include "ui/events/event_target.h"

namespace views {

// static
std::unique_ptr<EventMonitor> EventMonitor::CreateApplicationMonitor(
    ui::EventHandler* event_handler) {
  return base::WrapUnique(
      new EventMonitorAndroid(event_handler, nullptr));
}

// static
std::unique_ptr<EventMonitor> EventMonitor::CreateWindowMonitor(
    ui::EventHandler* event_handler,
    gfx::NativeWindow target_window) {
  return base::WrapUnique(new EventMonitorAndroid(event_handler, nullptr));
}

// static
gfx::Point EventMonitor::GetLastMouseLocation() {
  return gfx::Point();
}

EventMonitorAndroid::EventMonitorAndroid(ui::EventHandler* event_handler,
                                   ui::EventTarget* event_target)
    : event_handler_(event_handler), event_target_(event_target) {
  event_handler_ = nullptr;
  event_target_ = nullptr;
}

EventMonitorAndroid::~EventMonitorAndroid() {
}

}  // namespace views
