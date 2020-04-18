// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EXTENSIONS_BROWSER_API_WEB_REQUEST_WEB_REQUEST_PROXYING_URL_LOADER_FACTORY_H_
#define EXTENSIONS_BROWSER_API_WEB_REQUEST_WEB_REQUEST_PROXYING_URL_LOADER_FACTORY_H_

#include <cstdint>
#include <map>

#include "base/callback.h"
#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/memory/ref_counted_delete_on_sequence.h"
#include "base/memory/weak_ptr.h"
#include "base/optional.h"
#include "extensions/browser/api/web_request/web_request_info.h"
#include "mojo/public/cpp/bindings/binding.h"
#include "mojo/public/cpp/bindings/binding_set.h"
#include "net/base/completion_callback.h"
#include "net/traffic_annotation/network_traffic_annotation.h"
#include "services/network/public/cpp/resource_request.h"
#include "services/network/public/cpp/resource_response.h"
#include "services/network/public/mojom/url_loader.mojom.h"
#include "services/network/public/mojom/url_loader_factory.mojom.h"
#include "url/gurl.h"

namespace content {
class ResourceContext;
}  // namespace content

namespace extensions {

class ExtensionNavigationUIData;
class InfoMap;

// Owns URLLoaderFactory bindings for WebRequest proxies with the Network
// Service enabled. This is loosely controlled by the WebRequestAPI on the UI
// thread, but does all its real work on the IO thread. This is only because
// it is tightly coupled to ExtensionsWebRequestEventRouter, and that object
// must stay on the IO thread until we can deprecate the non-Network Service
// path. Once Network Service is the only path, we can move all this stuff to
// the UI thread.
class WebRequestProxyingURLLoaderFactory
    : public base::RefCountedDeleteOnSequence<
          WebRequestProxyingURLLoaderFactory>,
      public network::mojom::URLLoaderFactory {
 public:
  class InProgressRequest : public network::mojom::URLLoader,
                            public network::mojom::URLLoaderClient {
   public:
    InProgressRequest(
        WebRequestProxyingURLLoaderFactory* factory,
        uint64_t request_id,
        int32_t routing_id,
        int32_t network_service_request_id,
        uint32_t options,
        const network::ResourceRequest& request,
        const net::MutableNetworkTrafficAnnotationTag& traffic_annotation,
        network::mojom::URLLoaderRequest loader_request,
        network::mojom::URLLoaderClientPtr client);
    ~InProgressRequest() override;

    void Restart();

    // network::mojom::URLLoader:
    void FollowRedirect(const base::Optional<net::HttpRequestHeaders>&
                            modified_request_headers) override;
    void ProceedWithResponse() override;
    void SetPriority(net::RequestPriority priority,
                     int32_t intra_priority_value) override;
    void PauseReadingBodyFromNet() override;
    void ResumeReadingBodyFromNet() override;

    // network::mojom::URLLoaderClient:
    void OnReceiveResponse(
        const network::ResourceResponseHead& head,
        network::mojom::DownloadedTempFilePtr downloaded_file) override;
    void OnReceiveRedirect(const net::RedirectInfo& redirect_info,
                           const network::ResourceResponseHead& head) override;
    void OnDataDownloaded(int64_t data_length, int64_t encoded_length) override;
    void OnUploadProgress(int64_t current_position,
                          int64_t total_size,
                          OnUploadProgressCallback callback) override;
    void OnReceiveCachedMetadata(const std::vector<uint8_t>& data) override;
    void OnTransferSizeUpdated(int32_t transfer_size_diff) override;
    void OnStartLoadingResponseBody(
        mojo::ScopedDataPipeConsumerHandle body) override;
    void OnComplete(const network::URLLoaderCompletionStatus& status) override;

   private:
    void ContinueToBeforeSendHeaders(int error_code);
    void ContinueToSendHeaders(int error_code);
    void ContinueToResponseStarted(
        network::mojom::DownloadedTempFilePtr downloaded_file,
        int error_code);
    void ContinueToBeforeRedirect(const net::RedirectInfo& redirect_info,
                                  int error_code);
    void HandleResponseOrRedirectHeaders(
        const net::CompletionCallback& continuation);
    void OnRequestError(const network::URLLoaderCompletionStatus& status);

    WebRequestProxyingURLLoaderFactory* const factory_;
    network::ResourceRequest request_;
    const uint64_t request_id_;
    const int32_t network_service_request_id_;
    const int32_t routing_id_;
    const uint32_t options_;
    const net::MutableNetworkTrafficAnnotationTag traffic_annotation_;
    mojo::Binding<network::mojom::URLLoader> proxied_loader_binding_;
    network::mojom::URLLoaderClientPtr target_client_;

    base::Optional<WebRequestInfo> info_;

    mojo::Binding<network::mojom::URLLoaderClient> proxied_client_binding_;
    network::mojom::URLLoaderPtr target_loader_;

    // NOTE: This is state which ExtensionWebRequestEventRouter needs to have
    // persisted across some phases of this request -- namely between
    // |OnHeadersReceived()| and request completion or restart. Pointers to
    // these fields are stored in a |BlockedRequest| (created and owned by
    // ExtensionWebRequestEventRouter) through much of the request's lifetime.
    // That code supports both Network Service and non-Network Service behavior,
    // which is why this weirdness exists here.
    network::ResourceResponseHead current_response_;
    scoped_refptr<net::HttpResponseHeaders> override_headers_;
    GURL redirect_url_;
    GURL allowed_unsafe_redirect_url_;

    // Iff |true| we will ignore the next incoming |FollowRedirect()| call from
    // our client.
    bool ignore_next_follow_redirect_ = false;

    base::WeakPtrFactory<InProgressRequest> weak_factory_;

    DISALLOW_COPY_AND_ASSIGN(InProgressRequest);
  };

  WebRequestProxyingURLLoaderFactory(void* browser_context,
                                     content::ResourceContext* resource_context,
                                     InfoMap* info_map);

  void StartProxying(
      int render_process_id,
      int render_frame_id,
      std::unique_ptr<ExtensionNavigationUIData> navigation_ui_data,
      network::mojom::URLLoaderFactoryRequest loader_request,
      network::mojom::URLLoaderFactoryPtrInfo target_factory_info,
      base::OnceClosure on_disconnect);

  // network::mojom::URLLoaderFactory:
  void CreateLoaderAndStart(network::mojom::URLLoaderRequest loader_request,
                            int32_t routing_id,
                            int32_t request_id,
                            uint32_t options,
                            const network::ResourceRequest& request,
                            network::mojom::URLLoaderClientPtr client,
                            const net::MutableNetworkTrafficAnnotationTag&
                                traffic_annotation) override;
  void Clone(network::mojom::URLLoaderFactoryRequest loader_request) override;

 private:
  friend class base::DeleteHelper<WebRequestProxyingURLLoaderFactory>;
  friend class base::RefCountedDeleteOnSequence<
      WebRequestProxyingURLLoaderFactory>;

  ~WebRequestProxyingURLLoaderFactory() override;

  void OnTargetFactoryError();
  void OnProxyBindingError();
  void RemoveRequest(uint64_t request_id);

  void* const browser_context_;
  content::ResourceContext* const resource_context_;
  int render_process_id_ = -1;
  int render_frame_id_ = -1;
  std::unique_ptr<ExtensionNavigationUIData> navigation_ui_data_;
  InfoMap* const info_map_;
  mojo::BindingSet<network::mojom::URLLoaderFactory> proxy_bindings_;
  network::mojom::URLLoaderFactoryPtr target_factory_;
  base::OnceClosure on_disconnect_;
  uint64_t next_request_id_ = 1;

  std::map<int32_t, std::unique_ptr<InProgressRequest>> requests_;

  DISALLOW_COPY_AND_ASSIGN(WebRequestProxyingURLLoaderFactory);
};

}  // namespace extensions

#endif  // EXTENSIONS_BROWSER_API_WEB_REQUEST_WEB_REQUEST_PROXYING_URL_LOADER_FACTORY_H_
