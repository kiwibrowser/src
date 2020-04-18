// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/webui/site_settings_helper.h"

#include "chrome/browser/content_settings/host_content_settings_map_factory.h"
#include "chrome/common/pref_names.h"
#include "chrome/test/base/testing_profile.h"
#include "components/content_settings/core/browser/host_content_settings_map.h"
#include "components/content_settings/core/common/content_settings_utils.h"
#include "components/content_settings/core/test/content_settings_mock_provider.h"
#include "components/content_settings/core/test/content_settings_test_utils.h"
#include "components/prefs/pref_service.h"
#include "content/public/test/test_browser_thread_bundle.h"
#include "extensions/browser/extension_registry.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

namespace site_settings {

namespace {
constexpr ContentSettingsType kContentType = CONTENT_SETTINGS_TYPE_GEOLOCATION;
}

class SiteSettingsHelperTest : public testing::Test {
 public:
  void VerifySetting(const base::ListValue& exceptions,
                     int index,
                     const std::string& pattern,
                     const std::string& pattern_display_name,
                     const ContentSetting setting) {
    const base::DictionaryValue* dict;
    exceptions.GetDictionary(index, &dict);
    std::string actual_pattern;
    dict->GetString("origin", &actual_pattern);
    EXPECT_EQ(pattern, actual_pattern);
    std::string actual_display_name;
    dict->GetString(kDisplayName, &actual_display_name);
    EXPECT_EQ(pattern_display_name, actual_display_name);
    std::string actual_setting;
    dict->GetString(kSetting, &actual_setting);
    EXPECT_EQ(content_settings::ContentSettingToString(setting),
              actual_setting);
  }

  void AddSetting(HostContentSettingsMap* map,
                  const std::string& pattern,
                  ContentSetting setting) {
    map->SetContentSettingCustomScope(
        ContentSettingsPattern::FromString(pattern),
        ContentSettingsPattern::Wildcard(), kContentType, std::string(),
        setting);
  }

