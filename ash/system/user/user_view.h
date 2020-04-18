// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_SYSTEM_USER_USER_VIEW_H_
#define ASH_SYSTEM_USER_USER_VIEW_H_

#include <memory>

#include "ash/public/cpp/session_types.h"
#include "ash/system/tray/tray_constants.h"
#include "ash/system/user/tray_user.h"
#include "base/macros.h"
#include "ui/views/controls/button/button.h"
#include "ui/views/focus/focus_manager.h"
#include "ui/views/layout/box_layout.h"
#include "ui/views/view.h"

namespace gfx {
class Rect;
}

namespace views {
class FocusManager;
}

namespace ash {

enum class LoginStatus;
class SystemTrayItem;

namespace tray {

class UserCardView;

// The view of a user item in system tray bubble.
class UserView : public views::View,
                 public views::ButtonListener,
                 public views::FocusChangeListener {
 public:
  UserView(SystemTrayItem* owner, LoginStatus login);
  ~UserView() override;

  TrayUser::TestState GetStateForTest() const;
  gfx::Rect GetBoundsInScreenOfUserButtonForTest();

  views::View* user_card_view_for_test() const { return user_card_container_; }

 private:
  // Overridden from views::View.
  int GetHeightForWidth(int width) const override;

  // Overridden from views::ButtonListener.
  void ButtonPressed(views::Button* sender, const ui::Event& event) override;

  // Overridden from views::FocusChangeListener:
  void OnWillChangeFocus(View* focused_before, View* focused_now) override;
  void OnDidChangeFocus(View* focused_before, View* focused_now) override;

  void AddLogoutButton(LoginStatus login);
  void AddUserCard(LoginStatus login);

  // Create the menu option to add another user.
  void ToggleUserDropdownWidget(bool toggled_by_key_event);

  // Removes the add user menu option.
  void HideUserDropdownWidget();

  views::Widget* GetBubbleWidget();

  // If |user_card_view_| is clickable, this is a ButtonFromView that wraps it.
  // If |user_card_view_| is not clickable, this will be equal to
  // |user_card_view_|.
  views::View* user_card_container_ = nullptr;

  // The |UserCardView| for the active user.
  UserCardView* user_card_view_ = nullptr;

  // This is the owner system tray item of this view.
  SystemTrayItem* owner_;

  views::View* logout_button_ = nullptr;
  std::unique_ptr<views::Widget> user_dropdown_widget_;
  // Tracks whether |user_dropdown_widget_| was opened with a key event. If
  // true, HideUserDropdownWidget() will return focus to |user_card_container_|.
  bool user_dropdown_widget_toggled_by_key_event_ = false;

  // False when the add user panel is visible but not activatable.
  bool add_user_enabled_ = true;

  // The focus manager which we use to detect focus changes.
  views::FocusManager* focus_manager_ = nullptr;

  DISALLOW_COPY_AND_ASSIGN(UserView);
};

}  // namespace tray
}  // namespace ash

#endif  // ASH_SYSTEM_USER_USER_VIEW_H_
