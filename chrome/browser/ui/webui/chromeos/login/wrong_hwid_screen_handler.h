// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_WEBUI_CHROMEOS_LOGIN_WRONG_HWID_SCREEN_HANDLER_H_
#define CHROME_BROWSER_UI_WEBUI_CHROMEOS_LOGIN_WRONG_HWID_SCREEN_HANDLER_H_

#include "base/compiler_specific.h"
#include "base/macros.h"
#include "chrome/browser/chromeos/login/screens/wrong_hwid_screen_view.h"
#include "chrome/browser/ui/webui/chromeos/login/base_screen_handler.h"
#include "content/public/browser/web_ui.h"

namespace chromeos {

// WebUI implementation of WrongHWIDScreenActor.
class WrongHWIDScreenHandler : public WrongHWIDScreenView,
                               public BaseScreenHandler {
 public:
  WrongHWIDScreenHandler();
  ~WrongHWIDScreenHandler() override;

  // WrongHWIDScreenActor implementation:
  void Show() override;
  void Hide() override;
  void SetDelegate(Delegate* delegate) override;

  // BaseScreenHandler implementation:
  void DeclareLocalizedValues(
      ::login::LocalizedValuesBuilder* builder) override;
  void Initialize() override;

  // WebUIMessageHandler implementation:
  void RegisterMessages() override;

 private:
  // JS messages handlers.
  void HandleOnSkip();

  Delegate* delegate_ = nullptr;

  // Keeps whether screen should be shown right after initialization.
  bool show_on_init_ = false;

  DISALLOW_COPY_AND_ASSIGN(WrongHWIDScreenHandler);
};

}  // namespace chromeos

#endif  // CHROME_BROWSER_UI_WEBUI_CHROMEOS_LOGIN_WRONG_HWID_SCREEN_HANDLER_H_

