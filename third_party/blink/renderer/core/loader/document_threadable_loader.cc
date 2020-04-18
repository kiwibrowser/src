/*
 * Copyright (C) 2011, 2012 Google Inc. All rights reserved.
 * Copyright (C) 2013, Intel Corporation
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "third_party/blink/renderer/core/loader/document_threadable_loader.h"

#include <memory>
#include "base/memory/weak_ptr.h"
#include "base/single_thread_task_runner.h"
#include "services/network/public/mojom/cors.mojom-blink.h"
#include "services/network/public/mojom/fetch_api.mojom-blink.h"
#include "third_party/blink/public/platform/platform.h"
#include "third_party/blink/public/platform/task_type.h"
#include "third_party/blink/public/platform/web_cors.h"
#include "third_party/blink/public/platform/web_security_origin.h"
#include "third_party/blink/public/platform/web_url_request.h"
#include "third_party/blink/renderer/core/dom/document.h"
#include "third_party/blink/renderer/core/frame/frame_console.h"
#include "third_party/blink/renderer/core/frame/local_frame.h"
#include "third_party/blink/renderer/core/frame/local_frame_client.h"
#include "third_party/blink/renderer/core/frame/web_feature.h"
#include "third_party/blink/renderer/core/inspector/console_message.h"
#include "third_party/blink/renderer/core/inspector/inspector_network_agent.h"
#include "third_party/blink/renderer/core/inspector/inspector_trace_events.h"
#include "third_party/blink/renderer/core/loader/base_fetch_context.h"
#include "third_party/blink/renderer/core/loader/document_threadable_loader_client.h"
#include "third_party/blink/renderer/core/loader/frame_loader.h"
#include "third_party/blink/renderer/core/loader/threadable_loader_client.h"
#include "third_party/blink/renderer/core/loader/threadable_loading_context.h"
#include "third_party/blink/renderer/core/probe/core_probes.h"
#include "third_party/blink/renderer/platform/exported/wrapped_resource_request.h"
#include "third_party/blink/renderer/platform/loader/cors/cors.h"
#include "third_party/blink/renderer/platform/loader/cors/cors_error_string.h"
#include "third_party/blink/renderer/platform/loader/fetch/fetch_parameters.h"
#include "third_party/blink/renderer/platform/loader/fetch/resource.h"
#include "third_party/blink/renderer/platform/loader/fetch/resource_fetcher.h"
#include "third_party/blink/renderer/platform/loader/fetch/resource_loader.h"
#include "third_party/blink/renderer/platform/loader/fetch/resource_loader_options.h"
#include "third_party/blink/renderer/platform/loader/fetch/resource_request.h"
#include "third_party/blink/renderer/platform/shared_buffer.h"
#include "third_party/blink/renderer/platform/weborigin/scheme_registry.h"
#include "third_party/blink/renderer/platform/weborigin/security_origin.h"
#include "third_party/blink/renderer/platform/weborigin/security_policy.h"
#include "third_party/blink/renderer/platform/wtf/assertions.h"

namespace blink {

namespace {

// Fetch API Spec: https://fetch.spec.whatwg.org/#cors-preflight-fetch-0
AtomicString CreateAccessControlRequestHeadersHeader(
    const HTTPHeaderMap& headers) {
  Vector<String> filtered_headers;
  for (const auto& header : headers) {
    // Exclude CORS-safelisted headers.
    if (CORS::IsCORSSafelistedHeader(header.key, header.value))
      continue;
    // Calling a deprecated function, but eventually this function,
    // |CreateAccessControlRequestHeadersHeader| will be removed.
    // When the request is from a Worker, referrer header was added by
    // WorkerThreadableLoader. But it should not be added to
    // Access-Control-Request-Headers header.
    if (DeprecatedEqualIgnoringCase(header.key, "referer"))
      continue;
    filtered_headers.push_back(header.key.DeprecatedLower());
  }
  if (!filtered_headers.size())
    return g_null_atom;

  // Sort header names lexicographically.
  std::sort(filtered_headers.begin(), filtered_headers.end(),
            WTF::CodePointCompareLessThan);
  StringBuilder header_buffer;
  for (const String& header : filtered_headers) {
    if (!header_buffer.IsEmpty())
      header_buffer.Append(",");
    header_buffer.Append(header);
  }

  return header_buffer.ToAtomicString();
}

class EmptyDataHandle final : public WebDataConsumerHandle {
 private:
  class EmptyDataReader final : public WebDataConsumerHandle::Reader {
   public:
    explicit EmptyDataReader(
        WebDataConsumerHandle::Client* client,
        scoped_refptr<base::SingleThreadTaskRunner> task_runner)
        : factory_(this) {
      task_runner->PostTask(
          FROM_HERE, WTF::Bind(&EmptyDataReader::Notify, factory_.GetWeakPtr(),
                               WTF::Unretained(client)));
    }

   private:
    Result BeginRead(const void** buffer,
                     WebDataConsumerHandle::Flags,
                     size_t* available) override {
      *available = 0;
      *buffer = nullptr;
      return kDone;
    }
    Result EndRead(size_t) override {
      return WebDataConsumerHandle::kUnexpectedError;
    }
    void Notify(WebDataConsumerHandle::Client* client) {
      client->DidGetReadable();
    }
    base::WeakPtrFactory<EmptyDataReader> factory_;
  };

  std::unique_ptr<Reader> ObtainReader(
      Client* client,
      scoped_refptr<base::SingleThreadTaskRunner> task_runner) override {
    return std::make_unique<EmptyDataReader>(client, std::move(task_runner));
  }
  const char* DebugName() const override { return "EmptyDataHandle"; }
};

}  // namespace

// DetachedClient is a ThreadableLoaderClient for a "detached"
// DocumentThreadableLoader. It's for fetch requests with keepalive set, so
// it keeps itself alive during loading.
class DocumentThreadableLoader::DetachedClient final
    : public GarbageCollectedFinalized<DetachedClient>,
      public ThreadableLoaderClient {
 public:
  explicit DetachedClient(DocumentThreadableLoader* loader)
      : self_keep_alive_(this), loader_(loader) {}
  ~DetachedClient() override {}

  void DidFinishLoading(unsigned long identifier) override {
    self_keep_alive_.Clear();
  }
  void DidFail(const ResourceError&) override { self_keep_alive_.Clear(); }
  void DidFailRedirectCheck() override { self_keep_alive_.Clear(); }
  void Trace(Visitor* visitor) { visitor->Trace(loader_); }

 private:
  SelfKeepAlive<DetachedClient> self_keep_alive_;
  // Keep it alive.
  const Member<DocumentThreadableLoader> loader_;
};

// Max number of CORS redirects handled in DocumentThreadableLoader. Same number
// as net/url_request/url_request.cc, and same number as
// https://fetch.spec.whatwg.org/#concept-http-fetch, Step 4.
// FIXME: currently the number of redirects is counted and limited here and in
// net/url_request/url_request.cc separately.
static const int kMaxCORSRedirects = 20;

// static
void DocumentThreadableLoader::LoadResourceSynchronously(
    ThreadableLoadingContext& loading_context,
    const ResourceRequest& request,
    ThreadableLoaderClient& client,
    const ThreadableLoaderOptions& options,
    const ResourceLoaderOptions& resource_loader_options) {
  (new DocumentThreadableLoader(loading_context, &client, kLoadSynchronously,
                                options, resource_loader_options))
      ->Start(request);
}

// static
std::unique_ptr<ResourceRequest>
DocumentThreadableLoader::CreateAccessControlPreflightRequest(
    const ResourceRequest& request,
    const SecurityOrigin* origin) {
  const KURL& request_url = request.Url();

  DCHECK(request_url.User().IsEmpty());
  DCHECK(request_url.Pass().IsEmpty());

  std::unique_ptr<ResourceRequest> preflight_request =
      std::make_unique<ResourceRequest>(request_url);
  preflight_request->SetHTTPMethod(HTTPNames::OPTIONS);
  preflight_request->SetHTTPHeaderField(
      HTTPNames::Access_Control_Request_Method, request.HttpMethod());
  preflight_request->SetPriority(request.Priority());
  preflight_request->SetRequestContext(request.GetRequestContext());
  preflight_request->SetFetchCredentialsMode(
      network::mojom::FetchCredentialsMode::kOmit);
  preflight_request->SetSkipServiceWorker(true);
  preflight_request->SetHTTPReferrer(
      Referrer(request.HttpReferrer(), request.GetReferrerPolicy()));

  if (request.IsExternalRequest()) {
    preflight_request->SetHTTPHeaderField(
        HTTPNames::Access_Control_Request_External, "true");
  }

  const AtomicString request_headers =
      CreateAccessControlRequestHeadersHeader(request.HttpHeaderFields());
  if (request_headers != g_null_atom) {
    preflight_request->SetHTTPHeaderField(
        HTTPNames::Access_Control_Request_Headers, request_headers);
  }

  if (origin)
    preflight_request->SetHTTPOrigin(origin);

  return preflight_request;
}

// static
std::unique_ptr<ResourceRequest>
DocumentThreadableLoader::CreateAccessControlPreflightRequestForTesting(
    const ResourceRequest& request) {
  return CreateAccessControlPreflightRequest(request, nullptr);
}

// static
DocumentThreadableLoader* DocumentThreadableLoader::Create(
    ThreadableLoadingContext& loading_context,
    ThreadableLoaderClient* client,
    const ThreadableLoaderOptions& options,
    const ResourceLoaderOptions& resource_loader_options) {
  return new DocumentThreadableLoader(loading_context, client,
                                      kLoadAsynchronously, options,
                                      resource_loader_options);
}

DocumentThreadableLoader::DocumentThreadableLoader(
    ThreadableLoadingContext& loading_context,
    ThreadableLoaderClient* client,
    BlockingBehavior blocking_behavior,
    const ThreadableLoaderOptions& options,
    const ResourceLoaderOptions& resource_loader_options)
    : client_(client),
      loading_context_(&loading_context),
      options_(options),
      resource_loader_options_(resource_loader_options),
      out_of_blink_cors_(RuntimeEnabledFeatures::OutOfBlinkCORSEnabled()),
      cors_flag_(false),
      security_origin_(resource_loader_options_.security_origin),
      is_using_data_consumer_handle_(false),
      async_(blocking_behavior == kLoadAsynchronously),
      request_context_(WebURLRequest::kRequestContextUnspecified),
      fetch_request_mode_(network::mojom::FetchRequestMode::kSameOrigin),
      fetch_credentials_mode_(network::mojom::FetchCredentialsMode::kOmit),
      timeout_timer_(
          GetExecutionContext()->GetTaskRunner(TaskType::kNetworking),
          this,
          &DocumentThreadableLoader::DidTimeout),
      request_started_seconds_(0.0),
      cors_redirect_limit_(0),
      redirect_mode_(network::mojom::FetchRedirectMode::kFollow),
      override_referrer_(false) {
  DCHECK(client);
}

void DocumentThreadableLoader::Start(const ResourceRequest& request) {
  // Setting an outgoing referer is only supported in the async code path.
  DCHECK(async_ || request.HttpReferrer().IsEmpty());

  bool cors_enabled =
      CORS::IsCORSEnabledRequestMode(request.GetFetchRequestMode());

  // kPreventPreflight can be used only when the CORS is enabled.
  DCHECK(request.CORSPreflightPolicy() ==
             network::mojom::CORSPreflightPolicy::kConsiderPreflight ||
         cors_enabled);

  if (cors_enabled)
    cors_redirect_limit_ = kMaxCORSRedirects;

  request_context_ = request.GetRequestContext();
  fetch_request_mode_ = request.GetFetchRequestMode();
  fetch_credentials_mode_ = request.GetFetchCredentialsMode();
  redirect_mode_ = request.GetFetchRedirectMode();

  if (request.GetFetchRequestMode() ==
      network::mojom::FetchRequestMode::kNoCORS) {
    SECURITY_CHECK(WebCORS::IsNoCORSAllowedContext(request_context_));
  } else {
    cors_flag_ = !GetSecurityOrigin()->CanRequest(request.Url());
  }

  // The CORS flag variable is not yet used at the step in the spec that
  // corresponds to this line, but divert |cors_flag_| here for convenience.
  if (cors_flag_ && request.GetFetchRequestMode() ==
                        network::mojom::FetchRequestMode::kSameOrigin) {
    probe::documentThreadableLoaderFailedToStartLoadingForClient(
        GetExecutionContext(), client_);
    ThreadableLoaderClient* client = client_;
    Clear();
    ResourceError error = ResourceError::CancelledDueToAccessCheckError(
        request.Url(), ResourceRequestBlockedReason::kOther,
        CORS::GetErrorString(
            CORS::ErrorParameter::CreateForDisallowedByMode(request.Url())));
    GetExecutionContext()->AddConsoleMessage(ConsoleMessage::Create(
        kJSMessageSource, kErrorMessageLevel, error.LocalizedDescription()));
    client->DidFail(error);
    return;
  }

  request_started_seconds_ = CurrentTimeTicksInSeconds();

  // Save any headers on the request here. If this request redirects
  // cross-origin, we cancel the old request create a new one, and copy these
  // headers.
  request_headers_ = request.HttpHeaderFields();

  ResourceRequest new_request(request);

  // Set the service worker mode to none if "bypass for network" in DevTools is
  // enabled.
  bool should_bypass_service_worker = false;
  probe::shouldBypassServiceWorker(GetExecutionContext(),
                                   &should_bypass_service_worker);
  if (should_bypass_service_worker)
    new_request.SetSkipServiceWorker(true);

  // Process the CORS protocol inside the DocumentThreadableLoader for the
  // following cases:
  //
  // - When the request is sync or the protocol is unsupported since we can
  //   assume that any service worker (SW) is skipped for such requests by
  //   content/ code.
  // - When |skip_service_worker| is true, any SW will be skipped.
  // - If we're not yet controlled by a SW, then we're sure that this
  //   request won't be intercepted by a SW. In case we end up with
  //   sending a CORS preflight request, the actual request to be sent later
  //   may be intercepted. This is taken care of in LoadPreflightRequest() by
  //   setting |skip_service_worker| to true.
  //
  // From the above analysis, you can see that the request can never be
  // intercepted by a SW inside this if-block. It's because:
  // - |skip_service_worker| needs to be false, and
  // - we're controlled by a SW at this point
  // to allow a SW to intercept the request. Even when the request gets issued
  // asynchronously after performing the CORS preflight, it doesn't get
  // intercepted since LoadPreflightRequest() sets the flag to kNone in advance.
  if (!async_ || new_request.GetSkipServiceWorker() ||
      !SchemeRegistry::ShouldTreatURLSchemeAsAllowingServiceWorkers(
          new_request.Url().Protocol()) ||
      !loading_context_->GetResourceFetcher()->IsControlledByServiceWorker()) {
    DispatchInitialRequest(new_request);
    return;
  }

  if (CORS::IsCORSEnabledRequestMode(request.GetFetchRequestMode())) {
    // Save the request to fallback_request_for_service_worker to use when the
    // service worker doesn't handle (call respondWith()) a CORS enabled
    // request.
    fallback_request_for_service_worker_ = ResourceRequest(request);
    // Skip the service worker for the fallback request.
    fallback_request_for_service_worker_.SetSkipServiceWorker(true);
  }

  LoadRequest(new_request, resource_loader_options_);
}

void DocumentThreadableLoader::DispatchInitialRequest(
    ResourceRequest& request) {
  if (out_of_blink_cors_ || (!request.IsExternalRequest() && !cors_flag_)) {
    LoadRequest(request, resource_loader_options_);
    return;
  }

  DCHECK(CORS::IsCORSEnabledRequestMode(request.GetFetchRequestMode()) ||
         request.IsExternalRequest());

  MakeCrossOriginAccessRequest(request);
}

void DocumentThreadableLoader::PrepareCrossOriginRequest(
    ResourceRequest& request) const {
  if (GetSecurityOrigin())
    request.SetHTTPOrigin(GetSecurityOrigin());
  if (override_referrer_)
    request.SetHTTPReferrer(referrer_after_redirect_);
}

void DocumentThreadableLoader::LoadPreflightRequest(
    const ResourceRequest& actual_request,
    const ResourceLoaderOptions& actual_options) {
  std::unique_ptr<ResourceRequest> preflight_request =
      CreateAccessControlPreflightRequest(actual_request, GetSecurityOrigin());

  actual_request_ = actual_request;
  actual_options_ = actual_options;

  // Explicitly set |skip_service_worker| to true here. Although the page is
  // not controlled by a SW at this point, a new SW may be controlling the
  // page when this actual request gets sent later. We should not send the
  // actual request to the SW. See https://crbug.com/604583.
  actual_request_.SetSkipServiceWorker(true);

  // Create a ResourceLoaderOptions for preflight.
  ResourceLoaderOptions preflight_options = actual_options;

  LoadRequest(*preflight_request, preflight_options);
}

void DocumentThreadableLoader::MakeCrossOriginAccessRequest(
    const ResourceRequest& request) {
  DCHECK(CORS::IsCORSEnabledRequestMode(request.GetFetchRequestMode()) ||
         request.IsExternalRequest());
  DCHECK(client_);
  DCHECK(!GetResource());

  // Cross-origin requests are only allowed certain registered schemes. We would
  // catch this when checking response headers later, but there is no reason to
  // send a request, preflighted or not, that's guaranteed to be denied.
  if (!SchemeRegistry::ShouldTreatURLSchemeAsCORSEnabled(
          request.Url().Protocol())) {
    probe::documentThreadableLoaderFailedToStartLoadingForClient(
        GetExecutionContext(), client_);
    DispatchDidFailAccessControlCheck(
        ResourceError::CancelledDueToAccessCheckError(
            request.Url(), ResourceRequestBlockedReason::kOther,
            String::Format(
                "Cross origin requests are only supported for protocol "
                "schemes: %s.",
                SchemeRegistry::ListOfCORSEnabledURLSchemes().Ascii().data())));
    return;
  }

  // Non-secure origins may not make "external requests":
  // https://wicg.github.io/cors-rfc1918/#integration-fetch
  String error_message;
  if (!GetExecutionContext()->IsSecureContext(error_message) &&
      request.IsExternalRequest()) {
    DispatchDidFailAccessControlCheck(
        ResourceError::CancelledDueToAccessCheckError(
            request.Url(), ResourceRequestBlockedReason::kOrigin,
            "Requests to internal network resources are not allowed "
            "from non-secure contexts (see https://goo.gl/Y0ZkNV). "
            "This is an experimental restriction which is part of "
            "'https://mikewest.github.io/cors-rfc1918/'."));
    return;
  }

  ResourceRequest cross_origin_request(request);
  ResourceLoaderOptions cross_origin_options(resource_loader_options_);

  cross_origin_request.RemoveUserAndPassFromURL();

  // Enforce the CORS preflight for checking the Access-Control-Allow-External
  // header. The CORS preflight cache doesn't help for this purpose.
  if (request.IsExternalRequest()) {
    LoadPreflightRequest(cross_origin_request, cross_origin_options);
    return;
  }

  if (request.GetFetchRequestMode() !=
      network::mojom::FetchRequestMode::kCORSWithForcedPreflight) {
    if (request.CORSPreflightPolicy() ==
        network::mojom::CORSPreflightPolicy::kPreventPreflight) {
      PrepareCrossOriginRequest(cross_origin_request);
      LoadRequest(cross_origin_request, cross_origin_options);
      return;
    }

    DCHECK_EQ(request.CORSPreflightPolicy(),
              network::mojom::CORSPreflightPolicy::kConsiderPreflight);

    // We use ContainsOnlyCORSSafelistedOrForbiddenHeaders() here since
    // |request| may have been modified in the process of loading (not from
    // the user's input). For example, referrer. We need to accept them. For
    // security, we must reject forbidden headers/methods at the point we
    // accept user's input. Not here.
    if (CORS::IsCORSSafelistedMethod(request.HttpMethod()) &&
        CORS::ContainsOnlyCORSSafelistedOrForbiddenHeaders(
            request.HttpHeaderFields())) {
      PrepareCrossOriginRequest(cross_origin_request);
      LoadRequest(cross_origin_request, cross_origin_options);
      return;
    }
  }

  // Now, we need to check that the request passes the CORS preflight either by
  // issuing a CORS preflight or based on an entry in the CORS preflight cache.

  bool should_ignore_preflight_cache = false;
  // Prevent use of the CORS preflight cache when instructed by the DevTools
  // not to use caches.
  probe::shouldForceCORSPreflight(GetExecutionContext(),
                                  &should_ignore_preflight_cache);
  if (should_ignore_preflight_cache ||
      !CORS::CheckIfRequestCanSkipPreflight(
          GetSecurityOrigin()->ToString(), cross_origin_request.Url(),
          cross_origin_request.GetFetchCredentialsMode(),
          cross_origin_request.HttpMethod(),
          cross_origin_request.HttpHeaderFields())) {
    LoadPreflightRequest(cross_origin_request, cross_origin_options);
    return;
  }

  // We don't want any requests that could involve a CORS preflight to get
  // intercepted by a foreign SW, even if we have the result of the preflight
  // cached already. See https://crbug.com/674370.
  cross_origin_request.SetSkipServiceWorker(true);

  PrepareCrossOriginRequest(cross_origin_request);
  LoadRequest(cross_origin_request, cross_origin_options);
}

DocumentThreadableLoader::~DocumentThreadableLoader() {
  CHECK(!client_);
  DCHECK(!GetResource());
}

void DocumentThreadableLoader::OverrideTimeout(
    unsigned long timeout_milliseconds) {
  DCHECK(async_);

  // |m_requestStartedSeconds| == 0.0 indicates loading is already finished and
  // |m_timeoutTimer| is already stopped, and thus we do nothing for such cases.
  // See https://crbug.com/551663 for details.
  if (request_started_seconds_ <= 0.0)
    return;

  timeout_timer_.Stop();
  // At the time of this method's implementation, it is only ever called by
  // XMLHttpRequest, when the timeout attribute is set after sending the
  // request.
  //
  // The XHR request says to resolve the time relative to when the request
  // was initially sent, however other uses of this method may need to
  // behave differently, in which case this should be re-arranged somehow.
  if (timeout_milliseconds) {
    double elapsed_time =
        CurrentTimeTicksInSeconds() - request_started_seconds_;
    double next_fire = timeout_milliseconds / 1000.0;
    double resolved_time = std::max(next_fire - elapsed_time, 0.0);
    timeout_timer_.StartOneShot(resolved_time, FROM_HERE);
  }
}

void DocumentThreadableLoader::Cancel() {
  // Cancel can re-enter, and therefore |resource()| might be null here as a
  // result.
  if (!client_ || !GetResource()) {
    Clear();
    return;
  }

  DispatchDidFail(ResourceError::CancelledError(GetResource()->Url()));
}

void DocumentThreadableLoader::Detach() {
  Resource* resource = GetResource();
  if (!resource)
    return;
  client_ = new DetachedClient(this);
}

void DocumentThreadableLoader::SetDefersLoading(bool value) {
  if (GetResource() && GetResource()->Loader())
    GetResource()->Loader()->SetDefersLoading(value);
}

void DocumentThreadableLoader::Clear() {
  client_ = nullptr;
  timeout_timer_.Stop();
  request_started_seconds_ = 0.0;
  if (GetResource())
    checker_.WillRemoveClient();
  ClearResource();
}

// In this method, we can clear |request| to tell content::WebURLLoaderImpl of
// Chromium not to follow the redirect. This works only when this method is
// called by RawResource::willSendRequest(). If called by
// RawResource::didAddClient(), clearing |request| won't be propagated to
// content::WebURLLoaderImpl. So, this loader must also get detached from the
// resource by calling clearResource().
// TODO(toyoshim): Implement OOR-CORS mode specific redirect code.
bool DocumentThreadableLoader::RedirectReceived(
    Resource* resource,
    const ResourceRequest& new_request,
    const ResourceResponse& redirect_response) {
  DCHECK(client_);
  DCHECK_EQ(resource, GetResource());
  DCHECK(async_);

  checker_.RedirectReceived();

  const KURL& new_url = new_request.Url();
  const KURL& original_url = redirect_response.Url();

  if (!actual_request_.IsNull()) {
    ReportResponseReceived(resource->Identifier(), redirect_response);

    HandlePreflightFailure(
        original_url, CORS::GetErrorString(
                          CORS::ErrorParameter::CreateForDisallowedRedirect()));

    return false;
  }

  if (redirect_mode_ == network::mojom::FetchRedirectMode::kManual) {
    // We use |redirect_mode_| to check the original redirect mode.
    // |new_request| is a new request for redirect. So we don't set the
    // redirect mode of it in WebURLLoaderImpl::Context::OnReceivedRedirect().
    DCHECK(new_request.UseStreamOnResponse());
    // There is no need to read the body of redirect response because there is
    // no way to read the body of opaque-redirect filtered response's internal
    // response.
    // TODO(horo): If we support any API which expose the internal body, we will
    // have to read the body. And also HTTPCache changes will be needed because
    // it doesn't store the body of redirect responses.
    ResponseReceived(resource, redirect_response,
                     std::make_unique<EmptyDataHandle>());

    if (client_) {
      DCHECK(actual_request_.IsNull());
      NotifyFinished(resource);
    }

    return false;
  }

  if (redirect_mode_ == network::mojom::FetchRedirectMode::kError) {
    ThreadableLoaderClient* client = client_;
    Clear();
    client->DidFailRedirectCheck();

    return false;
  }

  // Allow same origin requests to continue after allowing clients to audit the
  // redirect.
  if (IsAllowedRedirect(new_request.GetFetchRequestMode(), new_url)) {
    client_->DidReceiveRedirectTo(new_url);
    if (client_->IsDocumentThreadableLoaderClient()) {
      return static_cast<DocumentThreadableLoaderClient*>(client_)
          ->WillFollowRedirect(new_url, redirect_response);
    }
    return true;
  }

  if (cors_redirect_limit_ <= 0) {
    ThreadableLoaderClient* client = client_;
    Clear();
    client->DidFailRedirectCheck();
    return false;
  }

  --cors_redirect_limit_;

  probe::didReceiveCORSRedirectResponse(
      GetExecutionContext(), resource->Identifier(),
      GetDocument() && GetDocument()->GetFrame()
          ? GetDocument()->GetFrame()->Loader().GetDocumentLoader()
          : nullptr,
      redirect_response, resource);

  base::Optional<network::mojom::CORSError> redirect_error =
      CORS::CheckRedirectLocation(new_url);
  if (redirect_error) {
    DispatchDidFailAccessControlCheck(
        ResourceError::CancelledDueToAccessCheckError(
            original_url, ResourceRequestBlockedReason::kOther,
            CORS::GetErrorString(CORS::ErrorParameter::CreateForRedirectCheck(
                *redirect_error, original_url, new_url))));
    return false;
  }

  if (cors_flag_) {
    // The redirect response must pass the access control check if the CORS
    // flag is set.
    base::Optional<network::mojom::CORSError> access_error = CORS::CheckAccess(
        original_url, redirect_response.HttpStatusCode(),
        redirect_response.HttpHeaderFields(),
        new_request.GetFetchCredentialsMode(), *GetSecurityOrigin());
    if (access_error) {
      DispatchDidFailAccessControlCheck(
          ResourceError::CancelledDueToAccessCheckError(
              original_url, ResourceRequestBlockedReason::kOther,
              CORS::GetErrorString(CORS::ErrorParameter::CreateForAccessCheck(
                  *access_error, original_url,
                  redirect_response.HttpStatusCode(),
                  redirect_response.HttpHeaderFields(), *GetSecurityOrigin(),
                  request_context_, new_url))));
      return false;
    }
  }

  client_->DidReceiveRedirectTo(new_url);

  // FIXME: consider combining this with CORS redirect handling performed by
  // CrossOriginAccessControl::handleRedirect().
  if (GetResource())
    checker_.WillRemoveClient();
  ClearResource();

  // If
  // - CORS flag is set, and
  // - the origin of the redirect target URL is not same origin with the origin
  //   of the current request's URL
  // set the source origin to a unique opaque origin.
  //
  // See https://fetch.spec.whatwg.org/#http-redirect-fetch.
  if (cors_flag_) {
    scoped_refptr<const SecurityOrigin> original_origin =
        SecurityOrigin::Create(original_url);
    scoped_refptr<const SecurityOrigin> new_origin =
        SecurityOrigin::Create(new_url);
    if (!original_origin->IsSameSchemeHostPort(new_origin.get()))
      security_origin_ = SecurityOrigin::CreateUnique();
  }

  // Set |cors_flag_| so that further logic (corresponds to the main fetch in
  // the spec) will be performed with CORS flag set.
  // See https://fetch.spec.whatwg.org/#http-redirect-fetch.
  cors_flag_ = true;

  // Save the referrer to use when following the redirect.
  override_referrer_ = true;
  referrer_after_redirect_ =
      Referrer(new_request.HttpReferrer(), new_request.GetReferrerPolicy());

  ResourceRequest cross_origin_request(new_request);

  // Remove any headers that may have been added by the network layer that cause
  // access control to fail.
  cross_origin_request.ClearHTTPReferrer();
  cross_origin_request.ClearHTTPOrigin();
  cross_origin_request.ClearHTTPUserAgent();
  // Add any request headers which we previously saved from the
  // original request.
  for (const auto& header : request_headers_)
    cross_origin_request.SetHTTPHeaderField(header.key, header.value);
  MakeCrossOriginAccessRequest(cross_origin_request);

  return false;
}

void DocumentThreadableLoader::RedirectBlocked() {
  checker_.RedirectBlocked();

  // Tells the client that a redirect was received but not followed (for an
  // unknown reason).
  ThreadableLoaderClient* client = client_;
  Clear();
  client->DidFailRedirectCheck();
}

void DocumentThreadableLoader::DataSent(
    Resource* resource,
    unsigned long long bytes_sent,
    unsigned long long total_bytes_to_be_sent) {
  DCHECK(client_);
  DCHECK_EQ(resource, GetResource());
  DCHECK(async_);

  checker_.DataSent();
  client_->DidSendData(bytes_sent, total_bytes_to_be_sent);
}

void DocumentThreadableLoader::DataDownloaded(Resource* resource,
                                              int data_length) {
  DCHECK(client_);
  DCHECK_EQ(resource, GetResource());
  DCHECK(actual_request_.IsNull());
  DCHECK(async_);

  checker_.DataDownloaded();
  client_->DidDownloadData(data_length);
}

void DocumentThreadableLoader::DidReceiveResourceTiming(
    Resource* resource,
    const ResourceTimingInfo& info) {
  DCHECK(client_);
  DCHECK_EQ(resource, GetResource());
  DCHECK(async_);

  client_->DidReceiveResourceTiming(info);
}

void DocumentThreadableLoader::DidDownloadToBlob(
    Resource* resource,
    scoped_refptr<BlobDataHandle> blob) {
  DCHECK(client_);
  DCHECK_EQ(resource, GetResource());
  DCHECK(async_);

  checker_.DidDownloadToBlob();
  client_->DidDownloadToBlob(std::move(blob));
}

void DocumentThreadableLoader::ResponseReceived(
    Resource* resource,
    const ResourceResponse& response,
    std::unique_ptr<WebDataConsumerHandle> handle) {
  DCHECK_EQ(resource, GetResource());
  DCHECK(async_);

  checker_.ResponseReceived();

  if (handle)
    is_using_data_consumer_handle_ = true;

  HandleResponse(resource->Identifier(), fetch_request_mode_,
                 fetch_credentials_mode_, response, std::move(handle));
}

void DocumentThreadableLoader::HandlePreflightResponse(
    const ResourceResponse& response) {
  base::Optional<network::mojom::CORSError> cors_error =
      CORS::CheckPreflightAccess(response.Url(), response.HttpStatusCode(),
                                 response.HttpHeaderFields(),
                                 actual_request_.GetFetchCredentialsMode(),
                                 *GetSecurityOrigin());
  if (cors_error) {
    HandlePreflightFailure(
        response.Url(),
        CORS::GetErrorString(CORS::ErrorParameter::CreateForAccessCheck(
            *cors_error, response.Url(), 0 /* do not provide the status_code */,
            response.HttpHeaderFields(), *GetSecurityOrigin(),
            request_context_)));
    return;
  }

  base::Optional<network::mojom::CORSError> preflight_error =
      CORS::CheckPreflight(response.HttpStatusCode());
  if (preflight_error) {
    HandlePreflightFailure(
        response.Url(), CORS::GetErrorString(
                            CORS::ErrorParameter::CreateForPreflightStatusCheck(
                                response.HttpStatusCode())));
    return;
  }

  if (actual_request_.IsExternalRequest()) {
    base::Optional<network::mojom::CORSError> external_preflight_status =
        CORS::CheckExternalPreflight(response.HttpHeaderFields());
    if (external_preflight_status) {
      HandlePreflightFailure(
          response.Url(),
          CORS::GetErrorString(
              CORS::ErrorParameter::CreateForExternalPreflightCheck(
                  *external_preflight_status, response.HttpHeaderFields())));
      return;
    }
  }

  String access_control_error_description;
  if (!CORS::EnsurePreflightResultAndCacheOnSuccess(
          response.HttpHeaderFields(), GetSecurityOrigin()->ToString(),
          actual_request_.Url(), actual_request_.HttpMethod(),
          actual_request_.HttpHeaderFields(),
          actual_request_.GetFetchCredentialsMode(),
          &access_control_error_description)) {
    HandlePreflightFailure(response.Url(), access_control_error_description);
  }
}

