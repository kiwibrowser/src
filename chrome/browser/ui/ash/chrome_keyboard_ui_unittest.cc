// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/ash/chrome_keyboard_ui.h"

#include "base/macros.h"
#include "chrome/test/base/chrome_render_view_host_test_harness.h"
#include "content/public/browser/web_contents.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/aura/window.h"
#include "ui/gfx/geometry/rect.h"

namespace {

class TestChromeKeyboardUI : public ChromeKeyboardUI {
 public:
  explicit TestChromeKeyboardUI(std::unique_ptr<content::WebContents> contents)
      : ChromeKeyboardUI(contents->GetBrowserContext()),
        contents_(std::move(contents)) {}
  ~TestChromeKeyboardUI() override {}

  ui::InputMethod* GetInputMethod() override { return nullptr; }
  void RequestAudioInput(content::WebContents* web_contents,
                         const content::MediaStreamRequest& request,
                         const content::MediaResponseCallback& callback) {}

  std::unique_ptr<content::WebContents> CreateWebContents() override {
    return std::move(contents_);
  }

 private:
  std::unique_ptr<content::WebContents> contents_;

  DISALLOW_COPY_AND_ASSIGN(TestChromeKeyboardUI);
};

}  // namespace

using ChromeKeyboardUITest = ChromeRenderViewHostTestHarness;

// A test for crbug.com/734534
TEST_F(ChromeKeyboardUITest, DoesNotCrashWhenParentDoesNotExist) {
  std::unique_ptr<content::WebContents> contents = CreateTestWebContents();
  TestChromeKeyboardUI keyboard_ui(std::move(contents));

  EXPECT_FALSE(keyboard_ui.HasContentsWindow());
  aura::Window* view = keyboard_ui.GetContentsWindow();
  EXPECT_TRUE(keyboard_ui.HasContentsWindow());

  EXPECT_FALSE(view->parent());

  // Change window size to trigger OnWindowBoundsChanged.
  view->SetBounds(gfx::Rect(0, 0, 1200, 800));
}
