// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_KEYBOARD_KEYBOARD_LAYOUT_DELEGATE_H_
#define UI_KEYBOARD_KEYBOARD_LAYOUT_DELEGATE_H_

#include <stdint.h>

#include "ui/keyboard/keyboard_export.h"

namespace display {
class Display;
}

namespace keyboard {

// A delegate class to control the virtual keyboard layout
class KEYBOARD_EXPORT KeyboardLayoutDelegate {
 public:
  virtual ~KeyboardLayoutDelegate() {}

  virtual void MoveKeyboardToDisplay(const display::Display& display) = 0;

  // Move the keyboard to the touchable display which has the input focus, or
  // the first touchable display.
  virtual void MoveKeyboardToTouchableDisplay() = 0;
};

}  // namespace keyboard

#endif
