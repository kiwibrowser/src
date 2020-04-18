// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/wm/native_cursor_manager_ash_classic.h"

#include "ash/display/cursor_window_controller.h"
#include "ash/display/window_tree_host_manager.h"
#include "ash/shell.h"
#include "base/logging.h"
#include "ui/aura/env.h"
#include "ui/aura/window_event_dispatcher.h"
#include "ui/aura/window_tree_host.h"
#include "ui/base/cursor/cursor.h"
#include "ui/base/cursor/image_cursors.h"
#include "ui/base/layout.h"

namespace ash {
namespace {

void SetCursorOnAllRootWindows(gfx::NativeCursor cursor) {
  aura::Window::Windows root_windows = Shell::Get()->GetAllRootWindows();
  for (aura::Window::Windows::iterator iter = root_windows.begin();
       iter != root_windows.end(); ++iter)
    (*iter)->GetHost()->SetCursor(cursor);

  Shell::Get()
      ->window_tree_host_manager()
      ->cursor_window_controller()
      ->SetCursor(cursor);
}

void NotifyCursorVisibilityChange(bool visible) {
  aura::Window::Windows root_windows = Shell::Get()->GetAllRootWindows();
  for (aura::Window::Windows::iterator iter = root_windows.begin();
       iter != root_windows.end(); ++iter)
    (*iter)->GetHost()->OnCursorVisibilityChanged(visible);

  Shell::Get()
      ->window_tree_host_manager()
      ->cursor_window_controller()
      ->SetVisibility(visible);
}

void NotifyMouseEventsEnableStateChange(bool enabled) {
  aura::Window::Windows root_windows = Shell::Get()->GetAllRootWindows();
  for (aura::Window::Windows::iterator iter = root_windows.begin();
       iter != root_windows.end(); ++iter)
    (*iter)->GetHost()->dispatcher()->OnMouseEventsEnableStateChanged(enabled);
  // Mirror window never process events.
}

}  // namespace

NativeCursorManagerAshClassic::NativeCursorManagerAshClassic()
    : native_cursor_enabled_(true), image_cursors_(new ui::ImageCursors) {}

NativeCursorManagerAshClassic::~NativeCursorManagerAshClassic() = default;

void NativeCursorManagerAshClassic::SetNativeCursorEnabled(bool enabled) {
  native_cursor_enabled_ = enabled;

  ::wm::CursorManager* cursor_manager = Shell::Get()->cursor_manager();
  SetCursor(cursor_manager->GetCursor(), cursor_manager);
}

float NativeCursorManagerAshClassic::GetScale() const {
  return image_cursors_->GetScale();
}

display::Display::Rotation NativeCursorManagerAshClassic::GetRotation() const {
  return image_cursors_->GetRotation();
}

void NativeCursorManagerAshClassic::SetDisplay(
    const display::Display& display,
    ::wm::NativeCursorManagerDelegate* delegate) {
  DCHECK(display.is_valid());
  // Use the platform's device scale factor instead of the display's, which
  // might have been adjusted for the UI scale.
  const float original_scale = Shell::Get()
                                   ->display_manager()
                                   ->GetDisplayInfo(display.id())
                                   .device_scale_factor();
  // And use the nearest resource scale factor.
  const float cursor_scale =
      ui::GetScaleForScaleFactor(ui::GetSupportedScaleFactor(original_scale));

  if (image_cursors_->SetDisplay(display, cursor_scale))
    SetCursor(delegate->GetCursor(), delegate);

  Shell::Get()
      ->window_tree_host_manager()
      ->cursor_window_controller()
      ->SetDisplay(display);
}

void NativeCursorManagerAshClassic::SetCursor(
    gfx::NativeCursor cursor,
    ::wm::NativeCursorManagerDelegate* delegate) {
  if (native_cursor_enabled_) {
    image_cursors_->SetPlatformCursor(&cursor);
  } else {
    gfx::NativeCursor invisible_cursor(ui::CursorType::kNone);
    image_cursors_->SetPlatformCursor(&invisible_cursor);
    cursor.SetPlatformCursor(invisible_cursor.platform());
  }
  cursor.set_device_scale_factor(image_cursors_->GetScale());

  delegate->CommitCursor(cursor);

  if (delegate->IsCursorVisible())
    SetCursorOnAllRootWindows(cursor);
}

void NativeCursorManagerAshClassic::SetCursorSize(
    ui::CursorSize cursor_size,
    ::wm::NativeCursorManagerDelegate* delegate) {
  image_cursors_->SetCursorSize(cursor_size);
  delegate->CommitCursorSize(cursor_size);

  // Sets the cursor to reflect the scale change immediately.
  if (delegate->IsCursorVisible())
    SetCursor(delegate->GetCursor(), delegate);

  Shell::Get()
      ->window_tree_host_manager()
      ->cursor_window_controller()
      ->SetCursorSize(cursor_size);
}

void NativeCursorManagerAshClassic::SetVisibility(
    bool visible,
    ::wm::NativeCursorManagerDelegate* delegate) {
  delegate->CommitVisibility(visible);

  if (visible) {
    SetCursor(delegate->GetCursor(), delegate);
  } else {
    gfx::NativeCursor invisible_cursor(ui::CursorType::kNone);
    image_cursors_->SetPlatformCursor(&invisible_cursor);
    SetCursorOnAllRootWindows(invisible_cursor);
  }

  NotifyCursorVisibilityChange(visible);
}

void NativeCursorManagerAshClassic::SetMouseEventsEnabled(
    bool enabled,
    ::wm::NativeCursorManagerDelegate* delegate) {
  delegate->CommitMouseEventsEnabled(enabled);

  if (enabled) {
    aura::Env::GetInstance()->SetLastMouseLocation(disabled_cursor_location_);
  } else {
    disabled_cursor_location_ = aura::Env::GetInstance()->last_mouse_location();
  }

  SetVisibility(delegate->IsCursorVisible(), delegate);
  NotifyMouseEventsEnableStateChange(enabled);
}

}  // namespace ash
