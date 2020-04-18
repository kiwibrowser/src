// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_SERVICE_WORKER_SERVICE_WORKER_NEW_SCRIPT_LOADER_H_
#define CONTENT_BROWSER_SERVICE_WORKER_SERVICE_WORKER_NEW_SCRIPT_LOADER_H_

#include "base/macros.h"
#include "content/browser/service_worker/service_worker_version.h"
#include "content/common/content_export.h"
#include "mojo/public/cpp/bindings/binding.h"
#include "net/traffic_annotation/network_traffic_annotation.h"
#include "services/network/public/cpp/net_adapters.h"
#include "services/network/public/cpp/resource_request.h"
#include "services/network/public/mojom/url_loader.mojom.h"
#include "url/gurl.h"

namespace content {

class ServiceWorkerCacheWriter;
class ServiceWorkerVersion;
struct HttpResponseInfoIOBuffer;

// S13nServiceWorker:
// This is the URLLoader used for loading scripts for a new (installing) service
// worker. It fetches the script (the main script or imported script) from
// network, and returns the response to |client|, while also writing the
// response into the service worker script storage.
//
// This works as follows:
//   1. Makes a network request.
//   2. OnReceiveResponse() is called, writes the response headers to the
//      service worker script storage and responds with them to the |client|
//      (which is the service worker in the renderer).
//   3. OnStartLoadingResponseBody() is called, reads the network response from
//      the data pipe. While reading the response, writes it to the service
//      worker script storage and responds with it to the |client|.
//   4. OnComplete() for the network load and OnWriteDataComplete() are called,
//      calls CommitCompleted() and closes the connections with the network
//      service and the renderer process.
//
// In case there is already an installed service worker for this registration,
// this class also performs the "byte-for-byte" comparison for updating the
// worker. If the script is identical, the load succeeds but no script is
// written, and ServiceWorkerVersion is told to terminate startup.
//
// NOTE: To perform the network request, this class uses |loader_factory_| which
// may internally use a non-NetworkService factory if URL has a non-http(s)
// scheme, e.g., a chrome-extension:// URL. Regardless, that is still called a
// "network" request in comments and naming. "network" is meant to distinguish
// from the load this URLLoader does for its client:
//     "network" <------> SWNewScriptLoader <------> client
class CONTENT_EXPORT ServiceWorkerNewScriptLoader
    : public network::mojom::URLLoader,
      public network::mojom::URLLoaderClient {
 public:
  // |loader_factory| is used to load the script, see class comments.
  ServiceWorkerNewScriptLoader(
      int32_t routing_id,
      int32_t request_id,
      uint32_t options,
      const network::ResourceRequest& original_request,
      network::mojom::URLLoaderClientPtr client,
      scoped_refptr<ServiceWorkerVersion> version,
      scoped_refptr<network::SharedURLLoaderFactory> loader_factory,
      const net::MutableNetworkTrafficAnnotationTag& traffic_annotation);
  ~ServiceWorkerNewScriptLoader() override;

  // network::mojom::URLLoader:
  void FollowRedirect(const base::Optional<net::HttpRequestHeaders>&
                          modified_request_headers) override;
  void ProceedWithResponse() override;
  void SetPriority(net::RequestPriority priority,
                   int32_t intra_priority_value) override;
  void PauseReadingBodyFromNet() override;
  void ResumeReadingBodyFromNet() override;

  // network::mojom::URLLoaderClient for the network load:
  void OnReceiveResponse(
      const network::ResourceResponseHead& response_head,
      network::mojom::DownloadedTempFilePtr downloaded_file) override;
  void OnReceiveRedirect(
      const net::RedirectInfo& redirect_info,
      const network::ResourceResponseHead& response_head) override;
  void OnDataDownloaded(int64_t data_len, int64_t encoded_data_len) override;
  void OnUploadProgress(int64_t current_position,
                        int64_t total_size,
                        OnUploadProgressCallback ack_callback) override;
  void OnReceiveCachedMetadata(const std::vector<uint8_t>& data) override;
  void OnTransferSizeUpdated(int32_t transfer_size_diff) override;
  void OnStartLoadingResponseBody(
      mojo::ScopedDataPipeConsumerHandle body) override;
  void OnComplete(const network::URLLoaderCompletionStatus& status) override;

  // Buffer size for reading script data from network.
  const static uint32_t kReadBufferSize;

 private:
  enum class State {
    kNotStarted,
    kStarted,
    kWroteHeaders,
    kWroteData,
    kCompleted,
  };
  void AdvanceState(State new_state);

  // Writes the given headers into the service worker script storage.
  void WriteHeaders(scoped_refptr<HttpResponseInfoIOBuffer> info_buffer);
  void OnWriteHeadersComplete(net::Error error);

  // Starts watching the data pipe for the network load (i.e.,
  // |network_consumer_|) if it's ready.
  void MaybeStartNetworkConsumerHandleWatcher();

  // Called when |network_consumer_| is ready to be read. Can be called multiple
  // times.
  void OnNetworkDataAvailable(MojoResult);

  // Writes the given data into the service worker script storage.
  void WriteData(scoped_refptr<network::MojoToNetPendingBuffer> pending_buffer,
                 uint32_t bytes_available);
  void OnWriteDataComplete(
      scoped_refptr<network::MojoToNetPendingBuffer> pending_buffer,
      uint32_t bytes_written,
      net::Error error);

  // This is the last method that is called on this class. Notifies the final
  // result to |client_| and clears all mojo connections etc.
  void CommitCompleted(const network::URLLoaderCompletionStatus& status,
                       const std::string& status_message);

  const GURL request_url_;

  // This is RESOURCE_TYPE_SERVICE_WORKER for the main script or
  // RESOURCE_TYPE_SCRIPT for an imported script.
  const ResourceType resource_type_;

  scoped_refptr<ServiceWorkerVersion> version_;

  std::unique_ptr<ServiceWorkerCacheWriter> cache_writer_;

  // Used for fetching the script from network, which might not actually
  // use the direct network factory, see class comments.
  network::mojom::URLLoaderPtr network_loader_;
  mojo::Binding<network::mojom::URLLoaderClient> network_client_binding_;
  mojo::ScopedDataPipeConsumerHandle network_consumer_;
  mojo::SimpleWatcher network_watcher_;
  bool network_load_completed_ = false;
  scoped_refptr<network::SharedURLLoaderFactory> loader_factory_;

  // Used for responding with the fetched script to this loader's client.
  network::mojom::URLLoaderClientPtr client_;
  mojo::ScopedDataPipeProducerHandle client_producer_;

  State state_ = State::kNotStarted;

  base::WeakPtrFactory<ServiceWorkerNewScriptLoader> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(ServiceWorkerNewScriptLoader);
};

}  // namespace content

#endif  // CONTENT_BROWSER_SERVICE_WORKER_SERVICE_WORKER_NEW_SCRIPT_LOADER_H_