void DocumentThreadableLoader::ReportResponseReceived(
    unsigned long identifier,
    const ResourceResponse& response) {
  LocalFrame* frame = GetDocument() ? GetDocument()->GetFrame() : nullptr;
  if (!frame)
    return;
  DocumentLoader* loader = frame->Loader().GetDocumentLoader();
  probe::didReceiveResourceResponse(GetExecutionContext(), identifier, loader,
                                    response, GetResource());
  frame->Console().ReportResourceResponseReceived(loader, identifier, response);
}

void DocumentThreadableLoader::HandleResponse(
    unsigned long identifier,
    network::mojom::FetchRequestMode request_mode,
    network::mojom::FetchCredentialsMode credentials_mode,
    const ResourceResponse& response,
    std::unique_ptr<WebDataConsumerHandle> handle) {
  DCHECK(client_);

  // TODO(toyoshim): Support OOR-CORS preflight and Service Worker case.
  // Note that CORS-preflight is usually handled in the Network Service side,
  // but still done in Blink side when it is needed on redirects.
  // https://crbug.com/736308.
  if (out_of_blink_cors_ && actual_request_.IsNull() &&
      !response.WasFetchedViaServiceWorker()) {
    client_->DidReceiveResponse(identifier, response, std::move(handle));
    return;
  }

  // Code path for legacy Blink CORS.
  if (!actual_request_.IsNull()) {
    ReportResponseReceived(identifier, response);
    HandlePreflightResponse(response);
    return;
  }

  if (response.WasFetchedViaServiceWorker()) {
    if (response.WasFallbackRequiredByServiceWorker()) {
      // At this point we must have m_fallbackRequestForServiceWorker. (For
      // SharedWorker the request won't be CORS or CORS-with-preflight,
      // therefore fallback-to-network is handled in the browser process when
      // the ServiceWorker does not call respondWith().)
      DCHECK(!fallback_request_for_service_worker_.IsNull());
      ReportResponseReceived(identifier, response);
      LoadFallbackRequestForServiceWorker();
      return;
    }

    // It's possible that we issue a fetch with request with non "no-cors"
    // mode but get an opaque filtered response if a service worker is involved.
    // We dispatch a CORS failure for the case.
    // TODO(yhirano): This is probably not spec conformant. Fix it after
    // https://github.com/w3c/preload/issues/100 is addressed.
    if (request_mode != network::mojom::FetchRequestMode::kNoCORS &&
        response.ResponseTypeViaServiceWorker() ==
            network::mojom::FetchResponseType::kOpaque) {
      DispatchDidFailAccessControlCheck(
          ResourceError::CancelledDueToAccessCheckError(
              response.Url(), ResourceRequestBlockedReason::kOther,
              CORS::GetErrorString(
                  CORS::ErrorParameter::CreateForInvalidResponse(
                      response.Url(), *GetSecurityOrigin()))));
      return;
    }

    fallback_request_for_service_worker_ = ResourceRequest();
    client_->DidReceiveResponse(identifier, response, std::move(handle));
    return;
  }

  // Even if the request met the conditions to get handled by a Service Worker
  // in the constructor of this class (and therefore
  // |m_fallbackRequestForServiceWorker| is set), the Service Worker may skip
  // processing the request. Only if the request is same origin, the skipped
  // response may come here (wasFetchedViaServiceWorker() returns false) since
  // such a request doesn't have to go through the CORS algorithm by calling
  // loadFallbackRequestForServiceWorker().
  DCHECK(fallback_request_for_service_worker_.IsNull() ||
         GetSecurityOrigin()->CanRequest(
             fallback_request_for_service_worker_.Url()));
  fallback_request_for_service_worker_ = ResourceRequest();

  if (CORS::IsCORSEnabledRequestMode(request_mode) && cors_flag_) {
    base::Optional<network::mojom::CORSError> access_error = CORS::CheckAccess(
        response.Url(), response.HttpStatusCode(), response.HttpHeaderFields(),
        credentials_mode, *GetSecurityOrigin());
    if (access_error) {
      ReportResponseReceived(identifier, response);
      DispatchDidFailAccessControlCheck(
          ResourceError::CancelledDueToAccessCheckError(
              response.Url(), ResourceRequestBlockedReason::kOther,
              CORS::GetErrorString(CORS::ErrorParameter::CreateForAccessCheck(
                  *access_error, response.Url(), response.HttpStatusCode(),
                  response.HttpHeaderFields(), *GetSecurityOrigin(),
                  request_context_))));
      return;
    }
  }

  client_->DidReceiveResponse(identifier, response, std::move(handle));
}

