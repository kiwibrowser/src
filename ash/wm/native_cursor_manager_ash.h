// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_WM_NATIVE_CURSOR_MANAGER_ASH_H_
#define ASH_WM_NATIVE_CURSOR_MANAGER_ASH_H_

#include "ash/ash_export.h"
#include "base/macros.h"
#include "ui/display/display.h"
#include "ui/wm/core/native_cursor_manager.h"
#include "ui/wm/core/native_cursor_manager_delegate.h"

namespace ash {

// Ash specific extensions to NativeCursorManager. This lets us switch whether
// we're using the native cursor, along with exposing additional data about how
// we're going to render cursors.
class ASH_EXPORT NativeCursorManagerAsh : public ::wm::NativeCursorManager {
 public:
  ~NativeCursorManagerAsh() override = default;

  // Toggle native cursor enabled/disabled.
  // The native cursor is enabled by default. When disabled, we hide the native
  // cursor regardless of visibility state, and let CursorWindowManager draw
  // the cursor.
  virtual void SetNativeCursorEnabled(bool enabled) = 0;

  // Returns the scale and rotation of the currently loaded cursor.
  virtual float GetScale() const = 0;
  virtual display::Display::Rotation GetRotation() const = 0;

 protected:
  NativeCursorManagerAsh() = default;

 private:
  DISALLOW_COPY_AND_ASSIGN(NativeCursorManagerAsh);
};

}  // namespace ash

#endif  // ASH_WM_NATIVE_CURSOR_MANAGER_ASH_H_
