// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_DOWNLOAD_NETWORK_DOWNLOAD_URL_LOADER_FACTORY_GETTER_H_
#define CONTENT_BROWSER_DOWNLOAD_NETWORK_DOWNLOAD_URL_LOADER_FACTORY_GETTER_H_

#include "components/download/public/common/download_url_loader_factory_getter.h"
#include "services/network/public/mojom/url_loader_factory.mojom.h"

namespace content {

class URLLoaderFactoryGetter;

// Wrapper of URLLoaderFactoryGetter to retrieve URLLoaderFactory.
class NetworkDownloadURLLoaderFactoryGetter
    : public download::DownloadURLLoaderFactoryGetter {
 public:
  NetworkDownloadURLLoaderFactoryGetter(
      scoped_refptr<URLLoaderFactoryGetter> url_loader_factory_getter,
      network::mojom::URLLoaderFactoryPtrInfo proxy_factory_ptr_info,
      network::mojom::URLLoaderFactoryRequest proxy_factory_request);

  // download::DownloadURLLoaderFactoryGetter implementation.
  scoped_refptr<network::SharedURLLoaderFactory> GetURLLoaderFactory() override;

 protected:
  ~NetworkDownloadURLLoaderFactoryGetter() override;

 private:
  scoped_refptr<URLLoaderFactoryGetter> url_loader_factory_getter_;
  scoped_refptr<network::SharedURLLoaderFactory> lazy_factory_;
  network::mojom::URLLoaderFactoryPtrInfo proxy_factory_ptr_info_;
  network::mojom::URLLoaderFactoryRequest proxy_factory_request_;

  DISALLOW_COPY_AND_ASSIGN(NetworkDownloadURLLoaderFactoryGetter);
};

}  // namespace content

#endif  // CONTENT_BROWSER_DOWNLOAD_NETWORK_DOWNLOAD_URL_LOADER_FACTORY_GETTER_H_
