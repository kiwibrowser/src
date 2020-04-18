// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <string>

#include "base/command_line.h"
#include "base/test/scoped_feature_list.h"
#include "content/browser/frame_host/frame_tree_node.h"
#include "content/browser/web_contents/web_contents_impl.h"
#include "content/public/common/content_features.h"
#include "content/public/test/browser_test_utils.h"
#include "content/public/test/content_browser_test.h"
#include "content/public/test/content_browser_test_utils.h"
#include "content/public/test/test_frame_navigation_observer.h"
#include "content/public/test/test_navigation_observer.h"
#include "content/shell/browser/shell.h"
#include "content/test/content_browser_test_utils_internal.h"
#include "net/dns/mock_host_resolver.h"
#include "net/test/embedded_test_server/embedded_test_server.h"
#include "url/gurl.h"

namespace content {

class TopDocumentIsolationTest : public ContentBrowserTest {
 public:
  TopDocumentIsolationTest() {}

 protected:
  std::string DepictFrameTree(FrameTreeNode* node) {
    return visualizer_.DepictFrameTree(node);
  }

  void SetUp() override {
    scoped_feature_list_.InitAndEnableFeature(features::kTopDocumentIsolation);
    ContentBrowserTest::SetUp();
  }

  void SetUpOnMainThread() override {
    host_resolver()->AddRule("*", "127.0.0.1");
    SetupCrossSiteRedirector(embedded_test_server());
    ASSERT_TRUE(embedded_test_server()->Start());
  }

  FrameTreeNode* root() {
    return static_cast<WebContentsImpl*>(shell()->web_contents())
        ->GetFrameTree()
        ->root();
  }

  void GoBack() {
    TestNavigationObserver back_load_observer(shell()->web_contents());
    shell()->web_contents()->GetController().GoBack();
    back_load_observer.Wait();
  }

  Shell* OpenPopup(FrameTreeNode* opener, const std::string& url) {
    GURL gurl =
        opener->current_frame_host()->GetLastCommittedURL().Resolve(url);
    return content::OpenPopup(opener, gurl, "_blank");
  }

  void RendererInitiatedNavigateToURL(FrameTreeNode* node, const GURL& url) {
    TestFrameNavigationObserver nav_observer(node);
    ASSERT_TRUE(
        ExecuteScript(node, "window.location.href='" + url.spec() + "'"));
    nav_observer.Wait();
  }

 private:
  FrameTreeVisualizer visualizer_;
  base::test::ScopedFeatureList scoped_feature_list_;

