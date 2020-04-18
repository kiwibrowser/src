// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/modules/push_messaging/push_manager.h"

#include <memory>

#include "base/memory/scoped_refptr.h"
#include "third_party/blink/public/platform/modules/push_messaging/web_push_client.h"
#include "third_party/blink/public/platform/modules/push_messaging/web_push_provider.h"
#include "third_party/blink/public/platform/modules/push_messaging/web_push_subscription_options.h"
#include "third_party/blink/public/platform/platform.h"
#include "third_party/blink/renderer/bindings/core/v8/script_promise.h"
#include "third_party/blink/renderer/bindings/core/v8/script_promise_resolver.h"
#include "third_party/blink/renderer/core/dom/document.h"
#include "third_party/blink/renderer/core/dom/dom_exception.h"
#include "third_party/blink/renderer/core/dom/exception_code.h"
#include "third_party/blink/renderer/core/execution_context/execution_context.h"
#include "third_party/blink/renderer/core/frame/frame.h"
#include "third_party/blink/renderer/core/frame/local_frame.h"
#include "third_party/blink/renderer/modules/push_messaging/push_controller.h"
#include "third_party/blink/renderer/modules/push_messaging/push_error.h"
#include "third_party/blink/renderer/modules/push_messaging/push_messaging_bridge.h"
#include "third_party/blink/renderer/modules/push_messaging/push_subscription.h"
#include "third_party/blink/renderer/modules/push_messaging/push_subscription_callbacks.h"
#include "third_party/blink/renderer/modules/push_messaging/push_subscription_options.h"
#include "third_party/blink/renderer/modules/push_messaging/push_subscription_options_init.h"
#include "third_party/blink/renderer/modules/serviceworkers/service_worker_registration.h"
#include "third_party/blink/renderer/platform/bindings/script_state.h"
#include "third_party/blink/renderer/platform/wtf/assertions.h"

namespace blink {
namespace {

WebPushProvider* PushProvider() {
  WebPushProvider* web_push_provider = Platform::Current()->PushProvider();
  DCHECK(web_push_provider);
  return web_push_provider;
}

}  // namespace

PushManager::PushManager(ServiceWorkerRegistration* registration)
    : registration_(registration) {
  DCHECK(registration);
}

// static
Vector<String> PushManager::supportedContentEncodings() {
  return Vector<String>({"aes128gcm", "aesgcm"});
}

ScriptPromise PushManager::subscribe(ScriptState* script_state,
                                     const PushSubscriptionOptionsInit& options,
                                     ExceptionState& exception_state) {
  if (!registration_->active())
    return ScriptPromise::RejectWithDOMException(
        script_state,
        DOMException::Create(kAbortError,
                             "Subscription failed - no active Service Worker"));

  const WebPushSubscriptionOptions& web_options =
      PushSubscriptionOptions::ToWeb(options, exception_state);
  if (exception_state.HadException())
    return ScriptPromise();

  ScriptPromiseResolver* resolver = ScriptPromiseResolver::Create(script_state);
  ScriptPromise promise = resolver->Promise();

  // The document context is the only reasonable context from which to ask the
  // user for permission to use the Push API. The embedder should persist the
  // permission so that later calls in different contexts can succeed.
  if (ExecutionContext::From(script_state)->IsDocument()) {
    Document* document = ToDocument(ExecutionContext::From(script_state));
    LocalFrame* frame = document->GetFrame();
    if (!document->domWindow() || !frame)
      return ScriptPromise::RejectWithDOMException(
          script_state,
          DOMException::Create(kInvalidStateError,
                               "Document is detached from window."));
    PushController::ClientFrom(frame).Subscribe(
        registration_->WebRegistration(), web_options,
        Frame::HasTransientUserActivation(frame, true /* checkIfMainThread */),
        std::make_unique<PushSubscriptionCallbacks>(resolver, registration_));
  } else {
    PushProvider()->Subscribe(
        registration_->WebRegistration(), web_options,
        Frame::HasTransientUserActivation(nullptr,
                                          true /* checkIfMainThread */),
        std::make_unique<PushSubscriptionCallbacks>(resolver, registration_));
  }

  return promise;
}

ScriptPromise PushManager::getSubscription(ScriptState* script_state) {
  ScriptPromiseResolver* resolver = ScriptPromiseResolver::Create(script_state);
  ScriptPromise promise = resolver->Promise();

  PushProvider()->GetSubscription(
      registration_->WebRegistration(),
      std::make_unique<PushSubscriptionCallbacks>(resolver, registration_));
  return promise;
}

ScriptPromise PushManager::permissionState(
    ScriptState* script_state,
    const PushSubscriptionOptionsInit& options,
    ExceptionState& exception_state) {
  if (ExecutionContext::From(script_state)->IsDocument()) {
    Document* document = ToDocument(ExecutionContext::From(script_state));
    if (!document->domWindow() || !document->GetFrame())
      return ScriptPromise::RejectWithDOMException(
          script_state,
          DOMException::Create(kInvalidStateError,
                               "Document is detached from window."));
  }

  return PushMessagingBridge::From(registration_)
      ->GetPermissionState(script_state, options);
}

void PushManager::Trace(blink::Visitor* visitor) {
  visitor->Trace(registration_);
  ScriptWrappable::Trace(visitor);
}

}  // namespace blink
