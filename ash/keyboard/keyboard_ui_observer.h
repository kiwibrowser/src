// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_KEYBOARD_KEYBOARD_UI_OBSERVER_H_
#define ASH_KEYBOARD_KEYBOARD_UI_OBSERVER_H_

#include "ash/ash_export.h"
#include "base/macros.h"

namespace ash {

class ASH_EXPORT KeyboardUIObserver {
 public:
  virtual void OnKeyboardEnabledStateChanged(bool new_enabled) = 0;

 protected:
  virtual ~KeyboardUIObserver() {}
};

}  // namespace ash

#endif  // ASH_KEYBOARD_KEYBOARD_UI_OBSERVER_H_
