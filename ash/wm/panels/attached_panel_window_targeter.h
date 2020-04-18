// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_WM_PANELS_ATTACHED_PANEL_WINDOW_TARGETER_H_
#define ASH_WM_PANELS_ATTACHED_PANEL_WINDOW_TARGETER_H_

#include "ash/shell_observer.h"
#include "base/macros.h"
#include "ui/wm/core/easy_resize_window_targeter.h"

namespace gfx {
class Insets;
}

namespace ash {

class PanelLayoutManager;

// A window targeter installed on a panel container to disallow touch
// hit-testing of attached panel edges that are adjacent to the shelf. This
// makes it significantly easier to correctly target shelf buttons with touch.
class AttachedPanelWindowTargeter : public ::wm::EasyResizeWindowTargeter,
                                    public ShellObserver {
 public:
  AttachedPanelWindowTargeter(aura::Window* container,
                              const gfx::Insets& default_mouse_extend,
                              const gfx::Insets& default_touch_extend,
                              PanelLayoutManager* panel_layout_manager);
  ~AttachedPanelWindowTargeter() override;

  // ShellObserver:
  void OnShelfCreatedForRootWindow(aura::Window* root_window) override;
  void OnShelfAlignmentChanged(aura::Window* root_window) override;

 private:
  void UpdateTouchExtend(aura::Window* root_window);

  gfx::Insets GetTouchExtendForShelfAlignment() const;

  aura::Window* panel_container_;
  PanelLayoutManager* panel_layout_manager_;
  const gfx::Insets default_touch_extend_;

  DISALLOW_COPY_AND_ASSIGN(AttachedPanelWindowTargeter);
};

}  // namespace ash

#endif  // ASH_WM_PANELS_ATTACHED_PANEL_WINDOW_TARGETER_H_
