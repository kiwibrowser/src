// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/ash/wallpaper_controller_client.h"

#include "ash/public/interfaces/constants.mojom.h"
#include "base/path_service.h"
#include "base/sha1.h"
#include "base/strings/string_number_conversions.h"
#include "chrome/browser/chrome_notification_types.h"
#include "chrome/browser/chromeos/customization/customization_wallpaper_util.h"
#include "chrome/browser/chromeos/login/startup_utils.h"
#include "chrome/browser/chromeos/login/wizard_controller.h"
#include "chrome/browser/chromeos/policy/device_local_account.h"
#include "chrome/browser/extensions/extension_service.h"
#include "chrome/browser/profiles/profile_manager.h"
#include "chrome/browser/ui/extensions/app_launch_params.h"
#include "chrome/browser/ui/extensions/application_launch.h"
#include "chrome/common/chrome_paths.h"
#include "chrome/common/extensions/extension_constants.h"
#include "chromeos/chromeos_switches.h"
#include "chromeos/cryptohome/system_salt_getter.h"
#include "components/user_manager/known_user.h"
#include "components/user_manager/user_manager.h"
#include "content/public/browser/notification_service.h"
#include "content/public/common/service_manager_connection.h"
#include "extensions/browser/extension_system.h"
#include "extensions/common/constants.h"
#include "services/service_manager/public/cpp/connector.h"

namespace {

// Known user keys.
const char kWallpaperFilesId[] = "wallpaper-files-id";

WallpaperControllerClient* g_wallpaper_controller_client_instance = nullptr;

// Creates a mojom::WallpaperUserInfo for the account id. Returns nullptr if
// user manager cannot find the user.
ash::mojom::WallpaperUserInfoPtr AccountIdToWallpaperUserInfo(
    const AccountId& account_id) {
  if (!account_id.is_valid()) {
    // |account_id| may be invalid in tests.
    return nullptr;
  }
  const user_manager::User* user =
      user_manager::UserManager::Get()->FindUser(account_id);
  if (!user)
    return nullptr;

  ash::mojom::WallpaperUserInfoPtr wallpaper_user_info =
      ash::mojom::WallpaperUserInfo::New();
  wallpaper_user_info->account_id = account_id;
  wallpaper_user_info->type = user->GetType();
  wallpaper_user_info->is_ephemeral =
      user_manager::UserManager::Get()->IsUserNonCryptohomeDataEphemeral(
          account_id);
  wallpaper_user_info->has_gaia_account = user->HasGaiaAccount();

  return wallpaper_user_info;
}

// This has once been copied from
// brillo::cryptohome::home::SanitizeUserName(username) to be used for
// wallpaper identification purpose only.
//
// Historic note: We need some way to identify users wallpaper files in
// the device filesystem. Historically User::username_hash() was used for this
// purpose, but it has two caveats:
// 1. username_hash() is defined only after user has logged in.
// 2. If cryptohome identifier changes, username_hash() will also change,
//    and we may lose user => wallpaper files mapping at that point.
// So this function gives WallpaperManager independent hashing method to break
// this dependency.
std::string HashWallpaperFilesIdStr(const std::string& files_id_unhashed) {
  chromeos::SystemSaltGetter* salt_getter = chromeos::SystemSaltGetter::Get();
  DCHECK(salt_getter);

  // System salt must be defined at this point.
  const chromeos::SystemSaltGetter::RawSalt* salt = salt_getter->GetRawSalt();
  if (!salt)
    LOG(FATAL) << "WallpaperManager HashWallpaperFilesIdStr(): no salt!";

  unsigned char binmd[base::kSHA1Length];
  std::string lowercase(files_id_unhashed);
  std::transform(lowercase.begin(), lowercase.end(), lowercase.begin(),
                 ::tolower);
  std::vector<uint8_t> data = *salt;
  std::copy(files_id_unhashed.begin(), files_id_unhashed.end(),
            std::back_inserter(data));
  base::SHA1HashBytes(data.data(), data.size(), binmd);
  std::string result = base::HexEncode(binmd, sizeof(binmd));
  std::transform(result.begin(), result.end(), result.begin(), ::tolower);
  return result;
}

// Returns true if wallpaper files id can be returned successfully.
bool CanGetFilesId() {
  return chromeos::SystemSaltGetter::IsInitialized() &&
         chromeos::SystemSaltGetter::Get()->GetRawSalt();
}

// Calls |callback| when system salt is ready. (|CanGetFilesId| returns true.)
void AddCanGetFilesIdCallback(const base::Closure& callback) {
  // System salt may not be initialized in tests.
  if (chromeos::SystemSaltGetter::IsInitialized())
    chromeos::SystemSaltGetter::Get()->AddOnSystemSaltReady(callback);
}

// Returns true if |users| contains users other than device local accounts.
bool HasNonDeviceLocalAccounts(const user_manager::UserList& users) {
  for (const user_manager::User* user : users) {
    if (!policy::IsDeviceLocalAccountUser(user->GetAccountId().GetUserEmail(),
                                          nullptr))
      return true;
  }
  return false;
}

// Returns the first public session user found in |users|, or null if there's
// none.
user_manager::User* FindPublicSession(const user_manager::UserList& users) {
  for (size_t i = 0; i < users.size(); ++i) {
    if (users[i]->GetType() == user_manager::USER_TYPE_PUBLIC_ACCOUNT)
      return users[i];
  }
  return nullptr;
}

}  // namespace

