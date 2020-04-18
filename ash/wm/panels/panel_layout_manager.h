// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_WM_PANELS_PANEL_LAYOUT_MANAGER_H_
#define ASH_WM_PANELS_PANEL_LAYOUT_MANAGER_H_

#include <list>
#include <memory>

#include "ash/ash_export.h"
#include "ash/display/window_tree_host_manager.h"
#include "ash/root_window_controller.h"
#include "ash/shelf/shelf_observer.h"
#include "ash/shell_observer.h"
#include "ash/wm/window_state_observer.h"
#include "base/compiler_specific.h"
#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "base/scoped_observer.h"
#include "ui/aura/layout_manager.h"
#include "ui/aura/window_observer.h"
#include "ui/aura/window_tracker.h"
#include "ui/keyboard/keyboard_controller.h"
#include "ui/keyboard/keyboard_controller_observer.h"
#include "ui/wm/public/activation_change_observer.h"

namespace gfx {
class Rect;
}

namespace views {
class Widget;
}

namespace ash {
class PanelCalloutWidget;
class Shelf;

namespace wm {
class RootWindowController;
}

// PanelLayoutManager is responsible for organizing panels within the
// workspace. It is associated with a specific container window (i.e.
// kShellWindowId_PanelContainer) and controls the layout of any windows
// added to that container.
//
// The constructor takes a |panel_container| argument which is expected to set
// its layout manager to this instance, e.g.:
// panel_container->SetLayoutManager(
//     std::make_unique<PanelLayoutManager>(panel_container));

class ASH_EXPORT PanelLayoutManager
    : public aura::LayoutManager,
      public wm::WindowStateObserver,
      public ::wm::ActivationChangeObserver,
      public WindowTreeHostManager::Observer,
      public ShellObserver,
      public aura::WindowObserver,
      public keyboard::KeyboardControllerObserver,
      public ShelfObserver {
 public:
  explicit PanelLayoutManager(aura::Window* panel_container);
  ~PanelLayoutManager() override;

  // Returns the PanelLayoutManager in the specified hierarchy. This searches
  // from the root of |window|.
  static PanelLayoutManager* Get(aura::Window* window);

  // Call Shutdown() before deleting children of panel_container.
  void Shutdown();

  void StartDragging(aura::Window* panel);
  void FinishDragging();

  void ToggleMinimize(aura::Window* panel);

  // Hide / Show the panel callout widgets.
  void SetShowCalloutWidgets(bool show);

  // Returns the callout widget (arrow) for |panel|.
  views::Widget* GetCalloutWidgetForPanel(aura::Window* panel);

  Shelf* shelf() { return shelf_; }
  void SetShelf(Shelf* shelf);

  // aura::LayoutManager:
  void OnWindowResized() override;
  void OnWindowAddedToLayout(aura::Window* child) override;
  void OnWillRemoveWindowFromLayout(aura::Window* child) override;
  void OnWindowRemovedFromLayout(aura::Window* child) override;
  void OnChildWindowVisibilityChanged(aura::Window* child,
                                      bool visible) override;
  void SetChildBounds(aura::Window* child,
                      const gfx::Rect& requested_bounds) override;

  // ShellObserver:
  void OnOverviewModeEnded() override;
  void OnShelfAlignmentChanged(aura::Window* root_window) override;
  void OnVirtualKeyboardStateChanged(bool activated,
                                     aura::Window* root_window) override;

  // aura::WindowObserver
  void OnWindowPropertyChanged(aura::Window* window,
                               const void* key,
                               intptr_t old) override;

  // wm::WindowStateObserver:
  void OnPostWindowStateTypeChange(wm::WindowState* window_state,
                                   mojom::WindowStateType old_type) override;

  // wm::ActivationChangeObserver:
  void OnWindowActivated(
      ::wm::ActivationChangeObserver::ActivationReason reason,
      aura::Window* gained_active,
      aura::Window* lost_active) override;

  // WindowTreeHostManager::Observer:
  void OnDisplayConfigurationChanged() override;

  // ShelfObserver:
  void WillChangeVisibilityState(ShelfVisibilityState new_state) override;
  void OnShelfIconPositionsChanged() override;

 private:
  friend class PanelLayoutManagerTest;
  friend class PanelWindowResizerTest;
  friend class WorkspaceControllerTest;
  friend class AcceleratorControllerTest;

  views::Widget* CreateCalloutWidget();

  struct ASH_EXPORT PanelInfo {
    PanelInfo() : window(NULL), callout_widget(NULL), slide_in(false) {}

    bool operator==(const aura::Window* other_window) const {
      return window == other_window;
    }

    // Returns |callout_widget| as a widget. Used by tests.
    views::Widget* CalloutWidget();

    // A weak pointer to the panel window.
    aura::Window* window;

    // The callout widget for this panel. This pointer must be managed
    // manually as this structure is used in a std::list. See
    // http://www.chromium.org/developers/smart-pointer-guidelines
    PanelCalloutWidget* callout_widget;

    // True on new and restored panel windows until the panel has been
    // positioned. The first time Relayout is called the panel will be shown,
    // and slide into position and this will be set to false.
    bool slide_in;
  };

  typedef std::list<PanelInfo> PanelList;

  void MinimizePanel(aura::Window* panel);
  void RestorePanel(aura::Window* panel);

  // Called whenever the panel layout might change.
  void Relayout();

  // Called whenever the panel stacking order needs to be updated (e.g. focus
  // changes or a panel is moved).
  void UpdateStacking(aura::Window* active_panel);

  // Update the callout arrows for all managed panels.
  void UpdateCallouts();

  // Overridden from keyboard::KeyboardControllerObserver:
  void OnKeyboardWorkspaceOccludedBoundsChanged(
      const gfx::Rect& keyboard_bounds) override;
  void OnKeyboardClosed() override;

  // Parent window associated with this layout manager.
  aura::Window* panel_container_;

  RootWindowController* root_window_controller_;

  // Protect against recursive calls to OnWindowAddedToLayout().
  bool in_add_window_;
  // Protect against recursive calls to Relayout().
  bool in_layout_;
  // Indicates if the panel callout widget should be created.
  bool show_callout_widgets_;
  // Ordered list of unowned pointers to panel windows.
  PanelList panel_windows_;
  // The panel being dragged.
  aura::Window* dragged_panel_;
  // The shelf we are observing for shelf icon changes.
  Shelf* shelf_;

  // When not NULL, the shelf is hidden (i.e. full screen) and this tracks the
  // set of panel windows which have been temporarily hidden and need to be
  // restored when the shelf becomes visible again.
  std::unique_ptr<aura::WindowTracker> restore_windows_on_shelf_visible_;

  // The last active panel. Used to maintain stacking order even if no panels
  // are currently focused.
  aura::Window* last_active_panel_;

  ScopedObserver<keyboard::KeyboardController,
                 keyboard::KeyboardControllerObserver>
      keyboard_observer_;

  base::WeakPtrFactory<PanelLayoutManager> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(PanelLayoutManager);
};

}  // namespace ash

#endif  // ASH_WM_PANELS_PANEL_LAYOUT_MANAGER_H_
