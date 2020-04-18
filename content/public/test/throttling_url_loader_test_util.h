// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_PUBLIC_TEST_THROTTLING_URL_LOADER_TEST_UTIL_H_
#define CONTENT_PUBLIC_TEST_THROTTLING_URL_LOADER_TEST_UTIL_H_

#include <memory>
#include <vector>

#include "base/memory/scoped_refptr.h"
#include "base/single_thread_task_runner.h"
#include "content/public/common/url_loader_throttle.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"

namespace content {

// Allows tests outside of content to interface with a ThrottlingURLLoader.
std::unique_ptr<network::mojom::URLLoaderClient> CreateThrottlingLoaderAndStart(
    scoped_refptr<network::SharedURLLoaderFactory> factory,
    std::vector<std::unique_ptr<URLLoaderThrottle>> throttles,
    int32_t routing_id,
    int32_t request_id,
    uint32_t options,
    network::ResourceRequest* url_request,
    network::mojom::URLLoaderClient* client,
    const net::NetworkTrafficAnnotationTag& traffic_annotation,
    scoped_refptr<base::SingleThreadTaskRunner> task_runner);

}  // namespace content

#endif  // CONTENT_PUBLIC_TEST_THROTTLING_URL_LOADER_TEST_UTIL_H_
