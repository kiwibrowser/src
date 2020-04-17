// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_NOTIFICATIONS_BLINK_NOTIFICATION_SERVICE_IMPL_H_
#define CONTENT_BROWSER_NOTIFICATIONS_BLINK_NOTIFICATION_SERVICE_IMPL_H_

#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "content/browser/service_worker/service_worker_context_wrapper.h"
#include "content/common/content_export.h"
#include "content/public/browser/browser_context.h"
#include "mojo/public/cpp/bindings/binding.h"
#include "mojo/public/cpp/bindings/interface_request.h"
#include "third_party/blink/public/platform/modules/notifications/notification_service.mojom.h"
#include "third_party/blink/public/platform/modules/permissions/permission_status.mojom.h"
#include "url/origin.h"

namespace content {

struct NotificationDatabaseData;
class PlatformNotificationContextImpl;
struct PlatformNotificationData;

// Implementation of the NotificationService used for Web Notifications. Is
// responsible for displaying, updating and reading of both non-persistent
// and persistent notifications. Primarily lives on the UI thread, but jumps to
// the IO thread when needing to interact with the PlatformNotificationContext.
class CONTENT_EXPORT BlinkNotificationServiceImpl
    : public blink::mojom::NotificationService {
 public:
  BlinkNotificationServiceImpl(
      PlatformNotificationContextImpl* notification_context,
      BrowserContext* browser_context,
      scoped_refptr<ServiceWorkerContextWrapper> service_worker_context,
      const url::Origin& origin,
      mojo::InterfaceRequest<blink::mojom::NotificationService> request);
  ~BlinkNotificationServiceImpl() override;

  // blink::mojom::NotificationService implementation.
  void GetPermissionStatus(GetPermissionStatusCallback callback) override;
  void DisplayNonPersistentNotification(
      const std::string& token,
      const PlatformNotificationData& platform_notification_data,
      const NotificationResources& notification_resources,
      blink::mojom::NonPersistentNotificationListenerPtr listener_ptr) override;
  void CloseNonPersistentNotification(const std::string& token) override;
  void DisplayPersistentNotification(
      int64_t service_worker_registration_id,
      const PlatformNotificationData& platform_notification_data,
      const NotificationResources& notification_resources,
      DisplayPersistentNotificationCallback) override;
  void ClosePersistentNotification(const std::string& notification_id) override;
  void GetNotifications(int64_t service_worker_registration_id,
                        const std::string& filter_tag,
                        GetNotificationsCallback callback) override;

 private:
  // Called when an error is detected on binding_.
  void OnConnectionError();

  // Check the permission status for the current |origin_|.
  blink::mojom::PermissionStatus CheckPermissionStatus();

  void DisplayPersistentNotificationOnIOThread(
      int64_t service_worker_registration_id,
      const PlatformNotificationData& platform_notification_data,
      const NotificationResources& notification_resources,
      DisplayPersistentNotificationCallback callback);

  void DisplayPersistentNotificationWithIdOnIOThread(
      int64_t service_worker_registration_id,
      const PlatformNotificationData& platform_notification_data,
      const NotificationResources& notification_resources,
      DisplayPersistentNotificationCallback callback,
      bool success,
      const std::string& notification_id);

  void DisplayPersistentNotificationWithServiceWorkerOnIOThread(
      const std::string& notification_id,
      const PlatformNotificationData& platform_notification_data,
      const NotificationResources& notification_resources,
      DisplayPersistentNotificationCallback callback,
      ServiceWorkerStatusCode service_worker_status,
      scoped_refptr<ServiceWorkerRegistration> registration);

  void DidGetNotificationsOnIOThread(
      const std::string& filter_tag,
      GetNotificationsCallback callback,
      bool success,
      const std::vector<NotificationDatabaseData>& notifications);

  // The notification context that owns this service instance.
  PlatformNotificationContextImpl* notification_context_;

  BrowserContext* browser_context_;

  scoped_refptr<ServiceWorkerContextWrapper> service_worker_context_;

  // The origin that this notification service is communicating with.
  url::Origin origin_;

  mojo::Binding<blink::mojom::NotificationService> binding_;

  base::WeakPtrFactory<BlinkNotificationServiceImpl> weak_factory_for_io_{this};
  base::WeakPtrFactory<BlinkNotificationServiceImpl> weak_factory_for_ui_{this};

  DISALLOW_COPY_AND_ASSIGN(BlinkNotificationServiceImpl);
};

}  // namespace content

#endif  // CONTENT_BROWSER_NOTIFICATIONS_BLINK_NOTIFICATION_SERVICE_IMPL_H_
