// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/webui/chromeos/login/network_screen_handler.h"

#include <stddef.h>

#include <utility>

#include "base/bind.h"
#include "base/bind_helpers.h"
#include "base/command_line.h"
#include "base/strings/string16.h"
#include "base/strings/utf_string_conversions.h"
#include "base/task_runner_util.h"
#include "base/values.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/chromeos/accessibility/accessibility_manager.h"
#include "chrome/browser/chromeos/customization/customization_document.h"
#include "chrome/browser/chromeos/idle_detector.h"
#include "chrome/browser/chromeos/login/screens/core_oobe_view.h"
#include "chrome/browser/chromeos/login/screens/network_screen.h"
#include "chrome/browser/chromeos/login/ui/input_events_blocker.h"
#include "chrome/browser/chromeos/policy/browser_policy_connector_chromeos.h"
#include "chrome/browser/chromeos/policy/device_cloud_policy_manager_chromeos.h"
#include "chrome/browser/chromeos/system/input_device_settings.h"
#include "chrome/browser/chromeos/system/timezone_util.h"
#include "chrome/browser/ui/webui/chromeos/login/l10n_util.h"
#include "chrome/browser/ui/webui/chromeos/login/oobe_ui.h"
#include "chrome/browser/ui/webui/chromeos/network_element_localized_strings_provider.h"
#include "chrome/common/pref_names.h"
#include "chrome/grit/generated_resources.h"
#include "chromeos/chromeos_switches.h"
#include "chromeos/network/network_handler.h"
#include "chromeos/network/network_state_handler.h"
#include "components/login/localized_values_builder.h"
#include "components/prefs/pref_service.h"
#include "components/strings/grit/components_strings.h"
#include "components/user_manager/user_manager.h"
#include "content/public/browser/browser_thread.h"
#include "ui/base/ime/chromeos/extension_ime_util.h"
#include "ui/gfx/geometry/rect.h"
#include "ui/views/layout/fill_layout.h"
#include "ui/views/widget/widget.h"

namespace {

const char kJsScreenPath[] = "login.NetworkScreen";

}  // namespace

