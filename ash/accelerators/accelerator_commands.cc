// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/accelerators/accelerator_commands.h"

#include "ash/shell.h"
#include "ash/wm/mru_window_tracker.h"
#include "ash/wm/screen_pinning_controller.h"
#include "ash/wm/window_state.h"
#include "ash/wm/window_util.h"
#include "ash/wm/wm_event.h"
#include "base/metrics/user_metrics.h"
#include "chromeos/chromeos_switches.h"
#include "ui/display/display.h"
#include "ui/display/display_switches.h"
#include "ui/display/manager/display_manager.h"
#include "ui/display/manager/managed_display_info.h"
#include "ui/display/screen.h"
#include "ui/gfx/geometry/point.h"

namespace ash {
namespace accelerators {

bool IsInternalDisplayZoomEnabled() {
  display::DisplayManager* display_manager = Shell::Get()->display_manager();
  return display_manager->IsDisplayUIScalingEnabled() ||
         display_manager->IsInUnifiedMode() ||
         features::IsDisplayZoomSettingEnabled();
}

bool ZoomDisplay(bool up) {
  if (up)
    base::RecordAction(base::UserMetricsAction("Accel_Scale_Ui_Up"));
  else
    base::RecordAction(base::UserMetricsAction("Accel_Scale_Ui_Down"));

  display::DisplayManager* display_manager = Shell::Get()->display_manager();

  if (display_manager->IsInUnifiedMode() ||
      !features::IsDisplayZoomSettingEnabled()) {
    return display_manager->ZoomInternalDisplay(up);
  }

  gfx::Point point = display::Screen::GetScreen()->GetCursorScreenPoint();
  display::Display display =
      display::Screen::GetScreen()->GetDisplayNearestPoint(point);
  return display_manager->ZoomDisplay(display.id(), up);
}

void ResetDisplayZoom() {
  base::RecordAction(base::UserMetricsAction("Accel_Scale_Ui_Reset"));
  display::DisplayManager* display_manager = Shell::Get()->display_manager();
  if (features::IsDisplayZoomSettingEnabled() &&
      !display_manager->IsInUnifiedMode()) {
    gfx::Point point = display::Screen::GetScreen()->GetCursorScreenPoint();
    display::Display display =
        display::Screen::GetScreen()->GetDisplayNearestPoint(point);
    display_manager->ResetDisplayZoom(display.id());
  } else {
    display_manager->ResetInternalDisplayZoom();
  }
}

bool ToggleMinimized() {
  aura::Window* window = wm::GetActiveWindow();
  // Attempt to restore the window that would be cycled through next from
  // the launcher when there is no active window.
  if (!window) {
    MruWindowTracker::WindowList mru_windows(
        Shell::Get()->mru_window_tracker()->BuildMruWindowList());
    if (!mru_windows.empty())
      wm::GetWindowState(mru_windows.front())->Activate();
    return true;
  }
  wm::WindowState* window_state = wm::GetWindowState(window);
  if (!window_state->CanMinimize())
    return false;
  window_state->Minimize();
  return true;
}

void ToggleMaximized() {
  aura::Window* active_window = wm::GetActiveWindow();
  if (!active_window)
    return;
  base::RecordAction(base::UserMetricsAction("Accel_Toggle_Maximized"));
  wm::WMEvent event(wm::WM_EVENT_TOGGLE_MAXIMIZE);
  wm::GetWindowState(active_window)->OnWMEvent(&event);
}

void ToggleFullscreen() {
  aura::Window* active_window = wm::GetActiveWindow();
  if (!active_window)
    return;
  const wm::WMEvent event(wm::WM_EVENT_TOGGLE_FULLSCREEN);
  wm::GetWindowState(active_window)->OnWMEvent(&event);
}

bool CanUnpinWindow() {
  // WindowStateType::TRUSTED_PINNED does not allow the user to press a key to
  // exit pinned mode.
  wm::WindowState* window_state = wm::GetActiveWindowState();
  return window_state &&
         window_state->GetStateType() == mojom::WindowStateType::PINNED;
}

void UnpinWindow() {
  aura::Window* pinned_window =
      Shell::Get()->screen_pinning_controller()->pinned_window();
  if (pinned_window)
    wm::GetWindowState(pinned_window)->Restore();
}

}  // namespace accelerators
}  // namespace ash
