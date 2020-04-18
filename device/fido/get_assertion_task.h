// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef DEVICE_FIDO_GET_ASSERTION_TASK_H_
#define DEVICE_FIDO_GET_ASSERTION_TASK_H_

#include <stdint.h>

#include <vector>

#include "base/callback.h"
#include "base/component_export.h"
#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "base/optional.h"
#include "device/fido/ctap_get_assertion_request.h"
#include "device/fido/fido_constants.h"
#include "device/fido/fido_task.h"

namespace device {

class AuthenticatorGetAssertionResponse;

// Represents per device sign operation on CTAP1/CTAP2 devices.
// https://fidoalliance.org/specs/fido-v2.0-rd-20161004/fido-client-to-authenticator-protocol-v2.0-rd-20161004.html#authenticatorgetassertion
class COMPONENT_EXPORT(DEVICE_FIDO) GetAssertionTask : public FidoTask {
 public:
  using GetAssertionTaskCallback = base::OnceCallback<void(
      CtapDeviceResponseCode,
      base::Optional<AuthenticatorGetAssertionResponse>)>;

  GetAssertionTask(FidoDevice* device,
                   CtapGetAssertionRequest request,
                   GetAssertionTaskCallback callback);
  ~GetAssertionTask() override;

 private:
  // FidoTask:
  void StartTask() override;

  void GetAssertion();
  void U2fSign();

  // PublicKeyUserEntity field in GetAssertion response is optional with the
  // following constraints:
  // - If assertion has been made without user verification, user identifiable
  //   information must not be included.
  // - For resident key credentials, user id of the user entity is mandatory.
  // - When multiple accounts exist for specified RP ID, user entity is
  //   mandatory.
  // TODO(hongjunchoi) : Add link to section of the CTAP spec once it is
  // published.
  bool CheckRequirementsOnReturnedUserEntities(
      const AuthenticatorGetAssertionResponse& response);

  // Checks whether credential ID returned from the authenticator was included
  // in the allowed list for authenticators. If the device has resident key
  // support, returned credential ID may be resident credential. Thus, returned
  // credential ID need not be in allowed list.
  // TODO(hongjunchoi) : Add link to section of the CTAP spec once it is
  // published.
  bool CheckRequirementsOnReturnedCredentialId(
      const AuthenticatorGetAssertionResponse& response);

  // Checks UserVerificationRequirement enum passed from the relying party
  // is compatible with the authenticator using the following logic:
  //  - If UserVerificationRequirement is set to kRequired, user verification
  //    option parameter should be set to true.
  //  - If UserVerificationRequirement is set to kPreferred, user verification
  //    option is set to true only if the authenticator supports UV.
  //  - If UserVerificationRequirement is set to kDiscouraged, user verification
  //    is set to false.
  // https://w3c.github.io/webauthn/#enumdef-userverificationrequirement
  bool CheckUserVerificationCompatible();

  void OnCtapGetAssertionResponseReceived(
      base::Optional<std::vector<uint8_t>> device_response);

  CtapGetAssertionRequest request_;
  GetAssertionTaskCallback callback_;
  base::WeakPtrFactory<GetAssertionTask> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(GetAssertionTask);
};

}  // namespace device

#endif  // DEVICE_FIDO_GET_ASSERTION_TASK_H_
