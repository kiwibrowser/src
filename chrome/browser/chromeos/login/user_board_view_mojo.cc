// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/login/user_board_view_mojo.h"

#include <utility>

#include "chrome/browser/chromeos/login/lock_screen_utils.h"
#include "chrome/browser/ui/ash/login_screen_client.h"

namespace chromeos {

namespace {

ash::mojom::EasyUnlockIconId GetEasyUnlockIconIdFromUserPodCustomIconId(
    proximity_auth::ScreenlockBridge::UserPodCustomIcon icon) {
  switch (icon) {
    case proximity_auth::ScreenlockBridge::USER_POD_CUSTOM_ICON_NONE:
      return ash::mojom::EasyUnlockIconId::NONE;
    case proximity_auth::ScreenlockBridge::USER_POD_CUSTOM_ICON_HARDLOCKED:
      return ash::mojom::EasyUnlockIconId::HARDLOCKED;
    case proximity_auth::ScreenlockBridge::USER_POD_CUSTOM_ICON_LOCKED:
      return ash::mojom::EasyUnlockIconId::LOCKED;
    case proximity_auth::ScreenlockBridge::
        USER_POD_CUSTOM_ICON_LOCKED_TO_BE_ACTIVATED:
      return ash::mojom::EasyUnlockIconId::LOCKED_TO_BE_ACTIVATED;
    case proximity_auth::ScreenlockBridge::
        USER_POD_CUSTOM_ICON_LOCKED_WITH_PROXIMITY_HINT:
      return ash::mojom::EasyUnlockIconId::LOCKED_WITH_PROXIMITY_HINT;
    case proximity_auth::ScreenlockBridge::USER_POD_CUSTOM_ICON_UNLOCKED:
      return ash::mojom::EasyUnlockIconId::UNLOCKED;
    case proximity_auth::ScreenlockBridge::USER_POD_CUSTOM_ICON_SPINNER:
      return ash::mojom::EasyUnlockIconId::SPINNER;
  }
}

// Converts parameters to a mojo struct that can be sent to the
// screenlock view-based UI.
ash::mojom::EasyUnlockIconOptionsPtr ToEasyUnlockIconOptionsPtr(
    const proximity_auth::ScreenlockBridge::UserPodCustomIconOptions&
        icon_options) {
  ash::mojom::EasyUnlockIconOptionsPtr options =
      ash::mojom::EasyUnlockIconOptions::New();
  options->icon =
      GetEasyUnlockIconIdFromUserPodCustomIconId(icon_options.icon());

  if (!icon_options.tooltip().empty()) {
    options->tooltip = icon_options.tooltip();
    options->autoshow_tooltip = icon_options.autoshow_tooltip();
  }

  if (!icon_options.aria_label().empty())
    options->aria_label = icon_options.aria_label();

  if (icon_options.hardlock_on_click())
    options->hardlock_on_click = true;

  if (icon_options.is_trial_run())
    options->is_trial_run = true;

  return options;
}

}  // namespace

UserBoardViewMojo::UserBoardViewMojo() : weak_factory_(this) {}

UserBoardViewMojo::~UserBoardViewMojo() = default;

void UserBoardViewMojo::SetPublicSessionDisplayName(
    const AccountId& account_id,
    const std::string& display_name) {
  LoginScreenClient::Get()->login_screen()->SetPublicSessionDisplayName(
      account_id, display_name);
}

void UserBoardViewMojo::SetPublicSessionLocales(
    const AccountId& account_id,
    std::unique_ptr<base::ListValue> locales,
    const std::string& default_locale,
    bool multiple_recommended_locales) {
  DCHECK(locales);
  LoginScreenClient::Get()->login_screen()->SetPublicSessionLocales(
      account_id,
      lock_screen_utils::FromListValueToLocaleItem(std::move(locales)),
      default_locale, multiple_recommended_locales);

  // Send a request to get keyboard layouts for |default_locale|.
  LoginScreenClient::Get()->RequestPublicSessionKeyboardLayouts(account_id,
                                                                default_locale);
}

void UserBoardViewMojo::ShowUserPodCustomIcon(
    const AccountId& account_id,
    const proximity_auth::ScreenlockBridge::UserPodCustomIconOptions&
        icon_options) {
  ash::mojom::EasyUnlockIconOptionsPtr icon =
      ToEasyUnlockIconOptionsPtr(icon_options);
  if (!icon)
    return;
  LoginScreenClient::Get()->login_screen()->ShowUserPodCustomIcon(
      account_id, std::move(icon));
}

void UserBoardViewMojo::HideUserPodCustomIcon(const AccountId& account_id) {
  LoginScreenClient::Get()->login_screen()->HideUserPodCustomIcon(account_id);
}

void UserBoardViewMojo::SetAuthType(const AccountId& account_id,
                                    proximity_auth::mojom::AuthType auth_type,
                                    const base::string16& initial_value) {
  LoginScreenClient::Get()->login_screen()->SetAuthType(account_id, auth_type,
                                                        initial_value);
}

base::WeakPtr<UserBoardView> UserBoardViewMojo::GetWeakPtr() {
  return weak_factory_.GetWeakPtr();
}

}  // namespace chromeos
