// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_APPCACHE_APPCACHE_UPDATE_URL_LOADER_REQUEST_H_
#define CONTENT_BROWSER_APPCACHE_APPCACHE_UPDATE_URL_LOADER_REQUEST_H_

#include <stddef.h>
#include <stdint.h>
#include <memory>

#include "base/macros.h"
#include "content/browser/appcache/appcache_update_request_base.h"
#include "mojo/public/cpp/bindings/binding.h"
#include "mojo/public/cpp/system/simple_watcher.h"
#include "net/base/io_buffer.h"
#include "services/network/public/cpp/net_adapters.h"
#include "services/network/public/cpp/resource_request.h"
#include "services/network/public/cpp/resource_response.h"
#include "services/network/public/mojom/url_loader.mojom.h"

namespace net {
class HttpResponseInfo;
}

namespace content {

class URLLoaderFactoryGetter;

// URLLoaderClient subclass for the UpdateRequestBase class. Provides
// functionality to update the AppCache using functionality provided by the
// network URL loader.
class AppCacheUpdateJob::UpdateURLLoaderRequest
    : public AppCacheUpdateJob::UpdateRequestBase,
      public network::mojom::URLLoaderClient {
 public:
  ~UpdateURLLoaderRequest() override;

  // UpdateRequestBase overrides.
  void Start() override;
  void SetExtraRequestHeaders(const net::HttpRequestHeaders& headers) override;
  GURL GetURL() const override;
  void SetLoadFlags(int flags) override;
  int GetLoadFlags() const override;
  std::string GetMimeType() const override;
  void SetSiteForCookies(const GURL& site_for_cookies) override;
  void SetInitiator(const base::Optional<url::Origin>& initiator) override;
  net::HttpResponseHeaders* GetResponseHeaders() const override;
  int GetResponseCode() const override;
  const net::HttpResponseInfo& GetResponseInfo() const override;
  void Read() override;
  int Cancel() override;

  // network::mojom::URLLoaderClient implementation.
  // These methods are called by the network loader.
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

 private:
  UpdateURLLoaderRequest(URLLoaderFactoryGetter* loader_factory_getter,
                         const GURL& url,
                         int buffer_size,
                         URLFetcher* fetcher);

  // Helper function to initiate an asynchronous read on the data pipe.
  void StartReading(MojoResult unused);

  // Helper function to setup the data pipe watcher to start reading from
  // the pipe. We need to do this when the data pipe is available and there is
  // a pending read.
  void MaybeStartReading();

  friend class AppCacheUpdateJob::UpdateRequestBase;

  URLFetcher* fetcher_;
  // Used to retrieve the network URLLoader interface to issue network
  // requests
  scoped_refptr<URLLoaderFactoryGetter> loader_factory_getter_;

  network::ResourceRequest request_;
  network::ResourceResponseHead response_;
  network::URLLoaderCompletionStatus response_status_;
  // Response details.
  std::unique_ptr<net::HttpResponseInfo> http_response_info_;
  // Binds the URLLoaderClient interface to the channel.
  mojo::Binding<network::mojom::URLLoaderClient> client_binding_;
  // The network URL loader.
  network::mojom::URLLoaderPtr url_loader_;
  // Caller buffer size.
  int buffer_size_;
  // The mojo data pipe.
  mojo::ScopedDataPipeConsumerHandle handle_;
  // Used to watch the data pipe to initiate reads.
  mojo::SimpleWatcher handle_watcher_;
  // Set to true when the caller issues a read request. We set it to false in
  // the StartReading() function when the mojo BeginReadData API returns a
  // value indicating one of the following:
  // 1. Data is available.
  // 2. End of data has been reached.
  // 3. Error.
  // Please look at the StartReading() function for details.
  bool read_requested_;
  // Adapter for transferring data from a mojo data pipe to net.
  scoped_refptr<network::MojoToNetPendingBuffer> pending_read_;

  DISALLOW_COPY_AND_ASSIGN(UpdateURLLoaderRequest);
};

}  // namespace content

#endif  // CONTENT_BROWSER_APPCACHE_APPCACHE_UPDATE_URL_LOADER_REQUEST_H_
