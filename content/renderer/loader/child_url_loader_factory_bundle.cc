// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/renderer/loader/child_url_loader_factory_bundle.h"

#include "base/logging.h"
#include "mojo/public/cpp/bindings/strong_binding.h"
#include "url/gurl.h"
#include "url/url_constants.h"

namespace content {

namespace {

class URLLoaderRelay : public network::mojom::URLLoaderClient,
                       public network::mojom::URLLoader {
 public:
  URLLoaderRelay(network::mojom::URLLoaderPtr loader_sink,
                 network::mojom::URLLoaderClientRequest client_source,
                 network::mojom::URLLoaderClientPtr client_sink)
      : loader_sink_(std::move(loader_sink)),
        client_source_binding_(this, std::move(client_source)),
        client_sink_(std::move(client_sink)) {}

  // network::mojom::URLLoader implementation:
  void FollowRedirect(const base::Optional<net::HttpRequestHeaders>&
                          modified_request_headers) override {
    DCHECK(!modified_request_headers.has_value())
        << "Redirect with modified headers was not supported yet. "
           "crbug.com/845683";
    loader_sink_->FollowRedirect(base::nullopt);
  }

  void ProceedWithResponse() override { loader_sink_->ProceedWithResponse(); }

  void SetPriority(net::RequestPriority priority,
                   int32_t intra_priority_value) override {
    loader_sink_->SetPriority(priority, intra_priority_value);
  }

  void PauseReadingBodyFromNet() override {
    loader_sink_->PauseReadingBodyFromNet();
  }

  void ResumeReadingBodyFromNet() override {
    loader_sink_->ResumeReadingBodyFromNet();
  }

  // network::mojom::URLLoaderClient implementation:
  void OnReceiveResponse(
      const network::ResourceResponseHead& head,
      network::mojom::DownloadedTempFilePtr downloaded_file) override {
    client_sink_->OnReceiveResponse(head, std::move(downloaded_file));
  }

  void OnReceiveRedirect(const net::RedirectInfo& redirect_info,
                         const network::ResourceResponseHead& head) override {
    client_sink_->OnReceiveRedirect(redirect_info, head);
  }

  void OnDataDownloaded(int64_t data_length, int64_t encoded_length) override {
    client_sink_->OnDataDownloaded(data_length, encoded_length);
  }

  void OnUploadProgress(int64_t current_position,
                        int64_t total_size,
                        OnUploadProgressCallback callback) override {
    client_sink_->OnUploadProgress(current_position, total_size,
                                   std::move(callback));
  }

  void OnReceiveCachedMetadata(const std::vector<uint8_t>& data) override {
    client_sink_->OnReceiveCachedMetadata(data);
  }

  void OnTransferSizeUpdated(int32_t transfer_size_diff) override {
    client_sink_->OnTransferSizeUpdated(transfer_size_diff);
  }

  void OnStartLoadingResponseBody(
      mojo::ScopedDataPipeConsumerHandle body) override {
    client_sink_->OnStartLoadingResponseBody(std::move(body));
  }

  void OnComplete(const network::URLLoaderCompletionStatus& status) override {
    client_sink_->OnComplete(status);
  }

 private:
  network::mojom::URLLoaderPtr loader_sink_;
  mojo::Binding<network::mojom::URLLoaderClient> client_source_binding_;
  network::mojom::URLLoaderClientPtr client_sink_;
};

}  // namespace

ChildURLLoaderFactoryBundleInfo::ChildURLLoaderFactoryBundleInfo() = default;

ChildURLLoaderFactoryBundleInfo::ChildURLLoaderFactoryBundleInfo(
    std::unique_ptr<URLLoaderFactoryBundleInfo> base_info)
    : URLLoaderFactoryBundleInfo(std::move(base_info->default_factory_info()),
                                 std::move(base_info->factories_info())) {}

ChildURLLoaderFactoryBundleInfo::ChildURLLoaderFactoryBundleInfo(
    network::mojom::URLLoaderFactoryPtrInfo default_factory_info,
    std::map<std::string, network::mojom::URLLoaderFactoryPtrInfo>
        factories_info,
    PossiblyAssociatedURLLoaderFactoryPtrInfo direct_network_factory_info)
    : URLLoaderFactoryBundleInfo(std::move(default_factory_info),
                                 std::move(factories_info)),
      direct_network_factory_info_(std::move(direct_network_factory_info)) {}

ChildURLLoaderFactoryBundleInfo::~ChildURLLoaderFactoryBundleInfo() = default;

scoped_refptr<network::SharedURLLoaderFactory>
ChildURLLoaderFactoryBundleInfo::CreateFactory() {
  auto other = std::make_unique<ChildURLLoaderFactoryBundleInfo>();
  other->default_factory_info_ = std::move(default_factory_info_);
  other->factories_info_ = std::move(factories_info_);
  other->direct_network_factory_info_ = std::move(direct_network_factory_info_);

  return base::MakeRefCounted<ChildURLLoaderFactoryBundle>(std::move(other));
}

// -----------------------------------------------------------------------------

ChildURLLoaderFactoryBundle::ChildURLLoaderFactoryBundle() = default;

ChildURLLoaderFactoryBundle::ChildURLLoaderFactoryBundle(
    std::unique_ptr<ChildURLLoaderFactoryBundleInfo> info) {
  Update(std::move(info), base::nullopt);
}

ChildURLLoaderFactoryBundle::ChildURLLoaderFactoryBundle(
    PossiblyAssociatedFactoryGetterCallback direct_network_factory_getter,
    FactoryGetterCallback default_blob_factory_getter)
    : direct_network_factory_getter_(std::move(direct_network_factory_getter)),
      default_blob_factory_getter_(std::move(default_blob_factory_getter)) {}

ChildURLLoaderFactoryBundle::~ChildURLLoaderFactoryBundle() = default;

network::mojom::URLLoaderFactory* ChildURLLoaderFactoryBundle::GetFactoryForURL(
    const GURL& url) {
  if (url.SchemeIsBlob())
    InitDefaultBlobFactoryIfNecessary();

  auto it = factories_.find(url.scheme());
  if (it != factories_.end())
    return it->second.get();

  if (default_factory_)
    return default_factory_.get();

  InitDirectNetworkFactoryIfNecessary();
  DCHECK(direct_network_factory_);
  return direct_network_factory_.get();
}

void ChildURLLoaderFactoryBundle::CreateLoaderAndStart(
    network::mojom::URLLoaderRequest loader,
    int32_t routing_id,
    int32_t request_id,
    uint32_t options,
    const network::ResourceRequest& request,
    network::mojom::URLLoaderClientPtr client,
    const net::MutableNetworkTrafficAnnotationTag& traffic_annotation) {
  auto override_iter = subresource_overrides_.find(request.url);
  if (override_iter != subresource_overrides_.end()) {
    mojom::TransferrableURLLoaderPtr transferrable_loader =
        std::move(override_iter->second);
    subresource_overrides_.erase(override_iter);

    client->OnReceiveResponse(transferrable_loader->head, nullptr);
    mojo::MakeStrongBinding(
        std::make_unique<URLLoaderRelay>(
            network::mojom::URLLoaderPtr(
                std::move(transferrable_loader->url_loader)),
            std::move(transferrable_loader->url_loader_client),
            std::move(client)),
        std::move(loader));

    return;
  }

  network::mojom::URLLoaderFactory* factory_ptr = GetFactoryForURL(request.url);

  factory_ptr->CreateLoaderAndStart(std::move(loader), routing_id, request_id,
                                    options, request, std::move(client),
                                    traffic_annotation);
}

std::unique_ptr<network::SharedURLLoaderFactoryInfo>
ChildURLLoaderFactoryBundle::Clone() {
  return CloneInternal(true /* include_default */);
}

std::unique_ptr<network::SharedURLLoaderFactoryInfo>
ChildURLLoaderFactoryBundle::CloneWithoutDefaultFactory() {
  return CloneInternal(false /* include_default */);
}

void ChildURLLoaderFactoryBundle::Update(
    std::unique_ptr<ChildURLLoaderFactoryBundleInfo> info,
    base::Optional<std::vector<mojom::TransferrableURLLoaderPtr>>
        subresource_overrides) {
  if (info->direct_network_factory_info()) {
    direct_network_factory_.Bind(
        std::move(info->direct_network_factory_info()));
  }
  URLLoaderFactoryBundle::Update(std::move(info));

  if (subresource_overrides) {
    for (auto& element : *subresource_overrides) {
      subresource_overrides_[element->url] = std::move(element);
    }
  }
}

bool ChildURLLoaderFactoryBundle::IsHostChildURLLoaderFactoryBundle() const {
  return false;
}

void ChildURLLoaderFactoryBundle::InitDefaultBlobFactoryIfNecessary() {
  if (default_blob_factory_getter_.is_null())
    return;

  if (factories_.find(url::kBlobScheme) == factories_.end()) {
    network::mojom::URLLoaderFactoryPtr blob_factory =
        std::move(default_blob_factory_getter_).Run();
    if (blob_factory)
      factories_.emplace(url::kBlobScheme, std::move(blob_factory));
  } else {
    default_blob_factory_getter_.Reset();
  }
}

void ChildURLLoaderFactoryBundle::InitDirectNetworkFactoryIfNecessary() {
  if (direct_network_factory_getter_.is_null())
    return;

  if (!direct_network_factory_) {
    direct_network_factory_ = std::move(direct_network_factory_getter_).Run();
  } else {
    direct_network_factory_getter_.Reset();
  }
}

std::unique_ptr<network::SharedURLLoaderFactoryInfo>
ChildURLLoaderFactoryBundle::CloneInternal(bool include_default) {
  InitDefaultBlobFactoryIfNecessary();
  InitDirectNetworkFactoryIfNecessary();

  network::mojom::URLLoaderFactoryPtrInfo default_factory_info;
  if (include_default && default_factory_)
    default_factory_->Clone(mojo::MakeRequest(&default_factory_info));

  std::map<std::string, network::mojom::URLLoaderFactoryPtrInfo> factories_info;
  for (auto& factory : factories_) {
    network::mojom::URLLoaderFactoryPtrInfo factory_info;
    factory.second->Clone(mojo::MakeRequest(&factory_info));
    factories_info.emplace(factory.first, std::move(factory_info));
  }

  network::mojom::URLLoaderFactoryPtrInfo direct_network_factory_info;
  if (direct_network_factory_) {
    direct_network_factory_->Clone(
        mojo::MakeRequest(&direct_network_factory_info));
  }

  // Currently there is no need to override subresources from workers,
  // therefore |subresource_overrides| are not shared with the clones.

  return std::make_unique<ChildURLLoaderFactoryBundleInfo>(
      std::move(default_factory_info), std::move(factories_info),
      std::move(direct_network_factory_info));
}

std::unique_ptr<ChildURLLoaderFactoryBundleInfo>
ChildURLLoaderFactoryBundle::PassInterface() {
  InitDefaultBlobFactoryIfNecessary();
  InitDirectNetworkFactoryIfNecessary();

  network::mojom::URLLoaderFactoryPtrInfo default_factory_info;
  if (default_factory_)
    default_factory_info = default_factory_.PassInterface();

  std::map<std::string, network::mojom::URLLoaderFactoryPtrInfo> factories_info;
  for (auto& factory : factories_) {
    factories_info.emplace(factory.first, factory.second.PassInterface());
  }

  PossiblyAssociatedInterfacePtrInfo<network::mojom::URLLoaderFactory>
      direct_network_factory_info;
  if (direct_network_factory_) {
    direct_network_factory_info = direct_network_factory_.PassInterface();
  }

  return std::make_unique<ChildURLLoaderFactoryBundleInfo>(
      std::move(default_factory_info), std::move(factories_info),
      std::move(direct_network_factory_info));
}

}  // namespace content
