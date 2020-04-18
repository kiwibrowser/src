// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/profiles/profile_info_cache_unittest.h"

#include <stddef.h>
#include <stdint.h>

#include <vector>

#include "base/bind.h"
#include "base/command_line.h"
#include "base/files/file_util.h"
#include "base/macros.h"
#include "base/strings/stringprintf.h"
#include "base/strings/utf_string_conversions.h"
#include "base/time/time.h"
#include "build/build_config.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/profiles/avatar_menu.h"
#include "chrome/browser/profiles/profile_avatar_icon_util.h"
#include "chrome/browser/profiles/profile_info_cache.h"
#include "chrome/browser/profiles/profile_manager.h"
#include "chrome/common/chrome_switches.h"
#include "chrome/common/pref_names.h"
#include "chrome/test/base/testing_browser_process.h"
#include "components/account_id/account_id.h"
#include "components/prefs/testing_pref_service.h"
#include "components/signin/core/browser/profile_management_switches.h"
#include "components/sync_preferences/pref_service_syncable.h"
#include "content/public/test/test_browser_thread_bundle.h"
#include "content/public/test/test_utils.h"
#include "third_party/skia/include/core/SkBitmap.h"
#include "ui/base/resource/resource_bundle.h"
#include "ui/gfx/image/image.h"
#include "ui/gfx/image/image_unittest_util.h"

using base::ASCIIToUTF16;
using base::UTF8ToUTF16;
using content::BrowserThread;

ProfileNameVerifierObserver::ProfileNameVerifierObserver(
    TestingProfileManager* testing_profile_manager)
    : testing_profile_manager_(testing_profile_manager) {
  DCHECK(testing_profile_manager_);
}

ProfileNameVerifierObserver::~ProfileNameVerifierObserver() {
}

void ProfileNameVerifierObserver::OnProfileAdded(
    const base::FilePath& profile_path) {
  base::string16 profile_name = GetCache()->GetNameOfProfileAtIndex(
      GetCache()->GetIndexOfProfileWithPath(profile_path));
  EXPECT_TRUE(profile_names_.find(profile_name) == profile_names_.end());
  profile_names_.insert(profile_name);
}

void ProfileNameVerifierObserver::OnProfileWillBeRemoved(
    const base::FilePath& profile_path) {
  base::string16 profile_name = GetCache()->GetNameOfProfileAtIndex(
      GetCache()->GetIndexOfProfileWithPath(profile_path));
  EXPECT_TRUE(profile_names_.find(profile_name) != profile_names_.end());
  profile_names_.erase(profile_name);
}

void ProfileNameVerifierObserver::OnProfileWasRemoved(
    const base::FilePath& profile_path,
    const base::string16& profile_name) {
  EXPECT_TRUE(profile_names_.find(profile_name) == profile_names_.end());
}

void ProfileNameVerifierObserver::OnProfileNameChanged(
    const base::FilePath& profile_path,
    const base::string16& old_profile_name) {
  base::string16 new_profile_name = GetCache()->GetNameOfProfileAtIndex(
      GetCache()->GetIndexOfProfileWithPath(profile_path));
  EXPECT_TRUE(profile_names_.find(old_profile_name) != profile_names_.end());
  EXPECT_TRUE(profile_names_.find(new_profile_name) == profile_names_.end());
  profile_names_.erase(old_profile_name);
  profile_names_.insert(new_profile_name);
}

void ProfileNameVerifierObserver::OnProfileAvatarChanged(
    const base::FilePath& profile_path) {
  base::string16 profile_name = GetCache()->GetNameOfProfileAtIndex(
      GetCache()->GetIndexOfProfileWithPath(profile_path));
  EXPECT_TRUE(profile_names_.find(profile_name) != profile_names_.end());
}

ProfileInfoCache* ProfileNameVerifierObserver::GetCache() {
  return testing_profile_manager_->profile_info_cache();
}

ProfileInfoCacheTest::ProfileInfoCacheTest()
    : testing_profile_manager_(TestingBrowserProcess::GetGlobal()),
      name_observer_(&testing_profile_manager_) {}

ProfileInfoCacheTest::~ProfileInfoCacheTest() {
}

void ProfileInfoCacheTest::SetUp() {
  ASSERT_TRUE(testing_profile_manager_.SetUp());
  testing_profile_manager_.profile_info_cache()->AddObserver(&name_observer_);
}

void ProfileInfoCacheTest::TearDown() {
  // Drain remaining tasks to make sure all tasks are completed. This prevents
  // memory leaks.
  content::RunAllTasksUntilIdle();
}

ProfileInfoCache* ProfileInfoCacheTest::GetCache() {
  return testing_profile_manager_.profile_info_cache();
}

base::FilePath ProfileInfoCacheTest::GetProfilePath(
    const std::string& base_name) {
  return testing_profile_manager_.profile_manager()->user_data_dir().
      AppendASCII(base_name);
}

void ProfileInfoCacheTest::ResetCache() {
  testing_profile_manager_.DeleteProfileInfoCache();
}

