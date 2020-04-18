// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_DOWNLOAD_INTERNAL_COMMON_RESOURCE_DOWNLOADER_H_
#define COMPONENTS_DOWNLOAD_INTERNAL_COMMON_RESOURCE_DOWNLOADER_H_

#include "components/download/public/common/download_export.h"
#include "components/download/public/common/download_response_handler.h"
#include "components/download/public/common/url_download_handler.h"
#include "mojo/public/cpp/bindings/binding.h"
#include "net/cert/cert_status_flags.h"
#include "services/network/public/cpp/resource_request.h"
#include "services/network/public/mojom/url_loader.mojom.h"

namespace download {
class DownloadURLLoaderFactoryGetter;

// Class for handing the download of a url.
class COMPONENTS_DOWNLOAD_EXPORT ResourceDownloader
    : public download::UrlDownloadHandler,
      public download::DownloadResponseHandler::Delegate {
 public:
  // Called to start a download, must be called on IO thread.
  static std::unique_ptr<ResourceDownloader> BeginDownload(
      base::WeakPtr<download::UrlDownloadHandler::Delegate> delegate,
      std::unique_ptr<download::DownloadUrlParameters> download_url_parameters,
      std::unique_ptr<network::ResourceRequest> request,
      scoped_refptr<download::DownloadURLLoaderFactoryGetter>
          url_loader_factory_getter,
      const GURL& site_url,
      const GURL& tab_url,
      const GURL& tab_referrer_url,
      uint32_t download_id,
      bool is_parallel_request,
      const scoped_refptr<base::SingleThreadTaskRunner>& task_runner);

  // Create a ResourceDownloader from a navigation that turns to be a download.
  // No URLLoader is created, but the URLLoaderClient implementation is
  // transferred.
  static std::unique_ptr<ResourceDownloader> InterceptNavigationResponse(
      base::WeakPtr<UrlDownloadHandler::Delegate> delegate,
      std::unique_ptr<network::ResourceRequest> resource_request,
      int render_process_id,
      int render_frame_id,
      const GURL& site_url,
      const GURL& tab_url,
      const GURL& tab_referrer_url,
      std::vector<GURL> url_chain,
      const scoped_refptr<network::ResourceResponse>& response,
      net::CertStatus cert_status,
      network::mojom::URLLoaderClientEndpointsPtr url_loader_client_endpoints,
      scoped_refptr<download::DownloadURLLoaderFactoryGetter>
          url_loader_factory_getter,
      const scoped_refptr<base::SingleThreadTaskRunner>& task_runner);

  ResourceDownloader(
      base::WeakPtr<UrlDownloadHandler::Delegate> delegate,
      std::unique_ptr<network::ResourceRequest> resource_request,
      int render_process_id,
      int render_frame_id,
      const GURL& site_url,
      const GURL& tab_url,
      const GURL& tab_referrer_url,
      uint32_t download_id,
      const scoped_refptr<base::SingleThreadTaskRunner>& task_runner,
      scoped_refptr<download::DownloadURLLoaderFactoryGetter>
          url_loader_factory_getter);
  ~ResourceDownloader() override;

  // download::DownloadResponseHandler::Delegate
  void OnResponseStarted(
      std::unique_ptr<download::DownloadCreateInfo> download_create_info,
      download::mojom::DownloadStreamHandlePtr stream_handle) override;
  void OnReceiveRedirect() override;

 private:
  // Helper method to start the network request.
  void Start(
      std::unique_ptr<download::DownloadUrlParameters> download_url_parameters,
      bool is_parallel_request);

  // Intercepts the navigation response.
  void InterceptResponse(
      const scoped_refptr<network::ResourceResponse>& response,
      std::vector<GURL> url_chain,
      net::CertStatus cert_status,
      network::mojom::URLLoaderClientEndpointsPtr url_loader_client_endpoints);

  base::WeakPtr<download::UrlDownloadHandler::Delegate> delegate_;

  // The ResourceRequest for this object.
  std::unique_ptr<network::ResourceRequest> resource_request_;

  // Object that will handle the response.
  std::unique_ptr<network::mojom::URLLoaderClient> url_loader_client_;

  // URLLoaderClient binding. It sends any requests to the |url_loader_client_|.
  std::unique_ptr<mojo::Binding<network::mojom::URLLoaderClient>>
      url_loader_client_binding_;

  // URLLoader for sending out the request.
  network::mojom::URLLoaderPtr url_loader_;

  // ID of the download, or download::DownloadItem::kInvalidId if this is a new
  // download.
  uint32_t download_id_;

  // GUID of the download, or empty if this is a new download.
  std::string guid_;

  // Callback to run after download starts.
  download::DownloadUrlParameters::OnStartedCallback callback_;

  // Frame and process id associated with the request.
  int render_process_id_;
  int render_frame_id_;

  // Site URL for the site instance that initiated the download.
  GURL site_url_;

  // The URL of the tab that started us.
  GURL tab_url_;

  // The referrer URL of the tab that started us.
  GURL tab_referrer_url_;

  // URLLoader status when intercepting the navigation request.
  base::Optional<network::URLLoaderCompletionStatus> url_loader_status_;

  // TaskRunner to post callbacks to the |delegate_|
  scoped_refptr<base::SingleThreadTaskRunner> delegate_task_runner_;

  // URLLoaderFactory getter for issueing network requests.
  scoped_refptr<download::DownloadURLLoaderFactoryGetter>
      url_loader_factory_getter_;

  base::WeakPtrFactory<ResourceDownloader> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(ResourceDownloader);
};

}  // namespace download

#endif  // COMPONENTS_DOWNLOAD_INTERNAL_COMMON_RESOURCE_DOWNLOADER_H_
