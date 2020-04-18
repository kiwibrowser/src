// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/webui/chromeos/login/hid_detection_screen_handler.h"

#include "base/bind.h"
#include "base/command_line.h"
#include "base/compiler_specific.h"
#include "base/macros.h"
#include "base/strings/string16.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/chromeos/login/oobe_screen.h"
#include "chrome/browser/chromeos/login/screens/core_oobe_view.h"
#include "chrome/browser/chromeos/login/screens/hid_detection_screen.h"
#include "chrome/browser/ui/webui/chromeos/login/oobe_ui.h"
#include "chrome/common/pref_names.h"
#include "chrome/grit/generated_resources.h"
#include "chromeos/chromeos_switches.h"
#include "components/login/localized_values_builder.h"
#include "components/prefs/pref_service.h"

namespace {

const char kJsScreenPath[] = "login.HIDDetectionScreen";

}  // namespace

namespace chromeos {

HIDDetectionScreenHandler::HIDDetectionScreenHandler(
    CoreOobeView* core_oobe_view)
    : BaseScreenHandler(kScreenId), core_oobe_view_(core_oobe_view) {
  set_call_js_prefix(kJsScreenPath);
}

HIDDetectionScreenHandler::~HIDDetectionScreenHandler() {
  if (screen_)
    screen_->OnViewDestroyed(this);
}

void HIDDetectionScreenHandler::Show() {
  if (!page_is_ready()) {
    show_on_init_ = true;
    return;
  }
  core_oobe_view_->InitDemoModeDetection();

  PrefService* local_state = g_browser_process->local_state();
  int num_of_times_dialog_was_shown = local_state->GetInteger(
      prefs::kTimesHIDDialogShown);
  local_state->SetInteger(prefs::kTimesHIDDialogShown,
                          num_of_times_dialog_was_shown + 1);

  ShowScreen(kScreenId);
}

void HIDDetectionScreenHandler::Hide() {
}

void HIDDetectionScreenHandler::Bind(HIDDetectionScreen* screen) {
  screen_ = screen;
  BaseScreenHandler::SetBaseScreen(screen_);
  if (page_is_ready())
    Initialize();
}

void HIDDetectionScreenHandler::Unbind() {
  screen_ = nullptr;
  BaseScreenHandler::SetBaseScreen(nullptr);
}

void HIDDetectionScreenHandler::CheckIsScreenRequired(
      const base::Callback<void(bool)>& on_check_done) {
  screen_->CheckIsScreenRequired(on_check_done);
}

void HIDDetectionScreenHandler::DeclareLocalizedValues(
    ::login::LocalizedValuesBuilder* builder) {
  builder->Add("hidDetectionContinue", IDS_HID_DETECTION_CONTINUE_BUTTON);
  builder->Add("hidDetectionInvitation", IDS_HID_DETECTION_INVITATION_TEXT);
  builder->Add("hidDetectionPrerequisites",
      IDS_HID_DETECTION_PRECONDITION_TEXT);
  builder->Add("hidDetectionMouseSearching", IDS_HID_DETECTION_SEARCHING_MOUSE);
  builder->Add("hidDetectionKeyboardSearching",
      IDS_HID_DETECTION_SEARCHING_KEYBOARD);
  builder->Add("hidDetectionUSBMouseConnected",
      IDS_HID_DETECTION_CONNECTED_USB_MOUSE);
  builder->Add("hidDetectionPointingDeviceConnected",
      IDS_HID_DETECTION_CONNECTED_POINTING_DEVICE);
  builder->Add("hidDetectionUSBKeyboardConnected",
      IDS_HID_DETECTION_CONNECTED_USB_KEYBOARD);
  builder->Add("hidDetectionBTMousePaired",
      IDS_HID_DETECTION_PAIRED_BLUETOOTH_MOUSE);
  builder->Add("hidDetectionBTEnterKey", IDS_HID_DETECTION_BLUETOOTH_ENTER_KEY);
}

void HIDDetectionScreenHandler::DeclareJSCallbacks() {
  AddCallback(
      "HIDDetectionOnContinue", &HIDDetectionScreenHandler::HandleOnContinue);
}

void HIDDetectionScreenHandler::Initialize() {
  if (show_on_init_) {
    Show();
    show_on_init_ = false;
  }
}

void HIDDetectionScreenHandler::HandleOnContinue() {
  // Continue button pressed.
  core_oobe_view_->StopDemoModeDetection();
  if (screen_)
    screen_->OnContinueButtonClicked();
}

// static
void HIDDetectionScreenHandler::RegisterPrefs(PrefRegistrySimple* registry) {
  registry->RegisterIntegerPref(prefs::kTimesHIDDialogShown, 0);
}

}  // namespace chromeos
