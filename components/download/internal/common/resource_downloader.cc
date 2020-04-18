// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/download/internal/common/resource_downloader.h"

#include <memory>

#include "components/download/public/common/download_url_loader_factory_getter.h"
#include "components/download/public/common/stream_handle_input_stream.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"

namespace network {
struct ResourceResponseHead;
}

namespace download {

// This object monitors the URLLoaderCompletionStatus change when
// ResourceDownloader is asking |delegate_| whether download can proceed.
class URLLoaderStatusMonitor : public network::mojom::URLLoaderClient {
 public:
  using URLLoaderStatusChangeCallback =
      base::OnceCallback<void(const network::URLLoaderCompletionStatus&)>;
  explicit URLLoaderStatusMonitor(URLLoaderStatusChangeCallback callback);
  ~URLLoaderStatusMonitor() override = default;

  // network::mojom::URLLoaderClient
  void OnReceiveResponse(
      const network::ResourceResponseHead& head,
      network::mojom::DownloadedTempFilePtr downloaded_file) override {}
  void OnReceiveRedirect(const net::RedirectInfo& redirect_info,
                         const network::ResourceResponseHead& head) override {}
  void OnDataDownloaded(int64_t data_length, int64_t encoded_length) override {}
  void OnUploadProgress(int64_t current_position,
                        int64_t total_size,
                        OnUploadProgressCallback callback) override {}
  void OnReceiveCachedMetadata(const std::vector<uint8_t>& data) override {}
  void OnTransferSizeUpdated(int32_t transfer_size_diff) override {}
  void OnStartLoadingResponseBody(
      mojo::ScopedDataPipeConsumerHandle body) override {}
  void OnComplete(const network::URLLoaderCompletionStatus& status) override;

