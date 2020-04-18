// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef DEVICE_FIDO_FIDO_DEVICE_AUTHENTICATOR_H_
#define DEVICE_FIDO_FIDO_DEVICE_AUTHENTICATOR_H_

#include <string>
#include <vector>

#include "base/component_export.h"
#include "base/macros.h"
#include "base/optional.h"
#include "device/fido/fido_authenticator.h"

namespace device {

class AuthenticatorSelectionCriteria;
class CtapGetAssertionRequest;
class CtapMakeCredentialRequest;
class FidoDevice;
class FidoTask;

// Adaptor class from a |FidoDevice| to the |FidoAuthenticator| interface.
// Responsible for translating WebAuthn-level requests into serializations that
// can be passed to the device for transport.
class COMPONENT_EXPORT(DEVICE_FIDO) FidoDeviceAuthenticator
    : public FidoAuthenticator {
 public:
  FidoDeviceAuthenticator(FidoDevice* device);
  ~FidoDeviceAuthenticator() override;

  // FidoAuthenticator:
  void MakeCredential(
      AuthenticatorSelectionCriteria authenticator_selection_criteria,
      CtapMakeCredentialRequest request,
      MakeCredentialCallback callback) override;
  void GetAssertion(CtapGetAssertionRequest request,
                    GetAssertionCallback callback) override;
  void Cancel() override;
  std::string GetId() const override;

 protected:
  void OnCtapMakeCredentialResponseReceived(
      MakeCredentialCallback callback,
      base::Optional<std::vector<uint8_t>> response_data);
  void OnCtapGetAssertionResponseReceived(
      GetAssertionCallback callback,
      base::Optional<std::vector<uint8_t>> response_data);

  FidoDevice* device() { return device_; }
  void SetTaskForTesting(std::unique_ptr<FidoTask> task);

 private:
  FidoDevice* const device_;
  std::unique_ptr<FidoTask> task_;

  DISALLOW_COPY_AND_ASSIGN(FidoDeviceAuthenticator);
};

}  // namespace device

#endif  // DEVICE_FIDO_FIDO_DEVICE_AUTHENTICATOR_H_
