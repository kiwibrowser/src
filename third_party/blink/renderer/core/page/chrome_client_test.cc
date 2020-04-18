// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/page/chrome_client.h"

#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/blink/renderer/core/dom/document.h"
#include "third_party/blink/renderer/core/html/html_element.h"
#include "third_party/blink/renderer/core/html_names.h"
#include "third_party/blink/renderer/core/layout/hit_test_result.h"
#include "third_party/blink/renderer/core/loader/empty_clients.h"

namespace blink {

namespace {

class ChromeClientToolTipLogger : public EmptyChromeClient {
 public:
  void SetToolTip(LocalFrame&, const String& text, TextDirection) override {
    tool_tip_for_last_set_tool_tip_ = text;
  }

  String ToolTipForLastSetToolTip() const {
    return tool_tip_for_last_set_tool_tip_;
  }
  void ClearToolTipForLastSetToolTip() {
    tool_tip_for_last_set_tool_tip_ = String();
  }

 private:
  String tool_tip_for_last_set_tool_tip_;
};

}  // anonymous namespace

class ChromeClientTest : public testing::Test {};

TEST_F(ChromeClientTest, SetToolTipFlood) {
  ChromeClientToolTipLogger logger;
  ChromeClient* client = &logger;
  HitTestResult result(HitTestRequest(HitTestRequest::kMove),
                       LayoutPoint(10, 20));
  Document* doc = Document::CreateForTest();
  Element* element = HTMLElement::Create(HTMLNames::divTag, *doc);
  element->setAttribute(HTMLNames::titleAttr, "tooltip");
  result.SetInnerNode(element);

  client->SetToolTip(*doc->GetFrame(), result);
  EXPECT_EQ("tooltip", logger.ToolTipForLastSetToolTip());

  // seToolTip(HitTestResult) again in the same condition.
  logger.ClearToolTipForLastSetToolTip();
  client->SetToolTip(*doc->GetFrame(), result);
  // setToolTip(String,TextDirection) should not be called.
  EXPECT_EQ(String(), logger.ToolTipForLastSetToolTip());

  // Cancel the tooltip, and setToolTip(HitTestResult) again.
  client->ClearToolTip(*doc->GetFrame());
  logger.ClearToolTipForLastSetToolTip();
  client->SetToolTip(*doc->GetFrame(), result);
  // setToolTip(String,TextDirection) should not be called.
  EXPECT_EQ(String(), logger.ToolTipForLastSetToolTip());

  logger.ClearToolTipForLastSetToolTip();
  element->setAttribute(HTMLNames::titleAttr, "updated");
  client->SetToolTip(*doc->GetFrame(), result);
  // setToolTip(String,TextDirection) should be called because tooltip string
  // is different from the last one.
  EXPECT_EQ("updated", logger.ToolTipForLastSetToolTip());
}

}  // namespace blink
