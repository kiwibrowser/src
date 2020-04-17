// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/command_line.h"
#include "base/test/scoped_feature_list.h"
#include "content/browser/frame_host/back_forward_cache.h"
#include "content/browser/frame_host/frame_tree_node.h"
#include "content/browser/web_contents/web_contents_impl.h"
#include "content/public/common/content_features.h"
#include "content/public/test/browser_test_utils.h"
#include "content/public/test/content_browser_test.h"
#include "content/public/test/content_browser_test_utils.h"
#include "content/public/test/test_utils.h"
#include "content/shell/browser/shell.h"
#include "net/dns/mock_host_resolver.h"
#include "net/test/embedded_test_server/embedded_test_server.h"

namespace content {

namespace {

// Test about the BackForwardCache.
class BackForwardCacheBrowserTest : public ContentBrowserTest {
 protected:
  void SetUpCommandLine(base::CommandLine* command_line) override {
    feature_list_.InitAndEnableFeature(features::kBackForwardCache);
  }

  void SetUpOnMainThread() override {
    host_resolver()->AddRule("*", "127.0.0.1");
  }

  WebContentsImpl* web_contents() const {
    return static_cast<WebContentsImpl*>(shell()->web_contents());
  }

  RenderFrameHostImpl* current_frame_host() {
    return web_contents()->GetFrameTree()->root()->current_frame_host();
  }

