// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_WM_WORKSPACE_WORKSPACE_BACKDROP_DELEGATE_IMPL_H_
#define ASH_WM_WORKSPACE_WORKSPACE_BACKDROP_DELEGATE_IMPL_H_

#include <memory>

#include "ash/accessibility/accessibility_observer.h"
#include "ash/shell_observer.h"
#include "ash/wallpaper/wallpaper_controller_observer.h"
#include "ash/wm/splitview/split_view_controller.h"
#include "base/macros.h"

namespace aura {
class Window;
}

namespace views {
class Widget;
}

namespace ui {
class EventHandler;
}

namespace ash {
namespace mojom {
enum class WindowStateType;
}

namespace wm {
class WindowState;
}

class BackdropDelegate;

// A backdrop which gets created for a container |window| and which gets
// stacked behind the top level, activatable window that meets the following
// criteria.
//
// 1) Has a aura::client::kHasBackdrop property = true.
// 2) BackdropDelegate::HasBackdrop(aura::Window* window) returns true.
// 3) Active ARC window when the spoken feedback is enabled.
class BackdropController : public ShellObserver,
                           public AccessibilityObserver,
                           public SplitViewController::Observer,
                           public WallpaperControllerObserver {
 public:
  explicit BackdropController(aura::Window* container);
  ~BackdropController() override;

  void OnWindowAddedToLayout(aura::Window* child);
  void OnWindowRemovedFromLayout(aura::Window* child);
  void OnChildWindowVisibilityChanged(aura::Window* child, bool visible);
  void OnWindowStackingChanged(aura::Window* window);
  void OnPostWindowStateTypeChange(wm::WindowState* window_state,
                                   mojom::WindowStateType old_type);

  void SetBackdropDelegate(std::unique_ptr<BackdropDelegate> delegate);

  // Update the visibility of, and restack the backdrop relative to
  // the other windows in the container.
  void UpdateBackdrop();

  // ShellObserver:
  void OnOverviewModeStarting() override;
  void OnOverviewModeEnded() override;
  void OnAppListVisibilityChanged(bool shown,
                                  aura::Window* root_window) override;
  void OnSplitViewModeStarting() override;
  void OnSplitViewModeEnded() override;

  // AccessibilityObserver:
  void OnAccessibilityStatusChanged() override;

  // SplitViewController::Observer:
  void OnSplitViewStateChanged(SplitViewController::State previous_state,
                               SplitViewController::State state) override;
  void OnSplitViewDividerPositionChanged() override;

  // WallpaperControllerObserver:
  void OnWallpaperPreviewStarted() override;

 private:
  friend class WorkspaceControllerTestApi;

  void EnsureBackdropWidget();

  void UpdateAccessibilityMode();

  // Returns the current visible top level window in the container.
  aura::Window* GetTopmostWindowWithBackdrop();

  bool WindowShouldHaveBackdrop(aura::Window* window);

  // Show the backdrop window.
  void Show();

  // Hide the backdrop window.
  void Hide();

  // Returns true if the backdrop window should be fullscreen. It should not be
  // fullscreen only if 1) split view is active and 2) there is only one snapped
  // window and 3) the snapped window is the topmost window which should have
  // the backdrop.
  bool BackdropShouldFullscreen();

  // Gets the bounds for the backdrop window if it should not be fullscreen.
  // It's the case for splitview mode, if there is only one snapped window, the
  // backdrop should not cover the non-snapped side of the screen, thus the
  // backdrop bounds should be the bounds of the snapped window.
  gfx::Rect GetBackdropBounds();

  // The backdrop which covers the rest of the screen.
  views::Widget* backdrop_ = nullptr;

  // aura::Window for |backdrop_|.
  aura::Window* backdrop_window_ = nullptr;

  // The container of the window that should have a backdrop.
  aura::Window* container_;

  std::unique_ptr<BackdropDelegate> delegate_;

  // Event hanlder used to implement actions for accessibility.
  std::unique_ptr<ui::EventHandler> backdrop_event_handler_;
  ui::EventHandler* original_event_handler_ = nullptr;

  // If true, the |RestackOrHideWindow| might recurse.
  bool in_restacking_ = false;

  DISALLOW_COPY_AND_ASSIGN(BackdropController);
};

}  // namespace ash

#endif  // ASH_WM_WORKSPACE_WORKSPACE_BACKDROP_DELEGATE_IMPL_H_