  DISALLOW_COPY_AND_ASSIGN(TopDocumentIsolationTest);
};

IN_PROC_BROWSER_TEST_F(TopDocumentIsolationTest, SameSiteDeeplyNested) {
  if (content::AreAllSitesIsolatedForTesting())
    return;  // Top Document Isolation is disabled in this mode.

  GURL main_url(embedded_test_server()->GetURL(
      "a.com", "/cross_site_iframe_factory.html?a(a,a(a,a(a)))"));

  NavigateToURL(shell(), main_url);

  EXPECT_EQ(
      " Site A\n"
      "   |--Site A\n"
      "   +--Site A\n"
      "        |--Site A\n"
      "        +--Site A\n"
      "             +--Site A\n"
      "Where A = http://a.com/",
      DepictFrameTree(root()));
}

IN_PROC_BROWSER_TEST_F(TopDocumentIsolationTest, CrossSiteDeeplyNested) {
  if (content::AreAllSitesIsolatedForTesting())
    return;  // Top Document Isolation is disabled in this mode.

  GURL main_url(embedded_test_server()->GetURL(
      "a.com", "/cross_site_iframe_factory.html?a(b(c(d(b))))"));

  NavigateToURL(shell(), main_url);

  EXPECT_EQ(
      " Site A ------------ proxies for B\n"
      "   +--Site B ------- proxies for A\n"
      "        +--Site B -- proxies for A\n"
      "             +--Site B -- proxies for A\n"
      "                  +--Site B -- proxies for A\n"
      "Where A = http://a.com/\n"
      "      B = default subframe process",
      DepictFrameTree(root()));
}

IN_PROC_BROWSER_TEST_F(TopDocumentIsolationTest, ReturnToTopSite) {
  if (content::AreAllSitesIsolatedForTesting())
    return;  // Top Document Isolation is disabled in this mode.

  GURL main_url(embedded_test_server()->GetURL(
      "a.com", "/cross_site_iframe_factory.html?a(b(a(c)))"));

  NavigateToURL(shell(), main_url);

  EXPECT_EQ(
      " Site A ------------ proxies for B\n"
      "   +--Site B ------- proxies for A\n"
      "        +--Site A -- proxies for B\n"
      "             +--Site B -- proxies for A\n"
      "Where A = http://a.com/\n"
      "      B = default subframe process",
      DepictFrameTree(root()));
}

IN_PROC_BROWSER_TEST_F(TopDocumentIsolationTest, NavigateSubframeToTopSite) {
  if (content::AreAllSitesIsolatedForTesting())
    return;  // Top Document Isolation is disabled in this mode.

  GURL main_url(embedded_test_server()->GetURL(
      "a.com", "/cross_site_iframe_factory.html?a(b(c(d)))"));

  NavigateToURL(shell(), main_url);

  EXPECT_EQ(
      " Site A ------------ proxies for B\n"
      "   +--Site B ------- proxies for A\n"
      "        +--Site B -- proxies for A\n"
      "             +--Site B -- proxies for A\n"
      "Where A = http://a.com/\n"
      "      B = default subframe process",
      DepictFrameTree(root()));

  GURL ada_url(embedded_test_server()->GetURL(
      "a.com", "/cross_site_iframe_factory.html?a(d(a))"));
  RendererInitiatedNavigateToURL(root()->child_at(0)->child_at(0), ada_url);

  EXPECT_EQ(
      " Site A ------------ proxies for B\n"
      "   +--Site B ------- proxies for A\n"
      "        +--Site A -- proxies for B\n"
      "             +--Site B -- proxies for A\n"
      "                  +--Site A -- proxies for B\n"
      "Where A = http://a.com/\n"
      "      B = default subframe process",
      DepictFrameTree(root()));
}

IN_PROC_BROWSER_TEST_F(TopDocumentIsolationTest, NavigateToSubframeSite) {
  if (content::AreAllSitesIsolatedForTesting())
    return;  // Top Document Isolation is disabled in this mode.

  GURL ab_url(embedded_test_server()->GetURL(
      "a.com", "/cross_site_iframe_factory.html?a(b)"));
  GURL ba_url(embedded_test_server()->GetURL(
      "b.com", "/cross_site_iframe_factory.html?b(a, c)"));

  NavigateToURL(shell(), ab_url);

  EXPECT_EQ(
      " Site A ------------ proxies for B\n"
      "   +--Site B ------- proxies for A\n"
      "Where A = http://a.com/\n"
      "      B = default subframe process",
      DepictFrameTree(root()));

  NavigateToURL(shell(), ba_url);

  EXPECT_EQ(
      " Site C ------------ proxies for D\n"
      "   |--Site D ------- proxies for C\n"
      "   +--Site D ------- proxies for C\n"
      "Where C = http://b.com/\n"
      "      D = default subframe process",
      DepictFrameTree(root()));
}

IN_PROC_BROWSER_TEST_F(TopDocumentIsolationTest,
                       NavigateToSubframeSiteWithPopup) {
  if (content::AreAllSitesIsolatedForTesting())
    return;  // Top Document Isolation is disabled in this mode.

  // A(B) -> B(A), but while a separate B(A) popup exists.
  GURL ab_url(embedded_test_server()->GetURL(
      "a.com", "/cross_site_iframe_factory.html?a(b)"));

  NavigateToURL(shell(), ab_url);

  EXPECT_EQ(
      " Site A ------------ proxies for B\n"
      "   +--Site B ------- proxies for A\n"
      "Where A = http://a.com/\n"
      "      B = default subframe process",
      DepictFrameTree(root()));

  Shell* popup =
      OpenPopup(root()->child_at(0), "/cross_site_iframe_factory.html?b(a)");
  FrameTreeNode* popup_root =
      static_cast<WebContentsImpl*>(popup->web_contents())
          ->GetFrameTree()
          ->root();

  // This popup's main frame must stay in the default subframe siteinstance,
  // since its opener (the b.com subframe) may synchronously script it. Note
  // that the popup's subframe is same-site with window.top.opener.top, the
  // a.com main frame of the tab. But --top-document-isolation does not
  // currently place the popup subframe in the a.com process in this case.
  EXPECT_EQ(
      " Site B\n"
      "   +--Site B\n"
      "Where B = default subframe process",
      DepictFrameTree(popup_root));

  GURL ba_url(embedded_test_server()->GetURL(
      "b.com", "/cross_site_iframe_factory.html?b(a, c)"));
  EXPECT_TRUE(NavigateToURLInSameBrowsingInstance(shell(), ba_url));

  // This navigation destroys the popup's opener, so we allow the main frame to
  // commit in a top level process for b.com, in spite of the b.com popup in the
  // default subframe process.
  EXPECT_EQ(
      " Site C ------------ proxies for B\n"
      "   |--Site B ------- proxies for C\n"
      "   +--Site B ------- proxies for C\n"
      "Where B = default subframe process\n"
      "      C = http://b.com/",
      DepictFrameTree(root()));
  EXPECT_EQ(
      " Site B\n"
      "   +--Site B\n"
      "Where B = default subframe process",
      DepictFrameTree(popup_root));

  // Navigate the popup to a new site.
  GURL c_url(embedded_test_server()->GetURL(
      "c.com", "/cross_site_iframe_factory.html?c(c, c, c, c)"));
  EXPECT_TRUE(NavigateToURLInSameBrowsingInstance(popup, c_url));
  EXPECT_EQ(
      " Site D ------------ proxies for B\n"
      "   |--Site D ------- proxies for B\n"
      "   |--Site D ------- proxies for B\n"
      "   |--Site D ------- proxies for B\n"
      "   +--Site D ------- proxies for B\n"
      "Where B = default subframe process\n"
      "      D = http://c.com/",
      DepictFrameTree(popup_root));
  EXPECT_TRUE(NavigateToURLInSameBrowsingInstance(shell(), c_url));
  EXPECT_EQ(
      " Site D\n"
      "   |--Site D\n"
      "   |--Site D\n"
      "   |--Site D\n"
      "   +--Site D\n"
      "Where D = http://c.com/",
      DepictFrameTree(popup_root));
  EXPECT_EQ(
      " Site D\n"
      "   |--Site D\n"
      "   |--Site D\n"
      "   |--Site D\n"
      "   +--Site D\n"
      "Where D = http://c.com/",
      DepictFrameTree(root()));
}

IN_PROC_BROWSER_TEST_F(TopDocumentIsolationTest,
                       NavigateToSubframeSiteWithPopup2) {
  if (content::AreAllSitesIsolatedForTesting())
    return;  // Top Document Isolation is disabled in this mode.

  // A(B, C) -> C(A, B), but while a separate C(A) popup exists.
  //
  // This test is constructed so that c.com is the second site to commit in the
  // default subframe SiteInstance, so the default subframe SiteInstance does
  // not have a "c.com" as the value of GetSiteURL().
  GURL abb_url(embedded_test_server()->GetURL(
      "a.com", "/cross_site_iframe_factory.html?a(b, b)"));

  NavigateToURL(shell(), abb_url);

  EXPECT_EQ(
      " Site A ------------ proxies for B\n"
      "   |--Site B ------- proxies for A\n"
      "   +--Site B ------- proxies for A\n"
      "Where A = http://a.com/\n"
      "      B = default subframe process",
      DepictFrameTree(root()));

  // A(B, B) -> A(B, C)
  GURL c_url(embedded_test_server()->GetURL(
      "c.com", "/cross_site_iframe_factory.html?c"));
  NavigateFrameToURL(root()->child_at(1), c_url);

  EXPECT_EQ(
      " Site A ------------ proxies for B\n"
      "   |--Site B ------- proxies for A\n"
      "   +--Site B ------- proxies for A\n"
      "Where A = http://a.com/\n"
      "      B = default subframe process",
      DepictFrameTree(root()));

  // This test exercises what happens when the SiteURL of the default subframe
  // siteinstance doesn't match the subframe site.
  EXPECT_NE("c.com", root()
                         ->child_at(1)
                         ->current_frame_host()
                         ->GetSiteInstance()
                         ->GetSiteURL()
                         .host());

  // Subframe C creates C(A) popup.
  Shell* popup =
      OpenPopup(root()->child_at(1), "/cross_site_iframe_factory.html?c(a)");

  FrameTreeNode* popup_root =
      static_cast<WebContentsImpl*>(popup->web_contents())
          ->GetFrameTree()
          ->root();

  // The popup must stay with its opener, in the default subframe process.
  EXPECT_EQ(
      " Site B\n"
      "   +--Site B\n"
      "Where B = default subframe process",
      DepictFrameTree(popup_root));

  GURL cab_url(embedded_test_server()->GetURL(
      "c.com", "/cross_site_iframe_factory.html?c(a, b)"));
  {
    RenderFrameDeletedObserver deleted_observer(root()->current_frame_host());
    EXPECT_TRUE(NavigateToURLInSameBrowsingInstance(shell(), cab_url));
    deleted_observer.WaitUntilDeleted();
  }

  // This c.com navigation currently breaks out of the default subframe process,
  // even though that process houses a c.com pop-up.
  EXPECT_EQ(
      " Site C ------------ proxies for B\n"
      "   |--Site B ------- proxies for C\n"
      "   +--Site B ------- proxies for C\n"
      "Where B = default subframe process\n"
      "      C = http://c.com/",
      DepictFrameTree(root()));

  // c.com popup should remain where it was, in the subframe process.
  EXPECT_EQ(
      " Site B\n"
      "   +--Site B\n"
      "Where B = default subframe process",
      DepictFrameTree(popup_root));
  EXPECT_EQ(nullptr, popup_root->opener());

  // If we navigate the popup to a new site, it ought to transfer processes.
  GURL d_url(embedded_test_server()->GetURL(
      "d.com", "/cross_site_iframe_factory.html?d"));
  {
    RenderFrameDeletedObserver deleted_observer(
        popup_root->current_frame_host());
    EXPECT_TRUE(NavigateToURLInSameBrowsingInstance(popup, d_url));
    deleted_observer.WaitUntilDeleted();
  }
  EXPECT_EQ(
      " Site D ------------ proxies for B\n"
      "Where B = default subframe process\n"
      "      D = http://d.com/",
      DepictFrameTree(popup_root));
  {
    RenderFrameDeletedObserver deleted_observer(root()->current_frame_host());
    EXPECT_TRUE(NavigateToURLInSameBrowsingInstance(shell(), d_url));
    deleted_observer.WaitUntilDeleted();
  }
  EXPECT_EQ(
      " Site D\n"
      "Where D = http://d.com/",
      DepictFrameTree(popup_root));
  EXPECT_EQ(
      " Site D\n"
      "Where D = http://d.com/",
      DepictFrameTree(root()));
}

IN_PROC_BROWSER_TEST_F(TopDocumentIsolationTest, FramesForSitesInHistory) {
  if (content::AreAllSitesIsolatedForTesting())
    return;  // Top Document Isolation is disabled in this mode.

  // First, do a series of navigations.
  GURL a_url = embedded_test_server()->GetURL(
      "a.com", "/cross_site_iframe_factory.html?a");
  GURL b_url = embedded_test_server()->GetURL(
      "b.com", "/cross_site_iframe_factory.html?b");
  GURL c_url = embedded_test_server()->GetURL(
      "c.com", "/cross_site_iframe_factory.html?c");

  // Browser-initiated navigation to a.com.
  NavigateToURL(shell(), a_url);
  EXPECT_EQ(
      " Site A\n"
      "Where A = http://a.com/",
      DepictFrameTree(root()));

  // Browser-initiated navigation to b.com.
  {
    // For any cross-process navigations, we must wait for the old RenderFrame
    // to be deleted before calling DepictFrameTree, or else there's a chance
    // the old SiteInstance could be listed while pending deletion.
    RenderFrameDeletedObserver deleted_observer(root()->current_frame_host());
    NavigateToURL(shell(), b_url);
    deleted_observer.WaitUntilDeleted();
  }
  EXPECT_EQ(
      " Site B\n"
      "Where B = http://b.com/",
      DepictFrameTree(root()));

  // Renderer-initiated navigation back to a.com. This shouldn't swap processes.
  RendererInitiatedNavigateToURL(root(), a_url);
  EXPECT_EQ(
      " Site B\n"
      "Where B = http://b.com/",
      DepictFrameTree(root()));

  // Browser-initiated navigation to c.com.
  {
    RenderFrameDeletedObserver deleted_observer(root()->current_frame_host());
    NavigateToURL(shell(), c_url);
    deleted_observer.WaitUntilDeleted();
  }
  EXPECT_EQ(
      " Site C\n"
      "Where C = http://c.com/",
      DepictFrameTree(root()));

  // Now, navigate to a fourth site with iframes to the sites in the history.
  {
    RenderFrameDeletedObserver deleted_observer(root()->current_frame_host());
    NavigateToURL(shell(),
                  embedded_test_server()->GetURL(
                      "d.com", "/cross_site_iframe_factory.html?d(a,b,c)"));
    deleted_observer.WaitUntilDeleted();
  }

  EXPECT_EQ(
      " Site D ------------ proxies for E\n"
      "   |--Site E ------- proxies for D\n"
      "   |--Site E ------- proxies for D\n"
      "   +--Site E ------- proxies for D\n"
      "Where D = http://d.com/\n"
      "      E = default subframe process",
      DepictFrameTree(root()));

  // Now try going back.
  {
    RenderFrameDeletedObserver deleted_observer(root()->current_frame_host());
    GoBack();
    deleted_observer.WaitUntilDeleted();
  }
  EXPECT_EQ(
      " Site C\n"
      "Where C = http://c.com/",
      DepictFrameTree(root()));
  {
    RenderFrameDeletedObserver deleted_observer(root()->current_frame_host());
    GoBack();
    deleted_observer.WaitUntilDeleted();
  }
  EXPECT_EQ(
      " Site B\n"
      "Where B = http://b.com/",
      DepictFrameTree(root()));
  GoBack();
  EXPECT_EQ(
      " Site B\n"
      "Where B = http://b.com/",
      DepictFrameTree(root()));
  {
    RenderFrameDeletedObserver deleted_observer(root()->current_frame_host());
    GoBack();
    deleted_observer.WaitUntilDeleted();
  }
  EXPECT_EQ(
      " Site A\n"
      "Where A = http://a.com/",
      DepictFrameTree(root()));
}

IN_PROC_BROWSER_TEST_F(TopDocumentIsolationTest, CrossSiteAtLevelTwo) {
  if (content::AreAllSitesIsolatedForTesting())
    return;  // Top Document Isolation is disabled in this mode.

  GURL main_url(embedded_test_server()->GetURL(
      "a.com", "/cross_site_iframe_factory.html?a(a(b, a))"));

  NavigateToURL(shell(), main_url);

  EXPECT_EQ(
      " Site A ------------ proxies for B\n"
      "   +--Site A ------- proxies for B\n"
      "        |--Site B -- proxies for A\n"
      "        +--Site A -- proxies for B\n"
      "Where A = http://a.com/\n"
      "      B = default subframe process",
      DepictFrameTree(root()));

  GURL c_url(embedded_test_server()->GetURL(
      "c.com", "/cross_site_iframe_factory.html?c"));
  NavigateFrameToURL(root()->child_at(0)->child_at(1), c_url);

  // This navigation should complete in the default subframe siteinstance.
  EXPECT_EQ(
      " Site A ------------ proxies for B\n"
      "   +--Site A ------- proxies for B\n"
      "        |--Site B -- proxies for A\n"
      "        +--Site B -- proxies for A\n"
      "Where A = http://a.com/\n"
      "      B = default subframe process",
      DepictFrameTree(root()));
}

IN_PROC_BROWSER_TEST_F(TopDocumentIsolationTest, PopupAndRedirection) {
  if (content::AreAllSitesIsolatedForTesting())
    return;  // Top Document Isolation is disabled in this mode.

  GURL main_url(embedded_test_server()->GetURL(
      "page.com", "/cross_site_iframe_factory.html?page(adnetwork)"));

  // User opens page on page.com which contains a subframe from adnetwork.com.
  NavigateToURL(shell(), main_url);

  EXPECT_EQ(
      " Site A ------------ proxies for B\n"
      "   +--Site B ------- proxies for A\n"
      "Where A = http://page.com/\n"
      "      B = default subframe process",
      DepictFrameTree(root()));

  GURL ad_url(embedded_test_server()->GetURL(
      "ad.com", "/cross_site_iframe_factory.html?ad"));

  // adnetwork.com retrieves an ad from advertiser (ad.com) and redirects the
  // subframe to ad.com.
  RendererInitiatedNavigateToURL(root()->child_at(0), ad_url);

  // The subframe still uses the default subframe SiteInstance after navigation.
  EXPECT_EQ(
      " Site A ------------ proxies for B\n"
      "   +--Site B ------- proxies for A\n"
      "Where A = http://page.com/\n"
      "      B = default subframe process",
      DepictFrameTree(root()));

  // User clicks the ad in the subframe, which opens a popup on the ad
  // network's domain.
  GURL popup_url(embedded_test_server()->GetURL(
      "adnetwork.com", "/cross_site_iframe_factory.html?adnetwork"));
  Shell* popup = OpenPopup(root()->child_at(0), popup_url.spec());

  FrameTreeNode* popup_root =
      static_cast<WebContentsImpl*>(popup->web_contents())
          ->GetFrameTree()
          ->root();

  // It's ok for the popup to break out of the subframe process because it's
  // currently cross-site from its opener frame.
  EXPECT_EQ(
      " Site C ------------ proxies for B\n"
      "Where B = default subframe process\n"
      "      C = http://adnetwork.com/",
      DepictFrameTree(popup_root));

  EXPECT_EQ(
      " Site A ------------ proxies for B C\n"
      "   +--Site B ------- proxies for A C\n"
      "Where A = http://page.com/\n"
      "      B = default subframe process\n"
      "      C = http://adnetwork.com/",
      DepictFrameTree(root()));

  // The popup redirects itself to the advertiser's website (ad.com).
  RenderFrameDeletedObserver deleted_observer(popup_root->current_frame_host());
  RendererInitiatedNavigateToURL(popup_root, ad_url);
  deleted_observer.WaitUntilDeleted();

  // This must join its same-site opener, in the default subframe SiteInstance.
  EXPECT_EQ(
      " Site A ------------ proxies for B\n"
      "   +--Site B ------- proxies for A\n"
      "Where A = http://page.com/\n"
      "      B = default subframe process",
      DepictFrameTree(root()));
  EXPECT_EQ(
      " Site B\n"
      "Where B = default subframe process",
      DepictFrameTree(popup_root));
}

}  // namespace content
