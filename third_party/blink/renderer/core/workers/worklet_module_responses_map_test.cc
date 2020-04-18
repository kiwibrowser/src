// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/workers/worklet_module_responses_map.h"
#include "base/optional.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/blink/public/platform/web_url_loader_mock_factory.h"
#include "third_party/blink/renderer/core/loader/modulescript/module_script_creation_params.h"
#include "third_party/blink/renderer/core/workers/worker_fetch_test_helper.h"
#include "third_party/blink/renderer/platform/loader/testing/fetch_testing_platform_support.h"
#include "third_party/blink/renderer/platform/loader/testing/mock_fetch_context.h"
#include "third_party/blink/renderer/platform/testing/testing_platform_support.h"
#include "third_party/blink/renderer/platform/testing/unit_test_helpers.h"
#include "third_party/blink/renderer/platform/testing/url_test_helpers.h"
#include "third_party/blink/renderer/platform/weborigin/kurl.h"
#include "third_party/blink/renderer/platform/wtf/functional.h"

namespace blink {

class WorkletModuleResponsesMapTest : public testing::Test {
 public:
  WorkletModuleResponsesMapTest() = default;

  void SetUp() override {
    platform_->AdvanceClockSeconds(1.);  // For non-zero DocumentParserTimings
    auto* context =
        MockFetchContext::Create(MockFetchContext::kShouldLoadNewResource);
    fetcher_ = ResourceFetcher::Create(context);
    map_ = new WorkletModuleResponsesMap(fetcher_);
  }

  void Fetch(const KURL& url, ClientImpl* client) {
    ResourceRequest resource_request(url);
    // TODO(nhiroki): Specify worklet-specific request context (e.g.,
    // "paintworklet").
    resource_request.SetRequestContext(WebURLRequest::kRequestContextScript);
    FetchParameters fetch_params(resource_request);
    map_->Fetch(fetch_params, client);
  }

