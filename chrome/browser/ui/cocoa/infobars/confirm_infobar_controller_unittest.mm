// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "chrome/browser/ui/cocoa/infobars/confirm_infobar_controller.h"

#include <utility>

#include "base/mac/scoped_nsobject.h"
#include "base/strings/string_util.h"
#include "base/strings/sys_string_conversions.h"
#include "chrome/browser/infobars/infobar_service.h"
#import "chrome/browser/ui/cocoa/infobars/infobar_cocoa.h"
#include "chrome/browser/ui/cocoa/infobars/mock_confirm_infobar_delegate.h"
#include "chrome/browser/ui/cocoa/test/cocoa_profile_test.h"
#include "chrome/browser/ui/cocoa/test/run_loop_testing.h"
#include "chrome/test/base/testing_profile.h"
#include "components/infobars/core/confirm_infobar_delegate.h"
#import "content/public/browser/web_contents.h"
#include "ipc/ipc_message.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "testing/platform_test.h"

using content::WebContents;

@interface InfoBarController (ExposedForTesting)
- (NSString*)labelString;
- (NSRect)labelFrame;
@end

@implementation InfoBarController (ExposedForTesting)
- (NSString*)labelString {
  return [label_.get() string];
}
- (NSRect)labelFrame {
  return [label_.get() frame];
}
@end


@interface TestConfirmInfoBarController : ConfirmInfoBarController
- (void)removeSelf;
@end

@implementation TestConfirmInfoBarController
- (void)removeSelf {
  [self infobarWillClose];
  if ([self infobar])
    [self infobar]->CloseSoon();
}
@end

