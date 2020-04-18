// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/feed/core/feed_host_service.h"

#include <utility>

namespace feed {

FeedHostService::FeedHostService(
    std::unique_ptr<FeedImageManager> image_manager,
    std::unique_ptr<FeedNetworkingHost> networking_host)
    : image_manager_(std::move(image_manager)),
      networking_host_(std::move(networking_host)) {}

FeedHostService::~FeedHostService() = default;

FeedImageManager* FeedHostService::GetImageManager() {
  return image_manager_.get();
}

FeedNetworkingHost* FeedHostService::GetNetworkingHost() {
  return networking_host_.get();
}

}  // namespace feed
