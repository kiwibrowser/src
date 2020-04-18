// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_ASH_WALLPAPER_POLICY_HANDLER_H_
#define CHROME_BROWSER_UI_ASH_WALLPAPER_POLICY_HANDLER_H_

#include "ash/public/interfaces/wallpaper.mojom.h"
#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "chrome/browser/chromeos/settings/cros_settings.h"
#include "url/gurl.h"

namespace chromeos {
class CustomizationWallpaperDownloader;
}  // namespace chromeos

// This class observes changes of device policies related to wallpaper, such as
// kDeviceWallpaperImage and kAccountsPrefShowUserNamesOnSignIn.
//
// It's responsible for downloading the new device policy controlled wallpaper
// and saving it to local file system if the provided hash value matches the
// downloaded wallpaper. It then notifies its delegate about the changes. The
// delegate will decide if/when to set device policy controlled wallpaper.
class WallpaperPolicyHandler {
 public:
  // Delegate of WallpaperPolicyHandler.
  class Delegate {
   public:
    virtual void OnDeviceWallpaperChanged() = 0;
    virtual void OnDeviceWallpaperPolicyCleared() = 0;
    virtual void OnShowUserNamesOnLoginPolicyChanged() = 0;

   protected:
    virtual ~Delegate() {}
  };

  explicit WallpaperPolicyHandler(Delegate* delegate);
  ~WallpaperPolicyHandler();

  // Returns true if the device wallpaper policy is currently in effect.
  bool IsDeviceWallpaperPolicyEnforced();

  // Returns true if user names should be shown on the login screen.
  bool ShouldShowUserNamesOnLogin();

  base::FilePath device_wallpaper_file_path() {
    return device_wallpaper_file_path_;
  }

 private:
  // Gets the device policy controlled wallpaper's url and hash values. Returns
  // false if the values can't be retrieved.
  bool GetDeviceWallpaperPolicyStrings(std::string* url, std::string* hash);

  // This is called whenever the device wallpaper policy changes.
  void DeviceWallpaperPolicyChanged();

  // Called when kAccountsPrefShowUserNamesOnSignIn changes.
  void ShowUserNamesOnSignInPolicyChanged();

  void OnDeviceWallpaperFileExists(bool exists);
  void OnCheckExistingDeviceWallpaperMatchHash(const std::string& url,
                                               const std::string& hash,
                                               bool match);
  void OnDeviceWallpaperDownloaded(const std::string& hash,
                                   bool success,
                                   const GURL& url);
  void OnCheckFetchedDeviceWallpaperMatchHash(bool match);

  base::FilePath device_wallpaper_file_path_;
  Delegate* delegate_;

  std::unique_ptr<chromeos::CrosSettings::ObserverSubscription>
      device_wallpaper_image_subscription_;
  std::unique_ptr<chromeos::CustomizationWallpaperDownloader>
      device_wallpaper_downloader_;

  // Observes if user names should be shown on the login screen, which
  // determines whether a user wallpaper or a default wallpaper should be shown.
  // TODO(wzang|784495): Views-based login should observe this and send
  // different requests accordingly.
  std::unique_ptr<chromeos::CrosSettings::ObserverSubscription>
      show_user_names_on_signin_subscription_;

  base::WeakPtrFactory<WallpaperPolicyHandler> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(WallpaperPolicyHandler);
};

#endif  // CHROME_BROWSER_UI_ASH_WALLPAPER_POLICY_HANDLER_H_
