// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/modules/serviceworkers/service_worker_window_client_callback.h"

#include <memory>

#include "base/memory/ptr_util.h"
#include "third_party/blink/public/mojom/service_worker/service_worker_error_type.mojom-blink.h"
#include "third_party/blink/renderer/bindings/core/v8/script_promise_resolver.h"
#include "third_party/blink/renderer/core/dom/dom_exception.h"
#include "third_party/blink/renderer/modules/serviceworkers/service_worker_error.h"
#include "third_party/blink/renderer/modules/serviceworkers/service_worker_window_client.h"
#include "third_party/blink/renderer/platform/bindings/v8_throw_exception.h"

namespace blink {

void NavigateClientCallback::OnSuccess(
    std::unique_ptr<WebServiceWorkerClientInfo> client_info) {
  if (!resolver_->GetExecutionContext() ||
      resolver_->GetExecutionContext()->IsContextDestroyed())
    return;
  resolver_->Resolve(ServiceWorkerWindowClient::Take(
      resolver_.Get(), base::WrapUnique(client_info.release())));
}

void NavigateClientCallback::OnError(const WebServiceWorkerError& error) {
  if (!resolver_->GetExecutionContext() ||
      resolver_->GetExecutionContext()->IsContextDestroyed())
    return;

  if (error.error_type == mojom::blink::ServiceWorkerErrorType::kNavigation) {
    ScriptState::Scope scope(resolver_->GetScriptState());
    resolver_->Reject(V8ThrowException::CreateTypeError(
        resolver_->GetScriptState()->GetIsolate(), error.message));
    return;
  }

  resolver_->Reject(ServiceWorkerError::Take(resolver_.Get(), error));
}

}  // namespace blink
