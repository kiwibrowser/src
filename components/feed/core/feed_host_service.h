// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_FEED_CORE_FEED_HOST_SERVICE_H_
#define COMPONENTS_FEED_CORE_FEED_HOST_SERVICE_H_

#include <memory>

#include "base/macros.h"
#include "components/feed/core/feed_image_manager.h"
#include "components/feed/core/feed_networking_host.h"
#include "components/keyed_service/core/keyed_service.h"

namespace feed {

// KeyedService responsible for managing the lifetime of Feed Host API
// implementations. It instantiates and owns these API implementations, and
// provides access to non-owning pointers to them. While host implementations
// may be created on demand, it is possible they will not be fully initialized
// yet.
class FeedHostService : public KeyedService {
 public:
  FeedHostService(std::unique_ptr<FeedImageManager> image_manager,
                  std::unique_ptr<FeedNetworkingHost> networking_host);
  ~FeedHostService() override;

  FeedImageManager* GetImageManager();
  FeedNetworkingHost* GetNetworkingHost();

 private:
  std::unique_ptr<FeedImageManager> image_manager_;
  std::unique_ptr<FeedNetworkingHost> networking_host_;

  DISALLOW_COPY_AND_ASSIGN(FeedHostService);
};

}  // namespace feed

#endif  // COMPONENTS_FEED_CORE_FEED_HOST_SERVICE_H_