WallpaperControllerClient::WallpaperControllerClient()
    : policy_handler_(this), binding_(this), weak_factory_(this) {
  DCHECK(!g_wallpaper_controller_client_instance);
  g_wallpaper_controller_client_instance = this;
}

WallpaperControllerClient::~WallpaperControllerClient() {
  weak_factory_.InvalidateWeakPtrs();
  DCHECK_EQ(this, g_wallpaper_controller_client_instance);
  g_wallpaper_controller_client_instance = nullptr;
}

void WallpaperControllerClient::Init() {
  content::ServiceManagerConnection::GetForProcess()
      ->GetConnector()
      ->BindInterface(ash::mojom::kServiceName, &wallpaper_controller_);
  BindAndSetClient();
}

void WallpaperControllerClient::InitForTesting(
    ash::mojom::WallpaperControllerPtr controller) {
  wallpaper_controller_ = std::move(controller);
  BindAndSetClient();
}

// static
WallpaperControllerClient* WallpaperControllerClient::Get() {
  return g_wallpaper_controller_client_instance;
}

std::string WallpaperControllerClient::GetFilesId(
    const AccountId& account_id) const {
  DCHECK(CanGetFilesId());
  std::string stored_value;
  if (user_manager::known_user::GetStringPref(account_id, kWallpaperFilesId,
                                              &stored_value)) {
    return stored_value;
  }

  const std::string wallpaper_files_id =
      HashWallpaperFilesIdStr(account_id.GetUserEmail());
  user_manager::known_user::SetStringPref(account_id, kWallpaperFilesId,
                                          wallpaper_files_id);
  return wallpaper_files_id;
}

void WallpaperControllerClient::SetCustomWallpaper(
    const AccountId& account_id,
    const std::string& wallpaper_files_id,
    const std::string& file_name,
    ash::WallpaperLayout layout,
    const gfx::ImageSkia& image,
    bool preview_mode) {
  ash::mojom::WallpaperUserInfoPtr user_info =
      AccountIdToWallpaperUserInfo(account_id);
  if (!user_info)
    return;
  wallpaper_controller_->SetCustomWallpaper(std::move(user_info),
                                            wallpaper_files_id, file_name,
                                            layout, image, preview_mode);
}

void WallpaperControllerClient::SetOnlineWallpaperIfExists(
    const AccountId& account_id,
    const std::string& url,
    ash::WallpaperLayout layout,
    bool preview_mode,
    ash::mojom::WallpaperController::SetOnlineWallpaperIfExistsCallback
        callback) {
  ash::mojom::WallpaperUserInfoPtr user_info =
      AccountIdToWallpaperUserInfo(account_id);
  if (!user_info)
    return;
  wallpaper_controller_->SetOnlineWallpaperIfExists(
      std::move(user_info), url, layout, preview_mode, std::move(callback));
}

void WallpaperControllerClient::SetOnlineWallpaperFromData(
    const AccountId& account_id,
    const std::string& image_data,
    const std::string& url,
    ash::WallpaperLayout layout,
    bool preview_mode) {
  ash::mojom::WallpaperUserInfoPtr user_info =
      AccountIdToWallpaperUserInfo(account_id);
  if (!user_info)
    return;
  wallpaper_controller_->SetOnlineWallpaperFromData(
      std::move(user_info), image_data, url, layout, preview_mode);
}