void DocumentThreadableLoader::SetSerializedCachedMetadata(Resource*,
                                                           const char* data,
                                                           size_t size) {
  checker_.SetSerializedCachedMetadata();

  if (!actual_request_.IsNull())
    return;
  client_->DidReceiveCachedMetadata(data, size);
}

void DocumentThreadableLoader::DataReceived(Resource* resource,
                                            const char* data,
                                            size_t data_length) {
  DCHECK_EQ(resource, GetResource());
  DCHECK(async_);

  checker_.DataReceived();

  if (is_using_data_consumer_handle_)
    return;

  // TODO(junov): Fix the ThreadableLoader ecosystem to use size_t. Until then,
  // we use safeCast to trap potential overflows.
  HandleReceivedData(data, SafeCast<unsigned>(data_length));
}

void DocumentThreadableLoader::HandleReceivedData(const char* data,
                                                  size_t data_length) {
  DCHECK(client_);

  // Preflight data should be invisible to clients.
  if (!actual_request_.IsNull())
    return;

  DCHECK(fallback_request_for_service_worker_.IsNull());

  client_->DidReceiveData(data, data_length);
}

void DocumentThreadableLoader::NotifyFinished(Resource* resource) {
  DCHECK(client_);
  DCHECK_EQ(resource, GetResource());
  DCHECK(async_);

  checker_.NotifyFinished(resource);

  if (resource->ErrorOccurred()) {
    DispatchDidFail(resource->GetResourceError());
  } else {
    HandleSuccessfulFinish(resource->Identifier());
  }
}

