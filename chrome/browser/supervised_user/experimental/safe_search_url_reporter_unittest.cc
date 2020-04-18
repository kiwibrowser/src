// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/supervised_user/experimental/safe_search_url_reporter.h"

#include <memory>

#include "base/message_loop/message_loop.h"
#include "base/threading/thread_task_runner_handle.h"
#include "base/values.h"
#include "components/signin/core/browser/fake_profile_oauth2_token_service.h"
#include "net/base/net_errors.h"
#include "net/http/http_util.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"
#include "services/network/public/cpp/weak_wrapper_shared_url_loader_factory.h"
#include "services/network/test/test_url_loader_factory.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace {

const char kSafeSearchReportApiUrl[] =
    "https://safesearch.googleapis.com/v1:report";
const char kAccountId[] = "account@gmail.com";

}  // namespace

class SafeSearchURLReporterTest : public testing::Test {
 public:
  SafeSearchURLReporterTest()
      : test_shared_loader_factory_(
            base::MakeRefCounted<network::WeakWrapperSharedURLLoaderFactory>(
                &test_url_loader_factory_)),
        report_url_(&token_service_, kAccountId, test_shared_loader_factory_) {
    token_service_.UpdateCredentials(kAccountId, "refresh_token");
  }

 protected:
  void IssueAccessTokens() {
    token_service_.IssueAllTokensForAccount(
        kAccountId, "access_token",
        base::Time::Now() + base::TimeDelta::FromHours(1));
  }

  void IssueAccessTokenErrors() {
    token_service_.IssueErrorForAllPendingRequestsForAccount(
        kAccountId, GoogleServiceAuthError::FromServiceError("Error!"));
  }

  void SetupResponse(net::Error error) {
    network::ResourceResponseHead head;
    std::string headers("HTTP/1.1 200 OK\n\n");
    head.headers = base::MakeRefCounted<net::HttpResponseHeaders>(
        net::HttpUtil::AssembleRawHeaders(headers.c_str(), headers.size()));
    network::URLLoaderCompletionStatus status(error);
    test_url_loader_factory_.AddResponse(GURL(kSafeSearchReportApiUrl), head,
                                         std::string(), status);
  }

  void CreateRequest(const GURL& url) {
    report_url_.ReportUrl(
        url, base::BindOnce(&SafeSearchURLReporterTest::OnRequestCreated,
                            base::Unretained(this)));
  }

  void WaitForResponse() { base::RunLoop().RunUntilIdle(); }

  MOCK_METHOD1(OnRequestCreated, void(bool success));

  base::MessageLoop message_loop_;
  FakeProfileOAuth2TokenService token_service_;
  network::TestURLLoaderFactory test_url_loader_factory_;
  scoped_refptr<network::SharedURLLoaderFactory> test_shared_loader_factory_;
  SafeSearchURLReporter report_url_;
};

TEST_F(SafeSearchURLReporterTest, Success) {
  CreateRequest(GURL("http://google.com"));
  CreateRequest(GURL("http://url.com"));

  EXPECT_GT(token_service_.GetPendingRequests().size(), 0U);

  IssueAccessTokens();

  EXPECT_CALL(*this, OnRequestCreated(true)).Times(2);
  SetupResponse(net::OK);
  SetupResponse(net::OK);
  WaitForResponse();
}

TEST_F(SafeSearchURLReporterTest, AccessTokenError) {
  CreateRequest(GURL("http://google.com"));

  EXPECT_EQ(1U, token_service_.GetPendingRequests().size());

  EXPECT_CALL(*this, OnRequestCreated(false));
  IssueAccessTokenErrors();
}

TEST_F(SafeSearchURLReporterTest, NetworkError) {
  CreateRequest(GURL("http://google.com"));

  EXPECT_EQ(1U, token_service_.GetPendingRequests().size());

  IssueAccessTokens();

  EXPECT_CALL(*this, OnRequestCreated(false));
  SetupResponse(net::ERR_ABORTED);
  WaitForResponse();
}
