// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/url_loader_factory_getter.h"

#include <memory>
#include <utility>

#include "base/bind.h"
#include "base/feature_list.h"
#include "base/lazy_instance.h"
#include "content/browser/storage_partition_impl.h"
#include "content/common/service_worker/service_worker_utils.h"
#include "services/network/public/cpp/features.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"
#include "services/network/public/mojom/network_service.mojom.h"

namespace content {

namespace {
base::LazyInstance<URLLoaderFactoryGetter::GetNetworkFactoryCallback>::Leaky
    g_get_network_factory_callback = LAZY_INSTANCE_INITIALIZER;
}

class URLLoaderFactoryGetter::URLLoaderFactoryForIOThreadInfo
    : public network::SharedURLLoaderFactoryInfo {
 public:
  URLLoaderFactoryForIOThreadInfo() = default;
  explicit URLLoaderFactoryForIOThreadInfo(
      scoped_refptr<URLLoaderFactoryGetter> factory_getter)
      : factory_getter_(std::move(factory_getter)) {}
  ~URLLoaderFactoryForIOThreadInfo() override = default;

  scoped_refptr<URLLoaderFactoryGetter>& url_loader_factory_getter() {
    return factory_getter_;
  }

 protected:
  // SharedURLLoaderFactoryInfo implementation.
  scoped_refptr<network::SharedURLLoaderFactory> CreateFactory() override;

  scoped_refptr<URLLoaderFactoryGetter> factory_getter_;

  DISALLOW_COPY_AND_ASSIGN(URLLoaderFactoryForIOThreadInfo);
};

class URLLoaderFactoryGetter::URLLoaderFactoryForIOThread
    : public network::SharedURLLoaderFactory {
 public:
  explicit URLLoaderFactoryForIOThread(
      scoped_refptr<URLLoaderFactoryGetter> factory_getter)
      : factory_getter_(std::move(factory_getter)) {
    DCHECK_CURRENTLY_ON(BrowserThread::IO);
  }

  explicit URLLoaderFactoryForIOThread(
      std::unique_ptr<URLLoaderFactoryForIOThreadInfo> info)
      : factory_getter_(std::move(info->url_loader_factory_getter())) {
    DCHECK_CURRENTLY_ON(BrowserThread::IO);
  }

  // mojom::URLLoaderFactory implementation:
  void CreateLoaderAndStart(network::mojom::URLLoaderRequest request,
                            int32_t routing_id,
                            int32_t request_id,
                            uint32_t options,
                            const network::ResourceRequest& url_request,
                            network::mojom::URLLoaderClientPtr client,
                            const net::MutableNetworkTrafficAnnotationTag&
                                traffic_annotation) override {
    DCHECK(BrowserThread::CurrentlyOn(BrowserThread::IO));
    if (!factory_getter_)
      return;
    factory_getter_->GetURLLoaderFactory()->CreateLoaderAndStart(
        std::move(request), routing_id, request_id, options, url_request,
        std::move(client), traffic_annotation);
  }

  void Clone(network::mojom::URLLoaderFactoryRequest request) override {
    if (!factory_getter_)
      return;
    factory_getter_->GetURLLoaderFactory()->Clone(std::move(request));
  }

  // SharedURLLoaderFactory implementation:
  std::unique_ptr<network::SharedURLLoaderFactoryInfo> Clone() override {
    NOTREACHED() << "This isn't supported. If you need a SharedURLLoaderFactory"
                    " on the UI thread, get it from StoragePartition.";
    return nullptr;
  }

 private:
  friend class base::RefCounted<URLLoaderFactoryForIOThread>;
  ~URLLoaderFactoryForIOThread() override = default;

  scoped_refptr<URLLoaderFactoryGetter> factory_getter_;

  DISALLOW_COPY_AND_ASSIGN(URLLoaderFactoryForIOThread);
};

scoped_refptr<network::SharedURLLoaderFactory>
URLLoaderFactoryGetter::URLLoaderFactoryForIOThreadInfo::CreateFactory() {
  auto other = std::make_unique<URLLoaderFactoryForIOThreadInfo>();
  other->factory_getter_ = std::move(factory_getter_);

  return base::MakeRefCounted<URLLoaderFactoryForIOThread>(std::move(other));
}

// -----------------------------------------------------------------------------

URLLoaderFactoryGetter::URLLoaderFactoryGetter() {}

void URLLoaderFactoryGetter::Initialize(StoragePartitionImpl* partition) {
  DCHECK(partition);
  DCHECK(!pending_network_factory_request_.is_pending());
  DCHECK(!pending_blob_factory_request_.is_pending());

  partition_ = partition;
  network::mojom::URLLoaderFactoryPtr network_factory;
  network::mojom::URLLoaderFactoryPtr blob_factory;
  pending_network_factory_request_ = MakeRequest(&network_factory);
  pending_blob_factory_request_ = mojo::MakeRequest(&blob_factory);

  // If NetworkService is disabled, HandleFactoryRequests should be called after
  // NetworkContext in |partition_| is ready.
  if (base::FeatureList::IsEnabled(network::features::kNetworkService))
    HandleFactoryRequests();

  BrowserThread::PostTask(
      BrowserThread::IO, FROM_HERE,
      base::BindOnce(&URLLoaderFactoryGetter::InitializeOnIOThread, this,
                     network_factory.PassInterface(),
                     blob_factory.PassInterface()));
}

void URLLoaderFactoryGetter::HandleFactoryRequests() {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  DCHECK(pending_network_factory_request_.is_pending());
  DCHECK(pending_blob_factory_request_.is_pending());
  HandleNetworkFactoryRequestOnUIThread(
      std::move(pending_network_factory_request_));

  // |partition->blob_url_loader_factory_| is not available without the feature.
  if (base::FeatureList::IsEnabled(network::features::kNetworkService) ||
      ServiceWorkerUtils::IsServicificationEnabled()) {
    DCHECK(partition_->GetBlobURLLoaderFactory());
    partition_->GetBlobURLLoaderFactory()->HandleRequest(
        std::move(pending_blob_factory_request_));
  } else {
    pending_blob_factory_request_ = nullptr;
  }
}

void URLLoaderFactoryGetter::OnStoragePartitionDestroyed() {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  partition_ = nullptr;
}

scoped_refptr<network::SharedURLLoaderFactory>
URLLoaderFactoryGetter::GetNetworkFactory() {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  return base::MakeRefCounted<URLLoaderFactoryForIOThread>(
      base::WrapRefCounted(this));
}

std::unique_ptr<network::SharedURLLoaderFactoryInfo>
URLLoaderFactoryGetter::GetNetworkFactoryInfo() {
  return std::make_unique<URLLoaderFactoryForIOThreadInfo>(
      base::WrapRefCounted(this));
}

network::mojom::URLLoaderFactory*
URLLoaderFactoryGetter::GetURLLoaderFactory() {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  if (g_get_network_factory_callback.Get() && !test_factory_)
    g_get_network_factory_callback.Get().Run(this);

  if (test_factory_)
    return test_factory_;

  if (!network_factory_.is_bound() || network_factory_.encountered_error()) {
    BrowserThread::PostTask(
        BrowserThread::UI, FROM_HERE,
        base::BindOnce(
            &URLLoaderFactoryGetter::HandleNetworkFactoryRequestOnUIThread,
            this, mojo::MakeRequest(&network_factory_)));
  }
  return network_factory_.get();
}

void URLLoaderFactoryGetter::CloneNetworkFactory(
    network::mojom::URLLoaderFactoryRequest network_factory_request) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  GetURLLoaderFactory()->Clone(std::move(network_factory_request));
}

