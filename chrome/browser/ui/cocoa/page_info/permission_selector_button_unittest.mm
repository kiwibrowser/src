// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "chrome/browser/ui/cocoa/page_info/permission_selector_button.h"

#include "base/mac/scoped_nsobject.h"
#include "base/test/scoped_feature_list.h"
#import "chrome/browser/ui/cocoa/test/cocoa_test_helper.h"
#include "chrome/browser/ui/page_info/page_info_ui.h"
#include "chrome/test/base/testing_profile.h"
#include "content/public/test/test_browser_thread_bundle.h"
#include "ui/base/ui_base_features.h"

@interface PermissionSelectorButton (Testing)
- (NSMenu*)permissionMenu;
@end

namespace {

const ContentSettingsType kTestPermissionType =
    CONTENT_SETTINGS_TYPE_MEDIASTREAM_MIC;

class PermissionSelectorButtonTest : public CocoaTest {
 public:
  PermissionSelectorButtonTest() {
    // This file only tests Cocoa UI and can be deleted when kSecondaryUiMd is
    // default.
    scoped_feature_list_.InitAndDisableFeature(features::kSecondaryUiMd);

    got_callback_ = false;
    PageInfoUI::PermissionInfo test_info;
    test_info.type = kTestPermissionType;
    test_info.setting = CONTENT_SETTING_BLOCK;
    test_info.source = content_settings::SETTING_SOURCE_USER;
    test_info.is_incognito = false;
    GURL test_url("http://www.google.com");
    PermissionMenuModel::ChangeCallback callback = base::Bind(
        &PermissionSelectorButtonTest::Callback, base::Unretained(this));
    view_.reset([[PermissionSelectorButton alloc]
        initWithPermissionInfo:test_info
                        forURL:test_url
                  withCallback:callback
                       profile:&profile_]);
    [[test_window() contentView] addSubview:view_];
  }

  void Callback(const PageInfoUI::PermissionInfo& permission) {
    EXPECT_TRUE(permission.type == kTestPermissionType);
    got_callback_ = true;
  }

  content::TestBrowserThreadBundle thread_bundle_;
  TestingProfile profile_;

  bool got_callback_;
  base::scoped_nsobject<PermissionSelectorButton> view_;

 private:
  base::test::ScopedFeatureList scoped_feature_list_;

  DISALLOW_COPY_AND_ASSIGN(PermissionSelectorButtonTest);
};

TEST_VIEW(PermissionSelectorButtonTest, view_);

TEST_F(PermissionSelectorButtonTest, Callback) {
  NSMenu* menu = [view_ permissionMenu];
  NSMenuItem* item = [menu itemAtIndex:0];
  [[item target] performSelector:[item action] withObject:item];
  EXPECT_TRUE(got_callback_);
}

}  // namespace