void WallpaperControllerClient::SetDefaultWallpaper(const AccountId& account_id,
                                                    bool show_wallpaper) {
  ash::mojom::WallpaperUserInfoPtr user_info =
      AccountIdToWallpaperUserInfo(account_id);
  if (!user_info)
    return;

  // Postpone setting the wallpaper until we can get files id.
  if (!CanGetFilesId()) {
    LOG(WARNING)
        << "Cannot get wallpaper files id in SetDefaultWallpaper. This "
           "should never happen under normal circumstances.";
    AddCanGetFilesIdCallback(
        base::Bind(&WallpaperControllerClient::SetDefaultWallpaper,
                   weak_factory_.GetWeakPtr(), account_id, show_wallpaper));
    return;
  }

  wallpaper_controller_->SetDefaultWallpaper(
      std::move(user_info), GetFilesId(account_id), show_wallpaper);
}

void WallpaperControllerClient::SetCustomizedDefaultWallpaperPaths(
    const base::FilePath& customized_default_small_path,
    const base::FilePath& customized_default_large_path) {
  wallpaper_controller_->SetCustomizedDefaultWallpaperPaths(
      customized_default_small_path, customized_default_large_path);
}

void WallpaperControllerClient::SetPolicyWallpaper(
    const AccountId& account_id,
    std::unique_ptr<std::string> data) {
  if (!data)
    return;

  ash::mojom::WallpaperUserInfoPtr user_info =
      AccountIdToWallpaperUserInfo(account_id);
  if (!user_info)
    return;

  // Postpone setting the wallpaper until we can get files id. See
  // https://crbug.com/615239.
  if (!CanGetFilesId()) {
    AddCanGetFilesIdCallback(base::Bind(
        &WallpaperControllerClient::SetPolicyWallpaper,
        weak_factory_.GetWeakPtr(), account_id, base::Passed(std::move(data))));
    return;
  }

  wallpaper_controller_->SetPolicyWallpaper(std::move(user_info),
                                            GetFilesId(account_id), *data);
}

void WallpaperControllerClient::SetThirdPartyWallpaper(
    const AccountId& account_id,
    const std::string& wallpaper_files_id,
    const std::string& file_name,
    ash::WallpaperLayout layout,
    const gfx::ImageSkia& image,
    ash::mojom::WallpaperController::SetThirdPartyWallpaperCallback callback) {
  ash::mojom::WallpaperUserInfoPtr user_info =
      AccountIdToWallpaperUserInfo(account_id);
  if (!user_info)
    return;
  wallpaper_controller_->SetThirdPartyWallpaper(
      std::move(user_info), wallpaper_files_id, file_name, layout, image,
      std::move(callback));
}

void WallpaperControllerClient::ConfirmPreviewWallpaper() {
  wallpaper_controller_->ConfirmPreviewWallpaper();
}

void WallpaperControllerClient::CancelPreviewWallpaper() {
  wallpaper_controller_->CancelPreviewWallpaper();
}

void WallpaperControllerClient::UpdateCustomWallpaperLayout(
    const AccountId& account_id,
    ash::WallpaperLayout layout) {
  ash::mojom::WallpaperUserInfoPtr user_info =
      AccountIdToWallpaperUserInfo(account_id);
  if (!user_info)
    return;
  wallpaper_controller_->UpdateCustomWallpaperLayout(std::move(user_info),
                                                     layout);
}

void WallpaperControllerClient::ShowUserWallpaper(const AccountId& account_id) {
  ash::mojom::WallpaperUserInfoPtr user_info =
      AccountIdToWallpaperUserInfo(account_id);
  if (!user_info)
    return;
  wallpaper_controller_->ShowUserWallpaper(std::move(user_info));
}

void WallpaperControllerClient::ShowSigninWallpaper() {
  wallpaper_controller_->ShowSigninWallpaper();
}

void WallpaperControllerClient::RemoveUserWallpaper(
    const AccountId& account_id) {
  ash::mojom::WallpaperUserInfoPtr user_info =
      AccountIdToWallpaperUserInfo(account_id);
  if (!user_info)
    return;

  // Postpone removing the wallpaper until we can get files id.
  if (!CanGetFilesId()) {
    LOG(WARNING)
        << "Cannot get wallpaper files id in RemoveUserWallpaper. This "
           "should never happen under normal circumstances.";
    AddCanGetFilesIdCallback(
        base::Bind(&WallpaperControllerClient::RemoveUserWallpaper,
                   weak_factory_.GetWeakPtr(), account_id));
    return;
  }

  wallpaper_controller_->RemoveUserWallpaper(std::move(user_info),
                                             GetFilesId(account_id));
}

