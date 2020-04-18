// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ssl/typed_navigation_timing_throttle.h"

#include "base/test/histogram_tester.h"
#include "chrome/test/base/chrome_render_view_host_test_harness.h"
#include "content/public/browser/navigation_handle.h"
#include "content/public/browser/navigation_throttle.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

class TypedNavigationTimingThrottleTest
    : public ChromeRenderViewHostTestHarness {};

// Tests that the throttle is created for navigations to HTTP URLs.
TEST_F(TypedNavigationTimingThrottleTest, IsCreatedForHTTP) {
  GURL http_url("http://example.test");
  std::unique_ptr<content::NavigationHandle> handle =
      content::NavigationHandle::CreateNavigationHandleForTesting(http_url,
                                                                  main_rfh());
  std::unique_ptr<content::NavigationThrottle> throttle =
      TypedNavigationTimingThrottle::MaybeCreateThrottleFor(handle.get());
  EXPECT_TRUE(throttle);
}

// Tests that the throttle is not created if the URL is HTTPS.
TEST_F(TypedNavigationTimingThrottleTest, NotCreatedForHTTPS) {
  GURL https_url("https://example.test");
  std::unique_ptr<content::NavigationHandle> handle =
      content::NavigationHandle::CreateNavigationHandleForTesting(
          https_url, main_rfh(), false, /* committed */
          net::OK,                      /* error */
          false,                        /* is_same_document */
          false,                        /* is_post */
          ui::PAGE_TRANSITION_TYPED);
  std::unique_ptr<content::NavigationThrottle> throttle =
      TypedNavigationTimingThrottle::MaybeCreateThrottleFor(handle.get());
  EXPECT_FALSE(throttle);
}

// Tests that the timing histogram is correctly updated.
TEST_F(TypedNavigationTimingThrottleTest, URLUpgraded) {
  base::HistogramTester test;

  GURL http_url("http://example.test");
  std::unique_ptr<content::NavigationHandle> handle =
      content::NavigationHandle::CreateNavigationHandleForTesting(
          http_url, main_rfh(), false, /* committed */
          net::OK,                     /* error */
          false,                       /* is_same_document */
          false,                       /* is_post */
          ui::PAGE_TRANSITION_TYPED);

  EXPECT_EQ(content::NavigationThrottle::PROCEED,
            handle->CallWillStartRequestForTesting().action());

  GURL https_url("https://example.test");
  EXPECT_EQ(content::NavigationThrottle::PROCEED,
            handle
                ->CallWillRedirectRequestForTesting(
                    https_url, false, /* new_method_is_post */
                    http_url,         /* new_referrer_url */
                    false /* new_is_external_protocol */)
                .action());

  test.ExpectTotalCount("Omnibox.URLNavigationTimeToRedirectToHTTPS", 1);
}

// Tests that the histogram is not updated if the URL was never upgraded.
TEST_F(TypedNavigationTimingThrottleTest, URLNotUpgraded) {
  base::HistogramTester test;

  GURL http_url("http://example.test");
  std::unique_ptr<content::NavigationHandle> handle =
      content::NavigationHandle::CreateNavigationHandleForTesting(
          http_url, main_rfh(), false, /* committed */
          net::OK,                     /* error */
          false,                       /* is_same_document */
          false,                       /* is_post */
          ui::PAGE_TRANSITION_TYPED);

  EXPECT_EQ(content::NavigationThrottle::PROCEED,
            handle->CallWillStartRequestForTesting().action());

  test.ExpectTotalCount("Omnibox.URLNavigationTimeToRedirectToHTTPS", 0);
}

// Tests that the histogram is not updated if the URL is redirected to a
// different URL (not an HTTPS upgrade).
TEST_F(TypedNavigationTimingThrottleTest, NonHTTPSRedirect) {
  base::HistogramTester test;

  GURL http_url("http://example.test");
  std::unique_ptr<content::NavigationHandle> handle =
      content::NavigationHandle::CreateNavigationHandleForTesting(
          http_url, main_rfh(), false, /* committed */
          net::OK,                     /* error */
          false,                       /* is_same_document */
          false,                       /* is_post */
          ui::PAGE_TRANSITION_TYPED);

  EXPECT_EQ(content::NavigationThrottle::PROCEED,
            handle->CallWillStartRequestForTesting().action());

  GURL other_url("http://nonexample.test");
  EXPECT_EQ(content::NavigationThrottle::PROCEED,
            handle
                ->CallWillRedirectRequestForTesting(
                    other_url, false, /* new_method_is_post */
                    http_url,         /* new_referrer_url */
                    false /* new_is_external_protocol */)
                .action());

  test.ExpectTotalCount("Omnibox.URLNavigationTimeToRedirectToHTTPS", 0);
}

