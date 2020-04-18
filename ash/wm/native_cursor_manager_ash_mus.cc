// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/wm/native_cursor_manager_ash_mus.h"

#include <memory>

#include "ash/display/cursor_window_controller.h"
#include "ash/display/window_tree_host_manager.h"
#include "ash/shell.h"
#include "ui/aura/env.h"
#include "ui/aura/mus/window_manager_delegate.h"
#include "ui/aura/mus/window_port_mus.h"
#include "ui/aura/window_event_dispatcher.h"
#include "ui/aura/window_tree_host.h"
#include "ui/base/cursor/image_cursors.h"
#include "ui/base/cursor/ozone/cursor_data_factory_ozone.h"
#include "ui/base/layout.h"
#include "ui/wm/core/cursor_manager.h"

namespace ash {
namespace {

void NotifyCursorVisibilityChange(bool visible) {
  // Communicate the cursor visibility state to the mus server.
  Shell::window_manager_client()->SetCursorVisible(visible);

  // Communicate the cursor visibility change to our local root window objects.
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

NativeCursorManagerAshMus::NativeCursorManagerAshMus() {
  // If we're in a mus client, we aren't going to have all of ozone initialized
  // even though we're in an ozone build. All the hard coded USE_OZONE ifdefs
  // that handle cursor code in //content/ expect that there will be a
  // CursorFactoryOzone instance. Partially initialize the ozone cursor
  // internals here, like we partially initialize other ozone subsystems in
  // ChromeBrowserMainExtraPartsViews.
  cursor_factory_ozone_ = std::make_unique<ui::CursorDataFactoryOzone>();
  image_cursors_ = std::make_unique<ui::ImageCursors>();
}

NativeCursorManagerAshMus::~NativeCursorManagerAshMus() = default;

void NativeCursorManagerAshMus::SetNativeCursorEnabled(bool enabled) {
  native_cursor_enabled_ = enabled;

  ::wm::CursorManager* cursor_manager = Shell::Get()->cursor_manager();
  SetCursor(cursor_manager->GetCursor(), cursor_manager);
}

float NativeCursorManagerAshMus::GetScale() const {
  return image_cursors_->GetScale();
}

display::Display::Rotation NativeCursorManagerAshMus::GetRotation() const {
  return image_cursors_->GetRotation();
}

void NativeCursorManagerAshMus::SetDisplay(
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

void NativeCursorManagerAshMus::SetCursor(
    gfx::NativeCursor cursor,
    ::wm::NativeCursorManagerDelegate* delegate) {
  if (image_cursors_) {
    if (native_cursor_enabled_) {
      image_cursors_->SetPlatformCursor(&cursor);
    } else {
      gfx::NativeCursor invisible_cursor(ui::CursorType::kNone);
      image_cursors_->SetPlatformCursor(&invisible_cursor);
      cursor.SetPlatformCursor(invisible_cursor.platform());
    }
  }
  cursor.set_device_scale_factor(image_cursors_->GetScale());

  delegate->CommitCursor(cursor);

  if (delegate->IsCursorVisible())
    SetCursorOnAllRootWindows(cursor);
}

void NativeCursorManagerAshMus::SetVisibility(
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

void NativeCursorManagerAshMus::SetCursorSize(
    ui::CursorSize cursor_size,
    ::wm::NativeCursorManagerDelegate* delegate) {
  delegate->CommitCursorSize(cursor_size);

  Shell::window_manager_client()->SetCursorSize(cursor_size);

  Shell::Get()
      ->window_tree_host_manager()
      ->cursor_window_controller()
      ->SetCursorSize(cursor_size);
}

void NativeCursorManagerAshMus::SetMouseEventsEnabled(
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

void NativeCursorManagerAshMus::SetCursorOnAllRootWindows(
    gfx::NativeCursor cursor) {
  ui::CursorData mojo_cursor;

  // Only send a real mojo cursor to the window server when native cursors are
  // enabled. Otherwise send a kNone cursor as the global override cursor. If
  // you need to debug window manager side cursor window positioning, setting
  // |native_cursor_enabled| to always be true will display both.
  if (native_cursor_enabled_) {
    if (cursor.platform()) {
      mojo_cursor =
          ui::CursorDataFactoryOzone::GetCursorData(cursor.platform());
    } else {
      mojo_cursor = ui::CursorData(cursor.native_type());
    }
  } else {
    mojo_cursor = ui::CursorData(ui::CursorType::kNone);
  }

  if (!mojo_cursor.IsSameAs(last_cursor_sent_to_window_server_)) {
    // As the window manager, tell mus to use |mojo_cursor| everywhere. We do
    // this instead of trying to set per-window because otherwise we run into
    // the event targeting issue.
    last_cursor_sent_to_window_server_ = mojo_cursor;
    Shell::window_manager_client()->SetGlobalOverrideCursor(mojo_cursor);
  }

  // Make sure the local state is set properly, so that local queries show that
  // we set the cursor.
  for (aura::Window* root : Shell::Get()->GetAllRootWindows())
    root->GetHost()->SetCursor(cursor);

  Shell::Get()
      ->window_tree_host_manager()
      ->cursor_window_controller()
      ->SetCursor(cursor);
}

}  // namespace ash
