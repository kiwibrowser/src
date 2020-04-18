// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/views/intent_picker_bubble_view.h"

#include <string>

#include "base/bind.h"
#include "base/bind_helpers.h"
#include "base/callback.h"
#include "base/macros.h"
#include "chrome/browser/chromeos/apps/intent_helper/apps_navigation_types.h"
#include "chrome/browser/chromeos/arc/intent_helper/arc_navigation_throttle.h"
#include "chrome/test/base/browser_with_test_window_test.h"
#include "components/arc/intent_helper/arc_intent_helper_bridge.h"
#include "content/public/browser/web_contents.h"
#include "ui/base/resource/resource_bundle.h"
#include "ui/events/base_event_utils.h"
#include "ui/gfx/image/image.h"
#include "ui/views/controls/button/button.h"
#include "ui/views/controls/scroll_view.h"
#include "ui/views/resources/grit/views_resources.h"
#include "url/gurl.h"

using AppInfo = chromeos::IntentPickerAppInfo;
using content::WebContents;
using content::OpenURLParams;
using content::Referrer;

class IntentPickerBubbleViewTest : public BrowserWithTestWindowTest {
 public:
  IntentPickerBubbleViewTest() = default;

  void TearDown() override {
    // Make sure the bubble is destroyed before the profile to avoid a crash.
    bubble_.reset();

    BrowserWithTestWindowTest::TearDown();
  }

 protected:
  void CreateBubbleView(bool use_icons, bool disable_stay_in_chrome) {
    // Pushing a couple of fake apps just to check they are created on the UI.
    app_info_.emplace_back(chromeos::AppType::ARC, gfx::Image(), "package_1",
                           "dank app 1");
    app_info_.emplace_back(chromeos::AppType::ARC, gfx::Image(), "package_2",
                           "dank_app_2");
    // Also adding the corresponding Chrome's package name on ARC, even if this
    // is given to the picker UI as input it should be ignored.
    app_info_.emplace_back(
        chromeos::AppType::ARC, gfx::Image(),
        arc::ArcIntentHelperBridge::kArcIntentHelperPackageName,
        "legit_chrome");

    if (use_icons)
      FillAppListWithDummyIcons();

    // We create |web_contents| since the Bubble UI has an Observer that
    // depends on this, otherwise it wouldn't work.
    GURL url("http://www.google.com");
    WebContents* web_contents = browser()->OpenURL(
        OpenURLParams(url, Referrer(), WindowOpenDisposition::CURRENT_TAB,
                      ui::PAGE_TRANSITION_TYPED, false));

    std::vector<AppInfo> app_info;

    // AppInfo is move only. Manually create a new app_info array to pass into
    // the bubble constructor.
    for (const auto& app : app_info_) {
      app_info.emplace_back(app.type, app.icon, app.launch_name,
                            app.display_name);
    }

    bubble_ = IntentPickerBubbleView::CreateBubbleView(
        std::move(app_info), disable_stay_in_chrome,
        base::Bind(&IntentPickerBubbleViewTest::OnBubbleClosed,
                   base::Unretained(this)),
        web_contents);
  }

  void FillAppListWithDummyIcons() {
    ui::ResourceBundle& rb = ui::ResourceBundle::GetSharedInstance();
    gfx::Image dummy_icon = rb.GetImageNamed(IDR_CLOSE);
    for (auto& app : app_info_)
      app.icon = dummy_icon;
  }

  // Dummy method to be called upon bubble closing.
  void OnBubbleClosed(const std::string& selected_app_package,
                      chromeos::AppType app_type,
                      chromeos::IntentPickerCloseReason close_reason,
                      bool should_persist) {}

  std::unique_ptr<IntentPickerBubbleView> bubble_;
  std::vector<AppInfo> app_info_;

 private:
  DISALLOW_COPY_AND_ASSIGN(IntentPickerBubbleViewTest);
};

// Verifies that we didn't set up an image for any LabelButton.
TEST_F(IntentPickerBubbleViewTest, NullIcons) {
  CreateBubbleView(false, false);
  size_t size = bubble_->GetScrollViewSize();
  for (size_t i = 0; i < size; ++i) {
    gfx::ImageSkia image = bubble_->GetAppImageForTesting(i);
    EXPECT_TRUE(image.isNull()) << i;
  }
}

// Verifies that all the icons contain a non-null icon.
TEST_F(IntentPickerBubbleViewTest, NonNullIcons) {
  CreateBubbleView(true, false);
  size_t size = bubble_->GetScrollViewSize();
  for (size_t i = 0; i < size; ++i) {
    gfx::ImageSkia image = bubble_->GetAppImageForTesting(i);
    EXPECT_FALSE(image.isNull()) << i;
  }
}

