// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/modules/cookie_store/global_cookie_store.h"

#include <utility>

#include "services/network/public/mojom/restricted_cookie_manager.mojom-blink.h"
#include "services/service_manager/public/cpp/interface_provider.h"
#include "third_party/blink/renderer/core/frame/local_dom_window.h"
#include "third_party/blink/renderer/core/frame/local_frame.h"
#include "third_party/blink/renderer/core/workers/worker_thread.h"
#include "third_party/blink/renderer/modules/cookie_store/cookie_store.h"
#include "third_party/blink/renderer/modules/serviceworkers/service_worker_global_scope.h"
#include "third_party/blink/renderer/platform/heap/handle.h"
#include "third_party/blink/renderer/platform/supplementable.h"

namespace blink {

namespace {

template <typename T>
class GlobalCookieStoreImpl final
    : public GarbageCollected<GlobalCookieStoreImpl<T>>,
      public Supplement<T> {
  USING_GARBAGE_COLLECTED_MIXIN(GlobalCookieStoreImpl);

 public:
  static const char kSupplementName[];

  static GlobalCookieStoreImpl& From(T& supplementable) {
    GlobalCookieStoreImpl* supplement =
        Supplement<T>::template From<GlobalCookieStoreImpl>(supplementable);
    if (!supplement) {
      supplement = new GlobalCookieStoreImpl(supplementable);
      Supplement<T>::ProvideTo(supplementable, supplement);
    }
    return *supplement;
  }

  CookieStore* GetCookieStore(T& scope) {
    if (!cookie_store_) {
      ExecutionContext* execution_context = scope.GetExecutionContext();

      service_manager::InterfaceProvider* interface_provider =
          execution_context->GetInterfaceProvider();
      if (!interface_provider)
        return nullptr;
      cookie_store_ = BuildCookieStore(execution_context, interface_provider);
    }
    return cookie_store_;
  }

  CookieStore* BuildCookieStore(ExecutionContext*,
                                service_manager::InterfaceProvider*);

  void Trace(blink::Visitor* visitor) override {
    visitor->Trace(cookie_store_);
    Supplement<T>::Trace(visitor);
  }

 private:
  explicit GlobalCookieStoreImpl(T& supplementable)
      : Supplement<T>(supplementable) {}

  Member<CookieStore> cookie_store_;
};

template <>
CookieStore* GlobalCookieStoreImpl<LocalDOMWindow>::BuildCookieStore(
    ExecutionContext* execution_context,
    service_manager::InterfaceProvider* interface_provider) {
  network::mojom::blink::RestrictedCookieManagerPtr cookie_manager_ptr;
  interface_provider->GetInterface(mojo::MakeRequest(&cookie_manager_ptr));

  return CookieStore::Create(execution_context, std::move(cookie_manager_ptr),
                             blink::mojom::blink::CookieStorePtr());
}

template <>
CookieStore* GlobalCookieStoreImpl<WorkerGlobalScope>::BuildCookieStore(
    ExecutionContext* execution_context,
    service_manager::InterfaceProvider* interface_provider) {
  network::mojom::blink::RestrictedCookieManagerPtr cookie_manager_ptr;
  interface_provider->GetInterface(mojo::MakeRequest(&cookie_manager_ptr));

  blink::mojom::blink::CookieStorePtr cookie_store_ptr;
  interface_provider->GetInterface(mojo::MakeRequest(&cookie_store_ptr));

  return CookieStore::Create(execution_context, std::move(cookie_manager_ptr),
                             std::move(cookie_store_ptr));
}

// static
template <typename T>
const char GlobalCookieStoreImpl<T>::kSupplementName[] =
    "GlobalCookieStoreImpl";

}  // namespace

CookieStore* GlobalCookieStore::cookieStore(LocalDOMWindow& window) {
  return GlobalCookieStoreImpl<LocalDOMWindow>::From(window).GetCookieStore(
      window);
}

CookieStore* GlobalCookieStore::cookieStore(ServiceWorkerGlobalScope& worker) {
  // ServiceWorkerGlobalScope is Supplementable<WorkerGlobalScope>, not
  // Supplementable<ServiceWorkerGlobalScope>.
  return GlobalCookieStoreImpl<WorkerGlobalScope>::From(worker).GetCookieStore(
      worker);
}

}  // namespace blink
