// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "chrome/browser/ui/cocoa/location_bar/content_setting_decoration.h"

#include "chrome/browser/ui/browser_window.h"
#import "chrome/browser/ui/cocoa/browser_window_controller.h"
#import "chrome/browser/ui/cocoa/location_bar/location_bar_view_mac.h"
#include "chrome/browser/ui/content_settings/content_setting_image_model.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/views/scoped_macviews_browser_mode.h"
#include "content/public/test/test_utils.h"

class ContentSettingDecorationTest : public InProcessBrowserTest {
 protected:
  ContentSettingDecorationTest() : InProcessBrowserTest() {}

  void SetUpOnMainThread() override {
    std::unique_ptr<ContentSettingImageModel> content_setting_image_model(
        ContentSettingImageModel::CreateForContentType(
            ContentSettingImageModel::ImageType::GEOLOCATION));
    BrowserWindowController* controller = [BrowserWindowController
        browserWindowControllerForWindow:browser()
                                             ->window()
                                             ->GetNativeWindow()];

    content_setting_decoration_.reset(new ContentSettingDecoration(
        std::move(content_setting_image_model), [controller locationBarBridge],
        browser()->profile()));
  }

  // Returns the content settings bubble window anchored to
  // |content_setting_decoration_|.
  NSWindow* GetBubbleWindow() const {
    return content_setting_decoration_->bubbleWindow_.get();
  }

  // Simulates a mouse press on the decoration.
  void PressDecoration() {
    content_setting_decoration_->OnMousePressed(NSZeroRect, NSZeroPoint);
  }

 private:
  std::unique_ptr<ContentSettingDecoration> content_setting_decoration_;

  test::ScopedMacViewsBrowserMode cocoa_browser_mode_{false};

  DISALLOW_COPY_AND_ASSIGN(ContentSettingDecorationTest);
};

// Tests to check if pressing the decoration will open/close the content
// settings bubble.
IN_PROC_BROWSER_TEST_F(ContentSettingDecorationTest, DecorationPress) {
  // Bubble should appear.
  PressDecoration();
  EXPECT_TRUE(GetBubbleWindow());

  // Bubble should be closed.
  PressDecoration();
  EXPECT_FALSE(GetBubbleWindow());

  // Bubble should reappear.
  PressDecoration();
  EXPECT_TRUE(GetBubbleWindow());
}
