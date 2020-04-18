// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/download/download_url_loader_factory_getter_impl.h"

namespace content {

DownloadURLLoaderFactoryGetterImpl::DownloadURLLoaderFactoryGetterImpl(
    std::unique_ptr<network::SharedURLLoaderFactoryInfo> url_loader_factory)
    : url_loader_factory_info_(std::move(url_loader_factory)) {}

DownloadURLLoaderFactoryGetterImpl::~DownloadURLLoaderFactoryGetterImpl() =
    default;

scoped_refptr<network::SharedURLLoaderFactory>
DownloadURLLoaderFactoryGetterImpl::GetURLLoaderFactory() {
  if (!url_loader_factory_) {
    url_loader_factory_ = network::SharedURLLoaderFactory::Create(
        std::move(url_loader_factory_info_));
  }
  return url_loader_factory_;
}

}  // namespace content
