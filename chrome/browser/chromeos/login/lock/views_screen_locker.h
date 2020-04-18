// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CHROMEOS_LOGIN_LOCK_VIEWS_SCREEN_LOCKER_H_
#define CHROME_BROWSER_CHROMEOS_LOGIN_LOCK_VIEWS_SCREEN_LOCKER_H_

#include "base/memory/weak_ptr.h"
#include "chrome/browser/chromeos/lock_screen_apps/focus_cycler_delegate.h"
#include "chrome/browser/chromeos/login/lock/screen_locker.h"
#include "chrome/browser/chromeos/login/version_info_updater.h"
#include "chrome/browser/chromeos/settings/cros_settings.h"
#include "chrome/browser/ui/ash/login_screen_client.h"
#include "chromeos/dbus/power_manager_client.h"

namespace chromeos {

class UserBoardViewMojo;
class UserSelectionScreen;

// ViewsScreenLocker acts like LoginScreenClient::Delegate which handles method
// calls coming from ash into chrome.
// It is also a ScreenLocker::Delegate which handles calls from chrome into
// ash (views-based lockscreen).
class ViewsScreenLocker : public LoginScreenClient::Delegate,
                          public ScreenLocker::Delegate,
                          public PowerManagerClient::Observer,
                          public lock_screen_apps::FocusCyclerDelegate,
                          public VersionInfoUpdater::Delegate {
 public:
  explicit ViewsScreenLocker(ScreenLocker* screen_locker);
  ~ViewsScreenLocker() override;

  void Init();
  void OnLockScreenReady();

  // ScreenLocker::Delegate:
  void SetPasswordInputEnabled(bool enabled) override;
  void ShowErrorMessage(int error_msg_id,
                        HelpAppLauncher::HelpTopic help_topic_id) override;
  void ClearErrors() override;
  void AnimateAuthenticationSuccess() override;
  void OnLockWebUIReady() override;
  void OnLockBackgroundDisplayed() override;
  void OnHeaderBarVisible() override;
  void OnAshLockAnimationFinished() override;
  void SetFingerprintState(const AccountId& account_id,
                           ScreenLocker::FingerprintState state) override;
  content::WebContents* GetWebContents() override;

  // LoginScreenClient::Delegate
  void HandleAuthenticateUser(const AccountId& account_id,
                              const std::string& password,
                              bool authenticated_by_pin,
                              AuthenticateUserCallback callback) override;
  void HandleAttemptUnlock(const AccountId& account_id) override;
  void HandleHardlockPod(const AccountId& account_id) override;
  void HandleRecordClickOnLockIcon(const AccountId& account_id) override;
  void HandleOnFocusPod(const AccountId& account_id) override;
  void HandleOnNoPodFocused() override;
  bool HandleFocusLockScreenApps(bool reverse) override;
  void HandleLoginAsGuest() override;
  void HandleLaunchPublicSession(const AccountId& account_id,
                                 const std::string& locale,
                                 const std::string& input_method) override;

  // PowerManagerClient::Observer:
  void SuspendDone(const base::TimeDelta& sleep_duration) override;

  // lock_screen_apps::FocusCyclerDelegate:
  void RegisterLockScreenAppFocusHandler(
      const LockScreenAppFocusCallback& focus_handler) override;
  void UnregisterLockScreenAppFocusHandler() override;
  void HandleLockScreenAppFocusOut(bool reverse) override;

  // VersionInfoUpdater::Delegate:
  void OnOSVersionLabelTextUpdated(
      const std::string& os_version_label_text) override;
  void OnEnterpriseInfoUpdated(const std::string& message_text,
                               const std::string& asset_id) override;
  void OnDeviceInfoUpdated(const std::string& bluetooth_name) override;

 private:
  void UpdatePinKeyboardState(const AccountId& account_id);
  void OnAllowedInputMethodsChanged();
  void OnDevChannelInfoUpdated();
  void OnPinCanAuthenticate(const AccountId& account_id, bool can_authenticate);

  std::unique_ptr<UserBoardViewMojo> user_board_view_mojo_;
  std::unique_ptr<UserSelectionScreen> user_selection_screen_;

  // The ScreenLocker that owns this instance.
  ScreenLocker* const screen_locker_ = nullptr;

  // Time when lock was initiated, required for metrics.
  base::TimeTicks lock_time_;

  base::Optional<AccountId> focused_pod_account_id_;

  // Input Method Engine state used at lock screen.
  scoped_refptr<input_method::InputMethodManager::State> ime_state_;

  std::unique_ptr<CrosSettings::ObserverSubscription>
      allowed_input_methods_subscription_;

  bool lock_screen_ready_ = false;

  // Callback registered as a lock screen apps focus handler - it should be
  // called to hand focus over to lock screen apps.
  LockScreenAppFocusCallback lock_screen_app_focus_handler_;

  // Updates when version info is changed.
  VersionInfoUpdater version_info_updater_;

  std::string os_version_label_text_;
  std::string enterprise_info_text_;
  std::string bluetooth_name_;

  base::WeakPtrFactory<ViewsScreenLocker> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(ViewsScreenLocker);
};

}  // namespace chromeos

#endif  // CHROME_BROWSER_CHROMEOS_LOGIN_LOCK_VIEWS_SCREEN_LOCKER_H_
