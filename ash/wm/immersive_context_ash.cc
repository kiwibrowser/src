// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/wm/immersive_context_ash.h"

#include "ash/public/cpp/immersive/immersive_fullscreen_controller.h"
#include "ash/shelf/shelf.h"
#include "ash/shell.h"
#include "ash/shell_port.h"
#include "ash/wm/resize_handle_window_targeter.h"
#include "ash/wm/window_state.h"
#include "ash/wm/window_util.h"
#include "base/logging.h"
#include "ui/display/display.h"
#include "ui/display/screen.h"
#include "ui/views/widget/widget.h"

namespace ash {

ImmersiveContextAsh::ImmersiveContextAsh() = default;

ImmersiveContextAsh::~ImmersiveContextAsh() = default;

void ImmersiveContextAsh::InstallResizeHandleWindowTargeter(
    ImmersiveFullscreenController* controller) {
  wm::InstallResizeHandleWindowTargeterForWindow(
      controller->widget()->GetNativeWindow(), controller);
}

void ImmersiveContextAsh::OnEnteringOrExitingImmersive(
    ImmersiveFullscreenController* controller,
    bool entering) {
  aura::Window* window = controller->widget()->GetNativeWindow();
  wm::WindowState* window_state = wm::GetWindowState(window);
  // Auto hide the shelf in immersive fullscreen instead of hiding it.
  window_state->SetHideShelfWhenFullscreen(!entering);
  // Update the window's immersive mode state for the window manager.
  window_state->SetInImmersiveFullscreen(entering);

  for (aura::Window* root_window : Shell::GetAllRootWindows())
    Shelf::ForWindow(root_window)->UpdateVisibilityState();
}

gfx::Rect ImmersiveContextAsh::GetDisplayBoundsInScreen(views::Widget* widget) {
  display::Display display =
      display::Screen::GetScreen()->GetDisplayNearestWindow(
          widget->GetNativeWindow());
  return display.bounds();
}

void ImmersiveContextAsh::AddPointerWatcher(
    views::PointerWatcher* watcher,
    views::PointerWatcherEventTypes events) {
  ShellPort::Get()->AddPointerWatcher(watcher, events);
}

void ImmersiveContextAsh::RemovePointerWatcher(views::PointerWatcher* watcher) {
  ShellPort::Get()->RemovePointerWatcher(watcher);
}

bool ImmersiveContextAsh::DoesAnyWindowHaveCapture() {
  return wm::GetCaptureWindow() != nullptr;
}

bool ImmersiveContextAsh::IsMouseEventsEnabled() {
  return ShellPort::Get()->IsMouseEventsEnabled();
}

}  // namespace ash
