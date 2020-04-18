// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_WM_GESTURES_OVERVIEW_GESTURE_HANDLER_H_
#define ASH_WM_GESTURES_OVERVIEW_GESTURE_HANDLER_H_

#include "ash/ash_export.h"
#include "base/macros.h"

namespace ui {
class ScrollEvent;
}

namespace ash {

// This handles 3-finger touchpad scroll events to enter/exit overview mode.
class ASH_EXPORT OverviewGestureHandler {
 public:
  OverviewGestureHandler();
  virtual ~OverviewGestureHandler();

  // Processes a scroll event and may start overview. Returns true if the event
  // has been handled and should not be processed further, false otherwise.
  bool ProcessScrollEvent(const ui::ScrollEvent& event);

 private:
  friend class OverviewGestureHandlerTest;

  // The total distance scrolled with three fingers up to the point when an
  // action is triggered. When the action (enter / exit overview mode or move
  // selection in overview) is triggered those values are reset to zero.
  float scroll_x_;
  float scroll_y_;

  // The threshold before engaging overview with a touchpad three-finger scroll.
  static const float vertical_threshold_pixels_;

  // The threshold before moving selector horizontally when using a touchpad
  // two or three-finger scroll.
  static const float horizontal_threshold_pixels_;

  DISALLOW_COPY_AND_ASSIGN(OverviewGestureHandler);
};

}  // namespace ash

#endif  // ASH_WM_GESTURES_OVERVIEW_GESTURE_HANDLER_H_
