// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/webui/chromeos/login/controller_pairing_screen_handler.h"

#include "base/command_line.h"
#include "base/strings/string_util.h"
#include "chrome/browser/chromeos/login/oobe_screen.h"
#include "chrome/grit/generated_resources.h"
#include "chromeos/chromeos_switches.h"
#include "components/login/localized_values_builder.h"
#include "content/public/browser/web_contents.h"

namespace chromeos {

namespace {

const char kJsScreenPath[] = "login.ControllerPairingScreen";

const char kMethodContextChanged[] = "contextChanged";

const char kCallbackUserActed[] = "userActed";
const char kCallbackContextChanged[] = "contextChanged";

bool IsBootstrappingMaster() {
  return base::CommandLine::ForCurrentProcess()->HasSwitch(
      chromeos::switches::kOobeBootstrappingMaster);
}

}  // namespace

ControllerPairingScreenHandler::ControllerPairingScreenHandler()
    : BaseScreenHandler(kScreenId) {
  set_call_js_prefix(kJsScreenPath);
}

ControllerPairingScreenHandler::~ControllerPairingScreenHandler() {
  if (delegate_)
    delegate_->OnViewDestroyed(this);
}

void ControllerPairingScreenHandler::HandleUserActed(
    const std::string& action) {
  if (!delegate_)
    return;
  delegate_->OnUserActed(action);
}

void ControllerPairingScreenHandler::HandleContextChanged(
    const base::DictionaryValue* diff) {
  if (!delegate_)
    return;
  delegate_->OnScreenContextChanged(*diff);
}

void ControllerPairingScreenHandler::Initialize() {
  if (!page_is_ready() || !delegate_)
    return;

  if (show_on_init_) {
    Show();
    show_on_init_ = false;
  }
}

void ControllerPairingScreenHandler::DeclareLocalizedValues(
    ::login::LocalizedValuesBuilder* builder) {
  // TODO(dzhioev): Move the prefix logic to the base screen handler after
  // migration.
  std::string prefix;
  base::RemoveChars(kJsScreenPath, ".", &prefix);

  // TODO(xdai): Remove unnecessary strings.
  builder->Add(prefix + "WelcomeTitle", IDS_PAIRING_CONTROLLER_WELCOME);
  builder->Add(prefix + "Searching", IDS_PAIRING_CONTROLLER_SEARCHING);
  builder->Add(prefix + "HelpBtn", IDS_PAIRING_NEED_HELP);
  builder->Add(prefix + "TroubleConnectingTitle",
               IDS_PAIRING_CONTROLLER_TROUBLE_CONNECTING);
  builder->Add(prefix + "ConnectingAdvice",
               IDS_PAIRING_CONTROLLER_CONNECTING_ADVICE);
  builder->Add(prefix + "AdviceGotItBtn", IDS_PAIRING_CONTROLLER_ADVICE_GOT_IT);
  builder->Add(prefix + "SelectTitle", IDS_PAIRING_CONTROLLER_SELECT_TITLE);
  builder->Add(prefix + "ConnectBtn", IDS_PAIRING_CONTROLLER_CONNECT);
  builder->Add(prefix + "Connecting", IDS_PAIRING_CONTROLLER_CONNECTING);
  builder->Add(prefix + "ConfirmationTitle",
               IDS_PAIRING_CONTROLLER_CONFIRMATION_TITLE);
  builder->Add(prefix + "ConfirmationQuestion",
               IDS_PAIRING_CONTROLLER_CONFIRMATION_QUESTION);
  builder->Add(prefix + "RejectCodeBtn", IDS_PAIRING_CONTROLLER_REJECT_CODE);
  builder->Add(prefix + "AcceptCodeBtn", IDS_PAIRING_CONTROLLER_ACCEPT_CODE);
  builder->Add(prefix + "UpdateTitle", IDS_PAIRING_CONTROLLER_UPDATE_TITLE);
  builder->Add(prefix + "UpdateText", IDS_PAIRING_CONTROLLER_UPDATE_TEXT);
  builder->Add(prefix + "ConnectionLostTitle",
               IDS_PAIRING_CONTROLLER_CONNECTION_LOST_TITLE);
  builder->Add(prefix + "ConnectionLostText",
               IDS_PAIRING_CONTROLLER_CONNECTION_LOST_TEXT);
  builder->Add(prefix + "HostNetworkErrorTitle",
               IDS_PAIRING_CONTROLLER_HOST_NETWORK_ERROR_TITLE);
  builder->Add(prefix + "EnrollTitle", IDS_PAIRING_CONTROLLER_ENROLL_TITLE);
  builder->Add(prefix + "EnrollText1", IDS_PAIRING_CONTROLLER_ENROLL_TEXT_1);
  builder->Add(prefix + "EnrollText2", IDS_PAIRING_CONTROLLER_ENROLL_TEXT_2);
  builder->Add(prefix + "ContinueBtn", IDS_PAIRING_CONTROLLER_CONTINUE);
  builder->Add(prefix + "EnrollmentInProgress",
               IDS_PAIRING_CONTROLLER_ENROLLMENT_IN_PROGRESS);
  builder->Add(prefix + "EnrollmentErrorTitle",
               IDS_PAIRING_ENROLLMENT_ERROR_TITLE);
  builder->Add(prefix + "EnrollmentErrorHostRestarts",
               IDS_PAIRING_CONTROLLER_ENROLLMENT_ERROR_HOST_RESTARTS);
  builder->Add(prefix + "SuccessTitle", IDS_PAIRING_CONTROLLER_SUCCESS_TITLE);
  builder->Add(prefix + "SuccessText", IDS_PAIRING_CONTROLLER_SUCCESS_TEXT);
  builder->Add(prefix + "ContinueToHangoutsBtn",
               IDS_PAIRING_CONTROLLER_CONTINUE_TO_HANGOUTS);

  if (IsBootstrappingMaster()) {
    // These strings are only for testing/demo purpose, so they are not put into
    // grd file for translation.
    builder->Add(prefix + "WelcomeTitle",
                 "Welcome to the bootstrapping process");
    builder->Add(prefix + "Searching",
                 "Searching for nearby Chrome OS devices...");
    builder->Add(prefix + "ConnectingAdvice",
                 "Please make sure that your Chrome OS device is turned on.");
    builder->Add(prefix + "SelectTitle", "Select a Chrome OS device to set up");
    builder->Add(prefix + "ConfirmationTitle", "Initialize the connection");
    builder->Add(prefix + "ConfirmationQuestion",
                 "Does this code appear on your Chrome OS device\'s screen?");
    builder->Add(prefix + "UpdateTitle", "Updating...");
    builder->Add(prefix + "UpdateText",
                 "In order to bring you the latest features, your Chrome OS "
                 "device needs to update.");
    builder->Add(prefix + "ConnectionLostTitle",
                 "Connection to Chrome OS device lost");
    builder->Add(prefix + "ConnectionLostText",
                 " Lost connection to your Chrome OS device. Please move "
                 "closer, or check your device and try again.");
    builder->Add(prefix + "HostNetworkErrorTitle",
                 "Failed to set up your Chrome OS device\'s network");
  }
}

void ControllerPairingScreenHandler::RegisterMessages() {
  AddPrefixedCallback(kCallbackUserActed,
                      &ControllerPairingScreenHandler::HandleUserActed);
  AddPrefixedCallback(kCallbackContextChanged,
                      &ControllerPairingScreenHandler::HandleContextChanged);
}

void ControllerPairingScreenHandler::Show() {
  if (!page_is_ready()) {
    show_on_init_ = true;
    return;
  }
  ShowScreen(kScreenId);
}

void ControllerPairingScreenHandler::Hide() {
}

void ControllerPairingScreenHandler::SetDelegate(Delegate* delegate) {
  delegate_ = delegate;
  if (page_is_ready())
    Initialize();
}

void ControllerPairingScreenHandler::OnContextChanged(
    const base::DictionaryValue& diff) {
  CallJS(kMethodContextChanged, diff);
}

content::BrowserContext* ControllerPairingScreenHandler::GetBrowserContext() {
  return web_ui()->GetWebContents()->GetBrowserContext();
}

}  // namespace chromeos
