// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/message_center/ui_controller.h"

#include <memory>

#include "base/macros.h"
#include "base/observer_list.h"
#include "base/strings/utf_string_conversions.h"
#include "build/build_config.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/models/simple_menu_model.h"
#include "ui/message_center/message_center.h"
#include "ui/message_center/message_center_types.h"
#include "ui/message_center/notification_blocker.h"
#include "ui/message_center/ui_delegate.h"
#include "ui/message_center/views/notification_menu_model.h"
#include "ui/strings/grit/ui_strings.h"

namespace message_center {

UiController::UiController(UiDelegate* delegate)
    : message_center_(MessageCenter::Get()),
      message_center_visible_(false),
      popups_visible_(false),
      delegate_(delegate) {
  message_center_->AddObserver(this);
}

UiController::~UiController() {
  message_center_->RemoveObserver(this);
}

bool UiController::ShowMessageCenterBubble(bool show_by_click) {
  if (message_center_visible_)
    return true;

  HidePopupBubbleInternal();

  message_center_visible_ = delegate_->ShowMessageCenter(show_by_click);
  if (message_center_visible_) {
    message_center_->SetVisibility(VISIBILITY_MESSAGE_CENTER);
    NotifyUiControllerChanged();
  }
  return message_center_visible_;
}

bool UiController::HideMessageCenterBubble() {
  if (!message_center_visible_)
    return false;

  delegate_->HideMessageCenter();
  MarkMessageCenterHidden();

  return true;
}

void UiController::MarkMessageCenterHidden() {
  if (!message_center_visible_)
    return;
  message_center_visible_ = false;
  message_center_->SetVisibility(VISIBILITY_TRANSIENT);

  // Some notifications (like system ones) should appear as popups again
  // after the message center is closed.
  if (message_center_->HasPopupNotifications()) {
    ShowPopupBubble();
    return;
  }

  NotifyUiControllerChanged();
}

void UiController::ShowPopupBubble() {
  if (message_center_visible_)
    return;

  if (popups_visible_) {
    NotifyUiControllerChanged();
    return;
  }

  if (!message_center_->HasPopupNotifications())
    return;

  popups_visible_ = delegate_->ShowPopups();

  NotifyUiControllerChanged();
}

bool UiController::HidePopupBubble() {
  if (!popups_visible_)
    return false;
  HidePopupBubbleInternal();
  NotifyUiControllerChanged();

  return true;
}

void UiController::HidePopupBubbleInternal() {
  if (!popups_visible_)
    return;

  delegate_->HidePopups();
  popups_visible_ = false;
}

void UiController::ShowNotifierSettingsBubble() {
  if (popups_visible_)
    HidePopupBubbleInternal();

  message_center_visible_ = delegate_->ShowNotifierSettings();
  message_center_->SetVisibility(VISIBILITY_SETTINGS);

  NotifyUiControllerChanged();
}

void UiController::OnNotificationAdded(const std::string& notification_id) {
  OnMessageCenterChanged();
}

void UiController::OnNotificationRemoved(const std::string& notification_id,
                                         bool by_user) {
  OnMessageCenterChanged();
}

void UiController::OnNotificationUpdated(const std::string& notification_id) {
  OnMessageCenterChanged();
}

void UiController::OnNotificationClicked(
    const std::string& notification_id,
    const base::Optional<int>& button_index,
    const base::Optional<base::string16>& reply) {
  if (popups_visible_)
    OnMessageCenterChanged();
}

void UiController::OnNotificationSettingsClicked(bool handled) {
  if (!handled)
    ShowNotifierSettingsBubble();
}

void UiController::OnNotificationDisplayed(const std::string& notification_id,
                                           const DisplaySource source) {
  NotifyUiControllerChanged();
}

void UiController::OnQuietModeChanged(bool in_quiet_mode) {
  NotifyUiControllerChanged();
}

void UiController::OnBlockingStateChanged(NotificationBlocker* blocker) {
  OnMessageCenterChanged();
}

void UiController::OnMessageCenterChanged() {
  if (message_center_visible_ && message_center_->NotificationCount() == 0) {
    HideMessageCenterBubble();
    return;
  }

  if (popups_visible_ && !message_center_->HasPopupNotifications())
    HidePopupBubbleInternal();
  else if (!popups_visible_ && message_center_->HasPopupNotifications())
    ShowPopupBubble();

  NotifyUiControllerChanged();
}

void UiController::NotifyUiControllerChanged() {
  delegate_->OnMessageCenterContentsChanged();
}

}  // namespace message_center
