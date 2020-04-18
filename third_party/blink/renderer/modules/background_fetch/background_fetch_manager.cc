// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/modules/background_fetch/background_fetch_manager.h"

#include "third_party/blink/public/platform/modules/serviceworker/web_service_worker_request.h"
#include "third_party/blink/renderer/bindings/core/v8/request_or_usv_string.h"
#include "third_party/blink/renderer/bindings/core/v8/script_promise_resolver.h"
#include "third_party/blink/renderer/bindings/modules/v8/request_or_usv_string_or_request_or_usv_string_sequence.h"
#include "third_party/blink/renderer/core/dom/dom_exception.h"
#include "third_party/blink/renderer/core/dom/exception_code.h"
#include "third_party/blink/renderer/core/fetch/request.h"
#include "third_party/blink/renderer/core/frame/csp/content_security_policy.h"
#include "third_party/blink/renderer/core/frame/deprecation.h"
#include "third_party/blink/renderer/core/frame/use_counter.h"
#include "third_party/blink/renderer/core/loader/mixed_content_checker.h"
#include "third_party/blink/renderer/modules/background_fetch/background_fetch_bridge.h"
#include "third_party/blink/renderer/modules/background_fetch/background_fetch_icon_loader.h"
#include "third_party/blink/renderer/modules/background_fetch/background_fetch_options.h"
#include "third_party/blink/renderer/modules/background_fetch/background_fetch_registration.h"
#include "third_party/blink/renderer/modules/background_fetch/background_fetch_type_converters.h"
#include "third_party/blink/renderer/modules/serviceworkers/service_worker_registration.h"
#include "third_party/blink/renderer/platform/bindings/script_state.h"
#include "third_party/blink/renderer/platform/bindings/v8_throw_exception.h"
#include "third_party/blink/renderer/platform/loader/cors/cors.h"
#include "third_party/blink/renderer/platform/loader/fetch/fetch_utils.h"
#include "third_party/blink/renderer/platform/network/network_utils.h"
#include "third_party/blink/renderer/platform/weborigin/known_ports.h"
#include "third_party/blink/renderer/platform/weborigin/kurl.h"
#include "third_party/blink/renderer/platform/weborigin/security_origin.h"
#include "third_party/skia/include/core/SkBitmap.h"

