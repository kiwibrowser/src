// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <memory>

#include "base/bind.h"
#include "base/callback.h"
#include "base/run_loop.h"
#include "base/test/scoped_feature_list.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ssl/cert_verifier_browser_test.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/common/chrome_features.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "content/public/browser/browser_thread.h"
#include "net/cert/mock_cert_verifier.h"
#include "net/http/transport_security_state.h"
#include "net/test/embedded_test_server/embedded_test_server.h"
#include "net/test/embedded_test_server/http_request.h"
#include "net/test/embedded_test_server/http_response.h"
#include "net/url_request/url_request_context.h"
#include "net/url_request/url_request_context_getter.h"

namespace {

// A test fixture that allows tests to wait for an Expect-CT report to be
// received by a server.
class ExpectCTBrowserTest : public CertVerifierBrowserTest {
 public:
  ExpectCTBrowserTest() : CertVerifierBrowserTest() {}

  void SetUpOnMainThread() override {
    run_loop_ = std::make_unique<base::RunLoop>();
  }

  void TearDown() override { run_loop_.reset(nullptr); }

  std::unique_ptr<net::test_server::HttpResponse> ExpectCTHeaderRequestHandler(
      const net::test_server::HttpRequest& request) {
    std::unique_ptr<net::test_server::BasicHttpResponse> http_response(
        new net::test_server::BasicHttpResponse());
    http_response->set_code(net::HTTP_OK);
    http_response->AddCustomHeader(
        "Expect-CT", "max-age=100, report-uri=" + report_uri_.spec());
    return http_response;
  }

  std::unique_ptr<net::test_server::HttpResponse> ReportRequestHandler(
      const net::test_server::HttpRequest& request) {
    EXPECT_TRUE(request.method == net::test_server::METHOD_POST ||
                request.method == net::test_server::METHOD_OPTIONS)
        << "Request method must be POST or OPTIONS. It is " << request.method
        << ".";
    std::unique_ptr<net::test_server::BasicHttpResponse> http_response(
        new net::test_server::BasicHttpResponse());
    http_response->set_code(net::HTTP_OK);

    // Respond properly to CORS preflights.
    if (request.method == net::test_server::METHOD_OPTIONS) {
      http_response->AddCustomHeader("Access-Control-Allow-Origin", "*");
      http_response->AddCustomHeader("Access-Control-Allow-Methods", "POST");
      http_response->AddCustomHeader("Access-Control-Allow-Headers",
                                     "content-type");
    } else if (request.method == net::test_server::METHOD_POST) {
      auto it = request.headers.find("Content-Type");
      EXPECT_NE(it, request.headers.end());
      // The above EXPECT_NE is really an ASSERT_NE in spirit, but can't ASSERT
      // because a response must be returned.
      if (it != request.headers.end()) {
        EXPECT_EQ("application/expect-ct-report+json; charset=utf-8",
                  it->second);
      }
      run_loop_->QuitClosure().Run();
    }

    return http_response;
  }

 protected:
  void WaitForReport() { run_loop_->Run(); }

  // Set the report-uri value to be used in the Expect-CT header for requests
  // handled by ExpectCTHeaderRequestHandler.
  void set_report_uri(const GURL& report_uri) { report_uri_ = report_uri; }

 private:
  std::unique_ptr<base::RunLoop> run_loop_;
  // The report-uri value to use in the Expect-CT header for requests handled by
  // ExpectCTHeaderRequestHandler.
  GURL report_uri_;

  DISALLOW_COPY_AND_ASSIGN(ExpectCTBrowserTest);
};

void AddExpectCTHeaderOnIO(net::URLRequestContextGetter* getter,
                           const std::string& host,
                           const GURL& report_uri) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::IO);
  net::URLRequestContext* context = getter->GetURLRequestContext();
  context->transport_security_state()->AddExpectCT(
      host, base::Time::Now() + base::TimeDelta::FromSeconds(1000), true,
      report_uri);
}

