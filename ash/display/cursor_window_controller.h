// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_DISPLAY_CURSOR_WINDOW_CONTROLLER_H_
#define ASH_DISPLAY_CURSOR_WINDOW_CONTROLLER_H_

#include <memory>

#include "ash/ash_export.h"
#include "ash/public/cpp/ash_constants.h"
#include "base/macros.h"
#include "ui/aura/window.h"
#include "ui/base/cursor/cursor.h"
#include "ui/display/display.h"

namespace cursor {
class CursorView;
}

namespace ash {

class CursorWindowControllerTest;
class CursorWindowDelegate;

// Draws a mouse cursor on a given container window.
// When cursor compositing is disabled, draw nothing as the native cursor is
// shown.
// When cursor compositing is enabled, just draw the cursor as-is.
class ASH_EXPORT CursorWindowController {
 public:
  CursorWindowController();
  ~CursorWindowController();

  bool is_cursor_compositing_enabled() const {
    return is_cursor_compositing_enabled_;
  }

  void SetLargeCursorSizeInDip(int large_cursor_size_in_dip);

  // If at least one of the features that use cursor compositing is enabled, it
  // should not be disabled. Future features that require cursor compositing
  // should be added in this function.
  bool ShouldEnableCursorCompositing();

  // Sets cursor compositing mode on/off.
  void SetCursorCompositingEnabled(bool enabled);

  // Updates the container window for the cursor window controller.
  void UpdateContainer();

  // Sets the display on which to draw cursor.
  // Only applicable when cursor compositing is enabled.
  void SetDisplay(const display::Display& display);

  // Sets cursor location, shape, set and visibility.
  void UpdateLocation();
  void SetCursor(gfx::NativeCursor cursor);
  void SetCursorSize(ui::CursorSize cursor_size);
  void SetVisibility(bool visible);

 private:
  friend class CursorWindowControllerTest;
  friend class MirrorWindowTestApi;

  // Sets the container window for the cursor window controller.
  // Closes the cursor window if |container| is NULL.
  void SetContainer(aura::Window* container);

  // Sets the bounds of the container in screen coordinates and rotation.
  void SetBoundsInScreenAndRotation(const gfx::Rect& bounds,
                                    display::Display::Rotation rotation);

  // Updates cursor image based on current cursor state.
  void UpdateCursorImage();

  // Hides/shows cursor window based on current cursor state.
  void UpdateCursorVisibility();

  // Updates cursor view based on current cursor state.
  void UpdateCursorView();

  const gfx::ImageSkia& GetCursorImageForTest() const;

  aura::Window* container_ = nullptr;

  // The current cursor-compositing state.
  bool is_cursor_compositing_enabled_ = false;

  // The bounds of the container in screen coordinates.
  gfx::Rect bounds_in_screen_;

  // The rotation of the container.
  display::Display::Rotation rotation_ = display::Display::ROTATE_0;

  // The native cursor, see definitions in cursor.h
  gfx::NativeCursor cursor_ = ui::CursorType::kNone;

  // The last requested cursor visibility.
  bool visible_ = true;

  ui::CursorSize cursor_size_ = ui::CursorSize::kNormal;
  gfx::Point hot_point_;

  int large_cursor_size_in_dip_ = ash::kDefaultLargeCursorSize;

  // The display on which the cursor is drawn.
  // For mirroring mode, the display is always the primary display.
  display::Display display_;

  std::unique_ptr<aura::Window> cursor_window_;
  std::unique_ptr<CursorWindowDelegate> delegate_;
  std::unique_ptr<cursor::CursorView> cursor_view_;

  const bool is_cursor_motion_blur_enabled_;

  DISALLOW_COPY_AND_ASSIGN(CursorWindowController);
};

}  // namespace ash

#endif  // ASH_DISPLAY_CURSOR_WINDOW_CONTROLLER_H_
