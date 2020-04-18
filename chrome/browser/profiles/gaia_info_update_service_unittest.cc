// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/profiles/gaia_info_update_service.h"

#include <stddef.h>

#include <string>

#include "base/strings/utf_string_conversions.h"
#include "build/build_config.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/profiles/profile_attributes_entry.h"
#include "chrome/browser/profiles/profile_attributes_storage.h"
#include "chrome/browser/profiles/profile_downloader.h"
#include "chrome/browser/profiles/profile_info_cache.h"
#include "chrome/browser/profiles/profile_info_cache_unittest.h"
#include "chrome/browser/profiles/profiles_state.h"
#include "chrome/browser/signin/account_tracker_service_factory.h"
#include "chrome/browser/signin/chrome_signin_client_factory.h"
#include "chrome/browser/signin/signin_manager_factory.h"
#include "chrome/browser/signin/test_signin_client_builder.h"
#include "chrome/common/pref_names.h"
#include "chrome/test/base/testing_browser_process.h"
#include "chrome/test/base/testing_profile.h"
#include "chrome/test/base/testing_profile_manager.h"
#include "components/prefs/pref_service.h"
#include "components/signin/core/browser/account_tracker_service.h"
#include "components/signin/core/browser/signin_pref_names.h"
#include "components/sync_preferences/pref_service_syncable.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "ui/gfx/image/image.h"
#include "ui/gfx/image/image_unittest_util.h"

using ::testing::Return;
using ::testing::NiceMock;

namespace {

class ProfileDownloaderMock : public ProfileDownloader {
 public:
  explicit ProfileDownloaderMock(ProfileDownloaderDelegate* delegate)
      : ProfileDownloader(delegate) {
  }

  ~ProfileDownloaderMock() override {}

  MOCK_CONST_METHOD0(GetProfileFullName, base::string16());
  MOCK_CONST_METHOD0(GetProfileGivenName, base::string16());
  MOCK_CONST_METHOD0(GetProfilePicture, SkBitmap());
  MOCK_CONST_METHOD0(GetProfilePictureStatus,
                     ProfileDownloader::PictureStatus());
  MOCK_CONST_METHOD0(GetProfilePictureURL, std::string());
  MOCK_CONST_METHOD0(GetProfileHostedDomain, base::string16());
};

class GAIAInfoUpdateServiceMock : public GAIAInfoUpdateService {
 public:
  explicit GAIAInfoUpdateServiceMock(Profile* profile)
      : GAIAInfoUpdateService(profile) {
  }

  ~GAIAInfoUpdateServiceMock() override {}

  MOCK_METHOD0(Update, void());
};

// TODO(anthonyvd) : remove ProfileInfoCacheTest from the test fixture.
class GAIAInfoUpdateServiceTest : public ProfileInfoCacheTest {
 protected:
  GAIAInfoUpdateServiceTest() : profile_(NULL) {
  }

  Profile* profile() {
    if (!profile_)
      profile_ = CreateProfile("Person 1");
    return profile_;
  }

  ProfileAttributesStorage* storage() {
    return testing_profile_manager_.profile_attributes_storage();
  }

  NiceMock<GAIAInfoUpdateServiceMock>* service() { return service_.get(); }
  NiceMock<ProfileDownloaderMock>* downloader() { return downloader_.get(); }

  Profile* CreateProfile(const std::string& name) {
    TestingProfile::TestingFactories testing_factories;
    testing_factories.push_back(std::make_pair(
        ChromeSigninClientFactory::GetInstance(),
        signin::BuildTestSigninClient));
    Profile* profile = testing_profile_manager_.CreateTestingProfile(
        name, std::unique_ptr<sync_preferences::PrefServiceSyncable>(),
        base::UTF8ToUTF16(name), 0, std::string(), testing_factories);
    // The testing manager sets the profile name manually, which counts as
    // a user-customized profile name. Reset this to match the default name
    // we are actually using.
    ProfileAttributesEntry* entry = nullptr;
    // TODO(anthonyvd) : refactor the function so the following assertion can be
    // changed to ASSERT_TRUE.
    EXPECT_TRUE(storage()->GetProfileAttributesWithPath(profile->GetPath(),
                                                        &entry));
    entry->SetIsUsingDefaultName(true);
    return profile;
  }

  static std::string GivenName(const std::string& id) {
    return id + "first";
  }
  static std::string FullName(const std::string& id) {
    return GivenName(id) + " " + id + "last";
  }
  static base::string16 GivenName16(const std::string& id) {
    return base::UTF8ToUTF16(GivenName(id));
  }
  static base::string16 FullName16(const std::string& id) {
    return base::UTF8ToUTF16(FullName(id));
  }

