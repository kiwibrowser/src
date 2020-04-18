// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "chrome/browser/ui/cocoa/extensions/toolbar_actions_bar_bubble_mac.h"
#include "base/logging.h"
#import "base/mac/foundation_util.h"
#import "base/mac/scoped_nsobject.h"
#include "base/macros.h"
#include "base/strings/utf_string_conversions.h"
#import "chrome/browser/ui/cocoa/test/cocoa_test_helper.h"
#include "chrome/browser/ui/cocoa/test/run_loop_testing.h"
#include "chrome/browser/ui/toolbar/test_toolbar_actions_bar_bubble_delegate.h"
#include "chrome/grit/generated_resources.h"
#include "components/vector_icons/vector_icons.h"
#include "ui/base/l10n/l10n_util.h"
#import "ui/events/test/cocoa_test_event_utils.h"

// A simple class to observe when a window is destructing.
@interface WindowObserver : NSObject {
  BOOL windowIsClosing_;
}

- (id)initWithWindow:(NSWindow*)window;

- (void)dealloc;

- (void)onWindowClosing:(NSNotification*)notification;

@property(nonatomic, assign) BOOL windowIsClosing;

@end

@implementation WindowObserver

- (id)initWithWindow:(NSWindow*)window {
  if ((self = [super init])) {
    [[NSNotificationCenter defaultCenter]
        addObserver:self
           selector:@selector(onWindowClosing:)
               name:NSWindowWillCloseNotification
             object:window];
  }
  return self;
}

- (void)dealloc {
  [[NSNotificationCenter defaultCenter] removeObserver:self];
  [super dealloc];
}

- (void)onWindowClosing:(NSNotification*)notification {
  windowIsClosing_ = YES;
}

@synthesize windowIsClosing = windowIsClosing_;

@end

class ToolbarActionsBarBubbleMacTest : public CocoaTest {
 public:
  ToolbarActionsBarBubbleMacTest() {}
  ~ToolbarActionsBarBubbleMacTest() override {}

  void SetUp() override;

  // Create and display a new bubble with the given |delegate|.
  ToolbarActionsBarBubbleMac* CreateAndShowBubble(
      TestToolbarActionsBarBubbleDelegate* delegate);

  // Test that clicking on the corresponding button produces the
  // |expected_action|, and closes the bubble.
  void TestBubbleButton(
      ToolbarActionsBarBubbleDelegate::CloseAction expected_action);

  base::string16 HeadingString() { return base::ASCIIToUTF16("Heading"); }
  base::string16 BodyString() { return base::ASCIIToUTF16("Body"); }
  base::string16 ActionString() { return base::ASCIIToUTF16("Action"); }
  base::string16 DismissString() { return base::ASCIIToUTF16("Dismiss"); }
  base::string16 LearnMoreString() { return base::ASCIIToUTF16("LearnMore"); }
  base::string16 ItemListString() { return base::ASCIIToUTF16("ItemList"); }

 private:
  DISALLOW_COPY_AND_ASSIGN(ToolbarActionsBarBubbleMacTest);
};

void ToolbarActionsBarBubbleMacTest::SetUp() {
  CocoaTest::SetUp();
  [ToolbarActionsBarBubbleMac setAnimationEnabledForTesting:NO];
}

ToolbarActionsBarBubbleMac* ToolbarActionsBarBubbleMacTest::CreateAndShowBubble(
    TestToolbarActionsBarBubbleDelegate* delegate) {
  ToolbarActionsBarBubbleMac* bubble =
      [[ToolbarActionsBarBubbleMac alloc]
          initWithParentWindow:test_window()
                   anchorPoint:NSZeroPoint
              anchoredToAction:NO
                      delegate:delegate->GetDelegate()];
  EXPECT_FALSE(delegate->shown());
  [bubble showWindow:nil];
  chrome::testing::NSRunLoopRunAllPending();
  EXPECT_FALSE(delegate->close_action());
  EXPECT_TRUE(delegate->shown());
  return bubble;
}

