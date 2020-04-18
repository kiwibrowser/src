// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_PUSH_MESSAGING_PUSH_MESSAGING_SERVICE_IMPL_H_
#define CHROME_BROWSER_PUSH_MESSAGING_PUSH_MESSAGING_SERVICE_IMPL_H_

#include <stdint.h>
#include <memory>
#include <set>
#include <vector>

#include "base/callback.h"
#include "base/compiler_specific.h"
#include "base/gtest_prod_util.h"
#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "chrome/browser/push_messaging/push_messaging_notification_manager.h"
#include "chrome/common/buildflags.h"
#include "components/content_settings/core/browser/content_settings_observer.h"
#include "components/content_settings/core/common/content_settings.h"
#include "components/content_settings/core/common/content_settings_types.h"
#include "components/gcm_driver/common/gcm_messages.h"
#include "components/gcm_driver/crypto/gcm_encryption_provider.h"
#include "components/gcm_driver/gcm_app_handler.h"
#include "components/gcm_driver/gcm_client.h"
#include "components/gcm_driver/instance_id/instance_id.h"
#include "components/keyed_service/core/keyed_service.h"
#include "content/public/browser/notification_observer.h"
#include "content/public/browser/notification_registrar.h"
#include "content/public/browser/push_messaging_service.h"
#include "content/public/common/push_event_payload.h"

class Profile;
class PushMessagingAppIdentifier;
class PushMessagingServiceTest;
class ScopedKeepAlive;
struct PushSubscriptionOptions;

namespace content {
namespace mojom {
enum class PushDeliveryStatus;
enum class PushRegistrationStatus;
}  // namespace mojom
}  // namespace content

namespace gcm {
class GCMDriver;
}
namespace instance_id {
class InstanceIDDriver;
}

