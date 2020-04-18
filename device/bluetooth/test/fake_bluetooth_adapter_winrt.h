// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef DEVICE_BLUETOOTH_TEST_FAKE_BLUETOOTH_ADAPTER_WINRT_H_
#define DEVICE_BLUETOOTH_TEST_FAKE_BLUETOOTH_ADAPTER_WINRT_H_

#include <windows.devices.bluetooth.h>
#include <windows.foundation.h>
#include <wrl/client.h>
#include <wrl/implements.h>

#include <stdint.h>

#include "base/macros.h"
#include "base/strings/string_piece_forward.h"

namespace device {

class FakeBluetoothAdapterWinrt
    : public Microsoft::WRL::RuntimeClass<
          Microsoft::WRL::RuntimeClassFlags<
              Microsoft::WRL::WinRt | Microsoft::WRL::InhibitRoOriginateError>,
          ABI::Windows::Devices::Bluetooth::IBluetoothAdapter> {
 public:
  explicit FakeBluetoothAdapterWinrt(base::StringPiece address);
  ~FakeBluetoothAdapterWinrt() override;

  // IBluetoothAdapter:
  IFACEMETHODIMP get_DeviceId(HSTRING* value) override;
  IFACEMETHODIMP get_BluetoothAddress(UINT64* value) override;
  IFACEMETHODIMP get_IsClassicSupported(boolean* value) override;
  IFACEMETHODIMP get_IsLowEnergySupported(boolean* value) override;
  IFACEMETHODIMP
  get_IsPeripheralRoleSupported(boolean* value) override;
  IFACEMETHODIMP get_IsCentralRoleSupported(boolean* value) override;
  IFACEMETHODIMP
  get_IsAdvertisementOffloadSupported(boolean* value) override;
  IFACEMETHODIMP
  GetRadioAsync(ABI::Windows::Foundation::IAsyncOperation<
                ABI::Windows::Devices::Radios::Radio*>** operation) override;

 private:
  uint64_t raw_address_;

  DISALLOW_COPY_AND_ASSIGN(FakeBluetoothAdapterWinrt);
};

class FakeBluetoothAdapterStaticsWinrt
    : public Microsoft::WRL::RuntimeClass<
          Microsoft::WRL::RuntimeClassFlags<
              Microsoft::WRL::WinRt | Microsoft::WRL::InhibitRoOriginateError>,
          ABI::Windows::Devices::Bluetooth::IBluetoothAdapterStatics> {
 public:
  explicit FakeBluetoothAdapterStaticsWinrt(
      Microsoft::WRL::ComPtr<
          ABI::Windows::Devices::Bluetooth::IBluetoothAdapter> default_adapter);
  ~FakeBluetoothAdapterStaticsWinrt() override;

  // IBluetoothAdapterStatics:
  IFACEMETHODIMP GetDeviceSelector(HSTRING* result) override;
  IFACEMETHODIMP FromIdAsync(
      HSTRING device_id,
      ABI::Windows::Foundation::IAsyncOperation<
          ABI::Windows::Devices::Bluetooth::BluetoothAdapter*>** operation)
      override;
  IFACEMETHODIMP GetDefaultAsync(
      ABI::Windows::Foundation::IAsyncOperation<
          ABI::Windows::Devices::Bluetooth::BluetoothAdapter*>** operation)
      override;

 private:
  Microsoft::WRL::ComPtr<ABI::Windows::Devices::Bluetooth::IBluetoothAdapter>
      default_adapter_;

  DISALLOW_COPY_AND_ASSIGN(FakeBluetoothAdapterStaticsWinrt);
};

}  // namespace device

#endif  // DEVICE_BLUETOOTH_TEST_FAKE_BLUETOOTH_ADAPTER_WINRT_H_
