// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/views/event_monitor_aura.h"

#include "base/logging.h"
#include "base/memory/ptr_util.h"
#include "ui/aura/env.h"
#include "ui/aura/window.h"
#include "ui/events/event_target.h"

namespace views {

// static
std::unique_ptr<EventMonitor> EventMonitor::CreateApplicationMonitor(
    ui::EventHandler* event_handler) {
  return base::WrapUnique(
      new EventMonitorAura(event_handler, aura::Env::GetInstance()));
}

// static
std::unique_ptr<EventMonitor> EventMonitor::CreateWindowMonitor(
    ui::EventHandler* event_handler,
    gfx::NativeWindow target_window) {
  return base::WrapUnique(new EventMonitorAura(event_handler, target_window));
}

// static
gfx::Point EventMonitor::GetLastMouseLocation() {
  return aura::Env::GetInstance()->last_mouse_location();
}

EventMonitorAura::EventMonitorAura(ui::EventHandler* event_handler,
                                   ui::EventTarget* event_target)
    : event_handler_(event_handler), event_target_(event_target) {
  DCHECK(event_handler_);
  DCHECK(event_target_);
  event_target_->AddPreTargetHandler(event_handler_);
}

EventMonitorAura::~EventMonitorAura() {
  event_target_->RemovePreTargetHandler(event_handler_);
}

}  // namespace views
