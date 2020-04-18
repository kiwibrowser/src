// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/wm/window_util.h"

#include <memory>
#include <vector>

#include "ash/public/cpp/ash_constants.h"
#include "ash/public/cpp/config.h"
#include "ash/public/cpp/shell_window_ids.h"
#include "ash/public/cpp/window_properties.h"
#include "ash/root_window_controller.h"
#include "ash/session/session_controller.h"
#include "ash/shelf/shelf.h"
#include "ash/shell.h"
#include "ash/wm/resize_handle_window_targeter.h"
#include "ash/wm/widget_finder.h"
#include "ash/wm/window_positioning_utils.h"
#include "ash/wm/window_properties.h"
#include "ash/wm/window_state.h"
#include "ash/wm/wm_event.h"
#include "ui/aura/client/aura_constants.h"
#include "ui/aura/client/capture_client.h"
#include "ui/aura/client/focus_client.h"
#include "ui/aura/mus/window_manager_delegate.h"
#include "ui/aura/mus/window_port_mus.h"
#include "ui/aura/mus/window_tree_client.h"
#include "ui/aura/window.h"
#include "ui/aura/window_delegate.h"
#include "ui/aura/window_event_dispatcher.h"
#include "ui/base/hit_test.h"
#include "ui/compositor/dip_util.h"
#include "ui/display/display.h"
#include "ui/display/screen.h"
#include "ui/gfx/geometry/rect.h"
#include "ui/gfx/geometry/size.h"
#include "ui/views/view.h"
#include "ui/views/widget/widget.h"
#include "ui/wm/core/coordinate_conversion.h"
#include "ui/wm/core/easy_resize_window_targeter.h"
#include "ui/wm/core/window_util.h"
#include "ui/wm/public/activation_client.h"

