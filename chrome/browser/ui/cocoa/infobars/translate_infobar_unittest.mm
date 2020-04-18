// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import <Cocoa/Cocoa.h>
#include <stddef.h>

#include <utility>

#import "base/mac/scoped_nsobject.h"
#include "base/macros.h"
#import "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"
#include "chrome/browser/infobars/infobar_service.h"
#import "chrome/browser/translate/chrome_translate_client.h"
#import "chrome/browser/ui/cocoa/infobars/before_translate_infobar_controller.h"
#import "chrome/browser/ui/cocoa/infobars/infobar_cocoa.h"
#import "chrome/browser/ui/cocoa/infobars/translate_infobar_base.h"
#include "chrome/browser/ui/cocoa/test/cocoa_profile_test.h"
#include "chrome/test/base/testing_profile.h"
#import "components/translate/core/browser/options_menu_model.h"
#import "components/translate/core/browser/translate_infobar_delegate.h"
#include "components/translate/core/browser/translate_language_list.h"
#include "components/translate/core/browser/translate_manager.h"
#import "content/public/browser/web_contents.h"
#include "ipc/ipc_message.h"
#import "testing/gmock/include/gmock/gmock.h"
#import "testing/gtest/include/gtest/gtest.h"
#import "testing/platform_test.h"

using content::WebContents;

namespace {

// All states the translate toolbar can assume.
translate::TranslateStep kTranslateToolbarStates[] = {
    translate::TRANSLATE_STEP_BEFORE_TRANSLATE,
    translate::TRANSLATE_STEP_AFTER_TRANSLATE,
    translate::TRANSLATE_STEP_TRANSLATING,
    translate::TRANSLATE_STEP_TRANSLATE_ERROR};

class MockTranslateInfoBarDelegate
    : public translate::TranslateInfoBarDelegate {
 public:
  MockTranslateInfoBarDelegate(content::WebContents* web_contents,
                               translate::TranslateStep step,
                               translate::TranslateErrors::Type error)
      : translate::TranslateInfoBarDelegate(
            ChromeTranslateClient::GetManagerFromWebContents(web_contents)
                ->GetWeakPtr(),
            false,
            step,
            "en",
            "es",
            error,
            false) {}

  MOCK_METHOD0(Translate, void());
  MOCK_METHOD0(RevertTranslation, void());

  MOCK_METHOD0(TranslationDeclined, void());

  bool IsTranslatableLanguageByPrefs() override { return true; }
  MOCK_METHOD0(ToggleTranslatableLanguageByPrefs, void());
  bool IsSiteBlacklisted() override { return false; }
  MOCK_METHOD0(ToggleSiteBlacklist, void());
  bool ShouldAlwaysTranslate() override { return false; }
  MOCK_METHOD0(ToggleAlwaysTranslate, void());
};

}  // namespace

class TranslationInfoBarTest : public CocoaProfileTest {
 public:
  TranslationInfoBarTest() : CocoaProfileTest(), infobar_(NULL) {
  }

  // Each test gets a single Mock translate delegate for the lifetime of
  // the test.
  void SetUp() override {
    translate::TranslateLanguageList::DisableUpdate();
    CocoaProfileTest::SetUp();
    web_contents_ = WebContents::Create(WebContents::CreateParams(profile()));
    InfoBarService::CreateForWebContents(web_contents_.get());
    ChromeTranslateClient::CreateForWebContents(web_contents_.get());
  }

  void TearDown() override {
    if (infobar_) {
      infobar_->CloseSoon();
      infobar_ = NULL;
    }
    CocoaProfileTest::TearDown();
  }

