// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CHROMEOS_LOGIN_APP_LAUNCH_SIGNIN_SCREEN_H_
#define CHROME_BROWSER_CHROMEOS_LOGIN_APP_LAUNCH_SIGNIN_SCREEN_H_

#include <string>

#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "chrome/browser/ui/webui/chromeos/login/signin_screen_handler.h"
#include "chromeos/components/proximity_auth/screenlock_bridge.h"
#include "chromeos/login/auth/auth_status_consumer.h"
#include "chromeos/login/auth/authenticator.h"
#include "components/user_manager/user.h"
#include "components/user_manager/user_manager.h"

class AccountId;

namespace chromeos {

class OobeUI;

// The app launch signin screen shows the user pod of the device owner
// and requires the user to login in order to access the network dialog.
// This screen is quite similar to the standard lock screen, but we do not
// create a new view to superimpose over the desktop.
//
// TODO(tengs): This class doesn't quite follow the idiom of the other
// screen classes, as SigninScreenHandler is very tightly coupled with
// the login screen. We should do some refactoring in this area.
class AppLaunchSigninScreen : public SigninScreenHandlerDelegate,
                              public AuthStatusConsumer {
 public:
  class Delegate {
   public:
    virtual void OnOwnerSigninSuccess() = 0;

   protected:
    virtual ~Delegate() {}
  };

  AppLaunchSigninScreen(OobeUI* oobe_ui, Delegate* delegate);
  ~AppLaunchSigninScreen() override;

  void Show();

  static void SetUserManagerForTesting(user_manager::UserManager* user_manager);

 private:
  void InitOwnerUserList();
  user_manager::UserManager* GetUserManager();
  const user_manager::UserList& GetUsers() const;

  // SigninScreenHandlerDelegate implementation:
  void CancelPasswordChangedFlow() override;
  void CancelUserAdding() override;
  void Login(const UserContext& user_context,
             const SigninSpecifics& specifics) override;
  void MigrateUserData(const std::string& old_password) override;
  void OnSigninScreenReady() override;
  void RemoveUser(const AccountId& account_id) override;
  void ResyncUserData() override;
  void ShowEnterpriseEnrollmentScreen() override;
  void ShowEnableDebuggingScreen() override;
  void ShowKioskEnableScreen() override;
  void ShowKioskAutolaunchScreen() override;
  void ShowUpdateRequiredScreen() override;
  void ShowWrongHWIDScreen() override;
  void SetWebUIHandler(LoginDisplayWebUIHandler* webui_handler) override;
  bool IsShowGuest() const override;
  bool IsShowUsers() const override;
  bool ShowUsersHasChanged() const override;
  bool IsAllowNewUser() const override;
  bool AllowNewUserChanged() const override;
  bool IsSigninInProgress() const override;
  bool IsUserSigninCompleted() const override;
  void Signout() override;
  void HandleGetUsers() override;
  void CheckUserStatus(const AccountId& account_id) override;

  // AuthStatusConsumer implementation:
  void OnAuthFailure(const AuthFailure& error) override;
  void OnAuthSuccess(const UserContext& user_context) override;

  OobeUI* oobe_ui_;
  Delegate* delegate_;
  LoginDisplayWebUIHandler* webui_handler_;
  scoped_refptr<Authenticator> authenticator_;

  // This list should have at most one user, and that user should be the owner.
  user_manager::UserList owner_user_list_;

  static user_manager::UserManager* test_user_manager_;

  DISALLOW_COPY_AND_ASSIGN(AppLaunchSigninScreen);
};

}  // namespace chromeos

#endif  // CHROME_BROWSER_CHROMEOS_LOGIN_APP_LAUNCH_SIGNIN_SCREEN_H_
