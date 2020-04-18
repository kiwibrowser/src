// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_WM_NATIVE_CURSOR_MANAGER_ASH_CLASSIC_H_
#define ASH_WM_NATIVE_CURSOR_MANAGER_ASH_CLASSIC_H_

#include "ash/wm/native_cursor_manager_ash.h"

namespace ui {
class ImageCursors;
}

namespace ash {

// This does the ash-specific setting of cursor details like cursor
// visibility. It communicates back with the CursorManager through the
// NativeCursorManagerDelegate interface, which receives messages about what
// changes were acted on.
class ASH_EXPORT NativeCursorManagerAshClassic : public NativeCursorManagerAsh {
 public:
  NativeCursorManagerAshClassic();
  ~NativeCursorManagerAshClassic() override;

 private:
  friend class CursorManagerTestApi;

  // Overridden from NativeCursorManagerAsh:
  void SetNativeCursorEnabled(bool enabled) override;
  float GetScale() const override;
  display::Display::Rotation GetRotation() const override;

  // Overridden from ::wm::NativeCursorManager:
  void SetDisplay(const display::Display& display,
                  ::wm::NativeCursorManagerDelegate* delegate) override;
  void SetCursor(gfx::NativeCursor cursor,
                 ::wm::NativeCursorManagerDelegate* delegate) override;
  void SetVisibility(bool visible,
                     ::wm::NativeCursorManagerDelegate* delegate) override;
  void SetCursorSize(ui::CursorSize cursor_size,
                     ::wm::NativeCursorManagerDelegate* delegate) override;
  void SetMouseEventsEnabled(
      bool enabled,
      ::wm::NativeCursorManagerDelegate* delegate) override;

  // The cursor location where the cursor was disabled.
  gfx::Point disabled_cursor_location_;

  bool native_cursor_enabled_;

  std::unique_ptr<ui::ImageCursors> image_cursors_;

  DISALLOW_COPY_AND_ASSIGN(NativeCursorManagerAshClassic);
};

}  // namespace ash

#endif  // ASH_WM_NATIVE_CURSOR_MANAGER_ASH_CLASSIC_H_
