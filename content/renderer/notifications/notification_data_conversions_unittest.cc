// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/renderer/notifications/notification_data_conversions.h"

#include <stddef.h>
#include <stdint.h>

#include "base/macros.h"
#include "base/strings/utf_string_conversions.h"
#include "content/public/common/platform_notification_data.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/blink/public/platform/modules/notifications/web_notification_data.h"
#include "third_party/blink/public/platform/web_string.h"
#include "third_party/blink/public/platform/web_url.h"

namespace content {

const char kNotificationTitle[] = "My Notification";
const char kNotificationLang[] = "nl";
const char kNotificationBody[] = "Hello, world!";
const char kNotificationTag[] = "my_tag";
const char kNotificationImageUrl[] = "https://example.com/image.jpg";
const char kNotificationIconUrl[] = "https://example.com/icon.png";
const char kNotificationBadgeUrl[] = "https://example.com/badge.png";
const int kNotificationVibrationPattern[] = {100, 200, 300};
const double kNotificationTimestamp = 621046800.;
const unsigned char kNotificationData[] = {0xdf, 0xff, 0x0, 0x0, 0xff, 0xdf};
const char kAction1Name[] = "btn1";
const char kAction1Title[] = "Button 1";
const char kAction1IconUrl[] = "https://example.com/action_icon_1.png";
const char kAction1Placeholder[] = "Run into the... friendliness pellets.";
const char kAction2Name[] = "btn2";
const char kAction2Title[] = "Button 2";
const char kAction2IconUrl[] = "https://example.com/action_icon_2.png";

TEST(NotificationDataConversionsTest, ToWebNotificationData) {
  std::vector<int> vibration_pattern(
      kNotificationVibrationPattern,
      kNotificationVibrationPattern + arraysize(kNotificationVibrationPattern));

  std::vector<char> developer_data(
      kNotificationData, kNotificationData + arraysize(kNotificationData));

  PlatformNotificationData platform_data;
  platform_data.title = base::ASCIIToUTF16(kNotificationTitle);
  platform_data.direction = PlatformNotificationData::DIRECTION_LEFT_TO_RIGHT;
  platform_data.lang = kNotificationLang;
  platform_data.body = base::ASCIIToUTF16(kNotificationBody);
  platform_data.tag = kNotificationTag;
  platform_data.image = GURL(kNotificationImageUrl);
  platform_data.icon = GURL(kNotificationIconUrl);
  platform_data.badge = GURL(kNotificationBadgeUrl);
  platform_data.vibration_pattern = vibration_pattern;
  platform_data.timestamp = base::Time::FromJsTime(kNotificationTimestamp);
  platform_data.renotify = true;
  platform_data.silent = true;
  platform_data.require_interaction = true;
  platform_data.data = developer_data;
  platform_data.actions.resize(2);
  platform_data.actions[0].type = PLATFORM_NOTIFICATION_ACTION_TYPE_BUTTON;
  platform_data.actions[0].action = kAction1Name;
  platform_data.actions[0].title = base::ASCIIToUTF16(kAction1Title);
  platform_data.actions[0].icon = GURL(kAction1IconUrl);
  platform_data.actions[0].placeholder =
      base::NullableString16(base::ASCIIToUTF16(kAction1Placeholder), false);
  platform_data.actions[1].type = PLATFORM_NOTIFICATION_ACTION_TYPE_TEXT;
  platform_data.actions[1].action = kAction2Name;
  platform_data.actions[1].title = base::ASCIIToUTF16(kAction2Title);
  platform_data.actions[1].icon = GURL(kAction2IconUrl);
  platform_data.actions[1].placeholder = base::NullableString16();

  blink::WebNotificationData web_data = ToWebNotificationData(platform_data);
  EXPECT_EQ(kNotificationTitle, web_data.title);
  EXPECT_EQ(blink::WebNotificationData::kDirectionLeftToRight,
            web_data.direction);
  EXPECT_EQ(kNotificationLang, web_data.lang);
  EXPECT_EQ(kNotificationBody, web_data.body);
  EXPECT_EQ(kNotificationTag, web_data.tag);
  EXPECT_EQ(kNotificationImageUrl, web_data.image.GetString());
  EXPECT_EQ(kNotificationIconUrl, web_data.icon.GetString());
  EXPECT_EQ(kNotificationBadgeUrl, web_data.badge.GetString());

  ASSERT_EQ(vibration_pattern.size(), web_data.vibrate.size());
  for (size_t i = 0; i < vibration_pattern.size(); ++i)
    EXPECT_EQ(vibration_pattern[i], web_data.vibrate[i]);

  EXPECT_DOUBLE_EQ(kNotificationTimestamp, web_data.timestamp);
  EXPECT_TRUE(web_data.renotify);
  EXPECT_TRUE(web_data.silent);
  EXPECT_TRUE(web_data.require_interaction);

  ASSERT_EQ(developer_data.size(), web_data.data.size());
  for (size_t i = 0; i < developer_data.size(); ++i)
    EXPECT_EQ(developer_data[i], web_data.data[i]);

  ASSERT_EQ(platform_data.actions.size(), web_data.actions.size());
  EXPECT_EQ(blink::WebNotificationAction::kButton, web_data.actions[0].type);
  EXPECT_EQ(kAction1Name, web_data.actions[0].action);
  EXPECT_EQ(kAction1Title, web_data.actions[0].title);
  EXPECT_EQ(kAction1IconUrl, web_data.actions[0].icon.GetString());
  EXPECT_EQ(kAction1Placeholder, web_data.actions[0].placeholder);
  EXPECT_EQ(blink::WebNotificationAction::kText, web_data.actions[1].type);
  EXPECT_EQ(kAction2Name, web_data.actions[1].action);
  EXPECT_EQ(kAction2Title, web_data.actions[1].title);
  EXPECT_EQ(kAction2IconUrl, web_data.actions[1].icon.GetString());
  EXPECT_TRUE(web_data.actions[1].placeholder.IsNull());
}

}  // namespace content
