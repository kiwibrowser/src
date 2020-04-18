// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/modules/cookie_store/cookie_store.h"

#include <utility>

#include "base/optional.h"
#include "services/network/public/mojom/restricted_cookie_manager.mojom-blink.h"
#include "third_party/blink/renderer/bindings/core/v8/script_promise_resolver.h"
#include "third_party/blink/renderer/core/dom/document.h"
#include "third_party/blink/renderer/core/dom/dom_exception.h"
#include "third_party/blink/renderer/core/dom/exception_code.h"
#include "third_party/blink/renderer/modules/cookie_store/cookie_change_event.h"
#include "third_party/blink/renderer/modules/cookie_store/cookie_list_item.h"
#include "third_party/blink/renderer/modules/cookie_store/cookie_store_get_options.h"
#include "third_party/blink/renderer/modules/cookie_store/cookie_store_set_options.h"
#include "third_party/blink/renderer/modules/event_modules.h"
#include "third_party/blink/renderer/modules/event_target_modules.h"
#include "third_party/blink/renderer/modules/serviceworkers/service_worker_global_scope.h"
#include "third_party/blink/renderer/modules/serviceworkers/service_worker_registration.h"
#include "third_party/blink/renderer/platform/bindings/script_state.h"
#include "third_party/blink/renderer/platform/heap/handle.h"
#include "third_party/blink/renderer/platform/weborigin/kurl.h"
#include "third_party/blink/renderer/platform/weborigin/security_origin.h"
#include "third_party/blink/renderer/platform/wtf/functional.h"
#include "third_party/blink/renderer/platform/wtf/text/wtf_string.h"
#include "third_party/blink/renderer/platform/wtf/time.h"

