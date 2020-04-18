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
#include "url/origin.h"

namespace content {

struct NotificationDatabaseData;
class PlatformNotificationContextImpl;
struct PlatformNotificationData;
class ResourceContext;

// Implementation of the NotificationService used for Web Notifications. Is
// responsible for displaying, updating and reading of both non-persistent
// and persistent notifications. Lives on the IO thread.
class CONTENT_EXPORT BlinkNotificationServiceImpl
    : public blink::mojom::NotificationService {
 public:
  BlinkNotificationServiceImpl(
      PlatformNotificationContextImpl* notification_context,
      BrowserContext* browser_context,
      ResourceContext* resource_context,
      scoped_refptr<ServiceWorkerContextWrapper> service_worker_context,
      int render_process_id,
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

  void DisplayNonPersistentNotificationOnUIThread(
      const std::string& notification_id,
      const GURL& origin,
      const content::PlatformNotificationData& notification_data,
      const content::NotificationResources& notification_resources,
      blink::mojom::NonPersistentNotificationListenerPtrInfo listener_ptr_info);

  void DisplayPersistentNotificationWithId(
      int64_t service_worker_registration_id,
      const PlatformNotificationData& platform_notification_data,
      const NotificationResources& notification_resources,
      DisplayPersistentNotificationCallback callback,
      bool success,
      const std::string& notification_id);

  void DisplayPersistentNotificationWithIdForServiceWorker(
      const std::string& notification_id,
      const PlatformNotificationData& platform_notification_data,
      const NotificationResources& notification_resources,
      DisplayPersistentNotificationCallback callback,
      content::ServiceWorkerStatusCode service_worker_status,
      scoped_refptr<content::ServiceWorkerRegistration> registration);

  void CloseNonPersistentNotificationOnUIThread(
      const std::string& notification_id);

  blink::mojom::PermissionStatus CheckPermissionStatus();

  void DidGetNotifications(
      const std::string& filter_tag,
      GetNotificationsCallback callback,
      bool success,
      const std::vector<NotificationDatabaseData>& notifications);

  // The notification context that owns this service instance.
  PlatformNotificationContextImpl* notification_context_;

  BrowserContext* browser_context_;

  ResourceContext* resource_context_;

  scoped_refptr<ServiceWorkerContextWrapper> service_worker_context_;

  int render_process_id_;

  // The origin that this notification service is communicating with.
  url::Origin origin_;

  mojo::Binding<blink::mojom::NotificationService> binding_;

  base::WeakPtrFactory<BlinkNotificationServiceImpl> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(BlinkNotificationServiceImpl);
};

}  // namespace content

#endif  // CONTENT_BROWSER_NOTIFICATIONS_BLINK_NOTIFICATION_SERVICE_IMPL_H_
