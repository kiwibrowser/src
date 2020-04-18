// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/network/url_loader_factory.h"

#include <memory>
#include <utility>

#include "base/logging.h"
#include "services/network/network_context.h"
#include "services/network/network_service.h"
#include "services/network/network_usage_accumulator.h"
#include "services/network/public/cpp/resource_request.h"
#include "services/network/resource_scheduler_client.h"
#include "services/network/url_loader.h"

namespace network {

constexpr int URLLoaderFactory::kMaxKeepaliveConnections;
constexpr int URLLoaderFactory::kMaxKeepaliveConnectionsPerProcess;
constexpr int URLLoaderFactory::kMaxKeepaliveConnectionsPerProcessForFetchAPI;

URLLoaderFactory::URLLoaderFactory(
    NetworkContext* context,
    mojom::URLLoaderFactoryParamsPtr params,
    scoped_refptr<ResourceSchedulerClient> resource_scheduler_client,
    mojom::URLLoaderFactoryRequest request)
    : context_(context),
      params_(std::move(params)),
      resource_scheduler_client_(std::move(resource_scheduler_client)) {
  DCHECK(context);
  DCHECK_NE(mojom::kInvalidProcessId, params_->process_id);

  binding_set_.AddBinding(this, std::move(request));
  binding_set_.set_connection_error_handler(base::BindRepeating(
      &URLLoaderFactory::DeleteIfNeeded, base::Unretained(this)));

  if (context_->network_service()) {
    context_->network_service()->keepalive_statistics_recorder()->Register(
        params_->process_id);
  }
}

URLLoaderFactory::~URLLoaderFactory() {
  if (context_->network_service()) {
    context_->network_service()->keepalive_statistics_recorder()->Unregister(
        params_->process_id);
    // Reset bytes transferred for the process if this is the last
    // |URLLoaderFactory|.
    if (!context_->network_service()
             ->keepalive_statistics_recorder()
             ->HasRecordForProcess(params_->process_id)) {
      context_->network_service()
          ->network_usage_accumulator()
          ->ClearBytesTransferredForProcess(params_->process_id);
    }
  }
}

void URLLoaderFactory::CreateLoaderAndStart(
    mojom::URLLoaderRequest request,
    int32_t routing_id,
    int32_t request_id,
    uint32_t options,
    const ResourceRequest& url_request,
    mojom::URLLoaderClientPtr client,
    const net::MutableNetworkTrafficAnnotationTag& traffic_annotation) {
  DCHECK(!url_request.download_to_file);
  bool report_raw_headers = false;
  if (url_request.report_raw_headers) {
    const NetworkService* service = context_->network_service();
    report_raw_headers =
        service && service->HasRawHeadersAccess(params_->process_id);
    if (!report_raw_headers)
      DLOG(ERROR) << "Denying raw headers request by process "
                  << params_->process_id;
  }

  mojom::NetworkServiceClient* network_service_client = nullptr;
  base::WeakPtr<KeepaliveStatisticsRecorder> keepalive_statistics_recorder;
  base::WeakPtr<NetworkUsageAccumulator> network_usage_accumulator;
  if (context_->network_service()) {
    network_service_client = context_->network_service()->client();
    keepalive_statistics_recorder = context_->network_service()
                                        ->keepalive_statistics_recorder()
                                        ->AsWeakPtr();
    network_usage_accumulator =
        context_->network_service()->network_usage_accumulator()->AsWeakPtr();
  }

  if (url_request.keepalive && keepalive_statistics_recorder) {
    // This logic comes from
    // content::ResourceDispatcherHostImpl::BeginRequestInternal.
    bool exhausted = false;
    // This is needed because we want to know whether the request is initiated
    // by fetch() or not. We hope that we can unify these restrictions and
    // remove the reference to fetch_request_context_type in the future.
    constexpr uint32_t kInitiatedByFetchAPI = 8;
    const bool is_initiated_by_fetch_api =
        url_request.fetch_request_context_type == kInitiatedByFetchAPI;
    const auto& recorder = *keepalive_statistics_recorder;
    if (recorder.num_inflight_requests() >= kMaxKeepaliveConnections)
      exhausted = true;
    if (recorder.NumInflightRequestsPerProcess(params_->process_id) >=
        kMaxKeepaliveConnectionsPerProcess) {
      exhausted = true;
    }
    if (is_initiated_by_fetch_api &&
        recorder.NumInflightRequestsPerProcess(params_->process_id) >=
            kMaxKeepaliveConnectionsPerProcessForFetchAPI) {
      exhausted = true;
    }
    if (exhausted) {
      if (client) {
        URLLoaderCompletionStatus status;
        status.error_code = net::ERR_INSUFFICIENT_RESOURCES;
        status.exists_in_cache = false;
        status.completion_time = base::TimeTicks::Now();
        client->OnComplete(status);
      }
      return;
    }
  }

  url_loaders_.insert(std::make_unique<URLLoader>(
      context_->url_request_context(), network_service_client,
      base::BindOnce(&URLLoaderFactory::DestroyURLLoader,
                     base::Unretained(this)),
      std::move(request), options, url_request, report_raw_headers,
      std::move(client),
      static_cast<net::NetworkTrafficAnnotationTag>(traffic_annotation),
      params_.get(), request_id, resource_scheduler_client_,
      std::move(keepalive_statistics_recorder),
      std::move(network_usage_accumulator)));
}

void URLLoaderFactory::Clone(mojom::URLLoaderFactoryRequest request) {
  binding_set_.AddBinding(this, std::move(request));
}

void URLLoaderFactory::DestroyURLLoader(URLLoader* url_loader) {
  auto it = url_loaders_.find(url_loader);
  DCHECK(it != url_loaders_.end());
  url_loaders_.erase(it);
  DeleteIfNeeded();
}

void URLLoaderFactory::DeleteIfNeeded() {
  if (!binding_set_.empty() || !url_loaders_.empty())
    return;
  context_->DestroyURLLoaderFactory(this);
}

}  // namespace network