TEST_F(ProfileInfoCacheTest, AddProfiles) {
  EXPECT_EQ(0u, GetCache()->GetNumberOfProfiles());

  ui::ResourceBundle& rb = ui::ResourceBundle::GetSharedInstance();
  for (uint32_t i = 0; i < 4; ++i) {
    base::FilePath profile_path =
        GetProfilePath(base::StringPrintf("path_%ud", i));
    base::string16 profile_name =
        ASCIIToUTF16(base::StringPrintf("name_%ud", i));
    const SkBitmap* icon = rb.GetImageNamed(
        profiles::GetDefaultAvatarIconResourceIDAtIndex(
            i)).ToSkBitmap();
    std::string supervised_user_id = i == 3 ? "TEST_ID" : "";

    GetCache()->AddProfileToCache(profile_path, profile_name, std::string(),
                                  base::string16(), i, supervised_user_id,
                                  EmptyAccountId());
    GetCache()->SetBackgroundStatusOfProfileAtIndex(i, true);
    base::string16 gaia_name = ASCIIToUTF16(base::StringPrintf("gaia_%ud", i));
    GetCache()->SetGAIANameOfProfileAtIndex(i, gaia_name);

    EXPECT_EQ(i + 1, GetCache()->GetNumberOfProfiles());
    EXPECT_EQ(profile_name, GetCache()->GetNameOfProfileAtIndex(i));
    EXPECT_EQ(profile_path, GetCache()->GetPathOfProfileAtIndex(i));
    const SkBitmap* actual_icon =
        GetCache()->GetAvatarIconOfProfileAtIndex(i).ToSkBitmap();
    EXPECT_EQ(icon->width(), actual_icon->width());
    EXPECT_EQ(icon->height(), actual_icon->height());
    EXPECT_EQ(i == 3, GetCache()->ProfileIsSupervisedAtIndex(i));
    EXPECT_EQ(i == 3, GetCache()->IsOmittedProfileAtIndex(i));
    EXPECT_EQ(supervised_user_id,
              GetCache()->GetSupervisedUserIdOfProfileAtIndex(i));
  }

  // Reset the cache and test the it reloads correctly.
  ResetCache();

  EXPECT_EQ(4u, GetCache()->GetNumberOfProfiles());
  for (uint32_t i = 0; i < 4; ++i) {
    base::FilePath profile_path =
          GetProfilePath(base::StringPrintf("path_%ud", i));
    EXPECT_EQ(i, GetCache()->GetIndexOfProfileWithPath(profile_path));
    base::string16 profile_name =
        ASCIIToUTF16(base::StringPrintf("name_%ud", i));
    EXPECT_EQ(profile_name, GetCache()->GetNameOfProfileAtIndex(i));
    EXPECT_EQ(i, GetCache()->GetAvatarIconIndexOfProfileAtIndex(i));
    EXPECT_EQ(true, GetCache()->GetBackgroundStatusOfProfileAtIndex(i));
    base::string16 gaia_name = ASCIIToUTF16(base::StringPrintf("gaia_%ud", i));
    EXPECT_EQ(gaia_name, GetCache()->GetGAIANameOfProfileAtIndex(i));
  }
}

TEST_F(ProfileInfoCacheTest, DeleteProfile) {
  EXPECT_EQ(0u, GetCache()->GetNumberOfProfiles());

  base::FilePath path_1 = GetProfilePath("path_1");
  GetCache()->AddProfileToCache(path_1, ASCIIToUTF16("name_1"), std::string(),
                                base::string16(), 0, std::string(),
                                EmptyAccountId());
  EXPECT_EQ(1u, GetCache()->GetNumberOfProfiles());

  base::FilePath path_2 = GetProfilePath("path_2");
  base::string16 name_2 = ASCIIToUTF16("name_2");
  GetCache()->AddProfileToCache(path_2, name_2, std::string(), base::string16(),
                                0, std::string(), EmptyAccountId());
  EXPECT_EQ(2u, GetCache()->GetNumberOfProfiles());

  GetCache()->DeleteProfileFromCache(path_1);
  EXPECT_EQ(1u, GetCache()->GetNumberOfProfiles());
  EXPECT_EQ(name_2, GetCache()->GetNameOfProfileAtIndex(0));

  GetCache()->DeleteProfileFromCache(path_2);
  EXPECT_EQ(0u, GetCache()->GetNumberOfProfiles());
}

TEST_F(ProfileInfoCacheTest, MutateProfile) {
  GetCache()->AddProfileToCache(
      GetProfilePath("path_1"), ASCIIToUTF16("name_1"), std::string(),
      base::string16(), 0, std::string(), EmptyAccountId());
  GetCache()->AddProfileToCache(
      GetProfilePath("path_2"), ASCIIToUTF16("name_2"), std::string(),
      base::string16(), 0, std::string(), EmptyAccountId());

  base::string16 new_name = ASCIIToUTF16("new_name");
  GetCache()->SetNameOfProfileAtIndex(1, new_name);
  EXPECT_EQ(new_name, GetCache()->GetNameOfProfileAtIndex(1));
  EXPECT_NE(new_name, GetCache()->GetNameOfProfileAtIndex(0));

  base::string16 new_user_name = ASCIIToUTF16("user_name");
  std::string new_gaia_id = "12345";
  GetCache()->SetAuthInfoOfProfileAtIndex(1, new_gaia_id, new_user_name);
  EXPECT_EQ(new_user_name, GetCache()->GetUserNameOfProfileAtIndex(1));
  EXPECT_EQ(new_gaia_id, GetCache()->GetGAIAIdOfProfileAtIndex(1));
  EXPECT_NE(new_user_name, GetCache()->GetUserNameOfProfileAtIndex(0));

  const size_t new_icon_index = 3;
  GetCache()->SetAvatarIconOfProfileAtIndex(1, new_icon_index);
  EXPECT_EQ(new_icon_index, GetCache()->GetAvatarIconIndexOfProfileAtIndex(1));
  // Not much to test.
  GetCache()->GetAvatarIconOfProfileAtIndex(1);

  const size_t wrong_icon_index = profiles::GetDefaultAvatarIconCount() + 1;
  const size_t generic_icon_index = 0;
  GetCache()->SetAvatarIconOfProfileAtIndex(1, wrong_icon_index);
  EXPECT_EQ(generic_icon_index,
            GetCache()->GetAvatarIconIndexOfProfileAtIndex(1));
}

