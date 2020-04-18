// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef DEVICE_FIDO_U2F_SIGN_H_
#define DEVICE_FIDO_U2F_SIGN_H_

#include <memory>
#include <string>
#include <vector>

#include "base/component_export.h"
#include "base/containers/flat_set.h"
#include "base/macros.h"
#include "base/optional.h"
#include "device/fido/authenticator_get_assertion_response.h"
#include "device/fido/fido_constants.h"
#include "device/fido/fido_transport_protocol.h"
#include "device/fido/u2f_request.h"

namespace service_manager {
class Connector;
}

namespace device {

class COMPONENT_EXPORT(DEVICE_FIDO) U2fSign : public U2fRequest {
 public:
  using SignResponseCallback = base::OnceCallback<void(
      FidoReturnCode status_code,
      base::Optional<AuthenticatorGetAssertionResponse> response_data)>;

  static std::unique_ptr<U2fRequest> TrySign(
      service_manager::Connector* connector,
      const base::flat_set<FidoTransportProtocol>& transports,
      std::vector<std::vector<uint8_t>> registered_keys,
      std::vector<uint8_t> challenge_digest,
      std::vector<uint8_t> application_parameter,
      base::Optional<std::vector<uint8_t>> alt_application_parameter,
      SignResponseCallback completion_callback);

  U2fSign(service_manager::Connector* connector,
          const base::flat_set<FidoTransportProtocol>& transports,
          std::vector<std::vector<uint8_t>> registered_keys,
          std::vector<uint8_t> challenge_digest,
          std::vector<uint8_t> application_parameter,
          base::Optional<std::vector<uint8_t>> alt_application_parameter,
          SignResponseCallback completion_callback);
  ~U2fSign() override;

 private:
  void TryDevice() override;
  void OnTryDevice(std::vector<std::vector<uint8_t>>::const_iterator it,
                   ApplicationParameterType application_parameter_type,
                   base::Optional<std::vector<uint8_t>> response);

  base::Optional<std::vector<uint8_t>> alt_application_parameter_;
  SignResponseCallback completion_callback_;

  base::WeakPtrFactory<U2fSign> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(U2fSign);
};

}  // namespace device

#endif  // DEVICE_FIDO_U2F_SIGN_H_
