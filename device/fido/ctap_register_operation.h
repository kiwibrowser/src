// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef DEVICE_FIDO_CTAP_REGISTER_OPERATION_H_
#define DEVICE_FIDO_CTAP_REGISTER_OPERATION_H_

#include <stdint.h>

#include <memory>
#include <vector>

#include "base/callback.h"
#include "base/component_export.h"
#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "base/optional.h"
#include "device/fido/device_operation.h"
#include "device/fido/fido_constants.h"

namespace device {

class FidoDevice;
class CtapMakeCredentialRequest;
class AuthenticatorMakeCredentialResponse;

// Represents per device registration logic for CTAP device.
// CtapRegisterOperation is owned by MakeCredentialTask, and the lifetime of
// CtapRegisterOperation does not exceed that of MakeCredentialTask. As so,
// |request_| member variable is dependency injected from MakeCredentialTask.
class COMPONENT_EXPORT(DEVICE_FIDO) CtapRegisterOperation
    : public DeviceOperation<CtapMakeCredentialRequest,
                             AuthenticatorMakeCredentialResponse> {
 public:
  CtapRegisterOperation(FidoDevice* device,
                        const CtapMakeCredentialRequest* request,
                        DeviceResponseCallback callback);

  ~CtapRegisterOperation() override;

  // DeviceOperation:
  void Start() override;

 private:
  void OnResponseReceived(base::Optional<std::vector<uint8_t>> device_response);

  const CtapMakeCredentialRequest* const request_;

  base::WeakPtrFactory<CtapRegisterOperation> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(CtapRegisterOperation);
};

}  // namespace device

#endif  // DEVICE_FIDO_CTAP_REGISTER_OPERATION_H_
