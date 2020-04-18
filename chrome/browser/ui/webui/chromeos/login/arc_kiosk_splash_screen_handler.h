// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_WEBUI_CHROMEOS_LOGIN_ARC_KIOSK_SPLASH_SCREEN_HANDLER_H_
#define CHROME_BROWSER_UI_WEBUI_CHROMEOS_LOGIN_ARC_KIOSK_SPLASH_SCREEN_HANDLER_H_

#include <string>

#include "base/macros.h"
#include "chrome/browser/chromeos/login/screens/arc_kiosk_splash_screen_view.h"
#include "chrome/browser/ui/webui/chromeos/login/base_screen_handler.h"

namespace base {
class DictionaryValue;
}

namespace chromeos {

// A class that handles the WebUI hooks for the ARC kiosk splash screen.
class ArcKioskSplashScreenHandler : public BaseScreenHandler,
                                    public ArcKioskSplashScreenView {
 public:
  ArcKioskSplashScreenHandler();
  ~ArcKioskSplashScreenHandler() override;

 private:
  // BaseScreenHandler implementation:
  void DeclareLocalizedValues(
      ::login::LocalizedValuesBuilder* builder) override;
  void Initialize() override;

  // WebUIMessageHandler implementation:
  void RegisterMessages() override;

  // ArcKioskSplashScreenView implementation:
  void Show() override;
  void UpdateArcKioskState(ArcKioskState state) override;
  void SetDelegate(ArcKioskSplashScreenHandler::Delegate* delegate) override;

  void PopulateAppInfo(base::DictionaryValue* out_info);
  void SetLaunchText(const std::string& text);
  int GetProgressMessageFromState(ArcKioskState state);
  void HandleCancelArcKioskLaunch();

  ArcKioskSplashScreenHandler::Delegate* delegate_ = nullptr;
  bool show_on_init_ = false;

  DISALLOW_COPY_AND_ASSIGN(ArcKioskSplashScreenHandler);
};

}  // namespace chromeos

#endif  // CHROME_BROWSER_UI_WEBUI_CHROMEOS_LOGIN_ARC_KIOSK_SPLASH_SCREEN_HANDLER_H_
