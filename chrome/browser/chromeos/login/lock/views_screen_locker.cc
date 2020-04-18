// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/login/lock/views_screen_locker.h"

#include <memory>
#include <string>
#include <utility>

#include "ash/public/interfaces/login_user_info.mojom.h"
#include "base/bind.h"
#include "base/i18n/time_formatting.h"
#include "base/metrics/histogram_macros.h"
#include "base/strings/utf_string_conversions.h"
#include "base/threading/thread_task_runner_handle.h"
#include "base/time/time.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/chromeos/lock_screen_apps/state_controller.h"
#include "chrome/browser/chromeos/login/lock_screen_utils.h"
#include "chrome/browser/chromeos/login/quick_unlock/pin_backend.h"
#include "chrome/browser/chromeos/login/quick_unlock/quick_unlock_factory.h"
#include "chrome/browser/chromeos/login/quick_unlock/quick_unlock_storage.h"
#include "chrome/browser/chromeos/login/screens/chrome_user_selection_screen.h"
#include "chrome/browser/chromeos/login/user_board_view_mojo.h"
#include "chrome/browser/chromeos/system/system_clock.h"
#include "chrome/browser/ui/ash/session_controller_client.h"
#include "chrome/browser/ui/ash/wallpaper_controller_client.h"
#include "chrome/common/channel_info.h"
#include "chrome/common/pref_names.h"
#include "chrome/grit/generated_resources.h"
#include "chromeos/components/proximity_auth/screenlock_bridge.h"
#include "chromeos/login/auth/authpolicy_login_helper.h"
#include "components/user_manager/known_user.h"
#include "components/user_manager/user_manager.h"
#include "components/version_info/version_info.h"
#include "google_apis/gaia/gaia_auth_util.h"
#include "ui/base/ime/chromeos/ime_keyboard.h"
#include "ui/base/l10n/l10n_util.h"

