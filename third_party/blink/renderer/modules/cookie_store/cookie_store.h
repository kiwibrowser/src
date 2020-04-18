// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_MODULES_COOKIE_STORE_COOKIE_STORE_H_
#define THIRD_PARTY_BLINK_RENDERER_MODULES_COOKIE_STORE_COOKIE_STORE_H_

#include "mojo/public/cpp/bindings/binding.h"
#include "services/network/public/mojom/restricted_cookie_manager.mojom-blink.h"
#include "third_party/blink/public/mojom/cookie_store/cookie_store.mojom-blink.h"
#include "third_party/blink/renderer/bindings/core/v8/exception_state.h"
#include "third_party/blink/renderer/bindings/core/v8/script_promise.h"
#include "third_party/blink/renderer/core/dom/context_lifecycle_observer.h"
#include "third_party/blink/renderer/core/dom/events/event_target.h"
#include "third_party/blink/renderer/platform/bindings/script_wrappable.h"
#include "third_party/blink/renderer/platform/heap/handle.h"
#include "third_party/blink/renderer/platform/wtf/text/wtf_string.h"
#include "third_party/blink/renderer/platform/wtf/vector.h"

namespace blink {

class CookieStoreGetOptions;
class CookieStoreSetOptions;
class ScriptPromiseResolver;
class ScriptState;

class CookieStore final : public EventTargetWithInlineData,
                          public ContextLifecycleObserver,
                          public network::mojom::blink::CookieChangeListener {
  DEFINE_WRAPPERTYPEINFO();
  USING_GARBAGE_COLLECTED_MIXIN(CookieStore);

 public:
  // Needed because of the network::mojom::blink::RestrictedCookieManagerPtr
  ~CookieStore() override;

  static CookieStore* Create(
      ExecutionContext* execution_context,
      network::mojom::blink::RestrictedCookieManagerPtr backend,
      blink::mojom::blink::CookieStorePtr subscription_backend) {
    return new CookieStore(execution_context, std::move(backend),
                           std::move(subscription_backend));
  }

  ScriptPromise getAll(ScriptState*,
                       const CookieStoreGetOptions&,
                       ExceptionState&);
  ScriptPromise getAll(ScriptState*,
                       const String& name,
                       const CookieStoreGetOptions&,
                       ExceptionState&);
  ScriptPromise get(ScriptState*,
                    const CookieStoreGetOptions&,
                    ExceptionState&);
  ScriptPromise get(ScriptState*,
                    const String& name,
                    const CookieStoreGetOptions&,
                    ExceptionState&);
  ScriptPromise has(ScriptState*,
                    const CookieStoreGetOptions&,
                    ExceptionState&);
  ScriptPromise has(ScriptState*,
                    const String& name,
                    const CookieStoreGetOptions&,
                    ExceptionState&);

  ScriptPromise set(ScriptState*,
                    const CookieStoreSetOptions&,
                    ExceptionState&);
  ScriptPromise set(ScriptState*,
                    const String& name,
                    const String& value,
                    const CookieStoreSetOptions&,
                    ExceptionState&);
  ScriptPromise Delete(ScriptState*,
                       const CookieStoreSetOptions&,
                       ExceptionState&);
  ScriptPromise Delete(ScriptState*,
                       const String& name,
                       const CookieStoreSetOptions&,
                       ExceptionState&);
  ScriptPromise subscribeToChanges(
      ScriptState*,
      const HeapVector<CookieStoreGetOptions>& subscriptions,
      ExceptionState&);
  ScriptPromise getChangeSubscriptions(ScriptState*, ExceptionState&);

  // GarbageCollected
  void Trace(blink::Visitor* visitor) override {
    EventTargetWithInlineData::Trace(visitor);
    ContextLifecycleObserver::Trace(visitor);
  }

  // ActiveScriptWrappable
  void ContextDestroyed(ExecutionContext*) override;

  // EventTargetWithInlineData
  DEFINE_ATTRIBUTE_EVENT_LISTENER(change);
  const AtomicString& InterfaceName() const override;
  ExecutionContext* GetExecutionContext() const override;
  void RemoveAllEventListeners() override;

  // RestrictedCookieChangeListener
  void OnCookieChange(network::mojom::blink::CanonicalCookiePtr,
                      network::mojom::blink::CookieChangeCause) override;

 protected:
  // EventTarget overrides.
  void AddedEventListener(const AtomicString& event_type,
                          RegisteredEventListener&) final;
  void RemovedEventListener(const AtomicString& event_type,
                            const RegisteredEventListener&) final;

 private:
  using DoReadBackendResultConverter =
      void (*)(ScriptPromiseResolver*,
               Vector<network::mojom::blink::CanonicalCookiePtr>);

  CookieStore(ExecutionContext*,
              network::mojom::blink::RestrictedCookieManagerPtr backend,
              blink::mojom::blink::CookieStorePtr subscription_backend);

  // Common code in CookieStore::{get,getAll,has}.
  //
  // All cookie-reading methods use the same RestrictedCookieManager API, and
  // only differ in how they present the returned data. The difference is
  // captured in the DoReadBackendResultConverter argument, which should point
  // to one of the static methods below.
  ScriptPromise DoRead(ScriptState*,
                       const String& name,
                       const CookieStoreGetOptions&,
                       DoReadBackendResultConverter,
                       ExceptionState&);

  // Converts the result of a RestrictedCookieManager::GetAllForUrl mojo call to
  // the promise result expected by CookieStore.getAll.
  static void GetAllForUrlToGetAllResult(
      ScriptPromiseResolver*,
      Vector<network::mojom::blink::CanonicalCookiePtr> backend_result);

  // Converts the result of a RestrictedCookieManager::GetAllForUrl mojo call to
  // the promise result expected by CookieStore.get.
  static void GetAllForUrlToGetResult(
      ScriptPromiseResolver*,
      Vector<network::mojom::blink::CanonicalCookiePtr> backend_result);

  // Converts the result of a RestrictedCookieManager::GetAllForUrl mojo call to
  // the promise result expected by CookieStore.has.
  static void GetAllForUrlToHasResult(
      ScriptPromiseResolver*,
      Vector<network::mojom::blink::CanonicalCookiePtr> backend_result);

  // Common code in CookieStore::delete and CookieStore::set.
  ScriptPromise DoWrite(ScriptState*,
                        const String& name,
                        const String& value,
                        const CookieStoreSetOptions&,
                        bool is_deletion,
                        ExceptionState&);

  static void OnSetCanonicalCookieResult(ScriptPromiseResolver*,
                                         bool backend_result);

  static void OnSubscribeToCookieChangesResult(ScriptPromiseResolver*,
                                               bool backend_result);
  static void OnGetCookieChangeSubscriptionResult(
      ScriptPromiseResolver*,
      Vector<blink::mojom::blink::CookieChangeSubscriptionPtr> backend_result,
      bool backend_success);

  // Called when a change event listener is added.
  //
  // This is idempotent during the time intervals between StopObserving() calls.
  void StartObserving();

  // Called when all the change event listeners have been removed.
  void StopObserving();

  // Wraps an always-on Mojo pipe for sending requests to the Network Service.
  network::mojom::blink::RestrictedCookieManagerPtr backend_;

  // Wraps a Mojo pipe for managing service worker cookie change subscriptions.
  //
  // This pipe is always connected in service worker execution contexts, and
  // never connected in document contexts.
  blink::mojom::blink::CookieStorePtr subscription_backend_;

  // Wraps a Mojo pipe used to receive cookie change notifications.
  //
  // This binding is set up on-demand, when the cookie store has at least one
  // change event listener. If all the listeners are unregistered, the binding
  // is torn down.
  mojo::Binding<network::mojom::blink::CookieChangeListener>
      change_listener_binding_;

  // Default for cookie_url in CookieStoreGetOptions.
  //
  // This is the current document's URL. API calls coming from a document
  // context are not allowed to specify a different cookie_url, whereas Service
  // Workers may specify any URL that falls under their registration.
  const KURL default_cookie_url_;

  // The RFC 6265bis "site for cookies" for this store's ExecutionContext.
  const KURL default_site_for_cookies_;
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_MODULES_COOKIE_STORE_COOKIE_STORE_H_
