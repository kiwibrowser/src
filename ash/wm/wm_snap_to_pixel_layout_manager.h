// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_WM_WM_SNAP_TO_PIXEL_LAYOUT_MANAGER_H_
#define ASH_WM_WM_SNAP_TO_PIXEL_LAYOUT_MANAGER_H_

#include "ash/ash_export.h"
#include "base/macros.h"
#include "ui/aura/layout_manager.h"

namespace ash {
namespace wm {

// A layout manager that places children's layer at the physical pixel
// boundaries.
class ASH_EXPORT WmSnapToPixelLayoutManager : public aura::LayoutManager {
 public:
  WmSnapToPixelLayoutManager();
  ~WmSnapToPixelLayoutManager() override;

  // Sets WmSnapToPixelLayoutManager as the LayoutManager on the appropriate
  // descendants of |window|.
  static void InstallOnContainers(aura::Window* window);

 protected:
  // Overridden from aura::LayoutManager:
  void OnWindowResized() override;
  void OnWindowAddedToLayout(aura::Window* child) override;
  void OnWillRemoveWindowFromLayout(aura::Window* child) override;
  void OnWindowRemovedFromLayout(aura::Window* child) override;
  void OnChildWindowVisibilityChanged(aura::Window* child,
                                      bool visible) override;
  void SetChildBounds(aura::Window* child,
                      const gfx::Rect& requested_bounds) override;

 private:
  DISALLOW_COPY_AND_ASSIGN(WmSnapToPixelLayoutManager);
};

}  // namespace wm
}  // namespace ash

#endif  // ASH_WM_WM_SNAP_TO_PIXEL_LAYOUT_MANAGER_H_
