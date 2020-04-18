// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROMEOS_CRYPTOHOME_ASYNC_METHOD_CALLER_H_
#define CHROMEOS_CRYPTOHOME_ASYNC_METHOD_CALLER_H_

#include <string>

#include "base/callback_forward.h"
#include "chromeos/attestation/attestation_constants.h"
#include "chromeos/chromeos_export.h"
#include "chromeos/dbus/cryptohome_client.h"
#include "third_party/cros_system_api/dbus/service_constants.h"

namespace cryptohome {

class Identification;

// Note: This file is placed in ::cryptohome instead of ::chromeos::cryptohome
// since there is already a namespace ::cryptohome which holds the error code
// enum (MountError) and referencing ::chromeos::cryptohome and ::cryptohome
// within the same code is confusing.

// This class manages calls to Cryptohome service's 'async' methods.
class CHROMEOS_EXPORT AsyncMethodCaller {
 public:
  // A callback type which is called back on the UI thread when the results of
  // method calls are ready.
  typedef base::Callback<void(bool success, MountError return_code)> Callback;
  typedef base::Callback<void(bool success, const std::string& data)>
      DataCallback;

  virtual ~AsyncMethodCaller() {}

  // Asks cryptohomed to asynchronously try to find the cryptohome for
  // |user_id| and then change from using |old_hash| to lock the
  // key to using |new_hash|.
  // |callback| will be called with status info on completion.
  virtual void AsyncMigrateKey(const Identification& user_id,
                               const std::string& old_hash,
                               const std::string& new_hash,
                               Callback callback) = 0;

  // Asks cryptohomed to asynchronously to mount a tmpfs for guest mode.
  // |callback| will be called with status info on completion.
  virtual void AsyncMountGuest(Callback callback) = 0;

  // Asks cryptohomed to asynchronously try to find the cryptohome for
  // |user_id| and then nuke it.
  virtual void AsyncRemove(const Identification& user_id,
                           Callback callback) = 0;

  // Asks cryptohomed to asynchronously create an attestation enrollment
  // request.  On success the data sent to |callback| is a request to be sent
  // to the Privacy CA of type |pca_type|.
  virtual void AsyncTpmAttestationCreateEnrollRequest(
      chromeos::attestation::PrivacyCAType pca_type,
      const DataCallback& callback) = 0;

  // Asks cryptohomed to asynchronously finish an attestation enrollment.
  // |pca_response| is the response to the enrollment request emitted by the
  // Privacy CA of type |pca_type|.
  virtual void AsyncTpmAttestationEnroll(
      chromeos::attestation::PrivacyCAType pca_type,
      const std::string& pca_response,
      const Callback& callback) = 0;

  // Asks cryptohomed to asynchronously create an attestation certificate
  // request according to |certificate_profile|.  Some profiles require that the
  // |user_id| of the currently active user and an identifier of the
  // |request_origin| be provided.  On success the data sent to |callback| is a
  // request to be sent to the Privacy CA of type |pca_type|.  The
  // |request_origin| may be sent to the Privacy CA but the |user_id| will never
  // be sent.
  virtual void AsyncTpmAttestationCreateCertRequest(
      chromeos::attestation::PrivacyCAType pca_type,
      chromeos::attestation::AttestationCertificateProfile certificate_profile,
      const Identification& user_id,
      const std::string& request_origin,
      const DataCallback& callback) = 0;

  // Asks cryptohomed to asynchronously finish an attestation certificate
  // request.  On success the data sent to |callback| is a certificate chain
  // in PEM format.  |pca_response| is the response to the certificate request
  // emitted by the Privacy CA.  |key_type| determines whether the certified key
  // is to be associated with the current user.  |key_name| is a name for the
  // key.  If |key_type| is KEY_USER, a |user_id| must be provided.  Otherwise
  // |user_id| is ignored.  For normal GAIA users the |user_id| is
  // an AccountId-derived string (see AccountId::GetAccountIdKey).
  virtual void AsyncTpmAttestationFinishCertRequest(
      const std::string& pca_response,
      chromeos::attestation::AttestationKeyType key_type,
      const Identification& user_id,
      const std::string& key_name,
      const DataCallback& callback) = 0;

  // Asks cryptohomed to asynchronously register the attestation key specified
  // by |key_type| and |key_name|.  If |key_type| is KEY_USER, a |user_id| must
  // be provided.  Otherwise |user_id| is ignored.  For normal GAIA users the
  // |user_id| is an AccountId-derived string (see AccountId::GetAccountIdKey).
  virtual void TpmAttestationRegisterKey(
      chromeos::attestation::AttestationKeyType key_type,
      const Identification& user_id,
      const std::string& key_name,
      const Callback& callback) = 0;

  // Asks cryptohomed to asynchronously sign an enterprise challenge with the
  // key specified by |key_type| and |key_name|.  The |domain| and |device_id|
  // parameters will be included in the challenge response.  |challenge| must be
  // a valid enterprise challenge.  On success, the data sent to |callback| is
  // the challenge response.  If |key_type| is KEY_USER, a |user_id| must be
  // provided.  Otherwise |user_id| is ignored.  For normal GAIA users the
  // |user_id| is an AccountaId-derived string (see AccountId::GetAccountIdKey).
  virtual void TpmAttestationSignEnterpriseChallenge(
      chromeos::attestation::AttestationKeyType key_type,
      const Identification& user_id,
      const std::string& key_name,
      const std::string& domain,
      const std::string& device_id,
      chromeos::attestation::AttestationChallengeOptions options,
      const std::string& challenge,
      const DataCallback& callback) = 0;

  // Asks cryptohomed to asynchronously sign a simple challenge with the key
  // specified by |key_type| and |key_name|.  |challenge| can be any arbitrary
  // set of bytes.  On success, the data sent to |callback| is the challenge
  // response.  If |key_type| is KEY_USER, a |user_id| must be provided.
  // Otherwise |user_id| is ignored.  For normal GAIA users the |user_id| is an
  // AccountId-derived string (see AccountId::GetAccountIdKey).
  virtual void TpmAttestationSignSimpleChallenge(
      chromeos::attestation::AttestationKeyType key_type,
      const Identification& user_id,
      const std::string& key_name,
      const std::string& challenge,
      const DataCallback& callback) = 0;

  // Asks cryptohome to asynchronously retrieve a string associated with given
  // |user_id| that would be used in mount path instead of |user_id|.
  // On success the data is sent to |callback|.
  virtual void AsyncGetSanitizedUsername(const Identification& user_id,
                                         const DataCallback& callback) = 0;

  // Creates the global AsyncMethodCaller instance.
  static void Initialize();

  // Similar to Initialize(), but can inject an alternative
  // AsyncMethodCaller such as MockAsyncMethodCaller for testing.
  // The injected object will be owned by the internal pointer and deleted
  // by Shutdown().
  static void InitializeForTesting(AsyncMethodCaller* async_method_caller);

  // Destroys the global AsyncMethodCaller instance if it exists.
  static void Shutdown();

  // Returns a pointer to the global AsyncMethodCaller instance.
  // Initialize() should already have been called.
  static AsyncMethodCaller* GetInstance();
};

}  // namespace cryptohome

#endif  // CHROMEOS_CRYPTOHOME_ASYNC_METHOD_CALLER_H_
