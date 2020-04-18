// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "chrome/browser/ui/cocoa/page_info/page_info_bubble_controller.h"

#include <stddef.h>

#include "base/i18n/rtl.h"
#include "base/macros.h"
#include "base/strings/utf_string_conversions.h"
#include "chrome/browser/ui/browser_window.h"
#include "chrome/browser/ui/cocoa/browser_window_controller.h"
#include "chrome/browser/ui/cocoa/location_bar/location_bar_view_mac.h"
#import "chrome/browser/ui/cocoa/location_bar/page_info_bubble_decoration.h"
#import "chrome/browser/ui/cocoa/test/cocoa_profile_test.h"
#include "content/public/test/test_web_contents_factory.h"
#include "net/test/test_certificate_data.h"
#include "testing/gtest_mac.h"

@interface PageInfoBubbleController (ExposedForTesting)
- (NSView*)permissionsView;
- (NSButton*)resetDecisionsButton;
- (NSButton*)connectionHelpButton;
@end

@implementation PageInfoBubbleController (ExposedForTesting)
- (NSView*)permissionsView {
  return permissionsView_;
}
- (NSButton*)resetDecisionsButton {
  return resetDecisionsButton_;
}
- (NSButton*)connectionHelpButton {
  return connectionHelpButton_;
}
- (NSButton*)changePasswordButton {
  return changePasswordButton_;
}
- (NSButton*)whitelistPasswordReuseButton {
  return whitelistPasswordReuseButton_;
}
@end

@interface PageInfoBubbleControllerForTesting : PageInfoBubbleController {
 @private
  CGFloat defaultWindowWidth_;
}
@end

@implementation PageInfoBubbleControllerForTesting
- (void)setDefaultWindowWidth:(CGFloat)width {
  defaultWindowWidth_ = width;
}
- (CGFloat)defaultWindowWidth {
  // If |defaultWindowWidth_| is 0, use the superclass implementation.
  return defaultWindowWidth_ ? defaultWindowWidth_ : [super defaultWindowWidth];
}
@end

namespace {

// Indices of the menu items in the permission menu.
enum PermissionMenuIndices {
  kMenuIndexContentSettingAllow = 0,
  kMenuIndexContentSettingBlock,
  kMenuIndexContentSettingDefault
};

const ContentSettingsType kTestPermissionTypes[] = {
    CONTENT_SETTINGS_TYPE_IMAGES,
    CONTENT_SETTINGS_TYPE_MEDIASTREAM_CAMERA,
    CONTENT_SETTINGS_TYPE_JAVASCRIPT,
    CONTENT_SETTINGS_TYPE_PLUGINS,
    CONTENT_SETTINGS_TYPE_POPUPS,
    CONTENT_SETTINGS_TYPE_GEOLOCATION,
    CONTENT_SETTINGS_TYPE_NOTIFICATIONS,
    CONTENT_SETTINGS_TYPE_MEDIASTREAM_MIC};

const ContentSetting kTestSettings[] = {
    CONTENT_SETTING_DEFAULT, CONTENT_SETTING_DEFAULT, CONTENT_SETTING_DEFAULT,
    CONTENT_SETTING_ALLOW,   CONTENT_SETTING_BLOCK,   CONTENT_SETTING_ALLOW,
    CONTENT_SETTING_BLOCK,   CONTENT_SETTING_BLOCK};

const ContentSetting kTestDefaultSettings[] = {
    CONTENT_SETTING_ALLOW, CONTENT_SETTING_BLOCK, CONTENT_SETTING_ASK};

const content_settings::SettingSource kTestSettingSources[] = {
    content_settings::SETTING_SOURCE_USER,
    content_settings::SETTING_SOURCE_USER,
    content_settings::SETTING_SOURCE_USER,
    content_settings::SETTING_SOURCE_USER,
    content_settings::SETTING_SOURCE_USER,
    content_settings::SETTING_SOURCE_POLICY,
    content_settings::SETTING_SOURCE_POLICY,
    content_settings::SETTING_SOURCE_EXTENSION};

class PageInfoBubbleControllerTest : public CocoaProfileTest {
 public:
  PageInfoBubbleControllerTest() { controller_ = nil; }

  void TearDown() override {
    [controller_ close];
    CocoaProfileTest::TearDown();
  }

 protected:
  PageInfoUIBridge* bridge_;  // Weak, owned by controller.

  enum MatchType { TEXT_EQUAL = 0, TEXT_NOT_EQUAL };

  // Creates a new page info bubble, with the given default width.
  // If |default_width| is 0, the *default* default width will be used.
  void CreateBubbleWithWidth(CGFloat default_width) {
    bridge_ = new PageInfoUIBridge(nullptr);

    // The controller cleans up after itself when the window closes.
    controller_ = [PageInfoBubbleControllerForTesting alloc];
    [controller_ setDefaultWindowWidth:default_width];
    [controller_ initWithParentWindow:browser()->window()->GetNativeWindow()
                     pageInfoUIBridge:bridge_
                          webContents:web_contents_factory_.CreateWebContents(
                                          browser()->profile())
                                  url:GURL("https://www.google.com")];
    window_ = [controller_ window];
    [controller_ showWindow:nil];
  }

