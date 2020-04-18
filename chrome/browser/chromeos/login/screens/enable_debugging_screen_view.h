// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CHROMEOS_LOGIN_SCREENS_ENABLE_DEBUGGING_SCREEN_VIEW_H_
#define CHROME_BROWSER_CHROMEOS_LOGIN_SCREENS_ENABLE_DEBUGGING_SCREEN_VIEW_H_

#include <string>
#include "chrome/browser/chromeos/login/oobe_screen.h"

namespace chromeos {

// Interface between enable debugging screen and its representation.
// Note, do not forget to call OnViewDestroyed in the dtor.
class EnableDebuggingScreenView {
 public:
  // Allows us to get info from reset screen that we need.
  class Delegate {
   public:
    virtual ~Delegate() {}

    // Called when screen is exited.
    virtual void OnExit(bool success) = 0;

    // This method is called, when view is being destroyed. Note, if Delegate
    // is destroyed earlier then it has to call SetDelegate(nullptr).
    virtual void OnViewDestroyed(EnableDebuggingScreenView* view) = 0;
  };

  constexpr static OobeScreen kScreenId =
      OobeScreen::SCREEN_OOBE_ENABLE_DEBUGGING;

  virtual ~EnableDebuggingScreenView() {}

  virtual void Show() = 0;
  virtual void Hide() = 0;
  virtual void SetDelegate(Delegate* delegate) = 0;
};

}  // namespace chromeos

#endif  // CHROME_BROWSER_CHROMEOS_LOGIN_SCREENS_ENABLE_DEBUGGING_SCREEN_VIEW_H_
