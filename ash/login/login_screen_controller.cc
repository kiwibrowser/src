// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/login/login_screen_controller.h"

#include "ash/login/ui/lock_screen.h"
#include "ash/login/ui/login_data_dispatcher.h"
#include "ash/public/cpp/ash_pref_names.h"
#include "ash/root_window_controller.h"
#include "ash/session/session_controller.h"
#include "ash/shell.h"
#include "ash/system/status_area_widget.h"
#include "base/debug/alias.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/utf_string_conversions.h"
#include "chromeos/cryptohome/system_salt_getter.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/session_manager/session_manager_types.h"

namespace ash {

namespace {

enum class SystemTrayVisibility {
  kNone,     // Tray not visible anywhere.
  kPrimary,  // Tray visible only on primary display.
  kAll,      // Tray visible on all displays.
};

void SetSystemTrayVisibility(SystemTrayVisibility visibility) {
  RootWindowController* primary_window_controller =
      Shell::GetPrimaryRootWindowController();
  for (RootWindowController* window_controller :
       Shell::GetAllRootWindowControllers()) {
    StatusAreaWidget* status_area = window_controller->GetStatusAreaWidget();
    if (!status_area)
      continue;
    if (window_controller == primary_window_controller) {
      status_area->SetSystemTrayVisibility(
          visibility == SystemTrayVisibility::kPrimary ||
          visibility == SystemTrayVisibility::kAll);
    } else {
      status_area->SetSystemTrayVisibility(visibility ==
                                           SystemTrayVisibility::kAll);
    }
  }
}

}  // namespace

LoginScreenController::LoginScreenController() : weak_factory_(this) {}

LoginScreenController::~LoginScreenController() = default;

// static
void LoginScreenController::RegisterProfilePrefs(PrefRegistrySimple* registry,
                                                 bool for_test) {
  if (for_test) {
    // There is no remote pref service, so pretend that ash owns the pref.
    registry->RegisterStringPref(prefs::kQuickUnlockPinSalt, "");
    return;
  }

  // Pref is owned by chrome and flagged as PUBLIC.
  registry->RegisterForeignPref(prefs::kQuickUnlockPinSalt);
}

void LoginScreenController::BindRequest(mojom::LoginScreenRequest request) {
  bindings_.AddBinding(this, std::move(request));
}

void LoginScreenController::AuthenticateUser(const AccountId& account_id,
                                             const std::string& password,
                                             bool authenticated_by_pin,
                                             OnAuthenticateCallback callback) {
  // It is an error to call this function while an authentication is in
  // progress.
  LOG_IF(ERROR, authentication_stage_ != AuthenticationStage::kIdle)
      << "Authentication stage is " << static_cast<int>(authentication_stage_);
  CHECK_EQ(authentication_stage_, AuthenticationStage::kIdle);

  if (!login_screen_client_) {
    std::move(callback).Run(base::nullopt);
    return;
  }

  // If auth is disabled by the debug overlay bypass the mojo call entirely, as
  // it will dismiss the lock screen if the password is correct.
  switch (force_fail_auth_for_debug_overlay_) {
    case ForceFailAuth::kOff:
      break;
    case ForceFailAuth::kImmediate:
      OnAuthenticateComplete(std::move(callback), false /*success*/);
      return;
    case ForceFailAuth::kDelayed:
      base::ThreadTaskRunnerHandle::Get()->PostDelayedTask(
          FROM_HERE,
          base::BindOnce(&LoginScreenController::OnAuthenticateComplete,
                         weak_factory_.GetWeakPtr(), base::Passed(&callback),
                         false),
          base::TimeDelta::FromSeconds(1));
      return;
  }

  // |DoAuthenticateUser| requires the system salt.
  authentication_stage_ = AuthenticationStage::kGetSystemSalt;
  chromeos::SystemSaltGetter::Get()->GetSystemSalt(
      base::AdaptCallbackForRepeating(
          base::BindOnce(&LoginScreenController::DoAuthenticateUser,
                         weak_factory_.GetWeakPtr(), account_id, password,
                         authenticated_by_pin, std::move(callback))));
}

void LoginScreenController::AttemptUnlock(const AccountId& account_id) {
  if (!login_screen_client_)
    return;
  login_screen_client_->AttemptUnlock(account_id);

  Shell::Get()->metrics()->login_metrics_recorder()->SetAuthMethod(
      LoginMetricsRecorder::AuthMethod::kSmartlock);
}

void LoginScreenController::HardlockPod(const AccountId& account_id) {
  if (!login_screen_client_)
    return;
  login_screen_client_->HardlockPod(account_id);
}

void LoginScreenController::RecordClickOnLockIcon(const AccountId& account_id) {
  if (!login_screen_client_)
    return;
  login_screen_client_->RecordClickOnLockIcon(account_id);
}

void LoginScreenController::OnFocusPod(const AccountId& account_id) {
  if (!login_screen_client_)
    return;
  login_screen_client_->OnFocusPod(account_id);
}

void LoginScreenController::OnNoPodFocused() {
  if (!login_screen_client_)
    return;
  login_screen_client_->OnNoPodFocused();
}

void LoginScreenController::LoadWallpaper(const AccountId& account_id) {
  if (!login_screen_client_)
    return;
  login_screen_client_->LoadWallpaper(account_id);
}

void LoginScreenController::SignOutUser() {
  if (!login_screen_client_)
    return;
  login_screen_client_->SignOutUser();
}

void LoginScreenController::CancelAddUser() {
  if (!login_screen_client_)
    return;
  login_screen_client_->CancelAddUser();
}

void LoginScreenController::LoginAsGuest() {
  if (!login_screen_client_)
    return;
  login_screen_client_->LoginAsGuest();
}

void LoginScreenController::OnMaxIncorrectPasswordAttempted(
    const AccountId& account_id) {
  if (!login_screen_client_)
    return;
  login_screen_client_->OnMaxIncorrectPasswordAttempted(account_id);
}

void LoginScreenController::FocusLockScreenApps(bool reverse) {
  if (!login_screen_client_)
    return;
  login_screen_client_->FocusLockScreenApps(reverse);
}

void LoginScreenController::ShowGaiaSignin(
    const base::Optional<AccountId>& account_id) {
  if (!login_screen_client_)
    return;
  login_screen_client_->ShowGaiaSignin(account_id);
}

void LoginScreenController::OnRemoveUserWarningShown() {
  if (!login_screen_client_)
    return;
  login_screen_client_->OnRemoveUserWarningShown();
}

void LoginScreenController::RemoveUser(const AccountId& account_id) {
  if (!login_screen_client_)
    return;
  login_screen_client_->RemoveUser(account_id);
}

void LoginScreenController::LaunchPublicSession(
    const AccountId& account_id,
    const std::string& locale,
    const std::string& input_method) {
  if (!login_screen_client_)
    return;
  login_screen_client_->LaunchPublicSession(account_id, locale, input_method);
}

void LoginScreenController::RequestPublicSessionKeyboardLayouts(
    const AccountId& account_id,
    const std::string& locale) {
  if (!login_screen_client_)
    return;
  login_screen_client_->RequestPublicSessionKeyboardLayouts(account_id, locale);
}

void LoginScreenController::AddObserver(
    LoginScreenControllerObserver* observer) {
  observers_.AddObserver(observer);
}

void LoginScreenController::RemoveObserver(
    LoginScreenControllerObserver* observer) {
  observers_.RemoveObserver(observer);
}

void LoginScreenController::FlushForTesting() {
  login_screen_client_.FlushForTesting();
}

void LoginScreenController::SetClient(mojom::LoginScreenClientPtr client) {
  login_screen_client_ = std::move(client);
}

void LoginScreenController::ShowLockScreen(ShowLockScreenCallback on_shown) {
  OnShow();
  ash::LockScreen::Show(ash::LockScreen::ScreenType::kLock);
  std::move(on_shown).Run(true);
}

void LoginScreenController::ShowLoginScreen(ShowLoginScreenCallback on_shown) {
  // Login screen can only be used during login.
  if (Shell::Get()->session_controller()->GetSessionState() !=
      session_manager::SessionState::LOGIN_PRIMARY) {
    LOG(ERROR) << "Not showing login screen since session state is "
               << static_cast<int>(
                      Shell::Get()->session_controller()->GetSessionState());
    std::move(on_shown).Run(false);
    return;
  }

  OnShow();
  // TODO(jdufault): rename ash::LockScreen to ash::LoginScreen.
  ash::LockScreen::Show(ash::LockScreen::ScreenType::kLogin);
  std::move(on_shown).Run(true);
}

void LoginScreenController::ShowErrorMessage(int32_t login_attempts,
                                             const std::string& error_text,
                                             const std::string& help_link_text,
                                             int32_t help_topic_id) {
  NOTIMPLEMENTED();
}

void LoginScreenController::ClearErrors() {
  NOTIMPLEMENTED();
}

void LoginScreenController::ShowUserPodCustomIcon(
    const AccountId& account_id,
    mojom::EasyUnlockIconOptionsPtr icon) {
  DataDispatcher()->ShowEasyUnlockIcon(account_id, icon);
}

void LoginScreenController::HideUserPodCustomIcon(const AccountId& account_id) {
  auto icon_options = mojom::EasyUnlockIconOptions::New();
  icon_options->icon = mojom::EasyUnlockIconId::NONE;
  DataDispatcher()->ShowEasyUnlockIcon(account_id, icon_options);
}

void LoginScreenController::SetAuthType(
    const AccountId& account_id,
    proximity_auth::mojom::AuthType auth_type,
    const base::string16& initial_value) {
  if (auth_type == proximity_auth::mojom::AuthType::USER_CLICK) {
    DataDispatcher()->SetClickToUnlockEnabledForUser(account_id,
                                                     true /*enabled*/);
  } else if (auth_type == proximity_auth::mojom::AuthType::ONLINE_SIGN_IN) {
    DataDispatcher()->SetForceOnlineSignInForUser(account_id);
  } else {
    NOTIMPLEMENTED();
  }
}

void LoginScreenController::LoadUsers(
    std::vector<mojom::LoginUserInfoPtr> users,
    bool show_guest) {
  DCHECK(DataDispatcher());

  DataDispatcher()->NotifyUsers(users);
}

void LoginScreenController::SetPinEnabledForUser(const AccountId& account_id,
                                                 bool is_enabled) {
  // Chrome will update pin pod state every time user tries to authenticate.
  // LockScreen is destroyed in the case of authentication success.
  if (DataDispatcher())
    DataDispatcher()->SetPinEnabledForUser(account_id, is_enabled);
}

void LoginScreenController::SetAvatarForUser(const AccountId& account_id,
                                             mojom::UserAvatarPtr avatar) {
  for (auto& observer : observers_)
    observer.SetAvatarForUser(account_id, avatar);
}

void LoginScreenController::SetAuthEnabledForUser(
    const AccountId& account_id,
    bool is_enabled,
    base::Optional<base::Time> auth_reenabled_time) {
  if (DataDispatcher()) {
    DataDispatcher()->SetAuthEnabledForUser(account_id, is_enabled,
                                            auth_reenabled_time);
  }
}

void LoginScreenController::HandleFocusLeavingLockScreenApps(bool reverse) {
  for (auto& observer : observers_)
    observer.OnFocusLeavingLockScreenApps(reverse);
}

void LoginScreenController::SetDevChannelInfo(
    const std::string& os_version_label_text,
    const std::string& enterprise_info_text,
    const std::string& bluetooth_name) {
  if (DataDispatcher()) {
    DataDispatcher()->SetDevChannelInfo(os_version_label_text,
                                        enterprise_info_text, bluetooth_name);
  }
}

void LoginScreenController::IsReadyForPassword(
    IsReadyForPasswordCallback callback) {
  std::move(callback).Run(LockScreen::IsShown() &&
                          authentication_stage_ == AuthenticationStage::kIdle);
}

void LoginScreenController::SetPublicSessionDisplayName(
    const AccountId& account_id,
    const std::string& display_name) {
  if (DataDispatcher())
    DataDispatcher()->SetPublicSessionDisplayName(account_id, display_name);
}

void LoginScreenController::SetPublicSessionLocales(
    const AccountId& account_id,
    std::vector<mojom::LocaleItemPtr> locales,
    const std::string& default_locale,
    bool show_advanced_view) {
  if (DataDispatcher()) {
    DataDispatcher()->SetPublicSessionLocales(
        account_id, locales, default_locale, show_advanced_view);
  }
}

void LoginScreenController::SetPublicSessionKeyboardLayouts(
    const AccountId& account_id,
    const std::string& locale,
    std::vector<mojom::InputMethodItemPtr> keyboard_layouts) {
  if (DataDispatcher()) {
    DataDispatcher()->SetPublicSessionKeyboardLayouts(account_id, locale,
                                                      keyboard_layouts);
  }
}

void LoginScreenController::SetFingerprintUnlockState(
    const AccountId& account_id,
    mojom::FingerprintUnlockState state) {
  if (DataDispatcher())
    DataDispatcher()->SetFingerprintUnlockState(account_id, state);
}

void LoginScreenController::DoAuthenticateUser(const AccountId& account_id,
                                               const std::string& password,
                                               bool authenticated_by_pin,
                                               OnAuthenticateCallback callback,
                                               const std::string& system_salt) {
  authentication_stage_ = AuthenticationStage::kDoAuthenticate;

  int dummy_value;
  bool is_pin =
      authenticated_by_pin && base::StringToInt(password, &dummy_value);

  Shell::Get()->metrics()->login_metrics_recorder()->SetAuthMethod(
      is_pin ? LoginMetricsRecorder::AuthMethod::kPin
             : LoginMetricsRecorder::AuthMethod::kPassword);

  login_screen_client_->AuthenticateUser(
      account_id, password, is_pin,
      base::BindOnce(&LoginScreenController::OnAuthenticateComplete,
                     weak_factory_.GetWeakPtr(), base::Passed(&callback)));
}

void LoginScreenController::OnAuthenticateComplete(
    OnAuthenticateCallback callback,
    bool success) {
  authentication_stage_ = AuthenticationStage::kUserCallback;
  std::move(callback).Run(success);
  authentication_stage_ = AuthenticationStage::kIdle;
}

LoginDataDispatcher* LoginScreenController::DataDispatcher() const {
  if (!ash::LockScreen::IsShown())
    return nullptr;
  return ash::LockScreen::Get()->data_dispatcher();
}

void LoginScreenController::OnShow() {
  SetSystemTrayVisibility(SystemTrayVisibility::kPrimary);
  if (authentication_stage_ != AuthenticationStage::kIdle) {
    AuthenticationStage authentication_stage = authentication_stage_;
    base::debug::Alias(&authentication_stage);
    LOG(FATAL) << "Unexpected authentication stage "
               << static_cast<int>(authentication_stage_);
  }
}

}  // namespace ash
