// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/fetch/request.h"

#include "third_party/blink/public/platform/modules/serviceworker/web_service_worker_request.h"
#include "third_party/blink/public/platform/web_url_request.h"
#include "third_party/blink/renderer/bindings/core/v8/dictionary.h"
#include "third_party/blink/renderer/core/dom/abort_signal.h"
#include "third_party/blink/renderer/core/execution_context/execution_context.h"
#include "third_party/blink/renderer/core/fetch/body_stream_buffer.h"
#include "third_party/blink/renderer/core/fetch/fetch_manager.h"
#include "third_party/blink/renderer/core/fetch/request_init.h"
#include "third_party/blink/renderer/core/fileapi/public_url_manager.h"
#include "third_party/blink/renderer/core/loader/threadable_loader.h"
#include "third_party/blink/renderer/platform/bindings/v8_private_property.h"
#include "third_party/blink/renderer/platform/loader/cors/cors.h"
#include "third_party/blink/renderer/platform/loader/fetch/fetch_utils.h"
#include "third_party/blink/renderer/platform/loader/fetch/resource_loader_options.h"
#include "third_party/blink/renderer/platform/loader/fetch/resource_request.h"
#include "third_party/blink/renderer/platform/network/http_names.h"
#include "third_party/blink/renderer/platform/network/http_parsers.h"
#include "third_party/blink/renderer/platform/runtime_enabled_features.h"
#include "third_party/blink/renderer/platform/weborigin/origin_access_entry.h"
#include "third_party/blink/renderer/platform/weborigin/referrer.h"

