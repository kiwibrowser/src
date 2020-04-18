// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <memory>
#include <utility>

#include "base/bind.h"
#include "base/command_line.h"
#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/run_loop.h"
#include "base/unguessable_token.h"
#include "content/browser/frame_host/navigation_request_info.h"
#include "content/browser/loader/navigation_url_loader.h"
#include "content/browser/loader/resource_dispatcher_host_impl.h"
#include "content/browser/loader/test_resource_handler.h"
#include "content/browser/loader_delegate_impl.h"
#include "content/common/navigation_params.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/navigation_ui_data.h"
#include "content/public/browser/resource_context.h"
#include "content/public/browser/resource_dispatcher_host_delegate.h"
#include "content/public/common/content_switches.h"
#include "content/public/test/test_browser_context.h"
#include "content/public/test/test_browser_thread_bundle.h"
#include "content/test/test_navigation_url_loader_delegate.h"
#include "net/base/load_flags.h"
#include "net/base/net_errors.h"
#include "net/cert/cert_status_flags.h"
#include "net/http/http_response_headers.h"
#include "net/test/embedded_test_server/embedded_test_server.h"
#include "net/traffic_annotation/network_traffic_annotation_test_helper.h"
#include "net/url_request/redirect_info.h"
#include "net/url_request/url_request.h"
#include "net/url_request/url_request_context.h"
#include "net/url_request/url_request_job_factory_impl.h"
#include "net/url_request/url_request_test_job.h"
#include "net/url_request/url_request_test_util.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/blink/public/mojom/page/page_visibility_state.mojom.h"
#include "url/origin.h"

namespace content {

namespace {

std::unique_ptr<ResourceHandler> CreateTestResourceHandler(
    net::URLRequest* request) {
  return std::make_unique<TestResourceHandler>();
}

}  // namespace

class NavigationURLLoaderTest : public testing::Test {
 public:
  NavigationURLLoaderTest()
      : thread_bundle_(TestBrowserThreadBundle::IO_MAINLOOP),
        browser_context_(new TestBrowserContext),
        host_(base::BindRepeating(&CreateTestResourceHandler),
              base::ThreadTaskRunnerHandle::Get(),
              /* enable_resource_scheduler */ true) {
    host_.SetLoaderDelegate(&loader_delegate_);
    BrowserContext::EnsureResourceContextInitialized(browser_context_.get());
    base::RunLoop().RunUntilIdle();
    net::URLRequestContext* request_context =
        browser_context_->GetResourceContext()->GetRequestContext();
    // Attach URLRequestTestJob.
    job_factory_.SetProtocolHandler(
        "test", net::URLRequestTestJob::CreateProtocolHandler());
    request_context->set_job_factory(&job_factory_);
  }

  std::unique_ptr<NavigationURLLoader> MakeTestLoader(
      const GURL& url,
      NavigationURLLoaderDelegate* delegate) {
    return CreateTestLoader(url, delegate);
  }

  std::unique_ptr<NavigationURLLoader> CreateTestLoader(
      const GURL& url,
      NavigationURLLoaderDelegate* delegate) {
    mojom::BeginNavigationParamsPtr begin_params =
        mojom::BeginNavigationParams::New(
            std::string() /* headers */, net::LOAD_NORMAL,
            false /* skip_service_worker */, REQUEST_CONTEXT_TYPE_LOCATION,
            blink::WebMixedContentContextType::kBlockable,
            false /* is_form_submission */, GURL() /* searchable_form_url */,
            std::string() /* searchable_form_encoding */,
            url::Origin::Create(url), GURL() /* client_side_redirect_url */,
            base::nullopt /* devtools_initiator_info */);
    CommonNavigationParams common_params;
    common_params.url = url;

    std::unique_ptr<NavigationRequestInfo> request_info(
        new NavigationRequestInfo(common_params, std::move(begin_params), url,
                                  true, false, false, -1, false, false, false,
                                  nullptr, base::UnguessableToken::Create()));
    return NavigationURLLoader::Create(
        browser_context_->GetResourceContext(),
        BrowserContext::GetDefaultStoragePartition(browser_context_.get()),
        std::move(request_info), nullptr, nullptr, nullptr, delegate);
  }