class PushMessagingServiceImpl : public content::PushMessagingService,
                                 public gcm::GCMAppHandler,
                                 public content_settings::Observer,
                                 public KeyedService,
                                 public content::NotificationObserver {
 public:
  // If any Service Workers are using push, starts GCM and adds an app handler.
  static void InitializeForProfile(Profile* profile);

  explicit PushMessagingServiceImpl(Profile* profile);
  ~PushMessagingServiceImpl() override;

  // Gets the permission status for the given |origin|.
  blink::mojom::PermissionStatus GetPermissionStatus(const GURL& origin,
                                                     bool user_visible);

  // gcm::GCMAppHandler implementation.
  void ShutdownHandler() override;
  void OnStoreReset() override;
  void OnMessage(const std::string& app_id,
                 const gcm::IncomingMessage& message) override;
  void OnMessagesDeleted(const std::string& app_id) override;
  void OnSendError(
      const std::string& app_id,
      const gcm::GCMClient::SendErrorDetails& send_error_details) override;
  void OnSendAcknowledged(const std::string& app_id,
                          const std::string& message_id) override;
  bool CanHandle(const std::string& app_id) const override;

  // content::PushMessagingService implementation:
  GURL GetEndpoint(bool standard_protocol) const override;
  void SubscribeFromDocument(const GURL& requesting_origin,
                             int64_t service_worker_registration_id,
                             int renderer_id,
                             int render_frame_id,
                             const content::PushSubscriptionOptions& options,
                             bool user_gesture,
                             const RegisterCallback& callback) override;
  void SubscribeFromWorker(const GURL& requesting_origin,
                           int64_t service_worker_registration_id,
                           const content::PushSubscriptionOptions& options,
                           const RegisterCallback& callback) override;
  void GetSubscriptionInfo(const GURL& origin,
                           int64_t service_worker_registration_id,
                           const std::string& sender_id,
                           const std::string& subscription_id,
                           const SubscriptionInfoCallback& callback) override;
  void Unsubscribe(content::mojom::PushUnregistrationReason reason,
                   const GURL& requesting_origin,
                   int64_t service_worker_registration_id,
                   const std::string& sender_id,
                   const UnregisterCallback&) override;
  bool SupportNonVisibleMessages() override;
  void DidDeleteServiceWorkerRegistration(
      const GURL& origin,
      int64_t service_worker_registration_id) override;
  void DidDeleteServiceWorkerDatabase() override;

  // content_settings::Observer implementation.
  void OnContentSettingChanged(const ContentSettingsPattern& primary_pattern,
                               const ContentSettingsPattern& secondary_pattern,
                               ContentSettingsType content_type,
                               std::string resource_identifier) override;

  // KeyedService implementation.
  void Shutdown() override;

  // content::NotificationObserver:
  void Observe(int type,
               const content::NotificationSource& source,
               const content::NotificationDetails& details) override;

  void SetMessageCallbackForTesting(const base::Closure& callback);
  void SetUnsubscribeCallbackForTesting(const base::Closure& callback);
  void SetContentSettingChangedCallbackForTesting(
      const base::Closure& callback);
  void SetServiceWorkerUnregisteredCallbackForTesting(
      const base::Closure& callback);
  void SetServiceWorkerDatabaseWipedCallbackForTesting(
      const base::Closure& callback);

 private:
  friend class PushMessagingBrowserTest;
  FRIEND_TEST_ALL_PREFIXES(PushMessagingServiceTest, NormalizeSenderInfo);
  FRIEND_TEST_ALL_PREFIXES(PushMessagingServiceTest, PayloadEncryptionTest);

  // A subscription is pending until it has succeeded or failed.
  void IncreasePushSubscriptionCount(int add, bool is_pending);
  void DecreasePushSubscriptionCount(int subtract, bool was_pending);

  // OnMessage methods ---------------------------------------------------------

  void DeliverMessageCallback(const std::string& app_id,
                              const GURL& requesting_origin,
                              int64_t service_worker_registration_id,
                              const gcm::IncomingMessage& message,
                              const base::Closure& message_handled_closure,
                              content::mojom::PushDeliveryStatus status);

  void DidHandleMessage(const std::string& app_id,
                        const base::Closure& completion_closure);

  // Subscribe methods ---------------------------------------------------------

  void DoSubscribe(const PushMessagingAppIdentifier& app_identifier,
                   const content::PushSubscriptionOptions& options,
                   const RegisterCallback& callback,
                   ContentSetting permission_status);

  void SubscribeEnd(const RegisterCallback& callback,
                    const std::string& subscription_id,
                    const std::vector<uint8_t>& p256dh,
                    const std::vector<uint8_t>& auth,
                    content::mojom::PushRegistrationStatus status);

  void SubscribeEndWithError(const RegisterCallback& callback,
                             content::mojom::PushRegistrationStatus status);

  void DidSubscribe(const PushMessagingAppIdentifier& app_identifier,
                    const std::string& sender_id,
                    const RegisterCallback& callback,
                    const std::string& subscription_id,
                    instance_id::InstanceID::Result result);

  void DidSubscribeWithEncryptionInfo(
      const PushMessagingAppIdentifier& app_identifier,
      const RegisterCallback& callback,
      const std::string& subscription_id,
      const std::string& p256dh,
      const std::string& auth_secret);

  // GetSubscriptionInfo methods -----------------------------------------------

  void DidValidateSubscription(const std::string& app_id,
                               const std::string& sender_id,
                               const SubscriptionInfoCallback& callback,
                               bool is_valid);

  void DidGetEncryptionInfo(const SubscriptionInfoCallback& callback,
                            const std::string& p256dh,
                            const std::string& auth_secret) const;

  // Unsubscribe methods -------------------------------------------------------

  // |origin|, |service_worker_registration_id| and |app_id| should be provided
  // whenever they can be obtained. It's valid for |origin| to be empty and
  // |service_worker_registration_id| to be kInvalidServiceWorkerRegistrationId,
  // or for app_id to be empty, but not both at once.
  void UnsubscribeInternal(content::mojom::PushUnregistrationReason reason,
                           const GURL& origin,
                           int64_t service_worker_registration_id,
                           const std::string& app_id,
                           const std::string& sender_id,
                           const UnregisterCallback& callback);

  void DidClearPushSubscriptionId(
      content::mojom::PushUnregistrationReason reason,
      const std::string& app_id,
      const std::string& sender_id,
      const UnregisterCallback& callback);

  void DidUnregister(bool was_subscribed, gcm::GCMClient::Result result);
  void DidDeleteID(const std::string& app_id,
                   bool was_subscribed,
                   instance_id::InstanceID::Result result);
  void DidUnsubscribe(const std::string& app_id_when_instance_id,
                      bool was_subscribed);

  // OnContentSettingChanged methods -------------------------------------------

  void UnsubscribeBecausePermissionRevoked(
      const PushMessagingAppIdentifier& app_identifier,
      const UnregisterCallback& callback,
      const std::string& sender_id,
      bool success,
      bool not_found);

  // Helper methods ------------------------------------------------------------

  // Normalizes the |sender_info|. In most cases the |sender_info| will be
  // passed through to the GCM Driver as-is, but NIST P-256 application server
  // keys have to be encoded using the URL-safe variant of the base64 encoding.
  std::string NormalizeSenderInfo(const std::string& sender_info) const;

  // Checks if a given origin is allowed to use Push.
  bool IsPermissionSet(const GURL& origin);

  // Wrapper around {GCMDriver, InstanceID}::GetEncryptionInfo.
  void GetEncryptionInfoForAppId(
      const std::string& app_id,
      const std::string& sender_id,
      gcm::GCMEncryptionProvider::EncryptionInfoCallback callback);

  gcm::GCMDriver* GetGCMDriver() const;

  instance_id::InstanceIDDriver* GetInstanceIDDriver() const;

  // Testing methods -----------------------------------------------------------

  // Callback to be invoked when a message has been dispatched. Enables tests to
  // observe message delivery before it's dispatched to the Service Worker.
  using MessageDispatchedCallback =
      base::Callback<void(const std::string& app_id,
                          const GURL& origin,
                          int64_t service_worker_registration_id,
                          const content::PushEventPayload& payload)>;

  void SetMessageDispatchedCallbackForTesting(
      const MessageDispatchedCallback& callback) {
    message_dispatched_callback_for_testing_ = callback;
  }

  Profile* profile_;

  int push_subscription_count_;
  int pending_push_subscription_count_;

  base::Closure message_callback_for_testing_;
  base::Closure unsubscribe_callback_for_testing_;
  base::Closure content_setting_changed_callback_for_testing_;
  base::Closure service_worker_unregistered_callback_for_testing_;
  base::Closure service_worker_database_wiped_callback_for_testing_;

  PushMessagingNotificationManager notification_manager_;

  // A multiset containing one entry for each in-flight push message delivery,
  // keyed by the receiver's app id.
  std::multiset<std::string> in_flight_message_deliveries_;

  MessageDispatchedCallback message_dispatched_callback_for_testing_;

#if BUILDFLAG(ENABLE_BACKGROUND_MODE)
  // KeepAlive registered while we have in-flight push messages, to make sure
  // we can finish processing them without being interrupted.
  std::unique_ptr<ScopedKeepAlive> in_flight_keep_alive_;
#endif

  content::NotificationRegistrar registrar_;

  // True when shutdown has started. Do not allow processing of incoming
  // messages when this is true.
  bool shutdown_started_ = false;

  base::WeakPtrFactory<PushMessagingServiceImpl> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(PushMessagingServiceImpl);
};

#endif  // CHROME_BROWSER_PUSH_MESSAGING_PUSH_MESSAGING_SERVICE_IMPL_H_
