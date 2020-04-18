// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/profiles/profile_downloader.h"

#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"
#include "chrome/browser/profiles/profile_downloader_delegate.h"
#include "chrome/browser/signin/account_fetcher_service_factory.h"
#include "chrome/browser/signin/account_tracker_service_factory.h"
#include "chrome/browser/signin/fake_account_fetcher_service_builder.h"
#include "chrome/browser/signin/fake_profile_oauth2_token_service_builder.h"
#include "chrome/browser/signin/profile_oauth2_token_service_factory.h"
#include "chrome/browser/signin/test_signin_client_builder.h"
#include "chrome/test/base/testing_profile.h"
#include "components/signin/core/browser/account_tracker_service.h"
#include "components/signin/core/browser/fake_account_fetcher_service.h"
#include "components/signin/core/browser/test_signin_client.h"
#include "content/public/test/test_browser_thread_bundle.h"
#include "net/url_request/test_url_fetcher_factory.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace {

const std::string kTestEmail = "test@example.com";
const std::string kTestGaia = "gaia";
const std::string kTestHostedDomain = "google.com";
const std::string kTestFullName = "full_name";
const std::string kTestGivenName = "given_name";
const std::string kTestLocale = "locale";
const std::string kTestValidPictureURL = "http://www.google.com/";
const std::string kTestInvalidPictureURL = "invalid_picture_url";

} // namespace

class ProfileDownloaderTest : public testing::Test,
                              public ProfileDownloaderDelegate {
 protected:
  ProfileDownloaderTest()
    : thread_bundle_(content::TestBrowserThreadBundle::IO_MAINLOOP) {}
  ~ProfileDownloaderTest() override {}

  void SetUp() override {
    TestingProfile::Builder builder;
    builder.AddTestingFactory(ProfileOAuth2TokenServiceFactory::GetInstance(),
                              &BuildAutoIssuingFakeProfileOAuth2TokenService);
    builder.AddTestingFactory(AccountFetcherServiceFactory::GetInstance(),
                              FakeAccountFetcherServiceBuilder::BuildForTests);
    profile_ = builder.Build();
    account_tracker_service_ =
        AccountTrackerServiceFactory::GetForProfile(profile_.get());
    account_fetcher_service_ = static_cast<FakeAccountFetcherService*>(
        AccountFetcherServiceFactory::GetForProfile(profile_.get()));
    profile_downloader_.reset(new ProfileDownloader(this));
  }

  bool NeedsProfilePicture() const override { return true; };
  int GetDesiredImageSideLength() const override { return 128; };
  std::string GetCachedPictureURL() const override { return ""; };
  Profile* GetBrowserProfile() override { return profile_.get(); };
  bool IsPreSignin() const override { return false; }
  void OnProfileDownloadSuccess(ProfileDownloader* downloader) override {

  }
  void OnProfileDownloadFailure(
      ProfileDownloader* downloader,
      ProfileDownloaderDelegate::FailureReason reason) override {}

  void SimulateUserInfoSuccess(const std::string& picture_url) {
    account_fetcher_service_->FakeUserInfoFetchSuccess(
        account_tracker_service_->PickAccountIdForAccount(kTestGaia,
                                                          kTestEmail),
        kTestEmail, kTestGaia, kTestHostedDomain, kTestFullName, kTestGivenName,
        kTestLocale, picture_url);
  }

  AccountTrackerService* account_tracker_service_;
  FakeAccountFetcherService* account_fetcher_service_;
  content::TestBrowserThreadBundle thread_bundle_;
  std::unique_ptr<Profile> profile_;
  std::unique_ptr<ProfileDownloader> profile_downloader_;
};

TEST_F(ProfileDownloaderTest, AccountInfoReady) {
  std::string account_id =
      account_tracker_service_->SeedAccountInfo(kTestGaia, kTestEmail);
  SimulateUserInfoSuccess(kTestValidPictureURL);

  ASSERT_EQ(ProfileDownloader::PICTURE_FAILED,
            profile_downloader_->GetProfilePictureStatus());
  profile_downloader_->StartForAccount(account_id);
  profile_downloader_->StartFetchingImage();
  ASSERT_EQ(kTestValidPictureURL, profile_downloader_->GetProfilePictureURL());
}

TEST_F(ProfileDownloaderTest, AccountInfoNotReady) {
  std::string account_id =
      account_tracker_service_->SeedAccountInfo(kTestGaia, kTestEmail);

  ASSERT_EQ(ProfileDownloader::PICTURE_FAILED,
            profile_downloader_->GetProfilePictureStatus());
  profile_downloader_->StartForAccount(account_id);
  profile_downloader_->StartFetchingImage();
  SimulateUserInfoSuccess(kTestValidPictureURL);
  ASSERT_EQ(kTestValidPictureURL, profile_downloader_->GetProfilePictureURL());
}

// Regression test for http://crbug.com/854907
TEST_F(ProfileDownloaderTest, AccountInfoNoPictureDoesNotCrash) {
  std::string account_id =
      account_tracker_service_->SeedAccountInfo(kTestGaia, kTestEmail);
  SimulateUserInfoSuccess(AccountTrackerService::kNoPictureURLFound);

  profile_downloader_->StartForAccount(account_id);
  profile_downloader_->StartFetchingImage();

  EXPECT_TRUE(profile_downloader_->GetProfilePictureURL().empty());
  ASSERT_EQ(ProfileDownloader::PICTURE_DEFAULT,
            profile_downloader_->GetProfilePictureStatus());
}

// Regression test for http://crbug.com/854907
TEST_F(ProfileDownloaderTest, AccountInfoInvalidPictureURLDoesNotCrash) {
  std::string account_id =
      account_tracker_service_->SeedAccountInfo(kTestGaia, kTestEmail);
  SimulateUserInfoSuccess(kTestInvalidPictureURL);

  profile_downloader_->StartForAccount(account_id);
  profile_downloader_->StartFetchingImage();

  EXPECT_TRUE(profile_downloader_->GetProfilePictureURL().empty());
  ASSERT_EQ(ProfileDownloader::PICTURE_FAILED,
            profile_downloader_->GetProfilePictureStatus());
}