  void CreateBubble() { CreateBubbleWithWidth(0.0); }

  // Return a pointer to the first NSTextField found that either matches, or
  // doesn't match, the given text.
  NSTextField* FindTextField(MatchType match_type, NSString* text) {
    // The window's only immediate child is an invisible view that has a flipped
    // coordinate origin. It is into this that all views get placed.
    NSArray* window_subviews = [[window_ contentView] subviews];
    EXPECT_EQ(1U, [window_subviews count]);
    NSArray* bubble_subviews = [[window_subviews lastObject] subviews];
    NSArray* security_section_subviews =
        [[bubble_subviews firstObject] subviews];

    /**
     *Expect 3 views:
     * - the identity
     * - identity status
     * - security details link
     */
    EXPECT_EQ(3U, [security_section_subviews count]);

    bool desired_result = match_type == TEXT_EQUAL;
    for (NSView* view in security_section_subviews) {
      if ([view isKindOfClass:[NSTextField class]]) {
        NSTextField* text_field = static_cast<NSTextField*>(view);
        if ([[text_field stringValue] isEqual:text] == desired_result)
          return text_field;
      }
    }
    return nil;
  }

  NSMutableArray* FindAllSubviewsOfClass(NSView* parent_view, Class a_class) {
    NSMutableArray* views = [NSMutableArray array];
    for (NSView* view in [parent_view subviews]) {
      if ([view isKindOfClass:a_class])
        [views addObject:view];
    }
    return views;
  }

  // Sets up the dialog with some test permission settings.
  void SetTestPermissions() {
    // Create a list of 5 different permissions, corresponding to all the
    // possible settings:
    // - [allow, block, ask] by default
    // - [block, allow] * by user
    PermissionInfoList permission_info_list;
    PageInfoUI::PermissionInfo info;
    for (size_t i = 0; i < arraysize(kTestPermissionTypes); ++i) {
      info.type = kTestPermissionTypes[i];
      info.setting = kTestSettings[i];
      if (info.setting == CONTENT_SETTING_DEFAULT)
        info.default_setting = kTestDefaultSettings[i];
      info.source = kTestSettingSources[i];
      info.is_incognito = false;
      permission_info_list.push_back(info);
    }
    ChosenObjectInfoList chosen_object_info_list;
    bridge_->SetPermissionInfo(permission_info_list,
                               std::move(chosen_object_info_list));
  }

  int NumSettingsNotSetByUser() const {
    int num_non_user_settings = 0;
    for (size_t i = 0; i < arraysize(kTestSettingSources); ++i) {
      num_non_user_settings +=
          (kTestSettingSources[i] != content_settings::SETTING_SOURCE_USER) ? 1
                                                                            : 0;
    }
    return num_non_user_settings;
  }

  content::TestWebContentsFactory web_contents_factory_;

