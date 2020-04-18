// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "device/fido/u2f_sign.h"

#include <utility>

#include "components/apdu/apdu_command.h"
#include "components/apdu/apdu_response.h"
#include "device/fido/u2f_command_constructor.h"
#include "services/service_manager/public/cpp/connector.h"

namespace device {

// static
std::unique_ptr<U2fRequest> U2fSign::TrySign(
    service_manager::Connector* connector,
    const base::flat_set<FidoTransportProtocol>& transports,
    std::vector<std::vector<uint8_t>> registered_keys,
    std::vector<uint8_t> challenge_digest,
    std::vector<uint8_t> application_parameter,
    base::Optional<std::vector<uint8_t>> alt_application_parameter,
    SignResponseCallback completion_callback) {
  std::unique_ptr<U2fRequest> request = std::make_unique<U2fSign>(
      connector, transports, registered_keys, challenge_digest,
      application_parameter, std::move(alt_application_parameter),
      std::move(completion_callback));
  request->Start();

  return request;
}

U2fSign::U2fSign(service_manager::Connector* connector,
                 const base::flat_set<FidoTransportProtocol>& transports,
                 std::vector<std::vector<uint8_t>> registered_keys,
                 std::vector<uint8_t> challenge_digest,
                 std::vector<uint8_t> application_parameter,
                 base::Optional<std::vector<uint8_t>> alt_application_parameter,
                 SignResponseCallback completion_callback)
    : U2fRequest(connector,
                 transports,
                 std::move(application_parameter),
                 std::move(challenge_digest),
                 std::move(registered_keys)),
      alt_application_parameter_(std::move(alt_application_parameter)),
      completion_callback_(std::move(completion_callback)),
      weak_factory_(this) {
  // U2F devices require at least one key handle.
  // TODO(crbug.com/831712): When CTAP2 authenticators are supported, this check
  // should be enforced by handlers in fido/device on a per-device basis.
  CHECK(!registered_keys_.empty());
}

U2fSign::~U2fSign() = default;

void U2fSign::TryDevice() {
  DCHECK(current_device_);

  // Try signing current device with the first registered key.
  auto it = registered_keys_.cbegin();
  InitiateDeviceTransaction(
      GetU2fSignApduCommand(application_parameter_, *it),
      base::BindOnce(&U2fSign::OnTryDevice, weak_factory_.GetWeakPtr(), it,
                     ApplicationParameterType::kPrimary));
}

void U2fSign::OnTryDevice(std::vector<std::vector<uint8_t>>::const_iterator it,
                          ApplicationParameterType application_parameter_type,
                          base::Optional<std::vector<uint8_t>> response) {
  const auto apdu_response =
      response ? apdu::ApduResponse::CreateFromMessage(std::move(*response))
               : base::nullopt;
  auto return_code = apdu_response ? apdu_response->status()
                                   : apdu::ApduResponse::Status::SW_WRONG_DATA;
  auto response_data = return_code == apdu::ApduResponse::Status::SW_WRONG_DATA
                           ? std::vector<uint8_t>()
                           : apdu_response->data();

  switch (return_code) {
    case apdu::ApduResponse::Status::SW_NO_ERROR: {
      state_ = State::COMPLETE;
      if (it == registered_keys_.cend()) {
        // This was a response to a fake enrollment. Return an empty key handle.
        std::move(completion_callback_)
            .Run(FidoReturnCode::kUserConsentButCredentialNotRecognized,
                 base::nullopt);
      } else {
        const std::vector<uint8_t>* const application_parameter_used =
            application_parameter_type == ApplicationParameterType::kPrimary
                ? &application_parameter_
                : &alt_application_parameter_.value();
        auto sign_response =
            AuthenticatorGetAssertionResponse::CreateFromU2fSignResponse(
                *application_parameter_used, std::move(response_data), *it);
        if (!sign_response) {
          std::move(completion_callback_)
              .Run(FidoReturnCode::kAuthenticatorResponseInvalid,
                   base::nullopt);
        } else {
          std::move(completion_callback_)
              .Run(FidoReturnCode::kSuccess, std::move(sign_response));
        }
      }
      break;
    }
    case apdu::ApduResponse::Status::SW_CONDITIONS_NOT_SATISFIED: {
      // Key handle is accepted by this device, but waiting on user touch. Move
      // on and try this device again later.
      state_ = State::IDLE;
      Transition();
      break;
    }
    case apdu::ApduResponse::Status::SW_WRONG_DATA:
    case apdu::ApduResponse::Status::SW_WRONG_LENGTH: {
      if (application_parameter_type == ApplicationParameterType::kPrimary &&
          alt_application_parameter_ && it != registered_keys_.cend()) {
        // |application_parameter_| failed, but there is also
        // |alt_application_parameter_| to try.
        InitiateDeviceTransaction(
            GetU2fSignApduCommand(*alt_application_parameter_, *it),
            base::Bind(&U2fSign::OnTryDevice, weak_factory_.GetWeakPtr(), it,
                       ApplicationParameterType::kAlternative));
      } else if (it == registered_keys_.cend()) {
        // The fake enrollment errored out. Move on to the next device.
        AbandonCurrentDeviceAndTransition();
      } else if (++it != registered_keys_.end()) {
        // Key is not for this device. Try signing with the next key.
        InitiateDeviceTransaction(
            GetU2fSignApduCommand(application_parameter_, *it),
            base::BindOnce(&U2fSign::OnTryDevice, weak_factory_.GetWeakPtr(),
                           it, ApplicationParameterType::kPrimary));
      } else {
        // No provided key was accepted by this device. Send registration
        // (Fake enroll) request to device.
        // We do this to prevent user confusion. Otherwise, if the device
        // doesn't blink, the user might think it's broken rather than that
        // it's not registered. Once the user consents to use the device,
        // the relying party can inform them that it hasn't been registered.
        InitiateDeviceTransaction(
            ConstructBogusU2fRegistrationCommand(),
            base::BindOnce(&U2fSign::OnTryDevice, weak_factory_.GetWeakPtr(),
                           registered_keys_.cend(),
                           ApplicationParameterType::kPrimary));
      }
      break;
    }
    default:
      // Some sort of failure occured. Abandon this device and move on.
      AbandonCurrentDeviceAndTransition();
      break;
  }
}

}  // namespace device
