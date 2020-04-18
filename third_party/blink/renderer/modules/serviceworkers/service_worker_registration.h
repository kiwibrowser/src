// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_MODULES_SERVICEWORKERS_SERVICE_WORKER_REGISTRATION_H_
#define THIRD_PARTY_BLINK_RENDERER_MODULES_SERVICEWORKERS_SERVICE_WORKER_REGISTRATION_H_

#include <memory>
#include "third_party/blink/public/platform/modules/serviceworker/web_service_worker_registration.h"
#include "third_party/blink/public/platform/modules/serviceworker/web_service_worker_registration_proxy.h"
#include "third_party/blink/renderer/bindings/core/v8/active_script_wrappable.h"
#include "third_party/blink/renderer/bindings/core/v8/script_promise_resolver.h"
#include "third_party/blink/renderer/core/dom/context_lifecycle_observer.h"
#include "third_party/blink/renderer/core/dom/events/event_target.h"
#include "third_party/blink/renderer/modules/serviceworkers/navigation_preload_manager.h"
#include "third_party/blink/renderer/modules/serviceworkers/service_worker.h"
#include "third_party/blink/renderer/modules/serviceworkers/service_worker_registration.h"
#include "third_party/blink/renderer/platform/supplementable.h"
#include "third_party/blink/renderer/platform/wtf/forward.h"

namespace blink {

class ScriptPromise;
class ScriptState;

// The implementation of a service worker registration object in Blink. Actual
// registration representation is in the embedder and this class accesses it
// via WebServiceWorkerRegistration::Handle object.
class ServiceWorkerRegistration final
    : public EventTargetWithInlineData,
      public ActiveScriptWrappable<ServiceWorkerRegistration>,
      public ContextLifecycleObserver,
      public WebServiceWorkerRegistrationProxy,
      public Supplementable<ServiceWorkerRegistration> {
  DEFINE_WRAPPERTYPEINFO();
  USING_GARBAGE_COLLECTED_MIXIN(ServiceWorkerRegistration);
  USING_PRE_FINALIZER(ServiceWorkerRegistration, Dispose);

 public:
  // Called from CallbackPromiseAdapter.
  using WebType = std::unique_ptr<WebServiceWorkerRegistration::Handle>;
  static ServiceWorkerRegistration* Take(
      ScriptPromiseResolver*,
      std::unique_ptr<WebServiceWorkerRegistration::Handle>);

  // ScriptWrappable overrides.
  bool HasPendingActivity() const final;

  // EventTarget overrides.
  const AtomicString& InterfaceName() const override;
  ExecutionContext* GetExecutionContext() const override {
    return ContextLifecycleObserver::GetExecutionContext();
  }

  // WebServiceWorkerRegistrationProxy overrides.
  void DispatchUpdateFoundEvent() override;
  void SetInstalling(std::unique_ptr<WebServiceWorker::Handle>) override;
  void SetWaiting(std::unique_ptr<WebServiceWorker::Handle>) override;
  void SetActive(std::unique_ptr<WebServiceWorker::Handle>) override;

  // Returns an existing registration object for the handle if it exists.
  // Otherwise, returns a new registration object.
  static ServiceWorkerRegistration* GetOrCreate(
      ExecutionContext*,
      std::unique_ptr<WebServiceWorkerRegistration::Handle>);

  ServiceWorker* installing() { return installing_; }
  ServiceWorker* waiting() { return waiting_; }
  ServiceWorker* active() { return active_; }
  NavigationPreloadManager* navigationPreload();

  String scope() const;
  String updateViaCache() const;

  WebServiceWorkerRegistration* WebRegistration() {
    return handle_->Registration();
  }

  ScriptPromise update(ScriptState*);
  ScriptPromise unregister(ScriptState*);

  DEFINE_ATTRIBUTE_EVENT_LISTENER(updatefound);

  ~ServiceWorkerRegistration() override;

  void Trace(blink::Visitor*) override;

 private:
  ServiceWorkerRegistration(
      ExecutionContext*,
      std::unique_ptr<WebServiceWorkerRegistration::Handle>);
  void Dispose();

  // ContextLifecycleObserver overrides.
  void ContextDestroyed(ExecutionContext*) override;

  // A handle to the registration representation in the embedder.
  std::unique_ptr<WebServiceWorkerRegistration::Handle> handle_;

  Member<ServiceWorker> installing_;
  Member<ServiceWorker> waiting_;
  Member<ServiceWorker> active_;
  Member<NavigationPreloadManager> navigation_preload_;

  bool stopped_;
};

class ServiceWorkerRegistrationArray {
  STATIC_ONLY(ServiceWorkerRegistrationArray);

 public:
  // Called from CallbackPromiseAdapter.
  using WebType = std::unique_ptr<
      WebVector<std::unique_ptr<WebServiceWorkerRegistration::Handle>>>;
  static HeapVector<Member<ServiceWorkerRegistration>> Take(
      ScriptPromiseResolver* resolver,
      WebType web_service_worker_registrations) {
    HeapVector<Member<ServiceWorkerRegistration>> registrations;
    for (auto& registration : *web_service_worker_registrations) {
      registrations.push_back(
          ServiceWorkerRegistration::Take(resolver, std::move(registration)));
    }
    return registrations;
  }
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_MODULES_SERVICEWORKERS_SERVICE_WORKER_REGISTRATION_H_
