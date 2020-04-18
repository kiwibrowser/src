// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <Carbon/Carbon.h>  // For the kVK_* constants.

#include <memory>

#include "base/json/json_reader.h"
#include "base/values.h"
#import "chrome/browser/ui/cocoa/autofill/save_card_bubble_view_bridge.h"
#import "chrome/browser/ui/cocoa/browser_window_controller.h"
#include "chrome/browser/ui/cocoa/test/cocoa_profile_test.h"
#include "components/autofill/core/browser/credit_card.h"
#include "components/autofill/core/browser/ui/save_card_bubble_controller.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#import "ui/events/test/cocoa_test_event_utils.h"

namespace autofill {

namespace {

class TestSaveCardBubbleController : public SaveCardBubbleController {
 public:
  TestSaveCardBubbleController() {
    ParseLegalMessageJson();

    on_save_button_was_called_ = false;
    on_cancel_button_was_called_ = false;
    on_learn_more_was_called_ = false;
    on_legal_message_was_called_ = false;
    on_bubble_closed_was_called_ = false;
  }

  // SaveCardBubbleController:
  MOCK_CONST_METHOD0(GetWindowTitle, base::string16());
  MOCK_CONST_METHOD0(GetExplanatoryMessage, base::string16());
  MOCK_CONST_METHOD0(GetCard, const CreditCard());

  void OnSaveButton() override { on_save_button_was_called_ = true; }
  void OnCancelButton() override { on_cancel_button_was_called_ = true; }
  void OnLearnMoreClicked() override { on_learn_more_was_called_ = true; }
  void OnLegalMessageLinkClicked(const GURL& url) override {
    on_legal_message_was_called_ = true;
    legal_message_url_ = url.spec();
  }
  void OnBubbleClosed() override { on_bubble_closed_was_called_ = true; }

  const LegalMessageLines& GetLegalMessageLines() const override {
    return lines_;
  }

  // Testing state.
  bool on_save_button_was_called() { return on_save_button_was_called_; }
  bool on_cancel_button_was_called() { return on_cancel_button_was_called_; }
  bool on_learn_more_was_called() { return on_learn_more_was_called_; }
  bool on_legal_message_was_called() { return on_legal_message_was_called_; }
  std::string legal_message_url() { return legal_message_url_; }
  bool on_bubble_closed_was_called() { return on_bubble_closed_was_called_; }

 private:
  void ParseLegalMessageJson() {
    std::string message_json =
        "{"
        "  \"line\" : ["
        "    {"
        "      \"template\": \"Please check out our {0}.\","
        "      \"template_parameter\": ["
        "        {"
        "          \"display_text\": \"terms of service\","
        "          \"url\": \"http://help.example.com/legal_message\""
        "        }"
        "      ]"
        "    },"
        "    {"
        "      \"template\": \"We also have a {0} and {1}.\","
        "      \"template_parameter\": ["
        "        {"
        "          \"display_text\": \"mission statement\","
        "          \"url\": \"http://www.example.com/our_mission\""
        "        },"
        "        {"
        "          \"display_text\": \"privacy policy\","
        "          \"url\": \"http://help.example.com/privacy_policy\""
        "        }"
        "      ]"
        "    }"
        "  ]"
        "}";
    std::unique_ptr<base::Value> value(base::JSONReader::Read(message_json));
    ASSERT_TRUE(value);
    base::DictionaryValue* dictionary = nullptr;
    ASSERT_TRUE(value->GetAsDictionary(&dictionary));
    LegalMessageLine::Parse(*dictionary, &lines_);
  }

  LegalMessageLines lines_;

  bool on_save_button_was_called_;
  bool on_cancel_button_was_called_;
  bool on_learn_more_was_called_;
  bool on_legal_message_was_called_;
  std::string legal_message_url_;
  bool on_bubble_closed_was_called_;
};

class SaveCardBubbleViewTest : public CocoaProfileTest {
 public:
  void SetUp() override {
    CocoaProfileTest::SetUp();
    ASSERT_TRUE(browser());

    browser_window_controller_ =
        [[BrowserWindowController alloc] initWithBrowser:browser()
                                           takeOwnership:NO];

    bubble_controller_.reset(new TestSaveCardBubbleController());

    // This will show the SaveCardBubbleViewCocoa.
    bridge_ = new SaveCardBubbleViewBridge(bubble_controller_.get(),
                                           browser_window_controller_);
  }

