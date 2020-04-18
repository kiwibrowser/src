// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/wm/tablet_mode/tablet_mode_window_resizer.h"

#include "ash/screen_util.h"
#include "ash/shell.h"
#include "ash/shell_port.h"
#include "ash/wm/drag_details.h"
#include "ash/wm/overview/window_selector_controller.h"
#include "ash/wm/workspace/phantom_window_controller.h"
#include "ui/aura/window.h"
#include "ui/base/hit_test.h"
#include "ui/display/screen.h"
#include "ui/wm/core/coordinate_conversion.h"

namespace ash {

namespace {

// The threshold to compute the the minimum vertical distance to start showing
// the phantom window.
constexpr float kThresholdRatio = 0.25;

}  // namespace

TabletModeWindowResizer::TabletModeWindowResizer(wm::WindowState* window_state)
    : WindowResizer(window_state),
      split_view_controller_(Shell::Get()->split_view_controller()),
      weak_ptr_factory_(this) {
  DCHECK(details().is_resizable);

  if (details().source != ::wm::WINDOW_MOVE_SOURCE_TOUCH &&
      !window_state->allow_set_bounds_direct()) {
    ShellPort::Get()->LockCursor();
    did_lock_cursor_ = true;
  }

  previous_location_in_parent_ = details().initial_location_in_parent;
  window_state_->OnDragStarted(details().window_component);

  // Disable the backdrop on the dragged window.
  original_backdrop_mode_ = GetTarget()->GetProperty(kBackdropWindowMode);
  GetTarget()->SetProperty(kBackdropWindowMode, BackdropWindowMode::kDisabled);
  split_view_controller_->OnWindowDragStarted(GetTarget());
}

TabletModeWindowResizer::~TabletModeWindowResizer() {
  if (did_lock_cursor_)
    ShellPort::Get()->UnlockCursor();
}

void TabletModeWindowResizer::Drag(const gfx::Point& location_in_parent,
                                   int event_flags) {
  // Update phantom window if necessary.
  UpdateSnapPhantomWindow(location_in_parent);

  // TODO(xdai): Do scale transform of the initiator window if the initiator
  // window is not the dragged window.
  // TODO(xdai): Show/Update the drag-to-snap indicator.

  // Update dragged window's bounds.
  gfx::Rect bounds = CalculateBoundsForDrag(location_in_parent);
  if (bounds != GetTarget()->bounds()) {
    base::WeakPtr<TabletModeWindowResizer> resizer(
        weak_ptr_factory_.GetWeakPtr());
    GetTarget()->SetBounds(bounds);
    if (!resizer)
      return;
  }

  previous_location_in_parent_ = location_in_parent;
}

void TabletModeWindowResizer::CompleteDrag() {
  gfx::Point previous_location_in_screen = previous_location_in_parent_;
  ::wm::ConvertPointToScreen(GetTarget()->parent(),
                             &previous_location_in_screen);
  window_state_->OnCompleteDrag(previous_location_in_screen);
  GetTarget()->SetProperty(kBackdropWindowMode, original_backdrop_mode_);
  phantom_window_controller_.reset();

  // At this moment we could not decide what might happen to the dragged window.
  // It can either 1) be a new window or 2) be destoryed due to attaching into
  // another browser window. We should avoid to snap a to-be-destroyed window.
  // Start observing it until we can decide what to do next.
  split_view_controller_->OnWindowDragEnded(GetTarget(), snap_position_,
                                            previous_location_in_screen);
}

void TabletModeWindowResizer::RevertDrag() {
  gfx::Point previous_location_in_screen = previous_location_in_parent_;
  ::wm::ConvertPointToScreen(GetTarget()->parent(),
                             &previous_location_in_screen);
  window_state_->OnRevertDrag(previous_location_in_screen);
  GetTarget()->SetProperty(kBackdropWindowMode, original_backdrop_mode_);
  phantom_window_controller_.reset();
  split_view_controller_->OnWindowDragEnded(
      GetTarget(),
      /*desired_snap_position=*/SplitViewController::NONE,
      previous_location_in_screen);
}

void TabletModeWindowResizer::UpdateSnapPhantomWindow(
    const gfx::Point& location_in_parent) {
  SplitViewController::SnapPosition previous_snap_position = snap_position_;
  snap_position_ = GetSnapPositionForPhantomWindow(location_in_parent);
  if (snap_position_ == SplitViewController::NONE ||
      snap_position_ != previous_snap_position) {
    phantom_window_controller_.reset();
    if (snap_position_ == SplitViewController::NONE)
      return;
  }

  if (!phantom_window_controller_) {
    phantom_window_controller_ =
        std::make_unique<PhantomWindowController>(GetTarget());
  }
  phantom_window_controller_->Show(
      split_view_controller_->GetSnappedWindowBoundsInScreen(GetTarget(),
                                                             snap_position_));
}

SplitViewController::SnapPosition
TabletModeWindowResizer::GetSnapPositionForPhantomWindow(
    const gfx::Point& location_in_parent) const {
  const gfx::Rect work_area_bounds(
      screen_util::GetDisplayWorkAreaBoundsInParent(GetTarget()));

  // The user has to drag pass the vertical threshold to snap the window.
  const int vertical_threshold = work_area_bounds.height() * kThresholdRatio;
  if (location_in_parent.y() < vertical_threshold)
    return SplitViewController::NONE;

  gfx::Point location_in_screen(location_in_parent);
  ::wm::ConvertPointToScreen(GetTarget()->parent(), &location_in_screen);
  const int divider_position =
      split_view_controller_->IsSplitViewModeActive()
          ? split_view_controller_->divider_position()
          : split_view_controller_->GetDefaultDividerPosition(GetTarget());
  const int position =
      split_view_controller_->IsCurrentScreenOrientationLandscape()
          ? location_in_screen.x()
          : location_in_screen.y();
  return (position <= divider_position)
             ? split_view_controller_->IsCurrentScreenOrientationPrimary()
                   ? SplitViewController::LEFT
                   : SplitViewController::RIGHT
             : split_view_controller_->IsCurrentScreenOrientationPrimary()
                   ? SplitViewController::RIGHT
                   : SplitViewController::LEFT;
}

}  // namespace ash