// Tests that the histogram is not updated if the URL is redirected to a
// different URL _with_ HTTPS.
TEST_F(TypedNavigationTimingThrottleTest, CrossSiteHTTPSRedirect) {
  base::HistogramTester test;

  GURL http_url("http://example.test");
  std::unique_ptr<content::NavigationHandle> handle =
      content::NavigationHandle::CreateNavigationHandleForTesting(
          http_url, main_rfh(), false, /* committed */
          net::OK,                     /* error */
          false,                       /* is_same_document */
          false,                       /* is_post */
          ui::PAGE_TRANSITION_TYPED);

  EXPECT_EQ(content::NavigationThrottle::PROCEED,
            handle->CallWillStartRequestForTesting().action());

  GURL other_url("https://nonexample.test");
  EXPECT_EQ(content::NavigationThrottle::PROCEED,
            handle
                ->CallWillRedirectRequestForTesting(
                    other_url, false, /* new_method_is_post */
                    http_url,         /* new_referrer_url */
                    false /* new_is_external_protocol */)
                .action());

  test.ExpectTotalCount("Omnibox.URLNavigationTimeToRedirectToHTTPS", 0);
}

// Tests that the histogram is not updated if the navigation isn't of type
// PAGE_TRANSITION_TYPED.
TEST_F(TypedNavigationTimingThrottleTest, NonTypedNavigation) {
  base::HistogramTester test;

  GURL http_url("http://example.test");
  std::unique_ptr<content::NavigationHandle> handle =
      content::NavigationHandle::CreateNavigationHandleForTesting(http_url,
                                                                  main_rfh());

  EXPECT_EQ(content::NavigationThrottle::PROCEED,
            handle->CallWillStartRequestForTesting().action());

  GURL https_url("https://example.test");
  EXPECT_EQ(content::NavigationThrottle::PROCEED,
            handle
                ->CallWillRedirectRequestForTesting(
                    https_url, false, /* new_method_is_post */
                    http_url,         /* new_referrer_url */
                    false /* new_is_external_protocol */)
                .action());

  test.ExpectTotalCount("Omnibox.URLNavigationTimeToRedirectToHTTPS", 0);
}

// Tests that the histogram is updated only once for a long chain of redirects
// which include the same-URL HTTP->HTTPS upgrade at the beginning.
TEST_F(TypedNavigationTimingThrottleTest, ManyRedirects) {
  base::HistogramTester test;

  GURL http_url("http://example.test");
  std::unique_ptr<content::NavigationHandle> handle =
      content::NavigationHandle::CreateNavigationHandleForTesting(
          http_url, main_rfh(), false, /* committed */
          net::OK,                     /* error */
          false,                       /* is_same_document */
          false,                       /* is_post */
          ui::PAGE_TRANSITION_TYPED);

  EXPECT_EQ(content::NavigationThrottle::PROCEED,
            handle->CallWillStartRequestForTesting().action());

  // Redirecting to the "upgraded" URL twice in a row should result in only one
  // activation of the histogram trigger.
  GURL https_url("https://example.test");
  EXPECT_EQ(content::NavigationThrottle::PROCEED,
            handle
                ->CallWillRedirectRequestForTesting(
                    https_url, false, /* new_method_is_post */
                    http_url,         /* new_referrer_url */
                    false /* new_is_external_protocol */)
                .action());
  EXPECT_EQ(content::NavigationThrottle::PROCEED,
            handle
                ->CallWillRedirectRequestForTesting(
                    https_url, false, /* new_method_is_post */
                    http_url,         /* new_referrer_url */
                    false /* new_is_external_protocol */)
                .action());

  // Other redirects should also not affect the histogram.
  GURL other_http_url("http://nonexample.test");
  EXPECT_EQ(content::NavigationThrottle::PROCEED,
            handle
                ->CallWillRedirectRequestForTesting(
                    other_http_url, false, /* new_method_is_post */
                    https_url,             /* new_referrer_url */
                    false /* new_is_external_protocol */)
                .action());

  GURL other_https_url("https://nonexample.test");
  EXPECT_EQ(content::NavigationThrottle::PROCEED,
            handle
                ->CallWillRedirectRequestForTesting(
                    other_https_url, false, /* new_method_is_post */
                    other_http_url,         /* new_referrer_url */
                    false /* new_is_external_protocol */)
                .action());

  test.ExpectTotalCount("Omnibox.URLNavigationTimeToRedirectToHTTPS", 1);
}

