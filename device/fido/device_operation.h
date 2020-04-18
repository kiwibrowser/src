// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef DEVICE_FIDO_DEVICE_OPERATION_H_
#define DEVICE_FIDO_DEVICE_OPERATION_H_

#include <stdint.h>

#include <utility>
#include <vector>

#include "base/macros.h"
#include "base/optional.h"
#include "device/fido/authenticator_get_assertion_response.h"
#include "device/fido/authenticator_make_credential_response.h"
#include "device/fido/ctap_get_assertion_request.h"
#include "device/fido/ctap_make_credential_request.h"
#include "device/fido/fido_constants.h"
#include "device/fido/fido_device.h"

namespace device {

template <class Request, class Response>
class DeviceOperation {
 public:
  using DeviceResponseCallback =
      base::OnceCallback<void(CtapDeviceResponseCode,
                              base::Optional<Response>)>;

  DeviceOperation(FidoDevice* device, DeviceResponseCallback callback)
      : device_(device), callback_(std::move(callback)) {}
  virtual ~DeviceOperation() = default;

  virtual void Start() = 0;

 protected:
  FidoDevice* const device_ = nullptr;
  DeviceResponseCallback callback_;

  DISALLOW_COPY_AND_ASSIGN(DeviceOperation);
};

}  // namespace device

#endif  // DEVICE_FIDO_DEVICE_OPERATION_H_
