// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_MODULES_PUSH_MESSAGING_PUSH_SUBSCRIPTION_H_
#define THIRD_PARTY_BLINK_RENDERER_MODULES_PUSH_MESSAGING_PUSH_SUBSCRIPTION_H_

#include <memory>
#include "base/gtest_prod_util.h"
#include "base/memory/scoped_refptr.h"
#include "third_party/blink/renderer/bindings/core/v8/script_promise.h"
#include "third_party/blink/renderer/bindings/core/v8/script_value.h"
#include "third_party/blink/renderer/core/dom/dom_time_stamp.h"
#include "third_party/blink/renderer/core/typed_arrays/dom_array_buffer.h"
#include "third_party/blink/renderer/modules/modules_export.h"
#include "third_party/blink/renderer/platform/bindings/script_wrappable.h"
#include "third_party/blink/renderer/platform/heap/handle.h"
#include "third_party/blink/renderer/platform/weborigin/kurl.h"

namespace blink {

class PushSubscriptionOptions;
class ServiceWorkerRegistration;
class ScriptPromiseResolver;
class ScriptState;
struct WebPushSubscription;

class MODULES_EXPORT PushSubscription final : public ScriptWrappable {
  DEFINE_WRAPPERTYPEINFO();

 public:
  static PushSubscription* Take(
      ScriptPromiseResolver* resolver,
      std::unique_ptr<WebPushSubscription> push_subscription,
      ServiceWorkerRegistration* service_worker_registration);
  static void Dispose(WebPushSubscription* subscription_raw);

  ~PushSubscription() override;

  KURL endpoint() const { return endpoint_; }
  DOMTimeStamp expirationTime(bool& out_is_null) const;

  PushSubscriptionOptions* options() const { return options_.Get(); }

  DOMArrayBuffer* getKey(const AtomicString& name) const;
  ScriptPromise unsubscribe(ScriptState* script_state);

  ScriptValue toJSONForBinding(ScriptState* script_state);

  void Trace(blink::Visitor* visitor) override;

 private:
  FRIEND_TEST_ALL_PREFIXES(PushSubscriptionTest,
                           SerializesToBase64URLWithoutPadding);

  PushSubscription(const WebPushSubscription& subscription,
                   ServiceWorkerRegistration* service_worker_registration);

  KURL endpoint_;

  Member<PushSubscriptionOptions> options_;

  Member<DOMArrayBuffer> p256dh_;
  Member<DOMArrayBuffer> auth_;

  Member<ServiceWorkerRegistration> service_worker_registration_;
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_MODULES_PUSH_MESSAGING_PUSH_SUBSCRIPTION_H_
