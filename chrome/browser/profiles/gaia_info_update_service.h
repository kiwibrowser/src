// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_PROFILES_GAIA_INFO_UPDATE_SERVICE_H_
#define CHROME_BROWSER_PROFILES_GAIA_INFO_UPDATE_SERVICE_H_

#include <memory>

#include "base/compiler_specific.h"
#include "base/gtest_prod_util.h"
#include "base/macros.h"
#include "base/timer/timer.h"
#include "chrome/browser/profiles/profile_downloader.h"
#include "chrome/browser/profiles/profile_downloader_delegate.h"
#include "components/keyed_service/core/keyed_service.h"
#include "components/signin/core/browser/signin_manager.h"

class Profile;
class ProfileDownloader;

// This service kicks off a download of the user's name and profile picture.
// The results are saved in the profile info cache.
class GAIAInfoUpdateService : public KeyedService,
                              public ProfileDownloaderDelegate,
                              public SigninManagerBase::Observer {
 public:
  explicit GAIAInfoUpdateService(Profile* profile);
  ~GAIAInfoUpdateService() override;

  // Updates the GAIA info for the profile associated with this instance.
  virtual void Update();

  // Checks if downloading GAIA info for the given profile is allowed.
  static bool ShouldUseGAIAProfileInfo(Profile* profile);

  // ProfileDownloaderDelegate:
  bool NeedsProfilePicture() const override;
  int GetDesiredImageSideLength() const override;
  Profile* GetBrowserProfile() override;
  std::string GetCachedPictureURL() const override;
  bool IsPreSignin() const override;
  void OnProfileDownloadSuccess(ProfileDownloader* downloader) override;
  void OnProfileDownloadFailure(
      ProfileDownloader* downloader,
      ProfileDownloaderDelegate::FailureReason reason) override;

  // Overridden from KeyedService:
  void Shutdown() override;

 private:
  FRIEND_TEST_ALL_PREFIXES(GAIAInfoUpdateServiceTest, ScheduleUpdate);

  void OnUsernameChanged(const std::string& username);
  void ScheduleNextUpdate();

  // Overridden from SigninManagerBase::Observer:
  void GoogleSigninSucceeded(const std::string& account_id,
                             const std::string& username) override;
  void GoogleSignedOut(const std::string& account_id,
                       const std::string& username) override;

  Profile* profile_;
  std::unique_ptr<ProfileDownloader> profile_image_downloader_;
  base::Time last_updated_;
  base::OneShotTimer timer_;

  DISALLOW_COPY_AND_ASSIGN(GAIAInfoUpdateService);
};

#endif  // CHROME_BROWSER_PROFILES_GAIA_INFO_UPDATE_SERVICE_H_