TEST_F(ProfileInfoCacheTest, Sort) {
  base::string16 name_a = ASCIIToUTF16("apple");
  GetCache()->AddProfileToCache(GetProfilePath("path_a"), name_a, std::string(),
                                base::string16(), 0, std::string(),
                                EmptyAccountId());

  base::string16 name_c = ASCIIToUTF16("cat");
  GetCache()->AddProfileToCache(GetProfilePath("path_c"), name_c, std::string(),
                                base::string16(), 0, std::string(),
                                EmptyAccountId());

  // Sanity check the initial order.
  EXPECT_EQ(name_a, GetCache()->GetNameOfProfileAtIndex(0));
  EXPECT_EQ(name_c, GetCache()->GetNameOfProfileAtIndex(1));

  // Add a new profile (start with a capital to test case insensitive sorting.
  base::string16 name_b = ASCIIToUTF16("Banana");
  GetCache()->AddProfileToCache(GetProfilePath("path_b"), name_b, std::string(),
                                base::string16(), 0, std::string(),
                                EmptyAccountId());

  // Verify the new order.
  EXPECT_EQ(name_a, GetCache()->GetNameOfProfileAtIndex(0));
  EXPECT_EQ(name_b, GetCache()->GetNameOfProfileAtIndex(1));
  EXPECT_EQ(name_c, GetCache()->GetNameOfProfileAtIndex(2));

  // Change the name of an existing profile.
  name_a = UTF8ToUTF16("dog");
  GetCache()->SetNameOfProfileAtIndex(0, name_a);

  // Verify the new order.
  EXPECT_EQ(name_b, GetCache()->GetNameOfProfileAtIndex(0));
  EXPECT_EQ(name_c, GetCache()->GetNameOfProfileAtIndex(1));
  EXPECT_EQ(name_a, GetCache()->GetNameOfProfileAtIndex(2));

  // Delete a profile.
  GetCache()->DeleteProfileFromCache(GetProfilePath("path_c"));

  // Verify the new order.
  EXPECT_EQ(name_b, GetCache()->GetNameOfProfileAtIndex(0));
  EXPECT_EQ(name_a, GetCache()->GetNameOfProfileAtIndex(1));
}

// Will be removed SOON with ProfileInfoCache tests.
TEST_F(ProfileInfoCacheTest, BackgroundModeStatus) {
  GetCache()->AddProfileToCache(
      GetProfilePath("path_1"), ASCIIToUTF16("name_1"), std::string(),
      base::string16(), 0, std::string(), EmptyAccountId());
  GetCache()->AddProfileToCache(
      GetProfilePath("path_2"), ASCIIToUTF16("name_2"), std::string(),
      base::string16(), 0, std::string(), EmptyAccountId());

  EXPECT_FALSE(GetCache()->GetBackgroundStatusOfProfileAtIndex(0));
  EXPECT_FALSE(GetCache()->GetBackgroundStatusOfProfileAtIndex(1));

  GetCache()->SetBackgroundStatusOfProfileAtIndex(1, true);

  EXPECT_FALSE(GetCache()->GetBackgroundStatusOfProfileAtIndex(0));
  EXPECT_TRUE(GetCache()->GetBackgroundStatusOfProfileAtIndex(1));

  GetCache()->SetBackgroundStatusOfProfileAtIndex(0, true);

  EXPECT_TRUE(GetCache()->GetBackgroundStatusOfProfileAtIndex(0));
  EXPECT_TRUE(GetCache()->GetBackgroundStatusOfProfileAtIndex(1));

  GetCache()->SetBackgroundStatusOfProfileAtIndex(1, false);

  EXPECT_TRUE(GetCache()->GetBackgroundStatusOfProfileAtIndex(0));
  EXPECT_FALSE(GetCache()->GetBackgroundStatusOfProfileAtIndex(1));
}

