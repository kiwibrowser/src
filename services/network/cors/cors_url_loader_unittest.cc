// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/network/cors/cors_url_loader.h"

#include "base/logging.h"
#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "base/message_loop/message_loop.h"
#include "base/run_loop.h"
#include "base/test/scoped_feature_list.h"
#include "net/http/http_request_headers.h"
#include "net/traffic_annotation/network_traffic_annotation_test_helper.h"
#include "services/network/cors/cors_url_loader_factory.h"
#include "services/network/public/cpp/features.h"
#include "services/network/public/mojom/url_loader.mojom.h"
#include "services/network/public/mojom/url_loader_factory.mojom.h"
#include "services/network/test/test_url_loader_client.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace network {

namespace cors {

namespace {

class TestURLLoaderFactory : public mojom::URLLoaderFactory {
 public:
  TestURLLoaderFactory() : weak_factory_(this) {}
  ~TestURLLoaderFactory() override = default;

  base::WeakPtr<TestURLLoaderFactory> GetWeakPtr() {
    return weak_factory_.GetWeakPtr();
  }

  void NotifyClientOnReceiveResponse(const std::string& extra_header) {
    DCHECK(client_ptr_);
    ResourceResponseHead response;
    response.headers = new net::HttpResponseHeaders(
        "HTTP/1.1 200 OK\n"
        "Content-Type: image/png\n");
    if (!extra_header.empty())
      response.headers->AddHeader(extra_header);

    client_ptr_->OnReceiveResponse(response, nullptr /* downloaded_file */);
  }

  void NotifyClientOnComplete(int error_code) {
    DCHECK(client_ptr_);
    client_ptr_->OnComplete(URLLoaderCompletionStatus(error_code));
  }

  bool IsCreateLoaderAndStartCalled() { return !!client_ptr_; }

  const GURL& GetRequestedURL() const { return requested_url_; }

 private:
  // mojom::URLLoaderFactory implementation.
  void CreateLoaderAndStart(mojom::URLLoaderRequest request,
                            int32_t routing_id,
                            int32_t request_id,
                            uint32_t options,
                            const ResourceRequest& resource_request,
                            mojom::URLLoaderClientPtr client,
                            const net::MutableNetworkTrafficAnnotationTag&
                                traffic_annotation) override {
    DCHECK(client);
    requested_url_ = resource_request.url;
    client_ptr_ = std::move(client);
  }

  void Clone(mojom::URLLoaderFactoryRequest request) override { NOTREACHED(); }

  mojom::URLLoaderClientPtr client_ptr_;

  GURL requested_url_;

  base::WeakPtrFactory<TestURLLoaderFactory> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(TestURLLoaderFactory);
};

class CORSURLLoaderTest : public testing::Test {
 public:
  CORSURLLoaderTest() {
    std::unique_ptr<TestURLLoaderFactory> factory =
        std::make_unique<TestURLLoaderFactory>();
    test_url_loader_factory_ = factory->GetWeakPtr();
    cors_url_loader_factory_ =
        std::make_unique<CORSURLLoaderFactory>(std::move(factory));
  }

 protected:
  // testing::Test implementation.
  void SetUp() override {
    feature_list_.InitAndEnableFeature(features::kOutOfBlinkCORS);
  }

  void CreateLoaderAndStart(const GURL& origin,
                            const GURL& url,
                            mojom::FetchRequestMode fetch_request_mode) {
    ResourceRequest request;
    request.fetch_request_mode = fetch_request_mode;
    request.method = net::HttpRequestHeaders::kGetMethod;
    request.url = url;
    request.request_initiator = url::Origin::Create(origin);

    cors_url_loader_factory_->CreateLoaderAndStart(
        mojo::MakeRequest(&url_loader_), 0 /* routing_id */, 0 /* request_id */,
        mojom::kURLLoadOptionNone, request,
        test_cors_loader_client_.CreateInterfacePtr(),
        net::MutableNetworkTrafficAnnotationTag(TRAFFIC_ANNOTATION_FOR_TESTS));
  }

