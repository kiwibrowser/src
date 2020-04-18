// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "device/fido/u2f_register.h"

#include <utility>

#include "base/stl_util.h"
#include "components/apdu/apdu_command.h"
#include "components/apdu/apdu_response.h"
#include "device/fido/authenticator_make_credential_response.h"
#include "device/fido/u2f_command_constructor.h"
#include "services/service_manager/public/cpp/connector.h"

namespace device {

// static
std::unique_ptr<U2fRequest> U2fRegister::TryRegistration(
    service_manager::Connector* connector,
    const base::flat_set<FidoTransportProtocol>& transports,
    std::vector<std::vector<uint8_t>> registered_keys,
    std::vector<uint8_t> challenge_digest,
    std::vector<uint8_t> application_parameter,
    bool individual_attestation_ok,
    RegisterResponseCallback completion_callback) {
  std::unique_ptr<U2fRequest> request = std::make_unique<U2fRegister>(
      connector, transports, std::move(registered_keys),
      std::move(challenge_digest), std::move(application_parameter),
      individual_attestation_ok, std::move(completion_callback));
  request->Start();
  return request;
}

U2fRegister::U2fRegister(
    service_manager::Connector* connector,
    const base::flat_set<FidoTransportProtocol>& transports,
    std::vector<std::vector<uint8_t>> registered_keys,
    std::vector<uint8_t> challenge_digest,
    std::vector<uint8_t> application_parameter,
    bool individual_attestation_ok,
    RegisterResponseCallback completion_callback)
    : U2fRequest(connector,
                 transports,
                 std::move(application_parameter),
                 std::move(challenge_digest),
                 std::move(registered_keys)),
      individual_attestation_ok_(individual_attestation_ok),
      completion_callback_(std::move(completion_callback)),
      weak_factory_(this) {}

U2fRegister::~U2fRegister() = default;

void U2fRegister::TryDevice() {
  DCHECK(current_device_);
  if (!registered_keys_.empty() && !CheckedForDuplicateRegistration()) {
    auto it = registered_keys_.cbegin();
    InitiateDeviceTransaction(
        GetU2fSignApduCommand(application_parameter_, *it,
                              true /* check_only */),
        base::BindOnce(&U2fRegister::OnTryCheckRegistration,
                       weak_factory_.GetWeakPtr(), it));
  } else {
    InitiateDeviceTransaction(
        GetU2fRegisterApduCommand(individual_attestation_ok_),
        base::BindOnce(&U2fRegister::OnTryDevice, weak_factory_.GetWeakPtr(),
                       false /* is_duplicate_registration */));
  }
}

void U2fRegister::OnTryCheckRegistration(
    std::vector<std::vector<uint8_t>>::const_iterator it,
    base::Optional<std::vector<uint8_t>> response) {
  const auto apdu_response =
      response ? apdu::ApduResponse::CreateFromMessage(std::move(*response))
               : base::nullopt;
  auto return_code = apdu_response ? apdu_response->status()
                                   : apdu::ApduResponse::Status::SW_WRONG_DATA;

  switch (return_code) {
    case apdu::ApduResponse::Status::SW_NO_ERROR:
    case apdu::ApduResponse::Status::SW_CONDITIONS_NOT_SATISFIED: {
      // Duplicate registration found. Call bogus registration to check for
      // user presence (touch) and terminate the registration process.
      InitiateDeviceTransaction(
          ConstructBogusU2fRegistrationCommand(),
          base::BindOnce(&U2fRegister::OnTryDevice, weak_factory_.GetWeakPtr(),
                         true /* is_duplicate_registration */));
      break;
    }

    case apdu::ApduResponse::Status::SW_WRONG_DATA:
      // Continue to iterate through the provided key handles in the exclude
      // list and check for already registered keys.
      if (++it != registered_keys_.end()) {
        InitiateDeviceTransaction(
            GetU2fSignApduCommand(application_parameter_, *it,
                                  true /* check_only */),
            base::BindOnce(&U2fRegister::OnTryCheckRegistration,
                           weak_factory_.GetWeakPtr(), it));
      } else {
        checked_device_id_list_.insert(current_device_->GetId());
        if (devices_.empty()) {
          // When all devices have been checked, proceed to registration.
          CompleteNewDeviceRegistration();
        } else {
          state_ = State::IDLE;
          Transition();
        }
      }
      break;
    default:
      // Some sort of failure occurred. Abandon this device and move on.
      AbandonCurrentDeviceAndTransition();
      break;
  }
}

void U2fRegister::CompleteNewDeviceRegistration() {
  if (current_device_)
    attempted_devices_.push_back(std::move(current_device_));

  devices_.splice(devices_.end(), std::move(attempted_devices_));
  state_ = State::IDLE;
  Transition();
  return;
}

bool U2fRegister::CheckedForDuplicateRegistration() {
  return base::ContainsKey(checked_device_id_list_, current_device_->GetId());
}

void U2fRegister::OnTryDevice(bool is_duplicate_registration,
                              base::Optional<std::vector<uint8_t>> response) {
  const auto apdu_response =
      response ? apdu::ApduResponse::CreateFromMessage(std::move(*response))
               : base::nullopt;
  auto return_code = apdu_response ? apdu_response->status()
                                   : apdu::ApduResponse::Status::SW_WRONG_DATA;
  switch (return_code) {
    case apdu::ApduResponse::Status::SW_NO_ERROR: {
      state_ = State::COMPLETE;
      if (is_duplicate_registration) {
        std::move(completion_callback_)
            .Run(FidoReturnCode::kUserConsentButCredentialExcluded,
                 base::nullopt);
        break;
      }
      auto response =
          AuthenticatorMakeCredentialResponse::CreateFromU2fRegisterResponse(
              application_parameter_, apdu_response->data());
      if (!response) {
        // The response data was corrupted / didn't parse properly.
        std::move(completion_callback_)
            .Run(FidoReturnCode::kAuthenticatorResponseInvalid, base::nullopt);
        break;
      }
      std::move(completion_callback_)
          .Run(FidoReturnCode::kSuccess, std::move(response));
      break;
    }
    case apdu::ApduResponse::Status::SW_CONDITIONS_NOT_SATISFIED:
      // Waiting for user touch, move on and try this device later.
      state_ = State::IDLE;
      Transition();
      break;
    default:
      // An error has occurred, quit trying this device.
      AbandonCurrentDeviceAndTransition();
      break;
  }
}

}  // namespace device
