// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/system/user/tray_user.h"

#include "ash/resources/vector_icons/vector_icons.h"
#include "ash/session/session_controller.h"
#include "ash/shelf/shelf.h"
#include "ash/shell.h"
#include "ash/strings/grit/ash_strings.h"
#include "ash/system/tray/system_tray.h"
#include "ash/system/tray/tray_constants.h"
#include "ash/system/tray/tray_item_view.h"
#include "ash/system/tray/tray_utils.h"
#include "ash/system/user/rounded_image_view.h"
#include "ash/system/user/user_view.h"
#include "base/logging.h"
#include "base/strings/string16.h"
#include "components/account_id/account_id.h"
#include "components/user_manager/user_info.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/gfx/geometry/insets.h"
#include "ui/gfx/image/image.h"
#include "ui/gfx/paint_vector_icon.h"
#include "ui/views/border.h"
#include "ui/views/controls/image_view.h"
#include "ui/views/controls/label.h"
#include "ui/views/layout/box_layout.h"
#include "ui/views/view.h"
#include "ui/views/widget/widget.h"

namespace {

const int kUserLabelToIconPadding = 5;

views::ImageView* CreateIcon() {
  gfx::ImageSkia icon_image =
      gfx::CreateVectorIcon(ash::kSystemMenuChildUserIcon, ash::kTrayIconColor);
  auto* image = new views::ImageView;
  image->SetImage(icon_image);
  return image;
}

}  // namespace