void DocumentThreadableLoader::HandleSuccessfulFinish(
    unsigned long identifier) {
  DCHECK(fallback_request_for_service_worker_.IsNull());

  if (!actual_request_.IsNull()) {
    DCHECK(actual_request_.IsExternalRequest() || cors_flag_);
    LoadActualRequest();
    return;
  }

  ThreadableLoaderClient* client = client_;
  // Protect the resource in |didFinishLoading| in order not to release the
  // downloaded file.
  Persistent<Resource> protect = GetResource();
  Clear();
  client->DidFinishLoading(identifier);
}

void DocumentThreadableLoader::DidTimeout(TimerBase* timer) {
  DCHECK(async_);
  DCHECK_EQ(timer, &timeout_timer_);
  // clearResource() may be called in clear() and some other places. clear()
  // calls stop() on |m_timeoutTimer|. In the other places, the resource is set
  // again. If the creation fails, clear() is called. So, here, resource() is
  // always non-nullptr.
  DCHECK(GetResource());
  // When |m_client| is set to nullptr only in clear() where |m_timeoutTimer|
  // is stopped. So, |m_client| is always non-nullptr here.
  DCHECK(client_);

  DispatchDidFail(ResourceError::TimeoutError(GetResource()->Url()));
}

void DocumentThreadableLoader::LoadFallbackRequestForServiceWorker() {
  if (GetResource())
    checker_.WillRemoveClient();
  ClearResource();
  ResourceRequest fallback_request(fallback_request_for_service_worker_);
  fallback_request_for_service_worker_ = ResourceRequest();
  DispatchInitialRequest(fallback_request);
}

