// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/logging.h"
#include "content/browser/accessibility/browser_accessibility.h"
#include "content/browser/accessibility/browser_accessibility_manager.h"
#include "content/browser/web_contents/web_contents_impl.h"
#include "content/public/common/use_zoom_for_dsf_policy.h"
#include "content/public/test/browser_test_utils.h"
#include "content/public/test/content_browser_test.h"
#include "content/public/test/content_browser_test_utils.h"
#include "content/public/test/test_utils.h"
#include "content/shell/browser/shell.h"
#include "content/test/accessibility_browser_test_utils.h"
#include "net/dns/mock_host_resolver.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace content {

class AccessibilityHitTestingBrowserTest : public ContentBrowserTest {
 public:
  AccessibilityHitTestingBrowserTest() {}
  ~AccessibilityHitTestingBrowserTest() override {}

 protected:
  BrowserAccessibility* HitTestAndWaitForResultWithEvent(
      const gfx::Point& point,
      ax::mojom::Event event_to_fire) {
    WebContentsImpl* web_contents =
        static_cast<WebContentsImpl*>(shell()->web_contents());
    FrameTree* frame_tree = web_contents->GetFrameTree();
    BrowserAccessibilityManager* manager =
        web_contents->GetRootBrowserAccessibilityManager();

    AccessibilityNotificationWaiter event_waiter(
        shell()->web_contents(), ui::kAXModeComplete, event_to_fire);
    for (FrameTreeNode* node : frame_tree->Nodes())
      event_waiter.ListenToAdditionalFrame(node->current_frame_host());
    ui::AXActionData action_data;
    action_data.action = ax::mojom::Action::kHitTest;
    action_data.target_point =
        IsUseZoomForDSFEnabled()
            ? ScaleToRoundedPoint(point, manager->device_scale_factor())
            : point;
    action_data.hit_test_event_to_fire = event_to_fire;
    manager->delegate()->AccessibilityPerformAction(action_data);
    event_waiter.WaitForNotification();

    RenderFrameHostImpl* target_frame = event_waiter.event_render_frame_host();
    BrowserAccessibilityManager* target_manager =
        target_frame->browser_accessibility_manager();
    int event_target_id = event_waiter.event_target_id();
    BrowserAccessibility* hit_node = target_manager->GetFromID(event_target_id);
    return hit_node;
  }

  BrowserAccessibility* HitTestAndWaitForResult(const gfx::Point& point) {
    return HitTestAndWaitForResultWithEvent(point, ax::mojom::Event::kHover);
  }

  BrowserAccessibility* CallCachingAsyncHitTest(const gfx::Point& point) {
    WebContentsImpl* web_contents =
        static_cast<WebContentsImpl*>(shell()->web_contents());
    FrameTree* frame_tree = web_contents->GetFrameTree();
    BrowserAccessibilityManager* manager =
        web_contents->GetRootBrowserAccessibilityManager();
    gfx::Point screen_point =
        point + manager->GetViewBounds().OffsetFromOrigin();

    // Each call to CachingAsyncHitTest results in at least one HOVER
    // event received. Block until we receive it.
    AccessibilityNotificationWaiter hover_waiter(
        shell()->web_contents(), ui::kAXModeComplete, ax::mojom::Event::kHover);
    for (FrameTreeNode* node : frame_tree->Nodes())
      hover_waiter.ListenToAdditionalFrame(node->current_frame_host());
    BrowserAccessibility* result = manager->CachingAsyncHitTest(screen_point);
    hover_waiter.WaitForNotification();
    return result;
  }
};

