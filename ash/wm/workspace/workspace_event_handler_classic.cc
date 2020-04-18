// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/wm/workspace/workspace_event_handler_classic.h"

#include "ash/wm/window_util.h"
#include "ui/aura/window.h"
#include "ui/events/event.h"

namespace ash {

WorkspaceEventHandlerClassic::WorkspaceEventHandlerClassic(
    aura::Window* workspace_window)
    : workspace_window_(workspace_window) {
  wm::AddLimitedPreTargetHandlerForWindow(this, workspace_window_);
}

WorkspaceEventHandlerClassic::~WorkspaceEventHandlerClassic() {
  wm::RemoveLimitedPreTargetHandlerForWindow(this, workspace_window_);
}

void WorkspaceEventHandlerClassic::OnMouseEvent(ui::MouseEvent* event) {
  WorkspaceEventHandler::OnMouseEvent(
      event, static_cast<aura::Window*>(event->target()));
}

void WorkspaceEventHandlerClassic::OnGestureEvent(ui::GestureEvent* event) {
  WorkspaceEventHandler::OnGestureEvent(
      event, static_cast<aura::Window*>(event->target()));
}

}  // namespace ash
