// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef DEVICE_BLUETOOTH_BLUETOOTH_DEVICE_WINRT_H_
#define DEVICE_BLUETOOTH_BLUETOOTH_DEVICE_WINRT_H_

#include <string>

#include "base/callback_forward.h"
#include "base/macros.h"
#include "base/optional.h"
#include "device/bluetooth/bluetooth_device.h"
#include "device/bluetooth/bluetooth_export.h"

namespace device {

class BluetoothAdapterWinrt;

class DEVICE_BLUETOOTH_EXPORT BluetoothDeviceWinrt : public BluetoothDevice {
 public:
  explicit BluetoothDeviceWinrt(BluetoothAdapterWinrt* adapter);
  ~BluetoothDeviceWinrt() override;

  // BluetoothDevice:
  uint32_t GetBluetoothClass() const override;
  std::string GetAddress() const override;
  VendorIDSource GetVendorIDSource() const override;
  uint16_t GetVendorID() const override;
  uint16_t GetProductID() const override;
  uint16_t GetDeviceID() const override;
  uint16_t GetAppearance() const override;
  base::Optional<std::string> GetName() const override;
  bool IsPaired() const override;
  bool IsConnected() const override;
  bool IsGattConnected() const override;
  bool IsConnectable() const override;
  bool IsConnecting() const override;
  bool ExpectingPinCode() const override;
  bool ExpectingPasskey() const override;
  bool ExpectingConfirmation() const override;
  void GetConnectionInfo(const ConnectionInfoCallback& callback) override;
  void SetConnectionLatency(ConnectionLatency connection_latency,
                            const base::Closure& callback,
                            const ErrorCallback& error_callback) override;
  void Connect(PairingDelegate* pairing_delegate,
               const base::Closure& callback,
               const ConnectErrorCallback& error_callback) override;
  void SetPinCode(const std::string& pincode) override;
  void SetPasskey(uint32_t passkey) override;
  void ConfirmPairing() override;
  void RejectPairing() override;
  void CancelPairing() override;
  void Disconnect(const base::Closure& callback,
                  const ErrorCallback& error_callback) override;
  void Forget(const base::Closure& callback,
              const ErrorCallback& error_callback) override;
  void ConnectToService(
      const BluetoothUUID& uuid,
      const ConnectToServiceCallback& callback,
      const ConnectToServiceErrorCallback& error_callback) override;
  void ConnectToServiceInsecurely(
      const device::BluetoothUUID& uuid,
      const ConnectToServiceCallback& callback,
      const ConnectToServiceErrorCallback& error_callback) override;

 protected:
  // BluetoothDevice:
  void CreateGattConnectionImpl() override;
  void DisconnectGatt() override;

  DISALLOW_COPY_AND_ASSIGN(BluetoothDeviceWinrt);
};

}  // namespace device

#endif  // DEVICE_BLUETOOTH_BLUETOOTH_DEVICE_WINRT_H_
