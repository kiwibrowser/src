// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef DEVICE_FIDO_VIRTUAL_FIDO_DEVICE_H_
#define DEVICE_FIDO_VIRTUAL_FIDO_DEVICE_H_

#include <stdint.h>

#include <map>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "base/component_export.h"
#include "base/containers/span.h"
#include "base/macros.h"
#include "base/memory/scoped_refptr.h"
#include "base/optional.h"
#include "device/fido/fido_device.h"
#include "device/fido/fido_parsing_utils.h"
#include "net/cert/x509_util.h"

namespace crypto {
class ECPrivateKey;
}  // namespace crypto

namespace device {

class COMPONENT_EXPORT(DEVICE_FIDO) VirtualFidoDevice : public FidoDevice {
 public:
  // Encapsulates information corresponding to one registered key on the virtual
  // authenticator device.
  struct COMPONENT_EXPORT(DEVICE_FIDO) RegistrationData {
    RegistrationData();
    RegistrationData(std::unique_ptr<crypto::ECPrivateKey> private_key,
                     std::vector<uint8_t> application_parameter,
                     uint32_t counter);

    RegistrationData(RegistrationData&& data);
    RegistrationData& operator=(RegistrationData&& other);

    ~RegistrationData();

    std::unique_ptr<crypto::ECPrivateKey> private_key;
    std::vector<uint8_t> application_parameter;
    uint32_t counter = 0;

    DISALLOW_COPY_AND_ASSIGN(RegistrationData);
  };

  // Stores the state of the device. Since |U2fDevice| objects only persist for
  // the lifetime of a single request, keeping state in an external object is
  // necessary in order to provide continuity between requests.
  class COMPONENT_EXPORT(DEVICE_FIDO) State : public base::RefCounted<State> {
   public:
    State();

    // The common name in the attestation certificate.
    std::string attestation_cert_common_name;

    // The common name in the attestation certificate if individual attestation
    // is requested.
    std::string individual_attestation_cert_common_name;

    // Registered keys. Keyed on key handle (a.k.a. "credential ID").
    std::map<std::vector<uint8_t>,
             RegistrationData,
             fido_parsing_utils::SpanLess>
        registrations;

    // If set, this callback is called whenever a "press" is required. It allows
    // tests to change the state of the world during processing.
    base::RepeatingCallback<void(void)> simulate_press_callback;

    // If true, causes the response from the device to be invalid.
    bool simulate_invalid_response = false;

    // Adds a registration for the specified credential ID with the application
    // parameter set to be valid for the given relying party ID (which would
    // typically be a domain, e.g. "example.com").
    //
    // Returns true on success. Will fail if there already exists a credential
    // with the given ID or if private-key generation fails.
    bool InjectRegistration(const std::vector<uint8_t>& credential_id,
                            const std::string& relying_party_id);

   private:
    friend class base::RefCounted<State>;
    ~State();

    DISALLOW_COPY_AND_ASSIGN(State);
  };

  // Constructs an object with ephemeral state. In order to have the state of
  // the device persist between operations, use the constructor that takes a
  // scoped_refptr<State>.
  VirtualFidoDevice();

  // Constructs an object that will read from, and write to, |state|.
  explicit VirtualFidoDevice(scoped_refptr<State> state);

  ~VirtualFidoDevice() override;

  State* mutable_state() { return state_.get(); }

 protected:
  static std::vector<uint8_t> GetAttestationKey();

  static bool Sign(crypto::ECPrivateKey* private_key,
                   base::span<const uint8_t> sign_buffer,
                   std::vector<uint8_t>* signature);

  // Constructs certificate encoded in X.509 format to be used for packed
  // attestation statement and FIDO-U2F attestation statement.
  // https://w3c.github.io/webauthn/#defined-attestation-formats
  base::Optional<std::vector<uint8_t>> GenerateAttestationCertificate(
      bool individual_attestation_requested) const;

  // FidoDevice:
  void TryWink(WinkCallback cb) override;
  std::string GetId() const override;

 private:
  scoped_refptr<State> state_ = base::MakeRefCounted<State>();

  DISALLOW_COPY_AND_ASSIGN(VirtualFidoDevice);
};

}  // namespace device

#endif  // DEVICE_FIDO_VIRTUAL_FIDO_DEVICE_H_
