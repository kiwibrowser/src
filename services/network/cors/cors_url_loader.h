// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_NETWORK_CORS_CORS_URL_LOADER_H_
#define SERVICES_NETWORK_CORS_CORS_URL_LOADER_H_

#include "base/memory/weak_ptr.h"
#include "base/optional.h"
#include "mojo/public/cpp/bindings/binding.h"
#include "net/traffic_annotation/network_traffic_annotation.h"
#include "services/network/public/cpp/cors/cors_error_status.h"
#include "services/network/public/mojom/fetch_api.mojom.h"
#include "services/network/public/mojom/url_loader_factory.mojom.h"
#include "url/gurl.h"
#include "url/origin.h"

namespace network {

namespace cors {

// Wrapper class that adds cross-origin resource sharing capabilities
// (https://fetch.spec.whatwg.org/#http-cors-protocol), delegating requests as
// well as potential preflight requests to the supplied
// |network_loader_factory|. It is owned by the CORSURLLoaderFactory that
// created it.
class COMPONENT_EXPORT(NETWORK_SERVICE) CORSURLLoader
    : public mojom::URLLoader,
      public mojom::URLLoaderClient {
 public:
  // Assumes network_loader_factory outlives this loader.
  // TODO(yhirano): Remove |preflight_finalizer| when the network service is
  // fully enabled.
  CORSURLLoader(
      int32_t routing_id,
      int32_t request_id,
      uint32_t options,
      const ResourceRequest& resource_request,
      mojom::URLLoaderClientPtr client,
      const net::MutableNetworkTrafficAnnotationTag& traffic_annotation,
      mojom::URLLoaderFactory* network_loader_factory,
      const base::RepeatingCallback<void(int)>& preflight_finalizer);

  ~CORSURLLoader() override;

  // mojom::URLLoader overrides:
  void FollowRedirect(const base::Optional<net::HttpRequestHeaders>&
                          modified_request_headers) override;
  void ProceedWithResponse() override;
  void SetPriority(net::RequestPriority priority,
                   int intra_priority_value) override;
  void PauseReadingBodyFromNet() override;
  void ResumeReadingBodyFromNet() override;

  // mojom::URLLoaderClient overrides:
  void OnReceiveResponse(const ResourceResponseHead& head,
                         mojom::DownloadedTempFilePtr downloaded_file) override;
  void OnReceiveRedirect(const net::RedirectInfo& redirect_info,
                         const ResourceResponseHead& head) override;
  void OnDataDownloaded(int64_t data_length, int64_t encoded_length) override;
  void OnUploadProgress(int64_t current_position,
                        int64_t total_size,
                        base::OnceCallback<void()> callback) override;
  void OnReceiveCachedMetadata(const std::vector<uint8_t>& data) override;
  void OnTransferSizeUpdated(int32_t transfer_size_diff) override;
  void OnStartLoadingResponseBody(
      mojo::ScopedDataPipeConsumerHandle body) override;
  void OnComplete(const URLLoaderCompletionStatus& status) override;

 private:
  void StartNetworkRequest(
      int32_t routing_id,
      int32_t request_id,
      uint32_t options,
      const net::MutableNetworkTrafficAnnotationTag& traffic_annotation,
      base::Optional<CORSErrorStatus> status);

  // Called when there is a connection error on the upstream pipe used for the
  // actual request.
  void OnUpstreamConnectionError();

  // Handles OnComplete() callback.
  void HandleComplete(const URLLoaderCompletionStatus& status);

  // This raw URLLoaderFactory pointer is shared with the CORSURLLoaderFactory
  // that created and owns this object.
  mojom::URLLoaderFactory* network_loader_factory_;

  // For the actual request.
  mojom::URLLoaderPtr network_loader_;
  mojo::Binding<mojom::URLLoaderClient> network_client_binding_;
  ResourceRequest request_;

  // To be a URLLoader for the client.
  mojom::URLLoaderClientPtr forwarding_client_;

  // The last response URL, that is usually the requested URL, but can be
  // different if redirects happen.
  GURL last_response_url_;

  // A flag to indicate that the instance is waiting for that forwarding_client_
  // calls FollowRedirect.
  bool is_waiting_follow_redirect_call_ = false;

  // Corresponds to the Fetch spec, https://fetch.spec.whatwg.org/.
  bool fetch_cors_flag_;

  // Used to run asynchronous class instance bound callbacks safely.
  base::WeakPtrFactory<CORSURLLoader> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(CORSURLLoader);
};

}  // namespace cors

}  // namespace network

#endif  // SERVICES_NETWORK_CORS_CORS_URL_LOADER_H_
