// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ssl/ssl_error_navigation_throttle.h"

#include "base/bind.h"
#include "base/command_line.h"
#include "base/run_loop.h"
#include "base/threading/thread_task_runner_handle.h"
#include "chrome/browser/ssl/certificate_reporting_test_utils.cc"
#include "chrome/browser/ssl/ssl_blocking_page.h"
#include "chrome/common/chrome_switches.h"
#include "chrome/test/base/chrome_render_view_host_test_harness.h"
#include "content/public/browser/navigation_handle.h"
#include "content/public/browser/navigation_throttle.h"
#include "net/cert/cert_status_flags.h"
#include "net/test/cert_test_util.h"
#include "net/test/test_data_directory.h"

namespace {

// Replacement for SSLErrorHandler::HandleSSLError that calls
// |blocking_page_ready_callback|. |async| specifies whether this call should be
// done synchronously or using PostTask().
void MockHandleSSLError(
    bool async,
    content::WebContents* web_contents,
    int cert_error,
    const net::SSLInfo& ssl_info,
    const GURL& request_url,
    bool expired_previous_decision,
    std::unique_ptr<SSLCertReporter> ssl_cert_reporter,
    const base::Callback<void(content::CertificateRequestResultType)>&
        decision_callback,
    base::OnceCallback<
        void(std::unique_ptr<security_interstitials::SecurityInterstitialPage>)>
        blocking_page_ready_callback) {
  std::unique_ptr<SSLBlockingPage> blocking_page(SSLBlockingPage::Create(
      web_contents, cert_error, ssl_info, request_url, 0,
      base::Time::NowFromSystemTime(), GURL(), nullptr,
      false /* is superfish */, decision_callback));
  if (async) {
    base::ThreadTaskRunnerHandle::Get()->PostTask(
        FROM_HERE, base::BindOnce(std::move(blocking_page_ready_callback),
                                  std::move(blocking_page)));
  } else {
    std::move(blocking_page_ready_callback).Run(std::move(blocking_page));
  }

}  // namespace

class TestSSLErrorNavigationThrottle : public SSLErrorNavigationThrottle {
 public:
  TestSSLErrorNavigationThrottle(
      content::NavigationHandle* handle,
      bool async_handle_ssl_error,
      base::OnceCallback<void(content::NavigationThrottle::ThrottleCheckResult)>
          on_cancel_deferred_navigation)
      : SSLErrorNavigationThrottle(
            handle,
            certificate_reporting_test_utils::CreateMockSSLCertReporter(
                base::Callback<void(const std::string&,
                                    const chrome_browser_ssl::
                                        CertLoggerRequest_ChromeChannel)>(),
                certificate_reporting_test_utils::CERT_REPORT_NOT_EXPECTED),
            base::Bind(&MockHandleSSLError, async_handle_ssl_error)),
        on_cancel_deferred_navigation_(
            std::move(on_cancel_deferred_navigation)) {}

  // NavigationThrottle:
  void CancelDeferredNavigation(
      content::NavigationThrottle::ThrottleCheckResult result) override {
    std::move(on_cancel_deferred_navigation_).Run(result);
  }

 private:
  base::OnceCallback<void(content::NavigationThrottle::ThrottleCheckResult)>
      on_cancel_deferred_navigation_;

  DISALLOW_COPY_AND_ASSIGN(TestSSLErrorNavigationThrottle);
};

class SSLErrorNavigationThrottleTest
    : public ChromeRenderViewHostTestHarness,
      public testing::WithParamInterface<bool> {
 public:
  SSLErrorNavigationThrottleTest() {}
  void SetUp() override {
    ChromeRenderViewHostTestHarness::SetUp();
    base::CommandLine::ForCurrentProcess()->AppendSwitch(
        switches::kCommittedInterstitials);

    async_ = GetParam();
    handle_ = content::NavigationHandle::CreateNavigationHandleForTesting(
        GURL(), main_rfh(), true /* committed */, net::ERR_CERT_INVALID);

    std::unique_ptr<TestSSLErrorNavigationThrottle> throttle =
        std::make_unique<TestSSLErrorNavigationThrottle>(
            handle_.get(), async_,
            base::BindOnce(
                &SSLErrorNavigationThrottleTest::RecordDeferredResult,
                base::Unretained(this)));
    handle_->RegisterThrottleForTesting(std::move(throttle));
  }

  // content::RenderViewHostTestHarness:
  void TearDown() override {
    handle_.reset();
    ChromeRenderViewHostTestHarness::TearDown();
  }

  void RecordDeferredResult(
      content::NavigationThrottle::ThrottleCheckResult result) {
    deferred_result_ = result;
  }

 protected:
  bool async_ = false;
  std::unique_ptr<content::NavigationHandle> handle_;
  content::NavigationThrottle::ThrottleCheckResult deferred_result_ =
      content::NavigationThrottle::DEFER;

  DISALLOW_COPY_AND_ASSIGN(SSLErrorNavigationThrottleTest);
};

// Tests that the throttle ignores a request without SSL info.
TEST_P(SSLErrorNavigationThrottleTest, NoSSLInfo) {
  SCOPED_TRACE(::testing::Message()
               << "Asynchronous MockHandleSSLError: " << async_);

  content::NavigationThrottle::ThrottleCheckResult result =
      handle_->CallWillFailRequestForTesting(main_rfh(), base::nullopt);

  EXPECT_FALSE(handle_->GetSSLInfo().is_valid());
  EXPECT_EQ(content::NavigationThrottle::PROCEED, result);
}

// Tests that the throttle ignores a request with a cert status that is not an
// cert error.
TEST_P(SSLErrorNavigationThrottleTest, SSLInfoWithoutCertError) {
  SCOPED_TRACE(::testing::Message()
               << "Asynchronous MockHandleSSLError: " << async_);

  net::SSLInfo ssl_info;
  ssl_info.cert_status = net::CERT_STATUS_IS_EV;
  content::NavigationThrottle::ThrottleCheckResult result =
      handle_->CallWillFailRequestForTesting(main_rfh(), ssl_info);

  EXPECT_EQ(net::CERT_STATUS_IS_EV, handle_->GetSSLInfo().cert_status);
  EXPECT_EQ(content::NavigationThrottle::PROCEED, result);
}

// Tests that the throttle defers and cancels a request with a cert status that
// is a cert error.
TEST_P(SSLErrorNavigationThrottleTest, SSLInfoWithCertError) {
  SCOPED_TRACE(::testing::Message()
               << "Asynchronous MockHandleSSLError: " << async_);

  net::SSLInfo ssl_info;
  ssl_info.cert =
      net::ImportCertFromFile(net::GetTestCertsDirectory(), "ok_cert.pem");
  ssl_info.cert_status = net::CERT_STATUS_COMMON_NAME_INVALID;
  content::NavigationThrottle::ThrottleCheckResult synchronous_result =
      handle_->CallWillFailRequestForTesting(main_rfh(), ssl_info);

  EXPECT_EQ(content::NavigationThrottle::DEFER, synchronous_result.action());
  base::RunLoop().RunUntilIdle();
  EXPECT_EQ(content::NavigationThrottle::CANCEL, deferred_result_.action());
  EXPECT_EQ(net::ERR_CERT_COMMON_NAME_INVALID,
            deferred_result_.net_error_code());
  EXPECT_TRUE(deferred_result_.error_page_content().has_value());
}

INSTANTIATE_TEST_CASE_P(,
                        SSLErrorNavigationThrottleTest,
                        ::testing::Values(false, true));

}  // namespace
