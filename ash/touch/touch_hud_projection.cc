// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/touch/touch_hud_projection.h"

#include "ash/root_window_controller.h"
#include "ash/shell.h"
#include "ash/touch_hud/touch_hud_renderer.h"
#include "ui/events/event.h"
#include "ui/views/widget/widget.h"

namespace ash {

TouchHudProjection::TouchHudProjection(aura::Window* initial_root)
    : TouchObserverHUD(initial_root, "TouchHud"),
      touch_hud_renderer_(new TouchHudRenderer(widget())) {}

TouchHudProjection::~TouchHudProjection() = default;

void TouchHudProjection::Clear() {
  touch_hud_renderer_->Clear();
}

void TouchHudProjection::OnTouchEvent(ui::TouchEvent* event) {
  touch_hud_renderer_->HandleTouchEvent(*event);
}

void TouchHudProjection::SetHudForRootWindowController(
    RootWindowController* controller) {
  controller->set_touch_hud_projection(this);
}

void TouchHudProjection::UnsetHudForRootWindowController(
    RootWindowController* controller) {
  controller->set_touch_hud_projection(NULL);
}

}  // namespace ash
