// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef DEVICE_FIDO_U2F_REGISTER_H_
#define DEVICE_FIDO_U2F_REGISTER_H_

#include <memory>
#include <set>
#include <string>
#include <vector>

#include "base/component_export.h"
#include "base/containers/flat_set.h"
#include "base/macros.h"
#include "base/optional.h"
#include "device/fido/authenticator_make_credential_response.h"
#include "device/fido/fido_constants.h"
#include "device/fido/fido_transport_protocol.h"
#include "device/fido/u2f_request.h"

namespace service_manager {
class Connector;
};

namespace device {

class COMPONENT_EXPORT(DEVICE_FIDO) U2fRegister : public U2fRequest {
 public:
  using RegisterResponseCallback = base::OnceCallback<void(
      FidoReturnCode status_code,
      base::Optional<AuthenticatorMakeCredentialResponse> response_data)>;

  static std::unique_ptr<U2fRequest> TryRegistration(
      service_manager::Connector* connector,
      const base::flat_set<FidoTransportProtocol>& transports,
      std::vector<std::vector<uint8_t>> registered_keys,
      std::vector<uint8_t> challenge_digest,
      std::vector<uint8_t> application_parameter,
      bool individual_attestation_ok,
      RegisterResponseCallback completion_callback);

  U2fRegister(service_manager::Connector* connector,
              const base::flat_set<FidoTransportProtocol>& transports,
              std::vector<std::vector<uint8_t>> registered_keys,
              std::vector<uint8_t> challenge_digest,
              std::vector<uint8_t> application_parameter,
              bool individual_attestation_ok,
              RegisterResponseCallback completion_callback);
  ~U2fRegister() override;

 private:
  void TryDevice() override;
  void OnTryDevice(bool is_duplicate_registration,
                   base::Optional<std::vector<uint8_t>> response);

  // Callback function called when non-empty exclude list was provided. This
  // function iterates through all key handles in |registered_keys_| for all
  // devices and checks for already registered keys.
  void OnTryCheckRegistration(
      std::vector<std::vector<uint8_t>>::const_iterator it,
      base::Optional<std::vector<uint8_t>> response);
  // Function handling registration flow after all devices were checked for
  // already registered keys.
  void CompleteNewDeviceRegistration();
  // Returns whether |current_device_| has been checked for duplicate
  // registration for all key handles provided in |registered_keys_|.
  bool CheckedForDuplicateRegistration();

  // Indicates whether the token should be signaled that using an individual
  // attestation certificate is acceptable.
  const bool individual_attestation_ok_;
  RegisterResponseCallback completion_callback_;

  // List of authenticators that did not create any of the key handles in the
  // exclude list.
  std::set<std::string> checked_device_id_list_;
  base::WeakPtrFactory<U2fRegister> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(U2fRegister);
};

}  // namespace device

#endif  // DEVICE_FIDO_U2F_REGISTER_H_
