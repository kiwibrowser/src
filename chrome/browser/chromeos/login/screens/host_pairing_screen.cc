// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/login/screens/host_pairing_screen.h"

#include "base/command_line.h"
#include "base/logging.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/chromeos/login/startup_utils.h"
#include "chrome/browser/chromeos/login/wizard_controller.h"
#include "chrome/browser/chromeos/policy/browser_policy_connector_chromeos.h"
#include "chrome/browser/chromeos/policy/enrollment_status_chromeos.h"
#include "components/pairing/host_pairing_controller.h"
#include "google_apis/gaia/google_service_auth_error.h"

namespace chromeos {

namespace {

// Gets the fine-grained enrollment error code. It's calculated by concatenating
// |main_error_code| and |sub_error_code| strings together. The reason that
// string concatenation is preferred to arithmetic addition is that the number
// of sub error states is not necessarily a one-digit number and may have
// arbitrary digits.
int GetEnrollmentErrorCode(
    pairing_chromeos::HostPairingController::ErrorCode main_error_code,
    int sub_error_code) {
  return std::stoi(std::to_string(static_cast<int>(main_error_code)) +
                   std::to_string(sub_error_code));
}

}  // namespace

using namespace host_pairing;
using namespace pairing_chromeos;

HostPairingScreen::HostPairingScreen(
    BaseScreenDelegate* base_screen_delegate,
    Delegate* delegate,
    HostPairingScreenView* view,
    pairing_chromeos::HostPairingController* remora_controller)
    : BaseScreen(base_screen_delegate, OobeScreen::SCREEN_OOBE_HOST_PAIRING),
      delegate_(delegate),
      view_(view),
      remora_controller_(remora_controller),
      weak_ptr_factory_(this) {
  view_->SetDelegate(this);
  remora_controller_->AddObserver(this);
}

HostPairingScreen::~HostPairingScreen() {
  if (view_)
    view_->SetDelegate(NULL);
  remora_controller_->RemoveObserver(this);
}

void HostPairingScreen::CommitContextChanges() {
  if (!context_.HasChanges())
    return;
  base::DictionaryValue diff;
  context_.GetChangesAndReset(&diff);
  if (view_)
    view_->OnContextChanged(diff);
}

void HostPairingScreen::Show() {
  if (view_)
    view_->Show();
  PairingStageChanged(remora_controller_->GetCurrentStage());
}

void HostPairingScreen::Hide() {
  if (view_)
    view_->Hide();
}

void HostPairingScreen::PairingStageChanged(Stage new_stage) {
  std::string desired_page;
  switch (new_stage) {
    case HostPairingController::STAGE_INITIALIZATION_ERROR: {
      desired_page = kPageIntializationError;
      break;
    }
    case HostPairingController::STAGE_WAITING_FOR_CONTROLLER:
    case HostPairingController::STAGE_WAITING_FOR_CONTROLLER_AFTER_UPDATE: {
      desired_page = kPageWelcome;
      break;
    }
    case HostPairingController::STAGE_WAITING_FOR_CODE_CONFIRMATION: {
      desired_page = kPageCodeConfirmation;
      context_.SetString(kContextKeyConfirmationCode,
                         remora_controller_->GetConfirmationCode());
      break;
    }
    case HostPairingController::STAGE_CONTROLLER_CONNECTION_ERROR: {
      desired_page = kPageConnectionError;
      break;
    }
    case HostPairingController::STAGE_SETUP_BASIC_CONFIGURATION: {
      desired_page = kPageSetupBasicConfiguration;
      break;
    }
    case HostPairingController::STAGE_SETUP_NETWORK_ERROR: {
      desired_page = kPageSetupNetworkError;
      break;
    }
    case HostPairingController::STAGE_WAITING_FOR_CREDENTIALS: {
      desired_page = kPageEnrollmentIntroduction;
      break;
    }
    case HostPairingController::STAGE_ENROLLING: {
      desired_page = kPageEnrollment;
      context_.SetString(kContextKeyEnrollmentDomain,
                         remora_controller_->GetEnrollmentDomain());
      break;
    }
    case HostPairingController::STAGE_ENROLLMENT_SUCCESS: {
      remora_controller_->RemoveObserver(this);
      Finish(ScreenExitCode::ENTERPRISE_ENROLLMENT_COMPLETED);
      break;
    }
    case HostPairingController::STAGE_ENROLLMENT_ERROR: {
      // TODO(xdai): Maybe return to the Network Setup page?
      remora_controller_->RemoveObserver(this);
      desired_page = kPageEnrollmentError;
      context_.SetString(kContextKeyEnrollmentError, enrollment_error_string_);
      break;
    }
    default:
      break;
  }
  current_stage_ = new_stage;
  context_.SetString(kContextKeyDeviceName,
                     remora_controller_->GetDeviceName());
  context_.SetString(kContextKeyPage, desired_page);
  CommitContextChanges();
}

void HostPairingScreen::ConfigureHostRequested(
    bool accepted_eula,
    const std::string& lang,
    const std::string& timezone,
    bool send_reports,
    const std::string& keyboard_layout) {
  VLOG(1) << "ConfigureHostMessage language=" << lang
          << ", timezone=" << timezone
          << ", keyboard_layout=" << keyboard_layout;

  if (delegate_) {
    delegate_->ConfigureHostRequested(accepted_eula, lang, timezone,
                                      send_reports, keyboard_layout);
  }
}

void HostPairingScreen::AddNetworkRequested(const std::string& onc_spec) {
  if (delegate_)
    delegate_->AddNetworkRequested(onc_spec);
}

void HostPairingScreen::EnrollHostRequested(const std::string& auth_token) {
  policy::EnrollmentConfig enrollment_config =
      g_browser_process->platform_part()
          ->browser_policy_connector_chromeos()
          ->GetPrescribedEnrollmentConfig();
  enrollment_helper_ = EnterpriseEnrollmentHelper::Create(
      this, nullptr, enrollment_config, std::string());
  enrollment_helper_->EnrollUsingToken(auth_token);
  remora_controller_->OnEnrollmentStatusChanged(
      HostPairingController::ENROLLMENT_STATUS_ENROLLING);
}

void HostPairingScreen::RebootHostRequested() {
  if (delegate_)
    delegate_->RebootHostRequested();
}

void HostPairingScreen::OnViewDestroyed(HostPairingScreenView* view) {
  if (view_ == view)
    view_ = NULL;
}

void HostPairingScreen::OnAuthError(const GoogleServiceAuthError& error) {
  enrollment_error_string_ = view_->GetErrorStringFromAuthError(error);
  enrollment_error_code_ =
      GetEnrollmentErrorCode(HostPairingController::ErrorCode::AUTH_ERROR,
                             static_cast<int>(error.state()));
  OnAnyEnrollmentError();
}

void HostPairingScreen::OnMultipleLicensesAvailable(
    const EnrollmentLicenseMap& licenses) {
  LOG(ERROR) << "Host-paired enrollment is not yet compatible "
             << "with Mixed Licenses Enrollment Flow";
  enrollment_error_string_ = view_->GetErrorStringFromOtherError(
      EnterpriseEnrollmentHelper::OTHER_ERROR_FATAL);
  enrollment_error_code_ = GetEnrollmentErrorCode(
      HostPairingController::ErrorCode::OTHER_ERROR,
      static_cast<int>(EnterpriseEnrollmentHelper::OTHER_ERROR_FATAL));
  OnAnyEnrollmentError();
}

void HostPairingScreen::OnEnrollmentError(policy::EnrollmentStatus status) {
  enrollment_error_string_ = view_->GetErrorStringFromEnrollmentError(status);
  enrollment_error_code_ =
      GetEnrollmentErrorCode(HostPairingController::ErrorCode::ENROLL_ERROR,
                             static_cast<int>(status.status()));
  OnAnyEnrollmentError();
}

void HostPairingScreen::OnOtherError(
    EnterpriseEnrollmentHelper::OtherError error) {
  enrollment_error_string_ = view_->GetErrorStringFromOtherError(error);
  enrollment_error_code_ = GetEnrollmentErrorCode(
      HostPairingController::ErrorCode::OTHER_ERROR, static_cast<int>(error));
  OnAnyEnrollmentError();
}

void HostPairingScreen::OnDeviceEnrolled(const std::string& additional_token) {
  StartupUtils::MarkDeviceRegistered(base::DoNothing());
  enrollment_helper_->ClearAuth(base::Bind(&HostPairingScreen::OnAuthCleared,
                                           weak_ptr_factory_.GetWeakPtr()));

  policy::BrowserPolicyConnectorChromeOS* connector =
      g_browser_process->platform_part()->browser_policy_connector_chromeos();
  const enterprise_management::PolicyData* policy =
      connector->GetDeviceCloudPolicyManager()->core()->store()->policy();

  remora_controller_->SetPermanentId(policy->directory_api_id());
  remora_controller_->OnEnrollmentStatusChanged(
      HostPairingController::ENROLLMENT_STATUS_SUCCESS);
}

void HostPairingScreen::OnDeviceAttributeUploadCompleted(bool success) {}

void HostPairingScreen::OnDeviceAttributeUpdatePermission(bool granted) {}

void HostPairingScreen::OnAuthCleared() {
  enrollment_helper_.reset();
}

void HostPairingScreen::OnAnyEnrollmentError() {
  enrollment_helper_->ClearAuth(base::Bind(&HostPairingScreen::OnAuthCleared,
                                           weak_ptr_factory_.GetWeakPtr()));
  remora_controller_->SetErrorCodeAndMessage(enrollment_error_code_,
                                             enrollment_error_string_);
  remora_controller_->OnEnrollmentStatusChanged(
      HostPairingController::ENROLLMENT_STATUS_FAILURE);
}

}  // namespace chromeos
