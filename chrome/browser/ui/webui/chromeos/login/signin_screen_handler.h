// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_WEBUI_CHROMEOS_LOGIN_SIGNIN_SCREEN_HANDLER_H_
#define CHROME_BROWSER_UI_WEBUI_CHROMEOS_LOGIN_SIGNIN_SCREEN_HANDLER_H_

#include <map>
#include <memory>
#include <set>
#include <string>

#include "ash/detachable_base/detachable_base_observer.h"
#include "ash/public/interfaces/wallpaper.mojom.h"
#include "base/callback.h"
#include "base/compiler_specific.h"
#include "base/containers/hash_tables.h"
#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/memory/weak_ptr.h"
#include "base/optional.h"
#include "base/scoped_observer.h"
#include "chrome/browser/chromeos/lock_screen_apps/state_observer.h"
#include "chrome/browser/chromeos/login/screens/error_screen.h"
#include "chrome/browser/chromeos/login/signin_specifics.h"
#include "chrome/browser/chromeos/login/ui/login_display.h"
#include "chrome/browser/chromeos/settings/cros_settings.h"
#include "chrome/browser/ui/ash/tablet_mode_client_observer.h"
#include "chrome/browser/ui/webui/chromeos/login/base_webui_handler.h"
#include "chrome/browser/ui/webui/chromeos/login/network_state_informer.h"
#include "chrome/browser/ui/webui/chromeos/login/oobe_ui.h"
#include "chromeos/components/proximity_auth/screenlock_bridge.h"
#include "chromeos/dbus/power_manager_client.h"
#include "chromeos/network/portal_detector/network_portal_detector.h"
#include "components/session_manager/core/session_manager_observer.h"
#include "components/user_manager/user_manager.h"
#include "content/public/browser/notification_observer.h"
#include "content/public/browser/notification_registrar.h"
#include "content/public/browser/web_ui.h"
#include "mojo/public/cpp/bindings/associated_binding.h"
#include "net/base/net_errors.h"
#include "ui/base/ime/chromeos/ime_keyboard.h"
#include "ui/base/ime/chromeos/input_method_manager.h"
#include "ui/events/event_handler.h"

class AccountId;

namespace ash {
namespace mojom {
enum class TrayActionState;
}  // namespace mojom

class DetachableBaseHandler;
}  // namespace ash

namespace base {
class DictionaryValue;
class ListValue;
}

namespace lock_screen_apps {
class StateController;
}

namespace session_manager {
class SessionManager;
}

namespace chromeos {

class CoreOobeView;
class ErrorScreensHistogramHelper;
class GaiaScreenHandler;
class LoginFeedback;
class NativeWindowDelegate;
class SupervisedUserCreationScreenHandler;
class User;
class UserContext;

// Helper class to pass initial parameters to the login screen.
class LoginScreenContext {
 public:
  LoginScreenContext();
  explicit LoginScreenContext(const base::ListValue* args);

  void set_email(const std::string& email) { email_ = email; }
  const std::string& email() const { return email_; }

  void set_oobe_ui(bool oobe_ui) { oobe_ui_ = oobe_ui; }
  bool oobe_ui() const { return oobe_ui_; }

 private:
  void Init();

