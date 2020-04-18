// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/platform/loader/testing/fetch_testing_platform_support.h"

#include <memory>
#include "third_party/blink/public/platform/platform.h"
#include "third_party/blink/public/platform/web_url.h"
#include "third_party/blink/public/platform/web_url_loader.h"
#include "third_party/blink/public/platform/web_url_loader_mock_factory.h"
#include "third_party/blink/renderer/platform/loader/fetch/resource_error.h"
#include "third_party/blink/renderer/platform/loader/testing/mock_fetch_context.h"
#include "third_party/blink/renderer/platform/loader/testing/web_url_loader_factory_with_mock.h"
#include "third_party/blink/renderer/platform/testing/weburl_loader_mock_factory_impl.h"

namespace blink {

FetchTestingPlatformSupport::FetchTestingPlatformSupport()
    : url_loader_mock_factory_(new WebURLLoaderMockFactoryImpl(this)) {}

FetchTestingPlatformSupport::~FetchTestingPlatformSupport() {
  // Shutdowns WebURLLoaderMockFactory gracefully, serving all pending requests
  // first, then flushing all registered URLs.
  url_loader_mock_factory_->ServeAsynchronousRequests();
  url_loader_mock_factory_->UnregisterAllURLsAndClearMemoryCache();
}

MockFetchContext* FetchTestingPlatformSupport::Context() {
  if (!context_) {
    context_ =
        MockFetchContext::Create(MockFetchContext::kShouldLoadNewResource);
  }
  return context_;
}

WebURLLoaderMockFactory*
FetchTestingPlatformSupport::GetURLLoaderMockFactory() {
  return url_loader_mock_factory_.get();
}

std::unique_ptr<WebURLLoaderFactory>
FetchTestingPlatformSupport::CreateDefaultURLLoaderFactory() {
  return std::make_unique<WebURLLoaderFactoryWithMock>(
      url_loader_mock_factory_.get());
}

}  // namespace blink
