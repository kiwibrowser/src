// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <tuple>

#include "base/macros.h"
#include "base/strings/utf_string_conversions.h"
#include "content/common/frame_messages.h"
#include "content/public/test/render_view_test.h"
#include "content/renderer/render_frame_impl.h"
#include "content/renderer/render_view_impl.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/blink/public/platform/web_size.h"
#include "third_party/blink/public/web/web_view.h"

// Tests for the external select popup menu (Mac specific).

namespace content {
namespace {

const char* const kSelectID = "mySelect";
const char* const kEmptySelectID = "myEmptySelect";

}  // namespace

class ExternalPopupMenuTest : public RenderViewTest {
 public:
  ExternalPopupMenuTest() {}

  RenderViewImpl* view() {
    return static_cast<RenderViewImpl*>(view_);
  }

  RenderFrameImpl* frame() {
    return view()->GetMainRenderFrame();
  }

  void SetUp() override {
    RenderViewTest::SetUp();
    // We need to set this explictly as RenderMain is not run.
    blink::WebView::SetUseExternalPopupMenus(true);

    std::string html = "<select id='mySelect' onchange='selectChanged(this)'>"
                       "  <option>zero</option>"
                       "  <option selected='1'>one</option>"
                       "  <option>two</option>"
                       "</select>"
                       "<select id='myEmptySelect'>"
                       "</select>";
    if (ShouldRemoveSelectOnChange()) {
      html += "<script>"
              "  function selectChanged(select) {"
              "    select.parentNode.removeChild(select);"
              "  }"
              "</script>";
    }

    // Load the test page.
    LoadHTML(html.c_str());

    // Set a minimum size and give focus so simulated events work.
    view()->GetWidget()->GetWebWidget()->Resize(blink::WebSize(500, 500));
    view()->GetWidget()->GetWebWidget()->SetFocus(true);
  }

  int GetSelectedIndex() {
    base::string16 script(base::ASCIIToUTF16(kSelectID));
    script.append(base::ASCIIToUTF16(".selectedIndex"));
    int selected_index = -1;
    ExecuteJavaScriptAndReturnIntValue(script, &selected_index);
    return selected_index;
  }

 protected:
  virtual bool ShouldRemoveSelectOnChange() const { return false; }

  DISALLOW_COPY_AND_ASSIGN(ExternalPopupMenuTest);
};

// Normal case: test showing a select popup, canceling/selecting an item.
TEST_F(ExternalPopupMenuTest, NormalCase) {
  IPC::TestSink& sink = render_thread_->sink();

  // Click the text field once.
  EXPECT_TRUE(SimulateElementClick(kSelectID));

  // We should have sent a message to the browser to show the popup menu.
  const IPC::Message* message =
      sink.GetUniqueMessageMatching(FrameHostMsg_ShowPopup::ID);
  ASSERT_TRUE(message != NULL);
  std::tuple<FrameHostMsg_ShowPopup_Params> param;
  FrameHostMsg_ShowPopup::Read(message, &param);
  ASSERT_EQ(3U, std::get<0>(param).popup_items.size());
  EXPECT_EQ(1, std::get<0>(param).selected_item);

  // Simulate the user canceling the popup; the index should not have changed.
  frame()->OnSelectPopupMenuItem(-1);
  EXPECT_EQ(1, GetSelectedIndex());

  // Show the pop-up again and this time make a selection.
  EXPECT_TRUE(SimulateElementClick(kSelectID));
  frame()->OnSelectPopupMenuItem(0);
  EXPECT_EQ(0, GetSelectedIndex());

  // Show the pop-up again and make another selection.
  sink.ClearMessages();
  EXPECT_TRUE(SimulateElementClick(kSelectID));
  message = sink.GetUniqueMessageMatching(FrameHostMsg_ShowPopup::ID);
  ASSERT_TRUE(message != NULL);
  FrameHostMsg_ShowPopup::Read(message, &param);
  ASSERT_EQ(3U, std::get<0>(param).popup_items.size());
  EXPECT_EQ(0, std::get<0>(param).selected_item);
}

// Page shows popup, then navigates away while popup showing, then select.
TEST_F(ExternalPopupMenuTest, ShowPopupThenNavigate) {
  // Click the text field once.
  EXPECT_TRUE(SimulateElementClick(kSelectID));

  // Now we navigate to another pager.
  LoadHTML("<blink>Awesome page!</blink>");

  // Now the user selects something, we should not crash.
  frame()->OnSelectPopupMenuItem(-1);
}

// An empty select should not cause a crash when clicked.
// http://crbug.com/63774
TEST_F(ExternalPopupMenuTest, EmptySelect) {
  EXPECT_TRUE(SimulateElementClick(kEmptySelectID));
}

class ExternalPopupMenuRemoveTest : public ExternalPopupMenuTest {
 public:
  ExternalPopupMenuRemoveTest() {}

 protected:
  bool ShouldRemoveSelectOnChange() const override { return true; }
};

// Tests that nothing bad happen when the page removes the select when it
// changes. (http://crbug.com/61997)
TEST_F(ExternalPopupMenuRemoveTest, RemoveOnChange) {
  // Click the text field once to show the popup.
  EXPECT_TRUE(SimulateElementClick(kSelectID));

  // Select something, it causes the select to be removed from the page.
  frame()->OnSelectPopupMenuItem(0);

  // Just to check the soundness of the test, call SimulateElementClick again.
  // It should return false as the select has been removed.
  EXPECT_FALSE(SimulateElementClick(kSelectID));
}

class ExternalPopupMenuDisplayNoneTest : public ExternalPopupMenuTest {
  public:
  ExternalPopupMenuDisplayNoneTest() {}

  void SetUp() override {
    RenderViewTest::SetUp();
    // We need to set this explictly as RenderMain is not run.
    blink::WebView::SetUseExternalPopupMenus(true);

    std::string html = "<select id='mySelect'>"
                       "  <option value='zero'>zero</option>"
                       "  <optgroup label='hide' style='display: none'>"
                       "    <option value='one'>one</option>"
                       "  </optgroup>"
                       "  <option value='two'>two</option>"
                       "  <option value='three'>three</option>"
                       "  <option value='four'>four</option>"
                       "  <option value='five'>five</option>"
                       "</select>";
    // Load the test page.
    LoadHTML(html.c_str());

    // Set a minimum size and give focus so simulated events work.
    view()->GetWidget()->GetWebWidget()->Resize(blink::WebSize(500, 500));
    view()->GetWidget()->GetWebWidget()->SetFocus(true);
  }

};

TEST_F(ExternalPopupMenuDisplayNoneTest, SelectItem) {
  IPC::TestSink& sink = render_thread_->sink();

  // Click the text field once to show the popup.
  EXPECT_TRUE(SimulateElementClick(kSelectID));

  // Read the message sent to browser to show the popup menu.
  const IPC::Message* message =
      sink.GetUniqueMessageMatching(FrameHostMsg_ShowPopup::ID);
  ASSERT_TRUE(message != NULL);
  std::tuple<FrameHostMsg_ShowPopup_Params> param;
  FrameHostMsg_ShowPopup::Read(message, &param);
  // Number of items should match item count minus the number
  // of "display: none" items.
  ASSERT_EQ(5U, std::get<0>(param).popup_items.size());

  // Select index 1 item. This should select item with index 2,
  // skipping the item with 'display: none'
  frame()->OnSelectPopupMenuItem(1);

  EXPECT_EQ(2, GetSelectedIndex());
}

}  // namespace content
