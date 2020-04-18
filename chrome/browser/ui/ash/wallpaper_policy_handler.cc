// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/ash/wallpaper_policy_handler.h"

#include "base/bind.h"
#include "base/files/file_util.h"
#include "base/path_service.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_util.h"
#include "base/task_scheduler/post_task.h"
#include "base/values.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/chromeos/customization/customization_wallpaper_downloader.h"
#include "chrome/browser/chromeos/policy/browser_policy_connector_chromeos.h"
#include "chrome/common/chrome_paths.h"
#include "chromeos/settings/cros_settings_names.h"
#include "crypto/sha2.h"
#include "url/gurl.h"

using chromeos::CrosSettings;

namespace {

// The directory and file name to save the downloaded device policy wallpaper.
constexpr char kDeviceWallpaperDir[] = "device_wallpaper";
constexpr char kDeviceWallpaperFile[] = "device_wallpaper_image.jpg";

// A helper function to check the existing/downloaded device wallpaper file's
// hash value matches with the hash value provided in the policy settings.
bool CheckWallpaperFileMatchHash(const base::FilePath& device_wallpaper_file,
                                 const std::string& hash) {
  std::string image_data;
  if (base::ReadFileToString(device_wallpaper_file, &image_data)) {
    std::string sha_hash = crypto::SHA256HashString(image_data);
    if (base::ToLowerASCII(base::HexEncode(
            sha_hash.c_str(), sha_hash.size())) == base::ToLowerASCII(hash)) {
      return true;
    }
  }
  return false;
}

}  // namespace

WallpaperPolicyHandler::WallpaperPolicyHandler(Delegate* delegate)
    : delegate_(delegate), weak_factory_(this) {
  DCHECK(delegate_);
  device_wallpaper_image_subscription_ =
      CrosSettings::Get()->AddSettingsObserver(
          chromeos::kDeviceWallpaperImage,
          base::BindRepeating(
              &WallpaperPolicyHandler::DeviceWallpaperPolicyChanged,
              weak_factory_.GetWeakPtr()));
  show_user_names_on_signin_subscription_ =
      chromeos::CrosSettings::Get()->AddSettingsObserver(
          chromeos::kAccountsPrefShowUserNamesOnSignIn,
          base::Bind(
              &WallpaperPolicyHandler::ShowUserNamesOnSignInPolicyChanged,
              weak_factory_.GetWeakPtr()));

  // Initialize the desired file path for device policy wallpaper. The path will
  // be used by WallpaperController to access the wallpaper file.
  base::FilePath chromeos_wallpapers_path;
  CHECK(base::PathService::Get(chrome::DIR_CHROMEOS_WALLPAPERS,
                               &chromeos_wallpapers_path));
  device_wallpaper_file_path_ =
      chromeos_wallpapers_path.Append(kDeviceWallpaperDir)
          .Append(kDeviceWallpaperFile);
}

WallpaperPolicyHandler::~WallpaperPolicyHandler() {
  device_wallpaper_image_subscription_.reset();
  show_user_names_on_signin_subscription_.reset();
}

bool WallpaperPolicyHandler::IsDeviceWallpaperPolicyEnforced() {
  if (!g_browser_process->platform_part()
           ->browser_policy_connector_chromeos()
           ->IsEnterpriseManaged()) {
    return false;
  }

  std::string url, hash;
  return GetDeviceWallpaperPolicyStrings(&url, &hash);
}

bool WallpaperPolicyHandler::ShouldShowUserNamesOnLogin() {
  bool show_user_names;
  chromeos::CrosSettings::Get()->GetBoolean(
      chromeos::kAccountsPrefShowUserNamesOnSignIn, &show_user_names);
  return show_user_names;
}

bool WallpaperPolicyHandler::GetDeviceWallpaperPolicyStrings(
    std::string* url,
    std::string* hash) {
  const base::DictionaryValue* dict = nullptr;
  if (!CrosSettings::Get()->GetDictionary(chromeos::kDeviceWallpaperImage,
                                          &dict) ||
      !dict->GetStringWithoutPathExpansion("url", url) ||
      !dict->GetStringWithoutPathExpansion("hash", hash)) {
    return false;
  }
  return true;
}

