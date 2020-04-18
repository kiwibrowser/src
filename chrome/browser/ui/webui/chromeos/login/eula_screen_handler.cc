// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/webui/chromeos/login/eula_screen_handler.h"

#include <memory>
#include <string>

#include "base/macros.h"
#include "base/strings/stringprintf.h"
#include "base/strings/utf_string_conversions.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/chromeos/login/help_app_launcher.h"
#include "chrome/browser/chromeos/login/helper.h"
#include "chrome/browser/chromeos/login/oobe_screen.h"
#include "chrome/browser/chromeos/login/screens/core_oobe_view.h"
#include "chrome/browser/chromeos/login/screens/eula_screen.h"
#include "chrome/browser/chromeos/login/ui/login_display_webui.h"
#include "chrome/browser/chromeos/login/ui/login_web_dialog.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/common/url_constants.h"
#include "chrome/grit/chromium_strings.h"
#include "chrome/grit/generated_resources.h"
#include "chrome/grit/locale_settings.h"
#include "components/login/localized_values_builder.h"
#include "components/strings/grit/components_strings.h"
#include "content/public/browser/web_contents.h"
#include "rlz/buildflags/buildflags.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/views/widget/widget.h"
#include "url/gurl.h"

namespace {

constexpr char kJsScreenPath[] = "login.EulaScreen";

// Helper class to tweak display details of credits pages in the context
// of OOBE/EULA step.
class CreditsWebDialog : public chromeos::LoginWebDialog {
 public:
  CreditsWebDialog(Profile* profile,
                   gfx::NativeWindow parent_window,
                   int title_id,
                   const GURL& url)
      : chromeos::LoginWebDialog(profile, NULL, parent_window,
                                 l10n_util::GetStringUTF16(title_id),
                                 url) {
  }

