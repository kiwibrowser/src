// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/accessibility/browser_accessibility_mac.h"

#import <Cocoa/Cocoa.h>

#include <memory>
#include <string>
#include <vector>

#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"
#include "content/browser/accessibility/browser_accessibility_cocoa.h"
#include "content/browser/accessibility/browser_accessibility_manager.h"
#include "content/browser/accessibility/browser_accessibility_manager_mac.h"
#include "content/public/browser/ax_event_notification_details.h"
#include "testing/gtest/include/gtest/gtest.h"
#import "testing/gtest_mac.h"
#include "ui/accessibility/ax_tree_update.h"
#import "ui/base/test/cocoa_helper.h"

namespace content {

class BrowserAccessibilityTest : public ui::CocoaTest {
 public:
  void SetUp() override {
    CocoaTest::SetUp();
    RebuildAccessibilityTree();
  }

 protected:
  void RebuildAccessibilityTree() {
    // Clean out the existing root data in case this method is called multiple
    // times in a test.
    root_ = ui::AXNodeData();
    root_.id = 1000;
    root_.location.set_width(500);
    root_.location.set_height(100);
    root_.role = ax::mojom::Role::kRootWebArea;
    root_.AddStringAttribute(ax::mojom::StringAttribute::kDescription,
                             "HelpText");
    root_.child_ids.push_back(1001);
    root_.child_ids.push_back(1002);

    ui::AXNodeData child1;
    child1.id = 1001;
    child1.SetName("Child1");
    child1.location.set_width(250);
    child1.location.set_height(100);
    child1.role = ax::mojom::Role::kButton;

    ui::AXNodeData child2;
    child2.id = 1002;
    child2.location.set_x(250);
    child2.location.set_width(250);
    child2.location.set_height(100);
    child2.role = ax::mojom::Role::kHeading;

    manager_.reset(new BrowserAccessibilityManagerMac(
        MakeAXTreeUpdate(root_, child1, child2), nullptr));
    accessibility_.reset([ToBrowserAccessibilityCocoa(manager_->GetRoot())
        retain]);
  }

  void SetRootValue(std::string value) {
    if (!manager_)
      return;
    root_.SetValue(value);
    AXEventNotificationDetails param;
    param.update.nodes.push_back(root_);
    param.event_type = ax::mojom::Event::kValueChanged;
    param.id = root_.id;
    std::vector<AXEventNotificationDetails> events{param};
    manager_->OnAccessibilityEvents(events);
  }

