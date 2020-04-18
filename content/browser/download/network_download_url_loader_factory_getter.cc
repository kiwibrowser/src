// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/download/network_download_url_loader_factory_getter.h"

#include "components/download/public/common/download_task_runner.h"
#include "content/browser/url_loader_factory_getter.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"
#include "services/network/public/cpp/wrapper_shared_url_loader_factory.h"

namespace content {

NetworkDownloadURLLoaderFactoryGetter::NetworkDownloadURLLoaderFactoryGetter(
    scoped_refptr<URLLoaderFactoryGetter> url_loader_factory_getter,
    network::mojom::URLLoaderFactoryPtrInfo proxy_factory_ptr_info,
    network::mojom::URLLoaderFactoryRequest proxy_factory_request)
    : url_loader_factory_getter_(url_loader_factory_getter),
      proxy_factory_ptr_info_(std::move(proxy_factory_ptr_info)),
      proxy_factory_request_(std::move(proxy_factory_request)) {}

NetworkDownloadURLLoaderFactoryGetter::
    ~NetworkDownloadURLLoaderFactoryGetter() = default;

scoped_refptr<network::SharedURLLoaderFactory>
NetworkDownloadURLLoaderFactoryGetter::GetURLLoaderFactory() {
  DCHECK(download::GetIOTaskRunner());
  DCHECK(download::GetIOTaskRunner()->BelongsToCurrentThread());
  if (lazy_factory_)
    return lazy_factory_;
  if (proxy_factory_request_.is_pending()) {
    url_loader_factory_getter_->CloneNetworkFactory(
        std::move(proxy_factory_request_));
    lazy_factory_ =
        base::MakeRefCounted<network::WrapperSharedURLLoaderFactory>(
            std::move(proxy_factory_ptr_info_));
  } else {
    lazy_factory_ = url_loader_factory_getter_->GetNetworkFactory();
  }
  return lazy_factory_;
}

}  // namespace content
