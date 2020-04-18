// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/html/parser/css_preload_scanner.h"

#include <memory>

#include "base/macros.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/blink/public/platform/platform.h"
#include "third_party/blink/public/platform/task_type.h"
#include "third_party/blink/public/platform/web_url_loader_mock_factory.h"
#include "third_party/blink/renderer/core/frame/settings.h"
#include "third_party/blink/renderer/core/html/parser/html_resource_preloader.h"
#include "third_party/blink/renderer/core/testing/dummy_page_holder.h"
#include "third_party/blink/renderer/platform/exported/wrapped_resource_response.h"
#include "third_party/blink/renderer/platform/heap/heap.h"
#include "third_party/blink/renderer/platform/loader/fetch/fetch_context.h"
#include "third_party/blink/renderer/platform/loader/fetch/resource.h"
#include "third_party/blink/renderer/platform/loader/fetch/resource_error.h"
#include "third_party/blink/renderer/platform/loader/fetch/resource_fetcher.h"
#include "third_party/blink/renderer/platform/loader/fetch/resource_request.h"
#include "third_party/blink/renderer/platform/weborigin/kurl.h"
#include "third_party/blink/renderer/platform/wtf/text/text_encoding.h"

namespace blink {

namespace {

class CSSMockHTMLResourcePreloader : public HTMLResourcePreloader {

 public:
  explicit CSSMockHTMLResourcePreloader(Document& document,
                                        const char* expected_referrer = nullptr)
      : HTMLResourcePreloader(document),
        expected_referrer_(expected_referrer) {}

  void Preload(std::unique_ptr<PreloadRequest> preload_request,
               const NetworkHintsInterface&) override {
    if (expected_referrer_) {
      Resource* resource = preload_request->Start(GetDocument(), nullptr);
      EXPECT_EQ(expected_referrer_,
                resource->GetResourceRequest().HttpReferrer());
    }
  }

  const char* expected_referrer_;

