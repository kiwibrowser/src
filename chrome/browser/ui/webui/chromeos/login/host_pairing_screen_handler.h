// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_WEBUI_CHROMEOS_LOGIN_HOST_PAIRING_SCREEN_HANDLER_H_
#define CHROME_BROWSER_UI_WEBUI_CHROMEOS_LOGIN_HOST_PAIRING_SCREEN_HANDLER_H_

#include "base/macros.h"
#include "chrome/browser/chromeos/login/screens/host_pairing_screen_view.h"
#include "chrome/browser/ui/webui/chromeos/login/base_screen_handler.h"
#include "components/login/screens/screen_context.h"

namespace chromeos {

class HostPairingScreenHandler : public HostPairingScreenView,
                                 public BaseScreenHandler {
 public:
  HostPairingScreenHandler();
  ~HostPairingScreenHandler() override;

 private:
  void HandleContextReady();

  // Overridden from BaseScreenHandler:
  void Initialize() override;
  void DeclareLocalizedValues(
      ::login::LocalizedValuesBuilder* builder) override;

  // Overridden from content::WebUIMessageHandler:
  void RegisterMessages() override;

  // Overridden from HostPairingScreenActor:
  void Show() override;
  void Hide() override;
  void SetDelegate(Delegate* delegate) override;
  void OnContextChanged(const base::DictionaryValue& diff) override;
  std::string GetErrorStringFromAuthError(
      const GoogleServiceAuthError& error) override;
  std::string GetErrorStringFromEnrollmentError(
      policy::EnrollmentStatus status) override;
  std::string GetErrorStringFromOtherError(
      EnterpriseEnrollmentHelper::OtherError error) override;

  HostPairingScreenView::Delegate* delegate_ = nullptr;
  bool show_on_init_ = false;
  bool js_context_ready_ = false;

  // Caches context changes while JS part is not ready to receive messages.
  ::login::ScreenContext context_cache_;

  DISALLOW_COPY_AND_ASSIGN(HostPairingScreenHandler);
};

}  // namespace chromeos

#endif  // CHROME_BROWSER_UI_WEBUI_CHROMEOS_LOGIN_HOST_PAIRING_SCREEN_HANDLER_H_
