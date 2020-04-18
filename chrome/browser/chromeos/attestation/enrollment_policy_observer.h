// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CHROMEOS_ATTESTATION_ENROLLMENT_POLICY_OBSERVER_H_
#define CHROME_BROWSER_CHROMEOS_ATTESTATION_ENROLLMENT_POLICY_OBSERVER_H_

#include <memory>
#include <string>

#include "base/callback.h"
#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "chrome/browser/chromeos/settings/device_settings_service.h"
#include "chromeos/attestation/attestation_constants.h"

namespace policy {
class CloudPolicyClient;
}

namespace chromeos {

class CryptohomeClient;

namespace attestation {

class AttestationFlow;

// A class which observes policy changes and triggers uploading identification
// for enrollment if necessary.
class EnrollmentPolicyObserver : public DeviceSettingsService::Observer {
 public:
  // The observer immediately connects with DeviceSettingsService to listen for
  // policy changes.  The CloudPolicyClient is used to upload data to the
  // server; it must be in the registered state.  This class does not take
  // ownership of |policy_client|.
  explicit EnrollmentPolicyObserver(policy::CloudPolicyClient* policy_client);

  // A constructor which accepts custom instances useful for testing.
  EnrollmentPolicyObserver(policy::CloudPolicyClient* policy_client,
                           DeviceSettingsService* device_settings_service,
                           CryptohomeClient* cryptohome_client,
                           AttestationFlow* attestation_flow);

  ~EnrollmentPolicyObserver() override;

  // Sets the retry limit in number of tries; useful in testing.
  void set_retry_limit(int limit) { retry_limit_ = limit; }
  // Sets the retry delay in seconds; useful in testing.
  void set_retry_delay(int retry_delay) { retry_delay_ = retry_delay; }

 private:
  // Called when the device settings change.
  void DeviceSettingsUpdated() override;

  // Checks enrollment setting and starts any necessary work.
  void Start();

  // Gets an enrollment certificate.
  void GetEnrollmentCertificate();

  // Gets an enrollment identifier directly.
  void GetEnrollmentId();

  // Handles an enrollment identifer obtained directly.
  void HandleEnrollmentId(const std::string& enrollment_id);

  // Reschedule an attempt to get an enrollment identifier directly.
  void RescheduleGetEnrollmentId();

  // Handles a failure to get a certificate.
  void HandleGetCertificateFailure(AttestationStatus status);

  // Uploads an enrollment certificate to the policy server.
  void UploadCertificate(const std::string& pem_certificate_chain);

  // Called when a certificate or enrollment identifier upload operation
  // completes. On success, |status| will be true. The string |what| is
  // used in logging.
  void OnUploadComplete(const std::string& what, bool status);

  DeviceSettingsService* device_settings_service_;
  policy::CloudPolicyClient* policy_client_;
  CryptohomeClient* cryptohome_client_;
  AttestationFlow* attestation_flow_;
  std::unique_ptr<AttestationFlow> default_attestation_flow_;
  int num_retries_;
  int retry_limit_;
  int retry_delay_;

  // Note: This should remain the last member so it'll be destroyed and
  // invalidate the weak pointers before any other members are destroyed.
  base::WeakPtrFactory<EnrollmentPolicyObserver> weak_factory_;

  friend class EnrollmentPolicyObserverTest;

  DISALLOW_COPY_AND_ASSIGN(EnrollmentPolicyObserver);
};

}  // namespace attestation
}  // namespace chromeos

#endif  // CHROME_BROWSER_CHROMEOS_ATTESTATION_ENROLLMENT_POLICY_OBSERVER_H_