  void ProfileDownloadSuccess(
      const base::string16& full_name,
      const base::string16& given_name,
      const gfx::Image& image,
      const std::string& url,
      const base::string16& hosted_domain) {
    EXPECT_CALL(*downloader(), GetProfileFullName()).
        WillOnce(Return(full_name));
    EXPECT_CALL(*downloader(), GetProfileGivenName()).
        WillOnce(Return(given_name));
    const SkBitmap* bmp = image.ToSkBitmap();
    EXPECT_CALL(*downloader(), GetProfilePicture()).WillOnce(Return(*bmp));
    EXPECT_CALL(*downloader(), GetProfilePictureStatus()).
        WillOnce(Return(ProfileDownloader::PICTURE_SUCCESS));
    EXPECT_CALL(*downloader(), GetProfilePictureURL()).WillOnce(Return(url));
    EXPECT_CALL(*downloader(), GetProfileHostedDomain()).
        WillOnce(Return(hosted_domain));

    service()->OnProfileDownloadSuccess(downloader());
  }

  void RenameProfile(const base::string16& full_name,
                     const base::string16& given_name) {
    gfx::Image image = gfx::test::CreateImage(256, 256);
    std::string url("foo.com");
    ProfileDownloadSuccess(full_name, given_name, image, url, base::string16());

    // Make sure the right profile was updated correctly.
    ProfileAttributesEntry* entry;
    ASSERT_TRUE(
        storage()->GetProfileAttributesWithPath(profile()->GetPath(), &entry));
    EXPECT_EQ(full_name, entry->GetGAIAName());
    EXPECT_EQ(given_name, entry->GetGAIAGivenName());
  }

 private:
  void SetUp() override;
  void TearDown() override;

  Profile* profile_;
  std::unique_ptr<NiceMock<GAIAInfoUpdateServiceMock>> service_;
  std::unique_ptr<NiceMock<ProfileDownloaderMock>> downloader_;
};

void GAIAInfoUpdateServiceTest::SetUp() {
  ProfileInfoCacheTest::SetUp();
  service_.reset(new NiceMock<GAIAInfoUpdateServiceMock>(profile()));
  downloader_.reset(new NiceMock<ProfileDownloaderMock>(service()));
}

void GAIAInfoUpdateServiceTest::TearDown() {
  downloader_.reset();
  service_->Shutdown();
  service_.reset();
  ProfileInfoCacheTest::TearDown();
}

}  // namespace

TEST_F(GAIAInfoUpdateServiceTest, DownloadSuccess) {
  // No URL should be cached yet.
  EXPECT_EQ(std::string(), service()->GetCachedPictureURL());
  EXPECT_EQ(std::string(), profile()->GetPrefs()->
      GetString(prefs::kGoogleServicesHostedDomain));

  base::string16 name = base::ASCIIToUTF16("Pat Smith");
  base::string16 given_name = base::ASCIIToUTF16("Pat");
  gfx::Image image = gfx::test::CreateImage(256, 256);
  std::string url("foo.com");
  base::string16 hosted_domain(base::ASCIIToUTF16(""));
  ProfileDownloadSuccess(name, given_name, image, url, hosted_domain);

  // On success the GAIA info should be updated.
  ProfileAttributesEntry* entry;
  ASSERT_TRUE(storage()->GetProfileAttributesWithPath(profile()->GetPath(),
                                                      &entry));
  EXPECT_EQ(name, entry->GetGAIAName());
  EXPECT_EQ(given_name, entry->GetGAIAGivenName());
  EXPECT_TRUE(gfx::test::AreImagesEqual(image, *entry->GetGAIAPicture()));
  EXPECT_EQ(url, service()->GetCachedPictureURL());
  EXPECT_EQ(
      AccountTrackerService::kNoHostedDomainFound,
      profile()->GetPrefs()->GetString(prefs::kGoogleServicesHostedDomain));
}

TEST_F(GAIAInfoUpdateServiceTest, DownloadFailure) {
  ProfileAttributesEntry* entry;
  ASSERT_TRUE(storage()->GetProfileAttributesWithPath(profile()->GetPath(),
                                                      &entry));
  base::string16 old_name = entry->GetName();
  gfx::Image old_image = entry->GetAvatarIcon();

  EXPECT_EQ(std::string(), service()->GetCachedPictureURL());

  service()->OnProfileDownloadFailure(downloader(),
                                      ProfileDownloaderDelegate::SERVICE_ERROR);

  // On failure nothing should be updated.
  EXPECT_EQ(old_name, entry->GetName());
  EXPECT_EQ(base::string16(), entry->GetGAIAName());
  EXPECT_EQ(base::string16(), entry->GetGAIAGivenName());
  EXPECT_TRUE(gfx::test::AreImagesEqual(old_image, entry->GetAvatarIcon()));
  EXPECT_EQ(nullptr, entry->GetGAIAPicture());
  EXPECT_EQ(std::string(), service()->GetCachedPictureURL());
  EXPECT_EQ(std::string(),
      profile()->GetPrefs()->GetString(prefs::kGoogleServicesHostedDomain));
}

