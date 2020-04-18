// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_TOUCH_HUD_TOUCH_HUD_RENDERER_H_
#define ASH_TOUCH_HUD_TOUCH_HUD_RENDERER_H_

#include <map>

#include "ash/touch_hud/ash_touch_hud_export.h"
#include "base/macros.h"

namespace ui {
class LocatedEvent;
}

namespace views {
class Widget;
}

namespace ash {
class TouchHudProjectionTest;
class TouchPointView;

// Handles touch events to draw out touch points accordingly.
// TODO(jamescook): Delete this when the mojo touch_hud_app is the default.
// https://crbug.com/840380
class ASH_TOUCH_HUD_EXPORT TouchHudRenderer {
 public:
  explicit TouchHudRenderer(views::Widget* parent_widget);
  ~TouchHudRenderer();

  // Called to clear touch points and traces from the screen.
  void Clear();

  // Receives a touch event and draws its touch point.
  void HandleTouchEvent(const ui::LocatedEvent& event);

 private:
  friend class TouchHudProjectionTest;

  // The parent widget that all touch points would be drawn in.
  views::Widget* parent_widget_;

  // A map of touch ids to TouchPointView.
  std::map<int, TouchPointView*> points_;

  DISALLOW_COPY_AND_ASSIGN(TouchHudRenderer);
};

}  // namespace ash

#endif  // ASH_TOUCH_HUD_TOUCH_HUD_RENDERER_H_