  bool IsNetworkLoaderStarted() {
    DCHECK(test_url_loader_factory_);
    return test_url_loader_factory_->IsCreateLoaderAndStartCalled();
  }

  void NotifyLoaderClientOnReceiveResponse(
      const std::string& extra_header = std::string()) {
    DCHECK(test_url_loader_factory_);
    test_url_loader_factory_->NotifyClientOnReceiveResponse(extra_header);
  }

  void NotifyLoaderClientOnComplete(int error_code) {
    DCHECK(test_url_loader_factory_);
    test_url_loader_factory_->NotifyClientOnComplete(error_code);
  }

  const GURL& GetRequestedURL() {
    DCHECK(test_url_loader_factory_);
    return test_url_loader_factory_->GetRequestedURL();
  }

  const TestURLLoaderClient& client() const { return test_cors_loader_client_; }

  void RunUntilComplete() { test_cors_loader_client_.RunUntilComplete(); }

 private:
  // Be the first member so it is destroyed last.
  base::MessageLoop message_loop_;

  // Testing instance to enable kOutOfBlinkCORS feature.
  base::test::ScopedFeatureList feature_list_;

  // CORSURLLoaderFactory instance under tests.
  std::unique_ptr<mojom::URLLoaderFactory> cors_url_loader_factory_;

  // TestURLLoaderFactory instance owned by CORSURLLoaderFactory.
  base::WeakPtr<TestURLLoaderFactory> test_url_loader_factory_;

  // Holds URLLoaderPtr that CreateLoaderAndStart() creates.
  mojom::URLLoaderPtr url_loader_;

  // TestURLLoaderClient that records callback activities.
  TestURLLoaderClient test_cors_loader_client_;