namespace ash {
namespace wm {

namespace {

// Moves |window| to the given |root| window's corresponding container, if it is
// not already in the same root window. Returns true if |window| was moved.
bool MoveWindowToRoot(aura::Window* window, aura::Window* root) {
  if (!root || root == window->GetRootWindow())
    return false;
  aura::Window* container = RootWindowController::ForWindow(root)->GetContainer(
      window->parent()->id());
  if (!container)
    return false;
  container->AddChild(window);
  return true;
}

}  // namespace

// TODO(beng): replace many of these functions with the corewm versions.
void ActivateWindow(aura::Window* window) {
  ::wm::ActivateWindow(window);
}

void DeactivateWindow(aura::Window* window) {
  ::wm::DeactivateWindow(window);
}

bool IsActiveWindow(aura::Window* window) {
  return ::wm::IsActiveWindow(window);
}

aura::Window* GetActiveWindow() {
  return ::wm::GetActivationClient(Shell::GetPrimaryRootWindow())
      ->GetActiveWindow();
}

aura::Window* GetActivatableWindow(aura::Window* window) {
  return ::wm::GetActivatableWindow(window);
}

bool CanActivateWindow(aura::Window* window) {
  return ::wm::CanActivateWindow(window);
}

aura::Window* GetFocusedWindow() {
  return aura::client::GetFocusClient(Shell::GetPrimaryRootWindow())
      ->GetFocusedWindow();
}

aura::Window* GetCaptureWindow() {
  return aura::client::GetCaptureWindow(Shell::GetPrimaryRootWindow());
}

void GetBlockingContainersForRoot(aura::Window* root_window,
                                  aura::Window** min_container,
                                  aura::Window** system_modal_container) {
  if (Shell::Get()->session_controller()->IsUserSessionBlocked()) {
    *min_container =
        root_window->GetChildById(kShellWindowId_LockScreenContainersContainer);
    *system_modal_container =
        root_window->GetChildById(kShellWindowId_LockSystemModalContainer);
  } else {
    *min_container = nullptr;
    *system_modal_container =
        root_window->GetChildById(kShellWindowId_SystemModalContainer);
  }
}

bool IsWindowUserPositionable(aura::Window* window) {
  return GetWindowState(window)->IsUserPositionable();
}

void PinWindow(aura::Window* window, bool trusted) {
  wm::WMEvent event(trusted ? wm::WM_EVENT_TRUSTED_PIN : wm::WM_EVENT_PIN);
  wm::GetWindowState(window)->OnWMEvent(&event);
}

void SetAutoHideShelf(aura::Window* window, bool autohide) {
  wm::GetWindowState(window)->set_autohide_shelf_when_maximized_or_fullscreen(
      autohide);
  for (aura::Window* root_window : Shell::GetAllRootWindows())
    Shelf::ForWindow(root_window)->UpdateVisibilityState();
}

bool MoveWindowToDisplay(aura::Window* window, int64_t display_id) {
  DCHECK(window);
  WindowState* window_state = GetWindowState(window);
  if (window_state->allow_set_bounds_direct()) {
    aura::Window* root = Shell::GetRootWindowForDisplayId(display_id);
    if (root) {
      gfx::Rect bounds = window->bounds();
      MoveWindowToRoot(window, root);
      // Client controlled won't update the bounds upon the root window
      // Change. Explicitly update the bounds so that the client can
      // make decision.
      window->SetBounds(bounds);
      return true;
    }
    return false;
  }
  aura::Window* root = Shell::GetRootWindowForDisplayId(display_id);
  // Update restore bounds to target root window.
  if (window_state->HasRestoreBounds()) {
    gfx::Rect restore_bounds = window_state->GetRestoreBoundsInParent();
    ::wm::ConvertRectToScreen(root, &restore_bounds);
    window_state->SetRestoreBoundsInScreen(restore_bounds);
  }
  return root && MoveWindowToRoot(window, root);
}

bool MoveWindowToEventRoot(aura::Window* window, const ui::Event& event) {
  DCHECK(window);
  views::View* target = static_cast<views::View*>(event.target());
  if (!target)
    return false;
  aura::Window* root = target->GetWidget()->GetNativeView()->GetRootWindow();
  return root && MoveWindowToRoot(window, root);
}

void SnapWindowToPixelBoundary(aura::Window* window) {
  window->SetProperty(kSnapChildrenToPixelBoundary, true);
  aura::Window* snapped_ancestor = window->parent();
  while (snapped_ancestor) {
    if (snapped_ancestor->GetProperty(kSnapChildrenToPixelBoundary)) {
      ui::SnapLayerToPhysicalPixelBoundary(snapped_ancestor->layer(),
                                           window->layer());
      return;
    }
    snapped_ancestor = snapped_ancestor->parent();
  }
}

void SetSnapsChildrenToPhysicalPixelBoundary(aura::Window* container) {
  DCHECK(!container->GetProperty(kSnapChildrenToPixelBoundary))
      << container->GetName();
  container->SetProperty(kSnapChildrenToPixelBoundary, true);
}

int GetNonClientComponent(aura::Window* window, const gfx::Point& location) {
  return window->delegate()
             ? window->delegate()->GetNonClientComponent(location)
             : HTNOWHERE;
}

void SetChildrenUseExtendedHitRegionForWindow(aura::Window* window) {
  gfx::Insets mouse_extend(-kResizeOutsideBoundsSize, -kResizeOutsideBoundsSize,
                           -kResizeOutsideBoundsSize,
                           -kResizeOutsideBoundsSize);
  gfx::Insets touch_extend =
      mouse_extend.Scale(kResizeOutsideBoundsScaleForTouch);
  // TODO: EasyResizeWindowTargeter makes it so children get events outside
  // their bounds. This only works in mash when mash is providing the non-client
  // frame. Mus needs to support an api for the WindowManager that enables
  // events to be dispatched to windows outside the windows bounds that this
  // function calls into. http://crbug.com/679056.
  window->SetEventTargeter(std::make_unique<::wm::EasyResizeWindowTargeter>(
      window, mouse_extend, touch_extend));
}

void CloseWidgetForWindow(aura::Window* window) {
  if (Shell::GetAshConfig() == Config::MASH &&
      window->GetProperty(kWidgetCreationTypeKey) ==
          WidgetCreationType::FOR_CLIENT) {
    // NOTE: in the FOR_CLIENT case there is not necessarily a widget associated
    // with the window. Mash only creates widgets for top level windows if mash
    // renders the non-client frame.
    DCHECK(Shell::window_manager_client());
    Shell::window_manager_client()->RequestClose(window);
    return;
  }
  views::Widget* widget = GetInternalWidgetForWindow(window);
  DCHECK(widget);
  widget->Close();
}

void AddLimitedPreTargetHandlerForWindow(ui::EventHandler* handler,
                                         aura::Window* window) {
  // In mus AddPreTargetHandler() only works for windows created by this client.
  DCHECK(Shell::GetAshConfig() != Config::MASH ||
         Shell::window_tree_client()->WasCreatedByThisClient(
             aura::WindowMus::Get(window)));
  window->AddPreTargetHandler(handler);
}

void RemoveLimitedPreTargetHandlerForWindow(ui::EventHandler* handler,
                                            aura::Window* window) {
  window->RemovePreTargetHandler(handler);
}

void InstallResizeHandleWindowTargeterForWindow(
    aura::Window* window,
    ImmersiveFullscreenController* immersive_fullscreen_controller) {
  window->SetEventTargeter(std::make_unique<ResizeHandleWindowTargeter>(
      window, immersive_fullscreen_controller));
}

bool IsDraggingTabs(const aura::Window* window) {
  return window->GetProperty(ash::kIsDraggingTabsKey);
}

}  // namespace wm
}  // namespace ash
