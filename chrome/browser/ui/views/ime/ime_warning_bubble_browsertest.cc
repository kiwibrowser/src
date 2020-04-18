// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/extensions/api/input_ime/input_ime_api_nonchromeos.h"
#include "chrome/browser/extensions/extension_browsertest.h"
#include "chrome/browser/ui/browser_window.h"
#include "chrome/browser/ui/toolbar/toolbar_actions_bar.h"
#include "chrome/browser/ui/views/frame/browser_view.h"
#include "chrome/browser/ui/views/ime/ime_warning_bubble_view.h"
#include "ui/views/controls/button/checkbox.h"

class ImeWarningBubbleTest : public extensions::ExtensionBrowserTest {
 public:
  ImeWarningBubbleTest();
  ~ImeWarningBubbleTest() override {}

  void SetUpOnMainThread() override;

 protected:
  void OnPermissionBubbleFinished(ImeWarningBubblePermissionStatus status);

  bool IsVisible();
  void CloseBubble(bool ok, bool checked);

  const extensions::Extension* extension_;
  ImeWarningBubbleView* ime_warning_bubble_;
  base::Callback<void(ImeWarningBubblePermissionStatus status)> callback_;
  bool ok_button_pressed_;
  bool never_show_checked_;

  DISALLOW_COPY_AND_ASSIGN(ImeWarningBubbleTest);
};

ImeWarningBubbleTest::ImeWarningBubbleTest()
    : extension_(nullptr),
      ime_warning_bubble_(nullptr),
      ok_button_pressed_(false),
      never_show_checked_(false) {}

void ImeWarningBubbleTest::SetUpOnMainThread() {
  ToolbarActionsBar::disable_animations_for_testing_ = true;
  extensions::ExtensionBrowserTest::SetUpOnMainThread();
  extension_ = LoadExtension(test_data_dir_.AppendASCII("input_ime"));
  callback_ =
      base::Bind(&ImeWarningBubbleTest::OnPermissionBubbleFinished,
                 base::Unretained(this));
  browser()->window()->ShowImeWarningBubble(extension_, callback_);
  ime_warning_bubble_ = ImeWarningBubbleView::ime_warning_bubble_for_test_;
}

void ImeWarningBubbleTest::OnPermissionBubbleFinished(
    ImeWarningBubblePermissionStatus status) {
  if (status == ImeWarningBubblePermissionStatus::GRANTED ||
      status == ImeWarningBubblePermissionStatus::GRANTED_AND_NEVER_SHOW) {
    ok_button_pressed_ = true;
  } else {
    ok_button_pressed_ = false;
  }
  if (status == ImeWarningBubblePermissionStatus::GRANTED_AND_NEVER_SHOW) {
    never_show_checked_ = true;
  } else {
    never_show_checked_ = false;
  }
}

bool ImeWarningBubbleTest::IsVisible() {
  return ime_warning_bubble_->GetWidget() &&
         ime_warning_bubble_->GetWidget()->IsVisible();
}

void ImeWarningBubbleTest::CloseBubble(bool ok, bool checked) {
  ime_warning_bubble_->never_show_checkbox_->SetChecked(checked);
  if (ok)
    ime_warning_bubble_->Accept();
  else
    ime_warning_bubble_->Cancel();
}

IN_PROC_BROWSER_TEST_F(ImeWarningBubbleTest, PressOKButton) {
  ASSERT_TRUE(!!ime_warning_bubble_);
  EXPECT_TRUE(IsVisible());
  CloseBubble(true, true);
  EXPECT_TRUE(ok_button_pressed_);
  EXPECT_TRUE(never_show_checked_);
}

IN_PROC_BROWSER_TEST_F(ImeWarningBubbleTest, PressCANCELButton) {
  ASSERT_TRUE(!!ime_warning_bubble_);
  EXPECT_TRUE(IsVisible());
  CloseBubble(false, false);
  EXPECT_FALSE(ok_button_pressed_);
  EXPECT_FALSE(never_show_checked_);
}
