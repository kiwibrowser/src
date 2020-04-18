// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_MODULES_PUSH_MESSAGING_PUSH_SUBSCRIPTION_CALLBACKS_H_
#define THIRD_PARTY_BLINK_RENDERER_MODULES_PUSH_MESSAGING_PUSH_SUBSCRIPTION_CALLBACKS_H_

#include "third_party/blink/public/platform/modules/push_messaging/web_push_provider.h"
#include "third_party/blink/renderer/platform/heap/handle.h"
#include "third_party/blink/renderer/platform/wtf/noncopyable.h"

namespace blink {

class ServiceWorkerRegistration;
class ScriptPromiseResolver;
struct WebPushError;
struct WebPushSubscription;

// This class is an implementation of WebPushSubscriptionCallbacks that will
// resolve the underlying promise depending on the result passed to the
// callback. It takes a ServiceWorkerRegistration in its constructor and will
// pass it to the PushSubscription.
class PushSubscriptionCallbacks final : public WebPushSubscriptionCallbacks {
  WTF_MAKE_NONCOPYABLE(PushSubscriptionCallbacks);
  USING_FAST_MALLOC(PushSubscriptionCallbacks);

 public:
  PushSubscriptionCallbacks(
      ScriptPromiseResolver* resolver,
      ServiceWorkerRegistration* service_worker_registration);
  ~PushSubscriptionCallbacks() override;

  // WebPushSubscriptionCallbacks interface.
  void OnSuccess(
      std::unique_ptr<WebPushSubscription> web_push_subscription) override;
  void OnError(const WebPushError& error) override;

 private:
  Persistent<ScriptPromiseResolver> resolver_;
  Persistent<ServiceWorkerRegistration> service_worker_registration_;
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_MODULES_PUSH_MESSAGING_PUSH_SUBSCRIPTION_CALLBACKS_H_
