// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_UI_WS_CURSOR_STATE_H_
#define SERVICES_UI_WS_CURSOR_STATE_H_

#include <memory>

#include "base/optional.h"
#include "services/ui/ws/cursor_state_delegate.h"
#include "ui/base/cursor/cursor_data.h"

namespace ui {

enum class CursorSize;

namespace ws {

class DisplayManager;

// Owns all the state about if and how the cursor is displayed in mus.
class CursorState {
 public:
  CursorState(DisplayManager* display_manager, CursorStateDelegate* delegate);
  ~CursorState();

  // Sets the normal cursor which would be used if the window manager hasn't
  // set an override cursor.
  void SetCurrentWindowCursor(const ui::CursorData& cursor);

  // When the cursor is locked, changes to the cursor are queued up. Queued
  // changes are performed atomically when the cursor is unlocked.
  void LockCursor();
  void UnlockCursor();

  // Whether the cursor is visible on the display.
  void SetCursorVisible(bool visible);

  // Sets a cursor globally, which overrides the per-window cursors.
  void SetGlobalOverrideCursor(const base::Optional<ui::CursorData>& cursor);

  // Sets the cursor size.
  void SetCursorSize(ui::CursorSize cursor_size);

  // Sets whether the cursor is hidden because the user is interacting with the
  // touch screen.
  void SetCursorTouchVisible(bool enabled);

 private:
  // A snapshot of the cursor state at a specific time.
  class StateSnapshot;

  // Notifies the window manager when the value of mouse events enabled changes.
  void NotifyCursorTouchVisibleChanged(bool enabled);

  // Synchronizes cursor set data with all platform displays.
  void SetPlatformCursorSize();

  // Synchronizes the current cursor state with all the platform displays.
  void SetPlatformCursor();

  // Contains are the displays we notify on cursor changes.
  DisplayManager* display_manager_;

  // Receives messages when mouse events enabled changes.
  CursorStateDelegate* delegate_;

  // Number of times LockCursor() has been invoked without a corresponding
  // UnlockCursor().
  int cursor_lock_count_ = 0;

  // The current state of the cursor.
  std::unique_ptr<StateSnapshot> current_state_;

  // The cursor state to restore when the cursor is unlocked.
  std::unique_ptr<StateSnapshot> state_on_unlock_;

  DISALLOW_COPY_AND_ASSIGN(CursorState);
};

}  // namespace ws
}  // namespace ui

#endif  // SERVICES_UI_WS_CURSOR_STATE_H_
