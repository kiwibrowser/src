// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_WEBUI_CHROMEOS_LOGIN_TERMS_OF_SERVICE_SCREEN_HANDLER_H_
#define CHROME_BROWSER_UI_WEBUI_CHROMEOS_LOGIN_TERMS_OF_SERVICE_SCREEN_HANDLER_H_

#include <string>

#include "base/compiler_specific.h"
#include "base/macros.h"
#include "chrome/browser/chromeos/base/locale_util.h"
#include "chrome/browser/chromeos/login/screens/terms_of_service_screen_view.h"
#include "chrome/browser/ui/webui/chromeos/login/base_screen_handler.h"

namespace chromeos {

class CoreOobeView;

// The sole implementation of the TermsOfServiceScreenView, using WebUI.
class TermsOfServiceScreenHandler : public BaseScreenHandler,
                                    public TermsOfServiceScreenView {
 public:
  explicit TermsOfServiceScreenHandler(CoreOobeView* core_oobe_view);
  ~TermsOfServiceScreenHandler() override;

  // content::WebUIMessageHandler:
  void RegisterMessages() override;

  // BaseScreenHandler:
  void DeclareLocalizedValues(
      ::login::LocalizedValuesBuilder* builder) override;

  // TermsOfServiceScreenView:
  void SetDelegate(Delegate* screen) override;
  void Show() override;
  void Hide() override;
  void SetDomain(const std::string& domain) override;
  void OnLoadError() override;
  void OnLoadSuccess(const std::string& terms_of_service) override;

 private:
  // BaseScreenHandler:
  void Initialize() override;

  // Callback invoked after the UI locale has been changed.
  void OnLanguageChangedCallback(
      const locale_util::LanguageSwitchResult& result);

  // Switch to the user's preferred input method and show the screen. This
  // method is called after it has been ensured that the current UI locale
  // matches the UI locale chosen by the user.
  void DoShow();

  // Update the domain name shown in the UI.
  void UpdateDomainInUI();

  // Update the UI to show an error message or the Terms of Service, depending
  // on whether the download of the Terms of Service was successful. Does
  // nothing if the download is still in progress.
  void UpdateTermsOfServiceInUI();

  // Called when the user declines the Terms of Service by clicking the "back"
  // button.
  void HandleBack();

  // Called when the user accepts the Terms of Service by clicking the "accept
  // and continue" button.
  void HandleAccept();

  TermsOfServiceScreenHandler::Delegate* screen_ = nullptr;

  CoreOobeView* core_oobe_view_ = nullptr;

  // Whether the screen should be shown right after initialization.
  bool show_on_init_ = false;

  // The domain name whose Terms of Service are being shown.
  std::string domain_;

  // Set to |true| when the download of the Terms of Service fails.
  bool load_error_ = false;

  // Set to the Terms of Service when the download is successful.
  std::string terms_of_service_;

  DISALLOW_COPY_AND_ASSIGN(TermsOfServiceScreenHandler);
};

}  // namespace chromeos

#endif  // CHROME_BROWSER_UI_WEBUI_CHROMEOS_LOGIN_TERMS_OF_SERVICE_SCREEN_HANDLER_H_
