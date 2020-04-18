// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/notifications/desktop_notification_profile_util.h"

#include "base/bind.h"
#include "chrome/test/base/chrome_render_view_host_test_harness.h"
#include "chrome/test/base/testing_profile.h"
#include "components/content_settings/core/common/content_settings.h"
#include "testing/gtest/include/gtest/gtest.h"

class DesktopNotificationServiceTest : public ChromeRenderViewHostTestHarness {
 protected:
  void SetUp() override { ChromeRenderViewHostTestHarness::SetUp(); }
};


TEST_F(DesktopNotificationServiceTest, GetNotificationsSettings) {
  DesktopNotificationProfileUtil::GrantPermission(profile(),
                                                  GURL("http://allowed2.com"));
  DesktopNotificationProfileUtil::GrantPermission(profile(),
                                                  GURL("http://allowed.com"));
  DesktopNotificationProfileUtil::DenyPermission(profile(),
                                                 GURL("http://denied2.com"));
  DesktopNotificationProfileUtil::DenyPermission(profile(),
                                                 GURL("http://denied.com"));

  ContentSettingsForOneType settings;
  DesktopNotificationProfileUtil::GetNotificationsSettings(
      profile(), &settings);
  // |settings| contains the default setting and 4 exceptions.
  ASSERT_EQ(5u, settings.size());

  EXPECT_EQ(ContentSettingsPattern::FromURLNoWildcard(
                GURL("http://allowed.com")),
            settings[0].primary_pattern);
  EXPECT_EQ(CONTENT_SETTING_ALLOW, settings[0].GetContentSetting());
  EXPECT_EQ(ContentSettingsPattern::FromURLNoWildcard(
                GURL("http://allowed2.com")),
            settings[1].primary_pattern);
  EXPECT_EQ(CONTENT_SETTING_ALLOW, settings[1].GetContentSetting());
  EXPECT_EQ(ContentSettingsPattern::FromURLNoWildcard(
                GURL("http://denied.com")),
            settings[2].primary_pattern);
  EXPECT_EQ(CONTENT_SETTING_BLOCK, settings[2].GetContentSetting());
  EXPECT_EQ(ContentSettingsPattern::FromURLNoWildcard(
                GURL("http://denied2.com")),
            settings[3].primary_pattern);
  EXPECT_EQ(CONTENT_SETTING_BLOCK, settings[3].GetContentSetting());
  EXPECT_EQ(ContentSettingsPattern::Wildcard(),
            settings[4].primary_pattern);
  EXPECT_EQ(CONTENT_SETTING_ASK, settings[4].GetContentSetting());
}