  // Helper function for fetching the body of a URL to a string.
  std::string FetchURL(const GURL& url) {
    net::TestDelegate delegate;
    net::URLRequestContext* request_context =
        browser_context_->GetResourceContext()->GetRequestContext();
    std::unique_ptr<net::URLRequest> request(request_context->CreateRequest(
        url, net::DEFAULT_PRIORITY, &delegate, TRAFFIC_ANNOTATION_FOR_TESTS));
    request->Start();
    base::RunLoop().Run();

    EXPECT_TRUE(request->status().is_success());
    EXPECT_EQ(200, request->response_headers()->response_code());
    return delegate.data_received();
  }

 protected:
  TestBrowserThreadBundle thread_bundle_;
  net::URLRequestJobFactoryImpl job_factory_;
  std::unique_ptr<TestBrowserContext> browser_context_;
  LoaderDelegateImpl loader_delegate_;
  ResourceDispatcherHostImpl host_;
};

// Tests that request failures are propagated correctly.
TEST_F(NavigationURLLoaderTest, RequestFailedNoCertError) {
  TestNavigationURLLoaderDelegate delegate;
  std::unique_ptr<NavigationURLLoader> loader =
      MakeTestLoader(GURL("bogus:bogus"), &delegate);

  // Wait for the request to fail as expected.
  delegate.WaitForRequestFailed();
  EXPECT_EQ(net::ERR_ABORTED, delegate.net_error());
  EXPECT_FALSE(delegate.ssl_info().is_valid());
  EXPECT_EQ(1, delegate.on_request_handled_counter());
}

// Tests that request failures are propagated correctly for a (non-fatal) cert
// error:
// - |ssl_info| has the expected values.
TEST_F(NavigationURLLoaderTest, RequestFailedCertError) {
  net::EmbeddedTestServer https_server(net::EmbeddedTestServer::TYPE_HTTPS);
  https_server.SetSSLConfig(net::EmbeddedTestServer::CERT_MISMATCHED_NAME);
  ASSERT_TRUE(https_server.Start());

  TestNavigationURLLoaderDelegate delegate;
  std::unique_ptr<NavigationURLLoader> loader =
      MakeTestLoader(https_server.GetURL("/"), &delegate);

  // Wait for the request to fail as expected.
  delegate.WaitForRequestFailed();
  ASSERT_EQ(net::ERR_ABORTED, delegate.net_error());
  net::SSLInfo ssl_info = delegate.ssl_info();
  EXPECT_TRUE(ssl_info.is_valid());
  EXPECT_TRUE(
      https_server.GetCertificate()->EqualsExcludingChain(ssl_info.cert.get()));
  EXPECT_EQ(net::ERR_CERT_COMMON_NAME_INVALID,
            net::MapCertStatusToNetError(ssl_info.cert_status));
  EXPECT_FALSE(ssl_info.is_fatal_cert_error);
  EXPECT_EQ(1, delegate.on_request_handled_counter());
}

// Tests that request failures are propagated correctly for a fatal cert error:
// - |ssl_info| has the expected values.
TEST_F(NavigationURLLoaderTest, RequestFailedCertErrorFatal) {
  net::EmbeddedTestServer https_server(net::EmbeddedTestServer::TYPE_HTTPS);
  https_server.SetSSLConfig(net::EmbeddedTestServer::CERT_MISMATCHED_NAME);
  ASSERT_TRUE(https_server.Start());
  GURL url = https_server.GetURL("/");

  // Set HSTS for the test domain in order to make SSL errors fatal.
  net::TransportSecurityState* transport_security_state =
      browser_context_->GetResourceContext()
          ->GetRequestContext()
          ->transport_security_state();
  base::Time expiry = base::Time::Now() + base::TimeDelta::FromDays(1000);
  bool include_subdomains = false;
  transport_security_state->AddHSTS(url.host(), expiry, include_subdomains);

  TestNavigationURLLoaderDelegate delegate;
  std::unique_ptr<NavigationURLLoader> loader = MakeTestLoader(url, &delegate);

  // Wait for the request to fail as expected.
  delegate.WaitForRequestFailed();
  ASSERT_EQ(net::ERR_ABORTED, delegate.net_error());
  net::SSLInfo ssl_info = delegate.ssl_info();
  EXPECT_TRUE(ssl_info.is_valid());
  EXPECT_TRUE(
      https_server.GetCertificate()->EqualsExcludingChain(ssl_info.cert.get()));
  EXPECT_EQ(net::ERR_CERT_COMMON_NAME_INVALID,
            net::MapCertStatusToNetError(ssl_info.cert_status));
  EXPECT_TRUE(ssl_info.is_fatal_cert_error);
  EXPECT_EQ(1, delegate.on_request_handled_counter());
}

// Tests that the destroying the loader cancels the request.
TEST_F(NavigationURLLoaderTest, CancelOnDestruct) {
  // Fake a top-level request. Choose a URL which redirects so the request can
  // be paused before the response comes in.
  TestNavigationURLLoaderDelegate delegate;
  std::unique_ptr<NavigationURLLoader> loader = MakeTestLoader(
      net::URLRequestTestJob::test_url_redirect_to_url_2(), &delegate);

  // Wait for the request to redirect.
  delegate.WaitForRequestRedirected();

  // Destroy the loader and verify that URLRequestTestJob no longer has anything
  // paused.
  loader.reset();
  base::RunLoop().RunUntilIdle();
  EXPECT_FALSE(net::URLRequestTestJob::ProcessOnePendingMessage());
}

// Test that the delegate is not called if OnResponseStarted and destroying the
// loader race.
TEST_F(NavigationURLLoaderTest, CancelResponseRace) {
  TestNavigationURLLoaderDelegate delegate;
  std::unique_ptr<NavigationURLLoader> loader = MakeTestLoader(
      net::URLRequestTestJob::test_url_redirect_to_url_2(), &delegate);

  // Wait for the request to redirect.
  delegate.WaitForRequestRedirected();

  // In the same event loop iteration, follow the redirect (allowing the
  // response to go through) and destroy the loader.
  loader->FollowRedirect();
  loader.reset();

  // Verify the URLRequestTestJob no longer has anything paused and that no
  // response body was received.
  base::RunLoop().RunUntilIdle();
  EXPECT_FALSE(net::URLRequestTestJob::ProcessOnePendingMessage());
  EXPECT_FALSE(delegate.has_url_loader_client_endpoints());
}

// Tests that the loader may be canceled by context.
TEST_F(NavigationURLLoaderTest, CancelByContext) {
  TestNavigationURLLoaderDelegate delegate;
  std::unique_ptr<NavigationURLLoader> loader = MakeTestLoader(
      net::URLRequestTestJob::test_url_redirect_to_url_2(), &delegate);

  // Wait for the request to redirect.
  delegate.WaitForRequestRedirected();

  // Cancel all requests.
  host_.CancelRequestsForContext(browser_context_->GetResourceContext());

  // Wait for the request to now be aborted.
  delegate.WaitForRequestFailed();
  EXPECT_EQ(net::ERR_ABORTED, delegate.net_error());
  EXPECT_EQ(1, delegate.on_request_handled_counter());
}

// Tests that the request stays alive as long as the URLLoaderClient endpoints
// are not destructed.
TEST_F(NavigationURLLoaderTest, OwnedByHandle) {
  // Fake a top-level request to a URL whose body does not load immediately.
  TestNavigationURLLoaderDelegate delegate;
  std::unique_ptr<NavigationURLLoader> loader =
      MakeTestLoader(net::URLRequestTestJob::test_url_2(), &delegate);

  // Wait for the response to come back.
  delegate.WaitForResponseStarted();

  // Proceed with the response.
  loader->ProceedWithResponse();

  // Release the URLLoaderClient endpoints.
  delegate.ReleaseURLLoaderClientEndpoints();
  base::RunLoop().RunUntilIdle();

  // Verify that URLRequestTestJob no longer has anything paused.
  EXPECT_FALSE(net::URLRequestTestJob::ProcessOnePendingMessage());
}

}  // namespace content
