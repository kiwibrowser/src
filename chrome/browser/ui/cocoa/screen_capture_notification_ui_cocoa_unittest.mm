// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "chrome/browser/ui/cocoa/screen_capture_notification_ui_cocoa.h"

#include "base/bind.h"
#include "base/mac/mac_util.h"
#include "base/macros.h"
#include "base/strings/utf_string_conversions.h"
#include "chrome/browser/ui/cocoa/test/cocoa_test_helper.h"

@interface ScreenCaptureNotificationController (ExposedForTesting)
- (NSButton*)stopButton;
- (NSButton*)minimizeButton;
@end

@implementation ScreenCaptureNotificationController (ExposedForTesting)
- (NSButton*)stopButton {
  return stopButton_;
}

- (NSButton*)minimizeButton {
  return minimizeButton_;
}
@end

class ScreenCaptureNotificationUICocoaTest : public CocoaTest {
 public:
  ScreenCaptureNotificationUICocoaTest()
      : callback_called_(0) {
  }

  void TearDown() override {
    callback_called_ = 0;
    target_.reset();
    EXPECT_EQ(0, callback_called_);

    CocoaTest::TearDown();
  }

  void StopCallback() {
    ++callback_called_;
  }

 protected:
  ScreenCaptureNotificationController* controller() {
    return target_->windowController_.get();
  }

  std::unique_ptr<ScreenCaptureNotificationUICocoa> target_;
  int callback_called_;

  DISALLOW_COPY_AND_ASSIGN(ScreenCaptureNotificationUICocoaTest);
};

TEST_F(ScreenCaptureNotificationUICocoaTest, CreateAndDestroy) {
  target_.reset(
      new ScreenCaptureNotificationUICocoa(base::UTF8ToUTF16("Title")));
}

TEST_F(ScreenCaptureNotificationUICocoaTest, CreateAndStart) {
  target_.reset(
      new ScreenCaptureNotificationUICocoa(base::UTF8ToUTF16("Title")));
  target_->OnStarted(
      base::Bind(&ScreenCaptureNotificationUICocoaTest::StopCallback,
                 base::Unretained(this)));
}

TEST_F(ScreenCaptureNotificationUICocoaTest, LongTitle) {
  target_.reset(new ScreenCaptureNotificationUICocoa(base::UTF8ToUTF16(
      "Very long title, with very very very very very very very very "
      "very very very very very very very very very very very very many "
      "words")));
  target_->OnStarted(
      base::Bind(&ScreenCaptureNotificationUICocoaTest::StopCallback,
                 base::Unretained(this)));
  // The elided label sometimes is a few pixels longer than the max width. So
  // allow a 5px off from the 1000px maximium.
  EXPECT_LE(NSWidth([[controller() window] frame]), 1005);
}

TEST_F(ScreenCaptureNotificationUICocoaTest, ShortTitle) {
  target_.reset(
      new ScreenCaptureNotificationUICocoa(base::UTF8ToUTF16("Title")));
  target_->OnStarted(
      base::Bind(&ScreenCaptureNotificationUICocoaTest::StopCallback,
                 base::Unretained(this)));
  // Window size may not match the target value exactly (460), as we don't set
  // it directly.
  EXPECT_NEAR(NSWidth([[controller() window] frame]), 460.0, 5.0);
}

TEST_F(ScreenCaptureNotificationUICocoaTest, ClickStop) {
  target_.reset(
      new ScreenCaptureNotificationUICocoa(base::UTF8ToUTF16("Title")));
  target_->OnStarted(
      base::Bind(&ScreenCaptureNotificationUICocoaTest::StopCallback,
                 base::Unretained(this)));

  [[controller() stopButton] performClick:nil];
  EXPECT_EQ(1, callback_called_);
}

TEST_F(ScreenCaptureNotificationUICocoaTest, CloseWindow) {
  target_.reset(
      new ScreenCaptureNotificationUICocoa(base::UTF8ToUTF16("Title")));
  target_->OnStarted(
      base::Bind(&ScreenCaptureNotificationUICocoaTest::StopCallback,
                 base::Unretained(this)));

  [[controller() window] close];

  EXPECT_EQ(1, callback_called_);
}

TEST_F(ScreenCaptureNotificationUICocoaTest, MinimizeWindow) {
  if (base::mac::IsOS10_10())
    return;  // Fails when swarmed. http://crbug.com/660582
  target_.reset(
      new ScreenCaptureNotificationUICocoa(base::UTF8ToUTF16("Title")));
  target_->OnStarted(
      base::Bind(&ScreenCaptureNotificationUICocoaTest::StopCallback,
                 base::Unretained(this)));

  [[controller() minimizeButton] performClick:nil];

  EXPECT_EQ(0, callback_called_);
  EXPECT_TRUE([[controller() window] isMiniaturized]);
}
