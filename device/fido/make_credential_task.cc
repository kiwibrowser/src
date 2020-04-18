// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "device/fido/make_credential_task.h"

#include <utility>

#include "base/bind.h"
#include "device/fido/ctap_empty_authenticator_request.h"
#include "device/fido/ctap_register_operation.h"
#include "device/fido/device_response_converter.h"

namespace device {

MakeCredentialTask::MakeCredentialTask(
    FidoDevice* device,
    CtapMakeCredentialRequest request_parameter,
    AuthenticatorSelectionCriteria authenticator_selection_criteria,
    MakeCredentialTaskCallback callback)
    : FidoTask(device),
      request_parameter_(std::move(request_parameter)),
      authenticator_selection_criteria_(
          std::move(authenticator_selection_criteria)),
      callback_(std::move(callback)),
      weak_factory_(this) {}

MakeCredentialTask::~MakeCredentialTask() = default;

void MakeCredentialTask::StartTask() {
  GetAuthenticatorInfo(base::BindOnce(&MakeCredentialTask::MakeCredential,
                                      weak_factory_.GetWeakPtr()),
                       base::BindOnce(&MakeCredentialTask::U2fRegister,
                                      weak_factory_.GetWeakPtr()));
}

void MakeCredentialTask::MakeCredential() {
  if (!CheckIfAuthenticatorSelectionCriteriaAreSatisfied()) {
    std::move(callback_).Run(CtapDeviceResponseCode::kCtap2ErrOther,
                             base::nullopt);
    return;
  }

  register_operation_ = std::make_unique<CtapRegisterOperation>(
      device(), &request_parameter_,
      base::BindOnce(&MakeCredentialTask::OnCtapMakeCredentialResponseReceived,
                     weak_factory_.GetWeakPtr()));
  register_operation_->Start();
}

void MakeCredentialTask::U2fRegister() {
  // TODO(hongjunchoi): Implement U2F register request logic to support
  // interoperability with U2F protocol. Currently all U2F devices are not
  // supported and request to U2F devices will be silently dropped.
  // See: https://crbug.com/798573
  std::move(callback_).Run(CtapDeviceResponseCode::kCtap2ErrOther,
                           base::nullopt);
}

void MakeCredentialTask::OnCtapMakeCredentialResponseReceived(
    CtapDeviceResponseCode return_code,
    base::Optional<AuthenticatorMakeCredentialResponse> response_data) {
  if (return_code != CtapDeviceResponseCode::kSuccess) {
    std::move(callback_).Run(return_code, base::nullopt);
    return;
  }

  if (!response_data ||
      !response_data->CheckRpIdHash(request_parameter_.rp().rp_id())) {
    std::move(callback_).Run(CtapDeviceResponseCode::kCtap2ErrOther,
                             base::nullopt);
    return;
  }

  std::move(callback_).Run(return_code, std::move(response_data));
}

bool MakeCredentialTask::CheckIfAuthenticatorSelectionCriteriaAreSatisfied() {
  using AuthenticatorAttachment =
      AuthenticatorSelectionCriteria::AuthenticatorAttachment;
  using UvAvailability =
      AuthenticatorSupportedOptions::UserVerificationAvailability;

  // U2F authenticators are non-platform devices that do not support resident
  // key or user verification.
  const auto& device_info = device()->device_info();
  if (!device_info) {
    return !authenticator_selection_criteria_.require_resident_key() &&
           authenticator_selection_criteria_.user_verification_requirement() !=
               UserVerificationRequirement::kRequired &&
           authenticator_selection_criteria_.authenticator_attachement() !=
               AuthenticatorAttachment::kPlatform;
  }

  const auto& options = device_info->options();
  if ((authenticator_selection_criteria_.authenticator_attachement() ==
           AuthenticatorAttachment::kPlatform &&
       !options.is_platform_device()) ||
      (authenticator_selection_criteria_.authenticator_attachement() ==
           AuthenticatorAttachment::kCrossPlatform &&
       options.is_platform_device())) {
    return false;
  }

  if (authenticator_selection_criteria_.require_resident_key() &&
      !options.supports_resident_key()) {
    return false;
  }

  const auto user_verification_requirement =
      authenticator_selection_criteria_.user_verification_requirement();
  if (user_verification_requirement == UserVerificationRequirement::kRequired) {
    request_parameter_.SetUserVerificationRequired(true);
  }

  return user_verification_requirement !=
             UserVerificationRequirement::kRequired ||
         options.user_verification_availability() ==
             UvAvailability::kSupportedAndConfigured;
}

}  // namespace device
