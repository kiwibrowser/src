// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/wm/panels/panel_window_event_handler.h"

#include "ash/wm/window_state.h"
#include "ash/wm/window_util.h"
#include "base/metrics/user_metrics.h"
#include "ui/aura/window.h"
#include "ui/aura/window_delegate.h"
#include "ui/base/hit_test.h"
#include "ui/events/event.h"

namespace ash {

PanelWindowEventHandler::PanelWindowEventHandler() = default;

PanelWindowEventHandler::~PanelWindowEventHandler() = default;

void PanelWindowEventHandler::OnMouseEvent(ui::MouseEvent* event) {
  aura::Window* target = static_cast<aura::Window*>(event->target());
  if (!event->handled() && event->type() == ui::ET_MOUSE_PRESSED &&
      event->flags() & ui::EF_IS_DOUBLE_CLICK &&
      event->IsOnlyLeftMouseButton() &&
      wm::GetNonClientComponent(target, event->location()) == HTCAPTION) {
    base::RecordAction(base::UserMetricsAction("Panel_Minimize_Caption_Click"));
    wm::GetWindowState(target)->Minimize();
    event->StopPropagation();
    return;
  }
}

void PanelWindowEventHandler::OnGestureEvent(ui::GestureEvent* event) {
  aura::Window* target = static_cast<aura::Window*>(event->target());
  if (!event->handled() && event->type() == ui::ET_GESTURE_TAP &&
      event->details().tap_count() == 2 &&
      wm::GetNonClientComponent(target, event->location()) == HTCAPTION) {
    base::RecordAction(
        base::UserMetricsAction("Panel_Minimize_Caption_Gesture"));
    wm::GetWindowState(target)->Minimize();
    event->StopPropagation();
    return;
  }
}

}  // namespace ash