 private:
  base::test::ScopedFeatureList feature_list_;
};

}  // namespace

// Navigate from A to B and go back.
IN_PROC_BROWSER_TEST_F(BackForwardCacheBrowserTest, Basic) {
  ASSERT_TRUE(embedded_test_server()->Start());
  GURL url_a(embedded_test_server()->GetURL("a.com", "/title1.html"));
  GURL url_b(embedded_test_server()->GetURL("b.com", "/title1.html"));

  // 1) Navigate to A.
  NavigateToURL(shell(), url_a);
  RenderFrameHostImpl* rfh_a = current_frame_host();
  RenderFrameDeletedObserver delete_rfh_a(rfh_a);

  // 2) Navigate to B.
  NavigateToURL(shell(), url_b);
  RenderFrameHostImpl* rfh_b = current_frame_host();
  RenderFrameDeletedObserver delete_rfh_b(rfh_b);
  EXPECT_FALSE(delete_rfh_a.deleted());
  EXPECT_TRUE(rfh_a->is_in_back_forward_cache());
  EXPECT_FALSE(rfh_b->is_in_back_forward_cache());

  // 3) Go back to A.
  web_contents()->GetController().GoBack();
  EXPECT_TRUE(WaitForLoadStop(shell()->web_contents()));
  EXPECT_FALSE(delete_rfh_a.deleted());
  EXPECT_FALSE(delete_rfh_b.deleted());
  EXPECT_EQ(rfh_a, current_frame_host());
  EXPECT_FALSE(rfh_a->is_in_back_forward_cache());
  EXPECT_TRUE(rfh_b->is_in_back_forward_cache());
}

// Navigate from A(B) to C and go back.
IN_PROC_BROWSER_TEST_F(BackForwardCacheBrowserTest, BasicIframe) {
  ASSERT_TRUE(embedded_test_server()->Start());
  GURL url_a(embedded_test_server()->GetURL(
      "a.com", "/cross_site_iframe_factory.html?a(b)"));
  GURL url_c(embedded_test_server()->GetURL("c.com", "/title1.html"));

  // 1) Navigate to A(B).
  NavigateToURL(shell(), url_a);
  RenderFrameHostImpl* rfh_a = current_frame_host();
  RenderFrameHostImpl* rfh_b = rfh_a->child_at(0)->current_frame_host();
  RenderFrameDeletedObserver delete_rfh_a(rfh_a);
  RenderFrameDeletedObserver delete_rfh_b(rfh_b);

  // 2) Navigate to C.
  NavigateToURL(shell(), url_c);
  RenderFrameHostImpl* rfh_c = current_frame_host();
  RenderFrameDeletedObserver delete_rfh_c(rfh_c);
  EXPECT_FALSE(delete_rfh_a.deleted());
  EXPECT_FALSE(delete_rfh_b.deleted());
  EXPECT_TRUE(rfh_a->is_in_back_forward_cache());
  EXPECT_TRUE(rfh_b->is_in_back_forward_cache());
  EXPECT_FALSE(rfh_c->is_in_back_forward_cache());

  // 3) Go back to A(B).
  web_contents()->GetController().GoBack();
  EXPECT_TRUE(WaitForLoadStop(shell()->web_contents()));
  EXPECT_FALSE(delete_rfh_a.deleted());
  EXPECT_FALSE(delete_rfh_b.deleted());
  EXPECT_FALSE(delete_rfh_c.deleted());
  EXPECT_EQ(rfh_a, current_frame_host());
  EXPECT_FALSE(rfh_a->is_in_back_forward_cache());
  EXPECT_FALSE(rfh_b->is_in_back_forward_cache());
  EXPECT_TRUE(rfh_c->is_in_back_forward_cache());
}

// Ensure flushing the BackForwardCache works properly.
IN_PROC_BROWSER_TEST_F(BackForwardCacheBrowserTest, BackForwardCacheFlush) {
  ASSERT_TRUE(embedded_test_server()->Start());
  GURL url_a(embedded_test_server()->GetURL("a.com", "/title1.html"));
  GURL url_b(embedded_test_server()->GetURL("b.com", "/title1.html"));

  // 1) Navigate to A.
  NavigateToURL(shell(), url_a);
  RenderFrameHostImpl* rfh_a = current_frame_host();
  RenderFrameDeletedObserver delete_rfh_a(rfh_a);

  // 2) Navigate to B.
  NavigateToURL(shell(), url_b);
  RenderFrameHostImpl* rfh_b = current_frame_host();
  RenderFrameDeletedObserver delete_rfh_b(rfh_b);
  EXPECT_FALSE(delete_rfh_a.deleted());

  // 3) Flush A.
  web_contents()->GetController().back_forward_cache().Flush();
  EXPECT_TRUE(delete_rfh_a.deleted());
  EXPECT_FALSE(delete_rfh_b.deleted());

  // 4) Go back to a new A.
  web_contents()->GetController().GoBack();
  EXPECT_TRUE(WaitForLoadStop(shell()->web_contents()));
  EXPECT_TRUE(delete_rfh_a.deleted());
  EXPECT_FALSE(delete_rfh_b.deleted());

  // 5) Flush B.
  web_contents()->GetController().back_forward_cache().Flush();
  EXPECT_TRUE(delete_rfh_b.deleted());
}

// Check the visible URL in the omnibox is properly updated when restoring a
// document from the BackForwardCache.
IN_PROC_BROWSER_TEST_F(BackForwardCacheBrowserTest, VisibleURL) {
  ASSERT_TRUE(embedded_test_server()->Start());
  GURL url_a(embedded_test_server()->GetURL("a.com", "/title1.html"));
  GURL url_b(embedded_test_server()->GetURL("b.com", "/title1.html"));

  // 1) Go to A.
  NavigateToURL(shell(), url_a);

  // 2) Go to B.
  NavigateToURL(shell(), url_b);

  // 3) Go back to A.
  web_contents()->GetController().GoBack();
  EXPECT_EQ(url_a, web_contents()->GetVisibleURL());

  // 4) Go forward to B.
  web_contents()->GetController().GoForward();
  EXPECT_EQ(url_b, web_contents()->GetVisibleURL());
}

// Test documents are evicted from the BackForwardCache at some point.
IN_PROC_BROWSER_TEST_F(BackForwardCacheBrowserTest, CacheEviction) {
  ASSERT_TRUE(embedded_test_server()->Start());
  GURL url_a(embedded_test_server()->GetURL("a.com", "/title1.html"));
  GURL url_b(embedded_test_server()->GetURL("b.com", "/title1.html"));

  NavigateToURL(shell(), url_a);  // BackForwardCache size is 0.
  RenderFrameHostImpl* rfh_a = current_frame_host();
  RenderFrameDeletedObserver delete_rfh_a(rfh_a);

  NavigateToURL(shell(), url_b);  // BackForwardCache size is 1.
  RenderFrameHostImpl* rfh_b = current_frame_host();
  RenderFrameDeletedObserver delete_rfh_b(rfh_b);

  // The number of document the BackForwardCache can hold per tab.
  static constexpr size_t kBackForwardCacheLimit = 3;

  for (size_t i = 2; i < kBackForwardCacheLimit; ++i) {
    NavigateToURL(shell(), i % 2 ? url_b : url_a);
    // After |i+1| navigations, |i| documents went into the BackForwardCache.
    // When |i| is greater than the BackForwardCache size limit, they are
    // evicted:
    EXPECT_EQ(i >= kBackForwardCacheLimit + 1, delete_rfh_a.deleted());
    EXPECT_EQ(i >= kBackForwardCacheLimit + 2, delete_rfh_b.deleted());
  }
}

}  // namespace content