 private:
  content::TestBrowserThreadBundle thread_bundle_;
};

TEST_F(SiteSettingsHelperTest, CheckExceptionOrder) {
  TestingProfile profile;
  HostContentSettingsMap* map =
      HostContentSettingsMapFactory::GetForProfile(&profile);

  base::ListValue exceptions;
  // Check that the initial state of the map is empty.
  GetExceptionsFromHostContentSettingsMap(
      map, kContentType, /*extension_registry=*/nullptr, /*web_ui=*/nullptr,
      /*incognito=*/false, /*filter=*/nullptr, &exceptions);
  EXPECT_EQ(0u, exceptions.GetSize());

  map->SetDefaultContentSetting(kContentType, CONTENT_SETTING_ALLOW);

  // Add a policy exception.
  std::string star_google_com = "http://[*.]google.com";
  auto policy_provider = std::make_unique<content_settings::MockProvider>();
  policy_provider->SetWebsiteSetting(
      ContentSettingsPattern::FromString(star_google_com),
      ContentSettingsPattern::Wildcard(), kContentType, "",
      new base::Value(CONTENT_SETTING_BLOCK));
  policy_provider->set_read_only(true);
  content_settings::TestUtils::OverrideProvider(
      map, std::move(policy_provider), HostContentSettingsMap::POLICY_PROVIDER);

  // Add user preferences.
  std::string http_star = "http://*";
  std::string maps_google_com = "http://maps.google.com";
  AddSetting(map, http_star, CONTENT_SETTING_BLOCK);
  AddSetting(map, maps_google_com, CONTENT_SETTING_BLOCK);
  AddSetting(map, star_google_com, CONTENT_SETTING_ALLOW);

  // Add an extension exception.
  std::string drive_google_com = "http://drive.google.com";
  auto extension_provider = std::make_unique<content_settings::MockProvider>();
  extension_provider->SetWebsiteSetting(
      ContentSettingsPattern::FromString(drive_google_com),
      ContentSettingsPattern::Wildcard(), kContentType, "",
      new base::Value(CONTENT_SETTING_ASK));
  extension_provider->set_read_only(true);
  content_settings::TestUtils::OverrideProvider(
      map, std::move(extension_provider),
      HostContentSettingsMap::CUSTOM_EXTENSION_PROVIDER);

  exceptions.Clear();
  GetExceptionsFromHostContentSettingsMap(
      map, kContentType, /*extension_registry=*/nullptr, /*web_ui=*/nullptr,
      /*incognito=*/false, /*filter=*/nullptr, &exceptions);

  EXPECT_EQ(5u, exceptions.GetSize());

  // The policy exception should be returned first, the extension exception
  // second and pref exceptions afterwards.
  // The default content setting should not be returned.
  int i = 0;
  // From policy provider:
  VerifySetting(exceptions, i++, star_google_com, star_google_com,
                CONTENT_SETTING_BLOCK);
  // From extension provider:
  VerifySetting(exceptions, i++, drive_google_com, drive_google_com,
                CONTENT_SETTING_ASK);
  // From user preferences:
  VerifySetting(exceptions, i++, maps_google_com, maps_google_com,
                CONTENT_SETTING_BLOCK);
  VerifySetting(exceptions, i++, star_google_com, star_google_com,
                CONTENT_SETTING_ALLOW);
  VerifySetting(exceptions, i++, http_star, "http://*", CONTENT_SETTING_BLOCK);
}

// Tests the following content setting sources: Chrome default, user-set global
// default, user-set pattern, user-set origin setting, extension, and policy.
TEST_F(SiteSettingsHelperTest, ContentSettingSource) {
  TestingProfile profile;
  HostContentSettingsMap* map =
      HostContentSettingsMapFactory::GetForProfile(&profile);

  GURL origin("https://www.example.com/");
  auto* extension_registry = extensions::ExtensionRegistry::Get(&profile);
  std::string source;
  std::string display_name;
  ContentSetting content_setting;

  // Built in Chrome default.
  content_setting =
      GetContentSettingForOrigin(&profile, map, origin, kContentType, &source,
                                 extension_registry, &display_name);
  EXPECT_EQ(SiteSettingSourceToString(SiteSettingSource::kDefault), source);
  EXPECT_EQ(CONTENT_SETTING_ASK, content_setting);

  // User-set global default.
  map->SetDefaultContentSetting(kContentType, CONTENT_SETTING_ALLOW);
  content_setting =
      GetContentSettingForOrigin(&profile, map, origin, kContentType, &source,
                                 extension_registry, &display_name);
  EXPECT_EQ(SiteSettingSourceToString(SiteSettingSource::kDefault), source);
  EXPECT_EQ(CONTENT_SETTING_ALLOW, content_setting);

  // User-set pattern.
  AddSetting(map, "https://*", CONTENT_SETTING_BLOCK);
  content_setting =
      GetContentSettingForOrigin(&profile, map, origin, kContentType, &source,
                                 extension_registry, &display_name);
  EXPECT_EQ(SiteSettingSourceToString(SiteSettingSource::kPreference), source);
  EXPECT_EQ(CONTENT_SETTING_BLOCK, content_setting);

  // User-set origin setting.
  map->SetContentSettingDefaultScope(origin, origin, kContentType,
                                     std::string(), CONTENT_SETTING_ALLOW);
  content_setting =
      GetContentSettingForOrigin(&profile, map, origin, kContentType, &source,
                                 extension_registry, &display_name);
  EXPECT_EQ(SiteSettingSourceToString(SiteSettingSource::kPreference), source);
  EXPECT_EQ(CONTENT_SETTING_ALLOW, content_setting);

// ChromeOS - DRM disabled.
#if defined(OS_CHROMEOS)
  profile.GetPrefs()->SetBoolean(prefs::kEnableDRM, false);
  // Note this is not testing |kContentType|, because this setting is only valid
  // for protected content.
  content_setting = GetContentSettingForOrigin(
      &profile, map, origin, CONTENT_SETTINGS_TYPE_PROTECTED_MEDIA_IDENTIFIER,
      &source, extension_registry, &display_name);
  EXPECT_EQ(SiteSettingSourceToString(SiteSettingSource::kDrmDisabled), source);
  EXPECT_EQ(CONTENT_SETTING_BLOCK, content_setting);
#endif

  // Extension.
  auto extension_provider = std::make_unique<content_settings::MockProvider>();
  extension_provider->SetWebsiteSetting(ContentSettingsPattern::FromURL(origin),
                                        ContentSettingsPattern::FromURL(origin),
                                        kContentType, "",
                                        new base::Value(CONTENT_SETTING_BLOCK));
  extension_provider->set_read_only(true);
  content_settings::TestUtils::OverrideProvider(
      map, std::move(extension_provider),
      HostContentSettingsMap::CUSTOM_EXTENSION_PROVIDER);
  content_setting =
      GetContentSettingForOrigin(&profile, map, origin, kContentType, &source,
                                 extension_registry, &display_name);
  EXPECT_EQ(SiteSettingSourceToString(SiteSettingSource::kExtension), source);
  EXPECT_EQ(CONTENT_SETTING_BLOCK, content_setting);

  // Enterprise policy.
  auto policy_provider = std::make_unique<content_settings::MockProvider>();
  policy_provider->SetWebsiteSetting(ContentSettingsPattern::FromURL(origin),
                                     ContentSettingsPattern::FromURL(origin),
                                     kContentType, "",
                                     new base::Value(CONTENT_SETTING_ALLOW));
  policy_provider->set_read_only(true);
  content_settings::TestUtils::OverrideProvider(
      map, std::move(policy_provider), HostContentSettingsMap::POLICY_PROVIDER);
  content_setting =
      GetContentSettingForOrigin(&profile, map, origin, kContentType, &source,
                                 extension_registry, &display_name);
  EXPECT_EQ(SiteSettingSourceToString(SiteSettingSource::kPolicy), source);
  EXPECT_EQ(CONTENT_SETTING_ALLOW, content_setting);

  // Insecure origins.
  content_setting = GetContentSettingForOrigin(
      &profile, map, GURL("http://www.insecure_http_site.com/"), kContentType,
      &source, extension_registry, &display_name);
  EXPECT_EQ(SiteSettingSourceToString(SiteSettingSource::kInsecureOrigin),
            source);
  EXPECT_EQ(CONTENT_SETTING_BLOCK, content_setting);
}

}  // namespace site_settings
