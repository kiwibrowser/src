// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/cryptauth/cryptauth_api_call_flow.h"

#include <memory>

#include "base/macros.h"
#include "base/test/test_simple_task_runner.h"
#include "net/base/net_errors.h"
#include "net/url_request/test_url_fetcher_factory.h"
#include "net/url_request/url_request_status.h"
#include "net/url_request/url_request_test_util.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace cryptauth {

namespace {

const char kSerializedRequestProto[] = "serialized_request_proto";
const char kSerializedResponseProto[] = "result_proto";
const char kRequestUrl[] = "https://googleapis.com/cryptauth/test";

}  // namespace

class CryptAuthApiCallFlowTest
    : public testing::Test,
      public net::TestURLFetcherDelegateForTests {
 protected:
  CryptAuthApiCallFlowTest()
      : url_request_context_getter_(new net::TestURLRequestContextGetter(
            new base::TestSimpleTaskRunner())) {
    flow_.SetPartialNetworkTrafficAnnotation(
        PARTIAL_TRAFFIC_ANNOTATION_FOR_TESTS);
  }

  void SetUp() override {
    // The TestURLFetcherFactory will override the global URLFetcherFactory for
    // the entire test.
    url_fetcher_factory_.reset(new net::TestURLFetcherFactory());
    url_fetcher_factory_->SetDelegateForTests(this);
  }

  void StartApiCallFlow() {
    StartApiCallFlowWithRequest(kSerializedRequestProto);
  }

  void StartApiCallFlowWithRequest(const std::string& serialized_request) {
    flow_.Start(GURL(kRequestUrl), url_request_context_getter_.get(),
                "access_token", serialized_request,
                base::Bind(&CryptAuthApiCallFlowTest::OnResult,
                           base::Unretained(this)),
                base::Bind(&CryptAuthApiCallFlowTest::OnError,
                           base::Unretained(this)));
    // URLFetcher object for the API request should be created.
    CheckCryptAuthHttpRequest(serialized_request);
  }

  void OnResult(const std::string& result) {
    EXPECT_FALSE(result_);
    result_.reset(new std::string(result));
  }

  void OnError(const std::string& error_message) {
    EXPECT_FALSE(error_message_);
    error_message_.reset(new std::string(error_message));
  }

  void CheckCryptAuthHttpRequest(const std::string& serialized_request) {
    ASSERT_TRUE(url_fetcher_);
    EXPECT_EQ(GURL(kRequestUrl), url_fetcher_->GetOriginalURL());
    EXPECT_EQ(serialized_request, url_fetcher_->upload_data());

    net::HttpRequestHeaders request_headers;
    url_fetcher_->GetExtraRequestHeaders(&request_headers);

    EXPECT_EQ("application/x-protobuf", url_fetcher_->upload_content_type());
  }

  // Responds to the current HTTP request. If the |error| is not |net::OK|, then
  // the |response_code| and |response_string| arguments will be ignored.
  void CompleteCurrentRequest(net::Error error,
                              int response_code,
                              const std::string& response_string) {
    ASSERT_TRUE(url_fetcher_);
    net::TestURLFetcher* url_fetcher = url_fetcher_;
    url_fetcher_ = nullptr;
    url_fetcher->set_status(net::URLRequestStatus::FromError(error));
    if (error == net::OK) {
      url_fetcher->set_response_code(response_code);
      url_fetcher->SetResponseString(response_string);
    }
    url_fetcher->delegate()->OnURLFetchComplete(url_fetcher);
  }

  // net::TestURLFetcherDelegateForTests overrides.
  void OnRequestStart(int fetcher_id) override {
    url_fetcher_ = url_fetcher_factory_->GetFetcherByID(fetcher_id);
  }

  void OnChunkUpload(int fetcher_id) override {}

  void OnRequestEnd(int fetcher_id) override {}

  net::TestURLFetcher* url_fetcher_;
  std::unique_ptr<std::string> result_;
  std::unique_ptr<std::string> error_message_;

 private:
  scoped_refptr<net::TestURLRequestContextGetter> url_request_context_getter_;
  std::unique_ptr<net::TestURLFetcherFactory> url_fetcher_factory_;
  CryptAuthApiCallFlow flow_;

  DISALLOW_COPY_AND_ASSIGN(CryptAuthApiCallFlowTest);
};

TEST_F(CryptAuthApiCallFlowTest, RequestSuccess) {
  StartApiCallFlow();
  CompleteCurrentRequest(net::OK, net::HTTP_OK, kSerializedResponseProto);
  EXPECT_EQ(kSerializedResponseProto, *result_);
  EXPECT_FALSE(error_message_);
}

TEST_F(CryptAuthApiCallFlowTest, RequestFailure) {
  StartApiCallFlow();
  CompleteCurrentRequest(net::ERR_FAILED, 0, std::string());
  EXPECT_FALSE(result_);
  EXPECT_EQ("Request failed", *error_message_);
}

TEST_F(CryptAuthApiCallFlowTest, RequestStatus500) {
  StartApiCallFlow();
  CompleteCurrentRequest(net::OK, net::HTTP_INTERNAL_SERVER_ERROR,
                         "CryptAuth Meltdown.");
  EXPECT_FALSE(result_);
  EXPECT_EQ("HTTP status: 500", *error_message_);
}

// The empty string is a valid protocol buffer message serialization.
TEST_F(CryptAuthApiCallFlowTest, RequestWithNoBody) {
  StartApiCallFlowWithRequest(std::string());
  CompleteCurrentRequest(net::OK, net::HTTP_OK, kSerializedResponseProto);
  EXPECT_EQ(kSerializedResponseProto, *result_);
  EXPECT_FALSE(error_message_);
}

// The empty string is a valid protocol buffer message serialization.
TEST_F(CryptAuthApiCallFlowTest, ResponseWithNoBody) {
  StartApiCallFlow();
  CompleteCurrentRequest(net::OK, net::HTTP_OK, std::string());
  EXPECT_EQ(std::string(), *result_);
  EXPECT_FALSE(error_message_);
}

}  // namespace cryptauth
