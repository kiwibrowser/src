// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/modules/push_messaging/push_subscription_callbacks.h"

#include <memory>

#include "base/memory/ptr_util.h"
#include "third_party/blink/public/platform/modules/push_messaging/web_push_subscription.h"
#include "third_party/blink/renderer/bindings/core/v8/script_promise_resolver.h"
#include "third_party/blink/renderer/modules/push_messaging/push_error.h"
#include "third_party/blink/renderer/modules/push_messaging/push_subscription.h"
#include "third_party/blink/renderer/modules/serviceworkers/service_worker_registration.h"
#include "third_party/blink/renderer/platform/wtf/assertions.h"

namespace blink {

PushSubscriptionCallbacks::PushSubscriptionCallbacks(
    ScriptPromiseResolver* resolver,
    ServiceWorkerRegistration* service_worker_registration)
    : resolver_(resolver),
      service_worker_registration_(service_worker_registration) {
  DCHECK(resolver_);
  DCHECK(service_worker_registration_);
}

PushSubscriptionCallbacks::~PushSubscriptionCallbacks() = default;

void PushSubscriptionCallbacks::OnSuccess(
    std::unique_ptr<WebPushSubscription> web_push_subscription) {
  if (!resolver_->GetExecutionContext() ||
      resolver_->GetExecutionContext()->IsContextDestroyed())
    return;

  resolver_->Resolve(PushSubscription::Take(
      resolver_.Get(), base::WrapUnique(web_push_subscription.release()),
      service_worker_registration_));
}

void PushSubscriptionCallbacks::OnError(const WebPushError& error) {
  if (!resolver_->GetExecutionContext() ||
      resolver_->GetExecutionContext()->IsContextDestroyed())
    return;
  resolver_->Reject(PushError::Take(resolver_.Get(), error));
}

}  // namespace blink