// Tests that an Expect-CT reporter is properly set up and used for violations
// of Expect-CT HTTP headers.
IN_PROC_BROWSER_TEST_F(ExpectCTBrowserTest, TestDynamicExpectCTReporting) {
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitWithFeatures(
      {features::kExpectCTReporting,
       net::TransportSecurityState::kDynamicExpectCTFeature},
      {});

  net::EmbeddedTestServer test_server(net::EmbeddedTestServer::TYPE_HTTPS);
  ASSERT_TRUE(test_server.Start());

  net::EmbeddedTestServer report_server;
  report_server.RegisterRequestHandler(base::Bind(
      &ExpectCTBrowserTest::ReportRequestHandler, base::Unretained(this)));
  ASSERT_TRUE(report_server.Start());

  // Set up the mock cert verifier to accept |test_server|'s certificate as
  // valid and as if it is issued by a known root. (CT checks are skipped for
  // private roots.)
  scoped_refptr<net::X509Certificate> cert(test_server.GetCertificate());
  net::CertVerifyResult verify_result;
  verify_result.is_issued_by_known_root = true;
  verify_result.verified_cert = cert;
  verify_result.cert_status = 0;
  mock_cert_verifier()->AddResultForCert(cert, verify_result, net::OK);

  // Fire off a task to simulate as if a previous request to |test_server| had
  // set a valid Expect-CT header.
  scoped_refptr<net::URLRequestContextGetter> url_request_context_getter =
      browser()->profile()->GetRequestContext();
  content::BrowserThread::PostTask(
      content::BrowserThread::IO, FROM_HERE,
      base::BindOnce(
          &AddExpectCTHeaderOnIO, base::RetainedRef(url_request_context_getter),
          test_server.GetURL("/").host(), report_server.GetURL("/")));

  // Navigate to a test server URL, which should trigger an Expect-CT report
  // because the test server doesn't serve SCTs.
  ui_test_utils::NavigateToURL(browser(), test_server.GetURL("/"));
  WaitForReport();
  // WaitForReport() does not return util ReportRequestHandler runs, and the
  // handler does all the assertions, so there are no more assertions needed
  // here.
}

// Tests that Expect-CT HTTP headers are processed correctly.
IN_PROC_BROWSER_TEST_F(ExpectCTBrowserTest,
                       TestDynamicExpectCTHeaderProcessing) {
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitWithFeatures(
      {features::kExpectCTReporting,
       net::TransportSecurityState::kDynamicExpectCTFeature},
      {});

  net::EmbeddedTestServer test_server(net::EmbeddedTestServer::TYPE_HTTPS);
  test_server.RegisterRequestHandler(
      base::Bind(&ExpectCTBrowserTest::ExpectCTHeaderRequestHandler,
                 base::Unretained(this)));
  ASSERT_TRUE(test_server.Start());

  net::EmbeddedTestServer report_server;
  report_server.RegisterRequestHandler(base::Bind(
      &ExpectCTBrowserTest::ReportRequestHandler, base::Unretained(this)));
  ASSERT_TRUE(report_server.Start());

  // Set up ExpectCTRequestHandler() to set Expect-CT headers that report to
  // |report_server|.
  set_report_uri(report_server.GetURL("/"));

  // Set up the mock cert verifier to accept |test_server|'s certificate as
  // valid and as if it is issued by a known root. (CT checks are skipped for
  // private roots.)
  scoped_refptr<net::X509Certificate> cert(test_server.GetCertificate());
  net::CertVerifyResult verify_result;
  verify_result.is_issued_by_known_root = true;
  verify_result.verified_cert = cert;
  verify_result.cert_status = 0;
  mock_cert_verifier()->AddResultForCert(cert, verify_result, net::OK);

  // Navigate to a test server URL, whose header should trigger an Expect-CT
  // report because the test server doesn't serve SCTs.
  ui_test_utils::NavigateToURL(browser(), test_server.GetURL("/"));
  WaitForReport();
  // WaitForReport() does not return util ReportRequestHandler runs, and the
  // handler does all the assertions, so there are no more assertions needed
  // here.
}

}  // namespace
