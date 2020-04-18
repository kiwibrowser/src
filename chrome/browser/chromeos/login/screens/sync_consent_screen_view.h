// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CHROMEOS_LOGIN_SCREENS_SYNC_CONSENT_SCREEN_VIEW_H_
#define CHROME_BROWSER_CHROMEOS_LOGIN_SCREENS_SYNC_CONSENT_SCREEN_VIEW_H_

#include "chrome/browser/chromeos/login/oobe_screen.h"

namespace chromeos {

class SyncConsentScreen;

// Interface for dependency injection between SyncConsentScreen and its
// WebUI representation.
class SyncConsentScreenView {
 public:
  constexpr static OobeScreen kScreenId = OobeScreen::SCREEN_SYNC_CONSENT;

  virtual ~SyncConsentScreenView() = default;

  // Sets screen this view belongs to.
  virtual void Bind(SyncConsentScreen* screen) = 0;

  // Shows the contents of the screen.
  virtual void Show() = 0;

  // Hides the contents of the screen.
  virtual void Hide() = 0;

  // Controls if the loading throbber is visible. This is used when
  // SyncScreenBehavior is unknown.
  virtual void SetThrobberVisible(bool visible) = 0;
};

}  // namespace chromeos

#endif  // CHROME_BROWSER_CHROMEOS_LOGIN_SCREENS_SYNC_CONSENT_SCREEN_VIEW_H_
