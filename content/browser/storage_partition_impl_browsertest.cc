// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/storage_partition_impl.h"

#include <string>

#include "base/test/scoped_feature_list.h"
#include "build/build_config.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/web_contents.h"
#include "content/public/test/content_browser_test.h"
#include "content/shell/browser/shell.h"
#include "content/shell/browser/shell_browser_context.h"
#include "content/test/storage_partition_test_utils.h"
#include "mojo/public/cpp/bindings/interface_request.h"
#include "net/http/http_response_headers.h"
#include "net/test/embedded_test_server/embedded_test_server.h"
#include "net/traffic_annotation/network_traffic_annotation_test_helper.h"
#include "services/network/public/cpp/features.h"
#include "services/network/public/cpp/resource_response_info.h"
#include "services/network/public/mojom/network_service.mojom.h"
#include "services/network/public/mojom/url_loader.mojom.h"
#include "services/network/public/mojom/url_loader_factory.mojom.h"
#include "services/network/test/test_url_loader_client.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

namespace content {

enum class NetworkServiceState {
  kDisabled,
  kEnabled,
};

class StoragePartititionImplBrowsertest
    : public ContentBrowserTest,
      public testing::WithParamInterface<NetworkServiceState> {
 public:
  StoragePartititionImplBrowsertest() {
    if (GetParam() == NetworkServiceState::kEnabled)
      feature_list_.InitAndEnableFeature(network::features::kNetworkService);
  }
  ~StoragePartititionImplBrowsertest() override {}

  GURL GetTestURL() const {
    // Use '/echoheader' instead of '/echo' to avoid a disk_cache bug.
    // See https://crbug.com/792255.
    return embedded_test_server()->GetURL("/echoheader");
  }

 private:
  base::test::ScopedFeatureList feature_list_;
};

// Make sure that the NetworkContext returned by a StoragePartition works, both
// with the network service enabled and with it disabled, when one is created
// that wraps the URLRequestContext created by the BrowserContext.
IN_PROC_BROWSER_TEST_P(StoragePartititionImplBrowsertest, NetworkContext) {
  ASSERT_TRUE(embedded_test_server()->Start());

  network::mojom::URLLoaderFactoryParamsPtr params =
      network::mojom::URLLoaderFactoryParams::New();
  params->process_id = network::mojom::kBrowserProcessId;
  params->is_corb_enabled = false;
  network::mojom::URLLoaderFactoryPtr loader_factory;
  BrowserContext::GetDefaultStoragePartition(
      shell()->web_contents()->GetBrowserContext())
      ->GetNetworkContext()
      ->CreateURLLoaderFactory(mojo::MakeRequest(&loader_factory),
                               std::move(params));

  network::ResourceRequest request;
  network::TestURLLoaderClient client;
  request.url = embedded_test_server()->GetURL("/set-header?foo: bar");
  request.method = "GET";
  network::mojom::URLLoaderPtr loader;
  loader_factory->CreateLoaderAndStart(
      mojo::MakeRequest(&loader), 2, 1, network::mojom::kURLLoadOptionNone,
      request, client.CreateInterfacePtr(),
      net::MutableNetworkTrafficAnnotationTag(TRAFFIC_ANNOTATION_FOR_TESTS));

  // Just wait until headers are received - if the right headers are received,
  // no need to read the body.
  client.RunUntilResponseBodyArrived();
  ASSERT_TRUE(client.response_head().headers);
  EXPECT_EQ(200, client.response_head().headers->response_code());

  std::string foo_header_value;
  ASSERT_TRUE(client.response_head().headers->GetNormalizedHeader(
      "foo", &foo_header_value));
  EXPECT_EQ("bar", foo_header_value);
}

// Make sure the factory info returned from
// |StoragePartition::GetURLLoaderFactoryForBrowserProcessIOThread()| works.
IN_PROC_BROWSER_TEST_P(StoragePartititionImplBrowsertest,
                       GetURLLoaderFactoryForBrowserProcessIOThread) {
  ASSERT_TRUE(embedded_test_server()->Start());

  base::ScopedAllowBlockingForTesting allow_blocking;
  auto shared_url_loader_factory_info =
      BrowserContext::GetDefaultStoragePartition(
          shell()->web_contents()->GetBrowserContext())
          ->GetURLLoaderFactoryForBrowserProcessIOThread();

  auto factory_owner = IOThreadSharedURLLoaderFactoryOwner::Create(
      std::move(shared_url_loader_factory_info));

  EXPECT_EQ(net::OK, factory_owner->LoadBasicRequestOnIOThread(GetTestURL()));
}

// Make sure the factory info returned from
// |StoragePartition::GetURLLoaderFactoryForBrowserProcessIOThread()| doesn't
// crash if it's called after the StoragePartition is deleted.
IN_PROC_BROWSER_TEST_P(StoragePartititionImplBrowsertest,
                       BrowserIOFactoryInfoAfterStoragePartitionGone) {
  ASSERT_TRUE(embedded_test_server()->Start());

  base::ScopedAllowBlockingForTesting allow_blocking;
  std::unique_ptr<ShellBrowserContext> browser_context =
      std::make_unique<ShellBrowserContext>(true, nullptr);
  auto* partition =
      BrowserContext::GetDefaultStoragePartition(browser_context.get());
  auto shared_url_loader_factory_info =
      partition->GetURLLoaderFactoryForBrowserProcessIOThread();

  browser_context.reset();

  auto factory_owner = IOThreadSharedURLLoaderFactoryOwner::Create(
      std::move(shared_url_loader_factory_info));

  EXPECT_EQ(net::ERR_FAILED,
            factory_owner->LoadBasicRequestOnIOThread(GetTestURL()));
}

// Make sure the factory constructed from
// |StoragePartition::GetURLLoaderFactoryForBrowserProcessIOThread()| doesn't
// crash if it's called after the StoragePartition is deleted.
IN_PROC_BROWSER_TEST_P(StoragePartititionImplBrowsertest,
                       BrowserIOFactoryAfterStoragePartitionGone) {
  ASSERT_TRUE(embedded_test_server()->Start());

  base::ScopedAllowBlockingForTesting allow_blocking;
  std::unique_ptr<ShellBrowserContext> browser_context =
      std::make_unique<ShellBrowserContext>(true, nullptr);
  auto* partition =
      BrowserContext::GetDefaultStoragePartition(browser_context.get());
  auto factory_owner = IOThreadSharedURLLoaderFactoryOwner::Create(
      partition->GetURLLoaderFactoryForBrowserProcessIOThread());

  EXPECT_EQ(net::OK, factory_owner->LoadBasicRequestOnIOThread(GetTestURL()));

  browser_context.reset();

  EXPECT_EQ(net::ERR_FAILED,
            factory_owner->LoadBasicRequestOnIOThread(GetTestURL()));
}

// NetworkServiceState::kEnabled currently DCHECKs on Android, as Android isn't
// expected to create extra processes.
#if defined(OS_ANDROID)
INSTANTIATE_TEST_CASE_P(
    /* No test prefix */,
    StoragePartititionImplBrowsertest,
    ::testing::Values(NetworkServiceState::kDisabled));
#else  // !defined(OS_ANDROID)
INSTANTIATE_TEST_CASE_P(
    /* No test prefix */,
    StoragePartititionImplBrowsertest,
    ::testing::Values(NetworkServiceState::kDisabled,
                      NetworkServiceState::kEnabled));
#endif

}  // namespace content
