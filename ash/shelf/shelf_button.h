// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_SHELF_SHELF_BUTTON_H_
#define ASH_SHELF_SHELF_BUTTON_H_

#include "ash/ash_export.h"
#include "base/macros.h"
#include "base/timer/timer.h"
#include "ui/gfx/shadow_value.h"
#include "ui/views/controls/button/button.h"

namespace views {
class ImageView;
}

namespace ash {
class InkDropButtonListener;
class ShelfView;

// Button used for items on the launcher, except for the AppList.
class ASH_EXPORT ShelfButton : public views::Button {
 public:
  static const char kViewClassName[];

  // Used to indicate the current state of the button.
  enum State {
    // Nothing special. Usually represents an app shortcut item with no running
    // instance.
    STATE_NORMAL = 0,
    // Button has mouse hovering on it.
    STATE_HOVERED = 1 << 0,
    // Underlying ShelfItem has a running instance.
    STATE_RUNNING = 1 << 1,
    // Underlying ShelfItem needs user's attention.
    STATE_ATTENTION = 1 << 2,
    // Hide the status (temporarily for some animations).
    STATE_HIDDEN = 1 << 3,
    // Button is being dragged.
    STATE_DRAGGING = 1 << 4,
    // App has at least 1 notification.
    STATE_NOTIFICATION = 1 << 5,
  };

  ShelfButton(InkDropButtonListener* listener, ShelfView* shelf_view);
  ~ShelfButton() override;

  // Sets the image to display for this entry.
  void SetImage(const gfx::ImageSkia& image);

  // Retrieve the image to show proxy operations.
  const gfx::ImageSkia& GetImage() const;

  // |state| is or'd into the current state.
  void AddState(State state);
  void ClearState(State state);
  int state() const { return state_; }

  // Returns the bounds of the icon.
  gfx::Rect GetIconBounds() const;

  views::InkDrop* GetInkDropForTesting();

  // Called when user started dragging the shelf button.
  void OnDragStarted(const ui::LocatedEvent* event);

  // Callback used when a menu for this ShelfButton is closed.
  void OnMenuClosed();

  // Overrides to views::Button:
  void ShowContextMenu(const gfx::Point& p,
                       ui::MenuSourceType source_type) override;

  // View override - needed by unit test.
  void OnMouseCaptureLost() override;

 protected:
  // View overrides:
  const char* GetClassName() const override;
  bool OnMousePressed(const ui::MouseEvent& event) override;
  void OnMouseReleased(const ui::MouseEvent& event) override;
  bool OnMouseDragged(const ui::MouseEvent& event) override;
  void GetAccessibleNodeData(ui::AXNodeData* node_data) override;
  void Layout() override;
  void ChildPreferredSizeChanged(views::View* child) override;

  // ui::EventHandler overrides:
  void OnGestureEvent(ui::GestureEvent* event) override;

  // views::Button overrides:
  std::unique_ptr<views::InkDropRipple> CreateInkDropRipple() const override;
  bool ShouldEnterPushedState(const ui::Event& event) override;
  std::unique_ptr<views::InkDrop> CreateInkDrop() override;
  void NotifyClick(const ui::Event& event) override;

  // Sets the icon image with a shadow.
  void SetShadowedImage(const gfx::ImageSkia& bitmap);

 private:
  class AppNotificationIndicatorView;
  class AppStatusIndicatorView;

  // Updates the parts of the button to reflect the current |state_| and
  // alignment. This may add or remove views, layout and paint.
  void UpdateState();

  // Invoked when |touch_drag_timer_| fires to show dragging UI.
  void OnTouchDragTimer();

  // Invoked when |ripple_activation_timer_| fires to activate the ink drop.
  void OnRippleTimer();

  // Scales up app icon if |scale_up| is true, otherwise scales it back to
  // normal size.
  void ScaleAppIcon(bool scale_up);

  InkDropButtonListener* listener_;

  // The shelf view hosting this button.
  ShelfView* shelf_view_;

  // The icon part of a button can be animated independently of the rest.
  views::ImageView* icon_view_;

  // Draws an indicator underneath the image to represent the state of the
  // application.
  AppStatusIndicatorView* indicator_;

  // Draws an indicator in the top right corner of the image to represent an
  // active notification.
  AppNotificationIndicatorView* notification_indicator_;

  // The current application state, a bitfield of State enum values.
  int state_;

  gfx::ShadowValues icon_shadows_;

  // If non-null the destuctor sets this to true. This is set while the menu is
  // showing and used to detect if the menu was deleted while running.
  bool* destroyed_flag_;

  // Whether the touchable context menu is enabled.
  const bool is_touchable_app_context_menu_enabled_;

  // A timer to defer showing drag UI when the shelf button is pressed.
  base::OneShotTimer drag_timer_;

  // A timer to activate the ink drop ripple during a long press.
  base::OneShotTimer ripple_activation_timer_;

  DISALLOW_COPY_AND_ASSIGN(ShelfButton);
};

}  // namespace ash

#endif  // ASH_SHELF_SHELF_BUTTON_H_