void ToolbarActionsBarBubbleMacTest::TestBubbleButton(
    ToolbarActionsBarBubbleDelegate::CloseAction expected_action) {
  TestToolbarActionsBarBubbleDelegate delegate(
      HeadingString(), BodyString(), ActionString());
  delegate.set_dismiss_button_text(DismissString());

  std::unique_ptr<ToolbarActionsBarBubbleDelegate::ExtraViewInfo>
      extra_view_info_linked_text =
          std::make_unique<ToolbarActionsBarBubbleDelegate::ExtraViewInfo>();
  extra_view_info_linked_text->text = LearnMoreString();
  extra_view_info_linked_text->is_learn_more = true;
  delegate.set_extra_view_info(std::move(extra_view_info_linked_text));

  ToolbarActionsBarBubbleMac* bubble = CreateAndShowBubble(&delegate);
  base::scoped_nsobject<WindowObserver> windowObserver(
      [[WindowObserver alloc] initWithWindow:[bubble window]]);
  EXPECT_FALSE([windowObserver windowIsClosing]);

  // Find the appropriate button to click.
  NSButton* button = nil;
  switch (expected_action) {
    case ToolbarActionsBarBubbleDelegate::CLOSE_EXECUTE:
      button = [bubble actionButton];
      break;
    case ToolbarActionsBarBubbleDelegate::CLOSE_DISMISS_USER_ACTION:
      button = [bubble dismissButton];
      break;
    case ToolbarActionsBarBubbleDelegate::CLOSE_LEARN_MORE:
      button = [bubble link];
      break;
    case ToolbarActionsBarBubbleDelegate::CLOSE_DISMISS_DEACTIVATION:
      NOTREACHED();  // Deactivation is tested below.
      break;
  }
  ASSERT_TRUE(button);

  // Click the button.
  std::pair<NSEvent*, NSEvent*> events =
      cocoa_test_event_utils::MouseClickInView(button, 1);
  [NSApp postEvent:events.second atStart:YES];
  [NSApp sendEvent:events.first];
  chrome::testing::NSRunLoopRunAllPending();

  // The bubble should be closed, and the delegate should be told that the
  // button was clicked.
  ASSERT_TRUE(delegate.close_action());
  EXPECT_EQ(expected_action, *delegate.close_action());
  EXPECT_TRUE([windowObserver windowIsClosing]);
}

// Test clicking on the action button and dismissing the bubble.
TEST_F(ToolbarActionsBarBubbleMacTest, CloseActionAndDismiss) {
  // Test all the possible actions.
  TestBubbleButton(ToolbarActionsBarBubbleDelegate::CLOSE_EXECUTE);
  TestBubbleButton(ToolbarActionsBarBubbleDelegate::CLOSE_DISMISS_USER_ACTION);
  TestBubbleButton(ToolbarActionsBarBubbleDelegate::CLOSE_LEARN_MORE);
  {
    // Test dismissing the bubble without clicking the button.
    TestToolbarActionsBarBubbleDelegate delegate(
        HeadingString(), BodyString(), ActionString());
    ToolbarActionsBarBubbleMac* bubble = CreateAndShowBubble(&delegate);
    base::scoped_nsobject<WindowObserver> windowObserver(
        [[WindowObserver alloc] initWithWindow:[bubble window]]);
    EXPECT_FALSE([windowObserver windowIsClosing]);
    // Close the bubble. The delegate should be told it was dismissed.
    [bubble close];
    chrome::testing::NSRunLoopRunAllPending();
    ASSERT_TRUE(delegate.close_action());
    EXPECT_EQ(ToolbarActionsBarBubbleDelegate::CLOSE_DISMISS_DEACTIVATION,
              *delegate.close_action());
    EXPECT_TRUE([windowObserver windowIsClosing]);
  }
}

