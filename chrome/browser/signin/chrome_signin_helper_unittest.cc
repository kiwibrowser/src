// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/signin/chrome_signin_helper.h"

#include <memory>
#include <string>

#include "base/run_loop.h"
#include "base/strings/stringprintf.h"
#include "components/signin/core/browser/profile_management_switches.h"
#include "components/signin/core/browser/scoped_account_consistency.h"
#include "components/signin/core/browser/signin_buildflags.h"
#include "content/public/browser/resource_request_info.h"
#include "content/public/test/test_browser_thread_bundle.h"
#include "net/http/http_response_headers.h"
#include "net/traffic_annotation/network_traffic_annotation_test_helper.h"
#include "net/url_request/url_request.h"
#include "net/url_request/url_request_context.h"
#include "net/url_request/url_request_filter.h"
#include "net/url_request/url_request_interceptor.h"
#include "net/url_request/url_request_test_job.h"
#include "net/url_request/url_request_test_util.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

namespace {

#if BUILDFLAG(ENABLE_DICE_SUPPORT)

const GURL kGaiaUrl("https://accounts.google.com");
const char kDiceResponseHeader[] = "X-Chrome-ID-Consistency-Response";

// URLRequestInterceptor adding a Dice response header to Gaia responses.
class TestRequestInterceptor : public net::URLRequestInterceptor {
 public:
  ~TestRequestInterceptor() override {}

 private:
  net::URLRequestJob* MaybeInterceptRequest(
      net::URLRequest* request,
      net::NetworkDelegate* network_delegate) const override {
    std::string response_headers =
        base::StringPrintf("HTTP/1.1 200 OK\n\n%s: Foo\n", kDiceResponseHeader);
    return new net::URLRequestTestJob(request, network_delegate,
                                      response_headers, "", true);
  }
};

#endif  // BUILDFLAG(ENABLE_DICE_SUPPORT)

}  // namespace

class ChromeSigninHelperTest : public testing::Test {
 protected:
  ChromeSigninHelperTest()
      : thread_bundle_(content::TestBrowserThreadBundle::IO_MAINLOOP) {}

  ~ChromeSigninHelperTest() override {}

  content::TestBrowserThreadBundle thread_bundle_;
  net::TestURLRequestContext url_request_context_;
  std::unique_ptr<net::TestDelegate> test_request_delegate_;
  std::unique_ptr<net::URLRequest> request_;
};

#if BUILDFLAG(ENABLE_DICE_SUPPORT)
// Tests that Dice response headers are removed after being processed.
TEST_F(ChromeSigninHelperTest, RemoveDiceSigninHeader) {
  signin::ScopedAccountConsistencyDiceFixAuthErrors scoped_dice_fix_auth_errors;

  // Create a response with the Dice header.
  test_request_delegate_ = std::make_unique<net::TestDelegate>();
  request_ = url_request_context_.CreateRequest(kGaiaUrl, net::DEFAULT_PRIORITY,
                                                test_request_delegate_.get(),
                                                TRAFFIC_ANNOTATION_FOR_TESTS);
  content::ResourceRequestInfo::AllocateForTesting(
      request_.get(), content::RESOURCE_TYPE_MAIN_FRAME, nullptr, -1, -1, -1,
      true, false, true, content::PREVIEWS_OFF, nullptr);
  net::URLRequestFilter::GetInstance()->AddUrlInterceptor(
      kGaiaUrl, std::make_unique<TestRequestInterceptor>());
  request_->Start();
  base::RunLoop().RunUntilIdle();
  net::URLRequestFilter::GetInstance()->RemoveUrlHandler(kGaiaUrl);

  // Check that the Dice response header is correctly set.
  net::HttpResponseHeaders* response_headers = request_->response_headers();
  ASSERT_TRUE(response_headers);
  ASSERT_TRUE(request_->response_headers()->HasHeader(kDiceResponseHeader));

  // Process the header.
  signin::ProcessAccountConsistencyResponseHeaders(request_.get(), GURL(),
                                                   false /* is_incognito */);

  // Check that the header has been removed.
  EXPECT_FALSE(request_->response_headers()->HasHeader(kDiceResponseHeader));
}
#endif  // BUILDFLAG(ENABLE_DICE_SUPPORT)
