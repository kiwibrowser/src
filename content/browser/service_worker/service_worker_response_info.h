// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_SERVICE_WORKER_SERVICE_WORKER_RESPONSE_INFO_H_
#define CONTENT_BROWSER_SERVICE_WORKER_SERVICE_WORKER_RESPONSE_INFO_H_

#include <vector>

#include "base/supports_user_data.h"
#include "content/common/content_export.h"
#include "content/common/service_worker/service_worker_types.h"
#include "services/network/public/mojom/fetch_api.mojom.h"
#include "url/gurl.h"

namespace net {
class URLRequest;
}

namespace network {
struct ResourceResponseInfo;
}

namespace content {


class CONTENT_EXPORT ServiceWorkerResponseInfo
    : public base::SupportsUserData::Data {
 public:
  static ServiceWorkerResponseInfo* ForRequest(net::URLRequest* request,
                                               bool create = false);
  static void ResetDataForRequest(net::URLRequest* request);

  ~ServiceWorkerResponseInfo() override;

  void GetExtraResponseInfo(network::ResourceResponseInfo* response_info) const;
  void OnPrepareToRestart(base::TimeTicks service_worker_start_time,
                          base::TimeTicks service_worker_ready_time,
                          bool did_navigation_preload);
  void OnStartCompleted(
      bool was_fetched_via_service_worker,
      bool was_fallback_required,
      const std::vector<GURL>& url_list_via_service_worker,
      network::mojom::FetchResponseType response_type_via_service_worker,
      base::TimeTicks service_worker_start_time,
      base::TimeTicks service_worker_ready_time,
      bool response_is_in_cache_storage,
      const std::string& response_cache_storage_cache_name,
      const ServiceWorkerHeaderList& cors_exposed_header_names,
      bool did_navigation_preload);
  void ResetData();

  // Returns true if a service worker responded to the request. If the service
  // worker received a fetch event and did not call respondWith(), or was
  // bypassed due to absence of a fetch event handler, this function
  // typically returns false but returns true if "fallback to renderer" was
  // required (however in this case the response is not an actual resource and
  // the request will be reissued by the renderer).
  bool was_fetched_via_service_worker() const {
    return was_fetched_via_service_worker_;
  }

  // Returns true if "fallback to renderer" was required. This happens when a
  // request was directed to a service worker but it did not call respondWith()
  // or was bypassed due to absense of a fetch event handler, and the browser
  // could not directly fall back to network because the CORS algorithm must be
  // run, which is implemented in the renderer.
  bool was_fallback_required() const { return was_fallback_required_; }

  // Returns the URL list of the Response object the service worker passed to
  // respondWith() to create this response. For example, if the service worker
  // calls respondWith(fetch('http://example.com/a')) and http://example.com/a
  // redirects to http://example.net/b which redirects to http://example.org/c,
  // the URL list is the vector <"http://example.com/a", "http://example.net/b",
  // "http://example.org/c">. This is empty if the response was programatically
  // generated as in respondWith(new Response()). It is also empty if a service
  // worker did not respond to the request or did not call respondWith().
  const std::vector<GURL>& url_list_via_service_worker() const {
    return url_list_via_service_worker_;
  }
  network::mojom::FetchResponseType response_type_via_service_worker() const {
    return response_type_via_service_worker_;
  }
  base::TimeTicks service_worker_start_time() const {
    return service_worker_start_time_;
  }
  base::TimeTicks service_worker_ready_time() const {
    return service_worker_ready_time_;
  }
  bool response_is_in_cache_storage() const {
    return response_is_in_cache_storage_;
  }
  const std::string& response_cache_storage_cache_name() const {
    return response_cache_storage_cache_name_;
  }
  bool did_navigation_preload() const { return did_navigation_preload_; }

 private:
  ServiceWorkerResponseInfo();

  bool was_fetched_via_service_worker_ = false;
  bool was_fallback_required_ = false;
  std::vector<GURL> url_list_via_service_worker_;
  network::mojom::FetchResponseType response_type_via_service_worker_ =
      network::mojom::FetchResponseType::kDefault;
  base::TimeTicks service_worker_start_time_;
  base::TimeTicks service_worker_ready_time_;
  bool response_is_in_cache_storage_ = false;
  std::string response_cache_storage_cache_name_;
  ServiceWorkerHeaderList cors_exposed_header_names_;
  bool did_navigation_preload_ = false;

  static int user_data_key_;  // Only address is used.
};

}  // namespace content

#endif  // CONTENT_BROWSER_SERVICE_WORKER_SERVICE_WORKER_RESPONSE_INFO_H_