  std::string email_;
  bool oobe_ui_;
};

// An interface for WebUILoginDisplay to call SigninScreenHandler.
class LoginDisplayWebUIHandler {
 public:
  virtual void ClearAndEnablePassword() = 0;
  virtual void ClearUserPodPassword() = 0;
  virtual void OnUserRemoved(const AccountId& account_id,
                             bool last_user_removed) = 0;
  virtual void OnUserImageChanged(const user_manager::User& user) = 0;
  virtual void OnPreferencesChanged() = 0;
  virtual void ResetSigninScreenHandlerDelegate() = 0;
  virtual void ShowError(int login_attempts,
                         const std::string& error_text,
                         const std::string& help_link_text,
                         HelpAppLauncher::HelpTopic help_topic_id) = 0;
  virtual void ShowErrorScreen(LoginDisplay::SigninError error_id) = 0;
  virtual void ShowSigninUI(const std::string& email) = 0;
  virtual void ShowPasswordChangedDialog(bool show_password_error,
                                         const std::string& email) = 0;
  // Show sign-in screen for the given credentials.
  // |services| - list of services returned by userInfo call as JSON array.
  // Should be empty array for regular user: "[]".
  virtual void ShowSigninScreenForTest(const std::string& username,
                                       const std::string& password,
                                       const std::string& services) = 0;
  virtual void ShowWhitelistCheckFailedError() = 0;
  virtual void ShowUnrecoverableCrypthomeErrorDialog() = 0;
  virtual void LoadUsers(const user_manager::UserList& users,
                         const base::ListValue& users_list) = 0;

 protected:
  virtual ~LoginDisplayWebUIHandler() {}
};

// An interface for SigninScreenHandler to call WebUILoginDisplay.
class SigninScreenHandlerDelegate {
 public:
  // --------------- Password change flow methods.
  // Cancels current password changed flow.
  virtual void CancelPasswordChangedFlow() = 0;

  // Decrypt cryptohome using user provided |old_password|
  // and migrate to new password.
  virtual void MigrateUserData(const std::string& old_password) = 0;

  // Ignore password change, remove existing cryptohome and
  // force full sync of user data.
  virtual void ResyncUserData() = 0;

  // --------------- Sign in/out methods.
  // Sign in using username and password specified as a part of |user_context|.
  // Used for both known and new users.
  virtual void Login(const UserContext& user_context,
                     const SigninSpecifics& specifics) = 0;

  // Returns true if sign in is in progress.
  virtual bool IsSigninInProgress() const = 0;

  // Signs out if the screen is currently locked.
  virtual void Signout() = 0;

  // --------------- Shared with login display methods.
  // Notify the delegate when the sign-in UI is finished loading.
  virtual void OnSigninScreenReady() = 0;

  // Shows Enterprise Enrollment screen.
  virtual void ShowEnterpriseEnrollmentScreen() = 0;

  // Shows Enable Developer Features screen.
  virtual void ShowEnableDebuggingScreen() = 0;

  // Shows Kiosk Enable screen.
  virtual void ShowKioskEnableScreen() = 0;

  // Shows Reset screen.
  virtual void ShowKioskAutolaunchScreen() = 0;

  // Show wrong hwid screen.
  virtual void ShowWrongHWIDScreen() = 0;

  // Show update required screen.
  virtual void ShowUpdateRequiredScreen() = 0;

  // --------------- Rest of the methods.
  // Cancels user adding.
  virtual void CancelUserAdding() = 0;

  // Attempts to remove given user.
  virtual void RemoveUser(const AccountId& account_id) = 0;

  // Let the delegate know about the handler it is supposed to be using.
  virtual void SetWebUIHandler(LoginDisplayWebUIHandler* webui_handler) = 0;

  // Whether login as guest is available.
  virtual bool IsShowGuest() const = 0;

  // Whether to show the user pods or only GAIA sign in.
  // Public sessions are always shown.
  virtual bool IsShowUsers() const = 0;

  // Whether the show user pods setting has changed.
  virtual bool ShowUsersHasChanged() const = 0;

  // Whether the create new account option in GAIA is enabled by the setting.
  virtual bool IsAllowNewUser() const = 0;

  // Whether the allow new user setting has changed.
  virtual bool AllowNewUserChanged() const = 0;

  // Whether user sign in has completed.
  virtual bool IsUserSigninCompleted() const = 0;

  // Request to (re)load user list.
  virtual void HandleGetUsers() = 0;

  // Runs an OAuth token validation check for user.
  virtual void CheckUserStatus(const AccountId& account_id) = 0;

