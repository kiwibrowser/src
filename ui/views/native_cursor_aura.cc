// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/views/native_cursor.h"

#include "ui/base/cursor/cursor.h"

namespace views {

gfx::NativeCursor GetNativeIBeamCursor() {
  return ui::CursorType::kIBeam;
}

gfx::NativeCursor GetNativeHandCursor() {
  return ui::CursorType::kHand;
}

gfx::NativeCursor GetNativeColumnResizeCursor() {
  return ui::CursorType::kColumnResize;
}

gfx::NativeCursor GetNativeEastWestResizeCursor() {
  return ui::CursorType::kEastWestResize;
}

gfx::NativeCursor GetNativeNorthSouthResizeCursor() {
  return ui::CursorType::kNorthSouthResize;
}

}  // namespace views
