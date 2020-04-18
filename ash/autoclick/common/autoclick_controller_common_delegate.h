// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_AUTOCLICK_COMMON_AUTOCLICK_CONTROLLER_COMMON_DELEGATE_H
#define ASH_AUTOCLICK_COMMON_AUTOCLICK_CONTROLLER_COMMON_DELEGATE_H

namespace views {
class Widget;
}

namespace gfx {
class Point;
}

namespace ash {

class AutoclickControllerCommonDelegate {
 public:
  // Creates a ring widget at |point_in_screen|.
  // AutoclickControllerCommonDelegate still has ownership of the widget they
  // created.
  virtual views::Widget* CreateAutoclickRingWidget(
      const gfx::Point& point_in_screen) = 0;

  // Moves |widget| to |point_in_screen|. The point may be on a different
  // display.
  virtual void UpdateAutoclickRingWidget(views::Widget* widget,
                                         const gfx::Point& point_in_screen) = 0;

  // Generates a click at |point_in_screen|. |mouse_event_flags| may contain key
  // modifiers (e.g. shift, control) for the click.
  virtual void DoAutoclick(const gfx::Point& point_in_screen,
                           const int mouse_event_flags) = 0;

  virtual void OnAutoclickCanceled() = 0;

 protected:
  virtual ~AutoclickControllerCommonDelegate() {}
};

}  // namespace ash

#endif  // ASH_AUTOCLICK_COMMON_AUTOCLICK_CONTROLLER_COMMON_DELEGATE_H
