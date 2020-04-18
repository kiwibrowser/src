// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/wm/root_window_finder.h"

#include "ash/root_window_controller.h"
#include "ash/shell.h"
#include "ui/aura/window.h"
#include "ui/display/display.h"
#include "ui/display/screen.h"
#include "ui/gfx/geometry/point.h"
#include "ui/gfx/geometry/rect.h"

namespace ash {
namespace wm {

aura::Window* GetRootWindowAt(const gfx::Point& point) {
  const display::Display& display =
      display::Screen::GetScreen()->GetDisplayNearestPoint(point);
  DCHECK(display.is_valid());
  RootWindowController* root_window_controller =
      Shell::GetRootWindowControllerWithDisplayId(display.id());
  return root_window_controller ? root_window_controller->GetRootWindow()
                                : nullptr;
}

aura::Window* GetRootWindowMatching(const gfx::Rect& rect) {
  const display::Display& display =
      display::Screen::GetScreen()->GetDisplayMatching(rect);
  RootWindowController* root_window_controller =
      Shell::GetRootWindowControllerWithDisplayId(display.id());
  return root_window_controller ? root_window_controller->GetRootWindow()
                                : nullptr;
}

}  // namespace wm
}  // namespace ash
