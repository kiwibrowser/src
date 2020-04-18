// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/keyboard/keyboard_observer_register.h"

#include "ui/keyboard/keyboard_controller.h"

namespace ash {

void UpdateKeyboardObserverFromStateChanged(
    bool keyboard_activated,
    aura::Window* keyboard_root_window,
    aura::Window* observer_root_window,
    ScopedObserver<keyboard::KeyboardController,
                   keyboard::KeyboardControllerObserver>* keyboard_observer) {
  if (keyboard_root_window != observer_root_window)
    return;

  keyboard::KeyboardController* const keyboard_controller =
      keyboard::KeyboardController::GetInstance();
  if (keyboard_activated &&
      !keyboard_observer->IsObserving(keyboard_controller)) {
    keyboard_observer->Add(keyboard_controller);
  } else if (!keyboard_activated &&
             keyboard_observer->IsObserving(keyboard_controller)) {
    keyboard_observer->Remove(keyboard_controller);
  }
}

}  // namespace ash
