// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/loader/resource/font_resource.h"

#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/blink/public/platform/modules/fetch/fetch_api_request.mojom-shared.h"
#include "third_party/blink/public/platform/platform.h"
#include "third_party/blink/public/platform/web_url_loader_mock_factory.h"
#include "third_party/blink/renderer/core/css/css_font_face_src_value.h"
#include "third_party/blink/renderer/core/loader/resource/mock_font_resource_client.h"
#include "third_party/blink/renderer/core/testing/dummy_page_holder.h"
#include "third_party/blink/renderer/platform/exported/wrapped_resource_response.h"
#include "third_party/blink/renderer/platform/loader/fetch/fetch_parameters.h"
#include "third_party/blink/renderer/platform/loader/fetch/memory_cache.h"
#include "third_party/blink/renderer/platform/loader/fetch/resource_error.h"
#include "third_party/blink/renderer/platform/loader/fetch/resource_fetcher.h"
#include "third_party/blink/renderer/platform/loader/fetch/resource_loader.h"
#include "third_party/blink/renderer/platform/loader/fetch/resource_request.h"
#include "third_party/blink/renderer/platform/loader/fetch/resource_response.h"
#include "third_party/blink/renderer/platform/loader/testing/mock_fetch_context.h"
#include "third_party/blink/renderer/platform/loader/testing/mock_resource_client.h"
#include "third_party/blink/renderer/platform/runtime_enabled_features.h"
#include "third_party/blink/renderer/platform/weborigin/kurl.h"