TEST_F(ProfileInfoCacheTest, GAIAName) {
  GetCache()->AddProfileToCache(
      GetProfilePath("path_1"), ASCIIToUTF16("Person 1"), std::string(),
      base::string16(), 0, std::string(), EmptyAccountId());
  base::string16 profile_name(ASCIIToUTF16("Person 2"));
  GetCache()->AddProfileToCache(GetProfilePath("path_2"), profile_name,
                                std::string(), base::string16(), 0,
                                std::string(), EmptyAccountId());

  int index1 = GetCache()->GetIndexOfProfileWithPath(GetProfilePath("path_1"));
  int index2 = GetCache()->GetIndexOfProfileWithPath(GetProfilePath("path_2"));

  // Sanity check.
  EXPECT_TRUE(GetCache()->GetGAIANameOfProfileAtIndex(index1).empty());
  EXPECT_TRUE(GetCache()->GetGAIANameOfProfileAtIndex(index2).empty());

  // Set GAIA name. This re-sorts the cache.
  base::string16 gaia_name(ASCIIToUTF16("Pat Smith"));
  GetCache()->SetGAIANameOfProfileAtIndex(index2, gaia_name);
  index1 = GetCache()->GetIndexOfProfileWithPath(GetProfilePath("path_1"));
  index2 = GetCache()->GetIndexOfProfileWithPath(GetProfilePath("path_2"));

  // Since there is a GAIA name, we use that as a display name.
  EXPECT_TRUE(GetCache()->GetGAIANameOfProfileAtIndex(index1).empty());
  EXPECT_EQ(gaia_name, GetCache()->GetGAIANameOfProfileAtIndex(index2));
  EXPECT_EQ(gaia_name, GetCache()->GetNameOfProfileAtIndex(index2));

  // Don't use GAIA name as profile name. This re-sorts the cache.
  base::string16 custom_name(ASCIIToUTF16("Custom name"));
  GetCache()->SetNameOfProfileAtIndex(index2, custom_name);
  GetCache()->SetProfileIsUsingDefaultNameAtIndex(index2, false);

  index1 = GetCache()->GetIndexOfProfileWithPath(GetProfilePath("path_1"));
  index2 = GetCache()->GetIndexOfProfileWithPath(GetProfilePath("path_2"));

  EXPECT_EQ(custom_name, GetCache()->GetNameOfProfileAtIndex(index2));
  EXPECT_EQ(gaia_name, GetCache()->GetGAIANameOfProfileAtIndex(index2));
}

TEST_F(ProfileInfoCacheTest, GAIAPicture) {
  const int kDefaultAvatarIndex = 0;
  const int kOtherAvatarIndex = 1;
  const int kGaiaPictureSize = 256;  // Standard size of a Gaia account picture.
  GetCache()->AddProfileToCache(
      GetProfilePath("path_1"), ASCIIToUTF16("name_1"), std::string(),
      base::string16(), kDefaultAvatarIndex, std::string(), EmptyAccountId());
  GetCache()->AddProfileToCache(
      GetProfilePath("path_2"), ASCIIToUTF16("name_2"), std::string(),
      base::string16(), kDefaultAvatarIndex, std::string(), EmptyAccountId());

  // Sanity check.
  EXPECT_EQ(NULL, GetCache()->GetGAIAPictureOfProfileAtIndex(0));
  EXPECT_EQ(NULL, GetCache()->GetGAIAPictureOfProfileAtIndex(1));
  EXPECT_FALSE(GetCache()->IsUsingGAIAPictureOfProfileAtIndex(0));
  EXPECT_FALSE(GetCache()->IsUsingGAIAPictureOfProfileAtIndex(1));

  // The profile icon should be the default one.
  EXPECT_TRUE(GetCache()->ProfileIsUsingDefaultAvatarAtIndex(0));
  EXPECT_TRUE(GetCache()->ProfileIsUsingDefaultAvatarAtIndex(1));
  int default_avatar_id =
      profiles::GetDefaultAvatarIconResourceIDAtIndex(kDefaultAvatarIndex);
  const gfx::Image& default_avatar_image(
      ui::ResourceBundle::GetSharedInstance().GetImageNamed(default_avatar_id));
  EXPECT_TRUE(gfx::test::AreImagesEqual(
      default_avatar_image, GetCache()->GetAvatarIconOfProfileAtIndex(1)));

  // Set GAIA picture.
  gfx::Image gaia_image(gfx::test::CreateImage(
      kGaiaPictureSize, kGaiaPictureSize));
  GetCache()->SetGAIAPictureOfProfileAtIndex(1, &gaia_image);
  EXPECT_EQ(NULL, GetCache()->GetGAIAPictureOfProfileAtIndex(0));
  EXPECT_TRUE(gfx::test::AreImagesEqual(
      gaia_image, *GetCache()->GetGAIAPictureOfProfileAtIndex(1)));
  // Since we're still using the default avatar, the GAIA image should be
  // preferred over the generic avatar image.
  EXPECT_TRUE(GetCache()->ProfileIsUsingDefaultAvatarAtIndex(1));
  EXPECT_TRUE(GetCache()->IsUsingGAIAPictureOfProfileAtIndex(1));
  EXPECT_TRUE(gfx::test::AreImagesEqual(
      gaia_image, GetCache()->GetAvatarIconOfProfileAtIndex(1)));

  // Set a non-default avatar. This should be preferred over the GAIA image.
  GetCache()->SetAvatarIconOfProfileAtIndex(1, kOtherAvatarIndex);
  GetCache()->SetProfileIsUsingDefaultAvatarAtIndex(1, false);
  EXPECT_FALSE(GetCache()->ProfileIsUsingDefaultAvatarAtIndex(1));
  EXPECT_FALSE(GetCache()->IsUsingGAIAPictureOfProfileAtIndex(1));
  int other_avatar_id =
      profiles::GetDefaultAvatarIconResourceIDAtIndex(kOtherAvatarIndex);
  const gfx::Image& other_avatar_image(
      ui::ResourceBundle::GetSharedInstance().GetImageNamed(other_avatar_id));
  EXPECT_TRUE(gfx::test::AreImagesEqual(
      other_avatar_image, GetCache()->GetAvatarIconOfProfileAtIndex(1)));

  // Explicitly setting the GAIA picture should make it preferred again.
  GetCache()->SetIsUsingGAIAPictureOfProfileAtIndex(1, true);
  EXPECT_TRUE(GetCache()->IsUsingGAIAPictureOfProfileAtIndex(1));
  EXPECT_TRUE(gfx::test::AreImagesEqual(
      gaia_image, *GetCache()->GetGAIAPictureOfProfileAtIndex(1)));
  EXPECT_TRUE(gfx::test::AreImagesEqual(
      gaia_image, GetCache()->GetAvatarIconOfProfileAtIndex(1)));

  // Clearing the IsUsingGAIAPicture flag should result in the generic image
  // being used again.
  GetCache()->SetIsUsingGAIAPictureOfProfileAtIndex(1, false);
  EXPECT_FALSE(GetCache()->IsUsingGAIAPictureOfProfileAtIndex(1));
  EXPECT_TRUE(gfx::test::AreImagesEqual(
      gaia_image, *GetCache()->GetGAIAPictureOfProfileAtIndex(1)));
  EXPECT_TRUE(gfx::test::AreImagesEqual(
      other_avatar_image, GetCache()->GetAvatarIconOfProfileAtIndex(1)));
}

