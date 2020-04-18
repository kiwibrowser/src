// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/notifications/desktop_notification_profile_util.h"

#include "chrome/browser/content_settings/host_content_settings_map_factory.h"
#include "chrome/browser/profiles/profile.h"
#include "components/content_settings/core/browser/host_content_settings_map.h"
#include "components/content_settings/core/common/content_settings_pattern.h"

void DesktopNotificationProfileUtil::ResetToDefaultContentSetting(
    Profile* profile) {
  HostContentSettingsMapFactory::GetForProfile(profile)
      ->SetDefaultContentSetting(CONTENT_SETTINGS_TYPE_NOTIFICATIONS,
                                 CONTENT_SETTING_DEFAULT);
}

// Clears the notifications setting for the given pattern.
void DesktopNotificationProfileUtil::ClearSetting(Profile* profile,
                                                  const GURL& origin) {
  HostContentSettingsMapFactory::GetForProfile(profile)
      ->SetContentSettingDefaultScope(
          origin, GURL(), CONTENT_SETTINGS_TYPE_NOTIFICATIONS,
          content_settings::ResourceIdentifier(), CONTENT_SETTING_DEFAULT);
}

// Methods to setup and modify permission preferences.
void DesktopNotificationProfileUtil::GrantPermission(
    Profile* profile, const GURL& origin) {
  HostContentSettingsMapFactory::GetForProfile(profile)
      ->SetContentSettingDefaultScope(
          origin, GURL(), CONTENT_SETTINGS_TYPE_NOTIFICATIONS,
          content_settings::ResourceIdentifier(), CONTENT_SETTING_ALLOW);
}

void DesktopNotificationProfileUtil::DenyPermission(
    Profile* profile, const GURL& origin) {
  HostContentSettingsMapFactory::GetForProfile(profile)
      ->SetContentSettingDefaultScope(
          origin, GURL(), CONTENT_SETTINGS_TYPE_NOTIFICATIONS,
          content_settings::ResourceIdentifier(), CONTENT_SETTING_BLOCK);
}

void DesktopNotificationProfileUtil::GetNotificationsSettings(
    Profile* profile, ContentSettingsForOneType* settings) {
  HostContentSettingsMapFactory::GetForProfile(profile)
      ->GetSettingsForOneType(CONTENT_SETTINGS_TYPE_NOTIFICATIONS,
                              content_settings::ResourceIdentifier(),
                              settings);
}
