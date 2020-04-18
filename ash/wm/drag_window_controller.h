// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_WM_DRAG_WINDOW_CONTROLLER_H_
#define ASH_WM_DRAG_WINDOW_CONTROLLER_H_

#include <memory>
#include <vector>

#include "ash/ash_export.h"
#include "base/gtest_prod_util.h"
#include "base/macros.h"
#include "ui/gfx/geometry/rect.h"

namespace aura {
class Window;
}

namespace ui {
class LayerTreeOwner;
}

namespace ash {

// DragWindowController is responsible for showing a semi-transparent window
// while dragging a window across displays.
class ASH_EXPORT DragWindowController {
 public:
  // Computes the opacity for drag window based on how much of the area
  // of the window is visible.
  static float GetDragWindowOpacity(const gfx::Rect& window_bounds,
                                    const gfx::Rect& visible_bounds);

  explicit DragWindowController(aura::Window* window);
  virtual ~DragWindowController();

  // This is used to update the bounds and opacity for the drag window
  // immediately.
  // This also creates/destorys the drag window when necessary.
  void Update(const gfx::Rect& bounds_in_screen,
              const gfx::Point& drag_location_in_screen);

 private:
  class DragWindowDetails;
  FRIEND_TEST_ALL_PREFIXES(DragWindowResizerTest, DragWindowController);
  FRIEND_TEST_ALL_PREFIXES(DragWindowResizerTest,
                           DragWindowControllerAcrossThreeDisplays);

  // Returns the currently active drag windows.
  int GetDragWindowsCountForTest() const;

  // Returns the drag window/layer owner for given index of the
  // currently active drag windows list.
  const aura::Window* GetDragWindowForTest(size_t index) const;
  const ui::LayerTreeOwner* GetDragLayerOwnerForTest(size_t index) const;

  // Call Layer::OnPaintLayer on all layers under the drag_windows_.
  void RequestLayerPaintForTest();

  // Window the drag window is placed beneath.
  aura::Window* window_;

  std::vector<std::unique_ptr<DragWindowDetails>> drag_windows_;

  DISALLOW_COPY_AND_ASSIGN(DragWindowController);
};

}  // namespace ash

#endif  // ASH_WM_DRAG_WINDOW_CONTROLLER_H_