void DocumentThreadableLoader::LoadActualRequest() {
  ResourceRequest actual_request = actual_request_;
  ResourceLoaderOptions actual_options = actual_options_;
  actual_request_ = ResourceRequest();
  actual_options_ = ResourceLoaderOptions();

  if (GetResource())
    checker_.WillRemoveClient();
  ClearResource();

  PrepareCrossOriginRequest(actual_request);
  LoadRequest(actual_request, actual_options);
}

void DocumentThreadableLoader::HandlePreflightFailure(
    const KURL& url,
    const String& error_description) {
  // Prevent handleSuccessfulFinish() from bypassing access check.
  actual_request_ = ResourceRequest();

  DispatchDidFailAccessControlCheck(
      ResourceError::CancelledDueToAccessCheckError(
          url, ResourceRequestBlockedReason::kOther, error_description));
}

void DocumentThreadableLoader::DispatchDidFailAccessControlCheck(
    const ResourceError& error) {
  const String message = "Failed to load " + error.FailingURL() + ": " +
                         error.LocalizedDescription();
  GetExecutionContext()->AddConsoleMessage(
      ConsoleMessage::Create(kJSMessageSource, kErrorMessageLevel, message));

  ThreadableLoaderClient* client = client_;
  Clear();
  client->DidFail(error);
}

