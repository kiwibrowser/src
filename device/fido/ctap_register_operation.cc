// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "device/fido/ctap_register_operation.h"

#include <utility>

#include "base/bind.h"
#include "device/fido/authenticator_make_credential_response.h"
#include "device/fido/ctap_make_credential_request.h"
#include "device/fido/device_response_converter.h"
#include "device/fido/fido_device.h"

namespace device {

CtapRegisterOperation::CtapRegisterOperation(
    FidoDevice* device,
    const CtapMakeCredentialRequest* request,
    DeviceResponseCallback callback)
    : DeviceOperation(device, std::move(callback)),
      request_(request),
      weak_factory_(this) {}

CtapRegisterOperation::~CtapRegisterOperation() = default;

void CtapRegisterOperation::Start() {
  device_->DeviceTransact(
      request_->EncodeAsCBOR(),
      base::BindOnce(&CtapRegisterOperation::OnResponseReceived,
                     weak_factory_.GetWeakPtr()));
}

void CtapRegisterOperation::OnResponseReceived(
    base::Optional<std::vector<uint8_t>> device_response) {
  if (!device_response) {
    std::move(callback_).Run(CtapDeviceResponseCode::kCtap2ErrOther,
                             base::nullopt);
    return;
  }

  std::move(callback_).Run(GetResponseCode(*device_response),
                           ReadCTAPMakeCredentialResponse(*device_response));
}

}  // namespace device
