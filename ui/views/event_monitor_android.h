// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_VIEWS_EVENT_MONITOR_ANDROID_H_
#define UI_VIEWS_EVENT_MONITOR_ANDROID_H_

#include "base/macros.h"
#include "ui/views/event_monitor.h"

namespace ui {
class EventTarget;
}

namespace views {

class EventMonitorAndroid : public EventMonitor {
 public:
  EventMonitorAndroid(ui::EventHandler* event_handler,
                   ui::EventTarget* event_target);
  ~EventMonitorAndroid() override;

 private:
  ui::EventHandler* event_handler_;  // Weak. Owned by our owner.
  ui::EventTarget* event_target_;    // Weak.

  DISALLOW_COPY_AND_ASSIGN(EventMonitorAndroid);
};

}  // namespace views

#endif  // UI_VIEWS_EVENT_MONITOR_ANDROID_H_