TEST_F(GAIAInfoUpdateServiceTest, ProfileLockEnabledForWhitelist) {
  // No URL should be cached yet.
  EXPECT_EQ(std::string(), service()->GetCachedPictureURL());

  base::string16 name = base::ASCIIToUTF16("Pat Smith");
  base::string16 given_name = base::ASCIIToUTF16("Pat");
  gfx::Image image = gfx::test::CreateImage(256, 256);
  std::string url("foo.com");
  base::string16 hosted_domain(base::ASCIIToUTF16("google.com"));
  ProfileDownloadSuccess(name, given_name, image, url, hosted_domain);

  EXPECT_EQ("google.com", profile()->GetPrefs()->
      GetString(prefs::kGoogleServicesHostedDomain));
}

// TODO(anthonyvd) : remove or update test once the refactoring of the internals
// of ProfileInfoCache is complete.
TEST_F(GAIAInfoUpdateServiceTest, HandlesProfileReordering) {
  size_t index = GetCache()->GetIndexOfProfileWithPath(profile()->GetPath());
  GetCache()->SetNameOfProfileAtIndex(index, FullName16("B"));
  GetCache()->SetProfileIsUsingDefaultNameAtIndex(index, true);

  CreateProfile(FullName("A"));
  CreateProfile(FullName("C"));
  CreateProfile(FullName("E"));

  size_t index_before =
      GetCache()->GetIndexOfProfileWithPath(profile()->GetPath());

  // Rename our profile.
  RenameProfile(FullName16("D"), GivenName16("D"));
  // Profiles should have been reordered in the cache.
  EXPECT_NE(index_before,
            GetCache()->GetIndexOfProfileWithPath(profile()->GetPath()));
  // Rename the profile back to the original name, it should go back to its
  // original position.
  RenameProfile(FullName16("B"), GivenName16("B"));
  EXPECT_EQ(index_before,
            GetCache()->GetIndexOfProfileWithPath(profile()->GetPath()));

  // Rename only the given name of our profile.
  RenameProfile(FullName16("B"), GivenName16("D"));
  // Rename the profile back to the original name, it should go back to its
  // original position.
  RenameProfile(FullName16("B"), GivenName16("B"));
  EXPECT_EQ(index_before,
            GetCache()->GetIndexOfProfileWithPath(profile()->GetPath()));

  // Rename only the full name of our profile.
  RenameProfile(FullName16("D"), GivenName16("B"));
  // Rename the profile back to the original name, it should go back to its
  // original position.
  RenameProfile(FullName16("B"), GivenName16("B"));
  EXPECT_EQ(index_before,
            GetCache()->GetIndexOfProfileWithPath(profile()->GetPath()));
}

TEST_F(GAIAInfoUpdateServiceTest, ShouldUseGAIAProfileInfo) {
#if defined(OS_CHROMEOS)
  // This feature should never be enabled on ChromeOS.
  EXPECT_FALSE(GAIAInfoUpdateService::ShouldUseGAIAProfileInfo(profile()));
#endif
}

TEST_F(GAIAInfoUpdateServiceTest, ScheduleUpdate) {
  EXPECT_TRUE(service()->timer_.IsRunning());
  service()->timer_.Stop();
  EXPECT_FALSE(service()->timer_.IsRunning());
  service()->ScheduleNextUpdate();
  EXPECT_TRUE(service()->timer_.IsRunning());
}

#if !defined(OS_CHROMEOS)

TEST_F(GAIAInfoUpdateServiceTest, LogOut) {
  SigninManager* signin_manager =
      SigninManagerFactory::GetForProfile(profile());
  signin_manager->SetAuthenticatedAccountInfo("gaia_id", "pat@example.com");
  base::string16 gaia_name = base::UTF8ToUTF16("Pat Foo");

  ASSERT_EQ(1u, storage()->GetNumberOfProfiles());
  ProfileAttributesEntry* entry = storage()->GetAllProfilesAttributes().front();
  entry->SetGAIAName(gaia_name);
  gfx::Image gaia_picture = gfx::test::CreateImage(256, 256);
  entry->SetGAIAPicture(&gaia_picture);

  // Set a fake picture URL.
  profile()->GetPrefs()->SetString(prefs::kProfileGAIAInfoPictureURL,
                                   "example.com");

  EXPECT_FALSE(service()->GetCachedPictureURL().empty());

  // Log out.
  signin_manager->SignOut(signin_metrics::SIGNOUT_TEST,
                          signin_metrics::SignoutDelete::IGNORE_METRIC);
  // Verify that the GAIA name and picture, and picture URL are unset.
  EXPECT_TRUE(entry->GetGAIAName().empty());
  EXPECT_EQ(nullptr, entry->GetGAIAPicture());
  EXPECT_TRUE(service()->GetCachedPictureURL().empty());
}

TEST_F(GAIAInfoUpdateServiceTest, LogIn) {
  // Log in.
  EXPECT_CALL(*service(), Update());
  AccountTrackerServiceFactory::GetForProfile(profile())
      ->SeedAccountInfo("gaia_id", "pat@example.com");
  SigninManager* signin_manager =
      SigninManagerFactory::GetForProfile(profile());
  signin_manager->OnExternalSigninCompleted("pat@example.com");
}

#endif
