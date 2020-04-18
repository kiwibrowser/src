// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "chrome/browser/ui/cocoa/profiles/avatar_button_controller.h"

#include "base/command_line.h"
#include "base/mac/scoped_nsobject.h"
#include "base/strings/sys_string_conversions.h"
#include "base/strings/utf_string_conversions.h"
#include "chrome/browser/profiles/profile_manager.h"
#include "chrome/browser/profiles/profiles_state.h"
#include "chrome/browser/sync/profile_sync_service_factory.h"
#include "chrome/browser/sync/profile_sync_test_util.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_window.h"
#import "chrome/browser/ui/cocoa/base_bubble_controller.h"
#include "chrome/browser/ui/cocoa/info_bubble_window.h"
#include "chrome/browser/ui/cocoa/test/cocoa_profile_test.h"
#include "chrome/common/chrome_switches.h"
#include "components/signin/core/browser/profile_management_switches.h"
#include "components/sync_preferences/pref_service_syncable.h"
#import "testing/gtest_mac.h"
#include "ui/base/material_design/material_design_controller.h"
#include "ui/base/resource/resource_bundle.h"

using ::testing::Return;

// Defined in the AvatarButtonController implementation.
@interface AvatarButtonController (ExposedForTesting)
- (void)setErrorStatus:(BOOL)hasError;
- (void)updateAvatarButtonAndLayoutParent:(BOOL)layoutParent;
@end

// Mocks the AvatarButtonController class so that we can mock its browser
// window's frame color.
@interface MockAvatarButtonController : AvatarButtonController {
  // True if the frame color is dark.
  BOOL isFrameDark_;
}

- (void)setIsFrameDark:(BOOL)isDark;

@end

@implementation MockAvatarButtonController

- (void)setIsFrameDark:(BOOL)isDark {
  isFrameDark_ = isDark;
}

- (BOOL)isFrameColorDark {
  return isFrameDark_;
}

@end

class AvatarButtonControllerTest : public CocoaProfileTest {
 public:
  AvatarButtonControllerTest() {
    TestingProfile::TestingFactories factories;
    factories.push_back(std::make_pair(ProfileSyncServiceFactory::GetInstance(),
                                       BuildMockProfileSyncService));
    AddTestingFactories(factories);
  }

  void SetUp() override {
    DCHECK(profiles::IsMultipleProfilesEnabled());

    CocoaProfileTest::SetUp();
    ASSERT_TRUE(browser());
    ASSERT_TRUE(browser()->profile());

    mock_sync_service_1_ = static_cast<browser_sync::ProfileSyncServiceMock*>(
        ProfileSyncServiceFactory::GetInstance()->GetForProfile(
            browser()->profile()));
    EXPECT_CALL(*mock_sync_service_1_, IsFirstSetupComplete())
        .WillRepeatedly(Return(false));
    EXPECT_CALL(*mock_sync_service_1_, IsFirstSetupInProgress())
        .WillRepeatedly(Return(true));
    EXPECT_CALL(*mock_sync_service_1_, IsSyncConfirmationNeeded())
        .WillRepeatedly(Return(false));

    controller_.reset([[MockAvatarButtonController alloc]
        initWithBrowser:browser()
                 window:nil]);
  }

  void TearDown() override {
    browser()->window()->Close();
    CocoaProfileTest::TearDown();
  }

  void AddSecondProfile() {
    TestingProfile::TestingFactories factories;
    factories.push_back(std::make_pair(ProfileSyncServiceFactory::GetInstance(),
                                       BuildMockProfileSyncService));

    Profile* new_profile = testing_profile_manager()->CreateTestingProfile(
        "batman", std::unique_ptr<sync_preferences::PrefServiceSyncable>(),
        base::UTF8ToUTF16("Person 1"), 0, std::string(), factories);

    mock_sync_service_2_ = static_cast<browser_sync::ProfileSyncServiceMock*>(
        ProfileSyncServiceFactory::GetInstance()->GetForProfile(new_profile));

    EXPECT_CALL(*mock_sync_service_2_, IsFirstSetupComplete())
        .WillRepeatedly(Return(false));
    EXPECT_CALL(*mock_sync_service_2_, IsFirstSetupInProgress())
        .WillRepeatedly(Return(true));
    EXPECT_CALL(*mock_sync_service_2_, IsSyncConfirmationNeeded())
        .WillRepeatedly(Return(false));
  }

  NSButton* button() { return [controller_ buttonView]; }

  NSView* view() { return [controller_ view]; }

  MockAvatarButtonController* controller() { return controller_.get(); }

 private:
  base::scoped_nsobject<MockAvatarButtonController> controller_;
  browser_sync::ProfileSyncServiceMock* mock_sync_service_1_;
  browser_sync::ProfileSyncServiceMock* mock_sync_service_2_;
};

TEST_F(AvatarButtonControllerTest, GenericButtonShown) {
  ASSERT_FALSE([view() isHidden]);
  // There is only one local profile, which means displaying the generic
  // avatar button.
  EXPECT_NSEQ(@"", [button() title]);
}

TEST_F(AvatarButtonControllerTest, ProfileButtonShown) {
  // Create a second profile, to force the button to display the profile name.
  testing_profile_manager()->CreateTestingProfile("batman");

  ASSERT_FALSE([view() isHidden]);
  EXPECT_NSEQ(@"Person 1", [button() title]);
}

TEST_F(AvatarButtonControllerTest, ProfileButtonWithErrorShown) {
  // Create a second profile, to force the button to display the profile name.
  AddSecondProfile();

  EXPECT_EQ(0, [button() image].size.width);
  [controller() setErrorStatus:true];

  ASSERT_FALSE([view() isHidden]);
  EXPECT_NSEQ(@"Person 1", [button() title]);

  // If the button has an authentication error, it should display an error
  // icon.
  EXPECT_EQ(16, [button() image].size.width);
}

TEST_F(AvatarButtonControllerTest, TitleColor) {
  // Create a second profile, to force the button to display the profile name.
  testing_profile_manager()->CreateTestingProfile("batman");

  // Set the frame color to be not dark. The button's title color should be
  // black.
  [controller() setIsFrameDark:NO];
  [controller() updateAvatarButtonAndLayoutParent:NO];
  NSColor* titleColor =
      [[button() attributedTitle] attribute:NSForegroundColorAttributeName
                                    atIndex:0
                             effectiveRange:nil];
  DCHECK_EQ(titleColor, [NSColor blackColor]);

  // Set the frame color to be dark. The button's title color should be white.
  [controller() setIsFrameDark:YES];
  [controller() updateAvatarButtonAndLayoutParent:NO];
  titleColor =
      [[button() attributedTitle] attribute:NSForegroundColorAttributeName
                                    atIndex:0
                             effectiveRange:nil];
  DCHECK_EQ(titleColor, [NSColor whiteColor]);
}
