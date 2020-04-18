// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/login/ui/login_display_mojo.h"

#include "chrome/browser/chrome_notification_types.h"
#include "chrome/browser/chromeos/login/screens/chrome_user_selection_screen.h"
#include "chrome/browser/chromeos/login/screens/user_selection_screen.h"
#include "chrome/browser/chromeos/login/ui/login_display_host_mojo.h"
#include "chrome/browser/chromeos/settings/cros_settings.h"
#include "chrome/browser/ui/ash/login_screen_client.h"
#include "chromeos/dbus/dbus_thread_manager.h"
#include "chromeos/dbus/session_manager_client.h"
#include "components/user_manager/known_user.h"
#include "content/public/browser/notification_service.h"

namespace chromeos {

LoginDisplayMojo::LoginDisplayMojo(Delegate* delegate,
                                   LoginDisplayHostMojo* host)
    : LoginDisplay(delegate), host_(host) {
  user_manager::UserManager::Get()->AddObserver(this);
}

LoginDisplayMojo::~LoginDisplayMojo() {
  user_manager::UserManager::Get()->RemoveObserver(this);
}

void LoginDisplayMojo::ClearAndEnablePassword() {}

void LoginDisplayMojo::Init(const user_manager::UserList& filtered_users,
                            bool show_guest,
                            bool show_users,
                            bool show_new_user) {
  host_->SetUsers(filtered_users);

  // Load the login screen.
  auto* client = LoginScreenClient::Get();
  client->SetDelegate(host_);
  client->login_screen()->ShowLoginScreen(base::BindOnce([](bool did_show) {
    CHECK(did_show);

    // Some auto-tests depend on login-prompt-visible, like
    // login_SameSessionTwice.
    VLOG(1) << "Emitting login-prompt-visible";
    chromeos::DBusThreadManager::Get()
        ->GetSessionManagerClient()
        ->EmitLoginPromptVisible();

    content::NotificationService::current()->Notify(
        chrome::NOTIFICATION_LOGIN_OR_LOCK_WEBUI_VISIBLE,
        content::NotificationService::AllSources(),
        content::NotificationService::NoDetails());
  }));

  UserSelectionScreen* user_selection_screen = host_->user_selection_screen();
  user_selection_screen->Init(filtered_users);
  client->login_screen()->LoadUsers(
      user_selection_screen->UpdateAndReturnUserListForMojo(), show_guest);
  user_selection_screen->SetUsersLoaded(true /*loaded*/);
}

void LoginDisplayMojo::OnPreferencesChanged() {
  NOTIMPLEMENTED();
}

void LoginDisplayMojo::SetUIEnabled(bool is_enabled) {
  NOTIMPLEMENTED();
}

void LoginDisplayMojo::ShowError(int error_msg_id,
                                 int login_attempts,
                                 HelpAppLauncher::HelpTopic help_topic_id) {
  NOTIMPLEMENTED();
}

void LoginDisplayMojo::ShowErrorScreen(LoginDisplay::SigninError error_id) {
  NOTIMPLEMENTED();
}

void LoginDisplayMojo::ShowPasswordChangedDialog(bool show_password_error,
                                                 const std::string& email) {
  NOTIMPLEMENTED();
}

void LoginDisplayMojo::ShowSigninUI(const std::string& email) {
  NOTIMPLEMENTED();
}

void LoginDisplayMojo::ShowWhitelistCheckFailedError() {
  NOTIMPLEMENTED();
}

void LoginDisplayMojo::ShowUnrecoverableCrypthomeErrorDialog() {
  NOTIMPLEMENTED();
}

void LoginDisplayMojo::OnUserImageChanged(const user_manager::User& user) {
  LoginScreenClient::Get()->login_screen()->SetAvatarForUser(
      user.GetAccountId(),
      UserSelectionScreen::BuildMojoUserAvatarForUser(&user));
}

}  // namespace chromeos