namespace blink {

namespace {

// Message for the TypeError thrown when an empty request sequence is seen.
const char kEmptyRequestSequenceErrorMessage[] =
    "At least one request must be given.";

// Message for the TypeError thrown when a null request is seen.
const char kNullRequestErrorMessage[] = "Requests must not be null.";

ScriptPromise RejectWithTypeError(ScriptState* script_state,
                                  const KURL& request_url,
                                  const String& reason) {
  return ScriptPromise::Reject(
      script_state, V8ThrowException::CreateTypeError(
                        script_state->GetIsolate(),
                        "Refused to fetch '" + request_url.ElidedString() +
                            "' because " + reason + "."));
}

bool ShouldBlockDueToCSP(ExecutionContext* execution_context,
                         const KURL& request_url) {
  return !ContentSecurityPolicy::ShouldBypassMainWorld(execution_context) &&
         !execution_context->GetContentSecurityPolicy()->AllowConnectToSource(
             request_url);
}

bool ShouldBlockPort(const KURL& request_url) {
  // https://fetch.spec.whatwg.org/#block-bad-port
  return !IsPortAllowedForScheme(request_url);
}

bool ShouldBlockCredentials(ExecutionContext* execution_context,
                            const KURL& request_url) {
  // "If parsedURL includes credentials, then throw a TypeError."
  // https://fetch.spec.whatwg.org/#dom-request
  // (Added by https://github.com/whatwg/fetch/issues/26).
  // "A URL includes credentials if its username or password is not the empty
  // string."
  // https://url.spec.whatwg.org/#include-credentials
  return !request_url.User().IsEmpty() || !request_url.Pass().IsEmpty();
}

bool ShouldBlockScheme(const KURL& request_url) {
  // Require http(s), i.e. block data:, wss: and file:
  // https://github.com/WICG/background-fetch/issues/44
  return !request_url.ProtocolIs(WTF::g_http_atom) &&
         !request_url.ProtocolIs(WTF::g_https_atom);
}

bool ShouldBlockMixedContent(ExecutionContext* execution_context,
                             const KURL& request_url) {
  // TODO(crbug.com/757441): Using MixedContentChecker::ShouldBlockFetch would
  // log better metrics.
  if (MixedContentChecker::IsMixedContent(
          execution_context->GetSecurityOrigin(), request_url)) {
    return true;
  }

  // Normally requests from e.g. http://127.0.0.1 aren't subject to Mixed
  // Content checks even though that is a secure context. Since this is a new
  // API only exposed on secure contexts, be strict pending the discussion in
  // https://groups.google.com/a/chromium.org/d/topic/security-dev/29Ftfgn-w0I/discussion
  // https://w3c.github.io/webappsec-mixed-content/#a-priori-authenticated-url
  if (!SecurityOrigin::Create(request_url)->IsPotentiallyTrustworthy() &&
      !request_url.ProtocolIsData()) {
    return true;
  }

  return false;
}

bool ShouldBlockDanglingMarkup(const KURL& request_url) {
  // "If request's url's potentially-dangling-markup flag is set, and request's
  // url's scheme is an HTTP(S) scheme, then set response to a network error."
  // https://github.com/whatwg/fetch/pull/519
  // https://github.com/whatwg/fetch/issues/546
  return request_url.PotentiallyDanglingMarkup() &&
         request_url.ProtocolIsInHTTPFamily();
}

bool ShouldBlockCORSPreflight(ExecutionContext* execution_context,
                              const WebServiceWorkerRequest& web_request,
                              const KURL& request_url) {
  // Requests that require CORS preflights are temporarily blocked, because the
  // browser side of Background Fetch doesn't yet support performing CORS
  // checks. TODO(crbug.com/711354): Remove this temporary block.

  // Same origin requests don't require a CORS preflight.
  // https://fetch.spec.whatwg.org/#main-fetch
  // TODO(crbug.com/711354): Make sure that cross-origin redirects are disabled.
  bool same_origin =
      execution_context->GetSecurityOrigin()->CanRequest(request_url);
  if (same_origin)
    return false;

  // Requests that are more involved than what is possible with HTML's form
  // element require a CORS-preflight request.
  // https://fetch.spec.whatwg.org/#main-fetch
  if (!CORS::IsCORSSafelistedMethod(web_request.Method()) ||
      !CORS::ContainsOnlyCORSSafelistedHeaders(web_request.Headers())) {
    return true;
  }

  if (RuntimeEnabledFeatures::CorsRFC1918Enabled()) {
    mojom::IPAddressSpace requestor_space =
        execution_context->GetSecurityContext().AddressSpace();

    // TODO(mkwst): This only checks explicit IP addresses. We'll have to move
    // all this up to //net and //content in order to have any real impact on
    // gateway attacks. That turns out to be a TON of work (crbug.com/378566).
    mojom::IPAddressSpace target_space = mojom::IPAddressSpace::kPublic;
    if (NetworkUtils::IsReservedIPAddress(request_url.Host()))
      target_space = mojom::IPAddressSpace::kPrivate;
    if (SecurityOrigin::Create(request_url)->IsLocalhost())
      target_space = mojom::IPAddressSpace::kLocal;

    bool is_external_request = requestor_space > target_space;
    if (is_external_request)
      return true;
  }

  return false;
}

}  // namespace

BackgroundFetchManager::BackgroundFetchManager(
    ServiceWorkerRegistration* registration)
    : ContextLifecycleObserver(registration->GetExecutionContext()),
      registration_(registration),
      loader_(new BackgroundFetchIconLoader()) {
  DCHECK(registration);
  bridge_ = BackgroundFetchBridge::From(registration_);
}

ScriptPromise BackgroundFetchManager::fetch(
    ScriptState* script_state,
    const String& id,
    const RequestOrUSVStringOrRequestOrUSVStringSequence& requests,
    const BackgroundFetchOptions& options,
    ExceptionState& exception_state) {
  if (!registration_->active()) {
    return ScriptPromise::Reject(
        script_state,
        V8ThrowException::CreateTypeError(script_state->GetIsolate(),
                                          "No active registration available on "
                                          "the ServiceWorkerRegistration."));
  }

  Vector<WebServiceWorkerRequest> web_requests =
      CreateWebRequestVector(script_state, requests, exception_state);
  if (exception_state.HadException())
    return ScriptPromise();

  ExecutionContext* execution_context = ExecutionContext::From(script_state);

  // Based on security steps from https://fetch.spec.whatwg.org/#main-fetch
  // TODO(crbug.com/757441): Remove all this duplicative code once Fetch (and
  // all its security checks) are implemented in the Network Service, such that
  // the Download Service in the browser process can use it without having to
  // spin up a renderer process.
  for (const WebServiceWorkerRequest& web_request : web_requests) {
    // TODO(crbug.com/757441): Decide whether to support upgrading requests to
    // potentially secure URLs (https://w3c.github.io/webappsec-upgrade-
    // insecure-requests/) and/or HSTS rewriting. Since this is a new API only
    // exposed on Secure Contexts, and the Mixed Content check below will block
    // any requests to insecure contexts, it'd be cleanest not to support it.
    // Depends how closely compatible with Fetch we want to be. If support is
    // added, make sure to report CSP violations before upgrading the URL.

    KURL request_url(web_request.Url());

    if (!request_url.IsValid()) {
      return RejectWithTypeError(script_state, request_url,
                                 "that URL is invalid");
    }

    // Check this before mixed content, so that if mixed content is blocked by
    // CSP they get a CSP warning rather than a mixed content warning.
    if (ShouldBlockDueToCSP(execution_context, request_url)) {
      return RejectWithTypeError(script_state, request_url,
                                 "it violates the Content Security Policy");
    }

    if (ShouldBlockPort(request_url)) {
      return RejectWithTypeError(script_state, request_url,
                                 "that port is not allowed");
    }

    if (ShouldBlockCredentials(execution_context, request_url)) {
      return RejectWithTypeError(script_state, request_url,
                                 "that URL contains a username/password");
    }

    if (ShouldBlockScheme(request_url)) {
      return RejectWithTypeError(script_state, request_url,
                                 "only the https: scheme is allowed, or http: "
                                 "for loopback IPs");
    }

    // Blocking fetches due to mixed content is done after Content Security
    // Policy to prioritize warnings caused by the latter.
    if (ShouldBlockMixedContent(execution_context, request_url)) {
      return RejectWithTypeError(script_state, request_url,
                                 "it is insecure; use https instead");
    }

    if (ShouldBlockDanglingMarkup(request_url)) {
      return RejectWithTypeError(script_state, request_url,
                                 "it contains dangling markup");
    }

    if (ShouldBlockCORSPreflight(execution_context, web_request, request_url)) {
      return RejectWithTypeError(script_state, request_url,
                                 "CORS preflights are not yet supported "
                                 "by this browser");
    }
  }

  ScriptPromiseResolver* resolver = ScriptPromiseResolver::Create(script_state);
  ScriptPromise promise = resolver->Promise();

  // Load Icons. Right now, we just load the first icon. Lack of icons or
  // inability to load them should not be fatal to the fetch.
  mojom::blink::BackgroundFetchOptionsPtr options_ptr =
      mojom::blink::BackgroundFetchOptions::From(options);
  if (options.icons().size()) {
    loader_->Start(
        bridge_.Get(), execution_context, options.icons(),
        WTF::Bind(&BackgroundFetchManager::DidLoadIcons, WrapPersistent(this),
                  id, WTF::Passed(std::move(web_requests)),
                  std::move(options_ptr), WrapPersistent(resolver)));
    return promise;
  }

  DidLoadIcons(id, std::move(web_requests), std::move(options_ptr),
               WrapPersistent(resolver), SkBitmap());
  return promise;
}

void BackgroundFetchManager::DidLoadIcons(
    const String& id,
    Vector<WebServiceWorkerRequest> web_requests,
    mojom::blink::BackgroundFetchOptionsPtr options,
    ScriptPromiseResolver* resolver,
    const SkBitmap& icon) {
  bridge_->Fetch(id, std::move(web_requests), std::move(options), icon,
                 WTF::Bind(&BackgroundFetchManager::DidFetch,
                           WrapPersistent(this), WrapPersistent(resolver)));
}

void BackgroundFetchManager::DidFetch(
    ScriptPromiseResolver* resolver,
    mojom::blink::BackgroundFetchError error,
    BackgroundFetchRegistration* registration) {
  switch (error) {
    case mojom::blink::BackgroundFetchError::NONE:
      DCHECK(registration);
      resolver->Resolve(registration);
      return;
    case mojom::blink::BackgroundFetchError::DUPLICATED_DEVELOPER_ID:
      DCHECK(!registration);
      resolver->Reject(DOMException::Create(
          kInvalidStateError,
          "There already is a registration for the given id."));
      return;
    case mojom::blink::BackgroundFetchError::STORAGE_ERROR:
      DCHECK(!registration);
      resolver->Reject(DOMException::Create(
          kAbortError, "Failed to store registration due to I/O error."));
      return;
    case mojom::blink::BackgroundFetchError::INVALID_ARGUMENT:
    case mojom::blink::BackgroundFetchError::INVALID_ID:
      // Not applicable for this callback.
      break;
  }

  NOTREACHED();
}

ScriptPromise BackgroundFetchManager::get(ScriptState* script_state,
                                          const String& id) {
  if (!registration_->active()) {
    return ScriptPromise::Reject(
        script_state,
        V8ThrowException::CreateTypeError(script_state->GetIsolate(),
                                          "No active registration available on "
                                          "the ServiceWorkerRegistration."));
  }

  ScriptPromiseResolver* resolver = ScriptPromiseResolver::Create(script_state);
  ScriptPromise promise = resolver->Promise();

  bridge_->GetRegistration(
      id, WTF::Bind(&BackgroundFetchManager::DidGetRegistration,
                    WrapPersistent(this), WrapPersistent(resolver)));

  return promise;
}

// static
Vector<WebServiceWorkerRequest> BackgroundFetchManager::CreateWebRequestVector(
    ScriptState* script_state,
    const RequestOrUSVStringOrRequestOrUSVStringSequence& requests,
    ExceptionState& exception_state) {
  Vector<WebServiceWorkerRequest> web_requests;

  if (requests.IsRequestOrUSVStringSequence()) {
    HeapVector<RequestOrUSVString> request_vector =
        requests.GetAsRequestOrUSVStringSequence();

    // Throw a TypeError when the developer has passed an empty sequence.
    if (!request_vector.size()) {
      exception_state.ThrowTypeError(kEmptyRequestSequenceErrorMessage);
      return Vector<WebServiceWorkerRequest>();
    }

    web_requests.resize(request_vector.size());

    for (size_t i = 0; i < request_vector.size(); ++i) {
      const RequestOrUSVString& request_or_url = request_vector[i];

      Request* request = nullptr;
      if (request_or_url.IsRequest()) {
        request = request_or_url.GetAsRequest();
      } else if (request_or_url.IsUSVString()) {
        request = Request::Create(script_state, request_or_url.GetAsUSVString(),
                                  exception_state);
        if (exception_state.HadException())
          return Vector<WebServiceWorkerRequest>();
      } else {
        exception_state.ThrowTypeError(kNullRequestErrorMessage);
        return Vector<WebServiceWorkerRequest>();
      }

      DCHECK(request);
      request->PopulateWebServiceWorkerRequest(web_requests[i]);
    }
  } else if (requests.IsRequest()) {
    DCHECK(requests.GetAsRequest());
    web_requests.resize(1);
    requests.GetAsRequest()->PopulateWebServiceWorkerRequest(web_requests[0]);
  } else if (requests.IsUSVString()) {
    Request* request = Request::Create(script_state, requests.GetAsUSVString(),
                                       exception_state);
    if (exception_state.HadException())
      return Vector<WebServiceWorkerRequest>();

    DCHECK(request);
    web_requests.resize(1);
    request->PopulateWebServiceWorkerRequest(web_requests[0]);
  } else {
    exception_state.ThrowTypeError(kNullRequestErrorMessage);
    return Vector<WebServiceWorkerRequest>();
  }

  return web_requests;
}

void BackgroundFetchManager::DidGetRegistration(
    ScriptPromiseResolver* resolver,
    mojom::blink::BackgroundFetchError error,
    BackgroundFetchRegistration* registration) {
  switch (error) {
    case mojom::blink::BackgroundFetchError::NONE:
    case mojom::blink::BackgroundFetchError::INVALID_ID:
      resolver->Resolve(registration);
      return;
    case mojom::blink::BackgroundFetchError::STORAGE_ERROR:
      DCHECK(!registration);
      resolver->Reject(DOMException::Create(
          kAbortError, "Failed to get registration due to I/O error."));
      return;
    case mojom::blink::BackgroundFetchError::DUPLICATED_DEVELOPER_ID:
    case mojom::blink::BackgroundFetchError::INVALID_ARGUMENT:
      // Not applicable for this callback.
      break;
  }

  NOTREACHED();
}

ScriptPromise BackgroundFetchManager::getIds(ScriptState* script_state) {
  if (!registration_->active()) {
    return ScriptPromise::Reject(
        script_state,
        V8ThrowException::CreateTypeError(script_state->GetIsolate(),
                                          "No active registration available on "
                                          "the ServiceWorkerRegistration."));
  }

  ScriptPromiseResolver* resolver = ScriptPromiseResolver::Create(script_state);
  ScriptPromise promise = resolver->Promise();

  bridge_->GetDeveloperIds(
      WTF::Bind(&BackgroundFetchManager::DidGetDeveloperIds,
                WrapPersistent(this), WrapPersistent(resolver)));

  return promise;
}

void BackgroundFetchManager::DidGetDeveloperIds(
    ScriptPromiseResolver* resolver,
    mojom::blink::BackgroundFetchError error,
    const Vector<String>& developer_ids) {
  switch (error) {
    case mojom::blink::BackgroundFetchError::NONE:
      resolver->Resolve(developer_ids);
      return;
    case mojom::blink::BackgroundFetchError::STORAGE_ERROR:
      DCHECK(developer_ids.IsEmpty());
      resolver->Reject(DOMException::Create(
          kAbortError, "Failed to get registration IDs due to I/O error."));
      return;
    case mojom::blink::BackgroundFetchError::DUPLICATED_DEVELOPER_ID:
    case mojom::blink::BackgroundFetchError::INVALID_ARGUMENT:
    case mojom::blink::BackgroundFetchError::INVALID_ID:
      // Not applicable for this callback.
      break;
  }

  NOTREACHED();
}

void BackgroundFetchManager::Trace(blink::Visitor* visitor) {
  visitor->Trace(registration_);
  visitor->Trace(bridge_);
  visitor->Trace(loader_);
  ContextLifecycleObserver::Trace(visitor);
  ScriptWrappable::Trace(visitor);
}

void BackgroundFetchManager::ContextDestroyed(ExecutionContext* context) {
  if (loader_) {
    loader_->Stop();
  }
}

}  // namespace blink
