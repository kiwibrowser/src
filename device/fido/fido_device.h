// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef DEVICE_FIDO_FIDO_DEVICE_H_
#define DEVICE_FIDO_FIDO_DEVICE_H_

#include <stdint.h>

#include <string>
#include <vector>

#include "base/callback.h"
#include "base/component_export.h"
#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "base/optional.h"
#include "device/fido/authenticator_get_info_response.h"
#include "device/fido/fido_constants.h"

namespace device {

// Device abstraction for an individual CTAP1.0/CTAP2.0 device.
class COMPONENT_EXPORT(DEVICE_FIDO) FidoDevice {
 public:
  using WinkCallback = base::OnceClosure;
  using DeviceCallback =
      base::OnceCallback<void(base::Optional<std::vector<uint8_t>>)>;

  // Internal state machine states.
  enum class State { kInit, kConnected, kBusy, kReady, kDeviceError };

  FidoDevice();
  virtual ~FidoDevice();
  // Pure virtual function defined by each device type, implementing
  // the device communication transaction. The function must not immediately
  // call (i.e. hairpin) |callback|.
  virtual void DeviceTransact(std::vector<uint8_t> command,
                              DeviceCallback callback) = 0;
  virtual void TryWink(WinkCallback callback) = 0;
  virtual void Cancel() = 0;
  virtual std::string GetId() const = 0;

  void SetDeviceInfo(AuthenticatorGetInfoResponse device_info);
  void set_supported_protocol(ProtocolVersion supported_protocol) {
    supported_protocol_ = supported_protocol;
  }
  void set_state(State state) { state_ = state; }

  ProtocolVersion supported_protocol() const { return supported_protocol_; }
  const base::Optional<AuthenticatorGetInfoResponse>& device_info() const {
    return device_info_;
  }
  State state() const { return state_; }

 protected:
  virtual base::WeakPtr<FidoDevice> GetWeakPtr() = 0;

  State state_ = State::kInit;
  ProtocolVersion supported_protocol_ = ProtocolVersion::kUnknown;
  base::Optional<AuthenticatorGetInfoResponse> device_info_;

  DISALLOW_COPY_AND_ASSIGN(FidoDevice);
};

}  // namespace device

#endif  // DEVICE_FIDO_FIDO_DEVICE_H_
