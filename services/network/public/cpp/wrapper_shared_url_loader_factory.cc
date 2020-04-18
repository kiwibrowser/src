// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/network/public/cpp/wrapper_shared_url_loader_factory.h"

namespace network {

WrapperSharedURLLoaderFactoryInfo::WrapperSharedURLLoaderFactoryInfo() =
    default;

WrapperSharedURLLoaderFactoryInfo::WrapperSharedURLLoaderFactoryInfo(
    network::mojom::URLLoaderFactoryPtrInfo factory_ptr_info)
    : factory_ptr_info_(std::move(factory_ptr_info)) {}

WrapperSharedURLLoaderFactoryInfo::~WrapperSharedURLLoaderFactoryInfo() =
    default;

scoped_refptr<network::SharedURLLoaderFactory>
WrapperSharedURLLoaderFactoryInfo::CreateFactory() {
  return base::MakeRefCounted<WrapperSharedURLLoaderFactory>(
      std::move(factory_ptr_info_));
}

}  // namespace network
