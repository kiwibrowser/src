// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/modules/serviceworkers/service_worker_registration.h"

#include <memory>
#include <utility>

#include "base/memory/ptr_util.h"
#include "third_party/blink/public/mojom/service_worker/service_worker_registration.mojom-blink.h"
#include "third_party/blink/public/platform/modules/serviceworker/web_service_worker_provider.h"
#include "third_party/blink/renderer/bindings/core/v8/callback_promise_adapter.h"
#include "third_party/blink/renderer/bindings/core/v8/script_promise.h"
#include "third_party/blink/renderer/core/dom/dom_exception.h"
#include "third_party/blink/renderer/core/dom/events/event.h"
#include "third_party/blink/renderer/core/dom/exception_code.h"
#include "third_party/blink/renderer/core/execution_context/execution_context.h"
#include "third_party/blink/renderer/modules/event_target_modules.h"
#include "third_party/blink/renderer/modules/serviceworkers/service_worker_container_client.h"
#include "third_party/blink/renderer/modules/serviceworkers/service_worker_error.h"
#include "third_party/blink/renderer/platform/bindings/script_state.h"

namespace blink {

ServiceWorkerRegistration* ServiceWorkerRegistration::Take(
    ScriptPromiseResolver* resolver,
    std::unique_ptr<WebServiceWorkerRegistration::Handle> handle) {
  return GetOrCreate(resolver->GetExecutionContext(), std::move(handle));
}

bool ServiceWorkerRegistration::HasPendingActivity() const {
  return !stopped_;
}

const AtomicString& ServiceWorkerRegistration::InterfaceName() const {
  return EventTargetNames::ServiceWorkerRegistration;
}

void ServiceWorkerRegistration::DispatchUpdateFoundEvent() {
  DispatchEvent(Event::Create(EventTypeNames::updatefound));
}

void ServiceWorkerRegistration::SetInstalling(
    std::unique_ptr<WebServiceWorker::Handle> handle) {
  if (!GetExecutionContext())
    return;
  installing_ = ServiceWorker::From(GetExecutionContext(),
                                    base::WrapUnique(handle.release()));
}

void ServiceWorkerRegistration::SetWaiting(
    std::unique_ptr<WebServiceWorker::Handle> handle) {
  if (!GetExecutionContext())
    return;
  waiting_ = ServiceWorker::From(GetExecutionContext(),
                                 base::WrapUnique(handle.release()));
}

void ServiceWorkerRegistration::SetActive(
    std::unique_ptr<WebServiceWorker::Handle> handle) {
  if (!GetExecutionContext())
    return;
  active_ = ServiceWorker::From(GetExecutionContext(),
                                base::WrapUnique(handle.release()));
}

ServiceWorkerRegistration* ServiceWorkerRegistration::GetOrCreate(
    ExecutionContext* execution_context,
    std::unique_ptr<WebServiceWorkerRegistration::Handle> handle) {
  DCHECK(handle);

  ServiceWorkerRegistration* existing_registration =
      static_cast<ServiceWorkerRegistration*>(handle->Registration()->Proxy());
  if (existing_registration) {
    DCHECK_EQ(existing_registration->GetExecutionContext(), execution_context);
    return existing_registration;
  }

  return new ServiceWorkerRegistration(execution_context, std::move(handle));
}

NavigationPreloadManager* ServiceWorkerRegistration::navigationPreload() {
  if (!navigation_preload_)
    navigation_preload_ = NavigationPreloadManager::Create(this);
  return navigation_preload_;
}

String ServiceWorkerRegistration::scope() const {
  return handle_->Registration()->Scope().GetString();
}

String ServiceWorkerRegistration::updateViaCache() const {
  switch (handle_->Registration()->UpdateViaCache()) {
    case mojom::ServiceWorkerUpdateViaCache::kImports:
      return "imports";
    case mojom::ServiceWorkerUpdateViaCache::kAll:
      return "all";
    case mojom::ServiceWorkerUpdateViaCache::kNone:
      return "none";
  }
  NOTREACHED();
  return "";
}

ScriptPromise ServiceWorkerRegistration::update(ScriptState* script_state) {
  ServiceWorkerContainerClient* client =
      ServiceWorkerContainerClient::From(GetExecutionContext());
  if (!client || !client->Provider())
    return ScriptPromise::RejectWithDOMException(
        script_state,
        DOMException::Create(kInvalidStateError,
                             "Failed to update a ServiceWorkerRegistration: No "
                             "associated provider is available."));

  ScriptPromiseResolver* resolver = ScriptPromiseResolver::Create(script_state);
  ScriptPromise promise = resolver->Promise();
  handle_->Registration()->Update(
      std::make_unique<
          CallbackPromiseAdapter<void, ServiceWorkerErrorForUpdate>>(resolver));
  return promise;
}

ScriptPromise ServiceWorkerRegistration::unregister(ScriptState* script_state) {
  ServiceWorkerContainerClient* client =
      ServiceWorkerContainerClient::From(GetExecutionContext());
  if (!client || !client->Provider())
    return ScriptPromise::RejectWithDOMException(
        script_state,
        DOMException::Create(kInvalidStateError,
                             "Failed to unregister a "
                             "ServiceWorkerRegistration: No "
                             "associated provider is available."));

  ScriptPromiseResolver* resolver = ScriptPromiseResolver::Create(script_state);
  ScriptPromise promise = resolver->Promise();
  handle_->Registration()->Unregister(
      std::make_unique<CallbackPromiseAdapter<bool, ServiceWorkerError>>(
          resolver));
  return promise;
}

ServiceWorkerRegistration::ServiceWorkerRegistration(
    ExecutionContext* execution_context,
    std::unique_ptr<WebServiceWorkerRegistration::Handle> handle)
    : ContextLifecycleObserver(execution_context),
      handle_(std::move(handle)),
      stopped_(false) {
  DCHECK(handle_);
  DCHECK(!handle_->Registration()->Proxy());

  if (!execution_context)
    return;
  handle_->Registration()->SetProxy(this);
}

ServiceWorkerRegistration::~ServiceWorkerRegistration() = default;

void ServiceWorkerRegistration::Dispose() {
  // Promptly clears a raw reference from content/ to an on-heap object
  // so that content/ doesn't access it in a lazy sweeping phase.
  handle_.reset();
}

void ServiceWorkerRegistration::Trace(blink::Visitor* visitor) {
  visitor->Trace(installing_);
  visitor->Trace(waiting_);
  visitor->Trace(active_);
  visitor->Trace(navigation_preload_);
  EventTargetWithInlineData::Trace(visitor);
  ContextLifecycleObserver::Trace(visitor);
  Supplementable<ServiceWorkerRegistration>::Trace(visitor);
}

void ServiceWorkerRegistration::ContextDestroyed(ExecutionContext*) {
  if (stopped_)
    return;
  stopped_ = true;
  handle_->Registration()->ProxyStopped();
}

}  // namespace blink