void WallpaperControllerClient::RemovePolicyWallpaper(
    const AccountId& account_id) {
  ash::mojom::WallpaperUserInfoPtr user_info =
      AccountIdToWallpaperUserInfo(account_id);
  if (!user_info)
    return;

  // Postpone removing the wallpaper until we can get files id.
  if (!CanGetFilesId()) {
    LOG(WARNING)
        << "Cannot get wallpaper files id in RemovePolicyWallpaper. This "
           "should never happen under normal circumstances.";
    AddCanGetFilesIdCallback(
        base::Bind(&WallpaperControllerClient::RemovePolicyWallpaper,
                   weak_factory_.GetWeakPtr(), account_id));
    return;
  }

  wallpaper_controller_->RemovePolicyWallpaper(std::move(user_info),
                                               GetFilesId(account_id));
}

void WallpaperControllerClient::GetOfflineWallpaperList(
    ash::mojom::WallpaperController::GetOfflineWallpaperListCallback callback) {
  wallpaper_controller_->GetOfflineWallpaperList(std::move(callback));
}

void WallpaperControllerClient::SetAnimationDuration(
    const base::TimeDelta& animation_duration) {
  wallpaper_controller_->SetAnimationDuration(animation_duration);
}

void WallpaperControllerClient::OpenWallpaperPickerIfAllowed() {
  wallpaper_controller_->OpenWallpaperPickerIfAllowed();
}

void WallpaperControllerClient::MinimizeInactiveWindows(
    const std::string& user_id_hash) {
  wallpaper_controller_->MinimizeInactiveWindows(user_id_hash);
}

void WallpaperControllerClient::RestoreMinimizedWindows(
    const std::string& user_id_hash) {
  wallpaper_controller_->RestoreMinimizedWindows(user_id_hash);
}

void WallpaperControllerClient::AddObserver(
    ash::mojom::WallpaperObserverAssociatedPtrInfo observer) {
  wallpaper_controller_->AddObserver(std::move(observer));
}

void WallpaperControllerClient::GetWallpaperImage(
    ash::mojom::WallpaperController::GetWallpaperImageCallback callback) {
  wallpaper_controller_->GetWallpaperImage(std::move(callback));
}

void WallpaperControllerClient::GetWallpaperColors(
    ash::mojom::WallpaperController::GetWallpaperColorsCallback callback) {
  wallpaper_controller_->GetWallpaperColors(std::move(callback));
}

void WallpaperControllerClient::IsWallpaperBlurred(
    ash::mojom::WallpaperController::IsWallpaperBlurredCallback callback) {
  wallpaper_controller_->IsWallpaperBlurred(std::move(callback));
}

void WallpaperControllerClient::IsActiveUserWallpaperControlledByPolicy(
    ash::mojom::WallpaperController::
        IsActiveUserWallpaperControlledByPolicyCallback callback) {
  wallpaper_controller_->IsActiveUserWallpaperControlledByPolicy(
      std::move(callback));
}

void WallpaperControllerClient::GetActiveUserWallpaperLocation(
    ash::mojom::WallpaperController::GetActiveUserWallpaperLocationCallback
        callback) {
  wallpaper_controller_->GetActiveUserWallpaperLocation(std::move(callback));
}

void WallpaperControllerClient::ShouldShowWallpaperSetting(
    ash::mojom::WallpaperController::ShouldShowWallpaperSettingCallback
        callback) {
  wallpaper_controller_->ShouldShowWallpaperSetting(std::move(callback));
}

void WallpaperControllerClient::OnDeviceWallpaperChanged() {
  wallpaper_controller_->SetDeviceWallpaperPolicyEnforced(true /*enforced=*/);
}

void WallpaperControllerClient::OnDeviceWallpaperPolicyCleared() {
  wallpaper_controller_->SetDeviceWallpaperPolicyEnforced(false /*enforced=*/);
}

void WallpaperControllerClient::OnShowUserNamesOnLoginPolicyChanged() {
  UpdateRegisteredDeviceWallpaper();
}

void WallpaperControllerClient::FlushForTesting() {
  wallpaper_controller_.FlushForTesting();
}

