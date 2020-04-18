// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <tuple>

#include "base/macros.h"
#include "base/strings/utf_string_conversions.h"
#include "base/time/time.h"
#include "build/build_config.h"
#include "content/common/accessibility_messages.h"
#include "content/common/frame_messages.h"
#include "content/common/view_message_enums.h"
#include "content/public/common/content_switches.h"
#include "content/public/test/render_view_test.h"
#include "content/renderer/accessibility/render_accessibility_impl.h"
#include "content/renderer/render_frame_impl.h"
#include "content/renderer/render_view_impl.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/blink/public/platform/web_size.h"
#include "third_party/blink/public/web/web_ax_object.h"
#include "third_party/blink/public/web/web_document.h"
#include "third_party/blink/public/web/web_local_frame.h"
#include "third_party/blink/public/web/web_view.h"
#include "ui/accessibility/ax_node_data.h"

using blink::WebAXObject;
using blink::WebDocument;

namespace content {

class TestRenderAccessibilityImpl : public RenderAccessibilityImpl {
 public:
  explicit TestRenderAccessibilityImpl(RenderFrameImpl* render_frame)
      : RenderAccessibilityImpl(render_frame, ui::kAXModeComplete) {}

  void SendPendingAccessibilityEvents() {
    RenderAccessibilityImpl::SendPendingAccessibilityEvents();
  }
};

class RenderAccessibilityImplTest : public RenderViewTest {
 public:
  RenderAccessibilityImplTest() {}

  RenderViewImpl* view() {
    return static_cast<RenderViewImpl*>(view_);
  }

  RenderFrameImpl* frame() {
    return static_cast<RenderFrameImpl*>(view()->GetMainRenderFrame());
  }

  void SetUp() override {
    RenderViewTest::SetUp();
    sink_ = &render_thread_->sink();
  }

  void TearDown() override {
#if defined(LEAK_SANITIZER)
     // Do this before shutting down V8 in RenderViewTest::TearDown().
     // http://crbug.com/328552
     __lsan_do_leak_check();
#endif
     RenderViewTest::TearDown();
  }

  void SetMode(ui::AXMode mode) { frame()->OnSetAccessibilityMode(mode); }

  void GetAllAccEvents(
      std::vector<AccessibilityHostMsg_EventParams>* param_list) {
    const IPC::Message* message =
        sink_->GetUniqueMessageMatching(AccessibilityHostMsg_Events::ID);
    ASSERT_TRUE(message);
    std::tuple<std::vector<AccessibilityHostMsg_EventParams>, int, int> param;
    AccessibilityHostMsg_Events::Read(message, &param);
    *param_list = std::get<0>(param);
  }

  void GetLastAccEvent(
      AccessibilityHostMsg_EventParams* params) {
    std::vector<AccessibilityHostMsg_EventParams> param_list;
    GetAllAccEvents(&param_list);
    ASSERT_GE(param_list.size(), 1U);
    *params = param_list[0];
  }

  int CountAccessibilityNodesSentToBrowser() {
    AccessibilityHostMsg_EventParams event;
    GetLastAccEvent(&event);
    return event.update.nodes.size();
  }

 protected:
  IPC::TestSink* sink_;

