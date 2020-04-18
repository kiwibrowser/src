/*
 * Copyright (C) 2006, 2007, 2010, 2011 Apple Inc. All rights reserved.
 *           (C) 2007 Graham Dennis (graham.dennis@gmail.com)
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 * 3.  Neither the name of Apple Computer, Inc. ("Apple") nor the names of
 *     its contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "third_party/blink/renderer/platform/loader/fetch/resource_loader.h"

#include "services/network/public/mojom/fetch_api.mojom-blink.h"
#include "third_party/blink/public/mojom/blob/blob_registry.mojom-blink.h"
#include "third_party/blink/public/platform/platform.h"
#include "third_party/blink/public/platform/web_cors.h"
#include "third_party/blink/public/platform/web_data.h"
#include "third_party/blink/public/platform/web_url_error.h"
#include "third_party/blink/public/platform/web_url_request.h"
#include "third_party/blink/public/platform/web_url_response.h"
#include "third_party/blink/renderer/platform/exported/wrapped_resource_request.h"
#include "third_party/blink/renderer/platform/exported/wrapped_resource_response.h"
#include "third_party/blink/renderer/platform/loader/cors/cors.h"
#include "third_party/blink/renderer/platform/loader/cors/cors_error_string.h"
#include "third_party/blink/renderer/platform/loader/fetch/fetch_context.h"
#include "third_party/blink/renderer/platform/loader/fetch/resource.h"
#include "third_party/blink/renderer/platform/loader/fetch/resource_error.h"
#include "third_party/blink/renderer/platform/loader/fetch/resource_fetcher.h"
#include "third_party/blink/renderer/platform/network/network_instrumentation.h"
#include "third_party/blink/renderer/platform/scheduler/public/thread_scheduler.h"
#include "third_party/blink/renderer/platform/shared_buffer.h"
#include "third_party/blink/renderer/platform/weborigin/security_violation_reporting_policy.h"
#include "third_party/blink/renderer/platform/wtf/assertions.h"
#include "third_party/blink/renderer/platform/wtf/text/string_builder.h"
#include "third_party/blink/renderer/platform/wtf/time.h"

namespace blink {

namespace {

bool IsThrottlableRequestContext(WebURLRequest::RequestContext context) {
  return context != WebURLRequest::kRequestContextEventSource &&
         context != WebURLRequest::kRequestContextFetch &&
         context != WebURLRequest::kRequestContextXMLHttpRequest;
}

}  // namespace

ResourceLoader* ResourceLoader::Create(ResourceFetcher* fetcher,
                                       ResourceLoadScheduler* scheduler,
                                       Resource* resource,
                                       uint32_t inflight_keepalive_bytes) {
  return new ResourceLoader(fetcher, scheduler, resource,
                            inflight_keepalive_bytes);
}

ResourceLoader::ResourceLoader(ResourceFetcher* fetcher,
                               ResourceLoadScheduler* scheduler,
                               Resource* resource,
                               uint32_t inflight_keepalive_bytes)
    : scheduler_client_id_(ResourceLoadScheduler::kInvalidClientId),
      fetcher_(fetcher),
      scheduler_(scheduler),
      resource_(resource),
      inflight_keepalive_bytes_(inflight_keepalive_bytes),
      is_cache_aware_loading_activated_(false),
      progress_binding_(this),
      cancel_timer_(Context().GetLoadingTaskRunner(),
                    this,
                    &ResourceLoader::CancelTimerFired) {
  DCHECK(resource_);
  DCHECK(fetcher_);

  resource_->SetLoader(this);
}

ResourceLoader::~ResourceLoader() = default;

void ResourceLoader::Trace(blink::Visitor* visitor) {
  visitor->Trace(fetcher_);
  visitor->Trace(scheduler_);
  visitor->Trace(resource_);
  ResourceLoadSchedulerClient::Trace(visitor);
}

void ResourceLoader::Start() {
  const ResourceRequest& request = resource_->GetResourceRequest();
  ActivateCacheAwareLoadingIfNeeded(request);
  loader_ = Context().CreateURLLoader(request, Context().GetLoadingTaskRunner(),
                                      resource_->Options());
  DCHECK_EQ(ResourceLoadScheduler::kInvalidClientId, scheduler_client_id_);
  auto throttle_option = ResourceLoadScheduler::ThrottleOption::kCanBeThrottled;

  // Synchronous requests should not work with a throttling. Also, disables
  // throttling for the case that can be used for aka long-polling requests.
  // We also disable throttling for non-http[s] requests.
  if (resource_->Options().synchronous_policy == kRequestSynchronously ||
      !IsThrottlableRequestContext(request.GetRequestContext()) ||
      !request.Url().ProtocolIsInHTTPFamily()) {
    throttle_option = ResourceLoadScheduler::ThrottleOption::kCanNotBeThrottled;
  }

  scheduler_->Request(this, throttle_option, request.Priority(),
                      request.IntraPriorityValue(), &scheduler_client_id_);
}

void ResourceLoader::Run() {
  StartWith(resource_->GetResourceRequest());
}

void ResourceLoader::StartWith(const ResourceRequest& request) {
  DCHECK_NE(ResourceLoadScheduler::kInvalidClientId, scheduler_client_id_);
  DCHECK(loader_);

  if (resource_->Options().synchronous_policy == kRequestSynchronously &&
      Context().DefersLoading()) {
    Cancel();
    return;
  }

  is_downloading_to_blob_ = request.DownloadToBlob();

  loader_->SetDefersLoading(Context().DefersLoading());

  if (is_cache_aware_loading_activated_) {
    // Override cache policy for cache-aware loading. If this request fails, a
    // reload with original request will be triggered in DidFail().
    ResourceRequest cache_aware_request(request);
    cache_aware_request.SetCacheMode(
        mojom::FetchCacheMode::kUnspecifiedOnlyIfCachedStrict);
    loader_->LoadAsynchronously(WrappedResourceRequest(cache_aware_request),
                                this);
    return;
  }

  if (resource_->Options().synchronous_policy == kRequestSynchronously)
    RequestSynchronously(request);
  else
    loader_->LoadAsynchronously(WrappedResourceRequest(request), this);
}

void ResourceLoader::Release(
    ResourceLoadScheduler::ReleaseOption option,
    const ResourceLoadScheduler::TrafficReportHints& hints) {
  DCHECK_NE(ResourceLoadScheduler::kInvalidClientId, scheduler_client_id_);
  bool released = scheduler_->Release(scheduler_client_id_, option, hints);
  DCHECK(released);
  scheduler_client_id_ = ResourceLoadScheduler::kInvalidClientId;
}

void ResourceLoader::Restart(const ResourceRequest& request) {
  CHECK_EQ(resource_->Options().synchronous_policy, kRequestAsynchronously);

  loader_ = Context().CreateURLLoader(request, Context().GetLoadingTaskRunner(),
                                      resource_->Options());
  StartWith(request);
}

void ResourceLoader::SetDefersLoading(bool defers) {
  DCHECK(loader_);
  loader_->SetDefersLoading(defers);
  if (defers) {
    resource_->VirtualTimePauser().UnpauseVirtualTime();
  } else {
    resource_->VirtualTimePauser().PauseVirtualTime();
  }
}

void ResourceLoader::DidChangePriority(ResourceLoadPriority load_priority,
                                       int intra_priority_value) {
  if (scheduler_->IsRunning(scheduler_client_id_)) {
    DCHECK(loader_);
    DCHECK_NE(ResourceLoadScheduler::kInvalidClientId, scheduler_client_id_);
    loader_->DidChangePriority(
        static_cast<WebURLRequest::Priority>(load_priority),
        intra_priority_value);
  } else {
    scheduler_->SetPriority(scheduler_client_id_, load_priority,
                            intra_priority_value);
  }
}

void ResourceLoader::ScheduleCancel() {
  if (!cancel_timer_.IsActive())
    cancel_timer_.StartOneShot(TimeDelta(), FROM_HERE);
}

void ResourceLoader::CancelTimerFired(TimerBase*) {
  if (loader_ && !resource_->HasClientsOrObservers())
    Cancel();
}

void ResourceLoader::Cancel() {
  HandleError(
      ResourceError::CancelledError(resource_->LastResourceRequest().Url()));
}

void ResourceLoader::CancelForRedirectAccessCheckError(
    const KURL& new_url,
    ResourceRequestBlockedReason blocked_reason) {
  resource_->WillNotFollowRedirect();

  if (loader_) {
    HandleError(
        ResourceError::CancelledDueToAccessCheckError(new_url, blocked_reason));
  }
}

static bool IsManualRedirectFetchRequest(const ResourceRequest& request) {
  return request.GetFetchRedirectMode() ==
             network::mojom::FetchRedirectMode::kManual &&
         request.GetRequestContext() == WebURLRequest::kRequestContextFetch;
}

bool ResourceLoader::WillFollowRedirect(
    const WebURL& new_url,
    const WebURL& new_site_for_cookies,
    const WebString& new_referrer,
    WebReferrerPolicy new_referrer_policy,
    const WebString& new_method,
    const WebURLResponse& passed_redirect_response,
    bool& report_raw_headers) {
  DCHECK(!passed_redirect_response.IsNull());

  if (is_cache_aware_loading_activated_) {
    // Fail as cache miss if cached response is a redirect.
    HandleError(
        ResourceError::CacheMissError(resource_->LastResourceRequest().Url()));
    return false;
  }

  std::unique_ptr<ResourceRequest> new_request =
      resource_->LastResourceRequest().CreateRedirectRequest(
          new_url, new_method, new_site_for_cookies, new_referrer,
          static_cast<ReferrerPolicy>(new_referrer_policy),
          !passed_redirect_response.WasFetchedViaServiceWorker());

  Resource::Type resource_type = resource_->GetType();

  const ResourceRequest& initial_request = resource_->GetResourceRequest();
  // The following parameters never change during the lifetime of a request.
  WebURLRequest::RequestContext request_context =
      initial_request.GetRequestContext();
  network::mojom::RequestContextFrameType frame_type =
      initial_request.GetFrameType();
  network::mojom::FetchRequestMode fetch_request_mode =
      initial_request.GetFetchRequestMode();
  network::mojom::FetchCredentialsMode fetch_credentials_mode =
      initial_request.GetFetchCredentialsMode();

  const ResourceLoaderOptions& options = resource_->Options();

  const ResourceResponse& redirect_response(
      passed_redirect_response.ToResourceResponse());

  if (!IsManualRedirectFetchRequest(initial_request)) {
    bool unused_preload = resource_->IsUnusedPreload();

    // Don't send security violation reports for unused preloads.
    SecurityViolationReportingPolicy reporting_policy =
        unused_preload ? SecurityViolationReportingPolicy::kSuppressReporting
                       : SecurityViolationReportingPolicy::kReport;

    // CanRequest() checks only enforced CSP, so check report-only here to
    // ensure that violations are sent.
    Context().CheckCSPForRequest(
        request_context, new_url, options, reporting_policy,
        ResourceRequest::RedirectStatus::kFollowedRedirect);

    base::Optional<ResourceRequestBlockedReason> blocked_reason =
        Context().CanRequest(
            resource_type, *new_request, new_url, options, reporting_policy,
            FetchParameters::kUseDefaultOriginRestrictionForType,
            ResourceRequest::RedirectStatus::kFollowedRedirect);

    if (Context().IsAdResource(new_url, resource_type,
                               new_request->GetRequestContext())) {
      new_request->SetIsAdResource();
    }

    if (blocked_reason) {
      CancelForRedirectAccessCheckError(new_url, blocked_reason.value());
      return false;
    }

    if (options.cors_handling_by_resource_fetcher ==
            kEnableCORSHandlingByResourceFetcher &&
        fetch_request_mode == network::mojom::FetchRequestMode::kCORS) {
      scoped_refptr<const SecurityOrigin> source_origin = GetSourceOrigin();
      WebSecurityOrigin source_web_origin(source_origin.get());
      WrappedResourceRequest new_request_wrapper(*new_request);
      base::Optional<network::mojom::CORSError> cors_error =
          WebCORS::HandleRedirect(
              source_web_origin, new_request_wrapper, redirect_response.Url(),
              redirect_response.HttpStatusCode(),
              redirect_response.HttpHeaderFields(), fetch_credentials_mode,
              resource_->MutableOptions());
      if (cors_error) {
        resource_->SetCORSStatus(CORSStatus::kFailed);

        if (!unused_preload) {
          Context().AddErrorConsoleMessage(
              CORS::GetErrorString(CORS::ErrorParameter::Create(
                  network::CORSErrorStatus(*cors_error),
                  redirect_response.Url(), new_url,
                  redirect_response.HttpStatusCode(),
                  redirect_response.HttpHeaderFields(), *source_origin.get(),
                  resource_->LastResourceRequest().GetRequestContext())),
              FetchContext::kJSSource);
        }

        CancelForRedirectAccessCheckError(new_url,
                                          ResourceRequestBlockedReason::kOther);
        return false;
      }

      source_origin = source_web_origin;
    }
    if (resource_type == Resource::kImage &&
        fetcher_->ShouldDeferImageLoad(new_url)) {
      CancelForRedirectAccessCheckError(new_url,
                                        ResourceRequestBlockedReason::kOther);
      return false;
    }
  }

  bool cross_origin =
      !SecurityOrigin::AreSameSchemeHostPort(redirect_response.Url(), new_url);
  fetcher_->RecordResourceTimingOnRedirect(resource_.Get(), redirect_response,
                                           cross_origin);

  if (options.cors_handling_by_resource_fetcher ==
          kEnableCORSHandlingByResourceFetcher &&
      fetch_request_mode == network::mojom::FetchRequestMode::kCORS) {
    bool allow_stored_credentials = false;
    switch (fetch_credentials_mode) {
      case network::mojom::FetchCredentialsMode::kOmit:
        break;
      case network::mojom::FetchCredentialsMode::kSameOrigin:
        allow_stored_credentials = !options.cors_flag;
        break;
      case network::mojom::FetchCredentialsMode::kInclude:
        allow_stored_credentials = true;
        break;
    }
    new_request->SetAllowStoredCredentials(allow_stored_credentials);
  }

  // The following two calls may rewrite the new_request.Url() to
  // something else not for rejecting redirect but for other reasons.
  // E.g. WebFrameTestClient::WillSendRequest() and
  // RenderFrameImpl::WillSendRequest(). We should reflect the
  // rewriting but currently we cannot. So, compare new_request.Url() and
  // new_url after calling them, and return false to make the redirect fail on
  // mismatch.

  Context().PrepareRequest(*new_request,
                           FetchContext::RedirectType::kForRedirect);
  if (Context().GetFrameScheduler()) {
    WebScopedVirtualTimePauser virtual_time_pauser =
        Context().GetFrameScheduler()->CreateWebScopedVirtualTimePauser(
            resource_->Url().GetString(),
            WebScopedVirtualTimePauser::VirtualTaskDuration::kNonInstant);
    virtual_time_pauser.PauseVirtualTime();
    resource_->VirtualTimePauser() = std::move(virtual_time_pauser);
  }
  Context().DispatchWillSendRequest(resource_->Identifier(), *new_request,
                                    redirect_response, resource_->GetType(),
                                    options.initiator_info);

  // First-party cookie logic moved from DocumentLoader in Blink to
  // net::URLRequest in the browser. Assert that Blink didn't try to change it
  // to something else.
  DCHECK(KURL(new_site_for_cookies) == new_request->SiteForCookies());

  // The following parameters never change during the lifetime of a request.
  DCHECK_EQ(new_request->GetRequestContext(), request_context);
  DCHECK_EQ(new_request->GetFrameType(), frame_type);
  DCHECK_EQ(new_request->GetFetchRequestMode(), fetch_request_mode);
  DCHECK_EQ(new_request->GetFetchCredentialsMode(), fetch_credentials_mode);

  if (new_request->Url() != KURL(new_url)) {
    CancelForRedirectAccessCheckError(new_request->Url(),
                                      ResourceRequestBlockedReason::kOther);
    return false;
  }

  if (!resource_->WillFollowRedirect(*new_request, redirect_response)) {
    CancelForRedirectAccessCheckError(new_request->Url(),
                                      ResourceRequestBlockedReason::kOther);
    return false;
  }

  report_raw_headers = new_request->ReportRawHeaders();

  return true;
}

void ResourceLoader::DidReceiveCachedMetadata(const char* data, int length) {
  resource_->SetSerializedCachedMetadata(data, length);
}

void ResourceLoader::DidSendData(unsigned long long bytes_sent,
                                 unsigned long long total_bytes_to_be_sent) {
  resource_->DidSendData(bytes_sent, total_bytes_to_be_sent);
}

FetchContext& ResourceLoader::Context() const {
  return fetcher_->Context();
}

scoped_refptr<const SecurityOrigin> ResourceLoader::GetSourceOrigin() const {
  scoped_refptr<const SecurityOrigin> origin =
      resource_->Options().security_origin;
  if (origin)
    return origin;

  return Context().GetSecurityOrigin();
}

CORSStatus ResourceLoader::DetermineCORSStatus(const ResourceResponse& response,
                                               StringBuilder& error_msg) const {
  // Service workers handle CORS separately.
  if (response.WasFetchedViaServiceWorker()) {
    switch (response.ResponseTypeViaServiceWorker()) {
      case network::mojom::FetchResponseType::kBasic:
      case network::mojom::FetchResponseType::kCORS:
      case network::mojom::FetchResponseType::kDefault:
      case network::mojom::FetchResponseType::kError:
        return CORSStatus::kServiceWorkerSuccessful;
      case network::mojom::FetchResponseType::kOpaque:
      case network::mojom::FetchResponseType::kOpaqueRedirect:
        return CORSStatus::kServiceWorkerOpaque;
    }
    NOTREACHED();
  }

  if (resource_->GetType() == Resource::Type::kMainResource)
    return CORSStatus::kNotApplicable;

  scoped_refptr<const SecurityOrigin> source_origin = GetSourceOrigin();
  DCHECK(source_origin);

  if (source_origin->CanRequest(response.Url()))
    return CORSStatus::kSameOrigin;

  // RequestContext, FetchRequestMode and FetchCredentialsMode never change
  // during the lifetime of a request.
  const ResourceRequest& initial_request = resource_->GetResourceRequest();

  if (resource_->Options().cors_handling_by_resource_fetcher !=
          kEnableCORSHandlingByResourceFetcher ||
      initial_request.GetFetchRequestMode() !=
          network::mojom::FetchRequestMode::kCORS) {
    return CORSStatus::kNotApplicable;
  }

  // Use the original response instead of the 304 response for a successful
  // revalidation.
  const ResourceResponse& response_for_access_control =
      (resource_->IsCacheValidator() && response.HttpStatusCode() == 304)
          ? resource_->GetResponse()
          : response;

  base::Optional<network::mojom::CORSError> cors_error = CORS::CheckAccess(
      response_for_access_control.Url(),
      response_for_access_control.HttpStatusCode(),
      response_for_access_control.HttpHeaderFields(),
      initial_request.GetFetchCredentialsMode(), *source_origin);

  if (!cors_error)
    return CORSStatus::kSuccessful;

  String resource_type = Resource::ResourceTypeToString(
      resource_->GetType(), resource_->Options().initiator_info.name);
  error_msg.Append("Access to ");
  error_msg.Append(resource_type);
  error_msg.Append(" at '");
  error_msg.Append(response.Url().GetString());
  error_msg.Append("' from origin '");
  error_msg.Append(source_origin->ToString());
  error_msg.Append("' has been blocked by CORS policy: ");
  error_msg.Append(CORS::GetErrorString(CORS::ErrorParameter::Create(
      network::CORSErrorStatus(*cors_error), initial_request.Url(), KURL(),
      response_for_access_control.HttpStatusCode(),
      response_for_access_control.HttpHeaderFields(), *source_origin,
      initial_request.GetRequestContext())));

  return CORSStatus::kFailed;
}

void ResourceLoader::DidReceiveResponse(
    const WebURLResponse& web_url_response,
    std::unique_ptr<WebDataConsumerHandle> handle) {
  DCHECK(!web_url_response.IsNull());

  if (Context().IsDetached()) {
    // If the fetch context is already detached, we don't need further signals,
    // so let's cancel the request.
    HandleError(ResourceError::CancelledError(web_url_response.Url()));
    return;
  }

  Resource::Type resource_type = resource_->GetType();

  const ResourceRequest& initial_request = resource_->GetResourceRequest();
  // The following parameters never change during the lifetime of a request.
  WebURLRequest::RequestContext request_context =
      initial_request.GetRequestContext();
  network::mojom::FetchRequestMode fetch_request_mode =
      initial_request.GetFetchRequestMode();

  const ResourceLoaderOptions& options = resource_->Options();

  const ResourceResponse& response = web_url_response.ToResourceResponse();

  // Later, CORS results should already be in the response we get from the
  // browser at this point.
  StringBuilder cors_error_msg;
  resource_->SetCORSStatus(DetermineCORSStatus(response, cors_error_msg));

  // Perform 'nosniff' checks against the original response instead of the 304
  // response for a successful revalidation.
  const ResourceResponse& nosniffed_response =
      (resource_->IsCacheValidator() && response.HttpStatusCode() == 304)
          ? resource_->GetResponse()
          : response;
  base::Optional<ResourceRequestBlockedReason> blocked_reason =
      Context().CheckResponseNosniff(request_context, nosniffed_response);
  if (blocked_reason) {
    HandleError(ResourceError::CancelledDueToAccessCheckError(
        response.Url(), blocked_reason.value()));
    return;
  }

  if (response.WasFetchedViaServiceWorker()) {
    if (options.cors_handling_by_resource_fetcher ==
            kEnableCORSHandlingByResourceFetcher &&
        fetch_request_mode == network::mojom::FetchRequestMode::kCORS &&
        response.WasFallbackRequiredByServiceWorker()) {
      ResourceRequest last_request = resource_->LastResourceRequest();
      DCHECK(!last_request.GetSkipServiceWorker());
      // This code handles the case when a controlling service worker doesn't
      // handle a cross origin request.
      if (!Context().ShouldLoadNewResource(resource_type)) {
        // Cancel the request if we should not trigger a reload now.
        HandleError(ResourceError::CancelledError(response.Url()));
        return;
      }
      last_request.SetSkipServiceWorker(true);
      Restart(last_request);
      return;
    }

    // If the response is fetched via ServiceWorker, the original URL of the
    // response could be different from the URL of the request. We check the URL
    // not to load the resources which are forbidden by the page CSP.
    // https://w3c.github.io/webappsec-csp/#should-block-response
    const KURL& original_url = response.OriginalURLViaServiceWorker();
    if (!original_url.IsEmpty()) {
      // CanRequest() below only checks enforced policies: check report-only
      // here to ensure violations are sent.
      Context().CheckCSPForRequest(
          request_context, original_url, options,
          SecurityViolationReportingPolicy::kReport,
          ResourceRequest::RedirectStatus::kFollowedRedirect);

      base::Optional<ResourceRequestBlockedReason> blocked_reason =
          Context().CanRequest(
              resource_type, initial_request, original_url, options,
              SecurityViolationReportingPolicy::kReport,
              FetchParameters::kUseDefaultOriginRestrictionForType,
              ResourceRequest::RedirectStatus::kFollowedRedirect);
      if (blocked_reason) {
        HandleError(ResourceError::CancelledDueToAccessCheckError(
            original_url, blocked_reason.value()));
        return;
      }
    }
  } else if (options.cors_handling_by_resource_fetcher ==
                 kEnableCORSHandlingByResourceFetcher &&
             fetch_request_mode == network::mojom::FetchRequestMode::kCORS) {
    if (!resource_->IsSameOriginOrCORSSuccessful()) {
      if (!resource_->IsUnusedPreload())
        Context().AddErrorConsoleMessage(cors_error_msg.ToString(),
                                         FetchContext::kJSSource);

      // Redirects can change the response URL different from one of request.
      HandleError(ResourceError::CancelledDueToAccessCheckError(
          response.Url(), ResourceRequestBlockedReason::kOther));
      return;
    }
  }

  // FrameType never changes during the lifetime of a request.
  Context().DispatchDidReceiveResponse(
      resource_->Identifier(), response, initial_request.GetFrameType(),
      request_context, resource_,
      FetchContext::ResourceResponseType::kNotFromMemoryCache);

  resource_->ResponseReceived(response, std::move(handle));
  if (!resource_->Loader())
    return;

  if (response.HttpStatusCode() >= 400 &&
      !resource_->ShouldIgnoreHTTPStatusCodeErrors())
    HandleError(ResourceError::CancelledError(response.Url()));
}

void ResourceLoader::DidReceiveResponse(const WebURLResponse& response) {
  DidReceiveResponse(response, nullptr);
}

void ResourceLoader::DidStartLoadingResponseBody(
    mojo::ScopedDataPipeConsumerHandle body) {
  DCHECK(is_downloading_to_blob_);
  DCHECK(!blob_response_started_);
  blob_response_started_ = true;

  const ResourceResponse& response = resource_->GetResponse();
  AtomicString mime_type = response.MimeType();

  mojom::blink::ProgressClientAssociatedPtrInfo progress_client_ptr;
  progress_binding_.Bind(MakeRequest(&progress_client_ptr));

  // Callback is bound to a WeakPersistent, as ResourceLoader is kept alive by
  // ResourceFetcher as long as we still care about the result of the load.
  mojom::blink::BlobRegistry* blob_registry = BlobDataHandle::GetBlobRegistry();
  blob_registry->RegisterFromStream(
      mime_type.IsNull() ? g_empty_string : mime_type.LowerASCII(), "",
      std::max(0ll, response.ExpectedContentLength()), std::move(body),
      std::move(progress_client_ptr),
      WTF::Bind(&ResourceLoader::FinishedCreatingBlob,
                WrapWeakPersistent(this)));
}

void ResourceLoader::DidDownloadData(int length, int encoded_data_length) {
  Context().DispatchDidDownloadData(resource_->Identifier(), length,
                                    encoded_data_length);
  resource_->DidDownloadData(length);
}

void ResourceLoader::DidReceiveData(const char* data, int length) {
  CHECK_GE(length, 0);

  Context().DispatchDidReceiveData(resource_->Identifier(), data, length);
  resource_->AppendData(data, length);
}

void ResourceLoader::DidReceiveTransferSizeUpdate(int transfer_size_diff) {
  DCHECK_GT(transfer_size_diff, 0);
  Context().DispatchDidReceiveEncodedData(resource_->Identifier(),
                                          transfer_size_diff);
}

void ResourceLoader::DidFinishLoadingFirstPartInMultipart() {
  network_instrumentation::EndResourceLoad(
      resource_->Identifier(),
      network_instrumentation::RequestOutcome::kSuccess);

  fetcher_->HandleLoaderFinish(resource_.Get(), TimeTicks(),
                               ResourceFetcher::kDidFinishFirstPartInMultipart,
                               0, false);
}

void ResourceLoader::DidFinishLoading(TimeTicks finish_time,
                                      int64_t encoded_data_length,
                                      int64_t encoded_body_length,
                                      int64_t decoded_body_length,
                                      bool blocked_cross_site_document) {
  resource_->SetEncodedDataLength(encoded_data_length);
  resource_->SetEncodedBodyLength(encoded_body_length);
  resource_->SetDecodedBodyLength(decoded_body_length);

  if (is_downloading_to_blob_ && !blob_finished_ && blob_response_started_) {
    load_did_finish_before_blob_ =
        DeferedFinishLoadingInfo{finish_time, blocked_cross_site_document};
    return;
  }

  Release(ResourceLoadScheduler::ReleaseOption::kReleaseAndSchedule,
          ResourceLoadScheduler::TrafficReportHints(encoded_data_length,
                                                    decoded_body_length));
  loader_.reset();

  network_instrumentation::EndResourceLoad(
      resource_->Identifier(),
      network_instrumentation::RequestOutcome::kSuccess);

  fetcher_->HandleLoaderFinish(
      resource_.Get(), finish_time, ResourceFetcher::kDidFinishLoading,
      inflight_keepalive_bytes_, blocked_cross_site_document);
}

void ResourceLoader::DidFail(const WebURLError& error,
                             int64_t encoded_data_length,
                             int64_t encoded_body_length,
                             int64_t decoded_body_length) {
  resource_->SetEncodedDataLength(encoded_data_length);
  resource_->SetEncodedBodyLength(encoded_body_length);
  resource_->SetDecodedBodyLength(decoded_body_length);
  HandleError(error);
}

void ResourceLoader::HandleError(const ResourceError& error) {
  if (is_cache_aware_loading_activated_ && error.IsCacheMiss() &&
      Context().ShouldLoadNewResource(resource_->GetType())) {
    resource_->WillReloadAfterDiskCacheMiss();
    is_cache_aware_loading_activated_ = false;
    Restart(resource_->GetResourceRequest());
    return;
  }

  Release(ResourceLoadScheduler::ReleaseOption::kReleaseAndSchedule,
          ResourceLoadScheduler::TrafficReportHints::InvalidInstance());
  loader_.reset();

  network_instrumentation::EndResourceLoad(
      resource_->Identifier(), network_instrumentation::RequestOutcome::kFail);

  fetcher_->HandleLoaderError(resource_.Get(), error,
                              inflight_keepalive_bytes_);
}

void ResourceLoader::RequestSynchronously(const ResourceRequest& request) {
  DCHECK(loader_);
  DCHECK_EQ(request.Priority(), ResourceLoadPriority::kHighest);

  WrappedResourceRequest request_in(request);
  WebURLResponse response_out;
  base::Optional<WebURLError> error_out;
  WebData data_out;
  int64_t encoded_data_length = WebURLLoaderClient::kUnknownEncodedDataLength;
  int64_t encoded_body_length = 0;
  base::Optional<int64_t> downloaded_file_length;
  WebBlobInfo downloaded_blob;
  loader_->LoadSynchronously(request_in, this, response_out, error_out,
                             data_out, encoded_data_length, encoded_body_length,
                             downloaded_file_length, downloaded_blob);

  // A message dispatched while synchronously fetching the resource
  // can bring about the cancellation of this load.
  if (!loader_)
    return;
  int64_t decoded_body_length = data_out.size();
  if (error_out) {
    DidFail(*error_out, encoded_data_length, encoded_body_length,
            decoded_body_length);
    return;
  }
  DidReceiveResponse(response_out);
  if (!loader_)
    return;
  DCHECK_GE(response_out.ToResourceResponse().EncodedBodyLength(), 0);

  // Follow the async case convention of not calling DidReceiveData or
  // appending data to m_resource if the response body is empty. Copying the
  // empty buffer is a noop in most cases, but is destructive in the case of
  // a 304, where it will overwrite the cached data we should be reusing.
  if (data_out.size()) {
    data_out.ForEachSegment([this](const char* segment, size_t segment_size,
                                   size_t segment_offset) {
      Context().DispatchDidReceiveData(resource_->Identifier(), segment,
                                       segment_size);
      return true;
    });
    resource_->SetResourceBuffer(data_out);
  }

  if (downloaded_file_length) {
    DCHECK(request.DownloadToFile());
    DidDownloadData(*downloaded_file_length, encoded_body_length);
  }
  if (request.DownloadToBlob()) {
    auto blob = downloaded_blob.GetBlobHandle();
    if (blob) {
      Context().DispatchDidReceiveData(resource_->Identifier(), nullptr,
                                       blob->size());
      resource_->DidDownloadData(blob->size());
    }
    Context().DispatchDidDownloadToBlob(resource_->Identifier(), blob.get());
    resource_->DidDownloadToBlob(blob);
  }
  DidFinishLoading(CurrentTimeTicks(), encoded_data_length, encoded_body_length,
                   decoded_body_length, false);
}

void ResourceLoader::Dispose() {
  loader_ = nullptr;

  // Release() should be called to release |scheduler_client_id_| beforehand in
  // DidFinishLoading() or DidFail(), but when a timer to call Cancel() is
  // ignored due to GC, this case happens. We just release here because we can
  // not schedule another request safely. See crbug.com/675947.
  if (scheduler_client_id_ != ResourceLoadScheduler::kInvalidClientId) {
    Release(ResourceLoadScheduler::ReleaseOption::kReleaseOnly,
            ResourceLoadScheduler::TrafficReportHints::InvalidInstance());
  }
}

void ResourceLoader::ActivateCacheAwareLoadingIfNeeded(
    const ResourceRequest& request) {
  DCHECK(!is_cache_aware_loading_activated_);

  if (resource_->Options().cache_aware_loading_enabled !=
      kIsCacheAwareLoadingEnabled)
    return;

  // Synchronous requests are not supported.
  if (resource_->Options().synchronous_policy == kRequestSynchronously)
    return;

  // Don't activate on Resource revalidation.
  if (resource_->IsCacheValidator())
    return;

  // Don't activate if cache policy is explicitly set.
  if (request.GetCacheMode() != mojom::FetchCacheMode::kDefault)
    return;

  // Don't activate if the page is controlled by service worker.
  if (fetcher_->IsControlledByServiceWorker())
    return;

  is_cache_aware_loading_activated_ = true;
}

bool ResourceLoader::ShouldBeKeptAliveWhenDetached() const {
  return resource_->GetResourceRequest().GetKeepalive() &&
         resource_->GetResponse().IsNull();
}

scoped_refptr<base::SingleThreadTaskRunner>
ResourceLoader::GetLoadingTaskRunner() {
  return Context().GetLoadingTaskRunner();
}

void ResourceLoader::OnProgress(uint64_t delta) {
  DCHECK(!blob_finished_);

  if (scheduler_client_id_ == ResourceLoadScheduler::kInvalidClientId)
    return;

  Context().DispatchDidReceiveData(resource_->Identifier(), nullptr, delta);
  resource_->DidDownloadData(delta);
}

void ResourceLoader::FinishedCreatingBlob(
    const scoped_refptr<BlobDataHandle>& blob) {
  DCHECK(!blob_finished_);

  if (scheduler_client_id_ == ResourceLoadScheduler::kInvalidClientId)
    return;

  Context().DispatchDidDownloadToBlob(resource_->Identifier(), blob.get());
  resource_->DidDownloadToBlob(blob);

  blob_finished_ = true;
  if (load_did_finish_before_blob_) {
    const ResourceResponse& response = resource_->GetResponse();
    DidFinishLoading(load_did_finish_before_blob_->finish_time,
                     response.EncodedDataLength(), response.EncodedBodyLength(),
                     response.DecodedBodyLength(),
                     load_did_finish_before_blob_->blocked_cross_site_document);
  }
}

}  // namespace blink
