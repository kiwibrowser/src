// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/login/screens/controller_pairing_screen.h"

#include "base/command_line.h"
#include "base/stl_util.h"
#include "base/values.h"
#include "chrome/browser/chromeos/login/wizard_controller.h"
#include "google_apis/gaia/gaia_auth_util.h"

using namespace chromeos::controller_pairing;
using namespace pairing_chromeos;

namespace chromeos {

ControllerPairingScreen::ControllerPairingScreen(
    BaseScreenDelegate* base_screen_delegate,
    Delegate* delegate,
    ControllerPairingScreenView* view,
    ControllerPairingController* shark_controller)
    : BaseScreen(base_screen_delegate,
                 OobeScreen::SCREEN_OOBE_CONTROLLER_PAIRING),
      delegate_(delegate),
      view_(view),
      shark_controller_(shark_controller),
      current_stage_(ControllerPairingController::STAGE_NONE),
      device_preselected_(false) {
  view_->SetDelegate(this);
  shark_controller_->AddObserver(this);
}

ControllerPairingScreen::~ControllerPairingScreen() {
  if (view_)
    view_->SetDelegate(nullptr);
  shark_controller_->RemoveObserver(this);
}

void ControllerPairingScreen::CommitContextChanges() {
  if (!context_.HasChanges())
    return;
  base::DictionaryValue diff;
  context_.GetChangesAndReset(&diff);
  if (view_)
    view_->OnContextChanged(diff);
}

bool ControllerPairingScreen::ExpectStageIs(Stage stage) const {
  DCHECK(stage == current_stage_);
  if (current_stage_ != stage)
    LOG(ERROR) << "Incorrect stage. Expected: " << stage
               << ", current stage: " << current_stage_;
  return stage == current_stage_;
}

void ControllerPairingScreen::Show() {
  if (view_)
    view_->Show();
  shark_controller_->StartPairing();
}

void ControllerPairingScreen::Hide() {
  if (view_)
    view_->Hide();
}

void ControllerPairingScreen::PairingStageChanged(Stage new_stage) {
  DCHECK(new_stage != current_stage_);

  std::string desired_page;
  switch (new_stage) {
    case ControllerPairingController::STAGE_DEVICES_DISCOVERY: {
      desired_page = kPageDevicesDiscovery;
      context_.SetStringList(kContextKeyDevices, ::login::StringList());
      context_.SetString(kContextKeySelectedDevice, std::string());
      device_preselected_ = false;
      break;
    }
    case ControllerPairingController::STAGE_DEVICE_NOT_FOUND: {
      desired_page = kPageDeviceNotFound;
      break;
    }
    case ControllerPairingController::STAGE_ESTABLISHING_CONNECTION: {
      desired_page = kPageEstablishingConnection;
      break;
    }
    case ControllerPairingController::STAGE_ESTABLISHING_CONNECTION_ERROR: {
      desired_page = kPageEstablishingConnectionError;
      break;
    }
    case ControllerPairingController::STAGE_WAITING_FOR_CODE_CONFIRMATION: {
      desired_page = kPageCodeConfirmation;
      context_.SetString(kContextKeyConfirmationCode,
                         shark_controller_->GetConfirmationCode());
      break;
    }
    case ControllerPairingController::STAGE_PAIRING_DONE: {
      if (delegate_) {
        delegate_->SetHostConfiguration();
        delegate_->SetHostNetwork();
      }
      break;
    }
    case ControllerPairingController::STAGE_HOST_NETWORK_ERROR: {
      desired_page = kPageHostNetworkError;
      break;
    }
    case ControllerPairingController::STAGE_HOST_UPDATE_IN_PROGRESS: {
      desired_page = kPageHostUpdate;
      break;
    }
    case ControllerPairingController::STAGE_HOST_CONNECTION_LOST: {
      desired_page = kPageHostConnectionLost;
      break;
    }
    case ControllerPairingController::STAGE_WAITING_FOR_CREDENTIALS: {
      shark_controller_->RemoveObserver(this);
      Finish(ScreenExitCode::CONTROLLER_PAIRING_FINISHED);
      desired_page = kPageEnrollmentIntroduction;
      break;
    }
    case ControllerPairingController::STAGE_INITIALIZATION_ERROR: {
      // TODO(achuith, dzhioev, zork): Handle this better.
      LOG(WARNING) << "Bluetooth initialization error";
      break;
    }
    default:
      NOTREACHED();
  }
  current_stage_ = new_stage;
  context_.SetString(kContextKeyPage, desired_page);
  context_.SetBoolean(kContextKeyControlsDisabled, false);
  CommitContextChanges();
  VLOG(1) << "PairingStageChanged " << desired_page << ", current stage "
          << current_stage_;
}

void ControllerPairingScreen::DiscoveredDevicesListChanged() {
  if (!ExpectStageIs(ControllerPairingController::STAGE_DEVICES_DISCOVERY))
    return;
  ControllerPairingController::DeviceIdList devices =
      shark_controller_->GetDiscoveredDevices();
  std::sort(devices.begin(), devices.end());
  context_.SetStringList(kContextKeyDevices, devices);
  context_.SetString(kContextKeyPage, devices.empty() ? kPageDevicesDiscovery
                                                      : kPageDeviceSelect);
  std::string selected_device = context_.GetString(kContextKeySelectedDevice);
  if (!base::ContainsValue(devices, selected_device))
    selected_device.clear();
  if (devices.empty()) {
    device_preselected_ = false;
  } else if (!device_preselected_) {
    selected_device = devices.front();
    device_preselected_ = true;
  }
  context_.SetString(kContextKeySelectedDevice, selected_device);
  context_.SetBoolean(kContextKeyControlsDisabled, selected_device.empty());
  CommitContextChanges();
}

void ControllerPairingScreen::OnViewDestroyed(
    ControllerPairingScreenView* view) {
  if (view_ == view)
    view_ = nullptr;
}

// Overridden from ControllerPairingView::Delegate:
void ControllerPairingScreen::OnUserActed(const std::string& action) {
  if (context_.GetBoolean(kContextKeyControlsDisabled)) {
    LOG(WARNING) << "User acted, but controls are disabled. Ignoring.";
    return;
  }
  bool disable_controls = true;
  if (action == kActionChooseDevice) {
    std::string selectedDevice = context_.GetString(kContextKeySelectedDevice);
    if (selectedDevice.empty())
      LOG(ERROR) << "Device was not selected.";
    else
      shark_controller_->ChooseDeviceForPairing(selectedDevice);
  } else if (action == kActionRepeatDiscovery) {
    shark_controller_->RepeatDiscovery();
  } else if (action == kActionAcceptCode) {
    shark_controller_->SetConfirmationCodeIsCorrect(true);
  } else if (action == kActionRejectCode) {
    shark_controller_->SetConfirmationCodeIsCorrect(false);
  } else if (action == kActionProceedToAuthentication) {
    context_.SetString(kContextKeyPage, kPageAuthentication);
    disable_controls = false;
  } else if (action == kActionEnroll) {
    const std::string account_id =
        gaia::SanitizeEmail(context_.GetString(kContextKeyAccountId));
    const std::string domain(gaia::ExtractDomainName(account_id));
    context_.SetString(kContextKeyEnrollmentDomain, domain);
  } else if (action == kActionStartSession) {
    shark_controller_->StartSession();
  }
  context_.SetBoolean(kContextKeyControlsDisabled, disable_controls);
  CommitContextChanges();
}

void ControllerPairingScreen::OnScreenContextChanged(
    const base::DictionaryValue& diff) {
  std::vector<std::string> changedKeys;
  context_.ApplyChanges(diff, &changedKeys);
  for (std::vector<std::string>::const_iterator key = changedKeys.begin();
       key != changedKeys.end(); ++key) {
    if (*key == kContextKeySelectedDevice) {
      context_.SetBoolean(kContextKeyControlsDisabled,
                          context_.GetString(*key).empty());
      CommitContextChanges();
    }
  }
}

}  // namespace chromeos
