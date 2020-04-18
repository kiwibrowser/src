// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_WM_TABLET_MODE_SCOPED_DISABLE_INTERNAL_MOUSE_AND_KEYBOARD_OZONE_H_
#define ASH_WM_TABLET_MODE_SCOPED_DISABLE_INTERNAL_MOUSE_AND_KEYBOARD_OZONE_H_

#include <memory>

#include "ash/ash_export.h"
#include "ash/wm/tablet_mode/scoped_disable_internal_mouse_and_keyboard.h"
#include "base/macros.h"

namespace ash {

// Disables the internal touchpad and keyboard for the duration of the class'
// lifetime.
class ASH_EXPORT ScopedDisableInternalMouseAndKeyboardOzone
    : public ScopedDisableInternalMouseAndKeyboard {
 public:
  ScopedDisableInternalMouseAndKeyboardOzone();
  ~ScopedDisableInternalMouseAndKeyboardOzone() override;

 private:
  class Disabler;
  std::unique_ptr<Disabler> disabler_;

  DISALLOW_COPY_AND_ASSIGN(ScopedDisableInternalMouseAndKeyboardOzone);
};

}  // namespace ash

#endif  // ASH_WM_TABLET_MODE_SCOPED_DISABLE_INTERNAL_MOUSE_AND_KEYBOARD_OZONE_H_