 private:
  URLLoaderStatusChangeCallback callback_;
  DISALLOW_COPY_AND_ASSIGN(URLLoaderStatusMonitor);
};

URLLoaderStatusMonitor::URLLoaderStatusMonitor(
    URLLoaderStatusChangeCallback callback)
    : callback_(std::move(callback)) {}

void URLLoaderStatusMonitor::OnComplete(
    const network::URLLoaderCompletionStatus& status) {
  std::move(callback_).Run(status);
}

// static
std::unique_ptr<ResourceDownloader> ResourceDownloader::BeginDownload(
    base::WeakPtr<UrlDownloadHandler::Delegate> delegate,
    std::unique_ptr<DownloadUrlParameters> params,
    std::unique_ptr<network::ResourceRequest> request,
    scoped_refptr<download::DownloadURLLoaderFactoryGetter>
        url_loader_factory_getter,
    const GURL& site_url,
    const GURL& tab_url,
    const GURL& tab_referrer_url,
    uint32_t download_id,
    bool is_parallel_request,
    const scoped_refptr<base::SingleThreadTaskRunner>& task_runner) {
  auto downloader = std::make_unique<ResourceDownloader>(
      delegate, std::move(request), params->render_process_host_id(),
      params->render_frame_host_routing_id(), site_url, tab_url,
      tab_referrer_url, download_id, task_runner,
      std::move(url_loader_factory_getter));

  downloader->Start(std::move(params), is_parallel_request);
  return downloader;
}

// static
std::unique_ptr<ResourceDownloader>
ResourceDownloader::InterceptNavigationResponse(
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
    const scoped_refptr<base::SingleThreadTaskRunner>& task_runner) {
  auto downloader = std::make_unique<ResourceDownloader>(
      delegate, std::move(resource_request), render_process_id, render_frame_id,
      site_url, tab_url, tab_referrer_url, download::DownloadItem::kInvalidId,
      task_runner, std::move(url_loader_factory_getter));
  downloader->InterceptResponse(std::move(response), std::move(url_chain),
                                cert_status,
                                std::move(url_loader_client_endpoints));
  return downloader;
}

ResourceDownloader::ResourceDownloader(
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
        url_loader_factory_getter)
    : delegate_(delegate),
      resource_request_(std::move(resource_request)),
      download_id_(download_id),
      render_process_id_(render_process_id),
      render_frame_id_(render_frame_id),
      site_url_(site_url),
      tab_url_(tab_url),
      tab_referrer_url_(tab_referrer_url),
      delegate_task_runner_(task_runner),
      url_loader_factory_getter_(std::move(url_loader_factory_getter)),
      weak_ptr_factory_(this) {}

ResourceDownloader::~ResourceDownloader() = default;

void ResourceDownloader::Start(
    std::unique_ptr<DownloadUrlParameters> download_url_parameters,
    bool is_parallel_request) {
  callback_ = download_url_parameters->callback();
  guid_ = download_url_parameters->guid();

  // Set up the URLLoaderClient.
  url_loader_client_ = std::make_unique<DownloadResponseHandler>(
      resource_request_.get(), this,
      std::make_unique<DownloadSaveInfo>(
          download_url_parameters->GetSaveInfo()),
      is_parallel_request, download_url_parameters->is_transient(),
      download_url_parameters->fetch_error_body(),
      download_url_parameters->request_headers(),
      download_url_parameters->request_origin(),
      download_url_parameters->download_source(),
      std::vector<GURL>(1, resource_request_->url));
  network::mojom::URLLoaderClientPtr url_loader_client_ptr;
  url_loader_client_binding_ =
      std::make_unique<mojo::Binding<network::mojom::URLLoaderClient>>(
          url_loader_client_.get(), mojo::MakeRequest(&url_loader_client_ptr));

  // Set up the URLLoader
  network::mojom::URLLoaderRequest url_loader_request =
      mojo::MakeRequest(&url_loader_);
  url_loader_factory_getter_->GetURLLoaderFactory()->CreateLoaderAndStart(
      std::move(url_loader_request),
      0,  // routing_id
      0,  // request_id
      network::mojom::kURLLoadOptionSendSSLInfoWithResponse,
      *(resource_request_.get()), std::move(url_loader_client_ptr),
      net::MutableNetworkTrafficAnnotationTag(
          download_url_parameters->GetNetworkTrafficAnnotation()));
  url_loader_->SetPriority(net::RequestPriority::IDLE,
                           0 /* intra_priority_value */);
}

void ResourceDownloader::InterceptResponse(
    const scoped_refptr<network::ResourceResponse>& response,
    std::vector<GURL> url_chain,
    net::CertStatus cert_status,
    network::mojom::URLLoaderClientEndpointsPtr endpoints) {
  // Set the URLLoader.
  url_loader_.Bind(std::move(endpoints->url_loader));

  // Create the new URLLoaderClient that will intercept the navigation.
  url_loader_client_ = std::make_unique<DownloadResponseHandler>(
      resource_request_.get(), this, std::make_unique<DownloadSaveInfo>(),
      false, /* is_parallel_request */
      false, /* is_transient */
      false, /* fetch_error_body */
      download::DownloadUrlParameters::RequestHeadersType(),
      std::string(), /* request_origin */
      download::DownloadSource::NAVIGATION, std::move(url_chain));

  // Simulate on the new URLLoaderClient calls that happened on the old client.
  response->head.cert_status = cert_status;
  url_loader_client_->OnReceiveResponse(
      response->head, network::mojom::DownloadedTempFilePtr());

  // Bind the new client.
  url_loader_client_binding_ =
      std::make_unique<mojo::Binding<network::mojom::URLLoaderClient>>(
          url_loader_client_.get(), std::move(endpoints->url_loader_client));
}

void ResourceDownloader::OnResponseStarted(
    std::unique_ptr<DownloadCreateInfo> download_create_info,
    mojom::DownloadStreamHandlePtr stream_handle) {
  download_create_info->download_id = download_id_;
  download_create_info->guid = guid_;
  download_create_info->site_url = site_url_;
  download_create_info->tab_url = tab_url_;
  download_create_info->tab_referrer_url = tab_referrer_url_;
  download_create_info->render_process_id = render_process_id_;
  download_create_info->render_frame_id = render_frame_id_;

  delegate_task_runner_->PostTask(
      FROM_HERE,
      base::BindOnce(
          &UrlDownloadHandler::Delegate::OnUrlDownloadStarted, delegate_,
          std::move(download_create_info),
          std::make_unique<StreamHandleInputStream>(std::move(stream_handle)),
          std::move(url_loader_factory_getter_), callback_));
}

void ResourceDownloader::OnReceiveRedirect() {
  url_loader_->FollowRedirect(base::nullopt);
}

}  // namespace download
