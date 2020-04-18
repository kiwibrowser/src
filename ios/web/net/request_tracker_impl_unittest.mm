// Copyright 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/web/net/request_tracker_impl.h"

#include <stddef.h>

#include <memory>
#include <vector>

#include "base/logging.h"
#include "base/macros.h"
#include "base/memory/ptr_util.h"
#include "base/message_loop/message_loop.h"
#include "base/run_loop.h"
#include "base/strings/sys_string_conversions.h"
#include "ios/web/public/cert_policy.h"
#include "ios/web/public/certificate_policy_cache.h"
#include "ios/web/public/ssl_status.h"
#include "ios/web/public/test/fakes/test_browser_state.h"
#include "ios/web/public/test/test_web_thread.h"
#include "net/cert/x509_certificate.h"
#include "net/http/http_response_headers.h"
#include "net/test/cert_test_util.h"
#include "net/test/test_data_directory.h"
#include "net/url_request/url_request.h"
#include "net/url_request/url_request_context.h"
#include "net/url_request/url_request_job_factory.h"
#include "net/url_request/url_request_job_factory_impl.h"
#include "net/url_request/url_request_test_job.h"
#include "net/url_request/url_request_test_util.h"
#include "testing/gtest/include/gtest/gtest.h"
#import "testing/gtest_mac.h"
#include "testing/platform_test.h"
#import "third_party/ocmock/OCMock/OCMock.h"
#include "third_party/ocmock/gtest_support.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

@interface RequestTrackerNotificationReceiverTest : NSObject {
 @public
  float value_;
  float max_;
 @private
  NSString* error_;
  scoped_refptr<net::HttpResponseHeaders> headers_;
}

- (NSString*)error;
- (net::HttpResponseHeaders*)headers;
@end

@implementation RequestTrackerNotificationReceiverTest

- (id)init {
  self = [super init];
  if (self) {
    value_ = 0.0f;
    max_ = 0.0f;
  }
  return self;
}

- (NSString*)error {
  return error_;
}

- (net::HttpResponseHeaders*)headers {
  return headers_.get();
}

@end

namespace {

// Used and incremented each time a tabId is created.
int g_count = 0;

// URLRequest::Delegate that does nothing.
class DummyURLRequestDelegate : public net::URLRequest::Delegate {
 public:
  DummyURLRequestDelegate() {}
  ~DummyURLRequestDelegate() override {}

  void OnResponseStarted(net::URLRequest* request, int net_error) override {}
  void OnReadCompleted(net::URLRequest* request, int bytes_read) override {}

 private:
  DISALLOW_COPY_AND_ASSIGN(DummyURLRequestDelegate);
};

class RequestTrackerTest : public PlatformTest {
 public:
  RequestTrackerTest()
      : loop_(base::MessageLoop::TYPE_IO),
        ui_thread_(web::WebThread::UI, &loop_),
        io_thread_(web::WebThread::IO, &loop_){};

  ~RequestTrackerTest() override {}

  void SetUp() override {
    DCHECK_CURRENTLY_ON(web::WebThread::UI);
    request_group_id_ = [NSString stringWithFormat:@"test%d", g_count++];

    tracker_ = web::RequestTrackerImpl::CreateTrackerForRequestGroupID(
        request_group_id_, &browser_state_, browser_state_.GetRequestContext());
  }

  void TearDown() override {
    DCHECK_CURRENTLY_ON(web::WebThread::UI);
    tracker_->Close();
  }

  base::MessageLoop loop_;
  web::TestWebThread ui_thread_;
  web::TestWebThread io_thread_;

  scoped_refptr<web::RequestTrackerImpl> tracker_;
  NSString* request_group_id_;
  web::TestBrowserState browser_state_;
  std::vector<std::unique_ptr<net::URLRequestContext>> contexts_;
  std::vector<std::unique_ptr<net::URLRequest>> requests_;
  net::URLRequestJobFactoryImpl job_factory_;

  GURL GetURL(size_t i) {
    std::stringstream ss;
    ss << "http://www/";
    ss << i;
    return GURL(ss.str());
  }

  GURL GetSecureURL(size_t i) {
    std::stringstream ss;
    ss << "https://www/";
    ss << i;
    return GURL(ss.str());
  }

  net::URLRequest* GetRequest(size_t i) {
    return GetInternalRequest(i, false);
  }

  net::URLRequest* GetSecureRequest(size_t i) {
    return GetInternalRequest(i, true);
  }