namespace {

class ConfirmInfoBarControllerTest : public CocoaProfileTest,
                                     public MockConfirmInfoBarDelegate::Owner {
 public:
  void SetUp() override {
    CocoaProfileTest::SetUp();
    web_contents_ = WebContents::Create(WebContents::CreateParams(profile()));
    InfoBarService::CreateForWebContents(web_contents_.get());

    std::unique_ptr<infobars::InfoBarDelegate> delegate(
        new MockConfirmInfoBarDelegate(this));
    infobar_ = new InfoBarCocoa(std::move(delegate));
    infobar_->SetOwner(InfoBarService::FromWebContents(web_contents_.get()));

    controller_.reset([[TestConfirmInfoBarController alloc]
        initWithInfoBar:infobar_]);
    infobar_->set_controller(controller_);

    [[test_window() contentView] addSubview:[controller_ view]];
    closed_delegate_ok_clicked_ = false;
    closed_delegate_cancel_clicked_ = false;
    closed_delegate_link_clicked_ = false;
    delegate_closed_ = false;
  }

  void TearDown() override {
    [controller_ removeSelf];
    CocoaProfileTest::TearDown();
  }

 protected:
  // True if delegate is closed.
  bool delegate_closed() const { return delegate_closed_; }

  MockConfirmInfoBarDelegate* delegate() const {
    return static_cast<MockConfirmInfoBarDelegate*>(infobar_->delegate());
  }

  base::scoped_nsobject<ConfirmInfoBarController> controller_;
  bool closed_delegate_ok_clicked_;
  bool closed_delegate_cancel_clicked_;
  bool closed_delegate_link_clicked_;

 private:
  void OnInfoBarDelegateClosed(MockConfirmInfoBarDelegate* delegate) override {
    closed_delegate_ok_clicked_ = delegate->ok_clicked();
    closed_delegate_cancel_clicked_ = delegate->cancel_clicked();
    closed_delegate_link_clicked_ = delegate->link_clicked();
    delegate_closed_ = true;
    controller_.reset();
  }

  std::unique_ptr<WebContents> web_contents_;
  InfoBarCocoa* infobar_;  // Weak, will delete itself.
  bool delegate_closed_;
};


TEST_VIEW(ConfirmInfoBarControllerTest, [controller_ view]);

TEST_F(ConfirmInfoBarControllerTest, ShowAndDismiss) {
  // Make sure someone looked at the message, link, and icon.
  EXPECT_TRUE(delegate()->message_text_accessed());
  EXPECT_TRUE(delegate()->link_text_accessed());
  EXPECT_TRUE(delegate()->icon_accessed());

  // Check to make sure the infobar message was set properly.
  EXPECT_EQ(MockConfirmInfoBarDelegate::kMessage,
            base::SysNSStringToUTF8([controller_.get() labelString]));

  // Check that dismissing the infobar deletes the delegate.
  [controller_ removeSelf];
  ASSERT_TRUE(delegate_closed());
  EXPECT_FALSE(closed_delegate_ok_clicked_);
  EXPECT_FALSE(closed_delegate_cancel_clicked_);
  EXPECT_FALSE(closed_delegate_link_clicked_);
}

TEST_F(ConfirmInfoBarControllerTest, ShowAndClickOK) {
  // Check that clicking the OK button calls Accept() and then closes
  // the infobar.
  [controller_ ok:nil];
  ASSERT_TRUE(delegate_closed());
  EXPECT_TRUE(closed_delegate_ok_clicked_);
  EXPECT_FALSE(closed_delegate_cancel_clicked_);
  EXPECT_FALSE(closed_delegate_link_clicked_);
}

TEST_F(ConfirmInfoBarControllerTest, ShowAndClickOKWithoutClosing) {
  delegate()->set_dont_close_on_action();

  // Check that clicking the OK button calls Accept() but does not close
  // the infobar.
  [controller_ ok:nil];
  ASSERT_FALSE(delegate_closed());
  EXPECT_TRUE(delegate()->ok_clicked());
  EXPECT_FALSE(delegate()->cancel_clicked());
  EXPECT_FALSE(delegate()->link_clicked());
}

TEST_F(ConfirmInfoBarControllerTest, ShowAndClickCancel) {
  // Check that clicking the cancel button calls Cancel() and closes
  // the infobar.
  [controller_ cancel:nil];
  ASSERT_TRUE(delegate_closed());
  EXPECT_FALSE(closed_delegate_ok_clicked_);
  EXPECT_TRUE(closed_delegate_cancel_clicked_);
  EXPECT_FALSE(closed_delegate_link_clicked_);
}

TEST_F(ConfirmInfoBarControllerTest, ShowAndClickCancelWithoutClosing) {
  delegate()->set_dont_close_on_action();

  // Check that clicking the cancel button calls Cancel() but does not close
  // the infobar.
  [controller_ cancel:nil];
  ASSERT_FALSE(delegate_closed());
  EXPECT_FALSE(delegate()->ok_clicked());
  EXPECT_TRUE(delegate()->cancel_clicked());
  EXPECT_FALSE(delegate()->link_clicked());
}

TEST_F(ConfirmInfoBarControllerTest, ShowAndClickLink) {
  // Check that clicking on the link calls LinkClicked() on the
  // delegate.  It should also close the infobar.
  [controller_ linkClicked];
  ASSERT_TRUE(delegate_closed());
  EXPECT_FALSE(closed_delegate_ok_clicked_);
  EXPECT_FALSE(closed_delegate_cancel_clicked_);
  EXPECT_TRUE(closed_delegate_link_clicked_);
}

TEST_F(ConfirmInfoBarControllerTest, ShowAndClickLinkWithoutClosing) {
  delegate()->set_dont_close_on_action();

  // Check that clicking on the link calls LinkClicked() on the
  // delegate.  It should not close the infobar.
  [controller_ linkClicked];
  ASSERT_FALSE(delegate_closed());
  EXPECT_FALSE(delegate()->ok_clicked());
  EXPECT_FALSE(delegate()->cancel_clicked());
  EXPECT_TRUE(delegate()->link_clicked());
}

TEST_F(ConfirmInfoBarControllerTest, ResizeView) {
  NSRect originalLabelFrame = [controller_ labelFrame];

  // Expand the view by 20 pixels and make sure the label frame changes
  // accordingly.
  const CGFloat width = 20;
  NSRect newViewFrame = [[controller_ view] frame];
  newViewFrame.size.width += width;
  [[controller_ view] setFrame:newViewFrame];

  NSRect newLabelFrame = [controller_ labelFrame];
  EXPECT_EQ(NSWidth(newLabelFrame), NSWidth(originalLabelFrame) + width);
}

}  // namespace
