// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/service_worker/service_worker_response_info.h"

#include "base/memory/ptr_util.h"
#include "net/url_request/url_request.h"
#include "services/network/public/cpp/resource_response_info.h"

namespace content {

// static
int ServiceWorkerResponseInfo::user_data_key_;

// static
ServiceWorkerResponseInfo* ServiceWorkerResponseInfo::ForRequest(
    net::URLRequest* request,
    bool create) {
  ServiceWorkerResponseInfo* info = static_cast<ServiceWorkerResponseInfo*>(
      request->GetUserData(&user_data_key_));
  if (!info && create) {
    info = new ServiceWorkerResponseInfo();
    request->SetUserData(&user_data_key_, base::WrapUnique(info));
  }
  return info;
}

// static
void ServiceWorkerResponseInfo::ResetDataForRequest(net::URLRequest* request) {
  ServiceWorkerResponseInfo* info = ForRequest(request);
  if (info)
    info->ResetData();
}

ServiceWorkerResponseInfo::~ServiceWorkerResponseInfo() {}

void ServiceWorkerResponseInfo::GetExtraResponseInfo(
    network::ResourceResponseInfo* response_info) const {
  response_info->was_fetched_via_service_worker =
      was_fetched_via_service_worker_;
  response_info->was_fallback_required_by_service_worker =
      was_fallback_required_;
  response_info->url_list_via_service_worker = url_list_via_service_worker_;
  response_info->response_type_via_service_worker =
      response_type_via_service_worker_;
  response_info->service_worker_start_time = service_worker_start_time_;
  response_info->service_worker_ready_time = service_worker_ready_time_;
  response_info->is_in_cache_storage = response_is_in_cache_storage_;
  response_info->cache_storage_cache_name = response_cache_storage_cache_name_;
  response_info->cors_exposed_header_names = cors_exposed_header_names_;
  response_info->did_service_worker_navigation_preload =
      did_navigation_preload_;
}

void ServiceWorkerResponseInfo::OnPrepareToRestart(
    base::TimeTicks service_worker_start_time,
    base::TimeTicks service_worker_ready_time,
    bool did_navigation_preload) {
  ResetData();

  // Update times, if not already set by a previous Job.
  if (service_worker_start_time_.is_null()) {
    service_worker_start_time_ = service_worker_start_time;
    service_worker_ready_time_ = service_worker_ready_time;
  }
  // Don't reset navigation preload flag it if a previous job already set it,
  // since the UseCounter should still reflect that navigation preload occurred
  // for this request.
  if (did_navigation_preload)
    did_navigation_preload_ = true;
}

void ServiceWorkerResponseInfo::OnStartCompleted(
    bool was_fetched_via_service_worker,
    bool was_fallback_required,
    const std::vector<GURL>& url_list_via_service_worker,
    network::mojom::FetchResponseType response_type_via_service_worker,
    base::TimeTicks service_worker_start_time,
    base::TimeTicks service_worker_ready_time,
    bool response_is_in_cache_storage,
    const std::string& response_cache_storage_cache_name,
    const ServiceWorkerHeaderList& cors_exposed_header_names,
    bool did_navigation_preload) {
  was_fetched_via_service_worker_ = was_fetched_via_service_worker;
  was_fallback_required_ = was_fallback_required;
  url_list_via_service_worker_ = url_list_via_service_worker;
  response_type_via_service_worker_ = response_type_via_service_worker;
  response_is_in_cache_storage_ = response_is_in_cache_storage;
  response_cache_storage_cache_name_ = response_cache_storage_cache_name;
  cors_exposed_header_names_ = cors_exposed_header_names;

  // Update times, if not already set by a previous Job.
  if (service_worker_start_time_.is_null()) {
    service_worker_start_time_ = service_worker_start_time;
    service_worker_ready_time_ = service_worker_ready_time;
  }

  did_navigation_preload_ = did_navigation_preload;
}

void ServiceWorkerResponseInfo::ResetData() {
  was_fetched_via_service_worker_ = false;
  was_fallback_required_ = false;
  url_list_via_service_worker_.clear();
  response_type_via_service_worker_ =
      network::mojom::FetchResponseType::kDefault;
  // Don't reset |service_worker_start_time_| or |service_worker_ready_time_|
  // since it's historical timing information that should persist between job
  // restarts.
  response_is_in_cache_storage_ = false;
  response_cache_storage_cache_name_ = std::string();
  cors_exposed_header_names_.clear();
  // Don't reset the |did_navigation_preload_| flag. This is used for the
  // UseCounter, and if it was ever true for a request, it should remain true
  // even if the job restarts.
}

ServiceWorkerResponseInfo::ServiceWorkerResponseInfo() {}

}  // namespace content
