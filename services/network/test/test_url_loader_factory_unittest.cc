// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/network/test/test_url_loader_factory.h"
#include "base/logging.h"
#include "base/test/scoped_task_environment.h"
#include "mojo/public/cpp/system/data_pipe_utils.h"
#include "net/traffic_annotation/network_traffic_annotation_test_helper.h"
#include "services/network/test/test_url_loader_client.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace network {

class TestURLLoaderFactoryTest : public testing::Test {
 public:
  TestURLLoaderFactoryTest() {}
  ~TestURLLoaderFactoryTest() override {}

  void SetUp() override {}

  void StartRequest(const std::string& url) { StartRequest(url, &client_); }

  void StartRequest(const std::string& url,
                    TestURLLoaderClient* client,
                    int load_flags = 0) {
    ResourceRequest request;
    request.url = GURL(url);
    request.load_flags = load_flags;
    factory_.CreateLoaderAndStart(
        mojo::MakeRequest(&loader_), 0, 0, 0, request,
        client->CreateInterfacePtr(),
        net::MutableNetworkTrafficAnnotationTag(TRAFFIC_ANNOTATION_FOR_TESTS));
  }

  std::string GetData(TestURLLoaderClient* client) {
    std::string response;
    EXPECT_TRUE(client->response_body().is_valid());
    EXPECT_TRUE(
        mojo::BlockingCopyToString(client->response_body_release(), &response));
    return response;
  }

  TestURLLoaderFactory* factory() { return &factory_; }
  TestURLLoaderClient* client() { return &client_; }

 private:
  base::test::ScopedTaskEnvironment scoped_task_environment_;
  TestURLLoaderFactory factory_;
  mojom::URLLoaderPtr loader_;
  TestURLLoaderClient client_;
};

TEST_F(TestURLLoaderFactoryTest, Simple) {
  std::string url = "http://foo";
  std::string data = "bar";

  factory()->AddResponse(url, data);

  StartRequest(url);
  client()->RunUntilComplete();
  EXPECT_EQ(GetData(client()), data);

  // Data can be fetched multiple times.
  mojom::URLLoaderPtr loader2;
  TestURLLoaderClient client2;
  StartRequest(url, &client2);
  client2.RunUntilComplete();
  EXPECT_EQ(GetData(&client2), data);
}

TEST_F(TestURLLoaderFactoryTest, AddResponse404) {
  std::string url = "http://foo";
  std::string body = "Sad robot";
  factory()->AddResponse(url, body, net::HTTP_NOT_FOUND);

  StartRequest(url);
  client()->RunUntilComplete();
  ASSERT_TRUE(client()->response_head().headers != nullptr);
  EXPECT_EQ(net::HTTP_NOT_FOUND,
            client()->response_head().headers->response_code());
  EXPECT_EQ(GetData(client()), body);
}

TEST_F(TestURLLoaderFactoryTest, MultipleSameURL) {
  std::string url = "http://foo";
  std::string data1 = "bar1";
  std::string data2 = "bar2";

  factory()->AddResponse(url, data1);
  factory()->AddResponse(url, data2);

  StartRequest(url);
  client()->RunUntilComplete();
  EXPECT_EQ(GetData(client()), data2);
}

TEST_F(TestURLLoaderFactoryTest, MultipleSameURL2) {
  // Tests for two requests to same URL happening before AddResponse.
  std::string url = "http://foo";
  std::string data = "bar1";

  StartRequest(url);
  TestURLLoaderClient client2;
  StartRequest(url, &client2);

  factory()->AddResponse(url, data);

  client()->RunUntilComplete();
  client2.RunUntilComplete();
  EXPECT_EQ(GetData(client()), data);
  EXPECT_EQ(GetData(&client2), data);
}

TEST_F(TestURLLoaderFactoryTest, Redirects) {
  GURL url("http://example.test/");

  net::RedirectInfo redirect_info;
  redirect_info.status_code = 301;
  redirect_info.new_url = GURL("http://example2.test/");
  network::TestURLLoaderFactory::Redirects redirects{
      {redirect_info, network::ResourceResponseHead()}};
  URLLoaderCompletionStatus status;
  std::string content = "foo";
  status.decoded_body_length = content.size();
  factory()->AddResponse(url, network::ResourceResponseHead(), content, status,
                         redirects);
  StartRequest(url.spec());
  client()->RunUntilComplete();

  EXPECT_EQ(GetData(client()), content);
  EXPECT_TRUE(client()->has_received_redirect());
  EXPECT_EQ(redirect_info.new_url, client()->redirect_info().new_url);
}

TEST_F(TestURLLoaderFactoryTest, IsPending) {
  std::string url = "http://foo/";

  // Normal lifecycle.
  EXPECT_FALSE(factory()->IsPending(url));
  StartRequest(url);
  EXPECT_TRUE(factory()->IsPending(url));
  factory()->AddResponse(url, "hi");
  client()->RunUntilComplete();
  EXPECT_FALSE(factory()->IsPending(url));

  // Cleanup between tests.
  client()->Unbind();
  factory()->ClearResponses();

  // Now with cancellation.
  StartRequest(url);
  EXPECT_TRUE(factory()->IsPending(url));
  client()->Unbind();
  EXPECT_FALSE(factory()->IsPending(url));
}

TEST_F(TestURLLoaderFactoryTest, IsPendingLoadFlags) {
  std::string url = "http://foo/";
  std::string url2 = "http://bar/";

  int load_flags_out = 0;
  StartRequest(url);
  EXPECT_TRUE(factory()->IsPending(url, &load_flags_out));
  EXPECT_EQ(0, load_flags_out);

  factory()->AddResponse(url, "hi");
  client()->RunUntilComplete();

  TestURLLoaderClient client2;
  StartRequest(url2, &client2, 42);
  EXPECT_TRUE(factory()->IsPending(url2, &load_flags_out));
  EXPECT_EQ(42, load_flags_out);
  factory()->AddResponse(url2, "bye");
  client2.RunUntilComplete();
}

TEST_F(TestURLLoaderFactoryTest, NumPending) {
  std::string url = "http://foo/";
  std::string url2 = "http://bar/";

  EXPECT_EQ(0, factory()->NumPending());
  StartRequest(url);
  EXPECT_EQ(1, factory()->NumPending());
  client()->Unbind();
  // All cancelled.
  EXPECT_EQ(0, factory()->NumPending());

  TestURLLoaderClient client2;
  StartRequest(url2, &client2);
  EXPECT_EQ(1, factory()->NumPending());
  factory()->AddResponse(url2, "hello");
  client2.RunUntilComplete();
  EXPECT_EQ(0, factory()->NumPending());
}

TEST_F(TestURLLoaderFactoryTest, NumPending2) {
  std::string url = "http://foo/";

  EXPECT_EQ(0, factory()->NumPending());
  StartRequest(url);
  EXPECT_EQ(1, factory()->NumPending());

  TestURLLoaderClient client2;
  StartRequest(url, &client2);
  EXPECT_EQ(2, factory()->NumPending());
  factory()->AddResponse(url, "hello");
  client2.RunUntilComplete();
  EXPECT_EQ(0, factory()->NumPending());
}

}  // namespace network