void DocumentThreadableLoader::DispatchDidFail(const ResourceError& error) {
  if (error.CORSErrorStatus()) {
    DCHECK(out_of_blink_cors_);
    // TODO(toyoshim): Should consider to pass correct arguments instead of
    // KURL(), 0, and HTTPHeaderMap() to GetErrorString().
    // We still need plumbing some more information.
    GetExecutionContext()->AddConsoleMessage(ConsoleMessage::Create(
        kJSMessageSource, kErrorMessageLevel,
        "Failed to load " + error.FailingURL() + ": " +
            CORS::GetErrorString(
                CORS::ErrorParameter::Create(
                    *error.CORSErrorStatus(), KURL(error.FailingURL()), KURL(),
                    0, HTTPHeaderMap(), *GetSecurityOrigin(), request_context_))
                .Utf8()
                .data()));
  }
  ThreadableLoaderClient* client = client_;
  Clear();
  client->DidFail(error);
}

void DocumentThreadableLoader::LoadRequestAsync(
    const ResourceRequest& request,
    ResourceLoaderOptions resource_loader_options) {
  if (!actual_request_.IsNull())
    resource_loader_options.data_buffering_policy = kBufferData;

  // The timer can be active if this is the actual request of a
  // CORS-with-preflight request.
  if (options_.timeout_milliseconds > 0 && !timeout_timer_.IsActive()) {
    timeout_timer_.StartOneShot(options_.timeout_milliseconds / 1000.0,
                                FROM_HERE);
  }

  FetchParameters new_params(request, resource_loader_options);
  if (request.GetFetchRequestMode() ==
      network::mojom::FetchRequestMode::kNoCORS) {
    new_params.SetOriginRestriction(FetchParameters::kNoOriginRestriction);
  }
  DCHECK(!GetResource());

  ResourceFetcher* fetcher = loading_context_->GetResourceFetcher();
  if (request.GetRequestContext() == WebURLRequest::kRequestContextVideo ||
      request.GetRequestContext() == WebURLRequest::kRequestContextAudio) {
    RawResource::FetchMedia(new_params, fetcher, this);
  } else if (request.GetRequestContext() ==
             WebURLRequest::kRequestContextManifest) {
    RawResource::FetchManifest(new_params, fetcher, this);
  } else {
    RawResource::Fetch(new_params, fetcher, this);
  }
  checker_.WillAddClient();

  if (GetResource()->IsLoading()) {
    unsigned long identifier = GetResource()->Identifier();
    probe::documentThreadableLoaderStartedLoadingForClient(
        GetExecutionContext(), identifier, client_);
  } else {
    probe::documentThreadableLoaderFailedToStartLoadingForClient(
        GetExecutionContext(), client_);
  }
}

