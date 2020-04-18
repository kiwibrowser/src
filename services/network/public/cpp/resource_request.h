// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_NETWORK_PUBLIC_CPP_RESOURCE_REQUEST_H_
#define SERVICES_NETWORK_PUBLIC_CPP_RESOURCE_REQUEST_H_

#include <stdint.h>
#include <string>

#include "base/component_export.h"
#include "base/memory/ref_counted.h"
#include "base/optional.h"
#include "net/base/request_priority.h"
#include "net/http/http_request_headers.h"
#include "net/url_request/url_request.h"
#include "services/network/public/cpp/resource_request_body.h"
#include "services/network/public/mojom/cors.mojom-shared.h"
#include "services/network/public/mojom/fetch_api.mojom-shared.h"
#include "services/network/public/mojom/request_context_frame_type.mojom-shared.h"
#include "url/gurl.h"
#include "url/origin.h"

namespace network {

struct COMPONENT_EXPORT(NETWORK_CPP_BASE) ResourceRequest {
  ResourceRequest();
  ResourceRequest(const ResourceRequest& request);
  ~ResourceRequest();

  // The request method: GET, POST, etc.
  std::string method = "GET";

  // The absolute requested URL encoded in ASCII per the rules of RFC-2396.
  GURL url;

  // URL representing the first-party origin for the request, which may be
  // checked by the third-party cookie blocking policy. This is usually the URL
  // of the document in the top-level window. Leaving it empty may lead to
  // undesired cookie blocking. Third-party cookie blocking can be bypassed by
  // setting site_for_cookies = url, but this should ideally only be
  // done if there really is no way to determine the correct value.
  GURL site_for_cookies;

  // Boolean indicating whether SameSite cookies are allowed to be attached
  // to the request. It should be used as additional input to network side
  // checks.
  bool attach_same_site_cookies = false;

  // First-party URL redirect policy: During server redirects, the first-party
  // URL for cookies normally doesn't change. However, if this is true, the
  // the first-party URL should be updated to the URL on every redirect.
  bool update_first_party_url_on_redirect = false;

  // The origin of the context which initiated the request, which will be used
  // for cookie checks like 'First-Party-Only'.
  base::Optional<url::Origin> request_initiator;

  // The referrer to use (may be empty).
  GURL referrer;

  // The referrer policy to use.
  net::URLRequest::ReferrerPolicy referrer_policy =
      net::URLRequest::NEVER_CLEAR_REFERRER;

  // Whether the frame that initiated this request is used for prerendering.
  bool is_prerendering = false;

  // Additional HTTP request headers.
  net::HttpRequestHeaders headers;

  // net::URLRequest load flags (0 by default).
  int load_flags = 0;

  // If this request originated from a pepper plugin running in a child
  // process, this identifies which process it came from. Otherwise, it
  // is zero.
  // -1 to match ChildProcessHost::kInvalidUniqueID
  // TODO(jam): remove this from the struct since network service shouldn't know
  // about this.
  int plugin_child_id = -1;

  // What this resource load is for (main frame, sub-frame, sub-resource,
  // object).
  // Note: this is an enum of type content::ResourceType.
  // TODO(jam): remove this from the struct since network service shouldn't know
  // about this.
  int resource_type = 0;

  // The priority of this request determined by Blink.
  net::RequestPriority priority = net::IDLE;

  // Indicates which frame (or worker context) the request is being loaded into,
  // or kAppCacheNoHostId.
  int appcache_host_id = 0;

  // True if corresponding AppCache group should be resetted.
  bool should_reset_appcache = false;

  // https://wicg.github.io/cors-rfc1918/#external-request
  // TODO(toyoshim): The browser should know better than renderers do.
  // This is used to plumb Blink decided information for legacy code path, but
  // eventually we should remove this.
  bool is_external_request = false;

  // A policy to decide if CORS-preflight fetch should be performed.
  mojom::CORSPreflightPolicy cors_preflight_policy =
      mojom::CORSPreflightPolicy::kConsiderPreflight;

  // Indicates which frame (or worker context) the request is being loaded into.
  // -1 corresponds to kInvalidServiceWorkerProviderId.
  // TODO(jam): remove this from the struct since network service shouldn't know
  // about this.
  int service_worker_provider_id = -1;

  // True if the request originated from a Service Worker, e.g. due to a
  // fetch() in the Service Worker script.
  bool originated_from_service_worker = false;

  // The service worker mode that indicates which service workers should get
  // events for this request.
  // TODO(jam): remove this from the struct since network service shouldn't know
  // about this.
  bool skip_service_worker = false;

  // The request mode passed to the ServiceWorker.
  mojom::FetchRequestMode fetch_request_mode =
      mojom::FetchRequestMode::kSameOrigin;

  // The credentials mode passed to the ServiceWorker.
  mojom::FetchCredentialsMode fetch_credentials_mode =
      mojom::FetchCredentialsMode::kOmit;

  // The redirect mode used in Fetch API.
  mojom::FetchRedirectMode fetch_redirect_mode =
      mojom::FetchRedirectMode::kFollow;

  // The integrity used in Fetch API.
  std::string fetch_integrity;

  // The request context passed to the ServiceWorker.
  // Note: this is an enum of type content::RequestContextType.
  // TODO(jam): remove this from the struct since network service shouldn't know
  // about this.
  int fetch_request_context_type = 0;

  // The frame type passed to the ServiceWorker.
  mojom::RequestContextFrameType fetch_frame_type =
      mojom::RequestContextFrameType::kAuxiliary;

  // Optional resource request body (may be null).
  scoped_refptr<ResourceRequestBody> request_body;

  // If true, then the response body will be downloaded to a file and the path
  // to that file will be provided in ResponseInfo::download_file_path.
  // Deprecated and not supported by the network service code.
  // TODO(mek): Remove this flag once all usage of it is gone (XHR and PPAPI).
  bool download_to_file = false;

  // True if the request can work after the fetch group is terminated.
  // https://fetch.spec.whatwg.org/#request-keepalive-flag
  bool keepalive = false;

  // True if the request was user initiated.
  bool has_user_gesture = false;

  // TODO(mmenke): Investigate if enable_load_timing is safe to remove.
  //
  // True if load timing data should be collected for request.
  bool enable_load_timing = false;

  // True if upload progress should be available for request.
  bool enable_upload_progress = false;

  // True if login prompts for this request should be supressed. Cached
  // credentials or default credentials may still be used for authentication.
  bool do_not_prompt_for_login = false;

  // The routing id of the RenderFrame.
  int render_frame_id = 0;

  // True if |frame_id| is the main frame of a RenderView.
  bool is_main_frame = false;

  // Note: this is an enum of type ui::PageTransition.
  // TODO(jam): remove this from the struct since network service shouldn't know
  // about this.
  int transition_type = 0;

  // Whether or not we should allow the URL to download.
  bool allow_download = false;

  // Whether to intercept headers to pass back to the renderer.
  // This also enables reporting of SSLInfo in URLLoaderClient's
  // OnResponseReceived and OnComplete, as well as invocation of
  // OnTransferSizeUpdated().
  bool report_raw_headers = false;

  // Whether or not to request a Preview version of the resource or let the
  // browser decide.
  // Note: this is an enum of type PreviewsState.
  // TODO(jam): remove this from the struct since network service shouldn't know
  // about this.
  int previews_state = 0;

  // Wether or not the initiator of this request is a secure context.
  bool initiated_in_secure_context = false;
};

}  // namespace network

#endif  // SERVICES_NETWORK_PUBLIC_CPP_RESOURCE_REQUEST_H_
