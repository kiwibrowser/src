// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_ASH_WALLPAPER_CONTROLLER_CLIENT_H_
#define CHROME_BROWSER_UI_ASH_WALLPAPER_CONTROLLER_CLIENT_H_

#include "ash/public/cpp/wallpaper_types.h"
#include "ash/public/interfaces/wallpaper.mojom.h"
#include "base/macros.h"
#include "chrome/browser/ui/ash/wallpaper_policy_handler.h"
#include "mojo/public/cpp/bindings/binding.h"

// Handles method calls sent from ash to chrome. Also sends messages from chrome
// to ash.
class WallpaperControllerClient : public ash::mojom::WallpaperControllerClient,
                                  public WallpaperPolicyHandler::Delegate {
 public:
  WallpaperControllerClient();
  ~WallpaperControllerClient() override;

  // Initializes and connects to ash.
  void Init();

  // Tests can provide a mock mojo interface for the ash controller.
  void InitForTesting(ash::mojom::WallpaperControllerPtr controller);

  static WallpaperControllerClient* Get();

  // Returns files identifier for the |account_id|.
  std::string GetFilesId(const AccountId& account_id) const;

  // Wrappers around the ash::mojom::WallpaperController interface.
  void SetCustomWallpaper(const AccountId& account_id,
                          const std::string& wallpaper_files_id,
                          const std::string& file_name,
                          ash::WallpaperLayout layout,
                          const gfx::ImageSkia& image,
                          bool preview_mode);
  void SetOnlineWallpaperIfExists(
      const AccountId& account_id,
      const std::string& url,
      ash::WallpaperLayout layout,
      bool preview_mode,
      ash::mojom::WallpaperController::SetOnlineWallpaperIfExistsCallback
          callback);
  void SetOnlineWallpaperFromData(const AccountId& account_id,
                                  const std::string& image_data,
                                  const std::string& url,
                                  ash::WallpaperLayout layout,
                                  bool preview_mode);
  void SetDefaultWallpaper(const AccountId& account_id, bool show_wallpaper);
  void SetCustomizedDefaultWallpaperPaths(
      const base::FilePath& customized_default_small_path,
      const base::FilePath& customized_default_large_path);
  void SetPolicyWallpaper(const AccountId& account_id,
                          std::unique_ptr<std::string> data);
  void SetThirdPartyWallpaper(
      const AccountId& account_id,
      const std::string& wallpaper_files_id,
      const std::string& file_name,
      ash::WallpaperLayout layout,
      const gfx::ImageSkia& image,
      ash::mojom::WallpaperController::SetThirdPartyWallpaperCallback callback);
  void ConfirmPreviewWallpaper();
  void CancelPreviewWallpaper();
  void UpdateCustomWallpaperLayout(const AccountId& account_id,
                                   ash::WallpaperLayout layout);
  void ShowUserWallpaper(const AccountId& account_id);
  void ShowSigninWallpaper();
  void RemoveUserWallpaper(const AccountId& account_id);
  void RemovePolicyWallpaper(const AccountId& account_id);
  void GetOfflineWallpaperList(
      ash::mojom::WallpaperController::GetOfflineWallpaperListCallback
          callback);
  void SetAnimationDuration(const base::TimeDelta& animation_duration);
  void OpenWallpaperPickerIfAllowed();
  void MinimizeInactiveWindows(const std::string& user_id_hash);
  void RestoreMinimizedWindows(const std::string& user_id_hash);
  void AddObserver(ash::mojom::WallpaperObserverAssociatedPtrInfo observer);
  void GetWallpaperImage(
      ash::mojom::WallpaperController::GetWallpaperImageCallback callback);
  void GetWallpaperColors(
      ash::mojom::WallpaperController::GetWallpaperColorsCallback callback);
  void IsWallpaperBlurred(
      ash::mojom::WallpaperController::IsWallpaperBlurredCallback callback);
  void IsActiveUserWallpaperControlledByPolicy(
      ash::mojom::WallpaperController::
          IsActiveUserWallpaperControlledByPolicyCallback callback);
  void GetActiveUserWallpaperLocation(
      ash::mojom::WallpaperController::GetActiveUserWallpaperLocationCallback
          callback);
  void ShouldShowWallpaperSetting(
      ash::mojom::WallpaperController::ShouldShowWallpaperSettingCallback
          callback);

  // chromeos::WallpaperPolicyHandler::Delegate:
  void OnDeviceWallpaperChanged() override;
  void OnDeviceWallpaperPolicyCleared() override;
  void OnShowUserNamesOnLoginPolicyChanged() override;

  // Flushes the mojo pipe to ash.
  void FlushForTesting();

 private:
  // Binds this object to its mojo interface and sets it as the ash client.
  void BindAndSetClient();

  // Updates the wallpaper of a registered device after device policy is
  // trusted, outside an user session. Note that before device is enrolled, it
  // proceeds with untrusted setting.
  void UpdateRegisteredDeviceWallpaper();

  // ash::mojom::WallpaperControllerClient:
  void OpenWallpaperPicker() override;
  void OnReadyToSetWallpaper() override;
  void OnFirstWallpaperAnimationFinished() override;

  // WallpaperController interface in ash.
  ash::mojom::WallpaperControllerPtr wallpaper_controller_;

  WallpaperPolicyHandler policy_handler_;

  // Binds to the client interface.
  mojo::Binding<ash::mojom::WallpaperControllerClient> binding_;

  base::WeakPtrFactory<WallpaperControllerClient> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(WallpaperControllerClient);
};

#endif  // CHROME_BROWSER_UI_ASH_WALLPAPER_CONTROLLER_CLIENT_H_
