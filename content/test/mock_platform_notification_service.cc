// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/test/mock_platform_notification_service.h"

#include "base/guid.h"
#include "base/strings/nullable_string16.h"
#include "base/strings/utf_string_conversions.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/notification_event_dispatcher.h"
#include "content/public/browser/permission_type.h"
#include "content/public/common/persistent_notification_status.h"
#include "content/public/common/platform_notification_data.h"

namespace content {
namespace {

// The Web Notification layout tests don't care about the lifetime of the
// Service Worker when a notificationclick event has been dispatched.
void OnEventDispatchComplete(PersistentNotificationStatus status) {}

}  // namespace

MockPlatformNotificationService::MockPlatformNotificationService() = default;

MockPlatformNotificationService::~MockPlatformNotificationService() = default;

void MockPlatformNotificationService::DisplayNotification(
    BrowserContext* browser_context,
    const std::string& notification_id,
    const GURL& origin,
    const PlatformNotificationData& notification_data,
    const NotificationResources& notification_resources) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);

  ReplaceNotificationIfNeeded(notification_id);
  non_persistent_notifications_.insert(notification_id);

  NotificationEventDispatcher::GetInstance()->DispatchNonPersistentShowEvent(
      notification_id);
  notification_id_map_[base::UTF16ToUTF8(notification_data.title)] =
      notification_id;
}

void MockPlatformNotificationService::DisplayPersistentNotification(
    BrowserContext* browser_context,
    const std::string& notification_id,
    const GURL& service_worker_scope,
    const GURL& origin,
    const PlatformNotificationData& notification_data,
    const NotificationResources& notification_resources) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);

  ReplaceNotificationIfNeeded(notification_id);

  PersistentNotification notification;
  notification.browser_context = browser_context;
  notification.origin = origin;

  persistent_notifications_[notification_id] = notification;

  notification_id_map_[base::UTF16ToUTF8(notification_data.title)] =
      notification_id;
}

void MockPlatformNotificationService::CloseNotification(
    BrowserContext* browser_context,
    const std::string& notification_id) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);

  const auto non_persistent_iter =
      non_persistent_notifications_.find(notification_id);
  if (non_persistent_iter == non_persistent_notifications_.end())
    return;

  non_persistent_notifications_.erase(non_persistent_iter);
}

void MockPlatformNotificationService::ClosePersistentNotification(
    BrowserContext* browser_context,
    const std::string& notification_id) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);

  persistent_notifications_.erase(notification_id);
}

void MockPlatformNotificationService::GetDisplayedNotifications(
    BrowserContext* browser_context,
    const DisplayedNotificationsCallback& callback) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  auto displayed_notifications = std::make_unique<std::set<std::string>>();

  for (const auto& kv : persistent_notifications_)
    displayed_notifications->insert(kv.first);
  for (const auto& notification_id : non_persistent_notifications_)
    displayed_notifications->insert(notification_id);

  BrowserThread::PostTask(
      BrowserThread::UI, FROM_HERE,
      base::BindOnce(callback, std::move(displayed_notifications),
                     true /* supports_synchronization */));
}

void MockPlatformNotificationService::SimulateClick(
    const std::string& title,
    const base::Optional<int>& action_index,
    const base::Optional<base::string16>& reply) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  const auto notification_id_iter = notification_id_map_.find(title);
  if (notification_id_iter == notification_id_map_.end())
    return;

  const std::string& notification_id = notification_id_iter->second;

  const auto persistent_iter = persistent_notifications_.find(notification_id);
  const auto non_persistent_iter =
      non_persistent_notifications_.find(notification_id);

  if (persistent_iter != persistent_notifications_.end()) {
    DCHECK(non_persistent_iter == non_persistent_notifications_.end());

    const PersistentNotification& notification = persistent_iter->second;
    NotificationEventDispatcher::GetInstance()->DispatchNotificationClickEvent(
        notification.browser_context, notification_id, notification.origin,
        action_index, reply, base::BindOnce(&OnEventDispatchComplete));
  } else if (non_persistent_iter != non_persistent_notifications_.end()) {
    DCHECK(!action_index.has_value())
        << "Action buttons are only supported for "
           "persistent notifications";
    NotificationEventDispatcher::GetInstance()->DispatchNonPersistentClickEvent(
        notification_id);
  }
}

void MockPlatformNotificationService::SimulateClose(const std::string& title,
                                                    bool by_user) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);

  const auto notification_id_iter = notification_id_map_.find(title);
  if (notification_id_iter == notification_id_map_.end())
    return;

  const std::string& notification_id = notification_id_iter->second;

  const auto& persistent_iter = persistent_notifications_.find(notification_id);
  if (persistent_iter == persistent_notifications_.end())
    return;

  const PersistentNotification& notification = persistent_iter->second;
  NotificationEventDispatcher::GetInstance()->DispatchNotificationCloseEvent(
      notification.browser_context, notification_id, notification.origin,
      by_user, base::BindOnce(&OnEventDispatchComplete));
}

void MockPlatformNotificationService::SetPermission(
    blink::mojom::PermissionStatus permission_status) {
  permission_status_ = permission_status;
}

blink::mojom::PermissionStatus
MockPlatformNotificationService::CheckPermissionOnUIThread(
    BrowserContext* browser_context,
    const GURL& origin,
    int render_process_id) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  return CheckPermission(origin);
}

blink::mojom::PermissionStatus
MockPlatformNotificationService::CheckPermissionOnIOThread(
    ResourceContext* resource_context,
    const GURL& origin,
    int render_process_id) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  return CheckPermission(origin);
}

void MockPlatformNotificationService::ReplaceNotificationIfNeeded(
    const std::string& notification_id) {
  persistent_notifications_.erase(notification_id);
  non_persistent_notifications_.erase(notification_id);
}

blink::mojom::PermissionStatus MockPlatformNotificationService::CheckPermission(
    const GURL& origin) {
  return permission_status_;
}

}  // namespace content
