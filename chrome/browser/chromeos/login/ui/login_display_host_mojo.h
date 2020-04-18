// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CHROMEOS_LOGIN_UI_LOGIN_DISPLAY_HOST_MOJO_H_
#define CHROME_BROWSER_CHROMEOS_LOGIN_UI_LOGIN_DISPLAY_HOST_MOJO_H_

#include <memory>
#include <string>
#include <vector>

#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "chrome/browser/chromeos/login/ui/login_display_host_common.h"
#include "chrome/browser/ui/ash/login_screen_client.h"
#include "chromeos/login/auth/auth_status_consumer.h"

namespace chromeos {

class ExistingUserController;
class GaiaDialogDelegate;
class UserBoardViewMojo;
class UserSelectionScreen;

// A LoginDisplayHost instance that sends requests to the views-based signin
// screen.
class LoginDisplayHostMojo : public LoginDisplayHostCommon,
                             public LoginScreenClient::Delegate,
                             public AuthStatusConsumer {
 public:
  LoginDisplayHostMojo();
  ~LoginDisplayHostMojo() override;

  // Called when the gaia dialog is destroyed.
  void OnDialogDestroyed(const GaiaDialogDelegate* dialog);

  // Set the users in the views login screen.
  void SetUsers(const user_manager::UserList& users);

  UserSelectionScreen* user_selection_screen() {
    return user_selection_screen_.get();
  }

  // LoginDisplayHost:
  LoginDisplay* CreateLoginDisplay(LoginDisplay::Delegate* delegate) override;
  gfx::NativeWindow GetNativeWindow() const override;
  OobeUI* GetOobeUI() const override;
  WebUILoginView* GetWebUILoginView() const override;
  void OnFinalize() override;
  void SetStatusAreaVisible(bool visible) override;
  void StartWizard(OobeScreen first_screen) override;
  WizardController* GetWizardController() override;
  void OnStartUserAdding() override;
  void CancelUserAdding() override;
  void OnStartSignInScreen(const LoginScreenContext& context) override;
  void OnPreferencesChanged() override;
  void OnStartAppLaunch() override;
  void OnStartArcKiosk() override;
  void OnBrowserCreated() override;
  void StartVoiceInteractionOobe() override;
  bool IsVoiceInteractionOobe() override;
  void UpdateGaiaDialogVisibility(
      bool visible,
      const base::Optional<AccountId>& account) override;
  void UpdateGaiaDialogSize(int width, int height) override;
  const user_manager::UserList GetUsers() override;

  // LoginScreenClient::Delegate:
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

  // AuthStatusConsumer:
  void OnAuthFailure(const AuthFailure& error) override;
  void OnAuthSuccess(const UserContext& user_context) override;

 private:
  // Initialize the dialog widget for webui (for gaia and post login screens).
  void InitWidgetAndView();

  // Callback that should be executed the authentication result is available.
  AuthenticateUserCallback on_authenticated_;

  std::unique_ptr<UserBoardViewMojo> user_board_view_mojo_;
  std::unique_ptr<UserSelectionScreen> user_selection_screen_;

  std::unique_ptr<ExistingUserController> existing_user_controller_;

  // Called after host deletion.
  std::vector<base::OnceClosure> completion_callbacks_;
  GaiaDialogDelegate* dialog_ = nullptr;
  std::unique_ptr<WizardController> wizard_controller_;

  // Users that are visible in the views login screen.
  // TODO(crbug.com/808277): consider remove user case.
  user_manager::UserList users_;

  // The account id of the user pod that's being focused.
  AccountId focused_pod_account_id_;

  base::WeakPtrFactory<LoginDisplayHostMojo> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(LoginDisplayHostMojo);
};

}  // namespace chromeos

#endif  // CHROME_BROWSER_CHROMEOS_LOGIN_UI_LOGIN_DISPLAY_HOST_MOJO_H_
