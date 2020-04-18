// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CHROMEOS_LOGIN_SCREENS_ARC_TERMS_OF_SERVICE_SCREEN_VIEW_H_
#define CHROME_BROWSER_CHROMEOS_LOGIN_SCREENS_ARC_TERMS_OF_SERVICE_SCREEN_VIEW_H_

#include <string>

#include "base/macros.h"
#include "chrome/browser/chromeos/login/oobe_screen.h"

namespace chromeos {

class ArcTermsOfServiceScreenViewObserver;

// Interface for dependency injection between TermsOfServiceScreen and its
// WebUI representation.
class ArcTermsOfServiceScreenView {
 public:
  constexpr static OobeScreen kScreenId =
      OobeScreen::SCREEN_ARC_TERMS_OF_SERVICE;

  virtual ~ArcTermsOfServiceScreenView() = default;

  // Adds/Removes observer for view.
  virtual void AddObserver(ArcTermsOfServiceScreenViewObserver* observer) = 0;
  virtual void RemoveObserver(
      ArcTermsOfServiceScreenViewObserver* observer) = 0;

  // Shows the contents of the screen.
  virtual void Show() = 0;

  // Hides the contents of the screen.
  virtual void Hide() = 0;

 protected:
  ArcTermsOfServiceScreenView() = default;

 private:
  DISALLOW_COPY_AND_ASSIGN(ArcTermsOfServiceScreenView);
};

}  // namespace chromeos

#endif  // CHROME_BROWSER_CHROMEOS_LOGIN_SCREENS_ARC_TERMS_OF_SERVICE_SCREEN_VIEW_H_
