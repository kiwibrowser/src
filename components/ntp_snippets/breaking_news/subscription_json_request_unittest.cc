// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/ntp_snippets/breaking_news/subscription_json_request.h"

#include "base/json/json_reader.h"
#include "base/message_loop/message_loop.h"
#include "base/test/gtest_util.h"
#include "base/test/mock_callback.h"
#include "base/values.h"
#include "net/url_request/test_url_fetcher_factory.h"
#include "net/url_request/url_request_test_util.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace ntp_snippets {

namespace internal {

namespace {

using testing::_;
using testing::SaveArg;

// TODO(mamir): Create a test_helper.cc file instead of duplicating all this
// code.
MATCHER_P(EqualsJSON, json, "equals JSON") {
  std::unique_ptr<base::Value> expected = base::JSONReader::Read(json);
  if (!expected) {
    *result_listener << "INTERNAL ERROR: couldn't parse expected JSON";
    return false;
  }

  std::string err_msg;
  int err_line, err_col;
  std::unique_ptr<base::Value> actual = base::JSONReader::ReadAndReturnError(
      arg, base::JSON_PARSE_RFC, nullptr, &err_msg, &err_line, &err_col);
  if (!actual) {
    *result_listener << "input:" << err_line << ":" << err_col << ": "
                     << "parse error: " << err_msg;
    return false;
  }
  return *expected == *actual;
}

}  // namespace

class SubscriptionJsonRequestTest : public testing::Test {
 public:
  SubscriptionJsonRequestTest()
      : request_context_getter_(
            new net::TestURLRequestContextGetter(message_loop_.task_runner())) {
  }

  scoped_refptr<net::URLRequestContextGetter> GetRequestContext() {
    return request_context_getter_.get();
  }

  net::TestURLFetcher* GetRunningFetcher() {
    // All created TestURLFetchers have ID 0 by default.
    net::TestURLFetcher* url_fetcher = url_fetcher_factory_.GetFetcherByID(0);
    DCHECK(url_fetcher);
    return url_fetcher;
  }

  void RespondWithData(const std::string& data) {
    net::TestURLFetcher* url_fetcher = GetRunningFetcher();
    url_fetcher->set_status(net::URLRequestStatus());
    url_fetcher->set_response_code(net::HTTP_OK);
    url_fetcher->SetResponseString(data);
    // Call the URLFetcher delegate to continue the test.
    url_fetcher->delegate()->OnURLFetchComplete(url_fetcher);
  }

  void RespondWithError(int error_code) {
    net::TestURLFetcher* url_fetcher = GetRunningFetcher();
    url_fetcher->set_status(net::URLRequestStatus::FromError(error_code));
    url_fetcher->SetResponseString(std::string());
    // Call the URLFetcher delegate to continue the test.
    url_fetcher->delegate()->OnURLFetchComplete(url_fetcher);
  }

 private:
  base::MessageLoop message_loop_;
  scoped_refptr<net::TestURLRequestContextGetter> request_context_getter_;
  net::TestURLFetcherFactory url_fetcher_factory_;

  DISALLOW_COPY_AND_ASSIGN(SubscriptionJsonRequestTest);
};

TEST_F(SubscriptionJsonRequestTest, BuildRequest) {
  std::string token = "1234567890";
  GURL url("http://valid-url.test");

  base::MockCallback<SubscriptionJsonRequest::CompletedCallback> callback;

  SubscriptionJsonRequest::Builder builder;
  std::unique_ptr<SubscriptionJsonRequest> request =
      builder.SetToken(token)
          .SetUrl(url)
          .SetUrlRequestContextGetter(GetRequestContext())
          .SetLocale("en-US")
          .SetCountryCode("us")
          .Build();
  request->Start(callback.Get());

  net::TestURLFetcher* url_fetcher = GetRunningFetcher();

  EXPECT_EQ(url, url_fetcher->GetOriginalURL());

  net::HttpRequestHeaders headers;
  url_fetcher->GetExtraRequestHeaders(&headers);

  std::string header;
  EXPECT_FALSE(headers.GetHeader("Authorization", &header));
  EXPECT_TRUE(headers.GetHeader("Content-Type", &header));
  EXPECT_EQ(header, "application/json; charset=UTF-8");

  std::string expected_body = R"(
    {
      "token": "1234567890",
      "locale": "en-US",
      "country_code": "us"
    }
  )";
  EXPECT_THAT(url_fetcher->upload_data(), EqualsJSON(expected_body));
}

TEST_F(SubscriptionJsonRequestTest, ShouldNotInvokeCallbackWhenCancelled) {
  std::string token = "1234567890";
  GURL url("http://valid-url.test");

  base::MockCallback<SubscriptionJsonRequest::CompletedCallback> callback;
  EXPECT_CALL(callback, Run(_)).Times(0);

  SubscriptionJsonRequest::Builder builder;
  std::unique_ptr<SubscriptionJsonRequest> request =
      builder.SetToken(token)
          .SetUrl(url)
          .SetUrlRequestContextGetter(GetRequestContext())
          .Build();
  request->Start(callback.Get());

  // Destroy the request before getting any response.
  request.reset();
}

TEST_F(SubscriptionJsonRequestTest, SubscribeWithoutErrors) {
  std::string token = "1234567890";
  GURL url("http://valid-url.test");

  base::MockCallback<SubscriptionJsonRequest::CompletedCallback> callback;
  ntp_snippets::Status status(StatusCode::PERMANENT_ERROR, "initial");
  EXPECT_CALL(callback, Run(_)).WillOnce(SaveArg<0>(&status));

  SubscriptionJsonRequest::Builder builder;
  std::unique_ptr<SubscriptionJsonRequest> request =
      builder.SetToken(token)
          .SetUrl(url)
          .SetUrlRequestContextGetter(GetRequestContext())
          .Build();
  request->Start(callback.Get());

  RespondWithData("{}");

  EXPECT_EQ(status.code, StatusCode::SUCCESS);
}

TEST_F(SubscriptionJsonRequestTest, SubscribeWithErrors) {
  std::string token = "1234567890";
  GURL url("http://valid-url.test");

  base::MockCallback<SubscriptionJsonRequest::CompletedCallback> callback;
  ntp_snippets::Status status(StatusCode::SUCCESS, "initial");
  EXPECT_CALL(callback, Run(_)).WillOnce(SaveArg<0>(&status));

  SubscriptionJsonRequest::Builder builder;
  std::unique_ptr<SubscriptionJsonRequest> request =
      builder.SetToken(token)
          .SetUrl(url)
          .SetUrlRequestContextGetter(GetRequestContext())
          .Build();
  request->Start(callback.Get());

  RespondWithError(net::ERR_TIMED_OUT);

  EXPECT_EQ(status.code, StatusCode::TEMPORARY_ERROR);
}

}  // namespace internal

}  // namespace ntp_snippets