IN_PROC_BROWSER_TEST_F(AccessibilityHitTestingBrowserTest,
                       HitTestOutsideDocumentBoundsReturnsRoot) {
  NavigateToURL(shell(), GURL(url::kAboutBlankURL));

  // Load the page.
  AccessibilityNotificationWaiter waiter(shell()->web_contents(),
                                         ui::kAXModeComplete,
                                         ax::mojom::Event::kLoadComplete);
  const char url_str[] =
      "data:text/html,"
      "<!doctype html>"
      "<html><head><title>Accessibility Test</title></head>"
      "<body>"
      "<a href='#'>"
      "This is some text in a link"
      "</a>"
      "</body></html>";
  GURL url(url_str);
  NavigateToURL(shell(), url);
  waiter.WaitForNotification();

  BrowserAccessibility* hit_node = HitTestAndWaitForResult(gfx::Point(-1, -1));
  ASSERT_TRUE(hit_node != nullptr);
  ASSERT_EQ(ax::mojom::Role::kRootWebArea, hit_node->GetRole());
}

IN_PROC_BROWSER_TEST_F(AccessibilityHitTestingBrowserTest,
                       HitTestingInIframes) {
  ASSERT_TRUE(embedded_test_server()->Start());

  NavigateToURL(shell(), GURL(url::kAboutBlankURL));

  AccessibilityNotificationWaiter waiter(shell()->web_contents(),
                                         ui::kAXModeComplete,
                                         ax::mojom::Event::kLoadComplete);
  GURL url(embedded_test_server()->GetURL(
      "/accessibility/html/iframe-coordinates.html"));
  NavigateToURL(shell(), url);
  waiter.WaitForNotification();

  WaitForAccessibilityTreeToContainNodeWithName(
      shell()->web_contents(), "Ordinary Button");
  WaitForAccessibilityTreeToContainNodeWithName(
      shell()->web_contents(), "Scrolled Button");

  // Send a series of hit test requests, and for each one
  // wait for the hover event in response, verifying we hit the
  // correct object.

  // (50, 50) -> "Button"
  BrowserAccessibility* hit_node;
  hit_node = HitTestAndWaitForResult(gfx::Point(50, 50));
  ASSERT_TRUE(hit_node != nullptr);
  ASSERT_EQ(ax::mojom::Role::kButton, hit_node->GetRole());
  ASSERT_EQ("Button",
            hit_node->GetStringAttribute(ax::mojom::StringAttribute::kName));

  // (50, 305) -> div in first iframe
  hit_node = HitTestAndWaitForResult(gfx::Point(50, 305));
  ASSERT_TRUE(hit_node != nullptr);
  ASSERT_EQ(ax::mojom::Role::kGenericContainer, hit_node->GetRole());

  // (50, 350) -> "Ordinary Button"
  hit_node = HitTestAndWaitForResult(gfx::Point(50, 350));
  ASSERT_TRUE(hit_node != nullptr);
  ASSERT_EQ(ax::mojom::Role::kButton, hit_node->GetRole());
  ASSERT_EQ("Ordinary Button",
            hit_node->GetStringAttribute(ax::mojom::StringAttribute::kName));

  // (50, 455) -> "Scrolled Button"
  hit_node = HitTestAndWaitForResult(gfx::Point(50, 455));
  ASSERT_TRUE(hit_node != nullptr);
  ASSERT_EQ(ax::mojom::Role::kButton, hit_node->GetRole());
  ASSERT_EQ("Scrolled Button",
            hit_node->GetStringAttribute(ax::mojom::StringAttribute::kName));

  // (50, 505) -> div in second iframe
  hit_node = HitTestAndWaitForResult(gfx::Point(50, 505));
  ASSERT_TRUE(hit_node != nullptr);
  ASSERT_EQ(ax::mojom::Role::kGenericContainer, hit_node->GetRole());

  // (50, 505) -> div in second iframe
  // but with a different event
  hit_node = HitTestAndWaitForResultWithEvent(gfx::Point(50, 505),
                                              ax::mojom::Event::kAlert);
  ASSERT_NE(hit_node, nullptr);
  ASSERT_EQ(ax::mojom::Role::kGenericContainer, hit_node->GetRole());
}

