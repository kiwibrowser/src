// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/message_center/fake_message_center.h"
#include "base/strings/string_util.h"
#include "ui/message_center/notification_list.h"

namespace message_center {

FakeMessageCenter::FakeMessageCenter() {
}

FakeMessageCenter::~FakeMessageCenter() {
}

void FakeMessageCenter::AddObserver(MessageCenterObserver* observer) {
}

void FakeMessageCenter::RemoveObserver(MessageCenterObserver* observer) {
}

void FakeMessageCenter::AddNotificationBlocker(NotificationBlocker* blocker) {
}

void FakeMessageCenter::RemoveNotificationBlocker(
    NotificationBlocker* blocker) {
}

size_t FakeMessageCenter::NotificationCount() const {
  return 0u;
}

bool FakeMessageCenter::HasPopupNotifications() const {
  return false;
}

bool FakeMessageCenter::IsQuietMode() const {
  return false;
}

Notification* FakeMessageCenter::FindVisibleNotificationById(
    const std::string& id) {
  for (auto* notification : GetVisibleNotifications()) {
    if (id == notification->id())
      return notification;
  }
  return nullptr;
}

NotificationList::Notifications FakeMessageCenter::FindNotificationsByAppId(
    const std::string& app_id) {
  return NotificationList::Notifications();
}

const NotificationList::Notifications&
FakeMessageCenter::GetVisibleNotifications() {
  return empty_notifications_;
}

NotificationList::PopupNotifications
    FakeMessageCenter::GetPopupNotifications() {
  return NotificationList::PopupNotifications();
}

void FakeMessageCenter::AddNotification(
    std::unique_ptr<Notification> notification) {}

void FakeMessageCenter::UpdateNotification(
    const std::string& old_id,
    std::unique_ptr<Notification> new_notification) {}

void FakeMessageCenter::RemoveNotification(const std::string& id,
                                           bool by_user) {
}

void FakeMessageCenter::RemoveNotificationsForNotifierId(
    const NotifierId& notifier_id) {}

void FakeMessageCenter::RemoveAllNotifications(bool by_user, RemoveType type) {}

void FakeMessageCenter::SetNotificationIcon(const std::string& notification_id,
                                            const gfx::Image& image) {
}

void FakeMessageCenter::SetNotificationImage(const std::string& notification_id,
                                             const gfx::Image& image) {
}

void FakeMessageCenter::SetNotificationButtonIcon(
    const std::string& notification_id,
    int button_index,
    const gfx::Image& image) {
}

void FakeMessageCenter::ClickOnNotification(const std::string& id) {
}

void FakeMessageCenter::ClickOnNotificationButton(const std::string& id,
                                                  int button_index) {
}

void FakeMessageCenter::ClickOnNotificationButtonWithReply(
    const std::string& id,
    int button_index,
    const base::string16& reply) {}

void FakeMessageCenter::ClickOnSettingsButton(const std::string& id) {}

void FakeMessageCenter::DisableNotification(const std::string& id) {}

void FakeMessageCenter::MarkSinglePopupAsShown(const std::string& id,
                                               bool mark_notification_as_read) {
}

void FakeMessageCenter::DisplayedNotification(const std::string& id,
                                              const DisplaySource source) {}

void FakeMessageCenter::SetQuietMode(bool in_quiet_mode) {}

void FakeMessageCenter::EnterQuietModeWithExpire(
    const base::TimeDelta& expires_in) {
}

void FakeMessageCenter::SetVisibility(Visibility visible) {
}

bool FakeMessageCenter::IsMessageCenterVisible() const {
  return false;
}

void FakeMessageCenter::SetHasMessageCenterView(bool has_message_center_view) {
  has_message_center_view_ = has_message_center_view;
}

bool FakeMessageCenter::HasMessageCenterView() const {
  return has_message_center_view_;
}

void FakeMessageCenter::RestartPopupTimers() {}

void FakeMessageCenter::PausePopupTimers() {}

const base::string16& FakeMessageCenter::GetSystemNotificationAppName() const {
  return base::EmptyString16();
}

void FakeMessageCenter::SetSystemNotificationAppName(
    const base::string16& product_os_name) {}

void FakeMessageCenter::DisableTimersForTest() {}

}  // namespace message_center
