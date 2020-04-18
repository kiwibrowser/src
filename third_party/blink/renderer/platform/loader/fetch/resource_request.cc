/*
 * Copyright (C) 2003, 2006 Apple Computer, Inc.  All rights reserved.
 * Copyright (C) 2009, 2012 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE COMPUTER, INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "third_party/blink/renderer/platform/loader/fetch/resource_request.h"

#include <memory>

#include "third_party/blink/public/platform/web_url_request.h"
#include "third_party/blink/renderer/platform/network/http_names.h"
#include "third_party/blink/renderer/platform/network/network_utils.h"
#include "third_party/blink/renderer/platform/runtime_enabled_features.h"
#include "third_party/blink/renderer/platform/weborigin/security_origin.h"

namespace blink {

double ResourceRequest::default_timeout_interval_ = INT_MAX;

ResourceRequest::ResourceRequest() : ResourceRequest(NullURL()) {}

ResourceRequest::ResourceRequest(const String& url_string)
    : ResourceRequest(KURL(url_string)) {}

ResourceRequest::ResourceRequest(const KURL& url)
    : url_(url),
      timeout_interval_(default_timeout_interval_),
      requestor_origin_(nullptr),
      http_method_(HTTPNames::GET),
      allow_stored_credentials_(true),
      report_upload_progress_(false),
      report_raw_headers_(false),
      has_user_gesture_(false),
      download_to_file_(false),
      download_to_blob_(false),
      use_stream_on_response_(false),
      keepalive_(false),
      should_reset_app_cache_(false),
      cache_mode_(mojom::FetchCacheMode::kDefault),
      skip_service_worker_(false),
      priority_(ResourceLoadPriority::kLowest),
      intra_priority_value_(0),
      requestor_id_(0),
      plugin_child_id_(-1),
      app_cache_host_id_(0),
      previews_state_(WebURLRequest::kPreviewsUnspecified),
      request_context_(WebURLRequest::kRequestContextUnspecified),
      frame_type_(network::mojom::RequestContextFrameType::kNone),
      fetch_request_mode_(network::mojom::FetchRequestMode::kNoCORS),
      fetch_importance_mode_(mojom::FetchImportanceMode::kImportanceAuto),
      fetch_credentials_mode_(network::mojom::FetchCredentialsMode::kInclude),
      fetch_redirect_mode_(network::mojom::FetchRedirectMode::kFollow),
      referrer_policy_(kReferrerPolicyDefault),
      did_set_http_referrer_(false),
      check_for_browser_side_navigation_(true),
      was_discarded_(false),
      ui_start_time_(0),
      is_external_request_(false),
      cors_preflight_policy_(
          network::mojom::CORSPreflightPolicy::kConsiderPreflight),
      is_same_document_navigation_(false),
      input_perf_metric_report_policy_(
          InputToLoadPerfMetricReportPolicy::kNoReport),
      redirect_status_(RedirectStatus::kNoRedirect) {}

ResourceRequest::ResourceRequest(CrossThreadResourceRequestData* data)
    : ResourceRequest(data->url_) {
  SetTimeoutInterval(data->timeout_interval_);
  SetSiteForCookies(data->site_for_cookies_);
  SetRequestorOrigin(data->requestor_origin_);
  SetHTTPMethod(AtomicString(data->http_method_));
  SetPriority(data->priority_, data->intra_priority_value_);

  http_header_fields_.Adopt(std::move(data->http_headers_));

  SetHTTPBody(data->http_body_);
  SetAllowStoredCredentials(data->allow_stored_credentials_);
  SetReportUploadProgress(data->report_upload_progress_);
  SetHasUserGesture(data->has_user_gesture_);
  SetDownloadToFile(data->download_to_file_);
  SetDownloadToBlob(data->download_to_blob_);
  SetUseStreamOnResponse(data->use_stream_on_response_);
  SetKeepalive(data->keepalive_);
  SetCacheMode(data->cache_mode_);
  SetSkipServiceWorker(data->skip_service_worker_);
  SetShouldResetAppCache(data->should_reset_app_cache_);
  SetRequestorID(data->requestor_id_);
  SetPluginChildID(data->plugin_child_id_);
  SetAppCacheHostID(data->app_cache_host_id_);
  SetPreviewsState(data->previews_state_);
  SetRequestContext(data->request_context_);
  SetFrameType(data->frame_type_);
  SetFetchRequestMode(data->fetch_request_mode_);
  SetFetchImportanceMode(data->fetch_importance_mode_);
  SetFetchCredentialsMode(data->fetch_credentials_mode_);
  SetFetchRedirectMode(data->fetch_redirect_mode_);
  SetFetchIntegrity(data->fetch_integrity_.IsolatedCopy());
  referrer_policy_ = data->referrer_policy_;
  did_set_http_referrer_ = data->did_set_http_referrer_;
  check_for_browser_side_navigation_ = data->check_for_browser_side_navigation_;
  ui_start_time_ = data->ui_start_time_;
  is_external_request_ = data->is_external_request_;
  cors_preflight_policy_ = data->cors_preflight_policy_;
  input_perf_metric_report_policy_ = data->input_perf_metric_report_policy_;
  redirect_status_ = data->redirect_status_;
  suggested_filename_ = data->suggested_filename_;
  is_ad_resource_ = data->is_ad_resource_;
  SetInitiatorCSP(data->navigation_csp_);
}

ResourceRequest::ResourceRequest(const ResourceRequest&) = default;

ResourceRequest& ResourceRequest::operator=(const ResourceRequest&) = default;

std::unique_ptr<ResourceRequest> ResourceRequest::CreateRedirectRequest(
    const KURL& new_url,
    const AtomicString& new_method,
    const KURL& new_site_for_cookies,
    const String& new_referrer,
    ReferrerPolicy new_referrer_policy,
    bool skip_service_worker) const {
  std::unique_ptr<ResourceRequest> request =
      std::make_unique<ResourceRequest>(new_url);
  request->SetHTTPMethod(new_method);
  request->SetSiteForCookies(new_site_for_cookies);
  String referrer =
      new_referrer.IsEmpty() ? Referrer::NoReferrer() : String(new_referrer);
  request->SetHTTPReferrer(
      Referrer(referrer, static_cast<ReferrerPolicy>(new_referrer_policy)));
  request->SetSkipServiceWorker(skip_service_worker);
  request->SetRedirectStatus(RedirectStatus::kFollowedRedirect);

  // Copy from parameters for |this|.
  request->SetDownloadToFile(DownloadToFile());
  request->SetDownloadToBlob(DownloadToBlob());
  request->SetUseStreamOnResponse(UseStreamOnResponse());
  request->SetRequestContext(GetRequestContext());
  request->SetFrameType(GetFrameType());
  request->SetShouldResetAppCache(ShouldResetAppCache());
  request->SetFetchRequestMode(GetFetchRequestMode());
  request->SetFetchCredentialsMode(GetFetchCredentialsMode());
  request->SetKeepalive(GetKeepalive());
  request->SetPriority(Priority());

  if (request->HttpMethod() == HttpMethod())
    request->SetHTTPBody(HttpBody());
  request->SetCheckForBrowserSideNavigation(CheckForBrowserSideNavigation());
  request->SetWasDiscarded(WasDiscarded());
  request->SetCORSPreflightPolicy(CORSPreflightPolicy());
  if (IsAdResource())
    request->SetIsAdResource();
  request->SetInitiatorCSP(GetInitiatorCSP());

  return request;
}

std::unique_ptr<CrossThreadResourceRequestData> ResourceRequest::CopyData()
    const {
  std::unique_ptr<CrossThreadResourceRequestData> data =
      std::make_unique<CrossThreadResourceRequestData>();
  data->url_ = Url().Copy();
  data->timeout_interval_ = TimeoutInterval();
  data->site_for_cookies_ = SiteForCookies().Copy();
  data->requestor_origin_ =
      RequestorOrigin() ? RequestorOrigin()->IsolatedCopy() : nullptr;
  data->http_method_ = HttpMethod().GetString().IsolatedCopy();
  data->http_headers_ = HttpHeaderFields().CopyData();
  data->priority_ = Priority();
  data->intra_priority_value_ = intra_priority_value_;

  if (http_body_)
    data->http_body_ = http_body_->DeepCopy();
  data->allow_stored_credentials_ = allow_stored_credentials_;
  data->report_upload_progress_ = report_upload_progress_;
  data->has_user_gesture_ = has_user_gesture_;
  data->download_to_file_ = download_to_file_;
  data->download_to_blob_ = download_to_blob_;
  data->use_stream_on_response_ = use_stream_on_response_;
  data->keepalive_ = keepalive_;
  data->cache_mode_ = GetCacheMode();
  data->skip_service_worker_ = skip_service_worker_;
  data->should_reset_app_cache_ = should_reset_app_cache_;
  data->requestor_id_ = requestor_id_;
  data->plugin_child_id_ = plugin_child_id_;
  data->app_cache_host_id_ = app_cache_host_id_;
  data->previews_state_ = previews_state_;
  data->request_context_ = request_context_;
  data->frame_type_ = frame_type_;
  data->fetch_request_mode_ = fetch_request_mode_;
  data->fetch_importance_mode_ = fetch_importance_mode_;
  data->fetch_credentials_mode_ = fetch_credentials_mode_;
  data->fetch_redirect_mode_ = fetch_redirect_mode_;
  data->fetch_integrity_ = fetch_integrity_.IsolatedCopy();
  data->referrer_policy_ = referrer_policy_;
  data->did_set_http_referrer_ = did_set_http_referrer_;
  data->check_for_browser_side_navigation_ = check_for_browser_side_navigation_;
  data->ui_start_time_ = ui_start_time_;
  data->is_external_request_ = is_external_request_;
  data->cors_preflight_policy_ = cors_preflight_policy_;
  data->input_perf_metric_report_policy_ = input_perf_metric_report_policy_;
  data->redirect_status_ = redirect_status_;
  data->suggested_filename_ = suggested_filename_;
  data->is_ad_resource_ = is_ad_resource_;
  data->navigation_csp_ = initiator_csp_;

  return data;
}

bool ResourceRequest::IsNull() const {
  return url_.IsNull();
}

const KURL& ResourceRequest::Url() const {
  return url_;
}

void ResourceRequest::SetURL(const KURL& url) {
  url_ = url;
}

void ResourceRequest::RemoveUserAndPassFromURL() {
  if (url_.User().IsEmpty() && url_.Pass().IsEmpty())
    return;

  url_.SetUser(String());
  url_.SetPass(String());
}

mojom::FetchCacheMode ResourceRequest::GetCacheMode() const {
  return cache_mode_;
}

void ResourceRequest::SetCacheMode(mojom::FetchCacheMode cache_mode) {
  cache_mode_ = cache_mode;
}

double ResourceRequest::TimeoutInterval() const {
  return timeout_interval_;
}

void ResourceRequest::SetTimeoutInterval(double timout_interval_seconds) {
  timeout_interval_ = timout_interval_seconds;
}

const KURL& ResourceRequest::SiteForCookies() const {
  return site_for_cookies_;
}

void ResourceRequest::SetSiteForCookies(const KURL& site_for_cookies) {
  site_for_cookies_ = site_for_cookies;
}

scoped_refptr<const SecurityOrigin> ResourceRequest::RequestorOrigin() const {
  return requestor_origin_;
}

void ResourceRequest::SetRequestorOrigin(
    scoped_refptr<const SecurityOrigin> requestor_origin) {
  requestor_origin_ = std::move(requestor_origin);
}

const AtomicString& ResourceRequest::HttpMethod() const {
  return http_method_;
}

void ResourceRequest::SetHTTPMethod(const AtomicString& http_method) {
  http_method_ = http_method;
}

const HTTPHeaderMap& ResourceRequest::HttpHeaderFields() const {
  return http_header_fields_;
}

const AtomicString& ResourceRequest::HttpHeaderField(
    const AtomicString& name) const {
  return http_header_fields_.Get(name);
}

void ResourceRequest::SetHTTPHeaderField(const AtomicString& name,
                                         const AtomicString& value) {
  http_header_fields_.Set(name, value);
}

void ResourceRequest::SetHTTPReferrer(const Referrer& referrer) {
  if (referrer.referrer.IsEmpty())
    http_header_fields_.Remove(HTTPNames::Referer);
  else
    SetHTTPHeaderField(HTTPNames::Referer, referrer.referrer);
  referrer_policy_ = referrer.referrer_policy;
  did_set_http_referrer_ = true;
}

void ResourceRequest::ClearHTTPReferrer() {
  http_header_fields_.Remove(HTTPNames::Referer);
  referrer_policy_ = kReferrerPolicyDefault;
  did_set_http_referrer_ = false;
}

void ResourceRequest::SetHTTPOrigin(const SecurityOrigin* origin) {
  SetHTTPHeaderField(HTTPNames::Origin, origin->ToAtomicString());
}

void ResourceRequest::ClearHTTPOrigin() {
  http_header_fields_.Remove(HTTPNames::Origin);
}

void ResourceRequest::SetHTTPOriginIfNeeded(const SecurityOrigin* origin) {
  if (NeedsHTTPOrigin())
    SetHTTPOrigin(origin);
}

void ResourceRequest::SetHTTPOriginToMatchReferrerIfNeeded() {
  if (NeedsHTTPOrigin()) {
    SetHTTPOrigin(
        SecurityOrigin::CreateFromString(HttpHeaderField(HTTPNames::Referer))
            .get());
  }
}

void ResourceRequest::ClearHTTPUserAgent() {
  http_header_fields_.Remove(HTTPNames::User_Agent);
}

EncodedFormData* ResourceRequest::HttpBody() const {
  return http_body_.get();
}

void ResourceRequest::SetHTTPBody(scoped_refptr<EncodedFormData> http_body) {
  http_body_ = std::move(http_body);
}

bool ResourceRequest::AllowStoredCredentials() const {
  return allow_stored_credentials_;
}

void ResourceRequest::SetAllowStoredCredentials(bool allow_credentials) {
  allow_stored_credentials_ = allow_credentials;
}

ResourceLoadPriority ResourceRequest::Priority() const {
  return priority_;
}

int ResourceRequest::IntraPriorityValue() const {
  return intra_priority_value_;
}

void ResourceRequest::SetPriority(ResourceLoadPriority priority,
                                  int intra_priority_value) {
  priority_ = priority;
  intra_priority_value_ = intra_priority_value;
}

void ResourceRequest::AddHTTPHeaderField(const AtomicString& name,
                                         const AtomicString& value) {
  HTTPHeaderMap::AddResult result = http_header_fields_.Add(name, value);
  if (!result.is_new_entry)
    result.stored_value->value = result.stored_value->value + ", " + value;
}

void ResourceRequest::AddHTTPHeaderFields(const HTTPHeaderMap& header_fields) {
  HTTPHeaderMap::const_iterator end = header_fields.end();
  for (HTTPHeaderMap::const_iterator it = header_fields.begin(); it != end;
       ++it)
    AddHTTPHeaderField(it->key, it->value);
}

void ResourceRequest::ClearHTTPHeaderField(const AtomicString& name) {
  http_header_fields_.Remove(name);
}

void ResourceRequest::SetExternalRequestStateFromRequestorAddressSpace(
    mojom::IPAddressSpace requestor_space) {
  static_assert(mojom::IPAddressSpace::kLocal < mojom::IPAddressSpace::kPrivate,
                "Local is inside Private");
  static_assert(mojom::IPAddressSpace::kLocal < mojom::IPAddressSpace::kPublic,
                "Local is inside Public");
  static_assert(
      mojom::IPAddressSpace::kPrivate < mojom::IPAddressSpace::kPublic,
      "Private is inside Public");

  // TODO(mkwst): This only checks explicit IP addresses. We'll have to move all
  // this up to //net and //content in order to have any real impact on gateway
  // attacks. That turns out to be a TON of work. https://crbug.com/378566
  if (!RuntimeEnabledFeatures::CorsRFC1918Enabled()) {
    is_external_request_ = false;
    return;
  }

  mojom::IPAddressSpace target_space = mojom::IPAddressSpace::kPublic;
  if (NetworkUtils::IsReservedIPAddress(url_.Host()))
    target_space = mojom::IPAddressSpace::kPrivate;
  if (SecurityOrigin::Create(url_)->IsLocalhost())
    target_space = mojom::IPAddressSpace::kLocal;

  is_external_request_ = requestor_space > target_space;
}

void ResourceRequest::SetNavigationStartTime(TimeTicks navigation_start) {
  navigation_start_ = navigation_start;
}

bool ResourceRequest::IsConditional() const {
  return (http_header_fields_.Contains(HTTPNames::If_Match) ||
          http_header_fields_.Contains(HTTPNames::If_Modified_Since) ||
          http_header_fields_.Contains(HTTPNames::If_None_Match) ||
          http_header_fields_.Contains(HTTPNames::If_Range) ||
          http_header_fields_.Contains(HTTPNames::If_Unmodified_Since));
}

void ResourceRequest::SetHasUserGesture(bool has_user_gesture) {
  has_user_gesture_ |= has_user_gesture;
}

const CacheControlHeader& ResourceRequest::GetCacheControlHeader() const {
  if (!cache_control_header_cache_.parsed) {
    cache_control_header_cache_ = ParseCacheControlDirectives(
        http_header_fields_.Get(HTTPNames::Cache_Control),
        http_header_fields_.Get(HTTPNames::Pragma));
  }
  return cache_control_header_cache_;
}

bool ResourceRequest::CacheControlContainsNoCache() const {
  return GetCacheControlHeader().contains_no_cache;
}

bool ResourceRequest::CacheControlContainsNoStore() const {
  return GetCacheControlHeader().contains_no_store;
}

bool ResourceRequest::HasCacheValidatorFields() const {
  return !http_header_fields_.Get(HTTPNames::Last_Modified).IsEmpty() ||
         !http_header_fields_.Get(HTTPNames::ETag).IsEmpty();
}

bool ResourceRequest::NeedsHTTPOrigin() const {
  if (!HttpOrigin().IsEmpty())
    return false;  // Request already has an Origin header.

  // Don't send an Origin header for GET or HEAD to avoid privacy issues.
  // For example, if an intranet page has a hyperlink to an external web
  // site, we don't want to include the Origin of the request because it
  // will leak the internal host name. Similar privacy concerns have lead
  // to the widespread suppression of the Referer header at the network
  // layer.
  if (HttpMethod() == HTTPNames::GET || HttpMethod() == HTTPNames::HEAD)
    return false;

  // For non-GET and non-HEAD methods, always send an Origin header so the
  // server knows we support this feature.
  return true;
}

}  // namespace blink