IN_PROC_BROWSER_TEST_F(AccessibilityHitTestingBrowserTest,
                       CachingAsyncHitTestingInIframes) {
  ASSERT_TRUE(embedded_test_server()->Start());

  NavigateToURL(shell(), GURL(url::kAboutBlankURL));

  AccessibilityNotificationWaiter waiter(shell()->web_contents(),
                                         ui::kAXModeComplete,
                                         ax::mojom::Event::kLoadComplete);
  GURL url(embedded_test_server()->GetURL(
      "/accessibility/hit_testing/hit_testing.html"));
  NavigateToURL(shell(), url);
  waiter.WaitForNotification();

  WaitForAccessibilityTreeToContainNodeWithName(
      shell()->web_contents(), "Ordinary Button");
  WaitForAccessibilityTreeToContainNodeWithName(
      shell()->web_contents(), "Scrolled Button");

  // For each point we try, the first time we call CachingAsyncHitTest it
  // should FAIL and return the wrong object, because this test page has
  // been designed to confound local synchronous hit testing using
  // z-indexes. However, calling CachingAsyncHitTest a second time should
  // return the correct result (since CallCachingAsyncHitTest waits for the
  // HOVER event to be received).

  // (50, 50) -> "Button"
  BrowserAccessibility* hit_node;
  hit_node = CallCachingAsyncHitTest(gfx::Point(50, 50));
  ASSERT_TRUE(hit_node != nullptr);
  ASSERT_NE(ax::mojom::Role::kButton, hit_node->GetRole());
  hit_node = CallCachingAsyncHitTest(gfx::Point(50, 50));
  ASSERT_EQ("Button",
            hit_node->GetStringAttribute(ax::mojom::StringAttribute::kName));

  // (50, 305) -> div in first iframe
  hit_node = CallCachingAsyncHitTest(gfx::Point(50, 305));
  ASSERT_TRUE(hit_node != nullptr);
  ASSERT_NE(ax::mojom::Role::kGenericContainer, hit_node->GetRole());
  hit_node = CallCachingAsyncHitTest(gfx::Point(50, 305));
  ASSERT_EQ(ax::mojom::Role::kGenericContainer, hit_node->GetRole());

  // (50, 350) -> "Ordinary Button"
  hit_node = CallCachingAsyncHitTest(gfx::Point(50, 350));
  ASSERT_TRUE(hit_node != nullptr);
  ASSERT_NE(ax::mojom::Role::kButton, hit_node->GetRole());
  hit_node = CallCachingAsyncHitTest(gfx::Point(50, 350));
  ASSERT_EQ(ax::mojom::Role::kButton, hit_node->GetRole());
  ASSERT_EQ("Ordinary Button",
            hit_node->GetStringAttribute(ax::mojom::StringAttribute::kName));

  // (50, 455) -> "Scrolled Button"
  hit_node = CallCachingAsyncHitTest(gfx::Point(50, 455));
  ASSERT_TRUE(hit_node != nullptr);
  ASSERT_NE(ax::mojom::Role::kButton, hit_node->GetRole());
  hit_node = CallCachingAsyncHitTest(gfx::Point(50, 455));
  ASSERT_EQ(ax::mojom::Role::kButton, hit_node->GetRole());
  ASSERT_EQ("Scrolled Button",
            hit_node->GetStringAttribute(ax::mojom::StringAttribute::kName));

  // (50, 505) -> div in second iframe
  hit_node = CallCachingAsyncHitTest(gfx::Point(50, 505));
  ASSERT_TRUE(hit_node != nullptr);
  ASSERT_NE(ax::mojom::Role::kGenericContainer, hit_node->GetRole());
  hit_node = CallCachingAsyncHitTest(gfx::Point(50, 505));
  ASSERT_EQ(ax::mojom::Role::kGenericContainer, hit_node->GetRole());
}

}  // namespace content
