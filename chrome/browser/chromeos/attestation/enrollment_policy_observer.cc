// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/attestation/enrollment_policy_observer.h"

#include <utility>

#include "base/bind.h"
#include "base/callback.h"
#include "base/location.h"
#include "base/optional.h"
#include "base/time/time.h"
#include "chrome/browser/chrome_notification_types.h"
#include "chrome/browser/chromeos/attestation/attestation_ca_client.h"
#include "chrome/browser/chromeos/attestation/attestation_key_payload.pb.h"
#include "chrome/browser/chromeos/settings/cros_settings.h"
#include "chromeos/attestation/attestation_flow.h"
#include "chromeos/cryptohome/async_method_caller.h"
#include "chromeos/cryptohome/cryptohome_parameters.h"
#include "chromeos/dbus/cryptohome_client.h"
#include "chromeos/dbus/dbus_method_call_status.h"
#include "chromeos/dbus/dbus_thread_manager.h"
#include "components/account_id/account_id.h"
#include "components/policy/core/common/cloud/cloud_policy_client.h"
#include "components/policy/core/common/cloud/cloud_policy_manager.h"
#include "components/user_manager/known_user.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/notification_details.h"
#include "net/cert/pem_tokenizer.h"
#include "net/cert/x509_certificate.h"

namespace {

const int kRetryDelay = 5;  // Seconds.
const int kRetryLimit = 100;

// A dbus callback which handles a string result.
//
// Parameters
//   on_success - Called when result is successful and has a value.
//   on_failure - Called otherwise.
void DBusStringCallback(
    base::OnceCallback<void(const std::string&)> on_success,
    base::OnceClosure on_failure,
    const base::Location& from_here,
    base::Optional<chromeos::CryptohomeClient::TpmAttestationDataResult>
        result) {
  if (!result.has_value() || !result->success) {
    LOG(ERROR) << "Cryptohome DBus method failed: " << from_here.ToString();
    if (!on_failure.is_null())
      std::move(on_failure).Run();
    return;
  }
  std::move(on_success).Run(result->data);
}

void DBusPrivacyCACallback(
    const base::RepeatingCallback<void(const std::string&)> on_success,
    const base::RepeatingCallback<
        void(chromeos::attestation::AttestationStatus)> on_failure,
    const base::Location& from_here,
    chromeos::attestation::AttestationStatus status,
    const std::string& data) {
  if (status == chromeos::attestation::ATTESTATION_SUCCESS) {
    on_success.Run(data);
    return;
  }
  LOG(ERROR) << "Cryptohome DBus method or server call failed with status: "
             << status << ": " << from_here.ToString();
  if (!on_failure.is_null())
    on_failure.Run(status);
}

}  // namespace

namespace chromeos {
namespace attestation {

EnrollmentPolicyObserver::EnrollmentPolicyObserver(
    policy::CloudPolicyClient* policy_client)
    : device_settings_service_(DeviceSettingsService::Get()),
      policy_client_(policy_client),
      cryptohome_client_(nullptr),
      attestation_flow_(nullptr),
      num_retries_(0),
      retry_limit_(kRetryLimit),
      retry_delay_(kRetryDelay),
      weak_factory_(this) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  device_settings_service_->AddObserver(this);
  Start();
}

EnrollmentPolicyObserver::EnrollmentPolicyObserver(
    policy::CloudPolicyClient* policy_client,
    DeviceSettingsService* device_settings_service,
    CryptohomeClient* cryptohome_client,
    AttestationFlow* attestation_flow)
    : device_settings_service_(device_settings_service),
      policy_client_(policy_client),
      cryptohome_client_(cryptohome_client),
      attestation_flow_(attestation_flow),
      num_retries_(0),
      retry_delay_(kRetryDelay),
      weak_factory_(this) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  device_settings_service_->AddObserver(this);
  Start();
}

EnrollmentPolicyObserver::~EnrollmentPolicyObserver() {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  DCHECK(DeviceSettingsService::IsInitialized());
  device_settings_service_->RemoveObserver(this);
}

void EnrollmentPolicyObserver::DeviceSettingsUpdated() {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  num_retries_ = 0;
  Start();
}

void EnrollmentPolicyObserver::Start() {
  // If identification for enrollment isn't needed, there is nothing to do.
  const enterprise_management::PolicyData* policy_data =
      device_settings_service_->policy_data();
  if (!policy_data || !policy_data->enrollment_id_needed())
    return;

  // We expect a registered CloudPolicyClient.
  if (!policy_client_->is_registered()) {
    LOG(ERROR) << "EnrollmentPolicyObserver: Invalid CloudPolicyClient.";
    return;
  }

  if (!cryptohome_client_)
    cryptohome_client_ = DBusThreadManager::Get()->GetCryptohomeClient();

  if (!attestation_flow_) {
    std::unique_ptr<ServerProxy> attestation_ca_client(
        new AttestationCAClient());
    default_attestation_flow_.reset(new AttestationFlow(
        cryptohome::AsyncMethodCaller::GetInstance(), cryptohome_client_,
        std::move(attestation_ca_client)));
    attestation_flow_ = default_attestation_flow_.get();
  }

  GetEnrollmentCertificate();
}