// Tests that the histogram is updated when the new URL adds "www.".
TEST_F(TypedNavigationTimingThrottleTest, AddWWW) {
  base::HistogramTester test;

  GURL http_url("http://example.test");
  std::unique_ptr<content::NavigationHandle> handle =
      content::NavigationHandle::CreateNavigationHandleForTesting(
          http_url, main_rfh(), false, /* committed */
          net::OK,                     /* error */
          false,                       /* is_same_document */
          false,                       /* is_post */
          ui::PAGE_TRANSITION_TYPED);

  EXPECT_EQ(content::NavigationThrottle::PROCEED,
            handle->CallWillStartRequestForTesting().action());

  GURL https_url("https://www.example.test");
  EXPECT_EQ(content::NavigationThrottle::PROCEED,
            handle
                ->CallWillRedirectRequestForTesting(
                    https_url, false, /* new_method_is_post */
                    http_url,         /* new_referrer_url */
                    false /* new_is_external_protocol */)
                .action());

  test.ExpectTotalCount("Omnibox.URLNavigationTimeToRedirectToHTTPS", 1);
}

// Tests that the histogram is updated when the new URL removed "www."
TEST_F(TypedNavigationTimingThrottleTest, RemoveWWW) {
  base::HistogramTester test;

  GURL http_url("http://www.example.test");
  std::unique_ptr<content::NavigationHandle> handle =
      content::NavigationHandle::CreateNavigationHandleForTesting(
          http_url, main_rfh(), false, /* committed */
          net::OK,                     /* error */
          false,                       /* is_same_document */
          false,                       /* is_post */
          ui::PAGE_TRANSITION_TYPED);

  EXPECT_EQ(content::NavigationThrottle::PROCEED,
            handle->CallWillStartRequestForTesting().action());

  GURL https_url("https://example.test");
  EXPECT_EQ(content::NavigationThrottle::PROCEED,
            handle
                ->CallWillRedirectRequestForTesting(
                    https_url, false, /* new_method_is_post */
                    http_url,         /* new_referrer_url */
                    false /* new_is_external_protocol */)
                .action());

  test.ExpectTotalCount("Omnibox.URLNavigationTimeToRedirectToHTTPS", 1);
}

// Tests that the histogram is updated when the URLs have different ports.
TEST_F(TypedNavigationTimingThrottleTest, NonstandardPorts) {
  base::HistogramTester test;

  GURL http_url("http://example.test:8080");
  std::unique_ptr<content::NavigationHandle> handle =
      content::NavigationHandle::CreateNavigationHandleForTesting(
          http_url, main_rfh(), false, /* committed */
          net::OK,                     /* error */
          false,                       /* is_same_document */
          false,                       /* is_post */
          ui::PAGE_TRANSITION_TYPED);

  EXPECT_EQ(content::NavigationThrottle::PROCEED,
            handle->CallWillStartRequestForTesting().action());

  GURL https_url("https://example.test:4443");
  EXPECT_EQ(content::NavigationThrottle::PROCEED,
            handle
                ->CallWillRedirectRequestForTesting(
                    https_url, false, /* new_method_is_post */
                    http_url,         /* new_referrer_url */
                    false /* new_is_external_protocol */)
                .action());

  test.ExpectTotalCount("Omnibox.URLNavigationTimeToRedirectToHTTPS", 1);
}
