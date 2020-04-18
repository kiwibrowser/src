// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CHROMEOS_LOGIN_SCREENS_ENCRYPTION_MIGRATION_SCREEN_VIEW_H_
#define CHROME_BROWSER_CHROMEOS_LOGIN_SCREENS_ENCRYPTION_MIGRATION_SCREEN_VIEW_H_

#include "base/callback_forward.h"
#include "chrome/browser/chromeos/login/oobe_screen.h"
#include "chrome/browser/chromeos/login/screens/encryption_migration_mode.h"

namespace chromeos {

class UserContext;

class EncryptionMigrationScreenView {
 public:
  using ContinueLoginCallback = base::OnceCallback<void(const UserContext&)>;
  using RestartLoginCallback = base::OnceCallback<void(const UserContext&)>;

  class Delegate {
   public:
    virtual ~Delegate() {}

    // Called when screen is exited.
    virtual void OnExit() = 0;

    // This method is called, when view is being destroyed. Note, if Delegate is
    // destroyed earlier then it has to call SetDelegate(NULL).
    virtual void OnViewDestroyed(EncryptionMigrationScreenView* view) = 0;
  };

  constexpr static OobeScreen kScreenId =
      OobeScreen::SCREEN_ENCRYPTION_MIGRATION;

  virtual ~EncryptionMigrationScreenView() {}

  virtual void Show() = 0;
  virtual void Hide() = 0;
  virtual void SetDelegate(Delegate* delegate) = 0;
  virtual void SetUserContext(const UserContext& user_context) = 0;
  virtual void SetMode(EncryptionMigrationMode mode) = 0;
  virtual void SetContinueLoginCallback(ContinueLoginCallback callback) = 0;
  virtual void SetRestartLoginCallback(RestartLoginCallback callback) = 0;
  virtual void SetupInitialView() = 0;
};

}  // namespace chromeos

#endif  // CHROME_BROWSER_CHROMEOS_LOGIN_SCREENS_ENCRYPTION_MIGRATION_SCREEN_VIEW_H_
