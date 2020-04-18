// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_PUBLIC_BROWSER_PUSH_MESSAGING_SERVICE_H_
#define CONTENT_PUBLIC_BROWSER_PUSH_MESSAGING_SERVICE_H_

#include <stdint.h>
#include <string>
#include <vector>

#include "base/callback_forward.h"
#include "content/common/content_export.h"
#include "url/gurl.h"

namespace blink {
namespace mojom {
enum class PermissionStatus;
}
}  // namespace blink

namespace content {

namespace mojom {
enum class PushRegistrationStatus;
enum class PushUnregistrationReason;
enum class PushUnregistrationStatus;
}  // namespace mojom

class BrowserContext;
struct PushSubscriptionOptions;

// A push service-agnostic interface that the Push API uses for talking to
// push messaging services like GCM. Must only be used on the UI thread.
class CONTENT_EXPORT PushMessagingService {
 public:
  using RegisterCallback =
      base::Callback<void(const std::string& registration_id,
                          const std::vector<uint8_t>& p256dh,
                          const std::vector<uint8_t>& auth,
                          mojom::PushRegistrationStatus status)>;
  using UnregisterCallback =
      base::Callback<void(mojom::PushUnregistrationStatus)>;
  using SubscriptionInfoCallback =
      base::Callback<void(bool is_valid,
                          const std::vector<uint8_t>& p256dh,
                          const std::vector<uint8_t>& auth)>;
  using StringCallback = base::Callback<void(const std::string& data,
                                             bool success,
                                             bool not_found)>;

  virtual ~PushMessagingService() {}

  // Returns the absolute URL to the endpoint of the push service where messages
  // should be posted to. Should return an endpoint compatible with the Web Push
  // Protocol when |standard_protocol| is true.
  virtual GURL GetEndpoint(bool standard_protocol) const = 0;

  // Subscribe the given |options.sender_info| with the push messaging service
  // in a document context. The frame is known and a permission UI may be
  // displayed to the user. It's safe to call this method multiple times for
  // the same registration information, in which case the existing subscription
  // will be returned by the server.
  virtual void SubscribeFromDocument(const GURL& requesting_origin,
                                     int64_t service_worker_registration_id,
                                     int renderer_id,
                                     int render_frame_id,
                                     const PushSubscriptionOptions& options,
                                     bool user_gesture,
                                     const RegisterCallback& callback) = 0;

  // Subscribe the given |options.sender_info| with the push messaging service.
  // The frame is not known so if permission was not previously granted by the
  // user this request should fail. It's safe to call this method multiple times
  // for the same registration information, in which case the existing
  // subscription will be returned by the server.
  virtual void SubscribeFromWorker(const GURL& requesting_origin,
                                   int64_t service_worker_registration_id,
                                   const PushSubscriptionOptions& options,
                                   const RegisterCallback& callback) = 0;

  // Retrieves the subscription associated with |origin| and
  // |service_worker_registration_id|, validates that the provided
  // |subscription_id| matches the stored one, then passes the encryption
  // information to the callback. |sender_id| is also required since an
  // InstanceID might have multiple tokens associated with different senders,
  // though in practice Push doesn't yet use that.
  virtual void GetSubscriptionInfo(
      const GURL& origin,
      int64_t service_worker_registration_id,
      const std::string& sender_id,
      const std::string& subscription_id,
      const SubscriptionInfoCallback& callback) = 0;

  // Unsubscribe the given |sender_id| from the push messaging service. Locally
  // deactivates the subscription, then runs |callback|, then asynchronously
  // attempts to unsubscribe with the push service.
  virtual void Unsubscribe(mojom::PushUnregistrationReason reason,
                           const GURL& requesting_origin,
                           int64_t service_worker_registration_id,
                           const std::string& sender_id,
                           const UnregisterCallback& callback) = 0;

  // Returns whether subscriptions that do not mandate user visible UI upon
  // receiving a push message are supported. Influences permission request and
  // permission check behaviour.
  virtual bool SupportNonVisibleMessages() = 0;

  // Unsubscribes the push subscription associated with this service worker
  // registration, if such a push subscription exists.
  virtual void DidDeleteServiceWorkerRegistration(
      const GURL& origin,
      int64_t service_worker_registration_id) = 0;

  // Unsubscribes all existing push subscriptions because the Service Worker
  // database has been deleted.
  virtual void DidDeleteServiceWorkerDatabase() = 0;

 protected:
  static void GetSenderId(BrowserContext* browser_context,
                          const GURL& origin,
                          int64_t service_worker_registration_id,
                          const StringCallback& callback);

  // Clear the push subscription id stored in the service worker with the given
  // |service_worker_registration_id| for the given |origin|.
  static void ClearPushSubscriptionId(BrowserContext* browser_context,
                                      const GURL& origin,
                                      int64_t service_worker_registration_id,
                                      const base::Closure& callback);

  // Stores a push subscription in the service worker for the given |origin|.
  // Must only be used by tests.
  static void StorePushSubscriptionForTesting(
      BrowserContext* browser_context,
      const GURL& origin,
      int64_t service_worker_registration_id,
      const std::string& subscription_id,
      const std::string& sender_id,
      const base::Closure& callback);
};

}  // namespace content

#endif  // CONTENT_PUBLIC_BROWSER_PUSH_MESSAGING_SERVICE_H_