void DocumentThreadableLoader::LoadRequestSync(
    const ResourceRequest& request,
    ResourceLoaderOptions resource_loader_options) {
  FetchParameters fetch_params(request, resource_loader_options);
  if (request.GetFetchRequestMode() ==
      network::mojom::FetchRequestMode::kNoCORS) {
    fetch_params.SetOriginRestriction(FetchParameters::kNoOriginRestriction);
  }
  if (options_.timeout_milliseconds > 0) {
    fetch_params.MutableResourceRequest().SetTimeoutInterval(
        options_.timeout_milliseconds / 1000.0);
  }
  RawResource* resource = RawResource::FetchSynchronously(
      fetch_params, loading_context_->GetResourceFetcher());
  ResourceResponse response = resource->GetResponse();
  unsigned long identifier = resource->Identifier();
  probe::documentThreadableLoaderStartedLoadingForClient(GetExecutionContext(),
                                                         identifier, client_);
  ThreadableLoaderClient* client = client_;
  const KURL& request_url = request.Url();

  // No exception for file:/// resources, see <rdar://problem/4962298>. Also, if
  // we have an HTTP response, then it wasn't a network error in fact.
  if (resource->LoadFailedOrCanceled() && !request_url.IsLocalFile() &&
      response.HttpStatusCode() <= 0) {
    DispatchDidFail(resource->GetResourceError());
    return;
  }

  // FIXME: A synchronous request does not tell us whether a redirect happened
  // or not, so we guess by comparing the request and response URLs. This isn't
  // a perfect test though, since a server can serve a redirect to the same URL
  // that was requested. Also comparing the request and response URLs as strings
  // will fail if the requestURL still has its credentials.
  if (request_url != response.Url() &&
      !IsAllowedRedirect(request.GetFetchRequestMode(), response.Url())) {
    client_ = nullptr;
    client->DidFailRedirectCheck();
    return;
  }

  HandleResponse(identifier, request.GetFetchRequestMode(),
                 request.GetFetchCredentialsMode(), response, nullptr);

  // HandleResponse() may detect an error. In such a case (check |m_client| as
  // it gets reset by clear() call), skip the rest.
  //
  // |this| is alive here since loadResourceSynchronously() keeps it alive until
  // the end of the function.
  if (!client_)
    return;

  if (scoped_refptr<const SharedBuffer> data = resource->ResourceBuffer()) {
    data->ForEachSegment([this](const char* segment, size_t segment_size,
                                size_t segment_offset) -> bool {
      HandleReceivedData(segment, segment_size);
      // The client may cancel this loader in handleReceivedData().
      return client_;
    });
  }

  // The client may cancel this loader in handleReceivedData(). In such a case,
  // skip the rest.
  if (!client_)
    return;

  base::Optional<int64_t> downloaded_file_length =
      resource->DownloadedFileLength();
  if (downloaded_file_length) {
    client_->DidDownloadData(*downloaded_file_length);
  }
  if (request.DownloadToBlob()) {
    if (resource->DownloadedBlob())
      client_->DidDownloadData(resource->DownloadedBlob()->size());
    client_->DidDownloadToBlob(resource->DownloadedBlob());
  }

  HandleSuccessfulFinish(identifier);
}