  void TearDown() override {
    [[browser_window_controller_ nsWindowController] close];
    CocoaProfileTest::TearDown();
  }

 protected:
  BrowserWindowController* browser_window_controller_;
  std::unique_ptr<TestSaveCardBubbleController> bubble_controller_;
  SaveCardBubbleViewBridge* bridge_;
};

}  // namespace

TEST_F(SaveCardBubbleViewTest, SaveShouldClose) {
  [bridge_->view_controller_ onSaveButton:nil];

  EXPECT_TRUE(bubble_controller_->on_save_button_was_called());
  EXPECT_FALSE(bubble_controller_->on_cancel_button_was_called());
  EXPECT_FALSE(bubble_controller_->on_learn_more_was_called());
  EXPECT_FALSE(bubble_controller_->on_legal_message_was_called());

  EXPECT_TRUE(bubble_controller_->on_bubble_closed_was_called());
}

TEST_F(SaveCardBubbleViewTest, CancelShouldClose) {
  [bridge_->view_controller_ onCancelButton:nil];

  EXPECT_FALSE(bubble_controller_->on_save_button_was_called());
  EXPECT_TRUE(bubble_controller_->on_cancel_button_was_called());
  EXPECT_FALSE(bubble_controller_->on_learn_more_was_called());
  EXPECT_FALSE(bubble_controller_->on_legal_message_was_called());

  EXPECT_TRUE(bubble_controller_->on_bubble_closed_was_called());
}

TEST_F(SaveCardBubbleViewTest, LearnMoreShouldNotClose) {
  NSTextView* textView = nil;
  NSObject* link = nil;
  [bridge_->view_controller_ textView:textView clickedOnLink:link atIndex:0];

  EXPECT_FALSE(bubble_controller_->on_save_button_was_called());
  EXPECT_FALSE(bubble_controller_->on_cancel_button_was_called());
  EXPECT_TRUE(bubble_controller_->on_learn_more_was_called());
  EXPECT_FALSE(bubble_controller_->on_legal_message_was_called());

  EXPECT_FALSE(bubble_controller_->on_bubble_closed_was_called());
}

TEST_F(SaveCardBubbleViewTest, LegalMessageShouldNotClose) {
  NSString* legalText = @"We also have a mission statement and privacy policy.";
  base::scoped_nsobject<NSTextView> textView(
      [[NSTextView alloc] initWithFrame:NSZeroRect]);
  base::scoped_nsobject<NSAttributedString> attributedMessage(
      [[NSAttributedString alloc] initWithString:legalText attributes:@{}]);
  [[textView textStorage] setAttributedString:attributedMessage];

  NSObject* link = nil;
  [bridge_->view_controller_ textView:textView clickedOnLink:link atIndex:40];

  EXPECT_FALSE(bubble_controller_->on_save_button_was_called());
  EXPECT_FALSE(bubble_controller_->on_cancel_button_was_called());
  EXPECT_FALSE(bubble_controller_->on_learn_more_was_called());
  EXPECT_TRUE(bubble_controller_->on_legal_message_was_called());

  std::string url("http://help.example.com/privacy_policy");
  EXPECT_EQ(url, bubble_controller_->legal_message_url());

  EXPECT_FALSE(bubble_controller_->on_bubble_closed_was_called());
}

TEST_F(SaveCardBubbleViewTest, ReturnInvokesDefaultAction) {
  [[bridge_->view_controller_ window]
      performKeyEquivalent:cocoa_test_event_utils::KeyEventWithKeyCode(
                               kVK_Return, '\r', NSKeyDown, 0)];

  EXPECT_TRUE(bubble_controller_->on_save_button_was_called());
  EXPECT_TRUE(bubble_controller_->on_bubble_closed_was_called());
}

TEST_F(SaveCardBubbleViewTest, EscapeCloses) {
  [[bridge_->view_controller_ window]
      performKeyEquivalent:cocoa_test_event_utils::KeyEventWithKeyCode(
                               kVK_Escape, '\e', NSKeyDown, 0)];

  EXPECT_TRUE(bubble_controller_->on_bubble_closed_was_called());
}

}  // namespace autofill
