// Copyright (c) 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CHROMEOS_LOGIN_SCREENS_RESET_SCREEN_H_
#define CHROME_BROWSER_CHROMEOS_LOGIN_SCREENS_RESET_SCREEN_H_

#include <set>
#include <string>

#include "base/compiler_specific.h"
#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/memory/weak_ptr.h"
#include "chrome/browser/chromeos/login/help_app_launcher.h"
#include "chrome/browser/chromeos/login/screens/base_screen.h"
#include "chrome/browser/chromeos/tpm_firmware_update.h"
#include "chromeos/dbus/update_engine_client.h"

class PrefRegistrySimple;

namespace chromeos {

class ErrorScreen;
class ResetView;

// Representation independent class that controls screen showing reset to users.
class ResetScreen : public BaseScreen, public UpdateEngineClient::Observer {
 public:
  ResetScreen(BaseScreenDelegate* base_screen_delegate, ResetView* view);
  ~ResetScreen() override;

  // Called when view is destroyed so there's no dead reference to it.
  void OnViewDestroyed(ResetView* view);

  // Registers Local State preferences.
  static void RegisterPrefs(PrefRegistrySimple* registry);

 private:
  // BaseScreen implementation:
  void Show() override;
  void Hide() override;
  void OnUserAction(const std::string& action_id) override;

  // UpdateEngineClient::Observer implementation:
  void UpdateStatusChanged(const UpdateEngineClient::Status& status) override;

  void OnRollbackCheck(bool can_rollback);
  void OnTPMFirmwareUpdateAvailableCheck(
      const std::set<tpm_firmware_update::Mode>& modes);

  enum State {
    STATE_RESTART_REQUIRED = 0,
    STATE_REVERT_PROMISE,
    STATE_POWERWASH_PROPOSAL,
    STATE_ERROR
  };

  void OnCancel();
  void OnPowerwash();
  void OnRestart();
  void OnToggleRollback();
  void OnShowConfirm();
  void OnConfirmationDismissed();

  void ShowHelpArticle(HelpAppLauncher::HelpTopic topic);

  // Returns an instance of the error screen.
  ErrorScreen* GetErrorScreen();

  ResetView* view_;

  // Help application used for help dialogs.
  scoped_refptr<HelpAppLauncher> help_app_;

  base::WeakPtrFactory<ResetScreen> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(ResetScreen);
};

}  // namespace chromeos

#endif  // CHROME_BROWSER_CHROMEOS_LOGIN_SCREENS_RESET_SCREEN_H_
