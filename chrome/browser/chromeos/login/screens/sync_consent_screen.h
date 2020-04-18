// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CHROMEOS_LOGIN_SCREENS_SYNC_CONSENT_SCREEN_H_
#define CHROME_BROWSER_CHROMEOS_LOGIN_SCREENS_SYNC_CONSENT_SCREEN_H_

#include <memory>
#include <string>

#include "base/macros.h"
#include "chrome/browser/chromeos/login/screens/base_screen.h"
#include "chrome/browser/chromeos/login/screens/sync_consent_screen_view.h"
#include "components/sync/driver/sync_service_observer.h"
#include "components/user_manager/user.h"

class Profile;

namespace chromeos {

class BaseScreenDelegate;

// This is Sync settings screen that is displayed as a part of user first
// sign-in flow.
class SyncConsentScreen : public BaseScreen,
                          public syncer::SyncServiceObserver {
 private:
  enum SyncScreenBehavior {
    UNKNOWN,  // Not yet known.
    SHOW,     // Screen should be shown.
    SKIP      // Skip screen for this user.
  };

 public:
  SyncConsentScreen(BaseScreenDelegate* base_screen_delegate,
                    SyncConsentScreenView* view);
  ~SyncConsentScreen() override;

  // BaseScreen:
  void Show() override;
  void Hide() override;
  void OnUserAction(const std::string& action_id) override;

  // syncer::SyncServiceObserver:
  void OnStateChanged(syncer::SyncService* sync) override;

 private:
  // Returns new SyncScreenBehavior value.
  SyncScreenBehavior GetSyncScreenBehavior() const;

  // Calculates updated |behavior_| and performs required update actions.
  void UpdateScreen();

  // Controls screen appearance.
  // Spinner is shown until sync status has been decided.
  SyncScreenBehavior behavior_ = UNKNOWN;

  SyncConsentScreenView* const view_;

  // Primary user ind his Profile (if screen is shown).
  const user_manager::User* user_ = nullptr;
  Profile* profile_ = nullptr;

  // True when screen is shown.
  bool shown_ = false;

  DISALLOW_COPY_AND_ASSIGN(SyncConsentScreen);
};

}  // namespace chromeos

#endif  // CHROME_BROWSER_CHROMEOS_LOGIN_SCREENS_SYNC_CONSENT_SCREEN_H_
