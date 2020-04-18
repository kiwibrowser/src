// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/webui/chromeos/login/host_pairing_screen_handler.h"

#include "base/command_line.h"
#include "base/strings/string_util.h"
#include "chrome/browser/chromeos/login/oobe_screen.h"
#include "chrome/browser/chromeos/policy/enrollment_status_chromeos.h"
#include "chrome/grit/generated_resources.h"
#include "chromeos/chromeos_switches.h"
#include "components/login/localized_values_builder.h"
#include "components/policy/core/browser/cloud/message_util.h"
#include "google_apis/gaia/google_service_auth_error.h"
#include "ui/base/l10n/l10n_util.h"

namespace chromeos {

namespace {

const char kJsScreenPath[] = "login.HostPairingScreen";

const char kMethodContextChanged[] = "contextChanged";

// Sent from JS when screen is ready to receive context updates.
// TODO(dzhioev): Move 'contextReady' logic to the base screen handler when
// all screens migrate to context-based communications.
const char kCallbackContextReady[] = "contextReady";

}  // namespace

HostPairingScreenHandler::HostPairingScreenHandler()
    : BaseScreenHandler(kScreenId) {
  set_call_js_prefix(kJsScreenPath);
}

HostPairingScreenHandler::~HostPairingScreenHandler() {
  if (delegate_)
    delegate_->OnViewDestroyed(this);
}

void HostPairingScreenHandler::HandleContextReady() {
  js_context_ready_ = true;
  OnContextChanged(context_cache_.storage());
}

void HostPairingScreenHandler::Initialize() {
  if (!page_is_ready() || !delegate_)
    return;

  if (show_on_init_) {
    Show();
    show_on_init_ = false;
  }
}

void HostPairingScreenHandler::DeclareLocalizedValues(
    ::login::LocalizedValuesBuilder* builder) {
  // TODO(dzhioev): Move the prefix logic to the base screen handler after
  // migration.
  std::string prefix;
  base::RemoveChars(kJsScreenPath, ".", &prefix);

  // TODO(xdai): Clean up all unrelated strings and rename others if necessary.
  builder->Add(prefix + "WelcomeTitle", IDS_PAIRING_HOST_WELCOME_TITLE);
  builder->Add(prefix + "WelcomeText", IDS_PAIRING_HOST_WELCOME_TEXT);
  builder->Add(prefix + "ConfirmationTitle", IDS_SLAVE_CONFIRMATION_TITLE);
  builder->Add(prefix + "UpdatingTitle", IDS_PAIRING_HOST_UPDATING_TITLE);
  builder->Add(prefix + "UpdatingText", IDS_PAIRING_HOST_UPDATING_TEXT);
  builder->Add(prefix + "EnrollTitle", IDS_SLAVE_ENROLL_TITLE);
  builder->Add(prefix + "EnrollingTitle", IDS_SLAVE_ENROLLMENT_IN_PROGRESS);
  builder->Add(prefix + "DoneTitle", IDS_PAIRING_HOST_DONE_TITLE);
  builder->Add(prefix + "DoneText", IDS_PAIRING_HOST_DONE_TEXT);
  builder->Add(prefix + "EnrollmentErrorTitle",
               IDS_SLAVE_ENROLLMENT_ERROR_TITLE);
  builder->Add(prefix + "ErrorNeedsRestart",
               IDS_PAIRING_HOST_ERROR_NEED_RESTART_TEXT);
  builder->Add(prefix + "SetupBasicConfigTitle",
               IDS_HOST_SETUP_BASIC_CONFIGURATION_TITLE);
  builder->Add(prefix + "SetupNetworkErrorTitle",
               IDS_HOST_SETUP_NETWORK_ERROR_TITLE);
  builder->Add(prefix + "InitializationErrorTitle",
               IDS_PAIRING_HOST_INITIALIZATION_ERROR_TITLE);
  builder->Add(prefix + "ConnectionErrorTitle",
               IDS_PAIRING_HOST_CONNECTION_ERROR_TITLE);
  builder->Add(prefix + "ErrorNeedRestartText",
               IDS_PAIRING_HOST_ERROR_NEED_RESTART_TEXT);
  builder->Add(prefix + "ErrorNeedsRestart",
               IDS_PAIRING_HOST_ERROR_NEED_RESTART_TEXT);
}

void HostPairingScreenHandler::RegisterMessages() {
  AddPrefixedCallback(kCallbackContextReady,
                      &HostPairingScreenHandler::HandleContextReady);
}

void HostPairingScreenHandler::Show() {
  if (!page_is_ready()) {
    show_on_init_ = true;
    return;
  }
  ShowScreen(kScreenId);
}

void HostPairingScreenHandler::Hide() {
}

void HostPairingScreenHandler::SetDelegate(Delegate* delegate) {
  delegate_ = delegate;
  if (page_is_ready())
    Initialize();
}

void HostPairingScreenHandler::OnContextChanged(
    const base::DictionaryValue& diff) {
  if (!js_context_ready_) {
    context_cache_.ApplyChanges(diff, NULL);
    return;
  }
  CallJS(kMethodContextChanged, diff);
}

std::string HostPairingScreenHandler::GetErrorStringFromAuthError(
    const GoogleServiceAuthError& error) {
  switch (error.state()) {
    case GoogleServiceAuthError::NONE:
    case GoogleServiceAuthError::CAPTCHA_REQUIRED:
    case GoogleServiceAuthError::TWO_FACTOR:
    case GoogleServiceAuthError::INVALID_GAIA_CREDENTIALS:
    case GoogleServiceAuthError::REQUEST_CANCELED:
    case GoogleServiceAuthError::UNEXPECTED_SERVICE_RESPONSE:
    case GoogleServiceAuthError::SERVICE_ERROR:
    case GoogleServiceAuthError::WEB_LOGIN_REQUIRED:
      return l10n_util::GetStringUTF8(
          IDS_ENTERPRISE_ENROLLMENT_AUTH_FATAL_ERROR);
    case GoogleServiceAuthError::USER_NOT_SIGNED_UP:
    case GoogleServiceAuthError::ACCOUNT_DELETED:
    case GoogleServiceAuthError::ACCOUNT_DISABLED:
      return l10n_util::GetStringUTF8(
          IDS_ENTERPRISE_ENROLLMENT_AUTH_ACCOUNT_ERROR);
    case GoogleServiceAuthError::CONNECTION_FAILED:
    case GoogleServiceAuthError::SERVICE_UNAVAILABLE:
      return l10n_util::GetStringUTF8(
          IDS_ENTERPRISE_ENROLLMENT_AUTH_NETWORK_ERROR);
    default:
      return std::string();
  }
}

std::string HostPairingScreenHandler::GetErrorStringFromEnrollmentError(
    policy::EnrollmentStatus status) {
  switch (status.status()) {
    case policy::EnrollmentStatus::NO_STATE_KEYS:
      return l10n_util::GetStringUTF8(
          IDS_ENTERPRISE_ENROLLMENT_STATUS_NO_STATE_KEYS);
    case policy::EnrollmentStatus::REGISTRATION_FAILED:
      switch (status.client_status()) {
        case policy::DM_STATUS_SERVICE_MANAGEMENT_NOT_SUPPORTED:
          return l10n_util::GetStringUTF8(
              IDS_ENTERPRISE_ENROLLMENT_ACCOUNT_ERROR);
        case policy::DM_STATUS_SERVICE_MISSING_LICENSES:
          return l10n_util::GetStringUTF8(
              IDS_ENTERPRISE_ENROLLMENT_MISSING_LICENSES_ERROR);
        case policy::DM_STATUS_SERVICE_DEPROVISIONED:
          return l10n_util::GetStringUTF8(
              IDS_ENTERPRISE_ENROLLMENT_DEPROVISIONED_ERROR);
        case policy::DM_STATUS_SERVICE_DOMAIN_MISMATCH:
          return l10n_util::GetStringUTF8(
              IDS_ENTERPRISE_ENROLLMENT_DOMAIN_MISMATCH_ERROR);
        default:
          return l10n_util::GetStringFUTF8(
              IDS_ENTERPRISE_ENROLLMENT_STATUS_REGISTRATION_FAILED,
              policy::FormatDeviceManagementStatus(status.client_status()));
      }
    case policy::EnrollmentStatus::ROBOT_AUTH_FETCH_FAILED:
      return l10n_util::GetStringUTF8(
          IDS_ENTERPRISE_ENROLLMENT_ROBOT_AUTH_FETCH_FAILED);
    case policy::EnrollmentStatus::ROBOT_REFRESH_FETCH_FAILED:
      return l10n_util::GetStringUTF8(
          IDS_ENTERPRISE_ENROLLMENT_ROBOT_REFRESH_FETCH_FAILED);
    case policy::EnrollmentStatus::ROBOT_REFRESH_STORE_FAILED:
      return l10n_util::GetStringUTF8(
          IDS_ENTERPRISE_ENROLLMENT_ROBOT_REFRESH_STORE_FAILED);
    case policy::EnrollmentStatus::REGISTRATION_BAD_MODE:
      return l10n_util::GetStringUTF8(
          IDS_ENTERPRISE_ENROLLMENT_STATUS_REGISTRATION_BAD_MODE);
    case policy::EnrollmentStatus::REGISTRATION_CERT_FETCH_FAILED:
      return l10n_util::GetStringUTF8(
          IDS_ENTERPRISE_ENROLLMENT_STATUS_REGISTRATION_CERT_FETCH_FAILED);
    case policy::EnrollmentStatus::POLICY_FETCH_FAILED:
      return l10n_util::GetStringFUTF8(
          IDS_ENTERPRISE_ENROLLMENT_STATUS_POLICY_FETCH_FAILED,
          policy::FormatDeviceManagementStatus(status.client_status()));
    case policy::EnrollmentStatus::VALIDATION_FAILED:
      return l10n_util::GetStringFUTF8(
          IDS_ENTERPRISE_ENROLLMENT_STATUS_VALIDATION_FAILED,
          policy::FormatValidationStatus(status.validation_status()));
    case policy::EnrollmentStatus::LOCK_ERROR:
      switch (status.lock_status()) {
        case InstallAttributes::LOCK_TIMEOUT:
          return l10n_util::GetStringUTF8(
              IDS_ENTERPRISE_ENROLLMENT_STATUS_LOCK_TIMEOUT);
        case InstallAttributes::LOCK_BACKEND_INVALID:
        case InstallAttributes::LOCK_ALREADY_LOCKED:
        case InstallAttributes::LOCK_SET_ERROR:
        case InstallAttributes::LOCK_FINALIZE_ERROR:
        case InstallAttributes::LOCK_READBACK_ERROR:
          return l10n_util::GetStringUTF8(
              IDS_ENTERPRISE_ENROLLMENT_STATUS_LOCK_ERROR);
        case InstallAttributes::LOCK_WRONG_DOMAIN:
          return l10n_util::GetStringUTF8(
              IDS_ENTERPRISE_ENROLLMENT_STATUS_LOCK_WRONG_USER);
        case InstallAttributes::LOCK_WRONG_MODE:
          return l10n_util::GetStringUTF8(
              IDS_ENTERPRISE_ENROLLMENT_STATUS_LOCK_WRONG_MODE);
        default:
          return std::string();
      }
    case policy::EnrollmentStatus::STORE_ERROR:
      return l10n_util::GetStringFUTF8(
          IDS_ENTERPRISE_ENROLLMENT_STATUS_STORE_ERROR,
          policy::FormatStoreStatus(status.store_status(),
                                    status.validation_status()));
    case policy::EnrollmentStatus::ATTRIBUTE_UPDATE_FAILED:
      return l10n_util::GetStringUTF8(
          IDS_ENTERPRISE_ENROLLMENT_ATTRIBUTE_ERROR);
    case policy::EnrollmentStatus::NO_MACHINE_IDENTIFICATION:
      return l10n_util::GetStringUTF8(
          IDS_ENTERPRISE_ENROLLMENT_STATUS_NO_MACHINE_IDENTIFICATION);
    case policy::EnrollmentStatus::ACTIVE_DIRECTORY_POLICY_FETCH_FAILED:
      return l10n_util::GetStringUTF8(
          IDS_ENTERPRISE_ENROLLMENT_ERROR_ACTIVE_DIRECTORY_POLICY_FETCH);
    case policy::EnrollmentStatus::DM_TOKEN_STORE_FAILED:
      return l10n_util::GetStringUTF8(
          IDS_ENTERPRISE_ENROLLMENT_ERROR_SAVE_DEVICE_CONFIGURATION);
    default:
      return std::string();
  }
}

std::string HostPairingScreenHandler::GetErrorStringFromOtherError(
    EnterpriseEnrollmentHelper::OtherError error) {
  switch (error) {
    case EnterpriseEnrollmentHelper::OTHER_ERROR_DOMAIN_MISMATCH:
      return l10n_util::GetStringUTF8(
          IDS_ENTERPRISE_ENROLLMENT_STATUS_LOCK_WRONG_USER);
    case EnterpriseEnrollmentHelper::OTHER_ERROR_FATAL:
      return l10n_util::GetStringUTF8(
          IDS_ENTERPRISE_ENROLLMENT_FATAL_ENROLLMENT_ERROR);
    default:
      return std::string();
  }
}

}  // namespace chromeos
