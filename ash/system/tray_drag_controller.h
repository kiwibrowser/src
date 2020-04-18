// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_SYSTEM_TRAY_DRAG_CONTROLLER_H_
#define ASH_SYSTEM_TRAY_DRAG_CONTROLLER_H_

#include "ash/ash_export.h"
#include "ash/shelf/shelf.h"
#include "ui/views/view.h"

namespace ash {
class TrayBackgroundView;

// The TrayDragController helps to process the swiping events that happened on
// TrayBackgroundView or TrayBubbleView. Not all the TrayBackgroundView can be
// dragged currently. Only ImeMenuTray, SystemTray, PaletteTray,
// NotificationTray and their associated tray bubbles can be dragged.
class ASH_EXPORT TrayDragController {
 public:
  // The threshold of the velocity of the fling event.
  static constexpr float kFlingVelocity = 100.0f;

  explicit TrayDragController(Shelf* shelf);

  // Processes a gesture event and updates the dragging state of the bubble when
  // appropriate.
  void ProcessGestureEvent(ui::GestureEvent* event,
                           TrayBackgroundView* tray_view);

 private:
  // Gesture related functions:
  void StartGestureDrag(const ui::GestureEvent& gesture);
  void UpdateGestureDrag(const ui::GestureEvent& gesture);
  void CompleteGestureDrag(const ui::GestureEvent& gesture);

  // Update the bounds of the tray bubble according to
  // |gesture_drag_amount_|.
  void UpdateBubbleBounds();

  // Return true if the bubble should be shown (i.e., animated upward to
  // be made fully visible) after a sequence of scroll events terminated by
  // |sequence_end|. Otherwise return false, indicating that the
  // partially-visible bubble should be animated downward and made fully
  // hidden.
  bool ShouldShowBubbleAfterScrollSequence(
      const ui::GestureEvent& sequence_end);

  // The shelf containing the TrayBackgroundView.
  Shelf* shelf_;

  // The specific TrayBackgroundView that dragging happened on. e.g, SystemTray.
  TrayBackgroundView* tray_view_ = nullptr;

  // The original bounds of the tray bubble.
  gfx::Rect tray_bubble_bounds_;

  // Tracks the amount of the drag. Only valid if |is_in_drag_| is true.
  float gesture_drag_amount_ = 0.f;

  // True if the user is in the process of gesture-dragging on
  // TrayBackgroundView to open the tray bubble, or on the already-opened
  // TrayBubbleView to close it. Otherwise false.
  bool is_in_drag_ = false;

  // True if the dragging happened on the bubble view, false if it happened on
  // the tray view. Note, only valid if |is_in_drag_| is true.
  bool is_on_bubble_ = false;

  DISALLOW_COPY_AND_ASSIGN(TrayDragController);
};

}  // namespace ash

#endif  // ASH_SYSTEM_TRAY_DRAG_CONTROLLER_H_