 private:
  DISALLOW_COPY_AND_ASSIGN(RenderAccessibilityImplTest);
};

TEST_F(RenderAccessibilityImplTest, SendFullAccessibilityTreeOnReload) {
  // The job of RenderAccessibilityImpl is to serialize the
  // accessibility tree built by WebKit and send it to the browser.
  // When the accessibility tree changes, it tries to send only
  // the nodes that actually changed or were reparented. This test
  // ensures that the messages sent are correct in cases when a page
  // reloads, and that internal state is properly garbage-collected.
  std::string html =
      "<body>"
      "  <div role='group' id='A'>"
      "    <div role='group' id='A1'></div>"
      "    <div role='group' id='A2'></div>"
      "  </div>"
      "</body>";
  LoadHTML(html.c_str());

  // Creating a RenderAccessibilityImpl should sent the tree to the browser.
  std::unique_ptr<TestRenderAccessibilityImpl> accessibility(
      new TestRenderAccessibilityImpl(frame()));
  accessibility->SendPendingAccessibilityEvents();
  EXPECT_EQ(4, CountAccessibilityNodesSentToBrowser());

  // If we post another event but the tree doesn't change,
  // we should only send 1 node to the browser.
  sink_->ClearMessages();
  WebDocument document = GetMainFrame()->GetDocument();
  WebAXObject root_obj = WebAXObject::FromWebDocument(document);
  accessibility->HandleAXEvent(root_obj, ax::mojom::Event::kLayoutComplete);
  accessibility->SendPendingAccessibilityEvents();
  EXPECT_EQ(1, CountAccessibilityNodesSentToBrowser());
  {
    // Make sure it's the root object that was updated.
    AccessibilityHostMsg_EventParams event;
    GetLastAccEvent(&event);
    EXPECT_EQ(root_obj.AxID(), event.update.nodes[0].id);
  }

  // If we reload the page and send a event, we should send
  // all 4 nodes to the browser. Also double-check that we didn't
  // leak any of the old BrowserTreeNodes.
  LoadHTML(html.c_str());
  document = GetMainFrame()->GetDocument();
  root_obj = WebAXObject::FromWebDocument(document);
  sink_->ClearMessages();
  accessibility->HandleAXEvent(root_obj, ax::mojom::Event::kLayoutComplete);
  accessibility->SendPendingAccessibilityEvents();
  EXPECT_EQ(4, CountAccessibilityNodesSentToBrowser());

  // Even if the first event is sent on an element other than
  // the root, the whole tree should be updated because we know
  // the browser doesn't have the root element.
  LoadHTML(html.c_str());
  document = GetMainFrame()->GetDocument();
  root_obj = WebAXObject::FromWebDocument(document);
  sink_->ClearMessages();
  const WebAXObject& first_child = root_obj.ChildAt(0);
  accessibility->HandleAXEvent(first_child,
                               ax::mojom::Event::kLiveRegionChanged);
  accessibility->SendPendingAccessibilityEvents();
  EXPECT_EQ(4, CountAccessibilityNodesSentToBrowser());
}

TEST_F(RenderAccessibilityImplTest, HideAccessibilityObject) {
  // Test RenderAccessibilityImpl and make sure it sends the
  // proper event to the browser when an object in the tree
  // is hidden, but its children are not.
  std::string html =
      "<body>"
      "  <div role='group' id='A'>"
      "    <div role='group' id='B'>"
      "      <div role='group' id='C' style='visibility:visible'>"
      "      </div>"
      "    </div>"
      "  </div>"
      "</body>";
  LoadHTML(html.c_str());

  std::unique_ptr<TestRenderAccessibilityImpl> accessibility(
      new TestRenderAccessibilityImpl(frame()));
  accessibility->SendPendingAccessibilityEvents();
  EXPECT_EQ(4, CountAccessibilityNodesSentToBrowser());

  WebDocument document = GetMainFrame()->GetDocument();
  WebAXObject root_obj = WebAXObject::FromWebDocument(document);
  WebAXObject node_a = root_obj.ChildAt(0);
  WebAXObject node_b = node_a.ChildAt(0);
  WebAXObject node_c = node_b.ChildAt(0);

  // Hide node 'B' ('C' stays visible).
  ExecuteJavaScriptForTests(
      "document.getElementById('B').style.visibility = 'hidden';");
  // Force layout now.
  ExecuteJavaScriptForTests("document.getElementById('B').offsetLeft;");

  // Send a childrenChanged on 'A'.
  sink_->ClearMessages();
  accessibility->HandleAXEvent(node_a, ax::mojom::Event::kChildrenChanged);

  accessibility->SendPendingAccessibilityEvents();
  AccessibilityHostMsg_EventParams event;
  GetLastAccEvent(&event);
  ASSERT_EQ(2U, event.update.nodes.size());

  // RenderAccessibilityImpl notices that 'C' is being reparented,
  // so it clears the subtree rooted at 'A', then updates 'A' and then 'C'.
  EXPECT_EQ(node_a.AxID(), event.update.node_id_to_clear);
  EXPECT_EQ(node_a.AxID(), event.update.nodes[0].id);
  EXPECT_EQ(node_c.AxID(), event.update.nodes[1].id);
  EXPECT_EQ(2, CountAccessibilityNodesSentToBrowser());
}

TEST_F(RenderAccessibilityImplTest, ShowAccessibilityObject) {
  // Test RenderAccessibilityImpl and make sure it sends the
  // proper event to the browser when an object in the tree
  // is shown, causing its own already-visible children to be
  // reparented to it.
  std::string html =
      "<body>"
      "  <div role='group' id='A'>"
      "    <div role='group' id='B' style='visibility:hidden'>"
      "      <div role='group' id='C' style='visibility:visible'>"
      "      </div>"
      "    </div>"
      "  </div>"
      "</body>";
  LoadHTML(html.c_str());

  std::unique_ptr<TestRenderAccessibilityImpl> accessibility(
      new TestRenderAccessibilityImpl(frame()));
  accessibility->SendPendingAccessibilityEvents();
  EXPECT_EQ(3, CountAccessibilityNodesSentToBrowser());

  // Show node 'B', then send a childrenChanged on 'A'.
  ExecuteJavaScriptForTests(
      "document.getElementById('B').style.visibility = 'visible';");
  ExecuteJavaScriptForTests("document.getElementById('B').offsetLeft;");

  sink_->ClearMessages();
  WebDocument document = GetMainFrame()->GetDocument();
  WebAXObject root_obj = WebAXObject::FromWebDocument(document);
  WebAXObject node_a = root_obj.ChildAt(0);
  WebAXObject node_b = node_a.ChildAt(0);
  WebAXObject node_c = node_b.ChildAt(0);

  accessibility->HandleAXEvent(node_a, ax::mojom::Event::kChildrenChanged);

  accessibility->SendPendingAccessibilityEvents();
  AccessibilityHostMsg_EventParams event;
  GetLastAccEvent(&event);

  ASSERT_EQ(3U, event.update.nodes.size());
  EXPECT_EQ(node_a.AxID(), event.update.node_id_to_clear);
  EXPECT_EQ(node_a.AxID(), event.update.nodes[0].id);
  EXPECT_EQ(node_b.AxID(), event.update.nodes[1].id);
  EXPECT_EQ(node_c.AxID(), event.update.nodes[2].id);
  EXPECT_EQ(3, CountAccessibilityNodesSentToBrowser());
}

}  // namespace content
