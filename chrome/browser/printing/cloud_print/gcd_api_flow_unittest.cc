// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/printing/cloud_print/gcd_api_flow.h"

#include <memory>
#include <utility>

#include "base/bind.h"
#include "base/threading/thread_task_runner_handle.h"
#include "base/values.h"
#include "chrome/browser/printing/cloud_print/gcd_api_flow_impl.h"
#include "content/public/test/test_browser_thread_bundle.h"
#include "google_apis/gaia/fake_oauth2_token_service.h"
#include "google_apis/gaia/google_service_auth_error.h"
#include "net/base/host_port_pair.h"
#include "net/base/net_errors.h"
#include "net/http/http_request_headers.h"
#include "net/url_request/test_url_fetcher_factory.h"
#include "net/url_request/url_request_test_util.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

using testing::_;
using testing::Return;

namespace cloud_print {

namespace {

const char kSampleConfirmResponse[] = "{}";

const char kFailedConfirmResponseBadJson[] = "[]";

const char kAccountId[] = "account_id";

class MockDelegate : public CloudPrintApiFlowRequest {
 public:
  MOCK_METHOD1(OnGCDApiFlowError, void(GCDApiFlow::Status));
  MOCK_METHOD1(OnGCDApiFlowComplete, void(const base::DictionaryValue&));
  MOCK_METHOD0(GetURL, GURL());
  MOCK_METHOD0(GetNetworkTrafficAnnotationType,
               GCDApiFlow::Request::NetworkTrafficAnnotation());
};

class GCDApiFlowTest : public testing::Test {
 public:
  GCDApiFlowTest()
      : request_context_(new net::TestURLRequestContextGetter(
            base::ThreadTaskRunnerHandle::Get())),
        account_id_(kAccountId) {}

  ~GCDApiFlowTest() override {}

 protected:
  void SetUp() override {
    token_service_.GetFakeOAuth2TokenServiceDelegate()->set_request_context(
        request_context_.get());
    token_service_.AddAccount(account_id_);

    std::unique_ptr<MockDelegate> delegate = std::make_unique<MockDelegate>();
    mock_delegate_ = delegate.get();
    EXPECT_CALL(*mock_delegate_, GetURL())
        .WillRepeatedly(Return(
            GURL("https://www.google.com/cloudprint/confirm?token=SomeToken")));
    gcd_flow_ = std::make_unique<GCDApiFlowImpl>(request_context_.get(),
                                                 &token_service_, account_id_);
    gcd_flow_->Start(std::move(delegate));
  }

  content::TestBrowserThreadBundle test_browser_thread_bundle_;
  scoped_refptr<net::TestURLRequestContextGetter> request_context_;
  net::TestURLFetcherFactory fetcher_factory_;
  FakeOAuth2TokenService token_service_;
  std::string account_id_;
  std::unique_ptr<GCDApiFlowImpl> gcd_flow_;
  MockDelegate* mock_delegate_;
};

TEST_F(GCDApiFlowTest, SuccessOAuth2) {
  gcd_flow_->OnGetTokenSuccess(NULL, "SomeToken", base::Time());
  net::TestURLFetcher* fetcher = fetcher_factory_.GetFetcherByID(0);

  EXPECT_EQ(GURL("https://www.google.com/cloudprint/confirm?token=SomeToken"),
            fetcher->GetOriginalURL());

  net::HttpRequestHeaders headers;
  fetcher->GetExtraRequestHeaders(&headers);
  std::string oauth_header;
  std::string proxy;
  EXPECT_TRUE(headers.GetHeader("Authorization", &oauth_header));
  EXPECT_EQ("Bearer SomeToken", oauth_header);
  EXPECT_TRUE(headers.GetHeader("X-Cloudprint-Proxy", &proxy));
  EXPECT_EQ("Chrome", proxy);

  fetcher->SetResponseString(kSampleConfirmResponse);
  fetcher->set_status(
      net::URLRequestStatus(net::URLRequestStatus::SUCCESS, net::OK));
  fetcher->set_response_code(200);

  EXPECT_CALL(*mock_delegate_, OnGCDApiFlowComplete(_));

  fetcher->delegate()->OnURLFetchComplete(fetcher);
}

TEST_F(GCDApiFlowTest, BadToken) {
  EXPECT_CALL(*mock_delegate_, OnGCDApiFlowError(GCDApiFlow::ERROR_TOKEN));
  gcd_flow_->OnGetTokenFailure(
      NULL, GoogleServiceAuthError(GoogleServiceAuthError::USER_NOT_SIGNED_UP));
}

TEST_F(GCDApiFlowTest, BadJson) {
  gcd_flow_->OnGetTokenSuccess(NULL, "SomeToken", base::Time());
  net::TestURLFetcher* fetcher = fetcher_factory_.GetFetcherByID(0);

  EXPECT_EQ(GURL("https://www.google.com/cloudprint/confirm?token=SomeToken"),
            fetcher->GetOriginalURL());

  fetcher->SetResponseString(kFailedConfirmResponseBadJson);
  fetcher->set_status(
      net::URLRequestStatus(net::URLRequestStatus::SUCCESS, net::OK));
  fetcher->set_response_code(200);

  EXPECT_CALL(*mock_delegate_,
              OnGCDApiFlowError(GCDApiFlow::ERROR_MALFORMED_RESPONSE));

  fetcher->delegate()->OnURLFetchComplete(fetcher);
}

}  // namespace

}  // namespace cloud_print