namespace ash {

TrayUser::TrayUser(SystemTray* system_tray)
    : SystemTrayItem(system_tray, UMA_USER), scoped_session_observer_(this) {}

TrayUser::~TrayUser() = default;

TrayUser::TestState TrayUser::GetStateForTest() const {
  if (!user_)
    return HIDDEN;
  return user_->GetStateForTest();
}

gfx::Size TrayUser::GetLayoutSizeForTest() const {
  return layout_view_ ? layout_view_->size() : gfx::Size();
}

gfx::Rect TrayUser::GetUserPanelBoundsInScreenForTest() const {
  return user_->GetBoundsInScreenOfUserButtonForTest();
}

void TrayUser::UpdateAfterLoginStatusChangeForTest(LoginStatus status) {
  UpdateAfterLoginStatusChange(status);
}

views::View* TrayUser::CreateTrayView(LoginStatus status) {
  DCHECK(!layout_view_);
  layout_view_ = new views::View;
  UpdateAfterLoginStatusChange(status);
  return layout_view_;
}

views::View* TrayUser::CreateDefaultView(LoginStatus status) {
  if (status == LoginStatus::NOT_LOGGED_IN)
    return nullptr;

  DCHECK(!user_);
  user_ = new tray::UserView(this, status);
  return user_;
}

void TrayUser::OnTrayViewDestroyed() {
  layout_view_ = nullptr;
  avatar_ = nullptr;
  label_ = nullptr;
}

void TrayUser::OnDefaultViewDestroyed() {
  user_ = nullptr;
}

void TrayUser::UpdateAfterLoginStatusChange(LoginStatus status) {
  SessionController* session = Shell::Get()->session_controller();
  bool need_label = session->IsUserSupervised();
  bool need_avatar = false;
  bool need_icon = false;
  switch (status) {
    case LoginStatus::LOCKED:
    case LoginStatus::USER:
    case LoginStatus::OWNER:
    case LoginStatus::PUBLIC:
      need_avatar = true;
      break;
    case LoginStatus::SUPERVISED:
      need_avatar = true;
      if (session->IsUserChild()) {
        need_label = false;
        need_icon = true;
      } else {
        need_label = true;
      }
      break;
    case LoginStatus::GUEST:
      need_label = true;
      break;
    case LoginStatus::KIOSK_APP:
    case LoginStatus::ARC_KIOSK_APP:
    case LoginStatus::NOT_LOGGED_IN:
      break;
  }

  if ((need_avatar != (avatar_ != nullptr)) ||
      (need_label != (label_ != nullptr)) ||
      (need_icon != (icon_ != nullptr))) {
    delete label_;
    delete avatar_;
    delete icon_;
    label_ = nullptr;
    avatar_ = nullptr;
    icon_ = nullptr;

    if (need_icon) {
      icon_ = CreateIcon();
      layout_view_->AddChildView(icon_);
    }
    if (need_label) {
      label_ = new views::Label;
      SetupLabelForTray(label_);
      layout_view_->AddChildView(label_);
    }
    if (need_avatar) {
      avatar_ = new tray::RoundedImageView(kTrayRoundedBorderRadius);
      avatar_->SetPaintToLayer();
      avatar_->layer()->SetFillsBoundsOpaquely(false);
      layout_view_->AddChildView(avatar_);
    }
  }

  if (label_ && session->IsUserSupervised()) {
    label_->SetText(
        l10n_util::GetStringUTF16(IDS_ASH_STATUS_TRAY_SUPERVISED_LABEL));
  } else if (status == LoginStatus::GUEST) {
    label_->SetText(l10n_util::GetStringUTF16(IDS_ASH_STATUS_TRAY_GUEST_LABEL));
  }

  UpdateAvatarImage(status);

  // Update layout after setting label_, icon_ and avatar_ with new login
  // status.
  UpdateLayoutOfItem();
}

void TrayUser::UpdateAfterShelfAlignmentChange() {
  if (system_tray()->shelf()->IsHorizontalAlignment()) {
    if (avatar_) {
      avatar_->SetCornerRadii(0, kTrayRoundedBorderRadius,
                              kTrayRoundedBorderRadius, 0);
    }
    if (label_) {
      // If label_ hasn't figured out its size yet, do that first.
      if (label_->GetContentsBounds().height() == 0)
        label_->SizeToPreferredSize();
      int height = label_->GetContentsBounds().height();
      int vertical_pad = (kTrayItemSize - height) / 2;
      int remainder = height % 2;
      label_->SetBorder(views::CreateEmptyBorder(
          vertical_pad + remainder,
          kTrayLabelItemHorizontalPaddingBottomAlignment, vertical_pad,
          kTrayLabelItemHorizontalPaddingBottomAlignment));
    }
    layout_view_->SetLayoutManager(std::make_unique<views::BoxLayout>(
        views::BoxLayout::kHorizontal, gfx::Insets(), kUserLabelToIconPadding));
  } else {
    if (avatar_) {
      avatar_->SetCornerRadii(0, 0, kTrayRoundedBorderRadius,
                              kTrayRoundedBorderRadius);
    }
    if (label_) {
      label_->SetBorder(views::CreateEmptyBorder(
          kTrayLabelItemVerticalPaddingVerticalAlignment,
          kTrayLabelItemHorizontalPaddingBottomAlignment,
          kTrayLabelItemVerticalPaddingVerticalAlignment,
          kTrayLabelItemHorizontalPaddingBottomAlignment));
    }
    layout_view_->SetLayoutManager(std::make_unique<views::BoxLayout>(
        views::BoxLayout::kVertical, gfx::Insets(), kUserLabelToIconPadding));
  }
}

void TrayUser::OnActiveUserSessionChanged(const AccountId& account_id) {
  OnUserSessionUpdated(account_id);
}

void TrayUser::OnUserSessionAdded(const AccountId& account_id) {
  // Enforce a layout change that newly added items become visible.
  UpdateLayoutOfItem();

  // Update the user item.
  UpdateAvatarImage(Shell::Get()->session_controller()->login_status());
}

void TrayUser::OnUserSessionUpdated(const AccountId& account_id) {
  UpdateAvatarImage(Shell::Get()->session_controller()->login_status());
}

void TrayUser::UpdateAvatarImage(LoginStatus status) {
  const SessionController* session_controller =
      Shell::Get()->session_controller();
  if (!avatar_ || session_controller->NumberOfLoggedInUsers() == 0)
    return;

  const mojom::UserSession* const user_session =
      session_controller->GetUserSession(0);
  avatar_->SetImage(user_session->user_info->avatar->image,
                    gfx::Size(kTrayItemSize, kTrayItemSize));

  // Unit tests might come here with no images for some users.
  if (avatar_->size().IsEmpty())
    avatar_->SetSize(gfx::Size(kTrayItemSize, kTrayItemSize));
}

void TrayUser::UpdateLayoutOfItem() {
  UpdateAfterShelfAlignmentChange();
}

}  // namespace ash
