// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef DEVICE_BLUETOOTH_BLUETOOTH_ADAPTER_WIN_H_
#define DEVICE_BLUETOOTH_BLUETOOTH_ADAPTER_WIN_H_

#include <stddef.h>

#include <memory>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "base/containers/hash_tables.h"
#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/memory/weak_ptr.h"
#include "base/threading/thread_checker.h"
#include "device/bluetooth/bluetooth_adapter.h"
#include "device/bluetooth/bluetooth_discovery_session.h"
#include "device/bluetooth/bluetooth_export.h"
#include "device/bluetooth/bluetooth_task_manager_win.h"

namespace device {

class BluetoothAdapterWinTest;
class BluetoothDevice;
class BluetoothSocketThread;

class DEVICE_BLUETOOTH_EXPORT BluetoothAdapterWin
    : public BluetoothAdapter,
      public BluetoothTaskManagerWin::Observer {
 public:
  static base::WeakPtr<BluetoothAdapter> CreateAdapter(
      InitCallback init_callback);

  static bool UseNewBLEWinImplementation();

  // BluetoothAdapter:
  std::string GetAddress() const override;
  std::string GetName() const override;
  void SetName(const std::string& name,
               const base::Closure& callback,
               const ErrorCallback& error_callback) override;
  bool IsInitialized() const override;
  bool IsPresent() const override;
  bool IsPowered() const override;
  void SetPowered(bool discoverable,
                  const base::Closure& callback,
                  const ErrorCallback& error_callback) override;
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

  // BluetoothTaskManagerWin::Observer override
  void AdapterStateChanged(
      const BluetoothTaskManagerWin::AdapterState& state) override;
  void DiscoveryStarted(bool success) override;
  void DiscoveryStopped() override;
  void DevicesPolled(
      const std::vector<std::unique_ptr<BluetoothTaskManagerWin::DeviceState>>&
          devices) override;

  const scoped_refptr<base::SingleThreadTaskRunner>& ui_task_runner() const {
    return ui_task_runner_;
  }
  const scoped_refptr<BluetoothSocketThread>& socket_thread() const {
    return socket_thread_;
  }

  scoped_refptr<BluetoothTaskManagerWin> GetWinBluetoothTaskManager() {
    return task_manager_;
  }

 protected:
  // BluetoothAdapter:
  void RemovePairingDelegateInternal(
      device::BluetoothDevice::PairingDelegate* pairing_delegate) override;

 private:
  friend class BluetoothAdapterWinTest;
  friend class BluetoothTestWin;

  enum DiscoveryStatus {
    NOT_DISCOVERING,
    DISCOVERY_STARTING,
    DISCOVERING,
    DISCOVERY_STOPPING
  };

  explicit BluetoothAdapterWin(InitCallback init_callback);
  ~BluetoothAdapterWin() override;

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

  void Init();
  void InitForTest(
      scoped_refptr<base::SingleThreadTaskRunner> ui_task_runner,
      scoped_refptr<base::SequencedTaskRunner> bluetooth_task_runner);

  void MaybePostStartDiscoveryTask();
  void MaybePostStopDiscoveryTask();

  InitCallback init_callback_;
  std::string address_;
  std::string name_;
  bool initialized_;
  bool powered_;
  DiscoveryStatus discovery_status_;
  base::hash_set<std::string> discovered_devices_;

  std::vector<std::pair<base::Closure, DiscoverySessionErrorCallback>>
      on_start_discovery_callbacks_;
  std::vector<base::Closure> on_stop_discovery_callbacks_;
  size_t num_discovery_listeners_;

  scoped_refptr<BluetoothSocketThread> socket_thread_;
  scoped_refptr<BluetoothTaskManagerWin> task_manager_;

  base::ThreadChecker thread_checker_;

  // Flag indicating a device update must be forced in DevicesPolled.
  bool force_update_device_for_test_;

  // NOTE: This should remain the last member so it'll be destroyed and
  // invalidate its weak pointers before any other members are destroyed.
  base::WeakPtrFactory<BluetoothAdapterWin> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(BluetoothAdapterWin);
};

}  // namespace device

#endif  // DEVICE_BLUETOOTH_BLUETOOTH_ADAPTER_WIN_H_