TEST_F(ProfileInfoCacheTest, PersistGAIAPicture) {
  GetCache()->AddProfileToCache(
      GetProfilePath("path_1"), ASCIIToUTF16("name_1"), std::string(),
      base::string16(), 0, std::string(), EmptyAccountId());
  gfx::Image gaia_image(gfx::test::CreateImage());

  GetCache()->SetGAIAPictureOfProfileAtIndex(0, &gaia_image);

  // Make sure everything has completed, and the file has been written to disk.
  content::RunAllTasksUntilIdle();

  EXPECT_TRUE(gfx::test::AreImagesEqual(
      gaia_image, *GetCache()->GetGAIAPictureOfProfileAtIndex(0)));

  ResetCache();
  // Try to get the GAIA picture. This should return NULL until the read from
  // disk is done.
  EXPECT_EQ(NULL, GetCache()->GetGAIAPictureOfProfileAtIndex(0));
  content::RunAllTasksUntilIdle();

  EXPECT_TRUE(gfx::test::AreImagesEqual(
    gaia_image, *GetCache()->GetGAIAPictureOfProfileAtIndex(0)));
}

TEST_F(ProfileInfoCacheTest, SetSupervisedUserId) {
  GetCache()->AddProfileToCache(GetProfilePath("test"), ASCIIToUTF16("Test"),
                                std::string(), base::string16(), 0,
                                std::string(), EmptyAccountId());
  EXPECT_FALSE(GetCache()->ProfileIsSupervisedAtIndex(0));

  GetCache()->SetSupervisedUserIdOfProfileAtIndex(0, "TEST_ID");
  EXPECT_TRUE(GetCache()->ProfileIsSupervisedAtIndex(0));
  EXPECT_EQ("TEST_ID", GetCache()->GetSupervisedUserIdOfProfileAtIndex(0));

  ResetCache();
  EXPECT_TRUE(GetCache()->ProfileIsSupervisedAtIndex(0));

  GetCache()->SetSupervisedUserIdOfProfileAtIndex(0, std::string());
  EXPECT_FALSE(GetCache()->ProfileIsSupervisedAtIndex(0));
  EXPECT_EQ("", GetCache()->GetSupervisedUserIdOfProfileAtIndex(0));
}

TEST_F(ProfileInfoCacheTest, EmptyGAIAInfo) {
  base::string16 profile_name = ASCIIToUTF16("name_1");
  int id = profiles::GetDefaultAvatarIconResourceIDAtIndex(0);
  const gfx::Image& profile_image(
      ui::ResourceBundle::GetSharedInstance().GetImageNamed(id));

  GetCache()->AddProfileToCache(GetProfilePath("path_1"), profile_name,
                                std::string(), base::string16(), 0,
                                std::string(), EmptyAccountId());

  // Set empty GAIA info.
  GetCache()->SetGAIANameOfProfileAtIndex(0, base::string16());
  GetCache()->SetGAIAPictureOfProfileAtIndex(0, NULL);
  GetCache()->SetIsUsingGAIAPictureOfProfileAtIndex(0, true);

  // Verify that the profile name and picture are not empty.
  EXPECT_EQ(profile_name, GetCache()->GetNameOfProfileAtIndex(0));
  EXPECT_TRUE(gfx::test::AreImagesEqual(
      profile_image, GetCache()->GetAvatarIconOfProfileAtIndex(0)));
}

TEST_F(ProfileInfoCacheTest, CreateSupervisedTestingProfile) {
  testing_profile_manager_.CreateTestingProfile("default");
  base::string16 supervised_user_name = ASCIIToUTF16("Supervised User");
  testing_profile_manager_.CreateTestingProfile(
      "test1", std::unique_ptr<sync_preferences::PrefServiceSyncable>(),
      supervised_user_name, 0, "TEST_ID", TestingProfile::TestingFactories());
  for (size_t i = 0; i < GetCache()->GetNumberOfProfiles(); i++) {
    bool is_supervised =
        GetCache()->GetNameOfProfileAtIndex(i) == supervised_user_name;
    EXPECT_EQ(is_supervised, GetCache()->ProfileIsSupervisedAtIndex(i));
    std::string supervised_user_id = is_supervised ? "TEST_ID" : "";
    EXPECT_EQ(supervised_user_id,
              GetCache()->GetSupervisedUserIdOfProfileAtIndex(i));
  }

  // Supervised profiles have a custom theme, which needs to be deleted on the
  // FILE thread. Reset the profile manager now so everything is deleted while
  // we still have a FILE thread.
  TestingBrowserProcess::GetGlobal()->SetProfileManager(NULL);
}

