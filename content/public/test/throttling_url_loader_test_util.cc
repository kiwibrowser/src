// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/public/test/throttling_url_loader_test_util.h"

#include <utility>

#include "content/common/throttling_url_loader.h"

namespace content {

std::unique_ptr<network::mojom::URLLoaderClient> CreateThrottlingLoaderAndStart(
    scoped_refptr<network::SharedURLLoaderFactory> factory,
    std::vector<std::unique_ptr<URLLoaderThrottle>> throttles,
    int32_t routing_id,
    int32_t request_id,
    uint32_t options,
    network::ResourceRequest* url_request,
    network::mojom::URLLoaderClient* client,
    const net::NetworkTrafficAnnotationTag& traffic_annotation,
    scoped_refptr<base::SingleThreadTaskRunner> task_runner) {
  return ThrottlingURLLoader::CreateLoaderAndStart(
      std::move(factory), std::move(throttles), routing_id, request_id, options,
      url_request, client, traffic_annotation, std::move(task_runner));
}

}  // namespace content
