// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "chrome/browser/ui/cocoa/profiles/avatar_icon_controller.h"

#include "base/command_line.h"
#include "base/mac/scoped_nsobject.h"
#include "base/strings/utf_string_conversions.h"
#include "chrome/browser/autocomplete/autocomplete_classifier_factory.h"
#include "chrome/browser/bookmarks/bookmark_model_factory.h"
#include "chrome/browser/search_engines/template_url_service_factory.h"
#include "chrome/browser/supervised_user/supervised_user_service.h"
#include "chrome/browser/supervised_user/supervised_user_service_factory.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_commands.h"
#include "chrome/browser/ui/browser_window.h"
#import "chrome/browser/ui/cocoa/base_bubble_controller.h"
#import "chrome/browser/ui/cocoa/browser_window_cocoa.h"
#import "chrome/browser/ui/cocoa/browser_window_controller.h"
#include "chrome/browser/ui/cocoa/info_bubble_window.h"
#import "chrome/browser/ui/cocoa/profiles/avatar_button_controller.h"
#include "chrome/browser/ui/cocoa/test/cocoa_profile_test.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/test/base/testing_profile.h"
#include "components/bookmarks/test/bookmark_test_helpers.h"
#include "components/signin/core/browser/profile_management_switches.h"
#include "components/sync_preferences/pref_service_syncable.h"

class AvatarIconControllerTest : public CocoaProfileTest {
 public:
  AvatarIconControllerTest() {}

  void SetUp() override {
    CocoaProfileTest::SetUp();
    ASSERT_TRUE(browser());
  }

  void TearDown() override {
    browser()->window()->Close();
    CocoaProfileTest::TearDown();
  }

  AvatarBaseController* icon_controller() {
    BrowserWindowCocoa* window =
        static_cast<BrowserWindowCocoa*>(browser()->window());
    return [window->cocoa_controller() avatarButtonController];
  }
};

TEST_F(AvatarIconControllerTest, ShowingAvatarIconInIncognito) {
  Browser* browser = new Browser(
      Browser::CreateParams(profile()->GetOffTheRecordProfile(), true));
  BrowserWindowCocoa* window =
      static_cast<BrowserWindowCocoa*>(browser->window());
  AvatarBaseController* icon_controller =
      [window->cocoa_controller() avatarButtonController];
  // In incognito, we should be using the AvatarIconController to show the
  // incognito icon.
  EXPECT_TRUE([icon_controller isKindOfClass:[AvatarIconController class]]);

  browser->window()->Close();
}

TEST_F(AvatarIconControllerTest, ShowingAvatarButtonInRegularSession) {
  // In a regular session, we should be using the AvatarButtonController to show
  // the profile name.
  EXPECT_TRUE([icon_controller() isKindOfClass:[AvatarButtonController class]]);
}