TEST_F(ProfileInfoCacheTest, AddStubProfile) {
  EXPECT_EQ(0u, GetCache()->GetNumberOfProfiles());

  // Add some profiles with and without a '.' in their paths.
  const struct {
    const char* profile_path;
    const char* profile_name;
  } kTestCases[] = {
    { "path.test0", "name_0" },
    { "path_test1", "name_1" },
    { "path.test2", "name_2" },
    { "path_test3", "name_3" },
  };

  for (size_t i = 0; i < arraysize(kTestCases); ++i) {
    base::FilePath profile_path = GetProfilePath(kTestCases[i].profile_path);
    base::string16 profile_name = ASCIIToUTF16(kTestCases[i].profile_name);

    GetCache()->AddProfileToCache(profile_path, profile_name, std::string(),
                                  base::string16(), i, "", EmptyAccountId());

    EXPECT_EQ(profile_path, GetCache()->GetPathOfProfileAtIndex(i));
    EXPECT_EQ(profile_name, GetCache()->GetNameOfProfileAtIndex(i));
  }

  ASSERT_EQ(4U, GetCache()->GetNumberOfProfiles());

  // Check that the profiles can be extracted from the local state.
  std::vector<base::string16> names;
  PrefService* local_state = g_browser_process->local_state();
  const base::DictionaryValue* cache = local_state->GetDictionary(
      prefs::kProfileInfoCache);
  base::string16 name;
  for (base::DictionaryValue::Iterator it(*cache); !it.IsAtEnd();
       it.Advance()) {
    const base::DictionaryValue* info = NULL;
    it.value().GetAsDictionary(&info);
    info->GetString("name", &name);
    names.push_back(name);
  }

  for (size_t i = 0; i < 4; i++)
    ASSERT_FALSE(names[i].empty());
}

TEST_F(ProfileInfoCacheTest, EntriesInAttributesStorage) {
  EXPECT_EQ(0u, GetCache()->GetNumberOfProfiles());

  // Add some profiles with and without a '.' in their paths.
  const struct {
    const char* profile_path;
    const char* profile_name;
  } kTestCases[] = {
    { "path.test0", "name_0" },
    { "path_test1", "name_1" },
    { "path.test2", "name_2" },
    { "path_test3", "name_3" },
  };

  // Profiles are added and removed using all combinations of the old and the
  // new interfaces. The content of |profile_attributes_entries_| in
  // ProfileAttributesStorage is checked after each insert and delete operation.

  // Add profiles.
  for (size_t i = 0; i < arraysize(kTestCases); ++i) {
    base::FilePath profile_path = GetProfilePath(kTestCases[i].profile_path);
    base::string16 profile_name = ASCIIToUTF16(kTestCases[i].profile_name);

    ASSERT_EQ(0u, GetCache()->profile_attributes_entries_.count(
                      profile_path.value()));

    // Use ProfileInfoCache in profiles 0 and 2, and ProfileAttributesStorage in
    // profiles 1 and 3.
    if (i | 1u) {
      GetCache()->AddProfileToCache(profile_path, profile_name, std::string(),
                                    base::string16(), i, "", EmptyAccountId());
    } else {
      GetCache()->AddProfile(profile_path, profile_name, std::string(),
                             base::string16(), i, "", EmptyAccountId());
    }

    ASSERT_EQ(i + 1, GetCache()->GetNumberOfProfiles());
    ASSERT_EQ(i + 1, GetCache()->profile_attributes_entries_.size());

    ASSERT_EQ(1u, GetCache()->profile_attributes_entries_.count(
                      profile_path.value()));
    // TODO(anthonyvd) : check that the entry in |profile_attributes_entries_|
    // is null before GetProfileAttributesWithPath is run. Currently this is
    // impossible to check because GetProfileAttributesWithPath is called during
    // profile creation.

    ProfileAttributesEntry* entry = nullptr;
    GetCache()->GetProfileAttributesWithPath(profile_path, &entry);
    EXPECT_EQ(
        entry,
        GetCache()->profile_attributes_entries_[profile_path.value()].get());
  }

  // Remove profiles.
  for (size_t i = 0; i < arraysize(kTestCases); ++i) {
    base::FilePath profile_path = GetProfilePath(kTestCases[i].profile_path);
    ASSERT_EQ(1u, GetCache()->profile_attributes_entries_.count(
                      profile_path.value()));

    // Use ProfileInfoCache in profiles 0 and 1, and ProfileAttributesStorage in
    // profiles 2 and 3.
    if (i | 2u)
      GetCache()->DeleteProfileFromCache(profile_path);
    else
      GetCache()->RemoveProfile(profile_path);

    ASSERT_EQ(0u, GetCache()->profile_attributes_entries_.count(
                      profile_path.value()));

    ProfileAttributesEntry* entry = nullptr;
    EXPECT_FALSE(GetCache()->GetProfileAttributesWithPath(profile_path,
                                                          &entry));
    ASSERT_EQ(0u, GetCache()->profile_attributes_entries_.count(
                      profile_path.value()));
  }
}