 protected:
  ScopedTestingPlatformSupport<FetchTestingPlatformSupport> platform_;
  Persistent<ResourceFetcher> fetcher_;
  Persistent<WorkletModuleResponsesMap> map_;
};

TEST_F(WorkletModuleResponsesMapTest, Basic) {
  const KURL kUrl("https://example.com/module.js");
  URLTestHelpers::RegisterMockedURLLoad(
      kUrl, test::CoreTestDataPath("module.js"), "text/javascript");
  HeapVector<Member<ClientImpl>> clients;

  // An initial read call initiates a fetch request.
  clients.push_back(new ClientImpl);
  Fetch(kUrl, clients[0]);
  EXPECT_EQ(ClientImpl::Result::kInitial, clients[0]->GetResult());
  EXPECT_FALSE(clients[0]->GetParams().has_value());

  // The entry is now being fetched. Following read calls should wait for the
  // completion.
  clients.push_back(new ClientImpl);
  Fetch(kUrl, clients[1]);
  EXPECT_EQ(ClientImpl::Result::kInitial, clients[1]->GetResult());

  clients.push_back(new ClientImpl);
  Fetch(kUrl, clients[2]);
  EXPECT_EQ(ClientImpl::Result::kInitial, clients[2]->GetResult());

  // Serve the fetch request. This should notify the waiting clients.
  platform_->GetURLLoaderMockFactory()->ServeAsynchronousRequests();
  for (auto client : clients) {
    EXPECT_EQ(ClientImpl::Result::kOK, client->GetResult());
    EXPECT_TRUE(client->GetParams().has_value());
  }
}

TEST_F(WorkletModuleResponsesMapTest, Failure) {
  const KURL kUrl("https://example.com/module.js");
  URLTestHelpers::RegisterMockedErrorURLLoad(kUrl);
  HeapVector<Member<ClientImpl>> clients;

  // An initial read call initiates a fetch request.
  clients.push_back(new ClientImpl);
  Fetch(kUrl, clients[0]);
  EXPECT_EQ(ClientImpl::Result::kInitial, clients[0]->GetResult());
  EXPECT_FALSE(clients[0]->GetParams().has_value());

  // The entry is now being fetched. Following read calls should wait for the
  // completion.
  clients.push_back(new ClientImpl);
  Fetch(kUrl, clients[1]);
  EXPECT_EQ(ClientImpl::Result::kInitial, clients[1]->GetResult());

  clients.push_back(new ClientImpl);
  Fetch(kUrl, clients[2]);
  EXPECT_EQ(ClientImpl::Result::kInitial, clients[2]->GetResult());

  // Serve the fetch request with 404. This should fail the waiting clients.
  platform_->GetURLLoaderMockFactory()->ServeAsynchronousRequests();
  for (auto client : clients) {
    EXPECT_EQ(ClientImpl::Result::kFailed, client->GetResult());
    EXPECT_FALSE(client->GetParams().has_value());
  }
}

TEST_F(WorkletModuleResponsesMapTest, Isolation) {
  const KURL kUrl1("https://example.com/module?1.js");
  const KURL kUrl2("https://example.com/module?2.js");
  URLTestHelpers::RegisterMockedErrorURLLoad(kUrl1);
  URLTestHelpers::RegisterMockedURLLoad(
      kUrl2, test::CoreTestDataPath("module.js"), "text/javascript");
  HeapVector<Member<ClientImpl>> clients;

  // An initial read call for |kUrl1| initiates a fetch request.
  clients.push_back(new ClientImpl);
  Fetch(kUrl1, clients[0]);
  EXPECT_EQ(ClientImpl::Result::kInitial, clients[0]->GetResult());
  EXPECT_FALSE(clients[0]->GetParams().has_value());

  // The entry is now being fetched. Following read calls for |kUrl1| should
  // wait for the completion.
  clients.push_back(new ClientImpl);
  Fetch(kUrl1, clients[1]);
  EXPECT_EQ(ClientImpl::Result::kInitial, clients[1]->GetResult());

  // An initial read call for |kUrl2| initiates a fetch request.
  clients.push_back(new ClientImpl);
  Fetch(kUrl2, clients[2]);
  EXPECT_EQ(ClientImpl::Result::kInitial, clients[2]->GetResult());
  EXPECT_FALSE(clients[2]->GetParams().has_value());

  // The entry is now being fetched. Following read calls for |kUrl2| should
  // wait for the completion.
  clients.push_back(new ClientImpl);
  Fetch(kUrl2, clients[3]);
  EXPECT_EQ(ClientImpl::Result::kInitial, clients[3]->GetResult());

  // The read call for |kUrl2| should not affect the other entry for |kUrl1|.
  EXPECT_EQ(ClientImpl::Result::kInitial, clients[0]->GetResult());

  // Serve the fetch requests.
  platform_->GetURLLoaderMockFactory()->ServeAsynchronousRequests();
  EXPECT_EQ(ClientImpl::Result::kFailed, clients[0]->GetResult());
  EXPECT_FALSE(clients[0]->GetParams().has_value());
  EXPECT_EQ(ClientImpl::Result::kFailed, clients[1]->GetResult());
  EXPECT_FALSE(clients[1]->GetParams().has_value());
  EXPECT_EQ(ClientImpl::Result::kOK, clients[2]->GetResult());
  EXPECT_TRUE(clients[2]->GetParams().has_value());
  EXPECT_EQ(ClientImpl::Result::kOK, clients[3]->GetResult());
  EXPECT_TRUE(clients[3]->GetParams().has_value());
}

TEST_F(WorkletModuleResponsesMapTest, InvalidURL) {
  const KURL kEmptyURL;
  ASSERT_TRUE(kEmptyURL.IsEmpty());
  ClientImpl* client1 = new ClientImpl;
  Fetch(kEmptyURL, client1);
  EXPECT_EQ(ClientImpl::Result::kFailed, client1->GetResult());
  EXPECT_FALSE(client1->GetParams().has_value());

  const KURL kNullURL = NullURL();
  ASSERT_TRUE(kNullURL.IsNull());
  ClientImpl* client2 = new ClientImpl;
  Fetch(kNullURL, client2);
  EXPECT_EQ(ClientImpl::Result::kFailed, client2->GetResult());
  EXPECT_FALSE(client2->GetParams().has_value());

  const KURL kInvalidURL;
  ASSERT_FALSE(kInvalidURL.IsValid());
  ClientImpl* client3 = new ClientImpl;
  Fetch(kInvalidURL, client3);
  EXPECT_EQ(ClientImpl::Result::kFailed, client3->GetResult());
  EXPECT_FALSE(client3->GetParams().has_value());
}

TEST_F(WorkletModuleResponsesMapTest, Dispose) {
  const KURL kUrl1("https://example.com/module?1.js");
  const KURL kUrl2("https://example.com/module?2.js");
  URLTestHelpers::RegisterMockedURLLoad(
      kUrl1, test::CoreTestDataPath("module.js"), "text/javascript");
  URLTestHelpers::RegisterMockedURLLoad(
      kUrl2, test::CoreTestDataPath("module.js"), "text/javascript");
  HeapVector<Member<ClientImpl>> clients;

  // An initial read call for |kUrl1| creates a placeholder entry and asks the
  // client to fetch a module script.
  clients.push_back(new ClientImpl);
  Fetch(kUrl1, clients[0]);
  EXPECT_EQ(ClientImpl::Result::kInitial, clients[0]->GetResult());
  EXPECT_FALSE(clients[0]->GetParams().has_value());

  // The entry is now being fetched. Following read calls for |kUrl1| should
  // wait for the completion.
  clients.push_back(new ClientImpl);
  Fetch(kUrl1, clients[1]);
  EXPECT_EQ(ClientImpl::Result::kInitial, clients[1]->GetResult());

  // An initial read call for |kUrl2| also creates a placeholder entry and asks
  // the client to fetch a module script.
  clients.push_back(new ClientImpl);
  Fetch(kUrl2, clients[2]);
  EXPECT_EQ(ClientImpl::Result::kInitial, clients[2]->GetResult());
  EXPECT_FALSE(clients[2]->GetParams().has_value());

  // The entry is now being fetched. Following read calls for |kUrl2| should
  // wait for the completion.
  clients.push_back(new ClientImpl);
  Fetch(kUrl2, clients[3]);
  EXPECT_EQ(ClientImpl::Result::kInitial, clients[3]->GetResult());

  // Dispose() should notify to all waiting clients.
  map_->Dispose();
  for (auto client : clients) {
    EXPECT_EQ(ClientImpl::Result::kFailed, client->GetResult());
    EXPECT_FALSE(client->GetParams().has_value());
  }
}

}  // namespace blink