namespace blink {

class FontResourceTest : public testing::Test {
  void TearDown() override {
    Platform::Current()
        ->GetURLLoaderMockFactory()
        ->UnregisterAllURLsAndClearMemoryCache();
  }
};

// Tests if ResourceFetcher works fine with FontResource that requires defered
// loading supports.
TEST_F(FontResourceTest,
       ResourceFetcherRevalidateDeferedResourceFromTwoInitiators) {
  KURL url("http://127.0.0.1:8000/font.woff");
  ResourceResponse response(url);
  response.SetHTTPStatusCode(200);
  response.SetHTTPHeaderField(HTTPNames::ETag, "1234567890");
  Platform::Current()->GetURLLoaderMockFactory()->RegisterURL(
      url, WrappedResourceResponse(response), "");

  MockFetchContext* context =
      MockFetchContext::Create(MockFetchContext::kShouldLoadNewResource);
  ResourceFetcher* fetcher = ResourceFetcher::Create(context);

  // Fetch to cache a resource.
  ResourceRequest request1(url);
  FetchParameters fetch_params1(request1);
  Resource* resource1 = FontResource::Fetch(fetch_params1, fetcher, nullptr);
  ASSERT_FALSE(resource1->ErrorOccurred());
  fetcher->StartLoad(resource1);
  Platform::Current()->GetURLLoaderMockFactory()->ServeAsynchronousRequests();
  EXPECT_TRUE(resource1->IsLoaded());
  EXPECT_FALSE(resource1->ErrorOccurred());

  // Set the context as it is on reloads.
  context->SetLoadComplete(true);

  // Revalidate the resource.
  ResourceRequest request2(url);
  request2.SetCacheMode(mojom::FetchCacheMode::kValidateCache);
  FetchParameters fetch_params2(request2);
  Resource* resource2 = FontResource::Fetch(fetch_params2, fetcher, nullptr);
  ASSERT_FALSE(resource2->ErrorOccurred());
  EXPECT_EQ(resource1, resource2);
  EXPECT_TRUE(resource2->IsCacheValidator());
  EXPECT_TRUE(resource2->StillNeedsLoad());

  // Fetch the same resource again before actual load operation starts.
  ResourceRequest request3(url);
  request3.SetCacheMode(mojom::FetchCacheMode::kValidateCache);
  FetchParameters fetch_params3(request3);
  Resource* resource3 = FontResource::Fetch(fetch_params3, fetcher, nullptr);
  ASSERT_FALSE(resource3->ErrorOccurred());
  EXPECT_EQ(resource2, resource3);
  EXPECT_TRUE(resource3->IsCacheValidator());
  EXPECT_TRUE(resource3->StillNeedsLoad());

  // StartLoad() can be called from any initiator. Here, call it from the
  // latter.
  fetcher->StartLoad(resource3);
  Platform::Current()->GetURLLoaderMockFactory()->ServeAsynchronousRequests();
  EXPECT_TRUE(resource3->IsLoaded());
  EXPECT_FALSE(resource3->ErrorOccurred());
  EXPECT_TRUE(resource2->IsLoaded());
  EXPECT_FALSE(resource2->ErrorOccurred());

  GetMemoryCache()->Remove(resource1);
}

// Tests if cache-aware font loading works correctly.
TEST_F(FontResourceTest, CacheAwareFontLoading) {
  KURL url("http://127.0.0.1:8000/font.woff");
  ResourceResponse response(url);
  response.SetHTTPStatusCode(200);
  Platform::Current()->GetURLLoaderMockFactory()->RegisterURL(
      url, WrappedResourceResponse(response), "");

  RuntimeEnabledFeatures::Backup features_backup;
  RuntimeEnabledFeatures::SetWebFontsCacheAwareTimeoutAdaptationEnabled(true);

  std::unique_ptr<DummyPageHolder> dummy_page_holder =
      DummyPageHolder::Create(IntSize(800, 600));
  Document& document = dummy_page_holder->GetDocument();
  ResourceFetcher* fetcher = document.Fetcher();
  CSSFontFaceSrcValue* src_value = CSSFontFaceSrcValue::Create(
      url.GetString(), url.GetString(),
      Referrer(document.Url(), document.GetReferrerPolicy()),
      kDoNotCheckContentSecurityPolicy);

  // Route font requests in this test through CSSFontFaceSrcValue::Fetch
  // instead of calling FontResource::Fetch directly. CSSFontFaceSrcValue
  // requests a FontResource only once, and skips calling FontResource::Fetch
  // on future CSSFontFaceSrcValue::Fetch calls. This tests wants to ensure
  // correct behavior in the case where we reuse a FontResource without it being
  // a "cache hit" in ResourceFetcher's view.
  Persistent<MockFontResourceClient> client = new MockFontResourceClient;
  FontResource& resource = src_value->Fetch(&document, client);

  fetcher->StartLoad(&resource);
  EXPECT_TRUE(resource.Loader()->IsCacheAwareLoadingActivated());
  resource.load_limit_state_ = FontResource::kUnderLimit;

  // FontResource callbacks should be blocked during cache-aware loading.
  resource.FontLoadShortLimitCallback();
  EXPECT_FALSE(client->FontLoadShortLimitExceededCalled());

  // Fail first request as disk cache miss.
  resource.Loader()->HandleError(ResourceError::CacheMissError(url));

  // Once cache miss error returns, previously blocked callbacks should be
  // called immediately.
  EXPECT_FALSE(resource.Loader()->IsCacheAwareLoadingActivated());
  EXPECT_TRUE(client->FontLoadShortLimitExceededCalled());
  EXPECT_FALSE(client->FontLoadLongLimitExceededCalled());

  // Add client now, FontLoadShortLimitExceeded() should be called.
  Persistent<MockFontResourceClient> client2 = new MockFontResourceClient;
  FontResource& resource2 = src_value->Fetch(&document, client2);
  EXPECT_EQ(&resource, &resource2);
  EXPECT_TRUE(client2->FontLoadShortLimitExceededCalled());
  EXPECT_FALSE(client2->FontLoadLongLimitExceededCalled());

  // FontResource callbacks are not blocked now.
  resource.FontLoadLongLimitCallback();
  EXPECT_TRUE(client->FontLoadLongLimitExceededCalled());

  // Add client now, both callbacks should be called.
  Persistent<MockFontResourceClient> client3 = new MockFontResourceClient;
  FontResource& resource3 = src_value->Fetch(&document, client3);
  EXPECT_EQ(&resource, &resource3);
  EXPECT_TRUE(client3->FontLoadShortLimitExceededCalled());
  EXPECT_TRUE(client3->FontLoadLongLimitExceededCalled());

  Platform::Current()->GetURLLoaderMockFactory()->ServeAsynchronousRequests();
  GetMemoryCache()->Remove(&resource);

  features_backup.Restore();
}

}  // namespace blink