  void CreateInfoBar(translate::TranslateStep type) {
    translate::TranslateErrors::Type error = translate::TranslateErrors::NONE;
    if (type == translate::TRANSLATE_STEP_TRANSLATE_ERROR)
      error = translate::TranslateErrors::NETWORK;
    [[infobar_controller_ view] removeFromSuperview];

    ChromeTranslateClient* chrome_translate_client =
        ChromeTranslateClient::FromWebContents(web_contents_.get());
    std::unique_ptr<translate::TranslateInfoBarDelegate> delegate(
        new MockTranslateInfoBarDelegate(web_contents_.get(), type, error));
    std::unique_ptr<infobars::InfoBar> infobar(
        chrome_translate_client->CreateInfoBar(std::move(delegate)));
    if (infobar_)
      infobar_->CloseSoon();
    infobar_ = static_cast<InfoBarCocoa*>(infobar.release());
    infobar_->SetOwner(InfoBarService::FromWebContents(web_contents_.get()));

    infobar_controller_.reset([static_cast<TranslateInfoBarControllerBase*>(
        infobar_->controller()) retain]);

    // We need to set the window to be wide so that the options button
    // doesn't overlap the other buttons.
    [test_window() setContentSize:NSMakeSize(2000, 500)];
    [[infobar_controller_ view] setFrame:NSMakeRect(0, 0, 2000, 500)];
    [[test_window() contentView] addSubview:[infobar_controller_ view]];
  }

  MockTranslateInfoBarDelegate* infobar_delegate() const {
    return static_cast<MockTranslateInfoBarDelegate*>(infobar_->delegate());
  }

  std::unique_ptr<WebContents> web_contents_;
  InfoBarCocoa* infobar_;  // weak, deletes itself
  base::scoped_nsobject<TranslateInfoBarControllerBase> infobar_controller_;
};

// Check that we can instantiate a Translate Infobar correctly.
TEST_F(TranslationInfoBarTest, Instantiate) {
  CreateInfoBar(translate::TRANSLATE_STEP_BEFORE_TRANSLATE);
  ASSERT_TRUE(infobar_controller_.get());
}

// Check that clicking the Translate button calls Translate().
TEST_F(TranslationInfoBarTest, TranslateCalledOnButtonPress) {
  CreateInfoBar(translate::TRANSLATE_STEP_BEFORE_TRANSLATE);

  EXPECT_CALL(*infobar_delegate(), Translate()).Times(1);
  [infobar_controller_ ok:nil];
}

// Check that clicking the "Retry" button calls Translate() when we're
// in the error mode - http://crbug.com/41315 .
TEST_F(TranslationInfoBarTest, TranslateCalledInErrorMode) {
  CreateInfoBar(translate::TRANSLATE_STEP_TRANSLATE_ERROR);

  EXPECT_CALL(*infobar_delegate(), Translate()).Times(1);

  [infobar_controller_ ok:nil];
}

// Check that clicking the "Show Original button calls RevertTranslation().
TEST_F(TranslationInfoBarTest, RevertCalledOnButtonPress) {
  CreateInfoBar(translate::TRANSLATE_STEP_BEFORE_TRANSLATE);

  EXPECT_CALL(*infobar_delegate(), RevertTranslation()).Times(1);
  [infobar_controller_ showOriginal:nil];
}

// Check that items in the options menu are hooked up correctly.
TEST_F(TranslationInfoBarTest, OptionsMenuItemsHookedUp) {
  CreateInfoBar(translate::TRANSLATE_STEP_BEFORE_TRANSLATE);
  EXPECT_CALL(*infobar_delegate(), Translate())
    .Times(0);

  [infobar_controller_ rebuildOptionsMenu:NO];
  NSMenu* optionsMenu = [infobar_controller_ optionsMenu];
  NSArray* optionsMenuItems = [optionsMenu itemArray];

  EXPECT_EQ(7U, [optionsMenuItems count]);

  // First item is the options menu button's title, so there's no need to test
  // that the target on that is setup correctly.
  for (NSUInteger i = 1; i < [optionsMenuItems count]; ++i) {
    NSMenuItem* item = [optionsMenuItems objectAtIndex:i];
    if (![item isSeparatorItem])
      EXPECT_EQ([item target], infobar_controller_.get());
  }
  NSMenuItem* alwaysTranslateLanguateItem = [optionsMenuItems objectAtIndex:1];
  NSMenuItem* neverTranslateLanguateItem = [optionsMenuItems objectAtIndex:2];
  NSMenuItem* neverTranslateSiteItem = [optionsMenuItems objectAtIndex:3];
  // Separator at 4.
  NSMenuItem* reportBadLanguageItem = [optionsMenuItems objectAtIndex:5];
  NSMenuItem* aboutTranslateItem = [optionsMenuItems objectAtIndex:6];

  {
    EXPECT_CALL(*infobar_delegate(), ToggleAlwaysTranslate())
    .Times(1);
    [infobar_controller_ optionsMenuChanged:alwaysTranslateLanguateItem];
  }

  {
    EXPECT_CALL(*infobar_delegate(), ToggleTranslatableLanguageByPrefs())
    .Times(1);
    [infobar_controller_ optionsMenuChanged:neverTranslateLanguateItem];
  }

  {
    EXPECT_CALL(*infobar_delegate(), ToggleSiteBlacklist())
    .Times(1);
    [infobar_controller_ optionsMenuChanged:neverTranslateSiteItem];
  }

  {
    // Can't mock these effectively, so just check that the tag is set
    // correctly.
    EXPECT_EQ(translate::OptionsMenuModel::REPORT_BAD_DETECTION,
              [reportBadLanguageItem tag]);
    EXPECT_EQ(translate::OptionsMenuModel::ABOUT_TRANSLATE,
              [aboutTranslateItem tag]);
  }
}

