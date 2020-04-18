// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/offline_pages/core/prefetch/test_prefetch_network_request_factory.h"

#include <memory>
#include <string>

#include "components/offline_pages/core/prefetch/generate_page_bundle_request.h"
#include "components/offline_pages/core/prefetch/get_operation_request.h"

namespace offline_pages {
namespace {
version_info::Channel kChannel = version_info::Channel::UNKNOWN;
const char kUserAgent[] = "Chrome/57.0.2987.133";
}  // namespace

TestPrefetchNetworkRequestFactory::TestPrefetchNetworkRequestFactory()
    : TestPrefetchNetworkRequestFactory(new net::TestURLRequestContextGetter(
          base::ThreadTaskRunnerHandle::Get())) {}

TestPrefetchNetworkRequestFactory::TestPrefetchNetworkRequestFactory(
    net::TestURLRequestContextGetter* request_context_getter)
    : PrefetchNetworkRequestFactoryImpl(request_context_getter,
                                        kChannel,
                                        kUserAgent) {
  request_context = request_context_getter;
}

TestPrefetchNetworkRequestFactory::~TestPrefetchNetworkRequestFactory() =
    default;

}  // namespace offline_pages
