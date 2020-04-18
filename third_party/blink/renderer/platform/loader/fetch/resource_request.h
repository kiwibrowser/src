/*
 * Copyright (C) 2003, 2006 Apple Computer, Inc.  All rights reserved.
 * Copyright (C) 2006 Samuel Weinig <sam.weinig@gmail.com>
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

#ifndef THIRD_PARTY_BLINK_RENDERER_PLATFORM_LOADER_FETCH_RESOURCE_REQUEST_H_
#define THIRD_PARTY_BLINK_RENDERER_PLATFORM_LOADER_FETCH_RESOURCE_REQUEST_H_

#include <memory>
#include "base/optional.h"
#include "services/network/public/mojom/cors.mojom-blink.h"
#include "services/network/public/mojom/fetch_api.mojom-blink.h"
#include "services/network/public/mojom/request_context_frame_type.mojom-shared.h"
#include "third_party/blink/public/mojom/net/ip_address_space.mojom-blink.h"
#include "third_party/blink/public/platform/modules/fetch/fetch_api_request.mojom-shared.h"
#include "third_party/blink/public/platform/resource_request_blocked_reason.h"
#include "third_party/blink/public/platform/web_content_security_policy_struct.h"
#include "third_party/blink/public/platform/web_url_request.h"
#include "third_party/blink/renderer/platform/loader/fetch/resource_load_priority.h"
#include "third_party/blink/renderer/platform/network/encoded_form_data.h"
#include "third_party/blink/renderer/platform/network/http_header_map.h"
#include "third_party/blink/renderer/platform/network/http_names.h"
#include "third_party/blink/renderer/platform/network/http_parsers.h"
#include "third_party/blink/renderer/platform/weborigin/kurl.h"
#include "third_party/blink/renderer/platform/weborigin/referrer.h"
#include "third_party/blink/renderer/platform/weborigin/security_origin.h"
#include "third_party/blink/renderer/platform/wtf/ref_counted.h"

namespace blink {

enum InputToLoadPerfMetricReportPolicy : uint8_t {
  kNoReport,    // Don't report metrics for this ResourceRequest.
  kReportLink,  // Report metrics for this request as initiated by a link click.
  kReportIntent,  // Report metrics for this request as initiated by an intent.
};

struct CrossThreadResourceRequestData;

// A ResourceRequest is a "request" object for ResourceLoader. Conceptually
// it is https://fetch.spec.whatwg.org/#concept-request, but it contains
// a lot of blink specific fields. WebURLRequest is the "public version"
// of this class and WebURLLoader needs it. See WebURLRequest and
// WrappedResourceRequest.
//
// There are cases where we need to copy a request across threads, and
// CrossThreadResourceRequestData is a struct for the purpose. When you add a
// member variable to this class, do not forget to add the corresponding
// one in CrossThreadResourceRequestData and write copying logic.
class PLATFORM_EXPORT ResourceRequest final {
  USING_FAST_MALLOC(ResourceRequest);

 public:
  enum class RedirectStatus : uint8_t { kFollowedRedirect, kNoRedirect };

  ResourceRequest();
  explicit ResourceRequest(const String& url_string);
  explicit ResourceRequest(const KURL&);
  explicit ResourceRequest(CrossThreadResourceRequestData*);

  // TODO(toyoshim): Use std::unique_ptr as much as possible, and hopefully
  // make ResourceRequest WTF_MAKE_NONCOPYABLE. See crbug.com/787704.
  ResourceRequest(const ResourceRequest&);
  ResourceRequest& operator=(const ResourceRequest&);

  // Constructs a new ResourceRequest for a redirect from this instance.
  std::unique_ptr<ResourceRequest> CreateRedirectRequest(
      const KURL& new_url,
      const AtomicString& new_method,
      const KURL& new_site_for_cookies,
      const String& new_referrer,
      ReferrerPolicy new_referrer_policy,
      bool skip_service_worker) const;

  // Gets a copy of the data suitable for passing to another thread.
  std::unique_ptr<CrossThreadResourceRequestData> CopyData() const;

  bool IsNull() const;

  const KURL& Url() const;
  void SetURL(const KURL&);

  void RemoveUserAndPassFromURL();

  mojom::FetchCacheMode GetCacheMode() const;
  void SetCacheMode(mojom::FetchCacheMode);

  double TimeoutInterval() const;  // May return 0 when using platform default.
  void SetTimeoutInterval(double);

  const KURL& SiteForCookies() const;
  void SetSiteForCookies(const KURL&);

  scoped_refptr<const SecurityOrigin> RequestorOrigin() const;
  void SetRequestorOrigin(scoped_refptr<const SecurityOrigin>);

  const AtomicString& HttpMethod() const;
  void SetHTTPMethod(const AtomicString&);

  const HTTPHeaderMap& HttpHeaderFields() const;
  const AtomicString& HttpHeaderField(const AtomicString& name) const;
  void SetHTTPHeaderField(const AtomicString& name, const AtomicString& value);
  void AddHTTPHeaderField(const AtomicString& name, const AtomicString& value);
  void AddHTTPHeaderFields(const HTTPHeaderMap& header_fields);
  void ClearHTTPHeaderField(const AtomicString& name);

  const AtomicString& HttpContentType() const {
    return HttpHeaderField(HTTPNames::Content_Type);
  }
  void SetHTTPContentType(const AtomicString& http_content_type) {
    SetHTTPHeaderField(HTTPNames::Content_Type, http_content_type);
  }

  bool DidSetHTTPReferrer() const { return did_set_http_referrer_; }
  const AtomicString& HttpReferrer() const {
    return HttpHeaderField(HTTPNames::Referer);
  }
  ReferrerPolicy GetReferrerPolicy() const { return referrer_policy_; }
  void SetHTTPReferrer(const Referrer&);
  void ClearHTTPReferrer();

  const AtomicString& HttpOrigin() const {
    return HttpHeaderField(HTTPNames::Origin);
  }
  void SetHTTPOrigin(const SecurityOrigin*);
  void ClearHTTPOrigin();
  void SetHTTPOriginIfNeeded(const SecurityOrigin*);
  void SetHTTPOriginToMatchReferrerIfNeeded();

  void SetHTTPUserAgent(const AtomicString& http_user_agent) {
    SetHTTPHeaderField(HTTPNames::User_Agent, http_user_agent);
  }
  void ClearHTTPUserAgent();

  void SetHTTPAccept(const AtomicString& http_accept) {
    SetHTTPHeaderField(HTTPNames::Accept, http_accept);
  }

  EncodedFormData* HttpBody() const;
  void SetHTTPBody(scoped_refptr<EncodedFormData>);

  bool AllowStoredCredentials() const;
  void SetAllowStoredCredentials(bool allow_credentials);

  // TODO(yhirano): Describe what Priority and IntraPriorityValue are.
  ResourceLoadPriority Priority() const;
  int IntraPriorityValue() const;
  void SetPriority(ResourceLoadPriority, int intra_priority_value = 0);

  bool IsConditional() const;

  // Whether the associated ResourceHandleClient needs to be notified of
  // upload progress made for that resource.
  bool ReportUploadProgress() const { return report_upload_progress_; }
  void SetReportUploadProgress(bool report_upload_progress) {
    report_upload_progress_ = report_upload_progress;
  }

  // Whether actual headers being sent/received should be collected and reported
  // for the request.
  bool ReportRawHeaders() const { return report_raw_headers_; }
  void SetReportRawHeaders(bool report_raw_headers) {
    report_raw_headers_ = report_raw_headers;
  }

  // Allows the request to be matched up with its requestor.
  int RequestorID() const { return requestor_id_; }
  void SetRequestorID(int requestor_id) { requestor_id_ = requestor_id; }

  // The unique child id (not PID) of the process from which this request
  // originated. In the case of out-of-process plugins, this allows to link back
  // the request to the plugin process (as it is processed through a render view
  // process).
  int GetPluginChildID() const { return plugin_child_id_; }
  void SetPluginChildID(int plugin_child_id) {
    plugin_child_id_ = plugin_child_id;
  }

  // Allows the request to be matched up with its app cache host.
  int AppCacheHostID() const { return app_cache_host_id_; }
  void SetAppCacheHostID(int id) { app_cache_host_id_ = id; }

  // True if request was user initiated.
  bool HasUserGesture() const { return has_user_gesture_; }
  void SetHasUserGesture(bool);

  // True if request should be downloaded to file.
  bool DownloadToFile() const { return download_to_file_; }
  void SetDownloadToFile(bool download_to_file) {
    download_to_file_ = download_to_file;
  }

  // True if request shuold be downloaded to blob.
  bool DownloadToBlob() const { return download_to_blob_; }
  void SetDownloadToBlob(bool download_to_blob) {
    download_to_blob_ = download_to_blob;
  }

  // True if the requestor wants to receive a response body as
  // WebDataConsumerHandle.
  bool UseStreamOnResponse() const { return use_stream_on_response_; }
  void SetUseStreamOnResponse(bool use_stream_on_response) {
    use_stream_on_response_ = use_stream_on_response;
  }

  // True if the request can work after the fetch group is terminated.
  bool GetKeepalive() const { return keepalive_; }
  void SetKeepalive(bool keepalive) { keepalive_ = keepalive; }

  // True if service workers should not get events for the request.
  bool GetSkipServiceWorker() const { return skip_service_worker_; }
  void SetSkipServiceWorker(bool skip_service_worker) {
    skip_service_worker_ = skip_service_worker;
  }

  // True if corresponding AppCache group should be resetted.
  bool ShouldResetAppCache() const { return should_reset_app_cache_; }
  void SetShouldResetAppCache(bool should_reset_app_cache) {
    should_reset_app_cache_ = should_reset_app_cache;
  }

  // Extra data associated with this request.
  WebURLRequest::ExtraData* GetExtraData() const {
    return sharable_extra_data_ ? sharable_extra_data_->data.get() : nullptr;
  }
  void SetExtraData(std::unique_ptr<WebURLRequest::ExtraData> extra_data) {
    if (extra_data) {
      sharable_extra_data_ =
          base::MakeRefCounted<SharableExtraData>(std::move(extra_data));
    } else {
      sharable_extra_data_ = nullptr;
    }
  }

  WebURLRequest::RequestContext GetRequestContext() const {
    return request_context_;
  }
  void SetRequestContext(WebURLRequest::RequestContext context) {
    request_context_ = context;
  }

  network::mojom::RequestContextFrameType GetFrameType() const {
    return frame_type_;
  }
  void SetFrameType(network::mojom::RequestContextFrameType frame_type) {
    frame_type_ = frame_type;
  }

  network::mojom::FetchRequestMode GetFetchRequestMode() const {
    return fetch_request_mode_;
  }
  void SetFetchRequestMode(network::mojom::FetchRequestMode mode) {
    fetch_request_mode_ = mode;
  }

  // A resource request's fetch_importance_mode_ is a developer-set priority
  // hint that differs from priority_. It is used in
  // ResourceFetcher::ComputeLoadPriority to possibly influence the resolved
  // priority of a resource request.
  // This member exists both here and in FetchParameters, as opposed just in
  // the latter because the fetch() API creates a ResourceRequest object long
  // before its associaed FetchParameters, so this makes it easier to
  // communicate an importance value down to the lower-level fetching code.
  mojom::FetchImportanceMode GetFetchImportanceMode() const {
    return fetch_importance_mode_;
  }
  void SetFetchImportanceMode(mojom::FetchImportanceMode mode) {
    fetch_importance_mode_ = mode;
  }

  network::mojom::FetchCredentialsMode GetFetchCredentialsMode() const {
    return fetch_credentials_mode_;
  }
  void SetFetchCredentialsMode(network::mojom::FetchCredentialsMode mode) {
    fetch_credentials_mode_ = mode;
  }

  network::mojom::FetchRedirectMode GetFetchRedirectMode() const {
    return fetch_redirect_mode_;
  }
  void SetFetchRedirectMode(network::mojom::FetchRedirectMode redirect) {
    fetch_redirect_mode_ = redirect;
  }

  const String& GetFetchIntegrity() const { return fetch_integrity_; }
  void SetFetchIntegrity(const String& integrity) {
    fetch_integrity_ = integrity;
  }

  WebURLRequest::PreviewsState GetPreviewsState() const {
    return previews_state_;
  }
  void SetPreviewsState(WebURLRequest::PreviewsState previews_state) {
    previews_state_ = previews_state;
  }

  bool CacheControlContainsNoCache() const;
  bool CacheControlContainsNoStore() const;
  bool HasCacheValidatorFields() const;

  bool CheckForBrowserSideNavigation() const {
    return check_for_browser_side_navigation_;
  }
  void SetCheckForBrowserSideNavigation(bool check) {
    check_for_browser_side_navigation_ = check;
  }

  bool WasDiscarded() const { return was_discarded_; }
  void SetWasDiscarded(bool was_discarded) { was_discarded_ = was_discarded; }

  double UiStartTime() const { return ui_start_time_; }
  void SetUIStartTime(double ui_start_time_seconds) {
    ui_start_time_ = ui_start_time_seconds;
  }

  // https://wicg.github.io/cors-rfc1918/#external-request
  bool IsExternalRequest() const { return is_external_request_; }
  void SetExternalRequestStateFromRequestorAddressSpace(mojom::IPAddressSpace);

  network::mojom::CORSPreflightPolicy CORSPreflightPolicy() const {
    return cors_preflight_policy_;
  }
  void SetCORSPreflightPolicy(network::mojom::CORSPreflightPolicy policy) {
    cors_preflight_policy_ = policy;
  }

  InputToLoadPerfMetricReportPolicy InputPerfMetricReportPolicy() const {
    return input_perf_metric_report_policy_;
  }
  void SetInputPerfMetricReportPolicy(
      InputToLoadPerfMetricReportPolicy input_perf_metric_report_policy) {
    input_perf_metric_report_policy_ = input_perf_metric_report_policy;
  }

  void SetRedirectStatus(RedirectStatus status) { redirect_status_ = status; }
  RedirectStatus GetRedirectStatus() const { return redirect_status_; }

  void SetSuggestedFilename(const base::Optional<String>& suggested_filename) {
    suggested_filename_ = suggested_filename;
  }
  const base::Optional<String>& GetSuggestedFilename() const {
    return suggested_filename_;
  }

  void SetNavigationStartTime(TimeTicks);
  TimeTicks NavigationStartTime() const { return navigation_start_; }

  void SetIsSameDocumentNavigation(bool is_same_document) {
    is_same_document_navigation_ = is_same_document;
  }
  bool IsSameDocumentNavigation() const { return is_same_document_navigation_; }

  void SetIsAdResource() { is_ad_resource_ = true; }
  bool IsAdResource() const { return is_ad_resource_; }

  void SetInitiatorCSP(const WebContentSecurityPolicyList& initiator_csp) {
    initiator_csp_ = initiator_csp;
  }
  const WebContentSecurityPolicyList& GetInitiatorCSP() const {
    return initiator_csp_;
  }

 private:
  using SharableExtraData =
      base::RefCountedData<std::unique_ptr<WebURLRequest::ExtraData>>;

  const CacheControlHeader& GetCacheControlHeader() const;

  bool NeedsHTTPOrigin() const;

  KURL url_;
  double timeout_interval_;  // 0 is a magic value for platform default on
                             // platforms that have one.
  KURL site_for_cookies_;

  // The SecurityOrigin specified by the ResourceLoaderOptions in case e.g.
  // when the fetching was initiated in an isolated world. Set by
  // ResourceFetcher but only when needed.
  //
  // TODO(crbug.com/811669): Merge with some of the other origin variables.
  scoped_refptr<const SecurityOrigin> requestor_origin_;

  AtomicString http_method_;
  HTTPHeaderMap http_header_fields_;
  scoped_refptr<EncodedFormData> http_body_;
  bool allow_stored_credentials_ : 1;
  bool report_upload_progress_ : 1;
  bool report_raw_headers_ : 1;
  bool has_user_gesture_ : 1;
  bool download_to_file_ : 1;
  bool download_to_blob_ : 1;
  bool use_stream_on_response_ : 1;
  bool keepalive_ : 1;
  bool should_reset_app_cache_ : 1;
  mojom::FetchCacheMode cache_mode_;
  bool skip_service_worker_ : 1;
  ResourceLoadPriority priority_;
  int intra_priority_value_;
  int requestor_id_;
  int plugin_child_id_;
  int app_cache_host_id_;
  WebURLRequest::PreviewsState previews_state_;
  scoped_refptr<SharableExtraData> sharable_extra_data_;
  WebURLRequest::RequestContext request_context_;
  network::mojom::RequestContextFrameType frame_type_;
  network::mojom::FetchRequestMode fetch_request_mode_;
  mojom::FetchImportanceMode fetch_importance_mode_;
  network::mojom::FetchCredentialsMode fetch_credentials_mode_;
  network::mojom::FetchRedirectMode fetch_redirect_mode_;
  String fetch_integrity_;
  ReferrerPolicy referrer_policy_;
  bool did_set_http_referrer_;
  bool check_for_browser_side_navigation_;
  bool was_discarded_;
  double ui_start_time_;
  bool is_external_request_;
  network::mojom::CORSPreflightPolicy cors_preflight_policy_;
  bool is_same_document_navigation_;
  InputToLoadPerfMetricReportPolicy input_perf_metric_report_policy_;
  RedirectStatus redirect_status_;
  base::Optional<String> suggested_filename_;

  mutable CacheControlHeader cache_control_header_cache_;

  static double default_timeout_interval_;

  TimeTicks navigation_start_;

  bool is_ad_resource_ = false;
  WebContentSecurityPolicyList initiator_csp_;
};

// This class is needed to copy a ResourceRequest across threads, because it
// has some members which cannot be transferred across threads (AtomicString
// for example).
// There are some rules / restrictions:
//  - This struct cannot contain an object that cannot be transferred across
//    threads (e.g., AtomicString)
//  - Non-simple members need explicit copying (e.g., String::IsolatedCopy,
//    KURL::Copy) rather than the copy constructor or the assignment operator.
struct CrossThreadResourceRequestData {
  WTF_MAKE_NONCOPYABLE(CrossThreadResourceRequestData);
  USING_FAST_MALLOC(CrossThreadResourceRequestData);

 public:
  CrossThreadResourceRequestData() = default;
  KURL url_;

  mojom::FetchCacheMode cache_mode_;
  double timeout_interval_;
  KURL site_for_cookies_;
  scoped_refptr<const SecurityOrigin> requestor_origin_;

  String http_method_;
  std::unique_ptr<CrossThreadHTTPHeaderMapData> http_headers_;
  scoped_refptr<EncodedFormData> http_body_;
  bool allow_stored_credentials_;
  bool report_upload_progress_;
  bool has_user_gesture_;
  bool download_to_file_;
  bool download_to_blob_;
  bool skip_service_worker_;
  bool use_stream_on_response_;
  bool keepalive_;
  bool should_reset_app_cache_;
  ResourceLoadPriority priority_;
  int intra_priority_value_;
  int requestor_id_;
  int plugin_child_id_;
  int app_cache_host_id_;
  WebURLRequest::RequestContext request_context_;
  network::mojom::RequestContextFrameType frame_type_;
  network::mojom::FetchRequestMode fetch_request_mode_;
  mojom::FetchImportanceMode fetch_importance_mode_;
  network::mojom::FetchCredentialsMode fetch_credentials_mode_;
  network::mojom::FetchRedirectMode fetch_redirect_mode_;
  String fetch_integrity_;
  WebURLRequest::PreviewsState previews_state_;
  ReferrerPolicy referrer_policy_;
  bool did_set_http_referrer_;
  bool check_for_browser_side_navigation_;
  double ui_start_time_;
  bool is_external_request_;
  network::mojom::CORSPreflightPolicy cors_preflight_policy_;
  InputToLoadPerfMetricReportPolicy input_perf_metric_report_policy_;
  ResourceRequest::RedirectStatus redirect_status_;
  base::Optional<String> suggested_filename_;
  bool is_ad_resource_;
  WebContentSecurityPolicyList navigation_csp_;
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_PLATFORM_LOADER_FETCH_RESOURCE_REQUEST_H_