// Verifies that the bubble contains as many rows as |app_info_| with one
// exception, if the Chrome package is present on the input list it won't be
// shown to the user on the picker UI, so there could be a difference
// represented by |chrome_package_repetitions|.
TEST_F(IntentPickerBubbleViewTest, LabelsPtrVectorSize) {
  CreateBubbleView(true, false);
  size_t size = app_info_.size();
  size_t chrome_package_repetitions = 0;
  for (const AppInfo& app_info : app_info_) {
    if (arc::ArcIntentHelperBridge::IsIntentHelperPackage(app_info.launch_name))
      ++chrome_package_repetitions;
  }

  EXPECT_EQ(size, bubble_->GetScrollViewSize() + chrome_package_repetitions);
}

// Verifies the InkDrop state when creating a new bubble.
TEST_F(IntentPickerBubbleViewTest, VerifyStartingInkDrop) {
  CreateBubbleView(true, false);
  size_t size = bubble_->GetScrollViewSize();
  for (size_t i = 0; i < size; ++i) {
    EXPECT_EQ(bubble_->GetInkDropStateForTesting(i),
              views::InkDropState::HIDDEN);
  }
}

// Press each button at a time and make sure it goes to ACTIVATED state,
// followed by HIDDEN state after selecting other button.
TEST_F(IntentPickerBubbleViewTest, InkDropStateTransition) {
  CreateBubbleView(true, false);
  const ui::MouseEvent event(ui::ET_MOUSE_PRESSED, gfx::Point(), gfx::Point(),
                             ui::EventTimeForNow(), 0, 0);
  size_t size = bubble_->GetScrollViewSize();
  for (size_t i = 0; i < size; ++i) {
    bubble_->PressButtonForTesting((i + 1) % size, event);
    EXPECT_EQ(bubble_->GetInkDropStateForTesting(i),
              views::InkDropState::HIDDEN);
    EXPECT_EQ(bubble_->GetInkDropStateForTesting((i + 1) % size),
              views::InkDropState::ACTIVATED);
  }
}

// Arbitrary press the first button twice, check that the InkDropState remains
// the same.
TEST_F(IntentPickerBubbleViewTest, PressButtonTwice) {
  CreateBubbleView(true, false);
  const ui::MouseEvent event(ui::ET_MOUSE_PRESSED, gfx::Point(), gfx::Point(),
                             ui::EventTimeForNow(), 0, 0);
  EXPECT_EQ(bubble_->GetInkDropStateForTesting(0), views::InkDropState::HIDDEN);
  bubble_->PressButtonForTesting(0, event);
  EXPECT_EQ(bubble_->GetInkDropStateForTesting(0),
            views::InkDropState::ACTIVATED);
  bubble_->PressButtonForTesting(0, event);
  EXPECT_EQ(bubble_->GetInkDropStateForTesting(0),
            views::InkDropState::ACTIVATED);
}

// Check that none of the app candidates within the picker corresponds to the
// Chrome browser.
TEST_F(IntentPickerBubbleViewTest, ChromeNotInCandidates) {
  CreateBubbleView(false, false);
  size_t size = bubble_->GetScrollViewSize();
  for (size_t i = 0; i < size; ++i) {
    EXPECT_FALSE(arc::ArcIntentHelperBridge::IsIntentHelperPackage(
        bubble_->GetAppInfoForTesting()[i].launch_name));
  }
}

// Check that 'Stay in Chrome' remains enabled/disabled accordingly. For this
// UI, DIALOG_BUTTON_CANCEL maps to 'Stay in Chrome'.
TEST_F(IntentPickerBubbleViewTest, StayInChromeTest) {
  CreateBubbleView(false, true);
  EXPECT_EQ(bubble_->IsDialogButtonEnabled(ui::DIALOG_BUTTON_CANCEL), false);

  CreateBubbleView(false, false);
  EXPECT_EQ(bubble_->IsDialogButtonEnabled(ui::DIALOG_BUTTON_CANCEL), true);
}

// Check that a non nullptr WebContents() has been created and observed.
TEST_F(IntentPickerBubbleViewTest, WebContentsTiedToBubble) {
  CreateBubbleView(false, true);
  EXPECT_TRUE(bubble_->web_contents());

  CreateBubbleView(false, false);
  EXPECT_TRUE(bubble_->web_contents());
}
