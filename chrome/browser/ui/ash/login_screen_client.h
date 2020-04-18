// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_ASH_LOGIN_SCREEN_CLIENT_H_
#define CHROME_BROWSER_UI_ASH_LOGIN_SCREEN_CLIENT_H_

#include "ash/public/interfaces/login_screen.mojom.h"
#include "base/macros.h"
#include "mojo/public/cpp/bindings/binding.h"
#include "ui/base/ime/chromeos/input_method_manager.h"

using AuthenticateUserCallback =
    ash::mojom::LoginScreenClient::AuthenticateUserCallback;

// Handles method calls sent from ash to chrome. Also sends messages from chrome
// to ash.
class LoginScreenClient : public ash::mojom::LoginScreenClient {
 public:
  // Handles method calls coming from ash into chrome.
  class Delegate {
   public:
    Delegate();
    virtual ~Delegate();
    virtual void HandleAuthenticateUser(
        const AccountId& account_id,
        const std::string& hashed_password,
        bool authenticated_by_pin,
        AuthenticateUserCallback callback) = 0;
    virtual void HandleAttemptUnlock(const AccountId& account_id) = 0;
    virtual void HandleHardlockPod(const AccountId& account_id) = 0;
    virtual void HandleRecordClickOnLockIcon(const AccountId& account_id) = 0;
    virtual void HandleOnFocusPod(const AccountId& account_id) = 0;
    virtual void HandleOnNoPodFocused() = 0;
    // Handles request to focus a lock screen app window. Returns whether the
    // focus has been handed over to a lock screen app. For example, this might
    // fail if a hander for lock screen apps focus has not been set.
    virtual bool HandleFocusLockScreenApps(bool reverse) = 0;
    virtual void HandleLoginAsGuest() = 0;
    virtual void HandleLaunchPublicSession(const AccountId& account_id,
                                           const std::string& locale,
                                           const std::string& input_method) = 0;

   private:
    DISALLOW_COPY_AND_ASSIGN(Delegate);
  };

  LoginScreenClient();
  ~LoginScreenClient() override;
  static bool HasInstance();
  static LoginScreenClient* Get();

  // Set the object which will handle calls coming from ash.
  void SetDelegate(Delegate* delegate);

  // Returns an object which can be used to make calls to ash.
  ash::mojom::LoginScreenPtr& login_screen();

  // ash::mojom::LoginScreenClient:
  void AuthenticateUser(const AccountId& account_id,
                        const std::string& password,
                        bool authenticated_by_pin,
                        AuthenticateUserCallback callback) override;
  void AttemptUnlock(const AccountId& account_id) override;
  void HardlockPod(const AccountId& account_id) override;
  void RecordClickOnLockIcon(const AccountId& account_id) override;
  void OnFocusPod(const AccountId& account_id) override;
  void OnNoPodFocused() override;
  void LoadWallpaper(const AccountId& account_id) override;
  void SignOutUser() override;
  void CancelAddUser() override;
  void LoginAsGuest() override;
  void OnMaxIncorrectPasswordAttempted(const AccountId& account_id) override;
  void FocusLockScreenApps(bool reverse) override;
  void ShowGaiaSignin(const base::Optional<AccountId>& account_id) override;
  void OnRemoveUserWarningShown() override;
  void RemoveUser(const AccountId& account_id) override;
  void LaunchPublicSession(const AccountId& account_id,
                           const std::string& locale,
                           const std::string& input_method) override;
  void RequestPublicSessionKeyboardLayouts(const AccountId& account_id,
                                           const std::string& locale) override;

 private:
  void SetPublicSessionKeyboardLayout(
      const AccountId& account_id,
      const std::string& locale,
      std::unique_ptr<base::ListValue> keyboard_layouts);

  // Lock screen mojo service in ash.
  ash::mojom::LoginScreenPtr login_screen_;

  // Binds this object to the client interface.
  mojo::Binding<ash::mojom::LoginScreenClient> binding_;
  Delegate* delegate_ = nullptr;

  base::WeakPtrFactory<LoginScreenClient> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(LoginScreenClient);
};

#endif  // CHROME_BROWSER_UI_ASH_LOGIN_SCREEN_CLIENT_H_
