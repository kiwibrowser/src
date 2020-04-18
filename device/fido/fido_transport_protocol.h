// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef DEVICE_FIDO_FIDO_TRANSPORT_PROTOCOL_H_
#define DEVICE_FIDO_FIDO_TRANSPORT_PROTOCOL_H_

namespace device {

// This enum represents the transport protocols over which Fido WebAuthN API is
// currently supported.
enum class FidoTransportProtocol {
  kUsbHumanInterfaceDevice,
  kNearFieldCommunication,
  kBluetoothLowEnergy,
  kCloudAssistedBluetoothLowEnergy,
  kInternal,
};

}  // namespace device

#endif  // DEVICE_FIDO_FIDO_TRANSPORT_PROTOCOL_H_
