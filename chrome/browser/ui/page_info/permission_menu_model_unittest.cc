// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/page_info/permission_menu_model.h"
#include "base/strings/utf_string_conversions.h"
#include "chrome/test/base/testing_profile.h"
#include "content/public/test/test_browser_thread_bundle.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace {

class TestCallback {
 public:
  TestCallback() : current_(-1) {}

  PermissionMenuModel::ChangeCallback callback() {
    return base::Bind(&TestCallback::PermissionChanged, base::Unretained(this));
  }
  void PermissionChanged(const PageInfoUI::PermissionInfo& permission) {
    current_ = permission.setting;
  }

  int current_;
};

class PermissionMenuModelTest : public testing::Test {
 protected:
  TestingProfile* profile() { return &profile_; }

 private:
  content::TestBrowserThreadBundle thread_bundle_;
  TestingProfile profile_;
};

}  // namespace

TEST_F(PermissionMenuModelTest, TestDefault) {
  TestCallback callback;
  PageInfoUI::PermissionInfo permission;
  permission.type = CONTENT_SETTINGS_TYPE_COOKIES;
  permission.setting = CONTENT_SETTING_ALLOW;
  permission.default_setting = CONTENT_SETTING_ALLOW;
  permission.source = content_settings::SETTING_SOURCE_USER;
  permission.is_incognito = false;

  PermissionMenuModel model(profile(), GURL("http://www.google.com"),
                            permission, callback.callback());
  EXPECT_EQ(3, model.GetItemCount());
}

TEST_F(PermissionMenuModelTest, TestDefaultMediaHttp) {
  for (int i = 0; i < 2; ++i) {
    ContentSettingsType type = i ? CONTENT_SETTINGS_TYPE_MEDIASTREAM_MIC
                                 : CONTENT_SETTINGS_TYPE_MEDIASTREAM_CAMERA;
    TestCallback callback;
    PageInfoUI::PermissionInfo permission;
    permission.type = type;
    permission.setting = CONTENT_SETTING_ALLOW;
    permission.default_setting = CONTENT_SETTING_ALLOW;
    permission.source = content_settings::SETTING_SOURCE_USER;
    permission.is_incognito = false;
    PermissionMenuModel model(profile(), GURL("http://www.google.com"),
                              permission, callback.callback());
    EXPECT_EQ(2, model.GetItemCount());
  }
}

TEST_F(PermissionMenuModelTest, TestIncognitoNotifications) {
  TestCallback callback;
  PageInfoUI::PermissionInfo permission;
  permission.type = CONTENT_SETTINGS_TYPE_NOTIFICATIONS;
  permission.setting = CONTENT_SETTING_ASK;
  permission.default_setting = CONTENT_SETTING_ASK;
  permission.source = content_settings::SETTING_SOURCE_USER;

  permission.is_incognito = false;
  PermissionMenuModel regular_model(profile(), GURL("https://www.google.com"),
                                    permission, callback.callback());
  EXPECT_EQ(3, regular_model.GetItemCount());

  permission.is_incognito = true;
  PermissionMenuModel incognito_model(profile(), GURL("https://www.google.com"),
                                      permission, callback.callback());
  EXPECT_EQ(2, incognito_model.GetItemCount());
}

TEST_F(PermissionMenuModelTest, TestUsbGuard) {
  TestCallback callback;
  PageInfoUI::PermissionInfo permission;
  permission.type = CONTENT_SETTINGS_TYPE_USB_GUARD;
  permission.setting = CONTENT_SETTING_ASK;
  permission.default_setting = CONTENT_SETTING_ASK;
  permission.source = content_settings::SETTING_SOURCE_USER;
  permission.is_incognito = false;

  PermissionMenuModel model(profile(), GURL("http://www.google.com"),
                            permission, callback.callback());
  EXPECT_EQ(3, model.GetItemCount());
}