  NSString* WaitUntilLoop(bool (^condition)(void)) {
    DCHECK_CURRENTLY_ON(web::WebThread::UI);
    base::Time maxDate = base::Time::Now() + base::TimeDelta::FromSeconds(10);
    while (!condition()) {
      if (base::Time::Now() > maxDate)
        return @"Time is up, too slow to go";
      base::RunLoop().RunUntilIdle();
      base::PlatformThread::Sleep(base::TimeDelta::FromMilliseconds(1));
    }
    return nil;
  }

  void TrimRequest(NSString* tab_id, const GURL& url) {
    DCHECK_CURRENTLY_ON(web::WebThread::UI);
    tracker_->StartPageLoad(url, nil);
  }

  void EndPage(NSString* tab_id, const GURL& url) {
    DCHECK_CURRENTLY_ON(web::WebThread::UI);
    tracker_->FinishPageLoad(url, false);
    base::RunLoop().RunUntilIdle();
  }

  net::TestJobInterceptor* AddInterceptorToRequest(size_t i) {
    // |interceptor| will be deleted from |job_factory_|'s destructor.
    net::TestJobInterceptor* protocol_handler = new net::TestJobInterceptor();
    job_factory_.SetProtocolHandler("http", base::WrapUnique(protocol_handler));
    contexts_[i]->set_job_factory(&job_factory_);
    return protocol_handler;
  }

 private:
  net::URLRequest* GetInternalRequest(size_t i, bool secure) {
    GURL url;
    if (secure)
      url = GetSecureURL(requests_.size());
    else
      url = GetURL(requests_.size());

    while (i >= requests_.size()) {
      contexts_.push_back(std::make_unique<net::URLRequestContext>());
      requests_.push_back(contexts_[i]->CreateRequest(
          url, net::DEFAULT_PRIORITY, &request_delegate_));

      if (secure) {
        // Put a valid SSLInfo inside
        net::HttpResponseInfo* response =
            const_cast<net::HttpResponseInfo*>(&requests_[i]->response_info());

        response->ssl_info.cert = net::ImportCertFromFile(
            net::GetTestCertsDirectory(), "ok_cert.pem");
        response->ssl_info.cert_status = 0;  // No errors.
        response->ssl_info.security_bits = 128;

        EXPECT_TRUE(requests_[i]->ssl_info().is_valid());
      }
    }
    EXPECT_TRUE(!secure == !requests_[i]->url().SchemeIsCryptographic());
    return requests_[i].get();
  }

  DummyURLRequestDelegate request_delegate_;

