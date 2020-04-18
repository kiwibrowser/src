// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_ACCESSIBILITY_ACCESSIBILITY_PANEL_LAYOUT_MANAGER_H_
#define ASH_ACCESSIBILITY_ACCESSIBILITY_PANEL_LAYOUT_MANAGER_H_

#include "ash/ash_export.h"
#include "ash/shell_observer.h"
#include "base/macros.h"
#include "ui/aura/layout_manager.h"
#include "ui/display/display_observer.h"
#include "ui/wm/public/activation_change_observer.h"

namespace aura {
class Window;
}

namespace ash {

// AccessibilityPanelLayoutManager manages the container window used for the
// ChromeVox spoken feedback panel, which sits at the top of the display. It
// insets the display work area bounds when ChromeVox is visible. The ChromeVox
// panel is created by Chrome because spoken feedback is implemented by an
// extension. Exported for test.
class ASH_EXPORT AccessibilityPanelLayoutManager
    : public aura::LayoutManager,
      public display::DisplayObserver,
      public ::wm::ActivationChangeObserver,
      public ash::ShellObserver {
 public:
  // Height of the panel in DIPs. Public for test.
  static constexpr int kPanelHeight = 35;

  AccessibilityPanelLayoutManager();
  ~AccessibilityPanelLayoutManager() override;

  // Sets whether the panel covers the entire display.
  void SetPanelFullscreen(bool fullscreen);

  // aura::LayoutManager:
  void OnWindowResized() override {}
  void OnWindowAddedToLayout(aura::Window* child) override;
  void OnWillRemoveWindowFromLayout(aura::Window* child) override {}
  void OnWindowRemovedFromLayout(aura::Window* child) override;
  void OnChildWindowVisibilityChanged(aura::Window* child,
                                      bool visible) override;
  void SetChildBounds(aura::Window* child,
                      const gfx::Rect& requested_bounds) override;

  // DisplayObserver:
  void OnDisplayAdded(const display::Display& new_display) override {}
  void OnDisplayRemoved(const display::Display& old_display) override {}
  void OnDisplayMetricsChanged(const display::Display& display,
                               uint32_t changed_metrics) override;

  // ::wm::ActivationChangeObserver:
  void OnWindowActivated(ActivationReason reason,
                         aura::Window* gained_active,
                         aura::Window* lost_active) override;

  // ShellObserver:
  void OnFullscreenStateChanged(bool is_fullscreen,
                                aura::Window* root_window) override;

  aura::Window* panel_window_for_test() { return panel_window_; }

 private:
  // Updates the panel window bounds.
  void UpdateWindowBounds();

  // Updates the display work area to account for the panel.
  void UpdateWorkArea();

  // The panel being managed (e.g. the ChromeVoxPanel's native aura window).
  aura::Window* panel_window_ = nullptr;

  // Whether the panel itself is filling the display.
  bool panel_fullscreen_ = false;

  DISALLOW_COPY_AND_ASSIGN(AccessibilityPanelLayoutManager);
};

}  // namespace ash

#endif  // ASH_ACCESSIBILITY_ACCESSIBILITY_PANEL_LAYOUT_MANAGER_H_
