// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CHROMEOS_LOGIN_SCREENS_KIOSK_AUTOLAUNCH_SCREEN_H_
#define CHROME_BROWSER_CHROMEOS_LOGIN_SCREENS_KIOSK_AUTOLAUNCH_SCREEN_H_

#include <string>

#include "base/compiler_specific.h"
#include "base/macros.h"
#include "chrome/browser/chromeos/login/screens/base_screen.h"
#include "chrome/browser/chromeos/login/screens/kiosk_autolaunch_screen_view.h"

namespace chromeos {

// Representation independent class that controls screen showing auto launch
// warning to users.
class KioskAutolaunchScreen : public BaseScreen,
                              public KioskAutolaunchScreenView::Delegate {
 public:
  KioskAutolaunchScreen(BaseScreenDelegate* base_screen_delegate,
                        KioskAutolaunchScreenView* view);
  ~KioskAutolaunchScreen() override;

  // BaseScreen implementation:
  void Show() override;
  void Hide() override {}

  // KioskAutolaunchScreenActor::Delegate implementation:
  void OnExit(bool confirmed) override;
  void OnViewDestroyed(KioskAutolaunchScreenView* view) override;

 private:
  KioskAutolaunchScreenView* view_;

  DISALLOW_COPY_AND_ASSIGN(KioskAutolaunchScreen);
};

}  // namespace chromeos

#endif  // CHROME_BROWSER_CHROMEOS_LOGIN_SCREENS_KIOSK_AUTOLAUNCH_SCREEN_H_