void DocumentThreadableLoader::LoadRequest(
    ResourceRequest& request,
    ResourceLoaderOptions resource_loader_options) {
  resource_loader_options.cors_handling_by_resource_fetcher =
      kDisableCORSHandlingByResourceFetcher;

  bool allow_stored_credentials = false;
  switch (request.GetFetchCredentialsMode()) {
    case network::mojom::FetchCredentialsMode::kOmit:
      break;
    case network::mojom::FetchCredentialsMode::kSameOrigin:
      // TODO(toyoshim): It's wrong to use |cors_flag| here. Fix it to use the
      // response tainting.
      //
      // TODO(toyoshim): The credentials mode must work even when the "no-cors"
      // mode is in use. See the following issues:
      // - https://github.com/whatwg/fetch/issues/130
      // - https://github.com/whatwg/fetch/issues/169
      allow_stored_credentials = !cors_flag_;
      break;
    case network::mojom::FetchCredentialsMode::kInclude:
      allow_stored_credentials = true;
      break;
  }
  request.SetAllowStoredCredentials(allow_stored_credentials);

  resource_loader_options.security_origin = security_origin_;
  if (async_)
    LoadRequestAsync(request, resource_loader_options);
  else
    LoadRequestSync(request, resource_loader_options);
}

bool DocumentThreadableLoader::IsAllowedRedirect(
    network::mojom::FetchRequestMode fetch_request_mode,
    const KURL& url) const {
  if (fetch_request_mode == network::mojom::FetchRequestMode::kNoCORS)
    return true;

  return !cors_flag_ && GetSecurityOrigin()->CanRequest(url);
}

const SecurityOrigin* DocumentThreadableLoader::GetSecurityOrigin() const {
  return security_origin_
             ? security_origin_.get()
             : loading_context_->GetFetchContext()->GetSecurityOrigin();
}

Document* DocumentThreadableLoader::GetDocument() const {
  ExecutionContext* context = GetExecutionContext();
  if (context->IsDocument())
    return ToDocument(context);
  return nullptr;
}

ExecutionContext* DocumentThreadableLoader::GetExecutionContext() const {
  DCHECK(loading_context_);
  return loading_context_->GetExecutionContext();
}

void DocumentThreadableLoader::Trace(blink::Visitor* visitor) {
  visitor->Trace(loading_context_);
  ThreadableLoader::Trace(visitor);
  RawResourceClient::Trace(visitor);
}

}  // namespace blink
