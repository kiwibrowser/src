// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CHROMEOS_LOGIN_SCREENS_ENCRYPTION_MIGRATION_SCREEN_H_
#define CHROME_BROWSER_CHROMEOS_LOGIN_SCREENS_ENCRYPTION_MIGRATION_SCREEN_H_

#include "base/callback_forward.h"
#include "chrome/browser/chromeos/login/screens/base_screen.h"
#include "chrome/browser/chromeos/login/screens/encryption_migration_mode.h"
#include "chrome/browser/chromeos/login/screens/encryption_migration_screen_view.h"

namespace chromeos {

class UserContext;

class EncryptionMigrationScreen
    : public BaseScreen,
      public EncryptionMigrationScreenView::Delegate {
 public:
  using ContinueLoginCallback = base::OnceCallback<void(const UserContext&)>;
  using RestartLoginCallback = base::OnceCallback<void(const UserContext&)>;

  EncryptionMigrationScreen(BaseScreenDelegate* base_screen_delegate,
                            EncryptionMigrationScreenView* view);
  ~EncryptionMigrationScreen() override;

  // BaseScreen implementation:
  void Show() override;
  void Hide() override;

  // EncryptionMigrationScreenView::Delegate implementation:
  void OnExit() override;
  void OnViewDestroyed(EncryptionMigrationScreenView* view) override;

  // Sets the UserContext for a user whose cryptohome should be migrated.
  void SetUserContext(const UserContext& user_context);

  // Sets the migration mode.
  void SetMode(EncryptionMigrationMode mode);

  // Sets a callback, which should be called when the user want to log in to the
  // session from the migration UI.
  void SetContinueLoginCallback(ContinueLoginCallback callback);

  // Sets a callback, which should be called when the user should re-enter their
  // password.
  void SetRestartLoginCallback(RestartLoginCallback callback);

  // Setup the initial view in the migration UI.
  // This should be called after other state like UserContext, etc... are set.
  void SetupInitialView();

 private:
  EncryptionMigrationScreenView* view_;

  DISALLOW_COPY_AND_ASSIGN(EncryptionMigrationScreen);
};

}  // namespace chromeos

#endif  // CHROME_BROWSER_CHROMEOS_LOGIN_SCREENS_ENCRYPTION_MIGRATION_SCREEN_H_