void WallpaperPolicyHandler::DeviceWallpaperPolicyChanged() {
  // First check if the device policy was cleared.
  const base::DictionaryValue* dict = nullptr;
  if (!CrosSettings::Get()->GetDictionary(chromeos::kDeviceWallpaperImage,
                                          &dict)) {
    // In this case delete the local device wallpaper file if it exists and
    // inform its delegate.
    base::PostTaskWithTraits(
        FROM_HERE,
        {base::MayBlock(), base::TaskPriority::BACKGROUND,
         base::TaskShutdownBehavior::CONTINUE_ON_SHUTDOWN},
        base::BindOnce(base::IgnoreResult(&base::DeleteFile),
                       device_wallpaper_file_path_, false /*=recursive*/));
    delegate_->OnDeviceWallpaperPolicyCleared();
    return;
  }

  // Check if the device wallpaper already exists and matches the new provided
  // hash. Fetch the wallpaper from the provided url if it doesn't exist.
  base::PostTaskWithTraitsAndReplyWithResult(
      FROM_HERE, {base::MayBlock()},
      base::BindOnce(&base::PathExists, device_wallpaper_file_path_),
      base::BindOnce(&WallpaperPolicyHandler::OnDeviceWallpaperFileExists,
                     weak_factory_.GetWeakPtr()));
}

void WallpaperPolicyHandler::ShowUserNamesOnSignInPolicyChanged() {
  delegate_->OnShowUserNamesOnLoginPolicyChanged();
}

void WallpaperPolicyHandler::OnDeviceWallpaperFileExists(bool exists) {
  std::string url, hash;
  if (!GetDeviceWallpaperPolicyStrings(&url, &hash)) {
    // Do nothing if the strings can not be retrieved.
    return;
  }

  if (exists) {
    base::PostTaskWithTraitsAndReplyWithResult(
        FROM_HERE, {base::MayBlock()},
        base::BindOnce(&CheckWallpaperFileMatchHash,
                       device_wallpaper_file_path_, hash),
        base::BindOnce(
            &WallpaperPolicyHandler::OnCheckExistingDeviceWallpaperMatchHash,
            weak_factory_.GetWeakPtr(), url, hash));
  } else {
    GURL wallpaper_url(url);
    device_wallpaper_downloader_.reset(
        new chromeos::CustomizationWallpaperDownloader(
            wallpaper_url, device_wallpaper_file_path_.DirName(),
            device_wallpaper_file_path_,
            base::BindRepeating(
                &WallpaperPolicyHandler::OnDeviceWallpaperDownloaded,
                weak_factory_.GetWeakPtr(), hash)));
    device_wallpaper_downloader_->Start();
  }
}

void WallpaperPolicyHandler::OnCheckExistingDeviceWallpaperMatchHash(
    const std::string& url,
    const std::string& hash,
    bool match) {
  if (match) {
    // Notify its delegate that device wallpaper is ready.
    delegate_->OnDeviceWallpaperChanged();
    return;
  }

  GURL wallpaper_url(url);
  device_wallpaper_downloader_.reset(
      new chromeos::CustomizationWallpaperDownloader(
          wallpaper_url, device_wallpaper_file_path_.DirName(),
          device_wallpaper_file_path_,
          base::BindRepeating(
              &WallpaperPolicyHandler::OnDeviceWallpaperDownloaded,
              weak_factory_.GetWeakPtr(), hash)));
  device_wallpaper_downloader_->Start();
}

void WallpaperPolicyHandler::OnDeviceWallpaperDownloaded(
    const std::string& hash,
    bool success,
    const GURL& url) {
  // Do nothing if device wallpaper download failed.
  if (!success)
    return;

  base::PostTaskWithTraitsAndReplyWithResult(
      FROM_HERE, {base::MayBlock()},
      base::BindOnce(&CheckWallpaperFileMatchHash, device_wallpaper_file_path_,
                     hash),
      base::BindOnce(
          &WallpaperPolicyHandler::OnCheckFetchedDeviceWallpaperMatchHash,
          weak_factory_.GetWeakPtr()));
}

void WallpaperPolicyHandler::OnCheckFetchedDeviceWallpaperMatchHash(
    bool match) {
  // Do nothing if the provided hash doesn't match with the downloaded image.
  if (!match)
    return;

  // Notify its delegate that device wallpaper is ready.
  delegate_->OnDeviceWallpaperChanged();
}
