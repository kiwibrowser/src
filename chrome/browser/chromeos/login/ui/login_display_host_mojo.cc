// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/login/ui/login_display_host_mojo.h"

#include <string>
#include <utility>

#include "base/threading/thread_task_runner_handle.h"
#include "chrome/browser/chromeos/login/existing_user_controller.h"
#include "chrome/browser/chromeos/login/screens/chrome_user_selection_screen.h"
#include "chrome/browser/chromeos/login/screens/gaia_view.h"
#include "chrome/browser/chromeos/login/ui/gaia_dialog_delegate.h"
#include "chrome/browser/chromeos/login/ui/login_display.h"
#include "chrome/browser/chromeos/login/ui/login_display_mojo.h"
#include "chrome/browser/chromeos/login/user_board_view_mojo.h"
#include "chrome/browser/chromeos/login/wizard_controller.h"
#include "chrome/browser/ui/ash/wallpaper_controller_client.h"
#include "chrome/browser/ui/webui/chromeos/login/signin_screen_handler.h"
#include "chromeos/login/auth/user_context.h"
#include "components/user_manager/user_names.h"

namespace chromeos {

namespace {
constexpr char kLoginDisplay[] = "login";
}  // namespace

LoginDisplayHostMojo::LoginDisplayHostMojo()
    : user_board_view_mojo_(std::make_unique<UserBoardViewMojo>()),
      user_selection_screen_(
          std::make_unique<ChromeUserSelectionScreen>(kLoginDisplay)),
      weak_factory_(this) {
  user_selection_screen_->SetView(user_board_view_mojo_.get());

  // Preload the WebUI for post-login screens.
  InitWidgetAndView();
}

LoginDisplayHostMojo::~LoginDisplayHostMojo() {
  LoginScreenClient::Get()->SetDelegate(nullptr);
  if (dialog_)
    dialog_->Close();
}

void LoginDisplayHostMojo::OnDialogDestroyed(const GaiaDialogDelegate* dialog) {
  if (dialog == dialog_) {
    dialog_ = nullptr;
    wizard_controller_.reset();
  }
}

void LoginDisplayHostMojo::SetUsers(const user_manager::UserList& users) {
  users_ = users;
  if (GetOobeUI())
    GetOobeUI()->SetLoginUserCount(users_.size());
}

LoginDisplay* LoginDisplayHostMojo::CreateLoginDisplay(
    LoginDisplay::Delegate* delegate) {
  user_selection_screen_->SetLoginDisplayDelegate(delegate);
  return new LoginDisplayMojo(delegate, this);
}

gfx::NativeWindow LoginDisplayHostMojo::GetNativeWindow() const {
  NOTIMPLEMENTED();
  return nullptr;
}

OobeUI* LoginDisplayHostMojo::GetOobeUI() const {
  if (!dialog_)
    return nullptr;
  return dialog_->GetOobeUI();
}

WebUILoginView* LoginDisplayHostMojo::GetWebUILoginView() const {
  NOTREACHED();
  return nullptr;
}

void LoginDisplayHostMojo::OnFinalize() {
  if (dialog_)
    dialog_->Close();

  base::ThreadTaskRunnerHandle::Get()->DeleteSoon(FROM_HERE, this);
}

void LoginDisplayHostMojo::SetStatusAreaVisible(bool visible) {
  NOTIMPLEMENTED();
}

void LoginDisplayHostMojo::StartWizard(OobeScreen first_screen) {
  DCHECK(GetOobeUI());

  // Dtor of the old WizardController should be called before ctor of the
  // new one to ensure only one |ExistingUserController| instance at a time.
  wizard_controller_.reset();
  wizard_controller_.reset(new WizardController(this, GetOobeUI()));
  wizard_controller_->Init(first_screen);

  // Post login screens should not be closable by escape key.
  dialog_->Show(false /*closable_by_esc*/);
}

WizardController* LoginDisplayHostMojo::GetWizardController() {
  NOTIMPLEMENTED();
  return nullptr;
}

void LoginDisplayHostMojo::OnStartUserAdding() {
  NOTIMPLEMENTED();
}

void LoginDisplayHostMojo::CancelUserAdding() {
  NOTIMPLEMENTED();
}

void LoginDisplayHostMojo::OnStartSignInScreen(
    const LoginScreenContext& context) {
  // This function may be called early in startup flow, before LoginScreenClient
  // has been initialized. Wait until LoginScreenClient is initialized as it is
  // a common dependency.
  if (!LoginScreenClient::HasInstance()) {
    // TODO(jdufault): Add a timeout here / make sure we do not post infinitely.
    base::ThreadTaskRunnerHandle::Get()->PostTask(
        FROM_HERE, base::BindOnce(&LoginDisplayHostMojo::OnStartSignInScreen,
                                  weak_factory_.GetWeakPtr(), context));
    return;
  }

  // There can only be one |ExistingUserController| instance at a time.
  existing_user_controller_.reset();
  existing_user_controller_ = std::make_unique<ExistingUserController>(this);

  // We need auth attempt results to notify views-based lock screen.
  existing_user_controller_->set_login_status_consumer(this);

  // Load the UI.
  existing_user_controller_->Init(user_manager::UserManager::Get()->GetUsers());

  user_selection_screen_->InitEasyUnlock();
}

void LoginDisplayHostMojo::OnPreferencesChanged() {
  NOTIMPLEMENTED();
}

void LoginDisplayHostMojo::OnStartAppLaunch() {
  NOTIMPLEMENTED();
}

void LoginDisplayHostMojo::OnStartArcKiosk() {
  NOTIMPLEMENTED();
}

void LoginDisplayHostMojo::OnBrowserCreated() {
  NOTIMPLEMENTED();
}

void LoginDisplayHostMojo::StartVoiceInteractionOobe() {
  NOTIMPLEMENTED();
}

bool LoginDisplayHostMojo::IsVoiceInteractionOobe() {
  NOTIMPLEMENTED();
  return false;
}

void LoginDisplayHostMojo::UpdateGaiaDialogVisibility(
    bool visible,
    const base::Optional<AccountId>& account) {
  DCHECK(dialog_);

  if (visible) {
    if (account) {
      // Make sure gaia displays |account| if requested.
      GetOobeUI()->GetGaiaScreenView()->ShowGaiaAsync(account);
      LoginDisplayHost::default_host()->LoadWallpaper(account.value());
    } else {
      LoginDisplayHost::default_host()->LoadSigninWallpaper();
    }

    dialog_->Show(true /*closable_by_esc*/);
    return;
  }
  // Show the wallpaper of the focused user pod when the dialog is hidden.
  LoginDisplayHost::default_host()->LoadWallpaper(focused_pod_account_id_);

  if (users_.empty() && GetOobeUI()) {
    // The dialog can not be closed if there is no user on the login screen.
    // Refresh the dialog instead.
    GetOobeUI()->GetGaiaScreenView()->ShowGaiaAsync(base::nullopt);
    return;
  }

  dialog_->Hide();
}

void LoginDisplayHostMojo::UpdateGaiaDialogSize(int width, int height) {
  if (dialog_)
    dialog_->SetSize(width, height);
}

const user_manager::UserList LoginDisplayHostMojo::GetUsers() {
  return users_;
}

void LoginDisplayHostMojo::HandleAuthenticateUser(
    const AccountId& account_id,
    const std::string& password,
    bool authenticated_by_pin,
    AuthenticateUserCallback callback) {
  DCHECK(!authenticated_by_pin);
  DCHECK_EQ(account_id.GetUserEmail(),
            gaia::SanitizeEmail(account_id.GetUserEmail()));

  on_authenticated_ = std::move(callback);

  const user_manager::User* const user =
      user_manager::UserManager::Get()->FindUser(account_id);
  DCHECK(user);
  UserContext user_context(*user);
  user_context.SetKey(
      Key(chromeos::Key::KEY_TYPE_PASSWORD_PLAIN, "" /*salt*/, password));
  if (account_id.GetAccountType() == AccountType::ACTIVE_DIRECTORY &&
      (user_context.GetUserType() !=
       user_manager::UserType::USER_TYPE_ACTIVE_DIRECTORY)) {
    LOG(FATAL) << "Incorrect Active Directory user type "
               << user_context.GetUserType();
  }

  existing_user_controller_->Login(user_context, chromeos::SigninSpecifics());
}

void LoginDisplayHostMojo::HandleAttemptUnlock(const AccountId& account_id) {
  user_selection_screen_->AttemptEasyUnlock(account_id);
}

void LoginDisplayHostMojo::HandleHardlockPod(const AccountId& account_id) {
  user_selection_screen_->HardLockPod(account_id);
}

void LoginDisplayHostMojo::HandleRecordClickOnLockIcon(
    const AccountId& account_id) {
  user_selection_screen_->RecordClickOnLockIcon(account_id);
}

void LoginDisplayHostMojo::HandleOnFocusPod(const AccountId& account_id) {
  // TODO(jdufault): Share common code between this and
  // ViewsScreenLocker::HandleOnFocusPod See https://crbug.com/831787.
  proximity_auth::ScreenlockBridge::Get()->SetFocusedUser(account_id);
  user_selection_screen_->CheckUserStatus(account_id);
  WallpaperControllerClient::Get()->ShowUserWallpaper(account_id);
  focused_pod_account_id_ = account_id;
}

void LoginDisplayHostMojo::HandleOnNoPodFocused() {
  NOTIMPLEMENTED();
}

bool LoginDisplayHostMojo::HandleFocusLockScreenApps(bool reverse) {
  NOTREACHED();
  return false;
}

void LoginDisplayHostMojo::HandleLoginAsGuest() {
  existing_user_controller_->Login(UserContext(user_manager::USER_TYPE_GUEST,
                                               user_manager::GuestAccountId()),
                                   chromeos::SigninSpecifics());
}

void LoginDisplayHostMojo::HandleLaunchPublicSession(
    const AccountId& account_id,
    const std::string& locale,
    const std::string& input_method) {
  UserContext context(user_manager::USER_TYPE_PUBLIC_ACCOUNT, account_id);
  context.SetPublicSessionLocale(locale);
  context.SetPublicSessionInputMethod(input_method);
  existing_user_controller_->Login(context, chromeos::SigninSpecifics());
}

void LoginDisplayHostMojo::OnAuthFailure(const AuthFailure& error) {
  if (on_authenticated_)
    std::move(on_authenticated_).Run(false);
}

void LoginDisplayHostMojo::OnAuthSuccess(const UserContext& user_context) {
  if (on_authenticated_)
    std::move(on_authenticated_).Run(true);
}

void LoginDisplayHostMojo::InitWidgetAndView() {
  if (dialog_)
    return;

  dialog_ = new GaiaDialogDelegate(weak_factory_.GetWeakPtr());
  dialog_->Init();
}

}  // namespace chromeos