#if !defined(OS_ANDROID) && !defined(OS_CHROMEOS)
TEST_F(ProfileInfoCacheTest, MigrateLegacyProfileNamesWithNewAvatarMenu) {
  EXPECT_EQ(0U, GetCache()->GetNumberOfProfiles());

  base::FilePath path_1 = GetProfilePath("path_1");
  GetCache()->AddProfileToCache(path_1, ASCIIToUTF16("Default Profile"),
                                std::string(), base::string16(), 0,
                                std::string(), EmptyAccountId());
  base::FilePath path_2 = GetProfilePath("path_2");
  GetCache()->AddProfileToCache(path_2, ASCIIToUTF16("First user"),
                                std::string(), base::string16(), 1,
                                std::string(), EmptyAccountId());
  base::string16 name_3 = ASCIIToUTF16("Lemonade");
  base::FilePath path_3 = GetProfilePath("path_3");
  GetCache()->AddProfileToCache(path_3, name_3, std::string(), base::string16(),
                                2, std::string(), EmptyAccountId());
  base::string16 name_4 = ASCIIToUTF16("Batman");
  base::FilePath path_4 = GetProfilePath("path_4");
  GetCache()->AddProfileToCache(path_4, name_4, std::string(), base::string16(),
                                3, std::string(), EmptyAccountId());
  base::string16 name_5 = ASCIIToUTF16("Person 2");
  base::FilePath path_5 = GetProfilePath("path_5");
  GetCache()->AddProfileToCache(path_5, name_5, std::string(), base::string16(),
                                2, std::string(), EmptyAccountId());

  EXPECT_EQ(5U, GetCache()->GetNumberOfProfiles());


  ResetCache();

  // Legacy profile names like "Default Profile" and "First user" should be
  // migrated to "Person %n" type names, i.e. any permutation of "Person 1" and
  // "Person 3".
  if (ASCIIToUTF16("Person 1") ==
      GetCache()->GetNameOfProfileAtIndex(
          GetCache()->GetIndexOfProfileWithPath(path_1))) {
    EXPECT_EQ(ASCIIToUTF16("Person 3"),
              GetCache()->GetNameOfProfileAtIndex(
                  GetCache()->GetIndexOfProfileWithPath(path_2)));
  } else {
    EXPECT_EQ(ASCIIToUTF16("Person 3"),
              GetCache()->GetNameOfProfileAtIndex(
                  GetCache()->GetIndexOfProfileWithPath(path_1)));
    EXPECT_EQ(ASCIIToUTF16("Person 1"),
              GetCache()->GetNameOfProfileAtIndex(
                  GetCache()->GetIndexOfProfileWithPath(path_2)));
  }

  // Other profile names should not be migrated even if they're the old
  // default cartoon profile names.
  EXPECT_EQ(name_3, GetCache()->GetNameOfProfileAtIndex(
      GetCache()->GetIndexOfProfileWithPath(path_3)));
  EXPECT_EQ(name_4, GetCache()->GetNameOfProfileAtIndex(
      GetCache()->GetIndexOfProfileWithPath(path_4)));
  EXPECT_EQ(name_5, GetCache()->GetNameOfProfileAtIndex(
      GetCache()->GetIndexOfProfileWithPath(path_5)));
}

TEST_F(ProfileInfoCacheTest, GetGaiaImageForAvatarMenu) {
  // The TestingProfileManager's ProfileInfoCache doesn't download avatars.
  ProfileInfoCache profile_info_cache(
      g_browser_process->local_state(),
      testing_profile_manager_.profile_manager()->user_data_dir());

  base::FilePath profile_path = GetProfilePath("path_1");

  GetCache()->AddProfileToCache(profile_path, ASCIIToUTF16("name_1"),
                                std::string(), base::string16(), 0,
                                std::string(), EmptyAccountId());

  gfx::Image gaia_image(gfx::test::CreateImage());
  GetCache()->SetGAIAPictureOfProfileAtIndex(0, &gaia_image);

  // Make sure everything has completed, and the file has been written to disk.
  content::RunAllTasksUntilIdle();

  // Make sure this profile is using GAIA picture.
  EXPECT_TRUE(GetCache()->IsUsingGAIAPictureOfProfileAtIndex(0));

  ResetCache();

  // We need to explicitly set the GAIA usage flag after resetting the cache.
  GetCache()->SetIsUsingGAIAPictureOfProfileAtIndex(0, true);
  EXPECT_TRUE(GetCache()->IsUsingGAIAPictureOfProfileAtIndex(0));

  gfx::Image image_loaded;

  // Try to get the GAIA image. For the first time, it triggers an async image
  // load from disk. The load status indicates the image is still being loaded.
  EXPECT_EQ(AvatarMenu::ImageLoadStatus::LOADING,
            AvatarMenu::GetImageForMenuButton(profile_path, &image_loaded));
  EXPECT_FALSE(gfx::test::AreImagesEqual(gaia_image, image_loaded));

  // Wait until the async image load finishes.
  content::RunAllTasksUntilIdle();

  // Since the GAIA image is loaded now, we can get it this time.
  EXPECT_EQ(AvatarMenu::ImageLoadStatus::LOADED,
            AvatarMenu::GetImageForMenuButton(profile_path, &image_loaded));
  EXPECT_TRUE(gfx::test::AreImagesEqual(gaia_image, image_loaded));
}
#endif