namespace blink {

namespace {

// Returns null if and only if an exception is thrown.
network::mojom::blink::CookieManagerGetOptionsPtr ToBackendOptions(
    const String& name,  // Value of the "name" positional argument.
    const CookieStoreGetOptions& options,
    ExceptionState& exception_state) {
  auto backend_options = network::mojom::blink::CookieManagerGetOptions::New();

  // TODO(crbug.com/729800): Handle the url option.

  if (options.matchType() == "startsWith") {
    backend_options->match_type =
        network::mojom::blink::CookieMatchType::STARTS_WITH;
  } else {
    DCHECK_EQ(options.matchType(), WTF::String("equals"));
    backend_options->match_type =
        network::mojom::blink::CookieMatchType::EQUALS;
  }

  if (name.IsNull()) {
    if (options.hasName()) {
      backend_options->name = options.name();
    } else {
      // No name provided. Use a filter that matches all cookies. This overrides
      // a user-provided matchType.
      backend_options->match_type =
          network::mojom::blink::CookieMatchType::STARTS_WITH;
      backend_options->name = g_empty_string;
    }
  } else {
    if (options.hasName()) {
      exception_state.ThrowTypeError(
          "Cookie name specified both as an argument and as an option");
      return nullptr;
    }
    backend_options->name = name;
  }

  return backend_options;
}

// Returns null if and only if an exception is thrown.
network::mojom::blink::CanonicalCookiePtr ToCanonicalCookie(
    const KURL& cookie_url,
    const String& name,   // Value of the "name" positional argument.
    const String& value,  // Value of the "value" positional argument.
    bool for_deletion,    // True for CookieStore.delete, false for set.
    const CookieStoreSetOptions& options,
    ExceptionState& exception_state) {
  auto canonical_cookie = network::mojom::blink::CanonicalCookie::New();

  if (name.IsNull()) {
    if (!options.hasName()) {
      exception_state.ThrowTypeError("Unspecified cookie name");
      return nullptr;
    }
    canonical_cookie->name = options.name();
  } else {
    if (options.hasName()) {
      exception_state.ThrowTypeError(
          "Cookie name specified both as an argument and as an option");
      return nullptr;
    }
    canonical_cookie->name = name;
  }

  if (for_deletion) {
    DCHECK(value.IsNull());
    if (options.hasValue()) {
      exception_state.ThrowTypeError(
          "Cookie value is meaningless when deleting");
      return nullptr;
    }
    canonical_cookie->value = g_empty_string;

    if (options.hasExpires()) {
      exception_state.ThrowTypeError(
          "Cookie expiration time is meaningless when deleting");
      return nullptr;
    }
    canonical_cookie->expiry = WTF::Time::Min();
  } else {
    if (value.IsNull()) {
      if (!options.hasValue()) {
        exception_state.ThrowTypeError("Unspecified cookie value");
        return nullptr;
      }
      canonical_cookie->value = options.value();
    } else {
      if (options.hasValue()) {
        exception_state.ThrowTypeError(
            "Cookie value specified both as an argument and as an option");
        return nullptr;
      }
      canonical_cookie->value = value;
    }

    if (canonical_cookie->name.IsEmpty() &&
        canonical_cookie->value.Contains('=')) {
      exception_state.ThrowTypeError(
          "Cookie value cannot contain '=' if the name is empty.");
      return nullptr;
    }

    if (options.hasExpires())
      canonical_cookie->expiry = WTF::Time::FromJavaTime(options.expires());
    // The expires option is not set in CookieStoreSetOptions for session
    // cookies. This is represented by a null expiry field in CanonicalCookie.
  }

  if (options.hasDomain()) {
    // TODO(crbug.com/729800): Checks and exception throwing.
    canonical_cookie->domain = options.domain();
  } else {
    // TODO(crbug.com/729800): Correct value?
    canonical_cookie->domain = cookie_url.Host();
  }

  if (options.hasPath()) {
    canonical_cookie->path = options.path();
  } else {
    canonical_cookie->path = String("/");
  }

  bool is_secure_origin = SecurityOrigin::IsSecure(cookie_url);
  if (options.hasSecure()) {
    canonical_cookie->secure = options.secure();
  } else {
    canonical_cookie->secure = is_secure_origin;
  }

  if (name.StartsWith("__Secure-") || name.StartsWith("__Host-")) {
    if (!canonical_cookie->secure) {
      exception_state.ThrowTypeError(
          "__Secure- and __Host- cookies must be secure");
      return nullptr;
    }
    if (!is_secure_origin) {
      exception_state.ThrowTypeError(
          "__Secure- and __Host- cookies must be written from secure origin");
      return nullptr;
    }
  }

  canonical_cookie->httponly = options.httpOnly();
  return canonical_cookie;
}

// Returns null if and only if an exception is thrown.
blink::mojom::blink::CookieChangeSubscriptionPtr ToBackendSubscription(
    const KURL& default_cookie_url,
    const CookieStoreGetOptions& subscription,
    ExceptionState& exception_state) {
  auto backend_subscription =
      blink::mojom::blink::CookieChangeSubscription::New();

  if (subscription.hasURL()) {
    KURL subscription_url(default_cookie_url, subscription.url());
    // TODO(crbug.com/729800): Check that the URL is under default_cookie_url.
    backend_subscription->url = subscription_url;
  } else {
    backend_subscription->url = default_cookie_url;
  }

  if (subscription.matchType() == "startsWith") {
    backend_subscription->match_type =
        network::mojom::blink::CookieMatchType::STARTS_WITH;
  } else {
    DCHECK_EQ(subscription.matchType(), WTF::String("equals"));
    backend_subscription->match_type =
        network::mojom::blink::CookieMatchType::EQUALS;
  }

  if (subscription.hasName()) {
    backend_subscription->name = subscription.name();
  } else {
    // No name provided. Use a filter that matches all cookies. This overrides
    // a user-provided matchType.
    backend_subscription->match_type =
        network::mojom::blink::CookieMatchType::STARTS_WITH;
    backend_subscription->name = g_empty_string;
  }

  return backend_subscription;
}

void ToCookieListItem(
    const network::mojom::blink::CanonicalCookiePtr& canonical_cookie,
    bool is_deleted,  // True for the information from a cookie deletion event.
    CookieListItem& cookie) {
  cookie.setName(canonical_cookie->name);
  if (!is_deleted)
    cookie.setValue(canonical_cookie->value);
}

void ToCookieChangeSubscription(
    const blink::mojom::blink::CookieChangeSubscription& backend_subscription,
    CookieStoreGetOptions& subscription) {
  subscription.setURL(backend_subscription.url);

  if (backend_subscription.match_type !=
          network::mojom::blink::CookieMatchType::STARTS_WITH ||
      !backend_subscription.name.IsEmpty()) {
    subscription.setName(backend_subscription.name);
  }

  switch (backend_subscription.match_type) {
    case network::mojom::blink::CookieMatchType::STARTS_WITH:
      subscription.setMatchType(WTF::String("startsWith"));
      break;
    case network::mojom::blink::CookieMatchType::EQUALS:
      subscription.setMatchType(WTF::String("equals"));
      break;
  }

  subscription.setURL(backend_subscription.url);
}

const KURL& DefaultCookieURL(ExecutionContext* execution_context) {
  DCHECK(execution_context);

  if (execution_context->IsDocument()) {
    Document* document = ToDocument(execution_context);
    return document->CookieURL();
  }

  DCHECK(execution_context->IsServiceWorkerGlobalScope());
  ServiceWorkerGlobalScope* scope =
      ToServiceWorkerGlobalScope(execution_context);
  return scope->Url();
}

KURL DefaultSiteForCookies(ExecutionContext* execution_context) {
  DCHECK(execution_context);

  if (execution_context->IsDocument()) {
    Document* document = ToDocument(execution_context);
    return document->SiteForCookies();
  }

  DCHECK(execution_context->IsServiceWorkerGlobalScope());
  ServiceWorkerGlobalScope* scope =
      ToServiceWorkerGlobalScope(execution_context);
  return scope->Url();
}

}  // namespace

CookieStore::~CookieStore() = default;

ScriptPromise CookieStore::getAll(ScriptState* script_state,
                                  const CookieStoreGetOptions& options,
                                  ExceptionState& exception_state) {
  return getAll(script_state, WTF::String(), options, exception_state);
}

ScriptPromise CookieStore::getAll(ScriptState* script_state,
                                  const String& name,
                                  const CookieStoreGetOptions& options,
                                  ExceptionState& exception_state) {
  return DoRead(script_state, name, options,
                &CookieStore::GetAllForUrlToGetAllResult, exception_state);
}

ScriptPromise CookieStore::get(ScriptState* script_state,
                               const CookieStoreGetOptions& options,
                               ExceptionState& exception_state) {
  return get(script_state, WTF::String(), options, exception_state);
}

ScriptPromise CookieStore::get(ScriptState* script_state,
                               const String& name,
                               const CookieStoreGetOptions& options,
                               ExceptionState& exception_state) {
  return DoRead(script_state, name, options,
                &CookieStore::GetAllForUrlToGetResult, exception_state);
}

ScriptPromise CookieStore::has(ScriptState* script_state,
                               const CookieStoreGetOptions& options,
                               ExceptionState& exception_state) {
  return has(script_state, WTF::String(), options, exception_state);
}

ScriptPromise CookieStore::has(ScriptState* script_state,
                               const String& name,
                               const CookieStoreGetOptions& options,
                               ExceptionState& exception_state) {
  return DoRead(script_state, name, options,
                &CookieStore::GetAllForUrlToHasResult, exception_state);
}

ScriptPromise CookieStore::set(ScriptState* script_state,
                               const CookieStoreSetOptions& options,
                               ExceptionState& exception_state) {
  return set(script_state, WTF::String(), WTF::String(), options,
             exception_state);
}

ScriptPromise CookieStore::set(ScriptState* script_state,
                               const String& name,
                               const String& value,
                               const CookieStoreSetOptions& options,
                               ExceptionState& exception_state) {
  return DoWrite(script_state, name, value, options, false /* is_deletion */,
                 exception_state);
}

ScriptPromise CookieStore::Delete(ScriptState* script_state,
                                  const CookieStoreSetOptions& options,
                                  ExceptionState& exception_state) {
  return Delete(script_state, WTF::String(), options, exception_state);
}

ScriptPromise CookieStore::Delete(ScriptState* script_state,
                                  const String& name,
                                  const CookieStoreSetOptions& options,
                                  ExceptionState& exception_state) {
  return DoWrite(script_state, name, WTF::String(), options,
                 true /* is_deletion */, exception_state);
}

ScriptPromise CookieStore::subscribeToChanges(
    ScriptState* script_state,
    const HeapVector<CookieStoreGetOptions>& subscriptions,
    ExceptionState& exception_state) {
  DCHECK(GetExecutionContext()->IsServiceWorkerGlobalScope());

  Vector<blink::mojom::blink::CookieChangeSubscriptionPtr>
      backend_subscriptions;
  backend_subscriptions.ReserveInitialCapacity(subscriptions.size());
  for (const CookieStoreGetOptions& subscription : subscriptions) {
    blink::mojom::blink::CookieChangeSubscriptionPtr backend_subscription =
        ToBackendSubscription(default_cookie_url_, subscription,
                              exception_state);
    if (backend_subscription.is_null())
      return ScriptPromise();  // ToBackendSubscription has thrown an exception.
    backend_subscriptions.emplace_back(std::move(backend_subscription));
  }

  if (!subscription_backend_) {
    exception_state.ThrowDOMException(kInvalidStateError,
                                      "CookieStore backend went away");
    return ScriptPromise();
  }

  ServiceWorkerGlobalScope* scope =
      ToServiceWorkerGlobalScope(GetExecutionContext());

  if (!scope->IsInstalling()) {
    exception_state.ThrowTypeError("Outside the installation phase");
    return ScriptPromise();
  }

  ScriptPromiseResolver* resolver = ScriptPromiseResolver::Create(script_state);
  int64_t service_worker_registration_id =
      scope->registration()->WebRegistration()->RegistrationId();
  subscription_backend_->AppendSubscriptions(
      service_worker_registration_id, std::move(backend_subscriptions),
      WTF::Bind(&CookieStore::OnSubscribeToCookieChangesResult,
                WrapPersistent(resolver)));
  return resolver->Promise();
}

ScriptPromise CookieStore::getChangeSubscriptions(
    ScriptState* script_state,
    ExceptionState& exception_state) {
  DCHECK(GetExecutionContext()->IsServiceWorkerGlobalScope());

  if (!subscription_backend_) {
    exception_state.ThrowDOMException(kInvalidStateError,
                                      "CookieStore backend went away");
    return ScriptPromise();
  }

  ScriptPromiseResolver* resolver = ScriptPromiseResolver::Create(script_state);
  ServiceWorkerGlobalScope* scope =
      ToServiceWorkerGlobalScope(GetExecutionContext());
  int64_t service_worker_registration_id =
      scope->registration()->WebRegistration()->RegistrationId();
  subscription_backend_->GetSubscriptions(
      service_worker_registration_id,
      WTF::Bind(&CookieStore::OnGetCookieChangeSubscriptionResult,
                WrapPersistent(resolver)));
  return resolver->Promise();
}

void CookieStore::ContextDestroyed(ExecutionContext* execution_context) {
  StopObserving();
  backend_.reset();
}

const AtomicString& CookieStore::InterfaceName() const {
  return EventTargetNames::CookieStore;
}

ExecutionContext* CookieStore::GetExecutionContext() const {
  return ContextLifecycleObserver::GetExecutionContext();
}

void CookieStore::RemoveAllEventListeners() {
  EventTargetWithInlineData::RemoveAllEventListeners();
  DCHECK(!HasEventListeners());
  StopObserving();
}

void CookieStore::OnCookieChange(
    network::mojom::blink::CanonicalCookiePtr backend_cookie,
    network::mojom::blink::CookieChangeCause change_cause) {
  HeapVector<CookieListItem> changed, deleted;

  switch (change_cause) {
    case ::network::mojom::blink::CookieChangeCause::INSERTED:
    case ::network::mojom::blink::CookieChangeCause::EXPLICIT: {
      CookieListItem& cookie = changed.emplace_back();
      ToCookieListItem(backend_cookie, false /* is_deleted */, cookie);
      break;
    }
    case ::network::mojom::blink::CookieChangeCause::UNKNOWN_DELETION:
    case ::network::mojom::blink::CookieChangeCause::EXPIRED:
    case ::network::mojom::blink::CookieChangeCause::EVICTED:
    case ::network::mojom::blink::CookieChangeCause::EXPIRED_OVERWRITE: {
      CookieListItem& cookie = deleted.emplace_back();
      ToCookieListItem(backend_cookie, true /* is_deleted */, cookie);
      break;
    }

    case ::network::mojom::blink::CookieChangeCause::OVERWRITE:
      // A cookie overwrite causes an OVERWRITE (meaning the old cookie was
      // deleted) and an INSERTED.
      break;
  }

  if (changed.IsEmpty() && deleted.IsEmpty()) {
    // The backend only reported OVERWRITE events, which are dropped.
    return;
  }

  DispatchEvent(CookieChangeEvent::Create(
      EventTypeNames::change, std::move(changed), std::move(deleted)));
}

void CookieStore::AddedEventListener(
    const AtomicString& event_type,
    RegisteredEventListener& registered_listener) {
  EventTargetWithInlineData::AddedEventListener(event_type,
                                                registered_listener);
  StartObserving();
}

void CookieStore::RemovedEventListener(
    const AtomicString& event_type,
    const RegisteredEventListener& registered_listener) {
  EventTargetWithInlineData::RemovedEventListener(event_type,
                                                  registered_listener);
  if (!HasEventListeners())
    StopObserving();
}

CookieStore::CookieStore(
    ExecutionContext* execution_context,
    network::mojom::blink::RestrictedCookieManagerPtr backend,
    blink::mojom::blink::CookieStorePtr subscription_backend)
    : ContextLifecycleObserver(execution_context),
      backend_(std::move(backend)),
      subscription_backend_(std::move(subscription_backend)),
      change_listener_binding_(this),
      default_cookie_url_(DefaultCookieURL(execution_context)),
      default_site_for_cookies_(DefaultSiteForCookies(execution_context)) {
  DCHECK(backend_);
}

ScriptPromise CookieStore::DoRead(
    ScriptState* script_state,
    const String& name,
    const CookieStoreGetOptions& options,
    DoReadBackendResultConverter backend_result_converter,
    ExceptionState& exception_state) {
  network::mojom::blink::CookieManagerGetOptionsPtr backend_options =
      ToBackendOptions(name, options, exception_state);
  if (backend_options.is_null())
    return ScriptPromise();  // ToBackendOptions has thrown an exception.

  if (!backend_) {
    exception_state.ThrowDOMException(kInvalidStateError,
                                      "CookieStore backend went away");
    return ScriptPromise();
  }

  ScriptPromiseResolver* resolver = ScriptPromiseResolver::Create(script_state);
  backend_->GetAllForUrl(
      default_cookie_url_, default_site_for_cookies_,
      std::move(backend_options),
      WTF::Bind(backend_result_converter, WrapPersistent(resolver)));
  return resolver->Promise();
}

// static
void CookieStore::GetAllForUrlToGetAllResult(
    ScriptPromiseResolver* resolver,
    Vector<network::mojom::blink::CanonicalCookiePtr> backend_cookies) {
  ScriptState* script_state = resolver->GetScriptState();
  if (!script_state->ContextIsValid())
    return;

  HeapVector<CookieListItem> cookies;
  cookies.ReserveInitialCapacity(backend_cookies.size());
  for (const auto& canonical_cookie : backend_cookies) {
    CookieListItem& cookie = cookies.emplace_back();
    ToCookieListItem(canonical_cookie, false /* is_deleted */, cookie);
  }

  resolver->Resolve(std::move(cookies));
}

// static
void CookieStore::GetAllForUrlToGetResult(
    ScriptPromiseResolver* resolver,
    Vector<network::mojom::blink::CanonicalCookiePtr> backend_cookies) {
  ScriptState* script_state = resolver->GetScriptState();
  if (!script_state->ContextIsValid())
    return;

  if (backend_cookies.IsEmpty()) {
    resolver->Resolve(v8::Null(script_state->GetIsolate()));
    return;
  }

  const auto& canonical_cookie = backend_cookies.front();
  CookieListItem cookie;
  ToCookieListItem(canonical_cookie, false /* is_deleted */, cookie);
  resolver->Resolve(cookie);
}

// static
void CookieStore::GetAllForUrlToHasResult(
    ScriptPromiseResolver* resolver,
    Vector<network::mojom::blink::CanonicalCookiePtr> backend_cookies) {
  ScriptState* script_state = resolver->GetScriptState();
  if (!script_state->ContextIsValid())
    return;

  resolver->Resolve(!backend_cookies.IsEmpty());
}

ScriptPromise CookieStore::DoWrite(ScriptState* script_state,
                                   const String& name,
                                   const String& value,
                                   const CookieStoreSetOptions& options,
                                   bool is_deletion,
                                   ExceptionState& exception_state) {
  network::mojom::blink::CanonicalCookiePtr canonical_cookie =
      ToCanonicalCookie(default_cookie_url_, name, value, is_deletion, options,
                        exception_state);
  if (canonical_cookie.is_null())
    return ScriptPromise();  // ToCanonicalCookie has thrown an exception.

  if (!backend_) {
    exception_state.ThrowDOMException(kInvalidStateError,
                                      "CookieStore backend went away");
    return ScriptPromise();
  }

  ScriptPromiseResolver* resolver = ScriptPromiseResolver::Create(script_state);
  backend_->SetCanonicalCookie(
      std::move(canonical_cookie), default_cookie_url_,
      default_site_for_cookies_,
      WTF::Bind(&CookieStore::OnSetCanonicalCookieResult,
                WrapPersistent(resolver)));
  return resolver->Promise();
}

// static
void CookieStore::OnSetCanonicalCookieResult(ScriptPromiseResolver* resolver,
                                             bool backend_success) {
  ScriptState* script_state = resolver->GetScriptState();
  if (!script_state->ContextIsValid())
    return;

  if (!backend_success) {
    resolver->Reject(DOMException::Create(
        kUnknownError, "An unknown error occured while writing the cookie."));
    return;
  }
  resolver->Resolve();
}

// static
void CookieStore::OnSubscribeToCookieChangesResult(
    ScriptPromiseResolver* resolver,
    bool backend_success) {
  ScriptState* script_state = resolver->GetScriptState();
  if (!script_state->ContextIsValid())
    return;

  if (!backend_success) {
    resolver->Reject(DOMException::Create(
        kUnknownError,
        "An unknown error occured while subscribing to cookie changes."));
    return;
  }
  resolver->Resolve();
}

// static
void CookieStore::OnGetCookieChangeSubscriptionResult(
    ScriptPromiseResolver* resolver,
    Vector<blink::mojom::blink::CookieChangeSubscriptionPtr> backend_result,
    bool backend_success) {
  ScriptState* script_state = resolver->GetScriptState();
  if (!script_state->ContextIsValid())
    return;

  if (!backend_success) {
    resolver->Reject(DOMException::Create(
        kUnknownError,
        "An unknown error occured while reading cookie change subscriptions."));
    return;
  }

  HeapVector<CookieStoreGetOptions> subscriptions;
  subscriptions.ReserveInitialCapacity(backend_result.size());
  for (const auto& backend_subscription : backend_result) {
    CookieStoreGetOptions& subscription = subscriptions.emplace_back();
    ToCookieChangeSubscription(*backend_subscription, subscription);
  }

  resolver->Resolve(std::move(subscriptions));
}

void CookieStore::StartObserving() {
  if (change_listener_binding_ || !backend_)
    return;

  network::mojom::blink::CookieChangeListenerPtr change_listener;
  change_listener_binding_.Bind(mojo::MakeRequest(&change_listener));
  backend_->AddChangeListener(default_cookie_url_, default_site_for_cookies_,
                              std::move(change_listener));
}

void CookieStore::StopObserving() {
  if (!change_listener_binding_.is_bound())
    return;
  change_listener_binding_.Close();
}

}  // namespace blink
