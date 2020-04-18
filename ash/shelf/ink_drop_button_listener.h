// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_SHELF_INK_DROP_BUTTON_LISTENER_H_
#define ASH_SHELF_INK_DROP_BUTTON_LISTENER_H_

#include "ash/ash_export.h"

namespace ui {
class Event;
}

namespace views {
class Button;
class InkDrop;
}

namespace ash {

// An interface used by buttons on shelf to notify ShelfView when they are
// pressed. |ink_drop| is used to do appropriate ink drop animation based on the
// action performed.
// TODO(mohsen): A better approach would be to return a value indicating the
// type of action performed such that the button can animate the ink drop.
// Currently, it is not possible because showing menu is synchronous and blocks
// the call. Fix this after menu is converted to synchronous.  Long-term, the
// return value can be merged into ButtonListener.
class ASH_EXPORT InkDropButtonListener {
 public:
  virtual void ButtonPressed(views::Button* sender,
                             const ui::Event& event,
                             views::InkDrop* ink_drop) = 0;

 protected:
  virtual ~InkDropButtonListener() {}
};

}  // namespace ash

#endif  // ASH_SHELF_INK_DROP_BUTTON_LISTENER_H_
