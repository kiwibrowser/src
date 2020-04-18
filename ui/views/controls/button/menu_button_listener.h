// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_VIEWS_CONTROLS_BUTTON_MENU_BUTTON_LISTENER_H_
#define UI_VIEWS_CONTROLS_BUTTON_MENU_BUTTON_LISTENER_H_

#include "ui/views/views_export.h"

namespace gfx {
class Point;
}

namespace ui {
class Event;
}

namespace views {
class MenuButton;

// An interface implemented by an object to let it know that a menu button was
// clicked.
class VIEWS_EXPORT MenuButtonListener {
 public:
  // Notifies that the MenuButton has been clicked. |point| is the default
  // point to display the menu, and |event| is the event causing the click, if
  // any. (Note: "Clicked" refers to any activation, including e.g. accelerators
  // and key events).
  virtual void OnMenuButtonClicked(MenuButton* source,
                                   const gfx::Point& point,
                                   const ui::Event* event) = 0;

 protected:
  virtual ~MenuButtonListener() {}
};

}  // namespace views

#endif  // UI_VIEWS_CONTROLS_BUTTON_MENU_BUTTON_LISTENER_H_
