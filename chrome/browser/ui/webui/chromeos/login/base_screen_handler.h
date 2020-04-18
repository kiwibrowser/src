// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_WEBUI_CHROMEOS_LOGIN_BASE_SCREEN_HANDLER_H_
#define CHROME_BROWSER_UI_WEBUI_CHROMEOS_LOGIN_BASE_SCREEN_HANDLER_H_

#include "base/macros.h"
#include "chrome/browser/chromeos/login/oobe_screen.h"
#include "chrome/browser/ui/webui/chromeos/login/base_webui_handler.h"

namespace chromeos {

// Base class for the OOBE/Login WebUI handlers which provide methods specific
// to a particular OobeScreen.
class BaseScreenHandler : public BaseWebUIHandler {
 public:
  explicit BaseScreenHandler(OobeScreen oobe_screen);
  ~BaseScreenHandler() override;

  OobeScreen oobe_screen() const { return oobe_screen_; }

 private:
  // OobeScreen that this handler corresponds to.
  OobeScreen oobe_screen_ = OobeScreen::SCREEN_UNKNOWN;

  DISALLOW_COPY_AND_ASSIGN(BaseScreenHandler);
};

}  // namespace chromeos

#endif  // CHROME_BROWSER_UI_WEBUI_CHROMEOS_LOGIN_BASE_SCREEN_HANDLER_H_
