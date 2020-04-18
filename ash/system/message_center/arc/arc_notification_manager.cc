// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/system/message_center/arc/arc_notification_manager.h"

#include <memory>
#include <utility>

#include "ash/system/message_center/arc/arc_notification_delegate.h"
#include "ash/system/message_center/arc/arc_notification_item_impl.h"
#include "ash/system/message_center/arc/arc_notification_manager_delegate.h"
#include "ash/system/message_center/arc/arc_notification_view.h"
#include "base/stl_util.h"
#include "base/strings/utf_string_conversions.h"
#include "components/arc/mojo_channel.h"
#include "ui/message_center/views/message_view_factory.h"

using arc::ConnectionHolder;
using arc::MojoChannel;
using arc::mojom::ArcNotificationData;
using arc::mojom::ArcNotificationDataPtr;
using arc::mojom::ArcNotificationEvent;
using arc::mojom::ArcNotificationPriority;
using arc::mojom::NotificationsHost;
using arc::mojom::NotificationsInstance;
using arc::mojom::NotificationsInstancePtr;

namespace ash {
namespace {

constexpr char kPlayStorePackageName[] = "com.android.vending";

std::unique_ptr<message_center::MessageView> CreateCustomMessageView(
    const message_center::Notification& notification) {
  DCHECK_EQ(notification.notifier_id().type,
            message_center::NotifierId::ARC_APPLICATION);
  auto* arc_delegate =
      static_cast<ArcNotificationDelegate*>(notification.delegate());
  return arc_delegate->CreateCustomMessageView(notification);
}

}  // namespace

class ArcNotificationManager::InstanceOwner {
 public:
  InstanceOwner() = default;
  ~InstanceOwner() = default;

  void SetInstancePtr(NotificationsInstancePtr instance_ptr) {
    DCHECK(!channel_);

    channel_ =
        std::make_unique<MojoChannel<NotificationsInstance, NotificationsHost>>(
            &holder_, std::move(instance_ptr));

    // Using base::Unretained because |this| owns |channel_|.
    channel_->set_connection_error_handler(
        base::BindOnce(&InstanceOwner::OnDisconnected, base::Unretained(this)));
    channel_->QueryVersion();
  }

  ConnectionHolder<NotificationsInstance, NotificationsHost>* holder() {
    return &holder_;
  }

 private:
  void OnDisconnected() { channel_.reset(); }

  ConnectionHolder<NotificationsInstance, NotificationsHost> holder_;
  std::unique_ptr<MojoChannel<NotificationsInstance, NotificationsHost>>
      channel_;