 protected:
  virtual ~SigninScreenHandlerDelegate() {}
};

// A class that handles the WebUI hooks in sign-in screen in OobeUI and
// LoginDisplay.
class SigninScreenHandler
    : public BaseWebUIHandler,
      public LoginDisplayWebUIHandler,
      public content::NotificationObserver,
      public NetworkStateInformer::NetworkStateInformerObserver,
      public PowerManagerClient::Observer,
      public input_method::ImeKeyboard::Observer,
      public TabletModeClientObserver,
      public lock_screen_apps::StateObserver,
      public OobeUI::Observer,
      public session_manager::SessionManagerObserver,
      public ash::mojom::WallpaperObserver,
      public ash::DetachableBaseObserver {
 public:
  SigninScreenHandler(
      const scoped_refptr<NetworkStateInformer>& network_state_informer,
      ErrorScreen* error_screen,
      CoreOobeView* core_oobe_view,
      GaiaScreenHandler* gaia_screen_handler,
      JSCallsContainer* js_calls_container);
  ~SigninScreenHandler() override;

  static std::string GetUserLastInputMethod(const std::string& username);

  // Update current input method (namely keyboard layout) in the given IME state
  // to last input method used by this user.
  static void SetUserInputMethod(
      const std::string& username,
      input_method::InputMethodManager::State* ime_state);

  // Shows the sign in screen.
  void Show(const LoginScreenContext& context);

  // Sets delegate to be used by the handler. It is guaranteed that valid
  // delegate is set before Show() method will be called.
  void SetDelegate(SigninScreenHandlerDelegate* delegate);

  void SetNativeWindowDelegate(NativeWindowDelegate* native_window_delegate);

  // NetworkStateInformer::NetworkStateInformerObserver implementation:
  void OnNetworkReady() override;
  void UpdateState(NetworkError::ErrorReason reason) override;

  // Required Local State preferences.
  static void RegisterPrefs(PrefRegistrySimple* registry);

  // OobeUI::Observer implementation:
  void OnCurrentScreenChanged(OobeScreen current_screen,
                              OobeScreen new_screen) override;
  void OnScreenInitialized(OobeScreen screen) override{};

  // ash::mojom::WallpaperObserver implementation:
  void OnWallpaperChanged(uint32_t image_id) override;
  void OnWallpaperColorsChanged(
      const std::vector<SkColor>& prominent_colors) override;
  void OnWallpaperBlurChanged(bool blurred) override;

  // ash::DetachableBaseObserver:
  void OnDetachableBasePairingStatusChanged(
      ash::DetachableBasePairingStatus pairing_status) override;
  void OnDetachableBaseRequiresUpdateChanged(bool requires_update) override;

  void SetFocusPODCallbackForTesting(base::Closure callback);

  // To avoid spurious error messages on flaky networks, the offline message is
  // only shown if the network is offline for a threshold number of seconds.
  // This method provides an ability to reduce the threshold to zero, allowing
  // the offline message to show instantaneously in tests. The threshold can
  // also be set to a high value to disable the offline message on slow
  // configurations like MSAN, where it otherwise triggers on every run.
  void SetOfflineTimeoutForTesting(base::TimeDelta offline_timeout);

  // Gets the keyboard remapped pref value for |pref_name| key. Returns true if
  // successful, otherwise returns false.
  bool GetKeyboardRemappedPrefValue(const std::string& pref_name, int* value);

 private:
  enum UIState {
    UI_STATE_UNKNOWN = 0,
    UI_STATE_GAIA_SIGNIN,
    UI_STATE_ACCOUNT_PICKER,
  };

  friend class GaiaScreenHandler;
  friend class ReportDnsCacheClearedOnUIThread;
  friend class SupervisedUserCreationScreenHandler;

  void ShowImpl();

  // Updates current UI of the signin screen according to |ui_state|
  // argument.  Optionally it can pass screen initialization data via
  // |params| argument.
  void UpdateUIState(UIState ui_state, base::DictionaryValue* params);

  void UpdateStateInternal(NetworkError::ErrorReason reason, bool force_update);
  void SetupAndShowOfflineMessage(NetworkStateInformer::State state,
                                  NetworkError::ErrorReason reason);
  void HideOfflineMessage(NetworkStateInformer::State state,
                          NetworkError::ErrorReason reason);
  void ReloadGaia(bool force_reload);

  // BaseScreenHandler implementation:
  void DeclareLocalizedValues(
      ::login::LocalizedValuesBuilder* builder) override;
  void Initialize() override;
  gfx::NativeWindow GetNativeWindow() override;

  // WebUIMessageHandler implementation:
  void RegisterMessages() override;

  // LoginDisplayWebUIHandler implementation:
  void ClearAndEnablePassword() override;
  void ClearUserPodPassword() override;
  void OnUserRemoved(const AccountId& account_id,
                     bool last_user_removed) override;
  void OnUserImageChanged(const user_manager::User& user) override;
  void OnPreferencesChanged() override;
  void ResetSigninScreenHandlerDelegate() override;
  void ShowError(int login_attempts,
                 const std::string& error_text,
                 const std::string& help_link_text,
                 HelpAppLauncher::HelpTopic help_topic_id) override;
  void ShowSigninUI(const std::string& email) override;
  void ShowPasswordChangedDialog(bool show_password_error,
                                 const std::string& email) override;
  void ShowErrorScreen(LoginDisplay::SigninError error_id) override;
  void ShowSigninScreenForTest(const std::string& username,
                               const std::string& password,
                               const std::string& services) override;
  void ShowWhitelistCheckFailedError() override;
  void ShowUnrecoverableCrypthomeErrorDialog() override;
  void LoadUsers(const user_manager::UserList& users,
                 const base::ListValue& users_list) override;

  // content::NotificationObserver implementation:
  void Observe(int type,
               const content::NotificationSource& source,
               const content::NotificationDetails& details) override;

  // PowerManagerClient::Observer implementation:
  void SuspendDone(const base::TimeDelta& sleep_duration) override;

  // TabletModeClientObserver:
  void OnTabletModeToggled(bool enabled) override;

  // session_manager::SessionManagerObserver:
  void OnSessionStateChanged() override;

  // lock_screen_apps::StateObserver:
  void OnLockScreenNoteStateChanged(ash::mojom::TrayActionState state) override;

  void UpdateAddButtonStatus();

  // Restore input focus to current user pod.
  void RefocusCurrentPod();

  // Enable or disable the pin keyboard for the given account.
  void UpdatePinKeyboardState(const AccountId& account_id);
  void SetPinEnabledForUser(const AccountId& account_id, bool is_enabled);
  // Callback run by PinBackend. If |should_preload| is true the PIN keyboard is
  // preloaded.
  void PreloadPinKeyboard(bool should_preload);

  // WebUI message handlers.
  void HandleGetUsers();
  void HandleAuthenticateUser(const AccountId& account_id,
                              const std::string& password,
                              bool authenticated_by_pin);
  void HandleAttemptUnlock(const std::string& username);
  void HandleLaunchIncognito();
  void HandleLaunchPublicSession(const AccountId& account_id,
                                 const std::string& locale,
                                 const std::string& input_method);
  void HandleOfflineLogin(const base::ListValue* args);
  void HandleShutdownSystem();
  void HandleRebootSystem();
  void HandleRemoveUser(const AccountId& account_id);
  void HandleToggleEnrollmentScreen();
  void HandleToggleEnrollmentAd();
  void HandleToggleEnableDebuggingScreen();
  void HandleToggleKioskEnableScreen();
  void HandleToggleResetScreen();
  void HandleToggleKioskAutolaunchScreen();
  void HandleAccountPickerReady();
  void HandleWallpaperReady();
  void HandleSignOutUser();
  void HandleOpenInternetDetailDialog();
  void HandleLoginVisible(const std::string& source);
  void HandleCancelPasswordChangedFlow(const AccountId& account_id);
  void HandleCancelUserAdding();
  void HandleMigrateUserData(const std::string& password);
  void HandleResyncUserData();
  void HandleLoginUIStateChanged(const std::string& source, bool active);
  void HandleUnlockOnLoginSuccess();
  void HandleLoginScreenUpdate();
  void HandleShowLoadingTimeoutError();
  void HandleShowSupervisedUserCreationScreen();
  void HandleFocusPod(const AccountId& account_id, bool is_large_pod);
  void HandleNoPodFocused();
  void HandleHardlockPod(const std::string& user_id);
  void HandleLaunchKioskApp(const AccountId& app_account_id,
                            bool diagnostic_mode);
  void HandleLaunchArcKioskApp(const AccountId& app_account_id);
  void HandleGetPublicSessionKeyboardLayouts(const AccountId& account_id,
                                             const std::string& locale);
  void HandleGetTabletModeState();
  void HandleLogRemoveUserWarningShown();
  void HandleFirstIncorrectPasswordAttempt(const AccountId& account_id);
  void HandleMaxIncorrectPasswordAttempts(const AccountId& account_id);
  void HandleSendFeedback();
  void HandleSendFeedbackAndResyncUserData();
  void HandleRequestNewNoteAction(const std::string& request_type);
  void HandleNewNoteLaunchAnimationDone();
  void HandleCloseLockScreenApp();

  // Sends the list of |keyboard_layouts| available for the |locale| that is
  // currently selected for the public session identified by |user_id|.
  void SendPublicSessionKeyboardLayouts(
      const AccountId& account_id,
      const std::string& locale,
      std::unique_ptr<base::ListValue> keyboard_layouts);

  // Returns true iff
  // (i)   log in is restricted to some user list,
  // (ii)  all users in the restricted list are present.
  bool AllWhitelistedUsersPresent();

  // Cancels password changed flow - switches back to login screen.
  // Called as a callback after cookies are cleared.
  void CancelPasswordChangedFlowInternal();

  // Returns true if current visible screen is the Gaia sign-in page.
  bool IsGaiaVisible() const;

  // Returns true if current visible screen is the error screen over
  // Gaia sign-in page.
  bool IsGaiaHiddenByError() const;

  // Returns true if current screen is the error screen over signin
  // screen.
  bool IsSigninScreenHiddenByError() const;

  bool ShouldLoadGaia() const;

  net::Error FrameError() const;

  // input_method::ImeKeyboard::Observer implementation:
  void OnCapsLockChanged(bool enabled) override;
  void OnLayoutChanging(const std::string& layout_name) override {}

  // Callback invoked after the feedback is finished.
  void OnFeedbackFinished();

  // Callback invoked after the feedback sent from the unrecoverable cryptohome
  // page is finished.
  void OnUnrecoverableCryptohomeFeedbackFinished();

  // Called when the cros property controlling allowed input methods changes.
  void OnAllowedInputMethodsChanged();

  // After proxy auth information has been supplied, this function re-enables
  // responding to network state notifications.
  void ReenableNetworkStateUpdatesAfterProxyAuth();

  // Determines whether a warning about the detachable base getting changed
  // should be shown to the user. The warning is shown a detachable base is
  // present, and the user whose pod is currently focused has used a different
  // base last time. It updates the detachable base warning visibility as
  // required.
  void UpdateDetachableBaseChangedError();

  // Sends a request to the UI to show a detachable base change warning for the
  // currently focused user pod. The warning warns the user that the currently
  // attached base is different than the one they last used, and that it might
  // not be trusted.
  void ShowDetachableBaseChangedError();

  // If a detachable base change warning was requested to be shown, sends a
  // request to UI to hide the warning.
  void HideDetachableBaseChangedError();

  // Current UI state of the signin screen.
  UIState ui_state_ = UI_STATE_UNKNOWN;

  // A delegate that glues this handler with backend LoginDisplay.
  SigninScreenHandlerDelegate* delegate_ = nullptr;

  // A delegate used to get gfx::NativeWindow.
  NativeWindowDelegate* native_window_delegate_ = nullptr;

  // Whether screen should be shown right after initialization.
  bool show_on_init_ = false;

  // Keeps whether screen should be shown for OOBE.
  bool oobe_ui_ = false;

  // Is account picker being shown for the first time.
  bool is_account_picker_showing_first_time_ = false;

  // Network state informer used to keep signin screen up.
  scoped_refptr<NetworkStateInformer> network_state_informer_;

  // Set to true once |LOGIN_WEBUI_VISIBLE| notification is observed.
  bool webui_visible_ = false;
  bool preferences_changed_delayed_ = false;

  ErrorScreen* error_screen_ = nullptr;
  CoreOobeView* core_oobe_view_ = nullptr;

  NetworkStateInformer::State last_network_state_ =
      NetworkStateInformer::UNKNOWN;

  base::CancelableClosure update_state_closure_;
  base::CancelableClosure connecting_closure_;

  content::NotificationRegistrar registrar_;

  std::unique_ptr<CrosSettings::ObserverSubscription>
      allowed_input_methods_subscription_;

  // Whether we're currently ignoring network state updates because a proxy auth
  // UI pending (or we're waiting for a grace period after the proxy auth UI is
  // finished for the network to switch into the ONLINE state).
  bool network_state_ignored_until_proxy_auth_ = false;

  // Used for pending GAIA reloads.
  NetworkError::ErrorReason gaia_reload_reason_ =
      NetworkError::ERROR_REASON_NONE;

  bool caps_lock_enabled_ = false;

  // If network has accidentally changed to the one that requires proxy
  // authentication, we will automatically reload gaia page that will bring
  // "Proxy authentication" dialog to the user. To prevent flakiness, we will do
  // it at most 3 times.
  int proxy_auth_dialog_reload_times_;

  // True if we need to reload gaia page to bring back "Proxy authentication"
  // dialog.
  bool proxy_auth_dialog_need_reload_ = false;

  // Non-owning ptr.
  // TODO(antrim@): remove this dependency.
  GaiaScreenHandler* gaia_screen_handler_ = nullptr;

  // Input Method Engine state used at signin screen.
  scoped_refptr<input_method::InputMethodManager::State> ime_state_;

  // This callback captures "focusPod finished" event for tests.
  base::Closure test_focus_pod_callback_;

  // True if SigninScreenHandler has already been added to OobeUI observers.
  bool oobe_ui_observer_added_ = false;

  bool is_offline_timeout_for_test_set_ = false;
  base::TimeDelta offline_timeout_for_test_;

  std::unique_ptr<ErrorScreensHistogramHelper> histogram_helper_;

  std::unique_ptr<LoginFeedback> login_feedback_;

  std::unique_ptr<AccountId> focused_pod_account_id_;

  // If set, the account for which detachable base change warning was shown in
  // the login UI.
  base::Optional<AccountId> account_with_detachable_base_error_;

  ScopedObserver<session_manager::SessionManager,
                 session_manager::SessionManagerObserver>
      session_manager_observer_;
  ScopedObserver<lock_screen_apps::StateController,
                 lock_screen_apps::StateObserver>
      lock_screen_apps_observer_;

  ScopedObserver<ash::DetachableBaseHandler, ash::DetachableBaseObserver>
      detachable_base_observer_;

  // The binding this instance uses to implement ash::mojom::WallpaperObserver.
  mojo::AssociatedBinding<ash::mojom::WallpaperObserver> observer_binding_;

  base::WeakPtrFactory<SigninScreenHandler> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(SigninScreenHandler);
};

}  // namespace chromeos

#endif  // CHROME_BROWSER_UI_WEBUI_CHROMEOS_LOGIN_SIGNIN_SCREEN_HANDLER_H_
