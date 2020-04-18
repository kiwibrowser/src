// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/system/unified/user_chooser_view.h"

#include "ash/public/cpp/shell_window_ids.h"
#include "ash/resources/vector_icons/vector_icons.h"
#include "ash/session/session_controller.h"
#include "ash/shell.h"
#include "ash/strings/grit/ash_strings.h"
#include "ash/system/tray/tray_constants.h"
#include "ash/system/unified/top_shortcuts_view.h"
#include "ash/system/unified/unified_system_tray_controller.h"
#include "ash/system/user/rounded_image_view.h"
#include "base/strings/utf_string_conversions.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/gfx/paint_vector_icon.h"
#include "ui/views/controls/button/label_button.h"
#include "ui/views/layout/box_layout.h"
#include "ui/views/widget/widget.h"

namespace ash {

views::View* CreateUserAvatarView(int user_index) {
  DCHECK(Shell::Get());
  const mojom::UserSession* const user_session =
      Shell::Get()->session_controller()->GetUserSession(user_index);
  DCHECK(user_session);

  auto* image_view = new tray::RoundedImageView(kTrayItemSize / 2);
  if (user_session->user_info->type == user_manager::USER_TYPE_GUEST) {
    gfx::ImageSkia icon =
        gfx::CreateVectorIcon(kSystemMenuGuestIcon, kMenuIconColor);
    image_view->SetImage(icon, icon.size());
  } else {
    image_view->SetImage(user_session->user_info->avatar->image,
                         gfx::Size(kTrayItemSize, kTrayItemSize));
  }
  return image_view;
}

namespace {

// A button item of a switchable user.
class UserItemButton : public views::Button, public views::ButtonListener {
 public:
  UserItemButton(int user_index, UnifiedSystemTrayController* controller);
  ~UserItemButton() override = default;

  // views::ButtonListener:
  void ButtonPressed(views::Button* sender, const ui::Event& event) override;

 private:
  int user_index_;
  UnifiedSystemTrayController* const controller_;

  DISALLOW_COPY_AND_ASSIGN(UserItemButton);
};

UserItemButton::UserItemButton(int user_index,
                               UnifiedSystemTrayController* controller)
    : Button(this), user_index_(user_index), controller_(controller) {
  SetLayoutManager(std::make_unique<views::BoxLayout>(
      views::BoxLayout::kHorizontal, gfx::Insets(kUnifiedTopShortcutSpacing),
      kUnifiedTopShortcutSpacing));
  AddChildView(CreateUserAvatarView(user_index));

  views::View* vertical_labels = new views::View;
  auto* layout = vertical_labels->SetLayoutManager(
      std::make_unique<views::BoxLayout>(views::BoxLayout::kVertical));
  layout->set_cross_axis_alignment(
      views::BoxLayout::CROSS_AXIS_ALIGNMENT_START);

  const mojom::UserSession* const user_session =
      Shell::Get()->session_controller()->GetUserSession(user_index);

  auto* name = new views::Label(
      base::UTF8ToUTF16(user_session->user_info->display_name));
  name->SetEnabledColor(kUnifiedMenuTextColor);
  name->SetAutoColorReadabilityEnabled(false);
  name->SetSubpixelRenderingEnabled(false);
  vertical_labels->AddChildView(name);

  auto* email = new views::Label(
      base::UTF8ToUTF16(user_session->user_info->display_email));
  email->SetEnabledColor(kUnifiedMenuSecondaryTextColor);
  email->SetAutoColorReadabilityEnabled(false);
  email->SetSubpixelRenderingEnabled(false);
  vertical_labels->AddChildView(email);

  AddChildView(vertical_labels);
}

void UserItemButton::ButtonPressed(views::Button* sender,
                                   const ui::Event& event) {
  controller_->HandleUserSwitch(user_index_);
}

// A button that will transition to multi profile login UI.
class AddUserButton : public views::Button, public views::ButtonListener {
 public:
  explicit AddUserButton(UnifiedSystemTrayController* controller);
  ~AddUserButton() override = default;

  // views::ButtonListener:
  void ButtonPressed(views::Button* sender, const ui::Event& event) override;

 private:
  UnifiedSystemTrayController* const controller_;

  DISALLOW_COPY_AND_ASSIGN(AddUserButton);
};

AddUserButton::AddUserButton(UnifiedSystemTrayController* controller)
    : Button(this), controller_(controller) {
  SetLayoutManager(std::make_unique<views::BoxLayout>(
      views::BoxLayout::kHorizontal, gfx::Insets(kUnifiedTopShortcutSpacing),
      kUnifiedTopShortcutSpacing));

  auto* icon = new views::ImageView;
  icon->SetImage(
      gfx::CreateVectorIcon(kSystemMenuNewUserIcon, kUnifiedMenuIconColor));
  AddChildView(icon);

  auto* label = new views::Label(
      l10n_util::GetStringUTF16(IDS_ASH_STATUS_TRAY_SIGN_IN_ANOTHER_ACCOUNT));
  label->SetEnabledColor(kUnifiedMenuTextColor);
  label->SetAutoColorReadabilityEnabled(false);
  label->SetSubpixelRenderingEnabled(false);
  AddChildView(label);
}

void AddUserButton::ButtonPressed(views::Button* sender,
                                  const ui::Event& event) {
  controller_->HandleAddUserAction();
}

}  // namespace

UserChooserView::UserChooserView(UnifiedSystemTrayController* controller) {
  SetLayoutManager(
      std::make_unique<views::BoxLayout>(views::BoxLayout::kVertical));
  const int num_users =
      Shell::Get()->session_controller()->NumberOfLoggedInUsers();
  for (int i = 0; i < num_users; ++i)
    AddChildView(new UserItemButton(i, controller));
  if (Shell::Get()->session_controller()->GetAddUserPolicy() ==
      AddUserSessionPolicy::ALLOWED) {
    AddChildView(new AddUserButton(controller));
  }
}

UserChooserView::~UserChooserView() = default;

}  // namespace ash