namespace blink {

FetchRequestData* CreateCopyOfFetchRequestDataForFetch(
    ScriptState* script_state,
    const FetchRequestData* original) {
  FetchRequestData* request = FetchRequestData::Create();
  request->SetURL(original->Url());
  request->SetMethod(original->Method());
  request->SetHeaderList(original->HeaderList()->Clone());
  // FIXME: Set client.
  DOMWrapperWorld& world = script_state->World();
  if (world.IsIsolatedWorld()) {
    request->SetOrigin(world.IsolatedWorldSecurityOrigin());
  } else {
    request->SetOrigin(
        ExecutionContext::From(script_state)->GetSecurityOrigin());
  }
  // FIXME: Set ForceOriginHeaderFlag.
  request->SetSameOriginDataURLFlag(true);
  request->SetReferrer(original->GetReferrer());
  request->SetMode(original->Mode());
  request->SetCredentials(original->Credentials());
  request->SetCacheMode(original->CacheMode());
  request->SetRedirect(original->Redirect());
  request->SetIntegrity(original->Integrity());
  request->SetKeepalive(original->Keepalive());
  if (original->URLLoaderFactory()) {
    network::mojom::blink::URLLoaderFactoryPtr factory_clone;
    original->URLLoaderFactory()->Clone(MakeRequest(&factory_clone));
    request->SetURLLoaderFactory(std::move(factory_clone));
  }
  return request;
}

Request* Request::CreateRequestWithRequestOrString(
    ScriptState* script_state,
    Request* input_request,
    const String& input_string,
    RequestInit& init,
    ExceptionState& exception_state) {
  // - "If |input| is a Request object and it is disturbed, throw a
  //   TypeError."
  if (input_request && input_request->bodyUsed()) {
    exception_state.ThrowTypeError(
        "Cannot construct a Request with a Request object that has already "
        "been used.");
    return nullptr;
  }
  // - "Let |temporaryBody| be |input|'s request's body if |input| is a
  //   Request object, and null otherwise."
  BodyStreamBuffer* temporary_body =
      input_request ? input_request->BodyBuffer() : nullptr;

  // "Let |request| be |input|'s request, if |input| is a Request object,
  // and a new request otherwise."

  auto* execution_context = ExecutionContext::From(script_state);
  scoped_refptr<const SecurityOrigin> origin =
      execution_context->GetSecurityOrigin();

  // "Let |signal| be null."
  AbortSignal* signal = nullptr;

  // TODO(yhirano): Implement the following steps:
  // - "Let |window| be client."
  // - "If |request|'s window is an environment settings object and its
  //   origin is same origin with entry settings object's origin, set
  //   |window| to |request|'s window."
  // - "If |init|'s window member is present and it is not null, throw a
  //   TypeError."
  // - "If |init|'s window member is present, set |window| to no-window."
  //
  // "Set |request| to a new request whose url is |request|'s current url,
  // method is |request|'s method, header list is a copy of |request|'s
  // header list, unsafe-request flag is set, client is entry settings object,
  // window is |window|, origin is "client", omit-Origin-header flag is
  // |request|'s omit-Origin-header flag, same-origin data-URL flag is set,
  // referrer is |request|'s referrer, referrer policy is |request|'s
  // referrer policy, destination is the empty string, mode is |request|'s
  // mode, credentials mode is |request|'s credentials mode, cache mode is
  // |request|'s cache mode, redirect mode is |request|'s redirect mode, and
  // integrity metadata is |request|'s integrity metadata."
  FetchRequestData* request = CreateCopyOfFetchRequestDataForFetch(
      script_state,
      input_request ? input_request->GetRequest() : FetchRequestData::Create());

  if (input_request) {
    // "Set |signal| to input’s signal."
    signal = input_request->signal_;
  }

  // We don't use fallback values. We set these flags directly in below.
  // - "Let |fallbackMode| be null."
  // - "Let |fallbackCredentials| be null."

  // "Let |baseURL| be entry settings object's API base URL."
  const KURL base_url = execution_context->BaseURL();

  // "If |input| is a string, run these substeps:"
  if (!input_request) {
    // "Let |parsedURL| be the result of parsing |input| with |baseURL|."
    KURL parsed_url = KURL(base_url, input_string);
    // "If |parsedURL| is failure, throw a TypeError."
    if (!parsed_url.IsValid()) {
      exception_state.ThrowTypeError("Failed to parse URL from " +
                                     input_string);
      return nullptr;
    }
    //   "If |parsedURL| includes credentials, throw a TypeError."
    if (!parsed_url.User().IsEmpty() || !parsed_url.Pass().IsEmpty()) {
      exception_state.ThrowTypeError(
          "Request cannot be constructed from a URL that includes "
          "credentials: " +
          input_string);
      return nullptr;
    }
    // "Set |request|'s url to |parsedURL| and replace |request|'s url list
    // single URL with a copy of |parsedURL|."
    request->SetURL(parsed_url);

    // Parsing URLs should also resolve blob URLs. This is important because
    // fetching of a blob URL should work even after the URL is revoked as long
    // as the request was created while the URL was still valid.
    if (parsed_url.ProtocolIs("blob") &&
        RuntimeEnabledFeatures::MojoBlobURLsEnabled()) {
      network::mojom::blink::URLLoaderFactoryPtr url_loader_factory;
      ExecutionContext::From(script_state)
          ->GetPublicURLManager()
          .Resolve(parsed_url, MakeRequest(&url_loader_factory));
      request->SetURLLoaderFactory(std::move(url_loader_factory));
    }

    // We don't use fallback values. We set these flags directly in below.
    // - "Set |fallbackMode| to "cors"."
    // - "Set |fallbackCredentials| to "omit"."
  }

  // "If any of |init|'s members are present, run these substeps:"
  if (init.AreAnyMembersSet()) {
    // "If |request|'s |mode| is "navigate", throw a TypeError."
    if (request->Mode() == network::mojom::FetchRequestMode::kNavigate) {
      exception_state.ThrowTypeError(
          "Cannot construct a Request with a Request whose mode is 'navigate' "
          "and a non-empty RequestInit.");
      return nullptr;
    }

    // TODO(yhirano): Implement the following substep:
    //   "Unset |request|'s omit-Origin-header flag."

    // The substep "Set |request|'s referrer to "client"." is performed by
    // the code below as follows:
    // - |init.referrer.referrer| gets initialized by the RequestInit
    //   constructor to "about:client" when any of |options|'s members are
    //   present.
    // - The code below does the equivalent as the step specified in the
    //   spec by processing the "about:client".

    // The substep "Set |request|'s referrer policy to the empty string."
    // is also performed by the code below similarly.
  }

  // The following if-clause performs the following two steps:
  // - "If |init|'s referrer member is present, run these substeps:"
  //   "If |init|'s referrerPolicy member is present, set |request|'s
  //    referrer policy to it."
  //
  // The condition "if any of |init|'s members are present"
  // (areAnyMembersSet) is used for the if-clause instead of conditions
  // indicating presence of each member as specified in the spec. This is to
  // perform the substeps in the previous step together here.
  if (init.AreAnyMembersSet()) {
    // Nothing to do for the step "Let |referrer| be |init|'s referrer
    // member."

    if (init.GetReferrer().referrer.IsEmpty()) {
      // "If |referrer| is the empty string, set |request|'s referrer to
      // "no-referrer" and terminate these substeps."
      request->SetReferrerString(AtomicString(Referrer::NoReferrer()));
    } else {
      // "Let |parsedReferrer| be the result of parsing |referrer| with
      // |baseURL|."
      KURL parsed_referrer(base_url, init.GetReferrer().referrer);
      if (!parsed_referrer.IsValid()) {
        // "If |parsedReferrer| is failure, throw a TypeError."
        exception_state.ThrowTypeError("Referrer '" +
                                       init.GetReferrer().referrer +
                                       "' is not a valid URL.");
        return nullptr;
      }
      if ((parsed_referrer.ProtocolIsAbout() &&
           parsed_referrer.Host().IsEmpty() &&
           parsed_referrer.GetPath() == "client") ||
          !origin->IsSameSchemeHostPort(
              SecurityOrigin::Create(parsed_referrer).get())) {
        // If |parsedReferrer|'s host is empty
        // it's cannot-be-a-base-URL flag must be set

        // "If one of the following conditions is true, then set
        // request’s referrer to "client":
        //
        //     |parsedReferrer|’s cannot-be-a-base-URL flag is set,
        //     scheme is "about", and path contains a single string "client".
        //
        //     parsedReferrer’s origin is not same origin with origin"
        //
        request->SetReferrerString(FetchRequestData::ClientReferrerString());
      } else {
        // "Set |request|'s referrer to |parsedReferrer|."
        request->SetReferrerString(AtomicString(parsed_referrer.GetString()));
      }
    }
    request->SetReferrerPolicy(init.GetReferrer().referrer_policy);
  }

  // The following code performs the following steps:
  // - "Let |mode| be |init|'s mode member if it is present, and
  //   |fallbackMode| otherwise."
  // - "If |mode| is "navigate", throw a TypeError."
  // - "If |mode| is non-null, set |request|'s mode to |mode|."
  if (init.Mode() == "navigate") {
    exception_state.ThrowTypeError(
        "Cannot construct a Request with a RequestInit whose mode member is "
        "set as 'navigate'.");
    return nullptr;
  }
  if (init.Mode() == "same-origin") {
    request->SetMode(network::mojom::FetchRequestMode::kSameOrigin);
  } else if (init.Mode() == "no-cors") {
    request->SetMode(network::mojom::FetchRequestMode::kNoCORS);
  } else if (init.Mode() == "cors") {
    request->SetMode(network::mojom::FetchRequestMode::kCORS);
  } else {
    // |inputRequest| is directly checked here instead of setting and
    // checking |fallbackMode| as specified in the spec.
    if (!input_request)
      request->SetMode(network::mojom::FetchRequestMode::kCORS);
  }

  // "Let |credentials| be |init|'s credentials member if it is present, and
  // |fallbackCredentials| otherwise."
  // "If |credentials| is non-null, set |request|'s credentials mode to
  // |credentials|."

  network::mojom::FetchCredentialsMode credentials_mode;
  if (ParseCredentialsMode(init.Credentials(), &credentials_mode)) {
    request->SetCredentials(credentials_mode);
  } else if (!input_request) {
    request->SetCredentials(network::mojom::FetchCredentialsMode::kSameOrigin);
  }

  // "If |init|'s cache member is present, set |request|'s cache mode to it."
  if (init.CacheMode() == "default") {
    request->SetCacheMode(mojom::FetchCacheMode::kDefault);
  } else if (init.CacheMode() == "no-store") {
    request->SetCacheMode(mojom::FetchCacheMode::kNoStore);
  } else if (init.CacheMode() == "reload") {
    request->SetCacheMode(mojom::FetchCacheMode::kBypassCache);
  } else if (init.CacheMode() == "no-cache") {
    request->SetCacheMode(mojom::FetchCacheMode::kValidateCache);
  } else if (init.CacheMode() == "force-cache") {
    request->SetCacheMode(mojom::FetchCacheMode::kForceCache);
  } else if (init.CacheMode() == "only-if-cached") {
    request->SetCacheMode(mojom::FetchCacheMode::kOnlyIfCached);
  }

  // If |request|’s cache mode is "only-if-cached" and |request|’s mode is not
  // "same-origin", then throw a TypeError.
  if (request->CacheMode() == mojom::FetchCacheMode::kOnlyIfCached &&
      request->Mode() != network::mojom::FetchRequestMode::kSameOrigin) {
    exception_state.ThrowTypeError(
        "'only-if-cached' can be set only with 'same-origin' mode");
    return nullptr;
  }

  // "If |init|'s redirect member is present, set |request|'s redirect mode
  // to it."
  if (init.Redirect() == "follow") {
    request->SetRedirect(network::mojom::FetchRedirectMode::kFollow);
  } else if (init.Redirect() == "error") {
    request->SetRedirect(network::mojom::FetchRedirectMode::kError);
  } else if (init.Redirect() == "manual") {
    request->SetRedirect(network::mojom::FetchRedirectMode::kManual);
  }

  // "If |init|'s integrity member is present, set |request|'s
  // integrity metadata to it."
  if (!init.Integrity().IsNull())
    request->SetIntegrity(init.Integrity());

  if (init.Keepalive().has_value())
    request->SetKeepalive(*init.Keepalive());

  // "If |init|'s method member is present, let |method| be it and run these
  // substeps:"
  if (!init.Method().IsNull()) {
    // "If |method| is not a method or method is a forbidden method, throw
    // a TypeError."
    if (!IsValidHTTPToken(init.Method())) {
      exception_state.ThrowTypeError("'" + init.Method() +
                                     "' is not a valid HTTP method.");
      return nullptr;
    }
    if (FetchUtils::IsForbiddenMethod(init.Method())) {
      exception_state.ThrowTypeError("'" + init.Method() +
                                     "' HTTP method is unsupported.");
      return nullptr;
    }
    // "Normalize |method|."
    // "Set |request|'s method to |method|."
    request->SetMethod(
        FetchUtils::NormalizeMethod(AtomicString(init.Method())));
  }

  // "If |init|'s signal member is present, then set |signal| to it."
  auto init_signal = init.Signal();
  if (init_signal.has_value()) {
    signal = init_signal.value();
  }

  // "Let |r| be a new Request object associated with |request| and a new
  // Headers object whose guard is "request"."
  Request* r = Request::Create(script_state, request);
  // Perform the following steps:
  // - "Let |headers| be a copy of |r|'s Headers object."
  // - "If |init|'s headers member is present, set |headers| to |init|'s
  //   headers member."
  //
  // We don't create a copy of r's Headers object when init's headers member
  // is present.
  Headers* headers = nullptr;
  if (init.GetHeaders().IsNull()) {
    headers = r->getHeaders()->Clone();
  }
  // "Empty |r|'s request's header list."
  r->request_->HeaderList()->ClearList();
  // "If |r|'s request's mode is "no-cors", run these substeps:
  if (r->GetRequest()->Mode() == network::mojom::FetchRequestMode::kNoCORS) {
    // "If |r|'s request's method is not a CORS-safelisted method, throw a
    // TypeError."
    if (!CORS::IsCORSSafelistedMethod(r->GetRequest()->Method())) {
      exception_state.ThrowTypeError("'" + r->GetRequest()->Method() +
                                     "' is unsupported in no-cors mode.");
      return nullptr;
    }
    // "Set |r|'s Headers object's guard to "request-no-cors"."
    r->getHeaders()->SetGuard(Headers::kRequestNoCORSGuard);
  }
  // "If |signal| is not null, then make |r|’s signal follow |signal|."
  if (signal) {
    r->signal_->Follow(signal);
  }
  // "Fill |r|'s Headers object with |headers|. Rethrow any exceptions."
  if (!init.GetHeaders().IsNull()) {
    r->getHeaders()->FillWith(init.GetHeaders(), exception_state);
  } else {
    DCHECK(headers);
    r->getHeaders()->FillWith(headers, exception_state);
  }
  if (exception_state.HadException())
    return nullptr;

  // "If either |init|'s body member is present or |temporaryBody| is
  // non-null, and |request|'s method is `GET` or `HEAD`, throw a TypeError.
  if (init.GetBody() || temporary_body) {
    if (request->Method() == HTTPNames::GET ||
        request->Method() == HTTPNames::HEAD) {
      exception_state.ThrowTypeError(
          "Request with GET/HEAD method cannot have body.");
      return nullptr;
    }
  }

  // "If |init|'s body member is present, run these substeps:"
  if (init.GetBody()) {
    // TODO(yhirano): Throw if keepalive flag is set and body is a
    // ReadableStream. We don't support body stream setting for Request yet.

    // Perform the following steps:
    // - "Let |stream| and |Content-Type| be the result of extracting
    //   |init|'s body member."
    // - "Set |temporaryBody| to |stream|.
    // - "If |Content-Type| is non-null and |r|'s request's header list
    //   contains no header named `Content-Type`, append
    //   `Content-Type`/|Content-Type| to |r|'s Headers object. Rethrow any
    //   exception."
    temporary_body =
        new BodyStreamBuffer(script_state, std::move(init.GetBody()), nullptr);
    if (!init.ContentType().IsEmpty() &&
        !r->getHeaders()->has(HTTPNames::Content_Type, exception_state)) {
      r->getHeaders()->append(HTTPNames::Content_Type, init.ContentType(),
                              exception_state);
    }
    if (exception_state.HadException())
      return nullptr;
  }

  // "Set |r|'s request's body to |temporaryBody|.
  if (temporary_body) {
    r->request_->SetBuffer(temporary_body);
    r->RefreshBody(script_state);
  }

  // "Set |r|'s MIME type to the result of extracting a MIME type from |r|'s
  // request's header list."
  r->request_->SetMIMEType(r->request_->HeaderList()->ExtractMIMEType());

  // "If |input| is a Request object and |input|'s request's body is
  // non-null, run these substeps:"
  if (input_request && input_request->BodyBuffer()) {
    // "Let |dummyStream| be an empty ReadableStream object."
    auto* dummy_stream = new BodyStreamBuffer(
        script_state, BytesConsumer::CreateClosed(), nullptr);
    // "Set |input|'s request's body to a new body whose stream is
    // |dummyStream|."
    input_request->request_->SetBuffer(dummy_stream);
    input_request->RefreshBody(script_state);
    // "Let |reader| be the result of getting reader from |dummyStream|."
    // "Read all bytes from |dummyStream| with |reader|."
    input_request->BodyBuffer()->CloseAndLockAndDisturb();
  }

  // "Return |r|."
  return r;
}

Request* Request::Create(ScriptState* script_state,
                         const RequestInfo& input,
                         const Dictionary& init,
                         ExceptionState& exception_state) {
  DCHECK(!input.IsNull());
  if (input.IsUSVString())
    return Create(script_state, input.GetAsUSVString(), init, exception_state);
  return Create(script_state, input.GetAsRequest(), init, exception_state);
}

Request* Request::Create(ScriptState* script_state,
                         const String& input,
                         ExceptionState& exception_state) {
  return Create(script_state, input, Dictionary(), exception_state);
}

Request* Request::Create(ScriptState* script_state,
                         const String& input,
                         const Dictionary& init,
                         ExceptionState& exception_state) {
  RequestInit request_init(ExecutionContext::From(script_state), init,
                           exception_state);
  return CreateRequestWithRequestOrString(script_state, nullptr, input,
                                          request_init, exception_state);
}

Request* Request::Create(ScriptState* script_state,
                         Request* input,
                         ExceptionState& exception_state) {
  return Create(script_state, input, Dictionary(), exception_state);
}

Request* Request::Create(ScriptState* script_state,
                         Request* input,
                         const Dictionary& init,
                         ExceptionState& exception_state) {
  RequestInit request_init(ExecutionContext::From(script_state), init,
                           exception_state);
  return CreateRequestWithRequestOrString(script_state, input, String(),
                                          request_init, exception_state);
}

Request* Request::Create(ScriptState* script_state, FetchRequestData* request) {
  return new Request(script_state, request);
}

Request* Request::Create(ScriptState* script_state,
                         const WebServiceWorkerRequest& web_request) {
  FetchRequestData* request =
      FetchRequestData::Create(script_state, web_request);
  return new Request(script_state, request);
}

bool Request::ParseCredentialsMode(
    const String& credentials_mode,
    network::mojom::FetchCredentialsMode* result) {
  if (credentials_mode == "omit") {
    *result = network::mojom::FetchCredentialsMode::kOmit;
    return true;
  }
  if (credentials_mode == "same-origin") {
    *result = network::mojom::FetchCredentialsMode::kSameOrigin;
    return true;
  }
  if (credentials_mode == "include") {
    *result = network::mojom::FetchCredentialsMode::kInclude;
    return true;
  }
  return false;
}

Request::Request(ScriptState* script_state,
                 FetchRequestData* request,
                 Headers* headers,
                 AbortSignal* signal)
    : Body(ExecutionContext::From(script_state)),
      request_(request),
      headers_(headers),
      signal_(signal) {
  RefreshBody(script_state);
}

Request::Request(ScriptState* script_state, FetchRequestData* request)
    : Request(script_state,
              request,
              Headers::Create(request->HeaderList()),
              new AbortSignal(ExecutionContext::From(script_state))) {
  headers_->SetGuard(Headers::kRequestGuard);
}

String Request::method() const {
  // "The method attribute's getter must return request's method."
  return request_->Method();
}

KURL Request::url() const {
  return request_->Url();
}

String Request::destination() const {
  // "The destination attribute’s getter must return request’s destination."
  switch (request_->Context()) {
    case WebURLRequest::kRequestContextUnspecified:
    case WebURLRequest::kRequestContextBeacon:
    case WebURLRequest::kRequestContextDownload:
    case WebURLRequest::kRequestContextEventSource:
    case WebURLRequest::kRequestContextFetch:
    case WebURLRequest::kRequestContextPing:
    case WebURLRequest::kRequestContextXMLHttpRequest:
    case WebURLRequest::kRequestContextSubresource:
    case WebURLRequest::kRequestContextPrefetch:
      return "";
    case WebURLRequest::kRequestContextCSPReport:
      return "report";
    case WebURLRequest::kRequestContextAudio:
      return "audio";
    case WebURLRequest::kRequestContextEmbed:
      return "embed";
    case WebURLRequest::kRequestContextFont:
      return "font";
    case WebURLRequest::kRequestContextFrame:
    case WebURLRequest::kRequestContextHyperlink:
    case WebURLRequest::kRequestContextIframe:
    case WebURLRequest::kRequestContextLocation:
    case WebURLRequest::kRequestContextForm:
      return "document";
    case WebURLRequest::kRequestContextImage:
    case WebURLRequest::kRequestContextFavicon:
    case WebURLRequest::kRequestContextImageSet:
      return "image";
    case WebURLRequest::kRequestContextManifest:
      return "manifest";
    case WebURLRequest::kRequestContextObject:
      return "object";
    case WebURLRequest::kRequestContextScript:
      return "script";
    case WebURLRequest::kRequestContextSharedWorker:
      return "sharedworker";
    case WebURLRequest::kRequestContextStyle:
      return "style";
    case WebURLRequest::kRequestContextTrack:
      return "track";
    case WebURLRequest::kRequestContextVideo:
      return "video";
    case WebURLRequest::kRequestContextWorker:
      return "worker";
    case WebURLRequest::kRequestContextXSLT:
      return "xslt";
    case WebURLRequest::kRequestContextImport:
    case WebURLRequest::kRequestContextInternal:
    case WebURLRequest::kRequestContextPlugin:
    case WebURLRequest::kRequestContextServiceWorker:
      return "unknown";
  }
  NOTREACHED();
  return "";
}

String Request::referrer() const {
  // "The referrer attribute's getter must return the empty string if
  // request's referrer is no referrer, "about:client" if request's referrer
  // is client and request's referrer, serialized, otherwise."
  DCHECK_EQ(FetchRequestData::NoReferrerString(), AtomicString());
  DCHECK_EQ(FetchRequestData::ClientReferrerString(),
            AtomicString("about:client"));
  return request_->ReferrerString();
}

String Request::getReferrerPolicy() const {
  switch (request_->GetReferrerPolicy()) {
    case kReferrerPolicyAlways:
      return "unsafe-url";
    case kReferrerPolicyDefault:
      return "";
    case kReferrerPolicyNoReferrerWhenDowngrade:
      return "no-referrer-when-downgrade";
    case kReferrerPolicyNever:
      return "no-referrer";
    case kReferrerPolicyOrigin:
      return "origin";
    case kReferrerPolicyOriginWhenCrossOrigin:
      return "origin-when-cross-origin";
    case kReferrerPolicySameOrigin:
      return "same-origin";
    case kReferrerPolicyStrictOrigin:
      return "strict-origin";
    case kReferrerPolicyStrictOriginWhenCrossOrigin:
      return "strict-origin-when-cross-origin";
  }
  NOTREACHED();
  return String();
}

String Request::mode() const {
  // "The mode attribute's getter must return the value corresponding to the
  // first matching statement, switching on request's mode:"
  switch (request_->Mode()) {
    case network::mojom::FetchRequestMode::kSameOrigin:
      return "same-origin";
    case network::mojom::FetchRequestMode::kNoCORS:
      return "no-cors";
    case network::mojom::FetchRequestMode::kCORS:
    case network::mojom::FetchRequestMode::kCORSWithForcedPreflight:
      return "cors";
    case network::mojom::FetchRequestMode::kNavigate:
      return "navigate";
  }
  NOTREACHED();
  return "";
}

String Request::credentials() const {
  // "The credentials attribute's getter must return the value corresponding
  // to the first matching statement, switching on request's credentials
  // mode:"
  switch (request_->Credentials()) {
    case network::mojom::FetchCredentialsMode::kOmit:
      return "omit";
    case network::mojom::FetchCredentialsMode::kSameOrigin:
      return "same-origin";
    case network::mojom::FetchCredentialsMode::kInclude:
      return "include";
  }
  NOTREACHED();
  return "";
}

String Request::cache() const {
  // "The cache attribute's getter must return request's cache mode."
  switch (request_->CacheMode()) {
    case mojom::FetchCacheMode::kDefault:
      return "default";
    case mojom::FetchCacheMode::kNoStore:
      return "no-store";
    case mojom::FetchCacheMode::kBypassCache:
      return "reload";
    case mojom::FetchCacheMode::kValidateCache:
      return "no-cache";
    case mojom::FetchCacheMode::kForceCache:
      return "force-cache";
    case mojom::FetchCacheMode::kOnlyIfCached:
      return "only-if-cached";
    case mojom::FetchCacheMode::kUnspecifiedOnlyIfCachedStrict:
    case mojom::FetchCacheMode::kUnspecifiedForceCacheMiss:
      NOTREACHED();
      break;
  }
  NOTREACHED();
  return "";
}

String Request::redirect() const {
  // "The redirect attribute's getter must return request's redirect mode."
  switch (request_->Redirect()) {
    case network::mojom::FetchRedirectMode::kFollow:
      return "follow";
    case network::mojom::FetchRedirectMode::kError:
      return "error";
    case network::mojom::FetchRedirectMode::kManual:
      return "manual";
  }
  NOTREACHED();
  return "";
}

String Request::integrity() const {
  return request_->Integrity();
}

bool Request::keepalive() const {
  return request_->Keepalive();
}

Request* Request::clone(ScriptState* script_state,
                        ExceptionState& exception_state) {
  if (IsBodyLocked() || bodyUsed()) {
    exception_state.ThrowTypeError("Request body is already used");
    return nullptr;
  }

  FetchRequestData* request = request_->Clone(script_state);
  RefreshBody(script_state);
  Headers* headers = Headers::Create(request->HeaderList());
  headers->SetGuard(headers_->GetGuard());
  auto* signal = new AbortSignal(ExecutionContext::From(script_state));
  signal->Follow(signal_);
  return new Request(script_state, request, headers, signal);
}

FetchRequestData* Request::PassRequestData(ScriptState* script_state) {
  DCHECK(!bodyUsed());
  FetchRequestData* data = request_->Pass(script_state);
  RefreshBody(script_state);
  // |data|'s buffer('s js wrapper) has no retainer, but it's OK because
  // the only caller is the fetch function and it uses the body buffer
  // immediately.
  return data;
}

bool Request::HasBody() const {
  return BodyBuffer();
}

void Request::PopulateWebServiceWorkerRequest(
    WebServiceWorkerRequest& web_request) const {
  web_request.SetMethod(method());
  web_request.SetMode(request_->Mode());
  web_request.SetCredentialsMode(request_->Credentials());
  web_request.SetCacheMode(request_->CacheMode());
  web_request.SetRedirectMode(request_->Redirect());
  web_request.SetIntegrity(request_->Integrity());
  web_request.SetRequestContext(request_->Context());

  // Strip off the fragment part of URL. So far, all users of
  // WebServiceWorkerRequest expect the fragment to be excluded.
  KURL url(request_->Url());
  if (request_->Url().HasFragmentIdentifier())
    url.RemoveFragmentIdentifier();
  web_request.SetURL(url);

  const FetchHeaderList* header_list = headers_->HeaderList();
  for (const auto& header : header_list->List()) {
    web_request.AppendHeader(header.first, header.second);
  }

  web_request.SetReferrer(
      request_->ReferrerString(),
      static_cast<WebReferrerPolicy>(request_->GetReferrerPolicy()));
  // FIXME: How can we set isReload properly? What is the correct place to load
  // it in to the Request object? We should investigate the right way to plumb
  // this information in to here.
}

String Request::MimeType() const {
  return request_->MimeType();
}

String Request::ContentType() const {
  String result;
  request_->HeaderList()->Get(HTTPNames::Content_Type, result);
  return result;
}

void Request::RefreshBody(ScriptState* script_state) {
  v8::Local<v8::Value> request = ToV8(this, script_state);
  if (request.IsEmpty()) {
    // |toV8| can return an empty handle when the worker is terminating.
    // We don't want the renderer to crash in such cases.
    // TODO(yhirano): Delete this block after the graceful shutdown
    // mechanism is introduced.
    return;
  }
  DCHECK(request->IsObject());
  v8::Local<v8::Value> body_buffer = ToV8(this->BodyBuffer(), script_state);
  V8PrivateProperty::GetInternalBodyBuffer(script_state->GetIsolate())
      .Set(request.As<v8::Object>(), body_buffer);
}

void Request::Trace(blink::Visitor* visitor) {
  Body::Trace(visitor);
  visitor->Trace(request_);
  visitor->Trace(headers_);
  visitor->Trace(signal_);
}

}  // namespace blink
