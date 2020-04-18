// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/common/navigation_subresource_loader_params.h"

namespace content {

SubresourceLoaderParams::SubresourceLoaderParams() = default;
SubresourceLoaderParams::~SubresourceLoaderParams() = default;

SubresourceLoaderParams::SubresourceLoaderParams(
    SubresourceLoaderParams&& other) {
  *this = std::move(other);
}

SubresourceLoaderParams& SubresourceLoaderParams::operator=(
    SubresourceLoaderParams&& other) {
  loader_factory_info = std::move(other.loader_factory_info);
  controller_service_worker_info =
      std::move(other.controller_service_worker_info);
  controller_service_worker_handle = other.controller_service_worker_handle;
  return *this;
}

}  // namespace content
