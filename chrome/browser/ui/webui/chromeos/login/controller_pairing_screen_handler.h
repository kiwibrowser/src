// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_WEBUI_CHROMEOS_LOGIN_CONTROLLER_PAIRING_SCREEN_HANDLER_H_
#define CHROME_BROWSER_UI_WEBUI_CHROMEOS_LOGIN_CONTROLLER_PAIRING_SCREEN_HANDLER_H_

#include "base/macros.h"
#include "chrome/browser/chromeos/login/screens/controller_pairing_screen_view.h"
#include "chrome/browser/ui/webui/chromeos/login/base_screen_handler.h"

namespace chromeos {

class ControllerPairingScreenHandler : public ControllerPairingScreenView,
                                       public BaseScreenHandler {
 public:
  ControllerPairingScreenHandler();
  ~ControllerPairingScreenHandler() override;

 private:
  void HandleUserActed(const std::string& action);
  void HandleContextChanged(const base::DictionaryValue* diff);

  // Overridden from BaseScreenHandler:
  void Initialize() override;
  void DeclareLocalizedValues(
      ::login::LocalizedValuesBuilder* builder) override;

  // Overridden from content::WebUIMessageHandler:
  void RegisterMessages() override;

  // Overridden from ControllerPairingScreenActor:
  void Show() override;
  void Hide() override;
  void SetDelegate(Delegate* delegate) override;
  void OnContextChanged(const base::DictionaryValue& diff) override;
  content::BrowserContext* GetBrowserContext() override;

  ControllerPairingScreenView::Delegate* delegate_ = nullptr;
  bool show_on_init_ = false;

  DISALLOW_COPY_AND_ASSIGN(ControllerPairingScreenHandler);
};

}  // namespace chromeos

#endif  // CHROME_BROWSER_UI_WEBUI_CHROMEOS_LOGIN_CONTROLLER_PAIRING_SCREEN_HANDLER_H_