// Check that selecting a new item from the "Source Language" popup in "before
// translate" mode doesn't trigger a translation or change state.
// http://crbug.com/36666
TEST_F(TranslationInfoBarTest, Bug36666) {
  CreateInfoBar(translate::TRANSLATE_STEP_BEFORE_TRANSLATE);
  EXPECT_CALL(*infobar_delegate(), Translate())
    .Times(0);

  int arbitrary_index = 2;
  NSString* arbitrary_language = @"es";
  [infobar_controller_ sourceLanguageModified:arbitrary_language
                            withLanguageIndex:arbitrary_index];
  EXPECT_CALL(*infobar_delegate(), Translate())
    .Times(0);
}

// Check that the infobar lays itself out correctly when instantiated in
// each of the states.
// http://crbug.com/36895
TEST_F(TranslationInfoBarTest, Bug36895) {
  for (size_t i = 0; i < arraysize(kTranslateToolbarStates); ++i) {
    CreateInfoBar(kTranslateToolbarStates[i]);
    EXPECT_CALL(*infobar_delegate(), Translate())
      .Times(0);
    EXPECT_TRUE(
        [infobar_controller_ verifyLayout]) << "Layout wrong, for state #" << i;
  }
}

// Verify that the infobar shows the "Always translate this language" button
// after doing 3 translations.
TEST_F(TranslationInfoBarTest, TriggerShowAlwaysTranslateButton) {
  std::unique_ptr<translate::TranslatePrefs> translate_prefs(
      ChromeTranslateClient::CreateTranslatePrefs(profile()->GetPrefs()));
  translate_prefs->ResetTranslationAcceptedCount("en");
  for (int i = 0; i < 4; ++i) {
    translate_prefs->IncrementTranslationAcceptedCount("en");
  }
  CreateInfoBar(translate::TRANSLATE_STEP_BEFORE_TRANSLATE);
  BeforeTranslateInfobarController* controller =
      (BeforeTranslateInfobarController*)infobar_controller_.get();
  EXPECT_TRUE([[controller alwaysTranslateButton] superview] !=  nil);
  EXPECT_TRUE([[controller neverTranslateButton] superview] == nil);
}

// Verify that the infobar shows the "Never translate this language" button
// after denying 3 translations.
TEST_F(TranslationInfoBarTest, TriggerShowNeverTranslateButton) {
  std::unique_ptr<translate::TranslatePrefs> translate_prefs(
      ChromeTranslateClient::CreateTranslatePrefs(profile()->GetPrefs()));
  translate_prefs->ResetTranslationDeniedCount("en");
  for (int i = 0; i < 4; ++i) {
    translate_prefs->IncrementTranslationDeniedCount("en");
  }
  CreateInfoBar(translate::TRANSLATE_STEP_BEFORE_TRANSLATE);
  BeforeTranslateInfobarController* controller =
      (BeforeTranslateInfobarController*)infobar_controller_.get();
  EXPECT_TRUE([[controller alwaysTranslateButton] superview] == nil);
  EXPECT_TRUE([[controller neverTranslateButton] superview] != nil);
}