// Test the basic layout of the bubble.
TEST_F(ToolbarActionsBarBubbleMacTest, ToolbarActionsBarBubbleLayout) {
  // Test with no optional fields.
  {
    TestToolbarActionsBarBubbleDelegate delegate(
        HeadingString(), BodyString(), ActionString());
    ToolbarActionsBarBubbleMac* bubble = CreateAndShowBubble(&delegate);
    EXPECT_TRUE([bubble actionButton]);
    EXPECT_FALSE([bubble iconView]);
    EXPECT_FALSE([bubble label]);
    EXPECT_FALSE([bubble link]);
    EXPECT_FALSE([bubble dismissButton]);
    EXPECT_TRUE([bubble bodyText]);
    EXPECT_FALSE([bubble itemList]);

    [bubble close];
    chrome::testing::NSRunLoopRunAllPending();
  }

  // Test with all possible buttons (action, link, dismiss).
  {
    TestToolbarActionsBarBubbleDelegate delegate(
        HeadingString(), BodyString(), ActionString());

    std::unique_ptr<ToolbarActionsBarBubbleDelegate::ExtraViewInfo>
        extra_view_info_linked_text =
            std::make_unique<ToolbarActionsBarBubbleDelegate::ExtraViewInfo>();
    extra_view_info_linked_text->text = LearnMoreString();
    extra_view_info_linked_text->is_learn_more = true;
    delegate.set_extra_view_info(std::move(extra_view_info_linked_text));

    delegate.set_dismiss_button_text(DismissString());
    ToolbarActionsBarBubbleMac* bubble = CreateAndShowBubble(&delegate);
    EXPECT_TRUE([bubble actionButton]);
    EXPECT_FALSE([bubble iconView]);
    EXPECT_FALSE([bubble label]);
    EXPECT_TRUE([bubble link]);
    EXPECT_TRUE([bubble dismissButton]);
    EXPECT_TRUE([bubble bodyText]);
    EXPECT_FALSE([bubble itemList]);

    [bubble close];
    chrome::testing::NSRunLoopRunAllPending();
  }

  // Test with only a dismiss button (no action button).
  {
    TestToolbarActionsBarBubbleDelegate delegate(
        HeadingString(), BodyString(), base::string16());
    delegate.set_dismiss_button_text(DismissString());
    ToolbarActionsBarBubbleMac* bubble = CreateAndShowBubble(&delegate);
    EXPECT_FALSE([bubble actionButton]);
    EXPECT_FALSE([bubble iconView]);
    EXPECT_FALSE([bubble label]);
    EXPECT_FALSE([bubble link]);
    EXPECT_TRUE([bubble dismissButton]);
    EXPECT_TRUE([bubble bodyText]);
    EXPECT_FALSE([bubble itemList]);

    [bubble close];
    chrome::testing::NSRunLoopRunAllPending();
  }

  // Test with an action button and an item list.
  {
    TestToolbarActionsBarBubbleDelegate delegate(
        HeadingString(), BodyString(), ActionString());
    delegate.set_item_list_text(ItemListString());
    ToolbarActionsBarBubbleMac* bubble = CreateAndShowBubble(&delegate);
    EXPECT_TRUE([bubble actionButton]);
    EXPECT_FALSE([bubble iconView]);
    EXPECT_FALSE([bubble label]);
    EXPECT_FALSE([bubble link]);
    EXPECT_FALSE([bubble dismissButton]);
    EXPECT_TRUE([bubble bodyText]);
    EXPECT_TRUE([bubble itemList]);

    [bubble close];
    chrome::testing::NSRunLoopRunAllPending();
  }

  // Test with no body text.
  {
    TestToolbarActionsBarBubbleDelegate delegate(
        HeadingString(), base::string16(), ActionString());
    ToolbarActionsBarBubbleMac* bubble = CreateAndShowBubble(&delegate);
    EXPECT_TRUE([bubble actionButton]);
    EXPECT_FALSE([bubble iconView]);
    EXPECT_FALSE([bubble label]);
    EXPECT_FALSE([bubble link]);
    EXPECT_FALSE([bubble dismissButton]);
    EXPECT_FALSE([bubble bodyText]);
    EXPECT_FALSE([bubble itemList]);

    [bubble close];
    chrome::testing::NSRunLoopRunAllPending();
  }

  // Test with a null extra view.
  {
    TestToolbarActionsBarBubbleDelegate delegate(HeadingString(), BodyString(),
                                                 ActionString());
    ToolbarActionsBarBubbleMac* bubble = CreateAndShowBubble(&delegate);
    EXPECT_TRUE([bubble actionButton]);
    EXPECT_FALSE([bubble iconView]);
    EXPECT_FALSE([bubble label]);
    EXPECT_FALSE([bubble link]);
    EXPECT_FALSE([bubble dismissButton]);
    EXPECT_TRUE([bubble bodyText]);
    EXPECT_FALSE([bubble itemList]);

    [bubble close];
    chrome::testing::NSRunLoopRunAllPending();
  }

  // Test with an extra view of a (unlinked) text and icon and action button.
  {
    TestToolbarActionsBarBubbleDelegate delegate(HeadingString(), BodyString(),
                                                 ActionString());

    std::unique_ptr<ToolbarActionsBarBubbleDelegate::ExtraViewInfo>
        extra_view_info =
            std::make_unique<ToolbarActionsBarBubbleDelegate::ExtraViewInfo>();
    extra_view_info->resource = &vector_icons::kBusinessIcon;
    extra_view_info->text =
        l10n_util::GetStringUTF16(IDS_EXTENSIONS_INSTALLED_BY_ADMIN);
    extra_view_info->is_learn_more = false;
    delegate.set_extra_view_info(std::move(extra_view_info));

    ToolbarActionsBarBubbleMac* bubble = CreateAndShowBubble(&delegate);
    EXPECT_TRUE([bubble actionButton]);
    EXPECT_TRUE([bubble iconView]);
    EXPECT_TRUE([bubble label]);
    EXPECT_FALSE([bubble link]);
    EXPECT_FALSE([bubble dismissButton]);
    EXPECT_TRUE([bubble bodyText]);
    EXPECT_FALSE([bubble itemList]);

    [bubble close];
    chrome::testing::NSRunLoopRunAllPending();
  }

  // Test with all possible fields.
  {
    TestToolbarActionsBarBubbleDelegate delegate(
        HeadingString(), BodyString(), ActionString());

    std::unique_ptr<ToolbarActionsBarBubbleDelegate::ExtraViewInfo>
        extra_view_info_linked_text =
            std::make_unique<ToolbarActionsBarBubbleDelegate::ExtraViewInfo>();
    extra_view_info_linked_text->text = LearnMoreString();
    extra_view_info_linked_text->is_learn_more = true;
    delegate.set_extra_view_info(std::move(extra_view_info_linked_text));

    delegate.set_dismiss_button_text(DismissString());
    delegate.set_item_list_text(ItemListString());
    ToolbarActionsBarBubbleMac* bubble = CreateAndShowBubble(&delegate);
    EXPECT_TRUE([bubble actionButton]);
    EXPECT_FALSE([bubble iconView]);
    EXPECT_FALSE([bubble label]);
    EXPECT_TRUE([bubble link]);
    EXPECT_TRUE([bubble dismissButton]);
    EXPECT_TRUE([bubble bodyText]);
    EXPECT_TRUE([bubble itemList]);

    [bubble close];
    chrome::testing::NSRunLoopRunAllPending();
  }
}