  DISALLOW_COPY_AND_ASSIGN(CSSMockHTMLResourcePreloader);
};

class PreloadRecordingCSSPreloaderResourceClient final
    : public CSSPreloaderResourceClient {
 public:
  PreloadRecordingCSSPreloaderResourceClient(HTMLResourcePreloader* preloader)
      : CSSPreloaderResourceClient(preloader) {}

  void FetchPreloads(PreloadRequestStream& preloads) override {
    for (const auto& it : preloads) {
      preload_urls_.push_back(it->ResourceURL());
      preload_referrer_policies_.push_back(it->GetReferrerPolicy());
    }
    CSSPreloaderResourceClient::FetchPreloads(preloads);
  }

  Vector<String> preload_urls_;
  Vector<ReferrerPolicy> preload_referrer_policies_;
};

class CSSPreloadScannerTest : public testing::Test {};

}  // namespace

TEST_F(CSSPreloadScannerTest, ScanFromResourceClient) {
  std::unique_ptr<DummyPageHolder> dummy_page_holder =
      DummyPageHolder::Create(IntSize(500, 500));
  dummy_page_holder->GetDocument()
      .GetSettings()
      ->SetCSSExternalScannerNoPreload(true);

  CSSMockHTMLResourcePreloader* preloader =
      new CSSMockHTMLResourcePreloader(dummy_page_holder->GetDocument());

  KURL url("http://127.0.0.1/foo.css");
  Platform::Current()->GetURLLoaderMockFactory()->RegisterURL(
      url, WrappedResourceResponse(ResourceResponse()), "");
  FetchParameters params{ResourceRequest(url)};
  PreloadRecordingCSSPreloaderResourceClient* resource_client =
      new PreloadRecordingCSSPreloaderResourceClient(preloader);
  Resource* resource = CSSStyleSheetResource::Fetch(
      params, dummy_page_holder->GetDocument().Fetcher(), resource_client);
  const char* data = "@import url('http://127.0.0.1/preload.css');";
  resource->AppendData(data, strlen(data));

  EXPECT_EQ(1u, resource_client->preload_urls_.size());
  EXPECT_EQ("http://127.0.0.1/preload.css",
            resource_client->preload_urls_.front());
  Platform::Current()->GetURLLoaderMockFactory()->UnregisterURL(url);
}

// Regression test for crbug.com/608310 where the client is destroyed but was
// not removed from the resource's client list.
TEST_F(CSSPreloadScannerTest, DestroyClientBeforeDataSent) {
  std::unique_ptr<DummyPageHolder> dummy_page_holder =
      DummyPageHolder::Create(IntSize(500, 500));
  dummy_page_holder->GetDocument()
      .GetSettings()
      ->SetCSSExternalScannerNoPreload(true);

  Persistent<CSSMockHTMLResourcePreloader> preloader =
      new CSSMockHTMLResourcePreloader(dummy_page_holder->GetDocument());

  KURL url("http://127.0.0.1/foo.css");
  Platform::Current()->GetURLLoaderMockFactory()->RegisterURL(
      url, WrappedResourceResponse(ResourceResponse()), "");
  FetchParameters params{ResourceRequest(url)};
  PreloadRecordingCSSPreloaderResourceClient* resource_client =
      new PreloadRecordingCSSPreloaderResourceClient(preloader);
  Resource* resource = CSSStyleSheetResource::Fetch(
      params, dummy_page_holder->GetDocument().Fetcher(), resource_client);

  // Destroys the resourceClient.
  ThreadState::Current()->CollectAllGarbage();

  const char* data = "@import url('http://127.0.0.1/preload.css');";
  // Should not crash.
  resource->AppendData(data, strlen(data));
  Platform::Current()->GetURLLoaderMockFactory()->UnregisterURL(url);
}

// Regression test for crbug.com/646869 where the client's data is cleared
// before DataReceived() is called.
TEST_F(CSSPreloadScannerTest, DontReadFromClearedData) {
  std::unique_ptr<DummyPageHolder> dummy_page_holder =
      DummyPageHolder::Create(IntSize(500, 500));
  dummy_page_holder->GetDocument()
      .GetSettings()
      ->SetCSSExternalScannerNoPreload(true);

  CSSMockHTMLResourcePreloader* preloader =
      new CSSMockHTMLResourcePreloader(dummy_page_holder->GetDocument());

  KURL url("data:text/css,@import url('http://127.0.0.1/preload.css');");
  FetchParameters params{ResourceRequest(url)};
  Resource* resource = CSSStyleSheetResource::Fetch(
      params, dummy_page_holder->GetDocument().Fetcher(), nullptr);
  ASSERT_FALSE(resource->ResourceBuffer());

  // Should not crash.
  PreloadRecordingCSSPreloaderResourceClient* resource_client =
      new PreloadRecordingCSSPreloaderResourceClient(preloader);
  Resource* resource2 = CSSStyleSheetResource::Fetch(
      params, dummy_page_holder->GetDocument().Fetcher(), resource_client);
  ASSERT_EQ(resource, resource2);
  EXPECT_EQ(0u, resource_client->preload_urls_.size());
}

// Regression test for crbug.com/645331, where a resource client gets callbacks
// after the document is shutdown and we have no frame.
TEST_F(CSSPreloadScannerTest, DoNotExpectValidDocument) {
  std::unique_ptr<DummyPageHolder> dummy_page_holder =
      DummyPageHolder::Create(IntSize(500, 500));
  dummy_page_holder->GetDocument()
      .GetSettings()
      ->SetCSSExternalScannerNoPreload(true);

  CSSMockHTMLResourcePreloader* preloader =
      new CSSMockHTMLResourcePreloader(dummy_page_holder->GetDocument());

  KURL url("http://127.0.0.1/foo.css");
  Platform::Current()->GetURLLoaderMockFactory()->RegisterURL(
      url, WrappedResourceResponse(ResourceResponse()), "");
  FetchParameters params{ResourceRequest(url)};
  PreloadRecordingCSSPreloaderResourceClient* resource_client =
      new PreloadRecordingCSSPreloaderResourceClient(preloader);
  Resource* resource = CSSStyleSheetResource::Fetch(
      params, dummy_page_holder->GetDocument().Fetcher(), resource_client);

  dummy_page_holder->GetDocument().Shutdown();

  const char* data = "@import url('http://127.0.0.1/preload.css');";
  resource->AppendData(data, strlen(data));

  // Do not expect to gather any preloads, as the document loader is invalid,
  // which means we can't notify WebLoadingBehaviorData of the preloads.
  EXPECT_EQ(0u, resource_client->preload_urls_.size());
  Platform::Current()->GetURLLoaderMockFactory()->UnregisterURL(url);
}

TEST_F(CSSPreloadScannerTest, ReferrerPolicyHeader) {
  std::unique_ptr<DummyPageHolder> dummy_page_holder =
      DummyPageHolder::Create(IntSize(500, 500));
  dummy_page_holder->GetDocument().GetSettings()->SetCSSExternalScannerPreload(
      true);

  CSSMockHTMLResourcePreloader* preloader = new CSSMockHTMLResourcePreloader(
      dummy_page_holder->GetDocument(), "http://127.0.0.1/foo.css");

  KURL url("http://127.0.0.1/foo.css");
  FetchParameters params{ResourceRequest(url)};
  Platform::Current()->GetURLLoaderMockFactory()->RegisterURL(
      url, WrappedResourceResponse(ResourceResponse()), "");

  ResourceResponse response(url);
  response.SetHTTPStatusCode(200);
  response.SetHTTPHeaderField("referrer-policy", "unsafe-url");

  PreloadRecordingCSSPreloaderResourceClient* resource_client =
      new PreloadRecordingCSSPreloaderResourceClient(preloader);
  Resource* resource = CSSStyleSheetResource::Fetch(
      params, dummy_page_holder->GetDocument().Fetcher(), resource_client);
  resource->SetResponse(response);

  KURL preload_url("http://127.0.0.1/preload.css");
  Platform::Current()->GetURLLoaderMockFactory()->RegisterURL(
      preload_url, WrappedResourceResponse(ResourceResponse()), "");

  const char* data = "@import url('http://127.0.0.1/preload.css');";
  resource->AppendData(data, strlen(data));

  EXPECT_EQ(1u, resource_client->preload_urls_.size());
  EXPECT_EQ("http://127.0.0.1/preload.css",
            resource_client->preload_urls_.front());
  EXPECT_EQ(kReferrerPolicyAlways,
            resource_client->preload_referrer_policies_.front());
  Platform::Current()->GetURLLoaderMockFactory()->UnregisterURL(url);
  Platform::Current()->GetURLLoaderMockFactory()->UnregisterURL(preload_url);
}

}  // namespace blink
