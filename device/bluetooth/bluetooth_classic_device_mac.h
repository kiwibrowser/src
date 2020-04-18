// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef DEVICE_BLUETOOTH_BLUETOOTH_CLASSIC_DEVICE_MAC_H_
#define DEVICE_BLUETOOTH_BLUETOOTH_CLASSIC_DEVICE_MAC_H_

#import <IOBluetooth/IOBluetooth.h>
#include <stdint.h>

#include <string>

#include "base/mac/scoped_nsobject.h"
#include "base/macros.h"
#include "base/observer_list.h"
#include "base/time/time.h"
#include "device/bluetooth/bluetooth_device_mac.h"

@class IOBluetoothDevice;

namespace device {

class BluetoothAdapterMac;

class BluetoothClassicDeviceMac : public BluetoothDeviceMac {
 public:
  explicit BluetoothClassicDeviceMac(BluetoothAdapterMac* adapter,
                                     IOBluetoothDevice* device);
  ~BluetoothClassicDeviceMac() override;

  // BluetoothDevice override
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
  UUIDSet GetUUIDs() const override;
  base::Optional<int8_t> GetInquiryRSSI() const override;
  base::Optional<int8_t> GetInquiryTxPower() const override;
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
      const BluetoothUUID& uuid,
      const ConnectToServiceCallback& callback,
      const ConnectToServiceErrorCallback& error_callback) override;
  void CreateGattConnection(
      const GattConnectionCallback& callback,
      const ConnectErrorCallback& error_callback) override;

  base::Time GetLastUpdateTime() const override;

  // Returns the Bluetooth address for the |device|. The returned address has a
  // normalized format (see below).
  static std::string GetDeviceAddress(IOBluetoothDevice* device);

 protected:
  // BluetoothDevice override
  void CreateGattConnectionImpl() override;
  void DisconnectGatt() override;

 private:
  friend class BluetoothAdapterMac;

  // Implementation to read the host's transmit power level of type
  // |power_level_type|.
  int GetHostTransmitPower(
      BluetoothHCITransmitPowerLevelType power_level_type) const;

  base::scoped_nsobject<IOBluetoothDevice> device_;

  DISALLOW_COPY_AND_ASSIGN(BluetoothClassicDeviceMac);
};

}  // namespace device

#endif  // DEVICE_BLUETOOTH_BLUETOOTH_CLASSIC_DEVICE_MAC_H_
