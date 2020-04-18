// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/modules/serviceworkers/navigation_preload_callbacks.h"

#include "third_party/blink/public/platform/modules/serviceworker/web_navigation_preload_state.h"
#include "third_party/blink/renderer/bindings/core/v8/script_promise_resolver.h"
#include "third_party/blink/renderer/core/dom/dom_exception.h"
#include "third_party/blink/renderer/modules/serviceworkers/navigation_preload_state.h"
#include "third_party/blink/renderer/modules/serviceworkers/service_worker_error.h"

namespace blink {

EnableNavigationPreloadCallbacks::EnableNavigationPreloadCallbacks(
    ScriptPromiseResolver* resolver)
    : resolver_(resolver) {
  DCHECK(resolver_);
}

EnableNavigationPreloadCallbacks::~EnableNavigationPreloadCallbacks() = default;

void EnableNavigationPreloadCallbacks::OnSuccess() {
  if (!resolver_->GetExecutionContext() ||
      resolver_->GetExecutionContext()->IsContextDestroyed())
    return;
  resolver_->Resolve();
}

void EnableNavigationPreloadCallbacks::OnError(
    const WebServiceWorkerError& error) {
  if (!resolver_->GetExecutionContext() ||
      resolver_->GetExecutionContext()->IsContextDestroyed())
    return;
  resolver_->Reject(ServiceWorkerError::Take(resolver_.Get(), error));
}

GetNavigationPreloadStateCallbacks::GetNavigationPreloadStateCallbacks(
    ScriptPromiseResolver* resolver)
    : resolver_(resolver) {
  DCHECK(resolver_);
}

GetNavigationPreloadStateCallbacks::~GetNavigationPreloadStateCallbacks() =
    default;

void GetNavigationPreloadStateCallbacks::OnSuccess(
    const WebNavigationPreloadState& state) {
  if (!resolver_->GetExecutionContext() ||
      resolver_->GetExecutionContext()->IsContextDestroyed())
    return;
  NavigationPreloadState dict;
  dict.setEnabled(state.enabled);
  dict.setHeaderValue(state.header_value);
  resolver_->Resolve(dict);
}

void GetNavigationPreloadStateCallbacks::OnError(
    const WebServiceWorkerError& error) {
  if (!resolver_->GetExecutionContext() ||
      resolver_->GetExecutionContext()->IsContextDestroyed())
    return;
  resolver_->Reject(ServiceWorkerError::Take(resolver_.Get(), error));
}

SetNavigationPreloadHeaderCallbacks::SetNavigationPreloadHeaderCallbacks(
    ScriptPromiseResolver* resolver)
    : resolver_(resolver) {
  DCHECK(resolver_);
}

SetNavigationPreloadHeaderCallbacks::~SetNavigationPreloadHeaderCallbacks() =
    default;

void SetNavigationPreloadHeaderCallbacks::OnSuccess() {
  if (!resolver_->GetExecutionContext() ||
      resolver_->GetExecutionContext()->IsContextDestroyed())
    return;
  resolver_->Resolve();
}

void SetNavigationPreloadHeaderCallbacks::OnError(
    const WebServiceWorkerError& error) {
  if (!resolver_->GetExecutionContext() ||
      resolver_->GetExecutionContext()->IsContextDestroyed())
    return;
  resolver_->Reject(ServiceWorkerError::Take(resolver_.Get(), error));
}

}  // namespace blink
