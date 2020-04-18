// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_KEYBOARD_KEYBOARD_CONTROLLER_OBSERVER_H_
#define UI_KEYBOARD_KEYBOARD_CONTROLLER_OBSERVER_H_

#include "ui/keyboard/keyboard_controller.h"
#include "ui/keyboard/keyboard_export.h"

namespace gfx {
class Rect;
}

namespace keyboard {

// Describes the various attributes of the keyboard's appearance and usability.
struct KeyboardStateDescriptor {
  bool is_available;
  bool is_locked;
  gfx::Rect visual_bounds;
  gfx::Rect occluded_bounds;
  gfx::Rect displaced_bounds;
};

// Observers to the KeyboardController are notified of significant events that
// occur with the keyboard, such as the bounds or visiility changing.
class KEYBOARD_EXPORT KeyboardControllerObserver {
 public:
  virtual ~KeyboardControllerObserver() {}

  // Called when the keyboard is shown or closed.
  virtual void OnKeyboardAvailabilityChanged(bool is_available) {}

  // Called when the keyboard bounds are changing.
  virtual void OnKeyboardVisibleBoundsChanged(const gfx::Rect& new_bounds) {}

  // Called when the keyboard bounds have changed in a way that should affect
  // the usable region of the workspace.
  virtual void OnKeyboardWorkspaceOccludedBoundsChanged(
      const gfx::Rect& new_bounds) {}

  // Called when the keyboard bounds have changed in a way that affects how the
  // workspace should change to not take up the screen space occupied by the
  // keyboard.
  virtual void OnKeyboardWorkspaceDisplacingBoundsChanged(
      const gfx::Rect& new_bounds){};

  // Redundant with other various notification methods. Use this if the state of
  // multiple properties need to be conveyed simultaneously to observer
  // implementations without the need to track multiple stateful properties.
  virtual void OnKeyboardAppearanceChanged(
      const KeyboardStateDescriptor& state){};

  // Called when the keyboard was closed.
  virtual void OnKeyboardClosed(){};

  // Called when the keyboard has been hidden and the hiding animation finished
  // successfully. This is same as |state| == HIDDEN on OnStateChanged.
  virtual void OnKeyboardHidden() {}

  // Called when the state changed.
  virtual void OnStateChanged(KeyboardControllerState state) {}

  // Called when the virtual keyboard IME config changed.
  virtual void OnKeyboardConfigChanged() {}
};

}  // namespace keyboard

#endif  // UI_KEYBOARD_KEYBOARD_CONTROLLER_OBSERVER_H_
