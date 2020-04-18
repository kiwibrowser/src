// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "device/fido/u2f_command_constructor.h"

#include <string>
#include <utility>

#include "components/apdu/apdu_command.h"
#include "device/fido/fido_parsing_utils.h"

namespace device {

bool IsConvertibleToU2fRegisterCommand(
    const CtapMakeCredentialRequest& request) {
  if (request.user_verification_required() || request.resident_key_supported())
    return false;

  const auto& public_key_credential_info =
      request.public_key_credential_params().public_key_credential_params();
  return std::any_of(
      public_key_credential_info.begin(), public_key_credential_info.end(),
      [](const auto& credential_info) {
        return credential_info.algorithm ==
               base::strict_cast<int>(CoseAlgorithmIdentifier::kCoseEs256);
      });
}

bool IsConvertibleToU2fSignCommand(const CtapGetAssertionRequest& request) {
  const auto& allow_list = request.allow_list();
  return request.user_verification() !=
             UserVerificationRequirement::kRequired &&
         allow_list && !allow_list->empty();
}

base::Optional<std::vector<uint8_t>> ConvertToU2fRegisterCommand(
    const CtapMakeCredentialRequest& request) {
  if (!IsConvertibleToU2fRegisterCommand(request))
    return base::nullopt;

  return ConstructU2fRegisterCommand(
      fido_parsing_utils::CreateSHA256Hash(request.rp().rp_id()),
      request.client_data_hash());
}

base::Optional<std::vector<uint8_t>> ConvertToU2fCheckOnlySignCommand(
    const CtapMakeCredentialRequest& request,
    const PublicKeyCredentialDescriptor& key_handle) {
  if (key_handle.credential_type() != CredentialType::kPublicKey)
    return base::nullopt;

  return ConstructU2fSignCommand(
      fido_parsing_utils::CreateSHA256Hash(request.rp().rp_id()),
      request.client_data_hash(), key_handle.id(), true /* check_only */);
}

base::Optional<std::vector<uint8_t>> ConvertToU2fSignCommand(
    const CtapGetAssertionRequest& request,
    ApplicationParameterType application_parameter_type,
    base::span<const uint8_t> key_handle,
    bool check_only) {
  if (!IsConvertibleToU2fSignCommand(request))
    return base::nullopt;

  auto application_parameter =
      application_parameter_type == ApplicationParameterType::kPrimary
          ? fido_parsing_utils::CreateSHA256Hash(request.rp_id())
          : std::vector<uint8_t>();

  return ConstructU2fSignCommand(std::move(application_parameter),
                                 request.client_data_hash(), key_handle,
                                 check_only);
}

base::Optional<std::vector<uint8_t>> ConstructU2fRegisterCommand(
    base::span<const uint8_t> application_parameter,
    base::span<const uint8_t> challenge_parameter,
    bool is_individual_attestation) {
  if (application_parameter.size() != kU2fParameterLength ||
      challenge_parameter.size() != kU2fParameterLength) {
    return base::nullopt;
  }

  std::vector<uint8_t> data;
  data.reserve(challenge_parameter.size() + application_parameter.size());
  fido_parsing_utils::Append(&data, challenge_parameter);
  fido_parsing_utils::Append(&data, application_parameter);

  apdu::ApduCommand command;
  command.set_ins(base::strict_cast<uint8_t>(U2fApduInstruction::kRegister));
  command.set_p1(kP1TupRequiredConsumed |
                 (is_individual_attestation ? kP1IndividualAttestation : 0));
  command.set_data(std::move(data));
  command.set_response_length(apdu::ApduCommand::kApduMaxResponseLength);
  return command.GetEncodedCommand();
}

base::Optional<std::vector<uint8_t>> ConstructU2fSignCommand(
    base::span<const uint8_t> application_parameter,
    base::span<const uint8_t> challenge_parameter,
    base::span<const uint8_t> key_handle,
    bool check_only) {
  if (application_parameter.size() != kU2fParameterLength ||
      challenge_parameter.size() != kU2fParameterLength ||
      key_handle.size() > kMaxKeyHandleLength) {
    return base::nullopt;
  }

  std::vector<uint8_t> data;
  data.reserve(challenge_parameter.size() + application_parameter.size() + 1 +
               key_handle.size());
  fido_parsing_utils::Append(&data, challenge_parameter);
  fido_parsing_utils::Append(&data, application_parameter);
  data.push_back(static_cast<uint8_t>(key_handle.size()));
  fido_parsing_utils::Append(&data, key_handle);

  apdu::ApduCommand command;
  command.set_ins(base::strict_cast<uint8_t>(U2fApduInstruction::kSign));
  command.set_p1(check_only ? kP1CheckOnly : kP1TupRequiredConsumed);
  command.set_data(std::move(data));
  command.set_response_length(apdu::ApduCommand::kApduMaxResponseLength);
  return command.GetEncodedCommand();
}

base::Optional<std::vector<uint8_t>> ConstructBogusU2fRegistrationCommand() {
  return ConstructU2fRegisterCommand(kBogusAppParam, kBogusChallenge);
}

}  // namespace device