void EnrollmentPolicyObserver::GetEnrollmentCertificate() {
  // We can reuse the dbus callback handler logic.
  attestation_flow_->GetCertificate(
      PROFILE_ENTERPRISE_ENROLLMENT_CERTIFICATE,
      EmptyAccountId(),  // Not used.
      std::string(),     // Not used.
      true,              // Get a fresh certificate.
      base::Bind(
          [](const base::RepeatingCallback<void(const std::string&)> on_success,
             const base::RepeatingCallback<void(
                 chromeos::attestation::AttestationStatus)> on_failure,
             const base::Location& from_here, AttestationStatus status,
             const std::string& data) {
            DBusPrivacyCACallback(on_success, on_failure, from_here, status,
                                  std::move(data));
          },
          base::BindRepeating(&EnrollmentPolicyObserver::UploadCertificate,
                              weak_factory_.GetWeakPtr()),
          base::BindRepeating(
              &EnrollmentPolicyObserver::HandleGetCertificateFailure,
              weak_factory_.GetWeakPtr()),
          FROM_HERE));
}

void EnrollmentPolicyObserver::GetEnrollmentId() {
  cryptohome_client_->TpmAttestationGetEnrollmentId(
      true /* ignore_cache */,
      base::BindOnce(
          DBusStringCallback,
          base::BindOnce(&EnrollmentPolicyObserver::HandleEnrollmentId,
                         weak_factory_.GetWeakPtr()),
          base::BindOnce(&EnrollmentPolicyObserver::RescheduleGetEnrollmentId,
                         weak_factory_.GetWeakPtr()),
          FROM_HERE));
}

void EnrollmentPolicyObserver::HandleEnrollmentId(
    const std::string& enrollment_id) {
  if (enrollment_id.empty()) {
    LOG(WARNING) << "EnrollmentPolicyObserver: The enrollment identifier"
                    " obtained is empty.";
  }
  policy_client_->UploadEnterpriseEnrollmentId(
      enrollment_id,
      base::BindRepeating(&EnrollmentPolicyObserver::OnUploadComplete,
                          weak_factory_.GetWeakPtr(), "Enrollment Identifier"));
}

void EnrollmentPolicyObserver::RescheduleGetEnrollmentId() {
  if (++num_retries_ < retry_limit_) {
    content::BrowserThread::PostDelayedTask(
        content::BrowserThread::UI, FROM_HERE,
        base::BindOnce(&EnrollmentPolicyObserver::GetEnrollmentId,
                       weak_factory_.GetWeakPtr()),
        base::TimeDelta::FromSeconds(retry_delay_));
  } else {
    LOG(WARNING) << "EnrollmentPolicyObserver: Retry limit exceeded.";
  }
}

void EnrollmentPolicyObserver::HandleGetCertificateFailure(
    AttestationStatus status) {
  if (status == ATTESTATION_SERVER_BAD_REQUEST_FAILURE) {
    // We cannot get an enrollment cert (no EID). However we can compute the
    // EID we will have after a device wipe, and should upload that.
    GetEnrollmentId();
  } else if (++num_retries_ < retry_limit_) {
    content::BrowserThread::PostDelayedTask(
        content::BrowserThread::UI, FROM_HERE,
        base::BindOnce(&EnrollmentPolicyObserver::Start,
                       weak_factory_.GetWeakPtr()),
        base::TimeDelta::FromSeconds(retry_delay_));
  } else {
    LOG(WARNING) << "EnrollmentPolicyObserver: Retry limit exceeded.";
  }
}

void EnrollmentPolicyObserver::UploadCertificate(
    const std::string& pem_certificate_chain) {
  policy_client_->UploadEnterpriseEnrollmentCertificate(
      pem_certificate_chain,
      base::BindRepeating(&EnrollmentPolicyObserver::OnUploadComplete,
                          weak_factory_.GetWeakPtr(),
                          "Enterprise Enrollment Certificate"));
}

void EnrollmentPolicyObserver::OnUploadComplete(const std::string& what,
                                                bool status) {
  if (!status) {
    LOG(ERROR) << "Failed to upload " << what << " to DMServer.";
    return;
  }
  VLOG(1) << what << " uploaded to DMServer.";
}

}  // namespace attestation
}  // namespace chromeos
