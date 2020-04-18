// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/webui/url_data_manager_backend.h"

#include <memory>

#include "base/macros.h"
#include "base/run_loop.h"
#include "content/public/test/mock_resource_context.h"
#include "content/public/test/test_browser_thread_bundle.h"
#include "net/http/http_response_headers.h"
#include "net/http/http_response_info.h"
#include "net/traffic_annotation/network_traffic_annotation_test_helper.h"
#include "net/url_request/url_request_context.h"
#include "net/url_request/url_request_job.h"
#include "net/url_request/url_request_job_factory_impl.h"
#include "net/url_request/url_request_test_util.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace content {

class CancelAfterFirstReadURLRequestDelegate : public net::TestDelegate {
 public:
  CancelAfterFirstReadURLRequestDelegate() {}

  ~CancelAfterFirstReadURLRequestDelegate() override {}

  void OnResponseStarted(net::URLRequest* request, int net_error) override {
    DCHECK_NE(net::ERR_IO_PENDING, net_error);
    // net::TestDelegate will start the first read.
    TestDelegate::OnResponseStarted(request, net_error);
    request->Cancel();
  }

  void OnReadCompleted(net::URLRequest* request, int bytes_read) override {
    // Read should have been cancelled.
    EXPECT_EQ(net::ERR_ABORTED, bytes_read);
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(CancelAfterFirstReadURLRequestDelegate);
};

class UrlDataManagerBackendTest : public testing::Test {
 public:
  UrlDataManagerBackendTest()
      : thread_bundle_(TestBrowserThreadBundle::IO_MAINLOOP) {
    // URLRequestJobFactory takes ownership of the passed in ProtocolHandler.
    url_request_job_factory_.SetProtocolHandler(
        "chrome", URLDataManagerBackend::CreateProtocolHandler(
                      &resource_context_, nullptr));
    url_request_context_.set_job_factory(&url_request_job_factory_);
  }

  std::unique_ptr<net::URLRequest> CreateRequest(
      net::URLRequest::Delegate* delegate,
      const char* origin) {
    std::unique_ptr<net::URLRequest> request =
        url_request_context_.CreateRequest(
            GURL(
                "chrome://resources/polymer/v1_0/polymer/polymer-extracted.js"),
            net::HIGHEST, delegate, TRAFFIC_ANNOTATION_FOR_TESTS);
    request->SetExtraRequestHeaderByName("Origin", origin, true);
    return request;
  }

 protected:
  TestBrowserThreadBundle thread_bundle_;
  MockResourceContext resource_context_;
  net::URLRequestJobFactoryImpl url_request_job_factory_;
  net::URLRequestContext url_request_context_;
  net::TestDelegate delegate_;
};

TEST_F(UrlDataManagerBackendTest, AccessControlAllowOriginChromeUrl) {
  std::unique_ptr<net::URLRequest> request(
      CreateRequest(&delegate_, "chrome://webui"));
  request->Start();
  base::RunLoop().RunUntilIdle();
  EXPECT_TRUE(request->response_headers()->HasHeaderValue(
      "Access-Control-Allow-Origin", "chrome://webui"));
}

TEST_F(UrlDataManagerBackendTest, AccessControlAllowOriginNonChromeUrl) {
  std::unique_ptr<net::URLRequest> request(
      CreateRequest(&delegate_, "http://www.example.com"));
  request->Start();
  base::RunLoop().RunUntilIdle();
  EXPECT_TRUE(request->response_headers()->HasHeaderValue(
      "Access-Control-Allow-Origin", "null"));
}

// Check that the URLRequest isn't passed headers after cancellation.
TEST_F(UrlDataManagerBackendTest, CancelBeforeResponseStarts) {
  std::unique_ptr<net::URLRequest> request(
      CreateRequest(&delegate_, "chrome://webui"));
  request->Start();
  request->Cancel();
  base::RunLoop().RunUntilIdle();
  EXPECT_EQ(net::URLRequestStatus::CANCELED, request->status().status());
  EXPECT_EQ(1, delegate_.response_started_count());
}

// Check that the URLRequest isn't passed data after cancellation.
TEST_F(UrlDataManagerBackendTest, CancelAfterFirstReadStarted) {
  CancelAfterFirstReadURLRequestDelegate cancel_delegate;
  std::unique_ptr<net::URLRequest> request(
      CreateRequest(&cancel_delegate, "chrome://webui"));
  request->Start();
  base::RunLoop().RunUntilIdle();

  EXPECT_EQ(net::URLRequestStatus::CANCELED, request->status().status());
  EXPECT_EQ(1, cancel_delegate.response_started_count());
  EXPECT_EQ("", cancel_delegate.data_received());
}

// Check for a network error page request via chrome://network-error/.
TEST_F(UrlDataManagerBackendTest, ChromeNetworkErrorPageRequest) {
  std::unique_ptr<net::URLRequest> error_request =
      url_request_context_.CreateRequest(GURL("chrome://network-error/-105"),
                                         net::HIGHEST, &delegate_,
                                         TRAFFIC_ANNOTATION_FOR_TESTS);
  error_request->Start();
  base::RunLoop().Run();
  EXPECT_EQ(net::URLRequestStatus::FAILED, error_request->status().status());
  EXPECT_EQ(net::ERR_NAME_NOT_RESOLVED, error_request->status().error());
}

// Check for an invalid network error page request via chrome://network-error/.
TEST_F(UrlDataManagerBackendTest, ChromeNetworkErrorPageRequestFailed) {
  std::unique_ptr<net::URLRequest> error_request =
      url_request_context_.CreateRequest(
          GURL("chrome://network-error/-123456789"), net::HIGHEST, &delegate_,
          TRAFFIC_ANNOTATION_FOR_TESTS);
  error_request->Start();
  base::RunLoop().Run();
  EXPECT_EQ(net::URLRequestStatus::FAILED, error_request->status().status());
  EXPECT_EQ(net::ERR_INVALID_URL, error_request->status().error());
}

}  // namespace content
