// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_WM_TABLET_MODE_TABLET_MODE_WINDOW_RESIZER_H_
#define ASH_WM_TABLET_MODE_TABLET_MODE_WINDOW_RESIZER_H_

#include "ash/public/cpp/window_properties.h"
#include "ash/wm/splitview/split_view_controller.h"
#include "ash/wm/window_resizer.h"
#include "base/macros.h"
#include "base/memory/weak_ptr.h"

namespace ash {

namespace wm {
class WindowState;
}  // namespace wm

class PhantomWindowController;

// WindowResizer implementation for windows in tablet mode. Currently we don't
// allow any resizing and any dragging happening on the area other than the
// caption tabs area in tablet mode. Only browser windows with tabs are allowed
// to be dragged. Depending on the event position, the dragged window may be 1)
// maximized, or 2) snapped in splitscreen, or 3) merged to an existing window.
class TabletModeWindowResizer : public WindowResizer {
 public:
  explicit TabletModeWindowResizer(wm::WindowState* window_state);
  ~TabletModeWindowResizer() override;

  // WindowResizer:
  void Drag(const gfx::Point& location_in_parent, int event_flags) override;
  void CompleteDrag() override;
  void RevertDrag() override;

 private:
  void UpdateSnapPhantomWindow(const gfx::Point& location_in_parent);

  // Gets the desired snap position to show the phantom window. Phnatom window
  // is only shown up when the user drags pass the vertical threshold.
  SplitViewController::SnapPosition GetSnapPositionForPhantomWindow(
      const gfx::Point& location_in_parent) const;

  SplitViewController* split_view_controller_;

  // Gives a preview of where the dragged window will snapped to.
  std::unique_ptr<PhantomWindowController> phantom_window_controller_;

  gfx::Point previous_location_in_parent_;

  bool did_lock_cursor_ = false;

  // The backdrop should be disabled during dragging and resumed after dragging.
  BackdropWindowMode original_backdrop_mode_;

  // Used to determine where to show the phantom window.
  SplitViewController::SnapPosition snap_position_ = SplitViewController::NONE;

  // Used to determine if this has been deleted during a drag such as when a tab
  // gets dragged into another browser window.
  base::WeakPtrFactory<TabletModeWindowResizer> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(TabletModeWindowResizer);
};

}  // namespace ash

#endif  // ASH_WM_TABLET_MODE_TABLET_MODE_WINDOW_RESIZER_H_
