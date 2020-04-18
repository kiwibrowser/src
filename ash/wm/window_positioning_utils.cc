// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/wm/window_positioning_utils.h"

#include <algorithm>

#include "ash/root_window_controller.h"
#include "ash/screen_util.h"
#include "ash/shell.h"
#include "ash/wm/system_modal_container_layout_manager.h"
#include "ash/wm/window_properties.h"
#include "ash/wm/window_state.h"
#include "ash/wm/window_util.h"
#include "ash/wm/wm_event.h"
#include "ui/aura/client/focus_client.h"
#include "ui/aura/window.h"
#include "ui/aura/window_delegate.h"
#include "ui/aura/window_tracker.h"
#include "ui/display/display.h"
#include "ui/display/screen.h"
#include "ui/display/types/display_constants.h"
#include "ui/gfx/geometry/rect.h"
#include "ui/gfx/geometry/size.h"
#include "ui/wm/core/window_util.h"

namespace ash {
namespace wm {

namespace {

// Returns the default width of a snapped window.
int GetDefaultSnappedWindowWidth(aura::Window* window) {
  const float kSnappedWidthWorkspaceRatio = 0.5f;

  int work_area_width =
      screen_util::GetDisplayWorkAreaBoundsInParent(window).width();
  int min_width =
      window->delegate() ? window->delegate()->GetMinimumSize().width() : 0;
  int ideal_width =
      static_cast<int>(work_area_width * kSnappedWidthWorkspaceRatio);
  return std::min(work_area_width, std::max(ideal_width, min_width));
}

// Return true if the window or one of its ancestor returns true from
// IsLockedToRoot().
bool IsWindowOrAncestorLockedToRoot(const aura::Window* window) {
  return window && (window->GetProperty(kLockedToRootKey) ||
                    IsWindowOrAncestorLockedToRoot(window->parent()));
}

}  // namespace

void AdjustBoundsSmallerThan(const gfx::Size& max_size, gfx::Rect* bounds) {
  bounds->set_width(std::min(bounds->width(), max_size.width()));
  bounds->set_height(std::min(bounds->height(), max_size.height()));
}

void AdjustBoundsToEnsureWindowVisibility(const gfx::Rect& visible_area,
                                          int min_width,
                                          int min_height,
                                          gfx::Rect* bounds) {
  AdjustBoundsSmallerThan(visible_area.size(), bounds);

  min_width = std::min(min_width, visible_area.width());
  min_height = std::min(min_height, visible_area.height());

  if (bounds->right() < visible_area.x() + min_width) {
    bounds->set_x(visible_area.x() + std::min(bounds->width(), min_width) -
                  bounds->width());
  } else if (bounds->x() > visible_area.right() - min_width) {
    bounds->set_x(visible_area.right() - std::min(bounds->width(), min_width));
  }
  if (bounds->bottom() < visible_area.y() + min_height) {
    bounds->set_y(visible_area.y() + std::min(bounds->height(), min_height) -
                  bounds->height());
  } else if (bounds->y() > visible_area.bottom() - min_height) {
    bounds->set_y(visible_area.bottom() -
                  std::min(bounds->height(), min_height));
  }
  if (bounds->y() < visible_area.y())
    bounds->set_y(visible_area.y());
}

void AdjustBoundsToEnsureMinimumWindowVisibility(const gfx::Rect& visible_area,
                                                 gfx::Rect* bounds) {
  AdjustBoundsToEnsureWindowVisibility(visible_area, kMinimumOnScreenArea,
                                       kMinimumOnScreenArea, bounds);
}

gfx::Rect GetDefaultLeftSnappedWindowBoundsInParent(aura::Window* window) {
  gfx::Rect work_area_in_parent(
      screen_util::GetDisplayWorkAreaBoundsInParent(window));
  return gfx::Rect(work_area_in_parent.x(), work_area_in_parent.y(),
                   GetDefaultSnappedWindowWidth(window),
                   work_area_in_parent.height());
}

gfx::Rect GetDefaultRightSnappedWindowBoundsInParent(aura::Window* window) {
  gfx::Rect work_area_in_parent(
      screen_util::GetDisplayWorkAreaBoundsInParent(window));
  int width = GetDefaultSnappedWindowWidth(window);
  return gfx::Rect(work_area_in_parent.right() - width, work_area_in_parent.y(),
                   width, work_area_in_parent.height());
}

void CenterWindow(aura::Window* window) {
  WMEvent event(WM_EVENT_CENTER);
  wm::GetWindowState(window)->OnWMEvent(&event);
}

void SetBoundsInScreen(aura::Window* window,
                       const gfx::Rect& bounds_in_screen,
                       const display::Display& display) {
  DCHECK_NE(display::kInvalidDisplayId, display.id());
  // Don't move a window to other root window if:
  // a) the window is a transient window. It moves when its
  //    transient parent moves.
  // b) if the window or its ancestor has IsLockedToRoot(). It's intentionally
  //    kept in the same root window even if the bounds is outside of the
  //    display.
  if (!::wm::GetTransientParent(window) &&
      !IsWindowOrAncestorLockedToRoot(window)) {
    RootWindowController* dst_root_window_controller =
        Shell::GetRootWindowControllerWithDisplayId(display.id());
    DCHECK(dst_root_window_controller);
    aura::Window* dst_root = dst_root_window_controller->GetRootWindow();
    DCHECK(dst_root);
    aura::Window* dst_container = nullptr;
    if (dst_root != window->GetRootWindow()) {
      int container_id = window->parent()->id();
      // All containers that use screen coordinates must have valid window ids.
      DCHECK_GE(container_id, 0);
      // Don't move modal background.
      if (!SystemModalContainerLayoutManager::IsModalBackground(window))
        dst_container = dst_root->GetChildById(container_id);
    }

    if (dst_container && window->parent() != dst_container) {
      aura::Window* focused = GetFocusedWindow();
      aura::Window* active = GetActiveWindow();

      aura::WindowTracker tracker;
      if (focused)
        tracker.Add(focused);
      if (active && focused != active)
        tracker.Add(active);

      gfx::Point origin = bounds_in_screen.origin();
      const gfx::Point display_origin = display.bounds().origin();
      origin.Offset(-display_origin.x(), -display_origin.y());
      gfx::Rect new_bounds = gfx::Rect(origin, bounds_in_screen.size());

      // Set new bounds now so that the container's layout manager can adjust
      // the bounds if necessary.
      window->SetBounds(new_bounds);

      dst_container->AddChild(window);

      // Restore focused/active window.
      if (focused && tracker.Contains(focused)) {
        aura::client::GetFocusClient(focused)->FocusWindow(focused);
        Shell::Get()->set_root_window_for_new_windows(focused->GetRootWindow());
      } else if (active && tracker.Contains(active)) {
        wm::ActivateWindow(active);
      }
      // TODO(oshima): We should not have to update the bounds again
      // below in theory, but we currently do need as there is a code
      // that assumes that the bounds will never be overridden by the
      // layout mananger. We should have more explicit control how
      // constraints are applied by the layout manager.
    }
  }
  gfx::Point origin(bounds_in_screen.origin());
  const gfx::Point display_origin = display::Screen::GetScreen()
                                        ->GetDisplayNearestWindow(window)
                                        .bounds()
                                        .origin();
  origin.Offset(-display_origin.x(), -display_origin.y());
  window->SetBounds(gfx::Rect(origin, bounds_in_screen.size()));
}

}  // namespace wm
}  // namespace ash
