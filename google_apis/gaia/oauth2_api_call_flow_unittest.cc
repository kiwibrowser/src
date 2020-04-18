// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// A complete set of unit tests for OAuth2MintTokenFlow.

#include "google_apis/gaia/oauth2_api_call_flow.h"

#include <memory>
#include <string>
#include <utility>

#include "base/message_loop/message_loop.h"
#include "base/time/time.h"
#include "google_apis/gaia/gaia_urls.h"
#include "google_apis/gaia/google_service_auth_error.h"
#include "google_apis/gaia/oauth2_access_token_consumer.h"
#include "google_apis/gaia/oauth2_access_token_fetcher_impl.h"
#include "net/base/net_errors.h"
#include "net/http/http_request_headers.h"
#include "net/http/http_status_code.h"
#include "net/traffic_annotation/network_traffic_annotation_test_helper.h"
#include "net/url_request/test_url_fetcher_factory.h"
#include "net/url_request/url_fetcher.h"
#include "net/url_request/url_fetcher_delegate.h"
#include "net/url_request/url_fetcher_factory.h"
#include "net/url_request/url_request.h"
#include "net/url_request/url_request_status.h"
#include "net/url_request/url_request_test_util.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

using net::HttpRequestHeaders;
using net::ScopedURLFetcherFactory;
using net::TestURLFetcher;
using net::URLFetcher;
using net::URLFetcherDelegate;
using net::URLFetcherFactory;
using net::URLRequestContextGetter;
using net::URLRequestStatus;
using testing::_;
using testing::ByMove;
using testing::Return;
using testing::StrictMock;

namespace {

const char kAccessToken[] = "access_token";

static std::string CreateBody() {
  return "some body";
}

static GURL CreateApiUrl() {
  return GURL("https://www.googleapis.com/someapi");
}

// Replaces the global URLFetcher factory so the test can return a custom
// URLFetcher to complete requests.
class MockUrlFetcherFactory : public ScopedURLFetcherFactory,
                              public URLFetcherFactory {
 public:
  MockUrlFetcherFactory()
      : ScopedURLFetcherFactory(this) {
  }
  ~MockUrlFetcherFactory() override {}

  MOCK_METHOD5(CreateURLFetcherMock,
               std::unique_ptr<URLFetcher>(
                   int id,
                   const GURL& url,
                   URLFetcher::RequestType request_type,
                   URLFetcherDelegate* d,
                   net::NetworkTrafficAnnotationTag traffic_annotation));

  std::unique_ptr<URLFetcher> CreateURLFetcher(
      int id,
      const GURL& url,
      URLFetcher::RequestType request_type,
      URLFetcherDelegate* d,
      net::NetworkTrafficAnnotationTag traffic_annotation) override {
    return CreateURLFetcherMock(id, url, request_type, d, traffic_annotation);
  }
};

class MockApiCallFlow : public OAuth2ApiCallFlow {
 public:
  MockApiCallFlow() {}
  ~MockApiCallFlow() override {}

  MOCK_METHOD0(CreateApiCallUrl, GURL());
  MOCK_METHOD0(CreateApiCallBody, std::string());
  MOCK_METHOD1(ProcessApiCallSuccess, void(const URLFetcher* source));
  MOCK_METHOD1(ProcessApiCallFailure, void(const URLFetcher* source));
  MOCK_METHOD1(ProcessNewAccessToken, void(const std::string& access_token));
  MOCK_METHOD1(ProcessMintAccessTokenFailure,
               void(const GoogleServiceAuthError& error));

  net::PartialNetworkTrafficAnnotationTag GetNetworkTrafficAnnotationTag()
      override {
    return PARTIAL_TRAFFIC_ANNOTATION_FOR_TESTS;
  }
};

}  // namespace

class OAuth2ApiCallFlowTest : public testing::Test {
 protected:
  OAuth2ApiCallFlowTest()
      : request_context_getter_(
            base::MakeRefCounted<net::TestURLRequestContextGetter>(
                message_loop_.task_runner())) {}

  std::unique_ptr<TestURLFetcher> CreateURLFetcher(const GURL& url,
                                                   bool fetch_succeeds,
                                                   int response_code,
                                                   const std::string& body) {
    auto url_fetcher = std::make_unique<TestURLFetcher>(0, url, &flow_);
    net::Error error = fetch_succeeds ? net::OK : net::ERR_FAILED;
    url_fetcher->set_status(URLRequestStatus::FromError(error));

    if (response_code != 0)
      url_fetcher->set_response_code(response_code);

    if (!body.empty())
      url_fetcher->SetResponseString(body);

    return url_fetcher;
  }

  TestURLFetcher* SetupApiCall(bool succeeds, net::HttpStatusCode status) {
    std::string body(CreateBody());
    GURL url(CreateApiUrl());
    EXPECT_CALL(flow_, CreateApiCallBody()).WillOnce(Return(body));
    EXPECT_CALL(flow_, CreateApiCallUrl()).WillOnce(Return(url));
    std::unique_ptr<TestURLFetcher> url_fetcher =
        CreateURLFetcher(url, succeeds, status, std::string());
    TestURLFetcher* url_fetcher_ptr = url_fetcher.get();
    EXPECT_CALL(factory_, CreateURLFetcherMock(_, url, _, _, _))
        .WillOnce(Return(ByMove(std::move(url_fetcher))));
    return url_fetcher_ptr;
  }

  base::MessageLoop message_loop_;
  scoped_refptr<net::TestURLRequestContextGetter> request_context_getter_;
  StrictMock<MockApiCallFlow> flow_;
  MockUrlFetcherFactory factory_;
};

TEST_F(OAuth2ApiCallFlowTest, ApiCallSucceedsHttpOk) {
  TestURLFetcher* url_fetcher = SetupApiCall(true, net::HTTP_OK);
  EXPECT_CALL(flow_, ProcessApiCallSuccess(url_fetcher));
  flow_.Start(request_context_getter_.get(), kAccessToken);
  flow_.OnURLFetchComplete(url_fetcher);
}

TEST_F(OAuth2ApiCallFlowTest, ApiCallSucceedsHttpNoContent) {
  TestURLFetcher* url_fetcher = SetupApiCall(true, net::HTTP_NO_CONTENT);
  EXPECT_CALL(flow_, ProcessApiCallSuccess(url_fetcher));
  flow_.Start(request_context_getter_.get(), kAccessToken);
  flow_.OnURLFetchComplete(url_fetcher);
}

TEST_F(OAuth2ApiCallFlowTest, ApiCallFailure) {
  TestURLFetcher* url_fetcher = SetupApiCall(true, net::HTTP_UNAUTHORIZED);
  EXPECT_CALL(flow_, ProcessApiCallFailure(url_fetcher));
  flow_.Start(request_context_getter_.get(), kAccessToken);
  flow_.OnURLFetchComplete(url_fetcher);
}

TEST_F(OAuth2ApiCallFlowTest, ExpectedHTTPHeaders) {
  std::string body = CreateBody();
  GURL url(CreateApiUrl());

  TestURLFetcher* url_fetcher = SetupApiCall(true, net::HTTP_OK);
  flow_.Start(request_context_getter_.get(), kAccessToken);
  HttpRequestHeaders headers;
  url_fetcher->GetExtraRequestHeaders(&headers);
  std::string auth_header;
  EXPECT_TRUE(headers.GetHeader("Authorization", &auth_header));
  EXPECT_EQ("Bearer access_token", auth_header);
  EXPECT_EQ(url, url_fetcher->GetOriginalURL());
  EXPECT_EQ(body, url_fetcher->upload_data());
}
