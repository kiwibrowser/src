// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/cocoa/translate/translate_bubble_controller.h"

#include "base/test/histogram_tester.h"
#include "base/test/scoped_feature_list.h"
#import "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_window.h"
#import "chrome/browser/ui/cocoa/browser_window_controller.h"
#import "chrome/browser/ui/cocoa/info_bubble_window.h"
#import "chrome/browser/ui/cocoa/test/cocoa_profile_test.h"
#include "chrome/browser/ui/cocoa/test/run_loop_testing.h"
#include "chrome/browser/ui/translate/translate_bubble_model.h"
#include "chrome/common/url_constants.h"
#include "content/public/browser/site_instance.h"
#include "ui/base/ui_base_features.h"

@implementation BrowserWindowController (ForTesting)

- (TranslateBubbleController*)translateBubbleController{
  return translateBubbleController_;
}

@end

@implementation TranslateBubbleController (ForTesting)

- (NSView*)errorView {
  NSNumber* key = @(TranslateBubbleModel::VIEW_STATE_ERROR);
  return [views_ objectForKey:key];
}
- (NSButton*)tryAgainButton {
  return tryAgainButton_;
}

@end

class TranslateBubbleControllerTest : public CocoaProfileTest {
 public:
  TranslateBubbleControllerTest() {}

  // CocoaProfileTest:
  void SetUp() override {
    // This file only tests Cocoa UI and can be deleted when kSecondaryUiMd is
    // default.
    scoped_feature_list_.InitAndDisableFeature(features::kSecondaryUiMd);

    CocoaProfileTest::SetUp();

    site_instance_ = content::SiteInstance::Create(profile());

    NSWindow* nativeWindow = browser()->window()->GetNativeWindow();
    bwc_ =
        [BrowserWindowController browserWindowControllerForWindow:nativeWindow];
    web_contents_ = AppendToTabStrip();
  }

  content::WebContents* AppendToTabStrip() {
    std::unique_ptr<content::WebContents> web_contents =
        content::WebContents::Create(content::WebContents::CreateParams(
            profile(), site_instance_.get()));
    content::WebContents* raw_web_contents = web_contents.get();
    browser()->tab_strip_model()->AppendWebContents(std::move(web_contents),
                                                    /*foreground=*/true);
    return raw_web_contents;
  }

  BrowserWindowController* bwc() { return bwc_; }

  TranslateBubbleController* bubble() {
    return [bwc() translateBubbleController];
  }

  void ShowBubble() {
    ASSERT_FALSE(bubble());
    translate::TranslateStep step = translate::TRANSLATE_STEP_BEFORE_TRANSLATE;

    [bwc_ showTranslateBubbleForWebContents:web_contents_
                                       step:step
                                  errorType:translate::TranslateErrors::NONE];

    // Ensure that there are no closing animations.
    InfoBubbleWindow* window = (InfoBubbleWindow*)[bubble() window];
    [window setAllowedAnimations:info_bubble::kAnimateNone];
  }

  void SwitchToErrorView() {
    translate::TranslateStep step = translate::TRANSLATE_STEP_TRANSLATE_ERROR;
    [bwc_
        showTranslateBubbleForWebContents:web_contents_
                                     step:step
                                errorType:translate::TranslateErrors::NETWORK];
  }

  void CloseBubble() {
    [bubble() close];
    chrome::testing::NSRunLoopRunAllPending();
  }

 private:
  base::test::ScopedFeatureList scoped_feature_list_;
  scoped_refptr<content::SiteInstance> site_instance_;
  BrowserWindowController* bwc_;
  content::WebContents* web_contents_;

  DISALLOW_COPY_AND_ASSIGN(TranslateBubbleControllerTest);
};

TEST_F(TranslateBubbleControllerTest, ShowAndClose) {
  EXPECT_FALSE(bubble());

  ShowBubble();
  EXPECT_TRUE(bubble());

  CloseBubble();
  EXPECT_FALSE(bubble());
}

TEST_F(TranslateBubbleControllerTest, SwitchToErrorView) {
  EXPECT_FALSE(bubble());
  ShowBubble();
  const TranslateBubbleModel* model = [bubble() model];

  EXPECT_TRUE(bubble());
  EXPECT_EQ(TranslateBubbleModel::ViewState::VIEW_STATE_BEFORE_TRANSLATE,
            model->GetViewState());

  SwitchToErrorView();

  NSView* errorView = [bubble() errorView];
  // We should have 4 subview inside the error view:
  // A NSTextField, a NSImageView and two NSButton.
  EXPECT_EQ(4UL, [[errorView subviews] count]);

  // one of the subview should be "Try again" button.
  EXPECT_TRUE([[errorView subviews] containsObject:[bubble() tryAgainButton]]);
  EXPECT_EQ(TranslateBubbleModel::ViewState::VIEW_STATE_ERROR,
            model->GetViewState());
  EXPECT_TRUE(bubble());
  CloseBubble();
}

TEST_F(TranslateBubbleControllerTest, SwitchViews) {
  // A basic test which just switch between views to make sure no crash.
  EXPECT_FALSE(bubble());

  ShowBubble();
  EXPECT_TRUE(bubble());

  // Switch to during translating view.
  [bubble()
      switchView:(TranslateBubbleModel::ViewState::VIEW_STATE_TRANSLATING)];

  EXPECT_TRUE(bubble());

  // Switch to after translating view.
  [bubble()
      switchView:(TranslateBubbleModel::ViewState::VIEW_STATE_AFTER_TRANSLATE)];

  EXPECT_TRUE(bubble());

  // Switch to advanced view.
  [bubble() switchView:(TranslateBubbleModel::ViewState::VIEW_STATE_ADVANCED)];

  EXPECT_TRUE(bubble());

  // Switch to before translating view.
  [bubble() switchView:
                (TranslateBubbleModel::ViewState::VIEW_STATE_BEFORE_TRANSLATE)];

  EXPECT_TRUE(bubble());

  CloseBubble();
  EXPECT_FALSE(bubble());
}

TEST_F(TranslateBubbleControllerTest, CloseRegistersDecline) {
  const char kDeclineTranslateDismissUI[] =
      "Translate.DeclineTranslateDismissUI";
  const char kDeclineTranslate[] = "Translate.DeclineTranslate";

  // A simple close without any interactions registers as a dismissal.
  {
    base::HistogramTester histogram_tester;
    ShowBubble();
    CloseBubble();
    histogram_tester.ExpectTotalCount(kDeclineTranslateDismissUI, 1);
    histogram_tester.ExpectTotalCount(kDeclineTranslate, 0);
  }

  // A close while pressing e.g. 'x', registers as decline.
  {
    base::HistogramTester histogram_tester;
    ShowBubble();
    [bubble() handleCloseButtonPressed:nil];

    CloseBubble();
    histogram_tester.ExpectTotalCount(kDeclineTranslateDismissUI, 0);
    histogram_tester.ExpectTotalCount(kDeclineTranslate, 1);
  }
}
