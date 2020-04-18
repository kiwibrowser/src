// Copyright (c) 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <memory>
#include <string>

#include "base/callback.h"
#include "base/macros.h"
#include "base/run_loop.h"
#include "base/test/scoped_task_environment.h"
#include "chrome/browser/media/router/discovery/dial/dial_url_fetcher.h"
#include "chrome/browser/media/router/test/test_helper.h"
#include "chrome/test/base/testing_profile.h"
#include "content/public/test/test_browser_thread_bundle.h"
#include "net/http/http_response_headers.h"
#include "net/http/http_status_code.h"
#include "net/url_request/test_url_fetcher_factory.h"
#include "net/url_request/url_fetcher.h"
#include "services/network/test/test_url_loader_factory.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

namespace media_router {

class DialURLFetcherTest : public testing::Test {
 public:
  DialURLFetcherTest() : url_("http://127.0.0.1/app/Youtube") {}

  void ExpectSuccess(const std::string& expected_app_info) {
    EXPECT_CALL(*this, OnSuccess(expected_app_info));
  }

  void ExpectError(const std::string& expected_error) {
    expected_error_ = expected_error;
    EXPECT_CALL(*this, DoOnError());
  }

  void StartGetRequest() {
    fetcher_ = std::make_unique<TestDialURLFetcher>(
        base::BindOnce(&DialURLFetcherTest::OnSuccess, base::Unretained(this)),
        base::BindOnce(&DialURLFetcherTest::OnError, base::Unretained(this)),
        &loader_factory_);
    fetcher_->Get(url_);
    base::RunLoop().RunUntilIdle();
  }

 protected:
  base::test::ScopedTaskEnvironment environment_;
  network::TestURLLoaderFactory loader_factory_;
  const GURL url_;
  std::string expected_error_;
  std::unique_ptr<TestDialURLFetcher> fetcher_;

 private:
  MOCK_METHOD1(OnSuccess, void(const std::string&));
  MOCK_METHOD0(DoOnError, void());

  void OnError(int response_code, const std::string& message) {
    EXPECT_TRUE(message.find(expected_error_) != std::string::npos)
        << "[" << expected_error_ << "] not found in message [" << message
        << "]";
    DoOnError();
  }

  DISALLOW_COPY_AND_ASSIGN(DialURLFetcherTest);
};

TEST_F(DialURLFetcherTest, FetchSuccessful) {
  std::string body("<xml>appInfo</xml>");
  ExpectSuccess(body);
  network::URLLoaderCompletionStatus status;
  status.decoded_body_length = body.size();
  loader_factory_.AddResponse(url_, network::ResourceResponseHead(), body,
                              status);
  StartGetRequest();
}

TEST_F(DialURLFetcherTest, FetchFailsOnMissingAppInfo) {
  ExpectError("404");

  loader_factory_.AddResponse(
      url_, network::ResourceResponseHead(), "",
      network::URLLoaderCompletionStatus(net::HTTP_NOT_FOUND));
  StartGetRequest();
}

TEST_F(DialURLFetcherTest, FetchFailsOnEmptyAppInfo) {
  ExpectError("Missing or empty response");

  loader_factory_.AddResponse(url_, network::ResourceResponseHead(), "",
                              network::URLLoaderCompletionStatus());
  StartGetRequest();
}

TEST_F(DialURLFetcherTest, FetchFailsOnBadAppInfo) {
  ExpectError("Invalid response encoding");
  std::string body("\xfc\x9c\xbf\x80\xbf\x80");
  network::URLLoaderCompletionStatus status;
  status.decoded_body_length = body.size();
  loader_factory_.AddResponse(url_, network::ResourceResponseHead(), body,
                              status);
  StartGetRequest();
}

}  // namespace media_router
