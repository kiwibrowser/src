// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/service_worker/service_worker_installed_script_loader.h"

#include <memory>
#include <string>
#include <utility>
#include "content/browser/service_worker/service_worker_cache_writer.h"
#include "content/browser/service_worker/service_worker_context_core.h"
#include "content/browser/service_worker/service_worker_storage.h"
#include "content/browser/service_worker/service_worker_version.h"
#include "content/browser/service_worker/service_worker_write_to_cache_job.h"
#include "content/browser/url_loader_factory_getter.h"
#include "content/common/service_worker/service_worker_utils.h"
#include "net/cert/cert_status_flags.h"
#include "services/network/public/cpp/resource_response.h"
#include "third_party/blink/public/common/mime_util/mime_util.h"

namespace content {

using FinishedReason = ServiceWorkerInstalledScriptReader::FinishedReason;

ServiceWorkerInstalledScriptLoader::ServiceWorkerInstalledScriptLoader(
    uint32_t options,
    network::mojom::URLLoaderClientPtr client,
    std::unique_ptr<ServiceWorkerResponseReader> response_reader)
    : options_(options),
      client_(std::move(client)),
      request_start_(base::TimeTicks::Now()) {
  reader_ = std::make_unique<ServiceWorkerInstalledScriptReader>(
      std::move(response_reader), this);
  reader_->Start();
  // We continue in OnStarted().
}

ServiceWorkerInstalledScriptLoader::~ServiceWorkerInstalledScriptLoader() =
    default;

void ServiceWorkerInstalledScriptLoader::OnStarted(
    std::string encoding,
    base::flat_map<std::string, std::string> headers,
    mojo::ScopedDataPipeConsumerHandle body_handle,
    uint64_t body_size,
    mojo::ScopedDataPipeConsumerHandle metadata_handle,
    uint64_t metadata_size) {
  encoding_ = encoding;
  body_handle_ = std::move(body_handle);
  body_size_ = body_size;

  // Just drain the metadata (V8 code cache): this entire class is just to
  // handle a corner case for non-installed service workers and high performance
  // is not needed.
  metadata_drainer_ =
      std::make_unique<mojo::DataPipeDrainer>(this, std::move(metadata_handle));

  // We continue in OnHttpInfoRead().
}

void ServiceWorkerInstalledScriptLoader::OnHttpInfoRead(
    scoped_refptr<HttpResponseInfoIOBuffer> http_info) {
  net::HttpResponseInfo* info = http_info->http_info.get();

  network::ResourceResponseHead head;
  head.request_start = request_start_;
  head.response_start = base::TimeTicks::Now();
  head.request_time = info->request_time;
  head.response_time = info->response_time;
  head.headers = info->headers;
  head.headers->GetMimeType(&head.mime_type);
  head.charset = encoding_;
  head.content_length = body_size_;
  head.was_fetched_via_spdy = info->was_fetched_via_spdy;
  head.was_alpn_negotiated = info->was_alpn_negotiated;
  head.connection_info = info->connection_info;
  head.alpn_negotiated_protocol = info->alpn_negotiated_protocol;
  head.socket_address = info->socket_address;
  head.cert_status = info->ssl_info.cert_status;

  if (options_ & network::mojom::kURLLoadOptionSendSSLInfoWithResponse)
    head.ssl_info = info->ssl_info;

  client_->OnReceiveResponse(head, nullptr /* downloaded_file */);
  client_->OnStartLoadingResponseBody(std::move(body_handle_));
  // We continue in OnFinished().
}

void ServiceWorkerInstalledScriptLoader::OnFinished(FinishedReason reason) {
  int net_error = net::ERR_FAILED;
  switch (reason) {
    case FinishedReason::kSuccess:
      net_error = net::OK;
      break;
    case FinishedReason::kCreateDataPipeError:
      net_error = net::ERR_INSUFFICIENT_RESOURCES;
      break;
    case FinishedReason::kNoHttpInfoError:
    case FinishedReason::kResponseReaderError:
      net_error = net::ERR_FILE_NOT_FOUND;
      break;
    case FinishedReason::kConnectionError:
    case FinishedReason::kMetaDataSenderError:
      net_error = net::ERR_FAILED;
      break;
    case FinishedReason::kNotFinished:
      NOTREACHED();
      break;
  }
  client_->OnComplete(network::URLLoaderCompletionStatus(net_error));
}

void ServiceWorkerInstalledScriptLoader::FollowRedirect(
    const base::Optional<net::HttpRequestHeaders>& modified_request_headers) {
  // This class never returns a redirect response to its client, so should never
  // be asked to follow one.
  NOTREACHED();
}

void ServiceWorkerInstalledScriptLoader::ProceedWithResponse() {
  // This function should only be called for navigations.
  NOTREACHED();
}

void ServiceWorkerInstalledScriptLoader::SetPriority(
    net::RequestPriority priority,
    int32_t intra_priority_value) {
  // Ignore: this class doesn't have a concept of priority.
}

void ServiceWorkerInstalledScriptLoader::PauseReadingBodyFromNet() {
  // Ignore: this class doesn't read from network.
}

void ServiceWorkerInstalledScriptLoader::ResumeReadingBodyFromNet() {
  // Ignore: this class doesn't read from network.
}

}  // namespace content
