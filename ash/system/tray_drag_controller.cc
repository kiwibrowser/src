// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/system/tray_drag_controller.h"

#include "ash/app_list/app_list_controller_impl.h"
#include "ash/shell.h"
#include "ash/system/tray/tray_background_view.h"
#include "ash/wm/tablet_mode/tablet_mode_controller.h"

namespace ash {

TrayDragController::TrayDragController(Shelf* shelf) : shelf_(shelf) {}

void TrayDragController::ProcessGestureEvent(ui::GestureEvent* event,
                                             TrayBackgroundView* tray_view) {
  if (!Shell::Get()
           ->tablet_mode_controller()
           ->IsTabletModeWindowManagerEnabled() ||
      !shelf_->IsHorizontalAlignment()) {
    return;
  }

  // Disable the tray view swiping if the app list is opened.
  if (Shell::Get()->app_list_controller()->IsVisible())
    return;

  tray_view_ = tray_view;
  is_on_bubble_ = event->target() != tray_view;
  if (event->type() == ui::ET_GESTURE_SCROLL_BEGIN) {
    StartGestureDrag(*event);

    // Should not handle the event if the scroll sequence begins to scroll
    // upward on the tray view, let the shelf handle the event instead.
    if (!is_on_bubble_ && event->details().scroll_y_hint() > 0)
      return;

    event->SetHandled();
    return;
  }

  if (!tray_view_->GetBubbleView() || !is_in_drag_)
    return;

  if (event->type() == ui::ET_GESTURE_SCROLL_UPDATE) {
    UpdateGestureDrag(*event);
    event->SetHandled();
    return;
  }

  if (event->type() == ui::ET_GESTURE_SCROLL_END ||
      event->type() == ui::ET_SCROLL_FLING_START) {
    CompleteGestureDrag(*event);
    event->SetHandled();
    return;
  }

  // Unexpected event. Reset the drag state and close the bubble.
  is_in_drag_ = false;
  tray_view_->CloseBubble();
}

void TrayDragController::StartGestureDrag(const ui::GestureEvent& gesture) {
  if (!is_on_bubble_) {
    // Dragging on the tray view when the tray bubble is opened should do
    // nothing.
    if (tray_view_->GetBubbleView())
      return;

    // Should not open the tray bubble if the scroll sequence begins to scroll
    // downward. The event will instead handled by the shelf.
    if (gesture.details().scroll_y_hint() > 0)
      return;

    tray_view_->ShowBubble(true /* show_by_click */);
  }

  if (!tray_view_->GetBubbleView())
    return;

  is_in_drag_ = true;
  gesture_drag_amount_ = 0.f;
  tray_bubble_bounds_ =
      tray_view_->GetBubbleView()->GetWidget()->GetWindowBoundsInScreen();
  UpdateBubbleBounds();
}

void TrayDragController::UpdateGestureDrag(const ui::GestureEvent& gesture) {
  gesture_drag_amount_ += gesture.details().scroll_y();
  UpdateBubbleBounds();
}

void TrayDragController::CompleteGestureDrag(const ui::GestureEvent& gesture) {
  const bool hide_bubble = !ShouldShowBubbleAfterScrollSequence(gesture);
  gfx::Rect target_bounds = tray_bubble_bounds_;

  if (hide_bubble)
    target_bounds.set_y(shelf_->GetIdealBounds().y());

  tray_view_->AnimateToTargetBounds(target_bounds, hide_bubble);
  is_in_drag_ = false;

  UserMetricsRecorder* metrics = Shell::Get()->metrics();
  if (is_on_bubble_) {
    metrics->RecordUserMetricsAction(
        hide_bubble ? UMA_TRAY_SWIPE_TO_CLOSE_SUCCESSFUL
                    : UMA_TRAY_SWIPE_TO_CLOSE_UNSUCCESSFUL);
  } else {
    metrics->RecordUserMetricsAction(hide_bubble
                                         ? UMA_TRAY_SWIPE_TO_OPEN_UNSUCCESSFUL
                                         : UMA_TRAY_SWIPE_TO_OPEN_SUCCESSFUL);
  }
}

void TrayDragController::UpdateBubbleBounds() {
  DCHECK(tray_view_->GetBubbleView());

  gfx::Rect current_tray_bubble_bounds = tray_bubble_bounds_;
  const int bounds_y =
      (is_on_bubble_ ? tray_bubble_bounds_.y() : shelf_->GetIdealBounds().y()) +
      gesture_drag_amount_;
  current_tray_bubble_bounds.set_y(std::max(bounds_y, tray_bubble_bounds_.y()));
  tray_view_->GetBubbleView()->GetWidget()->SetBounds(
      current_tray_bubble_bounds);
}

bool TrayDragController::ShouldShowBubbleAfterScrollSequence(
    const ui::GestureEvent& sequence_end) {
  // If the scroll sequence terminates with a fling, show the bubble if the
  // fling was fast enough and in the correct direction.
  if (sequence_end.type() == ui::ET_SCROLL_FLING_START &&
      fabs(sequence_end.details().velocity_y()) > kFlingVelocity) {
    return sequence_end.details().velocity_y() < 0;
  }

  DCHECK(sequence_end.type() == ui::ET_GESTURE_SCROLL_END ||
         sequence_end.type() == ui::ET_SCROLL_FLING_START);

  // Keep the bubble's original state if the |gesture_drag_amount_| doesn't
  // exceed one-third of the bubble's height.
  if (is_on_bubble_)
    return gesture_drag_amount_ < tray_bubble_bounds_.height() / 3.0;
  return -gesture_drag_amount_ >= tray_bubble_bounds_.height() / 3.0;
}

}  // namespace ash