#if defined(OS_CHROMEOS) || defined(OS_ANDROID)
TEST_F(ProfileInfoCacheTest,
       DontMigrateLegacyProfileNamesWithoutNewAvatarMenu) {
  EXPECT_EQ(0U, GetCache()->GetNumberOfProfiles());

  base::string16 name_1 = ASCIIToUTF16("Default Profile");
  base::FilePath path_1 = GetProfilePath("path_1");
  GetCache()->AddProfileToCache(path_1, name_1, std::string(), base::string16(),
                                0, std::string(), EmptyAccountId());
  base::string16 name_2 = ASCIIToUTF16("First user");
  base::FilePath path_2 = GetProfilePath("path_2");
  GetCache()->AddProfileToCache(path_2, name_2, std::string(), base::string16(),
                                1, std::string(), EmptyAccountId());
  base::string16 name_3 = ASCIIToUTF16("Lemonade");
  base::FilePath path_3 = GetProfilePath("path_3");
  GetCache()->AddProfileToCache(path_3, name_3, std::string(), base::string16(),
                                2, std::string(), EmptyAccountId());
  base::string16 name_4 = ASCIIToUTF16("Batman");
  base::FilePath path_4 = GetProfilePath("path_4");
  GetCache()->AddProfileToCache(path_4, name_4, std::string(), base::string16(),
                                3, std::string(), EmptyAccountId());
  EXPECT_EQ(4U, GetCache()->GetNumberOfProfiles());

  ResetCache();

  // Profile names should have been preserved.
  EXPECT_EQ(name_1, GetCache()->GetNameOfProfileAtIndex(
      GetCache()->GetIndexOfProfileWithPath(path_1)));
  EXPECT_EQ(name_2, GetCache()->GetNameOfProfileAtIndex(
      GetCache()->GetIndexOfProfileWithPath(path_2)));
  EXPECT_EQ(name_3, GetCache()->GetNameOfProfileAtIndex(
      GetCache()->GetIndexOfProfileWithPath(path_3)));
  EXPECT_EQ(name_4, GetCache()->GetNameOfProfileAtIndex(
      GetCache()->GetIndexOfProfileWithPath(path_4)));
}
#endif

TEST_F(ProfileInfoCacheTest, RemoveProfileByAccountId) {
  EXPECT_EQ(0u, GetCache()->GetNumberOfProfiles());

  base::FilePath path_1 = GetProfilePath("path_1");
  const AccountId account_id_1(
      AccountId::FromUserEmailGaiaId("email1", "111111"));
  base::string16 name_1 = ASCIIToUTF16("name_1");
  GetCache()->AddProfileToCache(path_1, name_1, account_id_1.GetGaiaId(),
                                UTF8ToUTF16(account_id_1.GetUserEmail()), 0,
                                std::string(), EmptyAccountId());
  EXPECT_EQ(1u, GetCache()->GetNumberOfProfiles());

  base::FilePath path_2 = GetProfilePath("path_2");
  base::string16 name_2 = ASCIIToUTF16("name_2");
  const AccountId account_id_2(
      AccountId::FromUserEmailGaiaId("email2", "222222"));
  GetCache()->AddProfileToCache(path_2, name_2, account_id_2.GetGaiaId(),
                                UTF8ToUTF16(account_id_2.GetUserEmail()), 0,
                                std::string(), EmptyAccountId());
  EXPECT_EQ(2u, GetCache()->GetNumberOfProfiles());

  base::FilePath path_3 = GetProfilePath("path_3");
  base::string16 name_3 = ASCIIToUTF16("name_3");
  const AccountId account_id_3(
      AccountId::FromUserEmailGaiaId("email3", "333333"));
  GetCache()->AddProfileToCache(path_3, name_3, account_id_3.GetGaiaId(),
                                UTF8ToUTF16(account_id_3.GetUserEmail()), 0,
                                std::string(), EmptyAccountId());
  EXPECT_EQ(3u, GetCache()->GetNumberOfProfiles());

  base::FilePath path_4 = GetProfilePath("path_4");
  base::string16 name_4 = ASCIIToUTF16("name_4");
  const AccountId account_id_4(
      AccountId::FromUserEmailGaiaId("email4", "444444"));
  GetCache()->AddProfileToCache(path_4, name_4, account_id_4.GetGaiaId(),
                                UTF8ToUTF16(account_id_4.GetUserEmail()), 0,
                                std::string(), EmptyAccountId());
  EXPECT_EQ(4u, GetCache()->GetNumberOfProfiles());

  GetCache()->RemoveProfileByAccountId(account_id_3);
  EXPECT_EQ(3u, GetCache()->GetNumberOfProfiles());
  EXPECT_EQ(name_1, GetCache()->GetNameOfProfileAtIndex(0));

  GetCache()->RemoveProfileByAccountId(account_id_1);
  EXPECT_EQ(2u, GetCache()->GetNumberOfProfiles());
  EXPECT_EQ(name_2, GetCache()->GetNameOfProfileAtIndex(0));

  // this profile is already deleted.
  GetCache()->RemoveProfileByAccountId(account_id_3);
  EXPECT_EQ(2u, GetCache()->GetNumberOfProfiles());
  EXPECT_EQ(name_2, GetCache()->GetNameOfProfileAtIndex(0));

  // Remove profile by partial match
  GetCache()->RemoveProfileByAccountId(
      AccountId::FromUserEmail(account_id_2.GetUserEmail()));
  EXPECT_EQ(1u, GetCache()->GetNumberOfProfiles());
  EXPECT_EQ(name_4, GetCache()->GetNameOfProfileAtIndex(0));

  // Remove last profile
  GetCache()->RemoveProfileByAccountId(account_id_4);
  EXPECT_EQ(0u, GetCache()->GetNumberOfProfiles());
}
