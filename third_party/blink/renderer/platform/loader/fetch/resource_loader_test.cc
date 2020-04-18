// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/platform/loader/fetch/raw_resource.h"
#include "third_party/blink/renderer/platform/loader/fetch/resource_fetcher.h"
#include "third_party/blink/renderer/platform/loader/fetch/resource_load_scheduler.h"
#include "third_party/blink/renderer/platform/loader/fetch/resource_loader.h"
#include "third_party/blink/renderer/platform/loader/fetch/resource_response.h"
#include "third_party/blink/renderer/platform/loader/testing/mock_fetch_context.h"
#include "third_party/blink/renderer/platform/testing/testing_platform_support_with_mock_scheduler.h"
#include "third_party/blink/renderer/platform/wtf/text/string_builder.h"

#include "testing/gtest/include/gtest/gtest.h"

namespace blink {

class ResourceLoaderTest : public testing::Test {
  DISALLOW_COPY_AND_ASSIGN(ResourceLoaderTest);

 public:
  ResourceLoaderTest()
      : foo_url_("https://foo.test"), bar_url_("https://bar.test"){};

  void SetUp() override {
    context_ =
        MockFetchContext::Create(MockFetchContext::kShouldLoadNewResource);
  }

 protected:
  enum ServiceWorkerMode { kNoSW, kSWOpaque, kSWClear };

  struct TestCase {
    const KURL& origin;
    const KURL& target;
    const KURL* allow_origin_url;
    const ServiceWorkerMode service_worker;
    const Resource::Type resource_type;
    const CORSStatus expectation;
  };

  Persistent<MockFetchContext> context_;

  ScopedTestingPlatformSupport<TestingPlatformSupportWithMockScheduler>
      platform_;

  const KURL foo_url_;
  const KURL bar_url_;
};

TEST_F(ResourceLoaderTest, DetermineCORSStatus) {
  TestCase cases[] = {
      // No CORS status for main resources:
      {foo_url_, foo_url_, nullptr, kNoSW, Resource::Type::kMainResource,
       CORSStatus::kNotApplicable},

      // Same origin:
      {foo_url_, foo_url_, nullptr, kNoSW, Resource::Type::kRaw,
       CORSStatus::kSameOrigin},

      // Cross origin CORS successful:
      {foo_url_, bar_url_, &foo_url_, kNoSW, Resource::Type::kRaw,
       CORSStatus::kSuccessful},

      // Cross origin not in CORS mode:
      {foo_url_, bar_url_, nullptr, kNoSW, Resource::Type::kRaw,
       CORSStatus::kNotApplicable},

      // Cross origin CORS failed:
      {foo_url_, bar_url_, &bar_url_, kNoSW, Resource::Type::kRaw,
       CORSStatus::kFailed},

      // CORS handled by service worker
      {foo_url_, foo_url_, nullptr, kSWClear, Resource::Type::kRaw,
       CORSStatus::kServiceWorkerSuccessful},
      {foo_url_, foo_url_, &foo_url_, kSWClear, Resource::Type::kRaw,
       CORSStatus::kServiceWorkerSuccessful},
      {foo_url_, bar_url_, nullptr, kSWClear, Resource::Type::kRaw,
       CORSStatus::kServiceWorkerSuccessful},
      {foo_url_, bar_url_, &foo_url_, kSWClear, Resource::Type::kRaw,
       CORSStatus::kServiceWorkerSuccessful},

      // Opaque response by service worker
      {foo_url_, foo_url_, nullptr, kSWOpaque, Resource::Type::kRaw,
       CORSStatus::kServiceWorkerOpaque},
      {foo_url_, bar_url_, nullptr, kSWOpaque, Resource::Type::kRaw,
       CORSStatus::kServiceWorkerOpaque},
      {foo_url_, bar_url_, &foo_url_, kSWOpaque, Resource::Type::kRaw,
       CORSStatus::kServiceWorkerOpaque},
  };

  ResourceLoadScheduler* scheduler = ResourceLoadScheduler::Create();

  for (const auto& test : cases) {
    SCOPED_TRACE(testing::Message()
                 << "Origin: " << test.origin.GetString()
                 << ", target: " << test.target.GetString()
                 << ", CORS access-control-allow-origin header: "
                 << (test.allow_origin_url ? test.allow_origin_url->GetString()
                                           : "-")
                 << ", service worker: "
                 << (test.service_worker == kNoSW
                         ? "no"
                         : (test.service_worker == kSWClear
                                ? "clear response"
                                : "opaque response"))
                 << ", expected CORSStatus == "
                 << static_cast<unsigned>(test.expectation));

    context_->SetSecurityOrigin(SecurityOrigin::Create(test.origin));
    ResourceFetcher* fetcher = ResourceFetcher::Create(context_);

    Resource* resource =
        RawResource::CreateForTest(test.target, test.resource_type);
    ResourceLoader* loader =
        ResourceLoader::Create(fetcher, scheduler, resource);

    ResourceRequest request;
    request.SetURL(test.target);

    ResourceResponse response(test.target);
    response.SetHTTPStatusCode(200);

    if (test.allow_origin_url) {
      request.SetFetchRequestMode(network::mojom::FetchRequestMode::kCORS);
      resource->MutableOptions().cors_handling_by_resource_fetcher =
          kEnableCORSHandlingByResourceFetcher;
      response.SetHTTPHeaderField(
          "access-control-allow-origin",
          SecurityOrigin::Create(*test.allow_origin_url)->ToAtomicString());
      response.SetHTTPHeaderField("access-control-allow-credentials", "true");
    }

    resource->SetResourceRequest(request);

    if (test.service_worker != kNoSW) {
      response.SetWasFetchedViaServiceWorker(true);

      if (test.service_worker == kSWOpaque) {
        response.SetResponseTypeViaServiceWorker(
            network::mojom::FetchResponseType::kOpaque);
      } else {
        response.SetResponseTypeViaServiceWorker(
            network::mojom::FetchResponseType::kDefault);
      }
    }

    StringBuilder cors_error_msg;
    CORSStatus cors_status =
        loader->DetermineCORSStatus(response, cors_error_msg);

    EXPECT_EQ(cors_status, test.expectation);
  }
}
}  // namespace blink