void WallpaperControllerClient::BindAndSetClient() {
  ash::mojom::WallpaperControllerClientPtr client;
  binding_.Bind(mojo::MakeRequest(&client));

  // Get the paths of wallpaper directories.
  base::FilePath user_data_path;
  CHECK(base::PathService::Get(chrome::DIR_USER_DATA, &user_data_path));
  base::FilePath chromeos_wallpapers_path;
  CHECK(base::PathService::Get(chrome::DIR_CHROMEOS_WALLPAPERS,
                               &chromeos_wallpapers_path));
  base::FilePath chromeos_custom_wallpapers_path;
  CHECK(base::PathService::Get(chrome::DIR_CHROMEOS_CUSTOM_WALLPAPERS,
                               &chromeos_custom_wallpapers_path));

  wallpaper_controller_->Init(
      std::move(client), user_data_path, chromeos_wallpapers_path,
      chromeos_custom_wallpapers_path,
      policy_handler_.device_wallpaper_file_path(),
      policy_handler_.IsDeviceWallpaperPolicyEnforced());
}

void WallpaperControllerClient::UpdateRegisteredDeviceWallpaper() {
  if (user_manager::UserManager::Get()->IsUserLoggedIn())
    return;

  const user_manager::UserList& users =
      user_manager::UserManager::Get()->GetUsers();
  user_manager::User* public_session = FindPublicSession(users);

  // Show the default signin wallpaper if there's no user to display.
  if ((!policy_handler_.ShouldShowUserNamesOnLogin() && !public_session) ||
      !HasNonDeviceLocalAccounts(users)) {
    ShowSigninWallpaper();
    return;
  }

  // Normal boot, load user wallpaper.
  const AccountId account_id = public_session ? public_session->GetAccountId()
                                              : users[0]->GetAccountId();
  ShowUserWallpaper(account_id);
}

void WallpaperControllerClient::OpenWallpaperPicker() {
  Profile* profile = ProfileManager::GetActiveUserProfile();
  DCHECK(profile);
  ExtensionService* service =
      extensions::ExtensionSystem::Get(profile)->extension_service();
  if (!service)
    return;

  const extensions::Extension* extension = service->GetExtensionById(
      extension_misc::kWallpaperManagerId, false /*include_disabled=*/);
  if (!extension)
    return;

  OpenApplication(AppLaunchParams(
      profile, extension, extensions::LAUNCH_CONTAINER_WINDOW,
      WindowOpenDisposition::NEW_WINDOW, extensions::SOURCE_CHROME_INTERNAL));
}

void WallpaperControllerClient::OnReadyToSetWallpaper() {
  // TODO(wzang|784495): Consider deprecating this method after views-based
  // login is enabled. It should be fast enough to request the first wallpaper
  // so that there's no visible delay. In other scenarios such as restart after
  // crash, user manager should request the wallpaper.

  // Apply device customization.
  namespace util = chromeos::customization_wallpaper_util;
  if (util::ShouldUseCustomizedDefaultWallpaper()) {
    base::FilePath customized_default_small_path;
    base::FilePath customized_default_large_path;
    if (util::GetCustomizedDefaultWallpaperPaths(
            &customized_default_small_path, &customized_default_large_path)) {
      wallpaper_controller_->SetCustomizedDefaultWallpaperPaths(
          customized_default_small_path, customized_default_large_path);
    }
  }

  // Guest wallpaper should be initialized when guest logs in.
  if (base::CommandLine::ForCurrentProcess()->HasSwitch(
          chromeos::switches::kGuestSession)) {
    return;
  }

  // Do not set wallpaper in tests.
  if (chromeos::WizardController::IsZeroDelayEnabled())
    return;

  // Show the wallpaper of the active user during an user session.
  if (user_manager::UserManager::Get()->IsUserLoggedIn()) {
    ShowUserWallpaper(
        user_manager::UserManager::Get()->GetActiveUser()->GetAccountId());
    return;
  }

  // If the device is not registered yet (e.g. during OOBE), show the default
  // signin wallpaper.
  if (!chromeos::StartupUtils::IsDeviceRegistered()) {
    ShowSigninWallpaper();
    return;
  }

  UpdateRegisteredDeviceWallpaper();
}

void WallpaperControllerClient::OnFirstWallpaperAnimationFinished() {
  content::NotificationService::current()->Notify(
      chrome::NOTIFICATION_WALLPAPER_ANIMATION_FINISHED,
      content::NotificationService::AllSources(),
      content::NotificationService::NoDetails());
}
