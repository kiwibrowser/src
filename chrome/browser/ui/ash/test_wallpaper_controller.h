// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_ASH_TEST_WALLPAPER_CONTROLLER_H_
#define CHROME_BROWSER_UI_ASH_TEST_WALLPAPER_CONTROLLER_H_

#include "ash/public/interfaces/wallpaper.mojom.h"
#include "base/macros.h"
#include "mojo/public/cpp/bindings/binding.h"
#include "mojo/public/cpp/bindings/interface_ptr_set.h"

// Simulates WallpaperController in ash.
class TestWallpaperController : ash::mojom::WallpaperController {
 public:
  TestWallpaperController();

  ~TestWallpaperController() override;

  // Simulates showing the wallpaper on screen by updating |current_wallpaper|
  // and notifying the observers.
  void ShowWallpaperImage(const gfx::ImageSkia& image);

  void ClearCounts();
  bool was_client_set() const { return was_client_set_; }
  int remove_user_wallpaper_count() const {
    return remove_user_wallpaper_count_;
  }
  int set_default_wallpaper_count() const {
    return set_default_wallpaper_count_;
  }
  int set_custom_wallpaper_count() const { return set_custom_wallpaper_count_; }

  // Returns a mojo interface pointer bound to this object.
  ash::mojom::WallpaperControllerPtr CreateInterfacePtr();

  // ash::mojom::WallpaperController:
  void Init(ash::mojom::WallpaperControllerClientPtr client,
            const base::FilePath& user_data_path,
            const base::FilePath& chromeos_wallpapers_path,
            const base::FilePath& chromeos_custom_wallpapers_path,
            const base::FilePath& device_policy_wallpaper_path,
            bool is_device_wallpaper_policy_enforced) override;
  void SetCustomWallpaper(ash::mojom::WallpaperUserInfoPtr user_info,
                          const std::string& wallpaper_files_id,
                          const std::string& file_name,
                          ash::WallpaperLayout layout,
                          const gfx::ImageSkia& image,
                          bool preview_mode) override;
  void SetOnlineWallpaperIfExists(
      ash::mojom::WallpaperUserInfoPtr user_info,
      const std::string& url,
      ash::WallpaperLayout layout,
      bool preview_mode,
      ash::mojom::WallpaperController::SetOnlineWallpaperIfExistsCallback
          callback) override;
  void SetOnlineWallpaperFromData(ash::mojom::WallpaperUserInfoPtr user_info,
                                  const std::string& image_data,
                                  const std::string& url,
                                  ash::WallpaperLayout layout,
                                  bool preview_mode) override;
  void SetDefaultWallpaper(ash::mojom::WallpaperUserInfoPtr user_info,
                           const std::string& wallpaper_files_id,
                           bool show_wallpaper) override;
  void SetCustomizedDefaultWallpaperPaths(
      const base::FilePath& customized_default_small_path,
      const base::FilePath& customized_default_large_path) override;
  void SetPolicyWallpaper(ash::mojom::WallpaperUserInfoPtr user_info,
                          const std::string& wallpaper_files_id,
                          const std::string& data) override;
  void SetDeviceWallpaperPolicyEnforced(bool enforced) override;
  void SetThirdPartyWallpaper(
      ash::mojom::WallpaperUserInfoPtr user_info,
      const std::string& wallpaper_files_id,
      const std::string& file_name,
      ash::WallpaperLayout layout,
      const gfx::ImageSkia& image,
      ash::mojom::WallpaperController::SetThirdPartyWallpaperCallback callback)
      override;
  void ConfirmPreviewWallpaper() override;
  void CancelPreviewWallpaper() override;
  void UpdateCustomWallpaperLayout(ash::mojom::WallpaperUserInfoPtr user_info,
                                   ash::WallpaperLayout layout) override;
  void ShowUserWallpaper(ash::mojom::WallpaperUserInfoPtr user_info) override;
  void ShowSigninWallpaper() override;
  void RemoveUserWallpaper(ash::mojom::WallpaperUserInfoPtr user_info,
                           const std::string& wallpaper_files_id) override;
  void RemovePolicyWallpaper(ash::mojom::WallpaperUserInfoPtr user_info,
                             const std::string& wallpaper_files_id) override;
  void GetOfflineWallpaperList(
      ash::mojom::WallpaperController::GetOfflineWallpaperListCallback callback)
      override;
  void SetAnimationDuration(base::TimeDelta animation_duration) override;
  void OpenWallpaperPickerIfAllowed() override;
  void MinimizeInactiveWindows(const std::string& user_id_hash) override;
  void RestoreMinimizedWindows(const std::string& user_id_hash) override;
  void AddObserver(
      ash::mojom::WallpaperObserverAssociatedPtrInfo observer) override;
  void GetWallpaperImage(
      ash::mojom::WallpaperController::GetWallpaperImageCallback callback)
      override;
  void GetWallpaperColors(
      ash::mojom::WallpaperController::GetWallpaperColorsCallback callback)
      override;
  void IsWallpaperBlurred(
      ash::mojom::WallpaperController::IsWallpaperBlurredCallback callback)
      override;
  void IsActiveUserWallpaperControlledByPolicy(
      ash::mojom::WallpaperController::
          IsActiveUserWallpaperControlledByPolicyCallback callback) override;
  void GetActiveUserWallpaperLocation(
      ash::mojom::WallpaperController::GetActiveUserWallpaperLocationCallback
          callback) override;
  void ShouldShowWallpaperSetting(
      ash::mojom::WallpaperController::ShouldShowWallpaperSettingCallback
          callback) override;

 private:
  mojo::Binding<ash::mojom::WallpaperController> binding_;

  bool was_client_set_ = false;
  int remove_user_wallpaper_count_ = 0;
  int set_default_wallpaper_count_ = 0;
  int set_custom_wallpaper_count_ = 0;

  mojo::AssociatedInterfacePtrSet<ash::mojom::WallpaperObserver>
      test_observers_;

  gfx::ImageSkia current_wallpaper;

  DISALLOW_COPY_AND_ASSIGN(TestWallpaperController);
};

#endif  // CHROME_BROWSER_UI_ASH_TEST_WALLPAPER_CONTROLLER_H_
