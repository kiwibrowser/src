// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chooser_controller/mock_chooser_controller.h"

#include <algorithm>

#include "base/logging.h"
#include "base/strings/utf_string_conversions.h"
#include "chrome/grit/generated_resources.h"
#include "ui/base/l10n/l10n_util.h"

MockChooserController::MockChooserController()
    : ChooserController(nullptr,
                        IDS_USB_DEVICE_CHOOSER_PROMPT_ORIGIN,
                        IDS_USB_DEVICE_CHOOSER_PROMPT_EXTENSION_NAME) {
  set_title_for_testing(base::ASCIIToUTF16("Mock Chooser Dialog"));
}

MockChooserController::~MockChooserController() {}

bool MockChooserController::ShouldShowIconBeforeText() const {
  return true;
}

base::string16 MockChooserController::GetNoOptionsText() const {
  return l10n_util::GetStringUTF16(IDS_DEVICE_CHOOSER_NO_DEVICES_FOUND_PROMPT);
}

base::string16 MockChooserController::GetOkButtonLabel() const {
  return l10n_util::GetStringUTF16(IDS_USB_DEVICE_CHOOSER_CONNECT_BUTTON_TEXT);
}

size_t MockChooserController::NumOptions() const {
  return options_.size();
}

int MockChooserController::GetSignalStrengthLevel(size_t index) const {
  return options_[index].signal_strength_level;
}

base::string16 MockChooserController::GetOption(size_t index) const {
  return options_[index].name;
}

bool MockChooserController::IsConnected(size_t index) const {
  return options_[index].connected_paired_status &
         ConnectedPairedStatus::CONNECTED;
}

bool MockChooserController::IsPaired(size_t index) const {
  return options_[index].connected_paired_status &
         ConnectedPairedStatus::PAIRED;
}

base::string16 MockChooserController::GetStatus() const {
  return status_text_;
}

void MockChooserController::OnAdapterPresenceChanged(
    content::BluetoothChooser::AdapterPresence presence) {
  ClearAllOptions();
  switch (presence) {
    case content::BluetoothChooser::AdapterPresence::ABSENT:
      NOTREACHED();
      break;
    case content::BluetoothChooser::AdapterPresence::POWERED_OFF:
      status_text_ = base::string16();
      if (view())
        view()->OnAdapterEnabledChanged(false /* Adapter is turned off */);
      break;
    case content::BluetoothChooser::AdapterPresence::POWERED_ON:
      status_text_ =
          l10n_util::GetStringUTF16(IDS_BLUETOOTH_DEVICE_CHOOSER_RE_SCAN);
      if (view())
        view()->OnAdapterEnabledChanged(true /* Adapter is turned on */);
      break;
  }
}

void MockChooserController::OnDiscoveryStateChanged(
    content::BluetoothChooser::DiscoveryState state) {
  switch (state) {
    case content::BluetoothChooser::DiscoveryState::DISCOVERING:
      ClearAllOptions();
      status_text_ =
          l10n_util::GetStringUTF16(IDS_BLUETOOTH_DEVICE_CHOOSER_SCANNING);
      if (view()) {
        view()->OnRefreshStateChanged(
            true /* Refreshing options is in progress */);
      }
      break;
    case content::BluetoothChooser::DiscoveryState::IDLE:
    case content::BluetoothChooser::DiscoveryState::FAILED_TO_START:
      status_text_ =
          l10n_util::GetStringUTF16(IDS_BLUETOOTH_DEVICE_CHOOSER_RE_SCAN);
      if (view()) {
        view()->OnRefreshStateChanged(
            false /* Refreshing options is complete */);
      }
      break;
  }
}

void MockChooserController::OptionAdded(const base::string16& option_name,
                                        int signal_strength_level,
                                        int connected_paired_status) {
  options_.push_back(
      {option_name, signal_strength_level, connected_paired_status});
  if (view())
    view()->OnOptionAdded(options_.size() - 1);
}

void MockChooserController::OptionRemoved(const base::string16& option_name) {
  auto it = std::find_if(options_.begin(), options_.end(),
                         [&option_name](const OptionInfo& option) {
                           return option.name == option_name;
                         });

  if (it != options_.end()) {
    size_t index = it - options_.begin();
    options_.erase(it);
    if (view())
      view()->OnOptionRemoved(index);
  }
}

void MockChooserController::OptionUpdated(
    const base::string16& previous_option_name,
    const base::string16& new_option_name,
    int new_signal_strength_level,
    int new_connected_paired_status) {
  auto it = std::find_if(options_.begin(), options_.end(),
                         [&previous_option_name](const OptionInfo& option) {
                           return option.name == previous_option_name;
                         });

  if (it != options_.end()) {
    *it = {new_option_name, new_signal_strength_level,
           new_connected_paired_status};
    if (view())
      view()->OnOptionUpdated(it - options_.begin());
  }
}

const int MockChooserController::kNoSignalStrengthLevelImage = -1;
const int MockChooserController::kSignalStrengthLevel0Bar = 0;
const int MockChooserController::kSignalStrengthLevel1Bar = 1;
const int MockChooserController::kSignalStrengthLevel2Bar = 2;
const int MockChooserController::kSignalStrengthLevel3Bar = 3;
const int MockChooserController::kSignalStrengthLevel4Bar = 4;
const int MockChooserController::kImageColorUnselected = 0;
const int MockChooserController::kImageColorSelected = 1;

void MockChooserController::ClearAllOptions() {
  options_.clear();
}
