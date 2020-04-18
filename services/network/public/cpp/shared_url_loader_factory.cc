// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/network/public/cpp/shared_url_loader_factory.h"

namespace network {

// static
scoped_refptr<SharedURLLoaderFactory> SharedURLLoaderFactory::Create(
    std::unique_ptr<SharedURLLoaderFactoryInfo> info) {
  return info->CreateFactory();
}

SharedURLLoaderFactory::~SharedURLLoaderFactory() = default;

SharedURLLoaderFactoryInfo::SharedURLLoaderFactoryInfo() = default;

SharedURLLoaderFactoryInfo::~SharedURLLoaderFactoryInfo() = default;

}  // namespace network
