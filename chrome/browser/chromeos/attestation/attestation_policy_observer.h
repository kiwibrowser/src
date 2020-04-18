// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CHROMEOS_ATTESTATION_ATTESTATION_POLICY_OBSERVER_H_
#define CHROME_BROWSER_CHROMEOS_ATTESTATION_ATTESTATION_POLICY_OBSERVER_H_

#include <memory>
#include <string>

#include "base/callback.h"
#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "chrome/browser/chromeos/settings/cros_settings.h"
#include "chromeos/attestation/attestation_constants.h"

namespace policy {
class CloudPolicyClient;
}

namespace chromeos {

class CrosSettings;
class CryptohomeClient;

namespace attestation {

class AttestationFlow;

// A class which observes policy changes and triggers device attestation work if
// necessary.
class AttestationPolicyObserver {
 public:
  // The observer immediately connects with CrosSettings to listen for policy
  // changes.  The CloudPolicyClient is used to upload the device certificate to
  // the server if one is created in response to policy changes; it must be in
  // the registered state.  This class does not take ownership of
  // |policy_client|.
  explicit AttestationPolicyObserver(policy::CloudPolicyClient* policy_client);

  // A constructor which allows custom CryptohomeClient and AttestationFlow
  // implementations.  Useful for testing.
  AttestationPolicyObserver(policy::CloudPolicyClient* policy_client,
                            CryptohomeClient* cryptohome_client,
                            AttestationFlow* attestation_flow);

  ~AttestationPolicyObserver();

  // Sets the retry limit in number of tries; useful in testing.
  void set_retry_limit(int limit) { retry_limit_ = limit; }
  // Sets the retry delay in seconds; useful in testing.
  void set_retry_delay(int retry_delay) { retry_delay_ = retry_delay; }

 private:
  // Called when the attestation setting changes.
  void AttestationSettingChanged();

  // Checks attestation policy and starts any necessary work.
  void Start();

  // Gets a new certificate for the Enterprise Machine Key (EMK).
  void GetNewCertificate();

  // Gets the existing EMK certificate and sends it to CheckCertificateExpiry.
  void GetExistingCertificate();

  // Checks if any certificate in the given pem_certificate_chain is expired
  // and, if so, gets a new one. If not renewing, calls CheckIfUploaded.
  void CheckCertificateExpiry(const std::string& pem_certificate_chain);

  // Uploads a machine certificate to the policy server.
  void UploadCertificate(const std::string& pem_certificate_chain);

  // Checks if a certificate has already been uploaded and, if not, upload.
  void CheckIfUploaded(const std::string& pem_certificate_chain,
                       const std::string& key_payload);

  // Gets the payload associated with the EMK and sends it to |callback|.
  void GetKeyPayload(base::Callback<void(const std::string&)> callback);

  // Called when a certificate upload operation completes.  On success, |status|
  // will be true.
  void OnUploadComplete(bool status);

  // Marks a key as uploaded in the payload proto.
  void MarkAsUploaded(const std::string& key_payload);

  // Handles failure of getting a certificate.
  void HandleGetCertificateFailure(AttestationStatus status);

  // Reschedules a policy check (i.e. a call to Start) for a later time.
  // TODO(dkrahn): A better solution would be to wait for a dbus signal which
  // indicates the system is ready to process this task. See crbug.com/256845.
  void Reschedule();

  CrosSettings* cros_settings_;
  policy::CloudPolicyClient* policy_client_;
  CryptohomeClient* cryptohome_client_;
  AttestationFlow* attestation_flow_;
  std::unique_ptr<AttestationFlow> default_attestation_flow_;
  int num_retries_;
  int retry_limit_;
  int retry_delay_;

  std::unique_ptr<CrosSettings::ObserverSubscription> attestation_subscription_;

  // Note: This should remain the last member so it'll be destroyed and
  // invalidate the weak pointers before any other members are destroyed.
  base::WeakPtrFactory<AttestationPolicyObserver> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(AttestationPolicyObserver);
};

}  // namespace attestation
}  // namespace chromeos

#endif  // CHROME_BROWSER_CHROMEOS_ATTESTATION_ATTESTATION_POLICY_OBSERVER_H_