  DISALLOW_COPY_AND_ASSIGN(CORSURLLoaderTest);
};

TEST_F(CORSURLLoaderTest, SameOriginRequest) {
  const GURL url("http://example.com/foo.png");
  CreateLoaderAndStart(url.GetOrigin(), url,
                       mojom::FetchRequestMode::kSameOrigin);

  NotifyLoaderClientOnReceiveResponse();
  NotifyLoaderClientOnComplete(net::OK);

  RunUntilComplete();

  EXPECT_TRUE(IsNetworkLoaderStarted());
  EXPECT_FALSE(client().has_received_redirect());
  EXPECT_TRUE(client().has_received_response());
  EXPECT_TRUE(client().has_received_completion());
  EXPECT_EQ(net::OK, client().completion_status().error_code);
}

TEST_F(CORSURLLoaderTest, CrossOriginRequestWithNoCORSMode) {
  const GURL origin("http://example.com");
  const GURL url("http://other.com/foo.png");
  CreateLoaderAndStart(origin, url, mojom::FetchRequestMode::kNoCORS);

  NotifyLoaderClientOnReceiveResponse();
  NotifyLoaderClientOnComplete(net::OK);

  RunUntilComplete();

  EXPECT_TRUE(IsNetworkLoaderStarted());
  EXPECT_FALSE(client().has_received_redirect());
  EXPECT_TRUE(client().has_received_response());
  EXPECT_TRUE(client().has_received_completion());
  EXPECT_EQ(net::OK, client().completion_status().error_code);
}

TEST_F(CORSURLLoaderTest, CrossOriginRequestFetchRequestModeSameOrigin) {
  const GURL origin("http://example.com");
  const GURL url("http://other.com/foo.png");
  CreateLoaderAndStart(origin, url, mojom::FetchRequestMode::kSameOrigin);

  RunUntilComplete();

  // This call never hits the network URLLoader (i.e. the TestURLLoaderFactory)
  // because it is fails right away.
  EXPECT_FALSE(IsNetworkLoaderStarted());
  EXPECT_FALSE(client().has_received_redirect());
  EXPECT_FALSE(client().has_received_response());
  EXPECT_EQ(net::ERR_FAILED, client().completion_status().error_code);
  ASSERT_TRUE(client().completion_status().cors_error_status);
  EXPECT_EQ(mojom::CORSError::kDisallowedByMode,
            client().completion_status().cors_error_status->cors_error);
}

TEST_F(CORSURLLoaderTest, CrossOriginRequestWithCORSModeButMissingCORSHeader) {
  const GURL origin("http://example.com");
  const GURL url("http://other.com/foo.png");
  CreateLoaderAndStart(origin, url, mojom::FetchRequestMode::kCORS);

  NotifyLoaderClientOnReceiveResponse();
  NotifyLoaderClientOnComplete(net::OK);

  RunUntilComplete();

  EXPECT_TRUE(IsNetworkLoaderStarted());
  EXPECT_FALSE(client().has_received_redirect());
  EXPECT_FALSE(client().has_received_response());
  EXPECT_EQ(net::ERR_FAILED, client().completion_status().error_code);
  ASSERT_TRUE(client().completion_status().cors_error_status);
  EXPECT_EQ(mojom::CORSError::kMissingAllowOriginHeader,
            client().completion_status().cors_error_status->cors_error);
}

TEST_F(CORSURLLoaderTest, CrossOriginRequestWithCORSMode) {
  const GURL origin("http://example.com");
  const GURL url("http://other.com/foo.png");
  CreateLoaderAndStart(origin, url, mojom::FetchRequestMode::kCORS);

  NotifyLoaderClientOnReceiveResponse(
      "Access-Control-Allow-Origin: http://example.com");
  NotifyLoaderClientOnComplete(net::OK);

  RunUntilComplete();

  EXPECT_TRUE(IsNetworkLoaderStarted());
  EXPECT_FALSE(client().has_received_redirect());
  EXPECT_TRUE(client().has_received_response());
  EXPECT_TRUE(client().has_received_completion());
  EXPECT_EQ(net::OK, client().completion_status().error_code);
}

TEST_F(CORSURLLoaderTest,
       CrossOriginRequestFetchRequestWithCORSModeButMismatchedCORSHeader) {
  const GURL origin("http://example.com");
  const GURL url("http://other.com/foo.png");
  CreateLoaderAndStart(origin, url, mojom::FetchRequestMode::kCORS);

  NotifyLoaderClientOnReceiveResponse(
      "Access-Control-Allow-Origin: http://some-other-domain.com");
  NotifyLoaderClientOnComplete(net::OK);

  RunUntilComplete();

  EXPECT_TRUE(IsNetworkLoaderStarted());
  EXPECT_FALSE(client().has_received_redirect());
  EXPECT_FALSE(client().has_received_response());
  EXPECT_EQ(net::ERR_FAILED, client().completion_status().error_code);
  ASSERT_TRUE(client().completion_status().cors_error_status);
  EXPECT_EQ(mojom::CORSError::kAllowOriginMismatch,
            client().completion_status().cors_error_status->cors_error);
}

TEST_F(CORSURLLoaderTest, StripUsernameAndPassword) {
  const GURL origin("http://example.com");
  const GURL url("http://foo:bar@other.com/foo.png");
  std::string stripped_url = "http://other.com/foo.png";
  CreateLoaderAndStart(origin, url, mojom::FetchRequestMode::kCORS);

  NotifyLoaderClientOnReceiveResponse(
      "Access-Control-Allow-Origin: http://example.com");
  NotifyLoaderClientOnComplete(net::OK);

  RunUntilComplete();

  EXPECT_TRUE(IsNetworkLoaderStarted());
  EXPECT_FALSE(client().has_received_redirect());
  EXPECT_TRUE(client().has_received_response());
  EXPECT_TRUE(client().has_received_completion());
  EXPECT_EQ(net::OK, client().completion_status().error_code);
  EXPECT_EQ(stripped_url, GetRequestedURL().spec());
}

}  // namespace

}  // namespace cors

}  // namespace network
