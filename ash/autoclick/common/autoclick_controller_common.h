// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_AUTOCLICK_COMMON_AUTOCLICK_CONTROLLER_COMMON_H
#define ASH_AUTOCLICK_COMMON_AUTOCLICK_CONTROLLER_COMMON_H

#include "ash/autoclick/common/autoclick_ring_handler.h"
#include "base/macros.h"
#include "base/timer/timer.h"
#include "ui/gfx/geometry/point.h"

namespace views {
class Widget;
}

namespace ui {
class MouseEvent;
class KeyEvent;
}

namespace ash {
class AutoclickControllerCommonDelegate;

// Autoclick is one of the accessibility features. If enabled, two circles will
// animate at the mouse event location and an automatic click event will happen
// after a certain amout of time at that location.
// AutoclickControllerCommon is the common code for both ash and mus to handle
// events and to manage autoclick time delay and timer.
class AutoclickControllerCommon {
 public:
  AutoclickControllerCommon(base::TimeDelta delay,
                            AutoclickControllerCommonDelegate* delegate);
  ~AutoclickControllerCommon();

  void HandleMouseEvent(const ui::MouseEvent& event);
  void HandleKeyEvent(const ui::KeyEvent& event);

  void SetAutoclickDelay(const base::TimeDelta delay);
  void CancelAutoclick();

 private:
  void InitClickTimer();

  void DoAutoclick();

  void UpdateRingWidget(const gfx::Point& mouse_location);

  base::TimeDelta delay_;
  int mouse_event_flags_;
  std::unique_ptr<base::Timer> autoclick_timer_;
  AutoclickControllerCommonDelegate* delegate_;
  views::Widget* widget_;
  // The position in screen coordinates used to determine
  // the distance the mouse has moved.
  gfx::Point anchor_location_;
  std::unique_ptr<AutoclickRingHandler> autoclick_ring_handler_;

  DISALLOW_COPY_AND_ASSIGN(AutoclickControllerCommon);
};

}  // namespace ash

#endif  // ASH_AUTOCLICK_COMMON_AUTOCLICK_CONTROLLER_COMMON_H
