// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CHROMEOS_LOGIN_SCREENS_WRONG_HWID_SCREEN_VIEW_H_
#define CHROME_BROWSER_CHROMEOS_LOGIN_SCREENS_WRONG_HWID_SCREEN_VIEW_H_

#include <string>
#include "chrome/browser/chromeos/login/oobe_screen.h"

namespace chromeos {

// Interface between wrong HWID screen and its representation.
// Note, do not forget to call OnViewDestroyed in the dtor.
class WrongHWIDScreenView {
 public:
  // Allows us to get info from wrong HWID screen that we need.
  class Delegate {
   public:
    virtual ~Delegate() {}

    // Called when screen is exited.
    virtual void OnExit() = 0;

    // This method is called, when view is being destroyed. Note, if Delegate
    // is destroyed earlier then it has to call SetDelegate(NULL).
    virtual void OnViewDestroyed(WrongHWIDScreenView* view) = 0;
  };

  constexpr static OobeScreen kScreenId = OobeScreen::SCREEN_WRONG_HWID;

  virtual ~WrongHWIDScreenView() {}

  virtual void Show() = 0;
  virtual void Hide() = 0;
  virtual void SetDelegate(Delegate* delegate) = 0;
};

}  // namespace chromeos

#endif  // CHROME_BROWSER_CHROMEOS_LOGIN_SCREENS_WRONG_HWID_SCREEN_VIEW_H_
