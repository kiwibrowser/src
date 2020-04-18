// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <stddef.h>

#include "base/callback.h"
#include "base/command_line.h"
#include "base/strings/utf_string_conversions.h"
#include "build/build_config.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/net/safe_search_util.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/profiles/profile_attributes_entry.h"
#include "chrome/browser/profiles/profile_attributes_storage.h"
#include "chrome/browser/profiles/profile_manager.h"
#include "chrome/browser/supervised_user/supervised_user_constants.h"
#include "chrome/browser/supervised_user/supervised_user_service.h"
#include "chrome/browser/supervised_user/supervised_user_service_factory.h"
#include "chrome/browser/supervised_user/supervised_user_settings_service.h"
#include "chrome/browser/supervised_user/supervised_user_settings_service_factory.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/common/chrome_switches.h"
#include "chrome/common/pref_names.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "components/prefs/pref_service.h"
#include "content/public/test/test_utils.h"
#include "google_apis/gaia/google_service_auth_error.h"

namespace {

class SupervisedUserServiceTestSupervised : public InProcessBrowserTest {
 public:
  // content::BrowserTestBase:
  void SetUpCommandLine(base::CommandLine* command_line) override {
    command_line->AppendSwitchASCII(switches::kSupervisedUserId, "asdf");
  }
};

}  // namespace

typedef InProcessBrowserTest SupervisedUserServiceTest;

IN_PROC_BROWSER_TEST_F(SupervisedUserServiceTest, LocalPolicies) {
  Profile* profile = browser()->profile();
  PrefService* prefs = profile->GetPrefs();
  EXPECT_FALSE(prefs->GetBoolean(prefs::kForceGoogleSafeSearch));
  EXPECT_EQ(prefs->GetInteger(prefs::kForceYouTubeRestrict),
            safe_search_util::YOUTUBE_RESTRICT_OFF);
  EXPECT_TRUE(prefs->IsUserModifiablePreference(prefs::kForceGoogleSafeSearch));
  EXPECT_TRUE(prefs->IsUserModifiablePreference(prefs::kForceYouTubeRestrict));
}

IN_PROC_BROWSER_TEST_F(SupervisedUserServiceTest, ProfileName) {
  Profile* profile = browser()->profile();
  PrefService* prefs = profile->GetPrefs();
  EXPECT_TRUE(prefs->IsUserModifiablePreference(prefs::kProfileName));

  std::string original_name = prefs->GetString(prefs::kProfileName);
  ProfileAttributesEntry* entry;
  ASSERT_TRUE(g_browser_process->profile_manager()->
                  GetProfileAttributesStorage().
                  GetProfileAttributesWithPath(profile->GetPath(), &entry));
  EXPECT_EQ(original_name, base::UTF16ToUTF8(entry->GetName()));
}

IN_PROC_BROWSER_TEST_F(SupervisedUserServiceTestSupervised, LocalPolicies) {
  Profile* profile = browser()->profile();
  PrefService* prefs = profile->GetPrefs();
  EXPECT_TRUE(prefs->GetBoolean(prefs::kForceGoogleSafeSearch));
  EXPECT_EQ(prefs->GetInteger(prefs::kForceYouTubeRestrict),
            safe_search_util::YOUTUBE_RESTRICT_MODERATE);
  EXPECT_FALSE(
      prefs->IsUserModifiablePreference(prefs::kForceGoogleSafeSearch));
  EXPECT_FALSE(prefs->IsUserModifiablePreference(prefs::kForceYouTubeRestrict));
}

IN_PROC_BROWSER_TEST_F(SupervisedUserServiceTestSupervised, ProfileName) {
  Profile* profile = browser()->profile();
  PrefService* prefs = profile->GetPrefs();
  std::string original_name = prefs->GetString(prefs::kProfileName);

  SupervisedUserSettingsService* settings =
      SupervisedUserSettingsServiceFactory::GetForProfile(profile);

  // Change the name. Both the profile pref and the entry in
  // ProfileAttributesStorage should be updated.
  std::string name = "Supervised User Test Name";
  settings->SetLocalSetting(
      supervised_users::kUserName,
      std::unique_ptr<base::Value>(new base::Value(name)));
  EXPECT_FALSE(prefs->IsUserModifiablePreference(prefs::kProfileName));
  EXPECT_EQ(name, prefs->GetString(prefs::kProfileName));

  ProfileAttributesEntry* entry;
  ASSERT_TRUE(g_browser_process->profile_manager()->
                  GetProfileAttributesStorage().
                  GetProfileAttributesWithPath(profile->GetPath(), &entry));
  EXPECT_EQ(name, base::UTF16ToUTF8(entry->GetName()));

  // Change the name once more.
  std::string new_name = "New Supervised User Test Name";
  settings->SetLocalSetting(
      supervised_users::kUserName,
      std::unique_ptr<base::Value>(new base::Value(new_name)));
  EXPECT_EQ(new_name, prefs->GetString(prefs::kProfileName));
  EXPECT_EQ(new_name, base::UTF16ToUTF8(entry->GetName()));

  // Remove the setting.
  settings->SetLocalSetting(supervised_users::kUserName,
                            std::unique_ptr<base::Value>());
  EXPECT_EQ(original_name, prefs->GetString(prefs::kProfileName));
  EXPECT_EQ(original_name, base::UTF16ToUTF8(entry->GetName()));
}