namespace chromeos {

// NetworkScreenHandler, public: -----------------------------------------------

NetworkScreenHandler::NetworkScreenHandler(CoreOobeView* core_oobe_view)
    : BaseScreenHandler(kScreenId), core_oobe_view_(core_oobe_view) {
  set_call_js_prefix(kJsScreenPath);
  DCHECK(core_oobe_view_);
}

NetworkScreenHandler::~NetworkScreenHandler() {
  if (screen_)
    screen_->OnViewDestroyed(this);
}

// NetworkScreenHandler, NetworkScreenView implementation: ---------------------

void NetworkScreenHandler::Show() {
  if (!page_is_ready()) {
    show_on_init_ = true;
    return;
  }

  PrefService* prefs = g_browser_process->local_state();
  if (prefs->GetBoolean(prefs::kFactoryResetRequested)) {
    if (core_oobe_view_)
      core_oobe_view_->ShowDeviceResetScreen();

    return;
  } else if (prefs->GetBoolean(prefs::kDebuggingFeaturesRequested)) {
    if (core_oobe_view_)
      core_oobe_view_->ShowEnableDebuggingScreen();

    return;
  }

  // Make sure all physical network technologies are enabled. On OOBE, the user
  // should be able to select any of the available networks on the device.
  NetworkStateHandler* handler = NetworkHandler::Get()->network_state_handler();
  handler->SetTechnologyEnabled(NetworkTypePattern::Physical(), true,
                                chromeos::network_handler::ErrorCallback());

  base::DictionaryValue network_screen_params;
  network_screen_params.SetBoolean("isDeveloperMode",
      base::CommandLine::ForCurrentProcess()->HasSwitch(
          chromeos::switches::kSystemDevMode));
  ShowScreenWithData(kScreenId, &network_screen_params);
  core_oobe_view_->InitDemoModeDetection();
}

void NetworkScreenHandler::Hide() {
}

void NetworkScreenHandler::Bind(NetworkScreen* screen) {
  screen_ = screen;
  BaseScreenHandler::SetBaseScreen(screen_);
}

void NetworkScreenHandler::Unbind() {
  screen_ = nullptr;
  BaseScreenHandler::SetBaseScreen(nullptr);
}

void NetworkScreenHandler::ShowError(const base::string16& message) {
  CallJS("showError", message);
}

void NetworkScreenHandler::ClearErrors() {
  if (page_is_ready())
    core_oobe_view_->ClearErrors();
}

void NetworkScreenHandler::StopDemoModeDetection() {
  core_oobe_view_->StopDemoModeDetection();
}

void NetworkScreenHandler::ShowConnectingStatus(
    bool connecting,
    const base::string16& network_id) {
}

void NetworkScreenHandler::ReloadLocalizedContent() {
  base::DictionaryValue localized_strings;
  GetOobeUI()->GetLocalizedStrings(&localized_strings);
  core_oobe_view_->ReloadContent(localized_strings);
}

// NetworkScreenHandler, BaseScreenHandler implementation: --------------------

void NetworkScreenHandler::DeclareLocalizedValues(
    ::login::LocalizedValuesBuilder* builder) {
  if (system::InputDeviceSettings::Get()->ForceKeyboardDrivenUINavigation())
    builder->Add("networkScreenGreeting", IDS_REMORA_CONFIRM_MESSAGE);
  else
    builder->Add("networkScreenGreeting", IDS_WELCOME_SCREEN_GREETING);

  builder->Add("networkScreenTitle", IDS_WELCOME_SCREEN_TITLE);
  builder->Add("selectLanguage", IDS_LANGUAGE_SELECTION_SELECT);
  builder->Add("selectKeyboard", IDS_KEYBOARD_SELECTION_SELECT);
  builder->Add("selectNetwork", IDS_NETWORK_SELECTION_SELECT);
  builder->Add("selectTimezone", IDS_OPTIONS_SETTINGS_TIMEZONE_DESCRIPTION);
  builder->Add("timezoneDropdownLabel", IDS_TIMEZONE_DROPDOWN_LABEL);
  builder->Add("proxySettings", IDS_OPTIONS_PROXIES_CONFIGURE_BUTTON);
  builder->Add("continueButton", IDS_NETWORK_SELECTION_CONTINUE_BUTTON);
  builder->Add("debuggingFeaturesLink", IDS_NETWORK_ENABLE_DEV_FEATURES_LINK);

  // MD-OOBE
  builder->Add("oobeOKButtonText", IDS_OOBE_OK_BUTTON_TEXT);
  builder->Add("welcomeNextButtonText", IDS_OOBE_WELCOME_NEXT_BUTTON_TEXT);
  builder->Add("languageButtonLabel", IDS_LANGUAGE_BUTTON_LABEL);
  builder->Add("languageSectionTitle", IDS_LANGUAGE_SECTION_TITLE);
  builder->Add("accessibilitySectionTitle", IDS_ACCESSIBILITY_SECTION_TITLE);
  builder->Add("accessibilitySectionHint", IDS_ACCESSIBILITY_SECTION_HINT);
  builder->Add("timezoneSectionTitle", IDS_TIMEZONE_SECTION_TITLE);
  builder->Add("networkSectionTitle", IDS_NETWORK_SECTION_TITLE);
  builder->Add("networkSectionHint", IDS_NETWORK_SECTION_HINT);
  builder->Add("advancedOptionsSectionTitle",
               IDS_OOBE_ADVANCED_OPTIONS_SCREEN_TITLE);
  builder->Add("advancedOptionsEEBootstrappingTitle",
               IDS_OOBE_ADVANCED_OPTIONS_EE_BOOTSTRAPPING_TITLE);
  builder->Add("advancedOptionsEEBootstrappingSubtitle",
               IDS_OOBE_ADVANCED_OPTIONS_EE_BOOTSTRAPPING_SUBTITLE);
  builder->Add("advancedOptionsCFMSetupTitle",
               IDS_OOBE_ADVANCED_OPTIONS_CFM_SETUP_TITLE);
  builder->Add("advancedOptionsCFMSetupSubtitle",
               IDS_OOBE_ADVANCED_OPTIONS_CFM_SETUP_SUBTITLE);
  builder->Add("advancedOptionsDeviceRequisitionTitle",
               IDS_OOBE_ADVANCED_OPTIONS_DEVICE_REQUISITION_TITLE);
  builder->Add("advancedOptionsDeviceRequisitionSubtitle",
               IDS_OOBE_ADVANCED_OPTIONS_DEVICE_REQUISITION_SUBTITLE);

  builder->Add("languageDropdownTitle", IDS_LANGUAGE_DROPDOWN_TITLE);
  builder->Add("languageDropdownLabel", IDS_LANGUAGE_DROPDOWN_LABEL);
  builder->Add("keyboardDropdownTitle", IDS_KEYBOARD_DROPDOWN_TITLE);
  builder->Add("keyboardDropdownLabel", IDS_KEYBOARD_DROPDOWN_LABEL);
  builder->Add("proxySettingsMenuName", IDS_PROXY_SETTINGS_MENU_NAME);
  builder->Add("addWiFiNetworkMenuName", IDS_ADD_WI_FI_NETWORK_MENU_NAME);

  builder->Add("highContrastOptionOff", IDS_HIGH_CONTRAST_OPTION_OFF);
  builder->Add("highContrastOptionOn", IDS_HIGH_CONTRAST_OPTION_ON);
  builder->Add("largeCursorOptionOff", IDS_LARGE_CURSOR_OPTION_OFF);
  builder->Add("largeCursorOptionOn", IDS_LARGE_CURSOR_OPTION_ON);
  builder->Add("screenMagnifierOptionOff", IDS_SCREEN_MAGNIFIER_OPTION_OFF);
  builder->Add("screenMagnifierOptionOn", IDS_SCREEN_MAGNIFIER_OPTION_ON);
  builder->Add("spokenFeedbackOptionOff", IDS_SPOKEN_FEEDBACK_OPTION_OFF);
  builder->Add("spokenFeedbackOptionOn", IDS_SPOKEN_FEEDBACK_OPTION_ON);
  builder->Add("virtualKeyboardOptionOff", IDS_VIRTUAL_KEYBOARD_OPTION_OFF);
  builder->Add("virtualKeyboardOptionOn", IDS_VIRTUAL_KEYBOARD_OPTION_ON);

  builder->Add("timezoneDropdownTitle", IDS_TIMEZONE_DROPDOWN_TITLE);
  builder->Add("timezoneButtonText", IDS_TIMEZONE_BUTTON_TEXT);
  network_element::AddLocalizedValuesToBuilder(builder);
}

void NetworkScreenHandler::GetAdditionalParameters(
    base::DictionaryValue* dict) {
  const std::string application_locale =
      g_browser_process->GetApplicationLocale();
  const std::string selected_input_method =
      input_method::InputMethodManager::Get()
          ->GetActiveIMEState()
          ->GetCurrentInputMethod()
          .id();

  std::unique_ptr<base::ListValue> language_list;
  if (screen_) {
    if (screen_->language_list() &&
        screen_->language_list_locale() == application_locale) {
      language_list.reset(screen_->language_list()->DeepCopy());
    } else {
      screen_->UpdateLanguageList();
    }
  }

  if (!language_list)
    language_list = GetMinimalUILanguageList();

  // GetAdditionalParameters() is called when OOBE language is updated.
  // This happens in three different cases:
  //
  // 1) User selects new locale on OOBE screen. We need to sync active input
  // methods with locale, so EnableLoginLayouts() is needed.
  //
  // 2) This is signin to public session. User has selected some locale & input
  // method on "Public Session User POD". After "Login" button is pressed,
  // new user session is created, locale & input method are changed (both
  // asynchronously).
  // But after public user session is started, "Terms of Service" dialog is
  // shown. It is a part of OOBE UI screens, so it initiates reload of UI
  // strings in new locale. It also happens asynchronously, that leads to race
  // between "locale change", "input method change" and
  // "EnableLoginLayouts()".  This way EnableLoginLayouts() happens after user
  // input method has been changed, resetting input method to hardware default.
  //
  // So we need to disable activation of login layouts if we are already in
  // active user session.
  //
  // 3) This is the bootstrapping process for a "Slave" device. The locale &
  // input of the "Slave" device is set up by a "Master" device. In this case we
  // don't want EnableLoginLayout() to reset the input method to the hardware
  // default method.
  const bool is_slave = g_browser_process->local_state()->GetBoolean(
      prefs::kOobeControllerDetected);

  const bool enable_layouts =
      !user_manager::UserManager::Get()->IsUserLoggedIn() && !is_slave;

  dict->Set("languageList", std::move(language_list));
  dict->Set("inputMethodsList",
            GetAndActivateLoginKeyboardLayouts(
                application_locale, selected_input_method, enable_layouts));
  dict->Set("timezoneList", GetTimezoneList());
}

void NetworkScreenHandler::Initialize() {
  if (show_on_init_) {
    show_on_init_ = false;
    Show();
  }

  // Reload localized strings if they are already resolved.
  if (screen_ && screen_->language_list())
    ReloadLocalizedContent();
}

// NetworkScreenHandler, private: ----------------------------------------------

// static
std::unique_ptr<base::ListValue> NetworkScreenHandler::GetTimezoneList() {
  std::string current_timezone_id;
  CrosSettings::Get()->GetString(kSystemTimezone, &current_timezone_id);

  std::unique_ptr<base::ListValue> timezone_list(new base::ListValue);
  std::unique_ptr<base::ListValue> timezones = system::GetTimezoneList();
  for (size_t i = 0; i < timezones->GetSize(); ++i) {
    const base::ListValue* timezone = NULL;
    CHECK(timezones->GetList(i, &timezone));

    std::string timezone_id;
    CHECK(timezone->GetString(0, &timezone_id));

    std::string timezone_name;
    CHECK(timezone->GetString(1, &timezone_name));

    std::unique_ptr<base::DictionaryValue> timezone_option(
        new base::DictionaryValue);
    timezone_option->SetString("value", timezone_id);
    timezone_option->SetString("title", timezone_name);
    timezone_option->SetBoolean("selected", timezone_id == current_timezone_id);
    timezone_list->Append(std::move(timezone_option));
  }

  return timezone_list;
}

}  // namespace chromeos
