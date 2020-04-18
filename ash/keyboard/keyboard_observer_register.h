// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_KEYBOARD_KEYBOARD_OBSERVER_REGISTER_H_
#define ASH_KEYBOARD_KEYBOARD_OBSERVER_REGISTER_H_

#include "base/scoped_observer.h"

namespace aura {
class Window;
}

namespace keyboard {
class KeyboardController;
class KeyboardControllerObserver;
}

namespace ash {

// Helper function to start/stop observing KeyboardController.
// |keyboard_root_window| is the root window where KeyboardController changes
// states.  |observer_root_window| is the root window relevant to the
// |keyboard_observer|. If |keyboard_root_window| is different from
// |observer_root_window|, this function is a no-op. Otherwise, the observing
// starts if |keyboard_activated| is true and stops if |keyboard_activated| is
// false.
void UpdateKeyboardObserverFromStateChanged(
    bool keyboard_activated,
    aura::Window* keyboard_root_window,
    aura::Window* observer_root_window,
    ScopedObserver<keyboard::KeyboardController,
                   keyboard::KeyboardControllerObserver>* keyboard_observer);

}  // namespace ash

#endif  // ASH_KEYBOARD_KEYBOARD_OBSERVER_REGISTER_H_
