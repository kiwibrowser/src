// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/ash/login_screen_client.h"

#include <utility>

#include "ash/public/interfaces/constants.mojom.h"
#include "chrome/browser/chromeos/login/lock/screen_locker.h"
#include "chrome/browser/chromeos/login/reauth_stats.h"
#include "chrome/browser/chromeos/login/ui/login_display_host.h"
#include "chrome/browser/chromeos/login/ui/user_adding_screen.h"
#include "chrome/browser/profiles/profile_metrics.h"
#include "chrome/browser/ui/ash/wallpaper_controller_client.h"
#include "chrome/browser/ui/webui/chromeos/login/l10n_util.h"
#include "components/user_manager/remove_user_delegate.h"
#include "content/public/common/service_manager_connection.h"
#include "services/service_manager/public/cpp/connector.h"

namespace {
LoginScreenClient* g_login_screen_client_instance = nullptr;
}  // namespace

LoginScreenClient::Delegate::Delegate() = default;
LoginScreenClient::Delegate::~Delegate() = default;

LoginScreenClient::LoginScreenClient()
    : binding_(this), weak_ptr_factory_(this) {
  content::ServiceManagerConnection::GetForProcess()
      ->GetConnector()
      ->BindInterface(ash::mojom::kServiceName, &login_screen_);
  // Register this object as the client interface implementation.
  ash::mojom::LoginScreenClientPtr client;
  binding_.Bind(mojo::MakeRequest(&client));
  login_screen_->SetClient(std::move(client));

  DCHECK(!g_login_screen_client_instance);
  g_login_screen_client_instance = this;
}

LoginScreenClient::~LoginScreenClient() {
  DCHECK_EQ(this, g_login_screen_client_instance);
  g_login_screen_client_instance = nullptr;
}

// static
bool LoginScreenClient::HasInstance() {
  return !!g_login_screen_client_instance;
}

// static
LoginScreenClient* LoginScreenClient::Get() {
  DCHECK(g_login_screen_client_instance);
  return g_login_screen_client_instance;
}

void LoginScreenClient::SetDelegate(Delegate* delegate) {
  delegate_ = delegate;
}

ash::mojom::LoginScreenPtr& LoginScreenClient::login_screen() {
  return login_screen_;
}

void LoginScreenClient::AuthenticateUser(const AccountId& account_id,
                                         const std::string& password,
                                         bool authenticated_by_pin,
                                         AuthenticateUserCallback callback) {
  if (delegate_) {
    delegate_->HandleAuthenticateUser(
        account_id, password, authenticated_by_pin, std::move(callback));
  } else {
    LOG(ERROR) << "Returning failed authentication attempt; no delegate";
    std::move(callback).Run(false);
  }
}

void LoginScreenClient::AttemptUnlock(const AccountId& account_id) {
  if (delegate_)
    delegate_->HandleAttemptUnlock(account_id);
}

void LoginScreenClient::HardlockPod(const AccountId& account_id) {
  if (delegate_)
    delegate_->HandleHardlockPod(account_id);
}

void LoginScreenClient::RecordClickOnLockIcon(const AccountId& account_id) {
  if (delegate_)
    delegate_->HandleRecordClickOnLockIcon(account_id);
}

void LoginScreenClient::OnFocusPod(const AccountId& account_id) {
  if (delegate_)
    delegate_->HandleOnFocusPod(account_id);
}

void LoginScreenClient::OnNoPodFocused() {
  if (delegate_)
    delegate_->HandleOnNoPodFocused();
}

void LoginScreenClient::FocusLockScreenApps(bool reverse) {
  // If delegate is not set, or it fails to handle focus request, call
  // |HandleFocusLeavingLockScreenApps| so the lock screen mojo service can
  // give focus to the next window in the tab order.
  if (!delegate_ || !delegate_->HandleFocusLockScreenApps(reverse))
    login_screen_->HandleFocusLeavingLockScreenApps(reverse);
}

void LoginScreenClient::ShowGaiaSignin(
    const base::Optional<AccountId>& account_id) {
  if (chromeos::LoginDisplayHost::default_host()) {
    chromeos::LoginDisplayHost::default_host()->UpdateGaiaDialogVisibility(
        true /*visible*/, account_id);
  }
}

void LoginScreenClient::OnRemoveUserWarningShown() {
  ProfileMetrics::LogProfileDeleteUser(
      ProfileMetrics::DELETE_PROFILE_USER_MANAGER_SHOW_WARNING);
}

void LoginScreenClient::RemoveUser(const AccountId& account_id) {
  ProfileMetrics::LogProfileDeleteUser(
      ProfileMetrics::DELETE_PROFILE_USER_MANAGER);
  user_manager::UserManager::Get()->RemoveUser(account_id,
                                               nullptr /*delegate*/);
}

void LoginScreenClient::LaunchPublicSession(const AccountId& account_id,
                                            const std::string& locale,
                                            const std::string& input_method) {
  if (delegate_)
    delegate_->HandleLaunchPublicSession(account_id, locale, input_method);
}

void LoginScreenClient::RequestPublicSessionKeyboardLayouts(
    const AccountId& account_id,
    const std::string& locale) {
  chromeos::GetKeyboardLayoutsForLocale(
      base::BindRepeating(&LoginScreenClient::SetPublicSessionKeyboardLayout,
                          weak_ptr_factory_.GetWeakPtr(), account_id, locale),
      locale);
}

void LoginScreenClient::LoadWallpaper(const AccountId& account_id) {
  WallpaperControllerClient::Get()->ShowUserWallpaper(account_id);
}

void LoginScreenClient::SignOutUser() {
  chromeos::ScreenLocker::default_screen_locker()->Signout();
}

void LoginScreenClient::CancelAddUser() {
  chromeos::UserAddingScreen::Get()->Cancel();
}

void LoginScreenClient::LoginAsGuest() {
  if (delegate_)
    delegate_->HandleLoginAsGuest();
}

void LoginScreenClient::OnMaxIncorrectPasswordAttempted(
    const AccountId& account_id) {
  RecordReauthReason(account_id,
                     chromeos::ReauthReason::INCORRECT_PASSWORD_ENTERED);
}

void LoginScreenClient::SetPublicSessionKeyboardLayout(
    const AccountId& account_id,
    const std::string& locale,
    std::unique_ptr<base::ListValue> keyboard_layouts) {
  std::vector<ash::mojom::InputMethodItemPtr> result;

  for (const auto& i : *keyboard_layouts) {
    const base::DictionaryValue* dictionary;
    if (!i.GetAsDictionary(&dictionary))
      continue;

    ash::mojom::InputMethodItemPtr input_method_item =
        ash::mojom::InputMethodItem::New();
    std::string ime_id;
    dictionary->GetString("value", &ime_id);
    input_method_item->ime_id = ime_id;

    std::string title;
    dictionary->GetString("title", &title);
    input_method_item->title = title;

    bool selected;
    dictionary->GetBoolean("selected", &selected);
    input_method_item->selected = selected;
    result.push_back(std::move(input_method_item));
  }
  login_screen_->SetPublicSessionKeyboardLayouts(account_id, locale,
                                                 std::move(result));
}
