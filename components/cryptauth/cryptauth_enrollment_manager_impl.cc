// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/cryptauth/cryptauth_enrollment_manager_impl.h"

#include <memory>
#include <utility>

#include "base/base64url.h"
#include "base/memory/ptr_util.h"
#include "base/time/clock.h"
#include "base/time/time.h"
#include "chromeos/components/proximity_auth/logging/logging.h"
#include "components/cryptauth/cryptauth_enroller.h"
#include "components/cryptauth/pref_names.h"
#include "components/cryptauth/secure_message_delegate.h"
#include "components/cryptauth/sync_scheduler_impl.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/pref_service.h"

namespace cryptauth {

namespace {

// The number of days that an enrollment is valid. Note that we try to refresh
// the enrollment well before this time elapses.
const int kValidEnrollmentPeriodDays = 45;

// The normal period between successful enrollments in days.
const int kEnrollmentRefreshPeriodDays = 30;

// A more aggressive period between enrollments to recover when the last
// enrollment fails, in minutes. This is a base time that increases for each
// subsequent failure.
const int kEnrollmentBaseRecoveryPeriodMinutes = 10;

// The bound on the amount to jitter the period between enrollments.
const double kEnrollmentMaxJitterRatio = 0.2;

// The value of the device_software_package field in the device info uploaded
// during enrollment. This value must be the same as the app id used for GCM
// registration.
const char kDeviceSoftwarePackage[] = "com.google.chrome.cryptauth";

std::unique_ptr<SyncScheduler> CreateSyncScheduler(
    SyncScheduler::Delegate* delegate) {
  return std::make_unique<SyncSchedulerImpl>(
      delegate, base::TimeDelta::FromDays(kEnrollmentRefreshPeriodDays),
      base::TimeDelta::FromMinutes(kEnrollmentBaseRecoveryPeriodMinutes),
      kEnrollmentMaxJitterRatio, "CryptAuth Enrollment");
}

}  // namespace

// static
CryptAuthEnrollmentManagerImpl::Factory*
    CryptAuthEnrollmentManagerImpl::Factory::factory_instance_ = nullptr;

// static
std::unique_ptr<CryptAuthEnrollmentManager>
CryptAuthEnrollmentManagerImpl::Factory::NewInstance(
    base::Clock* clock,
    std::unique_ptr<CryptAuthEnrollerFactory> enroller_factory,
    std::unique_ptr<SecureMessageDelegate> secure_message_delegate,
    const GcmDeviceInfo& device_info,
    CryptAuthGCMManager* gcm_manager,
    PrefService* pref_service) {
  if (!factory_instance_)
    factory_instance_ = new Factory();

  return factory_instance_->BuildInstance(
      clock, std::move(enroller_factory), std::move(secure_message_delegate),
      device_info, gcm_manager, pref_service);
}

// static
void CryptAuthEnrollmentManagerImpl::Factory::SetInstanceForTesting(
    Factory* factory) {
  factory_instance_ = factory;
}

CryptAuthEnrollmentManagerImpl::Factory::~Factory() = default;

std::unique_ptr<CryptAuthEnrollmentManager>
CryptAuthEnrollmentManagerImpl::Factory::BuildInstance(
    base::Clock* clock,
    std::unique_ptr<CryptAuthEnrollerFactory> enroller_factory,
    std::unique_ptr<SecureMessageDelegate> secure_message_delegate,
    const GcmDeviceInfo& device_info,
    CryptAuthGCMManager* gcm_manager,
    PrefService* pref_service) {
  return base::WrapUnique(new CryptAuthEnrollmentManagerImpl(
      clock, std::move(enroller_factory), std::move(secure_message_delegate),
      device_info, gcm_manager, pref_service));
}

CryptAuthEnrollmentManagerImpl::CryptAuthEnrollmentManagerImpl(
    base::Clock* clock,
    std::unique_ptr<CryptAuthEnrollerFactory> enroller_factory,
    std::unique_ptr<SecureMessageDelegate> secure_message_delegate,
    const GcmDeviceInfo& device_info,
    CryptAuthGCMManager* gcm_manager,
    PrefService* pref_service)
    : clock_(clock),
      enroller_factory_(std::move(enroller_factory)),
      secure_message_delegate_(std::move(secure_message_delegate)),
      device_info_(device_info),
      gcm_manager_(gcm_manager),
      pref_service_(pref_service),
      scheduler_(CreateSyncScheduler(this /* delegate */)),
      weak_ptr_factory_(this) {}

CryptAuthEnrollmentManagerImpl::~CryptAuthEnrollmentManagerImpl() {
  gcm_manager_->RemoveObserver(this);
}

void CryptAuthEnrollmentManagerImpl::Start() {
  gcm_manager_->AddObserver(this);

  bool is_recovering_from_failure =
      pref_service_->GetBoolean(
          prefs::kCryptAuthEnrollmentIsRecoveringFromFailure) ||
      !IsEnrollmentValid();

  base::Time last_successful_enrollment = GetLastEnrollmentTime();
  base::TimeDelta elapsed_time_since_last_sync =
      clock_->Now() - last_successful_enrollment;

  scheduler_->Start(elapsed_time_since_last_sync,
                    is_recovering_from_failure
                        ? SyncScheduler::Strategy::AGGRESSIVE_RECOVERY
                        : SyncScheduler::Strategy::PERIODIC_REFRESH);
}

void CryptAuthEnrollmentManagerImpl::ForceEnrollmentNow(
    InvocationReason invocation_reason) {
  // We store the invocation reason in a preference so that it can persist
  // across browser restarts. If the sync fails, the next retry should still use
  // this original reason instead of INVOCATION_REASON_FAILURE_RECOVERY.
  pref_service_->SetInteger(prefs::kCryptAuthEnrollmentReason,
                            invocation_reason);
  scheduler_->ForceSync();
}

bool CryptAuthEnrollmentManagerImpl::IsEnrollmentValid() const {
  base::Time last_enrollment_time = GetLastEnrollmentTime();
  return !last_enrollment_time.is_null() &&
         (clock_->Now() - last_enrollment_time) <
             base::TimeDelta::FromDays(kValidEnrollmentPeriodDays);
}

base::Time CryptAuthEnrollmentManagerImpl::GetLastEnrollmentTime() const {
  return base::Time::FromDoubleT(pref_service_->GetDouble(
      prefs::kCryptAuthEnrollmentLastEnrollmentTimeSeconds));
}

base::TimeDelta CryptAuthEnrollmentManagerImpl::GetTimeToNextAttempt() const {
  return scheduler_->GetTimeToNextSync();
}

bool CryptAuthEnrollmentManagerImpl::IsEnrollmentInProgress() const {
  return scheduler_->GetSyncState() ==
         SyncScheduler::SyncState::SYNC_IN_PROGRESS;
}

bool CryptAuthEnrollmentManagerImpl::IsRecoveringFromFailure() const {
  return scheduler_->GetStrategy() ==
         SyncScheduler::Strategy::AGGRESSIVE_RECOVERY;
}

void CryptAuthEnrollmentManagerImpl::OnEnrollmentFinished(bool success) {
  if (success) {
    pref_service_->SetDouble(
        prefs::kCryptAuthEnrollmentLastEnrollmentTimeSeconds,
        clock_->Now().ToDoubleT());
    pref_service_->SetInteger(prefs::kCryptAuthEnrollmentReason,
                              INVOCATION_REASON_UNKNOWN);
  }

  pref_service_->SetBoolean(prefs::kCryptAuthEnrollmentIsRecoveringFromFailure,
                            !success);

  sync_request_->OnDidComplete(success);
  cryptauth_enroller_.reset();
  sync_request_.reset();

  NotifyEnrollmentFinished(success);
}

std::string CryptAuthEnrollmentManagerImpl::GetUserPublicKey() const {
  std::string public_key;
  if (!base::Base64UrlDecode(
          pref_service_->GetString(prefs::kCryptAuthEnrollmentUserPublicKey),
          base::Base64UrlDecodePolicy::REQUIRE_PADDING, &public_key)) {
    PA_LOG(ERROR) << "Invalid public key stored in user prefs.";
    return std::string();
  }
  return public_key;
}

std::string CryptAuthEnrollmentManagerImpl::GetUserPrivateKey() const {
  std::string private_key;
  if (!base::Base64UrlDecode(
          pref_service_->GetString(prefs::kCryptAuthEnrollmentUserPrivateKey),
          base::Base64UrlDecodePolicy::REQUIRE_PADDING, &private_key)) {
    PA_LOG(ERROR) << "Invalid private key stored in user prefs.";
    return std::string();
  }
  return private_key;
}

void CryptAuthEnrollmentManagerImpl::SetSyncSchedulerForTest(
    std::unique_ptr<SyncScheduler> sync_scheduler) {
  scheduler_ = std::move(sync_scheduler);
}

void CryptAuthEnrollmentManagerImpl::OnGCMRegistrationResult(bool success) {
  if (!sync_request_)
    return;

  PA_LOG(INFO) << "GCM registration for CryptAuth Enrollment completed: "
               << success;
  if (success)
    DoCryptAuthEnrollment();
  else
    OnEnrollmentFinished(false);
}

void CryptAuthEnrollmentManagerImpl::OnKeyPairGenerated(
    const std::string& public_key,
    const std::string& private_key) {
  if (!public_key.empty() && !private_key.empty()) {
    PA_LOG(INFO) << "Key pair generated for CryptAuth enrollment";
    // Store the keypair in Base64 format because pref values require readable
    // string values.
    std::string public_key_b64, private_key_b64;
    base::Base64UrlEncode(public_key,
                          base::Base64UrlEncodePolicy::INCLUDE_PADDING,
                          &public_key_b64);
    base::Base64UrlEncode(private_key,
                          base::Base64UrlEncodePolicy::INCLUDE_PADDING,
                          &private_key_b64);
    pref_service_->SetString(prefs::kCryptAuthEnrollmentUserPublicKey,
                             public_key_b64);
    pref_service_->SetString(prefs::kCryptAuthEnrollmentUserPrivateKey,
                             private_key_b64);
    DoCryptAuthEnrollment();
  } else {
    OnEnrollmentFinished(false);
  }
}

void CryptAuthEnrollmentManagerImpl::OnReenrollMessage() {
  ForceEnrollmentNow(INVOCATION_REASON_SERVER_INITIATED);
}

void CryptAuthEnrollmentManagerImpl::OnSyncRequested(
    std::unique_ptr<SyncScheduler::SyncRequest> sync_request) {
  NotifyEnrollmentStarted();

  sync_request_ = std::move(sync_request);
  if (gcm_manager_->GetRegistrationId().empty() ||
      pref_service_->GetInteger(prefs::kCryptAuthEnrollmentReason) ==
          INVOCATION_REASON_MANUAL) {
    gcm_manager_->RegisterWithGCM();
  } else {
    DoCryptAuthEnrollment();
  }
}

void CryptAuthEnrollmentManagerImpl::DoCryptAuthEnrollment() {
  if (GetUserPublicKey().empty() || GetUserPrivateKey().empty()) {
    secure_message_delegate_->GenerateKeyPair(
        base::Bind(&CryptAuthEnrollmentManagerImpl::OnKeyPairGenerated,
                   weak_ptr_factory_.GetWeakPtr()));
  } else {
    DoCryptAuthEnrollmentWithKeys();
  }
}

void CryptAuthEnrollmentManagerImpl::DoCryptAuthEnrollmentWithKeys() {
  DCHECK(sync_request_);
  InvocationReason invocation_reason = INVOCATION_REASON_UNKNOWN;

  int reason_stored_in_prefs =
      pref_service_->GetInteger(prefs::kCryptAuthEnrollmentReason);

  if (InvocationReason_IsValid(reason_stored_in_prefs) &&
      reason_stored_in_prefs != INVOCATION_REASON_UNKNOWN) {
    invocation_reason = static_cast<InvocationReason>(reason_stored_in_prefs);
  } else if (GetLastEnrollmentTime().is_null()) {
    invocation_reason = INVOCATION_REASON_INITIALIZATION;
  } else if (!IsEnrollmentValid()) {
    invocation_reason = INVOCATION_REASON_EXPIRATION;
  } else if (scheduler_->GetStrategy() ==
             SyncScheduler::Strategy::PERIODIC_REFRESH) {
    invocation_reason = INVOCATION_REASON_PERIODIC;
  } else if (scheduler_->GetStrategy() ==
             SyncScheduler::Strategy::AGGRESSIVE_RECOVERY) {
    invocation_reason = INVOCATION_REASON_FAILURE_RECOVERY;
  }

  // Fill in the current GCM registration id before enrolling, and explicitly
  // make sure that the software package is the same as the GCM app id.
  GcmDeviceInfo device_info(device_info_);
  device_info.set_gcm_registration_id(gcm_manager_->GetRegistrationId());
  device_info.set_device_software_package(kDeviceSoftwarePackage);

  std::string public_key_b64;
  base::Base64UrlEncode(GetUserPublicKey(),
                        base::Base64UrlEncodePolicy::INCLUDE_PADDING,
                        &public_key_b64);
  PA_LOG(INFO) << "Making enrollment:\n"
               << "  public_key: " << public_key_b64 << "\n"
               << "  invocation_reason: " << invocation_reason << "\n"
               << "  gcm_registration_id: "
               << device_info.gcm_registration_id();

  cryptauth_enroller_ = enroller_factory_->CreateInstance();
  cryptauth_enroller_->Enroll(
      GetUserPublicKey(), GetUserPrivateKey(), device_info, invocation_reason,
      base::Bind(&CryptAuthEnrollmentManagerImpl::OnEnrollmentFinished,
                 weak_ptr_factory_.GetWeakPtr()));
}

}  // namespace cryptauth
