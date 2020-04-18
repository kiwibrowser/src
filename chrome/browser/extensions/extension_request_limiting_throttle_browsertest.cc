// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <memory>
#include <utility>

#include "base/bind.h"
#include "base/strings/string_util.h"
#include "base/strings/stringprintf.h"
#include "chrome/browser/extensions/extension_browsertest.h"
#include "chrome/browser/profiles/profile_io_data.h"
#include "chrome/common/chrome_switches.h"
#include "chrome/test/base/ui_test_utils.h"
#include "extensions/browser/extension_throttle_manager.h"
#include "extensions/test/result_catcher.h"
#include "net/base/backoff_entry.h"
#include "net/base/url_util.h"
#include "net/dns/mock_host_resolver.h"
#include "net/test/embedded_test_server/embedded_test_server.h"
#include "net/test/embedded_test_server/http_request.h"
#include "net/test/embedded_test_server/http_response.h"

namespace extensions {

namespace {

std::unique_ptr<net::test_server::HttpResponse> HandleRequest(
    bool set_cache_header_redirect_page,
    bool set_cache_header_test_throttle_page,
    const net::test_server::HttpRequest& request) {
  if (base::StartsWith(request.relative_url, "/redirect",
                       base::CompareCase::SENSITIVE)) {
    std::unique_ptr<net::test_server::BasicHttpResponse> http_response(
        new net::test_server::BasicHttpResponse());
    http_response->set_code(net::HTTP_FOUND);
    http_response->set_content("Redirecting...");
    http_response->set_content_type("text/plain");
    http_response->AddCustomHeader("Location", "/test_throttle");
    if (set_cache_header_redirect_page)
      http_response->AddCustomHeader("Cache-Control", "max-age=3600");
    return std::move(http_response);
  }

  if (base::StartsWith(request.relative_url, "/test_throttle",
                       base::CompareCase::SENSITIVE)) {
    std::unique_ptr<net::test_server::BasicHttpResponse> http_response(
        new net::test_server::BasicHttpResponse());
    http_response->set_code(net::HTTP_SERVICE_UNAVAILABLE);
    http_response->set_content("The server is overloaded right now.");
    http_response->set_content_type("text/plain");
    if (set_cache_header_test_throttle_page)
      http_response->AddCustomHeader("Cache-Control", "max-age=3600");
    return std::move(http_response);
  }

  // Unhandled requests result in the Embedded test server sending a 404.
  return std::unique_ptr<net::test_server::BasicHttpResponse>();
}

}  // namespace

class ExtensionRequestLimitingThrottleBrowserTest
    : public ExtensionBrowserTest {
 public:
  void SetUpOnMainThread() override {
    ExtensionBrowserTest::SetUpOnMainThread();
    ProfileIOData* io_data =
        ProfileIOData::FromResourceContext(profile()->GetResourceContext());
    ExtensionThrottleManager* manager = io_data->GetExtensionThrottleManager();
    if (manager) {
      // Requests issued within within |kUserGestureWindowMs| of a user gesture
      // are also considered as user gestures (see
      // resource_dispatcher_host_impl.cc), so these tests need to bypass the
      // checking of the net::LOAD_MAYBE_USER_GESTURE load flag in the manager
      // in order to test the throttling logic.
      manager->SetIgnoreUserGestureLoadFlagForTests(true);
      std::unique_ptr<net::BackoffEntry::Policy> policy(
          new net::BackoffEntry::Policy{
              // Number of initial errors (in sequence) to ignore before
              // applying
              // exponential back-off rules.
              1,

              // Initial delay for exponential back-off in ms.
              10 * 60 * 1000,

              // Factor by which the waiting time will be multiplied.
              10,

              // Fuzzing percentage. ex: 10% will spread requests randomly
              // between 90%-100% of the calculated time.
              0.1,

              // Maximum amount of time we are willing to delay our request in
              // ms.
              15 * 60 * 1000,

              // Time to keep an entry from being discarded even when it
              // has no significant state, -1 to never discard.
              -1,

              // Don't use initial delay unless the last request was an error.
              false,
          });
      manager->SetBackoffPolicyForTests(std::move(policy));
    }
    // Requests to 127.0.0.1 bypass throttling, so set up a host resolver rule
    // to use a fake domain.
    host_resolver()->AddRule("www.example.com", "127.0.0.1");
    extension_ =
        LoadExtension(test_data_dir_.AppendASCII("extension_throttle"));
    ASSERT_TRUE(extension_);
  }

  void RunTest(const std::string& file_path, const std::string& request_url) {
    ResultCatcher catcher;
    GURL test_url = net::AppendQueryParameter(
        extension_->GetResourceURL(file_path), "url", request_url);
    ui_test_utils::NavigateToURL(browser(), test_url);
    ASSERT_TRUE(catcher.GetNextResult());
  }

 private:
  const Extension* extension_;
};

class ExtensionRequestLimitingThrottleCommandLineBrowserTest
    : public ExtensionRequestLimitingThrottleBrowserTest {
 public:
  void SetUpCommandLine(base::CommandLine* command_line) override {
    ExtensionRequestLimitingThrottleBrowserTest::SetUpCommandLine(command_line);
    command_line->AppendSwitch(switches::kDisableExtensionsHttpThrottling);
  }
};

// Tests that if the same URL is requested repeatedly by an extension, it will
// eventually be throttled.
IN_PROC_BROWSER_TEST_F(ExtensionRequestLimitingThrottleBrowserTest,
                       ThrottleRequest) {
  embedded_test_server()->RegisterRequestHandler(
      base::Bind(&HandleRequest, false, false));
  ASSERT_TRUE(embedded_test_server()->Start());
  ASSERT_NO_FATAL_FAILURE(
      RunTest("test_request_eventually_throttled.html",
              base::StringPrintf("http://www.example.com:%d/test_throttle",
                                 embedded_test_server()->port())));
}

// Tests that if the same URL is repeatedly requested by an extension, and the
// response is served from the cache, it will not be throttled.
IN_PROC_BROWSER_TEST_F(ExtensionRequestLimitingThrottleBrowserTest,
                       DoNotThrottleCachedResponse) {
  embedded_test_server()->RegisterRequestHandler(
      base::Bind(&HandleRequest, false, true));
  ASSERT_TRUE(embedded_test_server()->Start());
  ASSERT_NO_FATAL_FAILURE(
      RunTest("test_request_not_throttled.html",
              base::StringPrintf("http://www.example.com:%d/test_throttle",
                                 embedded_test_server()->port())));
}

#if defined(OS_CHROMEOS)
// Flaky: https://crbug.com/836188.
#define MAYBE_ThrottleRequest_Redirect DISABLED_ThrottleRequest_Redirect
#else
#define MAYBE_ThrottleRequest_Redirect ThrottleRequest_Redirect
#endif
// Tests that the redirected request is also being throttled.
IN_PROC_BROWSER_TEST_F(ExtensionRequestLimitingThrottleBrowserTest,
                       MAYBE_ThrottleRequest_Redirect) {
  embedded_test_server()->RegisterRequestHandler(
      base::Bind(&HandleRequest, false, false));
  ASSERT_TRUE(embedded_test_server()->Start());
  // Issue a bunch of requests to a url which gets redirected to a new url that
  // generates 503.
  ASSERT_NO_FATAL_FAILURE(
      RunTest("test_request_eventually_throttled.html",
              base::StringPrintf("http://www.example.com:%d/redirect",
                                 embedded_test_server()->port())));

  // Now requests to both URLs should be throttled. Explicitly validate that the
  // second URL is throttled.
  ASSERT_NO_FATAL_FAILURE(
      RunTest("test_request_throttled_on_first_try.html",
              base::StringPrintf("http://www.example.com:%d/test_throttle",
                                 embedded_test_server()->port())));
}

// Tests that if both redirect (302) and non-redirect (503) responses are
// served from cache, the extension throttle does not throttle the request.
IN_PROC_BROWSER_TEST_F(ExtensionRequestLimitingThrottleBrowserTest,
                       DoNotThrottleCachedResponse_Redirect) {
  embedded_test_server()->RegisterRequestHandler(
      base::Bind(&HandleRequest, true, true));
  ASSERT_TRUE(embedded_test_server()->Start());
  ASSERT_NO_FATAL_FAILURE(
      RunTest("test_request_not_throttled.html",
              base::StringPrintf("http://www.example.com:%d/redirect",
                                 embedded_test_server()->port())));
}

#if defined(OS_CHROMEOS)
// Flaky: https://crbug.com/836188.
#define MAYBE_ThrottleRequest_RedirectCached \
  DISABLED_ThrottleRequest_RedirectCached
#else
#define MAYBE_ThrottleRequest_RedirectCached ThrottleRequest_RedirectCached
#endif
// Tests that if the redirect (302) is served from cache, but the non-redirect
// (503) is not, the extension throttle throttles the requests for the second
// url.
IN_PROC_BROWSER_TEST_F(ExtensionRequestLimitingThrottleBrowserTest,
                       MAYBE_ThrottleRequest_RedirectCached) {
  embedded_test_server()->RegisterRequestHandler(
      base::Bind(&HandleRequest, true, false));
  ASSERT_TRUE(embedded_test_server()->Start());
  ASSERT_NO_FATAL_FAILURE(
      RunTest("test_request_eventually_throttled.html",
              base::StringPrintf("http://www.example.com:%d/redirect",
                                 embedded_test_server()->port())));

  // Explicitly validate that the second URL is throttled.
  ASSERT_NO_FATAL_FAILURE(
      RunTest("test_request_throttled_on_first_try.html",
              base::StringPrintf("http://www.example.com:%d/test_throttle",
                                 embedded_test_server()->port())));
}

// Tests that if the redirect (302) is not served from cache, but the
// non-redirect (503) is, the extension throttle only throttles requests to the
// redirect URL.
IN_PROC_BROWSER_TEST_F(ExtensionRequestLimitingThrottleBrowserTest,
                       DoNotThrottleCachedResponse_NonRedirectCached) {
  embedded_test_server()->RegisterRequestHandler(
      base::Bind(&HandleRequest, false, true));
  ASSERT_TRUE(embedded_test_server()->Start());
  ASSERT_NO_FATAL_FAILURE(
      RunTest("test_request_not_throttled.html",
              base::StringPrintf("http://www.example.com:%d/redirect",
                                 embedded_test_server()->port())));

  // Explicitly validate that the second URL is not throttled.
  ASSERT_NO_FATAL_FAILURE(
      RunTest("test_request_not_throttled.html",
              base::StringPrintf("http://www.example.com:%d/test_throttle",
                                 embedded_test_server()->port())));
}

// Tests that if switches::kDisableExtensionsHttpThrottling is set on the
// command line, throttling is disabled.
IN_PROC_BROWSER_TEST_F(ExtensionRequestLimitingThrottleCommandLineBrowserTest,
                       ThrottleRequestDisabled) {
  embedded_test_server()->RegisterRequestHandler(
      base::Bind(&HandleRequest, false, false));
  ASSERT_TRUE(embedded_test_server()->Start());
  ASSERT_NO_FATAL_FAILURE(
      RunTest("test_request_not_throttled.html",
              base::StringPrintf("http://www.example.com:%d/test_throttle",
                                 embedded_test_server()->port())));
}

}  // namespace extensions