  DISALLOW_COPY_AND_ASSIGN(InstanceOwner);
};

// static
void ArcNotificationManager::SetCustomNotificationViewFactory() {
  message_center::MessageViewFactory::SetCustomNotificationViewFactory(
      base::BindRepeating(&CreateCustomMessageView));
}

ArcNotificationManager::ArcNotificationManager(
    std::unique_ptr<ArcNotificationManagerDelegate> delegate,
    const AccountId& main_profile_id,
    message_center::MessageCenter* message_center)
    : delegate_(std::move(delegate)),
      main_profile_id_(main_profile_id),
      message_center_(message_center),
      instance_owner_(std::make_unique<InstanceOwner>()) {
  instance_owner_->holder()->SetHost(this);
  instance_owner_->holder()->AddObserver(this);
  if (!message_center::MessageViewFactory::HasCustomNotificationViewFactory())
    SetCustomNotificationViewFactory();
}

ArcNotificationManager::~ArcNotificationManager() {
  instance_owner_->holder()->RemoveObserver(this);
  instance_owner_->holder()->SetHost(nullptr);

  // Ensures that any callback tied to |instance_owner_| is not invoked.
  instance_owner_.reset();
}

void ArcNotificationManager::SetInstance(NotificationsInstancePtr instance) {
  instance_owner_->SetInstancePtr(std::move(instance));
}

ConnectionHolder<NotificationsInstance, NotificationsHost>*
ArcNotificationManager::GetConnectionHolderForTest() {
  return instance_owner_->holder();
}

void ArcNotificationManager::OnConnectionReady() {
  DCHECK(!ready_);
  // TODO(hidehiko): Replace this by ConnectionHolder::IsConnected().
  ready_ = true;
}

void ArcNotificationManager::OnConnectionClosed() {
  DCHECK(ready_);
  while (!items_.empty()) {
    auto it = items_.begin();
    std::unique_ptr<ArcNotificationItem> item = std::move(it->second);
    items_.erase(it);
    item->OnClosedFromAndroid();
  }
  ready_ = false;
}

void ArcNotificationManager::OnNotificationPosted(ArcNotificationDataPtr data) {
  if (ShouldIgnoreNotification(data.get())) {
    VLOG(3) << "Posted notification was ignored.";
    return;
  }

  const std::string& key = data->key;
  auto it = items_.find(key);
  if (it == items_.end()) {
    // Show a notification on the primary logged-in user's desktop and badge the
    // app icon in the shelf if the icon exists.
    // TODO(yoshiki): Reconsider when ARC supports multi-user.
    auto item = std::make_unique<ArcNotificationItemImpl>(
        this, message_center_, key, main_profile_id_);
    // TODO(yoshiki): Use emplacement for performance when it's available.
    auto result = items_.insert(std::make_pair(key, std::move(item)));
    DCHECK(result.second);
    it = result.first;
  }

  delegate_->GetAppIdByPackageName(
      data->package_name.value_or(std::string()),
      base::BindOnce(&ArcNotificationManager::OnGotAppId,
                     weak_ptr_factory_.GetWeakPtr(), std::move(data)));
}

void ArcNotificationManager::OnNotificationUpdated(
    ArcNotificationDataPtr data) {
  if (ShouldIgnoreNotification(data.get())) {
    VLOG(3) << "Updated notification was ignored.";
    return;
  }

  const std::string& key = data->key;
  auto it = items_.find(key);
  if (it == items_.end())
    return;

  delegate_->GetAppIdByPackageName(
      data->package_name.value_or(std::string()),
      base::BindOnce(&ArcNotificationManager::OnGotAppId,
                     weak_ptr_factory_.GetWeakPtr(), std::move(data)));
}

void ArcNotificationManager::OpenMessageCenter() {
  delegate_->ShowMessageCenter();
}

void ArcNotificationManager::OnNotificationRemoved(const std::string& key) {
  auto it = items_.find(key);
  if (it == items_.end()) {
    VLOG(3) << "Android requests to remove a notification (key: " << key
            << "), but it is already gone.";
    return;
  }

  std::unique_ptr<ArcNotificationItem> item = std::move(it->second);
  items_.erase(it);
  item->OnClosedFromAndroid();
}

void ArcNotificationManager::SendNotificationRemovedFromChrome(
    const std::string& key) {
  auto it = items_.find(key);
  if (it == items_.end()) {
    VLOG(3) << "Chrome requests to remove a notification (key: " << key
            << "), but it is already gone.";
    return;
  }

  // The removed ArcNotificationItem needs to live in this scope, since the
  // given argument |key| may be a part of the removed item.
  std::unique_ptr<ArcNotificationItem> item = std::move(it->second);
  items_.erase(it);

  auto* notifications_instance = ARC_GET_INSTANCE_FOR_METHOD(
      instance_owner_->holder(), SendNotificationEventToAndroid);

  // On shutdown, the ARC channel may quit earlier than notifications.
  if (!notifications_instance) {
    VLOG(2) << "ARC Notification (key: " << key
            << ") is closed, but the ARC channel has already gone.";
    return;
  }

  notifications_instance->SendNotificationEventToAndroid(
      key, ArcNotificationEvent::CLOSED);
}

void ArcNotificationManager::SendNotificationClickedOnChrome(
    const std::string& key) {
  if (items_.find(key) == items_.end()) {
    VLOG(3) << "Chrome requests to fire a click event on notification (key: "
            << key << "), but it is gone.";
    return;
  }

  auto* notifications_instance = ARC_GET_INSTANCE_FOR_METHOD(
      instance_owner_->holder(), SendNotificationEventToAndroid);

  // On shutdown, the ARC channel may quit earlier than notifications.
  if (!notifications_instance) {
    VLOG(2) << "ARC Notification (key: " << key
            << ") is clicked, but the ARC channel has already gone.";
    return;
  }

  notifications_instance->SendNotificationEventToAndroid(
      key, ArcNotificationEvent::BODY_CLICKED);
}

void ArcNotificationManager::SendNotificationButtonClickedOnChrome(
    const std::string& key,
    int button_index) {
  if (items_.find(key) == items_.end()) {
    VLOG(3) << "Chrome requests to fire a click event on notification (key: "
            << key << "), but it is gone.";
    return;
  }

  auto* notifications_instance = ARC_GET_INSTANCE_FOR_METHOD(
      instance_owner_->holder(), SendNotificationEventToAndroid);

  // On shutdown, the ARC channel may quit earlier than notifications.
  if (!notifications_instance) {
    VLOG(2) << "ARC Notification (key: " << key
            << ")'s button is clicked, but the ARC channel has already gone.";
    return;
  }

  ArcNotificationEvent command;
  switch (button_index) {
    case 0:
      command = ArcNotificationEvent::BUTTON_1_CLICKED;
      break;
    case 1:
      command = ArcNotificationEvent::BUTTON_2_CLICKED;
      break;
    case 2:
      command = ArcNotificationEvent::BUTTON_3_CLICKED;
      break;
    case 3:
      command = ArcNotificationEvent::BUTTON_4_CLICKED;
      break;
    case 4:
      command = ArcNotificationEvent::BUTTON_5_CLICKED;
      break;
    default:
      VLOG(3) << "Invalid button index (key: " << key
              << ", index: " << button_index << ").";
      return;
  }

  notifications_instance->SendNotificationEventToAndroid(key, command);
}

void ArcNotificationManager::CreateNotificationWindow(const std::string& key) {
  if (items_.find(key) == items_.end()) {
    VLOG(3) << "Chrome requests to create window on notification (key: " << key
            << "), but it is gone.";
    return;
  }

  auto* notifications_instance = ARC_GET_INSTANCE_FOR_METHOD(
      instance_owner_->holder(), CreateNotificationWindow);
  if (!notifications_instance)
    return;

  notifications_instance->CreateNotificationWindow(key);
}

void ArcNotificationManager::CloseNotificationWindow(const std::string& key) {
  if (items_.find(key) == items_.end()) {
    VLOG(3) << "Chrome requests to close window on notification (key: " << key
            << "), but it is gone.";
    return;
  }

  auto* notifications_instance = ARC_GET_INSTANCE_FOR_METHOD(
      instance_owner_->holder(), CloseNotificationWindow);
  if (!notifications_instance)
    return;

  notifications_instance->CloseNotificationWindow(key);
}

void ArcNotificationManager::OpenNotificationSettings(const std::string& key) {
  if (items_.find(key) == items_.end()) {
    DVLOG(3) << "Chrome requests to fire a click event on the notification "
             << "settings button (key: " << key << "), but it is gone.";
    return;
  }

  auto* notifications_instance = ARC_GET_INSTANCE_FOR_METHOD(
      instance_owner_->holder(), OpenNotificationSettings);

  // On shutdown, the ARC channel may quit earlier than notifications.
  if (!notifications_instance)
    return;

  notifications_instance->OpenNotificationSettings(key);
}

bool ArcNotificationManager::IsOpeningSettingsSupported() const {
  const auto* notifications_instance = ARC_GET_INSTANCE_FOR_METHOD(
      instance_owner_->holder(), OpenNotificationSettings);
  return notifications_instance != nullptr;
}

void ArcNotificationManager::SendNotificationToggleExpansionOnChrome(
    const std::string& key) {
  if (items_.find(key) == items_.end()) {
    VLOG(3) << "Chrome requests to fire a click event on notification (key: "
            << key << "), but it is gone.";
    return;
  }

  auto* notifications_instance = ARC_GET_INSTANCE_FOR_METHOD(
      instance_owner_->holder(), SendNotificationEventToAndroid);

  // On shutdown, the ARC channel may quit earlier than notifications.
  if (!notifications_instance) {
    VLOG(2) << "ARC Notification (key: " << key
            << ") is clicked, but the ARC channel has already gone.";
    return;
  }

  notifications_instance->SendNotificationEventToAndroid(
      key, ArcNotificationEvent::TOGGLE_EXPANSION);
}

bool ArcNotificationManager::ShouldIgnoreNotification(
    ArcNotificationData* data) {
  if (data->priority == ArcNotificationPriority::NONE)
    return true;

  // Notifications from Play Store are ignored in Public Session and Kiosk mode.
  // TODO: Use centralized const for Play Store package.
  if (data->package_name.has_value() &&
      *data->package_name == kPlayStorePackageName &&
      delegate_->IsPublicSessionOrKiosk()) {
    return true;
  }

  return false;
}

void ArcNotificationManager::OnGotAppId(ArcNotificationDataPtr data,
                                        const std::string& app_id) {
  const std::string& key = data->key;
  auto it = items_.find(key);
  if (it == items_.end())
    return;

  it->second->OnUpdatedFromAndroid(std::move(data), app_id);
}

}  // namespace ash
