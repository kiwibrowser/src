// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/android/tab_web_contents_delegate_android.h"

#include "base/android/jni_android.h"
#include "components/previews/core/previews_experiments.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace {

class TestTabWebContentsDelegateAndroid
    : public android::TabWebContentsDelegateAndroid {
 public:
  explicit TestTabWebContentsDelegateAndroid(blink::WebDisplayMode display_mode)
      : TabWebContentsDelegateAndroid(base::android::AttachCurrentThread(),
                                      nullptr) {
    display_mode_ = display_mode;
  }

  blink::WebDisplayMode GetDisplayMode(
      const content::WebContents* web_contents) const override {
    return display_mode_;
  }

 private:
  blink::WebDisplayMode display_mode_ = blink::kWebDisplayModeBrowser;
};

}  // namespace

namespace android {

TEST(TabWebContentsDelegateAndroidTest,
     AdjustPreviewsStateForNavigationAllowsPreviews) {
  TestTabWebContentsDelegateAndroid browser_display_delegate(
      blink::kWebDisplayModeBrowser);
  content::PreviewsState noscript_previews_state = content::NOSCRIPT_ON;
  browser_display_delegate.AdjustPreviewsStateForNavigation(
      nullptr, &noscript_previews_state);
  EXPECT_EQ(content::NOSCRIPT_ON, noscript_previews_state);
  content::PreviewsState lofi_previews_state = content::CLIENT_LOFI_ON;
  browser_display_delegate.AdjustPreviewsStateForNavigation(
      nullptr, &lofi_previews_state);
  EXPECT_EQ(content::CLIENT_LOFI_ON, lofi_previews_state);
}

TEST(TabWebContentsDelegateAndroidTest,
     AdjustPreviewsStateForNavigationBlocksPreviews) {
  TestTabWebContentsDelegateAndroid standalone_display_delegate(
      blink::kWebDisplayModeStandalone);
  content::PreviewsState noscript_previews_state = content::NOSCRIPT_ON;
  standalone_display_delegate.AdjustPreviewsStateForNavigation(
      nullptr, &noscript_previews_state);
  EXPECT_EQ(content::PREVIEWS_OFF, noscript_previews_state);

  TestTabWebContentsDelegateAndroid fullscreen_display_delegate(
      blink::kWebDisplayModeFullscreen);
  content::PreviewsState lofi_previews_state = content::CLIENT_LOFI_ON;
  fullscreen_display_delegate.AdjustPreviewsStateForNavigation(
      nullptr, &lofi_previews_state);
  EXPECT_EQ(content::PREVIEWS_OFF, lofi_previews_state);

  TestTabWebContentsDelegateAndroid minimal_ui_display_delegate(
      blink::kWebDisplayModeMinimalUi);
  content::PreviewsState litepage_previews_state = content::SERVER_LITE_PAGE_ON;
  minimal_ui_display_delegate.AdjustPreviewsStateForNavigation(
      nullptr, &litepage_previews_state);
  EXPECT_EQ(content::PREVIEWS_OFF, litepage_previews_state);

  TestTabWebContentsDelegateAndroid undefined_display_delegate(
      blink::kWebDisplayModeUndefined);
  content::PreviewsState server_lofi_previews_state = content::SERVER_LOFI_ON;
  undefined_display_delegate.AdjustPreviewsStateForNavigation(
      nullptr, &server_lofi_previews_state);
  EXPECT_EQ(content::PREVIEWS_OFF, server_lofi_previews_state);
}

}  // namespace android
