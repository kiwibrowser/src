// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef DEVICE_BLUETOOTH_BLUETOOTH_ADAPTER_WINRT_H_
#define DEVICE_BLUETOOTH_BLUETOOTH_ADAPTER_WINRT_H_

#include <windows.devices.bluetooth.h>
#include <windows.devices.enumeration.h>
#include <wrl/client.h>

#include <memory>
#include <string>

#include "base/callback_forward.h"
#include "base/macros.h"
#include "base/threading/thread_checker.h"
#include "device/bluetooth/bluetooth_adapter.h"
#include "device/bluetooth/bluetooth_export.h"

namespace base {
class ScopedClosureRunner;
}

namespace device {

class DEVICE_BLUETOOTH_EXPORT BluetoothAdapterWinrt : public BluetoothAdapter {
 public:
  // BluetoothAdapter:
  std::string GetAddress() const override;
  std::string GetName() const override;
  void SetName(const std::string& name,
               const base::Closure& callback,
               const ErrorCallback& error_callback) override;
  bool IsInitialized() const override;
  bool IsPresent() const override;
  bool IsPowered() const override;
  bool IsDiscoverable() const override;
  void SetDiscoverable(bool discoverable,
                       const base::Closure& callback,
                       const ErrorCallback& error_callback) override;
  bool IsDiscovering() const override;
  UUIDList GetUUIDs() const override;
  void CreateRfcommService(
      const BluetoothUUID& uuid,
      const ServiceOptions& options,
      const CreateServiceCallback& callback,
      const CreateServiceErrorCallback& error_callback) override;
  void CreateL2capService(
      const BluetoothUUID& uuid,
      const ServiceOptions& options,
      const CreateServiceCallback& callback,
      const CreateServiceErrorCallback& error_callback) override;
  void RegisterAdvertisement(
      std::unique_ptr<BluetoothAdvertisement::Data> advertisement_data,
      const CreateAdvertisementCallback& callback,
      const AdvertisementErrorCallback& error_callback) override;
  BluetoothLocalGattService* GetGattService(
      const std::string& identifier) const override;

 protected:
  friend class BluetoothAdapterWin;
  friend class BluetoothTestWin;

  BluetoothAdapterWinrt();
  ~BluetoothAdapterWinrt() override;

  void Init(InitCallback init_cb);

  // BluetoothAdapter:
  bool SetPoweredImpl(bool powered) override;
  void AddDiscoverySession(
      BluetoothDiscoveryFilter* discovery_filter,
      const base::Closure& callback,
      DiscoverySessionErrorCallback error_callback) override;
  void RemoveDiscoverySession(
      BluetoothDiscoveryFilter* discovery_filter,
      const base::Closure& callback,
      DiscoverySessionErrorCallback error_callback) override;
  void SetDiscoveryFilter(
      std::unique_ptr<BluetoothDiscoveryFilter> discovery_filter,
      const base::Closure& callback,
      DiscoverySessionErrorCallback error_callback) override;
  void RemovePairingDelegateInternal(
      BluetoothDevice::PairingDelegate* pairing_delegate) override;

  // These are declared virtual so that they can be overridden by tests.
  virtual HRESULT GetBluetoothAdapterStaticsActivationFactory(
      ABI::Windows::Devices::Bluetooth::IBluetoothAdapterStatics** statics)
      const;
  virtual HRESULT GetDeviceInformationStaticsActivationFactory(
      ABI::Windows::Devices::Enumeration::IDeviceInformationStatics** statics)
      const;

 private:
  void OnGetDefaultAdapter(
      base::ScopedClosureRunner on_init,
      Microsoft::WRL::ComPtr<
          ABI::Windows::Devices::Bluetooth::IBluetoothAdapter> adapter);

  void OnCreateFromIdAsync(
      base::ScopedClosureRunner on_init,
      Microsoft::WRL::ComPtr<
          ABI::Windows::Devices::Enumeration::IDeviceInformation>
          device_information);

  bool is_initialized_ = false;
  bool is_present_ = false;
  std::string address_;
  std::string name_;

  THREAD_CHECKER(thread_checker_);

  // Note: This should remain the last member so it'll be destroyed and
  // invalidate its weak pointers before any other members are destroyed.
  base::WeakPtrFactory<BluetoothAdapterWinrt> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(BluetoothAdapterWinrt);
};

}  // namespace device

#endif  // DEVICE_BLUETOOTH_BLUETOOTH_ADAPTER_WINRT_H_