  ui::AXNodeData root_;
  base::scoped_nsobject<BrowserAccessibilityCocoa> accessibility_;
  std::unique_ptr<BrowserAccessibilityManager> manager_;
};

// Standard hit test.
TEST_F(BrowserAccessibilityTest, HitTestTest) {
  BrowserAccessibilityCocoa* firstChild =
      [accessibility_ accessibilityHitTest:NSMakePoint(50, 50)];
  EXPECT_NSEQ(@"Child1",
      [firstChild
       accessibilityAttributeValue:NSAccessibilityDescriptionAttribute]);
}

// Test doing a hit test on the edge of a child.
TEST_F(BrowserAccessibilityTest, EdgeHitTest) {
  BrowserAccessibilityCocoa* firstChild =
      [accessibility_ accessibilityHitTest:NSZeroPoint];
  EXPECT_NSEQ(@"Child1",
      [firstChild
       accessibilityAttributeValue:NSAccessibilityDescriptionAttribute]);
}

// This will test a hit test with invalid coordinates.  It is assumed that
// the hit test has been narrowed down to this object or one of its children
// so it should return itself since it has no better hit result.
TEST_F(BrowserAccessibilityTest, InvalidHitTestCoordsTest) {
  BrowserAccessibilityCocoa* hitTestResult =
      [accessibility_ accessibilityHitTest:NSMakePoint(-50, 50)];
  EXPECT_NSEQ(accessibility_, hitTestResult);
}

// Test to ensure querying standard attributes works.
TEST_F(BrowserAccessibilityTest, BasicAttributeTest) {
  NSString* helpText = [accessibility_
      accessibilityAttributeValue:NSAccessibilityHelpAttribute];
  EXPECT_NSEQ(@"HelpText", helpText);
}

// Test querying for an invalid attribute to ensure it doesn't crash.
TEST_F(BrowserAccessibilityTest, InvalidAttributeTest) {
  NSString* shouldBeNil = [accessibility_
      accessibilityAttributeValue:@"NSAnInvalidAttribute"];
  EXPECT_TRUE(shouldBeNil == nil);
}

TEST_F(BrowserAccessibilityTest, RetainedDetachedObjectsReturnNil) {
  // Get the first child.
  BrowserAccessibilityCocoa* retainedFirstChild =
      [accessibility_ accessibilityHitTest:NSMakePoint(50, 50)];
  EXPECT_NSEQ(@"Child1", [retainedFirstChild
      accessibilityAttributeValue:NSAccessibilityDescriptionAttribute]);

  // Retain it. This simulates what the system might do with an
  // accessibility object.
  [retainedFirstChild retain];

  // Rebuild the accessibility tree, which should detach |retainedFirstChild|.
  RebuildAccessibilityTree();

  // Now any attributes we query should return nil.
  EXPECT_EQ(nil, [retainedFirstChild
      accessibilityAttributeValue:NSAccessibilityDescriptionAttribute]);

  // Don't leak memory in the test.
  [retainedFirstChild release];
}

TEST_F(BrowserAccessibilityTest, TestComputeTextEdit) {
  BrowserAccessibility* wrapper = [accessibility_ browserAccessibility];
  ASSERT_NE(nullptr, wrapper);

  // Insertion but no deletion.

  SetRootValue("text");
  AXTextEdit text_edit = [accessibility_ computeTextEdit];
  EXPECT_EQ(base::UTF8ToUTF16("text"), text_edit.inserted_text);
  EXPECT_TRUE(text_edit.deleted_text.empty());

  SetRootValue("new text");
  text_edit = [accessibility_ computeTextEdit];
  EXPECT_EQ(base::UTF8ToUTF16("new "), text_edit.inserted_text);
  EXPECT_TRUE(text_edit.deleted_text.empty());

  SetRootValue("new text hello");
  text_edit = [accessibility_ computeTextEdit];
  EXPECT_EQ(base::UTF8ToUTF16(" hello"), text_edit.inserted_text);
  EXPECT_TRUE(text_edit.deleted_text.empty());

  SetRootValue("newer text hello");
  text_edit = [accessibility_ computeTextEdit];
  EXPECT_EQ(base::UTF8ToUTF16("er"), text_edit.inserted_text);
  EXPECT_TRUE(text_edit.deleted_text.empty());

  // Deletion but no insertion.

  SetRootValue("new text hello");
  text_edit = [accessibility_ computeTextEdit];
  EXPECT_EQ(base::UTF8ToUTF16("er"), text_edit.deleted_text);
  EXPECT_TRUE(text_edit.inserted_text.empty());

  SetRootValue("new text");
  text_edit = [accessibility_ computeTextEdit];
  EXPECT_EQ(base::UTF8ToUTF16(" hello"), text_edit.deleted_text);
  EXPECT_TRUE(text_edit.inserted_text.empty());

  SetRootValue("text");
  text_edit = [accessibility_ computeTextEdit];
  EXPECT_EQ(base::UTF8ToUTF16("new "), text_edit.deleted_text);
  EXPECT_TRUE(text_edit.inserted_text.empty());

  SetRootValue("");
  text_edit = [accessibility_ computeTextEdit];
  EXPECT_EQ(base::UTF8ToUTF16("text"), text_edit.deleted_text);
  EXPECT_TRUE(text_edit.inserted_text.empty());

  // Both insertion and deletion.

  SetRootValue("new text hello");
  text_edit = [accessibility_ computeTextEdit];
  SetRootValue("new word hello");
  text_edit = [accessibility_ computeTextEdit];
  EXPECT_EQ(base::UTF8ToUTF16("text"), text_edit.deleted_text);
  EXPECT_EQ(base::UTF8ToUTF16("word"), text_edit.inserted_text);

  SetRootValue("new word there");
  text_edit = [accessibility_ computeTextEdit];
  EXPECT_EQ(base::UTF8ToUTF16("hello"), text_edit.deleted_text);
  EXPECT_EQ(base::UTF8ToUTF16("there"), text_edit.inserted_text);

  SetRootValue("old word there");
  text_edit = [accessibility_ computeTextEdit];
  EXPECT_EQ(base::UTF8ToUTF16("new"), text_edit.deleted_text);
  EXPECT_EQ(base::UTF8ToUTF16("old"), text_edit.inserted_text);
}

}  // namespace content