  DISALLOW_COPY_AND_ASSIGN(RequestTrackerTest);
};

TEST_F(RequestTrackerTest, OnePage) {
  // Start a request.
  tracker_->StartRequest(GetRequest(0));
  // Start page load.
  TrimRequest(request_group_id_, GetURL(0));

  // Stop the request.
  tracker_->StopRequest(GetRequest(0));
  EndPage(request_group_id_, GetURL(0));
}

TEST_F(RequestTrackerTest, OneSecurePage) {
  net::URLRequest* request = GetSecureRequest(0);
  GURL url = GetSecureURL(0);

  // Start a page.
  TrimRequest(request_group_id_, url);

  // Start a request.
  tracker_->StartRequest(request);
  tracker_->CaptureReceivedBytes(request, 42);

  // Stop the request.
  tracker_->StopRequest(request);

  EndPage(request_group_id_, url);
}

TEST_F(RequestTrackerTest, OnePageAndResources) {
  // Start a page.
  TrimRequest(request_group_id_, GetURL(0));
  // Start two requests.
  tracker_->StartRequest(GetRequest(0));
  tracker_->StartRequest(GetRequest(1));

  tracker_->StopRequest(GetRequest(0));
  tracker_->StartRequest(GetRequest(2));
  tracker_->StopRequest(GetRequest(1));
  tracker_->StartRequest(GetRequest(3));

  tracker_->StopRequest(GetRequest(2));
  tracker_->StartRequest(GetRequest(4));

  tracker_->StopRequest(GetRequest(3));
  tracker_->StopRequest(GetRequest(4));
  EndPage(request_group_id_, GetURL(0));
}

TEST_F(RequestTrackerTest, OnePageOneBigImage) {
  // Start a page.
  TrimRequest(request_group_id_, GetURL(0));
  tracker_->StartRequest(GetRequest(0));
  tracker_->StopRequest(GetRequest(0));
  tracker_->StartRequest(GetRequest(1));

  tracker_->CaptureReceivedBytes(GetRequest(1), 10);
  tracker_->CaptureExpectedLength(GetRequest(1), 100);
  tracker_->CaptureReceivedBytes(GetRequest(1), 10);
  tracker_->CaptureReceivedBytes(GetRequest(1), 10);
  tracker_->CaptureReceivedBytes(GetRequest(1), 10);
  tracker_->CaptureReceivedBytes(GetRequest(1), 10);

  tracker_->CaptureReceivedBytes(GetRequest(1), 10);
  tracker_->CaptureReceivedBytes(GetRequest(1), 10);
  tracker_->CaptureReceivedBytes(GetRequest(1), 10);
  tracker_->CaptureReceivedBytes(GetRequest(1), 10);
  tracker_->CaptureReceivedBytes(GetRequest(1), 10);
  tracker_->StopRequest(GetRequest(1));
  EndPage(request_group_id_, GetURL(0));
}

TEST_F(RequestTrackerTest, TwoPagesPostStart) {
  tracker_->StartRequest(GetRequest(0));

  TrimRequest(request_group_id_, GetURL(0));
  tracker_->StartRequest(GetRequest(1));
  tracker_->StartRequest(GetRequest(2));

  tracker_->StopRequest(GetRequest(0));
  tracker_->StopRequest(GetRequest(1));
  tracker_->StopRequest(GetRequest(2));
  EndPage(request_group_id_, GetURL(0));

  tracker_->StartRequest(GetRequest(3));

  TrimRequest(request_group_id_, GetURL(3));

  tracker_->StopRequest(GetRequest(3));
  EndPage(request_group_id_, GetURL(3));
}

TEST_F(RequestTrackerTest, TwoPagesPreStart) {
  tracker_->StartRequest(GetRequest(0));

  TrimRequest(request_group_id_, GetURL(0));
  tracker_->StartRequest(GetRequest(1));
  tracker_->StartRequest(GetRequest(2));

  tracker_->StopRequest(GetRequest(0));
  tracker_->StopRequest(GetRequest(1));
  tracker_->StopRequest(GetRequest(2));
  EndPage(request_group_id_, GetURL(0));

  TrimRequest(request_group_id_, GetURL(3));
  tracker_->StartRequest(GetRequest(3));
  tracker_->StopRequest(GetRequest(3));
  EndPage(request_group_id_, GetURL(3));
}

TEST_F(RequestTrackerTest, TwoPagesNoWait) {
  tracker_->StartRequest(GetRequest(0));

  TrimRequest(request_group_id_, GetURL(0));
  tracker_->StartRequest(GetRequest(1));
  tracker_->StartRequest(GetRequest(2));

  tracker_->StopRequest(GetRequest(0));
  tracker_->StopRequest(GetRequest(1));
  tracker_->StopRequest(GetRequest(2));

  TrimRequest(request_group_id_, GetURL(3));
  tracker_->StartRequest(GetRequest(3));

  tracker_->StopRequest(GetRequest(3));

  EndPage(request_group_id_, GetURL(3));
}

// Do-nothing mock CertificatePolicyCache. Allows all certs for all hosts.
class MockCertificatePolicyCache : public web::CertificatePolicyCache {
 public:
  MockCertificatePolicyCache() {}

  void AllowCertForHost(net::X509Certificate* cert,
                        const std::string& host,
                        net::CertStatus error) override {
  }

  web::CertPolicy::Judgment QueryPolicy(net::X509Certificate* cert,
                                        const std::string& host,
                                        net::CertStatus error) override {
    return web::CertPolicy::Judgment::ALLOWED;
  }

  void ClearCertificatePolicies() override {
  }

 private:
  ~MockCertificatePolicyCache() override {}
};

void TwoStartsSSLCallback(bool* called, bool ok) {
  *called = true;
}

// crbug/386180
TEST_F(RequestTrackerTest, DISABLED_TwoStartsNoEstimate) {
  net::SSLInfo ssl_info;
  ssl_info.cert =
      net::ImportCertFromFile(net::GetTestCertsDirectory(), "ok_cert.pem");
  ssl_info.cert_status = net::CERT_STATUS_AUTHORITY_INVALID;
  scoped_refptr<MockCertificatePolicyCache> cache;
  tracker_->SetCertificatePolicyCacheForTest(cache.get());
  TrimRequest(request_group_id_, GetSecureURL(0));
  tracker_->StartRequest(GetSecureRequest(0));
  tracker_->StartRequest(GetSecureRequest(1));
  bool request_0_called = false;
  bool request_1_called = false;
  tracker_->OnSSLCertificateError(GetSecureRequest(0), ssl_info, true,
                                  base::Bind(&TwoStartsSSLCallback,
                                             &request_0_called));
  tracker_->OnSSLCertificateError(GetSecureRequest(1), ssl_info, true,
                                  base::Bind(&TwoStartsSSLCallback,
                                             &request_1_called));
  EXPECT_TRUE(request_0_called);
  EXPECT_TRUE(request_1_called);
}

}  // namespace