  void OnLoadingStateChanged(content::WebContents* source) override {
    chromeos::LoginWebDialog::OnLoadingStateChanged(source);
    // Remove visual elements that we can handle in EULA page.
    bool is_loading = source->IsLoading();
    if (!is_loading && source->GetWebUI()) {
      source->GetWebUI()->CallJavascriptFunctionUnsafe(
          "(function () {"
          "  document.body.classList.toggle('dialog', true);"
          "  keyboard.initializeKeyboardFlow(false);"
          "})");
    }
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(CreditsWebDialog);
};

void ShowCreditsDialog(Profile* profile,
                       gfx::NativeWindow parent_window,
                       int title_id,
                       const GURL& credits_url) {
  CreditsWebDialog* dialog = new CreditsWebDialog(profile,
                                                  parent_window,
                                                  title_id,
                                                  credits_url);
  dialog->SetDialogSize(l10n_util::GetLocalizedContentsWidthInPixels(
                            IDS_CREDITS_APP_DIALOG_WIDTH_PIXELS),
                        l10n_util::GetLocalizedContentsWidthInPixels(
                            IDS_CREDITS_APP_DIALOG_HEIGHT_PIXELS));
  dialog->Show();
  // The dialog object will be deleted on dialog close.
}

}  // namespace

namespace chromeos {

EulaScreenHandler::EulaScreenHandler(CoreOobeView* core_oobe_view)
    : BaseScreenHandler(kScreenId),
      core_oobe_view_(core_oobe_view),
      weak_factory_(this) {
  set_call_js_prefix(kJsScreenPath);
}

EulaScreenHandler::~EulaScreenHandler() {
  if (screen_)
    screen_->OnViewDestroyed(this);
}

void EulaScreenHandler::Show() {
  if (!page_is_ready()) {
    show_on_init_ = true;
    return;
  }
  ShowScreen(kScreenId);
}

void EulaScreenHandler::Hide() {
}

void EulaScreenHandler::Bind(EulaScreen* screen) {
  screen_ = screen;
  BaseScreenHandler::SetBaseScreen(screen_);
  if (page_is_ready())
    Initialize();
}

void EulaScreenHandler::Unbind() {
  screen_ = nullptr;
  BaseScreenHandler::SetBaseScreen(nullptr);
}

void EulaScreenHandler::DeclareLocalizedValues(
    ::login::LocalizedValuesBuilder* builder) {
  builder->Add("eulaScreenTitle", IDS_EULA_SCREEN_TITLE);
  builder->Add("eulaScreenAccessibleTitle", IDS_EULA_SCREEN_ACCESSIBLE_TITLE);
  builder->Add("checkboxLogging", IDS_EULA_CHECKBOX_ENABLE_LOGGING);
  builder->Add("back", IDS_EULA_BACK_BUTTON);
  builder->Add("next", IDS_EULA_NEXT_BUTTON);
  builder->Add("acceptAgreement", IDS_EULA_ACCEPT_AND_CONTINUE_BUTTON);
  builder->Add("eulaSystemInstallationSettings",
               IDS_EULA_SYSTEM_SECURITY_SETTING);

  builder->Add("eulaTpmDesc", IDS_EULA_SECURE_MODULE_DESCRIPTION);
  builder->Add("eulaTpmKeyDesc", IDS_EULA_SECURE_MODULE_KEY_DESCRIPTION);
  builder->Add("eulaTpmDescPowerwash",
               IDS_EULA_SECURE_MODULE_KEY_DESCRIPTION_POWERWASH);
  builder->Add("eulaTpmBusy", IDS_EULA_SECURE_MODULE_BUSY);
  ::login::GetSecureModuleUsed(base::BindOnce(
      &EulaScreenHandler::UpdateLocalizedValues, weak_factory_.GetWeakPtr()));

  builder->Add("eulaSystemInstallationSettingsOkButton", IDS_OK);
  builder->Add("termsOfServiceLoading", IDS_TERMS_OF_SERVICE_SCREEN_LOADING);
#if BUILDFLAG(ENABLE_RLZ)
  builder->AddF("eulaRlzDesc",
                IDS_EULA_RLZ_DESCRIPTION,
                IDS_SHORT_PRODUCT_NAME,
                IDS_PRODUCT_NAME);
  builder->AddF("eulaRlzEnable",
                IDS_EULA_RLZ_ENABLE,
                IDS_SHORT_PRODUCT_OS_NAME);
#endif

  builder->Add(
      "eulaOnlineUrl",
      base::StringPrintf(chrome::kOnlineEulaURLPath,
                         g_browser_process->GetApplicationLocale().c_str()));

  builder->Add("chromeCreditsLink", IDS_ABOUT_VERSION_LICENSE_EULA);
  builder->Add("chromeosCreditsLink", IDS_ABOUT_CROS_VERSION_LICENSE_EULA);

  /* MD-OOBE */
  builder->Add("oobeEulaSectionTitle", IDS_OOBE_EULA_SECTION_TITLE);
  builder->Add("oobeEulaIframeLabel", IDS_OOBE_EULA_IFRAME_LABEL);
  builder->Add("oobeEulaAcceptAndContinueButtonText",
               IDS_OOBE_EULA_ACCEPT_AND_CONTINUE_BUTTON_TEXT);
}

void EulaScreenHandler::DeclareJSCallbacks() {
  AddCallback("eulaOnLearnMore", &EulaScreenHandler::HandleOnLearnMore);
  AddCallback("eulaOnChromeOSCredits",
              &EulaScreenHandler::HandleOnChromeOSCredits);
  AddCallback("eulaOnChromeCredits", &EulaScreenHandler::HandleOnChromeCredits);
  AddCallback("eulaOnLearnMore", &EulaScreenHandler::HandleOnLearnMore);
  AddCallback("eulaOnInstallationSettingsPopupOpened",
              &EulaScreenHandler::HandleOnInstallationSettingsPopupOpened);
}

void EulaScreenHandler::GetAdditionalParameters(base::DictionaryValue* dict) {
#if BUILDFLAG(ENABLE_RLZ)
  dict->SetString("rlzEnabled", "enabled");
#else
  dict->SetString("rlzEnabled", "disabled");
#endif
}

void EulaScreenHandler::Initialize() {
  if (!page_is_ready() || !screen_)
    return;

  core_oobe_view_->SetUsageStats(screen_->IsUsageStatsEnabled());

  // This OEM EULA is a file:// URL which we're unable to load in iframe.
  // Instead if it's defined we use chrome://terms/oem that will load same file.
  if (!screen_->GetOemEulaUrl().is_empty())
    core_oobe_view_->SetOemEulaUrl(chrome::kChromeUITermsOemURL);

  if (show_on_init_) {
    Show();
    show_on_init_ = false;
  }
}

void EulaScreenHandler::OnPasswordFetched(const std::string& tpm_password) {
  core_oobe_view_->SetTpmPassword(tpm_password);
}

void EulaScreenHandler::HandleOnLearnMore() {
  if (!help_app_.get())
    help_app_ = new HelpAppLauncher(GetNativeWindow());
  help_app_->ShowHelpTopic(HelpAppLauncher::HELP_STATS_USAGE);
}

void EulaScreenHandler::HandleOnChromeOSCredits() {
  ShowCreditsDialog(
      Profile::FromBrowserContext(
          web_ui()->GetWebContents()->GetBrowserContext()),
      GetNativeWindow(),
      IDS_ABOUT_CROS_VERSION_LICENSE_EULA,
      GURL(chrome::kChromeUIOSCreditsURL));
}

void EulaScreenHandler::HandleOnChromeCredits() {
  ShowCreditsDialog(
      Profile::FromBrowserContext(
          web_ui()->GetWebContents()->GetBrowserContext()),
      GetNativeWindow(),
      IDS_ABOUT_VERSION_LICENSE_EULA,
      GURL(chrome::kChromeUICreditsURL));
}

void EulaScreenHandler::HandleOnInstallationSettingsPopupOpened() {
  if (screen_)
    screen_->InitiatePasswordFetch();
}

void EulaScreenHandler::UpdateLocalizedValues(
    ::login::SecureModuleUsed secure_module_used) {
  base::DictionaryValue updated_secure_module_strings;
  auto builder = std::make_unique<::login::LocalizedValuesBuilder>(
      &updated_secure_module_strings);
  if (secure_module_used == ::login::SecureModuleUsed::TPM) {
    builder->Add("eulaTpmDesc", IDS_EULA_TPM_DESCRIPTION);
    builder->Add("eulaTpmKeyDesc", IDS_EULA_TPM_KEY_DESCRIPTION);
    builder->Add("eulaTpmDescPowerwash",
                 IDS_EULA_TPM_KEY_DESCRIPTION_POWERWASH);
    builder->Add("eulaTpmBusy", IDS_EULA_TPM_BUSY);
    core_oobe_view_->ReloadEulaContent(updated_secure_module_strings);
  }
}

}  // namespace chromeos