network::mojom::URLLoaderFactory* URLLoaderFactoryGetter::GetBlobFactory() {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  return blob_factory_.get();
}

void URLLoaderFactoryGetter::SetNetworkFactoryForTesting(
    network::mojom::URLLoaderFactory* test_factory) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  DCHECK(!test_factory_ || !test_factory);
  test_factory_ = test_factory;
}

void URLLoaderFactoryGetter::SetGetNetworkFactoryCallbackForTesting(
    const GetNetworkFactoryCallback& get_network_factory_callback) {
  DCHECK(!BrowserThread::IsThreadInitialized(BrowserThread::IO) ||
         BrowserThread::CurrentlyOn(BrowserThread::IO));
  DCHECK(!g_get_network_factory_callback.Get() ||
         !get_network_factory_callback);
  g_get_network_factory_callback.Get() = get_network_factory_callback;
}

void URLLoaderFactoryGetter::FlushNetworkInterfaceOnIOThreadForTesting() {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  base::RunLoop run_loop;
  BrowserThread::PostTaskAndReply(
      BrowserThread::IO, FROM_HERE,
      base::BindOnce(&URLLoaderFactoryGetter::FlushNetworkInterfaceForTesting,
                     this),
      run_loop.QuitClosure());
  run_loop.Run();
}

void URLLoaderFactoryGetter::FlushNetworkInterfaceForTesting() {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  if (network_factory_)
    network_factory_.FlushForTesting();
}

URLLoaderFactoryGetter::~URLLoaderFactoryGetter() {}

void URLLoaderFactoryGetter::InitializeOnIOThread(
    network::mojom::URLLoaderFactoryPtrInfo network_factory,
    network::mojom::URLLoaderFactoryPtrInfo blob_factory) {
  network_factory_.Bind(std::move(network_factory));
  blob_factory_.Bind(std::move(blob_factory));
}

void URLLoaderFactoryGetter::HandleNetworkFactoryRequestOnUIThread(
    network::mojom::URLLoaderFactoryRequest network_factory_request) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  // |StoragePartitionImpl| may have went away while |URLLoaderFactoryGetter| is
  // still held by consumers.
  if (!partition_)
    return;
  network::mojom::URLLoaderFactoryParamsPtr params =
      network::mojom::URLLoaderFactoryParams::New();
  params->process_id = network::mojom::kBrowserProcessId;
  params->is_corb_enabled = false;
  partition_->GetNetworkContext()->CreateURLLoaderFactory(
      std::move(network_factory_request), std::move(params));
}

}  // namespace content