  PageInfoBubbleControllerForTesting* controller_;  // Weak, owns self.
  NSWindow* window_;  // Weak, owned by controller.
};

TEST_F(PageInfoBubbleControllerTest, ConnectionHelpButton) {
  PageInfoUI::IdentityInfo info;
  info.site_identity = std::string("example.com");
  info.identity_status = PageInfo::SITE_IDENTITY_STATUS_UNKNOWN;

  CreateBubble();

  bridge_->SetIdentityInfo(const_cast<PageInfoUI::IdentityInfo&>(info));

  EXPECT_EQ([[controller_ connectionHelpButton] action],
            @selector(openConnectionHelp:));
}

TEST_F(PageInfoBubbleControllerTest, ResetDecisionsButton) {
  PageInfoUI::IdentityInfo info;
  info.site_identity = std::string("example.com");
  info.identity_status = PageInfo::SITE_IDENTITY_STATUS_UNKNOWN;

  CreateBubble();

  // Set identity info, specifying that the button should not be shown.
  info.show_ssl_decision_revoke_button = false;
  bridge_->SetIdentityInfo(const_cast<PageInfoUI::IdentityInfo&>(info));
  EXPECT_EQ([controller_ resetDecisionsButton], nil);

  // Set identity info, specifying that the button should be shown.
  info.certificate = net::X509Certificate::CreateFromBytes(
      reinterpret_cast<const char*>(google_der), sizeof(google_der));
  ASSERT_TRUE(info.certificate);
  info.show_ssl_decision_revoke_button = true;
  bridge_->SetIdentityInfo(const_cast<PageInfoUI::IdentityInfo&>(info));
  EXPECT_NE([controller_ resetDecisionsButton], nil);

  // Check that clicking the button calls the right selector.
  EXPECT_EQ([[controller_ resetDecisionsButton] action],
            @selector(resetCertificateDecisions:));

  // Since the bubble is only created once per identity, we only need to check
  // the button is *added* when needed. So we don't check that it's removed
  // when we set an identity with `show_ssl_decision_revoke_button == false`
  // again.
}

TEST_F(PageInfoBubbleControllerTest, SetPermissionInfo) {
  CreateBubble();
  SetTestPermissions();

  // There should be three subviews per permission.
  NSArray* subviews = [[controller_ permissionsView] subviews];
  EXPECT_EQ(arraysize(kTestPermissionTypes) * 3,
            [subviews count] - NumSettingsNotSetByUser());

  // Ensure that there is a label for each permission.
  NSMutableArray* permission_labels = [NSMutableArray array];
  for (NSView* view in subviews) {
    if ([view isKindOfClass:[NSTextField class]])
      [permission_labels
          addObject:[static_cast<NSTextField*>(view) stringValue]];
  }
  EXPECT_EQ(arraysize(kTestPermissionTypes),
            [permission_labels count] - NumSettingsNotSetByUser());

  // Ensure that the button labels are distinct, and look for the correct
  // number of disabled buttons.
  int disabled_count = 0;
  NSMutableSet* button_labels = [NSMutableSet set];
  for (NSView* view in subviews) {
    if ([view isKindOfClass:[NSPopUpButton class]]) {
      NSPopUpButton* button = static_cast<NSPopUpButton*>(view);
      [button_labels addObject:[[button selectedCell] title]];

      if (![button isEnabled])
        ++disabled_count;
    }
  }
  EXPECT_EQ(5UL, [button_labels count]);

  // Permissions with a setting source of SETTING_SOURCE_POLICY or
  // SETTING_SOURCE_EXTENSION should have their buttons disabled.
  EXPECT_EQ(NumSettingsNotSetByUser(), disabled_count);
}

TEST_F(PageInfoBubbleControllerTest, WindowWidth) {
  const CGFloat kBigEnoughBubbleWidth = 310;
  // Creating a window that should fit everything.
  CreateBubbleWithWidth(kBigEnoughBubbleWidth);
  SetTestPermissions();

  CGFloat window_width = NSWidth([[controller_ window] frame]);

  // Check the window was made bigger to fit the content.
  EXPECT_EQ(kBigEnoughBubbleWidth, window_width);

  // Check that the window is wider than the right edge of all the permission
  // popup buttons (LTR locales) or wider than the left edge (RTL locales).
  bool is_rtl = base::i18n::IsRTL();
  for (NSView* view in [[controller_ permissionsView] subviews]) {
    if (is_rtl) {
      if ([view isKindOfClass:[NSPopUpButton class]]) {
        NSPopUpButton* button = static_cast<NSPopUpButton*>(view);
        EXPECT_GT(NSMinX([button frame]), 0);
      }
      if ([view isKindOfClass:[NSImageView class]]) {
        NSImageView* icon = static_cast<NSImageView*>(view);
        EXPECT_LT(NSMaxX([icon frame]), window_width);
      }
    } else {
      if ([view isKindOfClass:[NSImageView class]]) {
        NSImageView* icon = static_cast<NSImageView*>(view);
        EXPECT_GT(NSMinX([icon frame]), 0);
      }
      if ([view isKindOfClass:[NSPopUpButton class]]) {
        NSPopUpButton* button = static_cast<NSPopUpButton*>(view);
        EXPECT_LT(NSMaxX([button frame]), window_width);
      }
    }
  }
}

// Tests the page icon decoration's active state.
TEST_F(PageInfoBubbleControllerTest, PageIconDecorationActiveState) {
  NSWindow* window = browser()->window()->GetNativeWindow();
  BrowserWindowController* controller =
      [BrowserWindowController browserWindowControllerForWindow:window];
  LocationBarDecoration* decoration =
      [controller locationBarBridge]->page_info_decoration();

  CreateBubble();
  EXPECT_TRUE([[controller_ window] isVisible]);
  EXPECT_TRUE(decoration->active());

  [controller_ close];
  EXPECT_FALSE(decoration->active());
}

TEST_F(PageInfoBubbleControllerTest, PasswordReuseButtons) {
  PageInfoUI::IdentityInfo info;
  info.site_identity = std::string("example.com");
  info.identity_status = PageInfo::SITE_IDENTITY_STATUS_UNKNOWN;

  CreateBubble();

  // Set identity info, specifying that buttons should not be shown.
  info.show_change_password_buttons = false;
  bridge_->SetIdentityInfo(const_cast<PageInfoUI::IdentityInfo&>(info));
  EXPECT_EQ([controller_ changePasswordButton], nil);
  EXPECT_EQ([controller_ whitelistPasswordReuseButton], nil);

  // Set identity info, specifying that buttons should be shown.
  info.show_change_password_buttons = true;
  bridge_->SetIdentityInfo(const_cast<PageInfoUI::IdentityInfo&>(info));
  EXPECT_NE([controller_ changePasswordButton], nil);
  EXPECT_NE([controller_ whitelistPasswordReuseButton], nil);

  // Check that clicking the button calls the right selector.
  EXPECT_EQ([[controller_ changePasswordButton] action],
            @selector(changePasswordDecisions:));
  EXPECT_EQ([[controller_ whitelistPasswordReuseButton] action],
            @selector(whitelistPasswordReuseDecisions:));
}

}  // namespace
