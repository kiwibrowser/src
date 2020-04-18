// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/html/parser/html_resource_preloader.h"

#include <memory>
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/blink/renderer/core/html/parser/preload_request.h"
#include "third_party/blink/renderer/core/testing/page_test_base.h"

namespace blink {

struct HTMLResourcePreconnectTestCase {
  const char* base_url;
  const char* url;
  bool is_cors;
  bool is_https;
};

class PreloaderNetworkHintsMock : public NetworkHintsInterface {
 public:
  PreloaderNetworkHintsMock() : did_preconnect_(false) {}

  void DnsPrefetchHost(const String& host) const override {}
  void PreconnectHost(
      const KURL& host,
      const CrossOriginAttributeValue cross_origin) const override {
    did_preconnect_ = true;
    is_https_ = host.ProtocolIs("https");
    is_cross_origin_ = (cross_origin == kCrossOriginAttributeAnonymous);
  }

  bool DidPreconnect() { return did_preconnect_; }
  bool IsHTTPS() { return is_https_; }
  bool IsCrossOrigin() { return is_cross_origin_; }

 private:
  mutable bool did_preconnect_;
  mutable bool is_https_;
  mutable bool is_cross_origin_;
};

class HTMLResourcePreloaderTest : public PageTestBase {
 protected:
  void SetUp() override { PageTestBase::SetUp(IntSize()); }

  void Test(HTMLResourcePreconnectTestCase test_case) {
    // TODO(yoav): Need a mock loader here to verify things are happenning
    // beyond preconnect.
    PreloaderNetworkHintsMock network_hints;
    auto preload_request = PreloadRequest::CreateIfNeeded(
        String(), TextPosition(), test_case.url, KURL(test_case.base_url),
        Resource::kImage, ReferrerPolicy(), PreloadRequest::kDocumentIsReferrer,
        ResourceFetcher::kImageNotImageSet, FetchParameters::ResourceWidth(),
        ClientHintsPreferences(), PreloadRequest::kRequestTypePreconnect);
    DCHECK(preload_request);
    if (test_case.is_cors)
      preload_request->SetCrossOrigin(kCrossOriginAttributeAnonymous);
    HTMLResourcePreloader* preloader =
        HTMLResourcePreloader::Create(GetDocument());
    preloader->Preload(std::move(preload_request), network_hints);
    ASSERT_TRUE(network_hints.DidPreconnect());
    ASSERT_EQ(test_case.is_cors, network_hints.IsCrossOrigin());
    ASSERT_EQ(test_case.is_https, network_hints.IsHTTPS());
  }
};

TEST_F(HTMLResourcePreloaderTest, testPreconnect) {
  HTMLResourcePreconnectTestCase test_cases[] = {
      {"http://example.test", "http://example.com", false, false},
      {"http://example.test", "http://example.com", true, false},
      {"http://example.test", "https://example.com", true, true},
      {"http://example.test", "https://example.com", false, true},
      {"http://example.test", "//example.com", false, false},
      {"http://example.test", "//example.com", true, false},
      {"https://example.test", "//example.com", false, true},
      {"https://example.test", "//example.com", true, true},
  };

  for (const auto& test_case : test_cases)
    Test(test_case);
}

}  // namespace blink