namespace chromeos {

namespace {
constexpr char kLockDisplay[] = "lock";

ash::mojom::FingerprintUnlockState ConvertFromFingerprintState(
    ScreenLocker::FingerprintState state) {
  switch (state) {
    case ScreenLocker::FingerprintState::kRemoved:
    case ScreenLocker::FingerprintState::kHidden:
    case ScreenLocker::FingerprintState::kDefault:
      return ash::mojom::FingerprintUnlockState::UNAVAILABLE;
    case ScreenLocker::FingerprintState::kSignin:
      return ash::mojom::FingerprintUnlockState::AUTH_SUCCESS;
    case ScreenLocker::FingerprintState::kFailed:
      return ash::mojom::FingerprintUnlockState::AUTH_FAILED;
  }
}

}  // namespace

ViewsScreenLocker::ViewsScreenLocker(ScreenLocker* screen_locker)
    : screen_locker_(screen_locker),
      version_info_updater_(this),
      weak_factory_(this) {
  LoginScreenClient::Get()->SetDelegate(this);
  user_board_view_mojo_ = std::make_unique<UserBoardViewMojo>();
  user_selection_screen_ =
      std::make_unique<ChromeUserSelectionScreen>(kLockDisplay);
  user_selection_screen_->SetView(user_board_view_mojo_.get());

  allowed_input_methods_subscription_ =
      CrosSettings::Get()->AddSettingsObserver(
          kDeviceLoginScreenInputMethods,
          base::Bind(&ViewsScreenLocker::OnAllowedInputMethodsChanged,
                     base::Unretained(this)));
}

ViewsScreenLocker::~ViewsScreenLocker() {
  if (lock_screen_apps::StateController::IsEnabled())
    lock_screen_apps::StateController::Get()->SetFocusCyclerDelegate(nullptr);
  LoginScreenClient::Get()->SetDelegate(nullptr);
}

void ViewsScreenLocker::Init() {
  lock_time_ = base::TimeTicks::Now();
  user_selection_screen_->Init(screen_locker_->users());
  LoginScreenClient::Get()->login_screen()->LoadUsers(
      user_selection_screen_->UpdateAndReturnUserListForMojo(),
      false /* show_guests */);
  if (!ime_state_.get())
    ime_state_ = input_method::InputMethodManager::Get()->GetActiveIMEState();

  // Reset Caps Lock state when lock screen is shown.
  input_method::InputMethodManager::Get()->GetImeKeyboard()->SetCapsLockEnabled(
      false);

  // Enable pin for any users who can use it.
  if (user_manager::UserManager::IsInitialized()) {
    for (user_manager::User* user :
         user_manager::UserManager::Get()->GetLoggedInUsers()) {
      UpdatePinKeyboardState(user->GetAccountId());
    }
  }

  version_info::Channel channel = chrome::GetChannel();
  bool should_show_version = (channel == version_info::Channel::STABLE ||
                              channel == version_info::Channel::BETA)
                                 ? false
                                 : true;
  if (should_show_version) {
#if defined(OFFICIAL_BUILD)
    version_info_updater_.StartUpdate(true);
#else
    version_info_updater_.StartUpdate(false);
#endif
  }
}

void ViewsScreenLocker::OnLockScreenReady() {
  lock_screen_ready_ = true;
  user_selection_screen_->InitEasyUnlock();
  UMA_HISTOGRAM_TIMES("LockScreen.LockReady",
                      base::TimeTicks::Now() - lock_time_);
  screen_locker_->ScreenLockReady();
  if (lock_screen_apps::StateController::IsEnabled())
    lock_screen_apps::StateController::Get()->SetFocusCyclerDelegate(this);
  OnAllowedInputMethodsChanged();
}

void ViewsScreenLocker::SetPasswordInputEnabled(bool enabled) {
  NOTIMPLEMENTED();
}

void ViewsScreenLocker::ShowErrorMessage(
    int error_msg_id,
    HelpAppLauncher::HelpTopic help_topic_id) {
  // TODO(xiaoyinh): Complete the implementation here.
  LoginScreenClient::Get()->login_screen()->ShowErrorMessage(
      0 /* login_attempts */, std::string(), std::string(),
      static_cast<int>(help_topic_id));
}

void ViewsScreenLocker::ClearErrors() {
  LoginScreenClient::Get()->login_screen()->ClearErrors();
}

void ViewsScreenLocker::AnimateAuthenticationSuccess() {
  NOTIMPLEMENTED();
}

void ViewsScreenLocker::OnLockWebUIReady() {
  NOTIMPLEMENTED();
}

void ViewsScreenLocker::OnLockBackgroundDisplayed() {
  NOTIMPLEMENTED();
}

void ViewsScreenLocker::OnHeaderBarVisible() {
  NOTIMPLEMENTED();
}

void ViewsScreenLocker::OnAshLockAnimationFinished() {
  SessionControllerClient::Get()->NotifyChromeLockAnimationsComplete();
}

void ViewsScreenLocker::SetFingerprintState(
    const AccountId& account_id,
    ScreenLocker::FingerprintState state) {
  LoginScreenClient::Get()->login_screen()->SetFingerprintUnlockState(
      account_id, ConvertFromFingerprintState(state));
}

content::WebContents* ViewsScreenLocker::GetWebContents() {
  return nullptr;
}

void ViewsScreenLocker::HandleAuthenticateUser(
    const AccountId& account_id,
    const std::string& password,
    bool authenticated_by_pin,
    AuthenticateUserCallback callback) {
  DCHECK_EQ(account_id.GetUserEmail(),
            gaia::SanitizeEmail(account_id.GetUserEmail()));
  quick_unlock::QuickUnlockStorage* quick_unlock_storage =
      quick_unlock::QuickUnlockFactory::GetForAccountId(account_id);
  // If pin storage is unavailable, |authenticated_by_pin| must be false.
  DCHECK(!quick_unlock_storage ||
         quick_unlock_storage->IsPinAuthenticationAvailable() ||
         !authenticated_by_pin);

  const user_manager::User* const user =
      user_manager::UserManager::Get()->FindUser(account_id);
  DCHECK(user);
  UserContext user_context(*user);
  user_context.SetKey(
      Key(chromeos::Key::KEY_TYPE_PASSWORD_PLAIN, std::string(), password));
  user_context.SetIsUsingPin(authenticated_by_pin);
  user_context.SetSyncPasswordData(password_manager::PasswordHashData(
      account_id.GetUserEmail(), base::UTF8ToUTF16(password),
      false /*force_update*/));
  if (account_id.GetAccountType() == AccountType::ACTIVE_DIRECTORY &&
      (user_context.GetUserType() !=
       user_manager::UserType::USER_TYPE_ACTIVE_DIRECTORY)) {
    LOG(FATAL) << "Incorrect Active Directory user type "
               << user_context.GetUserType();
  }
  ScreenLocker::default_screen_locker()->Authenticate(user_context,
                                                      std::move(callback));
  UpdatePinKeyboardState(account_id);
}

void ViewsScreenLocker::HandleAttemptUnlock(const AccountId& account_id) {
  user_selection_screen_->AttemptEasyUnlock(account_id);
}

void ViewsScreenLocker::HandleHardlockPod(const AccountId& account_id) {
  user_selection_screen_->HardLockPod(account_id);
}

void ViewsScreenLocker::HandleRecordClickOnLockIcon(
    const AccountId& account_id) {
  user_selection_screen_->RecordClickOnLockIcon(account_id);
}

void ViewsScreenLocker::HandleOnFocusPod(const AccountId& account_id) {
  proximity_auth::ScreenlockBridge::Get()->SetFocusedUser(account_id);
  if (user_selection_screen_)
    user_selection_screen_->CheckUserStatus(account_id);

  focused_pod_account_id_ = base::Optional<AccountId>(account_id);

  const user_manager::User* user =
      user_manager::UserManager::Get()->FindUser(account_id);
  // |user| may be null in kiosk mode or unit tests.
  if (user && user->is_logged_in() && !user->is_active()) {
    SessionControllerClient::DoSwitchActiveUser(account_id);
  } else {
    lock_screen_utils::SetUserInputMethod(account_id.GetUserEmail(),
                                          ime_state_.get());
    lock_screen_utils::SetKeyboardSettings(account_id);
    WallpaperControllerClient::Get()->ShowUserWallpaper(account_id);

    bool use_24hour_clock = false;
    if (user_manager::known_user::GetBooleanPref(
            account_id, prefs::kUse24HourClock, &use_24hour_clock)) {
      g_browser_process->platform_part()
          ->GetSystemClock()
          ->SetLastFocusedPodHourClockType(
              use_24hour_clock ? base::k24HourClock : base::k12HourClock);
    }
  }
}

void ViewsScreenLocker::HandleOnNoPodFocused() {
  focused_pod_account_id_.reset();
  lock_screen_utils::EnforcePolicyInputMethods(std::string());
}

bool ViewsScreenLocker::HandleFocusLockScreenApps(bool reverse) {
  if (lock_screen_app_focus_handler_.is_null())
    return false;

  lock_screen_app_focus_handler_.Run(reverse);
  return true;
}

void ViewsScreenLocker::HandleLoginAsGuest() {
  NOTREACHED();
}

void ViewsScreenLocker::HandleLaunchPublicSession(
    const AccountId& account_id,
    const std::string& locale,
    const std::string& input_method) {
  NOTREACHED();
}

void ViewsScreenLocker::SuspendDone(const base::TimeDelta& sleep_duration) {
  for (user_manager::User* user :
       user_manager::UserManager::Get()->GetUnlockUsers()) {
    UpdatePinKeyboardState(user->GetAccountId());
  }
}

void ViewsScreenLocker::RegisterLockScreenAppFocusHandler(
    const LockScreenAppFocusCallback& focus_handler) {
  lock_screen_app_focus_handler_ = focus_handler;
}

void ViewsScreenLocker::UnregisterLockScreenAppFocusHandler() {
  lock_screen_app_focus_handler_.Reset();
}

void ViewsScreenLocker::HandleLockScreenAppFocusOut(bool reverse) {
  LoginScreenClient::Get()->login_screen()->HandleFocusLeavingLockScreenApps(
      reverse);
}

void ViewsScreenLocker::OnOSVersionLabelTextUpdated(
    const std::string& os_version_label_text) {
  os_version_label_text_ = os_version_label_text;
  OnDevChannelInfoUpdated();
}

void ViewsScreenLocker::OnEnterpriseInfoUpdated(const std::string& message_text,
                                                const std::string& asset_id) {
  if (asset_id.empty())
    return;
  enterprise_info_text_ = l10n_util::GetStringFUTF8(
      IDS_OOBE_ASSET_ID_LABEL, base::UTF8ToUTF16(asset_id));
  OnDevChannelInfoUpdated();
}

void ViewsScreenLocker::OnDeviceInfoUpdated(const std::string& bluetooth_name) {
  bluetooth_name_ = bluetooth_name;
  OnDevChannelInfoUpdated();
}

void ViewsScreenLocker::UpdatePinKeyboardState(const AccountId& account_id) {
  quick_unlock::PinBackend::GetInstance()->CanAuthenticate(
      account_id, base::BindOnce(&ViewsScreenLocker::OnPinCanAuthenticate,
                                 weak_factory_.GetWeakPtr(), account_id));
}

void ViewsScreenLocker::OnAllowedInputMethodsChanged() {
  if (!lock_screen_ready_)
    return;

  if (focused_pod_account_id_) {
    std::string user_input_method = lock_screen_utils::GetUserLastInputMethod(
        focused_pod_account_id_->GetUserEmail());
    lock_screen_utils::EnforcePolicyInputMethods(user_input_method);
  } else {
    lock_screen_utils::EnforcePolicyInputMethods(std::string());
  }
}

void ViewsScreenLocker::OnDevChannelInfoUpdated() {
  LoginScreenClient::Get()->login_screen()->SetDevChannelInfo(
      os_version_label_text_, enterprise_info_text_, bluetooth_name_);
}

void ViewsScreenLocker::OnPinCanAuthenticate(const AccountId& account_id,
                                             bool can_authenticate) {
  LoginScreenClient::Get()->login_screen()->SetPinEnabledForUser(
      account_id, can_authenticate);
}

}  // namespace chromeos
