// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_PUSH_MESSAGING_PUSH_MESSAGING_MANAGER_H_
#define CONTENT_BROWSER_PUSH_MESSAGING_PUSH_MESSAGING_MANAGER_H_

#include <stdint.h>

#include <memory>
#include <string>

#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/memory/weak_ptr.h"
#include "content/common/push_messaging.mojom.h"
#include "content/common/service_worker/service_worker_status_code.h"
#include "content/public/browser/browser_thread.h"
#include "mojo/public/cpp/bindings/binding_set.h"
#include "url/gurl.h"

namespace content {

namespace mojom {
enum class PushRegistrationStatus;
enum class PushUnregistrationStatus;
}  // namespace mojom

class PushMessagingService;
class ServiceWorkerContextWrapper;
struct PushSubscriptionOptions;

// Documented at definition.
extern const char kPushSenderIdServiceWorkerKey[];
extern const char kPushRegistrationIdServiceWorkerKey[];

class PushMessagingManager : public mojom::PushMessaging {
 public:
  PushMessagingManager(int render_process_id,
                       ServiceWorkerContextWrapper* service_worker_context);

  void BindRequest(mojom::PushMessagingRequest request);

  // mojom::PushMessaging impl, run on IO thread.
  void Subscribe(int32_t render_frame_id,
                 int64_t service_worker_registration_id,
                 const PushSubscriptionOptions& options,
                 bool user_gesture,
                 SubscribeCallback callback) override;
  void Unsubscribe(int64_t service_worker_registration_id,
                   UnsubscribeCallback callback) override;
  void GetSubscription(int64_t service_worker_registration_id,
                       GetSubscriptionCallback callback) override;

 private:
  struct RegisterData;
  class Core;

  friend class BrowserThread;
  friend class base::DeleteHelper<PushMessagingManager>;
  friend struct BrowserThread::DeleteOnThread<BrowserThread::IO>;

  ~PushMessagingManager() override;

  void DidCheckForExistingRegistration(
      RegisterData data,
      const std::vector<std::string>& subscription_id_and_sender_id,
      ServiceWorkerStatusCode service_worker_status);

  void DidGetSenderIdFromStorage(RegisterData data,
                                 const std::vector<std::string>& sender_id,
                                 ServiceWorkerStatusCode service_worker_status);

  // Called via PostTask from UI thread.
  void PersistRegistrationOnIO(RegisterData data,
                               const std::string& push_subscription_id,
                               const std::vector<uint8_t>& p256dh,
                               const std::vector<uint8_t>& auth,
                               mojom::PushRegistrationStatus status);

  void DidPersistRegistrationOnIO(
      RegisterData data,
      const std::string& push_subscription_id,
      const std::vector<uint8_t>& p256dh,
      const std::vector<uint8_t>& auth,
      mojom::PushRegistrationStatus push_registration_status,
      ServiceWorkerStatusCode service_worker_status);

  // Called both from IO thread, and via PostTask from UI thread.
  void SendSubscriptionError(RegisterData data,
                             mojom::PushRegistrationStatus status);
  // Called both from IO thread, and via PostTask from UI thread.
  void SendSubscriptionSuccess(RegisterData data,
                               mojom::PushRegistrationStatus status,
                               const std::string& push_subscription_id,
                               const std::vector<uint8_t>& p256dh,
                               const std::vector<uint8_t>& auth);

  void UnsubscribeHavingGottenSenderId(
      UnsubscribeCallback callback,
      int64_t service_worker_registration_id,
      const GURL& requesting_origin,
      const std::vector<std::string>& sender_id,
      ServiceWorkerStatusCode service_worker_status);

  // Called both from IO thread, and via PostTask from UI thread.
  void DidUnregister(UnsubscribeCallback callback,
                     mojom::PushUnregistrationStatus unregistration_status);

  void DidGetSubscription(
      GetSubscriptionCallback callback,
      int64_t service_worker_registration_id,
      const std::vector<std::string>& push_subscription_id_and_sender_info,
      ServiceWorkerStatusCode service_worker_status);

  // Helper methods on either thread -------------------------------------------

  // Creates an endpoint for |subscription_id| with either the default protocol,
  // or the standardized Web Push Protocol, depending on |standard_protocol|.
  GURL CreateEndpoint(bool standard_protocol,
                      const std::string& subscription_id) const;

  // Inner core of this message filter which lives on the UI thread.
  std::unique_ptr<Core, BrowserThread::DeleteOnUIThread> ui_core_;

  // Can be used on the IO thread as the |this| parameter when binding a
  // callback that will be called on the UI thread (an IO -> UI -> UI chain).
  base::WeakPtr<Core> ui_core_weak_ptr_;

  scoped_refptr<ServiceWorkerContextWrapper> service_worker_context_;

  // Whether the PushMessagingService was available when constructed.
  bool service_available_;

  GURL default_endpoint_;
  GURL web_push_protocol_endpoint_;

  mojo::BindingSet<mojom::PushMessaging> bindings_;

  base::WeakPtrFactory<PushMessagingManager> weak_factory_io_to_io_;

  DISALLOW_COPY_AND_ASSIGN(PushMessagingManager);
};

}  // namespace content

#endif  // CONTENT_BROWSER_PUSH_MESSAGING_PUSH_MESSAGING_MANAGER_H_
