// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_LOGIN_UI_LOGIN_BUBBLE_H_
#define ASH_LOGIN_UI_LOGIN_BUBBLE_H_

#include "ash/ash_export.h"
#include "ash/login/ui/login_base_bubble_view.h"
#include "base/strings/string16.h"
#include "components/user_manager/user_type.h"
#include "ui/aura/client/focus_change_observer.h"
#include "ui/compositor/layer_animation_observer.h"
#include "ui/views/view.h"
#include "ui/views/widget/widget_observer.h"

namespace ash {
class LoginButton;
class LoginMenuView;

// A wrapper for the bubble view in the login screen.
// This class observes keyboard events, mouse clicks and touch down events
// and dismisses the bubble accordingly.
class ASH_EXPORT LoginBubble : public views::WidgetObserver,
                               public ui::EventHandler,
                               public ui::LayerAnimationObserver,
                               public aura::client::FocusChangeObserver {
 public:
  static const int kUserMenuRemoveUserButtonIdForTest;

  // Flags passed to ShowErrorBubble().
  static constexpr uint32_t kFlagsNone = 0;
  // If set, the shown error bubble will not be closed due to an unrelated user
  // action - e.g. the bubble will not be closed if the user starts typing.
  static constexpr uint32_t kFlagPersistent = 1 << 0;

  LoginBubble();
  ~LoginBubble() override;

  // Shows an error bubble for authentication failure.
  // |anchor_view| is the anchor for placing the bubble view.
  void ShowErrorBubble(views::View* content,
                       views::View* anchor_view,
                       uint32_t flags);

  // Shows a user menu bubble.
  // |anchor_view| is the anchor for placing the bubble view.
  // |bubble_opener| is a view that could open/close the bubble.
  // |show_remove_user| indicate whether or not we show the
  // "Remove this user" action.
  void ShowUserMenu(const base::string16& username,
                    const base::string16& email,
                    user_manager::UserType type,
                    bool is_owner,
                    views::View* anchor_view,
                    LoginButton* bubble_opener,
                    bool show_remove_user,
                    base::OnceClosure on_remove_user_warning_shown,
                    base::OnceClosure on_remove_user_requested);

  // Shows a tooltip.
  void ShowTooltip(const base::string16& message, views::View* anchor_view);

  // Shows a selection menu.
  void ShowSelectionMenu(LoginMenuView* menu, LoginButton* bubble_opener);

  // Schedule animation for closing the bubble.
  // The bubble widget will be closed when the animation is ended.
  void Close();

  // Close the bubble immediately, without scheduling animation.
  // Used to clean up old bubble widget when a new bubble is going to be
  // created or it will be called before anchor view is hidden.
  void CloseImmediately();

  // True if the bubble is visible.
  bool IsVisible();

  // views::WidgetObservers:
  void OnWidgetClosing(views::Widget* widget) override;
  void OnWidgetDestroying(views::Widget* widget) override;

  // ui::EventHandler:
  void OnMouseEvent(ui::MouseEvent* event) override;
  void OnGestureEvent(ui::GestureEvent* event) override;
  void OnKeyEvent(ui::KeyEvent* event) override;

  // gfx::LayerAnimationObserver:
  void OnLayerAnimationEnded(ui::LayerAnimationSequence* sequence) override;
  void OnLayerAnimationAborted(ui::LayerAnimationSequence* sequence) override{};
  void OnLayerAnimationScheduled(
      ui::LayerAnimationSequence* sequence) override{};

  // aura::client::FocusChangeObserver:
  void OnWindowFocused(aura::Window* gained_focus,
                       aura::Window* lost_focus) override;

  LoginBaseBubbleView* bubble_view() { return bubble_view_; }

 private:
  // Show the bubble widget and schedule animation for bubble showing.
  void Show();

  void ProcessPressedEvent(const ui::LocatedEvent* event);

  // Starts show/hide animation.
  void ScheduleAnimation(bool visible);

  // Reset local states and close the widget if it is not already closing.
  // |widget_already_closing| : True if we don't need to close the widget
  // explicitly. False otherwise.
  void Reset(bool widget_already_closing);

  // Flags passed to ShowErrorBubble().
  uint32_t flags_ = kFlagsNone;

  LoginBaseBubbleView* bubble_view_ = nullptr;

  // A button that could open/close the bubble.
  LoginButton* bubble_opener_ = nullptr;

  // The status of bubble after animation ends.
  bool is_visible_ = false;

  DISALLOW_COPY_AND_ASSIGN(LoginBubble);
};

}  // namespace ash

#endif  // ASH_LOGIN_UI_LOGIN_BUBBLE_H_
